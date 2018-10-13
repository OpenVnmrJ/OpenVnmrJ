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
*  Routine related to statistics.					*
*									*
*************************************************************************/
#include <math.h>
#include "stderr.h"

/************************************************************************
*                                                                       *
*  Calculate statistics information for: min, max, area, median, mean,  *
*  and stdv, given historgram pixel values.                             *
*                                                                       *
*  Retun SUCCES or ERROR.                                               *
*                                                                       */
int
pro_statistics(int *histogram,	/* histogram containing pixel value */
	int num_level,		/* number of level in histogram */
	int *min,		/* result minimum pixel value */
	int *max,		/* result maximum pixel value */
	int *area,		/* result accumulative # pixels in histogram */
	int *median,		/* result median pixel value */
	double *mean,		/* result mean value */
	double *stdv,		/* result standard deviation */
	char *errmsg)		/* error message buffer */
{
   register int *hist=histogram;	/* pointer to histogram */
   register int i;		/* loop counter */
   register int sum;		/* # of pixels in histogram */
   register int total_pix;	/* accumulative of all pixel value */
   register int total_pix_sq;	/* summation of index square times # hist */

   /* Find minimum pixel value.  That is the lowest index of histogram  */
   /* which contains pixel value.					*/
   for (i=0; (i<num_level); i++);

   for (i=0; ((i<num_level) && (!(hist[i]))); i++);
   *min = i;

printf ( "Statistics: min = %d\n", i ) ;

   if (*min == num_level)
   {
      if (errmsg)
         (void)sprintf(errmsg, "pro_statistics: No single pixel in histogram");
      WARNING_OFF(Sid);
      return(ERROR);
   }

   /* Find Maximum pixel value.  That is the highest index of histogram */
   /* which contains pixel value.					*/
   for (i=num_level-1; (i>=0) && !hist[i]; i--);
   *max = i;

printf ( "Statistics: max = %d\n", i ) ;

   /* Find mean pixel value */
   sum = 0;
   total_pix = 0;
   for (i=0; i<num_level; i++)
   {
      sum += hist[i];
      total_pix += hist[i] * i;
   }
   *mean = (double)total_pix / (double)sum;

   /* Area */
   *area = sum;

   /* Find median */
   sum = 0;
   for (i=0; i<num_level; i++)
   {
      sum += hist[i];
      if ((sum * 2) > *area)
	 break;
   }
   *median = i;

   /* Find standard deviation */
   /* stdv = sqrt( #(x*x) - mean * #(x)) / (n -1 ) */
   /*   where # = sum all term                     */
   /*         n = area                             */
   total_pix_sq = 0;
   for (i=0; i<num_level; i++)
      total_pix_sq += hist[i] * i * i;

   if (*area == 1)
      *stdv = 0.0;
   else
      *stdv = sqrt((total_pix_sq - *mean * total_pix)/(double)(*area - 1));

   return(SUCCESS);
}
