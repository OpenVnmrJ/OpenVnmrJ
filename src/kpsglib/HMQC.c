// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* HMQC - Heteronuclear Multiple Quantum Coherence

	Paramters:
		sspul :		selects magnetization randomization option
		hsglvl:		Homospoil gradient level (DAC units)
		hsgt	:	Homospoil gradient time
		j1xh	:	One-bond XH coupling constant

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004

*/

#include <standard.h>
#include <chempack.h>

static int phs1[8] = {0,0,0,0,2,2,2,2},
	   phs3[2] = {0,2},
	   phs4[1] = {0},
	   phs5[4] = {0,0,2,2},
	   phs2[8] = {0,2,2,0,2,0,0,2};

void pulsesequence()
{
   double 	hsglvl = getval("hsglvl"),
		hsgt = getval("hsgt"),
	  	tau,
		null = getval("null");
   int		phase1 = (int)(getval("phase")+0.5);

   tau = 1.0 / (2.0*(getval("j1xh")));
/*
   mod2(id2,v14);
   dbl(v14,v14);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   settable(t1,8,phs1);
   settable(t2,8,phs2);
   settable(t3,2,phs3);
   settable(t4,1,phs4);
   settable(t5,4,phs5);

   getelem(t3,ct,v3);
   getelem(t2,ct,oph);

   if (phase1 == 2)
      incr(v3);

   add(v14, v3, v3);
   add(v14, oph, oph);


/* BEGIN ACTUAL PULSE SEQUENCE CODE */
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
        delay(tau);
        simpulse(2.0*pw,2.0*pwx,zero,zero,rof1,rof1);
        delay(tau);
        rgpulse(1.5*pw,two,rof1,rof1);
        zgradpulse(hsglvl,hsgt);
        delay(1e-3);
     } 
     else if (null != 0.0)
     {
        rgpulse(pw,zero,rof1,rof1);
        delay(tau);
        simpulse(2*pw,2*pwx,zero,zero,rof1,rof1);
        delay(tau);
        rgpulse(pw,two,rof1,rof1);
        if (satmode[1] == 'y')
           satpulse(null,zero,rof1,rof1);
        else
           delay(null);
      }
 
    
      rgpulse(pw, t1, rof1, rof1);
      delay(tau - (2*pw/PI) - 2*rof1);

      decrgpulse(pwx, v3, rof1, 1.0e-6);
      if (d2 > 0.0)
       delay(d2/2.0 - pw - 3.0e-6 - (2*pwx/PI));
      else
       delay(d2/2.0);
      rgpulse(2.0*pw, t4, 2.0e-6, 2.0e-6);
      if (d2 > 0.0) 
       delay(d2/2.0 - pw - 3.0e-6 - (2*pwx/PI)); 
      else 
       delay(d2/2.0);
      decrgpulse(pwx, t5, 1.0e-6, rof2);

      decpower(dpwr);
      delay(tau - POWER_DELAY);

   status(C);
}
