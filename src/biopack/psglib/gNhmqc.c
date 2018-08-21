/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNhmqc.c


	This pulse sequence will allow one to perform the following experiment:

    Gradient HMQC for N15 in D2O, or in H2O employing jump-return pulses

                      NOTE: dof2 MUST BE SET AT 120ppm 

	                  CHOICE OF DECOUPLING AND 2D MODES

    JNH is usually set to the true coupling constant value. The 1/2JNH
    delay is set to 5.26ms (95Hz) as default value and can be changed
    by setting JNH.

    Set dm = 'nny', dmm = 'ccg' for C13 garp1-decoupling.
    Set dm2 = 'nny', dmm2 = 'ccg' for N15 garp1-decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].

    No half-dwell time option. Phase suggestions: lp1=0 rp1=-180 with first
    two complex points in t1 linear predicted to remove intensity distortions
    coming from finite length of jumpreturn. Phasing of lp1 in F1 should be
    reasonalbly close to zero, permitting use of LP.
    
                  DETAILED INSTRUCTIONS FOR USE OF gNhmqc

    1. Apply the setup macro "gNhmqc".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    2. Splitting of resonances in the N15 dimension by C13 coupling in C13
       enriched samples can be removed by setting C13refoc='y'.

    3. Setting of GRADIENTS:

	-G0 not too strong (500us/2G)
	-G1 can be strong for good H2O-suppression (500us/16G)

    4. Sklenar's Radiation Damping suppression
       (J. Mag. Reson. A, 114, 132-135 (1995)

        -set gzlvlr=0.5G/cm

		sw=12ppm 
                sw1=40ppm
                diff=2ppm*sfrq Hz (~7ppm) 
                dof=110ppm
                dof2=120ppm

        diff-values should be optimized in that range!!

       Modified from rna_gNhqmc.c from RnaPack .
*/

#include <standard.h>

static double d2_init = 0.0;

static int	phi1[8]	= {0,0,1,1,2,2,3,3},
		phi2[2]	= {0,2},
		phi3[8]	= {2,2,3,3,0,0,1,1},
		phi4[1] = {0},
		rec[8]	= {0,2,2,0,0,2,2,0};

pulsesequence()

{

/* DECLARE VARIABLES */	

char	scuba[MAXSTR],	
	mess_flg[MAXSTR],
	jumpret[MAXSTR],
	C13refoc[MAXSTR];

int	phase,
	t1_counter;

double	hscuba,		/* length of 1/2 SCUBA delay 	*/
	taunh,		/* 1/4J(NH)			*/
	tofps,		/* water frequency		*/
	tpwrmess,	/* power level for Messerlie purge */
	dly_pg,		/* Messerlie purge delay	*/
	pwClvl,		/* power level for 13C hard pulse */
	pwNlvl,		/* power level for 15N hard pulse */
	pwC,		/* pulse width for 13C hard pulse */
	pwN,		/* pulse width for 15N hard pulse */
	JNH,		/* coupling for NH		*/
	diff,		/* freq diff H2O & exit center	*/
	tau1,		/* t1/2				*/
	grecov,		/* gradient recovery time 	*/
	gzlvl0,		/* level of grad. for purge	*/
	gzlvl1,		/* level of grad. for 1/2J	*/
	gt1,		/* grad. duration for 1/2J	*/
	gzlvlr;		/* level of RDt1-gradient (0.5G) */

/* LOAD VARIABLES */

	JNH	= getval("JNH");
	diff	= getval("diff");
	pwC	= getval("pwC");
	pwN	= getval("pwN");
	tofps	= getval("tofps");
	tpwrmess= getval("tpwrmess");
	dly_pg	= getval("dly_pg");
	pwClvl	= getval("pwClvl");
	pwNlvl	= getval("pwNlvl");
	hscuba	= getval("hscuba");
	phase	= (int)(getval("phase") + 0.5);
	sw1	= getval("sw1");
	at	= getval("at");
	grecov	= getval("grecov");
	gt1	= getval("gt1");
	gzlvl0	= getval("gzlvl0");
	gzlvl1	= getval("gzlvl1");
	gzlvlr  = getval("gzlvlr");

	getstr("scuba",scuba);
	getstr("mess_flg",mess_flg);
        getstr("jumpret",jumpret);
	getstr("C13refoc",C13refoc);

/* CHECK VALIDITY OF PARAMETER RANGE */

  if (dm[A] == 'y' || dm[B] == 'y')
   {
	printf(" dm must be 'nny' or 'nnn' ");
	psg_abort(1);
   }

  if ((dm[C] == 'y' || dm2[C] == 'y') && at > 0.21)
   {
	printf(" check at time! Don't fry probe \n");
	psg_abort(1);
   }

  if (dm2[A] == 'y' || dm2[B] == 'y')
   {
	printf(" dm2 must be 'nny' or 'nnn' ");
	psg_abort(1);
   }

  if ((satmode[A] == 'y') && (jumpret[A] == 'y'))
   {
	printf(" Choose EITHER presat (satmode='y') OR Jump-Return (jumpret='y') \n");
	psg_abort(1);
   }

  if (satpwr > 12)
   {
	printf(" satpwr must be less than 13 \n");
	psg_abort(1);
   }

  if (tpwrmess > 55)
   {
	printf(" tpwrmess must be less than 55 \n");
	psg_abort(1);
   }

  if (dly_pg > 0.010)
   {
	printf(" dly_pg must be less than 10msec \n");
	psg_abort(1);
   }

  if (dpwr > 50)
   {
	printf(" dpwr must be less than 49 \n");
	psg_abort(1);
   }

  if (dpwr2 > 50)
   {
	printf(" dpwr2 must be less than 46 \n");
	psg_abort(1);
   }

  if (gt1 > 5.0e-3)
   {
	printf(" gti must be less than 5msec \n");
	psg_abort(1);
   }

  if (gzlvl0 > 32768 || gzlvl1 > 32768 || gzlvl0 < -32768 || gzlvl1 < -32768)
   {
	printf(" gzlvli must be -32768 to 32768 (30G/cm) \n");
	psg_abort(1);
   }

  if (gzlvlr > 500 || gzlvlr < -500) 
   {
        printf(" RDt1-gzlvlr must be -500 to 500 (0.5G/cm) \n");
        psg_abort(1);
   }

/* LOAD PHASE PARAMETERS */

	settable(t1, 8, phi1);
	settable(t2, 2, phi2);
	settable(t3, 8, phi3);
	settable(t4, 1, phi4);
	settable(t5, 8, rec);

/* INITIALIZE VARIABLES */

	taunh = ((JNH != 0.0) ? 1/(4*(JNH)) : 2.78e-3);


/* Phase incrementation for hyper complex data in t1 */

  if (phase == 2)
	tsadd(t2, 1, 4);

/* Calculate modification to phase based on current t1 values: States - TPPI acquisition    */

  if( ix == 1)
	d2_init = d2;

	t1_counter = (int) ((d2 - d2_init)*sw1 + 0.5);

  if (t1_counter %2)
   {
	tsadd(t2, 2, 4);
	tsadd(t5, 2, 4);
   }

/* Set up so that you get (90, -180) phase correction in F1 */

	tau1 = d2;
	tau1 = tau1 -(4.0/PI)*pwN;
        tau1 = tau1 / 2.0;


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);

	obspower(satpwr);	/* Set power for presaturation	*/
	decpower(pwClvl);	/* Set DEC1 power to pwClvl	*/
	dec2power(pwNlvl);	/* Set DEC2 power to pwNlvl	*/

  if (mess_flg[A] == 'y')	/* Option for Messerlie purge */
   {
	obsoffset(tofps);
	obspower(tpwrmess);
	txphase(zero);
	xmtron();
	delay(dly_pg);
	xmtroff();
	txphase(one);
	xmtron();
	delay(dly_pg/1.62);
	xmtroff();
	obsoffset(tof);
	obspower(satpwr);
   }

  if (satmode[A] == 'y')		/* Presaturation Period */
   {
	txphase(zero);
	xmtron();		/* presaturation using transmitter	*/
	delay(d1);
	xmtroff();
	obspower(tpwr);		/* set power for hard pulse	*/

  if (scuba[A] == 'y')		/* SCUBA pulse	*/
   {
	hsdelay(hscuba);
	rgpulse(pw, zero, rof1, 0.0);
	rgpulse(2*pw, one, rof1, 0.0);
	rgpulse(pw, zero, rof1, 0.0);
	delay(hscuba);
   }
   }

  else 
   {
	obspower(tpwr);
	delay(d1);
   }

        dec2rgpulse(pwN,zero, 0.0, 0.0); /* destroy N15 magnetization */
        zgradpulse(gzlvl0, 0.5e-3);
        delay(1.0e-4);
        dec2rgpulse(pwN, one, 0.0, 0.0);
        zgradpulse(0.7*gzlvl0, 0.5e-3);
        decphase(t2);
        delay(grecov);

status(B);

  if (jumpret[A] == 'y')
   {
	rgpulse(pw, zero, rof1, rof1);	/* jump-return pulse */
	delay(0.25 / diff - 2.0*rof1);
	rgpulse(pw, two, rof1, rof1);
   }

  else
        rgpulse(pw, zero, rof1, rof1);

        zgradpulse(gzlvl1,gt1);
        delay(taunh - gt1 - 2.0*GRADIENT_DELAY );	/* delay = 1.0/2J(NH) */
        delay(taunh);
	dec2rgpulse(pwN, t2, 0.0, 0.0);
	dec2phase(t4);

 if (jumpret[A] == 'y')
  {
         tau1 = tau1-(pw+rof1+0.25/diff) ;
         if (tau1 < 0.0)  tau1 = 0.0;
      
        if (tau1 > 0.001)
         {
              zgradpulse(gzlvlr,(0.4*tau1-2.0*GRADIENT_DELAY));  /* delay = t1/2 */
              delay(0.1*tau1);
              zgradpulse(-gzlvlr,(0.4*tau1-2.0*GRADIENT_DELAY));
              delay(0.1*tau1);
         }
        else
             delay(tau1);				/* delay = t1/2 */
	rgpulse(pw, t1, rof1, rof1);

	if (C13refoc[A] == 'y')
	{
	delay((0.25 / diff) - pwC -rof1 );
	decrgpulse(2*pwC, zero, 0.0, 0.0);
	delay((0.25 / diff) - pwC -rof1) ;
   	}

  	else
   	{
	delay(0.25 / diff);
	delay(0.25 / diff);
   	}
	rgpulse(pw, t3, rof1, rof1);
        txphase(zero);
     if (tau1 > 0.001)
      {
        zgradpulse(gzlvlr,(0.4*tau1-2.0*GRADIENT_DELAY));  /* delay = t1/2 */
        delay(0.1*tau1);
        zgradpulse(-gzlvlr,(0.4*tau1-2.0*GRADIENT_DELAY));
        delay(0.1*tau1);
      }
     else
        delay(tau1);                               /* delay = t1/2 */
  }
else
  {
     if (C13refoc[A] == 'y')
         tau1 = tau1-(pwC+rof1) ;
     else
         tau1 = tau1-(pw+rof1) ;

     if (tau1 < 0.0)  tau1 = 0.0;
     if (tau1 > 0.001)
      {
        zgradpulse(gzlvlr,(0.4*tau1-2.0*GRADIENT_DELAY));  /* delay = t1/2 */
        delay(0.1*tau1);
        zgradpulse(-gzlvlr,(0.4*tau1-2.0*GRADIENT_DELAY));
        delay(0.1*tau1);
      }
     else
	delay(tau1);				/* delay = t1/2 */

     if (C13refoc[A] == 'y')
       simpulse(2*pw, 2*pwC, t1, zero, rof1, rof1);
     else
       rgpulse(2*pw, t1, rof1, rof1);
      txphase(zero);
	 
     if (tau1 > 0.001)
      {
        zgradpulse(gzlvlr,(0.4*tau1-2.0*GRADIENT_DELAY));  /* delay = t1/2 */
        delay(0.1*tau1);
        zgradpulse(-gzlvlr,(0.4*tau1-2.0*GRADIENT_DELAY));
        delay(0.1*tau1);
      }
     else
        delay(tau1);                               /* delay = t1/2 */
}
	dec2rgpulse(pwN, t4, 0.0, 0.0);

	zgradpulse(gzlvl1,gt1);
	decpower(dpwr);			/* low power for decoupling */
	dec2power(dpwr2);
	delay(taunh - gt1 - 2.0*GRADIENT_DELAY-2.0*POWER_DELAY);		/* delay = 1/2J(NH) */
        delay(taunh);
status(C);

	setreceiver(t5);
}
