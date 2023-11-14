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
 Dpfgdste - PFG Double Stimulated Echo sequence

Ref: Nilsson M, Gil AM, Delgadillo I, Morris GA. Anal Chem 2004;76:5418-5422 
  
        delflag   - 'y' runs the Dpfgdste sequence
                    'n' runs the normal s2pul sequence
        del       -  the actual diffusion delay
        del2      - delay parameter that can shift the convection compensation
                    sequence elements off the center of the pulse sequence
                    allowing to run a velocity profile
                    can also be negative but in absolute value cannot exceed
                    del minus the gradient and gradient-stabilization delays
                    (default value for diffusion measurements is zero)
        gt1       - total diffusion-encoding pulse width
        gzlvl1    - diffusion-encoding pulse strength
        gzlvl3    - gradient power for the 2nd homospoil gradient
        gt3       - gradient duration for the homospoil gradient
        gzlvl3    - gradient power for the 1st homospoil gradient
        gstab     - gradient stabilization delay (~0.0002-0.0003 sec)
        satmode   - 'y' turns on presaturation during d1
                      and/or during the diffusion delay
        satfrq    - presaturation frequency
        satdly    - saturation delay (part of d1)
        satpwr    - saturation power
        wet       - flag for optional wet solvent suppression
        alt_grd   - flag to invert gradient sign on alternating scans
                        (default = 'n')
       lkgate_flg - flag to gate the lock sampling off  during
                              gradient pulses
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
 
void pulsesequence() 
{ 
double	Ddelta,cdel1,cdel2,
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),
        gzlvl3 = getval("gzlvl3"),
        gt1 = getval("gt1"),
        gt3 = getval("gt3"),
        gstab = getval("gstab"),
        del = getval("del"),
        del2 = getval("del2"),
        satpwr = getval("satpwr"),
        satdly = getval("satdly"),
        satfrq = getval("satfrq"),
        gzlvlhs   = getval("gzlvlhs"),
        hsgt     = getval("hsgt"); 
char	delflag[MAXSTR],satmode[MAXSTR],alt_grd[MAXSTR],lkgate_flg[MAXSTR],
        sspul[MAXSTR]; 
double dosytimecubed,dosyfrq,CORR,del2check,del2max; 
 
  nt = getval("nt"); 
  getstr("delflag",delflag); 
  getstr("satmode",satmode);
  getstr("alt_grd",alt_grd);
  dosyfrq = getval("sfrq"); 
  getstr("lkgate_flg",lkgate_flg);
  getstr("sspul",sspul);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",1.0);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);

  if((del-(3.0*(gt1+gstab)+2.0*(pw+2.0*rof1)))< 0.0){ 
	abort_message("Warning: value of del too short - pulse sequence ABORTING!"); 
	} 
  CORR=3.5*gt1+gt3+4.0*gstab+2.0*pw+3*rof1;
  del2check=del/2.0-CORR-fabs(del2)-rof1;
  del2max=del/2.0-CORR-rof1;
  if (del2check< 0.0)
     abort_message("del2 in absolute value cannot exceed %.6f seconds", del2max);

  Ddelta=gt1; 
  dosytimecubed=Ddelta*Ddelta*(del -2.0*(Ddelta/3.0)); 
  putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq); 
/*	printf("dosytimecubed %e\n",dosytimecubed); */
 
	/* phase cycling calculation */ 
 
   if (delflag[0]=='y') 
      { 
	mod4(ct,v3); 
	hlv(ct,v9); 
	hlv(v9,v9); 
	mod4(v9,v4); 
	hlv(v9,v9); 
	hlv(v9,v9); 
	mod4(v9,v8); 
	add(v8,v3,v2); 
	mod4(v2,v2); 
	hlv(v9,v9); 
	hlv(v9,v9); 
	mod4(v9,v8); 
	add(v8,v4,v5); 
	hlv(v9,v9); 
	hlv(v9,v9); 
	mod4(v9,v1); 
	add(v1,v4,oph); 
	add(oph,v5,oph); 
	sub(oph,v2,oph); 
	sub(oph,v3,oph); 
      } 
      else 
      { 
	assign(ct,v4); 
	assign(oph,v4);	/* v4 used only for normal s2pul-type sequence */
      } 
 
   mod2(ct,v10);        /* gradients change sign at odd transients */

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
       if (d1 - satdly > 0)
         delay(d1 - satdly);
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
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl3,gt3);
            elsenz(v10);
              zgradpulse(gzlvl3,gt3);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl3,gt3);   /* homospoil -H1 */
	delay(gstab); 
 
   	rgpulse(pw, v1, rof1, 0.0);		/* first 90 */ 
 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(gzlvl1,gt1);
            elsenz(v10);
               zgradpulse(-1.0*gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1);
   	delay(gstab); 
 
	rgpulse(pw, v2, rof1, 0.0);		/* second 90 */ 
 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(-1.0*gzlvl1,gt1);
            elsenz(v10);
               zgradpulse(gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1,gt1);
   	delay(gstab); 
	 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl3,gt3);
            elsenz(v10);
              zgradpulse(-1.0*gzlvl3,gt3);
            endif(v10);
         }
         else zgradpulse(gzlvl3,gt3);   /* homospoil -H1 */
 
        cdel1 = del/2.0+del2-(2.0*gt3)-(3.0*gt1)-(3.0*gstab)-2.0*(pw+2.0*rof1);
         if (satmode[1] == 'y')
           {
             obspower(satpwr);
             if (satfrq != tof) obsoffset(satfrq);
             rgpulse(cdel1-2.0*rof1,zero,rof1,rof1);
             if (satfrq != tof) obsoffset(tof);
             obspower(tpwr);
           }
         else delay(cdel1);
 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl2,gt3);
            elsenz(v10);
              zgradpulse(gzlvl2,gt3);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl2,gt3);   /* homospoil -H2 */
 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(-1.0*gzlvl1,gt1);
            elsenz(v10);
               zgradpulse(gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1,gt1);
   	delay(gstab); 
 
   	rgpulse(pw, v3, rof1, 0.0);		/* third 90 */ 
 
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
        delay(gstab);

	rgpulse(pw, v4, rof1, 0.0);		/* fourth 90 */ 
 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(-1.0*gzlvl1,gt1);
            elsenz(v10);
               zgradpulse(gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1,gt1);
   	delay(gstab); 
 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl2,gt3);
            elsenz(v10);
              zgradpulse(-1.0*gzlvl2,gt3);
            endif(v10);
         }
         else zgradpulse(gzlvl2,gt3);   /* homospoil H2 */

        cdel2 = del/2.0-del2-gt3-(3.0*gt1)-(3.0*gstab)-2.0*(pw+2.0*rof1);
         if (satmode[1] == 'y')
           {
             obspower(satpwr);
             if (satfrq != tof) obsoffset(satfrq);
             rgpulse(cdel2-2.0*rof1,zero,rof1,rof1);
             if (satfrq != tof) obsoffset(tof);
             obspower(tpwr);
           }
         else delay(cdel2);
 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(-1.0*gzlvl1,gt1);
            elsenz(v10);
               zgradpulse(gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1,gt1);
   	delay(gstab); 
 
   	rgpulse(pw, v5, rof1, 0.0);		/* fifth 90 */ 
 
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
               zgradpulse(gzlvl1,gt1);
            elsenz(v10);
               zgradpulse(-1.0*gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1);
   	delay(gstab-2.0*pw/3.14159265358979323846); 
        if (lkgate_flg[0] == 'y') lk_sample(); /* turn lock sampling on */
   	} 
   else 
	{ 
   	rgpulse(pw, v4, rof1, rof2);		/* first 90 */ 
	} 
 
   /* --- observe period --- */ 
  
   status(C); 
} 
