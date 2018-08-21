/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

static char *Sid(){
    return "@(#)filter.c 18.1 03/21/08 (c)1991-92 SISCO";
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
*  Routines related to filter image data using masking.			*
*									*
*************************************************************************/
#include "stderr.h"
#include "process.h"
#include "ibcursors.h"

#define SWAP(x,y) {float t=(x); (x)=(y); (y)=t;}
float
median(int n, float *buf)
{
    float *middle;
    float partition;
    float *phigh;
    float *plow;

    float *left = buf;
    float *rank = left + n/2;
    float *right = left + n - 1;
    while (TRUE){
	if (right - left < 2){
	    // Either one or two elements left
	    if (right == left+1 && *right < *left){
		SWAP(*left, *right);
	    }
	    return *rank;
	}else{
	    middle = left + (right - left) / 2;
	    plow = left + 1;
	    // Partition on median of left/middle/right
	    // Put partition value on left, low in left+1 high in right
	    SWAP(*middle, *plow);
	    if (*plow > *right){
		SWAP(*plow, *right);
	    }
	    if (*left > *right){
		SWAP(*left, *right);
	    }
	    if (*plow > *left){
		SWAP(*plow, *left);
	    }
	    phigh = right;
	    partition = *left;
	    while (TRUE){
		do {
		    plow++;
		} while (*plow < partition);
		do {
		    phigh--;
		} while (*phigh > partition);
		if (phigh < plow){
		    break;
		}
		SWAP(*plow, *phigh);
	    }
	    *left = *phigh;
	    *phigh = partition;
	    if (phigh >= rank){
		right = phigh - 1;
	    }
	    if (phigh <= rank){
		left = plow;
	    }
	}
    }
}

/************************************************************************
 *                                                                       *
 *  Filters the input data with an (mask_wd x mask_ht) weighting mask.
 *  Output may be to the same buffer.
 *
 *  Returns SUCCESS or ERROR.						*
 *									*/
int
filter_mask(float *in,		// Input data
	    float *out,		// Output data
	    int wd, int ht,	// Width and height of the image
	    float *mask_buf,	// Length is mask_wd*mask_ht
	    int mask_wd,	// Width of mask
	    int mask_ht,	// Height of mask
	    int (*cancelled)(),	// Abort if *cancelled() returns True
	    void (*func_notify_stat)(int), // Report progress
	    char *err,		// Message in case of ERROR return
	    int do_median)	// If TRUE, calculate median instead of mean
{
    register float *indata = in;	// Input data
    register float *outdata = out;	// Output data
    register int width=wd;		// Data width
    register float *mask;		// Filter weights
    
    register int m,n;			// Loop counters for mask size
    register int mwd=mask_wd;		// Mask width
    
    register int ctr_x;			// Offset to center of mask
    register int ctr_y;			// Offset to center of mask
    register float result;		// Filtered value
    register int i;			// Loop counter
    register int j;			// Loop counter
    int k;
    float *medbuf = 0;
    int num_col;			// Number columns to process
    int num_row;			// Number rows to process
    int rtn = SUCCESS;
    
    // Check the mask size
    if ( ((mask_wd % 2) == 0) || ((mask_ht % 2) == 0)){
	if (err){
	    (void)sprintf(err,
			  "filter_mask(): Mask width and height must be odd");
	}
	return(ERROR);
    }
    
    ctr_x = mask_wd / 2;
    ctr_y = mask_ht / 2;
    
    /* Check for data width */
    if ((wd < mask_wd) || (ht < mask_ht)){
	if (err){
	    (void)sprintf(err, "filter_mask(): Filter is larger than image!");
	}
	return(ERROR);
    }

    // Generate a temporary result buffer (if necessary)
    if (in == out){
	outdata = new float[wd*ht];
    }
    
    // Generate a normalized filter (if possible)
    float sum = 0;
    float *mask_end = mask_buf + mask_wd * mask_ht;
    for (mask=mask_buf; mask<mask_end; sum += *mask++);
    if (sum == 0){
	sum = 1;
    }else if (sum < 0){
	sum = -sum;
    }
    
    float *norm_mask = new float[mask_wd * mask_ht];
    mask_end = mask_buf + mask_wd * mask_ht;
    float *outmask = norm_mask;
    for (mask=mask_buf; mask<mask_end; *outmask++ = *mask++ / sum);
    
    // The border of the image is not processed; fill it with zeros
    float *data = outdata;
    float *end = outdata + ctr_y * wd;		// End of first "ctr_y" rows
    while (data<end){
	*data++ = 0;				// Clear TOP border
    }
    data = outdata + (ht - ctr_y - 1) * wd;
    end = outdata + wd * ht;			// End of data
    while (data<end){
	*data++ = 0;				// Clear BOTTOM border
    }
    
    for (i=ctr_y; i<ht-ctr_y-1; i++){
	for (j=0; j<ctr_x; j++){
	    *(outdata + i * wd + j) = 0;		// Clear LEFT border
	    *(outdata + i * wd + wd-1-j) = 0;	// Clear RIGHT border
	}
    }

    if (do_median){
	medbuf = new float[mask_ht * mwd];
	if (!medbuf){
	    sprintf(err, "filter_mask(): Out of memory!");
	    return ERROR;
	}
    }

    int cursor = set_cursor_shape(IBCURS_BUSY);

    num_col = wd - mask_wd + 1;
    num_row = ht - mask_ht + 1;
    for (i=0; i<num_row; i++){			// Process rows
	/* Notify user function for status every 5% data */
	if (func_notify_stat && (((100 * i / num_row) % 5) == 0)){
	    (*func_notify_stat)(100 * i / num_row);
	}

	for (j=0; j<num_col; j++){		// Process columns
	    if (do_median){
		for (n=k=0; n<mask_ht; n++){ 	// Process m x n Mask
		    for (m=0; m<mwd; m++){
			medbuf[k++] = *(indata + (i+n) * width + (j+m));
		    }
		}
		result = median(k, medbuf);
	    }else{
		result = 0;
		for (n=0; n<mask_ht; n++){		// Process m x n Mask
		    for (m=0; m<mwd; m++){
			result += ( *(indata + (i+n) * width + (j+m))
				    * *(norm_mask + n * mwd + m) );
		    }
		}
	    }
	    // Assign the result to the pixel at the center of the filter
	    *(outdata + (i+ctr_y) * width + (j+ctr_x)) = result;
	}
	
	/* Check whether it is interrupted or not */
	if (cancelled && (*cancelled)()){
	    rtn = ERROR;
	    break;
	}
    }

    // Clean up
    delete [] norm_mask;
    delete [] medbuf;
    if (in == out && rtn == SUCCESS){
	// Move result from temp buffer to I/O buffer
	end=out+wd*ht;
	float *data2 = outdata;
	for (data=out; data<end; *data++ = *data2++);
	delete [] outdata;
    }

    (void)set_cursor_shape(cursor);
    return rtn;
}
