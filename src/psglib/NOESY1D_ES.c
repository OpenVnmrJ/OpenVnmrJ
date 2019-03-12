// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*  NOESY1D_ES - DPFGSENOE experiment experiment with water suppression 
           by gradient echo.

Literature reference:
        T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995) Excitation Sculpting
        C. Dalvit; J. Biol. NMR, 11, 437-444 (1998) Excitation Sculpting
        M.J. Trippleton and J. Keeler;
             Angew. Chem. Int. Ed. 2003, 42 3938-3941. ZQ suppression

Parameters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
        mixN        - NOESY mixing time
        Gzqfilt     - flag for optional ZQ artifact suppression
        zqfpw1      - pulse width for the ZQ filter
        zqfpwr1     - power level for the ZQ filter
        gzlvlzq1    - gradient amplitude for the ZQ filter
        zqfpat1     - shape pattern for the ZQ filter
        gzlvl1      - homospoil gradient amplitude
        gt1         - homospoil gradient duration
        gstab       - gradient stabilization delay
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
        selshapeA, selpwrA, selpwA, gzlvlA, gtA -
                    - shape, power, pulse, level and time for first PFG echo
        selshapeB, selpwrB, selpwB, gzlvlB, gtB -
                    - shape, power, pulse, level and time for 2nd PFG echo
        selfrq      - Selective frequency (for selective 180)
        sweeppw     - sech180 pulse width
        sweeppwr    - sech180 pulse power
        sweepshp    - sech180 pulse shape
        gzlvlC      - Gradient level during mixing
        gtC         - Gradient time during mixing

KrishK - includes purge option : Aug. 2006
PeterS - Excitation Sculpting added 2012
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

The water refocusing shape and the water flipback shape can be created/updated
using the "make_es_shape" and "make_es_flipshape" macros, respectively. For
multiple frequency solvent suppression the esshape file needs to be created
manually.

*/

#include <standard.h>
#include <chempack.h>
/*
#include <ExcitationSculpting.h>
#include <FlipBack.h>
*/

void pulsesequence()
{
   double	   mixN = getval("mixN"),
		   gzlvlC = getval("gzlvlC"),
		   gtC = getval("gtC"),
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
		   zqfpw3 = getval("zqfpw3"),
		   zqfpwr3 = getval("zqfpwr3"),
		   gzlvlzq3 = getval("gzlvlzq3"),
		   mixNcorr, fbcorr,
                   phincr1 = getval("phincr1"),
                   flippw = getval("flippw"),
		   sweeppw = getval("sweeppw"),
		   sweeppwr = getval("sweeppwr");
   char		   sweepshp[MAXSTR],alt_grd[MAXSTR],
                   selshapeA[MAXSTR],flipback[MAXSTR],
                   selshapeB[MAXSTR],zqfpat3[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtC = syncGradTime("gtC","gzlvlC",1.0);
        gzlvlC = syncGradLvl("gtC","gzlvlC",1.0);
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("sweepshp",sweepshp);
   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);
   getstr("zqfpat3",zqfpat3);
   getstr("flipback", flipback);
   getstr("alt_grd",alt_grd);
                     /* alternate gradient sign on every 2nd transient */

   if (flipback[A] == 'y') fbcorr = flippw + 2.0*rof1;
   else fbcorr = 0.0;    /* correck for flipback if used */
   if (phincr1 < 0.0) phincr1=360+phincr1;
   initval(phincr1,v12);
   mixNcorr = fbcorr;

   if (getflag("Gzqfilt"))
	mixNcorr += zqfpw3 + 2.0*rof1 + 4.0e-4;
   mixN = mixN-mixNcorr;

  assign(ct,v17);

   hlv(v17,v1); hlv(v1,v1); hlv(v1,v1); hlv(v1,v1); mod4(v1,v1);
   mod4(v17,v2); add(v1,v2,v2);
   hlv(v17,v3); hlv(v3,v3); mod4(v3,v3); add(v1,v3,v3); dbl(v3,v4);
   dbl(v2,oph); add(oph,v4,oph); add(oph,v1,oph);

   mod2(ct,v5);    /* 01 01 */
   hlv(ct,v7); hlv(v7,v7); mod2(v7,v7); dbl(v7,v7); /* 0000 2222 */
   add(v7,v5,v7); mod4(v7,v7); /* 0101 2323  first echo in Excitation Sculpting */
   hlv(ct,v11); mod2(v11,v11);    /* 0011 */
   hlv(ct,v8); hlv(v8,v8); hlv(v8,v8); dbl(v8,v8); add(v8,v11,v8);
   mod4(v8,v8);   /* 0011 0011 2233 2233 second echo in Excitation Sculpting */

   dbl(v5,v5);     /* 0202 */
   dbl(v11,v11);   /* 0022 */
   add(v5,v11,v11); /* 0220 correct oph for Excitation Sculpting */
   add(oph,v11,oph);   
   mod4(oph,oph);

   if (alt_grd[0] == 'y') mod2(ct,v10);

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
        rgpulse(pw, v1, rof1, 0.0);
        cpmg(v1, v15);
      }
      else
        rgpulse(pw, v1, rof1, rof1);

      if (selfrq != tof)
        obsoffset(selfrq);

        ifzero(v10); zgradpulse(gzlvlA,gtA);
        elsenz(v10); zgradpulse(-gzlvlA,gtA); endif(v10);
        delay(gstab);
        obspower(selpwrA);
        shaped_pulse(selshapeA,selpwA,v2,rof1,rof1);
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
        shaped_pulse(selshapeB,selpwB,v3,rof1,rof1);
        obspower(tpwr);
        ifzero(v10); zgradpulse(gzlvlB,gtB);
        elsenz(v10); zgradpulse(-gzlvlB,gtB); endif(v10);
        delay(gstab);

      if (selfrq != tof)
        obsoffset(tof);

      rgpulse(pw,v1,rof1,rof1);

        obspower(sweeppwr);
	delay(0.31*mixN);
        ifzero(v10); zgradpulse(gzlvlC,gtC);
	elsenz(v10); zgradpulse(-gzlvlC,gtC); endif(v10);
	delay(gstab);
	shaped_pulse(sweepshp,sweeppw,zero,rof1,rof1);
	delay(gstab);
        ifzero(v10); zgradpulse(-gzlvlC,gtC);
        elsenz(v10); zgradpulse(gzlvlC,gtC); endif(v10);
	delay(0.49*mixN);
        ifzero(v10); zgradpulse(-gzlvlC,2.0*gtC);
        elsenz(v10); zgradpulse(gzlvlC,2.0*gtC); endif(v10);
	delay(gstab);
	if (getflag("Gzqfilt"))
	   {
                obspower(zqfpwr3);
                ifzero(v10); rgradient('z',gzlvlzq3);
                elsenz(v10); rgradient('z',-gzlvlzq3); endif(v10);
                delay(100.0e-6);
                shaped_pulse(zqfpat3,zqfpw3,zero,rof1,rof1);
                delay(100.0e-6);
                rgradient('z',0.0);
	   }
	else
		shaped_pulse(sweepshp,sweeppw,zero,rof1,rof1);
	delay(gstab);
        ifzero(v10); zgradpulse(gzlvlC,2.0*gtC);
        elsenz(v10); zgradpulse(-gzlvlC,2.0*gtC); endif(v10);
	delay(0.2*mixN);
	obspower(tpwr);

        if (flipback[A] == 'y')
         FlipBack(v1,v12);

      rgpulse(pw,v1,rof1,2.0e-6);
      ExcitationSculpting(v7,v8,v10);
      delay(rof2);

   status(C);
}
