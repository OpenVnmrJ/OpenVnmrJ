/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "wjunk.h"
#include "vnmr_lm.h"

extern int abortflag;

void
nrerror (char error_text[])
{				/*{{{ */
  Werrprintf("DOSY fit function not available: %s",error_text);
  abortflag = 1;
}				/*}}} */

void lm_init(double x[], double y[], double sig[], int ndata,
		      double a[], int ia[], int ma, double **covar,
		      double **alpha, double *chisq,
                      void (*funcs) (double, double[], double *, double[], int))
{
   Werrprintf("DOSY fit function not available");
   abortflag = 1;

}

void lm_iterate(double x[], double y[], double sig[], int ndata,
		      double a[], int ia[], int ma, double **covar,
		      double **alpha, double *chisq,
                      void (*funcs) (double, double[], double *, double[], int))
{
   Werrprintf("DOSY fit function not available");
   abortflag = 1;
}

void lm_covar(double x[], double y[], double sig[], int ndata,
		      double a[], int ia[], int ma, double **covar,
		      double **alpha, double *chisq,
                      void (*funcs) (double, double[], double *, double[], int))
{
   Werrprintf("DOSY fit function not available");
   abortflag = 1;
}

void
lm_init2d (t1_x x[], double y[], double sig[], int ndata, double a[], int ia[],
	  int ma, double **covar, double **alpha, double *chisq,
	  void (*funcs) (t1_x, double[], double *, double[], int))
{
   Werrprintf("DOSY fit function not available");
   abortflag = 1;
}

void
lm_iterate2d (t1_x x[], double y[], double sig[], int ndata, double a[], int ia[],
	  int ma, double **covar, double **alpha, double *chisq,
	  void (*funcs) (t1_x, double[], double *, double[], int))
{
   Werrprintf("DOSY fit function not available");
   abortflag = 1;
}

void
lm_covar2d (t1_x x[], double y[], double sig[], int ndata, double a[], int ia[],
	  int ma, double **covar, double **alpha, double *chisq,
	  void (*funcs) (t1_x, double[], double *, double[], int))
{
   Werrprintf("DOSY fit function not available");
   abortflag = 1;
}
