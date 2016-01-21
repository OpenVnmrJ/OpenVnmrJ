// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*  gmqcosyHT.c - Hadamard DQ-COSY, Eriks Kupce, Oxford, 12.03.07  
                  based on gmqcosy.c 

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
        double rg1 = 2.0e-6,
	       compH = getval("compH"),
	       gt0=getval("gt0"),
	       gzlvl0=getval("gzlvl0"),
	       gt1=getval("gt1"),
	       gzlvl1=getval("gzlvl1"),
	       qlvl=getval("qlvl"),
	       grise=getval("grise"),
	       gstab=getval("gstab"),
	       taud2=getval("taud2"),
	       tau1=getval("tau1");
	int    icosel, iphase;
        char   pshape[MAXSTR], xptype[MAXSTR];
        shape  hdx;

  getstr("pshape", pshape);

  iphase = (int) (0.5 + getval("phase"));
  qlvl++ ; 
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

     status(A);
     
       delay(1.0e-4);
       zgradpulse(gzlvl0,gt0);

       pre_sat();
       if (getflag("wet"))
         wet4(zero,one);

       obspower(tpwr);
       delay(1.0e-5);

  status(B);

       pbox_pulse(&hdx, oph, rg1, rg1);
       obspower(tpwr);

       zgradpulse(gzlvl1,gt1);
       delay(grise); 
          
       rgpulse(pw, oph, rg1, rg1);
       delay(taud2);

       zgradpulse(gzlvl1,gt1);
       delay(grise+taud2); 

       rgpulse(pw,oph,rg1,rof2);
       delay(tau1);

       zgradpulse(gzlvl1*qlvl*(double)icosel,gt1);
       delay(grise+gstab); 

     status(C);
}
