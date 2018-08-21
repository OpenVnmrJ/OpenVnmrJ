/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gNhsqcnoesyNhsqc3D.c 
    
 3D HSQC-NOESY-HSQC gradient sensitivity enhanced version with amide proton-
 proton NOESY. Attached N15 shifts are in first and second indirect dimension.
 Suitable for NH NH noes in alpha-helical domains and extended regions. 
 [suits full C-H deuteration if available, but requires amide protons]

 3D pulse sequence: Zhang, Forman-Kay, Shortle and Kay (1997); JBNMR 9,181(97)
    Fig. 3e (tau_a~lambda, tau_c~gstab tau_b renamed tau_b_K, grads simplified) 

    optional magic-angle coherence transfer gradients

    Standard features include optional 13C sech/tanh pulse on CO and Cab to 
    refocus 13C coupling during t1 and t2; one lobe sinc pulse to put H2O back 
    along z (the sinc one-lobe is significantly more selective than gaussian, 
    square, or seduce 90 pulses); preservation of H20 along z during t1 and the 
    relaxation delays. Fine power adjustment(tpwrsf) is available for last sinc pulse.

 C13 frequency should be set at 100ppm for both aliphatic & carbonyl refocussing

    sech/tanh pulse: 	Silver, Joseph and Hoult, JMR, 59, 347 (1984)
			Bendall, JMR, A116, 46 (1995)
         
    N.E. Preece ,Burnham Inst/UCSD. Sep 2001
    Modified gradient placements to permit recovery. Modified tau1/tau2 calculations
    to eliminate first order phase correction. Made the sequence 3D. GG palo alto 12/04.

***********************************IMPORTANT******************************************
    VNMR processing:
    ****************
    use lsfrq=sw2/2 for proper alignment of N15 shifts.

    The first t1 point is distorted because the lack of time for refocussing pulse(s).
    The consequent result is a large baseline offset along F1. This may be corrected via
    dc2d along F1 or by LP. If these methods are insufficient, the fpmult1 parameter
    may be adjusted iteratively to give no baseline offset in F1. This may require a
    negative value of fpmult1 which is currently limited to positive values. This may
    be changed by setlimit('fpmult1',100,-100,.001).     
**************************************************************************************

        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nnn', dmm = 'ccc' for 15N-only labelled compounds.
    Set C13refocus = 'y'  if also C13-labelled as well.      
    Set dm2 = 'nny', dmm2 = 'ccg/w/p') for N15 decoupling.
    (13C is refocussed in t1 and t2 by 180 degree pulses)

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [N15]  and t2 [N15].
   
    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) and f2180='n' for (0,0) in N15 dim.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0. In 4D mode use f3180.

          	  DETAILED INSTRUCTIONS FOR USE OF gNhsqcnoesyNhsqc.c

         
    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for gNhsqcnoesyNhsqcq may be printed using:
                                      "printon man('gNhsqcnoesyNhsqcq') printoff".
             
    2. Apply the setup macro "gNhsqcnoesyNhsqc". This loads relevant parameter
       set and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 100ppm, and N15 
       frequency on the amide region (120 ppm).

    4. Pulse and frequency calibrations are as for gNhsqc, or as generally
       used for BioPack.

    6. Splitting of resonances in the 1st or 2nd dimension by C13 coupling in 
       C13-enriched samples can be removed by setting C13refoc='y'.

    7. H2O preservation is achieved according to Kay et al, except that a sinc
       one-lobe selective pulse is used to put H2O back along z.  This is much
       more selective than a hard, Seduce1, or gaussian pulse.  H2O is cycled
       back to z as much as possible during t1 and t2.

*/


#include <standard.h>
  


static int   phi1[2]  = {0,2},
             phi2[8]  = {0,0,1,1,2,2,3,3},
	     phi3[1]  = {0},
             phi4[1]  = {0},
             rec[4]   = {0,2,2,0}; 

static double   d2_init=0.0, d3_init=0.0;


pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR],       /*magic-angle coherence transfer gradients */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
	    f3180[MAXSTR],	  /* In 4D mode, flag to start t3 @ halfdwell */
            C13refoc[MAXSTR];   	/* C13 sech/tanh pulse in middle of t1*/
	   

 
int         icosel,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
            t2_counter;  	 	        /* used for states tppi in t2 */

double      mix = getval("mix"),	                 /* NOESY mixing time */
	    tau1,					        /*   t1 delay */
            tau2,        				        /*   t2 delay */
	    tauNH = 1/(4.0*getval("JNH")),                      /* 1/4J JNH   */
            factor =  getval("factor"),     /* relaxation optimization factor */
            lambda,                            /* relaxation optimized tauNH  */
            tau_b_K = getval("tau_b_K"),    /* echo at head of Sens Enh train */
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rfst,	                           /* fine power for the stCall pulse */
   compH = getval("compH"),        /* adjustment for H1 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */
        tpwrsf = getval("tpwrsf"),    /*fine power adj for flipback if used   */       
       
        pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),
  
        gstab=getval("gstab"),
        gzcal=getval("gzcal"),
	gt0 = getval("gt0"),
	gt1 = getval("gt1"),
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
        gzlvl0 = getval("gzlvl0"),
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5");
    getstr("f1180",f1180);
    getstr("mag_flg",mag_flg);
    getstr("f2180",f2180);
    getstr("f3180",f3180);
    getstr("C13refoc",C13refoc);



/*   LOAD PHASE TABLE    */

	settable(t1,2,phi1);	/* 0,2 */
	settable(t2,8,phi2);    /* 0,0,1,1,2,2,3,3 */
        settable(t3,2,phi3);    /* 0 */
        settable(t4,2,phi4);    /* 0 */
	settable(t11,4,rec);    /* 0,2,2,0 */

/*   INITIALIZE VARIABLES   */

/* relaxation optimized factor of JNH evolution delay */
	lambda = factor * tauNH;

/* fine power for C13 adiabatic pulses (initialize rfst) */
	rfst=0.0;

/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
    if (C13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
	rfst = (int) (rfst + 0.5);
	if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }}

    /* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/((compH*pw)*1.69));   /* needs 1.69 times more */
	tpwrs = (int) (tpwrs);                   /* power than a square pulse */


/* CHECK VALIDITY OF PARAMETER RANGES */
                                                                     
  if ( (tau_b_K - gt1/2 ) < 0.0 )
  { text_error("tau_b_K is too small. Make it equal to %f or more.\n",(gt1/2));
						   		    psg_abort(1); }
  if ( (mix - gt4 - gt5 ) < 0.0 )
  { text_error("mix is too small. Make mix equal to %f or more.\n",(gt4 + gt5));
                                                                    psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn'"); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' or 'nny' "); 
								    psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }

  if( pw > 20.0e-6 )
  { text_error("dont fry the probe, pw too high ! "); psg_abort(1); } 
  
  if( pwN > 100.0e-6 )
  { text_error("dont fry the probe, pwN too high ! "); psg_abort(1); }



/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2) 
        tsadd(t1,1,4);
  
    if (phase2 == 1) 
	{tsadd(t3,2,4); icosel = 1; }
    else icosel = -1;

    if (phase3 == 2)
        tsadd(t4,1,4); 

/*  Set up f1180  */
   
    tau1 = d2;
    if(f1180[A] == 'y')
	 tau1 += ( 1.0 / (2.0*sw1)); 
    if (C13refoc[A] == 'y') 
      tau1=tau1 -(4.0*pwN/PI);
     else
      tau1=tau1 -pw -(4.0*pwN/PI);
    tau1 = tau1/2.0;
    if(tau1 < 0.2e-6) tau1 = 0.0; 

/*  Set up f2180  */
    tau2 = d3;
    if(f2180[A] == 'y')
	 tau2 += ( 1.0 / (2.0*sw2)); 
    if (C13refoc[A] == 'y') 
      tau2=tau2 -(4.0*pwN/PI);
     else
      tau2=tau2 -pw -(4.0*pwN/PI);
    tau2 = tau2/2.0;
    if(tau2 < 0.2e-6) tau2 = 0.0; 


/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t1,2,4); tsadd(t11,2,4); }
 	
   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t11,2,4); }


/* BEGIN PULSE SEQUENCE */

status(A);
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	txphase(zero);
	decphase(zero);
        dec2phase(zero);

	delay(d1);

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 magnetization*/
        zgradpulse(gzlvl0, 0.5e-3);
	delay(gstab);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	zgradpulse(1.3*gzlvl0, 0.5e-3);
        decpwrf(rfst);

   	txphase(t1);
   	decphase(zero);
   	dec2phase(zero);
	initval(135.0,v1);
        obsstepsize(1.0);
        xmtrphase(v1);
	delay(gstab);
	rcvroff();

   	rgpulse(pw, zero, 50.0e-6, 0.0);
	zgradpulse(gzlvl0,gt0);

   	dec2phase(zero);
	delay(lambda - 1.5*pwN - gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);  

   	txphase(one);
	zgradpulse(gzlvl0,gt0);
	delay(lambda - 1.5*pwN - gt0);
 	rgpulse(pw, one, 0.0, 0.0);
	txphase(two);
   	obspower(tpwrs);
   	shaped_pulse("H2Osinc", pwHs, two, 5.0e-4, 0.0);
	obspower(tpwr);
	zgradpulse(gzlvl3, gt3);
	dec2phase(t1);
	delay(gstab);
   	dec2rgpulse(pwN, t1, 0.0, 0.0);
	decphase(zero);

        if (tau1>0.0)
        {
       	if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY); 
          simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
            delay(tau1 - 0.5e-3);}
	else
           {delay(tau1);
            rgpulse(2.0*pw, zero, 0.0, 0.0);
            delay(tau1);}
        }

	dec2rgpulse(pwN, zero, 0.0, 0.0);
        zgradpulse(gzlvl4, gt4);

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

	txphase(zero);
   	obspower(tpwrs);
	delay(gstab - 2.0*GRADIENT_DELAY);
   	shaped_pulse("H2Osinc", pwHs, three, 5.0e-4, 0.0);
	obspower(tpwr);
	rgpulse(pw, t4, 0.0, 0.0);
        zgradpulse(gzlvl5, gt5);
        delay(lambda - 1.5*pwN - gt5); 
	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        zgradpulse(gzlvl5, gt5);
        delay(lambda - 1.5*pwN - gt5 - GRADIENT_DELAY - 2.0*POWER_DELAY); 
   	rgpulse(pw, one, 50.0e-6, 0.0); 
	
status(B);
				/* NOE mix */
	delay(mix-gstab);

	zgradpulse(1.1*gzlvl4, gt4);   	
        delay(gstab);
	rgpulse(pw, three, rof1,0.0);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - 1.5*pwN - gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	txphase(one);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - 1.5*pwN - gt0);
 	rgpulse(pw, zero, 0.0, 0.0);
	txphase(two);
        if (tpwrsf<4095.0)
        {
       	 obspower(tpwrs+6.0); obspwrf(tpwrsf);
   	 shaped_pulse("H2Osinc", pwHs, three, 5.0e-4, 0.0);
	 obspower(tpwr); obspwrf(4095.0);
        }
        else
        {
       	 obspower(tpwrs);
   	 shaped_pulse("H2Osinc", pwHs, two, 5.0e-4, 0.0);
	 obspower(tpwr);
        }
	zgradpulse(1.1*gzlvl3, gt3);
	dec2phase(zero);
	delay(gstab);
   	dec2rgpulse(pwN, zero, 0.0, 0.0);
	decphase(zero);


  	if ( (C13refoc[A]=='y') && (tau2 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau2 - 0.5e-3 - WFG2_START_DELAY); 
          simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
            delay(tau2 - 0.5e-3);}
	else
           {delay(tau2);
            rgpulse(2.0*pw, zero, 0.0, 0.0);
            delay(tau2);}
        delay(gstab);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(-gzcal*gzlvl1, gt1/2);   
                }
                else
                {
                   zgradpulse(-gzlvl1, gt1/2);
                }

        delay(tau_b_K-gt1/2);
	dec2rgpulse(2.0*pwN, t2, 0.0, 0.0);
	delay(tau_b_K-gt1/2);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl1, gt1/2);   
                }
                else
                {
                   zgradpulse(gzlvl1, gt1/2);
                }
	dec2phase(t3);
	delay(gstab - 2.0*GRADIENT_DELAY);

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
	
	sim3pulse(pw, 0.0, pwN, zero, zero, t3, 0.0, 0.0);

	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	delay(lambda - 1.5*pwN - gt5);
	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	dec2phase(one);
	delay(lambda  - 1.5*pwN - gt5);
	sim3pulse(pw, 0.0, pwN, one, zero, one, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(1.5*gzlvl5, gt5);
	delay(lambda - 1.5*pwN - gt5);
	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(1.5*gzlvl5, gt5);
	delay(lambda - pwN - 0.5*pw - gt5);

	rgpulse(pw, zero, 0.0, 0.0);
        decpwrf(4095.0);
        dec2power(dpwr2);
        delay(gt1/10+gstab-0.5*pw + 2.0*GRADIENT_DELAY - 2.0*POWER_DELAY);

	rgpulse(2.0*pw, zero, 0.0, 0.0);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(icosel*icosel*gzcal*gzlvl2, gt1/10);
                }
                else
                   zgradpulse(icosel*gzlvl2, gt1/10);
        delay(gstab);
        rcvron();
	status(C);		

	setreceiver(t11);
}		 
