/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  hacahbA.c  Autocalibrated version of original hacahb_cosy_500a.c

    This pulse sequence will allow one to perform the following
    experiment:

    3D C(alpha) - H(beta) - H(alpha) Correlation
 
    Uses three channels:
         1)  1H             - 4.73 ppm 
         2) 13C             - 46.0 ppm 
         3) 15N             - 119.0 ppm  

    Set dm = 'nny', dmm = 'ccp' [13C decoupling during acquisition].
    Set dm2 = 'nyn', dmm2 = 'cgc', or 'cpc'

    Must set phase = 1,2 for States-TPPI
    acquisition in t1 [13C,alpha].

    Must set phase2 = 1,2 for States-TPPI
    acquisition in t2 [1H,beta].
    
    The flag f2180 should be set to 'y' (-90,180) phase corrects  
    Set f1180 = n (0,0) phase corrects

    Grzesiek and Bax, JACS 1995, 117, 5312.

    Note: When you run the f1-f3 plane you will see a "counter-
          diagonal" in the methyl (f3) region at 3* the slope
          of the diagonal. This is due to triple quantum magnetization
          of the methyl (3* the proton chemical shift).

    Written by L. E. Kay, August 16, 1995 
    

      The autocal flag is generated automatically in Pbox_bio.h
       If this flag does not exist in the parameter set, it is automatically 
       set to 'y' - yes. In order to change the default value, create the  
       flag in your parameter set and change as required. 
       For the autocal flag the available options are: 'y' (yes - by default), 
       and 'n' (no - use full manual setup, equivalent to 
       the original code).
            E. Kupce, Varian
*/

#include <standard.h>
#include "Pbox_bio.h"

#define CODEC   "SEDUCE1 20p 130p"                  /* off resonance SEDUCE1 on C' at 176 ppm */
#define CODECps "-dres 2.0 -stepsize 5.0 -attn i"

static shape codec;

static int  phi1[4] = {0,0,2,2},
            phi2[1] = {0},
            phi3[4] = {0,1,2,3},
            phi4[8] = {0,0,0,0,2,2,2,2},
            rec[4]  = {0,2,2,0};
           
static double d2_init=0.0, d3_init=0.0;
            
void pulsesequence()
{
/* DECLARE VARIABLES */

 char       autocal[MAXSTR],  /* auto-calibration flag */
            fsat[MAXSTR],
            fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell         */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell         */
            codecseq[MAXSTR];

 int         phase, phase2, t2_counter, ni2,
             t1_counter;   /* used for states tppi in t2,t1        */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JCH =  1.7 ms */
             taub,         /*   5.3 ms */
             tauc,         /*   8.7 ms */
             pwc,          /* 90 c pulse at dhpwr            */
             tsatpwr,      /* low level 1H trans.power for presat  */
             dhpwr,        /* power level for high power 13C pulses on dec1 */
             sw1,          /* sweep width in f1                    */ 
             sw2,          /* sweep width in f2                    */ 
             pwcodec,      /* pw90 for C' decoupling */
             dressed,      /* = 2 for seduce-1 decoupling */
             dpwrsed,
             compC,        /* compression factor for C-13 amp. */

             tau2_1,
             tau2_2, 
             tau2_1_max,
             tau2_2_max, 
             taub_adj,
             tauc_adj,

             gstab,
             gt0,
             gt1,
             gt2,
             gt3,
             gt4,

             gzlvl0,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4;


/* LOAD VARIABLES */

  getstr("autocal",autocal);
  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("fscuba",fscuba);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  tauc   = getval("tauc"); 
  pwc = getval("pwc");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  dhpwr = getval("dhpwr");
  dpwr = getval("dpwr");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  ni2 = getval("ni2");

  gt0 = getval("gt0");
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");

  gstab = getval("gstab");
  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");

  if(autocal[0]=='n')
  {     
    getstr("codecseq",codecseq);
    pwcodec = getval("pwcodec");
    dressed = getval("dressed");
    dpwrsed = getval("dpwrsed");
  }
  else
  {    
    strcpy(codecseq,"Pseduce1_130p");
    if (FIRST_FID)
    {
      compC = getval("compC");
      codec = pbox(codecseq, CODEC, CODECps, dfrq, compC*pwc, dhpwr);
    }
    dpwrsed = codec.pwr;  
    pwcodec = 1.0/codec.dmf;  
    dressed = codec.dres;
  }   
   

/* LOAD PHASE TABLE */

  settable(t1,4,phi1);
  settable(t2,1,phi2);
  settable(t3,4,phi3);
  settable(t4,8,phi4);
  settable(t5,4,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */



    if((dm[A] == 'y' || dm[B] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y'|| dm2[C] == 'y'))
    {
        printf("incorrect dec2 decoupler flags!  ");
        psg_abort(1);
    }

    if( tsatpwr > 6 )
    {
        printf("TSATPWR too large !!!  ");
        psg_abort(1);
    }

    if( dpwr > 50 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > 50 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( dhpwr > 63 )
    {
        printf("don't fry the probe, DHPWR too large!  ");
        psg_abort(1);
    }

    if( pw > 200.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 

    if(dpwrsed > 49)
    {
        printf("dpwrco is too high; < 49\n");
        psg_abort(1);
    }

    if(gt0 > 15e-3 || gt1 > 15e-3 || gt2 > 15e-3 
        || gt3 > 15e-3 || gt4 > 15e-3) 
    {
        printf("gradients on for too long. Must be < 15e-3 \n");
        psg_abort(1);
    }


/*  Phase incrementation for hypercomplex 2D data */

    if (phase2 == 2)
      tsadd(t2,1,4);  

    if (phase == 2) 
      tsadd(t1,1,4);

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    tau1 = tau1 - (4.0/PI*pw + 2.0*pwc);

    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) );
    }
    if(tau1 < 0.4e-6) tau1 = 0.4e-6;
    tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */
   
    tau2 = d3;
    if(f2180[A] == 'y')
        tau2 += ( 1.0 / (2.0*sw2) );

    taub_adj = taub - gt3 - 252.0e-6 
         - POWER_DELAY - PRG_START_DELAY
         - POWER_DELAY - PRG_START_DELAY
         - PRG_STOP_DELAY - POWER_DELAY - 4.0e-6;

    tauc_adj = tauc - gt2 - 152.0e-6 
         - POWER_DELAY - PRG_START_DELAY
         - POWER_DELAY - PRG_START_DELAY
         - PRG_STOP_DELAY - POWER_DELAY
         - PRG_STOP_DELAY - gt1 - 152e-6 - 4.0e-6;

    tau2_2 = taub_adj/(taub_adj + tauc_adj)*0.5*tau2;
    tau2_1 = (tauc_adj/taub_adj)*tau2_2;

    tau2_2_max = taub_adj/(taub_adj + tauc_adj)*0.5*(ni2-1)/sw2;
    tau2_1_max = (tauc_adj/taub_adj)*tau2_2_max;

    if(tau2_2 < 0.4e-6) tau2_2 = 0.2e-6;
    if(tau2_1 < 0.4e-6) tau2_1 = 0.2e-6;

    if( taub - tau2_2_max - gt3 - 252e-6 
           - POWER_DELAY - PRG_START_DELAY 
           - POWER_DELAY - PRG_START_DELAY 
           - PRG_STOP_DELAY - POWER_DELAY - 2.0e-6 < 0.2e-6 )
    {
        printf(" ni2 is too big\n");
        printf("tau2_2_max is %f\n",tau2_2_max);
        psg_abort(1);
    }

    if( tauc -  tau2_1_max  - gt2 - 152e-6
             - POWER_DELAY - PRG_START_DELAY
             - POWER_DELAY - PRG_START_DELAY
             - PRG_STOP_DELAY - POWER_DELAY
             - PRG_STOP_DELAY - gt1 - 152e-6 - 2.0e-6 < 0.2e-6 )
     {
         printf("ni2 is too big\n");
         printf("tau2_1_max is %f\n",tau2_1_max);
         psg_abort(1);
     } 

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t1,2,4);     
      tsadd(t5,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t2,2,4);     
      tsadd(t5,2,4);    
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   decoffset(dof);
   obspower(tsatpwr);     /* Set transmitter power for 1H presaturation */
   decpower(dhpwr);       /* Set Dec1 power for hard 13C pulses         */
   dec2power(dpwr2);      /* Set Dec2 power for 15N decoupling       */

/* Presaturation Period */
   if (fsat[0] == 'y')
   {
        delay(2.0e-5);
        rgpulse(d1,zero,2.0e-6,2.0e-6); /* presat */
        obspower(tpwr);    /* Set transmitter power for hard 1H pulses */
        delay(2.0e-5);
        if(fscuba[0] == 'y')
        {
                delay(2.2e-2);
                rgpulse(pw,zero,2.0e-6,0.0);
                rgpulse(2*pw,one,2.0e-6,0.0);
                rgpulse(pw,zero,2.0e-6,0.0);
                delay(2.2e-2);
        }
   }
   else
   {
    delay(d1);
   }

   obspower(tpwr);          /* Set transmitter power for hard 1H pulses */
   txphase(zero);
   dec2phase(zero);
   decphase(zero);
   delay(1.0e-5);

/* Begin Pulses */

   rcvroff();
   delay(20.0e-6);

/* first ensure that magnetization does infact start on H and not C */

   decrgpulse(pwc,zero,2.0e-6,2.0e-6);

   delay(2.0e-6);
   zgradpulse(gzlvl0,gt0);
   delay(gstab);

   decphase(t2);

/* this is the real start */

   rgpulse(pw,t1,4.0e-6,0.0);             /* 90 deg 1H pulse */

   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(gstab);

   delay(taua - gt1 -gstab -2.0e-6 - pwc
         + 2.0*pwc + 2.0*pw + 2.0*PRG_STOP_DELAY
         + 2.0*POWER_DELAY + 2.0*PRG_STOP_DELAY);                     

   decrgpulse(pwc,t2,0.0,0.0);
   decphase(zero);

   delay(2.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(gstab);
status(B);

   /* C' decoupling on */
   decpower(dpwrsed);
   decprgon(codecseq,pwcodec,dressed);
   decon();
   /* C' decoupling on */

   delay(taub - 2.0*pw - gt2 - 152.0e-6 
         - POWER_DELAY - PRG_START_DELAY
         - PRG_STOP_DELAY - POWER_DELAY);

   rgpulse(2.0*pw,t1,0.0,0.0);
  
   delay(tau2_1);
   /* C' decoupling off */
   decoff();
   decprgoff();
   decpower(dhpwr);
   /* C' decoupling off */
status(A);
   decrgpulse(2.0*pwc,zero,2.0e-6,2.0e-6); 
   decphase(t3);

   delay(2.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(gstab);
status(B);

   /* C' decoupling on */
   decpower(dpwrsed);
   decprgon(codecseq,pwcodec,dressed);
   decon();
   /* C' decoupling on */

   delay(tauc - tau2_1 - gt2 - gstab -2.0e-6 
         - POWER_DELAY - PRG_START_DELAY
         - PRG_STOP_DELAY - POWER_DELAY
         - PRG_STOP_DELAY - gt1 - gstab -2.0e-6);
  
   /* C' decoupling off */
   decoff();
   decprgoff();
   decpower(dhpwr);
   /* C' decoupling off */
status(A);


   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(gstab);

   rgpulse(pw,t1,0.0,0.0); txphase(t4);

   delay(tau1);

   decrgpulse(2.0*pwc,t3,0.0,0.0); decphase(zero);

   delay(tau1);

   rgpulse(pw,t4,0.0,0.0); txphase(zero);

   delay(2.0e-6);
   zgradpulse(gzlvl3,gt3);
   delay(gstab +100.0e-6);
   
   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(gstab);
status(B);

   /* C' decoupling on */
   decpower(dpwrsed);
   decprgon(codecseq,pwcodec,dressed);
   decon();
   /* C' decoupling on */

   delay(tauc - 2.0*pw - gt3 - gstab -102.0e-6 
         - gt1 - gstab -2.0e-6
         - POWER_DELAY - PRG_START_DELAY
         - PRG_STOP_DELAY - POWER_DELAY);

   rgpulse(2.0*pw,zero,0.0,0.0);

   delay(tau2_2);
  
   /* C' decoupling off */
   decoff();
   decprgoff();
   decpower(dhpwr);
   /* C' decoupling off */
status(A);
   decrgpulse(2.0*pwc,zero,2.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl3,gt3);
   delay(gstab +100.0e-6);
status(B);

   /* C' decoupling on */
   decpower(dpwrsed);
   decprgon(codecseq,pwcodec,dressed);
   decon();
   /* C' decoupling on */

   delay(taub - tau2_2 - gt3 - gstab -102.0e-6 
         - POWER_DELAY - PRG_START_DELAY
         - PRG_STOP_DELAY - POWER_DELAY);

   /* C' decoupling off */
   decoff();
   decprgoff();
   decpower(dhpwr);
   /* C' decoupling off */

   decrgpulse(pwc,zero,2.0e-6,0.0);

status(A);
   delay(taua - pwc - 2.0*pwc - 2.0*pw 
         - gt1 - gstab -2.0e-6);

   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(gstab);

   rgpulse(pw,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl4,gt4);
   delay(gstab +100.0e-6);

   decpower(dpwr);  /* Set power for decoupling */
   dec2power(dpwr2);  /* Set power for decoupling */

   rgpulse(pw,zero,0.0,0.0);

/* BEGIN ACQUISITION */

status(C);
setreceiver(t5);
}
