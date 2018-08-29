/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*     gNcpmgex.c (TROSY version)

This pulse sequence will allow one to perform the following experiments:
"Constant Relaxation Time CPMG experiment for NH" to identify Rex due to
(millisecond) motions. The resultant spectral intensity is attenuated by
the relaxation rate of the trosy component with a contribution Rex
that is dependent on the pulse spacing (2*tauCPMG) employed.

           F1      15N
           F2(acq)  1H (NH)
 
This sequence uses the standard three channel configuration
   1)  1H    - carrier (tof) @ 4.7 ppm [H2O]
   2) 13C    - carrier (dof) @  57 ppm [CA]
   3) 15N    - carrier (dof2)@ 119 ppm [centre of backbone amide NH]
 

 Constant Relaxation Time adaptation of:
      J.P. Loria, M. Rance & A.G. Palmer, III
      JACS 121, 2331-2332 (1999)
      (note there is a small mistake in their sequence (Fig.1);
       the second 1H 90 degree pulse should have phase y)

 Trosy adaptation of:
      Tollinger et al., JACS 123, 11341-11352, 2001

 If necessary one can do standard presaturation.  Note: presaturation
 is done using the transmitter with the power level set by 'tsatpwr'
 Irradiation is done with the carrier sitting at tofps. One can assign
 a different value to tof (e.g. center of amides) if required.

 No decoupling during acquisition
 Processing: use f1coef='1 0 -1 0 0 1 0 1' and wft2da

 Written by L.E.Kay on Nov. 19, 2001
 modified from  N15_CPMG_Rex_NH_trosy_lek_500.c for BioPack (GG jul03)

 f1180 setting of tau1 corrected and 13C decoupling option added 
   (Steve Van Doren, U.Missouri jan08)


Modified the amplitude of the flipback pulse(s) (pwHs) to permit user 
adjustment around theoretical value (tpwrsl). If tpwrsf < 4095.0 the 
value of tpwrsl is increased 6db and values of tpwrsf of 2048 or so 
should give equivalent amplitude. In cases of severe radiation damping
( which happens during pwHs) the needed flip angle may be much less than
90 degrees, so tpwrsf should be lowered to a value to minimize the 
observed H2O signal in single-scan experiments (with ssfilter='n').
(GG jul03)

Auto-calibrated version, E.Kupce, jul03.


 Radiation Damping:
   At fields 600MHz and higher with high-Q probes, radiation damping is a
   factor to be considered. Its primary effect is in the flipback pulse
   calibration. Radiation damping causes a rotation back to the +Z axis
   even without a flipback pulse. Hence, the pwHs pulse often needs to 
   be reduced in its flip-angle. This can be accomplished by using the
   parameter tpwrsf. If this value is less than 4095.0 the value of tpwrsl
   (calculated in the psg code) is increased by 6dB, thereby permitting
   the value of tpwrsf to be optimized to obtain minimum H2O in the 
   spectrum. The value of tpwrsf is typically lower than 2048 (half-maximum
   to compensate for the extra 6dB in tpwrsl). 

   The autocal and checkofs flags are generated automatically in Pbox_bio.h
   If these flags do not exist in the parameter set, they are automatically 
   set to 'y' - yes. In order to change their default values, create the  
   flag(s) in your parameter set and change them as required. 
   The available options for the checkofs flag are: 'y' (yes) and 'n' (no). 
   The offset (tof, dof, dof2 and dof3) checks can be switched off  
   individually by setting the corresponding argument to zero (0.0).
   For the autocal flag the available options are: 'y' (yes - by default), 
   'q' (quiet mode - suppress Pbox output), 'r' (read from file, no new  
   shapes are created), 's' (semi-automatic mode - allows access to user  
   defined parameters) and 'n' (no - use full manual setup, equivalent to 
   the original code).

   if BioPack power limits are in effect (BPpwrlimits=1) the cpmg 15N 
   amplitude is decreased by 3dB and pulse width increased 40%


 If comp_flg='y' there is a N15 pulse applied after d1 to compensate for RF/coil
 heating from the cpmg pulse train. This pulse be longest for the ncyc small and
 increases with ncyc to maintain a constant heating as a function of ncyc. Cryogenic
 probe tuning and shimming can be sensitive to power dissapation, so this compensates
 for the heating. Room temperature probes are not as sensitive to coil heating or
 deshimming, but may experience sample heating.

 The pulse is applied at dpwr2_comp power and the duration is calculated within the
 pulse sequence. The power should be determined by setting up a cpmg pulse train
 for the maximum ncyc to be used, starting the sequence, and then determining the
 heating power remaining (for a cold probe) after attaining steady-state. This should
 be done with comp_flg='n'. Then, set the N15 power levels other that dpwr2_comp to
 zero,ncyc=0 and set comp_flg='y'. Set ncyc_max to the largest ncyc value to be used
 in the queue of experiments. Try various compensation power levels until the heating
 power value achieves the same level as above. This represents a power level that
 produces the same heating as the cpmg pulse train.

 Now re-set ncyc and the cpmg pulse train power level to their proper values and leave
 comp_flg='y'. Use sufficient steady-state pulses to get to a stable heater value and
 then run the series of 2D experiments as a function of ncyc, one after the other.

 Once this is done the heating characteristics should be very similar from sample to
 sample so that the same settings could be used.
*/


#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */  
	     
static double   H1ofs=4.7, C13ofs=0.0, N15ofs=120.0, H2ofs=0.0;

static shape H2OsincA;

static int phi1[2] =  {0,2},
           phi2[8] =  {1,1,3,3,2,2,0,0},
           phi3[8] =  {1,1,3,3,0,0,2,2},
           phi4[1] =  {3},
           phi5[1] =  {1},
           phi6[1] =  {3},
           rec[8] =   {1,3,3,1,0,2,2,0};

static double d2_init = 0.0;

pulsesequence()

{
/* DECLARE VARIABLES */

 char       C13refoc[MAXSTR],comp_flg[MAXSTR],fsat[MAXSTR],f1180[MAXSTR];

 int	     phase,t1_counter;

 double   pwClvl = getval("pwClvl"),    /* coarse power for C13 pulse */ 
          pwC = getval("pwC"),     /* C13 90 degree pulse length at pwClvl */
          rf0,                     /* maximum fine power when using pwC pulses */
          rfst,                    /* fine power for the stCall pulse */
          compC = getval("compC"), /* adjustment for C13 amplifier compression */
             tau1,                 /* t1 delay */
             taua,                 /* < 1 / 4J(NH) 2.25 ms      */
             taub,                 /*   1 / 4J(NH) in NH : 2.68 ms  */
             pwn,                  /* PW90 for N-nuc            */
             pwN,                  /* N15 pw90 for BioPack      */
             pwNlvl,               /* N15 power for BioPack     */
             pwn_cp,               /* PW90 for N CPMG           */
             pwHs,                 /* BioPack selective PW90 for water excitation */
             compH,                /* amplifier compression factor*/
             compN,                /* amplifier compression factor*/
             phase_sl,
             tsatpwr,              /* low power level for presat */
             tpwrsf_u,             /* fine power adjustment on flip-up sel 90 */
             tpwrsf_d,             /* fine power adjustment on flip-down sel 90 */
             tpwrsl,               /* low power level for sel 90 */
             dhpwr2,               /* power level for N hard pulses */
             dpwr2_comp,           /* power level for CPMG compensation       */
             dpwr2_cp,             /* power level for N CPMG        */
             tauCPMG,              /* CPMG delay */
             ncyc,                 /* number of times to loop    */
             ncyc_max,              /* max number of times to loop    */
             time_T2,              /* total time for T2 measuring     */
             tofps,                /* water freq */
	     sw1,
             pwr_delay,            /* POWER_DELAY recalculated*/
             timeC,
             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gstab,                /* stabilization delay */
             BPpwrlimits,                    /*  =0 for no limit, =1 for limit */

             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6;

   P_getreal(GLOBAL,"BPpwrlimits",&BPpwrlimits,1);

/* LOAD VARIABLES */

  getstr("C13refoc", C13refoc);
/*  taub = 1/(8*93.0); */

  taua = getval("taua");
  taub = getval("taub");
  pwNlvl = getval("pwNlvl");
  pwN = getval("pwN");
  pwn = getval("pwn");
  pwn_cp = getval("pwn_cp");
  pwHs = getval("pwHs");
  compH = getval("compH");
  compN = getval("compN");
  phase_sl = getval("phase_sl");
  tsatpwr = getval("tsatpwr");
  tpwrsf_u = getval("tpwrsf_u");
  tpwrsf_d = getval("tpwrsf_d");
  tpwrsl = getval("tpwrsl");
  dhpwr2 = getval("dhpwr2"); 
  dpwr2_comp = getval("dpwr2_comp"); 
  dpwr2_cp = getval("dpwr2_cp"); 
  ncyc = getval("ncyc");
  ncyc_max = getval("ncyc_max");
  time_T2 = getval("time_T2");
  phase = (int) (getval("phase") + 0.5);
  sw1 = getval("sw1");
  tofps = getval("tofps");

  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gstab = getval("gstab");

  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  
  getstr("fsat",fsat); 
  getstr("comp_flg",comp_flg);
  getstr("f1180",f1180);



      setautocal();                        /* activate auto-calibration flags */ 
        
      if (autocal[0] == 'n') 
      {
        /* selective H20 one-lobe sinc pulse */
        if (pwHs > 0.0)
          tpwrsl = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
        else tpwrsl = 0.0;
        tpwrsl = (int) (tpwrsl);                   	  /*power than a square pulse */
      }
      else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
      {
        if(FIRST_FID)                                            /* call Pbox */
        {
          H2OsincA = pbox_Rsh("H2OsincA", "sinc90", pwHs, 0.0, compH*pw, tpwr);
          ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
        }
        pwHs = H2OsincA.pw; tpwrsl = H2OsincA.pwr-1.0;  /* 1dB correction applied */
        pwn = pwN; dhpwr2 = pwNlvl;
      }

      if (tpwrsf_u < 4095.0) 
      {
        tpwrsl = tpwrsl + 6.0;   
        pwr_delay = POWER_DELAY + PWRF_DELAY;
      }
      else pwr_delay = POWER_DELAY;

/* maximum fine power for pwC pulses (and initialize rfst) */
        rf0 = 4095.0;    rfst=0.0;

/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
     if (C13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));
        rfst = (int) (rfst + 0.5);
        if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n",
            (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }}

/* check validity of parameter range */

    if(dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y') 
	{
	printf("incorrect Dec1 decoupler flags! Should be nnn  ");
	psg_abort(1);
    } 

    if(dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' )
	{
	printf("incorrect Dec2 decoupler flags! Should be nnn  ");
	psg_abort(1);
    } 

    if( tsatpwr > 8 )
    {
	printf("tsatpwr too large !!!  ");
	psg_abort(1);
    }

    if( dpwr2_cp > 61 )
    {
        printf("don't fry the probe, dpwr2_cp too large for cpmg !");
	psg_abort(1);
    }

   if( ncyc > 100)
    {
       printf("ncyc exceeds 100. May be too much \n");
       psg_abort(1);
    }  

   if( time_T2 > 0.090 )
    {
       printf("total T2 recovery time exceeds 90 msec. May be too long \n");
       psg_abort(1);
    }  

   if( ncyc > 0)
    {
      tauCPMG = time_T2/(4*ncyc) - pwn_cp;
      if( ix == 1 )
      printf("nuCPMG for current experiment is (Hz): %5.3f \n",1/(4*(tauCPMG+pwn_cp)) );
    }
   else
    {
      tauCPMG = time_T2/4 - pwn_cp;
      if( ix == 1 )
      printf("nuCPMG for current experiment is (Hz): not applicable \n");
    }

   ncyc_max = time_T2/1e-3;
   if( tauCPMG + pwn_cp < 0.000250)
   {
      printf("WARNING: value of tauCPMG must be larger than or equal to 250 us\n");
      printf("maximum value of ncyc allowed for current time_T2 is: %5.2f \n",ncyc_max);
      psg_abort(1);
   }

   if(gt1 > 3e-3 || gt2 > 3e-3 || gt3 > 3e-3|| gt4 > 3e-3
                  || gt5 > 3e-3 || gt6 > 3e-3 )
   {
      printf("gti must be less than 3e-3\n");
      psg_abort(1);
   }

/* LOAD VARIABLES */

  settable(t1, 2, phi1);
  settable(t2, 8, phi2);
  settable(t3, 8, phi3);
  settable(t4, 1, phi4);
  settable(t5, 1, phi5);
  settable(t6, 1, phi6);
  settable(t7, 8, rec);

/* Phase incrementation for hypercomplex 2D data */

   if (phase == 2) {
     tsadd(t4,2,4);
     tsadd(t5,2,4);
     tsadd(t6,2,4);
     tsadd(t7,2,4);
   }

/* Set up f1180  */

   tau1 = d2;
   if(f1180[A] == 'y') 
     tau1 += ( 1.0 / (2.0*sw1) - (pw + pwN*2.0/3.1415));
   else
     tau1 = tau1 - pw; 

   if(tau1 < 0.2e-6) tau1 = 0.2e-6;
   tau1 = tau1/2.0;

/* Calculate modifications to phases for States-TPPI acquisition */

   if( ix == 1 ) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if( t1_counter %2 ) {
      tsadd(t2,2,4);
      tsadd(t3,2,4);
      tsadd(t7,2,4);
   }


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   decpower(dpwr);               /* Set decoupler1 power to dpwr */
decpower(pwClvl);
decpwrf(rfst);
decoffset(dof);
   dec2power(dhpwr2);            /* Set decoupler2 power to dhpwr2 */

/* Presaturation Period */

 if(fsat[0] == 'y')
{
  obspower(tsatpwr);            /* Set power for presaturation  */
  obsoffset(tofps);            /* move H carrier to the water */
  rgpulse(d1,zero,rof1,rof1);  /* presat. with transmitter */
  obspower(tpwr);                /* Set power for hard pulses  */
}

else
{
 obspower(tpwr);                /* Set power for hard pulses  */
 delay(d1);
}

  obsoffset(tof);

status(B);

/* apply the compensation 15N pulses if desired */
 if(comp_flg[A] == 'y') {

  dec2power(dpwr2_comp);            /* Set decoupler2 compensation power */

  timeC = time_T2*(ncyc_max-ncyc)/ncyc_max;

  dec2rgpulse(timeC,zero,0.0,0.0);
  dec2power(dhpwr2);
}

  rcvroff();
  delay(20.0e-6);

  /* shaped pulse on water */
  obspower(tpwrsl);
  if (tpwrsf_d<4095.0) obspwrf(tpwrsf_d);
  if (autocal[A] == 'y')
   shaped_pulse("H2OsincA",pwHs,three,rof1,0.0);
  else
   shaped_pulse("H2Osinc_d",pwHs,three,rof1,0.0);
  if (tpwrsf_d<4095.0) obspwrf(4095.0);
  obspower(tpwr);
  /* shaped pulse on water */
  
  rgpulse(pw,two,rof1,0.0);     

  txphase(zero);
  dec2phase(zero);

  delay(2.0e-6);
  zgradpulse(gzlvl1,gt1);
  delay(gstab);

  delay(taua - gt1 - gstab -2.0e-6);                   /* delay < 1/4J(XH)   */

  sim3pulse(2*pw,0.0e-6,2*pwn,zero,zero,zero,0.0,0.0);

  txphase(one);
  dec2phase(t1);

  delay(taua - gt1 - gstab -2.0e-6);

  delay(2.0e-6);
  zgradpulse(gzlvl1,gt1);
  delay(gstab);

  rgpulse(pw,one,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl2,gt2);
  delay(gstab);

  if (BPpwrlimits > 0.5)
   {
    dec2power(dpwr2_cp -3.0);    /* reduce for probe protection */
    pwn_cp=pwn_cp*compN*1.4;
   }
  else
   dec2power(dpwr2_cp);            /* Set decoupler2 power to dpwr2_cp for CPMG period */

  dec2rgpulse(pwn_cp,t1,rof1,2.0e-6);

  dec2phase(zero);

  /* start of the CPMG train for first period time_T2/2 on Ny(1-2Hz) */
  if(ncyc > 0) 
  {
    delay(tauCPMG - (2/PI)*pwn_cp - 2.0e-6);
    dec2rgpulse(2*pwn_cp,one,0.0,0.0);
    delay(tauCPMG);   
  }
 
  if(ncyc > 1) 
  {
  initval(ncyc-1,v4);
  loop(v4,v5);
 
    delay(tauCPMG);
    dec2rgpulse(2*pwn_cp,one,0.0,0.0);
    delay(tauCPMG);   
 
  endloop(v5);
  }
 
  /* eliminate cross-relaxation  */

  delay(2.0e-6);
  zgradpulse(gzlvl3,gt3);
  delay(gstab);

  delay(taub - gt3 - gstab -2.0e-6 - pwn_cp);

  /* composite 1H 90y-180x-90y on top of 15N 180x */
  dec2rgpulse(pwn_cp-2*pw,zero,0.0e-6,0.0);
  sim3pulse(pw,0.0e-6,pw,one,zero,zero,0.0,0.0);
  sim3pulse(2*pw,0.0e-6,2*pw,zero,zero,zero,0.0,0.0);
  sim3pulse(pw,0.0e-6,pw,one,zero,zero,0.0,0.0);
  dec2rgpulse(pwn_cp-2*pw,zero,0.0,0.0e-6);
  /* composite 1H 90y-180x-90y on top of 15N 180x */

  delay(taub - gt3 - gstab -2.0e-6 - pwn_cp - 4.0*pw); 

  delay(2.0e-6);
  zgradpulse(gzlvl3,gt3);
  delay(gstab);

  rgpulse(pw,one,0.0,0.0);
  rgpulse(2.0*pw,zero,0.0,0.0);
  rgpulse(pw,one,0.0,0.0);

  /* start of the CPMG train for second period time_T2/2 on Nx(1-2Iz) */
  if(ncyc > 1) 
  {
  initval(ncyc-1,v4);
  loop(v4,v5);
 
    delay(tauCPMG);
    dec2rgpulse(2*pwn_cp,zero,0.0,0.0);
    delay(tauCPMG);   
 
  endloop(v5);
  }
 
  if(ncyc > 0) 
  {
    delay(tauCPMG);
    dec2rgpulse(2*pwn_cp,zero,0.0,0.0);
    delay(tauCPMG - (2/PI)*pwn_cp - 2.0e-6);   
  }
 
  dec2phase(one);

  dec2rgpulse(pwn_cp,one,2.0e-6,0.0);

  delay(rof1);
  dec2power(dhpwr2);            /* Set decoupler2 power back to dhpwr2 */

  dec2phase(t3);

  delay(2.0e-6);
  zgradpulse(gzlvl4,gt4);
  delay(gstab);

  if(phase==1)
   dec2rgpulse(pwn,t2,rof1,0.0);
  if(phase==2)
   dec2rgpulse(pwn,t3,rof1,0.0);

  txphase(t4); 
  decphase(one);
  dec2phase(zero);

/* 15N chemical shift labeling with optional 13C decoupling of Ca & C'*/
        if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY);     /* WFG2_START_DELAY */
            decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);
            delay(tau1 - 0.5e-3);}
        else    delay(2.0*tau1);
/* finish of 15N shift labeling*/

  rgpulse(pw,t4,0.0,0.0);

  /* shaped pulse on water */
  obspower(tpwrsl);
  if (tpwrsf_u<4095.0) obspwrf(tpwrsf_u);
  if (autocal[A] == 'y')
   shaped_pulse("H2OsincA",pwHs,t5,rof1,0.0);
  else
   shaped_pulse("H2Osinc_u",pwHs,t5,rof1,0.0);
  if (tpwrsf_u<4095.0) obspwrf(4095.0);
  obspower(tpwr);
  /* shaped pulse on water */

  delay(2.0e-6);
  zgradpulse(gzlvl5,gt5);
  delay(gstab/2.0);

  delay(taua - pwr_delay - rof1 - WFG_START_DELAY
        - pwHs - WFG_STOP_DELAY - pwr_delay
        - gt5 - gstab/2.0 -2.0e-6);

  sim3pulse(2.0*pw,0.0,2.0*pwn,zero,zero,zero,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl5,gt5);
  delay(gstab/2.0);

  delay(taua 
        - gt5 - 2.0e-6 -gstab
        - pwr_delay - rof1 - WFG_START_DELAY
        - pwHs - WFG_STOP_DELAY - pwr_delay - 2.0e-6);

  /* shaped pulse on water */
  obspower(tpwrsl);
  if (tpwrsf_u<4095.0) obspwrf(tpwrsf_u);
  if (autocal[A] == 'y')
   shaped_pulse("H2OsincA",pwHs,zero,rof1,0.0);
  else
   shaped_pulse("H2Osinc_u",pwHs,zero,rof1,0.0);
  if (tpwrsf_u<4095.0) obspwrf(4095.0);
  obspower(tpwr);
  /* shaped pulse on water */

  sim3pulse(pw,0.0e-6,pwn,zero,zero,t6,2.0e-6,0.0);

  txphase(zero);
  dec2phase(zero);

  delay(2.0e-6);
  zgradpulse(gzlvl6,gt6);
  delay(gstab/2.0);

  delay(taua - gt6 - gstab/2.0 -2.0e-6 - pwr_delay
        - pwHs);

  initval(1.0,v3);
  obsstepsize(phase_sl);
  xmtrphase(v3);
  obspower(tpwrsl);
  if (tpwrsf_d<4095.0) obspwrf(tpwrsf_d);
  if (autocal[A] == 'y')
   shaped_pulse("H2OsincA",pwHs,two,rof1,0.0);
  else
   shaped_pulse("H2Osinc_d",pwHs,two,rof1,0.0);
  if (tpwrsf_d<4095.0) obspwrf(4095.0);
  obspower(tpwr);
  xmtrphase(zero);

  sim3pulse(2*pw,0.0e-6,2*pwn,zero,zero,zero,rof1,rof1);  

  initval(1.0,v3);
  obsstepsize(phase_sl);
  xmtrphase(v3);
  obspower(tpwrsl);
  if (tpwrsf_u<4095.0) obspwrf(tpwrsf_u);
  if (autocal[A] == 'y')
   shaped_pulse("H2OsincA",pwHs,two,rof1,0.0);
  else
   shaped_pulse("H2Osinc_u",pwHs,two,rof1,0.0);
  if (tpwrsf_u<4095.0) obspwrf(4095.0);
  obspower(tpwr);
  xmtrphase(zero);

  delay(2.0e-6);
  zgradpulse(gzlvl6,gt6);
  delay(gstab/2.0);

  delay(taua - pwHs - gt6 - gstab/2.0 -2.0e-6          
        + 2.0*pw/PI - pwn
        - 2.0*POWER_DELAY);

  dec2rgpulse(pwn,zero,0.0,0.0);

  decpower(dpwr);                                 /* lower power on dec */
  dec2power(dpwr2);                               /* lower power on dec2 */

/* acquire data */

status(C);
     setreceiver(t7);
}
