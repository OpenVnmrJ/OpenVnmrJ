/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef VNMRGXYZCALIBLM_H
#define VNMRGXYZCALIBLM_H

#include <math.h>
#include <stdlib.h>

static double **_lmx_alloc_matrix(int ma)
{
    int i;
    double **m = (double **)malloc((size_t)(ma+2)*sizeof(double *));
    if (!m) return NULL;
    double *d = (double *)calloc((size_t)((ma+1)*(ma+1)), sizeof(double));
    if (!d) { free(m); return NULL; }
    for (i = 1; i <= ma; i++) m[i] = d + i*(ma+1);
    return m;
}
static void _lmx_free_matrix(double **m) { if (m) { free(m[1]); free(m); } }

#ifndef VNMR_GPL

extern void lm_gxyz_init(double x1[],double y1[], double w[], double sig[],
    int ndata, double a[], int ia[], int ma, double **covar, double **alpha,
    double *chisq, void (*funcs)(double, double, double [], double *,
    double [], int));

extern void lm_gxyz_iterate(double x1[],double y1[], double w[], double sig[],
    int ndata, double a[], int ia[], int ma, double **covar, double **alpha,
    double *chisq, void (*funcs)(double, double, double [], double *,
    double [], int));

extern void lm_gxyz_covar(double x1[],double y1[], double w[], double sig[],
    int ndata, double a[], int ia[], int ma, double **covar, double **alpha,
    double *chisq, void (*funcs)(double, double, double [], double *,
    double [], int));

#define _LMX_CONV_TOL   0.1
#define _LMX_CONV_COUNT 4
#define _LMX_MAX_ITER   200

static int _run_nr_lm2(double x1[], double y1[], double obs[], double sig[],
                       int ndata, double a[], int ia[], int ma,
                       double **covar_out, double *chisq,
                       void (*funcs)(double, double, double[], double*,
                                     double[], int))
{
    int i, j, k=0, itst=0, mfit=0;
    double ochisq;
    double **alpha = _lmx_alloc_matrix(ma);
    double **covar = _lmx_alloc_matrix(ma);
    if (!alpha || !covar) {
        Werrprintf("vnmr_lm_width/midpoint (NR): matrix alloc failed\n");
        _lmx_free_matrix(alpha); _lmx_free_matrix(covar); return 1;
    }
    lm_gxyz_init(x1,y1,obs,sig,ndata,a,ia,ma,covar,alpha,chisq,funcs);
    do {
        k++; ochisq = *chisq;
        lm_gxyz_iterate(x1,y1,obs,sig,ndata,a,ia,ma,covar,alpha,chisq,funcs);
        if (*chisq > ochisq) itst = 0;
        else if (fabs(ochisq - *chisq) < _LMX_CONV_TOL) itst++;
    } while (itst < _LMX_CONV_COUNT && k < _LMX_MAX_ITER);
    lm_gxyz_covar(x1,y1,obs,sig,ndata,a,ia,ma,covar,alpha,chisq,funcs);
    /* undo NR internal chisq/(n-mfit) scaling */
    for (i=1;i<=ma;i++) if (ia[i]) mfit++;
    { int dof=ndata-mfit;
      double sc2=(dof>0 && *chisq>0.0)?(*chisq/(double)dof):1.0;
      for (i=1;i<=ma;i++) for(j=1;j<=ma;j++) covar_out[i][j]=covar[i][j]*sc2;
    }
    _lmx_free_matrix(alpha); _lmx_free_matrix(covar); return 0;
}

static int vnmr_lm_width(double x1[], double y1[], double w[], double sig[],
                         int ndata, double a[], int ia[], int ma,
                         double **covar, double *chisq)
{ return _run_nr_lm2(x1,y1,w,sig,ndata,a,ia,ma,covar,chisq,widthmodel); }

static int vnmr_lm_midpoint(double x1[], double y1[], double m[], double sig[],
                             int ndata, double a[], int ia[], int ma,
                             double **covar, double *chisq)
{ return _run_nr_lm2(x1,y1,m,sig,ndata,a,ia,ma,covar,chisq,midpointmodel); }

#else  /* VNMR_GPL */

#include <gsl/gsl_multifit_nlinear.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>

extern int abortflag;

typedef struct {
    int n, ma;
    double *x1, *y1, *obs, *sig;
    void (*funcs)(double, double, double[], double*, double[], int);
} _CalibData;

static int _cb_f(const gsl_vector *p, void *vd, gsl_vector *f)
{
    _CalibData *d = (_CalibData *)vd;
    double a[d->ma+1], dyda[d->ma+1], ymod;
    for (int j=1;j<=d->ma;j++) a[j]=gsl_vector_get(p,j-1);
    for (int i=0;i<d->n;i++) {
        d->funcs(d->x1[i+1],d->y1[i+1],a,&ymod,dyda,d->ma);
        gsl_vector_set(f,i,(ymod-d->obs[i+1])/d->sig[i+1]);
    }
    return GSL_SUCCESS;
}
static int _cb_df(const gsl_vector *p, void *vd, gsl_matrix *J)
{
    _CalibData *d = (_CalibData *)vd;
    double a[d->ma+1], dyda[d->ma+1], ymod;
    for (int j=1;j<=d->ma;j++) a[j]=gsl_vector_get(p,j-1);
    for (int i=0;i<d->n;i++) {
        d->funcs(d->x1[i+1],d->y1[i+1],a,&ymod,dyda,d->ma);
        for (int j=1;j<=d->ma;j++)
            gsl_matrix_set(J,i,j-1,dyda[j]/d->sig[i+1]);
    }
    return GSL_SUCCESS;
}

static int _run_gsl_lm2(double x1[], double y1[], double obs[], double sig[],
                        int ndata, double a[], int ma,
                        double **covar_out, double *chisq,
                        void (*funcs)(double, double, double[], double*,
                                      double[], int))
{
    int i, j;
    if (abortflag) { Werrprintf("vnmr_lm (GSL): abortflag set\n"); return 1; }
    _CalibData d = {ndata, ma, x1, y1, obs, sig, funcs};
    gsl_multifit_nlinear_fdf fdf = {_cb_f,_cb_df,NULL,(size_t)ndata,(size_t)ma,&d};
    gsl_multifit_nlinear_parameters fp = gsl_multifit_nlinear_default_parameters();
    fp.trs = gsl_multifit_nlinear_trs_lm;
    gsl_multifit_nlinear_workspace *ws =
        gsl_multifit_nlinear_alloc(gsl_multifit_nlinear_trust,&fp,
                                   (size_t)ndata,(size_t)ma);
    if (!ws) { Werrprintf("vnmr_lm (GSL): alloc failed\n"); abortflag=1; return 1; }
    gsl_vector *p0 = gsl_vector_alloc((size_t)ma);
    if (!p0) { gsl_multifit_nlinear_free(ws); abortflag=1; return 1; }
    for (j=0;j<ma;j++) gsl_vector_set(p0,j,a[j+1]);
    gsl_multifit_nlinear_init(p0,&fdf,ws); gsl_vector_free(p0);
    int info, status = gsl_multifit_nlinear_driver(200,1e-8,1e-8,0.0,
                                                    NULL,NULL,&info,ws);
    if (status!=GSL_SUCCESS && status!=GSL_EMAXITER) {
        Werrprintf("vnmr_lm (GSL): %s\n",gsl_strerror(status));
        gsl_multifit_nlinear_free(ws); return 1;
    }
    gsl_vector *r = gsl_multifit_nlinear_position(ws);
    for (j=0;j<ma;j++) a[j+1]=gsl_vector_get(r,j);
    gsl_vector *res = gsl_multifit_nlinear_residual(ws);
    gsl_blas_ddot(res,res,chisq);
    if (covar_out) {
        gsl_matrix *C = gsl_matrix_alloc((size_t)ma,(size_t)ma);
        if (C) {
            gsl_multifit_nlinear_covar(gsl_multifit_nlinear_jac(ws),0.0,C);
            for (i=0;i<ma;i++) for(j=0;j<ma;j++)
                covar_out[i+1][j+1]=gsl_matrix_get(C,i,j);
            gsl_matrix_free(C);
        }
    }
    gsl_multifit_nlinear_free(ws); return 0;
}

static int vnmr_lm_width(double x1[], double y1[], double w[], double sig[],
                         int ndata, double a[], int ia[], int ma,
                         double **covar, double *chisq)
{ (void)ia; return _run_gsl_lm2(x1,y1,w,sig,ndata,a,ma,covar,chisq,widthmodel); }

static int vnmr_lm_midpoint(double x1[], double y1[], double m[], double sig[],
                             int ndata, double a[], int ia[], int ma,
                             double **covar, double *chisq)
{
    (void)ia; (void)ma;
    a[1]=0.0; a[2]=0.0; a[3]=0.0;
    return _run_gsl_lm2(x1,y1,m,sig,ndata,a,3,covar,chisq,midpointmodel);
}

#endif  /* VNMR_GPL */

#endif
