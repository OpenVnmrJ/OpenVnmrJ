// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  ghsqc - heteronuclear single-quantum experiment using REVINEPT
           gradient selection
           using decoupler 2*/

#include <standard.h>

static int	ph1[4] = {1,1,3,3},
		ph2[2] = {0,2,},
		ph3[8] = {0,0,0,0,2,2,2,2},
		ph4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
		ph5[16] = {1,3,3,1,3,1,1,3,3,1,1,3,1,3,3,1};

pulsesequence()

{
   double   pwx2lvl,
            pwx2,
		gzlvl1,
		gt1,
		gzlvl3,
		gt3,
		gstab,
                hsglvl,
                hsgt,
            tau,
            j1xh,
            phase;
   int      iphase,
	    icosel;
   char     sspul[MAXSTR],
                nullflg[MAXSTR];

   pwx2lvl = getval("pwx2lvl");
   pwx2    = getval("pwx2");
   hsglvl = getval("hsglvl");
   hsgt = getval("hsgt");
   gzlvl1 = getval("gzlvl1");
   gzlvl3 = getval("gzlvl3");
   gt1 = getval("gt1");
   gt3 = getval("gt3");
   gstab = getval("gstab");
   getstr("nullflg",nullflg);
   j1xh    = getval("j1xh");
   tau  = 1/(4*j1xh);
   phase  = getval("phase");
   getstr("sspul",sspul);   

   iphase = (int) (phase + 0.5);
   icosel = 1;

   settable(t1,4,ph1);
   settable(t2,2,ph2);
   settable(t3,8,ph3);
   settable(t4,16,ph4);
   settable(t5,16,ph5);

   getelem(t2,ct,v2);
   getelem(t5,ct,oph);

   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   if (iphase == 2)
     icosel = -1;

   add(v2,v14,v2);
   add(oph,v14,oph);

   status(A);
     dec2power(pwx2lvl);
     obspower(tpwr);
     if (sspul[0] == 'y')
   {
         zgradpulse(hsglvl,hsgt);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(hsglvl,hsgt);
   }

      delay(d1);

    status(B);

      if (nullflg[0] == 'y')
      {
        rgpulse(0.5*pw,zero,rof1,rof1);
        delay(2*tau);
        sim3pulse(2.0*pw,0.0,2.0*pwx2,zero,zero,zero,rof1,rof1);
        delay(2*tau);
        rgpulse(1.5*pw,two,rof1,rof1);
        zgradpulse(hsglvl,hsgt);
        delay(1e-3);
      }

     rcvroff();
     rgpulse(pw,zero,rof1,rof1);
     delay(tau);
     sim3pulse(2*pw,0.0,2*pwx2,zero,zero,zero,rof1,rof1);
     delay(tau);
     rgpulse(pw,t1,rof1,rof1);
	zgradpulse(hsglvl,2*hsgt);
	delay(1e-3);
     dec2rgpulse(pwx2,v2,rof1,2.0e-6);
     if (d2/2 > 0.0)
      delay(d2/2 - (2*pwx2/PI) - pw - 4.0e-6);
     else
      delay(d2/2);
     rgpulse(2*pw,zero,2.0e-6,2.0e-6);
     if (d2/2 > 0.0) 
      delay(d2/2 - pw - 2.0e-6);  
     else
      delay(d2/2);

     delay(gt1+gstab + GRADIENT_DELAY + (2*pwx2/PI) + 2.0e-6);
     dec2rgpulse(2*pwx2,zero,rof1,rof1);
     zgradpulse(gzlvl1,gt1);
     delay(gstab);

     dec2rgpulse(pwx2,t4,2.0e-6,rof1);
	zgradpulse(-hsglvl,hsgt);
	delay(1e-3);
     rgpulse(pw,t3,rof1,rof1);
     delay(tau - (2*pw/PI) - 2*rof1);
     sim3pulse(2*pw,0.0,2*pwx2,zero,zero,zero,rof1, rof2);
     dec2power(dpwr2);
     zgradpulse(icosel*gzlvl3,gt3);
     delay(tau - gt3 - GRADIENT_DELAY - POWER_DELAY);
   status(C);
}
