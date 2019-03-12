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
 Dbppste_cc - Bipolar gradient pulses stimulated echo sequence
                	ref : J. Magn. Reson. Ser. A, 115, 260-264 (1995).   
               with convection compensation
                        ref: J. Magn. Reson. 125, 372-375 (1997).
           adapted by P. Sandor, Darmstadt, nov.1998

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
        alt_grd   - flag to invert gradient sign on alternating scans
                        (default = 'n')
       lkgate_flg - flag to gate the lock sampling off  during
                              gradient pulses
        gstab     - gradient stabilization delay (~0.0002-0.0003 sec)
        triax_flg - flag for using triax gradient amplifiers and probes
                    'y' - homospoil gradients are applied along X- and Y- axis
                          all the diffusion gradients are Z-gradients
                    'n' - all gradients in the sequence are Z-gradients
        kappa     - unbalancing factor between bipolar pulses as a
                       proportion of gradient strength (~0.2)
        gt2       - 1st homospoil gradient duration
        gzlvl2    - 1st homospoil gradient power level
                        will be X-gradient if triax_flg is set and triax
                        amplifier and probe is available
        gt3       - 2nd homospoil gradient duration
        gzlvl3    - 2nd homospoil gradient power level
                        will be Y-gradient if triax_flg is set and triax
                        amplifier and probe is available
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

static int phi1[8]  = {0,0,1,1,2,2,3,3},
           phi2[8]  = {1,2,2,3,3,0,0,1},
           phi3[1]  = {0},
           phi4[4]  = {2,2,3,3},
           phi5[16] = {3,3,0,0,3,3,0,0,0,0,1,1,0,0,1,1},
           phi6[1]  = {2},
           phi7[1]  = {0},
           phi8[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                       2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
           phi9[1]  = {0},
           phi10[1] = {0},
           rec[16]  = {0,2,0,2,2,0,2,0,2,0,2,0,0,2,0,2};

void pulsesequence()
{
double	Dtau,Ddelta,dosytimecubed,dosyfrq,CORR,del2check,del2max,
        gzlvl1   = getval("gzlvl1"),
	gt1      = getval("gt1"),
        gzlvl2   = getval("gzlvl2"),
        gt2      = getval("gt2"),
        gzlvl3   = getval("gzlvl3"),
        gt3      = getval("gt3"),
	del      = getval("del"),
        del2     = getval("del2"),
	gstab    = getval("gstab"),
        gzlvlhs   = getval("gzlvlhs"),
	hsgt     = getval("hsgt"),
        kappa    = getval("kappa"),
        gzlvlmax = getval("gzlvlmax"),
        satdly   = getval("satdly"),
        satpwr   = getval("satpwr"),
        satfrq   = getval("satfrq");
char	delflag[MAXSTR],sspul[MAXSTR],satmode[MAXSTR],
        triax_flg[MAXSTR],alt_grd[MAXSTR],lkgate_flg[MAXSTR];

   nt = getval("nt");
   getstr("delflag",delflag);
   getstr("sspul",sspul);
   getstr("triax_flg",triax_flg);
   getstr("satmode",satmode);
   getstr("alt_grd",alt_grd);
   getstr("lkgate_flg",lkgate_flg);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",0.5);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",0.5);

   CORR=1.5*gt1+3*gstab+3.0*pw+5*rof1; 
   if (ni > 1.0)
      abort_message("This is a 2D, not a 3D dosy sequence:  please set ni to 0 or 1");

   /* check the shortest executable delay in the sequence       */
   if ((del-2.0*CORR-gt2-gt3)< 0.0) 
      abort_message("The minimum value of del is %.6f seconds", 2*CORR+gt2+gt3);
   del2check = del/2.0-CORR-fabs(del2)-gt3;
   del2max   = del/2.0-CORR-gt3;
   if (del2check< 0.0) 
      abort_message("del2 in absolute value cannot exceed  %.6f seconds",del2max);
   if (gzlvl1*(1+kappa)>gzlvlmax)
      abort_message("Warning: 'gzlvl1*(1+kappa)>gzlvlmax");
   Ddelta  = gt1;
   Dtau    = 2.0*pw+2.0*rof1+gstab+gt1/2.0;
   dosyfrq = getval("sfrq");
   dosytimecubed = Ddelta*Ddelta*(del-2.0*(Ddelta/3.0)-2.0*(Dtau/2.0));
   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);

	/* phase cycling calculation */
  settable(t1, 8, phi1);
  settable(t2, 8, phi2);
  settable(t3, 1, phi3);
  settable(t4, 4, phi4);
  settable(t5,16, phi5);
  settable(t6, 1, phi6);
  settable(t7, 1, phi7);
  settable(t8,32, phi8);
  settable(t9, 1, phi9);
  settable(t10, 1, phi10);
  settable(t11, 16, rec);
   sub(ct,ssctr,v12);
   getelem(t1,v12,v1);
   getelem(t2,v12,v2);
   getelem(t3,v12,v3);
   getelem(t4,v12,v4);
   getelem(t5,v12,v5);
   getelem(t6,v12,v6);
   getelem(t7,v12,v7);
   getelem(t8,v12,v8);
   getelem(t9,v12,v9);
   getelem(t10,v12,v10);
   getelem(t11,v12,oph);

   mod2(ct,v11);        /* gradients change sign at odd transients */

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
        if (d1>satdly) delay(d1-satdly);
        obspower(satpwr);
        if (satfrq != tof) obsoffset(satfrq);
        rgpulse(satdly,zero,rof1,rof1);
        if (satfrq != tof) obsoffset(tof);
        obspower(tpwr);
     }
    else delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   status(B);
   /* first part of bppste sequence */
   if(delflag[0]=='y')
   	{
   	if(gt1>0 && gzlvl1!=0)
   		{
                 if (lkgate_flg[0] == 'y') lk_hold();   /* turn lock sampling off */
                 if (alt_grd[0] == 'y')
                   { ifzero(v11);
                     zgradpulse(2*kappa*gzlvl1,gt1/2.0);  /* comp. gradient */
                     elsenz(v11);
                     zgradpulse(-2.0*kappa*gzlvl1,gt1/2.0);  /* comp. gradient */
                     endif(v11);
                   }
                 else zgradpulse(2*kappa*gzlvl1,gt1/2.0);  /* comp. gradient */
                delay(gstab);
                   if (triax_flg[0] == 'y')    /* homospoil grad */
                    { rgradient('x',-1.0*gzlvl2);  /* along x if available */
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

   		rgpulse(pw, v1, rof1, rof1);		/* first 90, v1 */

                if (alt_grd[0] == 'y')
                   { ifzero(v11);
                       zgradpulse((1-kappa)*gzlvl1,gt1/2.0);
                     elsenz(v11);
                       zgradpulse(-1.0*(1-kappa)*gzlvl1,gt1/2.0);
                     endif(v11);
                   }
                else zgradpulse((1-kappa)*gzlvl1,gt1/2.0);
                delay(gstab);
		rgpulse(pw*2, v2, rof1, rof1);		/* first 180, v2 */
                if (alt_grd[0] == 'y')
                   { ifzero(v11);
                       zgradpulse(-1.0*(1+kappa)*gzlvl1,gt1/2.0);
                     elsenz(v11);
                       zgradpulse((1+kappa)*gzlvl1,gt1/2.0);
                     endif(v11);
                   }
                else zgradpulse(-1.0*(1+kappa)*gzlvl1,gt1/2.0);
                delay(gstab);

   		rgpulse(pw, v3, rof1, rof1);		/* second 90, v3 */

                   if (triax_flg[0] == 'y')    /* homospoil grad */
                    { rgradient('x',gzlvl2);  /* along x if available */
                      delay(gt2);
                      rgradient('x',0.0); }
                   else 
                    {
                     if (alt_grd[0] == 'y')
                       { ifzero(v11);
                            zgradpulse(gzlvl2,gt2); /*spoiler gradient*/
                         elsenz(v11);
                            zgradpulse(-1.0*gzlvl2,gt2); /*spoiler gradient*/
                         endif(v11);
                       }
                     else zgradpulse(gzlvl2,gt2); /*spoiler gradient*/
                    }
                delay(gstab);
                if (satmode[1] == 'y')
                  {
                     obspower(satpwr);
                     if (satfrq != tof) obsoffset(satfrq);
                     rgpulse(del/2.0-CORR+del2-gt2-gstab-2*rof1,zero,rof1,rof1);
                     if (satfrq != tof) obsoffset(tof);
                     obspower(tpwr);
                  }
                  else delay(del/2.0-CORR+del2-gt2-gstab);
                if (alt_grd[0] == 'y')
                   { ifzero(v11);
                       zgradpulse(2.0*kappa*gzlvl1,gt1/2.0);
                     elsenz(v11);
                       zgradpulse(-2.0*kappa*gzlvl1,gt1/2.0);
                     endif(v11);
                   }
                else zgradpulse(2.0*kappa*gzlvl1,gt1/2.0);
                delay(gstab);

   		rgpulse(pw, v4, rof1, rof1);		/* third 90, v4 */
                if (alt_grd[0] == 'y')
                   { ifzero(v11);
                        zgradpulse((1-kappa)*gzlvl1,gt1/2.0);
                     elsenz(v11);
                        zgradpulse(-1.0*(1-kappa)*gzlvl1,gt1/2.0);
                     endif(v11);
                   }
                else zgradpulse((1-kappa)*gzlvl1,gt1/2.0);
                delay(gstab);
                if (alt_grd[0] == 'y')
                   { ifzero(v11);
                        zgradpulse((1-kappa)*gzlvl1,gt1/2.0);
                     elsenz(v11);
                        zgradpulse(-1.0*(1-kappa)*gzlvl1,gt1/2.0);
                     endif(v11);
                   }
                else zgradpulse((1-kappa)*gzlvl1,gt1/2.0);
                delay(gstab);
                rgpulse(2*pw,v5,rof1,rof1);
                if (alt_grd[0] == 'y')
                   { ifzero(v11);
                        zgradpulse(-1.0*(1+kappa)*gzlvl1,gt1/2.0);
                     elsenz(v11);
                        zgradpulse((1+kappa)*gzlvl1,gt1/2.0);
                     endif(v11);
                   }
                else zgradpulse(-1.0*(1+kappa)*gzlvl1,gt1/2.0);
                delay(gstab);
                if (alt_grd[0] == 'y')
                   { ifzero(v11);
                        zgradpulse(-1.0*(1+kappa)*gzlvl1,gt1/2.0);
                     elsenz(v11);
                        zgradpulse((1+kappa)*gzlvl1,gt1/2.0);
                     endif(v11);
                   }
                else zgradpulse(-1.0*(1+kappa)*gzlvl1,gt1/2.0);
                delay(gstab);
                rgpulse(pw,v6,rof1,rof1);
                if (alt_grd[0] == 'y')
                   { ifzero(v11);
                         zgradpulse(2.0*kappa*gzlvl1,gt1/2.0);
                     elsenz(v11);
                         zgradpulse(-2.0*kappa*gzlvl1,gt1/2.0);
                     endif(v11);
                   }
                else zgradpulse(2.0*kappa*gzlvl1,gt1/2.0);
                
                if (satmode[1] == 'y')
                  {
                     obspower(satpwr);
                     if (satfrq != tof) obsoffset(satfrq);
                     rgpulse(del/2.0-CORR-del2-gt3-2*rof1,zero,rof1,rof1);
                     if (satfrq != tof) obsoffset(tof);
                     obspower(tpwr);
                  }
                  else delay(del/2.0-CORR-del2-gt3);

                if (triax_flg[0] == 'y')    /* homospoil grad */
                    { rgradient('y',gzlvl3); /* along y if available */
                      delay(gt3);
                      rgradient('y',0.0); }
                else 
                    {
                     if (alt_grd[0] == 'y')
                       { ifzero(v11);
                             zgradpulse(gzlvl3,gt3); /* 2nd spoiler gradient */
                         elsenz(v11);
                             zgradpulse(-1.0*gzlvl3,gt3); /* 2nd spoiler gradient */
                         endif(v11);
                       }
                     else zgradpulse(gzlvl3,gt3); /* 2nd spoiler gradient */
                    }
                delay(gstab); 

                rgpulse(pw,v7,rof1,rof1);

                if (alt_grd[0] == 'y')
                   { ifzero(v11);
                          zgradpulse((1-kappa)*gzlvl1,gt1/2.0);
                     elsenz(v11);
                          zgradpulse(-1.0*(1-kappa)*gzlvl1,gt1/2.0);
                     endif(v11);
                   }
                else zgradpulse((1-kappa)*gzlvl1,gt1/2.0);
                delay(gstab);
		rgpulse(pw*2, v8, rof1, rof1);		/* second 180, v8 */
                if (alt_grd[0] == 'y')
                   { ifzero(v11);
                          zgradpulse(-1.0*(1+kappa)*gzlvl1,gt1/2.0);
                     elsenz(v11);
                          zgradpulse((1+kappa)*gzlvl1,gt1/2.0);
                     endif(v11);
                   }
                else zgradpulse(-1.0*(1+kappa)*gzlvl1,gt1/2.0);
                delay(gstab);

                rgpulse(pw,v9,rof1,rof1);              /* z- filter  */
                if (triax_flg[0] == 'y')    /* homospoil grad */
                    { rgradient('y',-1.0*gzlvl3); /* along y if available */
                      delay(gt3);
                      rgradient('y',0.0); }
                else 
                    {
                     if (alt_grd[0] == 'y')
                       { ifzero(v11);
                            zgradpulse(-1.0*gzlvl3,gt3); /* 2nd spoiler comp. gradient */
                         elsenz(v11);
                            zgradpulse(gzlvl3,gt3); /* 2nd spoiler comp. gradient */
                         endif(v11);
                       }
                     else zgradpulse(-1.0*gzlvl3,gt3); /* 2nd spoiler comp. gradient */
                    }
                delay(gstab);
                 if (alt_grd[0] == 'y')
                   { ifzero(v11);
                         zgradpulse(2.0*kappa*gzlvl1,gt1/2.0);
                     elsenz(v11);
                         zgradpulse(-2.0*kappa*gzlvl1,gt1/2.0);
                     endif(v11);
                   }
                 else zgradpulse(2.0*kappa*gzlvl1,gt1/2.0);
                delay(gstab);
                rgpulse(pw,v10,rof1,rof2);
                if (lkgate_flg[0] == 'y') lk_sample();     /* turn lock sampling on */
   	        }
   	}
        else   rgpulse(pw, oph,rof1,rof2);

   /* --- observe period --- */
 
   status(C);
}

/******************************************************************
32 (or 16 or 8 or 4) steps phase cycle

receiver = v1-2*v2+v3-v4+2*v5-v6+v7-2*v8+v9+v10

Coherence pathway :

       90  180  90   90  180   90  90  180  90    90    Acq.
+2
             ____     ____               ____
+1          |    |   |    |             |    |
            |    |   |    |             |    |
 0-----|    |    |---|    |    |---|    |    |----|
       |    |             |    |   |    |         |
-1     |____|             |____|   |____|         |______________

-2
      v1   v2   v3  v4   v5   v6  v7   v8   v9   v10
******************************************************************/

