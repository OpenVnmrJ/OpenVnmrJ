/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif
/* 
 */
/* Dghmqcidosy.c - Pulsed Field Gradient HMQC dosy for long range couplings 
		phase sensitive version 
 
Warning: This pulse sequence requires that 1/(2*jnxh) > del, i.e. the 
	transfer delay is longer than the diffusion delay

Reference: Phys. Chem. Chem Phys. 6, 3221-3227 (2004).

Parameters:
        del - the actual diffusion delay.
        gt1 - total diffusion-encoding pulse width.
     gzlvl1 - gradient amplitude (-32768 to +32768)
     gzlvlE - gradient amplitude for coherence selection (HMQC)
        gtE - gradient duration in seconds for coherence selection(0.002)
    EDratio - gradient ratio for HMQC selection
   gzlvl_max - maximum gradient power (2048 for porforma I,
                  32768 for Performa II, IV, and Triax )
      gstab - optional delay for stability
        pwx - 90. deg, X pulse
     pwxlvl - power level for pwx
    alt_grd - flag to invert gradient sign on alternating scans
                        (default = 'n')
 lkgate_flg - flag to gate the lock sampling off during    
                              gradient pulses
       jnxh - heteronuclear coupling for the long-range transfer delay
    satmode - 'yn' turns on presaturation during satdly
              'yy' turns on presaturation during satdly and del
              the presauration happens at the transmitter position
              (set tof right if presat option is used)
     satdly - presatutation delay (part of d1)
     satpwr - presaturation power
        wet - flag for optional wet solvent suppression
      sspul - flag for a GRD-90-GRD homospoil block
    gzlvlhs - gradient level for sspul
       hsgt - gradient duration for sspul
 lkgate_flg - flag to hold lock apart from d1
      phase - 1,2 for phase sensitive data 
   convcomp - 'y': selects convection compensated hmqcidosy
              'n': normal hmqcidosy
 
*/ 
 
#include <standard.h> 
#include <chempack.h>
 
static int	   ph1[2] = {0,2}, 
	 	   ph2[4] = {0,0,2,2}, 
	   	   ph3[4] = {0,2,2,0}; 
 
pulsesequence() 
{ 
  double gzlvl1 = getval("gzlvl1"),
	 gt1 = getval("gt1"), 
         gzlvlE = getval("gzlvlE"),
         gtE = getval("gtE"),
         EDratio = getval("EDratio"),
         gzlvl_max = getval("gzlvl_max"),
         del = getval("del"),
         dosyfrq=getval("sfrq"),
         gstab = getval("gstab"),
         gzlvlhs = getval("gzlvlhs"),
         hsgt = getval("hsgt"),
         gtau,tauc,gt4,gzlvl4,Ddelta,dosytimecubed; 
  char   convcomp[MAXSTR],sspul[MAXSTR],lkgate_flg[MAXSTR],satmode[MAXSTR],
         alt_grd[MAXSTR],arraystring[MAXSTR]; 
  int    iphase, icosel;
 
//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gt1 = syncGradTime("gt1","gzlvl1",1.0);
        gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

  getstr("sspul",sspul); 
  getstr("convcomp",convcomp); 
  getstr("satmode",satmode);
  getstr("lkgate_flg",lkgate_flg); 
  getstr("alt_grd",alt_grd);
  getstr("array",arraystring);
  iphase = (int)(getval("phase")+0.5); 
  icosel = 1; 
  tau = 1/(2*(getval("jnxh"))); 
  gtau =  2*gstab + GRADIENT_DELAY; 
  tauc = gtau+gtE-gstab; 
  if (strcmp(arraystring,"gzlvl1,phase")!=0)
       fprintf(stdout,"Warning:  array should be 'gzlvl1,phase' for this experiment");
Ddelta=gt1;             /* the diffusion-encoding pulse width is gt1 */ 
if (convcomp[0] == 'y') dosytimecubed=2.0*Ddelta*Ddelta*(del-2.0*(Ddelta/3.0));
else dosytimecubed=2.0*Ddelta*Ddelta*(del-(Ddelta/3.0)); 
putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq); 
if (ni>1) putCmd("dosy3Dproc=\'y\'\n");
else putCmd("dosy3Dproc=\'n\'");
 
  if (tau < (gtE+gstab)) 
  { 
    abort_message("0.5/jnxh must be greater than gtE+gstab"); 
  } 
 
  if (tau < (1.0*(gt1+gstab)+del)) 
  { 
    abort_message("0.5/jnxh must be greater than del+gt1+gstab"); 
  } 
  settable(t1,2,ph1); 
  settable(t2,4,ph2); 
  settable(t3,4,ph3); 
 
  assign(zero,v4); 
  getelem(t1,ct,v1); 
  getelem(t3,ct,oph); 
 
  if (iphase == 2)  icosel = -1;
 
  initval(2.0*(double)((int)(d2*getval("sw1")+0.5)%2),v11);  
  add(v1,v11,v1); 
  add(v4,v11,v4); 
  add(oph,v11,oph); 
 
   mod2(ct,v10);        /* gradients change sign at odd transients */

 status(A); 
     decpower(pwxlvl); 
     if (sspul[0] == 'y') 
     { 
         zgradpulse(gzlvlhs,hsgt);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(gzlvlhs,hsgt);
     } 
 
  if (lkgate_flg[0]=='y') lk_sample();  /* turn lock sampling on */
 
	if (convcomp[0]=='y') 
		gt4=gt1*2.0; 
	else 
		gt4=gt1; 

	gzlvl4=sqrt(gzlvl_max*gzlvl_max-gzlvl1*gzlvl1); 
 
         if (satmode[0] == 'y')
          {
            if (d1 - satdly > 0) delay(d1 - satdly);
            obspower(satpwr);
            rgpulse(satdly,zero,rof1,rof1);
            obspower(tpwr);
          }
         else
         {  delay(d1); }

 if (getflag("wet")) wet4(zero,one);

  if (lkgate_flg[0]=='y') lk_hold();   /* turn lock sampling off */
 
 status(B); 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(gzlvl4,gt4);
            elsenz(v10);
               zgradpulse(-1.0*gzlvl4,gt4);
            endif(v10);
         }
         else zgradpulse(gzlvl4,gt4);
	delay(gstab); 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(-1.0*gzlvl4,gt4);
            elsenz(v10);
               zgradpulse(gzlvl4,gt4);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl4,gt4);
	delay(gstab); 
     rgpulse(pw,zero,rof1,1.0e-6); 
	if (convcomp[0]=='y') 
	{ 
		delay((tau - 1.0e-6 - (2*pw/PI)-del-2.333*gt1-2.0*gstab)/2.0); 
                if (alt_grd[0] == 'y')
                 {  ifzero(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                  elsenz(v10);
                     zgradpulse(gzlvl1,gt1);
                  endif(v10);
                 }
                else zgradpulse(-1.0*gzlvl1,gt1);
                if (satmode[1] == 'y')
                  {  obspower(satpwr);
                     rgpulse(del/2.0-5.0*gt1/6.0-2.0*rof1,zero,rof1,rof1);
                  }
		 else delay(del/2.0-5.0*gt1/6.0); 
                if (alt_grd[0] == 'y')
                 {  ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                  elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                  endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
		delay(gstab); 
                if (alt_grd[0] == 'y')
                 {  ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                  elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                  endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
                if (satmode[1] == 'y')
                  {  rgpulse(del/2.0-5.0*gt1/6.0-2.0*rof1,zero,rof1,rof1);
                     obspower(tpwr);
                  }
		else delay(del/2.0-5.0*gt1/6.0); 
                if (alt_grd[0] == 'y')
                 {  ifzero(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                  elsenz(v10);
                     zgradpulse(gzlvl1,gt1);
                  endif(v10);
                 }
                else zgradpulse(-1.0*gzlvl1,gt1);
		delay((tau - 1.0e-6 - (2*pw/PI)-del-2.333*gt1)/2.0); 
	} 
	else 
	{ 
		delay((tau - 1.0e-6 - (2*pw/PI)-del-gt1-gstab)/2.0); 
                if (alt_grd[0] == 'y')
                 {  ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                  elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                  endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
                if (satmode[1] == 'y')
                  {  obspower(satpwr);
                     rgpulse(del-gt1-2.0*rof1,zero,rof1,rof1);
                     obspower(tpwr);
                  }
		else delay(del-gt1); 
                if (alt_grd[0] == 'y')
                 {  ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                  elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                  endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
		delay(gstab); 
		delay((tau - 1.0e-6 - (2*pw/PI)-del-gt1-gstab)/2.0); 
	} 
 
     decrgpulse(pwx,v1,1.0e-6,1.0e-6); 
     delay(gtE+gtau); 
     decrgpulse(2*pwx,v4,1.0e-6,1.0e-6); 
     delay(gstab); 
     if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(gzlvlE,gtE);
            elsenz(v10);
               zgradpulse(-1.0*gzlvlE,gtE);
            endif(v10);
         }
     else zgradpulse(gzlvlE,gtE);
     delay(gstab); 
 
     if (d2/2 > pw) 
        delay(d2/2 - pw); 
     else 
        delay(d2/2); 
     rgpulse(2.0*pw,zero,1.0e-6,1.0e-6); 
     if (d2/2 > pw)  
        delay(d2/2 - pw); 
     else  
        delay(d2/2); 
 
     delay(gstab); 
     if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(gzlvlE,gtE);
            elsenz(v10);
               zgradpulse(-1.0*gzlvlE,gtE);
            endif(v10);
         }
     else zgradpulse(gzlvlE,gtE);
     delay(gstab); 
     decrgpulse(2*pwx,zero,1.0e-6,1.0e-6); 
     delay(gtE+gtau); 
     decrgpulse(pwx,t2,1.0e-6,1.0e-6); 
 
     delay(gstab); 
     if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(icosel*2.0*gzlvlE/EDratio,gtE);
            elsenz(v10);
               zgradpulse(-1.0*icosel*2.0*gzlvlE/EDratio,gtE);
            endif(v10);
         }
     else zgradpulse(icosel*2.0*gzlvlE/EDratio,gtE);
        if (convcomp[0]=='y') 
        { 
                delay((tau-tauc - 1.0e-6 - (2*pw/PI)-del-2.333*gt1-2.0*gstab)/2.0); 
                if (alt_grd[0] == 'y')
                 {  ifzero(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                  elsenz(v10);
                     zgradpulse(gzlvl1,gt1);
                  endif(v10);
                 }
                else zgradpulse(-1.0*gzlvl1,gt1);
                if (satmode[1] == 'y')
                  {  obspower(satpwr);
                     rgpulse(del/2.0-5.0*gt1/6.0-2.0*rof1,zero,rof1,rof1);
                  }
                 else delay(del/2.0-5.0*gt1/6.0);
                if (alt_grd[0] == 'y')
                 {  ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                  elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                  endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
                delay(gstab); 
                if (alt_grd[0] == 'y')
                 {  ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                  elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                  endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
                if (satmode[1] == 'y')
                  {  rgpulse(del/2.0-5.0*gt1/6.0-2.0*rof1,zero,rof1,rof1);
                     obspower(tpwr);
                  }
                else delay(del/2.0-5.0*gt1/6.0);
                zgradpulse(-gzlvl1,gt1); 
                delay((tau-tauc - 1.0e-6 - (2*pw/PI)-del-2.333*gt1)/2.0); 
                if (lkgate_flg[0]=='y') lk_sample();
        }         
        else 
        { 
                delay((tau-tauc - 1.0e-6 - (2*pw/PI)-del-gt1-gstab)/2.0); 
                if (alt_grd[0] == 'y')
                 {  ifzero(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                  elsenz(v10);
                     zgradpulse(gzlvl1,gt1);
                  endif(v10);
                 }
                else zgradpulse(-1.0*gzlvl1,gt1);
                if (satmode[1] == 'y')
                  {  obspower(satpwr);
                     rgpulse(del-gt1-2.0*rof1,zero,rof1,rof1);
                     obspower(tpwr);
                  }
                else delay(del-gt1);
                if (alt_grd[0] == 'y')
                 {  ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                  elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                  endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
                delay(gstab); 
                delay((tau-tauc - 1.0e-6 - (2*pw/PI)-del-gt1-gstab)/2.0); 
                if (lkgate_flg[0]=='y') lk_sample();
        }       
     decpower(dpwr); 
     delay(rof2 - POWER_DELAY); 
 
 status(C); 
}  
