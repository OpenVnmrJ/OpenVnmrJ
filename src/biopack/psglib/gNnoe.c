/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gNnoe.c

    This pulse sequence will allow one to perform the following experiment:

    2D determination of H1 to N15 NOE, with N15 shift in t1, and inverse
    INEPT gradient sensitivity enhanced transfer back to H1.
    Optional magic-angle coherence transfer gradients.

    Standard features include optional 13C sech/tanh pulse on CO and Cab to 
    refocus 13C coupling during t1; option of obtaining spectrum of only NH2 
    groups;

    pulse sequence: 	Farrow et al, Biochemistry, 33, 5984 (1994)

    sech/tanh pulse: 	Silver, Joseph and Hoult, JMR, 59, 347 (1984)
			Bendall, JMR, A116, 46 (1995)
         
    Written by MRB, January 1998, starting with gNhsqc from BioPack.
    Revised and improved to a standard format by MRB, BKJ and GG for the 
    BioPack, January 1998, so as to include calculation of the above 
    standard features within the pulse sequence code and associated macro.



        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nnn', dmm = 'ccc'
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for N15 decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.



          	  DETAILED INSTRUCTIONS FOR USE OF gNnoe

         
    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for gNnoe may be printed using:
                                      "printon man('gNnoe') printoff".
             
    2. Apply the setup macro "gNnoe".  This loads the relevant parameter
       set and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 100ppm, and N15 
       frequency on the amide region (120 ppm).

    4. Pulse and frequency calibrations are as for gNhsqc, or as generally
       used for BioPack.

    5. Splitting of resonances by C13 coupling in C13-enriched samples can be 
       removed by setting C13refoc='y'.

    8. NH2 GROUPS:
       A spectrum of NH2 groups, with NH groups reduced to 70%, can be obtained
       with flag NH2also='y'.  This utilises 1/8J (J=94Hz) periods of NH 
       J-coupling evolution added to t1 in place of 1/4J periods (normal INEPT
       behaviour).  In consequence the gt1 time is reduced to 1.125ms by the 
       pulse sequence code - for optimum use it would be a good
       idea to recalibrate gzlvl2 for gt1 = 1.125ms (in practice we observed
       a recalibration from 13060 at 2.5ms to 12900 at 1.125ms, which loses
       5% S/N if not recalibrated).

    9. relaxT is the total time for the N15 NOe to build up.  It should be 
       long relative to the N15 T1, eg 2.5 seconds.

   10. To maintain constant unblanking in the 1H amplifier for both noe/no noe
       cases, the same repetitive pulse train is used for both, but the power
       is decreased to mimimum on both the attenuator and modulator for the "no noe"
       case (only if relaxT=0). This mode will result in very little 
       excitation of the protons in the no noe case (rf field is on the order of
       6 millihertz), but will put the same load on the amplifier (unblanking
       is the primary loading process on the amplifier, not the amplitude of the
       rf pass through). This will permit more accurate measurements of noe.

       If relaxT>0 the pulse train will occur at saturation power levels.
       if relaxT=0 the same pulse train will occur (of length d1), but at the
       lowest power level(no saturation).

       Set d1=relaxT when relaxT>0 (the actual noe experiment). The variable d1 is
       used to calculate the number of cycles during the non-saturation period, such
       that the total time can be made equal to relaxT (if d1 is set to the same
       value of relaxT as used for the saturation experiment).

    11. Step-by-step:
       a. select value of relaxT to be used for proton saturation (e.g. 3 seconds).
       b. set d1=relaxT (this ensures the same repetition rate for both experiments).
       c. set nt, d1, phase for 2D experiment.
       d. copy parameters to a different experiment.
       e. set relaxT=0
       f. acquire both experiments.

       2D spectra should be done in separate runs for each value of relaxT.

   11. mag_flg = y     using magic angle pulsed field gradient 
                 n     using z-axis gradient only.


Note: a discussion of the proper selection of relaxation delay can be found
     in Renner et.al, J.Biomol.NMR, 23, 22-33(200). They indicate that a 5sec
     delay may be insufficient and recommend 10sec.(Jack Howarth, U. Cincinnati)

*/



#include <standard.h>
  


static int   phi3[2]  = {1,3},
	      
             phi9[8]  = {0,0,1,1,2,2,3,3},
	     phi10[1] = {0}, 
             rec[4]   = {0,2,2,0};

static double   d2_init=0.0;


void pulsesequence()
{


/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR],
            C13refoc[MAXSTR],		/* C13 sech/tanh pulse in middle of t1*/
	    NH2also[MAXSTR];		         /*  NH2 groups also obtained */

 
int         icosel,          			  /* used to get n and p type */
            t1_counter;  		        /* used for states tppi in t1 */

double      tau1,         				         /*  t1 delay */
	    lambda = 0.94/(4.0*getval("JNH")), 	   /* 1/4J H1 evolution delay */
	    tNH = 1.0/(4.0*getval("JNH")),	  /* 1/4J N15 evolution delay */
	    relaxT = getval("relaxT"),       /* total relaxation time for NOE */
            gzcal = getval("gzcal"),
	    ncyc,			 /* number of pulsed cycles in relaxT */
        
/* the sech/tanh pulse is automatically calculated by the macro "proteincal", */  /* and is called directly from your shapelib.                  		      */
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */
   compH = getval("compH"),         /* adjustment for H1  amplifier compression */
   compC = getval("compC"),         /* adjustment for C13 amplifier compression */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt3 = getval("gt3"),		   		   /* other gradients */
	gt5 = getval("gt5"),
	gstab = getval("gstab"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5");

    getstr("f1180",f1180);
    getstr("C13refoc",C13refoc);
    getstr("NH2also",NH2also);
    getstr("mag_flg", mag_flg);



/*   LOAD PHASE TABLE    */

	settable(t3,2,phi3);

	settable(t9,8,phi9);
	settable(t10,1,phi10);
	settable(t11,4,rec);




/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses (and initialize rfst) */
	rf0 = 4095.0;    rfst=0.0;

/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
     if (C13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
	rfst = (int) (rfst + 0.5);
	if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }}

/* use 1/8J times to obtain NH2 groups as well as NH */
  if (NH2also[A]=='y')	
     {  tNH = tNH/2.0;  }



/* CHECK VALIDITY OF PARAMETER RANGES */

  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }

  if( pw > 20.0e-6 )
  { text_error("dont fry the probe, pw too high ! "); psg_abort(1); } 
  
  if( pwN > 100.0e-6 )
  { text_error("dont fry the probe, pwN too high ! "); psg_abort(1); }



/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 1) 
	{ tsadd(t10,2,4); icosel = 1; }
    else icosel = -1; 



/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;



/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t11,2,4); }



/* BEGIN PULSE SEQUENCE */

status(A);
	obspower(tpwr);
	decpower(pwClvl);
	decpwrf(rf0);
 	dec2power(pwNlvl);
	txphase(zero);
        dec2phase(zero);


    if (relaxT < 1.0e-4 )
     {
      ncyc = (int)(200.0*d1 + 0.5);   /* no H1 saturation */
      initval(ncyc,v1);
      obspower(-15.0); obspwrf(0.0);  
              /* powers set to minimum, but amplifier is unblancked identically */
           loop(v1,v2);
         	   delay(2.5e-3 - 4.0*compH*1.34*pw);
           	   rgpulse(4.0*compH*1.34*pw, zero, 5.0e-6, 0.0e-6);  
    	          rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 0.0e-6);  
    	          rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 0.0e-6);  
           	   rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 5.0e-6);  
   	          delay(2.5e-3 - 4.0*compH*1.34*pw);
	   endloop(v2);
     }
    else
     {       
      ncyc = (int)(200.0*relaxT + 0.5); 			     /* H1 saturation */
      initval(ncyc,v1);
      if (ncyc > 0)
	  {
	   obspower(tpwr-12);
           loop(v1,v2);
	   delay(2.5e-3 - 4.0*compH*1.34*pw);
    	   rgpulse(4.0*compH*1.34*pw, zero, 5.0e-6, 0.0e-6);  
    	   rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 0.0e-6);  
    	   rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 0.0e-6);  
    	   rgpulse(4.0*compH*1.34*pw, zero, 0.0e-6, 5.0e-6);  
   	   delay(2.5e-3 - 4.0*compH*1.34*pw);
	   endloop(v2);
          }
      }
        obspower(tpwr); obspwrf(4095.0);
	rgpulse(pw, zero, rof1, rof1);          /*destroy any H1 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);          /*destroy C13 magnetization*/
	zgradpulse(gzlvl3, gt3);
	delay(1.0e-4);
	rgpulse(pw, zero, rof1, rof1);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl3, gt3);

	decpwrf(rfst);
   	txphase(zero);
   	decphase(zero);
   	dec2phase(t3);
	delay(2.0e-4);

 	dec2rgpulse(pwN, t3, 0.0, 0.0); 	     /*  begin inverse INEPT */


/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */

	dec2phase(t9);
      	delay(tau1);
         			  /* optional sech/tanh pulse in middle of t1 */
    	if (C13refoc[A]=='y') 				   /* WFG_START_DELAY */
           {decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);
            delay(tNH - 1.0e-3 - WFG_START_DELAY - 2.0*pw - rof1);}
    	else
           {delay(tNH - 2.0*pw -rof1);}
    	rgpulse(2.0*pw, zero, rof1, rof1);
    	if ((tNH+rof1) < gt1 + 1.99e-4)  delay(gt1 + 1.99e-4 - tNH -rof1);

    	delay(tau1);

    	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);

        if (mag_flg[A] == 'y')
           magradpulse(gzcal*gzlvl1, gt1);
        else
           zgradpulse(gzlvl1,gt1);
    	dec2phase(t10);
   	if (tNH > gt1 + 1.99e-4)  delay(tNH - gt1 - 2.0*GRADIENT_DELAY);
   	else   delay(1.99e-4 - 2.0*GRADIENT_DELAY);

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

	sim3pulse(pw, 0.0, pwN, zero, zero, t10,  rof1, rof1);

	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	delay(lambda - 1.5*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, rof1, rof1);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	dec2phase(one);
	delay(lambda  - 1.5*pwN - gt5);

	sim3pulse(pw, 0.0, pwN, one, zero, one, rof1, rof1);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(1.5*gzlvl5, gt5);
	delay(lambda - 1.5*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, rof1, rof1);

	zgradpulse(1.5*gzlvl5, gt5);
	delay(lambda - pwN - 0.5*pw - gt5);

	rgpulse(pw, zero, rof1, rof1);
	delay((gt1/10.0) + 1.0e-4 +gstab - 0.5*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

	rgpulse(2.0*pw, zero, rof1, rof1);

	dec2power(dpwr2);				       /* POWER_DELAY */

        if (mag_flg[A] == 'y')
           magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);
        else
           zgradpulse(icosel*gzlvl2,gt1/10.0);
        delay(gstab);
        rcvron();
statusdelay(C,1.0e-4 - rof1);

	setreceiver(t11);
}		 
