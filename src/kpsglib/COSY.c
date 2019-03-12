// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* COSY - homonuclear correlation spectroscopy (absolute value)

	Paramters:
		sspul   :	selects magnetization randomization option
		cmult   :	selects COSY-45/COSY-90/COSY-135 (0.5/1/1.5)	

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004
BHeise  - 	Includes COSY-beta/COSY-45 option

*/


#include <standard.h>
#include <chempack.h>

static int 	ph1[8] = {0, 0, 1, 1, 2, 2, 3, 3},
		ph2[8] = {0, 1, 3, 0, 2, 3, 1, 2},
		ph3[8] = {0, 2, 1, 3, 2, 0, 3, 1};

void pulsesequence()
{
        double cmult = getval("cmult");

	settable(t1,8,ph1);
	settable(t2,8,ph2);
	settable(t3,8,ph3);

	getelem(t1,ct,v1);
	getelem(t2,ct,v2);
	getelem(t3,ct,oph);

/*
	mod2(id2,v10);
	dbl(v10,v10);
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
	rgpulse(cmult*pw, v2, rof1, rof2);

status(C);
}
