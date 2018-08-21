/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dirent.h>

#define MAXSTR 1024
#define MAXELEM 1024


#include "utils.h"

typedef struct {
  char   indir[MAXSTR];
  char   outdir[MAXSTR];
  int    debug;
  int    pixel;
  int    avw;
  int    ME;
  int    array;
} user_input;

void get_args();
void print_usage();

char procpar[MAXSTR];


/*******************************************************************/  
/* MAIN PROGRAM ****************************************************/
/*******************************************************************/  

int main(int argc, char *argv[]) {
  char filename[MAXSTR], fname1[MAXSTR];
  char dirname[MAXSTR];
  fdf_header fdf_hdr;

  float *data;

  // Loop variables
  int iv, ifv;
  int sl;
  int slice, echo, element;
  int datapts, total_images, image_inx;
  int debug_pixel, debug;

  fileorg infiles;

  // input arguments
  user_input input = {
    "."           // indir
    "./AVW",      // outdir 
    0,            // debug
    -1,           // pixel 
    1,            // avw
    1,            // ME, all echoes in one volume
    0             // array, all array elements in one volume
  };

  
  /*******************************************************************/  
  /*** Initializations and mallocs ***********************************/  
  /*******************************************************************/  
  /* initialize input struct */
  input.debug = 0;
  input.avw   = 0;
  input.ME    = 1;
  input.array = 0;

  // Get arguments from commandline
  get_args(&input, argc, argv);
  debug = input.debug;
  
  if ((input.ME == 0) && (input.array == 1)) {
    printf("Sorry, does not support collecting array elements for multi-echo experiments\n");
    exit(0);
  }


  // Set up string for procpar
  strcpy(procpar,input.indir);
  strcat(procpar,"/procpar");
  if (debug) printf("indir = %s, procpar = %s\n",input.indir,procpar);

  if(get_file_lists(input.indir, &infiles))
  {
	   printf("Error organizing input files \n");
	   exit(0);
  }


  // Initialize fdf header
  init_fdf_header(&fdf_hdr);

  if (debug) {
    if (input.pixel < 0) 
      debug_pixel = fdf_hdr.datasize/2;
    else
      debug_pixel = input.pixel;
  }
  else debug_pixel = -1;
  if (debug) printf("Debugging for pixel %d (total %d)\n",debug_pixel,fdf_hdr.datasize);

  
  /* Allocate memory for all slices */
  datapts      = fdf_hdr.datasize;
  total_images = fdf_hdr.slices;
  
  if (input.ME)    total_images *= fdf_hdr.echoes;
  if (input.array) total_images *= fdf_hdr.array_dim;
  
  if ((data = (float *)malloc(datapts*total_images*sizeof(float))) == NULL) nomem();

  
  
  fdf_hdr.Smax = -999;
  fdf_hdr.Smin = 1e6;

  if (fdf_hdr.rank == 2) strcpy(fname1,"slice");
  if (fdf_hdr.rank == 3) strcpy(fname1,"img_slab");

  if (debug) printf("%d elements, %d echoes, %d slices\n",fdf_hdr.array_dim,fdf_hdr.echoes,fdf_hdr.slices);
 
  if (strcmp(input.outdir, "./")) {
    if (debug) printf("Making directory %s\n",input.outdir);
    mkdir(input.outdir,0777);
  }

  /*******************************************************************/  
  /* Loop through images *********************************************/  
  /*******************************************************************/  

	// loop on volumes
	for (iv = 0; iv < infiles.nvols; iv++) {
		for (ifv = 0; ifv < infiles.vlists[iv].nfiles; ifv++) {

			strcpy(filename, input.indir);
			strcat(filename,"/");
			strcat(filename, infiles.vlists[iv].names[ifv]);

			if (debug)
				printf("data[%d] = %s\n", ifv + 1, filename);

			image_inx = ifv;

			read_fdf_data(filename, &data[image_inx * datapts], &fdf_hdr);

		}

		// If all images are in a single file, write it here
		fdf_hdr.slices = infiles.vlists[iv].nfiles;
		strcpy(filename, input.outdir);
		strcat(filename, "/");
		strcat(filename, infiles.vlists[iv].volname);

		write_avw(filename, data, &fdf_hdr); // only have brute force now

	} /* End element loop */// end of volume loop


	// loop on groups
	for (iv = 0; iv < infiles.ngrps; iv++) {
		for (ifv = 0; ifv < infiles.glists[iv].nfiles; ifv++) {

			strcpy(filename, input.indir);
			strcat(filename,"/");
			strcat(filename, infiles.glists[iv].names[ifv]);

			if (debug)
				printf("data[%d] = %s\n", ifv + 1, filename);

			image_inx = ifv;

			read_fdf_data(filename, &data[image_inx * datapts], &fdf_hdr);

		}

		// If all images are in a single file, write it here
		fdf_hdr.slices = infiles.glists[iv].nfiles;
		strcpy(filename, input.outdir);
		strcat(filename, "/");
		strcat(filename, infiles.glists[iv].volname);


		write_avw(filename, data, &fdf_hdr); // only have brute force now

	}

	// convert miscellaneous fdfs

	for (ifv = 0; ifv < infiles.misc->nfiles; ifv++) {

			strcpy(filename, input.indir);
			strcat(filename,"/");
			strcat(filename, infiles.misc->names[ifv]);

			if (debug)
				printf("data[%d] = %s\n", ifv + 1, filename);

			image_inx = 0;

			read_fdf_data(filename, &data[image_inx * datapts], &fdf_hdr);

			// write each fdf to a nifti file
			fdf_hdr.slices = 1;


			strcpy(filename, input.outdir);
			strcat(filename, "/");
			sl=strlen(infiles.misc->names[ifv]);
			sl = sl - 4;
			strncat(filename, infiles.misc->names[ifv], sl);


			write_avw(filename, data, &fdf_hdr); // only have brute force now

		}

  
  

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

    /***** pixel ********************/
    else if (strncmp(argv[n],"-pixel", 2) == 0) {
      if (n+1 >= argc) {
	printf("Need a number after -pixel!\n");
	exit(1);
      }
      input->pixel = atoi(argv[++n]);
      if (input->pixel < 0) {
	printf("pixel must be > 0!\n");
	exit(1);
      }
    }

    /***** output directory ********************/
    else if (strncmp(argv[n],"-outdir", 4) == 0) {
      if (n+1 >= argc) {
	printf("Need a directory after -outdir!\n");
	exit(1);
      }
      strcpy(input->outdir, argv[++n]);
    }

    /***** input directory *********************/
    else if (strncmp(argv[n],"-indir", 3) == 0) {
      if (n+1 >= argc) {
	printf("Need a directory after -indir!\n");
	exit(1);
      }
      strcpy(input->indir, argv[++n]);
    }

    /***** use "brute force" to write AVW ******/
    else if (strncmp(argv[n],"-avw", 3) == 0)
      input->avw = 0;

    /***** Put multi-echo images in separate files ****/
    else if (strncmp(argv[n],"-me", 2) == 0)
      input->ME = 0;

    /***** Put arrayed images in separate files ****/
    else if (strncmp(argv[n],"-array", 3) == 0)
      input->array = 1;

    /***** unknown ******************/
    else {
      printf("Unknown argument: %s\n", argv[n]);
      print_usage(argv[0]);
    }
  }  // end for loop to go through all input arguments
  
}


void print_usage(char *pgm) {
  
    printf("Usage: %s [options]\n", pgm);
    printf("\nOptions:\n");
    printf("-outdir dir   Use this directory for output (default 'AVW')\n");
    printf("-indir dir    Use this directory for input (default '.')\n");
    printf("-debug        Print debug messages\n");
    printf("-pixel N      Print debug messages for pixel N (default center pixel)\n");
    printf("-me           Put images from a Multi-Echo experiment in separate files (default no)\n");
    printf("-arr          Put images from an arrayed experiment in a single file (default no)\n");
    printf("-avw          Use \"brute force\" to write AVW\n");
    printf("\n");

    exit(1);
  }

