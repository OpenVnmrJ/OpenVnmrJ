// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
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
                sspul :         selects magnetization randomization option
                gzlvlE  :       encoding Gradient level
                gtE     :       encoding gradient time
                EDratio :       encode/decode ratio
                jnxh    :       multiple bond XH coupling constant
                j1min   :       Minimum J1xh value
                j1max   :       Maximum J1xh value
                jfilter :       Selects 2-step jfilter
                gzlvl0  :       gradinets used for echos
                gt0     :       time for echo gradients
                gstab   :       recovery delay
                jscaleU :       Uniform scaling factor for JHH skewing [typically 0]
                jscaleD :	Differential scaling factor for 2JHH and 3JHH
                		[used to differentiate 2bond vs 3bond correlations to
                			protonated carbons]
KrishK  -       CIGAR-HMBC              : February '99
KrishK  -       can now be used for "static" gHMBC as well.  : Feb. 00
KrishK	-	Includes diffential scaling for 2JHH and 3JHH for protonated carbons.
KrishK	-	Modified : Sept. 2004
KrishK	-	Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/


#include <standard.h>
#include <chempack.h>

static double d2_init = 0.0;

static int ph1[1] = {0};
static int ph3[4] = {0,2};
static int ph4[1] = {0};
static int ph5[8] = {0,0,2,2};
static int ph6[8] = {0,2,2,0};

void pulsesequence()
{
  double j1min = getval("j1min"),
         j1max = getval("j1max"),
         pwxlvlS6 = getval("pwxlvlS6"),
         pwxS6 = getval("pwxS6"),
         gzlvl0 = getval("gzlvl0"),
         gt0 = getval("gt0"),
         gzlvlE = getval("gzlvlE"),
         gtE = getval("gtE"),
         EDratio = getval("EDratio"),
         gstab = getval("gstab"),
         grad1,
         grad2,
	 bird,
         tauX,
         tau,
         tautau,
         t1max,
	 tauA,
	 tauB,
         tau1,
         tau2,
         taumb;
  int    ijscaleU,
  	 ijscaleD,
	 icosel,
  	 t1_counter,
         prgcycle = (int)(getval("prgcycle")+0.5),
	 phase1 = (int)(getval("phase")+0.5);
  char	 accord[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gt0 = syncGradTime("gt0","gzlvl0",1.0);
        gzlvl0 = syncGradLvl("gt0","gzlvl0",1.0);
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

  grad1 = gzlvlE;
  grad2 = -1.0*gzlvlE*(EDratio-1)/(EDratio+1);
  tauX = 1/(2*(getval("jnmax")));
  tauA = 1/(2*(j1min + 0.146*(j1max - j1min)));
  tauB = 1/(2*(j1max - 0.146*(j1max - j1min)));
  bird = (tauA+tauB)/2;
  ijscaleD = (int)(getval("jscaleD") + 0.5);
  ijscaleU = (int)(getval("jscaleU") + 0.5);
  if (ijscaleU > 0)
	ijscaleD = 0;
  icosel = 1;
  getstr("accord",accord);

  t1max = ((getval("ni")-1)/getval("sw1")) + 0.0005;
  tautau = ((ijscaleU - 1)*d2);
  taumb = 1/(2*(getval("jnmin")));
  if (ix == 1)
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
 
  assign(ct,v17);
  assign(zero,v18);
  assign(zero,v19);

  if (getflag("prgflg") && (satmode[0] == 'y') && (prgcycle > 1.5))
    {
        hlv(ct,v17);
        mod2(ct,v18); dbl(v18,v18);
        if (prgcycle > 2.5)
           {
                hlv(v17,v17);
                hlv(ct,v19); mod2(v19,v19); dbl(v19,v19);
           }
     }
 
  settable(t1,1,ph1);
  settable(t3,2,ph3);
  settable(t4,1,ph4);
  settable(t5,4,ph5);
  settable(t6,4,ph6);

  getelem(t1,v17,v1);
  getelem(t3,v17,v3);
  getelem(t4,v17,v4);
  getelem(t5,v17,v5);
  getelem(t6,v17,oph);

  add(oph,v18,oph);
  add(oph,v19,oph);

/*
  mod2(id2,v10);
  dbl(v10,v10);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);

  add(v3,v10,v3);
  add(oph,v10,oph);

  if ((phase1 == 2) || (phase1 == 5))
        {  icosel = -1;
           grad1 = gzlvlE*(EDratio-1)/(EDratio+1);
           grad2 = -1.0*gzlvlE;
        }


  status(A);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        if (getflag("slpsat"))
           {
                shaped_satpulse("relaxD",satdly,zero);
                if (getflag("prgflg"))
                   shaped_purge(v1,zero,v18,v19);
           }
        else
           {
                satpulse(satdly,zero,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,zero,v18,v19);
           }
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   decpower(pwxlvl);

  status(B);
  if (getflag("cpmgflg"))
  {
     rgpulse(pw, v1, rof1, 0.0);
     cpmg(v1, v15);
  }
  else
     rgpulse(pw, v1, rof1, rof2);

/* Start of J filter  */
  if (getflag("jfilter"))
  {
     zgradpulse(gzlvl0/2,gt0);
     delay(tauA - gt0);
     decrgpulse(pwx, zero, rof1, rof1);
     zgradpulse(-gzlvl0/3,gt0);
     delay(tauB - gt0);
     decrgpulse(pwx, zero, rof1, rof1);
     zgradpulse(-gzlvl0/6,gt0);
     delay(gstab);
  }
/* End of J filter */


/* Start of STARR */
/* rev-BIRD in the middle of tau2 - 180 for C13-H and 360 for C12-H  */
/*      -   decouple long-range CH                                   */
/* 	    evolution and let HH evolution continue except for       */
/* 	         2-bond correlations				     */

  if (ijscaleD > 0)
  {
       delay(tau2/2);
        zgradpulse(gzlvl0*1.5,gt0);
        delay(gstab);
        rgpulse(pw,zero,rof1,rof1);
        delay(bird);
        decpower(pwxlvlS6);
        decrgpulse(158.0*pwxS6/90,zero,rof1,2.0e-6);
        decrgpulse(171.2*pwxS6/90,two,2.0e-6,2.0e-6);
        decrgpulse(342.8*pwxS6/90,zero,2.0e-6,2.0e-6);
        decrgpulse(145.5*pwxS6/90,two,2.0e-6,2.0e-6);
        decrgpulse(81.2*pwxS6/90,zero,2.0e-6,2.0e-6);
        decrgpulse(85.3*pwxS6/90,two,2.0e-6,rof1);
        rgpulse(2*pw,zero,rof1,rof1);
	decpower(pwxlvl);
	delay(985*pwxS6/90);
        delay(bird);
        rgpulse(pw,zero,rof1,rof2);
        zgradpulse(-gzlvl0*1.5,gt0);
        delay(gstab);
       delay(tau2/2);
   }
    
/* Simple C 180 in the middle of tau1 - decouple */
/*	CH evolution and let HH evolution continue  */
/* AND   Start of CT-VD */
    
    if (!((accord[0] == 'n') && (ijscaleU == 1)))
      {
       delay(tau1/2 + tau/2 + tautau/4);
        zgradpulse(gzlvl0,gt0);
        delay(gstab);
        decpower(pwxlvlS6);
        decrgpulse(158.0*pwxS6/90,zero,rof1,2.0e-6);
        decrgpulse(171.2*pwxS6/90,two,2.0e-6,2.0e-6);
        decrgpulse(342.8*pwxS6/90,zero,2.0e-6,2.0e-6);
        decrgpulse(145.5*pwxS6/90,two,2.0e-6,2.0e-6);
        decrgpulse(81.2*pwxS6/90,zero,2.0e-6,2.0e-6);
        decrgpulse(85.3*pwxS6/90,two,2.0e-6,rof1);
        decpower(pwxlvl);
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

     delay(d2/2.0);
     rgpulse(2*pw,v4,rof1,rof1);
     delay(d2/2.0);

     decpower(pwxlvlS6);
        decrgpulse(158.0*pwxS6/90,zero,rof1,2.0e-6);
        decrgpulse(171.2*pwxS6/90,two,2.0e-6,2.0e-6);
        decrgpulse(342.8*pwxS6/90,zero,2.0e-6,2.0e-6);
        decrgpulse(145.5*pwxS6/90,two,2.0e-6,2.0e-6);
        decrgpulse(81.2*pwxS6/90,zero,2.0e-6,2.0e-6);
        decrgpulse(85.3*pwxS6/90,two,2.0e-6,rof1);

     delay(2*pw + 2*POWER_DELAY + 4*rof1 + (4*pwx/3.1416));

     zgradpulse(icosel*grad1,gtE);
     delay(gstab);

        decrgpulse(158.0*pwxS6/90,zero,rof1,2.0e-6);
        decrgpulse(171.2*pwxS6/90,two,2.0e-6,2.0e-6);
        decrgpulse(342.8*pwxS6/90,zero,2.0e-6,2.0e-6);
        decrgpulse(145.5*pwxS6/90,two,2.0e-6,2.0e-6);
        decrgpulse(81.2*pwxS6/90,zero,2.0e-6,2.0e-6);
        decrgpulse(85.3*pwxS6/90,two,2.0e-6,rof1);

     zgradpulse(icosel*grad2,gtE);
     decpower(pwxlvl);
     delay(gstab);

     decrgpulse(pwx,v5,rof1,rof1);
     decpower(dpwr);
     delay(t1max/2 + tautau/2);
  status(C);
}
