/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/***********************************************************************
Gradient-echo Look-Locker imaging sequence

[ nseg [ nslices [ nti [ etl ] ] ] ]

************************************************************************/

#include <standard.h>
#include "sgl.c"
int delaylist1;                                  // declared global to retain list number across arrays

void pulsesequence()
{
  /* Internal variable declarations *********************/
  double  freqEx[MAXNSLICE];
  double  maxgradtime,etltime,perTime,sat_delay,tetime,trtime,te_delay,tr_delay;
  double  spoilMoment;
  double  trseg,trseg_delay,trsegmin,tiarray[256],tidelay[256];
  int     i,j,index,nti,sign,table,shapeEx;
  int     pelist[4096];        // array of kspace multipliers
  char    mintrseg[MAXSTR];

  /* Real-time variables used in this sequence **********/
  int  vpe_steps    = v1;      // Number of PE steps
  int  vpe_ctr      = v2;      // PE loop counter
  int  vms_slices   = v3;      // Number of slices
  int  vms_ctr      = v4;      // Slice loop counter
  int  vpe_offset   = v5;      // PE/2 for non-table offset
  int  vpe_mult     = v6;      // PE multiplier, ranges from -PE/2 to PE/2
  int  vper_mult    = v7;      // PE rewinder multiplier; turn off rewinder when 0
  int  vssc         = v8;      // Compressed steady-states
  int  vacquire     = v9;      // Argument for setacqvar, to skip steady state acquires
  int  vrfspoil_ctr = v10;     // RF spoil counter
  int  vrfspoil     = v11;     // RF spoil multiplier
  int  vnti         = v12;     // Number of times in Look-Locker series
  int  vnti_ctr     = v13;     // Counter for Look-Locker loop
  int  vseg         = v14;     // Number of ETL segments
  int  vseg_ctr     = v15;     // Segment counter
  int  vetl         = v16;     // Number of PE steps in each segment
  int  vetl_ctr     = v17;     // ETL counter in peloop
  int  vtemp        = v18;     // Temporary variable
  int  vif          = v19;     // Logic variable

  /* Initialize paramaters ******************************/
  init_mri();
  trseg = getval("trseg");
  nti = getarray("ti",tiarray);
  getstr("mintrseg",mintrseg);

  /*  Check for external PE table ***************************/
  table = 0;
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    table = 1;
    putCmd("pelist=0");                          // Zero pelist to allow use of petable
    text_message("gemsll: using petable '%s'",petable);
  }
  /*  Construct centric kspace table, kzero=0 ***************/
  else if (etl <= nv) {
    nseg = nv/etl;
    for (i=0; i<nseg; i++) {                     // Loop through each echo train
      sign = (i < nseg/2) ? -1 : 1;              // Negative kspace values in second half of table
      for (j=0; j<etl; j++) {  
        index = i*etl + j;     
        sign = (etl == nv) ? -sign : sign;       // If etl=nv, alternate sign of table values
        if (etl == 1) {                          // Single kspace line per inversion
          pelist[index] = -nv/2 + index;
        } else if (etl == nv) {                  // Handle this just in case
          pelist[index] = (1 - 2*(index % 2))*(index + 1)/2;
        } else {                                 // etl kspace lines per inversion
          if (etl > nv) abort_message("gemsir: etl cannot be greater than nv");
          pelist[index] = -nseg/2 + i + sign*j*nseg/2;
        }
      }
    }
    settable(t1,(int)nv,pelist);
    
    /*  Write kspace table to pelist **************************/
    putCmd("pelist=%d",pelist[0]);               // First pelist value zeros the array
    for (i=1; i<nv; i++) {
      putCmd("pelist[%d]=%d",i+1,pelist[i]);
    }
  } 

  /* Set Rcvr/Xmtr phase increments for RF Spoiling *****/
  /* Ref:  Zur, Y., Magn. Res. Med., 21, 251, (1991) ****/
  if (rfspoil[0] == 'y') {
    rcvrstepsize(rfphase);
    obsstepsize(rfphase);
  }

  /* Initialize gradient structures *********************/
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2 );     // excitation pulse
  init_slice(&ss_grad,"ss",thk);                 // slice select gradient
  init_slice_refocus(&ssr_grad,"ssr");           // slice refocus gradient
  init_readout(&ro_grad,"ro",lro,np,sw);         // readout gradient
  init_readout_refocus(&ror_grad,"ror");         // dephase gradient
  init_phase(&pe_grad,"pe",lpe,nv);              // phase encode gradient
  init_dephase(&spoil_grad,"spoil");             // Optimized spoiler

  /* RF Calculations ************************************/
  calc_rf(&p1_rf,"tpwr1","tpwr1f");

  /* Gradient calculations ******************************/
  calc_readout(&ro_grad, WRITE, "gro","sw","at");
  calc_readout_refocus(&ror_grad, &ro_grad, NOWRITE, "gror");
  calc_phase(&pe_grad, NOWRITE, "gpe","tpe");
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice_refocus(&ssr_grad,&ss_grad,WRITE,"gssr");

  spoilMoment = ro_grad.acqTime*ro_grad.roamp;   // Optimal spoiling is at*gro for 2pi per pixel
  spoilMoment -= ro_grad.m0def;                  // Subtract partial spoiling from back half of readout
  calc_dephase(&spoil_grad,WRITE,spoilMoment,"gspoil","tspoil");

  /* Is TE long enough for separate slab refocus? *******/
  maxgradtime = MAX(ror_grad.duration,pe_grad.duration);
  if (spoilflag[0] == 'y')
    maxgradtime = MAX(maxgradtime,spoil_grad.duration);
  tetime = ss_grad.rfCenterBack + ssr_grad.duration + maxgradtime + alfa + ro_grad.timeToEcho + 4e-6;

  /* Equalize refocus and PE gradient durations *********/
  calc_sim_gradient(&ror_grad,&pe_grad,&ssr_grad,tpemin,WRITE);
  calc_sim_gradient(&ror_grad,&spoil_grad,&null_grad,tpemin,NOWRITE);

  perTime = 0.0;
  if ((perewind[0] == 'y') || (spoilflag[0] == 'y'))
    perTime = spoil_grad.duration;
  if (spoilflag[0] == 'n')
    spoil_grad.amp = 0.0;

  /* Create optional prepulse events ********************/
  if (sat[0] == 'y')  create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (mt[0] == 'y')   create_mtc();
  if (ir[0] == 'y')   create_inversion_recovery();
  
  /* Check that all Gradient calculations are ok ********/
  sgl_error_check(sglerror);

  /* Min TE *********************************************/
  tetime = ss_grad.rfCenterBack + pe_grad.duration + alfa + ro_grad.timeToEcho;

  temin = tetime + 4e-6;  /* ensure that te_delay is at least 4us */
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (te < temin) {
    abort_message("TE too short.  Minimum TE= %.2fms\n",temin*1000+0.005);   
  }
  te_delay = te - tetime;

  /* Min TR *********************************************/   	
  trmin  = ss_grad.duration + te_delay + pe_grad.duration + ro_grad.duration + perTime + 4e-6;

  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr);
  }
  if (FP_LT(tr,trmin)) {
    abort_message("TR too short.  Minimum TR = %.2fms\n",trmin*1000+0.005);
  }

  tr_delay = granularity((tr-trmin)/ns,GRADIENT_RES);  // Delay required to achieve TR

  /* Inversion recovery timing calculations *************/
  etltime = etl*tr;                              // duration of one etl segment
  tidelay[0] = tiarray[0];                       // start with first ti value
  for (i=1; i<nti; i++) {                        // compute deltas between successive ti values
    tidelay[i] = tiarray[i] - tiarray[i-1] - etltime;
    if (tidelay[i] < 0.0)
      abort_message("gemsll: IR times %5.3f and %5.3f are too closely spaced; alter times, etl, or tr",tiarray[i-1],tiarray[i]);
  }
  ti = tidelay[0];                               // ti is used to determine built-in delay inside inversion_recovery function
  tidelay[0] = 4e-6;                             // first ti interval is part of inversion_recovery or sat functions

  if (ix == 1) 
    delaylist1 = create_delay_list(tidelay,nti);

  /* Increase TR if any options are selected ************/
  if (sat[0] == 'y')  trmin += satTime;
  if (fsat[0] == 'y') trmin += fsatTime;
  if (mt[0] == 'y')   trmin += mtTime;
  if (ticks > 0) trmin += 4e-6;

  check_nsblock();

  /* Inversion recovery *********************************/
  if (ir[0] == 'y') {
    /* tiaddTime is the additional time beyond IR component to be included in ti */
    /* satTime, fsatTime and mtTime all included as those modules will be after IR */
    tiaddTime = satTime + fsatTime + mtTime + 4e-6 + ss_grad.rfCenterFront;
    /* calc_irTime checks ti and returns the time of all IR components */
    trtype = 1;
    calc_irTime(tiaddTime,trmin,mintr[0],tr,&trtype);
  }

  trsegmin = ns*(tiarray[nti-1] + etltime - ss_grad.rfCenterFront + ssi_grad.rfCenterFront + 4e-6);
  if (mintrseg[0] == 'y') {
    trseg = trsegmin;
    putvalue("trseg",trseg);
  }
  if (FP_LT(trseg,trsegmin)) {
    abort_message("TR (Inv) too short.  Minimum TR (Inv) = %.2fms\n",trsegmin*1000+0.005);
  }
  trseg_delay = (trseg - trsegmin)/ns + 4e-6;

  /* Set up frequency offset pulse shape list ***********/   	
  offsetlist(pss,ss_grad.ssamp,0,freqEx,ns,seqcon[1]);
  shapeEx = shapelist(p1pat,ss_grad.rfDuration,freqEx,ns,ss_grad.rfFraction,seqcon[1]);
  
  /* Set pe_steps for profile or full image *************/   	
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&per_grad);
  if (profile[0] == 'y') { nseg = 1; }
  F_initval(pe_steps/2.0,vpe_offset);

  /* Shift DDR for pro **********************************/   	
  roff = -poffset(pro,ro_grad.roamp);

  /* Adjust experiment time for VnmrJ *******************/
  if (seqcon[2] == 'c') g_setExpTime(trseg*(ssc + nseg*nt*arraydim));
  else g_setExpTime(trseg*(ssc + nseg*etl*nt*arraydim));

  /* PULSE SEQUENCE *************************************/
  status(A);
  rotate();                                      // Initialize default orientation
  obsoffset(resto);
  delay(4e-6);
  initval(fabs(ssc),vssc);                       // Compressed steady-state counter
  if (seqcon[2]=='s') assign(zero,vssc);         // Zero for standard peloop
  assign(zero,vrfspoil_ctr);                     // RF spoil phase counter
  assign(zero,vrfspoil);                         // RF spoil multiplier
  assign(one,vacquire);                          // real-time acquire flag
  setacqvar(vacquire);                           // Turn on acquire when vacquire is zero 

  /* Prepulse options ***********************************/
  if (sat[0] == 'y')  satbands();
  if (fsat[0] == 'y') fatsat();
  if (mt[0] == 'y')   mtc();

  /* Begin PE segment loop ******************************/
  peloop(seqcon[2],nseg,vseg,vseg_ctr);          // begin PE segment loop for etl

    /* Compressed steady-states: 1st array & transient, all arrays if ssc is negative */
    if ((ix > 1) && (ssc > 0))
      assign(zero,vssc);
    sub(vseg_ctr,vssc,vseg_ctr);                 // vseg_ctr counts up from -ssc
    assign(zero,vssc);
    if (seqcon[2] == 's')
      assign(zero,vacquire);                     // Always acquire for non-compressed loop
    else {
      ifzero(vseg_ctr);
        assign(zero,vacquire);                   // Start acquiring when vpe_ctr reaches zero
      endif(vseg_ctr);
    }

    /* Begin multislice loop ******************************/       
    msloop(seqcon[1],ns,vms_slices,vms_ctr);
      sp1on(); delay(4e-6); sp1off();            // Scope trigger
      if (ir[0] == 'y')   inversion_recovery();

      if (ir[0] == 's') {                        // Saturation option
        init_rf(&p3_rf,p3pat,p3,flip3,rof1,rof2 );
        calc_rf(&p3_rf,"tpwr3","tpwr3f");
        obspower(p3_rf.powerCoarse);
        obspwrf(p3_rf.powerFine);
        delay(4e-6);
        shapedpulse(p3pat,p3,oph,rof1,rof2);
        obl_shapedgradient(spoil_grad.name,4*spoil_grad.duration,5,5,5,WAIT);
        sat_delay = tiarray[0] - p3_rf.rfDuration - spoil_grad.duration - ss_grad.rfCenterFront;
        delay(sat_delay);
      }

      /* Begin inversion times loop *************************/
      initval(nti,vnti);
      loop(vnti,vnti_ctr);                       // loop over array of ti values
        vdelay_list(delaylist1,vnti_ctr);

        /* Begin ETL loop *************************************/
        F_initval(etl,vetl);
        loop(vetl,vetl_ctr);

          mult(vseg_ctr,vetl,vpe_ctr);
          add(vpe_ctr,vetl_ctr,vpe_ctr);
          getelem(t1,vpe_ctr,vpe_mult);

          /* PE rewinder follows PE table; zero if turned off ***/       
          if (perewind[0] == 'y') assign(vpe_mult,vper_mult);
          else assign(zero,vper_mult);

          /* Slice select RF pulse ******************************/ 
          obspower(p1_rf.powerCoarse);
          obspwrf(p1_rf.powerFine);
          delay(4e-6);
          obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
          delay(ss_grad.rfDelayFront);
          shapedpulselist(shapeEx,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
          delay(ss_grad.rfDelayBack);

          /* Phase encode, refocus, and dephase gradient ********/
          pe_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,-ssr_grad.amp,-pe_grad.increment,vpe_mult,WAIT);
          delay(te_delay);                       // delay after refocus/pe

          /* Readout gradient and acquisition *******************/
          obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
          delay(ro_grad.atDelayFront);
          startacq(alfa);
          acquire(np,1.0/sw);
          delay(ro_grad.atDelayBack);
          endacq();

          /* Rewind / spoiler gradient **************************/
          if ((perewind[0] == 'y') || (spoilflag[0] == 'y')) {
            pe_shapedgradient(pe_grad.name,pe_grad.duration,spoil_grad.amp,0,0,
              pe_grad.increment,vper_mult,WAIT);
          }

          add(vetl_ctr,one,vtemp);
          ifrtLT(vtemp,vetl,vif); delay(tr_delay); endif(vif);
        endloop(vetl_ctr);
      endloop(vnti_ctr);
      delay(trseg_delay);
    endmsloop(seqcon[1],vms_ctr);
  endpeloop(seqcon[2],vseg_ctr);

  //calc_grad_duty(tr);

}
