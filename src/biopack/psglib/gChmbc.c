/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gChmbc - Gradient Selected aboslute value HMBC

	Features included:
		F1 Axial Displacement
		Randomization of Magnetization prior to relaxation delay
			with G-90-G 
			[selected by sspul flag]
		J-filter to suppress one-bond correlations
				
	Paramters:
		sspul :		y - selects magnetization randomization option
		gzlvl0:		Homospoil gradient level (DAC units)
		gt0	:	Homospoil gradient time
		gzlvl7	:	encoding Gradient level
		gt7	:	encoding gradient time
		gzlvl8	:	decoding Gradient level
		gt8	:	decoding gradient time
		gstab	:	recovery delay
		jch	:	One-bond XH coupling constant
		jnch	:	multiple bond XH coupling constant
		pwClvl  :	X-nucleus pulse power
		pwC	:	X-nucleus 90 deg pulse width
		d1	:	relaxation delay
		d2	:	Evolution delay

*/


#include <standard.h>

static int ph1[1] = {0};
static int ph2[1] = {0};
static int ph3[2] = {0,2};
static int ph4[1] = {0};
static int ph5[4] = {0,0,2,2};
static int ph6[4] = {0,2,2,0};

void pulsesequence()
{
  double jch,
         jnch,
	 pwClvl,
	 pwC,
	 gzlvl0,
	 gt0,
	 gzlvl7,
	 gt7,
	 gzlvl8,
	 gt8,
	 gstab,
	 satdly,
	 tau,
         taumb;
  char	 sspul[MAXSTR],
	 satmode[MAXSTR];

  jch = getval("jch");
  jnch = getval("jnch");
  pwClvl = getval("pwClvl");
  pwC = getval("pwC");
  getstr("sspul",sspul);
  gzlvl0=getval("gzlvl0");
  gt0=getval("gt0");
  gzlvl7 = getval("gzlvl7");
  gt7 = getval("gt7");
  gzlvl8 = getval("gzlvl8");
  gt8 = getval("gt8");
  gstab = getval("gstab");
  satdly = getval("satdly");
  getstr("satmode",satmode);
  taumb = 1/(2*jnch);
  tau=1/(2.0*jch);

  settable(t1,1,ph1);
  settable(t2,1,ph2);
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
     decpower(pwClvl);
     if (sspul[0] == 'y')
     {
	zgradpulse(gzlvl0*1.3,gt0);
	rgpulse(pw,zero,rof1,rof1);
	zgradpulse(gzlvl0*1.3,gt0);
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

  status(B);
     rgpulse(pw,t1,rof1,rof2);
     if (jch < 300)
     {
     zgradpulse(gzlvl0,gt0);
     delay(tau - rof2 - rof1 - 2*GRADIENT_DELAY - gt0);
     decrgpulse(pwC, t2, rof1, rof1);
     zgradpulse(-gzlvl0,gt0);
     if (satmode[1] == 'y')
      {
       obspower(satpwr);
        if (satfrq != tof)
         obsoffset(satfrq);
        rgpulse(taumb - pwC - 2*GRADIENT_DELAY - gt0 - 2e-5,zero,rof1,rof1);
        if (satfrq != tof)
         obsoffset(tof);
       obspower(tpwr);
       delay(1.0e-5);
      }
     else
      delay(taumb - rof1 - pwC - 2*GRADIENT_DELAY - gt0); 
     }
     decrgpulse(pwC,v3,rof1,rof1);
     zgradpulse(gzlvl7,gt7);
     delay(gstab);
     delay(d2/2);
     rgpulse(pw*2.0,t4,rof1,rof1);
     delay(d2/2);
     zgradpulse(gzlvl7,gt7);
     delay(gstab);
     decrgpulse(pwC,t5,rof1,rof2);
     zgradpulse(gzlvl8,gt8);
     decpower(dpwr);
     delay(gstab);
 
  status(C);
} 

