// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* zTOCSY1D_ES.c - 1D DPFGSE-TOCSY experiment w/ Keeler's zero-quantum filter (ZQF)
                      and Excitation Sculpting solvent suppression

Literature reference:
        T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995) Excitation Sculpting
        C. Dalvit; J. Biol. NMR, 11, 437-444 (1998) Excitation Sculpting
        M.J. Trippleton and J. Keeler;
             Angew. Chem. Int. Ed. 2003, 42 3938-3941. ZQ suppression & zTOCSY
Parameters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
        mixT        - TOCSY mixing time
        slpatT      - TOCSY pattern [dipsi2,dipsi3]
        slpwrT      - spin-lock power level
        slpwT       - 90 deg pulse width for spinlock
        selshapeA, selpwrA, selpwA, gzlvlA, gtA -
                         shape, power, pulse, level and time for first PFG echo
        selshapeB, selpwrB, selpwB, gzlvlB, gtB -
                         shape, power, pulse, level and time for 2nd PFG echo
        selfrq      - selective frequency (for selective 180)
        Gzqfilt     - flag for optional ZQ artifact suppression
        zqfpat1,zqfpat2 - adiabatic sweep 180 shape files
        zqfpw1,zqfpw2   - adiabatic sweep 180 pulse widths
        zqfpwr1,zqfpwr2 - adiabatic sweep 180 pulse power
        gzlvlzq1,gzlvlzq2 - gradient levels for ZQFs
        gzlvl1,gzlvl2   - gradient levels for crusher gradients
        gt1,gt2     - gradient durations for the crusher gradients
        gstab       - gradient stalilization delay
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
                                with fixed esoftpw 
        esgzlvl     - gradient power for the solvent suppression echo
        esgt        - gradient duration for the solvent suppression echo
        alt_grd     - alternate gradient sign(s) in dpfgse on even transients
        lkgate_flg  - lock gating option (on during d1 off during the seq. and at)

The water refocusing shape and the water flipback shape can be created/updated
using the "make_es_shape" and "make_es_flipshape" macros, respectively. For
multiple frequency solvent suppression the esshape file needs to be created
manually.

 Warning:
   For probes with very short RF coils, the calculated gradient levels for the ZQFs may cause
   the gradient amplifier to exceed its duty cycle!

************************************************************************
****NOTE:  v20,v21,v22,v23 and v24 are used by Hardware Loop and reserved ***
************************************************************************

KrishK : Aug. 2006
PeterS - Excitation Sculpting added 2012
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***
*/

#include <standard.h>
#include <chempack.h>
/*
#include <ExcitationSculpting.h>
#include <FlipBack.h>
*/

extern int dps_flag;

pulsesequence()
{
   double	   slpwrT = getval("slpwrT"),
		   slpwT = getval("slpwT"),
		   mixT = getval("mixT"),
		   gzlvl1 = getval("gzlvl1"),
		   gt1 = getval("gt1"),
		   gzlvl2 = getval("gzlvl2"),
		   gt2 = getval("gt2"),
		   gstab = getval("gstab"),
                   selpwrA = getval("selpwrA"),
                   selpwA = getval("selpwA"),
                   gzlvlA = getval("gzlvlA"),
                   gtA = getval("gtA"),
                   selpwrB = getval("selpwrB"),
                   selpwB = getval("selpwB"),
                   gzlvlB = getval("gzlvlB"),
                   gtB = getval("gtB"),
		   selfrq = getval("selfrq"),
                   zqfpw1 = getval("zqfpw1"),
                   zqfpwr1 = getval("zqfpwr1"),
                   zqfpw2 = getval("zqfpw2"),
                   zqfpwr2 = getval("zqfpwr2"),
                   gzlvlzq1 = getval("gzlvlzq1"),
                   gzlvlzq2 = getval("gzlvlzq2"),
                   phincr1 = getval("phincr1");
   char		   slpatT[MAXSTR], selshapeA[MAXSTR], selshapeB[MAXSTR], zqfpat1[MAXSTR],
                   zqfpat2[MAXSTR], flipback[MAXSTR], alt_grd[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("slpatT",slpatT);
   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);
   getstr("zqfpat1",zqfpat1);
   getstr("zqfpat2",zqfpat2);
   getstr("flipback", flipback);
   getstr("alt_grd",alt_grd);
                     /* alternate gradient sign on every 2nd transient */

   if (strcmp(slpatT,"mlev17c") &&
        strcmp(slpatT,"dipsi2") &&
        strcmp(slpatT,"dipsi3") &&
        strcmp(slpatT,"mlev17") &&
        strcmp(slpatT,"mlev16"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatT);

/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR) or off of CT */

   assign(ct,v17);

   ifzero(ssctr);
      assign(v17,v13);
   elsenz(ssctr);
        /* purge option does not adjust v13 during steady state*/
      sub(ssval, ssctr, v13); /* v13 = 0,...,ss-1 */
   endif(ssctr);

   mod4(v13,v1); /* v1 = 0 1 2 3 */
   hlv(v13,v13);
   hlv(v13,v13);
   mod4(v13,v11); /* v11 = 0000 1111 2222 3333 */
   dbl(v1,oph);
   add(v11,oph,oph);
   add(v11,oph,oph); /* oph = 2v1 + 2v11 */

/* CYCLOPS */
   hlv(v13,v13);
   hlv(v13,v14);
   add(v1,v14,v1);
   add(v11,v14,v11);
   add(oph,v14,oph);
   assign(v14,v21);
   add(one,v21,v21);
   add(two,v21,v12);

  if (phincr1 < 0.0) phincr1=360+phincr1;
  initval(phincr1,v5);

   mod2(ct,v2);    /* 01 01 */
   hlv(ct,v4); hlv(v4,v4); mod2(v4,v4); dbl(v4,v4); /* 0000 2222 */
   add(v4,v2,v4); mod4(v4,v4); /* 0101 2323  first echo in Excitation Sculpting */
   hlv(ct,v7); mod2(v7,v7);    /* 0011 */
   hlv(ct,v9); hlv(v9,v9); hlv(v9,v9); dbl(v9,v9); add(v9,v7,v9);
   mod4(v9,v9);   /* 0011 0011 2233 2233 second echo in Excitation Sculpting */

   dbl(v2,v2);    /* 0202 */
   dbl(v7,v7);    /* 0022 */
   add(v2,v7,v7); /* 0220 correct oph for Excitation Sculpting */
   add(oph,v7,oph); mod4(oph,oph);

  if (alt_grd[0] == 'y') mod2(ct,v10); /* alternate gradient sign on every 2nd transient */

/* BEGIN THE ACTUAL PULSE SEQUENCE */
status(A);

   if (getflag("lkgate_flg"))  lk_sample(); /* turn lock sampling on */

   obspower(tpwr);
   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   delay(d1);

   if (getflag("lkgate_flg"))  lk_hold(); /* turn lock sampling off */

   status(B);

      if (getflag("cpmgflg"))
      {
        rgpulse(pw, v14, rof1, 0.0);
        cpmg(v14, v15);
      }
      else
        rgpulse(pw, v14, rof1, rof1);

      if (selfrq != tof)
        obsoffset(selfrq);

        ifzero(v10); zgradpulse(gzlvlA,gtA);
        elsenz(v10); zgradpulse(-gzlvlA,gtA); endif(v10);
        delay(gstab);
        obspower(selpwrA);
        shaped_pulse(selshapeA,selpwA,v1,rof1,rof1);
        obspower(tpwr);
        ifzero(v10); zgradpulse(gzlvlA,gtA);
        elsenz(v10); zgradpulse(-gzlvlA,gtA); endif(v10);
        delay(gstab);

      if (selfrq != tof)
        delay(2*OFFSET_DELAY);

        ifzero(v10); zgradpulse(gzlvlB,gtB);
        elsenz(v10); zgradpulse(-gzlvlB,gtB); endif(v10);
        delay(gstab);
        obspower(selpwrB);
        shaped_pulse(selshapeB,selpwB,v11,rof1,rof1);
        obspower(tpwr);
        ifzero(v10); zgradpulse(gzlvlB,gtB);
        elsenz(v10); zgradpulse(-gzlvlB,gtB); endif(v10);
        delay(gstab);

      if (selfrq != tof)
        obsoffset(tof);

        rgpulse(pw, v14, rof1, rof1);
        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr1);
         ifzero(v10); rgradient('z',gzlvlzq1);
         elsenz(v10); rgradient('z',-gzlvlzq1); endif(v10);
         delay(100.0e-6);
         shaped_pulse(zqfpat1,zqfpw1,v14,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(gstab);
        }
        obspower(slpwrT);
        ifzero(v10); zgradpulse(gzlvl1,gt1);
        elsenz(v10); zgradpulse(-gzlvl1,gt1); endif(v10);
        delay(gstab);

	if (mixT > 0.0)
	{
          if (dps_flag)
          	rgpulse(mixT,v21,0.0,0.0);
          else
          	SpinLock(slpatT,mixT,slpwT,v21);
        }

        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr2);
         ifzero(v10); rgradient('z',gzlvlzq2);
         elsenz(v10); rgradient('z',-gzlvlzq2); endif(v10);
         delay(100.0e-6);
         shaped_pulse(zqfpat2,zqfpw2,v14,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(gstab);
        }
        obspower(tpwr);
        ifzero(v10); zgradpulse(gzlvl2,gt2);
        elsenz(v10); zgradpulse(-gzlvl2,gt2); endif(v10);
        delay(gstab);

      if (flipback[A] == 'y')
         FlipBack(v14,v5);

      rgpulse(pw,v14,rof1,2.0e-6);
      ExcitationSculpting(v4,v9,v10);
      delay(rof2);

   status(C);
}
