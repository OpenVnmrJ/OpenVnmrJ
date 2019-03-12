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
      Diffusion weighting               |   diff

*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#include "standard.h"
#include "group.h"
#include "sgl.c"

#define GDELAY 4e-6


void pulsesequence() {
  /* Acquisition variables */
  double dw;  /* nominal dwell time, = 1/sw */
  double aqtm = getval("aqtm");

  /* Delay variables */  
  double tref,
         te_delay1, te_delay2, tr_delay, ti_delay,
         del1, del2, del3, del4, del5, /* before and after diffusion gradients  */
         busy1, busy2,      /* time spent on rf pulses etc. in TE periods       */
         seqtime, invTime;
  int    use_minte;
  
  /* RF and receiver frequency variables */
  double freq90[MAXNSLICE],freq180[MAXNSLICE],freqIR[MAXNSLICE];  /* frequencies for multi-slice */
  int    shape90=0, shape180=0, shapeIR=0; /* List ID for RF shapes */
  double roff1, roff2, roffn; /* Receiver offsets when FOV is offset along readout */
  
  /* Gradient amplitudes, may vary depending on "image" parameter */
  double peramp, perinc, peamp, roamp, roramp;
         
  /* diffusion variables */
#define MAXDIR 1024           /* Will anybody do more than 1024 directions or b-values? */
  int    diff_in_one = 0;
  double tmp, tmp_ss2;
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
  euler_test();

  if (tep < 0) { // adjust by reducing gpropdelay by that amount
    gpropdelay += tep;
    tep = 0;
  }


  setacqmode(WACQ|NZ);  // Necessary for variable rate sampling
  use_minte = (minte[0] == 'y');



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
  else ss2_grad.duration = 0;  /* used for diffusion calculations */

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

  /* What is happening in the 2 TE/2 periods (except for diffusion)? */
  busy1 = ss_grad.rfCenterBack + ssr_grad.duration;
  busy2 = tep + nav_grad.duration*(epi_grad.center_echo + 0.5);
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


  /* Now fill in the diffusion delays: 
     del1 = between 90 and 1st diffusion gradient
     del2 = after 1st diffusion gradient 
     del3 = before 2nd diffusion gradient when both in same TE/2 period
     del4 = before 2nd diffusion gradient when in different TE/2 period
     del5 = before acquisition
     
     Ie, the order is:
     90 - del1 - diff - del2 - (diff - del3) - 180 - (del4 - diff) - del5 - acq 
     where one and only one of the two options (diff - del3) or (del4 - diff) is used
  */
  if (diff[0] == 'y') {
    tmp_ss2 = GDELAY + ss2_grad.duration;  /* ss2 grad + 4us delay */
    del1 = del2 = del3 = del4 = del5 = 0;
    if (tDELTA < (diff_grad.duration + tmp_ss2))  /* Minimum DELTA */
      abort_message("ERROR %s: tDELTA is too short, minimum is %.2fms\n",
	             seqfil,(diff_grad.duration + tmp_ss2)*1000+0.005);
    if (tDELTA + diff_grad.duration > te_delay1 + tmp_ss2 + te_delay2) {
      if (!use_minte) {
        abort_message("ERROR %s: Maximum tDELTA is %.2fms",
	  seqfil,te_delay1 + ss2_grad.duration + te_delay2 - diff_grad.duration);
      }
      else {
        tmp = (tDELTA + diff_grad.duration) - (te_delay1 + tmp_ss2 + te_delay2);
	if (spinecho[0] == 'y') {
  	  te_delay1 += (tmp/2);
	  te_delay2 += (tmp/2);
	}
	else 
	  te_delay1 += tmp;
        temin += tmp;
      }
    }

    if (spinecho[0] == 'y') { 
      if (te_delay1 >= (tDELTA + diff_grad.duration)) {  /* Put them both in 1st TE/2 period, */
        diff_in_one = (diff[0] == 'y');     /* no need to increase temin */
        del2 = tDELTA - diff_grad.duration; /* time between diffusion gradients */
        del3 = te_delay1 - (tDELTA+diff_grad.duration);  /* time after diffusion gradients   */
        del5 = te_delay2;                   /* delay in second TE/2 period      */
      }
      else {  /* put them on each side of the 180 */
        diff_in_one = 0;
	busy1 += diff_grad.duration;
	busy2 += diff_grad.duration;
	temin  = 2*MAX(busy1,busy2);

        /* Optimally, the 2nd diff grad is right after the 180 */
        del2 = tDELTA - diff_grad.duration - tmp_ss2; /* This is always > 0, or we would have aborted above */

	del1 = te_delay1 - (diff_grad.duration + del2);
	if (del1 < 0) {
	  del1 = 0;  /* Place the 1st right after the 90 and push the 2nd out */
	  del4 = tDELTA - te_delay1 - ss2_grad.duration; 
	}
	del5 = te_delay2 - (del4 + diff_grad.duration);
	/* del5 could still be < 0, when te_delay2 < diff_grad.duration */
	if (del5 < 0) {
	  del1  += fabs(del5);  /* Increase each TE/2 period by abs(del5) */
	  del5   = 0;
	}
      }
    }
    else { /* gradient echo */
      diff_in_one = (diff[0] == 'y');
      del1   = 0;
      del2   = tDELTA - diff_grad.duration; /* time between diffusion gradients */
      del3   = 0;
      del4   = 0;
      
      if (!use_minte) /* user defined TE */
        del5 = te_delay1 - (tDELTA + diff_grad.duration);
    }
  } /* End of Diffusion block */
  else {
    del1 = te_delay1;
    del5 = te_delay2;
    del2 = del3 = del4 = 0;
  }
  
  if (sgldisplay) {
    text_message("busy1/2, temin = %f, %f, %f",busy1*1e3, busy2*1e3, temin*1e3);
    text_message("te_delay1/2 = %f, %f",te_delay1*1e3, te_delay2*1e3);
    text_message("delays 1-5: %.2f, %.2f, %.2f, %.2f, %.2fms\n",del1*1000,del2*1000,del3*1000,del4*1000,del5*1000);
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
    abort_message("%s: Requested tr too short.  Min tr = %.2f ms\n",
                  seqfil,ceil(trmin*100000)/100.00);
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
      abort_message("%s: Number of directions/b-values must be the same for all axes (readout)",seqfil);
    if ((nbpe != nbval) && (nbpe != 1))
      abort_message("%s: Number of directions/b-values must be the same for all axes (phase)",seqfil);
    if ((nbsl != nbval) && (nbsl != 1))
      abort_message("%s: Number of directions/b-values must be the same for all axes (slice)",seqfil);

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
    dgss2  = p2/2;   Dgss2  = dgss2;
    dcrush = tcrush; Dcrush = dcrush + p2;
    bsl[i] = bval(gdiff*slarr[i],tdelta,tDELTA);
    if (spinecho[0] == 'y') {
      bsl[i] += bval(ss2_grad.ssamp,dgss2,Dgss2);
      bsl[i] += bval(gcrush,dcrush,Dcrush);
      bsl[i] += bval_nested(gcrush,dcrush,Dcrush,ss2_grad.ssamp,dgss2,Dgss2);
    }
    if (!diff_in_one) {
      bsl[i] += bval_nested(gdiff*slarr[i],tdelta,tDELTA,gcrush,dcrush,Dcrush);
      bsl[i] += bval_nested(gdiff*slarr[i],tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
    }

    /* Readout/Slice Cross-terms */
    brs[i]  = bval2(gdiff*roarr[i],gdiff*slarr[i],tdelta,tDELTA);
    if (spinecho[0] == 'y') {
      brs[i] += bval_cross(gdiff*roarr[i],tdelta,tDELTA,gcrush,dcrush,Dcrush);
      brs[i] += bval_cross(gdiff*roarr[i],tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
    }

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
  
  /* Generate phase-ramped pulses: 90, 180, and IR */
  offsetlist(pss,ss_grad.ssamp,0,freq90,ns,seqcon[1]);
  shape90 = shapelist(p1pat,ss_grad.rfDuration,freq90,ns,0,seqcon[1]);

  if (spinecho[0] == 'y') {
    offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,seqcon[1]);
    shape180 = shapelist(p2pat,ss2_grad.rfDuration,freq180,ns,0,seqcon[1]);
  }
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
  g_setExpTime(tr*nt*nseg*arraydim);

  /******************************************************/
  /* PULSE SEQUENCE *************************************/
  /******************************************************/
  rotate();
   
  F_initval(epi_grad.etl/2, vetl);  
  /* vetl is the loop counter in the acquisition loop     */
  /* that includes both a positive and negative readout lobe */
  F_initval(nseg, vnseg);
  /* NB. F_initval(-ssepi,vssepi); currently gives errors */
  initval(-ssepi,vssepi);  /* gradient steady state lobes */

  obsoffset(resto); delay(GDELAY);

  ifzero(rtonce); grad_advance(gpropdelay); endif(rtonce);
    
  loop(vnseg,vnseg_ctr);   /* Loop through segments in segmented EPI */
    msloop(seqcon[1],ns,vms_slices,vms_ctr);     /* Multislice loop */
      assign(vssepi,vssepi_ctr);
      sp1on(); delay(4e-6); sp1off();  /* Output trigger to look at scope */

      if (ticks) {
        xgate(ticks);
        grad_advance(gpropdelay);
        delay(4e-6);
      }

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

      /* 90 ss degree pulse */
      obspower(p1_rf.powerCoarse);
      obspwrf(p1_rf.powerFine);
      delay(GDELAY);
      obl_shapedgradient(ss_grad.name,ss_grad.duration,0.0,0.0,ss_grad.amp,NOWAIT);
      delay(ss_grad.rfDelayFront);
      shapedpulselist(shape90,p1_rf.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
      delay(ss_grad.rfDelayBack);

      /* Slice refocus */
      obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);

      delay(del1);    

      if (diff[0] == 'y') 
        obl_shapedgradient(diff_grad.name,diff_grad.duration,
	      diff_grad.amp*dro,diff_grad.amp*dpe,diff_grad.amp*dsl,WAIT);

      delay(del2);

      if (diff_in_one)
        obl_shapedgradient(diff_grad.name,diff_grad.duration,
	      -diff_grad.amp*dro,-diff_grad.amp*dpe,-diff_grad.amp*dsl,WAIT);

      delay(del3);
	
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

      delay(del4);      

      if ((diff[0] == 'y') && !diff_in_one)
        obl_shapedgradient(diff_grad.name,diff_grad.duration,
	      diff_grad.amp*dro,diff_grad.amp*dpe,diff_grad.amp*dsl,WAIT);

      delay(del5);


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
      
      delay(tr_delay);
    endmsloop(seqcon[1],vms_ctr);   /* end multislice loop */
  endloop(vnseg_ctr);                 /* end segments loop */
} /* end pulsesequence */

/*******************************************************************
                         MODIFICATION HISTORY

********************************************************************/
