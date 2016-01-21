/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef __INTERIX
#include <arpa/inet.h>
#else
#include <netinet/in.h>
#endif

#include "constant.h"
#include "process.h"
#include "struct3d.h"

#define FT3D
#include "data.h"
#undef FT3D

#include "fileio.h"


/*  VMS does not define M_PI in its math.h file */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef MAX
#define	MAX(a, b)	((a) < (b) ? (b) : (a))
#endif

extern int	maxfn12,	/* maximum F1-F2 real Fourier number	*/
		maxfn;		/* maximum real Fourier number		*/
extern void	Werrprintf(char *format, ...);


/*---------------------------------------
|                                       |
|         create_sinetable()/1          |
|                                       |
+--------------------------------------*/
float *create_sinetable(info3D)
proc3DInfo      *info3D;  /* pointer to 3D information structure */
{
   int                  sinefn;
   register int         i,
                        igrp2;
   register float       *sinetable,
                        t_re,
                        t_im,
                        w_re;


   maxfn12 = MAX(info3D->f1dim.scdata.fn, info3D->f2dim.scdata.fn);
   maxfn = MAX(maxfn12, info3D->f3dim.scdata.fn);
   sinefn = maxfn / COMPLEX;
 
   if ( (sinetable = (float *) malloc( (unsigned) (COMPLEX * sinefn *
		sizeof(float)))) == NULL )
   {
      Werrprintf("\ncreate_sinetable():  cannot allocate sinetable buffer");
      return(NULL);
   }
 
   w_re = M_PI / ((float) sinefn);
   t_re = w_re;
   igrp2 = sinefn/2;
   sinetable[0] = 1.0;
   sinetable[1] = 0.0;
   sinetable[2*igrp2] = 0.0;
   sinetable[2*igrp2+1] = -1.0;
 
   for (i = 1; i < igrp2; i++)
   {
      t_re = w_re * ((float) i);
      t_im = cos(t_re);
      sinetable[2*i] = t_im;
      sinetable[2*(igrp2 - i) + 1] = -t_im;
      sinetable[2*(sinefn - i)] = -t_im;
      sinetable[2*(igrp2 + i) + 1] = -t_im;
   }
 
   return(sinetable);
}


/*---------------------------------------
|                                       |
|                fft()/8                |
|                                       |
+--------------------------------------*/
void fft(data, sinetable, sinefn, fn, level, zfratio, skip, fnorm)
int     fn,             /* complex Fourier number               */
        sinefn,         /* complex sine table number            */
        level,          /* number of FT levels                  */
        zfratio,        /* zero-filling ratio                   */
        skip;           /* skip factor                          */
float   *data,          /* pointer to time-domain data          */
        *sinetable,     /* pointer to sine table                */
        fnorm;          /* normalization factor                 */
{
   int                  ngrp,
                        ip,  
                        ig,
                        wdelta,
                        igrp,
                        nozf = FALSE;   /* proper initialization */
   register int         i,
                        igrp2,
                        tonextreal,
                        imagskip;
   float                f3r,
                        f3i;
   register float       *data0,
                        *data1,
                        *wpnt,
                        t_re,
			t_im,
                        w_re,
                        w_im,
                        w_c,
                        f2r,
                        f2i;
 
 
/***********************************************************
*  All DFT passes are performed except for the last two.   *
*  A radix-four last pass is used to scale and rotate the  *
*  data.  The loops in the FFT are inside out in order to  *
*  minimize the number of complex multiplications.  The    *
*  bit reversal occurs after the DFT.                      *
***********************************************************/
 
   if (zfratio == 0)
   {
      nozf = TRUE;
      zfratio++;
   }
 
   tonextreal = skip;
   imagskip = skip/2;
   wdelta = 4*sinefn/fn;
   igrp = fn/2;
   ngrp = 1;
 
   for (ip = 0; ip < (level - 2); ip++)
   {
      igrp2 = igrp*tonextreal;
      wpnt = sinetable;
 
/*******************************
* Initialize indices for dft.  *
*******************************/
 
      data0 = data;
      data1 = data0 + igrp2;
 
/********************************************************
*  This is the outer loop on DFT's within a group.  If  *
*  zero filling has been used, one can take advantage   *
*  of knowing where zeros are to minimize the computa-  *
*  tional effort.                                       *
********************************************************/
 
      for (ig = 0; ig < (igrp/zfratio); ig++)
      {
 
/**********************************************
*  The innter DFT loop across the groups is   *
*  performed.  Rotate the result at the end.  *
**********************************************/
 
         w_re = *wpnt;
         w_im = *(wpnt + 1);
         if ((zfratio >= 1) && (!nozf))
         {
            for (i = 0; i < ngrp; i++)
            {
               t_re = *data0;
               t_im = *(data0 + imagskip);
               *data1 = t_re * w_re - t_im * w_im;
               *(data1 + imagskip) =  t_re * w_im + t_im * w_re;
               data0 = data1 + igrp2;
               data1 = data0 + igrp2;
            }
         }
         else
         {
            for (i = 0; i < ngrp; i++)
            {
               t_re = *data0 - *data1;
               t_im = *(data0 + imagskip) - (*(data1 + imagskip));
               *data0 += *data1;
               *(data0 + imagskip) += *(data1 + imagskip);
               *data1 = t_re * w_re - t_im * w_im;
               *(data1 + imagskip) = t_re * w_im + t_im * w_re;
               data0 = data1 + igrp2;
               data1 = data0 + igrp2;
            }
         }
 
/*******************************************************
*  Reset indices for next pass and rotate the vector.  *
*******************************************************/
 
         data0 = data + (ig + 1)*tonextreal;
         data1 = data0 + igrp2;
         wpnt += wdelta;
      }
 
/****************************************************
*  This is the end of the pass.  Set up next pass.  *
****************************************************/
 
      wdelta *= 2;
      igrp /= 2;
      ngrp *= 2;
      if (zfratio > 1)
      {
         zfratio /= 2;
      }
      else
      {
         nozf = TRUE;
      }
   }
 
 
/********************************************
*  Now do the last pass and scale the data  *
*  by "fnorm".                              *
********************************************/
 
   w_c = fnorm;
   data0 = data;
   igrp2 = fn/4;
 
   for (ip = 0; ip  < igrp2; ip++)
   { /* scaling is done here */
      data1 = data0;
      t_re = w_c * (*data1); /* preserves f0r */
      data1 += imagskip;
      t_im = w_c * (*data1); /* preserves f0i */
      data1 += imagskip;
      w_re = w_c * (*data1); /* preserves f1r */
      data1 += imagskip;
      w_im = w_c * (*data1); /* preserves f1i */
      data1 += imagskip;
      f2r = w_c * (*data1);
      data1 += imagskip;
      f2i = w_c * (*data1);
      data1 += imagskip;
      f3r = w_c * (*data1);
      data1 += imagskip;
      f3i = w_c * (*data1);
      data1 = data0;
 
/*********************************
*  Rotates  frequency to center  *
*  implicitly:  2 0 3 1.         *
*********************************/
 
      *data1 = t_re - w_re + f2r - f3r; /* f2r */
      data1 += imagskip;
      *data1 = t_im - w_im + f2i - f3i; /* f2i */
      data1 += imagskip;
      *data1 = t_re + w_re + f2r + f3r; /* f0r */
      data1 += imagskip;
      *data1 = t_im + w_im + f2i + f3i; /* f0i */
      data1 += imagskip;
      *data1 = t_re - w_im - f2r + f3i; /* f3r */
      data1 += imagskip;
      *data1 = t_im + w_re - f2i - f3r; /* f3i */
      data1 += imagskip;
      *data1 = t_re + w_im - f2r - f3i; /* f1r */
      data1 += imagskip;
      *data1 = t_im - w_re - f2i + f3r; /* f1i */
      data1 += imagskip;
 
      data0 = data1;
   }
 
/*****************************************
*  The FFT is now complete but the data  *
*  array is in base 2 scrambled order.   *
*  Unscramble the array.                 *
*****************************************/
 
   igrp = 0;
   for (i = 0; i < fn; i++)
   {
      if (i < igrp)
      {
         data1 = data + tonextreal*igrp;
         data0 = data + tonextreal*i;
         t_re = *data0;
         *data0 = *data1;
         data0 += imagskip;
         *data1 = t_re;
         data1 += imagskip;
         t_re = *data0;
         *data0 = *data1;
         *data1 = t_re;
      }
 
      ngrp = fn >> 1;
      while (igrp >= ngrp)
      {
         igrp -= ngrp;
         ngrp = (ngrp + 1) >> 1;
      }
 
      igrp += ngrp;
   }   
      
   data0 = data;
}


/*---------------------------------------
|                                       |
|              weight()/5               |
|                                       |
+--------------------------------------*/
void weight(wtvector, data, npts, flag, datatype)
int     npts,		/* number of "datatype" points	*/
	flag,		/* weighting flag		*/
        datatype;	/* type of data			*/
float   *wtvector,	/* pointer to weighting vector	*/
        *data;		/* pointer to time-domain data	*/
{
   register int         i,
			skip;
   register float       *wtv,
                        *dpntr,
			wmult;


   if (!flag)
      return;

   wtv = wtvector;
   dpntr = data;
   skip = datatype/2;

   for (i = 0; i < npts; i++)
   {
      wmult = *wtv++;
      *dpntr *= wmult;
      dpntr += skip;
      *dpntr *= wmult;
      dpntr += skip;
   }
}


/*---------------------------------------
|                                       |
|           cnvrts1632fl()/6            |
|                                       |
+--------------------------------------*/
void cnvrts1632fl(sf, inp, outp, npts, lspts, status)
char    *inp;		/* pointer to input time-domain data	*/
short	status;		/* FID block status			*/
int     npts,		/* number of complex time-domain points	*/
	lspts;		/* number of complex points to shift	*/
float   sf,		/* scaling factor for conversion	*/
        *outp;		/* pointer to converted output td data	*/
{
   int			npx;
   register short       *sinp;
   register int         i,
                        *linp;
   register float       *finp,
			*fout;
 
 
   npts *= COMPLEX;
   lspts *= COMPLEX;
   npx = npts + lspts;


   if (lspts < 0)
   {
      fout = outp + npts;

      if (status & S_FLOAT)
      {
         finp = (float *)inp + npx;
         for (i = 0; i < npx; i++)
            *(--fout) = sf * (*(--finp));
      }
      else if (status & S_32)
      {
         linp = (int *)inp + npx;
         for (i = 0; i < npx; i++)
            *(--fout) = sf * ( (float) (*(--linp)) );
      }
      else
      {
         sinp = (short *)inp + npx;
         for (i = 0; i < npx; i++)
            *(--fout) = sf * ( (float) (*(--sinp)) );
      }

      for (i = 0; i < (-1)*lspts; i++)
         *(--fout) = 0.0;
   }
   else
   {
      fout = outp;             
 
      if (status & S_FLOAT)
      {
         finp = (float *)inp + lspts;
         for (i = 0; i < npts; i++)
            *fout++ = sf * (*finp++);
      }
      else if (status & S_32)
      { 
         linp = (int *)inp + lspts;
         for (i = 0; i < npts; i++)
            *fout++ = sf * ( (float) (*linp++) );
      }
      else
      { 
         sinp = (short *)inp + lspts;
         for (i = 0; i < npts; i++)
            *fout++ = sf * ( (float) (*sinp++) );
      }
   }
}


#ifdef LINUX
/*---------------------------------------
|                                       |
|           swap1632fl()/6            |
|                                       |
+--------------------------------------*/
void swap1632fl(inp, npts, lspts, status)
char    *inp;		/* pointer to input time-domain data	*/
short	status;		/* FID block status			*/
int     npts,		/* number of complex time-domain points	*/
	lspts;		/* number of complex points to shift	*/
{
   int			npx;
   register short       *sinp;
   register int         i,
                        *linp;
 
   npts *= COMPLEX;
   lspts *= COMPLEX;
   npx = npts + lspts;

   if (lspts < 0)
   {
      if ( (status & S_32) || (status & S_FLOAT) )
      {
         linp = (int *)inp + npx;
         for (i = 0; i < npx; i++)
         {
           --linp;
           *linp = ntohl( *linp );
         }
      }
      else
      {
         sinp = (short *)inp + npx;
         for (i = 0; i < npx; i++)
         {
           --sinp;
           *sinp = ntohs( *sinp );
         }
      }
   }
   else
   {
      if ( (status & S_32) || (status & S_FLOAT) )
      { 
         linp = (int *)inp + lspts;
         for (i = 0; i < npts; i++)
         {
           *linp = ntohl( *linp );
           linp++;
         }
      }
      else
      { 
         sinp = (short *)inp + lspts;
         for (i = 0; i < npts; i++)
         {
           *sinp = ntohs( *sinp );
           sinp++;
         }
      }
   }
}
#endif
