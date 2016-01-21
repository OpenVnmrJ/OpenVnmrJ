// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*
  gH2BCAD - Gradient Selected H2BC/Adiabatic version

        Features included:

                Pure absorptive peak shape in F1
                3-step J-filter to suppress one-bond correlations

        Paramters:
                sspul :         selects magnetization randomization option
                gzlvlE  :       encoding Gradient level
                gtE     :       encoding gradient time
                EDratio :       encode/decode ratio
                j1min   :       Minimum J1xh value
                j1max   :       Maximum J1xh value
		nullflg :	selects TANGO-Gradient option for 1H-12C suppression
		BigT	:	constant time (~20 ms)
		dmfct	:	set to 20000 for garp1 decoupling during
				BigT-d2
		dpwrct	:	power for garp1 decoupling during BigT-d2

Reference: Nyberg N.T., Duus J.O., and Sorensen O. W., JACS 127 (2005) 6154-6155.

haitao -  first revision        : Sep 2005
haitao -  revised		: Mar 2006
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***

*/

#include <standard.h>
#include <chempack.h>

static int ph1[4] = {0, 2, 2, 0};
static int ph2[8] = {0, 0, 2, 2, 2, 2, 0, 0};
static int ph3[16] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};
static int ph6[2] = {0, 2};

pulsesequence()
{

  double j1min = getval("j1min"),
         j1max = getval("j1max"),
         gzlvlE = getval("gzlvlE"),
         gtE = getval("gtE"),
	 pwx180 = getval("pwx180"),
	 pwxlvl180 = getval("pwxlvl180"),
         EDratio = getval("EDratio"),
         gstab = getval("gstab"),
         hsglvl = getval("hsglvl"),
         hsgt = getval("hsgt"),
         dmfct = getval("dmfct"),
         dpwrct = getval("dpwrct"),
         tau,
	 tau1,
	 tau3,
         bigT = getval("BigT");

  char   pwx180ad[MAXSTR],
	 dmct[MAXSTR],
	 pwx180adR[MAXSTR];

  int	 icosel,
	 MAXni,
         prgcycle = (int)(getval("prgcycle")+0.5),
	 phase1 = (int)(getval("phase")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

  getstr("pwx180ad",pwx180ad);
  getstr("pwx180adR",pwx180adR);
  getstr("dmct",dmct);

  tau1 = 1/(2*(j1min + 0.07*(j1max - j1min)));
  tau3 = 1/(2*(j1max - 0.07*(j1max - j1min)));
  tau = 1 / (j1min+j1max);

/*  Make sure ni does not exceed what is allowed by BigT */

  MAXni = (((bigT/2 - 1/(j1min+j1max) - 2*gtE - 2*gstab - pwx180)*2*sw1) - 2);
  if ((ni > MAXni) && (ix==1))
  {
	ni = MAXni;
	putCmd("setvalue('ni',%.0f)\n",ni);
	putCmd("setvalue('ni',%.0f,'processed')\n",ni);
	fprintf(stdout, "ni set to maximum value of %0.f.\n",ni);
  }

  icosel = 1;

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

  settable(t1, 4, ph1);
  settable(t2, 8, ph2);
  settable(t3, 16, ph3);
  settable(t6, 2, ph6);

  getelem(t1, v17, v1);
  getelem(t2, v17, v2);
  getelem(t6, v17, oph);
  getelem(t3, v17, v3);

  assign(zero,v6);
  add(oph,one,v5);
  assign(zero,v4);

  add(oph,v18,oph);
  add(oph,v19,oph);

/*
  mod2(id2, v10);
  dbl(v10,v10);
*/

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);

  add(v2, v10, v2);
  add(v4, v10, v4);
  add(oph, v10, oph);

  if ((phase1 == 2) || (phase1 == 5))
    icosel = -1;

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

    obspower(tpwr);
    decpower(pwxlvl);

    if (getflag("nullflg"))
    {
      rgpulse(0.5 * pw, zero, rof1, rof1);
      delay(tau);
      decpower(pwxlvl180);
      decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
      rgpulse(2.0 * pw, zero, rof1, rof1);
      delay(tau + 2 * POWER_DELAY);
      decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
      decpower(pwxlvl);
      rgpulse(1.5 * pw, two, rof1, rof1);
      zgradpulse(hsglvl, hsgt);
      delay(1e-3);
    }

  status(B);

    rgpulse(pw, v6, rof1, rof1);

  if (getflag("dmct"))
  {
    decpower(dpwrct);
    decprgon("garp1",1/dmfct,1.0);
/*
    setstatus(DECch, TRUE, 'g', FALSE, dmfct);
*/
    decunblank();
    decon();
  }
    delay(bigT/2 - d2/2  - tau - 2*gtE - 2*gstab - pwx180 );
    rgpulse(2*pw,zero,rof1,rof1);
    delay(bigT/2 - d2/2  - tau - 2*gtE - 2*gstab - pwx180 );
  if (getflag("dmct"))
  {
    decoff();
    decblank();
    decprgoff();
/*
    setstatus(DECch, FALSE, 'c', FALSE, dmf);
*/
    decpower(pwxlvl);
  }

    delay(4*pw/PI+2*rof1);

    delay(tau);
    decrgpulse(pwx,v2,rof1,rof1);
    decpower(pwxlvl180);
    delay(gtE+2*GRADIENT_DELAY+gstab+pw-2*pwx/PI-POWER_DELAY);
    decshaped_pulse(pwx180ad,pwx180,v4,rof1,rof1);

    delay(d2/2);
    zgradpulse(icosel*gzlvlE,gtE);
    delay(gstab);
    rgpulse(2*pw,zero,rof1,rof1);
    zgradpulse(icosel*gzlvlE,gtE);
    delay(gstab);
    delay(d2/2);

    decshaped_pulse(pwx180ad,pwx180,v3,rof1,rof1);
    delay(gtE+2*GRADIENT_DELAY+gstab+pw-2*pwx/PI-POWER_DELAY);
    decpower(pwxlvl);
    decrgpulse(pwx,v1,rof1,rof1);
    delay(tau);

    simpulse(pw,pwx,one,zero,rof1,rof1);
    zgradpulse(6*gzlvlE/(10*EDratio),gtE);
    delay(tau3 - gtE - 2*GRADIENT_DELAY);
    decrgpulse(pwx,zero,rof1,rof1);
    zgradpulse(14*gzlvlE/(10*EDratio),gtE);
    delay(tau - gtE - 2*GRADIENT_DELAY);
    simpulse(2*pw,pwx,zero,zero,rof1,rof1);
    zgradpulse(40*gzlvlE/(10*EDratio),gtE);
    delay(tau1 - gtE - 2*GRADIENT_DELAY - POWER_DELAY);
    decpower(dpwr);

  status(C);

    delay(tau+tau3+3*pwx+6*rof1-tau1+WFG_START_DELAY);

}

