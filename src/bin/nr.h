/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

#define TRADITIONAL 1   /* but leave LINT_ARGS and ANSI undefined */
#ifdef LINT_ARGS
	void fourn(float *, int *, int, int);
	void svbksb(float **, float *, float **, int, int,
		float *, float *);
	void svdcmp(float **, int, int, float *, float **);
#endif
#ifdef ANSI
	void  fourn(float *data, int *nn, int ndim, int isign);
	void  svbksb(float **u, float *w, float **v, int m, int n, float *b,
		float *x);
	void  svdcmp(float **a, int m, int n, float *w, float **v);
#endif
#ifdef TRADITIONAL
	void fourn();
	void svbksb();
	void svdcmp();
#endif
