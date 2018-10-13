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
#define  NOISE_THR 4  // Threshold for fit (auto-noise seems to overestimate)
#define  FIT_PTS   3  // Minimum number of points to use in fit

typedef struct {
  int    debug;
  char   fit_type[MAXSTR];
  char   fit_var[MAXSTR];
  float  noise;
  int    pixel;
  int    residual;
  char   outdir[MAXSTR];
  char   indir[MAXSTR];
  int    mask;
  char   echoes[MAXSTR];
} user_input;

void get_args();

char procpar[MAXSTR];

int main(int argc, char *argv[]) {
  double *fitvar,*nevar;
  int     nelem, nimages, ne, start, skip;

  float  **data;
  double *image_arr;
  float  *map,*S0,*fitmask,*RMSres;
  double  residual,SOSres;
  
  fdf_header      fdf_hdr;
  char    filename[MAXSTR], basename[MAXSTR], recon[MAXSTR];

  // Variables for GSL functions
  gsl_vector      *map_vector;   // tensor elements arranged in vector
  gsl_matrix      *Vmatrix;      // matrix to hold echo times (and ones)
  gsl_vector      *S;            // vector of signal intensities
  gsl_vector      *tau;          // Used by QR decomposition
  gsl_vector      *res;          // Residuals from solving linear system

  // Variables used to determine if all data points should be used to fit
  float *Vdata,*Vuse,*Suse;
  int    Ssize;
  gsl_vector *Sdata;             // Primarily for debugging

  float noise_thr1 = 0, noise_thr2 = 0;

  // Loop variables
  int slice, image, images;
  int pixel, datapts=0, spixel;
  int r,i;
  int debug_pixel, debug, pixeldebug;

  /* NULL pointers */
  fitvar=nevar=image_arr=NULL;
  data=NULL;
  map=S0=fitmask=RMSres=Vdata=Vuse=Suse=NULL;
  map_vector=S=tau=res=Sdata=NULL;
  Vmatrix=NULL;

  /* input arguments */
  user_input input = {
    0,          // debug
    "ADC",      // fit_type
    "none",     // fit_var
    -1,         // noise
    -1,         // pixel 
    FALSE,      // residual
    "none",     // outdir 
    ".",        // indir 
    0,          // mask output
    ""          // echoes 
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
    if (!strcmp(input.fit_type,"T2"))  strcpy(input.fit_var,"te");
    if (!strcmp(input.fit_type,"ADC")) strcpy(input.fit_var,"bvalue");
  }

  if (!strcmp(input.fit_var,"none")) { //fit_var wasn't set => unknown fit_type
    printf("Fit function %s is not implemented\n",input.fit_type);
    exit(0);
  }

  // Number of echoes, ne=1 if not T2 fit
  ne=(int)getval("ne");
  if (strcmp(input.fit_type,"T2")) ne=1;

  // Determine the number of images
  nelem = getstatus(input.fit_var);
  if (nelem == 0) {
    exit(0);  // error message is printed by getstatus
  }

  // Read independent variable
  if ((fitvar = (double *) malloc(sizeof(double)*nelem)) == NULL) nomem();
  getarray(input.fit_var,fitvar);

  // If fit variable is TE then values are in ms
  if (!strcmp(input.fit_var,"TE")) for (i=0;i<nelem;i++) fitvar[i]/=1000.0;

  // For multiecho allow fit of all, odd or even echoes
  start=0; skip=0;
  if (ne>1) {
    if (!strcmp(input.echoes,"odd") || !strcmp(input.echoes,"even")) {
      i=nelem;
      nelem/=2;
      start=1; skip=1;
      if (!strcmp(input.echoes,"odd")) {
        start=0;
        if(i%2) nelem++;
      }
      if ((nevar = (double *) malloc(sizeof(double)*nelem)) == NULL) nomem();
      for (i=0;i<nelem;i++) nevar[i]=fitvar[start+2*i];
      free(fitvar);
      fitvar=nevar;
    }
  }

  /* Is this an epi scan with reference scan(s)? */
  images = getstatus("image");
  if (images == nelem) { // assume image is arrayed in parallel with the variable
    //find out how many ref scans
    if ((image_arr = (double *) malloc(sizeof(double)*images)) == NULL) nomem();
    getarray("image",image_arr);
    nimages = 0;
    for (i = 0; i < images; i++)
      if (image_arr[i] == 1) 
        nimages++;
  }
  else nimages = nelem;

  if (debug) printf("nelem, nimages, images = %d, %d, %d\n",nelem, nimages, images);

  if (nimages < 2) {
    printf("Must have at least 2 values for %s for the %s fit\n",
      input.fit_var,input.fit_type);
    exit(0);
  }

  /* There's no point output residual if there's only 2 values */
  if (nimages < 3) input.residual=FALSE;
  
  /* Allocate memory & initialize GSL variables */
  map_vector = gsl_vector_calloc(2);
  tau        = gsl_vector_calloc(2);
  Sdata      = gsl_vector_calloc(nimages);
  Suse       = malloc(nimages*sizeof(float));
  Vdata      = malloc(nimages*sizeof(float));
  Vuse       = malloc(nimages*sizeof(float));

  /* Create matrix of independent variable */
  i = 0;
  for (r = 0; r < nelem; r++) {
    if (images == nelem) {
      if (image_arr[r] == 1) {
        Vdata[i] = fitvar[r];
	i++;
      }
    }
    else {
      Vdata[r] = fitvar[r];
    }
  }

  /* Allocate memory for all images */
  datapts = fdf_hdr.datasize;
  if ((data = (float **) malloc(nimages*sizeof(float *))) == NULL) nomem();
  for (image = 0; image < nimages; image++) {
    if ((data[image] = (float *)malloc(datapts*sizeof(float))) == NULL) nomem();
  }
  if ((map = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();
  if ((S0  = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();
  if (input.residual) {
    if ((RMSres  = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();
  }
  if (input.mask) {
    if ((fitmask = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();
  }

  if (!strcmp(input.fit_type,"T2")) fdf_hdr.echoes=1;

  /* Create directory to put output in */
  if (!strcmp(input.outdir,"none")) { // user hasn't defined it, use defaults
    if (!strcmp(input.fit_type,"T2"))  strcpy(input.outdir,"T2fit");
    if (!strcmp(input.fit_type,"ADC")) strcpy(input.outdir,"ADCfit");
    strcat(input.outdir,".img");
  }
  if (strcmp(input.outdir, "./")) {
    if (debug) printf("Making directory %s\n",input.outdir);
    mkdir(input.outdir,0777);
  }

  /* Create subdirectories for FDF data */
  sprintf(basename,"%s.img",input.fit_type);
  createdir(basename,input.outdir);
  createdir("S0.img",input.outdir);
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
  /*** Slice by slice calculations ***********************************/  
  /*******************************************************************/  
  for (slice = 0; slice < fdf_hdr.slices; slice++) {
    for (image = 0; image < nimages; image++) {
      /* Read all images for this slice */
      if (ne>1) 
        sprintf(filename,"%s/%s%03dimage001echo%03d.fdf",input.indir,basename,slice+1,image*skip+image+start+1);
      else
        sprintf(filename,"%s/%s%03dimage%03decho001.fdf",input.indir,basename,slice+1,image+1);
      if (debug) printf("Read file %s\r",filename);
      read_fdf_data(filename,data[image],&fdf_hdr);
    } // end read image loop
    if (debug) printf("\n");    

    // Noise threshold - user input or based on histogram
    if (input.noise >= 0.0) {
      noise_thr1 = noise_thr2 = input.noise;
    }
    else {// Get noise from the first image
      noise_thr1 = threshold(data[0],&fdf_hdr);
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
      map[spixel] = S0[spixel] = 0.0;
      if (input.mask) fitmask[spixel] = 0.0;
      if (input.residual) RMSres[spixel] = 0.0;

      Ssize = 0;
      for (image = 0; image < nimages; image++) {
        gsl_vector_set(Sdata, image,data[image][pixel]);  
        // Sdata is raw signal intensity for all images,

//        if (data[0][pixel] > noise_thr1) {  
        if ((data[0][pixel] > noise_thr1) && (data[image][pixel] > noise_thr2)) {  
          Suse[Ssize]=data[image][pixel];
          Vuse[Ssize]=Vdata[image];
	  Ssize++;
	}
      }

      if (Ssize > FIT_PTS) {
        // Have at least FIT_PTS data points

        if (pixel == debug_pixel) pixeldebug=TRUE; else pixeldebug=FALSE;

        Vmatrix = gsl_matrix_calloc(Ssize,2);
        S       = gsl_vector_calloc(Ssize);
        res     = gsl_vector_calloc(Ssize);

        for (image = 0; image < Ssize; image++) {
          gsl_vector_set(S, image, -log((double)Suse[image]));
          gsl_matrix_set(Vmatrix, image, 0, Vuse[image]);
          gsl_matrix_set(Vmatrix, image, 1, 1.0);
        }

        if (pixeldebug)  {
          // Sdata is raw signal intensity for all images,
          // S is -log(signal) for images that are used for fitting
          gsl_print_vector(Sdata,"Sdata"),gsl_print_vector(S,"S");
          printf("Using %d points\n",Ssize);
          gsl_print_matrix(Vmatrix,"Vmatrix");
        }

        //mask to show number of pts used
        if (input.mask) fitmask[spixel] = Ssize; 

        /* Now decompose the Vmatrix */
        gsl_linalg_QR_decomp(Vmatrix, tau);  /// Note: Vmatrix is modified by this!!

        ///////////////////////////////////////////////////////////////////
	// Directly solve linear equation S = Vmatrix x map_vector, where 
	//     S          = measured signal
	//     Vmatrix    = Matrix of independent variable (and ones)
	//     map_vector = Vector of 1/map and -log(S(0))
        ///////////////////////////////////////////////////////////////////
	gsl_linalg_QR_lssolve(Vmatrix, tau, S, map_vector, res);
        if (pixeldebug) {
          gsl_print_vector(map_vector,"map_vector");
          gsl_print_vector(res,"Residuals");
        }

        ///////////////////////////////////////////////////////////////////
	// Assign output images 
        ///////////////////////////////////////////////////////////////////
	if (!strcmp(input.fit_type,"T2")) {  //Also check reasonable range of T2
	  if ((gsl_vector_get(map_vector,0) > 0) && (gsl_vector_get(map_vector,0) > 1.0/1000.0))
	    map[spixel] = 1.0/gsl_vector_get(map_vector,0);
	  else 
	    map[spixel] = 0;
	}
	else if (!strcmp(input.fit_type,"ADC"))
	    map[spixel] = gsl_vector_get(map_vector,0);
	else
	  printf("Unknown fittype (%s)\n",input.fit_type);

	S0[spixel]  = exp(-gsl_vector_get(map_vector,1));

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

        if (pixeldebug) printf("map = %f\n",map[spixel]);

        gsl_vector_free(S);
        gsl_vector_free(res);
        gsl_matrix_free(Vmatrix);

      } // end check positive S vector 

    } // end pixel loop    

    /* Write Parametric maps: */
    fdf_hdr.array_dim     = 1;
    fdf_hdr.array_index   = 1;
    fdf_hdr.echoes        = 1;
    fdf_hdr.echo_no       = 1;
    fdf_hdr.slice_no      = slice + 1;
    fdf_hdr.display_order = slice;
    fdf_hdr.pss           = (float) fdf_hdr.pss_array[slice];

    sprintf(filename,"%s/%s.img/%s_%03d.fdf",input.outdir,input.fit_type,input.fit_type,slice+1);
    write_fdf(filename,&map[slice*datapts],&fdf_hdr);

    sprintf(filename,"%s/S0.img/S0_%03d.fdf",input.outdir,slice+1);
    write_fdf(filename,&S0[slice*datapts],&fdf_hdr);

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
/*** Get arguments from commandline ********************************/  
/*******************************************************************/  
void get_args(user_input *input, int argc, char *argv[]) {
  int n;
  
  if ((argc == 2) && (!strncmp (argv[1], "-help", 2))) {
    fprintf(stderr, "Usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "-fittype      Type of fit (T2 | ADC)\n");
    fprintf(stderr, "-fitvar       Name of parameter (e.g., te)\n");
    fprintf(stderr, "-noise N      Use noise level N\n");
    fprintf(stderr, "-outdir dir   Use this directory for output (default 'T2' or 'ADC')\n");
    fprintf(stderr, "-indir dir    Use this directory for input (default '.')\n");
    fprintf(stderr, "-residual     Output residuals\n");
    fprintf(stderr, "-echoes       Use odd/even echoes for multi-echo (odd | even) otherwise all used\n");
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

    /***** echoes to process *******************/
    else if (strncmp(argv[n],"-echoes", 3) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need echo string after -echoes!\n");
	exit(1);
      }
      strcpy(input->echoes, argv[++n]);
    }

    /***** unknown ******************/
    else {
      fprintf(stderr, "Unknown argument: %s\n", argv[n]);
      exit(1);
    }
    

  }

}
