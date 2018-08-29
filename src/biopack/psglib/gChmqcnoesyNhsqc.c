/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*    gChmqcnoesyNhsqc.c

DESCRIPTION and INSTRUCTION:

    This pulse sequence will allow one to perform the following experiment:
    4D CN HMQC-NOESY-HSQC
    ni(t1) -->  1H
    ni2(t2)--> 13C
    ni3(t3)--> 15N

    OFFSET POSITION:
        tof =   ~4.75 ppm (1H on water).
        tofh =  ~3 ppm  (center of aliphatic protons).
        dof =   35 ppm (Center of aliphatic carbon).
        dofcaco = 120 ppm (Center of Ca and CO).
        dof2 =  120 ppm (15N region).

    CHOICE OF FLAGS:
    mag_flg = y -> Using magic angle gradients (Triax PFG probe is required).

    PARAMETERS RELATED TO 13C DECOUPLING:
       the shape codec.DEC is created by the setupwurst macro in BioPack.
       the proper decoupling parameters for codec.DEC are obtained by the setwurstparams macro.
       and stored in dmm,dpwr,dmf,dseq and dres. Use dm='nyn' for CO decoupling in t2.

       CA/CO decoupling in t3 is achieved with a stC200 pulse

    Written by Weixing Zhang, April 2001
    St. Jude Children's Research Hospital
    Memphis, TN 38134
    USA
    (901)495-3169
    Modified on April 26, 2002 for submission to BioPack
    Modified for status-control of CO decoupling (GG, Palo Alto May02')



*/

#include <standard.h>
extern int dps_flag;

static double d2_init = 0.0, d3_init = 0.0, d4_init=0.0;

static  int phi1[4] = {0,0,2,2},
        phi2[2] = {0,2},
        phi3[1] = {0},
        phi4[8]  = {0,0,0,0,1,1,1,1},
        phi5[1] = {0},
        rec_1[8] = {0,2,2,0,2,0,0,2};

pulsesequence()
{
char    f1180[MAXSTR],
        f2180[MAXSTR],
	f3180[MAXSTR],
        mag_flg[MAXSTR];

 int	phase, icosel,
	t1_counter,
	t2_counter,
	t3_counter;

 double	pwClvl,
        pwC,
        rf0 = 4095,
        rfst,
        compC = getval("compC"),
        tpwrs,
        pwHs = getval("pwHs"),
        compH = getval("compH"),

	pwNlvl,
	pwN,

	tau1,
	tau2,
	tau3,
	tauch,   /* 3.4 ms  */
	taunh,   /* 2.4 ms  */
	mix, ni2, ni3,
        tofh = getval("tofh"),
        dofcaco = getval("dofcaco"),  /* ~120 ppm  */

        gt0, gzlvl0,
 	gt1,gzlvl1,
	gzlvl2,
        gzcal = getval("gzcal"),
        gstab = getval("gstab"),
	gt3,gzlvl3,
	gt4,gzlvl4,
	gt5,gzlvl5,
	gt6,gzlvl6,
	gt7,gzlvl7,
	gt8, gzlvl8,
        gt9, gzlvl9;

/* LOAD VARIABLES */

  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("f3180",f3180);
  getstr("mag_flg", mag_flg);

  pwClvl = getval("pwClvl");
  pwC = getval("pwC");

  pwNlvl = getval("pwNlvl");
  pwN = getval("pwN");

  mix = getval("mix");
  tauch = getval("tauch");
  taunh = getval("taunh");
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  sw3 = getval("sw3");
  ni2= getval("ni2");
  ni3= getval("ni3");
  phase = (int) (getval("phase") + 0.5);
  phase2 = (int) (getval("phase2") + 0.5);
  phase3 = (int) (getval("phase3") + 0.5);

  gt0 = getval("gt0");
  gt1 = getval("gt1");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  gt9 = getval("gt9");
  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");
  gzlvl9 = getval("gzlvl9");


/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
  rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
  rfst = (int) (rfst + 0.5);

  if (1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
  { 
     text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
     (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) );
     psg_abort(1);
  }

/* selective H20 one-lobe sinc pulse */
   tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));
   tpwrs = (int)(tpwrs+0.5);

/* check validity of parameter range */

   if((dm[A] == 'y' || dm[C] == 'y'))
   {
      printf("incorrect Dec1 decoupler flags!  ");
      psg_abort(1);
   }

   if((dm2[A] == 'y' || dm2[B] == 'y'))
   {
      printf("incorrect Dec2 decoupler flags!  ");
      psg_abort(1);
   }
  
   if ((dpwr > 50) || (dpwr2 > 50))
   {
      printf("don't fry the probe, dpwr too high!  ");
      psg_abort(1);
   }

/* Load phase cycling variables */

   settable(t1, 4, phi1);
   settable(t2, 2, phi2);
   settable(t3, 1, phi3);
   settable(t4, 8, phi4);
   settable(t5, 1, phi5);

   settable(t11, 8, rec_1);

/* Phase incrementation for hypercomplex data */

   if ( phase == 2 )	
   {
      tsadd(t1,1,4);
   }
   if ( phase2 == 2 )	
   { 
      tsadd(t2,1,4);
   }
   if ( phase3 == 1 )	
   { 
      tsadd(t5,2,4); 
      icosel = 1; 
   }
   else icosel = -1; 

/* calculate modification to phases based on current t2 values
   to achieve STATES-TPPI acquisition */

   if(ix == 1) d2_init = d2;
   t1_counter = (int)((d2-d2_init)*sw1 + 0.5);
   if(t1_counter % 2)
   {
      tsadd(t1,2,4);
      tsadd(t11,2,4);
   }

   if(ix == 1) d3_init = d3;
   t2_counter = (int)((d3-d3_init)*sw2 + 0.5);
   if(t2_counter % 2)
   {
      tsadd(t2,2,4);
      tsadd(t11,2,4);
   }

   if(ix == 1) d4_init = d4;
   t3_counter = (int)((d4-d4_init)*sw3 + 0.5);
   if(t3_counter % 2)
   {
      tsadd(t3,2,4);
      tsadd(t11,2,4);
   }

/* set up so that get (90, -180) phase corrects in F1 if f1180 flag is 'y' */

   tau1 = d2;
   if(f1180[A] == 'y')
   {
      tau1 += (1.0/(2.0*sw1));
   }
   if (tau1 < 0.2e-6) tau1 = 0.0;
   tau1 = tau1/2.0;

/* set up so that get (90, -180) phase corrects in F2 if f2180 flag is 'y' */

   tau2 = d3 - (4.0*pwC/PI + 2.0*pw + 2.0e-6);
   if (dm[B] == 'y')
   {
      tau2 = tau2 - (2.0*POWER_DELAY + PRG_START_DELAY + PRG_STOP_DELAY);
   }
   if(f2180[A] == 'y')
   {
      tau2 += (1.0/(2.0*sw2));
   }
   if (tau2 < 0.2e-6) tau2 = 0.0;
   tau2 = tau2/2.0;

/* set up so that get (90, -180) phase corrects in F3 if f3180 flag is 'y' */

   tau3 = d4;
   if(f3180[A] == 'y')
   {
      tau3 += (1.0/(2.0*sw3));
   }
   if (tau3 < 0.2e-6) tau3 = 0.0;
   tau3 = tau3/2.0;

   initval(315.0, v7);
   obsstepsize(1.0);

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   delay(10.0e-6);
   obspower(tpwr);
   decpower(pwClvl);
   decpwrf(rf0);
   dec2power(pwNlvl);
   obsoffset(tofh);
   decoffset(dof);
   dec2offset(dof2);
   txphase(t1);
   xmtrphase(v7);
   delay(d1);

   if (gt0 > 0.2e-6)
   {
      decrgpulse(pwC,zero,2.0e-6,0.0);
      dec2rgpulse(pwN,zero,2.0e-6,0.0);
      zgradpulse(gzlvl0,gt0); 
      delay(1.0e-3);
   }
   decphase(t2);

   rgpulse(pw,t1,2.0e-6,0.0);	
   xmtrphase(zero);
   zgradpulse(gzlvl3,gt3);
   delay(tauch - gt3);			
   decrgpulse(pwC,t2,2.0e-6,0.0);	
status(B);
   decpower(dpwr);
   delay(tau2); 
   rgpulse(2.0*pw,t1,0.0,0.0);              
   decphase(zero);
   if (tau2 > 2.0*pwN)
   {
      dec2rgpulse(2.0*pwN,zero,0.0,0.0);              
      delay(tau2 - 2.0*pwN);
   }
   else
      delay(tau2); 
status(A);

   decpower(pwClvl);         
   decrgpulse(pwC,zero, 2.0e-6,2.0e-6);
   txphase(zero);
   delay(tauch + tau1 + SAPS_DELAY - gt3 - 4.0*pwC - 500.0e-6);		
   zgradpulse(gzlvl3,gt3);
   delay(500.0e-6);
   decrgpulse(pwC,zero,0.0, 0.0);	
   decphase(one);
   decrgpulse(2.0*pwC, one, 0.2e-6, 0.0);
   decphase(zero);
   decrgpulse(pwC, zero, 0.2e-6, 0.0);
   delay(tau1);
   rgpulse(pw,zero,0.0,0.0);

   delay(mix - gt4 - gt5 - pwN - 2.0e-3);		
   obsoffset(tof);

   zgradpulse(gzlvl4,gt4);
   delay(1.0e-3);
   sim3pulse((double)0.0,pwC,pwN,zero,zero,zero,0.0,2.0e-6);              
   zgradpulse(gzlvl5,gt5);
   delay(1.0e-3);			

   rgpulse(pw,zero,0.0,2.0e-6);		
   zgradpulse(gzlvl6,gt6);
   delay(taunh - gt6 - 2.0e-6);
   sim3pulse(2.0*pw,(double)0.0,2*pwN,zero,zero,zero,0.0,0.0);
   delay(taunh - gt6 - 500.0e-6);
   zgradpulse(gzlvl6,gt6);
   txphase(one);
   delay(500.0e-6);
   rgpulse(pw,one,0.0,0.0);	
   txphase(two);
   obspower(tpwrs);
   shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 2.0e-6);
   obspower(tpwr);
   zgradpulse(gzlvl7,gt7);	
   dec2phase(t3);
   decoffset(dofcaco);
   decpwrf(rfst);
   delay(200.0e-6);			

   dec2rgpulse(pwN,t3,0.0,0.0);

   dec2phase(t4);
   delay(tau3); 
   rgpulse(2.0*pw, zero, 0.0, 0.0);
   decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);
   delay(tau3);
   delay(gt1 + 202.0e-6 - 1.0e-3 - 2.0*pw);

   dec2rgpulse(2.0*pwN, t4, 0.0, 2.0e-6);

   dec2phase(t5);
   if(mag_flg[A] == 'y')
   {
      magradpulse(gzcal*gzlvl1, gt1);        
   }
   else
   {
      zgradpulse(gzlvl1, gt1);        
      delay(4.0*GRADIENT_DELAY);
   }
   delay(200.0e-6 + WFG_START_DELAY + WFG_STOP_DELAY - 6.0*GRADIENT_DELAY);

   sim3pulse(pw, 0.0, pwN, zero, zero, t5, 0.0, 2.0e-6);

   dec2phase(zero);
   zgradpulse(gzlvl8, gt8);
   delay(taunh - gt8);
   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
   delay(taunh  - gt8 - 400.0e-6);
   zgradpulse(gzlvl8, gt8);
   txphase(one);
   dec2phase(one);
   delay(400.0e-6);

   sim3pulse(pw, 0.0, pwN, one, zero, one, 0.0, 0.0);

   txphase(zero);
   dec2phase(zero);
   zgradpulse(gzlvl9, gt9);
   delay(taunh - gt9);
   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
   zgradpulse(gzlvl9, gt9);
   delay(taunh - gt9);

   rgpulse(pw, zero, 0.0, 0.0);

   delay((gt1/10.0) + gstab - 0.5*pw + 6.0*GRADIENT_DELAY + POWER_DELAY);

   rgpulse(2.0*pw, zero, 0.0, 0.0);

   if(mag_flg[A] == 'y')
   {
      magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);     	
   }
   else
   {
      zgradpulse(icosel*gzlvl2, gt1/10.0);     	
      delay(4.0*GRADIENT_DELAY);
   }
   dec2power(dpwr2);			   
   delay(gstab);		

status(C);
   setreceiver(t11);
}		 

