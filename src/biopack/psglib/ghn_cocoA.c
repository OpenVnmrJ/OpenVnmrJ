/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghn_cocoA.c

    3D (HN)CO(CO)NH gradient sensitivity enhanced version.


    Standard features include maintaining the 13C carrier in the CO region
    throughout using off-res SLP pulses; square pulses on Ca with first
    null at 13CO; one lobe sinc pulses on 13CO with first null at Ca; one lobe
    sinc pulse to put H2O back along z (the sinc one-lobe is significantly more
    selective than gaussian, square, or seduce 90 pulses); preservation of H20
    along z during t1 and t2; waltz H1 decoupling during N15 evolution to
    decrease S/N loss via H1 exchange with H2O.  

    Magic-angle option for coherence transfer gradients.  TROSY option NOT implemented.
 
    pulse sequence: Bax, & Grzesiek, J. Biomol. NMR, 9, 207-211 (1997)            
    SLP pulses:     J Magn. Reson. 96, 94-102 (1992)
 
    The sequence is modified to do Kay type Gradient enhancement.
    Auto-calibrated version, E.Kupce, 27.08.2002.


        	  CHOICE OF DECOUPLING AND 2D MODES

    The waltz16 field strength is enterable (waltzB1).
    Typical values would be ~6-8ppm, (3500-5000 Hz at 600 MHz).
  
    	Set dm = 'nnn', dmm = 'ccc' 
    	Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [C13]  and t2 [N15].
    
    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in C13 and f2180='n' for (0,0) in N15.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.



          	  DETAILED INSTRUCTIONS FOR USE OF ghn_coco

         
    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for ghn_co may be printed using:
                                      "printon man('ghn_co') printoff".
             
    2. Apply the setup macro "ghn_coco".  This loads the relevant parameter set
       and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral check.
       At the middle of the t1 period, the 180 degree pulses on Ca and 15N are 
       swapped to a 180 degree pulse on CO, for the first increment of t1, to 
       refocus CO chemical-shift evolution ensuring a zero first-order phase
       correction in F1. This is also the case for the 1D spectral check, or 
       for 2D/15N spectra, when ni=0.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 174ppm, and N15 
       frequency on the amide region (120 ppm).  The C13 frequency remains at 
       174ppm, ie at CO throughout the sequence.

    4. The normal 13C 180 degree pulse on Ca at the middle of t1 induces a
       phase shift, which should be field-invariant, and so this phase shift has
       been calibrated and compensated in the pulse sequence. This phase shift 
       can be checked by setting ni=1 whereby a special 1D method is invoked
       in which both the 13C Ca 180 degree pulse and the simultaneous 15N 180
       degree pulse are applied just as for all t1 times other than t1=0.  First
       eliminate the Ca pulse by setting pwC3=0 and obtain a 1D spectrum. This
       spectrum will have reduced intensity compared to ni=0 because of 13Ca
       chemical-shift evolution during the time of the 180 pulses. If the 
       phase shift is adequately compensated, a second very similar 1D spectrum 
       will be obtained with pwC3=pwC3a. As in point 5, a more sensitive 
       comparison of the two spectra with pwC3=0,pwC3a can be obtained with
       phase=2.  If not adequately compensated, the first increment will be out
       of phase with all succeeding increments and a zero-order phase-shift will
       be necessary in F1, which is easily done after the 2D/3D transform.
       Alternatively, change the calibration by changing the phshift3 parameter
       in the INITIALIZE VARIABLES section of the code. The pulse pwC3 is
       automatically reset to its calibrated value (=pwC3a) within the pulse
       sequence code for 3D work and 2D/t1 studies.  DO NOT CHANGE pwC3a from
       its calibrated value.  NOTE THAT dof MUST BE ACCURATELY CALIBRATED (to 	
       1ppm) BEFORE THE phshift3 CALIBRATION.  S/N can also be maximized by
       arraying compC when ni=1;pwC3=pwC3a.

    5. dof may be calibrated accurately to the center of the CO region, 
       when ni=1, by using the 13CO chemical shift evolution during a period of
       (1/dfrq*80). Set ni=1 and pwC3=1 and obtain a well-phased absorption mode
       1D spectrum.  Now set phase=2 and obtain an array of spectra with dof
       varied by up to 20ppm either side of your best guess. dof will be correct
       when the integral from 7.4 to 9ppm in the 1H spectrum is close to zero -
       this spectrum will have the appearance of a broad "noisy" dispersion mode
       signal caused by some signals having a +ve shift and some a -ve shift
       relative to dof.  Ignore the NH2 signals at about 7.1ppm. If phase=2 is
       also to be used for the phshift3 check as in point 4, it is handy to set
       dof using this method first before resetting pwC3=pwC3a.  Note also that
       dof can be set with more sensitivity with ni=1;pwC3=pwC3a because there
       is a longer period of 13CO shift evolution. 

    6. A similar method can be used to calibrate dof2.  Set ni=ni2=0 and
       phase=phase2=1 and obtain a well=phased absorption mode 1D spectrum.  
       Set d3=0.0001 and phase2=1,2, sum the two resulting spectra and display
       with a 90 degree phase shift:
       clradd select(1) spadd select(2) spadd jexp5 full rp=rp+90 ds   
       dof2 will be in the middle of the NH region when the signals between
       7.4 and 9ppm in the 1H spectrum are roughly balanced between +ve and
       -ve signals as in point 5.  Note that the same method with d3=0 should 
       give a zero spectrum.  Don't forget to reset d3=0 when you have finished.

    7. H2O preservation is achieved according to Kay et al, except that a sinc
       one-lobe selective pulse is used to put H2O back along z.  This is much
       more selective than a hard, Seduce1, or gaussian pulse.

    8. Another difference from the work of Kay et al is that the phases of both
       CO 90 degree pulses are alternated to eliminate artifacts from the Ca
       180 degree pulse.

    9. timeTN (12 ms) was determined for alphalytic protease and is listed in 
       dg2 for possible readjustment by the user.

   10. The coherence-transfer gradients using power levels
       gzlvl1 and gzlvl2 may be either z or magic-angle gradients. For the
       latter, a proper /vnmr/imaging/gradtable entry must be present and
       syscoil must contain the value of this entry (name of gradtable). The
       amplitude of the gzlvl1 and gzlvl2 should be lower than for a z axis
       probe to have the x and y gradient levels within the 32k range. For
       any value, a dps display (using power display) shows the x,y and z
       dac values. These must be <=32k.

   11. Radiation Damping:
       At fields 600MHz and higher with high-Q probes, radiation damping is a
       factor to be considered. Its primary effect is in the flipback pulse
       calibration. Radiation damping causes a rotation back to the +Z axis
       even without a flipback pulse. Hence, the pwHs pulse often needs to 
       be reduced in its flip-angle. This can be accomplished by using the
       parameter tpwrsf. If this value is less than 4095.0 the value of tpwrs
       (calculated in the psg code) is increased by 6dB, thereby permitting
       the value of tpwrsf to be optimized to obtain minimum H2O in the 
       spectrum. The value of tpwrsf is typically lower than 2048 (half-maximum
       to compensate for the extra 6dB in tpwrs). 

   12. The autocal and checkofs flags are generated automatically in Pbox_bio.h
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

static int   phx[1]={0},   
	     phy[1]={1},
	     phi3[2]  = {0,2},
	     phi5[4]  = {0,0,2,2},
             phi9[8]  = {0,0,0,0,2,2,2,2},
             rec[4]   = {0,2,2,0};

static double   d2_init=0.0, d3_init=0.0;
static double   H1ofs=4.75, C13ofs=174.0, N15ofs=120.0, H2ofs=0.0;

static shape H2Osinc, wz16, offC3, offC6, offC8;

void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
 	    TROSY[MAXSTR];			    /* To check for TROSY flag */
 
int         icosel,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni2 = getval("ni2");

double      p_d,
	    rfd,
	    ncyc,
	    COmix = getval("COmix"),
	    p_trim,
	    rftrim,
	    tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
	    kappa = 5.4e-3,
	    lambda = 2.4e-3,
            
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	rf0,            	  /* maximum fine power when using pwC pulses */
      bw, ofs, ppm,  /* bandwidth, offset, ppm - temporary Pbox parameters */

/* the following pulse lengths for SLP pulses are automatically calculated    */
/* by the macro "biocal".  SLP pulse shapes, "offC3" etc are called       */
/* directly from your shapelib.                    			      */
   pwC3 = getval("pwC3"),  /*180 degree pulse at Ca(56ppm) null at CO(174ppm) */
   pwC3a,                      /* pwC3a=pwC3, but not set to zero when pwC3=0 */
   phshift3,             /* phase shift induced on CO by pwC3 ("offC3") pulse */
   pwZ,					   /* the largest of pwC3 and 2.0*pwN */
   pwZ1,	       /* the largest of pwC3a and 2.0*pwN for 1D experiments */
   pwC6,                  /* 90 degree selective sinc pulse on CO(174ppm) */
   pwC8,                 /* 180 degree selective sinc pulse on CO(174ppm) */
   rf3,	                           /* fine power for the pwC3 ("offC3") pulse */
   rf6,	                           /* fine power for the pwC6 ("offC6") pulse */
   rf8,	                           /* fine power for the pwC8 ("offC8") pulse */

   compH = getval("compH"),       /* adjustment for C13 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrsf = getval("tpwrsf"),    /* fine power adjustment for flipback   */
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */

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

	settable(t3,2,phi3);
	settable(t4,1,phx);
	settable(t5,4,phi5);

        settable(t8,1,phx);
	settable(t9,8,phi9);
	settable(t10,1,phx);
	settable(t11,1,phy);
	settable(t12,4,rec);


/*   INITIALIZE VARIABLES   */

    if( dpwrf < 4095 )
	{ printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
	  psg_abort(1); }

    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;

      setautocal();                      /* activate auto-calibration */   

      if (autocal[0] == 'n') 
      {
    /* offC3 - 180 degree pulse on Ca, null at CO 118ppm away */
        pwC3a = getval("pwC3a");    
        rf3 = (compC*4095.0*pwC*2.0)/pwC3a;
	  rf3 = (int) (rf3 + 0.5);  
	
    /* 90 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */	
        pwC6 = getval("pwC6");    
	  rf6 = (compC*4095.0*pwC*1.69)/pwC6;	/* needs 1.69 times more     */
	  rf6 = (int) (rf6 + 0.5);		/* power than a square pulse */

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */
        pwC8 = getval("pwC8");
	  rf8 = (compC*4095.0*pwC*2.0*1.65)/pwC8;	/* needs 1.65 times more     */
	  rf8 = (int) (rf8 + 0.5);		      /* power than a square pulse */

    /* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69)); /* needs 1.69 times more */
        tpwrs = (int) (tpwrs);                       /* power than a square pulse */

    /* power level and pulse time for WALTZ 1H decoupling */
	  pwHd = 1/(4.0 * waltzB1) ;                          
	  tpwrd = tpwr - 20.0*log10(pwHd/(compH*pw));
	  tpwrd = (int) (tpwrd + 0.5);
      }
      else      /* if autocal = 'y'(yes), 'q'(quiet), 'r'(read) or 's'(semi) */
      {
        if(FIRST_FID)                                         /* make shapes */
        {
          ppm = getval("dfrq"); 
          bw = 118.0*ppm; ofs = -bw; 
          offC3 = pbox_make("offC3", "square180n", bw, ofs, compC*pwC, pwClvl);
          offC6 = pbox_make("offC6", "sinc90n", bw, 0.0, compC*pwC, pwClvl);
          offC8 = pbox_make("offC8", "sinc180n", bw, 0.0, compC*pwC, pwClvl);
          H2Osinc = pbox_Rsh("H2Osinc", "sinc90", pwHs, 0.0, compH*pw, tpwr);
          wz16 = pbox_Dcal("WALTZ16", 2.8*waltzB1, 0.0, compH*pw, tpwr);


          ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
        }
        pwC3a = offC3.pw; rf3 = offC3.pwrf;             /* set up parameters */
        pwC6 = offC6.pw; rf6 = offC6.pwrf; 
        pwC8 = offC8.pw; rf8 = offC8.pwrf;
        pwHs = H2Osinc.pw; tpwrs = H2Osinc.pwr-1.0;  /* 1dB correction applied */
        tpwrd = wz16.pwr; pwHd = 1.0/wz16.dmf;  
      }

      if (tpwrsf < 4095.0) tpwrs = tpwrs + 6.0;

    /* the pwC3 pulse at the middle of t1  */
	if ((ni2 > 0.0) && (ni == 1.0)) ni = 0.0;
        if (pwC3a > 2.0*pwN) pwZ = pwC3a; else pwZ = 2.0*pwN;
        if ((pwC3==0.0) && (pwC3a>2.0*pwN)) pwZ1=pwC3a-2.0*pwN; else pwZ1=0.0;
	if ( ni > 1 )     pwC3 = pwC3a;
	if ( pwC3 > 0 )   phshift3 = 48.0;
	else              phshift3 = 0.0;

 
   /* dipsi-3 decoupling on COCO */
        p_trim = 1/(4*5000*(sfrq/600.0));  /* 5 kHz trim pulse at 600MHz as per Bax */
        p_d = (5.0)/(9.0*4.0*2800.0*(sfrq/600.0)); /* 2.8 kHz DIPSI-3 at 600MHz as per Bax*/
        rftrim = (compC*4095.0*pwC)/p_trim;
        rftrim = (int)(rftrim+0.5);
        rfd = (compC*4095.0*pwC*5.0)/(p_d*9.0);
        rfd = (int) (rfd + 0.5);
        ncyc = ((COmix - 0.002)/51.8/4/p_d);
        ncyc = (int) (ncyc + 0.5);
        initval(ncyc,v9);


/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( 0.5*ni2*1/(sw2) > timeTN - WFG3_START_DELAY)
       { printf(" ni2 is too big. Make ni2 equal to %d or less.\n", 
  	 ((int)((timeTN - WFG3_START_DELAY)*2.0*sw2))); psg_abort(1);}

    if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
       { printf("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' )
       { printf("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dpwr2 > 50 )
       { printf("dpwr2 too large! recheck value  "); psg_abort(1);}

    if ( pw > 50.0e-6 )
       { printf(" pw too long ! recheck value "); psg_abort(1);} 
  
    if ( (pwN > 100.0e-6) && (ni>1 || ni2>1))
       { printf(" pwN too long! recheck value "); psg_abort(1);} 
 
    if ( TROSY[A] == 'y')
      { printf(" TROSY option is not implemented"); psg_abort(1);}
      


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)   tsadd(t3,1,4);  
    if (phase2 == 2)  
    {tsadd(t10,2,4); icosel = +1;}
    else 			       
    icosel = -1;    


/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }


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
	rcvroff();
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	decpwrf(rf0);
	obsoffset(tof);
	txphase(zero);
   	delay(1.0e-5);

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

   	rgpulse(pw,zero,0.0,0.0);                      /* 1H pulse excitation */

   	dec2phase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	txphase(one);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

 	rgpulse(pw, one, 0.0, 0.0);
    txphase(zero);
    
    obspower(tpwrs); 
    if (tpwrsf<4095.0) obspwrf(tpwrsf);
    shaped_pulse("H2Osinc", pwHs, zero, 5.0e-4, 0.0);
    obspower(tpwrd); 
    if (tpwrsf<4095.0) obspwrf(4095.0);
    zgradpulse(gzlvl3, gt3);
    delay(2.0e-4);
    dec2rgpulse(pwN, zero, 0.0, 0.0);

    txphase(one);
    delay(kappa - pwHd - 2.0e-6 - PRG_START_DELAY);

    rgpulse(pwHd,one,0.0,0.0);
    txphase(zero);
    delay(2.0e-6);
    obsprgon("waltz16", pwHd, 90.0);	          /* PRG_START_DELAY */
    xmtron();
    decphase(zero);
    dec2phase(zero);
    decpwrf(rf8);
    delay(timeTN - kappa - WFG3_START_DELAY);
   
							  /* WFG3_START_DELAY */
	sim3shaped_pulse("", "offC8", "", 0.0, pwC8, 2.0*pwN, zero, zero, zero, 
								     0.0, 0.0);
	decphase(t3);
	decpwrf(rf6);
	delay(timeTN);

	dec2rgpulse(pwN, zero, 0.0, 0.0);

    xmtroff();
    obsprgoff();
    rgpulse(pwHd,three,2.0e-6,0.0);
	zgradpulse(gzlvl3, gt3);
 	delay(2.0e-4);
 /***************************************************************/
 /* The sequence is different from here with respect to ghn_co **/
 /***************************************************************/

    rgpulse(pwHd,one,2.0e-6,0.0);	/* H1 decoupler is turned on */
    txphase(zero);
    delay(2.0e-6);
    obsprgon("waltz16", pwHd, 90.0);	          
    xmtron();
    decshaped_pulse("offC6", pwC6, t3, 0.0, 0.0);
    decphase(zero);


	/* Refocus CO, evolve CO, spinlock CO and defocus CO  */


	delay(timeTN - tau1/2 - 0.6*pwC6 - WFG3_START_DELAY);
	decpwrf(rf8);
	sim3shaped_pulse("", "offC8","",0.0,pwC8, 2.0*pwN, zero,zero,zero,0.0,0.0);
	decpwrf(rf3);
	delay(timeTN - WFG3_STOP_DELAY - WFG_START_DELAY - pwC3a/2);
	decshaped_pulse("offC3",pwC3a,zero,0.0,0.0);
	if (tau1 > 0)
	delay(tau1/2 - WFG_STOP_DELAY - pwC3a/2 - 2.0e-6);
	else
	  delay(tau1/2);
	  
/*******DO SPINLOCK ********/

	decpwrf(rftrim);		
	decrgpulse(0.002,zero,2.0e-6,0.0);
	decpwrf(rfd);
	starthardloop(v9);
		decrgpulse(6.4*p_d,zero,0.0,0.0);
		decrgpulse(8.2*p_d,two,0.0,0.0);
		decrgpulse(5.8*p_d,zero,0.0,0.0);
		decrgpulse(5.7*p_d,two,0.0,0.0);
		decrgpulse(0.6*p_d,zero,0.0,0.0);
		decrgpulse(4.9*p_d,two,0.0,0.0);
		decrgpulse(7.5*p_d,zero,0.0,0.0);
		decrgpulse(5.3*p_d,two,0.0,0.0);
		decrgpulse(7.4*p_d,zero,0.0,0.0);
		
		decrgpulse(6.4*p_d,two,0.0,0.0);
		decrgpulse(8.2*p_d,zero,0.0,0.0);
		decrgpulse(5.8*p_d,two,0.0,0.0);
		decrgpulse(5.7*p_d,zero,0.0,0.0);
		decrgpulse(0.6*p_d,two,0.0,0.0);
		decrgpulse(4.9*p_d,zero,0.0,0.0);
		decrgpulse(7.5*p_d,two,0.0,0.0);
		decrgpulse(5.3*p_d,zero,0.0,0.0);
		decrgpulse(7.4*p_d,two,0.0,0.0);
		
		decrgpulse(6.4*p_d,two,0.0,0.0);
		decrgpulse(8.2*p_d,zero,0.0,0.0);
		decrgpulse(5.8*p_d,two,0.0,0.0);
		decrgpulse(5.7*p_d,zero,0.0,0.0);
		decrgpulse(0.6*p_d,two,0.0,0.0);
		decrgpulse(4.9*p_d,zero,0.0,0.0);
		decrgpulse(7.5*p_d,two,0.0,0.0);
		decrgpulse(5.3*p_d,zero,0.0,0.0);
		decrgpulse(7.4*p_d,two,0.0,0.0);
		
		decrgpulse(6.4*p_d,zero,0.0,0.0);
		decrgpulse(8.2*p_d,two,0.0,0.0);
		decrgpulse(5.8*p_d,zero,0.0,0.0);
		decrgpulse(5.7*p_d,two,0.0,0.0);
		decrgpulse(0.6*p_d,zero,0.0,0.0);
		decrgpulse(4.9*p_d,two,0.0,0.0);
		decrgpulse(7.5*p_d,zero,0.0,0.0);
		decrgpulse(5.3*p_d,two,0.0,0.0);
		decrgpulse(7.4*p_d,zero,0.0,0.0);
		
	endhardloop();
	decpwrf(4095.0);
	
/*   End of spinlock */

	delay(timeTN - WFG3_START_DELAY);
	decpwrf(rf8);
	sim3shaped_pulse("","offC8","",0.0,pwC8,2*pwN,zero,zero,zero,0.0,0.0);
	decpwrf(rf6);
	delay(timeTN - WFG3_STOP_DELAY);
	
 /***************************************************************/
 /*      The sequence is same as ghn_co from this point  ********/
 /***************************************************************/
 
	decshaped_pulse("offC6", pwC6, t5, 0.0, 0.0);


/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */

	dec2phase(t8);
	zgradpulse(gzlvl4, gt4);
	txphase(one);
	dcplrphase(zero);
 	delay(2.0e-4);
	dec2rgpulse(pwN, t8, 0.0, 0.0);

	decphase(zero);
	dec2phase(t9);
	decpwrf(rf8);
	delay(timeTN - WFG3_START_DELAY - tau2);
							 /* WFG3_START_DELAY  */
	sim3shaped_pulse("", "offC8", "", 0.0, pwC8, 2.0*pwN, zero, zero, t9, 0.0, 0.0);
	dec2phase(t10);
	decpwrf(rf3);


    if (tau2 > kappa)
	{
          delay(timeTN - pwC3a - WFG_START_DELAY);     	   /* WFG_START_DELAY */
          decshaped_pulse("offC3", pwC3a, zero, 0.0, 0.0);
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
    else if (tau2 > (kappa - pwC3a - WFG_START_DELAY))
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  rgpulse(pwHd,three,2.0e-6,0.0);
	  txphase(t4);                                     /* WFG_START_DELAY */
          decshaped_pulse("offC3", pwC3a, zero, 0.0, 0.0);
          delay(kappa -pwC3a -WFG_START_DELAY -gt1 -2.0*GRADIENT_DELAY -1.0e-4);
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
          delay(kappa - tau2 - pwC3a - WFG_START_DELAY);   /* WFG_START_DELAY */
          decshaped_pulse("offC3", pwC3a, zero, 0.0, 0.0);
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
    	  delay(kappa-tau2-pwC3a-WFG_START_DELAY-gt1-2.0*GRADIENT_DELAY-1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - POWER_DELAY);                    /* WFG_START_DELAY */
          decshaped_pulse("offC3", pwC3a, zero, 0.0, 0.0);
          delay(tau2);
	}
/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
	sim3pulse(pw, 0.0, pwN, t4, zero, t10, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	delay(lambda - 1.3*pwN - gt5);

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
	delay(lambda - 0.65*pwN - gt5);

	rgpulse(pw, zero, 0.0, 0.0); 

	delay((gt1/10.0) + 1.0e-4 +gstab - 0.5*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

	rgpulse(2.0*pw, zero, 0.0,0.0);
	dec2power(dpwr2);				       /* POWER_DELAY */
        if (mag_flg[A] == 'y')    magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);
        else   zgradpulse(icosel*gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */
        delay(gstab);
        rcvron();
statusdelay(C,1.0e-4);

	setreceiver(t12);
}		 
