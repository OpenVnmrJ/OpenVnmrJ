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
extern float *cc,*ss;


void cossin(psi, cpsi, spsi)
float psi, *cpsi, *spsi;
{
/*interpolate in arrays cc, ss and return cos(psi), sin(psi)*/
int k,iarg;
float del,aa,sign,psi1;
if (psi<0.0) {
  sign = -1.0;
  psi1 = -psi;
  } else {
  sign = 1.0;
  psi1 = psi;}
k = floor(psi1/(2.0*pi)); 
aa = psi1-k*(2.0*pi);
iarg = floor(aa/rad);
del = aa-iarg*rad;
*cpsi = cc[iarg]-del*ss[iarg];
*spsi = sign*(ss[iarg]+del*cc[iarg]);
}

extern void finitespec();
extern void idealspec();
extern void pulsesetup();
extern void fidsetup();
extern void pulserelease();
extern void fidrelease();

extern void cbase();
extern void qbase();
extern void rotor();
extern void lab();
extern void shortpulse();
extern void shortpulse_c();
extern void four1();
extern void freqtofid_1();
extern float rms_1();
extern void envelope_1();

extern float **ssb,*shp,*shm,**rho,*rhofid;

extern float *fid,*qang,*cang,*f1th;
extern float *Ccr_re,*Ccr_im,*Qcr_re,*Qcr_im,*Crot_re,*Crot_im,*Qrot_re,*Qrot_im;
extern float gb1r,pwt,ival;
extern float *cfi,*sfi,*c2fi,*s2fi;
extern float mval[],tran[];
extern float vrotr,sfrqr;
extern int nt,pft,npar,ntrans,cflag,sflag,pulsestep,rmsflag,theta;
extern int *csa,*etas,*cq,*etaq,*amp,sites;
extern char pulse[];
extern float **ssbint;

float qcsaspec_1(param)
float param[];
{
int iy,iz,index,i,j,it,ndim,sitectr,mctr;
float th,fac,m0,rms;
float x,y,z,r,r2,r3fac,cb,sb,ca,sa,c2a,s2a,ampfac;
float aq2,bq2,aq1,bq1,ac2,bc2,ac1,bc1;


/* This procedure calculates the ssb intensities, i.e. only time-dependent 
   1'st order terms in the average Hamiltonian are retained.  */

ndim = (int)(2.0*ival+1.1);
pulsesetup(ndim,ntrans);

th = thetamag+param[theta]*pi/180.0; /* convert mismatch to absolut value */
fidsetup(th);

for (sitectr=1;sitectr<=sites;sitectr++) { /* loop through each site */
  for (i=0;i<=2*pft;i++) ssbint[sitectr][i]=0.0;
  if (strncmp(pulse,"short",5)==0) 
    for (i=-ntrans;i<=ntrans;i++) 
      for (j=0;j<2*pft;j++) ssb[i][j] = 0.0;

  cbase(param,Ccr_re,Ccr_im,sitectr);
  qbase(param,Qcr_re,Qcr_im,sitectr);
  

/* step through all crystallite orientations according to the algorithm by
Alderman et al, J. Chem. Phys.,84, 3717, 1986 */

for (iy=0; iy<=nt; iy++) {
  for (iz=0; iz<=nt; iz++) {
/* intensity correction factors, cf the interpolation scheme in procedure qcsaspec_2 */
    if ( ((iy==0) && (iz==0)) || ((iy==nt) && (iz==nt)) ) fac = 1.0/3.0;
      else if ( ((iy==0) && (iz==nt)) || ((iy==nt) && (iz==0)) ) fac = 2.0/3.0;
      else if ( (iy==0) || (iy==nt) || (iz==0) || (iz==nt) ) fac = 3.0/3.0;
      else fac = 6.0/3.0;

    if (iy>(nt-iz)) {
      x = nt-iz-iy;  /* alfa > 90 degrees */
      y = nt-iz;
      z = nt-iy; 
      } else { 
      x = nt-iy-iz;   /* alfa <= 90 degrees */
      y = iy;
      z = iz;
      }
    r2 = (x*x+y*y+z*z);
    r = sqrt(r2);
    r3fac = param[amp[sitectr]]*fac/(r2*r);
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
    index = iz;   
    do {  /* two step loop for 0<alfa<180 (index=iz) and 0>alfa>-180 (index=2*nt-iz) */
      rotor(Crot_re,Crot_im,Ccr_re,Ccr_im,c2a,s2a,ca,sa,cb,sb);
      rotor(Qrot_re,Qrot_im,Qcr_re,Qcr_im,c2a,s2a,ca,sa,cb,sb);

      aq2 = f1th[2]*Qrot_re[2]; 
      bq2 = f1th[2]*Qrot_im[2]; 
      aq1 = f1th[1]*Qrot_re[1]; 
      bq1 = f1th[1]*Qrot_im[1]; 
      ac2 = f1th[2]*Crot_re[2]; 
      bc2 = f1th[2]*Crot_im[2]; 
      ac1 = f1th[1]*Crot_re[1]; 
      bc1 = f1th[1]*Crot_im[1];

/* cang, qang contains the acquired phase angle due to csa and quadrupole
   interactions, respectively */
      for (it=1,cang[0]=0.0; it<pft; it++) 
        cang[it] = -bc1*cfi[it]-bc2*c2fi[it]+ac1*sfi[it]+ac2*s2fi[it];
      if (sflag) /* qang required for satellite transitions, only*/ 
        for (it=1,qang[0]=0.0; it<pft; it++) 
          qang[it] = -bq1*cfi[it]-bq2*c2fi[it]+aq1*sfi[it]+aq2*s2fi[it];
 
/* To calculate the  FID we need a summation over the three Euler angles alfa, beta, gamma, and to obtain the frequency spectrum we need to FT the FID.
When we combine the gamma summation and the FT we can do the calculations a lot faster. The gamma averaged spectrum (for the m to m-1 transition) is then calculated as

S(n) = FT(F(t))(n) * (FT(rho(gamma)*F(t))(n) )~
where ~ indicates complex conjugation,
rho is the m,(m-1) element of the density matrix after the pulse, 
F(t) = Exp(I*fi(t)), with fi the phase angle for the magnetization at time t, and for gamma=0. n indicates the n'th sideband (on the left -hand side) and the n'th Fourier component (right-hand side). 

This relation is programmed below for the "finite" pulse.

For the "ideal" pulse rho(gamma) = I*transition probality  and we thus find
S(n) = FT(F(t)) * (FT(F(t)) )~ *I*transition probality.

For the "short" pulse we take S(n) = G(n)*S'(n) with S'(n) the ideal spectrum and G(n) an excitation function.
*/

      if (strncmp(pulse,"finit",5) == 0) {
/* calculate the density matrix at end of the pulse */
        getrho(aq2/2.0,bq2/2.0,aq1/2.0,bq1/2.0,rho,mval,ntrans,TRUE,sitectr,param);
        }
      for (mctr=1; mctr<=ntrans; mctr++) {  /* loop through all transitions */
        m0 = (6.0*mval[mctr]-3.0)/2.0;
        ampfac = r3fac*tran[mctr];
        do {
        fid[0] = 1.0;
        fid[1] = 0.0;
          if (mval[mctr] == 0.5) { 
          for (it=1; it<pft; it++) 
            cossin(cang[it],&fid[2*it],&fid[2*it+1]); /*F(t) */
            } else {
            for (it=1; it<pft; it++) /* remember the 6m-3 factor for quad*/ 
              cossin(m0*qang[it]+cang[it],&fid[2*it],&fid[2*it+1]);
            }
          if (strncmp(pulse,"finit",5) == 0) {
            if (m0>=0) finitespec(ssbint[sitectr],rho[mctr],rhofid,fid,ampfac);
               else finitespec(ssbint[sitectr],rho[-mctr],rhofid,fid,ampfac);

            } else if (strncmp(pulse,"short",5)==0) {
            if (m0>=0) idealspec(ssb[mctr],fid,ampfac);
              else idealspec(ssb[-mctr],fid,ampfac);

            } else idealspec(ssbint[sitectr],fid,ampfac);
            m0 = -m0;  /* now do the (-m+1) to -m transition */
          } while ((mval[mctr]>0.5) && (m0<0.0));
        }
      sa = -sa;
      s2a = -s2a;
      index = 2*nt-index;  /* do the 3.rd and 4.th octants */
      } while (index>nt);
    }  /* end of iz loop */
  }  /* of iy-loop */ 

if (strncmp(pulse,"short",5) == 0) {
/* multiply the ideal spectrum with a "pulse effect" function. This function,
which is calculated in procedures shortpulse and shortpulse_c, is valid for a 
static sample, but may be used as an approximation to a short pulse in MAS */
  
  for (i=1;i<=ntrans;i++) {
    if (mval[i]<1.0) {
      for (j=0;j<2*pft;j++) shp[j] = 0.0; 
      shortpulse_c(shp,sitectr,param);
      for (j=0;j<pft;j++) {
        ssbint[sitectr][2*j+1] -= shp[2*j]*ssb[i][2*j];
        ssbint[sitectr][2*j] += shp[2*j+1]*ssb[i][2*j];
        }
      } else {
      for (j=0;j<2*pft;j++) { shp[j] = shm[j] = 0.0; }
      shortpulse(shp,shm,mval[i],sitectr,param);
      for (j=0;j<pft;j++) {
        ssbint[sitectr][2*j+1] -= shp[2*j]*ssb[i][2*j];
        ssbint[sitectr][2*j] += shp[2*j+1]*ssb[i][2*j];
        ssbint[sitectr][2*j+1] -= shm[2*j]*ssb[-i][2*j];
        ssbint[sitectr][2*j] += shm[2*j+1]*ssb[-i][2*j];
        }
      }
    }
  }
  } /* end of sitectr loop*/

pulserelease(ndim,ntrans);
rms = 0.0;
envelope_1(param);
if ((npar > 0) || rmsflag) 
  rms = rms_1(param); /* calculate rms error */
  else freqtofid_1(param);  /* inverse FT to construct the FID */
fidrelease();
return rms;
}


float csaspec_1(param)
float param[];
{
int iy,iz,i,it,ndim,sitectr;
float th,fac,rms;
float x,y,z,r,r2,r3fac,cb,sb,c2a,s2a;
float a2,b2,a1,b1;

/* This procedure is for a spin 1/2. See comments in procedure qcsaspec_1. */
ndim = (int)(2.0*ival+1.1); 
pulsesetup(ndim,ntrans);

th = (thetamag+param[theta]*pi/180.0); /* convert mismatch to absolut value */
fidsetup(th);

for (sitectr=1;sitectr<=sites;sitectr++) { 
  for (i=0;i<=2*pft;i++) ssbint[sitectr][i]=0.0;
  if (strncmp(pulse,"short",5)==0) for (i=0;i<=2*pft;i++) ssb[1][i]=0.0;
  Ccr_re[0] = sqrt3h*param[csa[sitectr]]*sfrqr;
  Ccr_re[2] = -0.5*param[csa[sitectr]]*param[etas[sitectr]]*sfrqr;
  Ccr_re[1] = Ccr_im[0] = Ccr_im[1] = Ccr_im[2] = 0.0;

  
/* step through all crystallite orientations according to the algorithm by
Alderman et al, J. Chem. Phys.,84, 3717, 1986 */

for (iy=0; iy<=nt; iy++) {
  for (iz=0; iz<=nt; iz++) { /* intensity factor as above, but multiplied by two, since this procedures sums only 0<=alfa<=pi */
    if ( ((iy==0) && (iz==0)) || ((iy==nt) && (iz==nt)) ) fac = 2.0/3.0;
      else if ( ((iy==0) && (iz==nt)) || ((iy==nt) && (iz==0)) ) fac = 4.0/3.0;
      else if ( (iy==0) || (iy==nt) || (iz==0) || (iz==nt) ) fac = 6.0/3.0;
      else fac = 12.0/3.0;
    if (iy>(nt-iz)) {
      x = (float)(nt-iz-iy);  
      y = (float)(nt-iz);
      z = (float)(nt-iy); 
      } else { 
      x = (float)(nt-iy-iz);  
      y = (float)(iy);
      z = (float)(iz);
      }
    r2 = (x*x+y*y+z*z);
    r = sqrt(r2);
    r3fac = param[amp[sitectr]]*fac/(r2*r);
    cb = z/r;
    sb = sqrt(1.0-cb*cb);
    if (sb>1.0e-5) {
      c2a = (x*x-y*y)/(r2*sb*sb);
      s2a = 2.0*x*y/(r2*sb*sb);
      } else {
      c2a = 1.0;
      s2a = 0.0;
      }

    rotor(Crot_re,Crot_im,Ccr_re,Ccr_im,c2a,s2a,1.0,0.0,cb,sb);
    a2 = f1th[2]*Crot_re[2];
    b2 = f1th[2]*Crot_im[2];
    a1 = f1th[1]*Crot_re[1];
    b1 = f1th[1]*Crot_im[1];

    for (it=1,cang[0]=0.0; it<pft; it++) 
      cang[it] = -b1*cfi[it]-b2*c2fi[it]+a1*sfi[it]+a2*s2fi[it];
        
    fid[0] = 1.0;
    fid[1] = 0.0;
    for (it=1; it<pft; it++) 
      cossin(cang[it],&fid[2*it],&fid[2*it+1]);
    if (strncmp(pulse,"finit",5) == 0) {
      getrho(a2,b2,a1,b1,rho,mval,ntrans,FALSE,sitectr,param);
      finitespec(ssbint[sitectr],rho[1],rhofid,fid,r3fac);
      } else if(strncmp(pulse,"short",5)==0) 
      idealspec(ssb[1],fid,r3fac);
      else idealspec(ssbint[sitectr],fid,r3fac);      
    }  /* end of iz loop */
  }  /* of iy-loop */ 
if (strncmp(pulse,"short",5) == 0) {
  shortpulse(shp,shm,0.5,sitectr,param);
  for (i=0;i<pft;i++) {
    ssbint[sitectr][2*i+1] = -shp[2*i]*ssb[1][2*i];
    ssbint[sitectr][2*i] = shp[2*i+1]*ssb[1][2*i];
    }
  }

  } /* end of sitectr-loop*/
pulserelease(ndim,ntrans);

envelope_1(param);
if ((npar > 0) || rmsflag) 
  rms = rms_1(param); /* calulate rms error */
  else freqtofid_1(param);
fidrelease();
return rms;
}

float quadspec_1(param)
float param[];
{
int iy,iz,i,j,it,ndim,sitectr,mctr;
float *fid1;
float th,fac,m0,rms,ampfac;
float x,y,z,r,r2,r3fac,cb,sb,c2a,s2a;
float a1,b1,a2,b2;

/* This procedure is for a pure quadrupolar interaction */
fid1 = vector(0,2*pft);

ndim = (int)(2.0*ival+1.1); 
pulsesetup(ndim,ntrans);

th = thetamag+param[theta]*pi/180.0; 
fidsetup(th);

for (sitectr=1;sitectr<=sites;sitectr++) { 
  for (i=0;i<2*pft;i++) ssbint[sitectr][i]=0.0;
  if (strncmp(pulse,"short",5)==0) 
    for (i=-ntrans;i<=ntrans;i++) {
      for (j=0;j<=2*pft;j++) ssb[i][j] = 0.0; }

  Qcr_re[0] = sqrt6*param[cq[sitectr]]/(4.0*ival*(2.0*ival-1));
  Qcr_re[2] = -Qcr_re[0]*param[etaq[sitectr]]/sqrt6;
  Qcr_re[1] = Qcr_im[0] = Qcr_im[1] = Qcr_im[2] = 0.0;


/* step through all crystallite orientations according to the algorithm by
Alderman et al, J. Chem. Phys.,84, 3717, 1986 */

for (iy=0; iy<=nt; iy++) {
  for (iz=0; iz<=nt; iz++) {
    if ( ((iy==0) && (iz==0)) || ((iy==nt) && (iz==nt)) ) fac = 2.0/3.0;
      else if ( ((iy==0) && (iz==nt)) || ((iy==nt) && (iz==0)) ) fac = 4.0/3.0;
      else if ((iy==0) || (iy==nt) || (iz==0) || (iz==nt)) fac = 6.0/3.0;
      else fac = 12.0/3.0;
        
    if (iy>(nt-iz)) {
      x = nt-iz-iy; 
      y = nt-iz;
      z = nt-iy; 
      } else { 
      x = nt-iy-iz;
      y = iy;
      z = iz;
      }
    r2 = (x*x+y*y+z*z);
    r = sqrt(r2);
    r3fac = param[amp[sitectr]]*fac/(r2*r);
    cb = z/r;
    sb = sqrt(1.0-cb*cb);
    if (sb>1.0e-5) {
      c2a = (x*x-y*y)/(r2*sb*sb);
      s2a = 2.0*x*y/(r2*sb*sb);
      } else {
      c2a = 1.0;
      s2a = 0.0;
      }

    rotor(Qrot_re,Qrot_im,Qcr_re,Qcr_im,c2a,s2a,1.0,0.0,cb,sb);
    a2 = f1th[2]*Qrot_re[2];
    b2 = f1th[2]*Qrot_im[2];
    a1 = f1th[1]*Qrot_re[1];
    b1 = f1th[1]*Qrot_im[1];
    for (it=1,qang[0]=0.0; it<pft; it++) 
      qang[it] = -b1*cfi[it]-b2*c2fi[it]+a1*sfi[it]+a2*s2fi[it];
        
    if (strncmp(pulse,"finit",5) == 0) {
      getrho(a2/2.0,b2/2.0,a1/2.0,b1/2.0,rho,mval,ntrans,TRUE,sitectr,param);
      }
    for (mctr=1; mctr<=ntrans; mctr++) {
    m0 = (6.0*mval[mctr]-3.0)/2.0;
    ampfac = r3fac*tran[mctr];

    fid[0] = 1.0;
    fid[1] = 0.0;
    for (it=1; it<pft; it++) 
      cossin(m0*qang[it],&fid[2*it],&fid[2*it+1]);

    if (strncmp(pulse,"finit",5) == 0) {
      if (mval[mctr]>0.5) {
/* do the -m to(-m+1) transition */
        for (i=0;i<pft;i++) {
          fid1[2*i] = fid[2*i];
          fid1[2*i+1] = -fid[2*i+1];
          }
        finitespec(ssbint[sitectr],rho[-mctr],rhofid,fid1,ampfac);
        }
/* and the m to (m-1) transition) */
      finitespec(ssbint[sitectr],rho[mctr],rhofid,fid,ampfac);
      } else if (strncmp(pulse,"short",5)==0) {
      if (mval[mctr]>0.5) {
        for (i=0;i<pft;i++) {
          fid1[2*i] = fid[2*i];
          fid1[2*i+1] = -fid[2*i+1];
          }
        idealspec(ssb[-mctr],fid1,ampfac);
        }
      idealspec(ssb[mctr],fid,ampfac);
      } else {
      if (mval[mctr]>0.5) {
        for (i=0;i<pft;i++) {
          fid1[2*i] = fid[2*i];
          fid1[2*i+1] = -fid[2*i+1];
          }
        idealspec(ssbint[sitectr],fid1,ampfac);
        } 
      idealspec(ssbint[sitectr],fid,ampfac); 
      }
    } /* end of mctr loop */ 
  }  /* end of iz loop */
 }  /* of iy-loop */ 
if (strncmp(pulse,"short",5) == 0) {
  for (i=1;i<=ntrans;i++) { 
    if (mval[i] >0.5) {
      for (j=0;j<2*pft;j ++) {
        shp[j] = shm[j] = 0.0; }
      shortpulse(shp,shm,mval[i],sitectr,param);
      for (j=0;j<pft;j++) {
        ssbint[sitectr][2*j+1] -= shp[2*j]*ssb[i][2*j];
        ssbint[sitectr][2*j] += shp[2*j+1]*ssb[i][2*j];
        ssbint[sitectr][2*j+1] -= shm[2*j]*ssb[-i][2*j];
        ssbint[sitectr][2*j] += shm[2*j+1]*ssb[-i][2*j];
        }
      } else {
      for (j=0;j<=2*pft;j++) shp[j] =  0.0; 
      shortpulse_c(shp,sitectr,param);
      for (j=0;j<pft;j++) {
        ssbint[sitectr][2*j+1] -= shp[2*j]*ssb[i][2*j];
        ssbint[sitectr][2*j] += shp[2*j+1]*ssb[i][2*j];
        }
      }
    }
  }
  } /* end of sitectr-loop*/
pulserelease(ndim,ntrans);

envelope_1(param);
if ((npar > 0) || rmsflag) 
  rms = rms_1(param); /* calculate rms error */
  else freqtofid_1(param);
fidrelease();
free_vector(fid1,0,2*pft);
return rms;
}

void finitespec(sptr,rhoptr,rhofid,fid,ampfac)
float *sptr,*rhoptr,*rhofid,*fid,ampfac;
{
int i;
for (i=0;i<pft; i++) { /* rho(gamma)*F(t) */
  rhofid[2*i] = (rhoptr[2*i]*fid[2*i]-rhoptr[2*i+1]*fid[2*i+1]);
  rhofid[2*i+1] = (rhoptr[2*i]*fid[2*i+1]+rhoptr[2*i+1]*fid[2*i]);
  }
four1(rhofid-1,pft,-1);  /* FT */
four1(fid-1,pft,-1);
for (i=0;i<pft; i++) {
  sptr[2*i+1] -= (rhofid[2*i]*fid[2*i]+rhofid[2*i+1]*fid[2*i+1])*ampfac;
  sptr[2*i] += (rhofid[2*i]*fid[2*i+1]-rhofid[2*i+1]*fid[2*i])*ampfac;
  }
}

void idealspec(sptr,f,ampfac)
float *sptr,*f,ampfac;
{
int i;
four1(f-1,pft,-1);  
for (i=0;i<pft; i++) {  /* "ideal" spectrum */
  sptr[2*i] += (f[2*i]*f[2*i]+f[2*i+1]*f[2*i+1])*ampfac;
  sptr[2*i+1] = 0.0;
  }
}
