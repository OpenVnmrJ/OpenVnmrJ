/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include	<stdio.h>
#include	<math.h>
#include	<stdlib.h>

#include	"aipStderr.h"
#include	"aipInterpolation.h"
#include	"vnmr_aipInterpolation.h"

#define		DEBUG1	  0 
#define		DEBUG2	  0

//
// Function declarations
//
static void
simple_interpolation(int na,
		     float *ya,
		     int nb,
		     float *yb );

/*==============================================================

    Image ( 2D data ) interpolation
	data: input data
	npf: data size at fast dimension of input data
	npm: data size at medium dimension of input data

	cdata: output data
	cnpf: data size at fast dimension of output data
	cnpm: data size at medium dimension of output data

	method: INTERP_LINEAR or INTERP_CUBIC_SPLINE

===============================================================*/

void
aipImageInterpolation(interpolation_t method,
		      float 	*data,
		      int 	npf,
		      int	npm,
		      float 	*cdata,
		      int 	cnpf,
		      int	cnpm )
{
    /*
     *	Memory allocation
     */
    float *buf = new float[npm];
    float *cbuf = new float[cnpm];
    float *buf2D = new float[npf*cnpm];
    /*
     *	Interpolation along "medium" axis
     */
    int	i, j;

    for( j=0; j<npf; j++ ){
	for( i=0; i<npm; i++ ){
	    buf[i] = data[ i*npf + j ];
	} 
	
	if( method == INTERP_CUBIC_SPLINE ) {
	    cubic_spline_interpolation( npm, buf, cnpm, cbuf ); 
	} else {
	    simple_interpolation( npm, buf, cnpm, cbuf );
	}
	
	for( i=0; i<cnpm; i++ ){
	    buf2D[i*npf+j] = cbuf[i];
	}
    }
    /*
     *	Interpolation along "fast" axis
     */
    for( j=0; j<cnpm; j++ ){
	if( method == INTERP_CUBIC_SPLINE ) {
	    cubic_spline_interpolation(npf, &buf2D[j*npf],
				       cnpf, &cdata[j*cnpf]);
	}else{
	    simple_interpolation(npf, &buf2D[j*npf],
				 cnpf, &cdata[j*cnpf]);
	}
    }
    
    delete [] buf;
    delete [] cbuf;
    delete [] buf2D;
}

/*==============================================================

    simple interpolation
	ya: input data
	na: data size of input data

	yb: output data
	nb: data size of output data
	
===============================================================*/
static void
simple_interpolation( 	int 	na,
		     float 	*ya,
		     int	nb,
		     float	*yb )
{
    /*
     *	Simple interpolation
     */
    register float 	*p_in  = ya;
    register float 	*p_out = yb;
    register int  	dv;
    register float	value = (float)0.0;
    register int    i;
    int	D1, D2;
    
    if( na == nb ) 		/* same size */
    {	
	for (i=0; i<na; i++){
	    yb[i] = ya[i];
	}
    }
    else if( na < nb ) 		/* expanding */
    {
	D1 = 2 * na;
	D2 = D1 - 2*nb;
	
    	/* set the starting value of the decision variable */
    	dv = D1 - 2*nb;
    	//  dv = D1 - nb;
    	value = *p_in;
	
    	for (i = nb; i > 0; --i, ++p_out)
    	{
	    /* load the data value into the output buffer */
	    *p_out = value;
	    
	    /* adjust the decision variable */
	    if (dv < 0)
	        dv += D1;
	    else
	    {
               	dv += D2;
		/* move to the next input point */
            	++p_in;
	    	value = *p_in;    
	    }
    	}
    }
    else			/* compress */
    {
	D1 = 2 * nb;
	D2 = D1 - 2*na;
	
    	/* set the starting value of the decision variable */
    	dv = D1 - na;
	
    	for (i = na; i > 0; --i, ++p_in)
    	{
	    if( *p_in > value )
	        value = *p_in;
	    
	    /* adjust the decision variable */
	    if (dv < 0)
	        dv += D1;
	    else
	    {
            	dv += D2;
	    	*p_out++ = value;
		value = (float)0.0;
	    }
    	}
    }
}

/*
 ***********************************************************************
 * The following routines all use the same function arguments:
 *
 * "inBuf" and "outBuf" are pointers to the logical first elements
 * of the input and output buffers.
 *
 * "inIncr" and "outIncr" allow you to go through a buffer in reverse
 * order, or go up or down a column in a 2D array.  To go forward
 * along a row set incr=1, backwards, incr=-1.  To go down a column,
 * incr=rowlen, to go up incr=-rowlen.  When going backwards, the "xxBuf"
 * argument points to the end of the row/column.  Regardless of the value
 * of incr, the "xxBuf" points to the logical first element; the i'th
 * element is addressed xxBuf[i*xxIncr].
 *
 * "inBins" and "outBins" give the size of the input and output buffers.
 * The functions may not look at all of the input bins, depending on the
 * values of instx and inwd.  Something will be written to all of the
 * output bins.
 *
 * "inStx" is the position in the input buffer of the leading edge of
 * the first output bin; roughly, the first data position to look at.
 * The first input data point spans the input space 0 to 1, so it is
 * centered on 0.5; the last input data point extends from inBins-1 to
 * inBins.  So the total input width is inBins, not inBins-1.
 *
 * "inWd" is the range of input data to map to the outBins output ins.
 * To map all of the input to the output, inStx=0 and inWd=inBins.
 *
 * Example:  inBins=6, inStx=0.14, inWd = 5.7, outBins=4
 *
 *      |    0    |    1    |    2    |    3    |    output buf
 *     |   0  |   1  |   2  |   3  |  4   |   5  |   input buf
 *      |                                       |
 *      |<------------- inWd ------------------>|
 *    inStx                                     |
 *
 ***********************************************************************
 */

/*
 * Works for expansion or compression.
 * Recommended for compression to mild expansion.
 * Conserves relative integrals of intensity over features.
 * Similar to pixel replication for expansion.
 * Very loose input restrictions, as noted below.
 *
 * Example:  inBins=6, inStx=0.14, inWd = 5.7, outBins=4
 *
 *      |    0    |    1    |    2    |    3    |    output buf
 *     |   0  |   1  |   2  |   3  |  4   |   5  |   input buf
 *      |         |                             |
 *      |<------------- inWd ------------------>|
 *    inStx       |                             |
 *      |<- dx -->|                             |
 *      x         x1                            |
 */
void
boxcarRebin(float *inBuf,
	    int inBins,		// >0
	    int inIncr,		// !=0
	    double inStx,
	    double inWd,	// >0
	    float *outBuf,
	    int outBins,	// >0
	    int outIncr)	// !=0
{
    int lastin = inBins - 1;
    double dx = inWd / outBins;
    double x = inStx;		// Start of current output bin on input data.
    double x1;			// End of current output bin
    int i = (int)x;
    if (i < 0) {i = 0;}
    if (i > lastin) {i = lastin;}
    inBuf += i * inIncr;
    double sum;
    int iout;
    int j;

    for (iout=0; iout < outBins; iout++) {
	sum = 0;
	x1 = x + dx;
	j = (int)x1;
	//if (j < 0) {j = 0;}
	if (j > lastin) {j = lastin;}
	while (i < j) {
	    /* Add in the rest of the current input bin. */
	    ++i;
	    sum += *inBuf * (i - x);
	    inBuf += inIncr;
	    x = (double)i;
	}
	/* i == j */
	/* Add in some fraction of the last input bin. */
	sum += *inBuf * (x1 - x);
	*outBuf = (float)(sum / dx);
	outBuf += outIncr;
	x = x1;
    }
}

/*
 * Works for expansion.
 * Linear interpolation between data points.
 * Approximate conservation of intensity integrals.
 */
void
linearExpand(float *inBuf,
	     int inBins,	// >1
	     int inIncr,	// !=0
	     double inStx,
	     double inWd,	// >0, <outBins
	     float *outBuf,
	     int outBins,	// >inWd
	     int outIncr)	// !=0
{
    int lastin = inBins - 1;
    double dx = inWd / outBins;	// 0 < dx < 1
    double x = inStx - 0.5;
    int i = (int)x;		// If x<0, we are extrapolating
    if (i < 0) {i = 0;}
    if (i > lastin) {i = lastin;}
    inBuf += i * inIncr;
    double b = *inBuf;
    double m = 0;
    if (i == lastin && i > 0) {
	b = *(inBuf - inIncr);
	m = *inBuf - b;
    }
    int iout = 0;
	
    /* "x" is position w.r.t. left data point */
    for (x -= i ; i < lastin && iout < outBins; i++, x--) {
	/* Do for each input bin pair */
	b = *inBuf;		// Intercept is left data point
	inBuf += inIncr;
	m = *inBuf - b;		// Slope is (right - left)
	while (x <= 1 && iout < outBins) {
	    /*
	     * Do all output bins up to the center of the next
	     * data point
	     */
	    *outBuf = (float)(m * x + b);
	    outBuf += outIncr;
	    x += dx;
	    iout++;
	}
    }
    while (iout < outBins) {
	/* Ran out of input bins; extrapolate */
	*outBuf = (float)(m * x + b);
	outBuf += outIncr;
	x += dx;
	iout++;
    }
}

void
biParabolicExpand(float *inBuf,
		  int inBins,	// >2
		  int inIncr,	// !=0
		  double inStx,	// >= 0
		  double inWd,	// >0
		  float *outBuf,
		  int outBins,	// >inWd
		  int outIncr)	// !=0
{
    int maxfirst = inBins - 3;	// Max index of first pt in parabolic fit
    double dx = inWd / outBins;	// 0 < dx < 1
    double x = inStx + dx / 2;	// Position of center of first output bin
    int i = (int)(x - 1.5);	// Index of first pt in triple
    x -= (i + 1.5);		// Dist from center point of triple
    if (i < 0) {
	x += i;			// Make x smaller
	i = 0;			// ... and i bigger
    }
    if (i > maxfirst) {
	x += i - maxfirst;	// Make x bigger
	i = maxfirst;		// ... and i smaller
    }
    inBuf += i * inIncr;	// Get first triple of points
    double y1 = *inBuf; inBuf += inIncr; i++;
    double y2 = *inBuf; inBuf += inIncr; i++;
    double y3 = *inBuf; inBuf += inIncr; i++;
    double a1, b1, c1, a2, b2, c2;
    a2 = (y1 - 2 * y2 + y3) / 2; // Fit parabola through triple
    b2 = (y3 - y1) / 2;
    c2 = y2;
    int iout = 0;

    while (x <= 0 && iout < outBins) {
	// Do some beginning points with one parabola
	*outBuf = (float)((a2 * x + b2) * x + c2);
	outBuf += outIncr;
	++iout;
	x += dx;
    }

    while (i < inBins && iout < outBins) {
	a1 = a2; b1 = b2; c1 = c2;

	y1 = y2; y2 = y3;	// Get next triple of points
	y3 = *inBuf; inBuf += inIncr; i++;

	a2 = (y1 - 2 * y2 + y3) / 2; // Fit next parabola
	b2 = (y3 - y1) / 2;
	c2 = y2;

	while (x <= 1 && iout < outBins) {
	    double yc1 = (a1 * x + b1) * x + c1; // y from first parabola
	    double xr = x - 1;	// NB: xr < 0
	    double yc2 = (a2 * xr + b2) * xr + c2; // y from second parabola
	    *outBuf = (float)(x * yc2 - xr * yc1); // Weighted average
	    outBuf += outIncr;
	    ++iout;
	    x += dx;
	}
	--x;
    }

    while (iout < outBins) {
	// Do some end points with one parabola
	*outBuf = (float)((a2 * x + b2) * x + c2);
	outBuf += outIncr;
	++iout;
	x += dx;
    }
}


/*
 * Preliminary method for combining histograms.
 * The output buffer is assumed to be already initialized, and the
 * data is just added in.  This allows summing of histograms into the
 * buffer.  Only works well if the input bins are smaller than the
 * output bins (outWd < inBins).
 * outStx: index on output buffer of first input bin.
 * outWd: number of output bins covered by the input buffer.
 * Assumes inIncr = outIncr = 1.
 */
void
boxcarRebinSum(int *inBuf,
               int inBins,      // >0
               int inIncr,      // =1
               double outStx,
               double outWd,    // >0
               int *outBuf,
               int outBins,     // >0
               int outIncr)     // =1
{
    double dx = outWd / inBins;
    int out = (int)outStx;
    for (int in = 0; in < inBins && out < outBins; in++) {
        if (out >= 0) {
            outBuf[out] += inBuf[in];
        }
        outStx += dx;
        out = (int)outStx;
    }
}
