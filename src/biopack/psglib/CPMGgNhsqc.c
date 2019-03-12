/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  CPMGgNhsqc.c

    This pulse sequence will allow one to perform the following experiment:

    HSQC gradient sensitivity enhanced version for N15/H1 chemical shift
    correlation employing a CPMG pulse train instead of regular INEPT transfer
    for improved magnetization transfer of exchange broadened resonances.
    Optional C13 refocusing and editing spectral regions.

    pulse sequence: Mulder, F.A.A. et al, J. Biom. NMR, 8 (1996) 223-28.



                  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for C13 decoupling.
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for N15 decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].

    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.



                  DETAILED INSTRUCTIONS FOR USE OF CPMGgNhsqc


    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro:
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for CPMGgNhsqc may be printed using:
                                      "printon man('CPMGgNhsqc') printoff".

    2. Apply the setup macro "CPMGgNhsqc".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 110ppm, and N15
       frequency on the amide N region (120 ppm).

    4. CPMG pulse train:
       ncyc should be set to multiple of 8 (8, 16 or 24).

       modified from rna_CPMGgNhsqc written by Peter Lukavsky(Stanford) for
       RnaPack
*/

#include <standard.h>


static int  phi1[1]  = {1},
	    phi2[2]  = {0,2},
	    phi3[1]  = {1},
	    phi9[8]  = {0,0,1,1,2,2,3,3},
            phi10[1] = {0},
	    phi11[1] = {0},
            phi12[1] = {1},
	    phi13[1] = {0},
	    rec[4]   = {0,2,2,0};

static double   d2_init=0.0;


void pulsesequence()
{

/* DECLARE VARIABLES */


char	f1180[MAXSTR];                        /* Flag to start t1 @ halfdwell */

int     icosel,                                   /* used to get n and p type */
        t1_counter;         

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

	ncyc = getval("ncyc"),	   	    /* number of cycles in CPMG train */

	sw1 = getval("sw1"),
        grecov = getval("grecov"),

        pwHs = getval("pwHs"),          /* H1 90 degree pulse length at tpwrs */
        tpwrs,                    /* power for the pwHs ("H2Osinc") pulse */

        pwHs2 = getval("pwHs2"),       /* H1 90 degree pulse length at tpwrs2 */
        tpwrs2,                           /* power for the pwHs2 square pulse */

        gt1 = getval("gt1"),                   /* coherence pathway gradients */
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),

  gzlvl0 = getval("gzlvl0"),
  gzlvl3 = getval("gzlvl3"),
  gt3 = getval("gt3");

  getstr("f1180",f1180);


/* LOAD PHASE TABLE */
 
	settable(t1,1,phi1);
	settable(t2,2,phi2);
	settable(t3,1,phi3);
	settable(t9,8,phi9);
	settable(t10,1,phi10);
        settable(t11,1,phi11);
        settable(t12,1,phi12);
        settable(t13,1,phi13);
	settable(t14,4,rec);

/* INITIALIZE VARIABLES */


/* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /* needs 1.69 times more */
        tpwrs = (int) (tpwrs);                   /* power than a square pulse */

/* selective H20 square pulse */
        tpwrs2 = tpwr - 20.0*log10(pwHs2/(compH*pw));
        tpwrs2 = (int) (tpwrs2);

/* number of cycles and mixing time */
        ncyc = (int) (ncyc + 0.5);


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 1)
         { tsadd(t10,2,4); icosel = 1; }
    else icosel = -1;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t2,2,4); tsadd(t14,2,4); }

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

    if (ncyc > 24)
    {
        printf("CPMG heating! Reduce no. of ncyc!! ");
    }



/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);

        obspower(tpwr);
        decpower(pwClvl);
        dec2power(pwNlvl);

        txphase(zero);
        decphase(zero);
        dec2phase(zero);

        delay(d1);
        rcvroff();

        dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 magnetization*/
        zgradpulse(gzlvl0, 0.5e-3);
        delay(grecov/2);
        dec2rgpulse(pwN, one, 0.0, 0.0);
        zgradpulse(0.7*gzlvl0, 0.5e-3);

	initval(ncyc,v1);
	lambdaN_int = ((2.0*lambdaN)/(2*ncyc));

        dec2phase(zero);
        delay(5.0e-4);
        rcvroff();

	rgpulse(pw, zero, 50.0e-6, 0.0);

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
	txphase(t1);
   }

	rgpulse(pw,t1,0.0, 0.0);
        txphase(one);
	dec2phase(t2);

        obspower(tpwrs);
        shaped_pulse("H2Osinc", pwHs, one, 5.0e-4, 0.0);
	txphase(t11);
        obspower(tpwr);

	zgradpulse(gzlvl3,gt3);
	delay(grecov);

	dec2rgpulse(pwN,t2,0.0,0.0);
	dec2phase(t9);

	delay(tau1);
	rgpulse(2.0*pw, t11, 0.0, 0.0);
	delay(tau1);

        delay(gt1 + grecov - 2.0*pw - SAPS_DELAY);

        dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);
        dec2phase(t10);

        zgradpulse(gzlvl1, gt1);                 /* 2.0*GRADIENT_DELAY */
        delay(grecov - 2.0*GRADIENT_DELAY);


	sim3pulse(pw,0.0,pwN,zero,zero,t10,0.0,0.0);

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

	txphase(t12);
	dec2phase(t3);

	sim3pulse(pw, 0.0, pwN, t12, zero, t3, 0.0, 0.0);

	txphase(zero);
        dec2phase(zero);

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

        rgpulse(pw, zero, 0.0, 0.0);
	txphase(t13);

        delay((gt1/10.0) + grecov/2 - 0.5*pw + 2.0*GRADIENT_DELAY - SAPS_DELAY + 2*POWER_DELAY);

        rgpulse(2.0*pw, t13, 0.0, 0.0);

        decpower(dpwr);
        dec2power(dpwr2);                                      /* POWER_DELAY */
        zgradpulse(icosel*gzlvl2, gt1/10.0);                   /* 2.0*GRADIENT_DELAY */
        delay(grecov/2);


status(C);
        setreceiver(t14);
}
