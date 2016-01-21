/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef lint
   static char *Sid = "Copyright (c) Varian Assoc., Inc.  All Rights Reserved.";
#endif (not) lint

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
*  Routines related to data convertion.					*
*									*
*************************************************************************/
#include "stderr.h"
#include <sys/types.h>

static void dummy()	/* Dummy function to shut off warning message */
{
   WARNING_OFF(Sid);
}

/************************************************************************
*									*
*  Convert type of floating point data to type of char			*
*  The input data must be a floating point absolute value, and the 	*
*  resulting output data will range from 0 to max_value.		*
*									*/
void
convert_float_to_char(float *indata, 	/* input data */
		char *outdata, 		/* output result */
		int numdata,		/* number of data */
		float vs,		/* vertical scale */
		int max_value)		/* maximum data value */
{
   register float *in=indata;			/* pointer to input data */
   register u_char *out=(u_char *)outdata;	/* pointer to output data */
   register int maxval=max_value;		/* mnaximum pixel value */
   register float rvs=vs;			/* vertical scale */
   register int n = numdata;			/* number of data points */
   register u_char result;			/* result data */

   while (n--)
   {
      if ((result = (u_char)(*in++ * rvs)) > maxval)
         *out++ = maxval;
      else 
         *out++ = result;
   }
}

/************************************************************************
*									*
*  Convert floating point data to non-negative signed short integers.  	*
*  The input data must be non-negative floating point data. The 	*
*  resulting output data will be scaled by the factor "vs" and will	*
*  be clipped at "max_value". It is up to the caller to insure		*
*  that the max_value can be held in a short integer.			*
*									*/
void
convert_float_to_short(float *indata, 	/* input data array */
		short *outdata, 	/* output data array */
		int numdata,		/* number of data values in arrays */
		float vs,		/* vertical scale (positive) */
		int max_value)		/* maximum data value (<= 0xefff) */
{
   register float *in=indata;			/* pointer to input data */
   register short *out=outdata;			/* pointer to output data */
   register short maxval=(short)max_value;	/* maximum pixel value */
   register float rvs=vs;			/* vertical scale */
   register int n = numdata;			/* number of data points */
   register short result;			/* result data */

   while (n--)
   {
      if ((result = (short)(*in++ * rvs)) > maxval)
         *out++ = maxval;
      else 
         *out++ = result;
   }
}

/************************************************************************
*									*
*  Convert type of floating point data to type of int			*
*  The input data must be a floating point absolute value, and the 	*
*  resulting output data will range from 0 to max_value.		*
*									*/
void
convert_float_to_int(float *indata, 	/* input data */
		int *outdata, 		/* output result */
		int numdata,		/* number of data */
		float vs,		/* vertical scale */
		int max_value)		/* maximum data value */
{
   register float *in=indata;			/* pointer to input data */
   register u_int *out=(u_int *)outdata;	/* pointer to output data */
   register int maxval=max_value;		/* mnaximum pixel value */
   register float rvs=vs;			/* vertical scale */
   register int n = numdata;			/* number of data points */
   register u_int result;			/* result data */

   while (n--)
   {
      if ((result = (u_int)(*in++ * rvs)) > maxval)
         *out++ = maxval;
      else 
         *out++ = result;
   }
}
