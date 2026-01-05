/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------
|					|
|		set3dproc.c		|
|					|
+--------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include <errno.h>
#include "vnmrsys.h"
#include "data.h"
#include "process.h"
#include "group.h"
#include "struct3d.h"
#include "tools.h"
#include "lock3D.h"
#include "variables.h"
#include "allocate.h"
#include "params.h"
#include "pvars.h"
#include "wjunk.h"

#ifdef VMS
#include file.h
#define  F_OK	0
#define  R_OK	4
#define  mkdir  vf_mkdir
#else 
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#endif 

#define ERROR		-1
#define COMPLETE	0
#define RTSVPAR		1
#define FALSE		0
#define TRUE		1
#define MAXSTR		256
#define MINDEGREE	0.005	/* same value as in `ft2d.c` */

#define ABSVAL		0
#define STATESVAL	1
#define TPPIVAL		3

#define Getpar(tree, name, val)					\
	if ( P_getreal(tree, name, val, 1) )			\
	{							\
	   Werrprintf("cannot get parameter %s", name);		\
	   releaseAllWithId("proc3d");				\
	   return(NULL);					\
	}

#define Getpar0(tree, name, val)					\
	if ( P_getreal(tree, name, val, 1) )			\
	{							\
	   Werrprintf("cannot get parameter %s", name);		\
	   releaseAllWithId("proc3d");				\
	   return(0);					\
	}

#define Setpar(tree, name, val)					\
	if ( P_setreal(tree, name, val, 0) )			\
	{							\
	   Werrprintf("cannot set parameter %s", name);		\
	   releaseAllWithId("proc3d");				\
	   return(NULL);					\
	}

#define Setpar0(tree, name, val)					\
	if ( P_setreal(tree, name, val, 0) )			\
	{							\
	   Werrprintf("cannot set parameter %s", name);		\
	   releaseAllWithId("proc3d");				\
	   return(0);					\
	}

#define Setsvpar(name, val)					\
	if ( set_svpar(name, val) )				\
	{							\
	   releaseAllWithId("proc3d");				\
	   return(NULL);					\
	}

#define rindex(a,b) strrchr((a),(b))

struct _curDataInfo
{
   int	status[MAX_DATA_BUFFERS + 1];
   int	active;
};

typedef struct _curDataInfo curDataInfo;
typedef struct wtparams	wtparms;

static curDataInfo	curdatainfo;
extern double getfpmult(int fdimname, int ddr);
extern int removephasefile();
extern int removelock(char *filepath);
extern void ls_ph_fid(char *lsfname, int *lsfval, char *phfname, double *phfval,
                      char *lsfrqname, double *lsfrqval);
extern void     Wturnoff_buttons();
extern int datapoint(double freq, double sw, int fn);
extern int init_wt1(struct wtparams *wtpar, int fdimname);
extern int init_wt2(struct wtparams *wtpar, float  *wtfunc,
             int n, int rftflag, int fdimname, double fpmult, int rdwtflag);
extern int setlppar(lpstruct *parLPinfo, int dimen, int pstatus,
             int nctdpts, int ncfdpts, int mode, char *memId);
extern int set_mode(char *m_name);
extern void phasefunc(float *phasepntr, int npnts, double lpval, double rpval);
extern int findphases(double *rpval, double *lpval, int fdimname);
extern int fidss_par(ssparInfo *sspar, int ncp0, int memalloc, int dimen);
extern int createlock(char *filepath, int type);
extern int pipeRead(int argc, char *argv[], int retc, char *retv[]);

static char *fname_cat(char *base, char *increment );
static int set_svpar(char *parname, double parval);
void resetf3();

/*---------------------------------------
|					|
|	       setproc()/1		|
|					|
+--------------------------------------*/
static int setproc(int dimname)
{
   char proctype[MAXSTR],
	procname[MAXSTR];
   int	procstatus;


   switch (dimname)
   {
      case S_NP:   strcpy(procname, "proc");  break;
      case S_NI:   strcpy(procname, "proc1"); break;
      case S_NI2:  strcpy(procname, "proc2"); break;
      default:	   Werrprintf("internal error");
		   dimname = S_NP;
		   break;
   }

   if (P_getstring(CURRENT, procname, proctype, 1, 10))
      strcpy(proctype, "ft");


   if ( strcmp(proctype, "ft") == 0 )
   {
      procstatus = (CMPLX | FT_PROC);
   }
   else if ( strcmp(proctype, "rft") == 0 )
   { 
      procstatus = FT_PROC;
   } 
   else if ( strcmp(proctype, "lp") == 0 ) 
   {  
      procstatus = (CMPLX | LP_PROC);
   }  
   else if ( strcmp(proctype, "rlp") == 0 )
   {
      procstatus = LP_PROC;
   }
   else
   {
      procstatus = (CMPLX | FT_PROC);
   }

   return(procstatus);
}


/*---------------------------------------
|					|
|	   open_3Dinfofile()/1		|
|					|
+--------------------------------------*/
static int open_3Dinfofile(char *info3Ddir)
{
   char		info3Dpath[MAXPATHL];
   int		fd;


   if ( strcmp(info3Ddir, "") == 0 )
   {
      strcpy(info3Ddir, curexpdir);
      fname_cat(info3Ddir, "info");
#ifdef VMS
      make_vmstree( info3Ddir, info3Ddir, MAXPATHL );
#endif 
   }

   strcpy(info3Dpath, info3Ddir);
   fname_cat(info3Dpath, "procdat");

   if ( mkdir(info3Ddir, 0777) )
   {
      if (errno != EEXIST)
      {
         Werrprintf("cannot create directory %s for 3D information file",
		info3Ddir);
         return(ERROR);
      }
   }

   if ( createlock(info3Dpath, INFO_LFILE) )
   {
      Werrprintf("cannot create 3D information lock file");
      Werrprintf("file path: \"%s\"\n", info3Dpath);
      return(ERROR);
   }

   if ( (fd = open(info3Dpath, (O_WRONLY|O_CREAT|O_TRUNC),
		0666)) < 0 )
   {
      Werrprintf("cannot open 3D INFO file `%s`\n", info3Dpath);
      return(ERROR);
   }

   return(fd);
}


/*---------------------------------------
|                                       |
|	       clearf3()/0		|
|					|
+--------------------------------------*/
static void clearf3()
{
   P_deleteVar(PROCESSED,"swsv");
   P_deleteVar(CURRENT,"swsv");
   P_deleteVar(PROCESSED,"fnsv");
   P_deleteVar(CURRENT,"fnsv");
   P_deleteVar(PROCESSED,"rflsv");
   P_deleteVar(CURRENT,"rflsv");
   P_deleteVar(PROCESSED,"rfpsv");
   P_deleteVar(CURRENT,"rfpsv");
   P_deleteVar(PROCESSED,"spsv");
   P_deleteVar(CURRENT,"spsv");
   P_deleteVar(PROCESSED,"wpsv");
   P_deleteVar(CURRENT,"wpsv");
}

/*---------------------------------------
|                                       |
|	       resetf3()/0		|
|					|
+--------------------------------------*/
void resetf3()
{
   double	tmp;


   if ( !P_getreal(PROCESSED, "swsv", &tmp, 1) )
   {
      P_setreal(CURRENT, "sw", tmp, 0);
      P_setreal(PROCESSED, "sw", tmp, 0);
   }

   if ( !P_getreal(PROCESSED, "fnsv", &tmp, 1) )
      P_setreal(CURRENT, "fn", tmp, 0);

   if ( !P_getreal(PROCESSED, "rflsv", &tmp, 1) )
      P_setreal(CURRENT, "rfl", tmp, 0);

   if ( !P_getreal(PROCESSED, "rfpsv", &tmp, 1) )
      P_setreal(CURRENT, "rfp", tmp, 0);

   if ( !P_getreal(PROCESSED, "spsv", &tmp, 1) )
      P_setreal(CURRENT, "sp", tmp, 0);

   if ( !P_getreal(PROCESSED, "wpsv", &tmp, 1) )
      P_setreal(CURRENT, "wp", tmp, 0);
}


/*---------------------------------------
|                                       |
|	      init3Dinfo()/0		|
|					|
+--------------------------------------*/
static proc3DInfo *init3Dinfo()
{
   char			npname[16],
			fnname[10],
			dmgname[10],
			swname[10],
			spname[10],
			wpname[10],
			rflname[10],
			rfpname[10],
			lsfrqname[10],
			phfidname[10],
			lsfidname[10],
			lpname[10],
			rpname[10],
			ntype[MAXDIM+1],
			fiddc[MAXDIM+1],
			specdc[MAXDIM+1],
			partspec[MAXDIM+1],
			tmppartspec[MAXDIM+1],
			pmode[10];
   char                 filepath[MAXPATH];
   int			i,
			j,
			dimval,
			dimension,
			npts,
   			nf,
                        res,
			diffpts;
   double		tmp,
			tmp2,
			lp,
			rp,
			sp,
			wp,
			sw,
			rfl,
			rfp,
			fpntmult;
   dimenInfo		*dinfo;
   wtparms		wtp;
   vInfo		info;
   proc3DInfo		*info3D;	
   dfilehead    	fidhead;


/*******************************
*  Allocate memory for the 3D  *
*  information structure.      *
*******************************/

   if ( (info3D = (proc3DInfo *) allocateWithId( (unsigned)
		sizeof(proc3DInfo), "proc3d" )) == NULL )
   {
      Werrprintf("cannot allocate space for 3D INFO structure");
      return(NULL);
   }

   D_trash(D_USERFILE);          /* instead of closing, trash it */

   if ( (res = D_getfilepath(D_USERFILE, filepath, curexpdir)) )
   {
      D_error(res);
      return(NULL);
   }

   if ( (res = D_open(D_USERFILE, filepath, &fidhead)) )
   {
      D_error(res);
      return(NULL);
   }
 

/***************************************
*  Load the 3D information structure.  *
***************************************/

   if ( P_getstring(CURRENT, "ntype3d", fiddc, 1, MAXDIM) )
   {
      strcpy(ntype, "nnn");
   }
   else
   { /* fiddc is a temporary variable */
      strcpy(ntype, "n");
      strcat(ntype, fiddc);
   }

   if ( P_getstring(CURRENT, "dcrmv", fiddc, 1, MAXDIM) )
      info3D->f3dim.scdata.dcflag = 0;
   else
      info3D->f3dim.scdata.dcflag = (fiddc[0] == 'y');

   if ( P_getstring(CURRENT, "fiddc3d", fiddc, 1, MAXDIM+1) )
      strcpy(fiddc, "nnn");

   if ( P_getstring(CURRENT, "specdc3d", specdc, 1, MAXDIM+1) )
      strcpy(specdc, "nnn");

   if ( P_getstring(CURRENT, "ptspec3d", partspec, 1, MAXDIM+1) )
      strcpy(partspec, "nnn");

   if ( !P_getstring(PROCESSED, "ptspec3d", tmppartspec, 1, MAXDIM+1) )
   {
      if (tmppartspec[0] == 'y')
         resetf3();
   }


   for (i = 0; i < MAXDIM; i++)
   {
      switch (i)
      {
         case 0:   strcpy(npname, "dimension3");
		   strcpy(fnname, "fn");
		   strcpy(dmgname, "dmg");
		   strcpy(swname, "sw");
		   strcpy(spname, "sp");
		   strcpy(wpname, "wp");
		   strcpy(rflname, "rfl");
		   strcpy(rfpname, "rfp");
		   strcpy(lpname, "lp");
		   strcpy(rpname, "rp");
		   strcpy(lsfidname, "lsfid");
		   strcpy(phfidname, "phfid");
		   strcpy(lsfrqname, "lsfrq");
		   dinfo = &(info3D->f3dim);
		   dimval = S_NP;
		   dimension = 3;
		   break;
         case 1:   strcpy(npname, "dimension1");
		   strcpy(fnname, "fn1");
		   strcpy(dmgname, "dmg1");
		   strcpy(swname, "sw1");
		   strcpy(spname, "sp1");
		   strcpy(wpname, "wp1");
		   strcpy(rflname, "rfl1");
		   strcpy(rfpname, "rfp1");
		   strcpy(lpname, "lp1");
		   strcpy(rpname, "rp1");
		   strcpy(lsfidname, "lsfid1");
		   strcpy(phfidname, "phfid1");
		   strcpy(lsfrqname, "lsfrq1");
		   dinfo = &(info3D->f1dim);
		   dimval = S_NI;
		   dimension = 1;
		   break;
         case 2:   strcpy(npname, "dimension2");
		   strcpy(fnname, "fn2");
		   strcpy(dmgname, "dmg2");
		   strcpy(swname, "sw2");
		   strcpy(spname, "sp2");
		   strcpy(wpname, "wp2");
		   strcpy(rflname, "rfl2");
		   strcpy(rfpname, "rfp2");
		   strcpy(lpname, "lp2");
		   strcpy(rpname, "rp2");
		   strcpy(lsfidname, "lsfid2");
		   strcpy(phfidname, "phfid2");
		   strcpy(lsfrqname, "lsfrq2");
		   dinfo = &(info3D->f2dim);
		   dimval = S_NI2;
		   dimension = 2;
		   break;
         default:  Werrprintf("internal error");
		   releaseAllWithId("proc3d");
		   return(NULL);
      }


/************************************************
*  Load the 3D information structure with the   *
*  appropriate values for `np`, `fn`, `lsfid`,  *
*  `phfid`, and `proc` values as a function of  *
*  the selected dimension.                      *
************************************************/

      dinfo->scdata.ntype = ( (ntype[i] == 'y') || (ntype[i] == 'Y') );
      dinfo->scdata.fiddc = ( (fiddc[i] == 'y') || (fiddc[i] == 'Y') );
      dinfo->scdata.specdc = ( (specdc[i] == 'y') ||
					(specdc[i] == 'Y') );

      Getpar(PROCESSED, npname, &tmp);
      dinfo->scdata.np = (int) (tmp + 0.5);

      if (dinfo->scdata.np < 1)
      {
         Werrprintf("number of complex time-domain points < 1 for t%1d",
			dimension);
         releaseAllWithId("proc3d");
         return(NULL);
      }
      else if ( dimval & (S_NI | S_NI2) )
      {
         if (dinfo->scdata.np < 2)
         {
            Werrprintf("not a 3D data set");
            releaseAllWithId("proc3d");
            return(NULL);
         }
      }

      if (dimval == S_NP)
      { /* for F3 */
         if ( fidss_par( &(dinfo->scdata.sspar), dinfo->scdata.np,
				FALSE, S_NP ) )
         {
            releaseAllWithId("proc3d");
            return(NULL);
         }
      }
      else
      { /* for dimensions F1 and F2 */
         dinfo->scdata.sspar.zfsflag = FALSE;
         dinfo->scdata.sspar.lfsflag = FALSE;
         dinfo->scdata.sspar.membytes = 0;
         dinfo->scdata.sspar.decfactor = 1;
         dinfo->scdata.sspar.ntaps = 1;
         dinfo->scdata.sspar.matsize = 0;
         dinfo->scdata.sspar.buffer = NULL;
      }

      if ( P_getVarInfo(CURRENT, fnname, &info) )
      {
         Werrprintf("cannot get `%s` parameter status", fnname);
         releaseAllWithId("proc3d");
         return(NULL);
      }

      if (!info.active)
      {
         int	k;

         k = 2;
         while (k < dinfo->scdata.np)
            k *= 2;

         dinfo->scdata.fn = 2*k;
      }
      else
      {
         if ( P_getreal(CURRENT, fnname, &tmp, 1) )
         {
            Werrprintf("cannot get value of `%s` parameter", fnname);
            releaseAllWithId("proc3d");
            return(NULL);
         }
         else
         {
            dinfo->scdata.fn = (int) (tmp + 0.5);
         }
      }

      Setpar(TEMPORARY, fnname, (double) (dinfo->scdata.fn));
      dinfo->scdata.proc = setproc(dimval);
      ls_ph_fid(lsfidname, &(dinfo->scdata.lsfid), phfidname, &tmp,
			lsfrqname, &tmp2);

      dinfo->scdata.lsfid /= 2;	   /* lsfid is now in complex points */
      dinfo->scdata.phfid = (float)tmp;
      dinfo->scdata.lsfrq = (float)tmp2;
      if (dinfo->scdata.ntype)
         dinfo->scdata.lsfrq *= -1.0;

      dinfo->scdata.pwr = 0;
      dinfo->scdata.zflvl = 0;
      dinfo->scdata.zfnum = 0;

      if ( i && (dinfo->scdata.lsfid > 0) )
      {
         dinfo->scdata.npadj = ( (dinfo->scdata.fn < 2*dinfo->scdata.np)
			? dinfo->scdata.fn/2 : dinfo->scdata.np );
      }
      else
      {
         dinfo->scdata.npadj = ( (dinfo->scdata.fn < 2*(dinfo->scdata.np
			- dinfo->scdata.lsfid)) ? (dinfo->scdata.fn/2 +
			dinfo->scdata.lsfid) : dinfo->scdata.np );
      }

      if ( ((dinfo->scdata.lsfid < 0) && (dinfo->scdata.npadj < 1)) ||
	   (dinfo->scdata.lsfid >= dinfo->scdata.npadj) )
      {
         Werrprintf("%s is too large for dimension %d", lsfidname, dimension);
         releaseAllWithId("proc3d");
         return(NULL);
      }

      dinfo->scdata.npadj -= dinfo->scdata.lsfid;

/*********************************************
*  Initialize weighting and phasing vectors  *
*  for the selected dimension.		     *
*********************************************/

      if ( (dinfo->ptdata.wtv = (float *) allocateWithId( (unsigned)
		   ( 3 * (dinfo->scdata.fn/2) * sizeof(float) ),
		   "proc3d" )) == NULL )
      {
         Werrprintf("cannot allocate space for weighting vectors");
         return(NULL);
      }

      dinfo->ptdata.phs = dinfo->ptdata.wtv + (dinfo->scdata.fn/2);
      wtp.wtflag = TRUE;	/* must be initialized */

      if ( init_wt1(&wtp, dimval) )
      {
         Werrprintf("cannot initialize weighting structure");
         releaseAllWithId("proc3d");
         return(NULL);
      }

      fpntmult = getfpmult(dimval, fidhead.status & S_DDR );

      if ( init_wt2(&wtp, dinfo->ptdata.wtv, dinfo->scdata.fn/2,
		     FALSE, dimval, fpntmult, FALSE) )
      {
         Werrprintf("cannot calculate weighting vector");
         releaseAllWithId("proc3d");
         return(NULL);
      }

      if ( findphases(&rp, &lp, dimval) )
      {
         releaseAllWithId("proc3d");
         return(NULL);
      }

      phasefunc(dinfo->ptdata.phs, dinfo->scdata.fn/2, lp, rp);
      dinfo->scdata.rp = (float)rp;
      dinfo->scdata.lp = (float)lp;

      dinfo->scdata.wtflag = wtp.wtflag;
      if ( (dinfo->scdata.dsply = set_mode(dmgname)) & PHMODE )
      {
         if ( (fabs(rp) <= MINDEGREE) && (fabs(lp) <= MINDEGREE) )
            dinfo->scdata.dsply = 0;
      }

/********************************************
*  If LP processing is to be done,  set up  *
*  the pointer to the LPinfo structure.     *
********************************************/

      if (dinfo->scdata.proc & LP_PROC)
      {
         int	maxlpnp;

         if ( setlppar(&(dinfo->parLPdata), dimval, dinfo->scdata.proc,
			  dinfo->scdata.npadj, dinfo->scdata.fn/2,
			  NO_LPALLOC, "proc3d") )
         {
            releaseAllWithId("proc3d");
            return(NULL);
         }

         maxlpnp = dinfo->scdata.npadj;

         for (j = 0; j < dinfo->parLPdata.sizeLP; j++)
         {
            int		nptmp;
            lpinfo	*lpdata;

            lpdata = dinfo->parLPdata.parLP + j;
            lpdata->lpmat_setup = NULL;
            lpdata->lpcoef_solve = NULL;

            if (lpdata->status & FORWARD)
            {
               nptmp = lpdata->startextpt + lpdata->ncextpt - 1;
               if (nptmp > maxlpnp)
                  maxlpnp = nptmp;
            }
         }

         dinfo->scdata.npadj = maxlpnp;
      }
      else
      {
         dinfo->parLPdata.lppntr = NULL;
         dinfo->parLPdata.parLP = NULL;
         dinfo->parLPdata.sizeLP = 0;
      }

/******************************************
*  Load in the structure elements for F3  *
*  region selection.                      *
******************************************/

      if ( (partspec[i] == 'y') && (i == 0) )
      { /* F3 region selection is active */
	 int	stptzero,
		stpt,
		endpt,
		endptzero;


         Getpar(CURRENT, rflname, &rfl);
         Getpar(CURRENT, rfpname, &rfp);
         Getpar(CURRENT, spname, &sp);
         Getpar(CURRENT, wpname, &wp);
         Getpar(CURRENT, swname, &sw);

         npts = (int) ( (dinfo->scdata.fn*wp/sw) + 0.5 );
         j = 2;
         while (j < npts)
            j *= 2;
         npts = j;

         endpt = datapoint(sp+rfl-rfp, sw, dinfo->scdata.fn);
         stpt = datapoint(sp+wp+rfl-rfp, sw, dinfo->scdata.fn);
         diffpts = endpt - stpt;

         if (diffpts != npts)
         {
            stptzero = stpt - (npts - diffpts)/2;
            endptzero = endpt + (npts - diffpts)/2;

            if (stptzero < 0)
            {
               endptzero -= stptzero;
               stptzero = 0;
               if (endptzero > dinfo->scdata.fn)
               {
                  Werrprintf("error in setting F3 region:  1");
                  releaseAllWithId("proc3d");
                  return(NULL);
               }
            }
            else if (endptzero > dinfo->scdata.fn)
            {
               stptzero -= (dinfo->scdata.fn - endptzero);
               endptzero = dinfo->scdata.fn;
               if (stptzero < 0)
               {
                  Werrprintf("error in setting F3 region:  2");
                  releaseAllWithId("proc3d");
                  return(NULL);
               }
            }

            if ( (stpt < stptzero) || (endpt > endptzero) )
	    {
               Werrprintf("error in setting F3 region:  3");
               releaseAllWithId("proc3d");
               return(NULL);
            }
         }
         else
         {
            stptzero = stpt;
            endptzero = endpt;
         }

         Setsvpar(rflname, rfl);
         Setsvpar(rfpname, rfp);
         Setsvpar(spname, sp);
         Setsvpar(wpname, wp);
         Setsvpar(swname, sw);
         Setsvpar(fnname, (double)(dinfo->scdata.fn));

         rfp = sp - ( sw/(double)(dinfo->scdata.fn-1) ) * (endptzero - endpt);
         sp = rfp;
         rfl = 0;
         wp = (npts * sw) / (double)(dinfo->scdata.fn);
         sw = wp;

         Setpar(TEMPORARY, rflname, rfl);
         Setpar(TEMPORARY, rfpname, rfp);
         Setpar(TEMPORARY, swname, sw);
         Setpar(TEMPORARY, spname, sp);
         Setpar(TEMPORARY, wpname, wp);
         Setpar(TEMPORARY, fnname, (double)npts);

         dinfo->scdata.reginfo.stptzero  = stptzero;
         dinfo->scdata.reginfo.stpt      = stpt;
         dinfo->scdata.reginfo.endpt     = endpt;
         dinfo->scdata.reginfo.endptzero = endptzero;
      }
      else
      { /* no F3 region selection; this is the only choice for F2 and F1! */
         dinfo->scdata.reginfo.stptzero  = 0;
         dinfo->scdata.reginfo.stpt      = 0;
         dinfo->scdata.reginfo.endpt     = dinfo->scdata.fn;
         dinfo->scdata.reginfo.endptzero = dinfo->scdata.fn;
      }
   }


/*********************************
*  Initialize the `version` and  *
*  `arraydim` parameters.        *
*********************************/

   info3D->vers = FT3D_VERSION;

   if ( P_getreal(PROCESSED, "nf", &tmp, 1) || tmp < 1.0)
   {
       nf = 1;
   }else{
       nf = (int)(tmp + 0.5);
   }
   Getpar(PROCESSED, "arraydim", &tmp);
   dimval = (int)(tmp + 0.5) * nf;
   dimval /= (info3D->f2dim.scdata.np * info3D->f1dim.scdata.np);
   info3D->arraydim = dimval;


/******************************
*  Initialize the `datatype'  *
*  parameter.                 *
******************************/

   if ( P_getstring(CURRENT, "pmode", pmode, 1, 10) )
      strcpy(pmode, "");

   if ( strcmp(pmode, "full") == 0 )
   {
      info3D->datatype = HYPERCOMPLEX;
   }
   else if ( strcmp(pmode, "partial") == 0 )
   {
      info3D->datatype = COMPLEX;
   }
   else
   { /* default value */
      info3D->datatype = REAL;
   }


/****************************
*  Return the completed 3D  *
*  information structure.   *
****************************/

   return(info3D);
}


/*---------------------------------------
|					|
|	    set_svpar()/2		|
|					|
+--------------------------------------*/
static int set_svpar(char *parname, double parval)
{
   char	tmpname[MAXPATHL];


   strcpy(tmpname, parname);
   strcat(tmpname, "sv");

   P_creatvar(TEMPORARY, tmpname, G_PROCESSING);
   if ( P_setreal(TEMPORARY, tmpname, parval, 0) )
   {
      Werrprintf("Cannot set temporary parameter `%s`", tmpname);
      return(ERROR);
   }

   P_creatvar(CURRENT, tmpname, G_PROCESSING);
   if ( P_setreal(CURRENT, tmpname, parval, 0) )
   {
      Werrprintf("Cannot set current parameter `%s`", tmpname);
      return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     write3Dinfo()/3		|
|					|
+--------------------------------------*/
static int write3Dinfo(int fd, proc3DInfo *info3D, char *info3Ddir)
{
   char		lockpath[MAXPATHL];
   int		i,
		j,
		nbytes;
   dimenInfo	dinfo;


   if ( write(fd, &(info3D->vers), sizeof(int)) != sizeof(int) )
   {
      Werrprintf("cannot write out 3D version number");
      return(ERROR);
   }

   if ( write(fd, &(info3D->arraydim), sizeof(int)) != sizeof(int) ) 
   {
      Werrprintf("cannot write out 3D arraydim value"); 
      return(ERROR); 
   }

   if ( write(fd, &(info3D->datatype), sizeof(int)) != sizeof(int) ) 
   {
      Werrprintf("cannot write out 3D datatype"); 
      return(ERROR); 
   }


   for (i = 0; i < MAXDIM; i++)
   {
      switch (i)
      {
         case 0:   dinfo = info3D->f3dim; break;
         case 1:   dinfo = info3D->f1dim; break;
         case 2:   dinfo = info3D->f2dim; break;
         default:  Werrprintf("internal error");
		   releaseAllWithId("proc3d");
		   return(ERROR);
      }

      if ( write(fd, &(dinfo.scdata), sizeof(sclrpar3D)) !=
		   sizeof(sclrpar3D) )
      {
         Werrprintf("cannot write out 3D scalar information");
         return(ERROR);
      }


      if ( write(fd, &(dinfo.parLPdata.sizeLP), sizeof(int)) !=
			sizeof(int) )
      {
         Werrprintf("cannot write out 3D LP information");
         return(ERROR);
      }

      if (dinfo.parLPdata.sizeLP)
      {
         if ( write(fd, &(dinfo.parLPdata.membytes), sizeof(int)) !=
			sizeof(int) )
         {
            Werrprintf("cannot write out 3D LP information");
            return(ERROR);
         }

         nbytes = sizeof(lpinfo) - LP_LABEL_SIZE;
					/* do not write out 'label' */
         for (j = 0; j < dinfo.parLPdata.sizeLP; j++)
         {
            lpinfo	*lpdata;

            lpdata = dinfo.parLPdata.parLP + j;
            if ( write(fd, lpdata, nbytes) != nbytes )
            {
               Werrprintf("cannot write out 3D LP information");
               return(ERROR);
            }
         }
      }

      nbytes = (dinfo.scdata.fn / 2) * sizeof(float);
      if ( write(fd, dinfo.ptdata.wtv, nbytes) != nbytes )
      {
         Werrprintf("cannot write out 3D weighting information");
         return(ERROR);
      }

      nbytes = dinfo.scdata.fn * sizeof(float); 
      if ( write(fd, dinfo.ptdata.phs, nbytes) != nbytes )
      { 
         Werrprintf("cannot write out 3D phasing information");
         return(ERROR); 
      }
   }

   close(fd);
   strcpy(lockpath, info3Ddir);
   fname_cat(lockpath, "procdat");
   if ( removelock(lockpath) )
   {
      return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	    init3Dprocpar()/0		|
|					|
+--------------------------------------*/
static void init3Dprocpar()
{
   P_treereset(TEMPORARY);	/* clear the `tree` first */
   clearf3();

   P_copygroup(PROCESSED, TEMPORARY, G_ACQUISITION);
   P_copygroup(PROCESSED, TEMPORARY, G_SAMPLE);
   P_copygroup(PROCESSED, TEMPORARY, G_SPIN);

   P_copygroup(CURRENT, TEMPORARY, G_PROCESSING);
   P_copygroup(CURRENT, TEMPORARY, G_DISPLAY);
}


/*---------------------------------------
|					|
|	    save3Dprocpar()/1		|
|					|
+--------------------------------------*/
static int save3Dprocpar(char *info3Ddir)
{
   char	path[MAXPATHL];


   strcpy(path, info3Ddir);
   fname_cat(path, "procpar3d");
   P_save(TEMPORARY, path);

   if ( access(path, R_OK) )
   {
      Werrprintf("creation of 3D parameter file failed");
      return(ERROR);
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     removeinfo3D()/1		|
|					|
+--------------------------------------*/
void removeinfo3D(char *info3Ddir)
{
   int ret __attribute__((unused));
   char	syscmd[MAXPATHL];

   sprintf(syscmd, "rm -rf %s\n", info3Ddir);
   ret = system(syscmd);
}


/*---------------------------------------
|					|
|	     writecoeffile()/1		|
|					|
+--------------------------------------*/
int writecoeffile(char *coefpath)
{
   char		arraystr[MAXSTR],
		arrayword[MAXSTR],
		namef1[MAXSTR],
		namef2[MAXSTR],
		*aryparse,
		*arychar;
   int		n,
		finishword = FALSE,
		f1first = FALSE,
		f2first = FALSE,
		f1acqtype,
		f2acqtype;
   double	tmp;
   vInfo	info;
   FILE		*fdcoef;


   if ( strcmp(coefpath, "") == 0 )
   {
      strcpy(coefpath, curexpdir);
      fname_cat(coefpath, "coef");
   }

   if ( P_getstring(PROCESSED, "array", arraystr, 1, MAXSTR-1) )
   {
      Werrprintf("cannot get `array` parameter for 3D coefficients");
      return(ERROR);
   }

   if ( P_getstring(CURRENT, "f1shpar", namef1, 1, MAXSTR-1) )
   {
      strcpy(namef1, "phase");
   }
   else
   {
      if ( strcmp(namef1, "") == 0 )
         strcpy(namef1, "phase");
   }

   if ( P_getstring(CURRENT, "f2shpar", namef2, 1, MAXSTR-1) )
   {
      strcpy(namef2, "phase2");
   }
   else
   {
      if ( strcmp(namef2, "") == 0 )
         strcpy(namef2, "phase2");
   }

   arychar = arrayword;
   aryparse = arraystr;
   n = strlen(arraystr) + 1;	/* must include the final '\0' */

/***********************************************
*  Determine if 'namef1' and 'namef2' exist,   *
*  if they are arrayed, and if they are not    *
*  arrayed, what their respective values are.  *
***********************************************/

   if ( P_getreal(PROCESSED, namef1, &tmp, 1) )
   {
      f1acqtype = ABSVAL;
   }
   else
   {
      if ( P_getVarInfo(PROCESSED, namef1, &info) )
      {
         Werrprintf("Cannot get %s parameter information", namef1);
         return(ERROR);
      }

      f1acqtype = ( (info.size > 1) ? STATESVAL : (int)(tmp + 0.5) );
   }

   if ( P_getreal(PROCESSED, namef2, &tmp, 1) )
   {
      f2acqtype = ABSVAL;
   }
   else
   {   
      if ( P_getVarInfo(PROCESSED, namef2, &info) )
      {
         Werrprintf("Cannot get %s parameter information", namef2);
         return(ERROR);
      }
 
      f2acqtype = ( (info.size > 1) ? STATESVAL : (int)(tmp + 0.5) ); 
   }

/***********************************
*  Parse the VNMR string `array`.  *
***********************************/

   if (n > 1)
   {
      while (n > 0)
      {
         switch (*aryparse)
         {
            case '(':
            case ')':	return(ERROR);
            case '\0':
            case ',':	*arychar = '\0';
			finishword = TRUE;
			break;
            default:	*arychar++ = *aryparse;
         }

         if (finishword)
         {
            if ( strcmp(arrayword, namef1) == 0 )
            {
               if (!f2first)
                  f1first = TRUE;
            }
            else if ( strcmp(arrayword, namef2) == 0 ) 
            {
               if (!f1first)
                  f2first = TRUE;
            }
            else
            {
               Werrprintf("Only `%s` and `%s' may be arrayed", namef1,
				namef2);
               return(ERROR);
            }

            if ( P_getVarInfo(PROCESSED, arrayword, &info) )
            {
               Werrprintf("Cannot get %s parameter information", arrayword);
               return(ERROR);
            }

            if (info.size != 2)
            { /* hard-coded condition */
               Werrprintf("%s has more than 2 elements", arrayword);
               return(ERROR);
            }

            arychar = arrayword;
            finishword = FALSE;
         }

         n -= 1;
         aryparse += 1;
      }
   }

/******************************
*  Open 3D coefficient file.  *
******************************/

   if ( (fdcoef = fopen(coefpath, "w")) == NULL )
   {
      Werrprintf("Cannot open 3D coefficient file %s\n", coefpath);
      return(ERROR);
   }

   if ( (f1acqtype == STATESVAL) && (f2acqtype == STATESVAL) )
   { /* SH(t1)-SH(t2) */
      if (f1first)
      {
         fprintf(fdcoef, "1 0 0 0 0 0 0 0\n0 0 0 0 1 0 0 0\n");
         fprintf(fdcoef, "0 0 1 0 0 0 0 0\n0 0 0 0 0 0 1 0\n");
      }
      else
      {
         fprintf(fdcoef, "1 0 0 0 0 0 0 0\n0 0 1 0 0 0 0 0\n");
         fprintf(fdcoef, "0 0 0 0 1 0 0 0\n0 0 0 0 0 0 1 0\n");
      }
   }
   else if (f1acqtype == STATESVAL)
   {
      if (f2acqtype == TPPIVAL)
      { /* SH(t1)-TPPI(t2) */
         fprintf(fdcoef, "1 0 0 0\n0 0 1 0\n0 0 0 0\n0 0 0 0\n");
      }
      else
      { /* SH(t1)-ABS(t2) */
         fprintf(fdcoef, "1 0 0 0\n0 0 1 0\n0 -1 0 0\n0 0 0 -1\n");
      }
   }
   else if (f2acqtype == STATESVAL)
   { /* TPPI(t1)-SH(t2) and ABS(t1)-SH(t2) */
      fprintf(fdcoef, "1 0 0 0\n0 1 0 0\n0 0 1 0\n0 0 0 1\n");
   }
   else if (f1acqtype == TPPIVAL)
   {
      if (f2acqtype == TPPIVAL)
      { /* TPPI(t1)-TPPI(t2) */
         fprintf(fdcoef, "1 0\n0 0\n0 0\n0 0\n");
      }
      else
      { /* TPPI(t1)-ABS(t2) */
         fprintf(fdcoef, "1 0\n0 0\n0 -1\n0 0\n");
      }
   }
   else
   {
      if (f2acqtype == TPPIVAL)
      { /* ABS(t1)-TPPI(t2) */  
         fprintf(fdcoef, "1 0\n0 -1\n0 0\n0 0\n");
      }
      else 
      { /* ABS(t1)-ABS(t2) */ 
         fprintf(fdcoef, "1 0\n0 0\n0 -1\n0 0\n");
      }
   }

   if (f1acqtype == ABSVAL)
   { /* to create t1 interferogram */
      fprintf(fdcoef, "1 0 0 0 0 -1 0 0\n");
   }
   else if (f1acqtype == STATESVAL)
   {
      fprintf(fdcoef, "1 0 0 0 0 0 1 0\n");
   }
   else
   {
      fprintf(fdcoef, "1 0 0 0 0 0 0 0\n");
   }

   fclose(fdcoef);

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	      set3dproc()/2		|
|					|
+--------------------------------------*/
int set3dproc(int argc, char *argv[], int retc, char *retv[])
{
   char		info3Ddir[MAXPATHL],
		coefpath[MAXPATHL];
   int		infofd,
		arg_no = 1,
		coeffile = TRUE;	/* initial condition */
   proc3DInfo	*info3D;


   strcpy(coefpath, "");
   strcpy(info3Ddir, "");

   while ( (arg_no < argc) && (!isReal(argv[arg_no])) )
   {
      if ( strcmp(argv[arg_no], "nocoef") == 0 )
      {
         coeffile = FALSE;
      }
      else
      {
         char	*lastchar,
		tmpcoefpath[MAXPATHL];
         int	n;

         strcpy(info3Ddir, argv[arg_no]);
         strcpy(tmpcoefpath, info3Ddir);
         n = strlen(tmpcoefpath);
         lastchar = tmpcoefpath + n - 1;
#ifdef UNIX
         while ( (*lastchar-- != '/') && (n-- > 0) );
#else 
         while ( (*lastchar-- != ']') && (n-- > 0) );
#endif 
         *(lastchar += 2) = '\0';
         strcpy(coefpath, tmpcoefpath);
         strcat(coefpath, "coef");
      }

      arg_no += 1;
   }

   if ( (infofd = open_3Dinfofile(info3Ddir)) == ERROR )
   {
      removeinfo3D(info3Ddir);
      ABORT;
   }

   init3Dprocpar();

   if ( (info3D = init3Dinfo()) == NULL )
   {
      removeinfo3D(info3Ddir);
      ABORT;
   }

   if ( save3Dprocpar(info3Ddir) )
   {
      removeinfo3D(info3Ddir);
      ABORT;
   }

   if (info3D->arraydim == 0)
   {
       Werrprintf("set3dproc: arraydim is 0");
       ABORT;
   }

   if ( write3Dinfo(infofd, info3D, info3Ddir) )
   {
      removeinfo3D(info3Ddir);
      ABORT;
   }

   if (coeffile)
   {
      if ( writecoeffile(coefpath) )
         RETURN;
   }

   RETURN;
}


/*---------------------------------------
|					|
|	      settrace()/2		|
|					|
+--------------------------------------*/
void settrace(char *dim1, char *dim2)
{
   char	trace[5];


   if ( P_getstring(CURRENT, "trace", trace, 1, 4) )
      return;

   if ( (strcmp(trace, dim1) == 0) ||
	(strcmp(trace, dim2) == 0) )
   {
      return;
   }
   else
   {
      P_setstring(CURRENT, "trace", dim1, 1);
   }
}


/*---------------------------------------
|					|
|	    load_newparams()/1		|
|					|
+--------------------------------------*/
static int load_newparams(char *parfilepath)
{
   char		rpname[5],
		lpname[5],
		strval[4][MAXSTR],
		parname[4],
		partspec[MAXDIM+1],
		rflname[6],
		rfpname[6],
		spname[6],
		wpname[6];
   int		i;
   double	tmp,
		rlval[7];


/********************************
*  Initialize the `strval` and  *
*  `rlval` variables.           *
********************************/

   for (i = 0; i < 7; i++)
      rlval[i] = 0.0;

   for (i = 0; i < 4; i++)
      strcpy(&strval[i][0], "");

/*******************************************
*  Save the values of the `r#`, `n#`, and  *
*  `macro` processing variables.           *
*******************************************/

   for (i = 0; i < 7; i++)
   {
      sprintf(parname, "r%d", i+1);
      P_getreal(CURRENT, parname, &rlval[i], 1);
   }

   for (i = 0; i < 3; i++)
   {
      sprintf(parname, "n%d", i+1);
      P_getstring(CURRENT, parname, &strval[i][0], 1, MAXSTR-1);
   }

   P_getstring(CURRENT, "macro", &strval[3][0], 1, MAXSTR-1);
      

/*********************************************
*  Reset the `temporary' parameter tree and  *
*  read the 3D parameters into this tree.    *
*********************************************/

   if ( P_treereset(TEMPORARY) )
   {
      Werrprintf("cannot reset TEMPORARY tree");
      return(ERROR);
   }
   else if ( P_read(TEMPORARY, parfilepath) )
   {
      Werrprintf("cannot get 3D parameters from `procpar3d` file");
      return(ERROR);
   }

/**************************************************
*  Copy the acquisition and processing variables  *
*  from the `temporary` tree into both the 'cur-  *
*  rent` and `processed` trees.                   *
**************************************************/

   if ( P_copygroup(TEMPORARY, CURRENT, G_ACQUISITION) )
   {
      Werrprintf("cannot re-initialize current parameters");
      return(ERROR);
   }

   if ( P_copygroup(TEMPORARY, CURRENT, G_PROCESSING) )
   {
      Werrprintf("cannot re-initialize current parameters");
      return(ERROR);
   }

   if ( P_copygroup(TEMPORARY, PROCESSED, G_PROCESSING) ) 
   {
      Werrprintf("cannot re-initialize processed parameters");
      return(ERROR);
   }

   if ( P_copygroup(TEMPORARY, PROCESSED, G_ACQUISITION) )
   {
      Werrprintf("cannot re-initialize processed parameters");
      return(ERROR);
   }

   if ( P_getstring(TEMPORARY, "ptspec3d", partspec, 1, MAXDIM+1) )
      strcpy(partspec, "nnn");


   for (i = 0; i < MAXDIM; i++)
   {
      if (partspec[i] == 'y')
      {
         switch (i)
         {
            case 0:   strcpy(rflname, "rfl");
		      strcpy(rfpname, "rfp");
		      strcpy(wpname, "wp");
		      strcpy(spname, "sp");
		      break;
            case 1:   strcpy(rflname, "rfl1");
		      strcpy(rfpname, "rfp1");
		      strcpy(wpname, "wp1");
		      strcpy(spname, "sp1");
		      break;
            case 2:   strcpy(rflname, "rfl2");
		      strcpy(rfpname, "rfp2");
		      strcpy(wpname, "wp2");
		      strcpy(spname, "sp2");
		      break;
            default:  break;
         }

         Getpar0(TEMPORARY, rflname, &tmp);
         Setpar0(CURRENT, rflname, tmp);
         Setpar0(PROCESSED, rflname, tmp);

         Getpar0(TEMPORARY, rfpname, &tmp);
         Setpar0(CURRENT, rfpname, tmp);
         Setpar0(PROCESSED, rfpname, tmp);

         Getpar0(TEMPORARY, spname, &tmp);
         Setpar0(CURRENT, spname, tmp);
         Setpar0(PROCESSED, spname, tmp);

         Getpar0(TEMPORARY, wpname, &tmp);
         Setpar0(CURRENT, wpname, tmp);
         Setpar0(PROCESSED, wpname, tmp);
      }
   }


/************************************************
*  Extract the `rp` and `lp` phasing constants  *
*  for all 3 dimensions and load into both the  *
*  current and processed trees.                 *
************************************************/

   for (i = 0; i < MAXDIM; i++)
   {
      switch (i)
      {
         case 0:  strcpy(rpname, "rp");  strcpy(lpname, "lp");  break;
         case 1:  strcpy(rpname, "rp1"); strcpy(lpname, "lp1"); break;
         case 2:  strcpy(rpname, "rp2"); strcpy(lpname, "lp2"); break;
         default: break;
      }

      Getpar0(TEMPORARY, rpname, &tmp);
      Setpar0(CURRENT, rpname, tmp);
      Setpar0(PROCESSED, rpname, tmp);

      Getpar0(TEMPORARY, lpname, &tmp);
      Setpar0(CURRENT, lpname, tmp);
      Setpar0(PROCESSED, lpname, tmp);
   }

/**********************************************
*  Restore the values of the `r#`, `n#`, and  *
*  `macro` processing variables.              *
**********************************************/

   for (i = 0; i < 7; i++)
   {
      sprintf(parname, "r%d", i+1);
      P_setreal(CURRENT, parname, rlval[i], 1);
      P_setreal(PROCESSED, parname, rlval[i], 1);
   }

   for (i = 0; i < 3; i++)
   {
      sprintf(parname, "n%d", i+1);
      P_setstring(CURRENT, parname, &strval[i][0], 1);
      P_setstring(PROCESSED, parname, &strval[i][0], 1);
   }

   P_setstring(CURRENT, "macro", &strval[3][0], 1);
   P_setstring(PROCESSED, "macro", &strval[3][0], 1);

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	      selectpl()/2		|
|					|
+--------------------------------------*/
int selectpl(int argc, char *argv[], int retc, char *retv[])
{
   char		msge[MAXSTR],
		plane[8],
		fileext[16],
		olddatapath[MAXPATH],
		newdatapath[MAXPATH],
		procparpath[MAXPATH],
		vnmrcmd[MAXPATH],
                proccmd[STR64];
   char        *pipeDir = NULL;
   char        *projPipeDir = NULL;
   int		r,
		numplanes,
		planeno = -1,
		plane2no = -1,
                nD = 3,
		datano = 0,
		newpar = TRUE,
		selproj = FALSE,
                pipeData = 0,
		arg_no = 1;
   double	tmp;
   double	tmp2;
   dfilehead	datafhead;


/**********************************
*  Deactivate the buttons.  Just  *
*  deactivating the mouse is not  *
*  sufficient.                    *
**********************************/

   Wturnoff_buttons();

/*******************************************
*  Parse STRING arguments first.  Get the  *
*  operative plane number and plane type   *
*  first.                                  *
*******************************************/

   if (!P_getreal(PROCESSED, "planeno", &tmp, 1) )
   {
      planeno = (int) (tmp + 0.5);
   }
   if (!P_getreal(PROCESSED, "plane2no", &tmp, 1) )
   {
      plane2no = (int) (tmp + 0.5);
   }


   if ( P_getstring(CURRENT, "plane", plane, 1, 5) )
   {
      Werrprintf("cannot get parameter `plane`");
      return(ERROR);
   }
   if ( ! P_getstring(CURRENT, "proccmd", proccmd, 1, STR64-1) )
   {
      pipeData = ( (strcmp(proccmd,"pipe")) ? 0 : 1 );
   }

   while ( (argc > arg_no) && (!isReal(argv[arg_no])) )
   {
      if ( (strcmp(argv[arg_no], "f1f3") == 0) ||
           (strcmp(argv[arg_no], "f3f1") == 0) )
      {
         strcpy(plane, "f1f3");
      }
      else if ( (strcmp(argv[arg_no], "f2f3") == 0) ||
		(strcmp(argv[arg_no], "f3f2") == 0) )
      {
         strcpy(plane, "f2f3");
      }
      else if ( (strcmp(argv[arg_no], "f1f2") == 0) ||
		(strcmp(argv[arg_no], "f2f1") == 0) )
      {
         strcpy(plane, "f1f2");
      }
      else if ( (strcmp(argv[arg_no], "f1f4") == 0) ||
		(strcmp(argv[arg_no], "f4f1") == 0) )
      {
         strcpy(plane, "f1f4");
      }
      else if ( (strcmp(argv[arg_no], "f2f4") == 0) ||
		(strcmp(argv[arg_no], "f4f2") == 0) )
      {
         strcpy(plane, "f2f4");
      }
      else if ( (strcmp(argv[arg_no], "f3f4") == 0) ||
		(strcmp(argv[arg_no], "f4f3") == 0) )
      {
         strcpy(plane, "f3f4");
      }
      else if ( (argv[arg_no][0] == 'p') && (argv[arg_no][1] == 'l') )
      {
         int	n,
		kk;

         n = strlen(argv[arg_no]) - 1;
         for (kk = 0; kk < n; kk++)
            argv[arg_no][kk] = argv[arg_no][kk+2];

         planeno = atoi(argv[arg_no]);
      }
      else if ( strcmp(argv[arg_no], "oldpar") == 0 )
      {
         newpar = FALSE;
      }
      else if ( strcmp(argv[arg_no], "proj") == 0 )
      {
         selproj = TRUE;
         planeno = 0;
      }
      else if ( strcmp(argv[arg_no], "next") == 0 )
      {
         newpar = FALSE;
         if (planeno > 0)
            planeno += 1;
      }
      else if ( strcmp(argv[arg_no], "prev") == 0 )
      {
         newpar = FALSE; 
         if (planeno > 1)
            planeno -= 1;
      }
      else
      {
         Werrprintf("usage  -  %s :  incorrect string argument", argv[0]);
         return(ERROR);
      }

      arg_no += 1;
   }

   P_setstring(CURRENT, "plane", plane, 1);

/******************************
*  Parse REAL arguments next  *
******************************/

   sprintf(msge,
        "usage  -  %s :  string arguments must precede numeric arguments",
          argv[0]);

/**********************************************
*  Get argument specifying the plane number.  *
*  Not used if a projection is to displayed.  * 
**********************************************/

   if ( (argc > arg_no) && (!selproj) )
   {
      if ( isReal(argv[arg_no]) )
      {
         planeno = (int) (stringReal(argv[arg_no++]));
      }
      else
      {
         Werrprintf(msge);
      }
      if ( (argc > arg_no) && (pipeData) )
      {
         if ( isReal(argv[arg_no]) )
         {
            plane2no = (int) (stringReal(argv[arg_no++]));
            nD = 4;
         }
         else
         {
            Werrprintf(msge);
         }
      }
   }


   if (!selproj)
      selproj = (planeno == 0);


/******************************************
*  Get argument specifying the data file  *
*  number.  Implemented and tested but    *
*  not documented at this time.           *
******************************************/

 if (! pipeData)
 {
   if ( !P_getreal(CURRENT, "multdfiles", &tmp, 1) )
   {
      if ( (int)(tmp + 0.5) )
      {
         if (argc > arg_no) 
         { 
            if ( isReal(argv[arg_no]) )
            {
               datano = (int) (stringReal(argv[arg_no++])); 
            } 
            else 
            {
               Werrprintf(msge); 
            } 
         }
      }
   }

/*****************************************
*  Get the `path3d` value from the VNMR  *
*  parameter set.                        *
*****************************************/

   if ( P_getstring(CURRENT, "path3d", newdatapath, 1, MAXPATHL) )
   {
      strcpy(newdatapath, curexpdir);
/*    strcat(newdatapath, "/datadir3d/extr/");  */
      fname_cat(newdatapath, "datadir3d");
      fname_cat(newdatapath, "extr");
   }
   else
   {
/*    strcat(newdatapath, "/extr");  */
      fname_cat(newdatapath, "extr");
      if ( access(newdatapath, F_OK) )
      {
         strcpy(newdatapath, curexpdir);
         fname_cat(newdatapath, "datadir3d");

         P_setstring(CURRENT, "path3d", newdatapath, 1);
         fname_cat(newdatapath, "extr");
      }

/*    strcat(newdatapath, "/");  */
   }

   strcpy(procparpath, newdatapath);

/*************************************
*  Read in the 3D parameters if the  *
*  `newpar` flag is set to TRUE.     *
*************************************/

   if (newpar)
   {
      fname_cat(procparpath, "procpar3d");
      if ( load_newparams(procparpath) )
         return(ERROR);
   }
 }  /* end of not pipeData */

/*******************************************
*  Set up the maximum plane value and the  *
*  data file extension based upon the      *
*  VNMR parameter `plane`.                 *
*******************************************/

  if (nD == 3)
  {
   if ( (strcmp(plane, "f1f2") == 0) ||
	(strcmp(plane, "f2f1") == 0) )
   {
      if ( P_getreal(PROCESSED, "fn", &tmp, 1) )
      {
         Werrprintf("cannot get parameter `fn`");
         return(ERROR);
      }

      pipeDir = "yz";
      projPipeDir = "YZ";
      strcpy(plane, "f1f2");
      if (newpar)
         settrace("f1", "f2");
   }
   else if ( (strcmp(plane, "f1f3") == 0) ||
	     (strcmp(plane, "f3f1") == 0) )
   {
      if ( P_getreal(PROCESSED, "fn2", &tmp, 1) )
      {
         Werrprintf("cannot get parameter `fn2`");
         return(ERROR);
      }

      pipeDir = "xy";
      projPipeDir = "XY";
      strcpy(plane, "f1f3");
      if (newpar)
         settrace("f1", "f3");
   }
   else if ( (strcmp(plane, "f2f3") == 0) ||
	     (strcmp(plane, "f3f2") == 0) )
   {
      if ( P_getreal(PROCESSED, "fn1", &tmp, 1) )
      {
         Werrprintf("cannot get parameter `fn1`");
         return(ERROR);
      }

      pipeDir = "xz";
      projPipeDir = "XZ";
      strcpy(plane, "f2f3");
      if (newpar)
         settrace("f2", "f3");
   }
   else
   {
      Werrprintf("`plane` is not set to an allowed value");
      return(ERROR);
   }
  }
  else /* 4D */
  {
   if ( (strcmp(plane, "f1f2") == 0) ||
	(strcmp(plane, "f2f1") == 0) )   /* yz */
   {
      if ( P_getreal(PROCESSED, "fn3", &tmp, 1) )
      {
         Werrprintf("cannot get parameter `fn3`");
         return(ERROR);
      }
      if ( P_getreal(PROCESSED, "fn", &tmp2, 1) )
      {
         Werrprintf("cannot get parameter `fn`");
         return(ERROR);
      }

      pipeDir = "yz";
      projPipeDir = "YZ";
      strcpy(plane, "f1f2");
      if (newpar)
         settrace("f1", "f2");
   }
   else if ( (strcmp(plane, "f1f3") == 0) ||
	     (strcmp(plane, "f3f1") == 0) )
   {
      if ( P_getreal(PROCESSED, "fn2", &tmp, 1) )
      {
         Werrprintf("cannot get parameter `fn2`");
         return(ERROR);
      }
      if ( P_getreal(PROCESSED, "fn", &tmp2, 1) )
      {
         Werrprintf("cannot get parameter `fn`");
         return(ERROR);
      }

      pipeDir = "ya";
      projPipeDir = "YA";
      strcpy(plane, "f1f3");
      if (newpar)
         settrace("f1", "f3");
   }
   else if ( (strcmp(plane, "f2f3") == 0) ||
	     (strcmp(plane, "f3f2") == 0) )
   {
      if ( P_getreal(PROCESSED, "fn1", &tmp, 1) )
      {
         Werrprintf("cannot get parameter `fn1`");
         return(ERROR);
      }
      if ( P_getreal(PROCESSED, "fn", &tmp2, 1) )
      {
         Werrprintf("cannot get parameter `fn`");
         return(ERROR);
      }

      pipeDir = "za";
      projPipeDir = "ZA";
      strcpy(plane, "f2f3");
      if (newpar)
         settrace("f2", "f3");
   }
   else if ( (strcmp(plane, "f1f4") == 0) ||
	     (strcmp(plane, "f4f1") == 0) )        /* xy */
   {
      if ( P_getreal(PROCESSED, "fn3", &tmp, 1) )
      {
         Werrprintf("cannot get parameter `fn3`");
         return(ERROR);
      }
      if ( P_getreal(PROCESSED, "fn2", &tmp2, 1) )
      {
         Werrprintf("cannot get parameter `fn2`");
         return(ERROR);
      }

      pipeDir = "xy";
      projPipeDir = "XY";
      strcpy(plane, "f1f4");
      if (newpar)
         settrace("f1", "f4");
   }
   else if ( (strcmp(plane, "f2f4") == 0) ||
	     (strcmp(plane, "f4f2") == 0) )    /* xz */
   {
      if ( P_getreal(PROCESSED, "fn3", &tmp, 1) )
      {
         Werrprintf("cannot get parameter `fn3`");
         return(ERROR);
      }
      if ( P_getreal(PROCESSED, "fn1", &tmp2, 1) )
      {
         Werrprintf("cannot get parameter `fn1`");
         return(ERROR);
      }

      pipeDir = "xz";
      projPipeDir = "XZ";
      strcpy(plane, "f2f4");
      if (newpar)
         settrace("f2", "f4");
   }
   else if ( (strcmp(plane, "f3f4") == 0) ||
	     (strcmp(plane, "f4f3") == 0) )
   {
      if ( P_getreal(PROCESSED, "fn2", &tmp, 1) )
      {
         Werrprintf("cannot get parameter `fn2`");
         return(ERROR);
      }
      if ( P_getreal(PROCESSED, "fn1", &tmp2, 1) )
      {
         Werrprintf("cannot get parameter `fn1`");
         return(ERROR);
      }

      pipeDir = "xa";
      projPipeDir = "XA";
      strcpy(plane, "f3f4");
      if (newpar)
         settrace("f3", "f4");
   }
   else
   {
      Werrprintf("plane parameter is not set to an allowed value");
      return(ERROR);
   }
  }

   if (!selproj)
   {
      numplanes = (int) (tmp + 0.5) / 2;
      if ( (planeno > numplanes) || (planeno < 1) )
      {
         Werrprintf("plane number exceeds maximum or minimum value");
         return(ERROR);
      }
   }

 if (pipeData)
 {
   char *argv[3];

   if (selproj)
      sprintf(newdatapath,"pipe/proj%s.dat", projPipeDir);
   else if (nD == 3)
      sprintf(newdatapath,"pipe/%s/spec%04d.ft3", pipeDir, planeno);
   else
      sprintf(newdatapath,"pipe/%s/spec%04d%04d.ft4", pipeDir, planeno, plane2no);
   argv[0] = "pread";
   argv[1] = newdatapath;
   argv[2] = NULL;
   pipeRead(2, argv, 0, NULL);
 }
 else
 {
/*********************************
*  Select the data file number.  *
*********************************/

   sprintf(vnmrcmd, "selbuf('oldpar',%d)\n", datano);
   if ( execString(vnmrcmd) )
   {
      Werrprintf("cannot select data file for 2D plane data");
      return(ERROR);
   }

/**************************************
*  Get the appropriate path for the   *
*  new data file and save the param-  *
*  eters to the appropriately named   *
*  text files.                        *
**************************************/

   if ( removephasefile() )
      return(ERROR);

   D_getfilepath(D_DATAFILE, olddatapath, curexpdir);
   unlink(olddatapath);

   fname_cat(newdatapath, "data");
   strcat(newdatapath, plane);

   if (selproj)
   {
      strcat(newdatapath, ".proj");
   }
   else
   {
      strcat(newdatapath, ".");
      sprintf(fileext, "%d", planeno);
      strcat(newdatapath, fileext);
   }

   if ( link(newdatapath, olddatapath) )
   {
      if ( symlink(newdatapath, olddatapath) )
      {
         Werrprintf("cannot create link to 2D plane data file");
         return(ERROR);
      }
   }

   if ( (r = D_open(D_DATAFILE, olddatapath, &datafhead)) )
   {
      D_error(r);
      return(ERROR);
   }
 }

   if ( P_setreal(CURRENT, "index2", (double)planeno, 1) )
   {
      P_creatvar(CURRENT,"index2",ST_INTEGER);
      P_setgroup(CURRENT,"index2",G_PROCESSING);
      P_setprot(CURRENT,"index2",P_ARR | P_ACT | P_VAL);
   }
   P_copyvar(CURRENT,PROCESSED,"index2","index2");
   if ( P_setreal(CURRENT, "planeno", (double)planeno, 1) )
   {
      P_creatvar(CURRENT,"planeno",ST_INTEGER);
      P_setgroup(CURRENT,"planeno",G_PROCESSING);
      P_setprot(CURRENT,"planeno",P_ARR | P_ACT | P_VAL);
   }
   P_copyvar(CURRENT,PROCESSED,"planeno","planeno");
   if (nD == 4)
   {
      if ( P_setreal(CURRENT, "plane2no", (double)planeno, 1) )
      {
         P_creatvar(CURRENT,"plane2no",ST_INTEGER);
         P_setgroup(CURRENT,"plane2no",G_PROCESSING);
         P_setprot(CURRENT,"plane2no",P_ARR | P_ACT | P_VAL);
      }
      P_copyvar(CURRENT,PROCESSED,"plane2no","plane2no");
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	       showbuf()/2		|
|					|
+--------------------------------------*/
int showbuf(int argc, char *argv[], int retc, char *retv[])
{
   char	fext[15],
	filename[MAXPATHL];
   int	i,
	arg_no,
	showactive = FALSE,
	showbufs = FALSE;


   arg_no = 1;

   while ( (arg_no < argc) && (!isReal(argv[arg_no])) )
   {
      if ( strcmp(argv[arg_no], "all") == 0 )
      {
         showbufs = showactive = TRUE;
      }
      else if ( strcmp(argv[arg_no], "active") == 0 )
      {
         showactive = TRUE;
      }
      else
      {
         Werrprintf("usage - %s:  improper string argument", argv[0]);
         ABORT;
      }

      arg_no += 1;
   }

   if (!showactive)
      showbufs = TRUE;

   if (showbufs)
   {
      Wscrprintf("\nCurrent Data Files:\n\n");
      for (i = 0; i < (MAX_DATA_BUFFERS + 1); i++)
      {
         if (curdatainfo.status[i])
         {
            strcpy(filename, "data");
            if (i)
            {
               sprintf(fext, ".%d", i);
               strcat(filename, fext);
            }

            Wscrprintf("\t%s", filename);
            strcpy(filename, "phasefile");
            if (i)
               strcat(filename, fext);

            Wscrprintf("\t%s\n", filename);
         }
      }
   }

   if (showactive)
   {
      Wscrprintf("\nActive Data File:\n\n");
      strcpy(filename, "data");
      if (curdatainfo.active)
      {
         sprintf(fext, ".%d", curdatainfo.active);
         strcat(filename, fext);
      }

      Wscrprintf("\t%s", filename);
      strcpy(filename, "phasefile");
      if (curdatainfo.active)
         strcat(filename, fext);

      Wscrprintf("\t%s\n", filename);
   }
               
   RETURN;
}


/*---------------------------------------
|					|
|	  removedatafiles()/2		|
|					|
+--------------------------------------*/
void removedatafiles(int datano, char *curdir)
{
   char	filepath[MAXPATHL];


   D_getfilepath(D_DATAFILE, filepath, curdir);
   unlink(filepath);
   D_getfilepath(D_PHASFILE, filepath, curdir);
   unlink(filepath);
   D_getparfilepath(CURRENT, filepath, curdir);
   unlink(filepath);
   D_getparfilepath(PROCESSED, filepath, curdir);
   unlink(filepath);

   curdatainfo.status[datano] = FALSE;
}


/*---------------------------------------
|					|
|	    finddatainfo()/0		|
|					|
+--------------------------------------*/
void finddatainfo()
{
   char		basepath[MAXPATHL],
		datapath[MAXPATHL],
		fext[4];
   int		i;


   curdatainfo.status[0] = TRUE;	/* default for main data file	*/
   curdatainfo.active = 0;		/* selects main data file	*/

   strcpy(basepath, curexpdir);
   fname_cat(basepath, "datdir");
   fname_cat(basepath, "data");

   for (i = 1; i <= MAX_DATA_BUFFERS; i++)
   {
      strcpy(datapath, basepath);
      sprintf(fext, ".%d", i);
      strcat(datapath, fext);
      curdatainfo.status[i] = ( access(datapath, F_OK) ? FALSE : TRUE );
   }
}


/*---------------------------------------
|					|
|	   resetdatafiles()/0		|
|					|
+--------------------------------------*/
void resetdatafiles()
{
   if ( execString("delbuf('all')\n") )
      Werrprintf("cannot destroy all the data files");
   finddatainfo();
}


/*---------------------------------------
|					|
|	   getactivefile()/0		|
|					|
+--------------------------------------*/
int getactivefile()
{
   return(curdatainfo.active);
}


/*---------------------------------------
|					|
|	      selbuf()/2		|
|					|
+--------------------------------------*/
int selbuf(int argc, char *argv[], int retc, char *retv[])
{
   char		filepath[MAXPATHL],
		msge[MAXSTR];
   int		datano = 0,
		arg_no = 1,
		oldpar = FALSE,
		destroyall = FALSE,
		select,
		files_exist,
		r,
		i;
   dfilehead	datafhead;


/************************************
*  Initialize all parameterizeable  *
*  variables                        *
************************************/

   select = ( strcmp(argv[0], "selbuf") == 0 );
   Wturnoff_buttons();

/********************************
*  Check for string arguments.  *
********************************/

   while ( (arg_no < argc) && (!isReal(argv[arg_no])) )
   {
      if ( (strcmp(argv[arg_no], "all") == 0) && (!select) )
      {
         destroyall = TRUE;
      }
      else if ( (strcmp(argv[arg_no], "oldpar") == 0) && select )
      {
         oldpar = TRUE;
      }
      else
      {
         Werrprintf("usage - %s:  invalid string argument", argv[0]);
         return(ERROR);
      }
         
      arg_no += 1;
   }

/********************************
*  Get argument specifying the  *
*  data file number.            *
********************************/

   if ( (argc > arg_no) && (!destroyall) )
   {
      if ( isReal(argv[arg_no]) )
      {
         datano = (int) (stringReal(argv[arg_no++]));
         if (datano < 0)
         {
            Werrprintf("usage  -  %s :  invalid data file number", argv[0]);
            ABORT;
         }
         else if (datano > MAX_DATA_BUFFERS)
         {
            Werrprintf(
		"usage  -  %s :  exceeds maximum data file number of %d",
		    argv[0], MAX_DATA_BUFFERS);
         }
      }
      else
      {
         Werrprintf(msge);
      }
   }


   if ( (!select) && (!destroyall) && (datano == 0) )
   {
      Werrprintf("usage - delbuf:  cannot destroy the main data file");
      return(ERROR);
   }

/****************************************
*  Close the data and phase files only  *
*  if they are already open.            *
****************************************/

   if (curdatainfo.active != datano)
   {
      r = D_gethead(D_DATAFILE, &datafhead);
      if (r)
      {
         if (r != D_NOTOPEN)
            D_trash(D_DATAFILE);
      }
      else
      {
         D_close(D_DATAFILE);
      }

      r = D_gethead(D_PHASFILE, &datafhead);
      if (r) 
      { 
         if (r != D_NOTOPEN) 
            D_trash(D_PHASFILE);
      }
      else
      {
         D_close(D_PHASFILE);
      }
   }
   else
   {
      D_trash(D_DATAFILE);
      D_trash(D_PHASFILE);
   }

/*************************************
*  Get paths of data and parameter   *
*  files either to be removed or to  *
*  be used.                          *
*************************************/

   if (!select)
   {
      if (destroyall)
      {
         for (i = 1; i < (MAX_DATA_BUFFERS + 1); i++)
         {
            if (curdatainfo.status[i])
            {
               setfilepaths(i);
               removedatafiles(i, curexpdir);
            }
         }

         setfilepaths(0);

         if (curdatainfo.active)
         {
            curdatainfo.active = 0;
            D_getparfilepath(CURRENT, filepath, curexpdir);
            P_read(CURRENT, filepath);
            D_getparfilepath(PROCESSED, filepath, curexpdir);
            P_read(PROCESSED, filepath);
         }
      }
      else
      {
         if (curdatainfo.status[datano])
         {
            if (datano == 0)
            {
               Werrprintf("cannot destroy main data files");
               ABORT;
            }

            setfilepaths(datano);
            removedatafiles(datano, curexpdir);

            if (curdatainfo.active == datano)
            {
               setfilepaths(0);
               curdatainfo.active = 0;
               D_getparfilepath(CURRENT, filepath, curexpdir);
               P_read(CURRENT, filepath);
               D_getparfilepath(PROCESSED, filepath, curexpdir);
               P_read(PROCESSED, filepath);
            }
            else
            {
               setfilepaths(curdatainfo.active);
            }
         }
      }

      RETURN;
   }

/*********************************************
*  Save parameters in the current parameter  *
*  files if `selbuf` has been executed.      *
*********************************************/

   if (curdatainfo.active == datano)
   {
      RETURN;
   }
   else
   {
      D_getparfilepath(CURRENT, filepath, curexpdir);
      if ( P_save(CURRENT, filepath) )
      {
         Werrprintf("cannot save current parameters");
         ABORT;
      }

      D_getparfilepath(PROCESSED, filepath, curexpdir);
      if ( P_save(PROCESSED, filepath) ) 
      { 
         Werrprintf("cannot save processed parameters");
         ABORT;
      }
   }

/**************************************************
*  Create parameter files either if ones should   *
*  not already exist or should exist but do not.  *
*  If parameter files should exist and do exist,  *
*  read them in to VNMR memory.                   *
**************************************************/

   files_exist = curdatainfo.status[datano];
   setfilepaths(datano);
   D_getparfilepath(CURRENT, filepath, curexpdir);

   if (!files_exist)
   {
      if ( P_save(CURRENT, filepath) )
      {
         Werrprintf("error saving new current parameters");
         setfilepaths(curdatainfo.active);
         ABORT;
      }
   }
   else if ( P_read(CURRENT, filepath) || oldpar )
   {
      if ( P_save(CURRENT, filepath) )
      {
         Werrprintf("error saving new current parameters");
         setfilepaths(curdatainfo.active);
         ABORT;
      }
   }

   D_getparfilepath(PROCESSED, filepath, curexpdir); 
   if (!files_exist)
   {   
      if ( P_save(PROCESSED, filepath) )
      {
         Werrprintf("error saving new processed parameters"); 
         setfilepaths(curdatainfo.active);
         ABORT; 
      } 
   }
   else if ( P_read(PROCESSED, filepath) || oldpar )
   { 
      if ( P_save(PROCESSED, filepath) ) 
      { 
         Werrprintf("error saving new processed parameters");
         setfilepaths(curdatainfo.active);
         curdatainfo.status[datano] = FALSE;
         ABORT;
      }

      curdatainfo.status[datano] = TRUE;  
   }

   curdatainfo.status[datano] = TRUE;  
   curdatainfo.active = datano;		/* last thing to do */

   RETURN;
}

static char *fname_cat(char *base, char *increment )
{
#ifdef UNIX
	strcat( base, "/" );
	strcat( base, increment );

	return( base );
#else 
#define  NAM$C_MAXRSS	256

	char	tbuf[ NAM$C_MAXRSS ];
	int	blen;

	blen = strlen( base );
	if (blen < 1) {
		strcpy( base, increment );
		return( base );
	}

	if (base[ blen-1 ] != ']' && base[ blen-1 ] != ':') {
		make_vmstree( base, &tbuf[ 0 ], sizeof( tbuf ) );
		strcpy( base, &tbuf[ 0 ] );
		strcat( base, increment );
	}
	else
	  strcat( base, increment );

	return( base );
#endif 
}

#ifdef VMS

#define  NAM$C_MAXRSS	256
#undef   mkdir

int vf_mkdir(char *newdirpath, int protval )
{
	int		 ival, len, present_and_last;
	int		 current_descr[ 2 ], parent_descr[ 2 ], rms_status;
	char		 current_dir[ NAM$C_MAXRSS ],
			 parent_dir[ NAM$C_MAXRSS ],
			 subdir_name[ NAM$C_MAXRSS ],
			 wspace[ NAM$C_MAXRSS ];
	char		*rbptr, *tptr;

	len = strlen( newdirpath );
	if (len < 1 || len > NAM$C_MAXRSS-5) {
		errno = EINVAL;
		return( -1 );
	}
	strcpy( &wspace[ 0 ], newdirpath );

	rbptr = rindex( &wspace[ 0 ], ']' );
	if (rbptr == NULL)
	  present_and_last = 0;
	else
	  present_and_last = (rbptr[ 1 ] == '\0');

	if (present_and_last) {
		(void) get_parentdir( &wspace[ 0 ], &wspace[ 0 ], NAM$C_MAXRSS );
		if (access( &wspace[ 0 ], 0) == 0) {
			errno = EEXIST;
			return( -1 );
		}

		ival = get_parentdir_filename(
			&wspace[ 0 ], &parent_dir[ 0 ], &subdir_name[ 0 ]
		);
		if (ival != 0)
		  return( -1 );
	}
	else {
		ival = get_parentdir_filename(
			&wspace[ 0 ], &parent_dir[ 0 ], &subdir_name[ 0 ]
		);
		if (ival != 0)
		  return( -1 );

		fix_subdir_name( &subdir_name[ 0 ] );

		strcpy( &wspace[ 0 ], &parent_dir[ 0 ] );
		strcat( &wspace[ 0 ], &subdir_name[ 0 ] );
		if (access( &wspace[ 0 ], 0) == 0) {
			errno = EEXIST;
			return( -1 );
		}
	}

	current_dir[ 0 ] = '\0';
	parent_descr[ 0 ] = strlen( &parent_dir[ 0 ] );
	if (parent_descr[ 0 ] > 0) {
		tptr = getwd( &current_dir[ 0 ] );
		if (tptr == NULL) {
			errno = ENOENT;
			return( -1 );
		}

		parent_descr[ 1 ] = &parent_dir[ 0 ];
		rms_status = SYS$SETDDIR( &parent_descr[ 0 ], NULL, NULL );
		if ( (rms_status & 3) != 1) {
			errno = ENOENT;
			return( -1 );
		}
	}
	ival = mkdir( &subdir_name[ 0 ], protval );
	if (strlen( &current_dir[ 0 ] ) > 0) {
		current_descr[ 0 ] = strlen( &current_dir[ 0 ] );
		current_descr[ 1 ] = &current_dir[ 0 ];
		rms_status = SYS$SETDDIR( &current_descr[ 0 ], NULL, NULL );
	}
	return( ival );
}

static int get_parentdir_filename(char *working, char *parentdir, char *filename )
{
	char	*rbptr;
	int	 rbndx;

	rbptr = rindex( working, ']' );
	if (rbptr == NULL)
	  rbptr = working-1;

	rbndx = rbptr - working;
	strcpy( parentdir, working );
	parentdir[ rbndx+1 ] = '\0';
	strcpy( filename, working + (rbndx + 1) );

	return( 0 );
}

static int fix_subdir_name(char *subdir_name )
{
	char	*dptr, *qptr;

	qptr = rindex( subdir_name, ';' );		/* Eliminate version # */
	if (qptr != NULL)
	  *qptr = '\0';

	dptr = rindex( subdir_name, '.' );	/* If no extension, append .DIR */
	if (dptr == NULL)
	  strcat( subdir_name, ".DIR" );

	return( 0 );
}
#endif 
