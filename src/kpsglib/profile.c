// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/*  profile - gradient calibrtion sequence */

#include <standard.h>
pulsesequence()
{
double	gzlvl1;
double	td;
double	at;

/* GATHER AND INITALIZE VARIABLES */
   gzlvl1 = getval("gzlvl1");
   at     = getval("at");
   dbl(oph,v1); mod4(v1,v1); hlv(v1,v1);
   hlv(ct,v2); hlv(v2,v2); mod4(v2,v2); dbl(v2,v2); add(v1,v2,v1);

/* BEGIN PULSE SEQUENCE */
   status(A);
      delay(d1);

   status(B);
      td = (d2-at/2.0)/2.0;
      if (td < 0.0)
        td = 0.0;
      rgpulse(p1,oph,rof1,rof2);
      delay(td);
      rgradient('z',gzlvl1);
      delay(at/2.0);
      rgradient('z',0.0);
      delay(td);

   status(C);
      pulse(pw,v1);
      delay(d2-at/2.0);
      rgradient('z',gzlvl1);
      delay(0.0001); /* let gradient stabilize */
   /* rely on psg safety to turn off gradient */
}

