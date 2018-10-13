/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  cocaco_sqA.c

    2D 13C observe (CO)CACO - SQ experiment.


    Set tn='C13', dn='H1', dn2='N15, dn3='H2'.

    Standard features include maintaining the 13C carrier in the CO region
    throughout using off-res SLP pulses. The 90 and 180 CO pulses are created  
    on the fly.
    
    
    Offsets:
    
    CO          = 174ppm
    C-aliphatic = 35 ppm
    N15         = 120ppm
    H1          = 4.7ppm

 
    pulse sequence: I. Bertini, B. Jimenez, M. Piccioli
                    J.Magn. Reson. 174 (2005) 125-132.


    Must set phase = 1,2 for States-TPPI acquisition in t1 [13C].
    set dm2='nyy' dmm2='cgg'  dm='nyy' dmm='cww'.

   
    
    02 November 2005
    NM


*/



#include <standard.h>
#include "Pbox_bio.h"
  


static int  
                      
	     phi1[1]  = {1},
	     phi2[2]  = {0,2},
	     phi3[4]  = {0,0,2,2},
             rec[4]   = {0,2,2,0};

static double   d2_init=0.0, C13ofs=174.0;
static shape CO_90,CO_180,CA_90, CA_180;

pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */


 
int         t1_counter;  		 /* used for states tppi in t1 */


double      tau1,			/*  t1 delay */
	    TC = getval("TC"), 		/* delay 1/(2JC'C) ~ 9.1 ms */          
            pwClvl = getval("pwClvl"), 	/* coarse power for C13 pulse */
            pwC = getval("pwC"),        /* C13 90 degree pulse length at pwClvl */
            compC = getval("compC"),    /* adjustment for C13 amplifier compression */
	    rf0,            	  	/* maximum fine power when using pwC pulses */
            ppm,
            
/* 90 degree pulse at CO (174ppm)    */

        pwCO_90,			/* 90 degree pulse length on C13  */
        pwrCO_90,		       	/*fine power  */
        CO_bw,
        
/* 180 degree pulse at CO (174ppm)  */

        pwCO_180,		/* 180 degree pulse length at rf2 */
        pwrCO_180,		/* fine power  */

/* 90 degree pulse at C-aliph (35ppm)  */

	pwCaliph1,  		/* 90 degree pulse on C-aliphatic */
	pwrCaliph1,		/* fine power */
        CA_bw,
        CA_ofs,			/* fine power */

/* 180 degree pulse at C-aliph (35ppm)  */

	pwCaliph2, 		/* 180 degree pulse on C-aliphatic */
	pwrCaliph2,		/* fine power */

	sw1 = getval("sw1"),

	gt1 = getval("gt1"),  		     
	gzlvl1 = getval("gzlvl1"),

	gt2 = getval("gt2"),				   
	gzlvl2 = getval("gzlvl2"),
        gstab = getval("gstab");
    
        
/*   LOAD PHASE TABLE    */
	settable(t1,1,phi1);
        settable(t2,2,phi2);
	settable(t3,4,phi3);
	settable(t12,4,rec);
	
	setautocal();           /* activate auto-calibration */


/*   INITIALIZE VARIABLES   */



    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)    tsadd(t2,1,4);  
   
    tau1 = d2;
    tau1 = tau1/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t12,2,4); }
	
	
if (autocal[0] == 'y')
{
   if (FIRST_FID)
   {
    ppm = getval("sfrq");
    ofs_check(C13ofs);
    CO_bw=80*ppm; CA_bw=100*ppm;
    CA_ofs=(35-C13ofs)*ppm;
    CO_90 = pbox_make("CO_90", "Q5", CO_bw, 0.0, pwC*compC, pwClvl);
    CO_180 = pbox_make("CO_180", "square180r", CO_bw, 0.0, pwC*compC, pwClvl);
    CA_90 = pbox_make("CA_90", "Q5", CA_bw, CA_ofs, pwC*compC, pwClvl);
    CA_180 = pbox_make("CA_180", "square180r", CA_bw, CA_ofs, pwC*compC, pwClvl);
    
    /* pbox_make creates shapes with coarse power at pwClvl and fine power is adjusted */
   }
}
    pwCO_90 = CO_90.pw; pwrCO_90 = CO_90.pwrf;
    pwCO_180 = CO_180.pw; pwrCO_180 = CO_180.pwrf;
    pwCaliph1 = CA_90.pw; pwrCaliph1 = CA_90.pwrf;
    pwCaliph2 = CA_180.pw; pwrCaliph2 = CA_180.pwrf;

	
/* BEGIN PULSE SEQUENCE */

status(A);
   	delay(d1);
   	obsoffset(tof);
   	rf0 = 4095.0;
        obspower(pwClvl);
        obspwrf(rf0);
        
status(B);
        obspwrf(pwrCO_90);
	shapedpulse("CO_90",pwCO_90,zero,0.0,0.0);
	obspwrf(pwrCaliph2);
	shapedpulse("CA_180",pwCaliph2,zero,0.0,0.0);
	zgradpulse(gzlvl1,gt1);
	delay(TC/2.0-gt1);
	obspwrf(pwrCO_180);
	shapedpulse("CO_180",pwCO_180,zero,0.0,0.0);
	obspwrf(pwrCaliph2);
	shapedpulse("CA_180",pwCaliph2,zero,0.0,0.0);
	zgradpulse(gzlvl1,gt1);
	delay(TC/2.0-gt1);
	obspwrf(pwrCO_90);
	shapedpulse("CO_90",pwCO_90,t1,0.0,0.0);
	zgradpulse(gzlvl1*0.75,gt1);
	delay(gstab);
	obspwrf(pwrCaliph1);
	shapedpulse("CA_90",pwCaliph1,t2,0.0,0.0);
	
	if (tau1 < pwCO_180)
	 {   
	  obspwrf(pwrCO_180);
	  delay(tau1);
	  shapedpulse("CO_180",pwCO_180,zero,0.0,0.0);
	  delay(tau1);
	 }
	    
	else
	 {
	  obspwrf(pwrCO_180);
	  delay(tau1-pwCO_180/2.0);
	  shapedpulse("CO_180",pwCO_180,zero,0.0,0.0);
	  delay(tau1-pwCO_180/2.0);
	 }
	obspwrf(pwrCaliph1);
	shapedpulse("CA_90",pwCaliph1,t3,0.0,0.0);
	zgradpulse(gzlvl2*0.8,gt2);
	delay(gstab);
	obspwrf(pwrCO_90);
	shapedpulse("CO_90",pwCO_90,t1,0.0,0.0);
	zgradpulse(gzlvl2,gt2);
	delay(TC/2.0-gt2);
	obspwrf(pwrCaliph2);
	shapedpulse("CA_180",pwCaliph2,zero,0.0,0.0);
	obspwrf(pwrCO_180);
	shapedpulse("CO_180",pwCO_180,zero,0.0,0.0);
	zgradpulse(gzlvl2,gt2);
	delay(TC/2.0-gt2);
	
status(C);
	setreceiver(t12);
	
}

        
   

