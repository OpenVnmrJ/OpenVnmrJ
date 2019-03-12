/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_CPMGnoesy.c

    This pulse sequence will allow one to perform the following experiment:

    A 2D N15-correlated NOESY employing a CPMG pulse train instead of regular
    INEPT transfer for improved magnetization transfer of exchange broadened
    resonances. Optional C13 refocusing.


                      NOTE dof MUST BE SET AT 110ppm ALWAYS
                      NOTE dof2 MUST BE SET AT 200ppm ALWAYS


    pulse sequence: Mulder, F.A.A. et al, J. Biom. NMR, 8 (1996) 223-28.



                  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for C13 decoupling.
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for N15 decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].

    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.



                  DETAILED INSTRUCTIONS FOR USE OF rna_CPMGnoesy


    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro:
                                      "printon man('RnaPack') printoff".
       These Detailed Instructions for rna_CPMGnoesy may be printed using:
                                      "printon man('rna_CPMGnoesy') printoff".

    2. Apply the setup macro "rna_CPMGnoesy".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 110ppm, and N15
       frequency on the aromatic N region (200 ppm).

    4. CPMG pulse train:
       ncyc should be set to multiple of 8 (8, 16 or 24).

    5. SETTING OF SW1:
       dof2 is set to 85ppm during the N15 labeling (tof set to 7.5ppm) and
       shifted to 120ppm for efficient decoupling during t2.
       dof is set to 110ppm for efficient C13 decoupling.

       Check the folding of the imino resonances in a N15 HSQC. Since the
       NOEs are N15-correlated it is important not to fold imino N15
       resonances onto amino N15 resonances.
       Since dof2 is fixed in all N15 HSQC sequences (85ppm) change sw1 for
       proper folding. sw1=60ppm is usually a good starting value.

    6. Splitting of resonances in the N15 dimension by C13 coupling in C13
       enriched samples can be removed by setting C13refoc='y'.

        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        @                                                      @
        @   Written for RnaPack by Peter Lukavsky (10/98).     @
        @                                                      @
        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/

#include <standard.h>


static int  phi1[2]  = {0,2},
	    phi11[8] = {0,0,2,2,1,1,3,3},
            phi12[8] = {2,2,0,0,3,3,1,1},
	    rec[8]   = {0,2,0,2,2,0,2,0};

static double   d2_init=0.0;


void pulsesequence()
{

/* DECLARE VARIABLES */

char	pw11[MAXSTR],
	C13refoc[MAXSTR],                         /* C13 pulse in middle of t1*/
	flip[MAXSTR],		 /* additional H2Osinc after first CPMG-train */
	f1180[MAXSTR];                        /* Flag to start t1 @ halfdwell */

int	t1_counter;

double      tau1,                                                /*  t1 delay */
	    lambdaN = 0.94/(4.0*getval("JNH")),       /* 1/4J N-H INEPT delay */
	    lambdaN_int,			    /* interval in CPMG train */

        pwClvl = getval("pwClvl"),              /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */

        pwNlvl = getval("pwNlvl"),                    /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

        tpwr = getval("tpwr"),    	               /* power for H1 pulses */
        pw = getval("pw"),               /* H1 90 degree pulse length at tpwr */
	compH = getval("compH"),

        tof_75,                   /* tof shifted to 7.5 ppm for CPMG transfer */

	dof2_85,       		  /* dof2 shifted to 85 ppm for CPMG transfer */
        dof2_120, 	    /* dof2 shifted to 120 ppm for mix and decoupling */

	ncyc = getval("ncyc"),	   	    /* number of cycles in CPMG train */

	sw1 = getval("sw1"),
        grecov = getval("grecov"),
	mix = getval("mix"),

        pwHs = getval("pwHs"),          /* H1 90 degree pulse length at tpwrs */
        tpwrs,                    /* power for the pwHs ("rna_H2Osinc") pulse */

        pwHs1 = getval("pwHs1"),       /* H1 90 degree pulse length at tpwrs1 */
        tpwrs1,                  /* power for the pwHs ("rna_H2Osinc1") pulse */

        pwHs2 = getval("pwHs2"),       /* H1 90 degree pulse length at tpwrs2 */
        tpwrs2,                           /* power for the pwHs2 square pulse */

  gzlvl0 = getval("gzlvl0"),
  gzlvl3 = getval("gzlvl3"),
  gt3 = getval("gt3"),
  gzlvl4 = getval("gzlvl4"),
  gt4 = getval("gt4"),
  gzlvlr = getval("gzlvlr");

  getstr("pw11",pw11);
  getstr("C13refoc",C13refoc);
  getstr("flip",flip);
  getstr("f1180",f1180);


/* LOAD PHASE TABLE */
 
	settable(t1,4,phi1);
	settable(t2,8,phi11);
	settable(t3,8,phi12);
	settable(t10,8,rec);

/* INITIALIZE VARIABLES */

/* different offset values tof=H2O, dof=110ppm, dof2=200ppm */

	tof_75 = tof + 2.5*sfrq;        /* tof shifted to nH2 */
	dof2_120 = dof2 - 80*dfrq2;	/* dof2 shifted to middle of Nh and Nh2 */
	dof2_85 = dof2 - 115*dfrq2;     /* dof2 shifted to Nh2 */

/* selective H20 one-lobe sinc pulse (1.2ppm) */
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /* needs 1.69 times more */
        tpwrs = (int) (tpwrs);                   /* power than a square pulse */

/* selective H20 one-lobe sinc pulse (0.5ppm) */
        tpwrs1 = tpwr - 20.0*log10(pwHs1/(compH*pw*1.69));   /* needs 1.69 times more */
        tpwrs1 = (int) (tpwrs1);                   /* power than a square pulse */

/* selective H20 square pulse */
        tpwrs2 = tpwr - 20.0*log10(pwHs2/(compH*pw));
        tpwrs2 = (int) (tpwrs2);

/* number of cycles and mixing time */
        ncyc = (int) (ncyc + 0.5);


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

    if (ncyc > 25)
    {
        printf("CPMG heating! Reduce no. of ncyc!! ");
    }



/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);

        rcvroff();

        obspower(tpwr);
        decpower(pwClvl);
        dec2power(pwNlvl);

        obsoffset(tof_75);      /* Set the proton frequency to nH2 */
	dec2offset(dof2_85);    /* Set the nitrogen frequency to Nh2 */
        decoffset(dof);		/* Set the carbon frequency for decoupling */

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

	initval(ncyc,v1);
	lambdaN_int = ((2.0*lambdaN)/(2*ncyc));

        decphase(zero);
        dec2phase(zero);
        delay(5.0e-4);
        rcvroff();

  if(pw11[A] == 'y')
   {
        rgpulse(pw/2, zero, 50.0e-6, 0.0);
	delay(1/(2*(tof_75-tof)));
        rgpulse(pw/2, zero, 0.0, 0.0);
   }

  else
   {
	rgpulse(pw, zero, 50.0e-6, 0.0);
   }

	assign(zero,v5);

  if ( ncyc > 0 )
   {
        loop(v1,v14);

        assign(v5,v6);   /* v6 = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9*/
        hlv(v5,v7);      /* v7 = 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, */
        hlv(v7,v8);      /* v7 = 0, 0, 0, 0, 1, 1, 1, 1, 2, 2 */
        mod2(v6,v6);     /* v6 = 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 */
        mod2(v8,v8);     /* v7 = 0, 0, 0, 0, 1, 1, 1, 1, 0, 0 */
        add(v6,v8,v9);   /* v9 = 0, 1, 0, 1, 1, 2, 1, 2, 0, 1 */
        mod2(v9,v9);     /* v9 = 0, 1, 0, 1, 1, 0, 1, 0, 0, 1*/
        hlv(v8,v10);     /* v7 = 0, 0, 0, 0, 0, 0, 0, 0,
                                 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 */
        mod2(v10,v10);   /* v10 = 0, 0, 0, 0, 0, 0, 0, 0,
                                  1, 1, 1, 1, 1, 1, 1, 1)*/
        dbl(v10,v10);    /* v10 = 0, 0, 0, 0, 0, 0, 0, 0,
                                  2, 2, 2, 2, 2, 2, 2, 2)*/
        add(v9,v10,v11); /* v11 = 0, 1, 0, 1, 1, 0, 1, 0,
                                  2, 3, 2, 3, 3, 2, 3, 2)*/
        incr(v5);        /* increment v5 by 1 */
        txphase(v11);
        decphase(v11);
        delay(lambdaN_int);
        sim3pulse(2*pw,0.0,2.0*pwN,v11,zero,v11,0.0,0.0);
        delay(lambdaN_int);

        endloop(v14);
   }

	rgpulse(pw,one,0.0, 0.0);
        txphase(two);

  if (flip[A] == 'y')
   {
	txphase(zero);
        obspower(tpwrs);

        shaped_pulse("rna_H2Osinc", pwHs, zero, 5.0e-4, 0.0);
	dec2phase(t1);
        obspower(tpwr);
   }

	zgradpulse(-gzlvl3,gt3); /* gradient (negative) to remove unwanted
					coherence during INEPT transfer */
	dec2phase(t1);
	delay(grecov);

	dec2rgpulse(pwN,t1,0.0,0.0);

  if (C13refoc[A]=='y')
   {

        if (tau1 > (2.0*GRADIENT_DELAY + pwC + 0.64*pw ))
        {
         if (tau1>0.001)
         {
          zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwC - 0.64*pw));
          delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwC - 0.64*pw) );
         }
         else
          delay(tau1 - pwC - 0.64*pw );
 
         simpulse(2*pw, 2.0*pwC, one, zero, 0.0, 0.0);
         if (tau1>0.001)
         {
          zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwC - 0.64*pw));
          delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwC - 0.64*pw) );
         }
         else
          delay(tau1 - pwC - 0.64*pw);
        }
        else 
         if (tau1 > 0.64*pw )
          delay(2.0*tau1 - 2.0*0.64*pw );
   }
  else
   {
        if (tau1 > (2.0*GRADIENT_DELAY + pw + 0.64*pw ))
        {
         if (tau1>0.001)
         {
          zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pw - 0.64*pw));
          delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pw - 0.64*pw) );
         }
         else
          delay(tau1 - pw - 0.64*pw );
 
         rgpulse(2*pw, one, 0.0, 0.0);
         if (tau1>0.001)
         {
          zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pw - 0.64*pw));
          delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pw - 0.64*pw) );
         }
         else
          delay(tau1 - pw - 0.64*pw);
        }
        else 
         if (tau1 > 0.64*pw )
          delay(2.0*tau1 - 2.0*0.64*pw );
   }

	sim3pulse(pw,0.0,pwN,one,zero,zero,0.0,0.0);	/* Transfer back to proton */

        assign(zero,v5);

  if ( ncyc > 0 )
   {
        loop(v1,v14);

        assign(v5,v6);   /* v6 = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9*/
        hlv(v5,v7);      /* v7 = 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, */
        hlv(v7,v8);      /* v7 = 0, 0, 0, 0, 1, 1, 1, 1, 2, 2 */
        mod2(v6,v6);     /* v6 = 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 */
        mod2(v8,v8);     /* v7 = 0, 0, 0, 0, 1, 1, 1, 1, 0, 0 */
        add(v6,v8,v9);   /* v9 = 0, 1, 0, 1, 1, 2, 1, 2, 0, 1 */
        mod2(v9,v9);     /* v9 = 0, 1, 0, 1, 1, 0, 1, 0, 0, 1*/
        hlv(v8,v10);     /* v7 = 0, 0, 0, 0, 0, 0, 0, 0,
                                 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 */
        mod2(v10,v10);   /* v10 = 0, 0, 0, 0, 0, 0, 0, 0,
                                  1, 1, 1, 1, 1, 1, 1, 1)*/
        dbl(v10,v10);    /* v10 = 0, 0, 0, 0, 0, 0, 0, 0,
                                  2, 2, 2, 2, 2, 2, 2, 2)*/
        add(v9,v10,v11); /* v11 = 0, 1, 0, 1, 1, 0, 1, 0,
                                  2, 3, 2, 3, 3, 2, 3, 2)*/
        incr(v5);        /* increment v5 by 1 */
        txphase(v11);
        decphase(v11);
        delay(lambdaN_int);
        sim3pulse(2*pw,0.0,2.0*pwN,v11,zero,v11,0.0,0.0);
        delay(lambdaN_int);

        endloop(v14);
   }

	rgpulse(pw,two,0.0,0.0);
	txphase(two);
	obsoffset(tof);
	dec2offset(dof2_120);

	delay(mix - gt3 - grecov - pwHs1);

	zgradpulse(gzlvl3,gt3);
	delay(grecov);

        txphase(two);
        obspower(tpwrs1);
        shaped_pulse("rna_H2Osinc1", pwHs1, two, 0.0, 0.0);
        obspower(tpwr);

	rgpulse(pw, zero, 2*rof1, 0.0);
	txphase(t2);
	obspower(tpwrs2);
	
	zgradpulse(gzlvl4,gt4);
        delay(grecov - 2*SAPS_DELAY - 2*POWER_DELAY - GRADIENT_DELAY);

        rgpulse(pwHs2, t2, 0.0, 0.0);
        txphase(t3);
        obspower(tpwr);

        rgpulse(2*pw, t3, 0.0, 0.0);
        txphase(t2);
        obspower(tpwrs2);

        rgpulse(pwHs2, t2, 0.0, 0.0);

        zgradpulse(gzlvl4,gt4);
        delay(grecov - SAPS_DELAY - 3*POWER_DELAY - GRADIENT_DELAY);

        dec2power(dpwr2);               /* 2*POWER_DELAY */
        decpower(dpwr);

status(C);
	rcvron();

 setreceiver(t10);
}
