/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_hcch_cosyA.c - autocalibrated version with pulse shaping "on fly".

    This pulse sequence will allow one to perform the following experiment:

    2D or 3D HCCH-COSY or RELAY with gradients in H2O or D2O and with editing
    of spectral regions.

                      NOTE dof MUST BE SET AT 110ppm ALWAYS
                      NOTE dof2 MUST BE SET AT 200ppm ALWAYS

    pulse sequence:     Kay et al. JACS, 112, 888 (1990).
                        Clore et al. Biochemistry, 29, 8172 (1990).
                        Pardi et al. JACS, 114, 9202 (1992).
    STUD+ decoupling    Bendall & Skinner, JMR, A124, 474 (1997) and in press



    In all the above cases:
    * H2 coupling in triply-labelled samples is refocused by dm3='nyn'.
    * Efficient STUD+ decoupling is invoked with STUD='y' without need to
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

    	Set dm2 = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
	Set dm3 = 'nnn' for no 2H decoupling, or
		  'nyn'  and dmm3 = 'cwc' for 2H decoupling. 
  
    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [1H]  and t2 [13C].
    2D experiment in t1: wft2da
    2D experiment in t2: wft2d('ni2',1,0,0,0,0,0,-1,0)
    ( or with 5.2F or above just use wft2da or wft2da('ni2') after setting
      f2coef='1 0 0 0 0 0 -1 0'
     for 3D just use ft3da )
    2D transforms using 3D data set will transform properly with the
    standard wft2da macro.
    F1F3-plane: wft2d('ni',1,1,0,0,0,0,0,0,0,0,0,0,0,-1,0,0,0)
    F2F3-plane: wft2d('ni2',1,1,0,0,0,0,0,0,0,0,0,-1,0,0,0,0,0)

    
    The flag f1180/f2180 should be set to 'y' if t1 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180 = 'n' for (0,0) in 1H and f2180 = 'n' for (0,0) in 13C.


          	  DETAILED INSTRUCTIONS FOR USE OF rna_hcch_cosy

    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro:
                                      "printon man('RnaPack') printoff".
       These Detailed Instructions for rna_hcch_cosy may be printed using:
                                      "printon man('rna_hcch_cosy') printoff".

    2. Apply the setup macro "rna_hcch_cosy".  This loads the relevant parameter set
       and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), N15 frequency on the aromatic N
       region (200 ppm), and C13 frequency on 110ppm.

    4. H2O_flg = 'y' for H2O samples, 'n' for D2O samples

    5. CHOICE OF C13 REGION:
       ribose='y' gives a spectrum of ribose resonances centered on dof=80ppm.
                                                             Set sw  to  5ppm.
                                                             Set sw1 to  4ppm.
                                                             Set sw2 to 55ppm.

       aromatic='y' gives a spectrum of pyrimidine-aromatic groups. dof is shifted
       automatically by the pulse sequence code to 130ppm.   Set sw1 to 10ppm.
                                                             Set sw2 to 60ppm.

       Setting SHAPE='n' and ribose='y' folds pyrimidine-aromatic groups.
							     Set sw1 to 10ppm.
							     Set sw2 to 60ppm.

       Use of SHAPE is only recommended for optimizing the setup. Do not use for
       2D or 3D experiment!!!!

    6. DELAY TIMES (JCH and JCC):
       These are listed in dg/dg2 for possible change by the user. JCH is used
       to calculate the 1/4J time (lambda=0.94*1/4J) for H1 CH coupling evolution
       and the 1/3J time (tCH) for C13 evolution.
       Lambda is calculated a little lower (0.94) than the theoretical time to
       minimise relaxation.
       JCC is used to calculate the delay (tCC=1/8*J) for COSY transfer and
       delay (tCCr) for RELAY transfer.
         ribose CH/CH2:           JCH=145Hz         JCC = 35Hz
         aromatic CH:             JCH=180Hz         JCC = 70Hz

    7. CHOICE OF SEQUENCE:
       RELAY='y' adds relay transfer and allows you to run 2D or 3D HCCH RELAY.
       Not with aromatic='y'.
       For 2D set ni2=0.

    8. STUD DECOUPLING.   SET STUD='y':
       Setting the flag STUD='y' overrides the decoupling parameters listed in
       dg2 and applies STUD+ decoupling (80ppm) instead. For further information
       refer to rna_gChsqc.

    9. If 2H decoupling is used, the 2H lock signal may become unstable because
       of 2H saturation.  Check that a 1D spectrum is stable/reproducible as 
       when 2H decoupling is not used.  You might also check this for d2 and d3
       equal to values achieved at say 75% of their maximum.  Remember to return
       d2=d3=0 before starting a 2D/3D experiment.

   Written by Weixing Zhang,  November 1998
   Department of Structural Biology
   St. Jude Children's Research Hospital.
   Memphis, TN 39105
   (901)495-3169
   Weixing.Zhang@stjude.org
   modified for BioPack format by GG, palo alto, dec 1998
   modified for RnaPack by Peter Lukavsky, Stanford, june 1999
   Auto-calibrated version, E.Kupce, 27.08.2002.
*/

#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */

extern int dps_flag;

static int 
           phi1[2] =  {0,2},
	   phi2[4] =  {0,0,2,2},
	   phi3[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
           phi4[2] =  {0,2},

	   rec[8] =  {0,2,2,0,2,0,0,2};

static double 	d2_init = 0.0, d3_init = 0.0;
static double   H1ofs=4.7, C13ofs=110.0, N15ofs=200.0, H2ofs=0.0;

static shape  stC50;

void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            RELAY[MAXSTR],                         /* Insert HCCH-relay delay */
            ribose[MAXSTR],                         /* ribose CHn groups only */
            aromatic[MAXSTR],                     /* aromatic CHn groups only */
            rna_stCshape[MAXSTR],     /* calls sech/tanh pulses from shapelib */
            rna_stCdec[MAXSTR],        /* calls STUD+ waveforms from shapelib */
	    mag_flg[MAXSTR],             /* Flag to use magic-angle gradients */
	    H2O_flg[MAXSTR],
	    sspul[MAXSTR],
            SHAPE[MAXSTR],
	    STUD[MAXSTR];   /* apply automatically calculated STUD decoupling */
 
int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
            ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
            delta1,delta2,
               ni = getval("ni"),
            lambda = 0.94/(4*getval("JCH")),       /* 1/4J H1 evolution delay */
            tCH = 1/(6.0*getval("JCH")),          /* 1/4J C13 evolution delay */
            tCC = 1/(8*getval("JCC")),

        pwClvl = getval("pwClvl"),              /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
   rfC,                           /* maximum fine power when using pwC pulses */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */
                               /* temporary Pbox parameters */
   bw, pws, ofs, ppm, nst,     /* bandwidth, pulsewidth, offset, ppm, # steps */

/* Sech/tanh inversion pulses automatically calculated by macro "rna_cal"     */
/* and string parameter rna_stCshape calls them from your shapelib.           */
   rfst = 0.0,          /* fine power for the rna_stCshape pulse, initialised */
   dofa,        /* dof shifted to 80 or 120ppm for ribose or aromatic spectra */

/* string parameter stCdec calls stud decoupling waveform from your shapelib.*/
   studlvl,	                         /* coarse power for STUD+ decoupling */
            stdmf = getval("dmf80"),     /* dmf for 80 ppm of STUD decoupling */
            rf80 = getval("rf80"),                /* rf in Hz for 80ppm STUD+ */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gt7 = getval("gt7"),
	gt8 = getval("gt8"),
	gt9 = getval("gt9"),
        gzcal = getval("gzcal"),
	grecov = getval("grecov"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),

	gzlvl7 = getval("gzlvl7"),		/* triax option */
	gzlvl8 = getval("gzlvl8"),
	gzlvl9 = getval("gzlvl9");

    getstr("f1180",f1180);
    getstr("mag_flg",mag_flg);
    getstr("f2180",f2180);
    getstr("RELAY",RELAY);
    getstr("ribose",ribose);
    getstr("aromatic",aromatic);
    getstr("H2O_flg",H2O_flg);
    getstr("sspul",sspul);
    getstr("SHAPE",SHAPE);
    getstr("STUD",STUD);


/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses */
        rfC = 4095.0;

  setautocal();                        /* activate auto-calibration flags */ 

  if (autocal[0] == 'n') 
  {
      /* 50ppm sech/tanh inversion */
      rfst = (compC*4095.0*pwC*4000.0*sqrt((7.5*sfrq/600+3.85)/0.41));
      rfst = (int) (rfst + 0.5);
  }
  else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
  {
    if(FIRST_FID)                                            /* call Pbox */
    {
      ppm = getval("dfrq"); 
      bw = 50.0*ppm; pws = 0.001; ofs = 0.0; nst = 500.0; 
      stC50 = pbox_makeA("rna_stC50", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
      if (dm3[B] == 'y') H2ofs = 3.2;
      ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
    }
    rfst = stC50.pwrf; 
  }
        strcpy(rna_stCshape, "rna_stC50");
        strcpy(rna_stCdec, "wurst80");
        studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
        studlvl = (int) (studlvl + 0.5);

/*  RIBOSE spectrum only, centered on 80ppm. */
  if (ribose[A]=='y')
        dofa = dof - 30.0*dfrq;

/*  AROMATIC spectrum only, centered on 120ppm */
  else
        dofa = dof + 10*dfrq;


/* CHECK VALIDITY OF PARAMETER RANGES */

  if( dpwrf < 4095 )
        { printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
          psg_abort(1); }

  if( pwC > (24.0e-6*600.0/sfrq) )
        { printf("Increase pwClvl so that pwC < 24*600/sfrq");
          psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y' ))
	{ printf("incorrect Dec1 decoupler flags!  "); psg_abort(1);}

  if((dm2[A] == 'y' || dm2[B] == 'y'))
	{ printf("incorrect Dec2 decoupler flags!  "); psg_abort(1);}

  if((dm3[A] == 'y' || dm3[C] == 'y' ))
	{printf("incorrect dec1 decoupler flags! Should be 'nyn' or 'nnn' ");
								psg_abort(1); }
  if ((dm3[B] == 'y'  &&   dpwr3 > 44 ))
	{ printf("Deuterium decoupling power too high ! "); psg_abort(1);}

  if( dpwr > 50 )
	{ printf("don't fry the probe, dpwr too large!  "); psg_abort(1);}

  if( dpwr2 > 50 )
	{ printf("don't fry the probe, dpwr2 too large!  "); psg_abort(1); }


/*  CHOICE OF PULSE SEQUENCE */

  if ( ((ribose[A]=='y') && (aromatic[A]=='y')) )
   { text_error("Choose  ONE  of  ribose='y'  OR  aromatic='y' ! ");
        psg_abort(1); }

  if ( ((aromatic[A]=='y') && (RELAY[A]=='y')) )
   { text_error("No RELAY with aromatic='y' ! ");
        psg_abort(1); }


/* LOAD VARIABLES */

   settable(t1, 2, phi1);
   settable(t2, 4, phi2);
   settable(t3, 16, phi3);
   settable(t4, 2, phi4);
   settable(t11,8, rec);


/* INITIALIZE VARIABLES */

/* Phase incrementation for hypercomplex data */

   if ( phase1 == 2 )     /* Hypercomplex in t1 */
     tsadd(t1,1,4);

   if ( phase2 == 2 )
     tsadd(t2,1,4);

/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */

   if(ix == 1) d2_init = d2;
   t1_counter = (int)((d2-d2_init)*sw1 + 0.5);
   if(t1_counter % 2)     
   {
       tsadd(t1,2,4);
       tsadd(t11,2,4);
   }

/* calculate modification to phases based on current t2 values
   to achieve States-TPPI acquisition */

   if(ix == 1) d3_init = d3;
   t2_counter = (int)((d3-d3_init)*sw2 + 0.5);
   if(t2_counter % 2)     
   {
       tsadd(t2,2,4);
       tsadd(t11,2,4);
   }
 
/* set up so that get (90, -180) phase corrects in F1 if f1180 flag is y */

   tau1 = d2;
   if(f1180[A] == 'y')
   {
      tau1 += (1.0/(2.0*sw1));
   }
   if (tau1 < 1.0e-6) tau1 = 0.0;
   tau1 = tau1/2.0;

/* set up so that get (90, -180) phase corrects in F2 if f2180 flag is y */

   tau2 = d3;
   if(f2180[A] == 'y')
   {
      tau2 += (1.0/(2.0*sw2));
   }
   if (tau2 < 1.0e-6) tau2 = 0.0;
   tau2 = tau2/2.0;

   if (ni > 1)
      delta1 = (double)(t1_counter*(lambda - gt5 - 0.2e-3))/((double)(ni-1));
   else delta1 = 0.0;
   if (ni2 > 1)
      delta2 = (double)(t2_counter*(tCC - 0.6e-3))/((double)(ni2-1));
   else delta2 = 0.0;

   initval(7.0, v1);
   obsstepsize (45.0);

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obsoffset(tof);
   decoffset(dofa);
   dec2offset(dof2);
   obspower(tpwr-12);                   
   decpower(pwClvl);
   decpwrf(rfC);
   dec2power(pwNlvl);                  
   decphase(zero);
   dec2phase(zero);
   if (sspul[0] == 'y')
   {
      rgpulse(200*pw, one, 10.0e-6, 0.0e-6);
      rgpulse(200*pw, zero, 0.0e-6, 1.0e-6);
   }
   obspower(tpwr);                   
   xmtrphase(v1);
   txphase(t1);
   if (dm3[B] == 'y') lk_sample();
   delay(d1);
   if (dm3[B] == 'y') lk_hold();
   rcvroff();

       decrgpulse(pwC, zero, rof1, rof1);
       delay(rof1);
       zgradpulse(gzlvl0,0.5e-3); 
       delay(grecov);

   if(dm3[B] == 'y')				  /*optional 2H decoupling on */
        { 
          dec3unblank();
          dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank();
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
         } 

status(B);
	rgpulse(pw, t1, 1.0e-4, rof1);
	xmtrphase(zero);
	txphase(zero);

	zgradpulse(gzlvl5,gt5); 

/*
        decpwrf(rfst);
        delay(lambda - gt5 - rof1 - SAPS_DELAY - GRADIENT_DELAY - POWER_DELAY -
                        WFG2_START_DELAY - 0.5e-3 + 70.0e-6 + tau1);

        decshaped_pulse(rna_stCshape, 1.0e-3, zero, 0.0, 0.0);

        delay(tau1 - delta1);

        rgpulse(2.0*pw, zero, 0.0, rof1);
        txphase(one);
        decpwrf(rfC);

        zgradpulse(gzlvl5,gt5);
        delay(lambda - delta1 - gt5 - rof1 - GRADIENT_DELAY - POWER_DELAY - 0.5e-3 + 70.0e-6);
*/


        delay(lambda - gt5 - rof1 - SAPS_DELAY - GRADIENT_DELAY + tau1);

        decrgpulse(2*pwC, zero, 0.0, 0.0);

        delay(tau1 - delta1);

        rgpulse(2.0*pw, zero, 0.0, rof1);
        txphase(one);

        zgradpulse(gzlvl5,gt5);
        delay(lambda - delta1 - gt5 - rof1 - GRADIENT_DELAY);

	rgpulse(pw, one, 0.0, rof1);
	decphase(t2);
	txphase(zero);

   if(mag_flg[A] == 'y')
      magradpulse(gzcal*gzlvl3,gt3); 
   else
      zgradpulse(gzlvl3,gt3); 

	delay(grecov);

	decrgpulse(pwC, t2, rof1, 0.0);
	decphase(zero);

	delay(tau2);
	
	dec2rgpulse(2.0*pwN,zero,0.0,0.0);

	delay(tCH - 2*pwN);

	rgpulse(2.0*pw, zero, 0.0, 0.0);
	decphase(t3);

	delay(tCC - tCH + tau2 - delta2 - 2.0*pw);
	
	decrgpulse(2.0*pwC, t3, 0.0, 0.0);
	decphase(t4);

	delay(tCC - delta2);

	decrgpulse(pwC, t4, 0.0, rof1);
	txphase(zero);
	decphase(zero);

  if(RELAY[A] == 'y')
   {
        zgradpulse(gzlvl4, gt4);
        delay(tCC - gt4 - GRADIENT_DELAY - pwC);

        decrgpulse(2.0*pwC, zero, 0.0, 0.0);

        zgradpulse(gzlvl4, gt4);
        delay(tCC - gt4 - GRADIENT_DELAY - pwC);

        decrgpulse(pwC, zero, 0.0, 0.0);
   }

	zgradpulse(gzlvl4,gt4); 
   	delay(tCC - gt4);

   	decrgpulse(2.0*pwC, zero, 0.0, rof1);

   if (H2O_flg[A] == 'y')
   {
      delay(tCC - gt4 - grecov - POWER_DELAY);

      zgradpulse(gzlvl4,gt4); 

      txphase(one);
      decphase(one);
      delay(grecov);

      decrgpulse(pwC, one, 0.0, rof1);
      rgpulse(900*pw, one, 0.0, rof1);
      txphase(zero);

      rgpulse(500*pw, zero, rof1, rof1);
      decphase(one);

      if(mag_flg[A] == 'y')
         magradpulse(gzcal*gzlvl7,gt7);
      else
         zgradpulse(gzlvl7,gt7);
      delay(200.0e-6);

      simpulse(pw, pwC, zero, one, 0.0, rof1);
      decphase(zero);

      zgradpulse(gzlvl4,gt4); 
      delay(tCH - gt4);

      simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, rof1);

      zgradpulse(gzlvl4,gt4); 
      delay(tCH - gt4);
   }
   else
   {
      delay(tCC - tCH - 2.0*pw - POWER_DELAY);
      rgpulse(2.0*pw, zero, 0.0, rof1);

      zgradpulse(gzlvl4,gt4); 
      delay(tCH - gt4 - rof1);
   }
   
	decrgpulse(pwC, zero, 0.0, rof1);
	txphase(zero);

   if(mag_flg[A] == 'y')
      magradpulse(gzcal*gzlvl8,gt8); 
   else
      zgradpulse(gzlvl8,gt8); 

	delay(grecov);

        if(dm3[B] == 'y')	         /*optional 2H decoupling off */
        {
         dec3rgpulse(1/dmf3, three, 0.0, 0.0);
         dec3blank();
         setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
         dec3blank();
        }

	rgpulse(pw, zero, 0.0, rof1);

  if (SHAPE[A] =='y')
   {
	decpwrf(rfst);

   if(mag_flg[A] == 'y')
      magradpulse(gzcal*gzlvl9,gt9);
   else
      zgradpulse(gzlvl9,gt9);

	delay(lambda - gt9 - GRADIENT_DELAY - POWER_DELAY - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

        simshaped_pulse("",rna_stCshape,2*pw, 1.0e-3, zero, zero, 0.0, rof1);
        decphase(zero);

   if(mag_flg[A] == 'y')
      magradpulse(gzcal*gzlvl9,gt9);
   else
      zgradpulse(gzlvl9,gt9);

	decpwrf(rfC);

if (STUD[A]=='y') decpower(studlvl); else decpower(dpwr);

	delay(lambda - gt9 -rof1 -0.5*pw - 2*POWER_DELAY - GRADIENT_DELAY - 0.5e-3 + 70.0e-6);
   }
  else
   {
   if(mag_flg[A] == 'y')
      magradpulse(gzcal*gzlvl9,gt9);
   else
      zgradpulse(gzlvl9,gt9);

        delay(lambda - gt9 - GRADIENT_DELAY);

        simpulse(2*pw, 2*pwC, zero, zero, 0.0, rof1);

   if(mag_flg[A] == 'y')
      magradpulse(gzcal*gzlvl9,gt9);
   else
      zgradpulse(gzlvl9,gt9);

if (STUD[A]=='y') decpower(studlvl); else decpower(dpwr);

        delay(lambda - gt9 -rof1 -0.5*pw - POWER_DELAY - GRADIENT_DELAY);
     } 

	rgpulse(pw, zero, rof1, rof2);
	rcvron();
if (dm3[B] == 'y') lk_sample();
	setreceiver(t11);

if ((STUD[A]=='y') && (dm[C] == 'y'))
	{
        decunblank();
        decon();
        decprgon(rna_stCdec,1/stdmf, 1.0);
        startacq(alfa);
        acquire(np, 1.0/sw);
        decprgoff();
        decoff();
        decblank();
        }
   else	
	 status(C);
setreceiver(t11);
}
