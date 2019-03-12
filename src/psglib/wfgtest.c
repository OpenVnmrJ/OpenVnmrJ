// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* wfgtest - single shaped pulse on RF channel or simultanious 
 *           shaped pulses on tweo RF channels (observe, and decoupler)
 *           Version 2.0 3-4-93
 */

#include <standard.h>

void pulsesequence()
{
   char obspat[MAXSTR];
   int  option;

   getstr("obspat", obspat);
   getstr("decpat", decpat);
   option = (int) (getval("option") + 0.5);

/* equilibrium period */
   status(A);
      hsdelay(d1);

/* --- tau delay --- */
   status(B);
      hsdelay(d2);

/* --- observe period --- */

   status(C);
      if (option == 1)
      {
         rgpulse(1e-6, oph, rof1, 0.0);
         shaped_pulse(obspat, pw, oph, rof1, rof2);
      }
      else if (option == 2)
      {
         decrgpulse(1e-6, oph, rof1, 0.0);
         decshaped_pulse(decpat, p1, oph, rof1, rof2);
      }
      else if (option == 3)
      {
         dec2rgpulse(1e-6, oph, rof1, 0.0);
         dec2shaped_pulse(decpat, p1, oph, rof1, rof2);
      }
      else if (option == 4)
      {
         dec3rgpulse(1e-6, oph, rof1, 0.0);
         dec3shaped_pulse(decpat, p1, oph, rof1, rof2);
      }
      else if (option == 5)
      {
         rgpulse(1e-6, oph, rof1, 0.0);
         simshaped_pulse(obspat, decpat, pw, p1, oph, oph, rof1, rof2);
      }
      else
      {
         text_error("Invalid value for `option`\n");
         psg_abort(1);
      }
}
