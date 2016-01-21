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
|			displayops.c				|
|								|
|     This source file contains all of the routines for		|
|     phasing, taking the absolute value, or taking the		|
|     power value of complex 1D or complex/hypercomplex		|
|     2D spectral data.						|
|								|
+--------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>
#include "group.h"
#include "pvars.h"
#include "wjunk.h"

/* VMS does not define M_PI in its math.h file */

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif 

#define SUM		0
#define SQRT		1
#define FALSE		0
#define TRUE		1
#define HYPERCOMPLEX	4
#define COMPLEX		2
#define REAL		1

#if defined (SOLARIS) || (__INTERIX)
#define logf(a) ( (float) log( (double) (a) ) )
#endif

void blockphspwr4(float *fromdata, float *todata, float *phasevector,
                  int nblocks, int blocknum, int nelems,
		  int ntraces, int imagskip, int modeflag);
void pwrval2(float *fromdata, float *todata, int nelems, int dataskip,
             int imagskip, int destskip, int zimag);

/*-----------------------------------------------
|						|
|	           rotate2()/4			|
|						|
|   This function performs a one-dimensional	|
|   phase rotation on a single 1D spectrum or	|
|   2D trace.  The phase rotation vector is	|
|   calculated on the fly.			|
|						|
+----------------------------------------------*/
/*
 * nelems number of complex points
 * spdata pointer to spectral data
 * lpval  first-order phasing constant
 * rpval  zero-order phasing constant
 */
void rotate2(float *spdata, int nelems, double lpval, double rpval)
{
   int			i;
   register float	*frompntr,
			*topntr,
			tmp1;
   double		phi,
			conphi;
   register double	cosd,
			sind,
			coold,
			siold,
			tmp2;


   conphi = M_PI/180.0;
   phi = (rpval + lpval)*conphi;
   lpval *= ( -conphi/((double) (nelems - 1)) );

   frompntr = spdata;
   topntr = spdata;
   cosd = cos(lpval);
   sind = sin(lpval);
   coold = cos(phi);
   siold = sin(phi);

   for (i = 0; i < nelems; i++)
   {
      tmp1 = (*(frompntr + 1)) * coold - (*frompntr) * siold;
      *topntr = (*frompntr++) * coold;
      (*topntr++) += (*frompntr++) * siold; 
      *topntr++ = tmp1;
      tmp2 = siold*cosd + coold*sind;
      coold = coold*cosd - siold*sind;
      siold = tmp2;
   }
}

/*-----------------------------------------------
|						|
|	           rotate2_center()/4		|
|						|
|   This function performs a one-dimensional	|
|   phase rotation on a single 1D spectrum or	|
|   2D trace.  The phase rotation vector is	|
|   calculated on the fly and lp is centered	|
|   about zero freq instead of sw/2 like	|
|   rotate2().					|
|						|
+----------------------------------------------*/
void rotate2_center(float *spdata, int nelems, double lpval, double rpval)
{
   int			i;
   register float	*frompntr,
			*topntr,
			tmp1;
   double		phi,
			conphi;
   register double	cosd,
			sind,
			coold,
			siold,
			tmp2;


   conphi = M_PI/180.0;
   phi = (rpval + lpval)*conphi/2.0;
   lpval *= ( -conphi/((double) (nelems - 1)) );

   frompntr = spdata;
   topntr = spdata;
   cosd = cos(lpval);
   sind = sin(lpval);
   coold = cos(phi);
   siold = sin(phi);

   for (i = 0; i < nelems; i++)
   {
      tmp1 = (*(frompntr + 1)) * coold - (*frompntr) * siold;
      *topntr = (*frompntr++) * coold;
      (*topntr++) += (*frompntr++) * siold; 
      *topntr++ = tmp1;
      tmp2 = siold*cosd + coold*sind;
      coold = coold*cosd - siold*sind;
      siold = tmp2;
   }
}




/*-----------------------------------------------
|						|
|	           rotate4()/9			|
|						|
|   This function performs a one-dimensional	|
|   phase rotation on a 2D hypercomplex trace	|
|   to yield the Real component for display.	|
|   The phasing vector is calculated on the	|
|   fly.					
|						|
+----------------------------------------------*/
/* nelems	 number of complex points     		*/
/* reverseflag	 F2 display = 0; F1 display = 1		*/
/* trace	 2D trace #					*/
/* nblockelems	 number of elements along the block dimension	*/
/* spdata	 pointer to un-phased, complex, spectral data	*/
/* lpval	 first-order phasing constant	(linear)	*/
/* rpval	 zero-order phasing constant	(linear)	*/
/* lp1val	 first-order phasing constant (block)		*/
/* rp1val        zero-order phasing constant  (block)		*/
void rotate4(float *spdata, int nelems, double lpval, double rpval,
             double lp1val, double rp1val, int nblockelems,
	     int trace, int reverseflag)
{
   int			i;
   float		rere,
			reim,
			imre;
   register float	*rerepntr,
			*reimpntr,
			*imrepntr,
			*imimpntr;
   double		conphi,
			phi,
			tmp,
			cosblock,
			sinblock;
   register double	siold,
			coold,
			cosd,
			sind,
			c1c2,
			c1s2,
			s1c2,
			s1s2;


   conphi = M_PI/180.0;
   phi = (rpval + lpval)*conphi;
   lpval *= ( -conphi/((double) (nelems - 1)) );
   tmp = rp1val + lp1val * ( 1.0 - trace/((double) (nblockelems - 1)) );
   tmp *= conphi;
 
   rerepntr = spdata++;
   reimpntr = spdata++;
   imrepntr = spdata++;
   imimpntr = spdata;

   cosd = cos(lpval);
   sind = sin(lpval);
   coold = cos(phi);		/* for linear direction	*/
   siold = sin(phi);		/* for linear direction	*/
   cosblock = cos(tmp);		/* for block direction	*/
   sinblock = sin(tmp);		/* for block direction	*/

   for (i = 0; i < nelems; i++)
   {
      if (reverseflag)
      {
         c1c2 = coold * cosblock;
         c1s2 = coold * sinblock;
         s1c2 = siold * cosblock;
         s1s2 = siold * sinblock;
      }
      else
      {
         c1c2 = cosblock * coold;
         c1s2 = cosblock * siold;
         s1c2 = sinblock * coold;
         s1s2 = sinblock * siold;
      }

      rere = c1c2 * (*rerepntr) + c1s2 * (*reimpntr) + s1c2 * (*imrepntr)
                  + s1s2 * (*imimpntr);
      imre = c1c2 * (*imrepntr) - s1c2 * (*rerepntr) - s1s2 * (*reimpntr)
                  + c1s2 * (*imimpntr);
      reim = c1c2 * (*reimpntr) - c1s2 * (*rerepntr) - s1s2 * (*imrepntr)
                  + s1c2 * (*imimpntr);
      *imimpntr = s1s2 * (*rerepntr) - c1s2 * (*imrepntr) -
                  	s1c2 * (*reimpntr) + c1c2 * (*imimpntr);
 
 
      *rerepntr = rere;
      *reimpntr = reim;
      *imrepntr = imre;
 
      rerepntr += 4;
      reimpntr += 4;
      imrepntr += 4;
      imimpntr += 4;

      tmp = siold*cosd + coold*sind;
      coold = coold*cosd - siold*sind;
      siold = tmp;
   }
}


/*-----------------------------------------------
|						|
|	     rotate4_center()/9			|
|						|
|   This function performs a one-dimensional	|
|   phase rotation on a 2D hypercomplex trace	|
|   to yield the Real component for display.	|
|   The phasing vector is calculated on the	|
|   fly	and lp is centered about zero frequency.|
|						|
+----------------------------------------------*/
/* nelems	 number of complex points     		*/
/* reverseflag	 F2 display = 0; F1 display = 1		*/
/* trace	 2D trace #					*/
/* nblockelems	 number of elements along the block dimension	*/
/* spdata	 pointer to un-phased, complex, spectral data	*/
/* lpval	 first-order phasing constant	(linear)	*/
/* rpval	 zero-order phasing constant	(linear)	*/
/* lp1val	 first-order phasing constant (block)		*/
/* rp1val        zero-order phasing constant  (block)		*/
void rotate4_center(float *spdata, int nelems, double lpval, double rpval,
             double lp1val, double rp1val, int nblockelems,
	     int trace, int reverseflag)
{
   int			i;
   float		rere,
			reim,
			imre;
   register float	*rerepntr,
			*reimpntr,
			*imrepntr,
			*imimpntr;
   double		conphi,
			phi,
			tmp,
			cosblock,
			sinblock;
   register double	siold,
			coold,
			cosd,
			sind,
			c1c2,
			c1s2,
			s1c2,
			s1s2;


   conphi = M_PI/180.0;
   phi = (rpval + lpval)*conphi/2.0;
   lpval *= ( -conphi/((double) (nelems - 1)) );
   tmp = rp1val + lp1val * ( 1.0 - trace/((double) (nblockelems - 1)) );
   tmp *= conphi;
 
   rerepntr = spdata++;
   reimpntr = spdata++;
   imrepntr = spdata++;
   imimpntr = spdata;

   cosd = cos(lpval);
   sind = sin(lpval);
   coold = cos(phi);		/* for linear direction	*/
   siold = sin(phi);		/* for linear direction	*/
   cosblock = cos(tmp);		/* for block direction	*/
   sinblock = sin(tmp);		/* for block direction	*/

   for (i = 0; i < nelems; i++)
   {
      if (reverseflag)
      {
         c1c2 = coold * cosblock;
         c1s2 = coold * sinblock;
         s1c2 = siold * cosblock;
         s1s2 = siold * sinblock;
      }
      else
      {
         c1c2 = cosblock * coold;
         c1s2 = cosblock * siold;
         s1c2 = sinblock * coold;
         s1s2 = sinblock * siold;
      }

      rere = c1c2 * (*rerepntr) + c1s2 * (*reimpntr) + s1c2 * (*imrepntr)
                  + s1s2 * (*imimpntr);
      imre = c1c2 * (*imrepntr) - s1c2 * (*rerepntr) - s1s2 * (*reimpntr)
                  + c1s2 * (*imimpntr);
      reim = c1c2 * (*reimpntr) - c1s2 * (*rerepntr) - s1s2 * (*imrepntr)
                  + s1c2 * (*imimpntr);
      *imimpntr = s1s2 * (*rerepntr) - c1s2 * (*imrepntr) -
                  	s1c2 * (*reimpntr) + c1c2 * (*imimpntr);
 
 
      *rerepntr = rere;
      *reimpntr = reim;
      *imrepntr = imre;
 
      rerepntr += 4;
      reimpntr += 4;
      imrepntr += 4;
      imimpntr += 4;

      tmp = siold*cosd + coold*sind;
      coold = coold*cosd - siold*sind;
      siold = tmp;
   }
}



/*-----------------------------------------------
|						|
|	        blockrotate2()/9		|
|						|
|   This function performs a one-dimensional	|
|   block phase rotation on complex 1D or 2D	|
|   data.					|
|						|
+----------------------------------------------*/
/* nelems	number of "type" data elements		*/
/* ntraces	number of spectral traces			*/
/* nblocks	number of 2D data blocks			*/
/* dataskip	2 for complex data; 4 for hypercomplex data	*/
/* imagskip	1 for F1 phasing; 2 for F2 phasing		*/
/* blocknum	block number of data in 2D data file		*/
/* modeflag	F2 phase = 0 or F1 phase = 1			*/
/* spdata	pointer to complex or hypercomplex 2D data	*/
/* phasevector	pointer to complex phase rotation vector	*/
void blockrotate2(float *spdata, float *phasevector, int nblocks, int blocknum,
          int nelems, int ntraces, int dataskip, int imagskip, int modeflag)
{
   int			nspectra,
			npnts,
			reverseflag,
			linearphase,
			block_offset;
   register int		i,
			j,
			inc1;
   register float	*datareal,
			*dataimag,
			*phasereal = NULL,
			*phaseimag = NULL,
			tmp;


   reverseflag = ( (blocknum >= nblocks) ? FALSE : TRUE );
		/*
		   reverseflag = TRUE    ==>   F1 display
		   reverseflag = FALSE   ==>   F2 display
		*/

   linearphase = (modeflag == reverseflag);
		/*
		   linearphase = TRUE    ==>   vector multiplication
		   linearphase = FALSE   ==>   block phasing
		*/

   if (reverseflag)
   {
      npnts = nelems;
      nspectra = ntraces;
      block_offset = 0;
   }    
   else
   {
      npnts = ntraces;
      nspectra = nelems;
      block_offset = nblocks;
   }

   if (!linearphase)
   {
      phasevector += 2*nspectra*(blocknum - block_offset);
      phasereal = phasevector;
      phaseimag = phasereal + 1;
   }

   inc1 = dataskip;
   datareal = spdata;
   dataimag = spdata + imagskip;

   for (i = 0; i < nspectra; i++)
   {
      if (linearphase)
      {
         phasereal = phasevector;
         phaseimag = phasereal + 1;
      }

      for (j = 0; j < npnts; j++)
      {
         tmp = (*datareal) * (*phasereal) - (*dataimag) * (*phaseimag);
         *dataimag = (*dataimag) * (*phasereal) + (*datareal) * (*phaseimag);
         *datareal = tmp;

         datareal += inc1;
         dataimag += inc1;
         if (linearphase)
         {
            phasereal += 2;
            phaseimag += 2;
         }
      }

      if (!linearphase)
      {
         phasereal += 2;
         phaseimag += 2;
      }
   }
}


/*-----------------------------------------------
|						|
|	        blockrotate4()/7		|
|						|
|   This function performs a two-dimensional	|
|   block phase rotation on hypercomplex 2D	|
|   spectral data.				|
|						|
+----------------------------------------------*/
/* nelems	number of "type" data elements		*/
/* ntraces	number of spectral traces			*/
/* nblocks	number of 2D data blocks			*/
/* blocknum	block number of data in 2D data file		*/
/* spdata	pointer to hypercomplex 2D data		*/
/* f1phsvector	pointer to F1 complex phase rotation vector	*/
/* f2phsvector	pointer to F2 complex phase rotation vector	*/
void blockrotate4(float *spdata, float *f1phsvector, float *f2phsvector,
           int nblocks, int blocknum, int nelems, int ntraces)
{
   int			nspectra,
			npnts,
			reverseflag;
   register int		i,
			j;
   float		c1,
			c2,
			s1,
			s2,
			rere,
			imre,
			reim;
   register float	*rerepntr,
			*reimpntr,
			*imrepntr,
			*imimpntr,
			*phsf1pntr,
			*phsf2pntr,
			c1c2,
			c1s2,
			s1c2,
			s1s2;


   reverseflag = ( (blocknum >= nblocks) ? FALSE : TRUE );
                /*
                   reverseflag = TRUE    ==>   F1 display
                   reverseflag = FALSE   ==>   F2 display
                */

   if (reverseflag)
   {
      nspectra = ntraces;
      npnts = nelems;
      f2phsvector += 2*nspectra*blocknum;
   }    
   else
   {
      nspectra = nelems;
      npnts = ntraces;
      f1phsvector += 2*nspectra*(blocknum - nblocks);
   }

   rerepntr = spdata++;
   reimpntr = spdata++;
   imrepntr = spdata++;
   imimpntr = spdata;
   phsf1pntr = f1phsvector;
   phsf2pntr = f2phsvector;

   for (i = 0; i < nspectra; i++)
   {
      c1 = *phsf1pntr;
      s1 = *(phsf1pntr + 1);
      c2 = *phsf2pntr;
      s2 = *(phsf2pntr + 1);
 
      for (j = 0; j < npnts; j++)
      {
         c1c2 = c1*c2;
         c1s2 = c1*s2;
         s1c2 = s1*c2;
         s1s2 = s1*s2;

         rere = c1c2 * (*rerepntr) - c1s2 * (*reimpntr) - s1c2 * (*imrepntr)
		  + s1s2 * (*imimpntr);
         imre = c1c2 * (*imrepntr) + s1c2 * (*rerepntr) - s1s2 * (*reimpntr)
		  - c1s2 * (*imimpntr);
         reim = c1c2 * (*reimpntr) + c1s2 * (*rerepntr) - s1s2 * (*imrepntr)
		  - s1c2 * (*imimpntr);
         *imimpntr = s1s2 * (*rerepntr) + c1s2 * (*imrepntr) + 
		       s1c2 * (*reimpntr) + c1c2 * (*imimpntr);


         *rerepntr = rere;
         *reimpntr = reim;
         *imrepntr = imre;

         rerepntr += 4;
         reimpntr += 4;
         imrepntr += 4;
         imimpntr += 4;

         if (reverseflag)
         {
            phsf1pntr += 2;
            c1 = *phsf1pntr;
            s1 = *(phsf1pntr + 1);
         }
         else
         {
            phsf2pntr += 2;
            c2 = *phsf2pntr;
            s2 = *(phsf2pntr + 1);
         }
      }

      if (reverseflag)
      {
         phsf2pntr += 2;
         phsf1pntr = f1phsvector;
      }
      else
      {
         phsf1pntr += 2;
         phsf2pntr = f2phsvector;
      }
   }
}


/*-----------------------------------------------
|						|
|		   phase2()/5			|
|						|
|   This function performs a one-dimensional	|
|   phasing operation on either a 1D spectrum	|
|   or a 2D trace to yield the Real component	|
|   for display.  The phasing vector is cal-	|
|   culated on the fly.  "fromdata" can be	|
|   the same as "todata" in this routine.	|
|						|
+----------------------------------------------*/
void phase2(float *fromdata, float *todata, int nelems, double lpval, double rpval)
{
   int			i;
   register float	*frompntr,
			*topntr;
   double		conphi,
			phi,
			tmp;
   register double	siold,
			coold,
			cosd,
			sind;


   conphi = M_PI/180.0;
   phi = (rpval + lpval)*conphi;
   lpval *= ( -conphi/((double) (nelems - 1)) );

   frompntr = fromdata;
   topntr = todata;
   cosd = cos(lpval);
   sind = sin(lpval);
   coold = cos(phi);
   siold = sin(phi);

   for (i = 0; i < nelems; i++)
   {
      *topntr = (*frompntr++) * coold;
      (*topntr++) +=  (*frompntr++) * siold; /* re=xcosp+ysinp */
      tmp = siold*cosd + coold*sind;
      coold = coold*cosd - siold*sind;
      siold = tmp;
   }
}


/*-----------------------------------------------
|						|
|		   phase4()/10			|
|						|
|   This function performs a one-dimensional	|
|   phasing operation on a 2D hypercomplex	|
|   trace to yield the Real component for	|
|   display.  The phasing vector is calculated	|
|   on the fly.  "fromdata" can be the same	|
|   as "todata" in this routine.		|
|						|
+----------------------------------------------*/
/* nelems	number of complex points     		*/
/* reverseflag	F2 display = 0; F1 display = 1		*/
/* trace	2D trace #					*/
/* nblockelems	number of elements along the block dimension	*/
/* fromdata	pointer to unphased, complex spectral data	*/
/* todata	pointer to phased, real spectral data	*/
/* lpval	first-order phasing constant	(linear)	*/
/* rpval	zero-order phasing constant	(linear)	*/
/* lp1val	first-order phasing constant (block)		*/
/* rp1val	zero-order phasing constant  (block)		*/
void phase4(float *fromdata, float *todata, int nelems, double lpval, double rpval,
        double lp1val, double rp1val, int nblockelems, int trace, int reverseflag)
{
   int			i;
   register float	*frompntr,
			*topntr;
   double		conphi,
			phi,
			tmp;
   register double	siold,
			coold,
			cosd,
			sind,
			cosblock,
			sinblock;


   conphi = M_PI/180.0;
   phi = (rpval + lpval)*conphi;
   lpval *= ( -conphi/((double) (nelems - 1)) );
   tmp = rp1val + lp1val * ( 1.0 - trace/((double) (nblockelems - 1)) );
   tmp *= conphi;
 
   frompntr = fromdata;
   topntr = todata;
   cosd = cos(lpval);
   sind = sin(lpval);
   coold = cos(phi);		/* for linear direction	*/
   siold = sin(phi);		/* for linear direction	*/
   cosblock = cos(tmp);		/* for block direction	*/
   sinblock = sin(tmp);		/* for block direction	*/

   if (reverseflag)
   {
      for (i = 0; i < nelems; i++)
      {
         *topntr = (*frompntr++) * coold * cosblock;
         *topntr += (*frompntr++) * coold * sinblock;
         *topntr += (*frompntr++) * siold * cosblock;
         (*topntr++) += (*frompntr++) * siold * sinblock;
         tmp = siold*cosd + coold*sind;
         coold = coold*cosd - siold*sind;
         siold = tmp;
      }
   }
   else
   {
      for (i = 0; i < nelems; i++)
      {
         *topntr = (*frompntr++) * coold * cosblock;
         *topntr += (*frompntr++) * siold * cosblock;
         *topntr += (*frompntr++) * coold * sinblock;
         (*topntr++) += (*frompntr++) * siold * sinblock;
         tmp = siold*cosd + coold*sind;
         coold = coold*cosd - siold*sind;
         siold = tmp;
      }
   }
}


/*-----------------------------------------------
|						|
|	        blockphase2()/11		|
|						|
|   This function performs a one-dimensional	|
|   phasing operation on 2D complex spectral	|
|   data to yield the ReRe component for	|
|   display.  "fromdata" can be the same as	|
|   "todata" in this routine.			|
|						|
+----------------------------------------------*/
/* nelems	number of complex elements			*/
/* ntraces	number of spectral traces			*/
/* nblocks	number of 2D data blocks			*/
/* dataskip	2 for complex data; 4 for hypercomplex data	*/
/* imagskip	1 for F1 phasing; 2 for F2 phasing		*/
/* blocknum	block number of data in 2D data file		*/
/* modeflag	F2 phase = 0 or F1 phase = 1			*/
/* zimag	flag for zeroing imaginary part of "todata"	*/
/* fromdata	pointer to unphased, complex spectral data	*/
/* todata	pointer to phased, real spectral data	*/
/* phasevector	pointer to complex phase rotation vector	*/
void blockphase2(float *fromdata, float *todata, float *phasevector, int nblocks,
       int blocknum, int nelems, int ntraces, int dataskip, int imagskip,
       int modeflag, int zimag)
{
  int			nspectra,
			npnts,
			reverseflag,
			linearphase,
			block_offset;
   register int		i,
			j,
			tonextreal;
   register float	*srcpntr,
			*destpntr,
			*phspntr = NULL;


   reverseflag = ( (blocknum >= nblocks) ? FALSE : TRUE );
                /*
                   reverseflag = TRUE    ==>   F1 display
                   reverseflag = FALSE   ==>   F2 display
                */

   linearphase = (modeflag == reverseflag);
                /*
                   linearphase = TRUE    ==>   vector multiplication
                   linearphase = FALSE   ==>   block phasing
                */

   if (reverseflag)
   {
      npnts = nelems;
      nspectra = ntraces;
      block_offset = 0;
   }
   else
   {
      npnts = ntraces;
      nspectra = nelems;
      block_offset = nblocks;
   }

   if (!linearphase)
   {
      phasevector += 2*nspectra*(blocknum - block_offset);
      phspntr = phasevector;
   }

   tonextreal = dataskip - imagskip;	/* > 0 */
   srcpntr = fromdata;
   destpntr = todata;

   for (i = 0; i < nspectra; i++)
   {
      if (linearphase)
         phspntr = phasevector;

      for (j = 0; j < npnts; j++)
      {
         *destpntr = (*srcpntr) * (*phspntr);
         srcpntr += imagskip;
         (*destpntr++) -= (*srcpntr) * (*(phspntr + 1)); /* re=xcosp-ysinp */
         srcpntr += tonextreal;
         if (zimag)
            *destpntr++ = 0.0;
         if (linearphase)
            phspntr += 2;
      }

      if (!linearphase)
         phspntr += 2;
   }
}

static void pa_level_calc(float *pa_level, float *pa_pntr, int npnts, int nspectra,
          int imagskip, int dataskip )
{
    int i, ntot;
    double dtmp;
    float *tmppntr, pa_levelmax, avval;
    if ( P_getreal(CURRENT, "palvl", &dtmp, 1) != 0 )
      dtmp = 0.0;
    if (dtmp == 0.0)
    {
       *pa_level = 0.0;
        return;
    }
    if (dtmp < 0)
    {
      dtmp = 0.0;
      Winfoprintf("palvl must be greater than zero, using default of 0.0");
    }
    else if (dtmp > 1)
    {
      dtmp = 0.0;
      Winfoprintf("palvl must be less than one, using default of 0.0");
    }
    pa_levelmax = 0;
    tmppntr = pa_pntr;
    ntot = npnts * nspectra;
    for (i=0; i<ntot; i++)
    {
      avval = sqrt((*tmppntr * *tmppntr) + (*(tmppntr+imagskip) * *(tmppntr+imagskip)) );
      if (avval > pa_levelmax)
      {
        pa_levelmax = avval;
      }
      tmppntr += dataskip;
    }
    *pa_level = ((float)dtmp) * (pa_levelmax);
}



/*-----------------------------------------------
|						|
|	        blockphaseangle2()/11		|
|						|
|   This function performs a one-dimensional	|
|   phasing operation on 2D complex spectral	|
|   data to yield the ReRe component for	|
|   display, then performs atan2() on x,y pairs.|
|   "fromdata" can be the same as "todata" 	|
|   in this routine.				|
|						|
+----------------------------------------------*/
/* nelems	number of complex elements			*/
/* ntraces	number of spectral traces			*/
/* nblocks	number of 2D data blocks			*/
/* dataskip	2 for complex data; 4 for hypercomplex data	*/
/* imagskip	1 for F1 phasing; 2 for F2 phasing		*/
/* blocknum	block number of data in 2D data file		*/
/* modeflag	F2 phase = 0 or F1 phase = 1			*/
/* zimag	flag for zeroing imaginary part of "todata"	*/
/* fromdata	pointer to unphased, complex spectral data	*/
/* todata	pointer to phased, real spectral data	*/
/* phasevector	pointer to complex phase rotation vector	*/
void blockphaseangle2(float *fromdata, float *todata, float *phasevector,
         int nblocks, int blocknum, int nelems, int ntraces, int dataskip,
         int imagskip, int modeflag, int zimag)
{
   int			nspectra,
			npnts,
			reverseflag,
			linearphase,
			block_offset;
   float		pa_level;
   register int		i,
			j;
   register float	*srcpntr,
			*destpntr,
			*phspntr = NULL;
   register double	re2,
			im2;


   reverseflag = ( (blocknum >= nblocks) ? FALSE : TRUE );
                /*
                   reverseflag = TRUE    ==>   F1 display
                   reverseflag = FALSE   ==>   F2 display
                */

   linearphase = (modeflag == reverseflag);
                /*
                   linearphase = TRUE    ==>   vector multiplication
                   linearphase = FALSE   ==>   block phasing
                */

   if (reverseflag)
   {
      npnts = nelems;
      nspectra = ntraces;
      block_offset = 0;
   }
   else
   {
      npnts = ntraces;
      nspectra = nelems;
      block_offset = nblocks;
   }

   if (!linearphase)
   {
      phasevector += 2*nspectra*(blocknum - block_offset);
      phspntr = phasevector;
   }

   srcpntr = fromdata;
   destpntr = todata;

   pa_level_calc( &pa_level, srcpntr, npnts, nspectra, imagskip, dataskip );
   srcpntr = fromdata;

   for (i = 0; i < nspectra; i++)
   {
      if (linearphase)
         phspntr = phasevector;

      for (j = 0; j < npnts; j++)
      {
	 re2 = (double) ((*srcpntr) * (*phspntr) - (*(srcpntr+imagskip)) * (*(phspntr + 1)));
	 im2 = (double) ((*srcpntr) * (*(phspntr+1)) + (*(srcpntr+imagskip)) * (*phspntr));
	 if (pa_level == 0.0)
	    *destpntr++ = (float) atan2(re2,im2);
	 else if (sqrt(re2*re2 + im2*im2) < pa_level)
	    *destpntr++ = 0.0;
	 else
	    *destpntr++ = (float) atan2(re2,im2);
         srcpntr += dataskip;
         if (zimag)
            *destpntr++ = 0.0;
         if (linearphase)
            phspntr += 2;
      }

      if (!linearphase)
         phspntr += 2;
   }
}

/*-----------------------------------------------
|						|
|		 blockphase4()/8		|
|						|
|   This function performs a two-dimensional	|
|   phasing operation on 2D hypercomplex	|
|   spectral data to yield the ReRe component	|
|   for display.  "fromdata" can be the same	|
|   as "todata" in this routine.		|
|						|
+----------------------------------------------*/
/* nelems	number of hypercomplex points		*/
/* ntraces	number of spectral traces			*/
/* nblocks	number of 2D data blocks			*/
/* blocknum	block number of data in 2D data file		*/
/* fromdata	pointer to unphased, hypercomplex data	*/
/* todata	pointer to phased, real spectral data	*/
/* f1phsvector	pointer to F1 complex phase rotation vector	*/
/* f2phsvector	pointer to F2 complex phase rotation vector	*/
void blockphase4(float *fromdata, float *todata, float *f1phsvector, float *f2phsvector,
           int nblocks, int blocknum, int nelems, int ntraces)
{
   int			nspectra,
			npnts,
			reverseflag;
   register int		i,
			j;
   register float	*srcpntr,
			*destpntr,
			*phsf1pntr,
			*phsf2pntr,
			c1,
			c2,
			s1,
			s2;


   reverseflag = ( (blocknum >= nblocks) ? FALSE : TRUE );
                /*
                   reverseflag = TRUE    ==>   F1 display
                   reverseflag = FALSE   ==>   F2 display
                */

   if (reverseflag)
   {
      nspectra = ntraces;
      npnts = nelems;
      f2phsvector += 2*nspectra*blocknum;
   }
   else
   {
      nspectra = nelems;
      npnts = ntraces;
      f1phsvector += 2*nspectra*(blocknum - nblocks);
   }

   srcpntr = fromdata;
   destpntr = todata;
   phsf1pntr = f1phsvector;
   phsf2pntr = f2phsvector;

   for (i = 0; i < nspectra; i++)
   {
      c1 = *phsf1pntr;
      s1 = *(phsf1pntr + 1);
      c2 = *phsf2pntr;
      s2 = *(phsf2pntr + 1);

      for (j = 0; j < npnts; j++)
      {
         *destpntr = c1 * c2 * (*srcpntr++);
         *destpntr -= c1 * s2 * (*srcpntr++);
         *destpntr -= s1 * c2 * (*srcpntr++);
         (*destpntr++) += s1 * s2 * (*srcpntr++);

         if (reverseflag)
         {
            phsf1pntr += 2;
            c1 = *phsf1pntr;
            s1 = *(phsf1pntr + 1);
         }
         else
         {
            phsf2pntr += 2;
            c2 = *phsf2pntr;
            s2 = *(phsf2pntr + 1);
         }
      }

      if (reverseflag)
      {
         phsf2pntr += 2;
         phsf1pntr = f1phsvector;
      }
      else
      {
         phsf1pntr += 2;
         phsf2pntr = f2phsvector;
      }
   }
}


/*-----------------------------------------------
|						|
|		 phasefunc()/4			|
|						|
|   This function calculates a complex phasing	|
|   vector based upon the number of complex or	|
|   hypercomplex data points, the first-order	|
|   phasing constant (lp), and the zero-order	|
|   phasing constant (rp).			|
|						|
+----------------------------------------------*/
void phasefunc(float *phasepntr, int npnts, double lpval, double rpval)
{
   register int		i;
   register float	*phasedata;
   double 		phi,
			conphi,
			tmp;
   register double	siold,
			coold,
			cosd,
			sind;


   conphi = M_PI/180.0;
   phi = (rpval + lpval)*conphi;
   if (npnts > 1)
   {
      lpval *= ( -conphi/((double) (npnts - 1)) );
   }
   else
   {
      lpval = 0.0;
   }

   phasedata = phasepntr;
   cosd = cos(lpval);
   sind = sin(lpval);
   coold = cos(phi);
   siold = sin(phi);

   for (i = 0; i < npnts; i++)
   {
      *phasedata++ = (float)coold;
      *phasedata++ = (-1) * (float)siold;
      tmp = siold*cosd + coold*sind;
      coold = coold*cosd - siold*sind;
      siold = tmp;
   }
}


/*-----------------------------------------------
|						|
|		blockphsabs4()/9		|
|						|
|   This function performs a two-dimensional	|
|   phasing/absolute value operation on 2D	|
|   hypercomplex spectral data to yield the	|
|   ReS component for display.  "fromdata"	|
|   cannot be the same as "todata" in this	|
|   routine.					|
|						|
+----------------------------------------------*/
void blockphsabs4(float *fromdata, float *todata, float *phasevector,
        int nblocks, int blocknum, int nelems, int ntraces, int imagskip, int modeflag)
{
   register int		i;
   register float	*destpntr;


   blockphspwr4(fromdata, todata, phasevector, nblocks, blocknum, nelems,
		  ntraces, imagskip, modeflag);

   destpntr = todata;
   for (i = 0; i < (nelems*ntraces); i++)
   {
      *destpntr = sqrt(*destpntr);
      destpntr++;
   }
}


/*-----------------------------------------------
|                                               |
|               blockphspwr4()/9		|
|                                               |
|   This function performs a two-dimensional    |
|   phasing/power operation on 2D hypercomplex	|
|   spectral data to yield the ReS component	|
|   for display.  "fromdata" cannot be the	|
|   same as "todata" in this routine.		|
|						|
+----------------------------------------------*/
/* int	nblocks,	number of 2D data blocks			*/
/* 	blocknum,	block number of data in 2D data file		*/
/* 	nelems,		number of hypercomplex elements		*/
/* 	ntraces,	number of spectral traces			*/
/* 	imagskip,	1 for F1 phasing; 2 for F2 phasing		*/
/* 	modeflag;	F2 phase = 0 or F1 phase = 1			*/
/* float *fromdata,	pointer to raw, hypercomplex spectral data	*/
/* 	*todata,	pointer to real spectral data		*/
/* 	*phasevector;	pointer to complex phase rotation vector	*/
void blockphspwr4(float *fromdata, float *todata, float *phasevector,
                  int nblocks, int blocknum, int nelems,
		  int ntraces, int imagskip, int modeflag)
{
   int			nspectra,
			npnts,
			reverseflag,
			linearphase,
			block_offset;
   register int		tonextpair,
			tonextreal,
			imaginc,
			i,
			j;
   register float	*srcpntr,
			*destpntr,
			*phspntr = NULL,
			tmp1,
			tmp2;


   reverseflag = ( (blocknum >= nblocks) ? FALSE : TRUE );
		/*
		   reverseflag = TRUE	 ==>	F1 display
		   reverseflag = FALSE	 ==>	F2 display
		*/

   linearphase = (modeflag == reverseflag);
		/*
		   linearphase = TRUE	 ==>	vector multiplication
		   linearphase = FALSE	 ==>	block phasing
		*/

   if (reverseflag)
   {
      npnts = nelems;
      nspectra = ntraces;
      block_offset = 0;
   }
   else
   {
      npnts = ntraces;
      nspectra = nelems;
      block_offset = nblocks;
   }

   if (!linearphase)
   {
      phasevector += 2*nspectra*(blocknum - block_offset);
      phspntr = phasevector;
   }

   tonextpair = ( (modeflag) ? REAL : COMPLEX );
   tonextreal = HYPERCOMPLEX - tonextpair;
   imaginc = imagskip;
   srcpntr = fromdata;
   destpntr = todata;


   for (i = 0; i < nspectra; i++)
   {
      if (linearphase)
         phspntr = phasevector;

      for (j = 0; j < npnts; j++)
      {
         tmp1 = (*srcpntr) * (*phspntr) -
			(*(srcpntr + imaginc)) * (*(phspntr + 1));
         srcpntr += tonextpair;
         tmp2 = (*srcpntr) * (*phspntr) -
			(*(srcpntr + imaginc)) * (*(phspntr + 1));
         *destpntr++ = tmp1*tmp1 + tmp2*tmp2;
         srcpntr += tonextreal;
         if (linearphase)
            phspntr += 2;
      }

      if (!linearphase)
         phspntr += 2;
   }
}


/*-----------------------------------------------
|						|
|	          absval2()/7			|
|						|
|   This function computes the one-dimensional	|
|   absolute value spectrum from either a com-	|
|   plex or hypercomplex data set.		|
|						|
+----------------------------------------------*/
/* dataskip	2 for complex data; 4 for hypercomplex data	*/
/* imagskip	1 for F1 absval; 2 for F2 absval		*/
/* destskip	skip for destination pointer			*/
/* nelems	number of complex points			*/
/* zimag	flag for zeroing imaginary part of "todata"	*/
/* fromdata	pointer to source data			*/
/* todata	pointer to destination data			*/
void absval2(float *fromdata, float *todata, int nelems, int dataskip, int imagskip,
		int destskip, int zimag)
{
   register int		i,
			skip;
   register float	*destpntr;


   pwrval2(fromdata, todata, nelems, dataskip, imagskip,
		destskip, zimag);

   skip = destskip;
   destpntr = todata;
   for (i = 0; i < nelems; i++)
   {
      *destpntr = sqrt(*destpntr);
      destpntr += skip;
   }
}

/*-----------------------------------------------
|						|
|		   absval4()/3			|
|						|
|   This function computes the absolute value	|
|   spectrum from a 2D hypercomplex data set.	|
|						|
|	S  =  sqrt(RR**2 + RI**2 + IR**2 +	|
|			II**2)			|
|						|
+----------------------------------------------*/
void absval4(float *fromdata, float *todata, int nelems)
{
   register int		i;
   register float	*srcpntr,
			*destpntr,
			rere2,
			reim2,
			imre2,
			imim2;


   srcpntr = fromdata;
   destpntr = todata;

   for (i = 0; i < nelems; i++)
   {
      rere2 = *srcpntr++;
      rere2 *= rere2;
      imre2 = *srcpntr++;
      imre2 *= imre2;
      reim2 = *srcpntr++;
      reim2 *= reim2;
      imim2 = *srcpntr++;
      imim2 *= imim2;
      *destpntr++ = rere2 + reim2 + imre2 + imim2;
   }

   destpntr = todata;
   for (i = 0; i < nelems; i++)
   {
      rere2 = sqrt(*destpntr);
      *destpntr++ = rere2;
   }
}

/*-----------------------------------------------
|						|
|	          dbmval2()/7			|
|						|
|   This function computes the one-dimensional	|
|   power value spectrum in dbm set so that 0 dbm
|   has a value of 140  Data may be from either a com-	|
|   plex or hypercomplex data set.		|
|						|
+----------------------------------------------*/
void dbmval2(float *fromdata, float *todata, int nelems, int dataskip, int imagskip,
		int destskip, int zimag)
{
   register int		i,
			skip;
   register float	*destpntr;
   float thispoint,  mymax, cmax, np2, tmp;

   mymax = 0.0;
   cmax = -20.0;
   np2 = (float) nelems*nelems;

   pwrval2(fromdata, todata, nelems, dataskip, imagskip,
		destskip, zimag);

   skip = destskip;
   destpntr = todata;

   for (i = 0; i < nelems; i++)
   { 
	  thispoint = *destpntr/np2;
	  if ( thispoint > mymax) mymax = thispoint;
	  if (thispoint < 1.0e-26)
	       tmp = 2;
	  else
	       tmp = 4.3429448*logf(thispoint)+171.0; /* logf is ln! */
	  if (tmp > cmax) cmax = tmp;
      *destpntr = tmp;
      destpntr += skip;
   }
}

/*-----------------------------------------------
|						|
|	           pwrval2()/7			|
|						|
|   This function computes the one-dimensional	|
|   power spectrum from either a complex or	|
|   hypercomplex data set.			|
|						|
+----------------------------------------------*/
/* int	dataskip,	2 for complex data; 4 for hypercomplex data	*/
/*     imagskip,	1 for F1 pwrval; 2 for F2 pwrval		*/
/* 	destskip,	skip for destination pointer			*/
/* 	nelems,		number of complex points			*/
/* 	zimag;		flag for zeroing imaginary part of "todata"	*/
/* float *fromdata,	pointer to source data			*/
/* 	*todata;	pointer to destination data			*/
void pwrval2(float *fromdata, float *todata, int nelems, int dataskip,
             int imagskip, int destskip, int zimag)
{
   register int		i,
			tonextreal;
   register float	*srcpntr,
			*destpntr,
			re2,
			im2;

   tonextreal = dataskip - imagskip;	/* > 0 */
   srcpntr = fromdata;
   destpntr = todata;

   for (i = 0; i < nelems; i++)
   {
      re2 = *srcpntr;
      re2 *= re2;
      srcpntr += imagskip;
      im2 = *srcpntr;
      im2 *= im2;
      srcpntr += tonextreal;
      *destpntr = re2 + im2;

      if (zimag)
         *(destpntr + 1) = 0.0;

      destpntr += destskip;
   }
}

/*-----------------------------------------------
|						|
|	           phaseangle2()/9		|
|						|
|   This function computes the one-dimensional	|
|   power spectrum from either a complex or	|
|   hypercomplex data set.			|
|						|
+----------------------------------------------*/
void phaseangle2(float *fromdata, float *todata, int nelems, int dataskip, int imagskip,
		int destskip, int zimag, double lpval, double rpval)
{
   register int		i;
   register float	*srcpntr,
			*destpntr,
			re2,
			im2;
   float		pa_level;
   double		conphi = M_PI / 180.0;
   double		phi, coold, siold, cosd, sind, tmp;

   phi = (rpval + lpval) * conphi;
   lpval *= ( -conphi / ((double)(nelems-1)));
   srcpntr = fromdata;
   destpntr = todata;

   pa_level_calc( &pa_level, srcpntr, nelems, 1, imagskip, dataskip );
   srcpntr = fromdata;

   cosd = cos(lpval);
   sind = sin(lpval);
   coold = cos(phi);
   siold = sin(phi);
   for (i = 0; i < nelems; i++)
   {
      re2 = (double) (*(srcpntr) * coold + *(srcpntr + imagskip) * siold);
      im2 = (double) (-(*(srcpntr) * siold) + *(srcpntr + imagskip) * coold);
      srcpntr += dataskip;
      if (pa_level == 0.0)
         *destpntr = (float) atan2(re2,im2);
      else if (sqrt(re2*re2+im2*im2) < pa_level)
         *destpntr = 0.0;
      else
         *destpntr = (float) atan2(re2,im2);
      if (zimag)
         *(destpntr + 1) = 0.0;
      destpntr += destskip;
      tmp = siold*cosd + coold*sind;
      coold = coold*cosd - siold*sind;
      siold = tmp;
   }
/*   destpntr = todata;
   unwrap_phaseangle2(todata,nelems,nelems/2); */
}


/*-----------------------------------------------
|						|
|	          pwrval4()/3			|
|						|
|   This function computes the power spectrum	|
|   from a 2D hypercomplex data set.		|
|						|
|	  R(F1)  =  RR**2  +  RI**2		|
|	  I(F1)  =  IR**2  +  II**2		|
|						|
|	  R(F2)  =  RR**2  +  IR**2		|
|	  I(F2)  =  RI**2  +  II**2		|
|						|
|	  S  =  0.5 * [R(F1)**2 + I(F1)**2	|
|		  + R(F2)**2 + I(F2)**2]	|
|						|
|	     =  (RR**2 + II**2)(RI**2 + IR**2)	|
|		+ RR**4 + RI**4 + IR**4 +	|
|		II**4				|
|						|
+----------------------------------------------*/
void pwrval4(float *fromdata, float *todata, int nelems)
{
   register int		i;
   register float	*srcpntr,
			*destpntr,
			rere2,
			reim2,
			imre2,
			imim2;


   srcpntr = fromdata;
   destpntr = todata;

   for (i = 0; i < nelems; i++)
   {
      rere2 = *srcpntr++;
      rere2 *= rere2;
      imre2 = *srcpntr++;
      imre2 *= imre2;
      reim2 = *srcpntr++;
      reim2 *= reim2;
      imim2 = *srcpntr++;
      imim2 *= imim2;
      *destpntr = (rere2 + imim2) * (reim2 + imre2);
      rere2 *= rere2;
      reim2 *= reim2;
      imre2 *= imre2;
      imim2 *= imim2;
      (*destpntr++) += rere2 + reim2 + imre2 + imim2;
   }
}


/*-----------------------------------------------
|						|
|	        blockpwrabs()/4			|
|						|
|   This function performs a mixed-mode (power	|
|   and absolute value) calculation on 2D	|
|   hypercomplex spectra data.			|
|						|
|						|
|	AV1 PWR:				|
|						|
|	    R(F1)  =  RR**2  +  RI**2		|
|	    I(F1)  =  IR**2  +  II**2		|
|						|
|	    S  =  sqrt[R(F1)**2 + I(F1)**2]	|
|						|
|						|
|	PWR1 AV:				|
|						|
|	    R(F2)  =  RR**2  +  IR**2		|
|	    I(F2)  =  RI**2  +  II**2		|
|						|
|	    S  =  sqrt[R(F2)**2 + I(F2)**2]	|
|						|
+----------------------------------------------*/
void blockpwrabs(float *fromdata, float *todata, int nelems, int pwrskip)
{
   register int		i,
			tonextpair,
			tonextreal,
			pwrinc;
   register float	*srcpntr,
			*destpntr,
			tmp1,
			tmp2;


   srcpntr = fromdata;
   destpntr = todata;
   pwrinc = pwrskip;
   tonextpair = HYPERCOMPLEX - 1 - pwrskip;
   tonextreal = HYPERCOMPLEX - tonextpair;

   for (i = 0; i < nelems; i++)
   {
      tmp1 = (*(srcpntr + pwrinc));
      tmp1 *= tmp1;
      tmp1 += (*srcpntr) * (*srcpntr);
      srcpntr += tonextpair;
      tmp2 = (*(srcpntr + pwrinc));
      tmp2 *= tmp2;    
      tmp2 += (*srcpntr) * (*srcpntr);
      *destpntr++ = sqrt(tmp1*tmp1 + tmp2*tmp2);
      srcpntr += tonextreal;
   }
}
