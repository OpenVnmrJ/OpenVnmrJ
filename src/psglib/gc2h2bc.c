// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*
  gc2h2bc - Gradient Selected C2H2BC
        Adiabatic/BIP version for both nuclei

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

haitao -  first revision        : Sep 2005
haitao -  revised		: Mar 2006
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***
*/

#include <standard.h>
#include <chempack.h>

static int ph1[4] = {0, 2, 2, 0};
static int ph2[8] = {0, 0, 2, 2, 2, 2, 0, 0};
static int ph3[16] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};
static int ph6[2] = {0, 2};

void pulsesequence()
{

  double j1min = getval("j1min"),
         j1max = getval("j1max"),
         gzlvlE = getval("gzlvlE"),
         gtE = getval("gtE"),
	 pwx180 = getval("pwx180"),
	 pwxlvl180 = getval("pwxlvl180"),
	 pw180 = getval("pw180"),
	 tpwr180 = getval("tpwr180"),
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
	 bipflg[MAXSTR],
	 pw180ad[MAXSTR];

  int	 icosel,
	 phase1 = (int)(getval("phase")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

  getstr("pwx180ad",pwx180ad);
  getstr("pw180ad",pw180ad);
  getstr("dmct",dmct);

  getstr("bipflg",bipflg);
  if (bipflg[0] == 'y')
  {
        tpwr180=getval("tnbippwr");
        pw180=getval("tnbippw");
        getstr("tnbipshp",pw180ad);
  }
  if (bipflg[1] == 'y')
  {
        pwxlvl180=getval("dnbippwr");
        pwx180=getval("dnbippw");
        getstr("dnbipshp",pwx180ad);
  }

  tau1 = 1/(2*(j1min + 0.07*(j1max - j1min)));
  tau3 = 1/(2*(j1max - 0.07*(j1max - j1min)));
  tau = 1 / (j1min+j1max);

  icosel = 1;

  settable(t1, 4, ph1);
  settable(t2, 8, ph2);
  settable(t3, 16, ph3);
  settable(t6, 2, ph6);

  getelem(t2, ct, v2);
  getelem(t6, ct, oph);
  assign(zero, v4);

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);
/*
  mod2(id2, v10);
  dbl(v10,v10);
*/
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
                shaped_satpulse("relaxD",satdly,zero);
        else
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
      decpower(pwxlvl180);
      rgpulse(0.5 * pw, zero, rof1, rof1);
      obspower(tpwr180);
      delay(tau/2.0);
      simshaped_pulse(pw180ad,pwx180ad,pw180,pwx180,zero,zero,rof1,rof1);
      delay(tau/2.0+POWER_DELAY+rof1);
      delay(tau/2.0+POWER_DELAY+rof1);
      simshaped_pulse(pw180ad,pwx180ad,pw180,pwx180,two,two,rof1,rof1);
      obspower(tpwr);
      delay(tau/2.0);
      rgpulse(0.5 * pw, zero, rof1, rof1);
      decpower(pwxlvl);
      zgradpulse(hsglvl, 1.4*hsgt);
      delay(1e-3);
    }

  status(B);

   if (getflag("cpmgflg"))
   {
      rgpulse(pw, zero, rof1, 0.0);
      cpmg(zero, v15);
   }
   else
      rgpulse(pw, zero, rof1, rof1);
   if (getflag("dmct"))
   {
    decpower(dpwrct);
    decprgon("garp1",1/dmfct,1.0);
/*
    setstatus(DECch, FALSE, 'g', FALSE, dmfct);
*/
    decunblank();
    decon();
   }
    obspower(tpwr180);
    delay(bigT/2 - d2/2  - tau - 2*gtE - 2*gstab - pwx180 );
    shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
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

    delay(4*pw/PI+2*rof1+POWER_DELAY);
    delay(tau);
    decrgpulse(pwx,v2,rof1,rof1);
    decpower(pwxlvl180);
    delay(gtE+2*GRADIENT_DELAY+gstab+(WFG_START_DELAY+pw180+WFG_STOP_DELAY)/2-2*pwx/PI-POWER_DELAY);
    decshaped_pulse(pwx180ad,pwx180,v4,rof1,rof1);

    delay(d2/2);
    zgradpulse(icosel*gzlvlE,gtE);
    delay(gstab);
    shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
    zgradpulse(icosel*gzlvlE,gtE);
    delay(gstab);
    delay(d2/2);

    decshaped_pulse(pwx180ad,pwx180,t3,rof1,rof1);
    delay(gtE+2*GRADIENT_DELAY+gstab+(WFG_START_DELAY+pw180+WFG_STOP_DELAY)/2-2*pwx/PI-POWER_DELAY);
    decpower(pwxlvl);
    decrgpulse(pwx,t1,rof1,rof1);
    obspower(tpwr);
    delay(tau);

    rgpulse(pw,one,rof1,rof2);
    decrgpulse(pwx,zero,rof1,rof1);
    obspower(tpwr180);
/*
    shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
    delay(pwx+2*rof1);
*/
    zgradpulse(6*gzlvlE/(10*EDratio),gtE);
    delay(tau3 - gtE - 2*GRADIENT_DELAY);
    decrgpulse(pwx,zero,rof1,rof1);
    zgradpulse(14*gzlvlE/(10*EDratio),gtE);
    delay(tau - gtE - 2*GRADIENT_DELAY);
    decrgpulse(pwx,zero,rof1,rof1);
    shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
    zgradpulse(4*gzlvlE/EDratio,gtE);
    delay(tau1 - gtE - 2*GRADIENT_DELAY - POWER_DELAY);
    decpower(dpwr);

  status(C);

    delay(tau+tau3+3*pwx+6*rof1-tau1+WFG_START_DELAY + WFG_STOP_DELAY);
}

