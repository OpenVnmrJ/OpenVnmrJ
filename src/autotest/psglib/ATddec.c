// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* ATddec-  standard two-pulse sequence using channel 2,3 or4 as transmitter

  The decoupler offset should equal transmitter offset to
  for proper signal detection
  Parameter homo must be set to 'n'

  diplexer_overfide(0) lets 2H observe via tn=lk, but keeps 2H input from
  decoupler channel rather than channel 2 (default operation)

*/

#include <standard.h>

static int phasecycle[4] = {0, 2, 1, 3};

void pulsesequence()
{
    settable(t1,4,phasecycle);
    delay(d1);
    if (tpwr>0)
     {
      obspower(tpwr);       /* default mode uses channel 2 for pulse       */
      rgpulse(pw, t1, rof1, rof2);
     }
    if (dpwr2>0)
     {
      dec2power(dpwr2); 
      diplexer_override(0);  /* channel 3 amp must be connected to diplexer */
      dec2rgpulse(pw, t1, rof1, rof2);
     }
    if (dpwr3>0)
     {
      dec3power(dpwr3); 
      diplexer_override(0);  /* channel 4 amp must be connected to diplexer */
      dec3rgpulse(pw, t1, rof1, rof2);
     }
    if (dpwr4>0)
     {
      dec4power(dpwr4);
      diplexer_override(0);  /* channel 5 amp must be connected to diplexer */
      dec4rgpulse(pw, t1, rof1, rof2);
     }
    setreceiver(t1);
}
