/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* PR42_gChsqcnoesyChsqc.c
This pulse sequences is used for PR4D methyl-NOE experiments.
Note that the HMQC-NOE-HMQC approach is superior for aliphatic NOEs.
For methyl NOEs, the narrow distribution of carbon chemical shifts makes it less
of an issue for multiple pulses.

Ref: (4,2)D Projection-Reconstruction Experiemnts for Protein Backbone
Assignment:  Application to Human Carbonic Anhydrase II and Calbindin
D28K.  Venters, R.A., Coggins, B.E., Kojetin, D., Cavanagh, J. and 
Zhou, P. JACS 127(24), 8785-8795 (2005)
 (modified from Ron Venters PR42_ChsqcNOEChsqc.c for BioPack, GG Varian)

To obtain reconstruction software package, please visit
http://zhoulab.biochem.duke.edu/software/pr-calc


Reference: Zwahlen et al, JACS, 120, 7617-7625 (1998).
 

Uses three channels:
    1)  1H             - carrier @ water  
    2) 13C             - carrier @ 35 ppm 
    3) 15N             - carrier @ 118 ppm
*/

#include <standard.h>
#include "bionmr.h"

static int  phi1[1]  = {0},
            phi2[2]  = {0,2},
            phi3[4]  = {0,0,2,2},
            phi4[8]  = {0,0,0,0,2,2,2,2},
            rec[8]   = {0,2,2,0,2,0,0,2};
                    
static double d2_init=0.0;

void pulsesequence()
{
/* DECLARE VARIABLES */

char satflg[MAXSTR];

int          t1_counter;

double    
   tau1, tau2, tau3,
   pw  = getval("pw"), 
   tpwr= getval("tpwr"),
   mix = getval("mix"),
   sw1 = getval("sw1"),
   jch = getval("jch"), 
   pwC = getval("pwC"),
   pwClvl = getval("pwClvl"),
   pwNlvl = getval("pwNlvl"),
   tauCH, 
   sw_hm1 = getval("sw_hm1"),
   sw_cm1 = getval("sw_cm1"),
   sw_cm2 = getval("sw_cm2"),
   pwHs = getval("pwHs"),
   swTilt, 
   angle_hm1 = getval("angle_hm1"),
   angle_cm1 = getval("angle_cm1"),
   angle_cm2 = getval("angle_cm2"),
   cos_hm1, cos_cm1, cos_cm2,
   satdly= getval("satdly"),
   gstab = getval("gstab"),
   gt0 = getval("gt0"),    gzlvl0 = getval("gzlvl0"),
   gt1 = getval("gt1"),    gzlvl1 = getval("gzlvl1"),
   gt2 = getval("gt2"),    gzlvl2 = getval("gzlvl2"),
   gt3 = getval("gt3"),    gzlvl3 = getval("gzlvl3"),
   gt4 = getval("gt4"),    gzlvl4 = getval("gzlvl4"),
   gt5 = getval("gt5"),    gzlvl5 = getval("gzlvl5"),
   gt6 = getval("gt6"),    gzlvl6 = getval("gzlvl6"),
   gt7 = getval("gt7"),    gzlvl7 = getval("gzlvl7"),
   gt8 = getval("gt8"),    gzlvl8 = getval("gzlvl8"),
   gt9 = getval("gt9"),    gzlvl9 = getval("gzlvl9"),
   gt10 = getval("gt10"),  gzlvl10 = getval("gzlvl10");

   cos_cm2=0.0; 
   getstr("satflg", satflg);

/* LOAD PHASE TABLE */

   tauCH = 1.0/4.0/jch;

   settable(t1,1,phi1);    settable(t2,2,phi2);
   settable(t3,4,phi3);    settable(t4,8,phi4);
   settable(t5,8,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */

   if (dpwr  > 49)  {printf("DPWR too large!" ); psg_abort(1); }
   if (dpwr2 > 49)  {printf("DPWR2 too large!"); psg_abort(1); }

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

   /* Note the reconstruction software assumes the indirectly determined dimension, here cm2 */
   /* always have the phase change first */



   if (phase1 == 1) {; }                                                             /* CC */
   else if (phase1 == 2) { tsadd (t1, 1, 4); }                                       /* SC */
   else if (phase1 == 3) { tsadd (t2, 1, 4); }                                       /* CS */
   else if (phase1 == 4) { tsadd (t1, 1, 4);  tsadd(t2, 1, 4); }                     /* SS */

   if (phase2 ==1) {;} else { tsadd (t3, 1, 4); }


   if (t1_counter %2) { tsadd(t3, 2, 4); tsadd(t5, 2, 4); }

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

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   delay(d1);
   rcvroff(); 

   obsoffset(satfrq);   obspower(tpwr);       obspwrf(4095.0);   txphase(zero);
   decoffset(dof);      decpower(pwClvl);     decpwrf(4095.0);   decphase(zero);
   dec2offset(dof2);    dec2power(pwNlvl);    dec2pwrf(4095.0);  dec2phase(zero);

   /* Crush water and steady state carbon magnetization */

   if (satflg[A] == 'y')
   {
      obspower(satpwr);
      rgpulse(satdly, zero, 20.0e-6, 2.0e-6);
      obspower(tpwr);      
   }

   decrgpulse(pwC, zero, 2.0e-6, 2.0e-6);  /*destroy C13 magnetization*/
   zgradpulse(gzlvl0, gt0);
   delay(gstab);


   if (satflg[A] == 'y')
   {
      shiftedpulse("sinc", pwHs, 90.0, 0.0, zero, 2.0e-6, 2.0e-6);
   }
   decrgpulse(pwC, one, 2.0e-6, 2.0e-6);
   zgradpulse(0.7*gzlvl0, gt0);
   txphase(t1);         
   delay(gstab);

   obsoffset(tof);     obspower(tpwr);
 
status(B);

   rgpulse(pw, t1, 2.0e-6, 2.0e-6);                       /* 1H pulse excitation */

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
                                                                /* point a */
      zgradpulse(gzlvl1, gt1);                       /* 2.0*GRADIENT_DELAY */
      txphase(zero); decphase(zero);
      delay(tauCH - gt1 - 4.0e-6);

   simpulse(2.0*pw, 2.0*pwC, zero, zero, 2.0e-6, 2.0e-6);

      delay(tauCH -gt1 -gstab -4.0e-6);
      zgradpulse(gzlvl1, gt1);                        /* 2.0*GRADIENT_DELAY */
      txphase(one);
      delay(gstab);
                                                                /* point b */
   rgpulse(pw, one, 2.0e-6, 2.0e-6);

   /* ======================HzCz=================== */
      zgradpulse(gzlvl2, gt2);
      txphase(zero);    decphase(t2);
      delay(gstab);
   /* ======================HzCz=================== */

   decrgpulse(pwC, t2, 2.0e-6, 0.0);

      if ((tau2 - 2.0*pwC/PI -pw) > 0 )
      {
         delay(tau2 - 2.0*pwC/PI - pw);
         rgpulse (2.0*pw, zero, 0.0, 0.0);
         decphase(zero);
         delay(tau2 - 2.0*pwC/PI - pw);
      }
      else
      {
         delay(2.0*tau2);
         decphase(one);    delay(2.0e-6);
         simpulse(2.0*pw, 2.0*pwC, zero, one, 0.0, 0.0);
         decphase(zero);   delay(2.0e-6);
      }

   decrgpulse(pwC, zero, 0.0, 2.0e-6);

   /* ======================HzCz=================== */
      zgradpulse(gzlvl3, gt3);
      txphase(zero);
      delay(gstab);
   /* ======================HzCz=================== */

   rgpulse(pw, zero, 2.0e-6, 2.0e-6);

      zgradpulse(gzlvl4, gt4);
      delay(tauCH - gt4 - 4.0e-6);
      
   simpulse(2.0*pw, 2.0*pwC, zero, zero, 2.0e-6, 2.0e-6);
      
      delay(tauCH - gt4 - gstab -4.0e-6);
      zgradpulse(gzlvl4, gt4);
      txphase(one);
      delay(gstab);

   rgpulse(pw, one, 2.0e-6, 2.0e-6);

   /* H only, beginning of NOE transfer period */
  
      obsoffset(satfrq);

      decphase(zero);
      delay(mix - gt5 - gt6 - pwC -1.0e-3 -2.0*pwHs ); 

      txphase(zero);
      shiftedpulse("sinc", pwHs, 90.0, 0.0, zero, 2.0e-6, 2.0e-6);

      zgradpulse(gzlvl5, gt5);
      delay(gstab);

      txphase(one);
      decrgpulse(pwC,zero,2.0e-6, 2.0e-6); 
      shiftedpulse("sinc", pwHs, 90.0, 0.0, one, 2.0e-6, 2.0e-6);
 
      zgradpulse(gzlvl6, gt6);
      txphase(zero);
      delay(gstab);

   /* End of NOE transfer period */
  
   /* Second HSQC step begins here */

   rgpulse(pw,zero,2.0e-6,2.0e-6);
      
      zgradpulse(gzlvl7, gt7);      
      delay(tauCH - gt7 - 4.0e-6 );

   simpulse(2.0*pw, 2.0*pwC, zero, zero, 2.0e-6, 2.0e-6);

      delay(tauCH - gt7 - gstab -4.0e-6 );
      zgradpulse(gzlvl7, gt7);      
      txphase(one);
      delay(gstab);

   rgpulse(pw, one, 2.0e-6, 2.0e-6);

   /* ------------HzCz----------------- */
      zgradpulse(gzlvl8, gt8);
      txphase(zero);  decphase(t3);
      delay(gstab);
   /* ------------HzCz----------------- */

   decrgpulse(pwC, t3, 2.0e-6,0.0);   
      if ( tau3 -2.0*pwC/PI - pw > 0.0 ) 
      {
         delay(tau3 - 2.0*pwC/PI - pw);
         rgpulse(2.0*pw, zero, 0.0, 0.0);
         decphase(zero);
         delay(tau3 - 2.0*pwC/PI - pw);
      }
      else
      {
         delay(2.0*tau3);
         decphase(one);    delay(2.0e-6);
         simpulse(2*pw, 2*pwC, zero, one, 0.0, 0.0);  
         decphase(zero);   delay(2.0e-6);
      }
   decrgpulse(pwC, zero, 0.0, 2.0e-6);

   /* ----  HzCz ------------*/
       zgradpulse(gzlvl9, gt9);
       txphase(t4);
       delay(gstab);
   /* ----  HzCz ------------*/

   rgpulse(pw, t4, 2.0e-6, 2.0e-6);
     
      zgradpulse(gzlvl10, gt10);
      delay(tauCH - gt10 - 4.0e-6 );

   simpulse(2.0*pw, 2.0*pwC, t4, zero, 2.0e-6, 2.0e-6);

      delay(tauCH - gt10 - gstab -4.0e-6 );
      zgradpulse(gzlvl10, gt10);
      delay(gstab);

   rgpulse(pw, t4, 2.0e-6, rof2);                        /* flip-back pulse  */

   setreceiver(t5);
   decpower(dpwr);

status(D);
}
