/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  initacqparms.c  */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "mfileObj.h"
#include "variables.h"
#include "data.h"
#include "group.h"
#include "pvars.h"
#include "abort.h"
#include "ACode32.h"
#include "acqparms.h"
#include "cps.h"

/*  PSG_LC  conditional compiles lc.h properly for PSG */
#ifndef  PSG_LC
#define  PSG_LC
#endif
#include "lc.h"
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


extern int    putcode(codeint);
extern char *RcvrMapStr(const char *parmname, char *mapstr);
extern void init_acqvar(int index,int val);
extern int getIlFlag();
extern void custom_lc_init(int val1, int val2, int val3, int val4,
                    int val5, int val6, int val7, int val8,
                    int val9, int val10, int val11, int val12,
                    int val13, int val14, int val15, int val16,
                    int val17, int val18);
extern void std_lc_init();
extern int option_check(const char *option);

extern int AcodeManager_getAcodeStageWriteFlag(int stage);


extern int clr_at_blksize_mode;
extern int lockfid_mode;
extern int bgflag;

extern int newacq;
extern int      acqiflag;	/* for interactive acq. or not? */
extern char fileRFpattern[];
extern double relaxdelay;

extern int ilssval,ilctss,ssval,ctss;

static int acqfd;
static struct datafilehead acqfileheader;
static struct datablockhead acqblockheader;
static int acqparmsize = sizeof(autodata) + sizeof(Acqparams);
static char infopath[256];
static int acqpar_seek(unsigned int elemindx);
int getStartFidNum();

static MFILE_ID ifile = NULL;	/* inova datafile for ra */
static char *savedoffsetAddr;   /* saved inova offset header */

#define AUTOLOC 1	/* offset into lc struct for autod struct */
#define ACODEB 11	/* offset to beginning of Acodes */
#define ACODEP 12

#define RRI_SHIMSET	5	/* shimset number for RRI Shims */

extern unsigned int start_elem;     /* elem (FID) for acquisition to start on (RA)*/
extern unsigned int completed_elem; /* total number of completed elements (FIDs)  (RA) */

extern int num_tables;		/* number of tables for acodes */

extern Acqparams *Alc;
SHR_EXP_STRUCT ExpInfo;
static int max_ct;
int ss2val;

double psync;
double ldshimdelay,oneshimdelay;
int ok2bumpflag;


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
|   5/2/91    Greg B.    1. low core element lc->acqelemid is a int now
+---------------------------------------------------------------*/
void initacqparms(unsigned int fidn)
{
    char dp[MAXSTR];
    char cp[MAXSTR];
    char ok2bumpstr[MAXSTR];
    int np_words;	/* total # of data point words */
    int ss;		/* steady state count */
    int cttime;		/* #CTs between screen updates to Host */
    int dpflag;		/* double precision flag = 2 or 4 */
    int blocks;		/*  data size in blocks  (256words) */
    int asize = 0;		/*  data size in blocks  (256words) */
    int tot_np;		/*  np size */
    int curct = 0;
    int bsct4ra;
    double tmpval;

    /* initialize lc */
    std_lc_init();

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
        ss = (int) (sign_add(getval("ss"), 0.0005));
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
        cttime = 0; 		/* no ct display with steady state */
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

    dpflag = (dp[0] == 'y') ? 4 : 2;	/* single - 2, double - 4  INOVA */


    /* --- setup real time np with total data points to be take --- */
    /*     the STM will expect this many points    */
    /*     data pts * # of fids */
    tot_np = (int) ((int) (np + .0005)) * ((int) (nf + 0.005));

    if (dpflag == 4)
       asize |= 0xC000;
    np_words = tot_np;
    if (dpflag == 4)
        np_words *= 2L; /* double size for double precision data */

    if (bgflag)
	fprintf(stderr,"np: %lf, nf: %lf , asize = %x\n",np,nf,asize);
    blocks = (int) ((np_words + 255L) / 256L);  /* block = 256 words */

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
    if (P_getstring(CURRENT, "ok2bump", ok2bumpstr, 1, sizeof( ok2bumpstr) - 1 ) >= 0 )
    {
	if (ok2bumpstr[ 0 ] == 'Y' || ok2bumpstr[ 0 ] == 'y')
	  ok2bumpflag = 1;
    }

    xmtrstep = 90.0;
    decstep = 90.0;
    custom_lc_init(
    /* Alc->acqidver    */ (int) ((idc & 0x00ff)),
    /* Alc->acqelemid   */ (int) fidn,
    /* Alc->ctctr    */ (int) cttime,
    /* Alc->acqdsize    */ (int) blocks,
    /* Alc->np       */ (int) tot_np,
    /* Alc->nt       */ (int) (nt + 0.0001),
    /* Alc->dpf      */ (int) dpflag,
    /* Alc->bs       */ (int) (bs + 0.005),
    /* Alc->bsct     */ (int) bsct4ra,
    /* Alc->ss       */ (int) ss,
    /* Alc->asize    */ (int) asize,
    /* Alc->acqcpf      */ (int) cpflag,
    /* Alc->acqmaxconst */ (int) (sign_add(tmpval,0.005)),
    /* arraydim         */ (int) (ExpInfo.NumAcodes),
    /* relaxdelay	*/ (int) (relaxdelay*1e8),
    /* Alc->ct	*/ (int) curct,
    /* Alc->clrbsflag	*/ (int) clr_at_blksize_mode,
    /* lockflag		*/ (int) lockfid_mode
       );
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
/* fidn   fid number to obtain the acqpar parameters for */
int ra_initacqparms(unsigned int fidn)
{
   Acqparams *lc,*ra_lc;
   int len1,len2;
   int acqbuffer[512];
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
   DPRINT2(PRTLEVEL,"ct=%ld, ra ct=%ld\n",lc->ct,ra_lc->ct);
   DPRINT2(PRTLEVEL,"np=%ld, ra np=%ld\n",lc->np,ra_lc->np);
   DPRINT2(PRTLEVEL,"nt=%ld, ra nt=%ld\n",lc->nt,ra_lc->nt);

   /* find the maximum ct acquired without actually completing the FID */
   /* the assumption here is that fids are acquired 1 to arraydim */
   /* i.e., 1st fid ct>0 is max ct acquired so far */
   if ( (ra_lc->ct != ra_lc->nt) && (max_ct == 0L) )
   {
      max_ct = ra_lc->ct;
      start_elem = fidn;        /* maybe start_elem */
   }

   DPRINT3(PRTLEVEL,"fid: %lu, start_elem = %lu, max_ct = %d  \n",
	fidn,start_elem,max_ct);

   DPRINT1(PRTLEVEL,"il='%s'\n",il);
   if ( (max_ct != 0L) && (max_ct != -1L) && (ra_lc->ct != ra_lc->nt) )
   {
     /* first elem with bs!=bsct or ct < max_ct , if il!='n' is the start_elem */
     if ( (il[0] != 'n') && (il[0] != 'N') )
     {
       if ( (ra_lc->bsct != ra_lc->bs) || (ra_lc->ct < max_ct) )
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


   if (ra_lc->ct == ra_lc->nt)
   {
      lc->nt = ra_lc->nt; /* nt can not be change for completed fids */
      completed_elem++;         /* total number of completed elements (FIDs)  (RA) */
   }
   else if ( lc->nt <= ra_lc->ct )
    {
      lc->nt = ra_lc->nt; /* nt can not be made <= to ct , for now. */
      text_error("WARNING: FID:%u  'nt' <= 'ct', original 'nt' used.\n",
		fidn);
    }

   DPRINT2(PRTLEVEL,"nt=%d, ra nt=%d\n",lc->nt,ra_lc->nt);
   if (ra_lc->np != lc->np)
   {
      text_error("WARNING 'np' has changed from %d to %d.\n",
		ra_lc->np,lc->np);
   }
   if (ra_lc->dpf != lc->dpf)
   {
      text_error("data precision changed, PSG aborting.");
      psg_abort(1);
   }
   lc->ct = ra_lc->ct;

   lc->oph = ra_lc->oph;
   lc->bsct = ra_lc->bsct;
   lc->ssct = ra_lc->ssct;
   lc->tablert = ra_lc->tablert;
   lc->v1 = ra_lc->v1;
   lc->v2 = ra_lc->v2;
   lc->v3 = ra_lc->v3;
   lc->v4 = ra_lc->v4;
   lc->v5 = ra_lc->v5;
   lc->v6 = ra_lc->v6;
   lc->v7 = ra_lc->v7;
   lc->v8 = ra_lc->v8;
   lc->v9 = ra_lc->v9;
   lc->v10 = ra_lc->v10;
   lc->v11 = ra_lc->v11;
   lc->v12 = ra_lc->v12;
   lc->v13 = ra_lc->v13;
   lc->v14 = ra_lc->v14;
   lc->v15 = ra_lc->v15;
   lc->v16 = ra_lc->v16;
   lc->v17 = ra_lc->v17;
   lc->v18 = ra_lc->v18;
   lc->v19 = ra_lc->v19;
   lc->v20 = ra_lc->v20;
   lc->v21 = ra_lc->v21;
   lc->v22 = ra_lc->v22;
   lc->v23 = ra_lc->v23;
   lc->v24 = ra_lc->v24;
   lc->v25 = ra_lc->v25;
   lc->v26 = ra_lc->v26;
   lc->v27 = ra_lc->v27;
   lc->v28 = ra_lc->v28;
   lc->v29 = ra_lc->v29;
   lc->v30 = ra_lc->v30;
   lc->v31 = ra_lc->v31;
   lc->v32 = ra_lc->v32;


   return(1); /* data found and lc updated */
}
/*---------------------------------------------------------------------
|       set_counter()/
|
|       Sets ss counters for each element
+-------------------------------------------------------------------*/
void set_counters()
{
    int ilsstmp, ilctsstmp;
    if (Alc->ss < 0)
    {
       Alc->ssct = -1 * Alc->ss;
       ilsstmp = Alc->ssct;
    }
    else if (ix == getStartFidNum())
    {
       Alc->ssct = Alc->ss;
       ilsstmp = ss2val;
    }
    else if (ss2val)
    {
       Alc->ssct = ss2val;
       ilsstmp = ss2val;
    }
    else
    {
       Alc->ssct = 0;
       ilsstmp = 0;
    }
    Alc->rtvptr = ( Alc->nt - (Alc->ssct % Alc->nt) + Alc->ct )
							 % Alc->nt;
    ilctsstmp = ( Alc->nt - (ilsstmp % Alc->nt) + Alc->ct )
							 % Alc->nt;
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
void ss4autoshim()
{

  if (newacq)
  {
    if ((Alc->ss > 0) && (ix != getStartFidNum()))
    {
       Alc->ssct = Alc->ss;
       Alc->rtvptr = (Alc->nt - (Alc->ssct % Alc->nt) + Alc->ct)
							 % Alc->nt;
       init_acqvar(ssval,(int)Alc->ssct);
       init_acqvar(ctss,(int)Alc->rtvptr);
    }
  }
}

/*---------------------------------------------------------------------
|       open_acqpar(path)/1
|
|       Position the disk read/write heads to the proper block offset
|       for the acqpar file (lc,auto structure parameters.
+-------------------------------------------------------------------*/
void open_acqpar(char *filename)
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
static int acqpar_seek(unsigned int elemindx)
{
    int acqoffset;
    int pos;

    acqoffset = sizeof(acqfileheader) +
        (acqfileheader.bbytes * ((unsigned int) (elemindx - 1)))  ;
    if (bgflag)
        fprintf(stderr,"acqpar_seek(): fid# = %d,acqoffset = %d \n",
           elemindx,acqoffset);
    pos = lseek(acqfd, acqoffset,SEEK_SET);
    if (pos == -1)
    {
        char *str_err;
        if ( (str_err = strerror(errno) ) != NULL )
        {
           fprintf(stdout,"acqpar_seek Error: offset %d,: %s\n",
               acqoffset,str_err);
        }
        return(-1);
    }
    return(pos);
}

int setup_parfile(int suflag)
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
    if(ExpInfo.NumFids>1)
        ExpInfo.NFmod=ExpInfo.NumFids;
    else
        ExpInfo.NFmod=1;

    if (ExpInfo.NumFids>1 && (P_getreal(CURRENT,"nfmod",&tmp,1)) >= 0 && var_active("nfmod",CURRENT))
    {
         if(tmp<=0 || tmp>ExpInfo.NumFids)
             tmp=ExpInfo.NumFids;
         ExpInfo.NFmod=(int) (tmp + 0.0005);
    }

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

    /* multiple reciever mapping for recvproc */
    ExpInfo.RvcrMapping[0] = (char) 0;
    RcvrMapStr("rcvrs",ExpInfo.RvcrMapping);
    if (bgflag) fprintf(stdout," [[[[[[[[[-->>  RcvrMapStr: '%s'\n",ExpInfo.RvcrMapping);

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
    strcpy(ExpInfo.InitCodefile,infopath);
    strcat(ExpInfo.InitCodefile,".init");
    strcpy(ExpInfo.PreCodefile,infopath);
    strcat(ExpInfo.PreCodefile,".pre");
    strcpy(ExpInfo.PSCodefile,infopath);
    strcat(ExpInfo.PSCodefile,".ps");
    strcpy(ExpInfo.PostCodefile,infopath);
    strcat(ExpInfo.PostCodefile,".post");

    strcpy(ExpInfo.RTParmFile,infopath);
    strcat(ExpInfo.RTParmFile,".RTpars");
    strcpy(ExpInfo.AcqRTTablefile,infopath);
    strcat(ExpInfo.AcqRTTablefile,".init.InitAcqObject");
    strcpy(ExpInfo.WaveFormFile,infopath);
    strcat(ExpInfo.WaveFormFile,".pat");

    /* Beware that infopath gets accessed again
       if acqiflag is set, for the data file path */

    /* --- file path to named acqfile or exp# acqfile  'file' --- */

    strcpy(ExpInfo.VpMsgID,"");
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
      if ( ! P_getstring(CURRENT,"vpmode",autopar,1,12) && (autopar[0] == 'y') )
      {
         ExpInfo.ExpFlags |= VPMODE_BIT;  /* set vpmode bit */
         if (P_getstring(CURRENT,"VPaddr",tmpstr,1,255) >= 0)
           strcpy(ExpInfo.VpMsgID,tmpstr);
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


void set_rcvrs_info()
{
    /* multiple reciever mapping for recvproc */
    ExpInfo.RvcrMapping[0] = (char) 0;
    RcvrMapStr("rcvrs",ExpInfo.RvcrMapping);
    if (bgflag) fprintf(stdout," [[[[[[[[[-->>  RcvrMapStr: '%s'\n",ExpInfo.RvcrMapping);
}


void set_acode_size(int sz)
{
    ExpInfo.acode_1_size = sz;
}

void set_max_acode_size(int sz)
{
    ExpInfo.acode_max_size = sz;
}

/* Allow user over ride of exptime calculation */
static double usertime = -1.0;
void g_setExpTime(double val)
{
  if (ix == 1) {
    usertime = val;
    putCmd("displayscantime(%f)\n",usertime);
  }
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
    /*
       BKJ: CAUTION: need to check ExpInfo.NumTables
       ExpInfo.NumTables = num_tables;
    */

    if ( ! AcodeManager_getAcodeStageWriteFlag(0) ) strcpy(ExpInfo.InitCodefile,"\0");
    if ( ! AcodeManager_getAcodeStageWriteFlag(1) ) strcpy(ExpInfo.PreCodefile,"\0");
    if ( ! AcodeManager_getAcodeStageWriteFlag(2) ) strcpy(ExpInfo.PSCodefile,"\0");
    if ( ! AcodeManager_getAcodeStageWriteFlag(3) ) strcpy(ExpInfo.PostCodefile,"\0");
    if ( ! AcodeManager_getAcodeStageWriteFlag(4) ) strcpy(ExpInfo.WaveFormFile,"\0");

    if (ExpInfo.InteractiveFlag)
    {
       char tmppath[256];
       sprintf(tmppath,"%s.new",infopath);
       unlink(tmppath);
       Infofd = open(tmppath,O_EXCL | O_WRONLY | O_CREAT,0666);
    }
    else
    {
       /* fprintf(stdout,"infopath: '%s'\n", infopath); */
       Infofd = open(infopath,O_EXCL | O_WRONLY | O_CREAT,0666);
    }
    if (Infofd < 0)
    {	text_error("Exp info file already exists. PSG Aborted..\n");
	psg_abort(1);
    }
    /* fprintf(stdout," [[[[[[[[[-->>writing  RcvrMapStr: '%s'\n",ExpInfo.RvcrMapping); */
    bytes = write(Infofd, (const void *) &ExpInfo, sizeof( SHR_EXP_STRUCT ) );
    /* fprintf(stdout,"sizeof( SHR_EXP_STRUCT ) = %d\n",sizeof( SHR_EXP_STRUCT )); */
    fsync(Infofd);
    if (bgflag)
      fprintf(stdout,"Bytes written to info file: %d (bytes).\n",bytes);
    close(Infofd);
}

void ra_inovaacqparms(unsigned int fidn)
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
     if (acqfileheader.np != ExpInfo.NumDataPts)
     {
	text_error("FidFile np: %d not equal NumDataPts: %d, RA aborted.\n",
		acqfileheader.np,ExpInfo.NumDataPts);
       psg_abort(1);
     }
     if (acqfileheader.ebytes != ExpInfo.DataPtSize)
     {
	text_error("FidFile dp: %d not equal DataPtSize: %d, RA aborted.\n",
		acqfileheader.ebytes,ExpInfo.DataPtSize);
       psg_abort(1);
     }
     if (acqfileheader.ntraces != ExpInfo.NumFids)
     {
	text_error("FidFile nf: %d not equal NumFids: %d, RA aborted.\n",
		acqfileheader.ntraces,ExpInfo.NumFids);
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
	curct = acqblockheader.ctcount;
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
	curct = acqblockheader.ctcount;
	if (curct != ExpInfo.CurrentTran)
	  text_error("Warning: File_ct(%d): %d not equal CurrentTran: %d\n",
		fidn,curct,ExpInfo.CurrentTran);
   }
   else curct = 0;

   if (fidn >= ExpInfo.NumAcodes)
        mClose(ifile);
   Alc->ct = curct;
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
   return(ExpInfo.Celem + 1);
}
