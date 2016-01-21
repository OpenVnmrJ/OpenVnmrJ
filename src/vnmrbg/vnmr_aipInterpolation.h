/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef VNMR_AIPINTERPOLATION_H
#define VNMR_AIPINTERPOLATION_H

// Spline interpolation.
// Interpolates an array of points that just spans the data.
// Note that if n>n0, we will need to extrapolate some points.
// That is because each of the n0 data points is assumed to apply
// to 1/n0 of the data space.
//
// A picture may help:
//      |   0   |   1   |   2   |   3   | Data points
//      | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | Interpolated points
//
// The "interpolated" points #0 and #7 are evidently outside the
// range of the data (if the data applies to the center of the
// data box).
// These points are extrapolated from the first derivative of
// the spline at the end-points.
//
// I believe this is dead code. The gsl cubic spline interpolation
// routines give an error if you try to interpolate outside the range
// of input values.

#ifdef VNMR_GPL
#define cubic_spline_interpolation(a,b,c,d) simple_interpolation( (a), (b), (c), (d) )
#else
extern void cubic_spline_interpolation(int na, float *ya, int nb, float *yb );
#endif

#endif 
