/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  hcch_tocsyP.c               

   This pulse sequence will allow one to perform the following
   experiment:

   3D HCCH-TOCSY utilising gradients but not coherence gradients.


   Correlates the sidechain aliphatic 13C resonances of a given amino acid.
   Uses isotropic 13C mixing.

   Standard features include maintaining the 13C carrier in the CaCb region
   throughout using off-res SLP pulses; full power square pulses on 13C 
   initially when 13CO excitation is irrelevant; square pulses on the Ca and
   CaCb with first null at 13CO; one lobe sinc pulses on 13CO with first null
   at Ca; optional 2H decoupling when CaCb magnetization is transverse and 
   during 1H shift evolution for 4 channel spectrometers.  

   pulse sequence: Bax, Clore and Gronenborn, JMR 88, 425 (1990)
   Kay, Xu, Muhandiram and Forman-Kay, JMR B101, 333 (1993)
   SLP pulses:     J Magn. Reson. 96, 94-102 (1992)

   Derived from gc_co_nh.c written by Robin Bendall, Varian, March 94 and 95
   based on hcchtocsy_3c_pfg_500.c written by Lewis Kay, Sept and Dec 92.  
   Revised and improved to a standard format by MRB, BKJ and GG for the 
   BioPack, January 1997. Increased and scaled spinlock field (GG).


   Efficient STUD+ decoupling is invoked with STUD='y' without need to 
   set any parameters.
   (STUD+ decoupling- Bendall & Skinner, JMR, A124, 474 (1997) )

   CC spinlock now called from decspinlock statement, permitting waveforms
   from shapelib. Pbox macros can prepare the waveform, allowing adiabatic
   decoupling for good bandwidth with reduced sample heating. E.Kupce sep01
   Added to BioPack, G.Gray sep01.


   STUD DECOUPLING.   SET STUD='y':
   Setting the flag STUD='y' overrides the decoupling parameters listed in
   dg2 and applies STUD+ decoupling instead.  In consequence is is easy
   to swap between the decoupling scheme you have been using to STUD+ for
   an easy comparison.  The STUD+ waveforms are calculated for your field
   strength at the time of BioPack installation and RF power is 
   calculated within the pulse sequence.  The calculations are for the most 
   peaks being greater than 90% of ideal. If you wish to compare different 
   decoupling schemes, the power level used for STUD+ can be obtained from 
   dps - subtract 3dB from that level to compare to decoupling schemes at
   a continuous RF level such as GARP.  The value of 90% has
   been set to limit sample heating as much as possible.  If you wish to 
   change STUD parameters, for example to increase the quality of decoupling
   (and RF heating) change the 95% level for the centerband
   by changing the relevant number in the macro makeSTUDpp and 
   rerun the macro (don't use 100% !!).  (At the time of writing STUD has
   been coded to use the coarse attenuator, because the fine attenuator
   is not presently included in the fail-safe calculation of decoupling 
   power which aborts an experiment if the power is too high - this may
   lower STUD efficiency a little).


   CHOICE OF DECOUPLING AND 2D MODES

   Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
   Set dm3 = 'nnn' for no 2H decoupling, or
   'nyn'  and dmm3 = 'cwc' for 2H decoupling. 

   Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
   t1 [1H]  and t2 [13C].

   The flag f1180/f2180 should be set to 'y' if t1 is to be started at
   halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
   'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
   set f1180 = 'n' for (0,0) in 1H and f2180 = 'n' for (0,0) in 13C.





   DETAILED INSTRUCTIONS FOR USE OF hcch_tocsyP


   1. Obtain a printout of the Philosopy behind the BioPack development,
   and General Instructions using the macro: 
   "printon man('BioPack') printoff".
   These Detailed Instructions for hcch_tocsyP may be printed using:
   "printon man('hcch_tocsyP') printoff".

   2. Apply the setup macro "hcch_tocsyP".  This loads the relevant parameter
   set and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral
   check. 


   3. Sufficient bandwidth is needed to cover the aliphatic region (50 ppm =
   10 kHz on 800 MHz spectrometers) and adequate for the CC J's.  The
   pulse sequence "hcch_tocsy.c" uses a "hard-coded" DIPSI-3 (constant
   amplitude modulation) with no parameter input except for the number
   of cycles. This version (hcch_tocsyP) uses the decspinlock function. 

   Parameters:
   mixpwr, mixpwrf, mixpat, mixdmf, mixres
   If Pbox is used to create the waveform, the *DEC file contains the
   values to be used for these parameters. The BioPack macro
   "setupshapes" creates one specific "cctocsy.DEC" file based on the
   values of pwC/pwClvl/compC. This macro also is run during the 
   AutoCalibration Process. The macro "setwurstparams" runs also, and
   it gets values from the "cctocsy.DEC" for the above paramters and
   updates the hcch_tocsyP.par appropriately. Thus, using the             
   "hcch_tocsyP" macro or button/menu entry will return a parameter set
   ready to go.


   4. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 35ppm, and N15 
   frequency on the amide region (120 ppm).  The C13 frequency remains at 
   35ppm, ie in the center of the aliphatic region throughout the sequence.

   5. The flag H2Opurge = 'y' is provided to bring H2O and other H1 z 
   magnetization to the xy plane for gradient suppression.

   6. taua (1.7 ms), taub (0.42 ms) and tauc (1 ms) were determined for
   alphalytic protease and are listed in dg2 for possible readjustment by
   the user.

   7. If 2H decoupling is used, the 2H lock signal may become unstable because
   of 2H saturation.  Check that a 1D spectrum is stable/reproducible as 
   when 2H decoupling is not used.  You might also check this for d2 and d3
   equal to values achieved at say 75% of their maximum.  Remember to return
   d2=d3=0 before starting a 2D/3D experiment.

 */



#include <standard.h>
#include <PpP.h>



static int phi3[2] = {0, 2},
	   phi5[4] = {0, 0, 2, 2},
	   rec[4]  = {0, 2, 2, 0};

static double d2_init = 0.0, d3_init = 0.0;



void pulsesequence()
{
  /* DECLARE AND LOAD VARIABLES */
  shape offC10P;
  char f1180[MAXSTR],		/* Flag to start t1 @ halfdwell */
       f2180[MAXSTR],		/* Flag to start t2 @ halfdwell */
       mixpat[MAXSTR],		/* Spinlock waveform            */
       H2Opurge[MAXSTR], stCdec[MAXSTR],	/* calls STUD+ waveforms from shapelib */
       STUD[MAXSTR];		/* apply automatically calculated STUD decoupling */

  int ncyc, t1_counter,		/* used for states tppi in t1 */
      first_FID, t2_counter;	/* used for states tppi in t2 */

  double tau1,			/*  t1 delay */
         tau2,			/*  t2 delay */
         ni = getval("ni"), ni2 = getval("ni2"),
	 stdmf = getval("dmf80"),	/* dmf for 80 ppm of STUD decoupling */
         rf80 = getval("rf80"),	/* rf in Hz for 80ppm STUD+ */
         taua = getval("taua"),	/* time delays for CH coupling evolution */
         taub = getval("taub"), tauc = getval("tauc"),
	 /* string parameter stCdec calls stud decoupling waveform from your shapelib. */
         studlvl,		/* coarse power for STUD+ decoupling */
         pwClvl = getval("pwClvl"),	/* coarse power for C13 pulse */
         pwC = getval("pwC"),	/* C13 90 degree pulse length at pwClvl */
         rf0,			/* maximum fine power when using pwC pulses */
         /* the following pulse length for the SLP pulse is automatically calculated   */
         /* by the macro "hcch_tocsyP".  The SLP pulse shape,"offC10P" is created       */
         /* by Pbox "on-the-fly"                                                       */
         pwC10,			/* 180 degree selective sinc pulse on CO(174ppm) */
         rf7,			/* fine power for the pwC10 ("offC10P") pulse */
         compC = getval("compC"),	/* adjustment for C13 amplifier compression */
         mixpwr = getval("mixpwr"), mixpwrf = getval("mixpwrf"), mixdmf = getval("mixdmf"),
         pwmix = getval("pwmix"), mixres = getval("mixres"),
	 pwNlvl = getval("pwNlvl"),		/* power for N15 pulses */
         pwN = getval("pwN"),	/* N15 90 degree pulse length at pwNlvl */
         sw1 = getval("sw1"), sw2 = getval("sw2"),
	 gt0 = getval("gt0"),	/* other gradients */
         gt3 = getval("gt3"), gt4 = getval("gt4"), gt5 = getval("gt5"),
         gt7 = getval("gt7"), gzlvl0 = getval("gzlvl0"), gzlvl3 = getval("gzlvl3"),
         gzlvl4 = getval("gzlvl4"), gzlvl5 = getval("gzlvl5"), gzlvl6 = getval("gzlvl6"),
         gzlvl7 = getval("gzlvl7");

  if ((getval("arraydim") < 1.5) || (ix == 1))
    first_FID = 1;
  else
    first_FID = 0;

  getstr("mixpat", mixpat);
  getstr("f1180", f1180);
  getstr("f2180", f2180);
  getstr("H2Opurge", H2Opurge);
  getstr("STUD", STUD);

  if (first_FID)		/* calculate the shape only once */
  {
    rf7 = 1.0e-6 * 80.5 * 600.0 / sfrq;
    offC10P = pbox_make("offC10P", "sinc180", rf7, 139.0 * dfrq, pwClvl, compC * pwC);
  }
  else
    offC10P = getRsh("offC10P");

  pwC10 = offC10P.pw;
  rf7 = offC10P.pwrf;


  /* 80 ppm STUD+ decoupling */
  strcpy(stCdec, "stCdec80");
  studlvl = pwClvl + 20.0 * log10(compC * pwC * 4.0 * rf80);
  studlvl = (int) (studlvl + 0.5);


  /*   LOAD PHASE TABLE    */
  settable(t3, 2, phi3);
  settable(t5, 4, phi5);
  settable(t11, 4, rec);



  /*   INITIALIZE VARIABLES   */

  if (dpwrf < 4095)
  {
    printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
    psg_abort(1);
  }

  if (pwC > (25.0e-6 * 600.0 / sfrq))
  {
    printf("Increase pwClvl so that pwC < 25*600/sfrq");
    psg_abort(1);
  }

  /* maximum fine power for pwC pulses */
  rf0 = 4095.0;

  /* CHECK VALIDITY OF PARAMETER RANGES */

  if ((dm[A] == 'y' || dm[B] == 'y'))
  {
    printf("incorrect dec1 decoupler flags! Should be 'nny' or 'nnn' ");
    psg_abort(1);
  }

  if ((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
  {
    printf("incorrect dec2 decoupler flags! Should be 'nnn' ");
    psg_abort(1);
  }

  if ((dm3[A] == 'y' || dm3[C] == 'y'))
  {
    printf("incorrect dec1 decoupler flags! Should be 'nyn' or 'nnn' ");
    psg_abort(1);
  }

  if (dpwr > 52)
  {
    printf("don't fry the probe, DPWR too large!  ");
    psg_abort(1);
  }

  if (pw > 80.0e-6)
  {
    printf("dont fry the probe, pw too high ! ");
    psg_abort(1);
  }

  if (pwN > 100.0e-6)
  {
    printf("dont fry the probe, pwN too high ! ");
    psg_abort(1);
  }


  /* PHASES AND INCREMENTED TIMES */

  /*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */
  if (phase1 == 2)
    tsadd(t3, 1, 4);
  if (phase2 == 2)
    tsadd(t5, 1, 4);


  /*  C13 TIME INCREMENTATION and set up f1180  */

  /*  Set up f1180  */
  tau1 = d2;
  if (f1180[A] == 'y')
  {
    tau1 += (1.0 / (2.0 * sw1));
    if (tau1 < 0.2e-6)
      tau1 = 0.0;
  }
  tau1 = tau1 / 2.0;

  /*  Set up f2180  */
  tau2 = d3;
  if (f2180[A] == 'y')
  {
    tau2 += (1.0 / (2.0 * sw2));
    if (tau2 < 0.2e-6)
      tau2 = 0.0;
  }
  tau2 = tau2 / 2.0;

  ncyc = getval("ncyc");
  if (ix < 2)
    printf("ncyc = %d, mix = %.6f\n", ncyc, (double) ncyc * pwmix);

  /* Calculate modifications to phases for States-TPPI acquisition          */
  if (ix == 1)
    d2_init = d2;
  t1_counter = (int) ((d2 - d2_init) * sw1 + 0.5);
  if (t1_counter % 2)
  {
    tsadd(t3, 2, 4);
    tsadd(t11, 2, 4);
  }

  if (ix == 1)
    d3_init = d3;
  t2_counter = (int) ((d3 - d3_init) * sw2 + 0.5);
  if (t2_counter % 2)
  {
    tsadd(t5, 2, 4);
    tsadd(t11, 2, 4);
  }

  /*   BEGIN PULSE SEQUENCE   */

  status(A);
  if (dm3[B] == 'y')
    lk_sample();
  if ((ni / sw1 - d2) > 0)
    delay(ni / sw1 - d2);	/*decreases as t1 increases for const.heating */
  if ((ni2 / sw2 - d3) > 0)
    delay(ni2 / sw2 - d3);	/*decreases as t2 increases for const.heating */
  delay(d1);
  if (dm3[B] == 'y')
  {
    lk_hold();
    lk_sampling_off();
  }				/*freezes z0 correction, stops lock pulsing */
  rcvroff();

  obspower(tpwr);
  decpower(pwClvl);
  dec2power(pwNlvl);
  decpwrf(rf0);
  obsoffset(tof);
  txphase(t3);
  delay(1.0e-5);

  decrgpulse(pwC, zero, 0.0, 0.0);	/*destroy C13 magnetization */
  zgradpulse(gzlvl0, 0.5e-3);
  delay(1.0e-4);
  decrgpulse(pwC, one, 0.0, 0.0);
  zgradpulse(0.7 * gzlvl0, 0.5e-3);
  delay(5.0e-4);

  if (dm3[B] == 'y')
  {
    dec3rgpulse(1 / dmf3, one, 10.0e-6, 2.0e-6);
    dec3unblank();
    dec3phase(zero);
    delay(2.0e-6);
    setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
  }
  rgpulse(pw, t3, 0.0, 0.0);	/* 1H pulse excitation */
  zgradpulse(gzlvl0, gt0);	/* 2.0*GRADIENT_DELAY */
  decphase(zero);
  delay(taua + tau1 - gt0 - 2.0 * GRADIENT_DELAY - 2.0 * pwC);
  decrgpulse(2.0 * pwC, zero, 0.0, 0.0);
  txphase(zero);
  delay(tau1);
  rgpulse(2.0 * pw, zero, 0.0, 0.0);
  zgradpulse(gzlvl0, gt0);
  txphase(one);
  decphase(t5);
  delay(taua - gt0);
  rgpulse(pw, one, 0.0, 0.0);
  zgradpulse(gzlvl3, gt3);
  delay(2.0e-4);
  decrgpulse(pwC, t5, 0.0, 0.0);
  delay(tau2);
  dec2rgpulse(2.0 * pwN, zero, 0.0, 0.0);
  zgradpulse(gzlvl4, gt4);	/* 2.0*GRADIENT_DELAY */
  decphase(zero);
  decpwrf(rf7);
  delay(taub - 2.0 * pwN - gt4 - 2.0 * GRADIENT_DELAY);
  decshaped_pulse("offC10P", pwC10, zero, 0.0, 0.0);
  txphase(zero);
  decpwrf(rf0);
  delay(taub - 2.0 * pw);
  rgpulse(2.0 * pw, zero, 0.0, 0.0);
  delay(tau2);
  decrgpulse(2.0 * pwC, zero, 0.0, 0.0);
  decpwrf(rf7);
  delay(taub);
  decshaped_pulse("offC10P", pwC10, zero, 0.0, 0.0);
  zgradpulse(gzlvl4, gt4);	/* 2.0*GRADIENT_DELAY */
  decpwrf(rf0);
  delay(taub - gt4 - 2.0 * GRADIENT_DELAY);
  decrgpulse(pwC, one, 0.0, 0.0);
  decpower(mixpwr);
  decpwrf(mixpwrf);
  if (ncyc > 0)
    decspinlock(mixpat, 1.0 / mixdmf, mixres, one, ncyc);
  decpower(pwClvl);
  decpwrf(rf0);
  if (H2Opurge[A] == 'y')
  {
    obspwrf(1000.0);
    rgpulse(900 * pw, zero, 0.0, 0.0);
    rgpulse(500 * pw, one, 0.0, 0.0);
    obspwrf(4095.0);
  }
  zgradpulse(gzlvl7, gt7);
  delay(50.0e-6);
  rgpulse(pw, zero, 0.0, 0.0);
  zgradpulse(gzlvl7, gt7 / 1.6);
  decrgpulse(pwC, three, 100.0e-6, 0.0);
  zgradpulse(gzlvl5, gt5);
  decphase(zero);
  delay(tauc - gt5);
  simpulse(2.0 * pw, 2.0 * pwC, zero, zero, 0.0, 0.0);
  zgradpulse(gzlvl5, gt5);
  delay(tauc - gt5);
  decrgpulse(pwC, zero, 0.0, 0.0);
  zgradpulse(gzlvl3, gt3);
  if (dm3[B] == 'y')
  {
    setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
    dec3rgpulse(1 / dmf3, three, 2.0e-6, 2.0e-6);
    dec3blank();
    lk_autotrig();		/* resumes lock pulsing */
  }
  delay(2.0e-4);
  rgpulse(pw, zero, 0.0, 0.0);
  zgradpulse(gzlvl6, gt5);
  delay(taua - gt5 + rof1);
  simpulse(2.0 * pw, 2.0 * pwC, zero, zero, 0.0, rof1);
  zgradpulse(gzlvl6, gt5);
  delay(taua - gt5 - 2.0 * pwC - 2.0 * POWER_DELAY);
  decrgpulse(pwC, zero, 0.0, 0.0);
  decrgpulse(pwC, one, 0.0, 0.0);
  if (STUD[A] == 'y')
    decpower(studlvl);
  else
    decpower(dpwr);
  dec2power(dpwr2);
  rgpulse(pw, zero, 0.0, rof2);
  rcvron();
  if (dm3[B] == 'y')
    lk_sample();
  setreceiver(t11);
  if ((STUD[A] == 'y') && (dm[C] == 'y'))
  {
        decunblank();
        decon();
        decprgon(stCdec,1/stdmf, 1.0);
        startacq(alfa);
        acquire(np, 1.0/sw);
        decprgoff();
        decoff();
        decblank();
    if (dm2[C] == 'y')
    {
      setstatus(DEC2ch, TRUE, dmm2[C], FALSE, dmf2);
    }
  }
  else
    status(C);
}
