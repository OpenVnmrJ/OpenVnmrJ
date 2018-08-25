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

Reference: Nyberg N.T., Duus J.O., and Sorensen O. W., JACS 127 (2005) 6154-6155.

haitao -  first revision        : Sep 2005
haitao -  revised		: Mar 2006

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
         EDratio = getval("EDratio"),
         gstab = getval("gstab"),
         hsglvl = getval("hsglvl"),
         hsgt = getval("hsgt"),
         tau,
	 tau1,
	 tau3,
         bigT = getval("BigT");

  int	 icosel,
	 MAXni,
	 phase1 = (int)(getval("phase")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

  tau1 = 1/(2*(j1min + 0.07*(j1max - j1min)));
  tau3 = 1/(2*(j1max - 0.07*(j1max - j1min)));
  tau = 1 / (j1min+j1max);

/*  Make sure ni does not exceed what is allowed by BigT */

  MAXni = (((bigT/2 - 1/(j1min+j1max) - 2*gtE - 2*gstab - 2*pwx)*2*(getval("sw1"))) - 2);
  if ((ni > MAXni) && (ix==1))
  {
	ni = MAXni;
	putCmd("setvalue('ni',%.0f)\n",ni);
	putCmd("setvalue('ni',%.0f,'processed')\n",ni);
	fprintf(stdout, "ni set to maximum value of %0.f.\n",ni);
  }

  icosel = 1;

  settable(t1, 4, ph1);
  settable(t2, 8, ph2);
  settable(t3, 16, ph3);
  settable(t6, 2, ph6);

  getelem(t1, ct, v1);
  getelem(t2, ct, v2);
  getelem(t6, ct, oph);
  getelem(t3, ct, v3);

  assign(zero,v6);
  add(oph,one,v5);
  assign(zero,v4);

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
        satpulse(satdly,zero,rof1,rof1);
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
      simpulse(2*pw,2*pwx,zero,zero,rof1,rof1);
      delay(tau);
      rgpulse(1.5 * pw, two, rof1, rof1);
      zgradpulse(hsglvl, hsgt);
      delay(1e-3);
    }

  status(B);

    rgpulse(pw, v6, rof1, rof1);

    delay(bigT/2 - d2/2  - tau - 2*gtE - 2*gstab - 2*pwx );
    rgpulse(2*pw,zero,rof1,rof1);
    delay(bigT/2 - d2/2  - tau - 2*gtE - 2*gstab - 2*pwx );

    delay(4*pw/PI+2*rof1);

    delay(tau);
    decrgpulse(pwx,v2,rof1,rof1);
    delay(gtE+2*GRADIENT_DELAY+gstab+pw-2*pwx/PI);
    decrgpulse(2*pwx,v4,rof1,rof1);

    delay(d2/2);
    zgradpulse(icosel*gzlvlE,gtE);
    delay(gstab);
    rgpulse(2*pw,zero,rof1,rof1);
    zgradpulse(icosel*gzlvlE,gtE);
    delay(gstab);
    delay(d2/2);

    decrgpulse(2*pwx,v3,rof1,rof1);
    delay(gtE+2*GRADIENT_DELAY+gstab+pw-2*pwx/PI);
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

