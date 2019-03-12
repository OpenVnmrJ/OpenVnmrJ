/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  hmcmcgcbcaA.c - adapted from hmcmcgcbca_lek_800_v2.c

    This pulse sequence will allow one to perform the following
    experiment:

    3D CT transfer from Cm to Cb to Ca for Val 
                   from Cm to Cg to Cb to Ca for Ile and Leu
       Use for deuterated samples
	  	F1     13Cb,a 
               	F2     13Cm
               	F3(acq) 1Hm

    Uses three channels:
    For proteins:
         1)  1H             - carrier @ 4.7 ppm [H2O]
         2) 13C             - carrier @ 40 ppm 


    Set dm = 'nnny', dmm = 'cccp' [13C decoupling during acquisition].
    Set dm2 = 'nnnn', dmm2 = 'cccc' 

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [H]  and t2 [C].

    Set f1180 = 'y' and f2180 = 'n' for (0,0) in F1 and F2.    

    Apply 3 ms (8.125 ppm) reburp pulses centered at 16 ppm to refocus the Cb-Cg2 Ile couplings

    Must reverse the spectrum in F1.

    Written by L.E.Kay on March 4, 2003

*/

#include <standard.h>
#include "Pbox_bio.h"      /* Bio Pack Shapes */

#define CODEC   "SEDUCE1 20p 136p"                  /* off resonance SEDUCE1 on C' at 176 ppm */
#define CODECps "-dres 1.0 -stepsize 5.0 -attn i"


static int  phi1[2]  = {0,2},
            phi2[4]  = {1,1,3,3},
            phi3[4]  = {1,1,3,3},
            phi4[4]  = {1,1,3,3},
            phi5[8]  = {1,1,1,1,3,3,3,3},
            phi6[8]  = {1,1,1,1,3,3,3,3},
            phi7[8]  = {1,1,1,1,3,3,3,3},
            phi8[1]  = {0},
            rec[2]   = {0,2};

static double d2_init=0.0, d3_init=0.0;
static shape    rb180, rb180_cg, cosed;
            
void pulsesequence()
{
/* DECLARE VARIABLES */

 char       fsat[MAXSTR],
	    fscuba[MAXSTR],
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
             BigTC1,       /* Carbon constant time period2 < 1/4Jcc to account for relaxation */ 
             pwN,          /* PW90 for 15N pulse @ pwNlvl           */
             pwC,          /* PW90 for c nucleus @ pwClvl         */
             pwcrb180,      /* PW180 for C 180 reburp @ rfrb */
             pwClvl,        /* power level for 13C pulses on dec1  */
             compC, compH,  /* compression factors for H1 and C13 amps */
	     rfrb,       /* power level for 13C reburp pulse     */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             tofps,        /* tof for presat                       */ 

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
             bw, ofs, ppm,

             pwd1,
             dpwr3_D,
             pwd,
             tpwrs,
             pwHs, 
             dof_me,
             
             tof_dtt,
             tpwrs1,
             pwHs1,

             dpwrsed,
             pwsed,
             dressed,
              
             rfrb_cg,
             pwrb_cg; 
             
   
/* LOAD VARIABLES */

  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("fscuba",fscuba);

  getstr("C_flg",C_flg);
  getstr("dtt_flg",dtt_flg); 

  tofps  = getval("tofps");
  taua   = getval("taua"); 
  taub   = getval("taub"); 
  BigTC  = getval("BigTC");
  BigTC1 = getval("BigTC1");
  pwC = getval("pwC");
  pwcrb180 = getval("pwcrb180");
  pwN = getval("pwN");
  tpwr = getval("tpwr");
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

  pwd1 = getval("pwd1");
  dpwr3_D = getval("dpwr3_D");
  pwd = getval("pwd");
  pwHs = getval("pwHs");
  dof_me = getval("dof_me");

  pwHs1 = pwHs; 
  tpwrs=-16.0; tpwrs1=tpwrs;
  tof_dtt = getval("tof_dtt");

  dpwrsed = -16;
  pwsed = 1000.0;
  dressed = 90.0;
  pwrb_cg = 0.0;  
  setautocal();                      /* activate auto-calibration */   

  if(FIRST_FID)                                         /* make shapes */
  {
    ppm = getval("dfrq"); 
    bw = 80.0*ppm;  
    rb180 = pbox_make("rb180P", "reburp", bw, 0.0, compC*pwC, pwClvl);
    bw = 8.125*ppm;  ofs = -24.0*ppm;
    rb180_cg = pbox_make("rb180_cgP", "reburp", bw, ofs, compC*pwC, pwClvl);
    bw = 20.0*ppm;  ofs = 136.0*ppm;
    cosed = pbox("COsedP", CODEC, CODECps, dfrq, compC*pwC, pwClvl);
    if(taua < (gt4+106e-6+pwHs)) printf("gt4 or pwHs may be too long! ");
    if(taub < rb180_cg.pw) printf("rb180_cgP pulse may be too long! ");
  }
  pwcrb180 = rb180.pw;   rfrb = rb180.pwrf;             /* set up parameters */
  pwrb_cg = rb180_cg.pw; rfrb_cg = rb180_cg.pwrf;       /* set up parameters */
  tpwrs = tpwr - 20.0*log10(pwHs/((compH*pw)*1.69));    /* sinc=1.69xrect */
  tpwrs = (int) (tpwrs); tpwrs1=tpwrs;              
  dpwrsed = cosed.pwr; pwsed = 1.0/cosed.dmf; dressed = cosed.dres;

/* LOAD PHASE TABLE */

  settable(t1,2,phi1);
  settable(t2,4,phi2);
  settable(t3,4,phi3);
  settable(t4,4,phi4);
  settable(t5,8,phi5);
  settable(t6,8,phi6);
  settable(t7,8,phi7);
  settable(t8,1,phi8);
  settable(t9,2,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */

    if( BigTC - 0.5*(ni2-1)*1/(sw2) - WFG_STOP_DELAY - POWER_DELAY 
              - 4.0e-6
              < 0.2e-6 )
    {
        printf(" ni2 is too big\n");
        psg_abort(1);
    }


    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' || dm2[D] == 'y'))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnnn' ");
        psg_abort(1);
    }

    if( satpwr > 6 )
    {
        printf("SATPWR too large !!!  ");
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

    if(dpwr3_D > 49)
    {
       printf("dpwr3_D is too high; < 50\n");
       psg_abort(1);
    }

   if(d1 < 1)
    {
       printf("d1 must be > 1\n");
       psg_abort(1);
    }

   if(dpwrsed > 48)
   {
       printf("dpwrsed must be less than 49\n");
       psg_abort(1);
   }

    if(  gt0 > 5.0e-3 || gt1 > 5.0e-3  || gt2 > 5.0e-3 ||
         gt3 > 5.0e-3 || gt4 > 5.0e-3  )
    {  printf(" all values of gti must be < 5.0e-3\n");
        psg_abort(1);
    }

   if(ix==1) {
     printf("make sure that BigTC1 is set properly for your application\n");
     printf("7 ms, neglecting relaxation \n");
   }

/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2) {
      tsadd(t1,1,4);
      tsadd(t2,1,4);
      tsadd(t3,1,4);
      tsadd(t4,1,4);
    }

    if (phase2 == 2)
      tsadd(t8,1,4);

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    tau1 = tau1 - 2.0*pw - 4.0/PI*pwC - POWER_DELAY - 2.0e-6 - PRG_START_DELAY
           - PRG_STOP_DELAY - POWER_DELAY - 2.0e-6;

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
      tsadd(t9,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t8,2,4);  
      tsadd(t9,2,4);    
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(satpwr);      /* Set transmitter power for 1H presaturation */
   decpower(pwClvl);        /* Set Dec1 power for hard 13C pulses         */
   dec2power(pwNlvl);      /* Set Dec2 to low power       */

/* Presaturation Period */

status(B);
   if (fsat[0] == 'y')
   {
        obsoffset(tofps);
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
   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   obsoffset(tof);
   txphase(t1);
   decphase(zero);
   dec2phase(zero);
   delay(1.0e-5);

/* Begin Pulses */

status(C);

   decoffset(dof_me);

   lk_hold();

   rcvroff();
   delay(20.0e-6);

/* ensure that magnetization originates on 1H and not 13C */

   if(dtt_flg[A] == 'y') {
     obsoffset(tof_dtt);
     obspower(tpwrs1);
     shaped_pulse("H2Osinc",pwHs1,zero,10.0e-6,0.0);
     obspower(tpwr);

     obsoffset(tof); 
   }
 
   decrgpulse(pwC,zero,0.0,0.0);
 
   zgradpulse(gzlvl0,gt0);
   delay(gstab);

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   zgradpulse(gzlvl1,gt1);
   delay(gstab);

   delay(taua - gt1 -gstab); 

   simpulse(2.0*pw,2.0*pwC,zero,zero,0.0,0.0);
   txphase(one);

   delay(taua - gt1 - gstab); 
   	
   zgradpulse(gzlvl1,gt1);
   delay(gstab);


   rgpulse(pw,one,0.0,0.0);

   /* shaped_pulse */
   obspower(tpwrs);
   shaped_pulse("H2Osinc",pwHs,zero,2.0e-6,0.0);
   obspower(tpwr);
   /* shaped_pulse */

   decoffset(dof);  /* jump 13C to 40 ppm */

   zgradpulse(gzlvl2,gt2);
   delay(gstab);

   decrgpulse(pwC,t1,4.0e-6,0.0); decphase(zero); 

   initval(1.0,v3);
   decstepsize(decstep1);
   dcplrphase(v3);

   decpwrf(rfrb);
   delay(BigTC - POWER_DELAY - WFG_START_DELAY);

   decshaped_pulse(rb180.name,pwcrb180,zero,0.0,0.0);
   dcplrphase(zero);
   decphase(t2);

   decpwrf(4095.0);
   delay(BigTC - WFG_STOP_DELAY - POWER_DELAY);

   decrgpulse(pwC,t2,0.0,0.0);
   decphase(zero);

   /* turn on 2H decoupling */
   dec3phase(one);
   dec3power(dpwr3); 
   dec3rgpulse(pwd1,one,4.0e-6,0.0); 
   dec3phase(zero);
   dec3unblank();
   dec3power(dpwr3_D);
   dec3prgon(dseq3,pwd,dres3);
   dec3on();
   /* turn on 2H decoupling */

   initval(1.0,v3);
   decstepsize(decstep1);
   dcplrphase(v3);

   decpwrf(rfrb);

   delay(BigTC1 - POWER_DELAY - 4.0e-6 - pwd1
         - POWER_DELAY - PRG_START_DELAY - POWER_DELAY - WFG_START_DELAY);

   decshaped_pulse(rb180.name,pwcrb180,zero,0.0,0.0);
   dcplrphase(zero);
   decphase(t3);

   decpwrf(4095.0);
   delay(BigTC1 - WFG_STOP_DELAY - POWER_DELAY);

   decrgpulse(pwC,t3,0.0,0.0);
   decpwrf(rfrb_cg); decphase(zero);

   if(taub > pwrb_cg)
     delay(taub/2.0 - pwrb_cg/2.0 - POWER_DELAY - WFG_START_DELAY);
   decshaped_pulse(rb180_cg.name,pwrb_cg,zero,0.0,0.0);
   decpwrf(rfrb);
   
   if(taub > pwrb_cg)
     delay(taub/2.0 - pwrb_cg/2.0 - WFG_STOP_DELAY - POWER_DELAY - SAPS_DELAY
         - 2.0e-6 - WFG_START_DELAY);

   initval(1.0,v3);
   decstepsize(decstep1);
   dcplrphase(v3);

   decshaped_pulse(rb180.name,pwcrb180,zero,2.0e-6,0.0);
   dcplrphase(zero);

   decpwrf(rfrb_cg); decphase(zero);

   if(taub > pwrb_cg)
     delay(taub/2.0 - pwrb_cg/2.0 - WFG_STOP_DELAY - SAPS_DELAY 
                  - POWER_DELAY - WFG_START_DELAY);

   decshaped_pulse(rb180_cg.name,pwrb_cg,zero,0.0,0.0);
   decpwrf(4095.0); decphase(t4);

   if(taub > pwrb_cg)
     delay(taub/2.0 - pwrb_cg/2.0 - WFG_STOP_DELAY - POWER_DELAY);

   decrgpulse(pwC,t4,0.0,0.0);

   if(C_flg[A] == 'n') {

   decpower(dpwrsed); decunblank(); decphase(zero); delay(2.0e-6);
   decprgon(cosed.name,pwsed,dressed);
   decon();
  
   delay(tau1);
   rgpulse(2.0*pw,zero,0.0,0.0);
   delay(tau1);

   decoff();
   decprgoff();
   decblank();
   decpower(pwClvl);
   }

   else 
    simpulse(2.0*pw,2.0*pwC,zero,zero,4.0e-6,4.0e-6);

   decrgpulse(pwC,t5,2.0e-6,0.0);
   decpwrf(rfrb_cg); decphase(zero);

   if(taub > pwrb_cg)
     delay(taub/2.0 - pwrb_cg/2.0 - POWER_DELAY - WFG_START_DELAY);
   decshaped_pulse(rb180_cg.name,pwrb_cg,zero,0.0,0.0);
   decpwrf(rfrb);

   if(taub > pwrb_cg)
     delay(taub/2.0 - pwrb_cg/2.0 - WFG_STOP_DELAY - POWER_DELAY - SAPS_DELAY
         - 2.0e-6 - WFG_START_DELAY);

   initval(1.0,v3);
   decstepsize(decstep1);
   dcplrphase(v3);

   decshaped_pulse(rb180.name,pwcrb180,zero,2.0e-6,0.0);
   dcplrphase(zero);

   decpwrf(rfrb_cg); decphase(zero);

   if(taub > pwrb_cg)
     delay(taub/2.0 - pwrb_cg/2.0 - WFG_STOP_DELAY - SAPS_DELAY 
                  - POWER_DELAY - WFG_START_DELAY);

   decshaped_pulse(rb180_cg.name,pwrb_cg,zero,0.0,0.0);
   decpwrf(4095.0); decphase(t6);

   if(taub > pwrb_cg)
     delay(taub/2.0 - pwrb_cg/2.0 - WFG_STOP_DELAY - POWER_DELAY);

   decrgpulse(pwC,t6,0.0,0.0);
   decphase(zero);

   initval(1.0,v3);
   decstepsize(decstep1);
   dcplrphase(v3);

   decpwrf(rfrb);
   delay(BigTC1 - POWER_DELAY - WFG_START_DELAY);

   decshaped_pulse(rb180.name,pwcrb180,zero,0.0,0.0);
   dcplrphase(zero);
   decphase(t7);

   decpwrf(4095.0);
   delay(BigTC1 - WFG_STOP_DELAY - POWER_DELAY
          - PRG_STOP_DELAY - POWER_DELAY - 4.0e-6 - pwd1);

   /* 2H decoupling off */
   dec3off();
   dec3prgoff();
   dec3blank();
   dec3power(dpwr3);
   dec3rgpulse(pwd1,three,4.0e-6,0.0);
   /* 2H decoupling off */

   decrgpulse(pwC,t7,0.0,0.0);
   decphase(zero);

   delay(tau2);
   rgpulse(2.0*pw,zero,0.0,0.0);

   initval(1.0,v3);
   decstepsize(decstep1);
   dcplrphase(v3);

   decpwrf(rfrb);
   delay(BigTC - 2.0*pw - POWER_DELAY - WFG_START_DELAY);

   decshaped_pulse(rb180.name,pwcrb180,zero,0.0,0.0);
   dcplrphase(zero);
   decphase(t8);
   decpwrf(4095.0);

   delay(BigTC - tau2 - WFG_STOP_DELAY - POWER_DELAY - 4.0e-6);

   decrgpulse(pwC,t8,4.0e-6,0.0);


   decoffset(dof_me);

   zgradpulse(gzlvl3,gt3);
   delay(gstab);

   lk_sample();

   /* shaped_pulse */
   obspower(tpwrs);
   shaped_pulse("H2Osinc",pwHs,two,2.0e-6,0.0);
   obspower(tpwr);
   /* shaped_pulse */

   rgpulse(pw,zero,4.0e-6,0.0);

   zgradpulse(gzlvl4,gt4);
   delay(gstab);

   delay(taua - gt4 -gstab 
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

   zgradpulse(gzlvl4,gt4);
   delay(gstab);
 
   delay(taua - POWER_DELAY - WFG_START_DELAY
         - pwHs - WFG_STOP_DELAY - POWER_DELAY 
         - gt4 - gstab - 2.0*POWER_DELAY);

   decpower(dpwr);  /* Set power for decoupling */
   dec2power(dpwr2);

/*   rcvron();  */          /* Turn on receiver to warm up before acq */ 

/* BEGIN ACQUISITION */

status(D);
   setreceiver(t9);

}
