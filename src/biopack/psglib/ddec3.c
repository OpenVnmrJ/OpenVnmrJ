/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ddec3-  standard two-pulse sequence using channel 3 as transmitter

  The decoupler offset 'dof2' should equal transmitter offset to
  for proper signal detection
  Parameter homo must be set to 'n'
*/

#include <standard.h>

static int phasecycle[4] = {0, 2, 1, 3};

void pulsesequence()
{
   settable(t1,4,phasecycle);
      dec2power(dpwr2); 
      delay(d1);
      diplexer_override(0);
      dec2rgpulse(pw, t1, rof1, rof2);
    setreceiver(t1);
}
