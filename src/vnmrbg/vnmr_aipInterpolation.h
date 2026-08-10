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

#ifdef VNMR_GPL
#include <gsl/gsl_spline.h>
#include <gsl/gsl_errno.h>

static void cubic_spline_interpolation(int na, float *ya, int nb, float *yb)
{
    /* Need at least 2 points to define a spline. */
    if (na < 2 || nb < 1) {
        for (int k = 0; k < nb; k++) yb[k] = (na > 0) ? ya[0] : 0.0f;
        return;
    }

    /* Build double-precision x/y arrays for GSL (x is simply 0..na-1). */
    double *x = new double[na];
    double *y = new double[na];
    for (int i = 0; i < na; i++) {
        x[i] = (double)i;
        y[i] = (double)ya[i];
    }

    gsl_interp_accel *acc    = gsl_interp_accel_alloc();
    gsl_spline       *spline = gsl_spline_alloc(gsl_interp_cspline, na);
    gsl_spline_init(spline, x, y, na);

    double xmax    = (double)(na - 1);
    double y2_1    = gsl_spline_eval_deriv2(spline, x[1],      acc); /* y2[1]    */
    double y2_nm2  = gsl_spline_eval_deriv2(spline, x[na - 2], acc); /* y2[na-2] */

    double slope_lo = (y[1]      - y[0])      - y2_1   / 6.0;
    double slope_hi = (y[na - 1] - y[na - 2]) + y2_nm2 / 6.0;

    /* Output t = j*(na/nb) - (1 - na/nb)*0.5 */
    double scale = (double)(na) / (double)(nb);
    for (int j = 0; j < nb; j++) {
        double t = (double)j * scale - (1.0 - scale) * 0.5;
        double v;
        if (t <= 0.0)
            v = y[0]      + t * slope_lo;
        else if (t >= xmax)
            v = y[na - 1] + (t - xmax) * slope_hi;
        else
            v = gsl_spline_eval(spline, t, acc);
        yb[j] = (float)v;
    }

    gsl_spline_free(spline);
    gsl_interp_accel_free(acc);
    delete [] x;
    delete [] y;
}
#else
extern void cubic_spline_interpolation(int na, float *ya, int nb, float *yb );
#endif

#endif 
