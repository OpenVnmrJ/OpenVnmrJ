/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  T1ND.c 

    This pulse sequence will allow one to perform the following
    experiment:

    2D 2H T1 measurment of samples with deuterium incorporation
    with resolution achieved by 2D 13C,1H CT. (Methyl groups)
    Used for high resolution high sensitivity 13C spectrum.

    Uses three channels:
         1)  1H             - 4.73 ppm 
         2) 13C             - 19.0 ppm 
         3) 15N             - 119.0 ppm  

    Set dm = 'nnny', dmm = 'cccp' [13C decoupling during acquisition].
    Set dm2 = 'nnnn', dmm2 = 'cccp'.

    Must set phase = 1,2 for States-TPPI
    acquisition in t1 [13C].
    
    The flag f1180 should be set to 'y' if t1 is 
    to be started at halfdwell time. This will give -90, 180
    phasing in f1. If it is set to 'n' the phasing will
    be 0,0 and will still give a perfect baseline.

    Modified by L. E. Kay, Aug 15, 1995 from CH2D_T1_D_v6.c
    Added deuterium flipback pulses, RM Oct 11, 1995.

    Records carbon chemical shift during the second CT period

    Modified by L.E.Kay on Jan 19,96 to ensure a delay of 4us
     between power statements and pulses. 

    Modified by L.E.Kay on Jan 31, 1996 to eliminate phase difference
     between rectangular and reburp pulses.

    Modified by A. Lee on 5/20/97 into a psuedo-3D experiment
    
    Relaxation delays(z_array) are "hard-coded", here set for eight values. The
    variable ni2 should be used to specify the total number of mixing times.    
    Duplicate times are included for a consistency check

*/

#include <standard.h>
/* #define PI 3.14159265358979323846 */

static int  phi2[2] = {0,2},
            phi13[4] = {0,0,2,2},
            phi17[8] = {0,0,0,0,2,2,2,2},
            rec_nd[8] = {0,2,0,2,2,0,2,0};
           
static double d2_init=0.0,

	/* order of IzCz relaxation times */


 z_array[8] = {10.0e-3,
                      280.0e-3,
                       28.9e-3,
                      234.7e-3,
                      53.1e-3,
                      192.1e-3,
                       10.0e-3,
                      280.0e-3};

/* the SetAPAttr statement belows sets the third amplifier into
'cw' mode and the fourth amplifier into "pulsed" mode. For
deuterium decoupling on the fourth channel, one wants the
fourth amplifier in "pulsed" mode so as to not disturb the lock */

/* pre_fidsequence()
{
        extern int ap_interface;

        SetAPAttr(RF_Amp3_4, SET_MASK, (int) 0x58, SET_VALUE, NULL);
        delay(0.01);
        putcode(7);
        putcode(9);
        curfifocount = 0;
        return;
}
*/
            
void pulsesequence()
{
/* DECLARE VARIABLES */

 char       fsat[MAXSTR],
            fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            codecseq[MAXSTR],
            ddseq[MAXSTR], 
            shca180[MAXSTR];

 int         phase,  ni, 
             t1_counter,   /* used for states tppi in t1           */ 
             tau2;

 double      tau1,         /*  t1 delay */
             taua,         /*  ~ 1/4JCH =  1.7 ms */
             taub,         /* ~ 1/2JCH for AX spin systems */
             TC,           /* carbon constant time period 1/2JCC */
             pwc,          /* 90 c pulse at dhpwr            */
             tsatpwr,      /* low level 1H trans.power for presat  */
             dhpwr,        /* power level for high power 13C pulses on dec1 */
             sw1,          /* sweep width in f1                    */ 
             time_T1,      /* total relaxation time for T1 measurement */
             pwcodec,      /* pw90 for C' decoupling */
             dressed,      /* = 2 for seduce-1 decoupling */
             dpwrsed,
             pwd,          /* pulse width for D decoupling at dpwr3_D  */
             dresD,
             dpwr3_D,
             pwd1,         /* D flipback pulse at dpwr3 */
             lk_wait,      /* delay for lk receiver recovery  */

             d_ca180,
             pwca180,

             gt1,
             gt2,
             gt3,
             gt6,
             gt7,
             gt8,
             gstab=getval("gstab"),

             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl6,
             gzlvl7,
             gzlvl8;

/*  variables commented out are already defined by the system      */


/* LOAD VARIABLES */

  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("fscuba",fscuba);
  getstr("codecseq",codecseq);
  getstr("ddseq",ddseq);
  getstr("shca180",shca180);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  TC = getval("TC");
  pwc = getval("pwc");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  dhpwr = getval("dhpwr");
  dpwr = getval("dpwr");
  phase = (int) ( getval("phase") + 0.5);
  sw1 = getval("sw1");
  ni = getval("ni");
  pwcodec = getval("pwcodec");
  dressed = getval("dressed");
  dpwrsed = getval("dpwrsed");
  pwd1 = getval("pwd1");
  pwd = getval("pwd");
  dresD = getval("dresD");
  dpwr3_D = getval("dpwr3_D");
  lk_wait = getval("lk_wait");


  d_ca180 = getval("d_ca180");
  pwca180 = getval("pwca180");

  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");

  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");

/* LOAD PHASE TABLE */

  settable(t2,2,phi2);
  settable(t13,4,phi13);
  settable(t17,8,phi17);
  settable(t6,8,rec_nd);

/* CHECK VALIDITY OF PARAMETER RANGES */

    if( TC - 0.50*(ni-1)*1/(sw1) - WFG_STOP_DELAY 
             - gt6 - 102e-6 - POWER_DELAY 
             - PRG_START_DELAY - POWER_DELAY
             - pwd1 - 4.0e-6 - POWER_DELAY
             - PRG_START_DELAY - PRG_STOP_DELAY
             - 2.0e-6 - POWER_DELAY - 2.0e-6
             < 0.2e-6 ) 
    {
        printf(" ni is too big\n");
        psg_abort(1);
    }


    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' || dm2[D] == 'y'))
    {
        printf("incorrect dec2 decoupler flags!  ");
        psg_abort(1);
    }

    if( tsatpwr > 6 )
    {
        printf("TSATPWR too large !!!  ");
        psg_abort(1);
    }

    if( dpwr > 48 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > 49 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( dhpwr > 63 )
    {
        printf("don't fry the probe, DHPWR too large!  ");
        psg_abort(1);
    }

    if( pw > 200.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 

    if(gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 || 
        gt6 > 15e-3 || gt7 > 15e-3
        || gt8 > 15e-3)
    {
        printf("gradients on for too long. Must be < 15e-3 \n");
        psg_abort(1);
    }

   if(dpwr3_D > 53)
   {
       printf("D decoupling power is too high\n");
       psg_abort(1);
   }

   if(dpwr3 > 61)
   {
       printf("power dpwr3 is too high\n");
       psg_abort(1);
   }

   if(lk_wait > .015 )
   {
       printf("lk_wait delay may be too long\n");
       psg_abort(1);
   }

/* Calculation of IzCz relaxation delay */

    tau2 = (int) (d3+0.1);
    time_T1 = z_array[tau2];

/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2) {
      tsadd(t17,1,4);  
    }

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) );
        if(tau1 < 0.4e-6) tau1 = 0.4e-6;
    }
        tau1 = tau1/2.0;


/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t17,2,4);     
      tsadd(t6,2,4);    
    }


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   decoffset(dof);
   obspower(tsatpwr);     /* Set transmitter power for 1H presaturation */
   decpower(dhpwr);       /* Set Dec1 power for hard 13C pulses         */
   dec2power(dpwr2);      /* Set Dec2 power for 15N decoupling       */

/* Presaturation Period */

status(B);


   if (fsat[0] == 'y')
   {
        delay(2.0e-5);
        rgpulse(d1,zero,2.0e-6,2.0e-6); /* presat */
        obspower(tpwr);    /* Set transmitter power for hard 1H pulses */
        delay(2.0e-5);
        if(fscuba[0] == 'y')
        {
                delay(2.2e-2);
                rgpulse(pw,zero,2.0e-6,0.0);
                rgpulse(2*pw,one,2.0e-6,0.0);
                rgpulse(pw,zero,2.0e-6,0.0);
                delay(2.2e-2);
        }
   }
   else
   {
    delay(d1);
   }

   obspower(tpwr);          /* Set transmitter power for hard 1H pulses */
   txphase(zero);
   dec2phase(zero);
   decphase(zero);
   delay(1.0e-5);

/* Begin Pulses */

status(C);

/* Prepare for signs of gradients 0 1 0 1 0 1   */

   mod2(ct,v1);

   rcvroff();
   lk_hold();
   delay(20.0e-6);

/* first ensure that magnetization does infact start on H and not C */

   decrgpulse(pwc,zero,2.0e-6,2.0e-6);

   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(gstab);


/* this is the real start */

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(2.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(2.0e-6);

   delay(taua - gt2 - 4.0e-6);                 /* taua <= 1/4JCH */                          

   simpulse(2*pw,2*pwc,zero,zero,0.0,0.0);

   txphase(one); decphase(t17);

   delay(2.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(2.0e-6);

   delay(taua - gt2 - 4.0e-6); 

   rgpulse(pw,one,0.0,0.0);
   txphase(zero);

   delay(2.0e-6);
   zgradpulse(gzlvl3,gt3);
   delay(gstab);

   /* 2D decoupling on */  
   dec3phase(one);
   dec3power(dpwr3);
   dec3rgpulse(pwd1,one,4.0e-6,0.0);
   dec3phase(zero);
   dec3unblank();
   dec3power(dpwr3_D);   /* keep power down */ 
   dec3prgon(ddseq,pwd,dresD);
   dec3on();
   /* 2D decoupling on */

   decrgpulse(pwc,t17,4.0e-6,0.0);

   /* C' decoupling on */
   decpower(dpwrsed);
   decprgon(codecseq,pwcodec,dressed);
   decon();
   /* C' decoupling on */

   delay(taub - POWER_DELAY - PRG_START_DELAY);

   rgpulse(pw,zero,0.0,0.0);
   rgpulse(pw,t2,2.0e-6,0.0);

   delay(tau1);

   rgpulse(2.0*pw,t13,2.0e-6,0.0);

   delay(TC - taub - 2.0*pw - 2.0e-6
         - 2.0*pw - 2.0e-6 - PRG_STOP_DELAY - POWER_DELAY - pwd1 
         - 4.0e-6 - PRG_STOP_DELAY - POWER_DELAY - gt6 - 102e-6
         - WFG_START_DELAY);

   /* 2D decoupling off */  
   dec3off();
   dec3prgoff();
   dec3blank();
   dec3phase(three);
   dec3power(dpwr3);
   dec3rgpulse(pwd1,three,4.0e-6,0.0);
   /*  2D decoupler off */

   /* C' decoupling off */
   decoff();
   decprgoff();
   decpower(d_ca180);  /* set power for reburp  */
   /* C' decoupling off */

   initval(1.0,v3);
   decstepsize(353.5);
   dcplrphase(v3);

   ifzero(v1);

   delay(2.0e-6);
   zgradpulse(gzlvl6,gt6);
   delay(gstab);

   elsenz(v1);

   delay(2.0e-6);
   zgradpulse(-1.0*gzlvl6,gt6);
   delay(gstab);

   endif(v1);

   decshaped_pulse(shca180,pwca180,zero,0.0,0.0);
   dcplrphase(zero);

   ifzero(v1);

   delay(2.0e-6);
   zgradpulse(gzlvl6,gt6);
   delay(gstab);

   elsenz(v1);

   delay(2.0e-6);
   zgradpulse(-1.0*gzlvl6,gt6);
   delay(gstab);

   endif(v1);

   /* C' decoupling on */
   decpower(dpwrsed);
   decprgon(codecseq,pwcodec,dressed);
   decon();
   /* C' decoupling on */

   /* 2D decoupling on */  
   dec3phase(one);
   dec3power(dpwr3);
   dec3rgpulse(pwd1,one,4.0e-6,0.0);
   dec3phase(zero);
   dec3unblank();
   dec3power(dpwr3_D);   /* keep power down */ 
   dec3prgon(ddseq,pwd,dresD);
   dec3on();
   /* 2D decoupling on */

   delay(TC - tau1 - WFG_STOP_DELAY
         - gt6 - 102e-6
         - POWER_DELAY - PRG_START_DELAY
         - POWER_DELAY - 4.0e-6 - pwd1 - POWER_DELAY 
         - PRG_START_DELAY
         - PRG_STOP_DELAY - POWER_DELAY
         - 4.0e-6);

   /* C' decoupling off */
   decoff();
   decprgoff();
   decpower(dhpwr);
   /* C' decoupling off */

   decrgpulse(pwc,zero,4.0e-6,0.0);

   /* 2D decoupling off */  
   dec3off();
   dec3prgoff();
   dec3blank();
   dec3phase(three);
   dec3power(dpwr3);
   dec3rgpulse(pwd1,three,4.0e-6,0.0);
   /*  2D decoupler off */

  ifzero(v1);

   delay(2.0e-6);
   zgradpulse(gzlvl7,gt7);
   delay(gstab);

  elsenz(v1);

   delay(2.0e-6);
   zgradpulse(-1.0*gzlvl7,gt7);
   delay(gstab);

  endif(v1);

  delay(time_T1);


   delay(lk_wait);   /* delay for lk receiver recovery */

   rgpulse(pw,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl8,gt8);
   delay(2.0e-6);

  decphase(zero);

  delay(taua - gt8 - 4.0e-6);

  simpulse(2*pw,2*pwc,zero,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl8,gt8);
   delay(2.0e-6);

   delay(taua - 2*POWER_DELAY - gt8 - 4.0e-6);

   decpower(dpwr);  /* Set power for decoupling */
   dec2power(dpwr2);  /* Set power for decoupling */

   rgpulse(pw,zero,0.0,rof2);

  lk_sample();

/*   rcvron();  */          /* Turn on receiver to warm up before acq */ 

/* BEGIN ACQUISITION */

status(D);
   setreceiver(t6);

}

