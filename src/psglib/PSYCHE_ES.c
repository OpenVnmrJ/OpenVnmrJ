// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*   PSYCHE_ES
     PureShift Yielded by CHirp Excitation with Excitation Sculpting
     Reference:
	Foroozandeh, M; Adams, RW; Meharry, NJ; Jeannerat, D, Nilsson, M; Morris, GA, Angew. Chem., Int. Ed., 2014, 53, 6990.
 
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/

#include <standard.h>
#include <chempack.h>

static int	ph1[2] = {0,2},
		ph2[8] = {2,2,2,2,3,3,3,3},
		ph3[8] = {0,0,0,0,1,1,1,1},
		ph4[4] = {2,2,3,3},
		ph5[4] = {0,0,1,1},
		ph6[4] = {0,0,1,1},
		ph7[8] = {0,2,2,0,2,0,0,2};

void pulsesequence()
{

   double	   gt1 = getval("gt1"),
		   gzlvl1=getval("gzlvl1"),
		   gt2 = getval("gt2"),
		   gzlvl2=getval("gzlvl2"),
		   selpwrPS = getval("selpwrPS"),
		   selpwPS = getval("selpwPS"),
		   gzlvlPS = getval("gzlvlPS"),
		   droppts=getval("droppts"),
		   gstab = getval("gstab"),
  		   espw180 = getval("espw180"),
                   essoftpw = getval("essoftpw"),
                   essoftpwr = getval("essoftpwr"),
                   essoftpwrf = getval("essoftpwrf");
   char		   selshapePS[MAXSTR],
		   esshape[MAXSTR];

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gt1 = syncGradTime("gt1","gzlvl1",1.0);
        gzlvl1 = syncGradLvl("gt1","gzlvl1",1.0);
	gt2 = syncGradTime("gt2","gzlvl2",1.0);
        gzlvl2 = syncGradLvl("gt2","gzlvl2",1.0);

   getstr("esshape",esshape);

   getstr("selshapePS",selshapePS);

  assign(ct,v17);

   settable(t1,2,ph1);
   settable(t2,8,ph2);
   settable(t3,8,ph3);
   settable(t4,4,ph4);
   settable(t5,4,ph5);
   settable(t6,4,ph6);
   settable(t7,8,ph7);

   getelem(t1,v17,v1);
   getelem(t2,v17,v2);
   getelem(t3,v17,v3);
   getelem(t4,v17,v4);
   getelem(t5,v17,v5);
   getelem(t6,v17,v6);
   getelem(t7,v17,v7);
   assign(v7,oph);

   /*assign(v1,v6);
   add(oph,v18,oph);
   add(oph,v19,oph);*/

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
      obspower(tpwr);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

  delay(d1);

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

	zgradpulse(gzlvl1,gt1);
	delay(gstab);
	delay((0.25/sw1) - gt1 - gstab - 2*pw/PI - rof2 - 2*GRADIENT_DELAY - pw - rof1-essoftpw/2.0-2.0e-6);
        obspower(essoftpwr+6); obspwrf(essoftpwrf);
        shaped_pulse(esshape,essoftpw,v2,2.0e-6,2.0e-6);
        obspower(tpwr); obspwrf(4095.0);
	rgpulse(2.0*pw,v3,rof1,rof1);
	zgradpulse(gzlvl1,gt1);
	delay(gstab);
	delay((0.25/sw1) - gt1 - gstab - 2*GRADIENT_DELAY - pw - rof1 - essoftpw/2.0-2.0e-6);

	delay(gstab);
	zgradpulse(gzlvl2,gt2);
	delay(gstab);
        obspower(essoftpwr+6); obspwrf(essoftpwrf);
        shaped_pulse(esshape,essoftpw,v4,2.0e-6,2.0e-6);
	obspower(selpwrPS); obspwrf(4095.0) ;
	rgradient('z',gzlvlPS);
	shaped_pulse(selshapePS,selpwPS,v5,rof1,rof1);
	rgradient('z',0.0);
	delay(gstab);
	obspower(tpwr);
	zgradpulse(gzlvl2,gt2);
	delay(gstab - droppts/sw);
	delay(d2/2.0);
        obspower(tpwr); obspwrf(4095.0);

   status(C);
}
