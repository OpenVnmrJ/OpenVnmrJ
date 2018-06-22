// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* bsHSQCAD - band-selective Heteronuclear Single Quantum Coherence
	Adiabatic Version
	Selection based on dpfgse;  IMPRESS option included
	Multiplicity editing is OFF

        Paramters:
                sspul :         selects magnetization randomization option
                hsglvl:         Homospoil gradient level (DAC units)
                hsgt    :       Homospoil gradient time
                j1xh    :       One-bond XH coupling constant
                impress :       Selects impress option
                                0 - sets non-impress bandselective option
                                  uses selpwxA,selpwxlvlA,selpwxshpA for shape1
                                       selpwxB,selpwxlvlB,selpwxshpB for shape2
                                non-zero - Number bands in impress
                imphase :       1,2 - 2 band IMPRESS
                                1,2,3,4 - 3 or 4 band IMPRESS
                                1-4 uses imppw1, imppwr1,impshp1 for shape1
                                1 - uses imppw1,imppwr1,impshp1 for shape2
                                2 - uses imppw2,imppwr2,impshp2 for shape2
                                3 - uses imppw3,imppwr3,impshp3 for shape2
                                4 - user imppw4,imppwr4,impshp4 for shape2

KrishK  -       Last revision   : June 1997
KrishK  -       Revised         : July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/

#include <standard.h>
#include <chempack.h>

static shape mixsh;

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
	    mixR = getval("mixR"),
	    pwx180 = getval("pwx180"),
	    pwxlvl180 = getval("pwxlvl180"),
	    selpwxA = getval("selpwxA"),
	    selpwxlvlA = getval("selpwxlvlA"),
	    gzlvlA = getval("gzlvlA"),
	    gtA = getval("gtA"),
	    selpwxB = getval("selpwxB"),
	    selpwxlvlB = getval("selpwxlvlB"),
	    gzlvlB = getval("gzlvlB"),
	    gtB = getval("gtB"),
	    gstab = getval("gstab"),
            gzlvl1 = getval("gzlvl1"),
            gt1 = getval("gt1"),
	    impress = getval("impress"),
            tof = getval("tof"), tpos1, tpos2,
            slpwrR = getval("slpwrR"),
            spinlockR = getval("spinlockR"),
            tiltfactor = getval("tiltfactor"),
            gzlvlz = getval("gzlvlz"),
            gtz = getval("gtz"),
            null = getval("null");
  int	    phase1 = (int)(getval("phase")+0.5),
            prgcycle = (int)(getval("prgcycle")+0.5),
	    iimphase = (int)(getval("imphase")+0.5);
  char      pwx180ad[MAXSTR],
	    selpwxshpA[MAXSTR],
	    selpwxshpB[MAXSTR],
            pwx180adR[MAXSTR],
            satmode[MAXSTR],
            zfilt[MAXSTR],
            wet[MAXSTR];


  getstr("pwx180ad", pwx180ad);
  getstr("pwx180adR", pwx180adR);
  getstr("selpwxshpA", selpwxshpA);
  getstr("selpwxshpB", selpwxshpB);
  getstr("satmode", satmode);
  getstr("zfilt", zfilt);
  getstr("wet", wet);

  tpos1 = tof + (spinlockR/tiltfactor);
  tpos2 = tof - (spinlockR/tiltfactor);

  tau  = 1/(4*(getval("j1xh")));

  if (impress > 0.5)
  {
    selpwxA = getval("imppw1");
    selpwxlvlA = getval("imppwr1");
    getstr("impshp1", selpwxshpA);
    if (iimphase == 1)
    {
    	selpwxB = getval("imppw1");
	selpwxlvlB = getval("imppwr1");
	getstr("impshp1",selpwxshpB);
    }
    if (iimphase == 2)
    {
        selpwxB = getval("imppw2");
        selpwxlvlB = getval("imppwr2");
        getstr("impshp2",selpwxshpB);
    }
    if (iimphase == 3)
    {
        selpwxB = getval("imppw3");
        selpwxlvlB = getval("imppwr3");
        getstr("impshp3",selpwxshpB);
    }
    if (iimphase == 4)
    {
        selpwxB = getval("imppw4");
        selpwxlvlB = getval("imppwr4");
        getstr("impshp4",selpwxshpB);
    }
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

  if (phase1 == 2)
    incr(v2);

 /*  Make adiabatic roesy mixing shape */
     if(FIRST_FID)
       {
         opx("Pbox");
         pboxpar("steps", 1000.0);
         mixsh = pbox_shape("roesweep", "amwu", 0.0, 0.0, 0.0,0.0);
       }
   
/*
  mod2(id2,v14);
  dbl(v14, v14);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

  if (impress > 0.5)
  {
        if (getflag("fadflg"))
        {
                add(v2, v14, v2);
                add(oph, v14, oph);
        }
  }
  else
        {
                add(v2, v14, v2);
                add(oph, v14, oph);
        }


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

    if (getflag("cpmgflg"))
    {
       rgpulse(pw, v6, rof1, 0.0);
       cpmg(v6, v15);
    }
    else
       rgpulse(pw, v6, rof1, rof1);
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

      zgradpulse(gzlvlA,gtA);
      delay(gstab);
      decpower(selpwxlvlA);
      decshaped_pulse(selpwxshpA, selpwxA, zero, rof1, rof1);
      decpower(pwxlvl);
      zgradpulse(gzlvlA,gtA);
      delay(gstab);

      zgradpulse(gzlvlB,gtB);
      delay(gstab);
      decpower(selpwxlvlB);
      decshaped_pulse(selpwxshpB, selpwxB, zero, rof1, rof1);
      decpower(pwxlvl);
      zgradpulse(gzlvlB,gtB);
      delay(gstab);

    decrgpulse(pwx, t4, 2.0e-6, rof1);
    if (getflag("PFGflg"))
    {
      zgradpulse(0.6 * hsglvl, 1.2 * hsgt);
      delay(1e-3);
    }
    rgpulse(pw, t3, rof1, rof1);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
    decpower(dpwr);
    delay(tau);
    rgpulse(2 * pw, zero, rof1, rof1);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    decpower(dpwr);
    delay(tau);

    rgpulse(pw,three,rof1,rof1);
      
if (zfilt[0] == 'y')
         zgradpulse(gzlvlz,gtz);
      delay(gstab);
      obsoffset(tpos1);
      obspower(slpwrR);
      shapedpulse("roesweep",mixR/2.0,three,rof1,rof2);
      
      obsoffset(tpos2);
      shapedpulse("roesweep",mixR/2.0,three,rof1,rof2);
      obspower(tpwr);
      obsoffset(tof);
      if (zfilt[0] == 'y')
         zgradpulse(gzlvlz*0.62,gtz);
      delay(gstab);
      rgpulse(pw,one,rof1,rof2);

  status(C);
}
