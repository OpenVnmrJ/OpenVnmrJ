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
3D Fast spin echo imaging sequence
************************************************************************/

#include <standard.h>
#include "sgl.c"

SLICE_SELECT_GRADIENT_T  ss3_grad; /* 3rd slice select gradient */

void pulsesequence() {
  /* Internal variable declarations *********************/
  int    shapelist90,shapelist180,shapelistte=0;
  double kzero,thk2fact,thk3fact;
  double te1=0.0,te1_delay,te2_delay,te3_delay,tr_delay,te_delay1=0.0,te_delay2=0.0;
  double crushm0,pem0,gcrushr,gcrushp,gcrushs;
  double freq90[MAXNSLICE],freq180[MAXNSLICE],freqte[MAXNSLICE];
  char   autocrush[MAXSTR];

  /* Phase encode variables */
  FILE  *fp;
  int    tab[4096],petab[4096],odd,seg0,tabscheme;
  char   tabname[MAXSTR],tabfile[MAXSTR];
  int    i,j,k;

  /* Diffusion variables */
  double Gro,Gss;          // "gdiff" for readout/readout refocus and slice/slice refocus
  double dgro,Dgro;        // delta and DELTA for readout dephase & readout
  double dgss,Dgss;        // delta and DELTA for excitation ss
  double dgss3,Dgss3;      // delta and DELTA for spin echo prep ss
  double dcrush3,Dcrush3;  // delta and DELTA for spin echo prep crusher
  double dgss2,Dgss2;      // delta and DELTA for refocus ss
  double dcrush2,Dcrush2;  // delta and DELTA for refocus crusher

  /* Real-time variables used in this sequence **********/
  int  vpe_ctr     = v2;   // PE loop counter
  int  vpe_mult    = v3;   // PE multiplier, ranges from -PE/2 to PE/2
  int  vms_slices  = v4;   // Number of slices
  int  vms_ctr     = v5;   // Slice loop counter
  int  vseg        = v6;   // Number of ETL segments 
  int  vseg_ctr    = v7;   // Segment counter
  int  vetl        = v8;   // Echo train length
  int  vetl_ctr    = v9;   // Echo train loop counter
  int  vpe2_steps  = v10;  // Number of PE2 steps
  int  vpe2_ctr    = v11;  // PE2 loop counter
  int  vpe2_mult   = v12;  // PE2 multiplier
  int  vpe2_offset = v13;  // PE2/2 for non-table offset
  int  vssc        = v14;  // Compressed steady-states
  int  vtrimage    = v15;  // Counts down from nt, trimage delay when 0
  int  vacquire    = v16;  // Argument for setacqvar, to skip steady state acquires
  int  vphase180   = v17;  // phase of 180 degree refocusing pulse
  int  vtrigblock  = v18;  // Number of slices per trigger block

  /* Initialize paramaters ******************************/
  init_mri();

  kzero = getval("kzero");
  getstr("autocrush",autocrush);
  tabscheme = getval("tabscheme");
  getstr("spoilflag",spoilflag);

  /* Allow ROxPE2 projection ****************************/
  if (profile[0] == 'y' && profile[1] == 'n') {
    etl=1; kzero=1; nv=nv2;
  } else {
    /* Check kzero is valid *****************************/
    if (kzero<1) kzero=1; if (kzero>etl) kzero=etl;
    putCmd("kzero = %d",(int)kzero); 
  }

  /* Set petable name and full path *********************/
  sprintf(tabname,"fse%d_%d_%d",(int)nv,(int)etl,(int)kzero);
  putCmd("petable = '%s'",tabname);
  strcpy(tabfile,userdir);
  strcat(tabfile,"/tablib/");
  strcat(tabfile,tabname);

  /* Generate phase encode table ************************/
  if (tabscheme) { /* New scheme */
    /* Calculate PE table for kzero=1 */
    seg0=nseg/2;
    for (j=0;j<seg0;j++) {
      for (i=0;i<etl/2;i++) tab[j*(int)etl+i] = i*nseg+seg0-j;
      for (i=1;i<=etl/2;i++) tab[(j+1)*(int)etl-i] = tab[j*(int)etl]-i*nseg;
    }
    for (j=seg0;j<nseg;j++) {
      for (i=0;i<=etl/2;i++) tab[j*(int)etl+i] = i*nseg+seg0-j;
      for (i=1;i<etl/2;i++) tab[(j+1)*(int)etl-i] = tab[j*(int)etl]-i*nseg;
    }
    /* Adjust for kzero */
    for (i=0;i<nseg;i++) { 
      k=i*etl;
      for (j=0;j<kzero-1;j++) petab[k+j]=tab[k+(int)etl-(int)kzero+j+1];
      for (j=kzero-1;j<etl;j++) petab[k+j]=tab[k+j-(int)kzero+1];
    }
  } else { /* Original scheme */
    /* Calculate PE table for kzero=1 */
    odd=(int)nseg%2; seg0=nseg/2+odd;
    k=0; for (i=0;i<etl;i++) for (j=seg0-odd*i%2-1;j>=0;j--) tab[j*(int)etl+i] = k--;
    k=1; for (i=0;i<etl;i++) for (j=seg0-odd*i%2;j<nseg;j++) tab[j*(int)etl+i] = k++;
    /* Adjust for kzero */
    for (i=0;i<nseg;i++) { 
      k=i*etl;
      for (j=0;j<kzero-1;j++) petab[k+j]=tab[k+(int)etl-j-1];
      for (j=kzero-1;j<etl;j++) petab[k+j]=tab[k+j-(int)kzero+1];
    }
  }
  /* Set petable name and full path *********************/
  sprintf(tabname,"fse%d_%d_%d",(int)nv,(int)etl,(int)kzero);
  putCmd("petable = '%s'",tabname);
  strcpy(tabfile,userdir);
  strcat(tabfile,"/tablib/");
  strcat(tabfile,tabname);
  /* Write to tabfile ***********************************/
  fp=fopen(tabfile,"w");
  fprintf(fp,"t1 =");
  for (i=0;i<nseg;i++) { 
    fprintf(fp,"\n");
    for (j=0;j<etl;j++) fprintf(fp,"%3d\t",petab[i*(int)etl+j]);
  }
  fclose(fp);

  /* Set pelist to contain table order ******************/
  putCmd("pelist = 0"); /* Re-initialize pelist */
  for (i=0;i<nseg*etl;i++) putCmd("pelist[%d] = %d",i+1,petab[i]);

  /* Avoid gradient slew rate errors */
  for (i=0;i<nseg*etl;i++) if (abs(petab[i]) > nv/2) petab[i]=0;

  /* Set phase encode table *****************************/
  settable(t1,(int)etl*nseg,petab);

  /* RF Power & Bandwidth Calculations ******************/
  shape_rf(&p1_rf,"p1",p1pat,p1,flip1,rof1,rof2);
  shape_rf(&p2_rf,"p2",p2pat,p2,flip2,rof1,rof2);
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");

  /* Calculate thk2fact to ensure gss=gss2 for the choice of p1 and p2 
     so that the sequence remains robust in the absence of correct
     balancing of slice select and slice refocus gradients */
  thk2fact=p2_rf.bandwidth/p1_rf.bandwidth;
  putvalue("thk2fact",thk2fact);
  
  /* Initialize gradient structures *********************/
  init_readout(&ro_grad,"ro",lro,np,sw);
  ro_grad.pad1=alfa; ro_grad.pad2=alfa;
  init_readout_refocus(&ror_grad,"ror");
  init_phase(&pe_grad,"pe",lpe,nv);
  init_phase(&pe2_grad,"pe2",lpe2,nv2);
  init_slice(&ss_grad,"ss",thk);
  init_slice(&ss2_grad,"ss2",thk*thk2fact);
  init_slice_refocus(&ssr_grad,"ssr");
  init_dephase(&crush_grad,"crush");

  /* Gradient calculations ******************************/
  calc_readout(&ro_grad,WRITE,"gro","sw","at");
  calc_readout_refocus(&ror_grad,&ro_grad,WRITE,"gror");
  calc_phase(&pe_grad,NOWRITE,"","");
  calc_phase(&pe2_grad,NOWRITE,"","");
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice(&ss2_grad,&p2_rf,WRITE,"");
  calc_slice_refocus(&ssr_grad,&ss_grad,WRITE,"gssr");

  /* Equalize refocus gradient durations ****************/
  calc_sim_gradient(&ror_grad,&null_grad,&ssr_grad,0.0,WRITE);

  /* Equalize PE gradient durations *********************/
  calc_sim_gradient(&pe_grad,&pe2_grad,&null_grad,0.0,WRITE);

  /* Set crushing gradient moment ***********************/
  crushm0=fabs(gcrush*tcrush);

  if (spoilflag[0] == 'y') {
    init_generic(&spoil_grad,"spoil",gspoil,tspoil);
    calc_generic(&spoil_grad,WRITE,"gspoil","tspoil");
  }

  /* Create optional prepulse events ********************/
  if (sat[0] == 'y')  create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (mt[0] == 'y')   create_mtc();
  if (ir[0] == 'y')   create_inversion_recovery();
  if (diff[0] == 'y') init_diffusion(&diffusion,&diff_grad,"diff",gdiff,tdelta);

  if (diff[0] == 'y') { /* Diffusion encoding is during spin echo preparation */
    spinecho[0]='y';
    putCmd("spinecho='y'");
  }

  if (spinecho[0] == 'y') { /* spin echo preparation */
    shape_rf(&p3_rf,"p3",p3pat,p3,flip3,rof1,rof2);
    calc_rf(&p3_rf,"tpwr3","tpwr3f");
    /* Calculate thk3fact to ensure gss=gss2=gss3 for the choice of  
       p1, p2 and p3 so that the sequence remains robust in the absence
       of correct balancing of slice select and slice refocus gradients */
    thk3fact=p3_rf.bandwidth/p1_rf.bandwidth;
    putvalue("thk3fact",thk3fact);
    init_slice(&ss3_grad,"ss3",thk*thk3fact);
    calc_slice(&ss3_grad,&p3_rf,WRITE,"");
    putvalue("gss3",ss3_grad.ssamp);
    offsetlist(pss,ss3_grad.ssamp,0,freqte,ns,seqcon[1]);
    shapelistte = shapelist(p3_rf.pulseName,ss3_grad.rfDuration,freqte,ns,ss3_grad.rfFraction,seqcon[1]);
    /* Automatically set crushers to avoid unwanted echoes */
    if (autocrush[0] == 'y') {
      if (crushm0 < 0.6*ro_grad.m0) crushm0=0.6*ro_grad.m0;
    }
  }

  /* Make sure crushing in PE dimensions does not refocus signal from 180 */
  pem0 = (pe_grad.m0 > pe2_grad.m0) ? pe_grad.m0 : pe2_grad.m0;
  calc_dephase(&crush_grad,WRITE,crushm0+pem0,"","");
  gcrushr = crush_grad.amp*crushm0/crush_grad.m0;
  gcrushp = crush_grad.amp*(crushm0+pe_grad.m0)/crush_grad.m0;
  gcrushs = crush_grad.amp*(crushm0+pe2_grad.m0)/crush_grad.m0;

  sgl_error_check(sglerror);

  /* Set up frequency offset pulse shape list ***********/
  offsetlist(pss,ss_grad.amp,0,freq90,ns,seqcon[1]);
  offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,seqcon[1]);
  shapelist90 = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freq90,ns,ss_grad.rfFraction,seqcon[1]);
  shapelist180 = shapelist(p2_rf.pulseName,ss2_grad.rfDuration,freq180,ns,ss2_grad.rfFraction,seqcon[1]);

  /* To ensure proper overlap spin and stimulated echoes ensure that the
     middle of the refocusing RF pulse is the centre of the pulse and that
     echoes are formed in the centre of the acquisition window */
  if (ss2_grad.rfFraction != 0.5)
    abort_message(
      "ERROR %s: Refocusing RF pulse must be symmetric (RF fraction = %.2f)",
      seqfil,ss2_grad.rfFraction);
  if (ro_grad.echoFraction != 1)
    abort_message("ERROR %s: Echo Fraction must be 1",seqfil);

  /* Find sum of all events in each half-echo period ****/
  esp = granularity(esp,2*GRADIENT_RES);
  tau1 = ss_grad.rfCenterBack + ssr_grad.duration + crush_grad.duration + ss2_grad.rfCenterFront + GRADIENT_RES;
  tau2 = ss2_grad.rfCenterBack + crush_grad.duration + pe_grad.duration + ro_grad.timeToEcho + GRADIENT_RES; 
  tau3 = ro_grad.timeFromEcho + pe_grad.duration + crush_grad.duration + ss2_grad.rfCenterFront + GRADIENT_RES;
  espmin  = 2*MAX(MAX(tau1,tau2),tau3);       // Minimum echo spacing

  if (minesp[0] == 'y') {
    esp = espmin;
    putvalue("esp",esp);
  }
  if (FP_LT(esp,espmin)) {
    abort_message("ERROR %s: Echo spacing too small, minimum is %.3fms\n",seqfil,espmin*1000);
  }
  te1_delay = esp/2.0 - tau1 + GRADIENT_RES;  // Intra-esp delays
  te2_delay = esp/2.0 - tau2 + GRADIENT_RES;
  te3_delay = esp/2.0 - tau3 + GRADIENT_RES;

  /* Spin echo preparation ******************************/
  if (spinecho[0] == 'y') {
    te = granularity(te,2*GRADIENT_RES);
    te1 = te-kzero*esp;
    tau1 = ss_grad.rfCenterBack + ssr_grad.duration + crush_grad.duration + ss3_grad.duration/2.0 + GRADIENT_RES;
    tau2 = ss3_grad.duration/2.0 + crush_grad.duration + GRADIENT_RES;
    temin = 2*MAX(tau1,tau2);
    /* Diffusion */
    if (diff[0] == 'y') {
      /* granulate tDELTA */
      tDELTA = granularity(tDELTA,GRADIENT_RES);
      /* taudiff is the duration of events between diffusion gradients */
      taudiff = ss3_grad.duration + 2*crush_grad.duration;
      /* set minimum diffusion structure requirements for gradient echo: taudiff, tDELTA, te and minte[0] */
      set_diffusion(&diffusion,taudiff,tDELTA,te1,minte[0]);
      /* set additional diffusion structure requirements for spin echo: tau1 and tau2 */
      set_diffusion_se(&diffusion,tau1,tau2);
      /* calculate the diffusion structure delays.
         address &temin is required in order to update temin accordingly */
      calc_diffTime(&diffusion,&temin);
    }
    /* TE delays */
    if (minte[0] == 'y') {
      te1 = temin;
      te = te1+kzero*esp;
      putvalue("te",te);
    }
    te_delay1 = te1/2 - tau1 + GRADIENT_RES;
    te_delay2 = te1/2 - tau2 + GRADIENT_RES;
    if (FP_LT(te,temin+kzero*esp)) {
      abort_message("ERROR %s: TE too short, minimum TE = %.3f ms\n",seqfil,temin*1000);
    }
  }

  else
    putvalue("te",kzero*esp);       // Return effective TE

  /* Check nsblock, the number of slices blocked together
     (used for triggering and/or inversion recovery) */
  check_nsblock();

  /* Calculate B values *********************************/
  if (ix==1) {
    /* Calculate bvalues according to main diffusion gradients */
    calc_bvalues(&diffusion,"dro","dpe","dsl");
    /* Add components from additional diffusion encoding imaging gradients peculiar to this sequence */
    /* Initialize variables */
    dgro = 0.5*(ror_grad.duration+ro_grad.timeToEcho);        // readout dephase & readout delta
    Gro = ro_grad.m0ref/dgro;                                 // readout dephase & readout gradient strength
    Dgro = dgro+2*crush_grad.duration+ss2_grad.duration+te2_delay+pe_grad.duration; // readout dephase & readout DELTA
    dgss = 0.5*(ss_grad.rfCenterBack+ssr_grad.duration);      // slice & slice refocus delta
    Gss = ss_grad.m0ref/dgss;                                 // slice & slice refocus gradient strength
    Dgss = dgss;                                              // slice & slice refocus DELTA
    dgss2 = (ss2_grad.duration-ss2_grad.tramp)/2.0;           // refocus slice select delta
    Dgss2 = dgss2;                                            // refocus slice select DELTA
    dcrush2 = crush_grad.duration-crush_grad.tramp;           // refocus crusher delta
    Dcrush2 = crush_grad.duration+ss2_grad.duration;          // refocus crusher DELTA
    dcrush3 = crush_grad.duration-crush_grad.tramp;           // spin echo prep crusher delta
    Dcrush3 = crush_grad.duration+ss3_grad.duration;          // spin echo prep crusher DELTA
    dgss3 = (ss3_grad.duration-ss3_grad.tramp)/2.0;           // spin echo prep slice select delta
    Dgss3 = dgss3;                                            // spin echo prep slice select DELTA
    for (i = 0; i < diffusion.nbval; i++)  {
      /* set droval, dpeval and dslval */
      set_dvalues(&diffusion,&droval,&dpeval,&dslval,i);
      /* Readout */
      diffusion.bro[i] += bval(Gro,dgro,Dgro);
      diffusion.bro[i] += bval(gcrushr,dcrush2,Dcrush2);
      diffusion.bro[i] += bval_nested(Gro,dgro,Dgro,gcrushr,dcrush2,Dcrush2);
      /* Phase */
      diffusion.bpe[i] += bval(gcrushp,dcrush2,Dcrush2);
      /* Slice */
      diffusion.bsl[i] += bval(Gss,dgss,Dgss);
      diffusion.bsl[i] += bval(gcrushs,dcrush2,Dcrush2);
      diffusion.bsl[i] += bval(ss2_grad.ssamp,dgss2,Dgss2);
      diffusion.bsl[i] += bval_nested(gcrushs,dcrush2,Dcrush2,ss2_grad.ssamp,dgss2,Dgss2);
      /* Readout/Phase Cross-terms */
      diffusion.brp[i] += bval2(gcrushr,gcrushp,dcrush2,Dcrush2);
      /* Readout/Slice Cross-terms */
      diffusion.brs[i] += bval_cross(Gro,dgro,Dgro,gcrushs,dcrush2,Dcrush2);
      diffusion.brs[i] += bval_cross(Gro,dgro,Dgro,ss2_grad.ssamp,dgss2,Dgss2);
      diffusion.brs[i] += bval2(gcrushr,gcrushs,dcrush2,Dcrush2);
      diffusion.brs[i] += bval_cross(gcrushr,dcrush2,Dcrush2,ss2_grad.ssamp,dgss2,Dgss2);
      /* Slice/Phase Cross-terms */
      diffusion.bsp[i] += bval2(gcrushs,gcrushp,dcrush2,Dcrush2);
      diffusion.bsp[i] += bval_cross(gcrushp,dcrush2,Dcrush2,ss2_grad.ssamp,dgss2,Dgss2);
      if (spinecho[0] == 'y') {
        /* Readout */
        diffusion.bro[i] += bval(crush_grad.amp,dcrush3,Dcrush3);  
        diffusion.bro[i] += bval_nested(gdiff*droval,tdelta,tDELTA,crush_grad.amp,dcrush3,Dcrush3);
        /* Slice */
        diffusion.bsl[i] += bval(ss3_grad.amp,dgss3,Dgss3);
        diffusion.bsl[i] += bval_nested(gdiff*dslval,tdelta,tDELTA,ss3_grad.ssamp,dgss3,Dgss3);
        /* Readout/Slice Cross-terms */
        diffusion.brs[i] += bval_cross(gdiff*dslval,tdelta,tDELTA,crush_grad.amp,dcrush3,Dcrush3);
        diffusion.brs[i] += bval_cross(gdiff*droval,tdelta,tDELTA,ss3_grad.amp,dgss3,Dgss3);
        diffusion.brs[i] += bval_cross(crush_grad.amp,dcrush3,Dcrush3,ss3_grad.amp,dgss3,Dgss3);
        /* Readout/Phase Cross-terms */
        diffusion.brp[i] += bval_cross(gdiff*dpeval,tdelta,tDELTA,crush_grad.amp,dcrush3,Dcrush3);
        /* Slice/Phase Cross-terms */
        diffusion.bsp[i] += bval_cross(gdiff*dpeval,tdelta,tDELTA,ss3_grad.amp,dgss3,Dgss3);
      }
    }  /* End for-all-directions */
    /* Write the values */
    write_bvalues(&diffusion,"bval","bvalue","max_bval");
  }

  /* Minimum TR *****************************************/
  trmin =  ss_grad.rfCenterFront + etl*esp + ro_grad.timeFromEcho + pe_grad.duration + te3_delay + 2*GRADIENT_RES;
  if (spoilflag[0] == 'y') trmin += spoil_grad.duration;

  /* Increase TR if any options are selected ************/
  if (sat[0] == 'y')  trmin += satTime;
  if (fsat[0] == 'y') trmin += fsatTime;
  if (mt[0] == 'y')   trmin += mtTime;
  if (spinecho[0] == 'y')   trmin += te1;
  if (ticks > 0) trmin += GRADIENT_RES;

  /* Adjust for all slices ******************************/
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
    abort_message("ERROR %s: TR too short, minimum TR = %.3fms\n",seqfil,trmin*1000);
  }

  /* Calculate tr delay *********************************/
  tr_delay = granularity((tr-trmin)/ns,GRADIENT_RES);

  /* Set number of segments for profile or full image ***/
  nseg = prep_profile(profile[0],nseg,&pe_grad,&null_grad);

  pe2_steps = prep_profile(profile[1],nv2,&pe2_grad,&null_grad);
  F_initval(pe2_steps/2.0,vpe2_offset);

  /* Shift DDR for pro **********************************/
  roff = -poffset(pro,ro_grad.roamp);

  /* Adjust experiment time for VnmrJ *******************/
  if (ssc<0) {
    if (seqcon[2] == 's' && seqcon[3]=='s') g_setExpTime(trmean*ntmean*arraydim - ssc*arraydim);
    else if (seqcon[2]=='s') g_setExpTime(trmean*nseg*(ntmean*pe2_steps*arraydim - ssc*arraydim));
    else if (seqcon[3]=='s') g_setExpTime(trmean*pe2_steps*(ntmean*nseg*arraydim - ssc*arraydim));
    else g_setExpTime(trmean*(ntmean*pe_steps*pe2_steps*arraydim - ssc*arraydim));
  }
  else g_setExpTime(trmean*ntmean*nseg*pe2_steps*arraydim + tr*ssc);

  /* PULSE SEQUENCE *************************************/
  status(A);                          // Set status A
  rotate();                           // Set gradient rotation according to psi, phi and theta
  triggerSelect(trigger);             // Select trigger input 1/2/3
  obsoffset(resto);                   // Set spectrometer frequency
  delay(GRADIENT_RES);                // Delay for frequency setting

  initval(fabs(ssc),vssc);            // Compressed steady-state counter
  if (seqcon[2]=='s' && seqcon[3]=='s') assign(zero,vssc); // Zero for standard peloop and pe2loop
  assign(one,vacquire);               // real-time acquire flag

  /* Phase cycle: Alternate 180 phase to cancel residual FID */
  mod2(ct,vphase180);                 // 0101
  dbl(vphase180,vphase180);           // 0202
  add(vphase180,one,vphase180);       // 1313 Phase difference from 90
  add(vphase180,oph,vphase180);

  /* trigger */
  if (ticks > 0) F_initval((double)nsblock,vtrigblock);

  /* Begin phase-encode loop ****************************/       
  peloop2(seqcon[3],pe2_steps,vpe2_steps,vpe2_ctr);

    /* Begin phase-encode loop ****************************/
    peloop(seqcon[2],nseg,vseg,vseg_ctr);

      if (trtype) delay(ns*tr_delay);   // relaxation delay

      /* Compressed steady-states: 1st array & transient, all arrays if ssc is negative */
      if ((ix > 1) && (ssc > 0))
	assign(zero,vssc);
      if (seqcon[2] == 'c')
        sub(vseg_ctr,vssc,vseg_ctr);    // vseg_ctr counts up from -ssc
      else if (seqcon[3] == 'c')
        sub(vpe2_ctr,vssc,vpe2_ctr);    // vpe2_ctr counts up from -ssc
      assign(zero,vssc);
      if (seqcon[2] == 's' && seqcon[3]=='s')
	assign(zero,vacquire);          // Always acquire for non-compressed loop
      else {
        if (seqcon[2] == 'c') {
	  ifzero(vseg_ctr);
            assign(zero,vacquire);      // Start acquiring when vseg_ctr reaches zero
	  endif(vseg_ctr);
        }
        else if (seqcon[3] == 'c') {
	  ifzero(vpe2_ctr);
            assign(zero,vacquire);      // Start acquiring when vpe2_ctr reaches zero
	  endif(vpe2_ctr);
        }
      }
      setacqvar(vacquire);              // Turn on acquire when vacquire is zero

      /* Use standard encoding order for 2nd PE dimension */
      ifzero(vacquire);
        sub(vpe2_ctr,vpe2_offset,vpe2_mult);
      elsenz(vacquire);
        sub(zero,vpe2_offset,vpe2_mult);
      endif(vacquire);

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
        shapedpulselist(shapelist90,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
        delay(ss_grad.rfDelayBack);

        /* Spin echo preparation ******************************/
        if (spinecho[0] == 'y') {
          obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0.0,0.0,-ssr_grad.amp,WAIT);
          if (diff[0] == 'y') {
            delay(diffusion.d1);
            diffusion_dephase(&diffusion,dro,dpe,dsl);
            delay(diffusion.d2);
          }
          else
            delay(te_delay1);
          obspower(p3_rf.powerCoarse);
          obspwrf(p3_rf.powerFine);
          obl_shapedgradient(crush_grad.name,crush_grad.duration,crush_grad.amp,0.0,0.0,WAIT);
          obl_shapedgradient(ss3_grad.name,ss3_grad.duration,0,0,ss3_grad.amp,NOWAIT);
          delay(ss3_grad.rfDelayFront);
          shapedpulselist(shapelistte,ss3_grad.rfDuration,vphase180,rof1,rof2,seqcon[1],vms_ctr);
          delay(ss3_grad.rfDelayBack);
          obl_shapedgradient(crush_grad.name,crush_grad.duration,crush_grad.amp,0.0,0.0,WAIT);
          if (diff[0] == 'y') {
            delay(diffusion.d3);
            diffusion_rephase(&diffusion,dro,dpe,dsl);
            delay(diffusion.d4);
          }
          else
            delay(te_delay2);
          delay(ss_grad.duration/2.0);
          delay(te1_delay);
          obspower(p2_rf.powerCoarse);
          obspwrf(p2_rf.powerFine);
          obl_shapedgradient(ror_grad.name,ror_grad.duration,ror_grad.amp,0.0,0.0,WAIT);
        }

        else {
          /* Read dephase and Slice refocus */
          obl_shapedgradient(ssr_grad.name,ssr_grad.duration,ror_grad.amp,0.0,-ssr_grad.amp,WAIT);
          /* First half-TE delay */
          obspower(p2_rf.powerCoarse);
          obspwrf(p2_rf.powerFine);
          delay(te1_delay);
        }
	
        F_initval(etl,vetl);
        loop(vetl,vetl_ctr);

          mult(vseg_ctr,vetl,vpe_ctr);
          add(vpe_ctr,vetl_ctr,vpe_ctr);
          getelem(t1,vpe_ctr,vpe_mult);

          /* 180 degree pulse *********************************/
          obl_shapedgradient(crush_grad.name,crush_grad.duration,gcrushr,gcrushp,gcrushs,WAIT);
          obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);   
          delay(ss2_grad.rfDelayFront); 
          shapedpulselist(shapelist180,ss2_grad.rfDuration,vphase180,rof1,rof2,seqcon[1],vms_ctr);
          delay(ss2_grad.rfDelayBack);   
          obl_shapedgradient(crush_grad.name,crush_grad.duration,gcrushr,gcrushp,gcrushs,WAIT);

          /* Second half-TE period ****************************/
          delay(te2_delay);
	 
          /* Phase-encode gradient ****************************/
          pe2_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,-pe_grad.increment,-pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

          /* Readout gradient *********************************/
          obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.roamp,0,0,NOWAIT);
          delay(ro_grad.atDelayFront-alfa);

          /* Acquire data *************************************/
          startacq(alfa);
          acquire(np,1.0/sw);
          endacq();

          delay(ro_grad.atDelayBack);

          /* Rewinding phase-encode gradient ******************/
          pe2_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,pe_grad.increment,pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

          /* Second half-TE delay *****************************/
          delay(te3_delay);

        endloop(vetl_ctr);

        if (spoilflag[0] == 'y') 
          obl_shapedgradient(spoil_grad.name,spoil_grad.duration,spoil_grad.amp,spoil_grad.amp,spoil_grad.amp,WAIT);

      endmsloop(seqcon[1],vms_ctr);

    endpeloop(seqcon[2],vseg_ctr);

  endpeloop(seqcon[3],vpe2_ctr);

  /* Inter-image delay **********************************/
  sub(ntrt,ct,vtrimage);
  decr(vtrimage);
  ifzero(vtrimage);
    delay(trimage);
  endif(vtrimage);
}
