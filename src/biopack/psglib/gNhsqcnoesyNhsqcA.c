/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  3D/(4D) gNhsqcnoesyNhsqcA.c v1.10
    
 3D HSQC-NOESY-HSQC gradient sensitivity enhanced version with amide proton-
 proton NOESY. Attached N15 shifts are in first and second indirect dimension.
 Suitable for NH NH noes in alpha-helical domains and extended regions. 
 [suits full C-H deuteration if available, but requires amide protons]

 In 4D mode (i.e. ni3 > ~24 phase3=1,2) source proton shift is also collected 
 with semi-constant time in an optional 3rd indirectly detected dimension.
 [3rd party processing software is required for 4D processing]

 3D pulse sequence: Zhang, Forman-Kay, Shortle and Kay (1997); JBNMR 9,181(97)
    Fig. 3e (tau_a~lambda, tau_c~gstab tau_b renamed tau_b_K, grads simplified) 

 4D (non-se.wg): Grzesiek, Wingfield, Stahl, Kaufman and Bax JACS 117,9594(95)
            (tau_a, tau_b, tau_c kept for the semi-constant time calculation)
 
    optional magic-angle coherence transfer gradients

    Standard features include optional 13C sech/tanh pulse on CO and Cab to 
    refocus 13C coupling during t1 and t2; one lobe sinc pulse to put H2O back 
    along z (the sinc one-lobe is significantly more selective than gaussian, 
    square, or seduce 90 pulses); preservation of H20 along z during t1 and the 
    relaxation delays.

    sech/tanh pulse: 	Silver, Joseph and Hoult, JMR, 59, 347 (1984)
			Bendall, JMR, A116, 46 (1995)
         
    N.E. Preece ,Burnham Inst/UCSD. Sep 2001
    Auto-calibrated version, E.Kupce, 27.08.2002.

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

static int   phi1[2]  = {0,2},
             phi2[8]  = {0,0,1,1,2,2,3,3},
	     phi3[1]  = {0},
             phi4[1]  = {0},
             rec[4]   = {0,2,2,0}; 

static double   d2_init=0.0, d3_init=0.0, d4_init=0.0;
static double   H1ofs=4.7, C13ofs=100.0, N15ofs=120.0, H2ofs=0.0;

static shape    stC200;

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
            t2_counter,  	 	        /* used for states tppi in t2 */
            t3_counter,             /* In 4D mode, used for states tppi in t3 */
	    ni2 = getval("ni2"),
	    ni3 = getval("ni3");

double      mix = getval("mix"),	                 /* NOESY mixing time */
	    tau1,					        /*   t1 delay */
            tau2,        				        /*   t2 delay */
            tau3,                                               /*   t3 delay */
	    tauNH = 1/(4.0*getval("JNH")),                      /* 1/4J JNH   */
            factor =  getval("factor"),     /* relaxation optimization factor */
            lambda,                            /* relaxation optimized tauNH  */
            tau_a = 0.0,  /* In 4D mode semi-constant time (SCT)  delays ~t3a */
	    tau_b = 0.0,					      /* ~t3b */
	    tau_c = 0.0,					      /* ~t3c */
            tau_b_K = getval("tau_b_K"),    /* echo at head of Sens Enh train */
                               /* temporary Pbox parameters */
   bw, pws, ofs, ppm, nst,  /* bandwidth, pulsewidth, offset, ppm, # steps */     
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
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
        sw3 = getval("sw3"),
  
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

/* maximum fine power for pwC pulses (and initialize rfst) */
	rf0 = 4095.0;    rfst=0.0;

      setautocal();                        /* activate auto-calibration flags */ 
        
      if (autocal[0] == 'n') 
      {
        /* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
        if (C13refoc[A]=='y')
        {
        rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
	  rfst = (int) (rfst + 0.5);
	  if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }
        }
      }  
      else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
      {
        if(FIRST_FID)                                            /* call Pbox */
        {
          if (C13refoc[A]=='y')
          {
            ppm = getval("dfrq"); ofs = 0.0;   pws = 0.001;  /* 1 ms long pulse */
            bw = 200.0*ppm;       nst = 1000;          /* nst - number of steps */
            stC200 = pbox_makeA("stC200", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
          }
          ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
        }
        if (C13refoc[A]=='y') rfst = stC200.pwrf;
      }

/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */

    /* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/((compH*pw)*1.69));   /* needs 1.69 times more */
	tpwrs = (int) (tpwrs);                   /* power than a square pulse */

/* ISSUE 4D MODE WARNING ON FIRST INCREMENT */

if ((ni>0.0) && (ni2>0.0) && (ni3>0.0) && (ix == 1)) 
  { text_error("VNMR (6.1C and earlier) can\'t yet process 4D data, requires 3rd party processing");} 

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
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) - (4*pwN/PI)); 
          if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;

/*  Set up f2180  */

    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 1.0)) 
	{ tau2 += ( 1.0 / (2.0*sw2) - (4*pwN/PI)); 
          if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2/2.0;

/*  Set up f3180  */

    tau3 = d4;
    if((f3180[A] == 'y') && (ni3 > 1.0))
        { tau3 += ( 1.0 / (2.0*sw3) - (4*pw/PI)); 
          if(tau3 < 0.2e-6) tau3 = 0.0; }
    tau3 = tau3/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t1,2,4); tsadd(t11,2,4); }
 	
   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t11,2,4); }

   if( ix == 1) d4_init = d4;
   t3_counter = (int) ( (d4-d4_init)*sw3 + 0.5 );
   if(t3_counter % 2)
        { tsadd(t4,2,4); tsadd(t11,2,4); }

/* If in 4D mode set-up semi-constant time for an indirect proton dimension */
/* hyperbolic sheila could be edited in here instead but still necessary to */
/* correct for GRADIENT_DELAY,POWER_DELAY else negative delays near ni3 max */

 if(tau3 > 0)
 {
   tau_a = lambda + (double) t3_counter*((((double) ni3/sw3)
                  + (2.0*(tauNH - gt5 - 1.5*pwN)))/2.0 - lambda)/((double)(ni3 - 1));
   tau_b = (double) t3_counter*((((double) ni3/sw3)
                  - (2.0*(tauNH - gt5 - 1.5*pwN)))/((double) (2*(ni3 - 1))));
   tau_c = lambda - (double) t3_counter*(lambda/((double) (ni3 - 1)));
 }

/* BEGIN PULSE SEQUENCE */

status(A);
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	decpwrf(rf0);
	txphase(zero);
	decphase(zero);
        dec2phase(zero);

	delay(d1);

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 magnetization*/
	if (C13refoc[A] == 'y' )		   /* and 13C if required */
	{decrgpulse(pwC, one, 0.0, 0.0);} 
        zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
	dec2rgpulse(pwN, one, 0.0, 0.0);
        if (C13refoc[A] == 'y' )
	{decrgpulse(pwC, one, 0.0, 0.0); decpwrf(rfst);}
	zgradpulse(1.3*gzlvl0, 0.5e-3);

   	txphase(t1);
   	decphase(zero);
   	dec2phase(zero);
	initval(135.0,v1);
        obsstepsize(1.0);
        xmtrphase(v1);
	delay(5.0e-4);
	rcvroff();

   	rgpulse(pw, zero, 50.0e-6, 0.0);
	zgradpulse(gzlvl0,gt0);

   	dec2phase(zero);
	delay(lambda - 1.5*pwN - gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);  

   	txphase(one);
	delay(lambda - 1.5*pwN - gt0);
	zgradpulse(gzlvl0,gt0);
 	rgpulse(pw, one, 0.0, 0.0);
	txphase(two);
   	obspower(tpwrs);
   	shaped_pulse("H2Osinc", pwHs, two, 5.0e-4, 0.0);
	obspower(tpwr);
	zgradpulse(gzlvl3, gt3);
	dec2phase(t1);
	delay(2.0e-4);
   	dec2rgpulse(pwN, t1, 0.0, 0.0);
	decphase(zero);

        if (tau1>0.0)
        {
       	if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY); 
          simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
          decphase(zero);
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
	delay(2.0e-4 - 2.0*GRADIENT_DELAY);
   	shaped_pulse("H2Osinc", pwHs, three, 5.0e-4, 0.0);
	obspower(tpwr);
	rgpulse(pw, t4, 0.0, 0.0);
        if (tau3 >  0)
	{
        zgradpulse(gzlvl5, gt5);
        delay(tau_a);  
	rgpulse(pw*2, zero, 0.0, 0.0);
	delay(tau_b);
	dec2rgpulse(pwN*2, zero, 0.0, 0.0);
        delay(tau_c);
/* omitted grad dly corrections in 4D mode for SCT in tau_a,b calc, see above */
        zgradpulse(gzlvl5, gt5);
 	}
	else
	{
        zgradpulse(gzlvl5, gt5);
        delay(lambda - 1.5*pwN - gt5); 
	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        delay(lambda - 1.5*pwN - gt5 - GRADIENT_DELAY - 2.0*POWER_DELAY); 
        zgradpulse(gzlvl5, gt5);
	}
        decpwrf(rf0);
   	rgpulse(pw, one, 50.0e-6, 0.0); 
	
status(B);
				/* NOE mix */
	delay(mix);

	zgradpulse(1.1*gzlvl4, gt4);   	
	rgpulse(pw, three, 200.0e-6,0.0);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - 1.5*pwN - gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	txphase(one);
	delay(lambda - 1.5*pwN - gt0);
	zgradpulse(gzlvl0, gt0);
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
	delay(2.0e-4);
   	dec2rgpulse(pwN, zero, 0.0, 0.0);
	decphase(zero);


  	if ( (C13refoc[A]=='y') && (tau2 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau2 - 0.5e-3 - WFG2_START_DELAY); 
          simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
          decphase(zero);
            delay(tau2 - 0.5e-3);}
	else
           {delay(tau2);
            rgpulse(2.0*pw, zero, 0.0, 0.0);
            delay(tau2);}
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
                   magradpulse(gzcal*gzlvl1, gt1/1);   
                }
                else
                {
                   zgradpulse(gzlvl1, gt1/2);
                }
	dec2phase(t3);
	delay(2.0e-4 - 2.0*GRADIENT_DELAY);

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
        dec2power(dpwr2);
        delay(gt1/10+1.0e-5+gstab-0.5*pw + 2.0*GRADIENT_DELAY - POWER_DELAY);

	rgpulse(2.0*pw, zero, 0.0, rof1);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(icosel*icosel*gzcal*gzlvl2, gt1/10);
                }
                else
                   zgradpulse(icosel*gzlvl2, gt1/10);
        delay(gstab);
        rcvron();
	statusdelay(C,1.0e-5 -rof1);		

	setreceiver(t11);
}		 
