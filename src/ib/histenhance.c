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
*  Routine related to histogram enhancement.				*
*									*
*************************************************************************/
#include "stderr.h"
#include <math.h>
#include "process.h"

/************************************************************************
*                                                                       *
*  This routine will build a look-up table from input histogram 'inhist'*
*  and put the result into histogram 'outhist' based on the choice of   *
*  its histtype.  Typically, this lookup table will be applied to the   *
*  data to enhance the image.                                           *
*  									*
*  Ref: Book -- Digital Image Processing, Rafael C. Gonzalez and Paul 	*
*		Wintz, 1977, pg. 119.					*
*	Paper - Image Enhancement by Histogram Hyperbolization, Warner	*
*	     	Frei, Computer Graphics and ImageProcessing 6, 1977,	*
*	        pg. 286.						*
*	      - Entropy-Constant Image Enhancement by Histogram 	*
*		Transformation, L O'Gorman, L S Brotman, SPIE Vol.575	*
*		Spplications of Digital Image Processing Viii(1985)	*
*		pg. 106.						*
*                                                                       *
*  Retun SUCCES or ERROR.                                               *
*                                                                       */
int
pro_histenhance(Histtype type,    /* histogram type */
	int *inhist,            /* input histogram */
	int *outhist,           /* output histogram */
	int num_hist_index,     /* number of histogram index */
	char *errmsg)
{
   int hcum[4096];    // cumulative histogram
   register int t;      // total sum of all histogram values
   register float ft;   // same as variable t (but in floating point)
   register int xl;     // the first index value of inhist[] which is not 0
   register int sum;    // temporary holding cumulative value
   register int i;      // loop counter
   register float num_gray;  // same value as num_hist_index (floating)
   register int result; // result of cummulcative

   if (num_hist_index > 4096)
   {
      WARNING_OFF(Sid);
      if (errmsg)
	 (void)sprintf(errmsg,
	    "pro_histenhance: # histogram index greater than 4096 is not supported");
      return(ERROR);
   }

   // Calculate the total cumulative distribution */
   t = 0;
   for (i=0; i<num_hist_index; i++)
      t += inhist[i];

   // Calculate the cumulative histogram distribution
   for (xl=0; (xl<num_hist_index) && (inhist[xl]==0); xl++);
   for (i=0; i<xl; i++) /* set unoccupied bin to zero */
      hcum[i] = 0 ;
   sum = 0;
   for (i=xl; i<num_hist_index; i++)
   {
      sum += inhist[i];
      result = sum - (inhist[i] + inhist[xl])/2;
      hcum[i] = (result > 0) ? result : 0;
   }

   // Build a look-up table
   ft = (float) t;
   num_gray = (float)num_hist_index;
   switch (type)
   {
      case HIST_EQUALIZATION:
         for (i=0; i<num_hist_index; i++)
            outhist[i] = (int)(num_gray/ft * (float)hcum[i]);
         break;
      case HIST_LOWINTENSITY:
         for (i=0; i<num_hist_index; i++)
            outhist[i] = (int)(num_gray *
                (1.0 - sqrt(1.0 - (float)hcum[i]/ft)));
         break;
      case HIST_HIGHINTENSITY:
         for (i=0; i<num_hist_index; i++)
            outhist[i] = (int)(num_gray * sqrt((float)hcum[i]/ft));
         break;
      case HIST_HYPERBOLIZATION:
         // Note: log(1.0 + 1.0/2.393) = 0.34916668
         for (i=0; i<num_hist_index; i++)
            outhist[i] = (int)(num_gray * 2.393 *
               (exp(0.34916668 * (float)hcum[i]/ft) - 1.0));
         break;
   }

   /* Make sure there is no value greater than or equal to num_hist_gray */
   for (i=0; i<num_hist_index; i++)
   {
      if (outhist[i] >= num_hist_index)
	 outhist[i] = num_hist_index - 1;
   }
   return(SUCCESS);
}
