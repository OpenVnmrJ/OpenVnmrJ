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

extern int *cq,*etaq,*csa,*etas,*psi,*chi,*xi;
extern float *param,ival,sfrqr;


void cbase(param,c_re,c_im,k)
float *param,*c_re,*c_im;
int k;
{
float r0,r2,c2a,s2a,cb,sb,x,gamang;
/* principal components for CSA*/
r0 = sqrt3h*param[csa[k]]*sfrqr;
r2 = -0.5*param[etas[k]]*param[csa[k]]*sfrqr;
/* transform to the molecular frame = quadrupole PAS */
c2a = cos(2.0*param[psi[k]]*pi/180.0);
s2a = sin(2.0*param[psi[k]]*pi/180.0);
cb = cos(param[chi[k]]*pi/180.0);
sb = sin(param[chi[k]]*pi/180.0);
c_re[0] = c2a*sqrt3h*sb*sb*r2-0.5*(1.0-3.0*cb*cb)*r0;
c_re[1] = (-c2a*r2+sqrt3h*r0)*cb*sb;
c_im[1] = s2a*sb*r2;
c_re[2] = 0.5*c2a*(1.0+cb*cb)*r2+0.25*sqrt6*sb*sb*r0;
c_im[2] = -s2a*cb*r2;
gamang = param[xi[k]]*pi/180.0;
x = c_re[1];
c_re[1] = x*cos(gamang)+c_im[1]*sin(gamang);
c_im[1] = -x*sin(gamang)+c_im[1]*cos(gamang);
x = c_re[2];
c_re[2] = x*cos(2.0*gamang)+c_im[2]*sin(2.0*gamang);
c_im[2] = -x*sin(2.0*gamang)+c_im[2]*cos(2.0*gamang);
}


void qbase(param,q_re,q_im,k)
float *param,*q_re,*q_im;
int k;
{
float r0,r2;

/* principal components for Quadrupole interaction*/
r0 = sqrt6*param[cq[k]]/(4.0*ival*(2.0*ival-1.0));
r2 = -r0*param[etaq[k]]/sqrt6;
q_re[0] = r0;
q_re[1] = 0.0;
q_im[1] = 0.0;
q_re[2] = r2;
q_im[2] = 0.0;
}


void rotor(r_re,r_im,p_re,p_im,c2a,s2a,ca,sa,cb,sb)
float *r_re,*r_im,*p_re,*p_im,c2a,s2a,ca,sa,cb,sb;
{
float p1,q1,p2,q2;
/* Transform interaction from molecular frame to rotor frame */
p1 = ca*p_re[1]+sa*p_im[1];
q1 = ca*p_im[1]-sa*p_re[1];
p2 = c2a*p_re[2]+s2a*p_im[2];
q2 = c2a*p_im[2]-s2a*p_re[2];
r_re[2] = 0.5*(1.0+cb*cb)*p2+cb*sb*p1+sqrt3h*0.5*sb*sb*p_re[0];
r_re[1] = -cb*sb*p2+(2.0*cb*cb-1.0)*p1+sqrt3h*cb*sb*p_re[0];
r_re[0] = sqrt3h*sb*(sb*p2-2.0*cb*p1)-0.5*(1.0-3.0*cb*cb)*p_re[0];
r_im[2] = cb*q2+sb*q1;
r_im[1] = -sb*q2+cb*q1;
r_im[0] = 0.0;
}


void lab(r_re,r_im,q_re,q_im)
float *r_re,*r_im,*q_re,*q_im;
{
/* Expressions to be used for 2'nd order quadrupole interaction.*/
r_re[1] = q_re[2]*q_re[2]-q_im[2]*q_im[2];
r_im[1] = 2.0*q_re[2]*q_im[2];
r_re[2] = q_re[1]*q_re[2]-q_im[1]*q_im[2];
r_im[2] = q_re[1]*q_im[2]+q_im[1]*q_re[2];
r_re[3] = q_re[2]*q_re[0];
r_im[3] = q_im[2]*q_re[0];
r_re[4] = q_re[1]*q_re[1]-q_im[1]*q_im[1];
r_im[4] = 2.0*q_re[1]*q_im[1];
r_re[5] = -q_re[2]*q_re[1]-q_im[2]*q_im[1];
r_im[5] = q_re[2]*q_im[1]-q_im[2]*q_re[1];
r_re[6] = q_re[1]*q_re[0];
r_im[6] = q_im[1]*q_re[0];
r_re[7] = q_re[2]*q_re[2]+q_im[2]*q_im[2];
r_re[8] = -q_re[1]*q_re[1]-q_im[1]*q_im[1];
r_re[9] = q_re[0]*q_re[0];
}

