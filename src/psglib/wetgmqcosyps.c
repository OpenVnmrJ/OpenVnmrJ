// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* wetgmqcosyps - made from gmqcosyps (from gmqcosy V1.1 10/28/92)
	phase-sensitive pulsed gradient enhanced mq cosy
	(currently (930326) this is set up with the gmqcosy macro)
   
   Parameters:
	glvl - set to 2 for a DQFCOSY
	gzlvl1 - try 15000 (must be < 32767/qlvl)
	gt1 - try 0.0025 seconds for organics;
		at least 0.008-0.012 sec for proteins
	phase - 1,2 (run and process as a standard hypercomplex dataset)
	pwwet
	wetpwr
	wetshape

   Processing:
	if phase=1,2: use wft2da
		(after running the gmqcosy macro to setup, set rp1=90
		and rp=rp+90 as compared to the phase 1D spectrum
		(the gmqcosy macro may do part of this))
	 
*/


#include <standard.h>

void pulsesequence()
{
        double gzlvl1,qlvl,grise,gstab,gt1,ss,phase;
	int icosel,iphase;
  	ss = getval("ss");
        grise=getval("grise");
        gstab=getval("gstab"); 
        gt1=getval("gt1");
        gzlvl1=getval("gzlvl1");
        qlvl=getval("qlvl");
        phase=getval("phase");
        iphase = (int) (phase + 0.5);



/* DETERMINE STEADY-STATE MODE */
   if (ss < 0) ss = (-1) * ss;
   else
      if ((ss > 0) && (ix == 1)) ss = ss;
      else
         ss = 0;
   initval(ss, ssctr);
   initval(ss, ssval);

   assign(oph,v2);
   assign(oph,v1);
   if (iphase == 2)
      incr(v1);

/* HYPERCOMPLEX MODE */
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v6);
   if ((iphase==1) || (iphase==2))
      {add(v1,v6,v1); add(oph,v6,oph);} 
	

     status(A);
        rcvroff();
	delay(d1);
     if (getflag("wet")) wet4(zero,one);
	rlpower(tpwr,TODEV);

	rgpulse(pw,v1,rof1,rof2);
     status(B);
        if (d2 > rof1 + 4.0*pw/3.1416)
           delay(d2 - rof1 - 4.0*pw/3.1416);
     status(C);
        rgpulse(pw,v2,rof1,rof2);
      
        delay(gt1+2.0*grise+24.4e-6);
        rgpulse(2.0*pw,v2,rof1,rof2);
        icosel=-1;
        rgradient('z',gzlvl1*(double)icosel);
	delay(gt1+grise);
	rgradient('z',0.0);
        txphase(oph);
	delay(grise);

	rgpulse(pw,v2,rof1,rof2);

        rgradient('z',gzlvl1*qlvl);

	delay(gt1+grise);
	rgradient('z',0.0);
	delay(grise);
       rcvron();
	delay(gstab);  
     status(D);
}

