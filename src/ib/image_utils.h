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
/* Function to find new threshold */
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
  return thr;
}


/***********************************************/
/* Find optimal threshold based on histograms  */
/* - better than just using noise level        */
/*   since it classifies ghosts as noise       */
/***********************************************/
float threshold(float *in_data, int img_height, int img_width) {
  int   img_size, pixel;
  float meanN, meanS, meanG;
  float thr0, thr1;
  
  img_size = img_height*img_width;

  /* for initial noise estimate, assume that the first 4 rows are noise */        
  /* We could use all "non-image" pixels, but this includes ghosting    */
  meanN = 0;
  for (pixel = 0; pixel < 4*img_width; pixel++)
    meanN += in_data[pixel];
  meanN /= (4*img_width);

  /* Initial rough estimate of mean signal intensity */
  meanS = 0;
  for (pixel = 0; pixel < img_size; pixel++)
    meanS += in_data[pixel];
  meanS /= img_size;

  /* Initial guesses for threshold between the two classes */
  thr0 = (meanS + meanN)/2;
  thr1 = new_thr(in_data,img_size,thr0);
  
  /* Now do the same in an iterative fashion */
  while (fabs((thr1 - thr0) / (thr1 + thr0)) < 1e-5) { // 1e-5 seems reasonable
    thr0 = thr1;
    thr1 = new_thr(in_data,img_size,thr0);
  }

  return(thr1);

}



/***********************************************/
/* Find object boundaries                      */
/***********************************************/
void find_object(float *in_data, float thr, int img_height, int img_width,
                 int *R0, int *C0, int *R1, int *C1, int *R2, int *C2, int *RAD) {
  int   r1, r2, c1, c2, r0, c0, rr, cr, rad, d,
        Nfilter, NF2, N;
  int   r,c, pixel;
  float meanN, stdN;
        
  r1 = img_height; c1 = img_width;
  r2 = c2 = 0;
  for (r = 2; r < img_height-2; r++) {
    for (c = 2; c < img_width-2; c++) {
      pixel = r*img_width + c;
      if (in_data[pixel] > thr)  { 
        /* determine first and last rows/columns for image */
        if (r < r1) r1 = r; if (c < c1) c1 = c;
	if (r > r2) r2 = r; if (c > c2) c2 = c;
      }
    } /* end of columns loop */
  }  /* end of rows loop */


  r0   = (int)((r1+r2)/2);
  c0   = (int)((c1+c2)/2);


  /* radius as a percent of image dimension */
  rr   = (int) (100*(r2-r1)/img_height);   /* row radius */
  cr   = (int) (100*(c2-c1)/img_width);    /* column radius */
  rad  = (rr <cr ? rr : cr)/2; /* smallest radius */


  /* Output */
  *R0 = r0;
  *R1 = r1;
  *R2 = r2;
  *C0 = c0;
  *C1 = c1;
  *C2 = c2;
  *RAD = rad;

}


/***********************************************/
/* Find noise level                            */
/* Assume that top left corner is noise        */
/* We could use all "non-image" pixels,        */
/* but this includes ghosting                  */
/***********************************************/
float find_noise(float *in_data, int r1, int c1, int r2, int c2) {
  float meanN, stdN;
  double X, sum2;
  int   r,c,n1,pixel;

  if (r1 < 3) r1 = 3;
  if (c1 < 3) c1 = 3;

  
  meanN = stdN = sum2 = 0; n1 = 0;
  
  /* Upper left corner */
  for (r = 0; r < r1; r++) {
    for (c = 0; c < c1; c++) {
      pixel = r*img_width + c;
      X = in_data[pixel];
      meanN += X;
      sum2  += (X*X); 
      n1++;
    }
  }

  /* Upper right corner */
  for (r = 0; r < r1; r++) {
    for (c = c2; c < img_width; c++) {
      pixel = r*img_width + c;
      X = in_data[pixel];
      meanN += X;
      sum2  += (X*X); 
      n1++;
    }
  }

  /* Lower left corner */
  for (r = r2; r < img_height; r++) {
    for (c = 0; c < c1; c++) {
      pixel = r*img_width + c;
      X = in_data[pixel];
      meanN += X;
      sum2  += (X*X); 
      n1++;
    }
  }

  /* Lower right corner */
  for (r = r2; r < img_height; r++) {
    for (c = c2; c < img_width; c++) {
      pixel = r*img_width + c;
      X = in_data[pixel];
      meanN += X;
      sum2  += (X*X); 
      n1++;
    }
  }


  meanN /= n1;
  stdN  = (float) sqrt((double)((sum2/n1) - (meanN*meanN)));

/*
  printf("Mean/Std of noise in %d pixels is (x1000) %.2f, %.2f, ratio = %.2f\n",
    n1,meanN*1000,stdN*1000,meanN/stdN);
*/

  return(meanN);
  
}


/***********************************************/
/* Apply a mean filter to the image            */
/***********************************************/
/* thr is a threshold in intensity, below which pixels are not added in */
void filter(float *in_data, float *newdata, int Nfilter, int img_height, int img_width, int thr) {
  int   r, c, r1, c1, img_size, pixel, pixel2, NF2, n;
  float meanV, V, meanS, meanG;
  
  img_size = img_height*img_width;

  NF2 = (int) (Nfilter/2);
  Nfilter = 2*NF2 + 1;

  for (r = 0; r < img_height; r++) {
    for (c = 0; c < img_width; c++) {
      pixel = r*img_width + c;
      meanV = 0;
      if ((r >= NF2) && (r < img_height-NF2) && (c >= NF2) && (c < img_width-NF2)) {
        /* Skip edges */
	n = 0;
        for (r1 = r-NF2; r1 <= r+NF2; r1++) {
          for (c1 = c-NF2; c1 <= c+NF2; c1++) {
            pixel2 = r1*img_width + c1;
            V = in_data[pixel2];
if (V > thr) {
            meanV += V; n++;
}
	  }
  	}
if (n > 0) 
        meanV /= n;
else
meanV = 0;
        newdata[pixel] = meanV;
      }
      else newdata[pixel] = 0;
    }
  }
}
