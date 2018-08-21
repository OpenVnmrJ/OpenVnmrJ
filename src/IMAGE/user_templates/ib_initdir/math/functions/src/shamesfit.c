/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef FUNCSELECTION
IF_FITCODE("shames2"){
    N_PARAMETERS = 2;
    FUNCTION = shames_function;
    switch (nbr_image_vecs){
      case 1:
	FIT_TYPE = LINEAR_FIXED;
	break;
      case 2:
	FIT_TYPE = LINEAR_RECALC_OFFSET;
	break;
      default:
	FIT_TYPE = LINEAR_RECALC;
	break;
    }

    if (nbr_params > 0) alpha = in_params[0];
    if (nbr_params > 1) sbv0 = in_params[1];
    if (nbr_params > 2) hct = in_params[2];
    return TRUE;
}
#else /* not FUNCSELECTION */

/* Constants for Shames model */
static double alpha = 0.136;	/* Time constant for [CA-plasma]-signal decay */
static double sbv0 = 0.13;	/* Initial value of [CA-plasma]-signal */
static double hct = 0.37;	/* Hematocrit */

/*-------------------------------------------------------------------------
   shames_function: function defining signal vs. time according to the
	 	    basic Demsar/Shames model.  

         Here the parameters: 
			alpha, sbv0, hct
         have default values that can be overridden by putting
	 additional constants in the fit() call in Image Math.

      Notes: (1) In comments, [] in comments indicates "concentration of" 
		 and CA stands for contrast agent

	     (2) It is assumed that the signals dealt with are directly 
		 proportional to [CA]

	     (3) This model is *linear* in the fitted parameters. 
-------------------------------------------------------------------------*/
static void
shames_function(int npts,	/* Nbr of data points */
		int npars,	/* Nbr of parameters--NOT USED */
		double *p,	/* npars parameter values */
		int nvars,	/* Number of independent variables--NOT USED */
		double *x,	/* npts*nvars values of indep variables */
		double *y)	/* Function values OUT */
{
    int i;
    double offset;
    double recip;		/* Stores reciprocal of (1 - Hematocrit) */
    double decay;		/* Temporary storage for exp(-alpha*t) */
    double plasma_sig;		/* Signal from plasma CA */
    double ees_sig;		/* Signal from extra-vascular,-cellular space */

    /* 
     * Fitted model parameters: 
     *    p[0] := bv := fraction of tissue that is blood (unitless)
     *    p[1] := ps := permeabiity surface-area product (1/time)
     */
    offset = 0;
    if (nbr_image_vecs > 1){
	offset = IN_DATA(1, 0, pixel_indx);
	if (nbr_image_vecs > 2){
	    alpha = IN_DATA(2, 0, pixel_indx);
	    if (nbr_image_vecs > 3){
		sbv0 = IN_DATA(3, 0, pixel_indx);
		if (nbr_image_vecs > 4){
		    hct = IN_DATA(4, 0, pixel_indx);
		}
	    }
	}
    }
    recip = 1 /( 1 - hct);    
    for (i=0; i<npts; i++){
	decay = exp( -alpha * x[i] );
	plasma_sig = p[0] * sbv0 * decay;
	ees_sig = p[1] * recip * sbv0 * (1 - decay) / alpha;
	y[i] = plasma_sig + ees_sig + offset;
    }
}

#endif /* not FUNCSELECTION */
