/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef FUNCSELECTION
IF_FITCODE("adc"){
    N_PARAMETERS = 2;
    FIT_TYPE = NONLINEAR;
    FUNCTION = adc_function;
    JACOBIAN = adc_jacobian;
    GUESS = adc_guess;

    return TRUE;
}
#else /* not FUNCSELECTION */

/* ---------------------------------------------------------------------------*/
/* FUNCTION DEFINITIONS - ADC experiment model                                */
/* ---------------------------------------------------------------------------*/
static void
adc_function(int npoints,	/* Nbr of data points */
	     int nparams,	/* Nbr of parameters--NOT USED */
	     float *params,	/* nparams parameter values */
	     int nvars,		/* Number of independent variables--NOT USED */
	     float *x,		/* npoints*nvars values of indep variables */
	     float *y)		/* Function values OUT */
{
    int i;
    for (i=0; i<npoints; i++){
	y[i] = params[1] * exp(-x[i] * params[0]);
    }
}

static void
adc_jacobian(int npoints,	/* Nbr of data points */
	     int nparams,	/* Nbr of parameters--NOT USED */
	     float *params,	/* nparams parameter values */
	     int nvars,		/* Number of independent variables--NOT USED */
	     float *x,		/* npoints*nvars values of indep variables */
	     float **dydp)	/* Derivative values OUT*/
{
    int i;

    for (i=0; i<npoints; i++){
	dydp[1][i] = exp(-x[i] * params[0]);
	dydp[0][i] = -dydp[1][i] * x[i] * params[1];
    }
}


/* ------------------GUESS FOR ADC FUNCTION----------------------
 * Initial guess for parameters of
 *
 *	y = p[1] * exp(-x * p[0])
 *
 * We note first that:
 * 
 *	ln(y) = ln(p[1] * exp(-x * p[0]))
 *	      = ln(p[1]) - x * p[0],  
 *
 * so we compute ln(y) for each x value and apply linear
 * regression to estimate ln(y) = m*x + b
 *
 * Transforming back, we get:
 * 
 *      p[0] = -m
 *      p[1] = exp(b)
 *     
 * NOTE:
 *    It is assumed that this routine will be called many times with the
 *    same number of points each time.  It does NOT check
 *    to make sure the number of points is unchanged.
 -------------------------------------------------------------------*/
static int
adc_guess(int npts,		/* Nbr of data points */
	  int npars,		/* Nbr of parameters-NOT USED */
	  float *p,		/* Parameter values OUT */
	  int nvars,		/* Number of independent variables-NOT USED */
	  float *x,		/* npts*nvars values of indep var */
	  float *y,		/* npts values of dependent variable */
	  float *resid,		/* Quality of fit OUT--OPTIONAL */
	  float *covar)		/* Covariance matrix OUT--OPTIONAL */
{
    float par[2];

    t2_guess(npts, npars, par, nvars, x, y, resid, covar);
    if (par[0] == 0){
	return FALSE;
    }
    p[0] = 1/par[0];
    p[1] = par[1];

    return TRUE;
}

#endif /* not FUNCSELECTION */
