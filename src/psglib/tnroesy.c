// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */


/* tnroesy- rotating frame NOE experiment
             OBSERVE TRANSMITTER SHOULD BE SET AT SOLVENT POSITION
             SATURATION,SPIN LOCK AND PULSES ALL USE OBS.XMTR

  Parameters:

        p1 = 90 degree pulse on protons (power level at "p1lvl")
        pw = small (30 degree) pulse on protons (active only if ratio > 0)
             if pw=0, pw is set to p1/3
     p1lvl = power level for the p1 pulse
      tpwr = power level for the spin lock pulse(s)
     ratio = tau/pw (uses cw lock if ratio is zero)
     phase = 1,2: F1 quadrature by the hypercomplex method
              (uses F1 axial peak displacement)
               3: F1 quadrature by the TPPI method
       mix = mixing time
     sspul = 'y': selects trim(x)-trim(y) sequence at start of pulse sequence
    rocomp = 'n': no resonance offset compensation
             'y': resonance offset compensation (recommended)
  satmode  = saturation mode. Use analogously to dm, i.e. satmode='nnn' or
             satmode='ynn' or  satmode='yyn' (recommended) 
   satdly  = length of saturation during relaxation delay  
   satpwr  = power level for solvent saturation
        nt = min:  multiple of 2
             max:  multiple of 8  (recommended)
   d2corr  = empirical correction(in us) of d2 (dependent on effective field of 
             spin lock, i.e. TPWR and/or RATIO). (Try 52 usec.)  
	     It can be determined from the lp1 and sw1 values from a 
	     properly phased spectrum by the relation

                    d2corr = (lp1*1e6)/(360*sw1)

             Note that the d2corr seems to be dependent on sw1. It is independent
             of sw1 since changes in sw1 result in corresponding changes in lp1
             so that their ratio is constant.

        Procedure for finding d2corr(so that lp1 will = 0, giving better
             baselines in F1):
                1. Run a tnroesy experiment with d2corr set either at 0 or at a
                   value found previously. (nt and ni can be smaller, and the
                   spectrum may be transformed early to do step 2)
                2. Phase the resulting spectrum in F1. Determine lp1 and 
                   calculate d2corr from the above relationship.
                3. Add this value to the value of d2corr used in step 1.
                4. Rerun the experiment and lp1 should be close to zero.
                5. Note this value for any future experiment with the same
                   value of tpwr and ratio.

   Use of any method to make lp1=0 will result in a dc offset of F1 slices. This
   should be removed by dc2d('f1') after the 2d transform. Enough noise should 
   be left on the edges (in F1) to permit this dc correction.

        Revised from roesy.c  G. Gray  Sept 1991  Palo Alto  
	P.A.Keifer 940916 - modified d2, rlpower
*/


#include <standard.h>

void pulsesequence()
{
   double          p1lvl,
                   mix,
                   ratio,d2corr,
                   cycles;
   int		   roc_flag;
   char            sspul[MAXSTR],
		   rocomp[MAXSTR];


/* LOAD AND INITIALIZE PARAMETERS */
   d2corr = getval("d2corr");
   mix = getval("mix");
   ratio = getval("ratio");
   p1lvl = getval("p1lvl");
   getstr("sspul", sspul);
   getstr("rocomp", rocomp);

   if (pw == 0.0)
      pw = p1 / 3.0;
   roc_flag = (rocomp[0] == 'y');


/* CHECK CONDITIONS */

   if ((rof1 < 9.9e-6) && (ix == 1))
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n");

   if (d2corr > 200e-6)
   {
       printf("d2corr seems very large. Recheck!./n");
       psg_abort(1);
   }

   if (satpwr > 40) 
   {
       printf("satpwr too large; acquisition aborted./n");
       psg_abort(1);
   }

/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      hlv(ct, v3);
      mod2(ct, v1);
      hlv(ct, v2);
   elsenz(ssctr);
      sub(ssval, ssctr, v7);	/* v7 = 0,...,ss-1 */
      hlv(v7, v3);
      mod2(v7, v1);
      hlv(v7, v2);
   endif(ssctr);


/* CALCULATE PHASES AND INITIALIZE LOOP COUNTER FOR MIXING TIME */
   mod2(v3, v3);		/* v3=00110011 */
   dbl(v1, v1);
   add(v1, v3, v1);
   incr(v1);
   assign(v1, oph);		/* v1=13201320 */
   if (phase1 == 2)
      incr(v1);			/* BC2D hypercomplex method */
   if (phase1 == 3)
      add(v1, id2, v1);         /* TPPI method */
   hlv(v2, v2);
   mod2(v2, v2);
   dbl(v2, v2);
   add(v3, v2, v2);		/* v2=00112233 */

  /* FOR HYPERCOMPLEX, USE REDFIED TRICK TO MOVE AXIALS TO EDGE */  
   if ((phase1==2)||(phase1==1))
   {
      initval(2.0*(double)(d2_index%2),v9); /* moves axials */
      add(v1,v9,v1); 
      add(oph,v9,oph);
   }

   cycles = mix / (16.0 * (ratio + 1.0) * pw);
   initval(cycles, v10);	/* mixing time cycles */


/* BEGIN ACTUAL PULSE SEQUENCE */
   status(A);
   obspower(p1lvl);
   if (sspul[A] == 'y')
   {
      rgpulse(1000.0*1e-6, zero, rof1, 0.0e-6);
      rgpulse(1000.0*1e-6, one, 0.0e-6, rof1);
   }
   hsdelay(d1);
   if (satmode[A] == 'y') 
   {
      obspower(satpwr);
      rgpulse(satdly,zero,rof1,rof2);
      obspower(p1lvl);
   }
   status(B);
   rgpulse(p1, v1, rof1, 1.0e-6);
   if (roc_flag)
   {
      if (satmode[B] == 'y')
      {
         obspower(satpwr);
         if (d2>0.0)
          rgpulse(d2 + d2corr - (2*POWER_DELAY) - 1.0e-6 -rof1 -(4*p1/3.14159265358979323846),
                  zero,0.0,0.0);
         obspower(p1lvl);
      }
      else
      {
         if (d2 >0.0)  delay(d2 + d2corr -1.0e-6 -rof1 -(4*p1/3.14159265358979323846));
      }
      rgpulse(p1,v2,rof1,0.0);
      obspower(tpwr);
      delay(ratio*pw/2);
   }
   else
   {
      if (satmode[B] == 'y')
      {
         obspower(satpwr);
         if (d2>0.0)
            rgpulse(d2 + d2corr -(2*POWER_DELAY) - 5.0e-6 -(2*p1/3.14159265358979323846),
                    zero,0.0,0.0);
         obspower(tpwr);
         delay(5.0e-6);
      }
      else
      {
         if (d2 >0.0) delay(d2 + d2corr -1.0e-6 -(2*p1/3.14159265358979323846));
      }
   }
   rcvroff();
   if (ratio > 0.00)
   {
      if (cycles > 1.5000)
      {
         starthardloop(v10);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
            rgpulse(pw, v2, 0.0, ratio * pw);
         endhardloop();
      }
/* The ROESY spin-lock unit is executed sixteen times within the
   hardware loop so that it is of sufficient duration to allow
   the acquisition hardware loop to be loaded in behind it on
   the last pass through the spin-lock loop. */
   }
   else
   {
      obspower(tpwr);
      rgpulse(mix,v2,0.0,0.0);          /* cw spin lock  */
   }
   obspower(p1lvl);
   if (roc_flag)
   {
      delay(ratio*pw/2);
      rgpulse(p1, v2, rof1, 0.0);
   }
   delay(rof2);
   rcvron();
   status(C);
}
