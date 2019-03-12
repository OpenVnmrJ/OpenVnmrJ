// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* HETERO2DJ.c - heteronuclear J-resolved experiment;
               absolute value mode is required.

   Parameters:

         pw = 90 degree xmtr pulse
         nt = multiple of 2  (minimum)
	      multiple of 8  (recommended)



   revised          S. Farmer Aug 1988 
   VnmrJ compatible B. Heise  Feb 2007
Chempack compatible B. Heise  Jun 2009
Echo family - NM (April 19, 2007)
*/

#include <standard.h>
#include <chempack.h>

/* PHASECYCLE CALCULATION */

static
int ph1[4] = {0,2,1,3},
    ph2[8] = {0,0,1,1,2,2,3,3};

void pulsesequence()
{
   double  hsglvl = getval("hsglvl"),
             hsgt = getval("hsgt"),
           satpwr = getval("satpwr"),
           satdly = getval("satdly"),
           tpwr180 = getval("tpwr180"),
           pw180 = getval("pw180"),
           pp = getval("pp"),
           pplvl = getval("pplvl");
   int     prgcycle = (int)(getval("prgcycle")+0.5);
   char    sspul[MAXSTR],satmode[MAXSTR],wet[MAXSTR],pw180ad[MAXSTR];

   getstr("satmode",satmode);
   getstr("sspul", sspul);
   getstr("wet",wet);
   getstr("pw180ad",pw180ad);

  assign(ct,v17);

 
   settable(t1,4,ph1);     getelem(t1,v17,v1);
   settable(t2,8,ph2);     getelem(t2,v17,v2);
   assign(v1,oph);

/* BEGIN ACTUAL SEQUENCE */
   status(A);
      obspower(tpwr);
 
   delay(5.0e-5);
   if (getflag("sspul"))
	steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        if (getflag("slpsat"))
           {
                shaped_satpulse("relaxD",satdly,v4);
                if (getflag("prgflg"))
                   shaped_purge(v1,v4,v18,v19);
           }
        else
           {
                satpulse(satdly,v4,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,v4,v18,v19);
           }

     }
   else
	delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   status(B);
      rgpulse(pw, v1, rof1, rof1);
      decpower(pplvl);
      obspower(tpwr180);
      delay(d2/2);
      simshaped_pulse(pw180ad,"",pw180,pp*2,v2,v2,rof1,2*rof1); 
      delay(d2/2);
      decpower(dpwr);
      obspower(tpwr);
   status(C);
}
