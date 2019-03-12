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

/***********************************************************************
Simple slice selection with FID acquire for slice shimming
ARR  3/15/10
************************************************************************/

#include <standard.h>
#include "sgl.c"

void pulsesequence()
{
  /* Internal variable declarations *********************/
  double  freq1;
  double  tr_delay;

  /* Initialize paramaters ******************************/
  init_mri();

  /* Initialize gradient structures *********************/
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2 );     // excitation pulse
  init_slice(&ss_grad,"ss",thk);                 // slice select gradient
  init_slice_refocus(&ssr_grad,"ssr");           // slice refocus gradient

  /* RF Calculations ************************************/
  calc_rf(&p1_rf,"tpwr1","tpwr1f");

  /* Gradient calculations ******************************/
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice_refocus(&ssr_grad,&ss_grad,WRITE,"gssr");

  /* Check that all Gradient calculations are ok ********/
  sgl_error_check(sglerror);

  /* Min TR *********************************************/   	
  trmin  = ss_grad.duration + ssr_grad.duration + at + 8e-6;

  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr);
  }
  if (FP_LT(tr,trmin)) {
    abort_message("TR too short.  Minimum TR = %.2fms\n",trmin*1000+0.005);
  }

  /* Calculate tr delay */
  tr_delay = granularity((tr-trmin)/ns,GRADIENT_RES);

  /* Set up frequency offset pulse shape list ***********/   	
  freq1 = poffset(pss[0],ss_grad.ssamp);

  /* Adjust experiment time for VnmrJ *******************/
  g_setExpTime(tr*nt*arraydim + tr*ssc);

  /* PULSE SEQUENCE *************************************/
  rotate();                          // Initialize default orientation
  obsoffset(resto);
  delay(4e-6);

  delay(tr_delay);  // relaxation delay

  if (ticks > 0) {
    xgate(ticks);
    grad_advance(gpropdelay);
    delay(4e-6);
  }

  sp1on(); delay(4e-6); sp1off();       // Scope trigger

  /* Slice select RF pulse ******************************/ 
  obspower(p1_rf.powerCoarse);
  obspwrf(p1_rf.powerFine);
  delay(4e-6);
  obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
  delay(ss_grad.rfDelayFront);
  shapedpulseoffset(p1_rf.pulseName,ss_grad.rfDuration,oph,rof2,rof2,freq1);
  delay(ss_grad.rfDelayBack);

  /* Slice refocus gradient *****************************/
  obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);

  /* Set frequency offset for acquire********************/   	
  obsoffset(tof);

  /* Acquire FID ****************************************/
  startacq(alfa);
  acquire(np,1.0/sw);
  endacq();
}
