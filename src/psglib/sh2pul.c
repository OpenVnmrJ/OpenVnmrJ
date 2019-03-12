// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/*  sh2pul - Shaped Two-Pulse Sequence */ 
/*  shaped pulses using waveform generators or linear modulators */

#include <standard.h>

void pulsesequence()
{
    getstr("pwpat",pwpat);
    getstr("p1pat",p1pat);
    if ((p1pat[0] == '\0') && (p1 != 0))
    {
        text_error("The parameter p1pat is missing or has a null value!");
        psg_abort(1);
    }
    if ((pwpat[0] == '\0') && (pw != 0))
    {
        text_error("The parameter pwpat is missing or has a null value!");
        psg_abort(1);
    }
    status(A);
      hsdelay(d1);
    status(B);
      if (is_y(rfwg[0])) shaped_pulse(p1pat,p1,zero,rof1,rof2);
      else apshaped_pulse(p1pat,p1,zero,t1,t2,rof1,rof2);
      hsdelay(d2);
    status(C);
      if (is_y(rfwg[0])) shaped_pulse(pwpat,pw,oph,rof1,rof2);
      else apshaped_pulse(pwpat,pw,oph,t1,t2,rof1,rof2);
}
