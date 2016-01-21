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

extern void four1();

extern int sites,pft,pft2,fn,fndim,np,npdim,ndata,npar,nt,rmsflag;
extern int *amp,*cq,*etaq,*csa,*etas,*psi,*chi,*xi,*viso,rfw,theta;
extern int lb,gf;
extern int quadflag,qcsaflag,csaflag;
extern float vrot,sw,sp,digres;
extern float **ssbint,**expint,*spec,*expspec;


void printrms(p,r)
float *p,r;
{
int j,k;
fprintf(resfile,"rms = %f\n",r);
for (j=1;j<=sites;j++) {
  k = 0;
  if (amp[j]<npar) {
    if (j==1) fprintf(resfile,"amp=%f ",p[amp[j]]);
      else fprintf(resfile,"amp%1d=%f ",j,p[amp[j]]);
    k++;}
  if (viso[j]<npar) {
    if (j==1) fprintf(resfile,"viso=%f ",p[viso[j]]);
      else fprintf(resfile,"viso%1d=%f ",j,p[viso[j]]);
    k++;}
  if (csaflag || qcsaflag) {
    if (csa[j]<npar) {
      if (j==1) fprintf(resfile,"csa= %f ",p[csa[j]]*1.0e6);
        else fprintf(resfile,"csa%1d= %f ",j,p[csa[j]]*1.0e6);
      k++;
      }
    if (etas[j]<npar) {
      if (j==1) fprintf(resfile,"etas= %f ",p[etas[j]]);
        else fprintf(resfile,"etas%1d= %f ",j,p[etas[j]]);
      k++;
      }
    if (k!=0) {
      fprintf(resfile,"\n");
      k = 0;
      }
    }
  if (quadflag || qcsaflag) {
    if (cq[j]<npar) {
      if (j==1) fprintf(resfile,"cq= %f ",p[cq[j]]/(2*pi*1.0e6));
        else fprintf(resfile,"cq%1d= %f ",j,p[cq[j]]/(2*pi*1.0e6));
      k++;}
    if (etaq[j]<npar) {
      if (j==1) fprintf(resfile,"etaq= %f ",p[etaq[j]]);
        else fprintf(resfile,"etaq%1d= %f ",j,p[etaq[j]]);
      k++;}
    if (psi[j]<npar) {
      if (j==1) fprintf(resfile,"psi= %f ",p[psi[j]]);
        else fprintf(resfile,"psi%1d= %f ",j,p[psi[j]]);
      k++;}
    if (chi[j]<npar) {
      if (j==1) fprintf(resfile,"chi= %f ",p[chi[j]]);
        else fprintf(resfile,"chi%1d= %f ",j,p[chi[j]]);
      k++;}
    if (xi[j]<npar) {
      if (j==1) fprintf(resfile,"xi= %f ",p[xi[j]]);
        else fprintf(resfile,"xi%1d= %f ",j,p[xi[j]]);
      k++;}
    }
  if (k!=0) {
    fprintf(resfile,"\n");
    k = 0;
    }
  }
if (theta<npar) {
  fprintf(resfile,"theta= %f ",p[theta]);
  k++; }
if (rfw<npar) {
  fprintf(resfile,"rfw= %f ",p[rfw]);
  k++; }
if (lb<npar) {
  fprintf(resfile,"lb= %f ",p[lb]);
  k++; }
if (gf<npar) {
  fprintf(resfile,"gf= %f ",p[gf]);
  k++; }
if (k!=0) {
  fprintf(resfile,"\n");
  k = 0;
  }
fprintf(resfile,"\n");
fflush(resfile);
}



void envelope_1(param)
float param[];
{
float t,rfw1;
int i,sitectr;
/* Apply the receiver response function, i.e. an Lorentzian with full half-
width rfw and center at viso. This function approximates the resonance curve
for the receiver coil/rf-circuitry */
if (rfw<npar || param[rfw]>0.1) {  /* Allways envelope when iterating on rfw*/
  if (rfw<1.0e-2) nrerror("No convergence for rfw. Try a new estimate");
  for (sitectr=1;sitectr<=sites;sitectr++) {
    rfw1 = param[rfw]*0.5*1.0e6/vrot;
    for (i=0; i<pft; i++) {
      t =( i-pft2)/rfw1;
      t = 1.0/(1.0+t*t);
      ssbint[sitectr][2*i] *= t;
      ssbint[sitectr][2*i+1] *= t;
      }
    }
  } 
}


float rms_1(param)
float param[];

/* calculate rms error for ssb intensities (integrals) */
{
float rms,sum,ratio;
int i,sitectr;
for (sum=0.0,sitectr=1;sitectr<=sites;sitectr++) {
  for (i=0; i<pft; i++) 
    if (expint[sitectr][i]>1.0e-6) {   /* assigned ssb's */
      ssbint[sitectr][2*i] = sqrt(pow(ssbint[sitectr][2*i],2.0)+pow(ssbint[sitectr][2*i+1],2.0));
      sum += ssbint[sitectr][2*i]; }
  }
ratio = expint[sites][pft]/sum;  /* scaling factor giving identical assigned intensity*/
for (sum=0.0, sitectr=1;sitectr<=sites;sitectr++) {
  for (i=0; i<pft; i++) {
    ssbint[sitectr][2*i] *= ratio;  
    if (expint[sitectr][i]>1.0e-6)  
      sum += pow((expint[sitectr][i]-ssbint[sitectr][2*i]),2.0); 
    }
  }
rms = sqrt(sum/ndata);
/*if (!rmsflag) printrms(param,rms); this line for a printout in each iteration cycle*/
return rms;
}


void freqtofid_1(param)
float param[];
{
int i,index,sitectr;
float fmax,f,fi,g1,aintfac,hzpt;

/* ssbint contains the ssb intensities. Insert into the spectrum array, spec
keeping in mind the isotropic shift, spinning rate and digital resolution */

for (i=0;i<=2*npmax;i++) spec[i] = 0.0;
hzpt = sw/(float)(npmax);
fmax = sp+sw;
for (sitectr=1;sitectr<=sites;sitectr++) {
  for (i=0; i<pft; i++) {
    f = param[viso[sitectr]]-(i-pft2)*vrot;
    if ((f>sp) && (f<=(sw+sp))) {
      index = (int)((fmax-f)/hzpt+0.5);
      spec[2*index] += ssbint[sitectr][2*i]; 
      spec[2*index+1] += ssbint[sitectr][2*i+1];
      }
    }
  }

/* Do an inverse FT to generate the FID */
 
four1(spec-1,npmax,1);

/* Normalize the result: Aldermans algorithm require multiplication by nt,
a division with pft is required to obtain a result independent of pft, and
a further division by pft is required to remove the pft factor introduced
by the FT employed in procedures {qcsa/csa/quad}spec  */
/* The g1 function removes the echo at the end of the FID. This is essential 
when the FID is zerofilled */

aintfac = (float)(nt)/(float)(pft*pft);
for (i=0; i<np; i++) {
  fi = pi*i/(float)(np);
  g1 = 0.5*(1.0+cos(fi));
  spec[2*i] *= g1*aintfac;
  spec[2*i+1] *= g1*aintfac;
  }
spec[0] = 0.5*spec[0];
}



/* The following procedures are analogous to the procedures above. See comments above */

extern float pwt;
extern char pulse[];

void envelope_2(param)
float param[];
{
float rfw1,t;
int i;

if (rfw<npar || param[rfw]>0.1) {  /* Allways envelope when iterating on rfw*/
  if (rfw<1.0e-2) nrerror("No convergence for rfw. Try a new estimate");
  rfw1 = param[rfw]*0.5*1.0e6;
  for (i=0; i<fn; i++) {
    t =(-0.5*sw+i*digres)/rfw1;
    t = 1.0/(1.0+t*t);
    spec[2*i] *= t;
    spec[2*i+1] *= t;
    }
  } 
}


float rms_2(param)
float param[];

/* calculate rms error for ssb intensities (phased mode) */
{
float rms,ratio,sum,xlb,xgf,lp,rp,fi,x;
int i;

/* For pulse = 'ideal' the spectrum is already phased. Otherwise phasing is required */

if (strcmp(pulse,"short") == 0 || strcmp(pulse,"finit") == 0) {
  lp = -pwt*sw*pi;
  rp = -lp*0.5;
  for (i=0;i<fn;i++) {
    fi = (float)(i)*lp/(float)(fn) + rp;
    x = spec[2*i]*cos(fi)-spec[2*i+1]*sin(fi);
    spec[2*i+1] = spec[2*i]*sin(fi) + spec[2*i+1]*cos(fi);
    spec[2*i] = x;
    }
  }
four1(spec-1,fn,1);

/* Apply weighting function */

xlb = param[lb];
xgf = param[gf];
/* Check convergence for lb and gf. Stop if the linewidth exceedes sw/4 */
if (xlb != 0.0) 
  if (xlb<0.0 || xlb>(sw/4.0)) nrerror("No convergence for lb. Try a new estimate");
if (xgf != 0.0) 
  if (xgf<0.0 || (sw/4.0)<(sqrt(log(2.0))*2.0/xgf/pi)) nrerror("No convergence for gf. Try a new estimate");

xlb = pi*param[lb]/sw;
if (xgf>0.0) xgf=1.0/(sw*sw*xgf*xgf);
for (i=0; i<fn; i++) {
  spec[2*i] *= exp(-(float)(i)*(xlb+(float)(i)*xgf));
  spec[2*i+1] *= exp(-(float)(i)*(xlb+(float)(i)*xgf));
  }
spec[0] *= 0.5;

four1(spec-1,fn,-1);
for (i=0, sum=0.0;i<fn; i++) 
  if (expspec[i]>1.0e-6) sum += spec[2*i];
ratio = expspec[fn]/sum;
for (i=0,sum=0.0; i<fn; i++) {  /* rms of absorption mode spectrum */ 
  if (expspec[i]>1.0e-6) { 
    sum += (expspec[i]-ratio*spec[2*i])*(expspec[i]-ratio*spec[2*i]); }
    }
rms = sqrt(sum/ndata);
/*if (!rmsflag) printrms(param,rms); this line for a printout in each iteration cycle*/
return rms;
}


void freqtofid_2(param)
float param[];
{
int i;
float fi,g1,aintfac;

  four1(spec-1,fn,1);
/* truncate the FID from fn to np complex datapoints*/
aintfac = (float)(nt)/(float)(pft*pft);
for (i=0; i<np; i++) {
  fi = pi*i/(float)(np);
  g1 = 0.5*(1.0+cos(fi));
  spec[2*i] *= g1*aintfac;
  spec[2*i+1] *= g1*aintfac;
  }
spec[0] *= 0.5;
}
