// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* FH_gHETCOR - Gradient selected F-H / H-F HETCOR (absolute value)

Paramters:
	sspul   :   y - selects magnetization randomization option
	hsglvl  :   Homospoil gradient level (DAC units)
	hsgt	:   Homospoil gradient time
	gzlvl1	:   Coherence defocusing gradient level
	gt1	:   Gradient time for gzlvl1
        gzlvl3  :   Coherence refocusing gradient level
        gt3     :   Gradient time for gzlvl3
	gstab	:   Recovery delay
	pw	:   90 deg. observe pulses (chan 1)
        tpwr    :   Power level for pw (chan 1)
        pwx     :   90 deg. decoupler pulses (chan 2)
        pwxlvl  :   Power level for pwx (chan 2)
        jFH     :   Magnitude of the F-H coupling used for 
                        polarization transfer
	d1	:   Relaxation delay
	d2	:   Evolution delay
        satmode :   Flag for optional solvent presaturation
        satpwr  :   Saturation power level
        satfrq  :   Presaturation frequency
        satdly  :   Presaturation delay (part of d1)
        f1coef  :   1 0 0 -1 (for gradients with opposite sign)

                   gzlvl3/gzlvl1 ratio = 1.06291 : 1  for F19 observe
                                       = 0.94081 : 1 for 1H observe
        Note: The sequence uses a dec. 180 deg. pulse in the middle
              of the evolution period to do F1 decoupling
              F2 decoupling during acquisition is optional via dm='nny'
              but may NOT be generally recommended.
                 1, As the XMTR and te DCPLR shares the same amplifier
                    band the receiver and the decoupler is time shared
                    during acquisition - this causes sensitivity losses
                    (60% by default)
                 2, In extended 1H and 19F coupling networks
                    refocussing all possible couplings may not be
                    possible resulting in unintended signal cancellation.

peter sandor, darmstadt*/

#include <standard.h>

static int ph1[ 4] = {0, 0, 0, 0},
	   ph2[ 4] = {0, 2, 1, 3},
           ph3[ 8] = {0, 0, 0, 0, 2, 2, 2, 2},
	   ph4[32] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
                      2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
           ph5[16] = {0, 2, 0, 2, 3, 1, 3, 1, 2, 0, 2, 0, 1, 3, 1, 3};

void pulsesequence()
{
	double	gzlvlE = getval("gzlvlE"),
                gtE = getval("gtE"),
                EDratio = getval("EDratio"),
		gstab = getval("gstab"),
		hsglvl = getval("hsglvl"),
		hsgt = getval("hsgt"),
		satdly = getval("satdly"),
                pwxlvl = getval("pwxlvl"),
		pwx = getval("pwx"),
                jFH = getval("jFH"),
                dly1, dly2;
	char	sspul[MAXSTR],
		satmode[MAXSTR];
        int     icosel;

	getstr("satmode",satmode);
	getstr("sspul",sspul);
        if (jFH == 0.0) jFH=7.0;
        dly1 = 1.0 / (2.0*jFH);
        dly2 = 1.0 / (3.0*jFH);
        icosel = -1;

	settable(t1,4,ph1);
	settable(t2,4,ph2);
	settable(t3,8,ph3);
        settable(t4,32,ph4);
        settable(t5,16,ph5);

	getelem(t1,ct,v1);
	getelem(t2,ct,v2);
	getelem(t3,ct,v3);
        getelem(t4,ct,v4);
        getelem(t5,ct,oph);

	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);
              {
		add(v1,v10,v1);
		add(oph,v10,oph); 
              }

       decpower(pwxlvl); decpwrf(4095.0);
status(A);

      if (sspul[0] == 'y')
	{
		zgradpulse(hsglvl,hsgt);
		rgpulse(pw,zero,rof1,rof1);
                decrgpulse(pwx,zero,rof1,rof1);
		zgradpulse(hsglvl,hsgt);
	}

      if (satmode[0] == 'y')
       {
        if (d1 - satdly > 0) delay(d1 - satdly);
        else delay(0.02);
        obspower(satpwr);
        if (satfrq != tof) obsoffset(satfrq);
        rgpulse(satdly,zero,rof1,rof1);
        if (satfrq != tof) obsoffset(tof);
        obspower(tpwr);
        delay(1.0e-5);
       }
      else delay(d1);

   if (getflag("wet")) 
     wet4(zero,one);

status(B);
	decrgpulse(pwx, v1, rof1, rof1);
        if (d2/2.0==0.0) delay(d2);
        else delay(d2/2.0-2*rof1-pw);
        rgpulse(2.0*pw,v3,rof1,rof1);
        if (d2/2.0==0.0) delay(d2);
        else delay(d2/2.0-2*rof1-pw);
        delay(dly1 - gtE-gstab);
	zgradpulse(gzlvlE*EDratio,gtE);
        delay(gstab);
        decrgpulse(pwx,v2,rof1,rof1);
	rgpulse(pw, v4, rof1, rof1); 
	zgradpulse(icosel*gzlvlE,gtE);
        decpower(dpwr);
	delay(dly2 - gtE);
status(C);
}
