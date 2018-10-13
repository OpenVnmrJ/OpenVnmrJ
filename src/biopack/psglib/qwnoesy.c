/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
 /* qwnoesy.c 
            2D cross relaxation experiment.  It can be performed in
            either phase-sensitive or absolute value mode.  Either
            TPPI or the hypercomplex method can be used to achieve
            F1 quadrature in a phase-sensitive presentation.  No
            attempt is made to suppress J-cross peaks in this pulse
            sequence.

           WET suppression available in relaxation delay and/or mix period
           gradients can suppress radiation damping in t1 and/or mix period
 
           "Quiet" option for suppression of spin-diffusion (requires
           labelled compound)

  Parameters:

       mix = mixing time.
     phase =   0: gives non-phase-sensitive experiment (P-type peaks);
                  nt(min) = multiple of 16
                  nt(max) = multiple of 64

             1,2: gives HYPERCOMPLEX phase-sensitive experiment;
               3: gives TPPI phase-sensitive experiment;
                  nt(min) = multiple of  8
                  nt(max) = multiple of 32

      pwN,pwNlvl   pw90 for N15 on channel 3 at power level pwNlvl
      pwC,pwClvl   pw90 for C13 on channel 2 at power level pwClvl
    Cquiet OR Nquiet  set='y' for "quiet" option (suppresses spin-diffusion caused
                     noesy cross peaks in labelled samples)

      c13refoc='y' uses adiabadic pulse (power derived from compC/pwClvl) in BIRD pulse
             wet   set to 'yn' for wet in relaxation delay
                          'ny' for wet in mix period
                          'yy' for both
                          'nn' for neither
      wet pulses may be different in relaxation and mix (could have multisite
          suppression in mix period with longer pulses)
 */


#include <standard.h>

/* Chess1 - CHEmical Shift Selective Suppression */
Chess1(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw1,gtw1,gswet1)  double pulsepower,duration,rx1,rx2,gzlvlw1,gtw1,gswet1;
  codeint phase;
  char* pulseshape;
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw1,gtw1);

  delay(gswet1);
}


/* Wet4first - Water Elimination */
Wet4first(phaseA,phaseB)
  codeint phaseA,phaseB;

{
  double finepwr,gzlvlw1,gtw1,gswet1,wetpwr1,pwwet1,dz1;
  char   wetshape1[MAXSTR];
  getstr("wetshape1",wetshape1);    /* Selective pulse shape (base)  */
  wetpwr1=getval("wetpwr1");        /* User enters power for 90 deg. */
  pwwet1=getval("pwwet1");        /* User enters power for 90 deg. */
  dz1=getval("dz1");
  finepwr=wetpwr1-(int)wetpwr1;     /* Adjust power to 152 deg. pulse*/
  wetpwr1=(double)((int)wetpwr1);
  if (finepwr==0.0) {wetpwr1=wetpwr1+5; finepwr=4095.0; }
  else {wetpwr1=wetpwr1+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(wetpwr1);         /* Set to low power level        */
  gzlvlw1=getval("gzlvlw1");      /* Z-Gradient level              */
  gtw1=getval("gtw1");            /* Z-Gradient duration           */
  gswet1=getval("gswet1");        /* Post-gradient stability delay */
  Chess1(finepwr*0.5059,wetshape1,pwwet1,phaseA,20.0e-6,rof1,gzlvlw1,gtw1,gswet1);
  Chess1(finepwr*0.6298,wetshape1,pwwet1,phaseB,20.0e-6,rof1,gzlvlw1/2.0,gtw1,gswet1);
  Chess1(finepwr*0.4304,wetshape1,pwwet1,phaseB,20.0e-6,rof1,gzlvlw1/4.0,gtw1,gswet1);
  Chess1(finepwr*1.00,wetshape1,pwwet1,phaseB,20.0e-6,rof1,gzlvlw1/8.0,gtw1,gswet1);
  obspower(tpwr); obspwrf(tpwrf);       /* Reset to normal power level   */
  rcvron();
  delay(dz1);
}

/* Chess - CHEmical Shift Selective Suppression */
Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)  double pulsepower,duration,rx1,rx2,gzlvlw,gtw,gswet;
  codeint phase;
  char* pulseshape;
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw,gtw);

  delay(gswet);
}

 
/* Wet4 - Water Elimination during MIX*/
Wet4(phaseA,phaseB)
  codeint phaseA,phaseB;

{
  double finepwr,gzlvlw,gtw,gswet,wetpwr,pwwet,dz;
  char   wetshape[MAXSTR];
  getstr("wetshape",wetshape);    /* Selective pulse shape (base)  */
  wetpwr=getval("wetpwr");        /* User enters power for 90 deg. */
  pwwet=getval("pwwet");        /* User enters power for 90 deg. */
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
  Chess(finepwr*0.5059,wetshape,pwwet,phaseA,20.0e-6,rof1,gzlvlw,gtw,gswet);
  Chess(finepwr*0.6298,wetshape,pwwet,phaseB,20.0e-6,rof1,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4304,wetshape,pwwet,phaseB,20.0e-6,rof1,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,pwwet,phaseB,20.0e-6,rof1,gzlvlw/8.0,gtw,gswet);
  obspower(tpwr); obspwrf(tpwrf);       /* Reset to normal power level   */
  rcvron();
  delay(dz);
}
 
pulsesequence()
{
   double          arraydim,
                   gzlvl1,gzlvl2,gt1,
                   rf0,            	          /* maximum fine power when using pwC pulses */
                   rfst,	                           /* fine power for the stCall pulse */
                   tau1,pwN,pwNlvl,jnh,
                   pwC,pwClvl,jch,compH,compC,compN,
                   mix,pwwet,pwwet1,gtw,gtw1,gswet,gswet1;
   int             phase;
   char            wet[MAXSTR],sspul[MAXSTR],
                   Nquiet[MAXSTR],Cquiet[MAXSTR],
                   c13refoc[MAXSTR];


/* LOAD VARIABLES */
    jnh=getval("jnh");
    jch=getval("jch");
   pwC=getval("pwC");
 pwClvl=getval("pwClvl");
   pwN=getval("pwN");
 pwNlvl=getval("pwNlvl");
   getstr("c13refoc",c13refoc);
   getstr("sspul",sspul);
   getstr("Cquiet",Cquiet);
   getstr("Nquiet",Nquiet);
  pwwet=getval("pwwet");        /* User enters power for 90 deg. */
  pwwet1=getval("pwwet1");        /* User enters power for 90 deg. */
  gtw1=getval("gtw1");            /* Z-Gradient duration           */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
  gswet1=getval("gswet1");        /* Post-gradient stability delay */
   ni = getval("ni");
   arraydim = getval("arraydim");
   gzlvl1=getval("gzlvl1"); gt1=getval("gt1");
   gzlvl2=getval("gzlvl2"); 
   mix = getval("mix");
   phase = (int) (getval("phase") + 0.5);
   getstr("wet", wet);






   compH = getval("compH");        /* adjustment for H1 amplifier compression */
   compN = getval("compN");       /* adjustment for N15 amplifier compression */
   compC = getval("compC");       /* adjustment for C13 amplifier compression */


/* maximum fine power for pwC pulses (and initialize rfst) */
	rf0 = 4095.0;    rfst=0.0;

/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
     if (c13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
	rfst = (int) (rfst + 0.5);
	if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }}


/* For TPPI,  initialize v14 with the t1 increment number */
   if (phase == 3)
      initval((double) ((int) ((ix - 1) / (arraydim / ni) + 1e-6)), v14);


/* CHECK CONDITIONS */
   if ((rof1 < 9.9e-6) && (ix == 1))
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n");
   if ((mix - rof1 - (4.0*pwwet) - (4.0*20.0e-6) - (4.0*rof1) - (4.0*gtw) - (4.0*gswet)) < 1.0e-6)
      {
      printf("Warning: mix time is too short for WET to be executed.\n");
      psg_abort(1);
      }

/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      mod2(ct, v2);
      hlv(ct, v3);
   elsenz(ssctr);
      sub(ssval, ssctr, v12);	/* v12 = 0,...,ss-1 */
      mod2(v12, v2);
      hlv(v12, v3);
   endif(ssctr);


/* CALCULATE PHASECYCLE */
   dbl(v2, v2);
   hlv(v3, v10);
   hlv(v10, v10);
   if (phase == 0)
   {
      assign(v10, v9);
      hlv(v10, v10);
      mod2(v9, v9);
   }
   else
   {
      assign(zero, v9);
   }
   hlv(v10, v1);
   mod2(v1, v1);
   dbl(v1, v1);
   add(v9, v2, v2);
   mod2(v10, v10);
   add(v1, v2, oph);
   add(v3, oph, oph);
   add(v10, oph, oph);
   add(v10, v1, v1);
   add(v10, v2, v2);
   add(v10, v3, v3);
   if (phase == 2)
      incr(v2);
   if (phase == 3)
      add(v2, v14, v2);		/* TPPI phase increment */

/* FAD added for phase=1 or phase=2 */
   if ((phase == 1) || (phase == 2))
   {
      initval(2.0*(double)((int)(d2*getval("sw1")+0.5)%2),v11);
      add(v2,v11,v2); add(oph,v11,oph);
   }

/* The first 90 degree pulse is cycled first to suppress axial peaks.
   This requires a 2-step phasecycle consisting of (0 2).  The
   third 90 degree pulse is cycled next using a 4-step phasecycle
   designed to select both longitudinal magnetization, J-ordered states,
   and zero-quantum coherence (ZQC) during the mixing period.  If the
   experiment is to collect data requiring an absolute value display,
   the first pulse and the receiver are next incremented by 1 to achieve
   w1 quadrature (P-type peaks).  If the data are to be presented in
   a phase-sensitive manner, this step is not done.  Next, the second
   90 degree pulse is cycled to suppress axial peaks.  Finally, all
   pulse and receiver phases are incremented by 90 degrees to achieve
   quadrature image suppression due to receiver channel imbalance. */
     tau1=d2/2.0;
     if (Nquiet[A]=='y') 
      tau1=d2/2.0-pwN-101.0e-6-GRADIENT_DELAY-(2.0*pw/PI);
     if (Cquiet[A]=='y')
      tau1=d2/2.0-pwC-101.0e-6-GRADIENT_DELAY-(2.0*pw/PI);
     if ((Cquiet[A]=='n') && (Nquiet[A]=='n'))
      tau1=d2/2.0-50.5e-6-GRADIENT_DELAY-(2.0*pw/PI);


/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
   decpower(pwClvl);
   dec2power(pwNlvl);
   if (sspul[0] == 'y')
   {
      zgradpulse(1000.0,.001);
      rgpulse(pw, v2, 1.0e-6, 1.0e-6);
      zgradpulse(1500.0,.001);
   }
      lk_sample();
       if (wet[A] == 'y')
	{
         delay(d1-4.0*pwwet1-4.0*gswet1);
         lk_hold();
	 Wet4first(zero,one);
	}
       else
        {
         delay(d1);
         lk_hold();
        }
  status(B); /*use dm[B]='y' and/or dm2[B]='y' for decoupling in t1"*/
      rgpulse(pw,v2,rof1,1.0e-6);
      if (tau1 > 0.0)
        {
         zgradpulse(gzlvl1,0.8*tau1);
         delay(0.2*tau1- 2.0*GRADIENT_DELAY);
         zgradpulse(-gzlvl1,0.8*tau1);
         delay(0.2*tau1- 2.0*GRADIENT_DELAY);
        }
      else
         delay(d2);
       rgpulse(pw, v1, 1.0e-6, 1.0e-6);
   status(C);
    delay(d3);
    zgradpulse(gzlvl1,gt1);
    if ((Nquiet[A] == 'y') || (Cquiet[A] == 'y'))
    {
      if (Nquiet[A] == 'y')
       {
        zgradpulse(gzlvl1,mix/2.0-d3-gswet-gt1-0.5/jnh);
        delay(gswet);
        rgpulse(pw,v3,rof1,rof1);
        zgradpulse(gzlvl2,0.4/jnh);
        delay(0.1/jnh);
        sim3pulse(2.0*pw,0.0,2.0*pwN,v3,zero,v3,rof1,rof1);
        zgradpulse(gzlvl2,0.4/jnh);
        delay(0.1/jnh);
       }
       else
       {
        zgradpulse(gzlvl1,mix/2.0-d3-gswet-gt1-0.5/jch);
        delay(gswet);
        if (c13refoc[A] == 'y')
        {
         rgpulse(pw,v3,rof1,rof1);
         decpwrf(rfst);
         zgradpulse(gzlvl2,0.4/jch);
         delay(0.1/jch - 5.0e-4 - WFG2_START_DELAY);
         simshaped_pulse("","stC200",2.0*pw,1.0e-3,v3,v3,rof1,rof1);
         decpwrf(4095.0);
         zgradpulse(gzlvl2,0.4/jch);
         delay(0.1/jch - 5.0e-4);

        }
        else
        {
         rgpulse(pw,v3,rof1,rof1);
         zgradpulse(gzlvl2,0.4/jch);
         delay(0.1/jch);
         simpulse(2.0*pw,2.0*pwC,v3,v3,rof1,rof1);
         zgradpulse(gzlvl2,0.4/jch);
         delay(0.1/jch);
        }
       }
       rgpulse(pw,v3,rof1,rof1);
      }
   else
    zgradpulse(gzlvl1,mix/2.0-d3-gt1);
    dec2power(dpwr2); decpower(dpwr); 
/* half-way point in mix period */
   if (wet[B] == 'y')
      {
       zgradpulse(gzlvl1,mix/2.0 - rof1  -(4.0*pwwet) - (4.0*20.0e-6)
        - (4.0*rof1) - (4.0*gtw) - (5.0*gswet));
       delay(gswet);
       Wet4(zero,one);
      }
   else
      {
     zgradpulse(gzlvl1,mix/2.0 -gswet - rof1);
     delay(gswet);
      }
   status(D);
      rgpulse(pw, v3, rof1, rof2);
      lk_sample();
}
