// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* AT_lkdec-  one-pulse sequence using lock/decoupler as transmitter

  Set tn='lk' for 2H observe via the lock rf path
  The lock/decoupler offset should equal transmitter offset
  d3 must contain then number of rf channels (numrfch)

  The lock/decoupler is always the highest number channel and is
  parameterized accordingly. Thus, if the lock/decoupler is defined
  in the system configuration (using "config") its channel number is
  the same as the vnmr variable "numrfch".

  In order to use this pulse sequence, one of two different methods may be used.

  Method #1:  Disable Relay.
  This may be done by disconnnecting the cable connected to port 14
  on the Magnet Relay Driver Board in the "magnet leg" (INOVA).
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

  The power level of the decoupler pulse is controlled by dpwr* (dpwr4 for lk/dec
  on channel 5, dpwr3 for lk/dec on channel 4, and dpwr2 for lk/dec on channel 3)   */

#include <standard.h>

pulsesequence()

{
      delay(d1);
      rcvroff();
      if (d3>4.5)
       dec4rgpulse(pw, oph, rof1, rof2);
      else
       {
        if (d3>3.5)
          dec3rgpulse(pw, oph, rof1, rof2);
        else
          dec2rgpulse(pw, oph, rof1, rof2);
       }        
}
