/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>

#include "data.h"
#include "ftpar.h"
#include "process.h"
#include "group.h"
#include "variables.h"
#include "vnmrsys.h"
#include "allocate.h"
#include "buttons.h"
#include "displayops.h"
#include "fft.h"
#include "pvars.h"
#include "sky.h"
#include "tools.h"
#include "params.h"
#include "wjunk.h"

#ifdef  DEBUG
extern int      debug1;
#define DPRINT(str) \
        if (debug1) Wscrprintf(str)
#define DPRINT1(str, arg1) \
        if (debug1) Wscrprintf(str,arg1)
#define DPRINT2(str, arg1, arg2) \
        if (debug1) Wscrprintf(str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
        if (debug1) Wscrprintf(str,arg1,arg2,arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4) \
        if (debug1) Wscrprintf(str,arg1,arg2,arg3,arg4)
#else 
#define DPRINT(str)
#define DPRINT1(str, arg2)
#define DPRINT2(str, arg1, arg2)
#define DPRINT3(str, arg1, arg2, arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4)
#endif 
 

#define COMPLETE	0
#define ERROR		1
#define MAXINT		0x7FFFFFFF	/* maximum integer	*/
#define APODIZE		8
#define FALSE		0
#define TRUE		1
#define CMPLX_DC	8
#define MAXSTR		256

#define zerofill(data_pntr, npoints_to_fill)			\
			datafill(data_pntr, npoints_to_fill,	\
				0.0)


int		start_from_ft = 1;

static lpinfo	lpparams;

extern int	interuption,
		Bnmr,
		specIndex,
		c_first,
		c_last,
		c_buffer,
		readwtflag,
		acqflag;

extern double	fpointmult;
extern double getfpmult(int fdimname, int ddr);
extern int getzfnumber(int ndpts, int fnsize);
extern int getzflevel(int ndpts, int fnsize);
extern int fnpower(int fni);
extern int  check_other_experiment(char *exppath, char *exp_no, int showError);
extern int init_wt1(struct wtparams *wtpar, int fdimname);
extern int init_wt2(struct wtparams *wtpar, register float  *wtfunc,
             register int n, int rftflag, int fdimname, double fpmult, int rdwtflag);
extern void weightfid(float *wtfunc, float *outp, int n, int rftflag, int dtype);
extern int setlppar(lpstruct *parLPinfo, int dimen, int pstatus,
             int nctdpts, int ncfdpts, int mode, char *memId);
extern int disp_current_seq();
extern void setheader(dpointers *block, int status_value, int mode_value,
               int index_value, int hypercomplex);
extern int i_ft(int argc, char *argv[], int setstatus, int checkstatus, int flag2d,
                ftparInfo *ftpar, dfilehead *fidhead, dfilehead *datahead,
                dfilehead *phasehead);
extern int i_ift(int argc, char *argv[], int arg_no, ftparInfo *ftpar,
          dfilehead *fidhead, dfilehead *datahead);
extern int getfid(int curfid, float *outp, ftparInfo *ftpar, dfilehead *fidhead,
                int *lastfid);
extern int downsamp(dsparInfo *dspar, float *data, int ncdpts,
             int nclspts, int fn0, int realdata);
extern int lpz(int trace, float *data, int nctdpts, lpinfo lpparvals);
extern int fidss(ssparInfo sspar, float *data, int ncdpts, int nclspts);
extern int setprocstatus(int ft_dimname, int full2d);
extern void currentDate(char* cstr, int len );
extern void rotate_fid(float *fidptr, double ph0, double ph1, int np, int datatype);
extern void driftcorrect_fid(float *outp, int np, int lsfid, int datamult);
extern int saveProcpar(char *path);


typedef struct wtparams wtPar;
static int ift(int argc, char *argv[], int retc, char *retv[], int arg_no);
void eccCorr(float *outp, ftparInfo ftpar);


void set_vnmrj_ft_params(int procdim, int argc, char *argv[])
{
    char     ptmp[MAXPATH];

    if (P_setgroup(CURRENT,"ref", G_PROCESSING))
    {
      P_creatvar(CURRENT,"ref",T_REAL);
      P_setgroup(CURRENT,"ref", G_PROCESSING);
      P_setreal(CURRENT,"ref",0.0,1);
      if (!Bnmr)
         appendvarlist("ref");
    }
    currentDate(ptmp, MAXPATH);
    if (P_setstring(CURRENT,"time_processed",ptmp,1))
    {
      P_creatvar(CURRENT,"time_processed",T_STRING);
      P_setgroup(CURRENT,"time_processed", G_PROCESSING);
      P_setstring(CURRENT,"time_processed",ptmp,1);
    }
    P_setgroup(CURRENT,"time_processed", G_PROCESSING);
    if (!Bnmr)
      appendvarlist("time_processed");
    /* Use P_getactive to test if parameter exists */
    if (P_getactive(CURRENT,"time_plotted") < 0)
    {
       P_setstring(CURRENT,"time_plotted","",1);
       if (!Bnmr)
          appendvarlist("time_plotted");
    }
    if (P_setreal(CURRENT,"procdim",((double)procdim),1))
    {
      P_creatvar(CURRENT,"procdim",ST_INTEGER);
      P_setreal(CURRENT,"procdim",((double)procdim),1);
    }
    P_setgroup(CURRENT,"procdim", G_PROCESSING);
    if (!Bnmr)
       appendvarlist("procdim");
    if (argc)
    {
       strcpy(ptmp,argv[0]);
       if (argc > 1)
       {
          int index = 1;

          strcat(ptmp,"(");
          while (index < argc)
          {
             strcat(ptmp,"'");
             strcat(ptmp,argv[index]);
             index++;
             strcat(ptmp, (index == argc) ? "')" : "'," );
          }
       }
       if (P_setstring(CURRENT,"proccmd",ptmp,1))
       {
          P_creatvar(CURRENT,"proccmd",T_STRING);
          P_setgroup(CURRENT,"proccmd", G_PROCESSING);
          P_setstring(CURRENT,"proccmd",ptmp,1);
          /* do not allow any user change */
          P_setprot(CURRENT,"proccmd",P_ARR | P_ACT | P_VAL);
       }
    }
}

static void initAutoPhase(ftparInfo *ftpar, float *data)
{
   int i;
   float *iptr;
   float *optr;
   double rms;
   double re, im;

   iptr = data + (2 * ftpar->ftarg.phaseSkipPnts);
   optr = ftpar->ftarg.autophase;
   for (i=0; i < ftpar->ftarg.phasePnts; i++)
   {
      *optr++ = *iptr++;
      *optr++ = *iptr++;
   }
   iptr = data + (ftpar->np0 - 2 * ftpar->ftarg.phaseRmsPnts);
   rms = 0;
   for (i=0; i < ftpar->ftarg.phaseRmsPnts; i++)
   {
      re = *iptr++;
      im = *iptr++;
      rms += re*re + im*im;
   }
   rms = sqrt(rms/(ftpar->ftarg.phaseRmsPnts - 1));
   ftpar->ftarg.phaseRmsMult *= rms;
}

static double calcAutoPhase(ftparInfo *ftpar, float *data)
{
   int i, k;
   float *iptr;
   float *optr;
   double abs_xy0;
   double abs_xy;
   double re, im;
   double dphase;
   double phase1 = 0.0;
   double avphase;
   double phadd = 0.0;

   iptr = data + (2 * ftpar->ftarg.phaseSkipPnts);
   optr = ftpar->ftarg.autophase;
   k = 0;
   avphase = 0.0;
   for (i=0; i < ftpar->ftarg.phasePnts; i++)
   {
      re = *iptr++;
      im = *iptr++;
      abs_xy0 = sqrt( (*optr * *optr) + ( *(optr+1) * *(optr+1) ) );
      abs_xy = sqrt( re * re + im * im);
      if ( (abs_xy0 > ftpar->ftarg.phaseRmsMult) && (abs_xy > ftpar->ftarg.phaseRmsMult) )
      {
         dphase = atan2(( *optr * im - re * *(optr+1)),( *optr * re + *(optr+1) * im));
         if (k == 0)
         {
            phase1 = dphase;
         }
         else
         {
            if (dphase - phase1 > 180.0)
               phadd -= 360.0;
            else if (phase1 - dphase > 180.0)
               phadd += 360.0;
         }
         avphase += dphase + phadd;
         k++;
      }
      optr++;
      optr++;
   }
   return( (k==0) ? 0.0 : ((avphase / k) * 180.0 / M_PI) - 90.0 );
}

/*-----------------------------------------------
|						|
|		    fidproc()			|
|						|
|  This function is the same as ft() except it  |
|  skips the wt, ft steps and writes the FID    |
|						|
+----------------------------------------------*/
int fidproc(int argc, char *argv[], int retc, char *retv[])
{
  char		msge[MAXSTR];
  char		filepath[MAXPATH];
  int		status,
		res,
		pwr,
		nfft,
		cblock,
		lastcblock,
		blocksdone,
		lsfidx,
		fidnum = 0,
		arg_no,
		npx,
		npadj,
                do_ds,
		ftflag,
		noreal,
		element_no,
		lastfid,
		wtsize,
		first,
		savefirst,
		last,
		step,
		i,
		tmpi,
		zflevel,
		zfnumber,
		dsfn0,
		dsnpx,
		dsnpadj,
		dspwr,
		fnActive,
		realt2data;
  int		inverseWT;
  float		*outp,
		*finaloutp = NULL,
		*ptr1,
		*ptr2,
		*wtfunc;
  short		*ptr2s;
  double	rx,
		fnSave,
		tmp;
  dpointers	outblock,
		fidoutblock;
  dfilehead	fidhead,
		datahead,
		phasehead;
  lpstruct	parLPinfo;
  ftparInfo	ftpar;
  wtPar		wtp;
  vInfo		info;
  int fidshim = FALSE;
  int fidShift = 0;
  double        phase0 = 0.0;
  char          newfidpath[MAXPATH];

#ifdef FIDPROC
  if (argc > 1 && strcmp(argv[1],"fidshim") == 0) {
      fidshim = TRUE;
  }

  if (!fidshim) {
      Wturnoff_buttons();
  }
#endif
  ftpar.procstatus = setprocstatus(S_NP, 0);

/************************************
*  Initialize all parameterizeable  *
*  variables                        *
************************************/

  arg_no = first = step = element_no = 1;
  ftpar.nblocks = 1;
  last = MAXINT;

  do_ds = noreal = ftflag = TRUE;
  nfft = acqflag = inverseWT = FALSE;
  ftpar.t2dc = -1;
  ftpar.zeroflag = FALSE;
  ftpar.sspar.lfsflag = ftpar.sspar.zfsflag = FALSE;
  ftpar.dspar.dsflag = FALSE;
  ftpar.dspar.fileflag = FALSE;
  ftpar.dspar.newpath[0] = '\0';
  ftpar.ftarg.useFtargs = 0;
  sprintf(newfidpath,"%s/fidproc.fid",curexpdir);


/*********************************
*  Parse STRING arguments first  *
*********************************/

  while ( (argc > arg_no) && (noreal = !isReal(argv[arg_no])) )
  {
     if ((strcmp(argv[arg_no], "i") == 0) ||
     	   (strcmp(argv[arg_no], "inverse") == 0))
     {
        if (ift(argc, argv, retc, retv, arg_no))
        {
           disp_status("        ");
           ABORT;
        }
        else
        {
           RETURN;
        }
     }
     else if (strcmp(argv[arg_no], "all") == 0)
     {
        ftpar.nblocks = MAXINT;
     }
     else if (strcmp(argv[arg_no], "acq") == 0)
     {
        acqflag = TRUE;
     }
     else if (strcmp(argv[arg_no], "nodc") == 0)
     {
        ftpar.t2dc = FALSE;
     }
     else if (strcmp(argv[arg_no], "dodc") == 0)
     {
        ftpar.t2dc = TRUE;
     }
     else if (strcmp(argv[arg_no], "nods") == 0)
     {
        do_ds = FALSE;
     }
     else if (strcmp(argv[arg_no], "noft") == 0)
     {
        ftflag = FALSE;
     }
     else if (strcmp(argv[arg_no], "zero") == 0)
     {
        ftpar.zeroflag = TRUE;
     }
     else if (strcmp(argv[arg_no], "rev") == 0)
     {
        ftpar.zeroflag = -1;
     }
     else if (strcmp(argv[arg_no], "nf") == 0)
     {
        nfft = TRUE;
     }
     else if (strcmp(argv[arg_no], "fidshim") == 0)
     {
         fidshim = TRUE;
     }
     else if (strcmp(argv[arg_no], "inversewt") == 0)
     {
         inverseWT = TRUE;
     }
     else if (strcmp(argv[arg_no], "ftargs") == 0)
     {
         ftpar.ftarg.useFtargs = 1;
     }
     else if (strcmp(argv[arg_no], "nopars") == 0)
     {
         /* Pass this on to i_ft() */
     }
     else if (strcmp(argv[arg_no], "downsamp") == 0)
     {
       if (!P_getVarInfo(CURRENT, "downsamp", &info))
         if (info.active)
           if (!P_getreal(CURRENT, "downsamp", &tmp, 1))
	     if (tmp > 0.999)
	     {
	       ftpar.dspar.dsflag = TRUE;
	       ftpar.dspar.fileflag = TRUE;
	       arg_no++;
               if (argc <= arg_no || !isReal(argv[arg_no]))
               {
                 Werrprintf("usage  -  %s :  experiment number must follow 'downsamp'",argv[0]);
	         disp_status("        ");
	         ABORT;
	       }
	       if (check_other_experiment(ftpar.dspar.newpath,argv[arg_no], 1))
	       {
	         return(ERROR);
	       }
	     }
       if (!ftpar.dspar.dsflag)
       {
         Werrprintf("parameter 'downsamp' must be active and > 1");
         disp_status("        ");
         ABORT;
       }
     }
     else if (strcmp(argv[arg_no], "file") == 0)
     {
        arg_no++;
        if (arg_no >= argc)
        {
           Werrprintf("%s : 'file' argument must be followed by filename", argv[0]);
           disp_status("        ");
	   ABORT;
        }
        if (argv[arg_no][0] == '/')
           sprintf(newfidpath,"%s",argv[arg_no]);
        else
           sprintf(newfidpath,"%s/%s",curexpdir,argv[arg_no]);
        if (strcmp(newfidpath+strlen(newfidpath)-4,".fid") != 0 &&
            strcmp(newfidpath+strlen(newfidpath)-5,".fid/") != 0)
           strcat(newfidpath,".fid");
     }
     else if (strcmp(argv[arg_no], "noop") != 0)
     {
        Werrprintf("usage  -  %s :  incorrect string argument", argv[0]);
        disp_status("        ");
	ABORT;
     }

     arg_no++;
  }

/******************************
*  Parse REAL arguments next  *
******************************/

  sprintf(msge,
	"usage  -  %s :  string arguments must precede numeric arguments\n",
	  argv[0]);

  if (argc > arg_no)
  {
     if (nfft)
     {
        if (isReal(argv[arg_no]))
        {
           element_no = (int) (stringReal(argv[arg_no++]));
        }
        else
        {
           Werrprintf(msge);
           ABORT;
        }
     }

     if (argc > arg_no)
     {
        if (isReal(argv[arg_no]))
        {
           first = last = (int) (stringReal(argv[arg_no++]));
           if (argc > arg_no)
           {
              if (isReal(argv[arg_no]))
              {
                 last = (int) (stringReal(argv[arg_no++]));
                 if (argc > arg_no)
                 {
                    if (isReal(argv[arg_no]))
                    {
                       step = (int) (stringReal(argv[arg_no++]));
                    }
                    else
                    {
                       Werrprintf(msge);
                       ABORT;
                    }
                 }
              }
              else
              {
                 Werrprintf(msge);
                 ABORT;
              }
           }
        }
        else
        {
           Werrprintf(msge);
           ABORT;
        }
     }
  }
  ftflag = nfft = FALSE;
  ftpar.dspar.dsflag = FALSE;
  do_ds = FALSE;
  if ( (fnActive = P_getactive(CURRENT, "fn")) )
  {
     double rval;

     P_getreal(CURRENT,"fn", &fnSave, 1);
     P_getreal(PROCESSED, "np", &rval, 1);
     if (fnSave < rval)
        P_setactive(CURRENT, "fn", ACT_OFF);
     else
        fnActive = 0;
  }

/******************************
*  Adjust parameters for FT.  *
******************************/

  if (first < 1)
     first = 1;
  if (step < 1)
     step = 1;
  if (last < first)
     last = first;
  ftpar.nblocks = last;

/*****************************************
*  Modify the FT parameters if ft('nf')  *
*  was executed.                         *
*****************************************/

  if (nfft)
  {
     if ( (res = P_getreal(PROCESSED, "nf", &rx, 1)) )
     {
        P_err(res, "nf", ":");
        ABORT;
     }

     ftpar.nblocks = (int) (rx + 0.5);
     if (last > ftpar.nblocks)
     {
        last = ftpar.nblocks;
     }
     else if (ftpar.nblocks > last)
     {
        ftpar.nblocks = last;
     }
  }

/******************************
*  Initialize data files and  *
*  FT parameters.             *
******************************/

  if ( i_ft(argc, argv, (S_DATA | S_SPEC | S_FLOAT),
            0, 0, &ftpar, &fidhead, &datahead, &phasehead) )
  {
      disp_status("        ");
      ABORT;
  }
  if (ftpar.t2dc == -1)
  {
      ftpar.t2dc = (fidhead.status & S_DDR) ? FALSE : TRUE;
  }
  disp_current_seq();

  if (fidhead.ntraces > 1)
  {
     ftpar.cf = ftpar.nf = 1;
     ftpar.np0 *= fidhead.ntraces;
     while (ftpar.fn0 < ftpar.np0)
        ftpar.fn0 *= 2;
  }
  ftpar.cf = ( (nfft) ? first : ftpar.cf );	/* ranges from 1 to nf */

  if (acqflag)
  {
     first = ++specIndex;
  }
  else
  {
     specIndex = first;		/* tells interactive programs
				   that new data exist */
  }

/***************************************************
*  np0  =  total number of points in the fid       *
*  npx  =  used number of points in the fid        *
*  fn0  =  number of points in the converted file  *
*                                                  *
*  Adjust "npx" and "lsfidx".                      *
***************************************************/

  lsfidx = ftpar.lsfid0;
// fprintf(stderr,"lsfidx= %d fn0= %d np0= %d\n",lsfidx, ftpar.fn0, ftpar.np0);

  if (ftpar.dspar.dsflag)
  {
    /* save values for use after filtering and downsampling */
    dsnpx =( (ftpar.fn0 < (ftpar.dspar.finalnp - lsfidx/ftpar.dspar.dsfactor)) ?
        (ftpar.fn0 + lsfidx/ftpar.dspar.dsfactor) : ftpar.dspar.finalnp );
    dsnpadj = dsnpx - lsfidx/ftpar.dspar.dsfactor;
    ftpar.dspar.finalnp = dsnpadj;
    dsfn0 = ftpar.fn0;
    dspwr = fnpower(dsfn0);

    /* values to use for processing before filtering and downsampling */
    /* must trick routines below into thinking fn0 >= np0 or they won't use
        all the points in the FID */
    ftpar.fn0 = (dsfn0*ftpar.dspar.dsfactor > ftpar.np0) ?
                 dsfn0*ftpar.dspar.dsfactor : ftpar.np0;
    tmpi = 32;
    while (tmpi < ftpar.fn0)
      tmpi *= 2;
    ftpar.fn0 = tmpi;
    npx = ( (ftpar.fn0 < (ftpar.np0 - lsfidx)) ? (ftpar.fn0 + lsfidx)
                : ftpar.np0 );
    npadj = npx - lsfidx;
    pwr = fnpower(ftpar.fn0);
  }
  else
  {
    npx = dsnpx = ( (ftpar.fn0 < (ftpar.np0 - lsfidx)) ? (ftpar.fn0 + lsfidx)
                : ftpar.np0 );
    npadj = dsnpadj = npx - lsfidx;     /* adjusted number of FID data points */
    pwr = dspwr = fnpower(ftpar.fn0);
    dsfn0 = ftpar.fn0;
  }
// fprintf(stderr,"npx= %d npadj= %d np0= %d\n",npx, npadj, ftpar.np0);

/*
  fpointmult = (ftpar.wtflag) ? getfpmult(S_NP, fidhead.status & S_DDR) : 1.0;
 */
/* first point multiply is needed for correct FT behavior. Since no ft is happening
 * I will skip it here. This prevents applying it twice
 */
  fpointmult = 1.0;
  if (ftpar.ftarg.numShift && (ftpar.ftarg.fidsPerSpec > 1) && ftpar.ftarg.numSa )
  {
     /* Assume PureShift */
     npadj = 2 * ftpar.ftarg.fidsPerSpec * *ftpar.ftarg.sa;
  }

  if (lsfidx < 0)
  {
     if (npx < 2)
     {
        Werrprintf("lsfid is too large in magnitude");
        ABORT;
     }
  }
  else
  {
     if (lsfidx >= npx)
     {
        Werrprintf("lsfid is too large in magnitude");
        ABORT;
     }
  }

  realt2data = (ftpar.procstatus & REAL_t2);
  if (ftpar.nblocks > fidhead.nblocks)
     ftpar.nblocks = fidhead.nblocks;

  if ( setlppar(&parLPinfo, S_NP, ftpar.procstatus, npadj/2, ftpar.fn0/2,
		  LPALLOC, "ft2d") )
  {
     disp_status("        ");
     releaseAllWithId("ft2d");
     Wsetgraphicsdisplay("");		/* TRY THIS!  SF */
     ABORT;
  }

  if (parLPinfo.sizeLP)
  {
     int	maxlpnp,
		nptmp;

     if (realt2data)
     {
        Werrprintf("LP analysis is not supported for real t2 data");
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }

     maxlpnp = npadj;

     for (i = 0; i < parLPinfo.sizeLP; i++)
     {
        lpparams = *(parLPinfo.parLP + i);

        if (lpparams.status & FORWARD)
        {
           nptmp = 2*(lpparams.startextpt + lpparams.ncextpt - 1);
           if (nptmp > maxlpnp)
              maxlpnp = nptmp;
        }
     }

     if (ftpar.dspar.dsflag)
     {
       dsnpadj += ((maxlpnp-npadj)/ftpar.dspar.dsfactor);
       if (dsnpadj > dsfn0)  dsnpadj = dsfn0;
       ftpar.dspar.finalnp += ((maxlpnp-npadj)/ftpar.dspar.dsfactor);
     }
     else
       dsnpadj = maxlpnp;
     npadj = maxlpnp;
  }

  if (ftpar.dspar.dsflag)
  {
    zflevel = getzflevel(dsnpadj, dsfn0);
    zfnumber = getzfnumber(dsnpadj, dsfn0);
  }
  else
  {
    zflevel = getzflevel(npadj, ftpar.fn0);
    zfnumber = getzfnumber(npadj, ftpar.fn0);
  }

  if (ftpar.dspar.dsflag)
  {
    ftpar.dspar.buffer = (double *)allocateWithId(sizeof(double)*ftpar.fn0,"ft2d");
    if (!ftpar.dspar.buffer)
    {
      Werrprintf("Error allocating digital filtering buffer");
      ftpar.dspar.dsflag = FALSE;
      ftpar.dspar.fileflag = FALSE;
      return(ERROR);
    }

    ftpar.dspar.data = (float *)allocateWithId(sizeof(float)*ftpar.fn0,"ft2d");
    if (!ftpar.dspar.data)
    {
      Werrprintf("Error allocating digital filtering data buffer");
      ftpar.dspar.dsflag = FALSE;
      ftpar.dspar.fileflag = FALSE;
      return(ERROR);
    }
  }


/****************************************
*  Initialize weighting parameters and  *
*  weighting vector.                    *
****************************************/

  wtp.wtflag = ftpar.wtflag;
  wtfunc = NULL;

  if (wtp.wtflag)
  {           /* allocate wtfunc, npadj/2 points, 4 bytes per point */
	      /* allocate wtfunc, npadj points if real FT */
     wtsize = npadj/2;
     if (realt2data)
        wtsize *= 2;

     if ((wtfunc = allocateWithId(sizeof(float)*wtsize, "ft2d")) == NULL)
     {
        Werrprintf("cannot allocate memory for weighting vector");
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }

     if (init_wt1(&wtp, S_NP))
     {
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }

     readwtflag = FALSE;	/* for user-defined weighting functions */
     if (init_wt2(&wtp, wtfunc, wtsize, realt2data, S_NP,
		(ftpar.dspar.dsflag) ? 1.0 :  fpointmult, readwtflag))
     {
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }

     if (!wtp.wtflag)
     {
        release(wtfunc);
        wtfunc = NULL;
     }
     else if (inverseWT)
     {
        int i;
        float *ptr;
        ptr = wtfunc;
        for (i=0; i<wtsize; i++)
        {
           if ( *ptr )
           {
              *ptr = 1.0 / *ptr;
              ptr++;
           }
           else
           {
              Werrprintf("Cannot invert weighting function with a zero in it.");
              releaseAllWithId("ft2d");
              disp_status("        ");
              ABORT;
           }
        }
     }
  }

/**************************
*  Start loop over FIDs.  *
**************************/

  lastfid = ftpar.ni0 * ftpar.ni1 * ftpar.arraydim;
  if (nfft)
  {
     if ((element_no < 1) || (element_no > lastfid))
     {
        Werrprintf("ft  -  FID index must be between 1 and %d\n", lastfid);
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }
  }
  else
  {
     if (lastfid > ftpar.nblocks * ftpar.ftarg.fidsPerSpec)
        lastfid = ftpar.nblocks * ftpar.ftarg.fidsPerSpec;
     if (first > lastfid)
     {
        last = lastfid;
        if (acqflag)
           specIndex--;
     }
  }

  status = (S_DATA | S_SPEC | S_FLOAT | S_COMPLEX | ftpar.D_cmplx);

  D_trash(D_DATAFILE);
  if(access(newfidpath,F_OK) != 0)
  {
   if(mkdir(newfidpath,0777)) {
     Winfoprintf("cannot create %s",newfidpath);
     ABORT;
   }
  }

   // now write out fid to newfidpath
  strcpy(filepath,newfidpath);
  strcat(filepath,"/fid");

  datahead.status = S_DATA | S_FLOAT | S_COMPLEX;
  datahead.vers_id = fidhead.vers_id;
  datahead.nbheaders = fidhead.nbheaders;
  datahead.nblocks = fidhead.nblocks;
  datahead.ntraces = fidhead.ntraces;
  datahead.np = (datahead.ntraces == 1) ? npadj : fidhead.np;
  datahead.ebytes = fidhead.ebytes;
  datahead.tbytes = datahead.np*fidhead.ebytes;
  datahead.bbytes = datahead.tbytes*datahead.ntraces + sizeof(dblockhead);

  if (D_newhead(D_DATAFILE, filepath, &datahead) )
  {
     Werrprintf("cannot open file %s", filepath);
     ABORT;
  }
  if (ftpar.ftarg.fidsPerSpec > 1)
  {
    ftpar.ftarg.buffer = (float *)allocateWithId(sizeof(float)*(npadj+ftpar.np0),"ft2d");
    if (!ftpar.ftarg.buffer)
    {
      Werrprintf("Error allocating combination buffer");
      return(ERROR);
    }
    /* Don't to dc correction until FID is constructed */
    ftpar.ftarg.saveT2dc = ftpar.t2dc;
    ftpar.t2dc = FALSE;
  }


/***********************************************
*  Necessary until I can devise a function to  *
*  read only the FID block header in order to  *
*  determine if FID data exists in that block. *
***********************************************/

  blocksdone = FALSE;
  lastcblock = ftpar.nblocks;

  if (nfft)
  {
     if (acqflag)
     {
        Werrprintf("ft  -  ft('acq','nf') are incomensurate\n");
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }

     cblock = 0;
     fidnum = element_no - 1;
  }
  else
  {
     cblock = ( (acqflag) ? (first - 1) : 0 );
  }

  savefirst = first;

  while ((cblock < lastcblock) && (!blocksdone))
  {
     int fidCnt;
     float *combineptr = NULL;

     DPRINT1("block %d\n", cblock);
     if ( (res = D_allocbuf(D_DATAFILE, cblock, &outblock)) )
     {
        D_error(res);
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }
     outblock.head->ctcount = 1;  /* default setting */
     outblock.head->scale = 0;    /* default setting */

     if (ftpar.dspar.dsflag)
     {
       outp = ftpar.dspar.data;
       finaloutp = (float *)outblock.data;
     }
     else
     {
       outp = (float *)outblock.data;
     }

/**********************************************************
*  Start filling at the start of the output data buffer.  *
*  This facilitates the automatic array-like processing   *
*  of 'cf' and 'nf' in 1D.                                *
**********************************************************/

     if (!nfft)
        fidnum = cblock;
     
     if (interuption)
     {
        releaseAllWithId("ft2d");
        D_trash(D_PHASFILE);
        D_trash(D_DATAFILE);
        disp_status("        ");
        ABORT;
     }

     if ((cblock + 1) == first)
     {
      fidCnt = 0;
      if (ftpar.ftarg.fidsPerSpec > 1)
      {
         combineptr = outp;
         outp = ftpar.ftarg.buffer;
         zerofill(combineptr, npadj );
         zerofill(outp, npadj );
      }
      while ( (fidCnt < ftpar.ftarg.fidsPerSpec) && status)
      {
        if (ftpar.ftarg.infoFD)
        {
           if ( ! fidCnt)
              fprintf(ftpar.ftarg.infoFD,"Fid %d\n",cblock+1);
           fprintf(ftpar.ftarg.infoFD,"   Component Fid %d\n",fidCnt+1);
        }
        if ( getfid((fidnum)*ftpar.ftarg.fidsPerSpec+fidCnt, outp, &ftpar, &fidhead, &lastfid) )
	{
	   Werrprintf("Unable to get FID data");
           releaseAllWithId("ft2d");
	   disp_index(0);
           disp_status("        ");
           ABORT;
        }
        fidCnt++;

	if (lastfid == 0)
	{
           releaseAllWithId("ft2d");
	   disp_index(0);
	   disp_status("        ");
           ABORT;
        }
        else if ((cblock == lastfid) && (!nfft))
	{
           status = 0;
           blocksdone = TRUE;
           if (last == MAXINT)
           {
              last = cblock;
           }
	}
        else
        {
           if (ftpar.ftarg.eccPnts)
           {
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      ECC correction using file %s and shift = %d\n",
                         ftpar.ftarg.eccFile, ftpar.ftarg.eccLsfid);
              }
              eccCorr(outp, ftpar);
           }
           if (ftpar.ftarg.numFreq > 0)
           {
              double ph1;
              int index;
              index = ftpar.ftarg.curFreq % ftpar.ftarg.numFreq;
              ftpar.ftarg.curFreq++;
              ph1 = *(ftpar.ftarg.freq+index);
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Frequency shift = %g\n", ph1);
              }
              rotate_fid(outp, (double) 0.0, ph1, ftpar.np0 - ftpar.lsfid0, COMPLEX);
           }
           else if (ftpar.ftarg.numFreq == -1)
           {
              double ph1;
              ph1 = ftpar.ftarg.initFreq + (fidCnt-1) * ftpar.ftarg.incrFreq;
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Incrementing Frequency shift = %g\n", ph1);
              }
              rotate_fid(outp, (double) 0.0, ph1, ftpar.np0 - ftpar.lsfid0, COMPLEX);
              
           }
           if (ftpar.ftarg.phasePnts)
           {
              if ( ! ftpar.ftarg.autoPhaseInit )
              {
                 initAutoPhase(&ftpar, outp);
                 ftpar.ftarg.autoPhaseInit = 1;
                 if (strlen(ftpar.ftarg.autoPhasePar))
                    P_setreal(CURRENT,ftpar.ftarg.autoPhasePar,0.0,0);
                 ftpar.ftarg.curPhase = 1;
                 phase0 = calcAutoPhase(&ftpar, outp);
                 if (ftpar.ftarg.infoFD)
                 {
                    fprintf(ftpar.ftarg.infoFD,"      Auto-calculated reference rotatation = %g\n", phase0);
                 }

              }
              else
              {
                 double ph0;
                 ph0 = calcAutoPhase(&ftpar, outp) - phase0;
                 if (ftpar.ftarg.infoFD)
                 {
                    fprintf(ftpar.ftarg.infoFD,"      Auto-calculated phase rotatation = %g\n", ph0);
                 }
                 ftpar.ftarg.curPhase++;
                 if (strlen(ftpar.ftarg.autoPhasePar))
                    P_setreal(CURRENT,ftpar.ftarg.autoPhasePar,ph0,ftpar.ftarg.curPhase);
                 rotate_fid(outp, ph0, (double) 0.0, ftpar.np0 - ftpar.lsfid0, COMPLEX);
              }
           }
           else if (ftpar.ftarg.numPhase)
           {
              double ph0;
              int index;
              index = ftpar.ftarg.curPhase % ftpar.ftarg.numPhase;
              ftpar.ftarg.curPhase++;
              ph0 = *(ftpar.ftarg.phase+index);
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Phase rotatation = %g\n", ph0);
              }
              rotate_fid(outp, ph0, (double) 0.0, ftpar.np0 - ftpar.lsfid0, COMPLEX);
           }

           if (ftpar.ftarg.numSa)
           {
              register int npntSa;
              register int fpntSa;
              register int index;
              register int npx;
              register float *ptr;

              index = ftpar.ftarg.curSa % ftpar.ftarg.numSa;
              ftpar.ftarg.curSa++;
              npntSa = *(ftpar.ftarg.sa+index);
              if (ftpar.ftarg.numSas)
              {
                 index = ftpar.ftarg.curSas % ftpar.ftarg.numSas;
                 ftpar.ftarg.curSas++;
                 fpntSa = *(ftpar.ftarg.sas+index);
              }
              else
              {
                 fpntSa = 0;
              }
              /* convert from complex pairs to sum of reals and imaginaries */
              fpntSa *= 2;
              npntSa *= 2;

              index = 0;
              npx = ftpar.np0 - ftpar.lsfid0;
              ptr = outp;
              while ( (index < npx) && (index < fpntSa) )
              {
                 *ptr++ = 0.0;
                 *ptr++ = 0.0;
                 index += 2;
              }
              ptr = outp+fpntSa+npntSa;
              index = fpntSa+npntSa;
              while (index < npx)
              {
                 *ptr++ = 0.0;
                 *ptr++ = 0.0;
                 index += 2;
              }
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Sampling window from point %d for %d points\n", fpntSa/2, npntSa/2);
              }
           }
           if (ftpar.ftarg.numAmp)
           {
              register float ampl;
              register int index;
              register int npx;
              register float *ptr;

              index = ftpar.ftarg.curAmp % ftpar.ftarg.numAmp;
              ftpar.ftarg.curAmp++;
              ampl = (float) *(ftpar.ftarg.amp+index);
              index = 0;
              npx = ftpar.np0 - ftpar.lsfid0;
              ptr = outp;
              while (index < npx)
              {
                 *ptr++ *= ampl;
                 *ptr++ *= ampl;
                 index += 2;
              }
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Amplitude multiplication = %g\n", ampl);
              }
           }
           if (ftpar.sspar.zfsflag || ftpar.sspar.lfsflag)
           {
              disp_status("SS  ");
              if ( fidss(ftpar.sspar, outp, npx/2, lsfidx/2) )
              {
                 Werrprintf("time-domain solvent subtraction failed");
                 releaseAllWithId("ft2d");
                 disp_index(0);
                 disp_status("        ");
                 ABORT;
              }
           }

           if (parLPinfo.sizeLP)
           {
              for (i = 0; i < parLPinfo.sizeLP; i++)
              {
                 lpparams = *(parLPinfo.parLP + i);
                 disp_status(lpparams.label);

                 if (lpz(fidnum, outp, (npx - lsfidx)/2, lpparams))
                 {
                    Werrprintf("LP analysis failed");
                    releaseAllWithId("ft2d");
                    disp_index(0);
                    disp_status("       ");
                    ABORT;
                 }
              }
           }
           if (ftpar.ftarg.numShift > 0)
           {
              int npx;
              int index;
              index = ftpar.ftarg.curShift % ftpar.ftarg.numShift;
              ftpar.ftarg.curShift++;

              fidShift = *(ftpar.ftarg.shift+index);
              npx = ftpar.np0 - ftpar.lsfid0;
              if (fidShift > npadj/2)
                 fidShift =  npadj/2;
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Data shift = %d\n", fidShift);
              }
              if (fidShift != 0)
              {
                 shiftComplexData(outp, fidShift, npx / 2, ftpar.fn0/2); 
              }
           }
           else if (ftpar.ftarg.numShift == -1)
           {
              int npx;
              fidShift = ftpar.ftarg.initShift + (fidCnt-1) * ftpar.ftarg.incrShift;
              npx = ftpar.np0 - ftpar.lsfid0;
              if (fidShift > npadj/2)
                 fidShift =  npadj/2;
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Incrementing Data shift = %d\n", fidShift);
              }
              if (fidShift != 0)
              {
                 int npx;
                 npx = ftpar.np0 - ftpar.lsfid0;
                 shiftComplexData(outp, fidShift, npx / 2, ftpar.fn0/2); 
              }
           }
           if (ftpar.ftarg.fidsPerSpec > 1)
           {
              combine(outp, combineptr, npadj/2, COMPLEX,
                      ftpar.ftarg.rr, ftpar.ftarg.ir,
                      ftpar.ftarg.ri, ftpar.ftarg.ii);
           }
           else if (ftpar.ftarg.multfid)
           {
              register int i;
              register float re, im;

              combineptr = outp;
              for (i = 0; i < npadj/2; i++)
              {
                 re = *combineptr * ftpar.ftarg.rr + *(combineptr+1) * ftpar.ftarg.ir;
                 im = *combineptr * ftpar.ftarg.ri + *(combineptr+1) * ftpar.ftarg.ii;
                 (*combineptr++) = re;
                 (*combineptr++) = im;
              }
           }
          }
        }
        if (status)
        {
           if (ftpar.ftarg.fidsPerSpec > 1)
           {
              outp = combineptr;
              if (ftpar.ftarg.saveT2dc)
                 driftcorrect_fid(outp, npadj/2, 0, COMPLEX);
           }

           if (wtp.wtflag)
           {
              if (!ftflag)
                 disp_status("WT  ");
              weightfid(wtfunc, outp, npadj/2, realt2data, COMPLEX);
           }

	   if (ftpar.dspar.dsflag)
	   {
	     disp_status("DIGFILT");
	     if (downsamp(&(ftpar.dspar), outp, npadj/2, 0,
			ftpar.fn0, ftpar.procstatus&REAL_t2) )
	     {
		 Werrprintf("digital filtering failed");
		 releaseAllWithId("ft2d");
		 disp_index(0);
		 disp_status("        ");
		 ABORT;
	     }
	     if (ftpar.dspar.fileflag)
	     {
	       D_close(D_USERFILE);
	       if ( (res=D_getfilepath(D_USERFILE,filepath,ftpar.dspar.newpath)) )
	       {
		 D_error(res);
		 return(ERROR);
	       }
	       if ( (res = D_open(D_USERFILE, filepath, &fidhead)) )
	       {
		 D_error(res);
		 return(ERROR);
	       }

	       ptr1 = outp;
	       D_allocbuf(D_USERFILE, cblock, &fidoutblock);
               setheader(&fidoutblock,datahead.status,ftpar.D_dsplymode,cblock+1,
				ftpar.hypercomplex);
	       fidoutblock.head->lpval = ftpar.dspar.lp;
	       fidoutblock.head->lvl = ftpar.dspar.lvl;
	       fidoutblock.head->tlt = ftpar.dspar.tlt;
	       fidoutblock.head->scale = 0;
	       fidoutblock.head->ctcount = 1;  /* avoid double scaling of FID */
	       if (ftpar.dspar.dp)
	       {
	         ptr2 = (float *)fidoutblock.data;
	         for (i=0;i<dsnpadj;i++)
	           *ptr2++ = (*ptr1++);
                 if (fpointmult != 1.0)
                 {
	            ptr2 = (float *)fidoutblock.data;
                    *ptr2 *= fpointmult;
                    *(ptr2 + 1) *= fpointmult;
                 }
	       }
	       else
	       {
	         ptr2s = (short *)fidoutblock.data;
	         for (i=0;i<dsnpadj;i++)
	           *ptr2s++ = (short)((*ptr1++)+0.5);
	       }
	       if ( (res = D_markupdated(D_USERFILE, cblock)) )
	       {
		 D_error(res);
		 releaseAllWithId("ft2d");
		 disp_index(0);
		 disp_status("        ");
		 ABORT;
	       }

	       if ( (res = D_release(D_USERFILE, cblock)) )
	       {
		 D_error(res);
		 releaseAllWithId("ft2d");
		 disp_index(0);
		 disp_status("        ");
		 ABORT;
	       }

	       D_close(D_USERFILE);
	       if ( (res=D_getfilepath(D_USERFILE,filepath,curexpdir)) )
	       {
		 D_error(res);
		 return(ERROR);
	       }
	       if ( (res = D_open(D_USERFILE, filepath, &fidhead)) )
	       {
		 D_error(res);
		 return(ERROR);
	       }
	     }
	     ptr1 = outp;
	     ptr2 = finaloutp;
	     for (i=0;i<dsnpadj;i++)
	       *ptr2++ = *ptr1++;
	     outp = finaloutp;
	/* set these back to correct values */
	     tmpi = ftpar.fn0; ftpar.fn0 = dsfn0; dsfn0 = tmpi;
	     tmpi = npx; npx = dsnpx; dsnpx = tmpi;
	     tmpi = npadj; npadj = dsnpadj; dsnpadj = tmpi;
	     tmpi = pwr; pwr = dspwr; dspwr = tmpi;
             if (fpointmult != 1.0)
             {
                *outp *= fpointmult;
                *(outp + 1) *= fpointmult;
             }
	   }
           else if ( ! wtp.wtflag &&  (fpointmult != 1.0) )
           {
              *outp *= fpointmult;
              *(outp + 1) *= fpointmult;
           }

/*
           if (ftflag)
           {
              disp_status("FT  ");
              if (zfnumber > 0)
                 zerofill(outp + npadj, zfnumber);

              if ( ftpar.procstatus & (FT_F2PROC | LP_F2PROC) )
              {
                 fft(outp, ftpar.fn0/2, pwr, zflevel, COMPLEX, COMPLEX,
                           -1.0, FTNORM, (npadj + zfnumber)/2);
                 if (realt2data)
                    postproc(outp, ftpar.fn0/2, COMPLEX);
                 if ((ftpar.dspar.dsflag) && (ftpar.dspar.lp != 0.0))
                   rotate2_center(outp, ftpar.fn0/2, ftpar.dspar.lp*
                        (ftpar.fn0/2)/(ftpar.fn0/2-1.0), (double) 0.0);
                 if (fabs(ftpar.lpval) > MINDEGREE)
                   rotate2_center(outp, ftpar.fn0/2, ftpar.lpval*
                        (ftpar.fn0/2)/(ftpar.fn0/2-1.0), (double) 0.0);
              }
              else if (ftpar.procstatus & MEM_F2PROC)
              {
                 Werrprintf("MEM processing:  not currently supported");
                 releaseAllWithId("ft2d");
                 disp_index(0);
                 disp_status("       ");
                 ABORT;
              }
           }
           else
           {
              zerofill(outp + npadj, ftpar.fn0 - npadj);
           }
 */

           last = cblock + 1;
           if (ftpar.dspar.dsflag)
           {
        /* switch these again */
             tmpi = ftpar.fn0; ftpar.fn0 = dsfn0; dsfn0 = tmpi;
             tmpi = npx; npx = dsnpx; dsnpx = tmpi;
             tmpi = npadj; npadj = dsnpadj; dsnpadj = tmpi;
             tmpi = pwr; pwr = dspwr; dspwr = tmpi;
           }
        }

	outblock.head->scale = 0;
	outblock.head->ctcount = 1;  /* avoid double scaling of FID */
        setheader(&outblock, datahead.status, ftpar.D_dsplymode, cblock,
			ftpar.hypercomplex);
        first += step;
        if (nfft)
           ftpar.cf += step;
     }
     else
     {
        setheader(&outblock, 0, 0, cblock, ftpar.hypercomplex);
     }

     if ( (res = D_markupdated(D_DATAFILE, cblock)) )
     {
        D_error(res);
        releaseAllWithId("ft2d");
        disp_index(0);
        disp_status("        ");
        ABORT;
     }

     if ( (res = D_release(D_DATAFILE, cblock)) )
     {
        D_error(res);
        releaseAllWithId("ft2d");
        disp_index(0);
        disp_status("        ");
        ABORT;
     }

     cblock++;
     if (!blocksdone)
        blocksdone = (first > lastcblock);
  }

  releaseAllWithId("ft2d");
  disp_index(0);
  D_close(D_USERFILE);
  if (ftpar.ftarg.infoFD)
  {
     fclose(ftpar.ftarg.infoFD);
     ftpar.ftarg.infoFD = NULL;
  }


  if ( ((last != ftpar.nblocks) && (last != MAXINT)) || (ftpar.ftarg.fidsPerSpec > 1) )
  {
     if ( (res = D_gethead(D_DATAFILE, &datahead)) )
     {
        D_error(res);
        disp_index(0);
        disp_status("        ");
        ABORT;
     }

     datahead.nblocks = last;
     if ( (res = D_updatehead(D_DATAFILE, &datahead)) )
     {
        D_error(res);
        disp_index(0);
        disp_status("        ");
        ABORT;
     }
  }

  releasevarlist();
  releaseAllWithId("ft2d");
  D_close(D_USERFILE);
  D_flush(D_DATAFILE);
  D_trash(D_DATAFILE);
  D_trash(D_PHASFILE);
  if (fnActive)
  {
     P_setactive(CURRENT, "fn", ACT_ON);
     P_setreal(CURRENT,"fn", fnSave, 1);
  }

  // save procpar
  strcpy(filepath,newfidpath);
  strcat(filepath,"/procpar");
  if(npx == npadj) {
     saveProcpar(filepath);
  } else {

     double sw,at, oldat;
     P_getreal(PROCESSED,"at", &oldat, 1);
     P_setreal(PROCESSED,"np",(double)npadj,1);
     if(!P_getreal(PROCESSED,"sw",&sw,1) && sw > 0) {
        at = npadj/(2*sw);
        P_setreal(PROCESSED,"at",at,1);
     }
     saveProcpar(filepath);
     P_setreal(PROCESSED,"np",(double)npx,1);
     P_setreal(PROCESSED,"at",oldat,1);
  }


  if (retc > 0)
  {
     retv[0] = realString( (double)savefirst );
     if (retc > 1)
     {
        retv[1] = realString( (double)last );
     }
     else
     {
        Werrprintf("no more than two values returned by %s\n", argv[0]);
        ABORT;
     }
  }

  RETURN;
}

/*-----------------------------------------------
|						|
|		    ft()			|
|						|
|  This function performs a 1D FT on a single	|
|  or arrayed FID data set. Entry point.	|
|						|
+----------------------------------------------*/
int ft(int argc, char *argv[], int retc, char *retv[])
{
  char		msge[MAXSTR];
  char		filepath[MAXPATHL];
  int		status,
		res,
		pwr,
		nfft,
		cblock,
		lastcblock,
		blocksdone,
		lsfidx,
		fidnum = 0,
		arg_no,
		npx,
		npadj,
                do_ds,
		ftflag,
		noreal,
		element_no,
		lastfid,
		wtsize,
		first,
		savefirst,
		last,
		step,
		i,
		tmpi,
		zflevel,
		zfnumber,
		dsfn0,
		dsnpx,
		dsnpadj,
		dspwr,
		realt2data;
  int		inverseWT;
  float		*outp,
		*finaloutp = NULL,
		*ptr1,
		*ptr2,
		*wtfunc;
  short		*ptr2s;
  double	rx,
		tmp;
  dpointers	outblock,
		fidoutblock;
  dfilehead	fidhead,
		datahead,
		phasehead;
  lpstruct	parLPinfo;
  ftparInfo	ftpar;
  wtPar		wtp;
  vInfo		info;
  int fidshim = FALSE;
  int fidShift = 0;
  double        phase0 = 0.0;
  int doDispFT = 1;
  int doDispLP = 1;

  if (argc > 1 && strcmp(argv[1],"fidshim") == 0) {
      fidshim = TRUE;
  }

  if (!fidshim) {
      Wturnoff_buttons();
  }
  ftpar.procstatus = setprocstatus(S_NP, 0);

/************************************
*  Initialize all parameterizeable  *
*  variables                        *
************************************/

  arg_no = first = step = element_no = 1;
  ftpar.nblocks = 1;
  last = MAXINT;

  do_ds = noreal = ftflag = TRUE;
  nfft = acqflag = inverseWT = FALSE;
  ftpar.t2dc = -1;
  ftpar.zeroflag = FALSE;
  ftpar.sspar.lfsflag = ftpar.sspar.zfsflag = FALSE;
  ftpar.dspar.dsflag = FALSE;
  ftpar.dspar.fileflag = FALSE;
  ftpar.dspar.newpath[0] = '\0';
  ftpar.ftarg.useFtargs = 0;

/*********************************
*  Parse STRING arguments first  *
*********************************/

  while ( (argc > arg_no) && (noreal = !isReal(argv[arg_no])) )
  {
     if ((strcmp(argv[arg_no], "i") == 0) ||
     	   (strcmp(argv[arg_no], "inverse") == 0))
     {
        if (ift(argc, argv, retc, retv, arg_no))
        {
           disp_status("        ");
           ABORT;
        }
        else
        {
           RETURN;
        }
     }
     else if (strcmp(argv[arg_no], "all") == 0)
     {
        ftpar.nblocks = MAXINT;
     }
     else if (strcmp(argv[arg_no], "acq") == 0)
     {
        acqflag = TRUE;
     }
     else if (strcmp(argv[arg_no], "nodc") == 0)
     {
        ftpar.t2dc = FALSE;
     }
     else if (strcmp(argv[arg_no], "dodc") == 0)
     {
        ftpar.t2dc = TRUE;
     }
     else if (strcmp(argv[arg_no], "nods") == 0)
     {
        do_ds = FALSE;
     }
     else if (strcmp(argv[arg_no], "noft") == 0)
     {
        ftflag = FALSE;
     }
     else if (strcmp(argv[arg_no], "zero") == 0)
     {
        ftpar.zeroflag = TRUE;
     }
     else if (strcmp(argv[arg_no], "rev") == 0)
     {
        ftpar.zeroflag = -1;
     }
     else if (strcmp(argv[arg_no], "nf") == 0)
     {
        nfft = TRUE;
     }
     else if (strcmp(argv[arg_no], "fidshim") == 0)
     {
         fidshim = TRUE;
     }
     else if (strcmp(argv[arg_no], "inversewt") == 0)
     {
         inverseWT = TRUE;
     }
     else if (strcmp(argv[arg_no], "ftargs") == 0)
     {
         ftpar.ftarg.useFtargs = 1;
     }
     else if (strcmp(argv[arg_no], "nopars") == 0)
     {
         /* Pass this on to i_ft() */
     }
     else if (strcmp(argv[arg_no], "downsamp") == 0)
     {
       if (!P_getVarInfo(CURRENT, "downsamp", &info))
         if (info.active)
           if (!P_getreal(CURRENT, "downsamp", &tmp, 1))
	     if (tmp > 0.999)
	     {
	       ftpar.dspar.dsflag = TRUE;
	       ftpar.dspar.fileflag = TRUE;
	       arg_no++;
               if (argc <= arg_no || !isReal(argv[arg_no]))
               {
                 Werrprintf("usage  -  %s :  experiment number must follow 'downsamp'",argv[0]);
	         disp_status("        ");
	         ABORT;
	       }
	       if (check_other_experiment(ftpar.dspar.newpath,argv[arg_no], 1))
	       {
	         return(ERROR);
	       }
	     }
       if (!ftpar.dspar.dsflag)
       {
         Werrprintf("parameter 'downsamp' must be active and > 1");
         disp_status("        ");
         ABORT;
       }
     }
     else if (strcmp(argv[arg_no], "noop") != 0)
     {
        Werrprintf("usage  -  %s :  incorrect string argument", argv[0]);
        disp_status("        ");
	ABORT;
     }

     arg_no++;
  }

/******************************
*  Parse REAL arguments next  *
******************************/

  sprintf(msge,
	"usage  -  %s :  string arguments must precede numeric arguments\n",
	  argv[0]);

  if (argc > arg_no)
  {
     if (nfft)
     {
        if (isReal(argv[arg_no]))
        {
           element_no = (int) (stringReal(argv[arg_no++]));
        }
        else
        {
           Werrprintf(msge);
           ABORT;
        }
     }

     if (argc > arg_no)
     {
        if (isReal(argv[arg_no]))
        {
           first = last = (int) (stringReal(argv[arg_no++]));
           if (argc > arg_no)
           {
              if (isReal(argv[arg_no]))
              {
                 last = (int) (stringReal(argv[arg_no++]));
                 if (argc > arg_no)
                 {
                    if (isReal(argv[arg_no]))
                    {
                       step = (int) (stringReal(argv[arg_no++]));
                    }
                    else
                    {
                       Werrprintf(msge);
                       ABORT;
                    }
                 }
              }
              else
              {
                 Werrprintf(msge);
                 ABORT;
              }
           }
        }
        else
        {
           Werrprintf(msge);
           ABORT;
        }
     }
  }

/******************************
*  Adjust parameters for FT.  *
******************************/

  if (first < 1)
     first = 1;
  if (step < 1)
     step = 1;
  if (last < first)
     last = first;
  ftpar.nblocks = last;

/*****************************************
*  Modify the FT parameters if ft('nf')  *
*  was executed.                         *
*****************************************/

  if (nfft)
  {
     if ( (res = P_getreal(PROCESSED, "nf", &rx, 1)) )
     {
        P_err(res, "nf", ":");
        ABORT;
     }

     ftpar.nblocks = (int) (rx + 0.5);
     if (last > ftpar.nblocks)
     {
        last = ftpar.nblocks;
     }
     else if (ftpar.nblocks > last)
     {
        ftpar.nblocks = last;
     }
  }

/******************************
*  Initialize data files and  *
*  FT parameters.             *
******************************/

  if ( i_ft(argc, argv, (S_DATA | S_SPEC | S_FLOAT),
            0, 0, &ftpar, &fidhead, &datahead, &phasehead) )
  {
      disp_status("        ");
      ABORT;
  }
  if (ftpar.t2dc == -1)
  {
      ftpar.t2dc = (fidhead.status & S_DDR) ? FALSE : TRUE;
  }
  disp_current_seq();

  ftpar.cf = ( (nfft) ? first : ftpar.cf );	/* ranges from 1 to nf */

  if (acqflag)
  {
     first = ++specIndex;
  }
  else
  {
     specIndex = first;		/* tells interactive programs
				   that new data exist */
  }

/***************************************************
*  np0  =  total number of points in the fid       *
*  npx  =  used number of points in the fid        *
*  fn0  =  number of points in the converted file  *
*                                                  *
*  Adjust "npx" and "lsfidx".                      *
***************************************************/

  lsfidx = ftpar.lsfid0;

  if (ftpar.dspar.dsflag)
  {
    /* save values for use after filtering and downsampling */
    dsnpx =( (ftpar.fn0 < (ftpar.dspar.finalnp - lsfidx/ftpar.dspar.dsfactor)) ?
        (ftpar.fn0 + lsfidx/ftpar.dspar.dsfactor) : ftpar.dspar.finalnp );
    dsnpadj = dsnpx - lsfidx/ftpar.dspar.dsfactor;
    ftpar.dspar.finalnp = dsnpadj;
    dsfn0 = ftpar.fn0;
    dspwr = fnpower(dsfn0);

    /* values to use for processing before filtering and downsampling */
    /* must trick routines below into thinking fn0 >= np0 or they won't use
        all the points in the FID */
    ftpar.fn0 = (dsfn0*ftpar.dspar.dsfactor > ftpar.np0) ?
                 dsfn0*ftpar.dspar.dsfactor : ftpar.np0;
    tmpi = 32;
    while (tmpi < ftpar.fn0)
      tmpi *= 2;
    ftpar.fn0 = tmpi;
    npx = ( (ftpar.fn0 < (ftpar.np0 - lsfidx)) ? (ftpar.fn0 + lsfidx)
                : ftpar.np0 );
    npadj = npx - lsfidx;
    pwr = fnpower(ftpar.fn0);
  }
  else
  {
    npx = dsnpx = ( (ftpar.fn0 < (ftpar.np0 - lsfidx)) ? (ftpar.fn0 + lsfidx)
                : ftpar.np0 );
    npadj = dsnpadj = npx - lsfidx;     /* adjusted number of FID data points */
    pwr = dspwr = fnpower(ftpar.fn0);
    dsfn0 = ftpar.fn0;
  }
  fpointmult = getfpmult(S_NP, fidhead.status & S_DDR);
  if (ftpar.ftarg.numShift && (ftpar.ftarg.fidsPerSpec > 1) && ftpar.ftarg.numSa )
  {
     /* Assume PureShift */
     npadj = 2 * ftpar.ftarg.fidsPerSpec * *ftpar.ftarg.sa;
     if (npadj > ftpar.fn0)
        npadj = ftpar.fn0;
  }

  if (lsfidx < 0)
  {
     if (npx < 2)
     {
        Werrprintf("lsfid is too large in magnitude");
        ABORT;
     }
  }
  else
  {
     if (lsfidx >= npx)
     {
        Werrprintf("lsfid is too large in magnitude");
        ABORT;
     }
  }

  realt2data = (ftpar.procstatus & REAL_t2);

  if ( setlppar(&parLPinfo, S_NP, ftpar.procstatus, npadj/2, ftpar.fn0/2,
		  LPALLOC, "ft2d") )
  {
     disp_status("        ");
     releaseAllWithId("ft2d");
     Wsetgraphicsdisplay("");		/* TRY THIS!  SF */
     ABORT;
  }

  if (parLPinfo.sizeLP)
  {
     int	maxlpnp,
		nptmp;

     if (realt2data)
     {
        Werrprintf("LP analysis is not supported for real t2 data");
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }

     maxlpnp = npadj;

     for (i = 0; i < parLPinfo.sizeLP; i++)
     {
        lpparams = *(parLPinfo.parLP + i);

        if (lpparams.status & FORWARD)
        {
           nptmp = 2*(lpparams.startextpt + lpparams.ncextpt - 1);
           if (nptmp > maxlpnp)
              maxlpnp = nptmp;
        }
     }

     if (ftpar.dspar.dsflag)
     {
       dsnpadj += ((maxlpnp-npadj)/ftpar.dspar.dsfactor);
       if (dsnpadj > dsfn0)  dsnpadj = dsfn0;
       ftpar.dspar.finalnp += ((maxlpnp-npadj)/ftpar.dspar.dsfactor);
     }
     else
       dsnpadj = maxlpnp;
     npadj = maxlpnp;
  }

  if (ftpar.dspar.dsflag)
  {
    zflevel = getzflevel(dsnpadj, dsfn0);
    zfnumber = getzfnumber(dsnpadj, dsfn0);
  }
  else
  {
    zflevel = getzflevel(npadj, ftpar.fn0);
    zfnumber = getzfnumber(npadj, ftpar.fn0);
  }

  if (ftpar.dspar.dsflag)
  {
    ftpar.dspar.buffer = (double *)allocateWithId(sizeof(double)*ftpar.fn0,"ft2d");
    if (!ftpar.dspar.buffer)
    {
      Werrprintf("Error allocating digital filtering buffer");
      ftpar.dspar.dsflag = FALSE;
      ftpar.dspar.fileflag = FALSE;
      return(ERROR);
    }

    ftpar.dspar.data = (float *)allocateWithId(sizeof(float)*ftpar.fn0,"ft2d");
    if (!ftpar.dspar.data)
    {
      Werrprintf("Error allocating digital filtering data buffer");
      ftpar.dspar.dsflag = FALSE;
      ftpar.dspar.fileflag = FALSE;
      return(ERROR);
    }
  }

  if (ftpar.ftarg.fidsPerSpec > 1)
  {
    ftpar.ftarg.buffer = (float *)allocateWithId(sizeof(float)*ftpar.fn0,"ft2d");
    if (!ftpar.ftarg.buffer)
    {
      Werrprintf("Error allocating combination buffer");
      return(ERROR);
    }
    /* Don't to dc correction until FID is constructed */
    ftpar.ftarg.saveT2dc = ftpar.t2dc;
    ftpar.t2dc = FALSE;
  }

/****************************************
*  Initialize weighting parameters and  *
*  weighting vector.                    *
****************************************/

  wtp.wtflag = ftpar.wtflag;
  wtfunc = NULL;

  if (wtp.wtflag)
  {           /* allocate wtfunc, npadj/2 points, 4 bytes per point */
	      /* allocate wtfunc, npadj points if real FT */
     wtsize = npadj/2;
     if (realt2data)
        wtsize *= 2;

     if ((wtfunc = allocateWithId(sizeof(float)*wtsize, "ft2d")) == NULL)
     {
        Werrprintf("cannot allocate memory for weighting vector");
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }

     if (init_wt1(&wtp, S_NP))
     {
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }

     readwtflag = FALSE;	/* for user-defined weighting functions */
     if (init_wt2(&wtp, wtfunc, wtsize, realt2data, S_NP,
		(ftpar.dspar.dsflag) ? 1.0 :  fpointmult, readwtflag))
     {
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }

     if (!wtp.wtflag)
     {
        release(wtfunc);
        wtfunc = NULL;
     }
     else if (inverseWT)
     {
        int i;
        float *ptr;
        ptr = wtfunc;
        for (i=0; i<wtsize; i++)
        {
           if ( *ptr )
           {
              *ptr = 1.0 / *ptr;
              ptr++;
           }
           else
           {
              Werrprintf("Cannot invert weighting function with a zero in it.");
              releaseAllWithId("ft2d");
              disp_status("        ");
              ABORT;
           }
        }
     }
  }

/**************************
*  Start loop over FIDs.  *
**************************/

  lastfid = ftpar.ni0 * ftpar.ni1 * ftpar.arraydim;
  if (nfft)
  {
     if ((element_no < 1) || (element_no > lastfid))
     {
        Werrprintf("ft  -  FID index must be between 1 and %d\n", lastfid);
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }
  }
  else
  {
     if (lastfid > ftpar.nblocks * ftpar.ftarg.fidsPerSpec)
        lastfid = ftpar.nblocks * ftpar.ftarg.fidsPerSpec;
     if (first > lastfid)
     {
        last = lastfid;
        if (acqflag)
           specIndex--;
     }
  }

  status = (S_DATA | S_SPEC | S_FLOAT | S_COMPLEX | ftpar.D_cmplx);

/***********************************************
*  Necessary until I can devise a function to  *
*  read only the FID block header in order to  *
*  determine if FID data exists in that block. *
***********************************************/

  blocksdone = FALSE;
  lastcblock = ftpar.nblocks;

  if (nfft)
  {
     if (acqflag)
     {
        Werrprintf("ft  -  ft('acq','nf') are incomensurate\n");
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }

     cblock = 0;
     fidnum = element_no - 1;
  }
  else
  {
     cblock = ( (acqflag) ? (first - 1) : 0 );
  }

  savefirst = first;

  while ((cblock < lastcblock) && (!blocksdone))
  {
     int fidCnt = 0;
     float *combineptr = NULL;

     DPRINT1("block %d\n", cblock);
     if ( (res = D_allocbuf(D_DATAFILE, cblock, &outblock)) )
     {
        D_error(res);
        releaseAllWithId("ft2d");
        disp_status("        ");
        ABORT;
     }
     outblock.head->ctcount = 0;  /* default setting */
     outblock.head->scale = 0;    /* default setting */

     if (ftpar.dspar.dsflag)
     {
       outp = ftpar.dspar.data;
       finaloutp = (float *)outblock.data;
     }
     else
     {
       outp = (float *)outblock.data;
     }

/**********************************************************
*  Start filling at the start of the output data buffer.  *
*  This facilitates the automatic array-like processing   *
*  of 'cf' and 'nf' in 1D.                                *
**********************************************************/

     if (!nfft)
        fidnum = cblock;
     
     if (interuption)
     {
        releaseAllWithId("ft2d");
        D_trash(D_PHASFILE);
        D_trash(D_DATAFILE);
        disp_status("        ");
        ABORT;
     }

     if ((cblock + 1) == first)
     {
      fidCnt = 0;
      if (ftpar.ftarg.fidsPerSpec > 1)
      {
         combineptr = outp;
         outp = ftpar.ftarg.buffer;
         zerofill(combineptr, ftpar.fn0 );
         zerofill(outp, ftpar.fn0 );
      }
      while ( (fidCnt < ftpar.ftarg.fidsPerSpec) && status)
      {
        if (ftpar.ftarg.infoFD)
        {
           if ( ! fidCnt)
              fprintf(ftpar.ftarg.infoFD,"Fid %d\n",cblock+1);
           fprintf(ftpar.ftarg.infoFD,"   Component Fid %d\n",fidCnt+1);
        }
        if ( getfid((fidnum)*ftpar.ftarg.fidsPerSpec+fidCnt, outp, &ftpar, &fidhead, &lastfid) )
	{
	   Werrprintf("Unable to get FID data");
           releaseAllWithId("ft2d");
	   disp_index(0);
           disp_status("        ");
           ABORT;
        }
        fidCnt++;

	if (lastfid == 0)
	{
           releaseAllWithId("ft2d");
	   disp_index(0);
	   disp_status("        ");
           ABORT;
        }
        else if ((cblock == lastfid) && (!nfft))
	{
           status = 0;
           blocksdone = TRUE;
           if (last == MAXINT)
           {
              last = cblock;
           }
	}
        else
        {
           if (ftpar.ftarg.eccPnts)
           {
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      ECC correction using file %s and shift = %d\n",
                         ftpar.ftarg.eccFile, ftpar.ftarg.eccLsfid);
              }
              eccCorr(outp, ftpar);
           }
           if (ftpar.ftarg.numFreq > 0)
           {
              double ph1;
              int index;
              index = ftpar.ftarg.curFreq % ftpar.ftarg.numFreq;
              ftpar.ftarg.curFreq++;
              ph1 = *(ftpar.ftarg.freq+index);
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Frequency shift = %g\n", ph1);
              }
              rotate_fid(outp, (double) 0.0, ph1, ftpar.np0 - ftpar.lsfid0, COMPLEX);
           }
           else if (ftpar.ftarg.numFreq == -1)
           {
              double ph1;
              ph1 = ftpar.ftarg.initFreq + (fidCnt-1) * ftpar.ftarg.incrFreq;
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Incrementing Frequency shift = %g\n", ph1);
              }
              rotate_fid(outp, (double) 0.0, ph1, ftpar.np0 - ftpar.lsfid0, COMPLEX);
              
           }
           if (ftpar.ftarg.phasePnts)
           {
              if ( ! ftpar.ftarg.autoPhaseInit )
              {
                 initAutoPhase(&ftpar, outp);
                 ftpar.ftarg.autoPhaseInit = 1;
                 if (strlen(ftpar.ftarg.autoPhasePar))
                    P_setreal(CURRENT,ftpar.ftarg.autoPhasePar,0.0,0);
                 ftpar.ftarg.curPhase = 1;
                 phase0 = calcAutoPhase(&ftpar, outp);
                 if (ftpar.ftarg.infoFD)
                 {
                    fprintf(ftpar.ftarg.infoFD,"      Auto-calculated reference rotatation = %g\n", phase0);
                 }

              }
              else
              {
                 double ph0;
                 ph0 = calcAutoPhase(&ftpar, outp) - phase0;
                 if (ftpar.ftarg.infoFD)
                 {
                    fprintf(ftpar.ftarg.infoFD,"      Auto-calculated phase rotatation = %g\n", ph0);
                 }
                 ftpar.ftarg.curPhase++;
                 if (strlen(ftpar.ftarg.autoPhasePar))
                    P_setreal(CURRENT,ftpar.ftarg.autoPhasePar,ph0,ftpar.ftarg.curPhase);
                 rotate_fid(outp, ph0, (double) 0.0, ftpar.np0 - ftpar.lsfid0, COMPLEX);
              }
           }
           else if (ftpar.ftarg.numPhase)
           {
              double ph0;
              int index;
              index = ftpar.ftarg.curPhase % ftpar.ftarg.numPhase;
              ftpar.ftarg.curPhase++;
              ph0 = *(ftpar.ftarg.phase+index);
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Phase rotatation = %g\n", ph0);
              }
              rotate_fid(outp, ph0, (double) 0.0, ftpar.np0 - ftpar.lsfid0, COMPLEX);
           }

           if (ftpar.ftarg.numSa)
           {
              register int npntSa;
              register int fpntSa;
              register int index;
              register int npx;
              register float *ptr;

              index = ftpar.ftarg.curSa % ftpar.ftarg.numSa;
              ftpar.ftarg.curSa++;
              npntSa = *(ftpar.ftarg.sa+index);
              if (ftpar.ftarg.numSas)
              {
                 index = ftpar.ftarg.curSas % ftpar.ftarg.numSas;
                 ftpar.ftarg.curSas++;
                 fpntSa = *(ftpar.ftarg.sas+index);
              }
              else
              {
                 fpntSa = 0;
              }
              /* convert from complex pairs to sum of reals and imaginaries */
              fpntSa *= 2;
              npntSa *= 2;

              index = 0;
              npx = ftpar.np0 - ftpar.lsfid0;
              ptr = outp;
              while ( (index < npx) && (index < fpntSa) )
              {
                 *ptr++ = 0.0;
                 *ptr++ = 0.0;
                 index += 2;
              }
              ptr = outp+fpntSa+npntSa;
              index = fpntSa+npntSa;
              while (index < npx)
              {
                 *ptr++ = 0.0;
                 *ptr++ = 0.0;
                 index += 2;
              }
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Sampling window from point %d for %d points\n", fpntSa/2, npntSa/2);
              }
           }
           if (ftpar.ftarg.numAmp)
           {
              register float ampl;
              register int index;
              register int npx;
              register float *ptr;

              index = ftpar.ftarg.curAmp % ftpar.ftarg.numAmp;
              ftpar.ftarg.curAmp++;
              ampl = (float) *(ftpar.ftarg.amp+index);
              index = 0;
              npx = ftpar.np0 - ftpar.lsfid0;
              ptr = outp;
              while (index < npx)
              {
                 *ptr++ *= ampl;
                 *ptr++ *= ampl;
                 index += 2;
              }
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Amplitude multiplication = %g\n", ampl);
              }
           }
           if (ftpar.sspar.zfsflag || ftpar.sspar.lfsflag)
           {
              disp_status("SS  ");
              if ( fidss(ftpar.sspar, outp, npx/2, lsfidx/2) )
              {
                 Werrprintf("time-domain solvent subtraction failed");
                 releaseAllWithId("ft2d");
                 disp_index(0);
                 disp_status("        ");
                 ABORT;
              }
           }

           if (parLPinfo.sizeLP)
           {
              for (i = 0; i < parLPinfo.sizeLP; i++)
              {
                 lpparams = *(parLPinfo.parLP + i);
                 if ( doDispLP )
                 {
                    disp_status(lpparams.label);
                    doDispLP = 0;
                 }

                 if (lpz(fidnum, outp, (npx - lsfidx)/2, lpparams))
                 {
                    Werrprintf("LP analysis failed");
                    releaseAllWithId("ft2d");
                    disp_index(0);
                    disp_status("       ");
                    ABORT;
                 }
              }
           }
           if (ftpar.ftarg.numShift > 0)
           {
              int npx;
              int index;
              index = ftpar.ftarg.curShift % ftpar.ftarg.numShift;
              ftpar.ftarg.curShift++;
              fidShift = *(ftpar.ftarg.shift+index);
              npx = ftpar.np0 - ftpar.lsfid0;
              if (fidShift > npadj/2)
                 fidShift =  npadj/2;
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Data shift = %d\n", fidShift);
              }
              if (fidShift != 0)
              {
                 shiftComplexData(outp, fidShift, npx / 2, ftpar.fn0/2); 
              }
           }
           else if (ftpar.ftarg.numShift == -1)
           {
              int npx;
              fidShift = ftpar.ftarg.initShift + (fidCnt-1) * ftpar.ftarg.incrShift;
              npx = ftpar.np0 - ftpar.lsfid0;
              if (fidShift > npadj/2)
                 fidShift =  npadj/2;
              if (ftpar.ftarg.infoFD)
              {
                 fprintf(ftpar.ftarg.infoFD,"      Incrementing Data shift = %d\n", fidShift);
              }
              if (fidShift != 0)
              {
                 int npx;
                 npx = ftpar.np0 - ftpar.lsfid0;
                 shiftComplexData(outp, fidShift, npx / 2, ftpar.fn0/2); 
              }
           }
           if (ftpar.ftarg.fidsPerSpec > 1)
           {
              combine(outp, combineptr, ftpar.fn0/2, COMPLEX,
                      ftpar.ftarg.rr, ftpar.ftarg.ir,
                      ftpar.ftarg.ri, ftpar.ftarg.ii);
           }
           else if (ftpar.ftarg.multfid)
           {
              register int i;
              register float re, im;

              combineptr = outp;
              for (i = 0; i < ftpar.fn0/2; i++)
              {
                 re = *combineptr * ftpar.ftarg.rr + *(combineptr+1) * ftpar.ftarg.ir;
                 im = *combineptr * ftpar.ftarg.ri + *(combineptr+1) * ftpar.ftarg.ii;
                 (*combineptr++) = re;
                 (*combineptr++) = im;
              }
           }
          }
        }
        if (status)
        {
           if (ftpar.ftarg.fidsPerSpec > 1)
           {
              outp = combineptr;
              if (ftpar.ftarg.saveT2dc)
                 driftcorrect_fid(outp, npadj/2, 0, COMPLEX);
           }

           if (wtp.wtflag)
           {
              if (!ftflag && doDispFT)
                 disp_status("WT  ");
              weightfid(wtfunc, outp, npadj/2, realt2data, COMPLEX);
           }

	   if (ftpar.dspar.dsflag)
	   {
	     disp_status("DIGFILT");
	     if (downsamp(&(ftpar.dspar), outp, npadj/2, 0,
			ftpar.fn0, ftpar.procstatus&REAL_t2) )
	     {
		 Werrprintf("digital filtering failed");
		 releaseAllWithId("ft2d");
		 disp_index(0);
		 disp_status("        ");
		 ABORT;
	     }
	     if (ftpar.dspar.fileflag)
	     {
	       D_close(D_USERFILE);
	       if ( (res=D_getfilepath(D_USERFILE,filepath,ftpar.dspar.newpath)) )
	       {
		 D_error(res);
		 return(ERROR);
	       }
	       if ( (res = D_open(D_USERFILE, filepath, &fidhead)) )
	       {
		 D_error(res);
		 return(ERROR);
	       }

	       ptr1 = outp;
	       D_allocbuf(D_USERFILE, cblock, &fidoutblock);
               setheader(&fidoutblock,fidhead.status,ftpar.D_dsplymode,cblock+1,
				ftpar.hypercomplex);
	       fidoutblock.head->lpval = ftpar.dspar.lp;
	       fidoutblock.head->lvl = ftpar.dspar.lvl;
	       fidoutblock.head->tlt = ftpar.dspar.tlt;
	       fidoutblock.head->scale = 0;
	       fidoutblock.head->ctcount = 1;  /* avoid double scaling of FID */
	       if (ftpar.dspar.dp)
	       {
	         ptr2 = (float *)fidoutblock.data;
	         for (i=0;i<dsnpadj;i++)
	           *ptr2++ = (*ptr1++);
                 if (fpointmult != 1.0)
                 {
	            ptr2 = (float *)fidoutblock.data;
                    *ptr2 *= fpointmult;
                    *(ptr2 + 1) *= fpointmult;
                 }
	       }
	       else
	       {
	         ptr2s = (short *)fidoutblock.data;
	         for (i=0;i<dsnpadj;i++)
	           *ptr2s++ = (short)((*ptr1++)+0.5);
	       }
	       if ( (res = D_markupdated(D_USERFILE, cblock)) )
	       {
		 D_error(res);
		 releaseAllWithId("ft2d");
		 disp_index(0);
		 disp_status("        ");
		 ABORT;
	       }

	       if ( (res = D_release(D_USERFILE, cblock)) )
	       {
		 D_error(res);
		 releaseAllWithId("ft2d");
		 disp_index(0);
		 disp_status("        ");
		 ABORT;
	       }

	       D_close(D_USERFILE);
	       if ( (res=D_getfilepath(D_USERFILE,filepath,curexpdir)) )
	       {
		 D_error(res);
		 return(ERROR);
	       }
	       if ( (res = D_open(D_USERFILE, filepath, &fidhead)) )
	       {
		 D_error(res);
		 return(ERROR);
	       }
	     }
	     ptr1 = outp;
	     ptr2 = finaloutp;
	     for (i=0;i<dsnpadj;i++)
	       *ptr2++ = *ptr1++;
	     outp = finaloutp;
	/* set these back to correct values */
	     tmpi = ftpar.fn0; ftpar.fn0 = dsfn0; dsfn0 = tmpi;
	     tmpi = npx; npx = dsnpx; dsnpx = tmpi;
	     tmpi = npadj; npadj = dsnpadj; dsnpadj = tmpi;
	     tmpi = pwr; pwr = dspwr; dspwr = tmpi;
             if (fpointmult != 1.0)
             {
                *outp *= fpointmult;
                *(outp + 1) *= fpointmult;
             }
	   }
           else if ( ! wtp.wtflag &&  (fpointmult != 1.0) )
           {
              *outp *= fpointmult;
              *(outp + 1) *= fpointmult;
           }

           if (ftflag)
           {
              if (doDispFT)
              {
                 disp_status("FT  ");
                 doDispFT = 0;
              }
              if (zfnumber > 0)
                 zerofill(outp + npadj, zfnumber);

              if ( ftpar.procstatus & (FT_F2PROC | LP_F2PROC) )
              {
                 fft(outp, ftpar.fn0/2, pwr, zflevel, COMPLEX, COMPLEX,
                           -1.0, FTNORM, (npadj + zfnumber)/2);
                 if (realt2data)
                    postproc(outp, ftpar.fn0/2, COMPLEX);
                 if ((ftpar.dspar.dsflag) && (ftpar.dspar.lp != 0.0))
                   rotate2_center(outp, ftpar.fn0/2, ftpar.dspar.lp*
                        (ftpar.fn0/2)/(ftpar.fn0/2-1.0), (double) 0.0);
                 if (fabs(ftpar.lpval) > MINDEGREE)
                   rotate2_center(outp, ftpar.fn0/2, ftpar.lpval*
                        (ftpar.fn0/2)/(ftpar.fn0/2-1.0), (double) 0.0);
              }
              else if (ftpar.procstatus & MEM_F2PROC)
              {
                 Werrprintf("MEM processing:  not currently supported");
                 releaseAllWithId("ft2d");
                 disp_index(0);
                 disp_status("       ");
                 ABORT;
              }
           }
           else
           {
              zerofill(outp + npadj, ftpar.fn0 - npadj);
           }

           last = cblock + 1;
           if (ftpar.dspar.dsflag)
           {
        /* switch these again */
             tmpi = ftpar.fn0; ftpar.fn0 = dsfn0; dsfn0 = tmpi;
             tmpi = npx; npx = dsnpx; dsnpx = tmpi;
             tmpi = npadj; npadj = dsnpadj; dsnpadj = tmpi;
             tmpi = pwr; pwr = dspwr; dspwr = tmpi;
           }
        }

        setheader(&outblock, status, ftpar.D_dsplymode, cblock,
			ftpar.hypercomplex);
        first += step;
        if (nfft)
           ftpar.cf += step;
     }
     else
     {
        setheader(&outblock, 0, 0, cblock, ftpar.hypercomplex);
     }

     if ( (res = D_markupdated(D_DATAFILE, cblock)) )
     {
        D_error(res);
        releaseAllWithId("ft2d");
        disp_index(0);
        disp_status("        ");
        ABORT;
     }

     if ( (res = D_release(D_DATAFILE, cblock)) )
     {
        D_error(res);
        releaseAllWithId("ft2d");
        disp_index(0);
        disp_status("        ");
        ABORT;
     }

     cblock++;
     if (!blocksdone)
        blocksdone = (first > lastcblock);
  }

  releaseAllWithId("ft2d");
  disp_index(0);
  D_close(D_USERFILE);
  if (ftpar.ftarg.infoFD)
  {
     fclose(ftpar.ftarg.infoFD);
     ftpar.ftarg.infoFD = NULL;
  }


  if ( (last != ftpar.nblocks) && (last != MAXINT) )
  {
     if ( (res = D_gethead(D_DATAFILE, &datahead)) )
     {
        D_error(res);
        disp_index(0);
        disp_status("        ");
        ABORT;
     }

     datahead.nblocks = last;
     if ( (res = D_updatehead(D_DATAFILE, &datahead)) )
     {
        D_error(res);
        disp_index(0);
        disp_status("        ");
        ABORT;
     }
  }

  if (!Bnmr && do_ds)
  {
     releasevarlist();
     appendvarlist("cr");
     Wsetgraphicsdisplay("ds");		/* activate the ds program */
     start_from_ft = 1;
  }
  set_vnmrj_ft_params( 1, argc, argv );

  disp_index(0);
  disp_status("    ");

  if (retc > 0)
  {
     retv[0] = realString( (double)savefirst );
     if (retc > 1)
     {
        retv[1] = realString( (double)last );
     }
     else
     {
        Werrprintf("no more than two values returned by %s\n", argv[0]);
        ABORT;
     }
  }

  RETURN;
}


/************************************/
int check_other_experiment(char *exppath, char *exp_no, int showError)
/************************************/
{
/*  On VMS, use "expN.dir" format in call to access().  Then
    convert to [.expN] format for current experiment check.	*/

  int exptmp = 0;
  if (!(exptmp = atoi(exp_no)) || (exptmp < 1) || (exptmp > MAXEXPS))
    {
      if (showError)
         Werrprintf("illegal experiment number");
      ABORT;
    }
  strcpy(exppath,userdir);
  strcat(exppath,"/exp");
  strcat(exppath,exp_no);
  if (access(exppath,6))
    {
      if (showError)
         Werrprintf("experiment %s is not accessible ",exppath);
      ABORT;
    }

  if (strcmp(exppath,curexpdir)==0)
    {
      if (showError)
         Werrprintf("cannot store in current experiment");
      ABORT;
    }
  RETURN;
}


/*************************************************************/
void cmplx_dc(float *inp, float *lvl_re, float *lvl_im,
              float *tlt_re, float *tlt_im, int npnts, int fraction)
/*************************************************************/
{ int i;

  i = npnts / fraction;
  vrsum(inp,  2,lvl_re,i);
  vrsum(inp+1,2,lvl_im,i);
  *lvl_re /= (float)i;
  *lvl_im /= (float)i;
  inp += 2 * (npnts - i -1);
  vrsum(inp,  2,tlt_re,i);
  vrsum(inp+1,2,tlt_im,i);
  *tlt_re /= (float)i;
  *tlt_im /= (float)i;
  *tlt_re = (*lvl_re - *tlt_re) / (float)(npnts - i);
  *lvl_re = - *lvl_re + *tlt_re * (float)( i / 2 );
  *tlt_im = (*lvl_im - *tlt_im) / (float)(npnts - i);
  *lvl_im = - *lvl_im + *tlt_im * (float)( i / 2 );
}


/**********************************/
void ift_apod(float *in1, int fraction, int np, int fn)
/**********************************/
{ register int    i;
  register float  start;
  register float  incr;
  register float *ptr1;
  register float *ptr2;

  i = np / 2;           /* count complex pairs */
  ptr1 = in1 + i - 1;
  ptr2 = in1 + fn - i;
  DPRINT4("in1= %d, ptr1= %d, ptr2= %d, i= %d\n",in1,ptr1,ptr2,i);
  i /= fraction;
  incr = 1.0 / (float) (i - 1);
  start = 0.0;
  while (i--)
  {
    *ptr1-- *= start;
    *ptr1-- *= start;
    *ptr2++ *= start;
    *ptr2++ *= start;
    start   += incr;
  }
}


/*-----------------------------------------------
|						|
|		     ift()			|
|						|
|   This function performs an inverse FT on	|
|   the spectral data.				|
|						|
+----------------------------------------------*/
static int ift(int argc, char *argv[], int retc, char *retv[], int arg_no)
{
  int			pwr,
			cblock,
			res,
			fidnum;
/*  int    		dc_correct = TRUE; */	/* hard-coded for now */
  register int		i,
			ntval;
  float			lvl_re,
			tlt_re,
			lvl_im,
			tlt_im,
  			*inp;
  register float	*outp;
  double		tmp;
  dpointers		inblock,
			outblock;
  dfilehead		fidhead,
			datahead;
  ftparInfo		ftpar;


/*****************************************
*  IFT cannot currently process arrays.  *
*  Normalization will fail if "nt" has 	 *
*  been arrayed.                         *
*****************************************/

  if (argv[0][0] == 'w')
  {
     Werrprintf("no weighting during inverse ft allowed");
     ABORT;
  }

  arg_no++;		/* skip the argument 'inverse' */
  if ( i_ift(argc, argv, arg_no, &ftpar, &fidhead, &datahead) )
     ABORT;

  pwr = fnpower(ftpar.fn0);
  ntval = 1;
  if (!P_getreal(PROCESSED, "nt", &tmp, 1))
  {
     ntval = (int) (tmp + 0.5);
     if (ntval < 1)
        ntval = 1;
  }

/************************************************
*  "fidhead.nblocks" is actually the number of  *
*  blocks in the source DATAFILE.               *
************************************************/

  disp_status("IFT    ");
  for (cblock = 0; cblock < fidhead.nblocks; cblock++)
  {
     DPRINT1("block %d\n", cblock);
     if ( (res = D_getbuf(D_DATAFILE, fidhead.nblocks, cblock, &inblock)) )
     {
        D_error(res);
        ABORT;
     }

     if ( (res = D_allocbuf(D_USERFILE, cblock, &outblock)) )
     {
        D_error(res);
        ABORT;
     }

     fidnum = cblock;
     if (!((fidnum + 1) & 15))
        disp_index(fidnum + 1);

/**********************************************
*  Check status of data in block.  Perform a  *
*  complex DC correction.  Then perform the   *
*  inverse FT on the spectral data.           *
**********************************************/

     if ( (inblock.head->status & (S_DATA|S_SPEC|S_FLOAT|S_COMPLEX)) ==
		(S_DATA|S_SPEC|S_FLOAT|S_COMPLEX) )
     {
        inp  = (float *)inblock.data;		
        outp = (float *)outblock.data;

/*        if (dc_correct) */
        if (ftpar.fn0 != ftpar.np0)
        {
           cmplx_dc(inp, &lvl_re, &lvl_im, &tlt_re, &tlt_im, ftpar.np0/2,
			CMPLX_DC);
           vvrramp(inp, 2, inp, 2, lvl_re, tlt_re, ftpar.np0/2);
           vvrramp(inp+1, 2, inp+1, 2, lvl_im, tlt_im, ftpar.np0/2);
        }

        movmem((char *)(inp + ftpar.np0/2), (char *)outp,
			sizeof(float)*ftpar.np0/2, 1, 4);
        movmem((char *)inp, (char *)(outp + ftpar.fn0 - ftpar.np0/2),
			sizeof(float)*ftpar.np0/2, 1, 4);

        i = ftpar.fn0 - ftpar.np0;	/* zerofill for IFT expansions */
        if (i > 0)
        {
           outp += ftpar.np0/2;
           while (i--)
              *outp++ = 0.0;
           outp = (float *)outblock.data;	/* reset pointer */
        }

/*********************************************************
*  The complex DC correction and spectral apodization    *
*  attempt to minimize the discontinuity between the     *
*  spectral baseline and the subsequent zeroes if the    *
*  FID is expanded.  The presence of this discontinuity  *
*  convolves a sin(x)/x function with the time domain    *
*  data and can lead to sometimes confusing results.     *
*********************************************************/

        if (ftpar.fn0 != ftpar.np0)
        {
           ift_apod(outp, APODIZE, ftpar.np0, ftpar.fn0);
        }

/*************************************************
*  To be rigorously correct, the scaling factor  *
*  for the IFT should be FTNORM/CTvalue.         *
*************************************************/

        fft(outp, ftpar.fn0/2, pwr, 0, COMPLEX, COMPLEX, 1.0,
		FTNORM , ftpar.np0/2);

        setheader(&outblock, (S_DATA|S_FLOAT|S_COMPLEX),
	   	   1, cblock, ftpar.hypercomplex);
        /* Remove default fpmult */
        if ( (datahead.status & S_DDR) == S_DDR)
        {
           *outp *= 2.0;
           *(outp+1) *= 2.0;
        }
        outblock.head->ctcount = 1;	/* default setting */
        outblock.head->scale = 0;	/* default setting */
        outblock.head->index = 1;	/* default setting */
     }
     else
     {
        setheader(&outblock, 0, 0, cblock, ftpar.hypercomplex);
     }

     if ( (res = D_release(D_DATAFILE, cblock)) )
     {
        D_error(res);
        disp_status("      ");
        ABORT;
     }

     if ( (res = D_markupdated(D_USERFILE, cblock)) )
     {
        D_error(res);
        disp_status("      ");
        ABORT;
     }

     if ( (res = D_release(D_USERFILE, cblock)) )
     {
        D_error(res);
        disp_status("      ");
        ABORT;
     }
  }

  disp_status("       ");
  disp_index(0);
  if ( (res = D_close(D_USERFILE)) )
  {
     D_error(res);
     ABORT;
  }

  Wscrprintf("\n\nexpanded fid stored in exp%s\n", argv[arg_no]);
  RETURN;
}

void eccCorr(float *outp, ftparInfo ftpar)
{
   double phi_w;
   double phi_w_ref = 0.0;
   double phi_m;
   double phi_m_ref = 0.0;
   double re, im, absdata;
   int j;

   for (j=0; j<ftpar.np0/2; j++)
   {
      if (j < ftpar.ftarg.eccPnts - ftpar.ftarg.eccLsfid)
      {
         phi_w = *(ftpar.ftarg.ecc + j + ftpar.ftarg.eccLsfid);
         if (j == 0)
            phi_w_ref = phi_w;
         phi_w -= phi_w_ref;
         re = (double) *(outp+j);
         im = (double) *(outp+j+1);
         phi_m = atan2(im,re);
         if (j == 0)
            phi_m_ref = phi_m;
         phi_m = phi_m - phi_m_ref - phi_w;
         absdata = sqrt(re*re + im*im);
         *(outp+j) = (float) (absdata * cos(phi_m));
         *(outp+j+1) = (float) (absdata * sin(phi_m));
      }
   }
}

/*-----------------------------------------------
|						|
|		    ft()			|
|						|
|  This function calculates relative phase of fid
|						|
+----------------------------------------------*/
int calcECC(int argc, char *argv[], int retc, char *retv[])
{
   FILE *infile, *outfile;
   char inpath[MAXPATH];
   dfilehead	fidhead;
   dblockhead	blockhead;
   int np0;
   int ebytes;
   int data_int;
   short data_short;
   double phi_w;
   double phi_w_ref = 0.0;
   double re;
   double im;
   int status;
   int i,k;
   union u_tag {
      float fval;
      int   ival;
   } uval;

   if (argc < 2)
   {
      Wscrprintf("%s: requires input and output file names\n", argv[0]);
      ABORT;
   }
   sprintf(inpath,"%s/fid",argv[1]);
   if ( (infile = fopen(inpath,"r")) == NULL)
   {
      Wscrprintf("%s: cannot read FID data from %s\n", argv[0], inpath);
      ABORT;
   }
   if ( (outfile = fopen(argv[2],"w")) == NULL)
   {
      Wscrprintf("%s: cannot write results to %s\n", argv[0], argv[2]);
      fclose(infile);
      ABORT;
   }

   if (fread(&fidhead,sizeof(fidhead),1,infile) != 1)
   {
      Wscrprintf("Error in reading data file header\n");
      fclose(infile);
      fclose(outfile);
      ABORT;
   }
   if (fread(&blockhead,sizeof(blockhead),1,infile) != 1)
   {
      Wscrprintf("Error in reading data block header\n");
      fclose(infile);
      fclose(outfile);
      ABORT;
   }
   np0 = ntohl(fidhead.np) / 2;
   phi_w = (double) np0;
   ebytes = ntohl(fidhead.ebytes);
   status = htons(fidhead.status);
   if (fwrite(&phi_w,sizeof(double),1,outfile) != 1)
   {
      Wscrprintf ("Error in writing np\n");
      fclose(infile);
      fclose(outfile);
      ABORT;
   }
   if ( (status & S_FLOAT) == S_FLOAT )
   {
      k = 0;
      for (i=0; i<np0; i++)
      {
         if (fread(&data_int,sizeof(data_int),1,infile) != 1)
         {
            Wscrprintf ("Error in reading floating point re (water)\n");
            fclose(infile);
            fclose(outfile);
            ABORT;
         }
         uval.ival = ntohl(data_int);
         re = (double) uval.fval;
         if (fread(&data_int,sizeof(data_int),1,infile) != 1)
         {
            Wscrprintf ("Error in reading floating point imag (water)\n");
            fclose(infile);
            fclose(outfile);
            ABORT;
         }
         uval.ival = ntohl(data_int);
         im = (double) uval.fval;
         phi_w = atan2(im,re);
         if (k == 0)
            phi_w_ref = phi_w;
         k++;
         phi_w = phi_w - phi_w_ref;
         if (fwrite(&phi_w,sizeof(double),1,outfile) != 1)
         {
            Wscrprintf ("Error in writing phase\n");
            fclose(infile);
            fclose(outfile);
            ABORT;
         }
      }
   }
   else if ( (status & S_32) == S_32 )
   {
      k = 0;
      for (i=0; i<np0; i++)
      {
         if (fread(&data_int,sizeof(data_int),1,infile) != 1)
         {
            Wscrprintf ("Error in reading integer point re (water)\n");
            fclose(infile);
            fclose(outfile);
            ABORT;
         }
         uval.ival = ntohl(data_int);
         re = (double) uval.ival;
         if (fread(&data_int,sizeof(data_int),1,infile) != 1)
         {
            Wscrprintf ("Error in reading integer point imag (water)\n");
            fclose(infile);
            fclose(outfile);
            ABORT;
         }
         uval.ival = ntohl(data_int);
         im = (double) uval.ival;
         phi_w = atan2(im,re);
         if (k == 0)
            phi_w_ref = phi_w;
         k++;
         phi_w = phi_w - phi_w_ref;
         if (fwrite(&phi_w,sizeof(double),1,outfile) != 1)
         {
            Wscrprintf ("Error in writing phase\n");
            fclose(infile);
            fclose(outfile);
            ABORT;
         }
      }
   }
   else
   {
      k = 0;
      for (i=0; i<np0; i++)
      {
         if (fread(&data_short,sizeof(data_short),1,infile) != 1)
         {
            Wscrprintf ("Error in reading short point re (water)\n");
            fclose(infile);
            fclose(outfile);
            ABORT;
         }
         re = (double) ntohs(data_short);
         if (fread(&data_short,sizeof(data_short),1,infile) != 1)
         {
            Wscrprintf ("Error in reading short point imag (water)\n");
            fclose(infile);
            fclose(outfile);
            ABORT;
         }
         im = (double) ntohs(data_short);
         phi_w = atan2(im,re);
         if (k == 0)
            phi_w_ref = phi_w;
         k++;
         phi_w = phi_w - phi_w_ref;
         if (fwrite(&phi_w,sizeof(double),1,outfile) != 1)
         {
            Wscrprintf ("Error in writing phase\n");
            fclose(infile);
            fclose(outfile);
            ABORT;
         }
      }
   }
   fclose(infile);
   fclose(outfile);
   RETURN;
}
