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
   Dgcstecosy.c - diffusion ordered- pulsed field gradient COSY (magnitude mode)
			with the option of double quantum filtering

Ref: JMR, 123 (Series A) 88-91 (1996).

Parameters:
        delflag - 'y' runs the Dgcstecosy sequence
                  'n' runs the normal gcosy sequence
	del     - the actual diffusion delay
	gt1	- total diffusion encoding pulse width
	gzlvl1	- diffusion encoding pulse strength
        gstab   - gradient stabilization time (~0.0002-0.0003 sec)
	tweek   - tuning factor to limit eddy currents, 
                   ( can be set from 0 to 1, usually set to 0.0 )
	gzlvl2  - gradient power for pathway selection
	gt2     - gradient duration for pathway selection
        sspul   - flag for a GRD-90-GRD homospoil block
        gzlvlhs - gradient level for sspul
        satmode - 'yn' turns on presaturation during satdly
                  'yy' turns on presaturation during satdly and del
                  the presauration happens at the transmitter position
                  (set tof right if presat option is used)
         satdly - presatutation delay (part of d1)
         satpwr - presaturation power
            wet - flag for optional wet solvent suppression
        hsgt    - gradient duration for sspul
        alt_grd - flag to invert gradient sign on alternating scans
                        (default = 'n')
     lkgate_flg - flag to gate the lock sampling off during    
                              gradient pulses
	qlvl    - quantum filter level (1=single quantum, 2=double quantum)

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

**********************************************************/

#include <standard.h>
#include <chempack.h>

void pulsesequence()
{
double	dosytimecubed,Ddelta,delcor,
        gzlvl1  = getval("gzlvl1"),
        gt1     = getval("gt1"),
        tweek   = getval("tweek"),
        gzlvl2  = getval("gzlvl2"),
        gt2     = getval("gt2"),
        del     = getval("del"),
        gstab   = getval("gstab"),
        gzlvlhs = getval("gzlvlhs"),
	hsgt    = getval("hsgt"),
        qlvl    = getval("qlvl"),
        satdly  = getval("satdly"),
        satpwr  = getval("satpwr"),
        satfrq  = getval("satfrq"),
        dosyfrq = sfrq;
char	delflag[MAXSTR],sspul[MAXSTR],alt_grd[MAXSTR],lkgate_flg[MAXSTR];
        
   getstr("sspul",sspul);
   getstr("satmode",satmode);
   getstr("delflag",delflag); 
   getstr("alt_grd",alt_grd);
   getstr("lkgate_flg",lkgate_flg);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",1.0);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);
          gt2 = syncGradTime("gt2","gzlvl2",1.0);
          gzlvl2 = syncGradLvl("gt2","gzlvl2",1.0);

   if (del < (3.0*gt1+3.0*gstab+4.0*rof1+pw*2.0))
   {  abort_message("Dgcstecosy error: 'del' is less than %f, too short!",
		(3.0*gt1+3.0*gstab+4.0*rof1+pw*2.0));
   }
			
   Ddelta=gt1;	/*the diffusion-encoding pulse width is gt1*/
   dosytimecubed=Ddelta*Ddelta*(del - (Ddelta/3.0));
   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);

   /* PHASE CYCLING CALCULATION */
   assign(zero,v1);
   assign(zero,v2);
   if (qlvl == 1)
   {  mod2(ct,v3);
      dbl(v3,v3);		/* 0 2, 3rd pulse */
      hlv(ct,v4);
      hlv(v4,v6);
      dbl(v4,v4);
      add(v4,v6,v4);
      mod4(v4,v4);		/* 0 0 2 2 1 1 3 3, 4th pulse */
   }
   else	if (qlvl == 2)
   {  mod2(ct,v3);
      dbl(v3,v3);
      hlv(ct,v4);
      add(v3,v4,v3);
      mod4(v3,v3);		/* 0 2 1 3, 3rd pulse */
      hlv(v4,v5);
      mod2(v5,v5);
      dbl(v5,v5);		/* (0)4 (2)4, 5th pulse */
      assign(zero,v4);
   }

   assign(zero,oph);
   add(oph,v1,oph);
   sub(oph,v2,oph);
   sub(oph,v3,oph);
   dbl(v4,v6);
   if (qlvl == 1)
   {  add(v6,oph,oph);
   }
   else if (qlvl == 2)
   {  add(v6,v4,v6);
      add(oph,v6,oph);
      sub(oph,v5,oph);
   }
   mod4(oph,oph);

   mod2(ct,v10);        /* gradients change sign at odd transients */

  /* FOR HYPERCOMPLEX, USE REDFIED TRICK TO MOVE AXIALS TO EDGE */
      initval(2.0*(double)(d2_index%2),v9);  /* moves axials */
      add(v1,v9,v1); add(v2,v9,v2); add(v3,v9,v3); add(oph,v9,oph);

   /* BEGIN ACTUAL PULSE SEQUENCE */
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

   if (delflag[0] == 'y') 
   { 
     rgpulse(pw, v1, rof1, rof1);        		/* first 90, v1 */
     if (lkgate_flg[0] == 'y') lk_hold();   /* turn lock sampling off */
         if (alt_grd[0] == 'y')                  /* First diff. G + CPS G */
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(-1.0*gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1);
     delay(gstab);
     rgpulse(pw, v2, rof1, rof1);      		/* second 90, v2 */
         if (alt_grd[0] == 'y')                  /* First diff. G + CPS G */
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl1*(1.0-tweek),gt1);
            elsenz(v10);
              zgradpulse(gzlvl1*(1.0-tweek),gt1);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1*(1.0-tweek),gt1);   /* Compensating +CPS G */
     delcor=del-(3.0*gt1+3.0*gstab+4.0*rof1+pw); 
     if (satmode[1] == 'y')
       {
        obspower(satpwr);
        if (satfrq != tof) obsoffset(satfrq);
        rgpulse(delcor,zero,rof1,rof1);
        if (satfrq != tof) obsoffset(tof);
        obspower(tpwr);
       }
     else delay(del-(3.0*gt1+3.0*gstab+4.0*rof1+pw));  /* diffusion delay */
         if (alt_grd[0] == 'y')                  /* First diff. G + CPS G */
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl1*(1.0+tweek),gt1);
            elsenz(v10);
              zgradpulse(gzlvl1*(1.0+tweek),gt1);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1*(1.0+tweek),gt1);     /* Compensating +CPS G */
     delay(gstab);
     rgpulse(pw, v3, rof1, rof1);            	/* third 90, v3 */
         if (alt_grd[0] == 'y')                  /* First diff. G + CPS G */
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(-1.0*gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1);           /* Refoc. diff. G + CPS G */
     delay(gstab);
   }
   else
     rgpulse(pw, oph, rof1, rof1);        		/* first 90, v1 */


/* END OF STIMULATED ECHO PART, BEGINNING OF THE GRADIENT COSY PART */

   delay(d2);

   if (qlvl == 1)
   {  /* N-type selection gradient */
         if (alt_grd[0] == 'y')                 
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl2,gt2);
            elsenz(v10);
              zgradpulse(gzlvl2,gt2);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl2,gt2); 
      delay(gstab);
      rgpulse(pw,v4,rof1,rof1);		/* fourth 90, v4 */
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl2,gt2);
            elsenz(v10);
              zgradpulse(gzlvl2,gt2);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl2,gt2);
      delay(gstab);
   }
   else if (qlvl == 2)
   {  rgpulse(pw,v4,rof1,rof1);		/* fourth 90, v4 */
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvl2,gt2);
            elsenz(v10);
              zgradpulse(-1.0*gzlvl2,gt2);
            endif(v10);
         }
         else zgradpulse(gzlvl2,gt2);
      delay(gstab);
      rgpulse(pw,v5,rof1,rof2);		/* fifth 90, v5 */
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(-2.0*gzlvl2,gt2);
            elsenz(v10);
              zgradpulse(2.0*gzlvl2,gt2);
            endif(v10);
         }
         else zgradpulse(-2.0*gzlvl2,gt2);
      delay(gstab);
   }
     if (lkgate_flg[0] == 'y') lk_sample();     /* turn lock sampling on */
   status(C);
}

/****************************************************************************
	Pathway for normal COSY:
	90	90	90	90 Acq.
	    -1      0       +1      -1

	Pathway for DQF COSY:
	90	90 	90	90	90 Acq.
	    -1      0       +1      -2      -1    
****************************************************************************/
