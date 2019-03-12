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

void pulsesequence()
{
  /* Internal variable declarations *************************/
  double  freq90[MAXNSLICE],freq180[MAXNSLICE],freqIR[MAXNSLICE];
  int     shape90=0, shape180=0, shapeIR=0;
  double  te_delay1, te_delay2, tr_delay, ti_delay = 0;
  double  del1=0, del2=0, del3=0, del4=0;
  double  tau1=0, tau2=0, difftime=0, tetime=0;
  int     table=0;

  /* Diffusion parameters */
#define MAXDIR 1024           /* Will anybody do more than 1024 directions or b-values? */
  double roarr[MAXDIR], pearr[MAXDIR], slarr[MAXDIR];
  int    nbval,               /* Total number of bvalues*directions */
         nbro, nbpe, nbsl,
	 i;    
  double bro[MAXDIR], bpe[MAXDIR], bsl[MAXDIR], /* b-values along RO, PE, SL */
         brs[MAXDIR], brp[MAXDIR], bsp[MAXDIR], /* and the cross-terms */
	 btrace[MAXDIR],                        /* and the trace */
	 max_bval=0,
         dcrush, dgss2,       /* "delta" for crusher and gss2 gradients */
         Dro, Dcrush, Dgss2;  /* "DELTA" for readout, crusher and gss2 gradients */

  /* Real-time variables ************************************/
  int  vpe_steps  = v1;
  int  vpe_ctr    = v2;
  int  vms_slices = v3;
  int  vms_ctr    = v4;
  int  vpe_offset = v5;
  int  vpe_index  = v6;
  int  vph180     = v7;  // Phase of 180 pulse
  int  vph2       = v8;  // alternate phase of 180 on odd transients

  /*  Initialize paramaters *********************************/
  init_mri();

  /*  Check for external PE table ***************************/
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    table = 1;
  }
  if ((diff[0] == 'y') && (gcrush < 4))
    warn_message("Advisory: set gcrush to higher value to avoid image artifacts");

  /* Initialize gradient structures *************************/
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);
  init_rf(&p2_rf,p2pat,p2,flip2,rof2,rof2);
  init_slice(&ss_grad,"ss",thk);
  init_slice_butterfly(&ss2_grad,"ss2",thk*1.1,gcrush,tcrush);
  init_slice_refocus(&ssr_grad,"ssr");
  init_readout_butterfly(&ro_grad,"ro",lro,np,sw,gcrushro,tcrushro);
  init_readout_refocus(&ror_grad,"ror");
  init_phase(&pe_grad,"pe",lpe,nv);

  /* RF Calculations ****************************************/
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");
  if (p2_rf.header.rfFraction != 0.5)
    abort_message("RF pulse for refocusing (%s) must be symmetric",p2pat);

  /* Gradient calculations **********************************/
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice(&ss2_grad,&p2_rf,WRITE,"gss2");
    
  calc_slice_refocus(&ssr_grad, &ss_grad, NOWRITE,"gssr");
  calc_readout(&ro_grad, WRITE, "gro","sw","at");
  ro_grad.m0ref *= grof;
  calc_readout_refocus(&ror_grad, &ro_grad, NOWRITE, "gror");

  calc_phase(&pe_grad, NOWRITE, "gpe","tpe");

  /* Equalize refocus and PE gradient durations *************/
  calc_sim_gradient(&ror_grad, &pe_grad, &ssr_grad,0,WRITE);

  /* Set up diffusion gradient */
  if (diff[0] == 'y') {
    init_generic(&diff_grad,"diff",gdiff,tdelta);
    calc_generic(&diff_grad,NOWRITE,"","");
    /* adjust duration, so tdelta is from start ramp up to start ramp down */    
    if (ix == 1) diff_grad.duration += diff_grad.tramp; 
    calc_generic(&diff_grad,WRITE,"","");
  }

  /* Min TE *************************************************/
  tau1 = ss_grad.rfCenterBack + pe_grad.duration + 4e-6 + ss2_grad.rfCenterFront;
  tau2 = ss2_grad.rfCenterBack + ror_grad.duration + ro_grad.timeToEcho + alfa;

  temin = 2*(MAX(tau1,tau2) + 2*4e-6);  /* have at least 4us between gradient events */

  /* Calculate te_delays with the current TE, then later see how diffusion fits */
  if ((minte[0] == 'y') || (te < temin)) {
    te_delay1 = temin/2 - tau1;
    te_delay2 = temin/2 - tau2;
  }
  else {
    te_delay1 = te/2 - tau1;
    te_delay2 = te/2 - tau2;
  }

  if (diff[0] =='y') {
    /* Is tDELTA long enough for RF refocusing gradient? */
    if (tDELTA < diff_grad.duration + ss2_grad.duration)
      abort_message("DELTA too short, increase to %.2fms",
        (diff_grad.duration + ss2_grad.duration)*1000+0.005);

    /* Is tDELTA too long for TE dead time? */
    difftime = tDELTA + diff_grad.duration;    // tDELTA + front & back half diff_grad
    tetime = ss2_grad.duration + te_delay1 + te_delay2;
    if (difftime > tetime) {
      temin += (difftime - tetime);
    }
  }

  /* We now know the minimum TE incl. diffusion */
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",ceil(te*1e6)*1e-6); /* round up to nearest us */
  }
  if (te < temin) {
    if (diff[0] == 'n') {
      abort_message("TE too short.  Minimum TE = %.2fms\n",temin*1000);   
    }
    else {
      abort_message("TE too short, increase to %.2fms or reduce DELTA to %.2fms",
        temin*1000,(tetime-diff_grad.duration)*1000);
    }
  }
  te_delay1 = te/2 - tau1;
  te_delay2 = te/2 - tau2;

  /* Set up delays around diffusion gradients */
  /* RF1 - del1 - diff - del2 - RF2 - del3 - diff - del4 - ACQ */
  if (diff[0] == 'y') {
    del1 = (tetime - difftime)/2;
    del4 = del1;
    del2 = te_delay1 - diff_grad.duration;
    del3 = te_delay2 - diff_grad.duration;

    if (del3 < 0.0) {             // shift diff block to right
      del1 += del3;
      del2 -= del3;
      del4 -= del3;
      del3 = 0;
    } else if (del2 < 0.0) {      // shift diff block to left
      del1 -= del2;
      del3 -= del2;
      del4 += del2;
      del2 = 0;
    }
  }
  else {  /* No diffusion */
    del1 = 0;
    del3 = 0;
    del2 = te_delay1;
    del4 = te_delay2;
  }

  /* Min TR *************************************************/   	
  trmin = (ss_grad.duration - ss_grad.rfCenterBack) + te + ro_grad.timeFromEcho;
  if (navigator[0] == 'y')
    trmin += (pe_grad.duration + ro_grad.duration);
  
  /* Optional prepulse calculations *************************/
  if (sat[0] == 'y') {
    create_satbands();
    trmin += satTime;
  }
  
  if (fsat[0] == 'y') {
    create_fatsat();
    trmin += fsatTime;
  }

  if (mt[0] == 'y') {
    create_mtc();
    trmin += mtTime;
  }

  if (ir[0] == 'y') {
    init_rf(&ir_rf,pipat,pi,flipir,rof2,rof2); 
    calc_rf(&ir_rf,"tpwri","tpwrif");
    init_slice_butterfly(&ssi_grad,"ssi",thk,gcrushir,tcrushir);
    calc_slice(&ssi_grad,&ir_rf,WRITE,"gssi");

    tau1 = ss_grad.duration - ss_grad.rfCenterBack; /* duration of ss_grad before RF center */
    ti_delay = ti - (ssi_grad.rfCenterBack + tau1);

    if (ti_delay < 0) {
      abort_message("TI too short, Minimum TI = %.2fms\n",(ti-ti_delay)*1000);
    }

    irTime = ti + ssi_grad.duration - ssi_grad.rfCenterBack;  /* time to add to TR */
    trmin += irTime;
    trmin -= tau1;  /* but subtract out ss_grad which was already included in TR */
  }

  trmin *= ns;
  if (mintr[0] == 'y'){
    tr = trmin + ns*4e-6;
    putvalue("tr",tr);
  }
  if (tr < trmin) {
    abort_message("TR too short.  Minimum TR= %.2fms\n",trmin*1000);   
  }
  tr_delay = (tr - trmin)/ns > 4e-6 ? (tr - trmin)/ns : 4e-6;


  /***************************************************/
  /* CALCULATE B VALUES ******************************/
  if (diff[0] == 'y') {
    /* Get multiplication factors and make sure they have same # elements */
    /* All this is only necessary because putCmd only work for ix==1      */
    nbro = (int) getarray("dro",roarr);  nbval = nbro;
    nbpe = (int) getarray("dpe",pearr);  if (nbpe > nbval) nbval = nbpe;
    nbsl = (int) getarray("dsl",slarr);  if (nbsl > nbval) nbval = nbsl;
    if ((nbro != nbval) && (nbro != 1))
      abort_message("%s: Number of directions/b-values must be the same for all axes (readout)",seqfil);
    if ((nbpe != nbval) && (nbpe != 1))
      abort_message("%s: Number of directions/b-values must be the same for all axes (phase)",seqfil);
    if ((nbsl != nbval) && (nbsl != 1))
      abort_message("%s: Number of directions/b-values must be the same for all axes (slice)",seqfil);


    if (nbro == 1) for (i = 1; i < nbval; i++) roarr[i] = roarr[0];
    if (nbpe == 1) for (i = 1; i < nbval; i++) pearr[i] = pearr[0];
    if (nbsl == 1) for (i = 1; i < nbval; i++) slarr[i] = slarr[0];

  }
  else {
    nbval = 1;
    roarr[0] = 0;
    pearr[0] = 0;
    slarr[0] = 0;
  }


  for (i = 0; i < nbval; i++)  {
    /* Readout */
    Dro     = ror_grad.duration;
    bro[i]  = bval(gdiff*roarr[i],tdelta,tDELTA);
    bro[i] += bval(ro_grad.amp,ro_grad.timeToEcho,Dro);

    /* Slice */
    dgss2   = Dgss2 = ss_grad.rfCenterFront;
    dcrush  = tcrush;                      //"delta" for crusher part of butterfly 
    Dcrush  = dcrush + ss_grad.rfDuration; //"DELTA" for crusher
    bsl[i]  = bval(gdiff*slarr[i],tdelta,tDELTA);
    bsl[i] += bval(gcrush,dcrush,Dcrush);
    bsl[i] += bval(ss2_grad.ssamp,dgss2,Dgss2);
    bsl[i] += bval_nested(gdiff*slarr[i],tdelta,tDELTA,gcrush,dcrush,Dcrush);
    bsl[i] += bval_nested(gdiff*slarr[i],tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
    bsl[i] += bval_nested(gcrush,dcrush,Dcrush,ss2_grad.ssamp,dgss2,Dgss2);

    /* Phase */
    bpe[i] = bval(gdiff*pearr[i],tdelta,tDELTA);

    /* Readout/Slice Cross-terms */
    brs[i]  = bval2(gdiff*roarr[i],gdiff*slarr[i],tdelta,tDELTA);
    brs[i] += bval_cross(gdiff*roarr[i],tdelta,tDELTA,gcrush,dcrush,Dcrush);
    brs[i] += bval_cross(gdiff*roarr[i],tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);

    /* Readout/Phase Cross-terms */
    brp[i]  = bval2(gdiff*roarr[i],gdiff*pearr[i],tdelta,tDELTA);

    /* Slice/Phase Cross-terms */
    bsp[i]  = bval2(gdiff*slarr[i],gdiff*pearr[i],tdelta,tDELTA);
    bsp[i] += bval_cross(gdiff*pearr[i],tdelta,tDELTA,gcrush,dcrush,Dcrush);
    bsp[i] += bval_cross(gdiff*pearr[i],tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);

    btrace[i] = (bro[i]+bsl[i]+bpe[i]);

    if (max_bval < btrace[i]) {
      max_bval = (bro[i]+bsl[i]+bpe[i]);
    }
  }  /* End for-all-directions */

  putarray("bvalrr",bro,nbval);
  putarray("bvalpp",bpe,nbval);
  putarray("bvalss",bsl,nbval);
  putarray("bvalrp",brp,nbval);
  putarray("bvalrs",brs,nbval);
  putarray("bvalsp",bsp,nbval);
  putarray("bvalue",btrace,nbval);
  putvalue("max_bval",max_bval);



  /* Generate phase-ramped pulses: 90, 180, and IR */
  offsetlist(pss,ss_grad.ssamp,0,freq90,ns,seqcon[1]);
  shape90 = shapelist(p1pat,ss_grad.rfDuration,freq90,ns,0,seqcon[1]);

  offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,seqcon[1]);
  shape180 = shapelist(p2pat,ss2_grad.rfDuration,freq180,ns,0,seqcon[1]);

  if (ir[0] == 'y') {
    offsetlist(pss,ssi_grad.ssamp,0,freqIR,ns,seqcon[1]);
    shapeIR = shapelist(pipat,ssi_grad.rfDuration,freqIR,ns,0,seqcon[1]);
  }

  /* Set pe_steps for profile or full image **********/   	
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&null_grad);
  initval(pe_steps/2.0,vpe_offset);

  sgl_error_check(sglerror);


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


  g_setExpTime(tr*(nt*pe_steps*arraydim + ssc));


  /* PULSE SEQUENCE *************************************/
  rotate();
  obsoffset(resto);
  roff = -poffset(pro,ro_grad.roamp);
  delay(4e-6);

  /* Begin phase-encode loop ****************************/       
  peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);

    /* Read external kspace table if set ******************/       
    if (table)
      getelem(t1,vpe_ctr,vpe_index);
    else
      sub(vpe_ctr,vpe_offset,vpe_index);

    settable(t2,2,ph180);        // initialize phase tables and variables
    getelem(t2,vpe_ctr,vph180);
    add(oph,vph180,vph180);      // 180 deg pulse phase alternates +/- 90 from receiver
    mod2(ct,vph2);
    dbl(vph2,vph2);
    add(vph180,vph2,vph180);     // Alternate phase for 180 on odd transients

    /* Begin multislice loop ******************************/       
    msloop(seqcon[1],ns,vms_slices,vms_ctr);
      if (ticks) {
        xgate(ticks);
        grad_advance(gpropdelay);
      }
      /* TTL scope trigger **********************************/       
       sp1on(); delay(5e-6); sp1off();

      /* Prepulses ******************************************/       
      if (sat[0]  == 'y') satbands();
      if (fsat[0] == 'y') fatsat();
      if (mt[0]   == 'y') mtc();

      /* Optional IR pulse **********************************/ 
      if (ir[0] == 'y') {
	obspower(ir_rf.powerCoarse);
	obspwrf(ir_rf.powerFine);
	delay(4e-6);
	obl_shapedgradient(ssi_grad.name,ssi_grad.duration,0,0,ssi_grad.amp,NOWAIT);
	delay(ssi_grad.rfDelayFront);
	shapedpulselist(shapeIR,ssi_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
	delay(ssi_grad.rfDelayBack);
	delay(ti_delay);
      }

      /* Slice select RF pulse ******************************/ 
      obspower(p1_rf.powerCoarse);
      obspwrf(p1_rf.powerFine);
      delay(4e-6);
      obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
      delay(ss_grad.rfDelayFront);
      shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
      delay(ss_grad.rfDelayBack);

      /* Phase encode, refocus, and dephase gradient ********/
      pe_shapedgradient(pe_grad.name,pe_grad.duration,0,0,-ssr_grad.amp,
          pe_grad.increment,vpe_index,WAIT);

      delay(del1);           // delay to start of first diffusion gradient
      if (diff[0] == 'y') {
        obl_shapedgradient(diff_grad.name,diff_grad.duration,
          diff_grad.amp*dro,diff_grad.amp*dpe,diff_grad.amp*dsl,WAIT);
      }
      delay(del2);           // delay from end of diffusion to slice refocusing

      /* Refocusing RF pulse ********************************/ 
      obspower(p2_rf.powerCoarse);
      obspwrf(p2_rf.powerFine);
      delay(4e-6);
      obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);
      delay(ss2_grad.rfDelayFront);
      shapedpulselist(shape180,ss2_grad.rfDuration,vph180,rof2,rof2,seqcon[1],vms_ctr);
      delay(ss2_grad.rfDelayBack);

      delay(del3);           // delay from slice refocusing to second diffusion gradient
      if (diff[0] == 'y') {
        obl_shapedgradient(diff_grad.name,diff_grad.duration,
          diff_grad.amp*dro,diff_grad.amp*dpe,diff_grad.amp*dsl,WAIT);
      }
      delay(del4);           // delay from end of diffusion gradient to readout event

      /* Readout gradient and acquisition ********************/
      roff = -poffset(pro,ro_grad.roamp);
      obl_shapedgradient(ror_grad.name,ror_grad.duration,-ror_grad.amp,0,0,WAIT);
      obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
      delay(ro_grad.atDelayFront);
      startacq(alfa);
      acquire(np,1.0/sw);
      delay(ro_grad.atDelayBack);
      endacq();

      /* Rewind Phase encoding ******************************/
      pe_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,
          pe_grad.increment,vpe_index,WAIT);

      if (navigator[0] == 'y') {
  roff = -poffset(pro,-ro_grad.roamp);
	obl_shapedgradient(ro_grad.name,ro_grad.duration,-ro_grad.amp,0,0,NOWAIT);
	delay(ro_grad.atDelayFront);
	startacq(alfa);
	acquire(np,1.0/sw);
	delay(ro_grad.atDelayBack);
	endacq();
      }

      /* Relaxation delay ***********************************/       
      delay(tr_delay);

    endmsloop(seqcon[1],vms_ctr);
  endpeloop(seqcon[2],vpe_ctr);
}
