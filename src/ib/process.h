/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _PROCESS_H
#define	_PROCESS_H
/************************************************************************
*									
*
*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*
*************************************************************************
*									
*  Description:
*  -----------
*  This file contain prototype functions related with data/histogram 
*  processings.  If last argument of the function is the error message
*  buffer, it will print the error message into the buffer. The user can
*  specify NULL if he doesn't care any error message.
*
*************************************************************************/

/* Maximum pixel value for 12 bits data */
#define	MAX_SHORT_VAL	4095

/* Histogram enhancement type */
typedef enum
{   
   HIST_EQUALIZATION,
   HIST_LOWINTENSITY,
   HIST_HIGHINTENSITY,
   HIST_HYPERBOLIZATION
} Histtype;

/************************************************************************
*                                                                       *
*  Calculate statistics information for: min, max, area, median, mean, 	*
*  and stdv, given historgram pixel values.                           	*
*                                                                       *
*  Retun SUCCES or ERROR.                                               *
*                                                                       */
extern int
pro_statistics(int *histogram,  /* input histogram */
        int num_level,          /* input number of level in histogram */
        int *min,               /* result minimum pixel value */
        int *max,               /* result maximum pixel value */
        int *area,              /* result accumulative # pixels in hist. */
        int *median,            /* result median pixel value */
        double *mean,           /* result mean value */
        double *stdv,           /* result standard deviation */
        char *errmsg);          /* error message buffer */

/************************************************************************
*                                                                       *
*  This routine will build a look-up table from input histogram 'inhist'*
*  and put the result into histogram 'outhist' based on the choice of	*
*  its histtype.  Typically, this lookup table will be applied to the	*
*  data to enhance the image.						*
*                                                                       *
*  Retun SUCCES or ERROR.                                               *
*                                                                       */
extern int
pro_histenhance(Histtype type,	/* histogram type */
	int *inhist,		/* input histogram */
	int *outhist,		/* output histogram */
	int num_hist_index,	/* number of histogram index */
	char *errmsg);		/* error message buffer */

/************************************************************************
*                                                                       *
*  Add, subtract, multiply or divide type of floating point data with   *
*  a constant. The result pixel will be guarenteed to be positive.      *
*  Return SUCCESS or ERROR.                                             *
*									*
*  Variable src : input data						*
*	    val : constant valeu to be operated				*
*	    dst : output data						*
*	    n   : number of data points					*
*           err : error message buffer					*
*                                                                       */
extern int
arith_fadd_image_const(float *src, float val, float *dst, int n, char *err);

extern int
arith_fsub_image_const(float *src, float val, float *dst, int n, char *err);

extern int
arith_fmul_image_const(float *src, float val, float *dst, int n, char *err);

extern int
arith_fdiv_image_const(float *src, float val, float *dst, int n, char *err);

/************************************************************************
*                                                                       *
*  Add, subtract, multiply or divide type of floating point data with   *
*  two images . The result pixel is guaranteed to be positive.          *
*  Return SUCCESS or ERROR.                                             *
*									*
*  Variable src : input data						*
*	    val : constant valeu to be operated				*
*	    dst : output data						*
*	    n   : number of data points					*
*           err : error message buffer					*
*									*
*  Operation : dst = src1 <op> src2					*
*                                                                       */
extern int
arith_fadd_images(float *src1, float *src2, float *dst, int n, char *err);

extern int
arith_fsub_images(float *src1, float *src2, float *dst, int n, char *err);

extern int
arith_fmul_images(float *src1, float *src2, float *dst, int n, char *err);

extern int
arith_fdiv_images(float *src1, float *src2, float *dst, int n, char *err);

/************************************************************************
*                                                                       *
*  Masking the image data using N x N filter
*
*  Programming Note:                                                    *
*  If 'mask_size' eguals n, then 'mask_buf' should point to array
*  of size nxn.                                                         *
*  During the processing, it will notify user function 'notify_stat'	*
*  every 5% of data which has been processed so that the user can	*
*  print out the current status.  It passes the argument of number of	*
*  percentage data which has been processed. The user can set it to 	*
*  NULL, if no status is required.					*
*  Besides, the user can set interrupt to stop processing.  The user	*
*  can set it to NULL if no interrupt is required.			*
*  Return SUCCESS or ERROR.                                             *
*                                                                       */
int
filter_mask(float *in,		// Input data
	    float *out,		// Output data
	    int wd, int ht,	// Width and height of the image
	    float *mask_buf,	// Length is mask_wd*mask_ht
	    int mask_wd,	// Width of mask
	    int mask_ht,	// Height of mask
	    int (*cancelled)(),	// Abort if *cancelled() returns True
	    void (*func_notify_stat)(int),// Report progress
	    char *err,		// Message in case of ERROR return
	    int do_median);	// If TRUE, calculate median instead of mean


#endif (_PROCESS_H)
