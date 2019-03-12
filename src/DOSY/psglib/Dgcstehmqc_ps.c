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
     Dgcstehmqc_ps.c - Diffusion ordered - HMQC
        phase sensitive version using States-Haberkorn (phase=1,2) 
        acqisition. No gradient selection is used for the HMQC step
        Use it for one-bond X-H correlations

Parameters:
	del       - the actual diffusion delay
	gt1	  - total diffusion encoding pulse width
	gzlvl1	  - diffusion encoding pulse strength
        gstab     - gradient stabilizarion delay
	pwx	  - 90 deg. X-pulse
	pwxlvl	  - power level for pwx
	j1xh	  - one-bond H-X coupling constant
	c180	  - flag to make the 180 deg. X-pulse a composite pulse
        satmode   - flag for optional solvent presaturation
                    'ynn' - does presat during satdly
                    'yyn' - does presat during satdly and the diffusion delay
        satdly    - presaturation delay before the sequence (part of d1)
        satpwr    - saturation power level
        satfrq    - saturation frequency
        wet       - flag for optional wet solvent suppression
        alt_grd   - alternate gradient sign(s) for odd scans
       lkgate_flg - flag to gate the lock sampling off during 
                              gradient pulses
        sspul     - flag for a GRD-90-GRD homospoil block
        gzlvlhs   - gradient level for sspul
        hsgt      - gradient duration for sspul
        phase     - 1,2 for States-Haberkorn acquisition

process it by : wft2da for phase=1,2

Run the phase sensive 2D HMQC spectra in the same experiment.
Set the array as follows:
	array='gzlvl1,phase'
The data can be processed with the optional DOSY package using the
dosy macro.
**********************************************************/

#include <standard.h>
#include <chempack.h>

static int      ph1[4] = {0,0,2,2},
                ph2[4] = {0,0,0,0},
                ph4[4] = {0,0,0,0},
                ph5[4] = {0,2,0,2},
                ph9[4] = {1,1,3,3},
                rec[4] = {0,2,2,0};


void pulsesequence()
{
int     phase;
double 	j1xh    = getval("j1xh"),
	gzlvl1  = getval("gzlvl1"),
	gt1     = getval("gt1"),
	del     = getval("del"),
	gstab   = getval("gstab"),
        satdly = getval("satdly"),
        satpwr = getval("satpwr"),
        satfrq = getval("satfrq"),
        gzlvlhs   = getval("gzlvlhs"),
        hsgt     = getval("hsgt"),
	dosyfrq = sfrq,
        arraydim = getval("arraydim"),
	jtau,dosytimecubed,Ddelta;
char	c180[MAXSTR],satmode[MAXSTR],alt_grd[MAXSTR],
        lkgate_flg[MAXSTR],sspul[MAXSTR],arraystring[MAXSTR];

   if (rof1 > 2.0e-6) rof1=2.0e-6;
   pwx = getval("pwx");
   pwxlvl = getval("pwxlvl");
   getstr("c180", c180);
   getstr("satmode", satmode);
   getstr("alt_grd",alt_grd);
   getstr("sspul",sspul);
   getstr("array",arraystring);
   getstr("lkgate_flg",lkgate_flg);
   phase = (int) (getval("phase") + 0.5);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",1.0);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);

   if (strcmp(arraystring,"gzlvl1,phase")!=0)
          fprintf(stdout,"Warning:  array should be 'gzlvl1,phase' for this experiment");
   if (j1xh>0)
     jtau=1/(2.0*j1xh);
   else
      jtau=0.5/140.0;
   Ddelta=gt1;     /*the diffusion-encoding pulse width is gt1*/
   dosytimecubed=Ddelta*Ddelta*(del - (Ddelta/3.0));
   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);
   if (ni>1) putCmd("dosy3Dproc=\'y\'\n");
   else      putCmd("dosy3Dproc=\'n\'\n");

   /* Phase cycle */
   /* The phase cycle favours the following coherence pathway :
   |90H| (-1 H;0 C) |90H| (0 H;0 C) |180C;90H| (+1 H;0 C) |90C| (+1 H;+1 C) |180H| (-1 H;+1 C) |90C| (-1 H;0 C)
   */
        settable(t1, 4, ph1);
        settable(t2, 4, ph2);
        settable(t4, 4, ph4);
        settable(t5, 4, ph5);
        settable(t9, 4, ph9);
        settable(t6, 4, rec);
        sub(ct,ssctr,v11);    /* set up steady-state phase cycling */
        getelem(t1,v11,v1);
        getelem(t2,v11,v2);
        getelem(t4,v11,v4);
        getelem(t5,v11,v5);
        getelem(t9,v11,v9);
        getelem(t6,v11,oph);

   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
        add(v4, v14, v4);
        add(v9, v14, v9);
        add(v5, v14, v5);
        add(oph,v14,oph);

   if (phase == 2)
    { incr(v5); }

   if (phase == 3)
    { initval((double) ((int)((ix - 1) / (arraydim / ni) +1e-6)), v12);
      add(v5,v12,v5); 
    }

        mod2(ct,v10);   /* alternate gradient sign on odd scans */
   status(A);
   if (sspul[0] == 'y')
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
    if (alt_grd[0] == 'y')
      {
       ifzero(v10); zgradpulse(gzlvl1,gt1); /* defoc. diff. gradient */
              elsenz(v10); zgradpulse(-gzlvl1,gt1); endif(v10);
      }
    else zgradpulse(gzlvl1,gt1);
   delay(jtau/2.0-gt1-pw-rof1);
   rgpulse(pw,v2,rof1,0.0);		/* 2nd H 90 */
    if (alt_grd[0] == 'y')
      {
       ifzero(v10); zgradpulse(-gzlvl1,gt1); /* 1st compensating gradient */
              elsenz(v10); zgradpulse(gzlvl1,gt1); endif(v10);
      }
    else  zgradpulse(-gzlvl1,gt1);
   delay(gstab);
   if (c180[0] == 'y')
    {
     if (satmode[1] == 'y')
      {
         obspower(satpwr);
         if (satfrq != tof) obsoffset(satfrq);
         rgpulse(del-pw-2.0*rof1-4.0*pwx-4.0e-6-3.0*(gt1+gstab),zero,rof1,rof1);
         if (satfrq != tof) obsoffset(tof);
         obspower(tpwr);
      }
      else 
      delay(del-pw-2.0*rof1-4.0*pwx-4.0e-6-3.0*(gt1+gstab));
    }
   else
     {
     if (satmode[1] == 'y')
        {
         obspower(satpwr);
         if (satfrq != tof) obsoffset(satfrq);
           rgpulse(del-pw-2.0*rof1-2.0*pwx-3.0*(gt1+gstab),zero,rof1,rof1);
           if (satfrq != tof) obsoffset(tof);
           obspower(tpwr);
        }
        else 
        delay(del-pw-2.0*rof1-2.0*pwx-3.0*(gt1+gstab));
      }
    if (alt_grd[0] == 'y')
      {
       ifzero(v10); zgradpulse(-gzlvl1,gt1); /* 2nd compensating gradient */ 
              elsenz(v10); zgradpulse(gzlvl1,gt1); endif(v10);
      }
    else zgradpulse(-gzlvl1,gt1);
   delay(gstab);
					/* 3rd 1H 90 + 13C 180 */
   if (c180[0] == 'y')
   {
      decrgpulse(pwx,v4,rof1,0.0); 
      simpulse(pw, 2.0 * pwx, v2, v9, 2.0e-6, 0.0); /* Composite 180 X pulse */
      decrgpulse(pwx,v4,2.0e-6,0.0);
   }
   else
      simpulse(pw, 2.0 * pwx, v2, v4, rof1, 0.0);
    if (alt_grd[0] == 'y')
      {
       ifzero(v10); zgradpulse(gzlvl1,gt1);  /* refoc. diff. gradient */
              elsenz(v10); zgradpulse(-gzlvl1,gt1); endif(v10);
      }
    else zgradpulse(gzlvl1,gt1);
   if (c180[0] == 'y')
      delay(jtau/2.0-gt1-(2*pwx+rof1)-2.0*pw/3.1416);
   else
      delay(jtau/2.0-gt1-(pwx+rof1)-2.0*pw/3.1416);
   if (lkgate_flg[0] == 'y') lk_sample();     /* turn lock sampling on */
   decrgpulse(pwx, v5,rof1,rof1);	/* X 90 */
   if (d2/2 > pw + 2*rof1 + 2*pwx/3.1416) 
      delay(d2/2.0 - pw - 2*rof1 - 2*pwx/3.1416);
   else delay(0.0);
      rgpulse(pw*2.0, v6,rof1,rof1);	/* H 180 */
   if (d2/2 > pw + 2*rof1 + 2*pwx/3.1416) 
      delay(d2/2.0 - pw - 2*rof1 - 2*pwx/3.1416);
   else delay(0.0);
   decrgpulse(pwx, v7,rof1,rof1);	/* X 90 */

   delay((jtau)/2.0-pwx-2*rof1);
   simpulse(2*pw,2*pwx,zero,zero,rof1,rof1);
   decpower(dpwr);
   delay((jtau)/2.0-pwx-rof1); 
   delay(rof2);
   status(C);
} 

