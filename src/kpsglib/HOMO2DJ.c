// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* Homo2dj.c - homonuclear J-resolved experiment;
               absolute value mode is required.

   Parameters:

         pw = 90 degree xmtr pulse
         nt = multiple of 2  (minimum)
	      multiple of 8  (recommended)


   Processing a J-resolved experiment requires that the command
   ROTATE(45.0) be executed.  The command FOLDJ may be optionally
   executed thereafter.

   This pulse sequence does not require fold-over correction.


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

pulsesequence()
{
   double  hsglvl = getval("hsglvl"),
             hsgt = getval("hsgt"),
           satpwr = getval("satpwr"),
           satdly = getval("satdly");
   char    sspul[MAXSTR],satmode[MAXSTR],wet[MAXSTR];

   getstr("satmode",satmode);
   getstr("sspul", sspul);
   getstr("wet",wet);

   settable(t1,4,ph1);     getelem(t1,ct,v1);
   settable(t2,8,ph2);     getelem(t2,ct,v2);
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
        satpulse(satdly,zero,rof1,rof1);
     }
   else
	delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   status(B);
      rgpulse(pw, v1, 2.0e-6,2.0e-6);
      delay(d2/2);
      rgpulse(2.0*pw, v2, 2.0e-6,rof2);
      delay(d2/2 + 2*pw/PI + 4.0e-6);
   status(C);
}
