// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* wetgHMBC - Gradient Selected aboslute value HMBC */

#include <standard.h>

static int ph1[1] = {0};
static int ph2[1] = {0};
static int ph3[2] = {0,2};
static int ph4[1] = {0};
static int ph5[4] = {0,0,2,2};
static int ph6[4] = {0,2,2,0};

pulsesequence()
{
  double j1xh,
         jnxh,
	 pwxlvl,
	 pwx,
	 gzlvl0,
	 gt0,
	 gzlvl1,
	 gt1,
	 gzlvl3,
	 gt3,
	 gstab,
	 hsglvl,
	 hsgt,
	 tau,
         taumb;
  char	 sspul[MAXSTR];

  j1xh = getval("j1xh");
  jnxh = getval("jnxh");
  pwxlvl = getval("pwxlvl");
  pwx = getval("pwx");
  getstr("sspul",sspul);
  gzlvl0=getval("gzlvl0");
  gt0=getval("gt0");
  gzlvl1 = getval("gzlvl1");
  gt1 = getval("gt1");
  gzlvl3 = getval("gzlvl3");
  gt3 = getval("gt3");
  gstab = getval("gstab");
  hsglvl = getval("hsglvl");
  hsgt = getval("hsgt");
  taumb = 1/(2*jnxh);
  tau=1/(2.0*j1xh);

  settable(t1,1,ph1);
  settable(t2,1,ph2);
  settable(t3,2,ph3);
  settable(t4,1,ph4);
  settable(t5,4,ph5);
  settable(t6,4,ph6);
 
  getelem(t3,ct,v3);
  getelem(t6,ct,oph);

  initval(2.0*(double)((int)(d2*getval("sw1")+0.5)%2),v10);
  add(v3,v10,v3);
  add(oph,v10,oph);

  status(A);
     decpower(pwxlvl);
     if (sspul[0] == 'y')
     {
	zgradpulse(hsglvl,hsgt);
	rgpulse(pw,zero,rof1,rof1);
	zgradpulse(hsglvl,hsgt);
     }

     delay(d1);
   if (getflag("wet")) 
     wet4(zero,one);
     decpower(pwxlvl);
  status(B);
     rgpulse(pw,t1,rof1,rof2);
     zgradpulse(gzlvl0,gt0);
     delay(tau - rof2 - rof1 - 2*GRADIENT_DELAY - gt0);
     decrgpulse(pwx, t2, rof1, rof1);
     zgradpulse(-gzlvl0,gt0);
     delay(taumb - rof1 - pwx - 2*GRADIENT_DELAY - gt0);
     decrgpulse(pwx,v3,rof1,rof1);
     zgradpulse(gzlvl1,gt1);
     delay(gstab);
     delay(d2/2);
     rgpulse(pw*2.0,t4,rof1,rof1);
     delay(d2/2);
     zgradpulse(gzlvl1,gt1);
     delay(gstab);
     decrgpulse(pwx,t5,rof1,rof2);
     zgradpulse(gzlvl3,gt3);
     decpower(dpwr);
     delay(gstab);
 
  status(C);
} 

