/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/****************************************************************/
/*								*/
/* fft.c - fast Fourier Transform Routines		 	*/
/*								*/
/****************************************************************/

#include <stdio.h>
#include <math.h>
#include "allocate.h"
#include "wjunk.h"

/*  VMS does not define M_PI in its math.h file */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif 

/* #include <sys/time.h> */

#define FALSE		0
#define TRUE		1
#define COMPLEX		2
#define HYPERCOMPLEX	4

static int	fsinefn = 0;
static float	*fsinetable;


/*---------------------------------------
|					|
|	       diff_time()/3		|
|					|
|   This function prints the time	|
|   differences.			|
|					|
+--------------------------------------*/
/*
diff_time(xx, tp1, tp2)
char		*xx;
struct timeval	tp1,
		tp2; 
{
   double	acc1,
		acc2;
   printf("%s ", xx);
   acc1 = 1e6*tp1.tv_sec + tp1.tv_usec;
   acc2 = 1e6*tp2.tv_sec + tp2.tv_usec;
   acc2 -= acc1;
   acc2 *= 1e-6;
   printf("duration is %g\n", acc2);
}
*/


/*-----------------------------------------------
|						|
|		     fft()/9			|
|						|
|   This function performs an FFT or I-FFT on	|
|   the input data.				|
|						|
|   Ift must have 0 frequency in zero element   |
+----------------------------------------------*/
/* fn		number of complex points */
/* level	number of FT levels */
/* zfratio	zero-filling ratio */
/* skip		skip factor */
/* datatype	type of data:  complex or hypercomplex */
/* nfidpts	TEMPORARY */
int fft(float *data, int fn, int level, int zfratio, int skip, int datatype, double sign,
        double fnorm, int nfidpts)
{
   int			ngrp,
			ip,
			ig,
			wdelta,
			igrp,
			nozf = FALSE,	/* proper initialization */
			save_zfratio;
   register int		i,
			igrp2,
			tonextreal,
			imagskip;
   float		f3r,
			f3i;
   register float	*data0,
			*data1,
			*wpnt,
			t_re,
			t_im,
			w_re,
			w_im,
			w_c,
			f2r,
			f2i;

/*
   struct timeval	tp1,
			tp2,
			tp3;

   gettimeofday(&tp1, NULL); */


   if ((fsinefn < fn) || ((fsinefn > 65000) && (fn < fsinefn)))
   {
      releaseAllWithId("sinetab");
      if ((fsinetable = (float *)allocateWithId(fn*2*sizeof(float),
		"sinetab")) == NULL)
      {
         Werrprintf("cannot allocate sine table buffer");
         return 1;
      }

      w_re = M_PI/((float) fn);
      t_re = w_re;
      igrp2 = fn/2;
      fsinetable[0] = 1.0;
      fsinetable[1] = 0.0;
      fsinetable[2*igrp2] = 0.0;
      fsinetable[2*igrp2+1] = -1.0;
      for (i = 1; i < igrp2; i++) 
      { 
	 t_re = w_re * (float) i;
         t_im = cos(t_re);
         fsinetable[2*i] = t_im;
         fsinetable[2*(igrp2 - i) +1] = -t_im;
	 fsinetable[2*(fn-i)] = -t_im;
         fsinetable[2*(igrp2 + i) +1] = -t_im;
      }

      fsinefn = fn;
   }

   if (sign > 0.0)	/* for a back transform, invert sinetable sign */
   {
      data0 = fsinetable + 1;
      zfratio = 0;
      for (i = 0; i < fsinefn; i++) 
      {
         *data0 *= -1.0;
	 data0 += 2;
      }
   }

/***********************************************************
*  All DFT passes are performed except for the last two.   *
*  A radix-four last pass is used to scale and rotate the  *
*  data.  The loops in the FFT are inside out in order to  *
*  minimize the number of complex multiplications.  The    *
*  bit reversal occurs after the DFT.                      *
***********************************************************/

   save_zfratio = zfratio;
   if (zfratio == 0)
   {
      nozf = TRUE;
      zfratio++;
   }

   tonextreal = skip;
   imagskip = skip/2;
   wdelta = 4*fsinefn/fn;
   igrp = fn/2;
   ngrp = 1;

   for (ip = 0; ip < (level - 2); ip++)
   {
      igrp2 = igrp*tonextreal;
      wpnt = fsinetable;

/*******************************
* Initialize indices for dft.  *
*******************************/

      data0 = data;
      data1 = data0 + igrp2;

/********************************************************
*  This is the outer loop on DFT's within a group.  If  *
*  zero filling has been used, one can take advantage   *
*  of knowing where zeros are to minimize the computa-  *
*  tional effort.                                       *
********************************************************/

      for (ig = 0; ig < (igrp/zfratio); ig++)
      {

/**********************************************
*  The innter DFT loop across the groups is   *
*  performed.  Rotate the result at the end.  *
**********************************************/

         w_re = *wpnt;
         w_im = *(wpnt + 1); 
         if ((zfratio >= 1) && (!nozf))
         {
            for (i = 0; i < ngrp; i++)
            {
               t_re = *data0;
               t_im = *(data0 + imagskip);
               *data1 = t_re * w_re - t_im * w_im;
               *(data1 + imagskip) =  t_re * w_im + t_im * w_re;
               data0 = data1 + igrp2;
               data1 = data0 + igrp2;
            }
         }
         else
         {
	    for (i = 0; i < ngrp; i++)
	    {
	       t_re = *data0 - *data1;
	       t_im = *(data0 + imagskip) - (*(data1 + imagskip));
	       *data0 += *data1;
	       *(data0 + imagskip) += *(data1 + imagskip);
	       *data1 = t_re * w_re - t_im * w_im;
	       *(data1 + imagskip) = t_re * w_im + t_im * w_re;
	       data0 = data1 + igrp2;
	       data1 = data0 + igrp2;
	    }
         }

/*******************************************************
*  Reset indices for next pass and rotate the vector.  *
*******************************************************/

         data0 = data + (ig + 1)*tonextreal;
	 data1 = data0 + igrp2;
         wpnt += wdelta;
      }

/****************************************************
*  This is the end of the pass.  Set up next pass.  *
****************************************************/

      /* wdelta *= 2; */
      wdelta = wdelta << 1;

      /* igrp /= 2; */
      igrp = igrp >> 1;

      /* ngrp *= 2; */
      ngrp = ngrp << 1;

      if (zfratio > 1)
      {
         /* zfratio /= 2; */
	 zfratio = zfratio >> 1;
      }
      else
      {
         nozf = TRUE;
      }
   }


/********************************************
*  Now do the last pass and scale the data  *
*  by "fnorm".				    *
********************************************/

   w_c = ( (sign > 0.0) ? 1/(fn*fnorm) : fnorm );
   data0 = data;
   igrp2 = fn/4;

   for (ip = 0; ip  < igrp2; ip++)
   {	
      /* scaling is done here */
      data1 = data0;
      t_re = w_c * (*data1); /* preserves f0r */
      data1 += imagskip;
      t_im = w_c * (*data1); /* preserves f0i */
      data1 += imagskip;
      w_re = w_c * (*data1); /* preserves f1r */
      data1 += imagskip;
      w_im = w_c * (*data1); /* preserves f1i */
      data1 += imagskip;
      f2r = w_c * (*data1);
      data1 += imagskip;
      f2i = w_c * (*data1);
      data1 += imagskip;
      f3r = w_c * (*data1);
      data1 += imagskip;
      f3i = w_c * (*data1);
      data1 = data0;

      if (sign < 0.0)
      {
      /* 2 0 3 1 */
      /* rotates 0 frequency to center implicitly */
	 *data1 = t_re - w_re + f2r - f3r; /* f2r */
         data1 += imagskip;
         *data1 = t_im - w_im + f2i - f3i; /* f2i */
         data1 += imagskip;
         *data1 = t_re + w_re + f2r + f3r; /* f0r */
         data1 += imagskip;
         *data1 = t_im + w_im + f2i + f3i; /* f0i */
         data1 += imagskip;
         *data1 = t_re - w_im - f2r + f3i; /* f3r */
         data1 += imagskip;
         *data1 = t_im + w_re - f2i - f3r; /* f3i */
         data1 += imagskip;
         *data1 = t_re + w_im - f2r - f3i; /* f1r */
         data1 += imagskip;
         *data1 = t_im - w_re - f2i + f3r; /* f1i */
         data1 += imagskip;
      }
      else
      {
      /* inverse radix 4 */
      /* 0 2 1 3 */
         *data1 = t_re + w_re + f2r + f3r; /* f0r */
         data1 += imagskip;
         *data1 = t_im + w_im + f2i + f3i; /* f0i */
         data1 += imagskip;
         *data1 = t_re - w_re + f2r - f3r; /* f2r */
         data1 += imagskip;
         *data1 = t_im - w_im + f2i - f3i; /* f2i */
         data1 += imagskip;
         *data1 = t_re - w_im - f2r + f3i; /* f1r */
         data1 += imagskip;
         *data1 = t_im + w_re - f2i - f3r; /* f1i */
         data1 += imagskip;
         *data1 = t_re + w_im - f2r - f3i; /* f3r */
         data1 += imagskip;
         *data1 = t_im - w_re - f2i + f3r; /* f3i */
         data1 += imagskip;
      }

      data0 = data1;
   }

/*****************************************************
*  The FFT is now complete but the data array is in  *
*  base 2 scrambled order.  Unscramble the array.    *
*****************************************************/

   /* gettimeofday(&tp3,  NULL); */

   igrp = 0;
   for (i = 0; i < fn; i++)
   { 
      if (i < igrp)
      {
         data1 = data + tonextreal*igrp;
         data0 = data + tonextreal*i;
         t_re = *data0; 
         *data0 = *data1;
         data0 += imagskip;
         *data1 = t_re;
         data1 += imagskip;
         t_re = *data0; 
         *data0 = *data1;
         *data1 = t_re;
      }

      ngrp = fn >> 1;
      while (igrp >= ngrp) 
      {
         igrp -= ngrp;
         ngrp = (ngrp + 1) >> 1; 
      }

      igrp += ngrp;
   }

   data0 = data;
   if (sign > 0.0)
   { 
      data0 = fsinetable + 1;
/* for a back transform, invert sinetable sign */
      for (i = 0; i < fsinefn; i++) 
      {
         *data0 *= -1.0;
	 data0 += 2;
      }
   }

   if (datatype == HYPERCOMPLEX)
   {
      fft(data + 1, fn, level, save_zfratio, HYPERCOMPLEX, COMPLEX, sign,
		fnorm, nfidpts);
   }

/*
   gettimeofday(&tp2,  NULL);
   diff_time("fft ", tp1, tp3);
   diff_time("bitr ", tp3, tp2);
   diff_time("total ", tp1, tp2);
*/

   return 0;
}
