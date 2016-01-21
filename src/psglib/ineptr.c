// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* ineptr.c - a simple refocussed 1D INEPT pulse sequence 
   INEPT - insensitive nuclei enhanced by polarisation transfer
               
   Ref:morris and freeman, JACS, 1o1, 760(1979)

   New parameters:

    pp     proton 90 degree pulse using decoupler
    pplvl  power level for proton decoupler pulse
    j      x-h coupling constant in hz
    mult   multiplicity of x-h multiplet in x spectrum
    focus  y to refocus before acquisition
           n to get up/down multiplets
           focus is automatically used for decoupled spectra

    Eriks Kupce, Varian R&D, Oxford, 2009
*/


#include <standard.h>

static int phasecycle[4] = {0, 2, 1, 3};

pulsesequence()

{
   int          mult = (0.5 + getval("mult"));
   char         focus[MAXSTR];
   double       tau1, tau2,
                pp = getval("pp"),
		pplvl = getval("pplvl"),
                j = getval("j");
                

   getstr("focus", focus);

   if (dm[C] == 'y') focus[A]='y';
   if (j < 0.1) j = 140.0;  
   if (rof1 < 1.0e-6) rof1 = 1.0e-6;		
  
   tau1 = 0.25/j; 
   tau2 = tau1;

   if (mult == 2) tau2 = 0.125/j;
   if (mult == 3) tau2 = 0.1/j;

                                /* setup phases */
      hlv(oph, v1);		/* 0011 */
      dbl(v1, v1);		/* 0022 */
      mod2(oph, v2);		/* 0101 */

      if ((dm[A] == 'y') || (dm[B] == 'y'))
      {
	 (void) printf("Decoupler must be set as dm=nny or n\n");
	 psg_abort(1);
      }


      status(A);

         delay(d1);
         decpower(pplvl);

      status(B);               /* excitation transfer */

         decrgpulse(pp, v1, rof1, rof1);
         delay(tau1);
         simpulse(2*pw, 2*pp, zero, zero, rof1, rof1);
         delay(tau1);

         if (focus[A] == 'y')  /* refocussing */
         {
            simpulse(pw, pp, v2, one, rof1, rof1);
	    delay(tau2);	
            simpulse(2*pw, 2*pp, v2, zero, rof1, rof1);
	    delay(tau2 - pp - POWER_DELAY);
            decrgpulse(pp, v1, rof1, rof2);
         }
         else
           simpulse(pw, pp, v2, one, rof1, rof2);

         decpower(dpwr); 

      status(C);
}
