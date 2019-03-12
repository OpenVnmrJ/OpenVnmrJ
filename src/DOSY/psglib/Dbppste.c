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
 Dbppste - Bipolar gradient pulses stimulated echo sequence
	ref : J. Magn. Reson. Ser. A, 115, 260-264 (1995).

parameters:
	delflag   - 'y' runs the Dbppste sequence
                    'n' runs the normal s2pul sequence
        del       -  the actual diffusion delay
        gt1       - total diffusion-encoding pulse width
        gzlvl1    - diffusion-encoding pulse strength
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

#include <standard.h>
#include <chempack.h>

void pulsesequence()
{
double	Dtau,Ddelta,dosytimecubed,dosyfrq,delcor,
        del    = getval("del"),
        gstab  = getval("gstab"),
	gt1    = getval("gt1"),
	gzlvl1 = getval("gzlvl1"),
	gzlvlhs   = getval("gzlvlhs"),
	hsgt     = getval("hsgt"),
        satpwr = getval("satpwr"),
        satdly = getval("satdly"),
        satfrq = getval("satfrq");
char	delflag[MAXSTR],sspul[MAXSTR],
        alt_grd[MAXSTR],satmode[MAXSTR],lkgate_flg[MAXSTR];

   getstr("delflag",delflag);
   getstr("satmode",satmode);
   getstr("alt_grd",alt_grd);
   getstr("lkgate_flg",lkgate_flg);
   getstr("sspul",sspul);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",0.5);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",0.5);

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

   /* Check for the validity of the number of transients selected ! 
   if ( (nt != 4) && ((int)nt%16 != 0) )
   {  abort_message("Dbppste error: nt must be 4, 16, 64 or n*16 (n: integer)!");
   }  */

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
   add(two,oph,oph);

   mod2(ct,v10);	/* gradients change sign at odd transients */

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

if (getflag("wet"))
     wet4(zero,one);

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
         else zgradpulse(gzlvl1,gt1/2.0);
         delay(gstab);

	 rgpulse(pw*2.0, v2, rof1, 0.0);	/* first 180, v2 */

         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl1,gt1/2.0);
            elsenz(v10);
              zgradpulse(gzlvl1,gt1/2.0);
   	    endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1,gt1/2.0);
         delay(gstab);

   	 rgpulse(pw, v3, rof1, 0.0);		/* second 90, v3 */

         delcor = del-4.0*pw-3.0*rof1-gt1-2.0*gstab;
         if (satmode[1] == 'y')
          {
           obspower(satpwr);
           if (satfrq != tof) obsoffset(satfrq);
           rgpulse(delcor,zero,rof1,rof1);
           if (satfrq != tof) obsoffset(tof);
           obspower(tpwr);
          }
         else delay(delcor);     /*diffusion delay */

   	 rgpulse(pw, v4, rof1, 0.0);		/* third 90, v4 */

         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1/2.0);
            elsenz(v10);
              zgradpulse(-1.0*gzlvl1,gt1/2.0);
   	    endif(v10);
         }
         else zgradpulse(gzlvl1,gt1/2.0);
         delay(gstab);

	 rgpulse(pw*2.0, v5, rof1, rof1);	/* second 180, v5 */

         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl1,gt1/2.0);
            elsenz(v10);
              zgradpulse(gzlvl1,gt1/2.0);
   	    endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1,gt1/2.0);
         delay(gstab+2*pw/PI);
         if (lkgate_flg[0] == 'y') lk_sample();     /* turn lock sampling on */
      }
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
