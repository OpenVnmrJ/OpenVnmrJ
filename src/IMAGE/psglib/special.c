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
/******************************************************************
    SPECIAL.C

    Volume localized spectroscopy using 1D ISIS

    Ref: R.J. Ordidge, J. Magn. Reson., 66, p. 283, (1086); 
    Mlynarik et al., MRM, 56, p. 965 (2006)
    
******************************************************************/
  #include "sgl.c"

  GENERIC_GRADIENT_T       tmcrush_grad;  // crusher in TM period
  GENERIC_GRADIENT_T       endspoil_grad;  // crusher after FID is acquired
  SLICE_SELECT_GRADIENT_T  vox3_crush,vox3r_crush, vox3a_crush;

  pulsesequence()
  {
  /***** Internal variable declarations *****/
  int    shapelist1,shapelist2,shapelist3; /* pulse shapes (lists) */
 
  
  double freq1,freq2,freq3,ws_delta;
  double rprof,pprof,sprof;
  double restol, resto_local,csd_ppm;
  char profile_ovs[MAXSTR];
  char profile_vox[MAXSTR];
  int    wsfirst; //wsfirst makes ws unit to be exececuted first

  int isis;
  int counter,noph;
  char autoph[MAXSTR],pcflag[MAXSTR];
   /* sequence timing variables */
  double te_delay1, te_delay2, newdelay,tr_delay, tm_delay;
  double tau1=0, tau2=0;

   /* Extra crushers */
  double gcrushtm,tcrushtm;
  double ky;
  double vox3_cr, vox3r_cr;
  double gcrush_end, tcrush_end;

  /*extra ws pulse flag*/
  char ws_tm[MAXSTR];
  double wsflipftm;
  double tmwstpwr,tmwstpwrf;
 

  
  init_mri();
  noph=(int)getval("noph");
  isis=(int)getval("isis");
 
  int inv1[32]= {0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 1, 1, 3, 3, 3, 3, 1, 1, 1, 1, 3, 3, 3, 3};//excitation pulse	
  int inv2[32]= {0, 0, 1, 1, 2, 2, 3, 3, 0, 0, 1, 1, 2, 2, 3, 3, 1, 1, 2, 2, 3, 3, 0, 0, 1, 1, 2, 2, 3, 3, 0, 0}; //refocusing pulse
  int inv3[32]= {0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3};//inversion pulse
 
  int phrec[32]=  {0, 2, 2, 0, 2, 0, 0, 2, 0, 2, 2, 0, 2, 0, 0, 2, 1, 3, 3, 1, 3, 1, 1, 3, 1, 3, 3, 1, 3, 1, 1, 3};// rec phase
  int phrec0[32]= {0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 1, 1, 3, 3, 3, 3, 1, 1, 1, 1, 3, 3, 3, 3, 1, 1};// rec phase for non-isis
  

  /***** Real-time variables used in this sequence *****/
  int vinv1  = v1;  // on/off flag first inversion pulse
  int vms    = v5;  // dummy shapedpulselist slice counter (= one)
  
  get_ovsparameters();
  get_wsparameters();

  
  rprof = getval("rprof");
  pprof = getval("pprof");
  sprof = getval("sprof");
 
  ky=getval("ky");
 
  getstr("autoph",autoph);
  getstr("pcflag",pcflag);
  getstr("profile_ovs",profile_ovs);
   getstr("profile_vox",profile_vox);
  wsfirst=(int)getval("wsfirst");

  restol=getval("restol");   //local frequency offset
  roff=getval("roff");       //receiver offset
  csd_ppm=getval("csd_ppm"); //chemical shift displacement factor
  gcrushtm = getval("gcrushtm");
  tcrushtm = getval("tcrushtm");
  wsflipftm = getval("wsflipftm");
  getstr("ws_tm",ws_tm);
  ws_delta=getval("ws_delta");

  vox3_cr=1000000;
   
  /***** RF power calculations *****/


  
  shape_rf(&p1_rf,"p1",p1pat,p1,flip1,rof1,rof2);
  shape_rf(&p2_rf,"p2",p2pat,p2,flip2,rof1,rof2);
  shape_rf(&p3_rf,"p3",p3pat,p3,flip3,rof1,rof2);
  shape_rf(&p4_rf,"p4",p4pat,p4,flip4,rof1,rof2);

   p4_rf.flipmult=wsflipftm;

  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");
  calc_rf(&p3_rf,"tpwr3","tpwr3f");
  calc_rf(&p4_rf,"tpwr4","tpwr4f");

  
 // wsfpwrtm=p4_rf.powerFine*wsflipftm;                   /* ws fine RF power */
  

  trampfixed=trise; //rise time =trise 
  tcrush=granularity(tcrush,GRADIENT_RES); //this is to avoid the granularity errors
  //if trampfixed is used, rise time needs to be checked 
  if (trise*2>tcrush){
  
   abort_message("tcrush too short. Minimum tcrush = %fms \n",1000*trise*2);
  }

  if (gcrush>gmax){
  
   abort_message("gcrush too large. Max gcrush = %f \n",gmax*0.95);
  }
  init_slice(&vox1_grad,"vox1",vox1);
  init_slice(&vox2_grad,"vox2",vox2);
  
  init_slice_butterfly(&vox3_crush,"vox3_crush",vox3_cr,gcrush,tcrush);
  init_slice_butterfly(&vox3r_crush,"vox3r_crush",vox3_cr,gcrush,tcrush);
  
  init_slice_butterfly(&vox3_grad,"vox3",vox3,gcrush,tcrush);

  init_generic(&tmcrush_grad,"tmcrush",gcrushtm,tcrushtm); //crusher grad during tm

  if (profile_vox[0] == 'y') {
    init_readout_butterfly(&ro_grad,"ro",lro,np,sw,gcrushro,tcrushro);
    init_readout_refocus(&ror_grad,"ror");
  }

  /***** Gradient calculations *****/
  calc_slice(&vox1_grad,&p1_rf,WRITE,"vox1_grad");
  calc_slice(&vox2_grad,&p2_rf,WRITE,"vox2_grad");
  
  calc_slice(&vox3_grad,&p3_rf,NOWRITE,"");

  calc_slice(&vox3_crush,&p3_rf,WRITE,"vox3_crush");
  calc_slice(&vox3r_crush,&p3_rf,NOWRITE,"");

  vox3r_crush.crusher1Moment0 -= vox2_grad.m0ref; //only now can re-calculate the moment
  vox3r_crush.crusher1CalcFlag=AMPLITUDE_FROM_MOMENT_DURATION_RAMP;
  calc_slice(&vox3r_crush,&p3_rf,WRITE,"vox3r_crush");

  vox3_grad.crusher2Moment0 *= vox3_grad.m0def/vox3_grad.m0ref*ky; //only now can re-calculate the moment
  vox3_grad.crusher2CalcFlag=AMPLITUDE_FROM_MOMENT_DURATION_RAMP;
  calc_slice(&vox3_grad,&p3_rf,WRITE,"vox3_grad");
  
  
  calc_generic(&tmcrush_grad,WRITE,"","");
  if (profile_vox[0] == 'y') {
    calc_readout(&ro_grad,WRITE,"gro","sw","at");
    putvalue("gro",ro_grad.roamp);       // RO grad
    calc_readout_refocus(&ror_grad,&ro_grad,WRITE,"gror");
    putvalue("tror",ror_grad.duration);  // ROR duration
  }

  if (profile_ovs[0]=='y'){
     if (rprof==1) {
     vox1_grad.amp=0;
      
     }
     else if(pprof==1) {
     vox2_grad.amp=0;
   
     }     
     else if(sprof==1) {
     vox3_grad.amp=0;
    
     }
  }

  /***** Check nt is a multiple of 2 *****/
  if (ix == 1) {
    if ((int)nt%2 != 0)
      text_message("WARNING: SPECIAL requires 2 steps. Set nt as a multiple of 2\n");
  }

  /* Optional Outer Volume Suppression */
  if (ovs[0] == 'y') create_ovsbands();
  if (sat[0] == 'y') create_satbands();

  /* Optional Water Suppression */
  if (ws[0] == 'y') create_watersuppress();

 

  /***** Set up frequency offset pulse shape list *****/
  offsetlist(&pos1,vox1_grad.ssamp,0,&freq1,1,'s');
  offsetlist(&pos2,vox2_grad.ssamp,0,&freq2,1,'s');
  offsetlist(&pos3,vox3_grad.ssamp,0,&freq3,1,'s');

  if (profile_ovs[0]=='y'&& sprof==1) freq3=0.0;
  if (profile_ovs[0]=='y'&& pprof==1) freq2=0.0;
  if (profile_ovs[0]=='y'&& rprof==1) freq1=0.0;

  
  freq1=freq1-csd_ppm*sfrq;
  freq2=freq2-csd_ppm*sfrq;
  freq3=freq3-csd_ppm*sfrq;

  
  shapelist1 = shapelist(p1_rf.pulseName,vox1_grad.rfDuration,&freq1,1,vox1_grad.rfFraction,'s');
  shapelist2 = shapelist(p2_rf.pulseName,vox2_grad.rfDuration,&freq2,1,vox2_grad.rfFraction,'s');
  shapelist3 = shapelist(p3_rf.pulseName,vox3_grad.rfDuration,&freq3,1,vox3_grad.rfFraction,'s');

   /* Calculate delta from resto to include local frequency line + chemical shift offset */
  resto_local=resto-restol;  

  /* Frequency offsets */
  if (profile_vox[0] == 'y') {
    /* Shift DDR for pro ************************************/
    roff = -poffset(pro,ro_grad.roamp);
  }

  /* Set tables */
  /* Real time variables for inversion pulses */
  settable(t1,noph,inv1);
  settable(t2,noph,inv2);
  settable(t3,noph,inv3);
  /* Phase cycle for excitation pulse and receiver */
  if (isis!=1) settable(t4,noph,phrec0);
  else settable(t4,noph,phrec);
  /* shapedpulselist variable */
  assign(one,vms);

 /* Put gradient information back into VnmrJ parameters */
  putvalue("gvox1",vox1_grad.ssamp);
  putvalue("gvox2",vox2_grad.ssamp);
  putvalue("gvox3",vox3_grad.ssamp);
  putvalue("rgvox1",vox1_grad.tramp);
  putvalue("rgvox2",vox2_grad.tramp);
  putvalue("rgvox3",vox3_grad.tramp);

  sgl_error_check(sglerror);
  if (ss<0) g_setExpTime(trmean*(nt-ss)*arraydim);
  else g_setExpTime(tr*(ntmean*arraydim+ss));

  /* PULSE SEQUENCE *************************************/
  /* Real time variables for inversion pulses */
 
  counter=(double)nt*(ix-1);
  if (autoph[0] == 'n') counter=0.0;
  
  initval(counter,v11);
  initval(noph,v13); //v13=number of phase cycling steps
  add(v11,ctss,v12);   //v12=counter
  modn(v12,v13,v12); //v12 runs from 1:v13 
  getelem(t1,v12,v8); /* 90 DEG. SPIN ECHO PULSE */
  getelem(t2,v12,v9); /* 180 DEG. SPIN ECHO P.   */
  getelem(t3,v12,v10); /* ISIS 180 DEG. ADIAB. PULSE */
  getelem(t4,v12,oph);  /*RCVR PHASE*/
  mod2(v12,vinv1); // this controls 1D isis on, off, on, of... up to noph(=32)

   /****************************************************/
  /* Sequence Timing **********************************/
  /****************************************************/
  /*  Min TE ******************************************/
   
  tau1 = vox2_grad.rfCenterBack + vox3_grad.rfCenterFront;
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

  printf("te delay1 is %f", te_delay1);
  printf("te delay2 is %f", te_delay2);


  /***************************************************/
  /* Min TM ******************************************/   	
  if (ws_tm[0] == 'y') {	
  tau1  = vox1_grad.rfCenterBack + rof1+rof2+p4_rf.rfDuration+tmcrush_grad.duration + 4e-6 + vox2_grad.rfCenterFront;
  }
  else tau1  = vox1_grad.rfCenterBack + rof2+tmcrush_grad.duration + 4e-6 + vox2_grad.rfCenterFront;

  tmmin = tau1 + 4e-6;  /* have at least 4us between gradient events */

  if (mintm[0] == 'y') {
    tm = tmmin;
    putvalue("tm",tm);
  }
  else if (tm < tmmin) {
    abort_message("TM too short.  Minimum TM = %.2fms\n",tmmin*1000);   
  }
  tm_delay = (tm - tau1);


  
  /* Relaxation delay ***********************************/
   /***** Min TR *****/
  trmin = vox1_grad.rfCenterFront + tm + te+alfa + at + 20e-6;
  if (profile_vox[0] == 'y') trmin += ror_grad.duration + ro_grad.duration - at; 
  if (ws[0]  == 'y') trmin += wsTime;
  if (ovs[0] == 'y') trmin += ovsTime;
  if (sat[0] == 'y') trmin += satTime;

  if (mintr[0] == 'y') {
    tr = trmin;  
    putCmd("setvalue('tr',%f,'current')\n",tr);
  }
   if ((trmin-tr) > 12.5e-9) {
    abort_message("tr too short. Minimum tr = %.2f ms\n",(trmin)*1000);
  }

  /***** Calculate TR delay *****/
  tr_delay = tr - trmin;

  /**Sequence Begin**/
  status(A);
  obsoffset(resto_local);
  delay(4e-6);
  set_rotation_matrix(vpsi,vphi,vtheta);

  if (ticks > 0) {
    xgate(ticks);
    grad_advance(gpropdelay);
    delay(4e-6);
  }

  /* TTL scope trigger **********************************/
  sp1on(); delay(4e-6); sp1off();

  
  
  /* Saturation bands ***********************************/
  if (ovs[0] == 'y') ovsbands();
  if (sat[0] == 'y') satbands();

  /* Post OVS water suppression *************************/
  if (ws[0] == 'y')  watersuppress();

  /* First inversion pulse *****/
  if (isis >= 1){ /*for the ISIS pulse on,off,on,off... or on,on,on,on... */

   obspower(p1_rf.powerCoarse);
   obspwrf(p1_rf.powerFine);
   delay(4e-6);
    if (isis == 1) /* for ISIS on,off,on,off,...  */{
    ifzero(vinv1);
      obl_shapedgradient(vox1_grad.name,vox1_grad.duration,vox1_grad.amp,0,0,NOWAIT);
      delay(vox1_grad.rfDelayFront);
      shapedpulselist(shapelist1,vox1_grad.rfDuration,v10,rof1,rof2,'s',vms);
      delay(vox1_grad.rfDelayBack);
    elsenz(vinv1);
      obl_shapedgradient(vox1_grad.name,vox1_grad.duration,vox1_grad.amp,0,0,WAIT);
    endif(vinv1);
    }
    else {  /* for ISIS on,on,on,on...*/
      obl_shapedgradient(vox1_grad.name,vox1_grad.duration,vox1_grad.amp,0,0,NOWAIT);
      delay(vox1_grad.rfDelayFront);
      shapedpulselist(shapelist1,vox1_grad.rfDuration,v10,rof1,rof2,'s',vms);
      delay(vox1_grad.rfDelayBack);
    }  
    
    
  }
  else delay(vox1_grad.duration); //this is for isis off,off,off,off  


  /* tm delay before excitation pulse *****/
  /* Optional TM water suppression ***********************/
   if (ws_tm[0] == 'y') {
   
    if (wsrf[0]=='y') {
    obspower(p4_rf.powerCoarse);
    obspwrf(p4_rf.powerFine);
    delay(4e-6);
    shapedpulseoffset(p4_rf.pulseName,p4_rf.rfDuration,zero,rof1,rof2,ws_delta);
    }
    else delay(p4_rf.rfDuration+rof1+rof2);
  }  //end of ws_tm='y' condition
  
    delay(tm_delay);

  /* TM Gradient crusher ********************************/
  obl_shapedgradient(tmcrush_grad.name,tmcrush_grad.duration,0,0,tmcrush_grad.amp,WAIT);
  
  /* 90 degree excitation pulse *****/
  obspower(p2_rf.powerCoarse);
  obspwrf(p2_rf.powerFine);
  delay(4e-6);
  obl_shapedgradient(vox2_grad.name,vox2_grad.duration,0,vox2_grad.amp,0,NOWAIT);
  delay(vox2_grad.rfDelayFront);
  shapedpulselist(shapelist2,vox2_grad.rfDuration,v8,rof1,rof2,'s',vms);
  delay(vox2_grad.rfDelayBack);
  delay(te_delay1);
  /* 180 degree pulse ********************************/
  obspower(p3_rf.powerCoarse);  
  obspwrf(p3_rf.powerFine);
  delay(4e-6);
  obl_shaped3gradient   (vox3_crush.name,vox3r_crush.name,vox3_grad.name,vox3_grad.duration,vox3_crush.amp,vox3r_crush.amp,vox3_grad.amp,NOWAIT);   
  delay(vox3_grad.rfDelayFront);
 
  
  shapedpulselist(shapelist3,vox3_grad.rfDuration,v9,rof1,rof2,'s',vms);
  delay(vox3_grad.rfDelayBack);
  delay(te_delay2);

  //acquisition starts

  if (profile_vox[0] == 'y') {
    obl_shapedgradient(ror_grad.name,ror_grad.duration,
      -rprof*ror_grad.amp,-pprof*ror_grad.amp,-sprof*ror_grad.amp,WAIT);
    delay(4e-6);
    obl_shapedgradient(ro_grad.name,ro_grad.duration,
      rprof*ro_grad.amp,pprof*ro_grad.amp,sprof*ro_grad.amp,NOWAIT); 
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

  delay(tr_delay);

}
