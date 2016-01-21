/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* wlogsy_noe - 1D-NOE ePHOGSY experiment for detection
                of ligands with WaterLOGSY
Literature:  C. Dalvit et al. J. Biomol. NMR, 18, 65-68 (2000).
             C. Dalvit, G. Fogliatto, A. Stewart, M. Veronesi & B. Stockman:
			J. Biomol. NMR 21, 349-359 (2001).

Parameters:
        sspul           - flag for optional GRD-90-GRD steady-state sequence
        gt0             - gradient duration for sspul
        gzlvl0          - gradient power for sspul
        mix             - mixing time
        gzlvlhs         - gradient power for the HS pulse during mix (< 1G/cm)
        led             - longitudinal Eddy-current compensation delay (2 msec)
        wselshape       - shape file of the 180 deg. H2O selection pulse       
        wselpw          - pulse width for wselshape (as given by Pbox)
        wselpwr         - power level for wselshape (as given by Pbox)
        alt_grd         - alternate gradient sign(s) on even transients
        gt1             - gradient duration for the solvent selection echo
        gzlvl1          - gradient power for the solvent selection echo
        wrefshape       - shape file of the 180 deg. selective refocussing pulse
                                on the solvent (may be convoluted for
                                                multiple solvents)
        wrefpw          - pulse width for wrefshape (as given by Pbox)
        wrefpwr         - power level for wrefshape (as given by Pbox)
        wrefpwrf        - fine power for wrefshape
                                by default it is 2048
                                needs optimization for multiple solvent
                                        with fixed wrefpw suppression only
        gt2             - gradient duration for the solvent suppression echo
        gzlvl2          - gradient power for the solvent suppression echo
        gstab           - gradient stabilization delay
        trim_flg	- flag for an optional trim pulse at the end of the
				sequence to suppress protein background
        trim		- length of the trim pulse for trim_flg='y'
        trimpwr		- power level for trim if trim_flg='y'
        tof             - transmitter offset (set on resonance for H2O)
        compH           - amplifier compression factor for 1H

The water selection shape and the water refocusing shape can be created/updatedted
using the "update_wselshape" and "update_wrefshape" macros, respectively. For
multiple frequency solvent suppression the wrefshape file needs to be created
manually.

p. sandor, darmstadt june 2003.

*/	  

#include <standard.h>

static int phi1[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
           phi2[16] = {0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1},
           phi3[16] = {2,2,2,2,3,3,3,3,2,2,2,2,3,3,3,3},
           phi4[16] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},
           phi5[16] = {2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3},
           phi9[16] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3},
           rec[16]  = {0,2,0,2,2,0,2,0,2,0,2,0,0,2,0,2};

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
        led = getval("led"),
        wrefpw = getval("wrefpw"),
        wrefpwr = getval("wrefpwr"),
        wrefpwrf = getval("wrefpwrf"),
        trim = getval("trim"),
        trimpwr = getval("trimpwr");

char    wrefshape[MAXSTR], wselshape[MAXSTR], sspul[MAXSTR],
	trim_flg[MAXSTR],alt_grd[MAXSTR]; 

rof1 = getval("rof1"); if(rof1 > 2.0e-6) rof1 = 2.0e-6;
  getstr("wrefshape", wrefshape);
  getstr("wselshape", wselshape);
  getstr("sspul", sspul);
  getstr("trim_flg", trim_flg);
  getstr("alt_grd",alt_grd);
   settable(t1,16,phi1);
   settable(t2,16,phi2);
   settable(t3,16,phi3);
   settable(t4,16,phi4);
   settable(t5,16,phi5);
   settable(t9,16,phi9);
   settable(t6,16,rec);

   sub(ct,ssctr,v12);

   getelem(t1,v12,v1);
   getelem(t2,v12,v2);
   getelem(t3,v12,v3);
   getelem(t4,v12,v4);
   getelem(t5,v12,v5);
   getelem(t9,v12,v9);
   getelem(t6,v12,oph);
   add(v1,one,v10);
   if (alt_grd[0] == 'y') mod2(ct,v6);
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
   rgpulse(pw,zero,rof1,rof1);
     ifzero(v6); zgradpulse(gzlvl1,gt1);
       elsenz(v6); zgradpulse(-1.0*gzlvl1,gt1); endif(v6);
   obspower(wselpwr);
   delay(gstab);
   shaped_pulse(wselshape,wselpw,v9,10.0e-6,rof1);
     ifzero(v6); zgradpulse(gzlvl1,gt1);
       elsenz(v6); zgradpulse(-1.0*gzlvl1,gt1); endif(v6);
   obspower(tpwr);
   delay(gstab);
                              /* do mixing */
   rgpulse(pw,zero,rof1,rof1);
      ifzero(v6); zgradpulse(gzlvlhs,mix); /* make it compatible with cold probes */
        elsenz(v6); zgradpulse(-1.0*gzlvlhs,mix); endif(v6);
   delay(gstab);
   delay(led);
   rgpulse(pw,v1,rof1,rof1);
                             /* suppress solvent */
     ifzero(v6); zgradpulse(gzlvl2,gt2);
       elsenz(v6); zgradpulse(-1.0*gzlvl2,gt2); endif(v6);
   obspower(wrefpwr+6); obspwrf(wrefpwrf);
   delay(gstab);
   shaped_pulse(wrefshape,wrefpw,v5,10.0e-6,rof1);
   obspower(tpwr); obspwrf(4095.0);
   rgpulse(2.0*pw,v4,rof1,rof1);
     ifzero(v6); zgradpulse(gzlvl2,gt2);
       elsenz(v6); zgradpulse(-1.0*gzlvl2,gt2); endif(v6);
   delay(gstab);
     ifzero(v6); zgradpulse(1.2*gzlvl2,gt2);
       elsenz(v6); zgradpulse(-1.2*gzlvl2,gt2); endif(v6);
   obspower(wrefpwr+6); obspwrf(wrefpwrf);
   delay(gstab);
   shaped_pulse(wrefshape,wrefpw,v3,10.0e-6,rof1);
   obspower(tpwr); obspwrf(4095.0);
   if (trim_flg[A] == 'y') rgpulse(2.0*pw,v2,rof1,0.0);
       else        rgpulse(2.0*pw,v2,rof1,rof2);
     ifzero(v6); zgradpulse(1.2*gzlvl2,gt2);
       elsenz(v6); zgradpulse(-1.2*gzlvl2,gt2); endif(v6);
   delay(gstab); 
       if (trim_flg[A] == 'y')  /* suppress protein background */
           { obspower(trimpwr);
             add(v1,one,v10);
            rgpulse(trim,v10,rof1,rof2);
           }
status(C);
}
