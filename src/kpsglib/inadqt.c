// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*  fastindqt.c - Fast INADEQUATE experiment.   

  Ref: Bourdonneau & Ancian, JMR 132, 316-327 (1998)
  
  
 Parameters:
  jcc   - carbon-carbon coupling (in Hz)
  tau   - conversion time (calculated from jcc unless
	   jcc=0)
  pw    - observe 90 degree pulse
  nt    - must be a multiple of 32. 
*/


#include <standard.h>
static int phs1[32] =  {0,0,2,2,0,0,2,2,1,1,3,3,1,1,3,3,
			2,2,0,0,2,2,0,0,3,3,1,1,3,3,1,1},
	   phs2[32] =  {2,0,0,2,2,0,0,2,3,1,1,3,3,1,1,3,
	   		0,2,2,0,0,2,2,0,1,3,3,1,1,3,3,1},
	   phs3[16] =  {0,1,2,3,1,0,3,2,2,3,0,1,3,2,1,0},
	   phs4[8]  =  {0,3,2,1,3,0,1,2};
	   
pulsesequence()
{
	double jcc,tau,phase;

	/* Retrieve the parameter values. */
	phase = (int)(getval("phase")+0.5);
	jcc = getval("jcc"); 
        tau = 1.0/(4.0*jcc);
 

	settable(t1,32,phs1);
	settable(t2,32,phs2);
	settable(t3,16,phs3);
	settable(t4,8,phs4);
	
	getelem(t3,ct,v3);
	getelem(t4,ct,oph);
	
/*  Uncomment the following three lines to add FAD  */
/*
	initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
        add(v3,v14,v3);
        add(oph,v14,oph);
 */
 
	obsstepsize(45.0);
	
   status(A);
	hsdelay(d1);
 
	if (phase==2) 
	 xmtrphase(one);   
	else 
	 xmtrphase(zero);

   status(B); 
	rcvroff();

	rgpulse(pw,t1,1.0e-6,1.0e-6);
	delay(tau);
	rgpulse(2.0*pw,t2,1.0e-6,1.0e-6);
	delay(tau);
	rgpulse(pw,t1,1.0e-6,1.0e-6);
	if (d2 > 0)
	 delay(d2 - 2.0e-6 - (4*pw/PI) - SAPS_DELAY);
	else
	 delay(d2);
	xmtrphase(zero);
	rgpulse(pw,v3,1.0e-6,rof2);
	rcvron();
   status(C);
 
}


