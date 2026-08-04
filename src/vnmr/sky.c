/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------
|								|
|   sky.c - simulate or call sky array processor routines 	|
|								|
+--------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif 

#define HYPERCOMPLEX	4
#define MAX16INT	32767

struct _fcmplx
{
   float	re;
   float	im;
};

typedef struct _fcmplx fcomplex;

struct _hypercomplex
{
   float	rere;
   float	reim;
   float	imre;
   float	imim;
};

typedef struct _hypercomplex hypercomplex;

#ifdef  DEBUG
extern int debug1;
#define DPRINT(str) \
	if (debug1) Wscrprintf(str)
#define DPRINT1(str, arg1) \
	if (debug1) Wscrprintf(str,arg1)
#define DPRINT2(str, arg1, arg2) \
	if (debug1) Wscrprintf(str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
	if (debug1) Wscrprintf(str,arg1,arg2,arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4) \
	if (debug1) Wscrprintf(str,arg1,arg2,arg3,arg4)
#else 
#define DPRINT(str) 
#define DPRINT1(str, arg2) 
#define DPRINT2(str, arg1, arg2) 
#define DPRINT3(str, arg1, arg2, arg3) 
#define DPRINT4(str, arg1, arg2, arg3, arg4) 
#endif 

/* vector,vector,vector multiply, real vectors */
/**********************************/
void vvvrmult(float *restrict in1, int is1, float *in2, int is2, float *out1, int os1, int n)
/**********************************/
{ int i;
  int is1r,is2r,os1r;
  float *in1r,*in2r,*out1r;

 if (is1 == 1 && is2 == os1)
  {
    switch (is2)
    {
      case 1:
        for (i = 0; i < n; i++) out1[i] = in1[i] * in2[i];
        return;
      case 2:
        for (i = 0; i < n; i++) out1[i * 2] = in1[i] * in2[i * 2];
        return;
      case 4:
        for (i = 0; i < n; i++) out1[i * 4] = in1[i] * in2[i * 4];
        return;
      default:
        break; /* uncommon stride: fall through to generic below */
    }
  }
 {
  is1r = is1; is2r = is2; os1r = os1;
  in1r = in1; in2r = in2; out1r = out1;
  i = n;
  while (i--)
    { *out1r = *in1r * *in2r; 
      in1r += is1r; in2r += is2r; out1r += os1r; 
    }
 }
}


/*---------------------------------------
|                                       |
|            datafill()/3               |
|                                       |
+--------------------------------------*/
void datafill(float *buffer, int n, float value)
{
   int         i;
   float       *buffer0,
               d;

   buffer0 = buffer;
   d = value;
   i = n;
   while (i--)
      *buffer0++ = d;
}


/*******************/
void skymax(float *in1, float *in2, float *out, int n)
/*******************/
{ int i;
  float *i1,*i2,*o1;

  i1 = in1; i2 = in2; o1 = out;
  i = n;
  while (i--) 
    { if (*i1 >= *i2) *o1 = *i1; else *o1 = *i2;
      i1++; i2++; o1++;
    }
}

/*******************/
void skyadd(float *in1, float *in2, float *out, int n)
/*******************/
{ int i;
  float *i1,*i2,*o1;

  i1 = in1; i2 = in2; o1 = out;
  i = n;
  while (i--) 
    { *o1 = *i1 + *i2;
      i1++; i2++; o1++;
    }
}

/* in1 	 input array */
/* is1 	 input increment */
/* sum 	 sum of the points */
/* npnt  number of points */
/*****************************************/
void vrsum(float *restrict in1, int is1, float *restrict sum, int npnt)
/*****************************************/
{ float *inptr;
  int    inincr;
  float  total;
  int i;

  inptr   = in1;
  inincr  = is1;
  total   = 0.0f;
 if (is1 == 1)
  {
      float acc0=0,acc1=0,acc2=0,acc3=0,acc4=0,acc5=0,acc6=0,acc7=0;
      for (i = 0; i + 8 <= npnt; i += 8)
      {
         acc0 += in1[i+0]; acc1 += in1[i+1]; acc2 += in1[i+2]; acc3 += in1[i+3];
         acc4 += in1[i+4]; acc5 += in1[i+5]; acc6 += in1[i+6]; acc7 += in1[i+7];
      }
      total = ((acc0+acc1)+(acc2+acc3)) + ((acc4+acc5)+(acc6+acc7));
      for (; i < npnt; i++) total += in1[i];
  }
  else if (is1 == 2)
  {
      float acc0=0,acc1=0,acc2=0,acc3=0,acc4=0,acc5=0,acc6=0,acc7=0;
      for (i = 0; i + 8 <= npnt; i += 8)
      {
         acc0 += in1[(i+0)*2]; acc1 += in1[(i+1)*2]; acc2 += in1[(i+2)*2]; acc3 += in1[(i+3)*2];
         acc4 += in1[(i+4)*2]; acc5 += in1[(i+5)*2]; acc6 += in1[(i+6)*2]; acc7 += in1[(i+7)*2];
      }
      total = ((acc0+acc1)+(acc2+acc3)) + ((acc4+acc5)+(acc6+acc7));
      for (; i < npnt; i++) total += in1[i*2];
  }
  else
 {
  i       = npnt;
  while (i--)
  {
    total += *inptr;
    inptr += inincr;
  }
 }
  *sum = total;
}

/* inptr	 input array            */
/* outptr	 output array           */
/* val		 value to be subtracted */
/* n		 number of points       */
/************************/
void vssubr(float *inptr, float val, float *outptr, int n)
/************************/
{ float d;
  int   i;
  float *inp;
  float *outp;

  inp = inptr;
  outp = outptr;
  d = val;
  i = n;
  while (i--) *outp++ = *inp++ - d;
}

/* float *in1;		input array */
/* int    is1;		input increment */
/* float *out1;		output pointer */
/* int    os1;		output increment */
/* float  start;	initial level of the ramp */
/* float  delta;	increment in the level of the ramp */
/* int    npnt;		number of points */
/*****************************************/
void vvrramp(float *in1, int is1, float *out1, int os1, float start, float delta, int npnt)
/*****************************************/
{ float *inptr;
  int    inincr;
  float *outptr;
  int    outincr;
  float  level;
  float  incr;
  int    i;

 if (is1 == 1 && os1 == 1)
  {
    for (i = 0; i < npnt; i++)
      out1[i] = in1[i] + (start + (float)i * delta);
  }
  else if (is1 == 2 && os1 == 2)
  {
    for (i = 0; i < npnt; i++)
      out1[i * 2] = in1[i * 2] + (start + (float)i * delta);
  }
  else
 {
  inptr   = in1;
  inincr  = is1;
  outptr  = out1;
  outincr = os1;
  level   = start;
  incr    = delta;
  i       = npnt;
  while (i--)
  {
    *outptr = *inptr + level;
    outptr += outincr;
    inptr  += inincr;
    level  += incr;
  }
 }
}


/*-----------------------------------------------
|                                               |
|                 vvvcmult()/7                  |
|                                               |
|   This function performs a vector-vector      |
|   multiplication of complex vectors.  The     |
|   routine does work in-place, i.e., the       |
|   output vector can be the same as one input  |
|   vector.                                     |
|                                               |
+----------------------------------------------*/
/* void	*in1,		first input vector                   */
/* 	*in2,		second input vector                  */
/* 	*out1;		output vector                        */
/* int     is1,         increment for first input vector     */
/*         is2,         increment for second input vector    */
/*         os1,         increment for output vector          */
/*         npoints;     number of complex points             */
void vvvcmult(void *in1, int is1, void *in2, int is2, void *out1, int os1, int npoints)
{
   int         i;
   float       tmp;
   fcomplex 	*input1,
				*input2,
				*output;

   input1 = (fcomplex *)in1;
   input2 = (fcomplex *)in2;
   output = (fcomplex *)out1;

   if (is1 == 1 && is2 == 0 && os1 == 1)
    {
      float scalar_re = input2->re, scalar_im = input2->im;
      for (i = 0; i < npoints; i++)
      {
         tmp = (input1[i].re * scalar_re) - (input1[i].im * scalar_im);
         output[i].im = (input1[i].im * scalar_re) + (input1[i].re * scalar_im);
         output[i].re = tmp;
      }
      return;
    }
	
   for (i = 0; i < npoints; i++)
   {
      tmp = (input1->re * input2->re) - (input1->im * input2->im);
      output->im = (input1->im * input2->re) + (input1->re * input2->im);
      output->re = tmp;
      input1 += is1;
      input2 += is2;
      output += os1;
   }
}


/*-----------------------------------------------
|                                               |
|                 vvvhcmult()/7                 |
|                                               |
|   This function performs a vector-vector      |
|   multiplication of hypercomplex vectors.     |
|   The routine does work in-place, i.e., the   |
|   output vector can be the same as one input  |
|   vector.                                     |
|                                               |
+----------------------------------------------*/
/* void		*in1,		first input vector                   */
/* 		*in2,		second input vector                  */
/* 		*out1;		output vector                        */
/* int             is1,         increment for first input vector	*/
/*                 is2,         increment for seocnd input vector	*/
/*                 os1,         increment for output vector		*/
/*                 npoints;     number of hypercomplex points	*/
void vvvhcmult(void *in1, int is1, void *in2, int is2, void *out1, int os1, int npoints)
{
   int                 i;
   float               rere,
                       reim,
                       imre,
                       imim,
					   tmp;
   hypercomplex        *input1,
                       *input2,
                       *output;
 
   input1 = (hypercomplex *)in1;
   input2 = (hypercomplex *)in2;
   output = (hypercomplex *)out1;
 
   for (i = 0; i < npoints; i++)
   {
      tmp = input2->rere;
      rere = (input1->rere) * tmp;
      imre = (input1->imre) * tmp;
      reim = (input1->reim) * tmp;
      imim = (input1->imim) * tmp;
 
      tmp = input2->imre;
      rere -= (input1->imre) * tmp;
      imre += (input1->rere) * tmp;
      reim -= (input1->imim) * tmp;
      imim += (input1->reim) * tmp;
 
      tmp = input2->reim;
      rere -= (input1->reim) * tmp;
      imre -= (input1->imim) * tmp;
      reim += (input1->rere) * tmp;
      imim += (input1->imre) * tmp;
 
      tmp = input2->imim;
      rere += (input1->imim) * tmp;
      imre -= (input1->reim) * tmp;
      reim -= (input1->imre) * tmp;
      imim += (input1->rere) * tmp;
 
      output->rere = rere;
      output->reim = reim;
      output->imre = imre;
      output->imim = imim;
 
      input1 += is1;
      input2 += is2;
      output += os1;
   }
}


/*---------------------------------------
|					|
|		scfix1()/6		|
|					|
+--------------------------------------*/
/* short *topntr;	pointer to destination data			*/
/* int	fromincr,	increment of pointer to source data		*/
/* 	toincr,		increment of pointer to destination data	*/
/* 	npnts;		number of points				*/
/* float *frompntr,	pointer to source data			*/
/* 	mult;		scaling factor				*/
void scfix1(float *frompntr, int fromincr, float mult, short *topntr, int toincr, int npnts)
{
   short	*destpntr;
   int		i;
   float	r,
			*srcpntr;

   srcpntr = frompntr;
   destpntr = topntr;

   for (i = 0; i < npnts; i++)
   {
      r = mult * (*srcpntr);
      if (r >= 0.0) 
      {
         *destpntr = ( (r > 32767.499) ? MAX16INT : ((short) (r + 0.5)) );
      }
      else 
      {
         *destpntr = ( (r < -32767.499) ? ((-1) * MAX16INT) :
				((short) (r - 0.5)) );
      }

      srcpntr += fromincr;
      destpntr += toincr;
   }
}

/*---------------------------------------
|
|		scabs()/6
|	Like scfix1(), but calc absval of (real,imag) pairs.
|	Set fromincr=2 to input consecutive pairs.
|
+--------------------------------------*/
/* short *topntr;	pointer to destination data			*/
/* int	fromincr,	increment of pointer to source data		*/
/* 	toincr,		increment of pointer to destination data	*/
/* 	npnts,		number of points				*/
/* 	sgn;		-1 ==> -absval, +1 ==> +absval		*/
/* float *frompntr,	pointer to source data			*/
/* 	mult;		scaling factor				*/
void scabs(float *frompntr, int fromincr, float mult, short *topntr, int toincr, int npnts, int sgn)
{
   short	*destpntr;
   int		i;
   float	a,b,r,
			*srcpntr;

   srcpntr = frompntr;
   destpntr = topntr;

   for (i = 0; i < npnts; i++)
   {
      a = mult * srcpntr[0];
      b = mult * srcpntr[1];
      /* Don't worry about floating overflow; r should be reasonable size. */
      r = sqrtf(a * a + b * b);
      *destpntr = r > 32767.499 ? sgn * MAX16INT : sgn * (short)(r + 0.5);

      srcpntr += fromincr;
      destpntr += toincr;
   }
}

/*---------------------------------------
|					|
|		preproc()/3		|
|					|
|  Performs pre-processing of real t1	|
|  data which have been acquired in	|
|  the manner:				|
|					|
|     t1=0   		phase incr=0	|
|     t1=0.5*delta_t1	phase incr=90	|
|     t1=delta_t1	phase incr=0	|
|     t1=1.5*delta_t1	phase incr=90	|
|					|
+--------------------------------------*/
/* int	n,		number of real t1 data points/2 */
/* 	datatype;	2 = complex    4 = hypercomplex */
void preproc(float *restrict datapntr, int n, int datatype)
{
   int		i, j;

   if (datatype == 2)
   {
      float *restrict ptr = datapntr + datatype;
      for (i = 1; i < n; i += 2)
      {
         ptr[0] *= -1.0f;
         ptr[1] *= -1.0f;
         ptr += datatype*2;
      }
   }
   else if (datatype == 4)
   {
      float *restrict ptr = datapntr + datatype;
      for (i = 1; i < n; i += 2)
      {
         ptr[0] *= -1.0f;
         ptr[1] *= -1.0f;
         ptr[2] *= -1.0f;
         ptr[3] *= -1.0f;
         ptr += datatype*2;
      }
   }
   else
   {
      float *restrict ptr = datapntr + datatype;
      long stride = (long)datatype * 2;
      for (i = 1; i < n; i += 2)
      {
         for (j = 0; j < datatype; j++)
            ptr[j] *= -1.0f;
         ptr += stride;
      }
   }
}


/*-----------------------------------------------------------------------
|									|
|  	postproc							|
|	does post processing routine for real data done with complex 	|
|		transform						|
|	This version worked for Bruker real data with 2nd and 3rd	|
|		points in set of 4 negated.				|
|	Halves of data table swapped afer ft (start of postproc).	|
|	Sin argument negated. (sign of wi changed).			|
|									|
+----------------------------------------------------------------------*/
/* int	n,		number of "datatype" data points  */
/* 	datatype;	2 = complex      4 = hypercomplex */
void postproc(float *datapntr, int n,  int datatype)
{
  int		i,
			j,
			skip;
  int		i1,
			i2,
			i3,
			i4;
  float		c1,
			c2,
			h1r,
			hli,
			h2r,
			h2i,
			temp,
			dtemp[2],
			d1,
			d2,
			d3,
			d4;
  float  	*pi1,
			*pi2,
			*pi3,
			*pi4;
  double	wpr,
			wpi,
			wtemp,
			theta;
  double	wr,
			wi;


  skip = datatype/2;
  pi1 = datapntr;
  pi2 = pi1 + n*skip;

  for (i = 0; i < (n*skip); i++)
  {
     temp = *pi2;
     *pi2++ = *pi1;
     *pi1++ = temp;
  }


  theta = M_PI / (double) (n);
  c1 = 0.5;
  c2 = -0.5;
  wr = 1.0;
  wi = 0.0;
  wpr = sin(0.5*theta);
  wpr *= (wpr * (-2));
  wpi = sin(theta);

  pi1 = datapntr;
  pi2 = datapntr + 1;
  h1r = *pi1;
  h2r = *pi2;
  *pi1 = h1r + h2r;
  *pi2 = h1r - h2r;


  for (i = 1; i <= (n/2); i++)
  {
     wtemp = wr;
     wr = (wr * wpr) - (wi * wpi) + wr;
     wi = (wi * wpr) + (wtemp * wpi) + wi;
     i1 = 2 * i;
     i2 = i1 + 1;
     i3 = (2 * n) + 1 - i2;
     i4 = i3 + 1;

     pi1 = datapntr + (i1 * skip);
     pi2 = datapntr + (i2 * skip);
     pi3 = datapntr + (i3 * skip);
     pi4 = datapntr + (i4 * skip);
     d1 = *pi1;
     d2 = *pi2;
     d3 = *pi3;
     d4 = *pi4;
     h1r =  c1 * (d1 + d3);
     hli =  c1 * (d2 - d4);
     h2r = -c2 * (d2 + d4);
     h2i =  c2 * (d1 - d3);
     *pi1 =  h1r + (wr * h2r) + (wi * h2i);
     *pi2 =  hli + (wr * h2i) - (wi * h2r);
     *pi3 =  h1r - (wr * h2r) - (wi * h2i);
     *pi4 = -hli + (wr * h2i) - (wi * h2r);

     if (datatype == HYPERCOMPLEX)
     {
        d1 = *(++pi1);
        d2 = *(++pi2);
        d3 = *(++pi3);
        d4 = *(++pi4);
        h1r =  c1 * (d1 + d3);
        hli =  c1 * (d2 - d4);
        h2r = -c2 * (d2 + d4);
        h2i =  c2 * (d1 - d3);
        *pi1 =  h1r + (wr * h2r) + (wi * h2i);
        *pi2 =  hli + (wr * h2i) - (wi * h2r);
        *pi3 =  h1r - (wr * h2r) - (wi * h2i);
        *pi4 = -hli + (wr * h2i) - (wi * h2r);
     }
  }


  for (i = 0; i < n; i += 2)
  {
     for (j = 0; j < 2; j++)
     {
	pi1 = datapntr + (i + j)*skip;
	pi2 = datapntr + (2 * n - i - 2 + j)*skip;
	dtemp[j] = *pi1;
	*pi1 = *pi2;
	*pi2 = dtemp[j];

        if (datatype == HYPERCOMPLEX)
        {
           dtemp[j] = *(++pi1);
           *pi1 = *(++pi2);
           *pi2 = dtemp[j];
        }
     }
  }
}


/*-----------------------------------------------
|						|
|		   combine()/8			|
|						|
|   This function constructs either a complex	|
|   or a hypercomplex t1 interferogram from	|
|   a set of coefficients and a set of F2	|
|   spectra.					|
|						|
+----------------------------------------------*/
/* int	datatype,	2 = complex     4 = hypercomplex		*/
/* 	npoints;	number of complex or hypercomplex points	*/
/* float *combinebuf,	pointer to single complex F2 spectrum	*/
/* 	*outp,		pointer to combined F2 spectra		*/
/* 	r1,		Abs(F2) * r1  ----->  Re(t1)			*/
/* 	r2,		Dsp(F2) * r2  ----->  Re(t1)			*/
/* 	r3,		Abs(F2) * r3  ----->  Im(t1)			*/
/* 	r4;		Dsp(F2) * r4  ----->  Im(t1)			*/
void combine(float *combinebuf, float *outp, int npoints, int datatype,
             float r1, float r2, float r3, float r4)
{
   int		i;
   float	*combuffer,
			*output;

   combuffer = combinebuf;
   output = outp;

   if (datatype == HYPERCOMPLEX)
   {
      for (i = 0; i < npoints; i++)
      {
         (*output++) += (*combuffer) * r1 + (*(combuffer + 1)) * r2;
         (*output++) += (*(combuffer + 1)) * r1 - (*combuffer) * r2;
         (*output++) += (*combuffer) * r3 + (*(combuffer + 1)) * r4;
         (*output++) += (*(combuffer + 1)) * r3 - (*combuffer) * r4;
         combuffer += 2;
      }
   }
   else
   {
      for (i = 0; i < npoints; i++)
      { 
         (*output++) += (*combuffer) * r1 + (*(combuffer + 1)) * r2; 
         (*output++) += (*combuffer) * r3 + (*(combuffer + 1)) * r4;
         combuffer += 2; 
      }
   }
}

/* shiftComplexData shifts complex data points */
/* If fills in zeros. */
/* Both shiftpts and npoints are numbers of complex pairs */
void shiftComplexData(float *ptr, int shiftpts, int npoints, int len)
{
   if (shiftpts == 0)
      return;
   if (shiftpts > 0) /* right shift */
   {
	if (npoints > len - shiftpts)
         npoints = len - shiftpts;

	memmove(ptr + (long)shiftpts * 2, ptr, (size_t)npoints * 2 * sizeof(float));
    memset(ptr, 0, (size_t)shiftpts * 2 * sizeof(float));
   }
   else if (shiftpts < 0) /* left shift */
   {
	// For this case, memmove is very slow
	if (shiftpts > -16)
	{
	  int	i;
      float	*endptr,
			*sptr;

      shiftpts *= -1;
      sptr = ptr+(shiftpts)*2;
      endptr = ptr;

      for (i = 0; i < npoints - shiftpts; i++)
      { 
         *endptr++ = *sptr++; 
         *endptr++ = *sptr++; 
      }
      for (i = 0; i < shiftpts; i++)
      { 
         *endptr++ = 0.0f;
         *endptr++ = 0.0f;
      }
	}
	else
	{
      int nshift = -shiftpts;
      memmove(ptr, ptr + (long)nshift * 2, (size_t)(npoints - nshift) * 2 * sizeof(float));
      memset(ptr + (long)(npoints - nshift) * 2, 0, (size_t)nshift * 2 * sizeof(float));
	}
   }
}


/*-----------------------------------------------
|						|
|	       negateimaginary()/3		|
|						|
|   This function negates the imaginary points	|
|   of either a complex or a hypercomplex t1	|
|   interferogram.				|
|						|
+----------------------------------------------*/
/* int	npoints,	number of complex/hypercomplex data points	*/
/* 	datatype;	2 = complex      4 = hypercomplex		*/
/* float *data;		pointer to complex/hypercomplex data		*/
void negateimaginary(float *data, int npoints, int datatype)
{
   int		i;
   float	*tmp;

   tmp = data + (datatype/2);
   if (datatype == HYPERCOMPLEX)
   {
      for (i = 0; i < npoints; i++)
      {
         (*tmp++) *= (-1);
         (*tmp++) *= (-1);
         tmp += 2;
      }
   }
   else
   {
      for (i = 0; i < npoints; i++)
      {
         (*tmp) *= (-1);
         tmp += 2;
       }
   }
}


/*-----------------------------------------------
|						|
|		  cnvrts32()/5			|
|						|
|   This function converts a 32-bit integer	|
|   into a floating point number and scales	|
|   by the factor "scalefactor".		|
|						|
+----------------------------------------------*/
/* npx,		number of real data points			*/
/* lsfidx;	number of real points to shift			*/
/* inp;		pointer to 32-bit, integer FID data		*/
/* scalefactor,	scaling factor					*/
/* outp;	pointer to 32-bit, floating point FID data	*/
void cnvrts32(float scalefactor, int *restrict inp, float *restrict outp, int npx, int lsfidx)
{
   int	  i;

   if (lsfidx == 0)
   {
      for (i = 0; i < npx; i++)
         outp[i] = scalefactor * (float)inp[i];
   }
   else if (lsfidx < 0)
   {
      for (i = 0; i < npx; i++)
         outp[i - lsfidx] = scalefactor * (float)inp[i];

      for (i = 0; i < (-1)*lsfidx; i++)
         outp[i] = 0.0f;
   }
   else
   {
      for (i = 0; i < (npx - lsfidx); i++) 
         outp[i] = scalefactor * (float)inp[i + lsfidx];
	  for (i = 0; i < lsfidx; i++)
         outp[i+npx-lsfidx] = 0.0f;
   }
}


/*-----------------------------------------------
|						|
|		  cnvrts16()/5			|
|						|
|   This function converts a 16-bit integer	|
|   into a floating point number and scales	|
|   by the factor "scalefactor".		|
|						|
+----------------------------------------------*/
/* int	npx,		number of real data points			*/
/* 	lsfidx;		number of real points to shift		*/
/* short *inp;		pointer to 16-bit, integer FID data		*/
/* float scalefactor,	scaling factor				*/
/* 	*outp;		pointer to 32-bit, floating point FID data	*/
void cnvrts16(float scalefactor, short *restrict inp, float *restrict outp, int npx, int lsfidx)
{
   int		i;

   if (lsfidx == 0)
   {
      for (i = 0; i < npx; i++)
         outp[i] = scalefactor * (float)inp[i];
   }
   else if (lsfidx < 0)
   {
      for (i = 0; i < npx; i++)
         outp[i - lsfidx] = scalefactor * (float)inp[i];
    
      for (i = 0; i < (-1)*lsfidx; i++)
         outp[i] = 0.0f;
   }
   else
   {
      for (i = 0; i < (npx - lsfidx); i++) 
         outp[i] = scalefactor * (float)inp[i + lsfidx];
	  for (i = 0; i < lsfidx; i++)
         outp[i+npx-lsfidx] = 0.0f;
   }
}
