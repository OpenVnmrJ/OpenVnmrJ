/* gHSQCAD - Gradient Selected phase-sensitive HSQC
		Adiabatic version
        Selection based on dpfgse;  IMPRESS option included
        Multiplicity editing is OFF

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
                mixT    :       TOCSY spinlock mixing time
                slpatT  :       TOCSY spinlock pattern (mlev17c,mlev17,dipsi2,dipsi3)
                trim    :       trim pulse preceeding spinlock
                slpwrT  :       spin-lock power level
                slpwT   :       90 deg pulse width for spinlock
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

************************************************************************
****NOTE:  v20,v21,v22,v23 and v24 are used by Hardware Loop and reserved ***
************************************************************************

KrishK  -       Last revision   : June 1997
KrishK  -       Revised         : July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
JohnR - bsgHSQCADTOXY converted from bsgHSQCAD : Feb 2014
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/

#include <standard.h>
#include <chempack.h>

extern int dps_flag;

static int ph1[4]  = {1,1,3,3},
           ph2[2]  = {0,2},
           ph3[8]  = {0,0,0,0,2,2,2,2},
           ph4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           ph5[16] = {1,3,3,1,3,1,1,3,3,1,1,3,1,3,3,1};

pulsesequence()

{

   double   gzlvlE = getval("gzlvlE"),
            gtE = getval("gtE"),
            EDratio = getval("EDratio"),
            gstab = getval("gstab"),
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
            hsglvl = getval("hsglvl"),
            hsgt = getval("hsgt"),
	    impress = getval("impress"),
            slpwT = getval("slpwT"),
            slpwrT = getval("slpwrT"),
            trim = getval("trim"),
            mixT = getval("mixT"),
	    evolcorr,
            tau;
   int      icosel,
            prgcycle = (int)(getval("prgcycle")+0.5),
	    phase1 = (int)(getval("phase")+0.5),
            iimphase = (int)(getval("imphase")+0.5);
   char	    pwx180ad[MAXSTR],
	    pwx180adR[MAXSTR],
            selpwxshpA[MAXSTR],
            selpwxshpB[MAXSTR],
	    slpatT[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",0.5);
        gzlvlE = syncGradLvl("gtE","gzlvlE",0.5);
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

  getstr("pwx180ad", pwx180ad);
  getstr("pwx180adR", pwx180adR);
  getstr("selpwxshpA", selpwxshpA);
  getstr("selpwxshpB", selpwxshpB);
  getstr("slpatT",slpatT);
  evolcorr=(4*pwx/PI)+2*pw+8.0e-6;
  tau = 1 / (4*(getval("j1xh")));
  icosel = 1;

   if (strcmp(slpatT,"mlev17c") &&
        strcmp(slpatT,"dipsi2") &&
        strcmp(slpatT,"dipsi3") &&
        strcmp(slpatT,"mlev17") &&
        strcmp(slpatT,"mlev16"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatT);

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

  if ((phase1 == 2) || (phase1 == 5))
    icosel = -1;

  assign(zero,v6);
  add(oph,v18,oph);
  add(oph,v19,oph);

  assign(two,v21);

/*
  mod2(id2, v14);
  dbl(v14,v14);
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

    if (getflag("nullflg"))
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
    rgpulse(pw, v1, rof1, rof1);
    zgradpulse(hsglvl, 2 * hsgt);
    delay(1e-3);
    decrgpulse(pwx, v2, rof1, 2.0e-6);

      delay(d2 / 2);
    rgpulse(2 * pw, zero, 2.0e-6, 2.0e-6);
      delay(d2 / 2);

      zgradpulse(gzlvlA,gtA);
      delay(gstab);
      decpower(selpwxlvlA);
      decshaped_pulse(selpwxshpA, selpwxA, zero, rof1, rof1);
      decpower(pwxlvl);
      zgradpulse(gzlvlA,gtA);
      delay(gstab + evolcorr);

    zgradpulse(gzlvlE, gtE/2);
    delay(100e-6);

      zgradpulse(gzlvlB,gtB);
      delay(gstab);
      decpower(selpwxlvlB);
      decshaped_pulse(selpwxshpB, selpwxB, zero, rof1, rof1);
      decpower(pwxlvl);
      zgradpulse(gzlvlB,gtB);
      delay(100e-6);

    zgradpulse(-gzlvlE, gtE/2);
    delay(gstab);

    decrgpulse(pwx, v4, 2.0e-6, rof1);
    zgradpulse(0.6 * hsglvl, 1.2 * hsgt);
    delay(1e-3);

    rgpulse(pw, v3, rof1, rof1);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180adR, pwx180, zero, rof1, rof1);
    decpower(dpwr);
    delay(tau - (2 * pw / PI) - 2*rof1);

    rgpulse(2 * pw, zero, rof1, rof2);
    decpower(pwxlvl180);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    delay(tau);
    obspower(slpwrT);

     if (mixT > 0.0)
     {
        rgpulse(trim,zero,0.0,0.0);
        if (dps_flag)
          rgpulse(mixT,v21,0.0,0.0);
        else
          SpinLock(slpatT,mixT,slpwT,v21);
     }

      obspower(tpwr);
      delay(gtE/2 + gstab + 2*GRADIENT_DELAY-rof1);
      rgpulse(2*pw,zero,rof1,rof2);
    decpower(dpwr);
    zgradpulse(icosel * 2.0*gzlvlE/EDratio, gtE/2.0);
      delay(gstab);

  status(C);
}
