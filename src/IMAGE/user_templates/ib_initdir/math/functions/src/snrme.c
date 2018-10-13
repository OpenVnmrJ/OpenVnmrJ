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
/* SNRME: Measures the SNR, ghosting level, and image uniformity */
/*        in a series of images; it is assumed that they're all the */
/*        same object, e.g., a multi-echo experiment */
/* Output image is a mean filtered image */


#include "imagemath.h"
#include "image_utils.h"


int mathfunc() {
  double meanS, noise, meanG, maxS, minS, sum2, stdS, thr, X, minmax;
  int   pixel, pixel2, im, n1, n2, r, c, r1, c1, rG, cG, rS1, rS2, cS1, cS2;
  int   im_r1, im_r2, im_c1, im_c2, r0, c0, rad, d,
        Nfilter, NF2, N, skip_pix, pixelG;
  double fract;
  char msg[1024],pgm[1024];
  FILE  *fp;

  if (nbr_infiles<1 || input_sizes_differ){
    return FALSE;
  }


  if (nbr_outfiles != in_vec_len[0]) {
    sprintf(msg,"Math: You must supply as many output images as input images\n");
    ib_errmsg(msg);
    return FALSE;
  }


  /**************************/
  /* Create output images ***/
  /**************************/
  if (want_output(1))
    create_output_files(nbr_infiles*2, in_object[0]);
  else
    create_output_files(nbr_infiles, in_object[0]);

  /* How large a (mean) filter do we apply to find the ghosting level */
  if ((img_width < 128) || (img_height < 128))
    Nfilter = 5; /* for small matrices, default to a 5x5 filter */
  else
    Nfilter = 11;  /* else default to a 11x11 filter */
  if (nbr_params > 0) Nfilter = (int) in_params[0];
  NF2 = (int) (Nfilter/2);

  fract = 0.8;
  if (nbr_params > 1)
    fract = in_params[1]/100;

  strcpy(pgm,"SNR");
  if (nbr_strings > 0)
    strcpy(pgm,in_strings[0]);

  /**************************/
  /* Calculations ***********/
  /**************************/
  /* Get threshold for segmenting image */
  thr = threshold(in_data[0],img_height,img_width);

  /* Find image boundaries (radius) *********/
  find_object(in_data[0],thr,img_height,img_width,
    &r0,&c0,&im_r1,&im_c1,&im_r2,&im_c2,&rad);
  /* rad is the smallest radius; im_r1/c1/r2/c2 gives maximum extent of object */

  /* Find noise standard deviation **************/
  noise = find_noise(in_data[0],im_r1-3, im_c1-3, im_r2+3, im_c2+3);


  for (im = 0; im < nbr_infiles; im++){

    /* Apply a NxN mean filter to the image */
    filter(in_data[im],out_data[im],Nfilter,img_height,img_width,thr);

    /***********************************************/
    /* Calculate signal intensity and uniformity ***/
    /***********************************************/
    meanS = sum2 = n2 = 0;
    maxS = 0; minS = 1e6;
    for (r = 0; r < img_height; r++) {
      for (c = 0; c < img_width; c++) {
        pixel = r*img_width + c;
        X = in_data[im][pixel];
        d = (int) sqrt((double) ((r-r0)*(r-r0) + (c-c0)*(c-c0)));  /* distance from center */
        if (d <= rad*fract) {
            meanS += X;
            sum2  += (X*X);
            n2++;

            minmax = out_data[im][pixel];
            /* Find max/min filtered signal intensity */
            if (maxS < minmax) {maxS = minmax; rS1=r; cS1 = c;}
            if (minS > minmax) {minS = minmax; rS2=r; cS2 = c;}

        }
      }
    }
    meanS /= n2;
    stdS  = (float) sqrt((double)((sum2/n2) - (meanS*meanS)));


    /******************************************/
    /* Calculate ghost intensity **************/
    /******************************************/
    /* Assume ghosting is in horizontal direction   */
    /* Search +/- Nfilter columns beyond maximum extent of object */
    meanG = 0;
    cG = rG = pixelG = 0;
    skip_pix = (Nfilter > 3 ? Nfilter : 3);
    for (r = im_r1-1; r <= im_r2; r++) {
      /* Check to the left of the image */
      for (c = skip_pix; c < im_c1-skip_pix; c++) {
        pixel = r*img_width + c;
        X = out_data[im][pixel];
        if (X > meanG) {
          meanG = X;
          rG = r; cG = c; pixelG = pixel;
        }
      }
      /* Check to the right of the image */
      for (c = im_c2+skip_pix; c < img_width-skip_pix; c++) {
        pixel = r*img_width + c;
        X = out_data[im][pixel];
        if (X > meanG) {
          meanG = X;
          rG = r; cG = c; pixelG = pixel;
        }
      }
    }

    for (r = 0; r < img_height; r++) {
      for (c = 0; c < img_width; c++) {
        pixel = r*img_width + c;
        X = in_data[im][pixel];
        d = (int) sqrt((double) ((r-r0)*(r-r0) + (c-c0)*(c-c0)));  /* distance from center */
        if (!(d <= rad*fract)) {
          out_data[im][pixel] = 0;
        }
      }
    }
    out_data[im][pixelG] = meanG;




    printf("=========== %s: image %d ===================\n",pgm,im+1);
    printf("Signal, Noise, Ghosting (x100): %.6f, %.6f, %.6f\n",maxS*100,noise*100, meanG*100);
    printf("SNR: %.f (NEMA standard: %.f)\n",maxS/noise, maxS/noise*1.253);
    printf("Ghosting: %.2f%% of max signal (in %dx%dROI)\n",(meanG-noise)/maxS*100,Nfilter,Nfilter);
    printf("Maximum ghosting is in pixel %d, %d\n",cG,rG);
    printf("Percent Image Uniformity is%.2f%%\n",(1-(maxS-minS)/(maxS+minS))*100);
    printf("minS, maxS = %f and %f at (%d,%d),(%d,%d)\n",minS*100,maxS*100,cS2,rS2,cS1,rS1);
/*    printf("Image variation is %.f%%\n",stdS/meanS*100); */

    if ((fp = fopen("SNR_measurements.txt","a")) == NULL) {
      sprintf(msg,"Can't open file SNR_measurements.txt for printing results");
      ib_errmsg(msg);
      return FALSE;
    }
    fprintf(fp,"=========== %s: image %d ===================\n",pgm,im+1);
    fprintf(fp,"Signal, Noise, Ghosting (x100): %.6f, %.6f, %.6f\n",maxS*100,noise*100, meanG*100);
    fprintf(fp,"SNR: %.f (NEMA standard: %.f)\n",maxS/noise, maxS/noise*1.253);
    fprintf(fp,"Ghosting: %.2f%% of max signal (in %dx%dROI)\n",(meanG-noise)/maxS*100,Nfilter,Nfilter);
    fprintf(fp,"Maximum ghosting is in pixel %d, %d\n",cG,rG);
    fprintf(fp,"Percent Image Uniformity is%.2f%%\n",(1-(maxS-minS)/(maxS+minS))*100);
    fprintf(fp,"minS, maxS = %f and %f at (%d,%d),(%d,%d)\n",minS*100,maxS*100,cS2,rS2,cS1,rS1);
    fclose(fp);

  }  /* end image loop */

  return TRUE;
}


