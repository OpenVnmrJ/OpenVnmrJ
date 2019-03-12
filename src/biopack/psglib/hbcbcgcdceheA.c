/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  hbcbcgcdceheA.c - auto-calibrated version of the original sequence
       hbcbcgcdcehe_aro_pfg_500a.c


    This pulse sequence will allow one to perform the following experiment:

    2D correlation of cb and hd, he of aromatic residues though JCC.
                       F1        CB
                       F2(acq)   HD and HE of aromatic

    Uses two channels:
         1)  1H             - carrier (tof) @ 4.7 ppm [H2O]
         2) 13C             - carrier dof @ 35 ppm shift to dofaro @ 125 ppm
                              (The centre of F1 is 35ppm (dof))

    Set dm = 'nny', dmm = 'ccp' [13C decoupling during acquisition].
    Set dm2 = 'nnn', dmm2 = 'ccc' 

    Must set phase = 1,2 for States-TPPI acquisition in t1 [cb].

    Flags
         fsat      'y' for presaturation of H2O
         fscuba    'y' to apply scuba pulse after presaturation of H2O
         mess_flg  'y' for 1H (H2O) purging before relaxtion delay
         f1180     'y' for 180deg linear phase correction in F1
                       otherwise 0deg linear phase correction in F1

    Standard Settings 
                       fsat='n', fscuba='n', mess_flg='y', f1180='n'
    
    The flag f1180 should be set to 'y' if t1 is 
    to be started at halfdwell time. This will give -90, 180
    phasing in f1. If it is set to 'n' the phasing will
    be 0,0 and will still give a perfect baseline.

    Written by T. Yamazaki  July 12, 1993
    Added 2us delays between power and pulse statements -
    to be compatible with Unity Inova RM Mar 06/97

    Added automatic Pbox, Eriks Kupce, Jan 03
       The autocal flag is generated automatically in Pbox_bio.h
       If this flag does not exist in the parameter set, it is automatically 
       set to 'y' - yes. In order to change the default value, create the  
       flag in your parameter set and change as required. 
       For the autocal flag the available options are: 'y' (yes - by default), 
       and 'n' (no - use full manual setup, equivalent to 
       the original code).
    Eliminated DODEV, TODEV, DO2DEV for channel independence, GG, Mar 03
    changed rgradient to zgradpulse  GG, Aug 03
    added gstab for recovery delay

REF: T. Yamazaki, J. D. Forman-Kay and L. E. Kay
                                          J. Am. Chem. Soc. 115, 11054, (1993)    
*/

#include <standard.h>
#include "Pbox_bio.h"

/* #include "delays.h" */
/* #define PI 3.1416   */


#define SEL90   "square90n 90p"                  /* square 90 on-resonance, null 90 ppm away  */
#define AR180a  "g3 85p 90p"                        /* 180 G3 on Aro at 125 ppm, 90 ppm away  */
#define AR180b  "g3 85p"                                        /* 180 on-resonance G3 on Aro */
#define CB180b  "g3 85p -90p"                        /* 180 G3 on Cb at 35 ppm, -90 ppm away  */
#define CB180ps "-maxincr 2.0 -attn i"                         /* seduce 180 shape parameters */

static shape sel90, ar_180a, ar_180b, cb_180b, w16;

static int  phi1[1]  = {1},
            phi2[1]  = {0},
            phi3[4]  = {0,1,2,3},
            phi4[8]  = {0,0,0,0,2,2,2,2},
            rec[8]   = {0,2,0,2,2,0,2,0};
           
static double d2_init=0.0;
            
void pulsesequence()
{
/* DECLARE VARIABLES */

 char       autocal[MAXSTR],  /* auto-calibration flag */
            fsat[MAXSTR],
	    fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            mess_flg[MAXSTR], /* water purging */
            ar180a[MAXSTR],   /* waveform shape for aromatic 180 pulse 
                                   with C transmitter at dof  */
            ar180b[MAXSTR],   /* waveform shape for aromatic 180 pulse 
                                   with C transmitter at dofar  */
            cb180b[MAXSTR];   /* waveform shape for aliphatic 180 pulse 
                                   with C transmitter at dofar   */
 int         phase, ni, 
             t1_counter;   /* used for states tppi in t1           */ 

 double      tau1,         /*  t1 delay */
             taua,         /*  ~ 1/4JCbHb =  1.7 ms */
             taub,         /*  ~ 1/4JCgCd =  2.7 ms */
             tauc,         /*  ~ 1/4JCgCd =  2.1 ms */
             tauc2,         /*  ~ 1/4JCgCd =  2.1 ms */
             taud,         /*  ~ 1/4JCdHd =  1.5 ms */
             taue,         /*  = 1/4JCbHb =  1.8 ms */
             tauf,         /*  ~ 1/2JCdHd =  3.1 ms */
             TCb,          /* carbon constant time period 
                              for recording the Cb chemical shifts    */
             dly_pg1,      /* delay for water purging */
             pwar180a,     /* 180 aro pulse at d_ar180a and dof  */
             pwcb180b,     /* 180 cb pulse at d_cb180b and dofar   */ 
             pwC,          /* 90 c pulse at pwClvl            */
             pwsel90,       /* 90 c pulse at d_sel90 */
             pwar180b,     /* 180 c pulse at d_ar180b */
             d_ar180a,
             d_cb180b,
             d_sel90,
             d_ar180b,     
             dofar, 
             tsatpwr,      /* low level 1H trans.power for presat  */
             tpwrmess,     /* power level for water purging */
             tpwrml,       /* power level for 1H decoupling */
             pwmlev,       /* 90 pulse at tpwrml */
             pwClvl,        /* power level for high power 13C pulses on dec1 */ 
             sw1,          /* sweep width in f1                    */
             at,
             gp11,         /* gap between 90-90 for selective 180 of Cb */
             fab,          /* chemical shift difference of Ca-Cb (Hz) */
             compC,        /* C-13 RF calibration parameters */
             compH,
             gstab,
             gt0,
             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt5a,
             gt6,
             gt7,
             gzlvl0,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl5a,
             gzlvl6,
             gzlvl7;


/*  variables commented out are already defined by the system      */


/* LOAD VARIABLES */

  getstr("autocal",autocal);
  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("fscuba",fscuba);
  getstr("mess_flg",mess_flg);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  tauc   = getval("tauc"); 
  tauc2   = getval("tauc2"); 
  taud   = getval("taud"); 
  taue   = getval("taue"); 
  tauf   = getval("tauf"); 
  TCb = getval("TCb");
  pwC = getval("pwC");
  dofar = getval("dofar");
  dly_pg1 = getval("dly_pg1");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  tpwrmess = getval("tpwrmess");
  tpwrml = getval("tpwrml");
  pwClvl = getval("pwClvl");
  dpwr = getval("dpwr");
  phase = (int) ( getval("phase") + 0.5);
  sw1 = getval("sw1");
  ni = getval("ni");
  at = getval("at");
  fab = getval("fab");

  gstab = getval("gstab");
  gt0 = getval("gt0");
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt5a = getval("gt5a");
  gt6 = getval("gt6");
  gt7 = getval("gt7");

  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl5a = getval("gzlvl5a");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");


  if(autocal[0]=='n')
  {     
    getstr("ar180a",ar180a);
    getstr("ar180b",ar180b);
    getstr("cb180b",cb180b);
    pwar180a = getval("pwar180a");
    pwar180b = getval("pwar180b");
    pwcb180b = getval("pwcb180b");
    pwsel90 = getval("pwsel90");
    d_ar180a = getval("d_ar180a");
    d_cb180b = getval("d_cb180b");
    d_ar180b  = getval("d_ar180b");
    d_sel90  = getval("d_sel90");
    pwmlev = getval("pwmlev");
  }
  else
  {    
    strcpy(ar180a,"Pg3_off_cb180a");
    strcpy(ar180b,"Pg3_off_cb180b");    
    strcpy(cb180b,"Pg3_on");
    if (FIRST_FID)
    {
      compC = getval("compC");
      compH = getval("compH");
      sel90 = pbox("cal", SEL90, "", dfrq, compC*pwC, pwClvl);
      ar_180a = pbox(ar180a, AR180a, CB180ps, dfrq, compC*pwC, pwClvl);
      ar_180b = pbox(ar180b, AR180b, CB180ps, dfrq, compC*pwC, pwClvl);
      cb_180b = pbox(cb180b, CB180b, CB180ps, dfrq, compC*pwC, pwClvl);
      w16 = pbox_dec("cal", "WALTZ16", tpwrml, sfrq, compH*pw, tpwr);
    }
    pwsel90 = sel90.pw;      d_sel90 = sel90.pwr;
    pwar180a = ar_180a.pw;   d_ar180a = ar_180a.pwr;
    pwar180b = ar_180b.pw;   d_ar180b = ar_180b.pwr;       
    pwcb180b = cb_180b.pw;   d_cb180b = cb_180b.pwr;  
    pwmlev = 1.0/w16.dmf;
  }   


/* LOAD PHASE TABLE */

  settable(t1,1,phi1);
  settable(t2,1,phi2);
  settable(t3,4,phi3);
  settable(t4,8,phi4);
  settable(t6,8,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */

    if( 0.5*ni*1/(sw1) > TCb - 2*POWER_DELAY 
        - WFG_START_DELAY - pwar180a  - WFG_STOP_DELAY)
    {
        printf(" ni is too big\n");
        psg_abort(1);
    }

    if((dm[A] == 'y' || dm[B] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if(dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y')
    {
        printf("incorrect dec2 decoupler flags!  ");
        psg_abort(1);
    }

    if( tsatpwr > 6 )
    {
        printf("TSATPWR too large !!!  ");
        psg_abort(1);
    }

    if( tpwrml > 53 )
    {
        printf("tpwrml too large !!!  ");
        psg_abort(1);
    }

    if( tpwrmess > 56 )
    {
        printf("tpwrmess too large !!!  ");
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

    if( pwClvl > 63 )
    {
        printf("don't fry the probe, DHPWR too large!  ");
        psg_abort(1);
    }

    if( pw > 20.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 

    if( pwcb180b > 500.0e-6 )
    {
        printf("dont fry the probe, pwcb180b too high ! ");
        psg_abort(1);
    } 

    if( pwar180a > 500.0e-6 )
    {
        printf("dont fry the probe, pwar180a too high ! ");
        psg_abort(1);
    } 

    if (pwar180b > 500.0e-6)
    {
        printf("dont fry the probe, pwsel90 too long !");
        psg_abort(1);
    }

    if (pwsel90 > 100.0e-6)
    {
        printf("dont fry the probe, pwsel90 too long !");
        psg_abort(1);
    }

    if(d_ar180a > 60)
    {
        printf("dont fry the probe, d_ar180a too high !");
        psg_abort(1);
    }

    if(d_cb180b > 60)
    {
        printf("dont fry the probe, d_cb180b too high !");
        psg_abort(1);
    }

    if (d_ar180b > 60)
    {
        printf("dont fry the probe, d_sel90 too high ! ");
        psg_abort(1);
    }

    if (d_sel90 > 50)
    {
        printf("dont fry the probe, d_sel90 too high ! ");
        psg_abort(1);
    }

    if( gt0 > 15e-3 || gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 
       || gt4 > 15e-3 || gt5 > 15e-3 || gt6 > 15e-3 || gt7 > 15e-3 
       || gt5a > 15e-3)
    {
        printf("gradients on for too long. Must be < 15e-3 \n");
        psg_abort(1);
    }

    if( fabs(gzlvl0) > 30000 || fabs(gzlvl1) > 30000 || fabs(gzlvl2) > 30000
      ||fabs(gzlvl3) > 30000 || fabs(gzlvl4) > 30000 || fabs(gzlvl5) > 30000
      ||fabs(gzlvl6) > 30000 || fabs(gzlvl7) > 30000)
    {
        printf("too strong gradient");
        psg_abort(1);
    }


    if( 2*TCb - taue > 0.1 )
    {
        printf("dont fry the probe, too long TCb");
        psg_abort(1);
    }

    if( at > 0.1 && (dm[C]=='y' || dm2[C]=='y'))
    {
        printf("dont fry the probe, too long at with decoupling");
        psg_abort(1);
    }

    if( pwC > 30.0e-6)
    {
        printf("dont fry the probe, too long pwC");
        psg_abort(1);
    }

    if( dly_pg1 > 10.0e-3)
    {
        printf("dont fry the probe, too long dly_pg1");
        psg_abort(1);
    }

    


/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2)
      tsadd(t2,1,4);  

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t2,2,4);     
      tsadd(t6,2,4);    
    }

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) );
    }
    tau1 = tau1/2.0;

/*  90-90 pulse for selective 180 of Cb but not Ca */

    gp11 = 1/(2*fab) - 4/PI*pwsel90;
    if (gp11 < 0.0) {
        printf("gap of 90-90 negative, check fab and pwsel90");
        psg_abort(1);
    }


/* BEGIN ACTUAL PULSE SEQUENCE */

/* Receiver off time */

status(A);
   decoffset(dof);
   obspower(tsatpwr);      /* Set transmitter power for 1H presaturation */
   decpower(pwClvl);        /* Set Dec1 power for hard 13C pulses         */
   dec2power(dpwr2);      /* Set Dec2 power for 15N decoupling       */

/* Presaturation Period */

   if(mess_flg[A] == 'y') {

     obspower(tpwrmess);
     rgpulse(dly_pg1,zero,20.0e-6,20.0e-6);
     rgpulse(dly_pg1/1.62,one,20.0e-6,20.0e-6);
     obspower(tsatpwr);

  }

   if (fsat[0] == 'y')
   {
	delay(2.0e-5);
        rgpulse(d1,zero,20.0e-6,20.0e-6);
   	obspower(tpwr);      /* Set transmitter power for hard 1H pulses */
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

   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   txphase(zero);
   dec2phase(zero);
   decphase(zero);
   delay(1.0e-5);

/* Begin Pulses */

   rcvroff();
   delay(10.0e-6);

/* first ensure that magnetization does infact start on H and not C */

   decrgpulse(pwC,zero,2.0e-6,2.0e-6);

   delay(2.0e-6);
   zgradpulse(gzlvl0,gt0);
   delay(gstab);


/* this is the real start */

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(2.0e-6);

   delay(taua - gt1 - 4.0e-6);   /* taua <= 1/4JCH */                          

   simpulse(2*pw,2*pwC,zero,zero,0.0,0.0);

   txphase(t1);

   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(2.0e-6);

   delay(taua - gt1 - 4.0e-6); 

   rgpulse(pw,t1,0.0,0.0);

   txphase(zero);

   delay(2.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(gstab);

   decphase(t2);
   decpower(d_sel90);
   decrgpulse(pwsel90,t2,2.0e-6,0.0);

   decphase(zero);
   decpower(d_ar180a);
   decshaped_pulse(ar180a,pwar180a,zero,2.0e-6,0.0);  /* bs effect */

   delay(taue 
     - POWER_DELAY - WFG_START_DELAY - 2.0e-6 - pwar180a - WFG_STOP_DELAY
     - POWER_DELAY - PRG_START_DELAY);

   /* H decoupling on */
   obspower(tpwrml);
   obsprgon("waltz16",pwmlev,90.0);
   xmtron();    /* TURN ME OFF  DONT FORGET  */
   /* Hldecoupling on */
   
   delay(TCb + tau1 - taue - POWER_DELAY - 2.0e-6);

   decphase(t3);

   decpower(d_sel90);
   decrgpulse(pwsel90,t3,2.0e-6,0.0);
   delay(gp11);
   decrgpulse(pwsel90,t3,0.0,0.0);

   decphase(zero);
   decpower(d_ar180a);
   decshaped_pulse(ar180a,pwar180a,zero,2.0e-6,0.0); 

   delay(TCb - tau1
     - POWER_DELAY - WFG_START_DELAY - 2.0e-6 - pwar180a - WFG_STOP_DELAY
     - POWER_DELAY - 2.0e-6);
   
   decphase(zero);
   decpower(d_sel90);
   decrgpulse(pwsel90,zero,2.0e-6,0.0);

   /* H decoupling off */
   xmtroff();
   obsprgoff();
   obspower(tpwr);
   /* H decoupling off */

   decoffset(dofar);

   delay(2.0e-6);
   zgradpulse(gzlvl3,gt3);
   delay(gstab);

   decphase(t4);
   decpower(d_sel90); 
   decrgpulse(pwsel90,t4,2.0e-6,0.0);

   decphase(zero);
   decpower(d_cb180b);
   decshaped_pulse(cb180b,pwcb180b,zero,2.0e-6,0.0);   /* B.S. */

   delay(2.0e-6);
   zgradpulse(gzlvl4,gt4);
   delay(2.0e-6);
   
   delay(taub 
     - POWER_DELAY - WFG_START_DELAY - 2.0e-6 - pwcb180b - WFG_STOP_DELAY
     - gt4 - 4.0e-6
     - POWER_DELAY - 2.0e-6);

   decpower(d_ar180b);
   decshaped_pulse(ar180b,pwar180b,zero,2.0e-6,0.0);
 
   decpower(d_cb180b);
   decshaped_pulse(cb180b,pwcb180b,zero,2.0e-6,0.0);
   
   delay(2.0e-6);
   zgradpulse(gzlvl4,gt4);
   delay(2.0e-6);

   delay(taub
     - POWER_DELAY - WFG_START_DELAY - 2.0e-6 - pwcb180b - WFG_STOP_DELAY
     - gt4 - 4.0e-6
     - POWER_DELAY - 2.0e-6);

   decpower(d_sel90);
   decrgpulse(pwsel90,zero,2.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl5,gt5);
   delay(100.0e-6);

   delay(tauc - POWER_DELAY - gt5 - 102.0e-6 - 2.0e-6);

   decphase(zero);
   decpower(pwClvl);
   decrgpulse(2*pwC,zero,2.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl5,gt5);
   delay(100.0e-6);

   txphase(zero);
   delay(tauc  
     - POWER_DELAY - gt5 - 102.0e-6 - 2.0e-6);

   decphase(zero);
   decpower(d_sel90); 
   decrgpulse(pwsel90,zero,2.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl5a,gt5a);
   delay(100.0e-6);
 
   delay(tauc2 - POWER_DELAY - gt5a - 102.0e-6 - 2.0e-6);

   decphase(zero);
   decpower(pwClvl);
   decrgpulse(2*pwC,zero,2.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl5a,gt5a);
   delay(100.0e-6);

   txphase(zero);
   delay(tauf - gt5a - 102.0e-6);

   rgpulse(2*pw,zero,0.0,0.0);

   delay(tauc2 - tauf - 2*pw
     - POWER_DELAY - 2.0e-6);

   decphase(zero);
   decpower(d_sel90); 
   decrgpulse(pwsel90,zero,2.0e-6,0.0);

   txphase(zero);
   delay(2.0e-6);
   zgradpulse(gzlvl6,gt6);
   delay(gstab);
   
   rgpulse(pw,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl7,gt7);
   delay(2.0e-6);

   delay(taud 
     - gt7 - 4.0e-6
     - POWER_DELAY - 2.0e-6);

   decphase(zero);
   decpower(pwClvl);
   simpulse(2*pw,2*pwC,zero,zero,2.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl7,gt7);
   delay(2.0e-6);

   delay(taud 
     - gt7 - 4.0e-6
     - 2*POWER_DELAY);

   decpower(dpwr);  /* Set power for decoupling */
   dec2power(dpwr2);  /* Set power for decoupling */

   rgpulse(pw,zero,0.0,rof2);  
    
/* BEGIN ACQUISITION */

status(C);
setreceiver(t6);

}
