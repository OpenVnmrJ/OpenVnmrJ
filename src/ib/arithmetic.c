/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
static char *Sid(){
    return "@(#)arithmetic.c 18.1 03/21/08 (c)1991-93 SISCO";
}

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Routines related to arithmetic operation:				*
*   	Addition							*
*	Subtraction							*
*	Multiplication							*
*	Division							*
*									*
*************************************************************************/
#include <math.h>
#include <sys/types.h>
#include "stderr.h"
#include "process.h"


/************************************************************************
*									*
*  Add, subtract, multiply or divide floating point data with
*  a constant.
*  Return SUCCESS or ERROR.						*
*									*/
int
arith_fadd_image_const(float *src, float val, float *dst, int n, char *err)
{
   register float *psrc = src;		/* pointer to source */
   register float *pdst = dst;		/* pointer to destination */
   register int ndata = n;		/* number of data */
   register float value = val;		/* value in register */

   if (ndata < 1)
   {
      if (err)
	 (void)sprintf(err, "arith_fadd_image_const: Number of pixels is < 1");
      return(ERROR);
   }
   while (ndata--)
   {
       *pdst++ = *psrc++ + val;
   }
   return(SUCCESS);
}

int
arith_fsub_image_const(float *src, float val, float *dst, int n, char *err)
{
   register float *psrc = src;		/* pointer to source */
   register float *pdst = dst;		/* pointer to destination */
   register int ndata = n;		/* number of data */
   register float value = val;		/* value in register */

   if (ndata < 1)
   {
      if (err)
	 (void)sprintf(err, "arith_fsub_image_const: Number of pixels is < 1");
      return(ERROR);
   }
   while (ndata--)
   {
       *pdst++ = *psrc++ - val;
   }
   return(SUCCESS);
}

int
arith_fmul_image_const(float *src, float val, float *dst, int n, char *err)
{
   register float *psrc = src;		/* pointer to source */
   register float *pdst = dst;		/* pointer to destination */
   register int ndata = n;		/* number of data */
   register float value = val;		/* value in register */

   if (ndata < 1)
   {
      if (err)
	 (void)sprintf(err, "arith_fmul_image_const: Number of pixels is < 1");
      return(ERROR);
   }

   while (ndata--)
   {
      *pdst++ = *psrc++ * val;
   }
   return(SUCCESS);
}

int
arith_fdiv_image_const(float *src, float val, float *dst, int n, char *err)
{
   register float *psrc = src;		/* pointer to source */
   register float *pdst = dst;		/* pointer to destination */
   register int ndata = n;		/* number of data */
   register float value = val;		/* value in register */

   if (ndata < 1)
   {
      if (err)
	 (void)sprintf(err,"arith_fdiv_image_const: Number of pixels is < 1");
      return(ERROR);
   }

   if (val == 0){
       if (err){
	   (void)sprintf(err,"arith_fdiv_image_const: Division by zero!");
       }
       return ERROR;
   }

   while (ndata--)
   {
      *pdst++ = *psrc++ / val;
   }
   return(SUCCESS);
}


/************************************************************************
*									*
*  Add, subtract, multiply or divide two images with floating point data.
*  Return SUCCESS or ERROR.						*
*									*/
int
arith_fadd_images(float *src1, float *src2, float *dst, int n, char *err)
{
   register float *psrc1 = src1;	/* pointer to source */
   register float *psrc2 = src2;	/* pointer to source */
   register float *pdst = dst;		/* pointer to destination */
   register int ndata = n;		/* number of data */

   if (ndata < 1)
   {
      if (err)
	 (void)sprintf(err, "arith_fadd_images: Number of pixels is < 1");
      return(ERROR);
   }
   while (ndata--)
   {
      *pdst++ = *psrc1++ + *psrc2++ ;
   }
   return(SUCCESS);
}

int
arith_fsub_images(float *src1, float *src2, float *dst, int n, char *err)
{
   register float *psrc1 = src1;	/* pointer to source */
   register float *psrc2 = src2;	/* pointer to source */
   register float *pdst = dst;		/* pointer to destination */
   register int ndata = n;		/* number of data */

   if (ndata < 1)
   {
      if (err)
	 (void)sprintf(err, "arith_fsub_images: Number of pixels is < 1");
      return(ERROR);
   }
   while (ndata--)
   {
       *pdst++ = *psrc1++ - *psrc2++;
   }
   return(SUCCESS);
}

int
arith_fmul_images(float *src1, float *src2, float *dst, int n, char *err)
{
   register float *psrc1 = src1;	/* pointer to source */
   register float *psrc2 = src2;	/* pointer to source */
   register float *pdst = dst;		/* pointer to destination */
   register int ndata = n;		/* number of data */

   if (ndata < 1)
   {
      if (err)
	 (void)sprintf(err, "arith_fmul_images: Number of pixels is < 1");
      return(ERROR);
   }
   while (ndata--)
   {
      *pdst++ = *psrc1++ * *psrc2++;
   }
   return(SUCCESS);
}

int
arith_fdiv_images(float *src1, float *src2, float *dst, int n, char *err)
{
   register float *psrc1 = src1;	/* pointer to source */
   register float *psrc2 = src2;	/* pointer to source */
   register float *pdst = dst;		/* pointer to destination */
   register int ndata = n;		/* number of data */
#ifdef SOLARIS
   float inf = HUGE_VAL;
#else
   float inf = HUGE;
#endif

   if (ndata < 1)
   {
      if (err)
	 (void)sprintf(err, "arith_fmul_images: Number of pixels is < 1");
      return(ERROR);
   }
   while (ndata--)
   {
      if (*psrc2 == 0)
      {
	  if (*psrc1 < 0){
	      *pdst++ = -inf;
	  }else{
	      *pdst++ = inf;
	  }
	  psrc1++;
	  psrc2++;
      }else{
         *pdst++ = *psrc1++ / *psrc2++;
      }
   }
   return(SUCCESS);
}
