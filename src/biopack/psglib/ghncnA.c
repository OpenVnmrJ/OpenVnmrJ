/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ghncnA.c  auto-calibrated version of 3D gradient sensitivity-enhanced  HN(C)N

    t1 evolution is not limited by kappa

    Correlates N(i) with N(i-1), NH(i-1).  Uses constant time evolution for the
    15N shifts.

    Standard features include square pulses on Cab with first null at 13CO; one
    lobe sinc pulses on 13CO with first null at Ca; shaka6 composite 180 pulse
    to simultaneously refocus CO and invert Ca; one lobe sinc pulse to put H2O
    back along z (the sinc one-lobe is significantly more selective than
    gaussian, square, or seduce 90 pulses); optional 2H decoupling when CaCb 
    magnetization is transverse for 4 channel spectrometers.  

    Magic-angle option for coherence transfer gradients.  TROSY option for
    N15/H1 evolution/detection.
 
        	  CHOICE OF DECOUPLING AND 2D MODES

    	Set dm = 'nnn', dmm = 'ccc' 
    	Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.
	Set dm3 = 'nnn' for no 2H decoupling, or
		  'nyn'  and dmm3 = 'cwc' for 2H decoupling. 
  
    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [15N]  and t2 [15N].
    
    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in 13C and f2180='n' for (0,0) in 15N.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.

    Sequence contributed by S.C.Panchal, N.S.Bhavesh and R.V.Hosur, Tata Institute,
      Mumbai, India (March 2002). See J.Biomol.NMR, 20, 135-147(2001).

    The waltz16 field strength is enterable (waltzB1).
    Typical values would be ~6-8ppm, (3500-5000 Hz at 600 MHz).
  
    Auto-calibrated version, E.Kupce, 27.08.2002.

          	  DETAILED INSTRUCTIONS FOR USE OF ghncn


    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for ghncn may be printed using:
                                      "printon man('ghncn') printoff".

    2. Apply the setup macro "ghncn". This loads the relevant parameter set
       and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral check.
       At the middle of the t1 period, the 180 degree pulses on CO and 15N are
       swapped to a 180 degree pulse on Ca, for the first increment of t1, to
       refocus Ca chemical-shift evolution ensuring a zero first-order phase
       correction in F1. This is also the case for the 1D spectral check, or
       for 2D/15N spectra, when ni=0.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 56ppm, and N15 
       frequency on the amide region (120 ppm).  The C13 frequency is shifted to
       the CO region during the sequence, but phase coherence is not required 
       after each shift.

    4. H2O preservation is achieved according to Kay et al, except that a sinc
       one-lobe selective pulse is used to put H2O back along z.  This is much
       more selective than a hard, Seduce1, or gaussian pulse.

    5. tauC (4.5 ms) and timeTN (12.5 ms) were determined for alphalytic 
       protease and are listed in dg2 for possible readjustment by the user.

    6. TROSY:
       Set TROSY='y' and dm2='nnn' for a TROSY spectrum of the bottom right
       peak of the 2D coupled NH quartet (high-field H1, low-field N15).  The 
       TROSY spectrum gives 50% S/N compared to the decoupled spectrum for a 
       small peptide.  To select any of the other three peaks of the 2D coupled
       quartet, in a clockwise direction from bottom right, change t4/t10
       from x/y to x/-y to -x/-y to -x/y.  NOTE, the phases of the SE train
       are almost the same as those determined for the gNhsqc sequence. 
       For these sequences H2O flipback is 
       achieved with two sinc one-lobe pulses, an additional one just before
       the SE train, similar to gNhsqc.

    7. The autocal and checkofs flags are generated automatically in Pbox_bio.h
       If these flags do not exist in the parameter set, they are automatically 
       set to 'y' - yes. In order to change their default values, create the  
       flag(s) in your parameter set and change them as required. 
       The available options for the checkofs flag are: 'y' (yes) and 'n' (no). 
       The offset (tof, dof, dof2 and dof3) checks can be switched off  
       individually by setting the corresponding argument to zero (0.0).
       For the autocal flag the available options are: 'y' (yes - by default), 
       'q' (quiet mode - suppress Pbox output), 'r' (read from file, no new  
       shapes are created), 's' (semi-automatic mode - allows access to user  
       defined parameters) and 'n' (no - use full manual setup, equivalent to 
       the original code).
*/



#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */

static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
                      phx[1]={0},   phy[1]={1},

	     phi3[4]  = {2,2,0,0},
	     phi5[4]  = {0,2,2,0},
	     phi6[4]  = {0,2,2,0},
             phi9[8]  = {0,0,0,0,2,2,2,2},
             rec[4]   = {0,0,2,2},		     recT[4]  = {3,1,1,3};

static double   d2_init=0.0, d3_init=0.0;
static double   H1ofs=4.75, C13ofs=56.0, N15ofs=120.0, H2ofs=0.0;

static shape    H2Osinc, wz16, offC1, offC2, offC3, offC6, offC8, offC9;

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
	    ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
            timeCN = getval("timeCN"),    /* time for CN transfer */  
	    tauC = getval("tauC"), 	      /* delay for CO to Ca evolution */
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
	    kappa = 5.4e-3,
	    lambda = 2.4e-3,
      bw, ofs, ppm,     /* bandwidth, offset, ppm - temporary Pbox parameters */      
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	rf0,            	  /* maximum fine power when using pwC pulses */

/* 90 degree pulse at Ca (56ppm), first off-resonance null at CO (174ppm)     */
        pwC1,		              /* 90 degree pulse length on C13 at rf1 */
        rf1,		       /* fine power for 4.7 kHz rf for 600MHz magnet */

/* 180 degree pulse at Ca (56ppm), first off-resonance null at CO(174ppm)     */
        pwC2,		                    /* 180 degree pulse length at rf2 */
        rf2,		      /* fine power for 10.5 kHz rf for 600MHz magnet */

/* the following pulse lengths for SLP pulses are automatically calculated    */
/* by the macro "biocal".  SLP pulse shapes, "offC6" etc are called     */
/* directly from your shapelib.                    			      */
   pwC3,                   /*180 degree pulse at Ca(56ppm) null at CO(174ppm) */
   pwC6,                      /* 90 degree selective sinc pulse on CO(174ppm) */
   pwC8,                     /* 180 degree selective sinc pulse on CO(174ppm) */
   pwC9,                     /* 180 degree selective sinc pulse on CO(174ppm) */
   phshift9,             /* phase shift induced on Ca by pwC9 ("offC9") pulse */
   pwZ,					   /* the largest of pwC9 and 2.0*pwN */
   pwZ1,                /* the larger of pwC9a and 2.0*pwN for 1D experiments */
   rf3,	                           /* fine power for the pwC3 ("offC3") pulse */
   rf6,	                           /* fine power for the pwC6 ("offC6") pulse */
   rf8,	                           /* fine power for the pwC8 ("offC8") pulse */
   rf9,	                           /* fine power for the pwC9 ("offC9") pulse */

   dofCO,			       /* channel 2 offset for most CO pulses */
	
   compH = getval("compH"),       /* adjustment for C13 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */
      tpwrsf = getval("tpwrsf"),
      pwr_dly = 0.0,
   	pwHd,	    		        /* H1 90 degree pulse length at tpwrd */
   	tpwrd,	  	                   /* rf for WALTZ decoupling */
        waltzB1 = getval("waltzB1"),  /* waltz16 field strength (in Hz)     */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
        gzcal  = getval("gzcal"),            /* g/cm to DAC conversion factor */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),
	gstab = getval("gstab"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gt7 = getval("gt7"),
	gt9 = getval("gt9"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6"),
	gzlvl7 = getval("gzlvl7"),
	gzlvl9 = getval("gzlvl9");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("mag_flg",mag_flg);
    getstr("TROSY",TROSY);



/*   LOAD PHASE TABLE    */
     
	settable(t3,4,phi3);
	settable(t4,1,phx);
	settable(t5,4,phi5);
	settable(t6,4,phi6);
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

    if( dpwrf < 4095 )
	{ printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
	  psg_abort(1); }

    /* offset during CO pulses, except for t1 evolution period */	
	dofCO = dof + 118.0*dfrq;

    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;

      setautocal();                        /* activate auto-calibration flags */ 

        
      if (autocal[0] == 'n') 
      {
    /* 90 degree pulse on Ca, null at CO 118ppm away */
        pwC1 = sqrt(15.0)/(4.0*118.0*dfrq);
        rf1 = 4095.0*(compC*pwC/pwC1);
        rf1 = (int) (rf1 + 0.5);

    /* 180 degree pulse on Ca, null at CO 118ppm away */
        pwC2 = sqrt(3.0)/(2.0*118.0*dfrq);
	rf2 = (compC*4095.0*pwC*2.0)/pwC2;
        rf2 = (int) (rf2 + 0.5);
        if( rf2 > 4095.0 )

       { printf("increase pwClvl so that C13 90 < 24us*(600/sfrq)"); psg_abort(1);}
    /* 180 degree pulse on Ca, null at CO 118ppm away */
        pwC3 = getval("pwC3");
	rf3 = rf2;
	rf3 = (int) (rf3 + 0.5);

    /* 90 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */
        pwC6 = getval("pwC6");
	rf6 = (compC*4095.0*pwC*1.69)/pwC6;	 /* needs 1.69 times more     */
	rf6 = (int) (rf6 + 0.5);		 /* power than a square pulse */

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */
        pwC8 = getval("pwC8");
	rf8 = (compC*4095.0*pwC*2.0*1.65)/pwC8;	 /* needs 1.65 times more     */
	rf8 = (int) (rf8 + 0.5);		 /* power than a square pulse */

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */
        pwC9 = getval("pwC9");
	rf9 = (compC*4095.0*pwC*2.0*1.65)/pwC9;	 /* needs 1.65 times more     */
	rf9 = (int) (rf9 + 0.5);		 /* power than a square pulse */
	
    /* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69)); /* needs 1.69 times more */
    tpwrs = (int) (tpwrs);                       /* power than a square pulse */

    /* power level and pulse time for WALTZ 1H decoupling */
	pwHd = 1/(4.0 * waltzB1) ;                          
	tpwrd = tpwr - 20.0*log10(pwHd/(compH*pw));
	tpwrd = (int) (tpwrd + 0.5);
      }
      else       /* if autocal = 'y'(yes), 'q'(quiet), 'r'(read) or 's'(semi) */
      {
        if(FIRST_FID)                                            /* call Pbox */
        {
          ppm = getval("dfrq"); 
          bw = 118.0*ppm; ofs = -bw;           
          offC1 = pbox_Rcal("square90n", bw, compC*pwC, pwClvl);
          offC2 = pbox_Rcal("square180n", bw, compC*pwC, pwClvl);
          offC3 = pbox_make("offC3", "square180n", bw, ofs, compC*pwC, pwClvl);
          offC6 = pbox_make("offC6", "sinc90n", bw, 0.0, compC*pwC, pwClvl);
          offC8 = pbox_make("offC8", "sinc180n", bw, 0.0, compC*pwC, pwClvl);
          offC9 = pbox_make("offC9", "sinc180n", bw, -ofs, compC*pwC, pwClvl);
          H2Osinc = pbox_Rsh("H2Osinc", "sinc90", pwHs, 0.0, compH*pw, tpwr);
          wz16 = pbox_Dcal("WALTZ16", 2.8*waltzB1, 0.0, compH*pw, tpwr);

          if (dm3[B] == 'y') H2ofs = 3.2;     
          ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
        }
        pwC1 = offC1.pw; rf1 = offC1.pwrf;
        pwC2 = offC2.pw; rf2 = offC2.pwrf;
        pwC3 = offC3.pw; rf3 = offC3.pwrf;
        pwC6 = offC6.pw; rf6 = offC6.pwrf;
        pwC8 = offC8.pw; rf8 = offC8.pwrf;
        pwC9 = offC9.pw; rf9 = offC9.pwrf;
        pwHs = H2Osinc.pw; tpwrs = H2Osinc.pwr-1.0;  /* 1dB correction applied */         
        tpwrd = wz16.pwr; pwHd = 1.0/wz16.dmf;

   /* Example of semi-automatic calibration - use parameters, if they exist : 

        if ((autocal[0] == 's') || (autocal[1] == 's'))       
        { 
          if (find("pwC1") > 0) pwC1 = getval("pwC1");
          if (find("rf1") > 0) rf1 = getval("rf1");
        }
   */
      }	
      if (tpwrsf < 4095.0) 
      {
        pwr_dly = POWER_DELAY + PWRF_DELAY;
        tpwrs = tpwrs + 6.0;
      }
      else pwr_dly = POWER_DELAY;

    /* the pwC9 pulse at the middle of t1  */
        if ((ni2 > 0.0) && (ni == 1.0)) ni = 0.0;
        if (pwC8 > 2.0*pwN) pwZ = pwC8; else pwZ = 2.0*pwN;
        if ((pwC9==0.0) && (pwC8>2.0*pwN)) pwZ1=pwC8-2.0*pwN; else pwZ1=0.0;
	if ( ni > 1 )     pwC9 = pwC8;
	if ( pwC9 > 0 )   phshift9 = 140.0;
	else              phshift9 = 0.0;
	
/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( tauC < (gt7+1.0e-4+0.5*10.933*pwC))  gt7=(tauC-1.0e-4-0.5*10.933*pwC);

    if ( 0.5*ni2*1/(sw2) > timeTN - WFG3_START_DELAY)
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n", 
  	 ((int)((timeTN - WFG3_START_DELAY)*2.0*sw2))); psg_abort(1);}

    if ( 0.5*ni*1/(sw1) > timeTN - pwC3 - WFG_START_DELAY)
       { printf(" ni is too big. Make ni equal to %d or less.\n", 
  	 ((int)((timeTN - pwC3- WFG_START_DELAY)*2.0*sw1))); psg_abort(1);} 

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
        if (dm3[B] == 'y') lk_hold();
 
	rcvroff();
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	decpwrf(rf0);
	obsoffset(tof);
	decoffset(dofCO);
	txphase(zero);
   	delay(1.0e-5);

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(-gzlvl0, 0.5e-3);
	delay(1.0e-4);
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
    obspower(tpwrs);
    if (tpwrsf < 4095.0) obspwrf(tpwrsf);
if (TROSY[A]=='y')
   {txphase(two);
    shaped_pulse("H2Osinc", pwHs, two, 5.0e-4, 0.0);
    obspower(tpwr);
    if (tpwrsf < 4095.0) obspwrf(4095.0);
    zgradpulse(gzlvl3, gt3);
    delay(2.0e-4);
    dec2rgpulse(pwN, zero, 0.0, 0.0);

    delay(0.5*kappa - 2.0*pw);

    rgpulse(2.0*pw, two, 0.0, 0.0);

    obspower(tpwrd);	  				       /* POWER_DELAY */
    decphase(zero);
    dec2phase(zero);
    decpwrf(rf8);
    delay(timeTN -0.5*kappa - POWER_DELAY - WFG3_START_DELAY);
   }
else
   {txphase(zero);
    shaped_pulse("H2Osinc",pwHs,zero,5.0e-4,0.0);
    obspower(tpwrd);
    if (tpwrsf < 4095.0) obspwrf(4095.0);
    dec2phase(t3);
    zgradpulse(gzlvl3, gt3);
    txphase(one);
    delay(2.0e-4);
    dec2rgpulse(pwN, t3, 0.0, 0.0);
    }

/* modification starts from here */
       decpwrf(rf3);
       decphase(zero);
       

 
/* if (TROSY[A]=='y')
{    if (tau1 > gt3 + 2.0*GRADIENT_DELAY + 1.5e-4 + pwHs)
        {
          txphase(three);
          delay(timeTN - pwC3 - WFG_START_DELAY);         WFG_START_DELAY  
          sim3shaped_pulse("","offC8","",0.0, pwC8, 2.0*pwN,zero,zero,zero, 0.0, 0.0);
          delay(tau1 - gt1 - 2.0*GRADIENT_DELAY - 1.5e-4 - pwHs);
          obspower(tpwrs);                                      POWER_DELAY 
          delay(1.0e-4 - POWER_DELAY);
          shaped_pulse("H2Osinc", pwHs, three, 0.0, 0.0);
          txphase(t4);
          obspower(tpwr);                                       POWER_DELAY 
          delay(0.5e-4 - POWER_DELAY);
        }

    else if (tau1 > pwHs + 0.5e-4)
        {
          txphase(three);
          delay(timeTN-pwC3-WFG_START_DELAY-gt1-2.0*GRADIENT_DELAY-1.0e-4);
          obspower(tpwrs);                                      POWER_DELAY 
          delay(1.0e-4 - POWER_DELAY);                      WFG_START_DELAY 
          sim3shaped_pulse("","offC8","",0.0, pwC8, 2.0*pwN,zero,zero,zero, 0.0, 0.0);
          delay(tau1 - pwHs - 0.5e-4);
          shaped_pulse("H2Osinc", pwHs, three, 0.0, 0.0);
          txphase(t4);
          obspower(tpwr);                                       POWER_DELAY 
          delay(0.5e-4 - POWER_DELAY);
        }
    else
        {
          txphase(three);
          delay(timeTN - pwC3 - WFG_START_DELAY - gt1 - 2.0*GRADIENT_DELAY
                                                            - 1.5e-4 - pwHs);
          obspower(tpwrs);                                      POWER_DELAY 
          delay(1.0e-4 - POWER_DELAY);                      WFG_START_DELAY 
          shaped_pulse("H2Osinc", pwHs, three, 0.0, 0.0);
          txphase(t4);
          obspower(tpwr);                                       POWER_DELAY 
          delay(0.5e-4 - POWER_DELAY);
          sim3shaped_pulse("","offC8","",0.0, pwC8, 2.0*pwN,zero,zero,zero, 0.0, 0.0);
          delay(tau1);
        }
}
else
{ */
    if (tau1 > kappa)
	{
          txphase(one);
          delay(kappa);     	   /* WFG_START_DELAY */
          rgpulse(pwHd,one,0.0,0.0);
          txphase(zero);
          delay(2.0e-6);
          obsprgon("waltz16",pwHd,90.0);
          xmtron();
          delay( tau1 - kappa - WFG3_START_DELAY - pwHd - 2.0e-6);
          decshaped_pulse("offC3",pwC3,zero, 0.0, 0.0);
          decphase(zero);
          dec2phase(zero);
          decpwrf(rf8); 
          delay(timeTN - pwC3 - WFG_START_DELAY);
	}
    else if (tau1 == kappa )
	{
          txphase(one);
          delay( tau1- pwC3-WFG_START_DELAY);
          decshaped_pulse("offC3",pwC3,zero, 0.0, 0.0);
          rgpulse(pwHd,one,0.0,0.0);
          txphase(zero);
          delay(2.0e-6);
          obsprgon("waltz16",pwHd,90.0);
          xmtron();
          decphase(zero);
          dec2phase(zero);
          decpwrf(rf8); 
          delay(timeTN+tau1-kappa-WFG3_START_DELAY-pwHd-2.0e-6);
	}
    else
	{
          txphase(one);
          delay( tau1);
          decshaped_pulse("offC3",pwC3,zero, 0.0, 0.0);
          delay( kappa-tau1-pwC3-WFG_START_DELAY );
          rgpulse(pwHd,one,0.0,0.0);
          txphase(zero);
          delay(2.0e-6);
          obsprgon("waltz16",pwHd,90.0);
          xmtron();
          decphase(zero);
          dec2phase(zero);
          decpwrf(rf8); 
          delay(timeTN+tau1-kappa-WFG3_START_DELAY-pwHd-2.0e-6);
	}
        

        

	sim3shaped_pulse("", "offC8", "", 0.0, pwC8, 2.0*pwN, zero, zero, zero, 
								     0.0, 0.0);
        decphase(t6);
	decpwrf(rf6);
        dec2phase(t5);
	delay(timeTN - tau1);

/* t1 modification ends here */


if (TROSY[A]=='n')
   {xmtroff();
    obsprgoff();
    rgpulse(pwHd,three,2.0e-6,0.0);}

	dec2rgpulse(pwN, t5, 0.0, 0.0);
	zgradpulse(-gzlvl3, gt3);
 	delay(2.0e-4);
	decshaped_pulse("offC6", pwC6, t6, 0.0, 0.0);

	zgradpulse(-gzlvl7, gt7);
	decpwrf(rf0);
	decphase(zero);
	delay(tauC - gt7 - 0.5*10.933*pwC);

	decrgpulse(pwC*158.0/90.0, zero, 0.0, 0.0);
	decrgpulse(pwC*171.2/90.0, two, 0.0, 0.0);
	decrgpulse(pwC*342.8/90.0, zero, 0.0, 0.0);      /* Shaka 6 composite */
	decrgpulse(pwC*145.5/90.0, two, 0.0, 0.0);
	decrgpulse(pwC*81.2/90.0, zero, 0.0, 0.0);
	decrgpulse(pwC*85.3/90.0, two, 0.0, 0.0);

	zgradpulse(-gzlvl7, gt7);
	decpwrf(rf6);
	decphase(one);
	txphase(one);
        delay(tauC - gt7 - 0.5*10.933*pwC - WFG_START_DELAY);
							   /* WFG_START_DELAY */
	decshaped_pulse("offC6", pwC6, one, 0.0, 0.0);
	decoffset(dof);
	zgradpulse(-gzlvl9, gt9);
	decpwrf(rf1); 
        decphase(zero);
	/* decphase(t3); */
	delay(2.0e-4);
        if(dm3[B] == 'y')			  /*optional 2H decoupling on */
         {dec3unblank(); dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank(); setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);} 
        rgpulse(pwHd,one,0.0,0.0);
	txphase(zero);
 	delay(2.0e-6);
	obsprgon("waltz16", pwHd, 90.0);
	xmtron();


/*   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    */
/* C-alpha and 15N transfer starts from here */



        decrgpulse(pwC1,zero, 0.0, 0.0);
         /* delay(2.0e-6);
         zgradpulse(gzlvl3,gt3); */


        decpwrf(rf9);
        decphase(zero);
        delay(tauC - pwC9);
	decshaped_pulse("offC9", pwC9, zero, 0.0, 0.0);


	decpwrf(rf2);
	decphase(zero);
	delay(timeCN  - tauC -0.5*2.0*pwN);


        sim3pulse(0.0,pwC2,2.0*pwN, zero, zero, zero, 0.0,0.0);
	decpwrf(rf1);
	decphase(one);
	delay(timeCN -  0.5*2.0*pwN );



          


        decrgpulse(pwC1,one,0.0,0.0);
        decpwrf(rf1);
        decphase(t5);
      /*  delay(timeCN);

        zgradpulse(gzlvl4,gt4);
        delay(2.0e-4); 

        decrgpulse(pwC1,t5,0.0,0.0); */


/* c-alpha and N15 transfer ends here */
	/* decphase(t5);
	decpwrf(rf1);
	decrgpulse(pwC1, t5, 2.0e-6, 0.0); */


        xmtroff();
	obsprgoff();
        rgpulse(pwHd,three,2.0e-6,0.0);

/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */

	dec2phase(t8);
	 zgradpulse(gzlvl4, gt4); 
	txphase(one);
	dcplrphase(zero);
 	delay(2.0e-4);
        if (TROSY[A]=='n')
	   {rgpulse(pwHd,one,0.0,0.0);
	    txphase(zero);
	    delay(2.0e-6);
	    obsprgon("waltz16", pwHd, 90.0);
	    xmtron();}
	dec2rgpulse(pwN, t8, 0.0, 0.0);

        decphase(zero);

        dec2phase(t9);
        decphase(zero);
        decpwrf(rf2);
        delay(timeTN - WFG3_START_DELAY - tau2);
                                                         /* WFG3_START_DELAY  */
        sim3pulse(0.0, pwC2, 2.0*pwN, zero, zero, t9,
                                                                   0.0, 0.0);

        dec2phase(t10);
        decpwrf(rf9);

if (TROSY[A]=='y')
{    if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.5e-4 + pwHs)
        {
          txphase(three);
          delay(timeTN - pwC3 - WFG_START_DELAY);         /* WFG_START_DELAY */
          decshaped_pulse("offC8", pwC8, zero, 0.0, 0.0);
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.5e-4 - pwHs);
          if (mag_flg[A]=='y')  magradpulse(gzcal*gzlvl1, gt1);
          else  zgradpulse(gzlvl1, gt1);         /* 2.0*GRADIENT_DELAY */
          obspower(tpwrs);                                     /* POWER_DELAY */
          if (tpwrsf < 4095.0) obspwrf(tpwrsf);
          delay(1.0e-4 - pwr_dly);
          shaped_pulse("H2Osinc", pwHs, three, 0.0, 0.0);
          txphase(t4);
          obspower(tpwr);                                      /* POWER_DELAY */
          if (tpwrsf < 4095.0) obspwrf(4095.0);
          delay(0.5e-4 - pwr_dly);
        }

    else if (tau2 > pwHs + 0.5e-4)
        {
          txphase(three);
          delay(timeTN-pwC8-WFG_START_DELAY-gt1-2.0*GRADIENT_DELAY-1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else  zgradpulse(gzlvl1, gt1);         /* 2.0*GRADIENT_DELAY */
          obspower(tpwrs);                                     /* POWER_DELAY */
          if (tpwrsf < 4095.0) obspwrf(tpwrsf);
          delay(1.0e-4 - pwr_dly);                     /* WFG_START_DELAY */
          decshaped_pulse("offC8", pwC8, zero, 0.0, 0.0);
          delay(tau2 - pwHs - 0.5e-4);
          shaped_pulse("H2Osinc", pwHs, three, 0.0, 0.0);
          txphase(t4);
          obspower(tpwr);                                      /* POWER_DELAY */
          if (tpwrsf < 4095.0) obspwrf(4095.0);
          delay(0.5e-4 - pwr_dly);
        }
    else
        {
          txphase(three);
          delay(timeTN - pwC8 - WFG_START_DELAY - gt1 - 2.0*GRADIENT_DELAY
                                                            - 1.5e-4 - pwHs);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else  zgradpulse(gzlvl1, gt1);         /* 2.0*GRADIENT_DELAY */
          obspower(tpwrs);                                     /* POWER_DELAY */
          if (tpwrsf < 4095.0) obspwrf(tpwrsf);
          delay(1.0e-4 - pwr_dly);                     /* WFG_START_DELAY */
          shaped_pulse("H2Osinc", pwHs, three, 0.0, 0.0);
          txphase(t4);
          obspower(tpwr);                                      /* POWER_DELAY */
          if (tpwrsf < 4095.0) obspwrf(4095.0);
          delay(0.5e-4 - pwr_dly);
          decshaped_pulse("offC8", pwC8, zero, 0.0, 0.0);
          delay(tau2);
        }
}
else
{
    if (tau2 > kappa)
	{
          delay(timeTN - pwC9 - WFG_START_DELAY);     	   /* WFG_START_DELAY */
          decshaped_pulse("offC9", pwC9, zero, 0.0, 0.0);
          delay(tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  rgpulse(pwHd,three,2.0e-6,0.0);
	  txphase(t4);
          delay(kappa - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - POWER_DELAY);
	}
    else if (tau2 > (kappa - pwC9 - WFG_START_DELAY))
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  rgpulse(pwHd,three,2.0e-6,0.0);
	  txphase(t4);                                     /* WFG_START_DELAY */
          decshaped_pulse("offC9", pwC9, zero, 0.0, 0.0);
          delay(kappa -pwC9 -WFG_START_DELAY -gt1 -2.0*GRADIENT_DELAY -1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - POWER_DELAY);
	}
    else if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  rgpulse(pwHd,three,2.0e-6,0.0);
	  txphase(t4);
          delay(kappa - tau2 - pwC9 - WFG_START_DELAY);   /* WFG_START_DELAY */
          decshaped_pulse("offC9", pwC9, zero, 0.0, 0.0);
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - POWER_DELAY);
	}
    else
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6);
          xmtroff();
	  obsprgoff();					    /* PRG_STOP_DELAY */
	  rgpulse(pwHd,three,2.0e-6,0.0);
	  txphase(t4);
    	  delay(kappa-tau2-pwC9-WFG_START_DELAY-gt1-2.0*GRADIENT_DELAY-1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - POWER_DELAY);                    /* WFG_START_DELAY */
          decshaped_pulse("offC9", pwC9, zero, 0.0, 0.0);
          delay(tau2);
	}
}
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

	delay((gt1/10.0) + gstab - 0.5*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

	rgpulse(2.0*pw, zero, 0.0, rof1);
	dec2power(dpwr2);				       /* POWER_DELAY */
        if (mag_flg[A] == 'y')    magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);
        else   zgradpulse(icosel*gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */

        rcvron();
statusdelay(C,gstab - rof1);
   if (dm3[B]=='y') lk_sample();

	setreceiver(t12);
}		 
