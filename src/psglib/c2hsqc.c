// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* c2hsqc - phase-sensitive C2HSQC
		Adiabatic/BIP Version for both nuclei

        Features included:
                F1 Axial Displacement

        Paramters:
                sspul :         selects magnetization randomization option
                nullflg:        selects TANGO-Gradient option
                hsglvl:         Homospoil gradient level (DAC units)
                hsgt    :       Homospoil gradient time
                j1xh    :       One-bond XH coupling constant

KrishK  -       Last revision   : June 1997
KrishK  -       Revised         : July 2004
haitao  -	Revised		: March 2006
KrishK	-	Includes purge option : Jan 2008
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***
KrishK  -	corrected the decoupler pulse shapes for mult=0 : March 2015
*/

#include <standard.h>
#include <chempack.h>

static int ph1[4]  = {1,1,3,3},
           ph2[2]  = {0,2,},
           ph3[8]  = {0,0,0,0,2,2,2,2},
           ph4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           ph5[16] = {1,3,3,1,3,1,1,3,3,1,1,3,1,3,3,1};

pulsesequence()

{

   double   mult = getval("mult"),
	    pwx180 = getval("pwx180"),
	    pwxlvl180 = getval("pwxlvl180"),
	    pwx180r = getval("pwx180r"),
	    pwxlvl180r = getval("pwxlvl180r"),
            tpwr180 = getval("tpwr180"),
            pw180 = getval("pw180"),
            hsglvl = getval("hsglvl"),
            hsgt = getval("hsgt"),
            gzlvl5 = getval("gzlvl5"),
            gt5 = getval("gt5"),
            tau,
            taug;
   int      phase1 = (int)(getval("phase")+0.5),
            prgcycle = (int)(getval("prgcycle")+0.5),
            ZZgsign;
   char	    pwx180ad[MAXSTR],
	    pwx180ref[MAXSTR],
	    pw180ad[MAXSTR],
	    bipflg[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gt5 = syncGradTime("gt5","gzlvl5",1.0);
        gzlvl5 = syncGradLvl("gt5","gzlvl5",1.0);

  getstr("pwx180ad", pwx180ad);
  getstr("pwx180ref", pwx180ref);
  getstr("pw180ad", pw180ad);

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

  tau = 1 / (4*(getval("j1xh")));

  if (mult > 0.5)
    taug = 2*tau + getval("tauC");
  else
    taug = 50e-6;

  ZZgsign = -1;
  if( mult > 1.5) ZZgsign = 1;

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

  if ((phase1 == 2) || (phase1 == 5))
    incr(v2);

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
/*
  mod2(id2, v14);
  dbl(v14,v14);
*/

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

   obspower(tpwr);
   decpower(pwxlvl180);

  status(B);

    if (getflag("nullflg"))
    {
      rgpulse(0.5 * pw, zero, rof1, rof1);
      obspower(tpwr180);
      delay(tau);
      simshaped_pulse(pw180ad,pwx180ad,pw180,pwx180,zero,zero,rof1,rof1);
      delay(tau+POWER_DELAY+rof1);
      delay(tau+POWER_DELAY+rof1);
      simshaped_pulse(pw180ad,pwx180ad,pw180,pwx180,zero,zero,rof1,rof1);
      obspower(tpwr);
      delay(tau);
      rgpulse(0.5 * pw, zero, rof1, rof1);
      zgradpulse(hsglvl, hsgt);
      delay(1e-3);
    }

    if (getflag("cpmgflg"))
    {
       rgpulse(pw, v6, rof1, 0.0);
       cpmg(v6, v15);
    }
    else
       rgpulse(pw, v6, rof1, rof1);
    obspower(tpwr180);
    shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
    delay(tau+2.0*POWER_DELAY+2.0*rof1);
    simshaped_pulse(pw180ad,pwx180ad,pw180,pwx180,zero,zero,rof1,rof1);
    delay(tau);
    obspower(tpwr);
    rgpulse(pw, v1, rof1, rof1);
    zgradpulse(hsglvl, 2 * hsgt);
    decpower(pwxlvl);
    obspower(tpwr180);
    delay(1e-3);
    decrgpulse(pwx, v2, rof1, 2.0e-6);

    delay(d2 / 2);
    shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
    delay(d2 / 2);

    delay(taug - POWER_DELAY);

    if (mult < 1.5) 
    {
    	decpower(pwxlvl180);
    	decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
        delay(4*pwx/PI + 4.0e-6 + taug + WFG_START_DELAY + pw180 +WFG_STOP_DELAY + 2*rof1);
	decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    }
    else
    {
	decpower(pwxlvl180r);
	decshaped_pulse(pwx180ref, pwx180r, zero, rof1, rof1);
	shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
	delay(4*pwx/PI + 4.0e-6 + POWER_DELAY + taug);
	decshaped_pulse(pwx180ref, pwx180r, zero, rof1, rof1);
    }
    decpower(pwxlvl);

    decrgpulse(pwx, v4, 2.0e-6, rof1);
    obspower(tpwr);
    decpower(pwxlvl180);
    zgradpulse(ZZgsign * 0.6 * hsglvl, 1.2 * hsgt);
    delay(1e-3);

    rgpulse(pw, v3, rof1, rof1);
    obspower(tpwr180);
    shaped_pulse(pw180ad,pw180,zero,rof1,rof1);
    zgradpulse(gzlvl5,gt5);
    delay(tau-gt5+2.0*POWER_DELAY);
    simshaped_pulse(pw180ad,pwx180ad,pw180,pwx180,zero,zero,rof1,rof2);
    zgradpulse(gzlvl5, gt5);
    delay(tau-gt5);
    decpower(dpwr);

  status(C);
}
