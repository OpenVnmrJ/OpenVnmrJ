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

#include "starsprog.h"
 
extern void reduce();
extern float brent();
extern void mnbrak();
extern void printresults();

extern FILE *resfile;
extern float qcpar[],qcinc[],digres;
extern int sites,fn,np,pft,npar,totalpar,progorder,maxiter;
extern int *cq,*etaq,*csa,*etas,lb,gf,*viso,rfw,*amp,theta;
extern float **expint,*expspec;

float f1dim(x,param,func,ncalls)
float x,param[],(*func)();
int *ncalls;
{
float rr;
param[0] = x;
++(*ncalls);
rr = (*func)(param);
return rr;
}

void iterate_1(progtype)
float (*progtype)();

/* iteration on one parameter*/
{
float param[parmax],ax,bx,cx,fa,fb,fc,xmin,tol,rms;
int i,j,ncalls;
for (i=0; i<totalpar; i++) param[i]=qcpar[i];
ax = param[0];
bx = param[0]+qcinc[0];
ncalls = 0;
mnbrak(&ax,&bx,&cx,&fa,&fb,&fc,progtype,param,&ncalls);
tol = 1.0e-2;
if (lb == 0) tol = param[0]*1.0e-2;
  else if (gf == 0) tol = param[0]*1.0e-2;
  else if (rfw == 0) tol = param[0]*1.0e-2;
  else if (theta == 0) tol = 0.003;
  else {
  for (j=1;j<=sites;j++) {
    if ((etas[j]==0) | (etaq[j]==0)) tol = 1.0e-4;
    else if ((cq[j]==0) | (csa[j]==0)) tol = param[0]*1.0e-4;
    else if (amp[j] == 0) tol = param[0]*1.0e-2;
    else if (viso[j] == 0) tol = 0.2*digres;}
  }

rms = brent(ax,bx,cx,progtype,tol,&xmin,param,&ncalls,maxiter);
qcpar[0]=param[0];
reduce();
printresults(qcpar,rms,ncalls);
if (progorder == 1) free_matrix(expint,1,sites,0,pft);
  else free_vector(expspec,0,fn);
}
 

extern void amoeba();


void iterate_n(progtype)
float (*progtype)();

/* iteration on more than one parameter*/
{
float p[parmax+1][parmax],y[parmax+1];
float ftol;
int i,j,ncalls;

/* set up matrix p with initial corners for a simplex minimization*/
for (i=0;i<totalpar;i++)
  for (j=0; j<=npar; j++) p[j][i] = qcpar[i];
y[0] = (*progtype)(p[0]);   /* rms for initial parameter estimates */
for (i=1; i<=npar; i++) {
  p[i][i-1] += qcinc[i-1];
  y[i] = (*progtype)(p[i]);  /* rms for the remaining corners*/
  }
ftol=0.001;
ncalls = 0;
amoeba(p,y,npar,totalpar,ftol,progtype,&ncalls,maxiter);
for (i=0; i<totalpar; i++) qcpar[i]=p[0][i];
reduce();
printresults(qcpar,y[0],ncalls);
if (progorder == 1) free_matrix(expint,1,sites,0,pft);
  else free_vector(expspec,0,fn);
}

 

