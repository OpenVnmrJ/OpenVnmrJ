/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  PR42_ghn_co_caP_TROSY.c

Ref: (4,2)D Projection-Reconstruction Experiemnts for Protein Backbone
Assignment:  Application to Human Carbonic Anhydrase II and Calbindin
D28K.  Venters, R.A., Coggins, B.E., Kojetin, D., Cavanagh, J. and
Zhou, P. JACS 127(24), 8785-8795 (2005)

To obtain reconstruction software package, please visit
http://zhoulab.biochem.duke.edu/software/pr-calc


    This pulse sequence will allow one to perform the following experiment:

    2D projection-reconstruction hncoca with trosy
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
        satmode            'y' for presaturation of H2O
        fscuba          'y' for apply scuba pulse after presaturation of H2O
        sel_flg         'y' for active suppression of the anti-TROSY component
        sel_flg         'n' for relaxation suppression of the anti-TROSY component

	Standard Settings
        satmode='n',fscuba='n'

    Set the carbon carrier on the C' and use the waveform to pulse the
        c-alpha

    Written By Daiwen Yang on September 16, 1998.

    Modified by L. E. Kay on Sept8, 1999 to include sel_flg
    Modified on 1/2/04 by R. Venters to a projection reconstruction sequence.
     Also added Cb decoupling
    Modified on 10/07/04 by R. Venters to collect proper tilts
    Modified on 03/16/06 by R. Venters for BioPack

*/

#include "standard.h"
#include "bionmr.h"

static int  phi1[1]  = {0},
            phi2[1]  = {1},
            phi3[4]  = {0,0,2,2},
            phi4[1]  = {0},
            phi5[1]  = {0},
            phi7[4]  = {0,0,2,2},
            phi8[4]  = {0,1,2,3},
            rec[4]   = {0,2,2,0};

static double d2_init=0.0;
            
pulsesequence()
{
/* DECLARE VARIABLES */

 char       satmode[MAXSTR],
	    fscuba[MAXSTR],
            fc180[MAXSTR],    /* Flag for checking sequence              */
            ddseq[MAXSTR],    /* 2H decoupling seqfile */
            fCTCa[MAXSTR],    /* Flag for CT or non_CT on Ca dimension */
            sel_flg[MAXSTR],
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
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
	     sphase,       /* small angle phase shift */
	     sphase1,
	     sphase2,      /* used only for constant t2 period */
             pwS4,         /* selective CO 180 */
             pwS3,         /* selective Ca 180 */
             pwS1,         /* selecive Ca 90 */
             pwS2,         /* selective CO 90 */
	     cbpwr,        /* power level for selective CB decoupling */
	     cbdmf,        /* pulse width for selective CB decoupling */
             cbres,        /* decoupling resolution of CB decoupling */

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
  getstr("ddseq",ddseq);
  getstr("fCTCa",fCTCa);

  getstr("sel_flg",sel_flg);

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
  dpwr = getval("dpwr");
  dpwr3 = getval("dpwr3");
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  sphase = getval("sphase");
  sphase1 = getval("sphase1");
  sphase2 = getval("sphase2");

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

   pwS1=c13pulsepw("ca", "co", "square", 90.0);
   pwS2=c13pulsepw("co", "ca", "sinc", 90.0);
   pwS3=c13pulsepw("ca", "co", "square", 180.0);
   pwS4=c13pulsepw("co", "ca", "sinc", 180.0);

   tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
   tpwrs = (int) (tpwrs);                          /*power than a square pulse */


/* CHECK VALIDITY OF PARAMETER RANGES */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnn' ");
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

    if (bigTN - 0.5*ni*(cos_N/swTilt) + pwS4 < 0.2e-6)
       { printf(" ni is too big. Make ni equal to %d or less.\n",
         ((int)((bigTN + pwS4)*2.0*swTilt/cos_N)));              psg_abort(1);}

    if ((fCTCa[A]=='y') && (bigTCa - 0.5*ni*(cos_Ca/swTilt) - WFG_STOP_DELAY 
             - POWER_DELAY - gt11 - 50.2e-6 < 0.2e-6))
       {
         printf(" ni is too big for Ca. Make ni equal to %d or less.\n",
            (int) ((bigTCa -WFG_STOP_DELAY
              - POWER_DELAY - gt11 - 50.2e-6)/(0.5*cos_Ca/swTilt)) );
         psg_abort(1);
       }

     if (bigTCo - 0.5*ni*(cos_CO/swTilt) - 4.0e-6 - POWER_DELAY < 0.2e-6)
       {
        printf(" ni is too big for CO. Make ni equal to %d or less.\n",
        (int) ((bigTCo -  4.0e-6 - POWER_DELAY) / (0.5*cos_CO/swTilt)) );
        psg_abort(1);
        }


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obsoffset(tof);
   obspower(satpwr);      /* Set transmitter power for 1H presaturation */
   obspwrf(4095.0);
   decpower(pwClvl);       /* Set Dec1 power for hard 13C pulses         */
   decpwrf(4095.0);
   dec2power(pwNlvl);      /* Set Dec2 power for 15N hard pulses         */
   dec2pwrf(4095.0);
   set_c13offset("ca");		/* set Dec1 carrier at Ca		      */
   sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 0.0,
                             zero, zero, zero, 2.0e-6, 0.0);
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
   txphase(one);
   dec2phase(zero);
   delay(1.0e-5);

/* Begin Pulses */

status(B);

   rcvroff();
   lk_hold();

   shiftedpulse("sinc", pwHs, 90.0, 0.0, one, 2.0e-6, 0.0);
   txphase(zero);
   delay(2.0e-6);


/*   xxxxxxxxxxxxxxxxxxxxxx    1HN to 15N TRANSFER   xxxxxxxxxxxxxxxxxx    */

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(0.2e-6);
   zgradpulse(gzlvl1, gt1);
   delay(2.0e-6);

   delay(taua - gt1 - 2.2e-6);   /* taua <= 1/4JNH */ 

   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

   txphase(three); dec2phase(zero); decphase(zero); 

   delay(taua - gt1 - gstab -0.2e-6 - 2.0e-6);

   delay(0.2e-6);
   zgradpulse(gzlvl1, gt1);
   delay(gstab);

/*   xxxxxxxxxxxxxxxxxxxxxx    15N to 13CO TRANSFER   xxxxxxxxxxxxxxxxxx    */

   if(sel_flg[A] == 'n') {

   rgpulse(pw,three,2.0e-6,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl2, gt2);
   delay(gstab);

   dec2rgpulse(pwN,zero,0.0,0.0);

   delay( zeta + pwS4 );

   dec2rgpulse(2*pwN,zero,0.0,0.0);
   c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
   dec2phase(one);

   delay(zeta - 2.0e-6);

   dec2rgpulse(pwN,one,2.0e-6,0.0);

  }

   else {

   rgpulse(pw,one,2.0e-6,0.0);

   initval(1.0,v6);
   dec2stepsize(45.0);
   dcplr2phase(v6);


   delay(0.2e-6);
   zgradpulse(gzlvl2, gt2);
   delay(gstab);

   dec2rgpulse(pwN,zero,0.0,0.0);
   dcplr2phase(zero); dec2phase(zero);

   delay(1.34e-3 - SAPS_DELAY - 2.0*pw);

   rgpulse(pw,one,0.0,0.0);
   rgpulse(2.0*pw,zero,0.0,0.0);
   rgpulse(pw,one,0.0,0.0);

   delay( zeta - 1.34e-3 - 2.0*pw + pwS4 );

   dec2rgpulse(2*pwN,zero,0.0,0.0);
   c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);
   dec2phase(one);

   delay(zeta - 2.0e-6);

   dec2rgpulse(pwN,one,2.0e-6,0.0);

  }

   dec2phase(zero); decphase(zero);

   delay(0.2e-6);
   zgradpulse(gzlvl3, gt3);
   delay(gstab);

/* xxxxxxxxxxxxxxxxxxxxx 13CO to 13CA TRANSFER xxxxxxxxxxxxxxxxxxxxxxx  */

   c13pulse("co", "ca", "sinc", 90.0, zero, 2.0e-6, 0.0);

                delay(2.0e-7);
                zgradpulse(gzlvl10, gt10);
                delay(100.0e-6);

  delay(tauc - POWER_DELAY - gt10 - 100.2e-6 - (0.5*10.933*pwC));

        decrgpulse(pwC*158.0/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC*171.2/90.0, two, 0.0, 0.0);
        decrgpulse(pwC*342.8/90.0, zero, 0.0, 0.0);      /* Shaka 6 composite */
        decrgpulse(pwC*145.5/90.0, two, 0.0, 0.0);
        decrgpulse(pwC*81.2/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC*85.3/90.0, two, 0.0, 0.0);

                delay(2.0e-7);
                zgradpulse(gzlvl10, gt10);
                delay(100.0e-6);

      delay(tauc - POWER_DELAY - 4.0e-6 - gt10 - 100.2e-6 - (0.5*10.933*pwC));

   c13pulse("co", "ca", "sinc", 90.0, one, 4.0e-6, 0.0);

   set_c13offset("ca");   /* change Dec1 carrier to Ca (55 ppm) */
   delay(0.2e-6);
   zgradpulse(gzlvl9, gt9);
   delay(gstab);

/* xxxxxxxxxxxxxxxxxx 13CA EVOLUTION xxxxxxxxxxxxxxxxxxxxxx */
                /* Turn on D decoupling using the third decoupler */
                dec3unblank(); dec3rgpulse(1/dmf3, one, 0.0, 0.0);
                dec3unblank(); setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
                /* Turn on D decoupling */

   c13pulse("ca", "co", "square", 90.0, t5, 2.0e-6, 0.0);

if (fCTCa[A]=='y')  
{
/* Constant t2 */
   decpower(cbpwr);
   decphase(zero);
   decprgon(cbdecseq,1/cbdmf,cbres);
   decon();
	   
   delay(tau1);

   decoff();
   decprgoff();
   decpower(pwClvl);

   dec2rgpulse(pwN,one,0.0,0.0);
   dec2rgpulse(2*pwN,zero,0.0,0.0);
   dec2rgpulse(pwN,one,0.0,0.0);
   c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);

   decpower(cbpwr);
   decphase(zero);
   decprgon(cbdecseq,1/cbdmf,cbres);
   decon();

   delay(bigTCa - 4.0*pwN - WFG_START_DELAY - pwS4
         - WFG_STOP_DELAY - POWER_DELAY - WFG_START_DELAY - gt11 - gstab -0.2e-6);

   decoff();
   decprgoff();
   decpower(pwClvl);

   delay(0.2e-6);
   zgradpulse(gzlvl11, gt11);
   delay(gstab);

       initval(1.0,v3);
       decstepsize(140);
       dcplrphase(v3);

   c13pulse("ca", "co", "square", 180.0, zero, 0.0, 0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl11, gt11);
   delay(gstab);

   decpower(cbpwr);
   decphase(zero);
   decprgon(cbdecseq,1/cbdmf,cbres);
   decon();

   delay(bigTCa - tau1 - WFG_STOP_DELAY - POWER_DELAY - gt11 - gstab -0.2e-6);

   decoff();
   decprgoff();
}

/* non_constant t2 */
else
{
  if (fc180[A]=='n')
   {
    if ((ni>1.0) && (tau1>0.0))
    {
    if (tau1 - 2.0*pwS1/PI - PRG_START_DELAY - 2*POWER_DELAY -
        PRG_STOP_DELAY - pwN > 0.0)
     {
   decpower(cbpwr);
   decphase(zero);
   decprgon(cbdecseq,1/cbdmf,cbres);
   decon();

   delay(tau1 - 2.0*pwS1/PI - PRG_START_DELAY - 2*POWER_DELAY -
        PRG_STOP_DELAY - pwN);
   decoff();
   decprgoff();

   decphase(zero); dec2phase(zero);
   decpower(pwClvl);

   sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 0.0, 0.0);

   decpower(cbpwr);
   decphase(zero);
   decprgon(cbdecseq,1/cbdmf,cbres);
   decon();

   delay(tau1 - 2.0*pwS1/PI - PRG_START_DELAY - 2*POWER_DELAY -
        PRG_STOP_DELAY - pwN);
   decoff();
   decprgoff();

   decstepsize(1.0);
   initval(sphase1, v3);
   dcplrphase(v3);

     }
    else
     {
       tsadd(t6,2,4);
       delay(2.0*tau1);
       delay(10.0e-6); 
       sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 0.0);
       delay(10.0e-6);
     }
   }
   else
   {

       tsadd(t6,2,4);
       delay(10.0e-6);
       sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 0.0);
       delay(10.0e-6);
   }
 }
   else
  {
   /* for checking sequence */
   c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);
  }
}

   decpower(pwClvl);
   decphase(t7);
   c13pulse("ca", "co", "square", 90.0, t7, 4.0e-6, 0.0);
   dcplrphase(zero);
 
                /* Turn off D decoupling */
                dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
                setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3); dec3blank();
                /* Turn off D decoupling */
 

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

   c13pulse("ca", "co", "square", 180.0, zero, 0.0, 0.0);
       decphase(t8);

                initval(1.0,v4);
                decstepsize(sphase);
                dcplrphase(v4);

      delay(bigTCo - taud
            - 0.5*(WFG_START_DELAY + pwS3 + WFG_STOP_DELAY) );

      c13pulse("co", "ca", "sinc", 180.0, t8, 0.0, 0.0);
      dcplrphase(zero); decphase(one);

    delay(bigTCo - tau2 - POWER_DELAY - 4.0e-6);

   c13pulse("co", "ca", "sinc", 90.0, one, 4.0e-6, 0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl4, gt4);
   delay(gstab);

/* t3 period */
   dec2rgpulse(pwN,t2,2.0e-6,0.0);

   dec2phase(t3);

   delay(bigTN - tau3 + pwS4);

     dec2rgpulse(2*pwN,t3,0.0,0.0);
     c13pulse("co", "ca", "sinc", 180.0, zero, 0.0, 0.0);

   txphase(zero);
   dec2phase(t4);

  delay(bigTN - gt5 - gstab -0.2e-6 - 2.0*GRADIENT_DELAY
	- 4.0e-6 - WFG_START_DELAY - pwS3 - WFG_STOP_DELAY);

   delay(0.2e-6);
   zgradpulse(icosel*gzlvl5, gt5);
   delay(gstab);

      c13pulse("ca", "co", "square", 180.0, zero, 4.0e-6, 0.0);

   delay(tau3);

   sim3pulse(pw,0.0,pwN,zero,zero,t4,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl6, gt6);
   delay(2.0e-6);

   dec2phase(zero);
   delay(taub - gt6 - 2.2e-6);

   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

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

   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl7, gt7);
   delay(200.0e-6);

   delay(taub - gt7 - 200.2e-6);

   sim3pulse(pw,0.0,pwN,zero,zero,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(-gzlvl8, gt8/2.0);
   delay(50.0e-6);

   delay(BigT1 - gt8/2.0 - 50.2e-6 - 0.5*(pwN - pw) - 2.0*pw/PI);

   rgpulse(2*pw,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl8, gt8/2.0);
   delay(50.0e-6);
   
   dec2power(dpwr2);
   decpower(dpwr);
   
   delay(BigT1 - gt8/2.0 - 50.2e-6 - 2.0*POWER_DELAY);

lk_sample();
/*   rcvron();  */          /* Turn on receiver to warm up before acq */ 

/* BEGIN ACQUISITION */

status(C);
         setreceiver(t6);

}
