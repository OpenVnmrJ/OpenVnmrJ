/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/******************************************************************
*  matrix.c  -  symmetric and non symmetric matrix transposition  *
*               either in-place or out-of-place                   *
******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "vnmrsys.h"			/* To define UNIX (or VMS)	*/

#define COMPLETE	0
#define ERROR		1
#define REAL		1		/* real word length		*/
#define COMPLEX		2		/* complex word length		*/
#define HYPERCOMPLEX	4		/* hypercomplex word length	*/
#define BUF_INIT	0x1		/* initialize transpose buffer	*/
#define BUF_ALLOCATE	0x2		/* allocate transpose buffer	*/
#define BUF_RELEASE	0x4		/* release transpose buffer	*/

struct _fcomplex
{
   float	re;
   float	im;
};

struct _hypercomplex
{
   float	rere;
   float	reim;
   float	imre;
   float	imim;
};

struct xposebuf
{
   int          buffersize;
   float        *bufferpntr;
};

typedef struct _fcomplex	fcomplex;
typedef struct _hypercomplex	hypercomplex;

static struct xposebuf  xpose;

static int xposebufalloc(int, int);


/*---------------------------------------
|					|
|	     initxposebuf()/0		|
|					|
+--------------------------------------*/
void initxposebuf()
{
   xposebufalloc(0, BUF_INIT);
}


/*---------------------------------------
|					|
|	     closexposebuf()/0		|
|					|
+--------------------------------------*/
void closexposebuf()
{
   xposebufalloc(0, BUF_RELEASE);
}


/*---------------------------------------
|					|
|	       invert4()/5		|
|					|
+--------------------------------------*/
static void invert4(float *dmatrix, int offset, int size,
                    int inum, int jnum, int max2)
{
   register int		m,
			l,
			k,
			i,
			j;
   register float	s,
			*i1,
			*i2,
			*i3,
			*i4,
			*submatrix;


   submatrix = dmatrix + offset;
   m = max2;

   for (j = 0; j < (size - 1); j++)
   {
      for (i = (j + 1); i < size; i++)
      {
         i3 = submatrix + (i*inum) + (max2*j);
	 i4 = submatrix + j + (max2*i*jnum);
	 k = inum;
	 while (k--)
	 {
            i1 = i3++;
	    i2 = i4++;
	    l = jnum;
	    while (l--)
	    {
               s = *i1;
               *i1 = *i2;
               *i2 = s;
	       i1 += m;
               i2 += m;
            }
	 }
      }
   }
}


/*---------------------------------------
|					|
|	       invert8()/5		|
|					|
+--------------------------------------*/
static void invert8(float *dmatrix, int offset, int size,
                    int inum, int jnum, int max2)
{
   register int		m,
			l,
			k,
			i,
			j;
   register float	s;
   register fcomplex	*i1,
			*i2,
			*i3,
			*i4,
			*submatrix;


   submatrix = ((fcomplex *) (dmatrix)) + offset;
   m = max2;

   for (j = 0; j < (size - 1); j++)
   {
      for (i = (j + 1); i < size; i++)
      {
         i3 = submatrix + (i*inum) + (max2*j);
	 i4 = submatrix + j + (max2*i*jnum);
	 k = inum;
	 while (k--)
	 {
            i1 = i3++;
	    i2 = i4++;
	    l = jnum;
	    while (l--)
	    {
               s = i1->re;
               i1->re = i2->re;
               i2->re = s;
	       s = i1->im;
               i1->im = i2->im;
               i2->im = s;
	       i1 += m;
               i2 += m;
            }
	 }
      }
   }
}


/*---------------------------------------
|					|
|	      invert16()/5		|
|					|
+--------------------------------------*/
static void invert16(float *dmatrix, int offset, int size,
                     int inum, int jnum, int max2)
{
   register int			m,
				l,
				k,
				i,
				j;
   register float		s;
   register hypercomplex	*i1,
				*i2,
				*i3,
				*i4,
				*submatrix;


   submatrix = ((hypercomplex *) (dmatrix)) + offset;
   m = max2;

   for (j = 0; j < (size - 1); j++)
   {
      for (i = (j + 1); i < size; i++)
      {
         i3 = submatrix + (i*inum) + (max2*j);
	 i4 = submatrix + j + (max2*i*jnum);
	 k = inum;
	 while (k--)
	 {
            i1 = i3++;
	    i2 = i4++;
	    l = jnum;
	    while (l--)
	    {
               s = i1->rere;
               i1->rere = i2->rere;
               i2->rere = s;
	       s = i1->reim;
               i1->reim = i2->reim;
               i2->reim = s;
               s = i1->imre;
               i1->imre = i2->imre;
               i2->imre = s;
               s = i1->imim;
               i1->imim = i2->imim;
               i2->imim = s;
	       i1 += m;
               i2 += m;
            }
	 }
      }
   }
}


/*---------------------------------------
|					|
|	       itrans()/4		|
|					|
+--------------------------------------*/
static void itrans(float *matrix, int max2, int max1, int datatype)
{
   int	ii,
	kk,
	matsize,
	blocks,
	length1,
	length2;
   void (*invertfunc)();	/* pointer to a function */


   if (datatype == HYPERCOMPLEX)
   {
      invertfunc = invert16;
   }
   else if (datatype == COMPLEX)
   {
      invertfunc = invert8;
   }
   else
   {
      invertfunc = invert4;
   }


   if (max1 >= max2)
   {
      blocks = max1/max2; 
      matsize=2;
      for (length1 = max1/2; length1 >= max2; length1 /= 2)
      {
         for (length2 = 1; length2 <= max2/2; length2 *= 2) 
         {
            for (ii = 0; ii < max2/(matsize*length2); ii++)
            {
               for (kk = 0; kk < max1/(matsize*length1); kk++)
	       {
                  (*invertfunc) (matrix,
			       matsize * (ii*length2 + max2*kk*length1),
			       matsize, length2, length1, max2);
               }
            }
         }
      }

      matsize = max2;
      for (ii = 0; ii < blocks; ii++)
         (*invertfunc) (matrix, max2*matsize*ii, matsize, 1, 1, max2);
   }
   else
   {
      blocks = max2/max1; 
      matsize = max1;
      for (ii = 0; ii < blocks; ii++)
         (*invertfunc) (matrix, matsize*ii, matsize, 1, 1, max2);

      matsize=2;
      for (length2 = max1; length2 <= max2/2; length2 *= 2)
      {
         for (length1 = max1/2; length1 >= 1; length1 /= 2)
         {
            for (ii = 0; ii < max1/(matsize*length1); ii++)
            {
               for (kk = 0; kk < max2/(matsize*length2); kk++)
	       {
                  (*invertfunc) (matrix,
			       matsize * (kk*length2 + max2*ii*length1),
			       matsize, length2, length1, max2);
	       }
            }
         }
      }
   }
}


/*-----------------------------------------------
|                                               |
|              xposebufalloc()/2                |
|                                               |
|   This function allocates a block of memory   |
|   for out-of-place transposition of a non-    |
|   symmetric matrix.                           |
|                                               |
+----------------------------------------------*/
int xposebufalloc(int nbytes, int bufstatus)
{
   if (bufstatus & BUF_INIT)
   {
      xpose.buffersize = 0;
      xpose.bufferpntr = NULL;
   }

   if (bufstatus & BUF_RELEASE)
   {
      xpose.buffersize = 0;
      if (xpose.bufferpntr != NULL)
      {
         free((char *)xpose.bufferpntr);
         xpose.bufferpntr = NULL;
      }
   }
 
   if (bufstatus & BUF_ALLOCATE)
   {
      if (xpose.buffersize != nbytes)
      {
         if (xpose.bufferpntr != NULL)
         {
            free((char *)xpose.bufferpntr);
            xpose.bufferpntr = NULL;
            xpose.buffersize = 0;
         }

         xpose.buffersize = nbytes;
         xpose.bufferpntr = (float *) (malloc( (unsigned) (nbytes) ));
         if (xpose.bufferpntr == NULL)
         {
            xpose.buffersize = 0;
            return(ERROR);
         }
      }
   }

   return(COMPLETE);
}   
 
 
/*-----------------------------------------------
|                                               |
|                  symxpose()/3                 |
|                                               |
|   This function performs an in-place trans-   |
|   position of a real, complex, or hyper-      |
|   complex half-transformed 2D data set.       |
|                                               |
+----------------------------------------------*/
/* npoints      number of "datatype" points			*/
/* datatype     1 = real    2 = complex    4 = hypercomplex	*/
static void symxpose(float *matrix, int npoints, int datatype)
{
   register int         inc1,
                        inc2,
                        skip,
                        k;
   register float       *tptr1,
                        *tptr2,
                        *pntr1,
                        *pntr2,
                        tmp;
 
   skip = datatype;
   inc1 = npoints - 1;
   inc2 = npoints*skip;
   pntr2 = matrix + inc2;
 
   for (pntr1 = matrix + skip; pntr1 < (matrix + (inc2 * npoints));
           pntr1 += (inc2 + skip))
   {
      tptr2 = pntr2;
      tptr1 = pntr1;
      while (tptr1 < (pntr1 + (skip * inc1)))
      {
         for (k = 0; k < skip; k++)
         {
            tmp = *tptr1;
            *tptr1++ = *tptr2;
            *tptr2++ = tmp;
         }
 
         tptr2 += (inc2 - skip);
      }
 
     pntr2 += (inc2 + skip);
     inc1 -= 1;
   }
}
 
 
/*-----------------------------------------------
|                                               |
|               nonsymxpose()/4                 |
|                                               |
|   This function performs an out-of-place      |
|   transpose of a non-symmetric matrix.  The   |
|   transposed matrix is stored back in the     |
|   original matrix.  All temporary storage is  |
|   handled within this routine.                |
|                                               |
+----------------------------------------------*/
static int nonsymxpose(float *matrix, int ncols, int nrows, int datatype)
{
   int                  nbytes;
   register int         i,
                        j,
                        skip1,
                        skip2;
   register float       *data,
                        *tmp,
                        *svtmp;
 

   skip1 = datatype;
   skip2 = skip1 * (nrows - 1);
   nbytes = sizeof(float) * nrows * ncols * skip1;
 
   if (xposebufalloc(nbytes, BUF_ALLOCATE))
   {
      itrans(matrix, ncols, nrows, datatype);
      return(COMPLETE);
   }

   tmp = xpose.bufferpntr;
   svtmp = tmp;
   data = matrix;
 
   if (skip1 == REAL)
   {
      for (i = 0; i < nrows; i++)
      {
         tmp = svtmp;
         for (j = 0; j < ncols; j++)
         {
            *tmp++ = *data++;
            tmp += skip2;
         }
 
         svtmp += skip1;
      }
   }
   else if (skip1 == COMPLEX)
   {
      for (i = 0; i < nrows; i++)
      {
         tmp = svtmp;
         for (j = 0; j < ncols; j++)
         {
            *tmp++ = *data++;
            *tmp++ = *data++;
            tmp += skip2;
         }

         svtmp += skip1;
      }   
   }
   else if (skip1 == HYPERCOMPLEX)
   {
      for (i = 0; i < nrows; i++)
      {
         tmp = svtmp;
         for (j = 0; j < ncols; j++)
         {
            *tmp++ = *data++;
            *tmp++ = *data++;
            *tmp++ = *data++;
            *tmp++ = *data++;
            tmp += skip2;
         }

         svtmp += skip1;
      }
   }
 
   tmp = xpose.bufferpntr;
   data = matrix;
   ncols *= (nrows * skip1);
 
   for (i = 0; i < ncols; i++)
      *data++ = *tmp++;
 
   return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|                 transpose()/4                 |
|                                               |
|   This function calls the appropriate xpose   |
|   routine.                                    |
|                                               |
+----------------------------------------------*/
int transpose(void *matrix, int ncols, int nrows, int datatype)
{
   if (nrows == ncols)
   {
      symxpose((float *)matrix, nrows, datatype);
   }
   else
   {
      if (nonsymxpose((float *)matrix, ncols, nrows, datatype))
         return(ERROR);
   }
 
   return(COMPLETE);
}
