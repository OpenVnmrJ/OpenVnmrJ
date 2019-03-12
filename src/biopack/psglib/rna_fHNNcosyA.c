/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_fHNNcosyA.c

    This pulse sequence will allow one to perform the following experiment:

    non-TROSY correlation of imino H-N frequencies to the adjacent
    J-correlated 15N nuclei (i.e. across the hydrogen-bond)
    with H2O flipback pulses and a final watergate sequence 
    for water suppression/preservation.

    This version may be more appropriate for small RNA's and for use in probes
    where fewer gradients and pulses are desirable. It is not TROSY or sensitivity-
    enhanced.

    pulse sequence: 	Dingley and Grzesiek, JACS, 120, 8293 (1998)

	- the actual C13 and N15 offsets, dof and dof2, respectively, are 
	  used in the pulse sequence (no hidden offset changes are done).
	  The rna_fHNNcosy macro will set the C13 offset to 110ppm and the
	  N15 offset to 185ppm assuming that the values saved in the probe
	  file are 174 and 120ppm for C13 (carbonyl region of proteins) 
	  and N15 (amide region of proteins), respectively.

	- if H2O flipbacks are not calibrated, set 
		tpwrsf_i=4095.0
		phincr_i=0.0
		tpwrsf_d=4095.0
		phincr_d=0.0
		tpwrsf_u=4095.0
		phincr_u=0.0

          *_d and *_u parameters for down and up flipbacks are calibrated by
	  the latest versions of BioPack and can be retrieved from the probefile.
 	  However, the *_i flipback parameters require a different calibration;
	  if *_d parameters are available, I would set 
		phincr_i=phincr_d 
		tpwrsf_i=tpwrsf_d

	- try using different values of gzlvl3 and gzlvl4 to improve water 
	  suppression (also try using gradients of opposite sign for gzlvl3 and
	  gzlvl4)
     


        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nnn', dmm = 'ccc'
    Set dm2 = 'nnn', dmm2 = 'ccc'

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give -90, 180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.
    f1coef='1 0 0 0 0 0 -1 0'



          	  DETAILED INSTRUCTIONS FOR USE OF rna_fHNNcosy

         
    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro: 
                                      "printon man('RnaPack') printoff".
       These Detailed Instructions for rna_fHNNcosy may be printed using:
                                      "printon man('rna_fHNNcosy') printoff".
             
    2. Apply the setup macro "rna_fHNNcosy".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm) 
       set sw1 to 100ppm.

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
       gzlvl4 and gzlvl2 may be either z or magic-angle gradients. For the
       latter, a proper /vnmr/imaging/gradtable entry must be present and
       syscoil must contain the value of this entry (name of gradtable). The
       amplitude of the gzlvl4 and gzlvl2 should be lower than for a z axis
       probe to have the x and y gradient levels within the 32k range. For
       any value, a dps display (using power display) shows the x,y and z
       dac values. These must be <=32k.

       If water suppression is not efficient or exchanging iminos are lost,
       optimize gzlvl4 and gzlvl2 by arraying them together. Take the best
       compromise between S/N for iminos and water suppression. Then optimize
       gzlvl2 in small steps for optimum signal (on a 500MHz INOVA, best results
       could be achieved with gzlvl4=10000 and gzlvl2=10090).

    9. 1/4J DELAY and timeT DELAY:
       These are determined from the NH coupling constant, JNH, listed in 
       dg for possible change by the user. lambda is the 1/4J time for H1 NH 
       coupling evolution. lambda is usually set a little lower than the
       theoretical time to minimize relaxation.
       timeT is the transfer time from N15 to N15 across hydrogen-bond.
       Usually set to 15-20ms.

        rna_HNNcosyA.c Auto-calibrated version, E.Kupce, 27.08.2002.
        Fast HSQC version coded by Marco Tonelli @NMRFAM (2004)

*/


#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */  

	     
static int   
             phi3[2]  = {0,2},
             phi4[1]  = {0},
	     phi5[8]  = {1,1,1,1,3,3,3,3},
             phi6[4]  = {0,0,2,2},
             phi7[4]  = {0,0,2,2},
             phi8[4]  = {2,2,0,0},
             rec[4]   = {0,2,2,0};

static double   d2_init=0.0;
static double   H1ofs=4.7, C13ofs=110.0, N15ofs=185.0, H2ofs=0.0;

static shape H2Osinc, stC140;


void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            wtg3919[MAXSTR],		    	   /* Flag for 3919 watergate */
	    C13refoc[MAXSTR];		/* C13 sech/tanh pulse in middle of t1*/
 
int         t1_counter;  		        /* used for states tppi in t1 */

double      tau1,         				         /*  t1 delay */
	    d3919=0.0,
	    lambda = 0.91/(4.0*getval("JNH")), 	   /* 1/4J H1 evolution delay */
	    timeT = getval("timeT"),	    /* HN->N transfer time: T=15-20ms */
        
/* the sech/tanh pulse is automatically calculated by the macro "rna_cal", */
/* and is called directly from your shapelib.                  		      */
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */
			    /* temporary Pbox parameters */
   bw, pws, ofs, ppm, nst,  /* bandwidth, pulsewidth, offset, ppm, # steps */

   compH = getval("compH"),        /* adjustment for H1 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */
   	tpwrsf_i=0.0,		 /* soft power for pre pwHs ("H2Osinc") pulse */
   	tpwrsf_d=0.0, 		/* soft power for 1st pwHs ("H2Osinc") pulse */
   	tpwrsf_u=0.0, 		/* soft power for 2nd pwHs ("H2Osinc") pulse */

   	phincr_i,
   	phincr_d,
   	phincr_u,

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),

	gstab = getval("gstab"),   /* Gradient recovery delay, typically 150-200us */
	gt0 = getval("gt0"),
	gt3 = getval("gt3"),
        gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gt7 = getval("gt7"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
        gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl7 = getval("gzlvl7");

    getstr("f1180",f1180);
    getstr("C13refoc",C13refoc);
    getstr("wtg3919",wtg3919);

    if (find("d3919") > 0) d3919=getval("d3919");
    if (d3919 == 0.0) d3919=.00011;

/*   LOAD PHASE TABLE    */
	
        settable(t3,2,phi3);
        settable(t4,1,phi4);
        settable(t5,8,phi5);
        settable(t6,4,phi6);
	settable(t7,4,phi7);
	settable(t8,4,phi8);
        settable(t12,4,rec);


        if (wtg3919[0] == 'y') 
          { tsadd(t7,1,4); tsadd(t8,1,4); }
         

/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses (and initialize rfst) */
	rf0 = 4095.0;    rfst=0.0;

  setautocal();                        /* activate auto-calibration flags */

  if (autocal[0] == 'n')
  {
/* 180 degree adiabatic C13 pulse covers 140 ppm */
     if (C13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35));
     rfst = (int) (rfst + 0.5);
     if ( 1.0/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n",
            (1.0e6/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35))) );    psg_abort(1); }}

/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                        /*power than a square pulse */
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

  if( pw > 50.0e-6 )
  { text_error("dont fry the probe, pw too high ! ");               psg_abort(1); } 
  
  if( pwN > 100.0e-6 )
  { text_error("dont fry the probe, pwN too high ! ");              psg_abort(1); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

   if (phase1 == 2) 
	{ tsadd(t3,1,4); tsadd(t4,1,4); tsadd(t5,1,4);}

/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) tau1 += (1.0 / (2.0*sw1)); 
    tau1 = tau1/2.0;
    if (tau1 < 0.2e-6) tau1=0.0;


/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t3,2,4); tsadd(t4,2,4); tsadd(t5,2,4); tsadd(t12,2,4); }


/* Set fine power and small angle phase for H2O flipback pulses */

    if (find("tpwrsf_i") > 0) tpwrsf_i=getval("tpwrsf_i");
    if (tpwrsf_i == 0.0) tpwrsf_i=4095.0;

    if (find("tpwrsf_d") > 0) tpwrsf_d=getval("tpwrsf_d");
    if (tpwrsf_d == 0.0) tpwrsf_d=4095.0;

    if (find("tpwrsf_u") > 0) tpwrsf_u=getval("tpwrsf_u");
    if (tpwrsf_u == 0.0) tpwrsf_u=4095.0;

    if (find("phincr_i") > 0)
    	{phincr_i=getval("phincr_i"); if (phincr_i < 0.0) phincr_i=phincr_i +360;}
      else phincr_i=0;
    if (find("phincr_d") > 0)
    	{phincr_d=getval("phincr_d"); if (phincr_d < 0.0) phincr_d=phincr_d +360;}
      else phincr_d=0;
    if (find("phincr_u") > 0)
    	{phincr_u=getval("phincr_u"); if (phincr_u < 0.0) phincr_u=phincr_u +360;}
      else phincr_u=0;


/* BEGIN PULSE SEQUENCE */

status(A);

	obspower(tpwr);
	decpower(pwClvl);
	decpwrf(rf0);
 	dec2power(pwNlvl);
	txphase(zero);
	obsstepsize(1.0);
        decphase(zero);
        dec2phase(zero);

	delay(d1);

status(B);
 
        rcvroff();

	if (C13refoc[A]=='y') 			/* destroy C13 magnetization */
	  {
           decrgpulse(pwC, zero, 0.0, 0.0);   
	   zgradpulse(gzlvl0, 0.5e-3);
	   delay(gstab/2);
	   decrgpulse(pwC, one, 0.0, 0.0);
	   zgradpulse(0.7*gzlvl0, 0.5e-3);
	   decpwrf(rfst);
	  }
	delay(5.0e-4);

   	rgpulse(pw,zero,0.0,0.0);                 /* 1H pulse excitation */

   	txphase(one);
   	dec2phase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, one, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

 	rgpulse(pw, one, 0.0, 0.0);
	txphase(zero);

        if (phincr_i > 0) {initval(phincr_i,v10); xmtrphase(v10);}
   	if (tpwrsf_i < 4095.0) {obspower(tpwrs+6.0); obspwrf(tpwrsf_i);}
	  else obspower(tpwrs);

   	shaped_pulse("H2Osinc", pwHs, zero, 0.0, 0.0);

	xmtrphase(zero);
	obspower(tpwr); if (tpwrsf_i < 4095.0) obspwrf(4095.0);

	zgradpulse(gzlvl3, gt3);
	dec2phase(t3);
	delay(gstab);

   	dec2rgpulse(pwN, t3, 0.0, 0.0);
	dec2phase(t4);
	zgradpulse(gzlvl7, gt7);

	delay(timeT -gt7 -2.0*GRADIENT_DELAY -SAPS_DELAY);

        dec2rgpulse(2.0*pwN, t4, 0.0, 0.0);
        dec2phase(t5);
	zgradpulse(gzlvl7, gt7);

        delay(timeT -gt7 -2.0*GRADIENT_DELAY -SAPS_DELAY);

	dec2rgpulse(pwN, t5, 0.0, 0.0);
/* N15 EVOLUTION STARTS HERE */
        dec2phase(one);

        if ((C13refoc[A]=='y') && 
	    (tau1 -SAPS_DELAY -0.5e-3 -WFG2_START_DELAY -2.0*pwN/PI > 0.2e-6) )
          {
	   delay(tau1 -SAPS_DELAY -0.5e-3 -WFG2_START_DELAY -2.0*pwN/PI);  
           simshaped_pulse("", "rna_stC140", 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
           delay(tau1 -0.5e-3 -WFG2_STOP_DELAY -2.0*pwN/PI);
	  }
        else if (tau1 -SAPS_DELAY -2.0*pwN/PI -pw > 0.2e-6)
          {
	   delay(tau1 -SAPS_DELAY -2.0*pwN/PI -pw);
   	   rgpulse(2.0*pw, zero, 0.0, 0.0);
	   delay(tau1 -2.0*pwN/PI -pw);
	  }
	else
          {
	   delay(0.2e-6);
   	   rgpulse(2.0*pw, zero, 0.0, 0.0);
	   delay(0.2e-6);
	  }
/* N15 EVOLUTION ENDS HERE */
	dec2rgpulse(pwN, one, 0.0, 0.0);
	dec2phase(zero);
	decpower(dpwr); decpwrf(rf0);
	zgradpulse(gzlvl7, gt7);

	delay(timeT -gt7 -2.0*GRADIENT_DELAY -SAPS_DELAY);

	dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
	dec2phase(t6);
	zgradpulse(gzlvl7, gt7);

	delay(timeT -gt7 -2.0*GRADIENT_DELAY -SAPS_DELAY);

	dec2rgpulse(pwN, t6, 0.0, 0.0);

        zgradpulse(gzlvl4, gt4); 
	dec2phase(zero);
	txphase(two);
	if (phincr_d > 0) {initval(phincr_d,v1); xmtrphase(v1);}
   	if (tpwrsf_d < 4095.0) {obspower(tpwrs+6.0); obspwrf(tpwrsf_d);}
	  else obspower(tpwrs);
	delay(gstab);

        shaped_pulse("H2Osinc", pwHs, two, 0.0, 0.0);
	txphase(zero); if (phincr_u > 0) xmtrphase(zero);
        obspower(tpwr); if (tpwrsf_d < 4095.0) obspwrf(4095.0);

	rgpulse(pw, zero, 0.0, 0.0);


	if (wtg3919[0] == 'y')
          {
	   zgradpulse(gzlvl5, gt5);
	   txphase(t7);
	   delay(lambda -gt5 -2.0*GRADIENT_DELAY -SAPS_DELAY 
      				-pw*2.385 -7.0*rof1 -d3919*2.5);

           rgpulse(pw*0.231,t7,rof1,rof1);
           delay(d3919);
           rgpulse(pw*0.692,t7,rof1,rof1);
           delay(d3919);
           rgpulse(pw*1.462,t7,rof1,rof1);

           delay(d3919/2.0 -pwN);
           dec2rgpulse(2*pwN, zero, rof1, rof1);
           txphase(t8);
           delay(d3919/2.0 -pwN);

           rgpulse(pw*1.462,t8,rof1,rof1);
           delay(d3919);
           rgpulse(pw*0.692,t8,rof1,rof1);
           delay(d3919);
           rgpulse(pw*0.231,t8,rof1,rof1);

	   zgradpulse(gzlvl5, gt5);
	   delay(lambda -gt5 -2.0*GRADIENT_DELAY -SAPS_DELAY -POWER_DELAY
      				-pw*2.385 -7.0*rof1 -d3919*2.5);
          }
         else
	  {
	   zgradpulse(gzlvl5, gt5);
	   txphase(t7);
	   if (phincr_d > 0) {initval(phincr_d,v1); xmtrphase(v1);}
   	   if (tpwrsf_d < 4095.0) {obspower(tpwrs+6.0); obspwrf(tpwrsf_d);}
	     else obspower(tpwrs);
	   delay(lambda -pwHs -gt5 -2.0*GRADIENT_DELAY -2.0*SAPS_DELAY 
			   -WFG_START_DELAY -2.0*POWER_DELAY);

           shaped_pulse("H2Osinc", pwHs, t7, 0.0, 0.0);

	   txphase(t8); if (phincr_u > 0) xmtrphase(zero);
           obspower(tpwr); if (tpwrsf_d < 4095.0) obspwrf(4095.0);
	   sim3pulse(2.0*pw, 0.0, 2.0*pwN, t8, zero, zero, 0.0, 0.0);

	   txphase(t7);
           if (phincr_u > 0) {initval(phincr_u,v2); xmtrphase(v2);}
   	   if (tpwrsf_u < 4095.0) {obspower(tpwrs+6.0); obspwrf(tpwrsf_u);}
	     else obspower(tpwrs);
           shaped_pulse("H2Osinc", pwHs, t7, 0.0, 0.0);

	   zgradpulse(gzlvl5, gt5);
	   txphase(t12); if (phincr_u > 0) xmtrphase(zero);
	   obspower(tpwr); if (tpwrsf_u < 4095.0) obspwrf(4095.0);
	   delay(lambda -pwHs -gt5 -2.0*GRADIENT_DELAY -2.0*SAPS_DELAY 
			   -WFG_START_DELAY -3.0*POWER_DELAY);	 
          }

	dec2power(dpwr2);	/* set coarse power for 15N decoupling */
status(C);		
	setreceiver(t12);
}		 
