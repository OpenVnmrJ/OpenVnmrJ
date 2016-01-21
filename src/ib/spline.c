/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

/* 
 */


/*===================================================================

	Program Information
		Source file  : spline_interpolation.c
		Process type : Library
		Package name : CSI Level-2
		
	Function purpose
		 cubic spline interpolation 

	Environment Information
		Computor	: Sun SPARC 1, SPARC 1+, SPARC 2
		OS		: Sun UNIX Ver. 4.1.1
		Language	: ANSI-C

	Administration Information
		Administrator : Yasuhiro Ueshima
			1120 Auburn Street, Fremont CA 94538

  Copyright (c) 1991-1996  by Spectroscopy Imaging Systems Corporation
====================================================================
< Algorithm >
	cubic spline interpolation
	simple interpolation

====================================================================*/

#include	<stdio.h>
#include	<math.h>
#include	<stdlib.h>

#include	"stderr.h"
#include	"spline.h"

#define		DEBUG1	  0 
#define		DEBUG2	  0

//
// Function declarations
//
void
cspline( float  *x, float  *y,
	 int    n,
	 float  yp1, float  ypn,
	 float  *y2 );

void
csplint( int	n0,
	 float 	*ya, float *y2a,
	 int	n,
	 float   *y );


/*==============================================================

    Image ( 2D data ) interpolation
	data: input data
	npf: data size at fast dimension of input data
	npm: data size at medium dimension of input data

	cdata: output data
	cnpf: data size at fast dimension of output data
	cnpm: data size at medium dimension of output data

	method: SIMPLE or CUBIC_SPLINE
	minus:  PASSASMINUS or CONV2ZERO

	RETURN value is always OK(!)

===============================================================*/
int
image_2D_interpolation( Interpolation_method	method,
			Minusval_type	minus,
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
	
	if( method == CUBIC_SPLINE ) {
	    cubic_spline_interpolation( npm, buf, cnpm, cbuf ); 
	}else{
	    simple_interpolation( npm, buf, cnpm, cbuf );
	}
	
	if( minus == CONV2ZERO ){
	    for( i=0; i<cnpm; i++ ){
		if( cbuf[i] < 0 )
		    buf2D[i*npf+j] = 0;
		else
		    buf2D[i*npf+j] = cbuf[i];
	    }
	}else{
	    for( i=0; i<cnpm; i++ ){
	        buf2D[i*npf+j] = cbuf[i];
	    }
	}
    }
    /*
     *	Interpolation along "fast" axis
     */
    for( j=0; j<cnpm; j++ ){
	if( method == CUBIC_SPLINE ) {
	    cubic_spline_interpolation(npf, &buf2D[j*npf],
				       cnpf, &cdata[j*cnpf]);
	}else{
	    simple_interpolation(npf, &buf2D[j*npf],
				 cnpf, &cdata[j*cnpf]);
	}
	
	if( minus == CONV2ZERO ){
	    for( i=0; i<cnpf; i++ ){
		if( cdata[i+j*cnpf] < 0 )
		cdata[i+j*cnpf] = 0;
	    }
	}
    }
    
    delete [] buf;
    delete [] cbuf;
    delete [] buf2D;

    return OK;
}


/*==============================================================

    Cubic spline 2D interpolation
	data: input data
	npf: data size at fast dimension of input data
	npm: data size at medium dimension of input data

	cdata: output data
	cnpf: data size at fast dimension of output data
	cnpm: data size at medium dimension of output data	

	mode:  CONV2ZERO -> convert a minus value to 0.0
	
	===============================================================*/
void
cubic_spline_2D_interpolation(Minusval_type	minus,
			      int 	npf,
			      int	npm,
			      float 	*data,
			      int 	cnpf,
			      int	cnpm,
			      float 	*cdata )
{
    image_2D_interpolation(CUBIC_SPLINE, minus, data, npf, npm,
			   cdata, cnpf, cnpm );
}

/*==============================================================

    Cubic spline interpolation
	ya: input data
	na: data size of input data

	yb: output data
	nb: data size of output data
	
	===============================================================*/
void
cubic_spline_interpolation( int 	na,
			   float 	*ya,
			   int		nb,
			   float	*yb )
{
    /*
     *	Preparation for spline interpolation
     */
    int	i;
    float	yp1, ypn;

    float *xa = new float[na];
    float *y2a = new float[na];

    for( i=0; i<na; i++ ) {
	xa[i] = (float)i;
    }

    // Force first derivative to 0 at endpoints.
    //yp1 = 0.0;
    //ypn = 0.0;

    // Let derivative at endpoints go to "natural" value.
    yp1 = 1.0e30;
    ypn = 1.0e30;

    if( DEBUG1 ){
	for( i=0; i<na; i++ )
	printf(" xa = %10.3f \n ", xa[i] );
    }
    
    cspline(xa, ya, na, yp1, ypn, y2a);
    
    if( DEBUG1 ){
	for( i=0; i<na; i++ )
	printf(" y2a %10.3f \n ", y2a[i] );
    }
    /*
     *	Spline interpolation
     */
    csplint(na, ya, y2a, nb, yb);
    
    delete [] xa;
    delete [] y2a;
}

//
// Cubic spline fitting to data.
// Adapted from "Numerical Recipes in C" to use zero-offset arrays.
//
void
cspline(float  *x,	// Data x values.
	float  *y,	// Data y values.
	int     n,	// Number of data points.
	float  yp1,	// Derivative at first point. ("Natural" fit if >=1e30)
	float  ypn,	// Derivative at last point. ("Natural" fit if >=1e30)
	float  *y2 )	// Returns second derivative of fit at data points.
{
    int	i,k;
    float	p, qn, sig, un;

    float *u = new float[n];
    
    if( yp1 > 0.99e30 ){
	// Lower boundary condition = "natural" spline.
	y2[0] = u[0] = 0.0;
    }else{
	// Specified first derivative.
	y2[0] = -0.5;
	u[0] = ( 3.0/(x[1]-x[0]) )*( (y[1]-y[0])/(x[1]-x[0]) - yp1 );
    }

    // Decomposition loop of the tridiagonal algorithm.
    // y2 and u are used for temp storage of the decomposed factors.
    for( i=1; i<=n-2; i++){
	sig = (x[i]-x[i-1]) / (x[i+1]-x[i-1]);
	p = sig * y2[i-1] + 2.0;
	y2[i] = (sig-1.0) / p;
	u[i] = (y[i+1]-y[i]) / (x[i+1]-x[i]) - (y[i]-y[i-1])/(x[i]-x[i-1]);
	u[i] = ( 6.0*u[i]/(x[i+1]-x[i-1]) - sig*u[i-1] ) / p;
    }
    
    if( ypn > 0.99e30 ){
	// Upper boundary condition = "natural" spline.
	qn = un = 0.0;
    }else{
	// Specified first derivative.
	qn = 0.5;
	un = ( 3.0/(x[n-1]-x[n-2]) )*( ypn - (y[n-1]-y[n-2])/(x[n-1]-x[n-2]) );
    }
    
    y2[n-1] = (un-qn*u[n-2]) / (qn*y2[n-2]+1.0);
    // Back substitution loop of the tridiagonal algorithm.
    for( k=n-2; k>=0; k-- ){
	y2[k] = y2[k]*y2[k+1] + u[k];
    }
    
    delete [] u;
}

//
// Spline interpolation.
// Interpolates an array of points that just spans the data.
// Note that if n>n0, we will need to extrapolate some points.
// That is because each of the n0 data points is assumed to apply
// to 1/n0 of the data space.
//
// A picture may help:
//	|   0   |   1   |   2   |   3   | Data points
//	| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | Interpolated points
//
// The "interpolated" points #0 and #7 are evidently outside the
// range of the data (if the data applies to the center of the
// data box).
// These points are extrapolated from the first derivative of
// the spline at the end-points.
//
void
csplint(int	n0,	// Number of points in the original data.
	float 	*ya,	// The data values.
	float 	*y2a,	// Second derivative array from cspline.
	int	n,	// Number of points to interpolate.
	float   *y )	// Returns the interpolated points.
{
    int		klo, khi, k;
    float	a;
    float	b;
    float	x;

    float scaling = (float)n0 / (float)n;
    float shift = 0.5 * (1.0 - scaling);

    for( k=0; k<n; k++ ){
	x = scaling * k - shift;
	if (x <= 0){
	    y[k] = ya[0] + x * (ya[1] - ya[0] - y2a[1] / 6.0);
	}else if (x >= (float)(n0-1)){
	    y[k] = ya[n0-1]
	          + (x - n0 + 1) * (ya[n0-1] - ya[n0-2] + y2a[n0-2] / 6.0);
	}else{
	    klo = (int)x;
	    khi = klo + 1;
	    a = ( (float)khi - x );
	    b = ( x - (float)klo );
	    y[k] = a * ya[klo] + b * ya[khi] 
	           + ( (a*a*a-a) * y2a[klo] + (b*b*b-b) * y2a[khi] ) / 6.0;
	}
    }
}


/*==============================================================

    simple interpolation
	ya: input data
	na: data size of input data

	yb: output data
	nb: data size of output data
	
===============================================================*/
void
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
