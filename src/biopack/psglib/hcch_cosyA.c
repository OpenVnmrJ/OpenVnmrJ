/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  hcch_cosyA.c  rev 05

   3D HCCH-COSY with semi-constant evolution in  t1(H1) and t2 (C13) dimension.


   Correlates the adjacent sidechain aliphatic 13C resonances of a given amino acid.

   Standard features include maintaining the 13C carrier in the CaCb region
   throughout using off-res SLP pulses; full power square pulses on 13C 
   initially when 13CO excitation is irrelevant; square pulses on the Ca and
   CaCb with first null at 13CO; one lobe sinc pulses on 13CO with first null
   at Ca; optional 2H decoupling when CaCb magnetization is transverse and 
   during 1H shift evolution for 4 channel spectrometers.  

   pulse sequence: 
   SLP pulses:     J Magn. Reson. 96, 94-102 (1992)

   Efficient STUD+ decoupling is invoked with STUD='y' without need to 
   set any parameters.
   (STUD+ decoupling- Bendall & Skinner, JMR, A124, 474 (1997) and in press)

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

   Set dm  = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
   Set dm3 = 'nnn' for no 2H decoupling, or
       dm3 = 'nyn'  and dmm3 = 'cwc' for 2H decoupling. 

   Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
   t1 [1H]  and t2 [13C].

   The flag f1180/f2180 should be set to 'y' if t1 is to be started at
   halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
   'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
   set f1180 = 'n' for (0,0) in 1H and f2180 = 'n' for (0,0) in 13C.


   DETAILED INSTRUCTIONS FOR USE OF hcch_cosy


   1. Obtain a printout of the Philosopy behind the BioPack development,
   and General Instructions using the macro: 
   "printon man('BioPack') printoff".
   These Detailed Instructions for hcch_cosy may be printed using:
   "printon man('hcch_cosy') printoff".

   2. Apply the setup macro "hcch_cosy".  This loads the relevant parameter
   set and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral
   check.

   3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 35ppm, and N15 
   frequency on the amide region (120 ppm).  The C13 frequency remains at 
   35ppm, ie in the center of the aliphatic region throughout the sequence.

   5. H2O_flg = 'y' for H2O samples, 'n' for D2O samples

   6. If 2H decoupling is used, the 2H lock signal may become unstable because
   of 2H saturation.  Check that a 1D spectrum is stable/reproducible as 
   when 2H decoupling is not used.  You might also check this for d2 and d3
   equal to values achieved at say 75% of their maximum.  Remember to return
   d2=d3=0 before starting a 2D/3D experiment.

   7. The autocal and checkofs flags are generated automatically in Pbox_bio.h
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

   Written by Weixing Zhang,  November 1998
   Department of Structural Biology
   St. Jude Children's Research Hospital.
   Memphis, TN 39105
   (901)495-3169
   Weixing.Zhang@stjude.org
   modified for BioPack format by GG, palo alto, dec 1998
   Auto-calibrated version, E.Kupce, 27.08.2002.

   H2O_flg = y      For protein in H2O
   H2O_flg = n      For protein in D2O
*/

#include <standard.h>
#include "Pbox_bio.h"		/* Pbox Bio Pack Pulse Shaping Utilities */

extern int dps_flag;

static int phi1[2] =  {0, 2},
	   phi2[4] =  {0, 0, 2, 2},
	   phi3[16] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3},
	   phi4[2] =  {0, 2},
	   rec[8] =   {0, 2, 2, 0, 2, 0, 0, 2};

static double d2_init = 0.0, d3_init = 0.0;
static double H1ofs = 4.7, C13ofs = 35.0, N15ofs = 120.0, H2ofs = 0.0;

static shape offC10;

void pulsesequence()
{

  /* DECLARE AND LOAD VARIABLES */

  char f1180[MAXSTR],		/* Flag to start t1 @ halfdwell */
       f2180[MAXSTR],		/* Flag to start t2 @ halfdwell */
       mag_flg[MAXSTR],		/* Flag to use magic-angle gradients */
       H2O_flg[MAXSTR], stCdec[MAXSTR],		/* calls STUD+ waveforms from shapelib */
       STUD[MAXSTR];		/* apply automatically calculated STUD decoupling */

  int t1_counter,		/* used for states tppi in t1 */
      t2_counter;		/* used for states tppi in t2 */

  double tau1,			/*  t1 delay */
         tau2,			/*  t2 delay */
         delta1, delta2, TC = getval("TC"),	/*  3.5 ms  */
         ni = getval("ni"), ni2 = getval("ni2"),
	 stdmf = getval("dmf80"),	/* dmf for 80 ppm of STUD decoupling */
         rf80 = getval("rf80"),	/* rf in Hz for 80ppm STUD+ */
         taua = getval("taua"),	/* time delays for CH coupling evolution */
         taub = getval("taub"), tauc = getval("tauc"),
	 /* string parameter stCdec calls stud decoupling waveform from your shapelib. */
         studlvl,		/* coarse power for STUD+ decoupling */
         bw, ofs, ppm,		/* temporary Pbox parameters */
         pwClvl = getval("pwClvl"),	/* coarse power for C13 pulse */
         pwC = getval("pwC"),	/* C13 90 degree pulse length at pwClvl */
  /* the following pulse length for the SLP pulse is automatically calculated   */
  /* by the macro "hcch_cosy".  The SLP pulse shape,"offC10" is called          */
  /* directly from your shapelib.                                               */
         pwC10,			/* 180 degree selective sinc pulse on CO(174ppm) */
         rf7,			/* fine power for the pwC10 ("offC10") pulse */
         compC = getval("compC"),	/* adjustment for C13 amplifier compression */
         pwmax, pwNlvl = getval("pwNlvl"),	/* power for N15 pulses */
         pwN = getval("pwN"),	/* N15 90 degree pulse length at pwNlvl */
         sw1 = getval("sw1"), sw2 = getval("sw2"), gt1 = getval("gt1"),
         gt2 = getval("gt2"), gt3 = getval("gt3"), gt4 = getval("gt4"),
         gt5 = getval("gt5"), gt7 = getval("gt7"), gt8 = getval("gt8"),
         gt9 = getval("gt9"), gzcal = getval("gzcal"), gzlvl1 = getval("gzlvl1"),
         gzlvl2 = getval("gzlvl2"), gzlvl3 = getval("gzlvl3"), gzlvl4 = getval("gzlvl4"),
         gzlvl5 = getval("gzlvl5"), gzlvl7 = getval("gzlvl7"), gzlvl8 = getval("gzlvl8"),
         gzlvl9 = getval("gzlvl9");

  getstr("f1180", f1180);
  getstr("f2180", f2180);
  getstr("H2O_flg", H2O_flg);
  getstr("STUD", STUD);
  /* 80 ppm STUD+ decoupling */
  strcpy(stCdec, "stCdec80");
  studlvl = pwClvl + 20.0 * log10(compC * pwC * 4.0 * rf80);
  studlvl = (int) (studlvl + 0.5);


  /*   INITIALIZE VARIABLES   */

  if (dpwrf < 4095)
  {
    printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
    psg_abort(1);
  }

  setautocal();			/* activate auto-calibration flags */

  if (autocal[0] == 'n')
  {
    /* "offC10": 180 degree one-lobe sinc pulse on CO, null at Ca 139ppm away */
    pwC10 = getval("pwC10");
    rf7 = (compC * 4095.0 * pwC * 2.0 * 1.65) / pwC10;	/* needs 1.65 times more     */
    rf7 = (int) (rf7 + 0.5);	/* power than a square pulse */

    if (pwC > (24.0e-6 * 600.0 / sfrq))
    {
      printf("Increase pwClvl so that pwC < 24*600/sfrq");
      psg_abort(1);
    }
  }
  else
    /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
  {
    if (FIRST_FID)		/* call Pbox */
    {
      ppm = getval("dfrq");
      bw = 118.0 * ppm;
      ofs = 139.0 * ppm;
      offC10 = pbox_make("offC10", "sinc180n", bw, ofs, compC * pwC, pwClvl);
      if (dm3[B] == 'y')
	H2ofs = 3.2;
      ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
    }
    rf7 = offC10.pwrf;
    pwC10 = offC10.pw;
  }

  if ((dm3[A] == 'y' || dm3[C] == 'y'))
  {
    printf("incorrect dec1 decoupler flags! Should be 'nyn' or 'nnn' ");
    psg_abort(1);
  }


  getstr("f1180", f1180);
  getstr("f2180", f2180);
  getstr("mag_flg", mag_flg);
  getstr("H2O_flg", H2O_flg);

  pwmax = 2.0 * pwN;
  if (pwC10 > pwmax)
    pwmax = pwC10;

  /* check validity of parameter range */

  if ((dm[A] == 'y' || dm[B] == 'y'))
  {
    printf("incorrect Dec1 decoupler flags!  ");
    psg_abort(1);
  }

  if ((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
  {
    printf("incorrect Dec2 decoupler flags! Should be nnn  ");
    psg_abort(1);
  }

  if (dpwr > 50)
  {
    printf("don't fry the probe, dpwr too large!  ");
    psg_abort(1);
  }

  if (dpwr2 > 50)
  {
    printf("don't fry the probe, dpwr2 too large!  ");
    psg_abort(1);
  }

  /* LOAD VARIABLES */

  settable(t1, 2, phi1);
  settable(t2, 4, phi2);
  settable(t3, 16, phi3);
  settable(t4, 2, phi4);

  settable(t11, 8, rec);

  /* INITIALIZE VARIABLES */

  /* Phase incrementation for hypercomplex data */

  if (phase1 == 2)		/* Hypercomplex in t1 */
  {
    tsadd(t1, 1, 4);
  }

  if (phase2 == 2)
  {
    tsadd(t2, 1, 4);
  }

  /* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */

  if (ix == 1)
    d2_init = d2;
  t1_counter = (int) ((d2 - d2_init) * sw1 + 0.5);
  if (t1_counter % 2)
  {
    tsadd(t1, 2, 4);
    tsadd(t11, 2, 4);
  }

  /* calculate modification to phases based on current t2 values
   to achieve States-TPPI acquisition */

  if (ix == 1)
    d3_init = d3;
  t2_counter = (int) ((d3 - d3_init) * sw2 + 0.5);
  if (t2_counter % 2)
  {
    tsadd(t2, 2, 4);
    tsadd(t11, 2, 4);
  }

  /* set up so that get (90, -180) phase corrects in F1 if f1180 flag is y */

  tau1 = d2;
  if (f1180[A] == 'y')
  {
    tau1 += (1.0 / (2.0 * sw1));
  }
  if (tau1 < 1.0e-6)
    tau1 = 0.0;
  tau1 = tau1 / 2.0;

  /* set up so that get (90, -180) phase corrects in F2 if f2180 flag is y */

  tau2 = d3;
  if (f2180[A] == 'y')
  {
    tau2 += (1.0 / (2.0 * sw2));
  }
  if (tau2 < 1.0e-6)
    tau2 = 0.0;
  tau2 = tau2 / 2.0;

  if (ni > 1)
    delta1 = (double) (t1_counter * (taua - gt2 - 0.2e-3)) / ((double) (ni - 1));
  else
    delta1 = 0.0;
  if (ni2 > 1)
    delta2 = (double) (t2_counter * (TC - 0.6e-3)) / ((double) (ni2 - 1));
  else
    delta2 = 0.0;

  initval(7.0, v1);
  obsstepsize(45.0);

  /* BEGIN ACTUAL PULSE SEQUENCE */

  status(A);
  delay(10.0e-6);
  obspower(tpwr);
  decpower(pwClvl);
  decpwrf(4095.0);
  dec2power(pwNlvl);
  decphase(zero);
  dec2phase(zero);
  xmtrphase(v1);
  txphase(t1);
  if (dm3[B] == 'y')
    lk_sample();
  delay(d1);
  if (dm3[B] == 'y')
  {
    lk_hold();
    lk_sampling_off();
  }				/*freezes z0 correction, stops lock pulsing */
  rcvroff();

  if (gt1 > 0.2e-6)
  {
    decrgpulse(pwC, zero, rof1, rof1);
    delay(2.0e-6);
    zgradpulse(gzlvl1, gt1);
    delay(1.0e-3);
  }

  if (dm3[B] == 'y')		/* begins optional 2H decoupling */
  {
    dec3rgpulse(1 / dmf3, one, 10.0e-6, 2.0e-6);
    dec3unblank();
    dec3phase(zero);
    delay(2.0e-6);
    setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
  }

  status(B);
  rgpulse(pw, t1, 1.0e-4, 2.0e-6);
  xmtrphase(zero);
  zgradpulse(gzlvl2, gt2);
  delay(taua - gt2 - 2.0 * pwC - 2.0e-6 - SAPS_DELAY);
  txphase(zero);
  delay(tau1);
  decrgpulse(2.0 * pwC, zero, 0.0, 0.0);
  delay(tau1 - delta1);
  rgpulse(2.0 * pw, zero, 0.0, 2.0e-6);
  zgradpulse(gzlvl2, gt2);
  txphase(one);
  delay(taua - delta1 - gt2 - 2.0e-6);
  rgpulse(pw, one, 0.0, 2.0e-6);

  if (mag_flg[A] == 'y')
  {
    magradpulse(gzcal * gzlvl3, gt3);
  }
  else
  {
    zgradpulse(gzlvl3, gt3);
  }
  decphase(t2);
  txphase(zero);
  delay(200.0e-6);

  decrgpulse(pwC, t2, 2.0e-6, 0.0);

  decphase(zero);
  decpwrf(rf7);
  delay(tau2);
  sim3shaped_pulse("", "offC10", "", 0.0, pwC10, 2.0 * pwN, zero, zero, zero, 0.0, 0.0);
  delay(taub - pwmax - WFG_START_DELAY - WFG_STOP_DELAY - POWER_DELAY);
  rgpulse(2.0 * pw, zero, 0.0, 0.0);

  decphase(t3);
  decpwrf(4095.0);
  delay(TC - taub + tau2 - delta2 - 2.0 * pw - POWER_DELAY);
  decrgpulse(2.0 * pwC, t3, 0.0, 0.0);
  decphase(t4);
  delay(TC - delta2 - POWER_DELAY);

  decrgpulse(pwC, t4, 0.0, 2.0e-6);
  zgradpulse(gzlvl4, gt4);
  txphase(zero);
  decphase(zero);
  delay(tauc - gt4);
  decrgpulse(2.0 * pwC, zero, 0.0, 2.0e-6);

  if (H2O_flg[A] == 'y')
  {
    delay(tauc - gt4 - 500.0e-6 - POWER_DELAY);
    zgradpulse(gzlvl4, gt4);
    decphase(one);
    obspwrf(1000.0);
    delay(500.0e-6);
    decrgpulse(pwC, one, 0.0, 1.0e-6);
    rgpulse(900 * pw, one, rof1, 0.0);
    txphase(zero);
    rgpulse(500 * pw, zero, 2.0e-6, 2.0e-6);
    obspwrf(4095.0);
    if (mag_flg[A] == 'y')
    {
      magradpulse(gzcal * gzlvl5, gt5);
    }
    else
    {
      zgradpulse(gzlvl5, gt5);
    }
    decphase(one);
    delay(200.0e-6);
    simpulse(pw, pwC, zero, one, 0.0, 2.0e-6);
    zgradpulse(gzlvl7, gt7);
    decphase(zero);
    delay(taub - gt7);
    simpulse(2.0 * pw, 2.0 * pwC, zero, zero, 0.0, 2.0e-6);
    zgradpulse(gzlvl7, gt7);
    delay(taub - gt7);
  }
  else
  {
    delay(tauc - taub - 2.0 * pw - POWER_DELAY);
    rgpulse(2.0 * pw, zero, 0.0, 2.0e-6);
    zgradpulse(gzlvl4, gt4);
    delay(taub - gt4 - 2.0e-6);
  }

  decrgpulse(pwC, zero, 0.0, 2.0e-6);
  if (mag_flg[A] == 'y')
  {
    magradpulse(gzcal * gzlvl8, gt8);
  }
  else
  {
    zgradpulse(gzlvl8, gt8);
  }
  txphase(zero);
  delay(200.0e-6);
  if (dm3[B] == 'y')		/* turns off 2H decoupling  */
  {
    setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
    dec3rgpulse(1 / dmf3, three, 2.0e-6, 2.0e-6);
    dec3blank();
    lk_autotrig();		/* resumes lock pulsing */
  }
  rgpulse(pw, zero, 0.0, 2.0e-6);
  if (mag_flg[A] == 'y')
  {
    magradpulse(gzcal * gzlvl9, gt9);
  }
  else
  {
    zgradpulse(gzlvl9, gt9);
  }
  delay(taua - gt9);
  simpulse(2.0 * pw, 2.0 * pwC, zero, zero, 0.0, 2.0e-6);
  if (mag_flg[A] == 'y')
  {
    magradpulse(gzcal * gzlvl9, gt9);
  }
  else
  {
    zgradpulse(gzlvl9, gt9);
  }

  if (STUD[A] == 'y')
    decpower(studlvl);
  else
    decpower(dpwr);
  dec2power(dpwr2);

  delay(taua - gt9 - rof1 - 0.5 * pw - 2.0 * POWER_DELAY);
  rgpulse(pw, zero, rof1, rof2);
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
  setreceiver(t11);
}
