/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* SSnoesy -  2D cross relaxation experiment using S or SS "read" pulse
 
     Noesy for observation of exchangeable protons using a
     Symmetrically-Shifted read pulse.
     Ref. Stephen Smallcombe, JACS 115 4776 (June 2) 1993. 
     Modified for RnaPack by Peter Lukavsky (1999).
     Adapted from rna_SSnoesy.c for BioPack (GG 2001)

     Use lsfid='n' and obtain first increment spectrum. Phase for anti-phase
     around the water for an "S" pulse and in-phase around the water for "SS"
     Use lsfid='n' and obtain first increment spectrum. Phase for anti-phase
     around the water for an "S" pulse and in-phase around the water for "SS"
     Use linear prediction and lsfid to remove lp in multiples of 360 degrees.
     (e.g. calculate 1e6/sw1 to give dwell time in usec. Make lsfid equal
     to -(# 360 degree rotations of lp). Then set backward Linear Prediction
     to calculate this same number of points.)  
     
     Run first increment. If there is still a non-zero lp after phasing:
       For INOVA or UNITYplus:
       Use "calfa" to set alfa for lp=0 to obtain flat baseline.
     
       For VNMRS:
       Use setddrtc macro (button on "DigitalFilter" page) to get good value
       of ddrtc.

     S_at_12ppm.RF and SS_at_12ppm.RF are created by macro BPsetupshapes and
     are created whenever a probefile is updated for 1H. See shapefile for
     power and pulse width. These shapes would be suitable for RNA/DNA.
     S_at_7.5ppm.RF and SS_at_7.5ppm.RF would be suitable for proteins.
     Use Pbox for excitation at different positions.

     With steering pulses for improved water suppression - 
     Array pwa and pwd for best suppression.
     Positive and negative vaules of pwa and pwd can be used.
     Homospoil in mix can be either normal homospoil, hs='ynn' or
     gradient homospoil, hs='nnn', and gt1>0.
     hsdly hould be set to place homospoil in middle or near end of mix time.
     Axial peaks are displaced via FAD for single transient spectra


                  DETAILED INSTRUCTIONS FOR USE OF SSnoesy


    1. These Detailed Instructions for SSnoesy may be printed using:
                                      "printon man('SSnoesy') printoff".

    2. Optimization of H2O-suppression.
       After setting the power for the S-pulse, set mixing time. Then optimize
       pwa and pwd for best H2O-suppression.
       Start by setting pwd=0 and array pwa from -5 to +5.
       Choose value of pwa that gives the best H2O-suppression anf then optimize pwd
       in the same manner.

    4. Processing.
       Best phasing across the whole spectrum could be obtained using backward LP
       and no digital filtering (lsfid=-2, ssfilter='n') for INOVA or UNITYplus.
       Signals upfield of the water are opposite in sign due to the exitation
       profile of the an S-pulse (if used), same sign for SS-pulse.

*/
#include <standard.h>
pulsesequence()
{
   double mix,SSpwr,pwSS,hsdly,pwa,pwd,gt1,gzlvl1,gzlvl2;
   char ptable[MAXSTR],SSshape[MAXSTR];
   getstr("ptable",ptable); getstr("SSshape",SSshape);
   mix=getval("mix"); hsdly=getval("hsdly"); 
   loadtable(ptable);
   pwa=getval("pwa"); pwd=getval("pwd");
   gt1=getval("gt1"); gzlvl1=getval("gzlvl1"); gzlvl2=getval("gzlvl2");
   SSpwr=getval("SSpwr"); pwSS=getval("pwSS");
     initval(7.0,v2);   /* set up 45 degree phase shift for first pulse    */
     obsstepsize(45.0);
   getelem(t1,ct,v1);     /* 1st pulse phase    */
   getelem(t3,ct,v3);     /* read pulse phase   */
   getelem(t4,ct,v4);     /* receiver phase     */
   if (phase1 > 1.5) incr(v1);     /* hypercomplex phase shift */
   assign(v4,oph);
   /*HYPERCOMPLEX MODE USES REDFIELD TRICK TO MOVE AXIAL PEAKS TO EDGE */
    initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v6);
  if ((phase1==1)||(phase1==2)) {add(v1,v6,v1); add(v4,v6,oph);}

    assign(v3,v7);       /* for steering pulses */
    add(v3,one,v8); 
    if (pwa<0.0) { pwa = -pwa; add(v7,two,v7); }
    if (pwd<0.0) { pwd = -pwd; add(v8,two,v8); }

/* CHECK VALIDITY OF PARAMETER RANGE */

  if (dm[A] == 'y' || dm[B] == 'y')
   {
        printf(" dm must be 'nny' or 'nnn' ");
        psg_abort(1);
   }

  if ((dm[C] == 'y' || dm2[C] == 'y') && at > 0.085)
   {
        printf(" check at time! Don't fry probe \n");
        psg_abort(1);
   }

  if (dm2[A] == 'y' || dm2[B] == 'y')
   {
        printf(" dm2 must be 'nny' or 'nnn' ");
        psg_abort(1);
   }

  if (dpwr > 50)
   {
        printf(" dpwr must be less than 49 \n");
        psg_abort(1);
   }

  if (dpwr2 > 50)
   {
        printf(" dpwr2 must be less than 46 \n");
        psg_abort(1);
   }

/* BEGIN PULSE SEQUENCE */

   status(A);
     xmtrphase(v2);
     txphase(v1);
     obspower(tpwr);
     hsdelay(d1);
   status(B);
      rgpulse(pw, v1, rof1, 1.0e-6);
      xmtrphase(zero);
      txphase(t2);
      if (d2 > 0.001)
       {
     zgradpulse(gzlvl2,0.4*d2-SAPS_DELAY/2.0-2.0*GRADIENT_DELAY-(2.0*pw/PI));
     delay(0.1*d2-rof1);
     zgradpulse(-gzlvl2,0.4*d2-SAPS_DELAY/2.0-2.0*GRADIENT_DELAY-(2.0*pw/PI));
     delay(0.1*d2-rof1);
       }
       else delay(d2);
      rgpulse(pw, t2, rof1, 1.0e-6);
   status(C);
      obspower(SSpwr);
      delay(hsdly);
      zgradpulse(gzlvl1,gt1);
      hsdelay(mix-hsdly-gt1);
      shaped_pulse(SSshape,pwSS,t3,rof1,0.0);
      obspower(SSpwr-12.0);
      rgpulse(pwa,v7,2.0e-6,0.0);    /* steering pulses */
      rgpulse(pwd,v8,0.0e-6,0.0);
      delay(rof2);
   status(D);
}

