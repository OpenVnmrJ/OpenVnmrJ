// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* inept -
       insensitive nuclei enhanced by polarisation transfer
                refocussing may be used

ref:morris and freeman, jacs,1o1,760(1979)

New parameters:

    pp     proton 90 degree pulse using decoupler
    pplvl  power level for proton decoupler pulse
    j      x-h coupling constant in hz
    mult   multiplicity of x-h multiplet in x spectrum
    focus  y to refocusbefore acquisition
           n to get up/down multiplets
           focus is automatically used for decoupled spectra
    d2     refocussing delay
    d3     excitation transfer delay

    d3   may be entered directly, or, if j<>0.0 then d3 is
         calculated as 1/2j

    d2   may be entered directly, or, if j<>0.0 then d2 is
         calculated to be
                         1/2j for doublets
                         3/4j for triplets
                         1/3j for quartets or all

For the pt2dj experiment d2 is varied from 0 to ni/sw2, where
sw2 > max j(x-h).  Observe nucleus shift is along f2, j(x-h) is
along f1. j must be set to zero, d2 set to zero and d3 set manually.


Systems without linear amplifiers:

   If dhp='n', the dlp value determines decoupler amplitude for both
   pulses and acquisition.  If dhp=a value, this value is used during
   acquisition and full power is used for pulses.

Systems with linear amplifiers:

   dpwr determines decoupler amplitude during acquisition.  The
   decoupler amplitude during a pulse is 63.


If normal='y', a 90 degree pulse is applied before acquisition to give
normal multiplets in coupled spectra

If mult = zero a normal one pulse experiment is done. */



#include <standard.h>

static int phasecycle[4] = {0, 2, 1, 3};

pulsesequence()

{
   char         normal[MAXSTR],
                focus[MAXSTR];
   int          rxgate;
   double       pp,
		pplvl,
                j,
                mult;


   rxgate = (rof1 == 0.0);
   if (rxgate)
      rof1 = 1.0e-6;		/* phase switching time */

   j = getval("j");
   pp = getval("pp");
   mult = getval("mult");
   getstr("focus", focus);
   getstr("normal", normal);

   if (newdecamp)
   {
      if (rxgate)
         rof1 = 40.0e-6;
   }

/* if mult is zero, then do normal s2pul sequence */
   if (mult == 0.0)
   {
      status(A);
         hsdelay(d1);
         pulse(p1, zero);
      status(B);
         delay(d2);
      status(C);
      settable(t1,4,phasecycle);
      pulse(pw,t1);
      setreceiver(t1);
   }
   else
   {
      if (j != 0.0)
      {				/* calculation of delays */
	 d3 = 1.0 / (2.0 * j);
	 if (mult < 2.5)
	 {
	    d2 = 1.0 / (2.0 * j);
	 }
	 else if (mult < 3.5)
	 {
	    d2 = 3.0 / (4.0 * j);
	 }
	 else
	 {
	    d2 = 1.0 / (3.0 * j);
	 }
      }

/* setup phases */
      hlv(oph, v1);		/* 0011 */
      dbl(v1, v1);		/* 0022 */
      mod2(oph, v2);		/* 0101 */

/* do equilibration delay */
      if ((dm[A] == 'y') || (dm[B] == 'y'))
      {
	 (void) printf("Decoupler must be set as dm=nny or n\n");
	 psg_abort(1);
      }
      else
      {
         if (newdecamp)
         {
            pplvl  = getval("pplvl");
            decpower(pplvl);	/* sets DEC atten = pplvl */
         }
         else
         {
	    declvlon();		/* sets dhp = 255 level */
         }
      }

      status(A);
         if (rxgate)
	    rcvroff();
         delay(d1);

/* excitation transfer */
      status(B);
         decrgpulse(pp, v1, rof1, rof1);
         delay(d3/2 - 2*rof1 - 3*pp/2);
         simpulse(2*pw, 2*pp, zero, zero, rof1, rof1);
         delay(d3/2 - 2*rof1 - 3*pp/2);
         simpulse(pw, pp, v2, one, rof1, rof2);

/* make decision on refocussing */
         if ((focus[A] == 'y') || (dm[C] == 'y'))
         {
	    delay(d2/2 - rof1 - pp);	/* refocussing delay varied for 2d */
            simpulse(2*pw, 2*pp, v2, zero, rof1, rof2);
	    delay(d2/2 - rof2 - pp);
         }
         else if (normal[A] == 'y')
         {
	    decrgpulse(pp, v1, 1.0e-6, 0.0);
         }

         if (newdecamp)
         {
            decpower(dpwr);	/* set DEC attten = dpwr */
         }
         else
         {
	    declvloff();
         }

      status(C);
         if (rxgate)
	    rcvron();
   }
}
