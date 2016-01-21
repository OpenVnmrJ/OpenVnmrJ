// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* lrinept -long-range INEPT; refocussing may be used
   This sequence ONLY for Mercury systems with PFG 

New parameters:

    pp     proton 90 degree pulse using decoupler
    pplvl  power level for proton decoupler pulse
    spp	   selective 90 degree proton pulse length
    spplvl power level used for selective H1 pulses
    gt	   gradient pulse length
    gzlvl  gradient pulse strength
    j      x-h coupling constant in hz
    mult   multiplicity of x-h multiplet in x spectrum
    focus  y to refocus before acquisition
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


If normal='y', a 90 degree pulse is applied before acquisition to give
normal multiplets in coupled spectra

If mult = zero a normal one pulse experiment is done. */


#include <standard.h>

pulsesequence()

{
char	normal[MAXSTR],
        focus[MAXSTR];
double	spp, spplvl, pp, mult, j, d3;
double  gt, gzlvl;

/* GATHER AND INTIALIZE */
   getstr("focus", focus);
   getstr("normal", normal);
   spp = getval("spp");
   spplvl = getval("spplvl");
   pp   = getval("pp");
   mult = getval("mult");
   j    = getval("j");
   gt = getval("gt");
   gzlvl = getval("gzlvl");

     /* calculate phases */
     mod2(ct,v1);  /* v1 = 01010101 */
     dbl(v1,v1);  /* v1 = 0 2 */
     hlv(ct,v2);
     mod2(v2,v2);  /* v2 = 0 0 1 1 */
     add(v1,v2,oph);  /* oph = 0 2 1 3 */

/* if mult is zero, then do normal s2pul sequence */
    if (mult == 0.0)
    {
      status(A);
         hsdelay(d1);
         pulse(p1, zero);
      status(B);
         delay(d2);
      status(C);
         pulse(pw,oph);
   }

   else  /* this is the INEPT part of the sequence  */
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
      else
	 d3 = getval("d3");


/* do equilibration delay */
      if ((dm[A] == 'y') || (dm[B] == 'y'))
      {
	 (void) printf("Decoupler must be set as dm=nny or n\n");
	 psg_abort(1);
      }
      if (declvlonoff)
         declvlon();		/* use pplvl for pulse */
      else
         decpower(pplvl);
      dec_pw_ovr(FALSE);

      status(A);
      delay(d1);

/* excitation transfer */
      status(B);
         decrgpulse(pp, v1, rof1, rof1);
	 decpower(spplvl); 
	 dec_pw_ovr(TRUE);
         delay(d3/2 - 2.0*(gt + spp + 1.0e-4)); 
		/*nominal 1/(4j) corrected for H1 & gradient pulses */
	 zgradpulse(gzlvl,gt);  delay(1.0e-4);
         simpulse(2*pw, 2*spp, zero, zero, rof1, rof1); 
	 delay(1.0e-4); zgradpulse(gzlvl,2.0*gt); delay(1.0e-4);
	 simpulse(2.0*pw, 2.0*spp, zero, two, rof1, rof1); 
	 zgradpulse(gzlvl,gt); delay(1.0e-4);
	 decpower(pplvl); 
	 dec_pw_ovr(FALSE);
         delay(d3/2 - 2.0*(gt + spp + 1.0e-4)); 
         simpulse(pw, pp, v2, one, 0.0, 0.0);

/* make decision on refocussing */
         if ((focus[A] == 'y') || (dm[C] == 'y'))
         {
	   decpower(spplvl);
	   dec_pw_ovr(TRUE);
	    delay(d2/2 - rof1 -spp);	/* refocussing delay varied for 2d */
            simpulse(2*pw, 2*spp, v2, zero, rof1, rof1);
	    delay(d2/2 - rof1 - spp);
	 }
         else if (normal[A] == 'y')
         {
	    decrgpulse(pp, v1, 1.0e-6, 0.0);
         }

      if (declvlonoff)
         declvloff();
      else
         decpower(dpwr);
      status(C);
   }
}
