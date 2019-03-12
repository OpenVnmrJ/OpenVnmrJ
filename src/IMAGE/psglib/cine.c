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
Cardiac phase cine sequence
************************************************************************/

#include <standard.h>
#include "sgl.c"

void pulsesequence() {
  /* Internal variable declarations *************************/
  double  freqEx[MAXNSLICE],freqIR[MAXNSLICE];  
  int     shapeEx,shapeIR=0;
  double  tedelay,trseg,trdelay,preptime=0.0; 
  double  pe_steps,maxgradtime,spoilMoment,spoilfact,perTime,tetime;
  char    localizer[MAXSTR],spoilflag[MAXSTR],rfspoilreset[MAXSTR];
  int     sepSliceRephase,table,nfmod;
  int     i,n;

  /* Phase encode variables */
  FILE   *fp;
  int     petab[4096];
  char    tabname[MAXSTR],tabfile[MAXSTR];
  int     kstep;
  int     nseg=1; 
  
  /* IR Variables */
  double  preirdelay=0.0,postirdelay=0.0,respirdelay=0.0,dirTime=0.0,dirtimin,timax,irdelay; 
  char    dir[MAXSTR];
    
  /* CINE parameters */
  char    mintrseg[MAXSTR],rgate[MAXSTR],gatezero[MAXSTR],trigmode[MAXSTR],freshresp[MAXSTR],setphases[MAXSTR];
  double  rrdelay,qrsdelay,ipdelay,triggerwindow,blankingdelay,rgateperiod,rgatewindow,rgatedelay;
  double  trperiod,trfill=0.0,maxne,tscale,fifopad=0.0;
  int     singlerr=FALSE,vflip;
  char    fliptable[MAXSTR];
  int     flip[100];

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
  int  vcine_frames = v12;     // Number of CINE frames
  int  vcine_ctr    = v13;     // CINE loop counter
  int  vvflip       = v14;     // Variable flip power level
  int  vactive      = v15;     // Initial steady state or real scan toggle
  int  vresp        = v16;     // Respiration evaluation from readMRIUserByte
  int  vpetab       = v17;     // PE table index
  int  vdiracq      = v18;     // Flag for dir in respgate cases   
  int  vresp_trig   = v19;     // Flag for resp triggering in IR sequences
  int  vresp_ctr    = v20;     // Counter for resp triggering in IR sequences
  int  vresp_wait   = v21;     // Max waiting loop for resp triggering in IR sequences

  /* Initialize variables */ 
  init_mri();
  getstr("spoilflag",spoilflag);
  spoilfact=getval("spoilfact");
  getstr("mintrseg",mintrseg);
  getstr("rfspoilreset",rfspoilreset);
  trseg = getval("trseg");   	                            
  getstr("localizer",localizer);
  kstep=(int)getval("kstep");  
  getstr("dir",dir);
  if (dir[0]=='y') ir[0]='y';
  rrdelay  = getval("rrdelay");
  qrsdelay = getval("qrsdelay");
  getstr("rgate",rgate);
  getstr("gatezero",gatezero);
  getstr("trigmode",trigmode);
  getstr("freshresp",freshresp);
  getstr("setphases",setphases);
  triggerwindow=getval("triggerwindow");
  blankingdelay=getval("blankingdelay");
  rgateperiod=getval("rgateperiod");  
  rgatedelay=getval("rgatedelay");
  rgatewindow=getval("rgatewindow");

  /* FIFO pad to avoid FIFO errors */
  if (rgate[0]=='y') fifopad=0.001; /* 1 ms pad for readMRIUserByte */
  else fifopad=GRADIENT_RES;

  /* Must have standard slice loop */
  if (strcmp(seqcon,"cscnn")) abort_message("%s: seqcon must be 'cscnn'",seqfil);

  /* Read flip table if it exists */
  getstr("fliptable",fliptable);
  vflip = 0;  
  if (strcmp(fliptable,"n") && strcmp(fliptable,"N") && strcmp(fliptable,"")) {
    loadtable(fliptable);
    vflip = 1;
  }

  /* Set petable name and full path ***********************/
  if (localizer[0]=='y') {
    /* Force etl to divide exactly into nv */
    while ((int)nv%(int)etl != 0) etl--;
    putCmd("etl = %d",(int)etl);
    /* Set table name */
    sprintf(tabname,"cine_loc_%d",(int)etl);
  } else {
    if (kstep>1)
      sprintf(tabname,"cine%d_%d",(int)nv,kstep); /* Set table name */
    else
      sprintf(tabname,"n");
  }

  putCmd("petable = '%s'",tabname);
  strcpy(tabfile,userdir);
  strcat(tabfile,"/tablib/");
  strcat(tabfile,tabname);
  putCmd("pelist = 0"); /* Re-initialize pelist */

  /* Generate phase encode tables *************************/
  if (localizer[0]=='y') kstep = (int)etl; /* For rapid segmentation */
  /* Otherwise k-space reordering spreads residual T1 related artefacts 
     when there is a fixed number of ECGs per respiration interval */
  if (kstep>1) {
    nseg=nv/kstep;
    for (i=0;i<nseg*kstep;i++)
      petab[i] = i/kstep + (i%kstep)*nseg - nv/2 + 1;
    for (i=nseg*kstep;i<nv;i++)
      petab[i]= i - nv/2 + 1;
    /* Write table to file */
    fp=fopen(tabfile,"w");
    fprintf(fp,"t1 =");
    for (i=0;i<nv;i++) {
      if (i%kstep == 0) fprintf(fp,"\n");
      fprintf(fp,"%3d\t",petab[i]);
    }
    fclose(fp);
    for (i=0;i<nv;i++) putCmd("pelist[%d] = %d",i+1,petab[i]);
  } else {
    /* Standard linear order */
    for (i=0;i<nv;i++) petab[i]= i - nv/2 + 1;
  }

  /* Set phase encode table *******************************/
  settable(t1,(int)nv,petab);

  /* Set Rcvr/Xmtr phase increments for RF Spoiling *****/
  /* Ref:  Zur, Y., Magn. Res. Med., 21, 251, (1991) ****/
  if (rfspoil[0] == 'y') {
    rcvrstepsize(rfphase);
    obsstepsize(rfphase);
  }

  /* RF structures ******************************************/
  shape_rf(&p1_rf,"p1",p1pat,p1,getval("flip1"),rof1,rof2 );  
  calc_rf(&p1_rf,"tpwr1","tpwr1f");

  /* Initialize gradient structures *************************/
  init_slice(&ss_grad,"ss",thk);
  init_slice_refocus(&ssr_grad, "ssr");
  init_readout(&ro_grad,"ro",lro,np,sw);
  ro_grad.pad1=alfa; ro_grad.pad2=alfa;
  init_readout_refocus(&ror_grad,"ror");
  init_phase(&pe_grad,"pe",lpe,nv);
  init_dephase(&spoil_grad,"spoil");

  /* Gradient calculations **********************************/
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");      
  calc_slice_refocus(&ssr_grad, &ss_grad,WRITE,"gssr");     
  calc_readout(&ro_grad, WRITE, "gro","sw","at");              
  calc_readout_refocus(&ror_grad, &ro_grad, NOWRITE, "gror");     
  calc_phase(&pe_grad, NOWRITE, "gpe","tpe"); 
  spoilMoment = spoilfact*ro_grad.acqTime*ro_grad.roamp;   // Optimal spoiling is at*gro for 2pi per pixel
  spoilMoment -= ro_grad.m0def;                            // Subtract partial spoiling from back half of readout
  calc_dephase(&spoil_grad,WRITE,spoilMoment,"","");

  /* Is TE long enough for separate slice refocus? *********/
  maxgradtime = MAX(ror_grad.duration,pe_grad.duration);
  if (spoilflag[0] == 'y' && perewind[0] == 'y')
    maxgradtime = MAX(maxgradtime,spoil_grad.duration);
  tetime = ss_grad.rfCenterBack + ssr_grad.duration + maxgradtime + ro_grad.timeToEcho + GRADIENT_RES;

  /* Equalize refocus and PE gradient durations ***********/
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
      calc_sim_gradient(&spoil_grad,&pe_grad,&ssr_grad,tpemin,WRITE);
    }
  }
  putvalue("gspoil",spoil_grad.amp);
  putvalue("tspoil",spoil_grad.duration);

  perTime = 0.0;
  if ((perewind[0] == 'y') || (spoilflag[0] == 'y')) perTime = spoil_grad.duration;
  if (spoilflag[0] == 'n') spoil_grad.amp = 0.0;

  /* Calculations for prepulse structures */
  /* hard 180 pulse for double IR pulse (black blood) */ 
  if(dir[0] == 'y') {
    shape_rf(&p3_rf,"p3",p3pat,p3,180,rof1,rof2);
    calc_rf(&p3_rf,"tpwr3","tpwr3f");
  }

  /* IR Prep */
  if (ir[0] == 'y'){	  	  
    shape_rf(&ir_rf,"pi",pipat,pi,getval("flipir"),rof1,rof2);
    init_slice(&ssi_grad,"ssi",thk*thkirfact);
    init_generic(&ircrush_grad,"ircrush",gcrushir,tcrushir);
    calc_rf(&ir_rf,"tpwri","tpwrif");
    calc_slice(&ssi_grad,&ir_rf,WRITE,"gssi");
    calc_generic(&ircrush_grad,WRITE,"gcrushir","tcrushir");
  }                                                          

  /* Create optional prepulse events */
  if (sat[0]  == 'y') create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (tag[0]  == 'y') {  
    initGeneric(&tag_grad);
    initGeneric(&tagcrush_grad);
    create_tag();
  }
  putvalue("tagtime",tagtime);  

  if (rgate[0]=='y' && ir[0]=='y' && freshresp[0]=='y') {
    if (rgatewindow>(rgateperiod-rgatedelay))
      abort_message("ERROR %s: Resp window must be less than %.3fms (Resp gate period - Resp gate delay)\n",seqfil,(rgateperiod-rgatedelay)*1000);
  }

  /************************************************************/
  /* Calculate RF powers to ramp up flip angle in CINE loop   */
  /* Using Eq. 8 in Williams et al, JMRI 2001, 14(?), 374-382 */
  /************************************************************/
  /* NB not yet implimented */
  for (i=0; i<ne-1; i++) {
    flip[i] = (int)(atan(1/sqrt(ne-i+1))/(PI/2)*32767);
  }
  flip[(int)ne-1] = 32767;
//  settable(t2,(int)ne,flip);
//  vflip = 1;

  /* Minimum TE */
  te=granularity(te,GRADIENT_RES);
  temin = ss_grad.rfCenterBack+pe_grad.duration+ro_grad.timeToEcho+GRADIENT_RES;
  temin += (sepSliceRephase) ? ssr_grad.duration : 0.0;   // Add slice refocusing if separate event
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (FP_LT(te,temin)) {
    abort_message("ERROR %s: TE too short.  Minimum TE= %.3fms\n",seqfil,temin*1000);   
  }
  tedelay = te-temin+GRADIENT_RES;

  /* Cine TR */
  preptime = satTime + fsatTime;              
  if (tag[0]  == 'y') preptime += tagtime; 
  if (rgate[0] == 'y') {
    if (trigmode[0] =='c') preptime += blankingdelay;
    if (trigmode[0] =='r') triggerwindow += blankingdelay;
  }
  if (ticks>0) {
    if (FP_LT(qrsdelay,preptime))
      abort_message("ERROR %s: Trigger delay must be at least %.3f ms)",seqfil,preptime*1000);
    qrsdelay -= preptime;
    preptime += qrsdelay;
  }

  trmin = ss_grad.rfCenterFront+te+ro_grad.timeFromEcho+perTime+2*GRADIENT_RES;
  if (mintr[0] == 'y' || localizer[0]=='y') {
    tr = trmin;
    putvalue("tr",tr);
  }
  /* Check the trigger delay is something sensible */
  if (ticks>0) {
    if (preptime>rrdelay-trmin*etl-triggerwindow-fifopad)
      abort_message("ERROR %s: Trigger delay can't be greater than %.3f ms\n",seqfil,(rrdelay-fifopad-triggerwindow-trmin*etl)*1000);
  }

  if (localizer[0]=='n' && ticks>0 && setphases[0]=='y') {
//    tr=granularity((rrdelay-preptime-fifopad-triggerwindow-trmin)/(ne-1),GRADIENT_RES); /* no delay after last frame */
    tr=granularity((rrdelay-preptime-fifopad-triggerwindow-3*GRADIENT_RES)/ne,GRADIENT_RES);
    putCmd("mintr = 'n'");
    putvalue("tr",tr);    
    if (FP_LT(tr,trmin)) {
//      maxne=1+floor((rrdelay-preptime-fifopad-triggerwindow-trmin)/trmin); /* no delay after last frame */
      maxne=floor((rrdelay-preptime-fifopad-triggerwindow-2*GRADIENT_RES)/trmin);
      abort_message("ERROR %s: CINE TR too short for %d phases, maximum is %d phases (minimum CINE TR is %.3fms)\n",seqfil,(int)ne,(int)maxne,trmin*1000);
    }
  }
  if (FP_LT(tr,trmin))
    abort_message("ERROR %s: CINE TR too short, minimum is %.3fms\n",seqfil,trmin*1000);
  ipdelay = tr-trmin+GRADIENT_RES;
  if (localizer[0]=='n' && ticks>0 && setphases[0]=='n') {
//    ne=floor((rrdelay-preptime-fifopad-triggerwindow+ipdelay)/tr); /* no delay after last frame */
    ne=floor((rrdelay-preptime-fifopad-triggerwindow-2*GRADIENT_RES)/tr);
    if (ne<1) ne=1;
    putvalue("ne",ne);
  } 

  /* FIFO underflow errors can occur if readMRIUserByte is used for 
     respiration gating.
     rlogin master1 during scanning shows which controllers give errors.
     For cine setloop sets nfmod=nv by default.
     This nfmod setting typically generates ddr errors.
     Setting nfmod between 2 and 32 appears to eliminate ddr errors.
     NB. nfmod=1 can give
     'Data transfer from console to host failed (data uplink rate too high)'
     errors due to a high number of network packets, particularly when
     multiple receivers are used. Short TR and activity that reduces
     resources available to handle network traffic increases the
     probablility of this error.
     The putvalue of ne (above) runs setloop (to update nf and nfmod) and 
     so we reset a better value here.
  */
  if (rgate[0]=='y' && nv>12) {
    nfmod=12;
    while (nfmod<nv && (int)(nv*ne)%nfmod !=0) nfmod++;
    putvalue("nfmod",nfmod);
  }

  /* Min tr */
  trmin=tr*ne*etl+preptime+fifopad+triggerwindow+2*GRADIENT_RES;
  trperiod=0.0;
  if (ticks>0) {
    trfill=rrdelay-trmin;
    if (trfill<0) trfill=0.0; /* avoid small negative delay due to granularity round up */
    trmin=rrdelay;
    trperiod=rrdelay;
  }

  /* Prepulse sequence times */	
  dirTime = 0.0;
  dirtimin=0.0;
  if (dir[0] == 'y') {
    dirTime = p3_rf.rfDuration+rof1+rof2+GRADIENT_RES;
    if (ticks<1) trmin+=dirTime; 
    dirtimin=(dirTime-GRADIENT_RES)/2.0+GRADIENT_RES+ssi_grad.duration/2.0;
  }

  if (ir[0] == 'y') {

    /* Can inversion and acquisition be in the same R-R interval? */
    singlerr=FALSE;
    if (ticks>0) {
      if (rgate[0] == 'y') irdelay=qrsdelay+blankingdelay;
      else irdelay=qrsdelay;
      if (dirTime+GRADIENT_RES+ssi_grad.duration+ircrush_grad.duration<irdelay) {
        timin=dirtimin+ssi_grad.duration/2.0+ircrush_grad.duration+preptime-irdelay+ss_grad.duration/2.0+3*GRADIENT_RES;
        timax=timin+qrsdelay-GRADIENT_RES-ssi_grad.duration-ircrush_grad.duration-dirTime;
        if (minti[0] == 'y') {
          ti = timin;
          putvalue("ti",ti);
        }
        if (FP_LT(ti,timin))     
          abort_message("ERROR %s: IR error: ti too short, minimum is %.3fms\n",seqfil,timin*1000);
        if (FP_LT(ti,timax)) {
          postirdelay=ti-timin+GRADIENT_RES;
          preirdelay=timax-ti;
          qrsdelay = 0.0;     
          singlerr=TRUE;
        }
      }
    }

    if (!singlerr) {
      timin=dirtimin+ssi_grad.duration/2.0+ircrush_grad.duration+preptime+ss_grad.duration/2.0+fifopad;
      if (ticks>0) timin+=triggerwindow;
      if (minti[0] == 'y') {
        ti = timin;
        putvalue("ti",ti);
      }
      if (FP_LT(ti,timin))
        abort_message("ERROR %s: IR error: ti too short, minimum is %.3fms\n",seqfil,timin*1000);
      postirdelay=ti-timin+fifopad;
      irTime = ssi_grad.duration+ircrush_grad.duration+postirdelay+GRADIENT_RES;
      trmin+=irTime+GRADIENT_RES;
      preirdelay=0.0;
      if (ticks>0) {
        n=2;
        preirdelay=rrdelay-triggerwindow-irTime-dirTime;
        while (preirdelay<0) {
          preirdelay+=rrdelay;
          n++;
        }
        if (rgate[0] == 'y') {
          if (preirdelay>blankingdelay) preirdelay-=blankingdelay; else preirdelay=0.0;
        }
        trmin=n*rrdelay;
      }
    }

    if (ticks<1 && rgate[0]=='y') trmin+=blankingdelay;

    if (rgate[0]=='y' && freshresp[0]=='y') {
      /* Figure the number of resp periods */
      n=0;
      while (trmin>(rgatewindow+n*rgateperiod)) n++;
      /* Figure an appropriate delay to center the trmin within respiration window(s) */
      respirdelay=(rgatewindow+n*rgateperiod-trmin)/2.0;
      n++;
      trfill=respirdelay+rgateperiod-rgatewindow;
      trmin=n*rgateperiod;
      trperiod=rgateperiod;
    }
  }

  /* Minimum tr */
  if (mintrseg[0] =='y'){    
    trseg = trmin;
    putvalue("trseg",trseg);
  }

  if (trperiod>0.0) {
    n=0;
    while (FP_GT(trseg,n*trperiod)) n++;
    trseg=n*trperiod;
    putvalue("trseg",trseg);
  }

  if (FP_LT(trseg,trmin))
    abort_message("ERROR %s: Requested TReff is too short.  Minimum TReff = %.3f ms\n",seqfil,trmin*1000);	

  /* Calculate tr delay *************************************/
  trdelay = granularity(trseg-trmin+fifopad,GRADIENT_RES);

  if (trperiod>0.0) {
    if (trdelay<trperiod) trdelay=fifopad;
    else trdelay -= 0.5*trperiod-trfill;
  }

  /* Set up frequency offset pulse shape list ********/   	
  offsetlist(pss,ss_grad.ssamp,0,freqEx,ns,seqcon[1]);
  shapeEx = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freqEx,ns,0,seqcon[1]);
  if (ir[0] == 'y') {
    offsetlist(pss,ssi_grad.ssamp,0,freqIR,ns,seqcon[1]);
    shapeIR = shapelist(ir_rf.pulseName,ssi_grad.rfDuration,freqIR,ns,0,seqcon[1]);
  } 
    
  /* Shift DDR for pro *******************************/   	
  roff = -poffset(pro,ro_grad.roamp); 
   
  /* Set pe_steps for profile or full image **********/
  pe_steps = prep_profile(profile[0],nv/etl,&pe_grad,&null_grad); 
  F_initval(pe_steps/2.0,vpe_offset);
  if (localizer[0]=='y') F_initval(etl,vcine_frames); 
  else F_initval(ne,vcine_frames);

  /* Adjust experiment time for VnmrJ *********************/
  /* Good:Bad respiration phase duration is approx 3:2 */
  if (rgate[0] == 'y') tscale=1.5; else tscale=1.0;
  g_setExpTime(tscale*trseg*(nt*pe_steps*arraydim + ssc));

  /* PULSE SEQUENCE *************************************/
  initval(fabs(ssc),vssc);      // Compressed steady-state counter
  assign(zero,vrfspoil_ctr);    // RF spoil phase counter
  assign(zero,vrfspoil);        // RF spoil multiplier
  assign(one,vacquire);         // Real-time acquire flag
  setacqvar(vacquire);          // vacquire controls acquisition or dummy scan mode
  assign(one,vactive);          // Start in steady state mode 

  if (gatezero[0] == 'o') { 
    assign(one,vresp);    /* initialize to dummy scan value */
    assign(zero,vdiracq); /* ensure dummy/acq conditions are correctly set with rgate and not dir */
  } else {
    assign(zero,vresp);
    assign(one, vdiracq);
  }
  
  rotate();
  triggerSelect(trigger);       // Select trigger input 1/2/3
  obsoffset(resto);
  delay(GRADIENT_RES);        
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
        assign(zero,vactive);
      endif(vpe_ctr);
    }

    /* PE rewinder follows PE table; zero if turned off ***/       
    if (perewind[0] == 'y')
      assign(vpe_mult,vper_mult);
    else
      assign(zero,vper_mult);

    /* Begin multislice loop *****************************/       
    msloop(seqcon[1],ns,vms_slices,vms_ctr); 

      /* Inversion recovery (IR) pulses */
      if (ir[0] == 'y') {	

        /* For fresh respiration phase loop readMRIUserByte to find it */
        if (rgate[0]=='y' && freshresp[0]=='y') {
          F_initval(10000.0, vresp_wait);  /* some big number */
          assign(one,vresp_trig); /* inizialize vresp_trigger */
          loop(vresp_wait,vresp_ctr);
            readMRIUserByte(vresp,blankingdelay);
            delay(fifopad);          /* pad delay to avoid FIFO errors */    
            if (gatezero[0] == 'o') {
              ifzero(vresp); 
                ifzero(vresp_trig);  /* it's the right condition for the trigger */      
                  assign(vresp_wait,vresp_ctr); /* exit the loop */ 
                endif(vresp_trig);
              elsenz(vresp);
                assign(zero,vresp_trig);
              endif(vresp);
            } else {                 /* gatezero not open */
              ifzero(vresp); 
                assign(zero,vresp_trig);
              elsenz(vresp);
                ifzero(vresp_trig);
                  assign(vresp_wait,vresp_ctr);  /* exit the loop */
                endif(vresp_trig);
              endif(vresp);
            }
          endloop(vresp_ctr);
          delay(respirdelay);
        }

        /* Cardiac trigger */
        if (ticks) {
          xgate(ticks);
          grad_advance(gpropdelay);       // Gradient propagation delay
          delay(GRADIENT_RES);
        }

        delay(preirdelay);

        if (rgate[0] == 'y' && freshresp[0]=='n') {
          readMRIUserByte(vdiracq,blankingdelay);
        }

        if (dir[0] == 'y') { /* hard 180 pulse for double IR pulse (black blood) */
          obspower(p3_rf.powerCoarse);	
          obspwrf(p3_rf.powerFine);
          delay(GRADIENT_RES);
          shapedpulse(p3_rf.pulseName,p3_rf.rfDuration,zero,rof1,rof2);
        } /* end of dir[0]='y' */

        obspower(ir_rf.powerCoarse);
        obspwrf(ir_rf.powerFine);
        delay(GRADIENT_RES);

        obl_shapedgradient(ssi_grad.name,ssi_grad.duration,0,0,ssi_grad.amp,NOWAIT);   
        delay(ssi_grad.rfDelayFront);
        shapedpulselist(shapeIR,ssi_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
        delay(ssi_grad.rfDelayBack);                
        obl_shapedgradient(ircrush_grad.name, ircrush_grad.duration,ircrush_grad.amp,ircrush_grad.amp,ircrush_grad.amp,WAIT);                
        delay(postirdelay);
      } /* End of Inversion pulse */

      /* Cardiac trigger if respiration gate is after cardiac trigger */
      if (!singlerr) {
        if (trigmode[0] == 'c') {
          if (ticks) {
            xgate(ticks);
            grad_advance(gpropdelay);       // Gradient propagation delay
            delay(GRADIENT_RES);
          }
        } 
      }

      /* Check respiration phase if initial steady states are over */
      ifzero(vactive);
        if (rgate[0] == 'y' ) {
          if (!singlerr) readMRIUserByte(vresp,blankingdelay);
          if (gatezero[0] == 'o') {
            if (singlerr) assign(zero,vresp);
            ifzero(vresp);
              ifzero(vdiracq);
                assign(zero,vacquire); /* acquire */
              elsenz(vdiracq);
                assign(one,vacquire);  /* don't acquire */
                decr(vpe_ctr);
              endif(vdiracq);            
            elsenz(vresp);
              assign(one,vacquire);  /* don't acquire */
              decr(vpe_ctr);
            endif(vresp);
          } else {
            if (singlerr) assign(one,vresp);
            ifzero(vresp);
              assign(one,vacquire);  /* don't acquire */
              decr(vpe_ctr);
            elsenz(vresp);
              ifzero(vdiracq);
                assign(one,vacquire);  /* don't acquire */
                decr(vpe_ctr);
              elsenz(vdiracq);
                assign(zero,vacquire); /* acquire */
              endif(vdiracq); 
            endif(vresp);
          }	  	   
        } else {  /* rgate[0]=='n' */
          assign(zero,vacquire); /* acquire */
        }
      elsenz(vactive); /* still in initial steady states */
        if (rgate[0] == 'y' && !singlerr) delay(blankingdelay);
      endif(vactive);

      /* Cardiac trigger if respiration gate is before cardiac trigger */
      if (!singlerr) {
        if (trigmode[0] == 'r') {
          if (ticks) {
            xgate(ticks);
            grad_advance(gpropdelay);       // Gradient propagation delay
            delay(GRADIENT_RES);
          }
        }
      }

      /* Prepulse options ***********************************/       
      if (sat[0]  == 'y') satbands();
      if (fsat[0] == 'y') fatsat(); 
      if (tag[0]  == 'y') tag_sinc();     

      if (ticks>0) delay(qrsdelay);

      sp1on(); delay(GRADIENT_RES); sp1off();     // Scope trigger

      /* Allow RF spoiling to be reset so that each frame starts from the same
         point in the spoiling cycle. This avoids jumps created when waiting
         for the next R wave and so replaces one artefact (banding) with 
         another (ghosting) */
      if (rfspoilreset[0] == 'y' ) {
        assign(zero,vrfspoil);
        assign(zero,vrfspoil_ctr);
      }

      if (localizer[0] =='n') getelem(t1,vpe_ctr,vpe_mult);

      loop(vcine_frames,vcine_ctr); /* CINE loop */

        if (localizer[0]=='y') {
          mult(vpe_ctr,vcine_frames,vpetab);
          add(vpetab,vcine_ctr,vpetab);
          getelem(t1,vpetab,vpe_mult);
        }

        /* Turn phase encodes off if we are dummy scanning ****/
        ifzero(vacquire);
        elsenz(vacquire);
          assign(zero,vpe_mult);
        endif(vacquire);

        /* PE rewinder follows PE table; zero if turned off ***/
        if (perewind[0] == 'y')
          assign(vpe_mult,vper_mult);
        else
          assign(zero,vper_mult);

        /* Set rcvr/xmtr phase for RF spoiling ****************/
        if (rfspoil[0] == 'y') {
          incr(vrfspoil_ctr);                   // vrfspoil_ctr = 1  2  3  4  5  6
          add(vrfspoil,vrfspoil_ctr,vrfspoil);  // vrfspoil =     1  3  6 10 15 21
          xmtrphase(vrfspoil);
          rcvrphase(vrfspoil);
        }

        /* NB rtvar vvfilp not yet used */
        if (vflip)
          getelem(t2,vcine_ctr,vvflip);  /* load RF amp from table to vvflip */
        else
          F_initval(32767.0,vvflip);

        /* RF pulse *******************************************/
        /* (re)set multislice offset frequencies */
        obspower(p1_rf.powerCoarse);
        obspwrf(p1_rf.powerFine);
        delay(GRADIENT_RES);

        /* slice select gradient*/
        obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);   
        delay(ss_grad.rfDelayFront);
        shapedpulselist(shapeEx,ss_grad.rfDuration,oph,rof1,rof2,seqcon[1],vms_ctr);
        delay(ss_grad.rfDelayBack);	  

        /* Phase encode, refocus, and dephase gradient ********/
        if (sepSliceRephase) {               // separate slice refocus gradient
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
    
        delay(ipdelay);         

      endloop(vcine_ctr); /* End CINE loop */

    endmsloop(seqcon[1],vms_ctr);

    delay(trdelay);

  endpeloop(seqcon[2],vpe_ctr);
}
