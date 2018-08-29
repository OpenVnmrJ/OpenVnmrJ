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


typedef struct {
  int    debug;
  char   tagvar[MAXSTR];  //vnmrj variable that controls tagging on/off
  float  noise;
  int    pixel;
  char   outdir[MAXSTR];
  char   indir[MAXSTR];
  int    write_all;
  int    subtr;
  char   method[MAXSTR];
} user_input;

/* Functions specific to DTI processing */
void get_args();

char procpar[MAXSTR];

int main(int argc, char *argv[]) {
  int     nelem, nimages;

  float  *tagon[MAXELEM], *tagoff[MAXELEM], *tagdiff[MAXELEM];  // Images
  double *tagctrl;            // tag control variable values
  float  *mean_on, *mean_off, // mean of tag on/off images
         *mean_diff,          // mean of pairwise difference images
	 *diff_mean;          // difference of mean on/off images
  double *image_arr;
  
  fdf_header      fdf_hdr;
  char    filename[MAXSTR];


  float noise_thr = 0;

  // Loop variables
  int slice, image, images, ndiff;
  int pixel, datapts;
  int i;
  int debug_pixel, debug;
  
  /* input arguments */
  user_input input = {
    0,          // debug
    "asltag",   // tagvar
    -1,         // noise
    -1,         // pixel 
    "ASL",      // outdir 
    ".",        // indir 
    0,          // write all pairs of subtracted images
    0,          // subtraction scheme (integer), default strict pairs
    "pair"      // subtraction scheme (string), default strict pairs
  };

  
  /*******************************************************************/  
  /*** Calculations **************************************************/  
  /*******************************************************************/  
  // Get arguments from commandline
  get_args(&input, argc, argv);
  debug = input.debug;
  if (debug) {
    if (input.pixel < 0) 
      debug_pixel = fdf_hdr.ro_size/2*fdf_hdr.pe_size + fdf_hdr.pe_size/2;
    else
      debug_pixel = input.pixel;
  }
  else debug_pixel = -1;

  /* Set up string for procpar; 
     this is for future expansion, to allow 
       - data to be in arbitrary directory
       - S(0) and S(non-zero)to be in different directories 
  */
  strcpy(procpar,input.indir);
  strcat(procpar,"/procpar");
  if (debug) printf("indir = %s, procpar = %s\n",input.indir,procpar);


  // Determine the number of images
  nelem = getstatus(input.tagvar);
  
  if (nelem == 0) {
    exit(0);  // error message is printed by getstatus
  }
    

  // Read tag control variable
  if ((tagctrl = (double *) malloc(sizeof(double)*nelem)) == NULL) nomem();
  getarray(input.tagvar,tagctrl);

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


/* XXX Check if we have an equal number of tag on/off ??? */

  
  /* Initialize fdf header */
  init_fdf_header(&fdf_hdr);


  /* Allocate memory for all images */
  datapts = fdf_hdr.ro_size*fdf_hdr.pe_size;
  switch(input.subtr) {
    case 0:  ndiff = nimages/2;   break; //strict pairwise
    case 1:  ndiff = nimages - 1; break; // adjacent pairs
    case 2:  ndiff = nimages - 2; break; // "surround" subtraction
    default: printf("Invalid subtraction scheme\n");exit(0);
  }
  
  
  for (image = 0; image < ndiff; image++) {
/* XXX CHECK if tagctrl = -1, 0 , 1 */
    if ((tagon[image]   = (float *)malloc(datapts*sizeof(float))) == NULL) nomem();
    if ((tagoff[image]  = (float *)malloc(datapts*sizeof(float))) == NULL) nomem();
    if ((tagdiff[image] = (float *)malloc(datapts*sizeof(float))) == NULL) nomem();
  }
  if ((mean_on   = (float *)malloc(datapts*sizeof(float))) == NULL) nomem();
  if ((mean_off  = (float *)malloc(datapts*sizeof(float))) == NULL) nomem();
  if ((mean_diff = (float *)malloc(datapts*sizeof(float))) == NULL) nomem();
  if ((diff_mean = (float *)malloc(datapts*sizeof(float))) == NULL) nomem();


  /* Create directory to put output in */
  if (strcmp(input.outdir, "./")) {
    if (debug) printf("Making directory %s\n",input.outdir);
    mkdir(input.outdir,0777);
  }

  
  /*******************************************************************/  
  /*** Slice by slice calculations ***********************************/  
  /*******************************************************************/  
  for (slice = 0; slice < fdf_hdr.slices; slice++) {
    i = 0;
    for (image = 0; image < nimages; image+=2) {
/* XXX check control variable */
      /* Read tag on */
      sprintf(filename,"%s/slice%03dimage%03decho001.fdf",
              input.indir,slice+1,image+1);
      if (debug) printf("tag on  [%d]: %s\n",i,filename);
      read_fdf_data(filename,tagon[i],&fdf_hdr);

      /* Read tag off */
      sprintf(filename,"%s/slice%03dimage%03decho001.fdf",
              input.indir,slice+1,image+2);
      if (debug) printf("tag off [%d]: %s\n",i,filename);
      read_fdf_data(filename,tagoff[i],&fdf_hdr);
      
      i++;
    } // end read image loop
    if (debug) printf("\n");    

    // Noise threshold - user input or based on histogram
    if (input.noise > 0) noise_thr = input.noise;
    else // Get noise from the first image
      noise_thr = threshold(tagon[0],&fdf_hdr);

    if (debug) printf("noise level %f\n",noise_thr);

    /*******************************************************************/  
    /*** Pixel loop ****************************************************/  
    /*******************************************************************/  
    for (pixel = 0; pixel < datapts; pixel++) {
      mean_diff[pixel] = 0;
     
      if (tagon[0][pixel] > noise_thr) {
        switch (input.subtr) {
	  case 0: // strictly pairwise: 0-0, 1-1, 2-2, ...
            for (image = 0; image < nimages/2; image++) {
	      tagdiff[image][pixel]  = (tagon[image][pixel] - tagoff[image][pixel]);
	      mean_diff[pixel]      += tagdiff[image][pixel];
            } 
	    break;

	  case 1:   // adjacent pairs: 0-0, 1-0, 1-1, 2-1, ...
	    i = 0;
            for (image = 0; image < nimages/2-1; image++) {
	      tagdiff[i][pixel] = (tagon[image][pixel] - tagoff[image][pixel]);
	      mean_diff[pixel] +=  tagdiff[i][pixel];
	      i++;
              if (pixel == debug_pixel) printf("subtract %d - %d\n",image,image);
	      
	      tagdiff[i][pixel] = (tagon[image+1][pixel] - tagoff[image][pixel]);
	      mean_diff[pixel] +=  tagdiff[i][pixel];
              if (pixel == debug_pixel) printf("subtract %d - %d\n",image+1,image);

	      i++;
            }
   	    tagdiff[i][pixel] = (tagon[nimages/2-1][pixel] - tagoff[nimages/2-1][pixel]);
            if (pixel == debug_pixel) printf("subtract %d - %d\n",nimages/2,nimages/2);

            mean_diff[pixel] +=  tagdiff[i][pixel];
	    break;

	  case 2: // surrounding pairs
	    i = 0;
            for (image = 0; image < nimages/2-1; image++) {
	      tagdiff[i][pixel] = ((tagon[image][pixel]+tagon[image+1][pixel])/2 - tagoff[image][pixel]);
	      mean_diff[pixel] +=  tagdiff[i][pixel];
	      i++;
              if (pixel == debug_pixel) printf("subtract %d+%d - %d\n",image,image+1,image);

	      tagdiff[i][pixel] = (tagon[image+1][pixel] - (tagoff[image][pixel]+tagoff[image+1][pixel])/2);
	      mean_diff[pixel] +=  tagdiff[i][pixel];
	      i++;
              if (pixel == debug_pixel) printf("subtract %d - %d+%d\n",image+1,image,image+1);
            }
	    break;
	  }
	  // divide by # pairs of images
	  mean_diff[pixel] /= ndiff;

      } // end check noise threshold 
    } // end pixel loop    
    

    /* Write Images maps: */
    fdf_hdr.array_dim     = 1 + input.write_all*ndiff;
    fdf_hdr.slice_no      = slice + 1;
    fdf_hdr.array_index   = 1;
    fdf_hdr.display_order = 1;

    if (input.write_all) {
      for (image = 0; image < ndiff; image++) {
	// write tag difference image
	sprintf(filename,"%s/tagdiff_%03d_%03d.fdf",input.outdir,image+1,slice+1);
	write_fdf(filename,tagdiff[image],&fdf_hdr);
	fdf_hdr.array_index++;
	fdf_hdr.display_order++;
      }
    }

    sprintf(filename,"%s/diff_%s_%03d.fdf",input.outdir,input.method,slice+1);
    write_fdf(filename,mean_diff,&fdf_hdr);
    fdf_hdr.array_index++;
    fdf_hdr.display_order++;

  } // end slice loop
  
}



/*******************************************************************/  
/*** Get arguments from commandline ********************************/  
/*******************************************************************/  
void get_args(user_input *input, int argc, char *argv[]) {
  int n;
  char str[MAXSTR];
  
  if ((argc == 2) && (!strncmp (argv[1], "-help", 2))) {
    fprintf(stderr, "Usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "-subtr S      Subtraction scheme (pair | adjacent | surround)\n");
    fprintf(stderr, "-tagvar       Name of tag parameter (default asltag)\n");
    fprintf(stderr, "-all          Write all difference images\n");
    fprintf(stderr, "-noise N      Use noise level N\n");
    fprintf(stderr, "-outdir dir   Use this directory for output (default 'ASL')\n");
    fprintf(stderr, "-indir dir    Use this directory for input (default '.')\n");
    fprintf(stderr, "-debug        Print debug messages\n");
    fprintf(stderr, "-pixel N      Print debug messages for pixel N\n");
    fprintf(stderr, "\n");

    exit(1);
  }

  
  for (n = 1; n < argc; n++) {

    if ((strncmp(argv[n],"-debug", 2) == 0)||(strncmp(argv[n],"-verbose", 2) == 0))
      input->debug = 1;



    /***** Fit variable*************************/
    else if (strncmp(argv[n],"-tagvar", 5) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a string after -tagvar!\n");
	exit(1);
      }
      strcpy(input->tagvar, argv[++n]);
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

    /***** write all images? ********/
    else if (strncmp(argv[n],"-all", 2) == 0)
      input->write_all = 1;

    /***** subtraction scheme *******/
    else if (strncmp(argv[n],"-subtr", 2) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a string after -subtr!\n");
	exit(1);
      }
      strcpy(str, argv[++n]);
   
      if ((strncmp(str,"pair",1) == 0) || (strncmp(str,"PAIR",1) == 0)) input->subtr = 0;
      if ((strncmp(str,"adj", 1) == 0) || (strncmp(str,"ADJ", 1) == 0)) input->subtr = 1;
      if ((strncmp(str,"surr",1) == 0) || (strncmp(str,"SURR",1) == 0)) input->subtr = 2;

      strcpy(input->method,str);

    }

    /***** unknown ******************/
    else {
      fprintf(stderr, "Unknown argument: %s\n", argv[n]);
      exit(1);
    }
    

  }

}
