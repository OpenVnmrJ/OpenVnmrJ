/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dpfgse_TroesyCN - rotating frame NOE experiment with T-Roesy modification 
              allows T-Roesy or standard cw Roesy
              and C13 and/or N15 decoupling for labeled samples

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
        nt = min:  multiple of 4; max:  multiple of 8
  C13refoc = flag for C13 decoupling in F1
       pwC = 90 deg. C13 pulse for F1-C13 decoupling
    pwClvl = C13 power level for pwC
  N15refoc = flag for N15 decoupling in F1 (using 3rd channel)
       pwN = 90 deg. N15 pulse for F1-N15 decoupling
    pwNlvl = N15 power level for pwN
   CNrefoc = flag for simultaneous C13 and N15 dec. in F1
              (for safety reasons both power levels are automatically
               droped by 3dB for the simultaneous pulses and
               the corresponding pulse widths (pwC and pwN) are also
               internaly corrected)
  F2 decoupling is set by the ususal decupling parameters
              (adiabatic decouplings schemes are recommended)

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
           phi6[8] = {3,1,3,1,0,2,0,2};

void pulsesequence()
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
                   compH = getval("compH"),
                   strength = getval("strength"), /* spinlock field strength in Hz */
                   pwN = getval("pwN"),
                   pwNlvl = getval("pwNlvl"),
                   pwC = getval("pwC"),
                   pwClvl = getval("pwClvl"),
                   cycles, corfact, slpw90, slpwr, slpwra;
   int             iphase;
   char            sspul[MAXSTR],T_flg[MAXSTR],C13refoc[MAXSTR],
                   N15refoc[MAXSTR],CNrefoc[MAXSTR], 
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
   getstr("C13refoc",C13refoc);
   getstr("N15refoc",N15refoc);
   getstr("CNrefoc",CNrefoc);
   rof1 = getval("rof1"); if(rof1 > 2.0e-6) rof1=2.0e-6;

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
     {
      incr(v1);			/* BC2D hypercomplex method */
      incr(v2);
     }

/* FOR HYPERCOMPLEX, USE REDFIED TRICK TO MOVE AXIALS TO EDGE */  
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v9); /* moves axials */
   if ((iphase==2)||(iphase==1)) {add(v1,v9,v1); add(oph,v9,oph);}

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
   status(A);
   obspower(tpwr); obspwrf(4095.0);
   if (sspul[A] == 'y')
   {
     zgradpulse(gzlvl1,gt1);
     delay(5.0e-5);
     rgpulse(pw,zero,rof1,rof1);
     zgradpulse(gzlvl1,gt1); 
     delay(5.0e-5);
   }
   decpower(pwClvl); dec2power(pwNlvl); decphase(zero); dec2phase(zero);
   if (CNrefoc[A] == 'y')
     {
      decpower(pwClvl-3.0); pwC=1.4*pwC;
      dec2power(pwNlvl-3.0); pwN=1.4*pwN;
     }
   obspower(tpwr); obspwrf(4095.0);
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
   rgpulse(pw,v1,rof1,rof1);
   xmtrphase(zero);   
   if (d2>0.0)
       {
        if ((C13refoc[A]=='n')&&(N15refoc[A]=='n')&&(CNrefoc[A]=='n'))
        {
         if (d2/2.0 > 2.0*pw/PI + rof1)
           delay(d2-4.0*pw/PI-SAPS_DELAY-2.0*rof1);
         else delay(0.0);
        }

        if ((C13refoc[A]=='n')&&(N15refoc[A]=='n')&&(CNrefoc[A]=='y'))
         {
         if (pwN > 2.0*pwC)
          {
           if (d2/2.0 > (pwN + 2.0*pw/PI + rof1))
            {
             delay(d2/2.0-pwN-2.0*pw/PI-SAPS_DELAY-rof1);
             dec2rgpulse(pwN-2.0*pwC,zero,0.0,0.0);
             sim3pulse(0.0,pwC,pwC, zero,zero,zero, 0.0, 0.0);
             sim3pulse(0.0,2.0*pwC,2.0*pwC, zero,one,zero, 0.0, 0.0);
             sim3pulse(0.0,pwC,pwC, zero,zero,zero, 0.0, 0.0);
             dec2rgpulse(pwN-2.0*pwC,zero,0.0,0.0);
             delay(d2/2.0-pwN-2.0*pw/PI-rof1);
            }
           else
             delay(d2-4.0*pw/PI-SAPS_DELAY-2.0*rof1);
          }
         else
          {
           if (d2/2.0 > (pwN + pwC+ 2.0*pw/PI + rof1))
            {
             delay(d2/2.0-pwN-pwC-2.0*pw/PI-SAPS_DELAY-rof1);
             decrgpulse(pwC,zero,0.0,0.0);
             sim3pulse(0.0,2.0*pwC,2.0*pwN, zero,one,zero, 0.0, 0.0);
             decrgpulse(pwC,zero,0.0,0.0);
             delay(d2/2.0-pwN-pwC-2.0*pw/PI-rof1);
            }
           else
             delay(d2-4.0*pw/PI-SAPS_DELAY-2.0*rof1);
          }
         }

        if ((C13refoc[A]=='n')&&(N15refoc[A]=='y')&&(CNrefoc[A]=='n'))
         {
          if (d2/2.0 > (pwN + 2.0*pw/PI + rof1))
           {
            delay(d2/2.0-pwN-2.0*pw/PI-SAPS_DELAY-rof1);
            dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
            delay(d2/2.0-pwN-2.0*pw/PI-rof1);
           }
          else
            delay(d2-4.0*pw/PI-SAPS_DELAY-2.0*rof1);
         }

        if ((C13refoc[A]=='y')&&(N15refoc[A]=='n')&&(CNrefoc[A]=='n'))
         {
         if (d2/2.0 > (2.0*pwC + 2.0*pw/PI + rof1))
          {
           delay(d2/2.0-2.0*pwC-2.0*pw/PI-rof1-SAPS_DELAY);
           decrgpulse(pwC,zero,0.0,0.0);
           decrgpulse(2.0*pwC, one, 0.0, 0.0);
           decrgpulse(pwC,zero,0.0,0.0);
           delay(d2/2.0-2.0*pwC-2.0*pw/PI-rof1);
          }
          else
            delay(d2-4.0*pw/PI-SAPS_DELAY-2.0*rof1);
         }
      } else delay(0.0);
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
    decpower(dpwr); dec2power(dpwr2);
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
