/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/***********************************************************************
Segmented Gradient echo imaging with interleaved Inversion Recovery
Version 1.0  ARR  2007
************************************************************************/
#include <standard.h>
#include "sgl.c"

void pulsesequence()
{
  /* Internal variable declarations *************************/
  double  freqEx[MAXNSLICE], freqIR[MAXNSLICE];
  double  pespoil_amp;
  double  perTime, seqtime, tau1, tauIR=0, te_delay, tr_delay, ti_delay=0,iti_delay=0;
  double  ninv;         // number of inversions/slices (up to ns) that fit within ti
  double  ninvseg;      // number of repetitions of ninv required for ns total slices
  double  ninvti;       // maximum # of inversion/slices that can fit in ti
  double  nseg;         // number of etl segments required to cover nv steps
  double  etltime;      // total length of one echo train of etl PE steps for one slice
  double  etldelay;     // delay between etl segments
  double  etlback;      // time from start of echo train to center of first acquire
  double  trinv;        // total recycle time between inversion/image segments for a slice
  double  timax;        // maximum value of ti that will fit within trinv for a single slice
  double  timin;        // minimum value of ti for a single slice
  double  titime;       // length of inversion pulse and crusher
  double  slicetime;    // time required for each additional slice
  double  invsegtime;   // time required for one multislice inversion/image segment
  double  etl_delay;    // delay between etl segments
  double  invseg_delay; // pad delay at end of last etl segment, before first slice inversion
  double  thkfact;      // factor to increase inversion slice thickness
  int     pelist[4096]; // array of kspace multipliers
  int     shapeEx, shapeIR=0;
  int     i,j,sign,index;
  char    spoilflag[MAXSTR],per_name[MAXSTR];

  /* Real-time variables used in this sequence **************/
  int  vpe_steps    = v1;      // Number of PE steps
  int  vpe_ctr      = v2;      // PE loop counter
  int  vms_slices   = v3;      // Number of slices
  int  vms_ctr      = v4;      // Slice loop counter
  int  vetl         = v5;      // Echo train length
  int  vetl_ctr     = v6;      // Echo train loop counter
  int  vinv         = v7;      // Number of inversions/slices per trseg period
  int  vinv_ctr     = v8;      // Inversions/slices loop counter
  int  vinvseg      = v9;      // Number of trseg periods to complete ns slices
  int  vinvseg_ctr  = v10;     // trseg loop counter
  int  vseg         = v11;     // Number of ETL segments
  int  vseg_ctr     = v12;     // Segment counter
  int  vpe_offset   = v13;     // PE/2 for non-table offset
  int  vpe_mult     = v14;     // PE multiplier, ranges from -PE/2 to PE/2
  int  vper_mult    = v15;     // PE rewinder multiplier; turn off rewinder when 0
  int  vssc         = v16;     // Compressed steady-states
  int  vacquire     = v17;     // Argument for setacqvar, to skip steady state acquires
  int  vrfspoil_ctr = v18;     // RF spoil counter
  int  vrfspoil     = v19;     // RF spoil multiplier
  int  vtrimage     = v20;     // Counts down from nt, trimage delay when 0
  int  vif          = v21;     // Scratch variable for ifrt
  int  vtemp        = v22;     // Spare variable for various uses

  /* Initialize paramaters **********************************/
  init_mri();
  getstr("spoilflag",spoilflag);
  trinv = getval("trinv");
  thkfact = getval("thkfact");

  /*  Check for external PE table ***************************/
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    putCmd("pelist=0");               // Zero pelist to allow use of petable
    text_message("gemsir: using petable '%s'",petable);
  }
  /*  Construct centric kspace table, kzero=0 ***************/
  else if (etl <= nv) {
    nseg = nv/etl;
    for (i=0; i<nseg; i++) {         // Loop through each echo train
      sign = (i < nseg/2) ? -1 : 1;  // Negative kspace values in second half of table
      for (j=0; j<etl; j++) {
        index = i*etl + j;
        sign = (etl == nv) ? -sign : sign;  // If etl=nv, alternate sign of table values
        if (etl == 1) {              // Single kspace line per inversion
          pelist[index] = -nv/2 + index;
        } else if (etl == nv) {      // Handle this just in case
          pelist[index] = (1 - 2*(index % 2))*(index + 1)/2;
        } else {                     // etl kspace lines per inversion
          if (etl > nv) abort_message("gemsir: etl cannot be greater than nv");
          pelist[index] = -nseg/2 + i + sign*j*nseg/2;
        }
      }
    }
    settable(t1,(int)nv,pelist);

    /*  Write kspace table to pelist **************************/
    putCmd("pelist=%d",pelist[0]);   // First pelist value zeros the array
    for (i=1; i<nv; i++) {
      putCmd("pelist[%d]=%d",i+1,pelist[i]);
    }
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
  init_phase(&per_grad,"per",lpe,nv);                // phase encode gradient
  init_generic(&spoil_grad,"spoil",gspoil,tspoil);   // spoiler gradient

  /* RF Calculations ****************************************/
  calc_rf(&p1_rf,"tpwr1","tpwr1f");

  /* Gradient calculations **********************************/
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice_refocus(&ssr_grad, &ss_grad, NOWRITE,"gssr");
  calc_readout(&ro_grad, WRITE, "gro","sw","at");
  calc_readout_refocus(&ror_grad, &ro_grad, NOWRITE, "gror");
  calc_phase(&pe_grad, NOWRITE, "gpe","tpe");

  /* Equalize refocus and PE gradient durations *************/
  calc_sim_gradient(&ror_grad, &pe_grad, &ssr_grad,tpemin, WRITE);

  /* Calculate phase-rewind & spoiler gradients *************/
  pespoil_amp = 0.0;
  perTime = 0.0;
  if ((perewind[0] == 'y') && (spoilflag[0] == 'n')) {       // Rewinder, no spoiler
    calc_phase(&per_grad,WRITE,"","");
    strcpy(per_name,per_grad.name);
    perTime = per_grad.duration;
    spoil_grad.amp = 0.0;
  }
  else if ((perewind[0] == 'n') && (spoilflag[0] == 'y')) {  // Spoiler, no rewinder
    calc_generic(&spoil_grad,WRITE,"","");
    strcpy(per_name,spoil_grad.name);
    perTime = spoil_grad.duration;
    pespoil_amp = spoil_grad.amp;      // Apply spoiler on PE axis if no rewinder
  }
  else if ((perewind[0] == 'y') && (spoilflag[0] == 'y')) {  // Rewinder and spoiler
    calc_phase(&per_grad,NOWRITE,"","");
    calc_generic(&spoil_grad,NOWRITE,"","");
    calc_sim_gradient(&per_grad,&spoil_grad,&null_grad,0.0,WRITE);
    strcpy(per_name,per_grad.name);
    perTime = per_grad.duration;
  }

  /* Create optional prepulse events ************************/
  if (sat[0] == 'y')  create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (ir[0] == 'y') {
    init_rf(&ir_rf,pipat,pi,flipir,rof2,rof2); 
    calc_rf(&ir_rf,"tpwri","tpwrif");
    init_slice_butterfly(&ssi_grad,"ssi",thk*thkfact,gcrushir,tcrushir);
    calc_slice(&ssi_grad,&ir_rf,WRITE,"gssi");
  }
  
  /* Check that all Gradient calculations are ok ************/
  sgl_error_check(sglerror);

  /* Min TE ******************************************/
  tau1 = ss_grad.rfCenterBack + pe_grad.duration + alfa + ro_grad.timeToEcho;
  temin = tau1 + 4e-6;  /* ensure that te_delay is at least 4us */
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (te < temin) {
    abort_message("TE too short.  Minimum TE= %.2fms\n",temin*1000+0.005);   
  }
  te_delay = te - tau1;
   
  /* Min TR ******************************************/   	
  seqtime  = ss_grad.duration + te_delay + pe_grad.duration
           + ro_grad.duration + perTime + alfa;

  trmin = seqtime + 4e-6;  /* ensure that tr_delay is at least 4us */
  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr);
  }
  if ((trmin-tr) > 12.5e-9) {
    abort_message("TR too short.  Minimum TR= %.2fms\n",trmin*1000);
  }
  tr_delay = tr - seqtime;

  titime = ssi_grad.duration;            // inversion event duration
  etltime = etl*tr - tr_delay;           // etl duration
  etlback = te + ss_grad.rfDelayFront;
  timax = trinv - titime - etltime + etlback;
  if (ti > timax)
    abort_message("ti %.1f msec exceeds maximum allowed %.1f for trinv %.1f",
        ti*1000,timax*1000,trinv*1000);
  timin = etlback;
  if (ti < timin)
    abort_message("ti %.1f msec less than minimum allowed %.1f for tr and etl",
        ti*1000,timin*1000);
  slicetime = (titime > etltime) ? titime : etltime;    // longest of inversion or etl segment
  ninvti = floor((ti - etlback - titime)/slicetime);   // #slices that can fit in ti
  ninv = (ninvti > ns) ? ns : ninvti;    // if more than ns can fit, set to ns
  ninv = (ninv < 1) ? 1 : ninv;          // At least one slice
  ninvseg = ceil(ns/ninv);    // # of repetitions of ninv to cover ns slices
  invsegtime = etltime - etlback + titime + ti + ninv*slicetime;
  if (invsegtime*ninvseg > trinv)
    abort_message("trinv %.1f less than minimum %.1f required for ti %.1f and ns %.0f",
        trinv*1000,invsegtime*ninvseg*1000+0.1,ti*1000,ns);
  ti_delay = ti - (ninv - 1)*slicetime - etlback;
  iti_delay = slicetime - titime;
  etl_delay = slicetime - etltime;
  invseg_delay = (trinv - ninvseg*invsegtime)/ninvseg;
  nseg = ceil(nv/etl);

  /* Increase segment duration if any options are selected **/
  if (sat[0] == 'y')  etltime += satTime;
  if (fsat[0] == 'y') etltime += fsatTime;
  if (mt[0] == 'y')   etltime += mtTime;

  /* Set up frequency offset pulse shape list ********/   	
  offsetlist(pss,ss_grad.ssamp,0,freqEx,ns,seqcon[1]);
  shapeEx = shapelist(p1pat,ss_grad.rfDuration,freqEx,ns,0,seqcon[1]);
  if (ir[0] == 'y') {
    offsetlist(pss,ssi_grad.ssamp,0,freqIR,ns,seqcon[1]);
    shapeIR = shapelist(pipat,ssi_grad.rfDuration,freqIR,ns,0,seqcon[1]);
  }

  /* Set number of segments for profile or full image **********/
  nseg = prep_profile(profile[0],nv/etl,&pe_grad,&per_grad);  // this is wrong

  g_setExpTime(trinv*(ssc + nseg*nt*arraydim));  // Set displayed exp time

  /* PULSE SEQUENCE *************************************/
  roff = -poffset(pro,ro_grad.roamp);  // Shift DDR for pro
  rotate();                     // Initialize default orientation
  obsoffset(resto);
  delay(4e-6);
  initval(ns,vms_slices);       // Number of total slices
  initval(fabs(ssc),vssc);      // Compressed steady-state counter
  assign(zero,vrfspoil_ctr);    // RF spoil phase counter
  assign(zero,vrfspoil);        // RF spoil multiplier
  assign(one,vacquire);         // real-time acquire flag
  setacqvar(vacquire);          // Turn on acquire when vacquire is zero 

  /* Begin PE segment loop ******************************/       
  initval(nseg,vseg);
  loop(vseg,vseg_ctr);          // begin PE segment loop for etl

    /* Compressed steady-states: 1st array & transient, all arrays if ssc is negative */
    if ((ix > 1) && (ssc > 0))
      assign(zero,vssc);
    sub(vseg_ctr,vssc,vseg_ctr);  // vseg_ctr counts up from -ssc
    assign(zero,vssc);
    if (seqcon[2] == 's')
      assign(zero,vacquire);    // Always acquire for non-compressed loop
    else {
      ifzero(vseg_ctr);
        assign(zero,vacquire);  // Start acquiring when vpe_ctr reaches zero
      endif(vseg_ctr);
    }

  /* Begin Inversion/image segment loop *****************/       
  assign(zero,vms_ctr);         // initialize multislice counter for each pass through invseg loop
  initval(ninvseg,vinvseg);     // initialize number of multislice groups
  loop(vinvseg,vinvseg_ctr);    // begin multislice group loop

    sp1on(); delay(4e-6); sp1off(); // TTL scope trigger

    /* Set rcvr/xmtr phase for RF spoiling *******************/
    if (rfspoil[0] == 'y') {
      incr(vrfspoil_ctr);                    // vrfspoil_ctr = 1  2  3  4  5  6
      add(vrfspoil,vrfspoil_ctr,vrfspoil);   // vrfspoil =     1  3  6 10 15 21
      xmtrphase(vrfspoil);
      rcvrphase(vrfspoil);
    }

    /* Get next kspace index from table *******************/       
    getelem(t1,vpe_ctr,vpe_mult);

    /* PE rewinder follows PE table; zero if turned off ***/       
    if (perewind[0] == 'y')
      assign(vpe_mult,vper_mult);
    else
      assign(zero,vper_mult);

    /* Begin inversion-recovery multislice segment loop ***********/       
    initval(ninv,vinv);
    if(ir[0] == 'y') {  /* IR for all slices prior to data acquisition */
      obspower(ir_rf.powerCoarse);
      obspwrf(ir_rf.powerFine);
      delay(4e-6);
      loop(vinv,vinv_ctr);
        obl_shapedgradient(ssi_grad.name,ssi_grad.duration,0,0,ssi_grad.amp,NOWAIT);
        delay(ssi_grad.rfDelayFront);
        shapedpulselist(shapeIR,ssi_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
        delay(ssi_grad.rfDelayBack);
        incr(vms_ctr);                    // Increment multislice counter by one
        ifrtEQ(vms_ctr,vms_slices,vif);   // if slice counter reaches total # slices, stop the loop
          sub(vms_ctr,vinv_ctr,vms_ctr);  // reset slice counter for imaging loop
          decr(vms_ctr);                  // vinv_ctr lags by one
          add(vms_ctr,vinv,vms_ctr);      // compensate for subtraction below
          sub(vinv,one,vinv_ctr);         // set loop counter to end value
        endif(vif);
        delay(iti_delay);                 // delay between multislice inversions
      endloop(vinv_ctr);
      sub(vms_ctr,vinv,vms_ctr);          // Subtract # inversions for imaging loop
      delay(ti_delay);
    }

    /* Begin imaging multislice segment loop **********************/       
    loop(vinv,vinv_ctr);
      if (ticks) {
        xgate(ticks);
        grad_advance(gpropdelay);         // Gradient propagation delay
        delay(4e-6);
      }

      /* Prepulse options ***********************************/       
      if (sat[0]  == 'y') satbands();
      if (fsat[0] == 'y') fatsat();
      if (mt[0]   == 'y') mtc();

      peloop(seqcon[2],etl,vetl,vetl_ctr);
        mult(vseg_ctr,vetl,vpe_ctr);
        add(vpe_ctr,vetl_ctr,vpe_ctr);
        getelem(t1,vpe_ctr,vpe_mult);

        /* Slice select RF pulse ******************************/ 
        obspower(p1_rf.powerCoarse);
        obspwrf(p1_rf.powerFine);
        delay(4e-6);
        obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
        delay(ss_grad.rfDelayFront);
        shapedpulselist(shapeEx,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
        delay(ss_grad.rfDelayBack);

        /* Phase encode, refocus, and dephase gradient ********/
        pe_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,-ssr_grad.amp,
            -pe_grad.increment,vpe_mult,WAIT);

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
        if ((perewind[0] == 'y') || (spoilflag[0] == 'y')) {
          pe_shapedgradient(per_name,perTime,spoil_grad.amp,pespoil_amp,spoil_grad.amp,
              per_grad.increment,vper_mult,WAIT);
        }

        add(vetl_ctr,one,vtemp);
        ifrtLT(vtemp,vetl,vif); delay(tr_delay); endif(vif);
      endpeloop(seqcon[2],vetl_ctr);

      incr(vms_ctr);                   // Increment multislice counter by one
      ifrtEQ(vms_ctr,vms_slices,vif);  // if slice counter reaches total # slices, stop the loop
        sub(vinv,one,vinv_ctr);
      elsenz(vif);
        delay(etl_delay);              // otherwise inter-etl delay
      endif(vif);
     endloop(vinv_ctr);                // end multislice group loop
     delay(invseg_delay);
   endloop(vinvseg_ctr);               // end multi-group loop
  endloop(vseg_ctr);                   // end PE segment loop

  /* Inter-image delay **********************************/
  sub(ntrt,ct,vtrimage);
  decr(vtrimage);
  ifzero(vtrimage);
    delay(trimage);
  endif(vtrimage);

}
