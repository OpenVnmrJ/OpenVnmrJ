// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* d2pul-  standard two-pulse sequence using decoupler as transmitter

  The decoupler offset 'dof' should equal transmitter offset to
  for proper signal detection
  Parameter homo must be set to 'n'

  The power level of the decoupler pulse is controlled by TPWR, not DHP,
  for systems with a linear amplifier on the decoupler RF channel, i.e.,
  amptype='_a'. 

  For tests of 2H decoupling using channels 3 or 4:
   (AutoTest macros: AT430, AT431, AT440, AT441)

  In order to use this pulse sequence, one of two different methods may be used.

  Method #1:  Disable Relay.
  This may be done by disconnnecting the cable connected to port 14
  on the Magnet Relay Driver Board in the "magnet leg".
  This cable drives the Deuterium Automation Relay (K5020) at the back of the
  "magnet leg" (look for the label on the driver cable). The cable connection
  is made on a small circuit board mounted next to the manual sample eject
  control. This permits the 2H pulse to be delivered by the 2H decoupler, even
  when using tn=lk for 2H observe. Otherwise, the relay switches to route the
  first broadband amplifier to the lowband module for low band observe.
  Remember to reconnect this cable for normal operation. 

  Method #2: Move Decoupler XMTR Cable.
  This is done by moving the cable carrying the decoupler channel rf from the top
  of relay K5022 to the bottom position. This relay is mounted at the back of the
  magnet leg and is open to view without opening the door. It is the leftmost
  relay when looking at the back. Remember to reconnect this cable for normal operation.

  The Decoupler XMTR cable could also be attached directly to the lock diplexer box
  but the pw90 will be somewhat shorter (~5%) but will prevent the 2H observe mode (tn=lk).
*/

#include <standard.h>

static int phasecycle[4] = {0, 2, 1, 3};

pulsesequence()
{
   settable(t1,4,phasecycle);
      obspower(0.0);
      decpower(tpwr); 
      delay(d1);
      decrgpulse(p1, zero, rof1, rof2);
      hsdelay(d2-rof1-rof2);
      decrgpulse(pw, t1, rof1, rof2);
    setreceiver(t1);
}
