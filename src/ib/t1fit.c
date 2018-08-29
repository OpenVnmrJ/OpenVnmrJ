/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef FUNCSELECTION
IF_FITCODE("t1"){
    N_PARAMETERS = 3;
    FIT_TYPE = NONLINEAR;
    FUNCTION = exp_function;
    JACOBIAN = exp_jacobian;
    GUESS = exp_guess;
    PARFIX = t1_parfix;
    return TRUE;
}
IF_FITCODE("abst1"){
    N_PARAMETERS = 3;
    FIT_TYPE = NONLINEAR;
    FUNCTION = exp_function;
    JACOBIAN = exp_jacobian;
    GUESS = abs_exp_guess;
    PARFIX = t1_parfix;
    return TRUE;
}
#else /* not FUNCSELECTION */

static void
exp_function(int npoints,	/* Nbr of data points */
	     int nparams,	/* Nbr of parameters--NOT USED */
	     float *params,	/* nparams parameter values */
	     int nvars,		/* Number of independent variables--NOT USED */
	     float *x,		/* npoints*nvars values of indep variables */
	     float *y)		/* Function values OUT */
{
    int i;
    for (i=0; i<npoints; i++){
	y[i] = params[0] + params[1] * exp(x[i] * params[2]);
    }
}

static void
exp_jacobian(int npoints,	/* Nbr of data points */
	     int nparams,	/* Nbr of parameters--NOT USED */
	     float *params,	/* nparams parameter values */
	     int nvars,		/* Number of independent variables--NOT USED */
	     float *x,		/* npoints*nvars values of indep variables */
	     float **dydp)	/* Derivative values OUT*/
{
    int i;

    for (i=0; i<npoints; i++){
	dydp[0][i] = 1;
	dydp[1][i] = exp(x[i] * params[2]);
	dydp[2][i] = dydp[1][i] * x[i] * params[1];
    }
}

static int
comp_pfloats(const void *pv1, const void *pv2)
{
    float v1;
    float v2;
    v1 = **((float **)(pv1));
    v2 = **((float **)(pv2));
    return v1 < v2 ? -1 : (v1 == v2 ? 0 : 1);
}

/*
 * Initial guess for parameters of
 *	y = params[0] + params[1] * exp(params[2] * x)
 * We first estimate params[2] by noting that
 *	dy/dx = params[2] * params[1] * exp(params[2] * x)
 *	      = params[2] * y - params[2] * params[0]
 * so we approximate dy/dx for each pair of x values by finite differences
 * and fit dy/dx = a*y + b, where a=estimate of params[2].
 * Given params[2], we estimate params[0] and params[1] from the original
 * equation.
 * Note that it is essential to weight the elements in the first fit,
 * because the accuracy of dy/dx will be terrible if the x increment is
 * very small.
 *
 * NOTE:
 * It is assumed that this routine will be called many times with the
 * same x vector each time, but different y vectors.  It does NOT check
 * to make sure the x vector is unchanged.
 */
static int
exp_guess(int npoints,		/* Nbr of data points */
	  int nparams,		/* Nbr of parameters-NOT USED */
	  float *params,	/* Parameter values OUT */
	  int nvars,		/* Number of independent variables-NOT USED */
	  float *x,		/* npoints*nvars values of indep var */
	  float *y,		/* npoints values of dependent variable */
	  float *resid,		/* Quality of fit OUT--OPTIONAL */
	  float *covar)		/* Covariance matrix OUT--OPTIONAL */
{
    int i;
    int j;
    int ndiffs;
    int match;
    float y1;
    float y2;

    static float **xptrs;
    static float *dydx;
    static float *ymid;
    static float *dx;
    static float *wts;
    static float *eax;
    static int *idx = NULL;

    /*
     * First time only:
     *  Allocate memory.
     *  Construct vector giving order of x values.
     *  Construct weight vector.
     */
    if (!idx){

	xptrs = (float **)getmem(npoints*sizeof(float *));
	idx = (int *)getmem(npoints*sizeof(int));
	dydx = (float *)getmem((npoints-1) * sizeof(float));
	ymid = (float *)getmem((npoints-1) * sizeof(float));
	dx = (float *)getmem((npoints-1) * sizeof(float));
	wts = (float *)getmem((npoints-1) * sizeof(float));
	eax = (float *)getmem(npoints * sizeof(float));
	if (!xptrs || !idx || !dydx || !ymid || !dx || !wts || !eax){
	    fprintf(stderr,"FIT: exp_guess(): memory allocation failed\n");
	    return FALSE;
	}

	/* xptrs is a vector of pointers to elements of x */
	for (i=0; i<npoints; i++){
	    xptrs[i] = &x[i];
	}
	/* Sort pointers so that x values pointed to increase monotonically */
	qsort(xptrs, npoints, sizeof(float *), comp_pfloats);
	/* idx is a vector of indices into x (or y) */
	for (i=0; i<npoints; i++){
	    idx[i] = xptrs[i] - &x[0];
	}

	/*
	 * Make a vector of x diffs and weights to use each time.
	 * If there are duplicated x values, we will ignore the data
	 * for all but the first.  This should not be a normal occurance?
	 */
	for (i=ndiffs=0, j=1; j<npoints; i++, j++){
	    dx[i] = x[idx[j]] - x[idx[i]];
	    if (dx[i]){
		/* This may not be optimal weighting. */
		wts[ndiffs++] = dx[i];
	    }
	}
    }

    /*
     * The rest is done every time.
     */

    /* Make vector of differences approximating dy/dx */
    for (i=ndiffs=0, j=1; j<npoints; i++, j++){
	if (dx[i]){
	    y1 = y[idx[i]];
	    y2 = y[idx[j]];
	    ymid[ndiffs] = (y2 + y1) / 2;
	    dydx[ndiffs] = (y2 - y1) / dx[i];
	    ndiffs++;
	}
    }

    /* Find exponent */
    linfit1(ndiffs, ymid, dydx, wts, &params[2], NULL, NULL);

    /* Make vector of exponential terms (known exponent) */
    for (i=0; i<npoints; i++){
	eax[i] = exp(params[2] * x[i]);
	if (!finite(eax[i])){
	    return FALSE;
	}
    }
    linfit1(npoints, eax, y, NULL, &params[1], &params[0], resid);
    if (!finite(params[0]) || !finite(params[1])){
	return FALSE;
    }

    if (covar){
	for (i=0; i < (nparams*(nparams+1)) / 2; i++){
	    covar[i] = 0;
	}
    }
    
    return TRUE;
}

/*
 * Initial guess for parameters of
 *	y =  | params[0] + params[1] * exp(params[2] * x) |
 * 
 * Basically, we modify the data by making the points before the point
 * with the minimum y value all negative, then call exp_guess().
 * NOTE THAT THE DATA IS LEFT MODIFIED FOR THE MAIN FITTING ROUTINE, so that
 * it does not have to deal with the absolute value function, which would
 * lead to discontinuous derivatives.
 */
static int
abs_exp_guess(int npoints,	/* Nbr of data points */
	      int nparams,	/* Nbr of parameters-NOT USED */
	      float *params,	/* Parameter values OUT */
	      int nvars,	/* Number of independent variables-NOT USED */
	      float *x,		/* npoints*nvars values of indep var */
	      float *y,		/* npoints values of dependent variable */
	      float *resid,	/* Residual of fit OUT */
	      float *covar)	/* Covariance matrix OUT--OPTIONAL */
{
    int i;
    int j;
    int n;
    int iofymin;
    float ymin;
    float xofymin;
    float params1[3];
    float resid1;
    float resid2;
    static int nn = 0;
    static int nnn = 0;

    if (nparams > sizeof(params1) / sizeof(params1[0])) {
	fprintf(stderr,"FIT: abs_exp_guess(): Too many parameters in fit.\n");
	return FALSE;
    }

    /* Find smallest y */
    ymin = y[0];
    xofymin = x[0];
    iofymin = 0;
    for (i=1; i<npoints; i++){
	if (y[i] < ymin){
	    ymin = y[i];
	    xofymin = x[i];
	    iofymin = i;
	}
    }

    /* Change sign of y for points with x < xofymin.
     * Do preliminary fits with ymin sign reversed and straight.
     * Use the one with better fit.
     */
    for (i=0; i<npoints; i++){
	if (x[i] < xofymin){
	    y[i] = -y[i];
	}
    }
    i = exp_guess(npoints, nparams, params1, nvars, x, y, &resid1, NULL);
    y[iofymin] = -y[iofymin];
    j = exp_guess(npoints, nparams, params, nvars, x, y, &resid2, NULL);
    if (!i || !j){
	return FALSE;
    }
    if (resid1 < resid2){
	y[iofymin] = -y[iofymin];
	resid2 = resid1;
	for (i=0; i<nparams; i++){
	    params[i] = params1[i];
	}
    }

    if (resid){
	*resid = resid2;
    }
    if (covar){
	for (i=0; i < (nparams*(nparams+1)) / 2; i++){
	    covar[i] = 0;
	}
    }
    
    return TRUE;
}

static int
t1_parfix(int nparams, float *params, float *covar)
{
    /* (Assumes nparams=3)
     * Before processing:
     *   y = params[0] + params[1] * exp(x * params[2])
     *
     * After processing:
     *   y = (M(0) - Mo) * exp(-x/T1) + Mo
     *   T1 = params[0]		Decay time
     *   M(0) = params[1]	M at t=0
     *   Mo = params[2]		M at equilibrium (t=inf)
     */
    float t1;
    float vart1;
    int rtn = TRUE;

    if (params[2]){
	t1 = -1 / params[2];
    }else{
	t1 = 0;
	rtn = FALSE;
    }
    params[1] += params[0];
    params[2] = params[0];
    params[0] = t1;

    if (covar){
	/* Only deal with diagonal terms of covariance matrix! */
	vart1 = t1 * t1 * t1 * t1 * TRI_ELEM(covar,2,2);
	TRI_ELEM(covar,1,1) = (TRI_ELEM(covar,1,1) + TRI_ELEM(covar,0,0)
			       + 2 * TRI_ELEM(covar,1,0));
	TRI_ELEM(covar,2,2) = TRI_ELEM(covar,0,0);
	TRI_ELEM(covar,0,0) = vart1;
    }

    return rtn;
}

#endif /* not FUNCSELECTION */
