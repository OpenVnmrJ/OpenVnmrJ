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
  double  tau1, tau2, te_delay1, te_delay2, tr_delay;
  double  restol, resto_local, csd_ppm;

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

  /*  Initialize paramaters *********************************/
  init_mri();
  get_wsparameters();

  restol=getval("restol");   //local frequency offset
  roff=getval("roff");       //receiver offset

  /* Initialize gradient structures *************************/
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);
  init_rf(&p2_rf,p2pat,p2,flip2,rof2,rof2);
  init_slice(&ss_grad,"ss",thk);
  init_slice_refocus(&ssr_grad,"ssr");                 
  init_slice_butterfly(&ss2_grad,"ss2",thk*1.1,gcrush,tcrush);
  init_phase(&pe_grad,"pe",lpe,nv);
  init_phase(&pe2_grad,"pe2",lpe2,nv2);

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

  /* Equalize refocus and PE gradient durations *************/
  calc_sim_gradient(&ssr_grad, &pe_grad, &pe2_grad, trise,WRITE);


  putvalue("gpe",pe_grad.peamp);       // PE max grad amp
  putvalue("tpe",pe_grad.duration);    // PE grad duration
  putvalue("gpe2",pe2_grad.peamp);     // PE2 max grad amp

  /* Min TE *************************************************/
  tau1 = ss_grad.rfCenterBack + pe_grad.duration;
  tau2 = alfa;
  temin = tau1 + tau2 + 4e-6;
  te_delay1 = te - (tau1 + tau2);
  te_delay2 = 0;

  if (spinecho[0] == 'y') {
    tau1 += 4e-6 + ss2_grad.rfCenterFront;
    tau2 += ss2_grad.rfCenterBack;
    temin = 2*(MAX(tau1,tau2) + 4e-6);  /* have at least 4us between gradient events */
    te_delay1 = te/2 - tau1;
    te_delay2 = te/2 - tau2;
  }
  
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",ceil(te*1e6)*1e-6); /* round up to nearest us */
  }
  if (te < temin) {
    abort_message("TE too short.  Minimum TE = %.2fms\n",temin*1000);   
  }

  /* Min TR *************************************************/   	
  trmin = 2*4e-6 + ss_grad.rfCenterFront + te + at;
  
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

  initval(pe_steps/2.0, vpe_offset);
  initval(pe2_steps/2.0,vpe2_offset);

  sgl_error_check(sglerror);

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

	
        if (spinecho[0] == 'y') {

        /* Phase encode, slice refocus gradient ********/
	pe3_shapedgradient(pe_grad.name,pe_grad.duration,
	                   0,0,-ssr_grad.amp,
            		   pe_grad.increment,pe2_grad.increment,0,
                           vpe_index,vpe2_index,zero,WAIT);
        }
        else {
        pe3_shapedgradient(pe_grad.name,pe_grad.duration,
	                   0,0,-ssr_grad.amp,
            		   -pe_grad.increment,-pe2_grad.increment,0,
                           vpe_index,vpe2_index,zero,WAIT);
         }
        
	delay(te_delay1);

        if (spinecho[0] == 'y') {
	  /* Refocusing RF pulse ********************************/ 
	  obspower(p2_rf.powerCoarse);
	  obspwrf(p2_rf.powerFine);
	  delay(4e-6);
	  obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);
	  delay(ss2_grad.rfDelayFront);
	  shapedpulselist(shape180,ss2_grad.rfDuration,vph180,rof2,rof2,seqcon[1],vms_ctr);
	  delay(ss2_grad.rfDelayBack);
	}
	delay(te_delay2);

	/* Acquisition ********************/
	startacq(alfa);
	acquire(np,1.0/sw);
	endacq();

	/* Relaxation delay ***********************************/       
	delay(tr_delay);

      endmsloop(seqcon[1],vms_ctr);
    endpeloop(seqcon[2],vpe_ctr);
  endpeloop(seqcon[3],vpe2_ctr);
}
