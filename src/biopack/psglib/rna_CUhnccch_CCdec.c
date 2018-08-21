/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_CUhnccch_CCdec.c

   (see Kwaku Dayie, J.Biomol.NMR, 32, 129-139(2005))

for C-optimized experiments
 C-decoupling instead of CT

    This pulse sequence will allow one to perform the following experiment:

    Base-correlation from Uridine-H5/H6 to imino or Cytidine H5/H6 to amino 

    H5/H6    -->   C5/C6   -->   C4   -->   N4 -->   H4 - det
      	INEPT	    t1    TOCSY        CP       INEPT     re-INEPT
	
	


                      NOTE dof MUST BE SET AT 110ppm ALWAYS
                      NOTE dof2 MUST BE SET AT 200ppm ALWAYS

    Experiment may not be feasible at >600 MHz because of power requirements.
    PSG will abort as a safety measure. 


    pulse sequence:	Wohnert et al J. Biom. NMR, 26 (2003), 79-83.
    pulse sequence:	Dayie J. Biom. NMR, 32 (2005), 129-39.


                  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [H1].

    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in H1.  f1180='y' is ignored if ni=0.



                  DETAILED INSTRUCTIONS FOR USE OF rna_CUhnccch


    3. Centre H1 frequency on H2O (4.7ppm), N15 frequency on the aromatic N
       region (200 ppm), and C13 frequency on 110ppm.

        @   Written for RnaPack by Kwaku Dayie (02/04).   @
        @   as  rna_CUhnccchWSC13N15_CdecIa.c             @
        @   Modified for BioPack, GG, Varian 12/2007      @
        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/

#include <standard.h>

static int  phi1[2] = {0,2},
	    phi3[8] = {0,0,0,0, 2,2,2,2},
	    phi5[4] = {0,0,2,2},
	    phi4[16]= {0,0,0,0, 0,0,0,0, 2,2,2,2, 2,2,2,2},
	    rec2[8] = {0,2,2,0, 2,0,0,2};

static double   d2_init=0.0,  d3_init=0.0;


pulsesequence()
{

/* DECLARE VARIABLES */

char
	URA[MAXSTR],				  /* Setup for U-imino - U-H6 */
	flipback[MAXSTR],			

        CCdseq[MAXSTR],

	CYT[MAXSTR],				  /* Setup for C-imino - C-H6 */
	CChomodec[MAXSTR],			  /* Setup for C-imino - C-H6 */
	C5[MAXSTR],				  /* Setup for C-imino - C-H6 */
	C6[MAXSTR],				  /* Setup for C-imino - C-H6 */
        CT[MAXSTR],                                /* constant time in t1 */
	N15refoc[MAXSTR],                         /* N15 pulse in middle of t1*/
	f1180[MAXSTR],                        /* Flag to start t1 @ halfdwell */
        f2180[MAXSTR];                        /* Flag to start t1 @ halfdwell */

int	ni2 = getval("ni2"),
        t1_counter,
        t2_counter;

double      
        CCdpwr = getval("CCdpwr"),    /*   power level for CC decoupling */
        CCdres = getval("CCdres"),    /*   dres for CC decoupling */
        CCdmf = getval("CCdmf"),      /*   dmf for CC decoupling */

	tau1,                                                /*  t1 delay */
        tau2,                                                /*  t2 delay */
	    lambda = 0.94/(4.0*getval("JCH")),        /* 1/4J C-H INEPT delay */
	    lambdaN = 0.94/(4.0*getval("JNH")),       /* 1/4J N-H INEPT delay */

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
        tof_65,                  /* tof shifted to 6.0 ppm for H4-N4 transfer */
        tof_125,                   /* tof shifted to 12 ppm for H3-N3 transfer */

	dof_169,		 /* dof shifted to 169 ppm for N3-C4 transfer */
	dof_140,     /* dof shifted to 140 ppm for C4-C5-C6 transfer and DEC1 */
	dof_104,     /* dof shifted to 104 ppm for C4-C5-C6 transfer and DEC1 */
	dof_153,     /* dof shifted to 153 ppm for C4-C5-C6 transfer and DEC1 */
	dof_135,     /* dof shifted to 135 ppm for C4-C5-C6 transfer and DEC1 */
	dof_120,     /* dof shifted to 120 ppm for C4-C5-C6 transfer and DEC1 */
	dof_130,     /* dof shifted to 130 ppm for C4-C5-C6 transfer and DEC1 */
	dof_141,     /* dof shifted to 141 ppm for C4-C5-C6 transfer and DEC1 */
	dof_133,     /* dof shifted to 132.5 ppm for C4-C5-C6 transfer and DEC1 */
	dof_123,     /* dof shifted to 122.5 ppm for C4-C5-C6 transfer and DEC1 */
	dof_98,     /* dof shifted to 98.0 ppm for C4-C5-C6 transfer and DEC1 */
	dof_175,     /* dof shifted to 175 ppm for C4-C5-C6 transfer and DEC1 */

	dof2_98,       /* dof2 shifted to 98.5 ppm for H4-N4 and N4-C4 transfer */
        dof2_160,     /* dof2 shifted to 160 ppm for H3-N3 and N3-C4 transfer */

/* p_d is used to calculate the isotropic mixing */
        p_d,                 /* 50 degree pulse for DIPSI-3 at rfdC-rfdN-rfdH */
        pwZa,                /* the largest of 2.0*pw and 2.0*pwN */
        rfdC,             /* fine C13 power for 1.9 kHz rf for 500MHz magnet  */
        p_d2,                /* 50 degree pulse for DIPSI-3 at rfdC3 */
        rfdC3,             /* fine C13 power for 10 kHz rf for 500MHz magnet */
        rfdN,             /* fine N15 power for 1.9 kHz rf for 500MHz magnet  */
        rfdH,              /* fine H1 power for 1.9 kHz rf for 500MHz magnet  */
        ncyc_hn = getval("ncyc_hn"),  /* number of pulsed cycles in HN half-DIPSI-3 */
        ncyc_nc = getval("ncyc_nc"), /* number of pulsed cycles in NC DIPSI-3 */
        ncyc_cc = getval("ncyc_cc"), /* number of pulsed cycles in CC DIPSI-3 */

        CTdelay = getval("CTdelay"),     /* total constant time evolution */

	sw1 = getval("sw1"),
        sw2 = getval("sw2"),
        gstab = getval("gstab"),

        finepwrf = getval("finepwrf"), /*     fine power adjustment           */

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

  getstr("URA",URA);
  getstr("flipback",flipback);
  getstr("CYT",CYT);
  getstr("C5",C5);
  getstr("C6",C6);
    getstr("CT",CT);
  getstr("N15refoc",N15refoc);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("CCdseq",CCdseq);
  getstr("CChomodec",CChomodec);


/* LOAD PHASE TABLE */
/*
static int  phi1[2] = {0,2},
	    phi3[8] = {0,0,0,0, 2,2,2,2},
            phi4[16]= {0,0,0,0, 0,0,0,0, 2,2,2,2, 2,2,2,2},
            phi5[4] = {0,0,2,2},
            rec2[8] = {0,2,2,0, 2,0,0,2};

*/
 
	settable(t1,2,phi1);
	settable(t3,8,phi3);
	settable(t4,16,phi4);
	settable(t5,4,phi5);
	settable(t10,8,rec2);


/* INITIALIZE VARIABLES */
  if (2.0*pw > 2.0*pwN) pwZa = 2.0*pw;
  else pwZa = 2.0*pwN;


/* maximum fine power for pwC pulses */
        rfC = 4095.0;

/* maximum fine power for pwN pulses */
        rfN = 4095.0;

/* maximum fine power for pw pulses */
        rfH = 4095.0;

/* different offset values tof=H2O, dof=110ppm, dof2=200ppm */
/* For U 10-15 ppm in Imino region during acquisition (ie 12.5 +/- 2.5 ppm)
   and 4.5 -9 ppm during indirect dimensional acquisition (ie 6.75 +/- 2.25 ppm)
   For C 4.5 -9ppm (6.75 +/- 2.25ppm) during indirect acqusisition and 6-9ppm during direct (7.5 +/- 1.5ppm)
*/
	tof_65 = tof + 2.05*sfrq;       /* tof shifted to nH2/nH */
	tof_75 = tof + 2.5*sfrq;        /* tof shifted to nH2 */
	tof_125 = tof + 7.8*sfrq;	/* tof shifted to nH */
	dof_175 = dof + 65*dfrq;	/* dof shifted to C4 */
	dof_169 = dof + 59*dfrq;	/* dof shifted to C4 */
	dof_140 = dof + 30*dfrq;	/* dof shifted to C6 */
	dof_104 = dof - 6.0*dfrq;	/* dof shifted to C6 */
	dof_141 = dof + 31*dfrq;	/* dof shifted to C6 */
	dof_153 = dof + 43*dfrq;	/* dof shifted to C6 */
	dof_135 = dof + 25*dfrq;	/* dof shifted to C6 */
	dof_133 = dof + 22.5*dfrq;	/* dof shifted to C6 */
	dof_120 = dof + 10*dfrq;	/* dof shifted to C6 */
	dof_130 = dof + 20*dfrq;	/* dof shifted to C6 */
	dof_98 = dof - 12*dfrq;	        /* dof shifted to C6 */
	dof_123 = dof + 12.5*dfrq;	/* dof shifted to C6 */
	dof2_160 = dof2 - 40*dfrq2;	/* dof2 shifted to Nh */
	dof2_98 = dof2 - 101.5*dfrq2;   /* dof2 shifted to Nh2 */

/* 1.9 kHz field strength DIPSI-3 at 500MHz adjusted for this sfrq*/
        p_d = (5.0)/(9.0*4.0*1900.0*(sfrq/500.0)); 

/* fine C13 power for dipsi-3 isotropic mixing on C4 region */
        rfdC = (compC*4095.0*pwC*5.0)/(p_d*9.0);
        rfdC = (int) (rfdC + 0.5);

/* 10 kHz field strength DIPSI-3 at 500MHz adjusted for this sfrq*/
        p_d2 = (5.0)/(9.0*4.0*10000.0*(sfrq/500.0)); 

/* fine C13 power for dipsi-3 isotropic mixing on C2/C6 region */
        rfdC3 = (compC*4095.0*pwC*5.0)/(p_d2*9.0);
        rfdC3 = (int) (rfdC3 + 0.5);

/* fine N15 power for dipsi-3 isotropic mixing on Nh region */
        rfdN = (compN*4095.0*pwN*5.0)/(p_d*9.0);
        rfdN = (int) (rfdN + 0.5);

/* fine H1 power for half dipsi-3 isotropic mixing on nH2 region */
        rfdH = (compH*4095.0*pw*5.0)/(p_d*9.0);
        rfdH = (int) (rfdH + 0.5);

/* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /* needs 1.69 times more */
        tpwrs = (int) (tpwrs);                         /* power than a square pulse */

/* selective H20 square pulse */
        tpwrs2 = tpwr - 20.0*log10(pwHs2/(compH*pw));
        tpwrs2 = (int) (tpwrs2);

/* number of cycles and mixing time */
        ncyc_nc = (int) (ncyc_nc + 0.5);
	ncyc_hn = (int) (ncyc_hn + 0.5);
        ncyc_cc = (int) (ncyc_cc + 0.5);

  if (ncyc_nc > 0 )
   {
        printf("NC-mixing time is %f ms.\n",(ncyc_nc*51.8*4*p_d));
   }

  if (ncyc_cc > 0 )
   {
        printf("CC-mixing time is %f s.\n",(ncyc_cc*51.8*4*p_d2));
   }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)
         tsadd(t5,1,4);

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t5,2,4); tsadd(t10,2,4); }

/*  Set up f1180  */

    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0))
        { tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;

        if (phase2 == 2)
        {
                tsadd(t3,1,4);
        }

/*  Set up f2180  */
       tau2 = d3;
  if((f2180[A] == 'y') && (ni2 > 1.0))
   {
        tau2 += ( 1.0 / (2.0*sw2) );
        if(tau2 < 0.2e-6) tau2 = 0.0;
   }

        tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */
  if( ix == 1) d3_init = d3;

        t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );

  if(t2_counter % 2)
   {
        tsadd(t3,2,4);
        tsadd(t10,2,4);
   }



/* CHECK VALIDITY OF PARAMETER RANGE */


  if ((CT[A]=='y') && (ni/sw1 > CTdelay))
  { text_error( " ni is too big. Make ni equal to %d or less.\n",
      ((int)(CTdelay*sw1)) );                                       psg_abort(1); }


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

    if( ncyc_cc > 7 )
    {
        text_error("check C->C dipsi-3 time !! ");
        psg_abort(1);
    }


    if((C5[A] == 'y') && ( ncyc_cc > 4) )
    {
        text_error("check C->C dipsi-3 time equal to 6.5 ms !! ");
        psg_abort(1);
    }


    if((C6[A] == 'y') && (( ncyc_cc > 6) || ( ncyc_cc < 3)))
    {
        text_error("check C->C dipsi-3 time equal to 13 ms !! ");
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


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);


        obspower(tpwr);
	obspwrf(rfH);
	obsstepsize(0.5);
        decpower(pwClvl);
        decpwrf(rfC);
	decstepsize(0.5);
        dec2power(pwNlvl);
	dec2pwrf(rfN);
	dec2stepsize(0.5);

        if (C6[A]=='y') decoffset(dof_141);	/* frequency for the NC-tocsy */

  if (URA[A] == 'y')
   {
        obsoffset(tof_65);	/* Set the proton frequency to U-nH */
        dec2offset(dof2_160);   /* Set the nitrogen frequency to U-Nh */
        if (C5[A]=='y') decoffset(dof_104);
   }
  else if (CYT[A] == 'y')
   {
        obsoffset(tof_65);      /* Set the proton frequency to C-nH2 */
	dec2offset(dof2_98);    /* Set the nitrogen frequency to C-Nh2 */
        if (C5[A]=='y') decoffset(dof_98);
   }
  else
   {
   }


        txphase(zero);
        decphase(zero);
        dec2phase(zero);

        delay(d1);
        rcvroff();
        dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
        decrgpulse(pwC, zero, 0.0, 0.0);
        zgradpulse(gzlvl0, 0.5e-3);
        delay(1.0e-4);
        dec2rgpulse(pwN, one, 0.0, 0.0);
        decrgpulse(pwC, one, 0.0, 0.0);
        zgradpulse(0.7*gzlvl0, 0.5e-3);
        delay(5.0e-4);


	initval(ncyc_nc,v11);

        initval(ncyc_cc,v2);

        txphase(t1);
        decphase(zero);
        dec2phase(zero);
        delay(5.0e-4);
        rcvroff();

	rgpulse(pw, t1, 50.0e-6, 0.0); /* x,-x */
	txphase(zero);


	delay(lambda);

	simpulse(2*pw, 2*pwC, zero, zero, 0.0, 0.0);
	decphase(t5);

	delay(lambda);

	simpulse(pw, pwC, one, t5, 0.0, 0.0); /* x, -x */
	decphase(zero);

	zgradpulse(gzlvl5,gt5);
	delay(lambda - gt5);

	simpulse(2*pw, 2*pwC, one, zero, 0.0, 0.0);

	zgradpulse(gzlvl5,gt5);
        delay(lambda - gt5 - 2*POWER_DELAY);



if (CChomodec[A]=='y')
{

decpower(CCdpwr); decphase(zero);
decprgon(CCdseq,1.0/CCdmf,CCdres); 
decon();  /* CC decoupling on */


   if (N15refoc[A]=='y')
    {
        if (tau1 > (pwN + 0.64*pw))
        {
        delay(tau1 - pwN - 0.64*pw);
        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        delay(tau1 - pwN - 0.64*pw);
        }
        else if (tau1 > 0.64*pw)
        delay(2.0*tau1 - 2.0*0.64*pw);
   }
  else
   {
        if (tau1 > pw)
        {
        delay(tau1 - 0.64*pw);
        rgpulse(2.0*pw, zero, 0.0, 0.0);
        delay(tau1 - 0.64*pw);
        }
        else 
        delay(2.0*tau1);
   }

decoff(); decprgoff();        /* CC decoupling off */
decpower(pwClvl);

} /* END CC HOMO DEC */


else
{
     /*****************     CONSTANT TIME EVOLUTION      *****************/
      if (CT[A]=='y') {
     /***************/

    delay(CTdelay/2.0 - tau1);

    decrgpulse(2.0*pwC, zero, 2.0e-6, 0.0);

    {delay(CTdelay/2.0 - pwZa);
           sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);}

    delay(tau1);

     /***************/
                      }


 else  if (N15refoc[A]=='y')
   {

        if (tau1 > (2.0*GRADIENT_DELAY + pwN + 0.64*pw + 5.0*SAPS_DELAY))
        {
        zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw) - SAPS_DELAY);
        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        zgradpulse(-1.0*gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        }
        else if (tau1 > (0.64*pw + 0.5*SAPS_DELAY))
        delay(2.0*tau1 - 2.0*0.64*pw - SAPS_DELAY );
   }
  else
   {
        if (tau1 > (2.0*GRADIENT_DELAY + pw + 5.0*SAPS_DELAY))
        {
        zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw) - SAPS_DELAY);
        rgpulse(2.0*pw, zero, 0.0, 0.0);
        zgradpulse(-1.0*gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
        }
        else if (tau1 > (pw + 0.5*SAPS_DELAY))
        delay(2.0*tau1 - 2.0*pw - SAPS_DELAY );
   }
} /* End No CC homodec */

        decrgpulse(pwC,one,0.0,0.0);  /* flip transferred 13C-magnetization to x */

        decoffset(dof_135);	/* frequency for the NC-tocsy */

        decrgpulse(pwC,three,0.0,0.0);  /* flip transferred 13C-magnetization to x */
        decphase(zero);
        decpwrf(rfdC3);

        starthardloop(v2);
    decrgpulse(6.4*p_d2,zero,0.0,0.0);
    decrgpulse(8.2*p_d2,two,0.0,0.0);
    decrgpulse(5.8*p_d2,zero,0.0,0.0);
    decrgpulse(5.7*p_d2,two,0.0,0.0);
    decrgpulse(0.6*p_d2,zero,0.0,0.0);
    decrgpulse(4.9*p_d2,two,0.0,0.0);
    decrgpulse(7.5*p_d2,zero,0.0,0.0);
    decrgpulse(5.3*p_d2,two,0.0,0.0);
    decrgpulse(7.4*p_d2,zero,0.0,0.0);

    decrgpulse(6.4*p_d2,two,0.0,0.0);
    decrgpulse(8.2*p_d2,zero,0.0,0.0);
    decrgpulse(5.8*p_d2,two,0.0,0.0);
    decrgpulse(5.7*p_d2,zero,0.0,0.0);
    decrgpulse(0.6*p_d2,two,0.0,0.0);
    decrgpulse(4.9*p_d2,zero,0.0,0.0);
    decrgpulse(7.5*p_d2,two,0.0,0.0);
    decrgpulse(5.3*p_d2,zero,0.0,0.0);
    decrgpulse(7.4*p_d2,two,0.0,0.0);

    decrgpulse(6.4*p_d2,two,0.0,0.0);
    decrgpulse(8.2*p_d2,zero,0.0,0.0);
    decrgpulse(5.8*p_d2,two,0.0,0.0);
    decrgpulse(5.7*p_d2,zero,0.0,0.0);
    decrgpulse(0.6*p_d2,two,0.0,0.0);
    decrgpulse(4.9*p_d2,zero,0.0,0.0);
    decrgpulse(7.5*p_d2,two,0.0,0.0);
    decrgpulse(5.3*p_d2,zero,0.0,0.0);
    decrgpulse(7.4*p_d2,two,0.0,0.0);

    decrgpulse(6.4*p_d2,zero,0.0,0.0);
    decrgpulse(8.2*p_d2,two,0.0,0.0);
    decrgpulse(5.8*p_d2,zero,0.0,0.0);
    decrgpulse(5.7*p_d2,two,0.0,0.0);
    decrgpulse(0.6*p_d2,zero,0.0,0.0);
    decrgpulse(4.9*p_d2,two,0.0,0.0);
    decrgpulse(7.5*p_d2,zero,0.0,0.0);
    decrgpulse(5.3*p_d2,two,0.0,0.0);
    decrgpulse(7.4*p_d2,zero,0.0,0.0);
        endhardloop();


        decphase(one);

        decpwrf(rfC);

        decrgpulse(pwC,three,0.0,0.0);  /* flip transferred 13C-magnetization to x */

	decoffset(dof_175);

        decrgpulse(pwC,one,0.0,0.0);  /* flip transferred 13C-magnetization to x */

        decpwrf(rfdC);          /* Set fine power for carbon */
        dec2pwrf(rfdN);         /* Set fine power for nitrogen */

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

	txphase(zero);
	decphase(one);


if (tau2 > 0.0)
   {

        if (tau2 > (2.0*GRADIENT_DELAY + pwC + 0.64*pw + 5.0*SAPS_DELAY))
        {
        zgradpulse(gzlvlr, 0.8*(tau2 - 2.0*GRADIENT_DELAY - pwC - 0.64*pw));
        delay(0.2*(tau2 - 2.0*GRADIENT_DELAY - pwC - 0.64*pw) - SAPS_DELAY);
        simpulse(2.0*pw, 2.0*pwC, zero, zero,  0.0, 0.0);
        zgradpulse(-1.0*gzlvlr, 0.8*(tau2 - 2.0*GRADIENT_DELAY - pwC - 0.64*pw));
        delay(0.2*(tau2 - 2.0*GRADIENT_DELAY - pwC - 0.64*pw));
        }
        else if (tau2 > (0.64*pw + 0.5*SAPS_DELAY))
        delay(2.0*tau2 - 2.0*0.64*pw - SAPS_DELAY );
   }


else
{;}

    if( CYT[A] == 'y' )
        {
        zgradpulse(gzlvl5,gt5);
        delay(lambdaN/2.0 - gt5);
        sim3pulse(2*pw, 0.0, 2*pwN,zero, zero,zero,0.0,0.0);
        zgradpulse(gzlvl5,gt5);
        delay(lambdaN/2.0 - gt5);

        }
    else if( URA[A] == 'y' )
        {
        zgradpulse(gzlvl5,gt5);
        delay(lambdaN - gt5);
        sim3pulse(2*pw, 0.0, 2*pwN,zero, zero,zero,0.0,0.0);
        zgradpulse(gzlvl5,gt5);
        delay(lambdaN - gt5);
        }

	dec2rgpulse(pwN,t3,0.0,0.0);

if (flipback[A]=='y')
{
	zgradpulse(gzlvl3,gt3);
	delay(gstab);

        txphase(zero);
        obspower(tpwrs);
        shaped_pulse("rna_H2Osinc", pwHs, zero, 5.0e-4, 0.0);
        obspower(tpwr);
}
	rgpulse(pw, zero, 2*rof1, 0.0);
	txphase(two);
	obspower(tpwrs2);
        obspwrf(finepwrf);
	
        zgradpulse(gzlvl4,gt4);
        delay(lambdaN - 2.0*POWER_DELAY - gt4 -rof1 -2.0*GRADIENT_DELAY - pwHs2);

        rgpulse(pwHs2, two, rof1, rof1);
        obspower(tpwr);
        obspwrf(4095.0);
        sim3pulse(2*pw, 0.0, 2*pwN, zero, zero, zero, rof1, rof1);
        obspwrf(finepwrf);
        obspower(tpwrs2);
        rgpulse(pwHs2, two, rof1, rof1);

        zgradpulse(gzlvl4,gt4);
        delay(lambdaN - 3*POWER_DELAY - gt4 - 2.0*GRADIENT_DELAY - pwHs2);


        dec2rgpulse(pwN,t4,0.0,0.0);
        dec2rgpulse(pwN,zero,0.0,0.0);
        dec2power(dpwr2);               /* 2*POWER_DELAY */
        decpower(dpwr);

status(C);
	rcvron();

 setreceiver(t10);
}
