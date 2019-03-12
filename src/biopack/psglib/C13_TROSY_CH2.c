/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* C13_TROSY_CH2.c

    This pulse sequence will allow one to perform the following experiment:
   13C-1H TROSY  for methylene protons
      reference E.Miclet.... Ad Bax JACS v126, pp 10560-10570, 2004
      some aspects: Shaka's papers
         JMR v136 p54  1999
         JMR v151 p269 2001

         NO CARE WAS TAKEN TO ACCOUNT FOR INOVA AND ALIIKE HIDDEN DELAYS  30APR2010

        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccp'  for C13 decoupling.
    Set dm2 = 'nnn'  for N15 decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 
         set f1coef= '1 0 -1 0 0 -1 0 -1
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.

    Evgeny Tishchenko, Agilent, December 2011
    (NO CARE WAS TAKEN TO ACCOUNT FOR INOVA AND ALIKE HIDDEN DELAYS. THIS
     MAY REQUIRE NON-ZERO FREQUENCY-DEPENDENT PHASE CORRECTIONS).
*/
#include <standard.h>
static int   phi1[]  = {1,1,1,1},  /*phi1 in paper, steady-state C13 to final*/

	     phi2[]  = {1,3,2,3},  
                           /*phi2 in paper 225,45, carbon multiplet selection*/
             phi3[]  = {0,0,1,1},     /*phi3 in paper*/
	     phi4[]  = {0,0,0,0},
	     phi5[]  = {0,0,0,0},     /*phi5 in paper*/
	     phi6[]  = {0,0,0,0},     /*phi6 in paper*/
             phix[]   ={0,0,0,0},     /*phi4 in paper */
             rec[]   = {0,2,2,0};

static double   d2_init=0.0;
void pulsesequence()
{
/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            refocN15[MAXSTR],
            refocCO[MAXSTR], COshape[MAXSTR],
            C180shape[MAXSTR];
	 
 
int         t1_counter,icosel;  		        /* used for states tppi in t1 */

double      tau1,         				         /*  t1 delay */
	    tauch =  getval("tauch"), 	   /* 1/4J   JCH , first INEPT  */

            tauch1 =  getval("tauch1"), 
                   /* 1/4J*0.34   reverse INEPT, first part, tau1/2 in paper */
            tauch2 =  getval("tauch2"),  
                  /* 1/4J*0.23   reverse INEPT, second part, tau2/2 in paper */


	pwClvl = getval("pwClvl"),	              /* power for hard C pulses */
        pwC180lvl = getval("pwC180lvl"),           /*power levels for 180 shaped pulse */
        pwC180lvlF = getval("pwC180lvlF"),
        pwC = getval("pwC"),          /* C 90 degree pulse length at pwClvl */	 
        pwC180 = getval("pwC180"),   /* shaped 180 pulse on C channel */
 
	sw1 = getval("sw1"),

        pwNlvl = getval("pwNlvl"),
        pwN = getval("pwN"),
 
        pwCOlvl = getval("pwCOlvl"),
        pwCO = getval("pwCO"),

        gstab = getval("gstab"),
        gt0 = getval("gt0"),
        gt1 = getval("gt1"),
	gt2 = getval("gt2"),
        gt3 = getval("gt3"),         /* other gradients */
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),

        gt6 = getval("gt6"),
        gt7 = getval("gt7"),
        gt8 = getval("gt8"),
        gt9 = getval("gt9"),  
	
        gzlvl0 = getval("gzlvl0"),
        gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),
        gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),

	gzlvl6 = getval("gzlvl6"),
	gzlvl7 = getval("gzlvl7"),
	gzlvl9 = getval("gzlvl9");


    getstr("f1180",f1180);
    getstr("C180shape",C180shape);
    getstr("COshape",COshape); 
    getstr("refocCO",refocCO);
    getstr("refocN15",refocN15);


/*   LOAD PHASE TABLE    */

	settable(t1,4,phi1);
        settable(t2,4,phi2);
	settable(t3,4,phi3);
	settable(t4,4,phi4);
        settable(t5,4,phi5);
        settable(t6,4,phi6);
        settable(t10,4,phix);
	settable(t11,4,rec);


/*   INITIALIZE VARIABLES   */

/* CHECK VALIDITY OF PARAMETER RANGES */

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' or 'nny' "); psg_abort(1); }

  if((dm2[A] != 'n' || dm2[B] != 'n' || dm2[C] != 'n'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if( dpwr2 > 45 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }
  if( dpwr > 50 )
  { text_error("don't fry the probe, DPWR too large!  "); psg_abort(1); }

  if( (pw > 20.0e-6) )
  { text_error("don't fry the probe, pw too high ! "); psg_abort(1); }

  if( (pwC > 200.0e-6)  )
  { text_error("don't fry the probe, pwC too high ! "); psg_abort(1); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-TPPI-Haberkorn element */

if (phase1 == 1)  {tsadd(t10,2,4); icosel = +1;}
       else       {icosel = -1;}
/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1)  d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t11,2,4); }



/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 0.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;




/* BEGIN PULSE SEQUENCE */

status(A);
	obsoffset(tof);        
	obspower(tpwr);
	decpower(pwClvl); decpwrf(4095.0);
 	obspwrf(4095.0);
        delay(d1);
        txphase(zero);
        decphase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(2.0*gstab);
	delay(5.0e-3);


/************ Hz-> CzHz */
        txphase(zero);
        decphase(zero);
   	rgpulse(pw,zero,rof1,rof1);                  
        delay(tauch - gt5 -gstab);
	zgradpulse(gzlvl5, gt5);
	delay(gstab);
        simpulse(2.0*pw,2.0*pwC, zero,zero, 0.0, 0.0);
        delay(tauch - gt5-gstab);
	zgradpulse(gzlvl5, gt5);
	delay(gstab);
 	rgpulse(pw, t1, rof1,rof1);
/*** purge   */  
        dec2power(pwNlvl); dec2pwrf(4095.0);
        txphase(zero);
        decphase(t2);
        delay(gstab);
	zgradpulse(gzlvl2, gt2);
	
        initval(1.0, v10);
        decstepsize(45.0);
        dcplrphase(v10);
        delay(3.0*gstab);
/************  second part of first INEPT  */

        decrgpulse(pwC, t2, 0.0, 0.0);
        dcplrphase(zero);
        delay(tauch*0.25 - gt1 -gstab);
	zgradpulse(gzlvl1, gt1);
        delay(gstab);
        simpulse(2.0*pw,2.0*pwC, zero,zero, 0.0, 0.0);
        delay(tauch*0.25 - gt1-gstab);
	zgradpulse(gzlvl1, gt1);
	delay(gstab);
        decrgpulse(pwC, zero, 0.0, 0.0);
	
  /*  evolution in t1 */
     decphase(zero);

     if (refocN15[A]=='n')
       delay(tau1-pwC/2.0);
     else
       delay(tau1 -pwN -pwC/2.0);

     if(refocN15[A]=='y')
       dec2rgpulse(2.0*pwN,zero,0.0,0.0); /*n15 refocusing */
     if(refocCO[A]=='y') 
       { 
         decpower(pwCOlvl);
         decshaped_pulse(COshape,pwCO,zero, 3.0e-6, 3.0e-6);
         decpower(pwClvl); decpwrf(4095.0);
        }

     if (refocN15[A]=='n') 
      delay(tau1 -POWER_DELAY -GRADIENT_DELAY);
     else
      delay(tau1 - pwN - POWER_DELAY -GRADIENT_DELAY);

     decpower(pwC180lvl); decpwrf(pwC180lvlF);
     zgradpulse(gzlvl3*icosel, gt3*2.0); 
     delay(gstab);
     decshaped_pulse(C180shape,pwC180,t3, 0.0, 0.0);
     zgradpulse(-gzlvl3*icosel, gt3*2.0);
     decpower(pwClvl); decpwrf(4095.0);
     delay(gstab);
     if(refocCO[A]=='y')  
       { 
         decpower(pwCOlvl);
         decshaped_pulse(COshape,pwCO,zero, 3.0e-6, 3.0e-6);
         decpower(pwClvl); decpwrf(4095.0);
        }
     simpulse(pw, pwC, zero, t10, 0.0, 0.0);
     delay(tauch1 - gt4 - gstab);  
     zgradpulse(gzlvl4,gt4);
     delay(gstab);
     simpulse(2.0*pw,2.0*pwC, t5,zero, 0.0, 0.0);   
     delay(tauch1 - gt4 - gstab);  
     zgradpulse(gzlvl4,gt4);
     delay(gstab);
     decphase(one); txphase(one);
     simpulse(pw, pwC, one, one, 0.0, 0.0);
     delay(tauch2 - gt6 - gstab);
     zgradpulse(gzlvl6,gt6);
     delay(gstab);  
     simpulse(2.0*pw,2.0*pwC, zero,zero, 0.0, 0.0);
     delay(tauch2- gt6 - gstab);
     zgradpulse(gzlvl6,gt6);
     delay(gstab-rof1);    
     simpulse(pw, pwC, zero,zero, 0.0, 0.0);

    /* echo and decoding */

     delay(gstab+gt9-rof1+10.0e-6);
     rgpulse(2.0*pw, zero,rof1,rof1);
     delay(10.0e-6);
     zgradpulse(gzlvl9,gt9);
     decpower(dpwr);	decpwrf(4095.0);
     dec2power(dpwr2); /* POWER_DELAY EACH */
     delay(gstab);
status(C);
     setreceiver(t11);
}		 
