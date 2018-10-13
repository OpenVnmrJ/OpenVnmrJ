// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* gDQCOSY - Gradient DQ selected Phase-sensitive DQCOSY
        Features included:
        	States-TPPI in F1

        Paramters:
                sspul   :       selects magnetization randomization option
                gzlvlE  :       encoding gradient level
                gtE     :       encoding gradient time
		EDratio	:	Encode/decode ratio [=0.5]
                gstab   :       Recovery delay

KrishK  -       last revision:  June 1997
KrishK	-	Revised: July 2004


*/


#include <standard.h>
#include <chempack.h>

static int 	ph1[8] = {0, 2, 0, 2, 1, 3, 1, 3},
                ph2[8] = {1, 1, 3, 3, 2, 2, 0, 0},
		ph3[8] = {0, 2, 0, 2, 1, 3, 1, 3},
		ph6[1] = {0};

pulsesequence()
{
	double	gzlvlE = getval("gzlvlE"),
		gtE = getval("gtE"),
		EDratio = getval("EDratio"),
		gstab = getval("gstab"),
		gstab2 = getval("gstab2");
	int	phase1 = (int)(getval("phase")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

	settable(t1,8,ph1);
	settable(t2,8,ph2);
	settable(t3,8,ph3);
	settable(t6,1,ph6);

	getelem(t1,ct,v1);
	getelem(t2,ct,v2);
	getelem(t6,ct,v6);
	getelem(t3,ct,oph);

	if (phase1 == 2)
	 { incr(v1); incr(v6); }
/*	
	mod2(id2,v10);
	dbl(v10,v10);
*/

	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);
	add(v1,v10,v1);
	add(oph,v10,oph);

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
        satpulse(satdly,v6,rof1,rof1);
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

status(B);

	rgpulse(pw, v1, rof1, 2.0e-6);
	if (d2 > 0.0)
	 delay(d2 - (4*pw/PI) - 4.0e-6);
	else
	 delay(d2); 
	rgpulse(pw, v2, 2.0e-6, rof1);
	delay(gtE + gstab + 2*GRADIENT_DELAY);
	rgpulse(2*pw, v2, rof1, rof1);
	zgradpulse(-gzlvlE,gtE);
	delay(gstab);
	rgpulse(pw, v2, rof1, rof2);
	zgradpulse(gzlvlE/EDratio,gtE);
	delay(gstab2 - 2*GRADIENT_DELAY);

status(C);
}
