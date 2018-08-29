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

extern FILE *resfile;


extern void jacobi();

extern float gb1r,ival,pwt,vrotr,mval[],sw,sp;
extern int pft,ntrans,*viso,sites;

/* This subroutine makes an approximate calculation of the effect of a 
 finite pulse. The routine sets up a Hamiltonian representing the pulse
 and either a CSA term (I=1/2 nuclei) or a first order quadrupole
 interaction (I>1/2). The CSA/quadrupole interaction parameters is set such
 that the transition frequency (including the off-resonance term) equals
 the position of the different spinning side bands. The calculation is
 performed for each sideband. 
 From this Hamiltonian the evolution of the density matrix from time 0 to
 pw is calculated. The element m,m-1 in the density matrix is multiplied
 on the ssb intensity (calculated in the ideal-pulse approx).
 The approximations used in the routines in this module correspond to
 the excitation for a static sample */

void shortpulse(shp,shm,mv,sitectr,param)
float *shp,*shm,mv,*param;
int sitectr;
{
int m,i,j,k,ndim,index,index1;
float **r,**pr,**ppi,**h,*l,*lamr,*lami,*iz,*b,*z;
ndim = (int)(2*ival+1.1);
h = matrix(1,ndim,1,ndim);
pr = matrix(1,ndim,1,ndim);
ppi = matrix(1,ndim,1,ndim);
r = matrix(1,ndim,1,ndim);
l = vector(1,ndim);
lamr = vector(1,ndim);
lami = vector(1,ndim);
b = vector(1,ndim);
z = vector(1,ndim);
iz = vector(1,ndim);
for (i=1;i<=ndim; i++) iz[i] = ival+1.0-i;

/* I = 1/2  */

if (ival==0.5) {
  for (m=-pft/2;m<pft/2;m++) { /* do the calculation for each sideband*/
    for (i=1;i<=ndim; i++) 
      h[i][i]=(m*vrotr+2.0*pi*(0.5*sw+sp-param[viso[sitectr]]))*iz[i];
    h[1][2] = h[2][1] = 0.5*gb1r; 
    jacobi(h,l,r,ndim,b,z); /*Diagonalize the Hamiltonian*/
    for (i=1;i<=ndim;i++) 
      cossin(-l[i]*pwt,&lamr[i],&lami[i]);  /* propagator elements in the diagonal basis*/
    for (i=1;i<=ndim;i++) {  /* Transform propagator back to normal basis*/
      for (j=1;j<=ndim;j++) {
        pr[i][j] = 0.0; ppi[i][j] = 0.0;
        for (k=1; k<=ndim;k++) {
          pr[i][j] += r[i][k]*lamr[k]*r[j][k];
          ppi[i][j] += r[i][k]*lami[k]*r[j][k];
          }
        }
      }
    shp[2*(m+pft/2)] = 0.0; shp[2*(m+pft/2)+1] = 0.0;
 /* Now calculate the element (1,2) (real and imaginary parts) in the
   density matrix after the pulse*/
    for (i=1;i<=ndim;i++) { 
      shp[2*(m+pft/2)] += pr[1][i]*iz[i]*pr[2][i]+ppi[1][i]*iz[i]*ppi[2][i];
      shp[2*(m+pft/2)+1] += pr[1][i]*iz[i]*ppi[2][i]-ppi[1][i]*iz[i]*pr[2][i];
      }

    } /* end of m-loop*/
 } else {  /* end of ival==0.5*/
  index = (int)(ival+1.1-mv);
  index1 = (int)(ival+mv+0.1);
  for (m=-pft/2+1;m<pft/2; m++) { /*loop for each side band*/
    for (i=1;i<=ndim;i++)
      for (j=1;j<=ndim;j++) h[i][j] = 0.0;
/* the 'pseudo' quadrupole interaction giving a transition at frequency
   m*vrot is m*vrot/(6m-3), for the m to m-1 transition*/
    for (i=1;i<=ndim;i++) {
      h[i][i] = m*vrotr/(6*mv-3)*(3*(ival+1-i)*(ival+1-i)-ival*(ival+1));
/* add the off-resonance term*/
      h[i][i] += 2.0*pi*(0.5*sw+sp-param[viso[sitectr]])*(ival-i+1);
      if (i<ndim) {
        h[i][i+1] = 0.5*gb1r*sqrt(ival*(ival+1)-(ival+1-i)*(ival-i));
        h[i+1][i] = h[i][i+1];
        }
      }
/* Calculate the propagator*/
    jacobi(h,l,r,ndim,b,z);
    for (i=1;i<=ndim;i++) 
      cossin(-l[i]*pwt,&lamr[i],&lami[i]); 
    for (i=1;i<=ndim;i++) {
      for (j=1;j<=ndim;j++) {
        pr[i][j] = 0.0; ppi[i][j] = 0.0;
        for (k=1; k<=ndim;k++) {
          pr[i][j] += r[i][k]*lamr[k]*r[j][k];
          ppi[i][j] += r[i][k]*lami[k]*r[j][k];
          }
        }
      }
/* and the (m,m-1) element in the density matrix*/
    shp[2*(m+pft/2)] = 0.0; shp[2*(m+pft/2)+1] = 0.0;
    for (i=1;i<=ndim;i++) { /* we need ro*(i,j)*/
      shp[2*(m+pft/2)] += pr[index][i]*iz[i]*pr[index+1][i]+ppi[index][i]*iz[i]*ppi[index+1][i];
      shp[2*(m+pft/2)+1] += pr[index][i]*iz[i]*ppi[index+1][i]-ppi[index][i]*iz[i]*pr[index+1][i];
      }
/* and for the symmetric (-m+1,-m) transition*/
    shm[2*(-m+pft/2)] = 0.0; shm[2*(-m+pft/2)+1] = 0.0;
    for (i=1;i<=ndim;i++) { /* we need ro*(i,j)*/
      shm[2*(-m+pft/2)] += pr[index1][i]*iz[i]*pr[index1+1][i]+ppi[index1][i]*iz[i]*ppi[index1+1][i];
      shm[2*(-m+pft/2)+1] += pr[index1][i]*iz[i]*ppi[index1+1][i]-ppi[index1][i]*iz[i]*pr[index1+1][i];
      }
    } /* end of m-loop */
  } /* end of I>1/2*/
free_matrix(h,1,ndim,1,ndim);
free_matrix(pr,1,ndim,1,ndim);
free_matrix(ppi,1,ndim,1,ndim);
free_matrix(r,1,ndim,1,ndim);
free_vector(l,1,ndim);
free_vector(lamr,1,ndim);
free_vector(lami,1,ndim);
free_vector(iz,1,ndim);
free_vector(b,1,ndim);
free_vector(z,1,ndim);
}



extern int *cq,*etaq,*csa,*etas,nt,npar;
extern float sfrqr;

/* The following routine is used for the central transition for a
half-integer  quadrupolar nucleus. The calculations are similar to 
above, but for this case we need to do an averaging over the different
crystalite orientations. */

void shortpulse_c(shp,sitectr,param)
float *shp,*param;
int sitectr;
{
int m,i,j,k,ndim,jy,jz,index;
float c2a,cb,sb,r,r2,r3,fac,fac1,a20,x,y,z;
float **rvec,**pr,**ppi,**h,*l,*lamr,*lami,*iz,*b,*zz;
float statwidth;
int ns;
ndim = (int)(2*ival+1.1);
rvec = matrix(1,ndim,1,ndim);
h = matrix(1,ndim,1,ndim);
pr = matrix(1,ndim,1,ndim);
ppi = matrix(1,ndim,1,ndim);
l = vector(1,ndim);
lamr = vector(1,ndim);
lami = vector(1,ndim);
b = vector(1,ndim);
zz = vector(1,ndim);
iz = vector(1,ndim);
for (i=1;i<=ndim; i++) iz[i] = ival+1.0-i;
index = (int)(ival+1.1-0.5);
for (i=0;i<2*pft;i++) shp[i] = 0.0;
/* First calculate the approximate width of the central transition to
see how many sidebands we need to do the calculation for */
statwidth = 2.0*9.0/144.0*(25.0+22.0*param[etaq[sitectr]]+param[etaq[sitectr]]*param[etaq[sitectr]])*(ival*(ival+1.0)-0.75)*pow(param[cq[sitectr]]/(2.0*ival*(2.0*ival-1)),2.0)/sfrqr;
if (csa[sitectr]<parmax+1) statwidth +=  0.5*fabs(param[csa[sitectr]]*sfrqr*(3.0+param[etas[sitectr]])) ; 
ns = (int)(2.0*statwidth/vrotr+5);
if (ns>pft) ns=pft; /* The number of sidebands*/
for (jy=0; jy<=nt; jy++) { /* Loops for the crystalite averaging*/
  if ((jy==0) | (jy==nt)) 
    fac1 =  0.5;
    else fac1 = 1.0;
  for (jz=0; jz<=nt; jz++) {
    if (jz==0)
      fac = fac1*0.5;
      else fac = fac1;
    if (jy>(nt-jz)) {
      x = nt-jz-jy;  /* this is alfa<=90 degrees */
      y = nt-jz;
      z = nt-jy; 
      } else { 
      x = nt-jy-jz;   /* and alfa >90 degrees */
      y = jy;
      z = jz;
      }
    r2 = (x*x+y*y+z*z);
    r = sqrt(r2);
    r3 = r2*r;
    cb = z/r;
    sb = sqrt(1.0-cb*cb);
    if (sb>1.0e-5) {
      c2a = (x*x-y*y)/(r2*sb*sb);
      } else {
      c2a =  1.0;
      }
    a20=param[cq[sitectr]]/(4.0*ival*(2.0*ival-1));
    a20 *= (1.5*cb*cb-0.5+0.5*sb*sb*param[etaq[sitectr]]*c2a);
    for (m=-ns/2;m<ns/2; m++) {
      for (i=1;i<=ndim;i++)
        for (j=1;j<=ndim;j++) h[i][j] = 0.0;
      for (i=1;i<=ndim;i++) {
        h[i][i] = ((float)(m)*vrotr+2.0*pi*(0.5*sw+sp-param[viso[sitectr]]))*iz[i];
        h[i][i] += a20*(3*(ival+1-i)*(ival+1-i)-ival*(ival+1));
        if (i<ndim) {
          h[i][i+1] = 0.5*gb1r*sqrt(ival*(ival+1)-(ival+1-i)*(ival-i));
          h[i+1][i] = h[i][i+1];
          }
        }
     jacobi(h,l,rvec,ndim,b,zz);
      for (i=1;i<=ndim;i++) 
        cossin(-l[i]*pwt,&lamr[i],&lami[i]); 
      for (i=1;i<=ndim;i++) {
        for (j=1;j<=ndim;j++) {
          pr[i][j] = 0.0; ppi[i][j] = 0.0;
          for (k=1; k<=ndim;k++) {
            pr[i][j] += rvec[i][k]*lamr[k]*rvec[j][k];
            ppi[i][j] += rvec[i][k]*lami[k]*rvec[j][k];
            }
          }
        }
  for (i=1;i<=ndim;i++) { /* we need ro*(i,j)*/
        shp[2*(m+pft/2)] += (pr[index][i]*iz[i]*pr[index+1][i]+ppi[index][i]*iz[i]*ppi[index+1][i])*fac/r3;
        shp[2*(m+pft/2)+1] += (pr[index][i]*iz[i]*ppi[index+1][i]-ppi[index][i]*iz[i]*pr[index+1][i])*fac/r3;
        }
      } /* end of m-loop */
    } /* end of jz */
  } /* end of jy*/
for (i=0;i<2*pft;i++) shp[i] *= (float)(nt)/(pi);
free_matrix(h,1,ndim,1,ndim);
free_matrix(pr,1,ndim,1,ndim);
free_matrix(ppi,1,ndim,1,ndim);
free_matrix(rvec,1,ndim,1,ndim);
free_vector(l,1,ndim);
free_vector(lamr,1,ndim);
free_vector(lami,1,ndim);
free_vector(iz,1,ndim);
free_vector(b,1,ndim);
free_vector(zz,1,ndim);
}


