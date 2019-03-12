// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  B1profile - B1 profile along Z */

#include <standard.h>
void pulsesequence()
{
   double gzlvl1;
   extern int dps_flag;
   char   gradaxis[MAXSTR];

   getstrnwarn("gradaxis",gradaxis);
   if (( gradaxis[0] != 'x') && ( gradaxis[0] != 'y') && ( gradaxis[0] != 'z') )
      strcpy(gradaxis,"z");
   gzlvl1 = getval("gzlvl1");
   at = getval("at");

   if ( ((1.0/sw1)*ni) > 500.0e-6) 
    {
     printf("Pulse length too long!");
     psg_abort(1);
    }


   status(A);
   delay(d1);
   if (dps_flag)
     rgpulse(d2, zero,rof1,rof2);
   else
    {
     if ( d2 > 500.0e-6 )
      {
       printf("maximum value of pulse length too long!, reduce ni");
       psg_abort(1);
      }
    rgpulse(d2, zero,rof1,rof2); 
    }
   rgradient(gradaxis[0],-gzlvl1);
   delay(at/2.0);
   rgradient(gradaxis[0],0.0);
   delay(0.001);
   rgradient(gradaxis[0],gzlvl1);
   delay(0.0001); /* let gradient stabilize */
   /* rely on psg safety to turn off gradient */
}
