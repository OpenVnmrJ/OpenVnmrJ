// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  g2pul - gradient equipment two-pulse sequence 

    Parameters: 
        gzlvl1 = gradient amplitude (-32768 to +32768)
        gt1 = gradient duration in seconds (0.002)   

*/

#include <standard.h>
void pulsesequence()
{
   double gzlvl1,gt1;
   char   gradaxis[MAXSTR];

   getstrnwarn("gradaxis",gradaxis);
   if (( gradaxis[0] != 'x') && ( gradaxis[0] != 'y') && ( gradaxis[0] != 'z') )
      strcpy(gradaxis,"z");

   gzlvl1 = getval("gzlvl1");
   gt1 = getval("gt1");

   status(A);
   delay(d1);

   status(B);
   rgpulse(p1, zero,rof1,rof2);
   rgradient(gradaxis[0],gzlvl1);
   delay(gt1);
   rgradient(gradaxis[0],0.0);
   delay(d2);

   status(C);
   pulse(pw,oph);
}

