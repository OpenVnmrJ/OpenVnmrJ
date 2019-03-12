/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghcch_tocsyA.c               

    3D HCCH tocsy utilising gradients and double sensitivity enhancement.
    Also known as DE-H(C)CH-TOCSY.

    Uses optional magic-angle gradients.

    Correlates the sidechain aliphatic 13C resonances of a given amino acid.
    Uses isotropic 13C mixing.

    Standard features include maintaining the 13C carrier in the CaCb region
    throughout using off-res SLP one-lobe sinc pulses on 13CO with first null
    at Ca.  Maximum sensitivity is obtained using full power square pulses on 
    the CaCb region throughout.  DIPSI-3 rather than DIPSI-2 (suggested in the
    JMR article) is used as standard as in the other BioPack sequences;
    optional 2H decoupling when CaCb magnetization is transverse and during 1H
    shift evolution for 4 channel spectrometers.  
 
    pulse sequence: Sattler, Schwendinger, Schleucher and Griesinger, JBNMR,
			Vol 6  No.1, 11-22 (1995).
    SLP pulses:     J Magn. Reson. 96, 94-102 (1992)

    Derived from gc_co_nh.c written by Robin Bendall, Varian, March 94 and 95.
    Revised and improved to a standard format by MRB, BKJ and GG for the 
    BioPack, January 1997.
    Auto-calibrated version, E.Kupce, 27.08.2002.

    Efficient STUD+ decoupling is invoked with STUD='y' without need to 
    set any parameters.
    (STUD+ decoupling- Bendall & Skinner, JMR, A124, 474 (1997) and in press)
     
    STUD DECOUPLING.   SET STUD='y':
       Setting the flag STUD='y' overrides the decoupling parameters listed in
       dg2 and applies STUD+ decoupling instead.  In consequence is is easy
       to swap between the decoupling scheme you have been using to STUD+ for
       an easy comparison.  The STUD+ waveforms are calculated for your field
       strength at the time of BioPack installation and RF power is 
       calculated within the pulse sequence.  The calculations are for the most 
       peaks being greater than 90% of ideal. If you wish to compare different 
       decoupling schemes, the power level used for STUD+ can be obtained from 
       dps - subtract 3dB from that level to compare to decoupling schemes at
       a continuous RF level such as GARP.  The value of 90% has
       been set to limit sample heating as much as possible.  If you wish to 
       change STUD parameters, for example to increase the quality of decoupling
       (and RF heating) change the 95% level for the centerband
       by changing the relevant number in the macro makeSTUDpp and 
       rerun the macro (don't use 100% !!).  (At the time of writing STUD has
       been coded to use the coarse attenuator, because the fine attenuator
       is not presently included in the fail-safe calculation of decoupling 
       power which aborts an experiment if the power is too high - this may
       lower STUD efficiency a little).


        	  CHOICE OF DECOUPLING AND 2D MODES

    	Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
	Set dm3 = 'nnn' for no 2H decoupling, or
		  'nyn'  and dmm3 = 'w' for 2H decoupling. 
  
    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [1H]  and t2 [13C].

    2D experiment in t1: wft2d(1,0,-1,0,0,1,0,1) (sensitivity-enhanced)
    2D experiment in t2: wft2d('ni2',1,0,-1,0,0,1,0,1) (sensitivity-enhanced)
    ( or with 5.2F or above just use wft2da or wft2da('ni2') after setting
      f1coef='1 0 -1 0 0 1 0 1'
      f2coef='1 0 -1 0 0 1 0 1'
     for 3D just use ft3d )
    
    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in 13C and f2180='n' for (0,0) in 15N.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.




          	  DETAILED INSTRUCTIONS FOR USE OF ghcch_tocsy

    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for ghcch_tocsy may be printed using:
                                      "printon man('ghcch_tocsy') printoff".
             
    2. Apply the setup macro "ghcch_tocsy".  This loads the relevant parameter
       set and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral
       check.

    3. The parameter ncyc corresponds to the number of cycles of DIPSI-3 mixing.
       Use ncyc = 2 or 3 usually.  This corresponds to a total mixing time of
       (2 or 3)*6.07*600/sfrq ms for a DIPSI rf field of 6-7 kHz for a 600Mhz 
       spectrometer, is sufficient to cover 14.4 kHz of bandwidth (96ppm at 600
       MHz, more than adequate for the CC J's. Change the value of spinlock to
       increase or reduce the bandwidth. 
      
    4. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 35ppm, and N15 
       frequency on the amide region (120 ppm).  The C13 frequency remains at 
       35ppm, ie at CaCb throughout the sequence.

    5. del (3.4 ms), del1 (1.9 ms), and del2 (1.6 ms), were determined for 
       alphalytic protease and are listed in dg2 for possible readjustment by
       the user. The above reference (Fig.1) gives theoretical values of 0.5/J,
       0.31/J and 0.23/J for del, del1 and del2 and elsewhere in the reference,
       3.6, 2.2 and 1.9 ms can be inferred. 

    6. If 2H decoupling is used, the 2H lock signal may become unstable because
       of 2H saturation.  Check that a 1D spectrum is stable/reproducible as 
       when 2H decoupling is not used.  You might also check this for d2 and d3
       equal to values achieved at say 75% of their maximum.  Remember to return
       d2=d3=0 before starting a 2D/3D experiment.

    7. A 2D CH spectrum will be an ordinary 13C hsqc since evolution in t2 occurs
       after the c13 spinlock. Therefore, CH 2D spectra using hcch_tocsy.c cannot
       be compared with CH 2D spectra using ghcch_tocsy.c . However, 3D spectra
       will show all crosspeaks in both experiments. The magic-angle option does
       permit somewhat better water suppression using ghcch_tocsy.

    8. The autocal and checkofs flags are generated automatically in Pbox_bio.h
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

static int   phi3[2]  = {0,2},
             phi6[1]  = {1},
             phi5[4]  = {0,0,2,2},
             phi10[1] = {1},
	     rec[4]   = {0,2,2,0};

static double   d2_init=0.0, d3_init=0.0;
static double   H1ofs=4.7, C13ofs=35.0, N15ofs=120.0, H2ofs=0.0;

static shape offC10;

void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR],                            /*magic angle gradient*/
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
	    stCdec[MAXSTR],	       /* calls STUD+ waveforms from shapelib */
	    STUD[MAXSTR];   /* apply automatically calculated STUD decoupling */
 
int         icosel1,          			  /* used to get n and p type */
	    icosel2,
            t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
	    del = getval("del"),     /* time delays for CH coupling evolution */
         BPdpwrspinlock,        /*  user-defined upper limit for spinlock(Hz) */
         BPpwrlimits,           /*  =0 for no limit, =1 for limit             */

	    del1 = getval("del1"),
	    del2 = getval("del2"),
/* STUD+ waveforms automatically calculated by macro "biocal"  	      */
/* and string parameter stCdec calls them from your shapelib. 	              */
   stdmf,                                /* dmf for STUD decoupling           */
   studlvl,	                         /* coarse power for STUD+ decoupling */
   rf80 = getval("rf80"), 			  /* rf in Hz for 80ppm STUD+ */
   bw, ofs, ppm,                            /* temporary Pbox parameters */
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	rf0,            	  /* maximum fine power when using pwC pulses */

/* p_d is used to calculate the isotropic mixing on the Cab region            */
        spinlock = getval("spinlock"), /* DIPSI-3 spinlock field strength in Hz */
        p_d,                  	       /* 50 degree pulse for DIPSI-2 at rfd  */
        rfd,                    /* fine power for 7 kHz rf for 500MHz magnet  */
	ncyc = getval("ncyc"), 			  /* no. of cycles of DIPSI-3 */

/* the following pulse lengths for SLP pulses are automatically calculated    */
/* by the macro "ghcch_tocsy" .  SLP pulse shapes, "offC10" etc are called   */
/* directly from your shapelib.                    			      */
   pwC10,                       /* 180 degree selective sinc pulse on CO(174ppm) */
   pwZ,					  /* the largest of pwC10 and 2.0*pwN */
   rf10,	                 /* fine power for the pwC10 ("offC10") pulse */

   compC = getval("compC"),         /* adjustment for C13 amplifier compression */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzcal = getval("gzcal"),               /* G/cm to DAC coversion factor*/
        gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt3 = getval("gt3"),				   /* other gradients */
	gt5 = getval("gt5"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6");

    getstr("STUD",STUD);
    getstr("mag_flg",mag_flg);
    getstr("f1180",f1180);
    getstr("f2180",f2180);
   strcpy(stCdec, "stCdec80");
   stdmf = getval("dmf80");
   studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
   studlvl = (int) (studlvl + 0.5);
  P_getreal(GLOBAL,"BPpwrlimits",&BPpwrlimits,1);
  P_getreal(GLOBAL,"BPdpwrspinlock",&BPdpwrspinlock,1);


/*   LOAD PHASE TABLE    */

	settable(t3,2,phi3);
	settable(t6,1,phi6);
	settable(t5,4,phi5);
	settable(t10,1,phi10);
	settable(t11,4,rec);

        

/*   INITIALIZE VARIABLES   */


  if (BPpwrlimits > 0.5)
  {
   if (spinlock > BPdpwrspinlock)
    {
     spinlock = BPdpwrspinlock;  
     printf("spinlock too large, reset to user-defined limit (BPdpwrspinlock)");
     psg_abort(1);
    }
  }
    if( dpwrf < 4095 )
	{ printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
	  psg_abort(1); }

    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;

      setautocal();                        /* activate auto-calibration flags */ 
        
      if (autocal[0] == 'n') 
      {
        /* "offC10": 180 degree one-lobe sinc pulse on CO, null at Ca 139ppm away */
        pwC10 = getval("pwC10");
	  rf10 = (compC*4095.0*pwC*2.0*1.65)/pwC10;     /* needs 1.65 times more     */
	  rf10 = (int) (rf10 + 0.5);		           /* power than a square pulse */

        if( pwC > (24.0e-6*600.0/sfrq) )
	  { printf("Increase pwClvl so that pwC < 24*600/sfrq");
	    psg_abort(1); 
        }
      }
      else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
      {
        if(FIRST_FID)                                            /* call Pbox */
        {
          ppm = getval("dfrq"); 
          bw = 118.0*ppm; ofs = 139.0*ppm;
          offC10 = pbox_make("offC10", "sinc180n", bw, ofs, compC*pwC, pwClvl);
          if(dm3[B] == 'y') H2ofs = 3.2;    
          ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
        }
        rf10 = offC10.pwrf;  pwC10 = offC10.pw;
      }
				
   /* dipsi-3 decoupling on CbCa */	
 	p_d = (5.0)/(9.0*4.0*spinlock);      /*  DIPSI-3 Field Strength */
 	rfd = (compC*4095.0*pwC*5.0)/(p_d*9.0);
	rfd = (int) (rfd + 0.5);
  	ncyc = (int) (ncyc + 0.5);


/* CHECK VALIDITY OF PARAMETER RANGES */

    if( gt1 > 0.5*del - 1.0e-4)
    {
        printf(" gt1 is too big. Make gt1 less than %f.\n", (0.5*del - 1.0e-4));
        psg_abort(1);
    }

    if( dm[A] == 'y' )
    {
        printf("incorrect dec1 decoupler flag! Should be 'nny' or 'nnn' ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[C] == 'y'))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnn' ");
        psg_abort(1);
    }
    if((dm3[A] == 'y' || dm3[C] == 'y'))
    {
        printf("incorrect dec3 decoupler flags! Should be 'nnn' or 'nyn' ");
        psg_abort(1);
    }

    if( dpwr > 52 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( pw > 50.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 
  
    if( pwN > 100.0e-6 )
    {
        printf("dont fry the probe, pwN too high ! ");
        psg_abort(1);
    } 
 

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    icosel1 = -1;  icosel2 = -1;
    if (phase1 == 2) 
	{ tsadd(t6,2,4); icosel1 = -1*icosel1; }
    if (phase2 == 2) 
	{ tsadd(t10,2,4); icosel2 = -1*icosel2; tsadd(t6,2,4); }


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
	{ tsadd(t3,2,4); tsadd(t11,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t5,2,4); tsadd(t11,2,4); }



/*   BEGIN PULSE SEQUENCE   */

status(A);
        if ( dm3[B] == 'y' )
          lk_sample();  
        if ((ni/sw1-d2)>0) 
         delay(ni/sw1-d2);       /*decreases as t1 increases for const.heating*/
        if ((ni2/sw2-d3)>0) 
         delay(ni2/sw2-d3);      /*decreases as t2 increases for const.heating*/
   	delay(d1);
        if ( dm3[B] == 'y' )
          { lk_hold(); lk_sampling_off();}  /*freezes z0 correction, stops lock pulsing*/

	rcvroff();
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	decpwrf(rf0);
	obsoffset(tof);
	txphase(t3);
	delay(1.0e-5);

	decrgpulse(pwC, zero, 0.0, 0.0);	   /*destroy C13 magnetization*/
	zgradpulse(gzlvl1, 0.5e-3);
	delay(1.0e-4);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl1, 0.5e-3);
	delay(5.0e-4);

      if ( dm3[B] == 'y' )     /* begins optional 2H decoupling */
        {
          dec3rgpulse(1/dmf3,one,10.0e-6,2.0e-6);
          dec3unblank();
          dec3phase(zero);
          delay(2.0e-6);
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
        }
	rgpulse(pw, t3, 0.0, 0.0);                    /* 1H pulse excitation */

        decphase(zero);
	delay(0.5*del + tau1 - 2.0*pwC);

        decrgpulse(2.0*pwC, zero, 0.0, 0.0);

        txphase(zero);
	delay(tau1);

	rgpulse(2.0*pw, zero, 0.0, 0.0);

                if (mag_flg[A] == 'y')
                {
                   magradpulse(icosel1*gzcal*gzlvl1,0.1*gt1);
                }
                else
                {
                   zgradpulse(icosel1*gzlvl1, 0.1*gt1);
                }
        decphase(t5);
	delay(0.5*del - 0.1*gt1);

	simpulse(pw, pwC, zero, t5, 0.0, 0.0);

	zgradpulse(gzlvl3, gt3);
        decphase(zero);
	delay(0.5*del2 - gt3);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl3, gt3);
        txphase(t6);
        decphase(one);
	delay(0.5*del2 - gt3);

	simpulse(pw, pwC, t6, one, 0.0, 0.0);

	zgradpulse(gzlvl4, gt3);
        txphase(zero);
        decphase(zero);
	delay(0.5*del1 - gt3);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl4, gt3);
	delay(0.5*del1 - gt3);

	decrgpulse(pwC, zero, 0.0, 0.0);
	decpwrf(rfd);
	delay(2.0e-6);
	initval(ncyc, v2);
	starthardloop(v2);
     decrgpulse(4.9*p_d,zero,0.0,0.0);
     decrgpulse(7.9*p_d,two,0.0,0.0);
     decrgpulse(5.0*p_d,zero,0.0,0.0);
     decrgpulse(5.5*p_d,two,0.0,0.0);
     decrgpulse(0.6*p_d,zero,0.0,0.0);
     decrgpulse(4.6*p_d,two,0.0,0.0);
     decrgpulse(7.2*p_d,zero,0.0,0.0);
     decrgpulse(4.9*p_d,two,0.0,0.0);
     decrgpulse(7.4*p_d,zero,0.0,0.0);
     decrgpulse(6.8*p_d,two,0.0,0.0);
     decrgpulse(7.0*p_d,zero,0.0,0.0);
     decrgpulse(5.2*p_d,two,0.0,0.0);
     decrgpulse(5.4*p_d,zero,0.0,0.0);
     decrgpulse(0.6*p_d,two,0.0,0.0);
     decrgpulse(4.5*p_d,zero,0.0,0.0);
     decrgpulse(7.3*p_d,two,0.0,0.0);
     decrgpulse(5.1*p_d,zero,0.0,0.0);
     decrgpulse(7.9*p_d,two,0.0,0.0);

     decrgpulse(4.9*p_d,two,0.0,0.0);
     decrgpulse(7.9*p_d,zero,0.0,0.0);
     decrgpulse(5.0*p_d,two,0.0,0.0);
     decrgpulse(5.5*p_d,zero,0.0,0.0);
     decrgpulse(0.6*p_d,two,0.0,0.0);
     decrgpulse(4.6*p_d,zero,0.0,0.0);
     decrgpulse(7.2*p_d,two,0.0,0.0);
     decrgpulse(4.9*p_d,zero,0.0,0.0);
     decrgpulse(7.4*p_d,two,0.0,0.0);
     decrgpulse(6.8*p_d,zero,0.0,0.0);
     decrgpulse(7.0*p_d,two,0.0,0.0);
     decrgpulse(5.2*p_d,zero,0.0,0.0);
     decrgpulse(5.4*p_d,two,0.0,0.0);
     decrgpulse(0.6*p_d,zero,0.0,0.0);
     decrgpulse(4.5*p_d,two,0.0,0.0);
     decrgpulse(7.3*p_d,zero,0.0,0.0);
     decrgpulse(5.1*p_d,two,0.0,0.0);
     decrgpulse(7.9*p_d,zero,0.0,0.0);

     decrgpulse(4.9*p_d,two,0.0,0.0);
     decrgpulse(7.9*p_d,zero,0.0,0.0);
     decrgpulse(5.0*p_d,two,0.0,0.0);
     decrgpulse(5.5*p_d,zero,0.0,0.0);
     decrgpulse(0.6*p_d,two,0.0,0.0);
     decrgpulse(4.6*p_d,zero,0.0,0.0);
     decrgpulse(7.2*p_d,two,0.0,0.0);
     decrgpulse(4.9*p_d,zero,0.0,0.0);
     decrgpulse(7.4*p_d,two,0.0,0.0);
     decrgpulse(6.8*p_d,zero,0.0,0.0);
     decrgpulse(7.0*p_d,two,0.0,0.0);
     decrgpulse(5.2*p_d,zero,0.0,0.0);
     decrgpulse(5.4*p_d,two,0.0,0.0);
     decrgpulse(0.6*p_d,zero,0.0,0.0);
     decrgpulse(4.5*p_d,two,0.0,0.0);
     decrgpulse(7.3*p_d,zero,0.0,0.0);
     decrgpulse(5.1*p_d,two,0.0,0.0);
     decrgpulse(7.9*p_d,zero,0.0,0.0);

     decrgpulse(4.9*p_d,zero,0.0,0.0);
     decrgpulse(7.9*p_d,two,0.0,0.0);
     decrgpulse(5.0*p_d,zero,0.0,0.0);
     decrgpulse(5.5*p_d,two,0.0,0.0);
     decrgpulse(0.6*p_d,zero,0.0,0.0);
     decrgpulse(4.6*p_d,two,0.0,0.0);
     decrgpulse(7.2*p_d,zero,0.0,0.0);
     decrgpulse(4.9*p_d,two,0.0,0.0);
     decrgpulse(7.4*p_d,zero,0.0,0.0);
     decrgpulse(6.8*p_d,two,0.0,0.0);
     decrgpulse(7.0*p_d,zero,0.0,0.0);
     decrgpulse(5.2*p_d,two,0.0,0.0);
     decrgpulse(5.4*p_d,zero,0.0,0.0);
     decrgpulse(0.6*p_d,two,0.0,0.0);
     decrgpulse(4.5*p_d,zero,0.0,0.0);
     decrgpulse(7.3*p_d,two,0.0,0.0);
     decrgpulse(5.1*p_d,zero,0.0,0.0);
     decrgpulse(7.9*p_d,two,0.0,0.0);
	endhardloop();

        dec2phase(zero);
        decphase(zero);
        txphase(zero);
	decpwrf(rf10);
	delay(tau2);
							  /* WFG3_START_DELAY */
	sim3shaped_pulse("", "offC10", "", 2.0*pw, pwC10, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        if(pwC10>2.0*pwN) pwZ=0.0; else pwZ=2.0*pwN - pwC10;

	delay(tau2);
	decpwrf(rf0);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(-icosel2*gzcal*gzlvl2, 1.8*gt1);
                }
                else
                {
                   zgradpulse(-icosel2*gzlvl2, 1.8*gt1);
                }
	delay(2.02e-4);

	decrgpulse(2.0*pwC, zero, 0.0, 0.0);

	decpwrf(rf10);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(icosel2*gzcal*gzlvl2, 1.8*gt1);
                }
                else
                {
                   zgradpulse(icosel2*gzlvl2, 1.8*gt1);
                }
	delay(2.0e-4 + WFG3_START_DELAY + pwZ);

	decshaped_pulse("offC10", pwC10, zero, 0.0, 0.0);
	decpwrf(rf0);
	decrgpulse(pwC, zero, 2.0e-6, 0.0);

	zgradpulse(gzlvl5, gt5);
	delay(0.5*del1 - gt5);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	decphase(t10);
	delay(0.5*del1 - gt5);

	simpulse(pw, pwC, one, t10, 0.0, 0.0);

	zgradpulse(gzlvl6, gt5);
	txphase(zero);
	decphase(zero);
	delay(0.5*del2 - gt5);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl6, gt5);
	delay(0.5*del2 - gt5);

	simpulse(pw, pwC, zero, zero, 0.0, 0.0);

	delay(0.5*del - 0.5*pwC);

	simpulse(2.0*pw,2.0*pwC, zero, zero, 0.0, 0.0);
        if (mag_flg[A] == 'y')
            magradpulse(gzcal*gzlvl1, gt1);
        else
            zgradpulse(gzlvl1, gt1);
        rcvron();
   if ((STUD[A]=='n') && (dm[C] == 'y'))
        decpower(dpwr);
        if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
        {
           delay(0.5*del-40.0e-6 -gt1 -1/dmf3);
           setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
           dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
           dec3blank();
           lk_autotrig();   /* resumes lock pulsing */
           lk_sample();
           if (mag_flg[A] == 'y')
            statusdelay(C,40.0e-6  - 2.0*VAGRADIENT_DELAY - POWER_DELAY);
           else
            statusdelay(C,40.0e-6  - 2.0*GRADIENT_DELAY - POWER_DELAY);
        }
      else
        {
         delay(0.5*del-40.0e-6 -gt1);
        if (mag_flg[A] == 'y')
         statusdelay(C,40.0e-6  - 2.0*VAGRADIENT_DELAY - POWER_DELAY);
        else
         statusdelay(C,40.0e-6  - 2.0*GRADIENT_DELAY - POWER_DELAY);
        }
  if ((STUD[A]=='y') && (dm[C] == 'y'))
        {decpower(studlvl);
         decunblank();
         decon();
         decprgon(stCdec,1/stdmf, 1.0);
         startacq(alfa);
         acquire(np, 1.0/sw);
         decprgoff();
         decoff();
         decblank();
        }
 setreceiver(t11);
}		 
		 
