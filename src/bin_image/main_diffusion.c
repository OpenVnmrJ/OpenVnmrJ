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
#define  PGM "main_diffusion"

typedef struct {
  int    sort;
  int    debug;
  float  noise;
  float  bvalue;
  int    pixel;
  int    residual;
  char   outdir[MAXSTR];
  char   indir[MAXSTR];
  int    mask;
} user_input;

// Functions specific to DTI processing
void get_args();
void print_usage();
int check_directions();

char procpar[MAXSTR];


/*******************************************************************/  
/* MAIN PROGRAM ****************************************************/
/*******************************************************************/  

int main(int argc, char *argv[]) {
  int     do_tensor=0,do_trace=0,do_adc=0,do_avadc=0;
  double *dro, *dpe, *dsl;
  double *bvalrr, *bvalpp, *bvalss, *bvalrp, *bvalrs, *bvalsp, 
         *bvalue, dw_bval, *image_arr;
  double  btmp, sumsq;
  int     ndir, ndir_ro, ndir_pe, ndir_sl, ndir_noncl, nfit=0;
  int    *group,**dgroup,**bgroup,g,ngroups=0;

  float  **data;
  float  *FA, *TR, *S0, *DW,
         *ADC1, *ADC2, *ADC3, *X1, *Y1, *Z1;
  float  *RMSres,*fitmask;
  double  residual,SOSres;
  float   e1=0.0,e2=0.0,e3=0.0,trace;
  int     nmaps=0;
  double  DDa, DD;
  
  fdf_header      fdf_hdr;
  char    filename[MAXSTR], basename[MAXSTR], recon[MAXSTR];

  // Variables for GSL functions
  gsl_matrix      *Tensor;       // 3x3 tensor
  gsl_vector      *FitElem;      // tensor elements arranged in vector
  gsl_matrix      *Bmatrix;      // ndir x 7 bvalues
  gsl_vector      *S;            // vector of signal intensities
  gsl_vector      *eigval;       // Eigen values
  gsl_matrix      *eigvec;       // Eigen vectors
  gsl_vector      *tau;          // Used by QR decomposition
  gsl_vector      *res;          // Residuals from solving linear system
  gsl_eigen_symmv_workspace *gsl_wsp = gsl_eigen_symmv_alloc(3);
                                 // Used by eigensystem calculations

  int    Ssize;

  float noise_thr = 0;

  // Loop variables
  int slice, image, images;
  int pixel, datapts, spixel;
  int r,c,i,gi,inx;
  int debug_pixel, debug, pixeldebug;

  /* NULL pointers */
  dro= dpe=dsl=bvalrr=bvalpp=bvalss=bvalrp=bvalrs=bvalsp=bvalue=image_arr=NULL;
  group=NULL;
  dgroup=bgroup=NULL;
  data=NULL;
  FA=TR=S0=DW=ADC1=ADC2=ADC3=X1=Y1=Z1=fitmask=RMSres=NULL;
  Tensor=Bmatrix=eigvec=NULL;
  FitElem=S=eigval=tau=res=NULL;

  // input arguments
  user_input input = {
    1,            // sort by direction
    0,            // debug
    -1,           // noise
    -1,           // bvalue
    -1,           // pixel
    FALSE,        // residual
    "./diffcalc", // outdir 
    ".",          // indir 
    0             // mask output
  };

  
  /*******************************************************************/  
  /*** Initializations and mallocs ***********************************/  
  /*******************************************************************/  
  // Get arguments from commandline
  get_args(&input, argc, argv);
  debug = input.debug;

  // Set up string for procpar
  strcpy(procpar,input.indir);
  strcat(procpar,"/procpar");
  if (debug) printf("indir = %s, procpar = %s\n",input.indir,procpar);

  // Initialize fdf header
  init_fdf_header(&fdf_hdr);

  if (debug) {
    if (input.pixel < 0) 
      debug_pixel = fdf_hdr.datasize/2;
    else
      debug_pixel = input.pixel;
  }
  else debug_pixel = -1;
  if (debug) printf("Debugging for pixel %d\n",debug_pixel);

  // Determine the number of directions
  ndir_ro = getstatus("dro");
  ndir_pe = getstatus("dpe");
  ndir_sl = getstatus("dsl");
  ndir = max(max(ndir_ro,ndir_pe),ndir_sl);
  if (ndir == 0) exit(0);  // error message is printed by getstatus

  // Check that dro, dpe, and dsl are arrayed in parallel
  if   (((ndir_ro > 1) && (ndir_ro != ndir))
     || ((ndir_pe > 1) && (ndir_pe != ndir))
     || ((ndir_sl > 1) && (ndir_sl != ndir))) {
    printf("All diffusion factors must have the same number of elements (%d, %d, %d)\n",
       ndir_ro,ndir_pe,ndir_sl);
    exit(0);
  }
  
  // Read dro, dpe, dsl
  if ((dro = (double *) malloc(sizeof(double)*ndir)) == NULL) nomem();
  if ((dpe = (double *) malloc(sizeof(double)*ndir)) == NULL) nomem();
  if ((dsl = (double *) malloc(sizeof(double)*ndir)) == NULL) nomem();
  getarray("dro",dro);
  getarray("dpe",dpe);
  getarray("dsl",dsl);

  // Zero elements for non-arrayed diffusion factors
  if (ndir_ro == 1) for (r = 1; r < ndir; r++) dro[r] = 0;
  if (ndir_pe == 1) for (r = 1; r < ndir; r++) dpe[r] = 0;
  if (ndir_sl == 1) for (r = 1; r < ndir; r++) dsl[r] = 0;
 
  // Read bvalue matrix elements
  if ((bvalrr = (double *) malloc(sizeof(double)*ndir)) == NULL) nomem();
  if ((bvalpp = (double *) malloc(sizeof(double)*ndir)) == NULL) nomem();
  if ((bvalss = (double *) malloc(sizeof(double)*ndir)) == NULL) nomem();
  if ((bvalrp = (double *) malloc(sizeof(double)*ndir)) == NULL) nomem();
  if ((bvalrs = (double *) malloc(sizeof(double)*ndir)) == NULL) nomem();
  if ((bvalsp = (double *) malloc(sizeof(double)*ndir)) == NULL) nomem();
  if ((bvalue = (double *) malloc(sizeof(double)*ndir)) == NULL) nomem();

  getarray("bvalue",bvalue);
  if (getstatus("bvalrr") > 0) {
    getarray("bvalrr",bvalrr);
    getarray("bvalpp",bvalpp);
    getarray("bvalss",bvalss);
    getarray("bvalrp",bvalrp);
    getarray("bvalrs",bvalrs);
    getarray("bvalsp",bvalsp);
  }
  else {
    // bvalXX parameters don't exist, use dro, dpe, dsl & bvalue instead
    for (r = 0; r < ndir; r++) {
      sumsq = dro[r]*dro[r]+dpe[r]*dpe[r]+dsl[r]*dsl[r];
      if (sumsq == 0) btmp = 0;
      else            btmp = bvalue[r]/sumsq;
      bvalrr[r] = btmp*dro[r]*dro[r];
      bvalpp[r] = btmp*dpe[r]*dpe[r];
      bvalss[r] = btmp*dsl[r]*dsl[r];
      bvalrp[r] = btmp*dro[r]*dpe[r];
      bvalrs[r] = btmp*dro[r]*dsl[r];
      bvalsp[r] = btmp*dsl[r]*dpe[r];
    }
  }

  if (debug) {
    printf("b = [\n");
    for (g = 0; g < ndir; g++) {
      printf("%.2f\n",bvalue[g]);
    }
    printf("];\n");
  }

  // get max bvalue; will be used for DW calculation
  if (input.bvalue > 0) 
    dw_bval = input.bvalue;
  else
    dw_bval = getval("max_bval");

  
  /* Allocate memory for all images
     This is a worst-case scenario, where all images are along
     the same direction, but memory is cheap... */
  datapts = fdf_hdr.datasize;
  if ((data = (float **) malloc(sizeof(float *)*ndir)) == NULL) nomem();
  for (image = 0; image < ndir; image++) {
    if ((data[image] = (float *)malloc(datapts*sizeof(float))) == NULL) nomem();
  }


  /*******************************************************************/  
  /* Sort images and bvalues according to direction ******************/  
  /*******************************************************************/  
  // group is used to keep track of which images 
  // have diffusion along the same direction
  if ((group = (int *) malloc(sizeof(int)*ndir)) == NULL) nomem();
  switch (input.sort) {
    case 0: // treat each image as separate direction, even if they're
            // actually the same; allows you to compare ADCs at different b
      g = 1;
      for (r=0; r<ndir; r++) {
	if ((dro[r] == 0) && (dpe[r] == 0) && (dsl[r] == 0))
          group[r] = 0;
	else {
          group[r] = g; g++;
	}
      }
      ngroups = g-1;
      nfit    = 2;           // ADC + S0
      nmaps   = ngroups + 1; // One ADC map per direction + S0
      do_adc  = 1;
      do_avadc = 1;
      do_trace = do_tensor = 0;
      break;
    case 1: // sort images by direction
            // the sorting is done by the check_directions function

      // Check how many non-colinear directions
      ndir_noncl = check_directions(dro,dpe,dsl,ndir,group);
      if (debug) printf("Got %d non-colinear directions (excl. b=0)\n",ndir_noncl);
      do_tensor = (ndir_noncl >= 6) *(ndir >= 7);  
      do_trace  = (ndir_noncl == -3)*(ndir >= 4);
      do_adc    = (!(do_tensor + do_trace));

      /* Note, that even having 6 non-colinear directions is not 
	 enough to ensure that we can calculate the tensor
	 but further checks are left up to the user in 
	 setting up the experiment.
	 See, e.g., Ozcan A, JMR 2005, 172(2), 238-41)
      */  

      if (do_tensor) { 
	nfit    = 7;  // 6 Tensor elements + S0
	ngroups = 1;  // Use all directions in a single calculation
        nmaps   = 10; // FA, TR, S0, DW, ADC1, ADC2, ADC3, X, Y, Z
	for (r=0; r<ndir; r++) {
          if (group[r] != 0) group[r] = 1;
	}
      }
      if (do_trace)  {
	nfit    = 4;  // 3 ADC values + S0
	ngroups = 1;  // Use all directions in a single calculation
        nmaps   = 6;  // TR, S0, DW, ADC1, ADC2, ADC3
	for (r=0; r<ndir; r++) {
          if (group[r] != 0) group[r] = 1;
	}
      }
      if (do_adc)    {
	nfit    = 2;  // ADC + S0
	ngroups = ndir_noncl;
        nmaps   = ngroups + 1; // One ADC map per direction + S0
        do_avadc = 1;
      }

      break;
    case 2: // treat all directions as one, even if they're not
            // allows you to get an average ADC over many directions
      g = 1;
      for (r=0; r<ndir; r++) {
	if ((dro[r] == 0) && (dpe[r] == 0) && (dsl[r] == 0))
          group[r] = 0;
	else {
          group[r] = 1;
	}
      }
      ngroups = 1;
      nfit    = 2;    // ADC + S0
      nmaps   = 2;    // ADC + S0
      do_adc  = 1;
      do_avadc = 0;
      do_trace = do_tensor = 0;
      break;
  }  // end switch
  if (debug) printf("Tensor/Trace/ADC/meanADC? %d/%d/%d/%d\n",do_tensor,do_trace,do_adc,do_avadc);

  // Is this an epi scan with reference scan(s)?
  images = getstatus("image");
  if (debug) printf("ndir, images = %d, %d\n",ndir, images);
  if ((image_arr = (double *) malloc(sizeof(double)*ndir)) == NULL) nomem();

  if (images == ndir) { // assume image is arrayed in parallel with dro/dpe/dsl
    getarray("image",image_arr);
    for (i = 0; i < images; i++)
      if (image_arr[i] != 1) group[i] = -1; // don't use this for analysis
  }

  // dgroup keeps track of fdf image numbers for each direction
  // bgroup keeps track of bvalXX indices for each direction
  if ((dgroup = (int **) malloc(sizeof(int *)*(ngroups+1))) == NULL) nomem();
  if ((bgroup = (int **) malloc(sizeof(int *)*(ngroups+1))) == NULL) nomem();

  inx=0;
  for (g = 0; g <= ngroups; g++) {
    // count how many images in this group
    r = 0;
    for (image = 0; image < ndir; image++) {
      if (group[image] == g) { 
        r++;
      }
    }
    // Group 0 (g=0) is reserved for b=0 so dgroup/bgroup must be set even if r=0
    if (r>0 || g==0) {
      if ((dgroup[inx] = (int *) malloc(sizeof(int)*(r+1))) == NULL) nomem();
      if ((bgroup[inx] = (int *) malloc(sizeof(int)*(r+1))) == NULL) nomem();
      // then fill in the image numbers in the tables
      dgroup[inx][0] = r;
      bgroup[inx][0] = r;
      r = 1; // here, r is index into dgroup/bgroup
      i = 1; // here, i is the number of the fdf image
      for (image = 0; image < ndir; image++) {
        if (group[image] == g) { 
          dgroup[inx][r] = i;
          bgroup[inx][r] = image;
          r++;
        }
        if (group[image] >= 0) i++;
      }
      inx++;
    }
  }

  ngroups=inx-1;
  switch (input.sort) {
    case 0:
      if (dgroup[0][0]<2) input.residual=FALSE;
      break;
    default:
      if (dgroup[0][0]+dgroup[1][0] <= nfit) input.residual=FALSE;
      break;
  }

  if (debug) {
    for (g = 0; g <= ngroups; g++) {
      printf("D Group %d (%d): ",g,dgroup[g][0]);
      for (r = 1; r <= dgroup[g][0]; r++) {
	printf("%d ",dgroup[g][r]);
      }
      printf("\n");
    }
    for (g = 0; g <= ngroups; g++) {
      printf("B Group %d (%d): ",g,bgroup[g][0]);
      for (r = 1; r <= dgroup[g][0]; r++) {
	printf("%d ",bgroup[g][r]);
      }
      printf("\n");
    }
  }

  // These mallocs can only happen after we determine do_trace/do_tensor/do_adc
  if ((S0   = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)   nomem();
  if ((DW   = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)   nomem();
  if ((ADC1 = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)   nomem();
  if (do_avadc || do_trace || do_tensor) {
    if ((TR   = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL) nomem();
  }
  if (do_trace || do_tensor) {
    if ((ADC2 = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL) nomem();
    if ((ADC3 = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL) nomem();
  }
  if (do_tensor) {
    if ((FA = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)   nomem();
    if ((X1 = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)   nomem();
    if ((Y1 = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)   nomem();
    if ((Z1 = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)   nomem();
  }
  if (input.residual) {
    if ((RMSres  = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();
  }
  if (input.mask) {
    if ((fitmask = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();
  }

  // Allocate memory & initialize GSL variables
  // These variables do not depend on # directions
  FitElem      = gsl_vector_calloc(nfit);
  if (do_tensor) {
    Tensor       = gsl_matrix_calloc(3,3);
    eigvec       = gsl_matrix_calloc(3,3);
    eigval       = gsl_vector_calloc(3);
  }

  if (do_avadc) {
    // Initialize TR, it will be used for an "average" ADC map if it's not trace/tensor
    for (slice = 0; slice < fdf_hdr.slices; slice++) {
      for (pixel = 0; pixel < datapts; pixel++) {
        spixel = slice*datapts + pixel;
        TR[spixel] = 0.0;
      }
    }
  }

  // Create directory to put output in
  if (strcmp(input.outdir, "./")) {
    if (debug) printf("Making directory %s\n",input.outdir);
    mkdir(input.outdir,0777);
  }

  /* Create subdirectories for FDF data */
  createdir("S0.img",input.outdir);
  if (do_avadc || do_trace || do_tensor) createdir("TR.img",input.outdir);
  createdir("DW.img",input.outdir);
  createdir("ADC.img",input.outdir);
  if (do_tensor) {
    createdir("FA.img",input.outdir);
    createdir("RPS.img",input.outdir);
  }
  if (input.residual) createdir("RMSres.img",input.outdir);
  if (input.mask) createdir("Mask.img",input.outdir);

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
  /*** Group by group calculations ***********************************/  
  /*******************************************************************/  
  for (g = 1; g <= ngroups; g++) {
    /* for each group we need to 
       - set up the b-matrix 
       - load the images for this group
       - do the math 
       - write the parametric map 
    */

    // Allocate memory & initialize GSL variables
    Ssize        = dgroup[0][0]+dgroup[g][0];
    Bmatrix      = gsl_matrix_calloc(Ssize,nfit);
    S            = gsl_vector_calloc(Ssize);
    tau          = gsl_vector_calloc((float) min((double)Ssize,nfit));
    res          = gsl_vector_calloc(Ssize);

    /* Create Bmatrix. Each row is: 
	[b 1]                                 for adc 
	[b_rr b_pp b_ss 1]                    for trace
	[b_rr b_pp b_ss 2b_rp 2b_rs 2b_sp 1]  for tensor */
	
    // Fill in b = 0 values    
    gi = 1;
    for (r = 0; r < dgroup[0][0]; r++) {
      c   = 0;
      inx = bgroup[0][gi];
          if (do_trace || do_tensor) {
  	    gsl_matrix_set(Bmatrix, r, c, bvalrr[inx]);   c++;
  	    gsl_matrix_set(Bmatrix, r, c, bvalpp[inx]);   c++;
	    gsl_matrix_set(Bmatrix, r, c, bvalss[inx]);   c++;
	  }
	  else {
	    gsl_matrix_set(Bmatrix, r, c, bvalue[inx]);   c++;
	  }
          if (do_tensor) {
	    gsl_matrix_set(Bmatrix, r, c, bvalrp[inx]*2); c++;
	    gsl_matrix_set(Bmatrix, r, c, bvalrs[inx]*2); c++;
	    gsl_matrix_set(Bmatrix, r, c, bvalsp[inx]*2); c++;
	  }
	  gsl_matrix_set(Bmatrix, r, c, 1.0);
	  gi++;

    }
    
    // Fill in b > 0 values
    gi = 1;
    for (r = dgroup[0][0]; r < Ssize; r++) {
      c = 0;
      inx = bgroup[g][gi];
          if (do_trace || do_tensor) {
  	    gsl_matrix_set(Bmatrix, r, c, bvalrr[inx]);   c++;
  	    gsl_matrix_set(Bmatrix, r, c, bvalpp[inx]);   c++;
	    gsl_matrix_set(Bmatrix, r, c, bvalss[inx]);   c++;
	  }
	  else {
	    gsl_matrix_set(Bmatrix, r, c, bvalue[inx]);   c++;
	  }
          if (do_tensor) {
	    gsl_matrix_set(Bmatrix, r, c, bvalrp[inx]*2); c++;
	    gsl_matrix_set(Bmatrix, r, c, bvalrs[inx]*2); c++;
	    gsl_matrix_set(Bmatrix, r, c, bvalsp[inx]*2); c++;
	  }
	  gsl_matrix_set(Bmatrix, r, c, 1.0);
	  gi++;
    }
    if (debug) gsl_print_matrix(Bmatrix,"Bmatrix");


    // Now decompose the Bmatrix; Note: Bmatrix is modified by this!!
    gsl_linalg_QR_decomp(Bmatrix, tau);


    /*******************************************************************/  
    /*** Slice by slice calculations ***********************************/  
    /*******************************************************************/  
    for (slice = 0; slice < fdf_hdr.slices; slice++) {
      for (r = 0; r < dgroup[0][0]; r++) {
        inx = dgroup[0][r+1];

	// Read all b = 0 images for this slice
	sprintf(filename,"%s/%s%03dimage%03decho001.fdf",input.indir,basename,slice+1,inx);
	if (debug) printf("data[%d] = %s\n",r,filename);
	read_fdf_data(filename,data[r],&fdf_hdr);
      } // end read b = 0 image loop

      for (r = dgroup[0][0]; r < Ssize; r++) {
        inx = dgroup[g][r-dgroup[0][0]+1];
	// Read all b > 0 images for this slice
	sprintf(filename,"%s/%s%03dimage%03decho001.fdf",input.indir,basename,slice+1,inx);
	if (debug) printf("data[%d] = %s\n",r,filename);
	read_fdf_data(filename,data[r],&fdf_hdr);
      } // end read b > 0 image loop

      // Noise threshold - user input or based on histogram
      if (input.noise >= 0.0) noise_thr = input.noise;
      else { // Get noise from the first image, often the b=0 image
	noise_thr = threshold(data[0],&fdf_hdr);
        // If vnmruser environment exists output threshold
        val2file("aipThreshold",(double)noise_thr,FLT32);
      }

      if (debug) printf("noise level %f\n",noise_thr);

      /*******************************************************************/  
      /*** Pixel loop ****************************************************/  
      /*******************************************************************/  
      for (pixel = 0; pixel < datapts; pixel++) {
        spixel = slice*datapts + pixel;
	ADC1[spixel] = S0[spixel] = DW[spixel] = 0.0;

	if (do_trace || do_tensor) {
          ADC2[spixel] = ADC3[spixel] = TR[spixel] = 0.0;
	}
	if (do_tensor) {
          FA[spixel] = X1[spixel] = Y1[spixel] = Z1[spixel] = 0.0;
	}
        if (input.mask) fitmask[spixel] = 0.0;
        if (input.residual) RMSres[spixel] = 0.0;

        if (pixel == debug_pixel) pixeldebug=TRUE; else pixeldebug=FALSE;

	// Initialize Signal vector
	for (image = 0; image < Ssize; image++) {
          if (pixeldebug) printf("data[%d] = %f\n",image,data[image][pixel]);
          gsl_vector_set(S, image, -log((double)data[image][pixel]));
	}
	if (pixeldebug)  gsl_print_vector(S,"S");


	if (data[0][pixel] > noise_thr) {  
          /*******************************************************************/
	  /*  Directly solve linear equation S = Bmatrix x FitElem, where    */
	  /*    S          = measured signal                                 */
	  /*    Bmatrix    = Matrix of diffusion weightings                  */
	  /*    FitElem    = Vector of Tensor elements                       */
          /*******************************************************************/  
	  gsl_linalg_QR_lssolve(Bmatrix, tau, S, FitElem, res);
          if (pixeldebug) {
            gsl_print_vector(FitElem,"FitElem");
            gsl_print_vector(res,"Residuals");
          }

          if (do_tensor) {
	    // Assign 3x3 Tensor
	    gsl_matrix_set(Tensor, 0, 0, gsl_vector_get(FitElem,0));
	    gsl_matrix_set(Tensor, 1, 1, gsl_vector_get(FitElem,1));
	    gsl_matrix_set(Tensor, 2, 2, gsl_vector_get(FitElem,2));
	    gsl_matrix_set(Tensor, 0, 1, gsl_vector_get(FitElem,3));
	    gsl_matrix_set(Tensor, 1, 0, gsl_vector_get(FitElem,3));
	    gsl_matrix_set(Tensor, 0, 2, gsl_vector_get(FitElem,4));
	    gsl_matrix_set(Tensor, 2, 0, gsl_vector_get(FitElem,4));
	    gsl_matrix_set(Tensor, 1, 2, gsl_vector_get(FitElem,5));
	    gsl_matrix_set(Tensor, 2, 1, gsl_vector_get(FitElem,5));
            if (pixeldebug)  gsl_print_matrix(Tensor,"Tensor");


	    // Calculate eigensystem and sort
	    gsl_eigen_symmv(Tensor, eigval, eigvec, gsl_wsp);
	    gsl_eigen_symmv_sort(eigval, eigvec, GSL_EIGEN_SORT_ABS_DESC);
            if (pixeldebug)  {
              gsl_print_vector(eigval,"Eigen Values");
              gsl_print_matrix(eigvec,"Eigen Vectors");
            }

	    e1 = gsl_vector_get(eigval,0);
	    e2 = gsl_vector_get(eigval,1);
	    e3 = gsl_vector_get(eigval,2);
          }
	  else {
	    e1 = gsl_vector_get(FitElem,0);
            if (do_trace) {
 	      e2 = gsl_vector_get(FitElem,1);
	      e3 = gsl_vector_get(FitElem,2);
	    }
	  }


          /*******************************************************************/
	  /* Calculate Output parameters *************************************/
          /*******************************************************************/
	  //check if any eigenvalue is < 0; can happen if the data is noisy
	  if (e1 < 0) e1 = 0; if (e2 < 0) e2 = 0; if (e3 < 0) e3 = 0;

          ADC1[spixel] = e1;
          if (do_trace || do_tensor) {
            ADC2[spixel] = e2;
            ADC3[spixel] = e3;

    	    trace = (e1 + e2 + e3)/3.0;
	    TR[spixel] = trace;
	  }
	  else {
	    trace = e1;
	    if (do_avadc) TR[spixel] += e1;
	  }

	  S0[spixel] = (float) exp((double)(-gsl_vector_get(FitElem,nfit-1)));
	  DW[spixel] = S0[spixel]*exp((double)(-dw_bval*trace)); //(isotropic) DW image


          if (do_tensor) {
	    DDa = (e1 - trace)*(e1 - trace)
		+ (e2 - trace)*(e2 - trace)
		+ (e3 - trace)*(e3 - trace);
	    DD  = e1*e1 + e2*e2 + e3*e3;

            if (DD > 0)
	      FA[spixel] = (float)sqrt((double) (3.0/2.0*DDa/DD));
	    else
	      FA[spixel] = 0;

	    // zeroth column is largest eigenvector after sort 
            X1[spixel] = (float)gsl_matrix_get(eigvec,0,0);
            Y1[spixel] = (float)gsl_matrix_get(eigvec,1,0);
            Z1[spixel] = (float)gsl_matrix_get(eigvec,2,0);

            if (pixeldebug) printf("FA = %f\n",FA[spixel]);
	  } // if tensor

	  if (input.residual) {
            SOSres=0.0;
            for (i=0;i<Ssize;i++) {
              residual = gsl_vector_get(S,i)-gsl_vector_get(res,i);
              residual = exp(-gsl_vector_get(S,i))-exp(-residual);
              SOSres += residual*residual;
            }
            SOSres /= Ssize;
            RMSres[spixel] = (float)sqrt(SOSres);
          }

          // mask to show number of pts used
          if (input.mask) fitmask[spixel] = (float)Ssize;

	} // end check positive S vector 
      } // end pixel loop    


      /*******************************************************************/
      /* Write Parametric maps *******************************************/
      /*******************************************************************/
      fdf_hdr.array_dim     = 1;
      fdf_hdr.array_index   = 1;
      fdf_hdr.echoes        = 1;
      fdf_hdr.echo_no       = 1;
      fdf_hdr.slice_no      = slice + 1;
      fdf_hdr.display_order = slice;
      fdf_hdr.pss           = (float) fdf_hdr.pss_array[slice];

      sprintf(filename,"%s/S0.img/S0_%03d.fdf",input.outdir,slice+1);
      write_fdf(filename,&S0[slice*datapts],&fdf_hdr);

      if (do_trace || do_tensor) {
        sprintf(filename,"%s/DW.img/DW_%03d.fdf",input.outdir,slice+1);
        write_fdf(filename,&DW[slice*datapts],&fdf_hdr);
        sprintf(filename,"%s/TR.img/TR_%03d.fdf",input.outdir,slice+1);
        write_fdf(filename,&TR[slice*datapts],&fdf_hdr);
        sprintf(filename,"%s/ADC.img/ADC1_%03d.fdf",input.outdir,slice+1);
        write_fdf(filename,&ADC1[slice*datapts],&fdf_hdr);
        fdf_hdr.array_index++;
        fdf_hdr.display_order += fdf_hdr.slices;
        sprintf(filename,"%s/ADC.img/ADC2_%03d.fdf",input.outdir,slice+1);
        write_fdf(filename,&ADC2[slice*datapts],&fdf_hdr);
        fdf_hdr.array_index++;
        fdf_hdr.display_order += fdf_hdr.slices;
        sprintf(filename,"%s/ADC.img/ADC3_%03d.fdf",input.outdir,slice+1);
        write_fdf(filename,&ADC3[slice*datapts],&fdf_hdr);
        fdf_hdr.array_index   = 1;
        fdf_hdr.display_order = slice;
      } else {
        fdf_hdr.array_index   = g;
        fdf_hdr.display_order = (g-1)*fdf_hdr.slices+slice;
        if (ngroups > 1) 
          sprintf(filename,"%s/ADC.img/ADC_g%03d_%03d.fdf",input.outdir,g,slice+1);
        else
          sprintf(filename,"%s/ADC.img/ADC_%03d.fdf",input.outdir,slice+1);
        write_fdf(filename,&ADC1[slice*datapts],&fdf_hdr);
        fdf_hdr.array_index   = 1;
        fdf_hdr.display_order = slice;
      }

      if (do_tensor) {
        sprintf(filename,"%s/FA.img/FA_%03d.fdf",input.outdir,slice+1);
        write_fdf(filename,&FA[slice*datapts],&fdf_hdr);
        sprintf(filename,"%s/RPS.img/R1_%03d.fdf",input.outdir,slice+1);
        write_fdf(filename,&X1[slice*datapts],&fdf_hdr);
        fdf_hdr.array_index++;
        fdf_hdr.display_order += fdf_hdr.slices;
        sprintf(filename,"%s/RPS.img/P1_%03d.fdf",input.outdir,slice+1);
        write_fdf(filename,&Y1[slice*datapts],&fdf_hdr);
        fdf_hdr.array_index++;
        fdf_hdr.display_order += fdf_hdr.slices;
        sprintf(filename,"%s/RPS.img/S1_%03d.fdf",input.outdir,slice+1);
        write_fdf(filename,&Z1[slice*datapts],&fdf_hdr);
        fdf_hdr.array_index   = 1;
        fdf_hdr.display_order = slice;
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

    gsl_matrix_free(Bmatrix);
    gsl_vector_free(S);
    gsl_vector_free(tau);
    gsl_vector_free(res);
    
  } // end group loop

  if (do_avadc) {  // create "average" ADC
    if (debug) printf("Write out average ADC\n");
    for (slice = 0; slice < fdf_hdr.slices; slice++) {
      for (pixel = 0; pixel < datapts; pixel++) {
	spixel = slice*datapts + pixel;
	TR[spixel] /= ngroups;
        DW[spixel]  = S0[spixel]*exp((double)(-dw_bval*TR[spixel])); //(isotropic) DW image
//	TR[spixel] *= 1e6;
      }
      fdf_hdr.array_dim     = 1;
      fdf_hdr.array_index   = 1;
      fdf_hdr.echoes        = 1;
      fdf_hdr.echo_no       = 1;
      fdf_hdr.slice_no      = slice + 1;
      fdf_hdr.display_order = slice;
      fdf_hdr.pss           = (float) fdf_hdr.pss_array[slice];

      sprintf(filename,"%s/TR.img/TR_%03d.fdf",input.outdir,slice+1);
      write_fdf(filename,&TR[slice*datapts],&fdf_hdr);
      sprintf(filename,"%s/DW.img/DW_%03d.fdf",input.outdir,slice+1);
      write_fdf(filename,&DW[slice*datapts],&fdf_hdr);
    } // end slice loop
  }  //end neither trace nor tensor

}



/*******************************************************************/  
/*** Check number of diffusion directions **************************/  
/*******************************************************************/  
int check_directions(double *Dro, double *Dpe, double *Dsl, int ndir, int *group) {
  int    ngroups = 0, new_group;
  int    got_b0 = 0, trace;
  double sumsq[ndir];
  double D_diff = 1e-6;
  float  vector[3][3], dot; // Used to check 3 groups for trace
  
  int g,g2;


  // Check how many entries
  for (g = 0; g < ndir; g++) {
    if ((Dro[g] == 0) && (Dpe[g] == 0) && (Dsl[g] == 0)) {
      got_b0 = 1;
      group[g] = 0;  // Group 0 is reserved for b=0
    }
    else {
      // vector length
      sumsq[g] = sqrt(Dro[g]*Dro[g] + Dpe[g]*Dpe[g] + Dsl[g]*Dsl[g]);

      // Compare directions after dividing by vector length
      // Compare both positive and negative directions
      new_group = 1;
      for (g2 = 0; g2 < g; g2++) {
        if (((fabs(Dro[g]/sumsq[g] -   Dro[g2]/sumsq[g2]) < D_diff) &&
             (fabs(Dpe[g]/sumsq[g] -   Dpe[g2]/sumsq[g2]) < D_diff) &&
             (fabs(Dsl[g]/sumsq[g] -   Dsl[g2]/sumsq[g2]) < D_diff))
            ||
            ((fabs(Dro[g]/sumsq[g] -  -Dro[g2]/sumsq[g2]) < D_diff) &&
             (fabs(Dpe[g]/sumsq[g] -  -Dpe[g2]/sumsq[g2]) < D_diff) &&
             (fabs(Dsl[g]/sumsq[g] -  -Dsl[g2]/sumsq[g2]) < D_diff))
            ) {
	  new_group = 0;  // we already counted this direction   
	  group[g]  = group[g2]; // and they're in the same group
	}
      }
      ngroups += new_group;
      if (new_group) group[g] = ngroups; 

      /* Keep track of the normalized vector for the first three groups; 
         if they are orthogonal we may be able to calculate the trace */
      if (ngroups <= 3) {
        vector[ngroups-1][0] = Dro[g]/sumsq[g];
        vector[ngroups-1][1] = Dpe[g]/sumsq[g];
        vector[ngroups-1][2] = Dsl[g]/sumsq[g];
      }
    }
  }
  
  if (ngroups == 3) {
    /* Check if the three groups are orthogonal;
       if so, then we can calculate the trace */
    trace = 1;
    for (g = 0; g < ngroups; g++) {
      for (g2 = 0; g2 < g; g2++) {
        dot = vector[g][0]*vector[g2][0]
	    + vector[g][1]*vector[g2][1]
	    + vector[g][2]*vector[g2][2];
        if (dot > D_diff) trace = 0;  // not orthogonal 
      }
    }
    if (trace) ngroups = -3;
  }
  
  return(ngroups);

}




/*******************************************************************/  
/*** Get arguments from commandline ********************************/  
/*******************************************************************/  
void get_args(user_input *input, int argc, char *argv[]) {
  int n;
  
  if ((argc == 2) && (!strncmp (argv[1], "-help", 2))) {
    print_usage(argv[0]);
  }

  
  for (n = 1; n < argc; n++) {

    if ((strncmp(argv[n],"-debug", 2) == 0)||(strncmp(argv[n],"-verbose", 2) == 0))
      input->debug = 1;

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

    /***** bvalue *******************/
    else if (strncmp(argv[n],"-bvalue", 2) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a number after -bvalue\n");
	exit(1);
      }
      input->bvalue = (float) atof(argv[++n]);
      if (input->bvalue < 0) {
	fprintf(stderr, "bvalue must be > 0!\n");
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

    /***** sorting ******************/
    else if (strncmp(argv[n],"-sort", 2) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a number after -sort!\n");
	exit(1);
      }
      input->sort = (float) atof(argv[++n]);
      if (input->sort < 0) input->sort = 0;
    }

    /***** mask *********************/
    else if (strncmp(argv[n],"-mask", 2) == 0)
      input->mask = 1;

    /***** unknown ******************/
    else {
      fprintf(stderr, "Unknown argument: %s\n", argv[n]);
      print_usage(argv[0]);
    }
    

  }

}


void print_usage(char *pgm) {
    fprintf(stderr, "Source code %s\n\n", PGM);
    
    fprintf(stderr, "Usage: %s [options]\n", pgm);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "-outdir dir   Use this directory for output (default 'diffcalc')\n");
    fprintf(stderr, "-indir dir    Use this directory for input (default '.')\n");
    fprintf(stderr, "-noise N      Use noise level N\n");
    fprintf(stderr, "-bvalue b     Output isotropic DWI at bvalue b\n");
    fprintf(stderr, "-sort N       Sort by direction (N=1, default), no sort (0), or treat all as same direction (2)\n");
    fprintf(stderr, "-residual     Output residuals\n");
    fprintf(stderr, "-debug        Print debug messages\n");
    fprintf(stderr, "-pixel N      Print debug messages for pixel N\n");
    fprintf(stderr, "-mask         Output mask image\n");
    fprintf(stderr, "\n");

    exit(1);
  }

