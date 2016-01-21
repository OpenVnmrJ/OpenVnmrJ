/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* sglRF.c: Shaped Pulse Library (SPL), formerly spl.c                       */
/*                                                                           */
/* History:                                                                  */
/*  v 1.3: 28Jan09 Added BIR4 and prototype HS. Paul Kinchesh                */
/*  v 1.2: 19Mar07 Merged with SGL. Paul Kinchesh                            */
/*  v 1.1: 14Feb07 Added calibration of HS-AFPs. Paul Kinchesh               */
/*  v 1.0: 05May06 Prototyped sinc,gauss,Mao,HS-AFP,HT-AHP. Paul Kinchesh    */
/*---------------------------------------------------------------------------*/


#include "sglRF.h"


/*----------------------------------------------------------*/
/*---- cosine and sine coefficients for Mao pulses      ----*/
/*---- J.Mao, T.H.Mareci, E.R.Andrew, JMR 79,1-10(1988) ----*/
/*----------------------------------------------------------*/
static double c4[] = { -2899.206,  5947.067, -6501.857,  8066.327, -4549.156,
                        1626.142,  -957.737,   438.683,  -253.037,   113.678,
                         -82.998,    21.608,   -34.128,    -2.017,   -16.472,
                          -6.672,   -10.017,    -6.742,    -7.066,    -5.836 };

static double s4[] = {     0.000,  -142.687,   318.650,  -597.457,   408.421,
                        -209.755,   135.247,   -82.075,    45.875,   -29.556,
                          17.243,    -9.123,     7.420,    -1.966,     3.347,
                           0.275,     1.918,     0.911,     1.349,     0.946 };

static double c5[] = { -2942.855,  5826.248, -6110.593,  6642.794, -8688.408,
                        5780.883, -2437.819,  1634.761,  -842.536,   599.012,
                        -305.518,   239.193,   -93.785,   111.894,   -17.147,
                          59.095,     7.119,    34.561,    13.097,    22.834 };

static double s5[] = {     0.000,  -143.696,   297.088,  -489.633,   859.552,
                        -661.952,   367.254,  -276.529,   172.948,  -130.452,
                          80.254,   -63.032,    31.346,   -34.357,     8.583,
                         -20.575,    -0.816,   -13.345,    -4.244,    -9.718 };

static double c6[] = { -2878.787,  5902.160, -5893.040,  6227.559, -6716.007,
                        9045.384, -6663.395,  3093.739, -2200.903,  1226.520,
                        -943.397,   525.994,  -439.315,   209.230,  -221.335,
                          65.234,  -124.294,     7.826,   -73.653,   -13.052 };

static double s6[] = {     0.000,  -143.793,   291.126,  -457.941,   662.951,
                       -1122.466,   939.046,  -537.718,   431.690,  -279.077,
                         233.593,  -148.334,   131.270,   -71.222,    77.648,
                         -26.816,    49.969,    -4.968,    33.534,     5.277 };

static double c7[] = { -2952.331,  5826.001, -5996.727,  6014.836, -6372.345,
                        6787.339, -9456.466,  7798.425, -3998.163,  3064.328,
                       -1831.484,  1507.142,  -919.137,   812.674,  -466.577,
                         460.505,  -219.407,   275.812,   -83.174,   174.720 };

static double s7[] = {     0.000,  -143.049,   293.800,  -445.893,   629.209,
                        -836.068,  1422.190, -1276.339,   787.395,  -679.682,
                         461.162,  -414.768,   281.252,  -267.125,   168.738,
                        -176.460,    92.240,  -121.141,    40.411,   -86.915 };


/* The round() function does not exist on Solaris */
/* For negative numbers, returns a 0.0            */
static double sglRoundPositive(double val)
{
   double rval = 0.0;
   double delta;

   if (val > 0.0)
   {
      rval = floor(val);
      delta = val - rval;
      if (delta >= 0.5)
         rval += 1.0;
   }
   return(rval);
}

/*--------------------------------------------------------*/
/*---- Main generation function,                      ----*/
/*---- calls subroutines according to requested shape ----*/
/*--------------------------------------------------------*/
void genRf(RF_PULSE_T *rf)
{
  /* Initialise with NULL shape type */
  rf->type = RF_NULL;

  /* Check shape and generate the pulse if the shape matches */
  if (!strcmp(rf->pulseName,"HS-AFP")) hsafpRf(rf);
  if (!strcmp(rf->pulseName,"HS")) hsRf(rf);
  if (!strcmp(rf->pulseName,"HT-AHP")) htahpRf(rf);
  if (!strcmp(rf->pulseName,"mao")) maoRf(rf);
  if (!strcmp(rf->pulseName,"SGLgauss")) gaussRf(rf);
  if (!strcmp(rf->pulseName,"SGLsinc")) sincRf(rf);
  if (!strcmp(rf->pulseName,"HT-BIR4")) htbir4Rf(rf);
  if (!strcmp(rf->pulseName,"sine")) sineRf(rf);
}


/*----------------------------------*/
/*---- sinc generation function ----*/
/*----------------------------------*/
void sincRf(RF_PULSE_T *rf)
{
  int i;
  double t;
  double a,b;

  /* Set sinc shape type */
  rf->type = RF_SINC;

  /* Set shape name */
  setshapeRf(rf);

  /* Get pulse parameters */
  getparsRf(rf);

  /* Round pulse resolution & duration and set the number of pulse points */
  roundparsRf(rf);

  /* Set default number of lobes */
  if (rf->lobes == 0) rf->lobes = 5;
  /* Otherwise make sure lobes is odd and between 1 and 13 */
  if (rf->lobes < 1) rf->lobes = 1;
  if (rf->lobes > 13) rf->lobes = 13;
  if (rf->lobes%2 == 0) rf->lobes++;

  /* Set excitation and inversion bandwidths */
  switch (rf->lobes) {
    case 1:
      rf->header.bandwidth = 1.998;
      rf->header.inversionBw = 1.252;
      break;
    case 3:
      rf->header.bandwidth = 4.018;
      rf->header.inversionBw = 2.598;
      break;
    case 5:
      rf->header.bandwidth = 5.944;
      rf->header.inversionBw = 4.702;
      break;
    case 7:
      rf->header.bandwidth = 7.884;
      rf->header.inversionBw = 6.298;
      break;
    case 9:
      rf->header.bandwidth = 9.856;
      rf->header.inversionBw = 8.398;
      break;
    case 11:
      rf->header.bandwidth = 11.790;
      rf->header.inversionBw = 9.985;
      break;
    case 13:
      rf->header.bandwidth = 13.783;
      rf->header.inversionBw = 11.918;
      break;
  }

  /* Set modulation type */
  strcpy(rf->header.modulation,"amplitude");

  /* Set rf fraction */
  rf->header.rfFraction = 0.5;

  /* Set pulse type */
  strcpy(rf->header.type,"selective");

  /* Set actual bandwidth of pulse */
  if ((FP_LT(rf->flip,FLIPLIMIT_LOW)) || (FP_GT(rf->flip,FLIPLIMIT_HIGH)))
    /* use excitation bandwidth */
    rf->bandwidth = rf->header.bandwidth/rf->rfDuration;
  else
    /* use inversion bandwidth */
    rf->bandwidth = rf->header.inversionBw/rf->rfDuration;

  /* Set SPL version */
  rf->header.version = SPLVERSION;
  
  /* Allocate memory for pulse shape */
  if ((rf->amp=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->phase=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();

  /* Generate shape */
  a = -rf->rfDuration/2.0 + rf->res/2.0;
  b = M_PI*(rf->lobes + 1)/(rf->rfDuration - rf->res);
  for (i=0;i<rf->pts;i++) {
    t = b*(a+rf->res*i);
    if (t == 0.0) rf->amp[i]=1.0;
    else rf->amp[i]=sin(t)/t;
    rf->phase[i]=0.0;
  }

  /* Calculate area under pulse */
  calcintRf(rf);

  /* Scale amplitude so max is 1023.0 */
  scaleampRf(rf);

  /* Pulse amplitudes can't be negative, so set opposite phase instead */
  absampRf(rf);

  /* Write the pulse to disk */
  writeRf(rf);

  /* Write pulse parameters */  
  putparsRf(rf);  

  /* Update pulseName with new shape name */
  strcpy(rf->pulseName,rf->shapeName);

  /* Free memory */
  free(rf->amp);
  free(rf->phase);
}


/*--------------------------------------*/
/*---- Gaussian generation function ----*/
/*--------------------------------------*/
void gaussRf(RF_PULSE_T *rf)
{
  int i;
  double t;
  double a,b;

  /* Set gauss shape type */
  rf->type = RF_GAUSS;

  /* Set shape name */
  setshapeRf(rf);

  /* Get pulse parameters */
  getparsRf(rf);

  /* Round pulse resolution & duration and set the number of pulse points */
  roundparsRf(rf);

  /* Set default cutoff (% of maximum) */
  if (rf->cutoff < 0.001) rf->cutoff = 1.0;
  /* Otherwise make sure cutoff is 0.1, 0.5, 1.0, 5.0 or 10.0 */
  if (rf->cutoff < 0.25) rf->cutoff = 0.1;
  else if ((rf->cutoff >= 0.25) && (rf->cutoff < 0.75))
    rf->cutoff = 0.5;
  else if ((rf->cutoff >= 0.75) && (rf->cutoff < 2.5))
    rf->cutoff = 1.0;
  else if ((rf->cutoff >= 2.5) && (rf->cutoff < 7.5))
    rf->cutoff = 5.0;
  else if (rf->cutoff >= 7.5) rf->cutoff = 10.0;

  /* Set excitation and inversion bandwidths */
  if (rf->cutoff == 0.1) {
    rf->header.bandwidth = 3.308;
    rf->header.inversionBw = 1.863;
  }
  if (rf->cutoff == 0.5) {
    rf->header.bandwidth = 2.896;
    rf->header.inversionBw = 1.635;
  }
  if (rf->cutoff == 1.0) {
    rf->header.bandwidth = 2.705;
    rf->header.inversionBw = 1.528;
  }
  if (rf->cutoff == 5.0) {
    rf->header.bandwidth = 2.216;
    rf->header.inversionBw = 1.263;
  }
  if (rf->cutoff == 10.0) {
    rf->header.bandwidth = 1.997;
    rf->header.inversionBw = 1.1455;
  }

  /* Set modulation type */
  strcpy(rf->header.modulation,"amplitude");

  /* Set rf fraction */
  rf->header.rfFraction = 0.5;

  /* Set pulse type */
  strcpy(rf->header.type,"selective");

  /* Set actual bandwidth of pulse */
  if ((FP_LT(rf->flip,FLIPLIMIT_LOW)) || (FP_GT(rf->flip,FLIPLIMIT_HIGH)))
    /* use excitation bandwidth */
    rf->bandwidth = rf->header.bandwidth/rf->rfDuration;
  else
    /* use inversion bandwidth */
    rf->bandwidth = rf->header.inversionBw/rf->rfDuration;

  /* Set SPL version */
  rf->header.version = SPLVERSION;

  /* Allocate memory for pulse shape */
  if ((rf->amp=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->phase=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();

  /* Generate shape */
  a = -log(rf->cutoff/100.0)*4.0/(rf->rfDuration*rf->rfDuration);
  b = -rf->rfDuration/2.0 +  rf->res/2.0;
  for (i=0;i<rf->pts;i++) {
    t = b+rf->res*i; 
    rf->amp[i]=exp(-a*t*t);
    rf->phase[i]=0.0;
  }

  /* Calculate area under pulse */
  calcintRf(rf);

  /* Scale amplitude so max is 1023.0 */
  scaleampRf(rf);

  /* Write the pulse to disk */
  writeRf(rf);

  /* Write pulse parameters */  
  putparsRf(rf);  

  /* Update pulseName with new shape name */
  strcpy(rf->pulseName,rf->shapeName);

  /* Free memory */
  free(rf->amp);
  free(rf->phase);
}


/*----------------------------------------------------------*/
/*---- Mao generation function                          ----*/
/*---- J.Mao, T.H.Mareci, E.R.Andrew, JMR 79,1-10(1988) ----*/
/*----------------------------------------------------------*/
void maoRf(RF_PULSE_T *rf)
{
  int i,j;
  double a[20],b[20];
  double t;

  /* Set mao shape type */
  rf->type = RF_MAO;

  /* Set shape name */
  setshapeRf(rf);

  /* Get pulse parameters */
  getparsRf(rf);

  /* Round pulse resolution & duration and set the number of pulse points */
  roundparsRf(rf);

  /* Set default number of lobes */
  if (rf->lobes == 0) rf->lobes = 7;
  /* Otherwise make sure lobes is odd and between 7 and 13 */
  if (rf->lobes < 7) rf->lobes = 7;
  if (rf->lobes > 13) rf->lobes = 13;
  if (rf->lobes%2 == 0) rf->lobes++;

  /* Set excitation and inversion bandwidths */
  switch (rf->lobes) {
    case 7:
      rf->header.bandwidth = -1.000;
      rf->header.inversionBw = 4.698;
      break;
    case 9:
      rf->header.bandwidth = -1.000;
      rf->header.inversionBw = 8.048;
      break;
    case 11:
      rf->header.bandwidth = -1.000;
      rf->header.inversionBw = 9.985;
      break;
    case 13:
      rf->header.bandwidth = -1.000;
      rf->header.inversionBw = 11.918;
      break;
  }

  /* Set modulation type */
  strcpy(rf->header.modulation,"amplitude");

  /* Set rf fraction */
  rf->header.rfFraction = 0.5;

  /* Set pulse type */
  strcpy(rf->header.type,"selective");

  /* Set actual bandwidth of pulse */
  rf->bandwidth = rf->header.inversionBw/rf->rfDuration;

  /* Set SPL version */
  rf->header.version = SPLVERSION;

  /* Allocate memory for pulse shape */
  if ((rf->amp=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->phase=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();

  /* Initialise amplitude and set phase */
  for (i=0;i<rf->pts;i++) {
    rf->amp[i]=0.0;
    rf->phase[i]=0.0;
  }

  /* Set appropriate sin and cos coefficients */
  switch (rf->lobes) {
    case 7:
      for (i=0;i<20;i++) {
        a[i]=c4[i];
        b[i]=s4[i];
      }
      break;
    case 9:
      for (i=0;i<20;i++) {
        a[i]=c5[i];
        b[i]=s5[i];
      }
      break;
    case 11:
      for (i=0;i<20;i++) {
        a[i]=c6[i];
        b[i]=s6[i];
      }
      break;
    case 13:
      for (i=0;i<20;i++) {
        a[i]=c7[i];
        b[i]=s7[i];
      }
      break;
    default:
      break;
  }

  /* Generate shape */
  t = 2*M_PI/rf->pts;
  for(i=1;i<=rf->pts;i++)
    for(j=0;j<20;j++)
      rf->amp[i-1] = rf->amp[i-1] - a[j]*cos(t*j*i) + b[j]*sin(t*j*i);

  /* Scale amplitude so max is 1023.0 */
  scaleampRf(rf);

  /* Calculate area under pulse */
  calcintRf(rf);
  rf->header.integral/=1023.0;

  /* Pulse amplitudes can't be negative, so set opposite phase instead */
  absampRf(rf);

  /* Write the pulse to disk */
  writeRf(rf);

  /* Write pulse parameters */  
  putparsRf(rf);  

  /* Update pulseName with new shape name */
  strcpy(rf->pulseName,rf->shapeName);

  /* Free memory */
  free(rf->amp);
  free(rf->phase);
}


/*-----------------------------------------------------------------*/
/*---- hyperbolic secant (HS) adiabatic generation function    ----*/
/*---- M.S.Silver, R.I.Joseph, D.I.Hoult, JMR 59,347-351(1984) ----*/
/*-----------------------------------------------------------------*/
void hsafpRf(RF_PULSE_T *rf)
{
  int i;
  double t;
  double a;

  /* Set HS-AFP shape type */
  rf->type = RF_HSAFP;

  /* Set shape name */
  setshapeRf(rf);

  /* Get pulse parameters */
  getparsRf(rf);

  /* Round pulse resolution & duration and set the number of pulse points */
  roundparsRf(rf);

  /* Set default bandwidth (kHz) */
  if (rf->bandwidth < 0.001) rf->bandwidth = 5000.0;
  /* Otherwise make sure bandwidth is sensible */
  if (rf->bandwidth < 100.0) rf->bandwidth = 100.0;
  if (rf->bandwidth > 50000.0) rf->bandwidth = 50000.0;
  /* Set default cutoff (% of maximum) */
  if (rf->cutoff < 0.001) rf->cutoff = 1.0;
  /* Make sure cutoff is less than 50% */
  if (rf->cutoff > 50.0) rf->cutoff = 50.0;

  /* Set excitation and inversion bandwidths */
  rf->header.bandwidth = rf->rfDuration*rf->bandwidth;
  rf->header.inversionBw = rf->rfDuration*rf->bandwidth;

  /* Set modulation type */
  strcpy(rf->header.modulation,"adiabatic");

  /* Set rf fraction */
  rf->header.rfFraction = 0.5;

  /* Set pulse type */
  strcpy(rf->header.type,"selective");

  /* Set SPL version */
  rf->header.version = SPLVERSION;

  /* Allocate memory for pulse shape */
  if ((rf->amp=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->phase=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->freq=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();

  /* Calculate beta from the cutoff */
  rf->beta=acosh(100.0/rf->cutoff)*2/rf->rfDuration;

  /* Calculate mu from bandwidth and beta */
  rf->mu=M_PI*rf->bandwidth/rf->beta;

  /* Generate shape */
  a = -rf->rfDuration/2.0 + rf->res/2.0;
  for (i=0;i<rf->pts;i++) {
    t = a+rf->res*i;
    rf->amp[i]=1.0/cosh(rf->beta*t);
    /* This is the frequency modulation in radians... */
    rf->freq[i]=-rf->mu*rf->beta*tanh(rf->beta*t);
  }

  /* Convert frequency modulation to phase modulation */
  /* We require zero phase at the centre of the pulse */
  if (rf->pts%2 == 0) { /* Max amp is two points */
    rf->phase[rf->pts/2-1] = -rf->res*rf->freq[rf->pts/2-1]/2.0;
    rf->phase[rf->pts/2] = rf->res*rf->freq[rf->pts/2]/2.0;
    for (i=rf->pts/2-2;i>=0;i--) 
      rf->phase[i] = rf->phase[i+1] - rf->res*rf->freq[i];
    for (i=rf->pts/2+1;i<rf->pts;i++)
      rf->phase[i] = rf->phase[i-1] + rf->res*rf->freq[i];
  } else { /* Max amp is central point */
    rf->phase[rf->pts/2] = 0.0;
    for (i=rf->pts/2-1;i>=0;i--) 
      rf->phase[i] = rf->phase[i+1] - rf->res*rf->freq[i];
    for (i=rf->pts/2+1;i<rf->pts;i++) 
      rf->phase[i] = rf->phase[i-1] + rf->res*rf->freq[i];
  }
  /* Convert from radians to degrees */
  for (i=0;i<rf->pts;i++) rf->phase[i] *= 180.0/M_PI;

  /* Remove cutoff to eliminate ripples in pulse envelopes
     generated from large cutoffs */
  rmcutoffRf(rf);

  /* Calculate a sutable 'integral' for the AFP by simulation */
  simAFPintRf(rf);

  /* Scale amplitude so max is 1023.0 */
  scaleampRf(rf);

  /* Write the pulse to disk */
  writeRf(rf);

  /* Write pulse parameters */  
  putparsRf(rf);  

  /* Update pulseName with new shape name */
  strcpy(rf->pulseName,rf->shapeName);

  /* Free memory */
  free(rf->amp);
  free(rf->phase);
  free(rf->freq);
}


/*-----------------------------------------------------------------*/
/*---- hyperbolic secant (HS) generation function              ----*/
/*---- M.S.Silver, R.I.Joseph, D.I.Hoult, JMR 59,347-351(1984) ----*/
/*-----------------------------------------------------------------*/
void hsRf(RF_PULSE_T *rf)
{
  int i;
  double t;
  double a;
  double targetmz;

  /* Set HS shape type */
  rf->type = RF_HS;

  /* Set shape name */
  setshapeRf(rf);

  /* Get pulse parameters */
  getparsRf(rf);

  /* Round pulse resolution & duration and set the number of pulse points */
  roundparsRf(rf);

  /* Set default bandwidth (kHz) */
  if (rf->bandwidth < 0.001) rf->bandwidth = 5000.0;
  /* Otherwise make sure bandwidth is sensible */
  if (rf->bandwidth < 100.0) rf->bandwidth = 100.0;
  if (rf->bandwidth > 50000.0) rf->bandwidth = 50000.0;
  /* Set default cutoff (% of maximum) */
  if (rf->cutoff < 0.001) rf->cutoff = 1.0;
  /* Make sure cutoff is less than 50% */
  if (rf->cutoff > 50.0) rf->cutoff = 50.0;

  /* Set excitation and inversion bandwidths */
  rf->header.bandwidth = rf->rfDuration*rf->bandwidth;
  rf->header.inversionBw = rf->rfDuration*rf->bandwidth;

  /* Set modulation type */
  strcpy(rf->header.modulation,"amplitude");

  /* Set rf fraction */
  rf->header.rfFraction = 0.5;

  /* Set pulse type */
  strcpy(rf->header.type,"selective");

  /* Set SPL version */
  rf->header.version = SPLVERSION;

  /* Allocate memory for pulse shape */
  if ((rf->amp=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->phase=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->freq=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();

  /* Calculate beta from the cutoff */
  rf->beta=acosh(100.0/rf->cutoff)*2/rf->rfDuration;

  /* Calculate mu from bandwidth and beta */
  rf->mu=M_PI*rf->bandwidth/rf->beta;

  /* Generate shape */
  a = -rf->rfDuration/2.0 + rf->res/2.0;
  for (i=0;i<rf->pts;i++) {
    t = a+rf->res*i;
    rf->amp[i]=1.0/cosh(rf->beta*t);
    /* This is the frequency modulation in radians... */
    rf->freq[i]=-rf->mu*rf->beta*tanh(rf->beta*t);
  }

  /* Convert frequency modulation to phase modulation */
  /* We require zero phase at the centre of the pulse */
  if (rf->pts%2 == 0) { /* Max amp is two points */
    rf->phase[rf->pts/2-1] = -rf->res*rf->freq[rf->pts/2-1]/2.0;
    rf->phase[rf->pts/2] = rf->res*rf->freq[rf->pts/2]/2.0;
    for (i=rf->pts/2-2;i>=0;i--) 
      rf->phase[i] = rf->phase[i+1] - rf->res*rf->freq[i];
    for (i=rf->pts/2+1;i<rf->pts;i++)
      rf->phase[i] = rf->phase[i-1] + rf->res*rf->freq[i];
  } else { /* Max amp is central point */
    rf->phase[rf->pts/2] = 0.0;
    for (i=rf->pts/2-1;i>=0;i--) 
      rf->phase[i] = rf->phase[i+1] - rf->res*rf->freq[i];
    for (i=rf->pts/2+1;i<rf->pts;i++) 
      rf->phase[i] = rf->phase[i-1] + rf->res*rf->freq[i];
  }
  /* Convert from radians to degrees */
  for (i=0;i<rf->pts;i++) rf->phase[i] *= 180.0/M_PI;

  /* Remove cutoff to eliminate ripples in pulse envelopes
     generated from large cutoffs */
  rmcutoffRf(rf);

  /* Calculate a sutable 'integral' by simulation */
  targetmz=cos(rf->flip*M_PI/180.0);
  if (targetmz<-0.999) targetmz=-0.999;
  simintRf(rf,targetmz);

  /* Scale amplitude so max is 1023.0 */
  scaleampRf(rf);

  /* Write the pulse to disk */
  writeRf(rf);

  /* Write pulse parameters */  
  putparsRf(rf);  

  /* Update pulseName with new shape name */
  strcpy(rf->pulseName,rf->shapeName);

  /* Free memory */
  free(rf->amp);
  free(rf->phase);
  free(rf->freq);
}


/*-------------------------------------------------------------------*/
/*---- tanh/tan adiabatic half passage (AHP) generation function ----*/
/*---- M.Garwood, Y.Ke, JMR 94,511-525(1991)                     ----*/
/*-------------------------------------------------------------------*/
void htahpRf(RF_PULSE_T *rf)
{
  int i;

  /* Set HT-AHP shape type */
  rf->type = RF_HTAHP;

  /* Set shape name */
  setshapeRf(rf);

  /* Get pulse parameters */
  getparsRf(rf);

  /* Round pulse resolution & duration and set the number of pulse points */
  roundparsRf(rf);

  /* Set default bandwidth (Hz) */
  if (rf->bandwidth < 0.001) rf->bandwidth = 5000.0;
  /* Otherwise make sure bandwidth is sensible */
  if (rf->bandwidth < 100.0) rf->bandwidth = 100.0;
  if (rf->bandwidth > 50000.0) rf->bandwidth = 50000.0;

  /* Set excitation bandwidth */
  rf->header.bandwidth = -1.0;

  /* Set integral */
  rf->header.integral = -1.0;

  /* Set inversion bandwidth */
  rf->header.inversionBw = -1.0;

  /* Set modulation type */
  strcpy(rf->header.modulation,"adiabatic");

  /* Set rf fraction */
  rf->header.rfFraction = 0.0;

  /* Set pulse type */
  strcpy(rf->header.type,"nonselective");

  /* Set SPL version */
  rf->header.version = SPLVERSION;

  /* Allocate memory for pulse shape */
  if ((rf->amp=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->phase=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->freq=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();

  /* Generate shape */
  for (i=0;i<rf->pts;i++) {
    rf->amp[i]=tanh(10*rf->res*i/rf->rfDuration);
    /* This is the frequency modulation ... */
    rf->freq[i]=2*M_PI*rf->bandwidth*tan(atan(20)*(1-rf->res*i/rf->rfDuration));
  }

  /* Convert frequency modulation to phase modulation */
  /* We require zero phase at the end of the pulse */
  rf->phase[rf->pts-1]=0.0;
  for (i=rf->pts-2;i>=0;i--)
    rf->phase[i] = rf->phase[i+1] - rf->res*rf->freq[i];
  /* Convert from radians to degrees */
  for (i=0;i<rf->pts;i++) rf->phase[i] *= 180.0/M_PI;

  /* Scale amplitude so max is 1023.0 */
  scaleampRf(rf);

  /* Write the pulse to disk */
  writeRf(rf);

  /* Write pulse parameters */  
  putparsRf(rf);  

  /* Update pulseName with new shape name */
  strcpy(rf->pulseName,rf->shapeName);

  /* Free memory */
  free(rf->amp);
  free(rf->phase);
  free(rf->freq);
}


/*-----------------------------------------------*/
/*---- tanh/tan BIR-4 generation function    ----*/
/*---- M.Garwood, Y.Ke, JMR 94,511-525(1991) ----*/
/*-----------------------------------------------*/
void htbir4Rf(RF_PULSE_T *rf)
{
  int i;

  /* Set HT-AHP shape type */
  rf->type = RF_HTBIR4;

  /* Set shape name */
  setshapeRf(rf);

  /* Get pulse parameters */
  getparsRf(rf);

  /* Round pulse resolution & duration and set the number of pulse points */
  roundpars4Rf(rf);

  /* Set default bandwidth (Hz) */
  if (rf->bandwidth < 0.001) rf->bandwidth = 5000.0;
  /* Otherwise make sure bandwidth is sensible */
  if (rf->bandwidth < 100.0) rf->bandwidth = 100.0;
  if (rf->bandwidth > 50000.0) rf->bandwidth = 50000.0;

  /* Set excitation bandwidth */
  rf->header.bandwidth = -1.0;

  /* Set integral */
  rf->header.integral = -1.0;

  /* Set inversion bandwidth */
  rf->header.inversionBw = -1.0;

  /* Set modulation type */
  strcpy(rf->header.modulation,"adiabatic");

  /* Set rf fraction */
  rf->header.rfFraction = 0.0;

  /* Set pulse type */
  strcpy(rf->header.type,"nonselective");

  /* Set SPL version */
  rf->header.version = SPLVERSION;

  /* Allocate memory for pulse shape */
  if ((rf->amp=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->phase=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->freq=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();

  /* Generate base HT-AHP shape */
  for (i=0;i<rf->pts/4;i++) {
    rf->amp[i]=tanh(10*rf->res*i/(rf->rfDuration/4));
    /* This is the frequency modulation ... */
    rf->freq[i]=2*M_PI*rf->bandwidth*tan(atan(20)*(1-rf->res*i/(rf->rfDuration/4)));
  }

  /* Fill 2nd 1/4 of BIR-4 from base shape */
  for (i=0;i<rf->pts/4;i++) {
    rf->amp[i+rf->pts/4]=rf->amp[i];
    rf->freq[i+rf->pts/4]=-rf->freq[i];
  }

  /* Fill 1st 1/4 of BIR-4 from 2nd 1/4 */
  for (i=0;i<rf->pts/4;i++) {
    rf->amp[i]=rf->amp[rf->pts/2-1-i];
    rf->freq[i]=-rf->freq[rf->pts/2-1-i];
  }

  /* Convert frequency modulation to phase modulation */
  /* We require zero phase at the beginning of the pulse */
  rf->phase[0]=0.0;
  for (i=1;i<rf->pts/2;i++)
    rf->phase[i] = rf->phase[i-1] + rf->res*rf->freq[i];
  /* Convert from radians to degrees */
  for (i=0;i<rf->pts/2;i++) rf->phase[i] *= 180.0/M_PI;

  /* Add the phase change for the required flip in degrees */
  for (i=rf->pts/4;i<rf->pts/2;i++) {
    rf->phase[i] += (180 + rf->flip/2);
  }

  /* Recalculate the frequency point rf->pts/4 to give the phase change */
  rf->freq[rf->pts/4] += ((M_PI + rf->flip*M_PI/360.0)/rf->res);

  /* Reflect the 1st 1/2 of pulse to give the full waveform */
  for (i=0;i<rf->pts/2;i++) {
    rf->amp[rf->pts/2+i]=rf->amp[rf->pts/2-1-i];
    rf->freq[rf->pts/2+i]=rf->freq[rf->pts/2-1-i];
    rf->phase[rf->pts/2+i]=rf->phase[rf->pts/2-1-i];
  }

  /* Scale amplitude so max is 1023.0 */
  scaleampRf(rf);

  /* Write the pulse to disk */
  writeRf(rf);

  /* Write pulse parameters */  
  putparsRf(rf);  

  /* Update pulseName with new shape name */
  strcpy(rf->pulseName,rf->shapeName);

  /* Free memory */
  free(rf->amp);
  free(rf->phase);
  free(rf->freq);
}


/*----------------------------------*/
/*---- sine generation function ----*/
/*----------------------------------*/
void sineRf(RF_PULSE_T *rf)
{
  int i;
  double t;
  double a;

  /* Set sine shape type */
  rf->type = RF_SINE;

  /* Set shape name */
  setshapeRf(rf);

  /* Get pulse parameters */
  getparsRf(rf);

  /* Round pulse resolution & duration and set the number of pulse points */
  roundparsRf(rf);

  /* Set excitation bandwidth */
  rf->header.bandwidth = -1.0;

  /* Set integral */
  rf->header.integral = -1.0;

  /* Set inversion bandwidth */
  rf->header.inversionBw = -1.0;

  /* Set modulation type */
  strcpy(rf->header.modulation,"amplitude");

  /* Set rf fraction */
  rf->header.rfFraction = 0.5;

  /* Set pulse type */
  strcpy(rf->header.type,"selective");

  /* Set SPL version */
  rf->header.version = SPLVERSION;
  
  /* Allocate memory for pulse shape */
  if ((rf->amp=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();
  if ((rf->phase=(double *)malloc(rf->pts*sizeof(double))) == NULL) nomem();

  /* Generate shape */
  a = 2*M_PI*rf->modfrq;
  for (i=0;i<rf->pts;i++) {
    t = a*rf->res*i;
    rf->amp[i]=sin(t);
    rf->phase[i]=0.0;
  }

  /* Calculate area under pulse */
  calcintRf(rf);

  /* Scale amplitude so max is 1023.0 */
  scaleampRf(rf);

  /* Pulse amplitudes can't be negative, so set opposite phase instead */
  absampRf(rf);

  /* Write the pulse to disk */
  writeRf(rf);

  /* Write pulse parameters */  
  putparsRf(rf);  

  /* Update pulseName with new shape name */
  strcpy(rf->pulseName,rf->shapeName);

  /* Free memory */
  free(rf->amp);
  free(rf->phase);
}


/*------------------------------------------------------------------------*/
/*---- function to round pulse resolution & duration to multiple of 4 ----*/
/*------------------------------------------------------------------------*/
void roundpars4Rf(RF_PULSE_T *rf)
{
  /* Limit the pulse resolution to multiples of 0.2 usec >= 0.4 usec */
  rf->res/=0.2;
  rf->res=sglRoundPositive(rf->res);
  rf->res*=0.2;
  if (rf->res < 0.4) rf->res = 0.4;

  /* Ensure the pulse duration is integer multiple of the pulse resolution */
  rf->rfDuration*=1.0e6;
  rf->rfDuration/=(4*rf->res);
  rf->rfDuration=sglRoundPositive(rf->rfDuration);
  rf->rfDuration*=(4*rf->res);
  
  /* Set the number of pulse points */
  rf->pts=(int)sglRoundPositive(rf->rfDuration/rf->res);

  /* Convert the duration and resolution to seconds */
  rf->rfDuration*=1.0e-6;
  rf->res*=1.0e-6;  
}


/*----------------------------------------------------------*/
/*---- function to scale amplitude to maximum of 1023.0 ----*/
/*----------------------------------------------------------*/
void scaleampRf(RF_PULSE_T *rf)
{
  int i;
  double scalef;
  double maxamp=0.0;

  /* Find the maximum amplitude */
  for (i=0;i<rf->pts;i++)
    if (fabs(rf->amp[i])>maxamp) maxamp=fabs(rf->amp[i]);

  /* Calculate scale factor */
  scalef = 1023.0/maxamp;

  /* Scale */
  for (i=0;i<rf->pts;i++) 
    rf->amp[i] *=scalef;
}


/*-------------------------------------------------------*/
/*---- function to remove the cutoff of pulses       ----*/
/*---- R.A.de Graff, K.Nicolay, MRM 40,690-696(1998) ----*/
/*-------------------------------------------------------*/
void rmcutoffRf(RF_PULSE_T *rf)
{
  int i;
  double a;
  double minamp=0.0;
  double maxamp=0.0;

  /* Find the minimum and maximum amplitude */
  minamp=fabs(rf->amp[0]);
  maxamp=fabs(rf->amp[0]);
  for (i=1;i<rf->pts;i++) {
    if (fabs(rf->amp[i])>maxamp) maxamp=fabs(rf->amp[i]);
    if (fabs(rf->amp[i])<minamp) minamp=fabs(rf->amp[i]);
  }

  /* Remove cutoff and rescale to 1.0 */
  a = 1.0/(maxamp-minamp);
  for (i=0;i<rf->pts;i++)
    rf->amp[i] = a*(rf->amp[i]-minamp); 
}


/*------------------------------------------------------*/
/*---- function to calculate the integral of pulses ----*/
/*------------------------------------------------------*/
void calcintRf(RF_PULSE_T *rf)
{
  int i;
  double integral=0.0;

  /* Calculate integral */
  for (i=0;i<rf->pts;i++)
    integral +=rf->amp[i];

  /* Set the integral for a 1 s pulse */
  rf->header.integral = rf->res*integral/rf->rfDuration;
}


/*-----------------------------------------------------*/
/*---- function to simulate an 'integral' for AFPs ----*/
/*-----------------------------------------------------*/
void simAFPintRf(RF_PULSE_T *rf)
{
  FILE *fp;
  char simfile[MAX_STR];
  int i,step;
  double mx,my,mz;
  double mx1,mz1;
  double mx2;
  double B1max,B1;
  double Beff,alpha,theta;

  /* Initialize magnetization */
  mx=0; my=0; mz=1;

  /* Initialize maximum B1 amplitude */
  B1max = 0.0;

  /* Set step size to use a maximum of 1000 pulse points in simulation */
  step=rf->pts/1000;

  /* Generate filename */
  strcpy(simfile,userdir);
  strcat(simfile,"/shapelib/");
  strcat(simfile,rf->shapeName);
  strcat(simfile,".sim");

  /* Open simfile */
  if ((fp=fopen(simfile,"w")) == NULL) 
    abort_message("ERROR: Unable to access %s\n",simfile);
  
  fprintf(fp,"B1/rad\t\tMz\n");

  /* Steadily increase amplitude until we get 99% inversion */
  while (mz > -0.99) {
    /* Re-initialize magnetization */
    mx=0; my=0; mz=1;
    /* Step maximum B1 amplitude by 250.0 radians */
    B1max+=250.0;
    /* Break just in case -0.99 can not be reached */
    if (B1max > 100000) break;
    /* Perform Bloch rotations over pulse */
    for (i=0;i<rf->pts;i++) {
      /* Set B1 */
      B1 = B1max*rf->amp[i];
      /* Caluculate effective field */
      Beff = sqrt(rf->freq[i]*rf->freq[i]+B1*B1);
      /* Calculate angle of Beff */
      alpha = atan2(B1,rf->freq[i]);
      /* Transform magnetization into frequency frame rotating about Beff */
      mx1 = -mz*sin(alpha)+mx*cos(alpha);
      mz1 = mz*cos(alpha)+mx*sin(alpha);
      /* Rotate magnetization about Beff */
      theta = (step+1)*rf->res*Beff;
      mx2 = my*sin(theta)+mx1*cos(theta);
      my = my*cos(theta)-mx1*sin(theta);
      /* Transform back to Larmor frame */
      mx = mx2*cos(alpha)+mz1*sin(alpha);	
      mz = -mx2*sin(alpha)+mz1*cos(alpha); 
      i+=step;
    }
    fprintf(fp,"%f\t%f\n",B1max,mz);
  }
  
  /* Close calcfile */
  fclose(fp);

  /* Add 30% for good measure */
  B1max *= 1.3;
    
  /* B1max for a pulse is flip/(integral*pw) */
  rf->header.integral=M_PI/(rf->rfDuration*B1max);
}


/*--------------------------------------------*/
/*---- function to simulate an 'integral' ----*/
/*--------------------------------------------*/
void simintRf(RF_PULSE_T *rf,double targetmz)
{
  FILE *fp;
  char simfile[MAX_STR];
  int i,step;
  double mx,my,mz;
  double mx1,mz1;
  double mx2;
  double B1max,B1;
  double Beff,alpha,theta;

  /* Initialize magnetization */
  mx=0; my=0; mz=1;

  /* Initialize maximum B1 amplitude */
  B1max = 0.0;

  /* Set step size to use a maximum of 1000 pulse points in simulation */
  step=rf->pts/1000;

  /* Generate filename */
  strcpy(simfile,userdir);
  strcat(simfile,"/shapelib/");
  strcat(simfile,rf->shapeName);
  strcat(simfile,".sim");

  /* Open simfile */
  if ((fp=fopen(simfile,"w")) == NULL) 
    abort_message("ERROR: Unable to access %s\n",simfile);
  
  fprintf(fp,"B1/rad\t\tMz\n");

  /* Steadily increase amplitude until we get target Mz */
  while (mz > targetmz) {
    /* Re-initialize magnetization */
    mx=0; my=0; mz=1;
    /* Step maximum B1 amplitude by 10.0 radians */
    B1max+=10.0;
    /* Break just in case targetmz can not be reached */
    if (B1max > 100000) break;
    /* Perform Bloch rotations over pulse */
    for (i=0;i<rf->pts;i++) {
      /* Set B1 */
      B1 = B1max*rf->amp[i];
      /* Caluculate effective field */
      Beff = sqrt(rf->freq[i]*rf->freq[i]+B1*B1);
      /* Calculate angle of Beff */
      alpha = atan2(B1,rf->freq[i]);
      /* Transform magnetization into frequency frame rotating about Beff */
      mx1 = -mz*sin(alpha)+mx*cos(alpha);
      mz1 = mz*cos(alpha)+mx*sin(alpha);
      /* Rotate magnetization about Beff */
      theta = (step+1)*rf->res*Beff;
      mx2 = my*sin(theta)+mx1*cos(theta);
      my = my*cos(theta)-mx1*sin(theta);
      /* Transform back to Larmor frame */
      mx = mx2*cos(alpha)+mz1*sin(alpha);	
      mz = -mx2*sin(alpha)+mz1*cos(alpha); 
      i+=step;
    }
    fprintf(fp,"%f\t%f\n",B1max,mz);
  }
  
  /* Close calcfile */
  fclose(fp);
    
  /* B1max for a pulse is flip/(integral*pw) */
  rf->header.integral=(rf->flip/180.0)*M_PI/(rf->rfDuration*B1max);
}


/*-------------------------------------------------------------*/
/*---- function to generate positive pulse amplitudes only ----*/
/*-------------------------------------------------------------*/
void absampRf(RF_PULSE_T *rf)
{
  int i;

  /* Pulse amplitudes can't be negative, so set opposite phase instead */
  for (i=0;i<rf->pts;i++) {
    if (rf->amp[i] < 0.0) {
      rf->amp[i] *=-1.0;
      rf->phase[i] +=180.0;
    }
  }
}


/*---------------------------------------------------*/
/*---- function to write the pulse shape to disk ----*/
/*---------------------------------------------------*/
void writeRf(RF_PULSE_T *rf)
{
  FILE *fp;
  char shapefile[MAX_STR];
  int i;

  /* Generate filename */
  strcpy(shapefile,userdir);
  strcat(shapefile,"/shapelib/");
  strcat(shapefile,rf->shapeName);
  strcat(shapefile,".RF");
  
  /* Open shapefile */
  if ((fp=fopen(shapefile,"w")) == NULL) 
    abort_message("ERROR: Unable to access %s\n",shapefile);

  /* Write header info */
  fprintf(fp,"# %s\n",shapefile);
  fprintf(fp,"# ***************************************************\n");
  fprintf(fp,"# Generation parameters:\n");
  fprintf(fp,"#   Shape = %s\n",rf->pulseName);
  fprintf(fp,"#   Duration = %.4f ms\n",rf->rfDuration*1.0e3);
  fprintf(fp,"#   Resolution = %.1f usec\n",rf->res*1.0e6);
  fprintf(fp,"#   Flip = %.1f degrees\n",rf->flip);
  fprintf(fp,"#   Bandwidth = %.1f Hz\n",rf->bandwidth);
  switch (rf->type) {
    case RF_NULL:
      break;
    case RF_GAUSS:
      fprintf(fp,"#   Cutoff = %.3f %%\n",rf->cutoff);
      break;     
    case RF_HS:
      fprintf(fp,"#   Cutoff = %.3f %%\n",rf->cutoff);
      fprintf(fp,"#   HS beta = %.3f rad/s\n",rf->beta);
      fprintf(fp,"#   HS mu = %.3f\n",rf->mu);
      break;     
    case RF_HSAFP:
      fprintf(fp,"#   Cutoff = %.3f %%\n",rf->cutoff);
      fprintf(fp,"#   HS beta = %.3f rad/s\n",rf->beta);
      fprintf(fp,"#   HS mu = %.3f\n",rf->mu);
      break;
    case RF_HTAHP:
      break;   
    case RF_HTBIR4:
      break; 
    case RF_MAO:
      fprintf(fp,"#   Lobes = %d\n",rf->lobes);
      break;     
    case RF_SINC:
      fprintf(fp,"#   Lobes = %d\n",rf->lobes);
      break;
    case RF_SINE:
      fprintf(fp,"#   Modulation = %.3f Hz\n",rf->modfrq);
      break;
  }
  fprintf(fp,"# ***************************************************\n");
  fprintf(fp,"# VERSION       SPLv%.1f\n",SPLVERSION);
  fprintf(fp,"# TYPE          %s\n",rf->header.type);
  fprintf(fp,"# MODULATION    %s\n",rf->header.modulation);
  fprintf(fp,"# EXCITEWIDTH   %.4f\n",rf->header.bandwidth);
  fprintf(fp,"# INVERTWIDTH   %.4f\n",rf->header.inversionBw);
  fprintf(fp,"# INTEGRAL      %.4f\n",rf->header.integral);
  fprintf(fp,"# RF_FRACTION   %.4f\n",rf->header.rfFraction);
  fprintf(fp,"# STEPS         %d\n",rf->pts);
  fprintf(fp,"# ***************************************************\n");

  if (sgldisplay){
    /* Write header info */
    fprintf(stdout,"\n--------------------- PULSE ------------------------\n");
    fprintf(stdout," %s\n",shapefile);
    fprintf(stdout," Generation parameters:\n");
    fprintf(stdout,"   Shape = %s\n",rf->pulseName); 
    fprintf(stdout,"   Duration = %.4f ms\n",rf->rfDuration*1.0e3);
    fprintf(stdout,"   Resolution = %.1f usec\n",rf->res*1.0e6);
    fprintf(stdout,"   Flip = %.1f degrees\n",rf->flip);
    fprintf(stdout,"   Bandwidth = %.1f Hz\n",rf->bandwidth);
    switch (rf->type) {
      case RF_NULL:
        break;
      case RF_GAUSS:
        fprintf(stdout,"   Cutoff = %.3f %%\n",rf->cutoff);
        break;     
      case RF_HS:
        fprintf(stdout,"   Cutoff = %.3f %%\n",rf->cutoff);
        fprintf(stdout,"   HS beta = %.3f rad/s\n",rf->beta);
        fprintf(stdout,"   HS mu = %.3f\n",rf->mu);
        break;     
      case RF_HSAFP:
        fprintf(stdout,"   Cutoff = %.3f %%\n",rf->cutoff);
        fprintf(stdout,"   HS beta = %.3f rad/s\n",rf->beta);
        fprintf(stdout,"   HS mu = %.3f\n",rf->mu);
        break;     
      case RF_HTAHP:
        break;   
      case RF_HTBIR4:
        break; 
      case RF_MAO:
        fprintf(stdout,"   Lobes = %d\n",rf->lobes);
        break;     
      case RF_SINC:
        fprintf(stdout,"   Lobes = %d\n",rf->lobes);
        break;
      case RF_SINE:
        fprintf(stdout,"   Modulation = %.3f Hz\n",rf->modfrq);
        break;
    }
    fprintf(stdout," VERSION       SPLv%.1f\n",SPLVERSION);
    fprintf(stdout," TYPE          %s\n",rf->header.type);
    fprintf(stdout," MODULATION    %s\n",rf->header.modulation);
    fprintf(stdout," EXCITEWIDTH   %.4f\n",rf->header.bandwidth);
    fprintf(stdout," INVERTWIDTH   %.4f\n",rf->header.inversionBw);
    fprintf(stdout," INTEGRAL      %.4f\n",rf->header.integral);
    fprintf(stdout," RF_FRACTION   %.4f\n",rf->header.rfFraction);
    fprintf(stdout," STEPS         %d\n",rf->pts);
    fprintf(stdout,"----------------------------------------------------\n");
  }

  /* Write shape */
  for (i=0;i<rf->pts;i++) 
    fprintf(fp," %11.1f %11.1f %11.1f\n",rf->phase[i],rf->amp[i],1.0);

  /* Close shape file */
  fclose(fp);
}


/*------------------------------------*/
/*---- function to set shape name ----*/
/*------------------------------------*/
void setshapeRf(RF_PULSE_T *rf)
{
  /* Generate new shape name */
  strcpy(rf->shapeName,rf->pulseName);
  strcat(rf->shapeName,"_");
  strcat(rf->shapeName,rf->pulseBase);
}


/*-----------------------------------------------*/
/*---- function to get parameters from VnmrJ ----*/
/*-----------------------------------------------*/
void getparsRf(RF_PULSE_T *rf)
{
  int i;
  char parname[MAX_STR];

  /* Initialise pars */
  for (i=0;i<MAXRFPARS;i++) rf->pars[i]=0.0;

  /* Generate "*pars" name from the base name */  
  strcpy(parname,rf->pulseBase);
  strcat(parname,"pars");

  /* This will report a suitable error if the "*pars" parameter does not exist */
  rf->npars=(int)getarray(parname,rf->pars);

  /* Current "*pars" definition is that it contains the following elements:
  
        pulse resolution in usec
        pulse bandwidth in Hz
        number of lobes
        amplitude cutoff as a % of maximum
        HS mu
        HS beta in rad/s
        modulation frequency in Hz
  */

  /* There must be at least one value */
  rf->res = rf->pars[0];
  
  /* Now get the rest */
  if (rf->npars > 1) rf->bandwidth = rf->pars[1];
  if (rf->npars > 2) rf->lobes = (int)sglRoundPositive(rf->pars[2]);
  if (rf->npars > 3) rf->cutoff = rf->pars[3];
  if (rf->npars > 4) rf->mu = rf->pars[4];
  if (rf->npars > 5) rf->beta = rf->pars[5];
  if (rf->npars > 6) rf->modfrq = rf->pars[6];
}


/*-------------------------------------------------------*/
/*---- function to round pulse resolution & duration ----*/
/*-------------------------------------------------------*/
void roundparsRf(RF_PULSE_T *rf)
{
  /* Limit the pulse resolution to multiples of 0.2 usec >= 0.4 usec */
  rf->res/=0.2;
  rf->res=sglRoundPositive(rf->res);
  rf->res*=0.2;
  if (rf->res < 0.4) rf->res = 0.4;

  /* Ensure the pulse duration is integer multiple of the pulse resolution */
  rf->rfDuration*=1.0e6;
  rf->rfDuration/=rf->res;
  rf->rfDuration=sglRoundPositive(rf->rfDuration);
  rf->rfDuration*=rf->res;
  
  /* Set the number of pulse points */
  rf->pts=(int)sglRoundPositive(rf->rfDuration/rf->res);

  /* Convert the duration and resolution to seconds */
  rf->rfDuration*=1.0e-6;
  rf->res*=1.0e-6;  
}


/*------------------------------------------------*/
/*---- function to return parameters to VnmrJ ----*/
/*------------------------------------------------*/
void putparsRf(RF_PULSE_T *rf)
{
  char str[MAX_STR];
   
  /* Generate "*pars" name from the base name */  
  strcpy(str,rf->pulseBase);
  strcat(str,"pars");

  /* Write the shape parameters back to "*pars" */
  putCmd("%s[1] = %f\n",str,rf->res*1.0e6);
  putCmd("%s[2] = %f\n",str,rf->bandwidth);
  putCmd("%s[3] = %d\n",str,rf->lobes);
  putCmd("%s[4] = %f\n",str,rf->cutoff);
  putCmd("%s[5] = %f\n",str,rf->mu);
  putCmd("%s[6] = %f\n",str,rf->beta);
}


/*-----------------------------------------*/
/*---- abort funtion for out of memory ----*/
/*-----------------------------------------*/
void nomem()
{
  abort_message("Insufficient memory");
}

