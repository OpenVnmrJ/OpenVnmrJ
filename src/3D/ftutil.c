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
#include "constant.h"
#include "process.h"
#include "struct3d.h"

#ifndef FT3D
#define FT3D
#endif
#include "data.h"
#undef FT3D

#include "fileio.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_SPEC_DCPTS	351
#define MIN_DEGREE	0.005

static float aver(float *data, int npts, int dtype);
extern void	Werrprintf(char *format, ...);


/*---------------------------------------
|                                       |
|             checkt2np()/2             |
|                                       |
+--------------------------------------*/
int checkt2np(nt2inc, p3Dinfo)
int		nt2inc;
proc3DInfo	*p3Dinfo;
{
   int	diffpts;


   if (nt2inc < MIN_NP)       
   {
      Werrprintf("\ncheckt2np():  insufficient t2 increments in 3D data");
      return(ERROR);
   }

   if ( p3Dinfo->f2dim.scdata.np < (p3Dinfo->f2dim.scdata.fn/COMPLEX) )
   {
      diffpts = p3Dinfo->f2dim.scdata.np - nt2inc;
      p3Dinfo->f2dim.scdata.npadj -= diffpts;
      p3Dinfo->f2dim.scdata.zfnum += COMPLEX*diffpts;
   }
   else
   {
      diffpts = (p3Dinfo->f2dim.scdata.fn/COMPLEX) - nt2inc;
      if (diffpts > 0)
      {
         p3Dinfo->f2dim.scdata.npadj -= diffpts;
         p3Dinfo->f2dim.scdata.zfnum += COMPLEX*diffpts;
      }
   }

   if (p3Dinfo->f2dim.scdata.npadj < MIN_NP)
   {
      Werrprintf("\ncheckt2np():  insufficient t2 increments for `lsfid2`");
      return(ERROR);
   }

   p3Dinfo->f2dim.scdata.np = nt2inc;
   return(COMPLETE);
}


/*---------------------------------------
|                                       |
|             datafill()/3              |
|                                       |
+--------------------------------------*/
void datafill(buffer, n, value)
int     n;		/* real number of "value's" to append	*/
float   *buffer,	/* data pointer to start of "value's"	*/
        value;		/* real number				*/
{
   register int         i;
   register float       *buffer0,
                        d;
 
   if (n < 1)
      return;

   buffer0 = buffer;
   d = value;
   i = n;
   while (i--)
      *buffer0++ = d;
}


/*---------------------------------------
|                                       |
|            datashift()/4              |
|                                       |
+--------------------------------------*/
void datashift(buffer, nadjpts, nlspts, datatype)
int	nadjpts,	/* number of "datatype" points in the data	*/
	nlspts,		/* number of "datatype" points to shift		*/
	datatype;	/* type of data, e.g., complex			*/
float	*buffer;	/* pointer to start of data			*/
{
   register int		i;
   register float	*srcpntr,
			*destpntr;


   if (nlspts == 0)
      return;

   if (nlspts > 0)
   {
      destpntr = buffer;
      srcpntr = destpntr + (datatype * nlspts);
      for (i = 0; i < (datatype * nadjpts); i++)
         *destpntr++ = *srcpntr++;
   }
   else
   {
      destpntr = buffer + (datatype * nadjpts);
      srcpntr = buffer + datatype*(nadjpts + nlspts);
      for (i = 0; i < datatype*(nadjpts + nlspts); i++)
         *(--destpntr) = *(--srcpntr);

      for (i = 0; i < (-1)*nlspts; i++)
         *(--destpntr) = 0.0;
   }
}


/*---------------------------------------
|                                       |
|            negateimag()/4             |
|                                       |
+--------------------------------------*/
void negateimag(data, nhcpts, ntype, dataskip)
int	nhcpts,
	ntype,
	dataskip;
float	*data;
{
   register int		i,
			dskip;
   register float	*tmpdata;


   if (!ntype)
      return;

   tmpdata = data;
   dskip = dataskip;

   for (i = 0; i < nhcpts; i++)
   {
      (*tmpdata) *= (-1.0);
      tmpdata += dskip;
   }
}


/*---------------------------------------
|                                       |
|            getzflevel()/2             |
|                                       |
+--------------------------------------*/
static int getzflevel(ndpts, fnsize)
int     ndpts,		/* real number of time-domain data points	*/
        fnsize;		/* real Fourier number				*/
{
   int  zflvl,
        i;

   zflvl = fnsize/(2*ndpts);
   if (zflvl > 0)
   {
      i = 2;
      while (i <= zflvl)
         i *= 2;
      zflvl = i/2;
   }
   else
   {          
      zflvl = 0;
   }

   return(zflvl);
}   
 
 
/*---------------------------------------
|                                       |
|            getzfnumber()/2            |
|                                       |
+--------------------------------------*/
static int getzfnumber(ndpts, fnsize)
int     ndpts,		/* real number of time-domain data points	*/
        fnsize;		/* real Fourier number				*/
{
   while (fnsize >= ndpts)
      fnsize /= 2;
   fnsize *= 2;
 
   return(fnsize - ndpts);      /* the ZF number */
}
 
 
/*---------------------------------------
|                                       |
|              fnpower()/1              |
|                                       |
+--------------------------------------*/
static int fnpower(fnval)
int     fnval;		/* real Fourier number	*/
{
   int  pwr = 4,
        fn = 32;
 
 
   while (fn < fnval)
   {
      fn *= 2;
      pwr++;
   }
 
   return(pwr);
}


/*---------------------------------------
|                                       |
|             phaseangle()/3           	|
|                                       |
+--------------------------------------*/
static void phaseangle2(data, nhcpts, imagskip)
int	nhcpts,
	imagskip;
float	*data;
{
   register int		i,
			iskip;
   register double	tmp1,
			tmp2;
   register float	*dpntr;

   dpntr = data;
   iskip = imagskip;

   for (i = 0; i < nhcpts; i++)
   {
      tmp1 = *dpntr;
      tmp2 = *(dpntr + iskip);
      *dpntr = (float) atan2(tmp1, tmp2);
      dpntr += HYPERCOMPLEX;
   }
}


/*---------------------------------------
|                                       |
|             abspwrval2()/4           	|
|                                       |
+--------------------------------------*/
static void abspwrval2(data, nhcpts, imagskip, dsplymode)
int	nhcpts,
	imagskip,
	dsplymode;
float	*data;
{
   register int		i,
			iskip;
   register float	tmp1,
			tmp2,
			*dpntr;


   dpntr = data;
   iskip = imagskip;

   if (dsplymode & AVMODE)
   { /* complex absolute value calculation */
      for (i = 0; i < nhcpts; i++)
      {
         tmp1 = *dpntr;
         tmp2 = *(dpntr + iskip);
         *dpntr = sqrt( tmp1*tmp1 + tmp2*tmp2 );
         dpntr += HYPERCOMPLEX;
      }
   }
   else
   { /* complex power calculation */
      for (i = 0; i < nhcpts; i++)
      {
         tmp1 = *dpntr; 
         tmp2 = *(dpntr + iskip);
         *dpntr = tmp1*tmp1 + tmp2*tmp2;
         dpntr += HYPERCOMPLEX; 
      }
   }
}


/*---------------------------------------
|                                       |
|             abspwrval4()/3           	|
|                                       |
+--------------------------------------*/
static void abspwrval4(data, nhcpts, dsplymode)
int	nhcpts,
	dsplymode;
float	*data;
{
   register int		i,
			iskip;
   register float	tmp1,
			tmp2,
			tmp3,
			tmp4,
			*dpntr;


   dpntr = data;

   if (dsplymode & AVMODE)
   { /* hypercomplex absolute value calculation */
      for (i = 0; i < nhcpts; i++)
      {
         tmp1 = *dpntr;
         tmp2 = *(dpntr + 1);
         tmp3 = *(dpntr + 2);
         tmp4 = *(dpntr + 3);
         *dpntr = sqrt( tmp1*tmp1 + tmp2*tmp2 + tmp3*tmp3 + tmp4*tmp4 );
         dpntr += HYPERCOMPLEX;
      }
   }
   else
   { /* hypercomplex power calculation */
      for (i = 0; i < nhcpts; i++)
      {
         tmp1 = *dpntr;
         tmp2 = *(dpntr + 1);
         tmp3 = *(dpntr + 2);
         tmp4 = *(dpntr + 3);
         *dpntr = tmp1*tmp1 + tmp2*tmp2 + tmp3*tmp3 + tmp4*tmp4;
         dpntr += HYPERCOMPLEX;
      }
   }
}


/*---------------------------------------
|                                       |
|             abs2pwr2()/3           	|
|                                       |
+--------------------------------------*/
static void abs2pwr2(data, nhcpts, f1dsplymode)
int	nhcpts,
	f1dsplymode;
float	*data;
{
   register int		i,
			iskip;
   register float	tmp1,
			tmp2,
			tmp3,
			tmp4,
			*dpntr;


   dpntr = data;

   if (f1dsplymode & AVMODE)
   {
      for (i = 0; i < nhcpts; i++)
      {
         tmp1 = *dpntr;
         tmp2 = *(dpntr + 1);
         tmp3 = *(dpntr + 2);
         tmp4 = *(dpntr + 3);
         tmp1 = tmp1*tmp1 + tmp2*tmp2;
         tmp3 = tmp3*tmp3 + tmp4*tmp4;
         *dpntr = sqrt( tmp1*tmp1 + tmp3*tmp3 );
         dpntr += HYPERCOMPLEX;
      }
   }
   else
   {
      for (i = 0; i < nhcpts; i++)
      {
         tmp1 = *dpntr;
         tmp2 = *(dpntr + 1);
         tmp3 = *(dpntr + 2);
         tmp4 = *(dpntr + 3);
         tmp1 = sqrt( tmp1*tmp1 + tmp2*tmp2 );
         tmp3 = sqrt( tmp3*tmp3 + tmp4*tmp4 );
         *dpntr = tmp1*tmp1 + tmp3*tmp3; 
         dpntr += HYPERCOMPLEX;
      }
   }
}


/*---------------------------------------
|                                       |
|           calc3Ddisplay()/4           |
|                                       |
+--------------------------------------*/
void calc3Ddisplay(data, nf1hcpts, f2dsply, f1dsply)
int	nf1hcpts,		/* number of F1 hypercomplex points	*/
	f2dsply,		/* type of F2 display			*/
	f1dsply;		/* type of F1 display			*/
float	*data;			/* pointer to the data			*/
{
   void	abspwrval2(),
	abspwrval4(),
	abs2pwr2();


   if ( ((f1dsply & PHMODE) || (f1dsply == 0)) &&
	((f2dsply & PHMODE) || (f2dsply == 0)) )
   {
      return;
   }

   if ((f1dsply & PHMODE) || (f1dsply == 0))
   {
      abspwrval2(data, nf1hcpts, REAL, f2dsply);
   }
   else if ((f2dsply & PHMODE) || (f2dsply == 0))
   {
      abspwrval2(data, nf1hcpts, COMPLEX, f1dsply);
   }
   else if ((f1dsply & PAMODE) || (f2dsply & PAMODE))
   {
      phaseangle2(data, nf1hcpts, COMPLEX);
   }
   else if ( (f1dsply & (AVMODE|PWRMODE)) == (f2dsply & (AVMODE|PWRMODE)) )
   {
      abspwrval4(data, nf1hcpts, f1dsply);
   }
   else
   {
      abs2pwr2(data, nf1hcpts, f1dsply);
   }
}


/*---------------------------------------
|                                       |
|             getmaxmin()/5             |
|                                       |
+--------------------------------------*/
void getmaxmin(datahead, data, npts, wrtype, datatype)
int		npts,		/* number of "datatype" points in data	*/
		wrtype,		/* type of output data after FT(t1)	*/
		datatype;	/* type of data, e.g., hypercomplex	*/
float		*data;		/* pointer to data			*/
datafileheader	*datahead;	/* pointer to the data file header	*/
{
   register int		i,
			skip,
			totalpts;
   register float	*buffer,
			tmp,
			max,
			min;


   buffer = data;
   skip = datatype/wrtype;
   totalpts = npts*wrtype;
   max = 0.0;
   min = 1.0e+6;

   for (i = 0; i < totalpts; i++)
   {
      tmp = fabs(*buffer);
      buffer += skip;
      if (tmp > max)
      {
         max = tmp;
      }
      else if (tmp < min)
      {
         min = tmp;
      }
   }

   datahead->maxval = max;
   datahead->minval = min;
}


/*---------------------------------------
|                                       |
|             vvvcmult()/4              |
|                                       |
+--------------------------------------*/
void vvvcmult(in1, in2, inc2, npoints)
int     npoints;        /* number of complex points     */
fcomplex *in1,		/* input data vector            */
	in2,		/* initial phase vector		*/
	inc2;		/* phase increment vector	*/
{
   int			incflag;
   register int         i;
   register float       tmp,
			reval,
			imval,
			reincr,
			imincr;
   register fcomplex    *input1;


   input1 = in1;
   reval = in2.re;
   imval = in2.im;
   reincr = inc2.re;
   imincr = inc2.im;
   incflag = ( (fabs(reincr) > 1e-20) || (fabs(imincr) > 1e-20) );

   for (i = 0; i < npoints; i++)
   {
      tmp = (input1->re * reval) + (input1->im * imval);
      input1->im = (input1->im * reval) - (input1->re * imval);
      input1->re = tmp;
      input1 += 1;

      if (incflag)
      {
         tmp = imval*reincr + reval*imincr;
         reval = reval*reincr - imval*imincr;
         imval = tmp;
      }
   }
}


/*---------------------------------------
|                                       |
|             vvvhcmult()/4             |
|                                       |
+--------------------------------------*/
void vvvhcmult(in1, in2, inc2, npoints)
int		npoints;        /* number of hypercomplex points	*/
fcomplex		in2,		/* initial phase vector			*/
		inc2;		/* phase increment vector		*/
hypercmplx	*in1;           /* input data vector			*/
{
   int			incflag;
   register int		i;
   register float	rere,
			reim,
			imre,
			imim,
			reval,
			imval,
			reincr,
			imincr;
   register hypercmplx	*input1;


   input1 = in1;
   reval = in2.re;
   imval = in2.im;
   reincr = inc2.re;
   imincr = inc2.im;
   incflag = ( (fabs(reincr) > 1e-20) || (fabs(imincr) > 1e-20) );

   for (i = 0; i < npoints; i++)  
   {
      rere = (input1->rere) * reval;
      imre = (input1->imre) * reval;
      reim = (input1->reim) * reval;
      imim = (input1->imim) * reval;

      rere += (input1->imre) * imval;
      imre -= (input1->rere) * imval;
      reim += (input1->imim) * imval;
      imim -= (input1->reim) * imval;

      input1->rere = rere;
      input1->reim = reim;
      input1->imre = imre;
      input1->imim = imim;

      input1 += 1;

      if (incflag)
      {
         rere = imval*reincr + reval*imincr;
         reval = reval*reincr - imval*imincr;
         imval = rere;
      }
   }
}


/*---------------------------------------
|                                       |
|            fidrotate()/5              |
|                                       |
+--------------------------------------*/
void fidrotate(data, npts, pc0, pc1, datatype)
int	npts,		/* number of "datatype" points in the data	*/
	datatype;	/* type of data, e.g., complex			*/
float	*data,		/* pointer to data				*/
	pc0,		/* zero-order FID phasing constant		*/
	pc1;		/* first-order FID phasing constant		*/
{
   
   fcomplex	phs,
		phsinc;
   void		vvvhcmult(),
		vvvcmult();


   if ( (fabs(pc0) < MIN_DEGREE) && (fabs(pc1) < 1e-20) )
      return;

   pc1 *= (-2)*M_PI/180.0;
   pc0 *= M_PI/180.0;
   phs.re = (float) cos(pc0);
   phs.im = (float) sin(pc0);
   phsinc.re = (float) cos(pc1);
   phsinc.im = (float) sin(pc1);

   if (datatype == HYPERCOMPLEX)
   {
      vvvhcmult((hypercmplx *)data, phs, phsinc, npts);
   }
   else
   {
      vvvcmult((fcomplex *)data, phs, phsinc, npts);
   }
}


/*---------------------------------------
|                                       |
|           phaserotate()/5             |
|                                       |
+--------------------------------------*/
void phaserotate(phsvector, data, npts, flag, datatype)
int     npts,		/* number of "datatype" points	*/
	flag,		/* display flag			*/
        datatype;	/* type of data			*/
float   *phsvector,	/* pointer to phasing vector	*/
        *data;		/* pointer to frequency data	*/
{
   register int         i,
                        skip;
   register float       tmp,
                        *phsr,
                        *phsi,
                        *dreal,
                        *dimag;
 
   if ( !(flag & PHMODE) && !(flag & PAMODE) )
      return;

   skip = datatype;
   phsr = phsvector;
   phsi = phsvector + 1;
   dreal = data;
   dimag = data + skip/2;
 
   for (i = 0; i < npts; i++)
   {
      tmp = (*dreal) * (*phsr) - (*dimag) * (*phsi);
      *dimag = (*dimag) * (*phsr) + (*dreal) * (*phsi);
      *dreal = tmp;
      dreal += skip;
      dimag += skip;
      phsr += COMPLEX;
      phsi += COMPLEX;
   }
}


/*---------------------------------------
|                                       |
|              fiddc()/5                |
|                                       |
+--------------------------------------*/
void fiddc(data, npts, lspts, dcflag, dtype)
int	npts,	/* number of "dtype" time-domain data points	*/
	lspts,	/* number of "dtype" left-shift data points	*/
	dcflag,	/* DC flag					*/
	dtype;	/* type of data, e.g., complex or hypercomplex	*/
float	*data;	/* pointer to time-domain data			*/
{
   register int         i,
                        j;
   register float       *pntr,
                        ftmp1,
                        ftmp2,
                        ftmp3,
                        ftmp4;


   if (!dcflag)
      return;

   pntr = data + dtype*npts;
   i = j = (npts - lspts)/16;
   ftmp1 = ftmp2 = 0.0;
 
   if (dtype == HYPERCOMPLEX)
   {
      ftmp3 = ftmp4 = 0.0;
      while (i--)
      {
         ftmp4 += *(--pntr);
         ftmp3 += *(--pntr);
         ftmp2 += *(--pntr);
         ftmp1 += *(--pntr);
      }
 
      ftmp3 /= j;
      ftmp4 /= j;
   }
   else
   {
      while (i--)
      {
         ftmp2 += *(--pntr);
         ftmp1 += *(--pntr);
      }
   }
 
   ftmp1 /= j;
   ftmp2 /= j;
   pntr = data;
   i = npts;
 
   if (lspts < 0)
   {
      pntr -= lspts;
      i -= lspts;
   }
 
   if (dtype == HYPERCOMPLEX)
   {
      while (i--)
      {   
         *pntr++ -= ftmp1;
         *pntr++ -= ftmp2;
         *pntr++ -= ftmp3;
         *pntr++ -= ftmp4;
      }
   }
   else
   {
      while (i--)
      {
         *pntr++ -= ftmp1;
         *pntr++ -= ftmp2;
      }
   }
}


/*---------------------------------------
|                                       |
|              specdc()/4               |
|                                       |
+--------------------------------------*/
void specdc(data, npts, dcflag, dtype)
int	npts,	/* number of "dtype" spectral data points	*/
	dcflag,	/* DC flag					*/
	dtype;	/* type of data, e.g., complex or hypercomplex	*/
float	*data;	/* pointer to spectral data			*/
{
   int			ndcpts,
			spacing;
   register int		i,
			j,
			datatype;
   float		avl,
			avr;
   register float	*tmpdata,
			tlt,
			lvl,
			start;
   extern float		aver();


   if (!dcflag)
      return;

   if ( (ndcpts = 1 + 2*(int) ((float) npts / 40.0)) > MAX_SPEC_DCPTS )
      ndcpts = MAX_SPEC_DCPTS;
 
   datatype = dtype;
   spacing = npts - ndcpts;
   avl = aver(data, ndcpts, dtype);
   avr = aver(data + datatype*spacing, ndcpts, datatype);
 
   tlt = (avl - avr) / (float)spacing;
   lvl = -avl - tlt * (float)(ndcpts / 2);

   for (i = 0; i < datatype; i++)
   {
      tmpdata = data + i;
      start = lvl;

      for (j = 0; j < npts; j++)
      {
         *tmpdata += start;
         start += tlt;
         tmpdata += datatype;
      }
   }
}


/*---------------------------------------
|                                       |
|               aver()/3                |
|                                       |
+--------------------------------------*/
static float aver(float *data, int npts, int dtype)
{
   register int		i,
			datatype;
   register float	*tmpdata,
			sum;


   tmpdata = data;
   datatype = dtype;
   sum = 0;

   for (i = 0; i < npts; i++)
   {
      sum += *tmpdata;
      tmpdata += datatype;
   }

   return( sum / (float)npts );
}


/*---------------------------------------
|                                       |
|             setftpar()/1              |
|                                       |
+--------------------------------------*/
int setftpar(infopntr)
proc3DInfo      *infopntr;	/* pointer to 3D information structure	*/
{
   int  	i,
		j,
		npx;
   dimenInfo	*dinfo;
   extern void	calc_digfilter(),
		set_calcfidss(),
		setlpfuncs();
 
 
   for (i = 0; i < MAXDIM; i++)
   {
      switch (i)
      {
         case 0:   dinfo = &(infopntr->f3dim); break;
         case 1:   dinfo = &(infopntr->f1dim); break;
         case 2:   dinfo = &(infopntr->f2dim); break;
         default:  break;
      }

      npx = 2*(dinfo->scdata.npadj + dinfo->scdata.lsfid); /* real points */

      if ( npx < (COMPLEX*MIN_NP) )
      {
         Werrprintf("\nsetftpar():  `np` too small for dimension %d",
			MAXDIM - i);
         return(ERROR);
      }

      if (dinfo->parLPdata.sizeLP)
      {
         setlpfuncs( &(dinfo->parLPdata) );
         if ( (dinfo->parLPdata.lppntr = (dcomplex *) malloc(
		dinfo->parLPdata.membytes)) == NULL )
         {
            Werrprintf("\nsetftpar(): cannot allocate memory for LP");
            return(ERROR);
         }

         for (j = 0; j < dinfo->parLPdata.sizeLP; j++)
            (dinfo->parLPdata.parLP + j)->lppntr = dinfo->parLPdata.lppntr;
      }
 
      dinfo->scdata.pwr = fnpower(dinfo->scdata.fn);
      dinfo->scdata.zflvl = getzflevel(2*dinfo->scdata.npadj,
				dinfo->scdata.fn);
      dinfo->scdata.zfnum = getzfnumber(2*dinfo->scdata.npadj,
				dinfo->scdata.fn);

      if (dinfo->scdata.sspar.zfsflag || dinfo->scdata.sspar.lfsflag)
      {
         if ( (dinfo->scdata.sspar.buffer = (double *) malloc(
			dinfo->scdata.sspar.membytes )) == NULL )
         {
            Werrprintf("\nsetftpar(): cannot allocate memory for SS");
            return(ERROR);
         }

         calc_digfilter(dinfo->scdata.sspar.buffer,
				dinfo->scdata.sspar.ntaps,
				dinfo->scdata.sspar.decfactor);
         set_calcfidss(TRUE);
		/*
		   must be done before the transform along each
		   dimension if ZFS is implemented for F2 and F1
		   transforms in a 3D FT.
		*/
      }
   }

 
   return(COMPLETE);
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
void rotate2_center(spdata, nelems, lpval, rpval)
int	nelems;		/* number of complex points	*/
float	*spdata;	/* pointer to spectral data	*/
double	lpval,		/* first-order phasing constant	*/
	rpval;		/* zero-order phasing constant	*/
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
