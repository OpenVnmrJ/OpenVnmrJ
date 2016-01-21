/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef AIPINTERPOLATION_H
#define AIPINTERPOLATION_H

typedef enum {
    INTERP_REPLICATION = 0,
    INTERP_LINEAR,
    INTERP_CUBIC_SPLINE
} interpolation_t;

void
aipImageInterpolation(interpolation_t method,
		      float *data,
		      int npf,
		      int npm,
		      float *cdata,
		      int cnpf,
		      int cnpm);

typedef void (*rebin_t)(float*, int, int, double, double, float*, int, int);

void
boxcarRebin(float *inBuf,
	    int inBins,		// >0
	    int inIncr,		// !=0
	    double inStx,
	    double inWd,	// >0
	    float *outBuf,
	    int outBins,	// >0
	    int outIncr);	// !=0

void
boxcarRebinSum(int *inBuf,
	       int inBins,	// >0
	       int inIncr,	// !=0
	       double inStx,
	       double inWd,	// >0
	       int *outBuf,
	       int outBins,	// >0
	       int outIncr);	// !=0

void
linearExpand(float *inBuf,
	     int inBins,	// >0
	     int inIncr,	// !=0
	     double inStx,
	     double inWd,	// >0, <outBins
	     float *outBuf,
	     int outBins,	// >inWd
	     int outIncr);	// !=0

void
biParabolicExpand(float *inBuf,
		  int inBins,	// >2
		  int inIncr,	// !=0
		  double inStx,	// >= 0
		  double inWd,	// >0
		  float *outBuf,
		  int outBins,	// >inWd
		  int outIncr);	// !=0

#endif /* AIPINTERPOLATION_H */
