// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* TOCSY_ES - TOCSY 2D with Excitation Sculpting solvent suppression
 
Literature reference:
        T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995) Excitation Sculpting
        C. Dalvit; J. Biol. NMR, 11, 437-444 (1998) Excitation Sculpting

	Features included:
		States-TPPI in F1
		z-filter pulse with flipback option
		
Parameters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
        mixT        - TOCSY mixing time
        slpatT      - TOCSY pattern [mlev17, mlev17c, dipsi2, dipsi3]
        slpwrT      - spin-lock power level
        slpwT       - 90 deg pulse width for spinlock
        trim        - trim pulse preceeding spinlock
        zfilt       - selects z-filter with flipback option
                           [Requires gradient and PFGflg=y]
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

static int ph1[4] = {0,0,1,1},   		/* presat */
           ph2[4] = {1,3,2,0},                  /* 90 */
           ph3[8] = {1,3,2,0,3,1,0,2},          /* receiver */
           ph4[4] = {0,0,1,1},                  /* trim */
           ph5[4] = {0,0,1,1},                  /* spin lock */
	   ph7[8] = {1,1,2,2,3,3,0,0},
	   ph8[4] = {3,3,0,0},
         phs8[16] = {0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3}, /* 1st ecdo in ES */
         phs9[16] = {0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3}, /* 2nd ecdo in ES */
       rec_cor[4] = {0,2,2,0};
	   
void pulsesequence()
{
   double          slpwrT = getval("slpwrT"),
                   slpwT = getval("slpwT"),
                   trim = getval("trim"),
                   mixT = getval("mixT"),
		   gzlvlz = getval("gzlvlz"),
		   gtz = getval("gtz"),
                   flippw = getval("flippw"),
		   phincr1 = getval("phincr1");
   char		   slpatT[MAXSTR], alt_grd[MAXSTR], flipback[MAXSTR];
   int		   phase1 = (int)(getval("phase")+0.5);

/* LOAD AND INITIALIZE VARIABLES */

//           (void) set_RS(0);   /* set up random sampling */

   getstr("slpatT",slpatT);
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

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
 
   settable(t1,4,ph1); 	getelem(t1,v17,v6);
   settable(t2,4,ph2); 	getelem(t2,v17,v1);
   settable(t3,8,ph3); 	
   settable(t4,4,ph4); 	getelem(t4,v17,v13);
   settable(t5,4,ph5); 	getelem(t5,v17,v21);
   settable(t7,8,ph7); 	getelem(t7,v17,v7);
   settable(t8,4,ph8); 	getelem(t8,v17,v8);   
   settable(t11,16,phs8); getelem(t11,v6,v3);   /* 1st echo in ES */
   settable(t12,16,phs9); getelem(t12,v6,v4);   /* 2nd exho in ES */
   settable(t13,4,rec_cor); getelem(t13,v6,v9);

   assign(v1,oph);
   if (getflag("zfilt")) 
	getelem(t3,v17,oph);
   
   if (phase1 == 2)
      {incr(v1); }

   add(v1, v14, v1);
   add(v6, v14, v6);
   add(oph,v14,oph);
   add(oph,v9,oph); mod4(oph,oph); /* correct oph for Excitation Sculpting */

/* The following is for flipback pulse */
   if (phincr1 < 0.0) phincr1=360+phincr1;
   initval(phincr1,v10);

   if (alt_grd[0] == 'y') mod2(ct,v2); /* alternate gradient sign on even scans */

/* BEGIN ACTUAL PULSE SEQUENCE CODE */
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
      obspower(slpwrT);
      txphase(v13);
      if (d2 > 0.0)
       delay(d2 - rof1 - POWER_DELAY - (2*pw/PI));
      else
       delay(d2);


      if (mixT > 0.0)
      { 
	rgpulse(trim,v13,0.0,0.0);
	if (dps_flag)
	  rgpulse(mixT,v21,0.0,0.0);
	else
	  SpinLock(slpatT,mixT,slpwT,v21);
       }
	
       if ((getflag("zfilt")) && (getflag("PFGflg")))
        {
           obspower(tpwr);
           rgpulse(pw,v7,1.0e-6,rof1);
           ifzero(v2); zgradpulse(gzlvlz,gtz);
           elsenz(v2); zgradpulse(-gzlvlz,gtz); endif(v2);
           delay(gtz/3);
	   if (getflag("flipback"))
		FlipBack(v8,v10);
           rgpulse(pw,v8,rof1,2.0e-6);
        }
      ExcitationSculpting(v3,v4,v2);
      delay(rof2);
           
   status(C);
}
