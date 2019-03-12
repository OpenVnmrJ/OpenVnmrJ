// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  cancel - standard two-pulse sequence */

#include <standard.h>

static int phasecycle[4] = {0, 2, 1, 3};

void pulsesequence()
{
   status(A);
   hsdelay(d1);
   pulse(p1, zero);
   hsdelay(d2);
   settable(t1,4,phasecycle);
   pulse(pw,t1);
   setreceiver(t1);
}
