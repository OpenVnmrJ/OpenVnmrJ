/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* CN4Dnoesy_trosyA.c

      This pulse sequence will allow one to perform the following experiment:
       4D 13C 15N edited noesy, with TROSY detection in F3,F4. 

        F2,F3 are 15N dimensions, F1,F4 are 1H dimensions

      Set C_flg to y to eliminate evolution during t2 (during the running of some test 2D sets
       before the 4D.

      Written by L.E.Kay on January 21, 2003 
      Place dof at 18 ppm in 13C. The adiabatic pulse must be centered at 117 ppm.

      modified from Kay's cn4Dnoesy_pfg_sel_trosy_lek_800.c, GG Varian July 2004

*/

#include <standard.h>
#include "Pbox_bio.h"

static double d2_init = 0.0, d3_init=0.0, d4_init=0.0;
static double   H1ofs=4.7, C13ofs=18.0, N15ofs=120.0, H2ofs=3.2;
static shape H2Osel,C13adiab;
static int phi1[1] = {0},
           phi2[2] = {0,2},
           phi4[1] = {1}, 
           phi6[1] = {3},
           phi7[1] = {1},
           phi8[1] = {3},
           rec[2]  = {0,2};

pulsesequence()
{
/* DECLARE VARIABLES */

 char        fscuba[MAXSTR],f1180[MAXSTR],f2180[MAXSTR],fsat[MAXSTR],
             shape[MAXSTR],sh_ad[MAXSTR],f3180[MAXSTR],
             C_flg[MAXSTR];

 int	     phase,
             phase2,
             phase3,
             t1_counter,
             t2_counter,
             t3_counter, icosel;

 double      hscuba,                /* length of 1/2 scuba delay */
             pwx2,                  /* PW90 for X-nuc            */
             tsatpwr,               /* low power level for presat*/
             dhpwr2,                /* power level for X hard pulses */
             jch,                   /* coupling for CH           */
             jnh,                   /* coupling for NH           */
             taunh,                 /* delay = 1/(2jnh)          */
             tauch,                 /* delay = 1/(2jch)          */
	     tau1,	      	    /* t1/2  H */
	     tau2,	      	    /* t2/2 N */
	     tau3,	      	    /* t3/2 N */
	     sw1,                  /* spectral width in 1H dimension */
             sw2,                  /* spectral width in 15N dimension */
             sw3,                  /* spectral width in 15N dimension */
             MIX,                  /* Total Mixing time for noesy portion */
             pw_sl,                /* selective 2ms pulse on water */
             tpwrsl,               /* power level for pw_sl   */ 
             ppm,nst,pws,bw,ofs,   /* used by Pbox */
             pwN,pwNlvl,compH,compC,pwC,pwClvl,

             gstab,

             d_ad,                 /* C high power for adiabatic pulses */
             pwc_ad,               /* C 90 pulse width   */

             tof_me,
             dhpwr,
             pwc,

             BigT,
             BigT1,

             gzlvl0,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl9,
             gzlvl10,
             gzlvl11,
             gzlvl12,

             gt0,
             gt1,
             gt2,
             gt3,
             gt4,
             gt9,
             gt10,
             gt11,
             gt12;

/* LOAD VARIABLES */

  jnh = getval("jnh");
  jch = getval("jch");
  dhpwr2 = getval("dhpwr2"); 
  pwx2 = getval("pwx2");
  tsatpwr = getval("tsatpwr");
  hscuba = getval("hscuba");
  phase = (int) (getval("phase") + 0.5);
  phase2 = (int) (getval("phase2") + 0.5);
  phase3 = (int) (getval("phase3") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  sw3 = getval("sw3");
  MIX  = getval("MIX");
  pw_sl = getval("pw_sl");
  tpwrsl = getval("tpwrsl");

  pwc_ad = getval("pwc_ad");
  d_ad = getval("d_ad");

  dhpwr = getval("dhpwr");
  pwc = getval("pwc");
  tof_me = getval("tof_me");

  BigT = getval("BigT");
  BigT1 = getval("BigT1");

  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl9 = getval("gzlvl9");
  gzlvl10 = getval("gzlvl10");
  gzlvl11 = getval("gzlvl11");
  gzlvl12 = getval("gzlvl12");

  gt0 = getval("gt0");
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt9 = getval("gt9");
  gt10 = getval("gt10");
  gt11 = getval("gt11");
  gt12 = getval("gt12");

  getstr("fscuba",fscuba); 
  getstr("fsat",fsat); 
  getstr("f1180",f1180); 
  getstr("f2180",f2180); 
  getstr("f3180",f3180); 
  getstr("shape",shape);

  getstr("C_flg",C_flg);
  getstr("sh_ad",sh_ad);
  gstab  = getval("gstab");

  pwN=getval("pwN"); pwNlvl=getval("pwN");
  pwC=getval("pwC"); pwClvl=getval("pwClvl");

/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' || dm[D] =='y'))
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' || dm2[D] == 'y'))
	{
	printf("incorrect Dec2 decoupler flags!  ");
	psg_abort(1);
    } 

    if( tsatpwr > 8 )
    {
	printf("tsatpwr too large !!!  ");
	psg_abort(1);
    }

    if( dpwr > -16 )
    {
	printf("No C decoupling ");
	psg_abort(1);
    }

    if( dpwr2 > -16 )
    {
	printf("No N decoupling ");
	psg_abort(1);
    }

    if( gt0 > 3e-3 || gt1 > 3e-3 || gt2 > 3e-3 || gt3 > 3e-3 || gt4 > 3e-3 
        || gt9 > 3e-3 || gt10 > 3e-3 || gt11 > 3e-3  
        || gt12 > 3e-3)
    {
        printf("gradients are on for too long !!! ");
        psg_abort(1);
    } 

    if(d_ad > 58) {
        printf("chirp power is too high \n");
        psg_abort(1);
    }

    if(pwc_ad > 0.5e-3) {
        printf("pw_chirp is too long; < 0.5 ms\n");
        psg_abort(1);
    } 

    if(ix==1)
    {
     if(f1180[A] != 'n' || f2180[A] !='y' || f3180[A] != 'n')
      {
        printf("f1180 should be n, f2180 y, and f3180 should be n\n");
      }
    }


/* LOAD VARIABLES */

  settable(t1, 1,  phi1);
  settable(t2, 2,  phi2);
  settable(t4, 1,  phi4);
  settable(t6, 1,  phi6);
  settable(t7, 1,  phi7);
  settable(t8, 1,  phi8);
  settable(t10,2,  rec);

/* INITIALIZE VARIABLES */

  taunh = 1/(4.0*jnh);
  tauch = 1/(2.0*jch);

/* Phase incrementation for hypercomplex data */

  if( phase == 2 ) {
        tsadd(t1, 1, 4);
   }

   if ( phase2 == 2 ) {    /* Hypercomplex in t2 */
        tsadd(t2, 1, 4);
   }

   if ( phase3 == 2 )     /* Hypercomplex in t3 */
   {
        tsadd(t6, 2, 4);
        tsadd(t7, 2, 4);
        tsadd(t8, 2, 4);
        tsadd(t10, 2, 4);
        icosel = -1;
   }
   else
       icosel = 1;

/* calculate modifications to phases based on current t2/t3 values
   to achieve States-TPPI acquisition */

   if(ix==1)
     d4_init = d4;
     t3_counter = (int) ( (d4-d4_init)*sw3 + 0.5);
     if(t3_counter %2) {
       tsadd(t4,2,4);
       tsadd(t10,2,4);
     }

   if(ix==1)
     d3_init = d3;
     t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5);
     if(t2_counter %2) {
       tsadd(t2,2,4);
       tsadd(t10,2,4); 
     }

   if(ix==1)
     d2_init = d2;
     t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
     if(t1_counter %2) {
   /*
       tsadd(t1,2,4);
       tsadd(t10,2,4); 
  */
     }

/* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */

     tau1 = d2;
     if(f1180[A] == 'y') {
            tau1 += 1/(2.0*sw1);
     }

     tau1 = tau1/2.0;
     if(tau1 < 0.2e-6) tau1 = 0.2e-6;
     

/* set up so that get (-90,180) phase corrects in F2 if f2180 flag is y */

   tau2 = d3;
   if(f2180[A] == 'y')  
      tau2 += (1.0/(2.0*sw2) - (4.0/PI)*pwc - 2.0*pw);
   if(f2180[A] == 'n')  
      tau2 = (tau2 - (4.0/PI)*pwc - 2.0*pw);

   tau2 = tau2/2.0;

/* set up so that get (-90,180) phase corrects in F3 if f3180 flag is y */

   tau3 = d4;
   if(f3180[A] == 'y')  tau3 += ( 1.0/(2.0*sw3) );

   tau3 = tau3/2.0;
   if( tau3 < 0.2e-6) tau3 = 2.0e-7; 


   setautocal();                        /* activate auto-calibration flags */ 
        
      if (autocal[0] != 'n') 
        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
      {
        strcpy(shape,"H2Osel");
        strcpy(sh_ad,"C13adiab");
        if(FIRST_FID)                                            /* call Pbox */
        {
          compC=getval("compC");
          compH=getval("compH");
          ppm = getval("dfrq"); ofs = 0.0;   pws = 0.0005; /*0.5 ms pulse */
          bw = 200.0*ppm;       nst = 1000;          /* nst - number of steps */
          C13adiab = pbox_makeA("C13adiab", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
          H2Osel = pbox_Rsh("H2Osel", "sinc90", pw_sl, 0.0, compH*pw, tpwr);
          ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
        }
        pw_sl = H2Osel.pw; tpwrsl = H2Osel.pwr-1.0;  /* 1dB correction applied */
        d_ad = C13adiab.pwr; pwc_ad = C13adiab.pw;
        pwx2=pwN; dhpwr2=pwNlvl; pwc=pwC; dhpwr2=pwClvl;
      }





/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);            /* Set power for presaturation  */
   decpower(dhpwr);              /* Set decoupler1 power to dhpwr */
   dec2power(dhpwr2);            /* Set decoupler2 power to dhpwr2 */

/* Presaturation Period */

 if(fsat[0] == 'y')
{
  txphase(zero);
  rgpulse(d1,zero,2.0e-6,2.0e-6);  /* presat */
  obspower(tpwr);                /* Set power for hard pulses  */

    if (fscuba[0] == 'y')            /* Scuba pulse sequence */
    {  
      hsdelay(hscuba);

      rgpulse(pw,zero,1.0e-6,0.0);	/* 90x180y90x */
      rgpulse(2*pw,one,1.0e-6,0.0);
      rgpulse(pw,zero,1.0e-6,0.0);
 
      txphase(zero);
      delay(hscuba);        
    }
}

else  {
 obspower(tpwr);                /* Set power for hard pulses  */
 delay(d1);
}

status(B);

   obsoffset(tof_me);

   rcvroff();
   delay(20.0e-6);

  /* eliminate all magnetization originating from 15N */

  decrgpulse(pwc,zero,0.0,0.0);
  dec2rgpulse(pwx2,zero,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl0,gt0);
  delay(150.0e-6);

  initval(7.0,v7);
  obsstepsize(45.0);
  xmtrphase(v7);

  rgpulse(pw,t1,4.0e-6,0.0);
  xmtrphase(zero);
  txphase(zero); decphase(t2); dec2phase(zero);

  delay(2.0e-6);
  zgradpulse(gzlvl1,gt1);
  delay(2.0e-6);

  delay(tauch - gt1 - 4.0e-6 - SAPS_DELAY);            

  decrgpulse(pwc,t2,0.0,0.0); decphase(zero);

  if(C_flg[A] == 'n') {
     delay(tau2);
     rgpulse(2.0*pw,zero,0.0,0.0);
     delay(tau2);
  }

  else
    simpulse(2.0*pw,2.0*pwc,zero,zero,2.0e-6,2.0e-6);

  decrgpulse(pwc,zero,0.0,0.0);

  delay(tau1 + tauch - gt1 - 102.0e-6 - 4.0*pwc - 4.0e-6);

  delay(2.0e-6);
  zgradpulse(gzlvl1,gt1);
  delay(100.0e-6);

  decrgpulse(pwc,zero,0.0,0.0);
  decrgpulse(2.0*pwc,one,2.0e-6,0.0);
  decrgpulse(pwc,zero,2.0e-6,0.0);

  delay(tau1);

  rgpulse(pw,zero,0.0,0.0);

  decpower(d_ad);

  delay(2.0e-6);
  zgradpulse(gzlvl2,gt2/2.0);
  delay(500.0e-6);

  obsoffset(tof);  /* jump 1H carrier to water */ 

  delay(MIX - 1.5*gt2 - 2.0*502.0e-6 - 2.0*pwx2); 

  dec2rgpulse(pwx2,zero,0.0,0.0); 

  delay(2.0e-6);
  zgradpulse(gzlvl2,gt2);
  delay(500.0e-6);

  rgpulse(pw,two,2.0e-6,0.0);

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,zero,2.0e-6,0.0);
  delay(2.0e-6); txphase(zero);
  obspower(tpwr);
  /* shaped pulse */

  delay(2.0e-6);
  zgradpulse(gzlvl3,gt3);
  delay(2.0e-6);

  delay(taunh  
      - POWER_DELAY - 2.0e-6 - WFG_START_DELAY
      - pw_sl - WFG_STOP_DELAY - 2.0e-6 - POWER_DELAY 
      - gt3 - 4.0e-6); 

  sim3pulse(2*pw,0.0e-6,2*pwx2,zero,zero,zero,0.0,0.0);

  delay(taunh 
      - gt3 - 202.0e-6
      - POWER_DELAY - 2.0e-6 - WFG_START_DELAY
      - pw_sl - WFG_STOP_DELAY - 4.0e-6 - POWER_DELAY); 

  delay(2.0e-6);
  zgradpulse(gzlvl3,gt3);
  delay(200.0e-6);

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,one,2.0e-6,0.0);
  obspower(tpwr);
  delay(4.0e-6); txphase(one);
  /* shaped pulse */

  rgpulse(pw,one,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl4,gt4);
  delay(200.0e-6);

  txphase(t6);

  dec2rgpulse(pwx2,t4,4.0e-6,0.0);
  dec2phase(zero); 

  if((tau3 - WFG_START_DELAY - 0.5*pwc_ad) < 0.2e-6) {

  delay(tau3);
  delay(tau3);

  }

  else {

  delay(tau3 - WFG_START_DELAY - 0.5*pwc_ad);

  decshaped_pulse(sh_ad,pwc_ad,zero,0.0,0.0); 

  delay(tau3 - 0.5*pwc_ad - WFG_STOP_DELAY);

  }

  delay(2.0e-6);
  zgradpulse(-1.0*gzlvl11,gt11);
  delay(100.0e-6);

  delay(BigT - 4.0/PI*pwx2 + pw - 2.0*GRADIENT_DELAY - gt11
        - 102.0e-6);

  dec2rgpulse(2.0*pwx2,zero,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl11,gt11);
  delay(100.0e-6);

  delay(BigT - 2.0*GRADIENT_DELAY - gt11
        - 102.0e-6);

  rgpulse(pw,t6,0.0,0.0);

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,t7,2.0e-6,0.0);
  delay(2.0e-6); txphase(zero);
  obspower(tpwr);
  /* shaped pulse */

  delay(2.0e-6);
  zgradpulse(gzlvl9,gt9);
  delay(100.0e-6);

  delay(taunh 
      - POWER_DELAY - 2.0e-6 - WFG_START_DELAY
      - pw_sl - WFG_STOP_DELAY - 2.0e-6 - POWER_DELAY 
      - gt9 - 102.0e-6);

  sim3pulse(2.0*pw,0.0,2.0*pwx2,zero,zero,zero,0.0,0.0);
  dec2phase(t8);

  delay(2.0e-6);
  zgradpulse(gzlvl9,gt9);
  delay(100.0e-6);

  delay(taunh 
      - gt9 - 102.0e-6
      - POWER_DELAY - 2.0e-6 - WFG_START_DELAY
      - pw_sl - WFG_STOP_DELAY - 4.0e-6 - POWER_DELAY); 

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,zero,2.0e-6,0.0);
  obspower(tpwr);
  delay(4.0e-6); txphase(zero);
  /* shaped pulse */

  sim3pulse(pw,0.0,pwx2,zero,zero,t8,0.0,0.0);

  dec2phase(zero);

  delay(2.0e-6);
  zgradpulse(gzlvl10,gt10);
  delay(2.0e-6);

  delay(taunh - gt10 - 4.0e-6);

  sim3pulse(2*pw,0.0e-6,2*pwx2,zero,zero,zero,2.0e-6,2.0e-6);  

  delay(2.0e-6);
  zgradpulse(gzlvl10,gt10);
  delay(100.0e-6);

  delay(taunh
        - gt10 - 102.0e-6
        + 2.0/PI*pw - pwx2 + 0.5*(pwx2-pw));

  dec2rgpulse(pwx2,zero,0.0,0.0);

  dec2power(dpwr2);  /* Very Important */
  decpower(dpwr);

  delay(BigT1 - 2.0*POWER_DELAY);

  rgpulse(2.0*pw,zero,0.0,0.0); 

  delay(2.0e-6);
  zgradpulse(-1.0*icosel*gzlvl12,gt12);
  delay(50.0e-6);

  delay(BigT1 - 2.0*GRADIENT_DELAY - 52.0e-6 - gt12);

/* acquire data */

status(C);
     setreceiver(t10);
}
