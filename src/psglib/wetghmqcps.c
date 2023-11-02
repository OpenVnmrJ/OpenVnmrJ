// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* ghmqcps - a phase-sensitive PFG HMQC
   PS version: uses three gradients - all set separately

	j = 1JXH in Hz (140 typical for 1H-13C)
	pwxpwr = (NOT PWXLVL!!) decoupler pulse power level
	pwx = decoupler pulsed pw90 
        gzlvl1 = gradient amplitude (-32768 to +32768)
        gt1 = gradient time (duration) in seconds (0.001)
        gzlvl2 = gradient amplitude (-32768 to +32768)
        gt2 = gradient time (duration) in seconds (0.001)
        gzlvl3 = gradient amplitude (-32768 to +32768)
        gt3 = gradient time (duration) in seconds (0.001)
        grise = gradient rise and fall time (in seconds; 0.00001)
        gstab = optional delay for stability (in seconds)
        nt - works with nt=1 (nt=2 improves data)
        phase - use phase=1,2 to select N,P-type selection to be sorted later
                (use phase=1 to generate an absolute value dataset)

   gzlvl1, gzlvl2, and gzlvl3 and their times (gt1,gt2,gt3) may eventually be fixed
   in their relationship (i.e.2:2:-1, etc) and driven off of one parameter

  Gradient Recommendations:
        for 13C try:
		gzlvl1=20000
                gt1=0.002
                gzlvl2=20000
                gt2=0.002
                gzlvl3=10050
                gt3=0.002

        for 15N try: 
		gzlvl1=
                gt1=0.00
                gzlvl2=
                gt2=0.00
                gzlvl3=00
                gt3=0.00

  PROCESSING:

        process phase=1 (N-type) data with wft2d(1,0,0,1)
        process phase=2 (P-type) data with wft2d(1,0,0,-1)
                   the ('t2dc') argument to wft2d may be useful

        process phase sensitive data (phase = 1,2) with:
                wft2d(1,0,0,1,0,1,1,0) (wft2dnp)

*/


#include <standard.h>
#define D2_FUDGEFACTOR 1.0e-06      /* to empirically make lp1=0 */
static double d2_init = 0.0;

static int ph1[8] = {0,0,0,0,0,0,0,0};
static int ph2[8] = {0,0,0,0,0,0,0,0};
static int ph3[8] = {0,0,2,2,0,0,2,2};
static int ph4[8] = {0,0,0,0,0,0,0,0};
static int ph5[8] = {0,0,0,0,0,0,0,0};
static int ph6[8] = {0,0,0,0,0,0,0,0};
static int ph7[8] = {0,0,0,0,0,0,0,0};
static int ph8[8] = {0,0,0,0,0,0,0,0};
static int ph9[8] = {0,2,0,2,0,2,0,2};
static int ph10[8] = {0,2,2,0,0,2,2,0};
 

void pulsesequence()
{
  double j,pwxpwr,gzlvl1,gt1,gzlvl2,gt2,gzlvl3,gt3,grise,gstab,phase;
  double taumb = 0.0;
  int  icosel,t1_counter;
  char mbond[MAXSTR];

  sw1 = getval("sw1");
  j = getval("j");
  pwxpwr = getval("pwxpwr");
  pwx = getval("pwx");
  gzlvl1 = getval("gzlvl1");
  gt1 = getval("gt1");
  gzlvl2 = getval("gzlvl2");
  gt2 = getval("gt2");
  gzlvl3 = getval("gzlvl3");
  gt3 = getval("gt3");
  grise = getval("grise");
  gstab = getval("gstab");
  phase = getval("phase");
  getstr("mbond", mbond);
  if (mbond[0] == 'y')
      taumb = getval("taumb");

  tau=1/(2.0*j);

  if(tau < (gt3+grise)) 
  {
    text_error("tau must be greater than gt3+grise\n");
    psg_abort(1);
  }

  settable(t1,4,ph1);
  settable(t2,8,ph2);
  settable(t3,4,ph3);
  settable(t4,4,ph4);
  settable(t5,4,ph5);
  settable(t6,4,ph6);
  settable(t7,4,ph7);
  settable(t8,4,ph8);
  settable(t9,4,ph9);
  settable(t10,4,ph10);

/* Gradient incrementation for hypercomplex data */

   if (phase == 2)     /* Hypercomplex in t1 */
   {
      icosel = -1; /* change sign of gradient */
   }
   else icosel = 1;

/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
 
   if(ix == 1)
      d2_init = d2;
 
      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
       
      if(t1_counter %2)  
      {
        tsadd(t9,2,4);
        tsadd(t10,2,4);
      }
 

  status(A);
     decpower(pwxpwr);
     delay(d1);
   if (getflag("wet")) wet4(zero,one);
     decpower(pwxpwr);
     rcvroff();

  status(B);
     rgpulse(pw,t1,rof1,rof2);
     delay(tau);
      if (mbond[0] == 'y')                      /* one-bond J(CH)-filter */
      {
         decrgpulse(pwx, t2, rof1, 0.0);
         delay(taumb - tau - rof1 - pwx);
      }

   decrgpulse(pwx,t3,rof1,rof2);
     decphase(t5);
     delay(gt1 + (grise*2.0) + (2.0*GRADIENT_DELAY));
     decrgpulse(pwx*2.0,t3,rof1,rof2);
     txphase(t6);
     rgradient('z',gzlvl1);
     delay(gt1+grise);
     rgradient('z',0.0);
     delay(grise);
   if (d2 > 0);   
     delay(d2/2 + (2.0*pwx/3.14159265358979323846) - pw + D2_FUDGEFACTOR);

 rgpulse(pw*2.0,t6,rof1,rof2);

     decphase(t9);
     rgradient('z',gzlvl2);
     delay(gt2+grise);
     rgradient('z',0.0);
     delay(grise);
   if (d2 > 0);   
     delay(d2/2 + (2.0*pwx/3.14159265358979323846) - pw + D2_FUDGEFACTOR);
     decrgpulse(pwx*2.0,t9,rof1,rof2);
     decphase(t9);
     delay(gt2 + (grise*2.0) + (2.0*GRADIENT_DELAY));
     decrgpulse(pwx,t9,rof1,rof2);

     rgradient('z',(double)icosel*gzlvl3);
     delay(gt3+grise);
     rgradient('z',0.0);
     decpower(dpwr);
     setreceiver(t10); 
     rcvron();
     delay(tau-(gt3+grise));
/*     delay(gstab); (Needs to be zero to phase f2 automatically) */
 
  status(C);
} 

