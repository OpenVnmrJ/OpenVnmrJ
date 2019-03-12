// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* gHETCOR  - Gradient selected heteronuclear chemical shift correlation - C observe

	Features included:
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
	   ph8[4] = {1,1,3,3},
	   ph7[2] = {0,2};

void pulsesequence()
{
   double	j1xh,
         	dly3,
		gzlvlE,
		gtE,
		EDratio,
		pp,
		pplvl,
         	dly4;
   int		icosel,
		phase1 = (int)(getval("phase")+0.5);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",0.5);
        gzlvlE = syncGradLvl("gtE","gzlvlE",0.5);

/* Get new variables from parameter table */
   j1xh = getval("j1xh");
   pp = getval("pp");
   gzlvlE = getval("gzlvlE");
   gtE = getval("gtE");
   EDratio = getval("EDratio");
   pplvl = getval("pplvl");
   icosel = -1;

      dly3 = 1.0 / (2.0 * j1xh);
      dly4 = 3.0/(4.0*j1xh);
/* PHASE CYCLING CALCULATION */

   settable(t2,4,ph2);
   settable(t8,4,ph8);
   settable(t7,2,ph7);
   assign(zero, v1);                  /* v1 = 0 */

   getelem(t2,ct,v2);
   getelem(t7,ct,v7);
   add(v2,two,v3);
   sub(v7,one,oph);
   add(v7,one,v6);

   if ((phase1 == 2) || (phase1 == 5))
	icosel = 1;
/*
   mod2(id2,v12);
   dbl(v12,v12);
*/
    initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v12);

   add(v1,v12,v1);
   add(oph,v12,oph);


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

         decpulse(pp, v1);

         if (d2/2.0 > 0.0) 
          delay(d2/2.0 - (4*pp/PI)); 
         else 
          delay(d2/2.0);

         decpulse(pp, v2);
         delay(dly3);
         simpulse(2*pw, 2.0*pp, t8, v2, 2.0e-6, 2.0e-6);
         delay(dly3);
         decpulse(pp, v3);

         if (d2/2.0 > 0.0)  
          delay(d2/2.0 - (2*pp/PI));  
         else  
          delay(d2/2.0);

         delay(dly3/2.0);
         simpulse(2*pw, 2.0*pp, one, one, rof1, rof1);
	 zgradpulse(2*gzlvlE*EDratio,gtE/2.0);
         delay(dly3/2.0 - gtE/2.0 - 2*GRADIENT_DELAY - (2*pp/PI) - rof1);

         simpulse(pw, pp, v7, one, rof1, rof1);
         delay(dly4/2.0 - (2*pp/PI) - 2*rof1);
         simpulse(2.0*pw, 2*pp, v6, one, rof1, rof2);
         decpower(dpwr);
	 zgradpulse(icosel*gzlvlE,gtE);
         delay(dly4/2.0 - POWER_DELAY - gtE - 2*GRADIENT_DELAY);
/* Observe period */
   status(C);
}
