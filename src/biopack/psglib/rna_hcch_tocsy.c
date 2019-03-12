/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_hcch_tocsy.c               

    This pulse sequence will allow one to perform the following
    experiment:

    3D HCCH-TOCSY utilising gradients but not coherence gradients.

    Correlates ribose/aromatic 13C resonances of a given ribonucleotide.
    Uses isotropic 13C mixing.

    Standard features include maintaining the C13 carrier in the ribose/aromatic
    region throughout.
    Optional 2H decoupling when C13 magnetization is transverse and 
    during 1H shift evolution for 4 channel spectrometers.  
 
    pulse sequence: Bax, Clore and Gronenborn, JMR 88, 425 (1990)
                    Kay, Xu, Muhandiram and Forman-Kay, JMR B101, 333 (1993)

    Based on hcchtocsy_3c_pfg_500.c written by Lewis Kay, Sept and Dec 92.  
    Revised and improved to a standard format by MRB, BKJ and GG for the 
    BioPack, January 1997. Increased and scaled spinlock field (GG).


    Efficient STUD+ decoupling is invoked with STUD='y' without need to 
    set any parameters.
    (STUD+ decoupling- Bendall & Skinner, JMR, A124, 474 (1997) and in press)
     
    STUD DECOUPLING.   SET STUD='y':
       Setting the flag STUD='y' overrides the decoupling parameters listed in
       dg2 and applies STUD+ decoupling instead.  In consequence is is easy
       to swap between the decoupling scheme you have been using to STUD+ for
       an easy comparison.  The STUD+ waveforms are calculated for your field
       strength at the time of RnaPack installation and RF power is 
       calculated within the pulse sequence.  The calculations are for the most 
       peaks being greater than 90% of ideal. If you wish to compare different 
       decoupling schemes, the power level used for STUD+ can be obtained from 
       dps - subtract 3dB from that level to compare to decoupling schemes at
       a continuous RF level such as GARP.  The value of 90% has
       been set to limit sample heating as much as possible.  If you wish to 
       change STUD parameters, for example to increase the quality of decoupling
       (and RF heating) change the 95% level for the centerband
       by changing the relevant number in the macro rna_makeSTUDpp and 
       rerun the macro (don't use 100% !!).  (At the time of writing STUD has
       been coded to use the coarse attenuator, because the fine attenuator
       is not presently included in the fail-safe calculation of decoupling 
       power which aborts an experiment if the power is too high - this may
       lower STUD efficiency a little).


        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.
    Not for STUD='y'
    	Set dm2 = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
	Set dm3 = 'nnn' for no 2H decoupling, or
		  'nyn'  and dmm3 = 'cwc' for 2H decoupling. 
  
    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [1H]  and t2 [13C].
    
    The flag f1180/f2180 should be set to 'y' if t1 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180 = 'n' for (0,0) in 1H and f2180 = 'n' for (0,0) in 13C.





          	  DETAILED INSTRUCTIONS FOR USE OF rna_hcch_tocsy


    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro: 
                                      "printon man('RnaPack') printoff".
       These Detailed Instructions for rna_hcch_tocsy may be printed using:
                                      "printon man('rna_hcch_tocsy') printoff".
             
    2. Apply the setup macro "rna_hcch_tocsy".  This loads the relevant parameter
       set and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral
       check.

    3. The parameter ncyc corresponds to the number of cycles of DIPSI-3 mixing.
       ribose='y':	mixing time = 25-30ms (35 ppm rf field).
       AH2H8='y':	mixing time = 100ms (30 ppm rf field).

       The DIPSI rf field should be sufficient to cover the ribose or
       C2-C4-C6-C8 aromatic region and adequate for the CC J's.
      
    4. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 110ppm, and N15 
       frequency on the aromatic N region (200 ppm).
       ribose='y' sets dof to 80ppm.				Set sw=5ppm.
								Set sw1=5ppm.
								Set sw2=55ppm.
       AH2H8='y' sets dof to 145ppm.                            Set sw=10ppm.
                                                                Set sw1=4ppm.
                                                                Set sw2=30ppm.

    5. The 90 degree carbon pulse following the DIPSI mixing is phase cycled
       when nt=8 to eliminate z magnetization.  However this should be removed 
       anyway by the 90/gradient pair, so 4 transients should suffice if 
       sufficient S/N is available.

    6. The flag H2Opurge = 'y' is provided to bring H2O and other H1 z 
       magnetization to the xy plane for gradient suppression.

    7. taua (1.7 ms), taub (0.42 ms) and tauc (1 ms) are a starting point
       The values are listed in dg2 for possible readjustment by the user.

    8. If 2H decoupling is used, the 2H lock signal may become unstable because
       of 2H saturation.  Check that a 1D spectrum is stable/reproducible as 
       when 2H decoupling is not used.  You might also check this for d2 and d3
       equal to values achieved at say 75% of their maximum.  Remember to return
       d2=d3=0 before starting a 2D/3D experiment.

        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        @                                                      @
        @   Rewritten for RnaPack by Peter Lukavsky (10/98).   @
        @                                                      @
        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

*/



#include <standard.h>



static int   phi3[2]  = {0,2},
             phi5[4]  = {0,0,2,2},
	     phi9[8]  = {1,1,1,1,3,3,3,3},
             rec[8]   = {0,2,2,0,2,0,0,2};

static double   d2_init=0.0, d3_init=0.0;



void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
	    ribose[MAXSTR],	 		    /* ribose CHn groups only */
	    AH2H8[MAXSTR],		         /* Adenine H2-H8 correlation */
	    H2Opurge[MAXSTR],
	    rna_stCdec[MAXSTR],	       /* calls STUD+ waveforms from shapelib */
	    STUD[MAXSTR];   /* apply automatically calculated STUD decoupling */
 
int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter;  	 	        /* used for states tppi in t2 */

double      tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
            stdmf = getval("dmf80"),     /* dmf for 80 ppm of STUD decoupling */
            rf80 = getval("rf80"),       	  /* rf in Hz for 80ppm STUD+ */
	    taua = getval("taua"),   /* time delays for CH coupling evolution */
	    taub = getval("taub"),
	    tauc = getval("tauc"),

/* string parameter rna_stCdec calls stud decoupling waveform from your shapelib.*/
   studlvl,	                         /* coarse power for STUD+ decoupling */

	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	rfC,            	  /* maximum fine power when using pwC pulses */
	dofa,	    /* dof shifted to 80 ppm for ribose and 145 ppm for AH2H8 */
	tofa,	    		    /* tof shifted to 7.5 ppm in t1 for AH2H8 */

/* p_d is used to calculate the isotropic mixing on the C-ribose region */
        p_d,                 	       /* 50 degree pulse for DIPSI-3 at rfdC */
        rfdC,               /* fine power for 7.5 kHz or 4.0 kHz rf at 500MHz */
	ncyc = getval("ncyc"), 			  /* no. of cycles of DIPSI-3 */

   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

 grecov = getval("grecov"),   /* Gradient recovery delay, typically 150-200us */

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gt7 = getval("gt7"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6"),
	gzlvl7 = getval("gzlvl7");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("ribose",ribose);
    getstr("AH2H8",AH2H8);
    getstr("H2Opurge",H2Opurge);
    getstr("STUD",STUD);

/*   LOAD PHASE TABLE    */

	settable(t3,2,phi3);
	settable(t5,4,phi5);
	settable(t9,8,phi9);
	settable(t11,8,rec);


/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses */
	rfC = 4095.0;

  if (ribose[A] == 'y')
   {
/*  Center dof in RIBOSE region on 80ppm. */
	tofa = tof;
        dofa = dof - 30.0*dfrq;
		
/* dipsi-3 decoupling on C-ribose */	
 	p_d = (5.0)/(9.0*4.0*7000.0*(sfrq/800));  /* 35ppm DIPSI-3 */
     	rfdC = (compC*4095.0*pwC*5.0)/(p_d*9.0); 
	rfdC = (int) (rfdC + 0.5);
  	ncyc = (int) (ncyc + 0.5);
   }
  else
   {
/*  Center dof in adenine C2-C4-C6-C8-C5 region on 145 ppm. */
	tofa = tof + 2.5*sfrq;
        dofa = dof + 35.0*dfrq;

/* dipsi-3 decoupling on C-aromatic */
        p_d = (5.0)/(9.0*4.0*8000.0*(sfrq/800));  /* 40ppm DIPSI-3 */
        rfdC = (compC*4095.0*pwC*5.0)/(p_d*9.0);
        rfdC = (int) (rfdC + 0.5);
        ncyc = (int) (ncyc + 0.5);
   }

/* 80 ppm STUD+ decoupling */
        strcpy(rna_stCdec, "wurst80");
        studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
        studlvl = (int) (studlvl + 0.5);


/* CHECK VALIDITY OF PARAMETER RANGES */


    if((ribose[A] == 'y' && AH2H8[A] == 'y' ))
    { text_error("Choose either ribose='y' or AH2H8='y' !! ");
        psg_abort(1); }

    if((dm[A] == 'y' || dm[B] == 'y' ))
    { text_error("incorrect dec1 decoupler flags! Should be 'nny' or 'nnn' ");
        psg_abort(1); }

    if((dm2[A] == 'y' || dm2[B] == 'y'))
    { text_error("incorrect dec2 decoupler flags! Should be 'nny' or 'nnn' ");
        psg_abort(1); }

    if( (((dm[C] == 'y') && (dm2[C] == 'y')) && (STUD[A] == 'y')) )
    { text_error("incorrect dec2 decoupler flags! Should be 'nnn' if STUD='y'"); psg_abort(1); }

    if((dm3[A] == 'y' || dm3[C] == 'y' ))
    { text_error("incorrect dec1 decoupler flags! Should be 'nyn' or 'nnn' "); psg_abort(1); }

    if( dpwr > 50 )
    { text_error("don't fry the probe, DPWR too large!  "); psg_abort(1); }

    if( dpwr2 > 50 )
    { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }

    if( (pw > 20.0e-6) && (tpwr > 56) )
    { text_error("don't fry the probe, pw too high ! "); psg_abort(1); }

    if( (pwC > 40.0e-6) && (pwClvl > 56) )
    { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }

    if( (pwN > 100.0e-6) && (pwNlvl > 56) )
    { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }
 
    if ((dm3[B] == 'y'  &&   dpwr3 > 44 ))
    { text_error ("Deuterium decoupling power too high ! "); psg_abort(1); }

    if ((ncyc > 1 ) && (ix == 1))
    { text_error("mixing time is %f ms.\n",(ncyc*97.8*4*p_d)); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2) 
	 tsadd(t3,1,4);  
    if (phase2 == 2) 
	 tsadd(t5,1,4);

/*  C13 TIME INCREMENTATION and set up f1180  */

/*  Set up f1180  */
   
    tau1 = d2;

    if(f1180[A] == 'y') 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }

    tau1 = tau1/2.0;


/*  Set up f2180  */

    tau2 = d3;

    if(f2180[A] == 'y') 
	{ tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }

    tau2 = tau2/2.0;



/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t11,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t5,2,4); tsadd(t11,2,4); }



/*   BEGIN PULSE SEQUENCE   */

status(A);
        if (dm3[B] == 'y') lk_sample();
	delay(d1);
        if (dm3[B] == 'y') lk_hold();
	rcvroff();

	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	decpwrf(rfC);
	obsoffset(tofa);
	decoffset(dofa);
	dec2offset(dof2);
	txphase(t3);
	delay(1.0e-5);

	decrgpulse(pwC, zero, 0.0, 0.0);	   /*destroy C13 magnetization*/
	zgradpulse(gzlvl0, 0.5e-3);
	delay(grecov/2);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

        if (dm3[B] == 'y')	  /*optional 2H decoupling on */
        {
          dec3unblank();
          dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank();
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
         } 

	rgpulse(pw, t3, 0.0, 0.0);                    /* 1H pulse excitation */
	zgradpulse(gzlvl0, gt0);		/* 2.0*GRADIENT_DELAY */	
        decphase(zero);
	delay(taua + tau1 - gt0 - 2.0*GRADIENT_DELAY - 2.0*pwC);
        decrgpulse(2.0*pwC, zero, 0.0, 0.0);
        txphase(zero);
	delay(tau1);
	rgpulse(2.0*pw, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, gt0);
        txphase(one);
	decphase(t5);
	obsoffset(tof);
	delay(taua - gt0);
	rgpulse(pw, one, 0.0, 0.0);
	zgradpulse(gzlvl3, gt3);
	delay(grecov);
        decrgpulse(pwC, t5, 0.0, 0.0);
	delay(tau2);
	dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
	zgradpulse(gzlvl4, gt4);		        /* 2.0*GRADIENT_DELAY */
        decphase(zero);
	delay(taub - 2.0*pwN - gt4 - 2.0*GRADIENT_DELAY);
        txphase(zero);
	decpwrf(rfC);
	delay(taub - 2.0*pw);
	rgpulse(2.0*pw, zero, 0.0, 0.0);
	delay(tau2);
	decrgpulse(2.0*pwC, zero, 0.0, 0.0);
	delay(taub);
	zgradpulse(gzlvl4, gt4);	        /* 2.0*GRADIENT_DELAY */	
	decpwrf(rfdC);
	delay(taub - gt4 - 2.0*GRADIENT_DELAY);
	decrgpulse(1.0e-3, zero, 0.0, 0.0);
	initval(ncyc, v2);
	starthardloop(v2);
     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);
     decrgpulse(5.0*p_d,one,0.0,0.0);
     decrgpulse(5.5*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.6*p_d,three,0.0,0.0);
     decrgpulse(7.2*p_d,one,0.0,0.0);
     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.4*p_d,one,0.0,0.0);
     decrgpulse(6.8*p_d,three,0.0,0.0);
     decrgpulse(7.0*p_d,one,0.0,0.0);
     decrgpulse(5.2*p_d,three,0.0,0.0);
     decrgpulse(5.4*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.5*p_d,one,0.0,0.0);
     decrgpulse(7.3*p_d,three,0.0,0.0);
     decrgpulse(5.1*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);

     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);
     decrgpulse(5.0*p_d,three,0.0,0.0);
     decrgpulse(5.5*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.6*p_d,one,0.0,0.0);
     decrgpulse(7.2*p_d,three,0.0,0.0);
     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.4*p_d,three,0.0,0.0);
     decrgpulse(6.8*p_d,one,0.0,0.0);
     decrgpulse(7.0*p_d,three,0.0,0.0);
     decrgpulse(5.2*p_d,one,0.0,0.0);
     decrgpulse(5.4*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.5*p_d,three,0.0,0.0);
     decrgpulse(7.3*p_d,one,0.0,0.0);
     decrgpulse(5.1*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);

     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);
     decrgpulse(5.0*p_d,three,0.0,0.0);
     decrgpulse(5.5*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.6*p_d,one,0.0,0.0);
     decrgpulse(7.2*p_d,three,0.0,0.0);
     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.4*p_d,three,0.0,0.0);
     decrgpulse(6.8*p_d,one,0.0,0.0);
     decrgpulse(7.0*p_d,three,0.0,0.0);
     decrgpulse(5.2*p_d,one,0.0,0.0);
     decrgpulse(5.4*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.5*p_d,three,0.0,0.0);
     decrgpulse(7.3*p_d,one,0.0,0.0);
     decrgpulse(5.1*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);

     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);
     decrgpulse(5.0*p_d,one,0.0,0.0);
     decrgpulse(5.5*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.6*p_d,three,0.0,0.0);
     decrgpulse(7.2*p_d,one,0.0,0.0);
     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.4*p_d,one,0.0,0.0);
     decrgpulse(6.8*p_d,three,0.0,0.0);
     decrgpulse(7.0*p_d,one,0.0,0.0);
     decrgpulse(5.2*p_d,three,0.0,0.0);
     decrgpulse(5.4*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.5*p_d,one,0.0,0.0);
     decrgpulse(7.3*p_d,three,0.0,0.0);
     decrgpulse(5.1*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);

	endhardloop();
        decrgpulse(9.0*p_d/5.0, t9, 2.0e-6, 0.0);
     if( H2Opurge[A] == 'y' )
         {obspwrf(1000.0); rgpulse(900*pw, zero, 0.0, 0.0);
	   rgpulse(500*pw, one, 0.0, 0.0); obspwrf(4095.0); }
	zgradpulse(gzlvl7, gt7);
	decpwrf(rfC);
	delay(50.0e-6);
        rgpulse(pw,zero,0.0,0.0);
	zgradpulse(gzlvl7, gt7/1.6);
        decrgpulse(pwC, three, 100.0e-6, 0.0); 
	zgradpulse(gzlvl5, gt5);
	decphase(zero);
	delay(tauc - gt5);
	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
	zgradpulse(gzlvl5, gt5);
	delay(tauc - gt5);
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl3, gt3);
        if(dm3[B] == 'y')	         /*optional 2H decoupling off */
        {
         dec3rgpulse(1/dmf3, three, 0.0, 0.0);
         dec3blank();
         setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
         dec3blank();
        }
	delay(grecov);
	rgpulse(pw, zero, 0.0, 0.0);
	zgradpulse(gzlvl6, gt5);
	delay(taua - gt5);
	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
	zgradpulse(gzlvl6, gt5);
  if (STUD[A]=='y') decpower(studlvl);

  else
   {
	decpower(dpwr);
	dec2power(dpwr2);
   }
	delay(taua - gt5);
	rgpulse(pw, zero, 0.0, rof2);
        rcvron();
        if (dm3[B] == 'y') lk_sample();
	setreceiver(t11);
  if ((STUD[A]=='y') && (dm[C] == 'y'))
       {
        decpower(studlvl);
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
}		 
