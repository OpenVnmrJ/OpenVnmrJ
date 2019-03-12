/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* STD-ztocsy with dpfgse solvent suppression

Literature reference:
        T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995) dpfgse
        C. Dalvit; J. Biol. NMR, 11, 437-444 (1998) dpfgse
        M.J. Trippleton and J. Keeler;
             Angew. Chem. Int. Ed. 2003, 42 3938-3941. ZQ suppression
        M Mayer and B. Meyer: Angew. Chem. Int. Ed. 38, No12 p. 1784 1999.
                                  STD difference

Parameters:
        sspul      - flag for optional GRD-90-GRD steady state block
        gzlvl0     - gradient power level for sspul
        gt0        - gradient duration for sspul
        mix        - duration of the DIPSI-2 mixing along Z
                     (the pulse width and the corresponding power level
                      are internally calculated from strength and compH)
        strength   - amplitufe of the B1 mixing field (in Hz)
        compH      - 1H amplifier compression factor
        gt2        - homospoil gradient duration after mixing
        flipback   - flag for an optional selective 90 flipback pulse
                          on the solvent to keep it along z all time
        flipshape  - shape of the selective pulse for flipback='y'
        flippw     - pulse width of the selective pulse for flipback='y'
        flippwr    - power level of the selective pulse for flipback='y'
        flippwrf   - fine power for flipshape by default it is 2048
                        may need optimization with fixed flippw and flippwr
        phincr1    - small angle phase shift between the hard and the
                            selective pulse for flipback='y' (1 deg. steps)
                            to be optimized for best result
        wrefshape  - shape file of the 180 deg. selective refocussing pulse
                        on the solvent (may be convoluted for
                                multiple solvents)
        wrefpw     - pulse width for wrefshape (as given by Pbox)
        wrefpwr    - power level for wrefshape (as given by Pbox)
        wrefpwrf   - fine power for wrefshape
                      by default it is 2048 needs optimization for
                      multiple solvent with fixed wrefpw suppression only
        gt3        - gradient duration for the solvent suppression echo
        gzlvl3     - gradient power for the solvent suppression echo
        gstab   - gradient stabilization delay
        zqflg   - optional flag for ZQ suppression
                  The shaped pulses and gradient powers are
                  calculatred on-the-fly.
        alt_grd - alternate gradient sign(s) in ZQ-filter on even transients
        xferdly     - saturation transfer delay ( ~1.5-2sec)
        satshape    - shape of the pulses in the pulse-train (def:gauss)
        d3          - interpulse delay in the xfer pulsetrain
                        ( 1 msec is recommenden in the literature
                                with no obvious reason)
        satpwr      - power level for the saturation pulse-train
                       (in the literature 86 Hz peak power is recommended
                       corresponding to a 630 deg. flip angle at 50 msec satpw
                       please note that the actual flip angle is irrelevant,
                       the selectivity is controlled by the power level, satpwr)
        satpw       - pulse width of the shaped pulses in the pulse train
                            duration ca 50 ms
        satfrq      - frequency for protein saturation
                         (use a region with an intense protein signal
                         and NO ligand signal)
        satfrqref   - reference frequency (outside the signal region ~at 30ppm)
        trim_flg   - flag for an optional trim pulse at the end of the
                       sequence
                       set trim_flg='y' to suppress protein background
        trim       - pulse width of the trim pusle for trim_flg='y'
        trimpwr    - power level for the trim pulse for trim_flg='y'

 The parameters: gcal_local, coil_size and h1freq_local necessary
 for ZQ suppression are taken from the probe file by the setup macro or
 calculated automatically.
 The parameter swfactor controling the width of the frequency range to be
 refocused for the ZQ filter is set to 9.0 in the pulse sequence. For
 wide spectral windows the inversion range is limited to 60 kHZ to prevent
 dangerously high gradient levels to be set.
*/ 
 
#include <standard.h> 

 
void dipsi(phse1,phse2) 
codeint phse1,phse2; 
{ 
        double slpw5; 
        slpw5 = 1.0/(4.0*getval("strength")*18.0);

        rgpulse(64*slpw5,phse1,0.0,0.0); 
        rgpulse(82*slpw5,phse2,0.0,0.0); 
        rgpulse(58*slpw5,phse1,0.0,0.0); 
        rgpulse(57*slpw5,phse2,0.0,0.0); 
        rgpulse(6*slpw5,phse1,0.0,0.0); 
        rgpulse(49*slpw5,phse2,0.0,0.0); 
        rgpulse(75*slpw5,phse1,0.0,0.0); 
        rgpulse(53*slpw5,phse2,0.0,0.0); 
        rgpulse(74*slpw5,phse1,0.0,0.0); 
} 
 
   static int ph1[16] =  {0,0,2,2,0,0,2,2,1,1,3,3,1,1,3,3},
              ph2[32] =  {2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,
                          0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},
              ph3[16] =  {0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3},
              ph6[32] =  {2,2,2,2,0,0,0,0,3,3,3,3,1,1,1,1,
                          1,1,1,1,3,3,3,3,2,2,2,2,0,0,0,0},
              ph7[32] =  {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
                          2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3},
              ph8[16] =  {1,1,1,1,3,3,3,3,2,2,2,2,0,0,0,0},
              ph4[16] =  {0,2,2,0,2,0,0,2,1,3,3,1,3,1,1,3};

void pulsesequence() 
{ 
   int     iphase; 
   char    flipback[MAXSTR], flipshape[MAXSTR], sspul[MAXSTR], satshape[MAXSTR],
           zqshape1[MAXSTR],zqshape2[MAXSTR],
           wrefshape[MAXSTR],zqflg[MAXSTR],alt_grd[MAXSTR],trim_flg[MAXSTR];
 
   double  mix = getval("mix"), 
           gstab = getval("gstab"), 
           gzlvl2 = getval("gzlvl2"),
           gt2 = getval("gt2"),
           gzlvl3 = getval("gzlvl3"),
           gt3 = getval("gt3"), 
           gzlvl0 = getval("gzlvl0"), 
           gt0 = getval("gt0"), 
           strength = getval("strength"), /* spinlock field strength in Hz */
           phincr1 = getval("phincr1"), 
           satpwr = getval("satpwr"),
           satfrq = getval("satfrq"),
           satfrqref = getval("satfrqref"),
           satpw = getval("satpw"),
           d3 = getval("d3"),
           xferdly = getval("xferdly"),
           flippw = getval("flippw"), 
           flippwr = getval("flippwr"), 
           flippwrf = getval("flippwrf"),
           wrefpwr = getval("wrefpwr"),
           wrefpw = getval("wrefpw"),
           wrefpwrf = getval("wrefpwrf"),
           trim = getval("trim"),
           trimpwr = getval("trimpwr"),
           h1freq_local = getval("h1freq_local"),
           gcal_local = getval("gcal_local"),
           coil_size = getval("coil_size"),
           compH = getval("compH"), /* adjustment for H1 amp. compression */
           zqpwr1=getval("zqpwr1"),
           zqpw1=getval("zqpw1"),
           zqpwr2=getval("zqpwr2"),
           zqpw2=getval("zqpw2"),
           swfactor = 9.0,    /* do the adiabatic sweep over 9.0*sw  */
           slpw5, cycles0,cycles,gzlvlzq,slpwr,slpw,invsw,slpwra,corfact; 

/* LOAD AND INITIALIZE VARIABLES */ 
 
   getstr("flipback",flipback); 
   getstr("satshape",satshape);
   getstr("flipshape",flipshape); 
   getstr("sspul",sspul); 
   getstr("wrefshape", wrefshape);
   getstr("zqflg",zqflg);
   getstr("zqshape1",zqshape1);
   getstr("zqshape2",zqshape2);
   getstr("alt_grd",alt_grd);
   getstr("trim_flg", trim_flg);
   cycles0 = xferdly/(d3+satpw) + 0.5;
     initval(cycles0,v4);
   if (coil_size == 0) coil_size=16;
   invsw = sw*swfactor;
   if (invsw > 60000.0) invsw = 60000.0; /* do not exceed 60 kHz */
   invsw = invsw/0.97;     /* correct for end effects of the cawurst-20 shape */

   gzlvlzq=(invsw*h1freq_local*2349)/(gcal_local*coil_size*sfrq*1e+6);

   iphase = (int) (getval("phase") + 0.5); 
   if (phincr1 < 0.0) phincr1=360+phincr1; 
   initval(phincr1,v8); 
   if (alt_grd[0] == 'y') hlv(ct,v6); mod2(v6,v6); /* 0011 0011 ..... */
               /* alternate gradient sign between transients 1-2 and 3-4 ... */
 
   sub(ct,ssctr,v10);
   settable(t1,16,ph1);       getelem(t1,v10,v1);
   settable(t2,32,ph2);       getelem(t2,v10,v2);
   settable(t3,16,ph3);       getelem(t3,v10,v3);
   settable(t6,32,ph6);       getelem(t6,v10,v12);
   settable(t7,32,ph7);       getelem(t7,v10,v13);
   settable(t4,16,ph4);       getelem(t4,v10,oph);
   settable(t5,16,ph8);       getelem(t5,v10,v11);
 
   if (iphase == 2) { incr(v1); incr(v11); }
 
   /* selective H20 one-lobe sinc pulse */ 
   slpw = 1/(4.0 * strength) ;     /* spinlock field strength  */
   slpw5 = slpw/18.0; 
   slpwra = tpwr - 20.0*log10(slpw/(compH*pw));
   slpwr = (int) (slpwra + 0.5);
   corfact = exp((slpwr-slpwra)*2.302585/20); /* correct for rounding errors */
   if (corfact < 1.00) { slpwr=slpwr+1; corfact=corfact*1.12202; }

   cycles = (mix/(2072*slpw5));
   cycles = 2.0*(double)(int)(cycles/2.0);
   initval(cycles, v9);       /* v9 is the MIX loop count */

   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14); 
	add(v1, v14, v1);
	add(oph,v14,oph);
        add(v11,v14,v11);

/* BEGIN ACTUAL PULSE SEQUENCE CODE */ 
   status(A); 
      obspower(tpwr); obspwrf(4095.0);
      if (sspul[0] == 'y') 
        { 
         zgradpulse(gzlvl0,gt0); 
         rgpulse(pw,zero,rof1,rof1); 
         zgradpulse(gzlvl0,gt0); 
        } 

   if (d1 > xferdly) delay(d1-xferdly);

             /* set saturation frequencies */
   mod2(ct,v5);                /*  0 1 0 1 0 1 0 1 ..frequency  switch
                                  on every second transient */
   ifzero(v5); obsoffset(satfrq);
   elsenz(v5); obsoffset(satfrqref);
   endif(v5);

 /*  Start the selective saturation of protein */

    obspower(satpwr);
    if (cycles > 0.0)
   {
    starthardloop(v4);
      delay(d3);
      shaped_pulse(satshape,satpw,zero,rof1,rof1);
      endhardloop();
   }
   obspower(tpwr); obsoffset(tof);
   obsstepsize(45.0); 
   initval(7.0,v7); 
   xmtrphase(v7); 
       
   status(B); 
      rgpulse(pw, v1, rof1, 2.0e-6); 
      if (trim_flg[0] == 'y')
        { obspower(trimpwr);
          rgpulse(trim,v11,2.0e-6,2.0e-6);
          obspower(tpwr);
        }
     if (trim_flg[0] == 'y')
       {
        if (d2-2.0*pw/3.14 - 4.0e-6 - SAPS_DELAY - 2.0*POWER_DELAY> 0)
                   delay(d2-2.0*pw/3.14-4.0e-6-SAPS_DELAY - 2.0*POWER_DELAY);
        else
          delay(0.0);
       }
     else
       {
        if (d2-4.0*pw/3.14 - 4.0e-6 - SAPS_DELAY> 0)
                   delay(d2-4.0*pw/3.14-4.0e-6-SAPS_DELAY);
        else
          delay(0.0);
       } 
      xmtrphase(zero); 
 
      rgpulse(pw,v2,2.0e-6,rof1);  /*    magnetization now on "Z"   */
      ifzero(v6); zgradpulse(gzlvl2,gt2);
              elsenz(v6); zgradpulse(-gzlvl2,gt2); endif(v6);
      delay(gstab);
         if (zqflg[0] == 'y')
         {
            ifzero(v6); rgradient('z',gzlvlzq);
                  elsenz(v6); rgradient('z',-gzlvlzq); endif(v6);
            obspower(zqpwr1);
            shaped_pulse(zqshape1,zqpw1,zero,rof1,rof1);
            obspower(tpwr);
            rgradient('z',0.0);
            delay(gstab);
         }
      obspower(slpwr); obspwrf(4095.0/corfact);
      if (cycles > 1.0) 
        { 
         starthardloop(v9); 
             dipsi(one,three); dipsi(three,one); dipsi(three,one); dipsi(one,three); 
         endhardloop(); 
        } 
      delay(gstab); 
      obspwrf(4095.0);
         if (zqflg[0] == 'y')
         {
            ifzero(v6); rgradient('z',gzlvlzq);
                 elsenz(v6); rgradient('z',-gzlvlzq); endif(v6);
            obspower(zqpwr2);
            shaped_pulse(zqshape2,zqpw2,zero,rof1,rof1);
            obspower(tpwr);
            rgradient('z',0.0);
            delay(gstab);
         }
      ifzero(v6); zgradpulse(gzlvl2,1.5*gt2);
              elsenz(v6); zgradpulse(-gzlvl2,1.5*gt2); endif(v6);
      delay(gstab);
 
      if (flipback[A] == 'y') 
       { 
        obspower(flippwr+6); obspwrf(flippwrf);
        obsstepsize(1.0); 
        add(v3,two,v3);
        xmtrphase(v8); 
        shaped_pulse(flipshape,flippw,v3,rof1,rof1); 
        xmtrphase(zero); 
        add(v3,two,v3);
       } 

  obspower(tpwr); obspwrf(4095.0);
  rgpulse(pw,v3,rof1,rof1); 

         ifzero(v6); zgradpulse(gzlvl3,gt3);
              elsenz(v6); zgradpulse(-1.0*gzlvl3,gt3); endif(v6);
     obspower(wrefpwr+6); obspwrf(wrefpwrf);
     add(v12,two,v12);
     delay(gstab);
     shaped_pulse(wrefshape,wrefpw,v12,rof1,rof1);
     add(v12,two,v12);
     obspower(tpwr); obspwrf(4095.0);
     rgpulse(2.0*pw,v12,rof1,rof1);
         ifzero(v6); zgradpulse(gzlvl3,gt3);
              elsenz(v6); zgradpulse(-1.0*gzlvl3,gt3); endif(v6);
     obspower(wrefpwr+6); obspwrf(wrefpwrf);
     add(v13,two,v13);
     delay(gstab);
         ifzero(v6); zgradpulse(1.2*gzlvl3,gt3);
              elsenz(v6); zgradpulse(-1.2*gzlvl3,gt3); endif(v6);
     delay(gstab);
     shaped_pulse(wrefshape,wrefpw,v13,rof1,rof1);
     add(v13,two,v13);
     obspower(tpwr); obspwrf(4095.0);
     rgpulse(2.0*pw,v13,rof1,rof2);
         ifzero(v6); zgradpulse(1.2*gzlvl3,gt3);
              elsenz(v6); zgradpulse(-1.2*gzlvl3,gt3); endif(v6);
     delay(gstab);
     status(C); 
} 
