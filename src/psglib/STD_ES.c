// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*STD_ES  1D STD Saturation Transfer Difference experiment with
	DPFGSE sculpted solvent suppression 
	Reference (STD): Mayer and Meyer J.A.Ch.Soc.2001,123,6108-6117
                  DPFGSE: T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995)
                          C. Dalvit; J. Biol. NMR, 11, 437-444 (1998)
	
	Paramters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
	ES_flg      - y - does post sequence water suppr. via excitation
		      sculpting
        prg_flg     - flag for an optional purge pulse at the end of the sequence
                                set prg_flg='y' to suppress protein background
        prgtime     - pulse width of the purge pulse for prg_flg='y' (in seconds!!)     
        prgpwr      - power level for the purge pulse for prg_flg='y'
	d1	    - relaxation delay
        xferdly     - saturation transfer delay ( ~1.5-2sec)
        xfershape   - shape of the pulses in the pulse-train (def:gauss)
        xferint     - interpulse delay in the xfer pulsetrain
                      (1 ms is recommended in the literature with no obvious reason)
        xferpwr     - power level for the saturation pulse-train
                      (in the literature 86 Hz peak power is recommended
                      corresponding to a 630 deg. flip angle at 50 msec satpw
                      please note that the actual flip angle is irrelevant,
                      the selectivity is controlled by the power level, satpwr)
        xferpw      - pulse width of the shaped pulses in the pulse train
                      duration ca 50 ms
	xferfrq     - frequency for protein saturation
                      (use a region with an intense protein signal
                      and NO ligand signal) 
        xferfrqref  - reference frequency (outside the signal region ~at 30ppm)
        esshape     - shape file of the 180 deg. selective refocussing pulse
                        on the solvent (may be convoluted for multiple solvents)
        essoftpw    - pulse width for esshape (as given by Pbox)
        essoftpwr   - power level for esshape (as given by Pbox)        
        essoftpwrf  - fine power for esshape by default it is 2048
                        needs optimization for multiple solvent suppression only
                                with fixed essoftpw
        esgzlvl     - gradient power for the solvent suppression echo
        esgt        - gradient duration for the solvent suppression echo
	gstab       - recovery delay
        alt_grd     - flag to alternate gradient signs after each subsequent nt
        lkgate_flg  - lock gating option (on during d1 off during the seq. and at)
        add_data    - 'internal' - coadd ALL scans producing difference 
                                       spectrum at the end
                      'external' - array "on_res" and store on-resonance and control
                                       FIDs separately; needs manual subtraction after
                                       acquisition for best results set blocksize 
                                       (bs) to 1 and use interleave (il='y')
        on_res     - flag to run on/off-resonance array ('y','n')

	Igor Goljer June 9 2003
        Peter Sandor Nov. 2003
        Bert Heise Feb. 2012 [Chempack/VJ3.x version]
        JohnR - includes CPMG option : Jan 2015
        ****v15 is reserved for CPMG ***

*/

#include <standard.h>
#include <chempack.h>
/*
#include <ExcitationSculpting.h>
*/

static int	ph1[8] = {0,0,2,2,1,1,3,3},  /* observe pulse with subtraction */
               ph11[4] = {0,2,1,3},          /* observe pulse with no subtraction */

		ph2[8] = {1,1,1,1,0,0,0,0},  /* PRG pulse with subrtaction    */
               ph21[4] = {1,1,0,0},          /* PRG pulse with no subtraction */

		ph3[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3}, /* 1st ES with subrtaction */
                ph31[8] = {0,0,1,1,2,2,3,3}, /* 1st ES with no subrtaction */
		ph5[32] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
                           2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3}, /* 2nd ES with subrtaction */
               ph51[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3}, /* 2nd ES with no subrtaction */

                 ph7[4] = {0,2,1,3}, /* RCVR no dpfgse no subtraction */
                ph71[8] = {0,0,0,0,1,1,1,1}, /* RCVR with no ES, internal subtraction */
                ph8[32] = {0,0,0,0,3,3,3,3,2,2,2,2,1,1,1,1,
                           0,0,0,0,3,3,3,3,2,2,2,2,1,1,1,1},  /* RCVR with ES & subtraction */
               ph81[16] = {0,2,3,1,2,0,1,3,0,2,3,1,2,0,1,3};  /* RCVR with ES, no subtraction */

pulsesequence()
{
   double	   prgpwr = getval("prgpwr"),
                   prgtime = getval("prgtime"),
		   gzlvl2 = getval("gzlvl2"),
		   gt2 = getval("gt2"),
		   gstab =getval("gstab"),
		   xferpwr = getval("xferpwr"),
                   xferfrq = getval("xferfrq"),
                   xferfrqref = getval("xferfrqref"),
                   xferpw = getval("xferpw"),
                   xferint = getval("xferint"),
                   xferdly = getval("xferdly"),
                   cycles;
   char            sspul[MAXSTR],xfershape[MAXSTR], esshape[MAXSTR],
                   ES_flg[MAXSTR],alt_grd[MAXSTR],prg_flg[MAXSTR],
                   add_data[MAXSTR],on_res[MAXSTR], ESmode[MAXSTR];

   getstr("xfershape",xfershape);
   getstr("esshape",esshape);
   getstr("sspul", sspul);
   getstr("ES_flg",ES_flg);
   getstr("alt_grd",alt_grd);
   getstr("prg_flg",prg_flg);
   getstr("add_data",add_data);
   getstr("on_res",on_res);
   cycles = xferdly/(xferint+xferpw) + 0.5;
   F_initval(cycles,v11);                /* make it interleave compatible */

   sub(ct,ssctr,v12);
   if ((strcmp(add_data,"internal"))&&(ES_flg[0] == 'n')) /* no subtraction, no ES */
    {
     settable(t1,4,ph11);  getelem(t1,v12,v1);
     settable(t2,4,ph21);  getelem(t2,v12,v2);
     settable(t7,4,ph7);   getelem(t7,v12,oph);
    }
   if ((strcmp(add_data,"internal"))&&(ES_flg[0] == 'y')) /* no subtraction, with ES */
    {
     settable(t1,4,ph11);  getelem(t1,v12,v1);
     settable(t2,4,ph21);  getelem(t2,v12,v2);
     settable(t3,8,ph31);  getelem(t3,v12,v3);
     settable(t5,16,ph51); getelem(t5,v12,v5);
     settable(t8,16,ph81); getelem(t8,v12,oph);
    }
   if ((!strcmp(add_data,"internal"))&&(ES_flg[0] == 'n')) /* do subtraction, no ES */
    {
     settable(t1,8,ph1);    getelem(t1,v12,v1);
     settable(t2,8,ph2);    getelem(t2,v12,v2);
     settable(t7,8,ph71);   getelem(t7,v12,oph);
    }
   if ((!strcmp(add_data,"internal"))&&(ES_flg[0] == 'y')) /* do subtraction, with ES */
    {
     settable(t1,8,ph1);    getelem(t1,v12,v1);
     settable(t2,8,ph2);    getelem(t2,v12,v2);
     settable(t3,16,ph3);   getelem(t3,v12,v3);
     settable(t5,32,ph5);   getelem(t5,v12,v5);
     settable(t8,32,ph8);   getelem(t8,v12,oph);
    }

   hlv(ct,v9); mod2(v9,v9);    /*  0 1 0 1... change saturation freq.
                                  on every second transient */
   if (alt_grd[0] == 'y') 
      {  mod2(ct,v10); }  /*  00 11 00 11 */
               /* alternate gradient sign on every 2nd transient pair */

/* BEGIN THE ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tpwr); obsoffset(tof);

   if (getflag("lkgate_flg"))  lk_sample(); /* turn lock sampling on */

   if (getflag("sspul"))
        steadystate();

 if (getflag("ESmode")) 
 {
   if (d1 > xferdly) delay(d1-xferdly); 

 /* Set saturation frequency */

   if (!strcmp(add_data,"internal")) /* if internal subtraction */
    { ifzero(v9); obsoffset(xferfrq);
      elsenz(v9); obsoffset(xferfrqref);
      endif(v9);
    }
   else
    { 
      if (on_res[0] == 'y' )
        obsoffset(xferfrq);
      else
        obsoffset(xferfrqref);
    }

 /*  Start the selective saturation of protein */ 

    obspower(xferpwr);
    delay(1e-5);

    if (cycles > 0.0)
   { 
    starthardloop(v11);
      delay(xferint);
      shaped_pulse(xfershape,xferpw,zero,rof1,rof1);
      endhardloop(); 
   }
   delay(1.0e-5);
   obspower(tpwr); obsoffset(tof);

   if (getflag("lkgate_flg"))  lk_hold(); /* turn lock sampling off */

status(B);
      if (getflag("cpmgflg"))
      {
        rgpulse(pw, v1, rof1, 0.0);
        cpmg(v1, v15);
      }
      else
      {
        if ((prg_flg[0] == 'n')&&(ES_flg[0]=='n'))
          rgpulse(pw, v1, rof1, rof2);
        else
          rgpulse(pw, v1, rof1, 2.0e-6);
      }
    if (ES_flg[0] == 'y') /*  solvent suppression using excitation sculpting    */
      {
         ExcitationSculpting(v3,v5,v10);
         if (prg_flg[0] == 'n') delay(rof2);
      }
    if (prg_flg[0] == 'y') /* spin lock pulse for dephasing of protein signals */
      { 
        obspower(prgpwr);
        rgpulse(prgtime,v2,rof1,rof2);
        obspower(tpwr);
      }
 }
 else
 {
     if (getflag("lkgate_flg")) lk_hold();
status(B);
     rgpulse(pw,v1,rof1,rof2);
 }
status(C);
}
