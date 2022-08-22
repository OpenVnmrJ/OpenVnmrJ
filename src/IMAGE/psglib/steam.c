/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************
    STEAM.C

    Volume localized spectroscopy using the stimulated echo
    technique, with optional water suppression.
    CHESS, WET, VAPOR - water suppression
************************************************************/

#include <standard.h>
#include "sgl.c"

GENERIC_GRADIENT_T       tmcrush_grad;  // crusher in TM period

/* Phases for phase cycling */
int counter,noph;
static int ph1[8]   = {0, 2, 0, 2,  0, 2, 0, 2};
static int ph2[8]   = {0, 0, 2, 2,  0, 0, 2, 2};
static int ph3[8]   = {0, 0, 0, 0,  2, 2, 2, 2};
static int phobs[8] = {0, 2, 2, 0,  2, 0, 0, 2};

char autoph[MAXSTR], pcflag[MAXSTR];

//extern RF_PULSE_T ws_rftm;

RF_PULSE_T    ws_rftm;              /* RF pulse structure */


void pulsesequence()
{
  /* INTERNAL VARIABLE DECLARATIONS *********************/
  double freq1,freq2,freq3,ws_delta;

  /* sequence timing variables */
  double te_delay1, te_delay2, tr_delay, tm_delay;
  double tau1=0, tau2=0;

  /*frequency and profile*/  
  double rprof,pprof,sprof;
  double restol, resto_local, csd_ppm;
  char profile_vox[MAXSTR];
  char profile_ovs[MAXSTR];
  
  /* Extra crushers */
  double gcrushtm, tcrushtm;

 /*extra ws pulse flag*/
 char ws_tm[MAXSTR];
 double wsflipftm;
    
  /* Real-time variables used in this sequence **************/
  int    vph1 =v1;  // Phase of first RF pulse  
  int    vph2 =v2;   // Phase of second RF pulse  
  int    vph3 =v3;   // Phase of third RF pulse  

  /* Initialize paramaters **********************************/
  init_mri();
  get_ovsparameters();
  get_wsparameters();

  //Read in parameters not defined in acqparms.h and sglHelper 
 
  restol=getval("restol");  //local frequency offset 
  csd_ppm=getval("csd_ppm"); //chemical shift displacement factor
  roff=getval("roff");       //receiver offset

  gcrushtm = getval("gcrushtm");
  tcrushtm = getval("tcrushtm");
  getstr("ws_tm",ws_tm);
  
  rprof = getval("rprof");
  pprof = getval("pprof");
  sprof = getval("sprof");
  ws_delta=getval("ws_delta");
  wsflipftm=getval("wsflipftm");
  getstr("profile_vox",profile_vox);
  getstr("profile_ovs",profile_ovs);
  

  

  /* Initialize gradient structures   ***************/
  init_rf(&p1_rf,p1pat,p1,flip1,rof2,rof2);
  init_rf(&ws_rftm,p4pat,p4,flip1,rof2,rof2);

  tcrush=granularity(tcrush,GRADIENT_RES); //this is to avoid the granularity errors
  

  if (gcrush>gmax){
  
   abort_message("gcrush too large. Max gcrush = %f \n",gmax*0.95);
  }
  

  init_slice(&vox1_grad,"vox1",vox1);
  init_slice_butterfly(&vox2_grad,"vox2",vox2,gcrush,tcrush);
  init_slice_butterfly(&vox3_grad,"vox3",vox3,gcrush,tcrush);
  init_slice_refocus(&vox1r_grad, "vox1r");
  init_slice_refocus(&vox2r_grad, "vox3r");
  init_slice_refocus(&vox3r_grad, "vox3r");
  if (profile_vox[0] == 'y') {
    init_readout_butterfly(&ro_grad,"ro",lro,np,sw,gcrushro,tcrushro);
    init_readout_refocus(&ror_grad,"ror");
  }

  /* Crushers used in this sequence:
     TE period:   vox2/3_grad  : gcrush,  tcrush
     TM period:   tmcrush_grad : gcrushtm, tcrushtm
  */
  init_generic(&tmcrush_grad,"tmcrush",gcrushtm,tcrushtm);

  /* Calculate RF */
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  /* We are using the same RF for all 3 pulses, so it must be symmetric */
  if (p1_rf.header.rfFraction != 0.5)
    abort_message("Use symmetric RF pulse");

    ws_rftm.flipmult=wsflipftm;
    calc_rf(&ws_rftm,"wstpwr","wstpwrf");




  /* Calculate all gradients */
  calc_slice(&vox1_grad,&p1_rf,WRITE,"gvox1");
  calc_slice(&vox2_grad,&p1_rf,WRITE,"gvox2");
  calc_slice(&vox3_grad,&p1_rf,WRITE,"gvox3");
  calc_slice_refocus(&vox1r_grad,&vox1_grad,NOWRITE,"gvox1r");
  calc_slice_refocus(&vox2r_grad,&vox2_grad,NOWRITE,"gvox2r");
  calc_slice_refocus(&vox3r_grad,&vox3_grad,NOWRITE,"gvox3r");
  calc_generic(&tmcrush_grad,WRITE,"","");
  if (profile_vox[0] == 'y') {
    calc_readout(&ro_grad,WRITE,"gro","sw","at");
    putvalue("gro",ro_grad.roamp);       // RO grad
    calc_readout_refocus(&ror_grad,&ro_grad,WRITE,"gror");
    putvalue("tror",ror_grad.duration);  // ROR duration
  }

  /* Equalize Refocusing Gradients  ********************/
  calc_sim_gradient(&vox1r_grad, &vox2r_grad, &vox3r_grad, 0, WRITE); 

  //enable the entire profile
  if (profile_ovs[0]=='y'){
     if (rprof==1) {
     vox1_grad.amp=0;
     vox1r_grad.amp=0;
      
     }
     else if(pprof==1) {
     vox2_grad.amp=0;
     vox2r_grad.amp=0;
   
     }     
     else if(sprof==1) {
     vox3_grad.amp=0;
     vox3r_grad.amp=0;
    
     }
  }

  /* Optional Outer Volume Suppression */
  /* Optional Outer Volume Suppression */
  if (ovs[0] == 'y')  create_ovsbands();
  if (ws[0]  == 'y')  create_watersuppress();
  if (sat[0] == 'y')  create_satbands();


  /****************************************************/
  /* Sequence Timing **********************************/
  /****************************************************/
  /*  Min TE ******************************************/
  tau1 = vox1_grad.rfCenterBack + vox2_grad.rfCenterFront;
  tau2 = vox3_grad.rfCenterBack + vox1r_grad.duration;

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


  /***************************************************/
  /* Min TM ******************************************/   	
  tau1  = vox2_grad.rfCenterBack + tmcrush_grad.duration + 4e-6 + vox3_grad.rfCenterFront;
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
  tau1 = 4e-6 + vox1_grad.rfCenterFront + te + tm + at;

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


  if (profile_ovs[0]=='y'&& sprof==1) freq3=0.0;
  if (profile_ovs[0]=='y'&& pprof==1) freq2=0.0;
  if (profile_ovs[0]=='y'&& rprof==1) freq1=0.0;
  


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
  putvalue("tvox1",vox1r_grad.duration);
  putvalue("tvox2",vox2r_grad.duration);
  putvalue("tvox3",vox3r_grad.duration);
  putvalue("rgvox1r",vox1r_grad.tramp);
  putvalue("rgvox2r",vox2r_grad.tramp);
  putvalue("rgvox3r",vox3r_grad.tramp);

  

  /* Create phase cycling */

  noph=getval("noph");
  getstr("autoph",autoph);
  getstr("pcflag",pcflag);

  //v1=ct;
  //v2=ct;
  //v3=ct;
  //v4=ct;
  assign(zero, oph); 
  counter=(double)nt*(ix-1); //counter of the array (0,4,8,12..) for nt=4
  if (autoph[0] == 'n') counter=0.0; //only goes through nt, if 'y' goes through nt*array
  initval(counter,v1);  //v1=(0,4,8,12) for nt=4
  initval(noph,v3);
  add(v1,ct,v5); //      v2=(0,5,9,13)
  modn(v5,v3,v5); 

   if (pcflag[0] == 'y') {
  settable(t1,noph,ph1);   getelem(t1,v5,vph1); //where ct is the counter, noph=8 for steam
  settable(t2,noph,ph2);   getelem(t2,v5,vph2);
  settable(t3,noph,ph3);   getelem(t3,v5,vph3);
  settable(t4,noph,phobs); getelem(t4,v5,oph);
  }
  else
  {
  assign(zero, vph1); 
  assign(zero, vph2); 
  assign(zero, vph3); 
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
  obl_shapedgradient(vox1_grad.name,vox1_grad.duration,vox1_grad.amp,0,0,NOWAIT);   
  delay(vox1_grad.rfDelayFront);
  shapedpulseoffset(p1_rf.pulseName,vox1_grad.rfDuration,vph1,rof2,rof2,freq1);
  delay(vox1_grad.rfDelayBack);

  delay(te_delay1);

  /* 2nd 90 degree pulse ********************************/
  obl_shapedgradient(vox2_grad.name,vox2_grad.duration,0,vox2_grad.amp,0,NOWAIT);   
  delay(vox2_grad.rfDelayFront);
  shapedpulseoffset(p1_rf.pulseName,vox2_grad.rfDuration,vph2,rof2,rof2,freq2);
  delay(vox2_grad.rfDelayBack);

  
   

  /* Optional TM water suppression ***********************/
  if (ws_tm[0] == 'y') {
    if (wsrf[0]=='y') {
    obspower(ws_rftm.powerCoarse);
    obspwrf(ws_rftm.powerFine);
    delay(4e-6);
    shapedpulseoffset(ws_rftm.pulseName,ws_rftm.rfDuration,zero,rof1,rof2,ws_delta);
    }
     else delay(ws_rftm.rfDuration+4e-6);
  }

  /* TM Gradient crusher ********************************/
  obl_shapedgradient(tmcrush_grad.name,tmcrush_grad.duration,0,0,tmcrush_grad.amp,WAIT);   

  /* TM period *******************************************/
  delay(tm_delay);

  /* 3rd 90 degree pulse ********************************/
  obspower(p1_rf.powerCoarse);  //may have been changed by WS during TM period
  obspwrf(p1_rf.powerFine);
  delay(4e-6);
  obl_shapedgradient(vox3_grad.name,vox3_grad.duration,0,0,vox3_grad.amp,NOWAIT);   
  delay(vox3_grad.rfDelayFront);
  shapedpulseoffset(p1_rf.pulseName,vox3_grad.rfDuration,vph3,rof2,rof2,freq3);
  delay(vox3_grad.rfDelayBack);

  /* Refocus all three Slice Select Gradients ***********/
  obl_shapedgradient(vox1r_grad.name,vox1r_grad.duration,
    vox1r_grad.amp,vox2r_grad.amp,-vox3r_grad.amp,WAIT);   

  delay(te_delay2);
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

  /* Data acquisition ***********************************/
  
  delay(tr_delay);

}  //end of the pulse sequence

/*********************************************************************
                      MODIFICATION HISTORY

051205 (mh)  Started from Inova steam sequence
*********************************************************************/
