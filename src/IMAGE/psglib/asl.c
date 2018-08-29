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
/*******************************************************************
    Multislice, blipped spin/gradient echo-planar imaging sequence.

    Options:				| Controlled by variable:
    					|
      Multi-shot			|   nseg
      Ramp sampling			|   rampsamp
      Fractional k-space sampling	|   fract_ky
      Navigator echo			|   navigator
      Fat suppression			|   fsat
      Inversion Recovery		|   ir
      Automatic calculation of min TE	|   minte
      Spin echo/Gradient echo		|   spinecho

*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#include "standard.h"
#include "group.h"
#include "sgl.c"
#define GDELAY 4e-6


pulsesequence() {
  /* Acquisition variables */
  double dw;  /* nominal dwell time, = 1/sw */
  double aqtm = getval("aqtm");

  /* Delay variables */  
  double tref,
         te_delay1, te_delay2, tr_delay, 
	 timin, ti1min, ti_delay, ti1_delay, tmp,
         busy1, busy2,      /* time spent on rf pulses etc. in TE periods       */
         seqtime, invTime;
  int    use_minte;
  
  /* RF and receiver frequency variables */
  double freq90[MAXNSLICE],freq180[MAXNSLICE];  /* frequencies for multi-slice */
  int    shape90=0, shape180=0; /* List ID for RF shapes */
  double roff1, roff2, roffn; /* Receiver offsets when FOV is offset along readout */
  
  /* Gradient amplitudes, may vary depending on "image" parameter */
  double peramp, perinc, peamp, roamp, roramp;
  
  char   slprofile[MAXSTR];
  double rtheta;  // rotated theta

  /* loop variable */
  int    i;


  /* Real-time variables used in this sequence **************/
  int vms_slices   = v1;   // Number of slices
  int vms_ctr      = v2;   // Slice loop counter
  int vnseg        = v3;   // Number of segments
  int vnseg_ctr    = v4;   // Segment loop counter
  int vetl         = v5;   // Number of choes in readout train
  int vetl_ctr     = v6;   // etl loop counter
  int vblip        = v7;   // Sign on blips in multi-shot experiment
  int vssepi       = v8;   // Number of Gradient Steady States lobes
  int vssepi_ctr   = v9;   // Steady State counter
  int vacquire     = v10;  // Argument for setacqvar, to skip steady states
  int vquipss      = v11;
  int vquipss_ctr  = v12;

  /******************************************************/
  /* VARIABLE INITIALIZATIONS ***************************/
  /******************************************************/
  init_mri();
  setacqmode(WACQ|NZ);  // Necessary for variable rate sampling
  use_minte = (minte[0] == 'y');
  
  getstr("slprofile",slprofile);
  
  
  /******************************************************/
  /* CALCULATIONS ***************************************/
  /******************************************************/
  if (ix == 1) {
    /* Calculate RF pulse */
    init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2); 
    calc_rf(&p1_rf,"tpwr1","tpwr1f"); 

    /* Calculate gradients:                               */
    init_slice(&ss_grad,"ss",thk);
    calc_slice(&ss_grad, &p1_rf,WRITE,"gss");

    init_slice_refocus(&ssr_grad,"ssr");
    calc_slice_refocus(&ssr_grad, &ss_grad, WRITE,"gssr");

    if (spinecho[0] == 'y') {
      init_rf(&p2_rf,p2pat,p2,flip2,rof1,rof1); 
      calc_rf(&p2_rf,"tpwr2","tpwr2f"); 
      init_slice_butterfly(&ss2_grad,"ss2",thk,gcrush,tcrush);
      calc_slice(&ss2_grad,&p2_rf,WRITE,"gss2");
    }
    else ss2_grad.duration = 0;

    init_readout(&epiro_grad,"epiro",lro,np,sw);
    init_readout_refocus(&ror_grad,"ror");
    init_phase(&epipe_grad, "epipe",lpe,nv);
    init_phase(&per_grad,"per",lpe,nv);
    init_readout(&nav_grad,"nav",lro,np,sw);
    init_epi(&epi_grad);

    if (!strcmp(orient,"oblique")) {
      if ((phi != 90) || (psi != 90) || (theta != 90)) {
	/* oblique slice - this should take care of most cases */
	epiro_grad.slewRate /= 3; /* = gmax/trise */
	epipe_grad.slewRate /= 3; 
      }
    }

    calc_epi(&epi_grad,&epiro_grad,&epipe_grad,&ror_grad,&per_grad,&nav_grad,NOWRITE);

    /* Make sure the slice refocus, readout refocus, 
       and phase dephaser fit in the same duration */
    tref = calc_sim_gradient(&ror_grad, &per_grad, &null_grad, getval("tpe"), WRITE);
    if (sgldisplay) displayEPI(&epi_grad);

    /* calc_sim_gradient recalculates per_grad, so reset its 
       base amplitude for centric ordering or fractional k-space*/
    switch(ky_order[0]) {
      case 'l':
	per_grad.amp *= (fract_ky/(epipe_grad.steps/2));
	break;
      case 'c':
	per_grad.amp = (nseg/2-1)*per_grad.increment;
	break;
    }

    if (fsat[0] == 'y') {
      create_fatsat();
    }

    /* Acquire top-down or bottom-up ? */
    if (ky_order[1] == 'r') {
      epipe_grad.amp     *= -1;
      per_grad.amp       *= -1;
      per_grad.increment *= -1;
    }


  }  /* end gradient setup if ix == 1 */

  create_asl();

  /* Load table used to determine multi-shot direction */
  settable(t2,(int) nseg,epi_grad.table2);

  /* What is happening in the 2 TE/2 periods */
  busy1 = ss_grad.rfCenterBack + ssr_grad.duration;
  busy2 = tep + nav_grad.duration*(epi_grad.center_echo + 0.5);
  if (diff[0] == 'y') busy1 += (2*diff_grad.duration);
  if (navigator[0] == 'y')
    busy2 += (tep + nav_grad.duration + per_grad.duration);

  /* How much extra time do we have in each TE/2 period? */
  if (spinecho[0] == 'y') {
    busy1    += (GDELAY + ss2_grad.rfCenterFront);
    busy2    += ss2_grad.rfCenterBack;
    temin     = MAX(busy1,busy2)*2;
    if (use_minte) te = temin;
    te_delay1 = te/2 - busy1;
    te_delay2 = te/2 - busy2;
  
    if (temin > te) { /* Use min TE and try and catch further violations of min TE */
      te_delay1 = temin/2 - busy1;
      te_delay2 = temin/2 - busy2;
    }
  }
  else { /* Gradient echo */
    temin     = (busy1 + busy2);
    if (use_minte) te = temin;
    te_delay1 = te - temin;
    te_delay2 = 0;

    if (temin > te) te_delay1 = 0; 
  }


  /* Check if TE is long enough */
  temin = ceil(temin*1e6)/1e6; /* round to nearest us */
  if (use_minte) {
    te = temin;
    putvalue("te",te);
  }
  else if (temin > te) {
    abort_message("TE too short, minimum is %.2f ms\n",temin*1000);
  }


  /* Minimum TI1 and TI */
  tmp = ssi_grad.rfCenterBack + aslcrush_grad.duration;
  if (quipss[0] == 'y') tmp += (GDELAY + sat_grad.rfCenterFront);  
  ti1min = tmp + GDELAY;
  if (ti1min > asl_ti1)
    abort_message("TI1 too short, minimum is %.2f ms\n",ti1min*1000);
  ti1_delay = asl_ti1 - tmp;
  
  tmp = asl_ti1 + GDELAY + ss_grad.rfCenterFront;
  if (quipss[0] == 'y') tmp += (quipssTime - sat_grad.rfCenterFront);  
  if (fsat[0]   == 'y') tmp += fsatTime;
  timin = tmp + GDELAY;
  if (timin > ti)
    abort_message("TI too short, minimum is %.2f ms\n",timin*1000);
  ti_delay = ti - tmp;


  /* Minimum TR per slice, w/o options */
  seqtime = GDELAY + ss_grad.rfCenterFront   // Before TE
          + te 
	  + (epiro_grad.duration - nav_grad.duration*(epi_grad.center_echo+0.5)); // After TE

  /* Add in time for options outside of TE */
  /* If asltag != 0, then the fat sat time is included in TI */
  if (asltag  !=  0)  
    seqtime += (ti + ssi_grad.rfCenterFront - ss_grad.rfCenterBack);
  else
    if (fsat[0] == 'y') seqtime += fsatTime;

	   
  trmin = seqtime + GDELAY; /* ensure a minimum of 4us in tr_delay */
  trmin *= ns;

 
  if (tr - trmin < 0.0) {
    abort_message("TR too short, minimum is %.2f ms\n",trmin*1000);
  }

  /* spread out multi-slice acquisition over total TR */
  tr_delay = (tr - ns*seqtime)/ns;

  /******************************************************/
  /* Return gradient values to VnmrJ interface */
  /******************************************************/
  putvalue("etl",epi_grad.etl+2*ssepi);
  putvalue("gro",epiro_grad.amp);
  putvalue("rgro",epiro_grad.tramp);
  putvalue("gror",ror_grad.amp);
  putvalue("tror",ror_grad.duration);
  putvalue("rgror",ror_grad.tramp);
  putvalue("gpe",epipe_grad.amp);
  putvalue("rgpe",epipe_grad.tramp);
  putvalue("gped",per_grad.amp);
  putvalue("tped",per_grad.duration);
  putvalue("rgped",per_grad.tramp);
  putvalue("gss",ss_grad.amp);
  putvalue("gss2",ss2_grad.ssamp);
  putvalue("rgss",ss_grad.tramp);
  putvalue("gssr",ssr_grad.amp);
  putvalue("tssr",ssr_grad.duration);
  putvalue("rgssr",ssr_grad.tramp);
  putvalue("rgss2",ss2_grad.crusher1RampToSsDuration);
  putvalue("rgssi",ssi_grad.crusher1RampToSsDuration);
  putvalue("rgcrush",ssi_grad.crusher1RampToCrusherDuration);
  putvalue("at_full",epi_grad.duration);
  putvalue("at_one",nav_grad.duration);
  putvalue("rcrush",ss2_grad.crusher1RampToCrusherDuration);
  putvalue("np_ramp",epi_grad.np_ramp);
  putvalue("np_flat",epi_grad.np_flat);


  /* Set all gradients depending on whether we do  */
  /* Use separate variables, because we only initialize & calculate gradients for ix==1 */
  peamp  = epipe_grad.amp;
  perinc = per_grad.increment;
  peramp = per_grad.amp;
  roamp  = epiro_grad.amp;
  roramp = ror_grad.amp;

  switch ((int)image) {
    case 1: /* Real image scan, don't change anything */
      break;
    case 0: /* Normal reference scan */
      peamp  = 0;
      perinc = 0;
      peramp = 0;
      roamp  = epiro_grad.amp;
      roramp = ror_grad.amp;
      break;
    case -1: /* Inverted image scan */
      roamp  = -epiro_grad.amp;
      roramp = -ror_grad.amp;
      break;
    case -2: /* Inverted reference scan */
      peamp  = 0;
      perinc = 0;
      peramp = 0;
      roamp  = -epiro_grad.amp;
      roramp = -ror_grad.amp;
      break;
    default: break;
  }
  
  /* Generate phase-ramped pulses: 90, 180, and IR */
  offsetlist(pss,ss_grad.ssamp,0,freq90,ns,seqcon[1]);
  shape90 = shapelist(p1pat,ss_grad.rfDuration,freq90,ns,0,seqcon[1]);

  if (spinecho[0] == 'y') {
    offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,seqcon[1]);
    shape180 = shapelist(p2pat,ss2_grad.rfDuration,freq180,ns,0,seqcon[1]);
  }
  
  sgl_error_check(sglerror);

  roff1 = -poffset(pro,epi_grad.amppos);
  roff2 = -poffset(pro,epi_grad.ampneg);
  roffn = -poffset(pro,nav_grad.amp);

  roff1 = -poffset(pro,epi_grad.amppos*roamp/epiro_grad.amp);
  roff2 = -poffset(pro,epi_grad.ampneg*roamp/epiro_grad.amp);
  roffn = -poffset(pro,nav_grad.amp);

  dw = granularity(1/sw,1/epi_grad.ddrsr);

  /* Total Scan Time */
  g_setExpTime(tr*nt*(getval("ss")+arraydim)*nseg);


  /******************************************************/
  /* PULSE SEQUENCE *************************************/
  /******************************************************/
  rotate();
   
  initval(epi_grad.etl/2, vetl);  
  /* vetl is the loop counter in the acquisition loop     */
  /* that includes both a positive and negative readout lobe */
  initval(nseg, vnseg);
  initval(-ssepi,vssepi);  /* gradient steady state lobes */

  obsoffset(resto); delay(GDELAY);
    
  loop(vnseg,vnseg_ctr);   /* Loop through segments in segmented EPI */
    msloop(seqcon[1],ns,vms_slices,vms_ctr);     /* Multislice loop */
      if (slprofile[0] == 'y') rotate(); // rotate for ASL block
      assign(vssepi,vssepi_ctr);
      sp1on(); delay(GDELAY); sp1off();  /* Output trigger to look at scope */

      if (ticks) {
        xgate(ticks);
        grad_advance(gpropdelay);
        delay(4e-6);
      }

      getelem(t2,vnseg_ctr,vblip);  /* vblip = t2[vnseg_ctr]; either 1 or -1 for pos/neg blip */

      /* Put TR delay here, otherwise asltag = 0 and asltag != 0
         have different timing */
      delay(tr_delay);

      /* ASL IR PULSE */
      if ((asltag != 0) && (asltype != TAGOFF)) {
	obspower(ir_rf.powerCoarse);
	obspwrf(ir_rf.powerFine);
	delay(GDELAY);
	obl_shapedgradient(ssi_grad.name,ssi_grad.duration,0.0,0.0,asl_ssiamp,NOWAIT);
	delay(ssi_grad.rfDelayBack);
        shapedpulselist(asl_shapeIR,ssi_grad.rfDuration,oph,rof1,rof1,seqcon[1],vms_ctr);
	delay(ssi_grad.rfDelayBack);
	
	obl_shapedgradient(aslcrush_grad.name,aslcrush_grad.duration,
	  aslcrush_grad.amp,aslcrush_grad.amp,aslcrush_grad.amp,WAIT);

        delay(ti1_delay);
	
        if (quipss[0] == 'y') {
	  
	  obspower(sat_rf.powerCoarse);
	  obspwrf(sat_rf.powerFine);
	  delay(GDELAY);
	  initval(nquipss,vquipss);
          loop(vquipss, vquipss_ctr);
	    obl_shapedgradient(sat_grad.name,sat_grad.duration,0.0,0.0,sat_grad.amp,NOWAIT);
	    delay(sat_grad.rfDelayBack);
	    shapedpulselist(asl_shapeQtag,sat_grad.rfDuration,oph,rof1,rof1,seqcon[1],vms_ctr);
	    delay(sat_grad.rfDelayBack);

	    obl_shapedgradient(qcrush_grad.name,qcrush_grad.duration,
	      qcrush_grad.amp,qcrush_grad.amp,qcrush_grad.amp,WAIT);
	  endloop(vquipss_ctr);
	}
	
        delay(ti_delay);
	
      }


      /* Optional FAT SAT */
      if (fsat[0] == 'y') fatsat();      


      obspower(p1_rf.powerCoarse);
      obspwrf(p1_rf.powerFine);
      delay(GDELAY);
      if (slprofile[0] == 'y') {
        /* Select the desired slice so it appears dark for reference, 
	   before changing the angles; 
	   no slice refocus effectively crushes signal */
	obl_shapedgradient(ss_grad.name,ss_grad.duration,0.0,0.0,ss_grad.amp,NOWAIT);
	delay(ss_grad.rfDelayFront);
	shapedpulselist(shape90,p1_rf.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
	delay(ss_grad.rfDelayBack);

	
	set_rotation_matrix(psi,phi,theta+90);
      }

      /* 90 ss degree pulse */
      obl_shapedgradient(ss_grad.name,ss_grad.duration,0.0,0.0,ss_grad.amp,NOWAIT);
      delay(ss_grad.rfDelayFront);
      shapedpulselist(shape90,p1_rf.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
      delay(ss_grad.rfDelayBack);

      /* Slice refocus */
      obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);


      /* Optional bipolar gradient for crushing vascular contribution to ASL signal */
      if (diff[0] == 'y') {
        obl_shapedgradient(diff_grad.name,diff_grad.duration,diff_grad.amp,diff_grad.amp,diff_grad.amp,WAIT);
        obl_shapedgradient(diff_grad.name,diff_grad.duration,-diff_grad.amp,-diff_grad.amp,-diff_grad.amp,WAIT);
      }

      delay(te_delay1);

      /* Optional 180 ss degree pulse with crushers */
      if (spinecho[0] == 'y') {
        obspower(p2_rf.powerCoarse);
	obspwrf(p2_rf.powerFine);
        delay(GDELAY);
        obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0.0,0.0,ss2_grad.amp,NOWAIT);
        delay(ss2_grad.rfDelayFront);
        shapedpulselist(shape180,ss2_grad.rfDuration,oph,rof1,rof1,seqcon[1],vms_ctr);
        delay(ss2_grad.rfDelayBack);
      }

      delay(te_delay2);


      /* Optional navigator echo */
      if (navigator[0] == 'y') {
        obl_shapedgradient(ror_grad.name,ror_grad.duration,roramp,0,0,WAIT);
        obl_shapedgradient(nav_grad.name,nav_grad.duration,
	                   -nav_grad.amp,0,0,NOWAIT);
        delay(tep);

        roff = roffn;   /* Set receiver offset for navigator gradient */
	delay(epi_grad.skip-alfa); /* ramp up */
	startacq(alfa);
	for(i=0;i<np/2;i++){
	  sample(dw);				
	  delay((epi_grad.dwell[i] - dw));
	}
	sample(aqtm-at);
	endacq();
	delay(epi_grad.skip - dw - (aqtm-at));
        
        /* Phase encode dephaser here if navigator echo was acquired */
        var_shapedgradient(per_grad.name,per_grad.duration,0,-peramp,0,perinc,vnseg_ctr,WAIT);
      }
      else {
        var_shapedgradient(per_grad.name,per_grad.duration,
   	            -roramp,-peramp,0,perinc,vnseg_ctr,WAIT);
      }		    
                 
      /* Start readout and phase encode gradient waveforms, NOWAIT */
      /* If alternating ky-ordering, get polarity on blips from table */
      var_shaped3gradient(epiro_grad.name,epipe_grad.name,"",  /* patterns */
                         epiro_grad.duration,                 /* duration */
                         roamp,0,0,                           /* amplitudes */
	                 peamp,vblip,                         /* step and multiplier */
	 		 NOWAIT);                             /* Don't wait */


      delay(tep);

      /* Acquisition loop */
      assign(one,vacquire);      // real-time acquire flag
      nowait_loop(epi_grad.etl/2 + ssepi,vetl,vetl_ctr); 
        ifzero(vssepi_ctr);      //vssepi_ctr = -ssepi, -ssepi+1, ..., 0, 1,2,...
	  assign(zero,vacquire); // turn on acquisition after all ss lobes
	endif(vssepi_ctr);
        incr(vssepi_ctr);
        setacqvar(vacquire);     // Set acquire flag 
	
        roff = roff1;   /* Set receiver offset for positive gradient */
	delay(epi_grad.skip-alfa); /* ramp up */
	startacq(alfa);
	for(i=0;i<np/2;i++){
	  sample(dw);	//dw = 1/sw			
	  delay((epi_grad.dwell[i] - dw));
	}
	if (aqtm > at) sample(aqtm-at);
	endacq();
	delay(epi_grad.skip - dw - (aqtm-at));

        roff = roff2;   /* Set receiver offset for negative gradient */
	delay(epi_grad.skip-alfa);
	startacq(alfa);
	for(i=0;i<np/2;i++){
	  sample(dw);
	  delay((epi_grad.dwell[i] - dw));
	}
	if (aqtm > at) sample(aqtm-at);
	endacq();
	delay(epi_grad.skip - dw - (aqtm-at));
      nowait_endloop(vetl_ctr);
      
    endmsloop(seqcon[1],vms_ctr);   /* end multislice loop */
  endloop(vnseg_ctr);                 /* end segments loop */
} /* end pulsesequence */

/*******************************************************************
                         MODIFICATION HISTORY

********************************************************************/
