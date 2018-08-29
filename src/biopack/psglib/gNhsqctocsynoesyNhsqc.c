/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gNhsqctocsynoesyNhsqc.c 
   

 3D HSQC-TOCSY-NOESY-HSQC gradient sens enhanced version, with N15 shifts
 in 1st & 2nd indirect dimensions. Applies intraresidue amideNH-alphaCH tocsy,
 then sequential amide NH-NH noesy. Suitable for beta-sheet/extended regions. 
 [Suits selective deuteration, but requires amide & alpha protons]

 pulse sequence: Zhang, Kay, Olivier and Forman-Kay JBNMR 4,845(1994); Fig. 1d
           (tau_b as Kay grp ref, tau_a~lambda, tau_c~gstab, grads simplified)

 optional magic-angle coherence transfer gradients

 Standard features include optional 13C sech/tanh pulse on CO and Cab to
 refocus 13C coupling during t1 and t2; one lobe sinc pulse to put H2O back
 along z (the sinc one-lobe is significantly more selective than gaussian,
 square, or seduce 90 pulses); preservation of H20 along z during t1 and the
 relaxation delays.

    sech/tanh pulse: 	Silver, Joseph and Hoult, JMR, 59, 347 (1984)
			Bendall, JMR, A116, 46 (1995)
         
 N.E.Preece Burnham Institute ,La Jolla, CA . Sep 2001: modified gtocsyNhsqc & gNtocsyhsqc.


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
    set f1180='n' for (0,0) in 1st 15N dim and f2180='n' for (0,0) in the 2nd
    N15 dim. f1180='y' is ignored if ni=0, and f2180='y' is ignored if ni2=0.

          	  DETAILED INSTRUCTIONS FOR USE OF gNhsqctocsynoesyNhsqc.c

         
    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for gNhsqctocsynoesyNhsqcq may be printed using:
                                      "printon man('gNhsqctocsynoesyNhsqcq') printoff".
             
    2. Apply the setup macro "gNhsqctocsynoesyNhsqc". This loads relevant parameter
       set and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 35ppm, and N15 
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
  
dipsi(phse1,phse2)
codeint phse1,phse2;

{
        double slpw5;
        slpw5 = 1.0/(4.0*getval("strength")*18.0);

        rgpulse(64*slpw5,phse1,0.0,0.0);
        rgpulse(82*slpw5,phse2,0.0,0.0);
        rgpulse(58*slpw5,phse1,0.0,0.0);
        rgpulse(57*slpw5,phse2,0.0,0.0);
        rgpulse(6*slpw5,phse1,0.0,0.0);
        rgpulse(49*slpw5,phse2,0.0,0.0);
        rgpulse(75*slpw5,phse1,0.0,0.0);
        rgpulse(53*slpw5,phse2,0.0,0.0);
        rgpulse(74*slpw5,phse1,0.0,0.0);

}

static int   phi1[2]  = {0,2},
             phi2[8]  = {0,0,1,1,2,2,3,3},
	     phi3[1]  = {0},
             rec[4]   = {0,2,2,0}; 

static double   d2_init=0.0, d3_init=0.0;


pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR],       /*magic-angle coherence transfer gradients */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            C13refoc[MAXSTR];   	/* C13 sech/tanh pulse in middle of t1*/
	   

 
int         icosel,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
            strength = getval("strength"),
            slpwr,
            slpw,
            slpw5,
            cycles,
            mixT = getval("mixT"),        /* TOCSY mix time, 30  msec in ref */
	    mixN = getval("mixN"),     	  /* NOESY mix time, 100 msec in ref */
            tau2,        				         /*  t2 delay */
	    lambda = 0.93/(4.0*getval("JNH")),	   /* 1/4J H1 evolution delay */
	    /* relaxation optimized true_lambda i.e. tau_a in Kay group ref */
            tau_b = getval("tau_b"),            /* 1.5 msec in Kay group ref */
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */
   compH = getval("compH"),        /* adjustment for H1 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */
   dof100,	      /* C13 frequency at 100ppm for both aliphatic & aromatic*/

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
    getstr("C13refoc",C13refoc);



/*   LOAD PHASE TABLE    */

	settable(t1,2,phi1);	/* 0,2 */
	settable(t2,8,phi2);    /* 0,0,1,1,2,2,3,3 */
        settable(t3,2,phi3);    /* 0 */
	settable(t11,4,rec);    /* 0,2,2,0 */

   slpw = 1/(4.0 * strength) ;           /* spinlock field strength  */
   slpw5 = slpw/18;
   slpwr = tpwr - 20.0*log10(slpw/(compH*pw));
   slpwr = (int) (slpwr + 0.5);
   cycles = (mixT/(2072*slpw5));
   cycles = 2.0*(double)(int)(cycles/2.0);
   initval(cycles, v9);             /* V9 is the mixT TOCSY loop count */

/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses (and initialize rfst) */
	rf0 = 4095.0;    rfst=0.0;

/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
 	dof100 = dof + 65.0*dfrq;
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

  if ( (tau_b - gt1/2 ) < 0.0 )
  { text_error("tau_b is too small. Make it equal to %f or more.\n",(gt1/2));
                                                                    psg_abort(1); }
  if ( (mixN + mixT - gt4 - gt5 ) < 0.0 )
  { text_error("mix times are too short. Make sum of mixN & mixT equal to %f or more.\n",(gt4 + gt5));
						   		    psg_abort(1); }



  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn'"); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' or 'nny' "); psg_abort(1); }

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

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t1,2,4); tsadd(t11,2,4);} 	
   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t11,2,4); }

/* BEGIN PULSE SEQUENCE */

status(A);
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	decoffset(dof);
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
	{decrgpulse(pwC, one, 0.0, 0.0);}
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

  	if ( (C13refoc[A]=='y') && (tau1 > 0.1e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY); 
          simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
          decphase(zero);
            delay(tau1 - 0.5e-3);}
	else
           {delay(tau1);
            rgpulse(2.0*pw, zero, 0.0, 0.0);
            delay(tau1);}

	dec2rgpulse(pwN, zero, 0.0, 0.0);
        zgradpulse(gzlvl4, gt4);

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

	txphase(zero);
   	obspower(tpwrs);
	delay(2.0e-4 - 2.0*GRADIENT_DELAY);
   	shaped_pulse("H2Osinc", pwHs, three, 5.0e-4, 0.0);
	obspower(tpwr);
	rgpulse(pw, zero, 0.0, zero);
	zgradpulse(gzlvl5, gt5);
	delay(lambda - 1.5*pwN - gt5);
	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	delay(lambda - 1.5*pwN - gt5 - GRADIENT_DELAY - 2.0*POWER_DELAY);
        decpwrf(rf0);
   	rgpulse(pw, one, 50.0e-6, 0.0); 
	
status(B);
				/* TOCSY mixT */
     obspower(slpwr);
          if (cycles > 1.0)
           {
             obspower(slpwr);
             starthardloop(v9);
              dipsi(one,three); dipsi(three,one); 
              dipsi(three,one); dipsi(one,three);
             endhardloop();
             obspower(tpwr);
           }
	delay(mixN);		/* NOESY mixN; TauM in ref Fig 1d*/
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
        delay(tau_b);
	dec2rgpulse(2.0*pwN, t2, 0.0, 0.0);
	delay(tau_b-gt1);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl1, gt1);   
                }
                else
                {
                   zgradpulse(gzlvl1, gt1);
                }
	dec2phase(t3);
	delay(2.0e-4 - 2.0*GRADIENT_DELAY);

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
	
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
	statusdelay(C,1.0e-5 -rof1);		

	setreceiver(t11);
}		 
