// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/*  mrsim - standard two-pulse sequence  for simultaneous multi receive */

#include <standard.h>


static int ph1[4] = {0, 2, 1, 3};
static int ph2[4] = {0, 2, 1, 3};

pulsesequence()
{

  double pwx;

  pwx = getval("pwx");

  /* check decoupling modes */
  
  if ( (dm[C] == 'y') || (homo[0] == 'y') )
  {
    printf("dm[C] should be set to 'n' and/or homo should set to 'n'");
    psg_abort(1);
  }

  settable(t1,4,ph1);
  getelem(t1,ct,v1);
  assign(v1,oph);

  settable(t2,4,ph2);

  status(A);
  obspower(tpwr);
  decpower(dpwr);
  delay(d1);

  status(B);
  delay(d2);

  simpulse(pw, pwx, t2, t2, rof1, rof2);

  status(C);
}
