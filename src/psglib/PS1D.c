// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*   PS1D - Pure shift 1D experiment

KrishK - includes purge option : Oct. 2007
****v17,v18,v19 are reserved for PURGE ***

2011-06-25 
  KrishK - single slice selective pulse (2nd gen seqeunce from GM)
2012-08-06
  KrishK - Modified to behave like 2D (using 2D parameters)
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/

#include <standard.h>
#include <chempack.h>

static int	ph1[4] = {0,1,2,3},
		ph2[32] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
			   2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3},
		ph3[8] = {0,0,1,1,2,2,3,3},
		ph4[16] = {0,1,0,1,0,1,0,1,2,3,2,3,2,3,2,3};

pulsesequence()
{

   double	   gtE = getval("gtE"),
		   gzlvlE=getval("gzlvlE"),
		   selpwrPS = getval("selpwrPS"),
		   selpwPS = getval("selpwPS"),
		   gzlvlPS = getval("gzlvlPS"),
		   droppts=getval("droppts"),
		   gstab = getval("gstab");
   int 		   prgcycle=(int)(getval("prgcycle")+0.5);
   char		   selshapePS[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

   getstr("selshapePS",selshapePS);

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

   settable(t1,4,ph1);
   settable(t2,32,ph2);
   settable(t3,8,ph3);
   settable(t4,16,ph4);

   getelem(t1,v17,v1);
   getelem(t2,v17,v2);
   getelem(t3,v17,v3);
   getelem(t4,v17,v4);
   assign(v4,oph);

   assign(v1,v6);
   add(oph,v18,oph);
   add(oph,v19,oph);

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
      obspower(tpwr);
        if (getflag("cpmgflg"))
        {
          rgpulse(pw, v1, rof1, 0.0);
          cpmg(v1, v15);
        }
        else
          rgpulse(pw, v1, rof1, rof2);

	delay(d2/2.0);

	delay((0.25/sw1) - gtE - gstab - 2*pw/PI - rof2 - 2*GRADIENT_DELAY);
	zgradpulse(gzlvlE,gtE);
	delay(gstab);
	rgpulse(2*pw,v2,rof1,rof1);
	delay(0.25/sw1);

	delay(gstab);
	zgradpulse(-1.0*gzlvlE,gtE);
	delay(gstab);
	obspower(selpwrPS);
	rgradient('z',gzlvlPS);
	shaped_pulse(selshapePS,selpwPS,v3,rof1,rof1);
	rgradient('z',0.0);
	obspower(tpwr);
	delay(gstab);
	zgradpulse(-2.0*gzlvlE,gtE);
	delay(gstab - droppts/sw);

	delay(d2/2.0);

   status(C);
}
