// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* ghmbcme.c - Multiplicity-edited Gradient Selected HMBC
	Adiabatic version

        Features included:
                Pure absorptive peak shape in F1
                2-step J-filter to suppress one-bond correlations
                        [controlled by jfilter flag]

        Paramters:
                sspul :         selects magnetization randomization option
                gzlvlE  :       encoding Gradient level
                gtE     :       encoding gradient time
                EDratio :       encode/decode ratio
                jnxh    :       multiple bond XH coupling constant
                j1min   :       Minimum J1xh value
                j1max   :       Maximum J1xh value
                jfilter :       Selects 2-step jfilter
                mult    :       Multiplicty editing (mult = 0, 2; array = 'mult,phase')

	Reference: Nyberg NT, Sorensen OW, MRC 44 (2006) 451-454.

haitao -  	First revision		: Feb 2006
haitao -  	Revised			: Mar 2006
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/

#include <standard.h>
#include <chempack.h>

static int ph1[4] = {0, 2, 2, 0};
static int ph2[8] = {0, 0, 2, 2, 2, 2, 0, 0};
static int ph3[16] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};
static int ph4[2] = {0, 2};

void pulsesequence()
{

  double j1min = getval("j1min"),
         j1max = getval("j1max"),
         pwx180 = getval("pwx180"),
         pwxlvl180 = getval("pwxlvl180"),
         pwx180r = getval("pwx180r"),
         pwxlvl180r = getval("pwxlvl180r"),
         mult = getval("mult"),
         gzlvl0 = getval("gzlvl0"),
         gt0 = getval("gt0"),
         gzlvlE = getval("gzlvlE"),
         gtE = getval("gtE"),
         EDratio = getval("EDratio"),
         gstab = getval("gstab"),
	 tauA,
         tauB,
         tau,
         taumb,
         taug,
         grad1,
         grad2,
         grad3,
         grad4;

  char   pwx180ad[MAXSTR], 
         pwx180ref[MAXSTR];

  int 	 icosel,
         prgcycle = (int)(getval("prgcycle")+0.5),
	 phase1 = (int)(getval("phase")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

  getstr("pwx180ad", pwx180ad);
  getstr("pwx180ref", pwx180ref);

  tauA = 1/(2*(j1min + 0.146*(j1max - j1min)));
  tauB = 1/(2*(j1max - 0.146*(j1max - j1min)));
  tau = 1/(j1max + j1min);
  taumb = 1 / (2 * (getval("jnxh")));

  taug = tau + getval("tauC");

  icosel = 1;

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

  settable(t1, 4, ph1);
  settable(t2, 8, ph2);
  settable(t3, 16, ph3);
  settable(t4, 2, ph4);

  getelem(t1, v17, v1);
  getelem(t2, v17, v2);
  getelem(t3, v17, v3);
  getelem(t4, v17, oph);

  add(oph,v18,oph);
  add(oph,v19,oph);
  assign(zero,v6);

/*
  mod2(id2, v14);
  dbl(v14,v14);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

  add(v2, v14, v2);
  add(oph, v14, oph);

  if ((phase1 == 2) || (phase1 == 5))
  {
    icosel = -1;
  }

  grad2 = gzlvlE;
  grad3 = -1.5*gzlvlE;
  grad1 = -1.0*grad2*(EDratio + icosel)/EDratio;
  grad4 = -1.0*grad3*(EDratio - icosel)/EDratio;

  if (mult > 0.5)
  {
    grad4 = -1.0*grad3*(EDratio + icosel)/EDratio;
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
                   shaped_purge(v6,zero,v18,v19);
           }
        else
           {
                satpulse(satdly,zero,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v6,zero,v18,v19);
           }
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   obspower(tpwr);
   decpower(pwxlvl);

  status(B);

   if (getflag("cpmgflg"))
   {
     rgpulse(pw, v6, rof1, 0.0);
     cpmg(v6, v15);
   }
   else
     rgpulse(pw, v6, rof1, rof2);

/* Start of J filter  */

   if (getflag("jfilter"))
   {
     zgradpulse(gzlvl0,gt0);
     delay(tauA - gt0);
     decrgpulse(pwx, zero, rof1, rof1);
     zgradpulse(-gzlvl0*2/3,gt0);
     delay(tauB - gt0);
     decrgpulse(pwx, zero, rof1, rof1);
     zgradpulse(-gzlvl0/3,gt0);
    }

/* End of J filter */

    delay(taumb);

    zgradpulse(grad1,gtE);
    delay(gstab);

    decrgpulse(pwx, v2, rof1, rof1);

    if (mult > 0.5)
    {
    decpower(pwxlvl180r);
    zgradpulse(grad2,gtE);
    delay(taug-gtE-2*GRADIENT_DELAY);
    decshaped_pulse(pwx180ref, pwx180r, zero, rof1, rof1);

    delay(d2/2);
    rgpulse(2*pw,v3,rof1,rof1);
    delay(d2/2);

    delay(taug-(gtE+gstab+2*GRADIENT_DELAY)+(4*pwx/PI+2*rof1+2*POWER_DELAY-2*pw-2*rof1));
    zgradpulse(grad3,gtE);
    delay(gstab);
    decshaped_pulse(pwx180ref, pwx180r, zero, rof1, rof1);
    }

    else
    {
    decpower(pwxlvl180);
    zgradpulse(grad2,gtE);
    delay(taug/2+(pwx180r-pwx180)/2- gtE - 2*GRADIENT_DELAY);
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    delay(taug/2+(pwx180r-pwx180)/2.0);

    delay(d2/2);
    rgpulse(2*pw,v3,rof1,rof1);
    delay(d2/2);

    delay(taug/2+(pwx180r-pwx180)/2.0+(4*pwx/PI+2*rof1+2*POWER_DELAY-2*pw-2*rof1));
    decshaped_pulse(pwx180ad, pwx180, zero, rof1, rof1);
    delay(taug/2+(pwx180r-pwx180)/2-gtE-gstab-2*GRADIENT_DELAY);
    zgradpulse(grad3,gtE);
    delay(gstab);
    }

    decpower(pwxlvl);

    decrgpulse(pwx, v1, rof1, rof2);
    decpower(dpwr);
    zgradpulse(grad4,gtE);
    delay(gstab);

  status(C);
}
