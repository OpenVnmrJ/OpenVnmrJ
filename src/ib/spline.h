/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
// @(#)spline.h 18.1 03/21/08 (c)1992 SISCO

//*********************************************************************
//	Constants
//*********************************************************************

typedef enum {
    SIMPLE = 0,
    LINEAR,
    CUBIC_SPLINE,
    NO_INTERPOLATION_METHOD
    } Interpolation_method;

typedef	enum {
    PASSASMINUS = 1,
    CONV2ZERO,
    NO_MINUSVAL
    } Minusval_type;


//*********************************************************************
//	Function declarations
//*********************************************************************

int
image_2D_interpolation( Interpolation_method	method,
			Minusval_type	minus,
			float 	*data,
			int 	npf,
			int	npm,
			float 	*cdata,
			int 	cnpf,
			int	cnpm );

void
cubic_spline_2D_interpolation(  Minusval_type	minus,
				int 	npf,
				int	npm,
			     	float 	*data,
				int 	cnpf,
				int	cnpm,
			     	float 	*cdata );

void
cubic_spline_interpolation( int 	na,
			    float 	*ya,
			    int		nb,
			    float	*yb );

void
simple_interpolation( 	int 	na,
			float 	*ya,
			int	nb,
			float	*yb );
