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
3D Gradient echo imaging sequence
************************************************************************/

#include <standard.h>
#include "sgl.c"

void pulsesequence()
{
  /* Internal variable declarations *************************/
  double  freqEx[MAXNSLICE];
  double  pespoil_amp,spoilMoment,maxgradtime,pe2_offsetamp;
  double  tetime,trtime,te_delay,tr_delay,perTime;
  int     table=0,shapeEx,sepSliceRephase=0;
  char    spoilflag[MAXSTR],perName[MAXSTR],skiptab[MAXSTR];;
  int     *skiptabvals;

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
  int  vtrigblock   = v18;     // Number of slices per trigger block
  int  vtabindex    = v19;     // Index for elliptical skip table
  int  vtemp1       = v20;     // Spare for calculations

  /* Initialize paramaters **********************************/
  init_mri();
  getstr("spoilflag",spoilflag);
  getstr("skiptab",skiptab);

  /*  Check for external PE table ***************************/
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    table = 1;
  }

  if (skiptab[0] == 'y') {
    if (seqcon[2] != 'c' && seqcon[3] != 'c')
      abort_message("ge3d_elliptical: must be compressed in both PE and PE2 dimensions for skiptab=y option");
    skiptabvals = (int *) malloc(nv*nv2*sizeof(int));
    create_skiptab(nv,nv2,skiptabvals);
    settable(t3,nv*nv2,skiptabvals);
  }

  /* Set Rcvr/Xmtr phase increments for RF Spoiling ********/
  /* Ref:  Zur, Y., Magn. Res. Med., 21, 251, (1991) *******/
  if (rfspoil[0] == 'y') {
    rcvrstepsize(rfphase);
    obsstepsize(rfphase);
  }

  /* Initialize gradient structures *************************/
  init_slice(&ss_grad,"ss",thk);                     // Slab select gradient
  init_slice_refocus(&ssr_grad,"ssr");               // Slab refocus gradient
  init_readout(&ro_grad,"ro",lro,np,sw);             // Read gradient
  init_readout_refocus(&ror_grad,"ror");             // Read dephase gradient
  init_phase(&pe_grad,"pe",lpe,nv);                  // Phase encode gradient
  init_phase(&pe2_grad,"pe2",lpe2,nv2);              // 2nd phase encode gradient

  if (spoilflag[0] == 'y') {
    init_dephase(&spoil_grad,"spoil");               // Optimized spoiler
  }

  /* RF Calculations ****************************************/
  shape_rf(&p1_rf,"p1",p1pat,p1,flip1,rof1,rof2 );   // Excitation pulse
  calc_rf(&p1_rf,"tpwr1","tpwr1f");

  /* Gradient calculations **********************************/
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice_refocus(&ssr_grad,&ss_grad,WRITE,"gssr");
  calc_readout(&ro_grad,WRITE,"gro","sw","at");
  calc_readout_refocus(&ror_grad,&ro_grad,NOWRITE,"gror");
  calc_phase(&pe_grad,NOWRITE,"gpe","tpe");          // Rewinder uses same shape
  calc_phase(&pe2_grad,NOWRITE,"gpe2","");           // Rewinder uses same shape

  if (spoilflag[0] == 'y') {                         // Calculate spoil grad if spoiling is turned on
    spoilMoment = ro_grad.acqTime*ro_grad.roamp;     // Optimal spoiling is at*gro for 2pi per pixel
    spoilMoment -= ro_grad.m0def;                    // Subtract partial spoiling from back half of readout
    calc_dephase(&spoil_grad,WRITE,spoilMoment,"gspoil","tspoil");
  }

  /* Is TE long enough for separate slab refocus? ***********/
  maxgradtime = MAX(ror_grad.duration,MAX(pe_grad.duration,pe2_grad.duration));
  if (spoilflag[0] == 'y')
    maxgradtime = MAX(maxgradtime,spoil_grad.duration);
  tetime = ss_grad.rfCenterBack + ssr_grad.duration + maxgradtime + alfa + ro_grad.timeToEcho + 4e-6;
  if ((te >= tetime) && (minte[0] != 'y')) {
    sepSliceRephase = 1;                                 // Set flag for separate slice rephase
  } else {
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
      calc_sim_gradient(&ror_grad,&spoil_grad,&null_grad,tpemin,NOWRITE);
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
  
  sgl_error_check(sglerror);                               // Check for any SGL errors

  /* Min TE ******************************************/
  tetime = ss_grad.rfCenterBack + pe_grad.duration + alfa + ro_grad.timeToEcho;
  tetime += (sepSliceRephase) ? ssr_grad.duration : 0.0;   // Add slice refocusing if separate event

  temin = tetime + 4e-6;                                   // Ensure that te_delay is at least 4us
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (te < temin) {
    abort_message("TE too short.  Minimum TE= %.2fms\n",temin*1000+0.005);   
  }
  te_delay = te - tetime;

  /* Check nsblock, the number of slabs blocked together
     (used for triggering and/or inversion recovery) */
  check_nsblock();
   
  /* Min TR ******************************************/   	
  trmin  = ss_grad.duration + te_delay + pe_grad.duration + ro_grad.duration + perTime;
  trmin += (sepSliceRephase) ? ssr_grad.duration : 0.0;   // Add slice refocusing if separate event
  trmin += 8e-6;

  /* Increase TR if any options are selected *********/
  if (sat[0] == 'y')  trmin += satTime;
  if (fsat[0] == 'y') trmin += fsatTime;
  if (mt[0] == 'y')   trmin += mtTime;
  if (ticks > 0) trmin += 4e-6;

  /* Adjust for all slabs ****************************/
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

  /* Set up frequency offset pulse shape list ********/   	
  offsetlist(pss,ss_grad.ssamp,0,freqEx,ns,seqcon[1]);
  shapeEx = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freqEx,ns,ss_grad.rfFraction,seqcon[1]);
  
  /* Set pe_steps for profile or full image **********/   	
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&null_grad);
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
    else if (skiptab[0] == 'y') g_setExpTime(trmean*ntmean*arraydim*nf + tr*ssc);
    else g_setExpTime(trmean*ntmean*pe_steps*pe2_steps*arraydim + tr*ssc);
  }

  /* PULSE SEQUENCE *************************************/
  status(A);
  rotate();
  triggerSelect(trigger);       // Select trigger input 1/2/3
  obsoffset(resto);
  delay(4e-6);
  initval(fabs(ssc),vssc);      // Compressed steady-state counter
  if (seqcon[2]=='s') assign(zero,vssc);
  assign(zero,vrfspoil_ctr);    // RF spoil phase counter
  assign(one,vacquire);         // real-time acquire flag
  setacqvar(vacquire);          // Turn on acquire when vacquire is zero 

  /* trigger */
  if (ticks > 0) F_initval((double)nsblock,vtrigblock);

  /* Begin phase-encode loop ****************************/       
  peloop2(seqcon[3],pe2_steps,vpe2_steps,vpe2_ctr);

    assign(zero,vtabskip);                       // initialize skip value to 0
    peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);
      sub(vpe_ctr,vtabskip,vpe_ctr);             // subtract previous end of loop value

      if (trtype) delay(ns*tr_delay);            // relaxation delay

      /* Compressed steady-states: 1st array & transient, all arrays if ssc is negative */
      if ((ix > 1) && (ssc > 0))
	assign(zero,vssc);
      sub(vpe_ctr,vssc,vpe_ctr);                 // vpe_ctr counts up from -ssc
      assign(zero,vssc);
      if (seqcon[2] == 's')
	assign(zero,vacquire);                   // Always acquire for non-compressed loop
      else {
	ifzero(vpe_ctr);
          assign(zero,vacquire);                 // Start acquiring when vpe_ctr reaches zero
	endif(vpe_ctr);
      }

      /* Skip table calculations for elliptical kspace *********/
      if (skiptab[0] == 'y') {
        mult(vpe2_ctr,vpe_steps,vtabindex);      // index to skip table is vpe2_ctr*nv + vpe_ctr
        add(vtabindex,vpe_ctr,vtabindex);
        getTabSkip(t3,vtabindex,vtabskip);       // get skip value vtabindex point in table
        add(vtabindex,vtabskip,vtabindex);       // advance skip index by skip value for next loop
        add(vpe_ctr,vtabskip,vpe_ctr);           // advance vpe_ctr by skip value
        divn(vpe_ctr,vpe_steps,vtemp1);          // if vpe_ctr exceeds vpe_steps, value to increment peloop2
        add(vpe2_ctr,vtemp1,vpe2_ctr);           // increment vpe2_ctr by integer number of vpe_steps blocks
        modn(vpe_ctr,vpe_steps,vpe_ctr);         // new vpe_ctr is remainder of vpe_ctr mod vpe_steps
      }

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
      sub(vpe2_ctr,vpe2_offset,vpe2_mult);

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

        sp1on(); delay(4e-6); sp1off(); // Scope trigger

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
        if (sepSliceRephase) {                       // separate slab refocus gradient
	  obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);
          delay(te_delay);                           // delay between slab refocus and pe
        }
	pe2_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,-pe2_offsetamp,
            -pe_grad.increment,-pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

	/* TE delay *******************************************/
        if (!sepSliceRephase)
	  delay(te_delay);                           // delay after refocus/pe

	/* Readout gradient and acquisition ********************/
	obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
	delay(ro_grad.atDelayFront);
	startacq(alfa);
	acquire(np,1.0/sw);
	delay(ro_grad.atDelayBack);
	endacq();

	/* Rewind / spoiler gradient *********************************/
	if (perewind[0] == 'y' || (spoilflag[0] == 'y')) {
          pe2_shapedgradient(perName,perTime,spoil_grad.amp,pespoil_amp,pespoil_amp,
            pe_grad.increment,pe2_grad.increment,vper_mult,vpe2r_mult,WAIT);
	}

      endmsloop(seqcon[1],vms_ctr);

      /* Skip table calculations for elliptical kspace *********/
      if (skiptab[0] == 'y') {
        incr(vtabindex);                         // increment index by one to look ahead
        getTabSkip(t3,vtabindex,vtabskip);       // get the number of loops to be skipped from table
        add(vpe_ctr,vtabskip,vpe_ctr);           // add to counter to see if it will exceed vpe_steps
        ifrtGE(vpe_ctr,vpe_steps,vtest);         // if table index goes beyond the end of this peloop cycle
          sub(vpe_steps,one,vpe_ctr);            // put it back to the end, let endpeloop finish things
        endif(vtest);
      }

    endpeloop(seqcon[2],vpe_ctr);
  endpeloop(seqcon[3],vpe2_ctr);

  /* Inter-image delay **********************************/
  sub(ntrt,ct,vtrimage);
  decr(vtrimage);
  ifzero(vtrimage);
    delay(trimage);
  endif(vtrimage);

  //calc_grad_duty(tr);
}                                                /**** End pulsesequence ****/

