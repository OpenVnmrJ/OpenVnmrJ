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

#include "imagemath.h"
#include <stdarg.h>
#include <math.h>

/* Gets the element row,col from a lower triangular matrix "m" that
 * is stored in the order m00, m10, m11, m20, m21, m22, m30, ...
 * Note that we require row >= col. */
#define TRI_ELEM(m, row, col) (m[((row)*((row)+1))/2 + (col)])

static int default_pars_set = 0;
static double *default_par_values = NULL;
static double pmin[] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
static double pmax[] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};

extern void fit_images(FDFptr *in_object, int n_infiles,
		       double *xvars, int n_xvars,
		       double thresh, double chisq, double snThresh,
		       int width, int height, int depth,
		       FDFptr *out_object, int n_outfiles, int want_output(),
		       int lin, int npars, int use_prev,
		       void func(), void jac(),
		       int method(), int guess(), int parfix());

extern int linfit_setup();
extern int linfit_go();
extern int linfit();
extern int divset_();
extern int dn2g_();
extern int dn2f_();

extern int marquardt();
extern int interrupt(void);
extern int interrupt_begin(void);
extern int interrupt_end(void);

void linfit1(int npts, double *x, double *y, double *w,
	     double *m, double *b, double *resid);

#include "userfit.c"

static int
fixed_guess(int npts, int npars, double *p, int nvars,
	    double *x, double *y, double *resid, double *covar)
{
    int i;
    static int firsttime = TRUE;

    if (nbr_params < npars && default_pars_set < npars){
	if (firsttime){
	    firsttime = FALSE;
	    ib_errmsg("MATH: fit: Initial parameter values not found");
	}
	return FALSE;
    }
    /* Get as many values from command line as available */
    for (i=0; i<npars && i<nbr_params; i++){
	p[i] = in_params[i];
    }
    /* Get the rest from hard-coded defaults */
    for ( ; i<npars; i++){
	p[i] = default_par_values[i];
    }
    if (resid){
	*resid = 0;
    }
    if (covar){
	for (i=0; i < (npars*(npars+1)) / 2; i++){
	    covar[i] = 0;
	}
    }

    return TRUE;
}

static void
set_default_parameters(int nargs, int *argtypep, ...)
{
    int i;
    va_list vargs;

    default_par_values = (double *)malloc(nargs * sizeof(double));
    default_pars_set = nargs;

    va_start(vargs, argtypep);
    for (i=0; i<nargs; i++){
	default_par_values[i] = va_arg(vargs, double);
    }
    va_end(vargs);
}

double *
set_xvars(FDFptr *ddl_objects, int npoints, char *name, int *nvars)
{
    int i;
    double *values;
    double dbl;

    if (!ddl_objects || npoints < 1 || !name){
	return NULL;
    }
    values = (double *)getmem(npoints * sizeof(double));
    if (!values){
	return NULL;
    }
    for (i=0; i<npoints; i++){
	if (!get_header_double(ddl_objects[i], name, &dbl)){
	    return NULL;
	}
	values[i] = dbl;
    }
    *nvars = 1;
    return values;
}

static int
getfunction(char *str,
	    int *nparms_default,
	    int *use_prev_params,
	    int *fit_type,
	    void (**function)(),
	    void (**jacobian)(),
	    int (**guess)(),
	    int (**parfix)())
{
#define IF_FITCODE(s) if (strcasecmp(str,(s)) == 0)
#define N_PARAMETERS *nparms_default
#define FIT_TYPE *fit_type
#define FUNCTION *function
#define JACOBIAN *jacobian
#define GUESS *guess
#define USE_PREVIOUS_PARAMETERS *use_prev_params
#define PARFIX *parfix
#define FUNCSELECTION
#include "userfit.c"
#undef FUNCSELECTION

    return FALSE;
}

int
mathfunc()
{
    int i;
    double val;
    double *xvars;		/* Values of the independent variable(s) */
    int nvars;			/* Nbr of independent variables */
    int np = 0;			/* Number of numerical parameters found */
    double threshold = 0;	/* Ignore pixels below this intensity */
    double sigLev = 1; // Significance level to set pixel's fit value (1=no significance)
    double chisq = 0; // Chi-square -- alternative to sigLev, if set
    double snThresh = 0; // min S/N to set value of parameter pixel
    int nparams = 0;		/* Nbr of parameters in fit */
    char *xname = "ti";
    char msg[256];
    char *str;

    int quick = FALSE;
    int noderiv = FALSE;
    int gotfun = FALSE;
    int fit_type = NONLINEAR;
    int use_prev_params = FALSE;
    int prev = FALSE;
    int noprev = FALSE;
    int pdone;

    void (*function)() = NULL;
    void (*jacobian)() = NULL;
    int (*guess)() = NULL;
    int (*parfix)() = NULL;
    int (*method)() = NULL;
    double *(*xvarfunc)() = set_xvars;

    int arg = 2;
    extern double d1mach_();

    if (in_vec_len[0]<1){
	ib_errmsg("MATH: fit: No input images");
	return FALSE;
    }
    if (input_sizes_differ){
	ib_errmsg("MATH: fit: Input image sizes differ");
	return FALSE;
    }
    if (!want_output(0)){
	ib_errmsg("MATH: fit: No frame for first output image");
	return FALSE;
    }

    /* Read numerical parameters (nothing to do with params of the fit!) */
    pdone = FALSE;
    for (i=0; i<nbr_params && !pdone; i++){
	val = in_params[i];
	switch (i){
	  case 0:
	    threshold = val;
	    pdone = TRUE;	/* Last parameter to read */
	    break;
	}
    }
    nbr_params -= i;
    in_params += i;

    /* Read string parameters */
    gotfun = FALSE;
    for (i=0; i<nbr_strings; i++){
	str = in_strings[i];
	if (!gotfun && getfunction(str, &nparams, &use_prev_params, &fit_type,
				   &function, &jacobian, &guess, &parfix))
	{
	    /* Got a functional form */
	    gotfun = TRUE;
	}else if (!quick && strcasecmp(str,"quick") == 0){
	    /* Use "quick" mode */
	    quick = TRUE;
	}else if (!noderiv && strcasecmp(str,"noderiv") == 0){
	    /* Do not use derivative, even if available */
	    noderiv = TRUE;
	}else if (!prev && strcasecmp(str,"prev") == 0){
	    /* Use previous parameter values for estimates */
	    prev = TRUE;
	}else if (!noprev && strcasecmp(str,"noprev") == 0){
	    /* Do not use previous parameter values for estimates */
	    noprev = TRUE;
	} else if (strncasecmp(str, "p=", 2) == 0) {
	    val = atof(str+2);
	    if (val != 0) {
	        val = val < 1e-20 ? 1e-20 : (val > 1 ? 1 : val);
	        sigLev = val;
	    }
        } else if (strncasecmp(str, "chisq=", 6) == 0) {
            val = atof(str+6);
            if (val != 0) {
                chisq = val;
            }
        } else if (strncasecmp(str, "snThresh=", 9) == 0) {
            val = atof(str+9);
            if (val != 0) {
                snThresh = val;
            }
	}else{
	    /* None of the above--assume independent variable name */
	    xname = str;
	}
    }

    /* Do not write to more output files than we can usefully use */
    if (nparams){
	int maxout;
	maxout = 2 * nparams + 1;
	if (maxout<nbr_outfiles) nbr_outfiles = maxout; /* Change global var */
    }
    create_output_files(nbr_outfiles, in_object[0]);

    /* Check the setup */
    if (!gotfun){
	ib_errmsg("MATH: fit: No known fit type specified");
	return FALSE;
    }

    if (noderiv){
	jacobian = NULL;
    }

    if (prev){
	use_prev_params = TRUE;
    }else if (noprev){
	use_prev_params = FALSE;
    }

    if (quick || !function){
	method = NULL;
    }else{
	method = marquardt;
    }

    /* Set the independent variable */
    xvars = (*xvarfunc)(in_object, in_vec_len[0], xname, &nvars);
    if (!xvars){
	sprintf(msg,"MATH: No values for independent variable \"%.200s\"",
                xname);
	ib_errmsg(msg);
	return FALSE;
    }

    if (chisq == 0) {
        chisq = chisqCompInv(sigLev, in_vec_len[0] - nparams + 1);
    }

    fit_images(in_object, in_vec_len[0], xvars, nvars,
	       threshold, chisq, snThresh, img_width, img_height, img_depth,
	       out_object, nbr_outfiles, want_output, fit_type,
	       nparams, use_prev_params,
	       function, jacobian, method, guess, parfix);

    write_output_files();

    return TRUE;
}

void
fit_images(FDFptr *in_object, int n_images,
	   double *xvars, int nbr_xvars,
	   double threshold, double chisq, double snThreshold,
	   int width, int height, int depth,
	   FDFptr *out_object, int n_out, int want_output(),
	   int fit_type, int nparams, int use_prev_params,
	   void function(), void jacobian(),
	   int method(), int guess(), int parfix())
{
    int i;
    int j;
    int k;
    int okflag;
    int size;
    int threshok;
    int max_outfiles;
    int setuperr = 0;
    int prev_params_set = FALSE;
    double *prev_params;
    double resid;
    double *p_resid;
    double sosFit; // sum-of-squares deviation from fit for one pixel
    char msg[256];

    double *covar;
    float **data;
    float **odata;
    double *params;
    double *y;

    data = (float **)getmem(n_images * sizeof(float *));
    odata = (float **)getmem(n_out * sizeof(float *));
    params = (double *)getmem(nparams * sizeof(double));
    y = (double *)getmem(n_images * sizeof(double));
    if (use_prev_params){
	prev_params = (double *)getmem(nparams * sizeof(double));
    }

    //p_resid = want_output(nparams) ? &resid : NULL;
    p_resid = &resid;
    max_outfiles = 2 * nparams + 1; /* params, rms residual, param sigmas */

    // NB: Always calculate covariances
    covar = (double *)getmem((sizeof(double) * nparams * (nparams + 1)) / 2);

    for (i=0; i<n_images; i++){
	data[i] = get_ddl_data(in_object[i]);
    }

    for (i=0; i<n_out; i++){
	odata[i] = get_ddl_data(out_object[i]);
    }

    if (fit_type & (LINEAR_FIXED | LINEAR_RECALC_OFFSET)){
	pixel_indx = 0;
	setuperr = linfit_setup(xvars, NULL, n_images,
				nparams, nbr_xvars, function);
    }

    interrupt_begin();
    size = width * height * depth;
    for (pixel_indx=0; pixel_indx<size; pixel_indx++){
        if (pixel_indx%width == 0){
            sprintf(msg,"Math: image line #%d", pixel_indx/width);
            ib_msgline(msg);
        }
        if (!interrupt()){
            // NB: Break out of loop over images as soon as we get threshok
            for (threshok=FALSE, j=0; j<n_images && !threshok; j++){
                threshok = fabs(data[j][pixel_indx]) >= threshold;
            }
        }
        if (!threshok || interrupt() || setuperr){
            /* Zero pixel in all output images */
            for (j=0; j<n_out; j++){
                if (want_output(j)){
                    odata[j][pixel_indx] = 0;
                }
            }
        }else{
            for (j=0; j<n_images; j++){
                y[j] = data[j][pixel_indx];
            }
            if (fit_type & (LINEAR_FIXED | LINEAR_RECALC_OFFSET)){
                okflag = linfit_go(xvars, y, n_images, nparams, nbr_xvars,
                        fit_type==LINEAR_RECALC_OFFSET,
                        function,
                        params, p_resid, covar);
            }else if (fit_type & LINEAR_RECALC){
                okflag = linfit(xvars, y, NULL, n_images,
                        nparams, nbr_xvars, function,
                        params, p_resid, covar);
            }else{ /* NONLINEAR */
                if (prev_params_set){
                    okflag = TRUE;
                    for (j=0; j<nparams; j++){
                        params[j] = prev_params[j];
                    }
                }else if (guess){
                    okflag = (*guess)(n_images, nparams, params,
                            nbr_xvars, xvars, y, p_resid, covar);
                }else if (default_pars_set >= nparams){
                    for (j=0; j<nparams; j++){
                        params[j] = default_par_values[j];
                    }
                    okflag = TRUE;
                }else{
                    okflag = FALSE;
                }
                if (okflag && method){
                    okflag = (*method)(n_images, y, nparams, params,
                            nbr_xvars, xvars, function, jacobian,
                            p_resid, covar);
                    if (okflag <= 0 && prev_params_set){
                        /* Fit failed using previous params for guess.
                         * Try another guess. */
                        if (guess){
                            okflag = (*guess)(n_images, nparams, params,
                                    nbr_xvars, xvars, y,
                                    p_resid, covar);
                        }else if (default_pars_set >= nparams){
                            for (j=0; j<nparams; j++){
                                params[j] = default_par_values[j];
                            }
                            okflag = TRUE;
                        }
                        okflag = (*method)(n_images, y, nparams, params,
                                nbr_xvars, xvars, function, jacobian,
                                p_resid, covar);
                    }
                }
	    }
	    // NB: resid == p_resid
            // Calculate sum-of-squares deviation from fit
            // Convert RMS back into sum-of-squares
            sosFit = resid * resid * n_images;
	    if (okflag <= 0){
		/* Fit failed */
		for (k=0; k<nparams; k++){
		    params[k] = 0;
		}
		resid = 0;
		if (covar){
		    for (k=0; k<(nparams * (nparams+1))/2; k++){
			covar[k] = 0;
		    }
		}
	    }else{
		if (use_prev_params){
		    /* Save starting params for next fit */
		    prev_params_set = TRUE;
		    for (k=0; k<nparams; k++){
			prev_params[k] = params[k];
		    }
		}
		if (parfix){
		    (*parfix)(nparams, params, covar);
		}
	    }

	    /* Write parameter info */
	    for (j=0; j<n_out && j<nparams; j++){
		if (want_output(j)){
		    odata[j][pixel_indx] = (float)params[j];
		}
	    }

            // Calculate sum-of-squares deviation from average value
            double r0, sos0, avg;

            sos0 = 0;
            for (j=0; j<n_images; j++){
                sos0 += data[j][pixel_indx];
            }
            avg = sos0 / n_images;
            sos0 = 0;
            for (j = 0; j < n_images; j++) {
                r0 = data[j][pixel_indx] - avg;
                sos0 += r0 * r0;
            }

            /* Residual info, if requested */
            if (want_output(nparams)) {
                odata[nparams][pixel_indx] = (float)resid;
            }
            /* Covariance info, if requested */
            if (covar){
                k = 0;
                for (j=nparams+1; j<n_out && j<max_outfiles; j++, k++){
                    if (want_output(j)){
                        /* Give sigma instead of variance.
                         * "fabs" _should_ never be necessary. */
                        odata[j][pixel_indx] = (float)sqrt(fabs(TRI_ELEM(covar,k,k)));
                    }
                }
            }

            // Check if parameter value is good enough
            if (sos0 / (resid * resid) < chisq) {
                // Overall fit is lousy by chisq test
                // Zero out all parameter data for this pixel
                for (j = 0; j < n_out && j < nparams; j++) {
                    if (want_output(j)){
                        odata[j][pixel_indx] = 0;
                    }
                }
            } else if (covar) {
                for (j = 0; j < n_out && j < nparams; j++) {
                    if (want_output(j)) {
                        double var = sqrt(fabs(TRI_ELEM(covar,j,j)));
                        double val = fabs(odata[j][pixel_indx]);
                        if (var > 0) {
                            if (val / var < snThreshold) {
                                // This parameter is badly estimated here
                                // Zero out this parameter for this pixel
                                odata[j][pixel_indx] = 0;
                            }
                        }
                    }
                }
            }
            // Check if parameter values are within limits
            for (j = 0; j < n_out && j < nparams; j++) {
                if (want_output(j)) {
                    double val = odata[j][pixel_indx];
                    if (val < pmin[j]) {
                        odata[j][pixel_indx] = (float)pmin[j];
                    } else if (val > pmax[j]) {
                        odata[j][pixel_indx] = (float)pmax[j];
                    }
                }
            }
        }
    }
    interrupt_end();
}

/*
 * Linear fit to one variable with weights: y = mx + b
 * Input: vectors "x", "y", and "w" of length "npts".
 *	If w is NULL, uses unit weights.
 * Output: "m", "b", and "r" (the RMS residual).  Any or all
 *	of the output pointers may be NULL if output is not desired.
 */
void
linfit1(int npts, double *x, double *y, double *w, double *m, double *b, double *r)
{
    int i;
    double *px;
    double *py;
    double *pw;
    double *yend;
    double sumwt;
    double mm;
    double bb;
    double invsigma;

    double xsum;
    double ysum;
    double dx;
    double dy;
    double xmean;
    double sumdx2;

    /* Find means */
    xsum = ysum = sumwt = 0;
    if (w){
	for (px=x, py=y, pw=w, yend=y+npts; py<yend; ){
	    xsum += *px++ * *pw;
	    ysum += *py++ * *pw;
	    sumwt += *pw++;
	}
    }else{
	for (px=x, py=y, yend=y+npts; py<yend; ){
	    xsum += *px++;
	    ysum += *py++;
	}
	sumwt = npts;
    }
    xmean = xsum / sumwt;

    /* Find slope */
    mm = sumdx2 = 0;
    if (w){
	for (px=x, py=y, pw=w, yend=y+npts; py<yend; ){
	    invsigma = sqrt(*pw++);
	    dx = (*px++ - xmean) * invsigma;
	    sumdx2 += dx * dx;
	    mm += dx * *py++ * invsigma;
	}
    }else{
	for (px=x, py=y, yend=y+npts; py<yend; ){
	    dx = *px++ - xmean;
	    sumdx2 += dx * dx;
	    mm += dx * *py++;
	}
    }
    mm /= sumdx2;
    bb = (ysum - mm * xsum) / sumwt;

    /* Set return values */
    if (m) *m = mm;
    if (b) *b = bb;

    /* Calculate RMS residual, if requested */
    if (r){
	if (w){
	    for (*r=0, px=x, py=y, pw=w, yend=y+npts; py<yend; ){
		dy = *py++ - (mm * *px++ + bb);
		*r += *pw++ * dy * dy;
	    }
	}else{
	    for (*r=0, px=x, py=y, yend=y+npts; py<yend; ){
		dy = *py++ - (mm * *px++ + bb);
		*r += dy * dy;
	    }
	}
	*r = sqrt(fabs(*r / sumwt));
    }
}

/*
 * Interface to Port3 routines
 */

static void (*gbl_function)();
static void (*gbl_jacobian)();

static int n2g_resid_func(int *npts, int *nparms, double *x, int *ncalls,
			  double *resid,
			  int *usr_ints, double *usr_doubles, int (*usr_func)());

static int n2g_deriv_func(int *npts, int *nparms, double *x, int *ncalls,
			  double *derivs,
			  int *usr_ints, double *usr_doubles, int (*usr_func)());

int
marquardt(int npoints, /* Nbr of data points */
	  double *y, /* npoints values of dependent var */
	  int nparams, /* Nbr of parameters */
	  double *params, /* IN/OUT nparams parameter values */
	  int nvars, /* Number of independent variables */
	  double *x, /* npoints*nvars values of indep var */
	  void function(),
	  void jacobian(),
	  double *resid, /* OUT: RMS residual of result */
	  double *covar) /* OUT: C11, C21, C22, C31, C32, C33, C41, ... */
{
    int i;
    int j;
    int k;
    int one = 1;
    static int ilen;
    static int flen;
    static int firsttime = TRUE;
    static int *iwork;
    static double *fwork;
    static double *usr_doubles;
    static int ntimes = 0;

    gbl_function = function;
    gbl_jacobian = jacobian;

    if (firsttime){
	firsttime = FALSE;
	/* Allocate working buffers */
	ilen = 82 * nparams;
	flen = 105 + nparams * (npoints + 2 * nparams + 17) + 2 * npoints;
	iwork = (int *)getmem(ilen * sizeof(int));
	fwork = (double *)getmem(flen * sizeof(double));
	usr_doubles = (double *)getmem((1 + nvars) * npoints * sizeof(double));

	/* Init independent variable array */
	for (i=0; i<npoints; i++){
	    usr_doubles[i] = x[i];
	}
    }

    /* Set operating modes (needs to be done every time!) */
    divset_(&one, iwork, &ilen, &flen, fwork);/* Get default parameters */
    iwork[21-1] = 0;	/* Turn off all printing */
    iwork[18-1] = 30;	/* Max iterations allowed */

    fwork[32-1] = 1.0e-8;	/* Relative convergence tolerance */
    fwork[33-1] = 1.0e-8;	/* X-convergence tolerance */

    if (covar){
	/* Note: Covariance calc increases time by about 50% */
	iwork[57-1] = 1;	/* Calc covariance; no regression diags */
    }else{
	iwork[57-1] = 0;	/* Do not calculate covariance, etc. */
    }

    /* Initialize dependent variable array */
    for (i=0; i<npoints; i++){
	usr_doubles[npoints + i] = y[i];
    }

    if (jacobian){
        //ib_errmsg("MATH: fit.c: calling nd2g_");
        // double: params(4), fwork(9), usr_doubles(11)
	dn2g_(&npoints, &nparams, params, n2g_resid_func, n2g_deriv_func,
	     iwork, &ilen, &flen, fwork,
	     NULL, usr_doubles, NULL);
    }else{
        //ib_errmsg("MATH: fit.c: calling n2f_");
	dn2f_(&npoints, &nparams, params, n2g_resid_func,
	     iwork, &ilen, &flen, fwork,
	     NULL, usr_doubles, NULL);
    }

    if (resid){
        /* RMS residual of final fit */
	*resid = sqrt(fwork[10-1] * 2 / npoints);
    }

    if (covar){
	/* fwork stores lower triangle of covariance array in order:
	 *	 C11, C21, C22, C31, C32, C33, C41, ...
	 */
	k = iwork[26-1];	/* Location of covariances in fwork array */
	if (k>0){
	    k--;
	    for (i=0; i < (nparams * (nparams+1)) / 2; i++){
		covar[i] = fwork[k++];
	    }
	}else{
	    /* Covariance not available */
	    for (i=0; i < (nparams * (nparams+1)) / 2; i++){
		covar[i] = 0;
	    }
	}
    }

    if (iwork[1-1] >= 3 && iwork[1-1] <= 6){
	return 1;
    }else{
	/* Error return--do not report specific error number */
	return 0;
    }
}

int
n2g_resid_func(int *npts, int *nparms, double *parms, int *ncalls,
	       double *resid,
	       int *usr_ints, double *usr_doubles, int (*usr_func)())
{
    int i;
    double *y;
    double *x;

    x = usr_doubles;
    y = usr_doubles + *npts;

    /* Function specified by "gbl_function" puts values into "resid" */
    (*gbl_function)(*npts, *nparms, parms, 1, x, resid);
    /* Convert to residuals */
    for (i=0; i<*npts; i++) {
	resid[i] -= y[i];
	if (!finite(resid[i])){
	    /* Function undefined here -- tell optimizer to back off */
	    *ncalls = 0;
	}
    }
    return 0;
}

int
n2g_deriv_func(int *npts, int *nparms, double *parms, int *ncalls,
	       double *jvector,
	       int *usr_ints, double *usr_doubles, int (*usr_func)())
{
    int i;
    int j;
    static double **dydp = NULL;
    /* dydp is a vector of pointers to derivative vector for each paramter */
    if (!dydp){
	dydp = (double **)getmem(*npts * *nparms * sizeof(double *));
    }
    for (i=0; i<*nparms; i++){
	dydp[i] = jvector + i * *npts;
    }
    (*gbl_jacobian)(*npts, *nparms, parms, 1, usr_doubles, dydp);

    for (i=0; i<*npts; i++){
	for (j=0; j<*nparms; j++){
	    if (!finite(dydp[j][i])){
		/* Gradient undefined here -- error return */
		*ncalls = 0;
	    }
	}
    }
    return 0;
}
