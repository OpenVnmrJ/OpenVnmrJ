/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* wettntocsy - made from tntocsy (V1.3 10/11/94)
     tntocsy - total correlation spectroscopy  (a.k.a. hohaha)
              "clean tocsy" used with windowing and MLEV16+60degree spin lock

    ref: a. bax and d.g. davis, j. magn. reson. 65, 355 (1985)
         m. levitt, r. freeman, and t. frenkiel, j. magn. reson. 47, 328 (1982)

  Parameters:
     window= clean-tocsy window(in us)
        p1 = 90 degree excitation pulse (at power p1lvl)
        pw = 90 degree pulse during mlev periods (at power level tpwr)
    satdly = length of presaturation;
    satmode  = flag for presat control
               'yn' for during relaxation delay 
               'yy' for during both "relaxation delay" and "d2" (recommended)
     phase = 1,2: for HYPERCOMPLEX phase-sensitive F1 quadrature
               3: for TPPI phase-sensitive F1 quadrature
     sspul = 'y':  trim(x)-trim(y) sequence initiates D1 delay
             'n':  normal D1 delay
      trim = spinlock trim pulse time
       mix = mixing time  (can be arrayed)
        nt = min:  multiple of 2
             max:  multiple of 8  (recommended)

           TRANSMITTER MUST BE POSITIONED AT SOLVENT FREQUENCY 
             this pulse sequence requires a T/R switch,
             linear amplifiers and computer-controlled attenuators on the
             observe channel.

        contact- G. Gray (palo alto)  revision- from tocsy.c
	P.A.Keifer - 940916 - fixed d2 timing
	P.A.Keifer 950406 - made wettocsy (std format)
	P.A.Keifer 950920 - updated wet
        P.A.Keifer 960116 - added tpwrf control to Wet4

	Changes for RnaPack by Peter Lukavsky, Stanford, Sept. 1999

        wetpw instead of pwwet
        rlpower changed to obspower
        pw      :	90 degree excitation pulse (at power level tpwr)
        p1      :       90 deg pulse width for spinlock

        reversed above assignment of pw/p1 to be consistent with other
        BioPack usage. GG aug2004


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
  double finepwr,gzlvlw,gtw,gswet,wetpwr,wetpw,dz;
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
  Chess(finepwr*0.5059,wetshape,wetpw,phaseA,20.0e-6,rof2,gzlvlw,gtw,gswet);
  Chess(finepwr*0.6298,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4304,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/8.0,gtw,gswet);
  obspower(tpwr); obspwrf(tpwrf);     /* Reset to normal power level   */
  rcvron();
  delay(dz);
}
void mleva(double window)
{
   txphase(v2); delay(pw);
   xmtroff(); delay(window); xmtron();
   txphase(v3); delay(2 * pw);
   xmtroff(); delay(window); xmtron();
   txphase(v2); delay(pw);
}

void mlevb(double window)
{
   txphase(v4); delay(pw);
   xmtroff(); delay(window); xmtron();
   txphase(v5); delay(2 * pw);
   xmtroff(); delay(window); xmtron();
   txphase(v4); delay(pw);
}


void pulsesequence()
{
   double          ss,
                   arraydim,
                   p1lvl,
                   trim,
                   mix,
                   window,
                   cycles,
                   phase;
   int             iphase;
   char            wet[MAXSTR],sspul[MAXSTR];


/* LOAD AND INITIALIZE VARIABLES */
   ni = getval("ni");
   arraydim = getval("arraydim");
   mix = getval("mix");
   trim = getval("trim");
   phase = getval("phase");
   iphase = (int) (phase + 0.5);
   p1lvl = getval("p1lvl");
   ss = getval("ss");
   window=getval("window");
   getstr("sspul", sspul);
   getstr("wet", wet);

   if (iphase == 3)
   {
      initval((double)((int)((ix-1)/(arraydim/ni)+1.0e-6)), v14);
   }
   else
   {
      assign(zero, v14);
   }


/* CHECK CONDITIONS */
   if ((iphase != 3) && (arrayelements > 2))
   {
      fprintf(stdout, "PHASE=3 is required if MIX is arrayed!\n");
      psg_abort(1);
   }
   if (satdly > 9.999)
   {
      fprintf(stdout, "Presaturation period is too long.\n");
      psg_abort(1);
   }
   if (!newtransamp)
   {
      fprintf(stdout, "TOCSY requires linear amplifiers on transmitter.\n");
      fprintf(stdout, "Use DECTOCSY with the appropriate re-cabling,\n");
      psg_abort(1);
   }
   if ((pw == 0.0) && (ix == 1))
      fprintf(stdout, "Warning:  PW has a zero value.\n");
   if ((rof1 < 9.9e-6) && (ix == 1))
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n");

   if (satpwr > 40)
        {
         printf("satpwr too large  - acquisition aborted.\n");
         psg_abort(1);
        }

/* DETERMINE STEADY-STATE MODE */
   if (ss < 0)
   {
      ss = (-1) * ss;
   }
   else
   {   
      if ((ss > 0) && (ix == 1))
      {
         ss = ss;
      }
      else
      {   
         ss = 0;
      }
   }
   initval(ss, ssctr);
   initval(ss, ssval);


/* STEADY-STATE PHASECYCLING*/
/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      hlv(ct, v13);
      mod2(ct, v1);
      hlv(ct, v2);
   elsenz(ssctr);
      sub(ssval, ssctr, v12);	/* v12 = 0,...,ss-1 */
      hlv(v12, v13);
      mod2(v12, v1);
      hlv(v12, v2);
   endif(ssctr);


/* CALCULATE PHASES */
/* A 2-step cycle is performed on the first pulse (90 degrees) to suppress
   axial peaks in the first instance.  Second, the 2-step F2 quadrature image
   suppression subcycle is added to all pulse phases and receiver phase.
   Finally, a 2-step cycle is performed on the spin-lock pulses. */

   mod2(v13, v13);
   dbl(v1, v1);
   incr(v1);
   hlv(v2, v2);
   mod2(v2, v2);
   dbl(v2, v2);
   incr(v2);
   add(v13, v2, v2);
   sub(v2, one, v3);
   add(two, v2, v4);
   add(two, v3, v5);
   add(v1, v13, v1);
   assign(v1, oph);
   if (iphase == 2)
      incr(v1);
   if (iphase == 3)
      add(v1, v14, v1);

 /*HYPERCOMPLEX MODE USES REDFIELD TRICK TO MOVE AXIAL PEAKS TO EDGE*/
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v6);
   if ((iphase==1)||(iphase==2)) {add(v1,v6,v1); add(oph,v6,oph);} 

/* CALCULATE AND INITIALIZE LOOP COUNTER */
      if (pw > 0.0)
      {
         cycles = (mix - trim) / (64.66*pw+32*window);
         cycles = 2.0*(double) (int) (cycles/2.0);
      }
      else
      {
         cycles = 0.0;
      }
      initval(cycles, v9);			/* V9 is the MIX loop count */

/* BEGIN ACTUAL PULSE SEQUENCE CODE */
   status(A);
      obspower(p1lvl-12);
      if (sspul[0] == 'y')
      {
         rgpulse(1000*1e-6, zero, rof1, 0.0e-6);
         rgpulse(1000*1e-6, one, 0.0e-6, rof1);
      }
      obspower(p1lvl);
      hsdelay(d1);
     if (getflag("wet")) Wet4(zero,one);
      obspower(p1lvl);
     if (satmode[A] == 'y')
     { obspower(satpwr);
      rgpulse(satdly,zero,rof1,rof1);
      obspower(p1lvl);}
   status(B);
      rgpulse(p1, v1, rof1, 1.0e-6);
      if (satmode[B] =='y')
       {
        if (d2 > 0.0)
         {
           obspower(satpwr);
           rgpulse(d2 - (2*POWER_DELAY) - 1.0e-6 - (2*pw/3.14159265358979323846),zero,0.0,0.0);
         }
       }
      else
       {
        if (d2 > 0.0)
          delay(d2 - POWER_DELAY - 1.0e-6  - (2*pw/3.14159265358979323846));
       } 
      rcvroff();
      obspower(tpwr); 
      txphase(v13);
      xmtron();
      delay(trim);
      if (cycles > 1.0)
      {
         starthardloop(v9);
            mleva(window); mleva(window); mlevb(window); mlevb(window);
            mlevb(window); mleva(window); mleva(window); mlevb(window);
            mlevb(window); mlevb(window); mleva(window); mleva(window);
            mleva(window); mlevb(window); mlevb(window); mleva(window);
            rgpulse(.66*pw,v3,rof1,rof1);
         endhardloop();
      }
      txphase(v13);
      xmtroff();

/* detection */
      delay(rof2);
      rcvron();
   status(C);
}
