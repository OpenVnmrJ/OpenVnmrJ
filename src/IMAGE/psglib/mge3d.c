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
    MGE3D.C
    
    3D Multi gradient echo imaging sequence

***********************************************************************/

#include <standard.h>
#include "sgl.c"

/* Refocus gradient structure */
GENERIC_GRADIENT_T ref_grad;

pulsesequence()
{
  /* Internal variable declarations *************************/
  double  freqEx[MAXNSLICE];
  double  pespoil_amp,maxgradtime,spoilMoment,perTime,pe2_offsetamp,tau1,te_delay,tr_delay;
  double  te2=0.0,te3=0.0,te2min,te3min,tau2,tau3,te2_delay,te3_delay=0;
  char    minte2[MAXSTR],minte3[MAXSTR],spoilflag[MAXSTR],perName[MAXSTR];
  int     sepSliceRephase,sepReadRephase=0,readrev,table,shapeEx;
  int     i;

  /* Real-time variables used in this sequence **************/
  int  vpe_steps    = v1;      // Number of PE steps
  int  vpe_ctr      = v2;      // PE loop counter
  int  vms_slices   = v3;      // Number of slices
  int  vms_ctr      = v4;      // Slice loop counter
  int  vpe_offset   = v5;      // PE/2 for non-table offset
  int  vpe_mult     = v6;      // PE multiplier, ranges from -PE/2 to PE/2
  int  vper_mult    = v7;      // PE rewinder multiplier; turn off rewinder when 0
  int  vpe2_steps   = v8;      // Number of PE2 steps
  int  vpe2_ctr     = v9;      // PE2 loop counter
  int  vpe2_mult    = v10;     // PE2 multiplier
  int  vpe2_offset  = v11;     // PE2/2 for non-table offset
  int  vpe2r_mult   = v12;     // PE2 rewinder multiplier
  int  vssc         = v13;     // Compressed steady-states
  int  vacquire     = v14;     // Argument for setacqvar, to skip steady state acquires
  int  vrfspoil_ctr = v15;     // RF spoil counter
  int  vrfspoil     = v16;     // RF spoil multiplier
  int  vtrimage     = v17;     // Counts down from nt, trimage delay when 0
  int  vne          = v18;     // Number of echoes
  int  vne_ctr      = v19;     // Echo loop counter
  int  vneindex     = v20;     // Echo index, odd or even
  int  vnelast      = v21;     // Check for last echo
  int  vtrigblock   = v22;     // Number of slices per trigger block

  /* Initialize paramaters **********************************/
  init_mri();

  getstr("spoilflag",spoilflag);
  te2=getval("te2");
  te3=getval("te3");
  getstr("minte2",minte2);
  getstr("minte3",minte3);
  readrev=(int)getval("readrev");

  /*  Check for external PE table ***************************/
  table = 0;
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    table = 1;
  }

  /* Set Rcvr/Xmtr phase increments for RF Spoiling ********/
  /* Ref:  Zur, Y., Magn. Res. Med., 21, 251, (1991) *******/
  if (rfspoil[0] == 'y') {
    rcvrstepsize(rfphase);
    obsstepsize(rfphase);
  }

  /* Initialize gradient structures *************************/
  shape_rf(&p1_rf,"p1",p1pat,p1,flip1,rof1,rof2 );   // excitation pulse
  init_slice(&ss_grad,"ss",thk);                     // slice select gradient
  init_slice_refocus(&ssr_grad,"ssr");               // slice refocus gradient
  init_readout(&ro_grad,"ro",lro,np,sw);             // readout gradient
  init_readout_refocus(&ror_grad,"ror");             // dephase gradient
  init_phase(&pe_grad,"pe",lpe,nv);                  // phase encode gradient
  init_phase(&pe2_grad,"pe2",lpe2,nv2);              // 2nd phase encode gradient
  init_dephase(&spoil_grad,"spoil");                 // optimized spoiler
  init_dephase(&ref_grad,"ref");                     // readout rephase

  /* RF Calculations ****************************************/
  calc_rf(&p1_rf,"tpwr1","tpwr1f");

  /* Gradient calculations **********************************/
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice_refocus(&ssr_grad, &ss_grad, WRITE,"gssr");
  calc_readout(&ro_grad, WRITE,"gro","sw","at");
  calc_readout_refocus(&ror_grad,&ro_grad,NOWRITE,"gror");
  calc_phase(&pe_grad, NOWRITE,"gpe","tpe");
  calc_phase(&pe2_grad,NOWRITE,"gpe2","");
  calc_dephase(&ref_grad,WRITE,ro_grad.m0,"","");

  if (spoilflag[0] == 'y') {
    spoilMoment = ro_grad.acqTime*ro_grad.roamp;   // Optimal spoiling is at*gro for 2pi per pixel
    spoilMoment -= ro_grad.m0def;                  // Subtract partial spoiling from back half of readout
    if (perewind[0] == 'y') 
      calc_dephase(&spoil_grad,NOWRITE,spoilMoment,"gspoil","tspoil");
    else
      calc_dephase(&spoil_grad,WRITE,spoilMoment,"gspoil","tspoil");
  }

  /* Is TE long enough for separate slab refocus? *******/
  maxgradtime = MAX(ror_grad.duration,pe_grad.duration);
  if (spoilflag[0] == 'y')
    maxgradtime = MAX(maxgradtime,spoil_grad.duration);
  tau1 = ss_grad.rfCenterBack + ssr_grad.duration + maxgradtime + alfa + ro_grad.timeToEcho + 4e-6;
  if ((te >= tau1) && (minte[0] != 'y')) sepSliceRephase = 1; // Set flag for separate slice rephase
  else {
    sepSliceRephase = 0;
    pe2_grad.areaOffset = ss_grad.m0ref;                 // Add slab refocus on pe2 axis
    calc_phase(&pe2_grad,NOWRITE,"gpe2","");             // Recalculate pe2 to include slab refocus
  }

  /* Equalize refocus and PE gradient durations *************/
  pespoil_amp = 0.0;
  perTime = 0.0;
  if ((perewind[0] == 'y') && (spoilflag[0] == 'y')) {   // All four must be single shape
    if (ror_grad.duration > spoil_grad.duration) {       // calc_sim first with ror
      calc_sim_gradient(&pe_grad,&pe2_grad,&ror_grad,tpemin,WRITE);
      calc_sim_gradient(&ror_grad,&spoil_grad,&null_grad,tpemin,NOWRITE);
    } else {                                             // calc_sim first with spoil
      calc_sim_gradient(&pe_grad,&pe2_grad,&spoil_grad,tpemin,WRITE);
      calc_sim_gradient(&ror_grad,&spoil_grad,&null_grad,tpemin,WRITE);      
    }
    strcpy(perName,pe_grad.name);
    perTime = pe_grad.duration;
  } else {                      // post-acquire shape will be either pe or spoil, but not both
    calc_sim_gradient(&ror_grad,&pe_grad,&pe2_grad,tpemin,WRITE);
    if ((perewind[0] == 'y') && (spoilflag[0] == 'n')) {     // Rewinder, no spoiler
      strcpy(perName,pe_grad.name);
      perTime = pe_grad.duration;
      spoil_grad.amp = 0.0;
    } else if ((perewind[0] == 'n') && (spoilflag[0] == 'y')) {  // Spoiler, no rewinder
      strcpy(perName,spoil_grad.name);
      perTime = spoil_grad.duration;
      pespoil_amp = spoil_grad.amp;      // Apply spoiler on PE & PE2 axis if no rewinder
    }
  }
  pe2_offsetamp = sepSliceRephase ? 0.0 : pe2_grad.offsetamp;  // pe2 slab refocus

  /* Create optional prepulse events ************************/
  if (sat[0] == 'y')  create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (mt[0] == 'y')   create_mtc();
  if (ir[0] == 'y')   create_inversion_recovery();

  /* Set up frequency offset pulse shape list ********/   	
  offsetlist(pss,ss_grad.ssamp,0,freqEx,ns,seqcon[1]);
  shapeEx = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freqEx,ns,ss_grad.rfFraction,seqcon[1]);
  
  /* Check that all Gradient calculations are ok ************/
  sgl_error_check(sglerror);

  /* Min TE ******************************************/
  tau1 = ss_grad.rfCenterBack + pe_grad.duration + alfa + ro_grad.timeToEcho;
  tau1 += (sepSliceRephase) ? ssr_grad.duration : 0.0;   // Add slice refocusing if separate event

  temin = tau1 + 4e-6;  /* ensure that te_delay is at least 4us */
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (te < temin) {
    abort_message("TE too short.  Minimum TE= %.2fms\n",temin*1000+0.005);   
  }
  te_delay = te - tau1;

  /* Min TE2 *****************************************/
  tau2 = (readrev) ? 2*ro_grad.timeFromEcho+alfa : ro_grad.duration+ref_grad.duration;
  te2min = tau2 + 4e-6;
  if (minte2[0] == 'y') {
    te2 = te2min;
    putvalue("te2",te2);
  }
  if (te2 < te2min) {
    abort_message("TE2 too short.  Minimum TE2= %.2fms\n",te2min*1000+0.005);
  }

  if (readrev) te2_delay = te2 - tau2;
  else {
    tau2 = ro_grad.duration + 2*ror_grad.duration;
    if (te2 >= tau2) {
      sepReadRephase = 1; // Set flag for separate read rephase
      te2_delay = te2 - ro_grad.duration - 2*ror_grad.duration;
    } else {
      sepReadRephase = 0;
      if (te2 > te2min+4e-6) {
        ref_grad.duration = granularity(te2-ro_grad.duration-8e-6,GRADIENT_RES);
        ref_grad.calcFlag = AMPLITUDE_FROM_MOMENT_DURATION;
        calc_dephase(&ref_grad,WRITE,ro_grad.m0,"","");
      }
      te2_delay = te2 - ro_grad.duration - ref_grad.duration;
    }
  }

  /* Min TE3 *****************************************/
  if (readrev) {  
    tau3 = 2*ro_grad.timeToEcho + alfa;
    te3min = tau3 + 4e-6;
    if (minte3[0] == 'y') {
      te3 = te3min;
      putvalue("te3",te3);
    }
    if (te3 < te3min) {
      abort_message("TE3 too short.  Minimum TE3= %.2fms\n",te3min*1000+0.005);
    }
    te3_delay = te3 - tau3;
  }

  /* Now set the TE array accordingly */
  putCmd("TE = 0"); /* Re-initialize TE */
  putCmd("TE[1] = %f",te*1000);
  if (readrev) {
    for (i=1;i<ne;i++) {
      if (i%2 == 0) putCmd("TE[%d] = TE[%d]+%f",i+1,i,te3*1000);
      else putCmd("TE[%d] = TE[%d]+%f",i+1,i,te2*1000);
    }
  } else {
    for (i=1;i<ne;i++) putCmd("TE[%d] = TE[%d]+%f",i+1,i,te2*1000);
  }

  /* Check nsblock, the number of slices blocked together
     (used for triggering and/or inversion recovery) */
  check_nsblock();

  /* Min TR ******************************************/
  trmin  = ss_grad.duration + te_delay + pe_grad.duration + ne*ro_grad.duration + perTime + 8e-6;
  trmin += (sepSliceRephase) ? ssr_grad.duration : 0.0;   // Add slice refocusing if separate event
  if (readrev) trmin += (ne/2)*te2_delay + ((ne-1)/2)*te3_delay;
  else trmin += (sepReadRephase) ? (ne-1)*(te2_delay+2*ror_grad.duration) : (ne-1)*(te2_delay+ref_grad.duration);

  /* Increase TR if any options are selected *********/
  if (sat[0] == 'y')  trmin += satTime;
  if (fsat[0] == 'y') trmin += fsatTime;
  if (mt[0] == 'y')   trmin += mtTime;
  if (ticks > 0) trmin += 4e-6;

  /* Adjust for all slices ***************************/
  trmin *= ns;

  /* Inversion recovery *********************************/
  if (ir[0] == 'y') {
    /* tiaddTime is the additional time beyond IR component to be included in ti */
    /* satTime, fsatTime and mtTime all included as those modules will be after IR */
    tiaddTime = satTime + fsatTime + mtTime + 4e-6 + ss_grad.rfCenterFront;
    /* calc_irTime checks ti and returns the time of all IR components */
    trmin += calc_irTime(tiaddTime,trmin,mintr[0],tr,&trtype);
  }

  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr);
  }
  if (FP_LT(tr,trmin)) {
    abort_message("TR too short.  Minimum TR = %.2fms\n",trmin*1000+0.005);
  }

  /* Calculate tr delay */
  tr_delay = granularity((tr-trmin)/ns,GRADIENT_RES);

  /* Set pe_steps for profile or full image **********/   	
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&per_grad);
  F_initval(pe_steps/2.0,vpe_offset);

  pe2_steps = prep_profile(profile[1],nv2,&pe2_grad,&null_grad);
  F_initval(pe2_steps/2.0,vpe2_offset);

  /* Shift DDR for pro *******************************/   	
  roff = -poffset(pro,ro_grad.roamp);

  /* Adjust experiment time for VnmrJ *******************/
  if (ssc<0) {
    if (seqcon[2]=='s') g_setExpTime(trmean*ntmean*pe_steps*pe2_steps*arraydim);
    else if (seqcon[3]=='s') g_setExpTime(trmean*pe2_steps*(ntmean*pe_steps*arraydim - ssc*arraydim));
    else g_setExpTime(trmean*(ntmean*pe_steps*pe2_steps*arraydim - ssc*arraydim));
  } else {
    if (seqcon[2]=='s') g_setExpTime(trmean*ntmean*pe_steps*pe2_steps*arraydim);
    else g_setExpTime(trmean*ntmean*pe_steps*pe2_steps*arraydim + tr*ssc);
  }

  /* Return parameters to VnmrJ */
  putvalue("tror",ror_grad.duration);  // ROR duration
  putvalue("gpe",pe_grad.peamp);       // PE max grad amp
  putvalue("gss",ss_grad.ssamp);       // Excitation slice grad
  putvalue("gro",ro_grad.roamp);       // RO grad

  /* PULSE SEQUENCE ***************************************/
  status(A);
  rotate();
  triggerSelect(trigger);       // Select trigger input 1/2/3
  obsoffset(resto);
  delay(4e-6);
  initval(fabs(ssc),vssc);      // Compressed steady-state counter
  if (seqcon[2]=='s') assign(zero,vssc); // Zero for standard peloop
  assign(zero,vrfspoil_ctr);    // RF spoil phase counter
  assign(zero,vrfspoil);        // RF spoil multiplier
  assign(one,vacquire);         // real-time acquire flag
  setacqvar(vacquire);          // Turn on acquire when vacquire is zero 

  /* trigger */
  if (ticks > 0) F_initval((double)nsblock,vtrigblock);

  /* Begin phase-encode loop ****************************/       
  peloop2(seqcon[3],pe2_steps,vpe2_steps,vpe2_ctr);

    /* Begin phase-encode loop ****************************/       
    peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);

      if (trtype) delay(ns*tr_delay);   // relaxation delay

      /* Compressed steady-states: 
         1st array & transient, all arrays if ssc is negative */
      if ((ix > 1) && (ssc > 0))
        assign(zero,vssc);
      sub(vpe_ctr,vssc,vpe_ctr);  // vpe_ctr counts up from -ssc
      assign(zero,vssc);
      if (seqcon[2] == 's')
        assign(zero,vacquire);    // Always acquire for non-compressed loop
      else {
        ifzero(vpe_ctr);
          assign(zero,vacquire);  // Start acquiring when vpe_ctr reaches zero
        endif(vpe_ctr);
      }

      /* Use standard encoding order for 2nd PE dimension */
      ifzero(vacquire);
        sub(vpe2_ctr,vpe2_offset,vpe2_mult);
      elsenz(vacquire);
        sub(zero,vpe2_offset,vpe2_mult);
      endif(vacquire);

      /* Set rcvr/xmtr phase for RF spoiling *******************/
      if (rfspoil[0] == 'y') {
        incr(vrfspoil_ctr);                    // vrfspoil_ctr = 1  2  3  4  5  6
        add(vrfspoil,vrfspoil_ctr,vrfspoil);   // vrfspoil =     1  3  6 10 15 21
        xmtrphase(vrfspoil);
        rcvrphase(vrfspoil);
      }

      /* Read external kspace table if set ******************/       
      if (table)
        getelem(t1,vpe_ctr,vpe_mult);
      else {
        ifzero(vacquire);
          sub(vpe_ctr,vpe_offset,vpe_mult);
        elsenz(vacquire);
          sub(zero,vpe_offset,vpe_mult);  // Hold PE mult at initial value for steady states
        endif(vacquire);
      }

      /* PE rewinder follows PE table; zero if turned off ***/       
      if (perewind[0] == 'y') {
        assign(vpe_mult,vper_mult);
        assign(vpe2_mult,vpe2r_mult);
      }
      else {
        assign(zero,vper_mult);
        assign(zero,vpe2r_mult);
      }

      /* Begin multislice loop ******************************/       
      msloop(seqcon[1],ns,vms_slices,vms_ctr);

        if (!trtype) delay(tr_delay);   // Relaxation delay

        if (ticks > 0) {
          modn(vms_ctr,vtrigblock,vtest);
          ifzero(vtest);                // if the beginning of an trigger block
            xgate(ticks);
            grad_advance(gpropdelay);
            delay(4e-6);
          elsenz(vtest);
            delay(4e-6);
          endif(vtest);
        }

        /* TTL scope trigger **********************************/       
        sp1on(); delay(4e-6); sp1off();

        /* Prepulse options ***********************************/       
        if (ir[0] == 'y')   inversion_recovery();
        if (sat[0] == 'y')  satbands();
        if (fsat[0] == 'y') fatsat();
        if (mt[0] == 'y')   mtc();

        /* Slice select RF pulse ******************************/ 
        obspower(p1_rf.powerCoarse);
        obspwrf(p1_rf.powerFine);
        delay(4e-6);
        obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
        delay(ss_grad.rfDelayFront);
        shapedpulselist(shapeEx,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
        delay(ss_grad.rfDelayBack);

       /* Phase encode, refocus, and dephase gradient ********/
        if (sepSliceRephase) {                // separate slice refocus gradient
          obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);
          delay(te_delay);                    // delay between slab refocus and pe
          pe2_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,-pe2_offsetamp,
            -pe_grad.increment,-pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);
        } else {
	  pe2_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,-pe2_offsetamp,
            -pe_grad.increment,-pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);
          delay(te_delay);                    // delay after refocus/pe
        }

        F_initval(ne,vne);
        loop(vne,vne_ctr);

          if (readrev) {
            mod2(vne_ctr,vneindex);
            ifzero(vneindex);
              /* Shift DDR for pro *******************************/
              roff = -poffset(pro,ro_grad.roamp);
              /* Readout gradient ********************************/
              obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
              delay(ro_grad.atDelayFront);
              /* Acquisition ***************************************/
              startacq(alfa);
              acquire(np,1.0/sw);
              delay(ro_grad.atDelayBack);
              endacq();
              sub(vne,vne_ctr,vnelast);
              sub(vnelast,one,vnelast);
              ifzero(vnelast);
              elsenz(vnelast);
                delay(te2_delay);
              endif(vnelast);
            elsenz(vneindex);
              /* Shift DDR for pro *******************************/
              roff = -poffset(pro,-ro_grad.roamp);
              /* Readout gradient ********************************/
              obl_shapedgradient(ro_grad.name,ro_grad.duration,-ro_grad.amp,0,0,NOWAIT);
              delay(ro_grad.atDelayFront);
              /* Acquisition ***************************************/
              startacq(alfa);
              acquire(np,1.0/sw);
              delay(ro_grad.atDelayBack);
              endacq();
              sub(vne,vne_ctr,vnelast);
              sub(vnelast,one,vnelast);
              ifzero(vnelast);
              elsenz(vnelast);
                delay(te3_delay);
              endif(vnelast);
            endif(vneindex);
          } else {
            /* Shift DDR for pro *******************************/
            roff = -poffset(pro,ro_grad.roamp);
            /* Readout gradient ********************************/
            obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
            delay(ro_grad.atDelayFront);
            /* Acquisition ***************************************/
            startacq(alfa);
            acquire(np,1.0/sw);
            delay(ro_grad.atDelayBack);
            endacq();
            sub(vne,vne_ctr,vnelast);
            sub(vnelast,one,vnelast);
            ifzero(vnelast);
            elsenz(vnelast);
              if (sepReadRephase) {
                obl_shapedgradient(ror_grad.name,ror_grad.duration,-ror_grad.amp,0,0,WAIT);
                delay(te2_delay);
                obl_shapedgradient(ror_grad.name,ror_grad.duration,-ror_grad.amp,0,0,WAIT);
              } else {
                obl_shapedgradient(ref_grad.name,ref_grad.duration,-ref_grad.amp,0,0,WAIT);
                delay(te2_delay);
              }
            endif(vnelast);
          }

        endloop(vne_ctr);

        /* Rewind / spoiler gradient **************************/
        if ((perewind[0] == 'y') || (spoilflag[0] == 'y')) {
          pe2_shapedgradient(perName,perTime,spoil_grad.amp,pespoil_amp,pespoil_amp,
            pe_grad.increment,pe2_grad.increment,vper_mult,vpe2r_mult,WAIT);
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
}
