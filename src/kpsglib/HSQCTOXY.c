// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* HSQCTOCSY - Heteronuclear Single Quantum Coherence followed
		by TOCSY spinlock

	Paramters:
		sspul :		selects magnetization randomization option
		hsglvl:		Homospoil gradient level (DAC units)
		hsgt	:	Homospoil gradient time
		mixT	:	TOCSY spinlock mixing time
		slpatT	:	TOCSY spinlock pattern (mlev17c,mlev17,dipsi2,dipsi3)
		trim	:	trim pulse preceeding spinlock
		slpwrT	:	spin-lock power level
		slpwT	:	90 deg pulse width for spinlock
		j1xh	:	One-bond XH coupling constant

************************************************************************
****NOTE:  v2,v3,v4,v5 and v9 are used by Hardware Loop and reserved ***
   v3 and v5 are spinlock phase
************************************************************************

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004
*/

#include <standard.h>
#include <chempack.h>
extern int dps_flag;

static int	ph1[4] = {1,1,3,3},
		ph2[2] = {0,2,},
		ph3[8] = {0,0,0,0,2,2,2,2},
		ph4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
		ph5[16] = {1,3,3,1,3,1,1,3,3,1,1,3,1,3,3,1};
		

pulsesequence()

{
   double   hsglvl = getval("hsglvl"),
            hsgt = getval("hsgt"),
            slpwT = getval("slpwT"),
            slpwrT = getval("slpwrT"),
            trim = getval("trim"),
            mixT = getval("mixT"),
            mult = getval("mult"),
            tau,
	    null = getval("null");
   char	    slpatT[MAXSTR];
   int	    phase1 = (int)(getval("phase")+0.5);

   tau  = 1/(4*(getval("j1xh")));
   getstr("slpatT",slpatT);

   if (strcmp(slpatT,"mlev17c") &&
        strcmp(slpatT,"dipsi2") &&
        strcmp(slpatT,"dipsi3") &&
        strcmp(slpatT,"mlev17") &&
        strcmp(slpatT,"mlev16"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatT);


   settable(t1,4,ph1);
   settable(t2,2,ph2);
   settable(t3,8,ph3);
   settable(t4,16,ph4);
   settable(t5,16,ph5);

   getelem(t2,ct,v6);
   getelem(t5,ct,oph);

   assign(two,v3);
   sub(v3,one,v2);
   add(v3,one,v4);
   add(v3,two,v5);
/*
   mod2(id2,v14);
   dbl(v14,v14);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   if (phase1 == 2)
     incr(v6);

   add(v6,v14,v6);
   add(oph,v14,oph);

status(A);

   obspower(tpwr);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        satpulse(satdly,zero,rof1,rof1);
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   decpower(pwxlvl);

status(B);

    if ((getflag("PFGflg")) && (getflag("nullflg")))
     {
        rgpulse(0.5*pw,zero,rof1,rof1);
        delay(2*tau);
        simpulse(2.0*pw,2.0*pwx,zero,zero,rof1,rof1);
        delay(2*tau);
        rgpulse(1.5*pw,two,rof1,rof1);
        zgradpulse(hsglvl,hsgt);
        delay(1e-3);
     }
     else if (null != 0.0)
     {
        rgpulse(pw,zero,rof1,rof1);
        delay(2*tau);
        simpulse(2*pw,2*pwx,zero,zero,rof1,rof1);
        delay(2*tau);
        rgpulse(pw,two,rof1,rof1);
        if (satmode[1] == 'y')
           satpulse(null,zero,rof1,rof1);
        else
           delay(null);
      }

     rgpulse(pw,zero,rof1,rof1);
     delay(tau - pwx - 2*pw/PI - 2*rof1);
     simpulse(2*pw,2*pwx,zero,zero,rof1,rof1);
     delay(tau - pwx - 2*pwx/PI - 2*rof1);
     rgpulse(pw,t1,rof1,rof1);
     if (getflag("PFGflg"))
      {
	zgradpulse(hsglvl,2*hsgt);
	delay(1.0e-3);
      }
     decrgpulse(pwx,v6,rof1,2.0e-6);
     if (d2/2 > 0.0)
      delay(d2/2 - (2*pwx/PI) - pw - 4.0e-6);
     else
      delay(d2/2);
     rgpulse(2*pw,zero,2.0e-6,2.0e-6);
     if (d2/2 > 0.0) 
      delay(d2/2 - (2*pwx/PI) - pw - 4.0e-6);  
     else
      delay(d2/2);
     decrgpulse(pwx,t4,2.0e-6,rof1);
     if (getflag("PFGflg"))
      {
	zgradpulse(-0.6*hsglvl,1.2*hsgt);
        delay(1.0e-3);
      }
     rgpulse(pw,t3,rof1,rof1);
     delay(tau - pwx - (2*pw/PI) - rof1);
     simpulse(2*pw,2*pwx,zero,zero,rof1, rof1);
     obspower(slpwrT);
     decpower(dpwr);
     delay(tau - rof1 - 2*POWER_DELAY);

     if (mixT > 0.0)
     {         
        rgpulse(trim,v5,0.0,0.0);
        if (dps_flag)
          rgpulse(mixT,v3,0.0,0.0);
        else
          SpinLock(slpatT,mixT,slpwT,v2,v3,v4,v5,v9);
     }  

     if (mult > 0.5)
     {
      obspower(tpwr);
      decpower(pwxlvl);
      delay(2*tau - POWER_DELAY - rof1);
      simpulse(2*pw,mult*pwx,zero,zero,rof1,rof2);
      decpower(dpwr);
      delay(2*tau);
     }
     else
        delay(rof2);

   status(C);
}
