/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* rna_HCP.c
    3-D proton-C13-P31 correlation experiment
    Marino et al JACS (1994) 116, 6472-73.
    Must set phase = 1,2 and phase2 = 1,2 for hypercomplex in t1 and t2.

    Typical acquisition times are  ms [t1],  ms [t2], and  ms [t3]
       with 128 complex [t1], 32 complex [t2], and 512 real [t3]. 

    Written by J. Puglisi 9-22-95

       EBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEB
       EB								 EB
       EB For qualitative measurement of torsion angles beta and epsilon EB
       EB Use with HPcosy for analysis.					 EB
       EB								 EB
       EBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEBEB

    Rewritten for RnaPack by Peter Lukavsky, Stanford 01/29/00.
    Use with D2O-sample, WATER='y' gives less signal!!
    Decoupling should be dm='nny' and dm2='nny' (P31-H3'/H5'/H5'' coupling!)




    */

#include <standard.h>

/* Chess - CHEmical Shift Selective Suppression */
void Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)
double pulsepower,duration,rx1,rx2,gzlvlw,gtw,gswet;
  codeint phase;
  char* pulseshape;
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
}

/* Wet4 - Water Elimination */
void Wet4(phaseA,phaseB)
  codeint phaseA,phaseB;
{
  double finepwr,gzlvlw,gtw,gswet,gswet2,wetpwr,wetpw,dz;
  char   wetshape[MAXSTR];
  getstr("wetshape",wetshape);    /* Selective pulse shape (base)  */
  wetpwr=getval("wetpwr");        /* User enters power for 90 deg. */
  wetpw=getval("wetpw");        /* User enters power for 90 deg. */
  dz=getval("dz");
  finepwr=wetpwr-(int)wetpwr;     /* Adjust power to 152 deg. pulse*/
  wetpwr=(double)((int)wetpwr);
  if (finepwr==0.0) {wetpwr=wetpwr+5; finepwr=4095.0; }
  else {wetpwr=wetpwr+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(wetpwr);         /* Set to low power level        */
  gzlvlw=getval("gzlvlw");      /* Z-Gradient level              */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
  gswet2=getval("gswet2");        /* Post-gradient stability delay */
  Chess(finepwr*0.5059,wetshape,wetpw,phaseA,20.0e-6,rof2,gzlvlw,gtw,gswet);
  Chess(finepwr*0.6298,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4304,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/8.0,gtw,gswet2);
  obspower(tpwr); obspwrf(tpwrf);     /* Reset to normal power level   */
  rcvron();
  delay(dz);
}
static int   phi1[1]  = {1},
             phi2[1]  = {1},
             phi3[2]  = {0,2},
             phi4[4]  = {1,1,3,3},
             rec[4]  = {0,2,2,0};



static double	d2_init = 0.0,
		d3_init = 0.0;

void pulsesequence()
{
/* DECLARE AND LOAD VARIABLES */
    
char        f1180[MAXSTR],                    /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],                    /* Flag to start t2 @ halfdwell */
            mag_flg[MAXSTR],  /* magic angle gradient                     */
            wet[MAXSTR],     /* Flag to select optional WET water suppression */
	    WATER[MAXSTR],
            rna_stCshape[MAXSTR];     /* calls sech/tanh pulses from shapelib */

int         t1_counter,                         /* used for states tppi in t1 */
            t2_counter;                         /* used for states tppi in t2 */

double      tau1,                                                 /* t1 delay */
	    tau2,						  /* t2 delay */
            lambdaC = 0.94/(4*getval("JCH")),      /* 1/4J H1 evolution delay */
	    timeT = getval("timeT"),        /* total constant time evolution
						and C13-P31 INEPT */

   pwClvl = getval("pwClvl"),                   /* coarse power for C13 pulse */
   pwC = getval("pwC"),               /* C13 90 degree pulse length at pwClvl */
   rfC,                           /* maximum fine power when using pwC pulses */
 compC = getval("compC"),         /* adjustment for C13 amplifier compression */

/* Sech/tanh inversion pulses automatically calculated by macro "rna_cal"     */
/* and string parameter rna_stCshape calls them from your shapelib.           */
   rfstC = 0.0,         /* fine power for the rna_stCshape pulse, initialised */
   dofa,      			       /* dof shifted to 75 for ribose region */

        pwPlvl = getval("pwPlvl"),                    /* power for N15 pulses */
        pwP = getval("pwP"),          /* N15 90 degree pulse length at pwNlvl */

        sw1 = getval("sw1"),
        sw2 = getval("sw2"),

 grecov = getval("grecov"),   /* Gradient recovery delay, typically 150-200us */
 gzcal = getval("gzcal"),        /* dac to G/cm conversion factor */


        gt0 = getval("gt0"),                               /* other gradients */
        gt3 = getval("gt3"),
        gt4 = getval("gt4"),
        gt5 = getval("gt5"),
        gzlvl0 = getval("gzlvl0"),
        gzlvl3 = getval("gzlvl3"),
        gzlvl4 = getval("gzlvl4"),
        gzlvl5 = getval("gzlvl5");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("WATER",WATER);
    getstr("mag_flg",mag_flg);
    getstr("wet",wet);


/*   LOAD PHASE TABLE    */

        settable(t1,1,phi1);
        settable(t2,1,phi2);
        settable(t3,2,phi3);
        settable(t4,4,phi4);
        settable(t11,4,rec);


/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses */
        rfC = 4095.0;

/*  DOF centered in RIBOSE region (75ppm) */
        dofa = dof - 35.0*dfrq;

        /* 50ppm sech/tanh inversion */
        rfstC = (compC*4095.0*pwC*4000.0*sqrt((7.5*sfrq/600+3.85)/0.41));
        rfstC = (int) (rfstC + 0.5);
        strcpy(rna_stCshape, "rna_stC50");

/* CHECK VALIDITY OF PARAMETER RANGE */

  if((dm[A] != 'n') || (dm[B] != 'n'))
  { printf("dm should be 'nnn' or 'nny' "); psg_abort(1); }

  if((dm2[A] != 'n') || (dm2[B] != 'n'))
  { printf("dm2 should be 'nnn'  or 'nny' "); psg_abort(1); }

  if( dpwr > 50 )
  { text_error("don't fry the probe, DPWR too large!  "); psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }

  if( (pw > 20.0e-6) && (tpwr > 56) )
  { text_error("don't fry the probe, pw too high ! "); psg_abort(1); }

  if( (pwC > 40.0e-6) && (pwClvl > 56) )
  { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }

  if( (pwP > 100.0e-6) && (pwPlvl > 56) )
  { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

        if (phase1 == 2)
        tsadd(t3,1,4);

        if (phase2 == 2)
        tsadd(t4,1,4);

 /*  Set up f1180  */
    tau1 = d2;
    if(f1180[A] == 'y')
        { tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;

/*  Set up f2180  */
    tau2 = d3;
    if(f2180[A] == 'y')
        { tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2/2.0;


/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t3,2,4); tsadd(t11,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2)
        { tsadd(t4,2,4); tsadd(t11,2,4); }


/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);

        obspower(tpwr);
        decpower(pwClvl);
        decpwrf(rfC);
        dec2power(pwPlvl);
        obsoffset(tof);
        decoffset(dofa);
        dec2offset(dof2);
        txphase(zero);
        decphase(zero);
        dec2phase(zero);

        delay(d1);

  if (wet[A] == 'y')
     Wet4(zero,one);

	obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
        rcvroff();

        dec2rgpulse(pwP, zero, 0.0, 0.0);
        decrgpulse(pwC, zero, 0.0, 0.0);
        zgradpulse(gzlvl0, 0.5e-3);
        delay(grecov/2);
        dec2rgpulse(pwP, one, 0.0, 0.0);
        decrgpulse(pwC, one, 0.0, 0.0);
        zgradpulse(0.7*gzlvl0, 0.5e-3);
        delay(5.0e-4);

status(B);

	rgpulse(pw,zero,rof1,0.0);                /* First 1H 90 degree pulse     */
        decphase(zero);

        zgradpulse(gzlvl5, gt5);
        delay(lambdaC - gt5);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
	txphase(t1);

	zgradpulse(gzlvl5, gt5);
	delay(lambdaC - gt5);


if (WATER[A]=='y')
{
        rgpulse(pw, t1, 0.0, 0.0);

                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl3, gt3);
                }
                else
                {
                   zgradpulse(gzlvl3, gt3);
                }

        decphase(zero);
        delay(grecov);
        decrgpulse(pwC, zero, 0.0, 0.0);
}
else
{
simpulse(pw,pwC,t1,zero, 0.0, 0.0);
}

	zgradpulse(gzlvl0, gt0);
	delay(timeT/2 - gt0);

	sim3pulse(0.0, 2*pwC, 2*pwP, zero, zero, zero, 0.0, 0.0);
	decphase(t2);
	dec2phase(t3);

	zgradpulse(gzlvl0, gt0);
        delay(timeT/2 - gt0);

	sim3pulse(0.0, pwC, pwP, zero, t2, t3, 0.0, 0.0);
	decphase(t4);
        dec2phase(zero);

	delay(tau1 - 2/PI*pwP - pwC);
	simpulse(2*pw, 2*pwC, zero, zero, 0.0, 0.0);
	delay(tau1 - 2/PI*pwP - pwC);

	sim3pulse(0.0, pwC, pwP, zero, t4, zero, 0.0, 0.0);
	decphase(zero);

        delay(tau2 - 2/PI*pwC - pw);

	rgpulse(2*pw, zero, 0.0, 0.0);

	delay(timeT/2);

	sim3pulse(0.0, 2*pwC, 2*pwP, zero, zero, zero, 0.0, 0.0);

	delay(timeT/2 - tau2 - 2/PI*pwC - pw);

if (WATER[A]=='y')
{
        decrgpulse(pwC,zero,2*rof1,2*rof1);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl4, gt4);
                }
                else
                {
                   zgradpulse(gzlvl4, gt4);
                }
        rgpulse(pw,zero,grecov,2*rof1);
}
else
{
simpulse(pw,pwC,zero, zero, 0.0, 0.0);
}

        zgradpulse(gzlvl5, gt5);
        delay(lambdaC - gt5);

        simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
        txphase(t1);

        zgradpulse(gzlvl5, gt5);
        delay(lambdaC - gt5 - 2*POWER_DELAY);

        decpower(dpwr);                                        /* POWER_DELAY */
        dec2power(dpwr2);                                     /* POWER_DELAY */

status(C);

   setreceiver(t11);
  }
