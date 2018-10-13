/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghnhb.c

    3D HNHB experiment for the measurement of N-HB coupling constants

    REF:  Sharon J. Archer, Mitsuhiko Ikura, Dennis A. Torchia,
          and A. Bax, JMR, 95, 636-641 (1991)

          Ad. Bax, Geerten W. Vuister, Stephan Grzesiek, 
          Frank Delaglio, Andy C. Wang, Rolf Tschudin, Guang Zhu,
          Methods in Enzymology, 239, 79-105 (1994)

    Writen by 
    Weixing Zhang
    St. Jude Children's Research Hospital.
    Memphis, TN 38105


    Uses three channels:
      1) HN (t3)       -  carrier 4.75 ppm (tof)
         HB (t1, ni)
      2) C13           -  carrier 46 ppm (dof)
      3) N15 (t2, ni2) -  carrier at 120 ppm (dof2)

    gzcal = 0.0022 gauss/dac,
    mag_flg  =  y  using magic angle pulsed field gradient
             =  n  using z-axis gradient only
    ref_flg  =  y  recording 2D HN-N reference spectrum in t2 dimension.
             =  n  recording 3D spectrum with N-HB coupling present.


     BioPack "H2Osinc.RF" shape(1418 steps)
     is used as default. Use pwHs=1418*n for no waveform
     error messages. Since only NH is detected, 1.418 msec is
     a reasonably selective pulse.

        (modified for BioPack 14july2000 by GG, Varian Palo Alto)
        (corrected proton phase values in 3919 portion, WJ Jan 2003


    Modified the amplitude of the flipback pulse(s) (pwHs) to permit user adjustment around
    theoretical value (tpwrs). If tpwrsf < 4095.0 the value of tpwrs is increased 6db and
    values of tpwrsf of 2048 or so should give equivalent amplitude. In cases of severe
    radiation damping( which happens during pwHs) the needed flip angle may be much less than
    90 degrees, so tpwrsf should be lowered to a value to minimize the observed H2O signal in 
    single-scan experiments (with ssfilter='n').(GG jan01)


  Radiation Damping:
       At fields 600MHz and higher with high-Q probes, radiation damping is a
       factor to be considered. Its primary effect is in the flipback pulse
       calibration. Radiation damping causes a rotation back to the +Z axis
       even without a flipback pulse. Hence, the pwHs pulse often needs to 
       be reduced in its flip-angle. This can be accomplished by using the
       parameter tpwrsf. If this value is less than 4095.0 the value of tpwrs
       (calculated in the psg code) is increased by 6dB, thereby permitting
       the value of tpwrsf to be optimized to obtain minimum H2O in the 
       spectrum. The value of tpwrsf is typically lower than 2048 (half-maximum
       to compensate for the extra 6dB in tpwrs). 
*/

#include <standard.h>
extern int dps_flag;

static int 
            phi1[4]  = {0,0,2,2},
            phi2[2]  = {0,2},
            phi3[8] = {0,0,0,0,1,1,1,1},
            phi4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},

            ref[8] = {0,2,0,2,2,0,2,0},
            rec[8] = {0,2,2,0,2,0,0,2};   
           
static double d2_init=0.0, d3_init=0.0;
            
pulsesequence()
{
 char       f1180[MAXSTR],   
            f2180[MAXSTR],  
            mag_flg[MAXSTR],
            flg_3919[MAXSTR],
            ref_flg[MAXSTR];

 int         phase, ni2,
             t1_counter,   
             t2_counter;   

 double      gzcal = getval("gzcal"),
             factor = 0.08, /* used for 3-9-19 water gate */
             tau_3919 = getval("tau_3919"),
             flipphase = getval("flipphase"),
             tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  1/4JNH   ~ 2.3 ms   */
             taub,        /*  1/4JNH    ~ 2.3 ms  */
             bigT,         /* ~ 19  ms   */

             pwNlvl,       
             pwN,  
             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gt8,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7,
             gzlvl8,


  compH = getval("compH"),         /* adjustment for C13 amplifier compression */
  pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
  tpwrsf = getval("tpwrsf"),     /* fine power adjustment for flipback pulse   */
  tpwrs;	  	              /* power for the pwHs ("H2Osinc") pulse */
	
/* LOAD VARIABLES */

  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("flg_3919", flg_3919);
  getstr("mag_flg", mag_flg);
  getstr("ref_flg", ref_flg);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  bigT   = getval("bigT");
   
  tpwr = getval("tpwr");
  pwNlvl = getval("pwNlvl");
  pwN = getval("pwN");
  
  dpwr = getval("dpwr");
  dpwr2 = getval("dpwr2");
  phase = (int)( getval("phase") + 0.5);
  phase2 = (int)( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");  
  ni = getval("ni");
  ni2 = getval("ni2");
  
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");

/* LOAD PHASE TABLE */
           
  settable(t1,4,phi1);
  settable(t2,2,phi2);
  settable(t3,8,phi3);
  settable(t4,16, phi4);

  if (ref_flg[A] == 'y')
  {
     settable(t10,8,ref);
  }
  else
  {
     settable(t10,8,rec);
  }
  

/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */

/* CHECK VALIDITY OF PARAMETER RANGES */

    if (ref_flg[A] == 'y' && ni > 1)
    {
       printf(" Incorrect setting of ni and ref_flg.\n");
       printf(" Please choose either ni=1 or ref_flg=n.\n");
       psg_abort(1);
    }
    
    if (ref_flg[A] == 'y' && dps_flag)
    {
       printf(" Please use phase2 and ni2 for 2D reference spectrum\n");
       if (ni2/sw2 > 2.0*(2.0*bigT - gt5 - 200.0e-6))
       {
           printf("ni2 is too big, should be < %f\n", 2.0*sw2*(2.0*bigT-gt5-200.0e-6));
           psg_abort(1);
       }
    }

    if ((ni2/sw2 > 2.0*(bigT -  gt5 - 200.0e-6)) && (ref_flg[A] !='y'))
    {
       printf(" ni2 is too big, should be < %f\n", 2.0*sw2*(bigT-gt6-200.0e-6));
       psg_abort(1);
    }

    if(dpwr2 > 50)
    {
        printf("don't fry the probe, dpwr2 is  too large!  ");
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
       tsadd(t1,1,4);
    }
  
    if (phase2 == 2)
    {
       tsadd(t2,1,4);   
    }

/*  Set up f1180  half_dwell time (1/sw1)/2.0           */
   
    tau1 = d2 - (4.0*pw/PI + 2.0*pwN);
    if(f1180[A] == 'y')
    {
        tau1 += (1.0/(2.0*sw1));
    }
    if(tau1 < 0.2e-6) tau1 = 0.0;
    tau1 = tau1/2.0;

/*  Set up f2180   half dwell time (1/sw2)/2.0              */

    tau2 = d3;
    if(f2180[A] == 'y')
    {
        tau2 += (1.0/(2.0*sw2)); 
    }
    if(tau2 < 0.2e-6) tau2 = 0.0;
    tau2 = tau2/2.0;


/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
 
   t1_counter = (int)((d2-d2_init)*sw1 + 0.5);
   if(t1_counter % 2) 
   {
      tsadd(t1,2,4);     
      tsadd(t10,2,4);    
   }
   
   if( ix == 1) d3_init = d3 ;
   t2_counter = (int)((d3-d3_init)*sw2 + 0.5);
   if(t2_counter % 2) 
   {
      tsadd(t2,2,4);  
      tsadd(t10,2,4);    
   }

   if (flipphase < -0.01)  flipphase = flipphase + 360.0;
   initval(flipphase, v10);
   obsstepsize(0.25);


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tpwr);
   dec2power(pwNlvl);
   txphase(zero);
   dec2phase(zero);

   delay(d1);

   if(gt1 > 0.2e-6)
   {
      dec2rgpulse(pwN, zero, 0.2e-6, 0.0);
      delay(2.0e-6);
      zgradpulse(gzlvl1, gt1);
      delay(0.001);
   }
   rcvroff();

status(B);

   rgpulse(pw, zero,rof1, 0.0);
   delay(2.0e-6);
   if (gt2 > 0.2e-6)
      zgradpulse(gzlvl2,gt2);
   delay(taua - gt2 - 2.0e-6);
   sim3pulse(2.0*pw,(double)0.0,2.0*pwN, zero, zero, zero, 0.0, 0.0);
   delay(taua - gt2 - 400.0e-6);
   if (gt2 > 0.2e-6)
   {
      zgradpulse(gzlvl2,gt2);
   }
   txphase(one);
   dec2phase(t2);
   delay(400.0e-6);
   rgpulse(pw, one, 0.0, 0.0);
   if (gt3 > 0.2e-6)
   {
      delay(2.0e-6);
      zgradpulse(gzlvl3, gt3);
      delay(200.0e-6);
   }
   txphase(zero);
   dec2rgpulse(pwN, t2, 0.0, 0.0);

   if (ref_flg[A] == 'y')
   {
      delay(tau2);
      rgpulse(2.0*pw, zero, 0.0, 0.0);
      dec2phase(t3);
      if (gt5 > 0.2e-6)
      {
         delay(2.0*bigT - gt5 - 2.0*pw - 1.0e-3);
         zgradpulse(gzlvl5, gt5);
         delay(1.0e-3);
         dec2rgpulse(2.0*pwN, t3, 0.0, 0.0);
         delay(2.0e-6);
         zgradpulse(gzlvl5, gt5);
         delay(2.0*bigT - tau2 - gt5 - 2.0e-6);
      }
      else
      {
         delay(2.0*bigT - 2.0*pw);
         dec2rgpulse(2.0*pwN, t3, 0.0, 0.0);
         delay(2.0*bigT - tau2);
      }
   }
   else
   {
      dec2phase(zero);
      if (gt4 > 0.2e-6)
      {
         delay(2.0e-6);
         zgradpulse(gzlvl4, gt4);
         delay(bigT - gt4 - 2.0e-6);
         sim3pulse(2.0*pw,(double)0.0, 2.0*pwN, zero,zero, zero, 0.0, 0.0);
         delay(2.0e-6);
         zgradpulse(gzlvl4, gt4);
         delay(1.0e-3 - 2.0e-6);
      }
      else
      {
         delay(bigT);
         sim3pulse(2.0*pw,(double)0.0, 2.0*pwN, zero,zero, zero, 0.0, 0.0);
         delay(1.0e-3);
         gt4 = 0.0;
      }

      zgradpulse(gzlvl5, gt5);
      txphase(t1);
      delay(bigT - gt4 - gt5 - 1.0e-3 - 2.0*GRADIENT_DELAY);

      rgpulse(pw, t1, 0.0, 0.0);
      delay(tau1);
      dec2rgpulse(2.0*pwN, t3, 0.0, 0.0);
      txphase(zero);
      delay(tau1);
      rgpulse(pw, zero, 0.0, 0.0);

      delay(2.0e-6);
      zgradpulse(gzlvl5, gt5);
      dec2phase(t4);

      if (gt6 > 0.2e-6)
      {
         delay(tau2 + 100.0e-6);
         zgradpulse(gzlvl6, gt6);
         delay(bigT - gt5 - gt6 - 100.0e-6 - 2.0*GRADIENT_DELAY);
         sim3pulse(2.0*pw,(double)0.0, 2.0*pwN, zero,zero, t4, 0.0, 0.0);
         delay(2.0e-6);
         dec2phase(zero);
         zgradpulse(gzlvl6, gt6);
         delay(bigT - tau2 - gt6 - 2.0e-6);
      }
      else
      {
         delay(bigT + tau2 - gt5 - 2.0*GRADIENT_DELAY);
         sim3pulse(2.0*pw,(double)0.0, 2.0*pwN, zero,zero, t4, 0.0, 0.0);
         dec2phase(zero);
         delay(bigT - tau2);
      }
   }

   if (gt7 > 0.2e-6)
   {
      dec2rgpulse(pwN, zero, 0.0,2.0e-6);
      zgradpulse(gzlvl7, gt7);
      txphase(zero);
      
      delay(200.0e-6);
      if (pwHs > 0.2e-6)
      {
         xmtrphase(v10);
         if (tpwrsf<4095.0) {obspower(tpwrs+6.0); obspwrf(tpwrsf);}
          else obspower(tpwrs);
         txphase(two);
         shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);
         xmtrphase(zero);
         obspower(tpwr); obspwrf(4095.0);
      }
      rgpulse(pw, zero, 2.0e-6, 0.0);
   }
   else
   {
      sim3pulse(pw,(double)0.0, pwN, zero,zero, zero, 0.0, 0.0);
   }


   delay(2.0e-6);
   if(mag_flg[A] == 'y')
   {
      magradpulse(gzcal*gzlvl8, gt8);
   }
   else
   {
      zgradpulse(gzlvl8, gt8);
   }
   if (flg_3919[A] == 'y')
   {
      delay(taub - 31.0*factor*pw - 2.5*tau_3919 - gt8 - 2.0e-6);
      rgpulse(pw*factor*3.0, two, 0.0, 0.0);
      delay(tau_3919);
      rgpulse(pw*factor*9.0, two, 0.0, 0.0);
      delay(tau_3919);
      rgpulse(pw*factor*19.0, two, 0.0, 0.0);
      delay(tau_3919/2.0 - pwN);
      dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
      delay(tau_3919/2.0 - pwN);
      rgpulse(pw*factor*19.0, zero, 0.0, 0.0);
      delay(tau_3919);
      rgpulse(pw*factor*9.0, zero, 0.0, 0.0);
      delay(tau_3919);
      rgpulse(pw*factor*3.0, zero, 0.0, 0.0);
      delay(taub - 31.0*factor*pw - 2.5*tau_3919 - gt8 - POWER_DELAY - 402.0e-6);
   }
   else
   {
      if (tpwrsf<4095.0) {obspower(tpwrs+6.0); obspwrf(tpwrsf);}
       else obspower(tpwrs);
      txphase(two);
      xmtrphase(v10);
      delay(taub - pwHs - gt8 - 2.0*POWER_DELAY - 2.0e-6);
      shaped_pulse("H2Osinc", pwHs, two, 0.0, 0.0);
      obspower(tpwr); obspwrf(4095.0);
      xmtrphase(zero);
      txphase(zero);
      sim3pulse(2.0*pw, (double)0.0, 2.0*pwN, zero, zero, zero, 2.0e-6, 0.0);
      if (tpwrsf<4095.0) {obspower(tpwrs+6.0); obspwrf(tpwrsf);}
       else obspower(tpwrs);
      txphase(two);
      xmtrphase(v10);
      shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);
      xmtrphase(zero);
      obspower(tpwr); obspwrf(4095.0);
      dec2power(dpwr2);
      delay(taub - pwHs  - gt8 - 3.0*POWER_DELAY - 402.0e-6);
   }
   dec2power(dpwr2);
   if(mag_flg[A] == 'y')
   {
      magradpulse(gzcal*gzlvl8, gt8);
   }
   else
   {
      zgradpulse(gzlvl8, gt8);
   }
   delay(400.0e-6);

status(C);
   setreceiver(t10);
   rcvron();
}

