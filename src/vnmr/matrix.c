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
#include <string.h>
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
   void (*invertfunc)(float *, int, int, int, int, int);	/* pointer to a function */


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
  
/* Unified SIMD-friendly tile-transpose template */
#ifndef RBS
#define RBS 16  /* REAL tile size (elements per side)  */
#endif
#ifndef CBS
#define CBS 16  /* COMPLEX tile size */
#endif
#ifndef HBS
#define HBS 8   /* HYPERCOMPLEX tile size */
#endif

#define DEFINE_TILE_XPOSE(SUFFIX, ELEM_T, BS)                                \
static inline __attribute__((always_inline))                                 \
void t##SUFFIX(const ELEM_T s[BS][BS], ELEM_T d[BS][BS])                     \
{                                                                             \
   for (int i = 0; i < BS; i++)                                              \
      for (int j = 0; j < BS; j++)                                           \
         d[j][i] = s[i][j];                                                  \
}                                                                             \
static inline __attribute__((always_inline))                                 \
void tile_##SUFFIX(const ELEM_T *src, int src_ld, ELEM_T *dst, int dst_ld)   \
{                                                                             \
   ELEM_T in[BS][BS], out[BS][BS];                                           \
   for (int i = 0; i < BS; i++)                                              \
      memcpy(in[i], src + (size_t)i * src_ld, BS * sizeof(ELEM_T));          \
   t##SUFFIX(in, out);                                                       \
   for (int i = 0; i < BS; i++)                                              \
      memcpy(dst + (size_t)i * dst_ld, out[i], BS * sizeof(ELEM_T));         \
}
 
#define DEFINE_SCALAR_BLOCK(SUFFIX, ELEM_T)                                  \
static void scalar_block_##SUFFIX(const ELEM_T *src, ELEM_T *dst,            \
                                   int bi, int bj, int bh, int bw,           \
                                   int src_cols, int dst_cols)               \
{                                                                             \
   for (int i = 0; i < bh; i++)                                              \
      for (int j = 0; j < bw; j++)                                           \
         dst[(bj+j)*dst_cols + (bi+i)] = src[(bi+i)*src_cols + (bj+j)];      \
}
 
#define DEFINE_INPLACE_SQUARE(SUFFIX, ELEM_T, BS)                            \
static void symxpose_##SUFFIX(ELEM_T *mat, int n)                            \
{                                                                             \
   int i, j;                                                                 \
   for (i = 0; i + BS <= n; i += BS) {                                       \
      tile_##SUFFIX(mat+(size_t)i*n+i, n, mat+(size_t)i*n+i, n);             \
      for (j = i + BS; j + BS <= n; j += BS) {                               \
         ELEM_T a_in[BS][BS], a_out[BS][BS], b_in[BS][BS], b_out[BS][BS];    \
         for (int k = 0; k < BS; k++) {                                      \
            memcpy(a_in[k], mat+(size_t)(i+k)*n+j, BS*sizeof(ELEM_T));       \
            memcpy(b_in[k], mat+(size_t)(j+k)*n+i, BS*sizeof(ELEM_T));       \
         }                                                                   \
         t##SUFFIX(a_in, a_out); t##SUFFIX(b_in, b_out);                     \
         for (int k = 0; k < BS; k++) {                                      \
            memcpy(mat+(size_t)(i+k)*n+j, b_out[k], BS*sizeof(ELEM_T));      \
            memcpy(mat+(size_t)(j+k)*n+i, a_out[k], BS*sizeof(ELEM_T));      \
         }                                                                   \
      }                                                                      \
   }                                                                         \
   for (int r = 0; r < n; r++) {                                             \
      int cstart = (r >= i) ? 0 : i;                                         \
      for (int c = (r+1 > cstart ? r+1 : cstart); c < n; c++) {              \
         ELEM_T tmp_ = mat[r*n+c]; mat[r*n+c] = mat[c*n+r]; mat[c*n+r] = tmp_;\
      }                                                                      \
   }                                                                         \
}
 
#define DEFINE_OUTOFPLACE(SUFFIX, ELEM_T, BS)                                \
static void xpose_into_buffer_##SUFFIX(const ELEM_T *matrix, ELEM_T *buffer, \
                                        int ncols, int nrows)                \
{                                                                             \
   int i, j;                                                                 \
   for (i = 0; i + BS <= nrows; i += BS) {                                   \
      for (j = 0; j + BS <= ncols; j += BS)                                  \
         tile_##SUFFIX(matrix+(size_t)i*ncols+j, ncols,                      \
                        buffer+(size_t)j*nrows+i, nrows);                    \
      if (j < ncols)                                                         \
         scalar_block_##SUFFIX(matrix, buffer, i, j, BS, ncols-j, ncols, nrows); \
   }                                                                         \
   if (i < nrows)                                                            \
      scalar_block_##SUFFIX(matrix, buffer, i, 0, nrows-i, ncols, ncols, nrows); \
}
 
DEFINE_TILE_XPOSE(real,  float,        RBS)
DEFINE_TILE_XPOSE(cplx,  fcomplex,     CBS)
DEFINE_TILE_XPOSE(hyper, hypercomplex, HBS)
 
DEFINE_SCALAR_BLOCK(real,  float)
DEFINE_SCALAR_BLOCK(cplx,  fcomplex)
DEFINE_SCALAR_BLOCK(hyper, hypercomplex)
 
DEFINE_INPLACE_SQUARE(real,  float,        RBS)
DEFINE_INPLACE_SQUARE(cplx,  fcomplex,     CBS)
DEFINE_INPLACE_SQUARE(hyper, hypercomplex, HBS)
 
DEFINE_OUTOFPLACE(real,  float,        RBS)
DEFINE_OUTOFPLACE(cplx,  fcomplex,     CBS)
DEFINE_OUTOFPLACE(hyper, hypercomplex, HBS)

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
  if (datatype == HYPERCOMPLEX)      symxpose_hyper((hypercomplex *)matrix, npoints);
  else if (datatype == COMPLEX)      symxpose_cplx((fcomplex *)matrix, npoints);
  else if (datatype == REAL)         symxpose_real(matrix, npoints);
}

static int is_pow2(int n)
{
   return (n > 0) && ((n & (n - 1)) == 0);
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
 
   nbytes = sizeof(float) * nrows * ncols * datatype;
 
   if (xposebufalloc(nbytes, BUF_ALLOCATE))
   {
/* itrans()'s block-recursive in-place algorithm
 * only produces correct output when BOTH dimensions 
 * are individually powers of two  */
	  if (!is_pow2(nrows) || !is_pow2(ncols))
         return(ERROR);
      itrans(matrix, ncols, nrows, datatype);
      return(COMPLETE);
   }

   if (datatype == REAL)
      xpose_into_buffer_real(matrix, xpose.bufferpntr, ncols, nrows);
   else if (datatype == COMPLEX)
      xpose_into_buffer_cplx((const fcomplex *)matrix, (fcomplex *)xpose.bufferpntr, ncols, nrows);
   else if (datatype == HYPERCOMPLEX)
      xpose_into_buffer_hyper((const hypercomplex *)matrix, (hypercomplex *)xpose.bufferpntr, ncols, nrows);
 
   memcpy(matrix, xpose.bufferpntr, nbytes);

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
