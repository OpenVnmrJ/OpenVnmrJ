/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gcbca_co_nhP.c

    3D CBCA(CO)NH gradient sensitivity enhanced version.

    Correlates Cb(i),Ca(i) with N(i+1), NH(i+1).  Uses constant time evolution
    for the CaCb shifts and for the 15N shifts.

    Standard features include maintaining the 13C carrier in the CaCb region
    throughout using off-res SLP pulses; full power square pulses on 13C 
    initially when 13CO excitation is irrelevant; square pulses on the Ca and
    CaCb with first null at 13CO; one lobe sinc pulses on 13CO with first null
    at Ca;  shaka6 composite 180 pulse to simultaneously refocus Ca and invert
    CO; optional 2H decoupling when CaCb magnetization is transverse for 4 
    channel spectrometers.  

    Magic-angle option for coherence transfer gradients.  TROSY option for
    N15/H1 evolution/detection.
 
    pulse sequence: 	Grzesiek and Bax, JACS 114 6291, 1992.
    SLP pulses:     	J Magn. Reson. 96, 94-102 (1992)
    shaka6 composite: 	Chem. Phys. Lett. 120, 201 (1985)
    TROSY:		 JACS, 120, 10778 (1998)
 
    Written by Lewis Kay 09/15/92 
    Modified by L.E.K 11/24/92 to include enhanced pfg and shaped C' pulses
    Modified by L.E.K. 01/03/93 to reduce phase cycle by adding gradients
             in the t1 evolution constant time domain. H decoupling now
             begins in the interval immediately following the t1 evolution
             domain.   
    Modified by G.G 10/14/93 to permit DPS and remove gate

    (comments such as "point g" refer to pulse sequence diagram in reference)

    Modified extensively by M Robin Bendall, Varian, March 1994 and March 95
    to include the above standard features.   Revised and improved to a
    standard format by MRB, BKJ and GG for the BioPack, January 1997.      
    TROSY added Dec 98, based on similar addition to gNhsqc. Shaped pulses 
    calculated within pulse sequence code, Jan 99 (Version March 99).



        	  CHOICE OF DECOUPLING AND 2D MODES

    	Set dm = 'nnn', dmm = 'ccc' 
    	Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.
	Set dm3 = 'nnn' for no 2H decoupling, or
		  'nyn'  and dmm3 = 'cwc' for 2H decoupling. 
  
    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [13C]  and t2 [15N].

    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in 13C and f2180='n' for (0,0) in 15N.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.


          	  DETAILED INSTRUCTIONS FOR USE OF gcbca_co_nhP


    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for gcbca_co_nhP may be printed using:
                                      "printon man('gcbca_co_nhP') printoff".
             
    2. Apply the setup macro "gcbca_co_nhP".  This loads the relevant parameter
       set and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral
       check.

    3. The two C13 180 degree pulses between points c and d have been
       replaced by a single 6-pulse composite dubbed "shaka6" which povides
       near perfect refocusing with no phase roll over the Cab region and
       near perfect inversion over the CO region 18kHz off-resonance provided
       pwC < 20 us in a 600 MHz system.
      
    4. Center H1 frequency on H2O (4.7ppm), C13 on 174ppm, and N15 frequency
       on the amide region (120ppm). The C13 frequency is calculated in the
       sequence to be at 46ppm, ie at Cab throughout the sequence.

    5. The H1 frequency is NOT shifted to the amide region during the sequence.
       The H1 DIPSI2 decoupling is controlled by the waltzB1 parameter.
       Its value is specified in the new shapelib file after go or dps.  

    6. The parameter phi7cal (listed in dg and dg2) is provided to adjust 
       the phase of the last 90 degree C13 pulse, which is phase-shifted by
       the prior 180 degree pulse on the Ca region and by the use of SLP
       pulses on the CO region. The experimentally determined value of this
       phase is also very sensitive to small timing differences (microseconds)
       between the two theta delays. Check this phase via 1D spectra - maximise
       signal, or for greater accuracy obtain the value for a null and then add 
       or subtract 90 degrees.  The value must be a positive number. It needs to
       be calibrated once only for each spectrometer and the gc_co_nhP .  The 
       value is the same for gc_co_nhP and gcbca_co_nhP. It will be different from
       the value for ghc_co_nh and gc_co_nh.

    7. tauCH (1.7 ms) and timeTN (14 ms) were determined for alphalytic protease
       and are listed in dg2 for possible readjustment by the user.

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
       gcbca_co_nhP so the N15 magnetization has not evolved with respect to the 
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
             phx[1]={0},   phy[1]={1},

             phi5[2]  = {0,2},
             phi6[2]  = {2,0},  
             phi9[8]  = {0,0,1,1,2,2,3,3},
             rec[4]   = {0,2,2,0},		     recT[2]  = {3,1};



void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
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
	    timeCH = 1.1e-3,				      /* other delays */
	    timeAB = 3.3e-3,
	    zeta = 3.0e-3,
	    eta = 4.6e-3,
	    theta = 14.0e-3,
            
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */

   pwS1,					/* length of square 90 on Cab */
   pwS2,					/* length of square 180 on Ca */
   pwS3,					  /* length of sinc 180 on CO */
   pwS4,				 /*  the greatest of pwS3 and 2.0*pwN */
   phi7cal = getval("phi7cal"),  /* phase in degrees of the last C13 90 pulse */

   waltzB1 = getval("waltzB1"),
   widthHd,

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("TROSY",TROSY);

     widthHd = 2.069*(waltzB1/sfrq);  /* produces same B1 as gcbca_co_nh.c */


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

 	kappa = 5.4e-3;
	lambda = 2.4e-3;

    if( pwC > 20.0*600.0/sfrq )
	{ printf("increase pwClvl so that pwC < 20*600/sfrq");
	  psg_abort(1); }

    /* get calculated pulse lengths of shaped C13 pulses */
	pwS1 = c13pulsepw("cab", "co", "square", 90.0); 
	pwS2 = c13pulsepw("ca", "co", "square", 180.0); 
	pwS3 = c13pulsepw("co", "ca", "sinc", 180.0); 
	

/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( 0.5*ni*1/(sw1) > timeAB - gt4 - WFG_START_DELAY - pwS3 )
       { printf(" ni is too big. Make ni equal to %d or less.\n",
         ((int)((timeAB - gt4 - WFG_START_DELAY - pwS3)*2.0*sw1))); psg_abort(1);}

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
        set_c13offset("cab");
	obsoffset(tof);
	obspower(tpwr);
 	obspwrf(4095.0);
	decpower(pwClvl);
	decpwrf(4095.0);
 	dec2power(pwNlvl);
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

   	rgpulse(pw,zero,0.0,0.0);                      /* 1H pulse excitation */

   	decphase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(tauCH - gt0);

   	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

   	txphase(one);
	decphase(t3);
	zgradpulse(gzlvl0, gt0);
	delay(tauCH - gt0);

   	rgpulse(pw, three, 0.0, 0.0);
	zgradpulse(gzlvl3, gt3);
	delay(2.0e-4);
      if ( dm3[B] == 'y' )     /* begins optional 2H decoupling */
        {
          gt4=0.0;             /* no gradients during 2H decoupling */
          dec3rgpulse(1/dmf3,one,10.0e-6,2.0e-6);
          dec3unblank();
          dec3phase(zero);
          delay(2.0e-6);
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
        }
    	decrgpulse(pwC, t3, 0.0, 0.0);
						         	/* point a */
	txphase(zero);
	decphase(zero);
     	delay(tau1);
						  /*  WFG3_START_DELAY & pwS3 */
	sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
					     zero, zero, zero, 2.0e-6, 0.0);

	zgradpulse(gzlvl4, gt4);
	pwS4 = pwS3;    if ( pwS4 < 2.0*pwN)	pwS4 = 2.0*pwN;
     	delay(timeCH - 0.6*pwC - pwS4 - gt4 - WFG3_START_DELAY - 2.0*pw);

     	rgpulse(2.0*pw, zero, 0.0, 0.0);

     	delay(timeAB - timeCH);

 	c13pulse("cab", "co", "square", 180.0, zero, 2.0e-6, 0.0);

	zgradpulse(gzlvl4, gt4);
     	delay(timeAB - tau1 - gt4 - 2.0*WFG_START_DELAY - pwS3 - 0.6*pwS1 - 
						POWER_DELAY - PWRF_DELAY);
 							/*  WFG_START_DELAY   */
	c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 0.0);       /* pwS3 */
		                                         	/* point b */
 	c13pulse("cab", "co", "square", 90.0, zero, 2.0e-6, 0.0);  /* pwS1 etc*/
	h1decon("DIPSI2", widthHd, 0.0);/*POWER_DELAY+PWRF_DELAY+PRG_START_DELAY */
                    						/* point c */
	decphase(t5);
	delay(zeta -PRG_START_DELAY -POWER_DELAY -PWRF_DELAY - 0.5*10.933*pwC);

	decrgpulse(pwC*158.0/90.0, t5, 0.0, 0.0);
	decrgpulse(pwC*171.2/90.0, t6, 0.0, 0.0);
	decrgpulse(pwC*342.8/90.0, t5, 0.0, 0.0);	/* Shaka composite   */
	decrgpulse(pwC*145.5/90.0, t6, 0.0, 0.0);
	decrgpulse(pwC*81.2/90.0, t5, 0.0, 0.0);
	decrgpulse(pwC*85.3/90.0, t6, 0.0, 0.0);

	decphase(zero);
	delay(zeta - 0.5*10.933*pwC - WFG_START_DELAY - 2.0e-6);

				        		  /* WFG_START_DELAY  */
	c13pulse("cab", "co", "square", 90.0, zero, 2.0e-6, 0.0);  /* point d */
	decphase(t5);
        if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
           {
           setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
           dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
           dec3blank();
           lk_autotrig();   /* resumes lock pulsing */
           }
	zgradpulse(gzlvl3, gt3);
	delay(2.0e-4);
	c13pulse("co", "ca", "sinc", 90.0, t5, 2.0e-6, 0.0);
	     							/* point e */ 

 	decphase(zero);
	delay(eta - 2.0*POWER_DELAY - 2.0*PWRF_DELAY);

					        /* 2*POWER_DELAY+2*PWRF_DELAY */
	c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);     /* pwS2 */

		
	dec2phase(zero);
	delay(theta - eta - pwS2 - WFG3_START_DELAY);

							  /* WFG3_START_DELAY */
	sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
					     zero, zero, zero, 2.0e-6, 2.0e-6);

	initval(phi7cal, v7);
	decstepsize(1.0);
	dcplrphase(v7);					        /* SAPS_DELAY */
	dec2phase(t8);
	delay(theta - SAPS_DELAY);
                              					/* point f */

	nh_evol_se_train("co", "ca"); /* common part of sequence in bionmr.h  */
        if (dm3[B]=='y') lk_sample();

}		 

