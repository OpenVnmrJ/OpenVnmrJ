/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef FUNCSELECTION
IF_FITCODE("poly"){
    N_PARAMETERS = 2;
    if (nbr_params > 0) N_PARAMETERS = in_params[0];
    FUNCTION = poly_function;
    FIT_TYPE = LINEAR_FIXED;
    return TRUE;
}
#else /* not FUNCSELECTION */

static void
poly_function(int npts,		/* Nbr of data points */
	      int npars,	/* Nbr of parameters */
	      float *p,		/* npars parameter values */
	      int nvars,	/* Number of independent variables--NOT USED */
	      float *x,		/* npts*nvars values of indep variables */
	      float *y)		/* Function values OUT */
{
    int i, j;

    for (i=0; i<npts; i++){
	/* Compute y = p0 + p1*x + p2*x**2 + ... */
	y[i] = 0;
	for (j=npars-1; j>=0; j--){
	    y[i] = x[i] * y[i] + p[j];
	}
    }
}

#endif /* not FUNCSELECTION */
