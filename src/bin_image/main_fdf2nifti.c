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

#include "nifti1.h"

#if defined __GNUC__ && __GNUC__ >= 14
#pragma GCC diagnostic warning "-Wimplicit-function-declaration"
#endif

#define MAXSTR 1024
#define MAXELEM 1024

typedef float MY_DATATYPE;

#define MIN_HEADER_SIZE 348
#define NII_HEADER_SIZE 352
#define DEG_TO_RAD M_PI/180.

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
  fdf_header fdf_hdr;
  int iv, ifv;
  int sl;
  float *data;

  // Loop variables
  int slice, echo, element;
  int datapts, total_images, image_inx;
  int debug_pixel, debug;

  fileorg infiles;


  // input arguments
  user_input input = {
    ".",           // indir
    "./NII",      // outdir 
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
  input.avw   = 1;
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

  debug_pixel=64;
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
		strcat(filename, ".nii");

		write_nifti_file(filename, data, &fdf_hdr); // only have brute force now

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
		strcat(filename, ".nii");

		write_nifti_file(filename, data, &fdf_hdr); // only have brute force now

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
			strcat(filename, ".nii");

			write_nifti_file(filename, data, &fdf_hdr); // only have brute force now

		}



	exit(0);
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
  printf("-outdir dir   Use this directory for output (default 'NII')\n");
  printf("-indir dir    Use this directory for input (default '.')\n");
  printf("-debug        Print debug messages\n");
  printf("-pixel N      Print debug messages for pixel N (default center pixel)\n");
  printf("-me           Put images from a Multi-Echo experiment in separate files (default no)\n");
  printf("-arr          Put images from an arrayed experiment in a single file (default no)\n");
  // printf("-avw          Use \"brute force\" to write AVW\n");
  printf("\n");

  exit(1);
}


/**********************************************************************
 *
 * write_nifti_file
 *
 **********************************************************************/
int write_nifti_file(outfile, vdata, vhdr  )
     char *outfile;
     float *vdata;
     fdf_header *vhdr;
{
  nifti_1_header hdr;
  nifti1_extender pad={0,0,0,0};
  FILE *fp;
  int ret,i;
  int slice, datasize;
  MY_DATATYPE *data=NULL;
  short do_nii = 1;  // put header + data in one file for now
  char tname[500];
  float rot[9];
  float rt[9];
  float tmp;
  float r11, r12, r13, r21, r22, r23, r31, r32, r33;
  float a;
  float user_x, user_y, user_z;
  double cosphi, sinphi, cospsi, sinpsi, costheta, sintheta;
  double pss[MAXIMAGES];

  datasize=vhdr->datasize;
  /********** fill in the minimal default header fields */
  bzero((void *)&hdr, sizeof(hdr));
  hdr.sizeof_hdr = MIN_HEADER_SIZE;
  hdr.dim[0] = 3; // no time dimension yet
  hdr.dim[2] = vhdr->ro_size;
  hdr.dim[1] = vhdr->pe_size;
  hdr.dim[3] = vhdr->slices;
  if (vhdr->rank > 2)  
    hdr.dim[3] = vhdr->pe2_size;
  hdr.dim[4] = 1;
  hdr.datatype = NIFTI_TYPE_FLOAT32;
  hdr.bitpix = 32;
  hdr.pixdim[2] = (vhdr->lro)/hdr.dim[2];
  hdr.pixdim[1] = (vhdr->lpe)/hdr.dim[1];
  hdr.pixdim[3] = vhdr->thk/10.;
  hdr.pixdim[4] = 1.;
  if (do_nii)
    hdr.vox_offset = (float) NII_HEADER_SIZE;
  else
    hdr.vox_offset = (float)0;
  hdr.scl_slope = 100.0;
  hdr.xyzt_units = NIFTI_UNITS_MM | NIFTI_UNITS_SEC;
  if (do_nii)
    strncpy(hdr.magic, "n+1\0", 4);
  else
    strncpy(hdr.magic, "ni1\0", 4);


  /* nift orientation as quaternions */
  hdr.qform_code =  NIFTI_XFORM_SCANNER_ANAT;
  
  cosphi = cos((double)DEG_TO_RAD*vhdr->phi);
  sinphi = sin((double)DEG_TO_RAD*vhdr->phi);
  cospsi = cos((double)DEG_TO_RAD*vhdr->psi);
  sinpsi = sin((double)DEG_TO_RAD*vhdr->psi);
  costheta = cos((double)DEG_TO_RAD*vhdr->theta);
  sintheta = sin((double)DEG_TO_RAD*vhdr->theta);

  /* direction cosines for head first, supine */
  rot[0]=-1*cosphi*cospsi - sinphi*costheta*sinpsi;
  rot[1]=cosphi*sinpsi - sinphi*costheta*cospsi;
  rot[2]=sinphi*sintheta;
  rot[3]=-1*sinphi*cospsi + cosphi*costheta*sinpsi;
  rot[4]=sinphi*sinpsi + cosphi*costheta*cospsi;
  rot[5]=-1*cosphi*sintheta;
  rot[6]=0;
  rot[7]=0;
  rot[8]=0;
  

  /* adjust for patient orientation */
  if(strstr(vhdr->position1,"feet"))
    {
      rot[0] *= -1;
      rot[2] *= -1;
      rot[3] *= -1;
      rot[5] *= -1;
    }
  if(strstr(vhdr->position2,"prone"))
    {
      rot[0] *= -1;
      rot[1] *= -1;
      rot[3] *= -1;
      rot[4] *= -1;
    }
  if(strstr(vhdr->position2,"left") || strstr(vhdr->position2,"right"))
    {
      tmp=rot[0];
      rot[0]=rot[1];
      rot[1]=tmp;

      tmp=rot[3];
      rot[3]=rot[4];
      rot[4]=tmp;

      if(strstr(vhdr->position2,"right"))
	{
	  rot[0] *= -1;
	  rot[1] *= -1;
	  rot[3] *= -1;
	  rot[4] *= -1;
	}
      else
	{
	  rot[1] *= -1;
	  rot[4] *= -1;
	}
    }


  /* change signs for nifti vs dicom handedness */
  rot[0] *= -1;
  rot[2] *= -1;
  rot[4] *= -1;
  rot[5] *= -1;

  /* compute X-product to get third column */
  rot[6]=rot[1]*rot[5]-rot[2]*rot[4];
  rot[7]=rot[2]*rot[3]-rot[0]*rot[5];
  rot[8]=rot[0]*rot[4]-rot[1]*rot[3];

  /* now make the quaternions, first transpose R */
  r11=rot[0]; r21=rot[1]; r31=rot[2];
  r12=rot[3]; r22=rot[4]; r32=rot[5];
  r13=rot[6]; r23=rot[7]; r33=rot[8];
 
  a=0.5*sqrt(1.0+r11+r22+r33);
  hdr.quatern_b=0.25*(r32-r23)/a;
  hdr.quatern_c=0.25*(r13-r31)/a;
  hdr.quatern_d=0.25*(r21-r12)/a;
      

  printf("rots are: %f %f %f \n",r11, r12, r13);
  printf("rots are: %f %f %f \n",21, r22, r23);
  printf("rots are: %f %f %f \n",r31, r32, r33);
  printf("quats are: %f %f %f \n",hdr.quatern_b, hdr.quatern_c, hdr.quatern_d);

  /* Now for the offsets, thank you, Paul K! */
  /* Determine the position of the first data point in the user frame */
  /* Assume data has been acquired in a positive readout gradient */
  user_x = (float)10*(vhdr->pro + (vhdr->lro)/2.0) - hdr.pixdim[1]/2;
  /* Assume phase encoding gradient starts positive and ends up negative */
  user_y = (float)10*(vhdr->ppe - (vhdr->lpe)/2.0);
  /* For 2D we just need centre of the first slice which we have ordered to be the most negative */
  if (vhdr->rank < 3)
    {
      (void) getarray("pss",pss);
       user_z = (float)10*(pss[0]);  // probably wrong, this is the center
    }
  /* Assume 2nd phase encoding gradient starts positive and ends up negative */
  if (vhdr->rank>= 3) user_z = (float)10*(vhdr->ppe2 - (vhdr->lpe2)/2.0);
  /* Rotate the first data point into the magnet coordinate system to get the offsets */
  hdr.qoffset_x = (r11*user_x + r12*user_y + r13*user_z);
  hdr.qoffset_y = (r21*user_x + r22*user_y + r23*user_z);
  hdr.qoffset_z = (r31*user_x + r32*user_y + r33*user_z);

  printf("offsets are: %f %f %f \n",hdr.qoffset_x, hdr.qoffset_y, hdr.qoffset_z);


  /* flip-flop the Varian data as required ****/



  for (slice = 0; slice <vhdr->slices; slice++) {
    if (vhdr->rank == 2)  // 2D, flip slice up-down
      flipud2D(&vdata[slice*datasize],vhdr);
    //    else // 3D, rotate planes counter-clockwise
    // rotate3D(vdata,vhdr,-1);
  }


  /********** write first 348 bytes of header   */
    
  if(!do_nii){
    strcpy(tname, outfile);
    strcat(outfile,".hdr");
  }

  fp = fopen(outfile,"w");	
    if (fp == NULL) {
      fprintf(stderr, "\nError opening header file %s for write\n",outfile);
      exit(1);
    }
    ret = fwrite(&hdr, MIN_HEADER_SIZE, 1, fp);
    if (ret != 1) {
      fprintf(stderr, "\nError writing header file %s\n",outfile);
      exit(1);
    }


    /********** if nii, write extender pad and image data   */
    if (do_nii == 1) {

      ret = fwrite(&pad, 4, 1, fp);
      if (ret != 1) {
	fprintf(stderr, "\nError writing header file extension pad %s\n",outfile);
	exit(1);
      }

      ret = fwrite(vdata, (size_t)(hdr.bitpix/8), hdr.dim[1]*hdr.dim[2]*hdr.dim[3]*hdr.dim[4], fp);
      if (ret != hdr.dim[1]*hdr.dim[2]*hdr.dim[3]*hdr.dim[4]) {
	fprintf(stderr, "\nError writing data to %s\n",outfile);
	exit(1);
      }

      fclose(fp);
    }


    /********** if hdr/img, close .hdr and write image data to .img */
    else {

      fclose(fp);     /* close .hdr file */

      strcat(tname,".img");

      fp = fopen(tname,"w");
      if (fp == NULL) {
	fprintf(stderr, "\nError opening data file %s for write\n",tname);
	exit(1);
      }
      ret = fwrite(vdata, (size_t)(hdr.bitpix/8), hdr.dim[1]*hdr.dim[2]*hdr.dim[3]*hdr.dim[4], fp);
      if (ret != hdr.dim[1]*hdr.dim[2]*hdr.dim[3]*hdr.dim[4]) {
	fprintf(stderr, "\nError writing data to %s\n",tname);
	exit(1);
      }

      fclose(fp);
    }
  
    return(0);
  

}
