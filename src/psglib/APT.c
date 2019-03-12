// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* APT - attached proton test experiment 

	Parameters:
	   j1xh : 	CH coupling constant
	   d3   : 	second echo delay time
	   		Can be set to zero if pw > 90 but < 180
	   		degree pulse
	   pw	:	First pulse (not necessarily a 90)
	   p1	:	90 deg pulse width - set to pw90
	   
KrishK	-	Last revision	:	June 1997
*/


#include <standard.h>
#include <chempack.h>

void pulsesequence()
{
double	j1xh,
	d3,
	tau;

 j1xh = getval("j1xh");
 d3 = getval("d3");
 tau = 1/j1xh;

/* CALCULATE PHASE CYCLE */
   hlv(oph, v1);
   dbl(v1, v1);
   incr(v1);				/* v1=1133 */
   mod2(oph, v2);
   dbl(v2, v2);
   incr(v2);				/* v2=1313 */
   add(v1, oph, v1);
   sub(v1, one, v3);
   add(v2, oph, v2);
   sub(v2, one, v4);

/* ACTUAL PULSE SEQUENCE */
   status(A);		
      hsdelay(d1);
   status(B);	
      rgpulse(pw, oph, rof1, rof1);
      delay(tau - rof1 - 2*pw/PI);
      rgpulse(p1, v3, rof1, 1.0e-6);	
      rgpulse(2*p1, v1, 1.0e-6, 1.0e-6);
      rgpulse(p1, v3, 1.0e-6, rof1);
   status(C);	
      delay(tau);
      if (d3 > 0.0) 
      {
        delay(d3);
        rgpulse(p1, v4, rof1, 1.0e-6);
        rgpulse(2*p1, v2, 1.0e-6, 1.0e-6);
        rgpulse(p1, v4, 1.0e-6, rof1);
        delay(d3);
      }
	delay(rof2);
}
