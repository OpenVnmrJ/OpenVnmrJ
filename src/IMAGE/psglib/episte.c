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
      Diffusion weighting               |   diff

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
         te_delay1, te_delay2, tr_delay, ti_delay,tm_delay,
         del1, del2, del3, del4, /* before and after diffusion gradients  */
         busy1, busy2,      /* time spent on rf pulses etc. in TE periods       */
	 tm1, tmmin,        /* Min TM calculations */
         seqtime, invTime;
  int    use_minte;
  
  /* RF and receiver frequency variables */
  double freq90[MAXNSLICE],freqIR[MAXNSLICE];  /* frequencies for multi-slice */
  int    shape90=0, shapeIR=0; /* List ID for RF shapes */
  double roff1, roff2, roffn; /* Receiver offsets when FOV is offset along readout */
  
  /* Gradient amplitudes, may vary depending on "image" parameter */
  double peramp, perinc, peamp, roamp, roramp;
         
  /* diffusion variables */
#define MAXDIR 1024           /* Will anybody do more than 1024 directions or b-values? */
  double tmp, dmin, dmax;
  double roarr[MAXDIR], pearr[MAXDIR], slarr[MAXDIR];
  int    nbval,               /* Total number of bvalues*directions */
         nbro, nbpe, nbsl;    /* bvalues*directions along RO, PE, and SL */
  double bro[MAXDIR], bpe[MAXDIR], bsl[MAXDIR], /* b-values along RO, PE, SL */
         brs[MAXDIR], brp[MAXDIR], bsp[MAXDIR], /* the cross-terms */
	 btrace[MAXDIR],                        /* and the trace */
	 max_bval=0,
	 dcrush, dgss2,       /* "delta" for crusher and gss2 gradients */
         Dro, Dcrush, Dgss2;  /* "DELTA" for readout, crusher and gss2 gradients */

  /* loop variable */
  int    i;


  /* Real-time variables used in this sequence **************/
  int vms_slices   = v3;   // Number of slices
  int vms_ctr      = v4;   // Slice loop counter
  int vnseg        = v5;   // Number of segments
  int vnseg_ctr    = v6;   // Segment loop counter
  int vetl         = v7;   // Number of choes in readout train
  int vetl_ctr     = v8;   // etl loop counter
  int vblip        = v9;   // Sign on blips in multi-shot experiment
  int vssepi       = v10;  // Number of Gradient Steady States lobes
  int vssepi_ctr   = v11;  // Steady State counter
  int vacquire     = v12;  // Argument for setacqvar, to skip steady states

  /******************************************************/
  /* VARIABLE INITIALIZATIONS ***************************/
  /******************************************************/
  get_parameters();
  setacqmode(WACQ|NZ);  // Necessary for variable rate sampling
  use_minte = (minte[0] == 'y');
  
  /******************************************************/
  /* CALCULATIONS ***************************************/
  /******************************************************/
if (ix == 1) {
  /* Calculate RF pulse */
  /* The same RF pulse will be used for all 3 pulses    */
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2); 
  calc_rf(&p1_rf,"tpwr1","tpwr1f"); 

  /* Slice  selective gradient                          */
  init_slice(&ss_grad,"ss",thk);
  calc_slice(&ss_grad, &p1_rf,WRITE,"gss");

  init_slice_refocus(&ssr_grad,"ssr");
  calc_slice_refocus(&ssr_grad, &ss_grad, WRITE,"gssr");

  /* Gradients for 2nd & 3rd pulses, with crushers      */
  init_slice_butterfly(&ss2_grad,"ss2",thk,gcrush,tcrush);
  calc_slice(&ss2_grad, &p1_rf,WRITE,"gss");
  
  /* Spoiler for TM period */
  init_generic(&spoil_grad,"TMspoil",gspoil,tspoil);
  calc_generic(&spoil_grad,WRITE,"","");

  /* Readout and Phase gradients                        */
  init_readout(&epiro_grad,"epiro",lro,np,sw);  // The entire readout train
  init_readout_refocus(&ror_grad,"ror");        // refocusing
  init_phase(&epipe_grad, "epipe",lpe,nv);      // The entire blip train
  init_phase(&per_grad,"per",lpe,nv);           // Initial dephaser
  init_readout(&nav_grad,"nav",lro,np,sw);      // Navigator
  init_epi(&epi_grad);

  if (!strcmp(orient,"oblique")) {
    if ((phi != 90) || (psi != 90) || (theta != 90)) {
      /* oblique slice - this should take care of most cases */
      epiro_grad.slewRate /= 3; /* = gmax/trise */
      epipe_grad.slewRate /= 3; 
    }
  }
  
  calc_epi(&epi_grad,&epiro_grad,&epipe_grad,&ror_grad,&per_grad,&nav_grad,NOWRITE);

  /* Make sure the readout refocus and phase dephaser fit in the same duration */
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

  if (ir[0] == 'y') {
    init_rf(&ir_rf,pipat,pi,flipir,rof1,rof1); 
    calc_rf(&ir_rf,"tpwri","tpwrif"); 
    init_slice_butterfly(&ssi_grad,"ssi",thk,gcrush,tcrush);
    calc_slice(&ssi_grad,&ir_rf,WRITE,"gssi");
  }
  if (fsat[0] == 'y') {
    create_fatsat();
  }

  if (diff[0] == 'y') {
    init_generic(&diff_grad,"diff",gdiff,tdelta);
    diff_grad.maxGrad = gmax;
    calc_generic(&diff_grad,NOWRITE,"","");
    /* adjust duration, so tdelta is from start ramp up to start ramp down */
    if (ix == 1) {
      diff_grad.duration += diff_grad.tramp; 
      calc_generic(&diff_grad,WRITE,"","");
    }
  }
  

  /* Acquire top-down or bottom-up ? */
  if (ky_order[1] == 'r') {
    epipe_grad.amp     *= -1;
    per_grad.amp       *= -1;
    per_grad.increment *= -1;
  }


}  /* end gradient setup if ix == 1 */


  /* Load table used to determine multi-shot direction */
  settable(t2,(int) nseg,epi_grad.table2);

  /* Minimum TE */
  busy1 = ss_grad.rfCenterBack + ssr_grad.duration + ss2_grad.rfCenterFront;
  busy2 = ss2_grad.rfCenterBack + per_grad.duration
        + tep + nav_grad.duration*(epi_grad.center_echo + 0.5);

  if (navigator[0] == 'y')
    busy2 += (ror_grad.duration+ nav_grad.duration + tep);

  if (diff[0] == 'y') {
    busy1 += diff_grad.duration;
    busy2 += diff_grad.duration;
  }

  temin = 2*(MAX(busy1,busy2) + 4e-6);
  if (use_minte) {
    te = temin;
    putvalue("te",te);
  }
  if ((temin - te) > 12.5e-9) {
    abort_message("TE too short.  Minimum TE= %.2fms\n",temin*1000);   
  }
  te_delay1 = te/2 - busy1;
  te_delay2 = te/2 - busy2;


  /* Calculate TM delay */
  tm1 = ss2_grad.duration + spoil_grad.duration;
  tmmin = tm1 + 4e-6;
  if (mintm[0] == 'y') {
    tm = tmmin;
    putvalue("tm",tm);
  }
  if ((tmmin - tm) > 12.5e-9) 
    abort_message("Requested TM too short.  Min TM = %.2f ms\n",
                  ceil(tmmin*100000)/100.00);
  tm_delay = tm - tm1;

  /* Now fill in the diffusion delays: 
     del1 = between 90 and 1st diffusion gradient
     del2 = between 1st diffusion gradient and 2nd RF
     del3 = between 3rd RF and 2nd diffusion gradient
     del4 = between 2nd diffusion gradient and acquisition
     
     Ie, the order is:
     90 - del1 - diff - del2 - 90 - TM - 90 - del3 - diff - del4 - acq 
  */
  del1 = te_delay1;
  del2 = 0;
  del3 = te_delay2;
  del4 = 0;
  if (diff[0] == 'y') {
    dmin = diff_grad.duration + ss2_grad.duration + tm;  // minimum DELTA
    if (tDELTA < dmin)
      abort_message("tDELTA is too short, increase to %.2fms or reduce TM\n",
	             dmin*1000+0.005);

    dmax = te_delay1 + dmin + te_delay2; // Maximum DELTA
    if (tDELTA > dmax) {
      if (mintm[0] == 'y') {
        // increase TM to fit DELTA
	tm += (tDELTA - dmax);
        tm_delay = tm - tm1;
        putvalue("tm",tm);
	
	// recalculate minimum DELTA
        dmin = diff_grad.duration + ss2_grad.duration + tm;
      }
      else
        abort_message("Maximum tDELTA is %.2fms, increase TM or TE",
	               dmax*1000);
    }

    /* Ideally, the 2nd diff grad is right after the RF pulse */
    del3 = 0;
    del4 = te_delay2;
    
    del2 = tDELTA - dmin;
    del1 = te_delay1 - del2;
    if (del1 < 0) {
      tmp  = -del1; // How much is missing?
      del1 = 0;     // Place the 1st right after the 90 and push the 2nd out
      del2 = te_delay1;
      del3 = tmp;
      del4 = te_delay2 - del3;
    }

  } /* End of Diffusion block */
  if ((del1 < -12.5e-9) || (del2 < -12.5e-9) || (del3 < -12.5e-9) || (del4 < -12.5e-9)) {
    abort_message("problem with delays: %.2f, %.2f, %.2f, %.2fms\n",
      del1*1000,del2*1000,del3*1000,del4*1e6);
  }

  if (ir[0] == 'y') {
    ti_delay = ti - (pi*ssi_grad.rfFraction + rof2 + ssi_grad.rfDelayBack)
                  - (ss_grad.rfDelayFront + rof1 + p1*(1-ss_grad.rfFraction));
    if (ti_delay < 0) {
      abort_message("TI too short, minimum is %.2f ms\n",(ti-ti_delay)*1000);
    }
  }
  else ti_delay = 0;
  invTime = GDELAY + ssi_grad.duration + ti_delay;

  /* Minimum TR per slice, w/o options */
  seqtime = GDELAY + ss_grad.rfCenterFront   // Before TE
          + te 
	  + (epiro_grad.duration - nav_grad.duration*(epi_grad.center_echo+0.5)); // After TE


  /* Add in time for options outside of TE */
  if (ir[0]        == 'y') seqtime += invTime;
  if (fsat[0]      == 'y') seqtime += fsatTime;
	   
  trmin = seqtime + 4e-6; /* ensure a minimum of 4us in tr_delay */
  trmin *= ns;

  if (tr - trmin < 0.0) {
    abort_message("Requested TR too short.  Min TR = %.2f ms\n",
                  ceil(trmin*100000)/100.00);
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

  if (diff[0] == 'y') {  /* CALCULATE B VALUES */
    /* Get multiplication factors and make sure they have same # elements */
    /* All this is only necessary because putCmd only work for ix==1      */
    nbro = (int) getarray("dro",roarr);  nbval = nbro;
    nbpe = (int) getarray("dpe",pearr);  if (nbpe > nbval) nbval = nbpe;
    nbsl = (int) getarray("dsl",slarr);  if (nbsl > nbval) nbval = nbsl;
    if ((nbro != nbval) && (nbro != 1))
      abort_message("Number of directions/b-values must be the same for all axes (readout)");
    if ((nbpe != nbval) && (nbpe != 1))
      abort_message("Number of directions/b-values must be the same for all axes (phase)");
    if ((nbsl != nbval) && (nbsl != 1))
      abort_message("Number of directions/b-values must be the same for all axes (slice)");

    if (nbro == 1) for (i = 1; i < nbval; i++) roarr[i] = roarr[0];
    if (nbpe == 1) for (i = 1; i < nbval; i++) pearr[i] = pearr[0];
    if (nbsl == 1) for (i = 1; i < nbval; i++) slarr[i] = slarr[0];
  }
  else {
    nbval = 1;
    roarr[0] = 0;
    pearr[0] = 0;
    slarr[0] = 0;
  }


  for (i = 0; i < nbval; i++)  {
    /* We need to worry about slice gradients & crushers for slice gradients */
    /* Everything else is outside diffusion gradients, and thus constant     */
    /* for all b-values/directions                                           */
     
    /* Readout */
    bro[i]  = bval(gdiff*roarr[i],tdelta,tDELTA);

    /* Phase */
    bpe[i] = bval(gdiff*pearr[i],tdelta,tDELTA);

    /* Slice */
    dgss2  = ss2_grad.rfCenterFront;   Dgss2  = dgss2 + tm;
    dcrush = tcrush; Dcrush = dcrush + ss2_grad.duration + tm;
    bsl[i]  = bval(gdiff*slarr[i],tdelta,tDELTA);  // Diffusion gradient
    bsl[i] += bval(ss2_grad.ssamp,dgss2,Dgss2);    // Slice gradient
    bsl[i] += bval(gcrush,dcrush,Dcrush);          // Crusher gradient
    bsl[i] += bval_nested(gdiff*slarr[i],tdelta,tDELTA,gcrush,dcrush,Dcrush);
    bsl[i] += bval_nested(gdiff*slarr[i],tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
    bsl[i] += bval_nested(gcrush,dcrush,Dcrush,ss2_grad.ssamp,dgss2,Dgss2);

    /* Readout/Slice Cross-terms */
    brs[i]  = bval2(gdiff*roarr[i],gdiff*slarr[i],tdelta,tDELTA);
    brs[i] += bval_cross(gdiff*roarr[i],tdelta,tDELTA,gcrush,dcrush,Dcrush);
    brs[i] += bval_cross(gdiff*roarr[i],tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);

    /* Readout/Phase Cross-terms */
    brp[i]  = bval2(gdiff*roarr[i],gdiff*pearr[i],tdelta,tDELTA);

    /* Slice/Phase Cross-terms */
    bsp[i]  = bval2(gdiff*slarr[i],gdiff*pearr[i],tdelta,tDELTA);
    bsp[i] += bval_cross(gdiff*pearr[i],tdelta,tDELTA,gcrush,dcrush,Dcrush);
    bsp[i] += bval_cross(gdiff*pearr[i],tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);

    btrace[i] = (bro[i]+bsl[i]+bpe[i]);

    if (max_bval < btrace[i]) {
      max_bval = (bro[i]+bsl[i]+bpe[i]);
    }


  }  /* End for-all-directions */


  putarray("bvalrr",bro,nbval);
  putarray("bvalpp",bpe,nbval);
  putarray("bvalss",bsl,nbval);
  putarray("bvalrp",brp,nbval);
  putarray("bvalrs",brs,nbval);
  putarray("bvalsp",bsp,nbval);

  putarray("bvalue",btrace,nbval);

  putvalue("max_bval",max_bval);



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
  
  if (navigator[0] == 'y') roramp *= -1;  // change sign

  /* Generate phase-ramped pulses: 90, 180, and IR */
  offsetlist(pss,ss_grad.ssamp,0,freq90,ns,seqcon[1]);
  shape90 = shapelist(p1pat,ss_grad.rfDuration,freq90,ns,0,seqcon[1]);

  if (ir[0] == 'y') {
    offsetlist(pss,ssi_grad.ssamp,0,freqIR,ns,seqcon[1]);
    shapeIR = shapelist(pipat,ssi_grad.rfDuration,freqIR,ns,0,seqcon[1]);
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
  g_setExpTime(tr*nt*arraydim);

  /******************************************************/
  /* PULSE SEQUENCE *************************************/
  /******************************************************/
  status(A);
  rotate();
   
  initval(epi_grad.etl/2, vetl);  
  /* vetl is the loop counter in the acquisition loop     */
  /* that includes both a positive and negative readout lobe */
  initval(nseg, vnseg);
  initval(-ssepi,vssepi);  /* gradient steady state lobes */

  obsoffset(resto); delay(GDELAY);
    
  loop(vnseg,vnseg_ctr);   /* Loop through segments in segmented EPI */
    msloop(seqcon[1],ns,vms_slices,vms_ctr);     /* Multislice loop */
      assign(vssepi,vssepi_ctr);
      sp1on(); delay(4e-6); sp1off();  /* Output trigger to look at scope */

      xgate(ticks);
      delay(4e-6);
      getelem(t2,vnseg_ctr,vblip);  /* vblip = t2[vnseg_ctr]; either 1 or -1 for pos/neg blip */

      /* Optional FAT SAT */
      if (fsat[0] == 'y') {
        fatsat();
      }
      

      /* Optional IR + TI delay */
      if (ir[0] == 'y') {
        obspower(ir_rf.powerCoarse);
	obspwrf(ir_rf.powerFine);
        delay(GDELAY);
        obl_shapedgradient(ssi_grad.name,ssi_grad.duration,0.0,0.0,ssi_grad.amp,NOWAIT);
        delay(ssi_grad.rfDelayBack);
        shapedpulselist(shapeIR,ssi_grad.rfDuration,oph,rof1,rof1,seqcon[1],vms_ctr);
        delay(ssi_grad.rfDelayBack);
        delay(ti_delay);
      }

      /* Excitation pulse */
      obspower(p1_rf.powerCoarse);
      obspwrf(p1_rf.powerFine);
      delay(GDELAY);
      obl_shapedgradient(ss_grad.name,ss_grad.duration,0.0,0.0,ss_grad.amp,NOWAIT);
      delay(ss_grad.rfDelayFront);
      shapedpulselist(shape90,p1_rf.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
      delay(ss_grad.rfDelayBack);

      /* Slice refocus here to minimize diffusion contribution */
      obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);
	
      delay(del1);    

      if (diff[0] == 'y') 
        obl_shapedgradient(diff_grad.name,diff_grad.duration,
	      diff_grad.amp*dro,diff_grad.amp*dpe,diff_grad.amp*dsl,WAIT);

      delay(del2);


      /* Store magnitization along Z */
      obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0.0,0.0,ss2_grad.amp,NOWAIT);
      delay(ss2_grad.rfDelayFront);
      shapedpulselist(shape90,p1_rf.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
      delay(ss2_grad.rfDelayBack);


      /* TM period */
      obl_shapedgradient(spoil_grad.name,spoil_grad.duration,
        spoil_grad.amp,spoil_grad.amp,spoil_grad.amp,WAIT);
      delay(tm_delay);


      /* Bring magnetization back in XY */
      obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0.0,0.0,ss2_grad.amp,NOWAIT);
      delay(ss2_grad.rfDelayFront);
      shapedpulselist(shape90,p1_rf.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
      delay(ss2_grad.rfDelayBack);


      delay(del3);

      if (diff[0] == 'y')
        obl_shapedgradient(diff_grad.name,diff_grad.duration,
	      diff_grad.amp*dro,diff_grad.amp*dpe,diff_grad.amp*dsl,WAIT);

      delay(del4);


      /* Optional navigator echo */
      if (navigator[0] == 'y') {
        // readout dephaser
        obl_shapedgradient(ror_grad.name,ror_grad.duration,-roramp,0,0,WAIT);
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

        /* Phase encoding dephaser */
        var_shapedgradient(per_grad.name,per_grad.duration,0,-peramp,0,perinc,vnseg_ctr,WAIT);
      }
      else {
        /* Readout refocus here to minimize diffusion contribution */
        /* Combine with phase encoding gradient */
        var_shapedgradient(per_grad.name,per_grad.duration,-roramp,-peramp,0,perinc,vnseg_ctr,WAIT);

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
      
      delay(tr_delay);
    endmsloop(seqcon[1],vms_ctr);   /* end multislice loop */
  endloop(vnseg_ctr);                 /* end segments loop */
} /* end pulsesequence */

/*******************************************************************
                         MODIFICATION HISTORY

********************************************************************/
