// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* gmqcosy
	pulsed gradient enhanced mq cosy
*/

#include <standard.h>

pulsesequence()
{
double	grise,gstab,gt1,gzlvl1,phase,qlvl,ss,taud2,tau1;
int	icosel;

/* GATHER AND INITIALIZE VARIABLES */
   grise  = getval("grise");
   gstab  = getval("gstab");
   gt1    = getval("gt1");
   gzlvl1 = getval("gzlvl1");
   phase  = getval("phase");
   qlvl   = getval("qlvl");
   ss     = getval("ss");
   tau1   = getval("tau1");
   taud2  = getval("taud2");

   if (phase == 2.0) 
   {  icosel=-1;          
      if (ix==1) printf("P-type MQCOSY\n");
   }
   else
   {  icosel=1;         /* Default to N-type experiment */ 
      if (ix==1) printf("N-type MQCOSY\n");
   }

   qlvl++ ; 

   initval(ss, ssctr);
   initval(ss, ssval);

/* BEGIN PULSE SEQUENCE */
   status(A);
      delay(d1);
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
