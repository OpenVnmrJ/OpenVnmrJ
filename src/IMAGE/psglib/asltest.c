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
ASL Test Sequence (centric out Gradient Echo)
************************************************************************/

#include <standard.h>
#include "sgl.c"

pulsesequence()
{
  /* Internal variable declarations *********************/
  double  testte,testtr,testflip,testns,testthk,testpss[MAXNSLICE];
  double  testpsi,testphi,testtheta,testlro,testpro,testlpe;
  double  tagcoiltr,tagcoilp1,tagcoiltpwr;
  double  maxgradtime,te_delay,tr_delay,trisesave,prepTime,seqtime;
  double  freqEx[MAXNSLICE];
  int     tagcoilcalib,shapeEx,*petab,inv,sepSliceRephase,nprep,i;
  char    mintestte[MAXSTR],mintesttr[MAXSTR],aslteststring[MAXSTR],mintagcoiltr[MAXSTR];

  /* Real-time variables used in this sequence **********/
  int  vpe_steps  = v1;      // Number of PE steps
  int  vpe_ctr    = v2;      // PE loop counter
  int  vms_slices = v3;      // Number of slices
  int  vms_ctr    = v4;      // Slice loop counter
  int  vpe_mult   = v5;      // PE multiplier, ranges from -PE/2 to PE/2

  /* The following real time variables reserved for IR are used in ASL: *
   * vslice_ctr    Slice counter                                        *
   * vslices       Total number of slices                               *
   * virblock      Number of slices per inversion recovery block        *
   * vnirpulses    Number of ir pulses first pass through ir block      *
   * vir           Flag to prescribe inversion pulses                   *
   * virslice_ctr  Inversion slice counter                              *
   * vnir          Number of ir pulses in loop (variable)               *
   * vnir_ctr      ir pulse loop counter                                */
  /* Rename some to be more meaningful */
  int vnps      = virblock;     /* Number of PS pulses */
  int vnips     = vnirpulses;   /* Number of IPS pulses */
  int vnq2      = vir;          /* Number of Q2TIPS pulses */
  int vspoil    = virslice_ctr; /* Gradient spoil multiplier */
  int vloop_ctr = vnir;         /* Loop counter */
  int vpwrf     = vnir_ctr;     /* Fine power */

  /* Initialize paramaters ******************************/
  init_mri();
  testte = getval("testte");
  testtr = getval("testtr");
  testflip = getval("testflip");
  testns = getval("testns");
  testthk = getval("testthk");
  if (seqcon[1] == 's') testpss[0] = getval("testpss");
  else if (seqcon[1] == 'c') getarray("testpss",testpss);
  testpsi = getval("testpsi");
  testphi = getval("testphi");
  testtheta = getval("testtheta");
  testlro = getval("testlro");
  testpro = getval("testpro");
  testlpe = getval("testlpe");
  tagcoiltr = getval("tagcoiltr");
  tagcoilp1 = getval("tagcoilp1");
  tagcoiltpwr = getval("tagcoiltpwr");
  tagcoilcalib = (int)getval("tagcoilcalib");
  getstr("mintestte",mintestte);
  getstr("mintesttr",mintesttr);
  getstr("aslteststring",aslteststring);
  getstr("mintagcoiltr",mintagcoiltr);

  /* For dedicated tagging coil calibration tagcoilcalib=1 */
  if (tagcoilcalib) {
    p1=tagcoilp1;
    testtr=tagcoiltr;
    strcpy(mintesttr,mintagcoiltr);
    tr=0.0;
  }

  /* Centric-out phase encode */
  if ((petab=(int *)malloc(nv*sizeof(int))) == NULL) nomem();
  putCmd("pelist = 0");   /* Re-initialize pelist */
  inv=1; petab[0]=0;
  for (i=1;i<nv;i++) {
    inv = -inv;
    petab[i] = petab[i-1] + inv*i;
    putCmd("pelist[%d] = %d",i+1,petab[i]);
  }
  settable(t1,nv,petab);

  /* If gradients are oblique trise must be increased */
  trisesave=trise;
  if (!trisesqrt3 && FP_EQ(trampfixed,0.0)) {
    if (remainder(testpsi,90.0) || remainder(testphi,90.0) || remainder(testtheta,90.0))
      trise = trise*sqrt(3.0);
  }

  /* Initialize gradient structures *********************/
  shape_rf(&p1_rf,"p1","gauss",p1,testflip,rof1,rof2);      // excitation pulse
  init_slice(&ss_grad,"ss",testthk);                  // slice select gradient
  init_slice_refocus(&ssr_grad,"ssr");                // slice refocus gradient
  init_readout(&ro_grad,"ro",testlro,np,sw);          // readout gradient
  ro_grad.pad1=alfa; ro_grad.pad2=alfa;
  init_readout_refocus(&ror_grad,"ror");              // dephase gradient
  init_phase(&pe_grad,"pe",testlpe,nv);               // phase encode gradient

  /* RF Calculations ************************************/
  calc_rf(&p1_rf,"","");

  /* Gradient calculations ******************************/
  calc_readout(&ro_grad, WRITE, "gro","sw","at");
  calc_readout_refocus(&ror_grad, &ro_grad, NOWRITE, "gror");
  calc_phase(&pe_grad, NOWRITE, "gpe","tpe");
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice_refocus(&ssr_grad,&ss_grad,WRITE,"gssr");

  /* Is TE long enough for separate slab refocus? *******/
  maxgradtime = MAX(ror_grad.duration,pe_grad.duration);
  temin = ss_grad.rfCenterBack + ssr_grad.duration + maxgradtime + ro_grad.timeToEcho + GRADIENT_RES;

  /* Equalize refocus and PE gradient durations *********/
  if ((testte >= temin) && (mintestte[0] != 'y')) {
    sepSliceRephase = 1;
    calc_sim_gradient(&ror_grad,&pe_grad,&null_grad,0,WRITE);
  } else {
    sepSliceRephase = 0;
    calc_sim_gradient(&ror_grad,&pe_grad,&ssr_grad,0,WRITE);
    temin -= ssr_grad.duration;
  }

  /* Reset trise */
  trise=trisesave;

  /* Create optional prepulse events ********************/
  /* The ASL module can not be used with the IR module */
  /* The ASL module uses the IR module real time variables and real time tables t57-t60 */
  create_arterial_spin_label();

  /* Check that all Gradient calculations are ok ********/
  sgl_error_check(sglerror);

  /* Min TE *********************************************/
  if (mintestte[0] == 'y') {
    testte = temin;
    putvalue("testte",testte);
  }
  if (FP_LT(testte,temin)) {
    abort_message("Test TE too short. Minimum Test TE= %.3fms\n",temin*1000);   
  }
  te_delay = testte - temin + GRADIENT_RES;

  /* Min TR *********************************************/   	
  trmin  = ss_grad.duration + te_delay + 2*pe_grad.duration + ro_grad.duration + GRADIENT_RES;
  trmin += (sepSliceRephase) ? ssr_grad.duration : 0.0;
  if (ticks > 0) trmin += 4e-6;
  trmin *= testns;
  if (mintesttr[0] == 'y') {
    testtr = trmin;
    if (tagcoilcalib) putvalue("tagcoiltr",testtr);
    else putvalue("testtr",testtr);
  }
  if (FP_LT(testtr,trmin)) {
    abort_message("Test TR too short. Minimum Test TR = %.3fms\n",trmin*1000);
  }
  tr_delay = granularity((testtr-trmin)/testns,GRADIENT_RES);

  /* Figure average duration of preparations */
  /* Tag and Ctrl are always run */
  prepTime = asl_grad.duration+GRADIENT_RES;
  if (tspoilasl > 0.0) prepTime += aslspoil_grad.duration;
  prepTime *=2;
  nprep = 2;
  if (aslteststring[0] == 'P') {
    prepTime += psTime;
    nprep++;
  }
  if (strstr(aslteststring,"IPS")!=NULL) {
    prepTime += ipsTime;
    nprep++;
  }
  if (strstr(aslteststring,"TagQ")!=NULL) {
    prepTime += 2*q2Time;
    nprep+=2;
  }
  prepTime/=nprep;

  /* Set up frequency offset pulse shape list ***********/   	
  offsetlist(testpss,ss_grad.ssamp,0,freqEx,testns,seqcon[1]);

  if (tagcoilcalib) {
    prepTime=0.0;
    shapeEx = dec2shapelist(p1_rf.pulseName,ss_grad.rfDuration,freqEx,testns,ss_grad.rfFraction,seqcon[1]);
  } else
    shapeEx = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freqEx,testns,ss_grad.rfFraction,seqcon[1]);
  
  /* Set pe_steps for profile or full image *************/   	
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&per_grad);

  /* Shift DDR for pro **********************************/   	
  roff = -poffset(testpro,ro_grad.roamp);

  /* Adjust experiment time for VnmrJ *******************/
  seqtime=tr+prepTime+testtr*pe_steps;
  g_setExpTime(1+seqtime*nt*arraydim);

  /* PULSE SEQUENCE *************************************/
  status(A);
  rotate();
  obsoffset(resto);
  /* Systems with 3rd channel may be configured for transmit sense, in which case 
     there may just be one frequency synthesizer that is used for channel 1 and 
     channel 3 which means we are already properly at resto with obsoffset(resto).
     This config requires system global rfGroupMap='10111' rather than rfGroupMap='HLHHH'
     If there is a 3rd channel frequency synthesizer then we need to set with dec2offset.
     So set dec2offset in any case. NB dof2=resto is set in aslset. 
  */
  if (tagcoilcalib) dec2offset(resto);
  delay(GRADIENT_RES);

  delay(tr);

  sp1on(); delay(4e-6); sp1off();

  /* Presaturation */
  if (!strcmp(asltest,"PS")) {
    set_rotation_matrix(pspsi,psphi,pstheta);
    obspower(ps_rf.powerCoarse);
    delay(GRADIENT_RES);
// Need to set vobspwrfstepsize(1.0) for DD2 here
// if (DD2) vobspwrfstepsize(1.0);
    loop(vnps,vloop_ctr);
      getelem(t60,vloop_ctr,vpwrf);
      getelem(t59,vloop_ctr,vspoil);
      vobspwrf(vpwrf);
      obl_shapedgradient(ps_grad.name,ps_grad.duration,0.0,0.0,ps_grad.ssamp,NOWAIT);
      delay(ps_grad.rfDelayFront);
      shapedpulselist(shapePs,ps_grad.rfDuration,oph,rof1,rof1,'i',0);
      delay(ps_grad.rfDelayBack);
      if (tspoilps > 0.0)
        var3_shapedgradient(psspoil_grad.name,psspoil_grad.duration,0.0,0.0,0.0,psgamp,psgamp,psgamp,vspoil,vspoil,vspoil,WAIT);
    endloop(vloop_ctr);
  }

  /* Tag */
  if (!strcmp(asltest,"Tag")) {
    set_rotation_matrix(aslpsi,aslphi,asltheta);
    /* Set power, offset and PDD switching for asltagcoil tag pulse if required */
    if (asltagcoil[0] == 'y') {
      /* Systems with 3rd channel may be configured for transmit sense, in which case 
         there may just be one frequency synthesizer that is used for channel 1 and 
         channel 3 which means we are already properly at resto with obsoffset(resto).
         This config requires system global rfGroupMap='10111' rather than rfGroupMap='HLHHH'
         If there is a 3rd channel frequency synthesizer then we need to set with dec2offset.
         So set dec2offset in any case. NB dof2=resto is set in aslset. 
      */
      dec2offset(resto);
      /* The coil config can be for three different coils, volume coil for XMT, 
         surface coil array for RCV and dedicated tag coil for CASL. 
         A 3 channel PDD is required with a channel dedicated to each coil.
         3 spare line outs were used to switch the PDD as required. 
         Normally sp3on/sp3off is used to tune/detune the XMT coil and RCV coils, 
         but in this experiment that does not work because when the tag coil is tuned
         for CASL both XMT and RCV coils need to be detuned.
         The default is that at rest the volume coil is on resonance.
      */
      asl_xmtoff();    /* switch regular transmit volume coil off-resonance */
      asl_tagcoilon(); /* switch tag coil on-resonance */
                       /* if (volumercv[0] == 'n') asl_xmtoff()/asl_xmton() will also be required about acquisition */
      if (!aslphaseramp) {
        obsoffset(resto+freqAsl[0]);
        dec2offset(resto+freqAsl[0]);
      }
      dec2power(asl_rf.powerCoarse);
      dec2pwrf(asl_rf.powerFine);
    } else {
      if (!aslphaseramp) obsoffset(resto+freqAsl[0]);
      obspower(asl_rf.powerCoarse);
      obspwrf(asl_rf.powerFine);
    }
    delay(GRADIENT_RES);
    obl_shapedgradient(asl_grad.name,asl_grad.duration,0.0,0.0,asltaggamp,NOWAIT);
    delay(asl_grad.rfDelayFront);
    if (asltagcoil[0] == 'y') {
    if (aslphaseramp) dec2shapedpulselist(shapeAsl,asl_grad.rfDuration,oph,rof1,rof1,'s',zero);
    else dec2shaped_pulse(asl_rf.pulseName,asl_grad.rfDuration,oph,rof1,rof1);
    } else {
      if (aslphaseramp) shapedpulselist(shapeAsl,asl_grad.rfDuration,oph,rof1,rof1,'s',zero);
      else rgpulse(asl_grad.rfDuration,oph,rof1,rof1);
    }
    delay(asl_grad.rfDelayBack);
    /* Reset PDD switching to default state if need be */
    if (asltagcoil[0] == 'y') {
      asl_xmton();      /* switch regular transmit volume coil on-resonance */
      asl_tagcoiloff(); /* switch tag coil off-resonance */
                        /* if (volumercv[0] == 'n') asl_xmtoff()/asl_xmton() will also be required about acquisition */
    }
    if (tspoilasl > 0.0)
      obl_shapedgradient(aslspoil_grad.name,aslspoil_grad.duration,0.0,0.0,aslspoil_grad.amp,WAIT);
    if (!aslphaseramp) { 
      obsoffset(resto); 
      if (asltagcoil[0] == 'y') dec2offset(resto);
      delay(GRADIENT_RES); 
    }
  }

  /* Ctrl */
  if (!strcmp(asltest,"Ctrl")) {
    set_rotation_matrix(aslctrlpsi,aslctrlphi,aslctrltheta);
    /* Set power, offset and PDD switching for asltagcoil control pulse if required */
    if (asltagcoil[0] == 'y') {
      /* Systems with 3rd channel may be configured for transmit sense, in which case 
         there may just be one frequency synthesizer that is used for channel 1 and 
         channel 3 which means we are already properly at resto with obsoffset(resto).
         This config requires system global rfGroupMap='10111' rather than rfGroupMap='HLHHH'
         If there is a 3rd channel frequency synthesizer then we need to set with dec2offset.
         So set dec2offset in any case. NB dof2=resto is set in aslset. 
      */
      dec2offset(resto);
      /* The coil config can be for three different coils, volume coil for XMT, 
         surface coil array for RCV and dedicated tag coil for CASL. 
         A 3 channel PDD is required with a channel dedicated to each coil.
         3 spare line outs were used to switch the PDD as required. 
         Normally sp3on/sp3off is used to tune/detune the XMT coil and RCV coils, 
         but in this experiment that does not work because when the tag coil is tuned
         for CASL both XMT and RCV coils need to be detuned.
         The default is that at rest the volume coil is on resonance.
      */
      asl_xmtoff();    /* switch regular transmit volume coil off-resonance */
      asl_tagcoilon(); /* switch tag coil on-resonance */
                       /* if (volumercv[0] == 'n') asl_xmtoff()/asl_xmton() will also be required about acquisition */
      if (!aslphaseramp) {
        obsoffset(resto+freqAslCtrl[0]);
        dec2offset(resto+freqAslCtrl[0]);
      }
      dec2power(aslctrl_rf.powerCoarse);
      dec2pwrf(aslctrl_rf.powerFine);
    } else {
      if (!aslphaseramp) obsoffset(resto+freqAslCtrl[0]);
      obspower(aslctrl_rf.powerCoarse);
      obspwrf(aslctrl_rf.powerFine);
    }
    delay(GRADIENT_RES);
    obl_shapedgradient(asl_grad.name,asl_grad.duration,0.0,0.0,asltaggamp,NOWAIT);
    delay(asl_grad.rfDelayFront);
    if (asltagcoil[0] == 'y') {
      if (aslphaseramp) dec2shapedpulselist(shapeAslCtrl,asl_grad.rfDuration,oph,rof1,rof1,'s',zero);
      else dec2shaped_pulse(aslctrl_rf.pulseName,asl_grad.rfDuration,oph,rof1,rof1);
    } else {
      if (aslphaseramp) shapedpulselist(shapeAslCtrl,asl_grad.rfDuration,oph,rof1,rof1,'s',zero);
      else shaped_pulse(aslctrl_rf.pulseName,asl_grad.rfDuration,oph,rof1,rof1);
    }
    delay(asl_grad.rfDelayBack);
    /* Reset PDD switching to default state if need be */
    if (asltagcoil[0] == 'y') {
      asl_xmton();      /* switch regular transmit volume coil on-resonance */
      asl_tagcoiloff(); /* switch tag coil off-resonance */
                        /* if (volumercv[0] == 'n') asl_xmtoff()/asl_xmton() will also be required about acquisition */
    }
    if (tspoilasl > 0.0)
      obl_shapedgradient(aslspoil_grad.name,aslspoil_grad.duration,0.0,0.0,aslspoil_grad.amp,WAIT);
    if (!aslphaseramp) { 
      obsoffset(resto); 
      if (asltagcoil[0] == 'y') dec2offset(resto);
      delay(GRADIENT_RES); 
    }
  }

  /* IPS */
  if (!strcmp(asltest,"IPS")) {
    set_rotation_matrix(ipspsi,ipsphi,ipstheta);
    obspower(ips_rf.powerCoarse);
    delay(GRADIENT_RES);
// Need to set vobspwrfstepsize(1.0) for DD2 here
// if (DD2) vobspwrfstepsize(1.0);
    loop(vnips,vloop_ctr);
      getelem(t58,vloop_ctr,vpwrf);
      getelem(t57,vloop_ctr,vspoil);
      vobspwrf(vpwrf);
      obl_shapedgradient(ips_grad.name,ips_grad.duration,0.0,0.0,ips_grad.ssamp,NOWAIT);
      delay(ips_grad.rfDelayFront);
      shapedpulselist(shapeIps,ips_grad.rfDuration,oph,rof1,rof1,'c',zero);
      delay(ips_grad.rfDelayBack);
      if (tspoilips > 0.0)
        var3_shapedgradient(ipsspoil_grad.name,ipsspoil_grad.duration,0.0,0.0,0.0,ipsgamp,ipsgamp,ipsgamp,vspoil,vspoil,vspoil,WAIT);
    endloop(vloop_ctr);
  }

  /* TagQ */
  if (!strcmp(asltest,"TagQ")) {
    set_rotation_matrix(q2psi,q2phi,q2theta);
    obspower(q2_rf.powerCoarse);
    obspwrf(q2_rf.powerFine);
    delay(GRADIENT_RES);
    loop(vnq2,vloop_ctr);
      obl_shapedgradient(q2_grad.name,q2_grad.duration,0.0,0.0,q2_grad.amp,NOWAIT);
      delay(q2_grad.rfDelayBack);
      shapedpulselist(shapeQ2,q2_grad.rfDuration,oph,rof1,rof1,'c',zero);
      delay(q2_grad.rfDelayBack);
      if (tspoilq2 > 0.0)
        obl_shapedgradient(q2spoil_grad.name,q2spoil_grad.duration,q2spoil_grad.amp,q2spoil_grad.amp,q2spoil_grad.amp,WAIT);
    endloop(vloop_ctr);
  }

  /* CtrlQ */
  if (!strcmp(asltest,"CtrlQ")) {
    set_rotation_matrix(q2psi,q2phi,q2theta);
    obspower(q2_rf.powerCoarse);
    obspwrf(q2_rf.powerFine);
    delay(GRADIENT_RES);
    loop(vnq2,vloop_ctr);
      obl_shapedgradient(q2_grad.name,q2_grad.duration,0.0,0.0,q2_grad.amp,NOWAIT);
      delay(q2_grad.rfDelayBack);
      shapedpulselist(shapeQ2,q2_grad.rfDuration,oph,rof1,rof1,'c',zero);
      delay(q2_grad.rfDelayBack);
      if (tspoilq2 > 0.0)
        obl_shapedgradient(q2spoil_grad.name,q2spoil_grad.duration,q2spoil_grad.amp,q2spoil_grad.amp,q2spoil_grad.amp,WAIT);
    endloop(vloop_ctr);
  }

  /* Set image rotation */
  set_rotation_matrix(testpsi,testphi,testtheta);

  /* Begin phase-encode loop ****************************/       
  peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);

    /* Read external kspace table if set ******************/       
    getelem(t1,vpe_ctr,vpe_mult);

    /* Begin multislice loop ******************************/       
    msloop(seqcon[1],testns,vms_slices,vms_ctr);

      delay(tr_delay); 

      /* Slice select RF pulse ******************************/ 
      if (tagcoilcalib) {
        /* A 3 channel PDD is required with a channel dedicated to each coil.
           3 spare line outs were used to switch the PDD as required. 
           Normally sp3on/sp3off is used to tune/detune the XMT coil and RCV coils, 
           but in this experiment that does not work because when the tag coil is tuned
           for CASL both XMT and RCV coils need to be detuned.
           The default is that at rest the volume coil is on resonance.
        */
        asl_xmtoff();    /* switch regular transmit volume coil off-resonance */
        asl_tagcoilon(); /* switch tag coil on-resonance */
        dec2power(tagcoiltpwr);
        dec2pwrf(4095.0);
        delay(GRADIENT_RES);
        obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
        delay(ss_grad.rfDelayFront);
        dec2shapedpulselist(shapeEx,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
        delay(ss_grad.rfDelayBack);
        asl_xmton();      /* switch regular transmit volume coil on-resonance */
        asl_tagcoiloff(); /* switch tag coil off-resonance */
      } else {
        obspower(p1_rf.powerCoarse);
        obspwrf(p1_rf.powerFine);
        delay(GRADIENT_RES);
        obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
        delay(ss_grad.rfDelayFront);
        shapedpulselist(shapeEx,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
        delay(ss_grad.rfDelayBack);
      }

      /* Phase encode, refocus, and dephase gradient ********/
      if (sepSliceRephase) {
        obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);
        delay(te_delay);
        pe_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,0,-pe_grad.increment,vpe_mult,WAIT);
      } else {
        pe_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,-ssr_grad.amp,-pe_grad.increment,vpe_mult,WAIT);
        delay(te_delay);
      }
      obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
      delay(ro_grad.atDelayFront-alfa);
      if (volumercv[0] == 'n') asl_xmtoff(); /* PDD switching only for dedicated ASL tagging coil configuration */
      startacq(alfa);
      acquire(np,1.0/sw);
      delay(ro_grad.atDelayBack);
      endacq();
      if (volumercv[0] == 'n') asl_xmton();  /* PDD switching only for dedicated ASL tagging coil configuration */
      pe_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,pe_grad.increment,vpe_mult,WAIT);

    endmsloop(seqcon[1],vms_ctr);

  endpeloop(seqcon[2],vpe_ctr);

  calc_grad_duty(testtr);

}
