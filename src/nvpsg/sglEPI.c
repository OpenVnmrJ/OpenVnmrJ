/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************************
*   HISTORY:
*     Revision 1.1  2006/09/11 16:21:48  deans
*     Added Maj's changes for epi
*     At Maj's request, renamed sglEpi.h and sglEpi.c -> sglEPI.h and sglEPI.c
*
*     Revision 1.2  2006/08/29 17:25:30  deans
*     1. changed sgl includes to use quotes (vs brackets)
*     2. added ncomm sources
*     3. mods to Makefile
*     4. added makenvpsg.sgl and makenvpsg.sgl.lnx
*
*     Revision 1.1  2006/08/23 14:09:58  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/08/22 23:30:02  deans
*     *** empty log message ***
*
*     Revision 1.3  2006/07/11 20:09:58  deans
*     Added explicit prototypes for getvalnowarn etc. to sglCommon.h
*     - these are also defined in  cpsg.h put can't #include that file because
*       gcc complains about the "extern C" statement which is only allowed
*       when compiling with g++ (at least for gcc version 3.4.5-64 bit)
*
*     Revision 1.2  2006/07/07 20:10:19  deans
*     sgl library changes
*     --------------------------
*     1.  moved most core functions in sgl.c to sglCommon.c (new).
*     2. sgl.c now contains only pulse-sequence globals and
*          parameter initialization functions.
*     3. sgl.c is not built into the nvpsg library but
*          is compiled in with the user sequence using:
*          include "sgl.c"
*        - as before ,so sequences don't need to be modified
*     4. sgl.h is no longer used and has been removed from the project
*
*     Revision 1.1  2006/07/07 01:11:30  mikem
*     modification to compile with psg
*
*
***************************************************************************/
#include "sglCommon.h"
#include "sglEPI.h"

/**************************************************************************/
/*** EPI WRAPPERS *********************************************************/
/**************************************************************************/
void init_epi(EPI_GRADIENT_T *epi_grad){  
  epi_grad->np_ramp     = 0;
  epi_grad->np_flat     = 0;
  epi_grad->tblip       = getval("tblip");
  epi_grad->skip        = getval("skip");

  epi_grad->tblip = MAX(epi_grad->tblip,120e-6);

  if (strlen(ky_order) == 1)  /* single character */
    ky_order[1] = 'n';        /* not reversed */
  
  epi_grad->skip        = 0; 
  epi_grad->error       = 0;

  epi_grad->ddrsr = getval("ddrsr");  // DDR sample rate limits dwell time resolution
  if (epi_grad->ddrsr == 0) epi_grad->ddrsr = 2.5e6;
}

/**************************************************************************/
/*** calc_readout_rampsampling*********************************************/
/* Calculates a TRAPEZOIDAL gradient optimized for ramp sampling          */
/* Option to specify a minimum "skipped" part of the ramp where           */
/* data acquisition does not occur (e.g., PE blip).                       */
/* Also calculates an array of dwell times for non-linear acquisition.    */
/**************************************************************************/
void calc_readout_rampsamp(READOUT_GRADIENT_T *grad, double *dwell, double *minskip, int *npr) {
  double this_gro, skip, skip_m0, dwint, dw, dwflat;
  double ramp_t, ramp_m0, flat_t, flat_m0, dwell_sum, rt;
  int    npro, np_ramp, np_flat;
  int    pt, pt2;
  GENERIC_GRADIENT_T gg;
  
  /* Calculate gradient amplitude based on FOV and sw */
  this_gro       = sw/grad->gamma/lro;
  grad->amp      = this_gro;

  /* Nominal dwell time is 1/sw */
  dw    = granularity(1/sw,1/epi_grad.ddrsr);
  dwflat = dw;  /* we may later end up with different dw on flat part */
  dwint = dw*grad->amp;   /* Gradient Integral per point */
  npro  = (int) np/2;

  /* Calculate the area that is skipped for phase encoding blip */
  skip    = *minskip;
  if (skip == 0)
    skip = dw + getval("aqtm")-at;                /* Need at least a dwell time at the end */

  skip_m0 = skip*(skip*grad->slewRate)/2;         /* Area of that skipped part */

  /* Calculate the ramp time */
  grad->tramp     = granularity(grad->amp/grad->slewRate,grad->resolution);
  grad->slewRate  = grad->amp/grad->tramp;

  ramp_m0 = grad->tramp*grad->amp/2;
  np_ramp = (int) ((ramp_m0 - skip_m0)/dwint);
  
  /* Check that we even need points on the flat part */
  if (np_ramp >= npro/2) {
    np_ramp = npro/2 - 1;  /* Center two points considered "flat part" */
    
    /* We may not need to go all the way to full gradient
       in order to fit all points on ramp; adjust amplitude */
    ramp_m0 = np_ramp*dwint + skip_m0;
    grad->tramp = granularity(sqrt(2*ramp_m0/grad->slewRate),grad->resolution);
    grad->amp   = grad->tramp * grad->slewRate;

    /* recalculate ramp area and np_ramp */
    ramp_m0 = grad->tramp*grad->amp/2;
    
    /* Now adjust the dwell time necessary on flat part with this amplitude */
    dwflat = granularity(dwint/grad->amp,1/epi_grad.ddrsr);
  }

  np_flat = npro - 2*np_ramp;

  /* How long must the flat part be? */
  grad->duration = granularity((np_flat-1)*dwflat,grad->resolution); 

  /* Due to rounding of gradient duration, 
     we may be able to fit more points on flat part */
  while (grad->duration - (np_flat-1)*dwflat >= 2*dwflat) {
    np_flat += 2;
    np_ramp -= 1;
  }

  /* Now add the ramp times to the total duration */
  grad->duration += 2*grad->tramp; 
  
  /* Use a generic to calculate the actual shape, ie fill in dataPoints */
  initGeneric(&gg);
  gg.amp         = grad->amp;
  gg.duration    = grad->duration;
  gg.tramp       = grad->tramp;
  gg.slewRate    = grad->slewRate;
  gg.calcFlag    = MOMENT_FROM_DURATION_AMPLITUDE_RAMP;
  gg.writeToDisk = FALSE;

  calcGeneric(&gg);

  if (gg.error != ERR_NO_ERROR) {
    printf("Temporary generic gradient: \n");
    displayGeneric(&gg);
    abort_message("Problem calculating RO gradient with ramp sampling");
  }

  grad->m0         = gg.m0;
  grad->m1         = gg.m1;
  grad->m0ref      = grad->m0/2;
  grad->numPoints  = gg.numPoints;
  free(grad->dataPoints); // release original array
  grad->dataPoints = gg.dataPoints;

  /* Recalculate the skipped part with new gradient integral */
  skip_m0 = (grad->m0 - dwint*(np/2-1))/2;
  skip = sqrt(2*skip_m0/grad->slewRate);

  /* Skip rampsampling if gradient is longer than necessary */
  if ((grad->m0-ramp_m0*2) > npro*dwint) {
    warn_message("The gradient is too long, no ramp sampling");
    for (pt = 0; pt < npro; pt++) {
      dwell[pt]  = dwflat;
    }
    *minskip = grad->tramp;
    return;
  }

  /* Calculate where we start acquisition on the ramp */
  /* Do we have extra integral on flat part? */
  flat_m0 = (grad->m0 - 2*ramp_m0) - ((np_flat-1)*dwflat*grad->amp);
  
  /* if yes, then what is the duration of this? */
  if (flat_m0 > 0)
    flat_t = (grad->duration - 2*grad->tramp) / ((np_flat - 1)*dwflat);
  else
    flat_t = 0;

  /* Double-check that gradient is long enough */
  if (flat_m0 < -1e-9)
    abort_message("Gradient integral too small %f vs %f (%f) G/cm*us", 
                   (grad->m0-2*ramp_m0)*1e6,(np_flat-1)*dwint*1e6,flat_m0*1e9);

  /* If extra integral > 2*dwint, we could move a point from each ramp up to the flat */
  if (flat_m0 > 2*dwint)
    warn_message("Gradient flat part longer than necessary with ramp sampling, check gradient calculation\n");


  /**************************************************************************/
  /* Calculate dwell times */
  /**************************************************************************/
  dwell_sum = 0;
  /* Start at top of ramp and walk down ramp */
  pt = np_ramp - 1;
  rt = grad->tramp;  

  /* Top point on ramp first, it may use a bit of time from the flat part */
  if (flat_m0 > 0) {
    ramp_m0 -= (dwint - flat_m0/2);  /* area at ramp at previous point */
    flat_t = (grad->duration - 2*grad->tramp) - ((np_flat - 1)*dwflat); /* extra duration at top */
    flat_t /= 2;  /* divide evenly between both ramps */
  }
  else {
    ramp_m0 -= dwint;  /* area at ramp at previous point */
    flat_t = 0;
  }
  ramp_t    = sqrt(2*ramp_m0/grad->slewRate); /* time at ramp at previous point */
  dwell[pt] = granularity(flat_t + (rt-ramp_t), 1/epi_grad.ddrsr);
  dwell_sum += dwell[pt];

  ramp_t = rt - (dwell[pt] - flat_t); /* correct for rounding of dwell */
  /* Do all the rest of the points on the ramp */
  for(pt = np_ramp-2; pt >= 0; pt--) {  
    rt = ramp_t;       /* remember current time */
    ramp_m0 -= dwint;  /* total area of ramp at previous point */
    if (ramp_m0 <= 0) {
      if (fabs(ramp_m0) > dwint*0.01) abort_message("problem: ramp_m0 negative %f\n",ramp_m0*1000);
      ramp_t = 0;
    }
    else ramp_t  = sqrt(2*ramp_m0/grad->slewRate);  /* time at previous point */
    dwell[pt]  = granularity(rt-ramp_t,1/epi_grad.ddrsr);
    dwell_sum += dwell[pt];

    ramp_t = rt - dwell[pt];
  }       

  skip = ramp_t;
  dwell_sum += skip;

  for (pt = np_ramp; pt < np_ramp+np_flat-1; pt++) {  /* Flat part */
    dwell[pt]  = dwflat;
    dwell_sum += dwflat;
  }

  pt2 = np_ramp-1;
  for (pt = np_ramp+np_flat-1; pt < np_ramp*2 + np_flat - 1; pt++) {
    dwell[pt] = dwell[pt2--];
    dwell_sum += dwell[pt];
  }
  /* Very last point, just set dwell = 1/sw */
  pt = np_ramp*2 + np_flat - 1;
  dwell[pt] = dw;
  dwell_sum += skip;
  
  if (fabs(dwell_sum - grad->duration) > 12.5e-9)
    warn_message("Mismatch in sum of dwells (%.3fus) vs. gradient duration (%.3fus)",
                 dwell_sum*1e6,grad->duration*1e6);


  /* Return values */
  *minskip = skip;
  *npr     = np_ramp;

  if (sgldisplay) {    
    printf("============== DEBUG DWELL  (%d, %d, %d) ===============\n",np_ramp,np_flat,np_ramp);
    dwell_sum = skip;
    for (pt = 0; pt < npro-1; pt++) {
      printf("[%3d] %3.6f\t%3.3f\t%3.6f\n",pt+1,dwell_sum*1e6,dwell[pt]*1e6,(dwell[pt]+dwell_sum)*1e6);
      dwell_sum += dwell[pt];  
    }
    printf("[%3d] %3.6f\t%3.3f\t%3.6f\n",(pt+1),dwell_sum*1e6,0.0,dwell_sum*1e6);pt++;
    printf("[%3d] %3.6f\t%3.3f\t%3.6f\n",pt+1,dwell_sum*1e6,skip*1e6,(skip+dwell_sum)*1e6);
    dwell_sum += skip;

    printf("Did we get a full gradient's worth? %.6f, %.6f\n",grad->duration*1e6,dwell_sum*1e6);
    printf("=======================================================\n");
  }
  
}


void calc_epi(EPI_GRADIENT_T          *epi_grad, 
              READOUT_GRADIENT_T      *ro_grad,
              PHASE_ENCODE_GRADIENT_T *pe_grad,
              REFOCUS_GRADIENT_T      *ror_grad,
              PHASE_ENCODE_GRADIENT_T *per_grad,
              READOUT_GRADIENT_T      *nav_grad,
              int write_flag) {

  /* Variables for generating gradient shapes */
  double dwint,blipint,skip,dw;
  int    np_ramp, np_flat, npro;
  double *dwell, *dac;
  double pos, neg;
  int    pt, pts, inx=0, lobe, lobes, blippts, zeropts;
    
  /* Variables for generating tables */
  int  n, seg, steps;
  char order_str[MAXSTR],gpe_tab[MAXSTR],tab_file[MAXSTR];
  FILE *fp;

  /********************************************************************************/
  /* Error Checks */
  /********************************************************************************/
  /* make sure that the combination of nseg, ky_order and fract_ky make sense */
  if (ky_order[0] == 'c') {
    /* Can't do just one segment with centric */
    if (nseg == 1) {
      if (ix == 1) {
        warn_message("WARNING %s: ky_order set to 'l'\n",seqfil);
        warn_message("            Only linear ordering allowed with single-shot acquisition\n");
      }
      ky_order[0] = 'l';
    }
    /* Must have even number of shots with centric ordering */
    if (((int) nseg % 2) == 1)
      abort_message("ERROR %s: Must do even number of shots with centric ordering\n",seqfil);

    /* Can't do fractional ky with centric acquisition */
    if (fract_ky != pe_grad->steps/2)
      abort_message("ERROR %s: fract_ky must be = nv/2 (%d) for centric acquisition\n",seqfil,(int) (pe_grad->steps/2));
  }

  if (fract_ky > pe_grad->steps/2)
    abort_message("ERROR %s: fract_ky must be <= nv/2 (%d)\n",seqfil,(int) (pe_grad->steps/2));


  /* calculate etl */
  switch(ky_order[0]) {
    case 'l': 
      epi_grad->etl = (pe_grad->steps/2 + fract_ky)/nseg; 
      epi_grad->center_echo = fract_ky/nseg - 1;

      if (epi_grad->etl - ((int) epi_grad->etl) > 0.005)
        abort_message("%s: Echo train length ((%d/2+%d)/%d = %.2f) not an integer\n",
	  seqfil,(int)pe_grad->steps,(int) fract_ky, (int) nseg,epi_grad->etl);

      break;
    case 'c': 
      epi_grad->etl = pe_grad->steps/nseg; 
      epi_grad->center_echo = 0;
      if (epi_grad->etl - ((int) epi_grad->etl) > 0.005)
        abort_message("%s: Echo train length (%d/%d) not an integer\n",
	  seqfil,(int)pe_grad->steps,(int) nseg);

      break;
    default:
      abort_message("%s: ky_order %s not recognized, use 'l' (linear) or 'c' (centric)\n",
                     seqfil, ky_order);
      break;
  }	    
  epi_grad->center_echo += ssepi*2;


  /********************************************************************************/
  /* Generate a single phase encoding blip and the dephaser                       */
  /********************************************************************************/
  blipint = 1/(pe_grad->fov/10*nuc_gamma()); /* base step in PE dimension */
  switch(ky_order[0]) {
    case 'l': /* each blip will jump nseg lines in ky: */
      pe_grad->m0     = blipint*nseg;
      per_grad->steps = pe_grad->steps;   /* each increment is = one blip unit */
      per_grad->m0    = blipint*per_grad->steps/2; /* start at nv/2, step 1 */
      break;
    case 'c': /* each blip will jump nseg/2 lines in ky: */
      pe_grad->m0     = blipint*nseg/2;
      per_grad->steps = nseg; 
      per_grad->m0    = blipint*per_grad->steps/2;  /* start at nseg/2, step 1 */
      break;
    default:  
      abort_message("ky_order %s not recognized, use 'l' (linear) or 'c' (centric)\n",ky_order);
      break;
  }

  /* Phase encoding blip */
  pe_grad->calcFlag  = SHORTEST_DURATION_FROM_MOMENT;
  pe_grad->writeToDisk = FALSE;
  calcPhase(pe_grad);
 
  if ((pe_grad->duration < epi_grad->tblip) || (pe_grad->amp/pe_grad->tramp > pe_grad->slewRate)) {
    pe_grad->tramp    = granularity(MAX(epi_grad->tblip/2,pe_grad->amp/pe_grad->slewRate),pe_grad->resolution);
    pe_grad->duration = 2*pe_grad->tramp;
    pe_grad->calcFlag = AMPLITUDE_FROM_MOMENT_DURATION_RAMP;
    calcPhase(pe_grad);
  }

  /* Phase encoding dephaser */
  per_grad->calcFlag = SHORTEST_DURATION_FROM_MOMENT;
  per_grad->maxGrad *= glim;
  per_grad->writeToDisk = write_flag;
  calcPhase(per_grad);
  
  /* Now adjust the initial dephaser for fractional k-space */
  per_grad->amp *= (fract_ky/(pe_grad->steps/2));

  /* Create an array for dwell times */
  npro = ro_grad->numPointsFreq; 
  if ((dwell = (double *)malloc(sizeof(double)*npro)) == NULL) {
    abort_message("Can't allocate memory for dwell");
  }

  /********************************************************************************/
  /* Generate a single readout lobe */  
  /********************************************************************************/
  ro_grad->writeToDisk = nav_grad->writeToDisk = FALSE;
  calcReadout(ro_grad);

  dw    = granularity(1/sw,1/epi_grad->ddrsr);
  dwint = dw*ro_grad->amp;

  skip  = getval("skip");              /* User specified min delay between acquisitions */
  skip  = MAX(skip,pe_grad->duration); /* Is PE blip longer? Then use that */
  skip /= 2;  /* in further calculations, skip is the skipped part on either ramp */

  /* If RO ramp - skip isn't at least one dwell, then we can't do ramp sampling */
  if (ro_grad->tramp - (skip + dw + getval("aqtm") - at) < dw) {
    if (rampsamp[0] == 'y') {
      printf("Blip duration or Min echo spacing is long (%.2fus), ramp sampling turned off",2*skip*1e6);
      rampsamp[0] = 'n';
    }
    /* set ramp time and recalculate */
    ro_grad->tramp = skip + (dw + getval("aqtm") - at); 
    calcReadout(ro_grad);
  }
    
  /********************************************************************************/
  if (rampsamp[0] == 'n') {  /* No rampsampling, just use simple RO shape */
  /********************************************************************************/
    np_ramp = 0; 
    np_flat = npro;
    
    skip = (ro_grad->atDelayFront + ro_grad->atDelayBack)/2;

    /* Set dwell array */
    for (pt = 0; pt < npro; pt++) 
      dwell[pt] = dw;
    dwell[npro-1] = 2*dw;  /* gradient duration is actually dw too long */

  }
  /********************************************************************************/
  else {  /* rampsampling */
  /********************************************************************************/
    calc_readout_rampsamp(ro_grad,dwell,&skip,&np_ramp);
    np_flat = npro - 2*np_ramp;

  }  /* end if rampsampling = 'y' */

  switch (ro_grad->error) {
    case ERR_NO_ERROR:
      break;
    case ERR_AMPLITUDE:
      abort_message("Readout gradient too large, increase FOV to %.2fmm",
        ro_grad->bandwidth/(ro_grad->gamma * ro_grad->maxGrad * MM_TO_CM));
      break;
    default:
      abort_message("Error in calculation of readout gradient (error %d)",
        (int)ro_grad->error);
      break;
      
  }


  /* We now have a single lobe, keep that for the navigator echo */
  nav_grad->amp        = ro_grad->amp; 
  nav_grad->duration   = ro_grad->duration; 
  nav_grad->tramp      = ro_grad->tramp; 
  nav_grad->m0         = ro_grad->m0;
  nav_grad->m0ref      = ro_grad->m0ref;
  nav_grad->dataPoints = ro_grad->dataPoints;
  nav_grad->numPoints  = ro_grad->numPoints;
  nav_grad->slewRate   = ro_grad->slewRate;


  /********************************************************************************/
  /* Readout dephaser */
  /********************************************************************************/
  ror_grad->balancingMoment0  = nav_grad->m0ref; /* ideal moment */
  /* adjust with grora tweaker */
  if (grora == 0) {
    warn_message("grora tweaker is 0, probably using old protocol - changed to 1.0");
    grora = 1.0;
  }
  ror_grad->balancingMoment0 *= grora;
  ror_grad->writeToDisk       = write_flag;
  
  calcRefocus(ror_grad);

  /********************************************************************************/
  /* Expand gradient shapes to full echo train                                    */
  /********************************************************************************/
  /* Readout - The navigator shape holds a single lobe                            */
  /********************************************************************************/
  /* Does positive or negative lobe need to be adjusted for tweaker? */
  pos = neg = 1;
  if (groa >= 0)  /* Adjust negative downwards, since positive is already at max */
    neg = (1 - groa/nav_grad->amp);
  else /* groa < 0, Adjust positive downwards */
    pos = (1 + groa/nav_grad->amp);

  /* Increase the size of ro shape to hold full shape */
  /* free(ro_grad->dataPoints); Don't free this, it's now assigned to the navigator echo */
  lobes = (epi_grad->etl + ssepi*2);
  ro_grad->numPoints *= lobes;
  
  if ((ro_grad->dataPoints = (double *)malloc(ro_grad->numPoints*sizeof(double))) == NULL) 
    abort_message("%s: Problem allocating memory for EPI readout gradient",seqfil);
  ro_grad->duration *= lobes;

  /* Concatenate positive and negative lobes        */
  /* until we have a full echo train (plus ssepi)  */
  pts = nav_grad->numPoints;
  inx = 0;
  for (lobe = 0; lobe < lobes/2; lobe++) {
    for (pt = 0; pt < pts; pt++)
      ro_grad->dataPoints[inx++] =  nav_grad->dataPoints[pt] * pos;
    for (pt = 0; pt < pts; pt++)
      ro_grad->dataPoints[inx++] = -nav_grad->dataPoints[pt] * neg;
  }


  /********************************************************************************/
  /* Phase encoding  */
  /********************************************************************************/
  /* Keep blip */
  blippts = pe_grad->numPoints;
  if ((dac = (double *)malloc(blippts*sizeof(double))) == NULL) 
    abort_message("%s: Problem allocating memory for EPI blip",seqfil);

  for (pt = 0; pt < blippts; pt++)
    dac[pt] = pe_grad->dataPoints[pt];

  /* Increase the size of pe shape to hold full shape */
  free(pe_grad->dataPoints);
  if ((pe_grad->dataPoints = (double *)malloc(ro_grad->numPoints*sizeof(double))) == NULL) 
    abort_message("%s: Problem allocating memory for EPI phase encoding gradient",seqfil);

  /* Pad front of shape - including ssepi time - with zeros */
  inx = 0;
  lobes = ssepi*2;
  zeropts = nav_grad->numPoints;
  for (lobe = 0; lobe < lobes; lobe++) {
    for (pt = 0; pt < zeropts; pt++) {
      pe_grad->dataPoints[inx++] = 0;
    }
  }
  zeropts = nav_grad->numPoints - blippts/2;
  for (pt = 0; pt < zeropts; pt++)
    pe_grad->dataPoints[inx++] = 0;

  lobes = epi_grad->etl-1;
  zeropts = nav_grad->numPoints - blippts;
  for (lobe=0; lobe < lobes; lobe++) {
    for (pt = 0; pt < blippts; pt++)    
      pe_grad->dataPoints[inx++] = dac[pt];
    for (pt = 0; pt < zeropts; pt++)
      pe_grad->dataPoints[inx++] = 0;
  }
  
  /* Add a few zeros, half the duration of the blip + one lobe, at the very end */
  zeropts = blippts/2 + nav_grad->numPoints;
  zeropts = blippts/2;
  for (pt = 0; pt < zeropts; pt++)
    pe_grad->dataPoints[inx++] = 0;

  pe_grad->numPoints = ro_grad->numPoints;
  pe_grad->duration  = ro_grad->duration;
  
  /********************************************************************************/
  /* Keep some parameters in EPI struct */
  /********************************************************************************/
  epi_grad->skip      = skip;
  epi_grad->np_flat   = np_flat;
  epi_grad->np_ramp   = np_ramp;
  epi_grad->dwell     = dwell;
  epi_grad->duration  = ro_grad->duration;
  epi_grad->numPoints = ro_grad->numPoints;
  epi_grad->amppos    =  nav_grad->amp*pos;
  epi_grad->ampneg    = -nav_grad->amp*neg;
  epi_grad->amppe     =  pe_grad->amp;

  
  /********************************************************************************/
  /* Write shapes to disk */
  /********************************************************************************/
  if (writeToDisk(ro_grad->dataPoints, ro_grad->numPoints, 0, 
                  ro_grad->resolution, TRUE /* rollout */,
                  ro_grad->name) != ERR_NO_ERROR)
    abort_message("Problem writing shape %s (%d points) to disk",
                  ro_grad->name,(int) ro_grad->numPoints);

  if (writeToDisk(nav_grad->dataPoints, nav_grad->numPoints, 0, 
                  nav_grad->resolution, TRUE /* rollout */,
                  nav_grad->name) != ERR_NO_ERROR)
    abort_message("Problem writing shape %s (%d points) to disk",
                  nav_grad->name,(int) nav_grad->numPoints);

  if (writeToDisk(pe_grad->dataPoints, pe_grad->numPoints, 0, 
                  pe_grad->resolution, TRUE /* rollout */, 
                  pe_grad->name) != ERR_NO_ERROR)
    abort_message("Problem writing shape %s (%d points) to disk",
                  pe_grad->name,(int) pe_grad->numPoints);
 

  /********************************************************************************/
  /* Create EPI tables */
  /********************************************************************************/
  /* Generates two tables:  */ 
  /* t1 that specifies the k-space ordering, used by recon_all */
  /* t2 that specifies which direction in k-space to go in a given shot */
  /*     1 = positive blips, and -1 = negative blips */
  if ((epi_grad->table1 = (int *)malloc(pe_grad->steps*sizeof(int))) == NULL) 
    abort_message("%s: Problem allocating memory for EPI table1",seqfil);
  if ((epi_grad->table2 = (int *)malloc(nseg*sizeof(int))) == NULL) 
    abort_message("%s: Problem allocating memory for EPI table2",seqfil);
  
  switch(ky_order[0]) {
    case 'l':
      inx = 0;
      for (seg = 0; seg < nseg; seg++) {
        epi_grad->table1[inx++] = -fract_ky + seg;
        for (n = 0; n < epi_grad->etl-1; n++) {
          epi_grad->table1[inx] = (int) (epi_grad->table1[inx-1]+nseg);
          inx++;
        }
      }
      for (seg = 0; seg < nseg; seg++) 
        epi_grad->table2[seg] = 1;
      break;
 
    case 'c':
      inx = 0;
      for (seg = 0; seg < nseg; seg++) {
        epi_grad->table1[inx++] = -(nseg/2 - seg);
        for (n = 0; n < epi_grad->etl-1; n++) {
          if (epi_grad->table1[inx-1] >= 0)
            epi_grad->table1[inx] = (int) (epi_grad->table1[inx-1] + nseg/2);
          else
            epi_grad->table1[inx] = (int) (epi_grad->table1[inx-1] - nseg/2);
          inx++;
        }
      }
      for (seg = 0; seg < nseg; seg++)
        epi_grad->table2[seg] = ((nseg/2 - 1 - seg >= 0) ? -1 : 1);
      break;

      default:  /* This should have been caught earlier */
        break;
  }
  steps = inx;

  /* Print table t1 to file in tablib */
  switch(ky_order[0]) {
    case 'l': sprintf(order_str,"lin"); break;
    case 'c': sprintf(order_str,"cen"); break;
    default:  
      abort_message("%s: ky_order %s not recognized, use 'l' (linear) or 'c' (centric)\n",
                     seqfil,ky_order);
  } 

  if (ky_order[1] == 'r')  /* not reversed */
    sprintf(gpe_tab,"%s_nv%d_f%d_%s%d_rev",seqfil,
       (int)pe_grad->steps,(int)fract_ky,order_str,(int)nseg);
  else
    sprintf(gpe_tab,"%s_nv%d_f%d_%s%d",seqfil,
       (int)pe_grad->steps,(int)fract_ky,order_str,(int)nseg);
  sprintf(tab_file,"%s/tablib/%s",userdir,gpe_tab);

  if ((fp = fopen(tab_file,"w")) == NULL) {
    abort_message("Error opening file %s\n",gpe_tab);
  }

  fprintf(fp,"t1 = ");
  for (inx = 0; inx < steps; inx++) {
    if (ky_order[1] == 'r')
      fprintf(fp,"%d ", epi_grad->table1[inx]);
    else
      fprintf(fp,"%d ", -epi_grad->table1[inx]);
  }
  fprintf(fp,"\n"); 
  fclose(fp);
  strcpy(petable,gpe_tab);
  putstring("petable",gpe_tab);

  /* Return values in VnmrJ parameter pe_table */
  putCmd("exists('pe_table','parameter'):$ex\n");
  putCmd("if ($ex > 0) then\n");
  putCmd("  pe_table = 0\n"); //reset pe_table array
  for (inx = 0; inx < steps; inx++) {
    putCmd("  pe_table[%d] = %d\n",inx+1,
      ((ky_order[1] == 'r') ? 1 : -1)*epi_grad->table1[inx]);
  }
  putCmd("endif\n");
}




/***********************************************************************
*  Function Name: displayEPI
*  Example:    displayEPI(&epi_grad);
*  Purpose:    Displays the values of an EPI_GRADIENT_T structure.
*  Input
*     Formal:  &epi_grad - pointer to EPI_GRADIENT_T structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void displayEPI(EPI_GRADIENT_T *epi_grad){
  printf("\n----------------- EPI GRADIENT --------------------\n");
  printf("=== Options: ===\n");
  printf("fract_ky:      %.0f\n",                 fract_ky);
  printf("nseg,etl,echo: %.0f, %.0f, %.0f\n",     nseg,epi_grad->etl,epi_grad->center_echo);
  printf("navigator:     %s\n",                   navigator);
  printf("rampsamp:      '%s'\n",                 rampsamp);
  printf("ky_order:      '%s'\n",                 ky_order);
  printf("tblip:         %.2fus\n",               epi_grad->tblip*1e6);
  
  printf("=== Calculated ===\n");
  printf("np ramp,flat:  %.0f, %.0f\n",           epi_grad->np_ramp, epi_grad->np_flat);
  printf("skip:          %.2fus\n",               epi_grad->skip*1e6);
  printf("RO amps:       %.3f, %.3fG/cm\n",       epi_grad->amppos,epi_grad->ampneg);
  printf("PE amp:        %.3fG/cm\n",             epi_grad->amppe);
  printf("Total duration %.2fms\n",               epi_grad->duration*1e3);
  printf("Waveform pts   %d\n",                   epi_grad->numPoints);
  printf("petable:       '%s'\n",                 petable);

  printf("=== Tweakers: ===\n");
  printf("groa, grora:   %.3f, %.3f\n",           groa,    grora);
  printf("tep:           %.3f us\n",              tep*1e6);
  printf("ssepi:         %.0f\n",                 ssepi);
 
  printf("Errors:        %.0f\n",                 epi_grad->error);
  printf("----------------------------------------------------\n");

}
