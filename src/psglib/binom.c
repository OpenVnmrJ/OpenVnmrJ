// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* binom - binomial water suppression

 Parameters:

  p1,p2,p3:     if p1 = 0, p1 is derived from pw, which is assumed 
		to be a 90 degree pulse;
                if p2=0, p2 is derived from p1;
                if p3=0, p3 is derived from p1 (used for 1510 only);
		the pulse sequences are symmetric, so
		that p1 is the first and last pulse, etc;
		pulses are arranged for rf compensation with null
		on resonance, especially for 121 and 14641
  offset:       if >0, gives on-resonance suppression;
                if <0, gives off-resonance suppression
                with calculated d2;
                if =0, gives off-resonance suppression
                with entered d2
  d2:           directly entered only if offset=0;
                otherwise calculated
  seq:          11, 1331, or 1510 (gives 1-5-10-10-5-1);
		or 121 or 146 (gives 1-4-6-4-1)
  rof2:         rcvr gating time after last pulse in sequence
  rof1:         rcvr gating time before and after all other
                pulses; if rof1=0, receiver is left off during
                entire sequence.

 references: p.j. hore, j. magn. reson. 55:283-300 (1983) 
	     starcuk and sklenar, j. magn. reson. 61:567-570 (1985) */


#include "standard.h"

void pulsesequence()
{
   double          offset,
                   seq,
		   rof3,
		   rof4 = 0.0,
		   rof5 = 0.0;
   int             iseq;


/* LOAD VARIABLES */
   offset = getval("offset");
   seq = getval("seq");
   iseq = seq + 0.5;

   if (iseq > 121)
      p2 = getval("p2");
   if (iseq == 1510)
      p3 = getval("p3");


/* CALCULATE PHASECYCLE */
   if (offset > 0.0)
   {
      add(oph, two, v1);
   }
   else
   {
      add(oph, zero, v1);
   }


/* INITIALIZE VARIABLES */
   if (offset != 0.0)
      d2 = 0.5/offset;
   if (d2 < 0.0)
      d2 = -d2;
   if (d2 < 2*rof1)
   {
      fprintf(stdout, "ROF1 is too large for the desired OFFSET.\n");
      psg_abort(1);
   }

   if (rof1 == 0.0)
      rof1 = 10.0e-6;		/* allow for phase switching */


   if (iseq == 11)
   {
      if (p1 == 0.0)
         p1 = pw/2;
      rof3 = 2*rof1+p1;
   }
   else if (iseq == 121)	/* set up 1-2-1 as 1-(1-1)-1 */
   {				/* see starcuk and sklenar */
      if (p1 == 0.0)
         p1 = pw/4;
      rof3 = 3*rof1+3*p1/2;
   }
   else if (iseq == 1331)
   {				/* set up 1-3-3-1 */
      if (p1 == 0.0)
         p1 = pw/8;
      if (p2 == 0.0)
	 p2 = 3.0*p1;
      rof3 = 2*rof1+(p1+p2)/2;
      rof4 = 2*rof1+p2;
   }
   else if (iseq == 146) /* set up 1-4-6-4-1 as 1-(1-3)-(3-3)-(3-1)-1 */
   {			 /* see starcuk and sklenar */
      if (p1 == 0.0)
         p1 = pw/16;
      if (p2 == 0.0)
         p2 = 3.0*p1;
      rof3 = 3*rof1+p1+p2/2;
      rof4 = 4*rof1+3*p2/2;
   }
   else if (iseq == 1510)
   {				/* set up 1-5-10-10-5-1 */
      if (p1 == 0.0)
         p1 = pw/32;
      if (p2 == 0.0)
	 p2 = 5.0 * p1;
      if (p3 == 0.0)
	 p3 = 10.0 * p1;
      rof3 = 2*rof1+(p1+p2)/2;
      rof4 = 2*rof1+(p2+p3)/2;
      rof5 = 2*rof1+p3;
   }
   else				/* default, set up 1-1 */
   {
      if (p1 == 0.0)
         p1 = pw/2;
      rof3 = 2*rof1+p1;
   }


/* BEGIN ACTUAL SEQUENCE */
   status(A);
      hsdelay(d1);
      rcvroff();

   status(B);
      if (iseq == 11) 
      {
         rgpulse(p1, v1, rof1, rof1);
         delay(d2-rof3);
      }
      else if (iseq == 121)
      {
         rgpulse(p1, oph, rof1, rof1);
         delay(d2-rof3);
         rgpulse(p1, v1,  rof1, rof1);
         rgpulse(p1, v1,  rof1, rof1);
         delay(d2-rof3);
      }
      else if (iseq == 1331)
      {
         rgpulse(p1, v1,  rof1, rof1);
         delay(d2-rof3);
         rgpulse(p2, oph, rof1, rof1);
         delay(d2-rof4);
         rgpulse(p2, v1,  rof1, rof1);
         delay(d2-rof3);
      }
      else if (iseq == 146)
      {
         rgpulse(p1, oph, rof1, rof1);
         delay(d2-rof3);
         rgpulse(p1, v1,  rof1, rof1);
         rgpulse(p2, v1,  rof1, rof1);
         delay(d2-rof4);
         rgpulse(p2, oph, rof1, rof1);
         rgpulse(p2, oph, rof1, rof1);
         delay(d2-rof4);
         rgpulse(p2, v1,  rof1, rof1);
         rgpulse(p1, v1,  rof1, rof1);
         delay(d2-rof3);
      }
      else if (iseq == 1510)
      {
         rgpulse(p1, v1, rof1, rof1);
         delay(d2-rof3);
         rgpulse(p2, oph, rof1, rof1);
         delay(d2-rof4);
         rgpulse(p3, v1,  rof1, rof1);
         delay(d2-rof5);
         rgpulse(p3, oph, rof1, rof1);
         delay(d2-rof4);
         rgpulse(p2, v1,  rof1, rof1);
         delay(d2-rof3);
      }
      else /* default iseq=11 */
      {
         rgpulse(p1, v1, rof1, rof1);
         delay(d2-rof3);
      }
      rgpulse(p1, oph, rof1, rof2);

   status(C);
      rcvron();
}
