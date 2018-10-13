/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  PR42_sim_ghn_co_caP_TROSY.c

    This pulse sequence will allow one to perform the following experiment:

    4D projection-reconstruction hncoca (co and ca are in the same residue)with trosy
                       F1      CO
                       F2      CA
                       F3      15N + JNH/2
                       F4(acq) 1H (NH) - JNH/2

    This sequence uses the standard three channel configuration
         1)  1H             - carrier (tof) @ 4.7 ppm [H2O]
         2) 13C             - carrier (dof) @ 176 ppm [CO] or CA 55ppm
         3) 15N             - carrier (dof2)@ 119 ppm [centre of amide 15N]  
    
    Set dm = 'nnn', dmm = 'ccc' 
    Set dm2 = 'nnn', dmm2 = 'ccc'

    Flags
        satmode         'y' for presaturation of H2O
        fscuba          'y' for apply scuba pulse after presaturation of H2O
	fco180		'y' for checking N/NH 2D 
	fca180		'y' for checking N/NH 2D 
        sel_flg         'y' for active suppression of the anti-TROSY
        sel_flg         'n' for relaxation suppression of the anti-TROSY

	Standard Settings
        satmode='n',fscuba='n'

    Set the carbon carrier on the C' and use the waveform to pulse the
        c-alpha

    Written By Daiwen Yang on July 12, 1999.

    Modified by L. E. Kay on Aug. 22, 99 to include a sel_flg
    Modified by L.E.Kay on Aug. 9, 2001 to separate N and adiabatic pulses; especially
     for 800 MHz application, where the power to the probe is considerable.

    Modified on 12/03/03 by R. Venters to a projection reconstruction sequence.
    Also added Cb decoupling during Ca-CO transfers and Ca evolution.
    Modified on 10/07/04 by R. Venters to collect proper tilts
    Modified on 04/05/06 by R. Venters for BioPack


    Ref: (4,2)D Projection-Reconstruction Experiemnts for Protein Backbone
    Assignment:  Application to Human Carbonic Anhydrase II and Calbindin
    D28K.  Venters, R.A., Coggins, B.E., Kojetin, D., Cavanagh, J. and
    Zhou, P. JACS 127(24), 8785-8795 (2005)

    To obtain reconstruction software package, please visit
    http://zhoulab.biochem.duke.edu/software/pr-calc
*/

#include <standard.h>
#include "bionmr.h"
#include "Pbox_bio.h"  /* so Pbox-generated pulse can be used */

/* set up text strings to be used later in Pbox statement (more compact) */
#define CHIRP180   "chirp 200p/0.5m -60p"
#define CHIRP180ps "-stepsize 0.5 -attn i"

/* define name of Pbox-generated shape */
static shape chirp180;

static int  phi1[2]  = {0,2},
            phi2[2]  = {0,2},
            phi3[1]  = {0},
            phi4[8]  = {0,0,0,0,2,2,2,2},
            phi5[4]  = {0,0,2,2},
            rec[8]   = {0,0,2,2,2,2,0,0};

            
pulsesequence()
{
/* DECLARE VARIABLES */

 char       satmode[MAXSTR],
	    fscuba[MAXSTR],
            cbdecseq[MAXSTR],
            chirp_shp[MAXSTR],  /* name of variable containing name of Pbox shape */
            fco180[MAXSTR],    /* Flag for checking sequence              */
            fca180[MAXSTR],    /* Flag for checking sequence              */
            sel_flg[MAXSTR];

 int         icosel,
             ni = getval("ni"),
             t1_counter;   /* used for states tppi in t1           */ 

 double      d2_init=0.0,                        /* used for states tppi in t1 */
             tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             tau3,         /*  t2 delay */
             taua,         /*  ~ 1/4JNH =  2.25 ms */
             taub,         /*  ~ 1/4JNH =  2.25 ms */
             zeta,        /* time for C'-N to refocuss set to 0.5*24.0 ms */
             bigTN,       /* nitrogen T period */
             BigT1,       /* delay to compensate for gradient gt5 */
             satpwr,     /* low level 1H trans.power for presat  */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             cophase,      /* phase correction for CO evolution  */
             caphase,      /* phase correction for Ca evolution  */
             cbpwr,        /* power level for selective CB decoupling */
             cbdmf,        /* pulse width for selective CB decoupling */
             cbres,        /* decoupling resolution of CB decoupling */
             pwS1,         /* length of  90 on Ca */
             pwS2,         /* length of  90 on CO */
             pwS3,         /* length of 180 on Ca  */
             pwS4,         /* length of 180 on CO  */
             pwS5,         /* CHIRP inversion pulse on CO and CA  */
             pwrS5=0.0,        /* power of CHIRP pulse */

             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gt8,
             gt9,
             gstab,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7, 
             gzlvl8, 
             gzlvl9, 

             compH = getval("compH"),         /* adjustment for amplifier compression */
             pwHs = getval ("pwHs"),         /* H1 90 degree pulse at tpwrs */
             tpwrs,                          /* power for pwHs ("H2osinc") pulse */
             waltzB1 = getval("waltzB1"),

             pwClvl = getval("pwClvl"),                 /* coarse power for C13 pulse */
             pwC = getval("pwC"),             /* C13 90 degree pulse length at pwClvl */
             compC = getval("compC"),             /* ampl. compression */

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
  getstr("fco180",fco180);
  getstr("fca180",fca180);
  getstr("fscuba",fscuba);

  getstr("sel_flg",sel_flg);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  zeta  = getval("zeta");
  bigTN = getval("bigTN");
  BigT1 = getval("BigT1");
  tpwr = getval("tpwr");
  satpwr = getval("tsatpwr");
  dpwr = getval("dpwr");
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  cophase = getval("cophase");
  caphase = getval("caphase");

  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  gt9 = getval("gt9");

  gstab = getval("gstab");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");
  gzlvl9 = getval("gzlvl9");

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

    getstr("cbdecseq", cbdecseq);

/* LOAD PHASE TABLE */

  settable(t1,2,phi1);
  settable(t2,2,phi2);
  settable(t3,1,phi3);
  settable(t4,8,phi4);
  settable(t5,4,phi5);
  settable(t6,8,rec);

  /* get calculated pulse lengths of shaped C13 pulses */
        pwS1 = c13pulsepw("ca", "co", "square", 90.0);
        pwS2 = c13pulsepw("co", "ca", "sinc", 90.0);
        pwS3 = c13pulsepw("ca","co","square",180.0);
        pwS4 = c13pulsepw("co","ca","sinc",180.0);


  /*this section creates the chirp pulse inverting both co and ca*/
  /*Pcoca180 is the name of the shapelib file created            */
  /*chirp180 is a file produced by Pbox psg containing parameter values from shape*/

  strcpy(chirp_shp,"Pcoca180");
   if (FIRST_FID)                  /* make shape once */
    chirp180 = pbox(chirp_shp, CHIRP180, CHIRP180ps, dfrq, compC*pwC, pwClvl);
   pwrS5 = chirp180.pwr;             /* get pulse power from file */
   pwS5 = chirp180.pw;             /* get pulse width from file */

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

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' ))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnn' ");
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

    if( dpwr2 > 46 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
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
    if( pwN > 200.0e-6 )
    {
        printf("dont fry the probe, pwN too high ! ");
        psg_abort(1);
    } 
    if( pwC > 200.0e-6 )
    {
        printf("dont fry the probe, pwC too high ! ");
        psg_abort(1);
    } 

    if( gt3 > 2.5e-3 ) 
    {
        printf("gt3 is too long\n");
        psg_abort(1);
    }
    if( gt1 > 10.0e-3 || gt2 > 10.0e-3 || gt4 > 10.0e-3 || gt5 > 10.0e-3
        || gt6 > 10.0e-3 || gt7 > 10.0e-3 || gt8 > 10.0e-3
	|| gt9 > 10.0e-3)
    {
        printf("gt values are too long. Must be < 10.0e-3 or gt11=50us\n");
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
      printf ("Anlge_CO:\t%6.2f\n", angle_CO);
      printf ("Anlge_Ca:\t%6.2f\n", angle_Ca);
      printf ("Anlge_N :\t%6.2f\n", angle_N );
   }

/* Set up hyper complex */

   /* sw1 is used as symbolic index */
   if ( sw1 < 1000 ) { printf ("Please set sw1 to some value larger than 1000.\n"); psg_abort(1); }

   if (ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if (t1_counter % 2)  { tsadd(t2,2,4); tsadd(t6,2,4); }

   if (phase1 == 1)  { ;}                                                  /* CC */
   else if (phase1 == 2)  { tsadd(t1,1,4);}                                /* SC */
   else if (phase1 == 3)  { tsadd(t5,1,4); }                               /* CS */
   else if (phase1 == 4)  { tsadd(t1,1,4); tsadd(t5,1,4); }                /* SS */
   else { printf ("phase1 can only be 1,2,3,4. \n"); psg_abort(1); }

   if (phase2 == 2)  { tsadd(t4,2,4); icosel = +1; }                      /* N  */
            else                       icosel = -1;

   tau1 = 1.0*t1_counter*cos_CO/swTilt;
   tau2 = 1.0*t1_counter*cos_Ca/swTilt;
   tau3 = 1.0*t1_counter*cos_N/swTilt;

   tau1 = tau1/2.0;  tau2 = tau2/2.0;  tau3 = tau3/2.0;


/* CHECK VALIDITY OF PARAMETER RANGES */

    if (bigTN - 0.5*ni*(cos_N/swTilt) < 0.2e-6)
       { printf(" ni is too big. Make ni equal to %d or less.\n",
         ((int)((bigTN )*2.0*swTilt/cos_N)));         psg_abort(1);}


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   set_c13offset("co");		/* set Dec1 carrier at Co		      */
   obspower(satpwr);      /* Set transmitter power for 1H presaturation */
   obspwrf(4095.0);
   decpower(pwClvl);      /* Set Dec1 power for hard 13C pulses         */
   decpwrf(4095.0);
   dec2power(pwNlvl);      /* Set Dec2 power for 15N hard pulses         */
   dec2pwrf(4095.0);

/* Presaturation Period */

   if (satmode[0] == 'y')
   {
	delay(2.0e-5);
        rgpulse(d1,zero,2.0e-6,2.0e-6); /* presaturation */
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
   txphase(one);
   dec2phase(zero);
   delay(1.0e-5);

/* Begin Pulses */

status(B);

   rcvroff();
   lk_hold();
   delay(20.0e-6);
   shiftedpulse("sinc", pwHs, 90.0, 0.0, one, 2.0e-6, 2.0e-6);
   txphase(zero);

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(0.2e-6);
   zgradpulse(gzlvl1, gt1);
   delay(2.0e-6);

   delay(taua - gt1 - 2.2e-6);   /* taua <= 1/4JNH */ 

   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

   txphase(three); dec2phase(zero); decphase(zero); 

   delay(0.2e-6);
   zgradpulse(gzlvl1, gt1);
   delay(gstab);

   delay(taua - gt1 - gstab - 2.0e-6); 

   if(sel_flg[A] == 'n') {

   rgpulse(pw,three,2.0e-6,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl2, gt2);
   delay(gstab);

   dec2rgpulse(pwN,zero,0.0,0.0);
   decpower(pwrS5);
   delay( zeta -POWER_DELAY);
  
   dec2rgpulse(2.0*pwN,zero,0.0,0.0);
   decshapedpulse(chirp_shp, pwS5, zero, 0.0, 0.0);
   decpower(pwClvl);

   delay(zeta - pwS5 - POWER_DELAY - 2.0e-6);

   dec2rgpulse(pwN,zero,2.0e-6,0.0);

  }

  else {

   rgpulse(pw,one,2.0e-6,0.0);

   initval(1.0,v3);
   dec2stepsize(45.0); 
   dcplr2phase(v3);

   delay(0.2e-6);
   zgradpulse(gzlvl2, gt2);
   delay(gstab);

   dec2rgpulse(pwN,zero,0.0,0.0);
   dcplr2phase(zero);

   delay(1.34e-3 - SAPS_DELAY - 2.0*pw);

   rgpulse(pw,one,0.0,0.0);
   rgpulse(2.0*pw,zero,0.0,0.0);
   rgpulse(pw,one,0.0,0.0);

   decpower(pwrS5);
   delay( zeta - 1.34e-3 - 2.0*pw -POWER_DELAY);
  
   dec2rgpulse(2.0*pwN,zero,0.0,0.0);
   decshapedpulse(chirp_shp, pwS5, zero, 0.0, 0.0);
   decpower(pwClvl);

   delay(zeta - pwS5 - POWER_DELAY - 2.0e-6);

   dec2rgpulse(pwN,zero,2.0e-6,0.0);

   }

   dec2phase(zero); decphase(t1);

   delay(0.2e-6);
   zgradpulse(gzlvl3, gt3);
   delay(gstab);

/* t1 period for CO evolution */
   c13pulse("co", "ca", "sinc", 90.0, t1, 0.0, 0.0);

    if (!strcmp(fco180, "y"))
    {
      delay(10.0e-6);
      sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 2.0e-6);
      decstepsize(1.0);
      initval(cophase,v4);
      dcplrphase(v4);
      delay(10.0e-6);
    }
    else
    {
     if (tau1-2.0*pwS2/PI-pwN-WFG3_START_DELAY-POWER_DELAY-2.0e-6 > 0.0)
     {
      delay(tau1-2.0*pwS2/PI-pwN-WFG3_START_DELAY-POWER_DELAY-2.0e-6);
      sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 2.0e-6);

      decstepsize(1.0);
      initval(cophase,v4);
      dcplrphase(v4);

      delay(tau1-2.0*pwS2/PI-pwN-SAPS_DELAY-WFG3_STOP_DELAY-POWER_DELAY-2.0e-6);
     }
    else
     {
     c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
     }
    }

   c13pulse("co", "ca", "sinc", 90.0, zero, 4.0e-6, 0.0);
   dcplrphase(zero);

   set_c13offset("ca");   /* change Dec1 carrier to Ca (55 ppm) */
   delay(0.2e-6);
   zgradpulse(gzlvl4, gt4);
   delay(gstab);

/*  t2 period  for Ca evolution*/
 
                /* Turn on D decoupling using the third decoupler */
                dec3unblank(); dec3rgpulse(1/dmf3, one, 0.0, 0.0);
                dec3unblank(); setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
                /* Turn on D decoupling */

   c13pulse("ca", "co", "square", 90.0, t5, 0.0, 0.0);

    if (!strcmp(fca180, "y"))
    {
      delay(10.0e-6);
      sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 2.0e-6);
      decstepsize(1.0);
      initval(caphase,v5);
      dcplrphase(v5);
      delay(10.0e-6);
    }
    else
    {

    if (tau2-pwN-2.0*pwS1/PI-WFG3_START_DELAY-2*POWER_DELAY-
        -WFG_STOP_DELAY-WFG_START_DELAY-2.0e-6 > 0.0)
    {
      decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

     delay(tau2-pwN-2.0*pwS1/PI-WFG3_START_DELAY-2*POWER_DELAY-
           WFG_STOP_DELAY-WFG_START_DELAY-2.0e-6);

      decoff();
      decprgoff();

     decphase(zero); dec2phase(zero);
     decpower(pwClvl);
     sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 2.0e-6);

      decpower(cbpwr);
      decphase(zero);
      decprgon(cbdecseq,1/cbdmf,cbres);
      decon();

     delay(tau2-pwN-2.0*pwS1/PI-SAPS_DELAY-WFG3_STOP_DELAY-2*POWER_DELAY-
           WFG_STOP_DELAY-WFG_START_DELAY-2.0e-6);

      decoff();
      decprgoff();

      decstepsize(1.0);
      initval(caphase,v5);
      dcplrphase(v5);

     decpower(pwClvl);

    }
     else 
     {
     c13pulse("ca", "co", "square", 180.0, zero, 0.0, 0.0);
     }
    }
 
   c13pulse("ca", "co", "square", 90.0, zero, 4.0e-6, 0.0);
   dcplrphase(zero);
 
                /* Turn off D decoupling */
                dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
                setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3); dec3blank();
                /* Turn off D decoupling */
 
   set_c13offset("co");   /* set carrier back to Co */

   delay(0.2e-6);
   zgradpulse(gzlvl9, gt9);
   delay(gstab);


/* t3 period */
   dec2rgpulse(pwN,t2,2.0e-6,0.0);

   dec2phase(t3);
   decpower(pwrS5);
   delay(bigTN - tau3 -POWER_DELAY);

   dec2rgpulse(2.0*pwN,t3,0.0,0.0);
   decshapedpulse(chirp_shp, pwS5, zero, 0.0, 0.0);
   decpower(pwClvl);

   txphase(zero);
   dec2phase(t4);

   delay(0.2e-6);
   zgradpulse(icosel*gzlvl5, gt5);
   delay(gstab);

 
  delay(bigTN - WFG_START_DELAY - pwS5 - WFG_STOP_DELAY
         - gt5 - gstab - 2.0*GRADIENT_DELAY);

   delay(tau3);

   sim3pulse(pw,0.0,pwN,zero,zero,t4,0.0,0.0);

   c13pulse("co", "ca", "sinc", 90.0, zero, 4.0e-6, 0.0);
      set_c13offset("ca");
   c13pulse("ca", "co", "square", 90.0, zero, 20.0e-6, 0.0);


   delay(0.2e-6);
   zgradpulse(gzlvl6, gt6);
   delay(2.0e-6);

   dec2phase(zero);
   delay(taub - POWER_DELAY - 4.0e-6 - pwS1 - 20.0e-6 - pwS2 - gt6 - 2.2e-6);

   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

   set_c13offset("co");
   delay(0.2e-6);
   zgradpulse(gzlvl6, gt6);
   delay(gstab);
   
   txphase(one);
   dec2phase(one);

   delay(taub - gt6 - gstab);

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
   delay(gstab);

   delay(taub - gt7 - gstab);

   sim3pulse(pw,0.0,pwN,zero,zero,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(-gzlvl8, gt8/2.0);
   delay(gstab);

   delay(BigT1 - gt8/2.0 - gstab - 0.5*(pwN - pw) - 2.0*pw/PI);

   rgpulse(2*pw,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl8, gt8/2.0);
   delay(gstab);
   
   dec2power(dpwr2);
   decpower(dpwr);
   
   delay(BigT1 - gt8/2.0 - gstab - 2.0*POWER_DELAY);

lk_sample();

status(C);
         setreceiver(t6);

}
