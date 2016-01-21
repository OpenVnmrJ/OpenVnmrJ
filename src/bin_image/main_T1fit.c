/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*---------------------------------------------------------------------------*/
/* This is free software: you can redistribute it and/or modify              */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* This is distributed in the hope that it will be useful,                   */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* If not, see <http://www.gnu.org/licenses/>.                               */
/*---------------------------------------------------------------------------*/
/**/

#include "utils.h"
#define  NOISE_THR    4   // Threshold for fit (auto-noise seems to overestimate)
#define  FIT_PTS      3   // Minimum number of points to use in fit
#define  FITPARAMS    3   //Fit 3 parameters
#define  MAX_ITER   100   //Max iterations to fit T1
#define  ITER_LIMIT 1e-4  //Limit for stopping fit iterations


typedef struct {
  int    debug;
  char   fit_type[MAXSTR]; // IR, sat, or flip
  char   fit_var[MAXSTR];  // typically ti or flip1
  int    M0;
  float  noise;
  int    pixel;
  int    residual;
  char   outdir[MAXSTR];
  char   indir[MAXSTR];
  int    mask;
  int    fit_params;
} user_input;


struct fitdata {
  double *S;
  double *TI;
  int     n;  // # data points
  int     nP; // fitparams
};

void get_args();
int  T1_f();
int  T1_df();
int  T1_fdf();
void print_state();

char procpar[MAXSTR];


int main(int argc, char *argv[]) {
  double *fitvar;
  int     nelem, nimages, echoes;

  float  **data;
  double *image_arr;
  float  *T1,*S0,*F,*fitmask,*RMSres;
  double  residual,SOSres1,SOSres2;
  double  S[MAXELEM],TI[MAXELEM],Smin,zeroTI,R1=0.0;
  int     Ssize=0;
  
  fdf_header      fdf_hdr;
  char    filename[MAXSTR], basename[MAXSTR], recon[MAXSTR];
  int T1REC=FALSE;
  int T1FLIP=FALSE;

  /* Variables for T1 fitting */
  struct fitdata fitparams = {S, TI, Ssize, FITPARAMS};  
  const gsl_multifit_fdfsolver_type *gsl_fittype;
  gsl_multifit_function_fdf gsl_fitfunc;
  gsl_multifit_fdfsolver *gsl_solver;
  gsl_vector_view gsl_param_est;

  double param_est[FITPARAMS];
  float noise_thr1 = 0, noise_thr2 = 0;

  /* Variables for T1 calculation with 2 flip angles */
  double Sratio,a1=0.0,a2=0.0,TR=0.0,v1,v2;

  // Loop variables
  int slice, image, images;
  int pixel, datapts, spixel;
  int i,fit_iter, fit_status;
  int debug_pixel, debug, pixeldebug;

  /* NULL pointers */
  fitvar=image_arr=NULL;
  data=NULL;
  T1=S0=F=fitmask=RMSres=NULL;
  gsl_fittype=NULL;
  gsl_solver=NULL;

  /* input arguments */
  user_input input = {
    0,          // debug
    "T1_Rec",   // fit_type
    "none",     // fit_var
    FALSE,      // M(0)
    -1,         // noise
    -1,         // pixel
    FALSE,      // residual
    "none",     // outdir 
    ".",        // indir 
    0,          // mask output
    3           // fit_params 3 {M0, M(0), T1} or 2 {M0, T1}
  };

  
  /*******************************************************************/  
  /*** Calculations **************************************************/  
  /*******************************************************************/  

  // Get arguments from commandline
  get_args(&input, argc, argv);
  debug = input.debug;

  /* Set up string for procpar; 
     this is for future expansion, to allow 
       - data to be in arbitrary directory
       - S(0) and S(non-zero)to be in different directories 
  */
  strcpy(procpar,input.indir);
  strcat(procpar,"/procpar");
  if (debug) printf("indir = %s, procpar = %s\n",input.indir,procpar);

  /* Initialize fdf header */
  init_fdf_header(&fdf_hdr);

  if (debug) {
    if (input.pixel < 0)
      debug_pixel = fdf_hdr.datasize/2;
    else
      debug_pixel = input.pixel;
  }
  else debug_pixel = -1;
  if (debug) printf("Debugging for pixel %d\n",debug_pixel);


  //Determine the independent variable
  if (!strcmp(input.fit_var,"none")) { // user hasn't defined it, use defaults
    if (!strcmp(input.fit_type,"T1_Rec"))   strcpy(input.fit_var,"ti");
    if (!strcmp(input.fit_type,"T1_flip")) strcpy(input.fit_var,"flip1");
  }

  if (!strcmp(input.fit_var,"none")) { //fit_var wasn't set => unknown fit_type
    printf("Fit function %s is not implemented\n",input.fit_type);
    exit(0);
  }

  if (debug) printf("Fitting %s using variable %s\n",input.fit_type,input.fit_var);

  // Determine the number of images
  nelem = getstatus(input.fit_var);
  if (nelem == 0) {
    exit(0);  // error message is printed by getstatus
  }

  // Read independent variable
  if ((fitvar = (double *) malloc(sizeof(double)*nelem)) == NULL) nomem();
  getarray(input.fit_var,fitvar);

  /* Is this an epi scan with reference scan(s)? */
  images = getstatus("image");
  if (images == nelem) { // assume image is arrayed in parallel with the variable
    //find out how many ref scans
    if ((image_arr = (double *) malloc(sizeof(double)*images)) == NULL) nomem();
    getarray("image",image_arr);
    nimages = 0;
    for (i = 0; i < images; i++)
      if (image_arr[i] == 1) {
        fitvar[nimages]=fitvar[i];
        nimages++;
      }
    free(image_arr);
    image_arr=NULL;
  }
  else nimages = nelem;

  if (debug) printf("nelem, nimages, images = %d, %d, %d\n",nelem, nimages, images);

  if (nimages < input.fit_params) {
    printf("Must have at least %d values of %s for the %s fit\n",input.fit_params,input.fit_var,input.fit_type);
    exit(0);
  }

  if (!strcmp(input.fit_type,"T1_Rec")) {
    T1REC=TRUE;
    if (input.fit_params<3) input.M0=FALSE;
    /* There's no point output residual if there's not enough values */
    if (nimages < input.fit_params+1) input.residual=FALSE;  

    Ssize = nimages;
    fitparams.nP = input.fit_params;

    gsl_fitfunc.f   = &T1_f;
    gsl_fitfunc.df  = &T1_df;
    gsl_fitfunc.fdf = &T1_fdf;
    gsl_fitfunc.n   = nimages;
    gsl_fitfunc.p   = input.fit_params;
    gsl_fitfunc.params = &fitparams;

    gsl_fittype = gsl_multifit_fdfsolver_lmsder;
  }
  else if (!strcmp(input.fit_type,"T1_flip")){
    T1FLIP=TRUE;
    input.M0=FALSE;
    input.residual=FALSE;
    TR = getval("tr");
    a1 = fitvar[0];  //angle 1
    a2 = fitvar[1];  //angle 2
  }
  else {
    printf("Unknown fit type %s\n",input.fit_type);
    exit(0);
  }
  
  /* Allocate memory for all images */
  datapts = fdf_hdr.datasize;
  if ((data = (float **) malloc(nimages*sizeof(float *))) == NULL) nomem();  
  for (image = 0; image < nimages; image++) {
    if ((data[image] = (float *)malloc(datapts*sizeof(float))) == NULL) nomem();
  }
  if ((T1  = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();
  if ((S0  = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();
  if (T1REC) {
    if ((F   = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();
    if ((RMSres = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();
  }
  if (input.mask) {
    if ((fitmask = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();
  }

  echoes = fdf_hdr.echoes;
  
  /* Create directory to put output in */
  if (!strcmp(input.outdir,"none")) { // user hasn't defined it, use defaults
    if (!strcmp(input.fit_type,"T1_Rec"))    strcpy(input.outdir,"T1_Rec");
    if (!strcmp(input.fit_type,"T1_flip"))  strcpy(input.outdir,"T1_flip");
    strcat(input.outdir,".img");
  }
  if (strcmp(input.outdir, "./")) {
    if (debug) printf("Making directory %s\n",input.outdir);
    mkdir(input.outdir,0777);
  }

  /* Create subdirectories for FDF data */
  createdir("T1.img",input.outdir);
  createdir("S0.img",input.outdir);
  if (input.M0) createdir("M0.img",input.outdir);
  if (input.residual) createdir("RMSres.img",input.outdir);
  if (input.mask) createdir("Mask.img",input.outdir);

  //Set prefix for fdf file name
  if (fdf_hdr.rank == 2) strcpy(basename,"slice");
  if (fdf_hdr.rank == 3) {
    strcpy(recon,"");
    if (getstatus("recon")>0) {
      getstr("recon",recon);
      if (!strcmp(recon,"") || !strcmp(recon,"internal")) strcpy(basename,"img_slab");
      else strcpy(basename,"slab");
    }
    else strcpy(basename,"img_slab");
  }

  /*******************************************************************/  
  /*** Slice by slice calculations ***********************************/  
  /*******************************************************************/  
  for (slice = 0; slice < fdf_hdr.slices; slice++) {
//  for (slice = 0; slice < 1; slice++) { // For debugging purposes
    for (image = 0; image < nimages; image++) {
      /* Read all images for this slice */
      if (echoes == 1)
        sprintf(filename,"%s/%s%03dimage%03decho001.fdf",input.indir,basename,slice+1,image+1);
      else
        sprintf(filename,"%s/%s%03dimage001echo%03d.fdf",input.indir,basename,slice+1,image+1);
      if (debug) printf("Read file %s\r",filename),fflush(stdout);
      read_fdf_data(filename,data[image],&fdf_hdr);
    } // end read image loop
    if (debug) printf("\n");    

    // Noise threshold - user input or based on histogram
    if (input.noise >= 0) {
      noise_thr1 = noise_thr2 = input.noise;
    }
    else { // Get noise from the first or last image (whichever has greatest signal)
      noise_thr1 = threshold(data[0],&fdf_hdr);
      noise_thr2 = threshold(data[nimages-1],&fdf_hdr);
      if (noise_thr2>noise_thr1) noise_thr1=noise_thr2;
      // If vnmruser environment exists output threshold
      val2file("aipThreshold",(double)noise_thr1,FLT32);
      noise_thr2 = noise_thr1/NOISE_THR;
    }

    if (debug) printf("noise level %f, %f\n",noise_thr1,noise_thr2);

    /*******************************************************************/  
    /*** Pixel loop ****************************************************/  
    /*******************************************************************/  
    for (pixel = 0; pixel < datapts; pixel++) {
      spixel = slice*datapts + pixel;
      T1[spixel] = S0[spixel] = F[spixel] = 0.0;
      RMSres[spixel] = 0.0;
      if (input.mask) fitmask[spixel] = 0.0;

      if (T1REC) {
	Ssize = 0;
	Smin  = 1e6; zeroTI = 0;
	for (image = 0; image < nimages; image++) {
          if (data[0][pixel] > noise_thr1) {
            S[image]  = (double)data[image][pixel];
	    TI[image] = (double)fitvar[image];
	    Ssize++;
	    if (S[image] < Smin) { zeroTI = TI[image]; Smin = S[image]; }
	  }
	}

	/*******************************************************************/  
	/*** T1 FIT ********************************************************/  
	/*******************************************************************/  
	if (Ssize >= input.fit_params) {

          if (pixel == debug_pixel) pixeldebug=TRUE; else pixeldebug=FALSE;

          //Reverse signs for signals that are earlier than minimum
          if (pixeldebug) printf("Minimum signal at TI = %.3f\n",zeroTI);
          for (image = 0; image < nimages; image++) {
            if (pixeldebug) printf("S[%.2f] = %g => ",TI[image],S[image]);
            if (TI[image] < zeroTI) S[image] = -S[image];
            if (pixeldebug) printf("%g\n",S[image]);
          }

	  //Starting estimates for fit parameters
	  param_est[0] = S[Ssize-1]; // M0
	  if (input.fit_params == 2) {
	    param_est[1] = 1; // T1
	  }
	  else {
	    param_est[1] = 1.5; // F (1 for sat exp, 2 for IR exp)
	    param_est[2] = 1; // T1
	  }
	  gsl_param_est = gsl_vector_view_array(param_est, input.fit_params);     

	  fitparams.n = gsl_fitfunc.n = Ssize;

	  gsl_solver  = gsl_multifit_fdfsolver_alloc(gsl_fittype, Ssize, input.fit_params);
	  gsl_multifit_fdfsolver_set(gsl_solver, &gsl_fitfunc, &gsl_param_est.vector);

	  //Iterate to fit T1
	  fit_iter = 0;
	  do {
	    fit_iter++;
	    fit_status = gsl_multifit_fdfsolver_iterate(gsl_solver);
	    if (pixeldebug) print_state(fit_iter, gsl_solver,input.fit_params);
	    if (fit_status) break;
	    fit_status = gsl_multifit_test_delta(gsl_solver->dx, gsl_solver->x, ITER_LIMIT, ITER_LIMIT);
	  } while (fit_status == GSL_CONTINUE && fit_iter < MAX_ITER);

	  //Assign output images
	  S0[spixel] = (float)gsl_vector_get(gsl_solver->x,0);
          if (input.fit_params == 3) {
            R1 = gsl_vector_get(gsl_solver->x,2);
	    F[spixel] = (float)gsl_vector_get(gsl_solver->x,1);
	  }
	  else
            R1 = gsl_vector_get(gsl_solver->x,1);

          if (pixeldebug) { // print output
            printf("S0 = %.5f\n", gsl_vector_get(gsl_solver->x, 0));
            printf("T1 = %.5f\n", (R1 == 0) ? 0 : 1.0/R1);
            if (input.fit_params == 3) {
              printf ("F = %.5f\n", gsl_vector_get(gsl_solver->x, 1));
            }
          }

          if (Ssize > input.fit_params) {  // If residuals can be analyzed

            // Store sum of squares of residuals
            SOSres1=0.0;
            for (i=0;i<gsl_fitfunc.n;i++) {
              residual = gsl_vector_get(gsl_solver->f,i);
              SOSres1 += residual*residual;
            }

            if (pixeldebug) { // print output
              printf("Sum of squares of residuals = %g\n",SOSres1);
            }

            // Reverse sign of minimum signal to check for better fit
            for (image = 0; image < nimages; image++) if (TI[image] == zeroTI) break;
            S[image]=-S[image];
            if (pixeldebug) {
              printf ("Checking for better fit ...\n");
              printf("Minimum signal at TI = %.3f\n",zeroTI);
	      for (image = 0; image < nimages; image++)
	        printf("S[%.2f] = %g => %g\n",TI[image],fabs(S[image]),S[image]);
            }

            gsl_multifit_fdfsolver_set(gsl_solver, &gsl_fitfunc, &gsl_param_est.vector);

            // Iterate to fit T1
            fit_iter = 0;
            do {
              fit_iter++;
              fit_status = gsl_multifit_fdfsolver_iterate(gsl_solver);
              if (pixeldebug) print_state(fit_iter, gsl_solver,input.fit_params);
              if (fit_status) break;
              fit_status = gsl_multifit_test_delta(gsl_solver->dx, gsl_solver->x, ITER_LIMIT, ITER_LIMIT);
            } while (fit_status == GSL_CONTINUE && fit_iter < MAX_ITER);

            // Sum of squares of residuals
            SOSres2=0.0;
            for (i=0;i<gsl_fitfunc.n;i++) {
              residual = gsl_vector_get(gsl_solver->f,i);
              SOSres2 += residual*residual;
            }

            if (pixeldebug) { // print output
              printf("S0 = %.5f\n", gsl_vector_get(gsl_solver->x, 0));
              R1 = gsl_vector_get(gsl_solver->x,2);
              printf("T1 = %.5f\n", (R1 == 0) ? 0 : 1.0/R1);
              if (input.fit_params == 3) {
                printf ("F = %.5f\n", gsl_vector_get(gsl_solver->x, 1));
              }
              printf("Sum of squares of residuals = %g\n",SOSres2);
              if (SOSres2<SOSres1)
                printf("Using second fit\n");
              else
                printf("Using original fit\n");
            }

            // Use second fit if it is better
            if (SOSres2<SOSres1) {
              // Assign output images
              S0[spixel] = (float)gsl_vector_get(gsl_solver->x,0);
              if (input.fit_params == 3) {
                R1 = gsl_vector_get(gsl_solver->x,2);
                F[spixel] = (float)gsl_vector_get(gsl_solver->x,1);
              }
              else
                R1 = gsl_vector_get(gsl_solver->x,1);
              SOSres1=SOSres2;
            }

            if (input.residual) {
              SOSres1 /= gsl_fitfunc.n;
              RMSres[spixel] = (float)sqrt(SOSres1);
            }

          }  // end Ssize > fit_params

	  if (R1 > 1.0/100.0) //limit reasonable T1 values to < 100
            T1[spixel] = (float)(1.0/R1);

	  if (input.M0) {
            F[spixel] = (float)fabs((double)(S0[spixel]*(1-F[spixel])));
          }

          gsl_multifit_fdfsolver_free(gsl_solver);
	} // end Ssize >= fit_params 

	//mask to show where all pts are used
        if (input.mask) fitmask[spixel] = Ssize;

      } // end if T1_Rec

      else if (T1FLIP) {
        if (data[0][pixel] > noise_thr1) {
          //Check if nimages > 2?
	  if (nimages == 2) {
	    Sratio = (double) (data[0][pixel]/data[1][pixel]);
	    v1 = Sratio*sin(a2)*cos(a1)-sin(a1)*cos(a2);
	    v2 = Sratio*sin(a2)-sin(a1);
	    if ((v2 != 0) && (v1/v2 > 0))
  	      R1 = 1/TR*log(v1/v2);
	    else
	      R1 = 1e-6;  //dummy value, will not be used
	  }
	  else {
	    //What to do here?
	  }
	  //Assign output
          S0[spixel]  = data[0][pixel];
          if (R1 > 1.0/100.0) //limit reasonable T1 values to < 100
            T1[spixel] = 1.0/R1;
	}
	
      } // end if T1_flip

    } // end pixel loop    

    /* Write Parametric maps: */
    fdf_hdr.array_dim     = 1;
    fdf_hdr.array_index   = 1;
    fdf_hdr.echoes        = 1;
    fdf_hdr.echo_no       = 1;
    fdf_hdr.slice_no      = slice + 1;
    fdf_hdr.display_order = slice;
    fdf_hdr.pss           = (float) fdf_hdr.pss_array[slice];

    sprintf(filename,"%s/T1.img/T1_%03d.fdf",input.outdir,slice+1);
    write_fdf(filename,&T1[slice*datapts],&fdf_hdr);

    sprintf(filename,"%s/S0.img/S0_%03d.fdf",input.outdir,slice+1);
    write_fdf(filename,&S0[slice*datapts],&fdf_hdr);

    if (input.M0) {
      sprintf(filename,"%s/M0.img/M0_%03d.fdf",input.outdir,slice+1);
      write_fdf(filename,&F[slice*datapts],&fdf_hdr);
    }

    if (input.residual) {
      sprintf(filename,"%s/RMSres.img/RMSres_%03d.fdf",input.outdir,slice+1);
      write_fdf(filename,&RMSres[slice*datapts],&fdf_hdr);
    }

    if (input.mask) {
      sprintf(filename,"%s/Mask.img/mask_%03d.fdf",input.outdir,slice+1);
      write_fdf(filename,&fitmask[slice*datapts],&fdf_hdr);
    }

  } // end slice loop

}


/*******************************************************************/  
/*** Functions for fitting to T1 function **************************/  
/*******************************************************************/  
int T1_f(const gsl_vector *x, void *data, gsl_vector *f) {
  int     n  = ((struct fitdata *)data)->n;
  double *S  = ((struct fitdata *)data)->S;
  double *TI = ((struct fitdata *)data)->TI;

  /* Model S = S0*(1-F*exp(-TI*R1))
       - for IR exp, F should be 2
       - for sat exp, F should be 1
   */

  double S0,F,R1;
  int i;
  double Scalc,e;

  S0 = gsl_vector_get(x,0);
  F  = gsl_vector_get(x,1);
  R1 = gsl_vector_get(x,2);
       
  for (i = 0; i < n; i++) {
    e = exp(-TI[i]*R1);
    Scalc = S0*(1-F*e);
    gsl_vector_set(f, i, Scalc - S[i]);
  }
  return GSL_SUCCESS;
}


int T1_df(const gsl_vector * x, void *data, gsl_matrix * J) {
  int     n  = ((struct fitdata *)data)->n;
  double *TI = ((struct fitdata *)data)->TI;

  double S0,F,R1;
  int i;
  double e;

  S0 = gsl_vector_get(x,0);
  F  = gsl_vector_get(x,1);
  R1 = gsl_vector_get(x,2);

  for (i = 0; i < n; i++) {
      e = exp(-TI[i]*R1);
      gsl_matrix_set(J,i,0, 1-F*e);
      gsl_matrix_set(J,i,1, -S0*e); 
      gsl_matrix_set(J,i,2, F*S0*e*TI[i]);
  }
  return GSL_SUCCESS;
}


int T1_fdf(const gsl_vector *x, void *data, gsl_vector *f, gsl_matrix *J) {
  T1_f (x,data,f);
  T1_df(x,data,J);
  return GSL_SUCCESS;
}


void print_state(int iter, gsl_multifit_fdfsolver *s, int nP) {
  if (nP == 3) 
    printf("ITERATION %d: %.8f %.8f %.2f\n",iter,
	gsl_vector_get(s->x,0),
	gsl_vector_get(s->x,1),
      1/gsl_vector_get(s->x,2)
    );
  else
    printf("ITERATION %d: %.8f %.2f\n",iter,
	gsl_vector_get(s->x,0),
      1/gsl_vector_get(s->x,1)
    );
}


/*******************************************************************/  
/*** Get arguments from commandline ********************************/  
/*******************************************************************/  
void get_args(user_input *input, int argc, char *argv[]) {
  int n;
  
  if ((argc == 2) && (!strncmp (argv[1], "-help", 2))) {
    fprintf(stderr, "Usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "-fittype      Type of fit (T1_Rec or T1_flip)\n");
    fprintf(stderr, "-fitvar       Name of parameter (e.g., ti or flip1)\n");
//    fprintf(stderr, "-fitparam     Number of fit parameters (2 or 3)\n");
    fprintf(stderr, "-noise N      Use noise level N\n");
    fprintf(stderr, "-outdir dir   Use this directory for output (default 'T2' or 'ADC')\n");
    fprintf(stderr, "-indir dir    Use this directory for input (default '.')\n");
    fprintf(stderr, "-debug        Print debug messages\n");
    fprintf(stderr, "-mask         Output mask image\n");
    fprintf(stderr, "-pixel N      Print debug messages for pixel N\n");
    fprintf(stderr, "\n");
    exit(1);
  }
  
  for (n = 1; n < argc; n++) {

    if ((strncmp(argv[n],"-debug", 2) == 0)||(strncmp(argv[n],"-verbose", 2) == 0))
      input->debug = 1;

    /***** Fit type ****************************/
    else if (strncmp(argv[n],"-fittype", 5) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a string after -fittype!\n");
	exit(1);
      }
      strcpy(input->fit_type, argv[++n]);
    }

    /***** Fit variable*************************/
    else if (strncmp(argv[n],"-fitvar", 5) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a string after -fitvar!\n");
	exit(1);
      }
      strcpy(input->fit_var, argv[++n]);
    }

    /***** Fit params **************************/
    else if (strncmp(argv[n],"-fitparam", 5) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a number after -fitparam!\n");
	exit(1);
      }
      input->fit_params = atoi(argv[++n]);
    }

    /***** M(0) *********************/
    else if (strncmp(argv[n],"-M0", 3) == 0)
      input->M0 = TRUE;

    /***** noise ********************/
    else if (strncmp(argv[n],"-noise", 2) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a number after -noise!\n");
	exit(1);
      }
      input->noise = (float) atof(argv[++n]);
      if (input->noise < 0) {
	fprintf(stderr, "noise must be > 0!\n");
	exit(1);
      }
    }

    /***** pixel ********************/
    else if (strncmp(argv[n],"-pixel", 2) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a number after -pixel!\n");
	exit(1);
      }
      input->pixel = atoi(argv[++n]);
      if (input->pixel < 0) {
	fprintf(stderr, "pixel must be > 0!\n");
	exit(1);
      }
    }

    /***** output residuals ********************/
    else if (strncmp(argv[n],"-residual", 4) == 0) {
      input->residual=TRUE;
    }

    /***** output directory ********************/
    else if (strncmp(argv[n],"-outdir", 4) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a directory after -outdir!\n");
	exit(1);
      }
      strcpy(input->outdir, argv[++n]);
    }

    /***** input directory *********************/
    else if (strncmp(argv[n],"-indir", 3) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a directory after -indir!\n");
	exit(1);
      }
      strcpy(input->indir, argv[++n]);
    }

    /***** mask *********************/
    else if (strncmp(argv[n],"-mask", 2) == 0)
      input->mask = 1;

    /***** unknown ******************/
    else {
      fprintf(stderr, "Unknown argument: %s\n", argv[n]);
      exit(1);
    }

  }

}
