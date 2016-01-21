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
#include "sglWrappers.h"
extern int checkflag;
extern int option_check(const char *);

void calc_grad_duty(double time)
   {
   double  gradenergy[3],gradduty[3];
   double  mult=1.0;
   double  currentlimit,RMScurrentlimit;
   double  sglduty,dutylimit;
   int     checksilent,r,nrcvrs,arraydim,error=0;
   char    rcvrs[MAXSTR];

   currentlimit = getval("currentlimit");
   RMScurrentlimit = getval("RMScurrentlimit");
   getRealSetDefault(GLOBAL, "sglduty", &sglduty,0.0);
   dutylimit = RMScurrentlimit/currentlimit;
   checksilent = option_check("checksilent");

   /* Adjust array dimenstion for multiple receivers */
   nrcvrs = 0;
   getstr("rcvrs",rcvrs);
   arraydim = getvalnwarn("arraydim");
   for (r = 0; r < strlen(rcvrs); r++) {
     if (rcvrs[r] == 'y') nrcvrs++;
   }
   arraydim /= nrcvrs;

   if (seqcon[2] == 'c')
     mult *= nv;
   if (seqcon[3] == 'c')
     mult *= nv2;

   if (!checkflag)
     mult *= arraydim;

   getgradpowerintegral(gradenergy);
   gradduty[0] = sqrt(gradenergy[0]/(mult*time));
   gradduty[1] = sqrt(gradenergy[1]/(mult*time));
   gradduty[2] = sqrt(gradenergy[2]/(mult*time));

   if (sglduty && ((checkflag && !checksilent) || (!checksilent && ix == arraydim))) {
     text_message("Grad energy X: %.3g    Grad energy Y: %.3g    Grad energy Z: %.3g",gradenergy[0],gradenergy[1],gradenergy[2]);
     text_message("Grad duty X: %.3g%%    Grad duty Y: %.3g%%    Grad duty Z: %.3g%%",100*gradduty[0],100*gradduty[1],100*gradduty[2]);
   }

   if ((gradduty[0] > dutylimit) && ((checkflag && !checksilent) || (!checksilent && ix == arraydim))) {
     text_message("%s: X gradient duty cycle %5.1f%% exceeds allowed limit of %5.1f%%",seqfil,100*gradduty[0],100*dutylimit);
     error = 1;
   }
   if ((gradduty[1] > dutylimit) && ((checkflag && !checksilent) || (!checksilent && ix == arraydim))) {
     text_message("%s: Y gradient duty cycle %5.1f%% exceeds allowed limit of %5.1f%%",seqfil,100*gradduty[1],100*dutylimit);
     error = 1;
   }
   if ((gradduty[2] > dutylimit) && ((checkflag && !checksilent) || (!checksilent && ix == arraydim))) {
     text_message("%s: Z gradient duty cycle %5.1f%% exceeds allowed limit of %5.1f%%",seqfil,100*gradduty[2],100*dutylimit);
     error = 1;
   }
   if (error) {
     if (sglduty)
       warn_message("%s: Duty cycle exceeds allowed limit",seqfil);
     else
       abort_message("%s: Duty cycle exceeds allowed limit",seqfil);
   }
   }


/**************************************************************************/
/*** SLICE SELECT GRADIENT WRAPPERS **************************************/
/**************************************************************************/
void init_slice(SLICE_SELECT_GRADIENT_T *grad, char name[],double thk)
   {
   if ((ix > 1) && !sglarray) return;
   initSliceSelect(grad);                     /* initialize slice select gradient with defaults*/
   grad->thickness  = thk;		      /* assign slice thickness */
   grad->maxGrad    = gmax;                   /* assign maximum allowed gradient */
   strcpy(grad->name,name);                   /* assign waveform name */
   }

void init_slice_butterfly(SLICE_SELECT_GRADIENT_T *grad, char name[], 
                          double thk, double gcrush, double tcrush)
   {
   if ((ix > 1) && !sglarray) return;
   initSliceSelect(grad);                        /* initialize slice select gradient with defaults*/
   grad->thickness  = thk;                       /* assign slice thickness */
   grad->maxGrad    = gmax;                      /* assign maximum allowed gradient */     
   strcpy(grad->name,name);                      /* assign waveform name */
   grad->enableButterfly   = TRUE;               /* enable butterfly gradients */
   grad->cr1amp            = grad->cr2amp            = gcrush;       /* assign crusher amplitude */
   grad->crusher1Duration  = grad->crusher2Duration  = tcrush;       /* assign crusher duration */
   grad->crusher1CalcFlag  = grad->crusher2CalcFlag  = MOMENT_FROM_DURATION_AMPLITUDE; 
}

void calc_slice(SLICE_SELECT_GRADIENT_T *grad, RF_PULSE_T *rf,
                int write_flag, char VJparam_g[])
   {
   
   if ((ix > 1) && !sglarray) return;

   if (grad->thickness <= 0) {
     sgl_abort_message("ERROR gradient %s: Slice thickness <= 0",grad->name);
     return;
   }

   if ((grad->enableButterfly) && (grad->cr1amp > grad->maxGrad)) {
     sgl_abort_message("ERROR gradient %s: Crusher amplitude (%.2f) exceeds maximum (%.2f)",
       grad->name,grad->cr1amp,grad->maxGrad);
   }
     
   grad->writeToDisk = write_flag;              /* set writeToDisk flag */
   calcSlice(grad, rf);                         /* create slice select gradient */

   if (grad->error == ERR_AMPLITUDE) {
     sgl_abort_message("ERROR gradient %s: Thickness too small, minimum is %1.2fmm",
	           grad->name,grad->rfBandwidth /(grad->maxGrad * MM_TO_CM * grad->gamma));
   }
   
   /* return slice select gradient parameter to VnmrJ space */
   if (strcmp(VJparam_g, "")) 
      putvalue(VJparam_g, grad->ssamp);
   }

void init_slice_refocus(REFOCUS_GRADIENT_T *refgrad, char name[]) 
   {
   if ((ix > 1) && !sglarray) return;
   initRefocus(refgrad);                          /* initialize refocus gradient with defaults */
   refgrad->maxGrad = glim*gmax;                  /* assign maximum allowed gradient */     
   strcpy(refgrad->name,name);                    /* assign waveform name */
   }


void calc_slice_refocus(REFOCUS_GRADIENT_T *refgrad, SLICE_SELECT_GRADIENT_T *grad, 
                        int write_flag,char VJparam_g[]) 
   {   
   
   if ((ix > 1) && !sglarray) return;
   refgrad->balancingMoment0 = grad->m0ref * refgrad->gmult;  /* assign refocusing moment */
   strcpy(refgrad->param1,VJparam_g);                         /* assign VNMRJ parameter 1 */
   refgrad->writeToDisk      = write_flag;                    /* set writeToDisk flag */
   calcRefocus(refgrad);                                      /* calculate slice refocusign gradient */

   if (refgrad->error == ERR_AMPLITUDE) {
     sgl_abort_message("ERROR gradient %s: Amplitude too large (%f), check glim (%.0f)",grad->name,grad->amp,glim*100);
   }

      /* return slice refocusing gradient parameter to VnmrJ space */
      if (strcmp(VJparam_g, "")) 
         putvalue(VJparam_g, refgrad->amp);
   }

void calc_slice_dephase(REFOCUS_GRADIENT_T *refgrad, SLICE_SELECT_GRADIENT_T *grad, 
                        int write_flag,char VJparam_g[]) 
   {   
   
   if ((ix > 1) && !sglarray) return;
   refgrad->balancingMoment0 = grad->m0def * refgrad->gmult;  /* assign dephasing moment */
   strcpy(refgrad->param1,VJparam_g);                         /* assign VNMRJ parameter 1 */
   refgrad->writeToDisk      = write_flag;                    /* set writeToDisk flag */
   calcRefocus(refgrad);                                      /* calculate slice refocusign gradient */

   if (refgrad->error == ERR_AMPLITUDE) {
     sgl_abort_message("ERROR gradient %s: Amplitude too large (%f), check glim (%.0f)",grad->name,grad->amp,glim*100);
   }

      /* return slice refocusing gradient parameter to VnmrJ space */
      if (strcmp(VJparam_g, "")) 
         putvalue(VJparam_g, refgrad->amp);
   }

/**************************************************************************/
/*** READOUT GRADIENT WRAPPERS ********************************************/
/**************************************************************************/
void init_readout(READOUT_GRADIENT_T *grad, char name[], 
                  double lro, double np, double sw)      
   {   
   if ((ix > 1) && !sglarray) return;
   initReadout(grad);                               /* initialize with default values */
   grad->numPointsFreq = np/2.0;  		    /* assign number of points in frequncy direction */
   grad->acqTime       = np/(2*sw);   	            /* set acquisition time */
   grad->maxGrad       = gmax;                      /* assign maximum allowed gradient */
   grad->fov           = lro*10;                    /* set field of view [cm] */
   strcpy(grad->name,name);                         /* assign waveform name */
   }

void init_readout_butterfly(READOUT_GRADIENT_T *grad, char name[], 
                            double lro, double np, double sw,
		            double gcrush, double tcrush) 
   {
   if ((ix > 1) && !sglarray) return;
   initReadout(grad);                               /* initialize with default values */
   grad->numPointsFreq = np/2.0;            	    /* assign number of points in frequncy direction */
   grad->acqTime       = np/2/sw;   	            /* set acquisition time */
   grad->maxGrad       = gmax;                      /* assign maximum allowed gradient */
   grad->fov           = lro*10;                    /* set field of view [cm] */
   strcpy(grad->name,name);                         /* assign waveform name */

   grad->enableButterfly   = TRUE;                   /* enable butterfly gradients */
   grad->cr1amp            = grad->cr2amp            = gcrush;           /* assign crusher amplitude */
   grad->crusher1Duration  = grad->crusher2Duration  = tcrush;           /* assign crusher duration */
   grad->crusher1CalcFlag  = grad->crusher2CalcFlag  = MOMENT_FROM_DURATION_AMPLITUDE;
   }

void calc_readout(READOUT_GRADIENT_T *grad, int write_flag, 
                  char VJparam_g[], char VJparam_sw[], char VJparam_at[]) 
   {
   
   if ((ix > 1) && !sglarray) return;

   if ((grad->enableButterfly) && (grad->cr1amp > grad->maxGrad)) {
     sgl_abort_message("ERROR gradient %s: Crusher amplitude (%.2f) exceeds maximum (%.2f)",
       grad->name,grad->cr1amp,grad->maxGrad);
   }
     
   grad->writeToDisk = write_flag;          /* assign writeToDisk flag */
   calcReadout(grad);                       /* calculate and create readout gradient */
 

   if (grad->error == ERR_AMPLITUDE) {
     sgl_abort_message("ERROR gradient %s: FOV (RO) too small, increase to %.2fmm\n",
/*       grad->name,grad->acqTime * grad->gamma * grad->maxGrad * MM_TO_CM); */
		grad->name,grad->bandwidth/(grad->gamma * grad->maxGrad * MM_TO_CM));
   }
   
    /* return acquisition parameters to VnmrJ space */
   if (strcmp(VJparam_g, ""))
      putvalue(VJparam_g,  grad->roamp);
   if (strcmp(VJparam_sw, ""))
      putvalue(VJparam_sw, grad->bandwidth);
   if (strcmp(VJparam_at, ""))
      putvalue(VJparam_at, grad->acqTime);
   }

void init_readout_refocus(REFOCUS_GRADIENT_T *refgrad, char name[])
   {
   if ((ix > 1) && !sglarray) return;
   initDephase(refgrad);                          /* initialize dephase gradient with defaults */
   refgrad->maxGrad = glim*gmax;                  /* assign maximum allowed gradient */     
   strcpy(refgrad->name,name);                    /* assign waveform name */
   }

void calc_readout_refocus(REFOCUS_GRADIENT_T *refgrad, READOUT_GRADIENT_T *grad,
                          int write_flag, char VJparam_g[]) 
   {
   if ((ix > 1) && !sglarray) return;
   refgrad->balancingMoment0 = grad->m0ref * refgrad->gmult;                    /* Assign dephase moment */
   refgrad->writeToDisk      = write_flag;                     /* Set writeToDisk flag */
   strcpy(refgrad->param1,VJparam_g);                          /* assign VNMRJ parameter 1 */
   calcRefocus(refgrad);                                       /* calculate dephase gradient */
   
   /* return dephase gradient parameter to VnmrJ space */   
   if (strcmp(VJparam_g, ""))
      putvalue(VJparam_g, refgrad->amp);
   }

void calc_readout_rephase(REFOCUS_GRADIENT_T *refgrad, READOUT_GRADIENT_T *grad,
                          int write_flag, char VJparam_g[]) 
   {
   if ((ix > 1) && !sglarray) return;
   refgrad->balancingMoment0 = grad->m0def * refgrad->gmult;   /* Assign dephase moment */
   refgrad->writeToDisk      = write_flag;                     /* Set writeToDisk flag */
   strcpy(refgrad->param1,VJparam_g);                          /* assign VNMRJ parameter 1 */
   calcRefocus(refgrad);                                       /* calculate dephase gradient */
   
   /* return dephase gradient parameter to VnmrJ space */   
   if (strcmp(VJparam_g, ""))
      putvalue(VJparam_g, refgrad->amp);
   }


/**************************************************************************/
/*** PHASE ENCODING GRADIENT WRAPPERS *************************************/
/**************************************************************************/
void init_phase(PHASE_ENCODE_GRADIENT_T *grad, char name[], double lpe, double nv) 
   {
   if ((ix > 1) && !sglarray) return;
   initPhase(grad);                             /* initialize phase encode gradient with defaults */
   grad->fov      = lpe*10;                     /* field of view in phase encode directioin [cm] */
   grad->steps    = nv;                         /* number of phase encode steps */
   grad->maxGrad  = gmax*glimpe;                /* maximum allowed gradient */   
   grad->calcFlag = SHORTEST_DURATION_FROM_MOMENT;  
   strcpy(grad->name,name);                     /* assign waveform name */
   }

void calc_phase(PHASE_ENCODE_GRADIENT_T *grad, int write_flag, char VJparam_g[], char VJparam_t[]) 
   {
   if ((ix > 1) && !sglarray) return;
   grad->writeToDisk = write_flag;          /* set writeToDiskFlag */
   strcpy(grad->param1,VJparam_g);          /* assign VNMRJ parameter 1 */
   strcpy(grad->param2,VJparam_t);          /* assign VNMRJ parameter 2 */
   calcPhase(grad);                         /* calculate and create phase encode grdient */
   /* return phase encode gradient parameters to VnmrJ space */   
   if (strcmp(VJparam_g, ""))
      putvalue(VJparam_g, grad->amp);
   if (strcmp(VJparam_t, "") )
      putvalue(VJparam_t, grad->duration);
   }


/**************************************************************************/
/*** DEPHASING GRADIENT WRAPPERS ******************************************/
/**************************************************************************/

void init_dephase( GENERIC_GRADIENT_T *grad, char name[] )
{
	if( (ix > 1) && !sglarray ) return;
	initDephase( grad );
	grad->maxGrad = glim*gmax;
	strcpy(grad->name, name);	
}

void calc_dephase( GENERIC_GRADIENT_T *grad, int write_flag, double moment0,
					char VJparam_g[], char VJparam_t[] )
{
	if( (ix > 1) && !sglarray ) return;
	grad->balancingMoment0 = moment0 * grad->gmult;
	grad->writeToDisk = write_flag;
	strcpy( grad->param1, VJparam_g );
	strcpy( grad->param2, VJparam_t );
	calcRefocus( grad );
	if( strcmp( VJparam_g, "" ) )
		putvalue( VJparam_g, grad->amp );
	if( strcmp( VJparam_t, "" ) )
		putvalue( VJparam_t, grad->duration );
}


/**************************************************************************/
/*** GENERIC GRADIENT WRAPPERS ********************************************/
/**************************************************************************/
void init_generic(GENERIC_GRADIENT_T *grad, char name[], double amp, double time) 
   {
   if ((ix > 1) && !sglarray) return;
   initGeneric(grad);                            /* initialize generic gradient with defaults */
   grad->amp = amp;                              /* assign gradient amplitude */
   grad->duration  = time;                       /* assign gradient duration */
   grad->maxGrad   = gmax;                       /* assign maximum allowed gradient */               
   strcpy(grad->name,name);                      /* assign waveform name */
   }


void calc_generic(GENERIC_GRADIENT_T *grad, int write_flag, char VJparam_g[], char VJparam_t[])
   {
   double gradamp;

   if ((ix > 1) && !sglarray) return;
   grad->writeToDisk = write_flag;        /* assign writeToDisk flag */
   strcpy(grad->param1,VJparam_g);        /* Assign VNMRJ paramter 1 */
   strcpy(grad->param2,VJparam_t);        /* Assign VNMRJ paramter 2 */

   if (grad->amp > grad->maxGrad) {
     sgl_abort_message("ERROR gradient %s: Crusher amplitude (%.2f) exceeds maximum (%.2f)",
       grad->name,grad->amp,grad->maxGrad);
   }


   if (grad->duration == 0) {
     if (strcmp(grad->param2, ""))  /* The name of the duration is supplied */
       sgl_abort_message("ERROR gradient %s: Duration (%s) is zero",grad->name,grad->param2);
     else
       sgl_abort_message("ERROR gradient %s: Duration is zero",grad->name);
     return;
   }
   
   gradamp = grad->amp;
   if (grad->amp == 0) {
     /* temporarily set it to a non-zero value in order to calculate a shape */
     grad->amp = 1.0/32767.0*gmax;
   }
   
   
   calcGeneric(grad);                     /* calculate and create generic gradient */

   /* reset gradient amplitude */
   if (gradamp == 0) {
     grad->amp = 0;
     grad->m0  = 0;
   }

   if (strcmp(grad->param1, ""))
       putvalue(grad->param1, grad->amp);
   if (strcmp(grad->param2, ""))
       putvalue(grad->param2, grad->duration);
   }


void trapezoid(GENERIC_GRADIENT_T *grad, char name[], double amp, double time, double moment, int write_flag)
   {
   
   /* check is input values for moment, amplitude, time are set */
   if ((amp == 0) && (time == 0) && (moment == 0)) {
      sgl_abort_message("ERROR gradient %s: All input values to <trapezoid> function are zero",name);
      return;
   }

   initGeneric(grad);                            /* initialize generic gradient with defaults */

   grad->amp       = amp;                        /* assign amplitude */
   grad->duration  = time;                       /* assign duration */
   grad->m0        = moment;                     /* assign moment0 */
   grad->maxGrad   = gmax;                       /* assign maximum allowed gradient */               
   grad->writeToDisk = write_flag;               /* set writeToDisk flag */
   strcpy(grad->name,name);                      /* assign waveform name */

   /* check amplitude */
   if (amp > glim*gmax) {
      sgl_abort_message("ERROR gradient %s: amp too large (%f), reduced to glim*gmax (%f)",
                    name, amp, glim*gmax);
   }
      
   /* evaluate imputs and set correct calcFlag */   
   if ((amp != 0) && (time != 0) && (moment == 0)) {
     grad->calcFlag = MOMENT_FROM_DURATION_AMPLITUDE;
   }
   else if ((amp == 0) && (time != 0) && (moment != 0)) {
     grad->calcFlag = AMPLITUDE_FROM_MOMENT_DURATION;
   }
   else if ((amp != 0) && (time == 0) && (moment != 0)) {
     grad->calcFlag = DURATION_FROM_MOMENT_AMPLITUDE;
   }

   calcGeneric(grad);
   }


/**************************************************************************/
/*** SIMULTANEOUS GRADIENTS ***********************************************/
/**************************************************************************/
/***********************************************************************
*  Function Name: calc_sim_gradient
*  Example:       calc_sim_gradient (GENERIC_GRADIENT_T  *grad0, 
*				    GENERIC_GRADIENT_T  *grad1,
*                                   GENERIC_GRADIENT_T  *grad2, 
*                                   double min_tpe, 
*                                   int write_flag)
*  Purpose:    Blocks up to three gradient
*  Input
*     Formal:  *grad0     - pointer to generic structure
*              *grad0     - pointer to generic structure
*              *grad0     - pointer to generic structure  
*              min_tpe    - minimum duration
*              write_flag - write to disk flag 
*     Private: none
*     Public:  
*  Output
*     Return:  double - duration of blocked greadients
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      ONLY works for trapezoidal gradients
***********************************************************************/
double calc_sim_gradient(GENERIC_GRADIENT_T  *grad0, GENERIC_GRADIENT_T  *grad1,
                         GENERIC_GRADIENT_T  *grad2, double min_tpe, int write_flag)
{ 
   int    i;                                /* loop counter */
   int    typesum = 0;                      /* check for gradient type */
   double time    = 0.0;                    /* longest duration of gradient */
   double tramp   = 0.0;                    /* longest ramp time */
   double duration0, duration1, duration2;  /* gradient durations */
   double tramp0, tramp1,tramp2;            /* gradient ramp time */
   double amp0, amp1, amp2;
   GENERIC_GRADIENT_T* grad[3];  /* internal array of structs to allow indexing */

   if ((ix > 1) && !sglarray) return(grad0->duration);
   /* assign input structs to internal array to allow use in for loop ***/
   grad[0] = grad0;
   grad[1] = grad1;
   grad[2] = grad2;

   /* check for  shape -> all gradients need to be trapezoidal */
   for (i=0; i<3; i++) {
     if (grad[i]->shape != TRAPEZOID) {
       displayGeneric(grad[i]);
       sgl_abort_message("ERROR calc_sim_gradient: Gradient %d ('%s') must be trapezoidal",i,grad[i]->name);   
     }
   }
   
   /* find longest gradient duration */
   duration0 = grad[0]->duration - grad[0]->tramp; /* duration of a square gradient with same integral & amplitude */
   duration1 = grad[1]->duration - grad[1]->tramp;
   duration2 = grad[2]->duration - grad[2]->tramp;
  
   time = MAX(MAX(duration0,duration1),duration2);  /* should already be granulated */

	if( trampfixed > 0.0 ) {

		tramp = grad[0]->tramp;

	} else {
		amp0   = (fabs(grad[0]->m0) + fabs(grad[0]->areaOffset))/time;  /* amplitude of square gradient with same integral and duration "time" */
		amp1   = (fabs(grad[1]->m0) + fabs(grad[1]->areaOffset))/time;
		amp2   = (fabs(grad[2]->m0) + fabs(grad[2]->areaOffset))/time;
  
   		tramp0 = amp0/grad[0]->slewRate; /* ramp time with new amplitude */
   		tramp1 = amp1/grad[1]->slewRate;
   		tramp2 = amp2/grad[2]->slewRate;

   		tramp = MAX(MAX(tramp0,tramp1),tramp2);
   		tramp = granularity(tramp,grad[0]->resolution);

	
	}

   /* But use min_tpe as duration, if it is longer */
   time = MAX(time+tramp,granularity(min_tpe,grad[0]->resolution));
    
   /* assign new duration, ramp time, calc flag, and write flag to all 3 */
   grad[0]->duration = grad[1]->duration = grad[2]->duration = time;
   grad[0]->tramp    = grad[1]->tramp    = grad[2]->tramp    = tramp;
   grad[0]->calcFlag = grad[1]->calcFlag = grad[2]->calcFlag = AMPLITUDE_FROM_MOMENT_DURATION_RAMP;

   /* set writeToDisk flag */  
   grad[0]->writeToDisk = grad[1]->writeToDisk = grad[2]->writeToDisk = write_flag;  

   /* Re-calculate gradients */ 
   for (i=0; i<3; i++) {
     switch(grad[i]->type) 
     {
	   case REFOCUS_GRADIENT_TYPE:
         calcRefocus(grad[i]);
         if (strcmp(grad[i]->param1, ""))
           putvalue(grad[i]->param1, grad[i]->amp);
         break;
       case DEPHASE_GRADIENT_TYPE:
         calcDephase(grad[i]);
         if (strcmp(grad[i]->param1, ""))
           putvalue(grad[i]->param1, grad[i]->amp);
         break;
       case PHASE_GRADIENT_TYPE:
         calcPhase(grad[i]);
         if (strcmp(grad[i]->param1, ""))
           putvalue(grad[i]->param1, grad[i]->amp);
         if (strcmp(grad[i]->param2, "")) 
           putvalue(grad[i]->param2, grad[i]->duration);
         break;
       case GENERIC_GRADIENT_TYPE:
         calcGeneric(grad[i]);
         if (strcmp(grad[i]->param1, ""))
           putvalue(grad[i]->param1, grad[i]->amp);
         break;
       default:
         typesum += 1;
     }
   }

   if (typesum >=2) {
     sgl_abort_message("ERROR calc_sim_gradient: 2 or more unspecified gradients as arguments");
   }
   
   /* return longest duration */
   return(time);
   }

  /**************************************************************************/
/*** SIMULTANEOUS GRADIENTS WITH ZEROFILL ***********************************************/
/**************************************************************************/
/***********************************************************************
*  Function Name: calc_zfill_gradient
*  Example:       calc_zfill_gradient (GENERIC_GRADIENT_T  *grad0, 
*				    GENERIC_GRADIENT_T  *grad1,
*                                   GENERIC_GRADIENT_T  *grad2)
*                                    
*                                   
*  Purpose:    Creates up to three gradients with the duration of the longest one, and zerofill the rest
*  Input
*     Formal:  *grad0     - pointer to generic structure
*              *grad0     - pointer to generic structure
*              *grad0     - pointer to generic structure  
*               Private: none
*     Public:  
*  Output
*     Return:  double - duration of blocked greadients
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:      ONLY works for trapezoidal gradients
***********************************************************************/
double calc_zfill_gradient(FLOWCOMP_T *grad0, GENERIC_GRADIENT_T  *grad1,
                         GENERIC_GRADIENT_T  *grad2)

{ 
   
   double time    = 0.0;                    /* longest duration of gradient */
   double duration0, duration1, duration2;  /* gradient durations */
   
   ZERO_FILL_GRADIENT_T zf_grad0,zf_grad1, zf_grad2; /* define zero-fill grad structure in case of flow comp */

   if ((ix > 1) && !sglarray) return(grad0->duration);
   

   /* find longest gradient duration */
   duration0 = grad0->duration;//- grad[0]->tramp; /* duration of a square gradient with same integral & amplitude */
   duration1 = grad1->duration;// - grad[1]->tramp;
   duration2 = grad2->duration;// - grad[2]->tramp;
  
   time = MAX(MAX(duration0,duration1),duration2);  /* should already be granulated */
   text_message("time is %f", time);

/* zerofill new duration for each gradient */

    initZeroFillGradient(&zf_grad0); /* initialize zerofill structure for pe grad */
   // strcpy(zf_grad0.name,"zfgrad0");   /*fill in the unique name, otherwise the name will be repeated for each zerofill pattern*/
     strcpy(zf_grad0.name,"zfgrad0");  //Don't explode!!!
    text_message("grad0 name is %s", grad0->name);

    zf_grad0.numPoints= grad0->numPoints ;/* assign number of waveform points */
    zf_grad0.dataPoints = grad0->dataPoints; /* assign waveform to be zero-filled */
    zf_grad0.newDuration= time; /* duration of the fc grad for readout */
    zf_grad0.location = FRONT; /* add zero at front of waveform */
    zeroFillGradient(&zf_grad0);
    if (zf_grad0.error) abort_message("Gradient library error --> Check text window \n");
    //Now put it back
    grad0->duration=time ;
    grad0->numPoints=zf_grad0.numPoints ;/* assign number of waveform points */
    grad0->dataPoints= zf_grad0.dataPoints ;  /* assign waveform to be zero-filled */
    writeToDisk(grad0->dataPoints, grad0->numPoints, 0, grad0->resolution,
				     TRUE /*rolout*/, grad0->name); 
    initZeroFillGradient(&zf_grad1); /* initialize zerofill structure for pe grad */
    strcpy(zf_grad1.name,"zfgrad1");   /*fill in the unique name, otherwise the name will be repeated for each zerofill pattern*/
    

    zf_grad1.numPoints= grad1->numPoints ;/* assign number of waveform points */
    zf_grad1.dataPoints = grad1->dataPoints; /* assign waveform to be zero-filled */
    zf_grad1.newDuration= time; /* duration of the fc grad for readout */
    zf_grad1.location = FRONT; /* add zero at front of waveform */
    zeroFillGradient(&zf_grad1);
    if (zf_grad1.error) abort_message("Gradient library error --> Check text window \n");
    //Now put it back
    grad1->duration=time ;
    grad1->numPoints=zf_grad1.numPoints ;/* assign number of waveform points */
    grad1->dataPoints= zf_grad1.dataPoints ;  /* assign waveform to be zero-filled */
    writeToDisk(grad1->dataPoints, grad1->numPoints, 0, grad1->resolution,
				     grad1->rollOut, grad1->name); 

    initZeroFillGradient(&zf_grad2); /* initialize zerofill structure for pe grad */
    strcpy(zf_grad2.name,"zfgrad2");   /*fill in the unique name, otherwise the name will be repeated for each zerofill pattern*/
    

    zf_grad2.numPoints= grad2->numPoints ;/* assign number of waveform points */
    zf_grad2.dataPoints = grad2->dataPoints; /* assign waveform to be zero-filled */
    zf_grad2.newDuration= time; /* duration of the fc grad for readout */
    zf_grad2.location = FRONT; /* add zero at front of waveform */
    zeroFillGradient(&zf_grad2);
    if (zf_grad2.error) abort_message("Gradient library error --> Check text window \n");
    //Now put it back
    grad2->duration=time ;
    grad2->numPoints=zf_grad2.numPoints ;/* assign number of waveform points */
    grad2->dataPoints= zf_grad2.dataPoints ;  /* assign waveform to be zero-filled */
    writeToDisk(grad2->dataPoints, grad2->numPoints, 0, grad2->resolution,
				     grad2->rollOut, grad2->name); 
   

   return(time); 

}


double calc_zfill_gradient2(FLOWCOMP_T *grad0, FLOWCOMP_T  *grad1,
                         GENERIC_GRADIENT_T  *grad2)

{ 
   
   double time    = 0.0;                    /* longest duration of gradient */
   double duration0, duration1, duration2;  /* gradient durations */
   
   ZERO_FILL_GRADIENT_T zf_grad0,zf_grad1, zf_grad2; /* define zero-fill grad structure in case of flow comp */

   if ((ix > 1) && !sglarray) return(grad0->duration);
   

   /* find longest gradient duration */
   duration0 = grad0->duration;//- grad[0]->tramp; /* duration of a square gradient with same integral & amplitude */
   duration1 = grad1->duration;// - grad[1]->tramp;
   duration2 = grad2->duration;// - grad[2]->tramp;
  
   time = MAX(MAX(duration0,duration1),duration2);  /* should already be granulated */
   text_message("time is %f", time);

/* zerofill new duration for each gradient */

    initZeroFillGradient(&zf_grad0); /* initialize zerofill structure for pe grad */
   // strcpy(zf_grad0.name,"zfgrad0");   /*fill in the unique name, otherwise the name will be repeated for each zerofill pattern*/
     strcpy(zf_grad0.name,"zfgrad0");  
   

    zf_grad0.numPoints= grad0->numPoints ;/* assign number of waveform points */
    zf_grad0.dataPoints = grad0->dataPoints; /* assign waveform to be zero-filled */
    zf_grad0.newDuration= time; /* duration of the fc grad for readout */
    zf_grad0.location = FRONT; /* add zero at front of waveform */
    zeroFillGradient(&zf_grad0);
    if (zf_grad0.error) abort_message("Gradient library error --> Check text window \n");
    //Now put it back
    grad0->duration=time ;
    grad0->numPoints=zf_grad0.numPoints ;/* assign number of waveform points */
    grad0->dataPoints= zf_grad0.dataPoints ;  /* assign waveform to be zero-filled */
    writeToDisk(grad0->dataPoints, grad0->numPoints, 0, grad0->resolution,
				     TRUE /*rolout*/, grad0->name); 
    initZeroFillGradient(&zf_grad1); /* initialize zerofill structure for pe grad */
    strcpy(zf_grad1.name,"zfgrad1");   /*fill in the unique name, otherwise the name will be repeated for each zerofill pattern*/
   

    zf_grad1.numPoints= grad1->numPoints ;/* assign number of waveform points */
    zf_grad1.dataPoints = grad1->dataPoints; /* assign waveform to be zero-filled */
    zf_grad1.newDuration= time; /* duration of the fc grad for readout */
    zf_grad1.location = FRONT; /* add zero at front of waveform */
    zeroFillGradient(&zf_grad1);
    if (zf_grad1.error) abort_message("Gradient library error --> Check text window \n");
    //Now put it back
    grad1->duration=time ;
    grad1->numPoints=zf_grad1.numPoints ;/* assign number of waveform points */
    grad1->dataPoints= zf_grad1.dataPoints ;  /* assign waveform to be zero-filled */
    writeToDisk(grad1->dataPoints, grad1->numPoints, 0, grad1->resolution,
				     grad1->rollOut, grad1->name); 

    initZeroFillGradient(&zf_grad2); /* initialize zerofill structure for pe grad */
    strcpy(zf_grad2.name,"zfgrad2");   /*fill in the unique name, otherwise the name will be repeated for each zerofill pattern*/
    

    zf_grad2.numPoints= grad2->numPoints ;/* assign number of waveform points */
    zf_grad2.dataPoints = grad2->dataPoints; /* assign waveform to be zero-filled */
    zf_grad2.newDuration= time; /* duration of the fc grad for readout */
    zf_grad2.location = FRONT; /* add zero at front of waveform */
    zeroFillGradient(&zf_grad2);
    if (zf_grad2.error) abort_message("Gradient library error --> Check text window \n");
    //Now put it back
    grad2->duration=time ;
    grad2->numPoints=zf_grad2.numPoints ;/* assign number of waveform points */
    grad2->dataPoints= zf_grad2.dataPoints ;  /* assign waveform to be zero-filled */
    writeToDisk(grad2->dataPoints, grad2->numPoints, 0, grad2->resolution,
				     grad2->rollOut, grad2->name); 
   

   return(time); 

}



/**************************************************************************/
/*** RF pulse *************************************************************/
/**************************************************************************/
void init_rf (RF_PULSE_T *rf, char rfName[MAX_STR], double pw, double flip,
              double rof1, double rof2)
   {
   if ((ix > 1) && !sglarray) {
     rf->flip = flip;
     return;
   }
   initRf(rf);
   strcpy(rf->pulseName,rfName);
   strcpy(rf->rfcoil,rfcoil);
   rf->rfDuration = shapelistpw(rfName,pw);  /* Round to 200ns resolution*/
   rf->flip = flip;
   rf->flipmult = 1.0;   /* default to 1, programmer can reassign in sequence */
   rf->rof1 = rof1;
   rf->rof2 = rof2;
   readRfPulse(rf);

   switch (rf->error) {
     case ERR_RF_SHAPE_MISSING:
       sgl_abort_message("ERROR: Can not find RF shape '%s.RF'",rfName);
       break;
     case ERR_RF_HEADER_ENTRIES:
       sgl_abort_message("ERROR rf shape '%s': incorrect header information",rfName);
       break;
   }
   if ((rf->header.rfFraction < 0) || (rf->header.rfFraction > 1))
     sgl_abort_message("ERROR rf shape '%s': RF Fraction must be between 0 and 1",rfName);
   }

void shape_rf (RF_PULSE_T *rf, char rfBase[MAX_STR], char rfName[MAX_STR], double pw, double flip,
              double rof1, double rof2)
   {
   if ((ix > 1) && !sglarray) {
     rf->flip = flip;
     return;
   }
   initRf(rf);
   strcpy(rf->pulseBase,rfBase);
   strcpy(rf->pulseName,rfName);
   strcpy(rf->rfcoil,rfcoil);
   rf->rfDuration = pw;
   rf->flip = flip;
   rf->flipmult = 1.0;
   rf->rof1 = rof1;
   rf->rof2 = rof2;

   /* Generate the pulse shape if rf->pulseName is appropriate */
   genRf(rf);

   /* If the pulse shape has not been generated proceed as for init_rf */
   switch (rf->type) {
     case RF_NULL:
       rf->rfDuration = shapelistpw(rfName,pw);  /* Round to 200ns resolution*/
       readRfPulse(rf);

       /* Set actual bandwidth according to pulse duration */
       if ((FP_LT(rf->flip,FLIPLIMIT_LOW)) || (FP_GT(rf->flip,FLIPLIMIT_HIGH)))
         /* use excitation bandwidth */
         rf->bandwidth = rf->header.bandwidth/rf->rfDuration;
       else
         /* use inversion bandwidth */
         rf->bandwidth = rf->header.inversionBw/rf->rfDuration;
              
       break;
     default:
       /* Even though shape is properly quantized for some reason we need to 
          use shapelistpw to round to 200ns resolution, otherwise duration may 
          be significantly wrong ?? */
       rf->rfDuration = shapelistpw(rf->pulseName,pw);
       break;
   }

   switch (rf->error) {
     case ERR_RF_SHAPE_MISSING:
       sgl_abort_message("ERROR: Can not find RF shape '%s.RF'",rfName);
       break;
     case ERR_RF_HEADER_ENTRIES:
       sgl_abort_message("ERROR rf shape '%s': incorrect header information",rfName);
       break;
   }
   if ((rf->header.rfFraction < 0) || (rf->header.rfFraction > 1))
     sgl_abort_message("ERROR rf shape '%s': RF Fraction must be between 0 and 1",rfName);
   }

void calc_rf (RF_PULSE_T *rf, char VJparam_tpwr[], char VJparam_tpwrf[])
   {
   
   //if ((ix > 1) && !sglarray) return;
   strcpy(rf->param1,VJparam_tpwr);
   strcpy(rf->param2,VJparam_tpwrf);
   calcPower(rf, rf->rfcoil);
   switch (rf->error) {
     case ERR_RF_CALIBRATION_FILE_MISSING:
       sgl_abort_message("ERROR: Can't find a pulsecal file"); 
       break;
     case ERR_RF_COIL: 
       sgl_abort_message("ERROR: rfcoil '%s' not found in pulsecal file",rf->rfcoil); 
       break;
     case ERR_RF_CAL_CASE:
       sgl_abort_message("ERROR rf pulse '%s': Don't know how to calibrate pulse of type '%s'",
         rf->pulseName, rf->header.modulation);
       break;
     case ERR_RFPOWER_COARSE:
       sgl_abort_message("ERROR rf pulse '%s': Coarse power too large (pw = %.2fus, flip = %.2f",
         rf->pulseName, rf->rfDuration*1e6,rf->flip);
   }

   if ((rf->flip >= 0) && (!sglpower) && (ix == 1)) {
     /* return acquisition parameters to VnmrJ space */
     if (strcmp(VJparam_tpwr, ""))
	putvalue(VJparam_tpwr,  rf->powerCoarse);
     if (strcmp(VJparam_tpwrf, ""))
	putvalue(VJparam_tpwrf, rf->powerFine);
   }
      

}

