/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************************
Localization by Adiabatic Selective Refocusing (LASER)
Garwood et al, JMR 72 (1987), p.502-508
Version 2/15/09 (+SGL libraries) 
************************************************************************/
/**Standard include files**********************************************/
#include <standard.h>
#include "sgl.c"

  /*voxel crusher gradient structs for butterfly grad without ss */
  SLICE_SELECT_GRADIENT_T  vox1_crush,vox2_crush, vox3_crush;

pulsesequence()
{

  /* Internal variable declarations *************************/

  /*timing*/
  double tr_delay;
  double te_d1,te_d2,te_d3;             /* delays */
  double tau1,tau2,tau3;
  
  
  /*voxel crusher multipliers */
  double fx,fy,fz;
  
  /*localization parameters*/
  double freq1,freq2,freq3;
  double vox1_cr,vox2_cr, vox3_cr;
  int nDim;
 

  double rprof,pprof,sprof;
  char profile_vox[MAXSTR],profile_ovs[MAXSTR];

  double restol, resto_local, csd_ppm;

  /*phase cycle****/
  int counter,noph;
  char autoph[MAXSTR], pcflag[MAXSTR];
  int rf1_phase[64]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}; 
  int rf2_phase[64]  = {0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,
			0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3}; 
  int rf3_phase[64]  = {0,2,1,3,0,2,1,3,0,2,1,3,0,2,1,3,0,2,1,3,0,2,1,3,0,2,1,3,0,2,1,3,
			0,2,1,3,0,2,1,3,0,2,1,3,0,2,1,3,0,2,1,3,0,2,1,3,0,2,1,3,0,2,1,3}; 
 
 
  /* Initialize paramaters **********************************/
  init_mri();  //this gets all the parameters that are defined in acqparms.h, etc
  get_wsparameters();
  get_ovsparameters();

  rprof = getval("rprof");
  pprof = getval("pprof");
  sprof = getval("sprof");

  //read the crusher factors that are designed to create grad on the same axis without refoc 
  fx=getval("fx");
  fy=getval("fy");
  fz=getval("fz");

  
  getstr("profile_vox",profile_vox);
  getstr("profile_ovs",profile_ovs);

  /*set voxel sizes for butterfly crushers to 10^6 to set the slice portion to zero ***/
  vox1_cr=1000000;
  vox2_cr=1000000;
  vox3_cr=1000000;
  
  /***** RF power initialize *****/
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);
  init_rf(&p2_rf,p2pat,p2,flip2,rof1,rof2);

 

  
  
  /***** Initialize gradient structs *****/
  trampfixed=trise; //rise time =trise 
  tcrush=granularity(tcrush,GRADIENT_RES); //this is to avoid the granularity errors
  //if trampfixed is used, rise time needs to be checked 
  if (trise*2>tcrush){
  
   abort_message("tcrush too short. Minimum tcrush = %fms \n",1000*trise*2);
  }

  if (gcrush>gmax){
  
   abort_message("gcrush too large. Max gcrush = %f \n",gmax*0.95);
  }

  init_slice_butterfly(&vox1_grad,"vox1",vox1,gcrush,tcrush);
  init_slice_butterfly(&vox2_grad,"vox2",vox2,gcrush,tcrush);
  init_slice_butterfly(&vox3_grad,"vox3",vox3,gcrush,tcrush);

  init_slice_butterfly(&vox1_crush,"vox1_crush",vox1_cr,gcrush,tcrush);
  init_slice_butterfly(&vox2_crush,"vox2_crush",vox2_cr,gcrush,tcrush);
  init_slice_butterfly(&vox3_crush,"vox3_crush",vox3_cr,gcrush,tcrush); 
  if (profile_vox[0] == 'y') {
    init_readout_butterfly(&ro_grad,"ro",lro,np,sw,gcrushro,tcrushro);
    init_readout_refocus(&ror_grad,"ror");
  }
 
  /***** RF and Gradient calculations *****/
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");
  
  calc_slice(&vox1_grad,&p2_rf,WRITE,"gvox1");
  calc_slice(&vox2_grad,&p2_rf,WRITE,"gvox2");
  calc_slice(&vox3_grad,&p2_rf,WRITE,"gvox3");

  calc_slice(&vox1_crush,&p2_rf,WRITE,"vox1_crush");
  calc_slice(&vox2_crush,&p2_rf,WRITE,"vox2_crush");
  calc_slice(&vox3_crush,&p2_rf,WRITE,"vox3_crush");

  if (profile_vox[0] == 'y') {
    calc_readout(&ro_grad,WRITE,"gro","sw","at");
    putvalue("gro",ro_grad.roamp);       // RO grad
    calc_readout_refocus(&ror_grad,&ro_grad,WRITE,"gror");
    putvalue("tror",ror_grad.duration);  // ROR duration
  }

  //set all gradients along a particular direction to zero if profile is needed

  if (profile_ovs[0]=='y'){
     if (rprof==1) {
       vox1_grad.amp=0; //set slice selection in read direction to none 
       vox3_crush.amp=0; // set corresponding crusher gradients to none
       
     }
     else if(pprof==1) {
     vox2_grad.amp=0;
     vox1_crush.amp=0;
     }     
     else if(sprof==1) {
     vox3_grad.amp=0;
     vox2_crush.amp=0;
     }
  }


  

  /* Optional OVS and Water Suppression */
  
  if (ovs[0] == 'y')  create_ovsbands();
  if (sat[0] == 'y')  create_satbands();
  if (ws[0]  == 'y')  create_watersuppress();

  //Read in parameters not defined in acqparms.h and sglHelper 
  nDim=getval("nDim");
  restol=getval("restol");  //local frequency offset 
  roff=getval("roff");       //receiver offset
  csd_ppm=getval("csd_ppm"); //chemical shift displacement factor
  
  noph=getval("noph");
  getstr("autoph",autoph);
  getstr("pcflag",pcflag);
  settable(t3,noph,rf1_phase);
  settable(t2,noph,rf2_phase);
  settable(t1,noph,rf3_phase);

  /* tau1, tau2 and tau3 are sums of all events in TE*/
  tau1 = vox1_grad.rfCenterFront+GDELAY+rof2;
  tau2 = vox1_grad.rfCenterBack + vox1_grad.rfCenterFront+2*(GDELAY+rof2);
  tau3 = vox3_grad.rfCenterBack+GDELAY+rof2;
  temin  = tau1+5.0*tau2+tau3;  

  if (minte[0] == 'y') {
   
    te = temin;
    putvalue("te",te);
   }
  if (te < temin) {
    abort_message("te too short. Minimum te = %.2f ms\n",temin*1000);
  }
  

  /***** Calculate TE delays *****/
  te_d1 = te/12.0 - tau1+GDELAY;
  te_d2 = te/6.0 - tau2+2*(GDELAY+rof2);
  te_d3 = te/12.0 - tau3+GDELAY+rof2;

    //Calculate delta from resto to include local frequency line+ chemical shift offset
  resto_local=resto-restol;  


/***** Min TR *****/
  trmin = GDELAY + p1 + te + at+rof1+rof2;

  if (ws[0]  == 'y') trmin += wsTime;
  if (ovs[0] == 'y') trmin += ovsTime;
  if (sat[0] == 'y') trmin += satTime;
  if (profile_vox[0] == 'y') trmin += ror_grad.duration + ro_grad.duration - at; 

  if (mintr[0] == 'y') {
    tr = trmin;  // ensure at least 4us between gradient events
    putvalue("tr",tr);
  }
  if ((trmin-tr) > 12.5e-9) {
    abort_message("TR too short.  Minimum TR= %.2fms\n",trmin*1000);
  }
/***** Calculate TR delay *****/
  tr_delay = tr - trmin;

/* Frequency offsets */
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
  
  
  
  sgl_error_check(sglerror);
  
  if (ss<0) g_setExpTime(tr*(nt-ss)*arraydim);
  else g_setExpTime(tr*(nt*arraydim+ss));

/**[2.7] PHASE CYCLING ******************************************************/

  assign(zero, oph); 
  counter=(double)nt*(ix-1);
  if (autoph[0] == 'n') counter=0.0; //only goes through nt, if 'y' goes through nt*array
  initval(counter,v1);
  initval(noph,v3);
  add(v1,ct,v2);
  modn(v2,v3,v2);
  
  /* Full phase cycling requires 64 steps*/
  
  if (pcflag[0] == 'n') {
  assign(zero,v2);
  getelem(t1,v2,v10);
  getelem(t2,v2,v11);
  getelem(t3,v2,v12); 
  }
  else
  {
  getelem(t1,v2,v10);
  getelem(t2,v2,v11);
  getelem(t3,v2,v12); 
  }
  

 
 
  /*Start of the sequence*/
  obsoffset(resto_local);  // need it here for water suppression to work
  delay(GDELAY);
  rot_angle(vpsi,vphi,vtheta);

  if (ticks) {
    xgate(ticks);
    grad_advance(gpropdelay);
    delay(4e-6);
  }

  /* TTL scope trigger **********************************/
  //sp1on(); delay(4e-6); sp1off();

  /* Saturation bands ***********************************/
  
  
  if (ovs[0] == 'y') ovsbands();
  if (sat[0] == 'y') satbands();

  /* Water suppression **********************************/
  if (ws[0]  == 'y') watersuppress();

  /* Slice selective 90 degree RF pulse *****/
  obspower(p1_rf.powerCoarse);
  obspwrf(p1_rf.powerFine);
  delay(GDELAY);
  
  shaped_pulse(p1pat,p1,zero,rof1,rof2);

   /* start localization */
  obspower(p2_rf.powerCoarse);
  obspwrf(p2_rf.powerFine);
  
  if (nDim > 2.5) {
  
  delay(te_d1);   //this is at least GDELAY == 4 us
  
  obl_shaped3gradient(vox1_grad.name,vox1_crush.name,"",vox1_grad.duration,vox1_grad.amp,fy*vox1_crush.amp,0,NOWAIT);
  delay(vox1_grad.rfDelayFront);
  if (profile_ovs[0]=='y'&& rprof==1) freq1=0.0;
  shapedpulseoffset(p2_rf.pulseName,vox1_grad.rfDuration,v12,rof1,rof2,freq1);
  delay(vox1_grad.rfDelayBack);  
  delay(te_d2);
  obl_shaped3gradient (vox1_grad.name,vox1_crush.name,"",vox1_grad.duration,vox1_grad.amp,fy*0.777*vox1_crush.amp,0,NOWAIT);
  delay(vox1_grad.rfDelayFront);
  if (profile_ovs[0]=='y'&& rprof==1) freq1=0.0;
  shapedpulseoffset(p2_rf.pulseName,vox1_grad.rfDuration,v12,rof1,rof2,freq1);
  delay(vox1_grad.rfDelayBack);
  
  delay(te_d2);
  }
  
  if (nDim > 1.5) {   //this is 2nd slice selection
  obl_shaped3gradient("",vox2_grad.name,vox2_crush.name,vox2_grad.duration,0,vox2_grad.amp,fz*vox2_crush.amp,NOWAIT);
  delay(vox2_grad.rfDelayFront);
  if (profile_ovs[0]=='y'&& pprof==1) freq2=0.0;
  shapedpulseoffset(p2_rf.pulseName,vox2_grad.rfDuration,v11,rof1,rof2,freq2);
  delay(vox2_grad.rfDelayBack);
  
  delay(te_d2);
  
  obl_shaped3gradient("",vox2_grad.name,vox2_crush.name,vox2_grad.duration,0,vox2_grad.amp,fz*0.777*vox2_crush.amp,NOWAIT);
  delay(vox2_grad.rfDelayFront);
  if (profile_ovs[0]=='y'&& pprof==1) freq2=0.0;
  shapedpulseoffset(p2_rf.pulseName,vox2_grad.rfDuration,v11,rof1,rof2,freq2);
  delay(vox2_grad.rfDelayBack);
  
  delay(te_d2);
  }

  if (nDim > 0.5){    //this is 3rd slice selection
  obl_shaped3gradient(vox3_crush.name,"",vox3_grad.name,vox3_grad.duration,fx*vox3_crush.amp,0,vox3_grad.amp,NOWAIT);
  delay(vox3_grad.rfDelayFront);
  if (profile_ovs[0]=='y'&& sprof==1) freq3=0.0;
  shapedpulseoffset(p2_rf.pulseName,vox3_grad.rfDuration,v10,rof1,rof2,freq3);
  delay(vox3_grad.rfDelayBack);
  
  delay(te_d2);
   obl_shaped3gradient(vox3_crush.name,"",vox3_grad.name,vox3_grad.duration,fx*vox3_crush.amp,0,vox3_grad.amp,NOWAIT);
  delay(vox3_grad.rfDelayFront);
  if (profile_ovs[0]=='y'&& sprof==1) freq3=0.0;
  shapedpulseoffset(p2_rf.pulseName,vox3_grad.rfDuration,v10,rof1,rof2,freq3);
  delay(vox3_grad.rfDelayBack);
  
  delay(te_d3);
  }
  if (profile_vox[0] == 'y') {
    obl_shapedgradient(ror_grad.name,ror_grad.duration,
      -rprof*ror_grad.amp,-pprof*ror_grad.amp,-sprof*ror_grad.amp,WAIT);
    delay(GDELAY);
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
  } //end of the pulsesequence
