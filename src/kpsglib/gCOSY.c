// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* gCOSY - Gradient selected COSY

	Features included:
		F1 Axial displacement
		
	Parameters:
		sspul   :	selects magnetization randomization option
		gzlvlE	:	Encode gradient level
		gtE	:	Encode gradient time
		EDratio	:	Encode/Decode ratio [=1]
		cmult	:	selects COSY-45/COSY-90/COSY-135 (0.5/1/1.5)	

KrishK	-	First revision:	June 1997
		Modified: June 2004
BHeise  - 	Includes COSY-beta/COSY-45 option: June 2009

*/


#include <standard.h>
#include <chempack.h>

static int 	ph1[4] = {0, 2, 1, 3},
		ph2[4] = {0, 2, 1, 3},
		ph3[4] = {0, 2, 1, 3};

void pulsesequence()
{
	double	gzlvlE 	 = getval("gzlvlE"),
		gtE 	 = getval("gtE"),
		EDratio  = getval("EDratio"),
		gstab 	 = getval("gstab"),
		cmult    = getval("cmult");

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",1.0);
        gzlvlE = syncGradLvl("gtE","gzlvlE",1.0);

	settable(t1,4,ph1);
	settable(t2,4,ph2);
	settable(t3,4,ph3);

	getelem(t1,ct,v1);
	getelem(t2,ct,v2);
	getelem(t3,ct,oph);

/*
	mod2(id2,v10); dbl(v10,v10);
*/
  	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);

        add(v1,v10,v1);
        add(oph,v10,oph);

	assign(v1,v4);

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
        satpulse(satdly,v4,rof1,rof1);

     }
   else
	delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

status(B);
	rgpulse(pw, v1, rof1, rof1);
	delay(d2); 
	zgradpulse(gzlvlE,gtE);
	delay(gstab);
	rgpulse(cmult*pw, v2, rof1, rof2);
	zgradpulse(gzlvlE/EDratio,gtE);
	delay(gstab);

status(C);
}
