// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* wLOGSY_ES - 1D-NOE ePHOGSY experiment for detection
                   of ligands with WaterLOGSY

Literature:  C. Dalvit et al. J. Biomol. NMR, 18, 65-68 (2000).
             C. Dalvit, G. Fogliatto, A. Stewart, M. Veronesi & B. Stockman:
			      J. Biomol. NMR 21, 349-359 (2001).

Parameters:
        sspul           - flag for optional GRD-90-GRD steady-state sequence
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
        esshape     - shape file of the 180 deg. selective refocussing pulse
                        on the solvent (may be convoluted for multiple solvents)
        essoftpw    - pulse width for esshape (as given by Pbox)
        essoftpwr   - power level for esshape (as given by Pbox)        
        essoftpwrf  - fine power for esshape by default it is 2048
                        needs optimization for multiple solvent suppression only
                                with fixed essoftpw
        esgzlvl     - gradient power for the solvent suppression echo
        hsgt        - gradient duration for sspul
        hsglvl      - gradient power for sspul
        mixN        - mixing time
        gzlvlhs     - gradient power for the HS pulse during mix (< 1G/cm)
        wselshape   - shape file of the 180 deg. H2O selection pulse       
        wselpw      - pulse width for wselshape (as given by Pbox)
        wselpwr     - power level for wselshape (as given by Pbox)
        gt1         - gradient duration for the solvent selection echo
        gzlvl1      - gradient power for the solvent selection echo
        alt_grd     - alternate gradient sign(s) on even transients
        gstab       - gradient stabilization delay
        prg_flg     - flag for an optional purge pulse at the end of the sequence
                                set prg_flg='y' tu suppress protein background
        prgtime     - duration of the purge pulse for prg_flg='y' (in seconds!!)     
        prgpwr      - power level for the purge pulse for prg_flg='y'
        lkgate_flg  - lock gating option (on during d1 off during the seq. and at)
        tof         - transmitter offset (set on resonance for H2O)

The water refocusing, selection and water flipback shape can be created/updated
using the "make_es_shape", "make_wselshape" and "make_es_flipshape" macros,
respectively. For multiple frequency solvent suppression the esshape file needs
to be created manually.

p. sandor, darmstadt june 2003
b. heise, oxford may 2012 (Chempack/VJ3 version)

*/	  

#include <standard.h>
#include <chempack.h>
/*
#include <ExcitationSculpting.h>
#include <FlipBack.h>
*/

static int phi1[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
           phi3[16] = {2,2,2,2,3,3,3,3,2,2,2,2,3,3,3,3},
           phi5[16] = {2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3},
           phi9[16] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3},
           rec[16]  = {0,2,0,2,2,0,2,0,2,0,2,0,0,2,0,2};

pulsesequence ()
{

double  gstab = getval("gstab"),
	gt1 = getval("gt1"),
        gzlvl1 = getval("gzlvl1"),
        gzlvlhs = getval("gzlvlhs"),
        wselpw = getval("wselpw"),
        wselpwr = getval("wselpwr"),
        mixN = getval("mixN"),
        prgtime = getval("prgtime"),
        prgpwr = getval("prgpwr"),
        phincr1 = getval("phincr1");

char    wselshape[MAXSTR], sspul[MAXSTR],flipshape[MAXSTR], ESmode[MAXSTR],
	flipback[MAXSTR],prg_flg[MAXSTR],alt_grd[MAXSTR]; 

rof1 = getval("rof1"); if(rof1 > 2.0e-6) rof1 = 2.0e-6;
  getstr("wselshape", wselshape);
  getstr("flipshape", flipshape);
  getstr("prg_flg", prg_flg);
  getstr("alt_grd",alt_grd);
  if (phincr1 < 0.0) phincr1=360+phincr1;
  initval(phincr1,v8);

   settable(t1,16,phi1);
   settable(t3,16,phi3);
   settable(t5,16,phi5);
   settable(t6,16,rec);
   settable(t9,16,phi9);

   sub(ct,ssctr,v12);

   getelem(t1,v12,v1);
   getelem(t3,v12,v3);
   getelem(t5,v12,v5);
   getelem(t9,v12,v9);
   getelem(t6,v12,oph);
   add(v1,one,v10);
   if (alt_grd[0] == 'y') mod2(ct,v6);
                     /* alternate gradient sign on every 2nd transient */

status(A);
   if (getflag("lkgate_flg"))  lk_sample(); /* turn lock sampling on */

   decpower(dpwr); obspower(tpwr);
   if (getflag("sspul"))
        steadystate();
   delay(d1);

   if (getflag("lkgate_flg"))  lk_hold(); /* turn lock sampling off */

status(B);
 if (getflag("ESmode")) 
 {
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
      ifzero(v6); rgradient('z',gzlvlhs); /* make it compatible with cold probes */
        elsenz(v6); rgradient('z',-1.0*gzlvlhs); endif(v6);
     delay(mixN);
   rgradient('z',0.0);
   delay(gstab);
   if (getflag("flipback"))
     FlipBack(v1,v8);
   rgpulse(pw,v1,rof1,rof1);
   ExcitationSculpting(v5,v3,v6);
       if (prg_flg[A] == 'y')    /* optional purge pulse */
           { obspower(prgpwr);
             add(v1,one,v1);
            rgpulse(prgtime,v1,rof1,rof2);
           }
       else delay(rof2);
 }
 else
	rgpulse(pw,v1,rof1,rof2);
status(C);
}
