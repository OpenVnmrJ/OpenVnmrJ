// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  
     dsh2pul -
     shaped two-pulse sequence using wfg or apshaped_decpulse
     using channel2 for pulses
  The decoupler offset 'dof' should equal transmitter offset to
  for proper signal detection
  Parameter homo must be set to 'n'

  The power level of the decoupler pulse is controlled by TPWR, not DHP,
  for systems with a linear amplifier on the decoupler RF channel, i.e.,
  amptype='_a'. */
#include <standard.h>

void pulsesequence()
{
    char pwpat[MAXSTR],p1pat[MAXSTR];
    /* equilibrium period */
    getstr("pwpat",pwpat);
    if (pwpat[0] == '\0') 
    {
      abort_message("no pwpat? ABORT");
    }
    getstr("p1pat",p1pat);
    if (p1pat[0] == '\0')
    {
      abort_message("no p1pat? ABORT");
    }
      
    status(A);
    obspower(zero);
    decpower(tpwr);
    hsdelay(d1);
    rcvroff();

    status(B);
      if (is_y(rfwg[1])) decshaped_pulse(p1pat,p1,zero,rof1,rof2);
      else apshaped_decpulse(p1pat,p1,zero,t1,t2,rof1,rof2);
      hsdelay(d2);
    status(C);
      if (is_y(rfwg[1])) decshaped_pulse(pwpat,pw,oph,rof1,rof2);
      else apshaped_decpulse(pwpat,pw,oph,t1,t2,rof1,rof2);
}
