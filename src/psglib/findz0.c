// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*  s2pul - standard two-pulse sequence */

#include <standard.h>
#include <chempack.h>

pulsesequence()
{
   /* equilibrium period */
   status(A);

/*   lcsample(); */
   hsdelay(d1);

   /* --- tau delay --- */
   status(B);
   pulse(p1, zero);
   hsdelay(d2);

   /* --- observe period --- */
   status(C);
   pulse(pw,oph);
}
