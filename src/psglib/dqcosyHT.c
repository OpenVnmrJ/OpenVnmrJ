// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*  dqcosyHT.c - Hadamard DQ-COSY experiment, Eriks Kupce, Oxford, 12.03.07  
                  based on gDQCOSY.c 

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


    DQ COSY Paramters:
    
                sspul :         y - selects magnetization randomization option
                hsglvl:         Homospoil gradient level (DAC units)
                hsgt    :       Homospoil gradient time
                gzlvl1  :       Encoding gradient level
                gzlvl2 	:	Decoding gradient level
                gt1     :       Gradient time
                gstab   :       Recovery delay
                pw      :       First and second pulse widths
                d1      :       relaxation delay
*/


#include <standard.h>

static int 	ph1[8] = {0, 2, 0, 2, 1, 3, 1, 3},
		ph2[8] = {0, 0, 2, 2, 1, 1, 3, 3},
		ph3[8] = {0, 2, 0, 2, 1, 3, 1, 3};

void pulsesequence()
{
	double	gzlvl0 = getval("gzlvl0"),
		gzlvl1 = getval("gzlvl1"),
		gzlvl2 = getval("gzlvl2"),
		gt0 = getval("gt0"),
		gt1 = getval("gt1"),
		gstab = getval("gstab"),
		compH = getval("compH");
	char	sspul[MAXSTR], pshape[MAXSTR];
	int	iphase;
        shape  hdx;

	getstr("sspul",sspul);
	iphase = (int)(getval("phase")+0.5);

  /* Make HADAMARD encoding waveforms */
  
        getstr("pshape", pshape);
        hdx = pboxHT_F1e(pshape, pw*compH, tpwr);

  /* Setup Phase Cycling */

	settable(t1,8,ph1);
	settable(t2,8,ph2);
	settable(t3,8,ph3);

	getelem(t1,ct,v1);
	getelem(t2,ct,v2);
	getelem(t3,ct,oph);

	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);
		add(v1,v10,v1);
		add(oph,v10,oph);

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

        obspower(tpwr);
      
  status(B);

       pbox_pulse(&hdx, v1, rof1, 2.0e-6);
       obspower(tpwr);

       rgpulse(pw, v2, 2.0e-6, rof1);
       delay(gt1 + gstab + 2*GRADIENT_DELAY);
       rgpulse(2*pw, v2, rof1, rof1);
       zgradpulse(-gzlvl1,gt1);
       delay(gstab);
       rgpulse(pw, v2, rof1, rof2);
       zgradpulse(gzlvl2,gt1);
       delay(gstab - 2*GRADIENT_DELAY);

  status(C);
}
