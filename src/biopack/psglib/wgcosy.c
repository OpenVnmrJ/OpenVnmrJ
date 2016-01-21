/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* wgcosy - watergate(3919) Phase-sensitive COSY
        Features included:
        	States-TPPI in F1
                Randomization of Magnetization prior to relaxation delay
                        with G-90-G
                        [selected by sspul flag]
                watergate-3919 watersuppression activated by watergate flag. 
                Initial water flipback with watergate.

        Paramters:
                sspul   :       y - selects magnetization randomization option
               watergate:	y - selects 3919 watergate 
                gzlvl0  :       Homospoil gradient level (DAC units)
                gt0     :       Homospoil gradient time
                gzlvl3  :	watergate gradient level
                gt3 	:	watergate gradient time
                gstab   :       Recovery delay
                pw      :       First and second pulse widths
                d1      :       relaxation delay
                d2      :       Evolution delay

Nagarajan Murali, Varian - September 23, 2004
*/


#include <standard.h>
static int 	ph1[4] = {0, 2, 1, 3},
		ph2[8] = {0, 0, 1, 1, 2, 2, 3, 3},
		ph3[4] = {0, 2, 1, 3};

pulsesequence()
{
	double	gzlvl3,
		gt3,
                tpwrs,
                pwHs,
                compH,
                cor,
                tau,
		gstab,
		gzlvl0,
		gt0;
	char	watergate[MAXSTR],
		sspul[MAXSTR];
	int	iphase;

        gzlvl3 = getval("gzlvl3");
        gt3 = getval("gt3");
	gstab = getval("gstab");
        pwHs = getval("pwHs");
        compH = getval("compH");
        cor = getval("cor");
        tau = getval("tau");
	gzlvl0 = getval("gzlvl0");
	gt0 = getval("gt0");
	getstr("sspul",sspul);
        getstr("watergate",watergate);
	iphase = (int)(getval("phase")+0.5);

	settable(t1,4,ph1);
	settable(t2,8,ph2);
	settable(t3,4,ph3);

	getelem(t1,ct,v1);
	getelem(t2,ct,v2);
	getelem(t3,ct,oph);
        

	if (iphase == 2)
	 incr(v1);

	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v10);
		add(v1,v10,v1);
		add(oph,v10,oph);
        add(v1,two,v3);
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));
        tpwrs = (int) (tpwrs);
status(A);
        obspower(tpwr);
 	delay(5.0e-5);
	if (sspul[0] == 'y')
	{
		zgradpulse(gzlvl0,gt0);
		rgpulse(pw,zero,rof1,rof1);
		zgradpulse(gzlvl0,gt0);
	}
	delay(d1);

status(B);
        if (watergate[0] == 'y') 
        {
        obsstepsize(45.0);
        initval(7.0,v7);
        xmtrphase(v7); 
        obspower(tpwrs);
        shaped_pulse("H2Osinc",pwHs,v3,rof1,rof1);
        obspower(tpwr);
        }
	rgpulse(pw, v1, 2.0e-6, 2.0e-6);
        xmtrphase(zero);
	if (d2 > 0.0)
	 delay(d2 - (4*pw/PI) - 4.0e-6);
	else
	 delay(d2); 

        if (watergate[0] == 'y')
	{
	        rgpulse(pw, v2, 2.0e-6, 0.0);
	
       	        delay(tau);	
                zgradpulse(gzlvl3,gt3);
       		delay(gstab);
       		rgpulse(pw*0.231,v1,rof1,rof1);
       		delay(d3);
       		rgpulse(pw*0.692,v1,rof1,rof1);
       		delay(d3);
       		rgpulse(pw*1.462,v1,rof1,rof1);
       		delay(d3);
       		rgpulse(pw*1.462,v3,rof1,rof1);
       		delay(d3);
       		rgpulse(pw*0.692,v3,rof1,rof1);
       		delay(d3);
       		rgpulse(pw*0.231,v3,rof1,rof1);
                delay(tau);
                zgradpulse(gzlvl3,gt3);
		delay(gstab+cor);
               
        }
       else
	        rgpulse(pw, v2, 2.0e-6, rof2);



status(C);
}
