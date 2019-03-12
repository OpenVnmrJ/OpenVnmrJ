/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* wgroesy.c - rotating frame NOE experiment with T-Roesy modification 
                   allows T-Roesy or standard cw Roesy
                   WATERGATE version

             OBSERVE TRANSMITTER SHOULD BE SET AT SOLVENT POSITION
             SATURATION,SPIN LOCK AND PULSES ALL USE OBS.XMTR

        p1 = 90 degree pulse on protons (power level at "p1lvl")
        pw = 90 degree spin lock pulse at tpwr (active only if T_flg='y')
     p1lvl = power level for the p1 pulse
      tpwr = power level for the spin lock pulse(s)
    flippw = long pulse on water during echo
    flippwr = pwrlvl for flippw
   phincr2 = small angle phase shift for flippw
     T_flg= 'y' gives pulsed T-Roesy; 'n': cw Roesy
     phase = 1,2: F1 quadrature by the hypercomplex method
       mix = mixing time
     sspul = 'y': activates gradient-90degree-gradient pulse prior to d1
    gzlvl1 = gradient level for sspul
       gt1 = gradient duration foe sspul
        nt = min:  multiple of 4; max:  multiple of 8

   Use of any method to make lp1=0 will result in a dc offset of F1 slices. This
   should be removed by dc2d('f1') after the 2d transform. Enough noise should 
   be left on the edges (in F1) to permit this dc correction.

   Revised from roesy.c  G. Gray  Sept 1991  Palo Alto  
   Troesy version: ech jan 95 dast
   WG-version P.S. oct. 96.
   added to BioPack  Nov 2002
*/


#include <standard.h>
static int phi1[8] = {0,0,2,2,1,1,3,3},
           phi2[8] = {0,0,0,0,1,1,1,1},
           phi3[8] = {2,2,2,2,3,3,3,3},
           phi4[8] = {1,1,3,3,2,2,0,0},
           phi5[8] = {1,3,1,3,2,0,2,0},
           phi6[8] = {3,1,3,1,0,2,0,2};

void pulsesequence()
{
   double          p1lvl = getval("p1lvl"),
                   phase = getval("phase"),
                   mix = getval("mix"),
                   cycles, d2corr,
                   gt1 = getval("gt1"),
                   gzlvl1 = getval("gzlvl1"),
                   gt2 = getval("gt2"),
                   gzlvl2 = getval("gzlvl2"),
                   phincr2 = getval("phincr2"),
                   gstab = getval("gstab"),
                   p180 = getval("p180"),
                   flippw = getval("flippw"),
                   flippwr = getval("flippwr"),
                   d3 = getval("d3");
   int             iphase;
   char            sspul[MAXSTR],T_flg[MAXSTR],flag3919[MAXSTR];


/* LOAD AND INITIALIZE PARAMETERS */
   iphase = (int) (phase + 0.5);
   getstr("sspul", sspul);
   getstr("satmode", satmode);
   getstr("T_flg", T_flg);
   getstr("flag3919",flag3919);
   flippw = getval("flippw");
   tau = getval("tau");
   if (phincr2 < 0.0) phincr2=1440+phincr2;
   initval(phincr2,v8);

/* CALCULATE PHASES AND INITIALIZE LOOP COUNTER FOR MIXING TIME */
   settable(t1,8,phi1);
   settable(t2,8,phi2);
   settable(t3,8,phi3);
   settable(t4,8,phi4);
   settable(t5,8,phi5);
   settable(t6,8,phi6);
   getelem(t1,ct,v1);
   assign(v1,oph);	
   if (iphase == 2)
      incr(v1);			/* BC2D hypercomplex method */

  /* FOR HYPERCOMPLEX, USE REDFIED TRICK TO MOVE AXIALS TO EDGE */  
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v9); /* moves axials */
   if ((iphase==2)||(iphase==1)) {add(v1,v9,v1); add(oph,v9,oph);}

   cycles = mix / (16.0 * (4e-6 + 2.0*pw));
   initval(cycles, v10);	/* mixing time cycles */

/* BEGIN ACTUAL PULSE SEQUENCE */
   status(A);
   obspower(p1lvl);
   if (sspul[A] == 'y')
   {
     zgradpulse(gzlvl1,gt1);
     delay(5.0e-5);
     rgpulse(p1,zero,rof1,rof1);
     zgradpulse(gzlvl1,gt1); 
     delay(5.0e-5);
   }
   delay(d1);
   status(B);
   obsstepsize(45.0);
   initval(7.0,v4);  
   xmtrphase(v4);
   rgpulse(p1,v1,rof1,1.0e-6);
   xmtrphase(zero);   
   d2corr = rof1 + 1.0e-6 + SAPS_DELAY + 2*p1/3.1416;
   if (d2 > d2corr) delay(d2 - d2corr); else delay(0.0);
   status(A);
   if ((T_flg[0] == 'y')&&(cycles > 1.5))
    {
      rgpulse(p1,t4,rof1,rof1);
      obspower(tpwr);
      {
         starthardloop(v10);
            rgpulse(2.0*pw,t2,4e-6,0.0);
            rgpulse(2.0*pw,t3,4e-6,0.0);
            rgpulse(2.0*pw,t2,4e-6,0.0);
            rgpulse(2.0*pw,t3,4e-6,0.0);
            rgpulse(2.0*pw,t2,4e-6,0.0);
            rgpulse(2.0*pw,t3,4e-6,0.0);
            rgpulse(2.0*pw,t2,4e-6,0.0);
            rgpulse(2.0*pw,t3,4e-6,0.0);
            rgpulse(2.0*pw,t2,4e-6,0.0);
            rgpulse(2.0*pw,t3,4e-6,0.0);
            rgpulse(2.0*pw,t2,4e-6,0.0);
            rgpulse(2.0*pw,t3,4e-6,0.0);
            rgpulse(2.0*pw,t2,4e-6,0.0);
            rgpulse(2.0*pw,t3,4e-6,0.0);
            rgpulse(2.0*pw,t2,4e-6,0.0);
            rgpulse(2.0*pw,t3,4e-6,0.0);
         endhardloop();
      }
      obspower(p1lvl);
      rgpulse(p1,t5,rof1,rof1); 
    }  
/* The ROESY spin-lock unit is executed sixteen times within the
   hardware loop so that it is of sufficient duration to allow
   the acquisition hardware loop to be loaded in behind it on
   the last pass through the spin-lock loop. */

   else
    {
      obspower(tpwr);
      rgpulse(mix,t2,rof1,rof1);          /* cw spin lock  */
    }
   if (flag3919[0] == 'y')
    {
      obspower(p1lvl);
      delay(tau);
      zgradpulse(gzlvl2,gt2);
      delay(gstab);
      rgpulse(p1*0.231,t5,0.0,0.0);
      delay(d3);
      rgpulse(p1*0.692,t5,0.0,0.0);
      delay(d3);
      rgpulse(p1*1.462,t5,0.0,0.0);
      delay(d3);
      rgpulse(p1*1.462,t6,0.0,0.0);
      delay(d3);
      rgpulse(p1*0.692,t6,0.0,0.0);
      delay(d3);
      rgpulse(p1*0.231,t6,0.0,0.0);
      delay(tau);
      zgradpulse(gzlvl2,gt2);
      delay(gstab);
      delay(rof2);
    }
   else
    {
      obsstepsize(0.25);
      delay(tau);
      zgradpulse(gzlvl2,gt2);
      delay(gstab);
      obspower(flippwr);
      obsstepsize(0.25);
      xmtrphase(v8);
      rgpulse(flippw,t6,rof1,rof1);
      obspower(p1lvl);
      xmtrphase(zero);
      rgpulse(p180,t5,rof1,rof1);
      obspower(flippwr);
      delay(SAPS_DELAY);
      rgpulse(flippw,t6,rof1,rof2);
      obspower(tpwr);
      delay(SAPS_DELAY);
      zgradpulse(gzlvl2,gt2);
      delay(tau + gstab);
    }
   status(C);
}
