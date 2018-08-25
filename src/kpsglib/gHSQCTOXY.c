// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* gHSQCTOXY - Gradient Selected phase-sensitive HSQCTOXY

	Paramters:
                sspul :         selects magnetization randomization option
                nullflg:        selects TANGO-Gradient option
                hsglvl:         Homospoil gradient level (DAC units)
                hsgt    :       Homospoil gradient time
                gzlvlE  :       encoding Gradient level
                gtE     :       encoding gradient time
                EDratio :       Encode/Decode ratio
                gstab   :       recovery delay
                j1xh    :       One-bond XH coupling constant
                mixT    :       TOCSY spinlock mixing time
                slpatT  :       TOCSY spinlock pattern (mlev17c,mlev17,dipsi2,dipsi3)
                trim    :       trim pulse preceeding spinlock
                slpwrT  :       spin-lock power level
                slpwT   :       90 deg pulse width for spinlock

************************************************************************
****NOTE:  v2,v3,v4,v5 and v9 are used by Hardware Loop and reserved ***
   v3 and v5 are spinlock phase
************************************************************************

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004

*/

#include <standard.h>
#include <chempack.h>
extern int dps_flag;

static int	ph1[4] = {1,1,3,3},
		ph2[2] = {0,2},
		ph3[8] = {0,0,0,0,2,2,2,2},
		ph4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
		ph5[16] = {1,3,3,1,3,1,1,3,3,1,1,3,1,3,3,1};

pulsesequence()

{
   double   gzlvlE = getval("gzlvlE"),
   	    gtE = getval("gtE"), 
	    EDratio = getval("EDratio"),
	    gstab = getval("gstab"),
            hsglvl = getval("hsglvl"),
            hsgt = getval("hsgt"),
            slpwT = getval("slpwT"),
            slpwrT = getval("slpwrT"),
            trim = getval("trim"),
            mixT = getval("mixT"),
            mult = getval("mult"),
	    evolcorr,
            tau;
   int	    icosel,
	    phase1 = (int)(getval("phase")+0.5);
   char	    slpatT[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",0.5);
        gzlvlE = syncGradLvl("gtE","gzlvlE",0.5);

   getstr("slpatT",slpatT);
   evolcorr=2*pw+4.0e-6;

   if (strcmp(slpatT,"mlev17c") &&
        strcmp(slpatT,"dipsi2") &&
        strcmp(slpatT,"dipsi3") &&
        strcmp(slpatT,"mlev17") &&
        strcmp(slpatT,"mlev16"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatT);

   tau  = 1/(4*(getval("j1xh")));
   icosel = 1;

   settable(t1,4,ph1);
   settable(t2,2,ph2);
   settable(t3,8,ph3);
   settable(t4,16,ph4);
   settable(t5,16,ph5);

   assign(two,v3);
   sub(v3,one,v2);
   add(v3,one,v4);
   add(v3,two,v5);

   getelem(t2,ct,v6);
   getelem(t5,ct,oph);
/*
   mod2(id2,v14);
   dbl(v14,v14);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   if ((phase1 == 2) || (phase1 == 5))
     icosel = -1;

   add(v6,v14,v6);
   add(oph,v14,oph);

status(A);
  obspower(tpwr);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        satpulse(satdly,zero,rof1,rof1);
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   decpower(pwxlvl);

status(B);

      if (getflag("nullflg"))
      {
        rgpulse(0.5*pw,zero,rof1,rof1);
        delay(2*tau);
        simpulse(2.0*pw,2.0*pwx,zero,zero,rof1,rof1);
        delay(2*tau);
        rgpulse(1.5*pw,two,rof1,rof1);
        zgradpulse(hsglvl,hsgt);
        delay(1e-3);
      }

     rgpulse(pw,zero,rof1,rof1);
     delay(tau);
     simpulse(2*pw,2*pwx,zero,zero,rof1,rof1);
     delay(tau);
     rgpulse(pw,t1,rof1,rof1);
	zgradpulse(hsglvl,2*hsgt);
	delay(1e-3);
     decrgpulse(pwx,v6,rof1,2.0e-6);
      delay(d2/2);
     rgpulse(2*pw,zero,2.0e-6,2.0e-6);
      delay(d2/2);

     delay(gtE+gstab + 2*GRADIENT_DELAY);
     decrgpulse(2*pwx,zero,rof1,rof1);
     zgradpulse(gzlvlE,gtE);
     delay(gstab + evolcorr);

     decrgpulse(pwx,t4,2.0e-6,rof1);
	zgradpulse(-0.6*hsglvl,1.2*hsgt);
	delay(1e-3);
     rgpulse(pw,t3,rof1,rof1);
     delay(tau - (2*pw/PI) - rof1);
     simpulse(2*pw,2*pwx,zero,zero,rof1, rof1);
     delay(tau - POWER_DELAY);
     obspower(slpwrT);

     if (mixT > 0.0)
     {         
        rgpulse(trim,v5,0.0,0.0);
        if (dps_flag)
          rgpulse(mixT,v3,0.0,0.0);
        else
          SpinLock(slpatT,mixT,slpwT,v2,v3,v4,v5,v9);
     }  

      obspower(tpwr);
      if (mult > 0.5)
       delay(2*tau - rof1);
      else
	delay(gtE/2 + gstab + 2*GRADIENT_DELAY - rof1);
      simpulse(2*pw,mult*pwx,zero,zero,rof1,rof2);
      decpower(dpwr);
      zgradpulse(icosel*2.0*gzlvlE/EDratio,gtE/2.0);
      if (mult > 0.5)
        delay(2*tau - gtE/2.0 - 2*GRADIENT_DELAY);
      else
	delay(gstab);


   status(C);
}
