// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* hcchtocsy.c

    Use convoluted SLP 180 to decouple carbonyls during t2. 

    Set dm = 'nny' [13C decoupling during acquisition]
    One dipsi-3 cycle is 217.33*p1
      e.g. if p1= 36us a single dipsi cycle is 7.8ms

    Must set phase = 1,2 and phase2 = 1,2 for hypercomplex in t1 and t2.

    Typical acquisition times are 28 ms [t1], 10 ms [t2], and 47 ms [t3]
       with 128 complex [t1], 32 complex [t2], and 512 real [t3]. 

*/

#include <standard.h>

/* Define phase-table */
   static int  	psi1[1] = {0},
		phi1[2] = {0,1},
		phi2[1] = {0},
		phi3[1] = {0},
		phi4[1] = {1},
		phi5[8] = {0,0,1,1,2,2,3,3},
		phi6[8] = {0,0,0,0,2,2,2,2},

		rec[8]  = {0,2,2,0,0,2,2,0};

/* Create dipsi3 function */

void dipsi3a()
{
   decrgpulse(2.722*p1,one,0.0,0.0);      /*245 deg pulse */
   decrgpulse(4.389*p1,three,0.0,0.0);      /*395 deg pulse */
   decrgpulse(2.778*p1,one,0.0,0.0);      /*250 deg pulse */
   decrgpulse(3.056*p1,three,0.0,0.0);      /*275 deg pulse */
   decrgpulse(0.333*p1,one,0.0,0.0);      /* 30 deg pulse */
   decrgpulse(2.556*p1,three,0.0,0.0);      /*230 deg pulse */
   decrgpulse(4.000*p1,one,0.0,0.0);      /*360 deg pulse */
   decrgpulse(2.722*p1,three,0.0,0.0);      /*245 deg pulse */
   decrgpulse(4.111*p1,one,0.0,0.0);      /*370 deg pulse */
   decrgpulse(3.778*p1,three,0.0,0.0);      /*340 deg pulse */
   decrgpulse(3.889*p1,one,0.0,0.0);      /*350 deg pulse */
   decrgpulse(2.889*p1,three,0.0,0.0);      /*260 deg pulse */
   decrgpulse(3.000*p1,one,0.0,0.0);      /*270 deg pulse */
   decrgpulse(0.333*p1,three,0.0,0.0);      /* 30 deg pulse */
   decrgpulse(2.500*p1,one,0.0,0.0);      /*225 deg pulse */
   decrgpulse(4.056*p1,three,0.0,0.0);      /*365 deg pulse */
   decrgpulse(2.833*p1,one,0.0,0.0);      /*255 deg pulse */
   decrgpulse(4.389*p1,three,0.0,0.0);      /*395 deg pulse */
}

void dipsi3b()
{
   decrgpulse(2.722*p1,three,0.0,0.0);      /*245 deg pulse */
   decrgpulse(4.389*p1,one,0.0,0.0);      /*395 deg pulse */
   decrgpulse(2.778*p1,three,0.0,0.0);      /*250 deg pulse */
   decrgpulse(3.056*p1,one,0.0,0.0);      /*275 deg pulse */
   decrgpulse(0.333*p1,three,0.0,0.0);      /* 30 deg pulse */
   decrgpulse(2.556*p1,one,0.0,0.0);      /*230 deg pulse */
   decrgpulse(4.000*p1,three,0.0,0.0);      /*360 deg pulse */
   decrgpulse(2.722*p1,one,0.0,0.0);      /*245 deg pulse */
   decrgpulse(4.111*p1,three,0.0,0.0);      /*370 deg pulse */
   decrgpulse(3.778*p1,one,0.0,0.0);      /*340 deg pulse */
   decrgpulse(3.889*p1,three,0.0,0.0);      /*350 deg pulse */
   decrgpulse(2.889*p1,one,0.0,0.0);      /*260 deg pulse */
   decrgpulse(3.000*p1,three,0.0,0.0);      /*270 deg pulse */
   decrgpulse(0.333*p1,one,0.0,0.0);      /* 30 deg pulse */
   decrgpulse(2.500*p1,three,0.0,0.0);      /*225 deg pulse */
   decrgpulse(4.056*p1,one,0.0,0.0);      /*365 deg pulse */
   decrgpulse(2.833*p1,three,0.0,0.0);      /*255 deg pulse */
   decrgpulse(4.389*p1,one,0.0,0.0);      /*395 deg pulse */
}
void pulsesequence()
{
/* DECLARE VARIABLES */

 char        coshape[MAXSTR],   /* dec pattern for CO decoupling  */
             fhfdwt1[MAXSTR],   /* Flag to indicate half dwell start in t1 */
             fhfdwt2[MAXSTR];   /* Flag to indicate half dwell start in t2 */

 int       
	     satmove;

 double      tauhc,            /* 1 / 4*J[13C-H]     [~1.5ms]          */
	     tau1,		/* t1-evolution variable */
	     tau2,		/* t2-evolutionvariable */
             pwca,             /* PW90 for 13C nucleus                 */
             pwco,             /* PW90 for 13C carbonyl decoupling      */
             pwcalvl,           /* power level for 13C pulses on dec1   */
             pwcolvl,          /* power level for C=O decoupling pulse   */
             jch,              /* coupling for C-C  (set to 40 Hz   )  */
             ncyc,             /* # cycles through dipsi loop          */
             trim,             /* trim pulse length(sec)               */
             dipsipwr,         /* power level for 13C spin lock        */
             delta1,delta2;

/* LOAD VARIABLES */
  dipsipwr = getval("dipsipwr");
  ncyc = getval("ncyc");
  trim = getval("trim");
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  pwca = getval("pwca");
  pwco = getval("pwco");
  pwcalvl = getval("pwcalvl");
  pwcolvl = getval("pwcolvl");
  jch = getval("jch");       /*   Use 152   [overestimate; true J ~140 ] */
  getstr("coshape",coshape);
  getstr("fhfdwt1",fhfdwt1);
  getstr("fhfdwt2",fhfdwt2);


/* calculate delays */
  tauhc = (1.0/(4*jch)); 
  delta1 = (1.0/(6.0*jch));
  delta2 = (delta1);
  satmove = (fabs(tof - satfrq) >= 0.2);

/* CHECK VALIDITY OF PARAMETER RANGE */

    if((dm[A] != 'n') || (dm[B] != 'n'))
    {
        printf("dm should be 'nnn' or 'nny' ");
        psg_abort(1);
    }

    if((dm2[A] != 'n') || (dm2[B] != 'n') || (dm2[C] != 'n'))
    {
        printf("dm2 should be 'nnn'  ");
        psg_abort(1);
    }

    if( satpwr > 20 )
    {
        printf("SATPWR too large !!!  ");
        psg_abort(1);
    }


    if( pwcolvl > 60 )
    {
        printf("DCOPWR too large!");
        psg_abort(1);
    }

    if( dpwr > 48 )
    {
        printf("don't fry the probe, DCPWR too large!  ");
        psg_abort(1);
    }

    if(( pwca > 2.5e-5 || pw > 2.5e-5 ))
    {
        printf("Pulsewidths [pw, pwca] must be shorter than 25 us. Abort."); 
        psg_abort(1);
    }


    if ( pwco > 2.0e-3 )
    {
        printf("PWCO must be less than 2 ms. Abort.");
        psg_abort(1);
    }
 
    if ( ncyc*217.33*p1 > 0.03)
    {
        printf("mixing time must be < 30ms! Abort.");
        psg_abort(1);
    }

    if (ncyc > 4)
    {
       printf("ncyc should be no greater than 4. Abort.");
       psg_abort(1);
    }


/* INITIALIZE VARIABLES */

  if(fhfdwt1[0] == 'y')
    tau1 = (d2 + 1/(2*sw1) );
  else
    tau1 = d2;

  if(fhfdwt2[0] == 'y')
    tau2 = (d3 + 1/(2*sw2));
  else
    tau2 = d3;

 initval(ncyc,v7);                 /* v7 is the dipsi loop counter */

	settable(t21,1,psi1);
	settable(t11,2,phi1);
	settable(t12,1,phi2);
	settable(t13,1,phi3);
	settable(t14,1,phi4);
	settable(t15,8,phi5);
	settable(t16,8,phi6);
	settable(t10,8,rec);
                                  /* Phase table:

                                     phi1 = t11 = 0 1


                                     phi2 = t12 = 0

                                     phi3 = t13 = 0 0 0 0 0 0 0 0

                                     phi3'= t14 = 1 1 1 1 1 1 1 1 


                                     phi5 = t15 = 0 0 1 1 2 2 3 3

                                     phi6 = t16 = 0 0 0 0 2 2 2 2

   THESE TO BE SOFTWARE-MODIFIED BASED ON t1, t2 VALUES:
                                     rec  = t10 = 0 2 2 0 0 2 2 0
				     psi1 = t21 = 0

                                     psi2 = t22 = 0
*/

 if(phase1==2) tsadd(t21,1,4);
 if(phase2==2) tsadd(t12,1,4);
 
 if(d2_index%2)
 {
	tsadd(t21,2,4);
	tsadd(t10,2,4);
 }
 if(d3_index%2)
 {
	tsadd(t12,2,4);
	tsadd(t10,2,4);
 }


/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);
   if (satmode[A] == 'y')
   {
	if(satmove)
		obsoffset(satfrq);
	obspower(satpwr);
    	rgpulse(d1,zero,rof1,rof1);
	if(satmove)
		obsoffset(tof);
   }
   else
   {
    	delay(d1);
   }
   rcvroff();
   decphase(t13);
   obspower(tpwr);   /* Set transmitter power for hard 1H pulses */
   decpower(pwcalvl);
status(B);
   rgpulse(pw,t21,1.0e-5,0.0);                /* First 1H 90 degree pulse     */
   txphase(t11);
   if(tau1 < (2*pwca + pw))	/* first t1-value */
   {
	delay(tauhc - 2*pwca - 1.0e-6);  /* assuming pwca > pw */
     	decrgpulse(pwca,t13,0.0,0.0); /* 13C composite 180 deg pulse  */
	simpulse(2*pw, 2*pwca,t11, t14, 1.0e-6, 0.0);
     	decrgpulse(pwca,t13,1.0e-6,0.0); /* 13C composite 180 deg pulse  */
	txphase(one);
	decphase(t12);
	delay(tauhc - 2*pwca - 1.0e-6);  /* assuming pwca > pw */
   }
   else
   {
	delay(tauhc + tau1/2 - 2*pwca -1.0e-6); /* t1-evol. plus pol. trans. */
     	decrgpulse(pwca,t13,0.0,0.0); /* 13C composite 180 deg pulse  */
     	decrgpulse(2*pwca,t14,1.0e-6,0.0); /* 13C composite 180 deg pulse  */
     	decrgpulse(pwca,t13,0.0,0.0); /* 13C composite 180 deg pulse  */
	delay(tau1/2 - 2*pwca - pw);       /* continued t1-evol. */
        rgpulse(2*pw,t11,0.0,0.0);      /* proton echo pulse */
	txphase(one);
	decphase(t12);
	delay(tauhc - pw);
   }

   simpulse(pw,pwca,one,t12,1.0e-6,0.2e-6);
	decphase(zero);
      	decpower(pwcolvl);
        if(tau2/2 > POWER_DELAY)
           delay(tau2/2 - POWER_DELAY);     /* t2 evolution */
        else
           delay(tau2/2);
        decshaped_pulse(coshape,pwco,zero,0.0,0.0);
      	decpower(pwcalvl);
        delay(delta1 - pwco - POWER_DELAY - WFG_START_DELAY - WFG_STOP_DELAY - pw);
        rgpulse(2*pw,zero,0.0,0.0);
        if (tau2/2 > (pw + pwca))
          delay(tau2/2 - pw -pwca);         /* t2 evolution */
        else
          delay(tau2/2);
      	decrgpulse(2*pwca,t15,0.0,0.0);
	decphase(t16);
        decpower(dipsipwr);
        delay(delta1 - pwca - POWER_DELAY);
       if (ncyc>0.0)
       {
         decrgpulse(trim,t16,0.0,0.0); 
         starthardloop(v7);
          dipsi3a(); dipsi3b(); dipsi3b(); dipsi3a();
         endhardloop();
       }
        txphase(zero); decphase(zero);
      	decpower(pwcalvl);
        delay(delta2 - POWER_DELAY - pwca);
        simpulse(2*pw,2*pwca,zero,zero,0.0,0.0);
        txphase(t13);
        delay(delta2);
     /* start reversed INEPT */
        simpulse(pw,pwca,t13,zero,0.0,0.0);
   decphase(zero);
   txphase(zero);
   delay(tauhc - 2*pwca - 1.0e-6);    /* delay = 1/4J     */
   decrgpulse(pwca,zero,0.0,0.0);
   simpulse(2*pw,2*pwca,zero,one,1.0e-6,0.0);
   decrgpulse(pwca,zero,1.0e-6,rof2);
   delay(tauhc - 2*pwca - 1.0e-6 - rof2);     /* delay = 1/4J       */
   decrgpulse(pwca,zero,0.0,0.0);  /* Filter out IySz terms        */
   decrgpulse(pwca,t16,1.0e-6,0.0);
   decpower(dpwr);           /* Set power for decoupling     */

/* BEGIN ACQUISITION */
   setreceiver(t10);
status(C);
}
