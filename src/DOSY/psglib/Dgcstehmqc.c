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
/**********************************************************
Dgcstehmqc.c - Diffusion ordered - Pulsed Field Gradient HMQC
                    AV version: uses four gradients - all set separately

Ref: H. Barjat, G. A. Morris and A. Swanson: JMR, 131, 131-138 (1998)

Parameters:
	del       - the actual diffusion delay
	gt1	  - total diffusion encoding pulse width
	gzlvl1	  - diffusion encoding pulse strength
	gtE	  - coherence pathway selection gradient length in HMQC
	gzlvlE	  - gradient power for gtE
	gstab	  - gradient stabilization time (~0.2-0.3 ms)
        sspul     - flag for a GRD-90-GRD homospoil block
        gzlvlhs   - gradient level for sspul
        hsgt      - gradient duration for sspul
        alt_grd   - flag to invert gradient sign on alternating scans
                        (default = 'n')
       lkgate_flg - flag to gate the lock sampling off during    
                              gradient pulses
	pwx	  - 90 deg. X-pulse
	pwxlvl	  - power level for pwx
	j1xh	  - one-bond H-X coupling constant
        jnxh      - multiple-bond H-X coupling constant (for mbond='y')
	mbond	  - flag to select multiple-bond correlations (HMBC)
	c180	  - flag to make the 180 deg. X-pulse a composite pulse
        satmode   - presaturation flag
                        'yn' activates presatr during satdly
                        'yy' activates presat during satdly and del
        satfrq    - saturation frequency
        satdly    - saturation delay
        satpwr    - saturation power
        wet       - flag for optional wet solvent suppression

   for phase=1 (N-type data)  process with wft2d('t2dc')

**********************************************************/

#include <standard.h>
#include <chempack.h>

void pulsesequence()
{
double 	jtau,dosytimecubed,Ddelta,taumb,delcor,delcor2,
        j1xh    = getval("j1xh"),
        jnxh    = getval("jnxh"),
        pwx     = getval("pwx"),
        pwxlvl  = getval("pwxlvl"),
   	gzlvl1  = getval("gzlvl1"),
	gt1     = getval("gt1"),
	gzlvlE  = getval("gzlvlE"),
	gtE     = getval("gtE"),
        EDratio = getval("EDratio"),
	gzlvl3  = getval("gzlvl3"),
	gt3     = getval("gt3"),
	del     = getval("del"),
	gstab   = getval("gstab"),
        gzlvlhs = getval("gzlvlhs"),
	hsgt    = getval("hsgt"),
        satdly  = getval("satdly"),
        satpwr  = getval("satpwr"),
        satfrq  = getval("satfrq"),
        dosyfrq = sfrq;
char	sspul[MAXSTR],lkgate_flg[MAXSTR],
        mbond[MAXSTR],c180[MAXSTR],alt_grd[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gt1 = syncGradTime("gt1","gzlvl1",1.0);
        gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

   getstr("mbond", mbond);
   getstr("c180", c180);
   getstr("sspul",sspul);
   getstr("satmode",satmode);
   getstr("alt_grd",alt_grd);
   getstr("lkgate_flg",lkgate_flg);

   if (j1xh>0)
     jtau=1/(2.0*j1xh);
   else
     jtau=1/(2.0*140.0);
   if (jnxh>0)
     taumb=1/(2.0*jnxh);
   else
     taumb=1/(2.0*8.0);  


         
   Ddelta=gt1;     /*the diffusion-encoding pulse width is gt1*/
   dosytimecubed=Ddelta*Ddelta*(del - (Ddelta/3.0));
   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);

   if (jtau < (gt3+gstab)) 
   {  text_error("j1xh must be less than 1/(2*(gt3)+gstab)\n");
      psg_abort(1); 
   }

   /* Phase cycle */
   /* The phase cycle favours the following coherence pathway :
   |90H| (-1 H;0 C) |90H| (0 H;0 C) |180C;90H| (+1 H;0 C) |90C| (+1 H;+1 C) |180H| (-1 H;+1 C) |90C| (-1 H;0 C)
   */
   mod2(ct,v1);
   dbl(v1,v1);
   assign(v1,v5);	/* 0 2, first carbon 90 */
   hlv(ct,v3);
   hlv(v3,v8);
   mod2(v3,v2);
   mod2(v8,v8);
   dbl(v2,v2);
   add(v2,v8,v2);	/* 0 0 2 2 1 1 3 3, 2nd proton 90 */
   hlv(v3,v6);
   hlv(v6,v6);
   mod2(v6,v6);
   dbl(v6,v6);		/* (0)8 (2)8, proton 180 */
   assign(zero,v1);
   assign(zero,v3);
   assign(one,v9);
   add(v1,v9,v9);
   mod4(v9,v9);
   assign(zero,v7);
   assign(zero,v4);
   assign(zero,v8);

   assign(v5,oph);
   sub(oph,v2,oph);
   dbl(v6,v11);
   add(oph,v11,oph);
   sub(oph,v7,oph);
   mod4(oph,oph);

   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
   add(v14, v5, v5);
   add(v14, oph, oph);

   mod2(ct,v10);        /* gradients change sign at odd transients */

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
   decpower(pwxlvl);

   rgpulse(pw,v1,rof1,0.0);		/* 1st H 90 */
         if (lkgate_flg[0] == 'y') lk_hold();   /* turn lock sampling off */
         if (alt_grd[0] == 'y')           /* defoc. diff. gradient */
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(-1.0*gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1);
   delay(jtau/2.0-gt1-pw-rof1);
   rgpulse(pw,v2,rof1,0.0);		/* 2nd H 90 */
	
         if (alt_grd[0] == 'y')          /* 1st compensating gradient */ 
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1,gt1);
   delay(gstab);
   delcor=del-pw-2.0*rof1-4.0*pwx-4.0e-6-3.0*(gt1+gstab);
   delcor2=del-pw-2.0*rof1-2.0*pwx-3.0*(gt1+gstab);
   if (c180[0] == 'y')
    if (satmode[1] == 'y')
     {
        obspower(satpwr);
        if (satfrq != tof) obsoffset(satfrq);
        rgpulse(delcor,zero,rof1,rof1);
        if (satfrq != tof) obsoffset(tof);
        obspower(tpwr);
     }
    else delay(delcor);
   else
    if (satmode[1] == 'y')
     {
        obspower(satpwr);
        if (satfrq != tof) obsoffset(satfrq);
        rgpulse(delcor2,zero,rof1,rof1);
        if (satfrq != tof) obsoffset(tof);
        obspower(tpwr);
     }
    else delay(delcor2);
         if (alt_grd[0] == 'y')          /* 2nd compensating gradient */
         {  ifzero(v10);
              zgradpulse(-1.0*gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(-1.0*gzlvl1,gt1);
   delay(gstab);
					/* 3rd 1H 90 + 13C 180 */
   if (c180[0] == 'y')
   {
      decrgpulse(pwx,v4,rof1,0.0); 
      simpulse(pw, 2.0 * pwx, v3, v9, 2.0e-6, 0.0); /* Composite 180 C pulse */
      decrgpulse(pwx,v4,2.0e-6,0.0);
   }
   else
      simpulse(pw, 2.0 * pwx, v3, v4, rof1, 0.0);
         if (alt_grd[0] == 'y')           /* refoc. diff. gradient */
         {  ifzero(v10);
              zgradpulse(gzlvl1,gt1);
            elsenz(v10);
              zgradpulse(-1.0*gzlvl1,gt1);
            endif(v10);
         }
         else zgradpulse(gzlvl1,gt1);
   if (c180[0] == 'y')
      delay(jtau/2.0-gt1-(2*pwx+rof1));
   else
      delay(jtau/2.0-gt1-(pwx+rof1));
   if (mbond[0] == 'y')			/* one-bond J(CH)-filter */
   {  decrgpulse(pwx, v8, rof1, 0.0);
      delay(taumb - jtau - rof1 - pwx);
   }

   decrgpulse(pwx, v5,rof1,rof1);	/* C 90 */
   delay(d2/2);
         if (alt_grd[0] == 'y')         
         {  ifzero(v10);
              zgradpulse(gzlvlE,gtE);
            elsenz(v10);
              zgradpulse(-1.0*gzlvlE,gtE);
            endif(v10);
         }
         else zgradpulse(gzlvlE,gtE);
   delay(gstab);
   rgpulse(pw*2.0, v6,rof1,rof1);	/* H 180 */
         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(gzlvlE,gtE);
            elsenz(v10);
              zgradpulse(-1.0*gzlvlE,gtE);
            endif(v10);
         }
         else zgradpulse(gzlvlE,gtE);
   delay(gstab); 
   delay(d2/2);
   decrgpulse(pwx, v7,rof1,rof2);	/* C 90 */

         if (alt_grd[0] == 'y')
         {  ifzero(v10);
              zgradpulse(2.0*gzlvlE/EDratio,gtE);
            elsenz(v10);
              zgradpulse(-1.0*2.0*gzlvlE/EDratio,gtE);
            endif(v10);
         }
         else zgradpulse(2.0*gzlvlE/EDratio,gtE);
   decpower(dpwr);
   if (mbond[0]=='n')  delay(jtau-gtE);
   else delay(gstab);
   if (lkgate_flg[0] == 'y') lk_sample();     /* turn lock on sampling */
 
   status(C);
} 

