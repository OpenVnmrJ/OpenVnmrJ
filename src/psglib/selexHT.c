// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*
 */
/*  selexHT.c - test sequence for Hadamard excitation, Eriks Kupce, Oxford, 10.05.04

    Requires a frequency list of resonances from the parameter htfrq1.
    The list is generated, for example, by recording a 1D-H1 spectrum,
    and using the editht macro and popup to interactively select frequencies.
    The multiplet structure can be smoothed out by setting lb = htbw1 or
    slightly less.

    parameters:
    ==========
    Hdim - Hadamard dimension. Flag to switch between experiments in F1 (Hdim = 1)
           F2 (Hdim = 2), F3 (Hdim = 3) or HT (Hdim = 0, arrayed experiments;
           ixHT must be arrayed between 1 and niHT).

    parameters for Hdim = 1:
    ------------------------
    htbw1  - excitation bandwidth. For pshape = 'gaus180' good numbers are
             90, 45, 30 or 20 Hz.
    ni     - number of increments. Must be a power of 2. Other allowed values
             are n*12, n*20 and n*28, where n is a power of 2.
    nimax  - sets limit to ni to prevent unrealistic experiments.
    htofs1 - ni offset. Sets the number of increments to be omitted. Typical
            value is niofs = 1.
    pshape - shape used for Hadamard encoding, typically gaus180, square180,
             sinc180.
    httype - excitation type: 'i' (inversion), 's' (sequential inversion),
            'e' (excitation) and 'r' (refocusing). The pshape must be set
             accordingly.
    bscor  - Bloch-Siegert correction for Hadamard encoding, typically set
             bscor = 'y'.
    repflg - set repflg = 'n' to suppress Pbox reports, repflg = 'h' prints
             Hadamard matrix only.
    htss1  - stepsize for Hadamard encoding pulses. This parameter is adjusted
             by looking at the maximum phase increments in Hadamard encoding
             pulses, e.g. F1_2.RF. If unsure, set htss1 = 0 to disable it.
    compH  - H-1 amplifier compression factor.
    htcal1 - allows calibration of selective pulses. Set mix=0, htcal1=ni,
             ni=0 and array htpwr1. If ni=12 (and others that are not a power
             of two) use htcal1=8. Adjust compH so that pulse power matches the
             optimum power found from the calibration.

    Add presat and wet options.  Modify to remove higher
    dimensions (only F1 is supported in VnmrJ 2.1A).
       satmode  - flag for presat, usually 'ynn'
       satdly   - saturation delay for presaturation
       satpwr   - saturation power
       satfrq   - saturation frequency
       wet      - flag 'y' turns wet sequence on
                       -Daina Avizonis  4/4/05
*/

#include <standard.h>

void pulsesequence()
{
  char    pshape[MAXSTR], httype[MAXSTR];
  int     Hdim    = (int) (0.5 + getval("Hdim")); /* dimension for testing */
  double  rg1	  = 2.0e-6,
          compH   = getval("compH"),	/* compression factor for H-1 channel */
	  gt0     = getval("gt0"),	/* gradient pulse preceding d1 */
	  gt1     = getval("gt1"),	/* gradient pulse preceding mixing */
	  gt2     = getval("gt2"),	/* gradient pulse following mixing */
	  gzlvl0  = getval("gzlvl0"),	/* gradient level for gt1 */
	  gzlvl1  = getval("gzlvl1"),	/* gradient level for gt2 */
	  gzlvl2  = getval("gzlvl2"),	/* gradient level for gt3 */
	  gstab   = getval("gstab");	/* delay for gradient recovery */

  shape   hdx;

  getstr("pshape", pshape);
  getstr("httype", httype);
  add(one,oph,v1);
  
  /* MAKE PBOX SHAPES */

  if(Hdim == 0)
    hdx = pboxHT(pshape, pw*compH, tpwr, httype[0]); /* HADAMARD stuff */
  else /* if(Hdim == 1) */
    hdx = pboxHT_F1(pshape, pw*compH, tpwr, httype[0]);

/* Optional stuff to enable manual power calibration */

  if ((Hdim == 0) && (getval("htcal") > 0.5)) hdx.pwr = getval("htpwr");
  if ((Hdim == 1) && (getval("htcal1") > 0.5)) hdx.pwr = getval("htpwr1");

/* THE PULSE PROGRAMM STARTS HERE */

  status(A);
    delay(1.0e-4);
    zgradpulse(gzlvl0,gt0);

    pre_sat();
    if (getflag("wet"))
      wet4(zero,one);

    obspower(tpwr);
    delay(1.0e-5);

  status(B);

    if ((httype[0] == 'i') || (httype[0] == 's'))  /* inversion and sequential inversion */
    {
      pbox_pulse(&hdx, zero, rof1, rof1);
      zgradpulse(gzlvl2,gt2);
      obspower(tpwr);
      delay(gstab);
      rgpulse(pw,oph,rg1,rof2);
    }
    else if (httype[0] == 'r')              /* gradient echo */
    {
      obspower(tpwr);
      rgpulse(pw,oph,rg1,rg1);
      zgradpulse(gzlvl1,gt1);
      delay(gstab);
      pbox_pulse(&hdx, oph, rg1, rof2);
      zgradpulse(gzlvl1,gt1);
      delay(gstab);
    }
    else /* if (httype[0] == 'e') */        /* selective excitation */
      pbox_pulse(&hdx, oph, rof1, rof1);

  status(C);
}
