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
    PRESS.C

    Volume localized spectroscopy using the double spin echo
    PRESS technique
    Water suppression schemes (ws='y'): 
      VAPOR (wss='vapor'), WET (wss='wet') and DRY (wss='dry')
    

******************************************************************/

#include <standard.h>
#include "sgl.c"


void pulsesequence()
{
  /***** Internal variable declarations *****/
  double te_d1,te_d2,te_d3;             /* delays */
  double tau1,tau2,tau3;
  double te2min,te2,te1;
  double tr_delay;
  double freq1,freq2,freq3;
  double rprof,pprof,sprof;
  double restol, resto_local, csd_ppm;
  char autoph[MAXSTR], pcflag[MAXSTR];
  char profile_vox[MAXSTR];
  char profile_ovs[MAXSTR];
  

  init_mri();
  get_ovsparameters();
  get_wsparameters();

  getstr("pcflag",pcflag);
  getstr("profile_vox",profile_vox);
  getstr("profile_ovs",profile_ovs);

  rprof = getval("rprof");
  pprof = getval("pprof");
  sprof = getval("sprof");
  
  restol=getval("restol");   //local frequency offset
  roff=getval("roff");       //receiver offset
  csd_ppm=getval("csd_ppm"); //chemical shift displacement factor

  te2 = getvalnwarn("te2");
  te1 = getvalnwarn("te1");
  
  /***** RF power calculations *****/
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);
  //init_rf(&p2_rf,p2pat,p2,flip2,rof1,rof2);
  shape_rf(&p2_rf,"p2",p2pat,p2,flip2,rof1,rof2);
  
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");

  /***** Initialize gradient structs *****/

  
  tcrush=granularity(tcrush,GRADIENT_RES); //this is to avoid the granularity errors
  

  if (gcrush>gmax){
  
   abort_message("gcrush too large. Max gcrush = %f \n",gmax*0.95);
  }
  init_slice(&vox1_grad,"vox1",vox1);
  init_slice_butterfly(&vox2_grad,"vox2",vox2,gcrush,tcrush);
  init_slice_butterfly(&vox3_grad,"vox3",vox3,gcrush,tcrush);
  init_slice_refocus(&vox1r_grad,"vox1r");

  if (profile_vox[0] == 'y') {
    init_readout_butterfly(&ro_grad,"ro",lro,np,sw,gcrushro,tcrushro);
    init_readout_refocus(&ror_grad,"ror");
  }

  /***** Gradient calculations *****/
  calc_slice(&vox1_grad,&p1_rf,WRITE,"gvox1");
  calc_slice(&vox2_grad,&p2_rf,WRITE,"gvox2");
  calc_slice(&vox3_grad,&p2_rf,WRITE,"gvox3");
  calc_slice_refocus(&vox1r_grad,&vox1_grad,WRITE,"gvox1r");

  if (vox1_grad.rfDelayFront < 0.2e-6) vox1_grad.rfDelayFront = 0;
  if (vox1_grad.rfDelayBack  < 0.2e-6) vox1_grad.rfDelayBack  = 0;
  
  if (profile_vox[0] == 'y') {
    calc_readout(&ro_grad,WRITE,"gro","sw","at");
    putvalue("gro",ro_grad.roamp);       // RO grad
    calc_readout_refocus(&ror_grad,&ro_grad,WRITE,"gror");
    putvalue("tror",ror_grad.duration);  // ROR duration
  }


   if (profile_ovs[0]=='y'){
     if (rprof==1) {
     vox1_grad.amp=0;
     vox1r_grad.amp=0;
      
     }
     else if(pprof==1) {
     vox2_grad.amp=0;
   
     }     
     else if(sprof==1) {
     vox3_grad.amp=0;
    
     }
  }
                                                                                

  /* Optional Outer Volume Suppression */
  if (ovs[0] == 'y')  create_ovsbands();
  if (ws[0]  == 'y')  create_watersuppress();
  if (sat[0] == 'y')  create_satbands();


  /***** Min TE *****/
  /* tau1, tau2 and tau3 are sums of all events in TE = te1 + te2 */
  /* tau1: 90 - 1st 180,  tau2: 1st 180 - 2nd 180,  tau3: 2nd 180 - ACQ */
  tau1 = vox1_grad.rfCenterBack + vox1r_grad.duration + vox2_grad.rfCenterFront;
  tau2 = vox2_grad.rfCenterBack + vox3_grad.rfCenterFront;
  tau3 = vox3_grad.rfCenterBack;

  temin  = 2.0 * (tau1 + 4e-6);  //add 4us to ensure at least 4us between gradient events
  te2min = 2.0 * (tau3 + 4e-6);
  if (minte[0] == 'y') {
    te1 = temin;
    te2 = te2min;
    putvalue("te1",te1);
    putvalue("te2",te2);
  }
  if (te1 < temin) {
    abort_message("te1 too short. Minimum te1 = %.2f ms\n",temin*1000);
  }
  if (te2 < te2min) {
    abort_message("te2 too short. Minimum te2 = %.2f ms\n",te2min*1000);
  }

  /***** Calculate TE delays *****/
  te_d1 = te1/2.0 - tau1;
  te_d2 = te1/2.0 + te2/2.0 - tau2 ;
  te_d3 = te2/2.0 - tau3;

   //Calculate delta from resto to include local frequency line+ chemical shift offset
  resto_local=resto-restol;  

  /***** Min TR *****/
  trmin = 12e-6 + vox1_grad.rfCenterFront + te1 + te2 + alfa + at;

  if (ws[0]  == 'y') trmin += wsTime;
  if (ovs[0] == 'y') trmin += ovsTime;
  if (sat[0] == 'y') trmin += satTime;
  if (profile_vox[0] == 'y') trmin += ror_grad.duration + ro_grad.duration - at; 

  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr);
  }
  if (tr < trmin) {
    abort_message("tr too short. Minimum tr = %.2f ms\n",(trmin+4e-6)*1000);
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

  if (profile_ovs[0]=='y'&& sprof==1) freq3=0.0;
  if (profile_ovs[0]=='y'&& pprof==1) freq2=0.0;
  if (profile_ovs[0]=='y'&& rprof==1) freq1=0.0;

  
  /* profile_vox frequency offset */
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
  putvalue("rgvox1r",vox1r_grad.tramp);


  
  sgl_error_check(sglerror);
  
  if (ss<0) g_setExpTime(trmean*(nt-ss)*arraydim);
  else g_setExpTime(tr*(ntmean*arraydim+ss));

  /* PULSE SEQUENCE *************************************/

  /* Phase cycle ****************************************
  ct  =  1  2  3  4    5  6  7  8    10 11 12 13 ......
  v2  =  0  1  2  3    0  1  2  3    0  1  2  3
  v1  =  0  0  1  1    2  2  3  3    4  4  5  5
  v1  =  0  0  0  0    1  1  1  1    2  2  2  2
  v1  =  0  0  0  0    1  1  1  1    2  2  2  2
  oph =  0  2  4  6    0  2  4  6    0  2  4  6
  v9  =  0  0  0  0    2  2  2  2    4  4  4  4
  oph =  0  2  4  6    2  4  6  8    4  6  8  10

  v10 =  0  0  1  1    2  2  3  3    4  4  5  5
  v10 =  0  0  0  0    1  1  1  1    2  2  2  2  
  v10 =  0  0  0  0    0  0  0  0    1  1  1  1
  v10 =  0  0  0  0    0  0  0  0    0  0  0  0
  v3  =  1  1  1  1    1  1  1  1    1  1  1  1
  *******************************************************/

  mod4(ct,v2);
  hlv(ct,v1);
  hlv(v1,v1);
  mod4(v1,v1);
  dbl(v2,oph);
  dbl(v1,v9);
  add(v9,oph,oph);
  /* CYCLOPS */
  hlv(ct,v10);
  hlv(v10,v10);
  hlv(v10,v10);
  hlv(v10,v10);
  mod4(v10,v10);
 
  if (pcflag[0] == 'n') {
  assign(zero,v2);
  assign(zero,v1);
  assign(zero,v3);
  assign(zero,oph);
  }
  else {
  add(one,v10,v3);
  add(v10,v1,v1);
  add(v10,v2,v2);
  add(v10,oph,oph);
  }

  /* Relaxation delay ***********************************/
  obsoffset(resto_local);  // need it here for water suppression to work
  delay(4e-6);
  rot_angle(vpsi,vphi,vtheta);

  if (ticks) {
    xgate(ticks);
    grad_advance(gpropdelay);
    delay(4e-6);
  }

  /* TTL scope trigger **********************************/
  sp1on(); delay(4e-6); sp1off();

  /* Saturation bands ***********************************/
  if (ovs[0] == 'y') ovsbands();
  if (sat[0] == 'y') satbands();

  /* Water suppression **********************************/
  if (ws[0]  == 'y') watersuppress();

  /* Slice selective 90 degree RF pulse *****/
  obspower(p1_rf.powerCoarse);
  obspwrf(p1_rf.powerFine);
  delay(4e-6);
  obl_shapedgradient(vox1_grad.name,vox1_grad.duration,vox1_grad.amp,0,0,NOWAIT);
  delay(vox1_grad.rfDelayFront);
  shapedpulseoffset(p1_rf.pulseName,vox1_grad.rfDuration,v3,rof1,rof2,freq1);
  delay(vox1_grad.rfDelayBack);

  /* Slice selection refocus *****/
  obl_shapedgradient(vox1r_grad.name,vox1r_grad.duration,-vox1r_grad.amp,0,0,WAIT);

  /* First half-TE1 delay *****/
  obspower(p2_rf.powerCoarse);
  obspwrf(p2_rf.powerFine);
  delay(te_d1);  //this is always >= 4us

  /* First slice selective 180 degree pulse *****/
  obl_shapedgradient(vox2_grad.name,vox2_grad.duration,0,vox2_grad.amp,0,NOWAIT);
  delay(vox2_grad.rfDelayFront);
  shapedpulseoffset(p2_rf.pulseName,vox2_grad.rfDuration,v2,rof1,rof2,freq2);
  delay(vox2_grad.rfDelayBack);

  /* Second half-TE1 delay, first half-TE2 delay *****/
  delay(te_d2);

  /* Second slice selective 180 degree pulse *****/
  obl_shapedgradient(vox3_grad.name,vox3_grad.duration,0,0,vox3_grad.amp,NOWAIT);
  delay(vox3_grad.rfDelayFront);
  shapedpulseoffset(p2_rf.pulseName,vox3_grad.rfDuration,v1,rof1,rof2,freq3);
  delay(vox3_grad.rfDelayBack);

  /* Second half-TE2 delay *****/
  delay(te_d3);
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

}
