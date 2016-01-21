/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_gnoesyNhsqc.c


    This pulse sequence will allow one to perform the following experiment:

    3D NOESY-HSQC gradient sensitivity enhanced version for 15N, with
    H1-H1 NOESY in the first dimension, and N15 shifts in the second.

    optional magic-angle coherence transfer gradients

    Standard features include optional 13C sech/tanh pulse on CO and Cab to 
    refocus 13C coupling during t1 and t2; one lobe sinc pulse to put H2O back 
    along z (the sinc one-lobe is significantly more selective than gaussian, 
    square, or seduce 90 pulses); preservation of H20 along z during t1 and the 
    relaxation delays; option of obtaining spectrum of only NH2 groups;

    pulse sequence: 	Kay, Keifer and Saarinen, JACS, 114, 10663 (1992)
    			Zhang et al, J Biol NMR, 4, 845 (1994)
    sech/tanh pulse: 	Silver, Joseph and Hoult, JMR, 59, 347 (1984)
			Bendall, JMR, A116, 46 (1995)
         
    Written by MRB, January 1998, starting with gNhsqc from BioPack.
    Revised and improved to a standard format by MRB, BKJ and GG for the 
    BioPack, January 1998, so as to include calculation of the above 
    standard features within the pulse sequence code and associated macro.

    modified from proteinpack/gnoesyNhsqc.c for RNA  GG 10/4/00 based
    on Peter Lukavsky version.


        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nnn', dmm = 'ccc'
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for N15 decoupling.

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [H1]  and t2 [N15].
   
    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in H1 and f2180='n' for (0,0) in N15.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.



          	  DETAILED INSTRUCTIONS FOR USE OF gnoesyNhsqc

         
    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for gnoesyNhsqc may be printed using:
                                      "printon man('gnoesyNhsqc') printoff".
             
    2. Apply the setup macro "gnoesyNhsqc".  This loads the relevant parameter
       set and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 110ppm, and N15 
       frequency at 200 ppm.

    4. Pulse and frequency calibrations are as for gNhsqc, or as generally
       used for BioPack.

    6. Splitting of resonances in the 1st or 2nd dimension by C13 coupling in 
       C13-enriched samples can be removed by setting C13refoc='y'.

    7. H2O preservation is achieved according to Kay et al, except that a sinc
       one-lobe selective pulse is used to put H2O back along z.  This is much
       more selective than a hard, Seduce1, or gaussian pulse.  H2O is cycled
       back to z as much as possible during t1 and t2.

    8. NH2 GROUPS: (set amino='y')
       A spectrum of NH2 groups, with NH groups cancelled, can be obtained
       with flag NH2only='y'.  This utilises a 1/2J (J=94Hz) period of NH 
       J-coupling evolution added to t1 which cancels NH resonances and 
       inverts NH2 resonances (normal INEPT behaviour).  A 180 degree phase
       shift is added to a N15 90 pulse to provide positive NH2 signals.  The 
       NH2 resonances will be smaller (say 80%) than when NH2only='n'.

   SETETUP: dof2 is changed automatically
        imino='y'
        -IMINO-REGION:  sw=22ppm
                        sw1=22ppm
                        sw2=25ppm (folds aminos)
                        dof=110ppm
                        dof2=155ppm
        amino='y'
        -AMINO-HMQC:    sw=22ppm
                        sw1=22ppm
                        sw2=60ppm (folds iminos)
                        dof=110ppm
                        dof2=85ppm

        dof is set to 110 ppm (middle of C13 resonances).
        Always check the proper folding by running a quick NH plane by setting
        ni=0,phase=1 and ni2=32,phase=1,2.

*/



#include <standard.h>
  


static int   phi1[2]  = {0,2},
	     phi3[4]  = {0,0,2,2},
	      
             phi9[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
	     phi10[1] = {0}, 
             rec[8]   = {0,2,2,0,2,0,0,2};

static double   d2_init=0.0, d3_init=0.0;


pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            imino[MAXSTR], amino[MAXSTR],
            mag_flg[MAXSTR],       /*magic-angle coherence transfer gradients */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            C13refoc[MAXSTR],		/* C13 sech/tanh pulse in middle of t1*/
	    NH2only[MAXSTR];		       /* spectrum of only NH2 groups */

 
int         icosel,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
	    mix = getval("mix"),		 	    /* NOESY mix time */
            tau2,        				         /*  t2 delay */
	    lambda = 0.94/(4.0*getval("JNH")),	    /* 1/4J H1 evolution delay */
	    tNH = 1.0/(4.0*getval("JNH")),	  /* 1/4J N15 evolution delay */

        
        
/* the sech/tanh pulse is automatically calculated by the macro "proteincal", */  /* and is called directly from your shapelib.                  		      */
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */
   compH = getval("compH"),         /* adjustment for H1 amplifier compression */
   compC = getval("compC"),         /* adjustment for C13 amplifier compression */

   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

        gzcal=getval("gzcal"),
	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl6 = getval("gzlvl6"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5");

    getstr("f1180",f1180);
    getstr("imino",imino); getstr("amino",amino);
    getstr("mag_flg",mag_flg);
    getstr("f2180",f2180);
    getstr("C13refoc",C13refoc);
    getstr("NH2only",NH2only);



/*   LOAD PHASE TABLE    */

	settable(t1,2,phi1);
	settable(t3,4,phi3);

	settable(t9,16,phi9);
	settable(t10,1,phi10);
	settable(t11,8,rec);




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

    /* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/((compH*pw)*1.69));   /* needs 1.69 times more */
	tpwrs = (int) (tpwrs);                   /* power than a square pulse */




/* CHECK VALIDITY OF PARAMETER RANGES */

  if ( (mix - gt4 - gt5) < 0.0 )
  { text_error("mix is too small. Make mix equal to %f or more or reduce gt4+gt5.\n",(gt4 + gt5));
						   		    psg_abort(1); }

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

    if (phase1 == 2) 
	 tsadd(t1,1,4);  
    if (phase2 == 1) 
	{ tsadd(t10,2,4); icosel = 1; }
    else icosel = -1; 


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
	{ tsadd(t1,2,4); tsadd(t11,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t11,2,4); }



/*  Correct inverted signals for NH2 only spectra  */

   if (NH2only[A]=='y') 
      { tsadd(t3,2,4); }

if ( ((imino[A]=='y') && (amino[A]=='y')) )
   {
        printf(" Choose  ONE  of  imino='y'  OR  amino='y' ");
        psg_abort(1);
   }
 
  if ( ((imino[A]=='y') && (NH2only[A]=='y')) )
   {
        printf(" NH2only='y' only for amino='y'! ");
        psg_abort(1);
   }

/* BEGIN PULSE SEQUENCE */

status(A);
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
        if (imino[A] == 'y')
	  dec2offset(dof2 - 45*dfrq2);
        if (amino[A] == 'y')
	  dec2offset(dof2 - 115*dfrq2);
	decpwrf(rf0);
	txphase(zero);
        dec2phase(zero);

	delay(d1);

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);

   	txphase(t1);
   	decphase(zero);
   	dec2phase(zero);
	initval(135.0,v1);
	obsstepsize(1.0);
	xmtrphase(v1);
	delay(5.0e-4);
	rcvroff();


   	rgpulse(pw, t1, 50.0e-6, 0.0);                     /* 1H pulse excitation */

        xmtrphase(zero);					/* SAPS_DELAY */
   	txphase(zero);
	if (C13refoc[A]=='y')
         {decpower(pwClvl-3.0); dec2power(pwNlvl-3.0);}

  if (tau1 > (2.0*GRADIENT_DELAY + pwN*1.4 + 0.64*pw + 5.0*SAPS_DELAY))  
   {
    if (C13refoc[A]=='y')
    {
     zgradpulse(gzlvl6, 0.8*(tau1 - 2.0*GRADIENT_DELAY - 1.4*pwN - 0.64*pw));
     delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - 1.4*pwN - 0.64*pw) - SAPS_DELAY);
     sim3pulse(0.0, 2.0*pwC*1.4, 2.0*pwN*1.4, zero, zero, zero, 0.0, 0.0);
     zgradpulse(-1.0*gzlvl6, 0.8*(tau1 - 2.0*GRADIENT_DELAY - 1.4*pwN - 0.64*pw));
     delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - 1.4*pwN - 0.64*pw));
    }
  else
    {
     zgradpulse(gzlvl6, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
     delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw) - SAPS_DELAY);
     dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
     zgradpulse(-1.0*gzlvl6, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
     delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
    } 
  }
  else if (tau1 > (0.64*pw + 0.5*SAPS_DELAY))
    delay(2.0*tau1 - 2.0*0.64*pw - SAPS_DELAY );
   	rgpulse(pw, zero, 0.0, 0.0);
	if (C13refoc[A]=='y')
         {decpower(pwClvl); dec2power(pwNlvl);}

	delay(mix - gt4 - gt5 -400.0e-6);
	zgradpulse(gzlvl4, gt4);
	dec2rgpulse(pwN, zero, 200.0e-6, 0.0);
	zgradpulse(gzlvl5, gt5);

   	rgpulse(pw, zero, 200.0e-6,0.0);			       /* HSQC begins */

   	dec2phase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	txphase(one);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

 	rgpulse(pw, one, 0.0, 0.0);
	txphase(two);
   	obspower(tpwrs);
   	shaped_pulse("H2Osinc", pwHs, two, 5.0e-4, 0.0);
	obspower(tpwr);
	zgradpulse(gzlvl3, gt3);
	dec2phase(t3);
	decpwrf(rfst);
	delay(2.0e-4);
   	dec2rgpulse(pwN, t3, 0.0, 0.0);
	decphase(zero);


/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */

        txphase(zero);
	dec2phase(t9);

if (NH2only[A]=='y')	
{      
    	delay(tau2);
         			  /* optional sech/tanh pulse in middle of t2 */
    	if (C13refoc[A]=='y') 				   /* WFG_START_DELAY */
           {decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);
            delay(tNH - 1.0e-3 - WFG_START_DELAY - 2.0*pw);}
    	else
           {delay(tNH - 2.0*pw);}
    	rgpulse(2.0*pw, zero, 0.0, 0.0);
    	if (tNH < gt1 + 1.99e-4)  delay(gt1 + 1.99e-4 - tNH);

    	delay(tau2);

    	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl1, gt1);
                }
                else
                {
                   zgradpulse(gzlvl1, gt1);
                }
    	dec2phase(t10);
   	if (tNH > gt1 + 1.99e-4)  delay(tNH - gt1 - 2.0*GRADIENT_DELAY);
   	else   delay(1.99e-4 - 2.0*GRADIENT_DELAY);
}

else
{
  	if ( (C13refoc[A]=='y') && (tau2 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau2 - 0.5e-3 - WFG2_START_DELAY); /* WFG2_START_DELAY */
            simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
            delay(tau2 - 0.5e-3);
            delay(gt1 + 2.0e-4);}
	else
           {delay(tau2);
            rgpulse(2.0*pw, zero, 0.0, 0.0);
            delay(gt1 + 2.0e-4 - 2.0*pw);
            delay(tau2);}

	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);

                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl1, gt1);
                }
                else
                {
                   zgradpulse(gzlvl1, gt1);
                }
	dec2phase(t10);
	delay(2.0e-4 - 2.0*GRADIENT_DELAY);
}

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

	sim3pulse(pw, 0.0, pwN, zero, zero, t10, 0.0, 0.0);

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

	delay((gt1/10.0) + 1.0e-4 - 0.5*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

	rgpulse(2.0*pw, zero, 0.0, rof1);
	dec2power(dpwr2);				       /* POWER_DELAY */
                if (mag_flg[A] == 'y')
                {
                   magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);
                }
                else
                {
                   zgradpulse(icosel*gzlvl2, gt1/10.0);
                }
        rcvron();
	statusdelay(C,1.0e-4-rof1);		

	setreceiver(t11);
}		 
