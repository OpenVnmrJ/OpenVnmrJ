// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* HMBC - Heteronuclear Multiple Bond Correlation 

	Paramters:
		sspul :		selects magnetization randomization option
                jnxh    :       multiple bond XH coupling constant
                j1min   :       Minimum J1xh value
                j1max   :       Maximum J1xh value
                jfilter :       Selects jfilter
				[2-step if PFGflg=y else 1-step]

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***

*/


#include <standard.h>
#include <chempack.h>

static int ph1[1] = {0};
static int ph2[2] = {0,2};
static int ph3[4] = {0,0,2,2};
static int ph4[1] = {0};
static int ph5[8] = {0,0,0,0,2,2,2,2};
static int ph6[8] = {0,0,2,2,2,2,0,0};
static int ph7[4] = {0,2,2,0};

pulsesequence()
{
  double j1min = getval("j1min"),
         j1max = getval("j1max"),
         gzlvl0 = getval("gzlvl0"),
         gt0 = getval("gt0"),
	 tau,
         tauA,
         tauB,
         taumb;
  int	 phase1 = (int)(getval("phase")+0.5),
         prgcycle = (int)(getval("prgcycle")+0.5);
  tauA = 1/(2*(j1min + 0.146*(j1max - j1min)));
  tauB = 1/(2*(j1max - 0.146*(j1max - j1min)));
  taumb = 1/(2*(getval("jnxh")));
  tau=1/(j1min+j1max);

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

  settable(t1,1,ph1);
  settable(t2,2,ph2);
  settable(t3,4,ph3);
  settable(t4,1,ph4);
  settable(t5,8,ph5);
  settable(t6,8,ph6);
  settable(t7,4,ph7);

  getelem(t1,v17,v1);
  getelem(t4,v17,v5);
  getelem(t2,v17,v2);
if (getflag("jfilter"))
{
 if (getflag("PFGflg"))
   {
        getelem(t2,v17,v3);
        getelem(t7,v17,oph);
        getelem(t3,v17,v4);
   }
 else
   {
        getelem(t3,v17,v3);
        getelem(t6,v17,oph);
        getelem(t5,v17,v4);
   }
}
else
{
        getelem(t2,v17,v3);
        getelem(t7,v17,oph);
        getelem(t3,v17,v4);
}

  add(oph,v18,oph);
  add(oph,v19,oph);

  if (phase1 == 2)
    incr(v3);

/*
  mod2(id2,v10);
  dbl(v10,v10);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);

  add(v3,v10,v3);
  add(oph,v10,oph);

  status(A);

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
                   shaped_purge(v1,zero,v18,v19);
           }
        else
           {
                satpulse(satdly,zero,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,zero,v18,v19);
           }
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   decpower(pwxlvl);

  status(B);
     rgpulse(pw,v1,rof1,rof2);

/* Start of J filter  */
   if (getflag("jfilter"))
   {
     if (getflag("PFGflg"))
     {
     	zgradpulse(gzlvl0/2,gt0);
     	delay(tauA - gt0);
     	decrgpulse(pwx, zero, rof1, rof1);
     	zgradpulse(-gzlvl0/3,gt0);
     	delay(tauB - gt0);
     	decrgpulse(pwx, zero, rof1, rof1);
     	zgradpulse(-gzlvl0/6,gt0);
     	delay(gt0/3);
     }
     else
     {
     	delay(tau - rof2 - rof1);
	decrgpulse(pwx,v2,rof1,rof1);
     }
    }
/* End of J filter */

     delay(taumb);
     decrgpulse(pwx,v3,rof1,2.0e-6);
     if (d2/2 > 0.0)
      delay(d2/2 - 2*pwx/PI - pw - 4.0e-6);
     else
     delay(d2/2);
     rgpulse(pw*2.0,v5,2.0e-6,2.0e-6);
     if (d2/2 > 0.0)
      delay(d2/2 - 2*pwx/PI - pw - 4.0e-6);
     else
     delay(d2/2);
     decrgpulse(pwx,v4,2.0e-6,rof2);
     decpower(dpwr);
 
  status(C);
} 

