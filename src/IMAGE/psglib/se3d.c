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
Spin echo imaging sequence
************************************************************************/

#include <standard.h>
#include "sgl.c"

/* Phase cycling of 180 degree pulse */
static int ph180[2] = {1,3};

pulsesequence()
{
  /* Internal variable declarations *********************/
  double freq90[MAXNSLICE],freq180[MAXNSLICE];
  double te_delay1,te_delay2,tr_delay,tau1,tau2,thk2fact,te_delay3=0.0,te_delay4=0.0,navTime=0.0;
  double crushm0,pem0,gcrushr,gcrushp,gcrushs;
  int    shape90,shape180,table=0,sepRefocus;
  double crushsign=1,navsign=1;
  char   spoilflag[MAXSTR];

  /* sequence dependent diffusion variables */
  double Gro,Gss;          // "gdiff" for readout/readout refocus and slice/slice refocus
  double dgro,dgss;        // "delta" for readout/readout refocus and slice/slice refocus
  double Dgro,Dgss;        // "DELTA" for readout/readout refocus and slice/slice refocus
  double dcrush,dgss2;     // "delta" for crusher and gss2 gradients
  double Dcrush,Dgss2;     // "DELTA" for crusher and gss2 gradients

  int    i;

  /* Real-time variables ********************************/
  int  vpe_steps    = v1;      // Number of PE steps
  int  vpe_ctr      = v2;      // PE loop counter
  int  vms_slices   = v3;      // Number of slices
  int  vms_ctr      = v4;      // Slice loop counter
  int  vpe_offset   = v5;      // PE/2 for non-table offset
  int  vpe_mult     = v6;      // PE multiplier, ranges from -PE/2 to PE/2
  int  vpe2_steps   = v7;      // Number of PE2 steps
  int  vpe2_ctr     = v8;      // PE2 loop counter
  int  vpe2_mult    = v9;      // PE2 multiplier
  int  vpe2_offset  = v10;     // PE2/2 for non-table offset
  int  vssc         = v11;     // Compressed steady-states
  int  vph180       = v12;     // Phase of 180 pulse
  int  vph2         = v13;     // alternate phase of 180 on odd transients
  int  vacquire     = v14;     // Argument for setacqvar, to skip steady state acquires
  int  vtrimage     = v15;     // Counts down from nt, trimage delay when 0
  int  vtrigblock   = v16;     // Number of slabs per trigger block

  /*  Initialize paramaters *****************************/
  init_mri();

  thk2fact=getval("thk2fact");
  getstr("spoilflag",spoilflag);
  sepRefocus=getvalnwarn("sepRefocus");

  /*  Check for external PE table ***********************/
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    table = 1;
  }

  /* RF Power & Bandwidth Calculations ******************/
  shape_rf(&p1_rf,"p1",p1pat,p1,flip1,rof1,rof2);
  shape_rf(&p2_rf,"p2",p2pat,p2,flip2,rof1,rof2);
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");

  /* Initialize gradient structures *********************/
  init_slice(&ss_grad,"ss",thk);
  init_slice(&ss2_grad,"ss2",thk*thk2fact);
  init_dephase(&crush_grad,"crush");
  init_slice_refocus(&ssr_grad,"ssr");
  if (FP_LT(tcrushro,alfa)) tcrushro=alfa;
  init_readout_butterfly(&ro_grad,"ro",lro,np,sw,gcrushro,tcrushro);
  init_readout_refocus(&ror_grad,"ror");
  init_phase(&pe_grad,"pe",lpe,nv);
  init_phase(&pe2_grad,"pe2",lpe2,nv2);
  init_generic(&spoil_grad,"spoil",gspoil,tspoil);

  /* Gradient calculations ******************************/
  calc_readout(&ro_grad, WRITE,"gro","sw","at");
  ro_grad.m0ref *= grof;
  calc_readout_refocus(&ror_grad,&ro_grad,NOWRITE,"gror");
  calc_phase(&pe_grad,NOWRITE,"gpe","tpe");
  calc_phase(&pe2_grad,NOWRITE,"gpe2","");
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice(&ss2_grad,&p2_rf,WRITE,"gss2");
  calc_slice_refocus(&ssr_grad,&ss_grad,WRITE,"gssr");
  calc_generic(&spoil_grad,WRITE,"","");

  /* Make sure crushing in PE dimensions does not refocus signal from 180 */
  crushm0=fabs(gcrush*tcrush);
  pem0 = (pe_grad.m0 > pe2_grad.m0) ? pe_grad.m0 : pe2_grad.m0;
  calc_dephase(&crush_grad,WRITE,crushm0+pem0,"","");
  gcrushr = crush_grad.amp*crushm0/crush_grad.m0;
  gcrushp = crush_grad.amp*(crushm0+pe_grad.m0)/crush_grad.m0;
  gcrushs = crush_grad.amp*(crushm0+pe2_grad.m0)/crush_grad.m0;

  /* Allow phase encode and read dephase to be separated from slice refocus */
  if (sepRefocus) {
    /* Equalize read dephase and PE gradient durations */
    calc_sim_gradient(&ror_grad,&pe_grad,&pe2_grad,0,WRITE);
    crushsign=-1;
  } else {
    /* Equalize both refocus and PE gradient durations */
    pe2_grad.areaOffset = ss_grad.m0ref;     // Add slab refocus on pe2 axis
    calc_phase(&pe2_grad,NOWRITE,"gpe2",""); // Recalculate pe2 to include slab refocus
    calc_sim_gradient(&ror_grad,&pe_grad,&pe2_grad,0,WRITE);
  }

  /* Create optional prepulse events ********************/
  if (sat[0] == 'y')  create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (mt[0] == 'y')   create_mtc();
  if (ir[0] == 'y')   create_inversion_recovery();
  if (diff[0] == 'y') init_diffusion(&diffusion,&diff_grad,"diff",gdiff,tdelta);
  
  sgl_error_check(sglerror);          // Check for any SGL errors

  /* Min TE *********************************************/
  te = granularity(te,2*GRADIENT_RES);
  /* tau1, tau2 are the sum of events in each half echo period */
  /* tau1, tau2 include a GRADIENT_RES as this is minimum delay time */
  tau1 = ss_grad.rfCenterBack + ssr_grad.duration + crush_grad.duration + ss2_grad.rfCenterFront + 2*GRADIENT_RES;
  tau2 = ss2_grad.rfCenterBack + crush_grad.duration + ro_grad.timeToEcho + GRADIENT_RES;
  if (sepRefocus) tau2 += ror_grad.duration;
  temin = 2*MAX(tau1,tau2);

  /* Diffusion ******************************************/
  if (diff[0] == 'y') {
    /* granulate tDELTA */
    tDELTA = granularity(tDELTA,GRADIENT_RES);
    /* taudiff is the duration of events between diffusion gradients */
    taudiff = ss2_grad.duration + 2*crush_grad.duration + GRADIENT_RES;
    /* set minimum diffusion structure requirements for gradient echo: taudiff, tDELTA, te and minte[0] */
    set_diffusion(&diffusion,taudiff,tDELTA,te,minte[0]);
    /* set additional diffusion structure requirements for spin echo: tau1 and tau2 */
    set_diffusion_se(&diffusion,tau1,tau2);
    /* calculate the diffusion structure delays.
       address &temin is required in order to update temin accordingly */
    calc_diffTime(&diffusion,&temin);
  }

  /* TE delays ******************************************/
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (FP_LT(te,temin)) {
    abort_message("TE too short, minimum TE = %.3f ms\n",temin*1000);
  }
  te_delay1 = te/2 - tau1 + GRADIENT_RES;
  te_delay2 = te/2 - tau2 + GRADIENT_RES;

  if (navigator[0] == 'y') {
    /* tau1, tau2 are the sum of events in each half echo period */
    tau1 = ro_grad.timeFromEcho + pe_grad.duration + crush_grad.duration + ss2_grad.rfCenterFront;
    tau2 = ss2_grad.rfCenterBack + crush_grad.duration + ro_grad.timeToEcho;
    if (FP_GT(tau1,tau2)) {
      te_delay3 = GRADIENT_RES;
      te_delay4 = tau1-tau2+GRADIENT_RES;
    } else {
      te_delay3 = tau2-tau1+GRADIENT_RES;
      te_delay4 = GRADIENT_RES;
    }
    navTime = te_delay3 + ss2_grad.duration + 2*crush_grad.duration + ro_grad.duration + te_delay4 + 2*GRADIENT_RES;
  }

  /* Check nsblock, the number of slabs blocked together
     (used for triggering and/or inversion recovery) */
  check_nsblock();

  /* Min TR *********************************************/   	
  trmin = ss_grad.rfCenterFront  + te + ro_grad.timeFromEcho + pe_grad.duration + 3*GRADIENT_RES;

  /* Increase TR if any options are selected ************/
  if (spoilflag[0] == 'y') trmin += spoil_grad.duration;
  if (navigator[0] == 'y') trmin += navTime;
  if (sat[0] == 'y')       trmin += satTime;
  if (fsat[0] == 'y')      trmin += fsatTime;
  if (mt[0] == 'y')        trmin += mtTime;
  if (ticks > 0)           trmin += GRADIENT_RES;

  /* Adjust for all slabs *******************************/
  trmin *= ns;

  /* Inversion recovery *********************************/
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

  /* TR delay *******************************************/
  tr_delay = granularity((tr-trmin)/ns,GRADIENT_RES);

  /* Calculate B values *********************************/
  if (ix == 1) {
    /* Calculate bvalues according to main diffusion gradients */
    calc_bvalues(&diffusion,"dro","dpe","dsl");
    /* Add components from additional diffusion encoding imaging gradients peculiar to this sequence */
    /* Initialize variables */
    dgro = 0.5*(ror_grad.duration+ro_grad.timeToEcho);
    Gro = ro_grad.m0ref/dgro; Dgro = dgro;
    if (!sepRefocus) Dgro = te-ss_grad.rfCenterBack-ro_grad.timeToEcho;
    dgss = 0.5*(ss_grad.rfCenterBack+ssr_grad.duration);
    Gss = ss_grad.m0ref/dgss; Dgss = dgss;
    dgss2 = ss2_grad.duration/2; Dgss2 = dgss2;
    dcrush = crush_grad.duration-crush_grad.tramp; Dcrush = crush_grad.duration+ss2_grad.duration;
    for (i = 0; i < diffusion.nbval; i++)  {
      /* set droval, dpeval and dslval */
      set_dvalues(&diffusion,&droval,&dpeval,&dslval,i);
      /* Readout */
      diffusion.bro[i] += bval(Gro,dgro,Dgro);
      diffusion.bro[i] += bval(crushsign*gcrushr,dcrush,Dcrush);
      diffusion.bro[i] += bval_nested(gdiff*droval,tdelta,tDELTA,crushsign*gcrushr,dcrush,Dcrush);
      if (!sepRefocus) {
        diffusion.bro[i] += bval_nested(Gro,dgro,Dgro,gdiff*droval,tdelta,tDELTA);
        diffusion.bro[i] += bval_nested(Gro,dgro,Dgro,crushsign*gcrushr,dcrush,Dcrush);
      }
      /* Phase */
      diffusion.bpe[i] += bval(gcrushp,dcrush,Dcrush);
      diffusion.bpe[i] += bval_nested(gdiff*dpeval,tdelta,tDELTA,gcrushp,dcrush,Dcrush);
      /* Slice */
      diffusion.bsl[i] += bval(Gss,dgss,Dgss);
      diffusion.bsl[i] += bval(gcrushs,dcrush,Dcrush);
      diffusion.bsl[i] += bval(ss2_grad.ssamp,dgss2,Dgss2);
      diffusion.bsl[i] += bval_nested(gdiff*dslval,tdelta,tDELTA,gcrushs,dcrush,Dcrush);
      diffusion.bsl[i] += bval_nested(gdiff*dslval,tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
      diffusion.bsl[i] += bval_nested(gcrushs,dcrush,Dcrush,ss2_grad.ssamp,dgss2,Dgss2);
      /* Readout/Phase Cross-terms */
      diffusion.brp[i] += bval2(crushsign*gcrushr,gcrushp,dcrush,Dcrush);
      diffusion.brp[i] += bval_cross(gdiff*dpeval,tdelta,tDELTA,crushsign*gcrushr,dcrush,Dcrush);
      diffusion.brp[i] += bval_cross(gdiff*droval,tdelta,tDELTA,gcrushp,dcrush,Dcrush);
      if (!sepRefocus) {
        diffusion.brp[i] += bval_cross(Gro,dgro,Dgro,gdiff*dpeval,tdelta,tDELTA);
        diffusion.brp[i] += bval_cross(Gro,dgro,Dgro,gcrushp,dcrush,Dcrush);
      }
      /* Readout/Slice Cross-terms */
      diffusion.brs[i] += bval2(crushsign*gcrushr,gcrushs,dcrush,Dcrush);
      diffusion.brs[i] += bval_cross(gdiff*droval,tdelta,tDELTA,gcrushs,dcrush,Dcrush);
      diffusion.brs[i] += bval_cross(gdiff*dslval,tdelta,tDELTA,crushsign*gcrushr,dcrush,Dcrush);
      diffusion.brs[i] += bval_cross(gdiff*droval,tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
      if (!sepRefocus) {
        diffusion.brs[i] += bval_cross(Gro,dgro,Dgro,gdiff*dslval,tdelta,tDELTA);
        diffusion.brs[i] += bval_cross(Gro,dgro,Dgro,gcrushs,dcrush,Dcrush);
        diffusion.brs[i] += bval_cross(Gro,dgro,Dgro,ss2_grad.ssamp,dgss2,Dgss2);
      }
      /* Slice/Phase Cross-terms */
      diffusion.bsp[i] += bval2(gcrushs,gcrushp,dcrush,Dcrush);
      diffusion.bsp[i] += bval_cross(gdiff*dslval,tdelta,tDELTA,gcrushp,dcrush,Dcrush);
      diffusion.bsp[i] += bval_cross(gdiff*dpeval,tdelta,tDELTA,gcrushs,dcrush,Dcrush);
      diffusion.bsp[i] += bval_cross(gdiff*dpeval,tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
      diffusion.bsp[i] += bval_cross(gcrushp,dcrush,Dcrush,ss2_grad.ssamp,dgss2,Dgss2);
    }  /* End for-all-directions */
    /* Write the values */
    write_bvalues(&diffusion,"bval","bvalue","max_bval");
  }

  /* Generate phase-ramped pulses ***********************/
  offsetlist(pss,ss_grad.ssamp,0,freq90,ns,seqcon[1]);
  offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,seqcon[1]);
  shape90 = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freq90,ns,ss_grad.rfFraction,seqcon[1]);
  shape180 = shapelist(p2_rf.pulseName,ss2_grad.rfDuration,freq180,ns,ss2_grad.rfFraction,seqcon[1]);

  /* Set pe_steps for profile or full image *************/   	
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&null_grad);
  F_initval(pe_steps/2.0,vpe_offset);
  pe2_steps = prep_profile(profile[1],nv2,&pe2_grad,&null_grad);
  F_initval(pe2_steps/2.0,vpe2_offset);

  /* Shift DDR for pro **********************************/
  roff = -poffset(pro,ro_grad.roamp);

  /* Adjust experiment time for VnmrJ *******************/
  if (ssc<0) {
    if (seqcon[2] == 's' && seqcon[3]=='s') g_setExpTime(trmean*ntmean*arraydim - ssc*arraydim);
    else if (seqcon[2]=='s') g_setExpTime(trmean*pe_steps*(ntmean*pe2_steps*arraydim - ssc*arraydim));
    else if (seqcon[3]=='s') g_setExpTime(trmean*pe2_steps*(ntmean*pe_steps*arraydim - ssc*arraydim));
    else g_setExpTime(trmean*(ntmean*pe_steps*pe2_steps*arraydim - ssc*arraydim));
  }
  else g_setExpTime(trmean*ntmean*pe_steps*pe2_steps*arraydim + tr*ssc);

  /* Set phase cycle table ******************************/
  if (sepRefocus) settable(t2,1,ph180); // Phase encode is just before readout
  else settable(t2,2,ph180);

  /* PULSE SEQUENCE *************************************/
  status(A);                          // Set status A
  rotate();                           // Set gradient rotation according to psi, phi and theta
  triggerSelect(trigger);             // Select trigger input 1/2/3
  obsoffset(resto);                   // Set spectrometer frequency
  delay(GRADIENT_RES);                // Delay for frequency setting
  initval(fabs(ssc),vssc);            // Compressed steady-state counter
  if (seqcon[2]=='s' && seqcon[3]=='s') assign(zero,vssc); // Zero for standard peloop and pe2loop
  assign(one,vacquire);               // real-time acquire flag
  setacqvar(vacquire);                // Turn on acquire when vacquire is zero 

  /* trigger */
  if (ticks > 0) F_initval((double)nsblock,vtrigblock);

  /* Begin phase-encode loop ****************************/       
  peloop2(seqcon[3],pe2_steps,vpe2_steps,vpe2_ctr);

    peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);

      if (trtype) delay(ns*tr_delay); // relaxation delay

      /* Compressed steady-states: 1st array & transient, all arrays if ssc is negative */
      if ((ix > 1) && (ssc > 0))
	assign(zero,vssc);
      if (seqcon[2] == 'c')
        sub(vpe_ctr,vssc,vpe_ctr);    // vpe_ctr counts up from -ssc
      else if (seqcon[3] == 'c')
        sub(vpe2_ctr,vssc,vpe2_ctr);  // vpe2_ctr counts up from -ssc
      assign(zero,vssc);
      if (seqcon[2] == 's' && seqcon[3]=='s')
	assign(zero,vacquire);        // Always acquire for non-compressed loop
      else {
        if (seqcon[2] == 'c') {
	  ifzero(vpe_ctr);
            assign(zero,vacquire);    // Start acquiring when vpe_ctr reaches zero
	  endif(vpe_ctr);
        }
        else if (seqcon[3] == 'c') {
	  ifzero(vpe2_ctr);
            assign(zero,vacquire);    // Start acquiring when vpe2_ctr reaches zero
	  endif(vpe2_ctr);
        }
      }

      /* Read external kspace table for 1st PE dimension */       
      if (table)
	getelem(t1,vpe_ctr,vpe_mult);
      else {
	ifzero(vacquire);
          sub(vpe_ctr,vpe_offset,vpe_mult);
      	elsenz(vacquire);
          sub(zero,vpe_offset,vpe_mult); // Hold PE mult at initial value for steady states
      	endif(vacquire);
      }

      /* Use standard encoding order for 2nd PE dimension */
      ifzero(vacquire);
        sub(vpe2_ctr,vpe2_offset,vpe2_mult);
      elsenz(vacquire);
        sub(zero,vpe2_offset,vpe2_mult);
      endif(vacquire);

      getelem(t2,vpe_ctr,vph180);
      add(oph,vph180,vph180);         // 180 deg pulse phase alternates +/- 90 from receiver
      mod2(ct,vph2);
      dbl(vph2,vph2);
      add(vph180,vph2,vph180);        // Alternate phase for 180 on odd transients

      /* Begin multislice loop ******************************/       
      msloop(seqcon[1],ns,vms_slices,vms_ctr);

        if (!trtype) delay(tr_delay); // Relaxation delay

        if (ticks > 0) {
          modn(vms_ctr,vtrigblock,vtest);
          ifzero(vtest);              // if the beginning of an trigger block
            xgate(ticks);
            grad_advance(gpropdelay);
            delay(GRADIENT_RES);
          elsenz(vtest);
            delay(GRADIENT_RES);
          endif(vtest);
        }

        sp1on(); delay(GRADIENT_RES); sp1off(); // Scope trigger

        /* Prepulse options ***********************************/
        if (ir[0] == 'y')   inversion_recovery();
        if (sat[0] == 'y')  satbands();
        if (fsat[0] == 'y') fatsat();
        if (mt[0] == 'y')   mtc();

	/* Slice select RF pulse ******************************/ 
	obspower(p1_rf.powerCoarse);
	obspwrf(p1_rf.powerFine);
	delay(GRADIENT_RES);
	obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
	delay(ss_grad.rfDelayFront);
	shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
	delay(ss_grad.rfDelayBack);

        /* Slice refocus gradient *****************************/
        if (sepRefocus) 
          obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);
        else
          /* Include phase encode and readout dephase gradient if refocus gradients not separated */
	  pe2_shapedgradient(pe_grad.name,pe_grad.duration,ror_grad.amp,0,-pe2_grad.offsetamp,
            pe_grad.increment,pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

        if (diff[0] == 'y') {
          delay(diffusion.d1);
          diffusion_dephase(&diffusion,dro,dpe,dsl);
          delay(diffusion.d2);
        } 
        else 
          delay(te_delay1);

	/* Refocusing RF pulse ********************************/ 
	obspower(p2_rf.powerCoarse);
	obspwrf(p2_rf.powerFine);
	delay(GRADIENT_RES);
        obl_shapedgradient(crush_grad.name,crush_grad.duration,crushsign*gcrushr,gcrushp,gcrushs,WAIT);
	obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);
	delay(ss2_grad.rfDelayFront);
	shapedpulselist(shape180,ss2_grad.rfDuration,vph180,rof2,rof2,seqcon[1],vms_ctr);
	delay(ss2_grad.rfDelayBack);
        obl_shapedgradient(crush_grad.name,crush_grad.duration,crushsign*gcrushr,gcrushp,gcrushs,WAIT);

        if (diff[0] == 'y') {
          delay(diffusion.d3);
          diffusion_rephase(&diffusion,dro,dpe,dsl);
          delay(diffusion.d4);
        } 
        else 
          delay(te_delay2);

        /* Readout dephase, phase encode & readout gradients */
        roff = -poffset(pro,ro_grad.roamp); // incase inverted navigator is acquired

        if (sepRefocus) 
          pe2_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,0,-pe_grad.increment,-pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

	/* Readout gradient and acquisition *******************/
	obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
        delay(ro_grad.atDelayFront-alfa);
	startacq(alfa);
	acquire(np,1.0/sw);
	delay(ro_grad.atDelayBack);
	endacq();

        /* Rewind Phase encoding ******************************/
        pe2_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,pe_grad.increment,pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

        /* Navigator acquisition ******************************/
        if (navigator[0] == 'y') {
          delay(te_delay3);
          obl_shapedgradient(crush_grad.name,crush_grad.duration,-crushsign*gcrushr,0,-gcrushs,WAIT);
          obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);
          delay(ss2_grad.rfDelayFront);
          shapedpulselist(shape180,ss2_grad.rfDuration,vph180,rof2,rof2,seqcon[1],vms_ctr);
          delay(ss2_grad.rfDelayBack);
          obl_shapedgradient(crush_grad.name,crush_grad.duration,-crushsign*gcrushr,0,-gcrushs,WAIT);
          delay(te_delay4);
          obl_shapedgradient(ro_grad.name,ro_grad.duration,navsign*ro_grad.amp,0,0,NOWAIT);
          delay(ro_grad.atDelayFront-alfa);
          startacq(alfa);
          acquire(np,1.0/sw);
          delay(ro_grad.atDelayBack);
          endacq();
        }

        if (spoilflag[0] == 'y') {
          obl_shapedgradient(spoil_grad.name,spoil_grad.duration,navsign*spoil_grad.amp,spoil_grad.amp,spoil_grad.amp,WAIT);
        }

      endmsloop(seqcon[1],vms_ctr);

    endpeloop(seqcon[2],vpe_ctr);

  endpeloop(seqcon[3],vpe2_ctr);

  /* Inter-image delay **********************************/
  sub(ntrt,ct,vtrimage);
  decr(vtrimage);
  ifzero(vtrimage);
    delay(trimage);
  endif(vtrimage);

  /* Duty cycle *****************************************/
  calc_grad_duty(tr);

}
