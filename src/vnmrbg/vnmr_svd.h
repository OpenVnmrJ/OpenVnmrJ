/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef VNMRSVD_H
#define VNMRSVD_H

#ifdef VNMR_GPL
extern void vnmr_svd(double **a, int m, int n, double w[], double **v);
extern void vnmr_svd_solve(double **u, double w[], double **v, int m, int n, double b[], double x[]);

#else
extern void svdcmp(double **a, int m, int n, double w[], double **v);
extern void svbksb(double **u, double w[], double **v, int m, int n, double b[], double x[]);
#define vnmr_svd(a, m, n, w, v) svdcmp( (a), (m), (n), (w), (v) )
#define vnmr_svd_solve(u, w, v, m, n, b, x) svbksb((u), (w), (v), (m), (n), (b), (x) )

#endif

#endif 
