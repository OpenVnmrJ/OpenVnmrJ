/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  PR42_ghn_co_caP.c

    This pulse sequence will allow one to perform the following experiment:

    2D projection-reconstruction non-trosy hncoca
                       F1      CO
                       F2      CA
                       F3      15N
                       F4(acq) 1H(NH)

    This sequence uses the standard three channel configuration
         1)  1H             - carrier (tof) @ 4.7 ppm [H2O]
         2) 13C             - carrier (dof) @ 176 ppm [CO] or CA 55ppm
         3) 15N             - carrier (dof2)@ 119 ppm [centre of amide 15N]  
    
    Set dm = 'nnn', dmm = 'ccc' 
    Set dm2 = 'nny', dmm2 = 'ccp'

    Flags
        satmode         'y' for presaturation of H2O
        fscuba          'y' for apply scuba pulse after presaturation of H2O

	Standard Settings
        satmode='n',fscuba='n'

    Set the carbon carrier on the C' and use the waveform to pulse the
        c-alpha

    Written By Daiwen Yang on September 16, 1998.

    Modified on 1/2/04 by R. Venters to a projection reconstruction sequence.
     Also added Cb decoupling
    Modified on 10/0704 by R. Venters to collect proper tilts
    Modified on 3/16/06 by R. Venters for BioPack

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

/* CHECK */

static int  phi1[1]  = {0},
            phi2[1]  = {0},
            phi3[4]  = {0,0,2,2},
            phi4[1]  = {0},
            phi5[1]  = {0},
            phi7[4]  = {0,0,2,2},
            phi8[4]  = {0,1,2,3},
            rec[4]   = {2,0,0,2};

static double d2_init=0.0;
            
void pulsesequence()
{
/* DECLARE VARIABLES */

 char       satmode[MAXSTR],
	    fscuba[MAXSTR],
            fc180[MAXSTR],    /* Flag for checking sequence              */
	    cbdecseq[MAXSTR];

 int         icosel,
             ni = getval("ni"),
             t1_counter;   /* used for states tppi in t1           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             tau3,         /*  t2 delay */
             taua,         /*  ~ 1/4JNH =  2.25 ms */
             taub,         /*  ~ 1/4JNH =  2.25 ms */
             tauc,         /*  ~ 1/4JCaC' =  4 ms */
             taud,         /*  ~ 1/4JCaC' =  4.5 ms if bigTCo can be set to be
				less than 4.5ms and then taud can be smaller*/
             zeta,        /* time for C'-N to refocuss set to 0.5*24.0 ms */
             bigTCa,      /* Ca T period */
             bigTCo,      /* Co T period */
             bigTN,       /* nitrogen T period */
             BigT1,       /* delay to compensate for gradient gt5 */
             satpwr,     /* low level 1H trans.power for presat  */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
	     sphase,       /* small angle phase shift */
	     cbpwr,        /* power level for selective CB decoupling */
	     cbdmf,        /* pulse width for selective CB decoupling */
	     cbres,        /* decoupling resolution of CB decoupling */

   pwS1,                                         /* length of square 90 on Ca */
   phshift,        /*  phase shift induced on Ca by 180 on CO in middle of t1 */
   pwS2,                                               /* length of 180 on CO */
   pwS3,                                              /* length of 180 on Ca  */
   pwS4,                                             /* length of 90 on CO */
   pwS = getval("pwS"), /* used to change 180 on CO in t1 for 1D calibrations */
   pwZ,                                    /* the largest of pwS2 and 2.0*pwN */
   pwZ1,                /* the largest of pwS2 and 2.0*pwN for 1D experiments */

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
             gt12,
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
             gzlvl10, 
             gzlvl11, 
             gzlvl12,

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
	     swTilt,      /* This is the sweep width of the tilt vector */

	     cos_N, cos_CO, cos_Ca,
	     angle_N, angle_CO, angle_Ca;
             angle_N=0.0;                      /*initialize variable*/

/* LOAD VARIABLES */


  getstr("satmode",satmode);
  getstr("fc180",fc180);
  getstr("fscuba",fscuba);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  tauc   = getval("tauc"); 
  taud   = getval("taud"); 
  zeta  = getval("zeta");
  bigTCa = getval("bigTCa");
  bigTCo = getval("bigTCo");
  bigTN = getval("bigTN");
  BigT1 = getval("BigT1");
  tpwr = getval("tpwr");
  satpwr = getval("satpwr");
  dpwr = getval("dpwr");
  sw1 = getval("sw1");
  sw2 = getval("sw2");
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
  gt12 = getval("gt12");

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
  gzlvl10 = getval("gzlvl10");
  gzlvl11 = getval("gzlvl11");
  gzlvl12 = getval("gzlvl12");

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

  settable(t1,1,phi1);
  settable(t2,1,phi2);
  settable(t3,4,phi3);
  settable(t4,1,phi4);
  settable(t5,1,phi5);
  settable(t7,4,phi7);
  settable(t8,4,phi8);
  settable(t6,4,rec);

   kappa = 5.4e-3;

   /* get calculated pulse lengths of shaped C13 pulses */
        pwS1 = c13pulsepw("ca", "co", "square", 90.0);
        pwS2 = c13pulsepw("co", "ca", "sinc", 180.0);
        pwS3 = c13pulsepw("ca","co","square",180.0);
        pwS4 = c13pulsepw("co", "ca", "sinc", 90.0);

    /* the 180 pulse on CO at the middle of t1 */
        if (pwS2 > 2.0*pwN) pwZ = pwS2; else pwZ = 2.0*pwN;
        if ((pwS==0.0) && (pwS2>2.0*pwN)) pwZ1=pwS2-2.0*pwN; else pwZ1=0.0;
        if ( ni > 1 )     pwS = 180.0;
        if ( pwS > 0 )   phshift = 140.0;
        else             phshift = 0.0;

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

    if((dm2[A] == 'y' || dm2[B] == 'y' ))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nny' ");
        psg_abort(1);
    }

    if( satpwr > 6 )
    {
        printf("TSATPWR too large !!!  ");
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

    if( pwS3 > 200.0e-6 )
    {
        printf("dont fry the probe, pwS3 too high ! ");
        psg_abort(1);
    }

    if( gt3 > 2.5e-3 ) 
    {
        printf("gt3 is too long\n");
        psg_abort(1);
    }
    if( gt1 > 10.0e-3 || gt2 > 10.0e-3 || gt4 > 10.0e-3 || gt5 > 10.0e-3
        || gt6 > 10.0e-3 || gt7 > 10.0e-3 || gt8 > 10.0e-3
	|| gt9 > 10.0e-3 || gt10 > 10.0e-3 || gt11 > 50.0e-6)
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
      printf ("Angle_CO:\t%6.2f\n", angle_CO);
      printf ("Angle_Ca:\t%6.2f\n", angle_Ca);
      printf ("Angle_N :\t%6.2f\n", angle_N );
   }

/* Set up hyper complex */

   /* sw1 is used as symbolic index */
   if ( sw1 < 1000 ) { printf ("Please set sw1 to some value larger than 1000.\n"); psg_abort(1); }

   if (ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if (t1_counter % 2)  { tsadd(t2,2,4); tsadd(t6,2,4); }

   if (phase1 == 1)  { ;}                                                  /* CC */
   else if (phase1 == 2)  { tsadd(t5,1,4);}                                /* SC */
   else if (phase1 == 3)  { tsadd(t1,1,4); }                               /* CS */
   else if (phase1 == 4)  { tsadd(t5,1,4); tsadd(t1,1,4); }                /* SS */
   else { printf ("phase1 can only be 1,2,3,4. \n"); psg_abort(1); }

   if (phase2 == 2)  { tsadd(t4,2,4); icosel = 1; }                      /* N  */
            else                       icosel = -1;

   tau1 = 1.0*t1_counter*cos_Ca/swTilt;
   tau2 = 1.0*t1_counter*cos_CO/swTilt;
   tau3 = 1.0*t1_counter*cos_N/swTilt;

   tau1 = tau1/2.0;  tau2 = tau2/2.0;  tau3 = tau3/2.0;


/* CHECK VALIDITY OF PARAMETER RANGES */

    if (bigTN - 0.5*ni*(cos_N/swTilt) + pwS2 < 0.2e-6)
       { printf(" ni is too big. Make ni equal to %d or less.\n",
         ((int)((bigTN + pwS2)*2.0*swTilt/cos_N)));              psg_abort(1);}


     if (bigTCo - 0.5*ni*(cos_CO/swTilt) - 4.0e-6 - POWER_DELAY < 0.2e-6)
       {
        printf(" ni is too big for CO. Make ni equal to %d or less.\n",
        (int) ((bigTCo -  4.0e-6 - POWER_DELAY) / (0.5*cos_CO/swTilt)) );
        psg_abort(1);
        }


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obsoffset(tof);
   set_c13offset("ca");		/* set Dec1 carrier at Co		      */
   obspower(satpwr);      /* Set transmitter power for 1H presaturation */
   obspwrf(4095.0);
   decpower(pwClvl);       /* Set Dec1 power for hard 13C pulses         */
   decpwrf(4095.0);
   dec2power(pwNlvl);      /* Set Dec2 power for hard 15N pulses         */
   dec2pwrf(4095.0);
   sim3_c13pulse("", "ca", "co", "sinc", "", 0.0, 180.0, 0.0, /* to produce shape  */
                             zero, zero, zero, 2.0e-6, 2.0e-4);
   set_c13offset("co");		/* set Dec1 carrier at Co		      */

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

   delay(taua - gt1 - gstab -0.2e-6 - 2.0e-6);

   delay(0.2e-6);
   zgradpulse(gzlvl1, gt1);
   delay(gstab);

/*   xxxxxxxxxxxxxxxxxxxxxx    15N to 13CO TRANSFER   xxxxxxxxxxxxxxxxxx    */

   rgpulse(pw,one,2.0e-6,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl2, gt2);
   delay(gstab);

   dec2rgpulse(pwN,zero,0.0,0.0);

   delay(kappa - POWER_DELAY - PWRF_DELAY - pwHd - 4.0e-6 - PRG_START_DELAY);
                            /* delays for h1waltzon subtracted */
   h1waltzon("WALTZ16", widthHd, 0.0);
   decphase(zero);
   dec2phase(zero);
   delay(zeta - kappa - WFG3_START_DELAY);

   dec2rgpulse(2*pwN,zero,0.0,0.0);
   c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
   dec2phase(zero);

   delay(zeta - 2.0e-6);

   dec2rgpulse(pwN,zero,2.0e-6,0.0);

   h1waltzoff("WALTZ16", widthHd, 0.0);

   dec2phase(zero); decphase(zero);

   delay(0.2e-6);
/* CHECK Negative gradient */
   zgradpulse(-gzlvl3, gt3);
   delay(gstab);

/* xxxxxxxxxxxxxxxxxxxxx 13CO to 13CA TRANSFER xxxxxxxxxxxxxxxxxxxxxxx  */

   c13pulse("co", "ca", "sinc", 90.0, zero, 2.0e-6, 0.0);

                delay(2.0e-7);
                zgradpulse(gzlvl10, gt10);

  delay(tauc -  gt10 - 0.2e-6 - (0.5*10.933*pwC));

        decrgpulse(pwC*158.0/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC*171.2/90.0, two, 0.0, 0.0);
        decrgpulse(pwC*342.8/90.0, zero, 0.0, 0.0);      /* Shaka 6 composite */
        decrgpulse(pwC*145.5/90.0, two, 0.0, 0.0);
        decrgpulse(pwC*81.2/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC*85.3/90.0, two, 0.0, 0.0);

                delay(2.0e-7);
                zgradpulse(gzlvl10, gt10);

      delay(tauc -  4.0e-6 - gt10 - 0.2e-6 - (0.5*10.933*pwC));

   c13pulse("co", "ca", "sinc", 90.0, one, 4.0e-6, 0.0);

   set_c13offset("ca");   /* change Dec1 carrier to Ca (55 ppm) */
   delay(0.2e-6);
   zgradpulse(gzlvl9, gt9);
   decphase(t5);
   delay(gstab);

/* xxxxxxxxxxxxxxxxxx 13CA EVOLUTION xxxxxxxxxxxxxxxxxxxxxx */

   h1waltzon("WALTZ16", widthHd, 0.0);
   c13pulse("ca", "co", "square", 90.0, t5, 2.0e-6, 0.0);      /*  pwS1  */

 if (fc180[A]=='n')
  {
   if ((ni>1.0) && (tau1>0.0))
    {
     if (tau1 - 2.0*pwS1/PI - WFG3_START_DELAY - PRG_START_DELAY - 2*POWER_DELAY -
        PRG_STOP_DELAY - 2.0*PWRF_DELAY - 0.5*pwZ > 0.0)
      {
       decpower(cbpwr);
       decphase(zero);
       decprgon(cbdecseq,1/cbdmf,cbres);
       decon();

       delay(tau1 - 2.0*pwS1/PI - WFG3_START_DELAY - PRG_START_DELAY - 2*POWER_DELAY -
        PRG_STOP_DELAY - 2.0*PWRF_DELAY - 0.5*pwZ);
       decoff();
       decprgoff();

       decpower(pwClvl);
       decphase(zero); dec2phase(zero);

       sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 2.0e-6);

       decpower(cbpwr);
       decphase(zero);
       decprgon(cbdecseq,1/cbdmf,cbres);
       decon();

       delay(tau1 - 2.0*pwS1/PI - PRG_START_DELAY - 2*POWER_DELAY -
        PRG_STOP_DELAY - WFG_START_DELAY - 2.0*PWRF_DELAY - 0.5*pwZ);
       decoff();
       decprgoff();
       decpower(pwClvl);

       initval(140.0, v9);
       decstepsize(1.0);
       dcplrphase(v9);

      }
       else
      {
       tsadd(t6,2,4);
       delay(2.0*tau1);
       delay(10.0e-6); 
       sim3_c13pulse("", "ca", "co", "sinc", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 2.0e-6);

       delay(10.0e-6);
      }
    }
     else
    {

       tsadd(t6,2,4);
       delay(10.0e-6);
	    sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 2.0e-6);
       delay(10.0e-6);
    }
  }
   else
  {
   /* for checking sequence */
   c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);
  }

   decphase(t7);
   c13pulse("ca", "co", "square", 90.0, t7, 4.0e-6, 0.0);      /*  pwS1  */
   dcplrphase(zero);
   h1waltzoff("WALTZ16", widthHd, 0.0);
 
   set_c13offset("co");   /* set carrier back to Co */

   delay(0.2e-6);

   zgradpulse(gzlvl12, gt12);
   delay(gstab);


/* xxxxxxxxxxxxxxx  13CA to 13CO TRANSFER and CT 13CO EVOLUTION xxxxxxxxxxxxxxxxx */

   c13pulse("co", "ca", "sinc", 90.0, t1, 2.0e-6, 0.0);

   delay(tau2);
   dec2rgpulse(pwN,one,0.0,0.0);
   dec2rgpulse(2*pwN,zero,0.0,0.0);
   dec2rgpulse(pwN,one,0.0,0.0);

   delay(taud - 4.0*pwN - POWER_DELAY
         - 0.5*(WFG_START_DELAY + pwS3 + WFG_STOP_DELAY));

   c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);
   decphase(t8);

                initval(1.0,v4);
                decstepsize(sphase);
                dcplrphase(v4);

      delay(bigTCo - taud
            - 0.5*(WFG_START_DELAY + pwS3 + WFG_STOP_DELAY) );

      c13pulse("co", "ca", "sinc", 180.0, t8, 2.0e-6, 0.0);
      dcplrphase(zero); decphase(one);

    delay(bigTCo - tau2 - POWER_DELAY - 4.0e-6);

   c13pulse("co", "ca", "sinc", 90.0, one, 4.0e-6, 0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl4, gt4);
   delay(gstab);

   h1waltzon("WALTZ16", widthHd, 0.0);

/* t3 period */
   dec2rgpulse(pwN,t2,2.0e-6,0.0);

   dec2phase(t3);

   delay(bigTN - tau3 + pwS2);


     dec2rgpulse(2*pwN,t3,0.0,0.0);
     c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);

   dec2phase(t4);

   if (tau3 > (kappa + PRG_STOP_DELAY + pwHd + 2.0e-6))
   {
       delay(bigTN - pwS3 - WFG_START_DELAY - 2.0*POWER_DELAY
                                - 2.0*PWRF_DELAY - 2.0e-6);

      c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);

       delay(tau3 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6 - POWER_DELAY
                                         - PWRF_DELAY);
       h1waltzoff("WALTZ16", widthHd, 0.0);
       txphase(zero);

       delay(kappa - gt5 - 2.0*GRADIENT_DELAY - 1.0e-4);
       zgradpulse(gzlvl5, gt5);        /* 2.0*GRADIENT_DELAY */
       delay(1.0e-4);
   }
   else if (tau3 > (kappa - pwS3 - WFG_START_DELAY - 2.0*POWER_DELAY
                                - 2.0*PWRF_DELAY - 2.0e-6))
   {
      delay(bigTN + tau3 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
                                 - POWER_DELAY - PWRF_DELAY);
      h1waltzoff("WALTZ16", widthHd, 0.0);
      txphase(zero);

      c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);

      delay(kappa - pwS3 - WFG_START_DELAY - 2.0*POWER_DELAY
            - 2.0*PWRF_DELAY - 2.0e-6 - gt5 - 2.0*GRADIENT_DELAY - 1.0e-4);
      zgradpulse(gzlvl5, gt5);        /* 2.0*GRADIENT_DELAY */
      delay(1.0e-4);
   }
   else if (tau3 > gt5 + 2.0*GRADIENT_DELAY + 1.0e-4)
   {
      delay(bigTN + tau3 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
                                 - POWER_DELAY - PWRF_DELAY);
      h1waltzoff("WALTZ16", widthHd, 0.0);
      txphase(zero);
      delay(kappa - tau3 - pwS3 - WFG_START_DELAY - 2.0*POWER_DELAY
                                   - 2.0*PWRF_DELAY - 2.0e-6);

      c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);

      delay(tau3 - gt5 - 2.0*GRADIENT_DELAY - 1.0e-4);
      zgradpulse(gzlvl5, gt5);        /* 2.0*GRADIENT_DELAY */
      delay(1.0e-4);
   }
   else
   {
      delay(bigTN + tau3 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
                                 - POWER_DELAY - PWRF_DELAY);
      h1waltzoff("WALTZ16", widthHd, 0.0);
      txphase(zero);
      delay(kappa - tau3 - pwS3 - WFG_START_DELAY - 2.0*POWER_DELAY
            - 2.0*PWRF_DELAY - 2.0e-6 - gt5 - 2.0*GRADIENT_DELAY - 1.0e-4);
      zgradpulse(gzlvl5, gt5);        /* 2.0*GRADIENT_DELAY */
      delay(1.0e-4);

      c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);
      delay(tau3);
   }

   sim3pulse(pw,0.0,pwN,zero,zero,t4,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl6, gt6);
   delay(2.0e-6);

   dec2phase(zero);
   delay(taub - gt6 - 2.2e-6);

   sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl6, gt6);
   delay(200.0e-6);
   
   txphase(one);
   dec2phase(one);

   delay(taub - gt6 - 200.2e-6);

   sim3pulse(pw,0.0,pwN,one,zero,one,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl7, gt7);
   delay(2.0e-6);
 
   txphase(zero);
   dec2phase(zero);

   delay(taub - gt7 - 2.2e-6);

   sim3pulse(2.0*pw,0.0,2.0*pwN,zero,zero,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl7, gt7);
   delay(200.0e-6);

   delay(taub - gt7 - 200.2e-6);

   rgpulse(pw, zero, 0.0, 0.0);
   delay(gt8 + 1.0e-4 + 1.0e-4 - 0.3*pw + 2.0*GRADIENT_DELAY
                                   + POWER_DELAY);

   rgpulse(2*pw,zero,0.0,0.0);
   dec2power(dpwr2);
   decpower(dpwr);
   zgradpulse(icosel*gzlvl8, gt8);
   delay(1.0e-4);
   

/*   rcvron();  */          /* Turn on receiver to warm up before acq */

/* BEGIN ACQUISITION */

statusdelay(C, 1.0e-4);
         setreceiver(t6);
}
