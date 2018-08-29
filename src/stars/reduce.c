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

#include <math.h>

extern FILE *resfile;
extern int sites,*csa,*etas,*cq,*etaq,*psi,*chi,*xi;
extern int quadflag,qcsaflag,csaflag;
extern float qcpar[];

#define SWAP(a,b) temp = (a); (a) = (b); (b) = temp


/* This routine reduces the interaction parameters obtained by an iterative
fit into a "reduced set, i.e. with 0 <= etas, etaq <= 1, 0<= psi <=180, 
0 <= chi <= 90, and 0 <= xi <= 90. Because of symmetry, the "effective" ranges 
for psi, chi, and xi are reduced from the normal : (0,360), (0,180),(0,360),
respectively. */ 


void reduce()
{
float cpsi,spsi,cchi,schi,cxi,sxi;
float cpsip,spsip,cchip,schip,cxip,sxip,x,temp;
int i;

for (i=1;i<=sites;i++) {

/* First convert the csa parameters */

if (csaflag | qcsaflag) {
  if (qcpar[etas[i]] < 0.0) {  /* corresponds to interchange of x and y axis */
    qcpar[etas[i]] = -qcpar[etas[i]];
    if (qcsaflag) qcpar[psi[i]] += 90.0;
    }


  if (qcpar[etas[i]] > 1.0) {  /* require redefinition of x, y, and z axis */
    if (qcsaflag){
      cchi = cos(pi/180.0*qcpar[chi[i]]);
      schi = sin(pi/180.0*qcpar[chi[i]]);
      cpsi = cos(pi/180.0*qcpar[psi[i]]);
      spsi = sin(pi/180.0*qcpar[psi[i]]);
      cxi = cos(pi/180.0*qcpar[xi[i]]);
      sxi = sin(pi/180.0*qcpar[xi[i]]);
      cchip = -schi*cpsi;
      schip = sqrt(1.0-cchip*cchip);
      qcpar[chi[i]] = acos(cchip)*180.0/pi;
      if (fabs(cchip) < 1.0) {
        cpsip = cchi/schip;
        spsip = schi*spsi/schip;
        cxip = (cxi*cchi*cpsi-sxi*spsi)/schip;
        sxip = (sxi*cchi*cpsi+cxi*spsi)/schip;
        qcpar[psi[i]] = acos(cpsip)*180.0/pi;
        if (spsip<0) qcpar[psi[i]] = -qcpar[psi[i]];
        qcpar[xi[i]] = acos(cxip)*180.0/pi;
        if (sxip<0) qcpar[xi[i]] = -qcpar[xi[i]];
        } else {
        qcpar[psi[i]] = 0.0;
        /* xi is unchanged */
        }      
      if (qcpar[etas[i]] > 3.0) qcpar[psi[i]] += 90.0;
      }
    x = qcpar[etas[i]];
    qcpar[etas[i]] = (3.0-x)/(1.0+x);
    if (x>3.0) qcpar[etas[i]] = -qcpar[etas[i]];
    qcpar[csa[i]] = -0.5*qcpar[csa[i]]*(1.0+x);
    }
  }

/* Now check the quadrupole parameters */

if (quadflag || qcsaflag) {
  if (qcpar[etaq[i]]<0.0) {
    qcpar[etaq[i]] = -qcpar[etaq[i]];
    if (qcsaflag) qcpar[xi[i]] += 90.0;
    }
  
  if (qcpar[etaq[i]]>1.0) {
    if (qcsaflag) {
      if (qcpar[etaq[i]] > 3.0) qcpar[xi[i]] += 90.0;
      cchi = cos(pi/180.0*qcpar[chi[i]]);
      schi = sin(pi/180.0*qcpar[chi[i]]);
      cpsi = cos(pi/180.0*qcpar[psi[i]]);
      spsi = sin(pi/180.0*qcpar[psi[i]]);
      cxi = cos(pi/180.0*qcpar[xi[i]]);
      sxi = sin(pi/180.0*qcpar[xi[i]]);
      cchip = -cxi*schi;
      schip = sqrt(1.0-cchip*cchip);
      qcpar[chi[i]] = acos(cchip)*180.0/pi;
      if (fabs(cchip)<1.0) { 
        cxip = cchi/schip;
        sxip = sxi*schi/schip;
        cpsip = (cxi*cchi*cpsi-schi*spsi)/schip;
        spsip = (cxi*cchi*spsi+sxi*cpsi)/schip;
        qcpar[psi[i]] = acos(cpsip)*180.0/pi;
        if (spsip<0) qcpar[psi[i]] = -qcpar[psi[i]];
        qcpar[xi[i]] = acos(cxip)*180.0/pi;
        if (sxip<0) qcpar[xi[i]] = -qcpar[xi[i]];
        } else {
        qcpar[xi[i]] = 0.0;
        /* psi unchanged*/
        }
      }
    x = qcpar[etaq[i]];
    qcpar[etaq[i]] = (3.0-x)/(1.0+x);
    if (x>3.0) qcpar[etaq[i]] = -qcpar[etaq[i]];
    qcpar[cq[i]] = -0.5*qcpar[cq[i]]*(1.0+x);
    }
  }

/* reduce angles */
if (qcsaflag) {
  while (qcpar[chi[i]] > 180.0) {
    qcpar[chi[i]] -= 180.0;
    qcpar[xi[i]] = -qcpar[xi[i]];
    }   
  while (qcpar[chi[i]] < 0.0) {
    qcpar[chi[i]] += 180.0;
    qcpar[xi[i]] = -qcpar[xi[i]];
    }
  if (qcpar[chi[i]]>90.0) {
    qcpar[chi[i]] = 180.0-qcpar[chi[i]];
    qcpar[psi[i]] = 180.0-qcpar[psi[i]];
    }
/* psi, xi is obtained only as multiples of 180 degrees*/
  while (qcpar[xi[i]] > 180.0) {
    qcpar[xi[i]] -= 180.0;
    }   
  while (qcpar[xi[i]] < 0.0) {
    qcpar[xi[i]] += 180.0;
    }
  if (qcpar[xi[i]] > 90.0) {
    qcpar[xi[i]] = 180.0-qcpar[xi[i]];
    qcpar[psi[i]] = 180.0-qcpar[psi[i]]; 
    }
  while (qcpar[psi[i]] > 180.0) {
    qcpar[psi[i]] -= 180.0;
    }   
  while (qcpar[psi[i]] < 0.0) {
    qcpar[psi[i]] += 180.0;
    }   
  }

  }
} 
