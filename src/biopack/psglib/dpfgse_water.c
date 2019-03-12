/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dpfgse_water -   experiment with water suppression by gradient echo.  
Literature reference: 
	T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995)
	C. Dalvit; J. Biol. NMR, 11, 437-444 (1998)

Parameters:
	sspul	    - flag for optional GRD-90-GRD steady-state sequence
	gt0	    - gradient duration for sspul
	gzlvl0	    - gradient power for sspul
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
	wrefshape   - shape file of the 180 deg. selective refocussing pulse
			on the solvent (may be convoluted for multiple solvents)
	wrefpw	    - pulse width for wrefshape (as given by Pbox)
	wrefpwr	    - power level for wrefshape (as given by Pbox)	
        wrefpwrf    - fine power for wrefshape by default it is 2048
		        needs optimization for multiple solvent suppression only
				with fixed wrefpw 
        gt2	    - gradient duration for the solvent suppression echo
	gzlvl2	    - gradient power for the solvent suppression echo
        alt_grd     - alternate gradient sign(s) in dpfgse on even transients
	gstab	    - gradient stabilization delay
	trim_flg    - flag for an optional trim pulse at the end of the sequence
				set trim_flg='y' tu suppress protein background
	trim	    - pulse width of the trim pusle for trim_flg='y' (in seconds!!)	
        trimpwr	    - power level for the trim pulse for trim_flg='y'
        compH       - 1H amplifier compression

The water refocussing shape and the water flipback shape can be created/updatedted
using the "BPupdate_wrefshape" and "BPupdate_flipshape" macros, respectively. This is 
done automatically in the setup macro "dpfgse_water" run by the selection of the
experiment in the drop-down menu. 

For multiple frequency solvent suppression the wrefshape file needs to be created
manually.
p. sandor, darmstadt june 2003.
modified for BioPack, August 2007, G.Gray, Palo Alto
 
*/ 
#include <standard.h> 
 
static int 
 
    ph1[1] = {0}, 
    ph2[16] = {2,2,2,2,3,3,3,3,0,0,0,0,1,1,1,1},
    ph3[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
    ph4[4] = {2,3,0,1},
    ph5[4] = {0,1,2,3},
    phr[16] = {0,2,0,2,2,0,2,0,0,2,0,2,2,0,2,0};

void pulsesequence() 
{ 
   double          gzlvl2 = getval("gzlvl2"), 
                   gt2 = getval("gt2"), 
                   gzlvl0 = getval("gzlvl0"), 
                   gt0 = getval("gt0"), 
                   gstab = getval("gstab"), 
                   phincr1 = getval("phincr1"), 
                   trim = getval("trim"),
                   trimpwr = getval("trimpwr"),
                   flippwr = getval("flippwr"),
                   flippwrf = getval("flippwrf"),
                   flippw = getval("flippw"),
		   wrefpwr = getval("wrefpwr"),
                   wrefpw = getval("wrefpw"),
                   wrefpwrf = getval("wrefpwrf");
 
   char  sspul[MAXSTR],wrefshape[MAXSTR],flipshape[MAXSTR],flipback[MAXSTR],
	 trim_flg[MAXSTR],alt_grd[MAXSTR]; 
 
/* LOAD VARIABLES */ 
 
   rof1 = getval("rof1"); if (rof1 > 2.0e-6) rof1=2.0e-6;
   getstr("sspul",sspul); 
   getstr("wrefshape", wrefshape);
   getstr("flipshape", flipshape);
   getstr("flipback", flipback);
   getstr("trim_flg", trim_flg);
   getstr("alt_grd",alt_grd);
   if (phincr1 < 0.0) phincr1=360+phincr1;
   initval(phincr1,v9);
 
/* CALCULATE PHASECYCLE */ 
         
   settable(t1,1,ph1); 
   settable(t2,16,ph2); 
   settable(t3,16,ph3); 
   settable(t4,4,ph4);
   settable(t5,4,ph5);
   settable(t6,16,phr); 
 
   sub(ct,ssctr,v12); 
   getelem(t1,v12,v1); 
   getelem(t2,v12,v2); 
   getelem(t3,v12,v3); 
   getelem(t4,v12,v4);
   getelem(t5,v12,v5);
   getelem(t6,v12,oph); 
 
   if (alt_grd[0] == 'y') mod2(ct,v6);
               /* alternate gradient sign on every 2nd transient */
 
/* BEGIN THE ACTUAL PULSE SEQUENCE */ 
 status(A); 
      obspower(tpwr); 
      if (sspul[A] == 'y') 
      { 
       zgradpulse(gzlvl0,gt0); 
       rgpulse(pw,zero,rof1,rof1); 
       zgradpulse(gzlvl0,gt0); 
      } 
      if (satmode[A] == 'y') 
       { 
        if (d1>satdly) delay(d1-satdly);
        if (fabs(tof-satfrq)>0.0) obsoffset(satfrq); 
        obspower(satpwr); 
        rgpulse(satdly,zero,rof1,rof1); 
        if (fabs(tof-satfrq)>0.0) obsoffset(tof); 
        obspower(tpwr); 
       } 
      else 
       delay(d1); 

     status(B); 
      if (flipback[A] == 'y') 
      { 
      obsstepsize(1.0);
      xmtrphase(v9);
      add(v1,two,v8);
      obspower(flippwr+6); obspwrf(flippwrf);
      shaped_pulse(flipshape,flippw,v8,rof1,rof1); 
      xmtrphase(zero);
      obspower(tpwr); obspwrf(4095.0);
      } 
      rgpulse(pw, v1, rof1, rof1); 
 
       ifzero(v6); zgradpulse(gzlvl2,gt2);
              elsenz(v6); zgradpulse(-gzlvl2,gt2); endif(v6);
       obspower(wrefpwr+6); obspwrf(wrefpwrf);
       delay(gstab);
       shaped_pulse(wrefshape,wrefpw,v2,rof1,rof1);
       obspower(tpwr); obspwrf(4095.0);
       rgpulse(2.0*pw,v3,rof1,rof1);
       ifzero(v6); zgradpulse(gzlvl2,gt2);
              elsenz(v6); zgradpulse(-gzlvl2,gt2); endif(v6);
       obspower(wrefpwr+6); obspwrf(wrefpwrf);
       delay(gstab);
       ifzero(v6); zgradpulse(1.2*gzlvl2,gt2);
              elsenz(v6); zgradpulse(-1.2*gzlvl2,gt2); endif(v6);
       delay(gstab);
       shaped_pulse(wrefshape,wrefpw,v4,rof1,rof1);
       obspower(tpwr); obspwrf(4095.0);
       if (trim_flg[A] == 'y') rgpulse(2.0*pw,v5,rof1,0.0);
       else        rgpulse(2.0*pw,v5,rof1,rof2);
       ifzero(v6); zgradpulse(1.2*gzlvl2,gt2);
              elsenz(v6); zgradpulse(-1.2*gzlvl2,gt2); endif(v6);
       delay(gstab);
       if (trim_flg[A] == 'y')
           { obspower(trimpwr);
             add(v1,one,v10);
            rgpulse(trim,v10,rof1,rof2);
           }
     status(C);
} 
