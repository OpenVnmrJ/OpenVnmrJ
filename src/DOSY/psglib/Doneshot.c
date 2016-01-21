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
	Doneshot - Oneshot DOSY sequence based on modifications to the
	Bipolar pulse stimulated echo (BPPSTE) sequence

Ref: M. D. Pelta, G. A. Morris, M. J. Tschedroff and S. J. Hammond:
	MRC 40, 147-152 (2002)
  For eliminating radiation damping: 
	M. A. Conell, A. L. Davis, A. M. Kenwright and G. A. Morris:
	Anal. Bioanal. Chem. 378, 1568-1573, (2004).

Parameters:
        delflag   - 'y' runs the Doneshot sequence
                    'n' runs the normal s2pul sequence
	del       -  the actual diffusion delay
	gt1       - total diffusion-encoding pulse width
        gzlvl1    - diffusion-encoding pulse strength
        gstab     - gradient stabilization delay (~0.0002-0.0003 sec)
        gt3       - spoiling gradient duration (in sec)
        gzlvl3    - spoiling gradient strength (destroys transverse 
			magnetization during the diffusion delay)
        gzlvl_max - maximum accepted gradient strength
                       32767 with PerformaII, 2047 with PerformaI
        kappa     - unbalancing factor between bipolar pulses as a
                       proportion of gradient strength (~0.2)
	startflip - flip angle of the firts pulse to eliminate
			radiation damping for very concentrated samples
        alt_grd   - flag to invert gradient sign on alternating scans
                        (default = 'n')
       lkgate_flg - flag to gate the lock sampling off during    
                              gradient pulses
        sspul     - flag for a GRD-90-GRD homospoil block
        gzlvlhs   - gradient level for sspul
        hsgt      - gradient duration for sspul
        satmode   - flag for optional solvent presaturation
                    'yn' - does presat during satdly
                    'yy' - does presat during satdly and the diffusion delay
        satdly    - presaturation delay before the sequence (part of d1)
        satpwr    - saturation power level
        satfrq    - saturation frequency
        wet       - flag for optional wet solvent suppression
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

The parameters for the heating gradients (gt4, gzlvl4) are calculated
in the sequence. They cannot be set directly.
tau defined as time between the mid-points of the bipolar diffusion encoding
gradient pulses

**********************************************************/

#include <standard.h>
#include <chempack.h>

pulsesequence()
{
double	delcor,initdel,gzlvl4,gt4,Dtau,Ddelta,dosytimecubed,dosyfrq,
        kappa     = getval("kappa"), 
	gzlvl1    = getval("gzlvl1"),
	gzlvl3    = getval("gzlvl3"),
        gzlvl_max = getval("gzlvl_max"),
	gt1       = getval("gt1"),
	gt3       = getval("gt3"),
	del       = getval("del"),
        gstab     = getval("gstab"),
        satpwr    = getval("satpwr"),
        satdly    = getval("satdly"),
        satfrq    = getval("satfrq"),
        gzlvlhs   = getval("gzlvlhs"),
        hsgt     = getval("hsgt"),
        startflip = getval("startflip");
char delflag[MAXSTR], sspul[MAXSTR],
	satmode[MAXSTR],alt_grd[MAXSTR],lkgate_flg[MAXSTR],oneshot45_flg[MAXSTR];

   gt4 = 2.0*gt1;
   getstr("delflag",delflag);
   getstr("alt_grd",alt_grd);
   getstr("oneshot45_flg",oneshot45_flg);
   getstr("satmode",satmode);
   getstr("lkgate_flg",lkgate_flg);
   getstr("sspul",sspul);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",0.5);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",0.5);

/* Decrement gzlvl4 as gzlvl1 is incremented, to ensure constant 
   energy dissipation in the gradient coil 
   Current through the gradient coil is proportional to gzlvl */

   gzlvl4 = sqrt(2.0*gt1*(1+3.0*kappa*kappa)/gt4) * 
            sqrt(gzlvl_max*gzlvl_max/((1+kappa)*(1+kappa))-gzlvl1*gzlvl1);

/* In pulse sequence, del>4.0*pw+3*rof1+2.0*gt1+5.0*gstab+gt3 */

   if ((del-(4*pw+3.0*rof1+2.0*gt1+5.0*gstab+gt3)) < 0.0)
   {  del=(4*pw+3.0*rof1+2.0*gt1+5.0*gstab+gt3);
      text_message("Warning: del too short; reset to minimum value");
   }

   if ((d1 - (gt3+gstab) -2.0*(gt4/2.0+gstab)) < 0.0)
   {  d1 = (gt3+gstab) -2.0*(gt4/2.0+gstab);
      text_message("Warning: d1 too short;  reset to minimum value");
   }

   if ((gzlvl1*(1+kappa)) > gzlvl_max)
   {  abort_message("Max. grad. amplitude exceeded: reduce either gzlvl1 or kappa");
   }

   if (ni > 1.0)
   {  abort_message("This is a 2D, not a 3D dosy sequence: please set ni to 0 or 1");
   }

   if (delflag[0]=='y')
     { initdel=(gt3+gstab) -2.0*(gt4/2.0+gstab); }
   else
     { initdel=0; }

   Ddelta=gt1;
   Dtau=2.0*pw+gstab+rof1;
   dosyfrq = sfrq;
   dosytimecubed = Ddelta*Ddelta*(del+(Ddelta/6.0) *
                   (kappa*kappa-2.0) +
                   (Dtau/2.0)*(kappa*kappa-1.0));
   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);

/* phase cycling calculation */

   if(delflag[0]=='y')
   {
	 mod2(ct,v1); dbl(v1,v1);  hlv(ct,v2);
	 mod2(v2,v3); dbl(v3,v3);  hlv(v2,v2);
	 mod2(v2,v4); add(v1,v4,v1);			/*    v1      */
	 hlv(v2,v2);  add(v2,v3,v4);			/*    v4      */
	 hlv(v2,v2);  mod2(v2,v3); dbl(v3,v5);
	 hlv(v2,v2);  mod2(v2,v3); dbl(v3,v3);		/*    v3      */
	 hlv(v2,v2);  mod2(v2,v6); add(v5,v6,v5);	/*    v5      */
	 hlv(v2,v2);  mod2(v2,v2); dbl(v2,v2);		/*    v2      */
	 assign(v1,oph);  dbl(v2,v6);      sub(oph,v6,oph);
	 add(v3,oph,oph); sub(oph,v4,oph); dbl(v5,v6);
	 add(v6,oph,oph); mod4(oph,oph);                /* receiver phase */
	 mod2(ct,v6); dbl(v6,v6); /* 02 */
	 hlv(ct,v8); hlv(v8,v8); dbl(v8,v8); mod4(v8,v8); /* 00002222 */
	 add(oph,v8,v8);
	 add(v8,v6,v6);
	 add(v6,one,v6); /*Optional 45-degree pulse before acquisition- used when oneshot45_flg='y'*/

   }
      mod2(ct,v7);     /* gradients change sign with every scan */

   status(A);

   if(delflag[0]=='y')
   {  zgradpulse(-1.0*gzlvl4,gt4/2.0);	/* 1st dummy heating pulse */
      delay(gstab);

      zgradpulse(gzlvl4,gt4/2.0);	/* 2nd dummy heating pulse */
      delay(gstab);
   }

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
       else
       delay(0.02);
       obspower(satpwr);
        if (satfrq != tof)
         obsoffset(satfrq);
        rgpulse(satdly,zero,rof1,rof1);
        if (satfrq != tof)
         obsoffset(tof);
       obspower(tpwr);
       delay(1.0e-5);
      }
     else
     {  delay(d1-initdel); }

 if (getflag("wet")) wet4(zero,one);

 if (delflag[0]=='y')
    {
        if (lkgate_flg[0] == 'y')  lk_hold(); /* turn lock sampling off */
        if (alt_grd[0] == 'y')
         {
          ifzero(v7); zgradpulse(-1.0*gzlvl3,gt3);  /* Spoiler gradient balancing pulse */
 	  elsenz(v7); zgradpulse(1.0*gzlvl3,gt3);
          endif(v7);
         }
	else zgradpulse(-1.0*gzlvl3,gt3);
        delay(gstab);
    }
   status(B); /* first part of sequence */
   if (delflag[0]=='y')
   {
      if (gt1>0 && gzlvl1>0) 
      {
   	 rgpulse(startflip*pw/90.0, v1, rof1, 0.0);		/* first 90, v1 */

	if (alt_grd[0] == 'y')
	{ifzero(v7); zgradpulse(gzlvl1*(1.0-kappa),gt1/2.0); /*1st main gradient pulse*/
         elsenz(v7); zgradpulse(-1.0*gzlvl1*(1.0-kappa),gt1/2.0);
         endif(v7);
        }
	else zgradpulse(gzlvl1*(1.0-kappa),gt1/2.0);
   	 delay(gstab);
	 rgpulse(pw*2.0, v2, rof1, 0.0);	/* first 180, v2 */

        if (alt_grd[0] == 'y')
        {ifzero(v7); zgradpulse(-1.0*gzlvl1*(1.0+kappa),gt1/2.0); /*2nd main grad. pulse*/
         elsenz(v7); zgradpulse(1.0*gzlvl1*(1.0+kappa),gt1/2.0);
         endif(v7);
        }
	else zgradpulse(-1.0*gzlvl1*(1.0+kappa),gt1/2.0);
   	 delay(gstab);
   	 rgpulse(pw, v3, rof1, 0.0);		/* second 90, v3 */

        if (alt_grd[0] == 'y')
        {ifzero(v7); zgradpulse(gzlvl1*2.0*kappa,gt1/2.0);  /* Lock refocussing pulse*/
         elsenz(v7); zgradpulse(-1.0*gzlvl1*2.0*kappa,gt1/2.0);
         endif(v7);
        }
        else zgradpulse(gzlvl1*2.0*kappa,gt1/2.0);
   	 delay(gstab);

        if (alt_grd[0] == 'y')
        {ifzero(v7); zgradpulse(gzlvl3,gt3); /* Spoiler gradient balancing pulse */
         elsenz(v7); zgradpulse(-1.0*gzlvl3,gt3);
         endif(v7);
        }
        else zgradpulse(gzlvl3,gt3);
   	 delay(gstab);

         delcor = del-4.0*pw-3.0*rof1-2.0*gt1-5.0*gstab-gt3;
         if (satmode[1] == 'y')
           {
             obspower(satpwr);
             if (satfrq != tof)
             obsoffset(satfrq);
             rgpulse(delcor,zero,rof1,rof1);
             if (satfrq != tof)
             obsoffset(tof);
             obspower(tpwr);
           }
	 else delay(del-4.0*pw-3.0*rof1-2.0*gt1-5.0*gstab-gt3); /* diffusion delay */

        if (alt_grd[0] == 'y')
        {ifzero(v7); zgradpulse(2.0*kappa*gzlvl1,gt1/2.0);  /*Lock refocussing pulse*/
         elsenz(v7); zgradpulse(-2.0*kappa*gzlvl1,gt1/2.0);
         endif(v7);
        }
        else zgradpulse(2.0*kappa*gzlvl1,gt1/2.0);
   	 delay(gstab);
   	 rgpulse(pw, v4, rof1, 0.0);		/* third 90, v4 */

        if (alt_grd[0] == 'y')
        {ifzero(v7); zgradpulse(gzlvl1*(1.0-kappa),gt1/2.0); /*3rd main gradient pulse*/
         elsenz(v7); zgradpulse(-1.0*gzlvl1*(1.0-kappa),gt1/2.0);
         endif(v7);
        }
        else zgradpulse(gzlvl1*(1.0-kappa),gt1/2.0);
   	 delay(gstab);
	 rgpulse(pw*2.0, v5, rof1, rof1);	/* second 180, v5 */

        if (alt_grd[0] == 'y')
        {ifzero(v7); zgradpulse(-1.0*(1.0+kappa)*gzlvl1,gt1/2.0); /*4th main grad. pulse*/
         elsenz(v7); zgradpulse(1.0*(1.0+kappa)*gzlvl1,gt1/2.0);
         endif(v7);
        }
        else zgradpulse(-1.0*(1.0+kappa)*gzlvl1,gt1/2.0);
	if (oneshot45_flg[0] == 'y')
	{
   	 delay(gstab);
	 rgpulse(0.5*pw,v6,rof1,rof2);	/* 45-degree pulse, orthogonal to the last 90 */
	}
	else
   	 delay(gstab+2*pw/PI);
      }
      if (lkgate_flg[0] == 'y') lk_sample(); /* turn lock sampling on */
   }
   else
      rgpulse(pw,oph,rof1,rof2);
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

+1                ________                _______
                 |        |              |       |
 0------|        |        |--------------|       |
        |        |                               |
-1      |________|                               |____________________

-2
	v1       v2        v3             v4      v5
****************************************************************************/
