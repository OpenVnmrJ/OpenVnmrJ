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
Spin echo sequence (profile only) for power calibration
The flip angle passed to init_rf is always negative, 
in order to allow arraying the coarse/fine power levels independently.
************************************************************************/
#include <standard.h>
#include "sgl.c"

#define  GDELAY 4e-6

/* phase cycling of RF and receiver */
static int phr[4] = {0,2,1,3};              /* receiver phase */

pulsesequence() {
  /* Internal variable declarations *********************/
  double  freq90[MAXNSLICE],freq180[MAXNSLICE];
  int     shape90,shape180;
  double  minTE, te_delay1, te_delay2, minTR, tr_delay;
  double  tref, te1, te2;
  int     tpwr1f, tpwr2f;
    
  /* Real-time variables ****************************/
  int  vms_slices = v1;
  int  vms_ctr    = v2;

  /*  Initialize parameters *************************/
  init_mri();
  tpwr1f = (int) getval("tpwr1f");
  tpwr2f = (int) getval("tpwr2f");


  if ((nv > 0) && (profile[0] == 'n'))
    abort_message("Sorry, this sequence only acquires a profile, check the profile flag");

  /* Read RF shape but don't calculate powers *******/
  init_rf(&p1_rf,p1pat,p1,-1,rof1,rof2);
  calc_rf(&p1_rf,"","");
  init_rf(&p2_rf,p2pat,p2,-1,rof1,rof1);
  calc_rf(&p2_rf,"","");

  /* Gradient Calculations **************************/
  init_slice(&ss_grad,"gss",thk); 
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  init_slice_refocus(&ssr_grad, "ssr"); 
  calc_slice_refocus(&ssr_grad, &ss_grad, NOWRITE,"gssr");
  init_slice_butterfly(&ss2_grad,"gss2",thk,gcrush,tcrush); 
  calc_slice(&ss2_grad,&p2_rf,WRITE,"gss");

  init_readout(&ro_grad,"ro",lro,np,sw);
  calc_readout(&ro_grad, WRITE, "gro","sw","at"); 
  init_readout_refocus(&ror_grad,"ror");
  calc_readout_refocus(&ror_grad, &ro_grad, NOWRITE, "gror"); 

  /* Equalize Refocus Gradients  ********************/
  tref = calc_sim_gradient(&ror_grad, &ssr_grad, &null_grad, 0, WRITE); 

  /*  Min TE ******************************************/
  te1 = ss_grad.rfCenterBack + tref + 4e-6 + ss2_grad.rfCenterFront;
  te2 = ss2_grad.rfCenterBack + alfa + ro_grad.timeToEcho;
  minTE = 2*(te1 > te2 ? te1 : te2) + 2*4e-6;
  
  if (minte[0] == 'y') {
    te = minTE;
    putvalue("te",ceil(te*1e6)*1e-6); /* round up to nearest us */
  }
  if (te < minTE) {
    abort_message("TE too short.  Minimum TE= %.2fms\n",minTE*1000);   
  }
  te_delay1 = te/2 - te1;
  te_delay2 = te/2 - te2;

  /*  Min TR ******************************************/   	
  minTR =  (GDELAY + ss_grad.rfCenterFront + te + ro_grad.timeFromEcho) * ns;

  if (mintr[0] == 'y') {
    tr = minTR + 4e-6;
    putvalue("tr",tr);
  }
  if (tr < minTR + 4e-6) {
     abort_message("TR too short.  Minimum TR= %.2fms\n",(minTR + 4e-6)*1000);   
  }
  tr_delay = (tr - minTR)/ns;

  if (sglerror)
    abort_message("Sequence has error(s) and will not execute - See error message(s)!\n");


  offsetlist(pss,ss_grad.ssamp,0,freq90,ns,seqcon[1]);
  offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,seqcon[1]);
  shape90  = shapelist(p1pat,ss_grad.rfDuration,freq90,ns,0,seqcon[1]);
  shape180 = shapelist(p2pat,ss2_grad.rfDuration,freq180,ns,0,seqcon[1]);


  /* PULSE SEQUENCE *************************************/
  settable(t1,4,phr);
  getelem(t1,ct,oph);    /* receiver phase */

  rotate();
  obsoffset(resto);
  delay(GDELAY);

  /* Begin multislice loop ******************************/       
  msloop(seqcon[1],ns,vms_slices,vms_ctr);
    if (ticks) {
      xgate(ticks);
      grad_advance(gpropdelay);
      delay(4e-6);
    }

    /* RF pulse *******************************************/ 
    obspower(tpwr1);
    obspwrf(tpwr1f);
    delay(GDELAY);
    obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);   
    delay(ss_grad.rfDelayFront);
    shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
    delay(ss_grad.rfDelayBack);

    /* Refocusing gradients ******************/
    obl_shaped3gradient(ror_grad.name,"",ssr_grad.name,
                        ssr_grad.duration,
		        ror_grad.amp,0,-ssr_grad.amp,WAIT);   

    delay(te_delay1);

    /* RF pulse *******************************************/ 
    obspower(tpwr2);
    obspwrf(tpwr2f);
    delay(GDELAY);
    obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);   
    delay(ss2_grad.rfDelayFront);
    shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof1,seqcon[1],vms_ctr);
    delay(ss2_grad.rfDelayBack);

    delay(te_delay2);

    /* Readout gradient and acquisition ********************/
    obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
    delay(ro_grad.atDelayFront);
    startacq(alfa);
    acquire(np,1.0/sw);
    endacq();
    delay(ro_grad.atDelayBack);
    
    delay(tr_delay);

  endmsloop(seqcon[1],vms_ctr);
}
