/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  PR42_ghn_ca_coP.c

    This pulse sequence will allow one to perform the following
    experiment:

    4D projection-reconstruction non-trosy hncaco
	F1 	C-alpha
	F2 	CO
	F3 	N
	F-acq	HN

    This sequence uses the four channel configuration
         1)  1H             - carrier @ 4.7 ppm [H2O]
         2) 13C             - carrier @ 55 ppm (CA) and 176 ppm(Co)
         3) 15N             - carrier @ 118 ppm  
         4) 2H		    - carrier @ 4.5 ppm 

    Set dm = 'nnn', dmm = 'ccc' 
    Set dm2 = 'nny', dmm2 = 'ccp' 
    Set dm3 = 'nnn', dmm3 = 'ccc' 

    Flags
	satmode		'y' for presaturation of H2O
	fscuba		'y' for apply scuba pulse after presaturation of H2O

	Standard Settings
   satmode='n',fscuba='n',mess_flg='n'

   Use ampmode statement = 'dddp'
   Note the final coherence transfer gradients have been split
   about the last 180 (H)

   Calibration of carbon pulses
	
    To set correct phasing in the CO dimension: Set angle_CO=0 and
    ni=1.  Array sphase until a null is achieved. The correct value for 
    sphase is then this value plus 45.

    Ref:  Daiwen Yang and Lewis E. Kay, J.Am.Chem.Soc., 121, 2571(1999)
          Diawen Yang and Lewis E. Kay, J.Biomol.NMR, 13, 3(1999)

Written by Daiwen Yang on Sep. 9, 1998

Modified on 11/20/03 by R. Venters to a projection reconstruction sequence.
Also added Cb decoupling during Ca-CO transfers and Ca evolution.
Modified on 10/07/04 by R. Venters to add tilt proper tilt angles.
Modified on 03/31/06 by R. Venters to BioPack version

Ref: (4,2)D Projection-Reconstruction Experiemnts for Protein Backbone
Assignment:  Application to Human Carbonic Anhydrase II and Calbindin
D28K.  Venters, R.A., Coggins, B.E., Kojetin, D., Cavanagh, J. and
Zhou, P. JACS 127(24), 8785-8795 (2005)

To obtain reconstruction software package, please visit
http://zhoulab.biochem.duke.edu/software/pr-calc

*/


#include <standard.h>
#include "bionmr.h"
#include "Pbox_bio.h"

static int  phi1[1]  = {0},          /* was 1 */
            phi2[4]  = {0,0,2,2},    /* was 0022 */
	    phi3[1]  = {0},         /* was 0 */
	    phi4[1]  = {0},          /* was 0 */
	    phi5[4]  = {0,1,2,3},   /* was 0123 */
	    phi7[4]  = {0,0,2,2},   /* was 0022  */
            rec[4]  = {2,0,0,2};     /* was 0220 */


void pulsesequence()

{
/* DECLARE VARIABLES */

 char       satmode[MAXSTR],
	    fscuba[MAXSTR],
            cbdecseq[MAXSTR];

 int        icosel,
            ni = getval("ni"),
            t1_counter;   /* used for states tppi in t1           */

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             tau3,         /*  t3 delay */
             taua,         /*  ~ 1/4JNH =  2.25 ms */
             taub,         /*  ~ 1/4JNH =  2.25 ms */
             tauc,         /*  ~ 1/4JNCa =  ~13 ms */
             taud,         /*  ~ 1/4JCaC' =  3~4.5 ms ms */
             d2_init=0.0,                        /* used for states tppi in t1 */
             bigTN,        /* nitrogen T period */
             bigTC,        /* carbon T period */
             BigT1,        /* delay about 200 us */
             satpwr,      /* low level 1H trans.power for presat  */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             at,
             sphase,
             cbpwr,        /* power level for selective CB decoupling */
             cbdmf,        /* pulse width for selective CB decoupling */
             cbres,        /* decoupling resolution of CB decoupling */

             pwS1,         /* length of  90 on Ca */
             pwS2,         /* length of  90 on CO */
             pwS3,         /* length of 180 on Ca  */
             pwS4,         /* length of 180 on CO  */

             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gt8,
             gt9,
             gt10,
             gt11,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7,
             gzlvl8,
             gzlvl9,
             gzlvl10,
             gzlvl11,

             compH = getval("compH"),         /* adjustment for amplifier compression */
             pwHs = getval ("pwHs"),         /* H1 90 degree pulse at tpwrs */
             tpwrs,                          /* power for pwHs ("H2osinc") pulse */
             waltzB1 = getval("waltzB1"),

             pwClvl = getval("pwClvl"),                 /* coarse power for C13 pulse */
             pwC = getval("pwC"),             /* C13 90 degree pulse length at pwClvl */

             pwNlvl = getval("pwNlvl"),                       /* power for N15 pulses */
             pwN = getval("pwN"),             /* N15 90 degree pulse length at pwNlvl */

  swCa = getval("swCa"),
  swCO = getval("swCO"),
  swN  = getval("swN"),
  swTilt,                     /* This is the sweep width of the tilt vector */

  cos_N, cos_CO, cos_Ca,
  angle_N, angle_CO, angle_Ca;
  angle_N=0.0;

/* LOAD VARIABLES */

  getstr("satmode",satmode);
  getstr("fscuba",fscuba);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  tauc   = getval("tauc"); 
  taud   = getval("taud"); 
  bigTN = getval("bigTN");
  bigTC = getval("bigTC");
  BigT1 = getval("BigT1");
  tpwr = getval("tpwr");
  satpwr = getval("satpwr");
  dpwr = getval("dpwr");
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  at = getval("at");
  sphase = getval("sphase");

  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  gt9 = getval("gt9");
  gt10 = getval("gt10");
  gt11 = getval("gt11");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");
  gzlvl9 = getval("gzlvl9");
  gzlvl10 = getval("gzlvl10");
  gzlvl11 = getval("gzlvl11");

/* Load variable */
        cbpwr = getval("cbpwr");
        cbdmf = getval("cbdmf");
        cbres = getval("cbres");
        tau1 = 0;
        tau2 = 0;
        tau3 = 0;
        cos_N = 0;
        cos_CO = 0;
        cos_Ca = 0;
        kappa = 5.4e-3;

    getstr("cbdecseq", cbdecseq);

/* LOAD PHASE TABLE */

  settable(t1,1,phi1);
  settable(t2,4,phi2);
  settable(t3,1,phi3);
  settable(t4,1,phi4);
  settable(t5,4,phi5);
  settable(t7,4,phi7);
  settable(t6,4,rec);

   /* get calculated pulse lengths of shaped C13 pulses */
        pwS1 = c13pulsepw("ca", "co", "square", 90.0);
        pwS2 = c13pulsepw("co", "ca", "sinc", 90.0);
        pwS3 = c13pulsepw("ca","co","square",180.0);
        pwS4 = c13pulsepw("co","ca","sinc",180.0);

   tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
   tpwrs = (int) (tpwrs);                          /*power than a square pulse */
   widthHd = 2.681*waltzB1/sfrq;  /* bandwidth of H1 WALTZ16 decoupling */
   pwHd = h1dec90pw("WALTZ16", widthHd, 0.0);     /* H1 90 length for WALTZ16 */


/* CHECK VALIDITY OF PARAMETER RANGES */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'n'))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nny' ");
        psg_abort(1);
    }

    if( satpwr > 6 )
    {
        printf("SATPWR too large !!!  ");
        psg_abort(1);
    }

    if( dpwr > 46 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > 47 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( pw > 200.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 
    if( pwN > 200.0e-6 )
    {
        printf("dont fry the probe, pwN too high ! ");
        psg_abort(1);
    } 

    if( gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 
	|| gt4 > 15e-3 || gt5 > 15e-3 || gt6 > 15e-3 
	|| gt7 > 15e-3 || gt8 > 15e-3 || gt9 > 15e-3 || gt10 > 15.0e-3 
	|| gt11>15.0e-3)  
    {
       printf("gti values must be < 15e-3\n");
       psg_abort(1);
    } 

    if( dpwr3 > 56) {
       printf("dpwr3 too high\n");
       psg_abort(1);
    }


/* PHASES AND INCREMENTED TIMES */


   /* Set up angles and phases */

   angle_CO=getval("angle_CO");  cos_CO=cos(PI*angle_CO/180.0);
   angle_Ca=getval("angle_Ca");  cos_Ca=cos(PI*angle_Ca/180.0);

   if ( (angle_CO < 0) || (angle_CO > 90) )
   {  printf ("angle_CO must be between 0 and 90 degree.\n"); psg_abort(1); }

   if ( (angle_Ca < 0) || (angle_Ca > 90) )
   {  printf ("angle_Ca must be between 0 and 90 degree.\n"); psg_abort(1); }

   if ( 1.0 < (cos_CO*cos_CO + cos_Ca*cos_Ca) )
   {
       printf ("Impossible angles.\n"); psg_abort(1);
   }
   else
   {
           cos_N=sqrt(1.0- (cos_CO*cos_CO + cos_Ca*cos_Ca));
           angle_N = 180.0*acos(cos_N)/PI;
   }

   swTilt=swCO*cos_CO + swCa*cos_Ca + swN*cos_N;

   if (ix ==1)
   {
      printf("\n\nn\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
      printf ("Maximum Sweep Width: \t\t %f Hz\n", swTilt);
      printf ("Angle_CO:\t%6.2f\n", angle_CO);
      printf ("Angle_Ca:\t%6.2f\n", angle_Ca);
      printf ("Angle_N :\t%6.2f\n", angle_N );
   }

/* Set up hyper complex */

   /* sw1 is used as symbolic index */
   if ( sw1 < 1000 ) { printf ("Please set sw1 to some value larger than 1000.\n"); psg_abort(1); }

   if (ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if (t1_counter % 2)  { tsadd(t1,2,4); tsadd(t6,2,4); }

   if (phase1 == 1)  { ;}                                                  /* CC */
   else if (phase1 == 2)  { tsadd(t7,1,4);}                                /* SC */
   else if (phase1 == 3)  { tsadd(t4,3,4); }                               /* CS */
   else if (phase1 == 4)  { tsadd(t7,1,4); tsadd(t4,3,4); }                /* SS */
   else { printf ("phase1 can only be 1,2,3,4. \n"); psg_abort(1); }

   if (phase2 == 2)  { tsadd(t3,2,4); icosel = +1; }                      /* N  */
            else                       icosel = -1;

   tau1 = 1.0*t1_counter*cos_CO/swTilt;
   tau2 = 1.0*t1_counter*cos_Ca/swTilt;
   tau3 = 1.0*t1_counter*cos_N/swTilt;

   tau1 = tau1/2.0;  tau2 = tau2/2.0;  tau3 = tau3/2.0;


/* CHECK VALIDITY OF PARAMETER RANGES */

    if (bigTN - 0.5*ni*(cos_N/swTilt) + pwS3 < 0.2e-6) 
       { printf(" ni is too big. Make ni equal to %d or less.\n",
         ((int)((bigTN + pwS3)*2.0*swTilt/cos_N)));              psg_abort(1);}

    if (bigTC - 0.5*ni*(cos_Ca/swTilt) - pwS4
                 - pwS3/2 - WFG3_START_DELAY - WFG3_STOP_DELAY
                 -3*POWER_DELAY - PRG_START_DELAY - PRG_STOP_DELAY -
                  4.0e-6 < 0.2e-6)
       {
         printf(" ni is too big for Ca. Make ni equal to %d or less.\n",
            (int) ((bigTC - pwS4 - pwS3/2 - WFG3_START_DELAY - 
                  WFG3_STOP_DELAY - 3*POWER_DELAY - PRG_START_DELAY -
                  PRG_STOP_DELAY -4.0e-6 )/(0.5*cos_Ca/swTilt)) );
         psg_abort(1);
       }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(satpwr);     /* Set transmitter power for 1H presaturation */
   obspwrf(4095.0);
   decpower(pwClvl);       /* Set Dec1 power for hard 13C pulses         */
   decpwrf(4095.0);
   dec2power(pwNlvl);      /* Set Dec2 power for hard 15N pulses         */
   dec2pwrf(4095.0);
   set_c13offset("ca");

/* Presaturation Period */

   if (satmode[0] == 'y')
   {
	delay(2.0e-5);
    	rgpulse(d1,zero,2.0e-6,2.0e-6);
   	obspower(tpwr);      /* Set transmitter power for hard 1H pulses */
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
   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   txphase(three);
   dec2phase(zero);
   delay(1.0e-5);

/* Begin Pulses */

status(B);

   rcvroff();
   shiftedpulse("sinc", pwHs, 90.0, 0.0, three, 2.0e-6, 2.0e-6);
   txphase(zero);

/*   xxxxxxxxxxxxxxxxxxxxxx    1HN to 15N TRANSFER   xxxxxxxxxxxxxxxxxx    */

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(0.2e-6);
   zgradpulse(gzlvl1, gt1);
   delay(2.0e-6);

   delay(taua - gt1 - 2.2e-6);   /* taua <= 1/4JNH */ 

   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

   txphase(one); dec2phase(zero); decphase(zero); 

   delay(taua - gt1 - 200.2e-6 - 2.0e-6); 

   delay(0.2e-6);
   zgradpulse(gzlvl1, gt1);
   delay(200.0e-6);

/*   xxxxxxxxxxxxxxxxxxxxxx    15N to 13CA TRANSFER   xxxxxxxxxxxxxxxxxx    */

       rgpulse(pw,one,2.0e-6,0.0);

       delay(0.2e-6);
       zgradpulse(gzlvl2, gt2);
       delay(200.0e-6);

       dec2rgpulse(pwN,zero,0.0,0.0);

       delay(kappa - POWER_DELAY - PWRF_DELAY - pwHd - 4.0e-6 - PRG_START_DELAY);
                            /* delays for h1waltzon subtracted */

       h1waltzon("WALTZ16", widthHd, 0.0);
       decphase(zero);
       dec2phase(zero);

       delay(tauc - kappa - WFG3_START_DELAY );

       dec2rgpulse(2*pwN,zero,0.0,0.0);
       c13pulse("ca", "co", "square", 180.0, zero, 0.0, 0.0);
       dec2phase(zero); 

       delay(tauc - pwS3);

       dec2rgpulse(pwN,zero,0.0,0.0);

   h1waltzoff("WALTZ16", widthHd, 0.0);
   decphase(zero);

   delay(0.2e-6);
   zgradpulse(gzlvl3, gt3);
   delay(200.0e-6);

/* xxxxxxxxxxxxxxxxxxxxx 13CA to 13CO TRANSFER xxxxxxxxxxxxxxxxxxxxxxx  */

      c13pulse("ca", "co", "square", 90.0, zero, 0.0, 0.0);

      decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

                delay(taud - 2*POWER_DELAY -  PRG_START_DELAY - PRG_STOP_DELAY
                       - 0.5*10.933*pwC); 
      decoff();
      decprgoff();
      decpower(pwClvl);

/* CHECK if this freq jump is needed */
      set_c13offset("co");   /* change Dec1 carrier to Co  */

        decrgpulse(pwC*158.0/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC*171.2/90.0, two, 0.0, 0.0);
        decrgpulse(pwC*342.8/90.0, zero, 0.0, 0.0);      /* Shaka 6 composite */
        decrgpulse(pwC*145.5/90.0, two, 0.0, 0.0);
        decrgpulse(pwC*81.2/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC*85.3/90.0, two, 0.0, 0.0);

      set_c13offset("ca");   /* change Dec1 carrier to Co  */

      decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

                delay(taud - 2*POWER_DELAY
                - PRG_STOP_DELAY - PRG_START_DELAY - 0.5*10.933*pwC);

      decoff();
      decprgoff();
      decpower(pwClvl);

      c13pulse("ca", "co", "square", 90.0, one, 0.0, 0.0);

      set_c13offset("co");   /* change Dec1 carrier to Co  */

                delay(2.0e-7);
                zgradpulse(gzlvl4, gt4);
                delay(100.0e-6);


/*   xxxxxxxxxxxxxxxx 13CO CHEMICAL SHIFT EVOLUTION xxxxxxxxxxxxxx */
 
   c13pulse("co", "ca", "sinc", 90.0, t7, 0.0, 0.0);

   if ((ni>1.0) && (tau1>0.0))
   {
    if (tau1-2.0*pwS2/PI-pwN-WFG3_START_DELAY-POWER_DELAY-2.0e-6 > 0.0)
    {
   delay(tau1-2.0*pwS2/PI-pwN-WFG3_START_DELAY-POWER_DELAY-2.0e-6);

   sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 2.0e-6);

                initval(1.0,v3);
                decstepsize(sphase);
                dcplrphase(v3);     

   delay(tau1-2.0*pwS2/PI-SAPS_DELAY-pwN-WFG3_STOP_DELAY-POWER_DELAY-2.0e-6);
   }
   else
   {
     delay(2.0*tau1);
     delay(10.0e-6);
     c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
     delay (10.0e-6);
   }
  }
   else
  { 
     c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
  }

   c13pulse("co", "ca", "sinc", 90.0, zero, 4.0e-6, 0.0);
                dcplrphase(zero);
 
   set_c13offset("ca");  /* set carrier to Ca */
 
                decphase(t4);
                delay(2.0e-7);
                zgradpulse(gzlvl9, gt9);
                delay(100.0e-6);

/* xxxxxxxxxxxxxx 13CO to 13CA TRANSFER and 13CA EVOLUTION xxxxxxxxxxxxxxxx  */

      c13pulse("ca", "co", "square", 90.0, t4, 2.0e-6, 0.0);

      decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

      delay(bigTC - tau2 - 3*POWER_DELAY - 4.0e-6 - WFG3_START_DELAY
            - pwS4 - WFG3_STOP_DELAY - PRG_START_DELAY - PRG_STOP_DELAY  
            - pwS3/2 - 4.0e-6);

      decoff();
      decprgoff();

      decpower(pwClvl);
      c13pulse("co", "ca", "sinc", 180.0, zero, 4.0e-6, 0.0);
      decphase(t5);
      c13pulse("ca", "co", "square", 180.0, t5, 4.0e-6, 0.0);

      decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

      delay(bigTC -3*POWER_DELAY - 6.0e-6 -pwS3/2 - 2*pwN
      - WFG_START_DELAY- pwS4- WFG_STOP_DELAY - PRG_START_DELAY
      - PRG_STOP_DELAY - pwS1/2);

      dec2rgpulse(2*pwN,zero,0.0,0.0);
      delay(tau2);

      decoff();
      decprgoff();

      decpower(pwClvl);
      c13pulse("co", "ca", "sinc", 180.0, zero, 4.0e-6, 0.0);

      decphase(one);
      c13pulse("ca", "co", "square", 90.0, one, 2.0e-6, 0.0);

   txphase(zero);

                delay(2.0e-7);
                zgradpulse(gzlvl11, gt11);
                delay(100.0e-6);

/* Constant 15N period  */
   h1waltzon("WALTZ16", widthHd, 0.0);
   dec2rgpulse(pwN,t1,2.0e-6,0.0);

   dec2phase(t2);

   delay(bigTN - tau3 + pwS3);

   dec2rgpulse(2*pwN,t2,0.0,0.0);
   c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);

   dec2phase(t3);
   txphase(zero);

   if (tau3 > (kappa + PRG_STOP_DELAY + pwHd + 2.0e-6))
   {
       delay(bigTN - pwS4 - WFG_START_DELAY - 2.0*POWER_DELAY
                                - 2.0*PWRF_DELAY - 2.0e-6);
       c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
       delay(tau3 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6 - POWER_DELAY
                                         - PWRF_DELAY);
       h1waltzoff("WALTZ16", widthHd, 0.0);

       delay(kappa - gt5 - 2.0*GRADIENT_DELAY - 1.0e-4);
       zgradpulse(gzlvl5, gt5);
       delay(1.0e-4);
   }
   else if (tau3 > (kappa - pwS4 - WFG_START_DELAY - 2.0*POWER_DELAY
                                - 2.0*PWRF_DELAY - 2.0e-6))
   {
      delay(bigTN + tau3 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
                                 - POWER_DELAY - PWRF_DELAY);
      h1waltzoff("WALTZ16", widthHd, 0.0);
      c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);

      delay(kappa - pwS4 - WFG_START_DELAY - 2.0*POWER_DELAY
            - 2.0*PWRF_DELAY - 2.0e-6 - gt5 - 2.0*GRADIENT_DELAY - 1.0e-4);
      zgradpulse(gzlvl5, gt5);
      delay(1.0e-4);
   }
   else if (tau3 > gt5 + 2.0*GRADIENT_DELAY + 1.0e-4)
   {
      delay(bigTN + tau3 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
                                 - POWER_DELAY - PWRF_DELAY);
      h1waltzoff("WALTZ16", widthHd, 0.0);
      delay(kappa - tau3 - pwS4 - WFG_START_DELAY - 2.0*POWER_DELAY
                                   - 2.0*PWRF_DELAY - 2.0e-6);
      c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
      delay(tau3 - gt5 - 2.0*GRADIENT_DELAY - 1.0e-4);
      zgradpulse(gzlvl5, gt5);
      delay(1.0e-4);
   }
   else
   {
      delay(bigTN + tau3 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
                                 - POWER_DELAY - PWRF_DELAY);
      h1waltzoff("WALTZ16", widthHd, 0.0);
      delay(kappa - tau3 - pwS4 - WFG_START_DELAY - 2.0*POWER_DELAY
            - 2.0*PWRF_DELAY - 2.0e-6 - gt5 - 2.0*GRADIENT_DELAY - 1.0e-4);
      zgradpulse(gzlvl5, gt5);        /* 2.0*GRADIENT_DELAY */
      delay(1.0e-4);
      c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
      delay(tau3);
   }

   sim3pulse(pw,0.0,pwN,zero,zero,t3,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl6, gt6);
   delay(2.0e-6);
 
   dec2phase(zero);
   delay(taub - gt6 - 2.2e-6);
 
   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);
 
   delay(0.2e-6);
   zgradpulse(gzlvl6, gt6);
   delay(200.0e-6);
   
   delay(taub - gt6 - 200.2e-6);
   txphase(one);
   dec2phase(one);
 
   sim3pulse(pw,0.0,pwN,one,zero,one,0.0,0.0);
 
   delay(0.2e-6);
   zgradpulse(gzlvl7, gt7);
   delay(2.0e-6);
 
   txphase(zero);
   dec2phase(zero);
 
   delay(taub - gt7 - 2.2e-6);
 
   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);
 
   delay(0.2e-6);
   zgradpulse(gzlvl7, gt7);
   delay(200.0e-6);
 
   delay(taub - gt7 - 200.2e-6);

   rgpulse(pw, zero, 0.0, 0.0);
   delay(gt8 + 1.0e-4 + 50.2e-6 - 0.3*pw + 2.0*GRADIENT_DELAY
                                   + POWER_DELAY);
   rgpulse(2*pw,zero,0.0,0.0);
   dec2power(dpwr2);
   decpower(dpwr);
   zgradpulse(icosel*gzlvl8, gt8);
   delay(50.2e-6);

    
/*   rcvron();  */          /* Turn on receiver to warm up before acq */ 

/* BEGIN ACQUISITION */

status(C);
         setreceiver(t6);

}
