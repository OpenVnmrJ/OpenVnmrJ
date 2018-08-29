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
/* object: Locates object */


#include "imagemath.h"
#include "image_utils.h"


int mathfunc() {
  float thr, X, fract; 
  int pixel, im, r, c, r0, c0, r1, c1, r2, c2, rad, d;
  char msg[1024];

  if (nbr_infiles<1 || input_sizes_differ || !want_output(0)){
    return FALSE;
  }

  if (nbr_outfiles != in_vec_len[0]) {
    sprintf(msg,"object: You must supply as many output images as input images\n");
    ib_errmsg(msg);
    return FALSE;
  }
  
  create_output_files(nbr_infiles, in_object[0]);

  fract = 1.0;
  if (nbr_params > 0)
    fract = in_params[0]/100;

  /**************************/
  /* Create output images ***/
  /**************************/
  for (im = 0; im < nbr_infiles; im++){
    thr = threshold(in_data[im],img_height,img_width);
    find_object(in_data[im],thr,img_height,img_width,
      &r0,&c0,&r1,&c1,&r2,&c2,&rad);

    for (r = 0; r < img_height; r++) {
      for (c = 0; c < img_width; c++) {
        pixel = r*img_width + c;
        X = in_data[im][pixel];
        d = (int) sqrt((double) ((r-r0)*(r-r0) + (c-c0)*(c-c0)));  /* distance from center */
        if ((X > thr) && (r >= r1) && (r <= r2) && (c >= c1) && (c <= c2))
          out_data[im][pixel] = in_data[im][pixel];
        else
          out_data[im][pixel] = 0;
      }
    }
  }  /* end image loop */
  
  return TRUE;
}


