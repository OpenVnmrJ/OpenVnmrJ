/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
/* baseline: subtract mean of baseline from all images */
/* Output is all images with baseline subtracted */
/* Optional second input is the list of images to use as baseline */
/* Example: ##101-200 = baseline ##1-100 ##1-10 */

#include "imagemath.h"
#define PGM "baseline"


int mathfunc() {
  float Baseline, X; 
  int pixel, im;
  char msg[1024];

  if (nbr_infiles<1 || input_sizes_differ || !want_output(0)){
    return FALSE;
  }

  if (nbr_outfiles != in_vec_len[0]) {
    sprintf(msg,"%s: The number of output images (%d) must match number of input images (%d)\n",
      PGM,nbr_outfiles,in_vec_len[0]);
    ib_errmsg(msg);
    return FALSE;
  }
  
  create_output_files(nbr_infiles, in_object[0]);

  /* Loop through every pixel */
  for (pixel = 0; pixel < img_size; pixel++){

    if (nbr_image_vecs < 2)
      Baseline = in_data[0][pixel];
    else {
      Baseline = 0;
      for (im = vecindx[1]; im < vecindx[1]+in_vec_len[1]; im++)
        Baseline += in_data[im][pixel];
      Baseline /= in_vec_len[1];
    }

    /* Loop through all images in input vector */
    for (im = 0; im < nbr_infiles; im++){
      X = in_data[im][pixel];
    
      if (want_output(im)){
        out_data[im][pixel] = X - Baseline;
      }
    
    }  /* end of image loop */
  }  /* end pixel loop */
  return TRUE;
}
