/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
/*  spuls - single pulse sequence 

            ticks=1 enables external trigger  
            pwpat - pulse shape (e.g. hard, gauss)
            tpwr  - pulse power (dB)
	    to calculate correct tpwr for a shaped pulse, set fliplist to a 
	    particular flipangle and type ssprep
*/

#include <standard.h>
#include "sgl.c"

void pulsesequence()
{
   double pd, seqtime;

   initparms_sis();  /* initialize standard imaging parameters */

   seqtime = at+pw+rof1+rof2;
   pd = tr - seqtime;  /* predelay based on tr */
    if (pd <= 0.0) {
      abort_message("%s: Requested tr too short.  Min tr = %f ms",seqfil,seqtime*1e3);
    }

   status(A);
   delay(pd);
   xgate(ticks);

   /* --- observe period --- */
   obspower(tpwr);
   
   shapedpulse(pwpat,pw,oph,rof1,rof2);
   
   startacq(alfa);
   acquire(np,1.0/sw);
   endacq();
   
}


