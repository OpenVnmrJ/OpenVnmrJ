/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* CNfilnoesy.c

       This pulse sequence will allow one to perform the following experiment:
       2D noesy of bound peptide with suppression of signals from 15N,13C
       labeled protein. The experiment is performed in water.

       To perform the D2O version set samplet='d' (vs samplet = 'w' for water)
                  and set dofaro = dof = 43 ppm. 

        Do not decouple during acquisition

        Written by L. E. Kay Oct 12, 1996 based on noesyf1cnf2cn_h2o_pfg_600.c

        Modified by L. E. Kay on Oct. 28, 1996 to change the shape of the flip-back
        pulse after the mixing time 
 
        Modified by GG (Varian) from noesyf1cnf2cn_h2o_pfg_v2.c (LEK, U.Toronto) Sept 2003

*/

#include <standard.h>

static double d2_init = 0.0;

static int phi1[2] = {1,3},
           phi2[2] = {0,2},
           phi3[8] = {0,0,1,1,2,2,3,3}, 
           phi4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           phi6[8]  = {2,2,3,3,0,0,1,1},
           rec[8] = {0,2,3,1,2,0,1,3};

void pulsesequence()
{
/* DECLARE VARIABLES */

 char        f1180[MAXSTR],satmode[MAXSTR],
             samplet[MAXSTR],shpsl1[MAXSTR];

 int	     phase,
             t1_counter;

 double     
             pwN,                   /* PW90 for N-nuc at pwNlvl     */
             pwC,                   /* PW90 for C-nuc at pwClvl      */
             satpwr,               /* low power level for presat*/
             pwNlvl,                /* power level for N hard pulses */
             pwClvl,                 /* power level for C hard pulses */
             jhc1,                  /* coupling #1 for HC (F1)       */
             jhc2,                  /* coupling #2 for HC (F1)       */
             jhc3,                  /* coupling #1 for HC (F2)       */
             jhc4,                  /* coupling #1 for HC (F2)       */
             taua,                  /* defined below  */ 
             taub,                   
             tauc,                   
             taud,                   
             taue,                   
             tauf,                   
	     tau1,	      	    /* t1 */
	     sw1,                  /* spectral width in 1H dimension */
             mix,                  /* Total Mixing time for noesy portion */
             dofaro,              /* C offset for aromatics  */

             tpwrsl,              /* power level for water selective pulses */
             pw_sl,               /* proton 90 at tpwrsl     */
             phase_sl,

             tpwrsl1,              /* power level for water selective eburp-1 pulses */
             pw_sl1,               /* proton 90 at tpwrsl1     */
             phasesl1,

             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gstab,
             gt1,
             gt2,
             gt3,
             gt4;

/* LOAD VARIABLES */

  pwC = getval("pwC");
  pwN = getval("pwN");
  satpwr = getval("satpwr");
  pwNlvl = getval("pwNlvl"); 
  pwClvl = getval("pwClvl");
  jhc1 = getval("jhc1");
  jhc2 = getval("jhc2");
  jhc3 = getval("jhc3");
  jhc4 = getval("jhc4");
  phase = (int) (getval("phase") + 0.5);
  sw1 = getval("sw1");
  mix  = getval("mix");
  dofaro = getval("dofaro");

  pw_sl = getval("pw_sl");
  tpwrsl = getval("tpwrsl");
  phase_sl = getval("phase_sl");

  pw_sl1 = getval("pw_sl1");
  tpwrsl1 = getval("tpwrsl1");
  phasesl1 = getval("phasesl1");

  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gstab = getval("gstab");
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");

  getstr("satmode",satmode); 
  getstr("f1180",f1180); 
  getstr("samplet",samplet);

  getstr("shpsl1",shpsl1);

/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
	{
	printf("incorrect Dec1 decoupler flags! No decoupling in the seq. ");
	psg_abort(1);
    } 

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
	{
	printf("incorrect Dec2 decoupler flags! No decoupling in the seq. ");
	psg_abort(1);
    } 


   if(samplet[A] != 'w' && samplet[A] != 'd')
       {
         printf("samplet must be set to either w (H2O) or d (D2O)");
         psg_abort(1);
       }

    if( satpwr > 8 )
    {
	printf("satpwr too large !!!  ");
	psg_abort(1);
    }

    if( dpwr > 50 )
    {
	printf("don't fry the probe, dpwr too large!  ");
	psg_abort(1);
    }

    if( dpwr2 > 50 )
    {
	printf("don't fry the probe, dpwr2 too large!  ");
	psg_abort(1);
    }

    if( gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 || gt4 > 15e-3 ) 
    {
        printf("gradients are on for too long !!! ");
        psg_abort(1);
    } 
 
   if(jhc1 > jhc2)
   {
       printf(" jhc1 must be less than jhc2\n");
       psg_abort(1);
   }

   if(jhc3 > jhc4)
   {
       printf(" jhc3 must be less than jhc4\n");
       psg_abort(1);
   }

   if(tpwrsl > 20) {
       printf("tpwrsl is too high; < 21\n");
       psg_abort(1);
   } 

   if(tpwrsl1 > 32) {
       printf("tpwrsl is too high; < 32\n");
       psg_abort(1);
   } 

/* LOAD VARIABLES */

  settable(t1, 2,  phi1);
  settable(t2, 2,  phi2);
  settable(t3, 8,  phi3);
  settable(t4,16,  phi4);
  settable(t6, 8,  phi6);
  settable(t5, 8,  rec);

/* INITIALIZE VARIABLES */

  taua = 1.0/(2.0*jhc1);
  taub = taua - 1.0/(2.0*jhc2);
  tauc = 5.5e-3 - taua - taub;
  taud = 1.0/(2.0*jhc3);
  taue = taud - 1.0/(2.0*jhc4);
  tauf = 5.5e-3 - taud - taue;  /* 5.5e-3 = 1/(2JHN)  */

/* Phase incrementation for hypercomplex data */

   if ( phase == 2 )     /* Hypercomplex in t1 */
        tssub(t1, 1, 4);

/* calculate modifications to phases based on current t1 values
   to achieve States-TPPI acquisition */

   if(ix==1)
     d2_init = d2;
     t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
     if(t1_counter %2) {
       tssub(t1,2,4);
       tssub(t5,2,4);
     }

/* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */

   tau1 = d2;
   if(f1180[A] == 'y')  tau1 += ( 1.0/(2.0*sw1) );
   if( tau1 < 0.2e-6) tau1 = 0.2e-6; 

/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);
   decoffset(dof);
   obspower(satpwr);            /* Set power for presaturation  */
   decpower(pwClvl);              /* Set decoupler1 power to dpwr */
   dec2power(pwNlvl);            /* Set decoupler2 power to pwNlvl */


/* Presaturation Period */


 if(satmode[0] == 'y')
{
  txphase(zero);
  rgpulse(d1,zero,0.0,0.0);           /* presat */
  obspower(tpwr);                /* Set power for hard pulses  */
}

else  {
 obspower(tpwr);                /* Set power for hard pulses  */
 delay(d1);
}


   rcvroff();
   delay(20.0e-6);
status(B);
  if(samplet[A] == 'w') {

     initval(1.0,v2);
     obsstepsize(135.0);
     xmtrphase(v2);
 
  }


  rgpulse(pw,t1,1.0e-6,0.0);        /* Proton excitation pulse */

  if(samplet[A] == 'w') 
     xmtrphase(zero);

  txphase(zero);
  dec2phase(t2);

  if(samplet[A] == 'w') 
     delay(taua - pwC - SAPS_DELAY);
  else
     delay(taua - pwC);

  decrgpulse(pwC,zero,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl1,gt1);

  delay(taub - gt1 - 2.0e-6);

  sim3pulse(2*pw,0.0,2*pwN,zero,zero,t2,0.0,0.0);
  dec2phase(zero);

  delay(tauc - pwN);

  dec2rgpulse(pwN,zero,0.0,0.0);
  
  delay(2.0e-6);
  zgradpulse(gzlvl1,gt1);

  delay(taua - tauc - gt1 - 2.0e-6 - pwC);

  decrgpulse(pwC,zero,0.0,0.0);

  delay(taub);

  delay(tau1);

  rgpulse(pw,zero,0.0,0.0);

  delay(mix - 10.0e-3);

  delay(2.0e-6);
  zgradpulse(gzlvl2,gt2);
  delay(gstab);

  decrgpulse(pwC,zero,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl3,gt3);

  txphase(t3);
  
  delay(10.0e-3 - gt2 - gt3 - gstab -4.0e-6);

  decoffset(dofaro);

  txphase(t6);
  obspower(tpwr); 

  if(samplet[A] == 'w') 
  {
     obspower(tpwrsl1); 
     initval(1.0,v4);
     obsstepsize(phasesl1);
     xmtrphase(v4);
     shaped_pulse(shpsl1,pw_sl1,t6,4.0e-6,0.0);
     obspower(tpwr);
     xmtrphase(zero); 
  }
  rgpulse(pw,t3,4.0e-6,0.0);
  txphase(zero);
  if(samplet[A] == 'w') 
  {
     initval(1.0,v3);
     obsstepsize(phase_sl);
     xmtrphase(v3);
     delay(2.0e-6);
     zgradpulse(gzlvl4,gt4);

     delay(taud - gt4 - 2.0e-6 
       - (POWER_DELAY + 4.0e-6 + pw_sl - taue));

     obspower(tpwrsl);
     rgpulse(pw_sl - taue - pwC,two,4.0e-6,0.0);
     simpulse(pwC,pwC,two,zero,0.0,0.0);
     rgpulse(taue,two,0.0,0.0);

     xmtrphase(zero);
     obspower(tpwr);

     sim3pulse(2.0*pw,0.0e-6,2.0*pwN,zero,zero,t4,4.0e-6,4.0e-6);

     initval(1.0,v3);
     obsstepsize(phase_sl);
     xmtrphase(v3);

     obspower(tpwrsl);

     if(tauf - pw_sl - 4.0e-6 - POWER_DELAY - pwN > 0.2e-6) 
     {
       rgpulse(pw_sl,two,4.0e-6,0.0);
       obspower(tpwr);
       dec2phase(zero);
       delay(tauf - pw_sl - 4.0e-6 - POWER_DELAY -  pwN);
       dec2rgpulse(pwN,zero,0.0,0.0);
       delay(2.0e-6);
       zgradpulse(gzlvl4,gt4);
       delay(taud - tauf - gt4 - 2.0e-6 - pwC);
       decrgpulse(pwC,zero,0.0,0.0);
     }
     else 
     {
      rgpulse(tauf - pwN,two,4.0e-6,0.0);
      sim3pulse(pwN,0.0e-6,pwN,two,zero,zero,0.0,0.0);
      rgpulse(pw_sl - tauf,two,0.0,0.0);

      delay(taud - tauf - (pw_sl - tauf) - 4.0e-6 - gt4  -gstab - pwC);

      zgradpulse(gzlvl4,gt4);
      delay(gstab);
      decrgpulse(pwC,zero,0.0,0.0);
     }
     xmtrphase(zero);
     delay(taue + 2.0/PI*pw - 2*POWER_DELAY);
   
  }
  else
  {
    delay(2.0e-6);
    zgradpulse(gzlvl4,gt4);
    delay(taud - gt4 - 2.0e-6 - pwC); 
    decrgpulse(pwC,zero,0.0,0.0);
    delay(taue - 4.0e-6);
    sim3pulse(2.0*pw,0.0e-6,2.0*pwN,zero,zero,t4,4.0e-6,4.0e-6);
    dec2phase(zero);
    delay(tauf - pwN - 4.0e-6); 
    dec2rgpulse(pwN,zero,0.0,0.0);
    delay(2.0e-6);
    zgradpulse(gzlvl4,gt4);
    delay(taud - tauf - gt4 - 2.0e-6 - pwC);
    decrgpulse(pwC,zero,0.0,0.0);
    delay(taue + 2.0/PI*pw - 2*POWER_DELAY);
  }
   dec2power(dpwr2);  
   decpower(dpwr);  
/* acquire data */

 status(C);
  setreceiver(t5);
}

