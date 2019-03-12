// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* gmqcosy
	pulsed gradient enhanced mq cosy
	 
	pak 920506 -
*/


#include <standard.h>

void pulsesequence()
{
        double gzlvl1,qlvl,grise,gstab,gt1,taud2,tau1;
	int icosel;

        grise=getval("grise");
        gstab=getval("gstab");
        gt1=getval("gt1");
        gzlvl1=getval("gzlvl1");
        qlvl=getval("qlvl");
	tau1 = getval("tau1");
	taud2 = getval("taud2");

  if (phase1 == 2) 
    { 
     icosel=-1;          
     if (ix==1) 
     fprintf(stdout,"P-type MQCOSY\n");
    }
  else
    { 
     icosel=1;         /* Default to N-type experiment */ 
     if (ix==1) 
     fprintf(stdout,"N-type MQCOSY\n");
    }

  qlvl++ ; 

     status(A);
	obspower(satpwr);
	delay(d1);
	rgpulse(satdly,zero,rof1,rof2);
	obspower(tpwr);
      	delay(1.0e-6);

	rgpulse(pw,oph,rof1,rof2);
        delay(d2);

        rgradient('z',gzlvl1);
	delay(gt1+grise);
	rgradient('z',0.0);
        txphase(oph);
	delay(grise);

	rgpulse(pw,oph,0.0,rof2);
	delay(taud2);

        rgradient('z',gzlvl1);
	delay(gt1+grise);
	rgradient('z',0.0);
        txphase(oph);
	delay(grise);
	delay(taud2);

     status(B);
	 rgpulse(pw,oph,rof1,rof2);
	delay(tau1);

        rgradient('z',gzlvl1*qlvl*(double)icosel);
	delay(gt1+grise);
	rgradient('z',0.0);
	delay(grise);
	delay(gstab);
     status(C);
}
