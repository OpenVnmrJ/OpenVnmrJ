/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */

#include <standard.h>
#include "sgl.c"

pulsesequence()
{
  double sign,tr_delay,currentlimit,RMScurrentlimit,dutycycle;

  /* Initialize paramaters **********************************/
  init_mri();

  currentlimit=getval("currentlimit");
  RMScurrentlimit=getval("RMScurrentlimit");

  if (gspoil>0.0) sign = 1.0;
  else sign = -1.0;

  init_generic(&spoil_grad,"spoil",fabs(gspoil),tspoil);
  calc_generic(&spoil_grad,WRITE,"","tspoil");
  putvalue("ramp",spoil_grad.tramp);

  /* Calculate tr delay */
  trmin = 4e-6+spoil_grad.duration+alfa+aqtm;
  if (FP_LT(tr,trmin)) {
    abort_message("TR too short.  Minimum TR = %.2fms.\n",trmin*1000+0.005);
  }
  tr_delay = granularity(tr-trmin,GRADIENT_RES);

  /* Calculate duty cycle */
  dutycycle = (gspoil/gmax)*currentlimit/RMScurrentlimit;
  dutycycle *= dutycycle;
  dutycycle *= 100.0*(tspoil/tr);
  putCmd("dutycycle = %f",dutycycle);
  if (FP_GT(dutycycle,100.0)) {
    abort_message("Duty Cycle limit exceeded by %.1f percent.\n",dutycycle-100.0);
  }

  rotate();

  /* TTL scope trigger **********************************/       
  delay(tr_delay);
  sp1on(); delay(4e-6); sp1off();
  obl_shapedgradient(spoil_grad.name,spoil_grad.duration,0,0,spoil_grad.amp*sign,WAIT);

  startacq(alfa);
  acquire(np,1.0/sw);
  endacq();

}

