/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef VNMRLM_H
#define VNMRLM_H

typedef struct
{
  double tau, grad;
} t1_x;

#ifdef VNMR_GPL
#include <stdlib.h>
#include <math.h>
#include <gsl/gsl_multifit_nlinear.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_errno.h>

static double **
vnmr_gsl_matrix(long nrl, long nrh, long ncl, long nch)
{
    long nrows = nrh - nrl + 1;
    long ncols = nch - ncl + 1;
    long i;
    double **m;

    m = (double **)malloc((size_t)(nrows + 1) * sizeof(double *));
    if (!m) return NULL;
    m -= nrl;

    m[nrl] = (double *)calloc((size_t)(nrows * ncols + 1), sizeof(double));
    if (!m[nrl]) { free(m + nrl); return NULL; }
    m[nrl] -= ncl;

    for (i = nrl + 1; i <= nrh; i++)
        m[i] = m[i-1] + ncols;

    return m;
}

static void
vnmr_gsl_free_matrix(double **m, long nrl, long nrh, long ncl, long nch)
{
    (void)nrh; (void)nch;
    if (!m) return;
    free(m[nrl] + ncl);
    free(m + nrl);
}

#define matrix(nrl,nrh,ncl,nch)         vnmr_gsl_matrix((nrl),(nrh),(ncl),(nch))
#define free_matrix(m,nrl,nrh,ncl,nch)  vnmr_gsl_free_matrix((m),(nrl),(nrh),(ncl),(nch))

typedef struct {
    double  *x1;
    t1_x    *xt;
    double  *y;
    double  *sig;
    int      npts;
    int     *lista;
    int      ma;
    double   a_fixed[NPARAMS + 1];
    int      is2d;
    void   (*func1d)(double, double[], double *, double[], int);
    void   (*func2d)(t1_x,  double[], double *, double[], int);
} vnmr_gsl_data;

void dosyfunc  (double x, double a[], double *y, double dyda[], int na);
void t1func    (double x, double a[], double *y, double dyda[], int na);
void t1dosyfunc(t1_x   x, double a[], double *y, double dyda[], int na);

extern int abortflag;

/* -----------------------------------------------------------------------
 * GSL residual callback: f_i = (y_data - y_model) / sigma
 * --------------------------------------------------------------------- */
static int
vnmr_gsl_f(const gsl_vector *p, void *params, gsl_vector *fvec)
{
    vnmr_gsl_data *d = (vnmr_gsl_data *)params;
    double a[NPARAMS + 1], yval, dyda[NPARAMS + 1];
    int i, k, pfree = 0;

    for (k = 1; k <= d->ma; k++)
        a[k] = d->lista[k] ? gsl_vector_get(p, pfree++) : d->a_fixed[k];

    for (i = 0; i < d->npts; i++) {
        if (d->is2d)
            d->func2d(d->xt[i+1], a, &yval, dyda, d->ma);
        else
            d->func1d(d->x1[i+1], a, &yval, dyda, d->ma);
        gsl_vector_set(fvec, i, (d->y[i+1] - yval) / d->sig[i+1]);
    }
    return abortflag ? GSL_EBADFUNC : GSL_SUCCESS;
}

/* -----------------------------------------------------------------------
 * GSL Jacobian callback: J_ij = (dy_model/da_j) / sigma_i
 * --------------------------------------------------------------------- */
static int
vnmr_gsl_df(const gsl_vector *p, void *params, gsl_matrix *J)
{
    vnmr_gsl_data *d = (vnmr_gsl_data *)params;
    double a[NPARAMS + 1], yval, dyda[NPARAMS + 1];
    int i, k, pfree = 0, jcol;

    for (k = 1; k <= d->ma; k++)
        a[k] = d->lista[k] ? gsl_vector_get(p, pfree++) : d->a_fixed[k];

    for (i = 0; i < d->npts; i++) {
        if (d->is2d)
            d->func2d(d->xt[i+1], a, &yval, dyda, d->ma);
        else
            d->func1d(d->x1[i+1], a, &yval, dyda, d->ma);
        jcol = 0;
        for (k = 1; k <= d->ma; k++)
            if (d->lista[k])
                gsl_matrix_set(J, i, jcol++, dyda[k] / d->sig[i+1]);
    }
    return abortflag ? GSL_EBADFUNC : GSL_SUCCESS;
}

static int
vnmr_gsl_run(vnmr_gsl_data *d, double anr[], double **covar,
             double *chisq, double sumsq)
{
    gsl_multifit_nlinear_workspace *w   = NULL;
    gsl_vector                     *p0  = NULL;
    gsl_matrix                     *cov = NULL;
    int ret = 0, mfree = 0, k, pfree, itst;
    double ochisq_loc, chi2;

    for (k = 1; k <= d->ma; k++)
        if (d->lista[k]) mfree++;

    if (mfree == 0 || d->npts < mfree) { *chisq = 0.0; return 0; }

    for (k = 1; k <= d->ma; k++)
        d->a_fixed[k] = anr[k];

    p0 = gsl_vector_alloc(mfree);
    if (!p0) { ret = 1; goto cleanup; }
    pfree = 0;
    for (k = 1; k <= d->ma; k++)
        if (d->lista[k]) gsl_vector_set(p0, pfree++, anr[k]);

    {
        gsl_multifit_nlinear_parameters fp =
            gsl_multifit_nlinear_default_parameters();
        fp.trs = gsl_multifit_nlinear_trs_lm;
        w = gsl_multifit_nlinear_alloc(gsl_multifit_nlinear_trust,
                                       &fp, (size_t)d->npts, (size_t)mfree);
        if (!w) { ret = 1; goto cleanup; }
    }

    {
        gsl_multifit_nlinear_fdf fdf = { vnmr_gsl_f, vnmr_gsl_df, NULL,
                                         (size_t)d->npts, (size_t)mfree, d };
        if (gsl_multifit_nlinear_init(p0, &fdf, w)) { ret = 1; goto cleanup; }
    }

    {
        const gsl_vector *res = gsl_multifit_nlinear_residual(w);
        gsl_blas_ddot(res, res, &chi2);
        ochisq_loc = chi2;
        itst = 0;

        /* step 1: alamda<0 branch */
        {
            int r = gsl_multifit_nlinear_iterate(w);
            if (r == GSL_SUCCESS || r == GSL_ENOPROG) {
                gsl_blas_ddot(res, res, &chi2);
                if (abortflag) { ret = 1; goto cleanup; }
                if (chi2 < ochisq_loc)
                    itst = (fabs(ochisq_loc - chi2) < 0.01 * sumsq) ? 1 : 0;
                ochisq_loc = chi2;
            }
        }

        /* steps 2..999 */
        for (k = 2; k <= 999 && itst < MAXITERS; k++) {
            int r = gsl_multifit_nlinear_iterate(w);
            if (r != GSL_SUCCESS && r != GSL_ENOPROG) break;
            gsl_blas_ddot(res, res, &chi2);
            if (abortflag) { ret = 1; goto cleanup; }
            if (chi2 >= ochisq_loc)
                itst = 0;
            else
                itst = (fabs(ochisq_loc - chi2) < 0.01 * sumsq) ? itst + 1 : 0;
            ochisq_loc = chi2;
        }
        *chisq = chi2;
    }

    {
        const gsl_vector *result = gsl_multifit_nlinear_position(w);
        pfree = 0;
        for (k = 1; k <= d->ma; k++)
            if (d->lista[k]) anr[k] = gsl_vector_get(result, pfree++);
    }

    cov = gsl_matrix_alloc(mfree, mfree);
    if (!cov) { ret = 1; goto cleanup; }
    gsl_multifit_nlinear_covar(gsl_multifit_nlinear_jac(w), 0.0, cov);
    {
        int r, c, pr = 0, pc;
        for (r = 1; r <= d->ma; r++)
            for (c = 1; c <= d->ma; c++)
                covar[r][c] = 0.0;
        for (r = 1; r <= d->ma; r++) {
            if (!d->lista[r]) continue;
            pc = 0;
            for (c = 1; c <= d->ma; c++)
                if (d->lista[c])
                    covar[r][c] = gsl_matrix_get(cov, pr, pc++);
            pr++;
        }
    }

cleanup:
    if (cov) gsl_matrix_free(cov);
    if (w)   gsl_multifit_nlinear_free(w);
    if (p0)  gsl_vector_free(p0);
    return ret;
}

#define _VNMR_GSL_RUN1D(x_,y_,s_,n_,a_,l_,ma_,cv_,ch_,fn_)             \
    do {                                                                  \
        vnmr_gsl_data _gd;                                               \
        _gd.x1=x_; _gd.xt=NULL; _gd.y=y_; _gd.sig=s_;                  \
        _gd.npts=n_; _gd.lista=l_; _gd.ma=ma_;                          \
        _gd.is2d=0; _gd.func1d=fn_; _gd.func2d=NULL;                    \
        if (vnmr_gsl_run(&_gd,(a_),(cv_),(ch_),sumsq)) abortflag=1;     \
    } while(0)

#define _VNMR_GSL_RUN2D(xt_,y_,s_,n_,a_,l_,ma_,cv_,ch_,fn_)            \
    do {                                                                  \
        vnmr_gsl_data _gd;                                               \
        _gd.x1=NULL; _gd.xt=xt_; _gd.y=y_; _gd.sig=s_;                 \
        _gd.npts=n_; _gd.lista=l_; _gd.ma=ma_;                          \
        _gd.is2d=1; _gd.func1d=NULL; _gd.func2d=fn_;                    \
        if (vnmr_gsl_run(&_gd,(a_),(cv_),(ch_),sumsq)) abortflag=1;     \
    } while(0)

#define lm_init(x,y,s,n,a,l,ma,cv,al,ch,fn)       _VNMR_GSL_RUN1D(x,y,s,n,a,l,ma,cv,ch,fn)
#define lm_iterate(x,y,s,n,a,l,ma,cv,al,ch,fn)    /* no-op */
#define lm_covar(x,y,s,n,a,l,ma,cv,al,ch,fn)      /* no-op */

#define lm_init2d(xt,y,s,n,a,l,ma,cv,al,ch,fn)    _VNMR_GSL_RUN2D(xt,y,s,n,a,l,ma,cv,ch,fn)
#define lm_iterate2d(xt,y,s,n,a,l,ma,cv,al,ch,fn) /* no-op */
#define lm_covar2d(xt,y,s,n,a,l,ma,cv,al,ch,fn)   /* no-op */

#else
extern void lm_init2d (t1_x x[], double y[], double sig[], int ndata, double a[],
                 int ia[], int ma, double **covar, double **alpha, double *chisq,
                 void (*funcs) (t1_x, double[], double *, double[], int));
extern void lm_iterate2d (t1_x x[], double y[], double sig[], int ndata, double a[],
                 int ia[], int ma, double **covar, double **alpha, double *chisq,
                 void (*funcs) (t1_x, double[], double *, double[], int));
extern void lm_covar2d (t1_x x[], double y[], double sig[], int ndata, double a[],
                 int ia[], int ma, double **covar, double **alpha, double *chisq,
                 void (*funcs) (t1_x, double[], double *, double[], int));

extern void lm_init(double x[], double y[], double sig[], int ndata, double a[],
	int ia[], int ma, double **covar, double **alpha, double *chisq,
	void (*funcs)(double, double [], double *, double [], int));
extern void lm_iterate(double x[], double y[], double sig[], int ndata, double a[],
	int ia[], int ma, double **covar, double **alpha, double *chisq,
	void (*funcs)(double, double [], double *, double [], int));
extern void lm_covar(double x[], double y[], double sig[], int ndata, double a[],
	int ia[], int ma, double **covar, double **alpha, double *chisq,
	void (*funcs)(double, double [], double *, double [], int));

extern void nrerror(char error_text[]);
#endif 

#endif 
