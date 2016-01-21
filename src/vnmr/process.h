/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef _PROCESS_H_
#define _PROCESS_H_

/*-----------------------------------------------
|						|
|   process.h   -   processing header file 	|
|						|
|			Author:  S. Farmer	|
|			  Date:  6-8-90		|
+----------------------------------------------*/

/*---------------
|  Definitions  |
+--------------*/

/*********
*  Data  *
*********/

#ifndef FT3D
#define CMPLX_t2       0x0
#define REAL_t2        0x1
#define CMPLX_t1       0x0
#define REAL_t1        0x2
#endif 


/***************
*  Processing  *
***************/

#define MINDEGREE      0.005   /* in degrees			 */

#define FTNORM	       5.0e-7  /* FT scaling factor		 */
#define MAX2DARRAY     10      /* maximum fid's per 2D increment */
#define FT_F2PROC      0x4     /* FT processing for F2 dimen.    */
#define LP_F2PROC      0x8     /* LP processing for F2 dimen.    */
#define MEM_F2PROC     0x20    /* MEM processing for F2 dimen.   */
#define HT_F2PROC      0x40    /* HT processing for F2 dimen.    */
#define FT_F1PROC      0x1000  /* FT processing for F1 dimen.    */
#define LP_F1PROC      0x2000  /* LP processing for F1 dimen.    */
#define HT_F1PROC      0x4000  /* HT processing for F1 dimen.    */
#define MEM_F1PROC     0x8000  /* MEM processing for F1 dimen.   */

#define CMPLX	       0x1
#define FT_PROC	       (FT_F2PROC|FT_F1PROC)
#define LP_PROC	       (LP_F2PROC|LP_F1PROC)
#define MEM_PROC       (MEM_F2PROC|MEM_F1PROC)
#define HT_PROC        (HT_F2PROC|HT_F1PROC)

#define LP_LABEL_SIZE  5

/***********************
* 3D Display Constants *
***********************/

#define PHMODE		0x1
#define AVMODE		0x2
#define PWRMODE		0x4
#define PAMODE		0x8

/******************
*  LP Algorithms  *
******************/

#define LPFFT		0x1	/* LP + FFT processing option	*/
#define LPARFFT		0x2	/* LPAR + FFT processing option	*/


/***************
*  LP Options  *
***************/

#define LPACTIVE       0x1000  /* LP processing is active	 */
#define FORWARD	       0x2000  /* Forward LP prediction		 */


/****************
*  LP Printout  *
****************/

#define LPMATRIX    0x1     /* for external analysis of LP vector          */
#define LPCOEFS     0x2     /* for external analysis of LP coefficients    */
#define LPROOTS     0x4     /* for external analysis of LP roots	   */
#define DATACALC    0x8     /* for external analysis of LP data extension  */
#define LPPARAM     0x10    /* printout of LP parameter structure	   */

#define LPPRT_OPTS     0x1f

/*************
*  LP modes  *
*************/

#define NO_LPALLOC	0
#define LPALLOC		1

/***********************
*  Other LP Constants  *
***********************/

#define BEFORE_ROOTADJUST	0
#define AFTER_ROOTADJUST	1


/*--------------
|  Structures  |
+-------------*/
struct _fcomplex
{
   float	re;
   float	im;
};

typedef struct _fcomplex         fcomplex;

struct _dcomplex
{
   double	re;
   double	im;
};

typedef struct _dcomplex	dcomplex;


struct _lpinfo
{
   double	lstrace;
   dcomplex	*(*lpcoef_solve)();
   dcomplex	*lppntr;
   void		(*lpmat_setup)();
   int		membytes;
   int		status;
   int		ncfilt;
   int		startlppt;
   int		ncextpt;
   int		startextpt;
   int		ncupts;
   int		nsignals;
   int		printout;
   int		trace;
   int		index;
   char		label[5];
};

typedef struct _lpinfo          lpinfo;


struct _lpstruct
{
   lpinfo	*parLP;
   dcomplex	*lppntr;
   int		membytes;
   int		sizeLP;
};

typedef struct _lpstruct	lpstruct;


struct _hypercomplex
{
   float        rere;
   float        reim;
   float        imre;
   float        imim;
};

typedef struct _hypercomplex    hypercmplx;


#ifndef FT3D
struct _coefs
{
   fcomplex	re2d;
   fcomplex	im2d;
};

typedef struct _coefs		coefs;
#endif

#endif

