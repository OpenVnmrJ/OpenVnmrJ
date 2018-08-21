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
MPRAGE: 3D Gradient echo imaging sequence with segmented inversion recovery
************************************************************************/
#include <standard.h>
#include "sgl.c"

RF_PULSE_T ps_rf;
GENERIC_GRADIENT_T pscrush_grad;

pulsesequence()
{
  /* Internal variable declarations *************************/
  double  freqEx,freqIR;
  double  pe_steps,pespoil_amp,spoilMoment,maxgradtime,pe2_offsetamp;
  double  etltime,trseg,trseg_delay,tetime,trtime,irtime=0,te_delay,tr_delay,ti_delay=0,perTime;
  double  ps,flipps,tps,gcrushps,tcrushps,ps_delay=0.0,psTime=0.0,psmin;
  int     shapeEx,shapeIR=0,sepSliceRephase=0;
  int     i,j,index,sign,pelist[4096];
  char    spoilflag[MAXSTR],perName[MAXSTR],presat[MAXSTR],pspat[MAXSTR],peorder[MAXSTR],minps[MAXSTR],mintrseg[MAXSTR];

  /* Real-time variables used in this sequence **************/
  int  vpe_steps    = v1;      // Number of PE steps
  int  vpe_ctr      = v2;      // PE loop counter
  int  vpe_mult     = v3;      // PE multiplier, ranges from -PE/2 to PE/2
  int  vper_mult    = v4;      // PE rewinder multiplier; turn off rewinder when 0
  int  vpe2_steps   = v5;      // Number of PE2 steps
  int  vpe2_ctr     = v6;      // PE2 loop counter
  int  vpe2_mult    = v7;      // PE2 multiplier
  int  vpe2r_mult   = v8;      // PE2 rewinder multiplier
  int  vpe2_offset  = v9;      // PE2/2 for non-table offset
  int  vrfspoil_ctr = v10;     // RF spoil counter
  int  vrfspoil     = v11;     // RF spoil counter
  int  vseg         = v12;     // Number of ETL segments
  int  vseg_ctr     = v13;     // Segment counter
  int  vetl         = v14;     // Echo train length
  int  vetl_ctr     = v15;     // Echo train loop counter
  int  vssc         = v16;     // Compressed steady-states
  int  vacquire     = v17;     // Argument for setacqvar, to skip steady state acquires
  int  vtrimage     = v18;     // Counts down from nt, trimage delay when 0

  /* Initialize paramaters **********************************/
  init_mri();
  getstr("spoilflag",spoilflag);                                   
  trseg = getval("trseg");
  getstr("presat",presat);
  ps = getval("ps");     
  flipps = getval("flipps");   
  getstr("pspat",pspat);                                                             
  tps = getval("tps"); 
  gcrushps=getval("gcrushps");
  tcrushps=getval("tcrushps");
  getstr("peorder",peorder);   
  getstr("minps",minps);
  getstr("mintrseg",mintrseg);                                                                   
  
  /* Check some conditions *********************************/
  if (seqcon[2] != 'c')
    abort_message("mprage: First PE dimension must be compressed in seqcon");
  if (nseg*etl != nv)
    abort_message("mprage: nseg*etl must be equal to nv");
  nseg = nv/etl;
  if (nseg != rint(nseg))
    abort_message("mprage: nv must be divisible by etl and nseg");

  /*  Check for external PE table ***************************/
  if ((peorder[0] == 't') && strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    putCmd("pelist=0");                      // Zero pelist to allow use of petable
    text_message("mprage3d: using petable '%s'",petable);
  }
  else {
    if (peorder[0] == 'l') { // Construct linear kspace table, kzero=0
      for (i=0; i<nseg; i++) {                 // Loop through each echo train
        for (j=0; j<etl; j++) {
          index = i*etl + j;
          pelist[index] = -nv/2 + j*nseg + i;
        }
      }
    } else { // Construct centric kspace table, kzero=0
      for (i=0; i<nseg; i++) {                 // Loop through each echo train
        sign = (i < nseg/2) ? -1 : 1;          // Negative kspace values in second half of table
        for (j=0; j<etl; j++) {
          index = i*etl + j;
          sign = (etl == nv) ? -sign : sign;   // If etl=nv, alternate sign of table values
          if (etl == 1) {                      // Single kspace line per inversion
            pelist[index] = -nv/2 + index;
          } else if (etl == nv) {              // Handle this just in case
            pelist[index] = (1 - 2*(index % 2))*(index + 1)/2;
          } else {                             // etl kspace lines per inversion
            pelist[index] = -nseg/2 + i + sign*j*nseg/2;
          }
        }
      }
    }
    settable(t1,(int)nv,pelist);             // Init internal table

    /*  Write kspace table to pelist parameter ****************/
    putCmd("pelist=%d",pelist[0]);           // First pelist value resets the array
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
  init_slice(&ss_grad,"ss",thk);             // Slab select gradient
  init_slice_refocus(&ssr_grad,"ssr");       // Slab refocus gradient
  init_readout(&ro_grad,"ro",lro,np,sw);     // Read gradient
  init_readout_refocus(&ror_grad,"ror");     // Read dephase gradient
  init_phase(&pe_grad,"pe",lpe,nv);          // Phase encode gradient
  init_phase(&pe2_grad,"pe2",lpe2,nv2);      // 2nd phase encode gradient

  if (spoilflag[0] == 'y') {
    init_dephase(&spoil_grad,"spoil");       // Optimized spoiler
  }

  /* RF Calculations ****************************************/
  shape_rf(&p1_rf,"p1",p1pat,p1,flip1,rof1,rof2 ); // Excitation pulse
  calc_rf(&p1_rf,"tpwr1","tpwr1f");

  /* Gradient calculations **********************************/
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice_refocus(&ssr_grad,&ss_grad,WRITE,"gssr");
  calc_readout(&ro_grad,WRITE,"gro","sw","at");
  calc_readout_refocus(&ror_grad,&ro_grad,NOWRITE,"gror");
  calc_phase(&pe_grad,NOWRITE,"gpe","tpe");      // Rewinder uses same shape
  calc_phase(&pe2_grad,NOWRITE,"gpe2","");       // Rewinder uses same shape

  if (spoilflag[0] == 'y') {                     // Calculate spoil grad if spoiling is turned on
    spoilMoment = ro_grad.acqTime*ro_grad.roamp; // Optimal spoiling is at*gro for 2pi per pixel
    spoilMoment -= ro_grad.m0def;                // Subtract partial spoiling from back half of readout
    calc_dephase(&spoil_grad,WRITE,spoilMoment,"gspoil","tspoil");
  }

  /* Is TE long enough for separate slab refocus? ***********/
  maxgradtime = MAX(ror_grad.duration,MAX(pe_grad.duration,pe2_grad.duration));
  if (spoilflag[0] == 'y')
    maxgradtime = MAX(maxgradtime,spoil_grad.duration);
  tetime = ss_grad.rfCenterBack + ssr_grad.duration + maxgradtime + alfa + ro_grad.timeToEcho + 4e-6;
  if ((te >= tetime) && (minte[0] != 'y')) {
    sepSliceRephase = 1;                     // Set flag for separate slice rephase
  } else {
    pe2_grad.areaOffset = ss_grad.m0ref;     // Add slab refocus on pe2 axis
    calc_phase(&pe2_grad,NOWRITE,"gpe2",""); // Recalculate pe2 to include slab refocus
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
    if ((perewind[0] == 'y') && (spoilflag[0] == 'n')) {         // Rewinder, no spoiler
      strcpy(perName,pe_grad.name);
      perTime = pe_grad.duration;
      spoil_grad.amp = 0.0;
    } else if ((perewind[0] == 'n') && (spoilflag[0] == 'y')) {  // Spoiler, no rewinder
      strcpy(perName,spoil_grad.name);
      perTime = spoil_grad.duration;
      pespoil_amp = spoil_grad.amp;          // Apply spoiler on PE axis if no rewinder
    }
  }
  pe2_offsetamp = sepSliceRephase ? 0.0 : pe2_grad.offsetamp;    // pe2 slab refocus

  /* Create optional prepulse events ************************/
  if (sat[0]  == 'y') create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (mt[0] == 'y')   create_mtc();

  /* Min TE ******************************************/
  tetime = ss_grad.rfCenterBack + pe_grad.duration + alfa + ro_grad.timeToEcho;
  tetime += (sepSliceRephase) ? ssr_grad.duration : 0.0;   // Add slice refocusing if separate event

  temin = tetime + 4e-6;                     // Ensure that te_delay is at least 4us
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (te < temin) {
    abort_message("TE too short.  Minimum TE= %.2fms\n",temin*1000+0.005);   
  }
  te_delay = te - tetime;
   
  /* Min TR ******************************************/   	
  trtime  = ss_grad.duration + te_delay + pe_grad.duration + ro_grad.duration + perTime;
  trtime += (sepSliceRephase) ? ssr_grad.duration : 0.0;   // Add slice refocusing if separate event

  trmin = trtime + 4e-6;                     // Ensure that tr_delay is at least 4us
  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr);
  }
  if (tr < trmin) {
    abort_message("TR too short.  Minimum TR= %.2fms\n",trmin*1000);   
  }
  tr_delay = tr - trtime;                    // Delay between end/start of TR periods
  etltime = etl*tr;                          // Segment duration

  if (ir[0] == 'y') {
    shape_rf(&ir_rf,"pi",pipat,pi,flipir,rof1,rof1); 
    calc_rf(&ir_rf,"tpwri","tpwrif");
    init_slice_butterfly(&ssi_grad,"ssi",thk*thkirfact,gcrushir,tcrushir);
    calc_slice(&ssi_grad,&ir_rf,WRITE,"gssi");
    timin = ssi_grad.rfCenterBack + satTime + fsatTime + mtTime + ss_grad.rfCenterFront + 12e-6;
    if (peorder[0] == 'l')  {
      timin += etltime/2.0 ;
      etltime /= 2.0;
    }
    if (minti[0] == 'y') {
      ti = timin;
      putvalue("ti",ti);
    }
    if (ti < timin) {
      abort_message("TI too short.  Minimum TI = %.2fms\n",timin*1000+0.005);   
    }
    ti_delay = ti - timin + 4e-6;
    irTime = ti_delay + ssi_grad.rfCenterFront;
  }

  if (presat[0] == 'y') {
    shape_rf(&ps_rf,"ps",pspat,ps,flipps,rof1,rof1); 
    calc_rf(&ps_rf,"tpwrs","tpwrsf");
    init_generic(&pscrush_grad,"pscrush",gcrushps,tcrushps);
    calc_generic(&pscrush_grad,WRITE,"","");
    if (ir[0] == 'y') psmin = rof1 + pscrush_grad.duration + ssi_grad.rfCenterFront + 4e-6;
    else {
      psmin = rof1 + pscrush_grad.duration + ss_grad.rfCenterFront + 8e-6;
      if (peorder[0] == 'l')  {
        psmin += etltime/2.0 ;
        etltime /= 2.0;
      }
    }
    if (minps[0] == 'y') {
      tps = psmin;
      putvalue("tps",tps);
    }
    if (tps < psmin) {
      abort_message("Presat delay too short.  Minimum = %.2fms\n",psmin*1000+0.005);   
    }
    ps_delay = tps - psmin;    
    psTime = ps_delay + pscrush_grad.duration + ps_rf.rfDuration + 2*rof1;
  }

  sgl_error_check(sglerror);                 // Check for any SGL errors

  if (sat[0] == 'y')    etltime += satTime;  // Increase segment duration for prepulses 
  if (fsat[0] == 'y')   etltime += fsatTime;
  if (mt[0] == 'y')     etltime += mtTime;
  if (ir[0] == 'y')     etltime += irTime;
  if (presat[0] == 'y') etltime += psTime;

  if (mintrseg[0] == 'y') {
    trseg = etltime;
    putvalue("trseg",etltime);
  }
  if (trseg < etltime) {
    abort_message("Segment TR too short.  Minimum = %.2fms\n",etltime*1000+0.005);   
  }
  trseg_delay = trseg - etltime;

  /* Set up frequency offset pulse shape list ********/   	
  freqEx = poffset(pss[0],ss_grad.ssamp);    // Slab select frequency
  freqIR = poffset(pss[0],ssi_grad.ssamp);   // Inversion slab frequency
  
  /* Set segments/steps for profile or full image **********/
  nseg = prep_profile(profile[0],nv/etl,&pe_grad,&null_grad);    // segments for profile or image
  pe2_steps = prep_profile(profile[1],nv2,&pe2_grad,&null_grad); // pe2_steps for profile or image
  F_initval(pe2_steps/2.0,vpe2_offset);

  /* Shift DDR for pro *******************************/   	
  roff = -poffset(pro,ro_grad.roamp);

  g_setExpTime(trseg*(nt*nseg*pe2_steps*arraydim + ssc));

  /* PULSE SEQUENCE *************************************/
  status(A);
  rotate();                                  // Default (psi,phi,theta) Euler angles
  triggerSelect(trigger);                    // Select trigger input 1/2/3
  obsoffset(resto);                          // Set base spectrometer frequency
  delay(4e-6);
  initval(fabs(ssc),vssc);                   // Compressed steady-state counter
  if (seqcon[2]=='s') assign(zero,vssc);
  assign(zero,vrfspoil_ctr);                 // RF spoil phase counter
  assign(one,vacquire);                      // real-time acquire flag

  /* Begin PE2 outer loop *******************************/       
  peloop2(seqcon[3],pe2_steps,vpe2_steps,vpe2_ctr);

  /* Begin segmented PE inner loop **********************/
    peloop(seqcon[2],nseg,vseg,vseg_ctr);

      /* Compressed steady-states ***************************/
      if ((ix > 1) && (ssc > 0))               // Steady states only for first array and +ssc
        assign(zero,vssc);
      sub(vseg_ctr,vssc,vseg_ctr);             // vseg_ctr counts up from -ssc
      assign(zero,vssc);                       // Don't subtract again next time around the loop
      ifzero(vseg_ctr);
        assign(zero,vacquire);                 // Start acquiring when vseg_ctr reaches zero
      endif(vseg_ctr);
      setacqvar(vacquire);                     // Turn on acquire when vacquire is zero 

      /* Prepulse options ***********************************/       
      if (presat[0] == 'y') {
        obspower(ps_rf.powerCoarse);
        obspwrf(ps_rf.powerFine);
        delay(4e-6);
        shapedpulseoffset(ps_rf.pulseName,ps_rf.rfDuration,zero,rof1,rof1,0.0);
        obl_shapedgradient(pscrush_grad.name,pscrush_grad.duration,0,0,pscrush_grad.amp,WAIT);
        delay(ps_delay);
      }
      if (ir[0] == 'y') {
        obspower(ir_rf.powerCoarse);
        obspwrf(ir_rf.powerFine);
        delay(4e-6);
        obl_shapedgradient(ssi_grad.name,ssi_grad.duration,0,0,ssi_grad.amp,NOWAIT);
        delay(ssi_grad.rfDelayFront);
        shapedpulseoffset(ir_rf.pulseName,ssi_grad.rfDuration,zero,rof1,rof1,freqIR);
        delay(ssi_grad.rfDelayBack);
        delay(ti_delay);
      }
      if (sat[0]  == 'y') satbands();
      if (fsat[0] == 'y') fatsat();
      if (mt[0]   == 'y') mtc();

      /* Begin ETL loop for one segment *********************/
      F_initval(etl,vetl);
      loop(vetl,vetl_ctr);
        mult(vseg_ctr,vetl,vpe_ctr);
        add(vpe_ctr,vetl_ctr,vpe_ctr);
        getelem(t1,vpe_ctr,vpe_mult);          // Read kspace table for 1st PE dimension

        /* Set rcvr/xmtr phase for RF spoiling *******************/
        if (rfspoil[0] == 'y') {
          incr(vrfspoil_ctr);                  // vrfspoil_ctr = 1  2  3  4  5  6
          add(vrfspoil,vrfspoil_ctr,vrfspoil); // vrfspoil =     1  3  6 10 15 21
          xmtrphase(vrfspoil);
          rcvrphase(vrfspoil);
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

        if (ticks) {
          xgate(ticks);
          grad_advance(gpropdelay);
        }

        /* TTL scope trigger **********************************/       
        sp1on(); delay(4e-6); sp1off();

        /* Slice select RF pulse ******************************/ 
        obspower(p1_rf.powerCoarse);
        obspwrf(p1_rf.powerFine);
        delay(4e-6);
        obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
        delay(ss_grad.rfDelayFront);
        shapedpulseoffset(p1_rf.pulseName,ss_grad.rfDuration,oph,rof1,rof1,freqEx);
        delay(ss_grad.rfDelayBack);

        /* Phase encode, refocus, and dephase gradient ********/
        if (sepSliceRephase) {                 // separate slab refocus gradient
          obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);
          delay(te_delay);                     // delay between slab refocus and pe
        }
        pe2_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,-pe2_offsetamp,
          -pe_grad.increment,-pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

        /* TE delay *******************************************/
        if (!sepSliceRephase)
          delay(te_delay);                     // delay after refocus/pe

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

        delay(tr_delay);                       // Relaxation delay
      endloop(vetl_ctr);

      delay(trseg_delay);

    endpeloop(seqcon[2],vseg_ctr);

  endpeloop(seqcon[3],vpe2_ctr);

  /* Inter-image delay **********************************/
  sub(ntrt,ct,vtrimage);
  decr(vtrimage);
  ifzero(vtrimage);
    delay(trimage);
  endif(vtrimage);

}
