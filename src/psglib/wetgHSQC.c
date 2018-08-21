// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* wetgHSQC - Gradient Selected phase-sensitive HSQC */

#include <standard.h>

static int	ph1[4] = {1,1,3,3},
		ph2[2] = {0,2,},
		ph3[8] = {0,0,0,0,2,2,2,2},
		ph4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
		ph5[16] = {1,3,3,1,3,1,1,3,3,1,1,3,1,3,3,1};

pulsesequence()

{
   double   pwxlvl,
            pwx,
		gzlvl1,
		gt1,
		gzlvl3,
		gt3,
		gstab,
		mult,
                hsglvl,
                hsgt,
            tau,
            taug,
            j1xh,
            phase;
   int      iphase,
	    icosel;
   char     sspul[MAXSTR],
                nullflg[MAXSTR];

   pwxlvl = getval("pwxlvl");
   pwx    = getval("pwx");
   hsglvl = getval("hsglvl");
   hsgt = getval("hsgt");
   mult = getval("mult");
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

   if (mult > 0.5)
    taug = 2*tau;
   else
    taug = gt1 + gstab + 2*GRADIENT_DELAY;

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
     decpower(pwxlvl);
     obspower(tpwr);
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

      if (nullflg[0] == 'y')
      {
        rgpulse(0.5*pw,zero,rof1,rof1);
        delay(2*tau);
        simpulse(2.0*pw,2.0*pwx,zero,zero,rof1,rof1);
        delay(2*tau);
        rgpulse(1.5*pw,two,rof1,rof1);
        zgradpulse(hsglvl,hsgt);
        delay(1e-3);
      }

     rgpulse(pw,zero,rof1,rof1);
     delay(tau);
     simpulse(2*pw,2*pwx,zero,zero,rof1,rof1);
     delay(tau);
     rgpulse(pw,t1,rof1,rof1);
	zgradpulse(hsglvl,2*hsgt);
	delay(1e-3);
     decrgpulse(pwx,v2,rof1,2.0e-6);
     if (d2/2.0 > (2.0*pwx/PI) + pw + 4.0e-6)
      delay(d2/2.0 - (2.0*pwx/PI) - pw - 4.0e-6);
      
     else {
	if (ix == 1)
	  dps_show("delay",d2/2.0);
	else if ((ix > 2) && (phase < 1.5)) 
	  text_error("increment %d cannot be timed properly\n", (int) ix/2);
     }

     rgpulse(2*pw,zero,2.0e-6,2.0e-6);
     if (d2/2.0 > (pw + 2.0e-6))
      delay(d2/2.0 - pw - 2.0e-6);  
     else if (ix == 1)
	  dps_show("delay",d2/2.0);
      
     zgradpulse(gzlvl1,gt1);
     delay(taug - gt1 - 2*GRADIENT_DELAY);
     simpulse(mult*pw,2*pwx,zero,zero,rof1,rof1);
     delay(taug - (2*pwx/PI) - 2.0e-6); 
     decrgpulse(pwx,t4,2.0e-6,rof1);
	zgradpulse(-0.6*hsglvl,1.2*hsgt);
	delay(1e-3);
     rgpulse(pw,t3,rof1,rof1);
     delay(tau - (2*pw/PI) - 2*rof1);
     simpulse(2*pw,2*pwx,zero,zero,rof1, rof2);
     decpower(dpwr);
     zgradpulse(icosel*gzlvl3,gt3);
     delay(tau - gt3 - 2*GRADIENT_DELAY - POWER_DELAY);
   status(C);
}
