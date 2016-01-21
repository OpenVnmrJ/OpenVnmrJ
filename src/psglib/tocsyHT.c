// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*
 */
/*  tocsyHT.c - Hadamard TOCSY, Eriks Kupce, Oxford, 12.03.03   

    Requires a frequency list of resonances from the parameter htfrq1.
    The list is generated, for example, by recording a 1D-H1 spectrum,
    and using the editht macro and popup to interactively select frequencies.
    The multiplet structure can be smoothed out by setting lb = htbw1 or
    slightly less.

    parameters:
    ==========
    htfrq1 - list of frequencies used for Hadamard encoding.
    htbw1  - excitation bandwidth. For pshape = 'gaus180' good numbers are 
             90, 45, 30 or 20 Hz.
    ni     - number of increments. Must be a power of 2. Other allowed values 
             are n*12, n*20 and n*28, where n is a power of 2.
    htofs1 - ni offset. Sets the number of increments to be omitted. Typical
             value is htofs1 = 1.
    pshape - shape used for Hadamard encoding, typically gaus180, square180,
             sinc180.
    httype - Hadamard encoding type. Allowed values are 'i' (inversion pulses
             are used for longitudinal encoding), 'e' and 'r' (excitation and
             refocusing pulses are used for transverse encoding), and 'd' for 
             DPFGSE encoding. Note that pshape needs be chosen accordingly or
             can be set automatically by the HTchoice macro.
    bscor -  Bloch-Siegert correction for Hadamard encoding, typically set 
             bscor = 'y'.
    repflg - set repflg = 'n' to suppress Pbox reports, repflg = 'h' prints
             Hadamard matrix only.
    htss1  - stepsize for Hadamard encoding pulses. This parameter is adjusted
             by looking at the maximum phase increments in Hadamard encoding
             pulses, e.g. F1_2.RF. If unsure, set htss1 = 0 to disable it.
    compH  - H1 amplifier compression factor.           
    mix    - mixing time.
    mixpat - mixing pattern, as in /vnmr/wavelib/mixing.    
    htcal1 - allows calibration of selective pulses. Set mix=0, htcal1=ni, 
             ni=0 and array htpwr1. If ni=12 (and others that are not a power
             of two) use htcal1=8. Adjust compH so that pulse power matches
             the optimum power found from the calibration.

    Added presat and wet options (Daina Avizonis  4/4/05)
       satmode  - flag for presat, usually 'ynn' in H2O/D2O
       satdly	- saturation delay for presaturation
       satpwr	- saturation power
       satfrq	- saturation frequency
       wet	- flag 'y' turns wet sequence on                      
*/ 

#include <standard.h>

static shape  hhmix, ref180;

pulsesequence()
{
  char    mixpat[MAXSTR], pshape[MAXSTR], httype[MAXSTR], sspul[MAXSTR];
  double  rg1	= 2.0e-6,
          mix	= getval("mix"),	/* mixing time */
	  mixpwr = getval("mixpwr"),	/* mixing pwr */
	  compH  = getval("compH"),
	  gt0    = getval("gt0"),	/* gradient pulse in sspul */
	  gt2    = getval("gt2"),	/* gradient pulse preceding mixing */
	  gzlvl0 = getval("gzlvl0"),	/* gradient level for gt0 */
	  gzlvl2 = getval("gzlvl2"),	/* gradient level for gt2 */
	  gstab  = getval("gstab");	/* delay for gradient recovery */
  shape   hdx;
  void    setphases();

  getstr("sspul", sspul);
  getstr("pshape", pshape);
  getstr("httype", httype);
  getstr("mixpat", mixpat);
  setlimit("mixpwr", mixpwr, 48.0);

  (void) setphases();
  if (httype[0] == 'i')
    assign(zero,v2);

  /* MAKE PBOX SHAPES */

  if (FIRST_FID)
    hhmix = pbox_mix("HHmix", mixpat, mixpwr, pw*compH, tpwr);

  /* HADAMARD stuff */
  if(httype[0] == 'e')                           /* excitation */
    hdx = pboxHT_F1e(pshape, pw*compH, tpwr);
  else if(httype[0] == 'r')                      /* refocusing */
    hdx = pboxHT_F1r(pshape, pw*compH, tpwr);
  else if(httype[0] == 'd')                      /* DPFGSE */
  {
    hdx = pboxHT_F1r(pshape, pw*compH, tpwr);
    if (FIRST_FID)
      ref180 = hdx;
  }
  else /* httype[0] == 'i' */                    /* inversion */
    hdx = pboxHT_F1i(pshape, pw*compH, tpwr);

  if (getval("htcal1") > 0.5)          /* Optional fine power calibration */
    hdx.pwr += getval("htpwr1");

 /* THE PULSE PROGRAMM STARTS HERE */

  status(A);


    delay(5.0e-5);
    zgradpulse(gzlvl0,gt0);
    if (sspul[A] == 'y')
    {
      rgpulse(pw,zero,rof1,rof1);
      zgradpulse(gzlvl0,gt0);
    }

    pre_sat();
      
    if (getflag("wet"))
      wet4(zero,one);
      
    delay(1.0e-5);

    obspower(tpwr);
    delay(1.0e-5);

  status(B);

    if (httype[0] == 'i')           /* longitudinal encoding */
    {
      ifzero(v1);
        delay(2.0*(pw+rg1));
        zgradpulse(gzlvl2,gt2);
        delay(gstab);
      elsenz(v1);
        rgpulse(2.0*pw,v3,rg1,rg1);
        zgradpulse(gzlvl2,gt2);
        delay(gstab);
      endif(v1);

      pbox_pulse(&hdx, zero, rg1, rg1);
      zgradpulse(gzlvl2,gt2);
      delay(gstab);
    }
    else                            /* transverse encoding */
    {
      if (httype[0] == 'e')
        pbox_pulse(&hdx, oph, rg1, rg1);
      else
      {
        rgpulse(pw,oph,rg1,rg1);
        if (httype[0] == 'd')       /* DPFGSE */
        {
          zgradpulse(2.0*gzlvl2,gt2);
          delay(gstab);
          pbox_pulse(&ref180, oph, rg1, rg1);
          zgradpulse(2.0*gzlvl2,gt2);
          delay(gstab);
        }
        zgradpulse(gzlvl2,gt2);
        delay(gstab);
        pbox_pulse(&hdx, v2, rg1, rof2);
        zgradpulse(gzlvl2,gt2);
        delay(gstab - POWER_DELAY);
      }
    }

    if (mix)
      pbox_spinlock(&hhmix, mix, v2);

    if (httype[0] == 'i')
    {
      zgradpulse(0.87*gzlvl2,gt2);
      delay(gstab);
      obspower(tpwr);
      txphase(v3);
      rgpulse(pw,v3,rg1,rof2);
    }

  status(C);
}

/* ------------------------ Utility functions ------------------------- */

void setphases()
{
  mod4(ct, oph);            /* 0123 */
  mod2(ct,v1);
  dbl(v1,v1);               /* 0202 */
  add(v1,oph,v3);           /* 0321 */
  add(one,v3,v2);           /* 1032 */
  add(two,oph,oph);
}
