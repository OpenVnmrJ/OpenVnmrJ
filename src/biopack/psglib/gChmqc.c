/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gChmqc.c


	This pulse sequence will allow one to perform the following experiment:

    Gradient HMQC for C13 in D2O or in H2O employing wet water suppression. 

	                  CHOICE OF DECOUPLING AND 2D MODES

    JCH is usually set to the true coupling constant value. The 1/2JCH
    delay is set to 3.57ms (140Hz) as default value and can be changed
    by setting JCH.

    Set dm = 'nny', dmm = 'ccg' for C13 garp1-decoupling.
    Set dm2 = 'nny', dmm2 = 'ccg' for N15 garp1-decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].

    The initial value of t1 is set to be started at halfdwell time.
    This will give 90, -180 phasing in f1.

    
                  DETAILED INSTRUCTIONS FOR USE OF gChmqc

    1. Apply the setup macro "gChmqc".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    2. Splitting of resonances in the C13 dimension by N15 coupling in N15
       enriched samples can be removed by setting N15refoc='y'.

    3. Setting of GRADIENTS:

	-G0 not too strong (500us/2G)
	-G1 can be strong for good H2O-suppression (500us/16G)

    4. SETUP: dof is changed automatically
	arom='y'
                        sw=10ppm
                        sw1=30ppm
                        dof=125ppm
                        dof2=120ppm
        aliph='y' 
                        sw=10ppm
                        sw1=90ppm
                        dof=85ppm
                        dof2=200ppm
    5. PROCESSING:  If f1180='n', the first point will be distorted if N15refoc='y'. This
       can be corrected by linear predicting the first point, e.g. use setlp1(512,256,1)
       to extend the data set from 256 to 512 points, while predicting the first point.
       In VNMR, fpmult1 can also be set before the 2D ft to scale the first point of the
       interferrogram.

        gChmqc.c derived from rna_gChmqc.c by Peter Lukavsky for RnaPack
*/

#include <standard.h>

static double d2_init = 0.0;

static int	phi1[8]	= {0,0,1,1,2,2,3,3},
		phi2[2]	= {0,2},
		phi3[8]	= {2,2,3,3,0,0,1,1},
		phi4[1] = {0},
		rec[8]	= {0,2,2,0,0,2,2,0};
/* Chess - CHEmical Shift Selective Suppression */
Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)
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
Wet4(double pulsepower, char* shape,double duration,codeint phaseA,codeint phaseB)
/*  double pulsepower,duration;
  codeint phaseA,phaseB;
  char* shape;*/
{
  double wetpw,finepwr,gzlvlw,gtw,gswet;
  gzlvlw=getval("gzlvlw"); gtw=getval("gtw"); gswet=getval("gswet");
  wetpw=getval("wetpw");
  finepwr=pulsepower-(int)pulsepower;     /* Adjust power to 152 deg. pulse*/
  pulsepower=(double)((int)pulsepower);             
  if (finepwr == 0.0) {pulsepower=pulsepower+5; finepwr=4095.0; }
  else {pulsepower=pulsepower+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(pulsepower);         /* Set to low power level        */
  Chess(finepwr*0.6452,shape,wetpw,phaseA,20.0e-6,rof1,gzlvlw,gtw,gswet);
  Chess(finepwr*0.5256,shape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4928,shape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,shape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/8.0,gtw,gswet);
  obspower(tpwr); obspwrf(tpwrf);  /* Reset to normal power level   */
  rcvron();
}

pulsesequence()
{

/* DECLARE VARIABLES */	

char	scuba[MAXSTR],	
	mess_flg[MAXSTR],
	wetshape[MAXSTR],      /* shape for optional WET water suppression */
	wet[MAXSTR],      /* Flag to select optional WET water suppression */
        autosoft[MAXSTR],     /* flag to do automatic wet power calculation*/
	arom[MAXSTR],
	aliph[MAXSTR],
        f1180[MAXSTR],
	N15refoc[MAXSTR];

int	phase,
	t1_counter;

double	hscuba,		/* length of 1/2 SCUBA delay 	*/
        wetpwr=getval("wetpwr"),         /* power for wet pulse          */
        wetpw=getval("wetpw"),          /* pulse width for wet pulse    */
        compH=getval("compH"),          /* compression for 1H at tpwr   */
        dz=getval("dz"),             /* wet WEFT delay               */
	tauch,		/* 1/2J(CH)			*/
	tpwrmess,	/* power level for Messerlie purge */
	dly_pg,		/* Messerlie purge delay	*/
	pwClvl,		/* power level for 13C hard pulse */
	pwNlvl,		/* power level for 15N hard pulse */
	pwC,		/* pulse width for 13C hard pulse */
	pwN,		/* pulse width for 15N hard pulse */
	JCH,		/* coupling for CH		*/
	dofa,		/* offset for aromatic/aliphatic*/
	tau1,		/* t1/2				*/
	grecov,		/* gradient recovery time 	*/
	gzlvl0,		/* level of grad. for purge	*/
	gzlvl5,		/* level of grad. for 1/2J	*/
	gt5;		/* grad. duration for 1/2J	*/

/* LOAD VARIABLES */

	JCH	= getval("JCH");
	pwC	= getval("pwC");
	pwN	= getval("pwN");
	satdly  = getval("satdly");
	tpwrmess= getval("tpwrmess");
	dly_pg	= getval("dly_pg");
	pwClvl	= getval("pwClvl");
	pwNlvl	= getval("pwNlvl");
	hscuba	= getval("hscuba");
	phase	= (int)(getval("phase") + 0.5);
	sw1	= getval("sw1");
	at	= getval("at");
        dofa    = getval("dof");
	grecov	= getval("grecov");
	gt5	= getval("gt5");
	gzlvl0	= getval("gzlvl0");
	gzlvl5	= getval("gzlvl5");

	getstr("f1180",f1180);
	getstr("scuba",scuba);
	getstr("mess_flg",mess_flg);
	getstr("wet",wet);
        getstr("wetshape",wetshape);
        getstr("autosoft",autosoft); 
	getstr("arom",arom);
	getstr("aliph",aliph);
	getstr("N15refoc",N15refoc);

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

  if ( ((arom[A]=='y') && (aliph[A]=='y')) )
   {
        printf(" Choose  ONE  of  arom='y'  OR  aliph='y' ");
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
	printf(" dpwr must be less than 48 \n");
	psg_abort(1);
   }

  if (dpwr2 > 50)
   {
	printf(" dpwr2 must be less than 46 \n");
	psg_abort(1);
   }

  if (gt5 > 5.0e-3)
   {
	printf(" gti must be less than 5msec \n");
	psg_abort(1);
   }

  if (gzlvl0 > 32768 || gzlvl5 > 32768 || gzlvl0 < -32768 || gzlvl5 < -32768)
   {
	printf(" gzlvli must be -32768 to 32768 (30G/cm) \n");
	psg_abort(1);
   }

/* LOAD PHASE PARAMETERS */

	settable(t1, 8, phi1);
	settable(t2, 2, phi2);
	settable(t3, 8, phi3);
	settable(t4, 1, phi4);
	settable(t5, 8, rec);

/* INITIALIZE VARIABLES */

	tauch = ((JCH != 0.0) ? 1/(2*(JCH)) : 3.57e-3);

/* AROMATIC-region setting of dof */
	if (arom[A] == 'y')
		dofa = dof + (125-35)*dfrq;

/* ALIPHATIC-region setting of dof */
	else if (aliph[A] == 'y')
		dofa = dof;

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

	tau1 = d2 - (4.0/PI)*pwC;

/* Optional half-dwell delay (produces (90, -180) phase correction in F1) */

     if (f1180[A] == 'y')
	tau1 += 1.0/(2.0*sw1);
	
  if(d2 > 0)
   {
         if (N15refoc[A] == 'y')
                tau1 -= 2*pwN;
         else
                tau1 -= 2*pw;
   }

  tau1 = tau1 / 2.0;
  if (tau1 < 0.0) tau1 = 0.0;


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);

	obspower(tpwr);  	/* Set power 	*/
	decpower(pwClvl);	/* Set DEC1 power to pwClvl	*/
	dec2power(pwNlvl);	/* Set DEC2 power to pwNlvl	*/

	decoffset(dofa);
	dec2offset(dof2);

	txphase(zero);
	decphase(zero);
	dec2phase(zero);

	delay(d1);

  if (mess_flg[A] == 'y')	/* Option for Messerlie purge */
   {
	obspower(tpwrmess);
        rgpulse(dly_pg,zero,rof1,0.0);
        rgpulse(dly_pg/1.62,one,0.0,rof1);
	obspower(tpwr);
   }

  if (satmode[A] == 'y')		/* Presaturation Period */
   {
        obspower(satpwr);
        rgpulse(satdly,zero,rof1,rof1);
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

  if (wet[A] == 'y')

     {
      if (autosoft[A] == 'y') 
       { 
           /* selective H2O one-lobe sinc pulse */
        wetpwr = tpwr - 20.0*log10(wetpw/(pw*compH*1.69));  /* sinc needs 1.69 times more */
        wetpwr = (int) (wetpwr +0.5);                       /* power than a square pulse */
        Wet4(wetpwr,"H2Osinc",wetpw,zero,one); delay(dz); 
       } 
      else
        Wet4(wetpwr,wetshape,wetpw,zero,one); delay(dz); 
      delay(1.0e-4);
      obspower(tpwr);
      rcvroff();
     }
	decrgpulse(pwC,zero, 0.0, 0.0);	/* destroy C13 magnetization */
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
	decrgpulse(pwC, one, 0.0, 0.0);
        zgradpulse(0.7*gzlvl0, 0.5e-3);
	decphase(t2);
	delay(grecov);

status(B);

	rgpulse(pw, zero, rof1, rof1);
	txphase(t1);

        zgradpulse(gzlvl5,gt5);
 
        delay(tauch - rof1 -gt5 - 2.0*GRADIENT_DELAY );	/* delay = 1.0/2J(CH) */
 
	decrgpulse(pwC, t2, 0.0, 0.0);
	decphase(t4);
        	delay(tau1);				/* delay = t1/2 */

                if (N15refoc[A] == 'y')
                 sim3pulse(2*pw, 0.0, 2*pwN, t1, zero, zero, 0.0, 0.0);
                else
                 rgpulse(2*pw, t1, 0.0, 0.0);
	 
                delay(tau1 );                               /* delay = t1/2 */
	decrgpulse(pwC, t4, 0.0, 0.0);

	zgradpulse(gzlvl5,gt5);
	decpower(dpwr);			/* low power for decoupling */
	dec2power(dpwr2);

	delay(tauch - gt5 - 2.0*GRADIENT_DELAY -2.0*POWER_DELAY); /* delay = 1/2J(CH) */

/* ACQUIRE DATA */

status(C);

	setreceiver(t5);
}
