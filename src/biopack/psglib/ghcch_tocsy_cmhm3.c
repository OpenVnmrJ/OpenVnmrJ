/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghcch_tocsy_cmhm3.c               

    4D HCCH TOCSY triple sensitivity enhanced. Methyl version
  
       original version:  ghcch_tocsy_cmhm_ver3_pp.c               


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

*/



#include <standard.h>



static int   phi2[1]  = {2},   
	     phi3[2]  = {0,2},
             phi6[1]  = {1},
             phi5[4]  = {0,0,2,2},
             phi10[1] = {3},
	     rec[4]   = {0,2,2,0};

static double   d2_init=0.0, d3_init=0.0, d4_init=0.0;

void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR],                            /*magic angle gradient*/
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
	    f3180[MAXSTR];    		      /* Flag to start t3 @ halfdwell */
 
int         icosel1,
            icosel2,  				/* used to get n and p type */
	    icosel3,
            t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    t3_counter,				/* used for states tppi in t3, not yet set */ 	
	    ni2 = getval("ni2"),
	    ni3 = getval("ni3");

double      tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
	    tau3,						 /*  t3 delay */
	    kappa,						 /*  SCT t1   */
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

/* the following pulse lengths for SLP pulses are automatically calculated    */
/* by the macro "ghcch_tocsy" .  SLP pulse shapes, "offC10" etc are called   */
/* directly from your shapelib.                    			      */
   pwC10 = getval("pwC10"),  /* 180 degree selective sinc pulse on CO(174ppm) */
   pwZ,					  /* the largest of pwC10 and 2.0*pwN */
   rf10,	                 /* fine power for the pwC10 ("offC10") pulse */

   compC = getval("compC"),         /* adjustment for C13 amplifier compression */


	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	pwHd,                           /* H1 90 degree pulse length at tpwrd */
        tpwrd,                          /* 4.797 kHz rf for WALTZ decoupling */
	waltzB1 = getval("waltzB1"),
	spinlock = getval("spinlock"),

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),
	sw3 = getval("sw3"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzcal = getval("gzcal"),               /* G/cm to DAC coversion factor*/
        gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),
	gzlvl3 = getval("gzlvl3"),
	gt3 = getval("gt3"),				   /* other gradients */
	gt5 = getval("gt5"),
        gstab = getval("gstab"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6"),
	gzlvl7 = getval("gzlvl7"),
	gzlvl8 = getval("gzlvl8"),

	 TC = getval("TC");

    getstr("mag_flg",mag_flg);
    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("f3180",f3180);


/*   LOAD PHASE TABLE    */

	settable(t2,1,phi2);
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

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 139ppm away */
	rf10 = (compC*4095.0*pwC*2.0*1.65)/pwC10; /* needs 1.65 times more     */
	rf10 = (int) (rf10 + 0.5);		/* power than a square pulse */
		
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

    icosel1 = -1;  icosel2 = -1; icosel3 = -1;
    if (phase1 == 2) 
	{ tsadd(t6,2,4); icosel1 = -1*icosel1; }
    if (phase2 == 2) 
	{ tsadd(t6,2,4); tsadd(t2,2,4); icosel2 = -1*icosel2; }
    if (phase3 == 2) 
	{ tsadd(t10,2,4); icosel3 = -1*icosel3; tsadd(t2,2,4); }


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

/*  Set up f2180  */

    tau3 = d4;
    if((f3180[A] == 'y') && (ni3 > 1.0)) 
	{ tau3 += ( 1.0 / (2.0*sw3) ); if(tau3 < 0.2e-6) tau3 = 0.0; }
    tau3 = tau3/2.0;


/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t11,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t5,2,4); tsadd(t11,2,4); }

 if( ix == 1) d4_init = d4;
   t3_counter = (int) ( (d4-d4_init)*sw3 + 0.5 );
 /*  if(t3_counter % 2) 
	{ tsadd(t5,2,4); tsadd(t11,2,4); }    TPPI t3 to be set */

 if(ni > 1)
           kappa = (double)(del*sw1/(ni)) ;
      else kappa = 0.0;   
 /* if(ni2 > 1)
           kappa2 = (double)(sw2*(del2)) / ( (double) (ni2) );
      else kappa2 = 0.0;   */



/*   BEGIN PULSE SEQUENCE   */

status(A);
 if (dm3[B]=='y') lk_sample();
        if ((ni/sw1-d2)>0) 
         delay(ni/sw1-d2);       /*decreases as t1 increases for const.heating*/
        if ((ni2/sw2-d3)>0) 
         delay(ni2/sw2-d3);      /*decreases as t2 increases for const.heating*/
     	if ((ni3/sw3-d4)>0) 
         delay(ni3/sw3-d3);      /*decreases as t3 increases for const.heating*/

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


	rgpulse(pw, t3, rof1, 0.0);                    /* 1H pulse excitation */

        decphase(zero);
	if (mag_flg[A] == 'y')
                {
                   magradpulse(-icosel1*gzcal*gzlvl7, gt1);
                }
                else
                {
                   zgradpulse(-icosel1*gzlvl7, gt1);
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

        dec2phase(zero);
        decphase(zero);
        txphase(zero);
	decpwrf(rf10);
	delay(tau2);
							 
	sim3shaped_pulse("", "offC10", "", 2.0*pw, pwC10, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        if(pwC10>2.0*pwN) pwZ=0.0; else pwZ=2.0*pwN - pwC10; 

	delay(tau2);
	decpwrf(rf0);
        if (mag_flg[A] == 'y')
                {
                   magradpulse(-icosel2*gzcal*gzlvl8, gt1);
                }
                else
                {
                   zgradpulse(-icosel2*gzlvl8, gt1);
                }


	delay(150.0e-6);

	decrgpulse(2.0*pwC, zero, 0.0, 0.0);

	if (mag_flg[A] == 'y')
                {
                   magradpulse(icosel2*gzcal*gzlvl8, gt1);
                }
                else
                {
                   zgradpulse(icosel2*gzlvl8, gt1);
                }
	delay(150.0e-6);

	decpwrf(rf10);
	decshaped_pulse("offC10", pwC10, zero, 0.0, 0.0);

        decpwrf(rf0);
        decphase(t2);

	decrgpulse(pwC, t2, 0.0, 0.0);

	decpwrf(rfd);
	delay(2.0e-6); 
	initval(ncyc, v2);
	starthardloop(v2);
     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);
     decrgpulse(5.0*p_d,one,0.0,0.0);
     decrgpulse(5.5*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.6*p_d,three,0.0,0.0);
     decrgpulse(7.2*p_d,one,0.0,0.0);
     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.4*p_d,one,0.0,0.0);
     decrgpulse(6.8*p_d,three,0.0,0.0);
     decrgpulse(7.0*p_d,one,0.0,0.0);
     decrgpulse(5.2*p_d,three,0.0,0.0);
     decrgpulse(5.4*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.5*p_d,one,0.0,0.0);
     decrgpulse(7.3*p_d,three,0.0,0.0);
     decrgpulse(5.1*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);

     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);
     decrgpulse(5.0*p_d,three,0.0,0.0);
     decrgpulse(5.5*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.6*p_d,one,0.0,0.0);
     decrgpulse(7.2*p_d,three,0.0,0.0);
     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.4*p_d,three,0.0,0.0);
     decrgpulse(6.8*p_d,one,0.0,0.0);
     decrgpulse(7.0*p_d,three,0.0,0.0);
     decrgpulse(5.2*p_d,one,0.0,0.0);
     decrgpulse(5.4*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.5*p_d,three,0.0,0.0);
     decrgpulse(7.3*p_d,one,0.0,0.0);
     decrgpulse(5.1*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);

     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);
     decrgpulse(5.0*p_d,three,0.0,0.0);
     decrgpulse(5.5*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.6*p_d,one,0.0,0.0);
     decrgpulse(7.2*p_d,three,0.0,0.0);
     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.4*p_d,three,0.0,0.0);
     decrgpulse(6.8*p_d,one,0.0,0.0);
     decrgpulse(7.0*p_d,three,0.0,0.0);
     decrgpulse(5.2*p_d,one,0.0,0.0);
     decrgpulse(5.4*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.5*p_d,three,0.0,0.0);
     decrgpulse(7.3*p_d,one,0.0,0.0);
     decrgpulse(5.1*p_d,three,0.0,0.0);
     decrgpulse(7.9*p_d,one,0.0,0.0);

     decrgpulse(4.9*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);
     decrgpulse(5.0*p_d,one,0.0,0.0);
     decrgpulse(5.5*p_d,three,0.0,0.0);
     decrgpulse(0.6*p_d,one,0.0,0.0);
     decrgpulse(4.6*p_d,three,0.0,0.0);
     decrgpulse(7.2*p_d,one,0.0,0.0);
     decrgpulse(4.9*p_d,three,0.0,0.0);
     decrgpulse(7.4*p_d,one,0.0,0.0);
     decrgpulse(6.8*p_d,three,0.0,0.0);
     decrgpulse(7.0*p_d,one,0.0,0.0);
     decrgpulse(5.2*p_d,three,0.0,0.0);
     decrgpulse(5.4*p_d,one,0.0,0.0);
     decrgpulse(0.6*p_d,three,0.0,0.0);
     decrgpulse(4.5*p_d,one,0.0,0.0);
     decrgpulse(7.3*p_d,three,0.0,0.0);
     decrgpulse(5.1*p_d,one,0.0,0.0);
     decrgpulse(7.9*p_d,three,0.0,0.0);
	endhardloop();

	txphase(one);
	decpwrf(rf0);
        decphase(zero);
        decoffset(dof - 155*dfrq);
	obspower(tpwrd);
	decrgpulse(pwC,zero,0.0,0.0);

	rgpulse(pwHd,one,0.0,2.0e-6);
        txphase(zero);
        obsunblank();
        obsprgon("waltz16", pwHd, 90.0);              /* PRG_START_DELAY */
        xmtron();

	delay(TC - OFFSET_DELAY - POWER_DELAY - PRG_START_DELAY - tau3);

	decrgpulse(2.0*pwC, zero, 0.0, 0.0);

        delay(TC + tau3 - POWER_DELAY - PRG_STOP_DELAY - 2.0*gt1 - 2.0*gstab - 2.0*pw);
 
        xmtroff();
        obsprgoff();
        obsblank();
        rgpulse(pwHd,three,2.0e-6,0.0);
        obspower(tpwr);

                if (mag_flg[A] == 'y')
                {
                   magradpulse(-icosel3*gzcal*gzlvl2, gt1);
                }
                else
                {
                   zgradpulse(-icosel3*gzlvl2, gt1);
                }
	delay(gstab);

	rgpulse(2.0*pw, zero, 0.0, 0.0);

                if (mag_flg[A] == 'y')
                {
                   magradpulse(-icosel3*gzcal*gzlvl2, gt1);
                }
                else
                {
                   zgradpulse(-icosel3*gzlvl2, gt1);
                }
	delay(gstab);

	decphase(zero);
        simpulse(0.0,pwC, two, zero, 0.0, 0.0);

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
	zgradpulse(2.3*gzlvl6, gt1);

	delay(0.5*del4 - 0.5*pwC - gt1);

	simpulse(2.0*pw,2.0*pwC, zero, zero, 0.0, 0.0);
        decpower(dpwr);
        if (mag_flg[A] == 'y')
          magradpulse(gzcal*((2.3*gzlvl6)+gzlvl1), gt1);
        else
            zgradpulse(((2.3*gzlvl6)+gzlvl1), gt1);
        delay(0.5*del4 -gt1 - 2.0*GRADIENT_DELAY - POWER_DELAY);
   if(dm3[B] == 'y')			         /*optional 2H decoupling off */
        {
          dec3rgpulse(1/dmf3, three, 0.0, 0.0);
          setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
          dec3blank();
        }
  if (dm3[B]=='y') lk_sample();
status(C); 
 setreceiver(t11);
}		 
		 
