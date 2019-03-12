/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_WGgNhsqc.c

    This pulse sequence will allow one to perform the following experiment:

    WATERGATE HSQC for N15 with options for refocussing during t1 and
    editing spectral regions.


                      NOTE dof MUST BE SET AT 110ppm ALWAYS
                      NOTE dof2 MUST BE SET AT 200ppm ALWAYS


    pulse sequence: 	Sklenar V., JMR, A102, 241 (1993)


        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for C13 decoupling.
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for N15 decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.


          	  DETAILED INSTRUCTIONS FOR USE OF rna_WGgNhsqc

         
    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro: 
                                      "printon man('RnaPack') printoff".
       These Detailed Instructions for rna_WGgNhsqc may be printed using:
                                      "printon man('rna_WGgNhsqc') printoff".
             
    2. Apply the setup macro "rna_WGgNhsqc".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check of Nimino.

    3. Apply the setup macro "rna_WGgNhsqclr".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check of Naromatic.

    4. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 110ppm, and N15 
       frequency on the aromatic N region (200 ppm).

    5. CHOICE OF N15 REGION:
       amino='y' gives a spectrum of amino resonances centered on dof2=85ppm.
       This is a common usage.                               Set sw1 to 40ppm.

       imino='y' gives a spectrum of imino groups.  dof2 is shifted
       automatically by the pulse sequence code to 155ppm.  Set sw1 to 25ppm.

       In both cases set sw=22ppm and JNH=95Hz.

       Naro='y' gives a spectrum of Naromatic (N1/N3 and N7/N9) resonances
       centered on dof2=200ppm.				Set sw1 to 90ppm.
       Use JNH=22Hz (actual value 10-14Hz).

    6. CHOICE of t1 decoupling:
       H1refoc='y' and C13refoc='y' decouples H1 and C13 during t1.
       H1refoc='n' and C13refoc='y' decouples only C13 during t1.
       H1refoc='n' and C13refoc='n' does no decoupling during t1.

    7. WATERGATE:
       flipback='y' adds selective flipback H2O sinc one-lobe pulse.
       The WATERGATE employs low power square pulses.
       Water suppresssion may be improved by varying the finepwrf
       level. Theoretically, it should be ~2048. Vary +-10% to find 
       best result, as indicated by minimal H2O signal.

    8. SHAPE:
       Use shaped N15 pulses for selection of amino or imino or Naromatic.


        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        @                                                      @
        @   Written for RnaPack by Peter Lukavsky (10/98).     @
        @                                                      @
        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@



*/



#include <standard.h>
  


static int   phi1[8]  = {1,1,3,3,3,3,1,1},
	     phi2[2]  = {0,2},
             phi3[1]  = {2},
	     phi4[1]  = {0},
             rec[8]   = {0,2,2,0,2,0,0,2};

static double   d2_init=0.0;


void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
	    C13refoc[MAXSTR],	      /* C13 refocussing pulse in middle of t1*/
	    flipback[MAXSTR],                    /* flipback watergate option */
	    H1refoc[MAXSTR],
            SHAPE[MAXSTR],
            rna_stNshape[MAXSTR],     /* calls sech/tanh pulses from shapelib */
            imino[MAXSTR],			/* Sets dof2 for imino region */
            amino[MAXSTR],			/* Sets dof2 for amino region */
	    Naro[MAXSTR];			/* Sets dof2 for Narom region */
 
int         t1_counter;  		        /* used for states tppi in t1 */

double      tau1,         				         /*  t1 delay */
	    lambda = 0.94/(4.0*getval("JNH")), 	   /* 1/4J H1 evolution delay */
        
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */

   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrs,	  	          /* power for the pwHs ("rna_H2Osinc") pulse */
        finepwrf = getval("finepwrf"), /*     fine power adjustment           */
        pwHs2 = getval("pwHs2"),       /* H1 90 degree pulse length at tpwrs2 */
        tpwrs2,                           /* power for the pwHs2 square pulse */
	compH = getval("compH"),

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
	rfN,                      /* maximum fine power when using pwN pulses */
	compN = getval("compN"),
/* Sech/tanh inversion pulses automatically calculated by macro "rna_cal"     */
/* and string parameter rna_stCshape calls them from your shapelib.           */
   rfstN = 0.0,         /* fine power for the rna_stCshape pulse, initialised */
   dof2a, 				      /* offset for imino/amino/Naro  */


	pwZa,                            /* the largest of 2.0*pw and 2.0*pwC */

	sw1 = getval("sw1"),

        grecov = getval("grecov"),

	gt3 = getval("gt3"),                               /* other gradients */
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
        gzlvlr = getval("gzlvlr");

    getstr("f1180",f1180);
    getstr("C13refoc",C13refoc);
    getstr("flipback",flipback);
    getstr("H1refoc",H1refoc);
    getstr("SHAPE",SHAPE);
    getstr("imino",imino);
    getstr("amino",amino);
    getstr("Naro",Naro);



/*   LOAD PHASE TABLE    */

	settable(t1,8,phi1);
	settable(t2,2,phi2);
	settable(t3,1,phi3);
	settable(t4,1,phi4);
	settable(t11,8,rec);


/*   INITIALIZE VARIABLES   */

/* optional refocusing of C13 coupling for C13 enriched samples */
  if (C13refoc[A]=='n')  pwC = 0.0;
  if (2.0*pw > 2.0*pwC) pwZa = 2.0*pw;
  else pwZa = 2.0*pwC;

/* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /* needs 1.69 times more */
	tpwrs = (int) (tpwrs);                   /* power than a square pulse */

/* selective H20 square pulse */
        tpwrs2 = tpwr  - 20.0*log10(pwHs2/(compH*pw));
        tpwrs2 = (int) (tpwrs2);
        tpwrs2=tpwrs2+6;        /* increase attenuator setting so that
           fine power control can be varied about the nominal 2048 value */

/* maximum fine power for pwN pulses */
        rfN = 4095.0;

  if(amino[A] == 'y')
   {
        dof2a = dof2 - 115*dfrq2;

        /* 50ppm sech/tanh inversion */
        rfstN = (compN*4095.0*pwN*4000.0*sqrt((3.0*sfrq/600+3.85)/0.41));
        rfstN = (int) (rfstN + 0.5);
        strcpy(rna_stNshape, "rna_stN50");
   }
  else if(imino[A] == 'y')
   {
	dof2a = dof2 - 45*dfrq2;

        /* 50ppm sech/tanh inversion */
        rfstN = (compN*4095.0*pwN*4000.0*sqrt((3.0*sfrq/600+3.85)/0.41));
        rfstN = (int) (rfstN + 0.5);
        strcpy(rna_stNshape, "rna_stN50");
   }
  else
   {
	dof2a = dof2;

        /* 50ppm sech/tanh inversion */
        rfstN = (compN*4095.0*pwN*4000.0*sqrt((6.0*sfrq/600+3.85)/0.41));
        rfstN = (int) (rfstN + 0.5);
        strcpy(rna_stNshape, "rna_stN100");
   }


/* CHECK VALIDITY OF PARAMETER RANGES */

  if ( ((imino[A]=='y') && (amino[A]=='y')) || ((Naro[A]=='y') && (amino[A]=='y')) ||
	((imino[A]=='y') && (Naro[A]=='y')) )
   {
        printf(" Choose  ONE  of  imino='y'  OR  amino='y' or Naro='y' !!");
        psg_abort(1);
   }

  if ( (Naro[A] == 'y') && (lambda < 7.83e-3) )
  { text_error("Set JNH below 30 Hz !!!! "); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' or 'nny' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }

  if( (pw > 20.0e-6) && (tpwr > 56) )
  { text_error("don't fry the probe, pw too high ! "); psg_abort(1); }

  if( (pwC > 40.0e-6) && (pwClvl > 56) )
  { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }

  if( (pwN > 100.0e-6) && (pwNlvl > 56) )
  { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2) 
	tsadd(t2,1,4);

/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 0.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t11,2,4); }


/* BEGIN PULSE SEQUENCE */

status(A);
	obsoffset(tof);
	decoffset(dof+(50*dfrq));
	dec2offset(dof2a);
        obspwrf(4095.0);
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	dec2pwrf(rfN);

        delay(d1);

	dec2rgpulse(pwN, zero, rof1, rof1);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, rof1, rof1);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(grecov/2);
	dec2rgpulse(pwN, one, rof1, rof1);
	decrgpulse(pwC, one, rof1, rof1);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

   	rgpulse(pw,zero,rof1,rof1);                 /* 1H pulse excitation */

	zgradpulse(gzlvl5, gt5);

  if(SHAPE[A] == 'y')
  {
        dec2pwrf(rfstN);
	delay(lambda - gt5 - POWER_DELAY - 0.5e-3);

	sim3shaped_pulse("","",rna_stNshape,2.0*pw,0.0,1.0e-3,zero,zero,zero,rof1,rof1);
	dec2pwrf(rfN);

	zgradpulse(gzlvl5, gt5);
        delay(lambda - gt5 - POWER_DELAY - 0.5e-3);
   }
  else
   {
	delay(lambda - gt5);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, rof1, rof1);

	zgradpulse(gzlvl5, gt5);
	delay(lambda - gt5);
   }

 	rgpulse(pw, t1, rof1, rof1);

	zgradpulse(-gzlvl3, gt3);
	delay(grecov);
   	dec2rgpulse(pwN, t2, rof1, 0.0);

  if (H1refoc[A] == 'n')
   {
	if ( 0.2*tau1 > (2.0*GRADIENT_DELAY +2.0*pwN/PI +pwC) )
	{
         if (tau1>0.001)
         {
	  zgradpulse(gzlvlr, 0.8*tau1);
          delay(0.2*tau1 -2.0*GRADIENT_DELAY - 2.0*pwN/PI  -pwC );
         }
         else
          delay(tau1 - 2.0*pwN/PI  -pwC );
        decrgpulse(2.0*pwC,zero,0.0,0.0);
         if (tau1>0.001)
         {
	  zgradpulse(-gzlvlr, 0.8*tau1);
          delay(0.2*tau1 -2.0*GRADIENT_DELAY - 2.0*pwN/PI  -pwC );
         }
         else
          delay(tau1 - 2.0*pwN/PI  -pwC );
	}
        else
        {
        delay(2*tau1);
        }
   }
  else
   {
        if (0.2*tau1 > (2.0*GRADIENT_DELAY + rof1 + pwZa + 2.0*pwN/PI ))
        {
         if (tau1>0.001)
         {
          zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - rof1 - pwZa - 2.0*pwN/PI));
          delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwZa - rof1 -2.0*pwN/PI));
         }
         else
          delay(tau1  - pwZa - rof1 -2.0*pwN/PI);
        simpulse(2.0*pw, 2.0*pwC, zero, zero, rof1, rof1);
         if (tau1>0.001)
         {
          zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - rof1 - pwZa - 2.0*pwN/PI));
          delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwZa - rof1 -2.0*pwN/PI));
         }
         else
          delay(tau1  - pwZa - rof1 -2.0*pwN/PI);
        }
        else
	{
        if ( (tau1-rof1)>0.0) delay(tau1-rof1);
        simpulse(2.0*pw, 2.0*pwC, zero, zero, rof1, rof1);
        if ( (tau1-rof1)>0.0) delay(tau1-rof1);
	}
   }

	dec2rgpulse(pwN, t4, 0.0, 0.0);

        zgradpulse(gzlvl3,gt3);
        delay(grecov);

  if (flipback[A] == 'y')
   {
        obspower(tpwrs);
        shaped_pulse("rna_H2Osinc", pwHs, zero, 5.0e-4, rof1);
        obspower(tpwr);
   }

  if( (SHAPE[A] == 'y') && (Naro[A] == 'y') )
   {
	rgpulse(pw, t4, rof1, rof1);
        obspower(tpwrs2);
        obspwrf(finepwrf);
	dec2pwrf(rfstN);

        zgradpulse(gzlvl4,gt4);
	delay(lambda - 3.0*POWER_DELAY - gt4 - rof1 -2.0*GRADIENT_DELAY - pwHs2 - 0.5e-3);

        rgpulse(pwHs2, t3, rof1, rof1);
        obspower(tpwr);
        obspwrf(4095.0);

        sim3shaped_pulse("","",rna_stNshape,2.0*pw,0.0,1.0e-3,zero,zero,zero,rof1,rof1);
        dec2pwrf(rfN);
        obspwrf(finepwrf);
        obspower(tpwrs2);
        rgpulse(pwHs2, t3, rof1, rof1);
        decpower(dpwr);
        dec2power(dpwr2);                                      /* POWER_DELAY */

        zgradpulse(gzlvl4,gt4);
        delay(lambda  - 4.0*POWER_DELAY - gt4 - 2.0*GRADIENT_DELAY - pwHs2 - 0.5e-3);
   }
  else
   {
        rgpulse(pw, t4, rof1, rof1);
        obspower(tpwrs2);
        obspwrf(finepwrf);

        zgradpulse(gzlvl4,gt4);
	delay(lambda - 2.0*POWER_DELAY - gt4 -rof1 -2.0*GRADIENT_DELAY - pwHs2);

	rgpulse(pwHs2, t3, rof1, rof1);
	obspower(tpwr);
        obspwrf(4095.0);
	sim3pulse(2*pw, 0.0, 2*pwN, zero, zero, zero, rof1, rof1);
        obspwrf(finepwrf);
        obspower(tpwrs2);
        rgpulse(pwHs2, t3, rof1, rof1);
	decpower(dpwr);
	dec2power(dpwr2);				       /* POWER_DELAY */

        zgradpulse(gzlvl4,gt4);
        delay(lambda  - 3*POWER_DELAY - gt4 - 2.0*GRADIENT_DELAY - pwHs2);
   }

status(C);
	setreceiver(t11);
}		 
