/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* wgdqfcosy - Gradient DQ selected Phase-sensitive DQFCOSY
        Features included:
        	States-TPPI in F1
                Randomization of Magnetization prior to relaxation delay
                        with G-90-G
                        [selected by sspul flag]
                watergate-3919 watersuppression

        Paramters:
                sspul   :       y - selects magnetization randomization option
               watergate:       y - selects 3919 watergate
                gzlvl0  :       Homospoil gradient level (DAC units)
                gt0     :       Homospoil gradient time
                gzlvl1  :       Encoding gradient level
                gt1     :       Encoding gradient time
                gzlvl2  :	Decoding gradient level
                gt2 	:	Decoding gradient time
                gzlvl3  :       Watergate gradient level
                gt3     :       Watergate gradient time
                gstab   :       Recovery delay
                pw      :       First and second pulse widths
                d1      :       relaxation delay
                d2      :       Evolution delay

Nagarajan Murali, Varian - Sep 24, 2004
*/


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
                gzlvl3,
                gt3,
                tau,
		gstab,
		gzlvl0,
		gt0;
	char	watergate[MAXSTR],
		sspul[MAXSTR];
	int	iphase;

	gzlvl1 = getval("gzlvl1");
	gt1 = getval("gt1");
	gzlvl2 = getval("gzlvl2");
	gt2 = getval("gt2");
        gzlvl3 = getval("gzlvl3");
        gt3 = getval("gt3");
	gstab = getval("gstab");
	tau = getval("tau");
	gzlvl0 = getval("gzlvl0");
	gt0 = getval("gt0");
	getstr("sspul",sspul);
        getstr("watergate",watergate);
	iphase = (int)(getval("phase")+0.5);

	settable(t1,8,ph1);
	settable(t2,8,ph2);
	settable(t3,8,ph3);

	getelem(t1,ct,v1);
	getelem(t2,ct,v2);
        add(v2,two,v3);
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
		zgradpulse(gzlvl0,gt0);
		rgpulse(pw,zero,rof1,rof1);
		zgradpulse(gzlvl0,gt0);
	}
	delay(d1);

status(B);

	rgpulse(pw, v1, 2.0e-6, 2.0e-6);
	if (d2 > 0.0)
	 delay(d2 - (4*pw/PI) - 4.0e-6);
	else
	 delay(d2); 
	rgpulse(pw, v2, 2.0e-6, rof2);
	delay(gt1 + gstab + 2*GRADIENT_DELAY);
	rgpulse(2*pw, v2, rof1, rof2);
	zgradpulse(-gzlvl1,gt1);
	delay(gstab);
	rgpulse(pw, v2, rof1, rof2);
	zgradpulse(gzlvl2,gt2);
	delay(gstab - 2*GRADIENT_DELAY);
        if (watergate[0] == 'y')
	{
	
       		delay(tau-pw/2);
       		zgradpulse(gzlvl3,gt3);
       		delay(gstab);
       		pulse(pw*0.231,v2);
       		delay(d3);
       		pulse(pw*0.692,v2);
       		delay(d3);
       		pulse(pw*1.462,v2);
       		delay(d3);
       		pulse(pw*1.462,v3);
       		delay(d3);
       		pulse(pw*0.692,v3);
       		delay(d3);
       		pulse(pw*0.231,v3);
                zgradpulse(gzlvl3,gt3);
		delay(gstab);
                delay(tau);
        }


status(C);
}
