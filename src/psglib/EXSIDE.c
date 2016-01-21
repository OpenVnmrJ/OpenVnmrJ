// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* EXSIDE - Gradient Selected phase-sensitive HSQC
		Adiabatic version

        Features included:
                F1 Axial Displacement

        Paramters:
                sspul :         selects magnetization randomization option
                nullflg:        selects TANGO-Gradient option
                hsglvl:         Homospoil gradient level (DAC units)
                hsgt    :       Homospoil gradient time
                gzlvlE  :       encoding Gradient level
                gtE     :       encoding gradient time
                EDratio :       Encode/Decode ratio
                gstab   :       recovery delay
                j1xh    :       One-bond XH coupling constant

krish krishnamurthy Aug. 95

[REF: V.V. Krishnamurthy, JMR, A121, 33-41 (1996)]

KrishK  -       Revised         : Oct. 2004

*/

#include <standard.h>
#include <chempack.h>

static int      ph4[8]  =  {0,0,0,0,2,2,2,2},
                ph2[2]  =  {0,2},
                ph1[4]  =  {0,0,2,2},
                ph5[8]  =  {0,2,2,0,2,0,0,2};

pulsesequence()

{

   double   selpwrA = getval("selpwrA"),
            selpwA = getval("selpwA"),
            gzlvlA = getval("gzlvlA"),
            gtA = getval("gtA"),
            selpwrB = getval("selpwrB"),
            selpwB = getval("selpwB"),
            gzlvlB = getval("gzlvlB"),
            gtB = getval("gtB"),
   	    gzlvlE = getval("gzlvlE"),
            gtE = getval("gtE"),
            EDratio = getval("EDratio"),
            gstab = getval("gstab"),
	    pwx180 = getval("pwx180"),
	    pwxlvl180 = getval("pwxlvl180"),
            hsglvl = getval("hsglvl"),
            hsgt = getval("hsgt"),
            tau,
	    evolcorr,
	    jdly,
	    bird;
   int      icosel,
	    ijscale,
            prgcycle = (int)(getval("prgcycle")+0.5),
	    phase1=(int)(getval("phase")+0.5);
   char     selshapeA[MAXSTR],
            selshapeB[MAXSTR],
   	    pwx180ad[MAXSTR],
	    pwx180adR[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",0.5);
        gzlvlE = syncGradLvl("gtE","gzlvlE",0.5);
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

  getstr("selshapeA",selshapeA);
  getstr("selshapeB",selshapeB);
  getstr("pwx180ad", pwx180ad);
  getstr("pwx180adR", pwx180adR);
  evolcorr=(4*pwx/PI)+2*pw+8.0e-6;
  bird = 1 / (4*(getval("j1xh")));
  tau = 1/(4*(getval("jnxh")));
  ijscale=(int)(getval("jscale") + 0.5);
  jdly=(d2*ijscale);

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

   settable(t1,4,ph1);
   settable(t2,2,ph2);
   settable(t4,8,ph4);
   settable(t5,8,ph5);

   getelem(t1,v17,v1);
   getelem(t2,v17,v2);
   getelem(t4,v17,v4);
   getelem(t5,v17,oph);

  add(oph,v18,oph);
  add(oph,v19,oph);

  if ((phase1 == 2) || (phase1 == 5))
    icosel = -1;

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

    if (getflag("nullflg"))
    {
      rgpulse(0.5 * pw, zero, rof1, rof1);
      delay(2 * bird);
      decpower(pwxlvl180);
      decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
      rgpulse(2.0 * pw, zero, rof1, rof1);
      delay(2 * bird + 2 * POWER_DELAY);
      decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
      decpower(pwxlvl);
      rgpulse(1.5 * pw, zero, rof1, rof1);
      zgradpulse(hsglvl, hsgt);
      delay(1e-3);
    }

    rgpulse(pw, v1, rof1, rof1);
    delay(tau/2 - gtA - gstab);
    delay(jdly/4);

        zgradpulse(gzlvlA,gtA);
        delay(gstab);
        obspower(selpwrA);
        decpower(pwxlvl180);
        decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
        shaped_pulse(selshapeA,selpwA,zero,rof1,rof1);
        zgradpulse(gzlvlA,gtA);
        delay(gstab);

    delay(tau/2 - gtA - gstab);
    delay(jdly/2);
    delay(tau/2 - gtB - gstab);

        zgradpulse(gzlvlB,gtB);
        delay(gstab);
        obspower(selpwrB);
        decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
        shaped_pulse(selshapeB,selpwB,two,rof1,rof1);
	obspower(tpwr);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);
	decpower(pwxlvl);

    delay(jdly/4);
    delay(tau/2 - gtB - gstab);

    rgpulse(pw, one, rof1, rof1);
    zgradpulse(hsglvl, 2 * hsgt);
    delay(1e-3);
    decrgpulse(pwx, v2, rof1, 2.0e-6);

      delay(d2/2);
    rgpulse(2 * pw, zero, 2.0e-6, 2.0e-6);
      delay(d2/2);

    delay(gtE + gstab + 2*GRADIENT_DELAY - POWER_DELAY);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    zgradpulse(gzlvlE, gtE);
    delay(gstab + POWER_DELAY + evolcorr);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    decpower(pwxlvl);

    decrgpulse(pwx, v4, 2.0e-6, rof1);
    zgradpulse(-0.6 * hsglvl, 1.2 * hsgt);
    delay(1e-3);
    rgpulse(pw, zero, rof1, rof1);

    zgradpulse(icosel * 2.0*gzlvlE/EDratio, gtE/2.0);
    delay(gstab);

    delay(tau - gtA - 2*gstab - gtE/2);
        zgradpulse(gzlvlA,gtA);
        delay(gstab);
        obspower(selpwrA);
        decpower(pwxlvl180);
        decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
        shaped_pulse(selshapeA,selpwA,zero,rof1,rof1);
        zgradpulse(gzlvlA,gtA);
        delay(gstab);

    delay(tau - gtA - gstab);

    decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);

        decpower(dpwr);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);
        obspower(selpwrB);
        shaped_pulse(selshapeB,selpwB,two,rof1,rof2);
        obspower(tpwr);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);

  status(C);
}
