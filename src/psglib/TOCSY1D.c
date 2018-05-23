// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*  TOCSY1D - DPFGSE-TOCSY experiment

	Paramters:
		sspul :		selects magnetization randomization option
		mixT	:	TOCSY spinlock mixing time
		slpatT	:	TOCSY pattern [mlev17, mlev17c, dipsi2, dipsi3]
		trim	:	trim pulse preceeding spinlock
		slpwrT	:	spin-lock power level
		slpwT	:	90 deg pulse width for spinlock
		selshapeA, selpwrA, selpwA, gzlvlA, gtA -
			:	shape, power, pulse, level and time for
				first PFG echo
		selshapeB, selpwrB, selpwB, gzlvlB, gtB -
				shape, power, pulse, level and time for
				2nd PFG echo
                gstab   :       Gradient recovery delay
		selfrq	:	Selective frequency (for selective 180)

************************************************************************
****NOTE:  v20,v21,v22,v23 and v24 are used by Hardware Loop and reserved ***
   v21 and v23 are spinlock phase
************************************************************************

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/


#include <standard.h>
#include <chempack.h>

extern int dps_flag;

static int	ph1[4] = {0,2,3,1},
		ph2[4] = {2,2,1,1},
		ph3[8] = {1,1,0,0,3,3,2,2},
		ph4[8] = {0,2,3,1,2,0,1,3},
		ph7[8] = {2,2,1,1,0,0,3,3},
		ph8[4] = {0,0,3,3};

pulsesequence()
{
   double	   slpwrT = getval("slpwrT"),
		   slpwT = getval("slpwT"),
		   mixT = getval("mixT"),
		   trim = getval("trim"),
		   tauz1 = getval("tauz1"), 
		   tauz2 = getval("tauz2"), 
		   tauz3 = getval("tauz3"), 
		   tauz4 = getval("tauz4"),
                   selpwrA = getval("selpwrA"),
                   selpwA = getval("selpwA"),
                   gzlvlA = getval("gzlvlA"),
                   gtA = getval("gtA"),
                   selpwrB = getval("selpwrB"),
                   selpwB = getval("selpwB"),
                   gzlvlB = getval("gzlvlB"),
                   gtB = getval("gtB"),
                   gstab = getval("gstab"),
		   selfrq = getval("selfrq");
   char            selshapeA[MAXSTR],
                   selshapeB[MAXSTR],
   		   slpatT[MAXSTR];
   int 		   prgcycle=(int)(getval("prgcycle")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("slpatT",slpatT);
   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);

   if (strcmp(slpatT,"mlev17c") &&
        strcmp(slpatT,"dipsi2") &&
        strcmp(slpatT,"dipsi3") &&
        strcmp(slpatT,"mlev17") &&
        strcmp(slpatT,"mlev16"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatT);

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

   assign(v17,v6);
   if (getflag("zqfilt")) 
     {  hlv(v6,v6); hlv(v6,v6); }

   settable(t1,4,ph1);   getelem(t1,v6,v1);
   settable(t3,8,ph3);   getelem(t3,v6,v11);
   settable(t4,8,ph4);   
   settable(t2,4,ph2);   getelem(t2,v6,v2);
   settable(t7,8,ph7);   getelem(t7,v6,v7);
   settable(t8,4,ph8);   getelem(t8,v6,v8);
   
   if (getflag("zqfilt"))
     getelem(t4,v6,oph);
   else
     assign(v1,oph);

   add(oph,v18,oph);
   add(oph,v19,oph);

   assign(v6,v9);
  if (getflag("prgflg") && (satmode[0] == 'y'))
        assign(v1,v9);

   sub(v2,one,v21);
   add(v21,two,v23);

   mod4(ct,v10);

/* BEGIN THE ACTUAL PULSE SEQUENCE */
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
        if (getflag("slpsat"))
           {
                shaped_satpulse("relaxD",satdly,v9);
                if (getflag("prgflg"))
                   shaped_purge(v1,v9,v18,v19);
           }
        else
           {
                satpulse(satdly,v9,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,v9,v18,v19);
           }

     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   status(B);
      if (getflag("cpmgflg"))
      {
        rgpulse(pw, v1, rof1, 0.0);
        cpmg(v1, v15);
      }
      else
        rgpulse(pw, v1, rof1, rof1);

      if (selfrq != tof)
	obsoffset(selfrq);

        zgradpulse(gzlvlA,gtA);
        delay(gstab);
        obspower(selpwrA);
        shaped_pulse(selshapeA,selpwA,v1,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlA,gtA);
        delay(gstab);

      if (selfrq != tof)
        delay(2*OFFSET_DELAY);

        zgradpulse(gzlvlB,gtB);
        delay(gstab);
        obspower(selpwrB);
        shaped_pulse(selshapeB,selpwB,v2,rof1,rof1);
        obspower(slpwrT);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);

      if (selfrq != tof)
	obsoffset(tof);

     if (mixT > 0.0)
      { 
        rgpulse(trim,v11,0.0,0.0);
        if (dps_flag)
          rgpulse(mixT,v21,0.0,0.0);
        else
          SpinLock(slpatT,mixT,slpwT,v21);
       }

      if (getflag("zqfilt"))
      {
	obspower(tpwr);
	rgpulse(pw,v7,1.0e-6,rof1);
	ifzero(v10); delay(tauz1); endif(v10);
	decr(v10);
	ifzero(v10); delay(tauz2); endif(v10);
	decr(v10);
	ifzero(v10); delay(tauz3); endif(v10);
	decr(v10);
	ifzero(v10); delay(tauz4); endif(v10);
	rgpulse(pw,v8,rof1,rof2);
      }
      else
	 delay(rof2);

   status(C);
}
