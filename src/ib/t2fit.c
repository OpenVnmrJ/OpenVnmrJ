/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef FUNCSELECTION
IF_FITCODE("t2"){
    N_PARAMETERS = 2;
    FUNCTION = t2_function;
    JACOBIAN = t2_jacobian;
    GUESS = t2_guess;

    return TRUE;
}
#else /* not FUNCSELECTION */

/* ---------------------------------------------------------------------------*/
/* FUNCTION DEFINITIONS - model for t2 experiments */
/* ---------------------------------------------------------------------------*/
static void
t2_function(int npts, int npars, float *p, int nvars, float *x, float *y)
{
    int i;
    for (i=0; i<npts; i++){
	y[i] = p[1] * exp(-x[i] / p[0]);
    }
}

static void
t2_jacobian(int npts, int npars, float *p, int nvars, float *x, float **dydp)
{
    int i;

    for (i=0; i<npts; i++){
	dydp[1][i] = exp(-x[i] / p[0]);
	dydp[0][i] = dydp[1][i] * x[i] * p[1] / (p[0] * p[0]);
    }
}

/* ------------------GUESS FOR T2 FUNCTION----------------------
 * Initial guess for parameters of
 *
 *	y = p[1] * exp(-x / p[0])
 *
 * We note first that:
 * 
 *	ln(y) = ln(p[1] * exp(-x / p[0]))
 *	      = ln(p[1]) - x / p[0],  
 *
 * so we compute ln(y) for each x value and apply weighted linear
 * regression to estimate ln(y) = m*x + b
 *
 * Transforming back, we get:
 * 
 *      p[0] = -1/m
 *      p[1] = exp(b)
 *     
 * NOTE:
 *    It is assumed that this routine will be called many times with the
 *    same number of points each time.  It does NOT check
 *    to make sure the number of points is unchanged.
 -------------------------------------------------------------------*/
static int
t2_guess(int npts,		/* Nbr of data points */
	  int npars,		/* Nbr of parameters-NOT USED */
	  float *p,		/* Parameter values OUT */
	  int nvars,		/* Number of independent variables-NOT USED */
	  float *x,		/* npts*nvars values of indep var */
	  float *y,		/* npts values of dependent variable */
	  float *resid,		/* Quality of fit OUT--OPTIONAL */
	  float *covar)		/* Covariance matrix OUT--OPTIONAL */
{
    int i;
    int n;
    float m;
    float b;
    float ymax;
    static float *lny = NULL;
    static float *wt;

    /* First time only: Allocate memory */
    if (!lny){
	lny = (float *)getmem(npts*sizeof(float));
	wt = (float *)getmem(npts*sizeof(float));
	if (!lny || !wt) {
	    fprintf(stderr,"FIT: t2_guess(): memory allocation failed\n");
	    return FALSE;
	}
    }

    /* Make vectors of ln(y) values and weights.
     * Weights are y[i]**2 because deviations in ln(y) are approx.
     * d/dy(ln(y)) = 1/y times the deviations in y. Since the error sum
     * is in (dy)**2, the weights should be y**2. Of course, this is only
     * approximate: the weights should be on the y of the fit rather than
     * the y of the data points, and the analysis only holds for small errors.
     */
    for (ymax = y[0], i=1; i<npts; i++){
	if (ymax < y[i]) ymax = y[i];
    }
    if (ymax < 0){
	return FALSE;
    }
    for (i=n=0; i<npts; i++){
	if (y[i] > 0.01 * ymax){
	    lny[i] = log(y[i]);
            wt[i] = y[i] * y[i];
	    n++;
	}else{
	    wt[i] = lny[i] = 0;
	}
    }
    if (n<2){
	return FALSE;
    }

    /* Do linear fit:  lny = m*x + b */
    /* Note: residuals calculated by linfit would be more or less useless */
    linfit1(npts, x, lny, wt, &m, &b, NULL);
    p[0] = -1/m;
    p[1] = exp(b);

    if (!finite(p[0]) || !finite(p[1])){
        ib_errmsg("MATH: t2fit: Bad estimated parameters");
	return FALSE;
    }

    if (resid){
	float rsum;
	float dy;
	for (rsum=0, i=0; i<npts; i++){
	    dy = y[i] - p[1] * exp(-x[i] / p[0]);
	    rsum += dy * dy;
	}
	*resid = sqrt(rsum / npts);
    }

    if (covar){
	for (i=0; i < (npars*(npars+1)) / 2; i++){
	    covar[i] = 0;
	}
    }

    return TRUE;
}

#endif /* not FUNCSELECTION */
