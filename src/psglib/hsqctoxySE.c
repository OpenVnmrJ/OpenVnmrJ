// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*hsqctoxySE.c - 	sensitivity enhanced HSQC-TOCSY-3D	
	[S/N improvement by a factor of 2 for systems with one proton
	 attached to the heteroatom and square root of 2 for those
	 with two or more protons attached to the heteroatom]

			dipsi-2rc or "clean" mlev-17 spinlock option
			Gradient to kill unwanted signals during H-X INEPT
	[NOTE:
		phase = 1,2,3,4
		phase2 = 1,2,3,4	
			1&2 and 3&4 are hypercomplex pairs
			1&3 and 2&4 are "SE" pairs  ]

	Processing - There are many ways to process the 2D planes or the 3D
			see manual entry on this for more discussion
*/



#include <standard.h>

static int	ph1[1] = {0},
		ph2[1] = {0},
		ph3[2] = {0,2},
		ph4[2] = {1,3},
		ph5[1] = {0},
		ph6[1] = {1},
		ph7[2] = {0,2},
		ph8[1] = {1},
		ph9[1] = {3},
		ph10[1] = {0},
		ph11[1] = {0};

void mleva()
{
   double wdwfctr,window,slpw;
   wdwfctr=getval("wdwfctr");
   slpw = getval("slpw");
   window=(wdwfctr*slpw);
   rgpulse(slpw,v6,0.0,window);
   rgpulse(2*slpw,v7,0.0,window);
   rgpulse(slpw,v6,0.0,0.0);
}

void mlevb()
{
   double wdwfctr,window,slpw;
   wdwfctr=getval("wdwfctr");
   slpw = getval("slpw");
   window=(wdwfctr*slpw);
   rgpulse(slpw,v4,0.0,window); 
   rgpulse(2*slpw,v5,0.0,window); 
   rgpulse(slpw,v4,0.0,0.0);
}

void dipsi(phse1,phse2)
codeint phse1,phse2;
{
        double slpw5;
        slpw5 = getval("slpw")/18.0;

        rgpulse(64*slpw5,phse1,0.0,0.0);
        rgpulse(82*slpw5,phse2,0.0,0.0);
        rgpulse(58*slpw5,phse1,0.0,0.0);
        rgpulse(57*slpw5,phse2,0.0,0.0);
        rgpulse(6*slpw5,phse1,0.0,0.0);
        rgpulse(49*slpw5,phse2,0.0,0.0);
        rgpulse(75*slpw5,phse1,0.0,0.0);
        rgpulse(53*slpw5,phse2,0.0,0.0);
        rgpulse(74*slpw5,phse1,0.0,0.0);
}


void pulsesequence()
{
	double		satpwr,
			satdly,
			satfrq,
			hsgpwr,
			gzlvl,
			gt,
			gstab,
			sellvl,
			pwxsel,
			cycle1,
			shape_pw,
			h2off,
			cycles,
                        wdwfctr,
                        slpwr,
                        slpw,
			slpw5,
                        window,
                        mix,
			pwxlvl,
			pwx,
			d3corr,
			d2corr,
			jxh,
			tauxh;
	int		iphase,
			iphase2;
	char		sspul[MAXSTR],
			slpflg[MAXSTR],
			satshape[MAXSTR],
			pwxshape[MAXSTR],
			dipsiflg[MAXSTR],
			satflg[MAXSTR];

/* LOAD VARIABLES */
	satdly = getval("satdly");
	satfrq = getval("satfrq");
	satpwr = getval("satpwr");
	hsgpwr = getval("hsgpwr");
	sellvl = getval("sellvl");
	pwxsel = getval("pwxsel");
	getstr("pwxshape",pwxshape);
	gzlvl = getval("gzlvl");
	gstab = getval("gstab");
	gt = getval("gt");
	wdwfctr = getval("wdwfctr");
        slpw = getval("slpw");
        slpwr = getval("slpwr");
	slpw5 = slpw/18.0;
        mix = getval("mix");
        window = wdwfctr*slpw;
	h2off = getval("h2off");
	getstr("sspul", sspul);
	getstr("satflg",satflg); 
	getstr("satshape",satshape); 
	getstr("dipsiflg",dipsiflg);
	getstr("slpflg",slpflg); 
	iphase = (int) (getval("phase") + 0.5); 
	iphase2 = (int)(getval("phase2") + 0.5);
	pwx = getval("pwx");
	pwxlvl = getval("pwxlvl");
	jxh = getval("jxh");
	tauxh = 1/(4*(jxh));
	d2corr = ((2*pwx/PI) + pw);
	d3corr = ((4*pw/PI) + 2.0e-6);

	if ((slpflg[0] == 'y') && (h2off != 0.0))
	{
	shape_pw = 10/h2off;
	if (shape_pw < 0.0)
	shape_pw = -shape_pw;
	cycle1 = (satdly/(shape_pw + WFG_START_DELAY + WFG_STOP_DELAY)); 
	cycle1 = 2.0*(double)(int)(cycle1/2.0); 
	initval(cycle1,v8);
	}

	if (dipsiflg[0] == 'y')
                cycles = mix/(2072*slpw5);
        else
                cycles = (mix)/(64.66*slpw+32*window);

        cycles = 2.0*(double)(int)(cycles/2.0);
        initval(cycles,v9);


	settable(t1,1,ph1);
	settable(t2,1,ph2);
	settable(t3,2,ph3);
	settable(t4,2,ph4);
	settable(t5,1,ph5);
	settable(t6,1,ph6);
	settable(t7,2,ph7);
	settable(t8,1,ph8);
	settable(t9,1,ph9);
	settable(t10,1,ph10);
	settable(t11,1,ph11);


	getelem(t2,ct,v2);
	getelem(t3,ct,v3);
	getelem(t7,ct,oph);

	getelem(t8,ct,v11);

        getelem(t10,ct,v12);
        getelem(t5,ct,v10);
	getelem(t6,ct,v6);

	if (iphase2 == 2)
	{	incr(v11);
		incr(v10);
	}

	if (iphase == 2)
		incr(v2);

/* IySin+IxCos or IySin-IxCos MODULATION SELECTION IN t1 */
	if (iphase == 3)
		add(v3,two,v3);
	if (iphase == 4)
	{	add(v3,two,v3);
		incr(v2);
	}

/* IySin+IxCos or IySin-IxCos MODULATION SELECTION IN t2 */
	if (iphase2 == 3)
		{add(v12,two,v12);
		 add(v3,two,v3); }
	if (iphase2 == 4)
                {add(v12,two,v12);
                 add(v3,two,v3);
		 incr(v11);
		 incr(v10); }


	add(v6,one,v7);
	add(v7,one,v4);
	add(v4,one,v5);


/*FAD IN t1 and t2 */
	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
	initval(2.0*(double)(((int)(d3*getval("sw2")+0.5)%2)),v13);

		add(v11,v13,v11);
		add(v10,v13,v10);
		add(v2,v14,v2);
		add(oph,v13,oph);
		add(oph,v14,oph);



/* BEGIN THE ACTUAL PULSE SEQUENCE */
	status(A);
		obspower(tpwr);
		decpower(dpwr);
		delay(0.05);
		if (sspul[A] == 'y')
		{
			zgradpulse(hsgpwr,0.01);
			rgpulse(pw, zero, rof1,rof1);
			zgradpulse(hsgpwr,0.01);
		}
		delay(d1);

		if (satflg[0] == 'y')
		{
			obspower(satpwr);
			if (satfrq != tof)
			obsoffset(satfrq); 
			rgpulse(satdly,zero,rof1,rof1);
			if (satfrq != tof)
			obsoffset(tof);
			obspower(tpwr);
			delay(40.0e-6);
		}

	status(B);
		rcvroff();

		rgpulse(pw, t1, rof1, 1.0e-6);
		decpower(pwxlvl);

		txphase(t6);
		decphase(t6);
		delay(tauxh - 1.0e-6 - pwx - POWER_DELAY); 
		simpulse(2*pw,2*pwx,t6,t6,0.0,0.0); 

		txphase(t9);
		decphase(v2);
		delay(tauxh - (2*pw/PI) - pwx);
		rgpulse(pw,t9,0.0,rof1);
		zgradpulse(gzlvl,gt);
		delay(gstab);
		decrgpulse(pwx,v2,rof1,0.0);

		txphase(t6);
		if (d2/2 > 0.0)
		delay(d2/2 - d2corr); 
		else
		delay(d2/2);
		rgpulse(2*pw,t6,0.0,0.0);
		if (d2/2 > 0.0)
		delay(d2/2 - d2corr);
		else 
                delay(d2/2);

		txphase(v11);
		decphase(v3);
		simpulse(pw,pwx,v11,v3,0.0,0.0);

		txphase(v11);
		decphase(t6);
		delay(tauxh - (2*pwx/PI) - pwx);
		simpulse(2*pw,2*pwx,v11,t6,0.0,0.0);

		txphase(v10);
		decphase(t4);
		delay(tauxh - (2*pwx/PI) - pwx);
		simpulse(pw,pwx,v10,t4,0.0,0.0);

		txphase(v10);
		decphase(t6);
		delay(tauxh - (2*pwx/PI) - pwx);
		simpulse(2*pw,2*pwx,v10,t6,0.0,0.0);

		decpower(dpwr);
		txphase(v11);
		delay(tauxh - (2*pw/PI) - pwx - rof1 - POWER_DELAY);
		rgpulse(pw,v11,rof1,1.0e-6);
	status(C);

		if (d3 > 0.0)
                delay(d3 - d3corr);
                else
                delay(d3);
 
	status(D);
		rgpulse(pw,v12,1.0e-6,0.0);
                if (cycles > 1.0)
                {
                obspower(slpwr);
                if  (dipsiflg[0] == 'y')
                {
                        starthardloop(v9);
                        dipsi(v6,v4);
			dipsi(v4,v6);
                        dipsi(v4,v6);
			dipsi(v6,v4);
                        endhardloop();
                }
                else
                {
                        starthardloop(v9);
                          mleva(); mleva(); mlevb(); mlevb();
                          mlevb(); mleva(); mleva(); mlevb();
                          mlevb(); mlevb(); mleva(); mleva();
                          mleva(); mlevb(); mlevb(); mleva();
                          rgpulse(0.66*slpw,v7,0.0,0.0);
                        endhardloop();
                }
                obspower(tpwr);
                }
		rgpulse(pw,t11,0.0,rof2);
		rcvron();

	status(E); 
}
