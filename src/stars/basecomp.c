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


extern int cq,etaq,csa,etas,xi,psi,chi,xi;
extern float ival;


/* This procedure calculates the csa and quadrupolar interaction parameters in the
  quadrupolar principal axis system. The csa interactions is tranformed to this system
  using the Euler angles (psi,chi,xi) */

void basecomp(param,cbaser,cbasei,qbaser,qbasei)
float param[],cbaser[],cbasei[],qbaser[],qbasei[];
{
float c2psi,s2psi,cchi,schi;
float qro0,qro2,cro2,cro0,x,xiang;

/* principal components: */
qro0 = sqrt(6.0)*param[cq]/(4*ival*(2*ival-1.0));
qro2 = -qro0*param[etaq]/sqrt(6.0);
cro0 = sqrt(1.5)*param[csa];
cro2 = -0.5*param[etas]*param[csa];

qbaser[0] = qro0;
qbaser[1] = 0.0;
qbasei[1] = 0.0;
qbaser[2] = qro2;
qbasei[2] = 0.0;

c2psi = cos(2.0*param[psi]*pi/180.0);
s2psi = sin(2.0*param[psi]*pi/180.0);
cchi = cos(param[chi]*pi/180.0);
schi = sin(param[chi]*pi/180.0);
cbaser[0] = c2psi*sqrt(1.5)*schi*schi*cro2-0.5*(1.0-3.0*cchi*cchi)*cro0;
cbaser[1] = (-c2psi*cro2+sqrt(1.5)*cro0)*cchi*schi;
cbasei[1] = s2psi*schi*cro2;
cbaser[2] = 0.5*c2psi*(1.0+cchi*cchi)*cro2+0.25*sqrt(6.0)*schi*schi*cro0;
cbasei[2] = -s2psi*cchi*cro2;
xiang = param[xi]*pi/180.0;
x = cbaser[1];
cbaser[1] = x*cos(xiang)+cbasei[1]*sin(xiang);
cbasei[1] = -x*sin(xiang)+cbasei[1]*cos(xiang);
x = cbaser[2];
cbaser[2] = x*cos(2.0*xiang)+cbasei[2]*sin(2.0*xiang);
cbasei[2] = -x*sin(2.0*xiang)+cbasei[2]*cos(2.0*xiang);

}

