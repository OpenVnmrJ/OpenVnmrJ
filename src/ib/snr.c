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
/* SNR: Measures the SNR, ghosting level, and image uniformity */
/* Output image is a mean filtered image */


#include "imagemath.h"
#include "image_utils.h"


int mathfunc() {
  double meanS,noise,meanG,maxSf,maxS,minS,sum2,stdS,thr,X,minmax,minmaxf;
  int    pixel,pixel2,im,n1,n2,r,c,r1,c1,rG,cG,rS1,rS2,cS1,cS2,rS1f,cS1f;
  int    im_r1,im_r2,im_c1,im_c2,r0,c0,rad,
         Nfilter,NF2,N,skip_pix,pixelG;
  double lro,lpe,L1=0,L2=0,fract;
  double d,dr,dc;
  int    next,
         l1,l2,
	 Pmax,Pmin;
  
  char   msg[1024],pgm[1024];
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

  L1 = L2 = 0;
  
  next = 0;
  if (nbr_params > next) { L1 = in_params[next]; next++; }
  if (nbr_params > next) { L2 = in_params[next]; next++; }
  else L2 = L1;
  
  /* L1 and L2 should be given as the coil ID in mm */
  /* The default is to search within 80% of the coil ID */
  /* but allow the user to input this number */
  fract = 0.8;
  if (nbr_params > next) { fract = in_params[next]/100; next++; }
  /* Now adjust L1 and L2 to this fraction */
  L1 *= fract;  L2 *= fract;  

  /* How large a (mean) filter do we apply to find the ghosting level */
  if ((img_width < 128) || (img_height < 128))
    Nfilter = 5; /* for small matrices, default to a 5x5 filter */
  else
    Nfilter = 11;  /* else default to a 11x11 filter */

  if (nbr_params > next) Nfilter = (int) in_params[next];
  NF2 = (int) (Nfilter/2);

  strcpy(pgm,"SNR");
  if (nbr_strings > 0)
    strcpy(pgm,in_strings[0]);

  lro = lpe = 0;
  get_header_array_double(in_object[0],"span",0,&lpe);
  get_header_array_double(in_object[0],"span",1,&lro);

  /* and convert to pixels */
  l1 = (int) (L1/10/lpe*img_width);
  l2 = (int) (L2/10/lro*img_height);
  
  if (L1 == 0) {  // if no coil ID is given, use entire FOV
    l1 = img_width;
    l2 = img_height;
  }

  if ((fp = fopen("SNR_measurements.txt","a")) == NULL) {
    sprintf(msg,"Can't open file SNR_measurements.txt for printing results");
    ib_errmsg(msg);
    return FALSE;
  }

  /**************************/
  /* Calculations ***********/
  /**************************/
  for (im = 0; im < nbr_infiles; im++){

    /* Get threshold for segmenting image */
    thr = threshold(in_data[im],img_height,img_width);

    /* Find image boundaries (radius) *********/
    find_object(in_data[im],thr,img_height,img_width,
      &r0,&c0,&im_r1,&im_c1,&im_r2,&im_c2,&rad);
    /* rad is the smallest radius; 
       im_r1/c1/r2/c2 gives maximum extent of object */

    if (L1 == 0) {  // if no coil ID is given, use entire object
      l1 = (im_c2-im_c1)*fract;  // width
      l2 = (im_r2-im_r1)*fract;  // height
      L1 = (double)l1/img_width*lpe*10;
      L2 = (double)l2/img_height*lro*10;
    }

    /* Find noise standard deviation **************/
    noise = find_noise(in_data[im],im_r1-3, im_c1-3, im_r2+3, im_c2+3);

    /* Apply a NxN mean filter to the image */ 
    filter(in_data[im],out_data[im],Nfilter,img_height,img_width,thr);


    /***********************************************/
    /* Calculate signal intensity and uniformity ***/
    /***********************************************/
    meanS = sum2 = n2 = 0;
    maxS = maxSf = 0; minS = 1e6;
    Pmax = Pmin = 0;
    for (r = 0; r < img_height; r++) {
      for (c = 0; c < img_width; c++) {
        pixel = r*img_width + c;
        X = in_data[im][pixel];

        if (X > thr){ 
          meanS += X;
  	  sum2  += (X*X); 
   	  n2++;

          minmaxf = out_data[im][pixel];  /* Intensities of filtered image */
          minmax  = in_data[im][pixel];   /* Intensities of non-filtered */

      	  /* Find max/min of non-filtered signal intensity */
	  /* for determining uniformity                    */
	  /* But skip center pixel - could be a DC artifact here */
	  if ((r != img_height/2-1) && (c != img_width-1)) {
	    /* If it's a circle (L1=L2), check within the radius */
	    if (L1 == L2) {
	      dr = (double)(r-r0)/img_height*lro;
	      dc = (double)(c-c0)/img_width*lpe;
	      d =  (double) sqrt(dr*dr + dc*dc);  /* distance from center */

              if (d <= L1/10/2) {
        	if (maxS < minmax) {maxS = minmax; rS1 = r; cS1 = c; Pmax = pixel;}
        	if (minS > minmax) {minS = minmax; rS2 = r; cS2 = c; Pmin = pixel;}
              }

	    }
	    else {
	      /* but check within boundaries of l1 and l2 */
	      if ((c > c0-l1/2) && (c < c0+l1/2) && (r > r0-l2/2) && (r < r0+l2/2)) {
  		if (maxS < minmax) {maxS = minmax; rS1 = r; cS1 = c; Pmax = pixel;}
		if (minS > minmax) {minS = minmax; rS2 = r; cS2 = c; Pmin = pixel;}
	      }
	    }
	  }

      	  /* Find max of filtered signal intensity to measure ghost % */
  	  if (maxSf < minmaxf) {maxSf = minmaxf; rS1f = r; cS1f = c;}

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


    /******************************************/
    /* Create final image *********************/
    /******************************************/
    for (r = 0; r < img_height; r++) {
      for (c = 0; c < img_width; c++) {
        pixel = r*img_width + c;
        X = in_data[im][pixel];

	dr = (int) (100*100*(r-r0)*(r-r0)/(img_height*img_height));
	dc = (int) (100*100*(c-c0)*(c-c0)/(img_width*img_width));
        d = (int) sqrt((double) (dr + dc));  /* distance from center */

        if (X <= thr) { 
	  out_data[im][pixel] = 0;
        }
      }
    }


    out_data[im][pixelG] = meanG;  // Single bright pixel in maximum ghost
    out_data[im][Pmax]   = maxS*10;  // Single bright pixel in maximum ghost
    out_data[im][Pmin]   = maxS*10;  // Single bright pixel in maximum ghost


    printf("=========== %s: image %d ===================\n",pgm,im+1);
    printf("Signal, Noise, Ghosting (x100): %.6f, %.6f, %.6f\n",maxSf*100, noise*100, meanG*100);
    printf("SNR: %.f (NEMA standard: %.f)\n",maxSf/noise, maxSf/noise*1.253);
    printf("Ghosting: %.2f%% of max signal (in %dx%d ROI)\n",(meanG-noise)/maxSf*100,Nfilter,Nfilter);
    printf("Maximum ghosting is in pixel %d, %d\n",cG,rG);
    printf("Maximum (filtered) intensity is in pixel %d, %d\n",cS1f,rS1f);
    printf("Percent Image Uniformity is %.2f%%\n",(1-(maxS-minS)/(maxS+minS))*100);
    printf("Unfiltered minS, maxS = %f and %f at (%d,%d), (%d,%d)\n",minS*100,maxS*100,cS2,rS2,cS1,rS1);


    fprintf(fp,"=========== %s: image %d ===================\n",pgm,im+1);
    fprintf(fp,"Signal, Noise, Ghosting (x100): %.6f, %.6f, %.6f\n",maxSf*100, noise*100, meanG*100);
    fprintf(fp,"SNR: %.f (NEMA standard: %.f)\n",maxSf/noise, maxSf/noise*1.253);
    fprintf(fp,"Ghosting: %.2f%% of max signal (in %dx%d ROI)\n",(meanG-noise)/maxSf*100,Nfilter,Nfilter);
    fprintf(fp,"Maximum ghosting is in pixel %d, %d\n",cG,rG);
    fprintf(fp,"Maximum (filtered) intensity is in pixel %d, %d\n",cS1f,rS1f);
    fprintf(fp,"Percent Image Uniformity is %.2f%%\n",(1-(maxS-minS)/(maxS+minS))*100);
    fprintf(fp,"Unfiltered minS, maxS = %f and %f at (%d,%d), (%d,%d)\n",minS*100,maxS*100,cS2,rS2,cS1,rS1);
    
  }  /* end image loop */


  printf("\n\n\n");
  fprintf(fp,"\n\n\n");
  fclose(fp);

  return TRUE;
}


