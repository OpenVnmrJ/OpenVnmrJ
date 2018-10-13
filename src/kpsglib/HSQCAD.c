// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* HSQCAD - Heteronuclear Single Quantum Coherence
	Adiabatic Version

        Paramters:
                sspul :         selects magnetization randomization option
                hsglvl:         Homospoil gradient level (DAC units)
                hsgt    :       Homospoil gradient time
                j1xh    :       One-bond XH coupling constant

KrishK  -       Last revision   : June 1997
KrishK  -       Revised         : July 2004

*/

#include <standard.h>
#include <chempack.h>

static int      ph1[4] = {1,1,3,3},
                ph2[2] = {0,2,},
                ph3[8] = {0,0,0,0,2,2,2,2},
                ph4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
                ph5[16] = {1,3,3,1,3,1,1,3,3,1,1,3,1,3,3,1};

pulsesequence()

{

   double   hsglvl = getval("hsglvl"),
            hsgt = getval("hsgt"),
            tau,
            taug,
	    pwx180 = getval("pwx180"),
	    pwxlvl180 = getval("pwxlvl180"),
	    pwx180r = getval("pwx180r"),
	    pwxlvl180r = getval("pwxlvl180r"),
            mult = getval("mult"),
            null = getval("null");
  int	    phase1 = (int)(getval("phase")+0.5),
	    ZZgsign;

  char      pwx180ad[MAXSTR],
            pwx180adR[MAXSTR], 
            pwx180ref[MAXSTR];

  getstr("pwx180ad", pwx180ad);
  getstr("pwx180adR", pwx180adR);
  getstr("pwx180ref", pwx180ref);

  tau  = 1/(4*(getval("j1xh")));
  if (mult > 0.5)
    taug = 2*tau + getval("tauC");
  else
    taug = 0.0;
  ZZgsign=-1;
  if (mult == 2) ZZgsign=1;

  settable(t1, 4, ph1);
  settable(t2, 2, ph2);
  settable(t3, 8, ph3);
  settable(t4, 16, ph4);
  settable(t5, 16, ph5);

  getelem(t2, ct, v2);
  getelem(t5, ct, oph);

  if (phase1 == 2)
    incr(v2);

/*
  mod2(id2,v14);
  dbl(v14, v14);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

  add(v2, v14, v2);
  add(oph, v14, oph);

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
        rgpulse(0.5 * pw, zero, rof1, rof1);
        delay(2 * tau);
        decpower(pwxlvl180);
        decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
        rgpulse(2.0 * pw, zero, rof1, rof1);
        delay(2 * tau + 2 * POWER_DELAY);
        decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
        decpower(pwxlvl);
        rgpulse(1.5 * pw, two, rof1, rof1);
        zgradpulse(hsglvl, hsgt);
        delay(1e-3);
    }
    else if (null != 0.0)
    {
        rgpulse(pw, zero, rof1, rof1);
        delay(2 * tau);
        decpower(pwxlvl180);
        decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
        rgpulse(2.0 * pw, zero, rof1, rof1);
        delay(2 * tau + 2 * POWER_DELAY);
        decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
        decpower(pwxlvl);
        rgpulse(pw, two, rof1, rof1);
        if (satmode[1] == 'y')
	   satpulse(null,zero,rof1,rof1);
        else
	   delay(null);
    }

    rgpulse(pw, zero, rof1, rof1);
    delay(tau);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    rgpulse(2.0 * pw, zero, rof1, rof1);
    delay(tau + 2 * POWER_DELAY);
    decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
    decpower(pwxlvl);
    rgpulse(pw, t1, rof1, rof1);
    if (getflag("PFGflg"))
    {
      zgradpulse(hsglvl, 2 * hsgt);
      delay(1e-3);
    }
    decrgpulse(pwx, v2, rof1, 2.0e-6);
    if (d2 / 2 > 0.0)
      delay(d2 / 2 - (2 * pwx / PI) - pw - 4.0e-6);
    else
      delay(d2 / 2);
    rgpulse(2 * pw, zero, 2.0e-6, 2.0e-6);
    if (d2 / 2 > 0.0)
      delay(d2 / 2 - (2 * pwx / PI) - pw - 4.0e-6);
    else
      delay(d2 / 2);

    if (mult > 0.5)
    {
      delay(taug - POWER_DELAY);
      decpower(pwxlvl180r);
      decshaped_pulse(pwx180ref, pwx180r, zero, rof1, rof1);
      rgpulse(mult * pw, zero, rof1, rof1);
      delay(taug - mult * pw - 2 * rof1 + POWER_DELAY);
      decshaped_pulse(pwx180ref, pwx180r, zero, rof1, rof1);
      decpower(pwxlvl);
    }
    decrgpulse(pwx, t4, 2.0e-6, rof1);
    if (getflag("PFGflg"))
    {
      zgradpulse(ZZgsign*0.6 * hsglvl, 1.2 * hsgt);
      delay(1e-3);
    }
    rgpulse(pw, t3, rof1, rof1);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
    decpower(dpwr);
    delay(tau - 2 * rof1 - (2*pw/PI));
    rgpulse(2 * pw, zero, rof1, rof2);
    decpower(pwxlvl);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    decpower(dpwr);
    delay(tau);
  status(C);
}
