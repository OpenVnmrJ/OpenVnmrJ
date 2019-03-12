/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghbha_co_nh.c

    3D HBHA(CO)NH gradient sensitivity enhanced version.

    Correlates Hb(i),Ha(i) with N(i+1), NH(i+1).  Uses constant time evolution
    for the HaHb shifts and for the 15N shifts.

    Standard features include maintaining the 13C carrier in the CaCb region
    throughout using off-res SLP pulses; full power square pulses on 13C 
    initially when 13CO excitation is irrelevant; square pulses on the Ca and
    CaCb with first null at 13CO; one lobe sinc pulses on 13CO with first null
    at Ca;  shaka6 composite 180 pulse to simultaneously refocus Ca and invert
    CO; optional 2H decoupling when CaCb magnetization is transverse for 4 
    channel spectrometers.  

    Magic-angle option for coherence transfer gradients.  TROSY option for
    N15/H1 evolution/detection.
 
    pulse sequence: 	Grzesiek and Bax, J Biomol NMR 3 (1993) 185-204.
    SLP pulses:     	J Magn. Reson. 96, 94-102 (1992)
    shaka6 composite: 	Chem. Phys. Lett. 120, 201 (1985)
    TROSY:		 JACS, 120, 10778 (1998)
 
    Written by M.T. @NMRFAM on May 2003 by combining the ghc_co_nh and gcbca_co_nh 
    pulsesequences from BioPack.

        	  CHOICE OF DECOUPLING AND 2D MODES

    	Set dm = 'nnn', dmm = 'ccc' 
    	Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.
	Set dm3 = 'nnn' for no 2H decoupling, or
		  'nyn'  and dmm3 = 'cwc' for 2H decoupling. 
  
    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [13C]  and t2 [15N].

    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give -90, 180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in 13C and f2180='n' for (0,0) in 15N.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.


          	  DETAILED INSTRUCTIONS FOR USE OF ghbha_co_nh


    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for ghbha_co_nh may be printed using:
                                      "printon man('ghbha_co_nh') printoff".
             
    2. Apply the setup macro "ghbha_co_nh".  This loads the relevant parameter
       set and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral
       check.

    3. The two C13 180 degree pulses between points c and d have been
       replaced by a single 6-pulse composite dubbed "shaka6" which povides
       near perfect refocusing with no phase roll over the Cab region and
       near perfect inversion over the CO region 18kHz off-resonance provided
       pwC < 20 us in a 600 MHz system.
      
    4. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 46ppm, and N15 
       frequency on the amide region (120 ppm).  The C13 frequency remains at 
       46ppm, ie at CaCb throughout the sequence.

    5. The H1 frequency is NOT shifted to the amide region during the sequence.
       The H1 DIPSI2 decoupling is controlled by the waltzB1 parameter.

    6. The parameter phi7cal (listed in dg and dg2) is provided to adjust 
       the phase of the last 90 degree C13 pulse, which is phase-shifted by
       the prior 180 degree pulse on the Ca region and by the use of SLP
       pulses on the CO region. The experimentally determined value of this
       phase is also very sensitive to small timing differences (microseconds)
       between the two theta delays. Check this phase via 1D spectra - maximise
       signal, or for greater accuracy obtain the value for a null and then add 
       or subtract 90 degrees.  The value must be a positive number. It needs to
       be calibrated once only for each spectrometer and the gc_co_nh, the gcbca_co_nh 
       and the ghbha_co_nh pulse sequences.

    7. tauCH (1.7 ms) and timeTN (14 ms) were determined for alphalytic protease
       and are listed in dg2 for possible readjustment by the user.

       Cb to Ca magnetization trasfer is determined by timeAB. These are some of 
       the values found for ubiquitin at 800MHz:

	-Halpha gives maximum intensity at timeAB=1.7ms and a null at 3.5ms.
	-Hbeta has a null at 1.9ms and a maximum at 2.8ms

       Thus, to observe only Halphas set timeAB=1.9ms. 
       To maximize Hbetas set timeAB=2.8ms.
       For observing both Halpha and Hbeta a good compromise is timeAB=2.8ms.

       For ni<2, timeAB=1.5ms to avoid cancellation of Halpha and Hbeta signals of 
       opposite sign for calibration purposes. (Marco Tonelli @NMRFAM April 2004)


   8.  The coherence-transfer gradients using power levels
       gzlvl1 and gzlvl2 may be either z or magic-angle gradients. For the
       latter, a proper /vnmr/imaging/gradtable entry must be present and
       syscoil must contain the value of this entry (name of gradtable). The
       amplitude of the gzlvl1 and gzlvl2 should be lower than for a z axis
       probe to have the x and y gradient levels within the 32k range. For
       any value, a dps display (using power display) shows the x,y and z
       dac values. These must be <=32k.

   9. TROSY:
       Set TROSY='y' and dm2='nnn' for a TROSY spectrum of the bottom right
       peak of the 2D coupled NH quartet (high-field H1, low-field N15).  The 
       TROSY spectrum gives 50% S/N compared to the decoupled spectrum for a 
       small peptide.  To select any of the other three peaks of the 2D coupled
       quartet, in a clockwise direction from bottom right, change t4/t10
       from x/y to x/-y to -x/-y to -x/y.  NOTE, the phases of the SE train
       are almost the same as those determined for the gNhsqc sequence.  The
       major difference is that kappa is eliminated compared to normal
       gcbca_co_nh so the N15 magnetization has not evolved with respect to the 
       attached H's.  I.e. the N15 state would be Ix rather than IySz if there 
       was no coherence gradient - this imparts a 90 degree shift so t8 is 
       changed to y (from x in the normal spectrum).  Also gzlvl1 is after the
       180 N15 pulse rather than before as in gNhsqc, so the sign of icosel 
       and the t4/t10 phase2 increments are also swapped compared to gNhsqc.    */



#include <standard.h>
  


static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
                      phx[1]={0},   phy[1]={1},

             phi5[2]  = {0,2},
             phi6[2]  = {2,0},  
             phi9[8]  = {0,0,1,1,2,2,3,3},
             rec[4]   = {0,2,2,0},		     recT[2]  = {3,1};

static double   d2_init=0.0, d3_init=0.0;



void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
 	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */

 
int         icosel,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni = getval("ni"),
	    ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
            t1a,                       /* time increments for first dimension */
            t1b,
            t1c,
	    tauCH = getval("tauCH"), 		         /* 1/4J delay for CH */
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
            timeAB = getval("timeAB"),	   /* set timeAB=1.9e-3 to get only Ha */
            				  /* set timeAB=3.3e-3 to maximize Hb */
            				  /* set timeAB=2.8e-3 for both Ha/Hb */
	    zeta = 3.0e-3,
	    eta = 4.6e-3,
	    theta = 14.0e-3,
	    kappa = 5.4e-3,
	    lambda = 2.4e-3,
            sheila,  /* to transfer J evolution time hyperbolically into tau1 */
            
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	rf0,            	  /* maximum fine power when using pwC pulses */

/* 90 degree pulse at Cab(46ppm), first off-resonance null at CO (174ppm)     */
        pwC1,		              /* 90 degree pulse length on C13 at rf1 */
        rf1,		       /* fine power for 5.1 kHz rf for 600MHz magnet */

/* 180 degree pulse at Cab(46ppm), first off-resonance null at CO(174ppm)     */
        pwC2,		                    /* 180 degree pulse length at rf2 */
        rf2,		      /* fine power for 11.4 kHz rf for 600MHz magnet */

/* the following pulse lengths for SLP pulses are automatically calculated    */
/* by the macro "proteincal".  SLP pulse shapes, "offC4" etc are called       */
/* directly from your shapelib.                    			      */
   pwC4 = getval("pwC4"),  /*180 degree pulse at Ca(56ppm) null at CO(174ppm) */
   pwC5 = getval("pwC5"),     /* 90 degree selective sinc pulse on CO(174ppm) */
   pwC7 = getval("pwC7"),    /* 180 degree selective sinc pulse on CO(174ppm) */
   rf4,	                           /* fine power for the pwC4 ("offC4") pulse */
   rf5,	                           /* fine power for the pwC5 ("offC5") pulse */
   rf7,	                           /* fine power for the pwC7 ("offC7") pulse */

   compH = getval("compH"),       /* adjustment for C13 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */
   phi7cal = getval("phi7cal"),  /* phase in degrees of the last C13 90 pulse */

   	pwH,	    		        /* H1 90 degree pulse length at tpwr1 */
   	tpwr1,	  	                            /* 7.3 kHz rf for DIPSI-2 */
   	DIPSI2time,     	        /* total length of DIPSI-2 decoupling */
        ncyc_dec,
        waltzB1=getval("waltzB1"),     /* RF strength for 1H decoupling      */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
        gzcal  = getval("gzcal"),            /* g/cm to DAC conversion factor */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gstab = getval("gstab"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("mag_flg",mag_flg);
    getstr("TROSY",TROSY);



/*   LOAD PHASE TABLE    */

	settable(t3,1,phx);
	settable(t4,1,phx);
	settable(t5,2,phi5);
	settable(t6,2,phi6);
   if (TROSY[A]=='y')
       {settable(t8,1,phy);
	settable(t9,1,phx);
 	settable(t10,1,phy);
	settable(t11,1,phx);
	settable(t12,2,recT);}
    else
       {settable(t8,1,phx);
	settable(t9,8,phi9);
	settable(t10,1,phx);
	settable(t11,1,phy);
	settable(t12,4,rec);}




/*   INITIALIZE VARIABLES   */

    if( dpwrf < 4095 )
	{ printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
	  psg_abort(1); }

    if( pwC > 24.0e-6*600.0/sfrq )
	{ printf("increase pwClvl so that pwC < 24*600/sfrq");
	  psg_abort(1); }

    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;

    /* 90 degree pulse on Cab, null at CO 128ppm away */
	pwC1 = sqrt(15.0)/(4.0*128.0*dfrq);
        rf1 = (compC*4095.0*pwC)/pwC1;
	rf1 = (int) (rf1 + 0.5);
	
    /* 180 degree pulse on Cab, null at CO 128ppm away */
        pwC2 = sqrt(3.0)/(2.0*128.0*dfrq);
	rf2 = (4095.0*compC*pwC*2.0)/pwC2;
	rf2 = (int) (rf2 + 0.5);	
	
    /* 180 degree pulse on Ca, null at CO 118ppm away */
	rf4 = (compC*4095.0*pwC*2.0)/pwC4;
	rf4 = (int) (rf4 + 0.5);

    /* 90 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */
	rf5 = (compC*4095.0*pwC*1.69)/pwC5;	/* needs 1.69 times more     */
	rf5 = (int) (rf5 + 0.5);		/* power than a square pulse */

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */
	rf7 = (compC*4095.0*pwC*2.0*1.65)/pwC7;	/* needs 1.65 times more     */
	rf7 = (int) (rf7 + 0.5);		/* power than a square pulse */
	

    /* power level and pulse times for DIPSI 1H decoupling */
	DIPSI2time = 2.0*3.0e-3 + 2.0*14.0e-3 + 2.0*timeTN - 5.4e-3 + 0.5*pwC1 + 2.0*pwC5 + 5.0*pwN + 2.0*gt3 + 1.0e-4 + 4.0*GRADIENT_DELAY + 2.0*POWER_DELAY + 8.0*PRG_START_DELAY;
        pwH = 1.0/(4.0*waltzB1);
	ncyc_dec = (DIPSI2time*90.0)/(pwH*2590.0*4.0);
        ncyc_dec = (int) (ncyc_dec+0.5);
	pwH = (DIPSI2time*90.0)/(ncyc_dec*2590.0*4.0); /*fine correction of pwH */
	tpwr1 = 4095.0*(compH*pw/pwH);
	tpwr1 = (int) (2.0*tpwr1 + 0.5);   /* x2 because obs atten will be reduced by 6dB */
 


if (ix == 1)
      {
        fprintf(stdout, "\nNo of DIPSI-2 cycles = %4.1f\n",ncyc_dec);
        fprintf(stdout, "\nfine power for DIPSI-2 pulse =%6.1f\n",tpwr1);
      }


/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( 0.5*ni2*1/(sw2) > timeTN - WFG3_START_DELAY)
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n", 
  	 ((int)((timeTN - WFG3_START_DELAY)*2.0*sw2))); psg_abort(1);}

    if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
       { printf("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' )
       { printf("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dm3[A] == 'y' || dm3[C] == 'y' )
       { printf("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' ");
							             psg_abort(1);}
    if ( dpwr2 > 50 )
       { printf("dpwr2 too large! recheck value  "); psg_abort(1);}

    if ( pw > 20.0e-6 )
       { printf(" pw too long ! recheck value "); psg_abort(1);} 
  
    if ( pwN > 100.0e-6 )
       { printf(" pwN too long! recheck value "); psg_abort(1);} 
 
    if ( TROSY[A]=='y' && dm2[C] == 'y' )
       { text_error("Choose either TROSY='n' or dm2='n' ! "); psg_abort(1);}



/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)   tsadd(t3,1,4);  
    if (TROSY[A]=='y')
	 {  if (phase2 == 2)   				      icosel = +1;
            else 	    {tsadd(t4,2,4);  tsadd(t10,2,4);  icosel = -1;}
	 }
    else {  if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;}
            else 			       icosel = -1;    
	 }



/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;



/*  Hyperbolic sheila seems superior to original zeta approach  */

                                  /* subtract unavoidable delays from tauCH */
    tauCH = tauCH - gt0 - 2.0*GRADIENT_DELAY - 5.0e-5;

 if ((ni-1)/(2.0*sw1) > 2.0*tauCH)
    {
      if (tau1 > 2.0*tauCH) sheila = tauCH;
      else if (tau1 > 0) sheila = 1.0/(1.0/tau1+1.0/tauCH-1.0/(2.0*tauCH));
      else          sheila = 0.0;
    }
 else
    {
      if (tau1 > 0) sheila = 1.0/(1.0/tau1 + 1.0/tauCH - 2.0*sw1/((double)(ni-1)));
      else          sheila = 0.0;
    }
    t1a = tau1 + tauCH;
    t1b = tau1 - sheila;
    t1c = tauCH - sheila;



/*  Set up f2180  */

    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 1.0)) 
	{ tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2/2.0;



/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t12,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t8,2,4); tsadd(t12,2,4); }


/* For ni<2 (calibration) set timeAB=1.5ms to get avoid signal cancellation between Ha and Hb */
   if (ni < 2.0) timeAB=1.5e-3;



/* BEGIN PULSE SEQUENCE */


status(A);
        delay(d1);
        if ( dm3[B] == 'y' )
           lk_sample();  /*freezes z0 correction, stops lock pulsing*/

        if ((ni/sw1-d2)>0)
         delay(ni/sw1-d2);       /*decreases as t1 increases for const.heating*/
        if ((ni2/sw2-d3)>0)
         delay(ni2/sw2-d3);      /*decreases as t2 increases for const.heating*/
        if ( dm3[B] == 'y' )
          { lk_hold(); lk_sampling_off();}  /*freezes z0 correction, stops lock pulsing*/

        rcvroff();
        obspower(tpwr);
        decpower(pwClvl);
        dec2power(pwNlvl);
        decpwrf(rf0);
        obsoffset(tof);
        txphase(one);
        delay(1.0e-5);
        if (TROSY[A] == 'n')
        dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
        decrgpulse(pwC, zero, 0.0, 0.0);
        zgradpulse(gzlvl0, 0.5e-3);
        delay(1.0e-4);
        if (TROSY[A] == 'n')
        dec2rgpulse(pwN, one, 0.0, 0.0);
        decrgpulse(pwC, zero, 0.0, 0.0);
        zgradpulse(0.7*gzlvl0, 0.5e-3);
        delay(5.0e-4);

      if ( dm3[B] == 'y' )     /* begins optional 2H decoupling */
        {
          gzlvl0=0.0; gzlvl3=0.0; gzlvl4=0.0;   /* no gradients during 2H decoupling */
          dec3rgpulse(1/dmf3,one,10.0e-6,2.0e-6);
          dec3unblank();
          dec3phase(zero);
          delay(2.0e-6);
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
        }

        rgpulse(pw, one, 0.0, 0.0);                    /* 1H pulse excitation */
                                                                /* point a */
        txphase(zero);
        decphase(zero);
        zgradpulse(gzlvl0, gt0);                        /* 2.0*GRADIENT_DELAY */
        delay(5.0e-5);
        if((t1a -2.0*pwC) > 0.0) delay(t1a - 2.0*pwC);

        decrgpulse(2.0*pwC, zero, 0.0, 0.0);

        delay(t1b);

        rgpulse(2.0*pw, zero, 0.0, 0.0);

        zgradpulse(gzlvl0, gt0);                        /* 2.0*GRADIENT_DELAY */
        txphase(t3);
        delay(5.0e-5);
        delay(t1c);
                                                                /* point b */
        rgpulse(pw, t3, 0.0, 0.0);
        zgradpulse(gzlvl3, gt3);
        delay(2.0e-4);
        decrgpulse(pwC, zero, 0.0, 0.0);
                                                                /* point c */
        zgradpulse(gzlvl4, gt4);
        decpwrf(rf2);
        delay(timeAB - gt4);

        simpulse(2*pw, pwC2, zero, zero, 0.0, 0.0);

        zgradpulse(gzlvl4, gt4);
        delay(timeAB - gt4);
                                                                /* point d */

/* ghc_co_nh STOPS HERE */
/* gcbca_co_nh STARTS HERE */


	decpwrf(rf1);                                         	/* point b */
   	decrgpulse(pwC1, zero, 2.0e-6, 0.0);
	obspwrf(tpwr1); obspower(tpwr-6);				      /* POWER_DELAY */
	obsprgon("dipsi2", pwH, 5.0);		          /* PRG_START_DELAY */
	xmtron();
                    						/* point c */
	decpwrf(rf0);
	decphase(t5);
	delay(zeta - 2.0*POWER_DELAY - PRG_START_DELAY - 0.5*10.933*pwC);

	decrgpulse(pwC*158.0/90.0, t5, 0.0, 0.0);
	decrgpulse(pwC*171.2/90.0, t6, 0.0, 0.0);
	decrgpulse(pwC*342.8/90.0, t5, 0.0, 0.0);	/* Shaka composite   */
	decrgpulse(pwC*145.5/90.0, t6, 0.0, 0.0);
	decrgpulse(pwC*81.2/90.0, t5, 0.0, 0.0);
	decrgpulse(pwC*85.3/90.0, t6, 0.0, 0.0);

	decpwrf(rf1);
	decphase(zero);
	delay(zeta - 0.5*10.933*pwC - 0.5*pwC1);
                     						/* point d */
   	decrgpulse(pwC1, zero, 0.0, 0.0);
	decphase(t5);
	decpwrf(rf5);
        if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
           {
           setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
           dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
           dec3blank();
           lk_autotrig();   /* resumes lock pulsing */
           }
	zgradpulse(gzlvl3, gt3);
	delay(2.0e-4);
	decshaped_pulse("offC5", pwC5, t5, 0.0, 0.0);
					      			/* point e */
	decpwrf(rf4);
 	decphase(zero);
	delay(eta);

	decshaped_pulse("offC4", pwC4, zero, 0.0, 0.0);

	decpwrf(rf7);		
	dec2phase(zero);
	delay(theta - eta - pwC4 - WFG3_START_DELAY);
							 /* WFG3_START_DELAY */
	sim3shaped_pulse("", "offC7", "", 0.0, pwC7, 2.0*pwN, zero, zero, zero, 
								     0.0, 0.0);

	decpwrf(rf5);
	decpwrf(rf5);
	initval(phi7cal, v7);
	decstepsize(1.0);
	dcplrphase(v7);					       /* SAPS_DELAY */
	dec2phase(t8);
	delay(theta - SAPS_DELAY);
                           					/* point f */
	decshaped_pulse("offC5", pwC5, zero, 0.0, 0.0);

/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */

	zgradpulse(gzlvl3, gt3);
        if (TROSY[A]=='y') { xmtroff(); obsprgoff(); }
     	delay(2.0e-4);
	dcplrphase(zero);
	dec2rgpulse(pwN, t8, 0.0, 0.0);

	decpwrf(rf7);
	decphase(zero);
	dec2phase(t9);
	delay(timeTN - WFG3_START_DELAY - tau2);
							 /* WFG3_START_DELAY  */
	sim3shaped_pulse("", "offC7", "", 0.0, pwC7, 2.0*pwN, zero, zero, t9, 
								    0.0, 0.0);

	dec2phase(t10);
        decpwrf(rf4);

if (TROSY[A]=='y')
{
    if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
	{
	  txphase(t4);
          delay(timeTN - pwC4 - WFG_START_DELAY);          /* WFG_START_DELAY */
          decshaped_pulse("offC4", pwC4, zero, 0.0, 0.0);
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')  magradpulse(gzcal*gzlvl1, gt1);
          else  zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);
	}
    else
	{
	  txphase(t4);
          delay(timeTN -pwC4 -WFG_START_DELAY -gt1 -2.0*GRADIENT_DELAY -1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else  zgradpulse(gzlvl1, gt1);	   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);                    /* WFG_START_DELAY */
          decshaped_pulse("offC4", pwC4, zero, 0.0, 0.0);
          delay(tau2);
	}
}
else
{
    if (tau2 > kappa)
	{
          delay(timeTN - pwC4 - WFG_START_DELAY);     	   /* WFG_START_DELAY */
          decshaped_pulse("offC4", pwC4, zero, 0.0, 0.0);
          delay(tau2 - kappa - PRG_STOP_DELAY);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  txphase(t4);
          delay(kappa - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);
	}
    else if (tau2 > (kappa - pwC4 - WFG_START_DELAY))
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  txphase(t4);                                  /* WFG_START_DELAY */
          decshaped_pulse("offC4", pwC4, zero, 0.0, 0.0);
          delay(kappa -pwC4 -WFG_START_DELAY -gt1 -2.0*GRADIENT_DELAY -1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);
	}
    else if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  txphase(t4);
          delay(kappa - tau2 - pwC4 - WFG_START_DELAY);   /* WFG_START_DELAY */
          decshaped_pulse("offC4", pwC4, zero, 0.0, 0.0);
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);
	}
    else
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY);
          xmtroff();
	  obsprgoff();					    /* PRG_STOP_DELAY */
	  txphase(t4);
    	  delay(kappa-tau2-pwC4-WFG_START_DELAY-gt1-2.0*GRADIENT_DELAY-1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);                    /* WFG_START_DELAY */
          decshaped_pulse("offC4", pwC4, zero, 0.0, 0.0);
          delay(tau2);
	}
}                                                            	/* point g */
/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
	if (TROSY[A]=='y')  rgpulse(pw, t4, 0.0, 0.0);
	else                sim3pulse(pw, 0.0, pwN, t4, zero, t10, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	if (TROSY[A]=='y')   delay(lambda - 0.65*(pw + pwN) - gt5);
	else   delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	dec2phase(t11);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(pw, 0.0, pwN, one, zero, t11, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl6, gt5);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	dec2phase(t10);
	zgradpulse(gzlvl6, gt5);
	if (TROSY[A]=='y')   delay(lambda - 1.6*pwN - gt5);
	else   delay(lambda - 0.65*pwN - gt5);

	if (TROSY[A]=='y')   dec2rgpulse(pwN, t10, 0.0, 0.0); 
	else    	     rgpulse(pw, zero, 0.0, 0.0); 

	delay((gt1/10.0) + 1.0e-4 +gstab - 0.5*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

	rgpulse(2.0*pw, zero, 0.0, rof1);
	dec2power(dpwr2);				       /* POWER_DELAY */
        if (mag_flg[A] == 'y')    magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);
        else   zgradpulse(icosel*gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */
        delay(gstab);
        rcvron();
statusdelay(C,1.0e-4 - rof1);
   if (dm3[B]=='y') lk_sample();

	setreceiver(t12);
}		 


