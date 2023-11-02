/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif
/* 
 */

/* tnnoesy -  2D cross relaxation experiment.  It can be performed in
            either phase-sensitive or absolute value mode.  Either
            TPPI or the hypercomplex method can be used to achieve
            F1 quadrature in a phase-sensitive presentation.  No
            attempt is made to suppress J-cross peaks in this pulse
            sequence.
                TRANSMITTER SOLVENT SATURATION ONLY
                ASSUMES ON-RESONANCE SOLVENT (tof is at solvent position)

   satmode  determines when the saturation happens. Satmode should be set
            analogously to dm, i.e. satmode='yyyn' or 'ynyn' etc.
               satmode='yyyn' recommended
    satdly = length of presaturation period (saturation may also occur in d2 and mix
              as determined by satmode)
     sspul = 'y': selects for Trim(x)-Trim(y) sequence at start of pulse sequence
        G. Gray Palo Alto  Sept. 1991
	P.A.Keifer 940916 - fixed t1 timing
        Peter Lukavsky- combine status C and D to C and wetpw instead of pwwet
                        for RnaPack, Sept 1999.

*/


#include <standard.h>

void pulsesequence()
{
   double          mix;
   char            sspul[MAXSTR];


/* LOAD VARIABLES */
   mix = getval("mix");
   getstr("sspul", sspul);

   if (phase1 == 3)
      initval( (double)d2_index, v14);
   else
      assign(zero, v14);


/* CHECK CONDITIONS */
   if ((rof1 < 9.9e-6) && (ix == 1))
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n");

   if (satpwr > 40)
        {
         printf("satpwr too large  - acquisition aborted.\n");
         psg_abort(1);
        }

/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      mod2(ct, v2);
      hlv(ct, v3);
   elsenz(ssctr);
      sub(ssval, ssctr, v12);	/* v12 = 0,...,ss-1 */
      mod2(v12, v2);
      hlv(v12, v3);
   endif(ssctr);


/* CALCULATE PHASECYCLE */
   dbl(v2, v2);
   hlv(v3, v10);
   hlv(v10, v10);
   if (phase1 == 0)
   {
      assign(v10, v9);
      hlv(v10, v10);
      mod2(v9, v9);
   }
   else
   {
      assign(zero, v9);
   }
   assign(v10,v1);
   hlv(v10, v10);
   mod2(v1, v1);
   dbl(v1, v1);
   add(v9, v2, v2);
   mod2(v10, v10);
   add(v1, v2, oph);
   add(v3, oph, oph);
   add(v10, oph, oph);
   add(v10, v1, v1);
   add(v10, v2, v2);
   add(v10, v3, v3);
   add(v10,v14,v5);
   if (phase1 == 2)
      { incr(v2); incr(v5); }
   if (phase1 == 3)
      add(v2, v14, v2);		/* TPPI phase increment */

/*HYPERCOMPLEX MODE USES REDFIELD TRICK TO MOVE AXIAL PEAKS TO EDGE */
   if ((phase1==1)||(phase1==2))
   {
      initval(2.0*(double)(d2_index%2),v6);
      add(v2,v6,v2); add(oph,v6,oph); add(v5,v6,v5);
   }  

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
      if (sspul[A] == 'y')
       {
        obspower(tpwr-12);
        rgpulse(200*pw, zero, rof1, 0.0e-6);
        rgpulse(200*pw, one, 0.0e-6, rof1);
        obspower(tpwr);
       }
      if (d1>hst) hsdelay(d1);
      if (satmode[A] == 'y')
      {
        obspower(satpwr);
        rgpulse(satdly,v5,rof1,rof1);
        obspower(tpwr); 
      }
   status(B);
      rgpulse(pw, v2, rof1, 1.0e-6);
      if (satmode[B] =='y')
       {  
        if (d2 > 0.0) 
         { 
          obspower(satpwr);
          rgpulse(d2 - (2*POWER_DELAY) - rof1 - 11.0e-6 - (4.0*pw/3.14159265358979323846),zero,5.0e-6,5.0e-6);
          obspower(tpwr);
         }
       }
      else
       {
        if (d2 > 0.0)
         delay(d2 - 1.0e-6 - rof1 - (4.0*pw/3.14159265358979323846));
       }
      rgpulse(pw, v1, rof1, 1.0e-6);
      if (satmode[B] == 'y')
       {
          hsdelay(hst);
          obspower(satpwr);
          rgpulse(mix-hst,zero,2.0e-6,rof1);
          obspower(tpwr); 
       }
      else
          hsdelay(mix);
      rgpulse(pw, v3, rof1, rof2);
   status(C);

/*  Phase cycle:   ...satdly(v5)...pw(v2)..d2..pw(v1)..mix...pw(v3)..at(oph)
    (for phase=1. for phase = 2 incr(v2) and incr(v5) )
        v2 =[02] for axial peaks
        v1 =[02]16 for axial peaks
        v3 =[0123]2  "4 step phase cycle selection"
       v10 =[01]8   for quad image
       oph = v1+v2+v3+v10

v5: 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
v2: 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 1 3 1 3 1 3 1 3 1 3 1 3 1 3 1 3
v1: 0 0 0 0 0 0 0 0 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 3 3 3 3 3 3 3 3
v3: 0 0 1 1 2 2 3 3 0 0 1 1 2 2 3 3 1 1 2 2 3 3 0 0 1 1 2 2 3 3 0 0
oph:0 2 1 3 2 0 3 1 2 0 3 1 0 2 1 3 1 3 2 0 3 1 0 2 3 1 0 2 1 3 2 0    */
}
