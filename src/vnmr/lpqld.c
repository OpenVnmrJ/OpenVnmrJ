/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <math.h>

#include "ftpar.h"
#include "process.h"


#define MAXPATHL		128
#define ERROR			-1
#define COMPLETE		0
#define	FALSE			0
#define TRUE			1
#define LPEV_SN_FACTOR		3.0
#define COMPLEX			2

/* macros and constants for QLD matrix decomposition */
#define QLD_MAXITER		30
#define QLD_SMALL_NUMBER	1.0e-10
#define OPPOSITE_SIGN		0
#define SAME_SIGN		1
#define MIN_SEV			5.0e-14   /* minimum scaled eigenvalue */	

#define DSIGN(a,b)		( ((b) < 0.0) ? -fabs(a) : fabs(a) )

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef LINUX
#define LINUX_TOLERANCE 1e-8
#endif


/* constants for polynomial rooting */
#define ERROR_PRECISION		6.8e-8
#define MAX_PRECISION		1e-20
#define ROOT_PRECISION		1e-6
#define PR_MAXITER		1000

#define OS_MAX_SW		100000.0
#define OS_MAX_NP		512000
#define OS_MAX_NTAPS		50000
#define OS_MIN_NTAPS		3

#ifdef FT3D
extern void fidrotate(float *data, int npts, float pc0, float pc1, int datatype);
#else
extern void rotate_fid(float *fidptr, double ph0, double ph1, int np, int datatype);
#endif

static int	calcfidss = TRUE;
int coefcalc(dcomplex *scratch, dcomplex *lpcoef, lpinfo lppar, dcomplex *roots);
int findroots(dcomplex *polycoef, dcomplex *scratch, lpinfo lppar,
              dcomplex *roots, int rootadjust);
int invertdm(double *evals, int ncfilt, double lstrace);
int qldecomp(double *svonvector, double *svoffvector, double *evmatrix, int dimen);
int filterfid(ssparInfo parss, float *data, double *buffer,
              double *dfilter, int ncdatapts, int dataskip);

/*-------------------------------------------------------
|							|
|  Module:	lpqld.c					|
|  Author:	S. Farmer				|
|  Date:	December 12, 1991			|
|							|
|							|
|  This source module contains functions which calcu-	|
|  late the least-squares LP matrix (lsmatcalc) and	|
|  invert the least-squares matrix using QL matrix 	|
|  decomposition (hhtridiag + qldecomp).  There are a	|
|  number of additional supporting functions.		|
|							|
+------------------------------------------------------*/

/*-----------------------------------------------
|                                               |
|                  cxabs()/1                    |
|                                               |
+----------------------------------------------*/
double cxabs(dcomplex cval)
{
   register double      result,
                        tmp;


   result = cval.re;
   result *= result;
   tmp = cval.im;
   tmp *= tmp;
   result += tmp;
   result = sqrt(result);

   return(result);
}


/*-----------------------------------------------
|                                               |
|                  cdiv()/2                     |
|                                               |
|  This function divides two complex numbers    |
|  and returns a complex result.                |
+----------------------------------------------*/
dcomplex cdiv(dcomplex numerator, dcomplex denominator)
{
   register double      rval1,
                        rval2,
                        ival1,
                        ival2,
                        a,
                        b;
   dcomplex             result;

   rval1 = numerator.re;
   rval2 = denominator.re;
   ival1 = numerator.im;
   ival2 = denominator.im;

   a = rval1*rval2 + ival1*ival2;
   b = rval2*rval2 + ival2*ival2;
   result.re = a/b;
   a = ival1*rval2 - rval1*ival2;
   result.im = a/b;

   return(result);
}


/*-----------------------------------------------
|                                               |
|                  vnmrCsqrt()/1                    |
|                                               |
+----------------------------------------------*/
static dcomplex vnmrCsqrt(dcomplex cval)
{
   register double      rval,
                        ival,
                        rres,
                        ires;
   dcomplex             result;


   rval = cval.re;
   ival = cval.im;
   if ( fabs(rval) > fabs(ival) )
   {
      rres = sqrt(rval*rval + ival*ival) + rval;
      rres = sqrt(rres/2);
      ires = ival / (2*rres);
   }
   else
   {   
      ires = sqrt(rval*rval + ival*ival) - rval;
      ires = sqrt(ires/2);
      rres = ival / (2*ires);
   }

   result.re = rres;
   result.im = ires;

   return(result);
}


/*-----------------------------------------------
|                                               |
|		 lsmatcalc()/2			|
|						|
|   This function generates the Hermitian	|
|   LP**2 matrix which is to be later diagon-	|
|   alized.					|
|						|
|						|
|   Backwards Prediction:			|
|						|
|       |x1   x2   x3   x4|   |a1|     |x0|     |
|       |x2   x3   x4   x5|   |a2|  =  |x1|     |
|       |x3   x4   x5   x6|   |a3|     |x2|     |
|       |x4   x5   x6   x7|   |a4|     |x3|     |
|						|
|						|
|   Forwards Prediction:			|
|						|
|	|x0   x1   x2   x3|   |a4|     |x4|	|
|	|x1   x2   x3   x4|   |a3|  =  |x5|	|
|	|x2   x3   x4   x5|   |a2|     |x6|	|
|	|x3   x4   x5   x6|   |a1|     |x7|	|
|						|
|						|
|   The additional constraint which is used to  |
|   insure that the minimum length solution is  |
|   obtained is					|
|						|
|	  a1 + a2 + a3 + ... + aM = 0		|
|						|
+----------------------------------------------*/
void lsmatcalc(data, lppar)
fcomplex	*data;	/* pointer to original complex time-domain data	*/
lpinfo	*lppar;	/* LP parameter structure			*/
{
   int			nfpts,
			ncolpts;
   register int		i,
			j;
   register double	ftmp1,
			ftmp2,
			ftmp3,
			ftmp4,
			ftmp5,
			ftmp6;
   register fcomplex	*yval,
			*yval1,
			*yval2,
			*yval3;
   dcomplex		*svlpmatrix;
   register dcomplex	*lpmatrix,
			*tmplpmat;
   extern void		writelpmatrix();


   nfpts = lppar->ncfilt;
   ncolpts = lppar->ncupts - nfpts;
   lpmatrix = svlpmatrix = lppar->lppntr;

/****************************************
*  Calculate each element in the first  *
*  row of the Hermitian least-squares   *
*  LP matrix.                           *
****************************************/

   for (i = 0; i < nfpts; i++)
   {
      yval = data;
      if ( (~lppar->status) & FORWARD )
         yval += 1;

      yval1 = yval + i;
      lpmatrix->re = lpmatrix->im = 0.0;

      for (j = 0; j < ncolpts; j++)
      {
         ftmp1 = (double) (yval->re);
         ftmp2 = (-1.0) * (double) ((yval++)->im);
         ftmp3 = (double) (yval1->re);
         ftmp4 = (double) ((yval1++)->im);
         lpmatrix->re += ftmp1*ftmp3 - ftmp2*ftmp4;
         lpmatrix->im += ftmp1*ftmp4 + ftmp2*ftmp3;
      }

      lpmatrix += 1;
   }

/***********************************************
*  Start the recursion to calculate the other  *
*  elements of the symmetric least-squares     *
*  LP matrix.                                  *
***********************************************/

   yval = data;
   yval2 = yval + ncolpts;

   if ( (~lppar->status) & FORWARD )
   {
      yval += 1;	/* constant reference to be subtracted out */
      yval2 += 1;	/* constant reference to be added in	   */
   }

   for (i = 1; i < nfpts; i++)
   {
      tmplpmat = svlpmatrix + i;   /* stores the complex conjugate elements */
      for (j = 0; j < i; j++)
      {
         lpmatrix->re = tmplpmat->re;
         (lpmatrix++)->im = (-1.0) * tmplpmat->im;
         tmplpmat += nfpts;
      }

      yval1 = yval;
      yval3 = yval2;
      tmplpmat = svlpmatrix + (i - 1)*(nfpts + 1);

      ftmp1 = (double) (yval->re);
      ftmp2 = (-1.0) * (double) (yval->im);
      ftmp5 = (double) (yval2->re);
      ftmp6 = (-1.0) * (double) (yval2->im);

      for (j = i; j < nfpts; j++)
      {
         ftmp3 = (double) (yval1->re);
         ftmp4 = (double) ((yval1++)->im);
         lpmatrix->re = tmplpmat->re - (ftmp1*ftmp3 - ftmp2*ftmp4);
         lpmatrix->im = (tmplpmat++)->im - (ftmp1*ftmp4 + ftmp2*ftmp3);

         ftmp3 = (double) (yval3->re);
         ftmp4 = (double) ((yval3++)->im);
         lpmatrix->re += ftmp5*ftmp3 - ftmp6*ftmp4;
         (lpmatrix++)->im += ftmp5*ftmp4 + ftmp6*ftmp3;
      }

      yval += 1;	/* constant reference to be subtracted out */
      yval2 += 1;	/* constant reference to be added in	   */
   }

/*************************************************************
*  The result vector must now be calculated and appended to  *
*  the LP matrix.                                            *
*                                                            *
*  For backwards LP, the first M-1 elements of this result   *
*  vector can be quickly calculated from the LP matrix       *
*  elements (1,2) --> (1,M).                                 *
*                                                            *
*  For forwards LP, each element in the result vector must   *
*  be explicitly calculated.                                 *
*************************************************************/

   if (lppar->status & FORWARD)
   {
      lpmatrix += (nfpts*nfpts) + (nfpts - 1);
      for (i = 0; i < nfpts; i++)
      {
         yval = data + nfpts;
         yval1 = yval - 1 - i;
         lpmatrix->re = lpmatrix->im = 0.0;

         for (j = 0; j < ncolpts; j++)
         {
            ftmp1 = (double) (yval->re);
            ftmp2 = (double) ((yval++)->im);
            ftmp3 = (double) (yval1->re);
            ftmp4 = (-1.0) * (double) ((yval1++)->im);

            lpmatrix->re += ftmp1*ftmp3 - ftmp2*ftmp4;
            lpmatrix->im += ftmp1*ftmp4 + ftmp2*ftmp3;
         }

         lpmatrix -= 1;
      }
   }
   else
   {
      lpmatrix += (nfpts*nfpts);
      tmplpmat = svlpmatrix + 1;
      yval = data;
      yval1 = yval + 1;
      yval2 = data + ncolpts;
      yval3 = yval2 + 1;

      ftmp1 = (double) (yval2->re);
      ftmp2 = (-1.0) * (double) (yval2->im);
      ftmp5 = (double) (yval->re);
      ftmp6 = (-1.0) * (double) (yval->im);

      for (i = 1; i < nfpts; i++)
      {
         ftmp3 = (double) (yval3->re);
         ftmp4 = (double) ((yval3++)->im);
         lpmatrix->re = tmplpmat->re - (ftmp1*ftmp3 - ftmp2*ftmp4);
         lpmatrix->im = (tmplpmat++)->im - (ftmp1*ftmp4 + ftmp2*ftmp3);

         ftmp3 = (double) (yval1->re);
         ftmp4 = (double) ((yval1++)->im);
         lpmatrix->re += ftmp5*ftmp3 - ftmp6*ftmp4;
         lpmatrix->im += ftmp5*ftmp4 + ftmp6*ftmp3;
         (lpmatrix++)->im *= (-1.0);
      }

/*****************************************
*  The final Mth element of this result  *
*  vector must be calculated from first  *
*  principles.                           *
*****************************************/

      lpmatrix->re = lpmatrix->im = 0.0;
      yval = data;
      yval1 = data + nfpts;

      for (i = 0; i < ncolpts; i++)
      {
         ftmp1 = (double) (yval->re);
         ftmp2 = (double) ((yval++)->im);
         ftmp3 = (double) (yval1->re);
         ftmp4 = (-1.0) * (double) ((yval1++)->im);
         lpmatrix->re += ftmp1*ftmp3 - ftmp2*ftmp4;
         lpmatrix->im += ftmp1*ftmp4 + ftmp2*ftmp3;
      }
   }

/*********************************************
*  Calculate the trace of the least-squares  *
*  LP matrix.                                *
*********************************************/

   lpmatrix = svlpmatrix;
   ftmp1 = 0.0;
   for (i = 0; i < nfpts; i++)
   {
      ftmp1 += lpmatrix->re;
      lpmatrix += (nfpts + 1);
   }

   if (ftmp1 == 0.0)
      ftmp1 = 1.0;

   lppar->lstrace = ftmp1;
   writelpmatrix(*lppar);
}


/*-----------------------------------------------
|                                               |
|		  qld_solve()/1			|
|						|
+----------------------------------------------*/
dcomplex *qld_solve(lppar)
lpinfo	*lppar;
{
   int		nclpcoefs;
   double	*lsmatrix,
		*diagelem,
                *offdiagelem,
                *scratch;
   dcomplex	*lsresult,
		*lpcoef;
   void		shufflematrix(),
		hhtridiag(),
		sorteigensystem(),
		solvLS();
   extern void	writelpcoefs(),
		writelpeigenvalues();


/***********************************************
*  Memory required in double-precision words:  *
*                                              *
*  	   (4*lpfilt) * (lpfilt + 3) 	       *
***********************************************/

   nclpcoefs = lppar->ncfilt;
   lsmatrix = (double *)lppar->lppntr;
   lsresult = (dcomplex *) (lsmatrix + 4*nclpcoefs*nclpcoefs);
   diagelem = (double *) (lsresult + nclpcoefs);
   offdiagelem = diagelem + 2*nclpcoefs;
   scratch = offdiagelem + 2*nclpcoefs;
   lpcoef = (dcomplex *) (scratch + 4*nclpcoefs);

   shufflematrix(lsmatrix, 2*nclpcoefs);
   hhtridiag(lsmatrix, diagelem, offdiagelem, scratch, 2*nclpcoefs);
   if ( qldecomp(diagelem, offdiagelem, lsmatrix, 2*nclpcoefs) )
      return(NULL);

   sorteigensystem(diagelem, lsmatrix, (double **)offdiagelem,
			2*nclpcoefs,TRUE);

   writelpeigenvalues(diagelem, *lppar);
   lppar->nsignals = invertdm(diagelem, nclpcoefs, lppar->lstrace);
   solvLS(lpcoef, lsresult, diagelem, scratch, (double **)offdiagelem, *lppar);
   writelpcoefs(lpcoef, *lppar);
   return(lpcoef);
}


/*-----------------------------------------------
|						|
|	          solvLS()/6			|
|						|
+----------------------------------------------*/
void solvLS(lpcoef, lsresult, invevals, scratch, svevaddr, lppar)
double		*invevals,
		*scratch,
		**svevaddr;
dcomplex	*lpcoef,
		*lsresult;
lpinfo		lppar;
{
   register int		i,
			j,
			dimen,
			msize,
			nsignals;
   register double	ftmpr,
			ftmpi,
			*evmatr,
			*evmati,
			**evaddr;
   register dcomplex	*vec2,
			*vecs,
			*svvecs;


   dimen = lppar.ncfilt;
   msize = 2*dimen;
   nsignals = lppar.nsignals;
   vecs = svvecs = (dcomplex *)scratch;
   evaddr = svevaddr;

/*************************************************
*  Calculate the vector EV(t) * LSR where EV(t)  *
*  is the transpose of the eigenvector matrix    *
*  and LSR is the least-squares result vector.   *
*************************************************/
   
   for (i = 0; i < nsignals; i++)
   {
      vec2 = lsresult;
      vecs->re = vecs->im = 0.0;
      evmatr = *evaddr++;
      evmati = evmatr + (msize*msize)/2;

      for (j = 0; j < dimen; j++)
      {
         vecs->re += (*evmatr)*vec2->re - (*evmati)*vec2->im;
         vecs->im += (*evmatr)*vec2->im + (*evmati)*vec2->re;
         vec2 += 1;
         evmatr += msize;
         evmati += msize;
      }

      vecs += 1;
   }

/*********************************************
*  Calculate the vector 1/D * [EV(t) * LSR]  *
*  where 1/D is the inverse of the diagonal  *
*  matrix D.                                 *
*********************************************/

   evmatr = invevals;
   vecs = svvecs;

   for (i = 0; i < nsignals; i++)
   {
      vecs->re *= (*evmatr);
      (vecs++)->im *= (*evmatr++);
   }

/******************************************************
*  Calculate the vector EV(cc) * [1/D * EV(t) * LSR]  *
*  where EV(cc) is the complex conjugate of the       *
*  eigenvector matrix EV.                             *
******************************************************/

   evaddr = svevaddr;
   vec2 = lpcoef;
   vecs = svvecs;

   for (i = 0; i < dimen; i++)
   {
      vec2->re = vec2->im = 0.0;
      vec2 += 1;
   }

   for (j = 0; j < nsignals; j++)
   {
      vec2 = lpcoef;
      evmatr = *evaddr++;
      evmati = evmatr + (msize*msize)/2;
      ftmpr = vecs->re;
      ftmpi = (vecs++)->im;

      for (i = 0; i < dimen; i++)
      {
         vec2->re += (*evmatr)*ftmpr + (*evmati)*ftmpi;
         vec2->im += (*evmatr)*ftmpi - (*evmati)*ftmpr;
         vec2 += 1;
         evmatr += msize;
         evmati += msize;
      }
   }
}


/*-----------------------------------------------
|						|
|	         invertdm()/3			|
|						|
+----------------------------------------------*/
int invertdm(double *evals, int ncfilt, double lstrace)
{
   register int		invert = TRUE,
			nsignals = 0;
   register double	*vec1;


   vec1 = evals;

   while (invert)
   {
      if ( (fabs(*vec1)/lstrace) > MIN_SEV )
      {
         *vec1 = 1/(*vec1);
         vec1 += 1;		/*
				   necessary to split these two lines up
				   because of a bug in the VAX VMS C
				   compiler.   RL/SF 6-11-92
				*/
         nsignals += 1;
         invert = (nsignals < ncfilt);
      }
      else
      {
         invert = FALSE;
      }
   }

   return(nsignals);
}


/*-----------------------------------------------
|						|
|	      sorteigensystem()/5		|
|						|
+----------------------------------------------*/
void sorteigensystem(evals, evmatrix, svevaddr, dimen, pack)
int		dimen,
		pack;
double		*evals,
		*evmatrix,
		**svevaddr;
{
   int			sort = TRUE,
			setindex,
			stindex = 0;
   register int		i,
			msize;
   register double	*evmat1,
			*vec1,
			*vec2,
			**evaddr1,
			**evaddr2,
			*vec3,
			exch;


/**********************************************
*  Initialize the array storing the address   *
*  of each column of the eigenvector matrix.  *
*  The pointer is treated as complex because  *
*  the final matrix will be complex.          *
**********************************************/

   msize = dimen;
   evmat1 = evmatrix;
   *svevaddr = evmat1;
   evaddr1 = svevaddr;
   evaddr1 += 1;

   for (i = 1; i < msize; i++)
      *evaddr1++ = *svevaddr + i;

/********************************************
*  Sort the eigenvalues according to their  *
*  absolute magnitudes.  The eigenvecters   *
*  are shuffled in concert with the eigen-  *
*  values.                                  *
********************************************/

   while (sort)
   {
      sort = FALSE;
      setindex = TRUE;
      vec2 = evals + stindex;
      vec1 = vec2 - 1;
      evaddr2 = svevaddr + stindex;
      evaddr1 = evaddr2 - 1;
 
      for (i = stindex; i < (msize - 1); i++)
      {
         evaddr1 += 1;
         evaddr2 += 1;

         if ( fabs(*(++vec1)) < fabs(*(++vec2)) )
         {  
            sort = TRUE;
            exch = *vec1;		/* exchange eigenvalues		  */
            *vec1 = *vec2;
            *vec2 = exch;

            vec3 = *evaddr1;		/* exchange eigenvector addresses */
            *evaddr1 = *evaddr2;
            *evaddr2 = vec3;
 
            if (setindex)
            {
               stindex = ( i ? (i - 1) : 0 );
               setindex = FALSE;
            }
         }
      }
   }
 
/************************************************
*  Pack the eigenvalues, removing the two-fold  *
*  degeneracy.  Pack the eigenvectors into the  *
*  left-hand side of the matrix.                *
************************************************/

   if (pack)
   {
      vec2 = evals;                                  
      vec1 = vec2 + 1;
      evaddr2 = svevaddr;
      evaddr1 = evaddr2 + 1;

      for (i = 1; i < (msize/2); i++)
      {
         vec2 += 2;
         *vec1++ = *vec2;
         evaddr2 += 2;
         *evaddr1++ = *evaddr2;
      }
   }
}


/*-----------------------------------------------
|						|
|	       shufflematrix()/2		|
|						|
+----------------------------------------------*/
void shufflematrix(matrix, dimen)
int	dimen;
double	*matrix;
{
   register int		i,
			j;
   register double	*data,
			*realbuf,
			*imagbuf;


   data = matrix;
   realbuf = data + (dimen*dimen)/2;
   imagbuf = realbuf + (dimen/2);

/*******************************************
*  Separate the real and imaginary parts.  *
*******************************************/

   for (i = 0; i < (dimen/2); i++)
   {
      for (j = 0; j < (dimen/2); j++)
      {
         *realbuf++ = *data++;
         *imagbuf++ = *data++;
      }

      for (j = 0; j < (dimen/2); j++)
         *(--data) = *(--imagbuf);

      for (j = 0; j < (dimen/2); j++) 
         *(--data) = *(--realbuf);

      data += dimen;
   }

/*****************************
*  Duplicate the real part.  *
*****************************/

   data = matrix;
   realbuf = data + (dimen*dimen)/2;

   for (i = 0; i < (dimen/2); i++)
   {
      realbuf += (dimen/2);
      for (j = 0; j < (dimen/2); j++)
         *realbuf++ = *data++;

      data += (dimen/2);
   }

/**********************************
*  Duplicate the imaginary part.  *
*  Negate so that the matrix is   *
*  still symmetric.               *
**********************************/

   data = matrix;
   imagbuf = data + (dimen*dimen)/2;

   for (i = 0; i < (dimen/2); i++) 
   {
      data += (dimen/2);
      for (j = 0; j < (dimen/2); j++) 
         *imagbuf++ = (-1.0) * (*data++);

      imagbuf += (dimen/2);
   }
}


/*-----------------------------------------------
|						|
|		 hhtridiag()/5			|
|						|
|   This function reduces a Hermitian matrix    |
|   to a tri-diagonal matrix which is both      |
|   real and symmetric.  The decomposition is   |
|   done by the method of Householder.  The     |
|   eigenvector matrix is returned in place of  |
|   the original matrix and is defined such     |
|   that					|
|						|
|	      M = EV(adj) * D * EV              |
|						|
|						|
|   where D is the real, diagonal matrix and    |
|   EV(adj) is the adjoint of the eigenvector   |
|   matrix EV.  M is the original, Hermitian    |
|   matrix.					|
|						|
+----------------------------------------------*/
void hhtridiag(matrix, diag, offdiag, scratch, dimen)
int		dimen;
double		*diag,
		*offdiag,
		*matrix,
		*scratch;
{
   register int		i,
			j,
			r;
   double		hfactor,
			kfactor,
			srootsigma;
   register double	sigma,
			scale,
			*uvector,
			*qvector,
			*mat1,
			*mat2;


/************************
*  Initialize vectors.  *
************************/

   diag += dimen;
   offdiag += dimen;
   *(--offdiag) = 0.0;		/* initialization of last element */

/**************************
*  Start the Householder  *
*  decomposition.         *
**************************/

   for (r = dimen; r > 2; r--)
   {
      scale = sigma = 0.0;
      mat1 = matrix + (r - 1) * dimen;
      mat2 = mat1;

      for (i = 0; i < (r - 1); i++)
         scale += fabs(*mat1++);

      if (scale < QLD_SMALL_NUMBER)
      {
         *(--offdiag) = 0.0;
         *(--diag) = *(mat2 + r - 1);
      }
      else
      {

/***********************
*  Calculate "sigma".  *
***********************/

         mat1 = mat2;
         for (i = 0; i < (r - 1); i++)
         {
            *mat1 /= scale;
            sigma += (*mat1) * (*mat1);
            mat1 += 1;
         }

         mat1 -= 1;
         srootsigma = ( (*mat1 > 0.0) ? 1.0 : -1.0 ) * sqrt(sigma);
         *(--offdiag) = (-1) * scale * srootsigma;
         mat1 = mat2;
         uvector = scratch;	/* scratch is of dimension 2*dimen */

/*********************************
*  Calculate the "u" vector and  *
*  the "h" scaling factor.       *
*********************************/

         for (i = 0; i < (r - 2); i++)
            *uvector++ = *mat1++;

         *uvector = *mat1 + srootsigma;
         hfactor = sigma + (*mat1) * srootsigma;

/******************************
*  Calculate the "p" vector.  *
******************************/

         qvector = scratch + dimen;

         for (i = 0; i < (r - 1); i++)
         {
            mat1 = matrix + i*dimen;
            uvector = scratch;
            *qvector = 0.0;

            for (j = 0; j < (r - 1); j++)
               *qvector += (*mat1++) * (*uvector++);

            *qvector++ /= hfactor;
         }

         *qvector = 1.0;

/**************************************************
*  Store the "u" vector in the "r"-th row of the  *
*  original matrix.  Store the "u(adj)/H" vector  *
*  in the "r"-th column of the original matrix.   *
**************************************************/

         mat1 = mat2;
         uvector = scratch;

         for (i = 0; i < (r - 1); i++)
            *mat1++ = *uvector++;		/* load u	*/

         *(--diag) = *mat1;
         mat1 = matrix + (r - 1);
         uvector = scratch;

         for (i = 0; i < (r - 1); i++)		/* load u/H	*/
         {
            *mat1 = (*uvector++) / hfactor;
            mat1 += dimen;
         }

/******************************
*  Calculate the "k" factor.  *
******************************/

         kfactor = 0.0;
         uvector = scratch;
         qvector = uvector + dimen;

         for (i = 0; i < (r - 1); i++)
            kfactor += (*uvector++) * (*qvector++);

         kfactor /= 2*hfactor;

/******************************
*  Calculate the "q" vector.  *
******************************/

         uvector = scratch;
         qvector = uvector + dimen;

         for (i = 0; i < (r - 1); i++)
            *(qvector++) -= kfactor * (*uvector++);

         *qvector = 1.0;

/**************************************
*  Calculate new matrix which is now  *
*  tridiagonal in the "r"-th row and  *
*  column.                            *
**************************************/

         for (i = 0; i < (r - 1); i++)
         {
            mat1 = matrix + i*dimen;
            uvector = scratch;
            qvector = uvector + dimen;
            scale = *(qvector + i);
            sigma = *(uvector + i);

            for (j = 0; j < (r - 1); j++)
               (*mat1++) -= scale * (*uvector++) + (*qvector++) * sigma;
         }
      }
   }

   *(--diag) = *(matrix + dimen + 1);	/* penultimate diagonal element  */
   *(--diag) = *matrix; 		/* final diagonal element 	 */
   *(--offdiag) = *(matrix + 1);	/* final offdiagonal  element	 */

/**************************************
*  Calculate the eigenvector matrix.  *
**************************************/

   for (i = 0; i < dimen; i++)
   {
      if (i > 1)
      {
         for (j = 0; j < i; j++)
         {
            kfactor = 0.0;
            mat1 = matrix + i*dimen;
            mat2 = matrix + j;

            for (r = 0; r < i; r++)
            {
               kfactor += (*mat1++) * (*mat2);
               mat2 += dimen;
            }

            mat1 = matrix + j;
            mat2 = matrix + i;

            for (r = 0; r < i; r++)
            {
               *mat1 -= kfactor * (*mat2);
               mat1 += dimen;
               mat2 += dimen;
            }
         }
      }

      mat1 = matrix + i*dimen + i;
      *mat1 = 1.0;
      mat1 -= i;
      mat2 = matrix + i;

      for (j = 0; j < i; j++)
      {
         *mat1++ = *mat2 = 0.0;
         mat2 += dimen;
      }
   }
}


/*-----------------------------------------------
|                                               |
|                 qldecomp()/4                  |
|                                               |
|  This function performs the QL decomposition  |
|  of a tri-diagonal matrix using implicit      |
|  shifts.                                      |
|                                               |
+----------------------------------------------*/
int qldecomp(double *svonvector, double *svoffvector, double *evmatrix, int dimen)
{
   int          	i,
			j,
                	iter,
                	iterflag,
                	k,
                	m;
   double       	g,
			r,
                	p,
                	b;
   register double	s,
			c,
			f,
			*onvector,
			*evmat,
			*offvector;


   onvector = svonvector;
   offvector = svoffvector;

   for (k = 0; k < dimen; k++)
   {
      iter = 0;
      iterflag = TRUE;
 
      while (iterflag)
      {
         onvector = svonvector + k;
         offvector = svoffvector + k;
         m = k;
 
         while (m < (dimen - 1))
         {
            g = fabs(*onvector++);
            g += fabs(*onvector);
            if ( (fabs(*offvector++) + g) == g )
               break;
 
            m += 1;
         }
 
         if (m != k)
         {
            if (iter == QLD_MAXITER)
               return(ERROR);

            onvector = svonvector;
            offvector = svoffvector;
            iter += 1;

            g = ((*(onvector + k + 1)) - (*(onvector + k)))/
                   (2.0 * (*(offvector + k)));
            r = sqrt(g*g + 1.0);
            g = (*(onvector + m)) - (*(onvector + k)) +
                   (*(offvector + k))/(g + DSIGN(r,g));
 
            s = 1.0;
            c = 1.0;
            p = 0.0;
            onvector += m;
            offvector += (m - 1);
 
            for (i = (m - 1); i >= k; i--)
            {
               f = s * (*offvector);
               b = c * (*offvector);
             
               if (fabs(f) >= fabs(g))
               {
                  c = g/f;
                  r = sqrt(c*c + 1.0);
                  *(offvector + 1) = f * r;
                  s = 1.0/r;
                  c *= s;
               }
               else
               {
                  s = f/g;
                  r = sqrt(s*s + 1.0);
                  *(offvector + 1) = g*r;
                  c = 1.0/r;
                  s *= c;
               }

               g = (*onvector--) - p;
               r = ((*onvector) - g)*s + 2.0*c*b;
               p = s * r;
               *(onvector + 1) = g + p;
               g = c*r - b;
               offvector -= 1;
               evmat = evmatrix + i + 1;

               for (j = 0; j < dimen; j++)
               {
                  f = *evmat;
                  *evmat = s * (*(evmat - 1)) + (c*f);
                  *(--evmat) *= c;
                  *evmat -= s*f;
                  evmat += dimen + 1;
               } 
            }

            *(svonvector + k) -= p;
            *(svoffvector + k) = g;
            *(svoffvector + m) = 0.0;
         }
         else
         {
            iterflag = FALSE;
         }     
      }
   }

   return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|		  rootpolynm()/3		|
|						|
+----------------------------------------------*/
int rootpolynm(dcomplex *lpcoef, lpinfo lppar, int rootadjust)
{
   dcomplex	*scratch,
		*roots;
   extern void		writelpcoefs();


   if (lppar.nsignals == 0)
   {
      writelpcoefs(lpcoef, lppar);
      return(COMPLETE);
   }

   scratch = lppar.lppntr + (lppar.membytes / sizeof(dcomplex) )
                        - 2*lppar.ncfilt - 1;
   roots = scratch + lppar.ncfilt + 1;
 
   if ( findroots(lpcoef, scratch, lppar, roots, rootadjust) )
      return(ERROR);
 
   if ( coefcalc(scratch, lpcoef, lppar, roots) )
      return(ERROR);

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		  polysolve()/4			|
|						|
+----------------------------------------------*/
static int polysolve(polycoef, degree, polyroot, polish)
int		polish,
		degree;
dcomplex	*polycoef,
		*polyroot;
{
   int			i,
			j;
   dcomplex		b,
			d,
			f,
			droot,
			root;
   register double	tmp,
			old_droot,
			abs_droot,
			abs_root,
			err;
   register dcomplex	*data;


   root = *polyroot;
   old_droot = cxabs(root);

   for (i = 0; i < PR_MAXITER; i++)
   {
      data = polycoef + degree;
      b = *data--;
      err = cxabs(b);
      d.re = d.im = f.re = f.im = 0.0;
      abs_root = cxabs(root);

      for (j = 0; j < degree; j++)
      {
         tmp = (root.re*f.re) - (root.im*f.im) + d.re;
         f.im = (root.re*f.im) + (root.im*f.re) + d.im;
         f.re = tmp;
         tmp = (root.re*d.re) - (root.im*d.im) + b.re;
         d.im = (root.re*d.im) + (root.im*d.re) + b.im;
         d.re = tmp;
         tmp = (root.re*b.re) - (root.im*b.im) + data->re;
         b.im = (root.re*b.im) + (root.im*b.re) + (data--)->im;
         b.re = tmp;
         err = cxabs(b) + abs_root*err;
      }

      err *= ERROR_PRECISION;
      if ( cxabs(b) < err )
      {
         *polyroot = root;
         return(COMPLETE);
      }

      d = cdiv(d, b);
      f = cdiv(f, b);
      b.re = (d.re*d.re) - (d.im*d.im);
      b.im = 2*d.re*d.im;
      f.re = b.re - 2*f.re;
      f.im = b.im - 2*f.im;
      f.re = (double) (degree - 1) * (degree*f.re - b.re);
      f.im = (double) (degree - 1) * (degree*f.im - b.im);
      f = vnmrCsqrt(f);
      b.re = d.re + f.re;
      b.im = d.im + f.im;
      f.re = d.re - f.re;
      f.im = d.im - f.im;

      if ( cxabs(b) < cxabs(f) )
         b = f;

      f.re = (double)degree;
      f.im = 0.0;
      droot = cdiv(f, b);

      if ( cxabs(droot) < (MAX_PRECISION * cxabs(root)) )
      { 
         *polyroot = root; 
         return(COMPLETE);
      }

      root.re -= droot.re;
      root.im -= droot.im;
      abs_droot = cxabs(droot);

      if ( (i > 6) && (abs_droot > old_droot) )
      {  
         *polyroot = root;
         return(COMPLETE);
      }

      old_droot = abs_droot;
      if (!polish)
      {
         if ( abs_droot < (ROOT_PRECISION * cxabs(root)) )
         {
            *polyroot = root;
            return(COMPLETE);
         }
      }
   }

   return(ERROR);
}


/*-----------------------------------------------
|						|
|		  findroots()/5			|
|						|
|						|
| The polynomial coefficients must be arranged	|
| in the following order:			|
|						|
|       {a(ncfilt), ..., a3, a2, a1, C}		|
|						|
|						|
|  where C is the constant (1.0, 0.0) in com-	|
|  plex notation.				|
|						|
+----------------------------------------------*/
int findroots(dcomplex *polycoef, dcomplex *scratch, lpinfo lppar,
              dcomplex *roots, int rootadjust)
{
   int			degree,
			sort = TRUE,
			setindex,
			stindex = 0;
   register int		i,
			j;
   double		rmagn;
   dcomplex		b,
			c,
			root;
   register double	tmp;
   register dcomplex	*proots,
			*pcoefs;
   extern void		writelproots();


/******************************************
*  Add +1 to the front of the LP vector.  *
******************************************/

   degree = lppar.ncfilt;
   proots = polycoef;
   pcoefs = scratch;
   for (i = 0; i < degree; i++)
   {
      pcoefs->re = (-1.0) * proots->re;
      (pcoefs++)->im = (-1.0) * (proots++)->im;
   }

   pcoefs->re = 1.0;
   pcoefs->im = 0.0;
   proots = roots + degree;

   for (i = degree; i > 1; i--)
   {
      pcoefs = scratch + i;
      root.re = root.im = 0.0;
      if ( polysolve(scratch, i, &root, FALSE) )
         return(ERROR);

      b = *pcoefs--;
      for (j = i; j > 0; j--)
      {
         c = *pcoefs;
         *pcoefs-- = b;
         tmp = (root.re*b.re) - (root.im*b.im) + c.re;
         b.im = (root.re*b.im) + (root.im*b.re) + c.im;
         b.re = tmp;
      }

      *(--proots) = root;
   }

   (--proots)->re = (-1) * (++pcoefs)->re;
   proots->im = (-1) * pcoefs->im;

   proots = polycoef;
   pcoefs = scratch;
   for (i = 0; i < degree; i++)
   {
      pcoefs->re = (-1.0) * proots->re;
      (pcoefs++)->im = (-1.0) * (proots++)->im;
   }

   pcoefs->re = 1.0;
   pcoefs->im = 0.0;
   proots = roots;

   for (i = 0; i < degree; i++)
   {
      if ( polysolve(scratch, degree, proots++, TRUE) )
         return(ERROR);
   }

   while (sort)
   {
      sort = FALSE;
      setindex = TRUE;
      pcoefs = roots + stindex;
      proots = pcoefs - 1;

      for (i = stindex; i < (degree - 1); i++)
      {
#ifdef LINUX_TOLERANCE
         /*  For what appears to be a Linux compiler problem,
          *  the if statement fails if the values are the same to about
          *  six decimal places. This can cause the sort to get stuck
          *  in an endless loop.
          */
         double val1, val2;

         val1 = cxabs(*(++proots));
         val2 = cxabs(*(++pcoefs));
         if ((fabs(val1-val2) > LINUX_TOLERANCE) && ( val1 < val2 ))
#else
         if ( cxabs(*(++proots)) < cxabs(*(++pcoefs)) )
#endif
         {
            sort = TRUE;
            b.re = proots->re;
            b.im = proots->im;
            proots->re = pcoefs->re;
            proots->im = pcoefs->im;
            pcoefs->re = b.re;
            pcoefs->im = b.im;

            if (setindex)
            {
               stindex = ( i ? (i - 1) : 0 );
               setindex = FALSE;
            }
         }
      }
   }

   writelproots(polycoef, roots, lppar, BEFORE_ROOTADJUST);
   proots = roots;
   c.re = 1.0;
   c.im = 0.0;

   if (rootadjust)
   {
      for (i = 0; i < degree; i++)
      {
         if ( (rmagn = cxabs(*proots)) > 1.0 )
         {
            proots->re /= rmagn;
            proots->im /= rmagn;
         }   
            
         proots += 1;
      }

      writelproots(polycoef, roots, lppar, AFTER_ROOTADJUST);
   }

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		  coefcalc()/4			|
|						|
|						|
|  The LP coefficients recalculated from the 	|
|  LP roots are in the following order:		|
|						|
|	 {a1, a2, a3, a4, ..., a(ncfilt)}	|
|						|
+----------------------------------------------*/
int coefcalc(dcomplex *scratch, dcomplex *lpcoef, lpinfo lppar, dcomplex *roots)
{
   register int		i,
			j,
			degree;
   register double	re_root,
			im_root;
   register dcomplex	*cvector,
			*bvector,
			*vector,
			*proots;
   extern void		writelpcoefs();


   proots = roots;
   vector = scratch;
   cvector = vector;
   degree = lppar.ncfilt;

   cvector->re = 1.0;		/* initialization */
   (cvector++)->im = 0.0;
   for (i = 0; i < degree; i++)
   {
      cvector->re = 0.0;
      (cvector++)->im = 0.0;
   }

   for (i = 0; i < degree; i++)
   {
      re_root = proots->re;
      im_root = (proots++)->im;
      bvector = vector + i;
      cvector = bvector + 1;

      for (j = 0; j < (i + 1); j++)
      {
         cvector->re += im_root*bvector->im - re_root*bvector->re;
         (cvector--)->im -= re_root*bvector->im + im_root*bvector->re;
         bvector -= 1;
      }
   }

   vector = scratch + 1;
   bvector = lpcoef;
   for (i = 0; i < degree; i++)
   {
      bvector->re = (-1.0) * vector->re;
      (bvector++)->im = (-1.0) * (vector++)->im;
   }

   writelpcoefs(lpcoef, lppar);
   return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|               calcrootdiff()/3                |
|                                               |
+----------------------------------------------*/
double calcrootdiff(polycoefs, root, ncoefs)
int             ncoefs;
dcomplex        *polycoefs,
                root;
{
   register int         i;
   register double      reroot,
                        imroot,
                        reval,
                        imval,
                        tmpval;
   register dcomplex    *pcoefs;
 
 
/*********************************************
*  The characteristic equation is given by   *
*                                            *
*        a3*x^^3 + a2*x^^2 + a1*x + a0       *
*                                            *
*  There is one more polynomial coefficient  *
*  than there is LP coefficient.  One must   *
*  start with a3.                            *
*********************************************/

   pcoefs = polycoefs + ncoefs;
   reroot = root.re;
   imroot = root.im;
   reval = 1.0;
   imval = 0.0;

   for (i = 0; i < ncoefs; i++)
   {
      tmpval = reval*reroot - imval*imroot;
      imval = reval*imroot + imval*reroot;
      reval = tmpval;
      reval -= (--pcoefs)->re;
      imval -= pcoefs->im;
   }

   root.re = reval;
   root.im = imval;
 
   return( cxabs(root) );
}


/*-----------------------------------------------
|                                               |
|                  fidss()/4                    |
|                                               |
+----------------------------------------------*/
int fidss(ssparInfo sspar, float *data, int ncdpts, int nclspts)
{
   int                  nLSpts,
			msize,
                        i, j;
   register float	*lfsdata;
   double               plstrace,
			*lsmatrix,
                        *lsresult,
                        *diagelem,
                        *offdiagelem,
                        *scratch,
                        *polycoef,
                        *polycurve,
			*digfilter,
			calcpolyLS();
   register double	*pcurve;
   void                 solvpolyLS(),
                        correctfid();
   static int           nevals;
   static double	fidss_scale;
   double		sslsfrq_tmp;


   digfilter = sspar.buffer;
   msize = sspar.matsize;

   lsmatrix = digfilter + sspar.ntaps + 1;
   lsresult = lsmatrix + (msize*msize);
   diagelem = lsresult + msize;
   offdiagelem = diagelem + msize;
   scratch = offdiagelem + msize;
   polycoef = scratch + msize;
   polycurve = polycoef + msize;
 
/************************************
*  Correct the real data points in  *
*  the time-domain FID.             *
************************************/
 
   if (nclspts > 0)
   {
      ncdpts -= nclspts;	/* not taken care of in function call */
   }
   else if (nclspts < 0)
   {
      data -= 2*nclspts; 
   }

   for (j=0; j<sspar.sslsfrqsize; j++)
   {
      sslsfrq_tmp = sspar.sslsfrq[j];
      if (fabs(sslsfrq_tmp) > 0.0001)
      {
#ifdef FT3D
        fidrotate(data, ncdpts, (double) 0.0, sslsfrq_tmp, COMPLEX);
#else 
        rotate_fid(data, (double) 0.0, sslsfrq_tmp, 2*ncdpts, COMPLEX);
#endif 
      }

      nLSpts = filterfid(sspar, data, polycurve, digfilter, ncdpts, COMPLEX);
 
      if (sspar.zfsflag)
      {
         plstrace = calcpolyLS(lsmatrix, polycurve, lsresult, nLSpts, msize,
				&fidss_scale);
         if (calcfidss)
         {
            calcfidss = FALSE;
            hhtridiag(lsmatrix, diagelem, offdiagelem, scratch, msize);
            if ( qldecomp(diagelem, offdiagelem, lsmatrix, msize) )
               return(ERROR);
 
            sorteigensystem(diagelem, lsmatrix, (double **)offdiagelem, msize,
                           FALSE);
            nevals = invertdm(diagelem, msize, plstrace);
         }
 
         solvpolyLS(polycoef, lsresult, diagelem, scratch, (double **)offdiagelem,
                      msize, nevals, fidss_scale);
         correctfid(data, ncdpts, polycoef, msize, COMPLEX);
      }
      else
      {
         lfsdata = data;
         pcurve = polycurve;

         for (i = 0; i < ncdpts; i++)
         {
            *lfsdata -= (float) (*pcurve++);
            lfsdata += 2;
         }
      }

/*****************************************
*  Correct the imaginary data points in  *
*  the time-domain FID.                  *
*****************************************/
 
      nLSpts = filterfid(sspar, data + 1, polycurve, digfilter, ncdpts, COMPLEX);
 
      if (sspar.zfsflag)
      {
         plstrace = calcpolyLS(lsmatrix, polycurve, lsresult, nLSpts, msize,
				&fidss_scale);
         solvpolyLS(polycoef, lsresult, diagelem, scratch, (double **)offdiagelem,
                   msize, nevals, fidss_scale);
 
         correctfid(data + 1, ncdpts, polycoef, msize, COMPLEX);
      }
      else
      {
         lfsdata = data + 1;
         pcurve = polycurve;

         for (i = 0; i < ncdpts; i++)
         {
            *lfsdata -= (float) (*pcurve++);
            lfsdata += 2;
         }
      }

      if (fabs(sslsfrq_tmp) > 0.0001)
      {
#ifdef FT3D
        fidrotate(data, ncdpts, (double) 0.0, -sslsfrq_tmp, COMPLEX);
#else 
        rotate_fid(data, (double) 0.0, -sslsfrq_tmp, 2*ncdpts, COMPLEX);
#endif 
      }
   }

   return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|              set_calcfidss()/1                |
|                                               |
+----------------------------------------------*/
void set_calcfidss(value)
int     value;
{
   calcfidss = value;
   if (calcfidss)
      calcfidss = TRUE;
}


/*---------------------------------------
|                                       |
|           calc_digfilter()/3          |
|                                       |
+--------------------------------------*/
void calc_digfilter(dbuffer, ntaps, decfactor)
int	ntaps,
	decfactor;
double	*dbuffer;
{
   register int         i,
                        j,
			nfullpts;
   register double      *tmpfilter,
			*tmpfilter2,
                        wc,
                        hd,
                        w, 
			sum,
                        arg1;
   int max;

   tmpfilter = dbuffer;
   tmpfilter2 = dbuffer+ntaps;
   wc = M_PI / ((double)decfactor);
   sum = 0.0;
   nfullpts = ntaps;

   max=(ntaps+1)/2;
   for (i = 0; i < max; i++)         
   {
      j = i - max + 1;
      hd = ( j ? sin(wc * (double)j) / (M_PI * (double)j) :
		  1.0/(double)decfactor );
      arg1 = (2 * M_PI) / (double) (nfullpts - 1);
      w = 0.42 - 0.5*cos(arg1 * (double)i) + 0.08*cos(2 * arg1 * (double)i);
      *tmpfilter = hd * w;
      *(--tmpfilter2) = *tmpfilter;
      sum += (*tmpfilter++);
      if (tmpfilter-1 != tmpfilter2)
        sum += (*tmpfilter2);
   }

   *(dbuffer+ntaps) = sum;

#ifdef DEBUG
   for (i=0;i<ntaps;i++)
     fprintf(stderr,"%d  0\n",(int)(100000*dbuffer[i]+0.5));
#endif 
}


/*-----------------------------------------------
|                                               |
|                filterfid()/6                  |
|                                               |
+----------------------------------------------*/
int filterfid(ssparInfo parss, float *data, double *buffer,
              double *dfilter, int ncdatapts, int dataskip)
{
   int                  nLSpts,
			ntaps;
   register int         i,
                        j;
   register float       *tmpdata;
   double               sfactor;
   register double      *tmpbuffer,
                        *tmpdfilter,
                        sum;
   int coeff_start, coeff_len, ntaps_2; 

   ntaps = parss.ntaps;
   nLSpts = ncdatapts - ntaps + 1;
   tmpdata = data;
   tmpdfilter = dfilter;

   tmpbuffer = buffer;
   for (i = 0; i < ncdatapts; i++)
   {
      *tmpbuffer++ = (double) (*tmpdata);
      tmpdata += dataskip;
   }
   tmpbuffer = buffer;
   ntaps_2 = ntaps/2;
   coeff_start = ntaps_2;
   coeff_len = ntaps-coeff_start;
   sfactor = 0.0;
   tmpdfilter=dfilter+coeff_start;
   for (i = 0; i < coeff_len; i++)
     sfactor += (*tmpdfilter++);

   for (i = 0; i < ntaps_2; i++)  {
     tmpdfilter=dfilter+coeff_start;
     tmpbuffer = buffer-(ntaps_2-coeff_start);
     sum = 0.0;
     for (j=0;j<coeff_len;j++)
        sum += (*(tmpbuffer++)) * (*(tmpdfilter++));
     *buffer++ = sum/sfactor;
     coeff_len++;
     coeff_start--;
     sfactor += (*(dfilter+coeff_start));
     }
   for (i = 0; i < ncdatapts-2*(ntaps_2); i++)  {
     tmpdfilter=dfilter+coeff_start;
     tmpbuffer = buffer-(ntaps_2-coeff_start);
     sum = 0.0;
     for (j=0;j<coeff_len;j++)
        sum += (*(tmpbuffer++)) * (*(tmpdfilter++));
     *buffer++ = sum/sfactor;
     }
   for (i = 0; i < ntaps_2; i++)  {
     coeff_len--;
     sfactor -= (*(dfilter+coeff_start+coeff_len));
     tmpdfilter=dfilter+coeff_start;
     tmpbuffer = buffer-(ntaps_2-coeff_start);
     sum = 0.0;
     for (j=0;j<coeff_len;j++)
        sum += (*(tmpbuffer++)) * (*(tmpdfilter++));
     *buffer++ = sum/sfactor;
     }

   return(nLSpts);
}
 
 
/*-----------------------------------------------
|                                               |
|               calcpolyLS()/6                  |
|                                               |
+----------------------------------------------*/
double calcpolyLS(lsmatrix, polycurve, lsresult, nLSpts,
				lsmatsize, scale)
int     nLSpts,
        lsmatsize;
double  *lsmatrix,
        *lsresult,
        *polycurve,
        *scale;
{
   register int         i,
                        j,
                        count,
                        savecount;
   register double      sum,
                        tmpstore,
                        *tmpdata,
                        *tmpresult;
   static double	polylstrace = 1.0;	/* for safety */
 
 
/*****************************************
*  Calculate the unique elements in the  *
*  LS data matrix.                       *
*****************************************/
    
   if (calcfidss)
   {
      tmpdata = lsmatrix;
 
      for (i = 2*(lsmatsize - 1); i >= 0; i--)
      {
         sum = 0.0;
 
         for (j = 0; j < nLSpts; j++)
         {
            count = i;
            tmpstore = 1.0;
            while (count--)
               tmpstore *= j;
 
            sum += tmpstore;
         }
 
         *tmpdata++ = sum;
      }
 
/**************************
*  Scale these elements.  *
**************************/
 
      *scale = *(tmpdata - 1) / (*(tmpdata - 2));
      tmpdata = lsmatrix;
 
      for (i = 2*(lsmatsize - 1); i > 0; i--)
      {
         count = i;
         tmpstore = 1.0;
         while (count--)
            tmpstore *= (*scale);
 
         (*tmpdata++) *= tmpstore;
      }

/**************************************
*  Put the elements into a symmetric  *
*  LS data matrix.                    *
**************************************/
 
      tmpdata = lsmatrix + 2*(lsmatsize-1);     /* x0 coefficient */
      tmpresult = lsmatrix + lsmatsize*lsmatsize - 1;
                                /* last element in LS data matrix */
 
      for (i = 0; i < lsmatsize; i++)
      {
         for (j = 0; j < lsmatsize; j++)
            *tmpresult-- = *tmpdata--;
 
         tmpdata += (lsmatsize - 1);
      }

      tmpresult += 1;
      polylstrace = 0.0;

      for (i = 0; i < lsmatsize; i++)
      {
         polylstrace += *tmpresult++;
         tmpresult += lsmatsize;
      }
   }
 
/***************************************
*  Calculate the LS result vector and  *
*  scale the elements.                 *
***************************************/
 
   tmpresult = lsresult;
 
   for (i = 0; i < lsmatsize; i++)
   {
      sum = 0.0;
      savecount = lsmatsize - 1 - i;
      tmpdata = polycurve;
 
      for (j = (nLSpts - 1); j >= 0; j--)
      {
         count = savecount;
         tmpstore = 1.0;
         while (count--)
            tmpstore *= j;
 
         sum += (*tmpdata++) * tmpstore;
      }
 
      count = savecount;
      while (count--)
         sum *= (*scale);
 
      *tmpresult++ = sum;
   }

   return(polylstrace);
}


/*-----------------------------------------------
|                                               |
|               solvpolyLS()/8                  |
|                                               |
+----------------------------------------------*/
void solvpolyLS(coef, lsresult, invevals, scratch, svevaddr, msize,
			nevals, scale)
int     msize,
        nevals;
double  scale,
        *coef,
        *lsresult,
        *invevals,
        *scratch,
        **svevaddr;
{
   register int         i,
                        j;
   double               *svvecs;
   register double      ftmp,
                        *vecs,
                        *vec2,
                        *evmat,
                        **evaddr;


   vecs = svvecs = scratch;
   evaddr = svevaddr;

   for (i = 0; i < nevals; i++)
   {
      vec2 = lsresult;
      *vecs = 0.0;
      evmat = *evaddr++;

      for (j = 0; j < msize; j++)
      {
         *vecs += (*evmat) * (*vec2++);
         evmat += msize;
      }

      vecs += 1;
   }

   evmat = invevals;
   vecs = svvecs;

   for (i = 0; i < nevals; i++)
      (*vecs++) *= (*evmat++);

   evaddr = svevaddr;
   vec2 = coef;
   vecs = svvecs;

   for (i = 0; i < msize; i++)
      *vec2++ = 0.0;

   for (j = 0; j < nevals; j++)
   {
      vec2 = coef;
      evmat = *evaddr++;
      ftmp = *vecs++;

      for (i = 0; i < msize; i++)
      {
         (*vec2++) += (*evmat) * ftmp;
         evmat += msize;
      }
   }   

   vec2 = coef;
   for (i = (msize - 1); i > 0; i--)
   {
      j = i;
      while (j--)
         *vec2 *= scale;

      vec2 += 1;
   }
}


/*-----------------------------------------------
|                                               |
|               correctfid()/5                  |
|                                               |
+----------------------------------------------*/
void correctfid(data, ncdpts, polycoef, msize, datatype)
int     ncdpts,
        msize,
        datatype;
float   *data;
double  *polycoef;
{
   register int         i,
                        j;
   register float       *fiddata;
   register double      tmpdelta,
                        tmpstore,
                        calcdata,
                        *coef;


   fiddata = data;

   for (i = (ncdpts - 1); i >= 0; i--)
   {
      coef = polycoef + msize;
      tmpstore = tmpdelta = (double)i;
      calcdata = *(--coef);
 
      for (j = 1; j < msize; j++)
      {
         calcdata += (*(--coef)) * tmpstore;
         tmpstore *= tmpdelta;
      }
 
      *fiddata -= (float)calcdata;
      fiddata += datatype;
   }
}
