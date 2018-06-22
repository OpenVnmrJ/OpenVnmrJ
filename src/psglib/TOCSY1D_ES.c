// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*  TOCSY1D_ES - DPFGSE-TOCSY experiment 
                     with Excitation Sculpting solvent suppression

Literature reference:
        T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995) Excitation Sculpting
        C. Dalvit; J. Biol. NMR, 11, 437-444 (1998) Excitation Sculpting

Parameters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
        mixT        - TOCSY mixing time
        slpatT      - TOCSY pattern [mlev17, mlev17c, dipsi2, dipsi3]
        slpwrT      - spin-lock power level
        slpwT       - 90 deg pulse width for spinlock
        selshapeA, selpwrA, selpwA, gzlvlA, gtA -
                         shape, power, pulse, level and time for first PFG echo
        selshapeB, selpwrB, selpwB, gzlvlB, gtB -
                         shape, power, pulse, level and time for 2nd PFG echo
        selfrq      - selective frequency (for selective 180)
        gstab       - gradient stalilization delay
        trim        - trim pulse preceeding spinlock
        zqfilt      - optional z-filter after the spin-lock
        tauz1, tauz2, tauz3, tauz4, - delays for the zqfilter
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

The water refocusing shape can be created/updated using the "make_es_shape"
macro. For multiple frequency solvent suppression the esshape file needs to be
created manually.

************************************************************************
****NOTE:  v20,v21,v22,v23 and v24 are used by Hardware Loop and reserved ***
   v21 and v23 are spinlock phase
************************************************************************

KrishK  Aug. 2006
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

static int	ph1[4] = {0,2,3,1},
		ph2[4] = {2,2,1,1},
		ph3[8] = {1,1,0,0,3,3,2,2},
		ph4[8] = {0,2,3,1,2,0,1,3},
		ph7[8] = {2,2,1,1,0,0,3,3},
                ph5[4] = {0,2,2,0},         /* oph correction for the Excitation Sculpting */
              phs8[16] = {0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3}, /* 1st ecdo in ES */
              phs9[16] = {0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3}, /* 2nd ecdo in ES */
		ph8[4] = {0,0,3,3};

pulsesequence()
{
   double	   slpwrT = getval("slpwrT"),
		   slpwT = getval("slpwT"),
		   mixT = getval("mixT"),
		   trim = getval("trim"),
		   tauz1 = getval("tauz1"), 
		   tauz2 = getval("tauz2"), 
		   tauz3 = getval("tauz3"), 
		   tauz4 = getval("tauz4"),
                   selpwrA = getval("selpwrA"),
                   selpwA = getval("selpwA"),
                   gzlvlA = getval("gzlvlA"),
                   gtA = getval("gtA"),
                   selpwrB = getval("selpwrB"),
                   selpwB = getval("selpwB"),
                   gzlvlB = getval("gzlvlB"),
                   gtB = getval("gtB"),
                   gstab = getval("gstab"),
		   selfrq = getval("selfrq");
   char            selshapeA[MAXSTR], selshapeB[MAXSTR], slpatT[MAXSTR],
                   alt_grd[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("slpatT",slpatT);
   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);
   getstr("alt_grd",alt_grd);
                     /* alternate gradient sign on every 2nd transient */

   if (strcmp(slpatT,"mlev17c") &&
        strcmp(slpatT,"dipsi2") &&
        strcmp(slpatT,"dipsi3") &&
        strcmp(slpatT,"mlev17") &&
        strcmp(slpatT,"mlev16"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatT);

  assign(ct,v17);

   assign(v17,v6);
   if (getflag("zqfilt")) 
     {  hlv(v6,v6); hlv(v6,v6); }

   settable(t1,4,ph1);   getelem(t1,v6,v1);
   settable(t3,8,ph3);   getelem(t3,v6,v11);
   settable(t4,8,ph4);  
   settable(t5,4,ph5);   getelem(t5,v6,v5); 
   settable(t2,4,ph2);   getelem(t2,v6,v2);
   settable(t7,8,ph7);   getelem(t7,v6,v7);
   settable(t8,4,ph8);   getelem(t8,v6,v8);
   settable(t11,16,phs8); getelem(t11,v6,v3);   /* 1st echo in ES */
   settable(t12,16,phs9); getelem(t12,v6,v4);   /* 2nd exho in ES */
   
   if (getflag("zqfilt"))
     getelem(t4,v6,oph);
   else
     assign(v1,oph);

   add(oph,v5,oph); mod4(oph,oph);

   sub(v2,one,v21);
   add(v21,two,v23);

   mod4(ct,v10);
   if (alt_grd[0] == 'y') mod2(ct,v12); /* alternate gradient sign on every 2nd transient */

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

        ifzero(v12); zgradpulse(gzlvlA,gtA);
        elsenz(v12); zgradpulse(-gzlvlA,gtA); endif(v12);
        delay(gstab);
        obspower(selpwrA);
        shaped_pulse(selshapeA,selpwA,v1,rof1,rof1);
        obspower(tpwr);
        ifzero(v12); zgradpulse(gzlvlA,gtA);
        elsenz(v12); zgradpulse(-gzlvlA,gtA); endif(v12);
        delay(gstab);

      if (selfrq != tof)
        delay(2*OFFSET_DELAY);

        ifzero(v12); zgradpulse(gzlvlB,gtB);
        elsenz(v12); zgradpulse(-gzlvlB,gtB); endif(v12);
        delay(gstab);
        obspower(selpwrB);
        shaped_pulse(selshapeB,selpwB,v2,rof1,rof1);
        obspower(slpwrT);
        ifzero(v12); zgradpulse(gzlvlB,gtB);
        elsenz(v12); zgradpulse(-gzlvlB,gtB); endif(v12);
        delay(gstab);

      if (selfrq != tof)
	obsoffset(tof);

     if (mixT > 0.0)
      { 
        rgpulse(trim,v11,0.0,0.0);
        if (dps_flag)
          rgpulse(mixT,v21,0.0,0.0);
        else
          SpinLock(slpatT,mixT,slpwT,v21);
       }

      if (getflag("zqfilt"))
      {
	obspower(tpwr);
	rgpulse(pw,v7,1.0e-6,rof1);
	ifzero(v10); delay(tauz1); endif(v10);
	decr(v10);
	ifzero(v10); delay(tauz2); endif(v10);
	decr(v10);
	ifzero(v10); delay(tauz3); endif(v10);
	decr(v10);
	ifzero(v10); delay(tauz4); endif(v10);
	rgpulse(pw,v8,rof1,2.0e-6);
      }
      ExcitationSculpting(v3,v4,v12);
      delay(rof2);

   status(C);
}
