/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gihca_co_nP_pp.c               

    3D Intraresidual hca_nP gradient sensitivity enhanced version.

    3D ihca_co_nP correlates Ha(i), Ca(i) and N(i).
    Ref: Mäntylahti et al. J. Biomol. NMR, 47, 171-181 (2010).

    Uses four channels:
         1)  1H   (t3)       - carrier  4.7 ppm (tof)
         2) 13CO             - carrier  174 ppm (dof)
            13Ca  (t2, ni2)   - carrier  56 ppm 
         3) 15N   (t1, ni)   - carrier  120 ppm (dof2)
         4)  2H              - carrier   4.5ppm (dof3)


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



          	  DETAILED INSTRUCTIONS FOR USE OF ghc_co_nhP

    1. Obtain a printout of the Philosopy behind the ProteinPack development,
       and General Instructions using the macro: 
                                      "printon man('ProteinPack') printoff".
       These Detailed Instructions for ghc_co_nhP may be printed using:
                                      "printon man('ghc_co_nhP') printoff".
             
    2. Apply the setup macro "ghc_co_nhP".  This loads the relevant parameter set
       and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral check.

    3. The parameter ncyc corresponds to the number of cycles of DIPSI-3 mixing.
       Use ncyc = 2 or 3 usually.  This corresponds to a total mixing time of
       (2 or 3)*6.07*600/sfrq ms.  The DIPSI rf field of 9 kHz for a 600Mhz 
       spectrometer, which is scaled against field strength, is sufficient to
       cover 14.4 kHz of spectral width (96 ppm at any field strength) and is 
       more than adequate for the CC J's.  The 9 kHz is achieved by setting 
       the bandwidth no. to 120.0 ppm.  Change this in the c13decouple(...)
       pulse sequence statement if a different rf strength is required.  The
       new value is specified in the new shapelib file after go or dps.  NOTE
       that the bandwidth must be specified to one decimal place, eg. 120.0.

    4. The two C13 180 degree pulses after the DIPSI-3 decoupling have been
       replaced by a single 6-pulse composite dubbed "shaka6" which povides
       near perfect refocusing with no phase roll over the Cab region and
       near perfect inversion over the CO region 18kHz off-resonance provided
       pwC < 20 us in a 600 MHz system.
      
    5. Center H1 frequency on H2O (4.7ppm), C13 on 174ppm, and N15 frequency
       on the amide region (120ppm). The C13 frequency is calculated in the
       sequence to be at 46ppm, ie at Cab throughout the sequence.

    6. The H1 frequency is NOT shifted to the amide region during the sequence.
       The H1 DIPSI2 decoupling was increased to 7.3 kHz to achieve better H2O
       suppression.  Less rf may be chosen by changing the bandwidth no. from
       27.0 ppm in the h1decon(...) pulse sequence statement.  The
       new value is specified in the new shapelib file after go or dps.  NOTE
       that the bandwidth must be specified to one decimal place, eg. 20.0.

   7.  The parameter phi7cal (listed in dg and dg2) is provided to adjust 
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

   8.  tauCH (1.7 ms) and timeTN (14 ms) were determined for alphalytic protease
       and are listed in dg2 for possible readjustment by the user.

   9.  The coherence-transfer gradients using power levels
       gzlvl1 and gzlvl2 may be either z or magic-angle gradients. For the
       latter, a proper /vnmr/imaging/gradtable entry must be present and
       syscoil must contain the value of this entry (name of gradtable). The
       amplitude of the gzlvl1 and gzlvl2 should be lower than for a z axis
       probe to have the x and y gradient levels within the 32k range. For
       any value, a dps display (using power display) shows the x,y and z
       dac values. These must be <=32k.

   10. TROSY:
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
  11.  The N15 t2 evolution and the sensitivity enhancement train is common
       to all g...._nh sequences and the pulse sequence code for these final
       sections is in the include file, bionmr.h.
*/


#include <standard.h>
#include "bionmr.h"

static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
             phi1[2]  = {0,2},
             phi2[4]  = {0,0,2,2},
             phi3[8]  = {1,1,1,1,3,3,3,3},
             phi10[1] = {0},
	     rec[8]   = {0,2,2,0,2,0,0,2};



void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            stCshape[MAXSTR],
            mag_flg[MAXSTR];			    
 
int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni = getval("ni"),
            icosel,
	    ni2 = getval("ni2");

double      d2_init=0.0,  		        /* used for states tppi in t1 */
	    d3_init=0.0,  	 	        /* used for states tppi in t2 */
	    tau1,         				         /*  t1 delay */
	    tauCH = getval("tauCH"), 		         /* 1/4J delay for CH */
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
            timeNCA = getval("timeNCA"),
            timeC = getval("timeC"),      /* other delays */
	    zeta = getval("zeta"),
            tauCC = getval("tauCC"),

	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
        compC = getval("compC"),
        rf0,
        rfst,
        corr,

        waltzB1 = getval("waltzB1"),  /* waltz16 field strength (in Hz)     */
        compH = getval("compH"),
        pwHd,                           /* H1 90 degree pulse length at tpwrd */
        tpwrd,                                     /* rf for WALTZ decoupling */

   pwS1,					/* length of square 90 on Cab */
   pwS2,					/* length of square 180 on Ca */
   phi7cal = getval("phi7cal"),  /* phase in degrees of the last C13 90 pulse */
   phshift = getval("phshift"),        /*  phase shift induced on CO by 180 on CA in middle of t1 */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gzcal = getval("gzcal"),
	gstab = getval("gstab"),
    
	gt0 = getval("gt0"),  
        gt1 = getval("gt1"),  
        gt2 = getval("gt2"), 
	gt3 = getval("gt3"),
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),
	gzlvl3 = getval("gzlvl3"),
        gzlvl5 = getval("gzlvl5");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("mag_flg",mag_flg);


/*   LOAD PHASE TABLE    */

	settable(t1,2,phi1);
	settable(t2,4,phi2);
        settable(t3,8,phi3);
        settable(t10,1,phi10);
	settable(t12,8,rec);
        

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

/* power level and pulse time for WALTZ 1H decoupling */
        pwHd = 1/(4.0 * waltzB1) ;                  
        tpwrd = tpwr - 20.0*log10(pwHd/(compH*pw));
        tpwrd = (int) (tpwrd + 0.5);
	

/* CHECK VALIDITY OF PARAMETER RANGES */


       if ( dm[A] == 'y' || dm[B] == 'y')
       { printf("incorrect dec1 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' )
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
 

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2) tsadd(t1,1,4);  
    if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;}
      else                             icosel = -1;


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
	{ tsadd(t1,2,4); tsadd(t12,2,4); }

   if( ix == 1) d3_init = d3; 
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t12,2,4); }



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

/*        delay(tauCH - gt0);
        simpulse(2.0*pw, 2.0*pwC, zero,zero,0.0,0.0);
        delay(tauCH - gt0 - 150.0e-6); */

	decpwrf(rfst);
        delay(tauCH - gt0 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

        simshaped_pulse("",stCshape, 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);

        delay(tauCH - gt0 - 0.5e-3 + 70.0e-6 - 150.0e-6);

        decpwrf(rf0);

	zgradpulse(gzlvl0, gt0);   	 	        /* 2.0*GRADIENT_DELAY */
	delay(150.0e-6);
           
	rgpulse(pw, one, 0.0, 0.0);	
        obspower(tpwrd);
	
        decrgpulse(pwC, t3, 0.0, 0.0);

        set_c13offset("co");

	delay(zeta - 0.6*pwC - OFFSET_DELAY - 2*pwHd - PRG_START_DELAY);

	rgpulse(pwHd,zero,0.0,0.0);
        rgpulse(pwHd,one,0.0,0.0);
        txphase(zero);
        delay(2.0e-6);
        obsprgon("waltz16", pwHd, 90.0); /* PRG_START_DELAY */
        xmtron();
 
	delay(2.0*timeC - zeta);
 
        c13pulse("co", "ca", "sinc", 90.0, zero, 0.0, 0.0); /* pwS1 */		
	
        delay(timeNCA - 2.0*timeC);

        c13pulse("ca", "co", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);
        sim3_c13pulse("", "co", "ca", "square", "", 0.0, 180.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0); /* pwS2 */

        delay(timeNCA - 2.0*timeC + (60.0e-6));
        
        initval(phshift, v3);
        decstepsize(1.0);
        dcplrphase(v3);        
        c13pulse("co", "ca", "sinc", 90.0, one, 0.0, 0.0); /* pwS1 */

        delay(2.0*timeC + tauCC - OFFSET_DELAY - SAPS_DELAY);
        c13pulse("ca", "co", "sinc", 180.0, zero, 0.0, 0.0);
        delay(tauCC);
        sim3_c13pulse("", "co", "ca", "square", "", 0.0, 180.0, 2.0*pwN, zero, zero, zero, 0.0, 60.0e-6);

        set_c13offset("ca");

        initval(phi7cal, v7);
        decstepsize(1.0);
        dcplrphase(v7);                                   /* SAPS_DELAY */
        dec2phase(t1);

        c13pulse("ca", "co", "square", 90.0, zero, 0.0, 0.0); /* pwS1 */

        xmtroff();
        obsprgoff();
        rgpulse(pwHd,three,2.0e-6,0.0);

        zgradpulse(2*gzlvl3, 0.8*gt3);
	delay(2.0e-4);

        rgpulse(pwHd,one,0.0,0.0);
        txphase(zero);
        delay(2.0e-6);
        obsprgon("waltz16", pwHd, 90.0); /* PRG_START_DELAY */
        xmtron();        

        dec2rgpulse(pwN, t1, 0.0, 0.0);
	
      corr = 0.5*(4.0*pwN/PI +pwS1 +pwS2 +WFG2_START_DELAY +2.0*SAPS_DELAY +PWRF_DELAY);

      if (tau1 > corr) delay(tau1 -corr);

      c13pulse("ca", "co", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);
      c13pulse("co", "ca", "square", 180.0, zero, 2.0e-6, 2.0e-6);

      if (tau1 > corr) delay(tau1 -corr);
	
        dec2rgpulse(pwN, zero, 0.0, 0.0);

        xmtroff();
        obsprgoff();
        rgpulse(pwHd,three,2.0e-6,0.0);

 	zgradpulse(0.6*gzlvl3, 0.8*gt3);
	delay(2.0e-4);

        rgpulse(pwHd,one,0.0,0.0);
        txphase(zero);
        delay(2.0e-6);
        obsprgon("waltz16", pwHd, 90.0); /* PRG_START_DELAY */
        xmtron();        

        c13pulse("ca", "co", "square", 90.0, t2, 0.0, 0.0); /* pwS1 */		

/* Calpha CT evolution begins here */

    delay(timeTN -tau2 -SAPS_DELAY);

    sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0); /* pwS2 */

    if (tau2 > 2.0*tauCH)
        {
          delay(timeTN - 60.4e-6 -WFG_START_DELAY);           /* WFG_START_DELAY */
          c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);
          delay(tau2 -2.0*tauCH -PRG_STOP_DELAY -pwHd -2.0e-6);
          xmtroff();
          obsprgoff();                                      /* PRG_STOP_DELAY */
          rgpulse(pwHd,three,2.0e-6,0.0);

          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);      /* 2.0*GRADIENT_DELAY */
          obspower(tpwr);                                      /* POWER_DELAY */
          decpwrf(rf0);
          decphase(t10);
          delay(2.0*tauCH -gt1 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY -SAPS_DELAY);
        }
    else if (tau2 > (2.0*tauCH -60.4e-6 -WFG_START_DELAY))
        {
          delay(timeTN +tau2 -2.0*tauCH -PRG_STOP_DELAY -pwHd -2.0e-6);
          xmtroff();
          obsprgoff();                                      /* PRG_STOP_DELAY */
          rgpulse(pwHd,three,2.0e-6,0.0);                       /* WFG_START_DELAY */
          c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);      /* 2.0*GRADIENT_DELAY */
          obspower(tpwr);                                      /* POWER_DELAY */
          decpwrf(rf0);
          decphase(t10);
          delay(2.0*tauCH -60.4e-6 -WFG_START_DELAY -gt1 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY -SAPS_DELAY);
        }
    else if (tau2 > (gt1 +gstab))
        {
          delay(timeTN +tau2 -2.0*tauCH -PRG_STOP_DELAY -pwHd -2.0e-6);
          xmtroff();
          obsprgoff();                                      /* PRG_STOP_DELAY */
          rgpulse(pwHd,three,2.0e-6,0.0);

          delay(2.0*tauCH -tau2 -60.4e-6 -WFG_START_DELAY);   /* WFG_START_DELAY */
          c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);      /* 2.0*GRADIENT_DELAY */
          obspower(tpwr);                                      /* POWER_DELAY */
          decpwrf(rf0);
          decphase(t10);
          delay(tau2 -gt1 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY -SAPS_DELAY);
        }
    else
        {
          delay(timeTN +tau2 -2.0*tauCH -PRG_STOP_DELAY -pwHd -2.0e-6);
          xmtroff();
          obsprgoff();                                      /* PRG_STOP_DELAY */
          rgpulse(pwHd,three,2.0e-6,0.0);

          delay(2.0*tauCH -tau2 -60.4e-6 -WFG2_START_DELAY -gt1 -2.0*GRADIENT_DELAY -gstab);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);      /* 2.0*GRADIENT_DELAY */
          obspower(tpwr);                                      /* POWER_DELAY */
          delay(gstab -POWER_DELAY);                    /* WFG_START_DELAY */
          c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);
          decpwrf(rf0);
          decphase(t10);
          if (tau2 > POWER_DELAY -SAPS_DELAY) delay(tau2 -POWER_DELAY -SAPS_DELAY);
           else delay(tau2);
        }
              
        simpulse(pw, pwC, zero, t10, 0.0,0.0);
        zgradpulse(0.8*gzlvl5, gt5);
    decphase(zero);

    delay(tauCH -pwC -gt5 -2.0*GRADIENT_DELAY);

    simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

    zgradpulse(0.8*gzlvl5, gt5);
    txphase(one);
    decphase(one);
    delay(tauCH - pwC - gt5 -2.0*GRADIENT_DELAY);

    simpulse(pw, pwC, one, one, 0.0, 0.0);

    zgradpulse(gzlvl5, gt5);
    txphase(zero);
    decphase(zero);
    delay(tauCH -gt5 -2.0*GRADIENT_DELAY);

    simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

    zgradpulse(gzlvl5, gt5);
    decpower(dpwr);

    delay(tauCH -gt5 -2.0*GRADIENT_DELAY -POWER_DELAY);

    rgpulse(pw, zero, 0.0, 0.0);

    delay(1.0e-4 +gstab + gt1/4.0 + 2.0*GRADIENT_DELAY);
    rgpulse(2.0*pw, zero, 0.0, 0.0);
    if(mag_flg[A] == 'y') magradpulse(gzcal*gzlvl2, gt1/4.0);
      else zgradpulse(icosel*gzlvl2, gt1/4.0);

    delay(gstab);
    rcvron();
    statusdelay(C, 1.0e-4);
    if (dm3[B]=='y') lk_sample();

    setreceiver(t12);

}		 

