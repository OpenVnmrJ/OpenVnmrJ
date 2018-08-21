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
/****************************************************************************

  ct3d.c - constant time imaging - 3D version
  
  SGL version

****************************************************************************/

#include <standard.h>
#include "sgl.c"

pulsesequence()
{ 
  double  aqtm     = getval("aqtm");      /* "extra" sampling time for filters */
  double  acqdelay = getval("acqdelay");  /* minimum delay between echoes */
  double  gamma,sgro,sgpe1,sgpe2,gro,gpe1,gpe2,
          dw,te_delay,tr_delay;
  double  glimf,tset;
	  
  int     vnp      = v1;
  int     vnp_ctr  = v2;
  int     vpe1     = v3;
  int     vpe1_ctr = v4;
  int     vpe2     = v5;
  int     vpe2_ctr = v6;


  /* Single point acquisition */
  setacqmode(WACQ|NZ);

  init_mri();
  tset = getval("tset");
  gamma = nuc_gamma();
  
  /* temin determined by FOV and matrix size (minimum is rof2) */
  /* glim % limits gmax */

  glimf = 0.9;
  if (glimf <= 0.0) {
    abort_message("Invalid glim = %.2f\n",glim);   
  }
  temin = MAX(MAX(MAX(np/4/(gamma*gmax*glimf*lro),nv/2/(gamma*gmax*glimf*lpe)),
                      nv2/2/(gamma*gmax*glimf*lpe2)),rof2+(pw/2.0));

  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (te < temin) {
    abort_message("TE too short.  Minimum TE= %.2fus\n",temin*1e6+0.005);   
  }

  dw = 1.0/sw;

  /* Gradient stepsize and max amplitude */
  sgro  = 1/(gamma*te*lro);  gro  = sgro*np/4;
  sgpe1 = 1/(gamma*te*lpe);  gpe1 = sgpe1*nv/2;
  sgpe2 = 1/(gamma*te*lpe2); gpe2 = sgpe2*nv2/2;
  te_delay = te - (p1/2.0) - rof2 - alfa; 
  if(te_delay < 0) {
    abort_message("TE too short.  Minimum TE= %.2fus\n",temin*1e6+0.005);   
  }
  /* Check for TR */
  trmin = trise+tset+rof1+(p1/2.0)+te+trise+dw;
  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr+1e-6);
  }
  if (tr < trmin) {
    abort_message("TR too short.  Minimum TR= %.2fms\n",trmin*1000+0.005);   
  }
  tr_delay = tr - trmin;

  if ((pro != 0) || (ppe != 0) || (ppe2 != 0))
    abort_message("Sorry, can't do FOV offsets, check pro, ppe, ppe2\n");

  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);   /* hard pulse */
  calc_rf(&p1_rf,"tpwr1","tpwr1f");

  sgl_error_check(sglerror);

  g_setExpTime(tr*nt*((np/2)*nv*nv2) );

  /* PULSE SEQUENCE *************************************/
  initval(np/2.0, vnp);
  rotate();
  obsoffset(resto);
  obspower(p1_rf.powerCoarse);
  obspwrf(p1_rf.powerFine);
  delay(4e-6);

  if (profile[0] == 'y') {
    nv    = 1;
    sgpe1 = gpe1 = 0.0;
  }
  if (profile[1] == 'y') {
    nv2   = 1;
    sgpe2 = gpe2 = 0;
  }
  
  peloop2(seqcon[3],nv2,vpe2,vpe2_ctr);
    peloop(seqcon[2],nv,vpe1,vpe1_ctr);
      startacq(alfa);
      loop(vnp,vnp_ctr);
	
        pe3_gradient(-sgro*np/4.0,-sgpe1*nv/2.0,-sgpe2*nv2/2.0,
                     sgro,sgpe1,sgpe2,
                     vnp_ctr,vpe1_ctr,vpe2_ctr);
        delay(trise+tset);  /* settling time for gradients */

        rgpulse(p1,oph,rof1,rof2);
        delay(te_delay);
        rcvron();
	sample(dw);
        rcvroff();
	zero_all_gradients();
	
	delay(trise);
	delay(tr_delay);
      endloop(vnp_ctr);
//      if (aqtm > at) sample(aqtm-at); /* "extra" sampling time for filters */
      endacq();
      delay(acqdelay);                /* minimum delay between echoes */
    endpeloop(seqcon[2],vpe1_ctr);
  endpeloop(seqcon[3],vpe2_ctr);
}

/******************************************************************************************
		Modification History
		

20060609 - Modified for SGL; Note single point acquisition statements
           p1/tpwr1/flip1 rf pulse
20061220 - Bug; poor snr; fixed by adding startacq/endacq around sample()
20060516 - power levels
		
*****************************************************************************************/
