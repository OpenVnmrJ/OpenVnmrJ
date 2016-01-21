// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*
  gc2h2bcme.c - multiplicity-edited gradient selected C2H2BC
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
		mult    :       Multiplicty editing (mult = 0, 2; array = 'mult,phase')

KrishK  -       First Rivision		: Jul 2005
haitao -  Last revision         : Mar 2006
KrishK	- Includes purge option : Jan 2008

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
         mult = getval("mult"),
	 pwx180 = getval("pwx180"),
	 pwxlvl180 = getval("pwxlvl180"),
	 pw180 = getval("pw180"),
         pwx180r = getval("pwx180r"),
         pwxlvl180r = getval("pwxlvl180r"),
	 tpwr180 = getval("tpwr180"),
         EDratio = getval("EDratio"),
         hsglvl = getval("hsglvl"),
         hsgt = getval("hsgt"),
         gstab = getval("gstab"),
         dmfct = getval("dmfct"),
         dpwrct = getval("dpwrct"),
         tau,
	 tau1,
	 tau3,
	 taug,
         bigT = getval("BigT");

  char   pwx180ad[MAXSTR],
	 pw180ad[MAXSTR],
	 dmct[MAXSTR],
	 bipflg[MAXSTR],
         pwx180ref[MAXSTR],
         dmmct[MAXSTR];

  int	 icosel,
         igcorr,
         prgcycle = (int)(getval("prgcycle")+0.5),
	 phase1 = (int)(getval("phase")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

  getstr("pwx180ad",pwx180ad);
  getstr("pw180ad",pw180ad);
  getstr("dmct",dmct);
  getstr("pwx180ref", pwx180ref);
  getstr("dmmct", dmmct);

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

  taug = tau + getval("tauC");

  icosel = 1;
  igcorr = 2;
  
  if (mult > 0.5)
    igcorr = 1;

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
  getelem(t3, v17, v3);
  getelem(t2, v17, v2);
  getelem(t6, v17, oph);
  assign(zero, v4);

  assign(zero,v6);
  add(oph,v18,oph);
  add(oph,v19,oph);

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
/*
  mod2(id2, v14);
  dbl(v14,v14);
*/
  add(v2, v14, v2);
  add(v4, v14, v4);
  add(oph, v14, oph);

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
    delay(bigT/2 - d2/2  - tau - taug - pwx180r);
    delay(pw180+WFG_START_DELAY+WFG_STOP_DELAY);
    shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
    delay(bigT/2 - d2/2  - tau - taug - pwx180r);
    delay(4*pw/PI+2*rof1+POWER_DELAY+4*pwx/PI+2*POWER_DELAY);
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

    if (mult > 0.5)
    {
     delay(tau-gtE-2*GRADIENT_DELAY-gstab);
     zgradpulse(icosel*gzlvlE,gtE);
     delay(gstab);
    }
    else
    {
     delay(tau);
    }

    decrgpulse(pwx,v2,rof1,rof1);

    if (mult > 0.5)
    {
    decpower(pwxlvl180r);
    delay(taug);
    decshaped_pulse(pwx180ref, pwx180r, v4, rof1, rof1);

    delay(d2/2);
    shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
    delay(d2/2);
    zgradpulse(icosel*gzlvlE,gtE);

    delay(taug-gtE-2*GRADIENT_DELAY+(4*pwx/PI+2*rof1+2*POWER_DELAY-(WFG_START_DELAY+pw180+WFG_STOP_DELAY+2*rof1)));
    decshaped_pulse(pwx180ref, pwx180r, v3, rof1, rof1);
    }

    else
    {
    decpower(pwxlvl180);
    delay(taug/2+(pwx180r-pwx180)/2);
    decshaped_pulse(pwx180ad, pwx180, v4, rof1, rof1);
    delay(taug/2+(pwx180r-pwx180)/2.0-gtE-2*GRADIENT_DELAY-gstab);

    zgradpulse(icosel*gzlvlE,gtE);
    delay(gstab);

    delay(d2/2);
    shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
    delay(d2/2);

    zgradpulse(icosel*gzlvlE,gtE);

    delay(taug/2+(pwx180r-pwx180)/2.0+(4*pwx/PI+2*rof1+2*POWER_DELAY-WFG_START_DELAY-pw180-WFG_STOP_DELAY-2*rof1)-gtE-2*GRADIENT_DELAY);
    decshaped_pulse(pwx180ad, pwx180, v3, rof1, rof1);
  
    delay(taug/2+(pwx180r-pwx180)/2);
    }

    decpower(pwxlvl);
    decrgpulse(pwx,v1,rof1,rof1);

    obspower(tpwr);

    delay(tau);

    rgpulse(pw,one,rof1,rof2);
    decrgpulse(pwx,zero,rof1,rof1);
    obspower(tpwr180);
    zgradpulse(igcorr*3*gzlvlE/(10*EDratio),gtE);
    delay(tau3 - gtE - 2*GRADIENT_DELAY);
    decrgpulse(pwx,zero,rof1,rof1);
    zgradpulse(igcorr*7*gzlvlE/(10*EDratio),gtE);
    delay(tau - gtE - 2*GRADIENT_DELAY);
    decrgpulse(pwx,zero,rof1,rof1);
    shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
    zgradpulse(igcorr*2*gzlvlE/EDratio,gtE);
    delay(tau1 - gtE - 2*GRADIENT_DELAY - POWER_DELAY);
    decpower(dpwr);

  status(C);

    delay(tau+tau3+3*pwx+6*rof1-tau1+WFG_START_DELAY+WFG_STOP_DELAY);
}

