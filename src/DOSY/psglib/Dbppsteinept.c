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
 Dbppsteinept - Bipolar gradient pulses stimulated echo sequence
                combined with INEPT step

	ref : D.Wu, A.Chen and C.S.Johnson, Jr.,
              J. Magn. Reson. Series A, 123, 222-225 (1996)

Parameters:
	delflag - 'y' runs dosyinept
		  'n' runs normal inept without dosy
	del     - the actual diffusion delay
	gt1     - total length of the phase encoding gradient
	gzlvl1  - stenght of the phase encoding gradient
        pp      - 90 deg. hard 1H pulse
        pplvl   - decoupler power level for pp pulses
        sspul   - flag for a GRD-90-GRD homospoil block
        gzlvlhs - gradient level for sspul
        hsgt    - gradient duration for sspul
        sspulX  - flag for a GRD-90-GRD homospoil block during del
                   to destroy original X magnetization (using hsgt and gzlvlhs)
        j1xh    - one-bond X-H coupling
        mult    - multiplicity; 1   selects ch's (doublets);
                                1.5 gives ch2's down, ch's and ch3's up;
                                0.5 enhances all protonated carbons
       alt_grd  - flag to invert gradient sign on alternating scans
                        (default = 'n')
     lkgate_flg - flag to gate the lock sampling off  during
                              gradient pulses
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

#include <standard.h>
#include <chempack.h>

static int      ph1[4]   = {0,0,2,2},
                ph2[128] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
                            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
                            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
                            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
                ph3[32]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
                ph4[8]   = {0,0,0,0,2,2,2,2},
                ph5[64]  = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,
                            0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,
                            1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3,
                            1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3},
                ph6[1]   = {0},
                ph7[1]   = {0},
                ph8[2]   = {0,1},
                rec[64]  = {0,1,2,3,2,3,0,1,0,1,2,3,2,3,0,1,
                            2,3,0,1,0,1,2,3,2,3,0,1,0,1,2,3,
                            2,3,0,1,0,1,2,3,2,3,0,1,0,1,2,3,
                            0,1,2,3,2,3,0,1,0,1,2,3,2,3,0,1},
		ph10[4]  = {0,0,2,2},
		ph11[4]  = {0,1,0,1},
		ph12[4]  = {0,1,2,3};

void pulsesequence()
{
double	corr,tauxh,taum,Ddelta,dosytimecubed,dosyfrq,
        gzlvl1 = getval("gzlvl1"),
        gt1    = getval("gt1"),
       gzlvlhs = getval("gzlvlhs"),
        hsgt   = getval("hsgt"),
        gstab  = getval("gstab"),
        del    = getval("del"),
        pp     = getval("pp"),
        pplvl  = getval("pplvl"),
        j1xh   = getval("j1xh"),
        mult   = getval("mult");
char	alt_grd[MAXSTR],delflag[MAXSTR],sspul[MAXSTR],sspulX[MAXSTR],lkgate_flg[MAXSTR];

   getstr("delflag",delflag);
   getstr("sspul",sspul);
   getstr("sspulX",sspulX);
   getstr("alt_grd",alt_grd);
   getstr("lkgate_flg",lkgate_flg);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",0.5);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",0.5);
   
   if (rof1>10.0e-6) rof1=10.0e-6;
   corr=del-8*rof1-2*hsgt-2*GRADIENT_DELAY-pw-4.0e-4-gstab;
   if (j1xh == 0.0) j1xh=140.0;
      tauxh = 1.0 / (2.0 * j1xh);		/* calculation of delays */
      if (mult == 1.0)
         taum = 1.0 / (2.0 * j1xh);
      else if (mult == 1.5)
         taum = 3.0 / (4.0 * j1xh);
      else
         taum = 1.0 / (3.0 * j1xh);

   if ((tauxh-4*rof1-2*pp < gt1) && (delflag[0]=='y'))
   {  abort_message("Dbppsteinept: decrease gt1 to less than %f to fit into the INEPT delays",
                     (tauxh-4*rof1-2*pp));
   }
   if ((dm[A] == 'y') || (dm[B] == 'y'))
   {  abort_message("Dbppsteinept: Decoupler must be set as dm=nny or n");
   }

   /* Check for the validity of the number of transients selected ! */
   if ( (nt != 4) && ((int)nt%16 != 0) )
   {  abort_message("Dbppsteinept error: nt must be 4, 16 or n*16 (n: integer)");
   }

   if (ni > 1.0)
   {  abort_message("Dbppsteinept is a 2D, not a 3D dosy sequence:  please set ni to 0 or 1");
   }

   mod2(ct,v11);	/* gradients change sign on every odd scan */

   Ddelta=gt1;          /* Dtau = tauxh/2 */
   dosyfrq = dfrq;
   dosytimecubed=Ddelta*Ddelta*(del-(Ddelta/3.0)-(tauxh/4.0));
   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);

   /* phase cycling calculation */

   if (delflag[0]=='y')
   {  settable(t1, 4, ph1);
      settable(t2, 128, ph2);
      settable(t3, 32, ph3);
      settable(t4, 8, ph4);
      settable(t5, 64, ph5);
      settable(t6, 1, ph6);
      settable(t7, 1, ph7);
      settable(t8, 2, ph8);
      settable(t9, 64, rec);
      sub(ct,ssctr,v10);
      getelem(t1,v10,v1);
      getelem(t2,v10,v2);
      getelem(t3,v10,v3);
      getelem(t4,v10,v4);
      getelem(t5,v10,v5);
      getelem(t6,v10,v6);
      getelem(t7,v10,v7);
      getelem(t8,v10,v8);
      getelem(t9,v10,oph);
   }
   else
   {  settable(t10, 4, ph10);
      settable(t11, 4, ph11);
      settable(t12, 4, ph12);
      sub(ct,ssctr,v10);
      getelem(t10,v10,v7);
      getelem(t11,v10,v8);
      getelem(t12,v10,oph);
   }

   /* equilibrium period */
   status(A);
   decpower(pplvl);
   if (sspul[A] == 'y')
     {
      zgradpulse(gzlvlhs,hsgt);
      decrgpulse(pp,zero,rof1,rof1);
      zgradpulse(gzlvlhs,hsgt);
     }
   delay(d1);
   status(B);
   /* first part of bppste sequence */
   if (delflag[0]=='y')
   {  if ( (gt1 > 0) && (gzlvl1 > 0) )
      {  decrgpulse(pp, v1, rof1, rof1);	    /* first 90, v1 */

         if (lkgate_flg[0] == 'y') lk_hold();   /* turn lock sampling off */
         if (alt_grd[0] == 'y')
         {  ifzero(v11);
              zgradpulse(gzlvl1,gt1/2.0);
            elsenz(v11);
              zgradpulse(-1.0*gzlvl1,gt1/2.0);
   	    endif(v11);
         }
         else
            zgradpulse(gzlvl1,gt1/2.0);

         delay(tauxh/2-gt1/2.0-2.0*rof1);
	 decrgpulse(pp*2, v2, rof1, rof1);   /* first 180, v2 */
         if (alt_grd[0] == 'y')
         {  ifzero(v11);
              zgradpulse(-1.0*gzlvl1,gt1/2.0);
            elsenz(v11);
              zgradpulse(gzlvl1,gt1/2.0);
   	    endif(v11);
         }
         else
            zgradpulse(-1.0*gzlvl1,gt1/2.0);

   	 delay(tauxh/2-gt1/2.0-2.0*rof1);

   	 decrgpulse(pp, v3, rof1, rof1);	    /* second 90, v3 */
         if (sspulX[0]=='n')
           {
   	    delay(del-6.0*rof1-tauxh-4.0*pp);
           }
         else
         {  delay(corr-tauxh-4.0*pp);
              zgradpulse(gzlvlhs,hsgt);
              rgpulse(pw,zero,rof1,rof1);
              zgradpulse(gzlvlhs,hsgt);
            delay(gstab);
         }
   	 decrgpulse(pp, v4, rof1, rof1);	    /* third 90, v4 */

         if (alt_grd[0] == 'y')
         {  ifzero(v11);
              zgradpulse(gzlvl1,gt1/2.0);
            elsenz(v11);
              zgradpulse(-1.0*gzlvl1,gt1/2.0);
   	    endif(v11);
         }
         else
            zgradpulse(gzlvl1,gt1/2.0);

   	 delay(tauxh/2-gt1/2.0-2.0*rof1);
	 simpulse(2*pw,pp*2, zero, v5, rof1, rof1);   /* second 180, v5 */

         if (alt_grd[0] == 'y')
         {  ifzero(v11);
              zgradpulse(-1.0*gzlvl1,gt1/2.0);
            elsenz(v11);
              zgradpulse(gzlvl1,gt1/2.0);
   	    endif(v11);
         }
         else
            zgradpulse(-1.0*gzlvl1,gt1/2.0);

         delay(tauxh/2-gt1/2.0-rof1);
         if (lkgate_flg[0] == 'y') lk_sample();     /* turn lock on */
      }
   }
   else
   {  /* Do simple INEPT */
      decrgpulse(pp, v7, rof1, rof1);
      delay(tauxh/2 - 2*rof1 - 3*pp/2);
      simpulse(2*pw, 2*pp, zero, zero, rof1, rof1);
      delay(tauxh/2 - 2*rof1 - 3*pp/2);
   }
   simpulse(pw, pp, v8, one, rof1, rof1);
   delay(taum/2 - 2.0*rof1 - pp);    /* refoc. delay varied for 2D*/
   simpulse(2*pw, 2*pp, v8, zero, rof1, rof1);
   delay(taum/2 - pp);

   /* --- observe period --- */
   decpower(dpwr); 
   status(C);
}

/****************************************************************************
256 (or 16,32,64,128) steps phase cycle
Order of cycling: v1 (0,2), v4(0,2), v1(1,3), v4(1,3), v5(0,2), v3(0,2), v5(1,3) v2(0,2)

receiver = v1-2*v2+v3-v4+2*v5
Coherence pathway :

	90	180	 90		90	180	  Acq.
+2
                  ________                _______ 
+1               |        |              |       |
                 |        |              |       |
 0------|        |        |--------------|       |        
        |        |                               |      
-1      |________|                               |____________________
    
-2
	v1       v2        v3             v4      v5
****************************************************************************/
