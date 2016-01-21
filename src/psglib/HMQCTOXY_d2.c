// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 

 HMQCTOXY experiment  using decoupler 2
Echo family only for mult = 'y'- NM (April 23, 2007)
*/

#include <standard.h>

static int phs1[8] = {0,0,0,0,2,2,2,2},
	   phs3[2] = {0,2},
	   phs4[1] = {0},
	   phs5[4] = {0,0,2,2},
	   phs2[8] = {0,2,2,0,2,0,0,2};

mleva()
{
   double slpw;
   slpw = getval("slpw");
   txphase(zero); delay(slpw);
   xmtroff(); delay(slpw); xmtron();
   txphase(one); delay(2*slpw); 
   xmtroff(); delay(slpw); xmtron();
   txphase(zero); delay(slpw);
}
mlevb()
{
   double slpw;
   slpw = getval("slpw");
   txphase(two); delay(slpw);
   xmtroff(); delay(slpw); xmtron();
   txphase(three); delay(2*slpw);
   xmtroff(); delay(slpw); xmtron();
   txphase(two); delay(slpw);
}

pulsesequence()
{
   double          j1xh,
		   pwx2lvl,
		   pwx2,
		hsglvl,
		hsgt,
		   tau,
		   null,
                   slpw,
                   slpwr,
                   trim,
                   mix,
		mult,
                   cycles,
                satfrq,
                satdly,
                satpwr;

   int             iphase;
   char            sspul[MAXSTR],
		nullflg[MAXSTR],
		PFGflg[MAXSTR],
                satmode[MAXSTR];

   mix = getval("mix");
   slpwr = getval("slpwr");
   slpw = getval("slpw");
   trim = getval("trim");
   mult = getval("mult");
   null = getval("null");
   pwx2lvl = getval("pwx2lvl");
   pwx2 = getval("pwx2");
   hsglvl = getval("hsglvl");
   hsgt = getval("hsgt");
   j1xh = getval("j1xh");
   getstr("PFGflg",PFGflg);
   getstr("nullflg",nullflg);
        satfrq = getval("satfrq");
        satdly = getval("satdly");
        satpwr = getval("satpwr");
        getstr("satmode",satmode);

   iphase = (int) (getval("phase") + 0.5);
   getstr("sspul",sspul);

   tau = 1.0 / (2.0*j1xh);

      cycles = (mix - trim ) / (96.66*slpw);
      cycles = 2.0*(double) (int) (cycles/2.0);
      initval(cycles, v9);   
  
    if ((iphase == 1)||(iphase == 2))
     initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
    else
     assign(zero, v14);


/* Check for correct DM settings */
   if ((dm2[A] == 'y') || (dm2[B] == 'y'))
   {
      printf("DM2 must be set to either 'nny' or 'nnn'.\n");
      psg_abort(1);
   }


   settable(t1,8,phs1);
   settable(t2,8,phs2);
   settable(t3,2,phs3);
   settable(t4,1,phs4);
   settable(t5,4,phs5);

   getelem(t3,ct,v3);
   getelem(t2,ct,oph);

   if (iphase == 2)
      incr(v3);

   add(v14, v3, v3);
   add(v14, oph, oph);


/* BEGIN ACTUAL PULSE SEQUENCE CODE */
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
                if (satfrq != tof)
                obsoffset(satfrq);
                obspower(satpwr);
                rgpulse(satdly,zero,rof1,rof1);
                obspower(tpwr);
                if (satfrq != tof)
                obsoffset(tof);
        }

   status(B);
 
    
    if (PFGflg[0] == 'y')
     {
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
     } 
     else
     {
      if (null != 0.0)
	{
	rgpulse(pw,zero,rof1,rof1);
	delay(tau);
	sim3pulse(2*pw,0.0,2*pwx2,zero,zero,zero,rof1,rof1);
	delay(tau);
	rgpulse(pw,two,rof1,rof1);
	delay(null);
	}
      }

      rcvroff();
      rgpulse(pw, t1, rof1, rof1);
      delay(tau - (2*pw/PI) - 2*rof1);

      dec2rgpulse(pwx2, v3, rof1, 1.0e-6);
      if (d2 > 0.0)
       delay(d2/2.0 - pw - 3.0e-6 - (2*pwx2/PI));
      else
       delay(d2/2.0);
      rgpulse(2.0*pw, t4, 2.0e-6, 2.0e-6);
      if (d2 > 0.0) 
       delay(d2/2.0 - pw - 3.0e-6 - (2*pwx2/PI)); 
      else 
       delay(d2/2.0);
      dec2rgpulse(pwx2, t5, 1.0e-6, rof1);
      obspower(slpwr);
      txphase(one);
      delay(tau - rof1 - POWER_DELAY);

      if (cycles > 1.0)
      {
	 xmtron();
	 delay(trim);
         starthardloop(v9);
            mleva(); mleva(); mlevb(); mlevb();
            mlevb(); mleva(); mleva(); mlevb();
            mlevb(); mlevb(); mleva(); mleva();
            mleva(); mlevb(); mlevb(); mleva();
            txphase(one); delay(0.66*slpw);
         endhardloop();
         xmtroff();
       }

     if (mult > 0.5)
     {
      obspower(tpwr);
      delay(tau - POWER_DELAY - rof1);
      sim3pulse(2*pw,0.0,mult*pwx2,zero,zero,zero,rof1,0.0);
      rcvron();
      dec2power(dpwr2);
      delay(tau - POWER_DELAY);
     }
     else
     {
	dec2power(dpwr2);
	delay(rof2 - POWER_DELAY);
	rcvron();
     }

   status(C);
}
