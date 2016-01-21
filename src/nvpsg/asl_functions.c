/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ASL variables */
int    asltag;          // tag/control flag
int    asltype;         // type of tag: FAIR, EPISTAR, PICORE
char   quipss[MAXSTR];  // QUIPSS II/Q2TIPS saturation flag
double irthk,           // thickness of IR slab
       irgap,           // distance from imaging slice
       ssiamp,          // amplitude of ssi gradient; depends on asltag
       shapeIR,         // RF phase-ramped shapes
       shapeQtag,
       aslgcrush, asltcrush,
       qgcrush,qtcrush, // Crushers after Q2TIPS saturation
       ti1,             // Time to apply Q2TIPS sat bands
       qTime=0;         // Duration of Q2TIPS saturation block

GENERIC_GRADIENT_T      aslcrush_grad;
SLICE_SELECT_GRADIENT_T q2tips_grad;
GENERIC_GRADIENT_T      qcrush_grad;

enum {FAIR,STAR,PICORE};


void get_asl_parameters() {
  char asltype_str[MAXSTR];
  
  asltag    = (int) getval("asltag");
  irthk     =       getval("irthk");
  irgap     =       getval("irgap");
  qgcrush   =       getval("qgcrush");
  qtcrush   =       getval("qtcrush");
  nsat      = (int) getval("nsat");
  ti1       =       getval("ti1");
  getstr("asltype",asltype_str);
  getstr("quipss",quipss);
  
  if ((!strcmp(asltype_str,"fair")) || (!strcmp(asltype_str,"FAIR")))
    asltype = FAIR;
  else if ((!strcmp(asltype_str,"star")) || (!strcmp(asltype_str,"STAR")))
    asltype = STAR;
  else if ((!strcmp(asltype_str,"picore")) || (!strcmp(asltype_str,"PICORE")))
    asltype = PICORE;
  else
    abort_message("Unknown tag type %s",asltype_str);
}


void prep_asl() {
  double pssir[MAXSLICE],pss_q2tips[MAXSLICE],
         ir_dist;
  double freqIR[MAXSLICE],freqQ[MAXSLICE];
  int    s;

  /* Calculate all RF and Gradient shapes */    
  init_rf(&ir_rf,pipat,pi,flipir,rof1,rof1); 
  calc_rf(&ir_rf,"tpwri","tpwrif"); 

  init_slice(&ssi_grad,"aslssi",irthk);
  calc_slice(&ssi_grad,&ir_rf,WRITE,"");
  
  init_generic(&aslcrush_grad,"aslcrush",gcrushir,tcrushir);
  calc_generic(&aslcrush_grad,WRITE,"","");
  
  
  if (diff[0] == 'y') {
    init_generic(&diff_grad,"asldiff",gdiff,tdelta);
    diff_grad.maxGrad = gmax;
    calc_generic(&diff_grad,NOWRITE,"","");
    /* adjust duration, so tdelta is from start ramp up to start ramp down */
    if (ix == 1) {
      diff_grad.duration += diff_grad.tramp; 
      calc_generic(&diff_grad,WRITE,"","");
    }
  }


  /* Set up list of phase ramped pulses */
  /* Create list of slice positions for IR pulse, based on tag type */
  ir_dist = thk/10/2 + irgap + irthk/10/2;  // thk & irthk in mm
  ssiamp = ssi_grad.amp;  // keep in ssiamp; ssi_grad is only calculated for ix==1
  for (s = 0; s < ns; s++) {
    switch (asltype) {
      case FAIR:   // IR on imaging slice, selective vs non-selective
                   pssir[s]  = pss[s];
		   pss_q2tips[s] = pss[s] + (thk/10/2 + irgap + satthk[0]/10/2);
		   /* quipss with FAIR doesn't make sense, but set it just in case */
		   if (asltag == -1) ssiamp = 0;
                   break;
      case STAR:   // IR proximal vs distal to imaging slice 
                   if (asltag == 1){
		     pssir[s]      = pss[s] + ir_dist;
		   }
		   else if (asltag == -1) {
		     pssir[s]      = pss[s] - ir_dist;
		   }
		   else {
		     pssir[s]      = pss[s] + ir_dist;  // not used
		   }
                   break;
      case PICORE: // IR proximal to imaging slice, selective vs non-selective
                   pssir[s]      = pss[s] + ir_dist;
		   if (asltag == -1) ssiamp = 0;
                   break;
      default:     break;
    }
  }
  offsetlist(pssir,ssiamp,0,freqIR,ns,seqcon[1]);
  shapeIR = shapelist(pipat,ssi_grad.rfDuration,freqIR,ns,0,seqcon[1]);
  

  /* Set up Q2TIPS RF and Gradients */
  if (quipss[0] == 'y') {
    init_rf(&sat_rf,satpat,psat,flipsat,rof1,rof2); 
    calc_rf(&sat_rf,"tpwrsat","tpwrsatf"); 

    init_slice(&sat_grad,"q2tips",satthk[0]);
    calc_slice(&sat_grad,&sat_rf,WRITE,"");

    init_generic(&qcrush_grad,"qcrush",qgcrush,qtcrush);
    calc_generic(&qcrush_grad,WRITE,"","");

    for (s = 0; s < ns; s++) {
      if (asltag == 1){
        pss_q2tips[s] = pss[s] + (thk/10/2 + irgap + satthk[0]/10/2);
      }
      else if (asltag == -1) {
        pss_q2tips[s] = pss[s] - (thk/10/2 + irgap + satthk[0]/10/2);
      }
      else {
        pss_q2tips[s] = pss[s] + (thk/10/2 + irgap + satthk[0]/10/2);
      }
    }


    offsetlist(pss_q2tips,sat_grad.ssamp,0,freqQ,ns,seqcon[1]);
    shapeQtag = shapelist(satpat,sat_grad.rfDuration,freqQ,ns,0,seqcon[1]);
    
    qTime   = nsat*(sat_grad.duration + qcrush_grad.duration);
  
  }


}




