/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ddec4-  standard two-pulse sequence using channel 5 as transmitter

  The decoupler offset 'dof4' should equal transmitter offset to
  for proper signal detection
  Parameter homo must be set to 'n'
*/

#include <standard.h>

static int phasecycle[4] = {0, 2, 1, 3};

pulsesequence()
{
   settable(t1,4,phasecycle);
      dec4power(dpwr4); 
      diplexer_override(0);
      delay(d1);
      dec4rgpulse(pw, t1, rof1, rof2);
    setreceiver(t1);
}
