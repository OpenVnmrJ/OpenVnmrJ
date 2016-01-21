// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/*  ptune - new sequence to activate Pulse Tune through aux2 */

#include <standard.h>

pulsesequence()
{
   initval(nt,v1);
   sub(v1,ct,v2);
    sub(v2,one,v2);
   /* equilibrium period */
   status(A);
   ifzero(ssctr);
      aux1on();
   endif(ssctr);
   hsdelay(d1);

   /* --- tau delay --- */
   status(B);
   pulse(p1, zero);
   hsdelay(d2);

   /* --- observe period --- */
   status(C);
   ifzero(v2);
      aux12off();
   endif(v2);
   pulse(pw,oph);
}
