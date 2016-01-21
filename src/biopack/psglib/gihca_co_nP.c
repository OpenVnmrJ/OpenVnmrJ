/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gihca_co_nP.c               

    3D Intraresidual hca_co_nP gradient sensitivity enhanced version.

    3D ihca_co_nP correlates Ha(i), CO(i) and N(i).
    Ref: Mäntylahti et al. J. Biomol. NMR, 47, 171-181 (2010).

    Uses three channels:
         1)  1H   (t3)       - carrier  4.7 ppm (tof)
         2) 13CO  (t2, ni2)  - carrier  174 ppm (dof)
            13Ca             - carrier  56 ppm
         3) 15N   (t1, ni)   - carrier  120 ppm (dof2)
         4)  1H              - carrier  4.5 ppm (dof3)

*/


#include <standard.h>
#include "bionmr.h"

static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
             phi1[2]  = {0,2},
             phi2[4]  = {0,0,2,2},
             phi3[8]  = {1,1,1,1,3,3,3,3},
	     rec[8]   = {0,2,2,0,2,0,0,2};



pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            stCshape[MAXSTR],
            mag_flg[MAXSTR];			    
 
int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
	    ni = getval("ni"),
	    ni2 = getval("ni2");

double      d2_init=0.0,  		        /* used for states tppi in t1 */
	    d3_init=0.0,  	 	        /* used for states tppi in t2 */
	    tau1,         				         /*  t1 delay */
	    tauCH = getval("tauCH"), 		         /* 1/4J delay for CH */
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
            timeNCA = getval("timeNCA"),
            timeC = getval("timeC"),      /* other delays */
	    zeta = getval("zeta"),
            tauCC = getval("tauCC"),

	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
        compC = getval("compC"),
        rf0,
        rfst,
        corr,

        waltzB1 = getval("waltzB1"),  /* waltz16 field strength (in Hz)     */
        compH = getval("compH"),
        pwHd,                           /* H1 90 degree pulse length at tpwrd */
        tpwrd,                                     /* rf for WALTZ decoupling */

   pwS1,					/* length of square 90 on Cab */
   pwS2,					/* length of square 180 on Ca */
   phi7cal = getval("phi7cal"),  /* phase in degrees of the last C13 90 pulse */
   phshift = getval("phshift"),        /*  phase shift induced on CO by 180 on CA in middle of t1 */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gzcal = getval("gzcal"),
	gstab = getval("gstab"),
    
	gt0 = getval("gt0"),  
        gt1 = getval("gt1"),  
        gt2 = getval("gt2"), 
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
    getstr("f2180",f2180);
    getstr("mag_flg",mag_flg);


/*   LOAD PHASE TABLE    */

	settable(t1,2,phi1);
	settable(t2,4,phi2);
        settable(t3,8,phi3);
	settable(t12,8,rec);
        

/*   INITIALIZE VARIABLES   */

 	kappa = 5.4e-3;
	lambda = 2.4e-3;

/* maximum fine power for pwC pulses (and initialize rfst) */
        rf0 = 4095.0;    rfst=0.0;

    if( pwC > 20.0*600.0/sfrq )
	{ printf("increase pwClvl so that pwC < 20*600/sfrq");
	  psg_abort(1); }



/* 30 ppm sech/tanh inversion for Ca-Carbons */

        rfst = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));
        rfst = (int) (rfst + 0.5);
        strcpy(stCshape, "stC30");

    /* get calculated pulse lengths of shaped C13 pulses */
	pwS1 = c13pulsepw("co", "ca", "sinc", 90.0); 
	pwS2 = c13pulsepw("ca", "co", "square", 180.0); 

/* power level and pulse time for WALTZ 1H decoupling */
        pwHd = 1/(4.0 * waltzB1) ;                  
        tpwrd = tpwr - 20.0*log10(pwHd/(compH*pw));
        tpwrd = (int) (tpwrd + 0.5);
	

/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( gt4 > zeta - 0.6*pwC)
       { printf(" gt4 is too big. Make gt4 equal to %f or less.\n", 
  	 (zeta - 0.6*pwC)); 	     				     psg_abort(1);}

    if ( 0.5*ni2*1/(sw2) > 2.0*timeC + tauCC - OFFSET_DELAY)
       { printf(" ni2 is too big. Make ni equal to %d or less.\n", 
  	 ((int)((2.0*timeC + tauCC - OFFSET_DELAY)*2.0*sw2))); 	     psg_abort(1);}

    if ( dm[A] == 'y' || dm[B] == 'y')
       { printf("incorrect dec1 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' )
       { printf("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dm3[A] == 'y' || dm3[C] == 'y' )
       { printf("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' ");
							             psg_abort(1);}
    if ( dpwr2 > 46 )
       { printf("dpwr2 too large! recheck value  ");		     psg_abort(1);}

    if ( pw > 20.0e-6 )
       { printf(" pw too long ! recheck value ");	             psg_abort(1);} 
  
    if ( pwN > 100.0e-6 )
       { printf(" pwN too long! recheck value ");	             psg_abort(1);} 
 

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2) tsadd(t1,1,4);  
    if (phase2 == 2) tsadd(t2,1,4);  


/*  C13 TIME INCREMENTATION and set up f1180  */

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
	{ tsadd(t1,2,4); tsadd(t12,2,4); }

   if( ix == 1) d3_init = d3; 
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t12,2,4); }



/*   BEGIN PULSE SEQUENCE   */

status(A);
   	delay(d1);
        if (dm3[B]=='y') lk_hold();

	rcvroff();
        set_c13offset("ca");
	obsoffset(tof);
	obspower(tpwr);
 	obspwrf(4095.0);
	decpower(pwClvl);
	decpwrf(4095.0);
 	dec2power(pwNlvl);
	txphase(three);
	delay(1.0e-5);

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

        if(dm3[B] == 'y')			  /*optional 2H decoupling on */
         {dec3unblank(); dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank(); setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);} 

	rgpulse(pw, zero, 0.0, 0.0);                  /* 1H pulse excitation */
                          
        txphase(zero);
        decphase(zero);
	zgradpulse(gzlvl0, gt0);			/* 2.0*GRADIENT_DELAY */

/*        delay(tauCH - gt0);
        simpulse(2.0*pw, 2.0*pwC, zero,zero,0.0,0.0);
        delay(tauCH - gt0 - 150.0e-6); */

	decpwrf(rfst);
        delay(tauCH - gt0 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

        simshaped_pulse("",stCshape, 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);

        delay(tauCH - gt0 - 0.5e-3 + 70.0e-6 - 150.0e-6);

        decpwrf(rf0);

	zgradpulse(gzlvl0, gt0);   	 	        /* 2.0*GRADIENT_DELAY */
	delay(150.0e-6);
           
	rgpulse(pw, one, 0.0, 0.0);	
	zgradpulse(gzlvl3, gt3);
	delay(2.0e-4);
        obspower(tpwrd);
	
        decrgpulse(pwC, t3, 0.0, 0.0);

        set_c13offset("co");

	delay(zeta - 0.6*pwC - OFFSET_DELAY - pwHd - PRG_START_DELAY);

        rgpulse(pwHd,one,0.0,0.0);
        txphase(zero);
        delay(2.0e-6);
        obsprgon("waltz16", pwHd, 90.0); /* PRG_START_DELAY */
        xmtron();
 
	delay(2.0*timeC - zeta);
 
        c13pulse("co", "ca", "sinc", 90.0, t2, 0.0, 0.0); /* pwS1 */		
	
        delay(timeNCA - 2.0*timeC - 0.5*gt3 - 200.0e-6 - tau2);

	zgradpulse(0.2*gzlvl3, 0.5*gt3);
	delay(2.0e-4);

        c13pulse("ca", "co", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);
        sim3_c13pulse("", "co", "ca", "square", "", 0.0, 180.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0); /* pwS2 */

	zgradpulse(0.2*gzlvl3, 0.5*gt3);
	delay(2.0e-4);

        delay(timeNCA - 2.0*timeC - 0.5*gt3 - 200.0e-6 + tau2 + (60.0e-6));
        
        initval(phshift, v3);
        decstepsize(1.0);
        dcplrphase(v3);        
        c13pulse("co", "ca", "sinc", 90.0, one, 0.0, 0.0); /* pwS1 */

        delay(2.0*timeC + tauCC - OFFSET_DELAY - SAPS_DELAY - tau2);
        c13pulse("ca", "co", "sinc", 180.0, zero, 0.0, 0.0);
        delay(tauCC);
        sim3_c13pulse("", "co", "ca", "square", "", 0.0, 180.0, 2.0*pwN, zero, zero, zero, 0.0, 60.0e-6);
        delay(tau2);

        set_c13offset("ca");

        initval(phi7cal, v7);
        decstepsize(1.0);
        dcplrphase(v7);                                   /* SAPS_DELAY */
        dec2phase(t1);

        c13pulse("ca", "co", "square", 90.0, zero, 0.0, 0.0); /* pwS1 */

        xmtroff();
        obsprgoff();
        rgpulse(pwHd,three,2.0e-6,0.0);

        zgradpulse(3*gzlvl4, 0.5*gt3);
	delay(2.0e-4);

        rgpulse(pwHd,one,0.0,0.0);
        txphase(zero);
        delay(2.0e-6);
        obsprgon("waltz16", pwHd, 90.0); /* PRG_START_DELAY */
        xmtron();        

        dec2rgpulse(pwN, t1, 0.0, 0.0);
	
      corr = 0.5*(4.0*pwN/PI +pwS1 +pwS2 +WFG2_START_DELAY +2.0*SAPS_DELAY +PWRF_DELAY);

      if (tau1 > corr) delay(tau1 -corr);

      c13pulse("ca", "co", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);
      c13pulse("co", "ca", "square", 180.0, zero, 2.0e-6, 2.0e-6);

      if (tau1 > corr) delay(tau1 -corr);
	
        dec2rgpulse(pwN, zero, 0.0, 0.0);

        xmtroff();
        obsprgoff();
        rgpulse(pwHd,three,2.0e-6,0.0);

 	zgradpulse(5*gzlvl4, 0.5*gt3);
	delay(2.0e-4);

        rgpulse(pwHd,one,0.0,0.0);
        txphase(zero);
        delay(2.0e-6);
        obsprgon("waltz16", pwHd, 90.0); /* PRG_START_DELAY */
        xmtron();        

        c13pulse("ca", "co", "square", 90.0, zero, 0.0, 0.0); /* pwS1 */		
	
        delay(timeTN);

        sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0); /* pwS2 */

        delay(timeTN - zeta -PRG_STOP_DELAY -2.0e-6);

        xmtroff();
        obsprgoff();
        rgpulse(pwHd,three,2.0e-6,0.0);

        delay(zeta -gt1 -2.0*GRADIENT_DELAY -gstab - pwHd);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);      /* 2.0*GRADIENT_DELAY */
          obspower(tpwr); /* POWER_DELAY */          
          delay(gstab - POWER_DELAY); /* WFG_START_DELAY */
          decpwrf(rf0);
          decphase(zero);
              
        simpulse(pw, pwC, zero, zero, 0.0,0.0);
        zgradpulse(0.8*gzlvl5, gt5);
    decphase(zero);

    delay(tauCH -pwC -gt5 -2.0*GRADIENT_DELAY);

    simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

    zgradpulse(0.8*gzlvl5, gt5);
    txphase(one);
    decphase(one);
    delay(tauCH - pwC - gt5 -2.0*GRADIENT_DELAY);

    simpulse(pw, pwC, one, one, 0.0, 0.0);

    zgradpulse(gzlvl5, gt5);
    txphase(zero);
    decphase(zero);
    delay(tauCH -gt5 -2.0*GRADIENT_DELAY);

    simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

    zgradpulse(gzlvl5, gt5);
    decpower(dpwr);

    delay(tauCH -gt5 -2.0*GRADIENT_DELAY -POWER_DELAY);

    rgpulse(pw, zero, 0.0, 0.0);

    delay(1.0e-4 +gstab + gt1/4.0 + 2.0*GRADIENT_DELAY);
    rgpulse(2.0*pw, zero, 0.0, 0.0);
    if(mag_flg[A] == 'y') magradpulse(gzcal*gzlvl2, gt1/4.0);
      else zgradpulse(gzlvl2, gt1/4.0);

    delay(gstab);
    rcvron();
    statusdelay(C, 1.0e-4);
    if (dm3[B]=='y') lk_sample();

    setreceiver(t12);

}		 

