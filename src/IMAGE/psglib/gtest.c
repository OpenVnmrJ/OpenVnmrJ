/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************************
GTEST: simple gradient test sequence

gspoil: gradient amplitude
tspoil: gradient duration
d2:     inter-gradient delay
sign1:  scaling multiplier for first gradient, between 1 and -1
sign2:  scaling multiplier for second gradient, between 1 and -1
tr:     total repetition time
************************************************************************/
#include <standard.h>
#include "sgl.c"

pulsesequence()
{
  double  seqtime,tr_delay,sign1,sign2;

  sign1 = getval("sign1");                  // first gradient multiplier, -1 to 1
  sign2 = getval("sign2");                  // second gradient multiplier, -1 to 1

  /* Initialize paramaters **********************************/
  init_mri();

  /* Gradient calculations **********************************/
  init_generic(&spoil_grad,"gtest",gspoil,tspoil);
  calc_generic(&spoil_grad,WRITE,"","");

  seqtime = 2*(tspoil + d2) + at;
  tr_delay = tr - seqtime;
  if (tr_delay < 0.0)
    abort_message("gradtest: TR too short, minimum TR = %6.1f msec",1000*seqtime);

  /* PULSE SEQUENCE *************************************/
  rotate();                                 // Initialize default orientation
  sp1on(); delay(4e-6); sp1off();           // TTL scope trigger

  obl_shapedgradient(spoil_grad.name,spoil_grad.duration,sign1*gspoil,0,0,WAIT);
  delay(d2);
  obl_shapedgradient(spoil_grad.name,spoil_grad.duration,sign2*gspoil,0,0,WAIT);
  delay(d2);
  delay(tr_delay);
}
