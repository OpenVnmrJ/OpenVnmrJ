/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_HCN.c

    This pulse sequence will allow one to perform the following experiment:

    2D or 3D HCN experiment to correlate anomeric H1' protons and H6/H8 base
    protons with their one-bond C1' and C6/C8 carbons and two-bond-connected
    base nitrogens (N1/N9). 

                      NOTE dof MUST BE SET AT 110ppm ALWAYS
                      NOTE dof2 MUST BE SET AT 200ppm ALWAYS


    pulse sequence (MQ-HCN): 	Marino et al, JACS, 114, 10663 (1992)
    pulse sequence (TROSY-HCN): unpublished Peter Lukavsky, Stanford, june, 1999
				very similar to Riek et al, JACS 2001,123,658
     
	2D-PLANE-PROCESSING:
	t2 (nonSE): 3D mqHCN	wft2d('ni2',1,0,0,0,0,0,-1,0)
	t2 (TROSY): 3D mqHCN	wft2d('ni2',1,0,-1,0,0,1,0,1)

	AROonly:
	Use with TROSY !!
	For H6/H8 to N1/N9 correlation only (slightly better S/N for aromatics).
	timeT=28ms and JCH=190Hz was best.
	sw=10ppm, sw1=40ppm, sw2=30ppm

	RIBonly:
	Use with MQ-HCN only!!
	For H1' to N1/N9 correlation only (slightly better S/N for ribose).
	timeT=22ms and JCH=180Hz was best.
	sw=10ppm, sw1=40ppm, sw2=20ppm

	H1'-BASE (MQ or TROSY):
	timeT=25ms and JCH=180Hz was best.
	sw=10ppm, sw1=40ppm, sw2=70ppm

	timeT should be set to ~25ms. (total time 2*timeT 36-60ms)
	JCH should be set to 180-200Hz.


        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        @                                                      @
        @   Written for RnaPack by Peter Lukavsky (06/02).     @
        @                                                      @
        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

   Modified simultaneous hard 13C/15N 180's to be 3dB lower in power (GG. 12/04)

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

static int   phi3[8]  = {0,0,0,0,2,2,2,2},
	     phi1[2]  = {0,2},
	     phi2[4]  = {0,0,2,2},
	     phx[1]   = {0},
             ph_y[1]  = {3},
             rec[8]   = {0,2,2,0,2,0,0,2};

static double   d2_init=0.0, d3_init=0.0;


void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],                    /* Flag to start t2 @ halfdwell */
            wet[MAXSTR],      /* Flag to select optional WET water suppression */
	    SHAPE[MAXSTR],
	    TROSY[MAXSTR],
	    AROonly[MAXSTR],
	    RIBonly[MAXSTR],
            rna_stCshape[MAXSTR],     /* calls sech/tanh pulses from shapelib */
            rna_stNshape[MAXSTR],     /* calls sech/tanh pulses from shapelib */
            rna_stCdec[MAXSTR],        /* calls STUD+ waveforms from shapelib */
            STUD[MAXSTR];   /* apply automatically calculated STUD decoupling */

 
int         t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,                         /* used for states tppi in t2 */
	    icosel,
            ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
            tau2,                                                /*  t2 delay */
	    lambda = 0.94/(4.0*getval("JCH")), 	   /* 1/4J H1 evolution delay */
	    timeT = getval("timeT"),		       /* constant time delay */
        

        pwClvl = getval("pwClvl"),              /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	compC = getval("compC"),  /* adjustment for C13 amplifier compression */
        rfC,                      /* maximum fine power when using pwC pulses */
  	dofa,                                       /* dof shifted to 115 ppm */


        pwNlvl = getval("pwNlvl"),                    /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
        compN = getval("compN"),  /* adjustment for N15 amplifier compression */
        rfN,                      /* maximum fine power when using pwN pulses */
        dof2a,                                     /* dof2 shifted to 155 ppm */

            stdmf,                                 /* dmf for STUD decoupling */
/* STUD+ waveforms automatically calculated by macro "rna_cal" */
/* and string parameter rna_stCdec calls them from your shapelib. */
   studlvl,                              /* coarse power for STUD+ decoupling */
   rf30 = getval("rf30"),                         /* rf in Hz for 30ppm STUD+ */
   dmf30 = getval("dmf30"),                            /* dmf for 30ppm STUD+ */
   rf80 = getval("rf80"),                         /* rf in Hz for 80ppm STUD+ */
   dmf80 = getval("dmf80"),                            /* dmf for 80ppm STUD+ */

/* Sech/tanh inversion pulses automatically calculated by macro "rna_cal"     */
/* and string parameter rna_stCshape calls them from your shapelib.           */
   rfstC = 0.0,          /* fine power for the rna_stCshape pulse, initialised */
   rfstN = 0.0,          /* fine power for the rna_stNshape pulse, initialised */

	sw1 = getval("sw1"),
        sw2 = getval("sw2"),

        grecov = getval("grecov"),

        gt1 = getval("gt1"),                   /* coherence pathway gradients */
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),

	gt3 = getval("gt3"),                               /* other gradients */
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5"),
	gzlvlr = getval("gzlvlr");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("wet",wet);
    getstr("STUD",STUD);
    getstr("SHAPE",SHAPE);
    getstr("TROSY",TROSY);
    getstr("AROonly",AROonly);
    getstr("RIBonly",RIBonly);


/*   LOAD PHASE TABLE    */

  if (TROSY[A] == 'y')
   {
        settable(t3,1,phx);
        settable(t8,2,phi1);
        settable(t5,4,phi2);
        settable(t6,8,phi3);
        settable(t9,1,phx);
        settable(t10,1,ph_y);
        settable(t11,8,rec);
   }
  else
   {
	settable(t3,8,phi3);
	settable(t1,2,phi1);
	settable(t2,4,phi2);
	settable(t4,1,phx);
	settable(t11,8,rec);
   }


/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses */
        rfC = 4095.0;

/* maximum fine power for pwN pulses */
        rfN = 4095.0;

  if ( RIBonly[A]=='y' )
   {
/*  DOF centered on 90ppm (C1') */
        dofa = dof - 20.0*dfrq;

        /* 30 ppm STUD+ decoupling */
        strcpy(rna_stCdec, "rna_stCdec30");
        stdmf = dmf30;
        studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf30);
        studlvl = (int) (studlvl + 0.5);

        /* 30ppm sech/tanh inversion */
        rfstC = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));
        rfstC = (int) (rfstC + 0.5);
        strcpy(rna_stCshape, "rna_stC30");
   }

  else if ( AROonly[A]=='y' )
   {
/*  DOF centered on 135ppm (C8/6) */
        dofa = dof + 25.0*dfrq;

        /* 30 ppm STUD+ decoupling */
        strcpy(rna_stCdec, "rna_stCdec30");
        stdmf = dmf30;
        studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf30);
        studlvl = (int) (studlvl + 0.5);

        /* 30ppm sech/tanh inversion */
        rfstC = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));
        rfstC = (int) (rfstC + 0.5);
        strcpy(rna_stCshape, "rna_stC30");
   }

  else
   {
/*  DOF centered on 115ppm (C1'-C8/6) */
	dofa = dof + 5.0*dfrq;

        /* 80 ppm STUD+ decoupling */
        strcpy(rna_stCdec, "rna_stCdec80");
        stdmf = dmf80;
        studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
        studlvl = (int) (studlvl + 0.5);

        /* 60ppm sech/tanh inversion */
        rfstC = (compC*4095.0*pwC*4000.0*sqrt((9.0*sfrq/600+3.85)/0.41));
        rfstC = (int) (rfstC + 0.5);
        strcpy(rna_stCshape, "rna_stC60");
   }

/*  DOF2 centered on 155ppm (N9/N1) */
        dof2a = dof2 - 45.0*dfrq2;

        /* 50ppm sech/tanh inversion */
        rfstN = (compN*4095.0*pwN*4000.0*sqrt((3.0*sfrq/600+3.85)/0.41));
        rfstN = (int) (rfstN + 0.5);
        strcpy(rna_stNshape, "rna_stN50");


/* CHECK VALIDITY OF PARAMETER RANGES */

  if (ni2/(8*sw2) > (timeT/4 - pw - 0.5e-3))
  { text_error( " ni2 is too big. Make ni2 equal to %d or less.\n",
      ((int)((timeT/4 - pw - 0.5e-3)*8*sw2)) ); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if ((SHAPE[A]=='n') && ((AROonly[A] == 'y' || RIBonly[A] == 'y')))
  { text_error("AROonly or RIBonly requires SHAPE='y'"); psg_abort(1); }

  if ((AROonly[A] == 'y' && RIBonly[A] == 'y'))
  { text_error("Choose either AROonly or RIBonly !"); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( (((dm[C] == 'y') && (dm2[C] == 'y')) && (STUD[A] == 'y')) )
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' if STUD='y'"); psg_abort(1);}

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }

  if( (pw > 20.0e-6) && (tpwr > 56) )
  { text_error("don't fry the probe, pw too high ! "); psg_abort(1); }

  if( (pwC > 40.0e-6) && (pwClvl > 56) )
  { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }

  if( (pwN > 100.0e-6) && (pwNlvl > 56) )
  { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 3D data, States-Haberkorn element */

	icosel = -1;

  if (TROSY[A] == 'y')
   {
        icosel = 1;
    if (phase1 == 2)
        tsadd(t5,1,4);
    if (phase2 == 2)
        {
        tsadd(t10,2,4);
        icosel = -1;
        }
   }
  else
   {
    if (phase1 == 2) 
	tsadd(t2,1,4);
    if (phase2 == 2)
        tsadd(t4,1,4);
   }


/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;

/*  Set up f2180  */

    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 1.0))
        { tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

  if ( TROSY[A] == 'y' )
   {
   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t5,2,4); tsadd(t11,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2)
        { tsadd(t8,2,4); tsadd(t11,2,4); }
   }
  else
   {
   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t11,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2)
        { tsadd(t4,2,4); tsadd(t11,2,4); }
   }


/* BEGIN PULSE SEQUENCE */

status(A);
	obsoffset(tof);
	decoffset(dofa);
	dec2offset(dof2a);
	obspower(tpwr);
	decpower(pwClvl);
	decpwrf(rfC);
 	dec2power(pwNlvl);
	dec2pwrf(rfN);
	txphase(zero);
	decphase(zero);
        dec2phase(zero);

	delay(d1);

   if (wet[A] == 'y')
    {
     obsoffset(tof);
     Wet4(zero,one);             /* WET solvent suppression */
     delay(1.0e-4);
    }
	obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
	rcvroff();

  if ( TROSY[A]=='y' )
   {
	txphase(t3);
	delay(5.0e-4);
   }
  else
   {
        dec2rgpulse(pwN, zero, 0.0, 0.0);	/*destroy N15 and C13 magnetization*/
        decrgpulse(pwC, zero, 0.0, 0.0);
        zgradpulse(gzlvl0, 0.5e-3);
        delay(grecov/2);
        dec2rgpulse(pwN, one, 0.0, 0.0);
        decrgpulse(pwC, one, 0.0, 0.0);
        zgradpulse(0.7*gzlvl0, 0.5e-3);
        txphase(t3);
        delay(5.0e-4);
   }

   	rgpulse(pw,t3,0.0,0.0);                 /* 1H pulse excitation */
	txphase(zero);
   	decphase(zero);

	zgradpulse(gzlvl5, gt5);
  if (SHAPE[A] == 'y')
   {
	decpwrf(rfstC);
	delay(lambda - gt5 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

  if ( TROSY[A]=='y' )
   {
	txphase(one);
     /* coupling evol reduced by 140us using rna_stC pulses. Also WFG2_START_DELAY*/
        simshaped_pulse("", rna_stCshape, 2.0*pw, 1.0e-3, one, zero, 0.0, 0.0);
   }
  else
   {
     /* coupling evol reduced by 140us using rna_stC pulses. Also WFG2_START_DELAY*/
        simshaped_pulse("", rna_stCshape, 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
   }

	if (TROSY[A] == 'y')
	{
	txphase(one);
	decphase(t8);
	}
	else
	{
   	txphase(zero);
	decphase(t1);
	}

	zgradpulse(gzlvl5, gt5);
	decpwrf(rfC);
	delay(lambda - gt5 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);
   }
  else
   {
        delay(lambda - gt5);
  if ( TROSY[A]=='y' )
   {
        txphase(one);
        simpulse(2.0*pw, 2*pwC, one, zero, 0.0, 0.0);
   }
  else
        simpulse(2.0*pw, 2*pwC, zero, zero, 0.0, 0.0);

        if (TROSY[A] == 'y')
        {
        txphase(one);
        decphase(t8);
        }
	else
	{
        txphase(zero);
        decphase(t1);
	}
        zgradpulse(gzlvl5, gt5);
        delay(lambda - gt5);
   }

  if (TROSY[A]=='y')
   {
        simpulse(pw, pwC, one, t8, 0.0, 0.0);
        txphase(zero);
    if(SHAPE[A]=='y')
     {
        dec2pwrf(rfstN);
        decpower(pwClvl-3.0);
        delay(timeT/2 - 0.5e-3 - WFG2_START_DELAY);

        sim3shaped_pulse("", "", rna_stNshape, 0.0, 2.0*pwC*1.4, 1.0e-3, zero, zero, zero, 0.0, 0.0);
        decphase(one);
        dec2phase(t5);
        dec2pwrf(rfN); decpower(pwClvl);

        delay(timeT/2 - 0.5e-3 - WFG2_START_DELAY);
     }
    else
     {
        delay(timeT/2 - pwN-2.0*POWER_DELAY);
        decpower(pwClvl-3.0); dec2power(pwNlvl-3.0);
        sim3pulse(0.0, 2*pwC*1.4, 2*pwN*1.4, zero, zero, zero, 0.0, 0.0);
        decphase(one);
        dec2phase(t5);
        decpower(pwClvl); dec2power(pwNlvl);

        delay(timeT/2 - pwN-2.0*POWER_DELAY);
     }
        decrgpulse(pwC, one, 0.0, 0.0);
        decphase(zero);

        zgradpulse(gzlvl3, gt3);
        delay(grecov);

        dec2rgpulse(pwN, t5, 0.0, 0.0);
        dec2phase(zero);

    if (tau1 > (2.0*GRADIENT_DELAY + pwC + 2.0*pwN/PI))
     {
      if (tau1>0.001)
      {
       zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwC - 2.0*pwN/PI));
       delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwC - 2.0*pwN/PI));
      }
      else
       delay(tau1  - pwC - 2.0*pwN/PI);
      decrgpulse(2.0*pwC, zero, 0.0, 0.0);
      if (tau1>0.001)
      {
       zgradpulse(-gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwC - 2.0*pwN/PI));
       delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwC - 2.0*pwN/PI));
      }
      else
       delay(tau1  - pwC - 2.0*pwN/PI);
     }
    else if (tau1 > (4.0*pwN/PI))
      delay(2.0*tau1 - 4.0*pwN/PI);

        dec2rgpulse(pwN, zero, 0.0, 0.0);
        decphase(t6);
        zgradpulse(gzlvl3,gt3);
        delay(grecov);

        decrgpulse(pwC, t6, 0.0, 0.0);
        decphase(zero);
    if(SHAPE[A]=='y')
     {
        dec2pwrf(rfstN); decpower(pwClvl-3.0);

        delay(timeT/2 - lambda - 0.5e-3 - WFG2_START_DELAY - tau2);
 
        sim3shaped_pulse("", "", rna_stNshape, 0.0, 2.0*1.4*pwC, 1.0e-3, zero, zero, zero, 0.0, 0.0);
        txphase(t9);

        dec2pwrf(rfN); decpower(pwClvl);

        zgradpulse(gzlvl1, gt1);         /* 2.0*GRADIENT_DELAY */
        delay(timeT/2 - lambda - 0.5e-3 - gt1 - 2.0*GRADIENT_DELAY - WFG2_START_DELAY);

        delay(tau2);
     }
    else
     {
        delay(timeT/2 - lambda - pwN - tau2-2.0*POWER_DELAY);
        decpower(pwClvl-3.0); dec2power(pwNlvl-3.0);
        sim3pulse(0.0, 2*pwC*1.4, 2*pwN*1.4, zero, zero, zero, 0.0, 0.0);
        decpower(pwClvl); dec2power(pwNlvl);
        txphase(t9);

        zgradpulse(gzlvl1, gt1);         /* 2.0*GRADIENT_DELAY */
        delay(timeT/2 - lambda - pwN - gt1 - 2.0*POWER_DELAY - 2.0*GRADIENT_DELAY);

        delay(tau2);
     }

        rgpulse(pw, t9, 0.0, 0.0);
        txphase(zero);
        decphase(zero);

        zgradpulse(gzlvl5, gt5);
        delay(lambda - gt5 - 1.5*pwC);

        simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
        txphase(t10);

        zgradpulse(gzlvl5, gt5);
        delay(lambda - gt5 - 1.5*pwC);

        simpulse(pw, pwC, t10, zero, 0.0, 0.0);
        txphase(zero);

        delay(lambda - 1.5*pwC);

        simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(icosel*gzlvl2, gt1/4.0);
        delay(lambda - 1.5*pwC - gt1/4 - 2*POWER_DELAY);

   }
  else		/* acutal MQ-HCN with options */
   {
 	decrgpulse(pwC, t1, 0.0, 0.0);
	decphase(zero);

	delay(timeT/4 - pw);

	rgpulse(2*pw,zero,0.0,0.0);

  if(SHAPE[A]=='y')
  {
        dec2pwrf(rfstN); decpower(pwClvl-3.0);

        delay(timeT/4 - pw - 0.5e-3 - WFG2_START_DELAY);

        sim3shaped_pulse("", "", rna_stNshape, 0.0, 2.0*1.4*pwC, 1.0e-3, zero, zero, zero, 0.0, 0.0);
        decphase(one);
        dec2phase(t2);

        dec2pwrf(rfN); decpower(pwClvl);

        delay(timeT/4 - pw - 0.5e-3 - WFG2_START_DELAY);
   }
  else
   {
	delay(timeT/4 - pw - pwN-2.0*POWER_DELAY);
        decpower(pwClvl-3.0); dec2power(pwNlvl-3.0);
	sim3pulse(0.0, 2*pwC*1.4, 2*pwN*1.4, zero, zero, zero, 0.0, 0.0);
        decpower(pwClvl); dec2power(pwNlvl);
	decphase(one);
	dec2phase(t2);

        delay(timeT/4 -  pw - pwN - 2.0*POWER_DELAY);
   }

        rgpulse(2*pw,zero,0.0,0.0);

        delay(timeT/4 - pw);

	simpulse(pw, pwC, one, one, 0.0, 0.0);
	txphase(zero);
        decphase(zero);

        zgradpulse(gzlvl3, gt3);
	delay(grecov);

	dec2rgpulse(pwN, t2, 0.0, 0.0);
	dec2phase(zero);

  if (tau1 > (2.0*GRADIENT_DELAY + pwC + 2.0*pwN/PI))
   {
    if (tau1>0.001)
    {
     zgradpulse(gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwC - 2.0*pwN/PI));
     delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwC - 2.0*pwN/PI));
    }
    else
     delay(tau1  - pwC - 2.0*pwN/PI);
    decrgpulse(2.0*pwC, zero, 0.0, 0.0);
    if (tau1>0.001)
    {
     zgradpulse(-gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwC - 2.0*pwN/PI));
     delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwC - 2.0*pwN/PI));
    }
    else
     delay(tau1  - pwC - 2.0*pwN/PI);
   }
  else if (tau1 > (4.0*pwN/PI))
    delay(2.0*tau1 - 4.0*pwN/PI);

	dec2rgpulse(pwN, zero, 0.0, 0.0);
	txphase(one);
        decphase(one);

        zgradpulse(gzlvl3,gt3);
        delay(grecov);

	simpulse(pw, pwC, one, one, 0.0, 0.0);
	txphase(zero);
	decphase(zero);

        delay(timeT/4 -  pw + tau2/2);

        rgpulse(2*pw,zero,0.0,0.0);

  if(SHAPE[A]=='y')
  {
        dec2pwrf(rfstN); decpower(pwClvl-3.0);

        delay(timeT/4 - pw - 0.5e-3 - WFG2_START_DELAY + tau2/2);

        sim3shaped_pulse("", "", rna_stNshape, 0.0, 2.0*1.4*pwC, 1.0e-3, zero, zero, zero, 0.0, 0.0);
        decphase(t4);

        dec2pwrf(rfN); decpower(pwClvl);

        delay(timeT/4 - pw - 0.5e-3 - WFG2_START_DELAY - tau2/2);
   }
  else
   {
        delay(timeT/4 - pw - pwN + tau2/2 -2.0*POWER_DELAY);
        decpower(pwClvl-3.0); dec2power(pwNlvl-3.0);
        sim3pulse(0.0, 2*pwC*1.4, 2*pwN*1.4, zero, zero, zero, 0.0, 0.0);
        decpower(pwClvl); dec2power(pwNlvl);
        decphase(t4);

        delay(timeT/4 - pw - pwN - tau2/2 - 2.0*POWER_DELAY);
   }

        rgpulse(2*pw,zero,0.0,0.0);

        delay(timeT/4 - pw - tau2/2);

	decrgpulse(pwC, t4, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        delay(lambda - gt5);

        simpulse(2.0*pw, 2.0*pwC, zero, t4, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        delay(lambda - gt5 - 2*POWER_DELAY);
   }

if ((STUD[A]=='y') && (dm[C] == 'y'))
        {decpower(studlvl);
         decprgon(rna_stCdec, 1.0/stdmf, 1.0);
         decon();}
else    {decpower(dpwr);
         dec2power(dpwr2);
         status(C);}

	setreceiver(t11);
}		 

