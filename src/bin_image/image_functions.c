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

/*******************************************************************/  
/* Flip 2D image up down                                           */
/*******************************************************************/  
void flipud2D(float *data,fdf_header *hdr) {
  int    r,p,pixel,pixelud;
  float *dataud;
  
  if ((dataud = (float *)malloc(hdr->datasize*sizeof(float))) == NULL) nomem();

  for (r = 0; r < hdr->ro_size; r++){
    for (p = 0; p < hdr->pe_size; p++) {
      pixel   = r*hdr->pe_size + p;
      pixelud = (hdr->ro_size - r - 1)*hdr->pe_size + p;
      dataud[pixelud] = data[pixel];
    }
  }
  
  /* Put it back in original image */
  for (pixel = 0; pixel < hdr->datasize; pixel++) 
    data[pixel] = dataud[pixel];

  free(dataud);
}
/*******************************************************************/  
/* Flip 2D image left-right                                        */
/*******************************************************************/  
void fliplr2D(float *data,fdf_header *hdr) {
  int    r,p,pixel,pixelud;
  float *dataud;
  
  if ((dataud = (float *)malloc(hdr->datasize*sizeof(float))) == NULL) nomem();

  for (r = 0; r < hdr->ro_size; r++)
    for (p = 0; p < hdr->pe_size; p++) {
      pixel   = r*hdr->pe_size + p;
      pixelud = r*hdr->pe_size + (hdr->pe_size - p - 1);
      dataud[pixelud] = data[pixel];
    }

  /* Put it back in original image */
  for (pixel = 0; pixel < hdr->datasize; pixel++) 
    data[pixel] = dataud[pixel];

  free(dataud);
}


/*******************************************************************/  
/* Rotate 2D image clockwise                                       */
/*******************************************************************/  
void rotate2D(float *data,fdf_header *hdr,int dir) {
  int    r,p,pixel,pixelud;
  float *dataud;
  
  /*
  if (hdr->ro_size != hdr->pe_size) {
    printf("Sorry, don't know how to handle rectangular images (%.f x %.f)",hdr->ro_size,hdr->pe_size);
    exit(0);
  }
  */
     
  if ((dataud = (float *)malloc(hdr->datasize*sizeof(float))) == NULL) nomem();

  for (r = 0; r < hdr->ro_size; r++) {
    for (p = 0; p < hdr->pe_size; p++) {
      pixel   = r*hdr->pe_size + p;
      if (dir == -1) //rotate counter-clockwise
	//  	pixelud = p*hdr->ro_size + (hdr->pe_size - r - 1);
  	pixelud = p*hdr->ro_size + (hdr->ro_size - r - 1);
      else //rotate clockwise
	//  	pixelud = (hdr->pe_size-p-1)*hdr->pe_size + r;
  	pixelud = (hdr->pe_size-p-1)*hdr->ro_size + r;
      dataud[pixelud] = data[pixel];
    }
  }


  /* Put it back in original image */
  for (pixel = 0; pixel < hdr->datasize; pixel++) 
    data[pixel] = dataud[pixel];

  free(dataud);
}



/*******************************************************************/  
/* Flip 3D image up down                                           */
/*******************************************************************/  
void fliplr3D(float *data,fdf_header *hdr) {
  int    r,p,s,pixel,pixelud;
  float *dataud;
  
  
  if ((dataud = (float *)malloc(hdr->datasize*sizeof(float))) == NULL) nomem();

  for (s = 0; s < hdr->pe2_size; s++) {
    for (r = 0; r < hdr->ro_size; r++) {
      for (p = 0; p < hdr->pe_size; p++) {
	pixel   = s*hdr->ro_size*hdr->pe_size + r*hdr->pe_size + p;
	pixelud = s*hdr->ro_size*hdr->pe_size + (hdr->ro_size - r - 1)*hdr->pe_size + p;
	dataud[pixelud] = data[pixel];
      }
    }
  }

  /* Put it back in original image */
  for (pixel = 0; pixel < hdr->datasize; pixel++) 
    data[pixel] = dataud[pixel];

  free(dataud);
}



/*******************************************************************/  
/* Flip 3D image left right                                        */
/*******************************************************************/  
void flipud3D(float *data,fdf_header *hdr) {
  int    r,p,s,pixel,pixelud;
  float *dataud;
  
  
  if ((dataud = (float *)malloc(hdr->datasize*sizeof(float))) == NULL) nomem();

  for (s = 0; s < hdr->pe2_size; s++) {
    for (r = 0; r < hdr->ro_size; r++) {
      for (p = 0; p < hdr->pe_size; p++) {
	pixel   = s*hdr->ro_size*hdr->pe_size + r*hdr->pe_size + p;
	pixelud = s*hdr->ro_size*hdr->pe_size + r*hdr->pe_size + (hdr->pe_size - p - 1);
	dataud[pixelud] = data[pixel];
      }
    }
  }

  /* Put it back in original image */
  for (pixel = 0; pixel < hdr->datasize; pixel++) 
    data[pixel] = dataud[pixel];

  free(dataud);
}


/*******************************************************************/  
/* Rotate 3D image clockwise                                       */
/*******************************************************************/  
void rotate3D(float *data,fdf_header *hdr,int dir) {
  int    r,p,s,pixel,pixelud;
  float *dataud;
  
  /*
  if (hdr->ro_size != hdr->pe_size) {
    printf("Sorry, don't know how to handle rectangular matrices(%.f x %.f)",hdr->ro_size,hdr->pe_size);
    exit(0);
  }
  */
     
  if ((dataud = (float *)malloc(hdr->datasize*sizeof(float))) == NULL) nomem();

  for (s = 0; s < hdr->pe2_size; s++) {
    for (r = 0; r < hdr->ro_size; r++) {
      for (p = 0; p < hdr->pe_size; p++) {
	pixel   = s*hdr->ro_size*hdr->pe_size + r*hdr->pe_size + p;
        if (dir == -1) //rotate counter-clockwise
	  //  	  pixelud = s*hdr->ro_size*hdr->pe_size + p*hdr->ro_size + (hdr->pe_size - r - 1);

  	  pixelud = s*hdr->ro_size*hdr->pe_size + p*hdr->ro_size + (hdr->ro_size - r - 1);
	else //rotate clockwise
	  //  	  pixelud = s*hdr->ro_size*hdr->pe_size + (hdr->pe_size-p-1)*hdr->pe_size + r;
  	  pixelud = s*hdr->ro_size*hdr->pe_size + (hdr->pe_size-p-1)*hdr->ro_size + r;
	dataud[pixelud] = data[pixel];
      }
    }
  }

  /* Put it back in original image */
  for (pixel = 0; pixel < hdr->datasize; pixel++) 
    data[pixel] = dataud[pixel];

  free(dataud);
}



/*******************************************************************/  
/* Scale image by multiplication factor                            */
/*******************************************************************/  
void image_scale(float *data,fdf_header *hdr, float F) {
  int    pixel;
  
  for (pixel = 0; pixel < hdr->datasize; pixel++) data[pixel] *= F;
  hdr->Smax *= F;
  hdr->Smin *= F;
}



/***********************************************/
/* Find optimal threshold based on histograms  */
/***********************************************/
/* Sub-function to find new threshold */
float new_thr(float *data, int datapts, float thr0) {
  float mean1, mean2, v, thr;
  int   n1, n2, inx;

  mean1 = mean2 = 0;
  n1    = n2    = 0;

  for (inx = 0; inx < datapts; inx++) {
    v = data[inx];
    if (v < thr0) {
      mean1 += v;
      n1++;
    }
    else {
      mean2 += v;
      n2++;
    }
  }
  mean1 /= n1;
  mean2 /= n2;

  /* Next guess for threshold */
  thr = (mean1 + mean2)/2;
  return(thr);
}

/* Main thresholding function */
float threshold(float *data, fdf_header *hdr) {
  int   img_size, pixel;
  float meanN, meanS;
  float thr0, thr1;
  
//  img_size = hdr->ro_size*hdr->pe_size;
  img_size = hdr->datasize;
  
  /* for initial noise estimate, assume that the first 4 rows are noise */        
  meanN = 0;
  for (pixel = 0; pixel < 4*hdr->ro_size; pixel++)
    meanN += data[pixel];
  meanN /= (4*hdr->ro_size);

  /* Initial rough estimate of mean signal intensity */
  meanS = 0;
  for (pixel = 0; pixel < img_size; pixel++)
    meanS += data[pixel];
  meanS /= img_size;

  /* Initial guesses for threshold between the two classes */
  thr0 = (meanS + meanN)/2;
  thr1 = new_thr(data,img_size,thr0);
  
  /* Now do the same in an iterative fashion */
  while (fabs((thr1 - thr0) / (thr1 + thr0)) < 1e-5) { // 1e-5 seems reasonable
    thr0 = thr1;
    thr1 = new_thr(data,img_size,thr0);
  }

  return(thr1);
}


