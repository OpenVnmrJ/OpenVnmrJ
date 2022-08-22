/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************
    STEAM2.C

    Volume localized spectroscopy using the stimulated echo
    technique, with optional water suppression (chess, wet, vapor).
    This short TE sequence is limited to RF pulses with 512 us durations;
    based on methods developed at University of Minnesota by Ivan Tkac et al.
************************************************************/

  #include <standard.h>
  #include "sgl.c"

  GENERIC_GRADIENT_T       endspoil_grad;  // crusher after FID is acquired
  RF_PULSE_T    ws_rftm;              /* RF pulse structure in TM period */
  SLICE_SELECT_GRADIENT_T  ss1_grad,ss2_grad, ss3_grad; //This is dummy grad to calculate refocusing gradients without butterflies
  

 
  SLICE_SELECT_GRADIENT_T  vox2_crush, vox3_crush,vox2a_crush, vox3a_crush;
   
  
  /* Phases for phase cycling */
  int counter,noph;
  static int ph1[8]   = {0, 2, 0, 2,  0, 2, 0, 2};
  static int ph2[8]   = {0, 0, 0, 0,  2, 2, 2, 2};
  static int ph3[8]   = {0, 0, 2, 2,  0, 0, 2, 2};
  static int phobs[8] = {0, 2, 2, 0,  2, 0, 0, 2};

  char autoph[MAXSTR], pcflag[MAXSTR];
  


void pulsesequence()
  {
  /* INTERNAL VARIABLE DECLARATIONS *********************/
  double freq1,freq2,freq3,ws_delta;
 

  /* sequence timing variables */
  double te_delay1, te_delay2, newdelay,tr_delay, tm_delay;
  double tau1=0, tau2=0;

  /*frequency and profile*/  
  double rprof,pprof,sprof;
  double restol, resto_local, csd_ppm;
  char profile_ovs[MAXSTR];
  char profile_vox[MAXSTR];
  
  /* Extra crushers */
  double gcrushtm1,gcrushtm2, tcrushtm1,tcrushtm2;
  double fx,fy,fz;
  double vox2_cr, vox3_cr;
  double gcrush_end, tcrush_end;

 /*extra ws pulse flag*/
 char ws_tm[MAXSTR];
 double wsflipftm;
 

    
  /* Initialize paramaters **********************************/
  init_mri();
  get_ovsparameters();
  get_wsparameters();

  //Read in parameters not defined in acqparms.h and sglHelper 
 
  restol=getval("restol");  //local frequency offset 
  csd_ppm=getval("csd_ppm"); //chemical shift displacement factor
  roff=getval("roff");       //receiver offset
  

  gcrushtm1 = getval("gcrushtm1");
  gcrushtm2 = getval("gcrushtm2");
  tcrushtm1 = getval("tcrushtm1");
  tcrushtm2 = getval("tcrushtm2");

  gcrush_end = getval("gcrush_end");
  tcrush_end = getval("tcrush_end");

  getstr("ws_tm",ws_tm);
  wsflipftm=getval("wsflipftm");
  ws_delta=getval("ws_delta");

  rprof = getval("rprof");
  pprof = getval("pprof");
  sprof = getval("sprof");
  getstr("profile_ovs",profile_ovs);
  getstr("profile_vox",profile_vox);
 

  fx=getval("fx");
  fy=getval("fy");
  fz=getval("fz");

  /*set voxel sizes for butterfly crushers to 10^6 to set the slice portion to zero amplitude ***/
  
  vox2_cr=1000000;
  vox3_cr=1000000;

  

  /* Initialize gradient +rf structures   ***************/
  
  
  trampfixed=trise; //rise time =trise 
  tcrush=granularity(tcrush,GRADIENT_RES); //this is to avoid the granularity errors
  //if trampfixed is used, rise time needs to be checked 
  if (trise*2>tcrush){
  
   abort_message("tcrush too short. Minimum tcrush = %fms \n",1000*trise*2);
  }

  if (gcrush>gmax){
  
   abort_message("gcrush too large. Max gcrush = %f \n",gmax*0.95);
  }

  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);
  init_rf(&p2_rf,p2pat,p2,flip1,rof1,rof2);
  init_rf(&ws_rftm,p4pat,p4,flip1,rof1,rof2);
  //init_rf(&ws_rftm,p4pat,p4,wsflipftm*flip1,rof1,rof2);


  init_slice(&vox1_grad,"vox1",vox1);
  init_slice_butterfly(&vox2_grad,"vox2",vox2,gcrush,tcrush); //the second voxel selective gradient will have 2nd butterfly tm crusher
  init_slice_butterfly(&vox3_grad,"vox3",vox3,gcrush,tcrush);

  init_slice_butterfly(&vox2_crush,"vox2_crush",vox2_cr,gcrush,tcrush);
  init_slice_butterfly(&vox3_crush,"vox3_crush",vox3_cr,gcrush,tcrush);
  init_slice_butterfly(&vox2a_crush,"vox2a_crush",vox2_cr,gcrush,tcrush);
  init_slice_butterfly(&vox3a_crush,"vox3a_crush",vox3_cr,gcrush,tcrush);
  
 //this is to hold refocusing information, this is dummy gradient (not executed)
 
  init_slice(&ss2_grad,"ss2",vox2);
  init_slice(&ss3_grad,"ss3",vox3);
  

  
 
  if (profile_vox[0] == 'y') {

    init_readout_butterfly(&ro_grad,"ro",lro,np,sw,gcrushro,tcrushro);
    init_readout_refocus(&ror_grad,"ror");
  }

  
  init_generic(&endspoil_grad,"endspoil",gcrush_end,tcrush_end);
 
   
  
  /* Calculate RF */
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");

  ws_rftm.flipmult=wsflipftm;
  calc_rf(&ws_rftm,"wstpwr","wstpwrf");
  //calc_rf(&ws_rftm,"","");

  
  
 
   
  
 
  

  /*Calculate Gradients*/
  calc_slice(&ss2_grad,&p2_rf,NOWRITE,"");
  calc_slice(&ss3_grad,&p1_rf,NOWRITE,"");

  calc_slice(&vox1_grad,&p1_rf,WRITE,"gvox1");
  calc_slice(&vox2_grad,&p2_rf,NOWRITE,"");
  calc_slice(&vox3_grad,&p1_rf,NOWRITE,"");  

  calc_slice(&vox2_crush,&p2_rf,NOWRITE,"");
  calc_slice(&vox3_crush,&p2_rf,NOWRITE,"");
  calc_slice(&vox2a_crush,&p1_rf,NOWRITE,"");
  calc_slice(&vox3a_crush,&p1_rf,NOWRITE,"");

  //Now recalculate gradient shapes

  vox2_crush.crusher1Moment0 -= vox1_grad.m0ref; //only now can re-calculate the moment after it was defined above
  vox2_crush.crusher1CalcFlag=AMPLITUDE_FROM_MOMENT_DURATION_RAMP;
  vox2_crush.cr2amp=gcrushtm1;
  vox2_crush.crusher2Duration=tcrushtm1;
  calc_slice(&vox2_crush,&p2_rf,WRITE,"vox2_crush");

  vox3_crush.cr2amp=gcrushtm1;
  vox3_crush.crusher2Duration=tcrushtm1;
  calc_slice(&vox3_crush,&p2_rf,NOWRITE,"");
 // calc_slice(&vox3_crush,&p2_rf,WRITE,"vox3_crush");

  
  vox2_grad.crusher1Moment0 -= (ss2_grad.m0def);
  vox2_grad.crusher1CalcFlag=AMPLITUDE_FROM_MOMENT_DURATION_RAMP;
  vox2_grad.cr2amp=0; //=gcrushtm1;
  vox2_grad.crusher2Duration=tcrushtm1;
  calc_slice(&vox2_grad,&p2_rf,WRITE,"vox2_grad");

 
  
  vox3_grad.crusher2Moment0 -= (ss3_grad.m0ref); 
  vox3_grad.crusher2CalcFlag=AMPLITUDE_FROM_MOMENT_DURATION_RAMP;
  vox3_grad.cr1amp=gcrushtm2;
  vox3_grad.crusher1Duration=tcrushtm2;
  calc_slice(&vox3_grad,&p1_rf,WRITE,"vox3_grad");

  vox2a_crush.cr1amp=gcrushtm2;
  vox2a_crush.crusher1Duration=tcrushtm2;
  calc_slice(&vox2a_crush,&p1_rf,WRITE,"vox2a_crush");

  vox3a_crush.cr1amp=gcrushtm2;
  vox3a_crush.crusher1Duration=tcrushtm2;
  calc_slice(&vox3a_crush,&p1_rf,NOWRITE,"");

  calc_generic(&endspoil_grad,WRITE,"","");

  //additional corrections of the crusher gradients to ensure proper refocusing
  
   vox3_crush.crusher1Moment0 = vox3_grad.m0ref;
   vox3_crush.crusher1CalcFlag=AMPLITUDE_FROM_MOMENT_DURATION_RAMP;
   calc_slice(&vox3_crush,&p1_rf,WRITE,"vox3_crush");


   vox3a_crush.crusher2Moment0 = vox2_grad.m0def;
   vox3a_crush.crusher2CalcFlag=AMPLITUDE_FROM_MOMENT_DURATION_RAMP;
   calc_slice(&vox3a_crush,&p1_rf,WRITE,"vox3a_crush");

  //set all gradients along the particular directions to zero  
  if (profile_ovs[0]=='y'){
     if (rprof==1) {
       vox1_grad.amp=0; //set slice selection in read direction to none 
       vox2_crush.amp=0; // set corresponding crusher gradients to none
       vox2a_crush.amp=0;
     }
     else if(pprof==1) {
     vox2_grad.amp=0;
     vox3a_crush.amp=0;
     }     
     else if(sprof==1) {
     vox3_grad.amp=0;
     vox3_crush.amp=0;
     }
  }


  
  if (profile_vox[0] == 'y') {
    calc_readout(&ro_grad,WRITE,"gro","sw","at");
    putvalue("gro",ro_grad.roamp);       // RO grad
    calc_readout_refocus(&ror_grad,&ro_grad,WRITE,"gror");
    putvalue("tror",ror_grad.duration);  // ROR duration
  }


 
  //displaySlice(&vox1_grad);
  //displaySlice(&vox2_grad);
  //displaySlice(&vox3_grad);
 
 
  /* Optional Outer Volume Suppression */
  if (ovs[0] == 'y')  create_ovsbands();
  if (ws[0]  == 'y')  create_watersuppress();
  if (sat[0] == 'y')  create_satbands();


  /****************************************************/
  /* Sequence Timing **********************************/
  /****************************************************/
  /*  Min TE ******************************************/
   
  tau1 = vox1_grad.rfCenterBack + vox2_grad.rfCenterFront;
 
  
  tau2 = vox3_grad.rfCenterBack+alfa;

  temin = 2*(MAX(tau1,tau2) + 4e-6);  /* have at least 4us between gradient events */

  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  else if (te < temin) {
    abort_message("TE too short.  Minimum TE = %.2fms\n",temin*1000);   
  }
  te_delay1 = te/2 - tau1;
  te_delay2 = te/2 - tau2;

  //newdelay = te_delay2-(1/sw*2);
  //if (newdelay<0) abort_message("TE is too short to subtract two data points");

  /***************************************************/
  /* Min TM ******************************************/   	
  tau1  = vox2_grad.rfCenterBack + 4e-6 + vox3_grad.rfCenterFront;
  if (ws_tm[0] == 'y') {  // Apply water suppression pulse during TM period
    tau1 += ws_rftm.rfDuration;
  }  

  tmmin = tau1 + 4e-6;  /* have at least 4us between gradient events */

  if (mintm[0] == 'y') {
    tm = tmmin;
    putvalue("tm",tm);
  }
  else if (tm < tmmin) {
    abort_message("TM too short.  Minimum TM = %.2fms\n",tmmin*1000);   
  }
  tm_delay = (tm - tau1);
  

  /***************************************************/
  /* Min TR ******************************************/   	
  tau1 = 4e-6 + vox1_grad.rfCenterFront + te + tm + at+endspoil_grad.duration;

  if (ws[0]  == 'y') tau1 += wsTime;
  if (ovs[0] == 'y') tau1 += ovsTime;
  if (sat[0] == 'y') tau1 += satTime;
  
  trmin = tau1 + 4e-6;   /* have at least 4us between gradient events */

  if (mintr[0] == 'y') {
    tr = trmin;  // ensure at least 4us between gradient events
    putvalue("tr",tr);
  }
  if ((trmin-tr) > 12.5e-9) {
    abort_message("TR too short.  Minimum TR= %.2fms\n",trmin*1000);
  }

  tr_delay = tr - tau1;
  if (tr_delay < 0.0) {
      abort_message("tr too short.  Minimum tr = %.2f ms\n",trmin*1000);
  }
  

  /* Frequency offsets */

  resto_local=resto-restol;  

  
  freq1    = poffset(pos1,vox1_grad.ssamp); // First  RF pulse
  freq2    = poffset(pos2,vox2_grad.ssamp); // Second RF pulse
  freq3    = poffset(pos3,vox3_grad.ssamp); // Third  RF pulse
  
  
  

  freq1=freq1-csd_ppm*sfrq;
  freq2=freq2-csd_ppm*sfrq;
  freq3=freq3-csd_ppm*sfrq;
  


  /* Frequency offsets */
  if (profile_vox[0] == 'y') {
    /* Shift DDR for pro ************************************/
    roff = -poffset(pro,ro_grad.roamp);
  } 



  /* Put gradient information back into VnmrJ parameters */
  putvalue("gvox1",vox1_grad.ssamp);
  putvalue("gvox2",vox2_grad.ssamp);
  putvalue("gvox3",vox3_grad.ssamp);
  putvalue("rgvox1",vox1_grad.tramp);
  putvalue("rgvox2",vox2_grad.tramp);
  putvalue("rgvox3",vox3_grad.tramp);
  
  
  /* Create phase cycling */

  noph=getval("noph");
  getstr("autoph",autoph);
  getstr("pcflag",pcflag);
 

  

  assign(zero, oph); 
  counter=(double)nt*(ix-1); //counter of the array (0,4,8,12..) for nt=4
  if (autoph[0] == 'n') counter=0.0; //only goes through nt, if 'y' goes through nt*array
  initval(counter,v1);  
  initval(noph,v3);
  add(v1,ct,v5); 
  modn(v5,v3,v5); 

  if (pcflag[0] == 'y') {
  settable(t1,noph,ph1);   getelem(t1,v5,v6); //where ct is the counter, noph=8 for steam
  settable(t2,noph,ph2);   getelem(t2,v5,v7);
  settable(t3,noph,ph3);   getelem(t3,v5,v8);
  settable(t4,noph,phobs); getelem(t4,v5,oph);
  }
  else { 
  assign(zero, v6); 
  assign(zero, v7); 
  assign(zero, v8); 
  assign(zero, oph); 
  }


  sgl_error_check(sglerror);
  
  if (ss<0) g_setExpTime(trmean*(nt-ss)*arraydim);
  else g_setExpTime(tr*(ntmean*arraydim+ss));


  /* PULSE SEQUENCE *************************************/
  /* Relaxation delay ***********************************/
  obsoffset(resto_local);  // need it here for water suppression to work
  delay(4e-6);
  rot_angle(vpsi,vphi,vtheta);
  if (ticks) {
    xgate(ticks);
    grad_advance(gpropdelay);
    delay(4e-6);
  }

  /* Saturation bands ***********************************/
  if (ovs[0] == 'y') ovsbands();
  if (sat[0] == 'y') satbands();

  /* Water suppression **********************************/
  if (ws[0]  == 'y') watersuppress();

  /* 90 degree RF pulse *********************************/
 
  obspower(p1_rf.powerCoarse);
  obspwrf(p1_rf.powerFine);
  delay(4e-6);
  obl_shaped3gradient(vox1_grad.name,"","",vox1_grad.duration,vox1_grad.amp,0.0,0.0,NOWAIT);   
  delay(vox1_grad.rfDelayFront);
  
  if (profile_ovs[0]=='y'&& rprof==1) freq1=0.0;
  shapedpulseoffset(p1_rf.pulseName,vox1_grad.rfDuration,v6,rof1,rof2,freq1);
  delay(vox1_grad.rfDelayBack);
  
  
  delay(te_delay1);

  /* 2nd 90 degree pulse ********************************/
  obl_shaped3gradient(vox2_crush.name,vox2_grad.name,vox3_crush.name,vox2_grad.duration,vox2_crush.amp,vox2_grad.amp,fz*vox3_crush.amp,NOWAIT);   
 
 delay(vox2_grad.rfDelayFront);
  if (profile_ovs[0]=='y'&& pprof==1) freq2=0.0;
  shapedpulseoffset(p2_rf.pulseName,vox2_grad.rfDuration,v7,rof1,rof2,freq2);
  delay(vox2_grad.rfDelayBack);
 
  
   

 
  /* Optional TM water suppression ***********************/
  if (ws_tm[0] == 'y') {
    delay(tm_delay/2);
    if (wsrf[0]=='y') {
    obspower(ws_rftm.powerCoarse);
    obspwrf(ws_rftm.powerFine);
    delay(4e-6);
    shapedpulseoffset(ws_rftm.pulseName,ws_rftm.rfDuration,zero,rof1,rof2,ws_delta);
    }
    else delay(ws_rftm.rfDuration);
  

  
  /* TM period *******************************************/
  delay(tm_delay/2);

  } //end of ws_tm='y' condition
  else {

 delay(tm_delay);
  }

  /* 3rd 90 degree pulse ********************************/
  obspower(p1_rf.powerCoarse);  //may have been changed by WS during TM period
  obspwrf(p1_rf.powerFine);
  delay(4e-6);
  obl_shaped3gradient   (vox2a_crush.name,vox3a_crush.name,vox3_grad.name,vox3_grad.duration,fx*vox2a_crush.amp,fy*vox3a_crush.amp,vox3_grad.amp,NOWAIT);   
  delay(vox3_grad.rfDelayFront);
 
  if (profile_ovs[0]=='y'&& sprof==1) freq3=0.0;
  shapedpulseoffset(p1_rf.pulseName,vox3_grad.rfDuration,v8,rof1,rof2,freq3);
  delay(vox3_grad.rfDelayBack);
 
   
   delay(te_delay2);
   
  //delay(newdelay); //this delay can be used if 2 points are taken out of fid
  if (profile_vox[0] == 'y') {
    obl_shapedgradient(ror_grad.name,ror_grad.duration, -rprof*ror_grad.amp,-pprof*ror_grad.amp,-sprof*ror_grad.amp,WAIT);
    delay(GDELAY);
    obl_shapedgradient(ro_grad.name,ro_grad.duration,rprof*ro_grad.amp,pprof*ro_grad.amp,sprof*ro_grad.amp,NOWAIT); 
    delay(ro_grad.atDelayFront);
    startacq(alfa);
    acquire(np,1.0/sw);
    delay(ro_grad.atDelayBack);
    endacq();
  } else {
    startacq(alfa);
    acquire(np,1.0/sw);
    endacq();
  }

  /* Data acquisition ***********************************/
  obl_shaped3gradient   (endspoil_grad.name,endspoil_grad.name,endspoil_grad.name,endspoil_grad.duration,endspoil_grad.amp,endspoil_grad.amp,endspoil_grad.amp,WAIT);  
  delay(tr_delay);

}  //end of the pulse sequence

/*********************************************************************
                      MODIFICATION HISTORY

Jan/2010 (LK)
*********************************************************************/
