// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* ROESY_ES - cross-relaxation experiment in rotating frame
                     with dpfgse solvent suppression

Literature reference:
        T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995) Excitation Sculpting
        C. Dalvit; J. Biol. NMR, 11, 437-444 (1998) Excitation Sculpting
        M.J. Trippleton and J. Keeler;
             Angew. Chem. Int. Ed. 2003, 42 3938-3941. ZQ suppression & zTOCSY
Parameters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
        mixR        - ROESY spinlock mixRing time
        slpwrR      - spin-lock power level
        slpwR       - 90 deg pulse width for spinlock
        slpatR      - Spinlock pattern [cw and troesy]
        zfilt       - selects z-filter with flipback option
                                [requires gradient and PFGflg=y]
        gzlvlz      - homospoil gradient amplitude
        gtz         - homospoil gradient duration
        gstab       - gradient stalilization delay
        flipback    - flag for an optional selective 90 flipback pulse
                                on the solvent to keep it along z all time
                         onlz active if zfilter='y'
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

static int ph1[4] = {1,3,2,0},
	   ph2[8] = {3,3,0,0,1,1,2,2},  /*troesy spinlock */
           ph3[8] = {1,3,2,0,3,1,0,2},
           ph8[4] = {3,3,0,0},
           ph6[8] = {1,1,2,2,3,3,0,0},
           ph4[4] = {0,2,2,0}, /* oph correction for Excitation Sculpting */
         ph11[16] = {0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3}, /* 1st echo in ES */
         ph12[16] = {0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3}; /* 2nd echo in ES */

void pulsesequence()
{
   double          slpwrR = getval("slpwrR"),
                   slpwR = getval("slpwR"),
                   mixR = getval("mixR"),
                   gzlvlz = getval("gzlvlz"),
                   gtz = getval("gtz"),
                   gstab = getval("gstab"),
                   phincr1 = getval("phincr1");
   char		   slpatR[MAXSTR], alt_grd[MAXSTR];
   int             phase1 = (int)(getval("phase")+0.5);

/* LOAD AND INITIALIZE PARAMETERS */

//           (void) set_RS(0);   /* set up random sampling */ 
 
   getstr("slpatR",slpatR);
   getstr("alt_grd",alt_grd);
                     /* alternate gradient sign on every 2nd transient */

   if (strcmp(slpatR,"cw") &&
        strcmp(slpatR,"troesy") &&
        strcmp(slpatR,"dante"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatR);

  assign(ct,v17);

   settable(t1,4,ph1);	getelem(t1,v17,v1);
   settable(t2,8,ph2);	getelem(t2,v17,v20);
   settable(t3,8,ph3);	
   settable(t8,4,ph8);	getelem(t8,v17,v8);
   settable(t6,8,ph6);	getelem(t6,v17,v6);
   settable(t4,4,ph4);  getelem(t4,v17,v4);
   settable(t11,16,ph11); getelem(t11,v17,v11);
   settable(t12,16,ph12); getelem(t12,v17,v12);
  
   assign(v1,oph); 
   if (getflag("zfilt"))
	getelem(t3,v17,oph);

   assign(v20,v9);
   if (!strcmp(slpatR,"troesy"))
	assign(v20,v21);
   else
	add(v20,one,v21);

   if (phase1 == 2)
      {incr(v1); }			/* hypercomplex method */

   if (alt_grd[0] == 'y') mod2(ct,v10);

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v13);

       add(v1,v13,v1);
       add(oph,v13,oph);
       add(oph,v4,oph);    /* correct oph for ES */
 
/* The following is for flipback pulse */
   if (phincr1 < 0.0) phincr1=360+phincr1;
   initval(phincr1,v5);

/* BEGIN ACTUAL PULSE SEQUENCE */
   status(A);

   if (getflag("lkgate_flg"))  lk_sample(); /* turn lock sampling on */

   obspower(tpwr); decpower(dpwr);
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
      if (d2 > 0.0)
       delay(d2 - POWER_DELAY - (2*pw/PI) - rof1);
      else
       delay(d2);
      
      obspower(slpwrR);

      if (mixR > 0.0)
      {
        if (dps_flag)
          	rgpulse(mixR,v21,0.0,0.0);
        else
          SpinLock(slpatR,mixR,slpwR,v21);
      }

       if ((getflag("zfilt")) && (getflag("PFGflg")))
        {
           obspower(tpwr);
           rgpulse(pw,v9,1.0e-6,rof1);
           ifzero(v10); zgradpulse(gzlvlz,gtz);
           elsenz(v10); zgradpulse(-gzlvlz,gtz); endif(v10);
           delay(gstab);
           if (getflag("flipback"))
                FlipBack(v8,v5);
           rgpulse(pw,v8,rof1,2.0e-6);
        }
     ExcitationSculpting(v11,v12,v10);
     delay(rof2);

   status(C);
}
