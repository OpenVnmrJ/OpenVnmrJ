// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* zTOCSY_ES - ztocsy with Excitation Sculpting solvent suppression

Literature reference:
        T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995) Excitation Sculpting
        C. Dalvit; J. Biol. NMR, 11, 437-444 (1998) Excitation Sculpting
        M.J. Trippleton and J. Keeler;
             Angew. Chem. Int. Ed. 2003, 42 3938-3941. ZQ suppression & zTOCSY
Parameters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
        mixT        - TOCSY mixing time
        slpatT      - TOCSY pattern [mlev17,mlev17c,dipsi2,dipsi3]
        slpwrT      - spin-lock power level
        slpwT       - 90 deg pulse width for spinlock
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

HaitaoH		-	ZQ suppression Sept 2004
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

static int ph1[4] = {0,0,1,1},   		/* presat */
           ph2[4] = {1,3,2,0},                  /* excite */
           ph3[8] = {1,1,0,0,3,3,2,2},          /* receiver */
           ph5[4] = {0,0,1,1},                  /* spin lock */
	   ph7[8] = {1,1,2,2,3,3,0,0},		/* flipback */
	   ph8[4] = {3,3,0,0},			/* observe */
           ph4[16] = {0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3}, /* 1st echo */
           ph9[16] = {0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3}; /* 2nd echo */
	   
void pulsesequence()
{
   double          slpwrT = getval("slpwrT"),
                   slpwT = getval("slpwT"),
                   mixT = getval("mixT"),
                   gzlvl1 = getval("gzlvl1"),
                   gt1 = getval("gt1"),
                   gzlvl2 = getval("gzlvl2"),
                   gt2 = getval("gt2"),
                   gstab = getval("gstab"),
                   zqfpw1 = getval("zqfpw1"),
                   zqfpwr1 = getval("zqfpwr1"),
                   zqfpw2 = getval("zqfpw2"),
                   zqfpwr2 = getval("zqfpwr2"),
                   gzlvlzq1 = getval("gzlvlzq1"),
                   gzlvlzq2 = getval("gzlvlzq2"),
                   phincr1 = getval("phincr1"),
                   d2corr;
   char            slpatT[MAXSTR],alt_grd[MAXSTR],
                   zqfpat1[MAXSTR],zqfpat2[MAXSTR],
                   flipback[MAXSTR];
   int             phase1 = (int)(getval("phase")+0.5);

/* LOAD AND INITIALIZE VARIABLES */

//           (void) set_RS(0);   /* set up random sampling */ 
 
   getstr("slpatT",slpatT);
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

  assign(ct,v17);
  if (phincr1 < 0.0) phincr1=360+phincr1;
  initval(phincr1,v5);

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
 
   settable(t1,4,ph1); 	getelem(t1,v17,v6);
   settable(t2,4,ph2); 	getelem(t2,v17,v1);
   settable(t3,8,ph3); 	getelem(t3,v17,oph);
   settable(t5,4,ph5); 	getelem(t5,v17,v21);
   settable(t7,8,ph7); 	getelem(t7,v17,v7);
   settable(t8,4,ph8); 	getelem(t8,v17,v8);   
   settable(t4,16,ph4);  getelem(t4,v17,v4);
   settable(t6,16,ph9);  getelem(t6,v17,v9);
    
   if (phase1 == 2)
      {incr(v1); }

   add(v1, v14, v1);
   add(oph,v14,oph);

   if (alt_grd[0] == 'y') mod2(ct,v10);

/* BEGIN ACTUAL PULSE SEQUENCE CODE */
   status(A);

   if (getflag("lkgate_flg"))  lk_sample(); /* turn lock sampling on */

   obspower(tpwr);
   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   delay(d1);

   d2corr = 4.0e-6 + (4*pw/PI);

   if (getflag("lkgate_flg"))  lk_hold(); /* turn lock sampling off */

   status(B);
      if (getflag("cpmgflg"))
      {
        rgpulse(pw, v1, rof1, 0.0);
        cpmg(v1, v15);
      }
      else
        rgpulse(pw, v1, rof1, 2.0e-6);
      if (d2>d2corr)
        delay(d2 - d2corr); /*corrected evolution time */
      else
        delay(d2);

      rgpulse(pw,v7,2.0e-6,rof1);

      if (mixT > 0.0)
      {
        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr1);
         ifzero(v10); rgradient('z',gzlvlzq1);
         elsenz(v10); rgradient('z',-gzlvlzq1); endif(v10);
         delay(100.0e-6);
         shaped_pulse(zqfpat1,zqfpw1,zero,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(100e-6);
        }
        obspower(slpwrT);
        ifzero(v10); zgradpulse(gzlvl1,gt1);
        elsenz(v10); zgradpulse(-gzlvl1,gt1); endif(v10);
        delay(gstab);
        
        if (dps_flag)
          rgpulse(mixT,v21,0.0,0.0);
        else
          SpinLock(slpatT,mixT,slpwT,v21);

        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr2);
         ifzero(v10); rgradient('z',gzlvlzq2);
         elsenz(v10); rgradient('z',-gzlvlzq2); endif(v10);
         delay(100.0e-6);
         shaped_pulse(zqfpat2,zqfpw2,zero,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(100e-6);
        }
        obspower(tpwr);
        ifzero(v10); zgradpulse(gzlvl2,gt2);
        elsenz(v10); zgradpulse(-gzlvl2,gt2); endif(v10);
        delay(gstab);
      }

      if (flipback[A] == 'y')
         FlipBack(v8,v5);

     rgpulse(pw,v8,rof1,2.0e-6);
     ExcitationSculpting(v4,v9,v10);
     delay(rof2);
           
   status(C);
}
