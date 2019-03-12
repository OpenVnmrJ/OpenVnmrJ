// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* d2pul-  standard two-pulse sequence using decoupler as transmitter

  The decoupler offset 'dof' should equal transmitter offset to
  for proper signal detection
  Parameter homo must be set to 'n'

  The power level of the decoupler pulse is controlled by TPWR, not DHP,
  for systems with a linear amplifier on the decoupler RF channel, i.e.,
  amptype='_a'. */


#include <standard.h>

static int phasecycle[4] = {0, 2, 1, 3};

void pulsesequence()
{
/* equilibrium period */
   status(A);
      hsdelay(d1);

/* tau delay */
   status(B);
      if (newdecamp)
      {
         decpower(tpwr);
         decrgpulse(p1, zero, rof1, rof2);
         decpower(dpwr);
      }
      else
      {
         declvlon();
         decrgpulse(p1, zero, rof1, rof2);
         declvloff();
      }
      hsdelay(d2);

/* observe period */
   status(C);
   settable(t1,4,phasecycle);
      if (newdecamp)
      {
         decpower(tpwr);
         decrgpulse(pw, t1, rof1, rof2);
         decpower(dpwr);
      }
      else
      {
         declvlon();
         decrgpulse(pw, t1, rof1, rof2);
         declvloff();
      }
   setreceiver(t1);
}
