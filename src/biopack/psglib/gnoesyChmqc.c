/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  

   3D C13 edited noesy with separation via the carbon of the destination site
        recorded on a D2O sample (protein or RNA)

   Coded starting from gChmqc experiment.
   Marco Tonelli / Klaas Hallenga @ NMRFAM - October 2003

*/

#include <standard.h>

static int   phi1[4]  = {0,0,2,2},
	     phi2[1]  = {0},
	     phi3[2]  = {0,2},
             phi4[8]  = {0,0,0,0, 2,2,2,2},
             rec[8]   = {0,2,2,0, 2,0,0,2};

static double d2_init=0.0, d3_init=0.0;

pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
	    f2180[MAXSTR],   		      /* Flag to start t2 @ halfdwell */
	    N15refoc[MAXSTR],		      /* Flag for N15 labeled samples */
	    satmode[MAXSTR];
 
int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  		        /* used for states tppi in t2 */
	    ni2=getval("ni2");

double      tau1,tau2,         				   /* t1 and t2 delay */
	    mix = getval("mix"),		 	    /* NOESY mix time */
            lambda = 1.0 / (2.0*getval("JCH")),                 /* HMQC delay */

   satpwr = getval("satpwr"),
   satdly = getval("satdly"),
   satfrq = getval("satfrq"),
        
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

   pwNlvl = getval("pwNlvl"),	              		/* power for N15 pulses */
   pwN = getval("pwN"),          	/* N15 90 degree pulse length at pwNlvl */
   compN = getval("compN"),         /* adjustment for N15 amplifier compression */

   pwClw=getval("pwClw"), 
   pwNlw=getval("pwNlw"),
   pwZlw=0.0,

   sw1 = getval("sw1"),
   sw2 = getval("sw2"),


   gt1 = getval("gt1"),  		       
   gt5 = getval("gt5"),  		       
   gzlvl1 = getval("gzlvl1"),
   gzlvl5 = getval("gzlvl5"),
   gstab = getval("gstab");


   getstr("f1180",f1180);
   getstr("f2180",f2180);
   getstr("satmode",satmode);
   getstr("N15refoc",N15refoc);


/*   LOAD PHASE TABLE    */

   settable(t1,4,phi1);
   settable(t2,1,phi2);
   settable(t3,2,phi3);
   settable(t4,8,phi4);

   settable(t9,8,rec);

/*   INITIALIZE VARIABLES   */


/* CHECK VALIDITY OF PARAMETER RANGES */

  if ( (mix - gt1 -gstab -2.0*GRADIENT_DELAY) < 0.0 )
  { text_error("mix is too small. Make mix equal to %f or more.\n",(gt1 +gstab +2.0*GRADIENT_DELAY));
						   		    psg_abort(1); }

  if( dpwr2 > 46 )
  { text_error("don't fry the probe, DPWR2 too large!  ");   	    psg_abort(1); }

  if( pw > 20.0e-6 )
  { text_error("dont fry the probe, pw too high ! ");               psg_abort(1); } 
  
  if( pwN > 100.0e-6 )
  { text_error("dont fry the probe, pwN too high ! ");              psg_abort(1); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2) tsadd(t1,1,4);  
    if (phase2 == 2) tsadd(t3,1,4);  

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
	{ tsadd(t1,2,4); tsadd(t9,2,4); }

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t9,2,4); }



/* calculate 3db lower power hard pulses for simultaneous CN decoupling during
   indirect H1 evoluion pwNlw and pwClw should be calculated by the macro that
   calls the experiment. */

  if (N15refoc[A] == 'y')
    {
     if (pwNlw==0.0) pwNlw = compN*pwN*exp(3.0*2.303/20.0);
     if (pwClw==0.0) pwClw = compC*pwC*exp(3.0*2.303/20.0);
     if (pwNlw > 2.0*pwClw)
         pwZlw=pwNlw;
      else
         pwZlw=2.0*pwClw;
/* Uncomment to check pwClw and pwNlw */
     if (d2==0.0 && d3==0.0) printf(" pwClw = %.2f ; pwNlw = %.2f\n", pwClw*1e6,pwNlw*1e6);
    }


/* BEGIN PULSE SEQUENCE */

status(A);
   decpower(pwClvl); 
   dec2power(pwNlvl);
   txphase(zero);
   decphase(zero);
   dec2phase(zero);

   if ((satmode[A]=='y') && (satfrq!=tof)) obsoffset(satfrq);
   else obsoffset(tof);

   delay(d1);
   rcvroff();

/* presat */
   if (satmode[A]=='y')
     {
      obspower(satpwr);
      rgpulse(satdly,zero,rof1,rof1);
      if (satfrq!=tof) obsoffset(tof);
     }
   obspower(tpwr);

   rgpulse(pw, t1, rof1, rof1);
/* T1 EVOLUTION TIME STARTS */
   txphase(zero);


   if ((N15refoc[A]=='y') &&
       (tau1 > pwZlw +2.0*pw/PI +2.0*SAPS_DELAY +2.0*POWER_DELAY +2.0*rof1))
     {
      decpower(pwClvl-3.0); dec2power(pwNlvl-3.0);
      delay(tau1 -pwZlw -2.0*pw/PI -2.0*POWER_DELAY -2.0*SAPS_DELAY -2.0*rof1);

      if (pwNlw > 2.0*pwClw)
        {
         dec2rgpulse(pwNlw -2.0*pwClw,zero,rof1,0.0);
         sim3pulse(0.0,pwClw,pwClw,zero,zero,zero,0.0,0.0);
         decphase(one);
         sim3pulse(0.0,2*pwClw,2*pwClw,zero,one,zero,0.0,0.0);
         decphase(zero);
         sim3pulse(0.0,pwClw,pwClw,zero,zero,zero,0.0,0.0);
         dec2rgpulse(pwNlw -2.0*pwClw,zero,0.0,rof1);
        }
       else
        {
         decrgpulse(2.0*pwClw-pwNlw,zero,rof1,0.0);
         sim3pulse(0.0,pwNlw-pwClw,pwNlw-pwClw,zero,zero,zero,0.0,0.0);
         decphase(one);
         sim3pulse(0.0,2.0*pwClw,2.0*pwClw,zero,one,zero,0.0,0.0);
         decphase(zero);
         sim3pulse(0.0,pwNlw-pwClw,pwNlw-pwClw,zero,zero,zero,0.0,0.0);
         decrgpulse(2.0*pwClw-pwNlw,zero,0.0,rof1);
        }

      decpower(pwClvl); dec2power(pwNlvl);
      delay(tau1 -pwZlw -2.0*pw/PI -2.0*POWER_DELAY -SAPS_DELAY -2.0*rof1);
     }
   else if ((tau1 -(2.0*pwC +2.0*pw/PI +2.0*SAPS_DELAY +2.0*rof1)) > 2.0e-6)
     {
      delay(tau1 -2.0*pwC -2.0*pw/PI -2.0*SAPS_DELAY -2.0*rof1);
      decrgpulse(pwC, zero, rof1, 0.0);
      decphase(one);
      decrgpulse(2.0*pwC, one, 0.0, 0.0);
      decphase(zero);
      decrgpulse(pwC, zero, 0.0, rof1);
      delay(tau1 -2.0*pwC -2.0*pw/PI -SAPS_DELAY -2.0*rof1);
     }
   else if ((2.0*tau1 -(4.0*pw/PI +SAPS_DELAY +2.0*rof1)) > 2.0e-6)
      delay(2.0*tau1 -4.0*pw/PI -SAPS_DELAY -2.0*rof1);

/* T1 EVOLUTION TIME ENDS */
   rgpulse(pw, zero, rof1, rof1);
/* MIXING TIME STARTS */

   delay(mix -gt1 -2.0*GRADIENT_DELAY -gstab);
   zgradpulse(gzlvl1, gt1);
   delay(gstab);

/* MIXING TIME ENDS */

   rgpulse(pw, zero, rof1, rof1);

   zgradpulse(gzlvl5, gt5);
   txphase(t2);
   decphase(t3);
   delay(lambda -gt5 -2.0*GRADIENT_DELAY -2.0*SAPS_DELAY);	/* 1/2*JCH */

   decrgpulse(pwC, t3, rof1, rof1);
/* T2 EVOLUTION TIME STARTS */
   decphase(t4);

   if (N15refoc[A]=='y' && tau2 > (pwN +2*pwC/PI +SAPS_DELAY +2.0*rof1))
     {
      delay(tau2 -pwN -2*pwC/PI -SAPS_DELAY -2.0*rof1);
      sim3pulse(2.0*pw, 0.0, 2.0*pwN, t2, zero, zero, rof1, rof1);
      delay(tau2 -pwN -2*pwC/PI -2.0*rof1);
     }
   else if (tau2 > (pw +2*pwC/PI +SAPS_DELAY +2.0*rof1))
     {
      delay(tau2 -pw -2*pwC/PI -SAPS_DELAY -2.0*rof1);
      rgpulse(2.0*pw, t2, rof1, rof1);
      delay(tau2 -pw -2*pwC/PI -2.0*rof1);
     }
   else rgpulse(2.0*pw, zero, rof1, rof1);
     
/* T2 EVOLUTION TIME ENDS */
   decrgpulse(pwC, t4, rof1, rof1);

   zgradpulse(gzlvl5, gt5);
   decpower(dpwr);
   dec2power(dpwr2);
   delay(lambda -gt5 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY);

status(C);
    setreceiver(t9);
}
