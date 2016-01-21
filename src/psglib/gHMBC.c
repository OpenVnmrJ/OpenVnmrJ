// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* gHMBC - Gradient Selected HMBC

	Features included:
		Pure absorptive peak shape in F1
		Shaka6 inversion pulses in X
		2-step J-filter to suppress one-bond correlations
			[controlled by jfilter flag]
				
	Paramters:
		sspul :		selects magnetization randomization option
		gzlvlE	:	encoding Gradient level
		gtE	:	encoding gradient time
		EDratio	:	encode/decode ratio
		jnxh	:	multiple bond XH coupling constant
		j1min	:	Minimum J1xh value
		j1max	:	Maximum J1xh value
		jfilter :	Selects 2-step jfilter

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***

*/


#include <standard.h>
#include <chempack.h>

static int ph1[1] = {0};
static int ph2[1] = {0};
static int ph3[2] = {0,2};
static int ph4[1] = {0};
static int ph5[4] = {0,0,2,2};
static int ph6[4] = {0,2,2,0};

pulsesequence()
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
	 tauA,
	 tauB,
         taumb;
  int	 icosel,
         prgcycle = (int)(getval("prgcycle")+0.5),
	 phase1 = (int)(getval("phase")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

  grad1 = gzlvlE;
  grad2 = -1.0*gzlvlE*(EDratio-1)/(EDratio+1);
  tauA = 1/(2*(j1min + 0.146*(j1max - j1min)));
  tauB = 1/(2*(j1max - 0.146*(j1max - j1min)));
  taumb = 1/(2*(getval("jnxh")));
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

  settable(t1,1,ph1);
  settable(t2,1,ph2);
  settable(t3,2,ph3);
  settable(t4,1,ph4);
  settable(t5,4,ph5);
  settable(t6,4,ph6);

  getelem(t1,v17,v1);
  getelem(t2,v17,v2);
  getelem(t4,v17,v4);
  getelem(t5,v17,v5); 
  getelem(t3,v17,v3);
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
     rgpulse(pw,v1,rof1,rof2);

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

     delay(taumb);
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

     decrgpulse(pwx,v5,rof1,rof2);
     decpower(dpwr);

  status(C);
} 

