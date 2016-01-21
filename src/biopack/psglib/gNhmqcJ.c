/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNhmqcJ.c

       This pulse sequence will allow one to perform the following experiment:

       2D hmqcj for the measurement of NH-HA coupling constants.
                 F1        15N
                 F2(acq)   NH
      
       Uses channels 1 and 3 of the standard 3 channel configuration
       1) 1H        - carrier (tof) @ 4.7ppm [H2O]
       2)
       3) 15N       - carrier (dof2) @ 120ppm [ centre of amide N]

       Set dm = 'nnn'  dmm = 'ccc'
           dm2 = 'nny' [15N decoupling during acquisition]

       Must set phase=1,2 for States-TPPI acquisition in t1 [15N]

       Flags
            fsat      'y' for presaturation of H2O
            fscuba    'y' to apply scuba pulse after presaturation of H2O
            mess_flg  'y' for 1H (H2O) purging before the relaxation delay
            f1180     'y' for 180deg linear phase correction in F1
            shp_flg   'y' for selective excitation and dephasing H2O mag.

       standard settings:
               fsat='n', fscuba='n', mes_flg='n', f1180='y', shp_flg='y'
   
       Written by L. E. Kay  05-12-92
       Modified by L. E. Kay 07-16-93 to encorporate gradients and boban

       Modified by L. E. Kay Sept 12/93 to move the rcvroff
       Modified by L.E.K on Feb, 22/94 to put in water selective pulses
       Modified by L.E.K on July 29 to eliminate the need for presat
       Compatible with UnityInova

       Modified for BioPack, GG June2004

 REF: L. E. Kay and A. Bax     J. Magn. Reson. 86, 110-126 (1990).

*/

#include <standard.h>

static double d2_init = 0.0;

static int phi1[1] = {0},
           phi2[2] = {0,2},
           phi3[8] = {0,0,1,1,2,2,3,3}, 
           phi4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           rec[16] = {0,2,2,0,0,2,2,0,
                      2,0,0,2,2,0,0,2};


pulsesequence()
{
/* DECLARE VARIABLES */

 char        fscuba[MAXSTR],f1180[MAXSTR],fsat[MAXSTR],mess_flg[MAXSTR],
             shp_flg[MAXSTR],shp_sl[MAXSTR];


 int	     phase,
             t1_counter;

 double      hscuba,                /* length of 1/2 scuba delay */
             tauxh,                 /* 1 / 4J(XH)                */
             pwN,                  /* PW90 for X-nuc on channel 3  */
             tsatpwr,               /* low power level for presat */
             pwNlvl,                /* power level for X hard pulses */
             jxh,                   /* coupling for XH           */
	     tau1,	      	    /* t1/2 */
	     sw1,
             tpwrmess,    /* power level for Messerlie purge */
             dly_pg1,     /* duration of first part of purge */
             tpwrsl,
             pw_sl,
             d1_wait,
 
             gzlvl1,
             gt1,
             gzlvl2,
             gt2;


/* LOAD VARIABLES */

  jxh = getval("jxh");
  pwN = getval("pwN");
  tsatpwr = getval("tsatpwr");
  pwNlvl = getval("pwNlvl"); 
  hscuba = getval("hscuba");
  phase = (int) (getval("phase") + 0.5);
  sw1 = getval("sw1");
  tpwrmess = getval("tpwrmess");
  dly_pg1 = getval("dly_pg1");
  d1_wait = getval("d1_wait");
  tpwrsl = getval("tpwrsl");
  pw_sl = getval("pw_sl");

  gzlvl1 = getval("gzlvl1");
  gt1 = getval("gt1");
  gzlvl2 = getval("gzlvl2");
  gt2 = getval("gt2");
 

  getstr("fscuba",fscuba); 
  getstr("fsat",fsat); 
  getstr("f1180",f1180); 
  getstr("shp_flg",shp_flg);
  getstr("mess_flg",mess_flg);
  getstr("shp_sl",shp_sl);

/* check validity of parameter range */

    if((dm2[A] == 'y' || dm2[B] == 'y'))
	{
	printf("incorrect Dec2 decoupler flags!  ");
	psg_abort(1);
    } 

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
	{
	printf("incorrect Dec decoupler flags!  ");
	psg_abort(1);
    } 

    if( tsatpwr > 8 )
    {
	printf("tsatpwr too large !!!  ");
	psg_abort(1);
    }

    if( dpwr > 10 )
    {
	printf("don't fry the probe, dpwr too large!  ");
	psg_abort(1);
    }

    if( dpwr2 > 50 )
    {
	printf("don't fry the probe, dpwr2 too large!  ");
	psg_abort(1);
    }

    if( pwNlvl > 63 )
    {
	printf("don't fry the probe, pwNlvl too large!  ");
	psg_abort(1);
    }

    if(tpwrmess > 55)
    {
        printf("dont fry the probe, tpwrmess too large < 55");
        psg_abort(1);
    }

    if(dly_pg1 > 0.010)
    {
        printf("dont fry the probe, dly_pg1 is too long < 10 ms");
        psg_abort(1);
    }

    if(gt1 > 15.0e-3 || gt2 > 15.0e-3) 
    {
        printf("gti is too long\n");
        psg_abort(1);
    }

    if(tpwrsl > 25) 
    {
        printf("tpwrsl is too high\n");
        psg_abort(1);
    }
   
/* LOAD VARIABLES */

  settable(t1, 1, phi1);
  settable(t2, 2, phi2);
  settable(t3, 8, phi3);
  settable(t4, 16, phi4);
  settable(t5, 16, rec);

/* INITIALIZE VARIABLES */

  tauxh = ((jxh != 0.0) ? 1/(2*(jxh)) : 3.57e-3);

/* Phase incrementation for hypercomplex data */

   if ( phase == 2 )     /* Hypercomplex in t1 */
   {
        tsadd(t2, 1, 4);
   } 

/* calculate modifications to phases based on current t1 values
   to achieve States-TPPI acquisition */

   if(ix==1)
     d2_init = d2;
     t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
     if(t1_counter %2) {
       tsadd(t2,2,4);
       tsadd(t5,2,4);
     }

/* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */

   tau1 = d2;
   if(f1180[A] == 'y')  tau1 += ( 1.0/(2.0*sw1) - 2.0*pw - 4.0/PI*pwN );
   else tau1 = tau1 - 2.0*pw - 4.0/PI*pwN;
   if(tau1 < 0.2e-6) tau1 = 0.4e-6;
   tau1 = tau1/2.0;
   

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);            /* Set power for presaturation  */
   decpower(dpwr);               /* Set decoupler1 power to dpwr */
   dec2power(pwNlvl);            /* Set decoupler2 power to pwNlvl */

/* Presaturation Period */


   if (mess_flg[A] == 'y') {

      obspower(tpwrmess);
      rgpulse(dly_pg1,zero,2.0e-6,2.0e-6);
      rgpulse(dly_pg1/1.62,one,2.0e-6,2.0e-6);

      obspower(tsatpwr);
   }

 if(fsat[0] == 'y')
{
  txphase(zero);
  
  if(d1_wait == 0.0)
    rgpulse(d1,zero,2.0e-6,2.0e-6);     /* presaturation */ 
  else
    {
     delay(d1_wait);
     rgpulse(d1,zero,2.0e-6,2.0e-6);
    }
  obspower(tpwr);                /* Set power for hard pulses  */

    if (fscuba[0] == 'y')            /* Scuba pulse sequence */
    {  
      hsdelay(hscuba);

      rgpulse(pw,zero,1.0e-6,0.0);	/* 90x180y90x */
      rgpulse(2*pw,one,1.0e-6,0.0);
      rgpulse(pw,zero,1.0e-6,0.0);
 
      txphase(zero);
      delay(hscuba);        
    }
}

else  {
 obspower(tpwr);                /* Set power for hard pulses  */
 delay(d1);
}


status(B);

  obspower(tpwr);
  rcvroff();
  delay(20.0e-6);

  if (shp_flg[A] == 'y') {

    /* shaped pulse */
    obspower(tpwrsl);
    shaped_pulse(shp_sl,pw_sl,two,2.0e-6,0.0);
    delay(2.0e-6);
    obspower(tpwr); 
    /* shaped pulse */

     delay(2.0e-6);
     zgradpulse(gzlvl1,gt1);
     delay(1000.0e-6);

  }

  rgpulse(pw,t1,1.0e-6,0.0);           /* proton excit. pulse, */

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shp_sl,pw_sl,two,2.0e-6,0.0);
  delay(2.0e-6);
  obspower(tpwr); 
  /* shaped pulse */


  txphase(t3);
  dec2phase(t2);

  delay(2.0e-6);
  zgradpulse(gzlvl2,gt2);
  delay(2.0e-6);

  delay(tauxh - gt2 - 4.0e-6 - pw_sl);              /* delay=1/2J(XH)   */

  dec2rgpulse(pwN,t2,0.0,0.0);

  dec2phase(t4);

  delay(tau1);                  /* delay=t2/2      */ 
  
  rgpulse(2*pw,t3,0.0,0.0); 

  delay(tau1);                  /* delay=t2/2      */

  dec2rgpulse(pwN,t4,0.0,0.0);

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shp_sl,pw_sl,one,2.0e-6,0.0);
  delay(2.0e-6);
  obspower(tpwr); 
  /* shaped pulse */

  dec2power(dpwr2);  /* lower decoupler power for decoupling on decouper channel 2 */

  delay(2.0e-6);
  zgradpulse(gzlvl2,gt2);

  delay(tauxh - POWER_DELAY - gt2 - 4.0e-6 - 2.0e-6 - pw_sl);  


  rgpulse(pw,one,2.0e-6,0.0); 

/* acquire data */

status(C);
  setreceiver(t5);
}
