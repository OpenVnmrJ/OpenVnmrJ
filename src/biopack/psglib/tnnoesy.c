/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* tnnoesy  - 2D cross relaxation experiment with solvent suppression 
               by transmitter presaturation. F1 quadrature by States-TPPI.
               Gradient homospoil pulse (gt1,gzlvl1) can be used in mixing 
               time. Transmitter has to be at solvent frequency!!!
               Allows single transient spectra (ssfilter recommended!!!).

   satmode : determines when the saturation happens. Satmode should be set
             to satmode='ynyn' to enable solvent saturation during d1 and mix.
             NO solvent saturation during t1 is supported!!! 
    satdly : length of presaturation period during relaxation delay d1. 
    satpwr : saturation power ( < 50Hz; ~ 0dB)
     scuba : 'y' inserts comp(180) - scubad/2 - comp(180) - scubad/2 at the end of
             presat to recover the alpha-protons (scubad ~ 40ms - 60ms).
     sspul : 'y' selects gradient homospoil (gt1,gzlvl1) at beginning of d1.
     mfsat : 'y' selects multi-frequency saturation in relax and/or mix periods
           Uses satmode to control which period(s) used (at satpwr)
           Requires the frequency list mfpresat.ll in current experiment
           Pbox creates (saturation) shape "mfpresat.DEC"

                  use mfll('new') to initialize/clear the line list
                  use mfll to read the current cursor position into
                  the mfpresat.ll line list that is created in the
                  current experiment. 

           Note: copying pars or fids (mp or mf) from one exp to another does not copy
                 mfpresat.ll!
           Note: the line list is limited to 128 frequencies ! 
            E.Kupce, Varian, UK June 2005 - added multifrequency presat option 


               made C13refoc flag for C13 decoupling in t1.
               15N refocusing done as if N15refoc='y'
               Both 13C and 15N refocusing done if CNrefoc='y' 
               Dropped power 3db down for both N15 and 13C if simulaneous 180's.
               Uses composite 180 for 13C.

   Refocussing in t1 is limited by the bandwidth of the refocussing pulses. Optimal
   performance would be for aliphatic-only or aromatic-only noesys.
   e.c.hoffmann  darmstadt  may 1994   
   e.c.hoffmann  darmstadt  jan 1996 phasecycle completly rearranged 
   E.Kupce, Varian, UK June 2005 - added multifrequency presat option 
   Josh Kurutz, U of Chicago May 2006 - reorganized mfsat options to make presat periods 
       periods and delays the same whether mfsat='n' or 'y'

 */

#include <standard.h>
#include "mfpresat.h"

void pulsesequence()
{
   double  gzlvl1 = getval("gzlvl1"),
           gzlvl2 = getval("gzlvl2"),
              gt1 = getval("gt1"),
              gt2 = getval("gt2"),
           satdly = getval("satdly"),
              mix = getval("mix"),
              pwNlvl = getval("pwNlvl"),
              pwN = getval("pwN"),
              pwClvl = getval("pwClvl"),
              pwC = getval("pwC"),
           scubad = getval("scubad");
   int     iphase = (int) (getval("phase") + 0.5);
   char    sspul[MAXSTR],mfsat[MAXSTR],scuba[MAXSTR],
           C13refoc[MAXSTR],N15refoc[MAXSTR],CNrefoc[MAXSTR];
   
   getstr("C13refoc",C13refoc);
   getstr("N15refoc",N15refoc);
   getstr("CNrefoc",CNrefoc);
   getstr("mfsat",mfsat);
   getstr("sspul", sspul);
   getstr("scuba",scuba);

   loadtable("tnnoesy");
   sub(ct,ssctr,v12);
   getelem(t1,v12,v1);
   getelem(t2,v12,v2);
   getelem(t3,v12,v5);
   getelem(t4,v12,v8);
   getelem(t5,v12,v9);
   getelem(t6,v12,oph);
   assign(zero,v3);
   assign(one,v4);
   if (iphase == 2)
      {incr(v1); incr(v2); incr(v3); incr(v4);}

/* HYPERCOMPLEX MODE USES REDFIELD TRICK TO MOVE AXIAL PEAKS TO EDGE */
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v6);
   if ((iphase==1)||(iphase==2))
      {add(v1,v6,v1); add(v2,v6,v2); add(oph,v6,oph);
       add(v3,v6,v3); add(v4,v6,v4);}  

/* CHECK CONDITIONS */
   if ((dm[A]=='y') || (dm[B] == 'y') || (dm[C] == 'y'))
     {
      abort_message("Set dm to be nnnn or nnny");
     }
   if ((dm2[A]=='y') || (dm2[B] == 'y') || (dm2[C] == 'y'))
     {
      abort_message("Set dm2 to be nnnn or nnny");
     }

/* BEGIN THE ACTUAL PULSE SEQUENCE */

     decpower(pwClvl); dec2power(pwNlvl);  
     if (CNrefoc[A] == 'y')
      {
       decpower(pwClvl-3.0); pwC=1.4*pwC;
       dec2power(pwNlvl-3.0); pwN=1.4*pwN;
      }

      if (sspul[A] == 'y')
      {
         zgradpulse(gzlvl1,gt1);
         delay(1.0e-4);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(gzlvl1,gt1);
         delay(1.0e-4);
      }
   status(A);
      if (satmode[A] == 'y')
      {
      if (d1 > satdly) delay(d1 - satdly);
       if (mfsat[A] == 'y')
        {obsunblank(); mfpresat_on(); delay(satdly); mfpresat_off(); obsblank();}
       else
        {
         obspower(satpwr);
         rgpulse(satdly,v1,rof1,rof1); 
        }
         obspower(tpwr);
         if (scuba[0] == 'y')
            {
            rgpulse(pw,v3,2.0e-6,0.0);
            rgpulse(2.0*pw,v4,2.0e-6,0.0);
            rgpulse(pw,v3,2.0e-6,0.0);
            delay(scubad/2.0);
            rgpulse(pw,v3,2.0e-6,0.0); 
            rgpulse(2.0*pw,v4,2.0e-6,0.0); 
            rgpulse(pw,v3,2.0e-6,0.0); 
            delay(scubad/2.0); 
            }
      }
      else delay(d1);
      obsstepsize(45.0);
      initval(7.0,v7);
      xmtrphase(v7);
   status(B);
      rgpulse(pw,v2,rof1,0.0);
      xmtrphase(zero);
      if (d2>0.0)
      {
      if ((C13refoc[A] == 'n') &&  (N15refoc[A] == 'n') && (CNrefoc[A] == 'n'))
       {
         delay(d2-4.0*pw/PI-SAPS_DELAY-rof1);
       }

      else if ((C13refoc[A] == 'n') && (N15refoc[A] == 'n') && (CNrefoc[A] == 'y'))
       {
        if (pwN > 2.0*pwC)
         {
          if (d2/2.0 > (pwN +0.64*pw+rof1))
            {
             delay(d2/2.0-pwN-0.64*pw-SAPS_DELAY);
             dec2rgpulse(pwN-2.0*pwC,zero,0.0,0.0);
             sim3pulse(0.0,pwC,pwC, zero,zero,zero, 0.0, 0.0);
             sim3pulse(0.0,2.0*pwC,2.0*pwC, zero,one,zero, 0.0, 0.0);
             sim3pulse(0.0,pwC,pwC, zero,zero,zero, 0.0, 0.0);
             dec2rgpulse(pwN-2.0*pwC,zero,0.0,0.0);
             delay(d2/2.0-pwN-0.64*pw-rof1);
            }
          else
            delay(d2-4.0*pw/PI-SAPS_DELAY-rof1);
         }
        else
         {
           if (d2/2.0 > (pwN +pwC+ 0.64*pw+rof1))
            {
             delay(d2/2.0-pwN-pwC-0.64*pw-SAPS_DELAY);
             decrgpulse(pwC,zero,0.0,0.0);
             sim3pulse(0.0,2.0*pwC,2.0*pwN, zero,one,zero, 0.0, 0.0);
             decrgpulse(pwC,zero,0.0,0.0);
             delay(d2/2.0-pwN-pwC-0.64*pw-rof1);
            }
           else
            delay(d2-4.0*pw/PI-SAPS_DELAY-rof1);
         }
       }
      else if ((C13refoc[A] == 'n') && (N15refoc[A] == 'y') && (CNrefoc[A] == 'n'))
       {
       if (d2/2.0 > (pwN + 0.64*pw+rof1))  
        {
           delay(d2/2.0-pwN-0.64*pw-SAPS_DELAY);
           dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
           delay(d2/2.0-pwN-0.64*pw-rof1);
        }
       else
           delay(d2-1.28*pw-SAPS_DELAY-rof1);
       }

      else if ((C13refoc[A] == 'y') &&  (N15refoc[A] == 'n') && (CNrefoc[A] == 'n'))
       {
        if (d2/2.0 > (2.0*pwC + 0.64*pw +rof1))  
         {
           delay(d2/2.0-2.0*pwC-0.64*pw-SAPS_DELAY);
           decrgpulse(pwC,zero,0.0,0.0);
           decrgpulse(2.0*pwC, one, 0.0, 0.0);
           decrgpulse(pwC,zero,0.0,0.0);
           delay(d2/2.0-2.0*pwC-0.64*pw-rof1);
         }
        else
           delay(d2-1.28*pw-SAPS_DELAY -rof1);
       }
      else
       {
          abort_message("C13refoc, N15refoc, and CNrefoc must be nnn, nny, nyn, or ynn");
       }
      }
      rgpulse(pw,v5,rof1,1.0e-6);
   status(C);
      decpower(dpwr); dec2power(dpwr2);  
      if (satmode[C] == 'y')
      {
       if (mfsat[C] == 'y')
        {obsunblank(); mfpresat_on(); delay(mix*0.7); mfpresat_off(); obsblank();
         zgradpulse(gzlvl2,gt2);
         obsunblank(); mfpresat_on(); delay(mix*0.3-gt2); mfpresat_off(); obsblank();}
       else
        {
         obspower(satpwr);
         rgpulse(mix*0.7,v8,rof1,rof1);   
         zgradpulse(gzlvl2,gt2);
         rgpulse(mix*0.3-gt2,v8,rof1,rof1);  
        }
       obspower(tpwr);
      }
      else
      {
         delay(mix*0.7);
         zgradpulse(gzlvl2,gt2);
         delay(mix*0.3-gt2);
      }
   status(D);
      rgpulse(pw,v9,rof1,rof2);

/*  Phase cycle: .satdly(t1)..pw(t2)..d2..pw(t3)..mix(t4)..pw(t5)..at(t6)
    (for phase=1; for phase=2 incr t1 + t2; for TPPI add two to t1,t2,t6)

    t1 = 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3
    t2 = 2 0 2 0 2 0 2 0 3 1 3 1 3 1 3 1 2 0 2 0 2 0 2 0 3 1 3 1 3 1 3 1
    t3 = 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3
    t4 = 3 3 1 1 0 0 2 2 0 0 2 2 1 1 3 3 3 3 1 1 0 0 2 2 0 0 2 2 1 1 3 3 
    t5 = 0 0 2 2 1 1 3 3 1 1 3 3 2 2 0 0 0 0 2 2 1 1 3 3 1 1 3 3 2 2 0 0
    t6 = 0 2 2 0 1 3 3 1 1 3 3 1 2 0 0 2 2 0 0 2 3 1 1 3 3 1 1 3 0 2 2 0  */
}
