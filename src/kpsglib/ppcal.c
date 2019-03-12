// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* ppcal (semut) - spectral editing with multiple quantum trap
  (simplified version for use in decoupler pulse calibration)


         pp -  proton 90 degree decoupler pulse
      pplvl -  power level for proton decoupler pulse
  modified for use with Gemini 2000  */


#include <standard.h>

void pulsesequence()
{
   double	pp;
		

   pp = getval("pp");
   
/* calculate phases */
  mod2(ct,v1);  /* 0101 */
  dbl(v1,v1);   /* 0202 */
  hlv(ct,v2);   /* 0011 2233 */
  mod2(v2,v2);  /* 0011 0011 */
  add(v1,v2,v1);  /* 0213 0213*/
  assign(v1,oph); 


  

   status(A);
      hsdelay(d1);
   status(B);
      pulse(pw, v1);
      delay(d2);
         if (declvlonoff)
            declvlon();         /* sets power to pplvl */
         else
            decpower(pplvl);
      simpulse(p1, pp, v1, v1, rof1, rof1);
         if (declvlonoff)
            declvloff();
         else
            decpower(dpwr);
      delay(d2);
   status(C);
}
