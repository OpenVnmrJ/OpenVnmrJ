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
/************************************************************************* 
  Dhom2djidosy.c - homonuclear J-resolved experiment with diffusion encoding for dosy  
		          ; absolute value mode is required. 
 
  Nilsson M, Gil AM, Delgadillo I, Morris GA. Anal Chem 2004;76:5418-5422 
 
  Parameters: 
        del - the actual diffusion delay. 
       del2 - delay parameter that can shift the convection compensation
                sequence elements off the center of the pulse sequence
                allowing to run a velocity profile
                can also be negative but in absolute value cannot exceed
                del minus the gradient and gradient-stabilization delays
                (default value for diffusion measurements is zero)
        gt1 - total diffusion-encoding pulse width. 
     gzlvl1 - gradient amplitude (-32768 to +32768) 
     gzlvl2 - gradient amplitude (-32768 to +32768) 
        gt2 - gradient duration in seconds (0.001) 
      gstab - optional delay for stability 
         pw - 90 degree xmtr pulse 
         p1 - 180 degree xmtr pulse 
    satmode - 'yn' turns on presaturation during satdly
              'yy' turns on presaturation during satdly and del
              the presauration happens at the transmitter position
              (set tof right if presat option is used)
     satdly - presatutation delay (part of d1)
     satpwr - presaturation power
        wet - flag for optional wet solvent suppression
  alt_grd   - flag to invert gradient sign on alternating scans
                        (default = 'n')
 lkgate_flg - flag to gate the lock sampling off  during
                              gradient pulses
      sspul - flag for a GRD-90-GRD homospoil block
    gzlvlhs - gradient level for sspul
       hsgt - gradient duration for sspul
         nt - multiple of  1  (minimum) 
              multiple of 16  (maximum and recommended) 
   convcomp - 'y': selects convection compensated cosyidosy 
              'n': normal cosyidosy 
  nugflag   - 'n' uses simple mono- or multi-exponential fitting to
                     estimate diffusion coefficients
              'y' uses a modified Stejskal-Tanner equation in which the
                     exponent is replaced by a power series
nugcal_[1-5] - a 5 membered parameter array summarising the results of a
                     calibration of non-uniform field gradients. Used if
                     nugcal is set to 'y'
                     requires a preliminary NUG-calibration by the 
                     Deoneshot_nugmap sequence
 dosy3Dproc - 'ntype' calls dosy with 3D option with N-type selection
     probe_ - stores the probe name used for the dosy experiment

************************************************************************/ 
 
#include <standard.h> 
#include <chempack.h>
 
void pulsesequence() 
{ 
 double gstab = getval("gstab"),
	gt1 = getval("gt1"),
	gzlvl1 = getval("gzlvl1"),
	gt2 = getval("gt2"),
	gzlvl2 = getval("gzlvl2"),
        satpwr = getval("satpwr"),
        satdly = getval("satdly"),
	del = getval("del"),
	del2 = getval("del2"),
	dosyfrq = getval("sfrq"),
        gzlvlhs = getval("gzlvlhs"),
        hsgt = getval("hsgt"),
	Ddelta,dosytimecubed; 
 char convcomp[MAXSTR],satmode[MAXSTR],alt_grd[MAXSTR],lkgate_flg[MAXSTR],
      sspul[MAXSTR]; 
 
 getstr("convcomp",convcomp); 
 getstr("satmode",satmode);
 getstr("alt_grd",alt_grd);
 getstr("lkgate_flg",lkgate_flg);
 getstr("sspul",sspul);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",1.0);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);
          gt2 = syncGradTime("gt2","gzlvl2",1.0);
          gzlvl2 = syncGradLvl("gt2","gzlvl2",1.0);

/* CHECK CONDITIONS */ 
  if (p1 == 0.0) p1 = 2*pw;
 
/* STEADY-STATE PHASECYCLING 
 This section determines if the phase calculations trigger off of (SS - SSCTR) 
   or off of CT */ 
 
   ifzero(ssctr); 
      dbl(ct, v1); 
      hlv(ct, v3); 
   elsenz(ssctr); 
      sub(ssval, ssctr, v7);	/* v7 = 0,...,ss-1 */ 
      dbl(v7, v1); 
      hlv(v7, v3); 
   endif(ssctr); 
 
/* PHASECYCLE CALCULATION */ 
   hlv(v3, v2); 
   mod2(v3, v3); 
   add(v3, v1, v1); 
   assign(v1, oph); 
   dbl(v2, v4); 
   add(v4, oph, oph); 
   add(v2, v3, v2); 
 
 	Ddelta=gt1;             /*the diffusion-encoding pulse width is gt1*/ 
	if (convcomp[A]=='y') 
		dosytimecubed=Ddelta*Ddelta*(del - (4.0*Ddelta/3.0)); 
        else 
		dosytimecubed=Ddelta*Ddelta*(del - (Ddelta/3.0)); 
 
   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq); 
   mod2(ct,v10);        /* gradients change sign at odd transients */
 
/* BEGIN ACTUAL SEQUENCE */ 
 status(A); 
     if (sspul[0]=='y')
       {
         zgradpulse(gzlvlhs,hsgt);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(gzlvlhs,hsgt);
       }
     if (satmode[0] == 'y')
      {
        if (d1 - satdly > 0) delay(d1 - satdly);
        obspower(satpwr);
        rgpulse(satdly,zero,rof1,rof1);
        obspower(tpwr);
      }
     else delay(d1);

 if (getflag("wet")) wet4(zero,one);

 status(B); 
   if (del>0.0) { 
      if (convcomp[A]=='y')
	{ 
                if (lkgate_flg[0] == 'y')  lk_hold(); /* turn lock sampling off */
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(-gzlvl2,2.0*gt2);
                   elsenz(v10);
                     zgradpulse(gzlvl2,2.0*gt2);
                   endif(v10);
                 }
                else zgradpulse(-gzlvl2,2.0*gt2);
		delay(gstab); 
             rgpulse(pw, v1, rof1, rof1); 
		delay(d2/2.0); 
		delay(gstab); 
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                   elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                   endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
                if (satmode[1] == 'y')
                   { obspower(satpwr);
                     rgpulse(((del+del2)/4)-gt1-2.0*rof1,zero,rof1,rof1);
                     obspower(tpwr);
                   }
                else delay(((del+del2)/4)-gt1);
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                   elsenz(v10);
                     zgradpulse(gzlvl1,gt1);
                   endif(v10);
                 }
                else zgradpulse(-1.0*gzlvl1,gt1);
		delay(gstab); 
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                   elsenz(v10);
                     zgradpulse(gzlvl1,gt1);
                   endif(v10);
                 }
                else zgradpulse(-1.0*gzlvl1,gt1);
                if (satmode[1] == 'y')
                   { obspower(satpwr);
                     rgpulse(((del-del2)/4)-gt1-2.0*rof1,zero,rof1,rof1);
                     obspower(tpwr);
                   }
		else delay(((del-del2)/4)-gt1); 
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                   elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                   endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
		delay(gstab); 
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(gzlvl2,gt2);
                   elsenz(v10);
                     zgradpulse(-1.0*gzlvl2,gt2);
                   endif(v10);
                 }
                else zgradpulse(gzlvl2,gt2);
		delay(gstab); 
             rgpulse(p1, v2, rof1, rof1); 
		delay(gstab); 
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(gzlvl2,gt2);
                   elsenz(v10);
                     zgradpulse(-1.0*gzlvl2,gt2);
                   endif(v10);
                 }
                else zgradpulse(gzlvl2,gt2);
		delay(gstab); 
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                   elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                   endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
                if (satmode[1] == 'y')
                   { obspower(satpwr);
                     rgpulse(((del-del2)/4)-gt1-2.0*rof1,zero,rof1,rof1);
                     obspower(tpwr);
                   }
                else delay(((del-del2)/4)-gt1);
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                   elsenz(v10);
                     zgradpulse(gzlvl1,gt1);
                   endif(v10);
                 }
                else zgradpulse(-1.0*gzlvl1,gt1);
		delay(gstab); 
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                   elsenz(v10);
                     zgradpulse(gzlvl1,gt1);
                   endif(v10);
                 }
                else zgradpulse(-1.0*gzlvl1,gt1);
                if (satmode[1] == 'y')
                   { obspower(satpwr);
                     rgpulse(((del+del2)/4)-gt1-2.0*rof1,zero,rof1,rof1);
                     obspower(tpwr);
                   }
                else delay(((del+del2)/4)-gt1);
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                   elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                   endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
		delay(gstab); 
		delay(d2/2.0); 
                if (lkgate_flg[0] == 'y') lk_sample(); /* turn lock sampling on */
	} 
else
        { 
	     rgpulse(pw, v1, rof1, rof1); 
                if (lkgate_flg[0] == 'y')  lk_hold(); /* turn lock sampling off */
         	delay(d2/2.0); 
        	delay(gstab); 
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                   elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                   endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
                if (satmode[1] == 'y')
                   { obspower(satpwr);
                     rgpulse((del-p1-2.0*rof1-gt1)/2.0-2.0*rof1,zero,rof1,rof1);
                     obspower(tpwr);
                   }
                else delay((del-p1-2.0*rof1-gt1)/2.0);
             rgpulse(p1, v2, rof1, rof1); 
                if (satmode[1] == 'y')
                   { obspower(satpwr);
                     rgpulse((del-p1-2.0*rof1-gt1)/2.0-2.0*rof1,zero,rof1,rof1);
                     obspower(tpwr);
                   }
                else delay((del-p1-2.0*rof1-gt1)/2.0);
                if (alt_grd[0] == 'y')
                 { ifzero(v10);
                     zgradpulse(gzlvl1,gt1);
                   elsenz(v10);
                     zgradpulse(-1.0*gzlvl1,gt1);
                   endif(v10);
                 }
                else zgradpulse(gzlvl1,gt1);
        	delay(gstab); 
        	delay(d2/2.0); 
                if (lkgate_flg[0] == 'y') lk_sample(); /* turn lock sampling on */
	} 
    } 
   else 
	rgpulse(pw,oph,rof1,rof2); 
 status(C); 
} 
