/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gChmqcnoesy_Cfilt.c

    This pulse sequence will allow one to perform the following experiment:

    3D C13 edited noesy with separation via the carbon of the origination site 
    with purging so that NOEs are from carbon label to non-carbon label.
                       F1      1H		SW approx 4200 on 500
                       F2      13C		SW approx 3000 on 500
                       F3(acq) 1H 		SW approx 8000 on 500

    Uses three channels:
         1)  1H             - carrier @ 3.0 ppm 	  SW approx 4200 on 500
         2) 13C             - carrier @ 43 ppm [CA/CB]    SW approx 3000 on 500

    Set dm = 'nnnn', dmm = 'cccc' [NO 13C decoupling during acquisition].
    Set dm2 = 'nnnn', dmm2 = 'cccc'

    Typically run in D2O 

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [H]  and t2 [C].

    Flags
          satmode    'y' for presaturation of H2O
          f1180      'y' for 180deg linear phase correction in F1
                         otherwise 0deg linear phase correction in F1.
          f2180      'y' for 180deg linear phase correction in F2
 
    Standard Settings:
          satmode='n', f1180='n', f2180='n' 

   Written by L.E.K on Sept 22/93

   REF: Lee et. al.  FEBS Letters 350, 87-90 (1994)
   Modified 2 Feb 2007 by Brian Sykes and Ian Robertson (U.Alberta)
   from cnoesy_3c_sed_pfg_purge+_500.c

*/

#include <standard.h>
#include "mfpresat.h"

static int  phi1[2]  = {0,2},
            phi2[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
            phi3[4]  = {0,0,2,2},
            phi4[8]  = {0,0,0,0,1,1,1,1},
            rec[16]  = {0,2,0,2,3,1,3,1,2,0,2,0,1,3,1,3};
                    
static double d2_init=0.0, d3_init=0.0;
            
pulsesequence()
{
/* DECLARE VARIABLES */

 char       satmode[MAXSTR],
            mfsat[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR];    /* Flag to start t2 @ halfdwell             */


 int         phase,
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/2JHC =  3.6 ms */
             mix,          /* mixing time in seconds */
             pwC,          /* pw90 for C13 nuclei @ pwClvl         */
             pwClvl,       /* power level for 13C pulses on dec1  */
             satpwr,       /* low level 1H trans power for presat  */
             satfrq,       /* tof for presat                       */ 
             jhc1,         /* smallest coupling that you wish to purge */
             jhc2,         /* largest coupling that you wish to purge */
             taud,         /* 1/(2jhc1)   */
             taue,         /* 1/(2jhc2)   */
             tauch,        /* taua/2.0              */ 

             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gstab,gzlvl1, 
             gzlvl2, 
             gzlvl3, 
             gzlvl4, 
             gzlvl5; 

/* LOAD VARIABLES */


  getstr("satmode",satmode);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("mfsat",mfsat);

  satfrq  = getval("satfrq");
  satpwr = getval("satpwr");
  taua   = getval("taua"); 
  mix = getval("mix");
  pwC = getval("pwC");
  pwClvl = getval("pwClvl");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  jhc1 = getval("jhc1");
  jhc2 = getval("jhc2");

  tauch = taua/2.0;

  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");

  gstab = getval("gstab");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");

/* LOAD PHASE TABLE */

  settable(t1,2,phi1);
  settable(t2,16,phi2);
  settable(t3,4,phi3);
  settable(t4,8,phi4);
  settable(t5,16,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */


    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y'|| dm[D] == 'y' ))
    {
        printf("incorrect dec decoupler flags!  ");
        psg_abort(1);
    }

/*    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' || dm2[D] == 'y'))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnnn' ");
        psg_abort(1);
    }
*/
    if( satpwr > 6 )
    {
        printf("satpwr too large !!!  ");
        psg_abort(1);
    }

    if( dpwr > 49 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

   if( pwClvl > 62 )
    {
        printf("don't fry the probe, pwClvl too large!  ");
        psg_abort(1);
    }


    if( pw > 200.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 

    if( pwC > 200.0e-6 )
    {
        printf("dont fry the probe, pwC too high ! ");
        psg_abort(1);
    } 


    if( gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 || gt4 > 15e-3 || gt5 > 15e-3 ) 
    {
        printf("gti values < 15e-3\n");
        psg_abort(1);
    } 

    if(jhc1 > jhc2)
    {
        printf("set jhc1 < jhc2\n");
        psg_abort(1);
    }

/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2)
      tsadd(t1,3,4);
    if (phase2 == 2)
      tsadd(t2,1,4);

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) );
    if(tau1 < 0.2e-6) tau1 = 8.0e-7;
    }
    tau1=tau1/2.0;
/*  Set up f2180  tau2 = t2               */

    tau2 = d3;

    if(f2180[A] == 'y') 
        tau2 += ( 1.0 / (2.0*sw2) - 4.0/PI*pwC - 2.0*pw - 4.0e-6); 
     else
        tau2 = ( tau2 - 4.0/PI*pwC - 2.0*pw - 4.0e-6); 

    if(tau2 < 0.2e-6)  tau2 = 4.0e-7;
    tau2 = tau2/2.0;

    if(jhc1 != 0.0)
            taud = 1.0/(2.0*jhc1);
       else taud=0.0042;
    if(jhc2 != 0.0)
            taue = 1.0/(2.0*jhc2);
       else taue = 0.0033;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t1,2,4);     
      tsadd(t5,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t2,2,4);  
      tsadd(t5,2,4);    
    }

status(A);			 /* BEGIN ACTUAL PULSE SEQUENCE */
   obspower(tpwr);	         /* Set transmitter power for 1H pulse */
   decpower(pwClvl);	         /* Set dec power for hard 13C pulses  */
   if (satmode[A] == 'y')	 /* Presaturation Period */
    {
       if (mfsat[A] == 'y')
        {obsunblank(); mfpresat_on(); delay(satdly); mfpresat_off(); obsblank();}
       else
        {
         obspower(satpwr);	 /* Set transmitter power for H2O saturation  */
	 if (tof != satfrq) obsoffset(satfrq);
	 delay(2.0e-5);
         rgpulse(satdly,zero,2.0e-6,2.0e-6); /* presaturation */
   	 obspower(tpwr);       /* Set transmitter power for hard 1H pulses */
	 if (tof != satfrq) obsoffset(tof);
    	 delay(2.0e-5);
        }
    }
   else
    delay(d1);
   obspower(tpwr);
   txphase(t1);
   decphase(zero);

status(B);		 /* Begin HMQC period with 1H(F1) and 13C(F2) frequency labeling  */
   rcvroff();
   delay(20.0e-6);
   rgpulse(pw,t1,0.0,0.0);                    /* 90 deg 1H pulse */
   delay(2.0e-6);
   zgradpulse(gzlvl1, gt1);
   delay(gstab);
   txphase(t3); decphase(t2);
   delay(taua + 4*pwC + 4.0e-6 - gt1 - 102.0e-6);
   decrgpulse(pwC,t2,0.0,0.0);
   delay(tau2);
   rgpulse(2*pw,t3,0.0,0.0);
   delay(tau2);
   decrgpulse(pwC,zero,0.0,0.0);
   delay(2.0e-6);
   zgradpulse(gzlvl1, gt1);
   delay(gstab);
   txphase(zero);
   delay(taua + tau1 - gt1 - gstab -2.0e-6);
   decrgpulse(pwC,zero,0.0,0.0);
   decrgpulse(2*pwC,one,2.0e-6,0.0);
   decrgpulse(pwC,zero,2.0e-6,0.0);
   delay(tau1);
 status(C);			/*	nOe and purge period	*/
   rgpulse(pw,zero,0.0,0.0);
   if (satmode[B] == 'y')	 /* Presaturation Period */
    {
       if (mfsat[A] == 'y')
        {obsunblank(); mfpresat_on(); delay(mix/2.0); mfpresat_off(); obsblank();}
       else
        {
         obspower(satpwr);	 /* Set transmitter power for H2O saturation  */
	 if (tof != satfrq) obsoffset(satfrq);
	 delay(2.0e-5);
         rgpulse(mix/2.0,zero,2.0e-6,2.0e-6); /* presaturation */
   	 obspower(tpwr);       /* Set transmitter power for hard 1H pulses */
	 if (tof != satfrq) obsoffset(tof);
    	 delay(2.0e-5);
        }
    }
   else
    delay(mix/2.0);
   zgradpulse(gzlvl3, gt3);
   if (satmode[B] == 'y')	 /* Presaturation Period */
    {
       if (mfsat[A] == 'y')
        {obsunblank(); mfpresat_on(); delay(mix/2.0 - gt3); mfpresat_off(); obsblank();}
       else
        {
         obspower(satpwr);	 /* Set transmitter power for H2O saturation  */
	 if (tof != satfrq) obsoffset(satfrq);
	 delay(2.0e-5);
         rgpulse(mix/2.0,zero,2.0e-6,2.0e-6); /* presaturation */
   	 obspower(tpwr);       /* Set transmitter power for hard 1H pulses */
	 if (tof != satfrq) obsoffset(tof);
    	 delay(2.0e-5);
        }
    }
   else
    delay(mix/2.0-gt3);
   obspower(tpwr);
   rgpulse(pw,t4,2.0e-6,0.0);
   delay(2.0e-6);
   zgradpulse(gzlvl5, gt5);
   delay(gstab);
   delay(taud - gt5 - gstab -2.0e-6);
   simpulse(2.0*pw,pwC,zero,zero,0.0,0.0);
   delay(2.0e-6);
   zgradpulse(gzlvl5, gt5);
   delay(gstab);
   delay(taue - gt5 - gstab -2.0e-6);
   decrgpulse(pwC,zero,0.0,0.0);
   delay(taud - taue - pwC);
   decpower(dpwr);  /* Set power for decoupling */
   rcvron();        /* Turn on receiver to warm up before acq */ 
status(D);		/* Begin acquisition	*/
   setreceiver(t5);
}
