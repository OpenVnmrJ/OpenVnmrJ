/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* wnoesy.c 
            2D cross relaxation experiment.  It can be performed in
            either phase-sensitive or absolute value mode.  Either
            TPPI or the hypercomplex method can be used to achieve
            F1 quadrature in a phase-sensitive presentation.  No
            attempt is made to suppress J-cross peaks in this pulse
            sequence.

           WET suppression available in relaxation delay and/or mix period
           gradients can suppress radiation damping in t1 and/or mix period

  Parameters:

       mix = mixing time.
     phase =   0: gives non-phase-sensitive experiment (P-type peaks);
                  nt(min) = multiple of 16
                  nt(max) = multiple of 64

             1,2: gives HYPERCOMPLEX phase-sensitive experiment;
               3: gives TPPI phase-sensitive experiment;
                  nt(min) = multiple of  8
                  nt(max) = multiple of 32

     sspul = 'y': selects for HS-90-HS sequence at start of pulse sequence
             'n': normal NOESY experiment

 */


#include <standard.h>

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

/* Chess1 - CHEmical Shift Selective Suppression */
static void Chess1(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw1,gtw1,gswet1)
  double pulsepower,duration,rx1,rx2,gzlvlw1,gtw1,gswet1;
  codeint phase;
  char* pulseshape;
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw1,gtw1);                                                   
  delay(gswet1);
}

/* Wet4first - Water Elimination during relaxation delay */
static void Wet4first(pulsepower,wetshape,duration,phaseA,phaseB)
  double pulsepower,duration;
  codeint phaseA,phaseB;
  char* wetshape;
{
  double wetpw1,finepwr,gzlvlw1,gtw1,gswet1;
  gzlvlw1=getval("gzlvlw1"); gtw1=getval("gtw1"); gswet1=getval("gswet1");
  wetpw1=getval("wetpw1");
  finepwr=pulsepower-(int)pulsepower;     /* Adjust power to 152 deg. pulse*/
  pulsepower=(double)((int)pulsepower);             
  if (finepwr == 0.0) {pulsepower=pulsepower+5; finepwr=4095.0; }
  else {pulsepower=pulsepower+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(pulsepower);         /* Set to low power level        */
  Chess1(finepwr*0.6452,wetshape,wetpw1,phaseA,20.0e-6,rof1,gzlvlw1,gtw1,gswet1);
  Chess1(finepwr*0.5256,wetshape,wetpw1,phaseB,20.0e-6,rof1,gzlvlw1/2.0,gtw1,gswet1);
  Chess1(finepwr*0.4928,wetshape,wetpw1,phaseB,20.0e-6,rof1,gzlvlw1/4.0,gtw1,gswet1);
  Chess1(finepwr*1.00,wetshape,wetpw1,phaseB,20.0e-6,rof1,gzlvlw1/8.0,gtw1,gswet1);
  obspower(tpwr); obspwrf(tpwrf);  /* Reset to normal power level   */
  rcvron();
}

pulsesequence()
{
   double          arraydim,compH,wetpwr,wetpwr1,
                   corr1,corr,gstab,gzlvl1,gzlvl2,gt1,gt2,
                   mix,wetpw,wetpw1,gtw,gtw1,gswet,gswet1;
   int             phase;
   char            autosoft1[MAXSTR],autosoft2[MAXSTR],wetshape1[MAXSTR],
                   wetshape[MAXSTR],wet[MAXSTR],sspul[MAXSTR];


/* LOAD VARIABLES */
  wetpw=getval("wetpw");        /* User enters power for 90 deg. */
  wetpwr1=getval("wetpwr1");       
  wetpwr=getval("wetpwr");       
  wetpw1=getval("wetpw1");        /* User enters power for 90 deg. */
  gtw1=getval("gtw1");            /* Z-Gradient duration           */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
  gswet1=getval("gswet1");        /* Post-gradient stability delay */
    compH=getval("compH");
   ni = getval("ni");
   arraydim = getval("arraydim");
   gzlvl1=getval("gzlvl1"); gt1=getval("gt1");
   gzlvl2=getval("gzlvl2"); gt2=getval("gt2");
   gstab=getval("gstab");
   mix = getval("mix");
   phase = (int) (getval("phase") + 0.5);
   getstr("sspul", sspul);
   getstr("autosoft1",autosoft1);
   getstr("autosoft2",autosoft2);
   getstr("wetshape", wetshape);
   getstr("wetshape1", wetshape1);
   getstr("wet", wet);

/* For TPPI,  initialize v14 with the t1 increment number */
   if (phase == 3)
      initval((double) ((int) ((ix - 1) / (arraydim / ni) + 1e-6)), v14);


/* CHECK CONDITIONS */
   if ((rof1 < 9.9e-6) && (ix == 1))
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n");
   if ((mix - rof1 - (4.0*wetpw) - (4.0*20.0e-6) - (4.0*rof1) - (4.0*gtw) - (4.0*gswet)) < 1.0e-6)
      {
      printf("Warning: mix time is too short for WET to be executed.\n");
      psg_abort(1);
      }

/* STEADY-STATE PHASECYCLING
 This section determines if the phase calculations trigger off of (SS - SSCTR)
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



/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
   if (sspul[0] == 'y')
   {
      zgradpulse(1000.0,.001);
      rgpulse(pw, v2, 1.0e-6, 1.0e-6);
      zgradpulse(1500.0,.001);
   }
      lk_sample();
       if (wet[A] == 'y')
	{
         delay(d1-4.0*wetpw1-4.0*gswet1);

         if (autosoft1[A] == 'y') 
          { 
              /* selective H2O one-lobe sinc pulse */
           wetpwr1 = tpwr - 20.0*log10(wetpw1/(pw*compH*1.69));  /* sinc needs 1.69 times more */
           wetpwr1 = (int) (wetpwr1 +0.5);                       /* power than a square pulse */
           Wet4first(wetpwr1,"H2Osinc",wetpw1,zero,one);  
          } 
         else
	 Wet4first(wetpwr1,wetshape1,wetpw1,zero,one);
	}
       else
        delay(d1);

   status(B);
      rgpulse(pw, v2, rof1, 1.0e-6);
      corr = 1.0e-6 + rof1 +2.0*GRADIENT_DELAY+ 4.0*pw/3.1416;
      corr1 = 1.0e-6 + rof1 +4.0*pw/3.1416;
       if (gzlvl1>0) 
       {
         if (d2>0.001)          /*suppress radiation damping after 4th dwell time in t1*/
          {
           zgradpulse(gzlvl1,0.45*(d2-corr)-gstab/2.0);
           delay(0.1*(d2-corr));
           zgradpulse(-1.0*gzlvl1,0.45*(d2-corr)-gstab/2.0);
           delay(gstab);
          }
         else 
          if ((d2-corr1)>0.0) delay(d2-corr1);
       }
       else
         if ((d2-corr1)>0.0) delay(d2-corr1);
      rgpulse(pw, v1, rof1, 1.0e-6);
   status(C);
          lk_hold();
          delay(d4);
          zgradpulse(gzlvl2,gt2);
    if (wet[B] == 'y')
	{
          delay(mix - d4 -rof1 - 1.0e-4 -gt2 -(4.0*wetpw) - (4.0*20.0e-6)
               - (4.0*rof1) - (4.0*gtw) - (4.0*gswet));

          if (autosoft2[A] == 'y') 
           { 
               /* selective H2O one-lobe sinc pulse */
            wetpwr = tpwr - 20.0*log10(wetpw/(pw*compH*1.69));  /* sinc needs 1.69 times more */
            wetpwr = (int) (wetpwr +0.5);                       /* power than a square pulse */
            Wet4(wetpwr,"H2Osinc",wetpw,zero,one); 
           } 
          else
	   Wet4(wetpwr,wetshape,wetpw,zero,one);
	}
	else
          delay(mix - 1.0e-4 -gt2 -d4-rof1);

   statusdelay(D,1.0e-4);
      rgpulse(pw, v3, rof1, rof2);
      lk_sample();
}

/* phase table for WET portion of water.c

t1 = (0 2 0 2)4 (1 3 1 3)4 (2 0 2 0)4 (3 1 3 1)4 
t2 = (0 0 2 2)4 (1 1 3 3)4 (2 2 0 0)4 (3 3 1 1)4
t3 = 0 0 0 0 2 2 2 2 1 1 1 1 3 3 3 3
t4 = 0 0 0 0 2 2 2 2 1 1 1 1 3 3 3 3
 */
