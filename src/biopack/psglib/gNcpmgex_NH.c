/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gNcpmgex_NH

       This pulse sequence will allow one to perform the following experiments:
        "Constant Relaxation Time CPMG experiment for NH" to identify Rex due to
        (millisecond) motions. The resultant spectral intensity is attenuated by
        an average of Nx and NyHz relaxation rates, with a contribution Rex
        that is dependent on the pulse spacing (2*tauCPMG) employed.

    2D HSQC  Heteronuclear Single Quantum Coherence with enhanced sensitivity
             PFG and minimal perturbation of water.
           F1      15N
           F2(acq)  1H (NH)
 
    This sequence uses the standard three channel configuration
         1)  1H             - carrier (tof) @ 4.7 ppm [H2O]
         2) 13C             - carrier (dof) @  57 ppm [CA]
         3) 15N             - carrier (dof2)@ 119 ppm [centre of backbone amide NH]
 

       Constant Relaxation Time adaptation of:
            J.P. Loria, M. Rance & A.G. Palmer, III
            JACS 121, 2331-2332 (1999)
            (note there is a small mistake in their sequence (Fig.1);
             the second 1H 90 degree pulse should have phase y)

       If necessary one can do standard presaturation.  Note: presaturation
       is done using the transmitter with the power level set by 'tsatpwr'
       Irradiation is done with the carrier sitting at tofps. One can assign
       a different value to tof (e.g. center of amides) if required.

                 dm = 'nnny'
                 dmm= 'cccp'
                 dseq='waltz16'
       This sequence will then use the a waveform to give a waltz
       sequence at the appropriate times. (Of course, any other decoupling
       sequence could also be used instead of waltz.)
          
       Written by Frans Mulder,  5 Aug 2000 from N15_CPMG_Rex_NH2_fm_500.c
       Written by Frans Mulder & Martin Tollinger,15 Oct  2000

       Original sequence has poor water suppression at high field
                for long time_T2 due to radiation damping
       A watergate strategy is employed in conjunction with weak
                bipolar gradients during t1.

       Adaptation 15 Nov 2000 by fm: added small angle phase shift
                possibility for watergate pulses

       Modified by L.E.Kay on Dec. 3, 2002 for application to carbon samples.
            Use an adiabatic pulse (400 us), centered at 117 ppm.

       Modified by L.E.Kay on Dec. 6, 2004 to include comp_flg which attempts to compensate
            for changes in duty cycle that affect cryogenic probe performance and cause R2,eff to 
            increase with B1; set comp_flg to y and ncyc_max to the max ncyc that you are using.

       Modified by L.E.Kay on June 22, 2005 to move compensation immediately prior to d1 period.
            Simulations establish that this will not affect initial proton magnetization 

            The compensating pulse duration is automatically calculated, but the amplitude should
            be empirically determined to generate the same effective field. A single pulse is used
            at lower power to avoid coil heating in cryogenic probes.(N.Murali, Varian 12/05)

       Modified from  N15_CPMG_Rex_NH_fm_800_v5.c for BioPack (GG, Varian  Jan 06):

        Shaped pulses can be automatically calculated and power levels/pulse widths set
        if autocal='y'.
      
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
	     
static shape stC200,H2OsincA;

static int phi2[2] =  {0,2},
           phi3[8] =  {1,1,1,1,3,3,3,3}, 
           phi4[1] =  {0},
           phi5[4] =  {0,0,2,2},
           phi6[4] =  {2,2,0,0},
           rec[8] =   {0,2,0,2,2,0,2,0};

static double d2_init = 0.0;

pulsesequence()

{
/* DECLARE VARIABLES */

 char        fsat[MAXSTR],f1180[MAXSTR],c_flg[MAXSTR],shp_sl[MAXSTR],shape_wg[MAXSTR],
             sh_ad[MAXSTR],comp_flg[MAXSTR];

 int	     phase,t1_counter,icosel;

 double      tau1,                 /* t1 delay */
             taua,                 /* < 1 / 4J(NH) 2.25 ms      */
             taub,                 /*   1 / 4J(NH) in NH : 2.68 ms  */
             pwn,pwN,              /* PW90 for N-nuc            */
             pwn_cp,               /* PW90 for N CPMG           */
             pwC,pwClvl,compC,     /* PW90,power and compression for C hard pulse calibration */
             pwc_ad,               /* PW180 for adiabatic pulse            */
             pw_sl,           /* selective PW90 for water excitation */
             pw_wg,                /* selective PW90 for water in watergate */
             compH,                /* compression factor for 1H at tpwr */
             phase_sl,
             phase_wg,
             tsatpwr,              /* low power level for presat */
             tpwrsl,               /* low power level for sel 90 */
             tpwrwg,               /* low power level for watergate */
             dhpwr2,pwNlvl,        /* power level for N hard pulses */
             d_ad,                 /* power level for C adiabatic pulse */
             dpwr2_comp,           /* power level for N CPMG compensation pulse  */
             dpwr2_cp,             /* power level for N CPMG        */
             tauCPMG,              /* CPMG delay */
             ncyc,                 /* number of times to loop    */
             ncycmax,              /* max number of times allowed to loop    */
             time_T2,              /* total time for T2 measuring     */
             timeC,                /* compensation time */
             tofps,                /* water freq */
	     sw1,
             nst,bw,ppm,
             BigT1,

             ncyc_max,             /* max number of ncyc that is used in expt */

             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gt8,
             gt9,
             gt10,
             gt11,
             gstab,
             gzlvl0,
             gzlvl1,
             gzlvl2,
             gzlvl5,
             gzlvl6,
             gzlvl7,
             gzlvl8,
             gzlvl9,
             gzlvl10,
             gzlvl11;

/* LOAD VARIABLES */

/*  taub = 1/(8*93.0);
*/
  taua = getval("taua");
  taub = getval("taub");
  pwn = getval("pwn");
  pwN = getval("pwN");
  pwClvl = getval("pwClvl");
  pwC = getval("pwC");
  pwNlvl = getval("pwNlvl");
  pwn_cp = getval("pwn_cp");
  pwc_ad = getval("pwc_ad");
  compC = getval("compC");
  compH = getval("compH");
  pw_sl = getval("pw_sl");
  pw_wg = getval("pw_wg");
  phase_sl = getval("phase_sl");
  phase_wg = getval("phase_wg");
  tsatpwr = getval("tsatpwr");
  tpwrsl = getval("tpwrsl");
  tpwrwg = getval("tpwrwg");
  dhpwr2 = getval("dhpwr2"); 
  dpwr2_cp = getval("dpwr2_cp"); 
  dpwr2_comp = getval("dpwr2_comp"); 
  d_ad = getval("d_ad");
  ncyc = getval("ncyc");
  time_T2 = getval("time_T2");
  phase = (int) (getval("phase") + 0.5);
  sw1 = getval("sw1");
  tofps = getval("tofps");
  BigT1 = getval("BigT1");
  pwc_ad = getval("pwc_ad");

  ncyc_max = getval("ncyc_max");

  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  gt9 = getval("gt9");
  gt10 = getval("gt10");
  gt11 = getval("gt11");
  gstab  = getval("gstab");
  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");
  gzlvl9 = getval("gzlvl9");
  gzlvl10 = getval("gzlvl10");
  gzlvl11 = getval("gzlvl11");
  
  getstr("fsat",fsat); 
  getstr("f1180",f1180);
  getstr("c_flg",c_flg);
  getstr("shp_sl",shp_sl);
  getstr("shape_wg",shape_wg);
  getstr("sh_ad",sh_ad);
  
  getstr("comp_flg",comp_flg);

/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y'))
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if((dm2[A] == 'y' || dm2[B] == 'y'))
	{
	printf("incorrect Dec2 decoupler flags!  ");
	psg_abort(1);
    } 

    if( tsatpwr > 8 )
    {
	printf("tsatpwr too large !!!  ");
	psg_abort(1);
    }

    if( dpwr2 > 50 )
    {
	printf("don't fry the probe, dpwr2 too large!  ");
	psg_abort(1);
    }

    if( pwn < 2*pw )
    {
	printf("the length of pwn should exceed 2*pw ");
	psg_abort(1);
    }

    if( dpwr2_comp > dpwr2_cp )
    {
        printf("dpwr2_comp too large. Typ. 7-10 dB lower than dpwr2_cp !");
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

   ncycmax = time_T2/1e-3;

   if( tauCPMG + pwn_cp < 0.000250)
   {
      printf("WARNING: value of tauCPMG must be larger than or equal to 250 us\n");
      printf("maximum value of ncyc allowed for current time_T2 is: %5.2f \n",ncycmax);
      psg_abort(1);
   }

   if(gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3|| gt4 > 15e-3
                  || gt5 > 15e-3 || gt6 > 15e-3 || gt7 > 15e-3
                  || gt8 > 15e-3 || gt9 > 15e-3 || gt10 > 15e-3
                  || gt11 > 15e-3 )
   {
      printf("gti must be less than 15e-3\n");
      psg_abort(1);
   }

    if(gzlvl0 > 500) {
       printf("gzlvl0 is too high; <= 500\n");
       psg_abort(1);
    }

    if(d_ad > 58) {
       printf("d_ad is too high; < 59\n");
       psg_abort(1);
    }

/* LOAD VARIABLES */

  settable(t2, 2, phi2);
  settable(t3, 8, phi3);
  settable(t4, 1, phi4);
  settable(t5, 4, phi5);
  settable(t6, 4, phi6);
  settable(t7, 8, rec);

/* Phase incrementation for hypercomplex 2D data */

   if (phase == 2) {
     tsadd(t4,2,4);
     icosel = -1;
   }

   else
     icosel = 1;

/* Set up f1180  */

   tau1 = d2;
   if(f1180[A] == 'y') {
     tau1 += ( 1.0 / (2.0*sw1) );
     if(tau1 < 0.2e-6) tau1 = 0.2e-6;
   }
  
   tau1 = tau1/2.0;

/* Calculate modifications to phases for States-TPPI acquisition */

   if( ix == 1 ) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if( t1_counter %2 ) {
      tsadd(t3,2,4);
      tsadd(t7,2,4);
   }

      setautocal();                        /* activate auto-calibration flags */ 
        
      if (autocal[0] == 'y') 
      {
        strcpy(sh_ad,"stC200");
        if(FIRST_FID)                                            /* call Pbox */
        {
          H2OsincA = pbox_Rsh("H2OsincA", "sinc90", pw_sl, 0.0, compH*pw, tpwr);

            ppm = getval("dfrq");
            bw = 200.0*ppm;       nst = 1000;          /* nst - number of steps */
            stC200 = pbox_makeA("stC200", "sech", bw, 0.0004, 0.0, compC*pwC, pwClvl, nst);
        }
        pw_sl = H2OsincA.pw; tpwrsl = H2OsincA.pwr -1.0;  /* 1dB correction applied */
        d_ad=stC200.pwr; pwc_ad=stC200.pw; 
        tpwrwg = tpwrsl; pw_wg=pw_sl; 
        pwn = pwN; dhpwr2 = pwNlvl; 
      }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);            /* Set power for presaturation  */
   decpower(d_ad);               /* Set decoupler1 power to d_ad */
   dec2power(dhpwr2);            /* Set decoupler2 power to dhpwr2 */

 if(fsat[0] == 'y')
  {
    obsoffset(tofps);            /* move H carrier to the water */
    rgpulse(d1,zero,2.0e-6,2.0e-6);  /* presat. with transmitter */
    obspower(tpwr);                /* Set power for hard pulses  */
  }
 else
  {
   obspower(tpwr);                /* Set power for hard pulses  */
   delay(d1);
  }

  obsoffset(tof);

  dec2rgpulse(pwn,zero,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl1,gt1);
  delay(gstab);

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
  initval(1.0,v2);
  obsstepsize(phase_sl);
  xmtrphase(v2);
  obspower(tpwrsl);
  if (autocal[A] == 'y') 
   shaped_pulse("H2OsincA",pw_sl,three,4.0e-6,0.0);
  else
   shaped_pulse(shp_sl,pw_sl,three,4.0e-6,0.0);
  xmtrphase(zero);
  obspower(tpwr);
  txphase(zero);
  delay(4.0e-6);
  /* shaped pulse on water */
  
  rgpulse(pw,zero,0.0,0.0);     

  txphase(zero);
  dec2phase(zero);

  delay(2.0e-6);
  zgradpulse(gzlvl2,gt2);
  delay(gstab);

  delay(taua - gt2 - gstab -2.0e-6);                   /* delay < 1/4J(XH)   */

  sim3pulse(2*pw,0.0e-6,2*pwn,zero,zero,zero,0.0,0.0);

  txphase(one);
  dec2phase(t2);

  delay(taua - gt2 - gstab -2.0e-6);                   /* delay < 1/4J(XH)   */

  delay(2.0e-6);
  zgradpulse(gzlvl2,gt2);
  delay(gstab);

  rgpulse(pw,one,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl5,gt5);
  delay(gstab);

  dec2power(dpwr2_cp);            /* Set decoupler2 power to dpwr2_cp for CPMG period */

  dec2rgpulse(pwn_cp,t2,4.0e-6,2.0e-6);

  dec2phase(zero);

  /* start of the CPMG train for first period time_T2/2 on 2NyHz */
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
 
  /* change 2NyHz to Nx */
  delay(2.0e-6);
  zgradpulse(gzlvl6,gt6);
  delay(gstab);

  delay(taub - gt6 - gstab -2.0e-6 - pwn_cp - 10.0e-6  - pw_wg -2.0*POWER_DELAY
        - WFG_START_DELAY - WFG_STOP_DELAY - SAPS_DELAY);

  initval(1.0,v3);
  obsstepsize(phase_wg);
  xmtrphase(v3);
  obspower(tpwrwg);
  if (autocal[A] == 'y') 
   shaped_pulse("H2OsincA",pw_sl,t5,4.0e-6,0.0);
  else
  shaped_pulse(shape_wg,pw_wg,t5,4.0e-6,4.0e-6);
  obspower(tpwr);
  xmtrphase(zero);
 
  /* composite 1H 90y-180x-90y on top of 15N 180x */
  dec2rgpulse(pwn_cp-2*pw,zero,2.0e-6,0.0);
  sim3pulse(pw,0.0e-6,pw,one,zero,zero,0.0,0.0);
  sim3pulse(2*pw,0.0e-6,2*pw,t6,zero,zero,0.0,0.0);
  sim3pulse(pw,0.0e-6,pw,one,zero,zero,0.0,0.0);
  dec2rgpulse(pwn_cp-2*pw,zero,0.0,2.0e-6);
  /* composite 1H 90y-180x-90y on top of 15N 180x */

  initval(1.0,v3);
  obsstepsize(phase_wg);
  xmtrphase(v3);
  obspower(tpwrwg);
  if (autocal[A] == 'y') 
   shaped_pulse("H2OsincA",pw_sl,t5,4.0e-6,0.0);
  else
  shaped_pulse(shape_wg,pw_wg,t5,4.0e-6,4.0e-6);
  obspower(tpwr);
  xmtrphase(zero);
 
  delay(taub - gt6 - gstab -2.0e-6 - pwn_cp - 10.0e-6 - pw_wg -2.0*POWER_DELAY
        - WFG_START_DELAY - WFG_STOP_DELAY - SAPS_DELAY);

  delay(2.0e-6);
  zgradpulse(gzlvl6,gt6);
  delay(gstab);

  /* start of the CPMG train for second period time_T2/2 on Nx */
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

  delay(4.0e-6);
  dec2power(dhpwr2);            /* Set decoupler2 power back to dhpwr2 */

  dec2phase(t3);

  delay(2.0e-6);
  zgradpulse(gzlvl7,gt7);
  delay(gstab);

  dec2rgpulse(pwn,t3,0.0,0.0);

  txphase(zero); 
  decphase(one);
  dec2phase(zero);

  delay(2.0e-6);
  rgradient('z',gzlvl0);
  delay(tau1+50.0e-6);
  rgradient('z',0.0);
  delay(gstab);

  if(c_flg[A] == 'y') {
     decshaped_pulse(sh_ad,pwc_ad,zero,2.0e-6,2.0e-6);

     delay(taub - WFG_START_DELAY - 2.0e-6 - pwc_ad - 2.0e-6 - WFG_STOP_DELAY 
                - 2.0*pw - gt8 - 2.0*gstab -2.0e-6 - 4.0*GRADIENT_DELAY
           - 2.0e-6 - 50.0e-6 );
  }
  else
     delay(taub - 2.0*pw - gt8 - 2.0*gstab -2.0e-6 - 4.0*GRADIENT_DELAY
           - 2.0e-6 - 50.0e-6 );

  delay(2.0e-6);
  zgradpulse(1.0*gzlvl8,gt8);
  delay(gstab);

  rgpulse(2*pw,zero,0.0,0.0);

  delay(2.0e-6);
  rgradient('z',-1.0*gzlvl0);
  delay(tau1+50.0e-6);
  rgradient('z',0.0);
  delay(gstab);

  dec2rgpulse(2*pwn,zero,0.0,0.0);

  txphase(zero); 
  decphase(zero);
  dec2phase(t4);

  delay(2.0e-6);
  zgradpulse(-1.0*gzlvl8,gt8);
  delay(gstab);

  delay(taub - gt8 - gstab -2.0e-6 - 2.0*GRADIENT_DELAY + 2.0*GRADIENT_DELAY + 2.0e-6 + gstab + 50.0e-6);                   

  sim3pulse(pw,0.0e-6,pwn,two,zero,t4,0.0,0.0);

  txphase(zero);
  dec2phase(zero);

  delay(2.0e-6);
  zgradpulse(gzlvl9,gt9); delay(gstab);

  delay(taub - gt9 - gstab -2.0e-6);            /* delay=1/8J (XH)  */

  sim3pulse(2*pw,0.0e-6,2*pwn,zero,zero,zero,0.0,0.0);  

  delay(taub - gt9 - gstab -2.0e-6);            /* delay=1/8J (XH)  */

  txphase(one);
  dec2phase(one);

  delay(2.0e-6);
  zgradpulse(gzlvl9,gt9); delay(gstab);

  sim3pulse(pw,0.0e-6,pwn,one,zero,one,0.0,0.0);

  dec2phase(zero); txphase(zero);

  delay(2.0e-6);
  zgradpulse(gzlvl10,gt10); delay(gstab);

  delay(taua - gt10 - gstab -2.0e-6 - 0.5*(pwn-pw) );

  sim3pulse(2*pw,0.0e-6,2*pwn,zero,zero,zero,0.0,0.0);

  delay(taua - gt10 - gstab -2.0e-6);

  dec2phase(zero); txphase(zero);

  delay(2.0e-6);
  zgradpulse(gzlvl10,gt10); delay(gstab);

  rgpulse(pw,zero,0.0,0.0);

  delay(BigT1);

  rgpulse(2*pw,zero,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(icosel*gzlvl11,gt11); delay(gstab);

  decpower(0.0);                                 /* lower power on dec */
  dec2power(dpwr2);                               /* lower power on dec2 */

  delay(BigT1 - gt11 - 2.0*GRADIENT_DELAY - gstab -2.0e-6 - 2.0*POWER_DELAY);

/* acquire data */

status(C);
     setreceiver(t7);
}
