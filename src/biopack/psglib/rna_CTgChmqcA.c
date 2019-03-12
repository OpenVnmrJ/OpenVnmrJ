/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_CTgChmqcA.c - autocalibrated version with pulse shaping "on fly".

    This pulse sequence will allow one to perform the following experiment:

    2D CT-HMQC or CT-HTQC experiment with optional N15 refocusing, options 
    for C13 decoupling and editing of spectral regions.

                      NOTE dof MUST BE SET AT 110ppm ALWAYS
                      NOTE dof2 MUST BE SET AT 200ppm ALWAYS


    pulse sequence: 	Marino et al, JACS, 114, 10663 (1992)
     

        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for C13 decoupling.
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for N15 decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.



          	  DETAILED INSTRUCTIONS FOR USE OF rna_CTgChmqc

         
    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro: 
                                      "printon man('RnaPack') printoff".
       These Detailed Instructions for rna_CTgChmqc.c may be printed using:
                                      "printon man('rna_CTgChmqc') printoff".
             
    2. Apply the setup macro "rna_CTgChmqc".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 110ppm, and N15 
       frequency on the aromatic N region (200 ppm).

    4. CHOICE OF C13 REGION and CTdelay:
       ribose='y' gives a spectrum of ribose/C5 resonances centered on dof=85ppm.
       Set CTdelay to 1/35Hz to 1/40Hz.			Set sw1 to 60ppm.

       aromatic='y' gives a spectrum of aromatic C2/C6/C8 groups.  dof is shifted
       automatically by the pulse sequence code to 145ppm.
       Set CTdelay to 1/70Hz to 1/80Hz.			Set sw1 to 30ppm.

       allC='y' gives a spectrum of ribose and aromatic resonances. dof is
       set by the code to 110ppm.                           
       Set CTdelay to 1/35Hz to 1/40Hz.			Set sw1 to 110ppm.

    5. N15 COUPLING:
       Splitting of resonances in the C13 dimension by N15 coupling in N15
       enriched samples is removed by setting N15refoc='y'.  No N15 RF is
       delivered in the pulse sequence if N15refoc='n'.  N15 parameters are
       listed in dg2.

    6. 1/4J DELAY TIMES:
       These are listed in dg/dg2 for possible change by the user. JCH is used
       to calculate the 1/4J time (lambda=0.94*1/4J) for H1 CH coupling evolution.
       Lambda is calculated a little lower (0.94) than the theoretical time to
       minimize relaxation. So for:
         ribose CH/CH2:           JCH=145Hz
         aromatic CH:             JCH=180Hz
         allC:                    JCH=160Hz

    7. SPECTRAL EDITING FOR DIFFERENT CHn GROUPS.
       CH2only='y' provides spectra of just CH2 groups by setting lambda to 1/2J
       to generate triple and single quantum coherence.

    8. STUD DECOUPLING.   SET STUD='y':
       Setting the flag STUD='y' overrides the decoupling parameters listed in
       dg2 and applies STUD+ decoupling instead.  In consequence it is easy
       to swap between the decoupling scheme you have been using to STUD+ for
       an easy comparison.  The STUD+ waveforms are calculated for your field
       strength at the time of RnaPack installation and RF power is
       calculated within the pulse sequence.  The calculations are for the most
       efficient conditions to cover 140ppm when allC='y' with all decoupled
       peaks being greater than 85% of ideal; 80ppm/90% for ribose='y' and
       aromatic='y'.  If you wish to compare different
       decoupling schemes, the power level used for STUD+ can be obtained from
       dps - subtract 3dB from that level to compare to decoupling schemes at
       a continuous RF level such as GARP.  The values of 85, 90, and 95% have
       been set to limit sample heating as much as possible.  If you wish to
       change STUD parameters, for example to increase the quality of decoupling
       (and RF heating) for 30ppm decoupling say, change the 95% level for the
       centerband, by changing the relevant number in the macro makeSTUDpp and
       rerun the macro (don't use 100% !!).  (At the time of writing STUD has
       been coded to use the coarse attenuator, because the fine attenuator
       is not presently included in the fail-safe calculation of decoupling
       power which aborts an experiment if the power is too high - this may
       lower STUD efficiency a little).


        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        @                                                      @
        @   Written for RnaPack by Peter Lukavsky (07/99).     @
        @                                                      @
        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        
        Auto-calibrated version, E.Kupce, 27.08.2002.
*/



#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */
  
/* Chess - CHEmical Shift Selective Suppression */
static void Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)
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
static void Wet4(pulsepower,wetshape,duration,phaseA,phaseB)
  double pulsepower,duration;
  codeint phaseA,phaseB;
  char* wetshape;
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
  Chess(finepwr*0.6452,wetshape,wetpw,phaseA,20.0e-6,rof1,gzlvlw,gtw,gswet);
  Chess(finepwr*0.5256,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4928,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/8.0,gtw,gswet);
  obspower(tpwr); obspwrf(tpwrf);  /* Reset to normal power level   */
  rcvron();
}



static int   phi1[2]  = {0,2},
	       phi2[4]  = {0,0,2,2},
	       phi3[8]  = {0,0,0,0,2,2,2,2},
	       rec1[2]  = {0,2},
             rec2[8]  = {0,2,2,0,2,0,0,2};

static double   d2_init=0.0;
static double   H1ofs=4.7, C13ofs=110.0, N15ofs=200.0, H2ofs=0.0;

static shape stC30, stC50, stC140;

void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            wet[MAXSTR],                  /* option for wet water-suppression */
            wetshape[MAXSTR],
            autosoft[MAXSTR],            /* option for auto-calc shape/power */
            SHAPE[MAXSTR],
            ribose[MAXSTR],                         /* ribose CHn groups only */
            aromatic[MAXSTR],                     /* aromatic CHn groups only */
            allC[MAXSTR],                       /* ribose and aromatic groups */
            CH2only[MAXSTR],                   /* spectrum of only CH2 groups */
            rna_stCshape[MAXSTR],     /* calls sech/tanh pulses from shapelib */
            rna_stCdec[MAXSTR],        /* calls STUD+ waveforms from shapelib */
            N15refoc[MAXSTR],                    /* N15 pulse in middle of t1 */
            STUD[MAXSTR];   /* apply automatically calculated STUD decoupling */

int         t1_counter;  		        /* used for states tppi in t1 */

double      tau1,         				         /*  t1 delay */
	    lambda = 0.94/(4.0*getval("JCH")), 	   /* 1/4J H1 evolution delay */
	    CTdelay = getval("CTdelay"),	       /* constant time delay */
            stdmf,                                 /* dmf for STUD decoupling */
                               /* temporary Pbox parameters */
        bw, pws, ofs, ppm, nst,  /* bandwidth, pulsewidth, offset, ppm, # steps */

        pwClvl = getval("pwClvl"),              /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
        rfC,                      /* maximum fine power when using pwC pulses */
 compC = getval("compC"),         /* adjustment for C13 amplifier compression */

/* Sech/tanh inversion pulses automatically calculated by macro "rna_cal"     */
/* and string parameter rna_stCshape calls them from your shapelib.           */
   rfst = 0.0,          /* fine power for the rna_stCshape pulse, initialised */
   dofa,       /* dof shifted to 85 or 145ppm for ribose and aromatic spectra */

/* STUD+ waveforms automatically calculated by macro "rna_cal" */
/* and string parameter rna_stCdec calls them from your shapelib. */
   studlvl,                              /* coarse power for STUD+ decoupling */
   rf140 = getval("rf140"),                      /* rf in Hz for 140ppm STUD+ */
   dmf140 = getval("dmf140"),                         /* dmf for 140ppm STUD+ */
   rf80 = getval("rf80"),                         /* rf in Hz for 80ppm STUD+ */
   dmf80 = getval("dmf80"),                            /* dmf for 80ppm STUD+ */
   rf30 = getval("rf30"),                         /* rf in Hz for 30ppm STUD+ */
   dmf30 = getval("dmf30"),                            /* dmf for 30ppm STUD+ */

        pwNlvl = getval("pwNlvl"),                    /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
        rfN,                      /* maximum fine power when using pwN pulses */

	pwZa,                            /* the largest of 2.0*pw and 2.0*pwN */

	sw1 = getval("sw1"),
        wetpw = getval("wetpw"),
        compH = getval("compH"),
        wetpwr= getval("wetpwr"),
        dz  = getval("dz"),

        grecov = getval("grecov"),

	gt5 = getval("gt5"),			           /* other gradients */
	gzlvl0 = getval("gzlvl0"),
	gzlvl5 = getval("gzlvl5");

    getstr("f1180",f1180);
    getstr("wet",wet);
    getstr("wetshape",wetshape);
    getstr("autosoft",autosoft);
    getstr("SHAPE",SHAPE);
    getstr("ribose",ribose);
    getstr("aromatic",aromatic);
    getstr("allC",allC);
    getstr("CH2only",CH2only);
    getstr("N15refoc",N15refoc);
    getstr("STUD",STUD);


/*   LOAD PHASE TABLE    */

	settable(t1,2,phi1);
	settable(t2,4,phi2);
  if (CH2only[A] == 'y')
   {	settable(t11,8,rec2);
	settable(t3,8,phi3); }
  else
   {	settable(t11,2,rec1); }


/*   INITIALIZE VARIABLES   */

/* optional refocusing of N15 coupling for N15 enriched samples */
  if (N15refoc[A]=='n')  pwN = 0.0;
  if (2.0*pw > 2.0*pwN) pwZa = 2.0*pw;
  else pwZa = 2.0*pwN;

/* maximum fine power for pwC pulses */
        rfC = 4095.0;

/* maximum fine power for pwN pulses */
        rfN = 4095.0;

  setautocal();                        /* activate auto-calibration flags */ 

  if (autocal[0] == 'n') 
  {
/*  RIBOSE spectrum only, centered on 85ppm. */
    if (ribose[A]=='y')
     {
        /* 50ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((7.5*sfrq/600+3.85)/0.41));
        rfst = (int) (rfst + 0.5);
     }

/*  AROMATIC spectrum only, centered on 145ppm */
     if (aromatic[A]=='y')
     {
        /* 30ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));
        rfst = (int) (rfst + 0.5);
     }

/*  TOTAL CARBON spectrum, centered on 110ppm */
     if (allC[A]=='y')
     {
        /* 140ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35));
        rfst = (int) (rfst + 0.5);
        if ( 1.0/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n",
            (1.0e6/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }
     }
   }
  else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
  {
    if(FIRST_FID)                                            /* call Pbox */
    {
      ppm = getval("dfrq"); pws = 0.001; ofs = 0.0; 
      bw = 30.0*ppm; nst = 500.0;    
      stC30 = pbox_makeA("rna_stC30", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
      bw = 50.0*ppm; 
      stC50 = pbox_makeA("rna_stC50", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
      bw = 140.0*ppm; nst = 1000.0; 
      stC140 = pbox_makeA("rna_stC140", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
      ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
    }
    if (aromatic[A]=='y') rfst = stC30.pwrf; 
    if (ribose[A]=='y')   rfst = stC50.pwrf; 
    if (allC[A]=='y')     rfst = stC140.pwrf; 
  }

  stdmf=dmf80; dofa=dof; studlvl=0.0;
  if (ribose[A]=='y')
  {
    dofa = dof - 25.0*dfrq;
    strcpy(rna_stCshape, "rna_stC50");
    strcpy(rna_stCdec, "wurst80");
    stdmf = dmf80;
    studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
    studlvl = (int) (studlvl + 0.5);
  }
  if (aromatic[A]=='y')
  {
    dofa = dof + 35*dfrq;
    strcpy(rna_stCshape, "rna_stC30");
    strcpy(rna_stCdec, "wurst30");
    stdmf = dmf30;
    studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf30);
    studlvl = (int) (studlvl + 0.5);
  }
  if (allC[A]=='y')
  {
    dofa = dof;
    strcpy(rna_stCshape, "rna_stC140");
    strcpy(rna_stCdec, "wurst140");
    stdmf = dmf140;
    studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf140);
    studlvl = (int) (studlvl + 0.5);
  }

/* CHECK VALIDITY OF PARAMETER RANGES */

  if (ni/sw1 > CTdelay/2)
  { text_error( " ni is too big. Make ni equal to %d or less.\n",
      ((int)(CTdelay/2*sw1)) ); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( (((dm[C] == 'y') && (dm2[C] == 'y')) && (STUD[A] == 'y')) )
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' if STUD='y'"); psg_abort(1); }

  if( (((dm[C] == 'y') && (dm2[C] == 'y')) && (allC[A] == 'y')) )
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' if allC='y'"); psg_abort(1); }

  if( dpwr > 50 )
  { text_error("don't fry the probe, DPWR too large!  "); psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }

  if( (pw > 20.0e-6) && (tpwr > 56) )
  { text_error("don't fry the probe, pw too high ! "); psg_abort(1); }

  if( (pwC > 40.0e-6) && (pwClvl > 56) )
  { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }

  if( (pwN > 100.0e-6) && (pwNlvl > 56) )
  { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }


/*  CHOICE OF PULSE SEQUENCE */

  if ( ((ribose[A]=='y') && (allC[A]=='y')) || ((ribose[A]=='y') && (aromatic[A]=='y'))
       || ((aromatic[A]=='y') && (allC[A]=='y')) )
   { text_error("Choose  ONE  of  ribose='y'  OR  aromatic='y'  OR  allC='y' ! "); psg_abort(1);}

  if ( (CH2only[A]=='y') && ((aromatic[A]=='y') || (allC[A]=='y')) )
    { text_error("Set  CH2only='n'  OR  ribose='y' ! "); psg_abort(1); }


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2) 
	tsadd(t1,1,4);

/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t1,2,4); tsadd(t11,2,4); }

/* BEGIN PULSE SEQUENCE */

status(A);
	rcvroff();
	obsoffset(tof);
	decoffset(dofa);
	dec2offset(dof2);
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
      if (autosoft[A] == 'y') 
       { 
           /* selective H2O one-lobe sinc pulse */
        wetpwr = tpwr - 20.0*log10(wetpw/(pw*compH*1.69));  /* sinc needs 1.69 times more */
        wetpwr = (int) (wetpwr +0.5);                       /* power than a square pulse */
        Wet4(wetpwr,"rna_H2Osinc",wetpw,zero,one); delay(dz); 
       } 
      else
       Wet4(wetpwr,wetshape,wetpw,zero,one); delay(dz); 
       delay(1.0e-4);
       obspower(tpwr);
    }
        rcvroff();

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(grecov/2);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

  if (CH2only[A] == 'y')
   {
        rgpulse(pw,zero,0.0,0.0);                 /* 1H pulse excitation */
        decphase(t1);

        zgradpulse(gzlvl5, gt5);
        delay(2*lambda - gt5);

        simpulse(2.0*pw, pwC, zero, t1, 0.0, 0.0);
	txphase(t2);

        zgradpulse(gzlvl5, gt5);
        delay(2*lambda - gt5);

        rgpulse(pw,t2,0.0,0.0);
	txphase(zero);
	decphase(zero);

        delay(CTdelay/4 - 2*lambda - pw - pwZa + tau1/2);

        sim3pulse(2*pw, 0.0, 2*pwN, zero, zero, zero, 0.0, 0.0);

        delay(CTdelay/4 - pwZa - pwC + tau1/2);

        decrgpulse(2*pwC, zero, 0.0, 0.0);
        decphase(zero);

        delay(CTdelay/4 - pwZa - tau1/2);

        sim3pulse(2*pw, 0.0, 2*pwN, zero, zero, zero, 0.0, 0.0);

        delay(CTdelay/4 - 2*lambda - pw - pwZa - tau1/2);

        rgpulse(pw,zero,0.0,0.0);
	decphase(t3);

        zgradpulse(gzlvl5, gt5);
        delay(2*lambda - gt5);

        simpulse(2.0*pw, pwC, zero, t3, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
	delay(2*lambda - gt5 - 2*POWER_DELAY);
   }
  else
   {
   	rgpulse(pw,zero,0.0,0.0);                 /* 1H pulse excitation */
   	decphase(zero);

	zgradpulse(gzlvl5, gt5);

  if (SHAPE[A] == 'y')
   {
        decpwrf(rfst);
        delay(lambda - gt5 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

     /* coupling evol reduced by 140us using rna_stC pulses. Also WFG2_START_DELAY*/
        simshaped_pulse("", rna_stCshape, 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
	decphase(t1);

        zgradpulse(gzlvl5, gt5);
        decpwrf(rfC);
        delay(lambda - gt5 - 0.5e-3 + 70.0e-6);
   }
  else
   {
	delay(lambda - gt5);

   	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
	decphase(t1);

	zgradpulse(gzlvl5, gt5);
	delay(lambda - gt5);
   }

 	decrgpulse(pwC, t1, 0.0, 0.0);
	decphase(t2);

	delay(CTdelay/4 - pwZa + tau1/2);

	sim3pulse(2*pw, 0.0, 2*pwN, zero, zero, zero, 0.0, 0.0);

	delay(CTdelay/4 - pwZa - pwC + tau1/2);

	decrgpulse(2*pwC, t2, 0.0, 0.0);
	decphase(zero);

        delay(CTdelay/4 - pwZa - tau1/2);

        sim3pulse(2*pw, 0.0, 2*pwN, zero, zero, zero, 0.0, 0.0);

        delay(CTdelay/4 - pwZa - tau1/2);

	decrgpulse(pwC, zero, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        delay(lambda - gt5);

        simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

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

