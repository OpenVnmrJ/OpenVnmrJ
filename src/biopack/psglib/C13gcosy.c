/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* C13gcosy - Gradient selected COSY for 13C observe

	Features included:
		F1 Axial displacement
		
	Paramters:
		gzlvl1	:	Coherence selection gradient level
		gt1	:	Gradient time
		gstab	:	Recovery delay
		pw	:	First and second pulse widths
		d1	:	relaxation delay
		d2	:	Evolution delay

*/

#include <standard.h>

static int 	ph1[4] = {0, 2, 1, 3},
		ph2[4] = {0, 2, 1, 3},
		ph3[4] = {0, 2, 1, 3};

void pulsesequence()
{
	double	gzlvl1,pwClvl,
		gt1,
		gstab;

	gzlvl1 = getval("gzlvl1");
	gt1 = getval("gt1");
	gstab = getval("gstab");
        pwClvl = getval("pwClvl");

	settable(t1,4,ph1);
	settable(t2,4,ph2);
	settable(t3,4,ph3);

	getelem(t1,ct,v1);
	getelem(t2,ct,v2);
	getelem(t3,ct,oph);

	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);
		add(v1,v10,v1);
		add(oph,v10,oph);

status(A);
        obspower(pwClvl);
        delay(d1); 

status(B);
	rgpulse(pw, v1, rof1, rof1);
	delay(d2); 
	zgradpulse(gzlvl1,gt1);
	delay(gstab);
	rgpulse(pw, v2, rof1, rof2);
	zgradpulse(gzlvl1,gt1);
	delay(gstab);

status(C);
}
