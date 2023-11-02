/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* rna_HPcosyHCP - heteronuclear COSY with proton observe 


   H1  -      BB  -               - 90 - Acq.
   X          d1  - 90 -    t1    - 90

       EBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEB
       EB                                                                EB
       EB For qualitative measurement of torsion angles beta and epsilon EB
       EB Use with HCP for analysis.                                     EB
       EB                                                                EB
       EBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEB

   pwPlvl - X pulse power level
   pwP    - X 90 deg pulse
   hdpwr  - proton BB decoupling (during d1 using obs channel) power 
   hdres  - tip-angle resolution for the hdshape
   hd90   - 90 pulse width at hdpwr for BB decoupling
   hdshape - decoupling modulation shape
   hdflg - 'y' for decoupling
           'n' no decoupling
   phase - 1,2 (only hypercomplex with FAD is supported)
   sspul - HS-pw90-HS before d1


   Modified for RnaPack by Peter Lukavsky, June 2002, Stanford
   Use with D2O-sample!!
*/

#include <standard.h>

void pulsesequence()
{
  double   pwPlvl,
           pwP,
           hdpwr,
           hdres,
           hd90,
           phase;

  char     hdshape[MAXSTR],
           sspul[MAXSTR],
           hdflg[MAXSTR];

  int      iphase;


  pwPlvl = getval("pwPlvl");
  pwP    = getval("pwP");
  hdpwr  = getval("hdpwr");
  hd90   = getval("hd90");
  hdres  = getval("hdres");
  phase  = getval("phase");

  getstr("hdshape",hdshape);
  getstr("hdflg",hdflg);
  getstr("sspul",sspul);

  iphase = (int)(phase + 0.5);

  hlv(ct,v1);                    /* v1 = 0 0 1 1 2 2 3 3 */
  mod4(ct,v1);

  mod2(ct,v2);                  /*  v2 = 0 1 0 1 0 1 0 1 */
  dbl(v2,v2);                   /*  v2 = 0 2 0 2 0 2 0 2 */


  add(v2,v1,v2);                /*  v2 = 0 2 1 3 2 0 3 1 */

  assign(v2,oph);

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

  if (iphase == 2)
   incr(v2);

  add(v2,v14,v2);
  add(oph,v14,oph);

  status(A);

   dec2power(pwPlvl);
   obspower(tpwr);

   if (sspul[0] == 'y')
    { hsdelay(hst);
      rgpulse(pw,v1,rof1,rof2);
      hsdelay(hst+.001);
    }

   if (hdflg[0] == 'y')
    { obspower(hdpwr);
      delay(0.5);
      rcvroff();
      obsprgon(hdshape,hd90,hdres);
      xmtron();
      delay(d1);
      xmtroff();
      obsprgoff();
      obspower(tpwr);
      delay(0.05);
      rcvron();
    }
   else
    delay(d1);
    rcvroff();
    dec2phase(v2);

  status(B);
    dec2rgpulse(pwP,v2,rof1,1e-6);
    if (d2 > 0.0)
     delay(d2 - 2e-6 - (4*pwP/3.14159265358979323846));
    else
     delay(d2);
    sim3pulse(pw,0.0,pwP,v1,zero,v1,1e-6,rof2);

  status(C);
    rcvron();
}
