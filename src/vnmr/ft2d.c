/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/****************************************************************/
/* ft2d.c -  2D FT program                                      */
/*      ft1d    first ft                                        */
/*      wft1d   first ft with weighting                         */
/*      ft2d    first and second ft                             */
/*      wft2d   first and second ft with weighting              */
/****************************************************************/


#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "data.h"
#include "ftpar.h"
#include "process.h"
#include "group.h"
#include "variables.h"
#include "vnmrsys.h"
#include "sky.h"
#include "tools.h"
#include "allocate.h"
#include "pvars.h"
#include "wjunk.h"
#include "displayops.h"
#include "fft.h"
 

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
#define FALSE		0
#define TRUE		1
#define MAXSTR		256
#define DISPSIZE	1024		/* maxsize for no Index Display */
#define DISPMOD		255		/* Modulo for Index Display	*/
#define CONV_TO_RAD	0.01745329252
#define SMALL_VALUE	1.0e-20

#define zerofill(data_pntr, npoints_to_fill)			\
			datafill(data_pntr, npoints_to_fill,	\
				 0.0)

extern void Wturnoff_buttons();
extern int init_wt1(struct wtparams *wtpar, int fdimname);
extern int init_wt2(struct wtparams *wtpar, register float  *wtfunc,
             register int n, int rftflag, int fdimname, double fpmult, int rdwtflag);
extern int datapoint(double freq, double sw, int fn);
extern void ls_ph_fid(char *lsfname, int *lsfval, char *phfname, double *phfval,
                      char *lsfrqname, double *lsfrqval);
extern void phasefunc(float *phasepntr, int npnts, double lpval, double rpval);
extern int setlppar(lpstruct *parLPinfo, int dimen, int pstatus,
             int nctdpts, int ncfdpts, int mode, char *memId);
extern int init2d(int get_rev, int dis_setup);
extern int disp_current_seq();
extern int i_ft(int argc, char *argv[], int setstatus, int checkstatus, int flag2d,
                ftparInfo *ftpar, dfilehead *fidhead, dfilehead *datahead,
                dfilehead *phasehead);
extern void long_event();
extern int getfid(int curfid, float *outp, ftparInfo *ftpar, dfilehead *fidhead,
                int *lastfid);
extern int downsamp(dsparInfo *dspar, float *data, int ncdpts,
             int nclspts, int fn0, int realdata);
extern int lpz(int trace, float *data, int nctdpts, lpinfo lpparvals);
extern int fidss(ssparInfo sspar, float *data, int ncdpts, int nclspts);
extern void   set_vnmrj_ft_params(int procdim, int argc, char *argv[]);

extern int      interuption,	/* flag for "cancel command "		*/
                Bnmr,           /* background flag			*/
		acqflag,	/* acquisition flag			*/
                specIndex,
                c_first,
                c_last,
                c_buffer;
 
extern htPar htpar;
static int htindex;

int             readwtflag = FALSE;
int             start_from_ft2d = 1;

double		fpointmult;	/* value of first point multiplier      */
coefs		arraycoef[MAX2DARRAY];
static lpinfo	lpparams;
static void ft2d_cleanup(char message[], int reset);


typedef struct wtparams	wtPar;

double getfpmult(int fdimname, int ddr);
static int getrdblocks(int nblocks, int f1_zflevel);
static int reset_dataheader();
int getzfnumber(int ndpts, int fnsize);
int getzflevel(int ndpts, int fnsize);

	/********************
	*  Begin Functions  *
	********************/


/*---------------------------------------
|                                       |
|             setheader()/5             |
|                                       |
+--------------------------------------*/
void setheader(dpointers *block, int status_value, int mode_value,
               int index_value, int hypercomplex)
{
   dblockhead	*blockpointer;

   block->head->status = (short)status_value;
   block->head->index  = (short)index_value;
   block->head->mode   = (short)mode_value;
   block->head->rpval  = 0.0;
   block->head->lpval  = 0.0;
   block->head->lvl    = 0.0;
   block->head->tlt    = 0.0;

   blockpointer = block->head;
   if (hypercomplex)
   {
      hycmplxhead    *newblockpointer;

      blockpointer->status |= MORE_BLOCKS;
      blockpointer += 1;

      newblockpointer = (hycmplxhead *)blockpointer;
      newblockpointer->status = U_HYPERCOMPLEX;
      newblockpointer->s_spare1 = 0;
      newblockpointer->s_spare2 = 0;
      newblockpointer->s_spare3 = 0;
      newblockpointer->l_spare1 = 0;
      newblockpointer->lpval1 = 0.0;
      newblockpointer->rpval1 = 0.0;
      newblockpointer->f_spare1 = 0.0;
      newblockpointer->f_spare2 = 0.0;
   }

/*
   Additional "if" statements can be used to add and 
   initialize more block headers if desired.      SF
*/

}
 

/*---------------------------------------
|					|
|	     dim1count()/0		|
|					|
+--------------------------------------*/
int dim1count()
{
   int		r;
   double	rval;

   if ( (r = P_getreal(PROCESSED, "arraydim", &rval, 1)) ) 
   {
      P_err(r, "processed ", "arraydim:");
      return(0);
   }

   return((int)rval);
}


/*---------------------------------------
|					|
|	      zeroimag()/3		|
|					|
|  This function zeroes the imaginary	|
|  points in both F1 spectral and t2	|
|  FID data.				|
|					|
+--------------------------------------*/
void zeroimag(float *dataptr, int np, int dispflag)
{
  register int		i;
  register float	*inptr,
			zero;

  inptr = dataptr + 1;
  i = np/2;
  zero = 0.0;
  while (i--)
  {
     *inptr = zero;
     inptr += 2;
  }
}


/*---------------------------------------
|					|
|	     rotate_fid()/5		|
|					|
|   This function performs a zero-	|
|   order phase rotation on the t2	|
|   FID data.				|
|					|
+--------------------------------------*/
void rotate_fid(float *fidptr, double ph0, double ph1, int np, int datatype)
{
   ph1 *= 2*(np/2 - 1);
   ph0 -= ph1;

   switch (datatype)
   {
      case HYPERCOMPLEX:  rotate4(fidptr, np/2, ph1, ph0, 0.0, 0.0,
				    2, 1, TRUE);
			  break;
      case COMPLEX:
      default:		  rotate2(fidptr, np/2, ph1, ph0);
   }
}


/*---------------------------------------
|					|
|	      findphases()/3		|
|					|
|  This function finds the appropriate	|
|  phasing constants.			|
|					|
+--------------------------------------*/
int findphases(double *rpval, double *lpval, int fdimname)
{
  char	lpname[6],
	rpname[6];
  int	res;


/*********************************************
*  Determine which phasing constants to use  *
*********************************************/

  if (fdimname & S_NI2)
  {
     strcpy(lpname, "lp2");
     strcpy(rpname, "rp2");
  }
  else if (fdimname & (S_NI|S_NF))
  {
     strcpy(lpname, "lp1");
     strcpy(rpname, "rp1");
  }
  else if (fdimname & S_NP)
  {
     strcpy(lpname, "lp");
     strcpy(rpname, "rp");
  }


/*************************************
*  Get values for phasing constants  *
*************************************/

  if ( (res = P_getreal(CURRENT, lpname, lpval, 1)) )
  {
     P_err(res, "current ", lpname);
     return(ERROR);
  }

  if ( (res = P_getreal(CURRENT, rpname, rpval, 1)) )
  {
     P_err(res, "current ", rpname);
     return(ERROR);
  }

  return(COMPLETE);
}


/*---------------------------------------
|					|
|		fnpower()/1		|
|					|
|   This function computes the FT	|
|   level number.			|
|					|
+--------------------------------------*/
int fnpower(int fni)
{
  int	pwr,
	fn;

  pwr = 4;
  fn = 32;
  while (fn < fni)
  {
     fn *= 2;
     pwr++;
  }

  return(pwr);
}


/*-----------------------------------------------
|						|
|	       combinespectra()/21		|
|						|
|  Combines several spectra after the first FT  |
|  with defined coefficients.			|
|						|
+----------------------------------------------*/
static int combinespectra(fidnum, outp, combinebuf, addbuf, wtfunc,
			    rotatebuf, pwr, lastfid, datatype, npx,
			    nnp, nzflvl, nzfnum, ftpar, fidhead,
			    wtflag, parLPdata, dsfn0, dsnpx, dsnnp,
                            dspwr, ftflag)
int		fidnum,
		datatype,
		pwr,
		*lastfid,
		npx,
		nnp,
		nzflvl,
		nzfnum,
		wtflag,
		dsfn0,
		dsnpx,
		dsnnp,
		ftflag,
		dspwr;
float		*outp,
		*combinebuf,
		*addbuf,
		*wtfunc,
		*rotatebuf;
ftparInfo	*ftpar;
dfilehead	*fidhead;
lpstruct	*parLPdata;
{
  int		i,
		tmpi,
		fidindex,
		fidcnt,
		realt2data;
  int		gethtfid();
  void	weightfid();


  realt2data = (ftpar->procstatus & REAL_t2);
  
/***********************************
*  Clear the output buffer first.  *
***********************************/

  zerofill(outp, (dsfn0/2) * datatype);	/* complex or hypercomplex data */

/**************************************
*  Transform each FID and add to the  *
*  the output buffer.                 *
**************************************/

  for (fidcnt = 0; fidcnt < ftpar->arraydim; fidcnt++)
  {
     if (((ftpar->fn1 > DISPSIZE) || (ftpar->fn0 > DISPSIZE)) && 
				!(ftpar->displaynum & DISPMOD))
        disp_index(ftpar->displaynum);

/********************************
*  Calculate FID index (block)  *
********************************/

     fidindex = ( (ftpar->D_dimname & S_NF) ? fidnum : (fidnum + fidcnt) );

/****************
*  Get the FID  *
****************/

     if (ftpar->procstatus & HT_F1PROC)
     {
        if ((htindex < htpar.fsize) && (fidindex == htpar.index[htindex]))
        {
           if ( gethtfid(htindex+htpar.niofs, combinebuf, ftpar,
                         fidhead, lastfid, addbuf, 1, datatype) )
   	   { 
              ft2d_cleanup("", FALSE);
	      return(ERROR);
	   }
           htindex++;
        }
        else
        {
         ftpar->displaynum += 1;
         if (ftpar->D_dimname & S_NF)
            ftpar->cf += 1;
         return(COMPLETE);
        }
     }
     else if ( getfid(fidindex, combinebuf, ftpar, fidhead, lastfid) )
     {
	Werrprintf("Unable to get FID data");
        return(ERROR);
     }
     else if (fidindex == *lastfid)
     {
      
/**************************************
*  Check to insure there is at least  *
*  one FID with data.                 *
**************************************/
 
        if (*lastfid == 0)
        {
           if (reset_dataheader())
              Werrprintf("Unable to zero DATA file header");
           return(ERROR);
        }
 
/***********************************************
*  If there is no more FID data in the middle  *
*  of a combination, zero the "outp", set      *
*  "*lastfid" equal to the first FID of the    *
*  arrayed set, and exit the "for" loop.       *
***********************************************/
 
        *lastfid = fidnum;   /* --bf */
        disp_status("ZF     ");
	if ((ftpar->fn1 > DISPSIZE) || (ftpar->fn0 > DISPSIZE))
           disp_index(0);
        zerofill(outp, dsfn0/2*datatype);
        fidcnt = ftpar->arraydim;
        return(COMPLETE);
     }

/*************************************
*  Increment the FID display number  *
*  and CF if appropriate.            *
*************************************/

     ftpar->displaynum += 1;
     if (ftpar->D_dimname & S_NF)
        ftpar->cf += 1;

/*****************************************
*  Perform LP data extension at this time  *
*******************************************/

     if (ftpar->sspar.zfsflag || ftpar->sspar.lfsflag)
     {
        if ( fidss(ftpar->sspar, combinebuf, npx/2, ftpar->lsfid0/2) )
        {
           Werrprintf("time-domain solvent subtraction failed");
           return(ERROR);
        }
     }

     if (parLPdata->sizeLP)
     {
        for (i = 0; i < parLPdata->sizeLP; i++)
        {
           lpparams = *(parLPdata->parLP + i);
           if (lpz(fidindex, combinebuf, (npx - ftpar->lsfid0)/2, lpparams))
           {
              Werrprintf("LP analysis failed");
              return(ERROR);
           }
        }
     }

/**********************************************
*  Zero-fill time-domain data to the closest  *
*  power of 2.                                *
**********************************************/

     if (ftpar->dspar.dsflag)
     {
       if (ftpar->fn0-nnp > 0)
         zerofill(combinebuf + nnp, ftpar->fn0-nnp);
     }
     else
     {
       if (nzfnum > 0)
         zerofill(combinebuf + nnp, nzfnum);
     }

/************************
*  Weight the FID data  *
************************/

     if (wtflag)
     {
        weightfid(wtfunc, combinebuf, nnp/2, realt2data, COMPLEX);
     }
     else if (fpointmult != 1.0)
     {
        *combinebuf *= fpointmult;
        *(combinebuf + 1) *= fpointmult;
     }

   if (ftpar->dspar.dsflag)
   {
     if (downsamp(&(ftpar->dspar), combinebuf, nnp/2, 0,
                ftpar->fn0, ftpar->procstatus&REAL_t2))
     {
       Werrprintf("digital filtering failed");
       releaseAllWithId("ft2d");
       disp_index(0);
       disp_status("        ");
       ABORT;
     }

     tmpi = ftpar->fn0; ftpar->fn0 = dsfn0; dsfn0 = tmpi;
     tmpi = npx; npx = dsnpx; dsnpx = tmpi;
     tmpi = nnp; nnp = dsnnp; dsnnp = tmpi;
     tmpi = pwr; pwr = dspwr; dspwr = tmpi;
     if (nzfnum > 0)
       zerofill(combinebuf + nnp, nzfnum);
     }

/************************************
*  Process the data in some manner  *
************************************/

   if (ftflag)
   {
     if ( ftpar->procstatus & (FT_F2PROC|LP_F2PROC) )
     {
        fft(combinebuf, ftpar->fn0/2, pwr, nzflvl, COMPLEX, COMPLEX, -1.0,
		FTNORM, (nnp + nzfnum)/2);
        if (realt2data)
           postproc(combinebuf, ftpar->fn0/2, COMPLEX);
        if ((ftpar->dspar.dsflag) && (ftpar->dspar.lp != 0.0))
          rotate2_center(combinebuf, ftpar->fn0/2, ftpar->dspar.lp*
                (ftpar->fn0/2)/(ftpar->fn0/2-1.0), (double) 0.0);
        if (fabs(ftpar->lpval) > MINDEGREE)
          rotate2_center(combinebuf, ftpar->fn0/2, ftpar->lpval*
                (ftpar->fn0/2)/(ftpar->fn0/2-1.0), (double) 0.0);
        if (rotatebuf != NULL)
        {
           blockrotate2(combinebuf, rotatebuf, 1, 1, 1, ftpar->fn0/2,
			COMPLEX, 1, FALSE);
        }
     }
     else if (ftpar->procstatus & MEM_F2PROC)
     {
	Werrprintf("MEM processing:  not currently supported");
        Werrprintf("Use of coefficients not suitable for MEM processing");
        return(ERROR);
     }
   }
   else if (ftpar->fn0 > nnp)
   {
      zerofill(combinebuf + nnp, ftpar->fn0-nnp);
   }

/**********************************************
*  Form either the complex or hypercomplex    *
*  t1(t3)-interferogram from the F2 data      *
*  and the entered processing coefficients.   *
**********************************************/

     combine(combinebuf, outp, ftpar->fn0/2, datatype,
	arraycoef[fidcnt].re2d.re, arraycoef[fidcnt].re2d.im,
	arraycoef[fidcnt].im2d.re, arraycoef[fidcnt].im2d.im);

     if (ftpar->dspar.dsflag)
     {
        tmpi = ftpar->fn0; ftpar->fn0 = dsfn0; dsfn0 = tmpi;
        tmpi = npx; npx = dsnpx; dsnpx = tmpi;
        tmpi = nnp; nnp = dsnnp; dsnnp = tmpi;
        tmpi = pwr; pwr = dspwr; dspwr = tmpi;
     }
  }
  if (fabs(ftpar->daslp) > MINDEGREE)
  {
     if (datatype == 4)
     {
        rotate4_center(outp, ftpar->fn0/2, (double) -ftpar->daslp*(fidnum/2)*(ftpar->fn0/2)/(ftpar->fn0/2-1.0),
            (double)0.0, (double)0.0, (double)0.0, 0, 0, 1);
     }
     else 
     {
        rotate2_center(outp, ftpar->fn0/2,
                   (double) -ftpar->daslp*(fidnum/2)*(ftpar->fn0/2)/(ftpar->fn0/2-1.0),
                    (double)0.0);
     }
  }

  return(COMPLETE);
}


/*---------------------------------------
|					|
|	    ft2d_cleanup()/2		|
|					|
+--------------------------------------*/
static void ft2d_cleanup(char message[], int reset)
{
   if (strcmp(message, "") != 0)
      Werrprintf(message);

   skyreleaseAllWithId("ft2d");
   disp_index(0);
   disp_status("       ");
   if (reset)
   {
      if (reset_dataheader())
         Werrprintf("Unable to zero DATA file header");
   }
}


/*----------------------------------
|				   |
|            firstft()/3	   |
|				   |
+---------------------------------*/
static int firstft(int fullft, ftparInfo *ftpar, dfilehead *fidhead, int ftflag)
{
  int		i,
		startfid,
		stopfid,
		stepfid,
                fid0,
		nf_startfid = 0,
		dof2phase,
		npx,
		npadj,
		lsfidx,
		blockmode,
		blockstatus,
		pwr,
		cblock,
		blocksdone,
		res,
		fidnum,
		fidcnt,
		lastfid,
		wtsize,
		zflevel,
		zfnumber,
		realt2data,
		typeofdata;
  float		*outp,
		*combinebuf = NULL,
		*addbuf = NULL,
		*wtfunc,
		*rotatebuf;
  double	rpx,
		lpx;
  float		*finaloutp = NULL;
  int		dsfn0,
		dsnpx,
		dsnpadj,
		dspwr,
		tmpi;
  void	        weightfid();
  int		htinit(),
  		gethtfid();
  lpstruct	parLPinfo;
  dpointers	outblock;
  wtPar		wtp;


/*******************************************
*  Find phasing constants if appropriate.  *
*******************************************/

  if ( ((ftpar->D_dsplymode & NP_PHMODE) || (ftpar->D_dsplymode & NP_PAMODE))
		&& (~(ftpar->D_cmplx & NP_CMPLX)) )
  {
     if (findphases(&rpx, &lpx, S_NP))
        return(ERROR);
     dof2phase = ((fabs(rpx) > MINDEGREE) || (fabs(lpx) > MINDEGREE));
  }
  else
  {
     lpx = 0.0;
     rpx = 0.0;
     dof2phase = 0;
  }

/**************************************************
*  Set "type-of-data":  complex or hypercomplex.  *
**************************************************/

  typeofdata = ( (ftpar->hypercomplex) ? HYPERCOMPLEX : COMPLEX );

/***************************************************
*  np0  -  number of points in the FID             *
*  npx  -  used number of points in the FID        *
*  fn0  -  number of points in the converted file  *
*                                                  *
*  Adjust "npx" and "lsfidx".                      *
***************************************************/

  lsfidx = ftpar->lsfid0;
  if (ftpar->dspar.dsflag)
  {
    dsnpx = ((ftpar->fn0 < (ftpar->dspar.finalnp-lsfidx/ftpar->dspar.dsfactor))?
                 (ftpar->fn0 + lsfidx/ftpar->dspar.dsfactor) :
                  ftpar->dspar.finalnp );
    dsnpadj = dsnpx - lsfidx/ftpar->dspar.dsfactor;
    ftpar->dspar.finalnp = dsnpadj;
    dsfn0 = ftpar->fn0;
    dspwr = fnpower(dsfn0);
    ftpar->fn0 = (dsfn0*ftpar->dspar.dsfactor > ftpar->np0) ?
                 dsfn0*ftpar->dspar.dsfactor : ftpar->np0;
    tmpi = 32;
    while (tmpi < ftpar->fn0)
      tmpi *= 2;
    ftpar->fn0 = tmpi;
    npx = ((ftpar->fn0 < (ftpar->np0 - lsfidx)) ? (ftpar->fn0 + lsfidx) :
                        ftpar->np0);
    npadj = npx - lsfidx;
    pwr = fnpower(ftpar->fn0);
  }
  else
  {
    npx = dsnpx = ((ftpar->fn0 < (ftpar->np0 - lsfidx)) ? (ftpar->fn0 + lsfidx)
               : ftpar->np0 );
    npadj = dsnpadj = npx - lsfidx;
    pwr = dspwr = fnpower(ftpar->fn0);
    dsfn0 = ftpar->fn0;
  }

  if (lsfidx < 0)
  {
     if (npx < 2)
     {
        Werrprintf("lsfid is too large in magnitude");
        skyreleaseAllWithId("ft2d");
        return(ERROR);
     }
  }
  else
  {
     if (lsfidx >= npx)
     {
        Werrprintf("lsfid is too large in magnitude");
        skyreleaseAllWithId("ft2d");
        return(ERROR);
     }
  }

/*******************************************************
*  Allocate COMBINE buffer (combinebuf) to FN0 points  *
*  with 4 bytes per point for complex data; one must   *
*  double this amount for hypercomplex data.           *
*******************************************************/

  if (ftpar->combineflag)
  {
     if ( (combinebuf = (float *) skyallocateWithId(sizeof(float) * ftpar->fn0,
		"ft2d")) == NULL )
     {
        Werrprintf("cannot allocate combine buffer");
        return(ERROR);
     }
  }
  if (ftpar->procstatus & HT_F1PROC)
  {
    if ( (addbuf = (float *) skyallocateWithId(sizeof(float) * ftpar->fn0 * typeofdata,
		"ft2d")) == NULL )
    {
       Werrprintf("cannot allocate add buffer");
       return(ERROR);
    }
  }

/*******************************************************
*  Allocate PHASE buffer (rotatebuf) to FN0 points     *
*  with 4 bytes per point.  Note that if hypercomplex  *
*  data are being created, NO PHASING is performed     *
*  until the end of the 2D-FT.                         *
*******************************************************/

  if ( (dof2phase) && (!ftpar->hypercomplex) )
  {
     if ( (rotatebuf = (float *) skyallocateWithId(sizeof(float) * dsfn0,
		"ft2d")) == NULL )
     {
        Werrprintf("cannot allocate phase rotation buffer");
	skyreleaseAllWithId("ft2d");
	return(ERROR);
     }
  }
  else
  {
     rotatebuf = NULL;
  }

/***************************************************
*  Calculate the level of zero-filling requested   *
*  and the number of points to zero-fill in order  *
*  to bring the time-domain data to the nearest    *
*  power of 2.  Determine if the FID data is to    *
*  be treated as sequential REAL of simultaneous   *
*  COMPLEX data.  Initialize the pointer to the    *
*  weighting function.                             *
***************************************************/

  realt2data = (ftpar->procstatus & REAL_t2);
  fpointmult = getfpmult(S_NP, fidhead->status & S_DDR);
  wtp.wtflag = ftpar->wtflag;
  wtfunc = NULL;

  if ( setlppar(&parLPinfo, S_NP, ftpar->procstatus, npadj/2, ftpar->fn0/2,
		   LPALLOC, "ft2d") )
  {
     disp_status("        ");
     skyreleaseAllWithId("ft2d");
     return(ERROR);
  }

  if (parLPinfo.sizeLP)
  {
     int	maxlpnp,
		nptmp;

     if (realt2data)
     {
        Werrprintf("LP analysis is not supported for real t2 data");
        disp_status("        ");
        skyreleaseAllWithId("ft2d");
        return(ERROR);
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
     if (ftpar->dspar.dsflag)
     {
       dsnpadj += ((maxlpnp-npadj)/ftpar->dspar.dsfactor);
       if (dsnpadj > dsfn0)  dsnpadj = dsfn0;
       ftpar->dspar.finalnp += ((maxlpnp-npadj)/ftpar->dspar.dsfactor);
     }
     else
       dsnpadj = maxlpnp;
     npadj = maxlpnp;
  }

  if (ftpar->dspar.dsflag)
  {
    zflevel = getzflevel(dsnpadj, dsfn0);
    zfnumber = getzfnumber(dsnpadj, dsfn0);
  }
  else
  {
    zflevel = getzflevel(npadj, ftpar->fn0);
    zfnumber = getzfnumber(npadj, ftpar->fn0);
  }

  if (ftpar->dspar.dsflag)
  {
    ftpar->dspar.buffer = (double *)allocateWithId(sizeof(double)*ftpar->fn0,"ft2d");
    if (!ftpar->dspar.buffer)
    {
      Werrprintf("Error allocating digital filtering buffer");
      ftpar->dspar.dsflag = FALSE;
      ftpar->dspar.fileflag = FALSE;
      return(ERROR);
    }

    ftpar->dspar.data =(float *)allocateWithId(sizeof(float)*ftpar->fn0,"ft2d");
    if (!ftpar->dspar.data)
    {
      Werrprintf("Error allocating digital filtering data buffer");
      ftpar->dspar.dsflag = FALSE;
      ftpar->dspar.fileflag = FALSE;
      return(ERROR);
    }
  }

  if (wtp.wtflag)
  {

/************************************************************
*  Allocate WEIGHT buffer (wtfunc) to NPX/2 points for      *
*  complex t2 data or to NPX points for real t2 data, with  *
*  4 bytes per point.                                       *
************************************************************/

     wtsize = npadj/2;
     if (realt2data)
        wtsize *= 2;

     if ((wtfunc = (float *) skyallocateWithId(sizeof(float) * wtsize, "ft2d")) == NULL)
     {
        Werrprintf("cannot allocate wtfunc buffer");
	skyreleaseAllWithId("ft2d");
	return(ERROR);
     }

/***************************************
*  Initialize the weighting parameter  *
*  names and the weighting vector.     *
***************************************/

     if (init_wt1(&wtp, S_NP))
     { 
        skyreleaseAllWithId("ft2d");
        return(ERROR);
     }

     readwtflag = FALSE;	/* for user-defined weighting functions */
     if (init_wt2(&wtp, wtfunc, wtsize, realt2data, S_NP,
			  fpointmult, readwtflag))
     { 
        skyreleaseAllWithId("ft2d");
        return(ERROR);
     }

     if (!wtp.wtflag)
     {
        skyrelease((char *) wtfunc);
        wtfunc = NULL;
     }
  }

/********************************************
*  Initialize the phase rotation function.  *
*  if F2 phasing is to be performed immed-  *
*  iately after the FT.                     *
********************************************/

  if (rotatebuf != NULL)
     phasefunc(rotatebuf, dsfn0/2, lpx, rpx);

/****************************************
*  Write block header into all blocks.  *
****************************************/

  if ((2*ftpar->nblocks < dsfn0) && (ftpar->nblocks > 1))
  {
     if ( (res = D_allocbuf(D_DATAFILE, 0, &outblock)) )
     {
        D_error(res);
        skyreleaseAllWithId("ft2d");
        ABORT;
     }

     blockstatus = (S_DATA|S_SPEC|S_FLOAT|S_COMPLEX);
     blockmode = 0;
     if (ftpar->hypercomplex)
        blockstatus |= S_HYPERCOMPLEX;

     setheader(&outblock, blockstatus, blockmode, ftpar->nblocks,
		   ftpar->hypercomplex);
     outblock.head->rpval = rpx;
     outblock.head->lpval = lpx;
     outblock.head->scale = 0;
     outblock.head->ctcount = 0;
     D_writeallblockheaders(D_DATAFILE);
  }

/*****************************************
*  Setup FID start, stop, and increment  *
*  parameters.  Calculate "lastfid" for  *
*  the specific case of F2 processing.   *
*****************************************/

  if (ftpar->D_dimname & S_NF)
  {
     nf_startfid = ftpar->arrayindex + (ftpar->arraydim * ftpar->t1_offset);
     stepfid = ftpar->cfstep;
     ftpar->cf = 1;
     startfid = ftpar->arrayindex + (ftpar->arraydim * ftpar->t1_offset);
     lastfid = ftpar->arrayindex + (ftpar->arraydim * ftpar->t1_offset) + 
			ftpar->nf;

     if (ftpar->ni1 > 1)
     {
        Werrprintf("The processing of 3D-(nf,ni2) data sets is not supported");
        skyreleaseAllWithId("ft2d");
        ABORT;
     }
  }
  else if ( (ftpar->combineflag) &&  (ftpar->arraydim == 2) &&
            (ftpar->t2_offset > 1) && (ftpar->ni1 == 1) )
  {
     stepfid = ftpar->t2_offset * ftpar->arraydim;

     startfid = ftpar->arrayindex * ftpar->arraydim;

     lastfid = startfid + ftpar->ni0*stepfid;
  }
  else
  { /* this handles (ni,ni2) double internal arrays for 3D */
     stepfid = ftpar->arraydim;
     if (ftpar->D_dimname & S_NI2)
        stepfid *= ftpar->ni1;

     startfid = ftpar->arrayindex + (ftpar->arraydim * ftpar->t1_offset) +
			(ftpar->t2_offset * ftpar->ni0 * ftpar->arraydim);

     lastfid = ( (ftpar->D_dimname & S_NI) ? (startfid +
			ftpar->ni0*ftpar->arraydim) : (startfid +
			ftpar->ni0*ftpar->ni1*ftpar->arraydim) );
  }

/**********************************************
*  Initialize Hadamard transform parameters.  *
**********************************************/

  if (ftpar->procstatus & HT_F1PROC)
  {
     if ( (res = htinit(ftpar,lastfid)) )
     {
        D_error(res); 
        skyreleaseAllWithId("ft2d");
        return(ERROR); 
     }
     htindex = 0;
     lastfid = ftpar->fn1/2;
  }


/********************************
*  Start FID block processing.  *
********************************/

  disp_status("FT (t2)");
  blocksdone = FALSE;
  cblock = 0;
  ftpar->displaynum = 1;	/* initialize FID display number */
  fid0 = startfid;

  while ( (cblock < ftpar->nblocks) && (!(blocksdone && fullft)) )
  {
     DPRINT1("block %d\n", cblock);

/************************************
*  Block cblock+nblocks is used in  *
*  D_DATAFILE to transpose data.    *
************************************/

     if ( (res = D_allocbuf(D_DATAFILE, cblock + ftpar->nblocks, &outblock)) )
     {
        D_error(res);
        skyreleaseAllWithId("ft2d");
        return(ERROR);
     }

     if (ftpar->dspar.dsflag)
     {
       outp = ftpar->dspar.data;
       finaloutp = (float *)outblock.data;
     }
     else
     {
       outp = (float *)outblock.data;   /* initialize pointer to FID data */
     }

     stopfid = startfid + ftpar->sperblock0*stepfid;

/**************************************************
*  Start filling at start of output data buffer.  *
**************************************************/

     for (fidcnt = startfid; fidcnt < stopfid; fidcnt += stepfid)
     {
        fidnum = ( (ftpar->D_dimname & S_NF) ? nf_startfid : fidcnt );
           
/**************************************
*  Stop processing if CANCEL COMMAND  *
*  has been requested.                *
**************************************/

        if (interuption)
        { 
           ft2d_cleanup("Aborting 2D processing", FALSE);
           D_trash(D_PHASFILE);
           D_trash(D_DATAFILE);
           ABORT;
        }

/**********************************************
*  If the requested FID is past the LASTFID,  *
*  produce a vector of zeros  until all the   *
*  data blocks are filled IF AND ONLY IF a    *
*  half-transformed 2D data set is desired.   *
**********************************************/
         
        if (fidnum >= lastfid)
        {
           if (fullft)
           {
              blocksdone = TRUE;
              fidcnt = stopfid;         /* "fidcnt" is correct here */
              ftpar->np1 = (lastfid - fid0) * 2 / stepfid;
           }
           else
           {
              if (ftpar->dspar.dsflag)
              {
                /* This assignment is necessary since at the
                 * bottom of the loop the outp and finaloutp
                 * pointers are exchanged
                 */
                outp = finaloutp;
                zerofill(outp, (dsfn0/2)*typeofdata);
              }
              else
              {
                zerofill(outp, (ftpar->fn0/2)*typeofdata);
              }
              if ( (fidnum == lastfid) || (fidnum == (lastfid + 1)) )
              {
                 blocksdone = TRUE;
                 disp_status("ZF      ");
  		 if ((ftpar->fn1 > DISPSIZE) || (ftpar->fn0 > DISPSIZE))
                    disp_index(0);
              }
           }
        }
        else if (ftpar->combineflag)
	{
           if (ftpar->dspar.dsflag)
             outp = finaloutp;
           if ( combinespectra(fidnum, outp, combinebuf, addbuf, wtfunc,
                   rotatebuf, pwr, &lastfid, typeofdata, npx, npadj,
		   zflevel, zfnumber, ftpar, fidhead, wtp.wtflag,
		   &parLPinfo, dsfn0, dsnpx, dsnpadj, dspwr, ftflag ) )
	   { 
              ft2d_cleanup("", FALSE);
	      return(ERROR);
	   }
           else if (fidnum >= lastfid)
           {
              blocksdone = TRUE;
              if (!fullft)
              {
                 disp_status("ZF      ");
  		 if ((ftpar->fn1 > DISPSIZE) || (ftpar->fn0 > DISPSIZE))
                    disp_index(0);
              }
           }
	}
        else
        {
           int zerofilled = 0;

           if (((ftpar->fn1 > DISPSIZE) || (ftpar->fn0 > DISPSIZE)) && 
					!(ftpar->displaynum & DISPMOD))
              disp_index(ftpar->displaynum);

           if (ftpar->procstatus & HT_F1PROC)
	   { 
              if ((htindex < htpar.fsize) && (fidnum == htpar.index[htindex]))
              {
                 if ( gethtfid(htindex+htpar.niofs, outp, ftpar,
                               fidhead, &lastfid, addbuf, 1, typeofdata) )
   	         { 
                    ft2d_cleanup("", FALSE);
	            return(ERROR);
	         }
                 htindex++;
              }
              else
              {
                 zerofilled = 1;
                 zerofill(outp, ftpar->fn0);
              }
	   }
           else if ( getfid(fidnum, outp, ftpar, fidhead, &lastfid) )
	   { 
              ft2d_cleanup("", FALSE);
	      return(ERROR);
	   }

/*************************************************
*  NOTE:  the first FID that is not found will   *
*         still be processed, though it contain  *
*         only zeroes!                           *
*************************************************/

           ftpar->displaynum += 1;	/* increment the FID display number */
           if (ftpar->D_dimname & S_NF)
              ftpar->cf += ftpar->cfstep;
					/* increment cf for NF processing */

	   if (lastfid == 0)
	   {
              ft2d_cleanup("", TRUE);
	      return(ERROR);
	   }

           if (!zerofilled)
           {
              if (ftpar->sspar.zfsflag || ftpar->sspar.lfsflag)
              {
                 if ( fidss(ftpar->sspar, outp, npx/2, lsfidx/2) )
                 {
                    ft2d_cleanup("time-domain solvent subtraction failed", FALSE);
                    return(ERROR);
                 }
              }

              if (parLPinfo.sizeLP)
              {
                 for (i = 0; i < parLPinfo.sizeLP; i++)
                 {
                    lpparams = *(parLPinfo.parLP + i);
                    if (lpz(fidnum, outp, (npx - lsfidx)/2, lpparams))
                    {
                       ft2d_cleanup("LP analysis failed", FALSE);
                       return(ERROR);
                    }
                 }
              }

              if (ftpar->dspar.dsflag)
              {
                if (ftpar->fn0-npadj > 0)
                  zerofill(outp + npadj, ftpar->fn0-npadj);
              }
              else
              {
                if (zfnumber > 0)
                  zerofill(outp + npadj, zfnumber);
              }

	      if (wtp.wtflag)
              {
                 weightfid(wtfunc, outp, npadj/2, realt2data, COMPLEX);
              }
              else if (fpointmult != 1.0)
              {
                 *outp *= fpointmult;
                 *(outp + 1) *= fpointmult;
              }

              if (ftpar->dspar.dsflag)
              {
	        register float *ptr1, *ptr2;
                register int index, count;

                if (downsamp(&(ftpar->dspar), outp, npadj/2, 0,
                           ftpar->fn0, ftpar->procstatus&REAL_t2))
                {
                  Werrprintf("digital filtering failed");
                  releaseAllWithId("ft2d");
                  disp_index(0);
                  disp_status("        ");
                  ABORT;
                }

                ptr1 = outp;
                ptr2 = finaloutp;
                count = dsnpadj;
                for (index=0;index<count;index++)
                  *ptr2++ = *ptr1++;
                outp = finaloutp;
                tmpi = ftpar->fn0; ftpar->fn0 = dsfn0; dsfn0 = tmpi;
                tmpi = npx; npx = dsnpx; dsnpx = tmpi;
                tmpi = npadj; npadj = dsnpadj; dsnpadj = tmpi;
                tmpi = pwr; pwr = dspwr; dspwr = tmpi;
                if (zfnumber > 0)
                   zerofill(outp + npadj, zfnumber);
              }

              if (ftflag)
              {
                 if ( ftpar->procstatus & (FT_F2PROC|LP_F2PROC) )
                 {
                    fft(outp, ftpar->fn0/2, pwr, zflevel, COMPLEX, COMPLEX, -1.0,
			   FTNORM, (npadj + zfnumber)/2);
	            if (realt2data)
                       postproc(outp, ftpar->fn0/2, COMPLEX);
                    if (ftpar->dspar.dsflag)
                      rotate2_center(outp, ftpar->fn0/2, ftpar->dspar.lp*
                           (ftpar->fn0/2)/(ftpar->fn0/2-1.0), (double)0.0);
                    if (fabs(ftpar->lpval) > MINDEGREE)
                      rotate2_center(outp, ftpar->fn0/2, ftpar->lpval*
                           (ftpar->fn0/2)/(ftpar->fn0/2-1.0), (double)0.0);
                    if (fabs(ftpar->daslp) > MINDEGREE)
                      rotate2_center(outp, ftpar->fn0/2, -ftpar->daslp*
			   (fidnum/2)*(ftpar->fn0/2)/(ftpar->fn0/2-1.0),
			   (double)0.0);
                    if (rotatebuf != NULL)
                       blockrotate2(outp, rotatebuf, 1, 1, 1, ftpar->fn0/2,
				   COMPLEX, 1, FALSE);
                 }
                 else if (ftpar->procstatus & MEM_F2PROC)
                 {
                    ft2d_cleanup("MEM processing:  not currently supported", FALSE);
                    return(ERROR);
                 }
              }
              else if (ftpar->fn0 > npadj)
              {
                zerofill(outp + npadj, ftpar->fn0 - npadj);
              }
             if (ftpar->dspar.dsflag)
             {
               tmpi = ftpar->fn0; ftpar->fn0 = dsfn0; dsfn0 = tmpi;
               tmpi = npx; npx = dsnpx; dsnpx = tmpi;
               tmpi = npadj; npadj = dsnpadj; dsnpadj = tmpi;
               tmpi = pwr; pwr = dspwr; dspwr = tmpi;
             }
           }
        }

        if (ftpar->dspar.dsflag)
        {
          outp += (dsfn0/2) * typeofdata;
          finaloutp = outp;
          outp = ftpar->dspar.data;
        }
        else
          outp += (ftpar->fn0/2) * typeofdata;
     }

     blockmode = ftpar->D_dsplymode;
     blockstatus = (S_DATA|S_SPEC|S_FLOAT|S_COMPLEX|ftpar->D_cmplx);
     if (ftpar->D_cmplx == 0)
        switch (ftpar->D_dimname)
        {
           case (S_NF|S_NP)  : blockstatus |= NF_CMPLX;
                               break;
           case (S_NI|S_NP)  : blockstatus |= NI_CMPLX;
                               break;
           case (S_NI2|S_NP) : blockstatus |= NI2_CMPLX;
                               break;
           default           : blockstatus |= NI_CMPLX;
                               break;
        }
     if (ftpar->hypercomplex)
        blockstatus |= S_HYPERCOMPLEX;

     setheader(&outblock, blockstatus, blockmode, cblock + ftpar->nblocks,
		   ftpar->hypercomplex);
     outblock.head->scale = 0;
     outblock.head->ctcount = 0;

     if (rotatebuf != NULL)
     {
        outblock.head->rpval = rpx;
        outblock.head->lpval = lpx;
     }

     if ( (res = D_markupdated(D_DATAFILE, cblock + ftpar->nblocks)) )
     {
        D_error(res);
        ft2d_cleanup("", FALSE);
        return(ERROR);
     }

     if ( (res = D_release(D_DATAFILE, cblock + ftpar->nblocks)) )
     {
        D_error(res);
        ft2d_cleanup("", FALSE);
        return(ERROR);
     }

     cblock++;
     startfid += ftpar->sperblock0*stepfid;
     if (startfid == lastfid)
        blocksdone = TRUE;
  } /* end while */

  if (fullft)
  {
     skyreleaseAllWithId("ft2d");
     if ((ftpar->fn1 > DISPSIZE) || (ftpar->fn0 > DISPSIZE))
        disp_index(0);
     disp_status("       ");
  }
  else
  {
     ft2d_cleanup("", FALSE);
  }

  D_close(D_USERFILE);
  return(COMPLETE);
}
 

/*----------------------------------
|				   |
|           secondft()/1	   |
|				   |
+---------------------------------*/
static int secondft(ftparInfo *ftpar, int ftflag)
{
  char		lsfidname[10],
		phfidname[10],
		lsfrqname[10];
  int		npx,
		npadj,
		pwr,
		cblock,
		res,
		i,
		fidnum,
		zflevel,
		zfnumber,
		wtsize,
		realt1data,
		quad1=0,
		datamult=0,
		nf1readblocks,	/* number of actual non-zero blocks */
		datatype=0,	/* complex or hypercomplex */
		dmode=0,	/* data mode */
		dstatus=0,	/* data status */
		seltoggle = 1,
		*f2regpntr= NULL,
		*svf2regpntr;
  float		*outp,
		*lpbuffer= NULL,
		*ssbuffer= NULL,
		*tmpdest,
		*tmpsrc,
		*wtfunc,
		*rotatebuf;	/* phase rotation vector */
  double	rpx,
		lpx;
  void		weightfid(),
		zeroimag(),
		driftcorrect_fid();
  dfilehead	tmpdatahead;
  dpointers	outblock;
  lpstruct	parLPinfo;
  wtPar		wtp;
  extern void	getcmplx_intfgm(),
		putcmplx_intfgm(),
		set_calcfidss();

  disp_status("FT (t1)");
  if ((ftpar->fn1 > DISPSIZE) || (ftpar->fn0 > DISPSIZE))
     disp_index(0);

/****************************************
*  Check for consistency of processing  *
*  dimensions.                          *
****************************************/

  if ( (res = D_gethead(D_DATAFILE, &tmpdatahead)) )
  {
     D_error(res);
     disp_status("         ");
     return(ERROR);
  }

  if ( (tmpdatahead.status & (S_NP|S_NF|S_NI|S_NI2)) != ftpar->D_dimname )
  {
     Werrprintf("Data are inconsistent with requested processing array");
     if (reset_dataheader())
        Werrprintf("Unable to zero DATA file header");
     return(ERROR);
  }

/**************************************************
*  Get "lsfid1,phfid1" or "lsfid2,phfid2" values  *
*  from parameter set.                            *
**************************************************/

  if ( (ftpar->D_dimname & S_NF) || (ftpar->D_dimname & S_NI) )
  {
     strcpy(lsfidname, "lsfid1");
     strcpy(phfidname, "phfid1");
     strcpy(lsfrqname, "lsfrq1");
  }
  else
  {
     strcpy(lsfidname, "lsfid2");
     strcpy(phfidname, "phfid2");
     strcpy(lsfrqname, "lsfrq2");
  }

  ls_ph_fid(lsfidname, &(ftpar->lsfid1), phfidname, &(ftpar->phfid1),
		lsfrqname, &(ftpar->lsfrq1));

  if (!ftpar->ptype)
     ftpar->lsfrq1 *= (-1);	/* necessary because negation of the
				   imaginary part of the t1 interferogram
				   comes after time-domain frequency
				   shifting with lsfrq1.
				*/
     
/********************************************************
*  np1  -  number of total points in the interferogram  *
*  npx  -  used points in the interferogram             *
*  fn1  -  number of total points after the F1 FT       *
*                                                       *
*  Adjust "npx".                                        *
********************************************************/

  if (ftpar->lsfid1 < 0)
  {
     npx = ( (ftpar->fn1 < (ftpar->np1 - ftpar->lsfid1)) ?
		(ftpar->fn1 + ftpar->lsfid1) : ftpar->np1 );

     if (npx < 2)
     {
        Werrprintf("%s is too large in magnitude", lsfidname);
        return(ERROR);
     }
  }
  else
  {
     npx = ( (ftpar->fn1 < ftpar->np1) ? ftpar->fn1 : ftpar->np1 );

     if (ftpar->lsfid1 >= npx)
     {
        if (ftpar->lsfid1)
           Werrprintf("%s is too large in magnitude", lsfidname);
        else
           Werrprintf("too few increments for second ft");
        return(ERROR);
     }
  }

  npadj = npx - ftpar->lsfid1;		/* adjusted number of points	*/
  pwr = fnpower(ftpar->fn1);
  nf1readblocks = getrdblocks(ftpar->nblocks, getzflevel(npx, ftpar->fn1));

/**********************************************
*  Initialize first-point multiplier for the  *
*  interferogram.                             *
**********************************************/

  fpointmult = ( (ftpar->D_dimname & (S_NI | S_NF)) ? getfpmult(S_NI,0) :
			getfpmult(S_NI2,0) );

/*********************************************************
*  Allocate WTFUNC buffer for fn1/2 points with complex  *
*  t1 data or for fn1 points with real t1 data, with 4   *
*  bytes per point.                                      *
*********************************************************/

  realt1data = (ftpar->procstatus & REAL_t1);

  if ( setlppar(&parLPinfo, (ftpar->D_dimname & (S_NF|S_NI|S_NI2)),
		   ftpar->procstatus, npadj/2, ftpar->fn1/2,
		   LPALLOC, "ft2d") )
  {
     releaseAllWithId("ft2d");
     return(ERROR);
  }

  if (parLPinfo.sizeLP)
  {
     int	maxlpnp,
		nptmp;

     if (realt1data)
     {
        Werrprintf("LP analysis is not supported for real t1 data");
        releaseAllWithId("ft2d");
        return(ERROR);
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

     npadj = maxlpnp;

     if (ftpar->hypercomplex)
     {
        if ( (lpbuffer = (float *) skyallocateWithId(ftpar->fn1 *
		  sizeof(float), "ft2d")) == NULL )
        {
           Werrprintf("cannot allocate memory for LP buffer");
           releaseAllWithId("ft2d");
           return(ERROR);
        }
     }
  }

  if (ftpar->sspar1.zfsflag || ftpar->sspar1.lfsflag)
  {
     set_calcfidss(TRUE);	/* must be re-initialized */
     if (ftpar->hypercomplex)
     {
        if ( (ssbuffer = (float *) skyallocateWithId(ftpar->fn1 *
		  sizeof(float), "ft2d")) == NULL )
        {
           Werrprintf("cannot allocate memory for SS buffer");
           releaseAllWithId("ft2d");
           return(ERROR);
        }
     }
  }

  zflevel = getzflevel((npadj < 16) ? 16 : npadj, ftpar->fn1);
  zfnumber = getzfnumber(npadj, ftpar->fn1);
  if (zfnumber + npadj < 16)
     zfnumber = 16 - npadj;

  rotatebuf = NULL;
  wtp.wtflag = ftpar->wtflag;
  wtfunc = NULL;
  wtsize = npadj/2;
  if (realt1data)
     wtsize *= 2;

  if (wtp.wtflag)
  {
     if ( (wtfunc = (float *) skyallocateWithId(sizeof(float) * wtsize, "ft2d"))
		== NULL )
     {
        Werrprintf("cannot allocate wtfunc buffer");
        skyreleaseAllWithId("ft2d");
        return(ERROR);
     }
  }

  if (wtp.wtflag)
  {
     if (init_wt1(&wtp, ftpar->D_dimname))
     {
        skyreleaseAllWithId("ft2d");
        return(ERROR);
     }

     readwtflag = FALSE;	/* for user-defined weighting functions */
     if (init_wt2(&wtp, wtfunc, wtsize, realt1data, ftpar->D_dimname,
			  fpointmult, readwtflag))
     {
        skyreleaseAllWithId("ft2d");
        return(ERROR);
     }

     if (!wtp.wtflag)
     {
        skyrelease((char *) wtfunc);
        wtfunc = NULL;
     }
  }

/******************************************
*  Setup parameters for F1 processing of  *
*  selected F2 regions.                   *
******************************************/  

   if (ftpar->f2select)
   {
      int	sections,
		fntmp;
      double	swtmp,
		value;
      vInfo	info;


      sections = ( (P_getVarInfo(CURRENT, "lifrq", &info)) ? 1 : info.size);
      sections = (sections + 1)/2;
      if (sections < 1)
         ftpar->f2select = FALSE;

      if (ftpar->f2select)
      {
         if (!P_getreal(CURRENT, "sw", &swtmp, 1))
         {
            if (!P_getreal(CURRENT, "lifrq", &value, 1))
            {
               if (value > swtmp)
                  ftpar->f2select = FALSE;
            }
            else
            {
               ftpar->f2select = FALSE;
            }
         }
         else
         {
            ftpar->f2select = FALSE;
         }

         if (P_getreal(CURRENT, "fn", &value, 1))
            ftpar->f2select = FALSE;
      }

      if (ftpar->f2select)
      {
         fntmp = (int) (value + 0.5);
         i = 32;
         while (i < fntmp)
            i *= 2;
         fntmp = i;

         f2regpntr = (int *) allocateWithId(2 * sizeof(int) * (sections + 1),
				"ft2d");
         if (f2regpntr == NULL)
         {
            ftpar->f2select = FALSE;
         }
         else
         {
            svf2regpntr = f2regpntr;

            for (i = 1; i <= sections; i++)
            {
               if (P_getreal(CURRENT, "lifrq", &value, i*2 - 1))
               {
                  *f2regpntr++ = fntmp/2;
                  *f2regpntr++ = fntmp/2;
                  i = sections;
               }
               else
               {
                  *f2regpntr++ = datapoint(value, swtmp, fntmp/2);
                  if (P_getreal(CURRENT, "lifrq", &value, i*2))
                  {
                     *f2regpntr++ = fntmp/2;
                     i = sections;
                  }
                  else
                  {  
                     *f2regpntr++ = datapoint(value, swtmp, fntmp/2);
                  }
               }
            }

            *f2regpntr++ = fntmp/2;
            *f2regpntr++ = fntmp/2;
            f2regpntr = svf2regpntr;
         }

         seltoggle = -1;
      }
   }

/**********************************
*  Start processing data blocks.  *
**********************************/

  for (cblock = 0; cblock < ftpar->nblocks; cblock++)
  {
     DPRINT1("block %d\n",cblock);
     if ( (res = D_getbuf(D_DATAFILE, nf1readblocks, cblock, &outblock)) )
     {
        D_error(res); 
        skyreleaseAllWithId("ft2d");
        return(ERROR); 
     }

/*******************************************
*  Set up flags for processing block data  *
*  on the first block.                     *
*******************************************/

     if (cblock == 0)
     {
        dmode = (int) (outblock.head->mode);
        dstatus = (int) (outblock.head->status);
        if (ftpar->D_cmplx == 0)
           switch (ftpar->D_dimname)
           {
              case (S_NF|S_NP)  : dstatus &= (~NF_CMPLX);
                                  break;
              case (S_NI|S_NP)  : dstatus &= (~NI_CMPLX);
                                  break;
              case (S_NI2|S_NP) : dstatus &= (~NI2_CMPLX);
                                  break;
              default           : dstatus &= (~NI_CMPLX);
                                  break;
           }

/***********************************************
*  Check existing data for compatibility with  *
*  type of processing ("pmode"), and type of   *
*  display.                                    *
***********************************************/

        if ( (ftpar->hypercomplex) && ((~dstatus) & NP_CMPLX) )
        {
           ft2d_cleanup("Data are inconsistent with full processing mode",
			 TRUE);
           return(ERROR);
        }
        else if ( (!ftpar->hypercomplex) && (dstatus & NP_CMPLX) )
        {
           ft2d_cleanup("Data are inconsistent with full processing mode",
			 TRUE);
           return(ERROR);
        }
        else if ( ( ((ftpar->D_dsplymode & NP_PHMODE) && ((~dmode) & NP_PHMODE))
		|| ((ftpar->D_dsplymode & NP_PAMODE) && ((~dmode) & NP_PAMODE)))
			&& ((~ftpar->D_cmplx) & NP_CMPLX) )
        {
           ft2d_cleanup("Data can no longer be phased along F2", TRUE);
           return(ERROR);
        }

/*************************************
*  Calculate "datatype" and "quad1"  *
*  status for the data.              *
*************************************/

        datatype = ( (ftpar->hypercomplex) ? HYPERCOMPLEX : COMPLEX );
        datamult = datatype/2;
        quad1 = (!( (ftpar->D_cmplx & NP_CMPLX) ||
			(ftpar->D_cmplx & (NF_CMPLX|NI_CMPLX|NI2_CMPLX)) ));

/********************************************
*  Set up phase vector on the first block.  *
********************************************/

        if ( ((ftpar->dof1phase) || (ftpar->dof1phaseangle)) && quad1 )
        {
           if ( findphases(&rpx, &lpx, ftpar->D_dimname) )
           {
              ft2d_cleanup("Cannot find F1 phasing constants", FALSE);
              return(ERROR);
           }

           if ((fabs(rpx) > MINDEGREE) || (fabs(lpx) > MINDEGREE))
           {
              if ((rotatebuf = (float *) skyallocateWithId(sizeof(float) * ftpar->fn1,
			"ft2d")) == NULL)
              {
                 ft2d_cleanup("Cannot allocate phase rotation buffer", FALSE);
                 return(ERROR);
              }
              phasefunc(rotatebuf, ftpar->fn1/2, lpx, rpx);
           }
           else
           {
              ftpar->dof1phase = FALSE;
              ftpar->dof1phaseangle = FALSE;
           }
        }
        else
        {
           lpx = 0.0;
           rpx = 0.0;
        }
     }
        
/**************************************************
*  Begin working at start of output data buffer.  *
**************************************************/

     outp = (float *)outblock.data;
     for (fidnum = cblock*ftpar->sperblock1;
		fidnum < (cblock+1)*ftpar->sperblock1; fidnum++)
     {
        if (interuption)
        {
           ft2d_cleanup("", FALSE);
	   D_trash(D_PHASFILE);
	   D_trash(D_DATAFILE);
           return(ERROR);
        }
        long_event();

        if ((ftpar->fn1 > DISPSIZE) && !((fidnum + 1) & DISPMOD))
           disp_index(fidnum + 1);

        if (ftpar->f2select)
        {
           if ( fidnum > (*f2regpntr) )
           {
              f2regpntr += 1;
              seltoggle *= -1;           
           }
        }

/**********************************************
*  Left shift the data "lsfid1" real points.  *
*  Phase the FID data using "phfidx" degrees  *
*  of zero-order rotation.                    *
**********************************************/

        if (seltoggle == 1)
        { /* process this hypercomplex or complex t1 interferogram */
           if (ftpar->lsfid1 > 0)
           {
              tmpdest = outp;
              tmpsrc = outp + ftpar->lsfid1*datamult;
              for (i = 0; i < (npx - ftpar->lsfid1)*datamult; i++)
                 *tmpdest++ = *tmpsrc++;
           }
           else if (ftpar->lsfid1 < 0)
           {
              tmpsrc = outp + ftpar->np1*datamult;
              tmpdest = outp + (npx - ftpar->lsfid1)*datamult;
              for (i = 0; i < ftpar->np1*datamult; i++)
                 *(--tmpdest) = *(--tmpsrc);

              for (i = 0; i < (-1)*ftpar->lsfid1*datamult; i++)
                 *(--tmpdest) = 0.0;
           }

           if ( (fabs(ftpar->phfid1) > MINDEGREE) ||
	        (fabs(ftpar->lsfrq1) > 1e-20) )
           {
              rotate_fid(outp, ftpar->phfid1, ftpar->lsfrq1,
		   (npx - ftpar->lsfid1), ( ftpar->hypercomplex ?
			HYPERCOMPLEX : COMPLEX ));
           }

/********************************************
*  Do subtraction of low frequency signal.  *
********************************************/

           if (ftpar->sspar1.zfsflag || ftpar->sspar1.lfsflag)
           {
              if (ftpar->hypercomplex)
              {
                 getcmplx_intfgm(outp, ssbuffer, ftpar->fn1/2, COMPLEX);
                 if ( fidss(ftpar->sspar1, ssbuffer, npx/2, ftpar->lsfid1/2) )
                 {  
                    Werrprintf("low-frequency signal subtraction failed");
                    return(ERROR);
                 }

                 putcmplx_intfgm(outp, ssbuffer, ftpar->fn1/2, COMPLEX);
                 getcmplx_intfgm(outp + 1, ssbuffer, ftpar->fn1/2, COMPLEX);

                 if ( fidss(ftpar->sspar1, ssbuffer, npx/2, ftpar->lsfid1/2) )
                 {  
                    Werrprintf("low-frequency signal subtraction failed");
                    return(ERROR);
                 }

                 putcmplx_intfgm(outp + 1, ssbuffer, ftpar->fn1/2, COMPLEX);
              }
              else
              {
                 if ( fidss(ftpar->sspar1, outp, npx/2, ftpar->lsfid1/2) )
                 {
                    Werrprintf("low-frequency signal subtraction failed");
                    return(ERROR);
                 }
              }
           }

/************************************
*  Do DC correction at this stage.  *
************************************/

           if (ftpar->t1dc)
              driftcorrect_fid(outp, npx/2, ftpar->lsfid1/2, datatype);

/***************************************
*  Do LP data extension at this point  *
***************************************/
 
           if (parLPinfo.sizeLP)
           {
              if (ftpar->hypercomplex)
              {
                 getcmplx_intfgm(outp, lpbuffer, ftpar->fn1/2, COMPLEX);

                 for (i = 0; i < parLPinfo.sizeLP; i++)
                 {
                    lpparams = *(parLPinfo.parLP + i);
                    if (lpz(fidnum, lpbuffer, (npx - ftpar->lsfid1)/2,
				lpparams))
                    {
                       ft2d_cleanup("LP analysis failed", FALSE);
                       return(ERROR);
                    }
                 }

                 putcmplx_intfgm(outp, lpbuffer, ftpar->fn1/2, COMPLEX);
                 getcmplx_intfgm(outp + 1, lpbuffer, ftpar->fn1/2, COMPLEX);

                 for (i = 0; i < parLPinfo.sizeLP; i++) 
                 {
                    lpparams = *(parLPinfo.parLP + i);
                    if (lpz(fidnum, lpbuffer, (npx - ftpar->lsfid1)/2,
				lpparams)) 
                    {
                       ft2d_cleanup("LP analysis failed", FALSE); 
                       return(ERROR); 
                    }
                 }

                 putcmplx_intfgm(outp + 1, lpbuffer, ftpar->fn1/2, COMPLEX);
              }
              else
              {
                 for (i = 0; i < parLPinfo.sizeLP; i++)  
                 { 
                    lpparams = *(parLPinfo.parLP + i);
                    if (lpz(fidnum, outp, (npx - ftpar->lsfid1)/2, lpparams))
                    {
                       ft2d_cleanup("LP analysis failed", FALSE);
                       return(ERROR);
                    }
                 }
              }
           }

/****************************************
*  Zerofill to the nearest power of 2.  *
****************************************/

           if (zfnumber > 0)
              zerofill(outp + npadj*datamult, zfnumber*datamult);

/********************************************
*  Weight the FID data and apply the Ernst  *
*  correction to the first point.           *
********************************************/

	   if (wtp.wtflag)
           {
              weightfid(wtfunc, outp, npadj/2, realt1data, datatype);
           }
           else if (fpointmult != 1.0)
           {
              *outp *= fpointmult;
              *(outp + 1) *= fpointmult;
              if (ftpar->hypercomplex)
              {
                 *(outp + 2) *= fpointmult;
                 *(outp + 3) *= fpointmult;
              }
           }

           if (!ftpar->ptype)
              negateimaginary(outp, npadj/2, datatype);

           if ( ftpar->procstatus & (FT_F1PROC|LP_F1PROC) )
           {
              if (ftflag)
              {
                 if (realt1data)
                    preproc(outp, npadj/2, datatype);
                 fft(outp, ftpar->fn1/2, pwr, zflevel, datatype, datatype,
				-1.0, 1.0, npadj/2);
                 if (realt1data)
                    postproc(outp, ftpar->fn1/2, datatype);
              }

              if (quad1)
              {
                 if (ftpar->dof1phase)
                 {
                    blockphase2(outp, outp, rotatebuf, 1, 0, ftpar->fn1/2,
				   1, COMPLEX, 1, TRUE, TRUE);
                 }
                 else if (ftpar->dof1absval) 
                 {
                    absval2(outp, outp, ftpar->fn1/2, COMPLEX, 1, COMPLEX,
				   TRUE);
                 }
                 else if (ftpar->dof1power)
                 {
                    pwrval2(outp, outp, ftpar->fn1/2, COMPLEX, 1, COMPLEX,
				   TRUE);
                 }
                 else if (ftpar->dof1phaseangle)
                 {
                    blockphaseangle2(outp, outp, rotatebuf, 1, 0, ftpar->fn1/2,
				   1, COMPLEX, 1, TRUE, TRUE);
                 }
                 else
                 {
                    zeroimag(outp, ftpar->fn1, FALSE);
                 }
              }
           }
           else if (ftpar->procstatus & HT_F1PROC)
           {
              if (quad1)
              {
                 if (ftpar->dof1phase)
                 {
                    blockphase2(outp, outp, rotatebuf, 1, 0, ftpar->fn1/2,
				   1, COMPLEX, 1, TRUE, TRUE);
                 }
                 else if (ftpar->dof1absval) 
                 {
                    absval2(outp, outp, ftpar->fn1/2, COMPLEX, 1, COMPLEX,
				   TRUE);
                 }
                 else if (ftpar->dof1power)
                 {
                    pwrval2(outp, outp, ftpar->fn1/2, COMPLEX, 1, COMPLEX,
				   TRUE);
                 }
                 else if (ftpar->dof1phaseangle)
                 {
                    blockphaseangle2(outp, outp, rotatebuf, 1, 0, ftpar->fn1/2,
				   1, COMPLEX, 1, TRUE, TRUE);
                 }
                 else
                 {
                    zeroimag(outp, ftpar->fn1, FALSE);
                 }
              }
           }
           else if (ftpar->procstatus & MEM_F1PROC)
           {
              ft2d_cleanup("MEM processing:  not currently supported", FALSE);
              return(ERROR);
           }
        }
        else
        { /* "zero" hypercomplex or complex t1 interferogram */
           datafill(outp, ftpar->fn1*datamult, SMALL_VALUE);
        }

        outp += ftpar->fn1*datamult;
     }

/******************************************
*  Set and write header information into  *
*  the appropriate block.                 *
******************************************/

     dstatus |= ftpar->D_cmplx;
     dmode |= ftpar->D_dsplymode;
     setheader(&outblock, dstatus, dmode, cblock, ftpar->hypercomplex);

     if ((ftpar->dof1phase || ftpar->dof1phaseangle) && quad1)
     {
        outblock.head->rpval = rpx;
        outblock.head->lpval = lpx;
     }

     if ( (res = D_markupdated(D_DATAFILE, cblock)) )
     {
        D_error(res);
        ft2d_cleanup("", FALSE);
        return(ERROR);
     }

     if ( (res = D_release(D_DATAFILE, cblock)) )
     {
        D_error(res);
        ft2d_cleanup("", FALSE);
        return(ERROR);
     }
  }

/*********************************************
*  Compress the data file down to real data  *
*  if pmode = '' has been used.              *
*********************************************/

   if (quad1)
   {
      char	filepath[MAXPATHL];

      disp_status("REDUCE  ");
      if ((ftpar->fn1 > DISPSIZE) || (ftpar->fn0 > DISPSIZE))
         disp_index(0);
      D_close(D_DATAFILE);

      if ( (res = D_compress()) )
      {
         D_error(res);
         ft2d_cleanup("", FALSE);
         return(ERROR);
      }

      if ( (res = D_getfilepath(D_DATAFILE, filepath, curexpdir)) )
      {
         D_error(res);
         ft2d_cleanup("", FALSE);
         return(ERROR);
      }

      if ( (res = D_open(D_DATAFILE, filepath, &tmpdatahead)) )
      {
         D_error(res);
         ft2d_cleanup("", FALSE);
         return(ERROR);
      }
   }

/********************************************
*  Clean up and return to main FT routine.  *
********************************************/

  ft2d_cleanup("", FALSE);
  return(COMPLETE);
}


/*---------------------------------------
|					|
|	    setprocstatus()/1		|
|					|
|  This function determines the type	|
|  of data processing to be used.	|
|  Currently, only FT's are supported   |
|  on real or complex time-domain 	|
|  data.				|
|					|
+--------------------------------------*/
int setprocstatus(int ft_dimname, int full2d)
{
   char	proctype[MAXSTR],
	proc1type[MAXSTR];
   int	statusword = 0;


   proctype[0] = '\0';
   proc1type[0] = '\0';
   if (ft_dimname & S_NP)
   {
      if (P_getstring(CURRENT, "proc", proctype, 1, 10))
         strcpy(proctype, "ft");
   }

   if (ft_dimname & (S_NI|S_NF))
   {
      if (P_getstring(CURRENT, "proc1", proc1type, 1, 10))
         strcpy(proc1type, "ft");
   }
   else if (ft_dimname & S_NI2)
   {
      if (P_getstring(CURRENT, "proc2", proc1type, 1, 10))
         strcpy(proc1type, "ft");
   }

/***************************
*  Do the first dimension  *
***************************/

   if (strcmp(proctype, "ft") == 0)
   {
      statusword |= (CMPLX_t2|FT_F2PROC);
   }
   else if (strcmp(proctype, "rft") == 0)
   {
      statusword |= (REAL_t2|FT_F2PROC);
   }
   else if (strcmp(proctype, "lp") == 0)
   {
      statusword |= (CMPLX_t2|LP_F2PROC);
   }
   else if (strcmp(proctype, "rlp") == 0)
   {
      statusword |= (REAL_t2|LP_F2PROC);
   }
   else if (strcmp(proctype, "mem") == 0)
   {
      statusword |= (CMPLX_t2|MEM_F2PROC);
   }
   else if (strcmp(proctype, "rmem") == 0)
   {
      statusword |= (REAL_t2|MEM_F2PROC);
   }
   else
   {
      statusword |= (CMPLX_t2|FT_F2PROC);
   }

/****************************
*  Do the second dimension  *
****************************/

   if (strcmp(proc1type, "ft") == 0)
   {
      statusword |= (CMPLX_t1|FT_F1PROC);
   }
   else if (strcmp(proc1type, "rft") == 0)
   {
      statusword |= (REAL_t1|FT_F1PROC);
   }
   else if (strcmp(proc1type, "lp") == 0)
   {
      statusword |= (CMPLX_t1|LP_F1PROC);
   }
   else if (strcmp(proc1type, "rlp") == 0)
   {
      statusword |= (REAL_t1|LP_F1PROC);
   }
   else if (strcmp(proc1type, "mem") == 0)
   {
      statusword |= (CMPLX_t1|MEM_F1PROC);
   }
   else if (strcmp(proc1type, "rmem") == 0)
   {
      statusword |= (REAL_t1|MEM_F1PROC);
   }
   else if ((strcmp(proc1type, "ht") == 0) && full2d)
   {
      statusword |= (CMPLX_t1|HT_F1PROC);
   }
   else if ((strcmp(proc1type, "rht") == 0) && full2d)
   {
      statusword |= (REAL_t1|HT_F1PROC);
   }
   else
   {
      statusword |= (CMPLX_t1|FT_F1PROC);
   }

   return(statusword);
}


/*---------------------------------------
|					|
|		 ft2d() 		|
|					|
|     Entry point for ft2d program.	|
|					|
+--------------------------------------*/
int ft2d(int argc, char *argv[], int retc, char *retv[])
{
  int		chkdatastatus,
		datastatus,
                arg_no,
                do_ds,
                ftflag,
		full2dft;
  ftparInfo	ftpar;
  dfilehead	fidhead,
		datahead,
		phasehead;


  Wturnoff_buttons();
  acqflag = FALSE;
  ftpar.zeroflag = FALSE; 
  full2dft = ((argv[0][2] == '2') || (argv[0][3] == '2'));
  datastatus = S_DATA|S_SPEC|S_FLOAT;
  arg_no = 1;
  do_ds = ftflag = TRUE;
  while (argc > arg_no)
  {
     if (strcmp(argv[arg_no], "nods") == 0)
     {
        do_ds = FALSE;
     }
     else if (strcmp(argv[arg_no], "noft") == 0)
     {
        ftflag = FALSE;
     }
     arg_no++;
  }


  if (full2dft)
  {
     chkdatastatus = TRUE;
     datastatus |= S_SECND;
  }
  else
  {
     chkdatastatus = FALSE;
  }

  if (i_ft(argc, argv, datastatus, chkdatastatus, 1, &ftpar, &fidhead,
		&datahead, &phasehead))
  {
     disp_status("        ");
     ABORT;
  }

  ftpar.procstatus = setprocstatus(ftpar.D_dimname,full2dft);
  if ((!full2dft) || (ftpar.procstatus & HT_F1PROC))
     ftpar.dofirstft = TRUE;

  disp_current_seq();
  if (ftpar.dofirstft)
  {
     if ( firstft(full2dft, &ftpar, &fidhead, ftflag) )
     {
        disp_status("        ");
        ABORT;
     }
  }

  if (full2dft)
  {
     ftpar.wtflag = (argv[0][0] == 'w'); /* must reset weighting flag */
     if ( secondft(&ftpar, ftflag) )
     {
        disp_status("        ");
        ABORT;
     }
  }

  if (do_ds == FALSE)
  {
     register int i, npt;
     extern float *gettrace();
     extern int	fn1;

     init2d(1,0);
     npt = fn1/2;
     for (i=0; i<npt; i++)
       gettrace(i,0);
  }
  else if (!Bnmr)
  {
     releasevarlist();
     appendvarlist("dconi");
#ifdef SIS
     appendvarlist("gray"); /* default to linear gray scale if imager */
     appendvarlist("linear");
#endif 
     Wsetgraphicsdisplay("dconi");  /* activate the dconi program */
     start_from_ft2d = 1;
  }
  set_vnmrj_ft_params( 2, argc, argv );

  disp_status("        ");
  if ((ftpar.fn1 > DISPSIZE) || (ftpar.fn0 > DISPSIZE))
     disp_index(0);

  RETURN;
}

#define F1NORM 2500000.0
int readf1(int argc, char *argv[], int retc, char *retv[])
{
   int		r;
   dpointers    inblock;
   dfilehead	fidhead,
		datahead;
   dblockhead   fidblockheader;
   char		filepath[MAXPATH];
   char		fidpath[MAXPATH];
   register int npt;
   int outRe;
   int outIm = 0;
   int *bptr;
   register int *optr;
   typedef union 
   {
      float  f;
      int    i;
   } unionFI;
   unionFI tmp;



   if ( (r = D_getfilepath(D_DATAFILE, filepath, curexpdir)) )
   {
      D_error(r);
      ABORT;
   }

   r = D_gethead(D_DATAFILE, &datahead);
   if (r)
      r = D_open(D_DATAFILE, filepath, &datahead);   /* open the data file */

   if (r == COMPLETE)
   {
      int cblock;

      if ( ((datahead.status & S_SECND) != S_SECND) &&
           ((datahead.status & S_TRANSF) == S_TRANSF) &&
           ((datahead.status & S_SPEC) == S_SPEC) )
      {
         sprintf(fidpath,"%s/fidf1r",curexpdir);
         outRe = open(fidpath, O_RDONLY);
         if (outRe == -1)
         {
            Winfoprintf("Cannot open %s", fidpath);
            if (retc > 0)
            {
               retv[0] = intString( 0 );
               RETURN;
            }
            ABORT;
         }
         npt = datahead.np / datahead.nbheaders;
         read(outRe, & fidhead, sizeof(dfilehead) );
         DATAFILEHEADER_CONVERT_HTON( &fidhead );
         if ( (fidhead.nblocks != datahead.nblocks * datahead.ntraces) ||
              (fidhead.np != npt) ||
              (fidhead.ebytes != datahead.ebytes) )
         {
            Winfoprintf("Data mismatch in %s", fidpath);
            if (retc > 0)
            {
               retv[0] = intString( 0 );
               RETURN;
            }
            ABORT;
         }
         if (datahead.nbheaders == 2)
         {
            fidpath[strlen(fidpath)-1] = 'i';
            outIm = open(fidpath, O_RDONLY);
            if (outIm == -1)
            {
               Winfoprintf("Cannot open %s", fidpath);
               if (retc > 0)
               {
                  retv[0] = intString( 0 );
                  RETURN;
               }
               ABORT;
            }
            read(outIm, & fidhead, sizeof(dfilehead) );
            DATAFILEHEADER_CONVERT_HTON( &fidhead );
            if ( (fidhead.nblocks != datahead.nblocks * datahead.ntraces) ||
                 (fidhead.np != npt) ||
                 (fidhead.ebytes != datahead.ebytes) )
            {
               Winfoprintf("Data mismatch in %s", fidpath);
               if (retc > 0)
               {
                  retv[0] = intString( 0 );
                  RETURN;
               }
               ABORT;
            }
         }
         bptr =(int *)allocateWithId(sizeof(float)*npt,"1readf1");

         for (cblock=0; cblock<datahead.nblocks; cblock++)
         {
            int j;
            register int cnt;
            register float *ptr;

            if ( (r = D_getbuf(D_DATAFILE, datahead.nblocks, cblock, &inblock)) )
            {
               D_error(r);
               releaseAllWithId("1readf1");
               close(outRe);
               if (outIm)
                  close(outIm);
               if (retc > 0)
               {
                  retv[0] = intString( 0 );
                  RETURN;
               }
               ABORT;
            }
            for (j=0; j<datahead.ntraces; j++)
            {
               if (outIm)
               {
                  /* Real F2 component */
                  read(outRe, & fidblockheader, sizeof(dblockhead) );
                  read(outRe, bptr, sizeof(float)*npt );
                  ptr = (float *) inblock.data+(j*npt*2);
                  optr = bptr;
                  for (cnt=0; cnt< npt/2; cnt++)
                  {
                     tmp.i = ntohl(*optr);
                     *ptr = tmp.f / F1NORM;
                     ptr += 2;
                     optr++;
                     tmp.i = ntohl(*optr);
                     *ptr = - tmp.f / F1NORM;
                     ptr += 2;
                     optr++;
                  }

                  /* Imaginary F2 component */
                  read(outIm, & fidblockheader, sizeof(dblockhead) );
                  read(outIm, bptr, sizeof(float)*npt );
                  ptr = (float *) inblock.data+(j*npt*2);
                  ptr++;
                  optr = bptr;
                  for (cnt=0; cnt< npt/2; cnt++)
                  {
                     tmp.i = ntohl(*optr);
                     *ptr = tmp.f / F1NORM;
                     ptr += 2;
                     optr++;
                     tmp.i = ntohl(*optr);
                     *ptr = - tmp.f / F1NORM;
                     ptr += 2;
                     optr++;
                  }
               }
               else
               {
                  read(outRe, & fidblockheader, sizeof(dblockhead) );
                  read(outRe, bptr, sizeof(float)*npt );
                  ptr = (float *) inblock.data+(j*npt);
                  optr = bptr;
                  for (cnt=0; cnt< npt/2; cnt++)
                  {
                     tmp.i = ntohl(*optr);
                     *ptr++ = tmp.f / F1NORM;
                     optr++;
                     tmp.i = ntohl(*optr);
                     *ptr++ = - tmp.f / F1NORM;
                     optr++;
                  }
               }
            }
            D_markupdated(D_DATAFILE, cblock);
            D_release(D_DATAFILE, cblock);
         }
         releaseAllWithId("1readf1");
         close(outRe);
         if (outIm)
            close(outIm);
      }
      else
      {
         Winfoprintf("No interferrograms available");
         if (retc > 0)
         {
            retv[0] = intString( 0 );
            RETURN;
         }
         ABORT;
      }
   }
   else
   {
      D_error(r);
      if (retc > 0)
      {
         retv[0] = intString( 0 );
         RETURN;
      }
      ABORT;
   }
   if (retc > 0)
   {
      retv[0] = intString( 1 );
   }
   D_trash(D_PHASFILE);
   RETURN;
}

int writef1(int argc, char *argv[], int retc, char *retv[])
{
   int		r;
   dpointers    inblock;
   dfilehead	fidhead,
		datahead;
   dblockhead   fidblockheader;
   char		filepath[MAXPATH];
   char		fidpath[MAXPATH];
   register int npt;
   int tbytes;
   int outRe;
   int outIm = 0;
   int *bptr;
   register int *optr;
   typedef union 
   {
      float  f;
      int    i;
   } unionFI;
   unionFI tmp;



   if ( (r = D_getfilepath(D_DATAFILE, filepath, curexpdir)) )
   {
      D_error(r);
      ABORT;
   }

   r = D_gethead(D_DATAFILE, &datahead);
   if (r)
      r = D_open(D_DATAFILE, filepath, &datahead);   /* open the data file */

   if (r == COMPLETE)
   {
      int cblock;

      if ( ((datahead.status & S_SECND) != S_SECND) &&
           ((datahead.status & S_TRANSF) == S_TRANSF) &&
           ((datahead.status & S_SPEC) == S_SPEC) )
      {
         fidhead.nblocks = datahead.nblocks * datahead.ntraces;
         fidhead.ntraces = 1;
         if (datahead.nbheaders == 1)
         {
            npt = datahead.np;
         }
         else
         {
            npt = datahead.np / 2;
         }
         fidhead.np = npt;
         fidhead.ebytes = datahead.ebytes;

         tbytes = fidhead.tbytes = fidhead.np * fidhead.ebytes;
         fidhead.bbytes = sizeof(dblockhead) + fidhead.tbytes;
         fidhead.status = S_DATA | S_COMPLEX | S_FLOAT ;
         fidhead.nbheaders = 1;
         fidhead.vers_id = 0;
         DATAFILEHEADER_CONVERT_HTON( &fidhead );

         sprintf(fidpath,"%s/fidf1r",curexpdir);
         outRe = open(fidpath, O_CREAT|O_WRONLY|O_TRUNC, 0666);
         write(outRe, & fidhead, sizeof(dfilehead) );
         fidpath[strlen(fidpath)-1] = 'i';
         if (datahead.nbheaders == 2)
         {
            outIm = open(fidpath, O_CREAT|O_WRONLY|O_TRUNC, 0666);
            write(outIm, & fidhead, sizeof(dfilehead) );
         }
         else
         {
            unlink(fidpath);
         }

         fidblockheader.scale = (short) 0;
         fidblockheader.status = S_DATA | S_FLOAT | S_COMPLEX | S_DDR;/* init status to fid*/
         fidblockheader.index = (short) 0;
         fidblockheader.mode = (short) 0;
         fidblockheader.ctcount = (long) 1;
         fidblockheader.lpval = (float) 0.0;
         fidblockheader.rpval = (float) 0.0;
         fidblockheader.lvl = (float) 0.0;
         fidblockheader.tlt = (float) 0.0;
         DATABLOCKHEADER_CONVERT_HTON( &fidblockheader );
         bptr =(int *)allocateWithId(sizeof(float)*npt,"1writef1");

         for (cblock=0; cblock<datahead.nblocks; cblock++)
         {
            int j;
            register int cnt;
            register float *ptr;

            if ( (r = D_getbuf(D_DATAFILE, datahead.nblocks, cblock, &inblock)) )
            {
               D_error(r);
               releaseAllWithId("1writef1");
               close(outRe);
               if (outIm)
                  close(outIm);
               if (retc > 0)
               {
                  retv[0] = intString( 0 );
                  RETURN;
               }
               ABORT;
            }
            for (j=0; j<datahead.ntraces; j++)
            {
               fidblockheader.index = htons(j+1 + (cblock*datahead.ntraces) );

               if (outIm)
               {
                  write(outRe, & fidblockheader, sizeof(dblockhead) );
                  /* Real F2 component */
                  ptr = (float *) inblock.data+(j*npt*2);
                  optr = bptr;
                  for (cnt=0; cnt< npt/2; cnt++)
                  {
                     tmp.f = *ptr * F1NORM ;
                     *optr++ = ntohl(tmp.i);
                     ptr += 2;
                     tmp.f = - *ptr * F1NORM;
                     *optr++ = ntohl(tmp.i);
                     ptr += 2;
                  }
                  ptr = (float *) inblock.data+(j*npt*2);
                  write(outRe, bptr, tbytes );

                  /* Imaginary F2 component */
                  write(outIm, & fidblockheader, sizeof(dblockhead) );
                  ptr = (float *) inblock.data+(j*npt*2);
                  ptr++;
                  optr = bptr;
                  for (cnt=0; cnt< npt/2; cnt++)
                  {
                     tmp.f = *ptr * F1NORM;
                     *optr++ = ntohl(tmp.i);
                     ptr += 2;
                     tmp.f = - *ptr * F1NORM;
                     *optr++ = ntohl(tmp.i);
                     ptr += 2;
                  }
                  ptr = (float *) inblock.data+(j*npt*2);
                  write(outIm, bptr, tbytes );
               }
               else
               {
                  write(outRe, & fidblockheader, sizeof(dblockhead) );
                  ptr = (float *) inblock.data+(j*npt);
                  optr = bptr;
                  for (cnt=0; cnt< npt/2; cnt++)
                  {
                     tmp.f = *ptr * F1NORM;
                     *optr++ = ntohl(tmp.i);
                     ptr++;
                     tmp.f = - *ptr * F1NORM;
                     *optr++ = ntohl(tmp.i);
                     ptr++;
                  }
                  ptr = (float *) inblock.data+(j*npt);
                  write(outRe, bptr, tbytes );
               }
            }
            D_release(D_DATAFILE, cblock);
         }
         releaseAllWithId("1writef1");
         close(outRe);
         if (outIm)
            close(outIm);
      }
      else
      {
         Winfoprintf("No interferrograms available");
         if (retc > 0)
         {
            retv[0] = intString( 0 );
            RETURN;
         }
         ABORT;
      }
   }
   else
   {
      D_error(r);
      if (retc > 0)
      {
         retv[0] = intString( 0 );
         RETURN;
      }
      ABORT;
   }
   if (retc > 0)
   {
      retv[0] = intString( 1 );
   }
   RETURN;
}


/*---------------------------------------
|					|
|	   driftcorrect_fid()/4		|
|					|
+--------------------------------------*/
void driftcorrect_fid(float *outp, int np, int lsfid, int datamult)
{
   register int		i,
			j;
   register float	*pntr,
			ftmp1,
			ftmp2,
			ftmp3,
			ftmp4;


   
   pntr = outp + datamult*(np - lsfid);
   i = j = (np - lsfid) / 16;
   ftmp1 = ftmp2 = ftmp3 = ftmp4 = 0.0;

   if (datamult == HYPERCOMPLEX)
   {
      while (i--)
      {
         ftmp4 += *(--pntr);
         ftmp3 += *(--pntr);
         ftmp2 += *(--pntr);
         ftmp1 += *(--pntr);
      }

      ftmp3 /= j;
      ftmp4 /= j;
   }
   else
   {
      while (i--)
      {
         ftmp2 += *(--pntr);
         ftmp1 += *(--pntr);
      } 
   }

   ftmp1 /= j;
   ftmp2 /= j;

   if (lsfid < 0)
   {
      pntr = outp - (datamult * lsfid);
      i = np;
   }
   else
   {
      pntr = outp;
      i = np - lsfid;
   }

   if (datamult == HYPERCOMPLEX)
   {
      while (i--)
      {
         *pntr++ -= ftmp1;
         *pntr++ -= ftmp2;
         *pntr++ -= ftmp3;
         *pntr++ -= ftmp4;
      }
   }
   else
   {
      while (i--)
      {
         *pntr++ -= ftmp1;
         *pntr++ -= ftmp2;
      }
   }
}


/*---------------------------------------
|					|
|	    getzfnumber()/2		|
|					|
|  This function calculates the number	|
|  of "real" points to zero-fill.	|
|					|
+--------------------------------------*/
int getzfnumber(int ndpts, int fnsize)
{
   while (fnsize >= ndpts)
      fnsize /= 2;
   fnsize *= 2;

   return(fnsize - ndpts);	/* the ZF number */
}


/*---------------------------------------
|					|
|	       getzflevel()/2		|
|					|
|  This function calculates the zero-	|
|  filling level for the FT.		|
|					|
+--------------------------------------*/
int getzflevel(int ndpts, int fnsize)
{
   int	zflvl,
	i;

   zflvl = fnsize/(2*ndpts);
   if (zflvl > 0)
   {
      i = 2;
      while (i <= zflvl)
         i *= 2;
      zflvl = i/2;
   }
   else
   {   
      zflvl = 0;
   }

   return(zflvl);
}


/*---------------------------------------
|					|
|	     getfpmult()/1		|
|					|
+--------------------------------------*/
double getfpmult(int fdimname, int ddr)
{
  char		fp_parname[MAXSTR];
  double	fp_default,
		fp_value;
  double	lpstrt, lpext;
  vInfo		info;


  if (fdimname & S_NI2)
  {
     strcpy(fp_parname, "fpmult2");
     fp_default = 0.5;
  }
  else if (fdimname & (S_NF|S_NI))
  {
     strcpy(fp_parname, "fpmult1");
     fp_default = 0.5;
  }
  else
  {
     if (ddr)
     {
        fp_default = 0.5;
     }
     else
     {
        fp_default = 1.0;
        if (!P_getstring(CURRENT, "proc", fp_parname, 1,8))
          if (!strcmp(fp_parname,"lp"))
            if (!P_getstring(CURRENT, "lpopt", fp_parname, 1,2))
              if (fp_parname[0] == 'b')
                if (!P_getreal(CURRENT, "strtext", &lpstrt, 1) &&
	            !P_getreal(CURRENT, "lpext", &lpext, 1))
	          if (lpstrt-lpext <= 0)
	          {
	            fp_default = 0.5;
	          }
     }
     strcpy(fp_parname, "fpmult");
  }

  if (P_getreal(CURRENT, fp_parname, &fp_value, 1))
  {
     fp_value = fp_default;
  }
  else
  {
     if (P_getVarInfo(CURRENT, fp_parname, &info))
     {
        fp_value = fp_default;
     }
     else
     {
        if (!info.active)
           fp_value = fp_default;
     }
  }

  return(fp_value);
}


/*---------------------------------------
|					|
|	     getrdblocks()/2		|
|					|
+--------------------------------------*/
static int getrdblocks(int nblocks, int f1_zflevel)
{
   while ( (f1_zflevel--) && (nblocks > 1) )
      nblocks /= 2;

   return(nblocks);
}


/*-----------------------------------------------
|						|
|	      reset_dataheader()/0		|
|						|
|  Resets file header in DATA to all zeroes.	|
|						|
+----------------------------------------------*/
static int reset_dataheader()
{
   char		filepath[MAXPATHL];
   int		res;
   dfilehead	datahead;


   if ( (res = D_getfilepath(D_DATAFILE, filepath	, curexpdir)) )
   {
      D_error(res);
      return(ERROR);
   }

   if ( (res = D_gethead(D_DATAFILE, &datahead)) )
   {
      if ( (res = D_open(D_DATAFILE, filepath, &datahead)) )
      {
         D_error(res);
         return(ERROR);
      }
   }

   datahead.nblocks = 0;
   datahead.ntraces = 0;
   datahead.np = 0;
   datahead.ebytes = 0;
   datahead.tbytes = 0;
   datahead.bbytes = sizeof(dblockhead);
   datahead.status = 0;
   datahead.nbheaders = 1;

   if ( (res = D_updatehead(D_DATAFILE, &datahead)) )
      return(ERROR);

   return(COMPLETE);
}
