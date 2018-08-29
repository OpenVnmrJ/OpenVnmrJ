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
/* mean: calculate mean image of all input images */

#include "imagemath.h"

int mathfunc() {
  int pixel, im;
  float sum;  
    
  if (nbr_infiles<1 || input_sizes_differ || !want_output(0)){
    return FALSE;
  }
  create_output_files(1, in_object[0]);

  /* Loop through every pixel */
  for (pixel = 0; pixel < img_size; pixel++){
    sum = 0;
    /* Loop through all images in input vector */
    for (im = 0; im < nbr_infiles; im++){
      sum = sum + in_data[im][pixel];   
    }  /* end of image loop */

    if (want_output(0)){
       out_data[0][pixel] = sum/nbr_infiles;
    }
    
  }  /* end pixel loop */
  return TRUE;
}
