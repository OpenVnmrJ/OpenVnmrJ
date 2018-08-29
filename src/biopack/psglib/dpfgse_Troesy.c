/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dpfgse_Troesy.c - rotating frame NOE experiment with T-Roesy modification 
              allows T-Roesy or standard cw Roesy

             OBSERVE TRANSMITTER SHOULD BE SET AT SOLVENT POSITION
             SATURATION,SPIN LOCK AND PULSES ALL USE OBS.XMTR

        pw = 90 degree pulse on protons (power level at "tpwr")
      tpwr = power level for the spin lock pulse(s)
  strength = amplitufe of the B1 mixing field (in Hz)
     compH = 1H amplifier compression factor
     T_flg = 'y' gives pulsed T-Roesy; 'n': cw Roesy
     phase = 1,2: F1 quadrature by the hypercomplex method
       mix = mixing time
 wrefshape = shape file of the 180 deg. selective refocussing pulse
             on the solvent (may be convoluted for
             multiple solvents)
   wrefpw  = pulse width for wrefshape (as given by Pbox)
   wrefpwr = power level for wrefshape (as given by Pbox)
  wrefpwrf = fine power for wrefshape
                by default it is 2048 needs optimization for
                multiple solvent with fixed wrefpw suppression only
       gt2 = gradient duration for the solvent suppression echo
    gzlvl2 = gradient power for the solvent suppression echo
   alt_grd = flag for alternating gradient sign in mix-dpfgse segment
     gstab = gradient stabilization delay
     sspul = 'y': activates gradient-90degree-gradient pulse prior to d1
       gt1 = gradient pulse duration for sspul
    gzlvl1 = gradient pulse amplitude for sspul
   satmode = activates transmitter presat at satfrq 
             (satfrq = tof; satmode='nnn' or satmode='ynn')
    satdly = length of saturation during relaxation delay  
    satpwr = power level for solvent saturation
  trim_flg = flag for an optional trim pulse at the end of the
                  sequence
                  set trim_flg='y' tu suppress protein background
      trim = pulse width of the trim pusle for trim_flg='y'
   trimpwr = power level for the trim pulse for trim_flg='y'
        nt = min:  multiple of 4; max:  multiple of 8

   Revised from roesy.c  G. Gray  Sept 1991  Palo Alto  
   Troesy version: ech jan 95 dast
   dpfgse added p.s. 2004
*/


#include <standard.h>
static int phi1[8] = {0,0,2,2,1,1,3,3},
           phi2[8] = {0,0,0,0,1,1,1,1},
           phi3[8] = {2,2,2,2,3,3,3,3},
           phi4[8] = {1,1,3,3,2,2,0,0},
           phi5[8] = {1,3,1,3,2,0,2,0},
           phi6[8] = {3,1,3,1,0,2,0,2},
           phi7[8] = {1,1,1,1,2,2,2,2};

pulsesequence()
{
   double          phase = getval("phase"),
                   mix = getval("mix"),
                   wrefpwr = getval("wrefpwr"),
                   wrefpw = getval("wrefpw"),
                   wrefpwrf = getval("wrefpwrf"),
                   gt1 = getval("gt1"),
                   gzlvl1 = getval("gzlvl1"),
                   gt2 = getval("gt2"),
                   gzlvl2 = getval("gzlvl2"),
                   gstab = getval("gstab"),
                   trimpwr = getval("trimpwr"),
                   trim = getval("trim"),
                   compH = getval("compH"),
                   strength = getval("strength"), /* spinlock field strength in Hz */
                   cycles, d2corr, corfact, slpw90, slpwr, slpwra;
   int             iphase;
   char            sspul[MAXSTR],T_flg[MAXSTR], trim_flg[MAXSTR],
                   wrefshape[MAXSTR],alt_grd[MAXSTR];


/* LOAD AND INITIALIZE PARAMETERS */
   iphase = (int) (phase + 0.5);
   satdly = getval("satdly");
   satpwr = getval("satpwr");
   satfrq = getval("satfrq");
   getstr("sspul", sspul);
   getstr("satmode", satmode);
   getstr("T_flg", T_flg);
   getstr("wrefshape", wrefshape);
   getstr("alt_grd",alt_grd);
   getstr("trim_flg", trim_flg);
   rof1 = getval("rof1"); if(rof1 > 2.0e-6) rof1=2.0e-6;

/* CALCULATE PHASES AND INITIALIZE LOOP COUNTER FOR MIXING TIME */
   settable(t1,8,phi1);
   settable(t2,8,phi2);
   settable(t3,8,phi3);
   settable(t4,8,phi4);
   settable(t5,8,phi5);
   settable(t6,8,phi6);
   settable(t7,8,phi7);
   getelem(t1,ct,v1);
   getelem(t7,ct,v7);
   assign(v1,oph);	
   if (iphase == 2)
    {
      incr(v7);
      incr(v1);			/* BC2D hypercomplex method */
    }

/* FOR HYPERCOMPLEX, USE REDFIED TRICK TO MOVE AXIALS TO EDGE */  
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v9); /* moves axials */
   if ((iphase==2)||(iphase==1)) {add(v1,v9,v1); add(v7,v9,v7); add(oph,v9,oph);}

   if (alt_grd[0] == 'y') mod2(ct,v6);
               /* alternate gradient sign on every 2nd transient */

    /* CALCULATE SPIN>LOCK POWER AND PULSE WIDTHS        */

    slpw90 = 1/(4.0 * strength) ;     /* spinlock field strength  */
  /*  slpw1 = slpw90/90.0; */
    slpwra = tpwr - 20.0*log10(slpw90/(compH*pw));
    slpwr = (int) (slpwra + 0.5);
    corfact = exp((slpwr-slpwra)*2.302585/20);
    if (corfact < 1.00) { slpwr=slpwr+1; corfact=corfact*1.12202; }

   cycles = mix / (16.0 * (4e-6 + 2.0*slpw90));
   initval(cycles, v10);	/* mixing time cycles */

/* BEGIN ACTUAL PULSE SEQUENCE */
   status(C);
   obspower(tpwr); obspwrf(4095.0);
   if (sspul[A] == 'y')
   {
     zgradpulse(gzlvl1,gt1);
     delay(5.0e-5);
     rgpulse(pw,zero,rof1,rof1);
     zgradpulse(gzlvl1,gt1); 
     delay(5.0e-5);
   }
   status(A);
   if (satmode[A] == 'y') 
    {
    if (d1 > satdly) delay(d1-satdly);
    if (tof != satfrq) obsoffset(satfrq);
    obspower(satpwr);
    rgpulse(satdly,zero,rof1,rof1);
    obspower(tpwr);
    if (tof != satfrq) obsoffset(tof);
    }
    else delay(d1);
   status(B);
   obsstepsize(45.0);
   initval(7.0,v4);  
   xmtrphase(v4);
   rgpulse(pw,v1,rof1,1.0e-6);
   if (trim_flg[0] == 'y')
        { obspower(trimpwr);
          rgpulse(trim,v7,rof1,rof1);
          obspower(tpwr);
        }
   xmtrphase(zero);   
   if (T_flg[0] == 'n')
        d2corr = rof1 + 1.0e-6 + (2*pw/3.1416) + SAPS_DELAY;
   else
        d2corr = rof1 + 1.0e-6 + (4*pw/3.1416) + SAPS_DELAY;
   if (d2 > d2corr) delay(d2 - d2corr); else delay(0.0);
   if ((T_flg[0] == 'y')&&(cycles > 1.5))
    {
      rgpulse(pw,t4,rof1,rof1);
      obspower(slpwr); obspwrf(4095.0/corfact);
      {
         starthardloop(v10);
            rgpulse(2.0*slpw90,t2,4e-6,0.0);
            rgpulse(2.0*slpw90,t3,4e-6,0.0);
            rgpulse(2.0*slpw90,t2,4e-6,0.0);
            rgpulse(2.0*slpw90,t3,4e-6,0.0);
            rgpulse(2.0*slpw90,t2,4e-6,0.0);
            rgpulse(2.0*slpw90,t3,4e-6,0.0);
            rgpulse(2.0*slpw90,t2,4e-6,0.0);
            rgpulse(2.0*slpw90,t3,4e-6,0.0);
            rgpulse(2.0*slpw90,t2,4e-6,0.0);
            rgpulse(2.0*slpw90,t3,4e-6,0.0);
            rgpulse(2.0*slpw90,t2,4e-6,0.0);
            rgpulse(2.0*slpw90,t3,4e-6,0.0);
            rgpulse(2.0*slpw90,t2,4e-6,0.0);
            rgpulse(2.0*slpw90,t3,4e-6,0.0);
            rgpulse(2.0*slpw90,t2,4e-6,0.0);
            rgpulse(2.0*slpw90,t3,4e-6,0.0);
         endhardloop();
      }
      obspower(tpwr); obspwrf(4095.0);
      rgpulse(pw,t5,rof1,rof1); 
    }  
/* The ROESY spin-lock unit is executed sixteen times within the
   hardware loop so that it is of sufficient duration to allow
   the acquisition hardware loop to be loaded in behind it on
   the last pass through the spin-lock loop. */

   else
    {
      obspower(slpwr); obspwrf(4095.0/corfact);
      rgpulse(mix,t2,rof1,rof1);        /* cw spin lock  */
      obspower(tpwr); obspwrf(4095.0);
    }
/* DPFGSE solvent suppression  */
         ifzero(v6); zgradpulse(gzlvl2,gt2);
              elsenz(v6); zgradpulse(-1.0*gzlvl2,gt2); endif(v6);
     obspower(wrefpwr+6); obspwrf(wrefpwrf);
     delay(gstab);
     shaped_pulse(wrefshape,wrefpw,t5,rof1,rof1);
     obspower(tpwr); obspwrf(4095.0);
     rgpulse(2.0*pw,t6,rof1,rof1);
         ifzero(v6); zgradpulse(gzlvl2,gt2);
              elsenz(v6); zgradpulse(-1.0*gzlvl2,gt2); endif(v6);
     obspower(wrefpwr+6); obspwrf(wrefpwrf);
     delay(gstab);
         ifzero(v6); zgradpulse(1.2*gzlvl2,gt2);
              elsenz(v6); zgradpulse(-1.2*gzlvl2,gt2); endif(v6);
     delay(gstab);
     shaped_pulse(wrefshape,wrefpw,t5,rof1,rof1);
     obspower(tpwr); obspwrf(4095.0);
     rgpulse(2.0*pw,t6,rof1,rof2);
         ifzero(v6); zgradpulse(1.2*gzlvl2,gt2);
              elsenz(v6); zgradpulse(-1.2*gzlvl2,gt2); endif(v6);
     delay(gstab);
   status(C);
}
