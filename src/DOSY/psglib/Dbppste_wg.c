/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
/* Dbppste_wg - Bipolar gradient pulses stimulated echo sequence
	ref : J. Magn. Reson. Ser. A, 115, 260-264 (1995).
        with Watergate 3-9-19 solvent suppression

parameters:
	delflag   - 'y' runs the Dbppste_wg sequence
                    'n' runs the normal s2pul sequence
        del       -  the actual diffusion delay
        gt1       - total diffusion-encoding pulse width
        gzlvl1    - diffusion-encoding pulse strength
        alt_grd   - flag to invert gradient sign on alternating scans
                         (default='n')
       lkgate_flg - flag to gate the lock sampling off  during
                              gradient pulses
        d3        - watergate delay (the excitation maximum is defined
                        by 1.0/(2.0*d3)
	ex_max    - excitation maximum from XMTR (=1/(2*d3))
        gt2       - watergate diffusion-encoding pulse width
        gzlvl2    - watergate encoding pulse strength
        gstab     - gradient stabilization delay (~0.0002-0.0003 sec)
        satmode   - 'y' turns on presaturationa during d1 and/or del
        prg_flg   - 'y' selects purging trim pulse 
                    'n' omits purging pulse
        prgtime   - purging pulse length (~0.002 sec)
        prgpwr    - purging pulse power
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

#include <standard.h>
#include <chempack.h>

void pulsesequence()
{
double	del    = getval("del"),
        gstab  = getval("gstab"),
	gt1    = getval("gt1"),
	gzlvl1 = getval("gzlvl1"),
        gt2    = getval("gt2"),
        gzlvl2 = getval("gzlvl2"),
        satpwr = getval("satpwr"),
        satdly = getval("satdly"),
        prgtime = getval("prgtime"),
        prgpwr = getval("prgpwr"),
        gzlvlhs   = getval("gzlvlhs"),
        hsgt     = getval("hsgt"),
        d3 = getval("d3"),
	Dtau,Ddelta,dosytimecubed, dosyfrq;
char	delflag[MAXSTR],satmode[MAXSTR],prg_flg[MAXSTR],alt_grd[MAXSTR],
        sspul[MAXSTR],lkgate_flg[MAXSTR];

   getstr("delflag",delflag);
   getstr("satmode",satmode);
   getstr("prg_flg",prg_flg);
   getstr("alt_grd",alt_grd);
   getstr("lkgate_flg",lkgate_flg);
   getstr("sspul",sspul);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",0.5);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",0.5);
          gt2 = syncGradTime("gt2","gzlvl2",1.0);
          gzlvl2 = syncGradLvl("gt2","gzlvl2",1.0);

   /* In pulse sequence, minimum del=4.0*pw+3*rof1+gt1+2.0*gstab	*/
   if (del < (4*pw+3.0*rof1+gt1+2.0*gstab))
   {  abort_message("Dbppste error: 'del' is less than %g, too short!",
		(4.0*pw+3*rof1+gt1+2.0*gstab));
   }

   Ddelta=gt1;
   Dtau=2.0*pw+rof1+gstab+gt1/2.0;
   dosyfrq = sfrq;
   dosytimecubed=Ddelta*Ddelta*(del-(Ddelta/3.0)-(Dtau/2.0));
   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);


   /* phase cycling calculation */
   mod2(ct,v1); dbl(v1,v1);    hlv(ct,v2);
   mod2(v2,v3); dbl(v3,v3);    hlv(v2,v2);
   mod2(v2,v4); add(v1,v4,v1);			/*   v1    */
   hlv(v2,v2);  add(v2,v3,v4);			/*   v4    */
   hlv(v2,v2);  mod2(v2,v3);   dbl(v3,v5);
   hlv(v2,v2);  mod2(v2,v3);   dbl(v3,v3);	/*   v3    */
   hlv(v2,v2);  mod2(v2,v6);   add(v5,v6,v5);	/*   v5    */
   hlv(v2,v2);  mod2(v2,v2);   dbl(v2,v2);	/*   v2    */

   assign(v1,oph);  dbl(v2,v6);      sub(oph,v6,oph);
   add(v3,oph,oph); sub(oph,v4,oph); dbl(v5,v6);
   add(v6,oph,oph); mod4(oph,oph);		/*receiver phase*/
   add(v1,v3,v7); add(v4,v7,v7);
   add(two,v7,v8);

   mod2(ct,v10);        /* gradients change sign at odd transients */

   if (ni > 1.0)
   {  abort_message("Dbppste is a 2D, not a 3D dosy sequence:  please set ni to 0 or 1");
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
       if (d1 - satdly > 0)
         delay(d1 - satdly);
         obspower(satpwr);
         txphase(zero);
         rgpulse(satdly,zero,rof1,rof1);
         obspower(tpwr);
      }
      else
      delay(d1); 
   status(B);
   /* first part of bppdel sequence */
   if (delflag[0]=='y')
   {  if (gt1>0 && gzlvl1>0)
      {  rgpulse(pw, v1, rof1, 0.0);		/* first 90, v1 */

         if (lkgate_flg[0] == 'y') lk_hold();   /* turn lock sampling off */
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1/2.0);
            elsenz(v10);
              zgradpulse(-1.0*gzlvl1,gt1/2.0);
            endif(v10);
         }
         else 
            zgradpulse(gzlvl1,gt1/2.0);
   	 delay(gstab);
	 rgpulse(pw*2.0, v2, rof1, 0.0);	/* first 180, v2 */
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl1,gt1/2.0);
            elsenz(v10);
              zgradpulse(gzlvl1,gt1/2.0);
            endif(v10);
         }
         else
            zgradpulse(-1.0*gzlvl1,gt1/2.0);
   	 delay(gstab);
   	 rgpulse(pw, v3, rof1, 0.0);		/* second 90, v3 */

       if (satmode[1] == 'y')
        {
         obspower(satpwr);
         rgpulse(del-4.0*pw-3.0*rof1-gt1-2.0*gstab,zero,rof1,rof1);
         obspower(tpwr);
        }
       else
   	{
         delay(del-4.0*pw-3.0*rof1-gt1-2.0*gstab);/*diffusion delay */
        }
         rgpulse(pw, v4, rof1, 0.0);            /* third 90, v4 */

         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1/2.0);
            elsenz(v10);
              zgradpulse(-1.0*gzlvl1,gt1/2.0);
            endif(v10);
         }
         else
            zgradpulse(gzlvl1,gt1/2.0);
   	 delay(gstab);
  	 rgpulse(pw*2.0, v5, rof1, rof1);	/* second 180, v5 */
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl1,gt1/2.0);
            elsenz(v10);
              zgradpulse(gzlvl1,gt1/2.0);
            endif(v10);
         }
         else
            zgradpulse(-1.0*gzlvl1,gt1/2.0);
   	 delay(gstab);
           if (prg_flg[0] == 'y')
                     { 
                         add(one,v7,v9);
                         obspower(prgpwr);
                         rgpulse(prgtime, v9, rof1, rof1);
                         obspower(tpwr);
                     }

        }
           zgradpulse(gzlvl2,gt2);
           delay(gstab);
            rgpulse(pw*0.231,v7,rof1,rof1);
            delay(d3);
            rgpulse(pw*0.692,v7,rof1,rof1);
            delay(d3);
            rgpulse(pw*1.462,v7,rof1,rof1);
            delay(d3);
            rgpulse(pw*1.462,v8,rof1,rof1);
            delay(d3);
            rgpulse(pw*0.692,v8,rof1,rof1);
            delay(d3);
            rgpulse(pw*0.231,v8,rof1,rof1);
          zgradpulse(gzlvl2,gt2);
          delay(gstab);
          if (lkgate_flg[0] == 'y') lk_sample();     /* turn lock on */
   }
   else
      rgpulse(pw,oph,rof1,rof2);

   /* --- observe period --- */

   status(C);
}

/****************************************************************************
256 (or 16,32,64,128) steps phase cycle
Order of cycling: v1 (0,2), v4(0,2), v1(1,3), v4(1,3), v5(0,2), v3(0,2),
v5(1,3) v2(0,2)
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
