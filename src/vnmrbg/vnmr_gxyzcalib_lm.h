/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef VNMRGXYZCALIBLM_H
#define VNMRGXYZCALIBLM_H

extern void lm_gxyz_init(double x1[],double y1[], double w[], double sig[],
    int ndata, double a[], int ia[], int ma, double **covar, double **alpha,
    double *chisq, void (*funcs)(double, double, double [], double *,
    double [], int));

extern void lm_gxyz_iterate(double x1[],double y1[], double w[], double sig[],
    int ndata, double a[], int ia[], int ma, double **covar, double **alpha,
    double *chisq, void (*funcs)(double, double, double [], double *,
    double [], int));

extern void lm_gxyz_covar(double x1[],double y1[], double w[], double sig[],
    int ndata, double a[], int ia[], int ma, double **covar, double **alpha,
    double *chisq, void (*funcs)(double, double, double [], double *,
    double [], int));

#endif
