/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------------------------
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   4/21/89   Greg B.    1. Corrected alogrithm in ra_initacqparms()
|				to to handle both il='y' & 'n'.
|   4/28/89   Greg B.    1. More fixes to alogrithm in ra_initacqparms()
|  			    Important Note: alogrithm will not work 
|			     when il=f# is valid entry !!!!
|
|   6/29/89   Greg B.    1. Use new global parameters to initial lc & 
|			    calc.  acode offsets 
|
+------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include "mfileObj.h"
#include "variables.h"
#include "data.h"
#include "group.h"
#include "acodes.h"
#include "rfconst.h"
#include "abort.h"
#include "acqparms.h"
#include "dsp.h"
#include "pvars.h"

/*  PSG_LC  conditional compiles lc.h properly for PSG */
#ifndef  PSG_LC
#define  PSG_LC
#endif
#include "lc.h"
#include "lc_index.h"
#include "shrexpinfo.h"

#define PRTLEVEL 1
#ifdef  DEBUG
#define DPRINT(level, str) \
        if (bgflag >= level) fprintf(stderr,str)
#define DPRINT1(level, str, arg1) \
        if (bgflag >= level) fprintf(stderr,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
        if (bgflag >= level) fprintf(stderr,str,arg1,arg2)
#define DPRINT3(level, str, arg1, arg2, arg3) \
        if (bgflag >= level) fprintf(stderr,str,arg1,arg2,arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (bgflag >= level) fprintf(stderr,str,arg1,arg2,arg3,arg4)
#else
#define DPRINT(level, str)
#define DPRINT1(level, str, arg2)
#define DPRINT2(level, str, arg1, arg2)
#define DPRINT3(level, str, arg1, arg2, arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4)
#endif


extern double parmult();
extern double getval();
extern double sign_add();	/* add roundoff according to sign */
extern long acqpar_seek();
extern int putcode();
/*extern autodata autostruct; */

extern int clr_at_blksize_mode;
extern int lockfid_mode;
extern int bgflag;
/*extern int dsp_info[];*/
extern long rt_tab[];
extern int newacq;
extern int      acqiflag;	/* for interactive acq. or not? */
extern char fileRFpattern[];
extern double relaxdelay;
extern double dtmmaxsum;

extern codeint ilssval,ilctss,ssval,ctss;

static int acqfd;
static struct datafilehead acqfileheader;
static struct datablockhead acqblockheader;
static acqparmsize = sizeof(autodata) + sizeof(Acqparams);
static char infopath[256];

static MFILE_ID ifile = NULL;	/* inova datafile for ra */
static char *savedoffsetAddr;   /* saved inova offset header */

#define AUTOLOC 1	/* offset into lc struct for autod struct */
#define ACODEB 11	/* offset to beginning of Acodes */
#define ACODEP 12

#define RRI_SHIMSET	5	/* shimset number for RRI Shims */

extern unsigned long start_elem;     /* elem (FID) for acquisition to start on (RA)*/
extern unsigned long completed_elem; /* total number of completed elements (FIDs)  (RA) */

extern int num_tables;		/* number of tables for acodes */

extern Acqparams *Alc;
SHR_EXP_STRUCT ExpInfo;
static long max_ct;
static long ss2val;

double psync;
double ldshimdelay,oneshimdelay;
int ok2bumpflag;
double dsposskipdelay=0.0;

/*-----------------------------------------------------------------
|
|	initacqparms()
|	initialize acquisition parameters pass in code section 
|
|   get experimental parameters and setup data pointers
|				Author Greg Brissey 6/26/86
|   Note: setup_parfile should be executed before this routine.
|   Modified   Author     Purpose
|   --------   ------     -------
|   2/10/89   Greg B.    1. Added Code to set low core element lc->acqelemid 
|   5/2/91    Greg B.    1. low core element lc->acqelemid is a codelong now
+---------------------------------------------------------------*/
initacqparms(fidn)
unsigned long fidn;
{
    char dp[MAXSTR];
    char cp[MAXSTR];
    char ok2bumpstr[MAXSTR];
    long np_words;	/* total # of data point words */
    codeint ss;		/* steady state count */
    codeint cttime;		/* #CTs between screen updates to Host */
    codeint dpflag;		/* double precision flag = 2 or 4 */
    codeint blocks;		/*  data size in blocks  (256words) */
    codeint asize;		/*  data size in blocks  (256words) */
    codelong tot_np;		/*  np size */
    codelong curct, bsct4ra;
    int i;
    int topword;
    int botword;
    int times;
    int bytes;
    int words;
    int fifotype;
    double tmpval;

    /* initialize lc */
    std_lc_init();

    /* configuration test word for acquisition to confirm */
    fifotype = (fifolpsize < 70) ? 0x0 : 0x0100;

    /* --- calc. CTs between updates of CT to Host CPU --- */
    if ( P_getreal(CURRENT,"cttime",&tmpval,1) < 0 )
    {
        tmpval = 0.0;                /* if not found assume 0 */
    }
    cttime = (int) (sign_add(tmpval, 0.0005));
    if (cttime < 1) 
	cttime = 5;	/* update every 5 secs */
    cttime = (int) ((double) cttime / (d1+ (np/sw) + 0.1));
    ss2val = 0;
    if (!var_active("ss",CURRENT))
    {
	ss = 0;
    }
    else
    {
        ss = (codeint) (sign_add(getval("ss"), 0.0005));
        if (ss >= 0)
        {
           if ( P_getreal(CURRENT,"ss2",&tmpval,1) >= 0 )
           {
              if (var_active("ss2",CURRENT))
              {
                 ss2val = sign_add(tmpval, 0.0005);
                 if (ss2val < 1)
                    ss2val = 0;
              }
           }
        }
    }
    if (ss != 0) 
        cttime = 0; 		/* no ct display with steady state
    else if ( cttime > 32767) 
        cttime = 32767;		/* max ct between updates */
    else if (cttime < 1)
        cttime = 1;
  
    /* --- completed transients (ct) --- */

    if ((P_getreal(CURRENT,"ct",&tmpval,1)) >= 0)
    {
       curct = (int) (tmpval + 0.0005);
    }
    else
    {   text_error("initacqparms(): cannot find ct.");
	psg_abort(1);
    }
    bsct4ra = (bs + 0.005);
    if (newacq)               /* test for newacq added at request of DJI, 08/1996 */
    {
        if (bsct4ra > 0)
	  bsct4ra = curct/bsct4ra;

    /* for ra, check nt and ct and interleaving */
    /* for ra, curct handled in  ra_initacqparms or ra_inovaacqparms. */

        if (curct == nt)
        {
	    if (getIlFlag())
	    {
	        if (getStartFidNum() > 1) bsct4ra = bsct4ra - 1;
	    }
	    else
	    {
	        bsct4ra = 0;
	    }
        }
        else
        {
	    if (getIlFlag())
	    {
	        if (getStartFidNum() > 1) bsct4ra = bsct4ra - 1;
	    }
        }
        if (bsct4ra < 0) bsct4ra = 0;
    }


    getstr("dp",dp);
    if (newacq)      /* test for newacq since U+ expects 0 or 4 not 2 or 4 08/1996 */
    {
      dpflag = (dp[0] == 'y') ? 4 : 2;	/* single - 2, double - 4  INOVA */
    }
    else
    {
      dpflag = (dp[0] == 'y') ? 4 : 0;	/* single - 0, double - 4   UNITY Plus */
    }

    /* Init maxsum */
    if ( P_getreal(CURRENT,"dtmmaxsum",&dtmmaxsum,1) < 0 )
    {
        dtmmaxsum = (double) 0x7ff00000;     /* if not found assume max */
	if (dpflag == 2)
           dtmmaxsum = (double) 0x7fff;     /* single precision max */
    }

    /* --- setup real time np with total data points to be take --- */
    /*     the STM will expect this many points    */
    /*     data pts * # of fids */
    tot_np = (codelong) ((long) (np + .0005)) * ((long) (nf + 0.005));

    /*if (dsp_info[0] == 0)*/
    if (dsp_params.il_oversamp > 1)
    {
       /* Inline DSP acquires more points so data block allocation is larger */
       /*tot_np *= (codelong) dsp_info[1];*/
       /*tot_np += (codelong) dsp_info[3];*/
       tot_np *= (codelong) dsp_params.il_oversamp;
       tot_np += (codelong) dsp_params.il_extrapts;
       /* Bypass hardware if present */
       asize = 1;
    }
    else
    {
       char tmpstr[256];
       /* Real-time DSP uses this for oversampling and decimation */
       /*asize = (dsp_info[1] & 0xff);*/
       asize = (dsp_params.rt_oversamp & 0xff);
       if (P_getstring(CURRENT, "osfilt", tmpstr, 1, 255) >= 0 )
       {
          if ( (tmpstr[0] == 'b') || (tmpstr[0] == 'B') )
             if ( (tmpstr[1] >= '1') && (tmpstr[1] <= '7') )
                asize |= ((tmpstr[1]-'0') << 8);
             else 
                asize |= (1 << 8);
       }
    }
    if (dpflag == 4)
       asize |= 0xC000;
    np_words = tot_np;
    if (dpflag == 4) 
        np_words *= 2L; /* double size for double precision data */

    if (bgflag)
	fprintf(stderr,"np: %lf, nf: %lf , asize = %x\n",np,nf,asize);
    blocks = (codeint) ((np_words + 255L) / 256L);  /* block = 256 words */

    getstr("cp",cp);
    cpflag = (cp[0] == 'y') ? 0 : 1;

    if ( P_getreal(CURRENT,"mxconst",&tmpval,1) < 0 )
    {
        tmpval = 0.0;                /* if not found assume 0 */
    }

    /* Get relaxation delay for acqi and interelement delay */
    if (option_check("qtune"))
    {
        relaxdelay = -1.0;
    }
    else if ( P_getreal(CURRENT,"relaxdelay",&relaxdelay,1) < 0 )
    {
	if (acqiflag)
           relaxdelay  = 0.020;		/* if not found assume 20 millisecs */
	else
           relaxdelay  = 0.0;		/* if not found assume 0 */
    }
    else if (relaxdelay > 2.0)
    {
	text_error("relaxdelay truncated to maximum relaxdelay: 2 sec");
	relaxdelay = 2.0;
    }

    /* Get parser synchronization delay, for testing  */
    if ( P_getreal(CURRENT,"psync",&psync,1) < 0 )
    {
           psync  = 0.020;		/* if not found assume 20 millisecs */
    }

    /* Get parser synchronization delay, for testing  */
    if ( P_getreal(CURRENT,"ldshimdelay",&ldshimdelay,1) < 0 )
    {
      if ( P_getreal(GLOBAL,"ldshimdelay",&ldshimdelay,1) < 0 )
      {
	double tmpshimset;
	ldshimdelay  = 4.5; /* if not found assume 4.5 sec */
	if ( P_getreal(GLOBAL,"shimset",&tmpshimset,1) < 0 )
	{
	   tmpshimset = 1.0;
	}
	if ((int)(tmpshimset) == RRI_SHIMSET)
	   ldshimdelay  = 15.0; /* 15 secs to load RRI shims */
      }
    }
    /* Get parser synchronization delay, for one shim for testing  */
    if ( P_getreal(CURRENT,"oneshimdelay",&oneshimdelay,1) < 0 )
    {
      if ( P_getreal(GLOBAL,"oneshimdelay",&oneshimdelay,1) < 0 )
      {
	double tmpshimset;
	oneshimdelay  = 0.080; /* if not found assume 0.08 for one shim */
	if ( P_getreal(GLOBAL,"shimset",&tmpshimset,1) < 0 )
	{
	   tmpshimset = 1.0;
	}
	if ((int)(tmpshimset) == RRI_SHIMSET)
	   oneshimdelay  = 0.3; /* 0.3 secs to load one RRI shim */
      }
    }

    ok2bumpflag = 0;
    if (P_getstring(GLOBAL, "ok2bump", ok2bumpstr, 1, sizeof( ok2bumpstr - 1 )) >= 0 )
    {
	if (ok2bumpstr[ 0 ] == 'Y' || ok2bumpstr[ 0 ] == 'y')
	  ok2bumpflag = 1;
    }

    xmtrstep = 90.0;
    decstep = 90.0;
    custom_lc_init(
    /* Alc->acqidver    */ (codeint) ((idc & 0x00ff) | fifotype),
    /* Alc->acqelemid   */ (codeulong) fidn,
    /* Alc->acqctctr    */ (codeint) cttime,
    /* Alc->acqdsize    */ (codeint) blocks,
    /* Alc->acqnp       */ (codelong) tot_np,
    /* Alc->acqnt       */ (codelong) (nt + 0.0001),
    /* Alc->acqdpf      */ (codeint) dpflag,
    /* Alc->acqbs       */ (codeint) (bs + 0.005),
    /* Alc->acqbsct     */ (codeint) bsct4ra,
    /* Alc->acqss       */ (codeint) ss,
    /* Alc->acqasize    */ (codeint) asize,
    /* Alc->acqcpf      */ (codeint) cpflag,
    /* Alc->acqmaxconst */ (codeint) (sign_add(tmpval,0.005)),
    /* arraydim         */ (codeulong) (ExpInfo.NumAcodes),
    /* relaxdelay	*/ (codeulong) (relaxdelay*1e8),
    /* Alc->acqct	*/ (codelong) curct,
    /* Alc->clrbsflag	*/ (codeint) clr_at_blksize_mode,
    /* lockflag		*/ (codeint) lockfid_mode
       );
}

/*-----------------------------------------------------------------
|
|   convertdbl()/3
|   convert a double value into two integers.
|	 A top word and  a bottom word 
|   (e.g., 131572.0 ->   topword: 2	(131572.0 / 65536.0)
|			 botword: 500   ( remainder from above ).
|	   131572 = 2 * 65536  + 500
|
|				Author Greg Brissey 6/26/86
+------------------------------------------------------------------*/
convertdbl(value,topint,botint)
double value;
int *topint;
int *botint;
{
    long long val;
    int top;
    int bot;

    if (value < 0.0) value = -value;
    val = (long long) (value + 0.0005);
    top = val / 65536;
    bot = val % 65536;
    if (bgflag)
	fprintf(stderr,"convertdbl(): value: %lf top: %ld bot: %ld \n",
		value,top,bot); 
    *topint = top;
    *botint = bot;
}
/*-----------------------------------------------------------------
|
|	ra_initacqparms(fid#)
|	update and fill in lc parameters from exp#/acqfil/acqpar 
|       from previous go.
|				Author Greg Brissey 8/01/88
|   Modified   Author     Purpose
|   --------   ------     -------
|   2/10/89   Greg B.    1. Added alogrithm to calc. start_elem and 
|			    complete_elem for ra. 
|   4/21/89   Greg B.    1. Corrected alogrithm to to handle both 
|				il='y' & 'n'.
|   4/28/89   Greg B.    1. More fixes to alogrithm
|  			    Important Note: alogrithm will not work 
|			     when il=f# is valid entry !!!!
|   2/22/90   Greg B.    1. return 0 if no blockheader data, return 1 if 
|			    blockheader data present.
+---------------------------------------------------------------*/
ra_initacqparms(fidn)
unsigned long fidn;   /* fid number to obtain the acqpar parameters for */
{
   Acqparams *lc,*ra_lc;
   autodata *autoptr;
   char msge[256];
   int len1,len2;
   codeint acqbuffer[512];
   struct datablockhead acqblockheader;

   if (fidn == 1)
   {
     max_ct = 0L;
   }

   acqpar_seek(fidn);	/* move to proper fid entry in acqpar */
   len1 = read(acqfd,&acqblockheader,sizeof(struct datablockhead));
   len2 = read(acqfd,acqbuffer,acqparmsize);
   DPRINT3(PRTLEVEL,"ra_init: fid %lu, blockhdr %d, lc & auto %d\n",
		fidn,len1,len2);

   if ( (len1 == 0) || (len2 == 0) )
   {
      if (max_ct != -1L)  /* no start selected, 1st none acquire fid is start */
      {
        start_elem = fidn;
        max_ct = -1L;           /* only set start_elem once */
        DPRINT3(PRTLEVEL,"fid: %lu, start_elem = %lu, max_ct = %d  \n",
	  fidn,start_elem,max_ct);
      }
      return(0);	/* no further data */
   }
 
   ra_lc = (Acqparams *) acqbuffer; /* ra lc parameters */
   lc = (Acqparams *) lc_stadr; /* initialize lc parameters */

   DPRINT2(PRTLEVEL,"lc-> 0x%lx, ra_lc-> 0x%lx\n",lc,ra_lc);
   DPRINT2(PRTLEVEL,"ct=%ld, ra ct=%ld\n",lc->acqct,ra_lc->acqct);
   DPRINT2(PRTLEVEL,"np=%ld, ra np=%ld\n",lc->acqnp,ra_lc->acqnp);
   DPRINT2(PRTLEVEL,"nt=%ld, ra nt=%ld\n",lc->acqnt,ra_lc->acqnt);

   /* find the maximum ct acquired without actually completing the FID */
   /* the assumption here is that fids are acquired 1 to arraydim */
   /* i.e., 1st fid ct>0 is max ct acquired so far */
   if ( (ra_lc->acqct != ra_lc->acqnt) && (max_ct == 0L) )
   {
      max_ct = ra_lc->acqct;
      start_elem = fidn;        /* maybe start_elem */
   }

   DPRINT3(PRTLEVEL,"fid: %lu, start_elem = %lu, max_ct = %d  \n",
	fidn,start_elem,max_ct);
 
   DPRINT1(PRTLEVEL,"il='%s'\n",il);
   if ( (max_ct != 0L) && (max_ct != -1L) && (ra_lc->acqct != ra_lc->acqnt) )
   {
     /* first elem with bs!=bsct or ct < max_ct , if il!='n' is the start_elem */
     if ( (il[0] != 'n') && (il[0] != 'N') )
     {
       if ( (ra_lc->acqbsct != ra_lc->acqbs) || (ra_lc->acqct < max_ct) )
       {
          start_elem = fidn;
          max_ct = -1L;           /* only set start_elem once */
       }
     }
     else
     {
       start_elem = fidn;
       max_ct = -1L;           /* only set start_elem once */
     }
   }

   DPRINT3(PRTLEVEL,"fid: %lu, start_elem = %lu, max_ct = %d  \n",
	fidn,start_elem,max_ct);
 

   if (ra_lc->acqct == ra_lc->acqnt)
   {
      lc->acqnt = ra_lc->acqnt; /* nt can not be change for completed fids */
      completed_elem++;         /* total number of completed elements (FIDs)  (RA) */
   }
   else if ( lc->acqnt <= ra_lc->acqct )
    {
      lc->acqnt = ra_lc->acqnt; /* nt can not be made <= to ct , for now. */
      sprintf(msge,"WARNING: FID:%d  'nt' <= 'ct', original 'nt' used.\n",
		fidn);
      text_error(msge);
    }
 
   DPRINT2(PRTLEVEL,"nt=%ld, ra nt=%ld\n",lc->acqnt,ra_lc->acqnt);
   if (ra_lc->acqnp != lc->acqnp)
   {
      sprintf(msge,"WARNING 'np' has changed from %ld to %ld.\n",
		ra_lc->acqnp,lc->acqnp);
      text_error(msge);
   }
   if (ra_lc->acqdpf != lc->acqdpf)
   {
      text_error("data precision changed, PSG aborting.");
      psg_abort(1);
   }
   lc->acqct = ra_lc->acqct;
   lc->acqisum = ra_lc->acqisum;
   lc->acqrsum = ra_lc->acqrsum;
   lc->acqstmar = ra_lc->acqstmar;
   lc->acqstmcr = ra_lc->acqstmcr;
   DPRINT2(PRTLEVEL,"maxscale %hd, ra %hd\n",lc->acqmaxscale,ra_lc->acqmaxscale);
   lc->acqmaxscale = ra_lc->acqmaxscale;
   lc->acqicmode = ra_lc->acqicmode;
   lc->acqstmchk = ra_lc->acqstmchk;
   lc->acqnflag = ra_lc->acqnflag;
   DPRINT2(PRTLEVEL,"scale %hd, ra %hd\n",lc->acqscale,ra_lc->acqscale);
   lc->acqscale = ra_lc->acqscale;
   lc->acqcheck = ra_lc->acqcheck;
   lc->acqoph = ra_lc->acqoph;
   lc->acqbsct = ra_lc->acqbsct;
   lc->acqssct = ra_lc->acqssct;
   lc->acqctcom = ra_lc->acqctcom;
   lc->acqtablert = ra_lc->acqtablert;
   lc->acqv1 = ra_lc->acqv1;
   lc->acqv2 = ra_lc->acqv2;
   lc->acqv3 = ra_lc->acqv3;
   lc->acqv4 = ra_lc->acqv4;
   lc->acqv5 = ra_lc->acqv5;
   lc->acqv6 = ra_lc->acqv6;
   lc->acqv7 = ra_lc->acqv7;
   lc->acqv8 = ra_lc->acqv8;
   lc->acqv9 = ra_lc->acqv9;
   lc->acqv10 = ra_lc->acqv10;
   lc->acqv11 = ra_lc->acqv11;
   lc->acqv12 = ra_lc->acqv12;
   lc->acqv13 = ra_lc->acqv13;
   lc->acqv14 = ra_lc->acqv14;

   return(1); /* data found and lc updated */
}
/*---------------------------------------------------------------------
|       set_counter()/
|
|       Sets ss counters for each element
+-------------------------------------------------------------------*/
set_counters()
{
    long ilsstmp, ilctsstmp;
    if (Alc->acqss < 0)
    {
       Alc->acqssct = -1 * Alc->acqss;
       ilsstmp = Alc->acqssct;
    }
    else if (ix == getStartFidNum())
    {
       Alc->acqssct = Alc->acqss;
       ilsstmp = ss2val;
    }
    else if (ss2val)
    {
       Alc->acqssct = ss2val;
       ilsstmp = ss2val;
    }
    else
    {
       Alc->acqssct = 0;
       ilsstmp = 0;
    }
    Alc->acqrtvptr = ( Alc->acqnt - (Alc->acqssct % Alc->acqnt) + Alc->acqct )
							 % Alc->acqnt;
    ilctsstmp = ( Alc->acqnt - (ilsstmp % Alc->acqnt) + Alc->acqct )
							 % Alc->acqnt;
    if (newacq)
    {
       init_acqvar(ilssval,ilsstmp);
       init_acqvar(ilctss,ilctsstmp);
    }
}

/*---------------------------------------------------------------------
|       ss4autoshim()/
|
|       Sets ss counters if autoshimming.
|	Only set if ss > 1 and the counters have not been set.
+-------------------------------------------------------------------*/
ss4autoshim()
{

  if (newacq)
  {
    if ((Alc->acqss > 0) && (ix != getStartFidNum()))
    {
       Alc->acqssct = Alc->acqss;
       Alc->acqrtvptr = (Alc->acqnt - (Alc->acqssct % Alc->acqnt) + Alc->acqct)
							 % Alc->acqnt;
       init_acqvar(ssval,(long)Alc->acqssct);
       init_acqvar(ctss,(long)Alc->acqrtvptr);
    }
  }
}

/*---------------------------------------------------------------------
|       open_acqpar(path)/1
|
|       Position the disk read/write heads to the proper block offset
|       for the acqpar file (lc,auto structure parameters.
+-------------------------------------------------------------------*/
open_acqpar(filename)
char *filename;
{
   int len;
   if (bgflag)
     fprintf(stderr,"Opening Acqpar file: '%s' \n",filename);
   acqfd = open(filename,O_EXCL | O_RDONLY ,0666);
   if (acqfd < 0)
   { text_error("Cannot open acqpar file: '%s', RA not possible.\n",filename);
       psg_abort(1);
   }
   len = read(acqfd,&acqfileheader,sizeof(struct datafilehead));
   if (len < 1)
   { text_error("Cannot read acqpar file: '%s'\n",filename);
       psg_abort(1);
   }
      
}

/*---------------------------------------------------------------------
|       acqpar_seek(index)/1
|
|       Position the disk read/write heads to the proper block offset
|       for the acqpar file (lc,auto structure parameters.
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   2/10/89   Greg B.    1. Routine now returns the (long) position of seek
+-------------------------------------------------------------------*/
long acqpar_seek(elemindx)
unsigned long elemindx;
{
    int acqoffset;
    long pos;

    acqoffset = sizeof(acqfileheader) +
        (acqfileheader.bbytes * ((unsigned long) (elemindx - 1)))  ;
    if (bgflag)
        fprintf(stderr,"acqpar_seek(): fid# = %d,acqoffset = %d \n",
           elemindx,acqoffset);
    pos = lseek(acqfd, acqoffset,SEEK_SET);
    if (pos == -1)
    {
        char *str_err;
        if ( (str_err = strerror(errno) ) != NULL )
        {
           fprintf(stdout,"acqpar_seek Error: offset %ld,: %s\n",
               acqoffset,str_err);
        }
        return(-1);
    }    
    return(pos);
}

int setup_parfile(suflag)
int suflag;
{
    double tmp;
    char tmpstr[256];
    int t;
    char *ptr;

    ExpInfo.PSGident = 1;	/* identify as 'C' varient PSG, Java == 100 */

    if ((P_getreal(CURRENT,"priority",&tmp,1)) >= 0)
       ExpInfo.Priority = (int) (tmp + 0.0005);
    else
       ExpInfo.Priority = 0;

    if ((P_getreal(CURRENT,"nt",&tmp,1)) >= 0)
       ExpInfo.NumTrans = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set nt.");
	psg_abort(1);
    }

    if ((P_getreal(CURRENT,"bs",&tmp,1)) >= 0)
       ExpInfo.NumInBS = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set bs.");
	psg_abort(1);
    }
    if (!(var_active("bs",CURRENT)))
       ExpInfo.NumInBS = 0;

    if ((P_getreal(CURRENT,"np",&tmp,1)) >= 0)
       ExpInfo.NumDataPts = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set np.");
	psg_abort(1);
    }

    /* --- Number of FIDs per CT --- */

    if ((P_getreal(CURRENT,"nf",&tmp,1)) >= 0)
    { 
	DPRINT2(1,"initacqqueue(): nf = %5.0lf, active = %d \n",
				tmp,var_active("nf",CURRENT));
	if ( (tmp < 2.0 ) || (!(var_active("nf",CURRENT))) )
	{
	    tmp = 1.0;
	}
    }
    else  /* no nf, so set it to one.  */
    {
        tmp = 1.0;
    }
    ExpInfo.NumFids = (int) (tmp + 0.0005);

    if (P_getstring(CURRENT,"dp",tmpstr,1,4) < 0) 
    {   text_error("initacqqueue(): cannot get dp");
	psg_abort(1);
    }
    ExpInfo.DataPtSize = (tmpstr[0] == 'y') ? 4 : 2;

    /* --- receiver gain --- */

    if ((P_getreal(CURRENT,"gain",&tmp,1)) >= 0)
       ExpInfo.Gain = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set gain.");
	psg_abort(1);
    }

    /* --- sample spin rate --- */

    if ((P_getreal(CURRENT,"spin",&tmp,1)) >= 0)
       ExpInfo.Spin = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set spin.");
	psg_abort(1);
    }

    /* --- completed transients (ct) --- */

    if ((P_getreal(CURRENT,"ct",&tmp,1)) >= 0)
       ExpInfo.CurrentTran = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set ct.");
	psg_abort(1);
    }

    /* --- number of fids --- */

    if ((P_getreal(CURRENT,"arraydim",&tmp,1)) >= 0)
       ExpInfo.ArrayDim = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot read arraydim.");
	psg_abort(1);
    }

    if ((P_getreal(CURRENT,"acqcycles",&tmp,1)) >= 0)
	ExpInfo.NumAcodes = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot read acqcycles.");
	psg_abort(1);
    }

    ExpInfo.FidSize = ExpInfo.DataPtSize * ExpInfo.NumDataPts * ExpInfo.NumFids;
    ExpInfo.DataSize = sizeof(struct datafilehead);
    ExpInfo.DataSize +=  (unsigned long long) (sizeof(struct datablockhead) + ExpInfo.FidSize) *
                         (unsigned long long) ExpInfo.ArrayDim;

    /* --- path to the user's experiment work directory  --- */

    if (P_getstring(GLOBAL,"userdir",tmpstr,1,255) < 0) 
    {   text_error("initacqqueue(): cannot get userdir");
	psg_abort(1);
    }
    strcpy(ExpInfo.UsrDirFile,tmpstr);

    if (P_getstring(GLOBAL,"systemdir",tmpstr,1,255) < 0) 
    {   text_error("initacqqueue(): cannot get systemdir");
	psg_abort(1);
    }
    strcpy(ExpInfo.SysDirFile,tmpstr);

    if (P_getstring(GLOBAL,"curexp",tmpstr,1,255) < 0) 
    {   text_error("initacqqueue(): cannot get curexp");
	psg_abort(1);
    }
    strcpy(ExpInfo.CurExpFile,tmpstr);

    /* --- suflag					*/
    ExpInfo.GoFlag = suflag;

    /* -------------------------------------------------------------- 
    |      Unique name to this GO,
    |      vnmrsystem/acqqueue/id is path to acq proccess files
    |
    |      Notice that goid is an array of strings, with each
    |      element having a carefully defined meaning.
    +-----------------------------------------------------------------*/

    if (P_getstring(CURRENT,"goid",infopath,1,255) < 0) 
    {   text_error("initacqqueue(): cannot get goid");
	psg_abort(1);
    }
    strcpy(ExpInfo.Codefile,infopath);
    strcat(ExpInfo.Codefile,".Code");
    strcpy(ExpInfo.RTParmFile,infopath);
    strcat(ExpInfo.RTParmFile,".RTpars");
    strcpy(ExpInfo.TableFile,infopath);
    strcat(ExpInfo.TableFile,".Tables");
    ExpInfo.WaveFormFile[0] = '\0';
    ExpInfo.GradFile[0] = '\0';

    /* Beware that infopath gets accessed again
       if acqiflag is set, for the data file path */

    /* --- file path to named acqfile or exp# acqfile  'file' --- */

    if (!acqiflag)
    {
      int autoflag;
      char autopar[12];

      if (P_getstring(CURRENT,"exppath",tmpstr,1,255) < 0) 
      {   text_error("initacqqueue(): cannot get exppath");
	  psg_abort(1);
      }
      strcpy(ExpInfo.DataFile,tmpstr);
      ExpInfo.InteractiveFlag = 0;
      if (getparm("auto","string",GLOBAL,autopar,12))
          autoflag = 0;
      else
          autoflag = ((autopar[0] == 'y') || (autopar[0] == 'Y'));
      ExpInfo.ExpFlags = 0;
      if (autoflag)
      {
         strcat(ExpInfo.DataFile,".fid");
	 ExpInfo.ExpFlags |= AUTOMODE_BIT;  /* set automode bit */
      }
      if (ra_flag)
         ExpInfo.ExpFlags |=  RESUME_ACQ_BIT;  /* set RA bit */
      if (clr_at_blksize_mode)
	  ExpInfo.ExpFlags |=  CLR_AT_BS_BIT; /* For "Repeat Scan" */
    }
    else
    {
      strcpy(ExpInfo.DataFile,infopath);
      strcat(ExpInfo.DataFile,".Data");
      ExpInfo.InteractiveFlag = 1;
      ExpInfo.ExpFlags = 0;
    }

    if (P_getstring(CURRENT,"goid",tmpstr,2,255) < 0) 
    {   text_error("initacqqueue(): cannot get goid: user");
	psg_abort(1);
    }
    strcpy(ExpInfo.UserName,tmpstr);

    if (P_getstring(CURRENT,"goid",tmpstr,3,255) < 0) 
    {   text_error("initacqqueue(): cannot get goid: exp number");
	psg_abort(1);
    }
    ExpInfo.ExpNum = atoi(tmpstr);

    if (P_getstring(CURRENT,"goid",tmpstr,4,255) < 0) 
    {   text_error("initacqqueue(): cannot get goid: exp");
	psg_abort(1);
    }
    strcpy(ExpInfo.AcqBaseBufName,tmpstr);

    if (P_getstring(GLOBAL,"vnmraddr",tmpstr,1,255) < 0) 
    {   text_error("initacqqueue(): cannot get vnmraddr");
	psg_abort(1);
    }
    strcpy(ExpInfo.MachineID,tmpstr);

    /* --- interleave parameter 'il' --- */

    if (P_getstring(CURRENT,"il",tmpstr,1,4) < 0) 
    {   text_error("initacqqueue(): cannot get il");
	psg_abort(1);
    }
    ExpInfo.IlFlag = ((tmpstr[0] != 'n') && (tmpstr[0] != 'N')) ? 1 : 0;
    if (ExpInfo.IlFlag)
    {
	if (ExpInfo.NumAcodes <= 1) ExpInfo.IlFlag = 0;
	if (ExpInfo.NumInBS == 0) ExpInfo.IlFlag = 0;
	if (ExpInfo.NumTrans <= ExpInfo.NumInBS) ExpInfo.IlFlag = 0;
    }

    /* --- current element 'celem' --- */

    if ((P_getreal(CURRENT,"celem",&tmp,1)) >= 0)
       ExpInfo.Celem = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set celem.");
	psg_abort(1);
    }

    /* --- Check for valid RA --- */
    ExpInfo.RAFlag = 0;		/* RaFlag */
    if (ra_flag)
    {
    	/* --- Do RA stuff --- */
	ExpInfo.RAFlag = 1;		/* RaFlag */
	if (ExpInfo.IlFlag)
	{
	   if ((ExpInfo.CurrentTran % ExpInfo.NumInBS) != 0)
	   	ExpInfo.Celem = ExpInfo.Celem - 1;
	   else
	   {
		if ((ExpInfo.Celem < ExpInfo.NumAcodes) && 
			(ExpInfo.CurrentTran >= ExpInfo.NumInBS))
		   ExpInfo.CurrentTran = ExpInfo.CurrentTran - ExpInfo.NumInBS;
	   }
	}
	else
	{
    	   if ((ExpInfo.CurrentTran > 0) && (ExpInfo.CurrentTran < 
						ExpInfo.NumTrans))
	   	ExpInfo.Celem = ExpInfo.Celem - 1;
    	   if ((ExpInfo.CurrentTran == ExpInfo.NumTrans) &&
				(ExpInfo.Celem < ExpInfo.NumAcodes))
		   ExpInfo.CurrentTran = 0;
	}
    	if ((ExpInfo.Celem < 0) || (ExpInfo.Celem >= ExpInfo.NumAcodes))
	   ExpInfo.Celem = 0;
    	ExpInfo.CurrentElem = ExpInfo.Celem;
    	/* fprintf(stdout,"initacqparms: Celem = %d\n",ExpInfo.Celem); */
    }

    /* --- when_mask parameter  --- */

    if ((P_getreal(CURRENT,"when_mask",&tmp,1)) >= 0)
       ExpInfo.ProcMask = (int) (tmp + 0.0005);
    else
    {   text_error("initacqqueue(): cannot set when_mask.");
	psg_abort(1);
    }

    ExpInfo.ProcWait = (option_check("wait")) ? 1 : 0;
    ExpInfo.DspGainBits = 0;
    ExpInfo.DspOversamp = 0;
    ExpInfo.DspOsCoef = 0;
    ExpInfo.DspSw = 0.0;
    ExpInfo.DspFb = 0.0;
    ExpInfo.DspOsskippts = 0;
    ExpInfo.DspOslsfrq = 0.0;
    ExpInfo.DspFiltFile[0] = '\0';

    ExpInfo.UserUmask = umask(0);
    umask( ExpInfo.UserUmask );		/* make sure the process umask does not change */

    /* fill in the account info */
    strcpy(tmpstr,ExpInfo.SysDirFile);
    strcat(tmpstr,"/adm/accounting/acctLog.xml");
    if ( access(tmpstr,F_OK) != 0)
    {
       ExpInfo.Billing.enabled = 0;
    }
    else
    {
        ExpInfo.Billing.enabled = 1;
    }
        t = time(0);
        ExpInfo.Billing.submitTime = t;
        ExpInfo.Billing.startTime  = t;
        ExpInfo.Billing.doneTime   = t;
        if (P_getstring(GLOBAL, "operator", tmpstr, 1, 255) < 0)
           ExpInfo.Billing.Operator[0]='\000';
        else
           strncpy(ExpInfo.Billing.Operator,tmpstr,200);
        if (P_getstring(CURRENT, "account", tmpstr, 1, 255) < 0)
           ExpInfo.Billing.account[0]='\000';
        else
           strncpy(ExpInfo.Billing.account,tmpstr,200);
        if (P_getstring(CURRENT, "pslabel", tmpstr, 1, 255) < 0)
           ExpInfo.Billing.seqfil[0]='\000';
        else
           strncpy(ExpInfo.Billing.seqfil,tmpstr,200);
        ptr = strrchr(infopath,'/');
        if ( ptr )
        {
           ptr++;
           strncpy(ExpInfo.Billing.goID, ptr ,200);
        }
        else
        {
           ExpInfo.Billing.goID[0]='\000';
        }
    return(0);
}

set_acode_size(sz)
int sz;
{
    ExpInfo.acode_1_size = sz;
}

set_max_acode_size(sz)
int sz;
{
    ExpInfo.acode_max_size = sz;
}

setDSPgain(val)
int val;
{
    ExpInfo.DspGainBits = val;
}


/*-------------------------------------------------------------------
|
|   calcdspskip()
|   
+------------------------------------------------------------------*/
int calcdspskip()
{
  char dspflg[2], fsqflg[2];
  double dwelldsp=0.0, swval=0.0, oversampval=0.0;
  double skipdlyadj=0.0, prealfa, rof2val;
  int osskippts=0, prealfa_defined=0;

  /* optimize the delay to about 30us if no prealfa defined */
  double osskipdly=30.0e-6;

  dsposskipdelay=0.0;
  if ( P_getstring(GLOBAL,"dsp",dspflg,1,2) == 0 )
  {
    if ((dspflg[0] != 'r') && (dspflg[0] != 'i')) return (0);

    if (dspflg[0] == 'r') {
      if ( P_getstring(GLOBAL,"fsq",fsqflg,1,2) == 0 )
      {
        if (fsqflg[0] != 'y') return (0);
      }
      else return (0);
    }
  }

  /* check for prealfa defined? */
  if ((P_getreal(CURRENT,"prealfa",&prealfa,1)) >= 0)
  {
    if (prealfa > 0.0)
    {
      osskipdly=prealfa;
      prealfa_defined=1;
    }
  }
  /* fprintf(stderr,"\nosskipdly=%g\n",osskipdly); */

  P_getreal(CURRENT,"sw",&swval,1);
  P_getreal(CURRENT,"oversamp",&oversampval,1);
  /* fprintf(stderr,"sw=%g    oversamp=%g\n",swval,oversampval); */
  if ( (swval > 1.0) && (oversampval >= 4.0) )
  {
    if (dspflg[0] == 'r')
    {
      double dsp_il_factor = 4.0;
      dwelldsp = 1.0/(dsp_il_factor*swval);
    }
    else  if (dspflg[0] == 'i')
    {
      dwelldsp = 1.0/(oversampval*swval);
    }
  }
  else
  {
    return (0);
  }


  /* calculate skip delay and skip points */

  /* osskippts = (int) ((osskipdly/dwelldsp)+1.0001); */
  osskippts = (int) ((osskipdly/dwelldsp)+1.0001);

  if (osskippts < 2) osskippts=2;
  skipdlyadj=(osskippts-1)*dwelldsp;

  /* execute skip delay - this is done in test4acquire()
  if (skipdlyadj >= 0.0)  G_Delay(DELAY_TIME, skipdlyadj, 0);
  */

  /* commenting out the subtraction of rof2 below
     have to subtract rof2 delay of the last pulse before acquire. assuming it is rof2 
  if ((P_getreal(CURRENT,"rof2",&rof2val,1)) >= 0)
  {
    if (rof2val > 0.0)
    {
      dsposskipdelay = skipdlyadj - rof2val;
      if (dsposskipdelay < 0.0) dsposskipdelay = 0.0;
    }
  }
  else
  {
    dsposskipdelay = skipdlyadj;
  }
  commenting out the subtraction of rof2  */

  /* the following is to allow for empirical correction */
  dsposskipdelay = skipdlyadj - 7.7e-6 ;
  if (dsposskipdelay < 0.0) dsposskipdelay = 0.0;

  /* set the exact prealfa back into Vnmr & parameter set */
  if (prealfa_defined == 1)
  {
    double realvaluefactor=parmult(CURRENT,"prealfa");
    putCmd("setvalue('prealfa',%f)\n",skipdlyadj/realvaluefactor);
    putCmd("setvalue('prealfa',%f,'processed')\n",skipdlyadj/realvaluefactor);
  }

  return (osskippts);

}




#define OS_MAX_NTAPS		50000
#define OS_MIN_NTAPS		3
/*-----------------------------------------------
|                                               |
|               getDSPinfo()/0                  |
|                                               |
+----------------------------------------------*/
int getDSPinfo(factor,coef,sw,maxlbsw)
int factor,coef;
double sw;
double maxlbsw;
{
   int		np,
		ntaps;
   double	tmp;
   double  maxv, minv, stepv;
   char osskip[2];

   if (sw * (double) factor > maxlbsw+0.1)
   {
      text_error("oversamp * sw > %g", maxlbsw);
      psg_abort(1);
   }

   /*if ((P_getreal(CURRENT,"oversamp",&tmp,1)) >= 0)
      ExpInfo.DspOversamp =
        (var_active("oversamp",CURRENT)) ? (int) (tmp + 0.0005) : 0;
   else
   {   
      text_error("error copying oversamp");
      psg_abort(1);
   }*/

   ExpInfo.DspOversamp = factor;

   if (!newacq && ExpInfo.IlFlag)
   {
      text_error("Can't do DSP with il='y'");
      psg_abort(1);
   }
 
   np = ExpInfo.NumDataPts;
   par_maxminstep(CURRENT, "np", &maxv, &minv, &stepv);
   if (newacq) maxv *= 4.0;
   if (np*factor + coef/2 > maxv)
   {
      text_error("oversamp * np > %g", maxv);
      psg_abort(1);
   }
 
   if (coef/2 > OS_MAX_NTAPS)
   {
      text_error("oscoef must be less than 50000");
      psg_abort(1);
   }

   if (coef < OS_MIN_NTAPS)
   {
      text_error("oscoef must be greater than 3");
      psg_abort(1);
   }

   if ((P_getreal(CURRENT,"osfb",&tmp,1) >= 0) && var_active("osfb",CURRENT))
   {
      if (sw*(double)factor/(tmp*2.0) > 1.0)
      {
        ExpInfo.DspFb = tmp;
      }
   }

   if ((P_getreal(CURRENT,"oscoef",&tmp,1)) >= 0)
      ExpInfo.DspOsCoef = (int) (tmp + 0.0005);
   else
   {
      text_error("error with oscoef");
      psg_abort(1);
   }
   if ((P_getreal(CURRENT,"sw",&tmp,1)) >= 0)
      ExpInfo.DspSw = tmp;
   else
   {
      text_error("error with sw");
      psg_abort(1);
   }

   if ((P_getreal(CURRENT,"oslsfrq",&tmp,1) >= 0) &&
        var_active("oslsfrq",CURRENT))
      ExpInfo.DspOslsfrq = tmp;

   if ((P_getstring(CURRENT,"filtfile",ExpInfo.DspFiltFile,1,255)) < 0)
   {
      ExpInfo.DspFiltFile[0] = '\0';
   }

   ExpInfo.DspOsskippts = 0;
   if ( P_getstring(GLOBAL,"qcomp",osskip,1,2) == 0 )
   {
     if ((osskip[0] == 'y'))
     {
      ExpInfo.DspOsskippts = calcdspskip();
     }
   }

  /*
   fprintf(stderr,"getDSPinfo(): ExpInfo.DspOsskippts = %d\n", ExpInfo.DspOsskippts);
   fprintf(stderr,"getDSPinfo(): written ExpInfo Dsp info.\n");
  */

   return(0);
}


turnoff_swdsp()
{
   if (ix == 1)
   {
      text_error("Inline DSP turned off\n");
      putCmd("off('oversamp')\n");
      putCmd("off('oversamp','processed')\n");
   }
   /*dsp_info[0] = 0;
   dsp_info[1] = 1;
   dsp_info[2] = 1;
   dsp_info[3] = 0;*/

   dsp_params.flags = 0;
   dsp_params.rt_oversamp = 1;
   dsp_params.rt_extrapts = 0;
   dsp_params.il_oversamp = 1;
   dsp_params.il_extrapts = 0;
   dsp_params.rt_downsamp = 1;

    ExpInfo.DspOversamp = 0;
    ExpInfo.DspOsCoef = 0;
    ExpInfo.DspSw = 0.0;
    ExpInfo.DspFb = 0.0;
    ExpInfo.DspOsskippts = 0;
    ExpInfo.DspOslsfrq = 0.0;
    ExpInfo.DspFiltFile[0] = '\0';
}
turnoff_hwdsp()
{
   codeint *tmpptr;
   if (ix == 1)
   {
      text_error("Real time DSP turned off\n");
      putCmd("off('oversamp')\n");
      putCmd("off('oversamp','processed')\n");
   }
   /*dsp_info[0] = 0;
   dsp_info[1] = 1;
   dsp_info[2] = 1;
   dsp_info[3] = 0;*/

   dsp_params.flags = 0;
   dsp_params.rt_oversamp = 1;
   dsp_params.rt_extrapts = 0;
   dsp_params.il_oversamp = 1;
   dsp_params.il_extrapts = 0;
   dsp_params.rt_downsamp = 1;

   setDSPgain(0);
   Alc->acqasize = 1;
   tmpptr = Aacode + multhwlp_ptr -1; /* get address into codes multhwlp flag*/
   if (*tmpptr == NOISE)
   {
      tmpptr += 3;
      *tmpptr = 256;
   }
}

/* Allow user over ride of exptime calculation */
static double usertime = -1.0;
void g_setExpTime(double val)
{
  usertime = val;
}

double getExpTime()
{
  return(usertime);
}

void write_shr_info(double exp_time)
{
    int Infofd;	/* file discriptor Code disk file */
    int bytes;

    /* --- write parameter out to acqqueue file --- */

    if (usertime < 0.0)
       ExpInfo.ExpDur = exp_time;
    else
       ExpInfo.ExpDur = usertime;

    ExpInfo.SampleLoc = loc;
    if (newacq)
    {
       ExpInfo.NumTables = num_tables; /* set number of tables */
       strcpy(ExpInfo.WaveFormFile,fileRFpattern);
    }
    if (ExpInfo.InteractiveFlag)
    {
       char tmppath[256];
       sprintf(tmppath,"%s.new",infopath);
       unlink(tmppath);
       Infofd = open(tmppath,O_EXCL | O_WRONLY | O_CREAT,0666);
    }
    else
    {
       Infofd = open(infopath,O_EXCL | O_WRONLY | O_CREAT,0666);
    }
    if (Infofd < 0)
    {	text_error("Exp info file already exists. PSG Aborted..\n");
	psg_abort(1);
    }
    bytes = write(Infofd, (const void *) &ExpInfo, sizeof( SHR_EXP_STRUCT ) );
    if (bgflag)
      fprintf(stderr,"Bytes written to info file: %d (bytes).\n",bytes);
    close(Infofd);
}

void write_exp_info()
{
    int Infofd;	/* file discriptor Code disk file */
    int bytes;
#ifdef LINUX
    int cnt;
    long rt_tab_tmp[RT_TAB_SIZE];
#endif

    /* --- write parameter out real-time variable file --- */
    if (newacq)
    {
       /* These seem to not be deleted sometime. */
       /* Just delete it now.                    */
       unlink(ExpInfo.RTParmFile);
       Infofd = open(ExpInfo.RTParmFile,O_EXCL | O_WRONLY | O_CREAT,0666);
       if (Infofd < 0)
       {	text_error("Exp rt file already exists. PSG Aborted..\n");
   	   psg_abort(1);
       }
#ifdef LINUX
       for (cnt=0; cnt < RT_TAB_SIZE; cnt++)
       {
          rt_tab_tmp[cnt] = htonl(rt_tab[cnt]);
       }
       bytes = write(Infofd, rt_tab_tmp, sizeof(long) * get_rt_tab_elems() );
#else
       bytes = write(Infofd, rt_tab, sizeof(long) * get_rt_tab_elems() );
#endif
       if (bgflag)
         fprintf(stderr,"Bytes written to info file: %d (bytes).\n",bytes);
       close(Infofd);

       if (num_tables > 0)
       {
           writetablefile(ExpInfo.TableFile);
       }
       else
       {
           ExpInfo.TableFile[0] = '\000';
           fprintf(stderr,"PSG: WARNING: write_exp_info(): num_tables=0\n");
       }
       if (acqiflag) write_rtvar_acqi_file();
    }
}

int ra_inovaacqparms(fidn)
unsigned long fidn;
{
   char fidpath[512];
   int curct;

   sprintf(fidpath,"%s/fid",ExpInfo.DataFile);
   if (fidn == 1)
   {
     ifile = mOpen(fidpath,ExpInfo.DataSize,O_RDONLY );
     if (ifile == NULL) 
     {
	text_error("Cannot open data file: '%s', RA aborted.\n",
		ExpInfo.DataFile);
       psg_abort(1);
     }
     /* read in file header */
     memcpy((void*) &acqfileheader, (void*) ifile->offsetAddr,
					sizeof(acqfileheader));
     if (ntohl(acqfileheader.np) != ExpInfo.NumDataPts)
     {
	text_error("FidFile np: %d not equal NumDataPts: %d, RA aborted.\n",
		ntohl(acqfileheader.np),ExpInfo.NumDataPts);
       psg_abort(1);
     }
     if (ntohl(acqfileheader.ebytes) != ExpInfo.DataPtSize)
     {
	text_error("FidFile dp: %d not equal DataPtSize: %d, RA aborted.\n",
		ntohl(acqfileheader.ebytes),ExpInfo.DataPtSize);
       psg_abort(1);
     }
     if (ntohl(acqfileheader.ntraces) != ExpInfo.NumFids)
     {
	text_error("FidFile nf: %d not equal NumFids: %d, RA aborted.\n",
		ntohl(acqfileheader.ntraces),ExpInfo.NumFids);
       psg_abort(1);
     }
     ifile->offsetAddr += sizeof(acqfileheader);  /* move my file pointers */
     savedoffsetAddr = ifile->offsetAddr;
   }
   /* offset to fid location */
   if (fidn <= getStartFidNum())
   {
	ifile->offsetAddr = savedoffsetAddr +
		((ExpInfo.DataPtSize * ExpInfo.NumDataPts *
			ExpInfo.NumFids) + sizeof(acqblockheader)) * (fidn-1);
	memcpy((void*) &acqblockheader, (void*) ifile->offsetAddr,
					sizeof(acqblockheader));
	curct = ntohl(acqblockheader.ctcount);
	if (getIlFlag())
	{
	   if (fidn < getStartFidNum())
	   {
	      if (curct != (((ExpInfo.CurrentTran/ExpInfo.NumInBS)+1) * 
							ExpInfo.NumInBS))
	    text_error("Warning: File_ct(%d): %d not equal ct+bs: %d\n",
		fidn,curct,ExpInfo.CurrentTran+ExpInfo.NumInBS);
	   }
	   else
	   {
	      if (curct != ExpInfo.CurrentTran)
	    text_error("Warning: File_ct(%d): %d not equal ct: %d\n",
		fidn,curct,ExpInfo.CurrentTran);
	   }
	}
	else
	{
	   if (fidn < getStartFidNum())
	   {
	      if (curct != ExpInfo.NumTrans)
	    text_error("Warning: File_ct(%d): %d not equal nt: %d\n",
		fidn,curct,ExpInfo.NumTrans);
	   }
	   else
	   {
	      if (curct != ExpInfo.CurrentTran)
	    text_error("Warning: File_ct(%d): %d not equal ct: %d\n",
		fidn,curct,ExpInfo.CurrentTran);
	   }
	}

   }
   else if (getIlFlag() && (fidn > getStartFidNum()) && 
		(ExpInfo.CurrentTran >= ExpInfo.NumInBS))
   {
	/* Only when interleaving after 1st blocksize */
	ifile->offsetAddr = savedoffsetAddr +
		((ExpInfo.DataPtSize * ExpInfo.NumDataPts *
			ExpInfo.NumFids) + sizeof(acqblockheader)) * (fidn-1);
	memcpy((void*) &acqblockheader, (void*) ifile->offsetAddr,
					sizeof(acqblockheader));
	curct = ntohl(acqblockheader.ctcount);
	if (curct != ExpInfo.CurrentTran)
	  text_error("Warning: File_ct(%d): %d not equal CurrentTran: %d\n",
		fidn,curct,ExpInfo.CurrentTran);
   }
   else curct = 0;

   if (fidn >= ExpInfo.NumAcodes)
        mClose(ifile);
   Alc->acqct = curct;
}

int getExpNum()
{
	return(ExpInfo.ExpNum);
}

int getIlFlag()
{
	return(ExpInfo.IlFlag);
}

/*--------------------------------------------------------------*/
/* getStartFidNum						*/
/*	Return starting fid number for ra.  ExpInfo.Celem goes	*/
/*	from 0 to n-1. getStartFidNum goes from 1 to n		*/
/*--------------------------------------------------------------*/
int getStartFidNum()
{
   if (newacq)
	return(ExpInfo.Celem + 1);
   else
	return(1);
}
