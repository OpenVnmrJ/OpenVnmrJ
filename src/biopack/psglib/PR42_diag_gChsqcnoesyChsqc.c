/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* PR42_diag_gChsqcnoesyChsqc.c

   for methyl_NOE_diagonal peaks
    (modified from Ron Venter's PR42_ChsqcNOEChsqc_DIAG.c, GG Varian)

Ref: (4,2)D Projection-Reconstruction Experiemnts for Protein Backbone
Assignment:  Application to Human Carbonic Anhydrase II and Calbindin
D28K.  Venters, R.A., Coggins, B.E., Kojetin, D., Cavanagh, J. and
Zhou, P. JACS 127(24), 8785-8795 (2005)

To obtain reconstruction software package, please visit
http://zhoulab.biochem.duke.edu/software/pr-calc

   aliphatic CH3 (J~128Hz):       lambda = 0.83ms;  tauCH = 1.95 ms

    f1coef='' for 1H
    f1coef='1 0 -1 0 0 -1 0 -1' for carbon

    phase controls:

    phase1=1,2 controls "cos and sin of 1H"
    phase2=1 ++ for cm1/cm2
           2 -- for cm1/cm2
           3 -+ for cm1/cm2
           4 +- for cm1/cm2
*/

#include <standard.h>
#include "bionmr.h"

/* Left-hand phase list for ZZ='y'; right-hand list for SE='y'; phi3 common */

static int   phi1[1]  = {0},
	     phi3[2]  = {0,2},
             phi9[8]  = {0,0,1,1,2,2,3,3},
	     phi10[1] = {0}, 
             rec[4]   = {0,2,2,0};

static double   d2_init=0.0;

pulsesequence()
{

char  satflg[MAXSTR], stCshape[MAXSTR];    /* sech/tanh pulses from shapelib */       
 
int   icosel,  t1_counter;

double 
   tau1, tau2, tau3, swTilt,
   cos_hm1, cos_cm1, cos_cm2,
   sw1       = getval("sw1"),
   sw_hm1    = getval("sw_hm1"),
   sw_cm1    = getval("sw_cm1"),
   sw_cm2    = getval("sw_cm2"),
   angle_hm1 = getval("angle_hm1"),
   angle_cm1 = getval("angle_cm1"),
   angle_cm2,

   pwHs = getval("pwHs"),
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst = 0.0,	            /* fine power for the stCshape pulse, initialised */
   pwNlvl = getval("pwNlvl"),	                      /* power for N15 pulses */
   pwN = getval("pwN"),               /* N15 90 degree pulse length at pwNlvl */

   lambda = getval("lambda"),	                 /* J delay optimized for CH3 */
   tauCH = 1/(4.0*getval("jch")),                         /* 1/4J J  delay */

   gstab = getval("gstab"),
   gt0 = getval("gt0"),          gzlvl0 = getval("gzlvl0"),				   
   gt1 = getval("gt1"),          gzlvl1 = getval("gzlvl1"),
                                 gzlvl2 = getval("gzlvl2"),
   gt3 = getval("gt3"),          gzlvl3 = getval("gzlvl3"),
   gt4 = getval("gt4"),          gzlvl4 = getval("gzlvl4"),
   gt5 = getval("gt5"),          gzlvl5 = getval("gzlvl5"),
   gt6 = getval("gt6"),          gzlvl6 = getval("gzlvl6");

   getstr("satflg",satflg);
   pwN=pwN*1.0; cos_cm2=0.0; angle_cm2=0.0;

/*   LOAD PHASE TABLE    */

   settable(t1,1,phi1);   settable(t3,2,phi3);  settable(t9,8,phi9);
   settable(t10,1,phi10); settable(t11,4,rec);

/*   INITIALIZE VARIABLES   */

   /* 30 ppm sech pulse */
   rf0 = 4095.0; 
   rfst = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));   
   rfst = (int) (rfst + 0.5);
   strcpy(stCshape, "stC30");

/* CHECK VALIDITY OF PARAMETER RANGES */

  if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' ))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec  decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( (dpwr > 52) && (dm[C]=='y'))
  { text_error("don't fry the probe, DPWR too large!  ");   	    psg_abort(1); }

/* Phases and delays related to PR-NMR */
   /* sw1 is used as symbolic index */
   if ( sw1 < 1000 ) { printf ("Please set sw1 to some value larger than 1000.\n"); psg_abort(1); }
   if (angle_hm1 < 0 || angle_cm1 < 0 || angle_hm1 > 90 || angle_cm1 > 90 )
   { printf("angles must be set between 0 and 90 degree.\n"); psg_abort(1); }

   cos_hm1 = cos (PI*angle_hm1/180);  cos_cm1 = cos (PI*angle_cm1/180);

   if ( (cos_hm1*cos_hm1 + cos_cm1*cos_cm1) > 1.0) { printf ("Impossible angle combinations.\n"); psg_abort(1); }
   else { cos_cm2 = sqrt(1 - (cos_hm1*cos_hm1 + cos_cm1*cos_cm1) );  angle_cm2 = acos(cos_cm2)*180/PI;  }

   if (ix == 1) d2_init = d2;
   t1_counter = (int)((d2-d2_init)*sw1 + 0.5);

   swTilt = sw_hm1*cos_hm1 + sw_cm1*cos_cm1 + sw_cm2*cos_cm2;

   if  (phase1 == 1) {; }                                             /* C+/- */
   else if  (phase1 == 2)  { tsadd (t1, 1, 4); }                      /* S+/- */

   icosel=1; 

   if ( (phase2 == 1) || (phase2 == 3) )  { tsadd(t10,2,4); icosel = 1; }
        else   {   icosel = -1;    }

   if (t1_counter % 2)  { tsadd(t1,2,4); tsadd(t11,2,4); }    /* PZ TPPI */

   tau1 = 1.0*t1_counter*cos_hm1/swTilt;
   tau2 = 1.0*t1_counter*cos_cm1/swTilt;
   tau3 = 1.0*t1_counter*cos_cm2/swTilt;

   tau1 = tau1/2.0;  tau2 = tau2/2.0;  tau3 = tau3/2.0;

   if (ix ==1 )
   {
      printf ("Current Spectral Width:\t\t%5.2f\n", swTilt);
      printf ("Angle_hm1: %5.2f \n", angle_hm1);
      printf ("Angle_cm1: %5.2f \n", angle_cm1);
      printf ("Angle_cm2: %5.2f \n", angle_cm2);
      printf ("\n\n\n\n\n");
   }

/* BEGIN PULSE SEQUENCE */

status(A);
   delay(d1);
   rcvroff();

   txphase(zero);   obspower(tpwr);
   decphase(zero);  decpower(pwClvl);   decpwrf(rf0);
   dec2phase(zero); dec2power(pwNlvl);
   decoffset(dof);

   obsoffset(satfrq);

   if (satflg[A] == 'y')
   {
      obspower(satpwr);
      rgpulse(satdly, zero, 0.0, 0.0);
      obspower(tpwr);      
   }

   if (satflg[A] == 'y')
   {
      shiftedpulse("sinc", pwHs, 90.0, 0.0, zero, 2.0e-6, 2.0e-6);
   }
   decrgpulse(pwC, zero, 2.0e-6, 2.0e-6);  /*destroy C13 magnetization*/
   zgradpulse(gzlvl0, gt0);
   delay(gstab);

   if (satflg[A] == 'y')
   {
      shiftedpulse("sinc", pwHs, 90.0, 0.0, one, 2.0e-6, 2.0e-6);
   }
   decrgpulse(pwC, one, 2.0e-6, 2.0e-6);
   zgradpulse(0.7*gzlvl0, gt0);
   txphase(t1);

   obsoffset(tof);
   delay(gstab);

status(B);

   rgpulse(pw, t1, 2.0e-6,2.0e-6);                 /* 1H pulse excitation */

      if (tau1 > pwC)
      {
         delay(tau1 - pwC);
         decrgpulse(2.0*pwC, zero, 0.0, 0.0);
         delay(tau1 - pwC);
      }
      else
      {
         delay(2.0*tau1);
      }

      zgradpulse(gzlvl3, gt3);
      delay(tauCH - gt3 - 4.0e-6);

   simpulse(2.0*pw, 2.0*pwC, zero, zero, 2.0e-6, 2.0e-6);

      delay(tauCH - gt3 - gstab -4.0e-6);
      zgradpulse(gzlvl3, gt3);
      txphase(one);  
      delay(gstab);

   rgpulse(pw, one, 2.0e-6, 2.0e-6);

      zgradpulse(gzlvl4, gt4);
      obsoffset(satfrq);
      decphase(t3);
      delay(gstab);

   decrgpulse(pwC, t3, 4.0e-6, 2.0e-6);
   /*==================  Carbon evolution ===============*/
      txphase(zero); decphase(zero);

      if ((phase2 ==1) || (phase2 ==2))   { delay(tau2 + tau3); }
      else  {  delay(tau3);  }

   rgpulse(2.0*pw, zero, 2.0e-6, 2.0e-6);

      if ((phase2 == 1) || (phase2 ==2))  { delay(tau2 + tau3); }
      else  {  delay(tau3);  }

      zgradpulse(-1.0*gzlvl1, gt1/2);
      decphase(t9);
      delay(gstab - 2.0*GRADIENT_DELAY);

   decrgpulse(2.0*pwC, t9, 2.0e-6, 2.0e-6);

      if ((phase2 == 3) || (phase2 ==4))  { delay(tau2); }
   rgpulse(2.0*pw, zero, 2.0e-6, 2.0e-6);
      if ((phase2 == 3) || (phase2 ==4))  { delay(tau2); }

      zgradpulse(gzlvl1, gt1/2);
      decphase(t10);
      delay(gstab -2.0*GRADIENT_DELAY );

   /*================== End of  Carbon evolution ===============*/
   simpulse(pw, pwC, zero, t10, 2.0e-6, 2.0e-6);
      decphase(zero);
      zgradpulse(gzlvl5, gt5);
      delay(lambda - 1.5*pwC - gt5 - 4.0e-6);

   simpulse(2.0*pw, 2.0*pwC, zero, zero, 2.0e-6, 2.0e-6);

      delay(lambda  - 1.5*pwC - gt5 - gstab -4.0e-6);
      zgradpulse(gzlvl5, gt5);
      txphase(one); decphase(one);
      delay(gstab);

   simpulse(pw, pwC, one, one, 2.0e-6, 2.0e-6);

      txphase(zero);
      decphase(zero);
      zgradpulse(gzlvl6, gt6);
      delay(tauCH - 1.5*pwC - gt6 -4.0e-6);

   simpulse(2.0*pw, 2.0*pwC, zero, zero, 2.0e-6, 2.0e-6);

      delay(tauCH - pwC - 0.5*pw - gt6 -gstab -4.0e-6);
      zgradpulse(gzlvl6, gt6);
      delay(gstab);

   rgpulse(pw, zero, 2.0e-6, 2.0e-6);

      delay((gt1/4.0) + gstab -2.0e-6 - 0.5*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

   rgpulse(2.0*pw, zero, 2.0e-6, 2.0e-6);
      decpower(dpwr);					       /* POWER_DELAY */
      zgradpulse(icosel*gzlvl2, gt1/4.0);             /* 2.0*GRADIENT_DELAY */
      delay(gstab);

status(C);
   setreceiver(t11);
}		 
