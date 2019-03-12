/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* tnroesy.c - rotating frame NOE experiment with T-Roesy modification 
              allows T-Roesy or standard cw Roesy

             OBSERVE TRANSMITTER SHOULD BE SET AT SOLVENT POSITION
             SATURATION,SPIN LOCK AND PULSES ALL USE OBS.XMTR

        p1 = 90 degree pulse on protons (power level at "p1lvl")
        pw = 90 degree spin lock pulse at tpwr (active only if T_flg='y')
     p1lvl = power level for the p1 pulse
      tpwr = power level for the spin lock pulse(s)
     T_flg= 'y' gives pulsed T-Roesy; 'n': cw Roesy
     phase = 1,2: F1 quadrature by the hypercomplex method
       mix = mixing time
     sspul = 'y': activates gradient-90degree-gradient pulse prior to d1
   satmode = activates transmitter presat at satfrq 
             (satfrq = tof; satmode='nnn' or satmode='ynn')
    satdly = length of saturation during relaxation delay  
    satpwr = power level for solvent saturation
        nt = min:  multiple of 4; max:  multiple of 8

   mfsat='y'
           Multi-frequency saturation. 
           Requires the frequency list mfpresat.ll in current experiment
           Pbox creates (saturation) shape "mfpresat.DEC"

                  use mfll('new') to initialize/clear the line list
                  use mfll to read the current cursor position into
                  the mfpresat.ll line list that is created in the
                  current experiment. 

           Note: copying pars or fids (mp or mf) from one exp to another does not copy
                 mfpresat.ll!
           Note: the line list is limited to 128 frequencies ! 
            E.Kupce, Varian, UK June 2005 - added multifrequency presat option 

   Revised from roesy.c  G. Gray  Sept 1991  Palo Alto  
   Troesy version: ech jan 95 dast
   Troesy_da.c renames tnroesy.c for BioPack   Nov 202

   Added rcvroff() and rcvron() statements after status(A) and before last status(C)
     statements to enable mfsat function - Josh Kurutz, U. of Chicago, May 2006

*/


#include <standard.h>
#include "mfpresat.h"
static int phi1[8] = {0,0,2,2,1,1,3,3},
           phi2[8] = {0,0,0,0,1,1,1,1},
           phi3[8] = {2,2,2,2,3,3,3,3},
           phi4[8] = {1,1,3,3,2,2,0,0},
           phi5[8] = {1,3,1,3,2,0,2,0};

void pulsesequence()
{
   double          p1lvl = getval("p1lvl"),
                   phase = getval("phase"),
                   mix = getval("mix"),
                   cycles, d2corr,
                   gt1 = getval("gt1"),
                   gzlvl1 = getval("gzlvl1");
   int             iphase;
   char            mfsat[MAXSTR],sspul[MAXSTR],T_flg[MAXSTR];


/* LOAD AND INITIALIZE PARAMETERS */
   iphase = (int) (phase + 0.5);
   satdly = getval("satdly");
   satpwr = getval("satpwr");
   satfrq = getval("satfrq");
   getstr("sspul", sspul);
   getstr("mfsat", mfsat);
   getstr("satmode", satmode);
   getstr("T_flg", T_flg);

/* CALCULATE PHASES AND INITIALIZE LOOP COUNTER FOR MIXING TIME */
   settable(t1,8,phi1);
   settable(t2,8,phi2);
   settable(t3,8,phi3);
   settable(t4,8,phi4);
   settable(t5,8,phi5);
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
   status(C);
   obspower(p1lvl);
   if (sspul[A] == 'y')
   {
     zgradpulse(gzlvl1,gt1);
     delay(5.0e-5);
     rgpulse(p1,zero,rof1,rof1);
     zgradpulse(gzlvl1,gt1); 
     delay(5.0e-5);
   }
   status(A);
   if (satmode[A] == 'y') 
    {
    rcvroff();
    if (d1 > satdly) delay(d1-satdly);
    if (mfsat[A] == 'y')
     {obsunblank(); mfpresat_on(); delay(satdly); mfpresat_off(); obsblank();}
    else
     {
     if (tof != satfrq) obsoffset(satfrq);
     obspower(satpwr);
     rgpulse(satdly,zero,rof1,rof1);
     if (tof != satfrq) obsoffset(tof);
     }
     obspower(p1lvl);
    }
    else
     {
      delay(d1);
      rcvroff();
     }
   status(B);
   obsstepsize(45.0);
   initval(7.0,v4);  
   xmtrphase(v4);
   rgpulse(p1,v1,rof1,1.0e-6);
   xmtrphase(zero);   
   d2corr = rof1 + 1.0e-6 + (2*p1/3.1416) + SAPS_DELAY;
   if (d2 > d2corr) delay(d2 - d2corr); else delay(0.0);
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
      rgpulse(p1,t5,rof1,rof2); 
    }  
/* The ROESY spin-lock unit is executed sixteen times within the
   hardware loop so that it is of sufficient duration to allow
   the acquisition hardware loop to be loaded in behind it on
   the last pass through the spin-lock loop. */

   else
    {
      obspower(tpwr);
      rgpulse(mix,t2,rof1,rof2);        /* cw spin lock  */
    }
   rcvron();
   status(C);
}
