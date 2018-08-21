// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* tncosyps - homonuclear correlation   (phase-sensitive version)

 Parameters:

      p1 = 90 degree pulse on the observe nucleus
      pw = x degree pulse on the observe nucleus 
           x:  90 degrees for standard COSY experiment

  satdly = length of saturation period;
  sspul  = 'y' for trim(x)-trim(y) before d1

   phase =   0:  non-phase-sensitive experiment  (P-type peak selection)
           1,2:  phase-sensitive HYPERCOMPLEX experiment
             3:  phase-sensitive TPPI experiment
      nt = (min):  multiple of 4   (phase = 0)
                   multiple of 2   (phase = 1,2 or 3)
           (max):  multiple of 8   (phase = 0)
                   multiple 0f 4   (phase = 1,2 or 3)


 (all phase cycling taken from s.l. patt, to be submitted)

 s.l.patt   15 may  1985
 revised    24 february  1988  (s.f.)
             2 july      1992   (g.g)

 */


#include <standard.h>

void pulsesequence()
{
/* VARIABLE DECLARATION */
   char            sspul[MAXSTR];


/* INITIALIZE VARIABLES */
   getstr("sspul",sspul);
   if (p1 == 0.0)
      p1 = pw;


/* CHECK CONDITIONS */
   if ((rof1 < 9.9e-6) && (ix == 1))
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n");


/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      hlv(ct, v9);
      mod4(ct, v2);
      mod2(ct, v1);
   elsenz(ssctr);
      sub(ssval, ssctr, v12);	/* v12 = 0,...,ss-1 */
      hlv(v12, v9);
      mod4(v12, v2);
      mod2(v12, v1);
   endif(ssctr);


/* CALCULATE PHASECYCLE */
   hlv(v9, v8);
   add(v8, v9, v9);
   mod2(v9, v9);		/* 00111100 */
   dbl(v1, v1);
   add(v1, v9, v1);		/* 0202+00111100 */
   initval(4.0, v10);
   sub(v10, v2, v2);		/* 0321 */
   if ((phase1 == 1) || (phase1 == 2))
      assign(zero, v2);
   add(v2, v9, v2);		/* 0321+00111100 or 0+00111100 */
   mod4(v1, oph);
   if (phase1 == 2)
      incr(v1);
   if (phase1 == 3)
      add(v1, id2, v1);


/* BEGIN ACTUAL PULSE SEQUENCE */
   status(A);
     if (sspul[A]=='y')
      {
       obspower(tpwr-12);
       rgpulse(200*pw,zero,rof1,0.0);
       rgpulse(300*pw,one,0.0,rof1);
       obspower(tpwr);
      }
     if (satmode[A]=='y')
      {
       obspower(satpwr);
       rgpulse(satdly,zero,rof1,rof2);
       obspower(tpwr);
      }
     hsdelay(d1);
   status(B);
     rgpulse(p1, v1, rof1, 1.0e-6);
     if (satmode[B]=='y')
     {
       if (d2>0)
       {
        obspower(satpwr);
        rgpulse(d2-3.0*rof1-9.4e-6-4*pw/3.1414,zero,rof1,rof1);
        obspower(tpwr);
       }
     }
     else
       delay(d2 - rof1 - 1.0e-6 - 4*pw/3.1414);
     rgpulse(pw, v2, rof1, rof2);
   status(C);
}
