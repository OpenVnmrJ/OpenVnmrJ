/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghn_coca_cb.c

    3D HN(COCA)CB gradient sensitivity enhanced version.
    Designed for triply labeled proteins with H2(>80%), C13 and 15N.

    Correlates Cb(i-1) with N(i) and  NH(i).  Uses
    constant time evolution for the 15N shifts and 13Cb shifts (optional).
    TROSY option for   N15/H1 evolution/detection.
 
    pulse sequence: Yamazaki, Lee,Arrowsmith, Muhandiram,
                    LEK, JACS 116 (1994) p 11655
    
    Wittekind and Mueller, JMR B101, 201 (1993)
		    Muhandiram and Kay, JMR, B103, 203 (1994)
		    Kay, Xu, and Yamazaki, JMR, A109, 129-133 (1994)
    
    TROSY:	    Weigelt, JACS, 120, 10778 (1998)
    
    Shaka's BIP (broadband inversion Pulses )
                 Mari A. Smith, Haitao Hu, and A. J. Shaka
                Journal of Magnetic Resonance 151, 269 283 (2001)
     
    rewritten by Evgeny Tishchenko, Sept 2009, Varian
      
                 CHOICE OF DECOUPLING AND 2D MODES

        Set dm = 'nnn', dmm = 'ccc'
        Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.
        Set dm3 = 'nnn' for no 2H decoupling, or
                  'nyn'  and dmm3 = 'cwc' for 2H decoupling.


    Set CT_c='y' only if you have highly (>95%) deuterated protein
    Deuterium decoupling, dm3='nyn', is not default, but strongly recommended.

    Must set = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [13C]  and t2 [15N].

    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in 13C and f2180='n' for (0,0) in 15N.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.


                   DETAILED INSTRUCTIONS FOR USE OF ghn_coca_cb


    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro:
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for ghn_coca_cb may be printed using:
                                      "printon man('ghn_coca_cb') printoff".

    2. Apply the setup macro "ghn_coca_cb".  This loads the relevant parameter set
       and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral check.
       

    3.  The delay tauCC is adjustable, 6.6ms seems to be ok, maybe
	shortened for bigger proteins.

    4.  set  highSN='y' for samples with high signal-to-noise ratios; this 
	inserts a gradient filter to remove possible artifacts for a price of 
	approx 1.5ms delay in CB t1 evolution.
        Set highSN='n' if you want to maximize signal (large proteins, low S/N)
       
    5. Centre H1 frequency on H2O (4.7ppm), C13 frequency (dof) on 46ppm, 
         C13 frequency dofCO on 174 ppm
	 and N15 frequency on the amide region (120 ppm). 

    6. H2O preservation is achieved according to Kay et al, except that a sinc
       one-lobe selective pulse is used to put H2O back along z.  This is much
       more selective than a hard, Seduce1, or gaussian pulse.

    7. TROSY:
       Set TROSY='y' and dm2='nnn' for a TROSY spectrum of the bottom right
       peak of the 2D coupled NH quartet (high-field H1, low-field N15).  The
       TROSY spectrum gives 50% S/N compared to the decoupled spectrum for a
       small peptide.  To select any of the other three peaks of the 2D coupled
       quartet, in a clockwise direction from bottom right, change t4/t10
       from x/y to x/-y to -x/-y to -x/y.  NOTE, the phases of the SE train
       are almost the same as those determined for the gNhsqc sequence.  The
       major difference is that kappa is eliminated compared to normal
       ghn_coca_cb so the N15 magnetization has not evolved with respect to the
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
*/
#include <standard.h>
static int   phi5[4]  =  {0,1,0,1}, /* 180 on CA-cb inept */
	     phi4[4]  =  {2,2,2,2}, /* 180 on CBCA in t1 */
             phi1[4] =   {1,3,1,3},  /*last CB t1 evolution pulse */ 
	     phi6[4]  =  {0,0,1,1}, /* 180 in CB-CA inept */  
             phi3[4]  =  {0,0,0,0}, /* last 90 pulse in CB to CA inpet  */
	     phi2[4]  =  {0,0,0,0}, /* first N evolution pulse */
	     phi2t[4]  =  {1,1,1,1}, /* first N evolution pulse */      
	     phi10[4]  = {0,0,0,0}, /* first N pulse in the reverse inept transfer */

             rec[4]   =  {0,2,2,0}, /*receiver */
             recT[4]   =  {3,1,1,3},
	   
	     /* hsqc tables */
	     phi8[1]  ={0},     /* first 90 on 1H in trosy/reverse inept */
	     phi9[1]  ={0},     /*last 90  on15N  in trosy  */
	     

             phi11[1] ={0},    /* last 90 on 15N in N->CO transfer */
	     phi12[1] ={0},    /* first 180 on H1 in trosy/reverse inepst */
	     phi13[1] ={1},   /* second 90 on H1  trosy/reverse inept */
	     phi14[1] ={1},   /* first 90/second on 15N in  trosy/reverse inept */ 
	     
	     /** trosy tables */

	     phi7[1]  = {3},     /* first selective 90 on 1H in trosy */
	     phi8t[1]  ={0},     /* first 90 on 1H in trosy/reverse inept TROSY SELECTION*/
	     phi9t[1]  ={1},     /*last 90  on15N  in trosy TROSY SELECTION */	
 	     phi11t[1] ={0},    /* last 90 on 15N in N->CO transfer */	     
	     phi12t[1] ={0},    /* first 180 on H1 in trosy/reverse inepst */
	     phi13t[1] ={1},   /* second 90 on H1  trosy/reverse inept */
	     phi14t[1] ={0};   /* first 90/second on 15N in  trosy/reverse inept */ 
	      /*** end of trosy tables */
		    
static double   d2_init=0.0, d3_init=0.0;

pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
 	    TROSY[MAXSTR],			    /* do TROSY on N15 and H1 */
	    CT_c[MAXSTR],
	    h1dec[MAXSTR],
	    highSN[MAXSTR],

            shCO90[MAXSTR],
	    shCO180[MAXSTR], 
	    shCO180offCACB[MAXSTR],         /* shapes for 13C  pulses */
	    shCACB90[MAXSTR],
	    shCACB180[MAXSTR],
	    shCBIP[MAXSTR];
	    
int         icosel = 1,          		/* used to get n and p type   */
            t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
	    tauCC =  getval("tauCC"), 		   /* delay for Ca to Cb cosy */
	    tauC = 13.3e-3,	           /* constantTime for 13Cb evolution */
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
	    kappa = 5.4e-3,
	    lambda = 2.4e-3,
	    zeta = 4.5e-3,
	    taud = 1.7e-3,
	    dofCO = getval("dofCO"),
            dof = getval("dof"),
	    pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
            pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	    rf0,            	  /* maximum fine power when using pwC pulses */


        pwCO90 =   getval("pwCO90"),   /* 90 degree pulse length on carbonyl */
	pwCO90_phase_roll=getval("pwCO90_phase_roll"), /* phase roll compensation multiplier,2/pi for hard pulses */
        pwCO180 =  getval("pwCO180"), /* 180 degree pulse length on carbonyl */
	pwCACB90 = getval("pwCACB90"),   /* 90 degree pulse length on CACB */
        pwCACB90_phase_roll=getval("pwCACB90_phase_roll"),
	pwCACB180 =getval("pwCACB180"), /* 180 degree pulse length on CACB */
        pwCBIP =getval("pwCBIP"),       /* Broadband inversion pulse on C13 */
	
        pwCO90pwr =   getval("pwCO90pwr"),   /* 90 degree pwr on carbonyl */
        pwCO180pwr =  getval("pwCO180pwr"), /* 180 degree pwr on carbonyl */
	pwCACB90pwr = getval("pwCACB90pwr"),   /* 90 degree pwr on CACB */
        pwCACB180pwr =getval("pwCACB180pwr"), /* 180 degree pwr CACB */
 
   compH = getval("compH"),       /* adjustment for C13 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */

   	pwHd,	    		        /* H1 90 degree pulse length at tpwrd */
   	tpwrd,	  	                   /* rf for WALTZ decoupling */
        waltzB1 = getval("waltzB1"),  /* waltz16 field strength (in Hz)     */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

        gstab = getval("gstab"),
	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
        gzcal = getval("gzcal"),             /* g/cm to DAC conversion factor */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gt7 = getval("gt7"),
	gt8 = getval("gt8"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6"),
	gzlvl7 = getval("gzlvl7"),
	gzlvl8 = getval("gzlvl8");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("mag_flg",mag_flg);
    getstr("TROSY",TROSY);
    getstr("CT_c",CT_c);
    getstr("h1dec",h1dec);
    
getstr("shCO90",shCO90);
getstr("shCO180",shCO180); 
getstr("shCO180offCACB",shCO180offCACB);    
getstr("shCACB90",shCACB90);
getstr("shCACB180",shCACB180);
getstr("shCBIP",shCBIP);

getstr("highSN",highSN);   



/*   LOAD PHASE TABLE    */

settable(t1,4,phi1);  /*last in CACB t1 evolution */
	
settable(t3,4,phi3);  /* last pulse in CB -> CA transfer */
settable(t4,4,phi4);  /* cacb 180 in t1 */
settable(t5,4,phi5);  /* cacb 180 in CA-CB inept*/
settable(t6,4,phi6);  /* cacb 180 in CB-CA inept*/

settable(t7,1,phi7);  /* selective 90 on 1H    in trosy */
if (TROSY[A]=='n')
 {
   settable(t2,4,phi2);  /* first N in t2 evolution */
   settable(t8,1,phi8);  /* first 90 on 1H    in trosy TROSY SELECTION*/
   settable(t9,1,phi9);  /* last  90 on 15N in trosy TROSY SELECTION*/
   settable(t11,1,phi11);
   settable(t12,1,phi12);    /* first 180 on H1 in trosy/reverse inepst */
   settable(t13,1,phi13);   /* second 90 on H1  trosy/reverse inept */
   settable(t14,1,phi14);   /* first 90/second on 15N in  trosy/reverse inept */
   settable(t16,4,rec);
 }
else 
 {
   settable(t2,4,phi2t);  /* first N in t2 evolution */
   settable(t8,1,phi8t);  /* first 90 on 1H    in trosy TROSY SELECTION*/
   settable(t9,1,phi9t);  /* last  90 on 15N in trosy TROSY SELECTION*/
   settable(t11,1,phi11t);
   settable(t12,1,phi12t);    /* first 180 on H1 in trosy/reverse inepst */
   settable(t13,1,phi13t);   /* second 90 on H1  trosy/reverse inept */
   settable(t14,1,phi14t);   /* first 90/second on 15N in  trosy/reverse inept */
   settable(t16,4,recT);
 }

settable(t10,4,phi10);/* first N in reverse inept */

/*   INITIALIZE VARIABLES   */

    if( dpwrf < 4095 )
	{ printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
	  psg_abort(1); }

    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;
 	
    /* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69)); /* needs 1.69 times more */
    tpwrs = (int) (tpwrs);                       /* power than a square pulse */

    /* power level and pulse time for WALTZ 1H decoupling */
	pwHd = 1/(4.0 * waltzB1) ;                          
	tpwrd = tpwr - 20.0*log10(pwHd/(compH*pw));
	tpwrd = (int) (tpwrd + 0.5);
 
/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( 0.5*ni2*1.0/(sw2) > timeTN - pwCACB180 -POWER_DELAY)
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n", 
  	 ((int)((timeTN - pwCACB180-POWER_DELAY)*2.0*sw2))); psg_abort(1);}


/* limit 13C evolution time in general, 2H and  1H decoupling during t1 */
    if (  (ni/sw1 > 2.0*(tauC- pwCACB180 -pwCO180 -gt7-gstab)) )
       { printf(" ni is too big. Make ni equal to %d or less.\n", 
  	  (int)((tauC- pwCACB180-pwCO180 -gt7-gstab)*2*sw1) ); psg_abort(1);}
       
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

    if (phase1 == 2)   { tsadd(t1,1,4);tsadd(t3,1,4);} 
   
 /*trosy version */
if(TROSY[A]=='y') {     if (phase2 == 2)  icosel = +1;
			else {tsadd(t8,2,4); tsadd(t9,2,4); icosel = -1;} 
		  }
/* normal hsqc */
if(TROSY[A]=='n') {     if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;} 
			else icosel = -1;  
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
	{ tsadd(t1,2,4); tsadd(t3,2,4); tsadd(t16,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t16,2,4); }



/* BEGIN PULSE SEQUENCE */

status(A);
delay(d1);
if (dm3[B] == 'y') lk_hold(); /* taking care of lock */

rcvroff();

/* setting power levels */
obspower(tpwr); obspwrf(4095.0);
decpower(pwClvl); decpwrf(4095.0);
dec2power(pwNlvl);

obsoffset(tof);
txphase(zero);
decphase(zero);

delay(1.0e-5);
 /*destroy N15 and C13 magnetization*/
if(TROSY[A]=='n') 
 {
  dec2rgpulse(pwN, zero, 0.0, 0.0); 
  decrgpulse(pwC, zero, 0.0, 0.0);
  zgradpulse(gzlvl0, 0.5e-3);
  delay(gstab);
 }

/********** START Hz -> HzNz  ***/

 rgpulse(pw,zero,0.0,0.0);                      /* 1H pulse excitation */

 dec2phase(zero);
 
 delay(gstab);
 zgradpulse(gzlvl0, gt0);
 delay(lambda - gt0-gstab);

 sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
  delay(lambda - gt0 -gstab);
  txphase(one);
  zgradpulse(gzlvl0, gt0);
  delay(gstab);
 rgpulse(pw, one, 0.0, 0.0);

  obspower(tpwrs);
  txphase(zero);
  shaped_pulse("H2Osinc", pwHs, zero, 5.0e-4, 0.0);
  obspower(tpwrd);
  zgradpulse(gzlvl3, gt3);
  delay(gstab);
  
  
  /* carrier on CO  */

  decoffset(dofCO);
  decpower(pwCO180pwr);
  
  /* HzNz -> HzNy, then to NzCOz */
  dec2rgpulse(pwN, zero, 0.0, 0.0);


  
   if ( TROSY[A]=='n')
 {	
  txphase(one);
  delay(kappa - pwHd - 2.0e-6 - PRG_START_DELAY);
  rgpulse(pwHd,one,1.0e-6,0.0e-6); txphase(zero); delay(1.0e-6);    
  xmtron(); obsprgon("waltz16", pwHd, 90.0);
  delay(timeTN - kappa -WFG3_START_DELAY);
 }	          
   else 
 {
   txphase(zero); obspower(tpwrs);
   delay(0.5*kappa - pwHs -1.0*pw -2.0*POWER_DELAY - 3.0e-6);
   shaped_pulse("H2Osinc", pwHs, zero, 1.0e-6, 1.0e-6);
   obspower(tpwr);txphase(two); 
   rgpulse(2.0*pw, two, 1.0e-6, 1.0e-6);
   obspower(tpwrs);txphase(zero); 
   shaped_pulse("H2Osinc", pwHs, zero, 1.0e-6, 1.0e-6);
   obspower(tpwrd);
   delay(timeTN -0.5*kappa - pwHs -1.0*pw - 3.0e-6 -2.0*POWER_DELAY-WFG3_START_DELAY);
 }
   decphase(zero);
   dec2phase(zero);  
  
   /**** 180 on CO  ***/ 
   sim3shaped_pulse("",shCO180,"",0.0, pwCO180, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
   dec2phase(t11);
   delay(timeTN -WFG3_STOP_DELAY);
   dec2rgpulse(pwN, t11, 0.0, 0.0); 
  
   /*purge */
   if ( TROSY[A]=='n')
    { obsprgoff(); xmtroff(); txphase(three); rgpulse(pwHd,three,1.0e-6,0.0);}
   decpower(pwCO90pwr);
   delay(2.0e-6);
   zgradpulse(gzlvl3, gt3);
   delay(gstab);

   /* carrier is still on CO */
   /*******CO->CA***********/
 
decshaped_pulse(shCO90, pwCO90, zero, 0.0, 0.0);
decpower(pwClvl); 
delay(zeta - 0.5*pwCBIP -pwCO90*pwCO90_phase_roll - POWER_DELAY);
 decshaped_pulse(shCBIP, pwCBIP, zero, 0.0, 0.0);
delay(zeta - 0.5*pwCBIP);


delay(pwCO90*pwCO90_phase_roll + WFG_STOP_DELAY+ WFG_START_DELAY);
decphase(zero);
delay(5.0e-6  + POWER_DELAY);
decshaped_pulse(shCBIP, pwCBIP, zero, 0.0, 0.0); /*phase roll compensation */
decphase(one);
decpower(pwCO90pwr);
delay(5.0e-6); 
                                                        
decshaped_pulse(shCO90, pwCO90, one, 0.0, 0.0);

/*************  CO->CA DONE , coherence is NzCOzCAz now*********/

delay(2.0e-6);
zgradpulse(1.33*gzlvl3,gt3);
delay(gstab);

/*******************   H2 and 1H decouplers ON */

if(dm3[B] == 'y')
 {  
  if(1.0/dmf3>900.0e-6)
   {
    dec3power(dpwr3+6.0);
    dec3rgpulse(0.5/dmf3, one, 1.0e-6, 0.0e-6);
    dec3power(dpwr3);
   }
  else
    dec3rgpulse(1.0/dmf3, one, 1.0e-6,0.0e-6);
  dec3phase(zero); 
  dec3unblank(); 
  setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
 }	

if (h1dec[B]=='y') {
          	    txphase(one);  rgpulse(pwHd,one,1.0e-6,0.0e-6); txphase(zero); 
	            xmtron();   obsprgon("waltz16", pwHd, 90.0);                   
  	           }
 


/***** CARRIER ON CACB  */

decoffset(dof);
decphase(zero);

/************CA->CB,     NzCOzCAz -> NzCOzCAzCBx **********/
/*****  90 on CACb */
decpower(pwCACB90pwr);
decshaped_pulse(shCACB90, pwCACB90, zero, 0.0, 0.0);

delay(2.0e-6);
decphase(t5);
decpower(pwCACB180pwr);
 delay(tauCC);


decshaped_pulse(shCACB180, pwCACB180,t5, 0.0, 0.0);



decphase(one);
decpower(pwCACB90pwr);
delay(2.0e-6);
delay(tauCC);
 

/*   xxxxxxxxxxxxxxxxxxxxxx       13Cb EVOLUTION       xxxxxxxxxxxxxxxxxx    */

/* this pulse begins CB evolution */
decshaped_pulse(shCACB90, pwCACB90,one, 0.0, 0.0);


if (CT_c[0]=='n') 
   {
         
         if(ni>0){ delay( 2.0*(tau1)); };

         

/*This piece of ... code was neccessary to eliminate residual 13C phase distortions
due to imperfections in selective pulses, especially 90 on CACB */
   
  if(highSN[A]=='y')
  	{  
 		if(dm3[B] == 'y')  {                     
		                    setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);                  
					 if(1.0/dmf3>900.0e-6) {
							     dec3power(dpwr3+6.0);
							     dec3rgpulse(0.5/dmf3, three, 1.0e-6, 0.0e-6);
							     dec3power(dpwr3);
							     }
						else dec3rgpulse(1.0/dmf3, three, 1.0e-6, 0.0e-6);
				      dec3blank(); delay(PRG_START_DELAY);
			   	     }

		if(h1dec[A]=='y') {   obsprgoff();   xmtroff();
		                      rgpulse(pwHd,three,1.0e-6,0.0e-6);
				      delay(PRG_START_DELAY);
				    }

		delay(2.0e-6);	zgradpulse(gzlvl7,gt7); decphase(t4);   delay(gstab); 
         }		
        decpower(pwCACB180pwr);
	decshaped_pulse(shCACB180, pwCACB180,t4, 0.0, 0.0); 
        decpower(pwCACB90pwr);

   if(highSN[A]=='y') /* filter ON */
  	{   
		delay(2.0e-6); zgradpulse(gzlvl7,gt7); decphase(t1);   delay(gstab);

		if (h1dec[B]=='y') {  
          			     txphase(one);  rgpulse(pwHd,one,1.0e-6,0.0e-6); txphase(zero); 
	          	             xmtron();   obsprgon("waltz16", pwHd, 90.0);                   
  	          	             delay(PRG_STOP_DELAY);}

		if(dm3[B] == 'y'){  if(1.0/dmf3>900.0e-6) {
						     dec3power(dpwr3+6.0);
						     dec3rgpulse(0.5/dmf3, one, 1.0e-6, 0.0e-6);
						     dec3power(dpwr3);
						     }
			   else dec3rgpulse(1.0/dmf3, one, 1.0e-6,0.0e-6);
  			   dec3phase(zero);     dec3unblank();  setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
	                   delay(PRG_STOP_DELAY);}	
         }

 
  }

  /* END of REAL-TIME 13C EVOLUTION OPTION*/
  
  
  

/***13C constant-time with 15N and CO decoupling for glycines */

if (CT_c[0]=='y') 
   {
         decpower(pwCO180pwr);
	 decphase(zero);
	 delay(1.0e-6);
	 decshaped_pulse(shCO180offCACB, pwCO180,zero, 0.0, 0.0); /*ghost pulse */
         
	 
	 
         delay(tauC-tau1 -pwCACB180*0.5 -pwCO180 -gt7 -gstab -1.0/dmf3 -pwHd -1.0e-6
			-WFG_START_DELAY-WFG_STOP_DELAY);
         
/*This piece of ... code was neccessary to eliminate residual 13C phase distortions
due to imperfections in selective pulses, especially 90 on CACB
essentially  gradient-180onCACB-gradient */

	if(dm3[B] == 'y')  {                     /*optional 2H decoupling off */
	                    setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
	                    dec3rgpulse(1.0/dmf3, three, 1.0e-6, 0.0e-6); dec3blank();
	                    delay(PRG_START_DELAY);}
			    else {delay(1.0/dmf3);}

	if(h1dec[B]=='y') {   obsprgoff();   xmtroff();
	                      rgpulse(pwHd,three,1.0e-6,0.0e-6);
			      delay(PRG_START_DELAY);}
 			  else {delay(pwHd);}

	delay(2.0e-6);	zgradpulse(gzlvl7,gt7); decphase(t4);
	decpower(pwCACB180pwr); delay(gstab);

	decshaped_pulse(shCACB180, pwCACB180,t4, 0.0, 0.0);

	delay(2.0e-6); zgradpulse(gzlvl7,gt7); decphase(t1); 
        decpower(pwCO180pwr); delay(gstab);

	if (h1dec[B]=='y') { /*optional 1H decoupling on */
          		    txphase(one);  rgpulse(pwHd,one,1.0e-6,0.0e-6); txphase(zero); 
	                    xmtron();   obsprgon("waltz16", pwHd, 90.0);                   
 	                    delay(PRG_STOP_DELAY);}
                            else {delay(pwHd);}
	
	if(dm3[B] == 'y'){ /*optional 2H decoupling on */
			   dec3rgpulse(1.0/dmf3, one, 1.0e-6,0.0e-6); dec3phase(zero);   
                           dec3unblank();  setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
	                    delay(PRG_STOP_DELAY);}
			     else {delay(1.0/dmf3);}	



	 
	 	 
         delay(tauC - pwCACB180*0.5  -pwCO180 -gt7 -gstab -1.0/dmf3 -pwHd 
		-WFG3_START_DELAY-WFG3_STOP_DELAY); 
/*end of gradient-180onCACB-gradient */	 

	 sim3shaped_pulse("",shCO180offCACB,"",0.0, pwCO180, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
         
	 decphase(t1);
	 decpower(pwCACB90pwr);
	 delay(tau1);
	 
	 
  }

/*********BACK CB->CA  *******/
 

decshaped_pulse(shCACB90, pwCACB90,t1, 0.0, 0.0);
delay(2.0e-6);
decphase(t6);
decpower(pwCACB180pwr);
delay(tauCC);


decshaped_pulse(shCACB180, pwCACB180,t6, 0.0, 0.0);

delay(2.0e-6);
decphase(t3);
decpower(pwCACB90pwr);
delay(tauCC);
decshaped_pulse(shCACB90, pwCACB90, t3, 0.0, 0.0);
/* WE ARE ON NzCOzCAz NOW */





/***PURGE ****/

delay(1.0e-6);

/***  h1 and H2 decs OFF */
 	if(dm3[B] == 'y')  {                     
	                    setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
                             

                             if(1.0/dmf3>900.0e-6) {
						     dec3power(dpwr3+6.0);
						     dec3rgpulse(0.5/dmf3, three, 1.0e-6, 0.0e-6);
						     dec3power(dpwr3);
						     }

	                    else dec3rgpulse(1.0/dmf3, three, 1.0e-6, 0.0e-6);	    dec3blank();  
			   }

	if(h1dec[B]=='y') {   obsprgoff();   xmtroff();
	                      rgpulse(pwHd,three,1.0e-6,0.0e-6);
		        	}
 

delay(2.0e-6);
zgradpulse(gzlvl8, gt8);
delay(gstab);


/* carrier on CO again */
decoffset(dofCO);
decphase(one);
decpower(pwCO90pwr);

decshaped_pulse(shCO90, pwCO90, one, 0.0, 0.0);

delay(2.0e-6);
decphase(zero);
decpower(pwClvl);
delay(zeta - 0.5*pwCBIP -pwCO90*pwCO90_phase_roll -POWER_DELAY);

 decshaped_pulse(shCBIP, pwCBIP, zero, 0.0, 0.0);

delay(2.0e-6);
decphase(one);
delay(zeta - 0.5*pwCBIP);

delay(pwCO90*pwCO90_phase_roll + POWER_DELAY + 5.0e-6 + WFG_STOP_DELAY+ WFG_START_DELAY); /*phase roll compensation */
decshaped_pulse(shCBIP, pwCBIP, zero, 0.0, 0.0); 
decpower(pwCO90pwr);
delay(5.0e-6);
                                                           
decshaped_pulse(shCO90, pwCO90, zero, 0.0, 0.0);


/*purge and prepare stuff for 15N-CO back */
/*decs off */
decoffset(dof); /*back to cacb -  we use BIP for CO transfer and CACB180 to decouple from CA */
 decpower(pwCACB180pwr);
 
dec2phase(t2);
txphase(one);
decphase(zero);
obspower(tpwrd);


zgradpulse(gzlvl4, gt4);
delay(gstab);

 
 
 if (TROSY[A]=='n'){ rgpulse(pwHd,one,1.0e-6,0.0);
    			txphase(zero);	xmtron();
 			 obsprgon("waltz16", pwHd, 90.0);
                    }
 
 
/********************* NOW N t2 evolution together with COzNxy->Nxy ***/
          dec2rgpulse(pwN, t2, 0.0, 0.0);
          

          delay((timeTN-tau2)*0.5  - pwCACB180*0.5); 
	         
          decshaped_pulse(shCACB180, pwCACB180, zero, 0.0, 0.0);
          decpower(pwClvl);  dec2phase(zero);   
	  
          delay((timeTN-tau2)*0.5  - pwCACB180*0.5 -POWER_DELAY);
          
         /* Avoid simultaneous full-power C13 and 15N pulses, so they are sequential */
	           
	  
/*180 on  N15*/	 dec2rgpulse(pwN*2.0, zero, 0.0, 0.0);    
	  
	      
          delay(1.0e-6);
	  
          decshaped_pulse(shCBIP, pwCBIP, zero, 0.0, 0.0);
          decpower(pwCACB180pwr);
          
          delay((timeTN+tau2)*0.5-pwCBIP -pwCACB180*0.5 
 		-WFG_STOP_DELAY -WFG_START_DELAY-1.0e-6 -POWER_DELAY); 
	   
          decshaped_pulse(shCACB180, pwCACB180, zero, 0.0, 0.0);
          
          
           delay((timeTN+tau2)*0.5 - pwCACB180*0.5 -kappa);
	   
	   if (TROSY[A]=='n'){ obsprgoff();   xmtroff();   txphase(three); 
                               rgpulse(pwHd,three,1.0e-6,0.0);}
        	         else{delay(PRG_STOP_DELAY+pwHd+1.0e-6);};		    
	  
           
	   
	   delay(kappa - gt1 -gstab - 2.0*GRADIENT_DELAY
	           -PRG_STOP_DELAY -pwHd -2.0e-6 -pwHs -2.0*POWER_DELAY);
		   
	    
	   if (TROSY[A]=='n'){delay(pwHs+ 1.0e-6 +POWER_DELAY);};
	   
	   zgradpulse(gzlvl1, gt1); /*coding 15N */
	  
           
	   if (TROSY[A]=='y'){ 
	                       delay(gstab + 0.64*pwN-pw);obspower(tpwrs); txphase(t7);
           		       shaped_pulse("H2Osinc", pwHs,t7, 1.0e-6, 0.0);}
	   		     else  delay(gstab);
			     
          obspower(tpwr); txphase(t8);
	  
/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
	 
/* 90*/	if ( TROSY[A]=='n'){sim3pulse(pw, 0.0, pwN, t8, zero, t10,  1.0e-6, 0.0);}
                       else{sim3pulse(pw, 0.0, 0.0, t8, zero, zero, 1.0e-6, 0.0); };
		       
	 
	 txphase(t12); dec2phase(zero);
	 delay(gstab); zgradpulse(gzlvl5, gt5);	
	  
        
          
	  delay(lambda - gt5 -gstab);

/*180*/	 sim3pulse(2.0*pw, 0.0, 2.0*pwN, t12, zero, zero, 0.0, 0.0);
	 
	txphase(t13); dec2phase(t14);		 	 
	delay(lambda - gt5 - gstab); zgradpulse(gzlvl5, gt5);  delay(gstab);
	
/* 90*/	sim3pulse(pw, 0.0, pwN, t13, zero, t14, 0.0, 0.0); 
	
	txphase(zero); dec2phase(zero);   
	 
        delay(gstab); zgradpulse(gzlvl6, gt5); delay(lambda  - gt5  -gstab);
	
/*180*/	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
	
	 dec2phase(t9);
        
         delay(lambda  - gt5 -gstab);
		  		   
         zgradpulse(gzlvl6, gt5);	delay(gstab);
	 	 			 
	
/* 90*/	if ( TROSY[A]=='n'){rgpulse(pw, zero, 0.0, 0.0); }
	               else{dec2rgpulse(pwN, t9, 0.0, 0.0);}; 
		       
		       
	  
        txphase(zero);
	delay((gt1/10.0) + gstab  + 2.0*GRADIENT_DELAY + POWER_DELAY);

	rgpulse(2.0*pw, zero, 0.0, rof1);
	dec2power(dpwr2);	   /* POWER_DELAY */
	if (TROSY[A]=='y')  delay( pwN - 0.64*pw); 
                   
        zgradpulse(icosel*gzlvl2, gt1/10.0);
        statusdelay(C,gstab- rof1); rcvron();
   if (dm3[B]=='y') lk_sample();
      
	setreceiver(t16);
}		 
