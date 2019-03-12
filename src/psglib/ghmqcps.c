// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* ghmqcps.c - Pulsed Field Gradient HMQC
		phase sensitive version
		Does not do gradient HMBC!!

	Processing:   wft2d(1,0,1,0,0,1,0,-1)
			rp and lp same as in s2pul spectrum
			rp1 and lp1 will be close to zero
		      (wft2dnp can also be used with the "usual"
			45 deg shift in f1 and f2)

	Recommendations:
	  (small molecule in H2O)
		13C:    gzlvl1 = 10000
			gt1    = 0.001
			gzlvl3 = 5025
			gt3    = 0.001
			gstab  = 0.0005
			hsgpwr = 10000
			hsgt   = 0.005
			nt = 1 per inc (FAD'ed axials, though
					substantially reduced
					compared to original ghmqcps)
			     2 per inc (No significant axials or
					FAD'ed axials even with 
					high concentration samples
					-  recommended)
			     4 per inc (more improvement in axial
					suppression)

		Modification - (i) Two gt1's preceed and follow d2,
				   instead of follow and 
				   preceed X 90's. 
			      (ii) TANGO pulse followed by a 
				   homospoil gradient preceed 
				   the actual sequence
			     (iii) sspul option included
			      (iv) FAD on both first X 90 and
				   first X 180
			       (v) Two step phase cycling on X 90's
*/


#include <standard.h>

static int	   ph1[2] = {0,2},
	 	   ph2[4] = {0,0,2,2},
	   	   ph3[4] = {0,2,2,0};

void pulsesequence()
{
  double gzlvl1,
         gt1,
         gzlvl3,
         gt3,
         gstab,
	 gtau,
	 tauc,
	 hsgpwr,
	 hsgt;
  char   nullflg[MAXSTR],
	 sspul[MAXSTR];
  int    iphase,
         icosel;

  gzlvl1 = getval("gzlvl1");
  gt1 = getval("gt1");
  gzlvl3 = getval("gzlvl3");
  gt3 = getval("gt3");
  hsgpwr = getval("hsgpwr");
  hsgt = getval("hsgt");
  gstab = getval("gstab");
  getstr("nullflg",nullflg);
  getstr("sspul",sspul);
  iphase = (int)(getval("phase")+0.5);
  icosel = 1;
  tau = 1/(2*(getval("j1xh")));
  gtau =  2*gstab + GRADIENT_DELAY;
  tauc = gtau+gt3-gstab;

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
     decpower(pwxlvl);

     if (sspul[0] == 'y')
     {
        zgradpulse(hsgpwr,0.01);
        rgpulse(pw,zero,rof1,rof1);
        zgradpulse(hsgpwr,0.01);
     }

     delay(d1);

  status(B);

     rcvroff();
     if (nullflg[0] == 'y')
     {
        rgpulse(0.5*pw,zero,rof1,rof1);
        delay(tau);
        simpulse(2.0*pw,2.0*pwx,zero,zero,rof1,rof1);
        delay(tau);
        rgpulse(1.5*pw,two,rof1,rof1);
        zgradpulse(hsgpwr,hsgt);
        delay(1e-3);
     }

     rgpulse(pw,zero,rof1,1.0e-6);
     delay(tau - 1.0e-6 - (2*pw/PI));

     decrgpulse(pwx,v1,1.0e-6,1.0e-6);
     delay(gt1+gtau);
     decrgpulse(2*pwx,v4,1.0e-6,1.0e-6);
     delay(gstab);
     zgradpulse(gzlvl1,gt1);
     delay(gstab);

     if (d2/2 > pw)
        delay(d2/2 - pw);
     else
        delay(d2/2);
     rgpulse(2.0*pw,zero,1.0e-6,1.0e-6);
     if (d2/2 > pw) 
        delay(d2/2 - pw);
     else 
        delay(d2/2);

     delay(gstab);
     zgradpulse(gzlvl1,gt1);
     delay(gstab);
     decrgpulse(2*pwx,zero,1.0e-6,1.0e-6);
     delay(gt1+gtau);
     decrgpulse(pwx,t2,1.0e-6,1.0e-6);

     delay(gstab);
     zgradpulse(icosel*gzlvl3,gt3);
     delay(tau - tauc);
     decpower(dpwr);
     delay(rof2 - POWER_DELAY);
     rcvron();

  status(C);
} 

