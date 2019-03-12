/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghn_co_hbA.c

   3D HN(CO)HB experiment for the measurement of CO-HB coupling constant.

    REF:  Stephan Grzesiek, Mitsuhiko Ikura, G. Marius Clore,
          Angela M. Gronenborn, and Ad Bax, JMR, 96, 215 - 221 (1992).

          Ad Bax, Geerten W. Vuister, Stephan Grzesiek, 
          Frank Delaglio, Andy C. Wang, Rolf Tschudin, Guang Zhu, 
          Methods in Enzymology, 239, 79-105 (1994).

    Written by 
    Weixing Zhang
    St. Jude Children's Research Hospital
    Memphis, TN 38105
    Modified to use gradient selection. 
    Auto-calibrated version, E.Kupce, 27.08.2002.

    Uses three channels:
        1)  HN  (t3)             -carrier at 4.7 ppm (tof)
            NB  (t1, ni)
        2)  CO  (ref_flg=y,t1)   -carrier at 175 ppm (dof)
        3)  15N (t2, ni2)        -carrier at 118 ppm (dof2)

    mag_flg = y    using magic angle pulsed field gradient
    mag_flg = n    using z-axis gradient only

    ref_flg = y    recording 2D HN-CO (t1) or 3D HNCO reference spectrum.
            = n    recording 3D spectrum with CO-HB coupling present.

   The autocal and checkofs flags are generated automatically in Pbox_bio.h
   If these flags do not exist in the parameter set, they are automatically 
   set to 'y' - yes. In order to change their default values, create the flag(s) 
   in your parameter set and change them as required. 
   The available options for the checkofs flag are: 'y' (yes) and 'n' (no). 
   The offset (tof, dof, dof2 and dof3) checks can be switched off individually 
   by setting the corresponding argument to zero (0.0).
   For the autocal flag the available options are: 'y' (yes - by default), 
   'q' (quiet mode - suppress Pbox output), 'r' (read from file, no new shapes 
   are created), 's' (semi-automatic mode - allows access to user defined 
   parameters) and 'n' (no - use full manual setup, equivalent to the original 
   code).
*/

#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */

extern int dps_flag;

static int 
            phi1[4]  = {0,0,2,2},
            phi2[2]  = {0,2},
            phi3[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
            phi4[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
            phi5[1]  = {0},
     
            rec[16]  = {0,2,2,0,2,0,0,2,2,0,0,2,0,2,2,0};   
          
static double d2_init=0.0, d3_init=0.0;
static double   H1ofs=4.7, C13ofs=174.0, N15ofs=120.0, H2ofs=0.0;

static shape offC8, offC17, offC27;

void pulsesequence()
 {
  char    
    f1180[MAXSTR],    
    f2180[MAXSTR],
    mag_flg[MAXSTR],  /* y for magic angle, n for z-gradient only  */
    ref_flg[MAXSTR];  /* yes for recording reference spectrum      */

 int        
    icosel,
    phase,
    ni2,     
    t1_counter,   
    t2_counter;   

 double      
    gzcal = getval("gzcal"),
    tau1,       
    tau2,       
    taua,         /*  ~ 1/4JNH =  2.3-2.7 ms]        */
    taub,         /*  ~ 2.75 ms          */
    bigT,         /*  ~ 12 ms            */
    bigTCO,       /*  ~ 25 ms            */
    bigTN,        /*  ~ 12 ms            */

    pwClvl,   /* High power level for carbon on channel 2 */
    pwC,      /* C13 90 degree pulse length at pwClvl     */
    compC,     /* Compression factor for C13 on channel 2  */
    pwCa180,  /* 180 degree pulse length for Ca           */
    pwCab180,
    pwCO180,

    pwNlvl,   /* Power level for Nitrogen on channel 3    */
    pwN,      /* N15 90 degree pulse lenght at pwNlvl     */
    maxcan,   /*  The larger of 2.0*pwN and pwCa180        */
    
    bw, ofs, ppm, /* bandwidth, offset, ppm - temporary Pbox parameters */
    dpwrfC = 4095.0,
    dfCa180,
    dfCab180,
    dfCO180,

    gt1,
    gt3,
    gt2,
    gt0,
    gt5,
    gt6,
    gt7,
    gt8,
    gt9,
    gt10,
    gstab,
    gzlvl1,
    gzlvl2,
    gzlvl3,
    gzlvl0,
    gzlvl5,
    gzlvl6,
    gzlvl7,
    gzlvl8,
    gzlvl9,
    gzlvl10;

/* LOAD VARIABLES */

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("mag_flg", mag_flg);
    getstr("ref_flg", ref_flg);

    taua   = getval("taua"); 
    taub   = getval("taub");
    bigT = getval("bigT");
    bigTCO = getval("bigTCO");
    bigTN = getval("bigTN");
  
    pwClvl = getval("pwClvl");
    pwC = getval("pwC");
    compC = getval("compC");

    pwNlvl = getval("pwNlvl");
    pwN = getval("pwN");

    setautocal();                        /* activate auto-calibration flags */ 
        
    if (autocal[0] == 'n') 
    { 
      pwCa180 = getval("pwCa180");
      pwCab180 = getval("pwCab180");
      pwCO180 = getval("pwCO180");

      dfCa180 = (compC*4095.0*pwC*2.0*1.69)/pwCa180;     /*power for "offC17" pulse*/
      dfCab180 = (compC*4095.0*pwC*2.0*1.69)/pwCab180;   /*power for "offC27" pulse*/
      dfCO180  = (compC*4095.0*pwC*2.0*1.65)/pwCO180;    /*power for "offC8" pulse */

      dfCa180 = (int) (dfCa180 + 0.5);
      dfCO180 = (int) (dfCO180 + 0.5);	
      dfCab180 = (int) (dfCab180 +0.5);
    }
    else
    {
      if(FIRST_FID)                                            /* call Pbox */
      {
        ppm = getval("dfrq"); bw = 118.0*ppm; ofs = -118.0*ppm;
        offC8 = pbox_make("offC8", "sinc180n", bw, 0.0, compC*pwC, pwClvl);
        offC17 = pbox_make("offC17", "sinc180n", bw, ofs, compC*pwC, pwClvl);
        bw = 128.0*ppm; ofs = -128.0*ppm;
        offC27 = pbox_make("offC27", "sinc180n", bw, ofs, compC*pwC, pwClvl);
        ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
      } 
      dfCO180 = offC8.pwrf;    pwCO180 = offC8.pw;
      dfCa180 = offC17.pwrf;   pwCa180 = offC17.pw;
      dfCab180 = offC27.pwrf;  pwCab180 = offC27.pw;
    }

    maxcan = 2.0*pwN;
    if (pwCa180 > maxcan) maxcan = pwCa180;
 
    dpwr = getval("dpwr");
    phase = (int) ( getval("phase") + 0.5);
    phase2 = (int) ( getval("phase2") + 0.5);
    sw1 = getval("sw1");
    sw2 = getval("sw2");
    ni = getval("ni");
    ni2 = getval("ni2");
 
    gt1 = getval("gt1");
    gt2 = getval("gt2");
    gt3 = getval("gt3");
    gt0 = getval("gt0");
    gt5 = getval("gt5");
    gt6 = getval("gt6");
    gt7 = getval("gt7");
    gt8 = getval("gt8");
    gt9 = getval("gt9");
    gstab = getval("gstab");
    gt10 = getval("gt10");
 
    gzlvl1 = getval("gzlvl1");
    gzlvl2 = getval("gzlvl2");
    gzlvl3 = getval("gzlvl3");
    gzlvl5 = getval("gzlvl5");
    gzlvl0 = getval("gzlvl0");
    gzlvl6 = getval("gzlvl6");
    gzlvl7 = getval("gzlvl7");
    gzlvl8 = getval("gzlvl8");
    gzlvl9 = getval("gzlvl9");
    gzlvl10 = getval("gzlvl10");

/* LOAD PHASE TABLE */

    settable(t1,4,phi1);
    settable(t2,2,phi2);
    settable(t3,16,phi3);
    settable(t4,16,phi4);
    settable(t5, 1, phi5);
   
    settable(t10,16,rec);
  

/* CHECK VALIDITY OF PARAMETER RANGES */
        
    if((ref_flg[A] == 'y') && (dps_flag))
    {
       printf("ref_flg=y: for 2D HN-CO or 3D HNCO reference spectrum without CO-HB coupling.\n");
    }

    if(ni2/sw2 > 2.0*(bigTN))
    {
       printf(" ni2 is too big, should < %f\n", 2.0*sw2*(bigTN));
    }

    if((ni/sw1 > 2.0*(bigTCO - gt6 - maxcan))&&(ref_flg[A] == 'y'))
    {
       printf("ni is too big, should < %f\n", 2.0*sw1*(bigTCO-gt6-maxcan));
    }
  
    if(( dpwr > 50 ) || (dpwr2 > 50))
    {
        printf("don't fry the probe, either dpwr or dpwr2 is  too large!  ");
        psg_abort(1);
    }

    if((gt1 > 5.0e-3) ||(gt2>5e-3)||(gt3>5e-3)|| (gt0 > 5.0e-3))
    {
        printf("The length of gradients are too long\n");
        psg_abort(1);
    }

    if((taub - 2.0*pw - gt8 - 1.0e-3 - 6.0*GRADIENT_DELAY)<0.0)
    {
        printf("Shorten gt8 so that preceding delay is not negative\n");
        psg_abort(1);
    }

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y'))
    {
        printf("incorrect dec1 decoupler flags! should be 'nnn' ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' ))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nny' ");
        psg_abort(1);
    }

/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2)
    {
        if(ref_flg[A] == 'y') tsadd(t1,3,4);
        else tsadd(t1, 1, 4);
    }
  
    if (phase2 == 2)
    {
       tsadd(t5,2,4);
       icosel = 1; 
    }
    else icosel = -1;

/*  Set up f1180  half_dwell time (1/sw1)/2.0           */
   
    if (ref_flg[A] == 'y') tau1 = d2;
    else tau1 = d2 - (4.0*pw/PI);

    if(f1180[A] == 'y')
    {
        tau1 += (1.0/(2.0*sw1));
    }
    if(tau1 < 0.2e-6) tau1 = 0.0;
    tau1 = tau1/4.0;

/*  Set up f2180   half dwell time (1/sw2)/2.0           */

    tau2 = d3;
    if(f2180[A] == 'y')
    {
        tau2 += (1.0/(2.0*sw2)); 
    }
    if(tau2 < 0.2e-6) tau2 = 0.0;
    tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition   */

    if( ix == 1) d2_init = d2 ;
    t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
    if(t1_counter % 2) 
    {
       tsadd(t1,2,4);     
       tsadd(t10,2,4);    
    }
   
    if( ix == 1) d3_init = d3 ;
    t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
    if(t2_counter % 2) 
    {
       tsadd(t2,2,4);  
       tsadd(t10,2,4);    
    }
   

/* BEGIN ACTUAL PULSE SEQUENCE */

 status(A);
    decoffset(dof);
    obspower(tpwr);
    decpower(pwClvl);
    decpwrf(dpwrfC);
    dec2power(pwNlvl);
    txphase(zero);
    dec2phase(zero);

    delay(d1);
    dec2rgpulse(pwN, zero, 0.2e-6, 0.0);
    zgradpulse(gzlvl3, gt3);
    delay(0.001);
    rcvroff();

status(B);
    rgpulse(pw, zero, 1.0e-5, 1.0e-6);
    delay(2.0e-6);
    zgradpulse(0.8*gzlvl0,gt0);
    delay(taua - gt0 - 2.0e-6);
    sim3pulse(2.0*pw,(double)0.0,2.0*pwN,zero,zero,zero, 1.0e-6, 1.0e-6);
 
    delay(taua - gt0 - 500.0e-6);
    zgradpulse(0.8*gzlvl0,gt0);
    txphase(one);
    delay(500.0e-6);
    rgpulse(pw, one, 1.0e-6, 1.0e-6);

    delay(2.0e-6);
    zgradpulse(1.3*gzlvl3, gt3);

    decpwrf(dfCO180);
    txphase(zero);
    delay(200.0e-6);

    dec2rgpulse(pwN, zero, 0.0, 0.0);
    delay(2.0e-6);
    zgradpulse(gzlvl0, gt0);
    delay(taub - gt0 - 2.0*pw - 2.0e-6);
    rgpulse(2.0*pw, zero, 0.0, 0.0);

    delay(bigT - taub - WFG3_START_DELAY);
    sim3shaped_pulse("","offC8","",(double)0.0,pwCO180,2.0*pwN,zero,zero,zero,0.0,0.0);

    delay(bigT - gt0 - WFG3_STOP_DELAY - 1.0e-3);

    zgradpulse(gzlvl0, gt0);
    delay(1.0e-3);
  
    dec2rgpulse(pwN, zero, 0.0, 0.0);
    delay(2.0e-6);
    zgradpulse(gzlvl9, gt9);
    decpwrf(dpwrfC);
    decphase(t3);
    delay(200.0e-6);

    if(ref_flg[A] != 'y')
    {
       decrgpulse(pwC, t3, 0.0, 0.0);
       delay(2.0e-6);
       if(gt6 > 0.2e-6)  
          zgradpulse(gzlvl6, gt6);

       decpwrf(dfCO180);
       txphase(t1);
       delay(bigTCO - gt6 - 2.0e-6); 
       rgpulse(pw, t1, 0.0, 0.0);
       if(tau1 >(pwCab180/2.0 + WFG_START_DELAY +WFG_STOP_DELAY+ POWER_DELAY + pwCO180/2.0))
       {
          decpwrf(dfCab180);
          delay(tau1 - pwCab180/2.0 - WFG_START_DELAY - POWER_DELAY); 
          decshaped_pulse("offC27", pwCab180, zero, 0.0, 0.0);
          decpwrf(dfCO180);
          delay(tau1-pwCO180/2.0-pwCab180/2.0-WFG_STOP_DELAY-WFG_START_DELAY-POWER_DELAY);
          decshaped_pulse("offC8", pwCO180, zero, 0.0, 0.0);
          decpwrf(dfCab180);
          delay(tau1-pwCO180/2.0-pwCab180/2.0-WFG_STOP_DELAY-WFG_START_DELAY-POWER_DELAY);
          decshaped_pulse("offC27", pwCab180, zero, 0.0, 0.0);
          txphase(zero);
          delay(tau1 - pwCab180/2.0 - WFG_STOP_DELAY); 
       }
       else
       {
          delay(2.0*tau1);
          decshaped_pulse("offC8", pwCO180, zero, 0.0, 0.0);
          txphase(zero);
          delay(2.0*tau1);
       }
       rgpulse(pw, zero, 0.0, 0.0);
       delay(bigTCO - gt6 - POWER_DELAY - 1.0e-3);
       if (gt6 > 0.2e-6)
          zgradpulse(gzlvl6, gt6);
       decpwrf(dpwrfC);
       delay(1.0e-3);
       decrgpulse(pwC, zero, 0.0, 0.0);
    }
    else
    {
       decrgpulse(pwC, t3, 0.0, 0.0);
       decpwrf(dfCa180);
       sim3shaped_pulse("","offC17","",0.0e-6,pwCa180,0.0e-6,zero,zero,zero,2.0e-6,0.0);
       delay(2.0e-6);
       if(gt6 > 0.2e-6)  
          zgradpulse(gzlvl6, gt6);

       decpwrf(dfCO180);
       delay(bigTCO - 2.0*tau1 - maxcan - gt6 - 2.0*POWER_DELAY - 4.0e-6);

       decshaped_pulse("offC8", pwCO180, zero, 0.0, 0.0);
 
       delay(bigTCO - gt6 - maxcan - 2.0*POWER_DELAY - 1.0e-3);
       if (gt6 > 0.2e-6)
          zgradpulse(gzlvl6, gt6);
       decpwrf(dfCa180);
       delay(1.0e-3);
       sim3shaped_pulse("","offC17","",2.0*pw,pwCa180,2.0*pwN,zero,zero,zero,0.0,0.0);
     
       decpwrf(dpwrfC);
       delay(2.0*tau1);
       decrgpulse(pwC, t1, 2.0e-6, 0.0);
    }
    delay(2.0e-6);
    zgradpulse(gzlvl7, gt7);
    decpwrf(dfCO180);
    dec2phase(t2);
    delay(200.0e-6);

    dec2rgpulse(pwN, t2, 0.0, 0.0);

    delay(bigTN - tau2);
    dec2phase(t4);

    sim3shaped_pulse("","offC8","",(double)0.0,pwCO180,2.0*pwN,zero,zero,t4,0.0,0.0);

    decpwrf(dfCa180);
    delay(bigTN - taub - WFG_STOP_DELAY - POWER_DELAY);
    rgpulse(2.0*pw, zero, 0.0, 0.0);

    delay(taub - 2.0*pw - gt1 - 1.0e-3 - 6.0*GRADIENT_DELAY);
    if (mag_flg[A] == 'y')
    {
        magradpulse(gzcal*gzlvl1, gt1);
    }
    else
    {
        zgradpulse(gzlvl1, gt1);
        delay(4.0*GRADIENT_DELAY);
    }
    dec2phase(t5);
    delay(1.0e-3);
    decshaped_pulse("offC17", pwCa180, zero, 0.0, 0.0);
    delay(tau2);

    sim3pulse(pw,(double)0.0, pwN, zero,zero, t5, 0.0, 0.0);
    dec2phase(zero);
    delay(2.0e-6);
    zgradpulse(0.8*gzlvl5, gt5);
    delay(taua - gt5 - 2.0e-6);
    sim3pulse(2.0*pw,(double)0.0, 2.0*pwN, zero,zero, zero, 0.0, 0.0);
    delay(taua - gt5 - 500.0e-6);
    zgradpulse(0.8*gzlvl5, gt5);
    txphase(one);
    decphase(one);
    delay(500.0e-6);

    sim3pulse(pw,(double)0.0, pwN, one,zero, one, 0.0, 0.0);
    
    delay(2.0e-6);
    txphase(zero);
    dec2phase(zero);
    zgradpulse(gzlvl5, gt5);
    delay(taua - gt5 - 2.0e-6);
    sim3pulse(2.0*pw,(double)0.0, 2.0*pwN, zero,zero, zero, 0.0, 0.0);
  
    delay(taua - gt5 - 2.0*POWER_DELAY - 500.0e-6);
    zgradpulse(gzlvl5, gt5);
    decpower(dpwr);
    dec2power(dpwr2);
    delay(500.0e-6);

    rgpulse(pw, zero, 0.0, 0.0);

    delay(1.0e-4 +gstab + gt1/10.0 - 0.5*pw + 6.0*GRADIENT_DELAY);
    rgpulse(2.0*pw, zero, 0.0, 0.0);
    delay(2.0e-6);
    if(mag_flg[A] == 'y')
    {
       magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);
    }
    else
    {
       zgradpulse(icosel*gzlvl2, gt1/10.0);  
       delay(4.0*GRADIENT_DELAY);
    }           
    delay(1.0e-4  - 2.0e-6);               
statusdelay(C,1.0e-4);
    setreceiver(t10);
}
