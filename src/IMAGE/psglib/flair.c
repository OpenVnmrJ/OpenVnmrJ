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

pulsesequence() {
  /* Internal variable declarations *************************/
  int     shapelist90,shapelist180,shapelistIR;
  double  nseg;
  double  seqtime,tau1,tau2,tau3,
          te1_delay,te2_delay,te3_delay,
	  iti_delay, ti_delay,
	  tr_delay;
  double  kzero;
  double  freq90[MAXNSLICE], freq180[MAXNSLICE], freqIR[MAXNSLICE];

  /* Real-time variables used in this sequence **************/
  int  vpe_ctr    = v2;      // PE loop counter
  int  vpe_mult   = v3;      // PE multiplier, ranges from -PE/2 to PE/2
  int  vms_slices = v4;      // Number of slices
  int  vms_ctr    = v5;      // Slice loop counter
  int  vseg       = v6;      // Number of ETL segments 
  int  vseg_ctr   = v7;      // Segment counter
  int  vetl       = v8;      // Echo train length
  int  vetl_ctr   = v9;      // Echo train loop counter
  int  vssc       = v10;     // Compressed steady-states
  int  vtrimage   = v11;     // Counts down from nt, trimage delay when 0
  int  vacquire   = v12;     // Argument for setacqvar, to skip steady state acquires
  int  vphase180  = v13;     // phase of 180 degree refocusing pulse

  /* Initialize paramaters **********************************/
  init_mri();

  /*  Load external PE table ********************************/
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
  } else {
    abort_message("petable undefined");
  }
    
  seqtime = 0.0;
  espmin = 0.0;
  kzero = getval("kzero");

  /* RF Power & Bandwidth Calculations **********************/
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);
  init_rf(&p2_rf,p2pat,p2,flip2,rof1,rof2);
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");
 
  /* Initialize gradient structures *************************/
  init_readout_butterfly(&ro_grad,"ro",lro,np,sw,gcrushro,tcrushro);
  init_readout_refocus(&ror_grad,"ror");
  init_phase(&pe_grad,"pe",lpe,nv);
  init_slice(&ss_grad,"ss",thk);   /* NOTE assume same band widths for p1 and p2 */     
  init_slice_butterfly(&ss2_grad,"ss2",thk,gcrush,tcrush); 
  init_slice_refocus(&ssr_grad,"ssr");

  /* Gradient calculations **********************************/
  calc_readout(&ro_grad,WRITE,"gro","sw","at");
  calc_readout_refocus(&ror_grad,&ro_grad,NOWRITE,"gror");
  calc_phase(&pe_grad,WRITE,"gpe","tpe");
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice(&ss2_grad,&p1_rf,WRITE,"");
  calc_slice_refocus(&ssr_grad,&ss_grad,NOWRITE,"gssr");

  /* Equalize refocus and PE gradient durations *************/
  calc_sim_gradient(&ror_grad,&null_grad,&ssr_grad,0.0,WRITE);

  /* Create optional prepulse events ************************/
  if (sat[0] == 'y')  create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (mt[0] == 'y')   create_mtc();

  if (ir[0] == 'y') {
    init_rf(&ir_rf,pipat,pi,flipir,rof1,rof2);
    calc_rf(&ir_rf,"tpwri","tpwrif");
    init_slice_butterfly(&ssi_grad,"ssi",thk,gcrushir,tcrushir);
    calc_slice(&ssi_grad,&ir_rf,WRITE,"gssi");
  }

  /* Set up frequency offset pulse shape list ********/
  offsetlist(pss,ss_grad.ssamp, 0,freq90, ns,seqcon[1]);
  offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,seqcon[1]);
  offsetlist(pss,ssi_grad.ssamp,0,freqIR, ns,seqcon[1]);
  shapelist90  = shapelist(p1pat,ss_grad.rfDuration, freq90, ns,0,seqcon[1]);
  shapelist180 = shapelist(p2pat,ss2_grad.rfDuration,freq180,ns,0,seqcon[1]);
  shapelistIR  = shapelist(pipat,ssi_grad.rfDuration,freqIR, ns,0,seqcon[1]);

  /* same slice selection gradient and RF pattern used */
  if (ss_grad.rfFraction != 0.5)
    abort_message("ERROR %s: RF pulse must be symmetric (RF fraction = %.2f)",
      seqfil,ss_grad.rfFraction);
  if (ro_grad.echoFraction != 1)
    abort_message("ERROR %s: Echo Fraction must be 1",seqfil);

  /* Find sum of all events in each half-echo period ********/
  tau1 = ss_grad.rfCenterBack  + ssr_grad.duration + ss2_grad.rfCenterFront;
  tau2 = ss2_grad.rfCenterBack + pe_grad.duration  + ro_grad.timeToEcho; 
  tau3 = ro_grad.timeFromEcho  + pe_grad.duration  + ss2_grad.rfCenterFront;

  espmin = 2*MAX(MAX(tau1,tau2),tau3);   // Minimum echo spacing

  if (minesp[0] == 'y') {
    esp = espmin + 8e-6;  // ensure at least 4us delays in both TE periods
    putvalue("esp",esp);
  }
  else if (((espmin+8e-6)-esp) > 12.5e-9) {
    abort_message("ERROR %s: Echo spacing too small, minimum is %.2fms\n",seqfil,(espmin+8e-6)*1000);
  }
  te1_delay = esp/2.0 - tau1;    // Intra-esp delays
  te2_delay = esp/2.0 - tau2;
  te3_delay = esp/2.0 - tau3;

  te = kzero*esp;                // Return effective TE
  putvalue("te",te);

  /* Minimum TR **************************************/
  /* seqtime is total time per slice */
  seqtime = 2*4e-6 + ss_grad.rfCenterFront + etl*esp + ro_grad.timeFromEcho + pe_grad.duration + te3_delay;

  /* Increase TR if any options are selected****************/
  if (sat[0]  == 'y') seqtime += ns*satTime;
  if (fsat[0] == 'y') seqtime += ns*fsatTime;
  if (mt[0]   == 'y') seqtime += ns*mtTime;


  if (ir[0] == 'y') {

    /* Inter-IR delay */
    if (ns > 1) 
      iti_delay = seqtime - ssi_grad.duration;
      /* it is probably safe to assume that seqtime is always > the pulse widths */
    else 
      iti_delay = 0;

    /* Inversion Recovery */
    timin  = ssi_grad.rfCenterBack + ss_grad.rfCenterFront;
    timin += 8e-6; // from sp1on/off and after 90 pulse power setting 
    timin += seqtime*(ns-1) + iti_delay;

    if (ti < timin + 4e-6)  // ensure at least a 4us delay
      abort_message("%s: ti too short, minimum is %.2fms",seqfil,timin*1000);

    /* Delay after the last IR pulse */
    ti_delay = ti - timin;
    
    /* force all slices to be acquired back-to-back, with a single TR delay at end */
    trtype = 1;  

  }
  else {
    iti_delay = ti_delay = 0;
  }

  trmin = ns*(seqtime + 4e-6);
  
  if (ir[0] == 'y') {
    trmin += (4e-6 + ssi_grad.rfCenterFront + ti);
  }
  if (mintr[0] == 'y'){
    tr = trmin;
    putvalue("tr",tr);
  }


  if ((trmin-tr) > 12.5e-9) {
    abort_message("TR too short.  Minimum TR = %.2fms\n",trmin*1000);
  }
  tr_delay = (tr - trmin)/ns;



  /* Set number of segments for profile or full image **********/
  nseg = prep_profile(profile[0],nv/etl,&pe_grad,&per_grad);

  /* Shift DDR for pro *******************************/
  roff = -poffset(pro,ro_grad.roamp);

  /* Calculate total acquisition time */
  g_setExpTime(tr*(nt*nseg*getval("arraydim") + ssc) + trimage*getval("arraydim"));


  /* Return parameters to VnmrJ */
  putvalue("rgss",ss_grad.tramp);  //90  slice ramp
  if (ss2_grad.enableButterfly) {   //180 slice ramps
    putvalue("rcrush",ss2_grad.crusher1RampToCrusherDuration);
    putvalue("rgss2",ss2_grad.crusher1RampToSsDuration);
  }
  else {
    putvalue("rgss2",ss2_grad.tramp);
  }
  if (ro_grad.enableButterfly) {
    putvalue("rgro",ro_grad.crusher1RampToSsDuration);
  }
  else {   
    putvalue("rgro",ro_grad.tramp);      //RO ramp
  }
  putvalue("tror",ror_grad.duration);  //ROR duration
  putvalue("rgror",ror_grad.tramp);    //ROR ramp
  putvalue("gpe",pe_grad.peamp);         //PE max amp
  putvalue("gss",ss_grad.ssamp);
  putvalue("gro",ro_grad.roamp);



  /* PULSE SEQUENCE *************************************/
  initval(fabs(ssc),vssc);      // Compressed steady-state counter
  assign(one,vacquire);         // real-time acquire flag

  /* Phase cycle: Alternate 180 phase to cancel residual FID */
  mod2(ct,vphase180);           // 0101
  dbl(vphase180,vphase180);     // 0202
  add(vphase180,one,vphase180); // 1313 Phase difference from 90
  add(vphase180,oph,vphase180);

  obsoffset(resto);
  delay(4e-6);
    
  initval(nseg,vseg);
  loop(vseg,vseg_ctr);

    /* TTL scope trigger **********************************/       
    sp1on(); delay(4e-6); sp1off();

    /* Compressed steady-states: 1st array & transient, all arrays if ssc is negative */
    if ((ix > 1) && (ssc > 0))
      assign(zero,vssc);
    sub(vseg_ctr,vssc,vseg_ctr);   // vpe_ctr counts up from -ssc
    assign(zero,vssc);
    ifzero(vseg_ctr);
      assign(zero,vacquire);       // Start acquiring when vseg_ctr reaches zero
    endif(vseg_ctr);
    setacqvar(vacquire);           // Turn on acquire when vacquire is zero

    if (ticks) {
      xgate(ticks);
      grad_advance(gpropdelay);
      delay(4e-6);
    }

    if(ir[0] == 'y') {  /* IR for all slices prior to data acquisition */
      obspower(ir_rf.powerCoarse);
      obspwrf(ir_rf.powerFine);
      delay(4e-6);
      msloop(seqcon[1],ns,vms_slices,vms_ctr);
	obl_shapedgradient(ssi_grad.name,ssi_grad.duration,0,0,ssi_grad.amp,NOWAIT);   
	delay(ssi_grad.rfDelayFront);
	shapedpulselist(shapelistIR,ssi_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
	delay(ssi_grad.rfDelayBack);
	delay(iti_delay);
      endmsloop(seqcon[1],vms_ctr);
      delay(ti_delay);
    }

    msloop(seqcon[1],ns,vms_slices,vms_ctr);

      /* Prepulse options ***********************************/
      if (sat[0]  == 'y') satbands();
      if (fsat[0] == 'y') fatsat();
      if (mt[0]   == 'y') mtc();

      /* 90 degree pulse ************************************/         
      rotate();
      obspower(p1_rf.powerCoarse);
      obspwrf(p1_rf.powerFine);
      delay(4e-6);
      obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);   
      delay(ss_grad.rfDelayFront);
      shapedpulselist(shapelist90,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
      delay(ss_grad.rfDelayBack);

      /* Read dephase and Slice refocus *********************/
      obl_shapedgradient(ssr_grad.name,ssr_grad.duration,ror_grad.amp,0.0,-ssr_grad.amp,WAIT);

      /* First half-TE delay ********************************/
      obspower(p2_rf.powerCoarse);
      obspwrf(p2_rf.powerFine);
      delay(te1_delay);
	
      peloop(seqcon[2],etl,vetl,vetl_ctr);
        mult(vseg_ctr,vetl,vpe_ctr);
        add(vpe_ctr,vetl_ctr,vpe_ctr);
        getelem(t1,vpe_ctr,vpe_mult);

        /* 180 degree pulse *******************************/
        /* Note, ss2_grad.amp is max gradient for butterfly shape; flat top = _.ssamp */ 
        obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);   
    	delay(ss2_grad.rfDelayFront); 
        shapedpulselist(shapelist180,ss2_grad.rfDuration,vphase180,rof1,rof2,seqcon[1],vms_ctr);
        delay(ss2_grad.rfDelayBack);   

        /* Phase-encode gradient ******************************/
        pe_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,-pe_grad.increment,vpe_mult,WAIT);

        /* Second half-TE period ******************************/
	delay(te2_delay);
	 
        /* Readout gradient ************************************/
        obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
        delay(ro_grad.atDelayFront);

        /* Acquire data ****************************************/
        startacq(alfa);
        acquire(np,1.0/sw);
        endacq();

        delay(ro_grad.atDelayBack);

        /* Rewinding phase-encode gradient ********************/
        /* Phase encode, refocus, and dephase gradient ******************/
        pe_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,pe_grad.increment,vpe_mult,WAIT);

        /* Second half-TE delay *******************************/
        delay(te3_delay);
      endpeloop(seqcon[2],vetl_ctr);

      /* Relaxation delay ***********************************/
      if (!trtype)
        delay(tr_delay);
    endmsloop(seqcon[1],vms_ctr);
    if (trtype)
      delay(ns*tr_delay);
  endloop(vseg_ctr);

  /* Inter-image delay **********************************/
  sub(ntrt,ct,vtrimage);
  decr(vtrimage);
  ifzero(vtrimage);
    delay(trimage);
  endif(vtrimage);
}

