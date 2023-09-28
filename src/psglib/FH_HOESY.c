// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* FH_HOESY - gradient enhanced hetero NOESY sequence
                  n- and p-type selection, phase sensitive version
Ref: W. Bauer, MRC. Vol. 34. 532-537 (1996) absolute value version
     P. Sandor. varian darmstadt, phase sensitive version

Parameters:
          sspul     - grad-90-grad steady-state flag
          hsgt      - gradient duration for sspul (2 msec)
          hsglvl    - gradient power for sspul (15-20 G/cm)
          pw        - 90 deg. proton pulse
          tpwr      - power level for pw
          pwx       - 90 deg. X pulse
          pwxlvl    - power level for pwx
          mix       - mixing time
          gt1       - gradient duration for dephasing (C13)
          gzlvl1    - gradient power for dephasing (~2-3G/cm)
          gt2       - gradient duration for homospoil during mix
          gzlvl2    - gradient power for homospoil during mix
          gt3       - gradient duration for rephasing (H1) 
          gzlvl3    - gradient power for rephasing (~-2-3G/cm)
          gstab     - gradient stabilization delay
          grad_sel  - 'y' pulse sequence with gradient selection
                      'n' pulse sequence with NO gradiebnt selection

Note: This experiment with gradient coherence pathway selection may
      provide better cancellation efficiency BUT may result in
      unacceptable signal losses due to diffusion and possible 
      convection. Do not use long mixing times and strong
      (more then 2G/cm) gzlvl1 and gzlvl3 gradients if grad_sel
      is set to 'y'.

      The sequence uses a dec. 180 deg. pulse in the middle of the
      evolution period to do F1 decoupling. F2 decoupling during
      during acquisition is optional via dm='nny' but please consider
      that the XMTR and te DCPLR shares the same amplifier band therefore
      the receiver and the decoupler is time shared during acquisition 
      - this causes sensitivity losses (60% by default).

Processing: f1coef='1 0 1 0 0 -1 0 1' for grad_sel='y'
            f1coef=''                 for grad_sel='n'  

peter sandor, darmstadt*/

#include <standard.h>
static int ph1[8] = {0,0,2,2,1,1,3,3},
           ph2[8] = {0,0,0,0,2,2,2,2},
           ph3[8] = {0,0,2,2,1,1,3,3},
           ph4[8] = {0,0,0,0,1,1,1,1},
          ph5[16] = {0,2,2,0,0,2,2,0,1,3,3,1,1,3,3,1},
          ph6[32] = {0,2,2,0,0,2,2,0,1,3,3,1,1,3,3,1,
                     2,0,0,2,2,0,0,2,3,1,1,3,3,1,1,3},
        rec_1[16] = {0,2,0,2,0,2,0,2,1,3,1,3,1,3,1,3},

           ph11[1] = {0},
           ph12[4] = {0,0,2,2},
           ph13[2] = {0,2},
          ph14[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
         rec_2[16] = {0,2,0,2,1,3,1,3,2,0,2,0,3,1,3,1};

void pulsesequence()
{
/* DECLARE VARIABLES */

 int      iphase,icosel = 0;
 char     sspul[MAXSTR],satmode[MAXSTR],grad_sel[MAXSTR];
 double   EDratio = getval("EDratio"),
          gtE = getval("gtE"),
	  gt2 = getval("gt2"),
          hsgt = getval("hsgt"),
          gstab = getval("gstab"),
          gzlvl1 = getval("gzlvl1"),
          gzlvl2 = getval("gzlvl2"),
          gzlvlE = getval("gzlvlE"),
          hsglvl = getval("hsglvl"),
          mix = getval("mix"),
          pwx = getval("pwx"),
          pwxlvl = getval("pwxlvl");

  getstr("sspul", sspul);
  getstr("satmode",satmode);
  getstr("grad_sel",grad_sel);

  iphase = (int) (getval("phase") + 0.5);

/* Set phases */
 if (grad_sel[A]=='y')
  {
   settable(t1,8,ph1);
   settable(t2,8,ph2);
   settable(t3,8,ph3);
   settable(t4,8,ph4);
   settable(t5,16,ph5);
   settable(t6,32,ph6);
   settable(t7,16,rec_1);
   sub(ct,ssctr,v10);
   getelem(t1,v10,v1);
   getelem(t2,v10,v2);
   getelem(t3,v10,v3);
   getelem(t4,v10,v4);
   getelem(t5,v10,v5);
   getelem(t6,v10,v6);
   getelem(t7,v10,oph);
  }
 else
  {
   settable(t11,1,ph11);
   settable(t12,4,ph12);
   settable(t13,2,ph13);
   settable(t14,16,ph14);
   settable(t15,16,rec_2);
   sub(ct,ssctr,v10);
   getelem(t11,v10,v1);
   getelem(t12,v10,v2);
   getelem(t13,v10,v3);
   getelem(t14,v10,v4);
   getelem(t15,v10,oph);
  }

/* HYPERCOMPLEX MODE USES REDFIELD TRICK TO MOVE AXIAL PEAKS TO EDGE */
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v7);
   if ((iphase==1)||(iphase==2))
      {add(v1,v7,v1); add(oph,v7,oph); }

/* PHASE INCREMENTATION FOR HYPERCOMPLEX DATA */

 if (grad_sel[A]=='y')
  {
   if (iphase == 2) icosel = -1;
   else icosel = 1;
  }
 else
  {
    if (iphase == 2)  incr(v1);
  }

/* BEGIN ACTUAL PULSE SEQUENCE */
 status(A);
   decpower(pwxlvl); decpwrf(4095.0); obspower(tpwr); 
    if (sspul[A] == 'y')
      {
         zgradpulse(hsglvl,hsgt);
         delay(gstab);
         rgpulse(pw,zero,rof1,rof1);
         decrgpulse(pwx,zero,rof1,rof1);
         zgradpulse(hsglvl,hsgt);
      }
    if (satmode[0] == 'y')
      {
       obspower(satpwr);
       if (d1 - satdly > 0) delay(d1 - satdly);
       if (satfrq != tof) obsoffset(satfrq);
        rgpulse(satdly,zero,rof1,rof1);
       if (satfrq != tof) obsoffset(tof);
       obspower(tpwr);
       delay(1.0e-5);
      }
     else
       delay(d1); 

   if (getflag("wet"))
     wet4(zero,one);

 status(B);
   decrgpulse(pwx,v1,rof1,0.0);
   if (d2/2.0-pw-rof1-2.0*pwx/3.14159265358979323846>0.0)
        delay(d2/2.0-pw-rof1-2.0*pwx/3.14159265358979323846); 
        else delay(d2/2.0);
   rgpulse(2.0*pw,v2,rof1,rof1);
   if (d2/2.0-pw-rof1-2.0*pwx/3.14159265358979323846>0.0)
        delay(d2/2.0-pw-rof1-2.0*pwx/3.14159265358979323846);
        else delay(d2/2.0);

  if (grad_sel[A]=='y')
  {
        delay(gtE+gstab+2.0*GRADIENT_DELAY);
        decrgpulse(2*pwx,v3,rof1,rof1);
        zgradpulse(icosel*EDratio*gzlvlE,gtE);
        delay(gstab-rof1);
        decrgpulse(pwx,v4,rof1,rof1);
      status(C);
         if (satmode[2] == 'y')
           {
            obspower(satpwr);
            if (satfrq != tof) obsoffset(satfrq);
            rgpulse(0.7*mix-gt2,zero,rof1,rof1);
            zgradpulse(gzlvl2,gt2);
            rgpulse(0.3*mix,zero,rof1,rof1);
            if (satfrq != tof) obsoffset(tof);
            obspower(tpwr);
            delay(1.0e-5);
           }
         else 
           {
             delay(mix*0.7-gt2);
             zgradpulse(gzlvl2,gt2);
             delay(mix*0.3);
           }
        rgpulse(pw,v5,rof1,0.0);
      status(A);
        delay(gtE+gstab+2.0*GRADIENT_DELAY);
        rgpulse(2.0*pw,v6,rof1,rof1); 
        zgradpulse(gzlvlE,gtE);
        decpower(dpwr);
        delay(gstab-POWER_DELAY);
  }
  else
  {
        decrgpulse(pwx,v3,rof1,rof1);
      status(C);
         if (satmode[2] == 'y')
           {
            obspower(satpwr);
            if (satfrq != tof) obsoffset(satfrq);
            rgpulse(0.7*mix-gt2,zero,rof1,rof1);
            zgradpulse(gzlvl2,gt2);
            rgpulse(0.3*mix,zero,rof1,rof1);
            if (satfrq != tof) obsoffset(tof);
            obspower(tpwr);
            delay(1.0e-5);
           }
         else 
           {
             delay(mix*0.7-gt2);
             zgradpulse(gzlvl2,gt2);
             delay(mix*0.3);
             decpower(dpwr);
           }
        rgpulse(pw,v4,rof1,rof2);
  }
 status(D);
}
