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

extern FILE* macfile;
extern FILE* resfile;
extern float **expint,**ssbint;
extern int rmsflag,progorder,sites,npar,lb,gf,rfw,theta,pft,ndata;
extern int *cq,*etaq,*csa,*etas,*psi,*chi,*xi,*viso;
extern int quadflag,qcsaflag,csaflag;

void printresults(qcpar,rms,ncalls)
float qcpar[],rms;
int ncalls;
{
int i,sitectr;
char *ip1,*ip2,*ip3;
/* Output parameter values in the form of a macro.*/
if (!rmsflag) {
  if (quadflag || qcsaflag) { 
    for (i=1;i<=sites;i++) {
      if (i==1) fprintf(macfile,"cq=%f etaq=%f\n",qcpar[cq[1]]/(2*pi*1.0e6),qcpar[etaq[1]]);
        else fprintf(macfile,"cq%1d=%f etaq%1d=%f\n",i,qcpar[cq[i]]/(2*pi*1.0e6),i,qcpar[etaq[i]]);
      }
    }
  if (csaflag || qcsaflag) { 
    for (i=1;i<=sites;i++) {
      if (i==1) {
        fprintf(macfile,"csa=%f etas=%f\n",qcpar[csa[1]]*1.0e6,qcpar[etas[1]]);
        fprintf(macfile,"psi=%f chi=%f xi=%f\n",qcpar[psi[1]],qcpar[chi[1]],qcpar[xi[1]]);
        } else {
        fprintf(macfile,"csa%1d=%f etas%1d=%f\n",i,qcpar[csa[i]]*1.0e6,i,qcpar[etas[i]]);
        fprintf(macfile,"psi%1d=%f chi%1d=%f xi%1d=%f\n",i,qcpar[psi[i]],i,qcpar[chi[i]],i,qcpar[xi[i]]);
        }
      }
    }
  if (progorder == 2) {
    if (gf<npar) fprintf(macfile,"gf=%f\n",qcpar[gf]);
    if (lb<npar) fprintf(macfile,"lb=%f\n",qcpar[lb]);
    for (i=1;i<=sites;i++) {
      if (i==1) fprintf(macfile,"viso=%f\n",qcpar[viso[i]]);
        else fprintf(macfile,"viso%1d=%f\n",i,qcpar[viso[i]]);
      }
    }
  fprintf(macfile,"theta=%f rfw=%f\n",qcpar[theta],qcpar[rfw]);
  fflush(macfile);
    

  fprintf(resfile,"Result of the iterative fit on %d experimental points\n",ndata);
  fprintf(resfile,"Final rms= %f after %d spectrum calculations\n",rms,ncalls);
  } else fprintf(resfile,"Rms error = %f\n for %d experimental data points",rms,ndata);

fprintf(resfile,"obtained with the following parameters:\n");
   
for (i=1;i<=sites;i++) {

if (quadflag || qcsaflag) { 
  if (cq[i]<npar) ip1 = "*";
    else ip1 = " ";
  if (etaq[i]<npar) ip2 = "*";
    else ip2 = " ";
  if (i==1) fprintf(resfile,"%1scq= %f %1setaq= %f\n",ip1,qcpar[cq[i]]/(2.0*pi*1.0e6),ip2,qcpar[etaq[i]]);
    else fprintf(resfile,"%1scq%1d= %f %1setaq%1d= %f\n",ip1,i,qcpar[cq[i]]/(2.0*pi*1.0e6),ip2,i,qcpar[etaq[i]]);
  }

if (csaflag || qcsaflag) {
  if (csa[i]<npar) ip1 = "*";
    else ip1 = " ";
  if (etas[i]<npar) ip2 = "*";
    else ip2 = " ";
  if (i==1) fprintf(resfile,"%1scsa= %f %1setas= %f\n",ip1,qcpar[csa[i]]*1.0e6,ip2,qcpar[etas[i]]);
    else fprintf(resfile,"%1scsa%1d= %f %1setas%1d= %f\n",ip1,i,qcpar[csa[i]]*1.0e6,ip2,i,qcpar[etas[i]]);
  }
  

if (qcsaflag) {
  if (psi[i]<npar) ip1 = "*";
    else ip1 = " ";
  if (chi[i]<npar) ip2 = "*";
    else ip2 = " ";
  if (xi[i]<npar) ip3 = "*";
    else ip3 = " ";
  if (i==1) fprintf(resfile,"%1spsi= %f %1schi= %f %1sxi= %f\n",ip1,qcpar[psi[i]],ip2,qcpar[chi[i]],ip3,qcpar[xi[i]]);
    else fprintf(resfile,"%1spsi%1d= %f %1schi%1d= %f %1sxi%1d= %f\n",ip1,i,qcpar[psi[i]],ip2,i,qcpar[chi[i]],ip3,i,qcpar[xi[i]]);
  }
if (progorder == 2) { 
  if (viso[i]<npar) ip1 = "*";
    else ip1 = " ";
  if (i==1) fprintf(resfile,"%1sviso= %f",ip1,qcpar[viso[i]]);
    else fprintf(resfile,"%1sviso%1d= %f",ip1,i,qcpar[viso[i]]);
  fprintf(resfile,"\n");
  }
  }
if (theta<npar) ip1 = "*";
  else ip1 = " ";
fprintf(resfile,"%1stheta= %f",ip1,qcpar[theta]);
if (rfw<npar) ip1 = "*";
  else ip1 = " ";
fprintf(resfile,"%1srfw= %f\n",ip1,qcpar[rfw]);
if (progorder == 2) { 
  if (lb<npar) ip1 = "*";
    else ip1 = " ";
  if (gf<npar) ip2 = "*";
    else ip2 = " ";
  fprintf(resfile,"%1slb= %f %1sgf= %f\n",ip1,qcpar[lb],ip2,qcpar[gf]);
  fflush(resfile);
  } else {
  for (sitectr=1;sitectr<=sites;sitectr++) {
    if (sites>1) fprintf(resfile,"\nResult for site %1d\n",sitectr);
    fprintf(resfile,"\n  i        exp              calc       exp-calc\n");
    for (i=0; i<pft; i++) 
        if (expint[sitectr][i] > 1.0e-6) 
        fprintf(resfile,"%3d      %7.4f         %7.4f      %7.4f\n",pft/2-i,expint[sitectr][i],ssbint[sitectr][2*i],expint[sitectr][i]-ssbint[sitectr][2*i]);
      else
        fprintf(resfile,"%3d                      %7.4f             \n",pft/2-i,ssbint[sitectr][2*i]);
    }
  fflush(resfile);
  }
}
