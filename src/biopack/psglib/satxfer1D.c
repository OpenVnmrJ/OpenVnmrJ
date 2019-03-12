/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 1D STD Saturation Transfer Difference experiment with
	sculpted suppression of solvent  
	Reference Mayer and Meyer J.A.Ch.Soc.2001,123,6108-6117
	Features included:
		Randomization of Magnetization prior to relaxation delay
			G-90-G
			[selected by sspul flag]
		Solvent suppression during relaxation and or detection 
			period selected by satmode flag 	 
	

		
	Paramters:
		sspul :		y - selects magnetization randomization option
		hsglvl:		Homospoil gradient level (DAC units)
		hsgt	:	Homospoil gradient time
		satmode	:	y - selects presaturation during relax
					delay
		satfrq	:	presaturation frequency
		satdly	:	presaturation delay
		satpwr	:	presaturation power
		mix	:	T1rho spinlock mixing time
		slpwr	:	spin-lock power level
		slpw	:	90 deg pulse width for spinlock
		d1	:	relaxation delay
		selfrq	:	frequency (for selective 180) for signal
			        inversion of protein at approx -2 ppm 		
       		selfrq1	:	frequency at 30ppm
		selpwr	:	Power of selective 180 pulse at  -2 ppm 
		selpw	:	Selective 180 deg pulse width -2 ppm
				duration ca 50 ms
		selshape:	shape of selective 180 pulse at -2ppm
		selfrqs	:	solvent frequency
                selpwrs :	power of 180 deg pulse for solvent suppression
		selpws	:	Selective 180 deg pulse width for solvent supp
				duration ca 3.1 ms
		selshapes : 	Shape of 180 deg pulse for solvent supp.
		gzlvl1, gzlvl2 : Gradient levels during the DPFG echos
		gt1, gt1:	Gradient times during the DPFG echos
		gstab	:	recovery delay
                cycles1 :	number of selective pulses for saturation
				of protein 
        the selective inversion pulse  is jumping between -2ppm and 30 ppm

	Igor Goljer  June 9 2003

*/


#include <standard.h>

static int	ph1[8] = {0,2,0,2,1,3,1,3},
		ph5[8] = {2,0,2,0,3,1,3,1},
		ph2[8] = {2,2,0,0,1,1,3,3},
                ph3[8] = {1,3,1,3,2,0,2,0},
		ph4[8] = {0,0,0,0,1,1,1,1};

void pulsesequence()
{
   double	   hsglvl,
		   hsgt,
		   slpwr,
		   slpw,
		   mix,
		   cycles,
                   cycles1,
		   gzlvl1,
		   gt1,
		   gzlvl2,
		   gt2,
		   satfrq,
		   satdly,
		   satpwr,
		   gstab,
		   selpwr, selpwrs, selfrqs,
		   selfrq, selfrq1,
		   selpws, selpw;
   char            sspul[MAXSTR],
		   selshape[MAXSTR], selshapes[MAXSTR],	
                   satmode[MAXSTR];

   hsglvl = getval("hsglvl");
   hsgt = getval("hsgt");
   cycles1 = getval("cycles1");
   slpwr = getval("slpwr");
   mix = getval("mix");
   slpw = getval("slpw");
   gzlvl1 = getval("gzlvl1");
   gt1 = getval("gt1");
   gzlvl2 = getval("gzlvl2");
   gt2 = getval("gt2");
   gstab =getval("gstab");
   satfrq = getval("satfrq");
   satpwr = getval("satpwr");
   satdly = getval("satdly");
   selpwr = getval("selpwr");
   selpwrs = getval("selpwrs");
   selfrq = getval("selfrq");
   selfrq1 = getval("selfrq1");
    selfrqs = getval("selfrqs");
   selpw = getval("selpw");
   selpws = getval("selpws");
   getstr("selshape",selshape);
    getstr("selshapes",selshapes);
   getstr("sspul", sspul);
   getstr("satmode",satmode);

   cycles = (mix)/(4*slpw);
   initval(cycles,v9);
    initval(cycles1,v5);

   settable(t1,8,ph1);
   settable(t5,8,ph5);
   settable(t4,8,ph4);
   settable(t2,8,ph2);
   settable(t3,8,ph3);

   getelem(t4,ct,oph);
   getelem(t2,ct,v2);
   getelem(t3,ct,v3);
/*   add(v2,two,v3);    
   add(v2,one,v3);            */
   mod2(ct,v4);                /*  0 1 0 1 0 1 0 1 ..frequency  switch */

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
   obspower(tpwr);
   delay(5.0e-5);

   if (sspul[0] == 'y')
   {
	zgradpulse(hsglvl,hsgt);
	rgpulse(pw,zero,rof1,rof1);
	zgradpulse(hsglvl,hsgt);
   }

   delay(d1);

   if (satmode[0] == 'y') 
     {
        obspower(satpwr);
	if (satfrq != tof)
	obsoffset(satfrq);
	rgpulse(satdly,zero,rof1,rof1);
	if (satfrq != tof)
	obsoffset(tof);
        obspower(tpwr);
     }
    ifzero(v4);
    obsoffset(selfrq);
    elsenz(v4);
    obsoffset(selfrq1);
    endif(v4);
 /*  Start the selective saturation of protein */ 

    obspower(selpwr);
    if (cycles1 > 0.0)
   {
    starthardloop(v5);
      delay(0.0005);
      shaped_pulse(selshape,selpw,zero,rof1,rof1);
      delay(0.0005);
      endhardloop(); 
    }

     obspower(tpwr);
       obsoffset(tof);
     delay(0.000001);
   status(B);
      rgpulse(pw, t1, rof1, rof2);
 /* spin lock pulse for dephasing of protein signals */
      obspower(slpwr);
       rcvroff();
      rgpulse(mix,v3,rof1,rof2);

/*  solvent suppression using excitation sculpting    */

    if (satmode[1] == 'y') 
      {
        zgradpulse(gzlvl1,gt1);
        delay(gstab);
	if (selfrqs != tof)
	 obsoffset(selfrqs);
        obspower(selpwrs);
	shaped_pulse(selshapes,selpws,t1,rof1,rof1); 
        obspower(tpwr);
        if (selfrqs != tof)
        obsoffset(tof);
         rgpulse(2*pw,t5,rof1,rof1);
        delay(gstab); 
        zgradpulse(gzlvl1,gt1);
        delay(gstab); 
        zgradpulse(gzlvl2,gt2);
        if (selfrqs != tof)
         obsoffset(selfrqs);
        obspower(selpwrs);
        delay(gstab); 
	shaped_pulse(selshapes,selpws,t5,rof1,rof1);   
        obspower(tpwr);
        if (selfrqs != tof)
         obsoffset(tof);
         rgpulse(2*pw,t1,rof1,rof1);
        delay(gstab); 
        zgradpulse(gzlvl2,gt2);
        delay(gstab);  
      }
	rcvron();

   status(C);
}
