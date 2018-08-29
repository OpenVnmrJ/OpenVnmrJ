/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ddec4-  standard two-pulse sequence using channel 4 as transmitter

  The decoupler offset 'dof3' should equal transmitter offset to
  for proper signal detection
  Parameter homo must be set to 'n'
*/

#include <standard.h>

static int phasecycle[4] = {0, 2, 1, 3};

pulsesequence()
{
   settable(t1,4,phasecycle);
      dec3power(dpwr3); 
      diplexer_override(0);
      delay(d1);
      dec3rgpulse(pw, t1, rof1, rof2);
    setreceiver(t1);
}
