/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 
   gNhsqcTROSY.c

   15N gradient coherence selection TROSY

   SOME CARE WAS TAKEN FOR CORRECTING FOR INOVA-TYPE AP DELAYS. NOT TESTED.

   15N/1H multiplet component selection

       peak='LR' default setting, selects  [L]ower [R]ight, narrowest component.
                 other options: 'UR' - [U]pper [Right], 'LL', 'UL'
   
       TROSY:   Pervushin et al. PNAS, 94, 12366-12371 (1997),
                     Weigelt, JACS, 120, 10778 (1998)

 
   exp_mode='R' for a rough estimate of effective R2 relaxation
                of 15N N(a/b) magnetization during timeTN. 
                This option can be used for estimation of protein 
                rotational correlation times "TRACT"    

   TRACT:   set exp_mode='R',  array timeTN up to ~100-200ms

            record 1D/2D series with peak='LR' and peak='UR' 
       
            The difference in relaxation rates for 'LR' and 'UR' 
            is the cross-correlated relaxation rate which is, in turn,
            proportional to the rotational correlation time.
             (See TRACT reference)

            TRACT:   Lee, Hilty, Wider, Wuthrich, JMR 178, 72-76(2006)

   NOTE:  array gzlvl6 (coherence-selection decoding gradient) to find max transfer efficiency
*/

#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */
static int   
                     
             phx[]={0},   
             phy[]={1},   
             psi1[]={2},
             psi2[]={3},
             psi2c[]={1}, 
	     phi1[]  = {0,0},	  /* HSQC first part */
             phi2[]  = {0,0,1,1}, 
             
            /* phi2 = 2 phi6=1 phi7=0 rec=+ */
            /* phi2 = 1 phi6=3 phi7=2 rec=+ */
            /* phi2 = 0 phi6=1 phi7=0 rec=-  for utilizing 15N steady state*/


             phi6[]  = {3,3,3,3},  /* last pw90 on proton in the first inept, 15N steady-state */ 
	     phi7[]  = {0,0,2,2}, /*flip backs and 180 in 1st trosy, double-trosy filter */	 
             phi8[]  = {2,2,0,0},  /* phi8 is a reverse of of ph7 */     

	     phi3[]  = {0,2,0,2}, /* 90 on ca */	
             phi4[]  = {0,2,0,2,0,2,0,2}, /* first 90 pwN in TROSY t1 */
	     phi5[]  = {0,0},  
	
             rec[]   = {0,2,0,2,0,2,0,2};

void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */
 

char    Cshape[MAXSTR], 
	exp_mode[MAXSTR],   /* flag to run 3D, or 2D time-shared 15N TROSY */   
        peak[MAXSTR],       /* peak selection: LR (default) = lower right multiplet component
                               UR  = upper right, LL, UL */
        Cdec[MAXSTR],
	f1180[MAXSTR];      /* Flag to start t1 @ halfdwell */
				    
double  icosel,         /* used to get n and p type data*/
        x=0.0,
        y=0.0,
        z=0.0, 
          
        tpwrs,       
        tau1,                                /*evolution times in indirect dimensions */
        tauNH=getval("tauNH"),                              /* 1/(4Jhn), INEPTs, 2.4ms*/
        timeTN=getval("timeTN"),                         /* CT time for last SE TROSY */ 

	pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
	pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
        pwCshp = getval("pwCshp"),      
        pwCshp_lvl = getval("pwCshp_lvl"),
	
        pwNlvl = getval("pwNlvl"),	                      /* power for N15 pulses */
        pwN = getval("pwN"),                  /* N15 90 degree pulse length at pwNlvl */
        
        tpwrsf_d = getval("tpwrsf_d"), /* fine power adustment for first soft pulse(down)*/
        tpwrsf_u = getval("tpwrsf_u"), /* fine power adustment for second soft pulse(up) */
        pwHs = getval("pwHs"),	                   /* H1 90 degree pulse length at tpwrs */
        compH =getval("compH"),

        gt0 = getval("gt0"),
        gt1 = getval("gt1"),
        gt2 = getval("gt2"),
        gt3 = getval("gt3"),
        gt4 = getval("gt4"),
        gt6 = getval("gt6"),

        gzlvl0 = getval("gzlvl0"),
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),
        gzlvl3 = getval("gzlvl3"),
        gzlvl4 = getval("gzlvl4"),
        gzlvl6 = getval("gzlvl6"),
        gzlvl11 = getval("gzlvl11"),

	gstab = getval("gstab"),
	g6bal= getval("g6bal"); /* balance of the decoding gradient
				 around last 180 pulse on 1H
				 g6bal=1.0 : full g6 is on the right side of the last pw180 on 1H
				 g6bal=0.0:  full g6 is on the left side*/

      
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));          /*needs 1.69 times more*/
        tpwrs = (int) (tpwrs);                               /*power than a square pulse */

        if (tpwrsf_d<4095.0)
         tpwrs=tpwrs+6.0;  /* add 6dB to let tpwrsf_d control fine power ~2048*/
 
        getstr("f1180",f1180);
        getstr("exp_mode",exp_mode);
        getstr("peak",peak);
        getstr("Cdec",Cdec); getstr("Cshape",Cshape); 


/*   LOAD PHASE TABLE    */
	
        
        settable(t1,1,phi1);

        settable(t2,4,phi2); /* default double trosy */
        settable(t3,4,phi3);
        settable(t4,8,phi4);
        settable(t5,2,phi5);
        settable(t6,4,phi6);
        
        settable(t7,4,phi7);
        settable(t8,4,phi8);

	settable(t21,1,psi1);  /*trosy and SE hsqc in reverse INPET */

	settable(t22,1,psi2);
        settable(t23,1,psi2c);

         settable(t31,8,rec); 
	 
         /* choose multiplet component upper/lower, left-right */
         /* make sure we utilize steady-state 15N magnetization (10% on signal-to noise) */

         if(peak[B]=='L') {tsadd(t22,2,4);}
         if(peak[A]=='U') {tsadd(t21,2,4); tsadd(t6,2,4);}
          


/* some checks */

         if((dm2[A] == 'y') || (dm2[B] == 'y') || (dm2[C] == 'y') || (dm2[D] == 'y'))
           { text_error("incorrect dec2 decoupler flags! Should be 'nnnn' "); psg_abort(1); };

         if ( dm3[A] == 'y' || dm3[C] == 'y' )
       { printf("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' "); psg_abort(1); };


/*   Ntrosy , last part */
/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

      if (phase1 == 1)    {				                      icosel =  1.0;  }
            else 	  {  tsadd(t21,2,4);  tsadd(t22,2,4); tsadd(t23,2,4); icosel = -1.0;  }

    if(d2_index % 2) 	  { tsadd(t4,2,4); tsadd(t5,2,4); tsadd(t31,2,4); }
                           /* ECHO-ANTIECHO + STATES-TPPI t1, TROSY last step */

    
/* setting up t1 (ni) dimension */
    tau1  = d2; 
    
  
    if((f1180[A] == 'y') && (ni > 0.0)) 
          {tau1 +=  0.5/sw1 ;  }  


/* BEGIN PULSE SEQUENCE */

status(A);

	obspower(tpwr);
        obspwrf(4095.0);
	decpower(pwClvl);
	dec2power(pwNlvl);

	txphase(zero);
        decphase(zero);
        dec2phase(zero);

        delay(d1); 
 
        zgradpulse(gzlvl2, gt2);
	delay(gstab*3.0);

 if (exp_mode[B]=='n')  /* test for steady-state 15N */
		dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);

       /* Hz -> HzXz INEPT */

   	rgpulse(pw,zero,rof1,rof1);                 /* 1H pulse excitation */

        zgradpulse(gzlvl0, gt0);
	delay(tauNH -gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	delay(tauNH - gt0 -gstab);
	zgradpulse(gzlvl0, gt0);
	delay(gstab);

 	rgpulse(pw, t6, rof1, rof1);

       /* on HzNz now */
      /* water flipback*/
        obspower(tpwrs); obspwrf(tpwrsf_u);
 	shaped_pulse("H2Osinc",pwHs,zero,rof1,rof1);
	obspower(tpwr); obspwrf(4095.0);

       /* purge */
       
	zgradpulse(gzlvl3, gt3);
        dec2phase(t2);
	delay(gstab*2.0);

/*  HzNz , -> HzNxy  +t1 evolution*/

   	dec2rgpulse(pwN, t4, 0.0, 0.0);
        dec2phase(zero);
        delay(tau1*0.5);
        if (Cdec[A]=='y') {
                              decpower(pwCshp_lvl);
			      decshaped_pulse(Cshape,pwCshp,zero, 0.0e-6, 0.0e-6);
  		              decpower(pwClvl);
                          }
	delay(tau1*0.5);

 if (exp_mode[A]=='R') /* relaxation measurements, move coding gradient to the end of the time */
        {
         delay(timeTN*0.5 + gt4 + gstab +2.0*GRADIENT_DELAY
               +pwHs +2.0*rof1 +2.0*POWER_DELAY +WFG_STOP_DELAY +WFG_START_DELAY);
         dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
         delay(timeTN*0.5);
         zgradpulse(-1.0*gzlvl4, gt4);
         delay(gstab);
        }
       else  /* regular TROSY, shave off a bit of time from overall evolution */
        {
         zgradpulse(gzlvl4, gt4); delay(gstab);
         x=gt4 + gstab  + 2.0*GRADIENT_DELAY
           -pwHs -2.0*rof1 -2.0*POWER_DELAY -WFG_STOP_DELAY -WFG_START_DELAY;
         if(x<0.0) delay(-1.0*x);
         dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
         if (x>0.0) delay(x);
        }
        delay(4.0*pwN/3.14159265358979323846-pw -rof1);
        if (Cdec[A]=='y')  delay(pwCshp+2.0*POWER_DELAY +WFG_STOP_DELAY +WFG_START_DELAY); 
        obspower(tpwrs); obspwrf(tpwrsf_d);
 	shaped_pulse("H2Osinc",pwHs,three,rof1,rof1);
	obspower(tpwr); obspwrf(4095.0);  

/* reverse double INEPT */
        rgpulse(pw, t21, rof1, rof1);  
        zgradpulse(gzlvl11, gt1);		 
        delay(tauNH  -gt1 -rof1);
        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        delay(tauNH - gt1 -gstab);
        zgradpulse(gzlvl11, gt1);	
        delay(gstab);
        sim3pulse(pw, 0.0, pwN, one, zero, zero, 0.0, 0.0);
        zgradpulse(gzlvl1, gt1);		 
        delay(tauNH -gt1);
        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        delay(tauNH  -POWER_DELAY  -gt1- gstab);
        zgradpulse(gzlvl1, gt1);
        dec2phase(t22);		 
        delay(gstab);
        sim3pulse(0.0,0.0, pwN, one, zero, t22, 0.0, 0.0);  
	zgradpulse(-(1.0-g6bal)*gzlvl6*icosel, gt6);             /* 2.0*GRADIENT_DELAY */
        delay( gstab  -pwN*0.5 +pw*(2.0/3.14159265358979323846-0.5) );
        rgpulse(2.0*pw, zero, rof1, rof1);
	dec2power(dpwr2); decpower(dpwr);	
        zgradpulse(g6bal*gzlvl6*icosel, gt6);		         /* 2.0*GRADIENT_DELAY */
        delay(gstab +2.0*POWER_DELAY );
   status(C);
	setreceiver(t31);
}
