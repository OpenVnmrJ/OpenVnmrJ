// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* HETCOR  - heteronuclear chemical shift correlation - C observe

	Features included:
		State-TPPI in F1
		Phase sensitive experiment
		Multiplicity selection
		
	Parameters:
	   	j1xh 	: 	CH coupling constant
	   	pplvl	:	proton pulse level
	   	pp	:	proton pulse width
		d1	:	relaxation delay
		d2	:	Evolution delay

KrishK	-	Last revision	: June 1997

*/


#include <standard.h>
#include <chempack.h>

static int ph2[4] = {0,1,2,3},
	   ph8[4] = {0,0,2,2},
	   ph7[2] = {0,2},
	   ph4[1] = {0};

void pulsesequence()
{
   double      	pplvl = getval("pplvl"),
		pp = getval("pp"),
		dly3,
         	dly4;
   int		phase1 = (int)(getval("phase")+0.5);

      dly3 = 1.0 / (2.0 * (getval("j1xh")));
      dly4 = 3.0/(4.0 * (getval("j1xh")));
/* PHASE CYCLING CALCULATION */

   settable(t2,4,ph2);
   settable(t8,4,ph8);
   settable(t7,2,ph7);
   settable(t4,1,ph4);


   assign(zero, v1);                  /* v1 = 0 */
   if (phase1 == 2)
      incr(v1);                      /* hypercomplex phase increment */

   getelem(t2,ct,v2);
   getelem(t8,ct,v8);
   getelem(t7,ct,v7);
   getelem(t4,ct,v4);

   add(v8,one,v9);
   add(v2,two,v3);
   add(v4,one,v4); 
   add(v4,v7,oph);
   add(v7,one,v6);

/*
   mod2(id2,v12);
   dbl(v12,v12);
*/
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v12);

   add(v1,v12,v1);
   add(oph,v12,oph);
   add(oph,two,oph);


/* ACTUAL PULSE-SEQUENCE BEGINS  */


   status(A);

   obspower(tpwr);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        satpulse(satdly,zero,rof1,rof1);
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);
 
      decpower(pplvl);

   status(B);
      delay(2.0e-5);
      decpulse(pp, v1);

         if (d2/2.0 > 0.0) 
          delay(d2/2.0 - (4*pp/PI)); 
         else 
          delay(d2/2.0);
         decpulse(pp, v2);
         delay(dly3 - rof1 - pw - 1.5 * pp);
         rgpulse(pw, v8, rof1, 2.0e-6);
         simpulse(2.67*pw, 2.0*pp, v9, v2, 2.0e-6, 2.0e-6);
         rgpulse(pw, v8, 2.0e-6, rof1);
         delay(dly3 - rof1 - pw - 1.5*pp);
         decpulse(pp, v3);
         if (d2/2.0 > 0.0)  
          delay(d2/2.0 - (2*pp/PI));  
         else  
          delay(d2/2.0);

         delay(dly3/2.0 - rof1 - 2.0*pp);
         simpulse(pw, pp, zero, zero, rof1, 0.0);
         simpulse(2.67*pw, 2.0*pp, one, one, 0.0, 0.0);
         simpulse(pw, pp, zero, zero, 0.0, rof1);
         delay(dly3/2.0 - 2.0*rof1 - 2*pp - (2*pp/PI));

         simpulse(pw, pp, v7, v4, rof1, rof1);
         delay(dly4/2.0 - 2.33*pp -(2*pp/PI) - 2*rof1);
          simpulse(pw, pp, v7, zero, rof1, 0.0);
          simpulse(2.0*pw, 2.67*pp, v6, one, 0.0, 0.0);
          simpulse(pw, pp, v7, zero, 0.0, rof2);
      decpower(dpwr);
      delay(dly4/2.0 - POWER_DELAY - 2.33*pp);
 
/* Observe period */
   status(C);
}
