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
/**********************************************************
 DgcsteSL - Stimulated echo sequence with self-compensating gradient schemes

Parameters:
	delflag   - 'y' runs the DgcsteSL sequence
                    'n' runs the normal s2pul sequence
        del       -  the actual diffusion delay
        gt1       - total diffusion-encoding pulse width
        gzlvl1    - diffusion-encoding pulse strength
        gstab     - gradient stabilization delay (~0.0002-0.0003 sec)
	tweek     - tuning factor for limiting eddy currents, 
                     ( can be set from 0 to 1, usually set to 0.0 )
        prg_flg   - 'y' selects purging pulse (default)
                    'n' omits purging pulse
	prgtime   - purging pulse length (~0.002 sec)
	prgpwr    - purging pulse power	
	alt_grd   - flag to invert gradient sign on alternating scans
			(default = 'n')
       lkgate_flg - flag to gate the lock sampling off during    
                              gradient pulses
        satmode   - 'y' turns on presaturation
        wet       - flag for optional wet solvent suppression
        sspul     - flag for a GRD-90-GRD homospoil block
        gzlvlhs   - gradient level for sspul
        hsgt      - gradient duration for sspul
        ncomp     - determines the number of components to be used in fitting
                        the signal decay in DOSY when dosyproc='discrete'.
        nugflag   - 'n' uses simple mono- or multi-exponential fitting to
                          estimate diffusion coefficients
                    'y' uses a modified Stejskal-Tanner equation in which the
                          exponent is replaced by a power series
     nugcal_[1-5] - a 5 membered parameter array summarising the results of a
                        calibration of non-uniform field gradients. Used if
                        nugcal is set to 'y'
                        requires a preliminary NUG-calibration by the 
                        Doneshot_nugmap sequence
        dosyproc  - 'discrete' - invokes monoexponential fitting with dosyfit if 
                        ncomp=1, and multiexponential fitting with the external
                        programme SPLMOD if ncomp>1.
                    'continuous' invokes processing with the external programme
                        CONTIN and gives a continuous distribution
                        in the diffusion domain.
     dosypypoints - 'n' divides the spectrum into individual peaks, creating one 
                        cross-peak for each individual peak found in the 1D spectrum
                    'y' performs a diffusion fit for every point in the displayed
                        region of the spectrum that lies above the threshold th
           probe_ - stores the probe name used for the dosy experiment

**********************************************************/

#include<standard.h>
#include <chempack.h>

pulsesequence()
{
double	dosytimecubed,dosyfrq,Ddelta,delcor,
        gzlvl1  = getval("gzlvl1"),
	gt1     = getval("gt1"),
	gstab   = getval("gstab"),
	del     = getval("del"),
	tweek   = getval("tweek"),
	prgtime = getval("prgtime"),
        prgpwr  = getval("prgpwr"),
        gzlvlhs  = getval("gzlvlhs"),
	hsgt    = getval("hsgt"),
        satpwr  = getval("satpwr"),
        satdly  = getval("satdly"),
        satfrq  = getval("satfrq");        
char	delflag[MAXSTR],lkgate_flg[MAXSTR],alt_grd[MAXSTR],
        prg_flg[MAXSTR],sspul[MAXSTR],satmode[MAXSTR];

   getstr("delflag",delflag);
   getstr("alt_grd",alt_grd);
   getstr("prg_flg",prg_flg);
   getstr("lkgate_flg",lkgate_flg);
   getstr("satmode",satmode);
   getstr("sspul",sspul);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",1.0);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);

   if (del<(3*gt1+3.0*gstab+2.0*rof1+pw*2.0))
   {  abort_message("DgcsteSL error: del is less than %f, too short!",
           (3*gt1+3.0*gstab+2.0*rof1+pw*2.0));
   }

   /* Safety check for the duration of the purge pulse */
   if (prgtime > 2.0e-3)
   {  text_error("prgtime has been reset to a maximum of 2 ms");
      prgtime = 2.0e-3;
   }
   dosyfrq = sfrq;
   Ddelta  = gt1;             /* the diffusion-encoding pulse width is gt1 */
   dosytimecubed = Ddelta*Ddelta*(del-(Ddelta/3.0));
   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);

   /* phase cycling calculation */
   if (delflag[0]=='y')
   {  hlv(ct,v4);
      mod2(v4,v2);
      mod2(ct,v1);
      dbl(v1,v1);
      add(v2,v1,v1);
      mod4(v1,v1); 	/* 0 2 1 3 , 1st 90 */
      hlv(v4,v4);
      mod2(v4,v3);
      dbl(v3,v3);	/* (0)4 (2)4 */
      hlv(v4,v4);
      mod2(v4,v2);
      dbl(v2,v2);	/* (0)8 (2)8 */
      hlv(v4,v4);
      mod2(v4,v5);
      add(v5,v3,v3);
      mod4(v3,v3);	/* {(0)4 (2)4}2 {(1)4 (3)4}2, 3rd 90 */
      hlv(v4,v4);
      assign(v4,v6);
      mod2(v4,v4);
      add(v4,v2,v2);
      mod4(v2,v2);	/* {(0)8 (2)8}2 {(1)8 (3)8}2, 2nd 90 */
      mod2(ct,v10);	/* gradients change sign on every odd scan */

      assign(v3,oph);
      add(oph,v2,oph);
      sub(oph,v1,oph);
      mod4(oph,oph);
      add(two,oph,oph);

      assign(oph,v5);
      add(one,v5,v5);
      mod2(ct,v7);
      dbl(v7,v7);
      hlv(v6,v6);
      mod2(v6,v6);
      dbl(v6,v6);
      add(v7,v5,v5);	/* the purge pulse is 90 deg. off the receiver phase */
      add(v6,v5,v5);	/* it is phase alternated every increment */
      mod4(v5,v5);	/* and also every 64 increments */
   }
   else
   {
      assign(ct,v4);
      assign(oph,v4);	/*v4 used only for normal s2pul- type sequence */
   }

   if (ni > 1.0)
   {  abort_message("DgcsteSL is a 2D, not a 3D dosy sequence:  please set ni to 0 or 1");
   }

   /* equilibrium period */
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
       if (satfrq != tof) obsoffset(satfrq);
       rgpulse(satdly,zero,rof1,rof1);
       if (satfrq != tof) obsoffset(tof);
       obspower(tpwr);
       delay(1.0e-5);
      }
     else
     {  delay(d1); }

 if (getflag("wet")) wet4(zero,one);
   obspower(tpwr);

   status(B);
   /* first part of bppdel sequence */
   if(delflag[0] == 'y')
   {  rgpulse(pw, v1, rof1, 0.0);	/* first 90, v1 */

      if (lkgate_flg[0] == 'y')  lk_hold(); /* turn lock sampling off */

      if (alt_grd[0] == 'y')
      {  ifzero(v10);
           zgradpulse(gzlvl1,gt1);
         elsenz(v10);
           zgradpulse(-1.0*gzlvl1,gt1);
	 endif(v10);
      }
      else
         zgradpulse(gzlvl1,gt1);
      delay(gstab);

      rgpulse(pw, v2, rof1, 0.0);	/* second 90, v2 */

      if (alt_grd[0] == 'y')
      {  ifzero(v10);			/* compensating AND CPS gradient */
   	   zgradpulse(-1.0*gzlvl1*(1.0-tweek),gt1);
	 elsenz(v10);
   	   zgradpulse(gzlvl1*(1.0-tweek),gt1);
	 endif(v10);
      }
      else  /* compensating AND CPS gradient */
	 zgradpulse(-1.0*gzlvl1*(1.0-tweek),gt1);

      delay(gstab);

      delcor=del-3.0*(gt1+gstab)-2.0*rof1-pw*2.0; /* diffusion delay */
      if (satmode[1] == 'y')
      {
        obspower(satpwr);
        if (satfrq != tof) obsoffset(satfrq);
        rgpulse(delcor,zero,rof1,rof1);
        if (satfrq != tof) obsoffset(tof);
        obspower(tpwr);
      }
      else delay(delcor);

      if (alt_grd[0] == 'y')
      {
	 ifzero(v10);
   	   zgradpulse(-1.0*gzlvl1*(1.0+tweek),gt1);
	 elsenz(v10);
   	   zgradpulse(gzlvl1*(1.0+tweek),gt1);
	 endif(v10);
      }
      else
         zgradpulse(-1.0*gzlvl1*(1.0+tweek),gt1);
      delay(gstab);

      rgpulse(pw, v3, rof1, rof1);	/* third 90, v3 */

      if (alt_grd[0] == 'y')
      {
	 ifzero(v10);
	   zgradpulse(gzlvl1,gt1);
	 elsenz(v10);
	   zgradpulse(-1.0*gzlvl1,gt1);
         endif(v10);
      }
      else
         zgradpulse(gzlvl1,gt1);

      /* purge pulse to scramble any magnetisation that 
		is not in phase with the receiver */
      if (prg_flg[0] == 'y')
      {
         delay(gstab);
         obspower(prgpwr);
         rgpulse(prgtime, v5, rof1, rof2);
      }
      else
         delay(gstab+2*pw/PI);
      if (lkgate_flg[0] == 'y') lk_sample(); /* turn lock sampling on */
   }
   else
      rgpulse(pw, v4, rof1, rof2);	/* first 90, v1 */

   status(C);
}

/******************************************************************
64 (or 16) steps phase cycle
Order of cycling: v1 (0 2 1 3), v3(0 2), v2(0 2), v3(1 3), v2(1 3)
receiver = v3-v1+v2

Coherence pathway :
	90	90		90	Acq.
+2
         ________
+1      |        |
        |        |
 0------|        |---------------|
                                 |
-1                               |__________________

-2
	v1       v2             v3
******************************************************************/
