// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* HSQC - Heteronuclear Single Quantum Coherence

	Paramters:
		sspul :		selects magnetization randomization option
		hsglvl:		Homospoil gradient level (DAC units)
		hsgt	:	Homospoil gradient time
		j1xh	:	One-bond XH coupling constant

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/

#include <standard.h>
#include <chempack.h>

static int	ph1[4] = {1,1,3,3},
		ph2[2] = {0,2},
		ph3[8] = {0,0,0,0,2,2,2,2},
		ph4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
		ph5[16] = {1,3,3,1,3,1,1,3,3,1,1,3,1,3,3,1};

pulsesequence()

{
   double   hsglvl = getval("hsglvl"),
            hsgt = getval("hsgt"),
            tau,
            evolcorr,
	    taug,
	    mult = getval("mult"),
	    null = getval("null");
   int	    phase1 = (int)(getval("phase")+0.5),
            prgcycle = (int)(getval("prgcycle")+0.5),
	    ZZgsign;

   tau  = 1/(4*(getval("j1xh")));
   if (mult > 0.5)
    taug = 2*tau;
   else
    taug = 0.0;
   evolcorr=2*pw+4.0e-6;
   ZZgsign=-1;
   if (mult == 2) ZZgsign=1;

  assign(ct,v17);
  assign(zero,v18);
  assign(zero,v19);

  if (getflag("prgflg") && (satmode[0] == 'y') && (prgcycle > 1.5))
    {
        hlv(ct,v17);
        mod2(ct,v18); dbl(v18,v18);
        if (prgcycle > 2.5)
           {
                hlv(v17,v17);
                hlv(ct,v19); mod2(v19,v19); dbl(v19,v19);
           }
     }

   settable(t1,4,ph1);
   settable(t2,2,ph2);
   settable(t3,8,ph3);
   settable(t4,16,ph4);
   settable(t5,16,ph5);

  getelem(t1, v17, v1);
  getelem(t3, v17, v3);
  getelem(t4, v17, v4);
  getelem(t2, v17, v2);
  getelem(t5, v17, oph);

  assign(zero,v6);
  add(oph,v18,oph);
  add(oph,v19,oph);

/*
   mod2(id2,v14);
   dbl(v14,v14);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   if (phase1 == 2)
     incr(v2);

   add(v2,v14,v2);
   add(oph,v14,oph);

   if (mult > 0.5) 
	add(oph,two,oph);

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
        if (getflag("slpsat"))
           {
                shaped_satpulse("relaxD",satdly,zero);
                if (getflag("prgflg"))
                   shaped_purge(v6,zero,v18,v19);
           }
        else
           {
                satpulse(satdly,zero,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v6,zero,v18,v19);
           }
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
	{
           if (getflag("slpsat"))
                shaped_satpulse("BIRDnull",null,zero);
           else
                satpulse(null,zero,rof1,rof1);
	}
	else
           delay(null);
      }

      if (getflag("cpmgflg"))
      {
        rgpulse(pw, v6, rof1, 0.0);
        cpmg(v6, v15);
      }
      else
        rgpulse(pw, v6, rof1, rof1);
     delay(tau);
     simpulse(2*pw,2*pwx,zero,zero,rof1,rof1);
     delay(tau);
     rgpulse(pw,v1,rof1,rof1);
     if (getflag("PFGflg"))
     {
	zgradpulse(hsglvl,2*hsgt);
	delay(1e-3);
     }
     decrgpulse(pwx,v2,rof1,2.0e-6);

     if (mult > 0.5)
     {
	delay(d2/2);
	rgpulse(2*pw,zero,2.0e-6,2.0e-6);
	delay(d2/2);
     delay(taug);
     simpulse(mult*pw,2*pwx,zero,zero,rof1,rof1);
     delay(taug + evolcorr);
     }
     else
     {
     if (d2/2 > 0.0)
      delay(d2/2 - (2*pwx/PI) - pw - 4.0e-6);
     else
      delay(d2/2);
     rgpulse(2*pw,zero,2.0e-6,2.0e-6);
     if (d2/2 > 0.0)
      delay(d2/2 - (2*pwx/PI) - pw - 4.0e-6);
     else
      delay(d2/2);
     }

     decrgpulse(pwx,v4,2.0e-6,rof1);
     if (getflag("PFGflg"))
     {
	zgradpulse(ZZgsign*0.6*hsglvl,1.2*hsgt);
	delay(1e-3);
     }
     rgpulse(pw,v3,rof1,rof1);
     delay(tau - (2*pw/PI) - 2*rof1);
     simpulse(2*pw,2*pwx,zero,zero,rof1, rof2);
     decpower(dpwr);
     delay(tau - POWER_DELAY);
   status(C);
}
