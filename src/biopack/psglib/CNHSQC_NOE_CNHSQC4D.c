/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* CNHSQC-NOE-CNHSQC4D.c   Agilent Technologies, July 2012 
     Time - shared (TS), up to 4D:

15N/13C-HSQC(t3,ni3) --> H-H  NOE (t2,ni2) --> hnTROSY/CHseHSQC (t1,ni; acq)

	in 3D/4D form this provides following NOE correlation:
        HN  <-> HN
        H3C <-> H3C
	HN   -> H3C
        H3C  -> HN
        
        Original pulse sequence reference: Frueh, Vosburg, Walsh, Wagner, 
        J. BioNMR 2006 v34 pp31-40 


 Sample is assumed to be all deuterated+12C, except 13CH3-IVLMAT,15NH.
 No care taken for N-CA, N-CO, C-C [de]coupling, no ZQ filters during NOE

    
    2012 CORRECTIONS FOR INOVA-TYPE APDELAYS ARE IN, NOT TESTED 
    2012 DECOUPLING OF 15N FROM CA/CO IS NOT FULLY IMPLEMENTED

    
    swN - spectral width in 15N
    swC - spectral width in 13C
    sw2 - spectral width in 1H (NOE) dimension. 
    Folding recommended, sw2=6ppm seems to good.

    pwC, pwN, pw, pwClvl, pwNlvl, tpwr etc - standard BioPack Parameters

    Coherence selection gradients:

	15N encoded by (gzlvl4+gzlvl5)*gt4, decoded by gzlvl6*gt6
	13C encoded by (gzlvl4-gzlvl5)*gt4, decoded by gzlvl6*gt6
     
	coherence selection optimization:

     1. array gzlvl4 +/- 10% of the default value, maximize HN signals

     2. array gzlvl4, gzlvl5  around found vaules, keeping gzlvl5+gzlvl4
        constant,NH signals should stay unchanged, maximize CH3 signals

   Decoupling from CO/CA [NOT FULLY IMPLEMENTED AS OF MAY 2012] decCACO='y'


   Experiment type: exp_mode setting

   '2D' - Time-Shared (TS) 15N TROSY/ 13C seHSQC
   '3D' - NOESY(t2)-(TS 15N TROSY/ 13C seHSQC, t1)
   '4D'- (TS 15N/13C HSQC, t3)-NOESY(t2)-(TS 15N TROSY/ 13C seHSQC, t1)
   '4Da'-(TS 15N/13C HSQC, t3)-NOESY(t2)-(TS 15N TROSY/ 13C seHSQC, t1), 
	  phases phi5/phi7 are inverted so that (HN<->H3C) cross-peaks
          are of a opposite sign of H3C<->H3C and HN<->HN  peaks

   fourth character in the exp_mode determines whether NOE(t2) part is
   flanked by selective water pulses as in paper (default). A 45-degree
   shift is used in NOE(t2) for exp_mode='***t'

   to run a "full" 4D  version:
	set ni,ni2,ni3 accordingly, 
	phase=1,2 phase2=1,2 phase3=1,2 exp_mode='4D' or '4Da'
	array='phase,phase2,phase3,exp_mode'
			  												
  exp_mode='4x' - special case for test 3D version 
	(TS 15N/13C HSQC, t2)-NOESY-(TS 15N TROSY/ 13C seHSQC, t1)
	 set ni3=1, phase3=1 !!! 

   Phasing in direct dimension.

   In general, one will need to process CH and HN part of the
   spectra separately. There will be different phasing parameters fo
   CH3 and HN in direct dimension. However, phasing differences between
   HN and CH3 can be minimized by adjusting parameter ch90corr around
   default value.
	
   ch90corr defines how last 1H-methyl selective 90 degree pulse
   is aligned around last 90 pulse on 15N. ch90corr depends upon the
   shape used for that selective pulse and has value range between
   0.0 and 1.0, ch90corr=0.5 means that the ch90 selective
   pulse is centered around last 90 pulse on 15N.

   How to adjust ch90corr:
   1. acquire high-S/N 1D with default value of ch90corr. Phase the HN region.
   2. array ch90corr around default value with steps of 0.01,
      choose one that gives spectra properly phased on both HN and CH3 parts.

   Phasing in indirect dimensions.

   The pulse sequence flows as follows:

   15N/13C-H[S/M]QC(t3) --> H-H  NOE (t2) --> hnTROSY/CHseHSQC (t1, acq)

   (NOT IMPLEMENTED): set f1180='y' to start at half-dwell time in t1 direction
         (Time-shared 15N-TROSY /13C-seHSQC)

   set f2180='y' to start at half-dwell time in t2 direction (H-H NOE)
	 
   t3 is hard-coded to start at half-dwell time, in this dimension use
   90,-180 phase corrections and first-point multiplier = 1.0
*/



#include <standard.h>
#define MAX_PC 8
  

	     
static int   
                     
             phx[]={0},                            /*phases, like in paper */
             phy[]={1},   
             psi1[]={0},
             psi2[]={1},
             psi2c[]={1},         /* psi2 for first 13C 90 in reverse inept */
 
	     phi1[]  = {0,0},	
             phi2[]  = {0,0,2,2},           /* first  proton 90 in NOE part */
	     phi3[]  = {0,0},	
             phi4[]  = {0,2},                  /* first 90 pwN in TROSY t1' */
	     phi5[]  = {0,2},                  /* first 90 pwC in SEHSQC t1 */	
             phi6[]  = {0,0},   /* refocusing 180 for CH3 groups in HSQC t1 */

	     phi7[]  = {0,0,0,0,2,2,2,2},	/* first 90 pwN in HSQC t3' */
             phi8[]  = {0,0,0,0,2,2,2,2},        /* first 90 pwC in HSQC t3 */

	
             rec[]   = {0,2,2,0,2,0,0,2};


static double   d2_init=0.0, relaxTmax;
pulsesequence()
{
/* DECLARE AND LOAD VARIABLES */
char
	ch90shape[MAXSTR],
	ch180shape[MAXSTR],
        exp_mode[MAXSTR],   /* flag to run 3D, or 2D time-shared 15N TROSY /13C HSQC-SE*/    
        decCACO[MAXSTR],    
        caco180shape[MAXSTR],
	f1180[MAXSTR],   		              /* Flag to start t1 @ halfdwell */
	f2180[MAXSTR],
	f3180[MAXSTR],
	f4180[MAXSTR];			                    /* do TROSY on N15 and H1 */
 
int         icosel, max_pcyc;      			  /* used to get n and p type */
     
double  
        tpwrs,
        ni2=getval("ni2"),
        ni3=getval("ni3"),
        tau1, tau1p,tau2,tau3,tau3p,         /*evolution times in indirect dimensions */
        tauNH=getval("tauNH"),                                             /* 1/(4Jhn)*/
        tauCH=getval("tauCH"),                                            /* 1/(4Jch) */
        tauCH1= getval("tauCH1"),     /* tauCH/2.0+tauNH/2.0,*/ /* 1/(8Jch) +1/(8Jnh) */
        tauCH2= getval("tauCH2"),
        swC = getval("swC"),                        /* spectral widths in 13C methyls */
	pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
	pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
        swN = getval("swN"),                                /* spectral widths in 15N */  	              
	pwNlvl = getval("pwNlvl"),	                      /* power for N15 pulses */
        pwN = getval("pwN"),                  /* N15 90 degree pulse length at pwNlvl */   
        ch90pwr=getval("ch90pwr"),
        ch90pw=getval("ch90pw"),
	ch90corr=getval("ch90corr"),
        ch90dres=getval("ch90dres"),
        ch90dmf=getval("ch90dmf"),
 	ch180pw=getval("ch180pw"),
	ch180pwr=getval("ch180pwr"),
        caco180pw=getval("caco180pw"),
        caco180pwr=getval("caco180pwr"),
        mix=getval("mix"),
        tpwrsf_d = getval("tpwrsf_d"), /* fine power adustment for first soft pulse(down)*/
        tpwrsf_u = getval("tpwrsf_u"), /* fine power adustment for second soft pulse(up) */
        pwHs = getval("pwHs"),                     /* H1 90 degree pulse length at tpwrs */
        compH =getval("compH"),

	gstab = getval("gstab"),
	
  	gt0 = getval("gt0"),     
        gt1 = getval("gt1"),
        gt2 = getval("gt2"),
 	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
        gt6 = getval("gt6"),
 	gt7 = getval("gt7"),
	gt8 = getval("gt8"),
	gt9 = getval("gt9"),
	gt10 = getval("gt10"),

	gzlvl0 = getval("gzlvl0"),
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),	
        gzlvl6 = getval("gzlvl6"),
	gzlvl7 = getval("gzlvl7"),
	gzlvl8 = getval("gzlvl8"),
	gzlvl9 = getval("gzlvl9"),
	gzlvl10 = getval("gzlvl10"),
        gzlvl11 = getval("gzlvl11");

        getstr("f1180",f1180);
        getstr("f2180",f2180);
        getstr("ch180shape",ch180shape);
        getstr("ch90shape",ch90shape);
        getstr("decCACO",decCACO);
        getstr("caco180shape",caco180shape);
        getstr("exp_mode",exp_mode);

        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));          /*needs 1.69 times more*/
        tpwrs = (int) (tpwrs);                               /*power than a square pulse */

        if (tpwrsf_d<4095.0)
         tpwrs=tpwrs+6.0;  /* add 6dB to let tpwrsf_d control fine power ~2048*/

      if( (exp_mode[A]!='2') && (exp_mode[A]!='3') && (exp_mode[A]!='4') )
          {text_error("invalid exp_mode, Should be either 2D or 3D or 4D\n "); psg_abort(1); }

/*   LOAD PHASE TABLE    */
	
        
        settable(t1,1,phi1);
        settable(t2,4,phi2);
	settable(t12,4,phi2); {tsadd(t12,2,4);}

        settable(t3,1,phi3);
        settable(t4,2,phi4);
        settable(t5,2,phi5);
        settable(t6,4,phi6);
        
        settable(t7,8,phi7);
        settable(t8,8,phi8);
 
        /* changing sign */

         if( (exp_mode[A]=='4') && (exp_mode[C]=='a') )
              {tsadd(t7,2,4); tsadd(t5,2,4); }
      

	settable(t21,1,psi1);                          /*trosy and SE hsqc in reverse INPET */
	settable(t22,1,psi2);
        settable(t23,1,psi2c);

        if(exp_mode[A]=='2') {settable(t31,2,rec);}
        if(exp_mode[A]=='3') {settable(t31,4,rec);}
	if(exp_mode[A]=='4') {settable(t31,8,rec);}

        if((dm2[A] == 'y') || (dm2[B] == 'y') || (dm2[C] == 'y') || (dm2[D] == 'y'))
        { text_error("incorrect dec2 decoupler flags! Should be 'nnnn' "); psg_abort(1); }


/* special case for  swapping t2 and t3 for test purposes */

if( (exp_mode[A]=='4') && (exp_mode[B]=='x') && (ni3=1) )
	{	
	 text_error("Acquiring t3 axis in ni2 dimension (instead of t2), set nt to 8! "); 
         tau3  = 0.5*(d3_index/swC+0.5/swC)-pw-rof1 -pwC*2.0/M_PI ;  /* increment corresponds to 13C increment */
	 tau3p = 0.5*(d3_index/swN+0.5/swN) -pw-rof1  -pwC -pwN*2.0/M_PI -tau3; 
         if(d3_index % 2)    { tsadd(t7,2,4); tsadd(t8,2,4); tsadd(t31,2,4); }
    	 if (phase2 == 2)  {tsadd(t7 ,1,4); tsadd(t8 ,1,4);}
         tau2=0.0;
        }
else    {    
         if (phase2 == 2)  {tsadd(t2 ,1,4); tsadd(t12,1,4);}
         if (phase3 == 2)  {tsadd(t7 ,1,4); tsadd(t8 ,1,4);}
         if(d3_index % 2)    { tsadd(t2,2,4);  tsadd(t12,2,4); tsadd(t31,2,4); }    
         tau3  = 0.5*(d4_index/swC+0.5/swC)-pw -rof1 -pwC*2.0/M_PI ;  /* increment corresponds to 13C increment */
         tau3p = 0.5*(d4_index/swN+0.5/swN) -pw -rof1 -pwC -pwN*2.0/M_PI -tau3; 
         if(d4_index % 2)    { tsadd(t7,2,4); tsadd(t8,2,4); tsadd(t31,2,4); }
         tau2 = d3;
         tau2 += 0.0*(-pw*4.0/M_PI-rof1*2.0);
         if((f2180[A] == 'y') && (ni2 > 0.0)) {tau2 += ( 1.0 / (2.0*sw2) );  }
         if(tau2 < 0.2e-6) {tau2 = 0.0;}
         tau2 = tau2/2.0;
        }

 
/* simultaneous Ntrosy-ChsqcSE, last part */
/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

       if (phase1 == 1)    {icosel =  1;  }
            else 	  {  tsadd(t21,2,4);  tsadd(t22,2,4); tsadd(t23,2,4); icosel = -1;  }

       if(d2_index % 2)   { tsadd(t4,2,4); tsadd(t5,2,4); tsadd(t31,2,4); }  
          /* ECHO-ANTIECHO + STATES-TPPI t1, t1' in TROSY/HSQC last step */

       tau1  = 1.0*d2_index/swC;  /* increment corresponds to 13C increment */
       tau1p = 1.0*d2_index*(1.0/swN-1.0/swC); 
    
/* BEGIN PULSE SEQUENCE */

status(A);

	obspower(tpwr);
	decpower(pwClvl);
	dec2power(pwNlvl);

	txphase(zero);
        decphase(zero);
        dec2phase(zero);

        delay(d1); 

  
/* Destroy 13C magnetization*/

       decrgpulse(pwC*1.0, zero, 0.0, 0.0); 
       zgradpulse(-gzlvl0, gt0);
       delay(gstab);

       /* NOESY */

if(exp_mode[A]!='2')
{  /* 3-4D */

     if(exp_mode[A]=='4')
	{ /* full 4D */
          /* t3 evolution, the very first HSQC */

 	      /* Hz -> HzXz INEPT */

	   	rgpulse(pw,two,rof1,rof1);                         /* 1H pulse excitation */
 	        zgradpulse(gzlvl7, gt0);      delay(tauCH-gt0);
	        decrgpulse(pwC*2.0, zero, 0.0, 0.0); delay(tauNH -tauCH -pwC*2.0 );
	   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
 	  	delay(tauNH - gt0 -gstab); zgradpulse(gzlvl7, gt0); delay(gstab);
 		rgpulse(pw, one, rof1, rof1);

             /* water defoc-refoc */
               delay(gstab); zgradpulse(gzlvl8, gt8); delay(gstab);

              /* t3 time */

               if((ni3==0))
                {
		 dec2rgpulse(pwN,t7,0.0,0.0);
     		 dec2rgpulse(pwN,two,0.0,0.0);   
		 decrgpulse(pwC, t8, 0.0, 0.0);
		 decrgpulse(pwC, two, 0.0, 0.0);
		 rgpulse(pw*2.0, zero, rof1, rof1);
                 delay(pwN*2.0+pwC*2.0);
                }
               else
                {
	      	 dec2rgpulse(pwN,t7,0.0,0.0);
                 delay(tau3p);
		 decrgpulse(pwC, t8, 0.0, 0.0);
                 delay(tau3);
		 rgpulse(pw*2.0, zero, rof1, rof1);
                 delay(tau3);
		 decrgpulse(pwC, two, 0.0, 0.0);
		 delay(tau3p);
		 dec2rgpulse(pwN,two,0.0,0.0);
                }

		/* water defoc-refoc */
               delay(gstab); zgradpulse(gzlvl8, gt8); delay(gstab);

	   	/* back inept, water to +Z */
 
		rgpulse(pw,one,rof1,rof1);                  
 	        zgradpulse(gzlvl9, gt0);      delay(tauCH-gt0);
	        decrgpulse(pwC*2.0, zero, 0.0, 0.0); delay(tauNH -tauCH -pwC*2.0 );
	   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
 	  	delay(tauNH - gt0 -gstab); zgradpulse(gzlvl9, gt0); delay(gstab);
 		rgpulse(pw,  two, rof1, rof1);

              /* purge */

		zgradpulse(gzlvl10, gt10); delay(2.0*gstab);


        }     /*end of full 4D */


  /************************* t2 evolution, 1H and NOE****************************** */
		/* for the case of no flipbacks in NOE part of the experiment, shift first pulse
                   in t2 time by 45 deg and let water bring itself back at the end
                   of mixing time by radiation dumping */

 		if(   (exp_mode[D]=='t') ) 
                 { initval(1.0, v10);
	           obsstepsize(45.0);
	           xmtrphase(v10);       
 		 } 
                else 
                 {
		   xmtrphase(zero);
		   obspower(tpwrs); obspwrf(tpwrsf_d);                         
		   shaped_pulse("H2Osinc",pwHs,t12,rof1,rof1);
		   obspower(tpwr); obspwrf(4095.0);
	         }
             
	        rgpulse(pw,t2,rof1,rof1);    
	        xmtrphase(zero);   /* SAPS_DELAY */
	        delay(tau2);

 	        decrgpulse(2.0*pwC,zero,0.0,0.0);  dec2rgpulse(2.0*pwN,zero,0.0,0.0);
	         
	        delay(tau2);

	        rgpulse(pw*2.0,zero,rof1,rof1);
	        delay(pwN*2.0+pwC*2.0 + SAPS_DELAY);
	        rgpulse(pw,zero,rof1,rof1);
               
		if(   (exp_mode[D]!='t') )
		{
		  obspower(tpwrs); obspwrf(tpwrsf_u);                         
		  shaped_pulse("H2Osinc",pwHs,zero,rof1,rof1);
		  obspower(tpwr);  obspwrf(4095.0);
 		}

	      /* NOESY period */

 	        delay(mix-gt2-4.0*gstab );     
		zgradpulse(gzlvl2, gt2);
		delay(4.0*gstab);
              
} /* end 3-4 D acquisition */


/* N-TROSY/C-HSQCse   */

       /* Hz -> HzXz INEPT */

   	rgpulse(pw,two,rof1,rof1);                 /* 1H pulse excitation */
        zgradpulse(gzlvl0, gt0);
        delay(tauCH-gt0);
        decrgpulse(pwC*2.0, zero, 0.0, 0.0); 	
	delay(tauNH -tauCH -pwC*2.0 );
   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
   	delay(tauNH - gt0 -gstab);
	zgradpulse(gzlvl0, gt0);
	delay(gstab);
 	rgpulse(pw, one, rof1, rof1);

       /* on HzXz now */
      /* water flipback*/
        obspower(tpwrs); obspwrf(tpwrsf_u);
 	shaped_pulse("H2Osinc",pwHs,two,rof1,rof1);
	obspower(tpwr); obspwrf(4095.0);

       /* purge */
	zgradpulse(gzlvl3, gt3);
	dec2phase(t4);
	delay(gstab*2.0);

       /* t1 (C) and  t1+t1'(N) evolution */

   	dec2rgpulse(pwN, t4, 0.0, 0.0);
        delay(gt4+gstab  + gt4+gstab + pwC*3.0 
			+2.0*(pwHs +2.0*rof1));
	if(decCACO[A]=='y'){ delay(2.0*caco180pw);}
	dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);  
 	delay(tau1p);   /* t1 */        
        decrgpulse(pwC,t5,0.0,0.0);
        delay(tau1*0.5);
	if(decCACO[A]=='y')
	 {
          decpower(caco180pwr);
          decshaped_pulse(caco180shape,caco180pw,zero, 0.0, 0.0);
          decpower(pwClvl);
         }
	obspower(ch180pwr);                           /*180 on methyls*/
        shaped_pulse(ch180shape,ch180pw,zero,rof1,rof1);
	obspower(tpwr);
        delay(tau1*0.5);             
       	zgradpulse(gzlvl4, gt4);        /*coding */
	delay(gstab + pwHs -ch180pw -2.0*GRADIENT_DELAY -2.0*POWER_DELAY -WFG_START_DELAY- WFG_STOP_DELAY);
        decrgpulse(2.0*pwC,zero,0.0,0.0);
	if(decCACO[A]=='y')
	 {
          decpower(caco180pwr);
          decshaped_pulse(caco180shape,caco180pw,zero, 0.0, 0.0);
          decpower(pwClvl);
         }
        /* delay(ch180pw+2.0*rof1);*/
       	zgradpulse(gzlvl5, gt4);
        delay(gstab - rof1 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY -WFG_START_DELAY- WFG_STOP_DELAY);
       
       /*Water flipback (flipdown actually ) */
         obspower(tpwrs); obspwrf(tpwrsf_d);                         
 	shaped_pulse("H2Osinc",pwHs,three,rof1,rof1);
	obspower(tpwr);  obspwrf(4095.0); 
     
/* reverse double INEPT */


        sim3pulse(pw, pwC, 0.0, t21, t23, zero, rof1, rof1);
        /* rgpulse(pw, t21, rof1, rof1); */
        zgradpulse(gzlvl11, gt1);		 
        delay(gstab);
        delay(tauCH1 -gt1 -gstab -2.0*pwC 
	+     (-2.0/M_PI*pwC-0.5*(pwN-pwC) +pwN)); 
        decrgpulse(2.0*pwC,zero,0.0,0.0);
        delay(tauNH -tauCH1 - 0.65*(pw + pwN)-rof1 -(pwC-pw) 
	-(-2.0/M_PI*pwC-0.5*(pwN-pwC) +pwN) );
        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        zgradpulse(gzlvl11, gt1);		 
        delay(gstab);
        delay(tauCH1-gt1 -gstab-2.0*pwC); 
        decrgpulse(2.0*pwC,zero,0.0,0.0);
        delay(tauNH -1.3*pwN -tauCH1);    
        sim3pulse(pw, pwC, pwN, one, zero, zero, 0.0, 0.0);
        zgradpulse(gzlvl1, gt1);		 
        delay(gstab);
        delay(tauCH2-2.0*pwC-gt1-gstab); 
        decrgpulse(2.0*pwC,zero,0.0,0.0);
        delay(tauNH -1.3*pwN-tauCH2);
        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0); 
        zgradpulse(gzlvl1, gt1);		 
        delay(gstab);
        delay(tauNH-1.6*pwN -POWER_DELAY -ch90pw*ch90corr -gt1-gstab +pwN*0.5 -PRG_START_DELAY);
  
        /* delay(ch90pw*0.5);
           sim3pulse(0.0,0.0, pwN, one, zero, t22, 0.0, 0.0);  
           delay(ch90pw*0.5);*/
        /* sim3shaped_pulse(ch90shape,"hard","hard",ch90pw,0.0, pwN, zero,zero, t22,0.0,0.0);*/

        /*  ch90corr is a fraction of ch90pw to correct for a 1H phase rollcaused by shaped 90 on
            CH3 protons for a sinc pulse ch90corr=0.41 seems to be good. */ 

        obspower(ch90pwr);
        txphase(one);
        obsunblank();    xmtron();
        obsprgon(ch90shape,1.0/ch90dmf,ch90dres);                          /*PRG_START_DELAY */
        delay(ch90pw*ch90corr-pwN*0.5);
        dec2rgpulse(pwN, t22, 0.0, 0.0);
        delay(ch90pw*(1.0-ch90corr)-pwN*0.5);
        obsprgoff();  xmtroff();    obsblank();                             /*PRG_STOP_DELAY */
        obspower(tpwr);                                                        /*POWER_DEALY */	 
        delay( gstab +gt6 +2.0*GRADIENT_DELAY 
               +2.0*POWER_DELAY -0.65*pw -POWER_DELAY
		+pwN*0.5 -ch90pw*(1.0-ch90corr) 
                -PRG_STOP_DELAY);
        rgpulse(2.0*pw, zero, rof1, rof1);
	dec2power(dpwr2); decpower(dpwr);	                            /* 2.0*POWER_DELAY */

        zgradpulse(gzlvl6*icosel, gt6);		                         /* 2.0*GRADIENT_DELAY */
        delay(gstab);
   status(C);

	setreceiver(t31);
}		 
