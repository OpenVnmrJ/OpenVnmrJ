/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  hmcmcbcaA.c

    This pulse sequence will allow one to perform the following
    experiment:

    3D CT transfer from Cm to Cb (Cg) to Ca (Cb) for Val (Leu/Ile)
       Use for deuterated samples
	  	F1     13Cb,a 
               	F2     13Cm
               	F3(acq) 1Hm

    Uses three channels:
    For proteins:
         1)  1H             - carrier @ 4.7 ppm [H2O]
         2) 13C             - carrier @ 40 ppm 


    Set dm = 'nny', dmm = 'ccp' [13C decoupling during acquisition].
    Set dm2 = 'nnn', dmm2 = 'ccc' 

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [H]  and t2 [C].

    Set f1180 = 'y' and f2180 = 'n' for (0,0) in F1 and F2.    

    Written by L.E.Kay on March 2, 2003
    Modified for AutoCal via Pbox by Eriks Kupce, Dec 2010
    Modified for consistency for Biopack, GG Feb 2011

*/

#include <standard.h>
#include "Pbox_bio.h"      /* Bio Pack Shapes */

static int  phi1[2]  = {0,2},
            phi2[2]  = {1,3},
            phi3[4]  = {1,1,3,3},
            phi4[8]  = {1,1,1,1,3,3,3,3},
            phi5[16] = {1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3},
            phi6[1] =  {0},
            rec[2]   = {0,2};

static double d2_init=0.0, d3_init=0.0;
static shape    rb180;
            
pulsesequence()
{
/* DECLARE VARIABLES */

 char       fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            C_flg[MAXSTR],
            dtt_flg[MAXSTR];

 int         phase, phase2, ni, ni2, 
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JHC =  1.6 ms */
             taub,         /*    1/6JCH =   1.1 ms  */
             BigTC,        /* Carbon constant time period = 1/4Jcc = 7.0 ms */ 
             pwN,          /* PW90 for 15N pulse @ pwNlvl           */
             pwC,          /* PW90 for c nucleus @ pwClvl         */
             pwcrb180,     /* PW180 for C 180 reburp @ dpwr_rb */
             pwClvl,       /* power level for 13C pulses on dec1  */
             compC, compH, /* compression factors for C13 and H1 amps */
	     dpwr_rb,      /* power level for 13C reburp pulse     */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             

	     gt0,
             gt1,
             gt2,
             gt3,
             gt4,

             gstab,
             gzlvl0,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
        
             decstep1,
             ppm, bw, tpwrs, 

             rfsl,
             pwHs, 
             dof_me,
             
             tof_dtt,
             rfsl1,
             pwHs1;
             
   
/* LOAD VARIABLES */

  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("fscuba",fscuba);
  getstr("C_flg",C_flg);
  getstr("dtt_flg",dtt_flg); 

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  BigTC  = getval("BigTC");
  pwC = getval("pwC");
  pwN = getval("pwN");
  pwClvl = getval("pwClvl");
  compC = getval("compC");
  compH = getval("compH");
  dpwr = getval("dpwr");
  pwNlvl = getval("pwNlvl");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  ni = getval("ni");
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
 
  decstep1 = getval("decstep1");
  dpwr_rb = 0.0;  /* calculated */

  
  pwHs = getval("pwHs");
  dof_me = getval("dof_me");

  pwHs1 = pwHs;
  tof_dtt = getval("tof_dtt");

  setautocal();                      /* activate auto-calibration */   

  if(FIRST_FID)                                         /* make shapes */
  {
    ppm = getval("dfrq"); 
    bw = 80.0*ppm;  
    rb180 = pbox_make("pwcrb180P", "reburp", bw, 0.0, compC*pwC, pwClvl);
    if(taua < (gt4+106e-6+pwHs)) printf("gt4 or pwHs may be too long! ");
  }
  pwcrb180 = rb180.pw; dpwr_rb = rb180.pwrf;             /* set up parameters */
  tpwrs = tpwr - 20.0*log10(pwHs/((compH*pw)*1.69));   /* sinc=1.69xrect */
  tpwrs = (int) (tpwrs);               


/* LOAD PHASE TABLE */

  settable(t1,2,phi1);
  settable(t2,2,phi2);
  settable(t3,4,phi3);
  settable(t4,8,phi4);
  settable(t5,16,phi5);
  settable(t6,1,phi6);
  settable(t7,2,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */

    if( BigTC - 0.5*(ni2-1)*1/(sw2) - WFG_STOP_DELAY - POWER_DELAY 
              - 4.0e-6
              < 0.2e-6 )
    {
        printf(" ni2 is too big\n");
        psg_abort(1);
    }


    if((dm[A] == 'y' || dm[B] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' ))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnn' ");
        psg_abort(1);
    }

    if( satpwr > 6 )
    {
        printf("satpwr too large !!!  ");
        psg_abort(1);
    }

    if( dpwr > 48 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > -16 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( pw > 200.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 

    if( pwN > 200.0e-6 )
    {
        printf("dont fry the probe, pwN too high ! ");
        psg_abort(1);
    } 

    if( pwC > 200.0e-6 )
    {
        printf("dont fry the probe, pwC too high ! ");
        psg_abort(1);
    } 

    if( pwcrb180 > 500.0e-6 )
    {  
        printf("dont fry the probe, pwcrb180 too high ! ");
        psg_abort(1);
    } 

    if(dpwr3 > 51)
    {
       printf("dpwr3 is too high; < 52\n");
       psg_abort(1);
    }

   if(d1 < 1)
    {
       printf("d1 must be > 1\n");
       psg_abort(1);
    }

    if(  gt0 > 5.0e-3 || gt1 > 5.0e-3  || gt2 > 5.0e-3 ||
         gt3 > 5.0e-3 || gt4 > 5.0e-3  )
    {  printf(" all values of gti must be < 5.0e-3\n");
        psg_abort(1);
    }

/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2) {
      tsadd(t1,1,4);
      tsadd(t2,1,4);
      tsadd(t3,1,4);
    }

    if (phase2 == 2)
      tsadd(t6,1,4);

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    tau1 = tau1 - 2.0*pw - 4.0/PI*pwC;

    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) );
        if(tau1 < 0.4e-6) tau1 = 4.0e-7;
    }
        tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */

    tau2 = d3;
    if(f2180[A] == 'y') {
        tau2 += ( 1.0 / (2.0*sw2) ); 
        if(tau2 < 0.4e-6) tau2 = 4.0e-7;
    }
        tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t1,2,4);     
      tsadd(t7,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t6,2,4);  
      tsadd(t7,2,4);    
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(satpwr);      /* Set transmitter power for 1H presaturation */
   decpower(pwClvl);      /* Set Dec1 power for hard 13C pulses         */
   dec2power(pwNlvl);     /* Set Dec2 to low power       */
   lk_hold(); lk_sampling_off();  /*freezes z0 correction, stops lock pulsing*/

/* Presaturation Period */

   if (satmode[A] == 'y')
   {
	delay(2.0e-5);
        rgpulse(d1,zero,2.0e-6,2.0e-6);  /* presat with transmitter */
   	obspower(tpwr);      /* Set transmitter power for hard 1H pulses */
	delay(2.0e-5);
	if(fscuba[0] == 'y')
	{
		delay(2.2e-2);
		rgpulse(pw,zero,2.0e-6,0.0);
		rgpulse(2.0*pw,one,2.0e-6,0.0);
		rgpulse(pw,zero,2.0e-6,0.0);
		delay(2.2e-2);
	}
   }
   else
   {
    delay(d1);
   }
status(B);
   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   obsoffset(tof);
   txphase(t1);
   decphase(zero);
   dec2phase(zero);
   delay(1.0e-5);

/* Begin Pulses */

   decoffset(dof_me);

   rcvroff();
   delay(20.0e-6);

/* ensure that magnetization originates on 1H and not 13C */

   if(dtt_flg[A] == 'y') {
     obsoffset(tof_dtt);
     obspower(tpwrs);
     shaped_pulse("H2Osinc",pwHs1,zero,10.0e-6,0.0);
     obspower(tpwr);

     obsoffset(tof); 
   }
 
   decrgpulse(pwC,zero,0.0,0.0);
 
   delay(2.0e-6);
   zgradpulse(gzlvl0,gt0);
   delay(gstab);

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(gstab);

   delay(taua - gt1 - gstab -2.0e-6 ); 

   simpulse(2.0*pw,2.0*pwC,zero,zero,0.0,0.0);
   txphase(one);

   delay(taua - gt1 - gstab -2.0e-6); 
   	
   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(gstab);

   rgpulse(pw,one,0.0,0.0);

   /* shaped_pulse */
   obspower(tpwrs);
   shaped_pulse("H2Osinc",pwHs,zero,2.0e-6,0.0);
   obspower(tpwr);
   /* shaped_pulse */

   decoffset(dof);  /* jump 13C to 40 ppm */

   delay(2.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(gstab);

   /* turn on 2H decoupling */
   dec3phase(one);
   dec3power(dpwr3);
   dec3rgpulse(1/dmf3,one,4.0e-6,0.0); 
   dec3phase(zero);
   dec3unblank();
   dec3prgon(dseq3,1/dmf3,dres3);
   dec3on();
   /* turn on 2H decoupling */

   decrgpulse(pwC,t1,4.0e-6,0.0); decphase(zero); 

   initval(1.0,v3);
   decstepsize(decstep1);
   dcplrphase(v3);

   decpwrf(dpwr_rb);
   delay(BigTC - POWER_DELAY - WFG_START_DELAY);

   decshaped_pulse(rb180.name,pwcrb180,zero,0.0,0.0);
   dcplrphase(zero);
   decphase(t2);

   decpwrf(4095.0);
   delay(BigTC - WFG_STOP_DELAY - POWER_DELAY);

   decrgpulse(pwC,t2,0.0,0.0);
   decphase(zero);

   initval(1.0,v3);
   decstepsize(decstep1);
   dcplrphase(v3);

   decpwrf(dpwr_rb);
   delay(taub - POWER_DELAY - WFG_START_DELAY);

   decshaped_pulse(rb180.name,pwcrb180,zero,0.0,0.0);
   dcplrphase(zero);
   decphase(t3);

   decpwrf(4095.0);
   delay(taub - WFG_STOP_DELAY - POWER_DELAY);

   decrgpulse(pwC,t3,0.0,0.0);
   decphase(t4);

   if(C_flg[A] == 'n') {
   delay(tau1);
   rgpulse(2.0*pw,zero,0.0,0.0);
   delay(tau1);
   }

   else 
    simpulse(2.0*pw,2.0*pwC,zero,zero,4.0e-6,4.0e-6);

   decrgpulse(pwC,t4,0.0,0.0);
   decphase(zero);

   initval(1.0,v3);
   decstepsize(decstep1);
   dcplrphase(v3);

   decpwrf(dpwr_rb);
   delay(taub - POWER_DELAY - WFG_START_DELAY);

   decshaped_pulse(rb180.name,pwcrb180,zero,0.0,0.0);
   dcplrphase(zero);
   decphase(t5);

   decpwrf(4095.0);
   delay(taub - WFG_STOP_DELAY - POWER_DELAY);

   decrgpulse(pwC,t5,0.0,0.0);
   decphase(zero);

   delay(tau2);
   rgpulse(2.0*pw,zero,0.0,0.0);

   initval(1.0,v3);
   decstepsize(decstep1);
   dcplrphase(v3);

   decpwrf(dpwr_rb);
   delay(BigTC - 2.0*pw - POWER_DELAY - WFG_START_DELAY);

   decshaped_pulse(rb180.name,pwcrb180,zero,0.0,0.0);
   dcplrphase(zero);
   decphase(t6);
   decpwrf(4095.0);

   delay(BigTC - tau2 - WFG_STOP_DELAY - POWER_DELAY - 4.0e-6);

   decrgpulse(pwC,t6,4.0e-6,0.0);

   /* 2H decoupling off */
   dec3off();
   dec3prgoff();
   dec3blank();
   dec3rgpulse(1/dmf3,three,4.0e-6,0.0);
   lk_autotrig();   /* resume lock pulsing */
   /* 2H decoupling off */

   decoffset(dof_me);

   delay(2.0e-6);
   zgradpulse(gzlvl3,gt3);
   delay(gstab);

   /* shaped_pulse */
   obspower(tpwrs);
   shaped_pulse("H2Osinc",pwHs,two,2.0e-6,0.0);
   obspower(tpwr);
   /* shaped_pulse */

   rgpulse(pw,zero,4.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl4,gt4);
   delay(gstab);

   delay(taua - gt4 - gstab -2.0e-6
         - POWER_DELAY - 2.0e-6 - WFG_START_DELAY
         - pwHs - WFG_STOP_DELAY - POWER_DELAY - 2.0e-6);

   /* shaped_pulse */
   obspower(tpwrs);
   shaped_pulse("H2Osinc",pwHs,two,2.0e-6,0.0);
   obspower(tpwr);
   /* shaped_pulse */

   simpulse(2.0*pw,2.0*pwC,zero,zero,2.0e-6,0.0);

   /* shaped_pulse */
   obspower(tpwrs);
   shaped_pulse("H2Osinc",pwHs,two,2.0e-6,0.0);
   obspower(tpwr);
   /* shaped_pulse */

   delay(2.0e-6);
   zgradpulse(gzlvl4,gt4);
   delay(gstab); 
 
   delay(taua 
         - POWER_DELAY - 2.0e-6 - WFG_START_DELAY
         - pwHs - WFG_STOP_DELAY - POWER_DELAY 
         - gt4 - gstab -2.0e-6 - 2.0*POWER_DELAY);

   decpower(dpwr);  /* Set power for decoupling */
   dec2power(dpwr2);

/*   rcvron();  */          /* Turn on receiver to warm up before acq */ 

   lk_sample();

/* BEGIN ACQUISITION */

status(C);
   setreceiver(t7);

}
