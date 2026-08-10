/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef VNMRGRADFITLM_H
#define VNMRGRADFITLM_H

#include <math.h>
#include <stdlib.h>

static double **_lm_alloc_matrix(int ma)
{
    int i;
    double **m = (double **)malloc((size_t)(ma+2)*sizeof(double *));
    if (!m) return NULL;
    double *d = (double *)calloc((size_t)((ma+1)*(ma+1)), sizeof(double));
    if (!d) { free(m); return NULL; }
    for (i = 1; i <= ma; i++) m[i] = d + i*(ma+1);
    return m;
}
static void _lm_free_matrix(double **m) { if (m) { free(m[1]); free(m); } }

#ifndef VNMR_GPL

extern int interuption;

extern void lm_g_init(double x[], double y[], double sig[], int ndata, double a[],
	int ia[], int ma, double **covar, double **alpha, double *chisq,
	void (*funcs)(double, double [], double *, double [], int));
extern void lm_g_iterate(double x[], double y[], double sig[], int ndata, double a[],
	int ia[], int ma, double **covar, double **alpha, double *chisq,
	void (*funcs)(double, double [], double *, double [], int));
extern void lm_g_covar(double x[], double y[], double sig[], int ndata, double a[],
	int ia[], int ma, double **covar, double **alpha, double *chisq,
	void (*funcs)(double, double [], double *, double [], int));

#define _LM_CONV_TOL   0.1
#define _LM_MAX_ITER   500

static int _run_nr_lm(double x[], double y[], double sig[], int ndata,
                      double a[], int ia[], int ma, double **covar_out,
                      double *chisq, int itst_max,
                      void (*funcs)(double, double[], double*, double[], int))
{
    int i, j, k=0, itst=0;
    double ochisq;
    double **alpha = _lm_alloc_matrix(ma);
    double **covar = _lm_alloc_matrix(ma);
    if (!alpha || !covar) {
        Werrprintf("vnmr_lm (NR): matrix alloc failed\n");
        _lm_free_matrix(alpha); _lm_free_matrix(covar); return 1;
    }
    lm_g_init(x,y,sig,ndata,a,ia,ma,covar,alpha,chisq,funcs);
    do {
        if (interuption) {
            Werrprintf("Fit halted\n");
            _lm_free_matrix(alpha); _lm_free_matrix(covar); return 1;
        }
        k++; ochisq = *chisq;
        lm_g_iterate(x,y,sig,ndata,a,ia,ma,covar,alpha,chisq,funcs);
        if (*chisq > ochisq) itst = 0;
        else if (fabs(ochisq - *chisq) < _LM_CONV_TOL) itst++;
    } while (itst < itst_max && k < _LM_MAX_ITER);
    lm_g_covar(x,y,sig,ndata,a,ia,ma,covar,alpha,chisq,funcs);
    for (i=1;i<=ma;i++) for(j=1;j<=ma;j++) covar_out[i][j]=covar[i][j];
    _lm_free_matrix(alpha); _lm_free_matrix(covar); return 0;
}

static int vnmr_lm_poly(double x[], double y[], double sig[], int ndata,
                        double a[], int ia[], int ma,
                        double **covar, double *chisq)
{ return _run_nr_lm(x,y,sig,ndata,a,ia,ma,covar,chisq,100,polyfunc); }

static int vnmr_lm_exp(double x[], double y[], double sig[], int ndata,
                       double a[], int ia[], int ma,
                       double **covar, double *chisq)
{ return _run_nr_lm(x,y,sig,ndata,a,ia,ma,covar,chisq,60,fitfunc); }

#else  /* VNMR_GPL */

#include <gsl/gsl_multifit_nlinear.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>

extern int interuption;

typedef struct {
    int n, ma;
    double *x, *y, *sig;
    void (*funcs)(double, double[], double*, double[], int);
} _GFData;

static int _gf_f(const gsl_vector *p, void *vd, gsl_vector *f)
{
    _GFData *d = (_GFData *)vd;
    double a[d->ma+1], dyda[d->ma+1], ymod;
    for (int j=1;j<=d->ma;j++) a[j]=gsl_vector_get(p,j-1);
    for (int i=0;i<d->n;i++) {
        d->funcs(d->x[i+1],a,&ymod,dyda,d->ma);
        gsl_vector_set(f,i,(ymod-d->y[i+1])/d->sig[i+1]);
    }
    return GSL_SUCCESS;
}
static int _gf_df(const gsl_vector *p, void *vd, gsl_matrix *J)
{
    _GFData *d = (_GFData *)vd;
    double a[d->ma+1], dyda[d->ma+1], ymod;
    for (int j=1;j<=d->ma;j++) a[j]=gsl_vector_get(p,j-1);
    for (int i=0;i<d->n;i++) {
        d->funcs(d->x[i+1],a,&ymod,dyda,d->ma);
        for (int j=1;j<=d->ma;j++)
            gsl_matrix_set(J,i,j-1,dyda[j]/d->sig[i+1]);
    }
    return GSL_SUCCESS;
}
static void _gf_cb(const size_t iter, void *p,
                   const gsl_multifit_nlinear_workspace *w)
{ (void)iter; (void)p; (void)w; }

static int _run_gsl_lm(double x[], double y[], double sig[], int ndata,
                       double a[], int ma, double **covar_out, double *chisq,
                       void (*funcs)(double, double[], double*, double[], int))
{
    int i, j;
    _GFData d = {ndata, ma, x, y, sig, funcs};
    gsl_multifit_nlinear_fdf fdf = {_gf_f,_gf_df,NULL,(size_t)ndata,(size_t)ma,&d};
    gsl_multifit_nlinear_parameters fp = gsl_multifit_nlinear_default_parameters();
    fp.trs = gsl_multifit_nlinear_trs_lm;
    gsl_multifit_nlinear_workspace *ws =
        gsl_multifit_nlinear_alloc(gsl_multifit_nlinear_trust,&fp,
                                   (size_t)ndata,(size_t)ma);
    if (!ws) { Werrprintf("vnmr_lm (GSL): alloc failed\n"); return 1; }
    gsl_vector *p0 = gsl_vector_alloc((size_t)ma);
    for (j=0;j<ma;j++) gsl_vector_set(p0,j,a[j+1]);
    gsl_multifit_nlinear_init(p0,&fdf,ws); gsl_vector_free(p0);
    int info, status = gsl_multifit_nlinear_driver(500,1e-8,1e-8,0.0,
                                                    _gf_cb,NULL,&info,ws);
    if (interuption) {
        Werrprintf("Fit halted\n"); gsl_multifit_nlinear_free(ws); return 1;
    }
    if (status!=GSL_SUCCESS && status!=GSL_EMAXITER) {
        Werrprintf("vnmr_lm (GSL): %s\n",gsl_strerror(status));
        gsl_multifit_nlinear_free(ws); return 1;
    }
    gsl_vector *r = gsl_multifit_nlinear_position(ws);
    for (j=0;j<ma;j++) a[j+1]=gsl_vector_get(r,j);
    gsl_vector *res = gsl_multifit_nlinear_residual(ws);
    gsl_blas_ddot(res,res,chisq);
    gsl_matrix *C = gsl_matrix_alloc((size_t)ma,(size_t)ma);
    if (C) {
        gsl_multifit_nlinear_covar(gsl_multifit_nlinear_jac(ws),0.0,C);
        for (i=0;i<ma;i++) for(j=0;j<ma;j++)
            covar_out[i+1][j+1]=gsl_matrix_get(C,i,j);
        gsl_matrix_free(C);
    }
    gsl_multifit_nlinear_free(ws); return 0;
}

static int vnmr_lm_poly(double x[], double y[], double sig[], int ndata,
                        double a[], int ia[], int ma,
                        double **covar, double *chisq)
{ (void)ia; return _run_gsl_lm(x,y,sig,ndata,a,ma,covar,chisq,polyfunc); }

static int vnmr_lm_exp(double x[], double y[], double sig[], int ndata,
                       double a[], int ia[], int ma,
                       double **covar, double *chisq)
{ (void)ia; return _run_gsl_lm(x,y,sig,ndata,a,ma,covar,chisq,fitfunc); }

#endif  /* VNMR_GPL */

#endif
