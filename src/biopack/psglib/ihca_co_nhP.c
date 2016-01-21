/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ihca_co_nhP.c               

    3D Intraresidual (HCA)CONH gradient sensitivity enhanced version.

    Standard features include maintaining the 13C carrier in the CaCb region
    throughout using off-res SLP pulses; full power square pulses on 13C 
    initially when 13CO excitation is irrelevant; square pulses on the Ca and
    CaCb with first null at 13CO; one lobe sinc pulses on 13CO with first null
    at Ca;  shaka6 composite 180 pulse to simultaneously refocus Ca and invert
    CO; optional 2H decoupling when CaCb magnetization is transverse and during 
    1H shift evolution for 4 channel spectrometers.  
 
    Magic-angle option for coherence transfer gradients.  TROSY option for
    N15/H1 evolution/detection.
 
    SLP pulses:     	 J Magn. Reson. 96, 94-102 (1992) 
    TROSY:		 JACS, 120, 10778 (1998)

    ( comments such as "point g" refer to pulse sequence diagram in reference)

    Written by M Robin Bendall, Varian, Jan 97 from ghc_co_nh.c written in
    March 94 and 95.  Revised and improved to a standard format by MRB, BKJ and
    GG for the ProteinPack, January 1997. TROSY added Dec 98, based on similar 
    addition to gNhsqc. Shaped pulses calculated within pulse sequence code,
    Jan 99 (Version March 99).

    Modified for intraresidue experiment from ghc_co_nhP.  Contributed by
    Perttu Permi, U.Helsinki. See "An intraresidual i(HCA)CO(CA)NH experiment
    for the assignment of main-chain resonances in 15N,13C labeled proteins",
    J.Biomol.NMR,45, 311-318(2009) 


        	  CHOICE OF DECOUPLING AND 2D MODES

    	Set dm = 'nnn', dmm = 'ccc' 
    	Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.
	Set dm3 = 'nnn' for no 2H decoupling, or
		  'nyn'  and dmm3 = 'cwc' for 2H decoupling. 
  
    Must set = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [13C]  and t2 [15N].

    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give -90, 180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in 13C and f2180='n' for (0,0) in 15N.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.



          	  DETAILED INSTRUCTIONS 

    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for ghc_co_nhP may be printed using:
                                      "printon man('ihca_co_nhP') printoff".
             
    2. Apply the setup macro "ihca_co_nhP".  This loads the relevant parameter set
       and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral check.

    3. Center H1 frequency on H2O (4.7ppm), C13 on 174ppm, and N15 frequency
       on the amide region (120ppm). The C13 frequency is calculated in the
       sequence to be at 46ppm, ie at Cab throughout the sequence.

   4.  The parameter phi7cal (listed in dg and dg2) is provided to adjust 
       the phase of the last 90 degree C13 pulse, which is phase-shifted by
       the prior 180 degree pulse on the Ca region and by the use of SLP
       pulses on the CO region. The experimentally determined value of this
       phase is also very sensitive to small timing differences (microseconds)
       between the two theta delays. Check this phase via 1D spectra - maximise
       signal, or for greater accuracy obtain the value for a null and then add 
       or subtract 90 degrees.  The value must be a positive number. It needs to
       be calibrated once only for each spectrometer and the gc_co_nhP .  The 
       value is the same for gc_co_nhP and gcbca_co_nhP. It will be different
       from the value for gc_co_nh and gcbca_co_nh.

   5.  tauCH (1.7 ms) was determined for alphalytic protease

   6.  The coherence-transfer gradients using power levels
       gzlvl1 and gzlvl2 may be either z or magic-angle gradients. For the
       latter, a proper /vnmr/imaging/gradtable entry must be present and
       syscoil must contain the value of this entry (name of gradtable). The
       amplitude of the gzlvl1 and gzlvl2 should be lower than for a z axis
       probe to have the x and y gradient levels within the 32k range. For
       any value, a dps display (using power display) shows the x,y and z
       dac values. These must be <=32k.

   7. TROSY:
       Set TROSY='y' and dm2='nnn' for a TROSY spectrum of the bottom right
       peak of the 2D coupled NH quartet (high-field H1, low-field N15).  The 
       TROSY spectrum gives 50% S/N compared to the decoupled spectrum for a 
       small peptide.  To select any of the other three peaks of the 2D coupled
       quartet, in a clockwise direction from bottom right, change t4/t10
       from x/y to x/-y to -x/-y to -x/y.  NOTE, the phases of the SE train
       are almost the same as those determined for the gNhsqc sequence.  The
       major difference is that kappa is eliminated compared to normal
       ghc_co_nhP so the N15 magnetization has not evolved with respect to the 
       attached H's.  I.e. the N15 state would be Ix rather than IySz if there 
       was no coherence gradient - this imparts a 90 degree shift so t8 is 
       changed to y (from x in the normal spectrum).  Also gzlvl1 is after the
       180 N15 pulse rather than before as in gNhsqc, so the sign of icosel 
       and the t4/t10 phase2 increments are also swapped compared to gNhsqc.     
  8.  The N15 t2 evolution and the sensitivity enhancement train is common
       to all g...._nh sequences and the pulse sequence code for these final
       sections is in the include file, bionmr.h.
*/


#include <standard.h>
#include "bionmr.h"

static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
             phx[1]={0},   phy[1]={1},
             phi1[2]  = {1,3},
             phi2[4]  = {0,0,2,2},
             phi9[16]  = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
             rec[8]   = {0,2,2,0,2,0,0,2},		     recT[4] = {3,1,1,3};



pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            stCshape[MAXSTR],
 	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */
 
int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni = getval("ni"),
	    ni2 = getval("ni2");

double      d2_init=0.0,  		        /* used for states tppi in t1 */
	    d3_init=0.0,  	 	        /* used for states tppi in t2 */
	    tau1,         				         /*  t1 delay */
	    tauCH = getval("tauCH"), 		         /* 1/4J delay for CH */
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
            timeNCA = getval("timeNCA"),
            timeC = getval("timeC"),      /* other delays */
            tauCC = getval("tauCC"),
	    zeta = getval("zeta"),

	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
        compC = getval("compC"),
        rf0,
        rfst,

   widthHd,
   pwS1,					/* length of square 90 on Cab */
   pwS2,					/* length of square 180 on Ca */
   phi7cal = getval("phi7cal"),  /* phase in degrees of the last C13 90 pulse */
   phshift = getval("phshift"),        /*  phase shift induced on CO by 180 on CA in middle of t1 */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),
        waltzB1 = getval("waltzB1"),
	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("TROSY",TROSY);

    widthHd=2.069*(waltzB1/sfrq);  /* produces same field as std. sequence */

/*   LOAD PHASE TABLE    */

	settable(t1,2,phi1);
	settable(t2,4,phi2);
        settable(t4,1,phx);
   if (TROSY[A]=='y')
       {settable(t8,1,phy);
	settable(t9,1,phx);
 	settable(t10,1,phy);
	settable(t11,1,phx);
	settable(t12,4,recT);}
    else
       {settable(t8,1,phx);
	settable(t9,16,phi9);
	settable(t10,1,phx);
	settable(t11,1,phy);
	settable(t12,8,rec);}

        

/*   INITIALIZE VARIABLES   */

 	kappa = 5.4e-3;
	lambda = 2.4e-3;

/* maximum fine power for pwC pulses (and initialize rfst) */
        rf0 = 4095.0;    rfst=0.0;

    if( pwC > 20.0*600.0/sfrq )
	{ printf("increase pwClvl so that pwC < 20*600/sfrq");
	  psg_abort(1); }



/* 30 ppm sech/tanh inversion for Ca-Carbons */

        rfst = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));
        rfst = (int) (rfst + 0.5);
        strcpy(stCshape, "stC30");

    /* get calculated pulse lengths of shaped C13 pulses */
	pwS1 = c13pulsepw("co", "ca", "sinc", 90.0); 
	pwS2 = c13pulsepw("ca", "co", "square", 180.0); 
	

/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( gt4 > zeta - 0.6*pwC)
       { printf(" gt4 is too big. Make gt4 equal to %f or less.\n", 
  	 (zeta - 0.6*pwC)); 	     				     psg_abort(1);}

    if ( 0.5*ni*1/(sw1) > 2.0*timeC + tauCC - OFFSET_DELAY - SAPS_DELAY)
       { printf(" ni is too big. Make ni equal to %d or less.\n", 
  	 ((int)((2.0*timeC - OFFSET_DELAY)*2.0*sw1))); 	     psg_abort(1);}

    if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
       { printf("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' )
       { printf("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dm3[A] == 'y' || dm3[C] == 'y' )
       { printf("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' ");
							             psg_abort(1);}
    if ( dpwr2 > 46 )
       { printf("dpwr2 too large! recheck value  ");		     psg_abort(1);}

    if ( pw > 20.0e-6 )
       { printf(" pw too long ! recheck value ");	             psg_abort(1);} 
  
    if ( pwN > 100.0e-6 )
       { printf(" pwN too long! recheck value ");	             psg_abort(1);} 
 
    if ( TROSY[A]=='y' && dm2[C] == 'y' )
       { text_error("Choose either TROSY='n' or dm2='n' ! ");        psg_abort(1);}
 

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2) tsadd(t2,1,4);  
    if (TROSY[A]=='y')
	 {  if (phase2 == 2)   				      icosel = +1;
            else 	    {tsadd(t4,2,4);  tsadd(t10,2,4);  icosel = -1;}
	 }
    else {  if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;}
            else 			       icosel = -1;    
	 }



/*  C13 TIME INCREMENTATION and set up f1180  */

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
	{ tsadd(t2,2,4); tsadd(t12,2,4); }

   if( ix == 1) d3_init = d3; 
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t8,2,4); tsadd(t12,2,4); }



/*   BEGIN PULSE SEQUENCE   */

status(A);
   	delay(d1);
        if (dm3[B]=='y') lk_hold();

	rcvroff();
        set_c13offset("ca");
	obsoffset(tof);
	obspower(tpwr);
 	obspwrf(4095.0);
	decpower(pwClvl);
	decpwrf(4095.0);
 	dec2power(pwNlvl);
	txphase(three);
	delay(1.0e-5);

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

        if(dm3[B] == 'y')			  /*optional 2H decoupling on */
         {dec3unblank(); dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank(); setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);} 

	rgpulse(pw, zero, 0.0, 0.0);                  /* 1H pulse excitation */
                          
        txphase(zero);
        decphase(zero);
	zgradpulse(gzlvl0, gt0);			/* 2.0*GRADIENT_DELAY */

	decpwrf(rfst);
        delay(tauCH - gt0 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

        simshaped_pulse("",stCshape, 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);

        delay(tauCH - gt0 - 0.5e-3 + 70.0e-6 - 150.0e-6);

        decpwrf(rf0);

	zgradpulse(gzlvl0, gt0);   	 	        /* 2.0*GRADIENT_DELAY */
	delay(150.0e-6);
           
	rgpulse(pw, one, 0.0, 0.0);	
	zgradpulse(gzlvl3, gt3);
	delay(2.0e-4);
        decrgpulse(pwC, t1, 0.0, 0.0);

        set_c13offset("co");

	delay(zeta - 0.6*pwC - OFFSET_DELAY - POWER_DELAY - PWRF_DELAY - PRG_START_DELAY);
        
        h1decon("DIPSI2", widthHd, 0.0); /*POWER_DELAY+PWRF_DELAY+PRG_START_DELAY */
         
	delay(2.0*timeC - zeta);
 
        c13pulse("co", "ca", "sinc", 90.0, t2, 0.0, 0.0); /* pwS1 */		
	
        delay(timeNCA - tau1);

        c13pulse("ca", "co", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);
        sim3_c13pulse("", "co", "ca", "square", "", 0.0, 180.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0); /* pwS2 */

        delay(timeNCA + tau1 + (60.0e-6));
        
        initval(phshift, v3);
        decstepsize(1.0);
        dcplrphase(v3);        
        c13pulse("co", "ca", "sinc", 90.0, one, 0.0, 0.0); /* pwS1 */

        delay(2.0*timeC + tauCC - OFFSET_DELAY - SAPS_DELAY - tau1);
        c13pulse("ca", "co", "sinc", 180.0, zero, 0.0, 0.0);
        delay(tauCC);
        sim3_c13pulse("", "co", "ca", "square", "", 0.0, 180.0, 2.0*pwN, zero, zero, zero, 0.0, 60.0e-6); 
        delay(tau1);

        set_c13offset("ca");

        initval(phi7cal, v7);
        decstepsize(1.0);
        dcplrphase(v7);                                         /* SAPS_DELAY */
        dec2phase(t8);

	nh_evol_se_train("ca", "co"); /* common part of sequence in bionmr.h  */

if (dm3[B] == 'y')  lk_sample();

}		 

