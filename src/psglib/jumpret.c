// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* jumpandreturn - binomial water suppression */

#include "standard.h"

pulsesequence()
{
   add(oph, two, v1);

   status(A);
      hsdelay(d1);
      rcvroff();
      rgpulse(pw, oph, rof1,rof1);
      delay(d2 - pw - 2.0*rof1);
      rgpulse(pw-p1, v1, rof1, rof2);
   status(B);
}
