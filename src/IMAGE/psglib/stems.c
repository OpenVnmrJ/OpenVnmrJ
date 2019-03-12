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
Stimulated echo sequence with optional diffusion weighting
************************************************************************/

#include <standard.h>
#include "sgl.c"

/* Phase cycling of last two 90 degree pulses */
static int ph90[2] = {1,3};

void pulsesequence() 
{
  /* Internal variable declarations *****************/
  double freq90[MAXNSLICE];
  int    shape90,table=0,navinvert;
  double te_delay1,te_delay2,tr_delay,tm_delay,navTime=0.0,navsign=1;
  char   slprofile[MAXSTR];

  /* sequence dependent diffusion variables */
  double Gro,Gss;          // "gdiff" for readout/readout refocus and slice/slice refocus
  double dgro,dgss;        // "delta" for readout/readout refocus and slice/slice refocus
  double Dgro,Dgss;        // "DELTA" for readout/readout refocus and slice/slice refocus
  double dcrush,dgss2;     // "delta" for crusher and gss2 gradients
  double Dcrush,Dgss2;     // "DELTA" for crusher and gss2 gradients

  int    i;

  /* Real-time variables used in this sequence ******/
  int  vpe_steps  = v1;
  int  vpe_ctr    = v2;
  int  vms_slices = v3;
  int  vms_ctr    = v4;
  int  vpe_offset = v5;
  int  vpe_mult   = v6;
  int  vph90      = v7;    // Phase of last two 90 pulses
  int  vph2       = v8;    // alternate phase of last two 90s on odd transients
  int  vssc       = v9;    // Compressed steady-states
  int  vtrimage   = v10;   // Counts down from nt, trimage delay when 0
  int  vacquire   = v11;   // Argument for setacqvar, to skip steady state acquires
  int  vtrigblock = v12;   // Number of slices per trigger block

  /*  Initialize paramters **************************/
  init_mri();

  navinvert=getvalnwarn("navinvert");
  getstrnwarn("slprofile",slprofile);

  /*  Check for external PE table *******************/
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    table = 1;
  }

  /* RF Power & Bandwidth Calculations **************/
  shape_rf(&p1_rf,"p1",p1pat,p1,flip1,rof1,rof2);
  calc_rf(&p1_rf,"tpwr1","tpwr1f");

  /* We are using the same RF for all 3 pulses, so it must be symmetric */
  if (p1_rf.header.rfFraction != 0.5)
    abort_message("Use symmetric RF pulse");

  /* Initialize gradient structures *****************/
  init_slice(&ss_grad,"ss",thk);
  init_slice_refocus(&ssr_grad, "ssr");
  init_slice_butterfly(&ss2_grad,"ss2",thk,gcrush,tcrush);
  init_readout(&ro_grad,"ro",lro,np,sw);
  ro_grad.pad1=alfa; ro_grad.pad2=alfa;
  init_readout_refocus(&ror_grad,"ror");
  init_phase(&pe_grad,"pe",lpe,nv);
  init_generic(&spoil_grad,"spoil",gspoil,tspoil); /* TM spoiler */

  /* Gradient calculations **************************/
  calc_readout(&ro_grad, WRITE,"gro","sw","at");
  calc_readout_refocus(&ror_grad,&ro_grad,NOWRITE,"gror");
  calc_phase(&pe_grad, NOWRITE,"gpe","tpe");
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice(&ss2_grad,&p1_rf,WRITE,"gss2");
  calc_slice_refocus(&ssr_grad,&ss_grad,WRITE,"gssr");
  calc_generic(&spoil_grad,WRITE,"","");

  /* Equalize Gradients  ****************************/
  calc_sim_gradient(&ror_grad,&pe_grad,&null_grad,0,WRITE); 

  /* Allow for navigator to be acquired with same polarity as read out gradient */
  if (navigator[0] == 'y') {
    if (navinvert) {                // polarity inverted
      navsign=-1;
      navTime=ro_grad.duration;
    } else {                        // polarity the same
      init_dephase(&navror_grad,"navror");
      calc_dephase(&navror_grad,WRITE,ro_grad.m0,"","");
      navTime=ro_grad.duration+navror_grad.duration;
    }
  }

  /* Create optional prepulse events ****************/
  if (sat[0] == 'y')  create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (mt[0] == 'y')   create_mtc();
  if (ir[0] == 'y')   create_inversion_recovery();
  if (diff[0] == 'y') init_diffusion(&diffusion,&diff_grad,"diff",gdiff,tdelta);

  sgl_error_check(sglerror);

  /* Min TE *****************************************/
  te = granularity(te,2*GRADIENT_RES);
  /* tau1, tau2 are the sum of events in each half echo period */
  /* tau1, tau2 include a GRADIENT_RES as this is minimum delay time */
  tau1 = ss_grad.rfCenterBack+ssr_grad.duration+ss2_grad.rfCenterFront+GRADIENT_RES;
  tau2 = ss2_grad.rfCenterBack+ror_grad.duration+ro_grad.timeToEcho+GRADIENT_RES;
  temin = 2*MAX(tau1,tau2);

  /* Min TM *****************************************/
  tm = granularity(tm,GRADIENT_RES);
  /* tmmin includes a GRADIENT_RES as this is minimum delay time */
  tmmin  = ss2_grad.duration+GRADIENT_RES;  /* one half of ss2_grad for each RF pulse */
  if (spoilflag[0] == 'y') tmmin += spoil_grad.duration;

  /* Diffusion **************************************/
  if (diff[0] == 'y') {
    /* granulate tDELTA */
    tDELTA = granularity(tDELTA,GRADIENT_RES);
    /* taudiff is the duration of events between diffusion gradients */
    taudiff=ss2_grad.duration+tmmin;
    /* set minimum diffusion structure requirements for gradient echo: taudiff, tDELTA, te and minte[0] */
    set_diffusion(&diffusion,taudiff,tDELTA,te,minte[0]);
    /* set additional diffusion structure requirements for spin echo: tau1 and tau2 */
    set_diffusion_se(&diffusion,tau1,tau2);
    /* set additional diffusion structure requirements for stimulated echo: tm and mintm[0] */
    set_diffusion_tm(&diffusion,tm,mintm[0]);
    /* calculate the diffusion structure delays.
       addresses &temin and &tmmin are required in order to update temin and tmmin accordingly */
    calc_diffTime_tm(&diffusion,&temin,&tmmin);
  }

  /* TE delays **************************************/
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (FP_LT(te,temin)) {
    abort_message("TE too short, minimum TE = %.3f ms\n",temin*1000);
  }
  te_delay1 = te/2 - tau1 + GRADIENT_RES;
  te_delay2 = te/2 - tau2 + GRADIENT_RES;

  /* TM delay ***************************************/
  if (mintm[0] == 'y') {
    tm = tmmin;
    putvalue("tm",tm);
  }
  if (FP_LT(tm,tmmin)) {
    abort_message("TM too short, minimum TM = %.3f ms\n",tmmin*1000);
  }
  tm_delay = tm - tmmin + GRADIENT_RES;

  /* Check nsblock, the number of slices blocked together
     (used for triggering and/or inversion recovery) */
  check_nsblock();

  /* Min TR *****************************************/   	
  trmin = ss_grad.rfCenterFront + te + tm + ro_grad.timeFromEcho + pe_grad.duration + 2*GRADIENT_RES;

  /* Increase TR if any options are selected ********/
  if (navigator[0] == 'y') trmin += navTime;
  if (sat[0] == 'y')       trmin += satTime;
  if (fsat[0] == 'y')      trmin += fsatTime;
  if (mt[0] == 'y')        trmin += mtTime;
  if (ticks > 0)           trmin += GRADIENT_RES;

  /* Adjust for all slices **************************/
  trmin *= ns;

  /* Inversion recovery *****************************/
  if (ir[0] == 'y') {
    /* tauti is the additional time beyond IR component to be included in ti */
    /* satTime, fsatTime and mtTime all included as those modules will be after IR */
    tauti = satTime + fsatTime + mtTime + GRADIENT_RES + ss_grad.rfCenterFront;
    /* calc_irTime checks ti and returns the time of all IR components */
    trmin += calc_irTime(tauti,trmin,mintr[0],tr,&trtype);
  }

  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr);
  }
  if (FP_LT(tr,trmin)) {
    abort_message("TR too short, minimum TR = %.3f ms\n",trmin*1000);
  }

  /* TR delay ***************************************/
  tr_delay = granularity((tr-trmin)/ns,GRADIENT_RES);

  /* Calculate B values *****************************/
  if (ix == 1) {
    /* Calculate bvalues according to main diffusion gradients */
    calc_bvalues(&diffusion,"dro","dpe","dsl");
    /* Add components from additional diffusion encoding imaging gradients peculiar to this sequence */
    /* Initialize variables */
    dgro = 0.5*(ror_grad.duration+ro_grad.timeToEcho);
    Gro = ro_grad.m0ref/dgro; Dgro = dgro;
    dgss = 0.5*(ss_grad.rfCenterBack+ssr_grad.duration);
    Gss = ss_grad.m0ref/dgss; Dgss = dgss;
    dgss2 = ss2_grad.rfDuration/2; Dgss2 = dgss2+tm;
    dcrush = tcrush; Dcrush = dcrush+ss2_grad.rfDuration+tm;
    for (i = 0; i < diffusion.nbval; i++)  {
      /* set droval, dpeval and dslval */
      set_dvalues(&diffusion,&droval,&dpeval,&dslval,i);
      /* Readout */
      diffusion.bro[i] += bval(Gro,dgro,Dgro);
      /* Slice */
      diffusion.bsl[i] += bval(Gss,dgss,Dgss);
      diffusion.bsl[i] += bval(gcrush,dcrush,Dcrush);
      diffusion.bsl[i] += bval(ss2_grad.ssamp,dgss2,Dgss2);
      diffusion.bsl[i] += bval_nested(gdiff*dslval,tdelta,tDELTA,gcrush,dcrush,Dcrush);
      diffusion.bsl[i] += bval_nested(gdiff*dslval,tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
      diffusion.bsl[i] += bval_nested(gcrush,dcrush,Dcrush,ss2_grad.ssamp,dgss2,Dgss2);
      /* Readout/Slice Cross-terms */
      diffusion.brs[i] += bval_cross(gdiff*droval,tdelta,tDELTA,gcrush,dcrush,Dcrush);
      diffusion.brs[i] += bval_cross(gdiff*droval,tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
      /* Slice/Phase Cross-terms */
      diffusion.bsp[i] += bval_cross(gdiff*dpeval,tdelta,tDELTA,gcrush,dcrush,Dcrush);
      diffusion.bsp[i] += bval_cross(gdiff*dpeval,tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
    }  /* End for-all-directions */
    /* Write the values */
    write_bvalues(&diffusion,"bval","bvalue","max_bval");
  }

  /* Generate phase-ramped pulses: 90, 180, and IR **/
  offsetlist(pss,ss_grad.ssamp,0,freq90,ns,seqcon[1]);
  shape90 = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freq90,ns,ss_grad.rfFraction,seqcon[1]);

  /* Set pe_steps for profile or full image *********/   	
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&null_grad);
  F_initval(pe_steps/2.0,vpe_offset);

  /* Shift DDR for pro ******************************/
  roff = -poffset(pro,ro_grad.roamp);

  /* Adjust experiment time for VnmrJ ***************/
  if (ssc<0) {
    if (seqcon[2] == 'c') g_setExpTime(trmean*(ntmean*pe_steps*arraydim - ssc*arraydim));
    else g_setExpTime(trmean*(ntmean*pe_steps*arraydim - ssc*pe_steps*arraydim));
  }
  else g_setExpTime(trmean*ntmean*pe_steps*arraydim + tr*ssc);

  /* Set phase cycle table */
  settable(t2,1,ph90);     // only use first element as phase encode is with readout dephase

  /* PULSE SEQUENCE *********************************/
  status(A);
  rotate();
  triggerSelect(trigger);  // Select trigger input 1/2/3
  obsoffset(resto);
  delay(GRADIENT_RES);
  initval(fabs(ssc),vssc); // Compressed steady-state counter
  if (seqcon[2]=='s') assign(zero,vssc); // Zero for standard peloop
  assign(one,vacquire);    // real-time acquire flag
  setacqvar(vacquire);     // Turn on acquire when vacquire is zero 

  /* trigger */
  if (ticks > 0) F_initval((double)nsblock,vtrigblock);

  /* Begin phase-encode loop ************************/       
  peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);

    if (trtype) delay(ns*tr_delay);   // relaxation delay

    /* Compressed steady-states: 1st array & transient, all arrays if ssc is negative */
    if ((ix > 1) && (ssc > 0))
      assign(zero,vssc);
    sub(vpe_ctr,vssc,vpe_ctr);        // vpe_ctr counts up from -ssc
    assign(zero,vssc);
    if (seqcon[2] == 's')
      assign(zero,vacquire);          // Always acquire for non-compressed loop
    else {
      ifzero(vpe_ctr);
        assign(zero,vacquire);        // Start acquiring when vpe_ctr reaches zero
      endif(vpe_ctr);
    }

    /* Read external kspace table if set **************/       
    if (table)
      getelem(t1,vpe_ctr,vpe_mult);
    else {
      ifzero(vacquire);
        sub(vpe_ctr,vpe_offset,vpe_mult);
      elsenz(vacquire);
        sub(zero,vpe_offset,vpe_mult);    // Hold PE mult at initial value for steady states
      endif(vacquire);
    }

    getelem(t2,vpe_ctr,vph90);        // For phase encoding with slice rephase
    add(oph,vph90,vph90);             // last two 90 deg pulses phase alternates +/- 90 from receiver
    mod2(ct,vph2);
    dbl(vph2,vph2);
    add(vph90,vph2,vph90);            // Alternate phase for last two 90s on odd transients

    /* Begin multislice loop **************************/       
    msloop(seqcon[1],ns,vms_slices,vms_ctr);

      if (!trtype) delay(tr_delay);   // Relaxation delay

      if (ticks > 0) {
        modn(vms_ctr,vtrigblock,vtest);
        ifzero(vtest);                // if the beginning of an trigger block
          xgate(ticks);
          grad_advance(gpropdelay);
          delay(GRADIENT_RES);
        elsenz(vtest);
          delay(GRADIENT_RES);
        endif(vtest);
      }

      sp1on(); delay(GRADIENT_RES); sp1off(); // Scope trigger

      /* Prepulse options *******************************/
      if (ir[0] == 'y')   inversion_recovery();
      if (sat[0] == 'y')  satbands();
      if (fsat[0] == 'y') fatsat();
      if (mt[0] == 'y')   mtc();

      /* RF pulse 1 *************************************/ 
      obspower(p1_rf.powerCoarse);
      obspwrf(p1_rf.powerFine);
      delay(GRADIENT_RES);
      obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);   
      delay(ss_grad.rfDelayFront);
      shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
      delay(ss_grad.rfDelayBack);

      /* slice refocus gradient *************************/
      obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);

      if (diff[0] == 'y') {
        delay(diffusion.d1);
        diffusion_dephase(&diffusion,dro,dpe,dsl);
        delay(diffusion.d2);
      } 
      else 
        delay(te_delay1);

      /* RF pulse 2 *************************************/ 
      obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);   
      delay(ss2_grad.rfDelayFront);
      shapedpulselist(shape90,ss_grad.rfDuration,vph90,rof1,rof2,seqcon[1],vms_ctr);
      delay(ss2_grad.rfDelayBack);

      /* TM Period with crusher *************************/
      if (spoilflag[0] == 'y') 
        obl_shapedgradient(spoil_grad.name,spoil_grad.duration,
                           spoil_grad.amp,spoil_grad.amp,spoil_grad.amp,WAIT);

      if (diff[0] == 'y')
        delay(diffusion.dm); 
      else 
        delay(tm_delay);

      /* RF pulse 3 *************************************/ 
      obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);   
      delay(ss2_grad.rfDelayFront);
      shapedpulselist(shape90,ss_grad.rfDuration,vph90,rof1,rof2,seqcon[1],vms_ctr);
      delay(ss2_grad.rfDelayBack);

      if (diff[0] == 'y') {
        delay(diffusion.d3);
        diffusion_rephase(&diffusion,dro,dpe,dsl);
        delay(diffusion.d4);
      } 
      else 
        delay(te_delay2);

      /* Readout gradient and acquisition ***************/
      roff = -poffset(pro,ro_grad.roamp);  // incase inverted navigator is acquired
      if (slprofile[0] == 'y') {
        obl_shapedgradient(ror_grad.name,ror_grad.duration,0,0,-ror_grad.amp,WAIT);
        obl_shapedgradient(ro_grad.name,ro_grad.duration,0,0,ro_grad.amp,NOWAIT);
      } else {
        pe_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,0,-pe_grad.increment,vpe_mult,WAIT);
        obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
      }
      delay(ro_grad.atDelayFront-alfa);
      startacq(alfa);
      acquire(np,1.0/sw);
      endacq();
      delay(ro_grad.atDelayBack);

      /* Rewind Phase encoding ******************************/
      pe_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,pe_grad.increment,vpe_mult,WAIT);

      /* Navigator acquisition ******************************/
      if (navigator[0] == 'y') {
        if (navinvert) roff = -poffset(pro,-ro_grad.roamp);
        else obl_shapedgradient(navror_grad.name,navror_grad.duration,-navror_grad.amp,0,0,WAIT);
        obl_shapedgradient(ro_grad.name,ro_grad.duration,navsign*ro_grad.amp,0,0,NOWAIT);
        delay(ro_grad.atDelayFront-alfa);
        startacq(alfa);
        acquire(np,1.0/sw);
        delay(ro_grad.atDelayBack);
        endacq();
      }

    endmsloop(seqcon[1],vms_ctr);

  endpeloop(seqcon[2],vpe_ctr);

  /* Inter-image delay ******************************/
  sub(ntrt,ct,vtrimage);
  decr(vtrimage);
  ifzero(vtrimage);
    delay(trimage);
  endif(vtrimage);

  /* Duty cycle *****************************************/
  calc_grad_duty(tr);

}
