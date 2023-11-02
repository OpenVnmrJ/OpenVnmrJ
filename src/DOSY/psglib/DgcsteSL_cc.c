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
  
 DgcsteSL_cc - Stimulated echo sequence with self-compensating gradient schemes 
               and convection compensation

Ref: J. Magn. Reson. 125, 372-375 (1997).
 
  Parameters:
        delflag   - 'y' runs the DgcsteSL_cc sequence
                    'n' runs the normal s2pul sequence
        del       - the actual diffusion delay
        del2      - delay parameter that can shift the convection compensation
                    sequence elements off the center of the pulse sequence 
                    allowing to run a velocity profile
                    can also be negative but in absolute value cannot exceed 
                    del/2 minus the gradient and gradient-stabilization delays
                    (default value for diffusion measurements is zero)
        gt1       - total diffusion-encoding pulse width
        gzlvl1    - diffusion-encoding pulse strength
        gstab     - gradient stabilization delay (~0.0002-0.0003 sec)
        alt_grd   - flag to invert gradient sign on alternating scans
                        (default = 'n')
       lkgate_flg - flag to gate the lock sampling off during    
                              gradient pulses
        triax_flg - flag for using triax gradient amplifiers and probes
                    'y' - homospoil gradients are applied along X- and Y- axis
                          all the diffusion gradients are Z-gradients
                    'n' - all gradients in the sequence are Z-gradients
        gt2	  - 1st homospoil gradient duration
        gzlvl2    - 1st homospoil gradient power level
                        will be X-gradient if triax_flg is set and triax
                        amplifier and probe is available
        gt3       - 2nd homospoil gradient duration
        gzlvl3    - 2nd homospoil gradient power level
                        will be Y-gradient if triax_flg is set and triax
                        amplifier and probe is available
        prg_flg   - 'y' selects purging pulse (default)
                    'n' omits purging pulse
        prgtime   - purging pulse length (~0.002 sec)
        prgpwr    - purging pulse power
        satmode   - flag for optional solvent presaturation
                    'ynn' - does presat during satdly
                    'yyn' - does presat during satdly and the diffusion delay
        satdly    - presaturation delay before the sequence (part of d1)
        satpwr    - saturation power level
        satfrq    - saturation frequency
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

static int ph1[8] =  {0,0,0,0,2,2,2,2},
           ph2[4] =  {0,3,2,1},
           ph3[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           ph4[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                      2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
           ph5[64] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                      2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
                      2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
           ph6[2] =  {1,2},
           ph7[64] = {0,1,2,3,2,3,0,1,2,3,0,1,0,1,2,3,
                      2,3,0,1,0,1,2,3,0,1,2,3,2,3,0,1,
                      2,3,0,1,0,1,2,3,0,1,2,3,2,3,0,1,
                      0,1,2,3,2,3,0,1,2,3,0,1,0,1,2,3};
 
void pulsesequence() 
{ 
double	Ddelta,dosytimecubed,CORR,del2check,del2max,
        gzlvl3  = getval("gzlvl3"),
        gt3     = getval("gt3"),
        gzlvl1  = getval("gzlvl1"),
        gt1     = getval("gt1"),
        gzlvl2  = getval("gzlvl2"),
        gt2     = getval("gt2"),
        gstab   = getval("gstab"),
        del     = getval("del"),
        del2    = getval("del2"),
        dosyfrq = getval("sfrq"),
        prgtime = getval("prgtime"),
        prgpwr  = getval("prgpwr"),
        gzlvlhs = getval("gzlvlhs"),
	hsgt    = getval("hsgt"),
        satdly  = getval("satdly"),
        satpwr  = getval("satpwr"),
        satfrq  = getval("satfrq"); 
char	delflag[MAXSTR],prg_flg[MAXSTR],satmode[MAXSTR],lkgate_flg[MAXSTR],
        triax_flg[MAXSTR],sspul[MAXSTR],alt_grd[MAXSTR]; 
 
   nt = getval("nt"); 
   getstr("delflag",delflag); 
   getstr("prg_flg",prg_flg);
   getstr("triax_flg",triax_flg);
   getstr("satmode",satmode);
   getstr("lkgate_flg",lkgate_flg);
   getstr("sspul",sspul);
   getstr("alt_grd",alt_grd);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",1.0);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);

   CORR=3.0*gt1+gt3+4.0*gstab+2.0*pw+3*rof1;
   if (ni > 1.0)
      abort_message("This is a 2D, not a 3D dosy sequence:  please set ni to 0 or 1");
   if((del-2.0*CORR-gt2-rof1)< 0.0)
      abort_message("The minimum value of del is %.6f seconds", 2*CORR+gt2+rof1);

   del2check=del/2.0-CORR-fabs(del2)-rof1;
   del2max=del/2.0-CORR-rof1;
   if (del2check< 0.0)
      abort_message("del2 in absolute value cannot exceed %.6f seconds", del2max);

   Ddelta=gt1; 
   dosytimecubed=Ddelta*Ddelta*(del - 2.0*(Ddelta/3.0)); 
   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq); 
 
	/* phase cycling calculation */ 
 
  if (delflag[0]=='y')
  { settable(t1,8,ph1);
   settable(t2,4,ph2);
   settable(t3,16,ph3);
   settable(t4,32,ph4);
   settable(t5,64,ph5);
   settable(t6,2,ph6);
   settable(t7,64,ph7);
   sub(ct,ssctr,v10);
   getelem(t1,v10,v1);
   getelem(t2,v10,v2);
   getelem(t3,v10,v3);
   getelem(t4,v10,v4);
   getelem(t5,v10,v5);
   getelem(t6,v10,v6);
   getelem(t7,v10,oph);
  }
  else
  {  add(oph,one,v1); } 

   mod2(ct,v11);        /* gradients change sign at odd transients */

   /* equilibrium period */ 
   status(A); 
     obspower(tpwr);
     if (sspul[0]=='y')
       {
         zgradpulse(gzlvlhs,hsgt);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(gzlvlhs,hsgt);
       }
   if (satmode[0] == 'y')
     {
        if (d1>satdly) delay(d1-satdly);
        obspower(satpwr);
        if (satfrq != tof) obsoffset(satfrq);
        rgpulse(satdly,zero,rof1,rof1);
        if (satfrq != tof) obsoffset(tof);
        obspower(tpwr);
     }
     else delay(d1);

 if (getflag("wet")) wet4(zero,one);
 
   status(B); 
   /* first part of bppdel sequence */ 
   if(delflag[0] == 'y') 
   { 
        if (lkgate_flg[0] == 'y')  lk_hold(); /* turn lock sampling off */
        if (triax_flg[0] == 'y')                 /* homospoil comp. grad */
          { rgradient('x',gzlvl2);              /* along x if available */
            delay(gt2);
            rgradient('x',0.0); }
        else 
          {
           if (alt_grd[0] == 'y')
              { ifzero(v11);
                  zgradpulse(gzlvl2,gt2); /*spoiler comp. gradient*/
                elsenz(v11);
                  zgradpulse(-1.0*gzlvl2,gt2); /*spoiler comp. gradient*/
                endif(v11);
              }
           else zgradpulse(gzlvl2,gt2); /*spoiler comp. gradient*/
          }
	delay(gstab); 
 
   	rgpulse(pw, v1, rof1, rof1);		/* first 90 */ 
 
        if (alt_grd[0] == 'y')
         {  ifzero(v11);
             zgradpulse(gzlvl1,gt1);
            elsenz(v11);
             zgradpulse(-1.0*gzlvl1,gt1);
           endif(v11);
         }
        else zgradpulse(gzlvl1,gt1);
   	delay(gstab); 
 
	rgpulse(pw, v2, rof1, rof1);		/* second 90, v1 */ 
 
        if (alt_grd[0] == 'y')
         {  ifzero(v11);
             zgradpulse(-1.0*gzlvl1,gt1);
            elsenz(v11);
             zgradpulse(1.0*gzlvl1,gt1);
           endif(v11);
         }
        else zgradpulse(-1.0*gzlvl1,gt1);
   	delay(gstab); 
	 
        if (triax_flg[0] == 'y')                 /* homospoil grad */
          { rgradient('x',-1.0*gzlvl2);              /* along x if available */
            delay(gt2);
            rgradient('x',0.0); }
        else 
          {
           if (alt_grd[0] == 'y')
              { ifzero(v11);
                  zgradpulse(-1.0*gzlvl2,gt2); /*spoiler comp. gradient*/
                elsenz(v11);
                  zgradpulse(1.0*gzlvl2,gt2); /*spoiler comp. gradient*/
                endif(v11);
              }
           else zgradpulse(-1.0*gzlvl2,gt2); /*spoiler comp. gradient*/
          }
 
        if (satmode[1] == 'y')
          {
             obspower(satpwr);
             if (satfrq != tof) obsoffset(satfrq);
             rgpulse(del/2.0+del2-CORR-gt3-3*rof1,zero,rof1,rof1);
             if (satfrq != tof) obsoffset(tof);
             obspower(tpwr);
          }
        else delay(del/2.0+del2-CORR-gt3);

        if (triax_flg[0] == 'y')                /* homospoil comp. grad */
          { rgradient('y',gzlvl3);             /* along y if available */
            delay(gt3);
            rgradient('y',0.0); }
        else 
          {
           if (alt_grd[0] == 'y')
              { ifzero(v11);
                  zgradpulse(1.0*gzlvl3,gt3); /*spoiler comp. gradient*/
                elsenz(v11);
                  zgradpulse(-1.0*gzlvl3,gt3); /*spoiler comp. gradient*/
                endif(v11);
              }
           else zgradpulse(1.0*gzlvl3,gt3); /*spoiler comp. gradient*/
          }
        delay(gstab);
 
        if (alt_grd[0] == 'y')
         {  ifzero(v11);
             zgradpulse(-1.0*gzlvl1,gt1);
            elsenz(v11);
             zgradpulse(1.0*gzlvl1,gt1);
           endif(v11);
         }
        else zgradpulse(-1.0*gzlvl1,gt1);
   	delay(gstab); 
 
   	rgpulse(pw, v3, rof1, rof1);		/* third 90 */ 
 
        if (alt_grd[0] == 'y')
         {  ifzero(v11);
             zgradpulse(gzlvl1,gt1);
            elsenz(v11);
             zgradpulse(-1.0*gzlvl1,gt1);
           endif(v11);
         }
        else zgradpulse(gzlvl1,gt1);
   	delay(gstab); 
 
        if (alt_grd[0] == 'y')
         {  ifzero(v11);
             zgradpulse(gzlvl1,gt1);
            elsenz(v11);
             zgradpulse(-1.0*gzlvl1,gt1);
           endif(v11);
         }
        else zgradpulse(gzlvl1,gt1);
   	delay(gstab); 
 
	rgpulse(pw, v4, rof1, rof1);		/* fourth 90 */ 
 
        if (alt_grd[0] == 'y')
         {  ifzero(v11);
             zgradpulse(-1.0*gzlvl1,gt1);
            elsenz(v11);
             zgradpulse(1.0*gzlvl1,gt1);
           endif(v11);
         }
        else zgradpulse(-1.0*gzlvl1,gt1);
   	delay(gstab); 
 
        if (triax_flg[0] == 'y')                /* homospoil grad */
          { rgradient('y',-1.0*gzlvl3);        /* along y if available */
            delay(gt3);
            rgradient('y',0.0); }
        else
          { 
           if (alt_grd[0] == 'y')
              { ifzero(v11);
                  zgradpulse(-1.0*gzlvl3,gt3); /*spoiler comp. gradient*/
                elsenz(v11);
                  zgradpulse(1.0*gzlvl3,gt3); /*spoiler comp. gradient*/
                endif(v11);
              }
           else zgradpulse(-1.0*gzlvl3,gt3); /*spoiler comp. gradient*/
          }
 
        if (satmode[1] == 'y')
          {
             obspower(satpwr);
             if (satfrq != tof) obsoffset(satfrq);
             rgpulse(del/2.0-CORR-del2-2*rof1,zero,rof1,rof1);
             if (satfrq != tof) obsoffset(tof);
             obspower(tpwr);
          }
        else delay(del/2.0-CORR-del2);

        if (alt_grd[0] == 'y')
         {  ifzero(v11);
             zgradpulse(-1.0*gzlvl1,gt1);
            elsenz(v11);
             zgradpulse(1.0*gzlvl1,gt1);
           endif(v11);
         }
        else zgradpulse(-1.0*gzlvl1,gt1);
        delay(gstab);

        if (prg_flg[0] == 'y')
          {  rgpulse(pw, v5, rof1, rof1); }           /* fifth 90 */
        else
          {  rgpulse(pw, v5, rof1, 0.0); }             /* fifth */

        if (alt_grd[0] == 'y')
         {  ifzero(v11);
             zgradpulse(gzlvl1,gt1);
            elsenz(v11);
             zgradpulse(-1.0*gzlvl1,gt1);
           endif(v11);
         }
        else zgradpulse(gzlvl1,gt1);
        delay(gstab-2.0*pw/3.14159265358979323846);
        if (prg_flg[0] == 'y')
            {   obspower(prgpwr);
                rgpulse(prgtime, v6, rof1, 0.0); }
        if (lkgate_flg[0] == 'y') lk_sample(); /* turn lock sampling on */
      } 
   else 
   	rgpulse(pw, v1, rof1, rof2);		/* first 90, v1 */ 
   /* --- observe period --- */ 
   status(C); 
} 
/******************************************************************
64 (or 32 or 16 or 8 or 4) steps phase cycle
Order of cycling: v2(3 2 1 0), v1(0 2), v3(0 2), v4(0 2), v5(0 2)

receiver = v1-v2-v3+v4+v5

Coherence pathway :

        90      90              90       90          90   Acq.
+2
                                  ________
+1                               |        |
                                 |        |
 0------|        |---------------|        |----------|
        |        |                                   |
-1      |________|                                   |______________

-2
        v1       v2             v3        v4         v5
******************************************************************/
