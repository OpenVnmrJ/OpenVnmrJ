// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* water_ES -   experiment with solvent suppression by excitation sculpting.  
Literature reference: 
	T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995)
	C. Dalvit; J. Biol. NMR, 11, 437-444 (1998)

Parameters:
	sspul	    - flag for optional GRD-90-GRD steady-state sequence
	flipback    - flag for an optional selective 90 flipback pulse
				on the solvent to keep it along z all time
	flipshape   - shape of the selective pulse for flipback='y'
	flippw	    - pulse width of the selective pulse for flipback='y'
	flippwr	    - power level of the selective pulse for flipback='y'
        flippwrf    - fine power for flipshape by default it is 2048
                        may need optimization with fixed flippw and flippwr
	phincr1	    - small angle phase shift between the hard and the
		            selective pulse for flipback='y' (1 deg. steps)
                            to be optimized for best result
	esshape     - shape file of the 180 deg. selective refocussing pulse
			on the solvent (may be convoluted for multiple solvents)
	essoftpw    - pulse width for esshape (as given by Pbox)
	essoftpwr   - power level for esshape (as given by Pbox)	
        essoftpwrf  - fine power for esshape by default it is 2048
		        needs optimization for multiple solvent suppression only
				with fixed essoftpw
	esgzlvl	    - gradient power for the solvent suppression echo
        alt_grd     - alternate gradient sign(s) in dpfgse on even transients
	prg_flg     - flag for an optional purge pulse at the end of the sequence
				set prg_flg='y' tu suppress protein background
	prgtime	    - pulse width of the purge pulse for prg_flg='y' (in seconds!!)	
        prgpwr	    - power level for the purge pulse for prg_flg='y'
        lkgate_flg  - lock gating option (on during d1 off during the seq. and at)

The water refocusing shape and the water flipback shape can be created/updated
using the "make_es_shape" and "make_es_flipshape" macros, respectively. For
multiple frequency solvent suppression the esshape file needs to be created
manually.

p. sandor, darmstadt june 2003.
b. heise, oxford february 2012 [Chempack/VJ3.x version]
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***
 
*/ 
#include <standard.h> 
#include <chempack.h> 
/*
#include <ExcitationSculpting.h>
#include <FlipBack.h>
*/

static int 
 
    ph1[1] = {0}, 
    ph2[16] = {2,2,2,2,3,3,3,3,0,0,0,0,1,1,1,1},
    ph3[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
    ph4[4] = {2,3,0,1},
    ph5[4] = {0,1,2,3},
    phr[16] = {0,2,0,2,2,0,2,0,0,2,0,2,2,0,2,0};

pulsesequence() 
{ 
   double          prgtime = getval("prgtime"),
                   prgpwr = getval("prgpwr"), 
                   phincr1 = getval("phincr1"); 
 
   char  sspul[MAXSTR],flipback[MAXSTR],ESmode[MAXSTR],
	 prg_flg[MAXSTR],alt_grd[MAXSTR]; 
 
/* LOAD VARIABLES */ 
 
   rof1 = getval("rof1"); if (rof1 > 2.0e-6) rof1=2.0e-6;
   getstr("sspul",sspul); 
   getstr("flipback", flipback);
   getstr("prg_flg", prg_flg);
   getstr("alt_grd",alt_grd);
   getstr("ESmode",ESmode);
   if (phincr1 < 0.0) phincr1=360+phincr1;
   initval(phincr1,v9); 
 
/* CALCULATE PHASECYCLE */ 
         
   sub(ct,ssctr,v12);
   settable(t1,1,ph1);		getelem(t1,v12,v1); 
   settable(t2,16,ph2); 	getelem(t2,v12,v2);
   settable(t3,16,ph3); 	getelem(t3,v12,v3);
   settable(t4,4,ph4);		getelem(t4,v12,v4);
   settable(t5,4,ph5);		getelem(t5,v12,v5);
   settable(t6,16,phr); 	getelem(t6,v12,oph);
 
   if (alt_grd[0] == 'y') mod2(ct,v6);
               /* alternate gradient sign on every 2nd transient */
 
/* BEGIN THE ACTUAL PULSE SEQUENCE */ 
 status(A); 

   if (getflag("lkgate_flg"))  lk_sample(); /* turn lock sampling on */

   obspower(tpwr); delay(5.0e-5);

   if (getflag("sspul"))
        steadystate();

   delay(d1);

   if (getflag("lkgate_flg"))  lk_hold(); /* turn lock sampling off */

     status(B); 
      if (getflag("ESmode"))
      {
      	if (flipback[A] == 'y') 
            FlipBack(v1,v9);            /* water flipback pulse */
        if (getflag("cpmgflg"))
        {
          rgpulse(pw, v1, rof1, 0.0);
          cpmg(v1, v15);
        }
        else
      	  rgpulse(pw, v1, rof1, rof1); 
      	ExcitationSculpting(v2,v4,v6);
       	if (prg_flg[A] == 'y')    /* optional purge pulse */
           { obspower(prgpwr);
             add(v1,one,v1);
             rgpulse(prgtime,v1,rof1,rof2);
           }
       	else 
	    delay(rof2);
      }
      else
        if (getflag("cpmgflg"))
        {
          rgpulse(pw, v1, rof1, 0.0);
          cpmg(v1, v15);
        }
        else
	  rgpulse(pw ,v1 ,rof1, rof2); 
     status(C);
} 
