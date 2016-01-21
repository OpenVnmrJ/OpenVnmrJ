// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/*
 */
/*  hsqcHT.c - 2D Hadamard HSQC, Eriks Kupce, Oxford, 12.03.03 

    Requires a frequency list of C13 resonances from the parameter htfrq1.
    The list is generated, for example, by recording a 1D-C13 spectrum,
    and using the editht macro and popup to interactively select frequencies.
    The multiplet structure can be smoothed out by setting lb = htbw1 or
    slightly less.

    parameters:
    ==========
    htfrq1 - list of frequencies used for Hadamard encoding.
    ni     - number of increments. Must be a power of 2. Other allowed values 
             are n*12, n*20 and n*28, where n is a power of 2.
    htofs1 - ni offset. Sets the number of increments to be omitted. Typical
             value is htofs1 = 1.
    htbw1  - excitation bandwidth. For pshape = 'gaus180' good numbers are 
             90, 45, 30 or 20 Hz.
    pshape - shape used for Hadamard encoding, typically gaus180, square180,
             sinc180.
    bscor  - Bloch-Siegert shift correction for Hadamard encoding, typically 
             set bscor = 'y'.
    repflg - set repflg = 'n' to suppress Pbox reports. repflg = 'h' prints
             Hadamard matrix only.
    htss1  - stepsize for Hadamard encoding pulses. This parameter is adjusted
             by looking at the maximum phase increments in Hadamard enconing
             pulses, e.g. F1_2.RF. If unsure, set htss1 = 0 to disable it.
    pwpat  - CH decoupling (.RF) pulse. Use inversion pulses of 1 - 1.5 ms and
             an appropriate supercycle. The total length must be the same as  
             that of the Hadamard encoding pulse (F1_1.RF). For example, 
             with 45 ms long pulse us a 40 step supercycle (e.g. t5,m8) and
             1.125 ms long adiabatic pulse:
             >Pbox CHdec -w "wurst2i 25k/1.125m" -sucyc t5,m8 -s 5.0 -p ... -l ...       
             They are generated on-the-fly by pboxHT_dec().
    htcal1 - allows calibration of selective pulses. Set htcal1=ni, ni=0, nt=2, 
             and array htpwr1. If ni=12 (and others that are not a power of two) 
             use htcal1=8. Set compH such that pulse power matches the optimum 
             power found from the calibration.
    zz       flag to use ZZ filter for better sensitivity
    ad180    flag to use adiabatic refocussing 
    mult     multiplicity editing options
             0 (all), 1 (CH-only), 2(CH2 negative)
*/

#include <standard.h>

static shape       ad180sh, Hdec;

pulsesequence()
{
   double          tau, rg1 = 2.0e-6,
                   jXH = getval("jXH"),
                   mult = getval("mult"),
                   gt0 = getval("gt0"),
                   gzlvl0 = getval("gzlvl0"),
                   gt1 = getval("gt1"),
                   gzlvl1 = getval("gzlvl1"),
                   gt2 = getval("gt2"),
                   gzlvl2 = getval("gzlvl2"),
                   gt3 = getval("gt3"),
                   gzlvl3 = getval("gzlvl3"),
                   Gratio = getval("Gratio"),
                   gstab = getval("gstab"),
                   compH = getval("compH"),
                   compX = getval("compX"),
                   Hpwr = getval("Hpwr"),
		   jevol = 0.25 / 140.0;    /* default for C-13 */

   char            pshape[MAXSTR], zz[MAXSTR], ad180[MAXSTR], sspul[MAXSTR];
   shape           hdx;                     /* HADAMARD stuff */
   void            setphases();

   getstr("pshape", pshape);              /* pulse shape as in wavelib */
   getstr("sspul", sspul);
   getstr("zz", zz);
   getstr("ad180", ad180);
   if (jXH > 0.0) jevol = 0.25 / jXH;
   
   tau = gt3 + gstab;
   if (mult > 0.5)
    tau = 2*jevol;

   setphases();

   /* MAKE PBOX SHAPES */

   if (FIRST_FID)
   {
     ad180sh = pbox_ad180("ad180", pwx, pwxlvl);    /* make adiabatic 180 */
     if (strcmp(pwpat,"Hdec") == 0)
     {
       /* make H decoupling shape */
       Hdec = pboxHT_dec(pwpat, pshape, jXH, 20.0, pw, compH, tpwr);
       Hpwr = Hdec.pwr;
     }
   }

   hdx = pboxHT_F1i(pshape, pwx*compX, pwxlvl);         /* HADAMARD stuff */
   if (getval("htcal1") > 0.5)           /* enable manual power calibration */
     hdx.pwr += getval("htpwr1");

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

   decoffset(dof);
   decpower(pwxlvl);
   obspower(tpwr);
   obsoffset(tof);
   delay(2.0e-5);
 
   status(B);

   rgpulse(pw, v2, rg1, rg1);
   delay(jevol - ad180sh.pw/2.0 - 3.0*rg1); /* +rg on both sides */
   decshaped_pulse(ad180sh.name, ad180sh.pw, zero, rg1, rg1);
   rgpulse(2.0*pw, zero, rg1, rg1);
   delay(jevol + ad180sh.pw/2.0 - rg1);
   rgpulse(pw, one, rg1, rg1);

   zgradpulse(-gzlvl2, gt2);
   delay(gstab);

   decpower(hdx.pwr);
   if (strcmp(pwpat,"") == 0)
     decshaped_pulse(hdx.name, hdx.pw, zero, rg1, rg1);
   else
   {
     obspower(Hpwr);
     simshaped_pulse(pwpat, hdx.name, hdx.pw, hdx.pw, zero, zero, rg1, rg1);
   }
   zgradpulse(gzlvl2*1.4, gt2);
   decpower(pwxlvl); obspower(tpwr);

   if((zz[A] == 'y') && (mult == 0)) 
   {
     ifzero(v1);
       decshaped_pulse(ad180sh.name, ad180sh.pw, zero, rg1, rg1);
     elsenz(v1);
       delay(0.5e-3);
     endif(v1);
   }
   else
   {
     delay(0.5e-3);
     decrgpulse(pwx,v1,rg1,rg1);
     if(ad180[A]=='y')
       decshaped_pulse(ad180sh.name, ad180sh.pw, zero, rg1, rg1);
     if(zz[A] == 'y')
       delay(tau);
     else
     {
       zgradpulse(gzlvl1,gt1);
       delay(tau - gt1 - 2*GRADIENT_DELAY);
     }
     if(ad180[A]=='y')
     {
       rgpulse(mult*pw, zero, rg1, rg1);
       decshaped_pulse(ad180sh.name, ad180sh.pw, zero, rg1, rg1);
     }
     else
       simpulse(mult*pw,2*pwx,zero,one,rg1,rg1);
     delay(tau); 
     decrgpulse(pwx,zero,rg1,rg1);
   }
   zgradpulse(gzlvl2/1.7, gt2);
   delay(gstab);

   rgpulse(pw, zero, rg1, rg1);
   delay(jevol - ad180sh.pw/2.0 - 3.0*rg1);  /* +rg on both sides */
   decshaped_pulse(ad180sh.name, ad180sh.pw, zero, rg1, rg1);
   rgpulse(2.0*pw, zero, rg1, rg1);

   if(zz[A] == 'y') 
   {
     delay(jevol + ad180sh.pw/2.0 - rg1);
     rgpulse(pw, one, rg1, rg1);
     zgradpulse(-gzlvl3, gt3);
     decpower(dpwr);
     delay(gstab);
     rgpulse(pw, three, rg1, rof2);
   }
   else
   {
     zgradpulse(2.0*gzlvl1/Gratio, gt1/2.0);
     decpower(dpwr);
     delay(jevol + ad180sh.pw/2.0 - rg1 - gt1/2.0 - 2*GRADIENT_DELAY - POWER_DELAY + rof2);
   }
   status(C);
}

/* ------------------------ Utility functions ------------------------- */

void setphases()
{
  mod2(ct, v1);  dbl(v1,v1);                  /* 0202 */
  hlv(ct,v2);    mod2(v2,v2); dbl(v2,v2);     /* 0022 */
  add(v1,v2,v3); mod4(v3,oph);                /* 0220 */
  add(one,oph,oph);
}
