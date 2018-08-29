/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_cchcosy_CCdec.c

    Improved H1'-C1'-C2'-H2' experiment
    C2' decoupling during the second carbon evolution period to improve resolution
    in C1' region.

 		     NOTE dof MUST BE SET AT 110ppm ALWAYS
                     NOTE dof2 MUST BE SET AT 200ppm ALWAYS

    Uses coherence gradients and Sensitivity Enhancement

    VNMR processing use f1coef='1 0 -1 0 0 -1 0 -1'

	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	@						       @
	@   Written for RnaPack by Kwaku Dayie (10/04).        @
        @   as Bax_H1C1C2_III.c                                @
	@                                                      @
	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


   (see Kwaku Dayie, J.Biomol.NMR, 32, 129-139(2005))
        	  
CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [C13].
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in C13.  f1180='y' is ignored if ni=0.
*/
#include <standard.h>
static int   
	     phi1[8]  = {3,3,3,3, 0,0,0,0},
	     phi2[4]  = {1,1,3,3},
 	     phi3[2]  = {0,1},	      		
 	     phi4[1]  = {1},	      		
	     phi5[1]  = {0},
	     phi6[1]  = {2},
	     phi9[16]  = {1,1,1,1, 3,3,3,3, 0,0,0,0, 2,2,2,2},
             rec[16]  = {0,2,2,0, 2,0,0,2, 2,0,0,2, 0,2,2,0};

static double   d2_init=0.0,
	        d3_init=0.0;
pulsesequence()
{
char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
	    f2180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            CCdseq[MAXSTR],
            CChomodec[MAXSTR],          /* Setup for C-imino - C-H6 */
	    pwC_Sel_Shape[MAXSTR],	       /* Selective C 180 refocusing pulse */
	    pwH_Sel_Shape[MAXSTR];	       /* Selective H 180 refocusing pulse */
 
int         icosel,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
	    ni2 = getval("ni2"),
            t2_counter;  		        /* used for states tppi in t2 */

double      tau1,         				          /* t1 delay */
	    tau2,         				          /* t2 delay */
            CCdpwr = getval("CCdpwr"),    /*   power level for CC decoupling */
            CCdres = getval("CCdres"),    /*   dres for CC decoupling */
            CCdmf = getval("CCdmf"),      /*   dmf for CC decoupling */

	    lambda = 1.0/(4*getval("JCH")),	   /* 1/4J H1 evolution delay */
	    taucc = 1.0/(4*getval("JCC")),	   /* 1/4J CC evolution delay */
	    delta_cc = 1.0/(4*getval("JCC")),	   /* 1/4J CC evolution delay */
	    pwH_Sel_pw = getval("pwH_Sel_pw"),	/* 180 Pulse selective pulse*/
	    pwH_Sel_lvl = getval("pwH_Sel_lvl"),	/* 180 Pulse selective pulse power level*/
	    pwC_Sel_pw = getval("pwC_Sel_pw"),	/* 180 C Pulse selective pulse*/
	    pwC_Sel_lvl = getval("pwC_Sel_lvl"),	/* 180 C Pulse selective pulse power level*/
            pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
            pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
            calC = getval("calC"),        /* multiplier on a pwC pulse for calibration */

            dofa,

            calH = getval("calH"),        /* multiplier on a pw pulse for H1 calibration */

	    pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
            pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */


	    sw1 = getval("sw1"),

            gstab = getval("gstab"),   /* Gradient recovery delay, probe-dependent */

        gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6");

    getstr("f1180",f1180);
    getstr("CCdseq",CCdseq);
    getstr("CChomodec",CChomodec);
    getstr("f2180",f2180);
    getstr("pwH_Sel_Shape",pwH_Sel_Shape);
    getstr("pwC_Sel_Shape",pwC_Sel_Shape);



/*   LOAD PHASE TABLE    */

        settable(t1,8,phi1);
        settable(t2,4,phi2);
	settable(t3,2,phi3);
	settable(t4,1,phi4);
	settable(t5,1,phi5);
	settable(t6,1,phi6);
	settable(t9,16,phi9);
	settable(t11,16,rec);

/* reset calH and calC if the user forgets */
  if (ni>1.0)
   {
	calH=1.0; calC=1.0;
   }

/* CHECK VALIDITY OF PARAMETER RANGES */

  if ((ni/sw1) > (4*taucc - 2*gt4))
  { text_error( " ni is too big. Make ni equal to %d or less.\n",    
      ((int)((4*taucc - 2*gt4)*sw1)) );					    psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' or 'nny' "); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( dpwr > 49 )
  { text_error("don't fry the probe, DPWR too large!  ");   	    psg_abort(1); }

  if( dpwr2 > 47 )
  { text_error("don't fry the probe, DPWR2 too large!  ");           psg_abort(1); }

  if( (pw > 20.0e-6) && (tpwr > 56) )
  { text_error("don't fry the probe, pw too high ! ");              psg_abort(1); }
 
  if( (pwC > 40.0e-6) && (pwClvl > 56) )
  { text_error("don't fry the probe, pwN too high ! ");             psg_abort(1); }

  if( (pwN > 100.0e-6) && (pwNlvl > 56) )
  { text_error("don't fry the probe, pwN too high ! ");             psg_abort(1); }

  if ( ni/sw1 > 4.0*taucc)
  { text_error( " ni is too big. Make ni equal to %d or less.\n",
      ((int)(4*taucc*sw1)) );                                       psg_abort(1); }

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

	icosel=1;

	if (phase1 == 2) 
	{
		tsadd(t1,1,4);
		tsadd(t4,1,4);
		tsadd(t5,1,4);
	}

	if (phase2 == 2) 
	{
		tsadd(t6,2,4);
		icosel = -1;
	}
	else icosel = 1;

/*  Set up f1180  */
       tau1 = d2;
  if((f1180[A] == 'y') && (ni > 1.0)) 
   {
	tau1 += ( 1.0 / (2.0*sw1) );
	if(tau1 < 0.2e-6) tau1 = 0.0;
   }

	tau1 = tau1/2.0;

/*  Set up f2180  */
       tau2 = d3;
  if((f2180[A] == 'y') && (ni2 > 1.0)) 
   {
	tau2 += ( 1.0 / (2.0*sw2) );
	if(tau2 < 0.2e-6) tau2 = 0.0;
   }

	tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */
  if( ix == 1) d2_init = d2;

	t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );

  if(t1_counter % 2) 
   {
	tsadd(t1,2,4);
	tsadd(t4,2,4);
	tsadd(t5,2,4);
	tsadd(t11,2,4);
   }

/* Calculate modifications to phases for States-TPPI acquisition          */
  if( ix == 1) d3_init = d3;

	t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );

/* BEGIN PULSE SEQUENCE */

status(A);
        dofa = dof - 35.0*dfrq; /*start at C2' */
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	obsoffset(tof);
	decoffset(dofa);
	dec2offset(dof2);
	txphase(zero);
	decphase(zero);
        dec2phase(zero);

	delay(d1);
        rcvroff();

status(B);

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(gstab/2);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

   	rgpulse(calH*pw,zero,0.0,0.0);                 /* 1H pulse excitation */
   	decphase(t4);
        delay(2.0*lambda);
        decrgpulse(calC*pwC, t4, 0.0, 0.0);
	zgradpulse(gzlvl3, gt3);
	obspower(pwH_Sel_lvl);
   	decphase(zero);
        delay(taucc -lambda - gt3 - WFG2_START_DELAY - pwH_Sel_pw/2.0 + 70.0e-6);

        shaped_pulse(pwH_Sel_Shape, pwH_Sel_pw, zero, 0.0, 0.0);
        delay(lambda - pwH_Sel_pw/2.5);

        decrgpulse(2.0*pwC, t1, 0.0, 0.0);

        zgradpulse(gzlvl3, gt3);
	obspower(tpwr);
   	decphase(t5);
   	txphase(t2);
        delay(taucc - gt3);

        simpulse(pw, pwC, t2, t5, 0.0, 0.0);

        zgradpulse(gzlvl4, gt4);       	/* 2.0*GRADIENT_DELAY */
        txphase(zero);
	decphase(t3);
	delay(2*taucc + tau1 - gt4);

    	decrgpulse(2.0*pwC, t3, 0.0, 0.0);
        zgradpulse(gzlvl4, gt4);       	/* 2.0*GRADIENT_DELAY */
    	decphase(zero);
		
        delay(2.0*taucc - tau1 - gt4);

        dofa = dof - 17.5*dfrq; /*change to C1' */
	decoffset(dofa);
/*  ffffffffffffffffffff   BEGIN SENSITIVITY ENHANCE   fffffffffffffffffffff  */
        decrgpulse(pwC, zero, 0.0, 0.0);

if (CChomodec[A]=='y')
 {
	delay(delta_cc);
    	decrgpulse(2.0*pwC, zero, 0.0, 0.0);
	delay(delta_cc);
        decpower(CCdpwr); decphase(zero);
        decprgon(CCdseq,1.0/CCdmf,CCdres);
        decon();  /* CC decoupling on */
	delay(tau2);
    	dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
	delay(tau2);

        decoff(); decprgoff();        /* CC decoupling off */
	decpower(pwC_Sel_lvl);

        zgradpulse(icosel*gzlvl1, gt1/2.0);       	/* 2.0*GRADIENT_DELAY */
        delay(gstab);
        decshaped_pulse(pwC_Sel_Shape, pwC_Sel_pw, t9, 0.0, 0.0);
        zgradpulse(-1.0*icosel*gzlvl1, gt1/2.0);       	/* 2.0*GRADIENT_DELAY */
	decphase(t6);
        decpower(pwClvl);
        delay(gstab);
		
 }
else
 {
	decphase(zero);
        zgradpulse(icosel*gzlvl1, gt1/2.0);       	/* 2.0*GRADIENT_DELAY */
	delay(delta_cc + tau2);
    	decrgpulse(2.0*pwC, t9, 0.0, 0.0);
        zgradpulse(-1.0*icosel*gzlvl1, gt1/2.0);       	/* 2.0*GRADIENT_DELAY */
	decphase(t6);
	delay(delta_cc - tau2);
 }		
	simpulse(pw, pwC, zero, t6, 0.0, 0.0);
	decpower(pwC_Sel_lvl);
	decphase(two);
	zgradpulse(gzlvl5, gt5);
	delay(lambda - pwC_Sel_pw/2.0 - gt5);
        simshaped_pulse("",pwC_Sel_Shape, 2.0*pw,pwC_Sel_pw, zero, two, 0.0, 0.0);
	decpower(pwClvl);
	zgradpulse(gzlvl5, gt5);
	txphase(one);
	decphase(one);
	delay(lambda - pwC_Sel_pw/2.0 - gt5);
	simpulse(pw, pwC, one, one, 0.0, 0.0);
	decpower(pwC_Sel_lvl);
	txphase(zero);
	decphase(zero);
	zgradpulse(gzlvl6, gt5);
	delay(lambda - pwC_Sel_pw/2.0 - gt5);
        simshaped_pulse("",pwC_Sel_Shape, 2.0*pw,pwC_Sel_pw, zero, zero, 0.0, 0.0);
	zgradpulse(gzlvl6, gt5);
	delay(lambda - pwC_Sel_pw/2.0 - gt5);
	rgpulse(pw, zero, 0.0, 0.0);
        zgradpulse(gzlvl2, gt1/8.0);         		/* 2.0*GRADIENT_DELAY */
	delay(gstab/2-2.0*GRADIENT_DELAY);		
	rgpulse(2.0*pw, zero, 0.0, 0.0);
	decpower(dpwr);					       /* POWER_DELAY */
	dec2power(dpwr2);				      /* POWER_DELAY */
        zgradpulse(-1.0*gzlvl2, gt1/8.0);         		/* 2.0*GRADIENT_DELAY */
	delay(gstab/2 -2.0*POWER_DELAY-2.0*GRADIENT_DELAY);		
status(C);
		 
	setreceiver(t11);
}		 
