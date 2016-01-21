// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*
  Parameters:

        pw = 90 degree excitation pulse on the observe transmitter
        pwx = 90 degree excitation pulse on first decoupler
      jC13 = J(C13-H1) coupling constant

 */
#include <standard.h>

pulsesequence()
{
   double jc13,jtau;

 
   jc13 = getval("jc13");
   jtau = 1.0 / (2.0 * jc13);

   mod4(ct, v1);		/*  v1 = 0 1 2 3 */
   dbl(v1, v10);		/* v10 = 0 2 0 2 */
   hlv(ct, v2);
   hlv(v2, v2);
   mod4(v2, v2);		/* v2 = 0 0 0 0 1 1 1 1 2 2 2 2 3 3 3 3 */
   add(v2, v1, v1);
   assign(v2, oph);
   add(v10, oph, oph);

   status(A);
   hsdelay(d1);
   if (getflag("wet")) wet4(zero,one);

   status(C);
   rgpulse(pw, v2, rof1, rof2);
   jtau -= pw + rof2;
   delay(jtau - rof1);
   simpulse(2*pw, pwx, v1, zero, rof1, rof2);
   delay(jtau);

   status(C);
}

