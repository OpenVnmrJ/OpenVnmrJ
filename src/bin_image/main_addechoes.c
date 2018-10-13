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
  int    start;
  int    end;
  int    pixel;
  char   outdir[MAXSTR];
  char   indir[MAXSTR];
} user_input;

void get_args();

char procpar[MAXSTR];

int main(int argc, char *argv[]) {

  int    ne,nechoes;

  float  **data;
  float  *map;
  
  fdf_header      fdf_hdr;
  char    filename[MAXSTR], basename[MAXSTR], recon[MAXSTR];

  struct stat buf;

  // Loop variables
  int slice, image;
  int pixel, datapts=0, spixel;
  int i,tmp;
  int debug_pixel, debug, pixeldebug;

  /* NULL pointers */
  data=NULL;
  map=NULL;

  /* input arguments */
  user_input input = {
    0,          // debug
    1,          // start echo
    1,          // end echo
    -1,         // pixel 
    "none",     // outdir 
    "."         // indir 
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

  // Number of echoes
  ne=(int)getval("ne");

  // Check range
  if (input.start<1) input.start=1;
  if (input.start>ne) input.start=ne;
  if (input.end<1) input.end=1;
  if (input.end>ne) input.end=ne;
  if (input.start>input.end) {
    tmp=input.start;
    input.start=input.end;
    input.end=tmp;
  }
  if (input.start==input.end) exit(0);

  nechoes=input.end-input.start+1;

  /* Allocate memory for all images */
  datapts = fdf_hdr.datasize;
  if ((data = (float **) malloc(nechoes*sizeof(float *))) == NULL) nomem();
  for (i=0;i<nechoes;i++) {
    if ((data[i] = (float *)malloc(datapts*sizeof(float))) == NULL) nomem();
  }
  if ((map = (float *)malloc(fdf_hdr.slices*datapts*sizeof(float))) == NULL)  nomem();

  fdf_hdr.echoes=1;

  /* Create directory to put output in */
  if (!strcmp(input.outdir,"none")) { // user hasn't defined it, use defaults
    strcpy(input.outdir,"Echoes.img");
  }
  if (strcmp(input.outdir, "./")) {
    if (debug) printf("Making directory %s\n",input.outdir);
    mkdir(input.outdir,0777);
  }

  /* Create subdirectories for FDF data */
  sprintf(basename,"SumEcho%d-%d.img",input.start,input.end);
  createdir(basename,input.outdir);

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
  image=0;
  while (image>-1) {

    for (slice = 0; slice < fdf_hdr.slices; slice++) {

      for (i = 0; i < nechoes; i++) {
        /* Read all images for this slice */
        sprintf(filename,"%s/%s%03dimage%03decho%03d.fdf",input.indir,basename,slice+1,image+1,i+input.start);
        if (stat(filename,&buf) == -1) {
          image=-1;
          exit(0);
        }
        if (debug) printf("Read file %s\r",filename);
        read_fdf_data(filename,data[i],&fdf_hdr);
      } // end read image loop
      if (debug) printf("\n");    

      /*******************************************************************/  
      /*** Pixel loop ****************************************************/  
      /*******************************************************************/  
      for (pixel = 0; pixel < datapts; pixel++) {
        spixel = slice*datapts + pixel;
        map[spixel] = 0.0;

        if (pixel == debug_pixel) pixeldebug=TRUE; else pixeldebug=FALSE;

        for (i = 0; i < nechoes; i++) map[spixel] += data[i][pixel];

        if (pixeldebug)  {
          for (i=0;i<nechoes;i++) printf("Echo %d = %f\n",i+input.start,data[i][pixel]);
        }

      } // end pixel loop    

      /* Write Parametric maps: */
      fdf_hdr.array_index   = image+1;
      fdf_hdr.echoes        = 1;
      fdf_hdr.echo_no       = 1;
      fdf_hdr.slice_no      = slice + 1;
      fdf_hdr.display_order = image*fdf_hdr.slices+slice;
      fdf_hdr.pss           = (float) fdf_hdr.pss_array[slice];

      sprintf(filename,"%s/SumEcho%d-%d.img/%s%03dimage%03decho001.fdf",input.outdir,input.start,input.end,basename,slice+1,image+1);
      write_fdf(filename,&map[slice*datapts],&fdf_hdr);

    } // end slice loop

    image++;

  } // end image loop

}


/*******************************************************************/  
/*** Get arguments from commandline ********************************/  
/*******************************************************************/  
void get_args(user_input *input, int argc, char *argv[]) {
  int n;
  
  if ((argc == 2) && (!strncmp (argv[1], "-help", 2))) {
    fprintf(stderr, "Usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "-start S      Start echo S\n");
    fprintf(stderr, "-end E        End echo E\n");
    fprintf(stderr, "-outdir dir   Use this directory for output (default 'Echoes')\n");
    fprintf(stderr, "-indir dir    Use this directory for input (default '.')\n");
    fprintf(stderr, "-debug        Print debug messages\n");
    fprintf(stderr, "-pixel N      Print debug messages for pixel N\n");
    fprintf(stderr, "\n");

    exit(1);
  }

  for (n = 1; n < argc; n++) {

    if ((strncmp(argv[n],"-debug", 2) == 0)||(strncmp(argv[n],"-verbose", 2) == 0))
      input->debug = 1;

    /***** start echo ****************/
    else if (strncmp(argv[n],"-start", 2) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a number after -start!\n");
	exit(1);
      }
      input->start = (float) atof(argv[++n]);
      if (input->start < 0) {
	fprintf(stderr, "start echo must be > 0!\n");
	exit(1);
      }
    }

    /***** end echo ******************/
    else if (strncmp(argv[n],"-end", 2) == 0) {
      if (n+1 >= argc) {
	fprintf(stderr, "Need a number after -end!\n");
	exit(1);
      }
      input->end = (float) atof(argv[++n]);
      if (input->end < 0) {
	fprintf(stderr, "end echo must be > 0!\n");
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

    /***** unknown ******************/
    else {
      fprintf(stderr, "Unknown argument: %s\n", argv[n]);
      exit(1);
    }

  }

}
