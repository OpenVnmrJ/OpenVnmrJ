/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* rna_WGgNtrosy.c
       This pulse sequence will allow one to perform trosy experiment(Ref.Pervushin
	et al.  PNAS, 94,12366,1997)
       
       Both cos and sin modulated magnetization(i.e. Nx and Ny)  were obtained. 

	Standard setting: bottom-right
           F1      15N + JNH/2 (J>0)
           F2(acq)  1H - JNH/2 (NH)

    This sequence uses the standard three channel configuration
         1)  1H             - carrier (tof) @ 4.7 ppm [H2O]
         2) 13C             - carrier (dof) @ 160 ppm [centre of arom C13]
         3) 15N             - carrier (dof2)@ 119 ppm [centre of imino N15]

			NOTE: dof must be set at 110ppm always
                        NOTE: dof2 must be set at 200ppm always

    Set dm = 'nnn', dmm = 'ccc'
    Set dm2 = 'nnn', dmm2 = 'ccc'

    Must set phase = 1,2.
    The fids must be manipulated
    (add/subtract) with 'grad_sort_nd' program (or equivalent, see below) prior to
    regular processing.

    Flags
        C13refoc        'y' for 15N and 13C labelled samples

   Based on L.E.Kay on July 28, 1998

   Modified by Peter Lukavsky for RnaPack; included peak selection.
   For RDC measurment use the downfield N15 doublet (bottom) and
   the upfield (right) and downfield (left) H1 component, since these are the
   most intense peaks of the multiplet in large RNAs.
   For RNAs in the range of 30-100kDa, we commonly use
   taua=taub=0.00153 and d1=0.8sec
   For processing use f1coef='1 0 -1 0 0 1 0 1' 
*/

#include <standard.h>


static int phi1[8] =  {1,1,3,3,2,2,0,0},
           ph_y[1] =  {3},
	   phy[1]  =  {1},
           phi4[8] =  {1,1,3,3,0,0,2,2},
           phi5[2] =  {0,2},
           phi6[2] =  {2,0},
           rec[8]  =  {1,1,3,3,0,0,2,2};

static double d2_init = 0.0;

pulsesequence()

{
/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],
            bottom[MAXSTR],
            right[MAXSTR],
            C13refoc[MAXSTR];


int	    t1_counter;

double       tau1,                 				  /* t1 delay */
             taua=getval("taua"),  		      /* < 1 / 4J(NH) 2.25 ms */
             taub=getval("taub"),  			/* 1 / 4J(NH) 2.75 ms */
   pwClvl = getval("pwClvl"),                   /* coarse power for C13 pulse */
   pwC = getval("pwC"),               /* C13 90 degree pulse length at pwClvl */
   compH = getval("compH"),        /* adjustment for H1 amplifier compression */
   compN = getval("compN"),       /* adjustment for N15 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

        pwHs = getval("pwHs"),          /* H1 90 degree pulse length at tpwrs */
        tpwrs,                        /* power for the pwHs ("H2Osinc") pulse */
	finepwrf = getval("finepwrf"), 		     /* fine power adjustment */
        pwNlvl = getval("pwNlvl"),                    /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

        sw1 = getval("sw1"),

	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
        gzlvlr = getval("gzlvlr"),
        gzlvl3 = getval("gzlvl3"),
        gzlvl4 = getval("gzlvl4"),
        gzlvl5 = getval("gzlvl5");

  getstr("f1180",f1180);
  getstr("bottom",bottom);
  getstr("right",right);
  getstr("C13refoc",C13refoc);

/*   INITIALIZE VARIABLES   */

compN=compN;
compC=compC;

/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                        /*power than a square pulse */
    tpwrs = tpwrs+6;        /* increase attenuator setting so that
           	fine power control can be varied about the nominal 2048 value */



/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y'))
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
	{
	printf("incorrect Dec2 decoupler flags!  ");
	psg_abort(1);
    } 

    if( dpwr > 50 )
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
        printf("don't fry the probe, pwNlvl too large !");
        psg_abort(1);
    }

   if( pwClvl > 63 )
    {
        printf("don't fry the probe, pwClvl too large !");
        psg_abort(1);
    }

   if( gzlvlr > 500 )
    {
        printf("don't fry the probe, gzlvl0 too large !");
        psg_abort(1);
    }

/* LOAD VARIABLES */

  settable(t1, 8, phi1);
        if (bottom[A]=='y')
{
  settable(t2, 1, ph_y);
  settable(t5, 2, phi5);
  settable(t7, 2, phi6);
}
	else
{
  settable(t2, 1, phy);
  settable(t5, 2, phi6);
  settable(t7, 2, phi5);
}
	if (right[A]=='y')
  settable(t3, 1, ph_y);
	else
  settable(t3, 1, phy);
  settable(t4, 8, phi4);
  settable(t6, 8, rec);


/* Set up f1180  */

   tau1 = d2;
   if(f1180[A] == 'y' && C13refoc[A] == 'n') 

        tau1 += ( 1.0 / (2.0*sw1) - pw);

   if(f1180[A] == 'y' && C13refoc[A] == 'y') 

        tau1 += ( 1.0 / (2.0*sw1) - 2.0*pwC - 4.0e-6 - pw);

   if(f1180[A] == 'n' && C13refoc[A] == 'n') 

        tau1 = tau1 - pw;

   if(f1180[A] == 'n' && C13refoc[A] == 'y') 

        tau1 = ( tau1 - 2.0*pwC - 4.0e-6 - pw);

   if(tau1 < 0.2e-6) tau1 = 0.2e-6;

/* set up for complex FID */

   if (phase1 == 2)
   {
      tsadd(t2,2,4);
      tsadd(t3,2,4);
      tsadd(t6,2,4);
   }


/* Calculate modifications to phases for States-TPPI acquisition  */
 
   if( ix == 1 ) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if( t1_counter %2 ) {
      tsadd(t1,2,4);
      tsadd(t4,2,4);
      tsadd(t6,2,4);
   }
    


/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);
        obsoffset(tof);
        decoffset(dof+(50*dfrq));	/* 160 ppm for DEC */
        dec2offset(dof2-(45*dfrq2));	/* Middle of imino-N15 = 155 ppm */
	obspower(tpwr);
        obspwrf(4095.0);
        decpower(pwClvl);
        dec2power(pwNlvl);
        txphase(two);
        decphase(zero);
        dec2phase(zero);

        delay(d1);
	rcvroff();
   	delay(20.0e-6);

	rgpulse(pw,two,0.0,0.0);
	txphase(zero);
	
	zgradpulse(gzlvl5, gt5);
	delay(taua - gt5 - 2.0*GRADIENT_DELAY);	/* delay < 1/4J(XH)   */

  	sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);
	txphase(one);

	zgradpulse(gzlvl5, gt5);
        delay(taua - gt5 - 2.0*GRADIENT_DELAY);

    if (phase1 == 1)
  	sim3pulse(pw,0.0,pwN,one,zero,t1,0.0,0.0);
    if (phase1 == 2) 
  	sim3pulse(pw,0.0,pwN,one,zero,t4,0.0,0.0);

   	txphase(zero);
	decphase(zero);

   if (C13refoc[A] == 'y')
    {
        if (tau1>0.001)
        {
         zgradpulse(gzlvlr,(0.8*tau1/2.0 - 2.0*GRADIENT_DELAY ));
         delay(0.2*tau1/2.0 );
        }
        else
         delay(tau1/2.0);
	decrgpulse(pwC,zero,0.0,0.0);
	decrgpulse(2.0*pwC,one,2.0e-6,0.0);
	decrgpulse(pwC,zero,2.0e-6,0.0);
        if (tau1>0.001)
        {
         zgradpulse(-gzlvlr,(0.8*tau1/2.0 - 2.0*GRADIENT_DELAY ));
         delay(0.2*tau1/2.0 );
        }
        else
         if ((tau1/2.0)>0.0) delay(tau1/2.0);
    }
    else
    {
        if (tau1>0.001)
        {
         zgradpulse(gzlvlr,(0.8*tau1 - 2.0*GRADIENT_DELAY));
         delay(0.2*tau1);
         zgradpulse(-gzlvlr,(0.8*tau1 - 2.0*GRADIENT_DELAY ));
         delay(0.2*tau1);
        }
        else
         delay(tau1);
     }

 
   	rgpulse(pw,t2,0.0,0.0);
  	txphase(zero);
	dec2phase(zero);

	zgradpulse(gzlvl3,gt3);
  	delay(taub - gt3 - 2.0*GRADIENT_DELAY);

  	sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);
  	dec2phase(t3);

	zgradpulse(gzlvl3,gt3);
	delay(taub - gt3 - 2.0*GRADIENT_DELAY);

	sim3pulse(pw,0.0,pwN,zero,zero,t3,0.0,0.0);
	txphase(t5);
	dec2phase(zero);
        obspower(tpwrs);
        obspwrf(finepwrf);

        zgradpulse(gzlvl4,gt4);
	delay(taub - gt4 - 4.0*POWER_DELAY - rof1 - 2.0*GRADIENT_DELAY - pwHs);

	shaped_pulse("H2Osinc",pwHs,t5,0.0,0.0);
	txphase(t7);
        obspower(tpwr);
        obspwrf(4095.0);

        sim3pulse(2.0*pw,0.0e-6,2.0*pwN,t7,zero,zero,rof1,rof1);
	txphase(t5);
        obspwrf(finepwrf);
        obspower(tpwrs);

	shaped_pulse("H2Osinc",pwHs,t5,0.0,0.0);
	txphase(zero);
	obspower(tpwr);
        obspwrf(4095.0);

	zgradpulse(gzlvl4,gt4);
	delay(taub - gt4 - 6.0*POWER_DELAY - rof1 - 2.0*GRADIENT_DELAY 
		- 0.5*(pwN-pw)	- pwHs);

  	dec2rgpulse(pwN,zero,0.0,0.0);
	decpower(dpwr);
	dec2power(dpwr2);
	
/* acquire data */
status(C);
     	setreceiver(t6);
}
