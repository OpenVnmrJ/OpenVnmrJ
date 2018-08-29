/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#define AVW_OFFSET 4096
/*
AVW_Volume *rotateVolume90();
int write_AVW();
*/

#include "dbh.h"

void write_avw_header(char *, fdf_header *);
void write_avw_hdr(char *, fdf_header *);
int write_avw2D();
int write_avw3D();




/*******************************************************************/  
/* Write AVW header using "brute force" approach, ie simply write  */
/* an ascii header with relevant information, not utilizing        */
/* Analyze functions                                               */
/*******************************************************************/  
void write_avw_hdr(filename, fdfhdr) 
     char *filename;
     fdf_header *fdfhdr;
{
  FILE *fp;

  int i;
  struct dsr hdr;

  char fn[MAXSTR];

  (void)strcpy(fn,filename);
  (void)strcat(fn,".hdr");
  if ((fp = fopen(fn,"w")) == NULL) {
    fprintf(stderr,"Can't open file %s\n",filename);
    exit(0);
  }


  memset(&hdr,0, sizeof(struct dsr));
  for(i=0;i<8;i++)
    hdr.dime.pixdim[i]=0.0;
   
  hdr.dime.vox_offset = 0.0;
  hdr.dime.roi_scale   = 1.0;
  hdr.dime.funused1    = 0.0;
  hdr.dime.funused2    = 0.0;
  hdr.dime.cal_max     = 0.0;
  hdr.dime.cal_min     = 0.0;
  
    
  hdr.dime.datatype = DT_FLOAT;
  hdr.dime.bitpix = 32;


  hdr.dime.dim[0] = 4;  /* all Analyze images are taken as 4 dimensional */
  hdr.hk.regular = 'r';
  hdr.hk.sizeof_hdr = sizeof(struct dsr);


  hdr.dime.dim[1] = fdfhdr->ro_size;  /* slice width  in pixels */
  hdr.dime.dim[2] = fdfhdr->pe_size;  /* slice height in pixels */
 if (fdfhdr->rank == 2)
   hdr.dime.dim[3] = fdfhdr->slices;
 else
   hdr.dime.dim[3] = fdfhdr->pe2_size;

  hdr.dime.dim[4] = 1;  /* number of volumes per file */

  hdr.dime.glmax  = fdfhdr->Smax;  /* maximum voxel value  */
  hdr.dime.glmin  = fdfhdr->Smin;  /* minimum voxel value */
    
  hdr.dime.pixdim[1] = fdfhdr->lro/fdfhdr->ro_size*10; /* voxel x dimension */
  hdr.dime.pixdim[2] = fdfhdr->lpe/fdfhdr->pe_size*10; /* voxel y dimension */
 
  if (fdfhdr->rank == 2)
    hdr.dime.pixdim[3] = fdfhdr->thk;
  else
    hdr.dime.pixdim[3] = fdfhdr->lpe/fdfhdr->pe2_size*10;

    
  /*   Assume zero offset in .img file, byte at which pixel
       data starts in the image file */

  hdr.dime.vox_offset = 0.0; 
    
  /*   Planar Orientation;    */
  /*   Movie flag OFF: 0 = transverse, 1 = coronal, 2 = sagittal
       Movie flag ON:  3 = transverse, 4 = coronal, 5 = sagittal  */  

  hdr.hist.orient     = 0;  
    
  /*   up to 3 characters for the voxels units label; i.e. 
       mm., um., cm.               */

  strcpy(hdr.dime.vox_units," ");
   
  /*   up to 7 characters for the calibration units label; i.e. HU */

  strcpy(hdr.dime.cal_units," ");  
    
  /*     Calibration maximum and minimum values;  
	 values of 0.0 for both fields imply that no 
	 calibration max and min values are used    */

  hdr.dime.cal_max = 0.0; 
  hdr.dime.cal_min = 0.0;

  fwrite(&hdr,sizeof(struct dsr),1,fp);
  fclose(fp);


} /* End write_avw_hdr */

/*******************************************************************/  
/* Write AVW header using "brute force" approach, ie simply write  */
/* an ascii header with relevant information, not utilizing        */
/* Analyze functions                                               */
/*******************************************************************/  
void write_avw_header(filename, hdr) 
     char *filename;
     fdf_header *hdr;
{
  FILE *fp;
  int  n;

  char fn[MAXSTR];

  (void)strcpy(fn,filename);
  (void)strcat(fn,".hdr");
  if ((fp = fopen(fn,"w")) == NULL) {
    fprintf(stderr,"Can't open file %s\n",filename);
    exit(0);
  }


  fprintf(fp,"AVW_ImageFile   1.00     %d\n",AVW_OFFSET);
  fprintf(fp," DataType=AVW_FLOAT\n");
  fprintf(fp," Width=%d\n",hdr->ro_size);
  fprintf(fp," Height=%d\n",hdr->pe_size);
  if (hdr->rank == 2)
    fprintf(fp," Depth=%d\n",hdr->slices);
  else
    fprintf(fp," Depth=%d\n",hdr->pe2_size);
  
  fprintf(fp," NumVols=1\n");
  fprintf(fp," Endian=Little\n");
  fprintf(fp," ColormapSize=0\n");
  fprintf(fp,"BeginInformation\n");
  fprintf(fp," DataFormat=\"AnalyzeAVW\"\n");
  fprintf(fp," MaximumDataValue=%.4f\n",hdr->Smax);
  fprintf(fp," MinimumDataValue=%.4f\n",hdr->Smin);
  fprintf(fp," VoxelHeight=%.4f\n",hdr->lpe/hdr->pe_size*10);
  fprintf(fp," VoxelWidth=%.4f\n",hdr->lro/hdr->ro_size*10);
  if (hdr->rank == 2)
    fprintf(fp," VoxelDepth=%.4f\n",hdr->thk);
  else
    fprintf(fp," VoxelDepth=%.4f\n",hdr->lpe/hdr->pe2_size*10);
  fprintf(fp,"EndInformation\n");
  fprintf(fp,"MoreInformation=-1\n");
  fprintf(fp,"Vol Slc  Offset    Length       Cmp Format\n");
  fprintf(fp,".CONTIG\n");
  fprintf(fp,"EndSliceTable\n");

  /* Fill in more zeros than we need */
  for (n = 0; n < AVW_OFFSET; n++)
    fprintf(fp,"#");
  
  
  fclose(fp);
} /* End write_avw_header */




/*******************************************************************/  
/* Write AVW file using "brute force"                              */
/* approach, ie not using Analyze functions                        */
/*******************************************************************/  
int write_avw(filename, data, hdr)
  char *filename;
float *data;
fdf_header *hdr;

 {
  FILE *fp;
  int  n,slice,datasize,pixel;
  float Smax, Smin;
  float tmp;
  char fn[MAXSTR];

  datasize = hdr->datasize;

  if (hdr->rank == 3) {
    if (hdr->slices > 1) {
      fprintf(stderr,"Sorry, don't know how to handle multi-slab 3D data\n");
      exit(0);
    }
  }
  
  /* Determine max & min signal */
  Smax = -1; Smin = 1e6;
  for (slice = 0; slice < hdr->slices; slice++) {  
    for (n = 0; n < datasize; n++) {
      pixel = slice*datasize + n;
      if (Smax < data[pixel]) Smax = data[pixel];
      if (Smin > data[pixel]) Smin = data[pixel];
    }
  }
  hdr->Smax = Smax;
  hdr->Smin = Smin;

  //  write_avw_header(filename,hdr);
  {
    tmp=hdr->pe_size;
    hdr->pe_size=hdr->ro_size;
    hdr->ro_size=tmp;
  }
  write_avw_hdr(filename,hdr);

  (void)strcpy(fn,filename);
  (void)strcat(fn,".img");
 
  if ((fp = fopen(fn,"w")) == NULL) {
    fprintf(stderr,"Can't open file %s for writing\n",filename);
    exit(0);
  }

  //  fseek(fp,AVW_OFFSET,SEEK_SET);

  for (slice = 0; slice < hdr->slices; slice++) {

	  /*
   if (hdr->rank == 2)  // 2D, flip slice up-down
      flipud2D(&data[slice*datasize],hdr);
    else // 3D, rotate planes counter-clockwise
      rotate3D(data,hdr,-1);
*/

    /* Really should check type */
    n = fwrite(&data[slice*datasize],sizeof(float),datasize,fp);
    if (n != datasize) {
      printf("Problem writing enough data, only %d of %d\n",n,datasize);
      exit(0);
    }
  }
  fclose(fp);
  
  return(0);

} /* End write_avw */


#ifdef NOTLIKELY

/*******************************************************************/  
/* write_AVW :                                                     */
/*   write AVW file using Analyze functions                        */
/*******************************************************************/  
int write_AVW(char *filename, float *data, fdf_header *hdr, char *inputDir) {
  //Analyze variables
  AVW_Volume	*initVol=NULL, *Vol_resize = NULL, *outVolume = NULL;
  AVW_ImageFile *file_info;

  float ro_size, pe_size, pe2_size, slices, min_size,
        lro, lpe, lpe2, thk,
	psi, phi, theta, ax90, cor90, sag90;
  char  string[MAXSTR];
  FILE *fp;
  int   write_ok;

  // Determine size of image in pixels & mm
  ro_size  = hdr->ro_size;
  pe_size  = hdr->pe_size;
  pe2_size = hdr->pe2_size;
  lro      = hdr->lro*10;
  lpe      = hdr->lpe*10;
  lpe2     = hdr->lpe2*10;
  
  // If this is 3D data, then "slices" is really the PE2 dimension
  if (hdr->rank == 2) {
    slices = hdr->slices;
    thk      = hdr->thk;
  }
  else {
    slices = pe2_size;
    thk    = lpe2/pe2_size;
  }

  // Create initial Volume
  initVol = AVW_CreateVolume(data, ro_size, pe_size, slices, AVW_FLOAT);


  // Make sure the voxels are isotropic
  min_size = (float)min((double)(lro/ro_size),(double)(lpe/pe_size));
  ro_size *= (lro/ro_size/min_size);
  pe_size *= (lpe/pe_size/min_size);

  Vol_resize = AVW_ResizeVolume(initVol,ro_size,pe_size,slices,AVW_RESIZE_LINEAR_ID,NULL);

  // Rotate images
  if (hdr->rank == 2) {
    outVolume = AVW_FlipVolume(Vol_resize, AVW_FLIPY, outVolume);
  }
  else {
    outVolume = rotateVolume90(Vol_resize, AVW_COUNTERCLOCKWISE, AVW_TRANSVERSE);
  }

  // Possibly rotate images again, if orientation is trans90, cor90 or sag90
  psi = hdr->psi; phi = hdr->phi; theta = hdr->theta;  
  ax90  = ((psi ==  0) && (phi == 90) && (theta ==  0));
  cor90 = ((psi ==  0) && (phi == 90) && (theta == 90));
  sag90 = ((psi == 90) && (phi == 90) && (theta == 90));
  if (ax90 || cor90 || sag90) {
    outVolume = rotateVolume90(outVolume, AVW_COUNTERCLOCKWISE, AVW_TRANSVERSE);
  }
 
  // Write AVW to disk
  ro_size = outVolume->Width;
  pe_size = outVolume->Height;
  slices  = outVolume->Depth;
  if ((file_info = AVW_CreateImageFile(filename,"AnalyzeAVW",ro_size,pe_size,slices,AVW_FLOAT))== NULL) {
    sprintf(string,"Create file %s failed", filename);
    AVW_Error(string);
    exit(0);
  }
  file_info->Info = AVW_PutNumericInfo("VoxelWidth",  (double)min_size, file_info->Info);
  file_info->Info = AVW_PutNumericInfo("VoxelHeight", (double)min_size, file_info->Info);
  file_info->Info = AVW_PutNumericInfo("VoxelDepth",  (double)thk, file_info->Info);
  file_info->Info = AVW_PutStringInfo("FDFSourceDir", inputDir, file_info->Info);
  if((write_ok = AVW_WriteVolume(file_info, 0, outVolume)) != AVW_SUCCESS) {
    sprintf(string,"Write to %s failed", filename);
    AVW_Error(string);
    AVW_CloseImageFile(file_info);
    exit(0);
  }
  AVW_CloseImageFile(file_info);


  // Clean up
  if (initVol    != NULL) AVW_DestroyVolume(initVol);
  if (Vol_resize != NULL) AVW_DestroyVolume(Vol_resize);
  if (outVolume  != NULL) AVW_DestroyVolume(outVolume);

  return(write_ok);
}



/*******************************************************************/  
/* rotateVolume90:                                                 */
/*   Rotate AVW volume; imported directly from Rick Carano's code  */
/*******************************************************************/  
AVW_Volume *rotateVolume90(AVW_Volume *inputVolume, int direction, int orientation) {
  int        xDim, yDim, zDim, i;
  AVW_Image  *image = NULL, *pimage = NULL;
  AVW_Volume *tempVolume;

  xDim = inputVolume->Width;
  yDim = inputVolume->Height;
  zDim = inputVolume->Depth;

  tempVolume = AVW_CreateVolume(NULL, yDim, xDim, zDim, inputVolume->DataType);

  for (i = 0; i < zDim; i++) {
    image  = AVW_GetOrthogonal(inputVolume, orientation, i, image);
    pimage = AVW_Rotate90Image(image, direction, pimage);
    AVW_PutOrthogonal(pimage, tempVolume, orientation, i);
  }

  AVW_DestroyImage(image);
  AVW_DestroyImage(pimage);

  return(tempVolume);
}

#endif
