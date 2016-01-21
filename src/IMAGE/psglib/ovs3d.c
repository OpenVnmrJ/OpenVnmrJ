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
Gradient echo imaging sequence
************************************************************************/
#include <standard.h>
#include "sgl.c"

pulsesequence()
{
  /* Internal variable declarations *************************/
  double  freqEx[MAXNSLICE], freqIR[MAXNSLICE];
  double  pe_steps,pespoil_amp;
  double  tau, tauIR=0, te_delay, tr_delay, ti_delay=0, perTime;
  int     table, shapeEx, shapeIR=0;
  char    spoilflag[MAXSTR], per_name[MAXSTR];

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
  int  vrfspoil     = v16;     // RF spoil counter
  int  vtrimage     = v17;     // Counts down from nt, trimage delay when 0

  /* Initialize paramaters **********************************/
  get_parameters();
  get_ovsparameters();
  getstr("spoilflag",spoilflag);                                   

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
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2 );         // excitation pulse
  init_slice(&ss_grad,"ss",thk);                     // slice select gradient
  init_slice_refocus(&ssr_grad,"ssr");               // slice refocus gradient
  init_readout(&ro_grad,"ro",lro,np,sw);             // readout gradient
  init_readout_refocus(&ror_grad,"ror");             // dephase gradient
  init_phase(&pe_grad,"pe",lpe,nv);                  // phase encode gradient
  init_phase(&per_grad,"per",lpe,nv);                // phase encode rewinder
  init_phase(&pe2_grad,"pe2",lpe2,nv2);              // 2nd phase encode gradient
  init_phase(&pe2r_grad,"pe2r",lpe2,nv2);            // 2nd phase encode rewinder
  init_generic(&spoil_grad,"spoil",gspoil,tspoil);

  /* RF Calculations ****************************************/
  calc_rf(&p1_rf,"tpwr1","tpwr1f");

  /* Gradient calculations **********************************/
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_readout(&ro_grad, WRITE, "gro","sw","at");
  calc_readout_refocus(&ror_grad, &ro_grad, NOWRITE, "gror");
  calc_phase(&pe_grad, NOWRITE, "gpe","tpe");
  pe2_grad.areaOffset = ss_grad.m0ref;
  calc_phase(&pe2_grad, NOWRITE, "gpe2","");

  /* Equalize refocus and PE gradient durations *************/
  calc_sim_gradient(&ror_grad, &pe_grad, &pe2_grad,tpemin, WRITE);

  /* Calculate phase-rewind & spoiler gradients *************/
  pespoil_amp = 0.0;
  perTime     = 0.0;
  if ((perewind[0] == 'y') && (spoilflag[0] == 'n')) {       // Rewinder, no spoiler
    calc_phase(&per_grad, NOWRITE,"","");
    calc_phase(&pe2r_grad,NOWRITE,"","");
    calc_sim_gradient(&per_grad,&pe2r_grad,&null_grad,0.0,WRITE);
    strcpy(per_name,per_grad.name);
    perTime        = per_grad.duration;
    spoil_grad.amp = 0.0;
  }
  else if ((perewind[0] == 'n') && (spoilflag[0] == 'y')) {  // Spoiler, no rewinder
    calc_generic(&spoil_grad,WRITE,"","");
    strcpy(per_name,spoil_grad.name);
    perTime     = spoil_grad.duration;
    pespoil_amp = spoil_grad.amp;      // Apply spoiler on PE & PE2 axis if no rewinder
  }
  else if ((perewind[0] == 'y') && (spoilflag[0] == 'y')) {  // Rewinder and spoiler
    calc_phase(&per_grad,    NOWRITE,"","");
    calc_phase(&pe2r_grad,   NOWRITE,"","");
    calc_generic(&spoil_grad,NOWRITE,"","");
    calc_sim_gradient(&per_grad,&pe2r_grad,&spoil_grad,0.0,WRITE);
    strcpy(per_name,per_grad.name);
    perTime = per_grad.duration;
  }


  /* Create optional prepulse events ************************/
  if (sat[0]  == 'y') create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (mt[0]   == 'y') create_mtc();
  if (ir[0]   == 'y') {
    init_rf(&ir_rf,pipat,pi,flipir,rof2,rof2); 
    calc_rf(&ir_rf,"tpwri","tpwrif");
    init_slice_butterfly(&ssi_grad,"ssi",thk,gcrush,tcrush);
    calc_slice(&ssi_grad,&ir_rf,WRITE,"gssi");

    tauIR = ss_grad.duration - ss_grad.rfCenterBack; // Duration of ss_grad before RF center
    ti_delay = ti - (ssi_grad.rfCenterFront + tauIR);

    if (ti_delay < 0) {
      abort_message("TI too short, Minimum TI = %.2fms\n",(ti-ti_delay)*1000);
    }
    irTime = 4e-6 + ti + ssi_grad.duration - ssi_grad.rfCenterBack;  // Time to add to TR
  }
  
  /* Optional Outer Volume Suppression */
  if (ovs[0] == 'y') {
    /* Must set up a few voxel specific parameters for create_ovsbands() to function */
    vox1_grad.thickness   = vox1;
    vox2_grad.thickness   = vox2;
    vox3_grad.thickness   = vox3;
    vox1_grad.rfBandwidth = vox2_grad.rfBandwidth = vox3_grad.rfBandwidth = p1_rf.bandwidth;

    create_ovsbands();
  }

  /* Check that all Gradient calculations are ok ************/
  sgl_error_check(sglerror);


  /* Min TE ******************************************/
  tau   = ss_grad.rfCenterBack + pe_grad.duration + alfa + ro_grad.timeToEcho;

  temin = tau + 4e-6;  /* ensure that te_delay is at least 4us */
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (te < temin) {
    abort_message("TE too short.  Minimum TE= %.2fms\n",temin*1000+0.005);   
  }
  te_delay = te - tau;
   
  /* Min TR ******************************************/   	
  tau  = ss_grad.duration + te_delay + pe_grad.duration + ro_grad.duration + per_grad.duration;
  tau += (2*4e-6 + alfa);

  /* Increase TR if any options are selected****************/
  if (sat[0]      == 'y')            tau += satTime;
  if (fsat[0]     == 'y')            tau += fsatTime;
  if (mt[0]       == 'y')            tau += mtTime;
  if (ir[0] == 'y') {
    trmin += irTime;
    trmin -= tauIR;  /* subtract out ss_grad which was already included in TR */
  }
  if (ovs[0] == 'y') tau += ovsTime;

  trmin = tau + 4e-6;  /* ensure that tr_delay is at least 4us */
  trmin *= ns;
  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr+1e-6);
  }
  if (tr < trmin) {
    abort_message("TR too short.  Minimum TR= %.2fms\n",trmin*1000+0.005);   
  }
  tr_delay = (tr - tau)/ns;

  /* Set up frequency offset pulse shape list ********/   	
  offsetlist(pss,ss_grad.ssamp,0,freqEx,ns,seqcon[1]);
  shapeEx = shapelist(p1pat,ss_grad.rfDuration,freqEx,ns,0,seqcon[1]);
  if (ir[0] == 'y') {
    offsetlist(pss,ssi_grad.ssamp,0,freqIR,ns,seqcon[1]);
    shapeIR = shapelist(pipat,ssi_grad.rfDuration,freqIR,ns,0,seqcon[1]);
  }
  
  /* Set pe_steps for profile or full image **********/   	
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&per_grad);
  initval(pe_steps/2.0,vpe_offset);

  pe2_steps = prep_profile(profile[1],nv2,&pe2_grad,&pe2r_grad);
  initval(pe2_steps/2.0,vpe2_offset);

  /* Shift DDR for pro *******************************/   	
  roff = -poffset(pro,ro_grad.roamp);

  g_setExpTime(tr*(nt*pe_steps*pe2_steps*arraydim + ssc));

  /* PULSE SEQUENCE *************************************/
  status(A);
  rotate();
  obsoffset(resto);
  delay(4e-6);
  initval(fabs(ssc),vssc);      // Compressed steady-state counter
  assign(zero,vrfspoil_ctr);    // RF spoil phase counter
  assign(one,vacquire);         // real-time acquire flag

  /* Begin phase-encode loop ****************************/       
  peloop(seqcon[3],pe2_steps,vpe2_steps,vpe2_ctr);
    peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);

      /* Compressed steady-states: 1st array & transient, all arrays if ssc is negative */
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
      setacqvar(vacquire);        // Turn on acquire when vacquire is zero 

      /* Set rcvr/xmtr phase for RF spoiling *******************/
      if (rfspoil[0] == 'y') {
	incr(vrfspoil_ctr);
	mult(vrfspoil_ctr,vrfspoil_ctr,vrfspoil);
	add(vrfspoil,vrfspoil_ctr,vrfspoil);
	hlv(vrfspoil,vrfspoil);
	xmtrphase(vrfspoil);
	rcvrphase(vrfspoil);
      }

      /* Read external kspace table for 1st PE dimension ****/       
      if (table)
	getelem(t1,vpe_ctr,vpe_mult);
      else {
	ifzero(vacquire);
          sub(vpe_ctr,vpe_offset,vpe_mult);
      	elsenz(vacquire);
          sub(zero,vpe_offset,vpe_mult);      // Hold PE mult at initial value for steady states
      	endif(vacquire);
      }

      /* Use standard encoding order for 2nd PE dimension */
      ifzero(vacquire);
        sub(vpe2_ctr,vpe2_offset,vpe2_mult);
      elsenz(vacquire);
        sub(zero,vpe2_offset,vpe2_mult);
      endif(vacquire);

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
	/* Selectable trigger input *******************/       
	triggerSelect(trigger);
	xgate(ticks);

	/* TTL scope trigger **********************************/       
	sp1on(); delay(4e-6); sp1off();

	/* Prepulse options ***********************************/       
	if (sat[0]  == 'y') satbands();
	if (fsat[0] == 'y') fatsat();
	if (mt[0]   == 'y') mtc();
        if (ovs[0]  == 'y') {ovsbands(); rotate();}

	/* Optional IR pulse **********************************/ 
	if (ir[0] == 'y') {
	  obspower(ir_rf.powerCoarse);
	  obspwrf(ir_rf.powerFine);
	  delay(4e-6);
	  obl_shapedgradient(ssi_grad.name,ssi_grad.duration,0,0,ssi_grad.amp,NOWAIT);
	  delay(ssi_grad.rfDelayFront);
	  shapedpulselist(shapeIR,ssi_grad.rfDuration,oph,rof2,rof2,seqcon[1],vms_ctr);
	  delay(ssi_grad.rfDelayBack);
	  delay(ti_delay);
	}

	/* Slice select RF pulse ******************************/ 
	obspower(p1_rf.powerCoarse);
	obspwrf(p1_rf.powerFine);
	delay(4e-6);
	obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
	delay(ss_grad.rfDelayFront);
	shapedpulselist(shapeEx,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
	delay(ss_grad.rfDelayBack);

	/* Phase encode, refocus, and dephase gradient ********/
	pe2_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,-pe2_grad.offsetamp,
            -pe_grad.increment,-pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

	/* TE delay *******************************************/
	delay(te_delay);

	/* Readout gradient and acquisition ********************/
	obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
	delay(ro_grad.atDelayFront);
	startacq(alfa);
	acquire(np,1.0/sw);
	delay(ro_grad.atDelayBack);
	endacq();

	/* Rewind / spoiler gradient *********************************/
	if (perewind[0] == 'y' || (spoilflag[0] == 'y')) {
          pe2_shapedgradient(per_name,perTime,spoil_grad.amp,pespoil_amp,pespoil_amp,
            per_grad.increment,pe2r_grad.increment,vper_mult,vpe2r_mult,WAIT);
	}

	/* Relaxation delay ***********************************/       
	if (!trtype)
          delay(tr_delay);
      endmsloop(seqcon[1],vms_ctr);

      if (trtype)
	delay(ns*tr_delay);
    endpeloop(seqcon[2],vpe_ctr);
  endpeloop(seqcon[3],vpe2_ctr);

  /* Inter-image delay **********************************/
  sub(ntrt,ct,vtrimage);
  decr(vtrimage);
  ifzero(vtrimage);
    delay(trimage);
  endif(vtrimage);

}
