/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-------------------------------------------------------
|							|
|	  bc   -    1D and 2D baseline correction	|
|	bc2d   -    2D baseline correction		|
|							|
+------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <math.h>

/*  VMS does not define M_PI in its math.h file */

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif 

#include "vnmrsys.h"
#include "data.h"
#include "init2d.h"
#include "group.h"
#include "tools.h"
#include "allocate.h"
#include "variables.h"
#include "pvars.h"
#include "wjunk.h"
#include "aipCInterface.h"

extern void Wturnoff_buttons();
extern int currentindex();
extern void rotate2(float *spdata, int nelems, double lpval, double rpval);
extern int removephasefile();
extern int datapoint(double freq, double sw, int fn);
extern void phasefunc(float *phasepntr, int npnts, double lpval, double rpval);
extern void rotate4(float *spdata, int nelems, double lpval, double rpval,
             double lp1val, double rp1val, int nblockelems,
             int trace, int reverseflag);


/*************************
*  Constant definitions  *
*************************/

#define CLVL            10000.0
#define COMPLETE 	0
#define ERROR 		1
#define FALSE           0
#define TRUE            1
#define MAXPTS          400
#define MAXORDER        20
#define RANGE           1.0
#define MINDEGREE	0.005

/**********************
*  Macro definitions  *
**********************/

#define C_GETPAR(name, value)					\
	if ( (r = P_getreal(CURRENT, name, value, 1)) )		\
	{ P_err(r, name, ":"); return(ERROR); }

#define getdatatype(status)					\
  	( (status & S_HYPERCOMPLEX) ? HYPERCOMPLEX :		\
    	( (status & S_COMPLEX) ? COMPLEX : REAL ) )

/*******************************
*  Definitinos to support the  *
*  debugging options           *
*******************************/

#ifdef  DEBUG
#define DBUG
#endif 

#ifdef  DBUG
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
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5) \
	if (debug1) Wscrprintf(str,arg1,arg2,arg3,arg4,arg5)
#else 
#define DPRINT(str) 
#define DPRINT1(str, arg2) 
#define DPRINT2(str, arg1, arg2) 
#define DPRINT3(str, arg1, arg2, arg3) 
#define DPRINT4(str, arg1, arg2, arg3, arg4) 
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5) 
#endif 


/*********************************************
*  Declaration of EXTERNAL GLOBAL variables  *
*********************************************/

extern int	interuption,
		Bnmr;
extern void data2d_rotate(float *datapntr, int f1phas, float *f1buffer,
                   int f2phas, float *f2buffer, int total_blocks,
                   int block_num, int nf1pnts, int nf2pnts, int hyperflag);

/*********************************************
*  Declaration of INTERNAL GLOBAL variables  *
*********************************************/

char	newtrace[5],
	oldtrace[5];
int	sections,
	bc2dflag,
	unbcflag;
double	*xval,
	*xpos,
	*yval;
FILE	*textfile;

struct base
{
   int    fpt;
   int    npt;
   int    bcpt;
   double bcincr;
};

// baseline buffer for current trace, i.e., bctrace 
float * bcdata = NULL;
int bctrace = 0;
int bcnpts = 0;

void bc_apply(char *key);
void bc_all(int altmode, int order, int numpts, int scale, float smooth, struct base  baseline[]);
void dis_bcdata(int altmode, int order, int numpts, int scale, float smooth, struct base  baseline[]);
void setS_BCbit(int fileindex, int b);
void clearBcdata();
void initBCparam(int *altmode, int *scale, float *smooth, int *order);
void writeFDFSpecfile(char *path, char *dataType, float *data, int numtraces, int datasize);

static float Sum(float *data, int startIndex, int endIndex)
{
   float sum = 0;
   int i;

   for (i = startIndex; i <= endIndex; i++)
   {
      sum = sum + data[i];
   }
   return(sum);
}
         

/*
    Function to calculate the first derivative of a noisy vector using a
    wavelet transform. Uses the Haar function as a basis. The scale parameter
    "ascale" is provided which is the number of points over which averaging
    "n" is the length of the input vector.
 */
static float *FirstDerivative(float *data, int n, int aScale)
{
   int nav = 0;
   int k;
   float *derivativeData;

   derivativeData = (float *) allocateWithId( n * sizeof(float), "bc3");
   for (k = 1; k < n; k++)
   {
      if ((k+1) < aScale + 1)
         nav = k;
      else if ((k+1) > (n - aScale))
         nav = n - (k+1);
      else
         nav = aScale;
      derivativeData[k] = (Sum(data, k+1, k+nav) - Sum(data, k-nav, k-1)) / nav;
   }

   derivativeData[0] = 0;
   derivativeData[n-1] = 0;
   return(derivativeData);
}

        
/*
    Baseline correction using wavelet derivative. Follows the steps,
    calculates the first derivative, squares the spectrum and looks for peaks
    and baseline by a recursive method (Cobas et. al. JMR 183 (2006) 145-151.
 */
void BaseMask(float *inData, float *basmask, int n, int aScale) {

   float *data;
   float *baseLine;
   int i;
   double mean = 0.0;
   double max =  0.0;
   double stdev = 0.0;
   double baslineMax = 0.0;
   double tol;
   double threshold;

   /* Wavelet derivative calculation of the input vector */
   data = FirstDerivative(inData, n, aScale);
   for (i = 0; i < n; i++)
   {
      data[i] = data[i] * data[i];
      if (max < data[i])
         max = data[i];
   }

   for (i = 0; i < n; i++)
   {
      data[i] = data[i] / max;
      mean = mean + data[i];
   }

   /* Identify the baseline regions */
   baseLine = (float *) allocateWithId(n * sizeof(float), "bc3");
   memcpy(baseLine, data, n * sizeof(float) );
   mean = mean / n;

            
   for (i = 0; i < n; i++)
   {
      stdev = stdev + pow((data[i] - mean), 2.0);
      basmask[i] = 1;
                        
      if (baslineMax < baseLine[i])
         baslineMax = baseLine[i];
   }
   stdev = sqrt(stdev) / n;
   tol = mean / stdev;
   threshold = mean + 20 * stdev;
            
   while (baslineMax > threshold)
   {
      double baslMinusMeanSum;
      double basMaskSum;

      baslineMax =0;
      for (i = 0; i < n; i++)
      {
         /* Refresh baseline mask */
         if (data[i] >= threshold)
            basmask[i] = 0;
                                        
         /* Renew the baseline */
         baseLine[i] = basmask[i] * data[i];
         if (baslineMax < baseLine[i])
            baslineMax = baseLine[i];
      }
      mean = Sum(baseLine, 0, n-1) / Sum(basmask, 0, n-1);

      baslMinusMeanSum = 0;
      for (i = 0; i < n; i++)
      {
         baslMinusMeanSum = baslMinusMeanSum + pow((baseLine[i] - mean), 2.0);
      }
      basMaskSum = Sum(basmask, 0, n-1);
      stdev = sqrt(baslMinusMeanSum - (n - basMaskSum) * mean * mean);
      stdev = stdev / basMaskSum;
      tol = 3 * (mean / stdev);
      threshold = (mean + tol * stdev);
   }

   for (i = 1; i < n - 1; i++)
   {
      if (basmask[i] == 1)
         if (basmask[i + 1] == 0)
            if (basmask[i - 1] == 0)
               basmask[i] = 0;
   }
   releaseAllWithId("bc3");
}

static float *BaselineMask(float *inData, int n, int aScale)
{
   float *basmask = (float *) allocateWithId(n * sizeof(float), "bc2");
   BaseMask(inData,basmask,n,aScale);
   return basmask;
}

/*
    Baselinefit which is an improved version of basefitnew that though uses
    the band nature of smatrix to invert it, splits the spectrum into regions
    that produce matrices of managable sizes to deal with. The present
    algorithm uses the sparse coding of the matrices thus being able to deal
    with the entire spectrum as one single region.
 */
void BaselineFit(float *indata, int n, float *basemask, double smoothness)
{
   float *lambda;
   float *smatrix;
   int i;
    
   lambda = (float *) allocateWithId(n * sizeof(float), "bc2");
   smatrix = (float *) allocateWithId(n * sizeof(float), "bc2");
   for (i = 0; i < n; i++)
   {
      /*
        Define the smatrix in a sparse representation
        the diagonal part of the lambda matrix
       */
      lambda[i] = 2;
   }
   lambda[n-1] = 1;
   lambda[0]  = smoothness*lambda[0] ;
   smatrix[0] = basemask[0] + lambda[0];

   /*
     The forward loop to calculate the last element of outdata 
     Here outdata is stored in the same array indata
    */
   for(i = 1; i < n; i++)
   {
      smatrix[i] = (basemask[i] + (smoothness*lambda[i])) - smoothness*smoothness/smatrix[i-1];
      indata[i] = indata[i] + smoothness*indata[i-1]/smatrix[i-1];
   }

   indata[n-1] = indata[n-1]/smatrix[n-1];
   /* The backward loop to recalculate the remaining elements of outdata */
   for (i = n-2; i>=0;i--)
   {
      indata[i] = (indata[i] + smoothness*indata[i+1])/smatrix[i];
   }
}

static void BaseFit(float *indata, int n, float *basemask, double smoothness)
{
   int i;
   for (i = 0; i < n; i++)
   {
      /* Weight the spectrum */
      indata[i] = basemask[i] * indata[i];
   }
   BaselineFit(indata, n, basemask, smoothness);
}
 
/*
    Baseline correction algorthim: Takes an input spectrum vector and returns
    the baseline corrected spectrum vector. "ascale" is the scale value of the wavelet
    used to
    calculate the derivative
    calculation, tolerance is the tolerance value used to calculate the
    baseline region and smoothness is the smoothness/fidelity factor used to
    fit the baseline
 */
static void BaseLineCorrection(int n, float *data, int datatype, int aScale, double smoothness)
{
   float *indata;
   float *baseLineFitInData;
   int i;
   float *bsm;
   float *dptr;

   indata = (float *) allocateWithId(n * sizeof(float), "bc2");
   dptr = data;
   for(i=0;i<n;i++)
   {
      indata[i] = *dptr;
      dptr += datatype;
   }
   /* Find the baseline mask */
   bsm = BaselineMask(indata,n,aScale);
         
   baseLineFitInData = (float *) allocateWithId(n * sizeof(float), "bc2");
   memcpy(baseLineFitInData, indata, n * sizeof(float) );

   /* Fit the baseline with a smoother */
   BaseFit(baseLineFitInData,n,bsm,smoothness); 

   /* return the corrected spectrum */
   dptr = data;
   for(i=0;i<n;i++)
   {
      *dptr = indata[i] - baseLineFitInData[i];
      dptr += datatype;
   }
   releaseAllWithId("bc2");
}


/***************************************/
void integ2(float *frompntr, int fpnt, int npnt, float *value, int offset)
/***************************************/
{
  register int		i,
			o;
  register float	v,
			*p;

/*********************************************
*  Calculate the sum of a series of points.  *
*********************************************/

  p = frompntr + (fpnt * offset); v = 0; i = npnt; o = offset;
  while (i--) { v += *p; p += o; }
  *value = v;
}


/*********************/
void integ(register float  *fptr, register float  *tptr, register int npnt)
/*********************/
{
  register float	tmp;

  *tptr++ = tmp = *fptr++;
  while (--npnt)
    *tptr++ = tmp += *fptr++;
}


/***************************/
void maxfloat(register float  *datapntr, register int npnt, register float  *max)
/***************************/
{
  register float	tmp,
			maxval,
			tmp2;

  maxval = 0.0;
  DPRINT1("function maxfloat  npnt= %d\n",npnt);
  while (--npnt)
  {
    tmp = *datapntr++;
    if (tmp > maxval)
      maxval = tmp;
    else if (tmp < 0.0)
    { tmp2 = -tmp;
      if (tmp2 > maxval)
        maxval = tmp2;
    }
  }
  DPRINT1("function maxfloat  max= %g\n",maxval);
  *max = maxval;
}


/***************************/
void maxfloat2(register float  *datapntr, register int npnt, register float  *max)
/***************************/
{
  register float	tmp,
			maxval;

  maxval = *datapntr;
  DPRINT1("function maxfloat2  npnt= %d\n",npnt);
  while (--npnt)
  {
    tmp = *datapntr++;
    if (tmp > maxval)
      maxval = tmp;
  }
  DPRINT1("function maxfloat2  max= %g\n",maxval);
  *max = maxval;
}


/***********************************/
void leveltilt(float *ptr, int offset, double oldlvl, double oldtlt)
/***********************************/
{
  register int		npts,
			o;
  register float	start,
			*p,
			incr;

/**********************************************************
*  Apply the lvl/tlt function prior to the bc operation.  *
**********************************************************/

  npts = fn / 2; p = ptr; o = offset;
  start = ((float)lvl - oldlvl) / CLVL;
  incr  = (((float)tlt - oldtlt) / CLVL) / (float) (fn / 2);
  while (npts--)
  {
    *p += start;
    start += incr;
    p += o;
  }
}


/*********************************************/
void dodc_correction(float *ptr, double newlvl, double newtlt, int pts, int offset)
/*********************************************/
{
  register int		npts,
			off;
  register float	start,
			*p,
			incr;

/**************************************************************
*  Apply the lvl/tlt function with the capability of offset.  *
**************************************************************/

  npts = pts;
  p = ptr;
  off = offset;
  start = newlvl;
  incr  = newtlt;

  while (npts--)
  {
    *p += start;
    start += incr;
    p += off;
  }

/*****************************
*  For debugging operations  *
*****************************/

  DPRINT4("dc_correction: ptr=%d, start=%g, incr=%g, npts=%d\n",
                    ptr,start,incr,npts);
}


/***************************************/
static void aver(float *ptr, int start, int length, float *av, int offset)
/***************************************/
{ 

/*************************************************
*  Calculate the average of a series of points.  *
*************************************************/

  integ2(ptr,start,length,av,offset);
  *av /= (float) length;
}

  
/******************************/
void dodc(float *ptr, int offset, double oldlvl, double oldtlt)
/******************************/
/* calculate lvl and tlt parameters */
{
  int		spacing,
		le;
  register int	num,
		i;
  float		avl,
		avr,
		corr_avl,
		corr_avr;

/**********************************
*  Calculate lvl/tlt parameters.  *
**********************************/

  le = 1 + 2 * (int) ((float) npnt / 40.0);
  if (le > 351) le = 351;
  spacing = npnt - le;

  DPRINT("function dodc\n");
  DPRINT4("npnt=%d, length=%d, spacing=%d, offset=%d\n",
                    npnt,le,spacing,offset);

  aver(ptr,fpnt,le,&avl,offset);
  aver(ptr,fpnt+spacing,le,&avr,offset);
  oldlvl /= CLVL;
  oldtlt /= (float) (fn / 2) * CLVL;
  num = 0;
  for (i = 1; i < le; i++)
    num += i;
  corr_avl = oldtlt * (float) num / (float) le;
  corr_avr = oldlvl + oldtlt * (float) (fpnt+spacing) + corr_avl;
  corr_avl = oldlvl + oldtlt * (float) fpnt + corr_avl;

  DPRINT3("avl=%g, corr_avl=%g, new_avl=%g\n",avl,corr_avl,avl-corr_avl);
  DPRINT3("avr=%g, corr_avr=%g, new_avr=%g\n",avr,corr_avr,avr-corr_avr);

  avl -= corr_avl;
  avr -= corr_avr;
  tlt = (avl - avr) / (float) spacing;
  lvl = - avl - tlt * (float)(fpnt + (le / 2));
  lvl = lvl * CLVL;
  tlt = tlt * (float) (fn / 2) * CLVL;
}


/***************************************************/
static void getbaseline(struct base baseline[], int *numpts,
                        int minpts, int minbcpts)
/***************************************************/
{
  int		index,
		j,
		totalpts,
		fewest,
		numfpts,
		r;
  double	value;

  DPRINT4("sections= %d, numpts= %d, min1= %d, min2= %d\n",
                sections,*numpts,minpts,minbcpts);

  baseline[1].fpt = 0;
  fewest = fn/2;
  totalpts = 0;
  for (index = 1; index <= sections; index++)
  {
    if ( (r=P_getreal(CURRENT,"lifrq",&value,index * 2 - 1)) )
      value = 0.0;          /*  no resets defined  */

/*****************************************************************
*  lifrq is the parameter in which the integral reset points     *
*  are stored.  The (index*2 - 1) aspect selects the odd order   *
*  reset points.                                                 *
*****************************************************************/

    baseline[index].npt = datapoint(value,sw,fn/2)  - baseline[index].fpt;

/***********************************************************************
*  The number of data points between 0 and value (see above) are       *
*  calculated.  The number of points between 0 and the beginning of    *
*  the region terminated by value is subtracted from this to give the  *
*  number of points actually in the baseline region terminated by      *
*  value.                                                              *
***********************************************************************/

    totalpts += baseline[index].npt;
    if (baseline[index].npt < fewest)
      fewest = baseline[index].npt;
    if (index != sections)
    {
      if ( (r=P_getreal(CURRENT,"lifrq",&value,index * 2)) )
        value = 0.0;          /*  no resets defined  */
      baseline[index + 1].fpt = datapoint(value,sw,fn/2);
     if (baseline[index + 1].fpt < totalpts)
     {
        baseline[index + 1].fpt = totalpts;
     }

/********************************************************************
*  This is necessary because we will need the number of data points *
*  spanned by the even baseline region as well.  Note that *.fpt is *
*  offset by 1 from *.npt.                                          *
********************************************************************/

    }
  }

  DPRINT2("fewest= %d, totalpts= %d\n",fewest,totalpts);

  if (fewest < minpts)
  {
    index = 0;
    while ((index++ < sections) && (sections > 1))
      if (baseline[index].npt < minpts)
      {
        totalpts -= baseline[index].npt;
        sections -= 1;
        DPRINT4("index= %d, totalpts= %d, sections= %d, npt= %d\n",
                  index,totalpts,sections,baseline[index].npt);
        if (index <= sections)
          for (j=index; j <= sections; j++)
          {
            baseline[j].fpt = baseline[j + 1].fpt;
            baseline[j].npt = baseline[j + 1].npt;

/**********************************************************
*  This removes *.npt data points and puts them into the  *
*  corresponding *.fpt if that baseline region has fewer  *
*  than the minimum number of points required.            *
**********************************************************/

          }
        index--;
      }

    DPRINT3("fewest= %d, totalpts= %d, sections= %d\n",
                  fewest,totalpts,sections);
  }

  if ((sections < 2) || (totalpts < *numpts) ||
      (*numpts < sections * minbcpts))
  {
    sections = 0;
    return;
  }
  numfpts = 0;
  value = (double) (*numpts - sections * minbcpts) / (double) totalpts;
  for (index = 1; index <= sections; index++)
  {
    baseline[index].bcpt = minbcpts +
                            (int) (value * (double)baseline[index].npt);
    numfpts += baseline[index].bcpt;
  }

/*******************************************************************
*  *.bcpt gives us the number of points per baseline region to be  *
*  used in the baseline corrrection.  There is intrinsic scaling   *
*  so that the number of points (for bc) in excess of the minimum  *
*  number is proportional to the actual number of points in that   *
*  baseline region.                                                *
*******************************************************************/

  DPRINT2("numfpts= %d, numpts= %d\n",numfpts,*numpts);

  while (numfpts < *numpts)
  { int max,maxindex=0;

    max = 0;
    for (index = 1; index <= sections; index++)
      if (baseline[index].npt / baseline[index].bcpt > max)
      {
        max = baseline[index].npt / baseline[index].bcpt;
        maxindex = index;
      }
    baseline[maxindex].bcpt += 1;
    numfpts++;

/**********************************************************************
*  If NUMFTPS (the actual number of points currently to be used in    *
*  the bc) is still less than NUMPTS (the requested number of points  *
*  to be used in the bc), additional bc points are taken from the     *
*  baseline regions that have the highest *.npt / *.bcpt ratio.       *
**********************************************************************/

  }

  DPRINT2("numfpts= %d, numpts= %d\n",numfpts,*numpts);

  while (numfpts > *numpts)
  { int min,minindex=0;

    min = totalpts;
    for (index = 1; index <= sections; index++)
      if ((baseline[index].npt / baseline[index].bcpt < min) &&
          (baseline[index].bcpt > minbcpts))
      {
        min = baseline[index].npt / baseline[index].bcpt;
        minindex = index;
      }
    baseline[minindex].bcpt -= 1;
    numfpts--;

/**********************************************************************
*  If NUMFTPS (the actual number of points currently to be used in    *
*  the bc) is greater than NUMPTS (the requested number of points to  *
*  be used in the bc), bc points are removed from the baseline reg-   *
*  ions that have the lowest *.npt / *.bcpt ratio.                    *
**********************************************************************/

  }

  DPRINT2("numfpts= %d, numpts= %d\n",numfpts,*numpts);

  for (index = 1; index <= sections; index++)
  {
    baseline[index].bcincr = (double) baseline[index].npt /
                             (double) baseline[index].bcpt;
    if (baseline[index].bcincr < 1.0)
      baseline[index].bcincr = 1.0;

/********************************************************************
*  *.bcincr contains the ratio of the number of data points in a    *
*  given baseline region to the number of points in that region to  *
*  be actually used in the bc.                                      *
********************************************************************/

  }

#ifdef  DBUG
  if (debug1)
    for (index = 1; index <= sections; index++)
      DPRINT5("index %d: fpt= %d, npt= %d, bcpt= %d, bcincr= %g\n",
                  index,baseline[index].fpt,baseline[index].npt,
                  baseline[index].bcpt,baseline[index].bcincr);
#endif 
}


/*-----------------------------------------------
|						|
|		 bc_input()/8			|
|						|
+----------------------------------------------*/
static int bc_input(int argc, char *argv[], int *order, int *numpts,
                    int *minpts, int *minbcpts, int *startspec_no,
                    int *endspec_no)
{
   int	temp,
	r;

/*****************************************************
*  BC2D flag is set.  The flag for the direction of  *
*  BC2D is also set.                                 *
*****************************************************/

   bc2dflag = unbcflag = FALSE;		/* necessary initialization */

   if (argc > 1)
   {
      if (strcmp(argv[1], "f1") == 0)
      {
         bc2dflag = TRUE;
         revflag = TRUE;
         strcpy(newtrace, "f1");
         argc--;
         argv++;
      }
      else if (strcmp(argv[1], "f2") == 0)
      {
         bc2dflag = TRUE;
         revflag = FALSE;
         strcpy(newtrace, "f2");
         argc--;
         argv++;
      }
   }
   else if (strcmp(argv[0], "bc2d") == 0)
   {
      Werrprintf("bc2d():  specify trace direction, e.g., 'f1' or 'f2'\n");
      return(ERROR);
   }

/********************************************************
*  Redefines the "trace" parameter so that init2d will  *
*  correctly set up the structures and parameters for   *
*  accessing either 1D or 2D data.                      *
********************************************************/

   if (bc2dflag)
   {
      if ( (r = P_getstring(CURRENT, "trace", oldtrace, 1, 4)) )
      {
         P_err(r, "trace", ":");
         return(ERROR);
      }

      if ( (r = P_setstring(CURRENT, "trace", newtrace, 0)) )
      {
         P_err(r, "trace", ":");
         return(ERROR);
      }
   }

   if (init2d(1, 1))
      return(ERROR);

   if ( (argc > 1) && ! strcmp(argv[1],"ifnotddr") )
   {
      if (datahead.status & S_DDR)
      {
         *order = 0;
         return(ERROR);
      }
      argc--;
      argv++;
   }

/************************************************
*  Flag for reversing the BC operation is set.  *
************************************************/

   if (!bc2dflag)
   {
      if (argc > 1)
      {
         if (strcmp(argv[1], "unbc") == 0)
         {
            unbcflag = TRUE;
            argv++;
            argc--;
         }
      }
   }

   if ( (argc > 1) && (!isReal( *(argv+1) )) )
   {
      Werrprintf("BC supports only 'unbc', 'f2', 'f1', and 'ifnotddr' as string args\n");
      return(ERROR);
   }
        
/**************************************************
*  The order of the polynomial to be used in the  *
*  baseline correction is set.                    *
**************************************************/

   if ((argc-- > 1) && isReal(*++argv))
   {
     temp = (int) stringReal(*argv);
     if (temp > 1)
       *order = temp;
     if (*order > MAXORDER)
       *order = MAXORDER;
   }

/**************************************************
*  For BC2D, the spectral trace numbers at which  *
*  the BC2D operation begins and ends are set.    *
**************************************************/

   if (bc2dflag)
   {
      *startspec_no = 1;
      if ((argc-- > 1) && isReal(*++argv))
         *startspec_no = (int) stringReal(*argv);

      *endspec_no = 0;
      if ((argc-- > 1) && isReal(*++argv))
         *endspec_no = (int) stringReal(*argv);
   }

/***************************************************
*  A default value for the number of points to be  *
*  used in the baseline correction is set based    *
*  upon fn.                                        *
***************************************************/

   if (fn < 2000)
   {
      *numpts = 20;
   }
   else if (fn < 5000)
   {
      *numpts = 40;
   }
   else
   {
      *numpts = 80;
   }

/**************************************************
*  The default value for the number of points to  *
*  be used in the baseline correction can be      *
*  overridden by parameter entry.                 *
**************************************************/

   if ((argc-- > 1) && isReal(*++argv))
   {
      temp = (int) stringReal(*argv);
      if (temp > 0)
         *numpts = temp;

      if (*numpts < 3)
      {
         *numpts = 3;
      }
      else if (*numpts > MAXPTS)
      {
         *numpts = MAXPTS;
      }
   }

/*****************************************************
*  The minimum number of points per baseline region  *
*  is defined at this stage.  The default value is   *
*  defined first.  If the parameter is entered, the  *
*  entered value overrides the default value.        *
*****************************************************/

   *minpts = fn / 1000;
   if (*minpts < 4)
      *minpts = 4;

   if ((argc-- > 1) && isReal(*++argv))
   {
      temp = (int) stringReal(*argv);
      if (temp > 0)
         *minpts = temp;
      if (*minpts < 1)
         *minpts = 1;
   }

/*****************************************************
*  The minimum number of points per baseline region  *
*  to be used in the baseline correction is defined  *
*  at this stage.  The default value is defined      *
*  first.  If the parameter is entered, the entered  *
*  value overrides the default value.                *
*****************************************************/

   *minbcpts = 1;
   if ((argc-- > 1) && isReal(*++argv))
   {
      temp = (int) stringReal(*argv);
      if (temp > 0)
         *minbcpts = temp;
      if (*minbcpts < 1)
         *minbcpts = 1;
   }

   return(COMPLETE);
}


/**************************************************/
static void getxpoints(struct base baseline[], int *numpts,
                       double xval[], double xpos[], int order)
/**************************************************/
{
  double midpoint,ptscale;
  int    index,piece,point,first;

  midpoint = (double) (fn / 4);
  ptscale  = midpoint / RANGE;
  point = 0;
  for (index = 1; index <= sections; index++)
    for (piece = 1; piece <= baseline[index].bcpt; piece++)
    {
      first = baseline[index].fpt +
              (int) ((double) (piece - 1) * baseline[index].bcincr);
      xval[++point] = (double) first + (baseline[index].bcincr / 2.0);
      if (order > 1)
        xval[point] = (xval[point] - midpoint) / ptscale;

/*********************************************************************
*  XVAL is scaled to go from -1 to +1.  *.bcincr is the increment    *
*  factor:  points used in the baseline correction within a given    *
*  baseline region are separated by *.bcincr points.  For a given    *
*  increment spanning *.bcincr points, the actual value used in      *
*  the baseline correction is the average over the *.bcincr points.  *
*********************************************************************/

    }
  point = *numpts / 2;
  if (order > 1)
    for (index = 1; index <= point; index++)
      xpos[index] = cos(M_PI * ((double)index - 0.5) / (double) point);

/*******************************************************************
*  The x-axis variables are now cast in the appropriate manner in  *
*  order to use the Chebychev algorithm in performing the least    *
*  squares analysis for the baseline correction.                   *
*******************************************************************/

}


/*---------------------------------------
|					|
|	     getypoints()/4		|
|					|
+--------------------------------------*/
static void getypoints(struct base baseline[], double yval[], float *specptr, int dtype)
{
   int		index,
		piece,
		point,
		first;
   float	val;


   point = 0;
   for (index = 1; index <= sections; index++)
   {
      for (piece = 1; piece <= baseline[index].bcpt; piece++)
      {
         first = baseline[index].fpt +
                 (int) ((double) (piece - 1) * baseline[index].bcincr);
         aver(specptr, first, (int) baseline[index].bcincr, &val, dtype);
         yval[++point] = (double) val;
      }
   }
}


/*---------------------------------------
|					|
|	      readcoefs()/2		|
|					|
+--------------------------------------*/
static int readcoefs(int *order, double coef[])
{
   char	filename[MAXPATHL];
   int	i;

/***********************************
*  The path to bc.out is defined.  *
***********************************/

   strcpy(filename, curexpdir);
#ifdef UNIX
   strcat(filename, "/bc.out");
#else 
   vms_fname_cat(filename, "bc.out");
#endif 

/************************************************
*  The bc.out text file is opened with read     *
*  permission only.  If the file cannot be      *
*  opened, an error message is displayed.  The  *
*  coefficients are then read from the file     *
*  and the file is closed.                      *
************************************************/

   textfile = fopen(filename, "r");
   if (textfile == 0)
      return(ERROR);

   fscanf(textfile, "      Order\n");
   fscanf(textfile, "      -----\n");
   fscanf(textfile, "\n");
   fscanf(textfile, "%d\n", order);

   fscanf(textfile, "\n");
   fscanf(textfile, "\n");
   fscanf(textfile, "   Coefficients\n");
   fscanf(textfile, "   ------------\n");
   fscanf(textfile, "\n");
   for (i = 1; i <= *order; i++)
      fscanf(textfile, "%lf\n", &coef[i]);

   fclose(textfile);
   return(COMPLETE);
}


/*---------------------------------------
|					|
|	       getcoefs()/6		|
|					|
+--------------------------------------*/
static int getcoefs(int order, int numpts, double xval[], double xpos[],
                    double yval[], double coef[])
{
   char		filename[MAXPATHL];
   int		i,
		j,
  		calcpts;
   double	mult,
  		sum,
		max,
		min,
  		y[MAXPTS+1];


   calcpts = numpts / 2;
   sum = 0.0;
   for (j = 1; j <= calcpts; j++)
   {
      i = 1;
      while ((i < numpts) && (xval[i] < xpos[j]))
         i++;

      if ((i == 1) || (xval[i] < xpos[j]))
      {
         y[j] = yval[i];
      }
      else
      {
         mult = (xval[i] - xpos[j]) / (xval[i] - xval[i-1]);
         y[j] = mult * yval[i-1] + (1.0 - mult) * yval[i];
      }

      sum += y[j];
   }


   max = min = coef[1] = sum * 2.0 / (double) calcpts;
   for (i = 2; i <= order; i++)
   {
      sum = 0.0;
      mult = (double) (i - 1) * M_PI / (double) calcpts;
      for (j = 1; j <= calcpts; j++)
         sum += y[j] * cos(mult * ((double)j - 0.5));

      coef[i] = sum * 2.0 / (double) calcpts;
      if (coef[i] > max)
      {
         max = coef[i];
      }
      else if (coef[i] < min)
      {
         min = coef[i];
      }
   }

/********************************************
*  Do not write least squares coefficients  *
*  to bc.out if a BC2D operation is being   *
*  performed.  Define the path to bc.out.   *
********************************************/

   if (!bc2dflag)
   {
      strcpy(filename, curexpdir);
#ifdef UNIX
      strcat(filename, "/bc.out");
#else 
      vms_fname_cat(filename, "bc.out");
#endif 

/***********************************************
*  The bc.out text file is either created or   *
*  truncated with writing permission.  If the  *
*  file cannot be opened, an error message is  *
*  displayed.                                  *
***********************************************/

      textfile = fopen(filename, "w");
      if (textfile == NULL)
      {
         Werrprintf("Cannot open file %s", filename);
         return(ERROR);
      }

/************************************************
*  The least squares coefficients are written   *
*  out in numerical order.  The bc.out file is  *
*  then closed.  It will be used if and only    *
*  if the function "unbc" is called.            *
************************************************/

      fprintf(textfile, "      Order\n");
      fprintf(textfile, "      -----\n");
      fprintf(textfile, "\n");
      fprintf(textfile, "        %d\n", order);
      fprintf(textfile, "\n");
      fprintf(textfile, "\n");
      fprintf(textfile, "   Coefficients\n");
      fprintf(textfile, "   ------------\n");
      fprintf(textfile, "\n");
      for (i = 1; i <= order; i++)
         fprintf(textfile, "%16.12f\n", coef[i]);

      fclose(textfile);
   }

   return(COMPLETE);
}


/*************************************************************/
static struct base *getfunction(int *numpts, int minpts, int minbcpts, int order)
/*************************************************************/
{
  vInfo  info;
  struct base *baseline;
  double value;

  sections = ((P_getVarInfo(CURRENT,"lifrq",&info)) ? 1 : info.size);
  sections = (sections + 1) / 2;

  if (sections < 2)
  {
    Werrprintf("Integral resets must be set before baseline correction.");
    disp_status("        ");
    return(0);
  }

  if ((P_getreal(CURRENT,"lifrq",&value,1) == 0) && (value > sw))
  {
    Werrprintf("Integral resets are inappropriate for this spectral width");
    disp_status("        ");
    return(0);
  }

/*************************************************
*  Memory is allocated to store the x points to  *
*  be used in the baseline correction.           *
*************************************************/

  if ((xval =
    (double *) allocateWithId(sizeof(double) * (*numpts + 1),"bc"))==0)
  {
    Werrprintf("cannot allocate buffer");
    disp_status("        ");
    return(0);
  }

/*******************************************************************
*  Memory is allocated to store the XPOS values which are needed   *
*  to implement the Chebychev approach to least squares analysis.  *
*******************************************************************/

  if ((xpos =
    (double *) allocateWithId(sizeof(double) * (*numpts/2 + 1),"bc"))==0)
  {
    Werrprintf("cannot allocate buffer");
    disp_status("        ");
    release(xval);
    return(0);
  }

/*****************************************************
*  Memory is allocated to store the y values of the  *
*  x points to be used in the baseline correction.   *
*****************************************************/

  if ((yval =
    (double *) allocateWithId(sizeof(double) * (*numpts + 1),"bc"))==0)
  {
    Werrprintf("cannot allocate buffer");
    disp_status("        ");
    release(xval);
    release(xpos);
    return(0);
  }

/********************************************************
*  Memory is allocated to store the structure BASE for  *
*  each baseline region in the spectrum.                *
********************************************************/

  if ((baseline =
  (struct base *) allocateWithId(sizeof(struct base)*(sections + 1),"bc"))==0)
  {
    Werrprintf("cannot allocate buffer");
    disp_status("        ");
    release(xval);
    release(xpos);
    release(yval);
    return(0);
  }

  getbaseline(baseline,numpts,minpts,minbcpts);
  if (sections < 2)
  {
    Werrprintf("integral resets must be set before baseline correction");
    disp_status("        ");
    releaseAllWithId("bc");

/*******************************************************
*  All memory allocated with the ID "bc" is released.  *
*******************************************************/

    return(0);
  }
  getxpoints(baseline,numpts,xval,xpos,order);
  return(baseline);
}


/*---------------------------------------
|					|
|		dobc()/5		|
|					|
+--------------------------------------*/
static void dobc(register float *specptr, register int npt, register double coef[],
                 register int order, int dtype)
{
   register int		point,
			num;
   register double	incr,
			xpt,
			d,
			dd,
			sv;

   npt--;
   incr = ((double) npt / 2.0) / RANGE;
   for (point = 0; point <= npt; point++)
   {
      xpt = -RANGE + (double) point / incr;
      xpt *= 2.0;
      d = 0.0;
      dd = 0.0;

      for (num = order; num >= 2; num--)
      {
         sv = d;
         d = xpt * d - dd + coef[num];
         dd = sv;
      }

/**********************************************
*  Baseline is either subtracted or added to  *
*  the real part (displayed part) or the      *
*  spectrum.                                  *
**********************************************/

      if (unbcflag)
      {
         *specptr += xpt * d / 2.0 - dd + 0.5 * coef[1];
      }
      else
      {
         *specptr -= xpt * d / 2.0 - dd + 0.5 * coef[1];
      }

/*********************************************
*  This forces the bc operation to skip the  *
*  imaginary part (undisplayed part) of the  *
*  spectrum in the bc opeation.              *
*********************************************/

      specptr += dtype;
   }
}


/*---------------------------------------
|					|
|		fitdc()/4		|
|					|
+--------------------------------------*/
void splineFit(float *data, int dtype, double *xval, double  *xpos, double *yval, int numpts) {

   int		point,
		last,
		spacing;
   double	lvlv,
		tltv;


/**********************************
*  Check for reverse bc request.  *
**********************************/

   spacing = xval[2] - xval[1];
   tltv = (yval[1] - yval[2]) / (double) spacing;
   lvlv = - tltv * xval[1] - yval[1];
   dodc_correction(data, lvlv, tltv, (int)(xval[2]), dtype);
   data += dtype * ((int) xval[2]);
   last = (int) xval[2];

   for (point = 3; point < numpts; point++)
   {
      spacing = xval[point] - last;
      tltv = (yval[point-1] - yval[point]) / (double) spacing;
      lvlv = - yval[point-1];
      dodc_correction(data, lvlv, tltv, spacing, dtype);
      data += dtype*spacing;
      last += spacing;
   }

   spacing = xval[numpts] - last;
   tltv = (yval[numpts-1] - yval[numpts]) / (double) spacing;
   lvlv = - yval[numpts-1];
   dodc_correction(data, lvlv, tltv, fn/2 - last, dtype);
}

static void fitdc(struct base baseline[], float *data, int numpts, int dtype)
{
   getypoints(baseline, yval, data, dtype);
   splineFit(data, dtype, xval, xpos, yval,numpts);
}


/*-----------------------------------------------
|						|
|		 fit2ddata()/6			|
|						|
+----------------------------------------------*/
static int fit2ddata(struct base baseline[], double coef[], int order,
                     int numpts, int startspec, int endspec)
{
   int		block_no,
		maxspec,
		f2phase=FALSE,
		f1phase=FALSE,
		npf1,
		npf2,
		nplinear,
		npblock,
		no_imagpnt,
		hypercomplex,
		datatype,
		halfft_flag = FALSE,
		trace_no,
		startblock,
		endblock,
		starttrace,
		endtrace,
		r,
		block_offset;
   float	*tracepnt,
		*pf1buffer,
        	*mf1buffer,
		*pf2buffer,
        	*mf2buffer;
   double	rpf1,
		lpf1,
		rpf2,
		lpf2;


/**********************
*  Check for 2D data  *
**********************/

   if (!d2flag)
   {
      Werrprintf("No 2D data in data file");
      return(ERROR);
   }

/*************************************
*  Check for half-transformed data.  *
*  Set flags for type of data.       *
*************************************/

   if ((datahead.status & (S_DATA|S_SPEC|S_SECND)) == (S_DATA|S_SPEC))
   {
      halfft_flag = TRUE;
      if (revflag)
      {
         Werrprintf("Cannot BC t1 interferograms");
         return(ERROR);
      }
   }

   if (datahead.status & S_HYPERCOMPLEX)
   {
      datatype = HYPERCOMPLEX;
      hypercomplex = TRUE;
   }
   else if (datahead.status & S_COMPLEX)
   {
      datatype = COMPLEX;
      hypercomplex = FALSE;
      f2phase = FALSE;	    /* initialized to FALSE for complex 2D data */
   }
   else
   {
      datatype = REAL;
      hypercomplex = FALSE;
      f2phase = FALSE;
      f1phase = FALSE;
   }
     
/***********************************************
*  The PHASEFILE is removed prior to starting  *
*  the bc2d operation since bc2d changes the   *
*  raw 2D spectral data.                       *
***********************************************/

   if (removephasefile())
      return(ERROR);

/****************************************
*  Set up the parameters for phase and  *
*  number of points.  Set BLOCK offset  *
*  based upon the status of the data.   *
****************************************/
 
   nplinear = pointsperspec;
   npblock = specperblock * nblocks;
   if (hypercomplex)
   {  
      npblock *= 2;
      nplinear /= 2;
   }
 
   if (revflag)
   {
      npf1 = nplinear;
      npf2 = npblock;
      block_offset = 0;
   }
   else
   {
      npf2 = nplinear;
      npf1 = npblock;
      block_offset = nblocks;
   }

/**********************************************************
*  Set maximum spectra index on status of processing and  *
*  direction of BC2D and calculate the number of spectra  *
*  per block.                                             *
**********************************************************/

   if (halfft_flag)
   {
      char	ni0name[10];
      double	ni0val;

      if (datahead.status & S_NI)
      {
         strcpy(ni0name, "ni");
      }
      else if (datahead.status & S_NI2)
      {
         strcpy(ni0name, "ni2");
      }
      else if (datahead.status & S_NF)
      {
         strcpy(ni0name, "nf");
      }
      else
      {
         Werrprintf("Not a valid 2D experiment\n");
         return(ERROR);
      }
 
      if ( (r = P_getreal(PROCESSED, ni0name, &ni0val, 1)) )
      {
         Werrprintf("Invalid 2D array parameter"); 
	 return(ERROR);
      }

      maxspec = (int) (ni0val + 0.5);
      if (maxspec > (fn1/2))
         maxspec = fn1/2;
   }
   else
   {
      maxspec = fn1/2;
   }

/***********************************************************
*  Calculate the starting and ending block values for the  *
*  portion of the 2D data to be baseline corrected.        *
***********************************************************/

   if (endspec == 0)
   {
      endspec = maxspec;
   }
   else if (endspec > maxspec)
   {
      endspec = maxspec;
   }

   if (startspec < 1)
   {
      startspec = 1;
   }
   else if (startspec > endspec)
   {
      startspec = endspec;
   }

   startblock = startspec/specperblock;
   if (startblock >= nblocks)
   {
      Werrprintf("Starting spectrum number is greater than the allowed range");
      return(ERROR);
   }

   endblock = endspec/specperblock;
   if ((endblock*specperblock) >= endspec)
      endblock--;
   if (endblock >= nblocks)
   {
      Werrprintf("Ending spectrum number is greater than the allowed range");
      return(ERROR);
   }

/*****************************************************
*  Allocate memory for and create the + and - phase  *
*  rotation vectors.                                 *
*****************************************************/ 

   C_GETPAR("lp", &lpf2);
   C_GETPAR("rp", &rpf2);
   if (datahead.status & (S_NI|S_NF))
   {
      C_GETPAR("lp1", &lpf1);
      C_GETPAR("rp1", &rpf1);
   }
   else
   {
      C_GETPAR("lp2", &lpf1);
      C_GETPAR("rp2", &rpf1);
   }
 
   pf1buffer = mf1buffer = NULL;        /* initialize pointers */
   pf2buffer = mf2buffer = NULL;        /* initialize pointers */
 
/****************************************
*  Display appropriate Vnmr processing  *
*  status.                              *
****************************************/

   if (revflag)
   {
      disp_status("BC2D  F1");
   }
   else
   {
      disp_status("BC2D  F2");
   }

/*******************************************
*  Start the execution of the 2D baseline  *
*  correction.                             *
*******************************************/

   for (block_no = startblock; block_no <= endblock; block_no++)
   {
      disp_index(endblock - block_no + 1);
      if ( (r = D_getbuf(D_DATAFILE, datahead.nblocks, block_no +
		block_offset, &c_block)) )
      {
         D_error(r);
         return(ERROR);
      }

      if (!c_block.head->status & S_DATA)
      {
         Werrprintf("no data in block %d", block_no + 1);
         return(ERROR);
      }
      else if (!(c_block.head->status & S_SPEC))
      {
         Werrprintf("no spectra in block %d", block_no + 1);
         return(ERROR);
      }
      else if (block_no == startblock)
      { /* only do for the first data block */
         if (hypercomplex)
         {
            if (c_block.head->mode & NP_PHMODE)
            {
               f2phase = ((fabs(rpf2) > MINDEGREE) || (fabs(lpf2) > MINDEGREE));
            }
            else
            {
               f2phase = FALSE;
            }
         }
 
         no_imagpnt = ( !(c_block.head->status &
                             (NF_CMPLX|NI_CMPLX|NI2_CMPLX)) );
 
         if ( (!no_imagpnt) && (!halfft_flag) && (c_block.head->mode &
                        (NI_PHMODE|NF_PHMODE|NI2_PHMODE)) )
         {
            f1phase = ((fabs(rpf1) > MINDEGREE) || (fabs(lpf1) > MINDEGREE));
         }
         else
         {
            f1phase = FALSE;
         }     

/************************************************
*  Setup the + and - F1 phase-rotation vectors  *
*  for phasing hypercomplex or complex 2D data. *
************************************************/

         if (f1phase)
         {
            pf1buffer = (float *) allocateWithId(npf1 * sizeof(float), "bc");
            if (pf1buffer == NULL)
            {
               Werrprintf("Cannot allocate +F1 phase-rotation vector");
               return(ERROR);
            }

            mf1buffer = (float *) allocateWithId(npf1 * sizeof(float), "bc");
            if (mf1buffer  == NULL)
            {
               Werrprintf("Cannot allocate -F1 phase-rotation vector");
               return(ERROR);
            }
 
            phasefunc(pf1buffer, npf1/2, lpf1, rpf1);
            phasefunc(mf1buffer, npf1/2, -lpf1, -rpf1);
         }

/************************************************
*  Setup the + and - F2 phase-rotation vectors  *
*  for phasing hypercomplex 2D data.            *
************************************************/
 
         if (f2phase)
         {
            pf2buffer = (float *) allocateWithId(npf2 * sizeof(float), "bc");
            if (pf2buffer == NULL)
            {
               Werrprintf("Cannot allocate +F2 phase-rotation vector");
               return(ERROR);
            }

            mf2buffer = (float *) allocateWithId(npf2 * sizeof(float), "bc");
            if (mf2buffer == NULL)
            {
               Werrprintf("Cannot allocate -F2 phase-rotation vector");
               return(ERROR);
            }

            phasefunc(pf2buffer, npf2/2, lpf2, rpf2);
            phasefunc(mf2buffer, npf2/2, -lpf2, -rpf2);
         }
 
/*************************************
*  Readjust "npf1" and "npf2" to be  *
*  per block.                        *
*************************************/
 
         if (revflag)
         {
            npf2 /= nblocks;
         }
         else
         {
            npf1 /= nblocks;
         }     
      }

/*******************************************
*  Phase-rotate the 2D data if necessary.  *
*******************************************/

      if (f1phase || f2phase)
      {
         data2d_rotate(c_block.data, f1phase, pf1buffer, f2phase, pf2buffer,
                           nblocks, block_no + block_offset, npf1/2,
                           npf2/2, hypercomplex);
      }

/********************************************
*  Calculate the starting and ending trace  *
*  numbers for each block of data.          *
********************************************/

      endtrace = ( (block_no == endblock) ? ((endspec - 1) % specperblock)
			: (specperblock - 1) );

      starttrace = ( (block_no == startblock) ? ((startspec -1) % specperblock)
			: 0 );

/*******************************************
*  Execute the 2D baseline correction for  *
*  one block of 2D data.                   *
*******************************************/

      for (trace_no = starttrace; trace_no <= endtrace; trace_no++)
      {
	 tracepnt = c_block.data + trace_no*datatype*(fn/2);
         getypoints(baseline, yval, tracepnt, datatype);
         if (order > 1)
         {
            if (getcoefs(order, numpts, xval, xpos, yval, coef))
            {
               Werrprintf("Unable to get BC coefficients");
               return(ERROR);
            }

            dobc(tracepnt, fn/2, coef, order, datatype);
         }
         else
         {
            fitdc(baseline, tracepnt, numpts, datatype);
         }

         if (halfft_flag)
         {
            tracepnt += (datatype/2);
            getypoints(baseline, yval, tracepnt, datatype);
            if (order > 1)
            {
               if (getcoefs(order, numpts, xval, xpos, yval, coef))
               {
                  Werrprintf("Unable to get BC coefficients");
                  return(ERROR);
               }
 
               dobc(tracepnt, fn/2, coef, order, datatype);
            }
            else
            {  
               fitdc(baseline, tracepnt, numpts, datatype);
            }
         }
      }

/**********************************************
*  Un-phase-rotate the 2D data if necessary.  *
**********************************************/
 
      if (f1phase || f2phase)
      {
         data2d_rotate(c_block.data, f1phase, mf1buffer, f2phase, mf2buffer,
                           nblocks, block_no + block_offset, npf1/2,
                           npf2/2, hypercomplex);
      }

/***********************************************
*  Mark the block in the datafile and release  *
*  the memory for this block.                  *
***********************************************/

      if ( (r = D_markupdated(D_DATAFILE, block_no + block_offset)) )
      {
         D_error(r);
         return(ERROR);
      }

      if ( (r = D_release(D_DATAFILE, block_no + block_offset)) )
      {
         D_error(r);
         return(ERROR);
      }

/***************************************************
*  If an interruption occurs, clear the displayed  *
*  status and index and abort the 2D baseline      *
*  correction.                                     *
***************************************************/

      if (interuption)
         return(ERROR);
   }


   disp_status("        ");
   disp_index(0);
   if (init2d(1, 1))
      return(ERROR);

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	      fitdata()/6		|
|					|
+--------------------------------------*/
static int polyFit(float *data, int dtype, double *xval, double *xpos, double *yval,
	int numpts, double coef[], int order) 
{
/**************************************************
*  Read order of polynomial and least squares     *
*  coefficients from bc.out if reverse BC         *
*  operation is requested.  Otherwise, calculate  *
*  least squares coefficients from the data.      *
**************************************************/

   if (unbcflag)
   {
      if (readcoefs(&order, coef))
      {
         Werrprintf("Unable to read BC coefficients");
         return(ERROR);
      }

      if (order == 1)
      {
         Werrprintf("Spline fit cannot be reversed.\n");
         return(ERROR);
      }
   }
   else
   {
      if (getcoefs(order, numpts, xval, xpos, yval, coef))
      {
         Werrprintf("Unable to get BC coefficients");
         return(ERROR);
      }
   }

   dobc(data, fn/2, coef, order, dtype);
   return(COMPLETE);
}

static int fitdata(struct base baseline[], float *data, double coef[],
                   int order, int numpts, int dtype)
{
   getypoints(baseline, yval, data, dtype);
   return polyFit(data, dtype, xval, xpos, yval, numpts, coef, order);
}

/*---------------------------------------
|					|
|	     freebuffers()/1		|
|					|				
+--------------------------------------*/
static int freebuffers(int update)
{
   int	res;

   releaseAllWithId("bc");
   if (update)
   {
      if (c_buffer >= 0) /* release last used block */
      {
         c_block.head->rpval += 1.0;
         if ( (res = D_markupdated(D_PHASFILE, c_buffer)) )
         {
            D_error(res);
            return(ERROR);
         }

         if ( (res = D_release(D_PHASFILE, c_buffer)) )
         {
            D_error(res); 
            D_close(D_PHASFILE);  
            return(ERROR);
         }
      }

      if ( (res = D_markupdated(D_DATAFILE, c_buffer)) )
      {
         D_error(res);
         return(ERROR);
      }

      if ( (res = D_release(D_DATAFILE, c_buffer)) )
      {
         D_error(res);
         return(ERROR);
      }
   }

   return(COMPLETE);
}


/*---------------------------------------
|					|
|	     getdata_4bc()/0		|
|					|
+--------------------------------------*/
static float *getdata_4bc()
{
   int		ctrace,
		r;
   float	*data;
   dpointers	datablock;

 
   ctrace = currentindex();
   if ((data = gettrace(ctrace-1, 0)) == NULL)	/* What does this do? */
      return(NULL);

   if ( (r = D_getbuf(D_DATAFILE, datahead.nblocks, c_buffer, &datablock)) )
   {
      D_error(r);
      return(NULL);
   }

   data = (float *)datablock.data;

/**************************************************
*  "data" is the pointer to the initial trace in  *
*  the block of spectra stored in the memory      *
*  buffer.                                        *
**************************************************/

   return(data + (ctrace - 1 - c_first)*pointsperspec);
}


/*---------------------------------------
|					|
|		  bc()/4		|
|					|
|   Main entry point for BC routines.	|
|					|
+--------------------------------------*/
int bc(int argc, char *argv[], int retc, char *retv[])
{
   int		order = 0,
		numpts,
		minpts,
		minbcpts,
		dtrace,
		r,
		startspec_no=0,
		endspec_no=0,
		datatype,
		phaseflag;
   int          altmode = 0;
 
   float	*data;
   float        smooth = 10000.0;
   double	coef[MAXORDER + 1];
   struct base	*baseline = NULL;
   int scale = 200; // scale and smooth are parameters for 'alt'.
   int disflag=0; // 1 for bc('dis'), 2 for bc('all')

   Wturnoff_buttons();
   if ( argc > 1 && !strcmp(argv[1],"apply") ) {
        if(argc > 2) bc_apply(argv[2]);
	else bc_apply("base");
        return(COMPLETE);
   }

   // skip "dis" and "all"
   if ( argc > 1 && !strcmp(argv[1],"dis") ) {
	disflag=1;
	argc--; 
	argv++;
   } else if ( argc > 1 && !strcmp(argv[1],"all") ) {
	disflag=2;
	argc--; 
	argv++;
   } else if ( argc > 1 && !strcmp(argv[1],"cancel") ) {
	clearBcdata();
        setS_BCbit(D_PHASFILE, 0);
        execString("ds('again')\n");
        return(COMPLETE);
   } else if ( argc > 1 && !strcmp(argv[1],"off") ) {
	clearBcdata();
        execString("ds('again')\n");
        return(COMPLETE);
   } 

   // clear bcdata if not bc('dis')
   if (disflag != 1 ) {
	clearBcdata();
   }

   initBCparam(&altmode, &scale, &smooth, &order);
 
/***************************************
*  Get input and setup parameters for  *
*  for baseline correction.            *
***************************************/

   if ( (argc >= 2) && ! strcmp(argv[1],"alt") )
   {
      altmode = 1;
      if ( (argc >= 3) && isReal(argv[2]))
        scale = (int) stringReal(argv[2]);
      if ( (argc >= 4) && isReal(argv[3]))
        smooth = stringReal(argv[3]);
   }

   if(altmode) {
      numpts = fn/2;
      if (scale < 1) scale = 1;
      if (smooth < 0.05) smooth = 0.05;
      else if (smooth > 100000.0) smooth = 100000.0;
   } else {
      if (bc_input(argc, argv, &order, &numpts, &minpts, &minbcpts,
		&startspec_no, &endspec_no))
      {
         if (order == 0)
         {
            if (retc > 0)
            {
               retv[0] = intString(2);
            }
            return(COMPLETE);
         }
         else
         {
            if (retc > 0)
            {
               retv[0] = intString(FALSE);
               return(COMPLETE);
            }
            else
               return(ERROR);
         }
      }
      if ((baseline = getfunction(&numpts, minpts, minbcpts, order)) == NULL)
      {
         if (retc > 0)
         {
            retv[0] = intString(FALSE);
            return(COMPLETE);
         }
         else
         return ERROR;
      }
   }

   if (disflag == 1 ) {
      dis_bcdata(altmode, order, numpts, scale, smooth, baseline);
      return(COMPLETE);
   } else if (disflag == 2 ) {
      bc_all(altmode, order, numpts, scale, smooth, baseline);
      return(COMPLETE);
   }

   datatype = getdatatype(datahead.status);

/*************************************************
*  Operations for normal 1D baseline correction  *
*  on a 1D spectrum or a selected 2D trace.      *
*************************************************/

   if (!bc2dflag)
   { /* get spectral data and return pointer to these data */
      disp_status("BC");
      if ((data = getdata_4bc()) == NULL)
      {
         freebuffers(FALSE);
         disp_status("  ");
         if (retc > 0)
         {
            retv[0] = intString(FALSE);
            return(COMPLETE);
         }
         else
         return ERROR;
      }

      dtrace = currentindex() - 1;
      phaseflag = ( ((fabs(rp) > MINDEGREE) || (fabs(lp) > MINDEGREE)) &&
			(datatype != REAL) );
      if ( (!phaseflag) && (datatype == HYPERCOMPLEX) )
         phaseflag = ((fabs(rp1) > MINDEGREE) || (fabs(lp1) > MINDEGREE));

/************************************************
*  Phase-rotate the spectral data if necessary  *
*  prior to the BC operation.                   *
************************************************/

      if (phaseflag)
      {
         if (datatype == HYPERCOMPLEX)
         {
            rotate4(data, fn/2, lp, rp, lp1, rp1, fn1/2, dtrace, revflag);
         }
         else
         {
            rotate2(data, fn/2, lp, rp);
         }
      }

/*********************************
*  Perform actual bc operation.  *
*********************************/

      if (altmode)
      {
           if (scale > fn/16)
              scale = fn/16;
           BaseLineCorrection(numpts, data, datatype, scale, smooth);
      }
      else if ((order == 1) && (!unbcflag))
      { /* Perform spline fit for order = 1 unless it is an un-bc task */
           fitdc(baseline, data, numpts, datatype);
      }
      else
      { /* perform polynomial fit for order > 1; default for "unbc" */
         if (fitdata(baseline, data, coef, order, numpts, datatype))
         {
            disp_status("  ");
            freebuffers(FALSE);
            if (retc > 0)
            {
               retv[0] = intString(FALSE);
               return(COMPLETE);
            }
            else
            return ERROR;
         }
      }

/**************************************************
*  Reverse phase-rotation operation if the data   *
*  were phase-rotated prior to the BC operation.  *
**************************************************/

      if (phaseflag)
      {
         if (datatype == HYPERCOMPLEX)
         {
            rotate4(data, fn/2, -lp, -rp, -lp1, -rp1, fn1/2, dtrace, revflag);
         }
         else
         {
            rotate2(data, fn/2, -lp, -rp);
         }
      }

      disp_status("  ");
   }

/**************************************************************
*  Data are phase-rotated prior to the BC operation unless    *
*  the data were either acquired or stored pre-phased.  The   *
*  BC operation will only work effectively if the real part   *
*  of the spectrum submitted for bc corresponds to the        *
*  ABSORPTIVE part of the spectrum.                           *
*                                                             *
*  Data are unphase-rotated following the BC operation if     *
*  they were phase-rotated prior thereto.  In this way, the   *
*  phase-rotating operation effected by the display routines  *
*  will produce the true baseline corrected spectrum.  This   *
*  is redundant but is necessary to maintain the Varian-      *
*  mandated separation of raw data and displayed data.        *
**************************************************************/

   else
   {
      if (fit2ddata(baseline, coef, order, numpts, startspec_no, endspec_no))
      {
         freebuffers(FALSE);
         disp_status("        ");
         disp_index(0);
         if (retc > 0)
         {
            retv[0] = intString(FALSE);
            return(COMPLETE);
         }
         else
         return ERROR;
      }
   }

/*****************************************************
*  Memory buffer cleanup operations for 1D BC only.  *
*****************************************************/

   if (freebuffers(!bc2dflag))
   {
      if (retc > 0)
      {
         retv[0] = intString(FALSE);
         return(COMPLETE);
      }
      else
         return ERROR;
   }

/*****************************************************
*  Reset "trace" parameter to its original value     *
*  and reinitialize data structures and parameters.  *
*****************************************************/

   if (bc2dflag)
   {
      if ( (r = P_setstring(CURRENT, "trace", oldtrace, 0)) )
      {
         P_err(r, "trace", ":");
         if (retc > 0)
         {
            retv[0] = intString(FALSE);
            return(COMPLETE);
         }
         else
         return ERROR;
      }

      if (init2d(1, 1))
      {
         disp_status("        ");
         if (retc > 0)
         {
            retv[0] = intString(FALSE);
            return(COMPLETE);
         }
         else
         return ERROR;
      }
   }

/************************************************
*  Reset lvl and tlt to zero at the end of the  *
*  BC operation.                                *
************************************************/

   if ( (r = P_setreal(CURRENT, "lvl", 0.0, 0)) )	/* reset lvl to zero */
   {
      P_err(r, "lvl", ":");
      if (retc > 0)
      {
         retv[0] = intString(FALSE);
         return(COMPLETE);
      }
      else
      return ERROR;
   }

   if ( (r = P_setreal(CURRENT, "tlt", 0.0, 0)) )	/* reset tlt to zero */
   {
      P_err(r, "tlt", ":");
      if (retc > 0)
      {
         retv[0] = intString(FALSE);
         return(COMPLETE);
      }
      else
      return ERROR;
   }

   releasevarlist();
   if (!bc2dflag)
   {
      appendvarlist("lvl, tlt");
   }
   else if (!Bnmr)
   {
      appendvarlist("dcon");
      Wsetgraphicsdisplay("dconi");
   }

   if (retc > 0)
   {
      retv[0] = intString(TRUE);
   }
   return COMPLETE;

}

// set D_PHASFILE header status S_BC bit, b=1 if spectrum is correct,b=2 if not.
void setS_BCbit(int fileindex, int b) {
   dfilehead datahead;
   if(!D_gethead(fileindex, &datahead)) {
      if(b) datahead.status |= S_BC;
      else datahead.status &= S_BC;
      D_updatehead(fileindex, &datahead);	
   }
}

int checkS_BCbit(int fileindex) {
   dfilehead datahead;
   if(!D_gethead(fileindex, &datahead) && (datahead.status & S_BC)) return(1);
   else return(0);
}

// called to end baseline display.
void clearBcdata() {
   if(bcdata != NULL) releaseWithId("bcdata");
   bcdata = NULL;
   bctrace = 0;
   bcnpts = 0;
}

// numpts=fn/2 if altmode=1
// numpts=bcpts if altmode=0
void calc_bcdata(int trace, int altmode, int order, int numpts, int scale, float smooth, struct base  baseline[]) {

    float *spectrum; 
    int i, updateflag = 0;
    double	coef[MAXORDER + 1];
    int datasize = fn/2;

    if ((spectrum = calc_spec(trace,0,FALSE,TRUE,&updateflag))==0)
    return;

    // clear and reallocate bcdata
    clearBcdata();
    bcnpts = datasize;
    bctrace = currentindex();
    bcdata = (float *) allocateWithId( datasize * sizeof(float), "bcdata");
    memcpy(bcdata, spectrum, datasize * sizeof(float) );
    if (altmode)
    {
           BaseLineCorrection(numpts, bcdata, 1, scale, smooth);
    }
      else if ((order > 1))
      { /* perform polynomial fit for order > 1; default for "unbc" */
         if (fitdata(baseline, bcdata, coef, order, numpts, 1))
         {
            disp_status("  ");
            freebuffers(FALSE);
            return;
	 }
    }
    else
    { /* Perform spline fit for order = 1 unless it is an un-bc task */
           fitdc(baseline, bcdata, numpts, 1);
    }

      // now bcdata is corrected spectrum. the difference is the baseline.       
    for(i=0;i<datasize;i++)
    {
        bcdata[i] = *spectrum - bcdata[i];
        spectrum++;
    }

    freebuffers(FALSE);
}

void initBCparam2(int *bcpts, int *minpts, int *minbcpts) {
   if (fn < 2000)
   {
      *bcpts = 20;
   }
   else if (fn < 5000)
   {
      *bcpts = 40;
   }
   else
   {
      *bcpts = 80;
   }
   *minpts = fn / 1000;
   if (*minpts < 4)
      *minpts = 4;
   *minbcpts = 1;
}

void initBCparam(int *altmode, int *scale, float *smooth, int *order) {
   // init order and altmode with parameter bcmode
   // bcmode[1] = 0,1,or 2 for 'alt', spline or polynomial
   // bcmode[2] is scale for 'alt'
   // bcmode[3] is smooth for 'alt'
   // bcmode[4] is polynomial order 

   double d;
   int mode;
   // init as spline
   *altmode = 0;
   *scale = 200;
   *smooth = 10000.0;
   *order = 1; 
   if ( !P_getreal(CURRENT, "bcmode", &d, 1) ) mode = (int)d;
   else mode = 1;

   if ( !P_getreal(CURRENT, "bcmode", &d, 2) ) *scale = (int)d;
   if ( !P_getreal(CURRENT, "bcmode", &d, 3) ) *smooth = (float)d;
   if (*scale < 1) *scale = 1;
   if (*smooth < 0.05) *smooth = 0.05;
   if (*smooth > 100000.0) *smooth = 100000.0;

   if(mode == 0) { // 'alt' 
        *altmode = 1;
        *order = 0;
   } else if(mode > 1) { // polynomial
        if(!P_getreal(CURRENT, "bcmode", &d, 4) ) *order = (int)d;
   } 
}

// TODO: may get baseline data from a fdf file.
// currently return whatever is in bcdata. 
// to make sure bcdata is up to date, call bc('dis') first.
float *getBcdata(int trace, int numpts, int update)
{
   if(update || (bcdata != NULL && (trace != bctrace || numpts != bcnpts))) {
      int altmode, order, scale, bcpts, minpts, minbcpts;
      float smooth;
      struct base  *baseline = NULL;
      initBCparam(&altmode, &scale, &smooth, &order);
      if(altmode) {
        calc_bcdata(trace, altmode, order, numpts, scale, smooth, baseline);
      } else {
        initBCparam2(&bcpts, &minpts, &minbcpts);
        if ((baseline = getfunction(&bcpts, minpts, minbcpts, order)) != NULL) { 
          calc_bcdata(trace, altmode, order, bcpts, scale, smooth, baseline);
        } else {
	  altmode=1;
          calc_bcdata(trace, altmode, order, numpts, scale, smooth, baseline);
	}
      }
   }
 
   return bcdata;
}

void dis_bcdata(int altmode, int order, int numpts, int scale, float smooth, struct base  baseline[]) {
   int trace = currentindex()-1;
   calc_bcdata(trace, altmode, order, numpts, scale, smooth, baseline);
   execString("ds('again')\n");
}

void bc_apply(char *key) {
    float *spectrum, *bcdataptr; 
    int i, r, updateflag = 0;
    int ind = 0; 
    int datasize = fn/2;
    int numtraces=nblocks * specperblock;

    disp_status("BC");

    while(ind < numtraces && (spectrum = calc_spec(ind,0,FALSE,TRUE,&updateflag)) != NULL) { 
      // the difference is the baseline.       
      bcdataptr = aipGetTrace(key, ind, 1.0, datasize);
      for(i=0;i<datasize;i++)
      {
        *spectrum = *spectrum - *bcdataptr;
	bcdataptr++;
        spectrum++;
      }
      ind++;
    }
  
    setS_BCbit(D_PHASFILE, 1);

/************************************************
*  Reset lvl and tlt to zero at the end of the  *
*  BC operation.                                *
************************************************/

   if ( (r = P_setreal(CURRENT, "lvl", 0.0, 0)) )	/* reset lvl to zero */
   {
      P_err(r, "lvl", ":");
   }

   if ( (r = P_setreal(CURRENT, "tlt", 0.0, 0)) )	/* reset tlt to zero */
   {
      P_err(r, "tlt", ":");
   }

   releasevarlist();
   appendvarlist("lvl, tlt");
}

// this will also write out a fdf file for the baselines
void bc_all(int altmode, int order, int numpts, int scale, float smooth, struct base  baseline[]) {

    float *spectrum, *bcdataptr; 
    int i, r, updateflag = 0;
    double	coef[MAXORDER + 1];
    int ind = 0; 
    int datasize = fn/2;
    int numtraces=nblocks * specperblock;
    char datdirPath[MAXSTR];

    disp_status("BC");
    clearBcdata();
    bcdata = (float *) allocateWithId( datasize * numtraces * sizeof(float), "bcdata");
    if(bcdata == 0) {
	Werrprintf("cannot allocate bcdata buffer");
        return;
    }
    bcdataptr = bcdata;
    
    while(ind < numtraces && (spectrum = calc_spec(ind,0,FALSE,TRUE,&updateflag)) != NULL) { 
      memcpy(bcdataptr, spectrum, datasize * sizeof(float) );
      if (altmode)
      {
           BaseLineCorrection(numpts, spectrum, 1, scale, smooth);
      }
      else if ((order > 1))
      { /* perform polynomial fit for order > 1; default for "unbc" */
         if (fitdata(baseline, spectrum, coef, order, numpts, 1))
         {
            disp_status("  ");
            freebuffers(FALSE);
    	    clearBcdata();
            return;
	 }
      }
      else
      { /* Perform spline fit for order = 1 unless it is an un-bc task */
           fitdc(baseline, spectrum, numpts, 1);
      }

      // the difference is the baseline.       
      for(i=0;i<datasize;i++)
      {
        *bcdataptr = *bcdataptr - *spectrum;
	bcdataptr++;
        spectrum++;
      }
      ind++;
    }
  
    setS_BCbit(D_PHASFILE, 1);

    numtraces=ind;

// write fdf files to datdir. 
    sprintf(datdirPath, "%s/datdir/bc.fdf",curexpdir);
    writeFDFSpecfile(datdirPath, "baseline", bcdata, numtraces, datasize);
  
    clearBcdata();
    freebuffers(FALSE);

/************************************************
*  Reset lvl and tlt to zero at the end of the  *
*  BC operation.                                *
************************************************/

   if ( (r = P_setreal(CURRENT, "lvl", 0.0, 0)) )	/* reset lvl to zero */
   {
      P_err(r, "lvl", ":");
   }

   if ( (r = P_setreal(CURRENT, "tlt", 0.0, 0)) )	/* reset tlt to zero */
   {
      P_err(r, "tlt", ":");
   }

   releasevarlist();
   appendvarlist("lvl, tlt");
}

// model is weight spectral data (non-zero only if mask=1)
// so non zero points in model are baseline points to be fitted.
// npts is # of spectral data points
// bcpts is # of baseline points (non zero points in model) 
int getXYpoints(float *model, float *mask, int npts, double *xv, double *xp, double *yv, int bcpts, int order) {
   double midpoint = (double)npts/2.0;
   double ptscale = midpoint/RANGE;
   int i,k;

   k=1;
   for(i=0; i<npts; i++) {
       if(mask[i]==0) continue;
       xval[k]=i;
       yval[k]=model[i];
       if(order > 1) xval[k]= (xval[k] - midpoint)/ptscale;
       k++;
   }
    
   k--;
   if(k != bcpts) {
	Winfoprintf("Error: baseline points not match %d %d", k, bcpts); 
        return 0;
   }

   if(order > 1) {
      k /= 2;
      for(i=1; i<=k; i++) {
	xpos[i]=cos(M_PI * ((double)i-0.5)/(double)k);
      }
   }
   
   return 1;
}

int calcBCFit(float *data, float *model, float *mask, int npts, int bcpts) {
      int i, altmode, order, scale ;
      float smooth;
      double      coef[MAXORDER + 1];

      if(!bcpts) return 0;

      initBCparam(&altmode, &scale, &smooth, &order);
      if(altmode) return 0;

  if ((xval =
    (double *) allocateWithId(sizeof(double) * (bcpts + 1),"bc4"))==0)
  {
    Werrprintf("cannot allocate buffer");
    disp_status("        ");
    return(0);
  }


  if ((xpos =
    (double *) allocateWithId(sizeof(double) * (bcpts/2 + 1),"bc4"))==0)
  {
    Werrprintf("cannot allocate buffer");
    disp_status("        ");
    release(xval);
    return(0);
  }

  if ((yval =
    (double *) allocateWithId(sizeof(double) * (bcpts + 1),"bc4"))==0)
  {
    Werrprintf("cannot allocate buffer");
    disp_status("        ");
    release(xval);
    release(xpos);
    return(0);
  }

      if(!getXYpoints(model,mask,npts,xval,xpos,yval,bcpts,order)) {
        releaseAllWithId("bc4");
        return(0); 
      }

      // overwrite model with data
      memcpy(model, data, npts * sizeof(float) );

      if ((order > 1))
      { // perform polynomial fit for order > 1;
         if (polyFit(model, 1, xval, xpos, yval, bcpts, coef, order)) 
         {
            disp_status("  ");
      	    releaseAllWithId("bc");
            return 0;
	 }
      } else
      { // Perform spline fit for order = 1
   	   splineFit(model, 1, xval, xpos, yval,bcpts);
      }

      for(i=0;i<npts;i++)
      {
        model[i] = data[i] - model[i];
      }

      releaseAllWithId("bc4");
      return 1;
}
