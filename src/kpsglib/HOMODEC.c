// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/*  HOMODEC - standard two-pulse sequence for
		homonuclear decoupling
		Forces the value of dmm and homo parameters 

*/

#include <standard.h>

void pulsesequence()
{
   char homo[MAXSTR];

   strcpy(dmm, "ccc");
   strcpy(homo, "y");

   /* equilibrium period */
   status(A);
   hsdelay(d1);

   /* --- tau delay --- */
   status(B);
   pulse(p1, zero);
   hsdelay(d2);

   /* --- observe period --- */
   status(C);
   pulse(pw,oph);
}
