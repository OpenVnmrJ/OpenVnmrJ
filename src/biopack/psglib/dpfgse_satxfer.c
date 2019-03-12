/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*dpfgse_satxfer  1D STD Saturation Transfer Difference experiment with
                	DPFGSE sculpted solvent suppression 
	Reference (STD): Mayer and Meyer J.A.Ch.Soc.2001,123,6108-6117
                  DPFGSE: T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995)
                          C. Dalvit; J. Biol. NMR, 11, 437-444 (1998)
	
	Paramters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
        gt0         - gradient duration for sspul
        gzlvl0      - gradient power for sspul
	dpfgse_flg  - y - does post sequence water suppr. via excitation
		 		sculpting
        trim_flg    - flag for optional trim pulse to suppress
				protein background
	trim        - T1rho spinlock mixing time (CW)
	trimpwr	    - spin-lock power level during trim
	d1	    - relaxation delay
        xferdly     - saturation transfer delay ( ~1.5-2sec)
        satshape    - shape of the pulses in the pulse-train (def:gauss)
        d3          - interpulse delay in the xfer pulsetrain
                        ( 1 msec is recommenden in the literature
                                with no obvious reason)
        satpwr      - power level for the saturation pulse-train
                       (in the literature 86 Hz peak power is recommended
                       corresponding to a 630 deg. flip angle at 50 msec satpw
                       please note that the actual flip angle is irrelevant,
                       the selectivity is controlled by the power level, satpwr)
        satpw       - pulse width of the shaped pulses in the pulse train
                            duration ca 50 ms
	satfrq      - frequency for protein saturation
                         (use a region with an intense protein signal
                         and NO ligand signal) 
        satfrqref   - reference frequency (outside the signal region ~at 30ppm)
        wrefshape   - shape of the 180 deg pulse for solvent supp.
                         rectangular shape is recommended
        wrefpwr     - power of 180 deg pulse for solvent suppression
        wrefpwrf    - fine power for wrefshape by default it is 2048
                       needs optimization for multiple solvent suppression only
                          with fixed wrefpw
	wrefpw	    - selective 180 deg pulse width for solvent supp
				duration ca 2-4 ms
	gzlvl2      - gradient levels during the DPFG echos
	gt2         - gradient time during the DPFG echos
	gstab       - recovery delay
        alt_grd     - flag to alternate gradient signs after each subsequent
                           nt pairs

	Igor Goljer  June 9 2003 / Peter Sandor Nov. 2003
*/

#include <standard.h>

static int	ph1[8] = {0,0,2,2,1,1,3,3},  /* observe pulse  */
		ph2[8] = {1,1,1,1,0,0,0,0},  /* trim pulse     */

		ph3[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
                ph4[16] = {2,2,2,2,3,3,3,3,0,0,0,0,1,1,1,1},
		ph5[32] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
                           2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3},
                ph6[32] = {2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,
                           0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},

                ph7[8] = {0,2,2,0,1,3,3,1}, /* RCVR with no dpfgse */
                ph8[32] = {0,2,2,0,3,1,1,3,2,0,0,2,1,3,3,1,
                           0,2,2,0,3,1,1,3,2,0,0,2,1,3,3,1};
                                          /* RCVR with dpfgse */
void pulsesequence()
{
   double	   gzlvl0 = getval("gzlvl0"),
		   gt0 = getval("gt0"),
		   trimpwr = getval("trimpwr"),
		   trim = getval("trim"),
		   gzlvl2 = getval("gzlvl2"),
		   gt2 = getval("gt2"),
		   gstab =getval("gstab"),
		   satpwr = getval("satpwr"),
                   wrefpwr = getval("wrefpwr"),
                   wrefpwrf = getval("wrefpwrf"),
                   satfrq = getval("satfrq"),
                   satfrqref = getval("satfrqref"),
                   satpw = getval("satpw"),
                   wrefpw = getval("wrefpw"),
                   d3 = getval("d3"),
                   xferdly = getval("xferdly"),
                   cycles;
   char            sspul[MAXSTR],satshape[MAXSTR], wrefshape[MAXSTR],dpfgse_flg[MAXSTR],
                   alt_grd[MAXSTR],trim_flg[MAXSTR];

   getstr("satshape",satshape);
   getstr("wrefshape",wrefshape);
   getstr("sspul", sspul);
   getstr("dpfgse_flg",dpfgse_flg);
   getstr("alt_grd",alt_grd);
   getstr("trim_flg",trim_flg);
   cycles = xferdly/(d3+satpw) + 0.5;
   initval(cycles,v11);

   sub(ct,ssctr,v12);
   settable(t1,8,ph1);    getelem(t1,v12,v1);
   settable(t2,8,ph2);    getelem(t2,v12,v2);
   settable(t3,16,ph3);    getelem(t3,v12,v3);
   settable(t4,16,ph4);    getelem(t4,v12,v4);
   settable(t5,32,ph5);    getelem(t5,v12,v5);
   settable(t6,32,ph6);    getelem(t6,v12,v6);

   if (dpfgse_flg[0] == 'n')
      { settable(t7,8,ph7); getelem(t7,v12,oph); }
   else
      {settable(t8,32,ph8); getelem(t8,v12,oph); }

   mod2(ct,v9);                /*  0 1 0 1 0 1 0 1 ..frequency  switch 
                                  on every second transient */
   if (alt_grd[0] == 'y') { hlv(ct,v10); mod2(v10,v10); } /*  00 11 */
               /* alternate gradient sign on every 2nd transient pair */

/* BEGIN THE ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tpwr);
   if (sspul[0] == 'y')
     {
	zgradpulse(gzlvl0,gt0);
	rgpulse(pw,zero,rof1,rof1);
	zgradpulse(gzlvl0,gt0);
     }
   if (d1 > xferdly) delay(d1-xferdly); 

             /* set saturation frequencies */
   ifzero(v9); obsoffset(satfrq);
   elsenz(v9); obsoffset(satfrqref);
   endif(v9);

 /*  Start the selective saturation of protein */ 

    obspower(satpwr);
    if (cycles > 0.0)
   {
    starthardloop(v11);
      delay(d3);
      shaped_pulse(satshape,satpw,zero,rof1,rof1);
      endhardloop(); 
   }
   obspower(tpwr); obsoffset(tof);
status(B);
      if ((trim_flg[0] == 'n')&&(dpfgse_flg[0]=='n'))
         rgpulse(pw, v1, rof1, rof2);
      else rgpulse(pw, v1, rof1, rof1);
              /* spin lock pulse for dephasing of protein signals */
     if (trim_flg[0] == 'y')
      { obspower(trimpwr);
        if (dpfgse_flg[0]=='n') rgpulse(trim,v2,rof1,rof2);        
        else rgpulse(trim,v2,rof1,rof1);
      }

    /*  solvent suppression using excitation sculpting    */
    if (dpfgse_flg[0] == 'y') 
      {
       ifzero(v10); zgradpulse(gzlvl2,gt2);
              elsenz(v10); zgradpulse(-gzlvl2,gt2); endif(v10);
       obspower(wrefpwr+6); obspwrf(wrefpwrf);
       delay(gstab);
       shaped_pulse(wrefshape,wrefpw,v3,rof1,rof1);
       obspower(tpwr); obspwrf(4095.0);
       rgpulse(2.0*pw,v4,rof1,rof1);
       ifzero(v10); zgradpulse(gzlvl2,gt2);
              elsenz(v10); zgradpulse(-gzlvl2,gt2); endif(v10);
       obspower(wrefpwr+6); obspwrf(wrefpwrf);
       delay(gstab);
       ifzero(v10); zgradpulse(1.2*gzlvl2,gt2);
              elsenz(v10); zgradpulse(-1.2*gzlvl2,gt2); endif(v10);
       delay(gstab);
       shaped_pulse(wrefshape,wrefpw,v5,rof1,rof1);
       obspower(tpwr); obspwrf(4095.0);
       rgpulse(2.0*pw,v6,rof1,rof2);
       ifzero(v10); zgradpulse(1.2*gzlvl2,gt2);
              elsenz(v10); zgradpulse(-1.2*gzlvl2,gt2); endif(v10);
       delay(gstab);
      }
status(C);
}
