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

extern float *cfi,*sfi,*c2fi,*s2fi,*c3fi,*s3fi,*c4fi,*s4fi;
extern float mval[],tran[],*param;
extern float pwt,vrotr,gb1r,sfrqr,ival,sw,sp,vrot,digres;
extern int rmsflag,nt,pft,pft2,npar,ntrans,fn;
extern int sites,*cq,*etaq,*csa,*etas,*psi,*chi,*xi,*viso,*amp,theta;
extern int quadflag,csaflag,qcsaflag,progorder;
extern float *spec;
extern char pulse[];
extern FILE *resfile;

extern void tent();
extern void cbase();
extern void qbase();
extern void rotor();
extern void lab();
extern void idealspec();
extern void finitespec();
extern void pulsesetup();
extern void pulserelease();
extern float **matrix();
extern float *vector();
extern void free_matrix();
extern void free_vector();


extern float *shp,*shm,**rho,*rhofid;

float **oldssbint,**curssbint,**oldint,**curint;
float *oldssbfreq,*curssbfreq,*oldfreq,*curfreq,*tptr,**dptr;

float *fid,**ssb,*qang,*cang,*f1th,*f21th,*f22th;
float *Ccr_re,*Ccr_im,*Qcr_re,*Qcr_im,*Crot_re,*Crot_im,*Qrot_re,*Qrot_im;
float *Q2_re,*Q2_im;

void fidsetup(th)
float th;
{
float ct,ct2,ct3,ct4,st,st2,st3,st4;

fid = vector(0,2*pft);
if (csaflag || qcsaflag) {
  cang = vector(0,2*pft);
  Ccr_re = vector(0,2);
  Ccr_im = vector(0,2);
  Crot_re = vector(0,2);
  Crot_im = vector(0,2);}
if (quadflag || qcsaflag) {
  qang = vector(0,2*pft);
  Qcr_re = vector(0,2);
  Qcr_im = vector(0,2);
  Qrot_re = vector(0,2);
  Qrot_im = vector(0,2);}
f1th = vector(0,2);
ct = cos(th);
ct2 = ct*ct;
st = sin(th);
st2=st*st;
/* f1th contain 1'st order theta (angle between B0 and rotor axis) dependencies, which 
   consists of a reduced matrix element and a numerical factor from the
   T20 tensor for the CSA and quadrupole interaction. The CSA numerical factor
   is 1/sqrt(3/2) and is coded in the expressions below. For the quadrupole
   interaction the factor is 1/sqrt(6). For quad we thus need an extra factor of 0.5,
   which is coded into the (6m-3) factor we need to multiply f1th with.*/

f1th[2] = sqrt3h*st2/sqrt3h; /* /sqrt3h from the T20 factor*/
f1th[1] = -sqrt6*st*ct/sqrt3h;
f1th[0] = 0.5*(3.0*ct2-1.0)/sqrt3h;
if (progorder==2) {
  if (qcsaflag) {
    curssbint = matrix(0,2*nt,0,2*pft);
    oldssbint = matrix(0,2*nt,0,2*pft);
    curssbfreq = vector(0,2*nt);
    oldssbfreq = vector(0,2*nt);
    } else {
    curssbint = matrix(0,nt,0,2*pft);
    oldssbint = matrix(0,nt,0,2*pft);
    curssbfreq = vector(0,nt);
    oldssbfreq = vector(0,nt);
    }
  if (quadflag || qcsaflag) {
    Q2_re = vector(1,9);
    Q2_im = vector(1,9);
    f21th = vector(1,9);
    f22th = vector(1,9);
    ct3 = ct2*ct;
    ct4 = ct2*ct2;
    st3=st2*st;
    st4=st2*st2;
/* f21th and f22th contain theta dependencies for the second order expressions*/
    f21th[1] = st4/2.0;
    f21th[2] = -2.0*ct*st3;
    f21th[3] = sqrt6*ct2*st2;
    f21th[4] = st2*(4.0*ct2-1.0)/2.0;
    f21th[5] = -2.0*st*ct3;
    f21th[6] = -sqrt6*ct*st*(2.0*ct2-1.0);
    f21th[7] = -st2*(1.0+ct2)/2.0;
    f21th[8] = (4.0*ct4-3.0*ct2+1.0)/2.0;
    f21th[9] = -3.0*ct2*st2/2.0;

    f22th[1] = st4/8.0;
    f22th[2] = -ct*st3/2.0;
    f22th[3] = sqrt6*(1.0+ct2)*st2/4.0;
    f22th[4] = -st4/2.0;
    f22th[5] = -ct*st*(3.0+ct2)/2.0;
    f22th[6] = sqrt3h*ct*st3;
    f22th[7] = (ct4+6.0*ct2+1.0)/8.0;
    f22th[8] = -st2*(1.0+ct2)/2.0;
    f22th[9] = st4*3.0/8.0;
    }
  }
}

void fidrelease()
{
free_vector(fid,0,2*pft);
if (csaflag || qcsaflag) {
  free_vector(cang,0,2*pft);
  free_vector(Ccr_re,0,2);
  free_vector(Ccr_im,0,2);
   free_vector(Crot_re,0,2);
   free_vector(Crot_im,0,2);}
if (quadflag || qcsaflag) {
  free_vector(qang,0,2*pft);
  free_vector(Qcr_re,0,2);
  free_vector(Qcr_im,0,2);
   free_vector(Qrot_re,0,2);
   free_vector(Qrot_im,0,2);}
free_vector(f1th,0,2);
if (progorder==2) {
  if (qcsaflag) {
    free_matrix(curssbint,0,2*nt,0,2*pft);
    free_matrix(oldssbint,0,2*nt,0,2*pft);
    free_vector(curssbfreq,0,2*nt);
    free_vector(oldssbfreq,0,2*nt);
    } else {
    free_matrix(curssbint,0,nt,0,2*pft);
    free_matrix(oldssbint,0,nt,0,2*pft);
    free_vector(curssbfreq,0,nt);
    free_vector(oldssbfreq,0,nt);
    }
  if (quadflag || qcsaflag) {
    free_vector(Q2_re,1,9);
    free_vector(Q2_im,1,9);
    free_vector(f21th,1,9);
    free_vector(f22th,1,9);
    }
  }
}

void fill(freq1,freq2,int1,int2,iz,param,k)
float *freq1,*freq2,**int1,**int2,param[];
int iz,k;
{
int j,i1,i2,istart,iend;
float g1,g2,g3,fmin,fmid,fmax,intensityr,intensityi,dum;
g1 = freq1[iz];
g2 = freq1[iz+1];
g3 = freq2[iz];

if (g1>g2) {dum = g1; g1 = g2; g2 = dum;}
if (g1>g3) {
  fmin = g3; fmid = g1; fmax = g2;
  } else {
  fmin = g1;
  fmax = (g2>g3) ? (fmid = g3, g2): (fmid = g2, g3);
  }


for (j=0; j<pft; j++) {
  g1 = fmin + sw + sp - param[viso[k]] + (j-pft/2)*vrot;
  g3 = fmax + sw + sp - param[viso[k]] + (j-pft/2)*vrot;
  if (g3>0.0 && g1<sw) {
    intensityr = (int1[iz][2*j]+int1[iz+1][2*j]+int2[iz][2*j])/3.0;
    intensityi = (int1[iz][2*j+1]+int1[iz+1][2*j+1]+int2[iz][2*j+1])/3.0;
    tent(sw+sp-param[viso[k]]+(j-pft/2)*vrot,fmin,fmid,fmax,intensityr,intensityi);
    }
  }

g1 = freq2[iz];
g2 = freq2[iz+1];
g3 = freq1[iz+1];
if (g1>g2) {dum = g1; g1 = g2; g2 = dum;}
if (g1>g3) {
  fmin = g3; fmid = g1; fmax = g2;
  } else {
  fmin = g1;
  fmax = (g2>g3) ? (fmid = g3, g2): (fmid = g2, g3);
  }


for (j=0; j<pft; j++) {
  g1 = fmin + sw + sp - param[viso[k]] + (j-pft/2)*vrot;
  g3 = fmax + sw + sp - param[viso[k]] + (j-pft/2)*vrot;
  if (g3>0.0 && g1<sw) {
    intensityr = (int2[iz][2*j]+int2[iz+1][2*j]+int1[iz+1][2*j])/3.0;
    intensityi = (int2[iz][2*j+1]+int2[iz+1][2*j+1]+int1[iz+1][2*j+1])/3.0;
    tent(sw+sp-param[viso[k]]+(j-pft/2)*vrot,fmin,fmid,fmax,intensityr,intensityi);
    }
  }
}      

extern void shortpulse_c();
extern void shortpulse();
extern void four1();
extern void freqtofid_2();
extern float rms_2();
extern void envelope_2();

float qcsaspec_2(param)
float param[];
{
int iy,iz,index,i,j,it,itrans,sitectr;
int idim;
float mspin,m0,m1,m2;
float rms,th;
float x,y,z,r,r2,r3fac,cb,sb,ca,sa,c2a,s2a;
float t1,t2;
float q2r,q2i,q1r,q1i;
float a0,a1,a2,a3,a4,b1,b2,b3,b4;

/* All procedures in this module employ the second order, average Hamiltonian to calculate the spectra with lineshape. Note, only pure quad. interaction is retained in the second order. Second order csa and the mix terms (Hq*Hcsa) are neglected. 
This procedure is the general procedure for combined quadrupole and csa interaction.*/
/* See also comments in procedure qcsaspec_1 */
for (i=0; i<2*fn; spec[i++]=0.0);

idim = (int)(2.0*ival+1.1);
pulsesetup(idim,ntrans);
th = thetamag+param[theta]*pi/180.0; /* convert mismatch to absolut value */
fidsetup(th);

for (sitectr=1;sitectr<=sites;sitectr++) {
/* interaction parameters in the molecular frame*/
  cbase(param,Ccr_re,Ccr_im,sitectr);
  qbase(param,Qcr_re,Qcr_im,sitectr);
  for (itrans=1;itrans<=ntrans; itrans++) {
    mspin=mval[itrans];
    if (strncmp(pulse,"short",5) == 0) {
      if (mspin <1.0) shortpulse_c(shp,sitectr,param);
        else shortpulse(shp,shm,mspin,sitectr,param);
      }

/* loop through mspin to mspin-1 and (-mspin+1) to (-mspin) */
    do {
      m0 = (6.0*mspin-3.0)/2.0; 
/* m0 should be (6m-3)/sqrt6, but 1/sqrt(3/2) included in f1th*/
      m1 = (2*ival*(ival+1.0)-12.0*mspin*(mspin-1.0)-4.5)/(-sfrqr);
      m2 = (2*ival*(ival+1.0)-6.0*mspin*(mspin-1.0)-3.0)/(-2.0*sfrqr);
/* step through all crystallite orientations according to the algorithm by
Alderman et al, J. Chem. Phys.,84, 3717, 1986 */

curint=curssbint;
oldint=oldssbint;
curfreq = curssbfreq;
oldfreq = oldssbfreq;
for (i=0;i<2*pft;i++) 
  for (j=0;j<=2*nt;j++)  curint[j][i] = 0.0;
for (iy=0; iy<=nt; iy++) {
  dptr = oldint; oldint = curint; curint = dptr;
  tptr = oldfreq; oldfreq = curfreq; curfreq = tptr;
  for (i=0;i<2*pft;i++) 
    for (j=0;j<=2*nt;j++)  curint[j][i] = 0.0;
  for (iz=0; iz<=nt; iz++) {
    if (iy>(nt-iz)) {
      x = nt-iz-iy;  /* this is alfa>90 degrees */
      y = nt-iz;
      z = nt-iy; 
      } else { 
      x = nt-iy-iz;   /* and alfa <=90 degrees */
      y = iy;
      z = iz;
      }
    r2 = (x*x+y*y+z*z);
    r = sqrt(r2);
    r3fac = tran[itrans]*param[amp[sitectr]]/(r2*r);
    cb = z/r;
    sb = sqrt(1.0-cb*cb);
    if (sb>1.0e-5) {
      c2a = (x*x-y*y)/(r2*sb*sb);
      s2a = 2.0*x*y/(r2*sb*sb);
      sa = y/(r*sb);
      ca = x/(r*sb);
      } else {
      c2a = ca = 1.0;
      s2a = sa = 0.0;
      }
   index = iz;    /* loop counter for complete alfa averaging */
   do {  /*loop twice for alfa (index=iz) and -alfa (index=2*nt-iz) */
/* interaction parameters in the rotor fixed frame*/
      rotor(Crot_re,Crot_im,Ccr_re,Ccr_im,c2a,s2a,ca,sa,cb,sb);
      rotor(Qrot_re,Qrot_im,Qcr_re,Qcr_im,c2a,s2a,ca,sa,cb,sb);
      lab(Q2_re,Q2_im,Qrot_re,Qrot_im);
      q2r = f1th[2]/2.0*Qrot_re[2]; /* remember the factor 1/2 for f1th */
      q2i = f1th[2]/2.0*Qrot_im[2];
      q1r = f1th[1]/2.0*Qrot_re[1];
      q1i = f1th[1]/2.0*Qrot_im[1];
      a4 = b4 = a3 = b3 = a2 = b2 = a1 = b1 = a0 = 0.0;
/* first order terms */
      a2 += f1th[2]*(Crot_re[2]+Qrot_re[2]*m0);
      a1 += f1th[1]*(Crot_re[1]+Qrot_re[1]*m0);
      b2 += f1th[2]*(Crot_im[2]+Qrot_im[2]*m0);
      b1 += f1th[1]*(Crot_im[1]+Qrot_im[1]*m0);
      a0 += f1th[0]*(Crot_re[0]+Qrot_re[0]*m0);
/* second order terms */
      a4 += Q2_re[1]*(m2*f22th[1]+m1*f21th[1]);
      b4 += Q2_im[1]*(m2*f22th[1]+m1*f21th[1]);
      a3 += Q2_re[2]*(m2*f22th[2]+m1*f21th[2]);
      b3 += Q2_im[2]*(m2*f22th[2]+m1*f21th[2]);
      a2 += Q2_re[3]*(m2*f22th[3]+m1*f21th[3]);
      b2 += Q2_im[3]*(m2*f22th[3]+m1*f21th[3]);
      a2 += Q2_re[4]*(m2*f22th[4]+m1*f21th[4]);
      b2 += Q2_im[4]*(m2*f22th[4]+m1*f21th[4]);
      a1 += Q2_re[5]*(m2*f22th[5]+m1*f21th[5]);
      b1 += Q2_im[5]*(m2*f22th[5]+m1*f21th[5]);
      a1 += Q2_re[6]*(m2*f22th[6]+m1*f21th[6]);
      b1 += Q2_im[6]*(m2*f22th[6]+m1*f21th[6]);
      a0 += Q2_re[7]*(m2*f22th[7]+m1*f21th[7]);
      a0 += Q2_re[8]*(m2*f22th[8]+m1*f21th[8]);
      a0 += Q2_re[9]*(m2*f22th[9]+m1*f21th[9]);
      curfreq[index] = a0/(2.0*pi);
      for (it=1,qang[0]=0.0; it<(pft/2); it++) { 
        t1 = -b1*cfi[it]-b2*c2fi[it];
        t1 += -b3*c3fi[it]-b4*c4fi[it];
        t2 = a1*sfi[it]+a2*s2fi[it];
        t2 += a3*s3fi[it]+a4*s4fi[it];
        qang[it] = t1+t2;
        qang[pft-it] = t1-t2;
        } 
      qang[pft/2] = -b1*cfi[pft/2]-b3*c3fi[pft/2];
      fid[0] = 1.0;
      fid[1] = 0.0;
      for (it=1; it<pft; it++) 
        cossin(qang[it],&fid[2*it],&fid[2*it+1]);
      
      if (strncmp(pulse,"finit",5) == 0) {
        getrho(q2r,q2i,q1r,q1i,rho,&mspin-1,1,FALSE,sitectr,param); 
        finitespec(curint[index],rho[1],rhofid,fid,r3fac);
        } else {
        idealspec(curint[index],fid,r3fac);
        if (strncmp(pulse,"short",5) == 0) {
          for (i=0;i<(2*pft-1);i += 2) {
            curint[index][i+1] = -curint[index][i]*shp[i];
            curint[index][i] = curint[index][i]*shp[i+1];
            }
          } 
        }
      sa = -sa;
      s2a = -s2a;
      index = 2*nt-index;  /* do the 3.rd and 4.th octants */
      } while (index>nt);
    }  /* end of iz loop */
  if (iy != 0) {

    for (iz=0; iz<nt; iz++) {
      fill(oldfreq,curfreq,oldint,curint,iz,param,sitectr);
      } 
    for (iz=nt; iz<2*nt; iz++) {
      fill(curfreq,oldfreq,curint,oldint,iz,param,sitectr);
      } 

    }   /* end of iy != 0 */
  } /* end of iy-loop */
 
      mspin = -mspin+1.0;
      } while ( (mspin <= 0.0));
    } /* end of itrans-loop*/
  } /* end of sitectr loop*/

rms = 0.0;
envelope_2(param);
if ((npar > 0) || rmsflag)
  rms = rms_2(param);
  else freqtofid_2(param);

pulserelease(idim,ntrans);
fidrelease();

return rms;
}


float csaspec_2(param)
float param[];
{
int iy,iz,index,i,j,sitectr,it;
int idim;
float mspin,fac;
float rms,th;
float x,y,z,r,r2,r3fac,cb,sb,c2a,s2a;
float t1,t2;
float c2r,c2i,c1r,c1i;
float a0,a1,a2,b1,b2;

/*This procedure for the spin 1/2 case */
for (i=0; i<2*fn; spec[i++]=0.0);

idim = (int)(2.0*ival+1.1);
pulsesetup(idim,ntrans);
th = thetamag+param[theta]*pi/180.0; /* convert mismatch to absolut value */
fidsetup(th);

mspin=mval[1];

for (sitectr=1;sitectr<=sites;sitectr++) {
  Ccr_re[0] = sqrt3h*param[csa[sitectr]]*sfrqr;
  Ccr_re[2] = -0.5*param[csa[sitectr]]*param[etas[sitectr]]*sfrqr;
  Ccr_re[1] = Ccr_im[0] = Ccr_im[1] = Ccr_im[2] = 0.0;
if (strncmp(pulse,"short",5) == 0) shortpulse(shp,shm,mspin,sitectr,param);

  curint=curssbint;
  oldint=oldssbint;
  for (i=0;i<2*pft;i++) 
    for (j=0;j<=nt;j++)  curint[j][i] = 0.0;
  curfreq = curssbfreq;
  oldfreq = oldssbfreq;
  fac = 2.0*param[amp[sitectr]];
  for (iy=0; iy<=nt; iy++) {
    dptr = oldint; oldint = curint; curint = dptr;
    tptr = oldfreq; oldfreq = curfreq; curfreq = tptr;
    for (i=0;i<2*pft;i++) 
      for (j=0;j<=nt;j++)  curint[j][i] = 0.0;
    for (iz=0; iz<=nt; iz++) {
      if (iy>(nt-iz)) {
        x = nt-iz-iy;
        y = nt-iz;
        z = nt-iy; 
        } else { 
        x = nt-iy-iz;  
        y = iy;
        z = iz;
        }
      index = iz;   
      r2 = (x*x+y*y+z*z);
      r = sqrt(r2);
      r3fac = fac/(r2*r);
      cb = z/r;
      sb = sqrt(1.0-cb*cb);
      if (sb>1.0e-5) {
        c2a = (x*x-y*y)/(r2*sb*sb);
        s2a = 2.0*x*y/(r2*sb*sb);
        } else {
        c2a =  1.0;
        s2a =  0.0;
        }

      rotor(Crot_re,Crot_im,Ccr_re,Ccr_im,c2a,s2a,1.0,0.0,cb,sb);
      c2r = f1th[2]*Crot_re[2];
      c2i = f1th[2]*Crot_im[2];
      c1r = f1th[1]*Crot_re[1];
      c1i = f1th[1]*Crot_im[1];
      a2 = b2 = a1 = b1 = a0 = 0.0;
/* first order terms */
      a2 += f1th[2]*Crot_re[2];
      a1 += f1th[1]*Crot_re[1];
      b2 += f1th[2]*Crot_im[2];
      b1 += f1th[1]*Crot_im[1];
      a0 += f1th[0]*Crot_re[0];
      curfreq[index] = a0/(2.0*pi);
      for (it=1,cang[0]=0.0; it<(pft/2); it++) { 
        t1 = -b1*cfi[it]-b2*c2fi[it];
        t2 = a1*sfi[it]+a2*s2fi[it];
        cang[it] = t1+t2;
        cang[pft-it] = t1-t2;
        } 
      cang[pft/2] = -b1*cfi[pft/2];

      fid[0] = 1.0;
      fid[1] = 0.0;
      for (it=1; it<pft; it++) 
        cossin(cang[it],&fid[2*it],&fid[2*it+1]);
      if (strncmp(pulse,"finit",5) == 0) {
        getrho(c2r,c2i,c1r,c1i,rho,&mspin-1,1,FALSE,sitectr,param); 
        finitespec(curint[index],rho[1],rhofid,fid,r3fac);
        } else {
        idealspec(curint[index],fid,r3fac);
        if (strncmp(pulse,"short",5) == 0) {
          for (i=0;i<(2*pft-1);i += 2) {
            curint[index][i+1] = -curint[index][i]*shp[i];
            curint[index][i]= curint[index][i]*shp[i+1];
            }
          } 
        }
      }  /* end of iz loop */
    
    if (iy != 0) {
      for (iz=0; iz<nt; iz++) {
        fill(oldfreq,curfreq,oldint,curint,iz,param,sitectr);
        } 
      }   /* end of iy != 0 */
  
    } /* end of iy-loop */
  } /* end of sitectr-loop*/

rms = 0.0;
envelope_2(param);
if ((npar > 0) || rmsflag)
  rms = rms_2(param);
  else freqtofid_2(param);

pulserelease(idim,ntrans);
fidrelease();

return rms;
}



float quadspec_2(param)
float param[];
{
int iy,iz,index,i,j,sitectr,it,itrans;
int idim;
float mspin,m0,m1,m2,fac;
float rms,th;
float x,y,z,r,r2,r3fac,cb,sb,c2a,s2a;
float t1,t2;
float q2r,q2i,q1r,q1i;
float a0,a1,a2,a3,a4,b1,b2,b3,b4;


/* This procedure for a pure quadrupole interaction */
for (i=0; i<2*fn; spec[i++]=0.0);

idim = (int)(2.0*ival+1.1);
pulsesetup(idim,ntrans);
th = thetamag+param[theta]*pi/180.0; 
fidsetup(th);

for (sitectr=1;sitectr<=sites;sitectr++) {
  Qcr_re[0] = sqrt6*param[cq[sitectr]]/(4.0*ival*(2.0*ival-1.0));
  Qcr_re[2] = -Qcr_re[0]*param[etaq[sitectr]]/sqrt6;
  Qcr_re[1] = Qcr_im[0] = Qcr_im[1] = Qcr_im[2] = 0.0;
  for (itrans=1;itrans<=ntrans; itrans++) {
    mspin=mval[itrans];
  
    if (strncmp(pulse,"short",5) == 0) {
      if (mspin <1.0) shortpulse_c(shp,sitectr,param);
        else shortpulse(shp,shm,mspin,sitectr,param);
      }
  
 
/* if mspin <> 1/2 loop through mspin to mspin-1 and (-mspin+1) to (-mspin) */

    do {
      m0 = (6.0*mspin-3.0)/2.0;
      m1 = (2*ival*(ival+1.0)-12.0*mspin*(mspin-1)-4.5)/(-sfrqr);
      m2 = (2*ival*(ival+1)-6*mspin*(mspin-1)-3)/(-2*sfrqr);

      curint=curssbint;
      oldint=oldssbint;
      for (i=0;i<2*pft;i++) 
        for (j=0;j<=nt;j++)  curint[j][i] = 0.0;
      curfreq = curssbfreq;
      oldfreq = oldssbfreq;
      fac = 2.0*param[amp[sitectr]];

for (iy=0; iy<=nt; iy++) {
  dptr = oldint; oldint = curint; curint = dptr;
  tptr = oldfreq; oldfreq = curfreq; curfreq = tptr;
  for (i=0;i<2*pft;i++) 
    for (j=0;j<=nt;j++)  curint[j][i] = 0.0;
  for (iz=0; iz<=nt; iz++) { 
    if (iy>(nt-iz)) {
      x = nt-iz-iy;       
      y = nt-iz;
      z = nt-iy; 
      } else { 
      x = nt-iy-iz;  
      y = iy;
      z = iz;
      }
    index = iz;
    r2 = (x*x+y*y+z*z);
    r = sqrt(r2);
    r3fac = tran[itrans]*fac/(r2*r);
    cb = z/r;
    sb = sqrt(1.0-cb*cb);
    if (sb>1.0e-5) {
      c2a = (x*x-y*y)/(r2*sb*sb);
      s2a = 2.0*x*y/(r2*sb*sb);
      } else {
      c2a  = 1.0;
      s2a  = 0.0;
      }

      rotor(Qrot_re,Qrot_im,Qcr_re,Qcr_im,c2a,s2a,1.0,0.0,cb,sb);
      lab(Q2_re,Q2_im,Qrot_re,Qrot_im);
      q2r = f1th[2]/2.0*Qrot_re[2];
      q2i = f1th[2]/2.0*Qrot_im[2];
      q1r = f1th[1]/2.0*Qrot_re[1];
      q1i = f1th[1]/2.0*Qrot_im[1];
      a4 = b4 = a3 = b3 = a2 = b2 = a1 = b1 = a0 = 0.0;
/* first order terms */
      a2 += f1th[2]*Qrot_re[2]*m0;
      a1 += f1th[1]*Qrot_re[1]*m0;
      b2 += f1th[2]*Qrot_im[2]*m0;
      b1 += f1th[1]*Qrot_im[1]*m0;
      a0 += f1th[0]*Qrot_re[0]*m0;
/* second order terms */
      a4 += Q2_re[1]*(m2*f22th[1]+m1*f21th[1]);
      b4 += Q2_im[1]*(m2*f22th[1]+m1*f21th[1]);
      a3 += Q2_re[2]*(m2*f22th[2]+m1*f21th[2]);
      b3 += Q2_im[2]*(m2*f22th[2]+m1*f21th[2]);
      a2 += Q2_re[3]*(m2*f22th[3]+m1*f21th[3]);
      b2 += Q2_im[3]*(m2*f22th[3]+m1*f21th[3]);
      a2 += Q2_re[4]*(m2*f22th[4]+m1*f21th[4]);
      b2 += Q2_im[4]*(m2*f22th[4]+m1*f21th[4]);
      a1 += Q2_re[5]*(m2*f22th[5]+m1*f21th[5]);
      b1 += Q2_im[5]*(m2*f22th[5]+m1*f21th[5]);
      a1 += Q2_re[6]*(m2*f22th[6]+m1*f21th[6]);
      b1 += Q2_im[6]*(m2*f22th[6]+m1*f21th[6]);
      a0 += Q2_re[7]*(m2*f22th[7]+m1*f21th[7]);
      a0 += Q2_re[8]*(m2*f22th[8]+m1*f21th[8]);
      a0 += Q2_re[9]*(m2*f22th[9]+m1*f21th[9]);
      curfreq[index] = a0/(2.0*pi);
      
      for (it=1,qang[0]=0.0; it<(pft/2); it++) { 
        t1 = -b1*cfi[it]-b2*c2fi[it];
        t1 += -b3*c3fi[it]-b4*c4fi[it];
        t2 = a1*sfi[it]+a2*s2fi[it];
        t2 += a3*s3fi[it]+a4*s4fi[it];
        qang[it] = t1+t2;
        qang[pft-it] = t1-t2;
        } 
      qang[pft/2] = -b1*cfi[pft/2]-b3*c3fi[pft/2];
     
      fid[0] = 1.0;
      fid[1] = 0.0;
      for (it=1; it<pft; it++) 
        cossin(qang[it],&fid[2*it],&fid[2*it+1]);
      if (strncmp(pulse,"finit",5) == 0) {
        getrho(q2r,q2i,q1r,q1i,rho,&mspin-1,1,FALSE,sitectr,param); 
        finitespec(curint[index],rho[1],rhofid,fid,r3fac);
        } else {
        idealspec(curint[index],fid,r3fac);
        if (strncmp(pulse,"short",5) == 0) {
          for (i=0;i<(2*pft-1);i += 2) {
            curint[index][i+1] = -curint[index][i]*shp[i];
            curint[index][i] = curint[index][i]*shp[i+1];
            }
          }
        }
      }  /* end of iz loop */
  
  if (iy != 0) {

    for (iz=0; iz<nt; iz++) {
      fill(oldfreq,curfreq,oldint,curint,iz,param,sitectr);
      } 

    }   /* end of iy != 0 */
  } /* end of iy-loop */

      mspin = -mspin+1.0;
      } while ( (mspin <= 0.0));
    } /* end of itrans-loop*/
  }  /* end of sitectr-loop */

rms = 0.0;
envelope_2(param);
if ((npar > 0) || rmsflag)
  rms = rms_2(param);
  else freqtofid_2(param);

pulserelease(idim,ntrans);
fidrelease();

return rms;

}
