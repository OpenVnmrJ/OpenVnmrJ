// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* gHMQC - Gradient Selected phase-sensitive HMQC

	Paramters:
		sspul :		selects magnetization randomization option
		nullflg:	selects TANGO-Gradient option
		hsglvl:		Homospoil gradient level (DAC units)
		hsgt	:	Homospoil gradient time
                gzlvlE  :       encoding Gradient level
                gtE     :       encoding gradient time
                EDratio :       Encode/Decode ratio
		gstab	:	recovery delay
		j1xh	:	One-bond XH coupling constant

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***

*/


#include <standard.h>
#include <chempack.h>

static int	   ph1[2] = {0,2},
	 	   ph2[4] = {0,0,2,2},
	   	   ph3[4] = {0,2,2,0};

pulsesequence()
{
  double gzlvlE = getval("gzlvlE"),
         gtE = getval("gtE"),
	 EDratio = getval("EDratio"),
         gstab = getval("gstab"),
	 tau,
	 gtau,
	 hsglvl = getval("hsglvl"),
	 hsgt = getval("hsgt");
  int    icosel,
         prgcycle = (int)(getval("prgcycle")+0.5),
	 phase1 = (int)(getval("phase")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",0.5);
        gzlvlE = syncGradLvl("gtE","gzlvlE",0.5);

  icosel = 1;
  tau = 1/(2*(getval("j1xh")));
  gtau =  2*gstab + 2*GRADIENT_DELAY;

  if (tau < (gtE/2.0+gstab))
  {
    text_error("tau must be greater than 0.5*gtE+gstab\n");
    psg_abort(1);
  }

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

  settable(t1,2,ph1);
  settable(t2,4,ph2);
  settable(t3,4,ph3);

  assign(zero,v4);
  getelem(t1,v17,v1);
  getelem(t2,v17,v2);
  getelem(t3,v17,oph);

  assign(zero,v6);
  add(oph,v18,oph);
  add(oph,v19,oph);

  if ((phase1 == 2) || (phase1 == 5))
	icosel = -1;

/*
  mod2(id2,v10);
  dbl(v10,v10);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);

  add(v1,v10,v1);
  add(v4,v10,v4);
  add(oph,v10,oph);

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

     if (getflag("nullflg"))
     {
        rgpulse(0.5*pw,zero,rof1,rof1);
        delay(tau);
        simpulse(2.0*pw,2.0*pwx,zero,zero,rof1,rof1);
        delay(tau);
        rgpulse(1.5*pw,two,rof1,rof1);
        zgradpulse(hsglvl,hsgt);
        delay(1e-3);
     }

     rgpulse(pw,v6,rof1,rof1);
     delay(tau - 2*rof1 - (2*pw/PI));

     decrgpulse(pwx,v1,rof1,1.0e-6);
     delay(gtE/2.0+gtau - (2*pwx/PI) - pwx - 1.0e-6 - rof1);
     decrgpulse(2*pwx,v4,rof1,1.0e-6);
     delay(gstab - pwx - 1.0e-6);
     zgradpulse(gzlvlE,gtE/2.0);
     delay(gstab - rof1 - pw);

     delay(d2/2);
     rgpulse(2.0*pw,zero,rof1,rof1);
     delay(d2/2);

     delay(gstab - rof1 - pw);
     zgradpulse(gzlvlE,gtE/2.0);
     delay(gstab - pwx - rof1);
     decrgpulse(2*pwx,zero,rof1,1.0e-6);
     delay(gtE/2.0+gtau - (2*pwx/PI) - pwx - 1.0e-6 - rof1);
     decrgpulse(pwx,v2,rof1,rof2);

     decpower(dpwr);
     delay(gstab - POWER_DELAY);
     zgradpulse(icosel*2*gzlvlE/EDratio,gtE/2.0);
     delay(tau - gtE/2.0 - gstab - 2*GRADIENT_DELAY);

  status(C);
} 

