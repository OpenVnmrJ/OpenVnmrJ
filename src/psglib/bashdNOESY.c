// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* bashDnosy -  BAnd-Selected Homonuclear Decoupled NOESY 
            States-Hypercomplex only

	Note: very similar to bashdtoxy.  See bashdtoxy for more details.
	Krish	Aug, 1995	
Updated for CP4 Jan, 2008
	- phase cycling similar to NOESY
	- Includes all CP options
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***
*/


#include <standard.h>
#include <chempack.h>

static int phs1[32] = {0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,1,1,1,1,3,3,3,3},
           phs2[32] = {2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1},
           phs3[32] = {0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,1,1,1,1,3,3,3,3},
           phs4[32] = {2,0,2,0,0,2,0,2,3,1,3,1,1,3,1,3,3,1,3,1,1,3,1,3,0,2,0,2,2,0,2,0},
           phs5[32] = {0,0,2,2,0,0,2,2,1,1,3,3,1,1,3,3,1,1,3,3,1,1,3,3,2,2,0,0,2,2,0,0},
           phs6[32] = {0,2,2,0,2,0,0,2,1,3,3,1,3,1,1,3,1,3,3,1,3,1,1,3,2,0,0,2,0,2,2,0};

void pulsesequence()
{
   double          selpwrA = getval("selpwrA"),
                   selpwA = getval("selpwA"),
                   gzlvlA = getval("gzlvlA"),
                   gtA = getval("gtA"),
                   selpwrB = getval("selpwrB"),
                   selpwB = getval("selpwB"),
                   gzlvlB = getval("gzlvlB"),
                   gtB = getval("gtB"),
                   gstab = getval("gstab"),
		   mixN = getval("mixN"),
                   gzlvl1 = getval("gzlvl1"),
                   gt1 = getval("gt1"),
                   zqfpw1 = getval("zqfpw1"),
                   zqfpwr1 = getval("zqfpwr1"),
                   gzlvlzq1 = getval("gzlvlzq1"),
		   mixNcorr,
		   alfa1,
		   t1dly;
   int             phase1 = (int)(getval("phase")+0.5),
		   prgcycle = (int)(getval("prgcycle")+0.5);
   char            selshapeA[MAXSTR],
                   selshapeB[MAXSTR],
                   zqfpat1[MAXSTR],
                   wet[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtA = syncGradTime("gtA","gzlvlA",1.0);
        gzlvlA = syncGradLvl("gtA","gzlvlA",1.0);
        gtB = syncGradTime("gtB","gzlvlB",1.0);
        gzlvlB = syncGradLvl("gtB","gzlvlB",1.0);

   getstr("selshapeA",selshapeA);
   getstr("selshapeB",selshapeB);
   getstr("wet",wet);
   getstr("zqfpat1",zqfpat1);
   mixNcorr = 0.0;
   if (getflag("PFGflg"))
   {
        mixNcorr = gt1 + gstab;
        if (getflag("Gzqfilt"))
                mixNcorr += gstab + zqfpw1;
        if (wet[1] == 'y')
                mixNcorr += 4*(getval("pwwet")+getval("gtw")+getval("gswet"));
   }

   if (mixNcorr > mixN)
        mixN=mixNcorr;

   alfa1 = 2.0e-6+(4*pw/PI);
   if (getflag("homodec"))
	alfa1 = alfa1 + 2.0e-6 + 2*pw;
   t1dly = d2-alfa1;
   if (t1dly > 0.0)
	t1dly = t1dly;
   else
	t1dly = 0.0;

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

   settable(t1,32,phs1);
   settable(t2,32,phs2);
   settable(t3,32,phs3);
   settable(t5,32,phs5);
   settable(t4,32,phs4);
   settable(t6,32,phs6);

   getelem(t1,v17,v6);   /* v6 - presat */
   getelem(t2,v17,v1);   /* v1 - first 90 */
   getelem(t3,v17,v2);   /* v2 - 2nd 90 */
   getelem(t4,v17,v7);   /* v7 - presat during mixN */
   getelem(t5,v17,v3);   /* v3 - 3rd 90 */
   getelem(t6,v17,oph);  /* oph - receiver */

   add(oph,v18,oph);
   add(oph,v19,oph);

   assign(v1,v13);
   if (phase1 == 2) 
      {incr(v1);  incr(v6); }

   assign(v1,v8);
   add(v8,two,v9);

   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
   if (getflag("fadflg"))
   {
   add(v1, v14, v1);
   add(oph,v14,oph);
   }

   if (getflag("homodec"))
	add(oph,two,oph);

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
                shaped_satpulse("relaxD",satdly,v6);
                if (getflag("prgflg"))
                   shaped_purge(v1,v6,v18,v19);
             }

        else
             {
                satpulse(satdly,v6,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,v6,v18,v19);
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
         rgpulse(pw, v1, rof1, 1.0e-6);

      if (getflag("homodec"))
       {
                delay(t1dly/2);
                rgpulse(2*pw,v13,1.0e-6,1.0e-6);
       }
      else
                delay(t1dly);

        zgradpulse(gzlvlA,gtA);
        delay(gstab);
        obspower(selpwrA);
        shaped_pulse(selshapeA,selpwA,v8,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlA,gtA);
        delay(gstab);

      if (getflag("homodec"))
                delay(t1dly/2);

        zgradpulse(gzlvlB,gtB);
        delay(gstab);
        obspower(selpwrB);
        shaped_pulse(selshapeB,selpwB,v9,rof1,rof1);
        obspower(tpwr);
        zgradpulse(gzlvlB,gtB);
        delay(gstab);

      rgpulse(pw,v2,1.0e-6,1.0e-6); 

      if (satmode[1] == 'y')
        satpulse((mixN-mixNcorr)*0.7,zero,rof1,rof1);
      else
        delay((mixN - mixNcorr)*0.7);

      if (getflag("PFGflg"))
      {
        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr1);
         rgradient('z',gzlvlzq1);
         delay(100.0e-6);
         shaped_pulse(zqfpat1,zqfpw1,zero,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(gstab);
         obspower(tpwr);
        }
        zgradpulse(gzlvl1,gt1);
        delay(gstab);
      }
      if (satmode[1] == 'y')
        {
        if (getflag("slpsat"))
                shaped_satpulse("mixN2",(mixN-mixNcorr)*0.3,v7);
        else
                satpulse((mixN-mixNcorr)*0.3,v7,rof1,rof1);
        }
      else
         delay((mixN - mixNcorr)*0.3);
      if (wet[1] == 'y')
        wet4(zero,one);
   status(C);
      rgpulse(pw,v3,1.0e-6,rof2);

}
