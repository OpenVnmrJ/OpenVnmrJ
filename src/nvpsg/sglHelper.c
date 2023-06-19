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
*
***************************************************************************/

#include "sglCommon.h"
#include "sglHelper.h"

extern int checkflag;

/*********************************************************************
   Resolution limiting function for tr and te 

   This function limits the resolution of the incoming quantity to TRES
   as defined in sgl.h
**********************************************************************/
double limitResolution(double in)
{
   double tmp, out;

   tmp = in / TRES;
   if (fabs(tmp - floor(tmp + 0.5)) > (PRECISION / TRES))
   {
      out = TRES * ceil(tmp);
   }
   else
   {
      out = TRES * floor(tmp + 0.5);
   }
   return(out);
}


/***********************************************************************
*  Function Name: create_skiptab
*  Example:    create_skiptab(nv,nv2,skiptabvals);
*  Purpose:    create and store table of kspace skip values for elliptical kspace acquisition
*  Input
*    nv	            - number of PE steps in first phase encode dimension
*    nv2            - number of PE steps in first second encode dimension
*    skiptabvals    - pointer to array of skip values
***********************************************************************/
int create_skiptab(double nvval,double nv2val,int *skiptabvals)
{
  int i,j,nvproduct,factor=1,index=0,count=0,product;
  double a,b,x,y;
  int *tabpntr;
  double *skipint;                               // double for compatibility with putarray
  unsigned int val,newval;

  // Equation of an ellipse x^2/a^2 + y^2/b^2 = 1
  // y = b*sqrt(1 - x*x/a/a)

  nvproduct = nvval*nv2val;
  a = (int)nvval/2;                              // doesn't handle odd values of nv or nv2
  b = (int)nv2val/2;
  for (j=-b; j<b; j++) {                         // traverse from -nv2/2 to nv2/2
    for (i=-a; i<a; i++) {                       // traverse from -nv/2 to nv/2
      x = (double)i;
      y = b*sqrt(1 - x*x/a/a);
      if ((double)abs(j) <= 0.95*y) {            // 5% tolerance to push out sampled areas to edges
        skiptabvals[index] = 1;                  // 1 means sample that kspace line at this point in the matrix
        count++;
      }
      else {
        skiptabvals[index] = 0;                  // 0 means skip this kspace line
      }
      //printf("%i ",skiptabvals[index]);        // diagnostic print of values to text window
      index++;                                   // index tracks the total number of kspace points completed in the loops
    }
    //printf("  \n");
  }
  //printf("count: %i   Fraction = %4.2f\n",count,(double)count/nvval/nv2val);
  putvalue("nf",count);                          // nf is the number of 1s found

  /*  Find prime factors of nf to set nfmod **********************/
  product = 1;
  for (i=2; i<=count; i++) {                     // 2 is first prime number to look for
    if (count % i == 0) {                        // when i is a factor of count
      factor = i;
      count = count/factor;                      // divide by this factor
      product *= factor;                         // product of all factors so far

      /** if an individual factor is in the range of nv or nv2, use that value **/
      if ((factor >= (int)nvval) || (factor >= (int)nv2val)) {
        putvalue("nfmod",factor);
        break;
      }
      /** otherwise, use the product of factors that reaches nv or nv2 **/
      if ((product >= (int)nvval) || (product >= (int)nv2val)) {
        putvalue("nfmod",product);
        break;
      }
      if (count == 1) {                          // no prime factors other than 1 and count
        putvalue("nfmod",factor);                // if we get to this point, use the last factor, which should be nf
        break;
      }
      i--;                                       // check for this value again before incrementing
    }
  }

  // Next take the array of 1s and 0s in skiptabvals and store in 32 bit words to reduce the
  // number of values written back to procpar by a factor of 32.  Each successive skiptabval is
  // left shifted into the next bit in skipint.  skipint is declared as double for compatibility
  // with putarray.  val and newval are declared as unsigned to avoid binary two's complement, 
  // and writing out negative decimal values to procpar.

  skipint = (double *) malloc(nvval*nv2val);     // overload skipint by bit compression, so don't need to multiply by size
  if (skipint == NULL) {
    text_error("Error in create_skiptable. memory allocation failed\n");
    psg_abort(1);
  }

  tabpntr = skiptabvals;                         // start at first skiptabvals array position
  count = 0;                                     // count tracks the number of compressed skipint values to write out at the end
  while ((count+1)*32 <= nvproduct) {
    newval = 0;
    for (j=0; j<32; j++) {
      val = (unsigned int) (*tabpntr++) ;
      if (val)
        newval |= (val << j);                    // left shift this skiptabval and bit-wise OR with newval
    }
    skipint[count] = (double)newval;
    //printf("%f  ",skipint[count]);
    count++;                                     // increment count to give total number of array values for putarray
  }
  if (nvproduct % 32) {                          // if nvproduct isn't a multiple of 32, include the leftover elements in last skipint word
    newval = 0;
    for (j=0; j<(nvproduct % 32); j++) {         // loop through number of values equal to remainder beyond 32
      val = (unsigned int) (*tabpntr++) ;
      if (val)
        newval |= (val << j);
    }
    skipint[count] = (double)newval;
    count++;                                     // add one more if there is a remainder word
  }
  //printf("\ni %d    count %d\n",i,count);
  putarray("skipint",skipint,count);             // write skipint array back to procpar
  return(0);
}





/***********************************************************************
*  Function Name: prep_profile
*  Example:    prep_profile(profile,nv,&pe_grad,&per_grad);
*  Purpose:    Set up parameters to acquire profile
*  Input
*     Formal:  profile		- flag to acquire profile or full data ('y' or 'n')
*              nv       	- number of points in the phase encoding dimension for full data
*              &pe_grad 	- pointer to phase encoding gradient struct
*              &per_grad	- pointer to phase rewinder gradient struct
*     Private: none	
*     Public:  none
*  Output
*     Return:  Number of phase encoding steps, 1 (profile) or nv/nv2, double
*     Formal:  
*     Private: none
*     Public:  none
*
*  Notes:      
***********************************************************************/
double prep_profile(char profilechar, double steps, 
                    PHASE_ENCODE_GRADIENT_T *pe,
                    PHASE_ENCODE_GRADIENT_T *per)
		{
  double pe_steps;

  if (profilechar == 'y') {
    pe_steps       = 1;
    pe->amp        = 0;
    pe->peamp      = 0;
    pe->increment  = 0;
    per->amp       = 0;
    per->peamp     = 0;
    per->increment = 0;
  }
  else if (profilechar == 'r') {
    pe_steps       = steps;
    pe->amp        = 0;
    pe->peamp      = 0;
    pe->increment  = 0;
    per->amp       = 0;
    per->peamp     = 0;
    per->increment = 0;
  }
  else {
    pe_steps = steps;
  } 
  return(pe_steps);
}


/***********************************************************************
*  Function Name: init_mri
*  Example:    init_mri();
*  Purpose:    Any initialization functions required at start of imaging sequences
               grad_advance
               Retrieve sequence and pre-pulse parameters.
*  Input
*     Formal:  none
*     Private: none
*     Public:  all
*
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void init_mri()
{   
    get_parameters();

    /* set grad_advance for first array, first acquired transient */
    if (ix==1) { ifzero(rtonce); grad_advance(gpropdelay); endif(rtonce); }
//    sub(ssval,ssctr,spare1rt);
//    add(spare1rt,ct,spare1rt);
//    if (ix == 1) { ifzero(spare1rt); grad_advance(gpropdelay); endif(spare1rt); }

    if( FP_EQ(trampfixed, 0.0) ) {    
      /* check for oblique orientation, increase ramp time by sqrt(3) if oblique */
      euler_test();
    }

    /* set images (1 if image/images doesn't exist or image is arrayed, images if image not arrayed) */
    set_images();
}

/***********************************************************************
*  Function Name: get_parameters
*  Example:    get_parameters();
*  Purpose:    Retrieve sequence and pre-pulse parameters.
*  Input
*     Formal:  none
*     Private: none
*     Public:  all
*
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void get_parameters()
{   
   /**************************************/
   /* Load VnmrJ paramters into sequence */
    retrieve_parameters();

   /* Calculate mean nt and mean tr of arrayed values */
    calc_mean_nt_tr();

   /***************************************/
   /* init gradients **********************/
    if (ix == 1) {
      init_structures();
    }			
}


/***********************************************************************
*  Function Name: euler_test
*  Example:    euler_test();
*  Purpose:    check the three euler angles; if it's oblique, increase trise
*  Input
*     Formal:  none
*     Private: none
*     Public:  all
*
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void euler_test()
{   
    double  psiarray[MAXSLICE],phiarray[MAXSLICE],thetaarray[MAXSLICE];
    int     i,npsi,nphi,ntheta,eulertest=0;

    /* Check for non-cardinal gradient rotation Euler angles ******/
    npsi = getarray("psi",psiarray);
    nphi = getarray("phi",phiarray);
    ntheta = getarray("theta",thetaarray);
    for (i=0; i<npsi; i++) {
      if (remainder(psiarray[i],90.0)) eulertest++;
    }
    for (i=0; i<nphi; i++) {
      if (remainder(phiarray[i],90.0)) eulertest++;
    }
    for (i=0; i<ntheta; i++) {
      if (remainder(thetaarray[i],90.0)) eulertest++;
    }
    /* If any angles are not multiples of 90, increase trise ******/
    if (eulertest) {
      trise = trise*sqrt(3.0);
      trisesqrt3=TRUE;
    }
}

/***********************************************************************
*  Function Name: set_images
*  Example:    set_images();
*  Purpose:    Set images appropriately for calculating exp time in g_setExpTime:
*              images = 1 if images doesn't exist.
*              images = 1 if image is arrayed
*              images = images if image is not arrayed
*
***********************************************************************/
void set_images()
{
  /* If images doesn't exist it's value is 0, make it 1 */
  if (FP_EQ(images,0.0)) images=1;
  /* If image is arrayed set images to 1 */
  if (!P_getVarInfo(CURRENT,"image",&dvarinfo))
    if (dvarinfo.size > 1) images=1;
  /* Otherwise leave it at the value that has been read in */
}

/***********************************************************************
*  Function Name: pdd_preacq
*  Example:    pdd_preacq();
*  Purpose:    Prescribes TTL signal for PDD switching for acquisition
*              as required by RF coil and PDD configuration
*              Current parameter volumercv='y': acquisition with volume coil
*              Current parameter volumercv='n': acquisition with array coils
*              Global parameter PDDacquire='y': PDD switches about acquisition
*              Global parameter PDDacquire='n': PDD switches about RF transmit pulses
*
***********************************************************************/
void pdd_preacq()
{
  char PDDacquireStr[MAXSTR], volumercvStr[MAXSTR];  

  /* Get value of PDDacquire */
  getStringSetDefault(GLOBAL,"PDDacquire",PDDacquireStr,"u");   /* undefined if PDDacquire doesn't exist */
  /* Get value of volumercv */
  getStringSetDefault(CURRENT,"volumercv",volumercvStr,"y");
  /* Prescribe TTL signal accordingly */
  if (PDDacquireStr[0] == 'y' && volumercvStr[0] == 'n') sp3on(); /* detune transmit, tune receive coils */
  if (PDDacquireStr[0] == 'n' && volumercvStr[0] == 'y') sp3on(); /* tune transmit, detune receive coils */
}

/***********************************************************************
*  Function Name: pdd_postacq
*  Example:    pdd_postacq();
*  Purpose:    Prescribes TTL signal for PDD switching after acquisition
*              as required by RF coil and PDD configuration
*              Current parameter volumercv='y': acquisition with volume coil
*              Current parameter volumercv='n': acquisition with array coils
*              Global parameter PDDacquire='y': PDD switches about acquisition
*              Global parameter PDDacquire='n': PDD switches about RF transmit pulses
*
***********************************************************************/
void pdd_postacq()
{
  char PDDacquireStr[MAXSTR], volumercvStr[MAXSTR];  

  /* Get value of PDDacquire */
  getStringSetDefault(GLOBAL,"PDDacquire",PDDacquireStr,"u");   /* undefined if PDDacquire doesn't exist */
  /* Get value of volumercv */
  getStringSetDefault(CURRENT,"volumercv",volumercvStr,"y");
  /* Prescribe TTL signal accordingly */
  if (PDDacquireStr[0] == 'y' && volumercvStr[0] == 'n') sp3off(); /* tune transmit, detune receive coils */
  if (PDDacquireStr[0] == 'n' && volumercvStr[0] == 'y') sp3off(); /* detune transmit, tune receive coils */
}

/***********************************************************************
*  Function Name: create_diffusion
*  Example:    create_diffusion(DIFF_SE);
*  Purpose:    Creates a standard diffusion gradient using standard pars
*              according to the specified diffusion type (GE, SE or STE).
*
***********************************************************************/
void create_diffusion(int difftype)
{
  if (diff[0] == 'y') {
    init_diffusion(&diffusion,&diff_grad,"diff",gdiff,tdelta);
    set_diffusion(&diffusion,taudiff,tDELTA,te,minte[0]);
    switch (difftype) {
      case DIFF_GE:
        calc_diffTime(&diffusion,&temin);
        break;
      case DIFF_SE:
        set_diffusion_se(&diffusion,tau1,tau2);
        calc_diffTime(&diffusion,&temin);
        break;
      case DIFF_STE:
        set_diffusion_se(&diffusion,tau1,tau2);
        set_diffusion_tm(&diffusion,tm,mintm[0]);
        calc_diffTime_tm(&diffusion,&temin,&tmmin);
        break;
      default:
        break;
    }
  }
}

/***********************************************************************
*  Function Name: init_diffusion
*  Example:    init_diffusion(&diffusion,&diff_grad,"diff",gdiff,tdelta);
*  Purpose:    Initializes and calculates a generic diffusion gradient
*              with the name, amplitude and duration as specified in the 
*              function arguments.
*
***********************************************************************/
void init_diffusion(DIFFUSION_T *diffusion,GENERIC_GRADIENT_T *grad,char *name,double amp,double delta)
{
  diffusion->type = DIFF_NULL; /* no diffusion type is selected at this point */
  init_generic(grad,name,amp,delta);
  grad->maxGrad = gmax;
  calc_generic(grad,NOWRITE,"","");
  /* adjust duration, so tdelta is from start ramp up to start ramp down */
  if ((ix == 1) || sglarray) {
    grad->duration += grad->tramp; 
    calc_generic(grad,WRITE,"","");
  }
  /* set diffusion structure members */
  diffusion->grad = grad;
  diffusion->gdiff = amp;
  diffusion->delta = delta;
}

/***********************************************************************
*  Function Name: set_diffusion
*  Example:    set_diffusion(&diffusion,taudiff,tDELTA,te,minte[0]);
*  Purpose:    Sets diffusion structure with taudiff (the additional diffusion 
*              encoding time), DELTA, echo time and flag for minimum echo time.
*              Aborts if initial type of diffusion is not DIFF_NULL
*              (i.e. init_diffusion must be called first).
*
***********************************************************************/
void set_diffusion(DIFFUSION_T *diffusion,double diffadd,double DELTA,double techo,char mintechoflag)
{
  if (diffusion->type > DIFF_NULL)
    abort_message("%s: init_diffusion must be called before set_diffusion\n",seqfil);
  diffusion->tadd = diffadd;
  diffusion->DELTA = DELTA;
  diffusion->te = techo;
  diffusion->minte = mintechoflag;
  diffusion->type = DIFF_GE; /* we are now set up for gradient echo diffusion */
}

/***********************************************************************
*  Function Name: set_diffusion_se
*  Example:    set_diffusion_se(&diffusion,tau1,tau2);
*  Purpose:    Sets diffusion structure members with the durations of events
*              in the first and second halves of a spin echo respectively.
*              Aborts if initial type of diffusion is not DIFF_GE
*              (i.e. set_diffusion must be called first).
*
***********************************************************************/
void set_diffusion_se(DIFFUSION_T *diffusion,double decho1,double decho2)
{
  if (diffusion->type != DIFF_GE)
    abort_message("%s: set_diffusion must be called before set_diffusion_se\n",seqfil);
  diffusion->tau1 = decho1;
  diffusion->tau2 = decho2;
  diffusion->type = DIFF_SE; /* we are now set up for spin echo diffusion */
}

/***********************************************************************
*  Function Name: set_diffusion_tm
*  Example:    set_diffusion_tm(&diffusion,tm,mintm[0]);
*  Purpose:    Sets diffusion structure members with the mixing time
*              and flag for minimum mixing time.
*              Aborts if initial type of diffusion is not DIFF_SE
*              (i.e. set_diffusion_se must be called first).
*
***********************************************************************/
void set_diffusion_tm(DIFFUSION_T *diffusion,double tmix,char mintmixflag)
{
  if (diffusion->type != DIFF_SE)
    abort_message("%s: set_diffusion_se must be called before set_diffusion_tm\n",seqfil);
  diffusion->tm = tmix;
  diffusion->mintm = mintmixflag;
  diffusion->type = DIFF_STE; /* we are now set up for stimulated echo diffusion */
}

/***********************************************************************
*  Function Name: calc_diffTime
*  Example:    calc_diffTime(&diffusion,&temin);
*  Purpose:    Calculates the minimum echo time including diffusion components
*              for gradient echo (GE) and spin echo (SE) diffusion encoding.
*              diffusion structure members are used for the calculations.
*              diffusion structure members are set using the init_diffusion,
*              set_diffusion and (optionally) set_diffusion_se functions.
*              diffusion structure delays d1->d4 are calculated according to
*              the scheme [d1 - Gdiff - d2] - taudiff - [d3 - Gdiff - d4] where
*              the first [...] corresponds to the diffusion dephase module and
*              the second [...] corresponds to the diffusion rephase module.
*
***********************************************************************/
void calc_diffTime(DIFFUSION_T *diffusion,double *temin)
{
  double minDELTA,maxDELTA,teminadd;

  if (diffusion->type == DIFF_NULL)
    abort_message("%s: calc_diffTime requires (at least) init_diffusion and set_diffusion\n",seqfil);
  if (diffusion->type == DIFF_STE)
    abort_message("%s: calc_diffTime can not be used for stimulated echo, use calc_diffTime_tm\n",seqfil);

  /* granulate DELTA */
  diffusion->DELTA = granularity(diffusion->DELTA,GRADIENT_RES);

  /* minimum DELTA */
  minDELTA = diffusion->grad->duration + diffusion->tadd + 2*GRADIENT_RES;;

  /* check DELTA is not too short */
  if (FP_LT(diffusion->DELTA,minDELTA))    
    abort_message("%s: DELTA is too short, minimum is %.3f ms\n",seqfil,minDELTA*1000);

  /* maximum DELTA with temin adjusted only to include the additional diffusion encoding gradients */
  maxDELTA = minDELTA;
  if (diffusion->type == DIFF_SE) maxDELTA += fabs(diffusion->tau1-diffusion->tau2);

  /* if DELTA < maxDELTA then only the additional diffusion encoding gradients 
     are required, so set maxDELTA=DELTA */
  if (FP_LT(diffusion->DELTA,maxDELTA)) maxDELTA=diffusion->DELTA;

  /* teminadd is the time to add to temin */
  teminadd = 2*diffusion->grad->duration+diffusion->DELTA-maxDELTA;

  /* for gradient echo there is already a minimum delay of GRADIENT_RES */
  /* for diffusion there are three additional delays each with minimum GRADIENT_RES */
  if (diffusion->type == DIFF_GE) teminadd += 3*GRADIENT_RES;

  /* for spin echo each half-echo should be properly granulated */
  /* there is already a minimum delay of GRADIENT_RES in each half-echo */
  /* for diffusion there are two additional delays each with minimum GRADIENT_RES */
  if (diffusion->type == DIFF_SE) 
    teminadd += granularity(*temin+teminadd,2*GRADIENT_RES)-*temin-teminadd+2*GRADIENT_RES;

  /* update minimum echo time */
  *temin += teminadd;

  /* if minimum echo time is selected then use that echo time */
  if (diffusion->minte == 'y') diffusion->te = *temin;

  /* recalculate maxDELTA for the new minimum echo time */
  maxDELTA = diffusion->DELTA + diffusion->te - *temin;

  /* if min echo time is not selected check that the echo time is valid */
  if ((diffusion->minte != 'y') && FP_LT(diffusion->te,*temin)) {
    if (maxDELTA>minDELTA)
      abort_message("%s: TE too short, increase to %.3f ms or reduce DELTA to %.3f ms\n",
        seqfil,*temin*1000,maxDELTA*1000);
    else
      abort_message("%s: TE too short, increase to %.3f ms\n",seqfil,*temin*1000);
  }

  /* set up the d* delays around the diffusion gradients.
     the scheme is [d1 - Gdiff - d2] - tadd - [d3 - Gdiff - d4].
     the minimum d* delays are GRADIENT_RES */
  switch (diffusion->type) {
    case DIFF_GE: /* gradient echo, diffusion gradients a bipolar pair */
      diffusion->d1 = GRADIENT_RES; 
      diffusion->d2 = diffusion->DELTA-minDELTA+GRADIENT_RES; 
      diffusion->d3 = GRADIENT_RES; 
      diffusion->d4 = diffusion->te-*temin+GRADIENT_RES;
      break;
    case DIFF_SE: /* spin echo */
      /* start with the 2nd diffusion gradient right after the 'taudiff' components */
      diffusion->d2 = diffusion->DELTA-minDELTA+GRADIENT_RES; 
      diffusion->d3 = GRADIENT_RES;
      diffusion->d4 = diffusion->te/2.0-diffusion->tau2-diffusion->grad->duration;
      diffusion->d1 = diffusion->te/2.0-diffusion->tau1-diffusion->grad->duration-diffusion->d2+GRADIENT_RES; 
      /* if d1 is negative diffusion gradients must be shifted later in time */
      if (FP_LT(diffusion->d1,GRADIENT_RES)) { 
        diffusion->d2 += diffusion->d1-GRADIENT_RES; 
        diffusion->d3 += -diffusion->d1+GRADIENT_RES;
        diffusion->d4 += diffusion->d1-GRADIENT_RES; 
        diffusion->d1 = GRADIENT_RES;
      }
      break;
    default:
      break;
  }

  /* calculate the duration of all diffusion components */
  diffusion->Time = 2*diffusion->grad->duration+diffusion->d1+diffusion->d2+diffusion->d3+diffusion->d4;

}

/***********************************************************************
*  Function Name: calc_diffTime_tm
*  Example:    calc_diffTime(&diffusion,&temin,&tmmin);
*  Purpose:    Calculates the minimum echo time including diffusion components
*              and minimum mixing time for stimulated echo (STE) diffusion encoding.
*              diffusion structure members are used for the calculations.
*              diffusion structure members are set using the init_diffusion,
*              set_diffusion, set_diffusion_se and set_diffusion_tm functions.
*              diffusion structure delays d1->d4 are calculated according to
*              the scheme [d1 - Gdiff - d2] - taudiff - dm - [d3 - Gdiff - d4] where
*              the first [...] corresponds to the diffusion dephase module and
*              the second [...] corresponds to the diffusion rephase module.
*              diffusion structure mixing delay dm is also calculated.
*
***********************************************************************/
void calc_diffTime_tm(DIFFUSION_T *diffusion,double *temin,double *tmmin)
{
  double minDELTA,maxDELTA,teminadd;

  if (diffusion->type < DIFF_STE)
    abort_message("%s: calc_diffTime_tm requires init_diffusion, set_diffusion, set_diffusion_se and set_diffusion_tm\n",seqfil);

  /* granulate DELTA */
  diffusion->DELTA = granularity(diffusion->DELTA,GRADIENT_RES);

  /* minimum DELTA */
  minDELTA = diffusion->grad->duration + diffusion->tadd + 2*GRADIENT_RES;

  /* check DELTA is not too short */
  if (FP_LT(diffusion->DELTA,minDELTA))    
    abort_message("%s: DELTA is too short, minimum is %.3f ms\n",seqfil,minDELTA*1000);

  /* teminadd is the time to add to temin */
  /* there is already a minimum delay of GRADIENT_RES in each half-echo */
  /* for diffusion there are two additional delays each with minimum GRADIENT_RES */
  teminadd = 2*diffusion->grad->duration+2*GRADIENT_RES;

  /* update minimum echo time */
  *temin += teminadd;

  /* if minimum echo time is selected then use that echo time */
  if (diffusion->minte == 'y') diffusion->te = *temin;

  /* if min echo time is not selected check that the echo time is valid */
  if ((diffusion->minte != 'y') && FP_LT(diffusion->te,*temin))
    abort_message("%s: TE too short, increase to %.3f ms\n",seqfil,*temin*1000);

  /* maximum DELTA with temin adjusted only to include the additional diffusion encoding gradients */
  maxDELTA = minDELTA + fabs(diffusion->tau1-diffusion->tau2);

  /* maximum DELTA with the specified te */
  maxDELTA += diffusion->te-*temin;

  /* check if minimum tm has been selected */
  if (diffusion->mintm == 'y') {
    /* minimum mixing delay */
    diffusion->dm = 0.0;
    /* if DELTA is greater than maxDELTA mixing delay must be adjusted */
    if (diffusion->DELTA>maxDELTA) diffusion->dm += diffusion->DELTA-maxDELTA;
    /* update the minimum tm, minimum DELTA and maximum DELTA */
    *tmmin += diffusion->dm;
    minDELTA += diffusion->dm;
    maxDELTA += diffusion->dm;
  } else { /* minimum tm not selected */
    /* calculate mixing delay */
    diffusion->dm = diffusion->tm-*tmmin;
    if (FP_LT(diffusion->dm,0.0))
      abort_message("%s: TM too short, increase to %.3f ms\n",seqfil,*tmmin*1000);
    /* update the minimum DELTA and maximum DELTA */
    minDELTA += diffusion->dm;
    maxDELTA += diffusion->dm;
    if (FP_GT(diffusion->DELTA,maxDELTA))
      abort_message("%s: TM too short, increase to %.3f ms or reduce DELTA to %.3f ms",
        seqfil,(diffusion->tm+diffusion->DELTA-maxDELTA)*1000,maxDELTA*1000);
    if (FP_LT(diffusion->DELTA,minDELTA))
      abort_message("%s: DELTA too short, increase to %.3f ms or reduce TM to %.3f ms",
        seqfil,minDELTA*1000,(diffusion->tm+diffusion->DELTA-minDELTA)*1000);
  }

  /* a minimum delay of GRADIENT_RES is assumed to be already accounted for, so it must be added */
  diffusion->dm += GRADIENT_RES;

  /* set up the d* delays around the diffusion gradients.
     the scheme is [d1 - Gdiff - d2] - tadd - [d3 - Gdiff - d4].
     the minimum d* delays are GRADIENT_RES
     start with the 2nd diffusion gradient right after the 'taudiff' components */
  diffusion->d2 = diffusion->DELTA-minDELTA+GRADIENT_RES; 
  diffusion->d3 = GRADIENT_RES;
  diffusion->d4 = diffusion->te/2.0-diffusion->tau2-diffusion->grad->duration;
  diffusion->d1 = diffusion->te/2.0-diffusion->tau1-diffusion->grad->duration-diffusion->d2+GRADIENT_RES; 
  /* if d1 is negative diffusion gradients must be shifted later in time */
  if (FP_LT(diffusion->d1,GRADIENT_RES)) { 
    diffusion->d2 += diffusion->d1-GRADIENT_RES; 
    diffusion->d3 += -diffusion->d1+GRADIENT_RES;
    diffusion->d4 += diffusion->d1-GRADIENT_RES; 
    diffusion->d1 = GRADIENT_RES;
  }

  /* calculate the duration of all diffusion components */
  diffusion->Time = 2*diffusion->grad->duration+diffusion->d1+diffusion->d2+diffusion->d3+diffusion->d4+diffusion->dm;

}

/***********************************************************************
*  Function Name: calc_bvalues
*  Example:    calc_bvalues(&diffusion,"dro","dpe","dsl")
*  Purpose:    Calculates b-values according to the main diffusion gradients
*
***********************************************************************/
void calc_bvalues(DIFFUSION_T *diffusion,char *ropar,char *pepar,char *slpar)
{
  int i;
  
  /* get and check the size of the diffusion gradient scale arrays (usually dro,dpe,dsl) */
  diffusion->nbro = P_getsize(CURRENT,ropar,NULL);
  diffusion->nbval = diffusion->nbro;
  diffusion->nbpe = P_getsize(CURRENT,pepar,NULL);
  if (diffusion->nbpe > diffusion->nbval) diffusion->nbval = diffusion->nbpe;
  diffusion->nbsl = P_getsize(CURRENT,slpar,NULL);
  if (diffusion->nbsl > diffusion->nbval) diffusion->nbval = diffusion->nbsl;
  if ((diffusion->nbro != diffusion->nbval) && (diffusion->nbro != 1))
    abort_message("%s: Number of directions/b-values must be the same for all axes (readout)",seqfil);
  if ((diffusion->nbpe != diffusion->nbval) && (diffusion->nbpe != 1))
    abort_message("%s: Number of directions/b-values must be the same for all axes (phase)",seqfil);
  if ((diffusion->nbsl != diffusion->nbval) && (diffusion->nbsl != 1))
    abort_message("%s: Number of directions/b-values must be the same for all axes (slice)",seqfil);
  if ((diffusion->dro=(double *)malloc(diffusion->nbro*sizeof(double))) == NULL) nomem();
  if ((diffusion->dpe=(double *)malloc(diffusion->nbpe*sizeof(double))) == NULL) nomem();
  if ((diffusion->dsl=(double *)malloc(diffusion->nbsl*sizeof(double))) == NULL) nomem();
  S_getarray(ropar,diffusion->dro,diffusion->nbro*sizeof(double)); /* NB S_getarray returns # elements */
  S_getarray(pepar,diffusion->dpe,diffusion->nbpe*sizeof(double));
  S_getarray(slpar,diffusion->dsl,diffusion->nbsl*sizeof(double));
  /* allocate for bvalues */
  if ((diffusion->bro=(double *)malloc(diffusion->nbval*sizeof(double))) == NULL) nomem();
  if ((diffusion->bpe=(double *)malloc(diffusion->nbval*sizeof(double))) == NULL) nomem();
  if ((diffusion->bsl=(double *)malloc(diffusion->nbval*sizeof(double))) == NULL) nomem();
  if ((diffusion->brs=(double *)malloc(diffusion->nbval*sizeof(double))) == NULL) nomem();
  if ((diffusion->brp=(double *)malloc(diffusion->nbval*sizeof(double))) == NULL) nomem();
  if ((diffusion->bsp=(double *)malloc(diffusion->nbval*sizeof(double))) == NULL) nomem();
  if ((diffusion->btrace=(double *)malloc(diffusion->nbval*sizeof(double))) == NULL) nomem();

  /* calculate bvalues according to main diffusion gradients */
  for (i = 0; i < diffusion->nbval; i++)  {
    set_dvalues(diffusion,&droval,&dpeval,&dslval,i);
    /* Readout */
    diffusion->bro[i] = bval(diffusion->gdiff*droval,diffusion->delta,diffusion->DELTA);
    /* Phase */
    diffusion->bpe[i] = bval(diffusion->gdiff*dpeval,diffusion->delta,diffusion->DELTA);
    /* Slice */
    diffusion->bsl[i] = bval(diffusion->gdiff*dslval,diffusion->delta,diffusion->DELTA);
    /* Readout/Slice Cross-terms */
    diffusion->brs[i] = bval2(diffusion->gdiff*droval,diffusion->gdiff*dslval,diffusion->delta,diffusion->DELTA);
    /* Readout/Phase Cross-terms */
    diffusion->brp[i] = bval2(diffusion->gdiff*droval,diffusion->gdiff*dpeval,diffusion->delta,diffusion->DELTA);
    /* Slice/Phase Cross-terms */
    diffusion->bsp[i] = bval2(diffusion->gdiff*dslval,diffusion->gdiff*dpeval,diffusion->delta,diffusion->DELTA);
  }

}

/***********************************************************************
*  Function Name: set_dvalues
*  Example:    set_dvalues(&diffusion,&droval,&dpeval,&dslval)
*  Purpose:    Sets the scaling values droval, dpeval and dslval of diffusion gradients
*              according to the specified index
*
***********************************************************************/
void set_dvalues(DIFFUSION_T *diffusion,double *roscale,double *pescale,double *slscale,int index)
{
  if (diffusion->nbro>1) *roscale=diffusion->dro[index]; else *roscale=diffusion->dro[0];
  if (diffusion->nbpe>1) *pescale=diffusion->dpe[index]; else *pescale=diffusion->dpe[0];
  if (diffusion->nbsl>1) *slscale=diffusion->dsl[index]; else *slscale=diffusion->dsl[0];
}

/***********************************************************************
*  Function Name: write_bvalues
*  Example:    write_bvalues(&diffusion,"bval","bvalue","max_bval")
*  Purpose:    Calculates the trace and maximum value b-value (s/mm2)
*              Writes the calculated values back to the parameter set
*
***********************************************************************/
void write_bvalues(DIFFUSION_T *diffusion,char *basepar,char *tracepar,char *maxpar)
{
  int i;
  char *par;

  /* calculate the trace and maximum value */
  for (i = 0; i < diffusion->nbval; i++)  {
    diffusion->btrace[i] = (diffusion->bro[i]+diffusion->bsl[i]+diffusion->bpe[i]);
    if (diffusion->max_bval < diffusion->btrace[i]) diffusion->max_bval = diffusion->btrace[i];
  }

  /* write the values */
  if (strlen(basepar)>0) {
    if ((par=(char *)malloc((strlen(basepar)+3)*sizeof(char))) == NULL) nomem();
    // BDZ note that all these strcpy/strcat calls are safe
    strcpy(par,basepar); strcat(par,"rr");
    putarray(par,diffusion->bro,diffusion->nbval); 
    strcpy(par,basepar); strcat(par,"pp");
    putarray(par,diffusion->bpe,diffusion->nbval);
    strcpy(par,basepar); strcat(par,"ss");
    putarray(par,diffusion->bsl,diffusion->nbval); 
    strcpy(par,basepar); strcat(par,"rp");
    putarray(par,diffusion->brp,diffusion->nbval);
    strcpy(par,basepar); strcat(par,"rs");
    putarray(par,diffusion->brs,diffusion->nbval);
    strcpy(par,basepar); strcat(par,"sp");
    putarray(par,diffusion->bsp,diffusion->nbval);
    free(par);
  }
  putarray(tracepar,diffusion->btrace,diffusion->nbval); 
  putvalue(maxpar,diffusion->max_bval);

  /* free memory */
  free(diffusion->bro); free(diffusion->bpe); free(diffusion->bsl); 
  free(diffusion->brs); free(diffusion->brp); free(diffusion->bsp);
  free(diffusion->dro); free(diffusion->dpe); free(diffusion->dsl); 
  free(diffusion->btrace);

}

void diffusion_dephase(DIFFUSION_T *diffusion,double roscale,double pescale,double slscale)
{
  obl_shapedgradient(diffusion->grad->name,diffusion->grad->duration,
                     diffusion->grad->amp*roscale,diffusion->grad->amp*pescale,diffusion->grad->amp*slscale,WAIT);
}

void diffusion_rephase(DIFFUSION_T *diffusion,double roscale,double pescale,double slscale)
{
  switch (diffusion->type) {
    case DIFF_GE:
      obl_shapedgradient(diffusion->grad->name,diffusion->grad->duration,
                         -diffusion->grad->amp*roscale,-diffusion->grad->amp*pescale,-diffusion->grad->amp*slscale,WAIT);
      break;
    default:
      obl_shapedgradient(diffusion->grad->name,diffusion->grad->duration,
                         diffusion->grad->amp*roscale,diffusion->grad->amp*pescale,diffusion->grad->amp*slscale,WAIT);
      break;
  }
}

/*----------------------------------------------------------------------*/
/*-------------- B-VALUE CALCULATIONS FOR DIFFUSION WEIGHTED EXPS ------*/
/*----------------------------------------------------------------------*/
/***********************************************************************
*  Function Name: bval
*  Example:    bval(gdiff,tdelta,tDELTA)
*  Purpose:    Calculates b-value (s/mm2) for a single pair of TRAPEZOID diffusion gradients
*              gradient in G/cm and delta/DELTA in s
*
***********************************************************************/
double bval(double g1, double d1, double D1) 
{
  double b;
  b=shapedbval(g1,d1,D1,TRAPEZOID);
  return(b);
}

/***********************************************************************
*  Function Name: shapedbval
*  Example:    shapedbval(gdiff,tdelta,tDELTA,diff_grad.shape)
*  Purpose:    Calculates b-value (s/mm2) for a single pair of shaped diffusion gradients
*              gradient in G/cm and delta/DELTA in s
*
***********************************************************************/
double shapedbval(double g1, double d1, double D1, int shape1) {
  /* Shape is either TRAPEZOID (1) or SINE (2), see sglCommon.h */
  double b,gamma;

  gamma = nuc_gamma()*2*PI;  
  b = gamma*gamma*g1*g1*d1*d1;

  switch (shape1) {
    case TRAPEZOID: b *= (D1-d1/3); break;
    case SINE:      b *= (4/PI/PI*(D1-d1/4)); break;
    default:        abort_message("shapedbval: Unknown shape (%d)",shape1);
  }

  /* b *= 1e4;    (G/cm)^2 -> (T/m)^2 */
  /* b *= 1e-6;   s/m2 -> s/mm2 */
  b *= 1e-2;
  return(b);
}

/***********************************************************************
*  Function Name: bval2
*  Example:    bval2(gdiff*roval,gdiff*peval,tdelta,tDELTA)
*  Purpose:    Calculates b-value (s/mm2) for TRAPEZOID diffusion gradients on two axes simultaneously
*              gradient in G/cm and delta/DELTA in s
*
***********************************************************************/
double bval2(double g1, double g2, double d1, double D1) 
{
  double b;
  b=shapedbval2(g1,g2,d1,D1,TRAPEZOID);
  return(b);
}

/***********************************************************************
*  Function Name: shapedbval2
*  Example:    shapedbval2(gdiff*roval,gdiff*peval,tdelta,tDELTA,diff_grad.shape)
*  Purpose:    Calculates b-value (s/mm2) for shaped diffusion gradients on two axes simultaneously
*              gradient in G/cm and delta/DELTA in s
*
***********************************************************************/
double shapedbval2(double g1, double g2, double d1, double D1, int shape1) 
{
  /* Note: the two pairs of gradients must be the same shape */
  double b,gamma;

  gamma = nuc_gamma()*2*PI; 
  b = gamma*gamma*g1*g2*d1*d1*1e-2;

  switch (shape1) {
    case TRAPEZOID: b *= (D1-d1/3); break;
    case SINE:      b *= (4/PI/PI*(D1-d1/4)); break;
    default:        abort_message("bval2: Unknown shape (%d)",shape1); 
  }

  return(b);
}

/***********************************************************************
*  Function Name: bval_nested
*  Example:    bval_nested(gdiff,tdelta,tDELTA,gcrush,dcrush,Dcrush);
*  Purpose:    Calculates b-value (s/mm2) with two pairs of TRAPEZOID gradients on a single axis
*              Assumes that one pair (inner) is nested within the other (outer)
*              gradient in G/cm and delta/DELTA in s
*
***********************************************************************/
double bval_nested(double g1, double d1, double D1, double g2, double d2, double D2) 
{
  double b;
  b=shapedbval_nested(g1,d1,D1,TRAPEZOID,g2,d2,D2,TRAPEZOID);
  return(b);
}

/***********************************************************************
*  Function Name: shapedbval_nested
*  Example:    shapedbval_nested(gdiff,tdelta,tDELTA,diff_grad.shape,gcrush,dcrush,Dcrush,crush_grad.shape);
*  Purpose:    Calculates b-value (s/mm2) with two pairs of shaped gradients on a single axis
*              Assumes that one pair (inner) is nested within the other (outer)
*              gradient in G/cm and delta/DELTA in s
*
***********************************************************************/
double shapedbval_nested(double g1, double d1, double D1, int shape1, double g2, double d2, double D2, int shape2) 
{
  double b,gamma;
  double inner_Delta;

  inner_Delta = (D1 < D2 ? D1 : D2);
  gamma = nuc_gamma()*2*PI;
  b = 2*gamma*gamma*g1*g2*d1*d2*inner_Delta*1e-2;
  
  switch (shape1) {
    case TRAPEZOID: break;
    case SINE:      b *= (2/PI); break;
    default:        abort_message("bval_nested: Unknown shape1 (%d)",shape1);
  }

  switch (shape2) {
    case TRAPEZOID: break;
    case SINE:      b *= (2/PI); break;
    default:        abort_message("bval_nested: Unknown shape2 (%d)",shape2);
  }
    
  return(b);
}

/***********************************************************************
*  Function Name: bval_cross
*  Example:    bval_cross(gdiff,tdelta,tDELTA,gcrush,dcrush,Dcrush);
*  Purpose:    Calculates b-value (s/mm2) for two pairs of TRAPEZOID gradients on different axes
*              Assumes that one pair (inner) is nested within the other (outer)
*              even though they're on different axes
*              gradient in G/cm and delta/DELTA in s
*
***********************************************************************/
double bval_cross(double g1, double d1, double D1, double g2, double d2, double D2) 
{
  double b;
  b=shapedbval_cross(g1,d1,D1,TRAPEZOID,g2,d2,D2,TRAPEZOID);
  return(b);
}

/***********************************************************************
*  Function Name: shapedbval_cross
*  Example:    shapedbval_cross(gdiff,tdelta,tDELTA,diff_grad.shape,gcrush,dcrush,Dcrush,crush_grad.shape);
*  Purpose:    Calculates b-value (s/mm2) for two pairs of shaped gradients on different axes
*              Assumes that one pair (inner) is nested within the other (outer)
*              even though they're on different axes
*              gradient in G/cm and delta/DELTA in s
*
***********************************************************************/
double shapedbval_cross(double g1, double d1, double D1, int shape1, double g2, double d2, double D2, int shape2) 
{
  double b,gamma;
  double inner_Delta;

  inner_Delta = (D1 < D2 ? D1 : D2);
  gamma = nuc_gamma()*2*PI;
  b = gamma*gamma*g1*g2*d1*d2*inner_Delta*1e-2;

  switch (shape1) {
    case TRAPEZOID: break;
    case SINE:      b *= (2/PI); break;
    default:        abort_message("bval_nested: Unknown shape1 (%d)",shape1);
  }

  switch (shape2) {
    case TRAPEZOID: break;
    case SINE:      b *= (2/PI); break;
    default:        abort_message("bval_nested: Unknown shape2 (%d)",shape2);
  }

  return(b);
}
/*----------------------------------------------------------------------*/

/***********************************************************************
*  Function Name: calc_mean_nt_tr
*  Example:    calc_mean_nt_tr();
*  Purpose:    Calculates mean nt and mean tr of arrayed values
*              (which will be used for experiment duration calculations)
*
***********************************************************************/
void calc_mean_nt_tr()
{
  int nnt,ntr;
  double *ntvals,*trvals;
  int i;

  if (ix > 1) return;

  /* Parse the array string */
  parse_array(&arraypars);

  /* If tr is arrayed get the size of the tr array */
  if (array_check("tr",&arraypars)) ntr=get_nvals("tr",&arraypars);
  else ntr=1;

  /* Calculate the mean */
  if ((trvals=(double *)malloc(ntr*sizeof(double))) == NULL) abort_message("Insufficient memory");
  if (ntr>1) S_getarray("tr",trvals,ntr*sizeof(double));
  else trvals[0]=tr;
  trmean = 0.0;
  for (i=0;i<ntr;i++) trmean += trvals[i];
  trmean /= ntr;

  /* If nt is arrayed get the size of the nt array */
  if (array_check("nt",&arraypars)) nnt=get_nvals("nt",&arraypars);
  else nnt=1;

  /* Get the array */
  if ((ntvals=(double *)malloc(nnt*sizeof(double))) == NULL) abort_message("Insufficient memory");
  if (nnt>1) S_getarray("nt",ntvals,nnt*sizeof(double));
  else ntvals[0]=nt;

  /* Check if nt and tr are arrayed together */
  ntmean = 0.0;
  if ((nnt>1) && (ntr>1) && (get_cycle("nt",&arraypars)==get_cycle("tr",&arraypars))) {
    for (i=0;i<nnt;i++) ntmean += (ntvals[i]*trvals[i]);
    ntmean /= nnt;
    ntmean /= trmean;
  } else {
    for (i=0;i<nnt;i++) ntmean += ntvals[i];
    ntmean /= nnt;
  }

  free(trvals);
  free(ntvals);
}

/***********************************************************************
*  Function Name: parse_array
*  Example:    parse_array(&arraypars);
*  Purpose:    Parses the array string, providing:
*              npars: the number of parameters in the array string
*              par:   the name of parameters in the array string
*              nvals: the number of values of each parameter in the array string
*              cycle: how often each parameter in the array string cycles
*
***********************************************************************/
void parse_array(ARRAYPARS_T *apars)
{
  char array[MAXSTR];
  int i,j,k,n,joint;
  int *cycles;

  getstr("array",array);
  apars->npars=0;            /* Initialise number of pars in array string */
  if (strlen(array)==0) return; /* Return if par is an empty string */
  apars->npars=1;            /* There is at least one parameter in the list */
  /* Count the ,s to see how many more parameters there are */
  for (i=1;i<strlen(array);i++) {
    if (array[i] == ',') apars->npars++;
  }
  /* Malloc the structure */
  if ((apars->par = (char **)malloc(apars->npars*sizeof(char *))) == NULL) abort_message("Insufficient memory");
  for (i=0;i<apars->npars;i++)
    if ((apars->par[i] = (char *)malloc((strlen(array)+1)*sizeof(char))) == NULL) abort_message("Insufficient memory");
  if ((apars->nvals = (int *)malloc(apars->npars*sizeof(int))) == NULL) abort_message("Insufficient memory");
  if ((apars->cycle = (int *)malloc(apars->npars*sizeof(int))) == NULL) abort_message("Insufficient memory");
  /* Initialise apars->cycle */
  for (i=0;i<apars->npars;i++) apars->cycle[i]=0;
  /* Fill apars->par with the arrayed parameters and index apars->cycle */
  joint=0; j=0; k=0;
  for (i=0;i<strlen(array);i++) {
    if (array[i] == '(') { i++; apars->cycle[j]=1; joint=1; }
    if (array[i] == ')') { i++; joint=0; }
    if (array[i] == ',') {
      if (joint) {
        apars->cycle[j]++;
        apars->cycle[j+1]=apars->cycle[j]; /* for a ',' j+1 will always exist */
      }
      apars->par[j++][k]=0; /* NULL terminate */
      k=0;
    }
    else
      apars->par[j][k++]=array[i];
  }
  apars->par[j][k]=0; /* NULL terminate */
  /* Fill apars->nvals with the number of values for each parameter */
  for (i=0;i<apars->npars;i++) {
    apars->nvals[i] = P_getsize(CURRENT,apars->par[i],NULL); 
  }
  /* Allocate for npars 'cycles' */
  if ((cycles = (int *)malloc(apars->npars*sizeof(int))) == NULL) abort_message("Insufficient memory");
  /* Initialise cycles and joint for the last parameter */
  cycles[apars->npars-1]=1;
  if (apars->cycle[apars->npars-1] > 0) joint=1;
  else joint=0;
  /* Figure how each parameter cycles */
  for (i=apars->npars-2;i>=0;i--) {
    if (joint) {
      n=apars->cycle[i+1]-1;
      for (j=0;j<n;j++) {
        cycles[i]=cycles[i+1];
        i--;
      }
      i++; joint=0;
    } else {
      cycles[i]=cycles[i+1]*apars->nvals[i+1];
      if (apars->cycle[i] > 0) joint=1;
    }
  }
  /* Copy cycles back to apars->cycle */
  for (i=0;i<apars->npars;i++) apars->cycle[i]=cycles[i];

  /* Print structure */
/*
  for (i=0;i<apars->npars;i++)
    fprintf(stdout,"par = %s has %d values, cycle = %d\n",apars->par[i],apars->nvals[i],apars->cycle[i]);
  fflush(stdout);
*/
}

/***********************************************************************
*  Function Name: array_check
*  Example:    array_check(par,&arraypars);
*  Purpose:    Checks to see if parameter par is arrayed.
*              Returns 1 if arrayed 0 otherwise. 
*
***********************************************************************/
int array_check(char *par,ARRAYPARS_T *apars)
{
  int i;
  /* Return if there are no arrayed parameters */
  if (apars->npars == 0) return(0);
  for (i=0;i<apars->npars;i++)
    if (!strcmp(apars->par[i],par)) return(1);
  return(0);
}

/***********************************************************************
*  Function Name: get_nvals
*  Example:    get_nvals(par,&arraypars);
*  Purpose:    Returns the number of values of the parameter.
*              Returns nvals if arrayed 0 otherwise.
*
***********************************************************************/
int get_nvals(char *par,ARRAYPARS_T *apars)
{
  int i;
  /* Return if there are no arrayed parameters */
  if (apars->npars == 0) return(0);
  for (i=0;i<apars->npars;i++)
    if (!strcmp(apars->par[i],par)) return(apars->nvals[i]);
  return(0);
}

/***********************************************************************
*  Function Name: get_cycle
*  Example:    get_cycle(par,&arraypars);
*  Purpose:    Returns how often the parameter par cycles.
*              Returns cycle if arrayed 0 otherwise.
*
***********************************************************************/
int get_cycle(char *par,ARRAYPARS_T *apars)
{
  int i;
  /* Return if there are no arrayed parameters */
  if (apars->npars == 0) return(0);
  for (i=0;i<apars->npars;i++)
    if (!strcmp(apars->par[i],par)) return(apars->cycle[i]);
  return(0);
}

/***********************************************************************
*  Function Name: check_nsblock
*  Example:    check_nsblock();
*  Purpose:    Check nsblock, the number of slices blocked together
*              (which will be used for triggering and inversion recovery)
*
***********************************************************************/
void check_nsblock()
{
  if (ticks>0 || ir[0]=='y') { /* if triggering or inversion recovery */
    if (blockslices) {
      /* nsblock has been read from parameters but we must check it is valid */
      if ((seqcon[1] == 'c') && ((int)ns%nsblock > 0))
        abort_message("Number of slices (%d) must be a multiple of %d (blocked slices)\n",(int)ns,nsblock);
    } else {
      /* just do the slices individually */
      nsblock = 1;
    }
  }
}

/***********************************************************************
*  Function Name: init_structures
*  Example:    init_structures();
*  Purpose:    Initializes the gradient and RF structures. The function 
*              is a support for gradient modifications not supported by 
*              the wrapper functions.  This function is required for 
*              for standard looping and parameter arraying when non-
*              wrapperized functions  are used.
*  Input
*     Formal:  none
*     Private: none
*     Public:  all
*
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void init_structures()
{ 
    initGeneric(&mtcrush_grad);                 /* init MTC crusher gradient structure */
    initGeneric(&fsatcrush_grad);               /* init FATSAT crusher gradient structure */
    initGeneric(&satcrush_grad);                /* init SATBAND crusher gradient structure */
    initGeneric(&tagcrush_grad);                /* init TAGGING crusher gradient structure */
    initGeneric(&tag_grad);                     /* init TAGGING gradient structure */
    initGeneric(&crush_grad);                   /* init crusher gradient structure */
    initGeneric(&spoil_grad);                   /* init spopiler gradient structure */
    initGeneric(&diff_grad);                    /* init diffusion gradient structure */
    initGeneric(&gg_grad);                      /* init generic gradient structure */
    initGeneric(&ircrush_grad);                 /* init IR crusher gradient structure */
    initNull(&null_grad);                       /* init null / dummy gradient */

    initSliceSelect(&ss_grad);                   /* init slice select gradient (90) */
    initSliceSelect(&ss2_grad);                  /* init slice select gradient (180) */
    initSliceSelect(&ssi_grad);                  /* init IR slice select gradient */

    initDephase(&ror_grad);                      /* init dephase gradient */
    initRefocus(&ssr_grad);                      /* init slice refocus gradient */

    initPhase(&pe_grad);                         /* init phase encode gradient */
    initPhase(&per_grad);                        /* init phase encode rewind gradient */
    initPhase(&epipe_grad);                      /* init EPI phase encode gradient */

    initReadout(&ro_grad);                       /* init readout gradient */
    initReadout(&epiro_grad);                    /* init EPI readout gradient */
    initReadout(&nav_grad);                      /* init navigator gradient */

    /***************************************/   
    /* init RF pulses **********************/
    initRf(&mt_rf);                    /* init RF pulse structure - MTC */
    initRf(&fsat_rf);                  /* init RF pulse structure - FATSAT */
    initRf(&sat_rf);                   /* init RF pulse structure - SATBAND */
    initRf(&tag_rf);                   /* init RF pulse structure - RF TAGGING */
    initRf(&p1_rf);                    /* init RF pulse structure */
    initRf(&p2_rf);                    
    initRf(&p3_rf);                    
    initRf(&p4_rf);                    
    initRf(&p5_rf);                    
    initRf(&ir_rf);                    /* init RF pulse structure - IR */

    /* init prepulse parameter */
    mtTime   = 0.0;
    fsatTime = 0.0;
    satTime  = 0.0;
    tagtime  = 0.0;   
    sglerror = 0;
}

/***********************************************************************
*  Function Name: retrieve_parameters
*  Example:    retrieve_parameters();
*  Purpose:    Retrieve VnmrJ parameters 
*  Input
*     Formal:  none
*     Private: none
*     Public:  all
*
*  Output
*     Return:  none
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void retrieve_parameters()
{ 
   char rcvrs[MAXSTR];
   int  nrcvrs,r;
   char asltype_str[MAXSTR];

   getStringSetDefault(GLOBAL,"PDDacquire",PDDacquire,"u"); /* undefined if PDDacquire doesn't exist */

   glim = getval("glim")/100.0;                   /* gradient strength limiting fraction */
   if (glim == 0.0) {
     glim = 0.20;
     warn_message("WARNING: glim is 0, or does not exist.  Running with %.0f",glim*100);
   }
   glimpe = getvalnwarn("glimpe")/100.0;          /* PE gradient strength limiting fraction */
   if (glimpe == 0.0) glimpe = 1.0;
   slewlim = getvalnwarn("slewlim");              /* gradient slew rate limiting factor [%] */
   ssc = getvalnwarn("ssc");                      /* Compressed Steady State scans */
   ss = getvalnwarn("ss");                        /* Steady State scans */
   esp = getvalnwarn("esp");                      /* echo spacing */
   tpemin = getvalnwarn("tpemin");                /* minimum duration of PE gradient */
   trimage = getvalnwarn("trimage");              /* inter-image delay */
   trtype = (int)getvalnwarn("trtype");           /* pack or distribute tr_delay */
   trigger = (int)getvalnwarn("trigger");         /* trigger select */
   trampfixed = getvalnwarn("trampfixed");        /* get trampfixed for this sequence */
   gcrushro  = getvalnwarn("gcrushro");           /* crusher for RO butterfly */
   tcrushro  = getvalnwarn("tcrushro");           /* 0.0 no fixed ramp durations are enforced */

   /*************************************************************/
   /* retrieve arraydim and adjust for compressed/standard mode */
   /* To be used for calculation of total scan time; we factor  */
   /* in the phase encoding steps directly at that point        */
   if (checkflag)
      arraydim = getvalnwarn("saveArraydim");
   else
      arraydim = getvalnwarn("arraydim");
   /* Divide arraydim by phase encoding steps */
   /* Don't just divide by ni because ni = nv*nv2 for 3D experiments */
   if (seqcon[2] == 's') arraydim /= nv;   // 1st phase encode dimension
   if (seqcon[3] == 's') arraydim /= nv2;  // 2nd phase encode dimension
   if (seqcon[4] == 's') arraydim /= nv3;  // 3rd phase encode dimension
   
   /* Adjust for multiple receivers */
   nrcvrs = 0;
   getstr("rcvrs",rcvrs);

   for (r = 0; r < strlen(rcvrs); r++)
     if (rcvrs[r] == 'y') nrcvrs++;
   
   arraydim /= nrcvrs;


   /**************************************/
   /* retrieve EPI variable values       */
   groa = getvalnwarn("groa");               /* EPI tweaker - readout */
   grora = getvalnwarn("grora");             /* EPI tweaker - dephase */
   image = getvalnwarn("image");             /* EPI repetitions */
   images = getvalnwarn("images");           /* EPI repetitions */
   ssepi = getvalnwarn("ssepi");             /* EPI readput steady state */
   tep   = getvalnwarn("tep");               /* Gradient propagation delay */
   nseg  = getvalnwarn("nseg");              /* Number of shots */
   etl   = getvalnwarn("etl");               /* Echo Train Length */
   getstrnwarn("rampsamp",rampsamp);         /* ramp sampling */
   fract_ky = getvalnwarn("fract_ky");       /* fractional k-space flag */
   getstrnwarn("spinecho",spinecho);         /* spin echo flag */
   getstrnwarn("navigator",navigator);

   /**************************************/
   /* retrieve k-space ordering flags    */
   getstrnwarn("ky_order",ky_order);         /* phase order flag */

   /**************************************/
   /* retrieve pre-pulse variable values */
   sglabort   = (int)getvalnwarn("sglabort");     /* error handling flag for SGL */
   sgldisplay = (int)getvalnwarn("sgldisplay");   /* display flag for SGL */
   sglarray   = (int)getvalnwarn("sglarray");     /* array flag for SGL */
   sglpower   = (int)getvalnwarn("sglpower");     /* power calc flag for SGL */
   flip1      = getvalnwarn("flip1");        /* RF pulse flip angle */
   flip2      = getvalnwarn("flip2");        
   flip3      = getvalnwarn("flip3");        
   flip4      = getvalnwarn("flip4");        
   flip5      = getvalnwarn("flip5");        
   echo_frac  = getvalnwarn("echo_frac");    /* echo fraction */
   getstrnwarn("minte",minte);               /* minimum TE flag */
   getstrnwarn("mintr",mintr);               /* minimum TR flag */      
   getstrnwarn("minti",minti);               /* minimum TI flag */      
   getstrnwarn("mintm",mintm);               /* minimum TM flag */      
   getstrnwarn("minesp",minesp);             /* minimum ESP flag for fsems */      
   getstrnwarn("spoilflag",spoilflag);       /* Flag for spoiler gradient */   
   getstrnwarn("fc",fc);                     /* flow com flag */
   getstrnwarn("rfcoil",rfcoil);             /* retrieve RF coil name */
   b1max      = getvalnwarn("b1max");        /* Maximum B1 (Hz) for the rf coil */ 
   getstrnwarn("perewind",perewind);         /* phase rewind flag */
   getstrnwarn("profile",profile);           /* flag to acquire only profile */

   /**************************************/
   /* retrieve PRE-pulse variables - MTC */
   flipmt    = getvalnwarn("flipmt");        /* get rf pulse flip angle */
   mtfrq     = getvalnwarn("mtfrq");         /* MTC offset freq */
   gcrushmt  = getvalnwarn("gcrushmt");      /* MTC crusher amplitude */
   tcrushmt  = getvalnwarn("tcrushmt");      /* MTC crusher duration */

   /*****************************************/
   /* retrieve PRE-pulse variables - FATSAT */
   getstrnwarn("fsat",fsat);                  /* FATSAT flag */
   getstrnwarn("fsatpat",fsatpat);            /* FATSAT RF-pulse shape */
   flipfsat  = getvalnwarn("flipfsat");       /* FATSAT RF-pulse flip angle */
   fsatfrq   = getvalnwarn("fsatfrq");        /* fat offset freq */
   pfsat     = getvalnwarn("pfsat");          /* FATSAT duration */
   gcrushfs  = getvalnwarn("gcrushfs");       /* FATSAT crusher amplitude */
   tcrushfs  = getvalnwarn("tcrushfs");       /* FATSAT crusher duration */

   /**************************************/
   /* retrieve SATBANDS ******************/   
   /*   nsat=getvalnwarn("nsat");   */           /* number of SATBANDS */   
   /*  use that until vnmrj planner returns nsat */
   nsat      = getarray("satthk",satthk);     /* number of SATBANDS     */
   
   if (nsat>MAXNSAT) 
      {
      abort_message("Number of satbands [%d] exceeds maximum allowed number [%d].\n",nsat,MAXNSAT);                	     	       
      }
   getstrnwarn("sat",sat);                    /* SATBAND flag */
   flipsat=getvalnwarn("flipsat");            /* SATBAND flip angle */
   tcrushsat = getvalnwarn("tcrushsat");      /* SATBAND crusher duration */
   gcrushsat = getvalnwarn("gcrushsat");      /* SATBAND crusher amplitude */

   /**************************************/
   /* retrieve RF tagging ****************/  
   getstrnwarn("tag",tag);                     /* RF TAGGING flag  */   
   getstrnwarn("tagpat",tagpat);               /* TAGGING pulse shape  */   
   gcrushtag = getvalnwarn("gcrushtag");       /* TAGGING crusher strength */
   tcrushtag = getvalnwarn("tcrushtag");       /* TAGGING crusher duration */
   wtag      = getvalnwarn("wtag");            /* spatial width of tag [cm]*/
   dtag      = getvalnwarn("dtag");            /* spatial separation of tag [cm]*/
   ptag      = getvalnwarn("ptag");            /* Total duration of RF train */
   tagdir    = getvalnwarn("tagdir");          /* Tagging flag / direction */
   fliptag   = getvalnwarn("fliptag");         /* Tagging flip angle */


   /*****************************************/
   /* retrieve PRE-pulse variables - IR    */
   gcrushir=getvalnwarn("gcrushir");           /* IR crusher strength */
   tcrushir=getvalnwarn("tcrushir");           /* IR crusher duration */
   flipir=getvalnwarn("flipir");               /* IR RF-pulse flip angle */
   thkirfact=getvalnwarn("thkirfact");         /* IR slice thickness factor */
   irsequential=(int)getvalnwarn("irsequential"); /* Sequential IR and readout for each slice */

   /*******************************************/
   /* retrieve Blocked slices variable values */
   blockslices=(int)getvalnwarn("blockslices"); /* Block slices */
   nsblock=(int)getvalnwarn("nsblock");        /* Number of slices per block */

   /**************************************/
   /* retrieve Diffusion variable values */
   getstrnwarn("diff",diff);                   /* Diffusion flag  */   
   dro = getvalnwarn("dro");                   /* Multipliers on diffusion gradient */
   dpe = getvalnwarn("dpe");
   dsl = getvalnwarn("dsl");

   /**************************************/
   /* retrieve Water suppresion variable values */
   getstrnwarn("wspat",wspat);                 /* RF shape  */   
   pws    = getvalnwarn("pws");                /* Multipliers on diffusion gradient */
   flipws = getvalnwarn("flipws");             /* flip angle */

   /***********************************************/
   /* retrieve velocity encoding variables values */
   venc = getvalnwarn("venc");                 /* velocity encoding [cm/s] */
 
   /***************************************/
   /* duty cycle calculations */
   getstrnwarn("checkduty",checkduty );
	
   /***************************************/
   /* sgl events */
   sglEventDebug = getvalnwarn("sgleventdebug");

   /* ASL Variables */
   getstrnwarn("asl",asl);                     /* ASL flag */
   getstrnwarn("asltype",asltype_str);         /* type of ASL: FAIR, STAR, PICORE, CASL */
   getstrnwarn("aslplan",aslplan);             /* ASL graphical planning flag */
   asltag = (int) getvalnwarn("asltag");       /* ASL tag off/tag/control, 0,1,-1 */
   getstrnwarn("asltagcoil",asltagcoil);       /* flag for separate tag coil (requires 3rd RF channel) */
   getstrnwarn("aslrfcoil",aslrfcoil);         /* tag coil name */
   getstrnwarn("paslpat",paslpat);             /* Pulsed ASL (PASL) pulse shape */
   pasl = getvalnwarn("pasl");                 /* PASL pulse duration */
   flipasl = getvalnwarn("flipasl");           /* PASL pulse flip */
   getstrnwarn("asltagrev",asltagrev);         /* Flag to reverse ASL tag position w.r.t. imaging slices */
   getstrnwarn("pcaslpat",pcaslpat);           /* Continuous ASL (CASL) pulse shape */
   pcasl = getvalnwarn("pcasl");               /* CASL pulse duration */
   flipcasl = getvalnwarn("flipcasl");         /* CASL pulse flip */
   caslb1 = getvalnwarn("caslb1");             /* B1 for CASL pulse */
   getstrnwarn("caslctrl",caslctrl);           /* CASL control type (default or sine modulated) */
   getstrnwarn("caslphaseramp",caslphaseramp); /* Flag to phase ramp CASL pulses (long pulses take time to phase ramp) */
   getstrnwarn("starctrl",starctrl);           /* STAR control type (default or double tag) */
   aslthk = getvalnwarn("aslthk");             /* ASL tag slice thickness */
   asladdthk = getvalnwarn("asladdthk");       /* ASL additional tag slice thickness (FAIR) */
   asltagthk = getvalnwarn("asltagthk");       /* Input STAR/PICORE ASL tag thickness */
   aslgap = getvalnwarn("aslgap");             /* ASL tag slice gap (to image slice) */
   aslpos = getvalnwarn("aslpos");             /* ASL tag slice position */
   aslpsi = getvalnwarn("aslpsi");             /* ASL tag slice psi */
   aslphi = getvalnwarn("aslphi");             /* ASL tag slice phi */
   asltheta = getvalnwarn("asltheta");         /* ASL tag slice theta */
   aslctrlthk = getvalnwarn("aslctrlthk");     /* ASL control slice thickness */
   aslctrlpos = getvalnwarn("aslctrlpos");     /* ASL control slice position */
   aslctrlpsi = getvalnwarn("aslctrlpsi");     /* ASL control slice psi */
   aslctrlphi = getvalnwarn("aslctrlphi");     /* ASL control slice phi */
   aslctrltheta = getvalnwarn("aslctrltheta"); /* ASL control slice theta */
   caslgamp = getvalnwarn("caslgamp");         /* Amplitude of CASL tag gradient (G/cm) */
   gspoilasl = getvalnwarn("gspoilasl");       /* ASL tag spoil gradient amplitude */
   tspoilasl = getvalnwarn("tspoilasl");       /* ASL tag spoil gradient duration */
   aslti = getvalnwarn("aslti");               /* ASL inflow time */
   getstrnwarn("minaslti",minaslti);           /* minimum ASL inflow time flag: y = ON, n = OFF */
   slicetr = getvalnwarn("slicetr");           /* TR to next slice (multislice mode) */
   getstrnwarn("minslicetr",minslicetr);       /* minimum TR to next slice (multislice mode) */
   getstrnwarn("ips",ips);                     /* In Plane Saturation (IPS) flag */
   getstrnwarn("ipsplan",ipsplan);             /* IPS graphical planning flag */
   getstrnwarn("ipspat",ipspat);               /* IPS pulse shape */
   pips = getvalnwarn("pips");                 /* IPS pulse duration */
   flipips = getvalnwarn("flipips");           /* IPS pulse flip */
   flipipsf = getvalnwarn("flipipsf");         /* IPS pulse flip factor */
   nips = (int)getvalnwarn("nips");            /* number of IPS pulses */
   ipsthk = getvalnwarn("ipsthk");             /* IPS slice thickness */
   ipsaddthk = getvalnwarn("ipsaddthk");       /* IPS additional slice thickness */
   ipspos = getvalnwarn("ipspos");             /* IPS slice position */
   ipspsi = getvalnwarn("ipspsi");             /* IPS slice psi */
   ipsphi = getvalnwarn("ipsphi");             /* IPS slice phi */
   ipstheta = getvalnwarn("ipstheta");         /* IPS slice theta */
   gspoilips = getvalnwarn("gspoilips");       /* IPS spoil gradient amplitude */
   tspoilips = getvalnwarn("tspoilips");       /* IPS spoil gradient duration */
   getstrnwarn("wetips",wetips);               /* WET IPS flag */
   getstrnwarn("mir",mir);                     /* Multiple Inversion Recovery (MIR) flag */
   getstrnwarn("mirpat",mirpat);               /* MIR pulse shape */
   pmir = getvalnwarn("pmir");                 /* MIR pulse duration */
   flipmir = getvalnwarn("flipmir");           /* MIR pulse flip */
   nmir = (int)getvalnwarn("nmir");            /* number of MIR pulses */
   gspoilmir = getvalnwarn("gspoilmir");       /* MIR spoil gradient amplitude */
   tspoilmir = getvalnwarn("tspoilmir");       /* MIR spoil gradient duration */
   getstrnwarn("autoirtime",autoirtime);       /* Auromatic MIR time calculation flag */
   getstrnwarn("ps",ps);                       /* Pre Saturation (PS) flag */
   getstrnwarn("psplan",psplan);               /* PS graphical planning flag */
   getstrnwarn("pspat",pspat);                 /* PS pulse shape */
   pps = getvalnwarn("pps");                   /* PS pulse duration */
   flipps = getvalnwarn("flipps");             /* PS pulse flip */
   flippsf = getvalnwarn("flippsf");           /* PS pulse flip factor */
   nps = (int)getvalnwarn("nps");              /* number of PS pulses */
   psthk = getvalnwarn("psthk");               /* PS slice thickness */
   psaddthk = getvalnwarn("psaddthk");         /* PS additional slice thickness */
   pspos = getvalnwarn("pspos");               /* PS slice position */
   pspsi = getvalnwarn("pspsi");               /* PS slice psi */
   psphi = getvalnwarn("psphi");               /* PS slice phi */
   pstheta = getvalnwarn("pstheta");           /* PS slice theta */
   gspoilps = getvalnwarn("gspoilps");         /* PS spoil gradient amplitude */
   tspoilps = getvalnwarn("tspoilps");         /* PS spoil gradient duration */
   getstrnwarn("wetps",wetps);                 /* WET PS flag */
   getstrnwarn("q2tips",q2tips);               /* Q2TIPS flag */
   getstrnwarn("q2plan",q2plan);               /* Q2TIPS graphical planning flag */
   getstrnwarn("q2pat",q2pat);                 /* Q2TIPS pulse shape */
   pq2 = getvalnwarn("pq2");                   /* Q2TIPS pulse duration */
   flipq2 = getvalnwarn("flipq2");             /* Q2TIPS pulse flip */
   nq2 = getvalnwarn("nq2");                   /* number of Q2TIPS pulses */
   q2thk = getvalnwarn("q2thk");               /* Q2TIPS slice thickness */
   q2pos = getvalnwarn("q2pos");               /* Q2TIPS slice position */
   q2psi = getvalnwarn("q2psi");               /* Q2TIPS slice psi */
   q2phi = getvalnwarn("q2phi");               /* Q2TIPS slice phi */
   q2theta = getvalnwarn("q2theta");           /* Q2TIPS slice theta */
   gspoilq2 = getvalnwarn("gspoilq2");         /* Q2TIPS spoil gradient amplitude */
   tspoilq2 = getvalnwarn("tspoilq2");         /* Q2TIPS spoil gradient duration */
   q2ti = getvalnwarn("q2ti");                 /* ASL inflow time to Q2TIPS */
   getstrnwarn("minq2ti",minq2ti);             /* minimum ASL inflow time to Q2TIPS flag: y = ON, n = OFF */
   getstrnwarn("vascsup",vascsup);             /* Vascular suppression (VS) flag */
   gvs = getvalnwarn("gvs");                   /* VS gradient amplitude */
   tdeltavs = getvalnwarn("tdeltavs");         /* VS gradient delta */
   bvalvs = getvalnwarn("bvalvs");             /* VS b-value */
   getstrnwarn("asltest",asltest);             /* ASL test parameter for test mode (asltestmode='y') */

   asltype = 0; // unknown type
   if ((!strcmp(asltype_str,"fair")) || (!strcmp(asltype_str,"FAIR")))
     asltype = FAIR;
   else if ((!strcmp(asltype_str,"star")) || (!strcmp(asltype_str,"STAR")))
     asltype = STAR;
   else if ((!strcmp(asltype_str,"picore")) || (!strcmp(asltype_str,"PICORE")))
     asltype = PICORE;
   else if ((!strcmp(asltype_str,"casl")) || (!strcmp(asltype_str,"CASL")))
     asltype = CASL;

   /* Below are additional variables required for asl sequence */
   irthk     =       getvalnwarn("irthk");
   irgap     =       getvalnwarn("irgap");
   qgcrush   =       getvalnwarn("qgcrush");
   qtcrush   =       getvalnwarn("qtcrush");
   nquipss   = (int) getvalnwarn("nquipss");
   asl_ti1   =       getvalnwarn("ti1");
   getstrnwarn("quipss",quipss);
   /* Above are additional variables required for asl sequence */

}	


/***********************************************************************
*  Function Name: init_tablepar
*  Example:    init_tablepar("pelist");
*  Purpose:    Initializes the specified parameter to hold real time table values.
*
***********************************************************************/
void init_tablepar(char *parname)
{
  /* P_getVarInfo() only seems to work if parameter is in acquisition group */
  /* Make sure parameter is in acquisition group and initialized to zero */
  putCmd("exists('%s','parameter'):$ex if ($ex > 0) then destroy('%s') endif\n",parname,parname);
  putCmd("create('%s','integer')\n",parname);
  putCmd("setlimit('%s',%d,%d,1)\n",parname,TABLEMAXVAL,-TABLEMAXVAL-1);
  putCmd("setprotect('%s','on',256)\n",parname);
}


/***********************************************************************
*  Function Name: writetabletopar
*  Example:    writetabletopar(t1,"pelist");
*  Purpose:    Writes real time table values to the specified parameter.
*
***********************************************************************/
void writetabletopar(int tablename,char *parname)
{
  int index,*elementpntr,ret,i;

  if (ix > 1) return;

  /* Make sure tablename is a valid table */
  if (!isTable(tablename))
    abort_message("Invalid table name in writetabletopar() call");

  /* Set index of table */
  index = tablename-BASEINDEX;

  /* Return if table is empty */
  if (Table[index]->table_size < 1) return;

  /* P_getVarInfo() only seems to work if parameter is in acquisition group */
  /* init_tablepar should initialize the parameter appropriately */
  /* Return if parameter does not exist in current tree (or is not in acquisition group) */
  if ( (ret = P_getVarInfo(CURRENT,parname,&dvarinfo)) ) return;

  /* Make sure parameter is an integer */
  if (dvarinfo.basicType != T_REAL || dvarinfo.subtype != ST_INTEGER)
    putCmd("destroy('%s') create('%s','integer')\n",parname,parname);

  /* Make sure parameter has appropriate limits */
  if ((int)dvarinfo.maxVal != TABLEMAXVAL || (int)dvarinfo.minVal != -TABLEMAXVAL-1)
    putCmd("setlimit('%s',%d,%d,1)\n",parname,TABLEMAXVAL,-TABLEMAXVAL-1);

  /* Make sure parameter does not cause acquisition array */
  if (!(dvarinfo.prot & 256)) putCmd("setprotect('%s','on',256)",parname);

  /* Initialize parameter value */
  putCmd("%s = 0",parname);

  /* Fill the parameter with table values */
  elementpntr = Table[index]->hold_pointer;
  for (i=0;i<Table[index]->table_size;i++)
    putCmd("%s[%d] = %d",parname,i+1,*elementpntr++/Table[index]->divn_factor);

  return;
}


void putarray(char *param, double *value, int n) 
{
/* Return a numerical arrayed parameter value to VnmrJ 
   in both current and processed trees */
  int i;

  /* Set parameter in 'current' tree */
  putCmd("exists('%s','parameter'):$ex\n",param);
  putCmd("if ($ex > 0) then \n");
  putCmd("  %s = %.10f\n",param,value[0]);   /* reset parameter to non-array */
  if( n > 1 ) {
    for (i = 1; i < n; i++)
      putCmd("  %s[%d] = %.10f\n",param,i+1,value[i]);
  }
  putCmd("endif\n");

  /* Set parameter in 'processed' tree */
  putCmd("exists('%s','parameter','processed'):$ex\n",param);
  putCmd("if ($ex > 0) then \n");
  putCmd("  setvalue('%s',%.10f,0,'processed')\n",param,value[0]);   /* reset parameter to non-array */
  if( n > 1 ) {
    for (i = 1; i < n; i++)
      putCmd("  setvalue('%s',%.10f,%d,'processed')\n",param,value[i],i+1);
  }
  putCmd("endif\n");
}


void putvalue(char *param, double value) 
{
  putarray(param,&value,1);
}


void putstring(char *param, char value[]) 
{
/* Return a string parameter value to VnmrJ in both current and processed trees */
  putCmd("exists('%s','parameter'):$ex\n",param);
  putCmd("if ($ex > 0) then %s='%s' endif\n",param,value);

  putCmd("exists('%s','parameter','processed'):$ex\n",param);
  putCmd("if ($ex > 0) then setvalue('%s','%s','processed') endif\n",param,value);
}

void sgl_abort_message(char *format, ...) 
{
  va_list vargs;
  char msg[2048];
  
  va_start(vargs, format);
  vsprintf(msg,format,vargs);
  va_end(vargs);

  if (!sglabort)
    abort_message("%s",msg);
  else
    warn_message("%s",msg);

  sglerror = 2;  /* We've already printed some error messages, 
                    no need to print an error in error_check */
}

void sgl_error_check(int error_flag) 
{
  if (error_flag) {
    if (sglabort == 0)
      abort_message("ERROR(s) in SGL, set sglabort = 1 for more information");
    else
      abort_message("ERROR(s) in SGL, see list or Text Output");
  }
}
   
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*-------------- E N D    H E L P E R    F U N C T I O N S -------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
