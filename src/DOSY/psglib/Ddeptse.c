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

/* dept SE- distortionless enhancement by polarization transfer with gradient spin echo
11dec09 AB

Attributes:

	1. composite 180 degree pulse on C13 only
	2. complete phasecycling of all pulses to eliminate
		artifacts due to off-resonance effects
	3. simultaneous pulses
	4. maximum (dhp=255) power decoupler pulses
		if dhp="value", "value" used for decoupling
		level during acquisition; for 500-like architecture
		"pplvl" governs decoupler for pulses and dpwr, for
		broadband decoupling during acquisition
	5. if dhp=n, dlp value is used for both pulses
		and observe decoupling; invalid for 500-like
		architecture
	6. presaturation of x-nuclei with 90 degree pulse
		and homospoil

Reference: A. Botana, P.W.A. Howe, V. Caer, G.A. Morris and M. Nilsson
        J. Magn. reson. 211, 25-29 (2011).

Reference for Dept:  d.m. doddrell, d.t. pegg, and m.r. bendall,
			j. magn. reson. 48:323 (1982)


Parameters:

	pp = proton 90 degree pulse length
	pplvl = power level for decoupler proton pulses (for 500);
	otherwise, dhp=255 is used by default
	pw = x 90 degree pulse length
	tpwr = power level for transmitter x pulses
	j1xh = x-h coupling constant (in hz)
	mult = proton flip angle theta is mult*pp
	satdly = optional saturation delay (ca. 0.05 sec)
	del= diffusion delay
	gt1     - total length of the phase encoding gradient
	gzlvl1  - stenght of the phase encoding gradient
	nt =    (min):  multiple of 2
		(max):  multiple of 128
		(suggested): multiple of 16

For DOSY use mult=0.5

For 13c spectral editing use mult = 0.5, 1.0, 1.0, 1.5:

				ch  subspectrum = s2 + s3
				ch2 subspectrum = s1 - s4
				ch3 subspectrum = -0.77*(s2 + s3) + s1 + s4

*/


#include <standard.h>
#include <chempack.h>

static int      ph1[2]   = {0,2},
                ph3[1]   = {1},
                ph4[32]  = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
                            2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3},
                ph6[8]   = {1,1,2,2,3,3,0,0},
                ph7[8]   = {0,0,1,1,0,0,1,1},
                ph8[1]   = {1},
                rec[32]  = {0,2,0,2,0,2,0,2,1,3,1,3,1,3,1,3,
                            2,0,2,0,2,0,2,0,3,1,3,1,3,1,3,1};

pulsesequence()
{
	/* VARIABLE DECLARATION */
double  del    = getval("del"),
        gzlvl1 = getval("gzlvl1"),
        gt1    = getval("gt1"),
        pplvl  = getval("pplvl"),
	pp     = getval("pp"),
	mult   = getval("mult"),
        gstab  = getval("gstab"),
        j1xh   = getval("j1xh"),
        hsglvl = getval("hsglvl"),
        hsgt   = getval("hsgt"),
        Dtau,Ddelta,dosytimecubed,dosyfrq,jtau,corr;
	
char	sspul[MAXSTR],sspulX[MAXSTR],lkgate_flg[MAXSTR],alt_grd[MAXSTR];
        getstr("sspul",sspul);
        getstr("sspulX",sspulX);
        getstr("alt_grd",alt_grd);
        getstr("lkgate_flg",lkgate_flg);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",0.5);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",0.5);

	jtau = 1.0/(2.0*j1xh); /* delay to establish x-h antiphase magnetization */
	Ddelta=gt1;
	dosyfrq = sfrq;
	dosytimecubed=Ddelta*Ddelta*(del-(Ddelta/3.0));
	putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);
	
	/* CHECK CONDITIONS */

        if (rof1>10.0e-6) rof1=10.0e-6;
        corr = gt1+2*rof1+4*pw;
        if (del - corr < 0)
           abort_message("Ddeptse: The diffusion delay must be longer than %f seconds",corr);

        if ((dm[A] == 'y') || (dm[B] == 'y'))
           abort_message("Ddeptse: Decoupler must be set as dm=nny or n");
   
	if (dmm[1] != 'c')
           abort_message("Ddeptse: Decoupler modulation must be set to 'c' during status B.\n");

        if (ni > 1.0)
           abort_message("Ddeptse is a 2D, not a 3D dosy sequence:  please set ni to 0");

	/* Set Phases */

        mod2(ct,v11);        /* gradients change sign on every odd scan */
        sub(ct,ssctr,v10);
        settable(t1, 2, ph1);	getelem(t1,v10,v1);
        settable(t3, 1, ph3);   getelem(t3,v10,v3);
        settable(t4,32, ph4);   getelem(t4,v10,v4);
        settable(t6, 8, ph6);   getelem(t6,v10,v6);
        settable(t7, 8, ph7);   getelem(t7,v10,v7);
        settable(t8, 1, ph8);   getelem(t8,v10,v8);
        settable(t9,32, rec);   getelem(t9,v10,oph);

	/* ACTUAL PULSESEQUENCE BEGINS */
	status(A);
	decpower(pplvl); obspower(tpwr);

   if (getflag("sspul"))
        steadystate();

        if (sspulX[0]=='y') 
            {
              delay(d1 - hsgt - 0.003);
              pulse(pw, zero);    /* destroy xy magnetization */
   	      zgradpulse(hsglvl,hsgt);
	      delay(0.003);
            }
        else delay(d1);
        if (lkgate_flg[0] == 'y') lk_hold();   /* turn lock sampling off */

	decrgpulse(pp, v1, rof1, rof1);
                /* to be corrected */
	if (pw > 2.0*pp) delay(jtau - pw/2 - 2*rof1 - 2*pp/3.14);
	else delay(jtau - pp - 2*rof1 - 2*pp/3.14);

	simpulse(pw, 2.0 * pp, v4, v3, rof1, 0.0);
                
        if ((pw > 2.0*pp)&&(mult * pp > 2.0 * pw))
                delay(jtau - mult*pp/2 - 1.5*pw - 2*rof1);
        if ((pw < 2.0*pp)&&(mult * pp > 2.0 * pw))
                delay(jtau - mult*pp/2 - pw - 2*rof1 - pp);
        if ((pw > 2.0*pp)&&(mult * pp < 2.0 * pw))
                delay(jtau - 2.5*pw - 2*rof1);
        if ((pw < 2.0*pp)&&(mult * pp < 2.0 * pw))
                delay(jtau - 2*pw - 2*rof1 - pp);
	
        rgpulse(pw,v6,rof1,0.0);
	simpulse(2.0 * pw, mult * pp, v7, v8, rof1, 0.0);   /* composite X */
        rgpulse(pw,v6,rof1,0.0);

	if (mult * pp > 2.0 * pw) delay(jtau - mult*pp/2 - pw - rof1);
	else                      delay(jtau - 2*pw - rof1);
	delay(gstab);
         if (alt_grd[0] == 'y')
         {  ifzero(v11);
              zgradpulse(gzlvl1,gt1);
            elsenz(v11);
              zgradpulse(-1.0*gzlvl1,gt1);
            endif(v11);
         }
         else
            zgradpulse(gzlvl1,gt1);
	delay((del-corr)/2);
        rgpulse(pw,v6,rof1,0.0);
	rgpulse(2*pw, v7, rof1, 0.0);         /* composite X */
        rgpulse(pw,v6,rof1,rof2);
	delay((del-corr)/2);
         if (alt_grd[0] == 'y')
         {  ifzero(v11);
              zgradpulse(gzlvl1,gt1);
            elsenz(v11);
              zgradpulse(-1.0*gzlvl1,gt1);
            endif(v11);
         }
         else
            zgradpulse(gzlvl1,gt1);
	delay(gstab);
        if (lkgate_flg[0] == 'y') lk_sample();     /* turn lock on */
	decpower(dpwr);

	status(C);
}
