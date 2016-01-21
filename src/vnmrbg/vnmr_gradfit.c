/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "wjunk.h"
#include "vnmr_gradfit_lm.h"

extern int abortflag;

void lm_g_init(double x[], double y[], double sig[], int ndata, double a[], int ia[],
        int ma, double **covar, double **alpha, double *chisq,
        void (*funcs)(double, double [], double *, double [], int))
{
   Werrprintf("gradfit fit function not available");
   abortflag = 1;
}

void lm_g_iterate(double x[], double y[], double sig[], int ndata, double a[], int ia[],
        int ma, double **covar, double **alpha, double *chisq,
        void (*funcs)(double, double [], double *, double [], int))
{
   Werrprintf("gradfit fit function not available");
   abortflag = 1;
}

void lm_g_covar(double x[], double y[], double sig[], int ndata, double a[], int ia[],
        int ma, double **covar, double **alpha, double *chisq,
        void (*funcs)(double, double [], double *, double [], int))
{
   Werrprintf("gradfit fit function not available");
   abortflag = 1;
}

void gaussj(double **a, int n, double **b, int m)
{
   Werrprintf("pcmap gauss jordan elimination not available");
   abortflag = 1;
}
