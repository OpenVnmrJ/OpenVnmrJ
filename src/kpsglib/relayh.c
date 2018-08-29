// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/*  relayh - relayed cosy, including regular cosy and delayed cosy

   Parameters:

         tau = delay to allow the relay of magnetization via
	       scalar coupling
       relay = the number of relays to be employed; if relay = 0,
	       a delayed COSY is performed for tau > 0
          nt = 2**(relay+2), e.g., nt = 8 for relay = 1

  f. sauriol july 1986; s. patt june/july 1987  */


#include <standard.h>

pulsesequence()
{
   int          i,
		relay;
   double       tau;

/* GET NEW PARAMETERS */
   relay = (int) (getval("relay") + 0.5);
   tau = getval("tau");


/* CHECK CONDITIONS */
   if (p1 == 0.0)
      p1 = pw;


/* CALCULATE PHASES */
   sub(ct, ssctr, v11);		     /* v11 = ct-ss */
   initval(256.0, v12);
   add(v11, v12, v11);		     /* v11 = ct-ss+256 */
   hlv(v11, v1);		     /* v1 = cyclops = 00112233 */
   for (i = 0; i < relay + 1; i++)
      hlv(v1, v1);		     /* cyclops = 2**(relay+2) 0's, 1's, 2's,
					3's */

   mod4(v11, v2);		     /* v2 = 0123 0123... */
   dbl(v2, oph);		     /* oph = 0202 0202 */
   add(v2, v1, v2);		     /* v2 = 0123 0123 + cyclops */
   add(oph, v1, oph);		     /* oph = 0202 0202 + cyclops */
   hlv(v11, v3);		     /* v3 = 0011 2233 4455 ... */


/* PULSE SEQUENCE */
   status(A);
      if (rof1 == 0.0)
      {
         rof1 = 1.0e-6;		     /* phase switching time */
         rcvroff();
      }
      hsdelay(d1);		     /* preparation period */

   status(B);
      rgpulse(pw, v1, rof1, rof1);
      delay(d2);		     /* evolution period */
   status(A);
      if (relay == 0)
         delay(tau);		     /* for delayed cosy */
      rgpulse(p1, v2, rof1, rof1);   /* start of mixing period */
      if (relay == 0)
         delay(tau);		     /* for delayed cosy */

      for (i = 0; i < relay; i++)    /* relay coherence */
      {
         hlv(v3, v3);		     /* v3=2**(relay+1) 0's, 1's, 2's, 3's */
         dbl(v3, v4);		     /* v4=2**(relay+1) 0's, 2's, 0's, 2's */
         add(v2, v4, v5);	     /* v5=v4+v2 (including cyclops) */
         delay(tau/2);
         rgpulse(2.0*pw, v2, rof1, rof1);
         delay(tau/2);
         rgpulse(pw, v5, rof1, rof1);
      }

   status(C);
      delay(rof2);
      rcvron();
}
