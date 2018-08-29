/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef DISPLAYOPS_H
#define DISPLAYOPS_H
/*---------------------------------------------------------------
|								|
|			displayops.c				|
|								|
|     This source file contains all of the routines for		|
|     phasing, taking the absolute value, or taking the		|
|     power value of complex 1D or complex/hypercomplex		|
|     2D spectral data.						|
|								|
+--------------------------------------------------------------*/

/*-----------------------------------------------
|						|
|	           rotate2()/4			|
|						|
|   This function performs a one-dimensional	|
|   phase rotation on a single 1D spectrum or	|
|   2D trace.  The phase rotation vector is	|
|   calculated on the fly.			|
|						|
+----------------------------------------------*/
/*
 * nelems number of complex points
 * spdata pointer to spectral data
 * lpval  first-order phasing constant
 * rpval  zero-order phasing constant
 */
extern void rotate2(float *spdata, int nelems, double lpval, double rpval);

/*-----------------------------------------------
|						|
|	           rotate2_center()/4		|
|						|
|   This function performs a one-dimensional	|
|   phase rotation on a single 1D spectrum or	|
|   2D trace.  The phase rotation vector is	|
|   calculated on the fly and lp is centered	|
|   about zero freq instead of sw/2 like	|
|   rotate2().					|
|						|
+----------------------------------------------*/
extern void rotate2_center(float *spdata, int nelems, double lpval, double rpval);

/*-----------------------------------------------
|						|
|	           rotate4()/9			|
|						|
|   This function performs a one-dimensional	|
|   phase rotation on a 2D hypercomplex trace	|
|   to yield the Real component for display.	|
|   The phasing vector is calculated on the	|
|   fly.					
|						|
+----------------------------------------------*/
/* nelems	 number of complex points     		*/
/* reverseflag	 F2 display = 0; F1 display = 1		*/
/* trace	 2D trace #					*/
/* nblockelems	 number of elements along the block dimension	*/
/* spdata	 pointer to un-phased, complex, spectral data	*/
/* lpval	 first-order phasing constant	(linear)	*/
/* rpval	 zero-order phasing constant	(linear)	*/
/* lp1val	 first-order phasing constant (block)		*/
/* rp1val        zero-order phasing constant  (block)		*/
extern void rotate4(float *spdata, int nelems, double lpval, double rpval,
             double lp1val, double rp1val, int nblockelems,
	     int trace, int reverseflag);


/*-----------------------------------------------
|                                               |
|            rotate4_center()/9                 |
|                                               |
|   This function performs a one-dimensional    |
|   phase rotation on a 2D hypercomplex trace   |
|   to yield the Real component for display.    |
|   The phasing vector is calculated on the     |
|   fly and lp is centered about zero frequency.|
|                                               |
+----------------------------------------------*/

/* nelems        number of complex points               */
/* reverseflag   F2 display = 0; F1 display = 1         */
/* trace         2D trace #                                     */
/* nblockelems   number of elements along the block dimension   */
/* spdata        pointer to un-phased, complex, spectral data   */
/* lpval         first-order phasing constant   (linear)        */
/* rpval         zero-order phasing constant    (linear)        */
/* lp1val        first-order phasing constant (block)           */
/* rp1val        zero-order phasing constant  (block)           */
extern void rotate4_center(float *spdata, int nelems, double lpval, double rpval,
             double lp1val, double rp1val, int nblockelems,
             int trace, int reverseflag);

/*-----------------------------------------------
|						|
|	        blockrotate2()/9		|
|						|
|   This function performs a one-dimensional	|
|   block phase rotation on complex 1D or 2D	|
|   data.					|
|						|
+----------------------------------------------*/
/* nelems	number of "type" data elements		*/
/* ntraces	number of spectral traces			*/
/* nblocks	number of 2D data blocks			*/
/* dataskip	2 for complex data; 4 for hypercomplex data	*/
/* imagskip	1 for F1 phasing; 2 for F2 phasing		*/
/* blocknum	block number of data in 2D data file		*/
/* modeflag	F2 phase = 0 or F1 phase = 1			*/
/* spdata	pointer to complex or hypercomplex 2D data	*/
/* phasevector	pointer to complex phase rotation vector	*/
extern void blockrotate2(float *spdata, float *phasevector, int nblocks, int blocknum,
          int nelems, int ntraces, int dataskip, int imagskip, int modeflag);

/*-----------------------------------------------
|						|
|	        blockrotate4()/7		|
|						|
|   This function performs a two-dimensional	|
|   block phase rotation on hypercomplex 2D	|
|   spectral data.				|
|						|
+----------------------------------------------*/
/* nelems	number of "type" data elements		*/
/* ntraces	number of spectral traces			*/
/* nblocks	number of 2D data blocks			*/
/* blocknum	block number of data in 2D data file		*/
/* spdata	pointer to hypercomplex 2D data		*/
/* f1phsvector	pointer to F1 complex phase rotation vector	*/
/* f2phsvector	pointer to F2 complex phase rotation vector	*/
extern void blockrotate4(float *spdata, float *f1phsvector, float *f2phsvector,
           int nblocks, int blocknum, int nelems, int ntraces);

/*-----------------------------------------------
|						|
|		   phase2()/5			|
|						|
|   This function performs a one-dimensional	|
|   phasing operation on either a 1D spectrum	|
|   or a 2D trace to yield the Real component	|
|   for display.  The phasing vector is cal-	|
|   culated on the fly.  "fromdata" can be	|
|   the same as "todata" in this routine.	|
|						|
+----------------------------------------------*/
extern void phase2(float *fromdata, float *todata, int nelems, double lpval, double rpval);


/*-----------------------------------------------
|						|
|		   phase4()/10			|
|						|
|   This function performs a one-dimensional	|
|   phasing operation on a 2D hypercomplex	|
|   trace to yield the Real component for	|
|   display.  The phasing vector is calculated	|
|   on the fly.  "fromdata" can be the same	|
|   as "todata" in this routine.		|
|						|
+----------------------------------------------*/
/* nelems	number of complex points     		*/
/* reverseflag	F2 display = 0; F1 display = 1		*/
/* trace	2D trace #					*/
/* nblockelems	number of elements along the block dimension	*/
/* fromdata	pointer to unphased, complex spectral data	*/
/* todata	pointer to phased, real spectral data	*/
/* lpval	first-order phasing constant	(linear)	*/
/* rpval	zero-order phasing constant	(linear)	*/
/* lp1val	first-order phasing constant (block)		*/
/* rp1val	zero-order phasing constant  (block)		*/
extern void phase4(float *fromdata, float *todata, int nelems, double lpval, double rpval,
        double lp1val, double rp1val, int nblockelems, int trace, int reverseflag);


/*-----------------------------------------------
|						|
|	        blockphase2()/11		|
|						|
|   This function performs a one-dimensional	|
|   phasing operation on 2D complex spectral	|
|   data to yield the ReRe component for	|
|   display.  "fromdata" can be the same as	|
|   "todata" in this routine.			|
|						|
+----------------------------------------------*/
/* nelems	number of complex elements			*/
/* ntraces	number of spectral traces			*/
/* nblocks	number of 2D data blocks			*/
/* dataskip	2 for complex data; 4 for hypercomplex data	*/
/* imagskip	1 for F1 phasing; 2 for F2 phasing		*/
/* blocknum	block number of data in 2D data file		*/
/* modeflag	F2 phase = 0 or F1 phase = 1			*/
/* zimag	flag for zeroing imaginary part of "todata"	*/
/* fromdata	pointer to unphased, complex spectral data	*/
/* todata	pointer to phased, real spectral data	*/
/* phasevector	pointer to complex phase rotation vector	*/
extern void blockphase2(float *fromdata, float *todata, float *phasevector, int nblocks,
       int blocknum, int nelems, int ntraces, int dataskip, int imagskip,
       int modeflag, int zimag);

/*-----------------------------------------------
|						|
|	        blockphaseangle2()/11		|
|						|
|   This function performs a one-dimensional	|
|   phasing operation on 2D complex spectral	|
|   data to yield the ReRe component for	|
|   display, then performs atan2() on x,y pairs.|
|   "fromdata" can be the same as "todata" 	|
|   in this routine.				|
|						|
+----------------------------------------------*/
/* nelems	number of complex elements			*/
/* ntraces	number of spectral traces			*/
/* nblocks	number of 2D data blocks			*/
/* dataskip	2 for complex data; 4 for hypercomplex data	*/
/* imagskip	1 for F1 phasing; 2 for F2 phasing		*/
/* blocknum	block number of data in 2D data file		*/
/* modeflag	F2 phase = 0 or F1 phase = 1			*/
/* zimag	flag for zeroing imaginary part of "todata"	*/
/* fromdata	pointer to unphased, complex spectral data	*/
/* todata	pointer to phased, real spectral data	*/
/* phasevector	pointer to complex phase rotation vector	*/
extern void blockphaseangle2(float *fromdata, float *todata, float *phasevector,
         int nblocks, int blocknum, int nelems, int ntraces, int dataskip,
         int imagskip, int modeflag, int zimag);

/*-----------------------------------------------
|						|
|		 blockphase4()/8		|
|						|
|   This function performs a two-dimensional	|
|   phasing operation on 2D hypercomplex	|
|   spectral data to yield the ReRe component	|
|   for display.  "fromdata" can be the same	|
|   as "todata" in this routine.		|
|						|
+----------------------------------------------*/
/* nelems	number of hypercomplex points		*/
/* ntraces	number of spectral traces			*/
/* nblocks	number of 2D data blocks			*/
/* blocknum	block number of data in 2D data file		*/
/* fromdata	pointer to unphased, hypercomplex data	*/
/* todata	pointer to phased, real spectral data	*/
/* f1phsvector	pointer to F1 complex phase rotation vector	*/
/* f2phsvector	pointer to F2 complex phase rotation vector	*/
extern void blockphase4(float *fromdata, float *todata, float *f1phsvector, float *f2phsvector,
           int nblocks, int blocknum, int nelems, int ntraces);

/*-----------------------------------------------
|						|
|		 phasefunc()/4			|
|						|
|   This function calculates a complex phasing	|
|   vector based upon the number of complex or	|
|   hypercomplex data points, the first-order	|
|   phasing constant (lp), and the zero-order	|
|   phasing constant (rp).			|
|						|
+----------------------------------------------*/
extern void phasefunc(float *phasepntr, int npnts, double lpval, double rpval);

/*-----------------------------------------------
|						|
|		blockphsabs4()/9		|
|						|
|   This function performs a two-dimensional	|
|   phasing/absolute value operation on 2D	|
|   hypercomplex spectral data to yield the	|
|   ReS component for display.  "fromdata"	|
|   cannot be the same as "todata" in this	|
|   routine.					|
|						|
+----------------------------------------------*/
extern void blockphsabs4(float *fromdata, float *todata, float *phasevector,
        int nblocks, int blocknum, int nelems, int ntraces, int imagskip, int modeflag);


/*-----------------------------------------------
|                                               |
|               blockphspwr4()/9		|
|                                               |
|   This function performs a two-dimensional    |
|   phasing/power operation on 2D hypercomplex	|
|   spectral data to yield the ReS component	|
|   for display.  "fromdata" cannot be the	|
|   same as "todata" in this routine.		|
|						|
+----------------------------------------------*/
/* int	nblocks,	number of 2D data blocks			*/
/* 	blocknum,	block number of data in 2D data file		*/
/* 	nelems,		number of hypercomplex elements		*/
/* 	ntraces,	number of spectral traces			*/
/* 	imagskip,	1 for F1 phasing; 2 for F2 phasing		*/
/* 	modeflag;	F2 phase = 0 or F1 phase = 1			*/
/* float *fromdata,	pointer to raw, hypercomplex spectral data	*/
/* 	*todata,	pointer to real spectral data		*/
/* 	*phasevector;	pointer to complex phase rotation vector	*/
extern void blockphspwr4(float *fromdata, float *todata, float *phasevector,
                  int nblocks, int blocknum, int nelems,
		  int ntraces, int imagskip, int modeflag);


/*-----------------------------------------------
|						|
|	          absval2()/7			|
|						|
|   This function computes the one-dimensional	|
|   absolute value spectrum from either a com-	|
|   plex or hypercomplex data set.		|
|						|
+----------------------------------------------*/
/* dataskip	2 for complex data; 4 for hypercomplex data	*/
/* imagskip	1 for F1 absval; 2 for F2 absval		*/
/* destskip	skip for destination pointer			*/
/* nelems	number of complex points			*/
/* zimag	flag for zeroing imaginary part of "todata"	*/
/* fromdata	pointer to source data			*/
/* todata	pointer to destination data			*/
extern void absval2(float *fromdata, float *todata, int nelems, int dataskip, int imagskip,
		int destskip, int zimag);

/*-----------------------------------------------
|						|
|		   absval4()/3			|
|						|
|   This function computes the absolute value	|
|   spectrum from a 2D hypercomplex data set.	|
|						|
|	S  =  sqrt(RR**2 + RI**2 + IR**2 +	|
|			II**2)			|
|						|
+----------------------------------------------*/
extern void absval4(float *fromdata, float *todata, int nelems);

/*-----------------------------------------------
|						|
|	          dbmval2()/7			|
|						|
|   This function computes the one-dimensional	|
|   power value spectrum in dbm set so that 0 dbm
|   has a value of 140  Data may be from either a com-	|
|   plex or hypercomplex data set.		|
|						|
+----------------------------------------------*/
extern void dbmval2(float *fromdata, float *todata, int nelems, int dataskip, int imagskip,
		int destskip, int zimag);

/*-----------------------------------------------
|						|
|	           pwrval2()/7			|
|						|
|   This function computes the one-dimensional	|
|   power spectrum from either a complex or	|
|   hypercomplex data set.			|
|						|
+----------------------------------------------*/
/* int	dataskip,	2 for complex data; 4 for hypercomplex data	*/
/*     imagskip,	1 for F1 pwrval; 2 for F2 pwrval		*/
/* 	destskip,	skip for destination pointer			*/
/* 	nelems,		number of complex points			*/
/* 	zimag;		flag for zeroing imaginary part of "todata"	*/
/* float *fromdata,	pointer to source data			*/
/* 	*todata;	pointer to destination data			*/
extern void pwrval2(float *fromdata, float *todata, int nelems, int dataskip,
             int imagskip, int destskip, int zimag);

/*-----------------------------------------------
|						|
|	           phaseangle2()/9		|
|						|
|   This function computes the one-dimensional	|
|   power spectrum from either a complex or	|
|   hypercomplex data set.			|
|						|
+----------------------------------------------*/
extern void phaseangle2(float *fromdata, float *todata, int nelems, int dataskip, int imagskip,
		int destskip, int zimag, double lpval, double rpval);

/*-----------------------------------------------
|						|
|	          pwrval4()/3			|
|						|
|   This function computes the power spectrum	|
|   from a 2D hypercomplex data set.		|
|						|
|	  R(F1)  =  RR**2  +  RI**2		|
|	  I(F1)  =  IR**2  +  II**2		|
|						|
|	  R(F2)  =  RR**2  +  IR**2		|
|	  I(F2)  =  RI**2  +  II**2		|
|						|
|	  S  =  0.5 * [R(F1)**2 + I(F1)**2	|
|		  + R(F2)**2 + I(F2)**2]	|
|						|
|	     =  (RR**2 + II**2)(RI**2 + IR**2)	|
|		+ RR**4 + RI**4 + IR**4 +	|
|		II**4				|
|						|
+----------------------------------------------*/
extern void pwrval4(float *fromdata, float *todata, int nelems);


/*-----------------------------------------------
|						|
|	        blockpwrabs()/4			|
|						|
|   This function performs a mixed-mode (power	|
|   and absolute value) calculation on 2D	|
|   hypercomplex spectra data.			|
|						|
|						|
|	AV1 PWR:				|
|						|
|	    R(F1)  =  RR**2  +  RI**2		|
|	    I(F1)  =  IR**2  +  II**2		|
|						|
|	    S  =  sqrt[R(F1)**2 + I(F1)**2]	|
|						|
|						|
|	PWR1 AV:				|
|						|
|	    R(F2)  =  RR**2  +  IR**2		|
|	    I(F2)  =  RI**2  +  II**2		|
|						|
|	    S  =  sqrt[R(F2)**2 + I(F2)**2]	|
|						|
+----------------------------------------------*/
extern void blockpwrabs(float *fromdata, float *todata, int nelems, int pwrskip);

#endif
