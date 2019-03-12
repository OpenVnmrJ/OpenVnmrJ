// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */

#include <standard.h>

void pulsesequence()
{

   dpwr = getval("dpwr");
   if (dpwr > 46)               /* Do not fry the probe */
   { abort_message("Decoupling power too large (max is 46) - acquisition aborted.");
   }

   /* equilibrium period */
   status(A);
   decpwrf(4095.0);
   hsdelay(d1);

   /* --- tau delay --- */
   status(B);
   pulse(p1, zero);
   hsdelay(d2);

   /* --- observe period --- */
   status(C);
   pulse(pw,oph);
}
