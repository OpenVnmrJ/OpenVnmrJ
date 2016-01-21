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

FILE *resfile;
extern float *spec;
extern float digres,sw,sp;


void tent(offset,fmin,fmid,fmax,ampr,ampi)
float offset,fmin,fmid,fmax,ampr,ampi;
{
int index;
float top,f0,gmin,gmid,gmax,f1,f2,ap,am,apd,amd,x,*sptr;
/*This procedure erects the "intensity" tent. Input to the procedure are the
3 frequencies fmin, fmid, fmax. The intensity is distributed between fmin and
fmax as described in the Alderman paper, JCP, 84, 3717 1986.
We also need to take care of tents partially outside the spectral range. Note, in this procedure the frequency axis goes from zero at the left end to sw at the right end. */

index = (int)((offset+fmin)/digres);
if (index<0) index = 0;
f0 =  -offset + index*digres;
sptr = &spec[2*index];

gmin = f0;
gmax = ((offset+fmax) > sw) ? (sw-offset) : fmax;
gmid = (gmax< fmid) ? gmax : fmid;

if (fabs(fmax-fmin)<1.0e-7) {
  *(sptr++) += ampr;
  *(sptr++) += ampi;
  return;}

top = 2.0/(fmax-fmin);
f1 = f0;
index = 1;
f2 = f0+index*digres;

if (gmin<fmid) {
  if (f2<=fmid) {
    ap = top/(fmid-fmin);
    apd = 0.5*ap*digres;
    if (f1<fmin) {
      x = 0.5*ap*(f2-fmin)*(f2-fmin);
      *(sptr++) += x*ampr;
      *(sptr++) += x*ampi;
      f1 = f2; f2 = f0+(++index)*digres;
      }
    for (;f2<=gmid;) {
      x = apd*(f2+f1-2.0*fmin);
      *(sptr++) += x*ampr;
      *(sptr++) += x*ampi;
      f1 = f2; f2 = f0+(++index)*digres;
      }
    if (gmid < fmid) return;
    x = 0.5*ap*(fmid-f1)*(fmid+f1-2.0*fmin);
    *(sptr) += x*ampr;
    *(sptr+1) += x*ampi;
    } else {
    if (f1<fmin) {
      x = 0.5*top*(fmid-fmin);
      *(sptr) += x*ampr;
      *(sptr+1) += x*ampi;
      } else {
      x = 0.5*top*(fmid-f1)*(fmid+f1-2.0*fmin)/(fmid-fmin);
      *(sptr) += x*ampr;
      *(sptr+1) += x*ampi;
      }
    }
  } /* end of gmin < fmid*/

if (gmax<=fmid) return;

if (f2<=fmax) {
  am = top/(fmid-fmax);
  amd = 0.5*am*digres;
  if (f1<=fmid) {
    x = 0.5*am*(f2+fmid-2.0*fmax)*(f2-fmid);
    *(sptr++) += x*ampr;
    *(sptr++) += x*ampi;
    f1 = f2; f2 = f0+(++index)*digres;
    }
  for (;f2<=gmax;) {
    x = amd*(f2+f1-2.0*fmax);
    *(sptr++) += x*ampr;
    *(sptr++) += x*ampi;
    f1 = f2; f2 = f0+(++index)*digres;
    }
  if (gmax < fmax) return;
  x = -0.5*am*(fmax-f1)*(fmax-f1);
  *(sptr++) += x*ampr;
  *(sptr++) += x*ampi;
  } else {
  if (f1<=fmid) {
    x = 0.5*(fmax-fmid)*top;
    *(sptr++) += x*ampr;
    *(sptr++) += x*ampi;
    } else {
    x = -0.5*top*(fmax-f1)*(fmax-f1)/(fmid-fmax);
    *(sptr++) += x*ampr;
    *(sptr++) += x*ampi;
    }
  }
}
