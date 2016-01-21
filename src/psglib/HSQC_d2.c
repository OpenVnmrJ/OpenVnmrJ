// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  hsqc - heteronuclear single-quantum experiment using REVINEPT
           in HYPERCOMPLEX mode only  
           using decoupler 2 */

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
                hsglvl,
                hsgt,
                satfrq,
                satdly,
                satpwr,
            tau,
            j1xh,
	    null,
            phase;
   int      iphase;
   char     sspul[MAXSTR],
                nullflg[MAXSTR],
                PFGflg[MAXSTR],
            satmode[MAXSTR];

   pwx2lvl = getval("pwx2lvl");
   pwx2    = getval("pwx2");
   hsglvl = getval("hsglvl");
   hsgt = getval("hsgt");
   getstr("PFGflg",PFGflg);
   getstr("nullflg",nullflg);
        satfrq = getval("satfrq");
        satdly = getval("satdly");
        satpwr = getval("satpwr");
   null = getval("null");
   j1xh    = getval("j1xh");
   tau  = 1/(4*j1xh);
   phase  = getval("phase");
   getstr("satmode",satmode);
   getstr("sspul",sspul);   

   iphase = (int) (phase + 0.5);


   settable(t1,4,ph1);
   settable(t2,2,ph2);
   settable(t3,8,ph3);
   settable(t4,16,ph4);
   settable(t5,16,ph5);

   getelem(t2,ct,v2);
   getelem(t5,ct,oph);

   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   if (iphase == 2)
     incr(v2);

   add(v2,v14,v2);
   add(oph,v14,oph);

   status(A);
     dec2power(pwx2lvl);
     obspower(tpwr);
     if (sspul[0] == 'y')
   {
        if (PFGflg[0] == 'y')
        {
         zgradpulse(hsglvl,hsgt);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(hsglvl,hsgt);
        }
        else
        {
        obspower(tpwr-12);
        rgpulse(500*pw,zero,rof1,rof1);
        rgpulse(500*pw,one,rof1,rof1);
        obspower(tpwr);
        }
   }

      delay(d1);

     if (satmode[0] == 'y')
      {
       obspower(satpwr);
        if (satfrq != tof)
         obsoffset(satfrq);
        rgpulse(satdly,zero,rof1,rof1);
        if (satfrq != tof)
         obsoffset(tof);
       obspower(tpwr);
       delay(1.0e-5);
      }

    status(B);

    if (PFGflg[0] == 'y')
     {
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
     } 
     else
     {
      if (null != 0.0)
        {
        rgpulse(pw,zero,rof1,rof1);
        delay(2*tau);
        sim3pulse(2*pw,0.0,2*pwx2,zero,zero,zero,rof1,rof1);
        delay(2*tau);
        rgpulse(pw,two,rof1,rof1);
        delay(null);
        }
      }

     rcvroff();
     rgpulse(pw,zero,rof1,rof1);
     delay(tau);
     sim3pulse(2*pw,0.0,2*pwx2,zero,zero,zero,rof1,rof1);
     delay(tau);
     rgpulse(pw,t1,rof1,rof1);
     if (PFGflg[0] == 'y')
     {
	zgradpulse(hsglvl,hsgt);
	delay(1e-3);
     }
     dec2rgpulse(pwx2,v2,rof1,2.0e-6);
     if (d2/2 > 0.0)
      delay(d2/2 - (2*pwx2/PI) - pw - 4.0e-6);
     else
      delay(d2/2);
     rgpulse(2*pw,zero,2.0e-6,2.0e-6);
     if (d2/2 > 0.0) 
      delay(d2/2 - (2*pwx2/PI) - pw - 4.0e-6);  
     else
      delay(d2/2);
     dec2rgpulse(pwx2,t4,2.0e-6,rof1);
     if (PFGflg[0] == 'y')
     {
	zgradpulse(hsglvl,hsgt);
	delay(1e-3);
     }
     rgpulse(pw,t3,rof1,rof1);
     delay(tau - (2*pw/PI) - 2*rof1);
     sim3pulse(2*pw,0.0,2*pwx2,zero,zero,zero,rof1, rof2);
     rcvron();
     dec2power(dpwr2);
     delay(tau - POWER_DELAY);
   status(C);
}
