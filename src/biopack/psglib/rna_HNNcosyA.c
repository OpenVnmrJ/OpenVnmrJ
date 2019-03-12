/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_HNNcosyA.c - autocalibrated version with pulse shaping "on fly".

    This pulse sequence will allow one to perform the following experiment:

    TROSY-type correlation of imino H-N frequencies to the adjacent
    J-correlated 15N nuclei (i.e. across the hydrogen-bond).


    pulse sequence: 	Dingley and Grzesiek, JACS, 120, 8293 (1998)
     

                        NOTE: dof must be set at 110ppm always
                        NOTE: dof2 must be set at 200ppm always

        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nnn', dmm = 'ccc'
    Set dm2 = 'nnn', dmm2 = 'ccc'

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.
    f1coef='0 -1 0 1 0 -1 0 -1'.



          	  DETAILED INSTRUCTIONS FOR USE OF rna_HNNcosy

         
    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro: 
                                      "printon man('RnaPack') printoff".
       These Detailed Instructions for rna_HNNcosy may be printed using:
                                      "printon man('rna_HNNcosy') printoff".
             
    2. Apply the setup macro "rna_HNNcosy".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 110ppm, and N15 
       frequency on the N-aromatic region (200ppm).
       The dof2-value will be shifted to 185ppm automatically by the sequence
       code; set sw1 to 100ppm.

    4. CALIBRATION OF pw AND pwN:
       Should be done in rna_gNhsqc.

    5. CALIBRATION OF dof2:  
       Should be done in rna_gNhsqc.

    6. Splitting of resonances in the N15 dimension by C13 coupling in C13
       enriched samples can be removed by setting C13refoc='y'.

    7. H2O preservation is achieved according to Kay et al, except that a sinc
       one-lobe selective pulse is used to put H2O back along z.  This is much
       more selective than a hard, Seduce1, or gaussian pulse.  H2O is cycled
       back to z as much as possible.

    8. The coherence-transfer gradients using power levels
       gzlvl1 and gzlvl2 may be either z or magic-angle gradients. For the
       latter, a proper /vnmr/imaging/gradtable entry must be present and
       syscoil must contain the value of this entry (name of gradtable). The
       amplitude of the gzlvl1 and gzlvl2 should be lower than for a z axis
       probe to have the x and y gradient levels within the 32k range. For
       any value, a dps display (using power display) shows the x,y and z
       dac values. These must be <=32k.

       If water suppression is not efficient or exchanging iminos are lost,
       optimize gzlvl1 and gzlvl2 by arraying them together. Take the best
       compromise between S/N for iminos and water suppression. Then optimize
       gzlvl2 in small steps for optimum signal (on a 500MHz INOVA, best results
       could be achieved with gzlvl1=10000 and gzlvl2=10090).

    9. 1/4J DELAY and timeT DELAY:
       These are determined from the NH coupling constant, JNH, listed in 
       dg for possible change by the user. lambda is the 1/4J time for H1 NH 
       coupling evolution. lambda is usually set a little lower than the
       theoretical time to minimize relaxation.
       timeT is the transfer time from N15 to N15 across hydrogen-bond.
       Usually set to 15-20ms.


        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        @                                                      @
        @   Written for RnaPack by Peter Lukavsky (05/99).     @
        @                                                      @
        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        Auto-calibrated version, E.Kupce, 27.08.2002.
*/



#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */  

	     
static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
                      phx[1]={0},   phy[1]={1},   ph_x[1]={2},     ph_y[1]={3},   

             phi3[4]  = {0,3,2,1},
	     phi5[8]  = {3,2,1,0,1,0,3,2},
             phi6[16] = {1,0,3,2,1,0,3,2,3,2,1,0,3,2,1,0},
             recT[4]  = {0,1,2,3};


static double   d2_init=0.0;
static double   H1ofs=4.7, C13ofs=110.0, N15ofs=200.0, H2ofs=0.0;

static shape H2Osinc, stC140;

void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
	    mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
            bottom[MAXSTR],
            right[MAXSTR],
	    C13refoc[MAXSTR];		/* C13 sech/tanh pulse in middle of t1*/
 
int         t1_counter;  		        /* used for states tppi in t1 */

double      tau1,         				         /*  t1 delay */
	    lambda = 0.91/(4.0*getval("JNH")), 	   /* 1/4J H1 evolution delay */
	    timeT = getval("timeT"),	    /* HN->N transfer time: T=15-20ms */
        
/* the sech/tanh pulse is automatically calculated by the macro "rna_cal", */
/* and is called directly from your shapelib.                  		      */
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rfC,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */
                               /* temporary Pbox parameters */
   bw, pws, ofs, ppm, nst,  /* bandwidth, pulsewidth, offset, ppm, # steps */

   compH = getval("compH"),        /* adjustment for H1 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),

 grecov = getval("grecov"),   /* Gradient recovery delay, typically 150-200us */

        gt1 = getval("gt1"),                   /* coherence pathway gradients */
        gzcal = getval("gzcal"),               /* dac to G/cm conversion      */
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5");


    getstr("f1180",f1180);
    getstr("mag_flg",mag_flg);
    getstr("C13refoc",C13refoc);
    getstr("bottom",bottom);
    getstr("right",right);

/*   LOAD PHASE TABLE    */
	
        settable(t3,4,phi3);
        /*settable(t4,1,ph_y);*/
        settable(t1,1,phx);
        settable(t5,8,phi5);
        settable(t6,16,phi6);

        if (bottom[A]=='y')
        settable(t4,1,ph_y);
        else
        settable(t4,1,phy);

        if (right[A]=='y')
        settable(t10,1,ph_x);
        else
        settable(t10,1,phx);

        settable(t9,1,phx);
        settable(t11,1,phy);
        settable(t12,4,recT);


/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses (and initialize rfst) */
	rfC = 4095.0;    rfst=0.0;

  setautocal();                        /* activate auto-calibration flags */ 

  if (autocal[0] == 'n') 
  {
/* 180 degree adiabatic C13 pulse covers 140 ppm */
     if (C13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35));   
     rfst = (int) (rfst + 0.5);
     if ( 1.0/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }}

/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */
  }
  else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
  {
    if(FIRST_FID)                                            /* call Pbox */
    {
      ppm = getval("dfrq"); 
      bw = 140.0*ppm; pws = 0.001; ofs = 0.0; nst = 1000.0;
      if (C13refoc[A]=='y') 
        stC140 = pbox_makeA("rna_stC140", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
      H2Osinc = pbox_Rsh("rna_H2Osinc", "sinc90", pwHs, 0.0, compH*pw, tpwr);
      if (dm3[B] == 'y') H2ofs = 3.2;
      ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
    }
    if (C13refoc[A]=='y') rfst = stC140.pwrf; 
    pwHs = H2Osinc.pw;    tpwrs = H2Osinc.pwr;
  }

/* CHECK VALIDITY OF PARAMETER RANGES */

  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' ))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if( pw > 50.0e-6 )
  { text_error("dont fry the probe, pw too high ! "); psg_abort(1); } 
  
  if( pwN > 100.0e-6 )
  { text_error("dont fry the probe, pwN too high ! "); psg_abort(1); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */


            if (phase1 == 2)
            {tsadd(t3,1,4);  tsadd(t5,1,4);}

/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;



/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t3,2,4); tsadd(t5,2,4); tsadd(t12,2,4); }


/* BEGIN PULSE SEQUENCE */

status(A);

	obspower(tpwr);
	decpower(pwClvl);
	decpwrf(rfC);
 	dec2power(pwNlvl);
	txphase(zero);
        decphase(zero);
        dec2phase(zero);
	dec2offset(dof2-(15*dfrq2));

	delay(d1);

status(B);
 
        rcvroff();

	decrgpulse(pwC, zero, 0.0, 0.0);   /*destroy C13 magnetization*/
	zgradpulse(gzlvl0, 0.5e-3);
	delay(grecov/2);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	decpwrf(rfst);
	txphase(t1);
	delay(5.0e-4);

   	rgpulse(pw,t1,0.0,0.0);                 /* 1H pulse excitation */

	txphase(zero);
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
   	shaped_pulse("H2Osinc", pwHs, zero, 5.0e-5, 0.0);
	obspower(tpwr);

	zgradpulse(gzlvl3, gt3);
	dec2phase(t3);
	delay(grecov);

   	dec2rgpulse(pwN, t3, 0.0, 0.0);
	txphase(zero);
	decphase(zero);

	delay(timeT - 2.0*SAPS_DELAY);

        dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
        dec2phase(t5);

        delay(timeT - SAPS_DELAY);

	dec2rgpulse(pwN, t5, 0.0, 0.0);
        dec2phase(t6);

        if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY);     /* WFG2_START_DELAY */
            decshaped_pulse("rna_stC140", 1.0e-3, zero, 0.0, 0.0);
            delay(tau1 - 0.5e-3);}
        else    delay(2.0*tau1);
	
	dec2rgpulse(pwN, t6, 0.0, 0.0);
	dec2phase(t9);

	delay(timeT - SAPS_DELAY);

	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);

        delay(timeT - gt1 - 2.0*GRADIENT_DELAY - pwHs - 1.5e-4 - 2.0*POWER_DELAY - SAPS_DELAY);

        if (mag_flg[A] == 'y')  magradpulse(gzcal*gzlvl1, gt1);
        else zgradpulse(gzlvl1, gt1); 

        txphase(t4);
        obspower(tpwrs);
        shaped_pulse("H2Osinc", pwHs, t4, 1.0e-4, 0.0);
        obspower(tpwr);
        delay(5.0e-5);

	rgpulse(pw, t4, 0.0, 0.0);
	txphase(one);
	dec2phase(zero);

	zgradpulse(gzlvl5, gt5);
	delay(lambda - 0.65*(pw + pwN) - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, one, zero, zero, 0.0, 0.0);
	txphase(two);
        dec2phase(t11);

	zgradpulse(gzlvl5, gt5);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(pw, 0.0, pwN, two, zero, t11, 0.0, 0.0);
	txphase(zero);
	dec2phase(zero);

	zgradpulse(1.5*gzlvl5, gt5);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
	dec2phase(t10);

	zgradpulse(1.5*gzlvl5, gt5);
	delay(lambda - 1.6*pwN - gt5);

	dec2rgpulse(pwN, t10, 0.0, 0.0); 

	delay((gt1/10.0) + 1.0e-4 - 0.65*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

	rgpulse(2.0*pw, zero, 0.0, rof1);
	dec2power(dpwr2);				       /* POWER_DELAY */

        if (mag_flg[A] == 'y')    magradpulse(gzcal*gzlvl2, 0.1*gt1);
        else   zgradpulse(gzlvl2, 0.1*gt1);             /* 2.0*GRADIENT_DELAY */

statusdelay(C,1.0e-4-rof1);		

	setreceiver(t12);
}		 
