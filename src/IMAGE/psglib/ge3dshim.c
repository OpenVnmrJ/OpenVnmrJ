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
3D Gradient echo imaging sequence for gradient shimming
************************************************************************/

#include <standard.h>
#include "sgl.c"

void pulsesequence()
{
  /* Internal variable declarations *************************/ 
  double  freqEx[MAXNSLICE];
  double  pespoil_amp,spoilMoment,maxgradtime,pe2_offsetamp=0.0,nvblock;
  double  tetime,te_delay,tr_delay,perTime;
  int     table=0,shapeEx=0,sepSliceRephase=0,image,blocknvs;
  char    spoilflag[MAXSTR],perName[MAXSTR],slab[MAXSTR];

  /* Real-time variables used in this sequence **************/
  int  vpe_steps    = v1;      // Number of PE steps
  int  vpe_ctr      = v2;      // PE loop counter
  int  vpe_offset   = v3;      // PE/2 for non-table offset
  int  vpe_mult     = v4;      // PE multiplier, ranges from -PE/2 to PE/2
  int  vper_mult    = v5;      // PE rewinder multiplier; turn off rewinder when 0
  int  vpe2_steps   = v6;      // Number of PE2 steps
  int  vpe2_ctr     = v7;      // PE2 loop counter
  int  vpe2_mult    = v8;      // PE2 multiplier
  int  vpe2_offset  = v9;      // PE2/2 for non-table offset
  int  vpe2r_mult   = v10;     // PE2 rewinder multiplier
  int  vtrigblock   = v11;     // Number of PE steps per trigger block
  int  vpe          = v12;     // Current PE step out of total PE*PE2 steps

  /*  Initialize paramaters *********************************/
  init_mri();
  getstr("spoilflag",spoilflag);                                     
  getstr("slab",slab);
  image = getval("image");
  blocknvs = (int)getval("blocknvs");
  nvblock = getval("nvblock");
  if (!blocknvs) nvblock=1;    // If blocked PEs for trigger not selected nvblock=1

  trmin = 0.0;
  temin = 0.0;

  /*  Check for external PE table ***************************/
  if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
    loadtable(petable);
    table = 1;
  }

  if (ns > 1)  abort_message("No of slices must be set to one");   

  /* RF Calculations ****************************************/
  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);   /* hard pulse */
  init_rf(&p2_rf,p2pat,p2,flip2,rof1,rof2);   /* soft pulse */
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  calc_rf(&p2_rf,"tpwr2","tpwr2f");

  /* Gradient calculations **********************************/
  if (slab[0] == 'y') {
    init_slice(&ss_grad,"ss",thk);
    init_slice_refocus(&ssr_grad,"ssr");
    calc_slice(&ss_grad,&p2_rf,WRITE,"gss");
    calc_slice_refocus(&ssr_grad,&ss_grad,WRITE,"gssr");
  }
  if (FP_GT(tcrushro,0.0))
    init_readout_butterfly(&ro_grad,"ro",lro,np,sw,gcrushro,tcrushro);
  else
    init_readout(&ro_grad,"ro",lro,np,sw);
  init_readout_refocus(&ror_grad,"ror");
  calc_readout(&ro_grad,WRITE,"gro","sw","at");
  ro_grad.m0ref *= grof;
  calc_readout_refocus(&ror_grad,&ro_grad,NOWRITE,"gror");
  init_phase(&pe_grad,"pe",lpe,nv);
  init_phase(&pe2_grad,"pe2",lpe2,nv2);
  calc_phase(&pe_grad,NOWRITE,"gpe","tpe");
  if (!blocknvs) nvblock=1;
  calc_phase(&pe2_grad,NOWRITE,"gpe2","");

  if (spoilflag[0] == 'y') {                         // Calculate spoil grad if spoiling is turned on
    init_dephase(&spoil_grad,"spoil");               // Optimized spoiler
    spoilMoment = ro_grad.acqTime*ro_grad.roamp;     // Optimal spoiling is at*gro for 2pi per pixel
    spoilMoment -= ro_grad.m0def;                    // Subtract partial spoiling from back half of readout
    calc_dephase(&spoil_grad,WRITE,spoilMoment,"gspoil","tspoil");
  }

  /* Is TE long enough for separate slab refocus? ***********/
  maxgradtime = MAX(ror_grad.duration,MAX(pe_grad.duration,pe2_grad.duration));
  if (spoilflag[0] == 'y')
    maxgradtime = MAX(maxgradtime,spoil_grad.duration);
  tetime = maxgradtime + alfa + ro_grad.timeToEcho + 4e-6;
  if (slab[0] == 'y') {
    tetime += ss_grad.rfCenterBack + ssr_grad.duration;
    if ((te >= tetime) && (minte[0] != 'y')) {
      sepSliceRephase = 1;                                 // Set flag for separate slice rephase
    } else {
      pe2_grad.areaOffset = ss_grad.m0ref;                 // Add slab refocus on pe2 axis
      calc_phase(&pe2_grad,NOWRITE,"gpe2","");             // Recalculate pe2 to include slab refocus
    }
  }
 
  /* Equalize refocus and PE gradient durations *************/
  pespoil_amp = 0.0;
  perTime = 0.0;
  if ((perewind[0] == 'y') && (spoilflag[0] == 'y')) {   // All four must be single shape
    if (ror_grad.duration > spoil_grad.duration) {       // calc_sim first with ror
      calc_sim_gradient(&pe_grad,&pe2_grad,&ror_grad,tpemin,WRITE);
      calc_sim_gradient(&ror_grad,&spoil_grad,&null_grad,tpemin,NOWRITE);
    } else {                                             // calc_sim first with spoil
      calc_sim_gradient(&pe_grad,&pe2_grad,&spoil_grad,tpemin,WRITE);
      calc_sim_gradient(&ror_grad,&spoil_grad,&null_grad,tpemin,NOWRITE);
    }
    strcpy(perName,pe_grad.name);
    perTime = pe_grad.duration;
    putvalue("tspoil",perTime);
    putvalue("gspoil",spoil_grad.amp);
  } else {                      // post-acquire shape will be either pe or spoil, but not both
    calc_sim_gradient(&ror_grad,&pe_grad,&pe2_grad,tpemin,WRITE);
    if ((perewind[0] == 'y') && (spoilflag[0] == 'n')) {     // Rewinder, no spoiler
      strcpy(perName,pe_grad.name);
      perTime = pe_grad.duration;
      spoil_grad.amp = 0.0;
      putvalue("tpe",perTime);
    } else if ((perewind[0] == 'n') && (spoilflag[0] == 'y')) {  // Spoiler, no rewinder
      strcpy(perName,spoil_grad.name);
      perTime = spoil_grad.duration;
      pespoil_amp = spoil_grad.amp;      // Apply spoiler on PE & PE2 axis if no rewinder
    }
  }

  if (slab[0] == 'y') pe2_offsetamp = sepSliceRephase ? 0.0 : pe2_grad.offsetamp;  // pe2 slab refocus

  /* Create optional prepulse events ************************/
  if (sat[0] == 'y')  create_satbands();
  if (fsat[0] == 'y') create_fatsat();

  sgl_error_check(sglerror);                               // Check for any SGL errors
  
  /* Min TE ******************************************/
  tetime = pe_grad.duration + alfa + ro_grad.timeToEcho;
  if (slab[0] == 'y') {
    tetime += ss_grad.rfCenterBack;
    tetime += (sepSliceRephase) ? ssr_grad.duration : 0.0;   // Add slice refocusing if separate event
  }
  else if (ws[0] == 'y')
    tetime += p2/2.0 + rof2;	/* soft pulse */
  else
    tetime += p1/2.0 + rof2;	/* hard pulse */
  temin = tetime + 4e-6;                                   // Ensure that te_delay is at least 4us
  if (minte[0] == 'y') {
    te = temin;
    putvalue("te",te);
  }
  if (te < temin) {
    abort_message("TE too short.  Minimum TE= %.2fms\n",temin*1000+0.005);   
  }
  te_delay = te - tetime;

  /* Min TR ******************************************/   	
  trmin  = te_delay + pe_grad.duration + ro_grad.duration + perTime;
  if (slab[0] == 'y') {
    trmin += ss_grad.duration;
    trmin += (sepSliceRephase) ? ssr_grad.duration : 0.0;   // Add slice refocusing if separate event
  }
  else if (ws[0] == 'y')
    trmin += p2 +rof1 + rof2;	/* soft pulse */
  else
    trmin += p1 +rof1 + rof2;	/* hard pulse */
  trmin += 8e-6;

  /* Increase TR if any options are selected *********/
  if (sat[0] == 'y')  trmin += satTime;
  if (fsat[0] == 'y') trmin += fsatTime;
  if (ticks > 0) trmin += 4e-6;

  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr);
  }
  if (FP_LT(tr,trmin)) {
    abort_message("TR too short.  Minimum TR = %.2fms\n",trmin*1000+0.005);
  }

  /* Calculate tr delay */
  tr_delay = granularity(tr-trmin,GRADIENT_RES);

  if(slab[0] == 'y') {
    /* Generate phase-ramped pulses: 90 */
    offsetlist(pss,ss_grad.ssamp,0,freqEx,ns,seqcon[1]);
    shapeEx = shapelist(p1pat,ss_grad.rfDuration,freqEx,ns,ss_grad.rfFraction,seqcon[1]);
  }

  /* Set pe_steps for profile or full image **********/   	
  pe_steps = prep_profile(profile[0],nv,&pe_grad,&null_grad);
  F_initval(pe_steps/2.0,vpe_offset);

  pe2_steps = prep_profile(profile[1],nv2,&pe2_grad,&null_grad);
  F_initval(pe2_steps/2.0,vpe2_offset);

  assign(zero,oph);

  /* Shift DDR for pro *******************************/   	
  roff = -poffset(pro,ro_grad.roamp);

  /* Adjust experiment time for VnmrJ *******************/
  g_setExpTime(tr*(nt*pe_steps*pe2_steps));

  /* PULSE SEQUENCE *************************************/
  status(A);
  rotate();
  triggerSelect(trigger);       // Select trigger input 1/2/3
  obsoffset(resto);
  delay(4e-6);

  /* trigger */
  if (ticks > 0) F_initval((double)nvblock,vtrigblock);

  /* Begin phase-encode loop ****************************/       
  peloop2(seqcon[3],pe2_steps,vpe2_steps,vpe2_ctr);

    peloop(seqcon[2],pe_steps,vpe_steps,vpe_ctr);

      delay(tr_delay);   // relaxation delay

      sub(vpe_ctr,vpe_offset,vpe_mult);
      sub(vpe2_ctr,vpe2_offset,vpe2_mult);

      mult(vpe2_ctr,vpe_steps,vpe);
      add(vpe_ctr,vpe,vpe);

      /* PE rewinder follows PE table; zero if turned off ***/       
      if (perewind[0] == 'y') {
        assign(vpe_mult,vper_mult);
        assign(vpe2_mult,vpe2r_mult);
      }
      else {
        assign(zero,vper_mult);
        assign(zero,vpe2r_mult);
      }

      if (ticks > 0) {
        modn(vpe,vtrigblock,vtest);
        ifzero(vtest);                // if the beginning of an trigger block
          xgate(ticks);
          grad_advance(gpropdelay);
          delay(4e-6);
        elsenz(vtest);
          delay(4e-6);
        endif(vtest);
      }

      sp1on(); delay(4e-6); sp1off(); // Scope trigger

      /* Prepulse options ***********************************/
      if (sat[0] == 'y')  satbands();
      if (fsat[0] == 'y') fatsat();

      if (slab[0] == 'y') {
        obspower(p2_rf.powerCoarse);
        obspwrf(p2_rf.powerFine);
        delay(4e-6);
	obl_shapedgradient(ss_grad.name,ss_grad.duration,0,0,ss_grad.amp,NOWAIT);
	delay(ss_grad.rfDelayFront);
	shapedpulselist(shapeEx,ss_grad.rfDuration,zero,rof1,rof2,seqcon[1],zero);
	delay(ss_grad.rfDelayBack);
        if (sepSliceRephase) {
          obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);
          delay(te_delay + tau);   /* tau is current B0 encoding delay */
        }
      } else {
        obspower(p1_rf.powerCoarse);
        obspwrf(p1_rf.powerFine);
        delay(4e-6);
        if (ws[0] == 'y')
          shapedpulse(p2pat,p2,zero,rof1,rof2);   /* soft CS pulse */
        else
          shapedpulse(p1pat,p1,zero,rof1,rof2);   /* hard pulse */
        delay(te_delay + tau);   /* tau is current B0 encoding delay */
      }        

      pe2_shapedgradient(pe_grad.name,pe_grad.duration,-ror_grad.amp*image,0,-pe2_offsetamp,
          -pe_grad.increment,-pe2_grad.increment,vpe_mult,vpe2_mult,WAIT);

      if ((slab[0] == 'y') && !sepSliceRephase) delay(te_delay + tau);   /* tau is current B0 encoding delay */

      /* Readout gradient and acquisition ********************/
      obl_shapedgradient(ro_grad.name,ro_grad.duration,ro_grad.amp*image,0,0,NOWAIT);
      delay(ro_grad.atDelayFront);
      startacq(alfa);
      acquire(np,1.0/sw);
      delay(ro_grad.atDelayBack);
      endacq();

      /* Rewind / spoiler gradient *********************************/
      if (perewind[0] == 'y' || (spoilflag[0] == 'y')) {
        pe2_shapedgradient(perName,perTime,spoil_grad.amp,pespoil_amp,pespoil_amp,
          pe_grad.increment,pe2_grad.increment,vper_mult,vpe2r_mult,WAIT);
      }  

    endpeloop(seqcon[2],vpe_ctr);

  endpeloop(seqcon[3],vpe2_ctr);

}
