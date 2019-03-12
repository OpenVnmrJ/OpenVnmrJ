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
/**********************************************************************
 *
 * NAME:
 *    tagcine.c
 *
 * DESCRIPTION:
 *    Cardiac cine sequence with tagging in one or two dimensions using the 
 *    SGL and sglwrapper functions with:
 *       - inversion recovery pulse.(T1 preparation)
 *       - T2 preparation - Not Tested
 *       - fat saturation
 *       - spatial satbands
 *       - transmitter fine power control
 *
 * MODIFICATION HISTORY:
 *       - non-slice selective 180 added for black blood imaging
 *
 *********************************************************************/
#include <standard.h>
#include "sgl.c"


REFOCUS_GRADIENT_T       ss2r_grad;                      /*90 deg refocus gradient*/
SLICE_SELECT_GRADIENT_T  sst2180_grad;                   /*180 deg ss gradient with buttterflys*/
GENERIC_GRADIENT_T       t2crush_grad;                   /*End of T2prep crusher gradient*/
GENERIC_GRADIENT_T       tag_grad;                       /* define the tagging gradient */
GENERIC_GRADIENT_T       tagcrush_grad ;                 /* define the tagging crusher  */                     

void pulsesequence(){
  /* INTERNAL VARIABLE DECLARATIONS *********************/
  double  freqEx[MAXNSLICE], freqIR[MAXNSLICE], freqT2[MAXNSLICE];  
  int     shapeEx, shapeIR=0, shapeT2=0;
  int     i;
  
  /* Echo (cine) loop tr, tr delay, refocusing time, phase rewind time*/
  double  cinetr,trdelay,tref,trewind,preptime=0.0; 
  
  /*time before echo,time after echo,delay before and after the echo(non-min te)*/ 
  double  tau1,tau2,tedelay;      	    
  double  pe_steps;
   
  /* IR Variables */
  double postirdelay1,postirdelay2; 
  char    dir[MAXSTR];  
    
  /*T2 prep variables */
  char    t2prep[MAXSTR],mint2te[MAXSTR];
  double  t2te,t2tedelay,t2preptime,t2temin;/*t2prep te, delay before,after the 180,
                                           t2prep time,minimum t2prep te*/
  double  tcrusht2,gcrusht2,gcrusht2180,tcrusht2180; /*t2 prep crushers*/    
   /* CINE parameters */
  double  rrdelay;                 
  double  qrsdelay, ipdelay, idelay;
  int     vflip, user_idelay;
  char    fliptable[MAXSTR];
  int     flip[100];

  double  maxgradtime,spoilMoment,spoilfact,perTime,tetime;
  int     sepSliceRephase,table;
  char    spoilflag[MAXSTR];
  char    mincinetr[MAXSTR];
  char    rfspoilreset[MAXSTR];

  /* Real-time variables used in this sequence **************/
  int  vpe_steps    = v1;      // Number of PE steps
  int  vpe_ctr      = v2;      // PE loop counter
  int  vms_slices   = v3;      // Number of slices
  int  vms_ctr      = v4;      // Slice loop counter
  int  vpe_offset   = v5;      // PE/2 for non-table offset
  int  vpe_mult     = v6;      // PE multiplier, ranges from -PE/2 to PE/2
  int  vper_mult    = v7;      // PE rewinder multiplier; turn off rewinder when 0
  int  vssc         = v8;      // Compressed steady-states
  int  vacquire     = v9;      // Argument for setacqvar, to skip steady state acquires
  int  vrfspoil_ctr = v10;     // RF spoil counter
  int  vrfspoil     = v11;     // RF spoil multiplier
  int  vtrimage     = v12;     // Counts down from nt, trimage delay when 0
  int  vcine_frames = v13;     // Number of CINE frames
  int  vcine_ctr    = v14;     // CINE loop counter
  int  vir_ctr      = v15;     // Slice loop counter in IR loop
  int  vt2_ctr      = v16;     // Slice loop counter in T2 prep loop  
  int  vvflip       = v17;     // Variable flip power level
  int  vph180       = v18;     // Phase of T2 prep 180
       
  /* Import and initialize variables */ 
  init_mri();
  getstr("spoilflag",spoilflag);
  spoilfact=getval("spoilfact");
  getstr("mincinetr",mincinetr);
  getstr("rfspoilreset",rfspoilreset);

  postirdelay1=0.0;
  postirdelay2=0.0;                                 
  cinetr = getval("cinetr");   	                            
  getstr("dir",dir);
  
  /* T2 prep variables  */
  getstr("t2prep",t2prep);            /* t2 prep flag */
  getstr("mint2te",mint2te);          /* minimum te (t2 prep) flag*/
  t2te=getval("t2te");                /* t2prep echo time */
  gcrusht2180=getval("gcrusht2180");  /* butterfly crusher strength around 180*/     	
  tcrusht2180=getval("tcrusht2180");  /* butterfly crusher duration around 180*/
  gcrusht2=getval("gcrusht2");        /* crusher strength at end of t2prep    */
  tcrusht2=getval("tcrusht2");        /* crusher duration at end of t2prep    */
  t2tedelay=0.0; 
  
  /* CINE parameters */
  if (strcmp(seqcon,"cscnn"))
    abort_message("%s: seqcon must be 'cscnn'",seqfil);
    
  rrdelay  = getval("rrdelay");  /* R-R separation, determines max # CINE images */
  qrsdelay = getval("qrsdelay"); /* delay after trigger before starting CINE loop */

  user_idelay = (int) getval("user_idelay");
  idelay   = getval("idelay");   /* delay between CINE images */  
  getstr("fliptable",fliptable);
  vflip = 0;

  if (strcmp(fliptable,"n") && strcmp(fliptable,"N") && strcmp(fliptable,"")) {
    loadtable(fliptable);
    vflip = 1;
  }

  /*  Check for external PE table ***********************/
  table = 0;
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    table = 1;
  }

  /* Set Rcvr/Xmtr phase increments for RF Spoiling *****/
  /* Ref:  Zur, Y., Magn. Res. Med., 21, 251, (1991) ****/
  if (rfspoil[0] == 'y') {
    rcvrstepsize(rfphase);
    obsstepsize(rfphase);
  }

  /* Initialize basic segment gradient and rf structures   ***************/
  shape_rf(&p1_rf,"p1",p1pat,p1,getval("flip1"),rof1,rof2 ); /* initialize excitation pulse */   
  init_slice(&ss_grad,"ss",thk);                       /* initialize slice select gradient */
  init_slice_refocus(&ssr_grad, "ssr");                /* initialize slice refocus gradient */     
  init_readout(&ro_grad,"ro",lro,np,sw);               /* initialize readout gradient */
  ro_grad.pad1=alfa; ro_grad.pad2=alfa;
  init_readout_refocus(&ror_grad,"ror");               /* initialize dephase gradient */
  init_phase(&pe_grad,"pe",lpe,nv);                    /* initialize phase encode gradient */
  init_dephase(&spoil_grad,"spoil");             // Optimized spoiler

  /*Basic segment sequence calculations *************/
  calc_rf(&p1_rf,"tpwr1","tpwr1f");   
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");      
  calc_slice_refocus(&ssr_grad, &ss_grad,WRITE,"gssr");     
  calc_readout(&ro_grad, WRITE, "gro","sw","at");              
  calc_readout_refocus(&ror_grad, &ro_grad, NOWRITE, "gror");     
  calc_phase(&pe_grad, NOWRITE, "gpe","tpe"); 
  spoilMoment = spoilfact*ro_grad.acqTime*ro_grad.roamp;   // Optimal spoiling is at*gro for 2pi per pixel
  spoilMoment -= ro_grad.m0def;                  // Subtract partial spoiling from back half of readout
  calc_dephase(&spoil_grad,WRITE,spoilMoment,"","");

  /* Is TE long enough for separate slice refocus? *******/
  maxgradtime = MAX(ror_grad.duration,pe_grad.duration);
  if (spoilflag[0] == 'y' && perewind[0] == 'y')
    maxgradtime = MAX(maxgradtime,spoil_grad.duration);
  tetime = ss_grad.rfCenterBack + ssr_grad.duration + maxgradtime + ro_grad.timeToEcho + GRADIENT_RES;

  /* Equalize refocus and PE gradient durations *********/
  if ((te >= tetime) && (minte[0] != 'y')) {
    sepSliceRephase = 1;                         // Set flag for separate slice rephase
    if (spoilflag[0] == 'y' && perewind[0] == 'y')
      calc_sim_gradient(&ror_grad,&pe_grad,&spoil_grad,tpemin,WRITE);
    else 
      calc_sim_gradient(&ror_grad,&pe_grad,&null_grad,tpemin,WRITE);
  } else {
    sepSliceRephase = 0;
    calc_sim_gradient(&ror_grad,&pe_grad,&ssr_grad,tpemin,WRITE);
    if (spoilflag[0] == 'y' && perewind[0] == 'y') {
      calc_sim_gradient(&ror_grad,&spoil_grad,&null_grad,tpemin,WRITE);
      calc_sim_gradient(&ror_grad,&pe_grad,&ssr_grad,tpemin,WRITE);
    }
  }
  putvalue("gspoil",spoil_grad.amp);
  putvalue("tspoil",spoil_grad.duration);

  perTime = 0.0;
  if ((perewind[0] == 'y') || (spoilflag[0] == 'y')) perTime = spoil_grad.duration;
  if (spoilflag[0] == 'n') spoil_grad.amp = 0.0;

  /*Calculations for prepulse structures*/
  /* hard 180 pulse for double IR pulse (black blood) */ 
  if(dir[0] == 'y') {
    shape_rf(&p3_rf,"p3",p3pat,p3,180,rof1,rof2);
    calc_rf(&p3_rf,"tpwr3","tpwr3f");
  }

  /* IR Prep     */
  if (ir[0] == 'y'){	  	  
    shape_rf(&ir_rf,"pi",pipat,pi,getval("flipir"),rof1,rof2); /*180 pulse(T2,IR preps)*/
    init_slice(&ssi_grad,"ssi",thk*thkirfact);                 /*ir (180) ss gradient; thk*thkirfact */
    init_generic(&ircrush_grad,"ircrush",gcrushir,tcrushir);   /*ir crusher gradient */
    calc_rf(&ir_rf,"tpwri","tpwrif");                          /* Calculate IR pulse power */	  
    calc_slice(&ssi_grad,&ir_rf,WRITE,"gssi");                 /* calculate IR slice select gradient*/                  
    calc_generic(&ircrush_grad,WRITE,"gcrushir","tcrushir");   /* Calculate crusher  */    
  }                                                          

  /*  T2 Prep  */
  if (t2prep[0] == 'y'){
    abort_message("Sorry, the t2prep option has not yet been completed.");

    shape_rf(&p2_rf,"p2",p2pat,p2,getval("flip2"),rof1,rof2 );     /*90 deg pulse*/
    init_slice(&ss2_grad,"ss2",thk);                         /*90 degree ss gradient*/
    init_slice_refocus(&ss2r_grad, "ss2r");
    init_slice_butterfly(&sst2180_grad,"sst2180",thk,gcrusht2180,tcrusht2180);
    init_generic(&t2crush_grad,"t2crush",gcrusht2,tcrusht2); /*T2prep crusher gradient*/

    calc_rf(&p2_rf,"tpwr2","tpwr2f");   
    calc_rf(&ir_rf,"tpwri","tpwrif");   
    calc_slice(&ss2_grad,&p2_rf,WRITE,"gss2");          /*calculate t2prep 90 ss gradient*/	   
    calc_slice_refocus(&ss2r_grad, &ss2_grad,WRITE,""); /*calculate t2prep ss refocus*/                 	                                                                                
    calc_slice(&sst2180_grad,&ir_rf,WRITE,"");               /*T2prep 180 ss w/butterflies*/
    calc_generic(&t2crush_grad,WRITE,"gcrusht2","tcrusht2");	    
  } 								
   
  /* Create optional prepulse events ************************/
  if (sat[0]  == 'y') create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (tag[0]  == 'y') {  
    initGeneric(&tag_grad);
    initGeneric(&tagcrush_grad);
    create_tag();
  }
  putvalue("tagtime",tagtime);  

  /*End of structure calculations ********/
  
  if (ticks > 0) {
    if (qrsdelay == 0) qrsdelay = tagtime;
    if (qrsdelay < tagtime)
      abort_message("%s: qrsdelay must be > tagtime (%.2fms)",seqfil, tagtime*1000);
    qrsdelay -= tagtime;    
  }

  /************************************************************/
  /* Calculate RF powers to ramp up flip angle in CINE loop   */
  /* Using Eq. 8 in Williams et al, JMRI 2001, 14(?), 374-382 */
  /************************************************************/
  for (i=0; i<ne-1; i++) {
    flip[i] = (int)(atan(1/sqrt(ne-i+1))/(PI/2)*32767);
  }
  flip[(int)ne-1] = 32767;
/*  settable(t2,(int)ne,flip);
  vflip = 1;  */ 

  /* TIMING CALCULATIONS */
       
  /* tau1 is the sum of the events between rf and the center of the echo */
  tau1 = ss_grad.rfCenterBack + pe_grad.duration + ro_grad.timeToEcho;
  tau1 += (sepSliceRephase) ? ssr_grad.duration : 0.0;   // Add slice refocusing if separate event
  /* Note: ror_grad.duration = ssr_grad.duration = pe_grad.duration */

  /* tau2 is the sum of the events after the echo */
  tau2 = ro_grad.timeFromEcho + perTime;
  /* True whether perewind[0]=='y' or not, as spoil_grad.duration = pe_grad.duration*/
      
  temin = tau1 + GRADIENT_RES;  /* ensure that te_delay is at least GRADIENT_RES */
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (FP_LT(te,temin)) {
    abort_message("TE too short.  Minimum TE= %.3fms\n",temin*1000);   
  }
  te=granularity(te,GRADIENT_RES);
  putvalue("te",te);
  tedelay = te-tau1;
  
  cinetr = te+tau2+ss_grad.rfCenterFront+GRADIENT_RES;
  putvalue("cinetr",cinetr);
  preptime += satTime + fsatTime;              
  if (tag[0]  == 'y') preptime += tagtime; 
  if (ticks > 0) preptime += qrsdelay;
  
  /* inter-image delay */
  if (ne > 1 && ticks > 0) {
    if (user_idelay) { /* User specified inter-image delay */
      if (idelay < cinetr)
        abort_message("%s: Inter-phase delay too small, minimum is %.2fms",seqfil,cinetr*1000+0.005);
      if ((idelay*ne+tagtime+qrsdelay) > rrdelay)
        abort_message("%s: Too many time frames for R-R interval, max frames is %d, or max delay is %.2fms", 
	     seqfil,(int) ((rrdelay-tagtime)/idelay), (rrdelay-tagtime)/ne*1000-0.005);
    }
    else {  /* Space out images equally */
      idelay = (rrdelay-tagtime-qrsdelay)/ne;
      if (idelay < cinetr) {
        abort_message("%s: Too many time frames for R-R interval, max is %d", seqfil,(int) ((rrdelay-preptime)/cinetr));
      }
      putvalue("idelay",idelay);
    }
    /* Subtract cinetime from idelay to use ipdelay in event code */
    ipdelay = idelay-cinetr;
    putvalue("cinetr",idelay);
  }
  else
    ipdelay = 0;

  /*Start calculation of min tr */
  trmin=(((cinetr + ipdelay)*ne)+preptime)*ns; 
  /*Prepulse sequence times*/	
  if (ir[0] == 'y') {
    postirdelay1=cinetr*ne-ss_grad.duration/2.0-ssi_grad.duration/2.0-ircrush_grad.duration+satTime+fsatTime;
    irTime = (ssi_grad.duration + ircrush_grad.duration + postirdelay1) * ns ;
    postirdelay2 = ti - irTime- ss_grad.duration/2.0;	   
    timin=irTime+ss_grad.duration/2.0;
    if (minti[0] == 'y') {
      ti = timin;
      putvalue("ti",ti);
    }
    if (FP_LT(ti,timin))
      abort_message("ERROR %s: IR error: ti too short, minimum is %.3fms\n",seqfil,timin*1000);
    trmin += irTime + postirdelay2;
  }

  if (dir[0] == 'y') 
    trmin += p3 + rof1 + rof2;
	
  if (t2prep[0] == 'y'){
    /* Calculate minimum te */
    t2temin= limitResolution(2.0*(ss2_grad.duration/2.0 + ss2r_grad.duration +  sst2180_grad.duration/2.0));
    t2te = limitResolution(t2te);	   
    /* Minimum t2te? */
    if (mint2te[0]=='y'){
      t2te=t2temin;
      putvalue("t2te",t2te);
    }
    if (t2te < t2temin )
      abort_message("%s: t2te too small.  Minimum t2te = %f\n",seqfil,t2temin);
    t2tedelay= (t2te-t2temin)/2.0;
    putvalue("t2tedelay",t2tedelay);
    t2preptime = t2te + 2.0*ss2_grad.duration/2.0 + t2crush_grad.duration;
    trmin += t2preptime*ns;
  }
     
  /*Minimum tr? */
  if (mintr[0] =='y'){    
    tr = trmin;
    putvalue("tr",tr);
  }
  if (tr < trmin) 
    abort_message("%s: Requested tr is too short.  Minimum tr is currently = %f ms\n",seqfil,trmin*1e3);	
  putvalue("tr",tr);
  if (ticks == 0) {
    ipdelay = (tr-trmin)/ne;
    putvalue("cinetr",(tr-preptime)/ne);
    trdelay = 0.0;
    qrsdelay=0.0;
  }
  else
    trdelay= (tr-trmin);	 

  /* fix up delays to avoid gradient timing error */
  if(postirdelay1 < 4e-6)  postirdelay1 = 4e-6;
  if(postirdelay2 < 4e-6)  postirdelay2 = 4e-6;
  if(tedelay < 4e-6)       tedelay      = 4e-6;
  if(trdelay < 4e-6)       trdelay      = 4e-6;
  if(ipdelay < 4e-6)       ipdelay      = 4e-6;
  if(t2tedelay < 4e-6)     t2tedelay    = 4e-6;
  
  /* Set up frequency offset pulse shape list ********/   	
  offsetlist(pss,ss_grad.ssamp,0,freqEx,ns,seqcon[1]);
  shapeEx = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freqEx,ns,0,seqcon[1]);
  if (t2prep[0] == 'y') {
    offsetlist(pss,ss2_grad.ssamp,0,freqT2,ns,seqcon[1]);
    shapeT2 = shapelist(p2_rf.pulseName,ss2_grad.rfDuration,freqT2,ns,0,seqcon[1]);
  } 
  if (ir[0] == 'y') {
    offsetlist(pss,ssi_grad.ssamp,0,freqIR,ns,seqcon[1]);
    shapeIR = shapelist(ir_rf.pulseName,ssi_grad.rfDuration,freqIR,ns,0,seqcon[1]);
  } 
    
  /* Shift DDR for pro *******************************/   	
  roff = -poffset(pro,ro_grad.roamp); 
   
  /* Set pe_steps for profile or full image **********/
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&null_grad);
  F_initval(pe_steps/2.0,vpe_offset);

  g_setExpTime(tr*(nt*pe_steps*arraydim + ssc));

  /* PULSE SEQUENCE *************************************/
  initval(ns,vms_slices);     // Also used in IR and T2 prep loops
  initval(fabs(ssc),vssc);    // Compressed steady-state counter
  assign(zero,vrfspoil_ctr);         // RF spoil phase counter
  assign(zero,vrfspoil);             // RF spoil multiplier

  setacqvar(vacquire);        // vacquire controls acquisition or dummy scan mode
  assign(zero,vacquire);
    
  rotate();
  triggerSelect(trigger);            // Select trigger input 1/2/3
  obsoffset(resto);
  delay(4e-6);        
  peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);
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

    /* Read external kspace table if set ******************/       
    if (table)
      getelem(t1,vpe_ctr,vpe_mult);
    else {
      ifzero(vacquire);
        sub(vpe_ctr,vpe_offset,vpe_mult);
      elsenz(vacquire);
        sub(zero,vpe_offset,vpe_mult);      // Hold PE mult at initial value for steady states
      endif(vacquire);
    }

    /* PE rewinder follows PE table; zero if turned off ***/       
    if (perewind[0] == 'y')
      assign(vpe_mult,vper_mult);
    else
      assign(zero,vper_mult);

    /*Slice-interleaved inversion recovery (IR) pulses */
    if (ir[0] == 'y') {	
      if (ticks) {
        xgate(ticks);
        grad_advance(gpropdelay);
        delay(4e-6);
      } else
        delay(4e-6);

      if(dir[0] == 'y') { /* hard 180 pulse for double IR pulse (black blood) */
        obspower(p3_rf.powerCoarse);	
        obspwrf(p3_rf.powerFine);
        shapedpulse(p3_rf.pulseName,p3,zero,rof1,rof2);
      }
        			
      loop(vms_slices,vir_ctr); 
        obspower(ir_rf.powerCoarse);
        obspwrf(ir_rf.powerFine);
        delay(4e-6);
        obl_shapedgradient(ssi_grad.name,ssi_grad.duration,0,0,
          ssi_grad.amp,NOWAIT);   
        delay(ssi_grad.rfDelayFront);
    	shapedpulselist(shapeIR,ssi_grad.rfDuration,oph,rof1,rof2,seqcon[1],vir_ctr);
 
    	delay(ssi_grad.rfDelayBack);                
        obl_shapedgradient(ircrush_grad.name, ircrush_grad.duration,
                           ircrush_grad.amp,ircrush_grad.amp,ircrush_grad.amp,WAIT);                
        delay(postirdelay1);
      endloop(vir_ctr); 
      delay(postirdelay2);  
    } /*End of Inversion pulse*/
        
    /* Begin multislice loop *****************************/       
    msloop(seqcon[1],ns,vms_slices,vms_ctr); 

      /* T2 Prep ************************  NOT TESTED */
      if (t2prep[0] == 'y') {       
        /***   90 deg pulse ****/
       loop(vms_slices,vt2_ctr);
        obspower(p2_rf.powerCoarse);
        obspwrf(p2_rf.powerFine);
        delay(4e-06);	               
        obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0.0,0.0,ss2_grad.amp,NOWAIT);    
        delay(ss2_grad.rfDelayFront);
	shapedpulselist(shapeT2,ss2_grad.rfDuration,oph,rof1,rof2,seqcon[1],vt2_ctr);
        delay(ss2_grad.rfDelayBack);	  
        obl_shapedgradient(ss2r_grad.name,ss2r_grad.duration,0.0,0.0,ss2r_grad.amp,WAIT);       

	delay(t2tedelay);

	/***** 180 degree pulse  is same as IR pulse */
        obspower(ir_rf.powerCoarse);
        obspwrf(ir_rf.powerFine);
        delay(4e-06);		   		
	obl_shapedgradient(sst2180_grad.name,sst2180_grad.duration,
                   0.0,0.0,sst2180_grad.amp,NOWAIT); 		   
	shapedpulselist(shapeIR,ssi_grad.rfDuration,vph180,rof1,rof2,seqcon[1],vt2_ctr);
	delay(sst2180_grad.rfDelayBack);
        obl_shapedgradient(ss2r_grad.name,ss2r_grad.duration,0.0,0.0,ss2r_grad.amp,WAIT);        
	delay(t2tedelay);
				 		
	/*****Second 90 degree pulse  ***/
	obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0.0,0.0,ss2_grad.amp,NOWAIT);
        delay(ss2_grad.rfDelayFront); 
        obspower(p2_rf.powerCoarse);
	obspwrf(p2_rf.powerFine);
	delay(4e-06);
	shapedpulselist(shapeIR,ss2_grad.rfDuration,oph,rof1,rof2,seqcon[1],vt2_ctr);
        delay(ss2_grad.rfDelayBack);  		       			 	
        
        obl_shapedgradient(t2crush_grad.name,t2crush_grad.duration,
                           t2crush_grad.amp,t2crush_grad.amp,t2crush_grad.amp,WAIT);                       
       endloop(vt2_ctr);
      }	 		 		    
      /*******End of T2 prep  *****/  	
    
      /* Prepulse options ***********************************/       
      if (sat[0]  == 'y') satbands();
      if (fsat[0] == 'y') fatsat();

      /* Trigger**********/
      if (ticks) {
        xgate(ticks);
        grad_advance(gpropdelay);
        delay(4e-6);
      } else
        delay(4e-6);

      /* TAGGING ***/
      if (tag[0] == 'y') tag_sinc();      
      delay(qrsdelay);

sp1on(); delay(4e-6); sp1off();       // Scope trigger

      /* Allow RF spoiling to be reset so that each frame starts from the same
         point in the spoiling cycle. This avoids jumps created when waiting
         for the next R wave and so replaces one artefact (banding) with 
         another (ghosting) */
      if (rfspoilreset[0] == 'y' ) {
        assign(zero,vrfspoil);
        assign(zero,vrfspoil_ctr);
      }

      initval(ne,vcine_frames);
      loop(vcine_frames,vcine_ctr);

        /* Set rcvr/xmtr phase for RF spoiling ****************/
        if (rfspoil[0] == 'y') {
          incr(vrfspoil_ctr);                   // vrfspoil_ctr = 1  2  3  4  5  6
          add(vrfspoil,vrfspoil_ctr,vrfspoil);  // vrfspoil =     1  3  6 10 15 21
          xmtrphase(vrfspoil);
          rcvrphase(vrfspoil);
        }

        if (vflip)
          getelem(t2,vcine_ctr,vvflip);  /* load RF amp from table to vvflip */
        else
          initval(32767.0,vvflip);
        /* RF pulse *******************************************/
        /* (re)set multislice offset frequencies */
        obspower(p1_rf.powerCoarse);
        obspwrf(p1_rf.powerFine);
        delay(4e-6);

        /* slice select gradient*/
        obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);   
        delay(ss_grad.rfDelayFront);
        shapedpulselist(shapeEx,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
        delay(ss_grad.rfDelayBack);	  

        /* Phase encode, refocus, and dephase gradient ********/
        if (sepSliceRephase) {                // separate slice refocus gradient
          obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);
          delay(tedelay);                    // delay between slab refocus and pe
          pe_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,0,-pe_grad.increment,vpe_mult,WAIT);
        } else {
          pe_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp,0,-ssr_grad.amp,-pe_grad.increment,vpe_mult,WAIT);
          delay(tedelay);                    // delay after refocus/pe
        }

        /* Acquire echo ***************************************/
        obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp,0,0,NOWAIT);
        delay(ro_grad.atDelayFront-alfa);
        startacq(alfa);
        acquire(np,1.0/sw);
        delay(ro_grad.atDelayBack);
        endacq();

        /* Rewind / spoiler gradient **************************/
        if ((perewind[0] == 'n') && (spoilflag[0] == 'y'))
          obl_shapedgradient(spoil_grad.name,spoil_grad.duration,spoil_grad.amp,0,0,WAIT);
        else if (perewind[0] == 'y')
          pe_shapedgradient(pe_grad.name,pe_grad.duration,spoil_grad.amp,0,0,pe_grad.increment,vper_mult,WAIT);
    
        if (ne > 1) delay(ipdelay);      
      endloop(vcine_ctr); /*  End CINE loop */
    endmsloop(seqcon[1],vms_ctr);
    delay(trdelay);
  endpeloop(seqcon[2],vpe_ctr);
}
/**************************************************************************
           MODIFICATION HISTORY

041214(mh) 	Started from gems sequence
		Add ne loop inside ms loop
                Fix list bug with seqcon[slice] = 's'
                Force seqcon to be 'cscnn'
                Check that all frames fit in TR (rrdelay > sseqtime*ne)
                
050105(mh)	Tagging
		Specify qrsdelay and inter-image delay 
		
050308-10(msb) Converted to use the SGL.
050322(msb) Added T2 prep, fatsat, satbands (_3), DELAY macro
May05(msb)    Varible name changes, updated params.h, remove sgl error messages

20060417(ss)  Modified for vnmrs 
	      rrdelay is not increased by 10%
	      
	      vnmrs steady state logic added
	      delay times checked for 4us limit
	      ir and t2 pulses modified for vnmrs
20060424(ss)  ir pulse thickness = thkirfact*thk
                              
                NOT YET Allow variable flip angle throughout CINE loop

**************************************************************************/
