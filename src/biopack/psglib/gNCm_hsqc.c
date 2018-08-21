/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gNCm_hsqc.c

Time-shared 15N 13Cm HSQC 

#  Pulse Sequence by Peter Würtz and Perttu Permi
P. Würtz, O. Aitio, M. Hellman and P. Permi
Simultaneous Detection of Amide and Methyl Correlations Using Time Shared NMR Experiment:
Application to Binding Epitope Mapping
J. Biomol NMR 39,97-105(October 2007)

    Set dm = 'nny', dmm = 'ccp', dm2 = 'nnn'

    Set phase = 1,2  for States-TPPI acquisition in t1.
    
   INSTRUCTIONS FOR USE OF gNCm_hsqc
         
    1. Run gNCm_hsqc macro.

    2. Set sw1 (C13) and sw2 (N15). sw1 >= sw2. kappa is calculated automatically.
    3. Processing of simultaneous absorptive phase of 13Cm-Hm and 15N-1H 
       is not possible. Use VnmrJ menus in Process...Process in t1 page.
    4. Refocusing gradients may need to be fine-tuned for optimal performance
       (array gzlvl2 for best NH signal. This may be closer to gzlvl1 than
       in gNhsqc).

*/

#include <standard.h>
	     
static int   
             phi4[1] = {0},
             phi3[2] = {0,2},
             phi5[2] = {0,2},
 	         phi7[1] = {2},	
             rec[2]  = {0,2};


static double   d2_init=0.0;


pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		 /* Flag to start c13 t1  @ halfdwell */
	    f2180[MAXSTR],		 /* Flag to start n15 t1  @ halfdwell */
            shape_rSnobMe[MAXSTR];       /* methyl refocussing shape name     */
 
int         icosel,          			  /* used to get n and p type */
            t1_counter; 		        /* used for states tppi in t1 */

double      tau1,         				         /*  t1 delay */
	    lambda = 1.0/(4.0*getval("JNH")),    /*1/(4J*NH) evolution delay */
	    del1,				   		  /* 1/4J(CH) */
	    kappa,                    /*  relation between sw1(C) and sw1(N)  */
      
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */

   compH = getval("compH"),        /* adjustment for H1 amplifier compression */

  tpwrsf_n = getval("tpwrsf_n"), /* fine power adustment for H2O soft pulse*/
  pwHs = getval("pwHs"),          /* H1 90 degree pulse length at tpwrs */
  tpwrs,                        /* power for the pwHs ("H2Osinc") pulse */

  pw_rSnobMe = getval("pw_rSnobMe"),     /* H1 90 degree pulse length at tpwr_rSnobMe */
  tpwr_rSnobMe = getval("tpwr_rSnobMe"),  /* power for the methyl ("Hm90") pulse */
  tpwrsf_rSnobMe = getval("tpwrsf_rSnobMe"), /* fine power adustment for methylH selective pulse */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
	sw1 = getval("sw1"),



	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
   	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt0 = getval("gt0"),				   /* other gradients */
        gstab = getval("gstab"),
	gt3 = getval("gt3"),
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("shape_rSnobMe",shape_rSnobMe);

/*   LOAD PHASE TABLE    */

    settable(t3,2,phi3);
    settable(t7,1,phi7);
 	settable(t4,1,phi4);
    settable(t5,2,phi5);
	settable(t12,2,rec);



/*   INITIALIZE VARIABLES   */


/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                        /*power than a square pulse */

/* Set delays in mixing */
	del1 = 1/(4.0*getval("JCH"));    /* 1/(4J*NH) evolution delay */

/* CHECK VALIDITY OF PARAMETER RANGES */


  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( dpwr2 > 46 )
  { text_error("don't fry the probe, DPWR2 too large!  ");   	    psg_abort(1); }

  if( pw > 50.0e-6 )
  { text_error("dont fry the probe, pw too high ! ");               psg_abort(1); } 
  
  if( pwN > 100.0e-6 )
  { text_error("dont fry the probe, pwN too high ! ");              psg_abort(1); }


 

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

icosel=-1;
if (phase1 == 1)  { icosel = -1;}
if (phase1 == 2)  { tsadd(t7,2,4); tsadd(t4,1,4); icosel = 1;}
if (phase1 == 3)  { tsadd(t4,2,4); icosel = -1;}
if (phase1 == 4)  { tsadd(t7,2,4); tsadd(t4,3,4); icosel = 1;}




/*  Set up f1180  */
   
    tau1 = d2 ;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) - (4.0/PI)*pwC - 2.0*pw); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0 ;


/* adjusting signal spread relation in N and C  */
	kappa = sw1/sw2 - 1.0 ;   

/* half dwell time  compensation for N15
	n15halfdwell = 0.0;
if((f1180[A] == 'y') && (f2180[A] == 'n') && (ni > 1.0)) {
		}
 if((f2180[A] == 'y') && (ni > 1.0)) 
	{ 
	n15halfdwell = ( 1.0/(2.0*sw2) - (1+kappa)*c13hd_tmp -2.0*pw - 2.0*pwC - (4.0/PI)*pwN); 
	if(n15halfdwell < 0.2e-6) n15halfdwell = 0.0; }
*/


/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t4,2,4); tsadd(t12,2,4); }



/* BEGIN PULSE SEQUENCE */

status(A);

        obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	txphase(zero);
        decphase(zero);
        dec2phase(zero);

	delay(d1);

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

    rcvroff();
	dec2rgpulse(pwN, zero, 0.0, 0.0);  
	decrgpulse(pwC, zero, 0.0, 0.0);   /*destroy N15 and C13 magnetization*/
	zgradpulse(1.7*gzlvl0, 0.5e-3);
	delay(1.0e-4);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	txphase(zero);
	delay(5.0e-4);

   if(dm3[B] == 'y')				  /*optional 2H decoupling on */
         {lk_hold();
	  dec3unblank();
          dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank();
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);} 

   	rgpulse(pw,zero,0.0,0.0);                 /* 1H pulse excitation */

	txphase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(del1 - gt0 - pwC);

	decrgpulse(2.0*pwC, zero, 0.0, 0.0); 

	delay(lambda - del1 - pwC);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	txphase(one);
	delay(lambda - gt0 - gstab);        
        zgradpulse(gzlvl0, gt0);
        delay(gstab);

 	rgpulse(pw, one, 0.0, 0.0);

if (tpwrsf_n<4095.0)
        {
         obspower(tpwrs+6.0);
           obspwrf(tpwrsf_n);
           shaped_pulse("H2Osinc_n", pwHs, two, 0.0, 0.0);
         obspower(tpwr); obspwrf(4095.0);
        }
        else
        {   
         obspower(tpwrs);
         shaped_pulse("H2Osinc_n", pwHs, two, 0.0, 0.0);
         obspower(tpwr);
        }

	zgradpulse(gzlvl3, gt3);
	txphase(zero);
	dec2phase(t3);
	delay(gstab);
   	dec2rgpulse(pwN, t3, 0.0, 0.0);  

	delay(kappa*tau1);       

	decphase(t4);

	decrgpulse(pwC, t4, 0.0, 0.0);

        delay(tau1);
        rgpulse(2.0*pw,zero,0.0,0.0);
        delay(tau1);
  
        decrgpulse(pwC, t5, 0.0,0.0);

        delay(kappa*tau1);
        
        zgradpulse(1*icosel*gzlvl1, 0.5*gt1); 
        delay(gstab - pwC + pw);
        dec2rgpulse(2.0*pwN,zero,0.0,0.0);
        zgradpulse(-1*icosel*gzlvl1, 0.5*gt1);
        delay(gstab + pwC - pw);

	dec2phase(t7);
	
/*  xxxxxxxxx     BACK TRANSFER      xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

	sim3pulse(pw, 0.0, pwN, zero, zero, t7, 0.0, 0.0);  

	txphase(zero);
	decphase(zero);
	dec2phase(zero);
	zgradpulse(0.45*gzlvl5, gt5);
  
	delay(del1 - gt5 - pwC);
	decrgpulse(2.0*pwC, zero, 0.0, 0.0); 
	delay(lambda - del1 - pwC - 1.5*pwN);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	txphase(one);
	decphase(zero);
	dec2phase(zero);

	delay(lambda - gt5 - gstab - 1.5*pwN);	
        zgradpulse(0.45*gzlvl5, gt5);
        delay(gstab);

	sim3pulse(pw, 0.0, pwN, one, zero, one, 0.0, 0.0);

	zgradpulse(0.64*gzlvl5, gt5);
 
	delay(lambda - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	delay(lambda - gt5 - gstab);
        zgradpulse(0.64*gzlvl5, gt5);
        delay(gstab);

	txphase(zero);
	dec2phase(zero);

	rgpulse(pw,zero,0.0,0.0);

	delay(0.5*gt1/10.0 + gstab);

   /* shaped pulse for methyl flip */
      obspower(tpwr_rSnobMe);
      obspwrf(tpwrsf_rSnobMe);
      shaped_pulse(shape_rSnobMe,pw_rSnobMe,zero,0.0,0.0);
      obspower(tpwr); obspwrf(4095.0);
   /* shaped pulse for methyl flip end*/

	delay(0.5*gt1/10.0 + gstab); 

	rgpulse(2.0*pw,zero,0.0,0.0);

        zgradpulse(gzlvl2, 0.5*gt1/10.0);
	delay(gstab -2.0*GRADIENT_DELAY);

   /* shaped pulse for methyl flip  */
      obspower(tpwr_rSnobMe); obspwrf(tpwrsf_rSnobMe);
      shaped_pulse(shape_rSnobMe,pw_rSnobMe,zero,0.0,0.0);
      obspower(tpwr); obspwrf(4095.0);
   /* shaped pulse for methyl flip end*/

        zgradpulse(gzlvl2, 0.5*gt1/10.0);
	delay(gstab - 2.0*POWER_DELAY -2.0*GRADIENT_DELAY);

  if (dm3[B] == 'y')			         /*optional 2H decoupling off */
        { dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
          setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
          dec3blank(); }

	decpower(dpwr);
	dec2power(dpwr2);		/* POWER_DELAY */
status(C);		

  if (dm3[B] == 'y') {delay(1/dmf3); lk_sample();}

	setreceiver(t12);
}		 
