// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*  ROESY1D - DPFGSE-ROESY experiment w/ optional ZQ-suppression
                   and dpfgse solvent suppression

Literature reference:
        T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995) Excitation Sculpting
        C. Dalvit; J. Biol. NMR, 11, 437-444 (1998) Excitation Sculpting
        M.J. Trippleton and J. Keeler;
             Angew. Chem. Int. Ed. 2003, 42 3938-3941. ZQ suppression 
Parameters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
        mixR        - ROESY spinlock mixRing time
        slpwrR      - spin-lock power level
        slpwR       - 90 deg pulse width for spinlock
        slpatR      - Spinlock pattern [cw and troesy]
        selshapeA, selpwrA, selpwA, gzlvlA, gtA -
                        shape, power, pulse, level and time for first PFG echo
        selshapeB, selpwrB, selpwB, gzlvlB, gtB -
                        shape, power, pulse, level and time for 2nd PFG echo
        selfrq      - selective frequency (for selective 180)
        Gzqfilt     - selects ZQ-filter with flipback option
                                [requires gradient and PFGflg=y]
        zqfpat1     - adiabatic sweep 180 shape files
        zqfpw1      - adiabatic sweep 180 pulse width
        zqfpwr1     - adiabatic sweep 180 pulse power
        gzlvlzq1    - gradient levels for ZQFs
        gzlvl1,gzlvl2 - gradient levels for crusher gradients
        gt1,gt2     - gradient durations for the crusher gradients
        gzlvlz      - homospoil gradient amplitude
        gtz         - homospoil gradient duration
        gstab       - gradient stalilization delay
        flipback    - flag for an optional selective 90 flipback pulse
                                on the solvent to keep it along z all time
                         onlz active if Gzqfilt='y'
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

************************************************************************
****NOTE:  v20,v21,v22,v23 and v24 are used by Hardware Loop and reserved ***
************************************************************************

KrishK - Aug. 2006
PeterS - Excitation Sculpting added 2012

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
   double	   slpwrR = getval("slpwrR"),
		   slpwR = getval("slpwR"),
		   mixR = getval("mixR"),
                   selpwrA = getval("selpwrA"),
                   selpwA = getval("selpwA"),
                   gzlvlA = getval("gzlvlA"),
                   gtA = getval("gtA"),
                   selpwrB = getval("selpwrB"),
                   selpwB = getval("selpwB"),
                   gzlvlB = getval("gzlvlB"),
                   gtB = getval("gtB"),
                   gstab = getval("gstab"),
		   selfrq = getval("selfrq"),
                   gzlvl1 = getval("gzlvl1"),
                   gt1 = getval("gt1"),
                   gzlvl2 = getval("gzlvl2"),
                   gt2 = getval("gt2"),
                   zqfpw1 = getval("zqfpw1"),
                   zqfpwr1 = getval("zqfpwr1"),
                   gzlvlzq1 = getval("gzlvlzq1"),
                   phincr1 = getval("phincr1");
   char            selshapeA[MAXSTR],selshapeB[MAXSTR], slpatR[MAXSTR],
                   zqfpat1[MAXSTR], alt_grd[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("slpatR",slpatR);
   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);
   getstr("zqfpat1",zqfpat1);
   getstr("alt_grd",alt_grd);

   if (strcmp(slpatR,"cw") &&
        strcmp(slpatR,"troesy") &&
        strcmp(slpatR,"dante"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatR);

/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR) or off of CT */

  assign(ct,v17);
   ifzero(ssctr);
      assign(v17,v13);
   elsenz(ssctr);
                /* purge option does not adjust v13 during steady state */
      sub(ssval, ssctr, v13);
   endif(ssctr);

/* Beginning phase cycling */

   dbl(v13,v1);		/* v1 = 0 2 */
   hlv(v13,v13);
   dbl(v13,v20);		/* v20 = 00 22 */
   hlv(v13,v13);
   dbl(v13,v6);		/* v6 = 0000 2222 */
   hlv(v13,v13);
   dbl(v13,v7);		/* v7 = 00000000 22222222 */

   assign(v1,oph);

   if (getflag("Gzqfilt"))
      add(v7,oph,oph);

/* CYCLOPS */

   assign(v13,v14);	/* v14 = 8x0 8x1 8x2 8x3 */
   
   if (getflag("Gzqfilt"))
      hlv(v13,v14);	/* v14 = 16x0 16x1 16x2 16x3 */

   add(v1,v14,v1);      
   add(v20,v14,v20);      
   add(v6,v14,v6);      
   add(v7,v14,v7);      
   add(oph,v14,oph);

/*  add(oph,v18,oph);
  add(oph,v19,oph); */
  assign(zero,v9);

   mod2(ct,v2);    /* 01 01 */
   hlv(ct,v11); hlv(v11,v11); mod2(v11,v11); dbl(v11,v11); /* 0000 2222 */
   add(v11,v2,v11); mod4(v11,v11); /* 0101 2323  first echo in Excitation Sculpting */
   hlv(ct,v4); mod2(v4,v4);    /* 0011 */
   hlv(ct,v12); hlv(v12,v12); hlv(v12,v12); dbl(v12,v12); add(v12,v4,v12);
   mod4(v12,v12);   /* 0011 0011 2233 2233 second echo in Excitation Sculpting */

   dbl(v2,v2);    /* 0202 */
   dbl(v4,v4);    /* 0022 */
   add(v2,v4,v4); /* 0220 correct oph for Excitation Sculpting */
   add(oph,v4,oph); mod4(oph,oph);

   if (!strcmp(slpatR,"troesy")) 
	assign(v20,v21);
   else
	add(v20,one,v21);

   if (alt_grd[0] == 'y') mod2(ct,v8); /* alternate gradient sign on even scans */

/* The following is for flipback pulse */
   if (phincr1 < 0.0) phincr1=360+phincr1;
   initval(phincr1,v5);

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
      rgpulse(pw, v1, rof1, rof1);
      if (selfrq != tof)
	obsoffset(selfrq);

        ifzero(v8); zgradpulse(gzlvlA,gtA);
        elsenz(v8); zgradpulse(-gzlvlA,gtA); endif(v8);
        delay(gstab);
        obspower(selpwrA);
        shaped_pulse(selshapeA,selpwA,v14,rof1,rof1);
        obspower(tpwr);
        ifzero(v8); zgradpulse(gzlvlA,gtA);
        elsenz(v8); zgradpulse(-gzlvlA,gtA); endif(v8);
        delay(gstab);

      if (selfrq != tof)
        delay(2*OFFSET_DELAY);

        ifzero(v8); zgradpulse(gzlvlB,gtB);
        elsenz(v8); zgradpulse(-gzlvlB,gtB); endif(v8);
        delay(gstab);
        obspower(selpwrB);
        shaped_pulse(selshapeB,selpwB,v6,rof1,rof1);
        obspower(slpwrR);
        ifzero(v8); zgradpulse(gzlvlB,gtB);
        elsenz(v8); zgradpulse(-gzlvlB,gtB); endif(v8);
        delay(gstab);

      if (selfrq != tof)
        obsoffset(tof);

     if (mixR > 0.0)
      { 
	  if (dps_flag)
		rgpulse(mixR,v21,0.0,0.0);
	  else
		SpinLock(slpatR,mixR,slpwR,v21);
      }

    if (getflag("Gzqfilt"))
    {
     obspower(tpwr);
     rgpulse(pw,v7,rof1,rof1);

     ifzero(v8); zgradpulse(gzlvl1,gt1);
     elsenz(v8); zgradpulse(-gzlvl1,gt1); endif(v8);
     delay(gstab);

     obspower(zqfpwr1);
     ifzero(v8); rgradient('z',gzlvlzq1);
     elsenz(v8); rgradient('z',-gzlvlzq1); endif(v8);
     delay(100.0e-6);
     shaped_pulse(zqfpat1,zqfpw1,zero,rof1,rof1);
     delay(100.0e-6);
     rgradient('z',0.0);
     delay(gstab);
    
     ifzero(v8); zgradpulse(-gzlvl2,gt2);
     elsenz(v8); zgradpulse(gzlvl2,gt2); endif(v8);
     obspower(tpwr);
     delay(gstab);

     if (getflag("flipback"))
           FlipBack(v14,v5);
     rgpulse(pw,v14,rof1,2.0e-6);
    }

    ExcitationSculpting(v11,v12,v8);
    delay(rof2);
   
   status(C);
}

