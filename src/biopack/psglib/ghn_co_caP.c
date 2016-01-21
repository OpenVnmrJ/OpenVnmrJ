/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghn_co_caP.c

    3D HNCOCA gradient sensitivity enhanced version.


    Correlates Ca(i) with N(i+1), NH(i+1).  Uses constant time evolution for the
    15N shifts.

    Standard features include square pulses on Cab with first null at 13CO; one
    lobe sinc pulses on 13CO with first null at Ca; shaka6 composite 180 pulse
    to simultaneously refocus CO and invert Ca; one lobe sinc pulse to put H2O
    back along z (the sinc one-lobe is significantly more selective than
    gaussian, square, or seduce 90 pulses); optional 2H decoupling when CaCb 
    magnetization is transverse for 4 channel spectrometers.  

    Magic-angle option for coherence transfer gradients.  TROSY option for
    N15/H1 evolution/detection.
 
    pulse sequence:
          Yamazaki, Lee, Arrowsmith, Muhandiram, Kay, JACS, 116, 11655 (1994)
    SLP pulses:     J Magn. Reson. 96, 94-102 (1992)
    shaka6 composite: Chem. Phys. Lett. 120, 201 (1985)
    TROSY:	    Weigelt, JACS, 120, 10778 (1998)
 
    Based on hncoca_D_sel_pfg_500.c written by LEK et al.

    Revised and improved to a standard format by MRB, BKJ and GG for the 
    BioPack, January 1997, so as to include calculation of the above 
    standard features within the pulse sequence code and associated macro.
    TROSY added Dec 98, based on similar addition to gNhsqc. Shaped pulses 
    calculated within pulse sequence code, Jan 99 (Version March 99).


        	  CHOICE OF DECOUPLING AND 2D MODES

    	Set dm = 'nnn', dmm = 'ccc' 
    	Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.
	Set dm3 = 'nnn' for no 2H decoupling, or
		  'nyn'  and dmm3 = 'cwc' for 2H decoupling. 
  
    Made the waltz16 field strength enterable (waltzB1) in Hz.  (GG jan03)

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [13C]  and t2 [15N].
    
    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in 13C and f2180='n' for (0,0) in 15N.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.



          	  DETAILED INSTRUCTIONS FOR USE OF ghn_co_caP


    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for ghn_co_caP may be printed using:
                                      "printon man('ghn_co_caP') printoff".

    2. Apply the setup macro "ghn_co_caP". This loads the relevant parameter set
       and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral check.
       At the middle of the t1 period, the 180 degree pulses on CO and 15N are
       swapped to a 180 degree pulse on Ca, for the first increment of t1, to
       refocus Ca chemical-shift evolution ensuring a zero first-order phase
       correction in F1. This is also the case for the 1D spectral check, or
       for 2D/15N spectra, when ni=0.

    3. Center H1 frequency on H2O (4.7ppm), C13 on 174ppm, and N15 frequency
       on the amide region (120ppm). The C13 frequency is shifted to the Ca
       region during the sequence, but phase coherence is not required after
       each shift.

    4. The normal 13C 180 degree pulse on CO at the middle of t1 induces a
       phase shift, which should be field-invariant, and so this phase shift has
       been calibrated and compensated in the pulse sequence. This phase shift
       can be checked by setting ni=1 whereby a special 1D method is invoked
       in which both the 13C CO 180 degree pulse and the simultaneous 15N 180
       degree pulse are applied just as for all t1 times other than t1=0.  First
       eliminate the CO pulse with (ni=1 pwS=0) and obtain a 1D spectrum. This
       spectrum will have reduced intensity compared to ni=0 because of 13Ca
       chemical-shift evolution during the time of the 180 pulses. If the
       phase shift is adequately compensated, a second very similar 1D spectrum
       will be obtained with pwS=180.  As described in more detail for  
       ghn_co, a more sensitive comparison of the two spectra with pwS=0,180 
       can be obtained with phase=2.  If not adequately compensated, the
       first increment will be out of phase with all succeeding increments and a
       zero-order phase-shift will be necessary in F1, which is easily done
       after the 2D/3D transform. Alternatively, change the calibration by
       changing the phshift parameter in the INITIALIZE VARIABLES section of
       the code.  NOTE THAT dof MUST BE ACCURATELY CALIBRATED (to 1ppm) BEFORE 
       THE phshift CALIBRATION. S/N can also be maximized by arraying compC
       when (ni=1 pwS=180).

    5. H2O preservation is achieved according to Kay et al, except that a sinc
       one-lobe selective pulse is used to put H2O back along z.  This is much
       more selective than a hard, Seduce1, or gaussian pulse.  WALTZ16 1H
       decoupling strength is controlled by the parameter waltzB1, specified in the 
       new shapelib file after go or dps. 

    6. Another difference from the work of Kay et al is that the phases of the
       first and last Ca 90 degree pulses are alternated to eliminate artifacts
       from the CO 180 degree pulse.

    7. tauC (4.5 ms) and timeTN (12.5 ms) were determined for alphalytic 
       protease and are listed in dg2 for possible readjustment by the user.

    8. TROSY:
       Set TROSY='y' and dm2='nnn' for a TROSY spectrum of the bottom right
       peak of the 2D coupled NH quartet (high-field H1, low-field N15).  The 
       TROSY spectrum gives 50% S/N compared to the decoupled spectrum for a 
       small peptide.  To select any of the other three peaks of the 2D coupled
       quartet, in a clockwise direction from bottom right, change t4/t10
       from x/y to x/-y to -x/-y to -x/y.  NOTE, the phases of the SE train
       are almost the same as those determined for the gNhsqc sequence.  The
       major difference is that kappa is eliminated compared to normal
       ghn_co_caP so the N15 magnetization has not evolved with respect to the 
       attached H's.  I.e. the N15 state would be Ix rather than IySz if there 
       was no coherence gradient - this imparts a 90 degree shift so t8 is 
       changed to y (from x in the normal spectrum).  Also gzlvl1 is after the
       180 N15 pulse rather than before as in gNhsqc, so the sign of icosel 
       and the t4/t10 phase2 increments are also swapped compared to gNhsqc.
       For ghn_c... type sequences, H1 decoupling during the first timeTN is
       replaced by a pi pulse at kappa/2 to reduce S/N loss for large molecules
       during the first TN period.  For these sequences H2O flipback is 
       achieved with two sinc one-lobe pulses, an additional one just before
       the SE train, similar to gNhsqc.

  12.  The N15 t2 evolution and the sensitivity enhancement train is common
       to all ghn_... sequences and the pulse sequence code for these final
       sections is in the include file, bionmr.h.
*/


#include <standard.h>
#include "bionmr.h"
  
static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
             phx[1]={0},   phy[1]={1},

	     phi3[2]  = {2,0},
	     phi5[4]  = {0,0,2,2},
             phi9[8]  = {0,0,0,0,2,2,2,2},
             rec[4]   = {0,2,2,0},		     recT[4]  = {3,1,1,3};



pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
 	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */
 
int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni2 = getval("ni2");

double      d2_init=0.0,  		        /* used for states tppi in t1 */
	    d3_init=0.0,  	 	        /* used for states tppi in t2 */
	    tau1,         				         /*  t1 delay */
	    tauC = getval("tauC"), 	      /* delay for CO to Ca evolution */
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
            
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */

   pwS1,					 /* length of square 90 on Ca */
   phshift,        /*  phase shift induced on Ca by 180 on CO in middle of t1 */
   pwS2,					       /* length of 180 on CO */
   pwS = getval("pwS"), /* used to change 180 on CO in t1 for 1D calibrations */
   pwZ,					   /* the largest of pwS2 and 2.0*pwN */
   pwZ1,	        /* the largest of pwS2 and 2.0*pwN for 1D experiments */

        waltzB1 = getval("waltzB1"),  /* waltz16 field strength (in Hz)     */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt7 = getval("gt7"),
	gt9 = getval("gt9"),
	gt10 = getval("gt10"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl7 = getval("gzlvl7"),
	gzlvl8 = getval("gzlvl8"),
	gzlvl9 = getval("gzlvl9"),
	gzlvl10 = getval("gzlvl10");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("TROSY",TROSY);


/*   LOAD PHASE TABLE    */
     
	settable(t3,2,phi3);
	settable(t4,1,phx);
	settable(t5,4,phi5);
   if (TROSY[A]=='y')
       {settable(t8,1,phy);
	settable(t9,1,phx);
 	settable(t10,1,phy);
	settable(t11,1,phx);
	settable(t12,4,recT);}
    else
       {settable(t8,1,phx);
	settable(t9,8,phi9);
	settable(t10,1,phx);
	settable(t11,1,phy);
	settable(t12,4,rec);}


/*   INITIALIZE VARIABLES   */

 	kappa = 5.4e-3;
	lambda = 2.4e-3;

   pwHs = 1.7e-3*500.0/sfrq;       /* length of H2O flipback, 1.7ms at 500 MHz*/
   widthHd = 2.681*(waltzB1/sfrq);   /* bandwidth of H1 WALTZ16 decoupling in ppm */
   pwHd = h1dec90pw("WALTZ16", widthHd, 0.0);     /* H1 90 length for WALTZ16 */

    /* get calculated pulse lengths of shaped C13 pulses */
	pwS1 = c13pulsepw("ca", "co", "square", 90.0); 
	pwS2 = c13pulsepw("co", "ca", "sinc", 180.0); 

    /* the 180 pulse on CO at the middle of t1 */
	if ((ni2 > 0.0) && (ni == 1.0)) ni = 0.0;
        if (pwS2 > 2.0*pwN) pwZ = pwS2; else pwZ = 2.0*pwN;
        if ((pwS==0.0) && (pwS2>2.0*pwN)) pwZ1=pwS2-2.0*pwN; else pwZ1=0.0;
	if ( ni > 1 )     pwS = 180.0;
	if ( pwS > 0 )   phshift = 140.0;
	else             phshift = 0.0;




/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( tauC < (gt7+1.0e-4+0.5*10.933*pwC))  gt7=(tauC-1.0e-4-0.5*10.933*pwC);

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
 
    if ( TROSY[A]=='y' && dm2[C] == 'y')
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



/* BEGIN PULSE SEQUENCE */

status(A);
   	delay(d1);
        if ( dm3[B] == 'y' )
          { lk_hold(); lk_sampling_off();}  /*freezes z0 correction, stops lock pulsing*/
 
	rcvroff();
        set_c13offset("co");
	obsoffset(tof);
	obspower(tpwr);
 	obspwrf(4095.0);
	decpower(pwClvl);
	decpwrf(4095.0);
 	dec2power(pwNlvl);
	txphase(zero);
   	delay(1.0e-5);

       if (TROSY[A] == 'n')
	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(-gzlvl0, 0.5e-3);
	delay(1.0e-4);
       if (TROSY[A] == 'n')
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(-0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

  	rgpulse(pw, zero, 0.0, 0.0);                   /* 1H pulse excitation */

   	dec2phase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	txphase(one);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

 	rgpulse(pw, one, 0.0, 0.0);

if (TROSY[A]=='y')
   {txphase(two);
    shiftedpulse("sinc", pwHs, 90.0, 0.0, two, 2.0e-6, 0.0);
    zgradpulse(gzlvl3, gt3);
    delay(2.0e-4);
    dec2rgpulse(pwN, zero, 0.0, 0.0);

    delay(0.5*kappa - 2.0*pw);

    rgpulse(2.0*pw, two, 0.0, 0.0);

    decphase(zero);
    dec2phase(zero);
    delay(timeTN - 0.5*kappa - WFG3_START_DELAY);
   }
else
   {txphase(zero);
    shiftedpulse("sinc", pwHs, 90.0, 0.0, zero, 2.0e-6, 0.0);
    zgradpulse(gzlvl3, gt3);
    delay(2.0e-4);
    dec2rgpulse(pwN, zero, 0.0, 0.0);

    delay(kappa - POWER_DELAY - PWRF_DELAY - pwHd - 4.0e-6 - PRG_START_DELAY);
					   /* delays for h1waltzon subtracted */
    h1waltzon("WALTZ16", widthHd, 0.0);
    decphase(zero);
    dec2phase(zero);
    delay(timeTN - kappa - WFG3_START_DELAY);
   }
							  /* WFG3_START_DELAY */
	sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
					     zero, zero, zero, 2.0e-6, 2.0e-6);
	decphase(zero);
	delay(timeTN);

	dec2rgpulse(pwN, zero, 0.0, 0.0);
	if (TROSY[A]=='n')   h1waltzoff("WALTZ16", widthHd, 0.0);
	zgradpulse(-gzlvl3, gt3);
 	delay(2.0e-4);
	c13pulse("co", "ca", "sinc", 90.0, zero, 2.0e-6, 0.0);

	zgradpulse(-gzlvl7, gt7);
	decphase(zero);
	delay(tauC - gt7 - 0.5*10.933*pwC);

	decrgpulse(pwC*158.0/90.0, zero, 0.0, 0.0);
	decrgpulse(pwC*171.2/90.0, two, 0.0, 0.0);
	decrgpulse(pwC*342.8/90.0, zero, 0.0, 0.0);      /* Shaka 6 composite */
	decrgpulse(pwC*145.5/90.0, two, 0.0, 0.0);
	decrgpulse(pwC*81.2/90.0, zero, 0.0, 0.0);
	decrgpulse(pwC*85.3/90.0, two, 0.0, 0.0);

	zgradpulse(-gzlvl7, gt7);
	decphase(one);
        delay(tauC - gt7 - 0.5*10.933*pwC - WFG_START_DELAY - 2.0e-6);
							   /* WFG_START_DELAY */
	c13pulse("co", "ca", "sinc", 90.0, one, 2.0e-6, 0.0);
        set_c13offset("ca");
	zgradpulse(-gzlvl9, gt9);
	decphase(t3);
	delay(2.0e-4);
      if ( dm3[B] == 'y' )     /* begins optional 2H decoupling */
        {
          dec3rgpulse(1/dmf3,one,10.0e-6,2.0e-6);
          dec3unblank();
          dec3phase(zero);
          delay(2.0e-6);
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
        }
        h1waltzon("WALTZ16", widthHd, 0.0);

/*   xxxxxxxxxxxxxxxxxxxxxx       13Ca EVOLUTION       xxxxxxxxxxxxxxxxxx    */

	c13pulse("ca", "co", "square", 90.0, t3, 2.0e-6, 0.0);      /*  pwS1  */
	decphase(zero);

if ((ni>1.0) && (tau1>0.0))          /* total 13C evolution equals d2 exactly */
   {           /* 2.0*pwS1/PI compensates for evolution at 64% rate during 90 */
     if (tau1 - 2.0*pwS1/PI - WFG3_START_DELAY - 0.5*pwZ - 2.0e-6
			 	- 2.0*PWRF_DELAY - 2.0*POWER_DELAY > 0.0)
	   {
	delay(tau1 - 2.0*pwS1/PI - WFG3_START_DELAY - 0.5*pwZ - 2.0e-6 
					 - 2.0*PWRF_DELAY - 2.0*POWER_DELAY);
							  /* WFG3_START_DELAY */
	sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
					     zero, zero, zero, 2.0e-6, 0.0);
	initval(phshift, v3);
	decstepsize(1.0);
	dcplrphase(v3);  				        /* SAPS_DELAY */
	delay(tau1 - 2.0*pwS1/PI  - SAPS_DELAY - 0.5*pwZ - WFG_START_DELAY 
				- 2.0e-6 - 2.0*PWRF_DELAY - 2.0*POWER_DELAY);
	   }
      else
	   {
	initval(180.0, v3);
	decstepsize(1.0);
	dcplrphase(v3);  				        /* SAPS_DELAY */
	delay(2.0*tau1 - 4.0*pwS1/PI - SAPS_DELAY - WFG_START_DELAY - 2.0e-6 
					 	- PWRF_DELAY - POWER_DELAY);
	   }
   }

else if (ni==1.0)            /* special 1D check of phshift enabled when ni=1 */
   {
	delay(10.0e-6 + SAPS_DELAY + 0.5*pwZ1 + WFG_START_DELAY);
							  /* WFG3_START_DELAY */
	sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, pwS, 2.0*pwN,
					        zero, zero, zero, 2.0e-6, 0.0);
	initval(phshift, v3);
	decstepsize(1.0);
	dcplrphase(v3);  					/* SAPS_DELAY */
	delay(10.0e-6 + WFG3_START_DELAY + 0.5*pwZ1);
   }

else            /* 13Ca evolution refocused for 1st increment, or when ni=0   */
   {
	delay(10.0e-6);					   /* WFG_START_DELAY */
	c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);
	delay(10.0e-6);
   }
        decphase(t5);
	c13pulse("ca", "co", "square", 90.0, t5, 2.0e-6, 0.0);      /*  pwS1  */

/*   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    */

	h1waltzoff("WALTZ16", widthHd, 0.0);
        set_c13offset("co");
	decphase(one);
        if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
           {
           setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
           dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
           dec3blank();
           lk_autotrig();   /* resumes lock pulsing */
           }
	zgradpulse(gzlvl10, gt10);
 	delay(2.0e-4);
	c13pulse("co", "ca", "sinc", 90.0, one, 2.0e-6, 0.0);

	zgradpulse(gzlvl8, gt7);
	decphase(zero);
	delay(tauC - gt7 - 0.5*10.933*pwC);

	decrgpulse(pwC*158.0/90.0, zero, 0.0, 0.0);
	decrgpulse(pwC*171.2/90.0, two, 0.0, 0.0);
	decrgpulse(pwC*342.8/90.0, zero, 0.0, 0.0);	/* Shaka 6 composite */
	decrgpulse(pwC*145.5/90.0, two, 0.0, 0.0);
	decrgpulse(pwC*81.2/90.0, zero, 0.0, 0.0);
	decrgpulse(pwC*85.3/90.0, two, 0.0, 0.0);

	zgradpulse(gzlvl8, gt7);
	decphase(zero);
	delay(tauC - gt7 - 0.5*10.933*pwC - WFG_START_DELAY - 2.0e-6);
							   /* WFG_START_DELAY */
	c13pulse("co", "ca", "sinc", 90.0, zero, 2.0e-6, 0.0);


/*  xxxxxxxxxxxxxxxxxxxx  N15 EVOLUTION & SE TRAIN   xxxxxxxxxxxxxxxxxxxxxxx  */	
	hn_evol_se_train("co", "ca"); /* common part of sequence in bionmr.h  */
        if (dm3[B] == 'y') lk_sample();

}
