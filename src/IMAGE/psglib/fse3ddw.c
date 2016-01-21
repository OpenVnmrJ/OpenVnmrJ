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
#include <standard.h>
#include "sgl.c"


static int 
  crro[8]   = {0,0,0,0,0,0,0,0},        // original
  crpe[8]   = {1,1,1,1,1,1,1,1},	// original
  crss[8]   = {0,0,0,0,0,0,0,0};	// original   


void pulsesequence() {
  /* Internal variable declarations *************************/
  int     shapelist90,shapelist180;
  double  seqtime,tau1,tau2,tau3,te1_delay,te2_delay,te3_delay,tr_delay;
  double  freq90[MAXNSLICE], freq180[MAXNSLICE];
  
  /* Diffusion variables */
  double  te1, te1min, del1, del2, del3, del4;
  double  te_diff1, te_diff2, tmp1, tmp2;
  double  diffamp;
  char    diffpat[MAXSTR];
  
  /* Navigator variables */
  double  etlnav;
  
  /* Variable crushers */
  double  cscale;
  double  vcrush;  // flag

  /* Diffusion parameters */
#define MAXDIR 1024           /* Will anybody do more than 1024 directions or b-values? */
  double roarr[MAXDIR], pearr[MAXDIR], slarr[MAXDIR];
  int    nbval,               /* Total number of bvalues*directions */
         nbro, nbpe, nbsl,
	 i;    
  double bro[MAXDIR], bpe[MAXDIR], bsl[MAXDIR], /* b-values along RO, PE, SL */
         brs[MAXDIR], brp[MAXDIR], bsp[MAXDIR], /* and the cross-terms */
	 btrace[MAXDIR],                        /* and the trace */
	 max_bval=0,
         dcrush, dgss2,       /* "delta" for crusher and gss2 gradients */
         Dro, Dcrush, Dgss2;  /* "DELTA" for readout, crusher and gss2 gradients */

  /* Real-time variables used in this sequence **************/
  int  vpe_ctr     = v1;      // PE loop counter
  int  vpe_mult    = v2;      // PE multiplier, ranges from -PE/2 to PE/2
  int  vpe2_ctr    = v3;      // PE loop counter
  int  vpe2_mult   = v4;      // PE multiplier, ranges from -PE/2 to PE/2
  int  vpe2_offset = v5;
  int  vpe2_steps  = v6;
  int  vms_slices  = v7;      // Number of slices
  int  vms_ctr     = v8;      // Slice loop counter
  int  vseg        = v9;      // Number of ETL segments 
  int  vseg_ctr    = v10;      // Segment counter
  int  vetl        = v11;      // Echo train length
  int  vetl_ctr    = v12;      // Echo train loop counter
  int  vssc        = v13;     // Compressed steady-states
  int  vtrimage    = v14;     // Counts down from nt, trimage delay when 0
  int  vacquire    = v15;     // Argument for setacqvar, to skip steady state acquires
  int  vphase180   = v16;     // phase of 180 degree refocusing pulse
  int  vetl_loop   = v17;     // Echo train length MINUS ONE, used on etl loop
  int  vnav        = v18;     // Echo train length
  int  vcr_ctr     = v19;     // variable crusher, index into table
  int  vcr1        = v20;     // multiplier along RO
  int  vcr2        = v21;     // multiplier along PE
  int  vcr3        = v22;     // multiplier along SL
  int  vetl1       = v23;     // = etl-1, determine navigator echo location in echo loop
  int  vcr_reset   = v24;     // check for navigator echoes, reset crushers

  /* Initialize paramaters **********************************/
  get_parameters();
  te1    = getval("te1");     /* te1 is the echo time for the first echo */
  cscale = getval("cscale");  /* Scaling factor on first 180 crushers */
  vcrush = getval("vcrush");  /* Variable crusher or set amplitude? */
  getstr("diffpat",diffpat);


  /*  Load external PE table ********************************/
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
  } else {
    abort_message("petable undefined");
  }

  /* Hold variable crushers in tables 5, 6, 7 */
  settable(t5,8,crro);
  settable(t6,8,crpe);    
  settable(t7,8,crss);
    
  seqtime = 0.0;
  espmin = 0.0;

  /* RF Power & Bandwidth Calculations **********************/
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);
  init_rf(&p2_rf,p2pat,p2,flip2,rof1,rof2);
//  shape_rf(&p1_rf,"p1",p1pat,p1,flip1,rof1,rof2);
//  shape_rf(&p2_rf,"p2",p2pat,p2,flip2,rof1,rof2);
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");
 
  /* Initialize gradient structures *************************/
  init_readout(&ro_grad,"ro",lro,np,sw); 
  init_readout_refocus(&ror_grad,"ror");
  init_phase(&pe_grad,"pe",lpe,nv);
  init_phase(&pe2_grad,"pe2",lpe2,nv2);
  init_slice(&ss_grad,"ss",thk);   /* NOTE assume same band widths for p1 and p2 */     
  init_slice(&ss2_grad,"ss2",thk);   /* not butterfly, want to scale crushers w/ echo */
  init_slice_refocus(&ssr_grad,"ssr");

  /* Gradient calculations **********************************/
  calc_readout(&ro_grad,WRITE,"gro","sw","at");
  calc_readout_refocus(&ror_grad,&ro_grad,NOWRITE,"gror");
  calc_phase(&pe_grad,NOWRITE,"gpe","tpe");
  calc_phase(&pe2_grad,NOWRITE,"gpe2","tpe2");
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice(&ss2_grad,&p1_rf,WRITE,"");
  calc_slice_refocus(&ssr_grad,&ss_grad,WRITE,"gssr");

  /* Equalize refocus and PE gradient durations *************/
  calc_sim_gradient(&ror_grad,&pe_grad,&pe2_grad,0.0,WRITE);

  /* Variable crusher */
  init_generic(&crush_grad,"crush",gcrush,tcrush);
  calc_generic(&crush_grad,WRITE,"","");

  /* Create optional prepulse events ************************/
  if (sat[0]  == 'y') create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (mt[0]   == 'y') create_mtc();
  
  /* Optional Diffusion gradient */
  if (diff[0] == 'y') {
    init_generic(&diff_grad,"diff",gdiff,tdelta);
    if (!strcmp("sine",diffpat)) {
      diff_grad.shape = SINE;
      diffamp         = gdiff*1;
    }
 
    /* adjust duration, so tdelta is from start ramp up to start ramp down */   
    if ((ix == 1) && (diff_grad.shape == TRAPEZOID)) {
      calc_generic(&diff_grad,NOWRITE,"","");
      diff_grad.duration += diff_grad.tramp; 
    }
    calc_generic(&diff_grad,WRITE,"","");  
  }

  /* Set up frequency offset pulse shape list ********/
  offsetlist(pss,ss_grad.amp,0,freq90,ns,seqcon[1]);
  offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,seqcon[1]);
  shapelist90  = shapelist(p1_rf.pulseName,ss_grad.rfDuration, freq90, ns,0,seqcon[1]);
  shapelist180 = shapelist(p2_rf.pulseName,ss2_grad.rfDuration,freq180,ns,0,seqcon[1]);

  /* same slice selection gradient and RF pattern used */
  if (ss_grad.rfFraction != 0.5)
    abort_message("RF pulse must be symmetric (RF fraction = %.2f)",ss_grad.rfFraction);
  if (ro_grad.echoFraction != 1)
    abort_message("Echo Fraction must be 1");


  /*****************************************************/
  /* TIMING FOR ECHOES *********************************/
  /*****************************************************/
  /* First echo time, without diffusion */
  tau1 = ss_grad.rfCenterBack + ssr_grad.duration + crush_grad.duration + ss2_grad.rfCenterFront;
  tau2 = ss2_grad.rfCenterBack + crush_grad.duration + pe_grad.duration + ro_grad.timeToEcho;
  te1min = 2*MAX(tau1,tau2);
  if (te1 < te1min + 2*4e-6) {
    abort_message("First echo time too small, minimum is %.2fms\n",(te1min+2*4e-6)*1000);
  }

  /* Each half-echo period in the ETL loop ********/
  tau3 = ro_grad.timeFromEcho + pe_grad.duration + crush_grad.duration + ss2_grad.rfCenterFront;
  espmin = 2*MAX(tau2,tau3);   // Minimum echo spacing
  if (minesp[0] == 'y') {
    esp = espmin + 2*4e-6;
    putvalue("esp",esp);
  }
  if (esp - (espmin + 2*4e-6) < -12.5e-9) {
    abort_message("Echo spacing too small, minimum is %.2fms\n",(espmin+2*4e-6)*1000);
  }


  te1_delay = te1/2.0 - tau1;
  te2_delay = esp/2.0 - tau2;
  te3_delay = esp/2.0 - tau3;


  /*****************************************************/
  /* TIMING FOR DIFFUSION ******************************/
  /*****************************************************/
  del1 = te1/2.0 - tau1;
  del2 = 0;
  del3 = te1/2.0 - tau2;
  del4 = 0;

  if (diff[0] == 'y') {
    tau1 += diff_grad.duration;
    tau2 += diff_grad.duration;

    te1min = 2*MAX(tau1,tau2);
    if (te1 < te1min + 4*4e-6) {  /* te1 is split into 4 delays, each of which must be >= 4us */
      abort_message("ERROR %s: First echo time too small, minimum is %.2fms\n",seqfil,te1min*1000);
    }

    /* te1 is the echo time for the first echo */
    te_diff1 = te1/2 - tau1;  /* Available time in first half of first echo */
    te_diff2 = te1/2 - tau2;  /* Available time in second half of first echo */

    tmp1 = ss2_grad.duration + 2*crush_grad.duration;  /* duration of 180 block */
    /* Is tDELTA long enough? */
    if (tDELTA < diff_grad.duration + tmp1)
      abort_message("DELTA too short, increase to %.2fms",
        (diff_grad.duration + tmp1)*1000);

    /* Is tDELTA too long? */
    tmp2 = diff_grad.duration + te_diff1 + tmp1 + te_diff2;
    if (tDELTA > tmp2) {
      abort_message("DELTA too long, increase te1 to %.2fms",
        (te1 + (tDELTA-tmp2))*1000);
    }

    /* First attempt to put lobes right after slice select, ie del1 = 0 */
    del1 = 4e-6;  /* At least 4us after setting power for 180 */
    del2 = te_diff1 - del1;
    del3 = tDELTA - (diff_grad.duration + del2 + tmp1);
    
    if (del3 < 4e-6) {  /* shift diffusion block towards acquisition */
      del3 = 4e-6;
      del2 = tDELTA - (diff_grad.duration + tmp1 + del3);
    }

    del1 = te_diff1 - del2;
    del4 = te_diff2 - del3;
    
    if (fabs(del4) < 12.5e-9) del4 = 0;
  
  }
  te = te1 + (kzero-1)*esp;                // Return effective TE
  putvalue("te",te);

  /* How many echoes in the echo loop, including navigators? */
  etlnav = (etl-1)+(navigator[0]=='y')*2.0;
  
  /* Minimum TR **************************************/
  seqtime  = 4e-6 + 2*nseg*ns*4e-6;  /* count all the 4us delays */
  seqtime += ns*(ss_grad.duration/2 + te1 + (etlnav)*esp + ro_grad.timeFromEcho + pe_grad.duration + te3_delay);

  /* Increase TR if any options are selected****************/
  if (sat[0] == 'y')  seqtime += ns*satTime;
  if (fsat[0] == 'y') seqtime += ns*fsatTime;
  if (mt[0] == 'y')   seqtime += ns*mtTime;

  trmin = seqtime + ns*4e-6;  /* Add 4us to ensure that tr_delay is always >= 4us */
  if (mintr[0] == 'y'){
    tr = trmin;
    putvalue("tr",tr+1e-6);
  }
  if (tr < trmin) {
    abort_message("TR too short.  Minimum TR = %.2fms\n",trmin*1000);
  }
  tr_delay = (tr - seqtime)/ns;


  /* Set number of segments for profile or full image **********/
  nseg      = prep_profile(profile[0],nv/etl,&pe_grad,&per_grad);
  pe2_steps = prep_profile(profile[1],nv2,&pe2_grad,&pe2r_grad);

  /* Calculate total scan time */
  g_setExpTime(tr*(nt*nseg*pe2_steps*arraydim + ssc));




  /***************************************************/
  /* CALCULATE B VALUES ******************************/
  if (diff[0] == 'y') {
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
    dcrush = crush_grad.duration;       //"delta" for crusher
    Dcrush = dcrush + ss_grad.duration; //"DELTA" for crusher

    /* Readout */
    Dro     = ror_grad.duration;
    bro[i]  = bval(gdiff*roarr[i],tdelta,tDELTA);
    bro[i] += bval(ro_grad.amp,ro_grad.timeToEcho,Dro);
    bro[i] += bval(crush_grad.amp,dcrush,Dcrush);
    bro[i] += bval_nested(gdiff*roarr[i],tdelta,tDELTA,
                crush_grad.amp,dcrush,Dcrush);

    /* Slice */
    dgss2   = Dgss2 = ss_grad.rfCenterFront;
    bsl[i]  = bval(gdiff*slarr[i],tdelta,tDELTA);
    bsl[i] += bval(crush_grad.amp,dcrush,Dcrush);
    bsl[i] += bval(ss2_grad.ssamp,dgss2,Dgss2);
    bsl[i] += bval_nested(gdiff*slarr[i],tdelta,tDELTA,
                crush_grad.amp,dcrush,Dcrush);
    bsl[i] += bval_nested(gdiff*slarr[i],tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
    bsl[i] += bval_nested(ss2_grad.ssamp,dgss2,Dgss2,
                crush_grad.amp,dcrush,Dcrush);

    /* Phase */
    bpe[i]  = bval(gdiff*pearr[i],tdelta,tDELTA);
    bpe[i] += bval(crush_grad.amp,dcrush,Dcrush);
    bpe[i] += bval_nested(gdiff*pearr[i],tdelta,tDELTA,
                crush_grad.amp,dcrush,Dcrush);

    /* Readout/Slice Cross-terms */
    brs[i]  = bval2(gdiff*roarr[i],gdiff*slarr[i],tdelta,tDELTA);
    brs[i] += bval2(crush_grad.amp,
                    crush_grad.amp,dcrush,Dcrush);
    brs[i] += bval_cross(gdiff*roarr[i],tdelta,tDELTA,
                crush_grad.amp,dcrush,Dcrush);
    brs[i] += bval_cross(gdiff*slarr[i],tdelta,tDELTA,
                crush_grad.amp,dcrush,Dcrush);
    brs[i] += bval_cross(gdiff*roarr[i],tdelta,tDELTA,
                ss2_grad.ssamp,dgss2,Dgss2);
    brs[i] += bval_cross(crush_grad.amp,dcrush,Dcrush,
                ss2_grad.ssamp,dgss2,Dgss2);

    /* Readout/Phase Cross-terms */
    brp[i]  = bval2(gdiff*roarr[i],gdiff*pearr[i],tdelta,tDELTA);
    brp[i] += bval2(crush_grad.amp,
                    crush_grad.amp,dcrush,Dcrush);
    brp[i] += bval_cross(gdiff*roarr[i],tdelta,tDELTA,
                crush_grad.amp,dcrush,Dcrush);
    brp[i] += bval_cross(gdiff*pearr[i],tdelta,tDELTA,
                crush_grad.amp,dcrush,Dcrush);

    /* Slice/Phase Cross-terms */
    bsp[i]  = bval2(gdiff*pearr[i],gdiff*slarr[i],tdelta,tDELTA);
    bsp[i] += bval2(crush_grad.amp,
                    crush_grad.amp,dcrush,Dcrush);
    bsp[i] += bval_cross(gdiff*pearr[i],tdelta,tDELTA,
                crush_grad.amp,dcrush,Dcrush);
    bsp[i] += bval_cross(gdiff*slarr[i],tdelta,tDELTA,
                crush_grad.amp,dcrush,Dcrush);
    bsp[i] += bval_cross(gdiff*pearr[i],tdelta,tDELTA,
                ss2_grad.ssamp,dgss2,Dgss2);
    bsp[i] += bval_cross(crush_grad.amp,dcrush,Dcrush,
                ss2_grad.ssamp,dgss2,Dgss2);


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





  /* Shift DDR for pro *******************************/
  roff = -poffset(pro,ro_grad.roamp);


  /* PULSE SEQUENCE *************************************/
  if (ix == 1) grad_advance(tep);
  initval(fabs(ssc),vssc);      // Compressed steady-state counter
  setacqvar(vacquire);          // Control acquisition through vacquire
  assign(one,vacquire);         // Turn on acquire when vacquire is zero

  /* Phase cycle: Alternate 180 phase to cancel residual FID */
  mod2(ct,vphase180);           // 0101
  dbl(vphase180,vphase180);     // 0202
  add(vphase180,one,vphase180); // 1313 Phase difference from 90
  add(vphase180,oph,vphase180);

  obsoffset(resto);
  delay(4e-6);
    
  initval(nseg,vseg);
  initval(pe2_steps/2.0,vpe2_offset);
  
  initval(etl,vetl);
  initval(etl-1,vetl1);

  peloop2(seqcon[3],pe2_steps,vpe2_steps,vpe2_ctr);
  /* Use standard encoding order for 2nd PE dimension */
  sub(vpe2_ctr,vpe2_offset,vpe2_mult);

    loop(vseg,vseg_ctr);

      /* Compressed steady-states: 1st array & transient, all arrays if ssc is negative */
      if ((ix > 1) && (ssc > 0))
	assign(zero,vssc);
      sub(vseg_ctr,vssc,vseg_ctr);   // vpe_ctr counts up from -ssc
      assign(zero,vssc);
      ifzero(vseg_ctr);
	assign(zero,vacquire);       // Start acquiring when vseg_ctr reaches zero
      endif(vseg_ctr);

      msloop(seqcon[1],ns,vms_slices,vms_ctr);
	if (ticks) {
          xgate(ticks);
          grad_advance(tep);
	}
	sp1on(); delay(4e-6); sp1off();    // Scope trigger

	/* Prepulse options ***********************************/
	if (sat[0]  == 'y') satbands();
	if (fsat[0] == 'y') fatsat();
	if (mt[0]   == 'y') mtc();

	/* 90 degree pulse ************************************/         
	rotate();
	obspower(p1_rf.powerCoarse);
	obspwrf(p1_rf.powerFine);
	delay(4e-6);
	obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);   
	delay(ss_grad.rfDelayFront);
	shapedpulselist(shapelist90,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
	delay(ss_grad.rfDelayBack);

	/* Read dephase and Slice refocus *********************/
	obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0.0,0.0,-ssr_grad.amp,WAIT);

	/* First half-TE delay ********************************/
	obspower(p2_rf.powerCoarse);
	obspwrf(p2_rf.powerFine);
	delay(del1);

	/* DIFFUSION GRADIENT */
	if (diff[0] == 'y')
	  obl_shapedgradient(diff_grad.name,diff_grad.duration,diff_grad.amp*dro,diff_grad.amp*dpe,diff_grad.amp*dsl,WAIT);

	delay(del2);


	/*****************************************************/
	    /* FIRST ECHO OUTSIDE LOOP ***************************/
	/*****************************************************/
	ifzero(vacquire);  // real acquisition, get PE multiplier from table
          mult(vseg_ctr,vetl,vpe_ctr);
          getelem(t1,vpe_ctr,vpe_mult);
	elsenz(vacquire);  // steady state scan 
          assign(zero,vpe_mult);
	endif(vacquire);

	/* Variable crusher */
	assign(zero,vcr_ctr);
	getelem(t5,vcr_ctr,vcr1); 
	getelem(t6,vcr_ctr,vcr2);	     
	getelem(t7,vcr_ctr,vcr3);

  if(vcrush) 
	phase_encode3_oblshapedgradient(crush_grad.name,crush_grad.name,crush_grad.name,
	  crush_grad.duration,
	  (double)0,(double)0,(double)0,                                     // base levels
	  crush_grad.amp*cscale,crush_grad.amp*cscale,crush_grad.amp*cscale, // step size
	  vcr1,vcr2,vcr3,                                                    // multipliers
	  (double)1.0,(double)1.0,(double)1.0,                               // upper limit on multipliers
	  1,WAIT,0);

  else 
  obl_shapedgradient(crush_grad.name,crush_grad.duration,
    crush_grad.amp,crush_grad.amp,crush_grad.amp,WAIT);

	/* 180 degree pulse *******************************/
	obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);   
	delay(ss2_grad.rfDelayFront); 
	shapedpulselist(shapelist180,ss2_grad.rfDuration,vphase180,rof1,rof2,seqcon[1],vms_ctr);
	delay(ss2_grad.rfDelayBack);   

  if (vcrush)
	phase_encode3_oblshapedgradient(crush_grad.name,crush_grad.name,crush_grad.name,
	  crush_grad.duration,
	  (double)0,(double)0,(double)0,                                     // base levels
	  crush_grad.amp*cscale,crush_grad.amp*cscale,crush_grad.amp*cscale, // step size
	  vcr1,vcr2,vcr3,                                                    // multipliers
	  (double)1.0,(double)1.0,(double)1.0,                               // upper limit on multipliers
	  1,WAIT,0);
  else
  obl_shapedgradient(crush_grad.name,crush_grad.duration,
    crush_grad.amp,crush_grad.amp,crush_grad.amp,WAIT);

	delay(del3);

	/* DIFFUSION GRADIENT */
	if (diff[0] == 'y')
	  obl_shapedgradient(diff_grad.name,diff_grad.duration,diff_grad.amp*dro,diff_grad.amp*dpe,diff_grad.amp*dsl,WAIT);

	delay(del4);

	/* Phase-encode gradient ******************************/
	pe2_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,0,
	  -pe_grad.increment,-pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

	/* Readout gradient ************************************/
	obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.roamp,0,0,NOWAIT);
	delay(ro_grad.atDelayFront);

	/* Acquire data ****************************************/
	startacq(10e-6);
	acquire(np,1.0/sw);
	endacq();

	delay(ro_grad.atDelayBack);

	/* Rewinding phase-encode gradient ********************/
	pe2_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,
	  pe_grad.increment,pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

	/* Second half-TE delay *******************************/
	delay(te3_delay);


	/*****************************************************/
	    /* LOOP THROUGH THE REST OF ETL **********************/
	/*****************************************************/
	peloop(seqcon[2],etlnav,vetl_loop,vetl_ctr);
	  ifzero(vacquire);  // real acquisition, get PE multiplier from table
            mult(vseg_ctr,vetl,vpe_ctr);
            add(vpe_ctr,vetl_ctr,vpe_ctr);
	    add(vpe_ctr,one,vpe_ctr);
            getelem(t1,vpe_ctr,vpe_mult);
    	  elsenz(vacquire);  // steady state scan 
	    assign(zero,vpe_mult);
	  endif(vacquire);

	  /* But don't phase encode navigator echoes */
          ifrtGE(vetl_ctr,vetl1,vnav);
	    assign(zero,vpe_mult);
	  endif(vnav);


    	  /* Variable crusher */
	  incr(vcr_ctr);  /* Get next crusher level */
	  /* Except if we're doing navigators, start over */
	  sub(vetl1,vetl_ctr,vcr_reset);
	  ifzero(vcr_reset);
	    assign(zero,vcr_ctr);
	  endif(vcr_reset);

    	  getelem(t5,vcr_ctr,vcr1); 
    	  getelem(t6,vcr_ctr,vcr2);	     
    	  getelem(t7,vcr_ctr,vcr3);

	  phase_encode3_oblshapedgradient(crush_grad.name,crush_grad.name,crush_grad.name,
	    crush_grad.duration,
	    (double)0,(double)0,(double)0,                                     // base levels
	    crush_grad.amp,crush_grad.amp,crush_grad.amp,                      // step size
	    vcr1,vcr2,vcr3,                                                    // multipliers
	    (double)1.0,(double)1.0,(double)1.0,                               // upper limit on multipliers
	    1,WAIT,0);

          /* 180 degree pulse *******************************/
          obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0,0,ss2_grad.amp,NOWAIT);   
    	  delay(ss2_grad.rfDelayFront); 
          shapedpulselist(shapelist180,ss2_grad.rfDuration,vphase180,rof1,rof2,seqcon[1],vms_ctr);
          delay(ss2_grad.rfDelayBack);   

          /* Variable crusher */
	  phase_encode3_oblshapedgradient(crush_grad.name,crush_grad.name,crush_grad.name,
	    crush_grad.duration,
	    (double)0,(double)0,(double)0,                                     // base levels
	    crush_grad.amp,crush_grad.amp,crush_grad.amp,                      // step size
	    vcr1,vcr2,vcr3,                                                    // multipliers
	    (double)1.0,(double)1.0,(double)1.0,                               // upper limit on multipliers
	    1,WAIT,0);

          /* Phase-encode gradient ******************************/
	  pe2_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,
	    -pe_grad.increment,-pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

          /* Second half-TE period ******************************/
	  delay(te2_delay);

          /* Readout gradient ************************************/
          obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.roamp,0,0,NOWAIT);
          delay(ro_grad.atDelayFront);
          startacq(10e-6);
          acquire(np,1.0/sw);
	  endacq();
          delay(ro_grad.atDelayBack);

          /* Rewinding phase-encode gradient ********************/
	  pe2_shapedgradient(pe_grad.name,pe_grad.duration,0,0,0,
	    pe_grad.increment,pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

          /* Second half-TE delay *******************************/
          delay(te3_delay);
	endpeloop(seqcon[2],vetl_ctr);

	/* Relaxation delay ***********************************/
	if (!trtype)
          delay(tr_delay);
      endmsloop(seqcon[1],vms_ctr);
      if (trtype)
	delay(ns*tr_delay);
    endloop(vseg_ctr);
  endpeloop(seqcon[3],vpe2_ctr);

  /* Inter-image delay **********************************/
  sub(ntrt,ct,vtrimage);
  decr(vtrimage);
  ifzero(vtrimage);
    delay(trimage);
  endif(vtrimage);
}


/*******************************************************************************************
		Modification History

		
********************************************************************************************/

