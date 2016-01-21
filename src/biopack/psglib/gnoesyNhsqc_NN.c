/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gnoesyNhsqc_NN.c for Heterodimer studies
    has been temporarily removed for pulse sequence revision
*/

#ifndef LINT
#endif

/*  s2pul - standard two-pulse sequence */

#include <standard.h>

pulsesequence()
{
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
