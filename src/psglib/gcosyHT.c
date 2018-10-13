// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*  gcosyHT.c - Hadamard gCOSY, Eriks Kupce, Oxford, 12.03.07   

    Requires a frequency list of resonances from the parameter htfrq1.
    The list is generated, for example, by recording a 1D H1 spectrum,
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
    pshape - shape used for Hadamard encoding, typically gaus90, square90 or
             sinc90.
    repflg - set repflg = 'n' to suppress Pbox reports, repflg = 'h' prints
             Hadamard matrix only.
    htss1  - stepsize for Hadamard encoding pulses. This parameter is adjusted
             by looking at the maximum phase increments in Hadamard encoding
             pulses, e.g. F1_2.RF. If unsure, set htss1 = 0 to disable it.
    compH  - H1 amplifier compression factor.           
*/ 

#include <standard.h>

pulsesequence()
{
  int     icosel, iphase;
  char    pshape[MAXSTR], xptype[MAXSTR], sspul[MAXSTR];
  double  rg1	= 2.0e-6,
          compH	= getval("compH"),	/* H-1 amplifier compression factor */
	  gt0    = getval("gt0"),	/* gradient pulse preceding d1 */
	  gt2    = getval("gt2"),	/* coherence gradient pulse  */
	  gzlvl0 = getval("gzlvl0"),	/* gradient level for gt0 */
	  gzlvl2 = getval("gzlvl2"),	/* gradient level for gt2 */
	  gstab  = getval("gstab");	/* delay for gradient recovery */
  shape   hdx;

  getstr("pshape", pshape);
  getstr("sspul", sspul);

  iphase = (int) (0.5 + getval("phase"));
  icosel=1;    /* Default to N-type experiment */
  strcpy(xptype, "N-type");
  if (iphase == 2) 
  { 
    icosel=-1;
    strcpy(xptype, "P-type"); 
  }
  if (FIRST_FID) fprintf(stdout,"%s COSY\n", xptype);


  /* Make HADAMARD encoding waveforms */
  
    hdx = pboxHT_F1e(pshape, pw*compH, tpwr);

 /* THE PULSE PROGRAMM STARTS HERE */

  status(A);

    delay(5.0e-5);
    zgradpulse(gzlvl0,gt0);
    if (sspul[0] == 'y')
    {
      rgpulse(pw,zero,rof1,rof1);
      zgradpulse(gzlvl0,gt0);
    }

    pre_sat();
      
    if (getflag("wet"))
      wet4(zero,one);
      
    delay(1.0e-5);

  status(B);

    pbox_pulse(&hdx, oph, rg1, rg1);
    obspower(tpwr);
    zgradpulse(gzlvl2,gt2);
    delay(gstab);    
    rgpulse(pw,oph,rg1,rof2);
    zgradpulse(gzlvl2*icosel,gt2);
    delay(gstab);
      
  status(C);
}


