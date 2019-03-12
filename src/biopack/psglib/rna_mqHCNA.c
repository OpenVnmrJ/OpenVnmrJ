/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_mqHCNA.c - autocalibrated version with pulse shaping "on fly".

    This pulse sequence will allow one to perform the following experiment:

    2D or 3D MQ-HCN experiment to correlate anomeric H1' protons and H6/H8 base
    protons with their one-bond C1' and C6/C8 carbons and two-bond-connected
    base nitrogens (N1/N9). 

                      NOTE dof MUST BE SET AT 110ppm ALWAYS
                      NOTE dof2 MUST BE SET AT 200ppm ALWAYS


    pulse sequence: 	Marino et al, JACS, 114, 10663 (1992)
     

	2D-PLANE-PROCESSING:
                            normal "states"  wft2da,wft2da('ni2')

	sw=10ppm, sw1=40ppm, sw2=70ppm
	ni=128 for 2D is OK
	ni2=64 for 3D(not tested yet)


        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        @                                                      @
        @   Written for RnaPack by Peter Lukavsky (06/99).     @
        @                                                      @
        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        Auto-calibrated version, E.Kupce, 27.08.2002.
*/



#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */  

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
             rec[8]   = {0,2,2,0,2,0,0,2};

static double   d2_init=0.0, d3_init=0.0;
static double   H1ofs=4.7, C13ofs=110.0, N15ofs=200.0, H2ofs=0.0;

static shape stC60, stN50;

void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],                    /* Flag to start t2 @ halfdwell */
            wet[MAXSTR],      /* Flag to select optional WET water suppression */
	    SHAPE[MAXSTR],
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
                               /* temporary Pbox parameters */
   bw, pws, ofs, ppm, nst,     /* bandwidth, pulsewidth, offset, ppm, # steps */
            stdmf,                                 /* dmf for STUD decoupling */
/* STUD+ waveforms automatically calculated by macro "rna_cal" */
/* and string parameter rna_stCdec calls them from your shapelib. */
   studlvl,                              /* coarse power for STUD+ decoupling */
   rf80 = getval("rf80"),                         /* rf in Hz for 80ppm STUD+ */
   dmf80 = getval("dmf80"),                            /* dmf for 80ppm STUD+ */

/* Sech/tanh inversion pulses automatically calculated by macro "rna_cal"     */
/* and string parameter rna_stCshape calls them from your shapelib.           */
   rfstC = 0.0,          /* fine power for the rna_stCshape pulse, initialised */
   rfstN = 0.0,          /* fine power for the rna_stNshape pulse, initialised */

	sw1 = getval("sw1"),
      sw2 = getval("sw2"),

        grecov = getval("grecov"),

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

/*   LOAD PHASE TABLE    */

	settable(t3,8,phi3);
	settable(t1,2,phi1);
	settable(t2,4,phi2);
	settable(t4,1,phx);
	settable(t11,8,rec);


/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses */
        rfC = 4095.0;

/* maximum fine power for pwN pulses */
        rfN = 4095.0;

/*  DOF centered on 115ppm (C1'-C8/6) */
	dofa = dof + 5.0*dfrq;
/*  DOF2 centered on 155ppm (N9/N1) */
        dof2a = dof2 - 45.0*dfrq2;

  setautocal();                        /* activate auto-calibration flags */ 

  if (autocal[0] == 'n') 
  {
        /* 60ppm sech/tanh inversion */
        rfstC = (compC*4095.0*pwC*4000.0*sqrt((9.0*sfrq/600+3.85)/0.41));
        rfstC = (int) (rfstC + 0.5);

        /* 50ppm sech/tanh inversion */
        rfstN = (compN*4095.0*pwN*4000.0*sqrt((3.0*sfrq/600+3.85)/0.41));
        rfstN = (int) (rfstN + 0.5);
  }
  else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
  {
    if(FIRST_FID)                                            /* call Pbox */
    {
      ppm = getval("dfrq"); 
      bw = 60.0*ppm; pws = 0.001; ofs = 0.0; nst = 500.0; 
      stC60 = pbox_makeA("rna_stC60", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
      bw = 50.0*ppm;  
      stN50 = pbox_makeA("rna_stN50", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
      if (dm3[B] == 'y') H2ofs = 3.2;
      ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
    }
    rfstC = stC60.pwrf; 
    rfstN = stN50.pwrf;
  }
        strcpy(rna_stNshape, "rna_stN50");
        strcpy(rna_stCshape, "rna_stC60");
        strcpy(rna_stCdec, "rna_stCdec80");
        stdmf = dmf80;
        studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
        studlvl = (int) (studlvl + 0.5);

/* CHECK VALIDITY OF PARAMETER RANGES */

  if (ni2/(8*sw2) > (timeT/4 - pw - 0.5e-3))
  { text_error( " ni2 is too big. Make ni2 equal to %d or less.\n",
      ((int)((timeT/4 - pw - 0.5e-3)*8*sw2)) ); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nny' "); psg_abort(1); }

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

    if (phase1 == 2) 
	tsadd(t2,1,4);
    if (phase2 == 2)
        tsadd(t4,1,4);


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

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t2,2,4); tsadd(t11,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2)
        { tsadd(t4,2,4); tsadd(t11,2,4); }


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
        dec2rgpulse(pwN, zero, 0.0, 0.0);
        decrgpulse(pwC, zero, 0.0, 0.0);
        zgradpulse(gzlvl0, 0.5e-3);
        delay(grecov/2);
        dec2rgpulse(pwN, one, 0.0, 0.0);
        decrgpulse(pwC, one, 0.0, 0.0);
        zgradpulse(0.7*gzlvl0, 0.5e-3);
        txphase(t3);
        delay(5.0e-4);

   	rgpulse(pw,t3,0.0,0.0);                 /* 1H pulse excitation */
	txphase(zero);
   	decphase(zero);

	zgradpulse(gzlvl5, gt5);
  if (SHAPE[A] == 'y')
   {
	decpwrf(rfstC);
	delay(lambda - gt5 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

     /* coupling evol reduced by 140us using rna_stC pulses. Also WFG2_START_DELAY*/
        simshaped_pulse("", rna_stCshape, 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);

   	txphase(zero);
	decphase(t1);

	zgradpulse(gzlvl5, gt5);
	decpwrf(rfC);
	delay(lambda - gt5 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);
   }
  else
   {
        delay(lambda - gt5);
        simpulse(2.0*pw, 2*pwC, zero, zero, 0.0, 0.0);
        txphase(zero);
        decphase(t1);
        zgradpulse(gzlvl5, gt5);
        delay(lambda - gt5);
   }

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
	delay(timeT/4 - pw - pwN -2.0*POWER_DELAY);
        decpower(pwClvl-3.0); dec2power(pwNlvl-3.0);
	sim3pulse(0.0, 2*pwC*1.4, 2*pwN*1.4, zero, zero, zero, 0.0, 0.0);
        decpower(pwClvl); dec2power(pwNlvl);
	decphase(one);
	dec2phase(t2);

        delay(timeT/4 -  pw - pwN);
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
     delay(tau1 - pwC - 2.0*pwN/PI);
    decrgpulse(2.0*pwC, zero, 0.0, 0.0);
    if (tau1>0.001)
    {
    zgradpulse(-gzlvlr, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwC - 2.0*pwN/PI));
    delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwC - 2.0*pwN/PI));
    }
    else
     delay(tau1 - pwC - 2.0*pwN/PI);
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

        delay(timeT/4 - pw - pwN - tau2/2 -2.0*POWER_DELAY);
   }

        rgpulse(2*pw,zero,0.0,0.0);

        delay(timeT/4 - pw - tau2/2);

	decrgpulse(pwC, t4, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        delay(lambda - gt5);

        simpulse(2.0*pw, 2.0*pwC, zero, t4, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        delay(lambda - gt5 - 2*POWER_DELAY);

if ((STUD[A]=='y') && (dm[C] == 'y'))
        {decpower(studlvl);
         decprgon(rna_stCdec, 1.0/stdmf, 1.0);
         decon();}
else    {decpower(dpwr);
         dec2power(dpwr2);
         status(C);}

	setreceiver(t11);
}		 

