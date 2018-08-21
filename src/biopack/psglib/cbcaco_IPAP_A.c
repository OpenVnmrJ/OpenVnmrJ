/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  cbcaco_IPAP_A.c

    3D 13C observe CBCACO experiment with IPAP option. 
    IPAP='y' for inphase splitting and IPAP='n' for anti-phase splitting.


    Set tn='C13', dn='H1', dn2='N15, dn3='H2'.

    Standard features include maintaining the 13C carrier in the CO region
    throughout using off-res SLP pulses. The 90 and 180 CO pulses are created  
    on the fly.
    
    
    Offsets:
    
    CO          = 174ppm    13C carrier position
    CAB		= 44.2 ppm    shifted for CACB pulses
    CA		= 57.7ppm   shifted for CA pulses
    N15         = 120ppm
    H1          = 4.7ppm

 
    pulse sequence: 
    W. Bermel, I. Bertini, L. Duma, I.C. Felli,L. Emsley,R. Pieratteli, and P.R. Ross
    Angew.Chem.Int.Ed. 2005, 44, 3089-3092.


    Must set phase = 1,2 phase2 = 1,2 for States-TPPI acquisition in t1 [13C] and t2.
    set dm2='nyy' dmm2='cgg'  dm='nyy' dmm='cww'.

   
    
    11 September 2006
    NM


*/



#include <standard.h>
#include "Pbox_bio.h"
  


static int  
                      
	     phi1[4]  = {0,0,2,2},
	     phi2[1]  = {1},
	     phi3[2]  = {0,2},
	     phi4[8]  = {0,0,0,0,1,1,1,1},
             phi5[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
             rec[16]   = {0,2,2,0,2,0,0,2,2,0,0,2,0,2,2,0};

static double   d2_init=0.0, d3_init=0.0, C13ofs=174.0;
static shape CO_90, CO_180, CA_90, CA_180, CAB_90, CAB_180;

pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */


 
int         t1_counter, t2_counter;	/* used for states tppi in t1 & t2*/
char	    IPAP[MAXSTR];


double      tau1,			/*  t1 delay */
            tau2,			/*  t2 delay */
	    TC = getval("TC"), 		/* delay 1/(2JCACB) ~  7.0ms in Ref. */ 
            del = getval("del"),	/* delay del = 1/(2JC'C) ~ 9.0ms in Ref. */         
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

/* 90 degree pulse at CA (57.7ppm)  */

	pwCA,  			/* 90 degree pulse on CA */
	pwrCA,		        /* fine power */
        CA_bw,
        CA_ofs,			/* Offset  */

/* 180 degree pulse at CA (57.7ppm)  */

	pwCA2, 			/* 180 degree pulse on CA */
	pwrCA2,		        /* fine power */

/* 90 degree pulse at CAB (44.2ppm)  */

        pwCAB,                   /* 90 degree pulse on CAB */
        pwrCAB,                  /* fine power */
        CAB_bw,
        CAB_ofs,                 /* Offset  */

/* 180 degree pulse at CAB (44.2ppm)  */

        pwCAB2,                  /* 180 degree pulse on CA */
        pwrCAB2,                 /* fine power */


	sw1 = getval("sw1"),
        sw2 = getval("sw2"),

	gt1 = getval("gt1"),  		     
	gzlvl1 = getval("gzlvl1"),

	gt2 = getval("gt2"),				   
	gzlvl2 = getval("gzlvl2"),
        gt3 = getval("gt3"),
        gzlvl3 = getval("gzlvl3"),
        gstab = getval("gstab");
        getstr("IPAP",IPAP);  
        
/*   LOAD PHASE TABLE    */
	settable(t1,4,phi1);
        settable(t2,1,phi2);
	settable(t3,2,phi3);
	settable(t4,8,phi4);
        settable(t5,16,phi5);
	settable(t12,16,rec);
        getelem(t5,ct,v5); assign(two,v6);
        if (IPAP[0] == 'y') { add(v5,one,v5); assign(three,v6); }

	setautocal();           /* activate auto-calibration */

/*   INITIALIZE VARIABLES   */


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)    tsadd(t1,1,4);  
    if (phase2 == 2) tsadd(t2,1,4); 

    tau1 = d2;
    tau1 = tau1/2.0;
    tau2 = d3;
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

if (autocal[0] == 'y')
{
   if (FIRST_FID)
   {
    ppm = getval("sfrq");
    ofs_check(C13ofs);
    CO_bw=80*ppm; CA_bw=100*ppm; CAB_bw=100*ppm;
    CA_ofs=(57.7-C13ofs)*ppm;
    CAB_ofs=(44.2-C13ofs)*ppm;
    CO_90 = pbox_make("CO_90", "Q5", CO_bw, 0.0, pwC*compC, pwClvl);
    CO_180 = pbox_make("CO_180", "square180r", CO_bw, 0.0, pwC*compC, pwClvl);
    CA_90 = pbox_make("CA_90", "Q5", CA_bw, CA_ofs, pwC*compC, pwClvl);
    CA_180 = pbox_make("CA_180", "q3", CA_bw, CA_ofs, pwC*compC, pwClvl);
    CAB_90 = pbox_make("CAB_90", "Q5", CAB_bw, CAB_ofs, pwC*compC, pwClvl);
    CAB_180 = pbox_make("CAB_180", "q3", CAB_bw, CAB_ofs, pwC*compC, pwClvl);
    
    /* pbox_make creates shapes with coarse power at pwClvl and fine power is adjusted */
   }
}
    pwCO_90 = CO_90.pw; pwrCO_90 = CO_90.pwrf;
    pwCO_180 = CO_180.pw; pwrCO_180 = CO_180.pwrf;
    pwCA = CA_90.pw; pwrCA = CA_90.pwrf;
    pwCA2 = CA_180.pw; pwrCA2 = CA_180.pwrf;
    pwCAB = CAB_90.pw; pwrCAB = CAB_90.pwrf;
    pwCAB2 = CAB_180.pw; pwrCAB2 = CAB_180.pwrf;


	
/* BEGIN PULSE SEQUENCE */

status(A);
   	delay(d1);
   	obsoffset(tof);
   	rf0 = 4095.0;
        obspower(pwClvl);
        obspwrf(rf0);
        
status(B);
        obspwrf(pwrCA);
	shapedpulse("CA_90",pwCA,t1,0.0,0.0);
	delay(tau1);
	obspwrf(pwrCO_180);
	shapedpulse("CO_180",pwCO_180,zero,0.0,0.0);
	obspwrf(pwrCA2);
        delay(TC/2);
	shapedpulse("CA_180",pwCA2,zero,0.0,0.0);
	delay(TC/2 - tau1);
	obspwrf(pwrCO_180);
	shapedpulse("CO_180",pwCO_180,zero,0.0,0.0);
	obspwrf(pwrCA);
	shapedpulse("CA_90",pwCA,one,0.0,0.0);
        delay(TC/2+tau2-gstab-gt1);
	zgradpulse(gzlvl1,gt1);
	delay(gstab);
	obspwrf(pwrCO_180);
	shapedpulse("CO_180",pwCO_180,zero,0.0,0.0);
        obspwrf(pwrCA2);
        shapedpulse("CA_180",pwCA2,zero,0.0,0.0);
        delay(TC/2-tau2-gt1-gstab);
	zgradpulse(gzlvl1,gt1);
        delay(gstab);
        obspwrf(pwrCO_180);
        shapedpulse("CO_180",pwCO_180,zero,0.0,0.0);
        obspwrf(pwrCAB);
        shapedpulse("CAB_90",pwCAB,t2,0.0,0.0);
        zgradpulse(gzlvl2,gt2);
        delay(gstab);
        obspwrf(pwrCO_90);
        shapedpulse("CO_90",pwCO_90,t3,0.0,0.0);
        if (IPAP[0] == 'y')
         {
          delay(del/2);
          obspwrf(pwrCAB2);     
          shapedpulse("CAB_180",pwCAB2,zero,0.0,0.0);
          obspwrf(pwrCO_180);    
          shapedpulse("CO_180",pwCO_180,t4,0.0,0.0);
          delay(del/2);
          }
         else
          {
           delay(del/4);
           obspwrf(pwrCAB2);                                                       
           shapedpulse("CAB_180",pwCAB2,zero,0.0,0.0);
           delay(del/4);
           obspwrf(pwrCO_180);
           shapedpulse("CO_180",pwCO_180,t4,0.0,0.0);
           delay(del/4);
           obspwrf(pwrCAB2);
           shapedpulse("CAB_180",pwCAB2,zero,0.0,0.0);
           delay(del/4);
           }
           obspwrf(pwrCO_90);
           shapedpulse("CO_90",pwCO_90,v5,0.0,0.0);
           zgradpulse(gzlvl3,gt3);
           delay(gstab);
           shapedpulse("CO_90",pwCO_90,v6,0.0,rof2);  
           obspwrf(pwrCO_180);
           shapedpulse("CO_180",pwCO_180,zero,0.0,rof2);	
status(C);
	setreceiver(t12);
	
}

        
   

