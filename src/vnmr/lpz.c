/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#ifndef FT3D
#include "vnmrsys.h"
#include "allocate.h"
#include "group.h"
#include "variables.h"
#include "pvars.h"
#include "wjunk.h"
#else 
#define MAXPATHL	128
#endif 

#include "data.h"
#include "process.h"


#define ERROR			-1
#define COMPLETE		0
#define FALSE			0
#define TRUE			1

#define LD_SMALL_NUMBER		1e-15

#ifndef FT3D
static char	lpfilename[MAXPATHL];	/* file name for external LP
					   analysis */
#endif
static int	dbgtrace = -1;		/* for external LP analysis */

static FILE	*fdanlyz = NULL;	/* file descriptor for external
					   LP analysis */

static void autocorcalc(fcomplex *, lpinfo  *);
static dcomplex * tplz_solve(lpinfo  *);
static void lpextend(fcomplex  *, dcomplex *, lpinfo);
static void reverselp(dcomplex  *, int);
static void closelpanalyz();
void writelpmatrix(lpinfo lppar);
void writelpcoefs(dcomplex *coef, lpinfo lppar);

extern double cxabs(dcomplex cval);
extern int rootpolynm(dcomplex *lpcoef, lpinfo lppar, int rootadjust);

#ifndef FT3D
extern char curexpdir[];
#endif 


#ifdef FT3D

/*---------------------------------------
|                                       |
|            setlpfuncs()/1             |
|                                       |
+--------------------------------------*/
void setlpfuncs(lpstruct *parLPinfo)
{
   int			i;
   extern void		lsmatcalc();
   extern dcomplex	*qld_solve();
   lpinfo		*lppar;


   lppar = parLPinfo->parLP;

   for (i = 0; i < parLPinfo->sizeLP; i++)
   {
      if (lppar->status & LPFFT)
      {
         lppar->lpmat_setup = lsmatcalc;
         lppar->lpcoef_solve = qld_solve;
         strcpy(lppar->label, "LP");
      }
      else
      { /* default is LPAR */
         lppar->lpmat_setup = autocorcalc;
         lppar->lpcoef_solve = tplz_solve;
         strcpy(lppar->label, "LPAR");
      }

      lppar += 1;
   }
}
#else 
/*---------------------------------------
|                                       |
|            findLPsize()/9             |
|                                       |
+--------------------------------------*/
int findLPsize(char *par1, char *par2, char *par3,
               char *par4, char *par5, char *par6,
               char *par7, char *par8, char *par9)
{
   char	*par;
   int	i,
	size = 1,	/* initialization */
	tmpsize;


   for (i = 2; i < 10; i++)
   {
      switch (i)
      {
         case 1:	par = par1; break;
         case 2:	par = par2; break;
         case 3:	par = par3; break;
         case 4:	par = par4; break;
         case 5:	par = par5; break;
         case 6:	par = par6; break;
         case 7:	par = par7; break;
         case 8:	par = par8; break;
         case 9:	par = par9; break;
         default:	par = par9;
      }

      if ( P_getsize(CURRENT, par, &tmpsize) )
         return(1);
      if (tmpsize > size)
         size = tmpsize;
   }

   return(size);
}


/*---------------------------------------
|                                       |
|            getLPindex()/2             |
|                                       |
+--------------------------------------*/
static int getLPindex(char *parname, int parindex)
{
   vInfo	info;


   if ( P_getVarInfo(CURRENT, parname, &info) )
      info.size = 1;

   return( (parindex > info.size) ? info.size : parindex );
}


/*---------------------------------------
|                                       |
|             setlppar()/7              |
|                                       |
+--------------------------------------*/
/* char	 *memId;	program ID for memory allocation		*/
/* int   dimen,         acquisition dimension, e.g., NP              */
/*       pstatus,       type of processing requested, e.g., LP       */
/*       nctdpts,       number of complex time-domain data points    */
/*       ncfdpts,       number of complex freq-domain data points    */
/* 	 mode;		allocate or not allocate memory		*/
int setlppar(lpstruct *parLPinfo, int dimen, int pstatus,
             int nctdpts, int ncfdpts, int mode, char *memId)
{
   char        		lpopt_str[15],
               		lpoptname[15],
               		lpalg_str[15],
               		lpalgname[15],
               		lpextname[15],
               		startextname[15],
               		lpfiltname[15],
               		startlpname[15],
               		lpnuptsname[15],
			lpprtname[15],
			lptrcname[15];
   int         		i,
			initvalues = FALSE,
			lpsize;
   double      		tmp;
   extern void		lsmatcalc();
   extern dcomplex	*qld_solve();
   lpinfo		*lppar;
 
 
/*************************************
*  Get the appropriate LP parameter  *
*  names.                            *
*************************************/

   if (dimen & S_NP)
   {
      if (pstatus & LP_F2PROC)
      {
         initvalues = TRUE;
         strcpy(lpalgname, "lpalg");
         strcpy(lpoptname, "lpopt");
         strcpy(lpextname, "lpext");
         strcpy(startextname, "strtext");
         strcpy(lpfiltname, "lpfilt");
         strcpy(startlpname, "strtlp");
         strcpy(lpnuptsname, "lpnupts");
         strcpy(lpprtname, "lpprint");
         strcpy(lptrcname, "lptrace");
         strcpy(lpfilename, "lpanalyz.out");
      }
   }   
   else if (dimen & (S_NF|S_NI))
   {
      if (pstatus & LP_F1PROC)
      {
         initvalues = TRUE;
         strcpy(lpalgname, "lpalg1");
         strcpy(lpoptname, "lpopt1");
         strcpy(lpextname, "lpext1");
         strcpy(startextname, "strtext1");
         strcpy(lpfiltname, "lpfilt1");
         strcpy(startlpname, "strtlp1");
         strcpy(lpnuptsname, "lpnupts1");
         strcpy(lpprtname, "lpprint1");
         strcpy(lptrcname, "lptrace1");
         strcpy(lpfilename, "lpanalyz1.out");
      }
   }  
   else if (dimen & S_NI2)
   {  
      if (pstatus & LP_F1PROC)
      {
         initvalues = TRUE;
         strcpy(lpalgname, "lpalg2");
         strcpy(lpoptname, "lpopt2");
         strcpy(lpextname, "lpext2");
         strcpy(startextname, "strtext2");
         strcpy(lpfiltname, "lpfilt2");
         strcpy(startlpname, "strtlp2");
         strcpy(lpnuptsname, "lpnupts2");
         strcpy(lpprtname, "lpprint2");
         strcpy(lptrcname, "lptrace2");
         strcpy(lpfilename, "lpanalyz2.out");
      }
   }

   parLPinfo->lppntr = NULL;		/* initialization */
   parLPinfo->membytes = 0;
   parLPinfo->parLP = NULL;
   parLPinfo->sizeLP = 0;

   if (!initvalues)
      return(COMPLETE);
      
 
/*****************************************
*  Obtain the values stored in the VNMR  *
*  parameter set for the LP parameter    *
*  structure.  Find the array size for   *
*  the LP parameters.                    *
*****************************************/

   lpsize = findLPsize(lpalgname, lpoptname, lpextname, startextname,
			lpfiltname, startlpname, lpnuptsname,
			lpprtname, lptrcname);

   if ( (parLPinfo->parLP = (lpinfo *) allocateWithId( sizeof(lpinfo) *
		lpsize, memId )) == NULL )
   {
      Werrprintf("insufficient memory for LP processing");
      return(ERROR);
   }

   parLPinfo->sizeLP = lpsize;
   lppar = parLPinfo->parLP;

/******************************************
*  Initialize values in the LP parameter  *
*  structure and then load the values.    *
******************************************/

   for (i = 0; i < lpsize; i++)
   {
      lppar->lppntr = NULL;	  /* pntr to memory block used by LP	    */
      lppar->lpmat_setup = NULL;  /* pnter to function setting up LP matrix */
      lppar->lpcoef_solve = NULL; /* pnter to function obtaining LP coefs.  */
      lppar->membytes = 0;	  /* memory require for set "index"	    */
      lppar->nsignals = 0;        /* number of complex sinusoidal signals   */
      lppar->lstrace = 1.0;       /* trace of the LP least-squares matrix   */
      lppar->ncfilt = 0;          /* number of complex LP/LPAR coefficients */
      lppar->startlppt = 0;       /* starting point for LP/LPAR analysis    */
      lppar->ncextpt = 0;         /* number of LP/LPAR-extended complex pts */
      lppar->startextpt = 0;      /* starting point for LP/LPAR extension   */
      lppar->ncupts = 0;          /* number of points used in AR vector     */
      lppar->status = 0;          /* LP status                              */
      lppar->trace = 0;           /* selected trace for LP printout         */
      lppar->index = i;		  /* LP parameter index			    */

/****************************************
*  Get the parameter specifying the LP  *
*  algorithm.                           *
****************************************/

      if ( !P_getstring(CURRENT, lpalgname, lpalg_str,
			getLPindex(lpalgname, i+1), 15) )
      {
         if (strcmp(lpalg_str, "lpfft") == 0)
         {
            lppar->status = LPFFT;
            lppar->lpmat_setup = lsmatcalc;
            lppar->lpcoef_solve = qld_solve;
            strcpy(lppar->label, "LP");
         }
         else if (strcmp(lpalg_str, "lparfft") == 0)
         {
            lppar->status = LPARFFT;
            lppar->lpmat_setup = autocorcalc;
            lppar->lpcoef_solve = tplz_solve;
            strcpy(lppar->label, "LPAR");
         }
         else
         {
            Werrprintf("unsupported method of LP analysis: %s", lpalgname);
            return(ERROR);
         }
      }
      else
      {
         Werrprintf("LP processing aborted:  %s not found", lpalgname);
         return(ERROR);
      }

/****************************************
*  Get the parameter specifying the LP  *
*  option.                              *
****************************************/

      if ( !P_getstring(CURRENT, lpoptname, lpopt_str,
			getLPindex(lpoptname, i+1), 15) )
      {
         if (strcmp(lpopt_str, "f") == 0)
         {
            lppar->status |= FORWARD;
         }
         else if (strcmp(lpopt_str, "b") != 0)
         {   
            Werrprintf("unsupported option with LP analysis: %s", lpoptname);
            return(ERROR);
         }
      }
      else
      {
         Werrprintf("LP processing aborted:  %s not found", lpoptname);
         return(ERROR);
      }

/***********************************************
*  Get the parameter specifying the number of  *
*  complex LP coefficients to be used.         *
***********************************************/

      if ( !P_getreal(CURRENT, lpfiltname, &tmp,
		getLPindex(lpfiltname, i+1)) )
      {
         lppar->ncfilt = (int) (tmp + 0.5);
         if (lppar->ncfilt < 1)
         {
            Werrprintf("value for %s is too small", lpfiltname);
            return(ERROR);
         }
         else if (lppar->ncfilt > (nctdpts/2 - 1))
         {
            Werrprintf("value for %s is too large", lpfiltname);
            return(ERROR);
         }
      }
      else
      {
         Werrprintf("LP processing aborted:  %s not found", lpfiltname);
         return(ERROR);
      }

/**********************************************
*  Get the parameter specifying the starting  *
*  point for data extension using the LP      *
*  coefficients.                              *
**********************************************/

      if ( !P_getreal(CURRENT, startextname, &tmp,
		getLPindex(startextname, i+1)) )
      {
         lppar->startextpt = (int) (tmp + 0.5);
         if (lppar->startextpt < 1)
         {
            Werrprintf("value for %s is too small", startextname);
            return(ERROR);
         }
         else if (lppar->startextpt > (nctdpts + 1))
         {
            Werrprintf("value for %s is too large", startextname);
            return(ERROR);
         }
      }
      else
      {
         Werrprintf("LP processing aborted:  %s not found", startextname);
         return(ERROR);
      }
 
/********************************************
*  Get the parameter specifying the number  *
*  of complex points that the data is to    *
*  be extended using the LP coefficients.   *
********************************************/

      if ( !P_getreal(CURRENT, lpextname, &tmp,
			getLPindex(lpextname, i+1)) )
      {
         lppar->ncextpt = (int) (tmp + 0.5);
         if (lppar->ncextpt < 1)
         {
            Werrprintf("value for %s cannot be < 1", lpextname);
            return(ERROR);
         }
 
         if (lppar->status & FORWARD)
         {
            if ( (lppar->ncextpt + lppar->startextpt - 1) > ncfdpts )
            {
               Werrprintf("forward-calculation value for %s is too large",
				lpextname);
               return(ERROR);
            }
         }   
         else
         {   
            if ( (lppar->startextpt - lppar->ncextpt) < 0 )
            {
               Werrprintf("back-calculation value for %s is too large",
				lpextname);
               return(ERROR);
            }
         }   
      }
      else
      {
         Werrprintf("LP processing aborted:  %s not found", lpextname);
         return(ERROR);
      }

/**********************************************
*  Get the parameter specifying the starting  *
*  point for extraction of the complex LP     *
*  coefficients.                              *
**********************************************/

      if ( !P_getreal(CURRENT, startlpname, &tmp,
			getLPindex(startlpname, i+1)) )
      {
         lppar->startlppt = (int) (tmp + 0.5);
         if (lppar->status & FORWARD)
         {
            if ( (lppar->startlppt - 2*lppar->ncfilt) < 0 )
            {
               Werrprintf("value for %s is too small", startlpname);
               return(ERROR);
            }
         }
         else
         {   
            if ( (lppar->startlppt + 2*lppar->ncfilt) > nctdpts )
            {
               Werrprintf("value for %s is too large", startlpname);
               return(ERROR);
            }
         }
      }
      else
      {
         Werrprintf("LP processing aborted:  %s not found", startlpname);
         return(ERROR);
      }

/***********************************************
*  Get the parameter specifying the number of  *
*  complex points to be used in calculating    *
*  the auto-regressive (AR) or least-squares   *
*  (LS) matrix elements.                       *
***********************************************/

      lppar->ncupts = 2*lppar->ncfilt;	/* initialization */
      if ( !P_getreal(CURRENT, lpnuptsname, &tmp,
			getLPindex(lpnuptsname, i+1)) )
      {
         lppar->ncupts = (int) (tmp + 0.5);
      }

      if ( lppar->ncupts < (2*lppar->ncfilt) )
      {
         Werrprintf("value for %s cannot be smaller than twice that for %s",
                          lpnuptsname, lpfiltname);
         return(ERROR);
      }

      if (lppar->status & FORWARD)
      {
         if ((lppar->startlppt - lppar->ncupts) < 0)
         {
            Werrprintf("value for %s is too small for the value of %s",
                          startlpname, lpnuptsname);
            return(ERROR);
         }
      }   
      else
      {   
         if ( (lppar->startlppt + lppar->ncupts -1) > nctdpts )
         {
            Werrprintf("value for %s is too large for the value of %s",
                          startlpname, lpnuptsname);
            return(ERROR);
         }
      }

/************************************************
*  Get the parameters specifying the LP print   *
*  level and the trace for which the printing   *
*  operations are active.                       *
************************************************/

      if ( !P_getreal(CURRENT, lpprtname, &tmp,
			getLPindex(lpprtname, i+1)) )
      {
         lppar->printout = (int) (tmp + 0.5);
         lppar->printout &= LPPRT_OPTS;
      }

      if ( !P_getreal(CURRENT, lptrcname, &tmp,
			getLPindex(lptrcname, i+1)) )
      {
         lppar->trace = (int) (tmp + 0.5) - 1;
      }

/**********************************************
*  Allocate the memory necessary for each     *
*  type of LP algorithm.  Forward Prediction  *
*  also requires memory for the polynomial    *
*  rooting operation.                         *
**********************************************/

      if (lppar->status & LPFFT)
      {
         lppar->membytes = ( 2*lppar->ncfilt*(lppar->ncfilt + 3) ) *
				sizeof(dcomplex);
      }
      else
      { /* default is LPAR */
         lppar->membytes = (6*lppar->ncfilt) * sizeof(dcomplex);
      }

      if (lppar->status & FORWARD)
         lppar->membytes += (2*lppar->ncfilt + 1) * sizeof(dcomplex);

      if (lppar->membytes > parLPinfo->membytes)
         parLPinfo->membytes = lppar->membytes;

      lppar += 1;	/* go to the next LP parameter structure */
   }

/****************************************
*  Allocate memory if LP processing is  *
*  being done within VNMR.              *
****************************************/

   if (mode)
   {
      if ( (parLPinfo->lppntr = (dcomplex *) allocateWithId(
			parLPinfo->membytes, memId)) == NULL )
      {
         Werrprintf("LP processing aborted:  memory allocation failed");
         return(ERROR);
      }

      lppar = parLPinfo->parLP;
      for (i = 0; i < lpsize; i++)
      {
         lppar->lppntr = parLPinfo->lppntr;
         lppar += 1;
      }
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	       lpanalyz()/1		|
|					|
+--------------------------------------*/
static void lpanalyz(lpinfo *lpparval)
{
   char	filepath[MAXPATHL],
	fext[16];


   if ( (lpparval->trace == dbgtrace) && (lpparval->printout & LPPRT_OPTS) )
   {
      strcpy(filepath, curexpdir);
#ifdef UNIX
      strcat(filepath, "/");
#endif 
      strcat(filepath, lpfilename);
      sprintf(fext, ".%d", lpparval->index + 1);
      strcat(filepath, fext);

      if ( (fdanlyz = fopen(filepath, "w")) == NULL )
      {
         Werrprintf("cannot open file %s for external LP analysis",
			   filepath);
         lpparval->printout = 0;
      }
   }
}
#endif 


/*---------------------------------------
|					|
|	     closelpanalyz()/0		|
|					|
+--------------------------------------*/
static void closelpanalyz()
{
   if (fdanlyz != NULL)
   {
      fclose(fdanlyz);
      fdanlyz = NULL;
   }
}


/*---------------------------------------
|					|
|	   getcmplx_intfgm()/4		|
|					|
+--------------------------------------*/
void getcmplx_intfgm(float *data, float *buffer, int nhcpts, int imagskip)
{
   register int		i,
			iskip,
			rskip;
   register float	*srcpntr,
			*destpntr;


   iskip = imagskip;
   rskip = HYPERCOMPLEX - iskip;
   srcpntr = data;
   destpntr = buffer;

   for (i = 0; i < nhcpts; i++)
   {
      *destpntr++ = *srcpntr;
      *destpntr++ = *(srcpntr += iskip);
      srcpntr += rskip;
   }
}


/*---------------------------------------
|					|
|	   putcmplx_intfgm()/4		|
|					|
+--------------------------------------*/
void putcmplx_intfgm(float *data, float *buffer, int nhcpts, int imagskip)
{
   register int		i,
			iskip,
			rskip;
   register float	*srcpntr,
			*destpntr;


   iskip = imagskip;
   rskip = HYPERCOMPLEX - iskip;
   srcpntr = buffer;
   destpntr = data;

   for (i = 0; i < nhcpts; i++)
   {
      *destpntr = *srcpntr++;
      *(destpntr += iskip) = *srcpntr++;
      destpntr += rskip;
   }
}


/*-------------------------------------------------------
|							|
|		        lpz()/4				|
|							|
|   This module performs LP and LPAR processing on	|
|   FID's using the Levinson-Durbin method to obtain	|
|   the LP coefficients from the asymmetric (LP) or	|
|   symmetric (LPAR) Toeplitz matrix.			|
|							|
+------------------------------------------------------*/
/* int	trace,		trace number of time-domain data		*/
/* 	nctdpts;	number of complex time-domain data points	*/
/* float *data; 	raw FID or t1-interferogram data		*/
/* lpinfo lpparvals     LP parameter structure			*/
int lpz(int trace, float *data, int nctdpts, lpinfo lpparvals)
{
   int			lprootadjust = TRUE;		/* future hook	*/
   fcomplex		*newdata,
			*lpfiddata;
   dcomplex	*lpcoef;

   (void) nctdpts;
#ifndef FT3D
   dbgtrace = trace;
   lpanalyz(&lpparvals);
#endif 

/*********************************************
*  Initialize the function pointers and the  *
*  data pointer.                             *
*********************************************/

   newdata = (fcomplex *)data + lpparvals.startextpt - 1;
   lpfiddata = (fcomplex *)data + lpparvals.startlppt;
   lpfiddata -= ( (lpparvals.status & FORWARD) ? lpparvals.ncupts : 1 );

/*********************************
*  Begin LP calculation.  Setup  *
*  the appropriate LP matrix.    *
*********************************/

   (*lpparvals.lpmat_setup)(lpfiddata, &lpparvals);

/**********************************
*  Solve for the LP coefficients  *
**********************************/

   if ( (lpcoef = (*lpparvals.lpcoef_solve)(&lpparvals)) == NULL )
   {
      closelpanalyz();
      return(ERROR);
   }

/****************************************************
*  If forward prediction is being used with the LP  *
*  algorithm, root the complex polynomial, reflect  *
*  any roots that are outside the unit circle back  *
*  into the unit circle, and recalculate the LP     *
*  coefficients.                                    *
****************************************************/

   if ( (lpparvals.status & FORWARD) && (lpparvals.status & LPFFT) )
   {
      if ( rootpolynm(lpcoef, lpparvals, lprootadjust) )
      {
         closelpanalyz();
         return(ERROR);
      }
   }

/*************************************************
*  Extend the time-domain data to the requested  *
*  number of complex points or back-calculate    *
*  the requested number of complex points from   *
*  a specified location.                         *
*************************************************/

   lpextend(newdata, lpcoef, lpparvals);

/****************************
*  Close LP analysis file.  *
****************************/

   closelpanalyz();
   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|	       autocorcalc()/2			|
|						|
|  This function sets up the auto-regressive	|
|  (AR) data matrix for the LP-AR method.  The	|
|  equations for backward prediction are:	|
|						|
|						|
|   |R(0)   R(-1)   R(-2)|   |a3|     |R(-3)|   |
|   |R(1)    R(0)   R(-1)|   |a2|  =  |R(-2)|   |
|   |R(2)    R(1)    R(0)|   |a1|     |R(-1)|   |
|						|
|						|
|  The equations for forward prediction are:    |
|                                               |
|                                               |
|   |R(0)   R(-1)   R(-2)|   |a1|     |R(1)|    |
|   |R(1)    R(0)   R(-1)|   |a2|  =  |R(2)|    |
|   |R(2)    R(1)    R(0)|   |a3|     |R(3)|    |
|                                               |
|                                               |
|  where R(1) is the complex conjugate of       |
|  R(-1).  The LP coefficient vector will need  |
|  to be reversed for Backwards Prediction to   |
|  be compatible with the lpextend() function.  |
|  For Forwards Prediction, the order a1-a4 is  |
|  required for the rooting function.		|
|						|
+----------------------------------------------*/
/* *data is pointer to FID or interferogram data */
/* *lppar is LP parameter structure              */
void autocorcalc(fcomplex *data, lpinfo  *lppar)
{
   register int		i,
			j,
			nfpts,
			nupts;
   register double	ftmp1,
			ftmp2,
			ftmp3,
			ftmp4;
   register fcomplex	*data1,
			*data2;
   register dcomplex	*armatrix,
			*tmparmatrix;


   nfpts = lppar->ncfilt;
   nupts = lppar->ncupts - nfpts;
   armatrix = lppar->lppntr + 2*nfpts - 1;

/*****************************************
*  Load the autocorrelation vector from  *
*  R(M-1) --> R(0).			 *
*****************************************/

   for (i = nfpts; i >= 0; i--)
   {
      armatrix->re = armatrix->im = 0.0;
      data1 = data + nfpts;
      data2 = data1 - i;

      for (j = 0; j < nupts; j++)
      {
         ftmp1 = (double) (data1->re);
         ftmp2 = (double) ((data1++)->im);
         ftmp3 = (double) (data2->re);
			/* will need complex conjugate of data2 */
         ftmp4 = (double) ((data2++)->im);
         armatrix->re += ftmp1*ftmp3 + ftmp2*ftmp4;
         armatrix->im += ftmp2*ftmp3 - ftmp1*ftmp4;
      }

      armatrix->re /= (double) nupts;
      armatrix->im /= (double) nupts;
      armatrix -= 1;
   }

/**************************************************
*  Now load the top half of the autocorrelation   *
*  vector, which is simply the complex conjugate  *
*  of the bottom half in reverse order:		  *
*  R(-1) --> R(-M+1).				  *
**************************************************/

   tmparmatrix = armatrix + 2;
   for (i = 1; i < nfpts; i++)
   {
      armatrix->re = tmparmatrix->re;
      (armatrix--)->im = (-1.0) * (tmparmatrix++)->im;
   }

/*****************************************
*  Now load the result vector for LPAR.  *
*****************************************/

   armatrix += nfpts + 1;

   if (lppar->status & FORWARD)
   {
      tmparmatrix += 1;
      for (i = 0; i < nfpts; i++)
         *tmparmatrix++ = *armatrix++;
   }
   else
   {
      tmparmatrix += nfpts;
      for (i = 0; i < nfpts; i++)
      {
         *(--tmparmatrix) = *armatrix++;
         tmparmatrix->im *= (-1.0);
      }
   }

   writelpmatrix(*lppar);
}


/*-----------------------------------------------
|						|
|	      writelpmatrix()/1			|
|						|
+----------------------------------------------*/
void writelpmatrix(lpinfo lppar)
{
   int		i,
		j;
   dcomplex	*datamatrix;


/***********************************
*  Produce output for external LP  *
*  analysis file "lpanalyz.out".   *
***********************************/

   if ( (lppar.trace != dbgtrace) || ((~lppar.printout) & LPMATRIX) )
      return;

   datamatrix = lppar.lppntr;

   if (lppar.status & LPFFT)
   {
      fprintf(fdanlyz, "\n\n\nLeast-squares X matrix for LP analysis:");
      for (i = 0; i < lppar.ncfilt; i++)
      {
         fprintf(fdanlyz, "\n\n\tRow %d:\n\n", i+1);
         for (j = 0; j < lppar.ncfilt; j++)
         {
            fprintf(fdanlyz, "\t%e\t%e\n", datamatrix->re, datamatrix->im);
            datamatrix += 1;
         }
      }

      datamatrix += (lppar.ncfilt*lppar.ncfilt);
      fprintf(fdanlyz,
		"\n\nLeast-squares Y column vector for LP analysis:\n\n");
      for (i = 0; i < lppar.ncfilt; i++)
      {
         fprintf(fdanlyz, "\t%e\t%e\n", datamatrix->re, datamatrix->im);
         datamatrix += 1;
      }
   }
   else
   { /* default is LPAR */
      fprintf(fdanlyz, "\n\n\nAutoregressive X matrix for LPAR analysis:\n\n");
      for (i = 0; i < (2*lppar.ncfilt - 1); i++)
      {
         fprintf(fdanlyz, "\t%e\t%e\n", datamatrix->re, datamatrix->im);
         datamatrix += 1;
      }

      fprintf(fdanlyz, "\n\nAutoregressive Y matrix:\n\n");
      datamatrix += 1;

      for (i = 0; i < lppar.ncfilt; i++)
      {
         fprintf(fdanlyz, "\t%e\t%e\n", datamatrix->re, datamatrix->im);
         datamatrix += 1;
      }
   }
}


/*-----------------------------------------------
|						|
|	        tplz_solve()/1			|
|						|
|   This function uses a version of the		|
|   Levinson-Durbin algorithm to solve the	|
|   nonsymmetric Toeplitz matrix for the	|
|   LP coefficients.				|
|						|
+----------------------------------------------*/
static dcomplex * tplz_solve(lpinfo  *lppar)
{
   int			dv1add = 1;
   register int		m,
			m1,
			j,
			nfpts;
   register double	ftmp1,
			ftmp2,
			ftmp3,
			ftmp4;
   dcomplex		*svlpvector,
			*svscratch_g,
			*svscratch_h,
			ctmp1,
			ctmp2,
			ctmp3,
			ctmp4,
			ctmp5,
			ctmp6;
   register dcomplex	*lpvector,
			*dvector1,
			*dvector2,
			*dvector3,
			*dvector4,
			*dvector5,
			*scratch_g,
			*scratch_h;
   extern dcomplex	cdiv();


   nfpts = lppar->ncfilt;
   lpvector = svlpvector = lppar->lppntr + 3*nfpts;
   scratch_g = lpvector + nfpts;
   svscratch_g = scratch_g;
   scratch_h = scratch_g + nfpts;
   svscratch_h = scratch_h;

   dvector2 = lppar->lppntr + nfpts - 1;	/* AR matrix vector     */
   dvector1 = lppar->lppntr + 2*nfpts;	/* result vector        */

   if ( cxabs(*dvector2) < LD_SMALL_NUMBER )
      return(NULL);

   *lpvector = cdiv(*dvector1, *dvector2);
   if (nfpts == 1)
   {
      writelpcoefs(svlpvector, *lppar);
      return(svlpvector);
   }

   *scratch_g = cdiv( *(dvector2 - 1), *dvector2 );
   *scratch_h = cdiv( *(dvector2 + 1), *dvector2 );

   for (m = 1; m < nfpts; m++)
   {
      m1 = m;
      dvector1 += dv1add;
      ctmp1.re = (-1.0) * dvector1->re;
      ctmp1.im = (-1.0) * dvector1->im;
      ctmp2.re = (-1.0) * dvector2->re;
      ctmp2.im = (-1.0) * dvector2->im;

      scratch_g = svscratch_g + m;
      dvector3 = dvector2 + m1 + 1;
      lpvector = svlpvector;

      for (j = 0; j < m; j++)
      {
         ftmp1 = (--dvector3)->re;
         ftmp2 = dvector3->im;
         ftmp3 = lpvector->re;
         ftmp4 = (lpvector++)->im;
         ctmp1.re += ftmp1*ftmp3 - ftmp2*ftmp4;
         ctmp1.im += ftmp1*ftmp4 + ftmp2*ftmp3;

         ftmp3 = (--scratch_g)->re;
         ftmp4 = scratch_g->im;
         ctmp2.re += ftmp1*ftmp3 - ftmp2*ftmp4;
         ctmp2.im += ftmp1*ftmp4 + ftmp2*ftmp3;
      }

      if ( cxabs(ctmp2) < LD_SMALL_NUMBER )
         return(NULL);

      scratch_g = svscratch_g + m;
      lpvector = svlpvector;
      dvector4 = lpvector + m1;
      *dvector4 = cdiv(ctmp1, ctmp2);

      ftmp1 = dvector4->re;
      ftmp2 = dvector4->im;

      for (j = 0; j < m; j++)
      {
         ftmp3 = (--scratch_g)->re;
         ftmp4 = scratch_g->im;
         lpvector->re -= ftmp1*ftmp3 - ftmp2*ftmp4;
         (lpvector++)->im -= ftmp1*ftmp4 + ftmp2*ftmp3;
      }

      if ( m1 == (nfpts - 1) )
      {
         reverselp(svlpvector, nfpts);
         writelpcoefs(svlpvector, *lppar);
         return(svlpvector);
      }

      dvector3 = dvector2 - m1 - 1;
      dvector4 = dvector2 + m1 + 1;
      scratch_g = svscratch_g;
      scratch_h = svscratch_h;
      dvector5 = scratch_h + m;

      ctmp3.re = (-1.0) * dvector3->re;
      ctmp3.im = (-1.0) * (dvector3++)->im;
      ctmp4.re = (-1.0) * dvector4->re;
      ctmp4.im = (-1.0) * dvector4->im;
      ctmp5.re = (-1.0) * dvector2->re;
      ctmp5.im = (-1.0) * dvector2->im;

      for (j = 0; j < m; j++)
      {
         ftmp1 = dvector3->re;
         ftmp2 = (dvector3++)->im;
         ftmp3 = scratch_g->re;
         ftmp4 = (scratch_g++)->im;
         ctmp3.re += ftmp1*ftmp3 - ftmp2*ftmp4;
         ctmp3.im += ftmp1*ftmp4 + ftmp2*ftmp3;

         ftmp3 = (--dvector5)->re;
         ftmp4 = dvector5->im;
         ctmp5.re += ftmp1*ftmp3 - ftmp2*ftmp4;
         ctmp5.im += ftmp1*ftmp4 + ftmp2*ftmp3;

         ftmp1 = (--dvector4)->re;
         ftmp2 = dvector4->im;
         ftmp3 = scratch_h->re;
         ftmp4 = (scratch_h++)->im;
         ctmp4.re += ftmp1*ftmp3 - ftmp2*ftmp4;
         ctmp4.im += ftmp1*ftmp4 + ftmp2*ftmp3;
      }

      if ( cxabs(ctmp2) < LD_SMALL_NUMBER )
         return(NULL);

      scratch_g = svscratch_g;
      scratch_h = svscratch_h;
      ctmp1 = *(scratch_g + m1) = cdiv(ctmp3, ctmp5);
      ctmp2 = *(scratch_h + m1) = cdiv(ctmp4, ctmp2);

      m1 = (m + 1) >> 1;
      dvector4 = scratch_g + m;
      dvector5 = scratch_h + m;

      for (j = 0; j < m1; j++)
      {
         ctmp3 = *scratch_g;
         ctmp4 = *(--dvector4);
         ctmp5 = *scratch_h;
         ctmp6 = *(--dvector5);

         ftmp1 = ctmp1.re;
         ftmp2 = ctmp1.im;
         ftmp3 = ctmp6.re;
         ftmp4 = ctmp6.im;
         scratch_g->re = ctmp3.re - (ftmp1*ftmp3 - ftmp2*ftmp4);
         (scratch_g++)->im = ctmp3.im - (ftmp1*ftmp4 + ftmp2*ftmp3);

         ftmp3 = ctmp5.re;
         ftmp4 = ctmp5.im;
         dvector4->re = ctmp4.re - (ftmp1*ftmp3 - ftmp2*ftmp4);
         dvector4->im = ctmp4.im - (ftmp1*ftmp4 + ftmp2*ftmp3);

         ftmp1 = ctmp2.re;
         ftmp2 = ctmp2.im;
         ftmp3 = ctmp4.re;
         ftmp4 = ctmp4.im;
         scratch_h->re = ctmp5.re - (ftmp1*ftmp3 - ftmp2*ftmp4);
         (scratch_h++)->im = ctmp5.im - (ftmp1*ftmp4 + ftmp2*ftmp3);

         ftmp3 = ctmp3.re;
         ftmp4 = ctmp3.im;
         dvector5->re = ctmp6.re - (ftmp1*ftmp3 - ftmp2*ftmp4);
         dvector5->im = ctmp6.im - (ftmp1*ftmp4 + ftmp2*ftmp3);
      }
   }

   return(NULL);
}


/*-----------------------------------------------
|						|
|	     writelpeigenvalues()/2		|
|						|
+----------------------------------------------*/
void writelpeigenvalues(double *evals, lpinfo lppar)
{
   int	i;


   if ( (lppar.trace != dbgtrace) || ((~lppar.printout) & LPCOEFS) )
      return;

   fprintf(fdanlyz, "\n\n\nEigenvalues of ~XX least-squares matrix:\n\n");
   fprintf(fdanlyz, "\tindex\tvalue\n");
   fprintf(fdanlyz, "\t-----\t-----\n\n");

   for (i = 0; i < lppar.ncfilt; i++)
      fprintf(fdanlyz, "\t %d\t%e\n", i+1, *evals++);
}


/*-----------------------------------------------
|						|
|	       writelpcoefs()/2			|
|						|
+----------------------------------------------*/
void writelpcoefs(dcomplex *coef, lpinfo lppar)
{
   int	i;


   if ( (lppar.trace != dbgtrace) || ((~lppar.printout) & LPCOEFS) )
      return;

   fprintf(fdanlyz, "\n\n\nComplex %s coefficients:\n\n", lppar.label);
   fprintf(fdanlyz, "\tindex\tvalue\n");
   fprintf(fdanlyz, "\t-----\t-----\n\n");

   for (i = 0; i < lppar.ncfilt; i++)
   {
      fprintf(fdanlyz, "\t %d\t%e\t%e\n", i+1, coef->re, coef->im);
      coef += 1;
   }
}


/*-----------------------------------------------
|						|
|		  lpextend()/3			|
|						|
|  This function extends the time-domain data	|
|  in the requested direction by the requested	|
|  number of complex points.			|
|						|
+----------------------------------------------*/
/*fcomplex	 *data;	 pointer to the start of LP extrapolated data	  */
/*dcomplex *lpvector;	 pointer to the start of the LP coefficients	  */
/*lpinfo	 lppar;	 LP parameter structure			  */

static void lpextend(fcomplex  *data, dcomplex *lpvector, lpinfo lppar)
{
   int			dataprint = FALSE;
   register int		i,
			j,
			ncfpts,
			newadd;
   register float	ftmp1,
			ftmp2,
			ftmp3,
			ftmp4;
   register fcomplex	*olddata,
			*newdata;
   register dcomplex	*lpcoef;


/***************************************************
*  Calculate "ncextpts" in the time-domain data    *
*  starting from point "startextpt".  In the VNMR  *
*  parameter set, the value for "startextpt" will  *
*  range from 1 to ni for a t1 interferogram.      *
*  Internally, "startextpt" is construed to range  *
*  from 0 to ni-1.  By not taking into account     *
*  the necessary conversion between these two      *
*  conventions, "startextpt" actually specifies    *
*  the complex data point, which is NOT to be      *
*  calculated, that is right next to the first     *
*  complex point TO be calculated.                 *
***************************************************/

   if ( (lppar.trace == dbgtrace) && (lppar.printout & LPPARAM) )
   {
      fprintf(fdanlyz, "\n\nElements of LP parameter structure:\n\n");
      fprintf(fdanlyz, "\tLP.label       = %s\n", lppar.label);
      fprintf(fdanlyz, "\tLP.status      = 0x%x\n", lppar.status);
      fprintf(fdanlyz, "\tLP.ncfilt      = %d\n", lppar.ncfilt);
      fprintf(fdanlyz, "\tLP.ncupts      = %d\n", lppar.ncupts);
      fprintf(fdanlyz, "\tLP.startlppt   = %d\n", lppar.startlppt);
      fprintf(fdanlyz, "\tLP.nsignals    = %d\n", lppar.nsignals);
      fprintf(fdanlyz, "\tLP.lstrace     = %e\n", lppar.lstrace);
      fprintf(fdanlyz, "\tLP.ncextpt     = %d\n", lppar.ncextpt);
      fprintf(fdanlyz, "\tLP.startextpt  = %d\n", lppar.startextpt);
      fprintf(fdanlyz, "\tLP.printout    = 0x%x\n", lppar.printout);
      fprintf(fdanlyz, "\tLP.trace       = %d\n", lppar.trace + 1);
      fprintf(fdanlyz, "\tLP.membytes    = %d\n", lppar.membytes);
      fprintf(fdanlyz, "\tLP.index       = %d\n", lppar.index + 1);
   }

   if ( (lppar.trace == dbgtrace) && (lppar.printout & DATACALC) )
   {
      dataprint = TRUE;
      fprintf(fdanlyz, "\n\n%s %s data extension:\n\n",
		( (lppar.status & FORWARD) ? "Forward" : "Backward" ),
		lppar.label);
      fprintf(fdanlyz, "\tindex\told value\t\t\tnew value\n");
      fprintf(fdanlyz, "\t-----\t---------\t\t\t---------\n\n");
   }

   ncfpts = lppar.ncfilt;
   lpcoef = lpvector;
   newdata = data;
   newadd = ( (lppar.status & FORWARD) ? -1 : 1 );
   olddata = newdata + newadd;

#ifdef LPTEST
   lppar.startextpt += newadd;
   lppar.ncextpt += 1;
   newdata += newadd;
   olddata += newadd;
#endif 

/**********************************
*  Perform the LP extrapolation.  *
**********************************/

   for (i = 0; i < lppar.ncextpt; i++)
   {
      if (dataprint)
      {
         fprintf(fdanlyz, "\t %d\t[%e, %e]\t", lppar.startextpt -
			newadd*i, newdata->re, newdata->im);
      }

      newdata->re = newdata->im = 0.0;

      for (j = 0; j < ncfpts; j++)
      {
         ftmp1 = (float) (lpcoef->re);
         ftmp2 = (float) ((lpcoef++)->im);
         ftmp3 = olddata->re;
         ftmp4 = olddata->im;
         newdata->re += ftmp1*ftmp3 - ftmp2*ftmp4;
         newdata->im += ftmp1*ftmp4 + ftmp2*ftmp3;
         olddata += newadd;
      }

      if (dataprint)
         fprintf(fdanlyz, "[%e, %e]\n", newdata->re, newdata->im);

      olddata = newdata;
      newdata -= newadd;
      lpcoef = lpvector;
   }
}


/*---------------------------------------
|					|
|	      reverselp()/2		|
|					|
+--------------------------------------*/
static void reverselp(dcomplex *lpcoef, int nclpfilt)
{
   register int		i;
   register double	exch;
   register dcomplex	*front,
			*back;


   front = lpcoef;
   back = lpcoef + nclpfilt;

   for (i = 0; i < (nclpfilt/2); i++)
   {
      exch = (--back)->re;
      back->re = front->re;
      front->re = exch;
      exch = back->im;
      back->im = front->im;
      (front++)->im = exch;
   }
}


/*-----------------------------------------------
|						|
|		writelproots()/4		|
|						|
+----------------------------------------------*/
void writelproots(dcomplex *lpcoefs, dcomplex *lproots, lpinfo lppar, int mode)
{
   int			i;
   double		rootdiff;
   extern double	calcrootdiff();


   if ( (lppar.trace != dbgtrace) || ((~lppar.printout) & LPROOTS) )
      return;

   fprintf(fdanlyz, "\n\n\nComplex %s roots %s adjustment:\n\n",
		lppar.label, ( (mode == BEFORE_ROOTADJUST) ?
		"before" : "after" ));

   fprintf(fdanlyz, "\tindex\troot\t\t\t\tmagnitude\tdifference\n");
   fprintf(fdanlyz, "\t-----\t----\t\t\t\t---------\t----------\n\n");

   for (i = 0; i < lppar.ncfilt; i++)
   {
      rootdiff = calcrootdiff(lpcoefs, *lproots, lppar.ncfilt);
      fprintf(fdanlyz, "\t %d\t%e\t%e\t%e\t%e\n", i+1, lproots->re,
		lproots->im, cxabs(*lproots), rootdiff);
      lproots += 1;
   }
}
