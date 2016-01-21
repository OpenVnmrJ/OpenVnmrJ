/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* cleanexp-pm - 1D-ROE experiment to detect
                 water - protein exchange and intermolecular NOEs
Literature:  Tsang-Lin Hwang, Susumu Mori, A.J. Shaka and P. C. M. van Zijl:
                  JACS, 119. 6203-6204 1997.

Parameters:
        sspul           - flag for optional GRD-90-GRD steady-state sequence
        gt0             - gradient duration for sspul
        gzlvl0          - gradient power for sspul
        strength        - RF amplitude of the CLEANEX-PM sequence
        mix             - mixing time - the length of the CLEANEX-PM sequence
        gzlvlhs         - gradient power for the HS pulse during mix (< 1G/cm)
        wselshape       - shape file of the 180 deg. H2O selection pulse       
        wselpw          - pulse width for wselshape (as given by Pbox)
        wselpwr         - power level for wselshape (as given by Pbox)
        alt_grd         - alternate gradient sign(s) on even transients
        gt1             - gradient duration for the solvent selection echo
        gzlvl1          - gradient power for the solvent selection echo
        dpfgse          - flag to select dpfgse water suppression
        wrefshape       - shape file of the 180 deg. selective refocussing pulse
                                on the solvent (only used with dpfgse='y')
        wrefpw          - pulse width for wrefshape (as given by Pbox) 
                                       (only used with dpfgse='y')
        wrefpwr         - power level for wrefshape (as given by Pbox)
                                       (only used with dpfgse='y')
        wrefpwrf        - fine power for wrefshape
                                by default it is 2048
                                       (only used with dpfgse='y')
        flipback    - flag for an optional selective 90 flipback pulse
                                on the solvent to keep it along z all time
        flipshape   - shape of the selective pulse for flipback='y'
        flippw      - pulse width of the selective pulse for flipback='y'
        flippwr     - power level of the selective pulse for flipback='y'
        flippwrf    - fine power for flipshape by default it is 2048
                        may need optimization with fixed flippw and flippwr
        phincr1     - small angle phase shift between the hard and the
                            selective pulse for flipback='y' (1 deg. steps)
                            to be optimized for best result
        gt2             - gradient duration for the solvent suppression echo
        gzlvl2          - gradient power for the solvent suppression echo
        gstab           - gradient stabilization delay
        wg3919          - flag for watergate 3-9-19 water supr. block
        d3              - delay in WATERGATE 3-9-19      
        trim_flg	- flag for an optional trim pulse at the end of the
				sequence to suppress protein background
        trim		- length of the trim pulse for trim_flg='y'
        trimpwr		- power level for trim if trim_flg='y'
        tof             - transmitter offset (set on resonance for H2O)
        compH           - amplifier compression factor for 1H

The water selection shape and the water refocusing and the water flipback shape 
can be created/updatedted using the "BPupdate_wselshape", "BPupdate_wrefshape" and
and BPupdate_flipshape" macros, respectively.

 For multiple frequency solvent
suppression the wrefshape file needs to be created manually.	

p. sandor, darmstadt dec 2004.
modified for BioPack aug 2007, G.Gray Varian Palo Alto
*/	  

#include <standard.h>

cleanex(phse1,phse2)
codeint phse1,phse2;
{
        double slpw1;
        slpw1 = 1.0/(4.0*getval("strength")*90.0);

        rgpulse(135*slpw1,phse1,2.0e-6,0.0);
        rgpulse(120*slpw1,phse2,2.0e-6,0.0);
        rgpulse(110*slpw1,phse1,2.0e-6,0.0);
        rgpulse(110*slpw1,phse2,2.0e-6,0.0);
        rgpulse(120*slpw1,phse1,2.0e-6,0.0);
        rgpulse(135*slpw1,phse2,2.0e-6,0.0);
}

static int phi1[1]  = {0},
           phi2[4]  = {0,1,2,3},
           phi3[1]  = {0},
           phi5[4]  = {0,2,0,2},
           phi6[1]  = {0},
            rec[4]  = {0,2,0,2};

pulsesequence ()
{

double  gstab = getval("gstab"),
        gt0 = getval("gt0"),
        gzlvl0 = getval("gzlvl0"),
	gt1 = getval("gt1"),
        gzlvl1 = getval("gzlvl1"),
        gt2 = getval("gt2"),
	gzlvl2 = getval("gzlvl2"),
        gzlvlhs = getval("gzlvlhs"),
        wselpw = getval("wselpw"),
        wselpwr = getval("wselpwr"),
        mix = getval("mix"),
        strength = getval("strength"), /* spinlock field strength in Hz */
        wrefpw = getval("wrefpw"),
        wrefpwr = getval("wrefpwr"),
        wrefpwrf = getval("wrefpwrf"),
        trim = getval("trim"),
        trimpwr = getval("trimpwr"),
        compH = getval("compH"),
        flippwr = getval("flippwr"),
        flippwrf = getval("flippwrf"),
        flippw = getval("flippw"),
        phincr1 = getval("phincr1"),
        d3 = getval("d3"),
        slpwr,slpw90,slpw1,cycles,flipdel,slpwra,corfact;

char    wrefshape[MAXSTR], wselshape[MAXSTR], sspul[MAXSTR],wg3919[MAXSTR],
	trim_flg[MAXSTR],alt_grd[MAXSTR],flipshape[MAXSTR],flipback[MAXSTR],
        dpfgse[MAXSTR];

  rof1 = getval("rof1"); if(rof1 > 2.0e-6) rof1 = 2.0e-6;
  getstr("wrefshape", wrefshape);
  getstr("wselshape", wselshape);
  getstr("flipshape", flipshape);
  getstr("flipback", flipback);
  getstr("wg3919",wg3919);
  getstr("dpfgse",dpfgse);
  getstr("sspul", sspul);
  getstr("trim_flg", trim_flg);
  getstr("alt_grd",alt_grd);
  if (phincr1 < 0.0) phincr1=360+phincr1;
  initval(phincr1,v9);
  flipdel = flippw + 2*rof1 + 4*POWER_DELAY + 2*SAPS_DELAY;
  if((wg3919[A] == 'y' && dpfgse[A] == 'y' ))
  { text_error("Incorrect flags! Select either the wg3919 or dpfgse flag  ");
         psg_abort(1); }
  if((wg3919[A] == 'n' && dpfgse[A] == 'n' ))
  { text_error("Incorrect flags! Select either wg3919 or dpfgse to 'y'   ");
         psg_abort(1); }

   slpw90 = 1/(4.0 * strength) ;     /* spinlock field strength  */
   slpw1 = slpw90/90.0;
   slpwra = tpwr - 20.0*log10(slpw90/(compH*pw));
   slpwr = (int) (slpwra + 0.5);
   corfact = exp((slpwr-slpwra)*2.302585/20);
   if (corfact < 1.00) { slpwr=slpwr+1; corfact=corfact*1.12202; }

   cycles = (mix/((730*slpw1)+1.2e-5));
   cycles = 2.0*(double)(int)(cycles/2.0);
   initval(cycles, v11);       /* v11 is the MIX loop count */

   settable(t1, 1,phi1);
   settable(t2, 4,phi2);
   settable(t3, 1,phi3);
   settable(t5, 4,phi5);
   settable(t6, 1,phi6);
   settable(t7, 4,rec);

   sub(ct,ssctr,v12);

   getelem(t1,v12,v1);
   getelem(t2,v12,v2);
   getelem(t3,v12,v3);
   add(v3,two,v4);
   getelem(t5,v12,v5);
   getelem(t6,v12,v6);
   add(v6,two,v7);
   getelem(t7,v12,oph);
   if (alt_grd[0] == 'y') mod2(ct,v8);
                     /* alternate gradient sign on every 2nd transient */

status(A);
   decpower(dpwr); obspower(tpwr);
   if (sspul[0] == 'y')
      {
        zgradpulse(gzlvl0,gt0);
        rgpulse(pw,zero,rof1,rof1);
        zgradpulse(gzlvl0,gt0);
        delay(gstab);
      }
   delay(d1);
status(B);
   rgpulse(pw,v1,rof1,rof1);
     ifzero(v8); zgradpulse(gzlvl1,gt1);
       elsenz(v8); zgradpulse(-1.0*gzlvl1,gt1); endif(v8);
   obspower(wselpwr);
   delay(gstab);
   shaped_pulse(wselshape,wselpw,v2,10.0e-6,rof1);
     ifzero(v8); zgradpulse(gzlvl1,gt1);
       elsenz(v8); zgradpulse(-1.0*gzlvl1,gt1); endif(v8);
   obspower(slpwr);   obspwrf(4095.0/corfact);
   delay(gstab);
                              /* do mixing */
    ifzero(v8); rgradient('z',gzlvlhs); /*make it compatible with cold probes*/
      elsenz(v8); rgradient('z',-1.0*gzlvlhs); endif(v8);
   if (cycles > 1.0)
      {
       starthardloop(v11);
           cleanex(v3,v4); 
       endhardloop();
      }
   rgradient('z',0.0);
   obspwrf(4095.0);

   if (flipback[A] == 'y')
      {
      obsstepsize(1.0);
      xmtrphase(v9);
      obspower(flippwr+6); obspwrf(flippwrf);
      shaped_pulse(flipshape,flippw,v5,rof1,rof1);
      xmtrphase(zero);
      obspower(tpwr); obspwrf(4095.0);
      }
                 /*  suppress solvent */
 if (dpfgse[A] == 'y')   /* Set up DPFGSE */
  {
         ifzero(v8); zgradpulse(gzlvl2,gt2);
          elsenz(v8); zgradpulse(-1.0*gzlvl2,gt2); endif(v8);
      obspower(wrefpwr+6); obspwrf(wrefpwrf);
      delay(gstab);
      shaped_pulse(wrefshape,wrefpw,v6,10.0e-6,rof1);
      obspower(tpwr); obspwrf(4095.0);
      rgpulse(2.0*pw,v7,rof1,rof1);
        ifzero(v8); zgradpulse(gzlvl2,gt2);
          elsenz(v8); zgradpulse(-1.0*gzlvl2,gt2); endif(v8);
      delay(gstab);
        ifzero(v8); zgradpulse(1.2*gzlvl2,gt2);
          elsenz(v8); zgradpulse(-1.2*gzlvl2,gt2); endif(v8);
      obspower(wrefpwr+6); obspwrf(wrefpwrf);
      delay(gstab);
      shaped_pulse(wrefshape,wrefpw,v6,10.0e-6,rof1);
      obspower(tpwr); obspwrf(4095.0);
      if (trim_flg[A] == 'y') rgpulse(2.0*pw,v7,rof1,0.0);
          else        rgpulse(2.0*pw,v7,rof1,rof2);
        ifzero(v8); zgradpulse(1.2*gzlvl2,gt2);
          elsenz(v8); zgradpulse(-1.2*gzlvl2,gt2); endif(v8);
      delay(gstab); 
  }
  if (wg3919[A] == 'y')       /* Set up hard WATERGATE */
  {
       ifzero(v8); zgradpulse(gzlvl2,gt2);
         elsenz(v8); zgradpulse(-1*gzlvl2,gt2); endif(v8);
       delay(gstab);
       rgpulse(pw*0.231,v6,rof1,rof1);
       delay(d3);
       rgpulse(pw*0.692,v6,rof1,rof1);
       delay(d3);
       rgpulse(pw*1.462,v6,rof1,rof1);
       delay(d3);
       rgpulse(pw*1.462,v7,rof1,rof1);
       delay(d3);
       rgpulse(pw*0.692,v7,rof1,rof1);
       delay(d3);
       if (trim_flg[A] == 'y') rgpulse(0.231*pw,v7,rof1,rof1);
          else rgpulse(pw*0.231,v7,rof1,rof2);
       ifzero(v8); zgradpulse(gzlvl2,gt2);
          elsenz(v8); zgradpulse(-1*gzlvl2,gt2); endif(v8);
       delay(gstab);
  }
  if (flipback[A] =='y') delay(flipdel);
       if (trim_flg[A] == 'y')  /* suppress protein background */
           { obspower(trimpwr);
             add(v6,one,v10);
            rgpulse(trim,v10,rof1,rof2);
           }
status(C);
}
