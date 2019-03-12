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
Multi-spin echo imaging sequence (slice selective CPMG).
************************************************************************/
#include <standard.h>
#include "sgl.c"



void pulsesequence()
{
  /* Variables for setting up phase-ramped pulses */
  double  freq90[MAXNSLICE],freq180[MAXNSLICE];
  int     shape90, shape180;

  /* Timing variables */
  double  tau1, tau2, tau3, te_delay1, te_delay2, te_delay3, 
          tautr, tr_delay;

  /* Crushers on readout gradient */
  double  trocrush, grocrush;

  /* Variable crusher variables */
  int     ncr;
  int     crval[MAXNECHO];
  int     cr_mult;
  double  cr_step;

  /* Flag to control phase encoding through table */
  int     table = 0;  

    
  /* Real-time variables ************************************/
  int vpe_steps  	= v1;
  int vpe_ctr    	= v2;
  int vms_slices 	= v3;
  int vms_ctr    	= v4;
  int vpe_offset 	= v5;
  int vpe_index  	= v6;
  int vssc       	= v7;
  int vne_echoes	= v8;
  int vne_ctr		= v9;
  int vph90 		= v10;
  int vph180 		= v11;
  int vcr_mult		= v12;
  int vacquire          = v13;

 
  /*  Initialize paramaters *********************************/
  init_mri();
  grocrush = getval("grocrush");
  trocrush = getval("trocrush");
  

  /* Set up multiplication factors on variable crushers */
  /* Poon-Henkelman scheme */
  cr_mult = ne/2;
  for (ncr = 0;ncr < (int)ne; ncr += 2) {
    crval[ncr]   =  cr_mult;
    crval[ncr+1] = -cr_mult;
    cr_mult -= 1;  /* Sean had 2 here */
  }
  settable(t2,(int)ne,crval);
  cr_step = gcrush*2/ne;  /* Step size for variable crusher */

  
  /*  Check for external PE table ***************************/
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    table = 1;
  }

  /* RF Calculations ****************************************/
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);
  init_rf(&p2_rf,p2pat,p2,flip2,rof2,rof2);
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");
  if (p2_rf.header.rfFraction != 0.5)
  abort_message("RF pulse for refocusing (%s) must be symmetric",p2pat);
  
  /* Initialize gradient structures *************************/
  init_slice(&ss_grad,"ss",thk);
  init_slice(&ss2_grad,"ss2",thk);
  init_slice_refocus(&ssr_grad,"ssr");
 
  init_readout_butterfly(&ro_grad,"ro",lro,np,sw,grocrush,trocrush);  
  init_readout_refocus(&ror_grad,"ror");
  
  init_phase(&pe_grad,"pe",lpe,nv);  

  init_generic(&crush_grad,"crush",gcrush,tcrush);  /* base shape for variable crusher */

  
    
  /* Gradient calculations **********************************/
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice(&ss2_grad,&p1_rf,WRITE,"gss2");  /* NOTE: using p1's bandwidth => gss = gss2 */
  calc_slice_refocus(&ssr_grad, &ss_grad, NOWRITE,"gssr");
  
  calc_readout(&ro_grad, WRITE, "gro","sw","at");
  calc_readout_refocus(&ror_grad, &ro_grad, NOWRITE, "gror");

  calc_phase(&pe_grad, WRITE, "gpe","tpe");
  	
  /* Equalize refocus and PE gradient durations *************/
  calc_sim_gradient(&ror_grad, &null_grad, &ssr_grad,0,WRITE);

  calc_generic(&crush_grad,WRITE,"","");

  
  /* Generate phase-ramped pulses: 90, 180 */
  offsetlist(pss,ss_grad.ssamp,0,freq90,ns,seqcon[1]);
  shape90 = shapelist(p1pat,ss_grad.rfDuration,freq90,ns,0,seqcon[1]);

  offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,seqcon[1]);
  shape180 = shapelist(p2pat,ss2_grad.rfDuration,freq180,ns,0,seqcon[1]);
  
  
  /* Min TE *************************************************/
  tau1 = ss_grad.rfCenterBack + ssr_grad.duration + crush_grad.duration + ss2_grad.rfCenterFront;
  tau2 = ss2_grad.rfCenterBack + crush_grad.duration + pe_grad.duration + ro_grad.timeToEcho;
  tau3 = ro_grad.timeFromEcho + pe_grad.duration + crush_grad.duration + ss2_grad.rfCenterBack;

  temin = 2*MAX(MAX(tau1,tau2),tau3);
  temin += 2*4e-6; /* ensure that te delays are always >= 4us */

  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (te < temin) {
    abort_message("TE too short.  Minimum TE = %.2fms\n",temin*1000);   
  }
  te_delay1 = te/2 - tau1;
  te_delay2 = te/2 - tau2;
  te_delay3 = te/2 - tau3;
 

  /* Min TR *************************************************/
  tautr  = 4*4e-6;
  tautr += ss_grad.rfCenterFront;
  tautr += (ne*te);
  tautr += (ro_grad.timeFromEcho + pe_grad.duration + te_delay3); /*back end of seq*/
   
  /* Optional prepulse calculations *************************/
  if (sat[0] == 'y') {
    create_satbands();
    tautr += satTime;
  }
  
  if (fsat[0] == 'y') {
    create_fatsat();
    tautr += fsatTime;
  }

  if (mt[0] == 'y') {
    create_mtc();
    tautr += mtTime;
  }


  trmin = tautr + ns*4e-6; /* ensure that tr delay is always >= 4us */
  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr);
  }
  if (tr < trmin) {
    abort_message("TR too short.  Minimum TR = %.2fms\n",trmin*1000);   
  }
  
  tr_delay = (tr-tautr)/ns;


  /* Set pe_steps for profile or full image **********/   	
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&null_grad);
  initval(pe_steps/2.0,vpe_offset);
  initval(ne,vne_echoes);

  sgl_error_check(sglerror);
  
  g_setExpTime(tr*(nt*pe_steps*arraydim + ssc));

  /* PULSE SEQUENCE *************************************/
  rotate();
  obsoffset(resto);
  roff = -poffset(pro,ro_grad.roamp);
  delay(4e-6);
  setacqvar(vacquire);
  
  /* Set up phases for RF pulses */
  assign(oph,vph90);
  add(vph90,one,vph180);  /* xyyyy scheme */
  
  /* Begin phase-encode loop ****************************/       
  peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);

    /* Read external kspace table if set ******************/       
    if (table)
      getelem(t1,vpe_ctr,vpe_index);
    else
      sub(vpe_ctr,vpe_offset,vpe_index);

    /* Compressed steady-states: 1st array & transient, all arrays if ssc is negative */
    if ((ix > 1) && (ssc > 0))
      assign(zero,vssc);
    sub(vpe_ctr,vssc,vpe_ctr);  // vpe_ctr counts up from -ssc
    assign(zero,vssc);
    if (seqcon[2] == 's')
      assign(zero,vacquire);    // Always acquire for non-compressed loop
    else {
      ifzero(vpe_ctr);
        assign(zero,vacquire);  // Start acquiring when vpe_ctr reaches zero
      endif(vpe_ctr);
    }

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
      if (fsat[0] == 'y') fatsat();
      if (mt[0]   == 'y') mtc();

      /* Slice select RF pulse ******************************/ 
      obspower(p1_rf.powerCoarse);
      obspwrf(p1_rf.powerFine);
      delay(4e-6);
      obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
      delay(ss_grad.rfDelayFront);
      shapedpulselist(shape90,ss_grad.rfDuration,vph90,rof2,rof2,seqcon[1],vms_ctr);
      delay(ss_grad.rfDelayBack);

 
      /*Slice refocus and read defocus*/
      obl_shapedgradient(ssr_grad.name,ssr_grad.duration,ror_grad.amp,0,-ssr_grad.amp,WAIT);

      /* Set up power for 180 pulse here, exploit that te_delay1 is >= 4us */
      obspower(p2_rf.powerCoarse);
      obspwrf(p2_rf.powerFine);     

      /* TE delay 1 */
      delay(te_delay1);
   
  
      /* CPMG loop */
      loop(vne_echoes,vne_ctr);
        getelem(t2,vne_ctr,vcr_mult);  
	
        /* Variable crusher */
	/* Need to use phase_encode3_oblshapedgradient because we need to 
	  specify the limits on the multipliers, other than nv, nv2, nv3 */
        phase_encode3_oblshapedgradient(crush_grad.name,crush_grad.name,crush_grad.name,
	  crush_grad.duration,         /* duration */
	  0.0,0.0,0.0,                 /* 3 static amplitudes */
	  cr_step,cr_step,cr_step,     /* step size */
	  vcr_mult,vcr_mult,vcr_mult,  /* multiplier */
	  (double)ne,(double)ne,(double)ne,
          1, WAIT, 0);
  
        /* Refocusing slice select RF pulse ******************************/ 
    	obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);
     	delay(ss2_grad.rfDelayFront);
	shapedpulselist(shape180,ss2_grad.rfDuration,vph180,rof2,rof2,seqcon[1],vms_ctr);
	delay(ss2_grad.rfDelayBack);	

        /* Variable crusher */
        phase_encode3_oblshapedgradient(crush_grad.name,crush_grad.name,crush_grad.name,
	  crush_grad.duration,         /* duration */
	  0.0,0.0,0.0,                 /* 3 static amplitudes */
	  cr_step,cr_step,cr_step,     /* step size */
	  vcr_mult,vcr_mult,vcr_mult,  /* multiplier */
	  (double)ne,(double)ne,(double)ne,
          1, WAIT, 0);
  	
        /* TE delay 2 */
	delay(te_delay2);

        /* Phase encode */
        pe_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,-pe_grad.increment,vpe_index,WAIT); 
	  
        /* Readout */
	obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
	delay(ro_grad.atDelayFront);
	startacq(alfa);
	acquire(np,1.0/sw);
	endacq();
	delay(ro_grad.atDelayBack);
     
        /* Phase rewind */
        pe_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,pe_grad.increment,vpe_index,WAIT);  
  
        /* TE delay 3 */
        delay(te_delay3);
       
      endloop(vne_ctr);

      /* Relaxation delay ***********************************/       
      delay(tr_delay);
    endmsloop(seqcon[1],vms_ctr);
  endpeloop(seqcon[2],vpe_ctr);
}
