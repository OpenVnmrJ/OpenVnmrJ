// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* gc2hmbc.c - Gradient Selected C2HMBC
	Adiabatic/BIP version for both nuclei

        Features included:
                Pure absorptive peak shape in F1
                2-step J-filter to suppress one-bond correlations
                        [controlled by jfilter flag]
		Includes IMPRESS option

        Paramters:
                sspul :         selects magnetization randomization option
                gzlvlE  :       encoding Gradient level
                gtE     :       encoding gradient time
                EDratio :       encode/decode ratio
                jnxh    :       multiple bond XH coupling constant
                j1min   :       Minimum J1xh value
                j1max   :       Maximum J1xh value
                jfilter :       Selects 2-step jfilter

		imphase	:	Selects impress option
				0 - uses pwxlvl180, pwx180, pwx180ad
				  for shape1 and shape2
				1-4 uses imppw1, imppwr1,impshp1 for shape1
				1 - uses imppw1,imppwr1,impshp1 for shape2 
				2 - uses imppw2,imppwr2,impshp2 for shape2
				3 - uses imppw3,imppwr3,impshp3 for shape2
				4 - user imppw4,imppwr4,impshp4 for shape2

haitao -  	First revision		: Sep 2005
haitao -  	Revised 		: Mar 2006
KrishK	-	Includes purge option	: Jan 2008
*/


#include <standard.h>
#include <chempack.h>

static int ph1[1] = {0};
static int ph2[1] = {0};
static int ph3[2] = {0, 2};
static int ph4[16] = {0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 1, 1, 3, 3, 3, 3};
static int ph5[4] = {0, 0, 2, 2};
static int ph6[16] = {0, 2, 2, 0, 0, 2, 2, 0, 2, 0, 0, 2, 2, 0, 0, 2};

pulsesequence()
{

  double j1min = getval("j1min"),
         j1max = getval("j1max"),
	 pwx180 = getval("pwx180"),
	 pwxlvl180 = getval("pwxlvl180"),
	 pw180 = getval("pw180"),
	 tpwr180 = getval("tpwr180"),
	 Ipw1,
	 Ipw2,
	 Ipwr1,
	 Ipwr2,
	 imphase = getval("imphase"),
         gzlvl0 = getval("gzlvl0"),
         gt0 = getval("gt0"),
         gzlvlE = getval("gzlvlE"),
         gtE = getval("gtE"),
         EDratio = getval("EDratio"),
         gstab = getval("gstab"),
	 tauA,
         tauB,
         taumb,
         grad1,
         grad2;

  char   pw180ad[MAXSTR], 
	 bipflg[MAXSTR],
	 Ishp1[MAXSTR], 
	 Ishp2[MAXSTR];

  int 	 iimphase, 
	 icosel,
         prgcycle = (int)(getval("prgcycle")+0.5),
	 phase1 = (int)(getval("phase")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",0.5);
        gzlvlE = syncGradLvl("gtE","gzlvlE",0.5);

  getstr("pwx180ad", Ishp1);
  getstr("pwx180ad", Ishp2);
  getstr("pw180ad", pw180ad);
  getstr("bipflg",bipflg);

  if (bipflg[0] == 'y')
  {
        tpwr180=getval("tnbippwr");
        pw180=getval("tnbippw");
        getstr("tnbipshp",pw180ad);
  }

  if (bipflg[1] == 'y')
  {
        pwxlvl180=getval("dnbippwr");
        pwx180=getval("dnbippw");
        getstr("dnbipshp",Ishp1);
        getstr("dnbipshp",Ishp2);
  }

  grad1 = gzlvlE;
  grad2 = -1.0*gzlvlE*(EDratio-1)/(EDratio+1);
  Ipw1 = pwx180;
  Ipw2 = pwx180;
  Ipwr1 = pwxlvl180;
  Ipwr2 = pwxlvl180;
  tauA = 1/(2*(j1min + 0.146*(j1max - j1min)));
  tauB = 1/(2*(j1max - 0.146*(j1max - j1min)));
  taumb = 1 / (2 * (getval("jnxh")));

  icosel = 1;
  iimphase = (int) (imphase + 0.5);

  if (iimphase == 1)
  {
    Ipw1 = getval("imppw1");
    Ipw2 = getval("imppw1");
    Ipwr1 = getval("imppwr1");
    Ipwr2 = getval("imppwr1");
    getstr("impshp1", Ishp1);
    getstr("impshp1", Ishp2);
  }
  if (iimphase == 2)
  {
    Ipw1 = getval("imppw1");
    Ipw2 = getval("imppw2");
    Ipwr1 = getval("imppwr1");
    Ipwr2 = getval("imppwr2");
    getstr("impshp1", Ishp1);
    getstr("impshp2", Ishp2);
  }
  if (iimphase == 3)
  {
    Ipw1 = getval("imppw1");
    Ipw2 = getval("imppw3");
    Ipwr1 = getval("imppwr1");
    Ipwr2 = getval("imppwr3");
    getstr("impshp1", Ishp1);
    getstr("impshp3", Ishp2);
  }
  if (iimphase == 4)
  {
    Ipw1 = getval("imppw1");
    Ipw2 = getval("imppw4");
    Ipwr1 = getval("imppwr1");
    Ipwr2 = getval("imppwr4");
    getstr("impshp1", Ishp1);
    getstr("impshp4", Ishp2);
  }

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

  settable(t1, 1, ph1);
  settable(t2, 1, ph2);
  settable(t3, 2, ph3);
  settable(t4, 16, ph4);
  settable(t5, 4, ph5);
  settable(t6, 16, ph6);

  getelem(t1, v17, v1);
  getelem(t2, v17, v2);
  getelem(t3, v17, v3);
  getelem(t4, v17, v4);
  getelem(t5, v17, v5);
  getelem(t6, v17, oph);

  assign(zero,v6);
  add(oph,v18,oph);
  add(oph,v19,oph);

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
/*
  mod2(id2, v14);
  dbl(v14,v14);
*/
  add(v3, v14, v3);
  add(oph, v14, oph);

  if ((phase1 == 2) || (phase1 == 5))
  {
    icosel = -1;
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

    rgpulse(pw, v1, rof1, rof2);

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
     delay(gstab);
    }

/* End of J filter */

    obspower(tpwr180);
    delay(taumb);

    decrgpulse(pwx, v3, rof1, rof1);

    delay(d2 / 2.0);
    shaped_pulse(pw180ad, pw180, zero, rof1, rof1);
    delay(d2 / 2.0);
    decpower(Ipwr1);
    decshaped_pulse(Ishp1, Ipw1, v2, rof1, rof1);
    delay(pw180 + POWER_DELAY + WFG_START_DELAY + WFG_STOP_DELAY + 4 * rof1 + 4 * pwx / PI);
    zgradpulse(icosel * grad1, gtE);
    delay(gstab);
    decpower(Ipwr2);
    decshaped_pulse(Ishp2, Ipw2, v4, rof1, rof1);
    zgradpulse(icosel * grad2, gtE);
    decpower(pwxlvl);
    delay(gstab);

    decrgpulse(pwx, v5, rof1, rof2);
    decpower(dpwr);

  status(C);
}
