/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghcch_tocsy_cmhm1.c               

    3D H(C)CH TOCSY Methyl Double sensitivity enhanced. Methyl version
    original version: ghcch_tocsy_3DHCmHm_pp.c               

    Uses optional magic-angle gradients.

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
    halfdwell time. This will give -90, 180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in 13C and f2180='n' for (0,0) in 15N.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.




          	  DETAILED INSTRUCTIONS FOR USE OF ghcch_tocsy_cmhm1

    1. These Detailed Instructions for ghcch_tocsy_cmhm1 may be printed using:
                                      "printon man('ghcch_tocsy_cmhm1') printoff".
             
    2. Apply the setup macro "ghcch_tocsy_cmhm1".  This loads the relevant parameter
       set and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral
       check.

*/


#include <standard.h>



static int   phi3[2]  = {0,2},
             phi6[1]  = {1},
             phi5[4]  = {0,0,2,2},
             phi10[1] = {3},
	     rec[4]   = {0,2,2,0};

static double   d2_init=0.0, d3_init=0.0;

pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR],                            /*magic angle gradient*/
            f2180[MAXSTR];    		      /* Flag to start t2 @ halfdwell */
 
int         icosel1,          			  /* used to get n and p type */
	    icosel2,
            t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
	    kappa,						 /*  SCT t1   */
            spinlock = getval("spinlock"),
	    del = getval("del"),     /* time delays for CH coupling evolution */
	    del1 = getval("del1"),
	    del2 = getval("del2"),
	    del3 = getval("del3"),
	    del4 = getval("del4"),
	    del5 = getval("del5"),

	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	rf0,            	  /* maximum fine power when using pwC pulses */

/* p_d is used to calculate the isotropic mixing on the Cab region            */
        p_d,                  	       /* 50 degree pulse for DIPSI-2 at rfd  */
        rfd,                    /* fine power for 7 kHz rf for 500MHz magnet  */
	ncyc = getval("ncyc"), 			  /* no. of cycles of DIPSI-3 */

   compC = getval("compC"),         /* adjustment for C13 amplifier compression */


	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	pwHd,                           /* H1 90 degree pulse length at tpwrd */
        tpwrd,                          /* 4.797 kHz rf for WALTZ decoupling */
	waltzB1 = getval("waltzB1"),

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
        gzlvl6 = getval("gzlvl6"),
        gzlvl7 = getval("gzlvl7"),

	 TC = getval("TC");

    getstr("mag_flg",mag_flg);
    getstr("f1180",f1180);
    getstr("f2180",f2180);


/*   LOAD PHASE TABLE    */

	settable(t3,2,phi3);
	settable(t6,1,phi6);
	settable(t5,4,phi5);
	settable(t10,1,phi10);
	settable(t11,4,rec);

        

/*   INITIALIZE VARIABLES   */

    if( dpwrf < 4095 )
	{ printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
	  psg_abort(1); }

    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 139ppm away 
	rf10 = (compC*4095.0*pwC*2.0*1.65)/pwC10;
	rf10 = (int) (rf10 + 0.5);		*/
		
   /* dipsi-3 decoupling on CbCa */	
 	p_d = (5.0)/(9.0*4.0*spinlock*(sfrq/600.0)); /* typ. 9 kHz DIPSI-3 at 600MHz*/
 	rfd = (compC*4095.0*pwC*5.0)/(p_d*9.0);
	rfd = (int) (rfd + 0.5);
  	ncyc = (int) (ncyc + 0.5);

 /* power level and pulse time for WALTZ 1H decoupling */
        pwHd = 1/(4.0 * waltzB1) ;                      /* 4.797 kHz rf   */
        tpwrd = tpwr - 20.0*log10(pwHd/(pw));
        tpwrd = (int) (tpwrd + 0.5);



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
	{ tsadd(t10,2,4); icosel2 = -1*icosel2; tsadd(t6,2,4);}


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

 if(ni > 1)
           kappa = (double)(del*sw1/(ni)) ;
      else kappa = 0.0;   



/*   BEGIN PULSE SEQUENCE   */

status(A);
 if (dm3[B]=='y') lk_sample();
        if ((ni/sw1-d2)>0) 
         delay(ni/sw1-d2);       /*decreases as t1 increases for const.heating*/
        if ((ni2/sw2-d3)>0) 
         delay(ni2/sw2-d3);      /*decreases as t2 increases for const.heating*/
   	delay(d1);
 if (dm3[B]=='y') lk_hold();

	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	decpwrf(rf0);
        decoffset(dof - 136*dfrq);
	obsoffset(tof);
	txphase(t3);
	delay(1.0e-5);

	decrgpulse(pwC, zero, 0.0, 0.0);	   /*destroy C13 magnetization*/
	zgradpulse(gzlvl1, 0.5e-3);
	delay(1.0e-4);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl1, 0.5e-3);
	delay(5.0e-4);

   if(dm3[B] == 'y')				  /*optional 2H decoupling on */
        { 
          dec3unblank();
          dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank();
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
         } 

         status(B);

	rgpulse(pw, t3, rof1, 0.0);                    /* 1H pulse excitation */

        decphase(zero);

	if (mag_flg[A] == 'y')
                {
                   magradpulse(icosel1*gzcal*gzlvl7, gt1);
                }
                else
                {
                   zgradpulse(icosel1*gzlvl7, gt1);
                }

	delay(0.5*del - gt1 + tau1 - 2.0*pwC);

        decrgpulse(2.0*pwC, zero, 0.0, 0.0);

        txphase(zero);
	delay(tau1-(tau1*kappa));

	rgpulse(2.0*pw, zero, 0.0, 0.0);

        decphase(t5);
	delay(0.5*del - kappa*tau1);

	simpulse(pw, pwC, zero, t5, 0.0, 0.0);

	zgradpulse(gzlvl3, gt3);
        decphase(zero);
	delay(0.5*del3 - gt3);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl3, gt3);
        txphase(t6);
        decphase(one);
	delay(0.5*del3 - gt3);

	simpulse(pw, pwC, t6, one, 0.0, 0.0);

	zgradpulse(gzlvl4, gt3);
        txphase(zero);
        decphase(zero);
	delay(0.5*del5 - gt3);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl4, gt3);
	delay(0.5*del5 - gt3);

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

        decphase(zero);
        txphase(one);
	decpwrf(rf0);
	obspower(tpwrd);
        decoffset(dof - 155*dfrq);

	rgpulse(pwHd,one,0.0,2.0e-6);
        txphase(zero);
        obsunblank();
        xmtron();
        obsprgon("waltz16", pwHd, 90.0);              /* PRG_START_DELAY */

	delay(TC - OFFSET_DELAY - POWER_DELAY - PRG_START_DELAY - tau2);

	decrgpulse(2.0*pwC, zero, 0.0, 0.0);

        delay(TC + tau2 - POWER_DELAY - PRG_STOP_DELAY - 2*gt1 - 240.0e-6 - 2.0*pw);
 
        obsprgoff();
        xmtroff();
        obsblank();
        rgpulse(pwHd,three,2.0e-6,0.0);
        obspower(tpwr);

                if (mag_flg[A] == 'y')
                {
                   magradpulse(icosel2*gzcal*gzlvl2, gt1);
                }
                else
                {
                   zgradpulse(icosel2*gzlvl2, gt1);
                }
	delay(1.2e-4);

	rgpulse(2.0*pw, zero, rof1, rof1);

                if (mag_flg[A] == 'y')
                {
                   magradpulse(icosel2*gzcal*gzlvl2, gt1);
                }
                else
                {
                   zgradpulse(icosel2*gzlvl2, gt1);
                }
	delay(1.2e-4);

	decphase(zero);
        simpulse(0.0,pwC, two, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	delay(0.5*del1 - gt5);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, rof1, rof1);

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
	zgradpulse(2.3*gzlvl6, gt1);

	delay(0.5*del4 - 0.5*pwC - gt1);

	simpulse(2.0*pw,2.0*pwC, zero, zero, 0.0, 0.0);
        decpower(dpwr);
        if (mag_flg[A] == 'y')
          magradpulse(gzcal*((2.3*gzlvl6)+gzlvl1), gt1);
        else
            zgradpulse(((2.3*gzlvl6)+gzlvl1), gt1);
        delay(0.5*del4  -gt1 - 2.0*GRADIENT_DELAY - POWER_DELAY);
   if(dm3[B] == 'y')			         /*optional 2H decoupling off */
        {
          dec3rgpulse(1/dmf3, three, 0.0, 0.0);
          setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
          dec3blank();
        }
  if (dm3[B]=='y') lk_sample();
 setreceiver(t11);
status(C); 
}		 
		 
