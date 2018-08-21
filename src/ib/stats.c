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
/* stats: Calculate statistics for a time series of images */
/* Output is 
     1 = mean
     2 = standard deviation
     3 = snr = mean/std
     4 = peak-to-peak deviation
*/

#include "imagemath.h"
#include "image_utils.h"

int mathfunc() {
  double sum, sum_x2, mean, std, min, max, X, thr; 
  double av_mean,av_std, av_pp, max_std, min_std, max_pp, min_pp;
  float  fract;
  float  **tmp_data;
  int    pixel, im, N, Nfilter;
  int    r,c, r0, c0, r1, r2, c1, c2;
  int    rad, d, do_filter;
  int    n1, n2, n3, n4;
  char   msg[1024],pgm[1024];
  enum  {nstdp, npp, nmean, nstd};  /* output images 0,1,2,3 */
  
  FILE *fp;
    
  if (nbr_infiles<1 || input_sizes_differ || !want_output(0))
    return FALSE;

  create_output_files(4, in_object[0]);

  /*************************************/
  /* Get input parameters              */
  /*************************************/
  /* Comment */
  strcpy(pgm,"stats");
  if (nbr_strings > 0) {
    strcpy(pgm,in_strings[0]);
  }

  /* Filter size */
  if ((img_width < 128) || (img_height < 128))
    Nfilter = 5; /* for small matrices, default to a 5x5 filter */
  else
    Nfilter = 11;  /* else default to a 11x11 filter */
  do_filter = 1;
  if (nbr_params > 0)
    Nfilter = in_params[0];  /* filter kernel size */

  /* How much of the image should we use? */
  if (nbr_params > 1)
    fract = in_params[1]/100;
  else
    fract = 0.80; /* default to 80% */


  /* Zero center pixels in output images? */
  if (nbr_strings > 1) {
    if (!strcmp("nodc",in_strings[1])) {
      r1 = r2 = c1 = c2 = -1;
      n1 = n2 = n3 = n4 = -1; /* default to zero'ing center pixel */
    }
  }
  else {
    /* Four pixels in center of image - avoid DC artifact */
    r1 = img_height/2-1;
    r2 = r1 + 1;
    c1 = img_width/2-1;
    c2 = c1 + 1;

    n1 = (r1-1)*img_width + c1;
    n2 = (r1-1)*img_width + c2;
    n3 = (r2-1)*img_width + c1;
    n4 = (r2-1)*img_width + c2;

  }

  /* Get threshold for segmenting image */
  thr = threshold(in_data[0],img_height,img_width);
  /* Find object based on first image */
  find_object(in_data[0],thr,img_height,img_width,
      &r0,&c0,&r1,&c1,&r2,&c2,&rad);

  /* Optional filtering of input images */
  if (Nfilter > 0) { /* Filter all images */
    do_filter = 1;      
    if ((tmp_data = (float **) malloc(sizeof(float *)*nbr_infiles)) == NULL)
      ib_errmsg("MALLOC ERROR 1");      
    for (im = 0; im < nbr_infiles; im++)
      if ((tmp_data[im] = (float *) malloc(sizeof(float)*img_size)) == NULL)
        ib_errmsg("MALLOC ERROR 2");      

    for (im = 0; im < nbr_infiles; im++)
      filter(in_data[im],tmp_data[im],Nfilter,img_height,img_width,thr);
  }

  /* Initialize output image */
  for (r = 0; r < img_height; r++) {
    for (c = 0; c < img_width; c++) {
      pixel = r*img_width + c;

      if (want_output(nmean)) out_data[nmean][pixel] = 0;
      if (want_output(nstd))  out_data[nstd][pixel]  = 0;
      if (want_output(nstdp)) out_data[nstdp][pixel] = 0;
      if (want_output(npp))   out_data[npp][pixel]   = 0;
    }
  }


  /* Loop through every pixel */
  av_std = av_pp = 0;
  
  for (r = r1; r < r2; r++){
    for (c = c1; c < c2; c++){
      pixel = r*img_width + c;
      d = (int) sqrt((double) ((r-r0)*(r-r0) + (c-c0)*(c-c0)));  /* distance from center */

      sum = sum_x2  = 0.0;
      min = 1e6; 
      max = 0.0;
      mean = std = 0;
      N = 0;
      
      if ((pixel != n1) && (pixel != n2) && (pixel != n3) && (pixel != n4)) {  /* skip center pixels */
        if (in_data[0][pixel] > thr) {
          /* Loop through all images in input vector */
          for (im = 0; im < nbr_infiles; im++) {
            if (do_filter)
              X = tmp_data[im][pixel];
            else
              X = in_data[im][pixel];

            sum    += X;
            sum_x2 += (X*X);
      
            if (X < min) min = X;
            if (X > max) max = X;
            
            N++;
          }  /* end of image loop */
          mean = sum/N;
          std  = (double) sqrt((double)((sum_x2/N) - (mean*mean)));

        }  /* End check threshold */
      } /* End check center pixel */

/* DEBUG
if (((r == 37) && (c == 28)) || ((r == 32) && (c == 25))){
  printf("At (%d,%d), max, min, mean, pp, std is %f, %f, %f, %f, %f\n",
    c,r,max, min, mean, (max - min)/mean*100.0,std);
  printf("sum, sumsq, sumsq/N, mean^2, sqr = %f, %f, %f, %f, %f, N = %d\n",
    sum,sum_x2*1000,sum_x2/N*1000*1000, mean*mean*1000*1000,(sum_x2/N - mean*mean)*1000*1000,N);
}
/**/
      if (want_output(nmean)) out_data[nmean][pixel] = mean;
      if (want_output(nstd))  out_data[nstd][pixel]  = std;
      if (want_output(nstdp)) {
        if (mean != 0)
          out_data[nstdp][pixel] = std/mean*100;
        else
          out_data[nstdp][pixel] = 0;
      }
      if (want_output(npp)) {
        if (mean != 0)
          out_data[npp][pixel] = (max - min)/mean*100.0;     
        else
          out_data[npp][pixel] = 0;
      }
    } /* end columns loop */  
  }  /* end rows loop */


  /* Get average standard deviation and PP within image */
  av_std = av_pp = 0; 
  max_std = max_pp = 0;
  min_std = min_pp = 1e6;
  N = 0;
  for (r = 0; r < img_height; r++){
    for (c = 0; c < img_width; c++){
      pixel = r*img_width + c;
      d = (int) sqrt((double) ((r-r0)*(r-r0) + (c-c0)*(c-c0)));  /* distance from center */
      if ((pixel != n1) && (pixel != n2) && (pixel != n3) && (pixel != n4)) {  /* skip center pixels */
        if ((d < rad) && (in_data[0][pixel] > thr)) {
	  N++;
          X = out_data[nstdp][pixel];
          av_std += X;
          if (max_std < X) max_std = X;
          if (min_std > X) min_std = X;

          if (want_output(npp)) {
            X = out_data[npp][pixel];
            av_pp += X;
            if (max_pp < X) max_pp = X;
            if (min_pp > X) min_pp = X;
          }
        }
      }
    }
  }
	

  if (N > 0) {
    printf("=========== STATS: %s ===================\n",pgm);
    printf("Average standard deviation in %d pixels is %.2f%%\n",N,av_std/N);
    printf("Standard deviations range from %.2f%% to %.2f%%\n",min_std,max_std);
    if (want_output(npp)) {
      printf("Average peak-to-peak is %.2f%%\n",av_pp/N);
      printf("Peak-to-peak range from %.2f%% to %.2f%%\n",min_pp,max_pp);
    }


    if ((fp = fopen("STAB_measurements.txt","a")) == NULL) {
      sprintf(msg,"Can't open STAB_measurements.txt for printing results");
      ib_errmsg(msg);
      return FALSE;
    }
    fprintf(fp,"=========== STATS: %s ===================\n",pgm);
    fprintf(fp,"Average standard deviation in %d pixels is %.2f%%\n",N,av_std/N);
    fprintf(fp,"Standard deviations range from %.2f%% to %.2f%%\n",min_std,max_std);
    if (want_output(npp)) {
      fprintf(fp,"Average peak-to-peak is %.2f%%\n",av_pp/N);
      fprintf(fp,"Peak-to-peak range from %.2f%% to %.2f%%\n",min_pp,max_pp);
    }
    fclose(fp);

  }

  return TRUE;
}
