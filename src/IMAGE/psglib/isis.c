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
    ISIS.C

    Volume localized spectroscopy using ISIS

    Ref: R.J. Ordidge, J. Magn. Reson., 66, 283, (1086).
    
******************************************************************/
#include "sgl.c"

void pulsesequence()
{
  /***** Internal variable declarations *****/
  int    shapelist1,shapelist2,shapelist3; /* pulse shapes (lists) */
  double tr_delay;
  double tidel;
  double freq1,freq2,freq3;
  double rprof,pprof,sprof;
  double restol, resto_local;
  int    wsfirst;
  char profile_vox[MAXSTR];
  char profile_ovs[MAXSTR];

  /********************
    ISIS cycle order, and excitation/receiver phase cycles
    Ref:  C. Burger, Magn. Reson. Med., 26, 218 (1992)

    scan:       ct =   0     1     2     3     4     5     6     7

    pulse 1:  inv1 =   -    180    -    180    -    180    -    180
    pulse 2:  inv2 =   -    180   180    -     -    180   180    - 
    pulse 3:  inv3 =   -    180   180    -    180    -     -    180

    pulse 4:  phex =   90   180   270    0    270    0     90   180
    rcvr:      oph =  270   180    90    0    270   180    90    0 
  ********************/
  int inv1[8]  = {1,0,1,0,1,0,1,0};  // inversion flag, 0=on
  int inv2[8]  = {1,0,0,1,1,0,0,1};  // inversion flag, 0=on
  int inv3[8]  = {1,0,0,1,0,1,1,0};  // inversion flag, 0=on
  int phex[8]  = {1,2,3,0,3,0,1,2};  // excitation phase
  int phrec[8] = {3,2,1,0,3,2,1,0};  // receiver phase

  /***** Real-time variables used in this sequence *****/
  int vinv1  = v1;  // on/off flag first inversion pulse
  int vinv2  = v2;  // on/off flag second inversion pulse
  int vinv3  = v3;  // on/off flag third inversion pulse
  int vphex  = v4;  // phase of excitation pulse
  int vms    = v5;  // dummy shapedpulselist slice counter (= one)
  int vindex = v6;  // table index
  int veight = v7;  // holds number eight
  
  init_mri();
  get_ovsparameters();
  get_wsparameters();

  tidel = getval("tidel");
  rprof = getval("rprof");
  pprof = getval("pprof");
  sprof = getval("sprof");
  getstr("profile_vox",profile_vox);
  getstr("profile_ovs",profile_ovs);
  wsfirst=(int)getval("wsfirst");

  restol=getval("restol");   //local frequency offset
  roff=getval("roff");       //receiver offset

  tcrush=granularity(tcrush,GRADIENT_RES); //this is to avoid the granularity errors
  

  if (gcrush>gmax){
  
   abort_message("gcrush too large. Max gcrush = %f \n",gmax*0.95);
  }

  /***** RF power calculations *****/
  shape_rf(&p1_rf,"p1",p1pat,p1,flip1,rof1,rof2);
  shape_rf(&p2_rf,"p2",p2pat,p2,flip2,rof1,rof2);
  shape_rf(&p3_rf,"p3",p3pat,p3,flip3,rof1,rof2);
  shape_rf(&p4_rf,"p4",p4pat,p4,flip4,rof1,rof2);
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");
  calc_rf(&p3_rf,"tpwr3","tpwr3f");
  calc_rf(&p4_rf,"tpwr4","tpwr4f");

  /***** Initialize gradient structs *****/
  init_slice_butterfly(&vox1_grad,"vox1",vox1,gcrush,tcrush);
  init_slice_butterfly(&vox2_grad,"vox2",vox2,gcrush,tcrush);
  init_slice_butterfly(&vox3_grad,"vox3",vox3,gcrush,tcrush);
  if (profile_vox[0] == 'y') {
    init_readout_butterfly(&ro_grad,"ro",lro,np,sw,gcrushro,tcrushro);
    init_readout_refocus(&ror_grad,"ror");
  }

  /***** Gradient calculations *****/
  calc_slice(&vox1_grad,&p1_rf,WRITE,"");
  calc_slice(&vox2_grad,&p2_rf,WRITE,"");
  calc_slice(&vox3_grad,&p3_rf,WRITE,"");
  if (profile_vox[0] == 'y') {
    calc_readout(&ro_grad,WRITE,"gro","sw","at");
    putvalue("gro",ro_grad.roamp);       // RO grad
    calc_readout_refocus(&ror_grad,&ro_grad,WRITE,"gror");
    putvalue("tror",ror_grad.duration);  // ROR duration
  }

  /***** Check nt is a multiple of eight *****/
  if (ix == 1) {
    if ((int)nt%8 != 0)
      text_message("WARNING: ISIS requires 8 steps. Set nt as a multiple of 8\n");
  }

  if (profile_ovs[0]=='y'){
     if (rprof==1) {
       vox1_grad.amp=0; //set slice selection in read direction to none 
       
       
     }
     else if(pprof==1) {
     vox2_grad.amp=0;
     
     }     
     else if(sprof==1) {
     vox3_grad.amp=0;
     
     }
  }

  /* Optional Outer Volume Suppression */
  if (ovs[0] == 'y') create_ovsbands();
  if (sat[0] == 'y') create_satbands();

  /* Optional Water Suppression */
  if (ws[0] == 'y') create_watersuppress();

  /***** Min TR *****/
  trmin = vox1_grad.duration + vox2_grad.duration + vox3_grad.duration 
          + 2.0*tidel + tm + p4 + alfa + at + 20e-6;
  if (profile_vox[0] == 'y') trmin += ror_grad.duration + ro_grad.duration - at; 
  if (ws[0]  == 'y') trmin += wsTime;
  if (ovs[0] == 'y') trmin += ovsTime;
  if (sat[0] == 'y') trmin += satTime;

  if (mintr[0] == 'y') {
    tr = trmin+4e-6;  // ensure at least 4us between gradient events;
    putCmd("setvalue('tr',%f,'current')\n",tr);
  }
  if (tr < trmin+4e-6) {
    abort_message("tr too short. Minimum tr = %.2f ms\n",(trmin+4e-6)*1000);
  }

  /***** Calculate TR delay *****/
  tr_delay = tr - trmin;

  /***** Set up frequency offset pulse shape list *****/
  if (profile_ovs[0]=='y'&& sprof==1) freq3=0.0;
  if (profile_ovs[0]=='y'&& pprof==1) freq2=0.0;
  if (profile_ovs[0]=='y'&& rprof==1) freq1=0.0;
  offsetlist(&pos1,vox1_grad.ssamp,0,&freq1,1,'s');
  offsetlist(&pos2,vox2_grad.ssamp,0,&freq2,1,'s');
  offsetlist(&pos3,vox3_grad.ssamp,0,&freq3,1,'s');
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
  settable(t1,8,inv1);
  settable(t2,8,inv2);
  settable(t3,8,inv3);
  /* Phase cycle for excitation pulse and receiver */
  settable(t4,8,phex);
  settable(t5,8,phrec);

  /* Initialise veight */
  initval(8,veight);

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
  modn(ctss,veight,vindex);
  //modn(ct,veight,vindex);

  /* Real time variables for inversion pulses */
  getelem(t1,vindex,vinv1);
  getelem(t2,vindex,vinv2);
  getelem(t3,vindex,vinv3);

  /* Phase cycle for excitation pulse and receiver */
  getelem(t4,vindex,vphex);
  getelem(t5,vindex,oph);
  
  /* Relaxation delay ***********************************/
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

  /* Pre OVS water suppression **************************/
  if ((ws[0] == 'y') && wsfirst) watersuppress();
  delay(4e-6);
  
  /* Saturation bands ***********************************/
  if (ovs[0] == 'y') ovsbands();
  if (sat[0] == 'y') satbands();

  /* Post OVS water suppression *************************/
  if ((ws[0] == 'y') && !wsfirst)  watersuppress();

  /* First inversion pulse *****/
  obspower(p1_rf.powerCoarse);
  obspwrf(p1_rf.powerFine);
  delay(4e-6);
  ifzero(vinv1);
    obl_shapedgradient(vox1_grad.name,vox1_grad.duration,vox1_grad.amp,0,0,NOWAIT);
    delay(vox1_grad.rfDelayFront);
    shapedpulselist(shapelist1,vox1_grad.rfDuration,zero,rof1,rof2,'s',vms);
    delay(vox1_grad.rfDelayBack);
  elsenz(vinv1);
    obl_shapedgradient(vox1_grad.name,vox1_grad.duration,vox1_grad.amp,0,0,WAIT);
  endif(vinv1);
  delay(tidel);

  /* Second inversion pulse *****/
  obspower(p2_rf.powerCoarse);
  obspwrf(p2_rf.powerFine);
  delay(4e-6);
  ifzero(vinv2);
    obl_shapedgradient(vox2_grad.name,vox2_grad.duration,0,vox2_grad.amp,0,NOWAIT);
    delay(vox2_grad.rfDelayFront);
    shapedpulselist(shapelist2,vox2_grad.rfDuration,zero,rof1,rof2,'s',vms);
    delay(vox2_grad.rfDelayBack);
  elsenz(vinv2);
    obl_shapedgradient(vox2_grad.name,vox2_grad.duration,0,vox2_grad.amp,0,WAIT);
  endif(vinv2);
  delay(tidel);

  /* Third inversion pulse *****/
  obspower(p3_rf.powerCoarse);
  obspwrf(p3_rf.powerFine);
  delay(4e-6);
  ifzero(vinv3);
    obl_shapedgradient(vox3_grad.name,vox3_grad.duration,0,0,vox3_grad.amp,NOWAIT);
    delay(vox3_grad.rfDelayFront);
    shapedpulselist(shapelist3,vox3_grad.rfDuration,zero,rof1,rof2,'s',vms);
    delay(vox3_grad.rfDelayBack);
  elsenz(vinv3);
    obl_shapedgradient(vox3_grad.name,vox3_grad.duration,0,0,vox3_grad.amp,WAIT);
  endif(vinv3);

  /* tm delay before excitation pulse *****/
  delay(tm);
  
  /* 90 degree excitation pulse *****/
  obspower(p4_rf.powerCoarse);
  obspwrf(p4_rf.powerFine);
  delay(4e-6);
  shapedpulseoffset(p4_rf.pulseName,p4_rf.rfDuration,vphex,rof1,rof1,0.0);
  delay(4e-6);
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
