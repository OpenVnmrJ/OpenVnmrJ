// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* 	CIGAR2j3j - Improved IMPEACH
                Introduces a Varaible J Scale for JHH skewing
                Uniform skewing or "differential" skewing.

        Features included:
                F1 Axial Displacement
                Randomization of Magnetization prior to relaxation delay
                        with G-90-G
                        [selected by sspul flag]
                Includes a 2-stage J filter and BIRD based one-bond correlation
                        suppression

        Paramters:
                sspul :         y - selects magnetization randomization option
                hsglvl:         Homospoil gradient level (DAC units)
                hsgt    :       Homospoil gradient time
                gzlvl1  :       encoding Gradient level
                gt1     :       encoding gradient time
                gzlvl3  :       decoding Gradient level
                gt3     :       decoding gradient time
                gzlvl0  :       gradinets used for echos
                gt0     :       time for echo gradients
                gstab   :       recovery delay
        j1max & j1min   :       One-bond XH coupling constants
                jnxh    :       multiple bond XH coupling constant for gHMBC
        jnmax & jnmin   :       multiple bond XH coupling constants for accordion
                jscaleU :       Uniform scaling factor for JHH skewing [typically 0]
                jscaleD :	Differential scaling factor for 2JHH and 3JHH
                		[used to differentiate 2bond vs 3bond correlations to
                			protonated carbons]
                pwxlvl  :       X-nucleus pulse power
                pwx     :       X-nucleus 90 deg pulse width
                d1      :       relaxation delay
                d2      :       Evolution delay
Echo family - NM (April 19, 2007)
*/


#include <standard.h>

static double d2_init = 0.0;

static int ph1[1] = {0};
static int ph3[4] = {0,2};
static int ph4[1] = {0};
static int ph5[8] = {0,0,2,2};
static int ph6[8] = {0,2,2,0};

void pulsesequence()
{
  double j1min,
  	 j1max,
         pwxlvl,
         pwx,
         gzlvl0,
         gt0,
	 gzlvl1,
	 gt1,
         gzlvl3,
         gt3,
	 bird,
         gstab,
         hsglvl,
         hsgt,
         satdly,
         tauX,
         tau,
         tautau,
         t1max,
	 tauA,
	 tauB,
         tau1,
         tau2,
         taumb;
  char   sspul[MAXSTR],
  	 accord[MAXSTR],
  	 satmode[MAXSTR];
  int    ijscaleU,
  	 ijscaleD,
  	 t1_counter;

  j1min = getval("j1min");
  j1max = getval("j1max");
  pwxlvl = getval("pwxlvl");
  pwx = getval("pwx");
  getstr("sspul",sspul);
  getstr("accord",accord);
  gzlvl0 = getval("gzlvl0");
  gt0 = getval("gt0");
  gzlvl1 = getval("gzlvl1");
  gt1 = getval("gt1");
  gzlvl3 = getval("gzlvl3");
  gt3 = getval("gt3");
  gstab = getval("gstab");
  hsglvl = getval("hsglvl");
  hsgt = getval("hsgt");
  satdly = getval("satdly");
  getstr("satmode",satmode);
  tauX = 1/(2*(getval("jnmax")));
  tauA = 1/(2*(j1min + 0.146*(j1max - j1min)));
  tauB = 1/(2*(j1max - 0.146*(j1max - j1min)));
  bird = (tauA+tauB)/2;
  ijscaleD = (int)(getval("jscaleD") + 0.5);
  ijscaleU = (int)(getval("jscaleU") + 0.5);
  t1max = ((getval("ni")-1)/getval("sw1")) + 0.0005;
  tautau = ((ijscaleU - 1)*d2);
  taumb = 1/(2*(getval("jnmin")));
   if(ix == 1)
     d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*(getval("sw1")) + 0.5);

  if (accord[0] == 'y')
  {
  if ((taumb - tauX) < t1max/2)
    taumb = tauX + t1max/2;  
  tau = ((taumb - tauX)*t1_counter/ni);
  }
  else
  {
    taumb = 1/(2*(getval("jnxh")));
    if (ijscaleU > 0) 
      tau = 0.0;
    else
      tau = t1max/2;
  }

  tau1 = ijscaleD * d2;
  tau2 = (ijscaleD*t1max) - tau1;
  
  if ((ijscaleD > 0) && (ijscaleU > 0))
  {
    printf("Either jscaleD or jscaleU must be set to zero.\n");
    psg_abort(1);
  }

  settable(t1,1,ph1);
  settable(t3,2,ph3);
  settable(t4,1,ph4);
  settable(t5,4,ph5);
  settable(t6,4,ph6);

  getelem(t3,ct,v3);
  getelem(t6,ct,oph);

  initval(2.0*(double)((int)(d2*getval("sw1")+0.5)%2),v10);
  add(v3,v10,v3);
  add(oph,v10,oph);

  status(A);
     decpower(pwxlvl);
     if (sspul[0] == 'y')
     {
        zgradpulse(hsglvl*1.3,hsgt);
        rgpulse(pw,zero,rof1,rof1);
        zgradpulse(hsglvl*1.3,hsgt);
     }

     if (satmode[0] == 'y')
      {
       if (d1 - satdly > 0)
         delay(d1 - satdly);
       else
       delay(0.02);
       obspower(satpwr);
        if (satfrq != tof)
         obsoffset(satfrq);
        rgpulse(satdly,zero,rof1,rof1);
        if (satfrq != tof)
         obsoffset(tof);
       obspower(tpwr);
       delay(1.0e-5);
      }
     else
     {  delay(d1); }

   if (getflag("wet")) 
     wet4(zero,one);

  status(B);
     rgpulse(pw,t1,rof1,rof2);

/* Start of J filter  */
     zgradpulse(gzlvl0/2,gt0);
     delay(tauA - gt0);
     decrgpulse(pwx, zero, rof1, rof1);
     zgradpulse(-gzlvl0/3,gt0);
     delay(tauB - gt0);
     decrgpulse(pwx, zero, rof1, rof1);
     zgradpulse(-gzlvl0/6,gt0);
     delay(gstab);
/* End of J filter */


/* Start of STARR */
/* rev-BIRD in the middle of tau2 - 180 for C13-H and 360 for C12-H  */
/*      -   decouple long-range CH                                   */
/* 	    evolution and let HH evolution continue except for       */
/* 	         2-bond correlations				     */

       delay(tau2/2);
        zgradpulse(gzlvl0*1.5,gt0);
        delay(gstab);
        rgpulse(pw,zero,rof1,rof1);
        delay(bird);
        simpulse(2*pw,2*pwx,zero,zero,rof1,rof1);
        delay(bird);
        rgpulse(pw,zero,rof1,rof2);
        zgradpulse(-gzlvl0*1.5,gt0);
        delay(gstab);
       delay(tau2/2);
       
/* Simple C 180 in the middle of tau1 - decouple */
/*	CH evolution and let HH evolution continue  */
/* AND   Start of CT-VD */
    
    if (!((accord[0] == 'n') && (ijscaleU == 1)))
      {
       delay(tau1/2 + tau/2 + tautau/4);
        zgradpulse(gzlvl0,gt0);
        delay(gstab);
        decrgpulse(2*pwx,two,rof1,rof1);
        zgradpulse(-gzlvl0,gt0);
        delay(gstab);
       delay(tau1/2 + tau/2 + tautau/4);
      }
       
/* End of STARR  */

    if (accord[0] == 'y')
     delay(taumb - tau);
    else
     delay(taumb);
     
/* End of CT-VD */

     decrgpulse(pwx,v3,rof1,rof1);
     zgradpulse(gzlvl1,gt1);
     delay(gstab);
     delay(d2/2);
     rgpulse(pw*2.0,t4,rof1,rof1);
     delay(d2/2);
     zgradpulse(gzlvl1,gt1);
     delay(gstab);
     decrgpulse(pwx,t5,rof1,rof1);

     zgradpulse(gzlvl3,gt3);
     decpower(dpwr);
     delay(t1max/2 + tautau/2);
  status(C);
}
