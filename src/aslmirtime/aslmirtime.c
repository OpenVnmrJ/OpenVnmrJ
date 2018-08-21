/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* aslmirtime.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* aslmirtime.c: Calculate ASL multiple IR (MIR) times                       */
/*                                                                           */
/* Copyright (C) 2011 Paul Kinchesh                                          */
/*                                                                           */
/* aslmirtime is free software: you can redistribute it and/or modify        */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* aslmirtime is distributed in the hope that it will be useful,             */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with aslmirtime. If not, see <http://www.gnu.org/licenses/>.        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/**/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Usage:   aslmirtime -n nmir -d ti -t nT1 T1(1) T1(2) .. T1(nT1)           */
/*            -n nmir  nmir is the number of MIR pulses to apply             */
/*            -d ti    ti is blood inflow delay in seconds                   */
/*            -t nT1   nT1 is the number of T1s                              */
/*             T1(n)   is the nth T1 of static background signals in seconds */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/**/

#define LOCAL /* EXTERN globals will not be 'extern' */
#include "aslmirtime.h"

int main(int argc,char *argv[])
{
  double *pars; /* input parameters */
  int nmir;     /* number of MIR pulses */
  double ti;    /* blood inflow delay */
  int i;
  char timefile[MAXPATHLEN];
  FILE *f_out;

  size_t iter = 0;
  int status;     
  const gsl_multimin_fdfminimizer_type *T;
  gsl_multimin_fdfminimizer *s;
  gsl_vector *x;
  gsl_multimin_function_fdf my_func;
  double fvalue=100.0;

  /* Get input: number of MIR pulses (pars[0])
                blood inflow delay (pars[1])
                number of static background T1s (pars[2])
                the T1s pars[3], pars[4], ... 
  */
  getinput(&pars,argc,argv);

  nmir = (int)pars[0]; /* number of MIR pulses */
  ti   = pars[1];      /* blood inflow delay */

  my_func.n      = nmir;
  my_func.f      = my_f;
  my_func.df     = my_df;
  my_func.fdf    = my_fdf;
  my_func.params = pars;
     
  /* Starting point, x = (0.5*ti/nmir,1.5*ti/nmir,...,(nmir-1+0.5)*ti/nmir) */
  x = gsl_vector_alloc(nmir);
  for (i=0;i<nmir;i++) gsl_vector_set(x,i,(i+0.5)*ti/nmir);

  /* Can't figure why vector_bfgs2 seems not to work */
  T = gsl_multimin_fdfminimizer_vector_bfgs;
  s = gsl_multimin_fdfminimizer_alloc(T,nmir);

  /* Use 4 us step size */
  gsl_multimin_fdfminimizer_set(s,&my_func,x,4e-6,1e-4);

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Using %s minimizer\n",gsl_multimin_fdfminimizer_name(s));
#endif

  do
    {
      if (iter>0) fvalue=s->f;
      iter++;
      status = gsl_multimin_fdfminimizer_iterate(s);
      if (status) break;
      status = gsl_multimin_test_gradient(s->gradient,1e-3);

#ifdef DEBUG
  if (status == GSL_SUCCESS) fprintf(stdout,"Minimum found at:\n");
  fprintf(stdout,"%5d ", (int)iter);
  for (i=0;i<nmir;i++) fprintf(stdout,"%.5f ",gsl_vector_get(s->x,i));
  fprintf(stdout,"%15.10f\n", s->f);
#endif

    }
  while (status == GSL_CONTINUE && iter < 100 && FP_NEQ(s->f,fvalue));

#ifdef DEBUG
  fprintf(stdout,"  irtime:");
  for (i=0;i<nmir;i++) fprintf(stdout," %f",ti-gsl_vector_get(s->x,nmir-i-1));
  fprintf(stdout,"\n\n");
#endif

  sprintf(timefile,"/tmp/aslmirtime_%s.txt",getenv("USER"));
  if ((f_out = fopen(timefile, "w")) == NULL) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Unable to write to %s\n",timefile);
    fflush(stderr);
    return(1);
  }
  for (i=0;i<nmir;i++) fprintf(f_out,"%f\n",ti-gsl_vector_get(s->x,nmir-i-1));
  fclose(f_out);

  gsl_multimin_fdfminimizer_free(s);
  gsl_vector_free(x);
  free(pars);

  exit(0);

}

double my_f(const gsl_vector *v,void *params)
{
  double *p = (double *)params;
  int nmir,nt1;
  double ti,*T1;
  double t;
  double M,m;
  int i,j;

  /* Set parameters */
  nmir = (int)p[0];
  ti = p[1];
  nt1 = (int)p[2];
  if ((T1=(double *)malloc((nt1)*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);  
  for (i=0;i<nt1;i++) T1[i]=p[i+3];

  /* Calculate magnetization */
  M=0.0;
  for (i=0;i<nt1;i++) {
    m = 1 + pow(-1,nmir+1)*exp(-ti/T1[i]);
    for (j=0;j<nmir;j++) {
      t = gsl_vector_get(v,j);
      m += 2*pow(-1,j+1)*exp(-t/T1[i]);
    }
    M += fabs(m);
  }

  /* Free T1 */
  free(T1);

  /* Return magnetization */
  return(M);
}
     
/* The gradient of f, df = (df/dx, df/dy). */
void my_df(const gsl_vector *v,void *params,gsl_vector *df)
{
  double *p = (double *)params;
  int nmir,nt1;
  double ti,*T1;
  double t;
  double G;
  int i,j;

  /* Set parameters */
  nmir = (int)p[0];
  ti = p[1];
  nt1 = (int)p[2];
  if ((T1=(double *)malloc((nt1)*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);  
  for (i=0;i<nt1;i++) T1[i]=p[i+3];

  /* Calculate dM/dt for each t */
  for (i=0;i<nmir;i++) {
    t = gsl_vector_get(v,i);
    G = 0.0;
    for (j=0;j<nt1;j++) {
      G += 2*pow(-1,i)*exp(-t/T1[j])/T1[j];
    }
    gsl_vector_set(df,i,G);
  }
  free(T1);
}
    
/* Compute both f and df together. */
void my_fdf(const gsl_vector *x,void *params,double *f,gsl_vector *df) 
{
  *f = my_f(x,params); 
  my_df(x,params,df);
}

void getinput(double **pars,int argc,char *argv[])
{
  int nmir,nt1;
  double ti,*T1;
  int i;

  /* Initialize input */
  nmir=0; ti=0.0; nt1=0; T1=NULL;

  /* Read arguments of form -x aaa -y bbb and of form -xyz */
  while ((--argc > 0) && ((*++argv)[0] == '-')) {
    char *s;

    /* Interpret each non-null character after the '-' (ie argv[0]) as an option */
    for (s=argv[0]+1;*s;s++) {
      switch (*s) {
        case 'n': /* Number of MIR pulses */
          sscanf(*++argv,"%d",&nmir);
          argc--;
          break;
        case 'd': /* blood inflow delay in seconds */
          sscanf(*++argv,"%lf",&ti);
          argc--;
          break;
        case 't': /* T1 in seconds */
          sscanf(*++argv,"%d",&nt1);
          argc--;
          if ((T1=(double *)malloc(nt1*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
          for (i=0;i<nt1;i++) {
            if (argc>1) {
              if (sscanf(*++argv,"%lf",&T1[i])<1) {
                fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
                fprintf(stderr,"  error reading T1s\n");
                usage_terminate();
              }
              argc--;
            } else {
              fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
              fprintf(stderr,"  error reading T1s\n");
              usage_terminate();
            }
          }
          break;
        default:
          fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
          fprintf(stderr,"  Illegal switch option\n");
          fflush(stderr);
          break;
      }
    }
  }

  if ((nmir==0) || FP_EQ(ti,0.0) || (nt1<1)) usage_terminate();

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  number of MIR pulses = %d\n",nmir);
  fprintf(stdout,"  blood inflow delay = %f seconds\n",ti);
  fprintf(stdout,"  number of T1s = %d\n",nt1);
  for (i=0;i<nt1;i++) fprintf(stdout,"  T1[%d] = %f seconds\n",i+1,T1[i]);
#endif

  /* Fill pars with the input */
  if ((*pars=(double *)malloc((nt1+3)*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  (*pars)[0]=(double)nmir;
  (*pars)[1]=ti;
  (*pars)[2]=(double)nt1;
  for (i=0;i<nt1;i++) (*pars)[i+3]=T1[i];

}

int nomem(char *file,const char *function,int line)
{
  fprintf(stderr,"\n%s: %s() line %d\n",file,function,line);
  fprintf(stderr,"  Insufficient memory for program!\n\n");
  fflush(stderr);
  exit(1);
}

int usage_terminate()
{
  fprintf(stderr,"\n  Usage: aslmirtime -n nmir -d ti -t nT1 T1(1) T1(2) .. T1(nT1) \n");
  fprintf(stderr,"    -n nmir  nmir is the number of MIR pulses to apply\n");
  fprintf(stderr,"    -d ti    ti is blood inflow delay in seconds\n");
  fprintf(stderr,"    -t nT1   nT1 is the number of T1s\n");
  fprintf(stderr,"     T1(n)   is the nth T1 of static background signals in seconds\n");
  fprintf(stderr,"\n");
  exit(1);
}
