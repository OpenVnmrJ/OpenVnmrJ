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
/* circ: Approximates object with circle */


#include "imagemath.h"
#include "image_utils.h"


int mathfunc() {
  float thr, X, fract; 
  int pixel, im, r, c, r0, c0, r1, c1, r2, c2, rad, d, dr, dc;
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

printf("Radius is %d\n",rad);
printf("Center is %d,%d\n",r0,c0);

    for (r = 0; r < img_height; r++) {
      for (c = 0; c < img_width; c++) {
        pixel = r*img_width + c;
        X = in_data[im][pixel];
	
	dr = (int) (100*100*(r-r0)*(r-r0)/(img_height*img_height));
	dc = (int) (100*100*(c-c0)*(c-c0)/(img_width*img_width));
        d = (int) sqrt((double) (dr + dc));  /* distance from center */
        if ((X > thr) && (d <= rad*fract)) {
          out_data[im][pixel] = in_data[im][pixel];
	  printf("Use pixel (%d, %d) at distance %d (%d, %d)\n",
	    r,c,d,dr,dc);
	}
        else
          out_data[im][pixel] = 0;
      }
    }
  }  /* end image loop */
  return TRUE;
}


