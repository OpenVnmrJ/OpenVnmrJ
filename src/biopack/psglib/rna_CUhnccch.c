/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_CUhnccch.c


    This pulse sequence will allow one to perform the following experiment:

    Base-correlation from Uridine-imino to H6 or Cytidine-amino to H6 with
    options for the initial H->N transfer.

    H   -->   N   -->   C4   -->   C5   -->   C6   -->   H6 - det
    t1  CP        CP        INEPT      INEPT     re-INEPT
	or
	INEPT


                      NOTE dof MUST BE SET AT 110ppm ALWAYS
                      NOTE dof2 MUST BE SET AT 200ppm ALWAYS

    Experiment may not be feasible at >600 MHz because of power requirements.
    PSG will abort as a safety measure. 


    pulse sequence:	Simorre, J.P. et al, J. Biom. NMR, 6 (1995), 427-32.


                  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [H1].

    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in H1.  f1180='y' is ignored if ni=0.



                  DETAILED INSTRUCTIONS FOR USE OF rna_CUhnccch


    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro:
                                      "printon man('RnaPack') printoff".
       These Detailed Instructions for rna_gChsqc may be printed using:
                                      "printon man('rna_CUhnccch') printoff".

    2. Apply the setup macro "rna_Chnccch" for the Cytidine and "rna_Uhnccch"
       for the Uridine correlation experiment.  This loads the relevant
       parameter set and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), N15 frequency on the aromatic N
       region (200 ppm), and C13 frequency on 110ppm.

    4. CHOICE of transfer and CYT/URA options:

       CP='y' uses CP for imino/amino H1 -> N15 transfer.
       ncyc_hn should be set to 1 or 2(at 500 MHz),~7.5msec, according to J(H-N) = 95Hz.

       INEPT='y' uses INEPT for imino/amino H1 -> N15 transfer.

       URA='y' gives a spectrum of uridine correlations.
       ncyc_nc should be set to 6(at 500 MHz),~90msec, according to J(N1-C4) = 10Hz.
       INEPT is generally better for uridine.		Set sw1 to 4ppm.

       CYT='y' gives a spectrum of cytidine correlations.
       ncyc_nc should be set to 3(at 500 MHz),~45msec, according to J(N4-C4) = 20Hz.
       CP is generally better for uridine.             Set sw1 to 4ppm.

    5. C13 COUPLING:
       Splitting of resonances in the t1 dimension by C13 coupling
       is removed by setting C13refoc='y'.

    6. GRADIENTS:
       G3 and G4 should be optimized for sufficient water suppression.
       Typical values are G3=12G/cm and G4:-28G/cm.

       G5 and G0 should be 2-3G/cm. 

       Gr suppresses radiation damping during t1 and should be 0.5 G/cm.

       In general, gradient-levels should be as low as possible.

    7. pw11-flag:

       Setting of pw11='y' replaces the first proton pulse by 
       an off resonance 1-1 type proton excitation pulse. This minimizes the
       excitation of H2O and allows to use lower gradient levels.
       Overall, setting pw11='y' leads to a slight loss in S/N.       


        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        @                                                      @
        @   Rewritten for RnaPack by Peter Lukavsky (10/98).   @
        @                                                      @
        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/

#include <standard.h>

static int  phi1[2] = {1,3},
	    phi3[8] = {1,1,1,1,3,3,3,3},
	    phi4[4] = {0,0,2,2},
	    phi5[16]= {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
	    rec1[8]  = {0,2,0,2,2,0,2,0},
	    rec2[16] = {0,2,0,2,2,0,2,0,2,0,2,0,0,2,0,2};

static double   d2_init=0.0;


void pulsesequence()
{

/* DECLARE VARIABLES */

char	pw11[MAXSTR],	    /* off resonance 1-1 type proton excitation pulse */
	URA[MAXSTR],				  /* Setup for U-imino - U-H6 */
	CYT[MAXSTR],				  /* Setup for C-imino - C-H6 */
	CP[MAXSTR],					  /* CP H->N transfer */
	INEPT[MAXSTR],				       /* INEPT H->N transfer */
	C13refoc[MAXSTR],                         /* C13 pulse in middle of t1*/
	f1180[MAXSTR];                        /* Flag to start t1 @ halfdwell */

int	t1_counter;

double      tau1,                                                /*  t1 delay */
	    lambda = 0.94/(4.0*getval("JCH")),        /* 1/4J C-H INEPT delay */
	    lambdaN = 0.94/(4.0*getval("JNH")),       /* 1/4J N-H INEPT delay */
            tCC = 1.0/(4.0*getval("JCC")),            /* 1/4J C-C INEPT delay */

        pwClvl = getval("pwClvl"),              /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
        rfC,                      /* maximum fine power when using pwC pulses */
	compC = getval("compC"),  /* adjustment for C13 amplifier compression */

        pwNlvl = getval("pwNlvl"),                    /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
        rfN,                      /* maximum fine power when using pwN pulses */
        compN = getval("compN"),  /* adjustment for N15 amplifier compression */

        tpwr = getval("tpwr"),    	               /* power for H1 pulses */
        pw = getval("pw"),               /* H1 90 degree pulse length at tpwr */
        rfH,                       /* maximum fine power when using pw pulses */
	compH = getval("compH"),   /* adjustment for H1 amplifier compression */


        tof_75,                  /* tof shifted to 7.5 ppm for H4-N4 transfer */
        tof_12,                   /* tof shifted to 12 ppm for H3-N3 transfer */

	dof_169,		 /* dof shifted to 169 ppm for N3-C4 transfer */
	dof_140,     /* dof shifted to 140 ppm for C4-C5-C6 transfer and DEC1 */

	dof2_97,       /* dof2 shifted to 97 ppm for H4-N4 and N4-C4 transfer */
        dof2_160,     /* dof2 shifted to 160 ppm for H3-N3 and N3-C4 transfer */

/* p_d is used to calculate the isotropic mixing */
        p_d,                 /* 50 degree pulse for DIPSI-3 at rfdC-rfdN-rfdH */
        rfdC,             /* fine C13 power for 1.9 kHz rf for 500MHz magnet  */
        rfdN,             /* fine N15 power for 1.9 kHz rf for 500MHz magnet  */
        rfdH,              /* fine H1 power for 1.9 kHz rf for 500MHz magnet  */
        ncyc_hn = getval("ncyc_hn"),  /* number of pulsed cycles in HN half-DIPSI-3 */
        ncyc_nc = getval("ncyc_nc"), /* number of pulsed cycles in NC DIPSI-3 */

	sw1 = getval("sw1"),
        grecov = getval("grecov"),

        pwHs = getval("pwHs"),         /* H1 90 degree pulse length at tpwrs */
        tpwrs,                   /* power for the pwHs ("rna_H2Osinc") pulse */

        pwHs2 = getval("pwHs2"),       /* H1 90 degree pulse length at tpwrs2 */
        tpwrs2,                           /* power for the pwHs2 square pulse */


  gzlvl0 = getval("gzlvl0"),
  gzlvl3 = getval("gzlvl3"),
  gt3 = getval("gt3"),
  gzlvl4 = getval("gzlvl4"),
  gt4 = getval("gt4"),
  gzlvl5 = getval("gzlvl5"),
  gt5 = getval("gt5"),
  gzlvlr = getval("gzlvlr");

  getstr("pw11",pw11);
  getstr("URA",URA);
  getstr("CYT",CYT);
  getstr("CP",CP);
  getstr("INEPT",INEPT);
  getstr("C13refoc",C13refoc);
  getstr("f1180",f1180);


/* LOAD PHASE TABLE */
 
	settable(t1,2,phi1);
	settable(t3,8,phi3);
	settable(t4,4,phi4);
	settable(t5,16,phi5);
  if ( CP[A] == 'y' )
	settable(t10,8,rec1);
  if ( INEPT[A] == 'y' )
	settable(t10,16,rec2);


/* INITIALIZE VARIABLES */

/* maximum fine power for pwC pulses */
        rfC = 4095.0;

/* maximum fine power for pwN pulses */
        rfN = 4095.0;

/* maximum fine power for pw pulses */
        rfH = 4095.0;

/* different offset values tof=H2O, dof=110ppm, dof2=200ppm */

	tof_75 = tof + 2.5*sfrq;        /* tof shifted to nH2 */
	tof_12 = tof + 7.0*sfrq;	/* tof shifted to nH */
	dof_169 = dof + 59*dfrq;	/* dof shifted to C4 */
	dof_140 = dof + 30*dfrq;	/* dof shifted to C6 */
	dof2_160 = dof2 - 40*dfrq2;	/* dof2 shifted to Nh */
	dof2_97 = dof2 - 103*dfrq2;     /* dof2 shifted to Nh2 */

/* 1.9 kHz DIPSI-3 at 500MHz*/
        p_d = (5.0)/(9.0*4.0*1900.0*(sfrq/500.0)); /* 1.9 kHz DIPSI-3 at 500MHz*/

/* fine C13 power for dipsi-3 isotropic mixing on C4 region */
        rfdC = (compC*4095.0*pwC*5.0)/(p_d*9.0);
        rfdC = (int) (rfdC + 0.5);

/* fine N15 power for dipsi-3 isotropic mixing on Nh region */
        rfdN = (compN*4095.0*pwN*5.0)/(p_d*9.0);
        rfdN = (int) (rfdN + 0.5);

/* fine H1 power for half dipsi-3 isotropic mixing on nH2 region */
        rfdH = (compH*4095.0*pw*5.0)/(p_d*9.0);
        rfdH = (int) (rfdH + 0.5);

/* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /* needs 1.69 times more */
        tpwrs = (int) (tpwrs);                   /* power than a square pulse */

/* selective H20 square pulse */
        tpwrs2 = tpwr - 20.0*log10(pwHs2/(compH*pw));
        tpwrs2 = (int) (tpwrs2);

/* number of cycles and mixing time */
        ncyc_nc = (int) (ncyc_nc + 0.5);
	ncyc_hn = (int) (ncyc_hn + 0.5);

  if (ncyc_nc > 0 )
   {
        printf("NC-mixing time is %f ms.\n",(ncyc_nc*51.8*4*p_d));
   }

  if (CP[A] == 'y')
   {
    if (ncyc_hn > 0 )
        printf("HN-mixing time is %f ms.\n",(ncyc_hn*51.8*2*p_d));
   }

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)
         tsadd(t1,1,4);

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t1,2,4); tsadd(t10,2,4); }

/*  Set up f1180  */

    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0))
        { tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;


/* CHECK VALIDITY OF PARAMETER RANGE */


    if( sfrq > 610 )
        { printf("Power Levels at 750/800 MHz may be too high for probe");
          psg_abort(1); }

    if( dpwrf < 4095 )
        { printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
          psg_abort(1); }

    if( dpwrf2 < 4095 )
        { printf("reset dpwrf2=4095 and recalibrate N15 90 degree pulse");
          psg_abort(1); }

    if((dm[A] == 'y' || dm[B] == 'y'))
    {
        printf("incorrect dec1 decoupler flag! Should be 'nny' or 'nnn' ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y'))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnn' or 'nny' ");
        psg_abort(1);
    }

    if( ((dm[C] == 'y') && (dm2[C] == 'y') && (at > 0.18)) )
    {
        text_error("check at time! Don't fry probe !! ");
        psg_abort(1);
    }

    if( dpwr > 50 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

   if( dpwr2 > 50 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( pw > 20.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    }

    if( pwC > 40.0e-6 )
    {
        printf("dont fry the probe, pwC too high ! ");
        psg_abort(1);
    }

    if( pwN > 100.0e-6 )
    {
        printf("dont fry the probe, pwN too high ! ");
        psg_abort(1);
    }

    if (gzlvlr > 500 || gzlvlr < -500)
    {
	text_error(" RDt1-gzlvlr must be -500 to 500 (0.5G/cm) \n");
	psg_abort(1);
    }

    if( (CP[A] == 'y') && (INEPT[A] == 'y') )
    {
        text_error("Choose either CP or INEPT for H->N transfer !! ");
        psg_abort(1);
    }

    if( ncyc_hn > 2 )
    {
        text_error("check H->N half-dipsi-3 time !! ");
        psg_abort(1);
    }

    if( ncyc_nc > 7 )
    {
        text_error("check N->C dipsi-3 time !! ");
        psg_abort(1);
    }

    if( (URA[A] == 'y') && (CYT[A] == 'y') )
    {
        text_error("Choose either URA or CYT !! ");
        psg_abort(1);
    }

    if( (URA[A] == 'n') && (CYT[A] == 'n') )
    {
        text_error("Do you really want to run this experiment ?? ");
        psg_abort(1);
    }

    if( (URA[A] == 'y') && (CP[A] == 'y') )
    {
        printf("Remember that CP covers just 3.8 ppm !!! ");
    }


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);

        rcvroff();

        obspower(tpwr);
	obspwrf(rfH);
	obsstepsize(0.5);
        decpower(pwClvl);
        decpwrf(rfC);
	decstepsize(0.5);
        dec2power(pwNlvl);
	dec2pwrf(rfN);
	dec2stepsize(0.5);

  if (URA[A] == 'y')
   {
        obsoffset(tof_12);	/* Set the proton frequency to U-nH */
        dec2offset(dof2_160);   /* Set the nitrogen frequency to U-Nh */
   }
  else if (CYT[A] == 'y')
   {
        obsoffset(tof_75);      /* Set the proton frequency to C-nH2 */
	dec2offset(dof2_97);    /* Set the nitrogen frequency to C-Nh2 */
   }
  else
   {
   }

        decoffset(dof_169);	/* Preset the carbon frequency for the NC-tocsy */

        txphase(zero);
        decphase(zero);
        dec2phase(zero);

        delay(d1);

        dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
        decrgpulse(pwC, zero, 0.0, 0.0);
        zgradpulse(gzlvl0, 0.5e-3);
        delay(1.0e-4);
        dec2rgpulse(pwN, one, 0.0, 0.0);
        decrgpulse(pwC, one, 0.0, 0.0);
        zgradpulse(0.7*gzlvl0, 0.5e-3);
        delay(5.0e-4);

  if (CP[A] == 'y')
	initval(ncyc_hn,v12);

	initval(ncyc_nc,v11);

        txphase(t1);
        decphase(zero);
        dec2phase(zero);
        delay(5.0e-4);
        rcvroff();

  if(pw11[A] == 'y')
   {
        rgpulse(pw/2, t1, 50.0e-6, 0.0);
	if (URA[A] == 'y')
		delay(1/(2*(tof_12-tof)));
	else if (CYT[A] == 'y')
        	delay(1/(2*(tof_75-tof)));
	else
		delay(1/(2*(tof_75-tof)));
        rgpulse(pw/2, t1, 0.0, 0.0);
   }

  else
   {
	rgpulse(pw, t1, 50.0e-6, 0.0);
   }
	txphase(zero);

  if (C13refoc[A]=='y')
   {

        if (tau1 > (0.001-(2.0*GRADIENT_DELAY + pwN + 0.64*pw )))
        {
        zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw) );
        sim3pulse(0.0, 2.0*pwC, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        zgradpulse(-1.0*gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        }
        else
        {
         if (tau1 > (0.001-(pwN + 0.64*pw )))
         {
          delay(tau1 - pwN - 0.64*pw );
          sim3pulse(0.0, 2.0*pwC, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
          delay(tau1 - pwN - 0.64*pw);
         }
         else
         {
          if (tau1 > (0.64*pw ))
           delay(2.0*tau1 - 2.0*0.64*pw );
         }
        }
   }
  else
   {
        if (tau1 > (0.001-(2.0*GRADIENT_DELAY + pwN + 0.64*pw )))
        {
        zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw) );
        dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
        zgradpulse(-1.0*gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        }
        else
        {
         if (tau1 > (0.001-(pwN + 0.64*pw )))
         {
          delay(tau1 - pwN - 0.64*pw );
          dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
          delay(tau1 - pwN - 0.64*pw);
         }
         else
         {
          if (tau1 > (0.64*pw ))
           delay(2.0*tau1 - 2.0*0.64*pw );
         }
        }
   }

  if (INEPT[A] == 'y')
   {
	delay(lambdaN);

	sim3pulse(2*pw, 0.0, 2*pwN, zero, zero, zero, 0.0, 0.0);
	dec2phase(t5);

	delay(lambdaN - SAPS_DELAY);

	sim3pulse(pw, 0.0, pwN, zero, zero, t5, 0.0, 0.0);
	dec2phase(zero);

	zgradpulse(gzlvl5,gt5);
	delay(lambdaN - SAPS_DELAY - gt5);

	sim3pulse(2*pw, 0.0, 2*pwN, one, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5,gt5);
        delay(lambdaN - 2*SAPS_DELAY - gt5 - 2*POWER_DELAY);

        decpwrf(rfdC);          /* Set fine power for carbon */
        dec2pwrf(rfdN);         /* Set fine power for nitrogen */
   }

  else if (CP[A] == 'y')
   {
        obspwrf(rfdH);          /* Set fine power for proton */
        decpwrf(rfdC);          /* Preset fine power for carbon */
        dec2pwrf(rfdN);         /* Set fine power for nitrogen */
        delay(2.0e-6);
        starthardloop(v12);
    sim3pulse(6.4*p_d,0.0,6.4*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(8.2*p_d,0.0,8.2*p_d,two,two,two,0.0,0.0);
    sim3pulse(5.8*p_d,0.0,5.8*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(5.7*p_d,0.0,5.7*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.6*p_d,0.0,0.6*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(4.9*p_d,0.0,4.9*p_d,two,two,two,0.0,0.0);
    sim3pulse(7.5*p_d,0.0,7.5*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(5.3*p_d,0.0,5.3*p_d,two,two,two,0.0,0.0);
    sim3pulse(7.4*p_d,0.0,7.4*p_d,zero,zero,zero,0.0,0.0);

    sim3pulse(6.4*p_d,0.0,6.4*p_d,two,two,two,0.0,0.0);
    sim3pulse(8.2*p_d,0.0,8.2*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(5.8*p_d,0.0,5.8*p_d,two,two,two,0.0,0.0);
    sim3pulse(5.7*p_d,0.0,5.7*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.6*p_d,0.0,0.6*p_d,two,two,two,0.0,0.0);
    sim3pulse(4.9*p_d,0.0,4.9*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(7.5*p_d,0.0,7.5*p_d,two,two,two,0.0,0.0);
    sim3pulse(5.3*p_d,0.0,5.3*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(7.4*p_d,0.0,7.4*p_d,two,two,two,0.0,0.0);
        endhardloop();
   }
  
  else
   {
   }

	dec2phase(zero);
        decphase(zero);

	starthardloop(v11);
    sim3pulse(0.0,6.4*p_d,6.4*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,8.2*p_d,8.2*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,5.8*p_d,5.8*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,5.7*p_d,5.7*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,0.6*p_d,0.6*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,4.9*p_d,4.9*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,7.5*p_d,7.5*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,5.3*p_d,5.3*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,7.4*p_d,7.4*p_d,zero,zero,zero,0.0,0.0);

    sim3pulse(0.0,6.4*p_d,6.4*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,8.2*p_d,8.2*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,5.8*p_d,5.8*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,5.7*p_d,5.7*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,0.6*p_d,0.6*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,4.9*p_d,4.9*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,7.5*p_d,7.5*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,5.3*p_d,5.3*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,7.4*p_d,7.4*p_d,two,two,two,0.0,0.0);

    sim3pulse(0.0,6.4*p_d,6.4*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,8.2*p_d,8.2*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,5.8*p_d,5.8*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,5.7*p_d,5.7*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,0.6*p_d,0.6*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,4.9*p_d,4.9*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,7.5*p_d,7.5*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,5.3*p_d,5.3*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,7.4*p_d,7.4*p_d,two,two,two,0.0,0.0);

    sim3pulse(0.0,6.4*p_d,6.4*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,8.2*p_d,8.2*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,5.8*p_d,5.8*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,5.7*p_d,5.7*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,0.6*p_d,0.6*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,4.9*p_d,4.9*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,7.5*p_d,7.5*p_d,zero,zero,zero,0.0,0.0);
    sim3pulse(0.0,5.3*p_d,5.3*p_d,two,two,two,0.0,0.0);
    sim3pulse(0.0,7.4*p_d,7.4*p_d,zero,zero,zero,0.0,0.0);
	endhardloop();

	obspwrf(rfH);
	decpwrf(rfC);
	dec2pwrf(rfN);

	obsoffset(tof);
	decoffset(dof_140);

	txphase(zero);
	decphase(one);

	decrgpulse(pwC,one,0.0,0.0);   /* flip transferred 13C-magn. to z */
	decphase(t3);

	decrgpulse(pwC,t3,0.0,0.0);  /* flip transferred 13C-magnetization to x */
	decphase(zero);
	zgradpulse(gzlvl5,gt5);

	delay(tCC - SAPS_DELAY - gt5);
        decrgpulse(2*pwC,zero,0.0,0.0);
        zgradpulse(gzlvl5,gt5);
        delay(tCC - gt5);

	decrgpulse(pwC,zero,0.0,0.0);  /* flip transferred 13C-magnetization to x */
	decphase(zero);
	zgradpulse(gzlvl5,gt5);

	delay(tCC - SAPS_DELAY - gt5);
	decrgpulse(2*pwC,zero,0.0,0.0);  
	zgradpulse(gzlvl5,gt5);
	delay(tCC - gt5);

	decrgpulse(pwC,zero,0.0,0.0);  /* flip transferred 13C-magnetization to x */
	decphase(zero);
	delay(tCC - SAPS_DELAY);
	decrgpulse(2*pwC,zero,0.0,0.0);
	delay(tCC - lambda - pw);
	rgpulse(2*pw,zero,0.0,0.0);  /* Invert water signal */
	delay(lambda - pw);
	decrgpulse(pwC,zero,0.0,0.0);

	zgradpulse(gzlvl3,gt3);
	delay(grecov);

        txphase(zero);
        obspower(tpwrs);
        shaped_pulse("rna_H2Osinc", pwHs, zero, 5.0e-4, 0.0);
        obspower(tpwr);

	rgpulse(pw, zero, 2*rof1, 0.0);
	txphase(two);
	obspower(tpwrs2);
	
	zgradpulse(gzlvl4,gt4);
        delay(grecov - 2*SAPS_DELAY - 2*POWER_DELAY - GRADIENT_DELAY);

        rgpulse((lambda-grecov-gt4-pwC), two, 0.0, 0.0);
        simpulse(pwC,pwC,two,three,0.0,0.0);
        simpulse(2*pwC,2*pwC,two,zero,0.0,0.0);
        simpulse(pwC,pwC,two,three,0.0,0.0);
        rgpulse((pwHs2-2*pwC-(lambda-grecov-gt4-pwC)), two, 0.0, 0.0);

        txphase(zero);
        obspower(tpwr);
        rgpulse(2*pw, zero, 0.0, 0.0);
        txphase(two);
        obspower(tpwrs2);

        rgpulse(pwHs2, two, 0.0, 0.0);
        decphase(t4);

        zgradpulse(gzlvl4,gt4);
        delay(grecov-2*pwC-2*SAPS_DELAY - POWER_DELAY - GRADIENT_DELAY);

        decrgpulse(pwC,t4,0.0,0.0);
        decrgpulse(pwC,zero,0.0,0.0);
        dec2power(dpwr2);               /* 2*POWER_DELAY */
        decpower(dpwr);

status(C);
	rcvron();

 setreceiver(t10);
}
