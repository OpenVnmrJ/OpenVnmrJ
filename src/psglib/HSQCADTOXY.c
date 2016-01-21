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
                mixT    :       TOCSY spinlock mixing time
                slpatT  :       TOCSY spinlock pattern (mlev17c,mlev17,dipsi2,dipsi3)
                trim    :       trim pulse preceeding spinlock
                slpwrT  :       spin-lock power level
                slpwT   :       90 deg pulse width for spinlock
                j1xh    :       One-bond XH coupling constant

************************************************************************
****NOTE:  v20,v21,v22,v23 and v24 are used by Hardware Loop and reserved ***
************************************************************************

KrishK  -       Last revision   : June 1997
KrishK  -       Revised         : July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***

*/

#include <standard.h>
#include <chempack.h>

extern int dps_flag;

static int      ph1[4] = {1,1,3,3},
                ph2[2] = {0,2},
                ph3[8] = {0,0,0,0,2,2,2,2},
                ph4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
                ph5[16] = {1,3,3,1,3,1,1,3,3,1,1,3,1,3,3,1};

pulsesequence()

{

   double   hsglvl = getval("hsglvl"),
            hsgt = getval("hsgt"),
            tau = 1/(4*(getval("j1xh"))),
	    pwx180 = getval("pwx180"),
	    pwxlvl180 = getval("pwxlvl180"),
            slpwT = getval("slpwT"),
            slpwrT = getval("slpwrT"),
            trim = getval("trim"),
            mixT = getval("mixT"),
            mult = getval("mult"),
	    evolcorr,
            null = getval("null");
  int	    phase1 = (int)(getval("phase")+0.5),
            prgcycle = (int)(getval("prgcycle")+0.5);

  char      pwx180ad[MAXSTR],
     	    slpatT[MAXSTR],
            pwx180adR[MAXSTR];

  getstr("pwx180ad", pwx180ad);
  getstr("pwx180adR", pwx180adR);
  getstr("slpatT",slpatT);
  evolcorr=(4*pwx/PI)+2*pw+8.0e-6;

   if (strcmp(slpatT,"mlev17c") &&
        strcmp(slpatT,"dipsi2") &&
        strcmp(slpatT,"dipsi3") &&
        strcmp(slpatT,"mlev17") &&
        strcmp(slpatT,"mlev16"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatT);


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
  settable(t2, 2, ph2);
  settable(t3, 8, ph3);
  settable(t4, 16, ph4);
  settable(t5, 16, ph5);

  getelem(t1, v17, v1);
  getelem(t3, v17, v3);
  getelem(t4, v17, v4);
  getelem(t2, v17, v2);
  getelem(t5, v17, oph);

  assign(zero,v6);
  add(oph,v18,oph);
  add(oph,v19,oph);

   assign(two,v21);

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
        {
	   if (getflag("slpsat"))
                shaped_satpulse("BIRDnull",null,zero);
           else
                satpulse(null,zero,rof1,rof1);
        }
	else
	   delay(null);
    }

    rgpulse(pw, v6, rof1, rof1);
    delay(tau);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    rgpulse(2.0 * pw, zero, rof1, rof1);
    delay(tau + 2 * POWER_DELAY);
    decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
    decpower(pwxlvl);
    rgpulse(pw, v1, rof1, rof1);
    if (getflag("PFGflg"))
    {
      zgradpulse(hsglvl, 2 * hsgt);
      delay(1e-3);
    }
    decrgpulse(pwx, v2, rof1, 2.0e-6);

        delay(d2/2);
        rgpulse(2*pw,zero,2.0e-6,2.0e-6);
        delay(d2/2);
        decpower(pwxlvl180);
        decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
        delay(2*POWER_DELAY + evolcorr);
        decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
        decpower(pwxlvl);

    decrgpulse(pwx, v4, 2.0e-6, rof1);
    if (getflag("PFGflg"))
    {
      zgradpulse(0.6 * hsglvl, 1.2 * hsgt);
      delay(1e-3);
    }
    rgpulse(pw, v3, rof1, rof1);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
    decpower(dpwr);
    delay(tau - (2 * pw / PI) - rof1);
    rgpulse(2 * pw, zero, rof1, rof1);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    obspower(slpwrT);
    decpower(dpwr);
    delay(tau - POWER_DELAY);

     if (mixT > 0.0)
     {
        rgpulse(trim,zero,0.0,0.0);
        if (dps_flag)
          rgpulse(mixT,v21,0.0,0.0);
        else
          SpinLock(slpatT,mixT,slpwT,v21);
     }

     if (mult > 0.5)
     {
      obspower(tpwr);
      delay(2*tau - POWER_DELAY - rof1);
      decpower(pwxlvl180);
      decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
      rgpulse(2.0 * pw, zero, rof1, rof2);
      delay(2*tau);
      decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
      decpower(dpwr);
     }
     else
        delay(rof2);


  status(C);
}
