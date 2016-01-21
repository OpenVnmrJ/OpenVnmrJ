// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* ghmqcps.c - Pulsed Field Gradient HMQC
		phase sensitive version
		Does not do gradient HMBC!!
		using decoupler 2
	Krish Krishnamurthy,  June 1995
*/


#include <standard.h>

static int	   ph1[2] = {0,2},
	 	   ph2[4] = {0,0,2,2},
	   	   ph3[4] = {0,2,2,0};

pulsesequence()
{
  double gzlvl1,
         gt1,
         gzlvl3,
         gt3,
         gstab,
	 pwx2lvl,
	 pwx2,
	 tau,
	 gtau,
	 hsglvl,
	 hsgt;
  char   nullflg[MAXSTR],
	 sspul[MAXSTR];
  int    iphase,
         icosel;

  gzlvl1 = getval("gzlvl1");
  gt1 = getval("gt1");
  gzlvl3 = getval("gzlvl3");
  gt3 = getval("gt3");
  pwx2lvl = getval("pwx2lvl");
  pwx2 = getval("pwx2");
  hsglvl = getval("hsglvl");
  hsgt = getval("hsgt");
  gstab = getval("gstab");
  getstr("nullflg",nullflg);
  getstr("sspul",sspul);
  iphase = (int)(getval("phase")+0.5);
  icosel = 1;
  tau = 1/(2*(getval("j1xh")));
  gtau =  2*gstab + GRADIENT_DELAY;

  if (tau < (gt3+gstab))
  {
    text_error("tau must be greater than gt3+gstab\n");
    psg_abort(1);
  }

  settable(t1,2,ph1);
  settable(t2,4,ph2);
  settable(t3,4,ph3);

  assign(zero,v4);
  getelem(t1,ct,v1);
  getelem(t3,ct,oph);

  if (iphase == 2)
	icosel = -1;

  initval(2.0*(double)((int)(d2*getval("sw1")+0.5)%2),v10); 
  add(v1,v10,v1);
  add(v4,v10,v4);
  add(oph,v10,oph);

  status(A);
     dec2power(pwx2lvl);

     if (sspul[0] == 'y')
     {
        zgradpulse(hsglvl,hsgt);
        rgpulse(pw,zero,rof1,rof1);
        zgradpulse(hsglvl,hsgt);
     }

     delay(d1);

  status(B);

     rcvroff();
     if (nullflg[0] == 'y')
     {
        rgpulse(0.5*pw,zero,rof1,rof1);
        delay(tau);
        sim3pulse(2.0*pw,0.0,2.0*pwx2,zero,zero,zero,rof1,rof1);
        delay(tau);
        rgpulse(1.5*pw,two,rof1,rof1);
        zgradpulse(hsglvl,hsgt);
        delay(1e-3);
     }

     rgpulse(pw,zero,rof1,rof1);
     delay(tau - rof1 - (2*pw/PI));

     dec2rgpulse(pwx2,v1,rof1,1.0e-6);
     delay(gt1+gtau - (2*pwx2/PI) - pwx2 - 1.0e-6 - rof1);
     dec2rgpulse(2*pwx2,v4,rof1,1.0e-6);
     delay(gstab - pwx2 - 1.0e-6);
     zgradpulse(gzlvl1,gt1);
     delay(gstab - rof1 - pw);
     delay(d2/2);
     rgpulse(2.0*pw,zero,rof1,rof1);
     delay(d2/2);

     delay(gstab - rof1 - pw);
     zgradpulse(gzlvl1,gt1);
     delay(gstab - pwx2 - rof1);
     dec2rgpulse(2*pwx2,zero,rof1,1.0e-6);
     delay(gt1+gtau - (2*pwx2/PI) - pwx2 - 1.0e-6 - rof1);
     dec2rgpulse(pwx2,t2,rof1,rof1);

     delay(gstab - rof1);
     zgradpulse(icosel*gzlvl3,gt3);
     delay(tau - gt3 - gstab - GRADIENT_DELAY);
     dec2power(dpwr2);
     delay(rof2 - POWER_DELAY);
     rcvron();

  status(C);
} 

