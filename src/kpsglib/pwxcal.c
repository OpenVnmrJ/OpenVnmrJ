// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
#include <standard.h>

pulsesequence()
{
   double pwx1;
   double tau;
   char   jname[MAXSTR];
   char   jval[MAXSTR];

 
   pwx1 = getval("pwx1");
   getstr("jname",jname);
   strcpy(jval,"j");
   strcat(jval,jname);
   tau = 1.0 / (2.0 * getval(jval) );

   mod4(ct, v2);                /*  v2 = 0 1 2 3 */
   assign(v2, oph);
   add(v2,one,v1);

   status(A);
   hsdelay(d1);

   status(B);
   rgpulse(pw, v2, rof1, rof2);
   delay(tau - rof1 - rof2);
   simpulse(2*pw,pwx1,v1, zero, rof1, rof2);
   delay(tau);

   status(C);
}

