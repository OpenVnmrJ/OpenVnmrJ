/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef FUNCSELECTION
if (strcasecmp(str,"qt1") == 0){
    if (nbr_params < 1){
	N_PARAMETERS = 3;
	FUNCTION = qt1_function;
	JACOBIAN = qt1_jacobian;
	GUESS = qt1_guess;
	pmin[0] = 0;
	return TRUE;
    }else{
	qfixed = in_params[0];
	N_PARAMETERS = 2;
	FUNCTION = qfixed_t1_function;
	JACOBIAN = qfixed_t1_jacobian;
	GUESS = qfixed_t1_guess;
	pmin[0] = 0;
	return TRUE;
    }
}
if (strcasecmp(str,"absqt1") == 0){
    if (nbr_params < 1){
	N_PARAMETERS = 3;
	FUNCTION = qt1_function;
	JACOBIAN = qt1_jacobian;
	GUESS = abs_qt1_guess;
	pmin[0] = 0;
	return TRUE;
    }else{
	qfixed = in_params[0];
	N_PARAMETERS = 2;
	FUNCTION = qfixed_t1_function;
	JACOBIAN = qfixed_t1_jacobian;
	GUESS = abs_qfixed_t1_guess;
	pmin[0] = 0;
	return TRUE;
    }
}
#else /* not FUNCSELECTION */

static double qfixed = 1;

static void
qt1_function(int npts, int npars, double *p, int nvars, double *x, double *y)
{
    int i;
    for (i=0; i<npts; i++){
	y[i] = p[1] * (1 - 2 * p[2] * exp(-x[i] / p[0]));
    }
}

static void
qt1_jacobian(int npts, int npars, double *p, int nvars, double *x, double **dydp)
{
    int i;
    double et;
    double pp;

    pp = p[1] * p[2] / (p[0] * p[0]);
    for (i=0; i<npts; i++){
	et = 2 * exp(-x[i] / p[0]);
	dydp[0][i] = -pp * x[i] * et;
	dydp[1][i] = 1 - p[2] * et;
	dydp[2][i] = -p[1] * et;
    }
}

static int
qt1_guess(int npts, int npars, double *params,
	   int nvars, double *x, double *y, double *resid, double *covar)
{
    int rtn;
    double p[3];
    if (rtn = exp_guess(npts, npars, p,
			nvars, x, y, resid, covar))
    {
	if (p[0] != 0 && p[2] != 0){
	    params[0] = -1.0 / p[2];
	    params[1] = p[0];
	    params[2] = -0.5 * p[1] / p[0];
	}else{
	    rtn = FALSE;
	}
    }
    return rtn;
}

static int
abs_qt1_guess(int npts, int npars, double *params,
	      int nvars, double *x, double *y, double *resid, double *covar)
{
    int rtn;
    double p[3];
    if (rtn = abs_exp_guess(npts, npars, p,
			    nvars, x, y, resid, covar))
    {
	if (p[0] != 0 && p[2] != 0){
	    params[0] = -1.0 / p[2];
	    params[1] = p[0];
	    params[2] = -0.5 * p[1] / p[0];
	}else{
	    rtn = FALSE;
	}
    }
    return rtn;
}

static void
qfixed_t1_function(int npts, int npars, double *p,
		    int nvars, double *x, double *y)
{
    int i;
    for (i=0; i<npts; i++){
	y[i] = p[1] * (1 - 2 * qfixed * exp(-x[i] / p[0]));
    }
}

static void
qfixed_t1_jacobian(int npts, int npars, double *p,
		    int nvars, double *x, double **dydp)
{
    int i;
    double et;
    double pp;

    pp = p[1] * qfixed / (p[0] * p[0]);
    for (i=0; i<npts; i++){
	et = 2 * exp(-x[i] / p[0]);
	dydp[0][i] = -pp * x[i] * et;
	dydp[1][i] = 1 - qfixed * et;
    }
}

static int
qfixed_t1_guess(int npts, int npars, double *params,
		 int nvars, double *x, double *y, double *resid, double *covar)
{
    int rtn;
    double p[3];
    if (rtn = exp_guess(npts, npars, p,
			nvars, x, y, resid, covar))
    {
	if (p[0] != 0 && p[2] != 0){
	    params[0] = -1.0 / p[2];
	    params[1] = p[0];
	}else{
	    rtn = FALSE;
	}
    }
    return rtn;
}

static int
abs_qfixed_t1_guess(int npts, int npars, double *params,
		    int nvars, double *x, double *y, double *resid, double *covar)
{
    int rtn;
    double p[3];
    if (rtn = abs_exp_guess(npts, npars, p,
			    nvars, x, y, resid, covar))
    {
	if (p[0] != 0 && p[2] != 0){
	    params[0] = -1.0 / p[2];
	    params[1] = p[0];
	}else{
	    rtn = FALSE;
	}
    }
    return rtn;
}

#endif /* not FUNCSELECTION */
