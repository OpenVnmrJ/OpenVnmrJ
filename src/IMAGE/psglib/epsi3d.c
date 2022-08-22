/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/***********************************************************************
csi2d - CSI imaging sequence
	- spinecho or gradient echo
	- slice selective 2D CSI
	- SGL version for vnmrs

************************************************************************/
#include <standard.h>
#include "sgl.c"

/* Phase cycling of 180 degree pulse */
static int ph180[2] = {1,3};

void pulsesequence()
{
  /* Internal variable declarations *************************/
  double  freq90[MAXNSLICE],freq180[MAXNSLICE];
  int     shape90=0, shape180=0;
  double  tau1, tau2, tau3,te_delay1, te_delay2, tr_delay;
  double  restol, resto_local, csd_ppm;
  double te_delay3=0.0;

  double vox1_cr;
  char  epsi[MAXSTR],slprofile[MAXSTR];
  int i,j,offset,sign;
  double dw;  /* nominal dwell time, = 1/sw */
  double aqtm = getval("aqtm");

  double sws; //spectral bandwidth = acquire length (np/2*dw)+rewinding grad length

  
  

 // SLICE_SELECT_GRADIENT_T  vox1_crush;
  REFOCUS_GRADIENT_T      ror1_grad;
 GENERIC_GRADIENT_T       endspoil_grad;  // crusher after FID is acquired
 

  /* Real-time variables ************************************/
  int  vpe_steps   = v1;
  int  vpe_ctr     = v2;
  int  vpe_index   = v3;
  int  vpe_offset  = v4;
  int  vpe2_steps  = v5;
  int  vpe2_ctr    = v6;
  int  vpe2_index  = v7;
  int  vpe2_offset = v8;
  int  vms_slices  = v9;
  int  vms_ctr     = v10;
  int  vph180      = v11;  /* Phase of 180 pulse */
  
  int vetl         = v12;   // Number of choes in readout train
  int vetl_ctr     = v13;   // etl_steps loop counter


   int    pt, pts, inx=0,etl_steps;
   double gcrush_end, tcrush_end;
 

  /*  Initialize paramaters *********************************/
  init_mri();
  get_wsparameters();

  restol=getval("restol");   //local frequency offset
  roff=getval("roff");       //receiver offset
  etl_steps=getval("etl_steps"); //etl_steps
  getstr("epsi", epsi);

   gcrush_end = getval("gcrush_end");
  tcrush_end = getval("tcrush_end");

  getstr("slprofile", slprofile);
  


 // thk=lro; 

  /*set voxel sizes for butterfly crushers to 10^6 to set the slice portion to zero ***/
 // vox1_cr=1000000;

  /* Initialize gradient structures *************************/
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);
  init_rf(&p2_rf,p2pat,p2,flip2,rof2,rof2);
  init_slice(&ss_grad,"ss",thk);
  init_slice_refocus(&ssr_grad,"ssr");                 
  

  init_slice_butterfly(&ss2_grad,"ss2",thk*10e6,gcrush,tcrush); //makes empty butterfly
  init_phase(&pe_grad,"pe",lpe,nv);
  init_phase(&pe2_grad,"pe2",lpe2,nv2);

   init_generic(&endspoil_grad,"endspoil",gcrush_end,tcrush_end);
  
  //init_slice_butterfly(&vox1_crush,"vox1_crush",vox1_cr,gcrush,tcrush);

  /* RF Calculations ****************************************/
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");
  if (p2_rf.header.rfFraction != 0.5)
    abort_message("RF pulse for refocusing (%s) must be symmetric",p2pat);
  
  /* Gradient calculations **********************************/
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice(&ss2_grad,&p2_rf,WRITE,"gss2");
  calc_slice_refocus(&ssr_grad, &ss_grad, NOWRITE,"gssr");


  
      
  calc_phase(&pe_grad,  NOWRITE, "gpe","tpe");
  calc_phase(&pe2_grad, NOWRITE, "gpe2","");
  calc_generic(&endspoil_grad,WRITE,"","");

 // calc_slice(&vox1_crush,&p2_rf,WRITE,"vox1_crush");

  /* Equalize refocus and PE gradient durations *************/
  calc_sim_gradient(&ssr_grad, &pe_grad, &pe2_grad, trise,WRITE);


  if (epsi[0]=='y') {
  /*epsi gradient construction*/
  /* Gradient calculations ******************************/
  
  init_readout(&ro_grad,"ro",lro,np,sw);         // readout gradient
  calc_readout(&ro_grad, WRITE, "gro","sw","at");
  //double the moment for the rewinder


 // init_readout(&ro1_grad,"ro1",lro/2,np,sw);         // readout gradient
 // calc_readout(&ro1_grad, NOWRITE, "gro","sw","at");
  
 
  //Define 1st lobe, which should be 1/2 of the ro_grad
  init_readout_refocus(&ror1_grad,"ror1");         // readout dephocuse
  calc_readout_refocus(&ror1_grad, &ro_grad, WRITE, "gror1");
  
  //Define main lobe, which will be used for the rest of the flyback
  init_readout_refocus(&ror_grad,"ror");         // readout dephocuse
  ro_grad.m0ref=ro_grad.m0ref*2;
  calc_readout_refocus(&ror_grad, &ro_grad, NOWRITE, "gror");


  init_generic(&crush_grad,"flyback",gcrush,etl_steps*ror_grad.duration+etl_steps*ro_grad.duration);
  calc_generic(&crush_grad,NOWRITE,"","");

  inx = 0;
  for(j=0; j<etl_steps; j++) {
  offset = j*ror_grad.numPoints;
  //if (j%) sign=1; else sign=-1;

  
  /*if (j==0) {
  for (i=0; i<ror_grad.numPoints; i++) {
    crush_grad.dataPoints[inx++] = -1*ror1_grad.dataPoints[i];
  //printf("%.2f   ",crush_grad.dataPoints[i]);
  }
  }
  else 
  {
*/
  
  for (i=0; i<ro_grad.numPoints; i++) {
    crush_grad.dataPoints[inx++] = ro_grad.dataPoints[i];
  //printf("%.2f   ",crush_grad.dataPoints[i]);
  }

  for (i=0; i<ror_grad.numPoints; i++) {
    crush_grad.dataPoints[inx++] = -1*ror_grad.dataPoints[i];
  //printf("%.2f   ",crush_grad.dataPoints[i]);
  }
  //}

  


}

writeToDisk(crush_grad.dataPoints, crush_grad.numPoints, 0,
                  crush_grad.resolution, TRUE /* rollout */,
                  crush_grad.name);

  
  } //end of epsi flag


  



  

  /*END of epsi GRAD*/


  putvalue("gpe",pe_grad.peamp);       // PE max grad amp
  putvalue("tpe",pe_grad.duration);    // PE grad duration
  putvalue("gpe2",pe2_grad.peamp);     // PE2 max grad amp

  putvalue("gror",ror_grad.amp);
  putvalue("gro",ro_grad.amp);
  putvalue("sws",1/(ror_grad.duration+at));

  /* Min TE *************************************************/
  tau1 = ss_grad.rfCenterBack + pe_grad.duration;
  tau2 = alfa;
  tau3= 0;
  temin = tau1 + tau2 + 4e-6+tau3;
  te_delay1 = te - (tau1 + tau2);
  te_delay2 = 0;

  if (spinecho[0] == 'y') {
    tau1 += 4e-6 + ss2_grad.rfCenterFront;
    tau2 += ss2_grad.rfCenterBack+ss2_grad.rfCenterFront;
    tau3 += ss2_grad.rfCenterBack;
   // temin = 2*(MAX(tau1,tau2,tau3) + 4e-6);  /* have at least 4us between gradient events */
    
   temin = 4*tau1+4e-6;
   // te_delay1 = te/2 - tau1;
    te_delay2 = te/2 - tau2;
    te_delay3= te/2- tau1-tau3;
  }
  
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",ceil(te*1e6)*1e-6); /* round up to nearest us */
  }
  if (te < temin) {
    abort_message("TE too short.  Minimum TE = %.2fms\n",temin*1000); 
  
  }

if (te_delay3 < 0) {
    abort_message("TE too short. Third delay is negative. Minimum TE = %.2fms\n",temin*1000);   
  }

  /* Min TR *************************************************/   	
  trmin = 2*4e-6 + ss_grad.rfCenterFront + te + at+endspoil_grad.duration;
  
  /* Optional prepulse calculations *************************/
  if (sat[0] == 'y') {
    create_satbands();
    trmin += satTime;
  }
  if (ws[0] == 'y') {
    create_watersuppress();
    trmin += wsTime;
  }
  
  trmin *= ns;
  if (mintr[0] == 'y'){
    tr = trmin + ns*4e-6;
    putvalue("tr",tr);
  }
  if (tr < trmin) {
    abort_message("TR too short.  Minimum TR= %.2fms\n",(trmin + ns*4e-6)*1000);   
  }
  tr_delay = (tr - trmin)/ns;

   //Calculate delta from resto to include local frequency line+ chemical shift offset
  resto_local=resto-restol;

  /* Generate phase-ramped pulses */
  offsetlist(pss,ss_grad.ssamp,0,freq90,ns,seqcon[1]);
  shape90 = shapelist(p1pat,ss_grad.rfDuration,freq90,ns,0,seqcon[1]);

  offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,seqcon[1]);
  shape180 = shapelist(p2pat,ss2_grad.rfDuration,freq180,ns,0,seqcon[1]);

  /* Set pe_steps for profile or full image **********/   	
  pe_steps  = prep_profile(profile[0],nv, &pe_grad, &null_grad);
  pe2_steps = prep_profile(profile[1],nv2,&pe2_grad,&null_grad);


 // printf("pe_steps = %d \n", pe_steps); 
 // printf("pe_steps = %c \n", profile[0]); 

  initval(pe_steps/2.0, vpe_offset);
  initval(pe2_steps/2.0,vpe2_offset);
  F_initval(etl_steps, vetl); //? 
 // initval(etl_steps, vetl);

  sgl_error_check(sglerror);

   //dw = granularity(1/sw,1/epsi_grad.ddrsr); //?

  dw = granularity(1/sw,GRADIENT_RES);

 

  g_setExpTime(tr*(nt*pe_steps*pe2_steps*arraydim));

  /* PULSE SEQUENCE *************************************/
  rotate();
  obsoffset(resto_local);
  delay(4e-6);

  /* Begin phase-encode loop ****************************/       
  peloop2(seqcon[3],pe2_steps,vpe2_steps,vpe2_ctr);
    peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);

      sub(vpe_ctr, vpe_offset, vpe_index);
      sub(vpe2_ctr,vpe2_offset,vpe2_index);

      settable(t2,2,ph180);        /* initialize phase tables and variables */
      getelem(t2,vpe_ctr,vph180);  /* 180 deg pulse phase alternates +/- 90 off the receiver */
      add(oph,vph180,vph180);

      /* Begin multislice loop ******************************/       
      msloop(seqcon[1],ns,vms_slices,vms_ctr);
        if (ticks) {
          xgate(ticks);
          grad_advance(gpropdelay);
        }

	/* TTL scope trigger **********************************/       
	 sp1on(); delay(4e-6); sp1off();

	/* Prepulses ******************************************/       
	if (sat[0]  == 'y') satbands();
	if (ws[0]   == 'y') watersuppress();

	/* Slice select RF pulse ******************************/ 
	obspower(p1_rf.powerCoarse);
	obspwrf(p1_rf.powerFine);
	delay(4e-6);
	obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
	delay(ss_grad.rfDelayFront);
	shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
	delay(ss_grad.rfDelayBack);

	if(epsi[0] =='y') {
        /* Phase encode, slice refocus gradient ********/
	pe3_shapedgradient(pe_grad.name,pe_grad.duration,
	                   0,0,-ssr_grad.amp,
            		   0,pe_grad.increment,pe2_grad.increment,
                           zero,vpe_index,vpe2_index,WAIT);

        }
        else {   //regular csi
        pe3_shapedgradient(pe_grad.name,pe_grad.duration,
	                   0,0,-ssr_grad.amp,
            		   pe_grad.increment,pe2_grad.increment,0,
                           vpe_index,vpe2_index,0,WAIT);

	}
	// delay(te_delay1);

        if (spinecho[0] == 'y') {
	  /* Refocusing RF pulse ********************************/ 
	  obspower(p2_rf.powerCoarse);
	  obspwrf(p2_rf.powerFine);
	  delay(4e-6);
	  obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);
	  delay(ss2_grad.rfDelayFront);
	  shapedpulselist(shape180,ss2_grad.rfDuration,vph180,rof2,rof2,seqcon[1],vms_ctr);
	  delay(ss2_grad.rfDelayBack);
	
	delay(te_delay2);
         // delay(te_delay2/2);
         obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);
	  delay(ss2_grad.rfDelayFront);
	  shapedpulselist(shape180,ss2_grad.rfDuration,vph180,rof2,rof2,seqcon[1],vms_ctr);
	  delay(ss2_grad.rfDelayBack);
          }
        
        delay(te_delay3);

    if (epsi[0] =='n') {
/* Acquisition ********************/
	startacq(alfa);
	acquire(np,1.0/sw);
	endacq();
    }
    else {
    
    
    //START epsi grad 
    //First play the dephasing part
    if (slprofile[0]=='n'){
    obl_shapedgradient(ror1_grad.name,ror1_grad.duration,-1*ror_grad.amp,0,0,WAIT); 
    obl_shapedgradient(crush_grad.name,crush_grad.duration,ror_grad.amp,0,0,NOWAIT);
    }
    else  {
    obl_shapedgradient(ror1_grad.name,ror1_grad.duration,0,0,-1*ror_grad.amp,WAIT); 
    obl_shapedgradient(crush_grad.name,crush_grad.duration,0,0,ror_grad.amp,NOWAIT);
    }
     //START epsi LOOP
     nowait_loop(etl_steps,vetl,vetl_ctr); 
     delay(ro_grad.atDelayFront); /* wait to ramp up */ 
     startacq(alfa);
	
        acquire(np,1.0/sw);		
	delay(ror_grad.duration+ro_grad.atDelayBack-alfa);
	
     endacq();
     nowait_endloop(vetl_ctr);

     obl_shaped3gradient   (endspoil_grad.name,endspoil_grad.name,endspoil_grad.name,endspoil_grad.duration,endspoil_grad.amp,endspoil_grad.amp,endspoil_grad.amp,WAIT);  
  

    // }  //end of EPSI


     

	/* Relaxation delay ***********************************/       
     } //end of epsi
    delay(tr_delay);

      endmsloop(seqcon[1],vms_ctr);
    endpeloop(seqcon[2],vpe_ctr);
  endpeloop(seqcon[3],vpe2_ctr);
}
