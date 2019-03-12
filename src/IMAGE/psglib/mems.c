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
Multi Echo Multi Slice Sequence
************************************************************************
     _                       __                                 __     _
    |                       |                                     |     |
RF  | ------ [90] ----------|-- [180] ------- |** ACQ **|---------|     |
    |                       |                                     |     |
    |          < - - - - - -|- - te - - - - - - - >               |     |
    |        ____           |   _____                             |     |
GSS | ______/////\     _____|__//////\____________________________|     |
    | <---->      \\\\/ gssr|                                     |     |
    | tr_delay              |                                     | ne  | ns
    |                       |                                     |     |
    |                       |            ___               ___    |     |
GPE |  _____________________|___________/   \_____________/   \___|     |
    |                       |           \___/             \___/   |     |
    |                       |                                     |     |
    |         gror ___      |                  _________          |     |
GRO |  ___________////\_____|_________________//////////\_________|     |
    |             <--> tror |                                     |     |
    |_                      |__                                 __|    _|

************************************************************************/

#include <standard.h>
#include "sgl.c"

/* Phase Cycle ********************************************/
/* If suppressSTE = 1 we will cycle phref and phrec to suppress STEs then FIDs */
/* If suppressSTE = 0 we will cycle phref and phrec to suppress FIDs */

/* This 2 step cycle will suppress FIDs */
/* 90x 180x AQ-x 180-x AQx 180x AQ-x 180-x AQx, 90x 180-x AQ-x 180x AQx 180-x AQ-x 180x AQ */
static int phref1[4] = {0,2,0,2};  // Odd refocusing pulses
static int phref2[4] = {2,0,2,0};  // Even refocusing pulses
static int phrec1[4] = {2,2,2,2};  // Odd acquisitions
static int phrec2[4] = {0,0,0,0};  // Even acqusitions

/* This 4 step cycle will suppress STEs then FIDs */
/* 90x 180x AQ-x 180-x AQx 180x AQ-x 180-x AQx, 90x 180y AQx 180-y AQx 180y AQx 180-y AQx
   90x 180-x AQ-x 180x AQx 180-x AQ-x 180x AQx, 90x 180-y AQx 180y AQx 180-y AQx 180y AQx */
static int phref1s[4] = {0,1,2,3};  // Odd refocusing pulses
static int phref2s[4] = {2,3,0,1};  // Even refocusing pulses
static int phrec1s[4] = {2,0,2,0};  // Odd acquisitions
static int phrec2s[4] = {0,0,0,0};  // Even acqusitions

void pulsesequence() {
  /* Internal variable declarations *************************/
  int     shapelist90,shapelist180;
  int     table = 0;
  double  tau1,tau2,tau3,te1_delay,te2_delay,te3_delay,tr_delay;
  double  freq90[MAXNSLICE],freq180[MAXNSLICE];
  double  thk2fact,crush_step,neby2,crush_ind;
  int     suppressSTE,*crushtab;
  char    crushmod[MAXSTR];
  int     i;

  /* Real-time variables used in this sequence **************/
  int  vpe_steps  = v1;    // Number of PE steps
  int  vpe_ctr    = v2;    // PE loop counter
  int  vpe_mult   = v3;    // PE multiplier, ranges from -PE/2 to PE/2
  int  vpe_offset = v4;    // PE/2 for non-table offset
  int  vms_slices = v5;    // Number of slices
  int  vms_ctr    = v6;    // Slice loop counter
  int  vne        = v7;    // Number of echoes
  int  vne_ctr    = v8;    // Echo loop counter
  int  vssc       = v9;    // Compressed steady-states
  int  vtrimage   = v10;   // Counts down from nt, trimage delay when 0
  int  vacquire   = v11;   // Argument for setacqvar, to skip steady state acquires
  int  vphase90   = v12;   // Phase of 90 degree excitation pulse
  int  vphase180  = v13;   // Phase of 180 degree refocusing pulse
  int  vphindex   = v14;   // Phase cycle index
  int  vneindex   = v15;   // Echo index, odd or even
  int  vcrush     = v16;   // Crusher modulation
  int  vtrigblock = v17;   // Number of slices per trigger block

  /* Initialize paramaters **********************************/
  init_mri();

  getstr("crushmod",crushmod);
  suppressSTE=getval("suppressSTE");

  /*  Load external PE table ********************************/
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    table = 1;
  }

  /* RF Power & Bandwidth Calculations **********************/
  shape_rf(&p1_rf,"p1",p1pat,p1,flip1,rof1,rof2);
  shape_rf(&p2_rf,"p2",p2pat,p2,flip2,rof1,rof2);
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");

  /* Calculate thk2fact to ensure gss=gss2 for the choice of p1 and p2 
     so that the sequence remains robust in the absence of correct
     balancing of slice select and slice refocus gradients */
  thk2fact=p2_rf.bandwidth/p1_rf.bandwidth;
  putvalue("thk2fact",thk2fact);
  
  /* Initialize gradient structures *************************/
  init_readout(&ro_grad,"ro",lro,np,sw); 
  ro_grad.pad1=alfa; ro_grad.pad2=alfa;
  init_readout_refocus(&ror_grad,"ror");
  init_phase(&pe_grad,"pe",lpe,nv);
  init_slice(&ss_grad,"ss",thk);
  init_slice(&ss2_grad,"ss2",thk*thk2fact);
  init_slice_refocus(&ssr_grad,"ssr");
  init_generic(&crush_grad,"crush",gcrush,tcrush);

  /* Gradient calculations **********************************/
  calc_readout(&ro_grad,WRITE,"gro","sw","at");
  calc_readout_refocus(&ror_grad,&ro_grad,NOWRITE,"gror");
  calc_phase(&pe_grad,WRITE,"gpe","tpe");
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice(&ss2_grad,&p2_rf,WRITE,"");
  calc_slice_refocus(&ssr_grad,&ss_grad,NOWRITE,"gssr");
  calc_generic(&crush_grad,WRITE,"","");

  /* Equalize slice refocus and PE gradient durations *******/
  calc_sim_gradient(&ror_grad,&null_grad,&ssr_grad,0.0,WRITE);

  /* Create optional prepulse events ************************/
  if (sat[0] == 'y')  create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (mt[0] == 'y')   create_mtc();
  if (ir[0] == 'y')   create_inversion_recovery();

  sgl_error_check(sglerror);

  /* Set up frequency offset pulse shape list ********/
  offsetlist(pss,ss_grad.amp,0,freq90,ns,seqcon[1]);
  offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,seqcon[1]);
  shapelist90 = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freq90,ns,ss_grad.rfFraction,seqcon[1]);
  shapelist180 = shapelist(p2_rf.pulseName,ss2_grad.rfDuration,freq180,ns,ss2_grad.rfFraction,seqcon[1]);

  /* To ensure proper overlap spin and stimulated echoes ensure that the
     middle of the refocusing RF pulse is the centre of the pulse and that
     echoes are formed in the centre of the acquisition window */
  if (ss2_grad.rfFraction != 0.5)
    abort_message("ERROR %s: Refocusing RF pulse must be symmetric (RF fraction = %.2f)",
      seqfil,ss2_grad.rfFraction);
  if (ro_grad.echoFraction != 1)
    abort_message("ERROR %s: Echo Fraction must be 1",seqfil);

  /* Find sum of all events in each half-echo period ********/
  tau1 = ss_grad.rfCenterBack  + ssr_grad.duration + crush_grad.duration + ss2_grad.rfCenterFront;
  tau2 = ss2_grad.rfCenterBack + pe_grad.duration  + crush_grad.duration + ro_grad.timeToEcho; 
  tau3 = ro_grad.timeFromEcho  + pe_grad.duration  + crush_grad.duration + ss2_grad.rfCenterFront;

  espmin  = 2*MAX(MAX(tau1,tau2),tau3);   // Minimum echo spacing
  espmin += 2*GRADIENT_RES; // Ensure that each delay is at least GRADIENT_RES

  te = granularity(te,2*GRADIENT_RES);
  if (minesp[0] == 'y') {
    te = espmin;
    putvalue("te",te);
  }
  if (FP_LT(te,espmin)) {
    abort_message("ERROR %s: Echo time too small, minimum is %.3fms\n",seqfil,espmin*1000);
  }
  te1_delay = te/2.0 - tau1;    // Intra-esp delays
  te2_delay = te/2.0 - tau2;
  te3_delay = te/2.0 - tau3;

  /* Now set the TE processing array accordingly */
  putCmd("TE = 0"); /* Re-initialize TE */
  for (i=0;i<ne;i++) putCmd("TE[%d] = %f",i+1,te*1000*(i+1));

  /* Check nsblock, the number of slices blocked together
     (used for triggering and/or inversion recovery) */
  check_nsblock();

  /* Minimum TR **************************************/
  trmin = ss_grad.rfCenterFront + ne*te + ro_grad.timeFromEcho + pe_grad.duration + te3_delay + 2*GRADIENT_RES;

  /* Increase TR if any options are selected *********/
  if (sat[0] == 'y')  trmin += satTime;
  if (fsat[0] == 'y') trmin += fsatTime;
  if (mt[0] == 'y')   trmin += mtTime;
  if (ticks > 0) trmin += GRADIENT_RES;

  /* Adjust for all slices ***************************/
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
    abort_message("ERROR %s: TR too short, minimum TR is %.3fms\n",seqfil,trmin*1000);
  }

  /* Calculate tr delay */
  tr_delay = granularity((tr-trmin)/ns,GRADIENT_RES);

  /* Set pe_steps for profile or full image **********/
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&per_grad);
  F_initval(pe_steps/2.0,vpe_offset);

  /* Shift DDR for pro ************************************/
  roff = -poffset(pro,ro_grad.roamp);

  /* Adjust experiment time for VnmrJ *********************/
  if (ssc<0) {
    if (seqcon[2] == 'c') g_setExpTime(trmean*(ntmean*pe_steps*arraydim - ssc*arraydim));
    else g_setExpTime(trmean*(ntmean*pe_steps*arraydim - ssc*pe_steps*arraydim));
  }
  else g_setExpTime(trmean*ntmean*pe_steps*arraydim + tr*ssc);

  /* Set phase cycle tables */
  if (suppressSTE) {
    if ((int)nt%2 == 1)
      abort_message("STE suppression requires a 2 step phase cycle.  Set nt as a multiple of 2\n");
    settable(t2,4,phref1s);
    settable(t3,4,phref2s);
    settable(t4,4,phrec1s);
    settable(t5,4,phrec2s);
  } else {
    settable(t2,4,phref1);
    settable(t3,4,phref2);
    settable(t4,4,phrec1);
    settable(t5,4,phrec2);
  }

  /* Set crusher table */
  crushtab=malloc((int)ne*sizeof(int));
  neby2=ceil(ne/2.0 - US); // US to handle precision errors
  crush_step=gcrush/neby2;
  for (i=0; i<ne; i++) {
    crush_ind = (1.0-2.0*(i%2))*(neby2-floor(i/2));
    crushtab[i] = (int)(crush_ind);
  }
  settable(t6,(int)ne,crushtab);

  /* PULSE SEQUENCE ***************************************/
  status(A);
  rotate();
  triggerSelect(trigger);       // Select trigger input 1/2/3
  obsoffset(resto);
  delay(GRADIENT_RES);
  initval(fabs(ssc),vssc);      // Compressed steady-state counter
  if (seqcon[2]=='s') assign(zero,vssc); // Zero for standard peloop
  assign(one,vacquire);         // real-time acquire flag
  setacqvar(vacquire);          // Turn on acquire when vacquire is zero

  /* Phase for excitation pulse */
  assign(zero,vphase90);

  /* trigger */
  if (ticks > 0) F_initval((double)nsblock,vtrigblock);
    
  /* Begin phase-encode loop ****************************/
  peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);

    if (trtype) delay(ns*tr_delay);   // relaxation delay

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

      /* Prepulse options ***********************************/
      if (ir[0] == 'y')   inversion_recovery();
      if (sat[0] == 'y')  satbands();
      if (fsat[0] == 'y') fatsat();
      if (mt[0] == 'y')   mtc();

      /* 90 degree pulse ************************************/         
      obspower(p1_rf.powerCoarse);
      obspwrf(p1_rf.powerFine);
      delay(GRADIENT_RES);
      obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);   
      delay(ss_grad.rfDelayFront);
      shapedpulselist(shapelist90,ss_grad.rfDuration,vphase90,rof1,rof2,seqcon[1],vms_ctr);
      delay(ss_grad.rfDelayBack);

      /* Slice refocus **************************************/
      obl_shapedgradient(ssr_grad.name,ssr_grad.duration,ror_grad.amp,0.0,-ssr_grad.amp,WAIT);

      /* First half-TE delay ********************************/
      obspower(p2_rf.powerCoarse);
      obspwrf(p2_rf.powerFine);
      delay(te1_delay);
	
      F_initval(ne,vne);
      loop(vne,vne_ctr);

        /* Phase cycle for refocusing pulse and receiver */
        mod4(ct,vphindex);
        mod2(vne_ctr,vneindex);
        ifzero(vneindex);
          getelem(t2,vphindex,vphase180);
          getelem(t4,vphindex,oph);
        elsenz(vneindex);
          getelem(t3,vphindex,vphase180);
          getelem(t5,vphindex,oph);
        endif(vneindex);

        /* Crusher gradient modulation */
        assign(one,vcrush);
        if (crushmod[0] == 'y') {
          assign(zero,vcrush);
          ifzero(vneindex);
            add(vcrush,one,vcrush);
          elsenz(vneindex);
            sub(vcrush,one,vcrush);
          endif(vneindex);
        }
        if (crushmod[0] == 'p') {
          getelem(t6,vne_ctr,vcrush);
          crush_grad.amp=crush_step;
        }

        /* 180 degree pulse *******************************/
        if (crushmod[0] == 'y' || crushmod[0] == 'p')
          var3_shapedgradient(crush_grad.name,crush_grad.duration,0.0,0.0,0.0,0.0,0.0,crush_grad.amp,zero,zero,vcrush,WAIT);
        else
          obl_shapedgradient(crush_grad.name,crush_grad.duration,crush_grad.amp,0,crush_grad.amp,WAIT);
        obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);   
        delay(ss2_grad.rfDelayFront);
        shapedpulselist(shapelist180,ss2_grad.rfDuration,vphase180,rof1,rof2,seqcon[1],vms_ctr);
        delay(ss2_grad.rfDelayBack);
        if (crushmod[0] == 'y' || crushmod[0] == 'p')
          var3_shapedgradient(crush_grad.name,crush_grad.duration,0.0,0.0,0.0,0.0,0.0,crush_grad.amp,zero,zero,vcrush,WAIT);
        else
          obl_shapedgradient(crush_grad.name,crush_grad.duration,crush_grad.amp,0,crush_grad.amp,WAIT);

        /* Second half-TE period ******************************/
	delay(te2_delay);
	 
        /* Phase-encode gradient ******************************/
        pe_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,-pe_grad.increment,vpe_mult,WAIT);

        /* Readout gradient ************************************/
        obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.roamp,0,0,NOWAIT);
        delay(ro_grad.atDelayFront-alfa);

        /* Acquire data ****************************************/
        startacq(alfa);
        acquire(np,1.0/sw);
        endacq();

        delay(ro_grad.atDelayBack);

        /* Rewinding phase-encode gradient ********************/
        pe_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,pe_grad.increment,vpe_mult,WAIT);

        /* Second half-TE delay *******************************/
        delay(te3_delay);

      endloop(vne_ctr);

    endmsloop(seqcon[1],vms_ctr);

  endpeloop(seqcon[2],vpe_ctr);

  /* Inter-image delay **********************************/
  sub(ntrt,ct,vtrimage);
  decr(vtrimage);
  ifzero(vtrimage);
    delay(trimage);
  endif(vtrimage);
}
