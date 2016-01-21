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
/* Pbox_dsp.h - Pbox Digital Filters 
   The user functions (d1, d2 & d3) are dummy functions which can be used
   for testing new functions before they are implemented. Any modification 
   of this file will need the main (Pbox) programm be recompiled.
   *********************************************************************** */

void sqf(c, n)					/* Square filter d1 */		
double     *c[];
int	      n; 
{
  int     j, k;

  k = 2*n + 1;
  for (j = 0; j < k; j++)
    (*c)[j] = 1.0; 
}

void hnw(c, n)					/* Hanning window */		
double     *c[];
int	      n; 
{
  int     j, k;
  double  a, dj, dnp;

  k = 2*n + 1; dnp = (double) n/M_PI; 
  for (j=0; j<k; j++)
  {
    a = (double) j/dnp;
    (*c)[j]=0.5-0.5*cos(a); 
  }
}

void hmw(c, n)					/* Hamming window */		
double     *c[];
int	      n; 
{
  int     j, k;
  double  a, dj, dnp;

  k = 2*n + 1; dnp = (double) n/M_PI; 
  for (j=0; j<k; j++)
  {
    a = (double) j/dnp;
    (*c)[j]=0.54-0.46*cos(a); 
  }
}

void blw(c, n)					/* Blackman window */		
double     *c[];
int	      n; 
{
  int     j, k;
  double  a, dj, dnp;

  k = 2*n + 1; dnp = (double) n/M_PI; 
  for (j=0; j<k; j++)
  {
    a = (double) j/dnp;
    (*c)[j]=0.42-0.5*cos(a)+0.08*cos(2.0*a); 
  }
}

/* - - - - - - - - - - - -  user functions d1, d2 & d3 - - - - - - - - - */

void d1(c, n)					/* User window d1 */		
double     *c[];
int	      n; 
{
  int     j, k;

  k = 2*n + 1;
  for (j = 0; j < k; j++)
    (*c)[j] = 1.0; 
}

void d2(c, n)					/* User window d2 */		
double     *c[];
int	      n; 
{
  int     j, k;

  k = 2*n + 1;
  for (j = 0; j < k; j++)
    (*c)[j] = 1.0; 
}

void d3(c, n)					/* User window d3 */		
double     *c[];
int	      n; 
{
  int     j, k;

  k = 2*n + 1;
  for (j = 0; j < k; j++)
    (*c)[j] = 1.0; 
}

/* - - - - - - - - - - - - - - Fourier Transforms - - - - - - - - - - - */

void reorder(re, im, np)
int   np;
double *re[], *im[];
{
  int j, k;
  double tm;

  np/=2;
  for(j=0, k=np; j<np; j++, k++)
  {   
    tm=(*re)[k]; 
    (*re)[k]=(*re)[j]; 
    (*re)[j]=tm;
    tm=(*im)[k]; 
    (*im)[k]=(*im)[j]; 
    (*im)[j]=tm;
  }
}

void shift(re, im, nn, zf)	       /* data reordering of zero filled iFFT */
double *re[], *im[];                           /* wrap 12345000 into 00123450 */
int    nn, zf;
{
int    i, j, k;
double ar0, tm;

  k = (zf - nn)/2;
  for(i=zf-k-1, j=nn-1; j>=0; i--, j--)
  {
    (*re)[i] = (*re)[j], (*im)[i] = (*im)[j];
    (*re)[j] = 0.0, (*im)[j] = 0.0;
  }
}

void reorderi(re, im, nn, zf)		/* wrap 12345000 into 34500012 */
double *re[], *im[];			/* replaces shift + reorder for iFFT */
int    nn, zf;
{
int    i, j;
double re0, im0;
 
  i = nn/2;
  re0 = (*re)[i], im0 = (*im)[i];
  for(i--, nn--, zf--, j=nn-i-1; i>=0; i--, j--, nn--, zf--)
  {
    tm = (*re)[i], (*re)[j] = (*re)[nn], (*re)[nn] = 0.0, (*re)[zf] = tm;
    tm = (*im)[i], (*im)[j] = (*im)[nn], (*im)[nn] = 0.0, (*im)[zf] = tm;
  }
  (*re)[0] = re0, (*im)[0] = im0; 
}

void fft(re, im, np, sign)
int  np;
double *re[], *im[], sign;
{
  int	  i, j, k, m, istep;	
  double  re0, im0, cs, sn, wr, wi, teta, tm;

  j = 0;					/* bit-reversal */
  for (i=0; i<np; i++)		
  {
    if (j > i) 	  
    {
      SWAP((*re)[j], (*re)[i]);
      SWAP((*im)[j], (*im)[i]); 
    }
    m = np >> 1;
    while (m>=2 && j>=m)
    {
      j-=m; 
      m>>=1;
    }
    j += m;
  }
  k = 1;					/* Danielson - Lanczos lemma */
  while (np > k)  
  {
    istep = 2*k;
    teta = sign*M_PI/k;
    cs = cos(teta);
    sn = sin(teta);
    wr = 1.0, wi = 0.0;
    for (m=0; m<k; m++) 
    {
      for (i=m; i<np; i+=istep) 
      {
        j = i + k;
        re0 = wr*(*re)[j] + wi*(*im)[j];
        im0 = wr*(*im)[j] - wi*(*re)[j];
        (*re)[j] = (*re)[i] - re0;	 
        (*im)[j] = (*im)[i] - im0;
        (*re)[i] += re0;
        (*im)[i] += im0;
      }
    wr = (tm=wr)*cs + wi*sn;
    wi = wi*cs - tm*sn;
    }
    k = istep;
  }
}

