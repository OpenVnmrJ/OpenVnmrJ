// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* wetgDQCOSY - Gradient DQ selected Phase-sensitive DQCOSY */

#include <standard.h>

static int 	ph1[8] = {0, 2, 0, 2, 1, 3, 1, 3},
		ph2[8] = {0, 0, 2, 2, 1, 1, 3, 3},
		ph3[8] = {0, 2, 0, 2, 1, 3, 1, 3};

void pulsesequence()
{
	double	gzlvl1,
		gt1,
		gzlvl2,
		gt2,
		gstab,
		gstab2,
		hsglvl,
		hsgt;
	char	sspul[MAXSTR];
	int	iphase;

	gzlvl1 = getval("gzlvl1");
	gt1 = getval("gt1");
	gzlvl2 = getval("gzlvl2");
	gt2 = getval("gt2");
	gstab = getval("gstab");
	gstab2 = getval("gstab2");
	hsglvl = getval("hsglvl");
	hsgt = getval("hsgt");
	getstr("sspul",sspul);
	iphase = (int)(getval("phase")+0.5);

	settable(t1,8,ph1);
	settable(t2,8,ph2);
	settable(t3,8,ph3);

	getelem(t1,ct,v1);
	getelem(t2,ct,v2);
	getelem(t3,ct,oph);

	if (iphase == 2)
	 incr(v1);

	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);
		add(v1,v10,v1);
		add(oph,v10,oph);

status(A);

 	delay(5.0e-5);
	if (sspul[0] == 'y')
	{
		zgradpulse(hsglvl,hsgt);
		rgpulse(pw,zero,rof1,rof1);
		zgradpulse(hsglvl,hsgt);
	}
	delay(d1);

	if (getflag("wet"))
	 wet4(zero,one);
status(B);

	rgpulse(pw, v1, rof1, 2.0e-6);
	if (d2 > (4.0*pw/PI + 4.0e-6))
	 delay(d2 - (4.0*pw/PI) - 4.0e-6);
	 
	else {
	if (ix == 1)
	  dps_show("delay",d2);
	else if ((ix > 2) && (iphase < 2))
	  text_error("increment %d cannot be timed properly\n", (int) ix/2);
	}

	rgpulse(pw, v2, 2.0e-6, rof1);
	delay(gt1 + gstab + 2*GRADIENT_DELAY);
	rgpulse(2*pw, v2, rof1, rof1);
	zgradpulse(-gzlvl1,gt1);
	delay(gstab);
	rgpulse(pw, v2, rof1, rof2);
	zgradpulse(gzlvl2,gt2);
	delay(gstab2 - 2*GRADIENT_DELAY);

status(C);
}
