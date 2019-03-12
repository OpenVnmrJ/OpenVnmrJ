/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_ghcch_tocsy.c               

    3D HCCH tocsy utilising gradients and double sensitivity enhancement.
    Also known as DE-H(C)CH-TOCSY.

    Correlates the ribose 13C resonances of a given ribonucleotide.
    Uses isotropic 13C mixing.

    Standard features include maintaining the C13 carrier in the ribose region
    throughout.
    Optional 2H decoupling when ribose C13 magnetization is transverse and
    during 1H shift evolution for 4 channel spectrometers.
    Maximum sensitivity is obtained using full power square pulses on 
    the ribose region throughout.  DIPSI-3 rather than DIPSI-2 (suggested in the
    JMR article) is used as standard as in the other RnaPack sequences;
 
    pulse sequence: Sattler, Schwendinger, Schleucher and Griesinger, JBNMR,
			Vol 6  No.1, 11-22 (1995).

    Derived from gc_co_nh.c written by Robin Bendall, Varian, March 94 and 95.
    Revised and improved to a standard format by MRB, BKJ and GG for the 
    BioPack, January 1997.

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

    2D experiment in t1: wft2d(1,0,-1,0,0,1,0,1) (sensitivity-enhanced)
    2D experiment in t2: wft2d('ni2',1,0,-1,0,0,1,0,1) (sensitivity-enhanced)
    ( or with 5.2F or above just use wft2da or wft2da('ni2') after setting
      f1coef='1 0 -1 0 0 1 0 1'
      f2coef='1 0 -1 0 0 1 0 1'
     for 3D just use ft3da )
     2D transforms using 3D data set will not transform properly with the
     standard wft2da macro. The f1coef entry must be changed but proper values 
     are not known at this time.
    
    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in 13C and f2180='n' for (0,0) in 15N.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.




          	  DETAILED INSTRUCTIONS FOR USE OF rna_ghcch_tocsy

    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro: 
                                      "printon man('RnaPack') printoff".
       These Detailed Instructions for rna_ghcch_tocsy may be printed using:
                                      "printon man('rna_ghcch_tocsy') printoff".
             
    2. Apply the setup macro "rna_ghcch_tocsy".  This loads the relevant parameter
       set and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral
       check.

    3. The parameter ncyc corresponds to the number of cycles of DIPSI-3 mixing.
       Use ncyc = 2 - 4 usually.  The corresponding total mixing time should be
       20-30ms.  The DIPSI rf field of 35ppm (7 kHz for a 800Mhz 
       spectrometer), which is scaled against field strength, is sufficient to
       cover the ribose region (at any field strength) and is 
       more than adequate for the CC J's.  However, change the
       number 7000 in the pulse sequence code calculation of p_d if a different 
       rf strength is required.
      
    4. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 110ppm, and N15
       frequency on the aromatic N region (200 ppm).  The C13 frequency remains at
       80ppm, ie in the center of the ribose region throughout the sequence.
       Set sw=sw1=4ppm and sw2=55ppm.

    5. del (3.4 ms), del1 (1.9 ms), and del2 (1.6 ms), were determined for 
       an RNA 19mer and are listed in dg2 for possible readjustment by
       the user. The above reference (Fig.1) gives theoretical values of 0.5/J,
       0.31/J and 0.23/J for del, del1 and del2 and elsewhere in the reference,
       3.6, 2.2 and 1.9 ms can be inferred. 

    6. If 2H decoupling is used, the 2H lock signal may become unstable because
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
             phi6[1]  = {1},
             phi5[4]  = {0,0,2,2},
             phi10[1] = {1},
	     rec[4]   = {0,2,2,0};

static double   d2_init=0.0, d3_init=0.0;

void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
	    rna_stCdec[MAXSTR],	       /* calls STUD+ waveforms from shapelib */
	    STUD[MAXSTR];   /* apply automatically calculated STUD decoupling */
 
int         icosel1,          			  /* used to get n and p type */
	    icosel2,
            t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
	    del = getval("del"),     /* time delays for CH coupling evolution */
	    del1 = getval("del1"),
	    del2 = getval("del2"),
/* STUD+ waveforms automatically calculated by macro "rnacal" */
/* and string parameter rna_stCdec calls them from your shapelib.*/
   stdmf,                              		   /* dmf for STUD decoupling */
   studlvl,	                         /* coarse power for STUD+ decoupling */
   rf80 = getval("rf80"), 			  /* rf in Hz for 80ppm STUD+ */

	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	rfC,            	  /* maximum fine power when using pwC pulses */
	dofa,                             /* dof shifted to 80 ppm for ribose */

/* p_d is used to calculate the isotropic mixing on the Cab region */
        p_d,                   	        /* 50 degree pulse for DIPSI-3 at rfd */
        rfd,                   			     /* fine power for 35 ppm */
	ncyc = getval("ncyc"), 			  /* no. of cycles of DIPSI-3 */

   compC = getval("compC"),       /* adjustment for C13 amplifier compression */


	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

 grecov = getval("grecov"),   /* Gradient recovery delay, typically 150-200us */

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt3 = getval("gt3"),				   /* other gradients */
	gt5 = getval("gt5"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6");

    getstr("STUD",STUD);
    getstr("f1180",f1180);
    getstr("f2180",f2180);

/*   LOAD PHASE TABLE    */

	settable(t3,2,phi3);
	settable(t6,1,phi6);
	settable(t5,4,phi5);
	settable(t10,1,phi10);
	settable(t11,4,rec);

        

/*   INITIALIZE VARIABLES   */

    if( dpwrf < 4095 )
	{ text_error("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
	  psg_abort(1); }

/* maximum fine power for pwC pulses */
	rfC = 4095.0;

/*  Center dof in RIBOSE region on 80 ppm. */
        dofa = dof - 30.0*dfrq;
		
/* dipsi-3 decoupling C-ribose */
 	p_d = (5.0)/(9.0*4.0*7000.0*(sfrq/800.0)); /* DIPSI-3 covers 35 ppm */
 	rfd = (compC*4095.0*pwC*5.0)/(p_d*9.0);
	rfd = (int) (rfd + 0.5);
  	ncyc = (int) (ncyc + 0.5);

/* 80 ppm STUD+ decoupling */
        strcpy(rna_stCdec, "wurst80");
	stdmf = getval("dmf80");
        studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
        studlvl = (int) (studlvl + 0.5);


/* CHECK VALIDITY OF PARAMETER RANGES */

  if( gt1 > 0.5*del - 0.5*grecov )
  { text_error(" gt1 is too big. Make gt1 less than %f.\n", (0.5*del - 0.5*grecov)); psg_abort(1);}

  if((dm3[A] == 'y' || dm3[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nyn' or 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' or 'nny' "); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( (((dm[C] == 'y') && (dm2[C] == 'y')) && (STUD[A] == 'y')) )
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' if STUD='y'"); psg_abort(1); }

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

    icosel1 = -1;  icosel2 = -1;
    if (phase1 == 2) 
	{ tsadd(t6,2,4); icosel1 = -1*icosel1; }
    if (phase2 == 2) 
	{ tsadd(t10,2,4); icosel2 = -1*icosel2; tsadd(t6,2,4); }


/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;


/*  Set up f2180  */

    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 1.0)) 
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
 if (dm3[B]=='y') lk_sample();
   	delay(d1);
 if (dm3[B]=='y') lk_hold();

	rcvroff();
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	decpwrf(rfC);
	obsoffset(tof);
        decoffset(dofa);
        dec2offset(dof2);
	txphase(t3);
	delay(1.0e-5);

	decrgpulse(pwC, zero, 0.0, 0.0);	   /*destroy C13 magnetization*/
	zgradpulse(gzlvl1, 0.5e-3);
	delay(grecov/2);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl1, 0.5e-3);
	delay(5.0e-4);

   if(dm3[B] == 'y')				  /*optional 2H decoupling on */
        { 
          dec3unblank();
          dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank();
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
         } 
	rgpulse(pw, t3, 0.0, 0.0);                    /* 1H pulse excitation */

        decphase(zero);
	delay(0.5*del + tau1 - 2.0*pwC);

        decrgpulse(2.0*pwC, zero, 0.0, 0.0);

        txphase(zero);
	delay(tau1);

	rgpulse(2.0*pw, zero, 0.0, 0.0);

	zgradpulse(icosel1*gzlvl1, 0.1*gt1);
        decphase(t5);
	delay(0.5*del - 0.1*gt1);

	simpulse(pw, pwC, zero, t5, 0.0, 0.0);

	zgradpulse(gzlvl3, gt3);
        decphase(zero);
	delay(0.5*del2 - gt3);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl3, gt3);
        txphase(t6);
        decphase(one);
	delay(0.5*del2 - gt3);

	simpulse(pw, pwC, t6, one, 0.0, 0.0);

	zgradpulse(gzlvl4, gt3);
        txphase(zero);
        decphase(zero);
	delay(0.5*del1 - gt3);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl4, gt3);
	delay(0.5*del1 - gt3);

	decrgpulse(pwC, zero, 0.0, 0.0);
	decpwrf(rfd);
	delay(2.0e-6);
	initval(ncyc, v2);
	starthardloop(v2);
     decrgpulse(4.9*p_d,zero,0.0,0.0);
     decrgpulse(7.9*p_d,two,0.0,0.0);
     decrgpulse(5.0*p_d,zero,0.0,0.0);
     decrgpulse(5.5*p_d,two,0.0,0.0);
     decrgpulse(0.6*p_d,zero,0.0,0.0);
     decrgpulse(4.6*p_d,two,0.0,0.0);
     decrgpulse(7.2*p_d,zero,0.0,0.0);
     decrgpulse(4.9*p_d,two,0.0,0.0);
     decrgpulse(7.4*p_d,zero,0.0,0.0);
     decrgpulse(6.8*p_d,two,0.0,0.0);
     decrgpulse(7.0*p_d,zero,0.0,0.0);
     decrgpulse(5.2*p_d,two,0.0,0.0);
     decrgpulse(5.4*p_d,zero,0.0,0.0);
     decrgpulse(0.6*p_d,two,0.0,0.0);
     decrgpulse(4.5*p_d,zero,0.0,0.0);
     decrgpulse(7.3*p_d,two,0.0,0.0);
     decrgpulse(5.1*p_d,zero,0.0,0.0);
     decrgpulse(7.9*p_d,two,0.0,0.0);

     decrgpulse(4.9*p_d,two,0.0,0.0);
     decrgpulse(7.9*p_d,zero,0.0,0.0);
     decrgpulse(5.0*p_d,two,0.0,0.0);
     decrgpulse(5.5*p_d,zero,0.0,0.0);
     decrgpulse(0.6*p_d,two,0.0,0.0);
     decrgpulse(4.6*p_d,zero,0.0,0.0);
     decrgpulse(7.2*p_d,two,0.0,0.0);
     decrgpulse(4.9*p_d,zero,0.0,0.0);
     decrgpulse(7.4*p_d,two,0.0,0.0);
     decrgpulse(6.8*p_d,zero,0.0,0.0);
     decrgpulse(7.0*p_d,two,0.0,0.0);
     decrgpulse(5.2*p_d,zero,0.0,0.0);
     decrgpulse(5.4*p_d,two,0.0,0.0);
     decrgpulse(0.6*p_d,zero,0.0,0.0);
     decrgpulse(4.5*p_d,two,0.0,0.0);
     decrgpulse(7.3*p_d,zero,0.0,0.0);
     decrgpulse(5.1*p_d,two,0.0,0.0);
     decrgpulse(7.9*p_d,zero,0.0,0.0);

     decrgpulse(4.9*p_d,two,0.0,0.0);
     decrgpulse(7.9*p_d,zero,0.0,0.0);
     decrgpulse(5.0*p_d,two,0.0,0.0);
     decrgpulse(5.5*p_d,zero,0.0,0.0);
     decrgpulse(0.6*p_d,two,0.0,0.0);
     decrgpulse(4.6*p_d,zero,0.0,0.0);
     decrgpulse(7.2*p_d,two,0.0,0.0);
     decrgpulse(4.9*p_d,zero,0.0,0.0);
     decrgpulse(7.4*p_d,two,0.0,0.0);
     decrgpulse(6.8*p_d,zero,0.0,0.0);
     decrgpulse(7.0*p_d,two,0.0,0.0);
     decrgpulse(5.2*p_d,zero,0.0,0.0);
     decrgpulse(5.4*p_d,two,0.0,0.0);
     decrgpulse(0.6*p_d,zero,0.0,0.0);
     decrgpulse(4.5*p_d,two,0.0,0.0);
     decrgpulse(7.3*p_d,zero,0.0,0.0);
     decrgpulse(5.1*p_d,two,0.0,0.0);
     decrgpulse(7.9*p_d,zero,0.0,0.0);

     decrgpulse(4.9*p_d,zero,0.0,0.0);
     decrgpulse(7.9*p_d,two,0.0,0.0);
     decrgpulse(5.0*p_d,zero,0.0,0.0);
     decrgpulse(5.5*p_d,two,0.0,0.0);
     decrgpulse(0.6*p_d,zero,0.0,0.0);
     decrgpulse(4.6*p_d,two,0.0,0.0);
     decrgpulse(7.2*p_d,zero,0.0,0.0);
     decrgpulse(4.9*p_d,two,0.0,0.0);
     decrgpulse(7.4*p_d,zero,0.0,0.0);
     decrgpulse(6.8*p_d,two,0.0,0.0);
     decrgpulse(7.0*p_d,zero,0.0,0.0);
     decrgpulse(5.2*p_d,two,0.0,0.0);
     decrgpulse(5.4*p_d,zero,0.0,0.0);
     decrgpulse(0.6*p_d,two,0.0,0.0);
     decrgpulse(4.5*p_d,zero,0.0,0.0);
     decrgpulse(7.3*p_d,two,0.0,0.0);
     decrgpulse(5.1*p_d,zero,0.0,0.0);
     decrgpulse(7.9*p_d,two,0.0,0.0);
	endhardloop();

        dec2phase(zero);
        decphase(zero);
        txphase(zero);
	decpwrf(rfC);
	delay(tau2);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	delay(tau2);
	decpwrf(rfC);
	zgradpulse(-icosel2*gzlvl2, 1.8*gt1);
	delay(grecov+2.0e-6);

	decrgpulse(2.0*pwC, zero, 0.0, 0.0);

	decpwrf(rfC);
	zgradpulse(icosel2*gzlvl2, 1.8*gt1);
	delay(grecov + pwN);

	decrgpulse(pwC, zero, 0.0, 0.0);
	decpwrf(rfC);
	decrgpulse(pwC, zero, 2.0e-6, 0.0);

	zgradpulse(gzlvl5, gt5);
	delay(0.5*del1 - gt5);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	decphase(t10);
	delay(0.5*del1 - gt5);

	simpulse(pw, pwC, one, t10, 0.0, 0.0);

	zgradpulse(gzlvl6, gt5);
	txphase(zero);
	decphase(zero);
	delay(0.5*del2 - gt5);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl6, gt5);
	delay(0.5*del2 - gt5);

	simpulse(pw, pwC, zero, zero, 0.0, 0.0);

	delay(0.5*del - 0.5*pwC);

	simpulse(2.0*pw,2.0*pwC, zero, zero, 0.0, 0.0);
   if (STUD[A]=='y') decpower(studlvl);

   else
    {
	decpower(dpwr);
	dec2power(dpwr2);
    }
	zgradpulse(gzlvl1, gt1);         		/* 2.0*GRADIENT_DELAY */
   if(dm3[B] == 'y') 
	delay(0.5*del - gt1 -1/dmf3 - 2.0*GRADIENT_DELAY - POWER_DELAY);
      else
	delay(0.5*del - gt1 - 2.0*GRADIENT_DELAY - POWER_DELAY);
   if(dm3[B] == 'y')			         /*optional 2H decoupling off */
        {
          dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
          setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
          dec3blank();
        }
	decpower(dpwr);				               /* POWER_DELAY */
  if (dm3[B]=='y') lk_sample();
  if ((STUD[A]=='y') && (dm[C] == 'y'))
        {decpower(studlvl);
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
