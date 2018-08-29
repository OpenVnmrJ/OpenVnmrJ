/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "allocate.h"
#include "data.h"
#include "ftpar.h"
#include "group.h"
#include "variables.h"
#include "vnmrsys.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"

/* #define DEBUG_DSP0		0 */
/* #define DEBUG_DSP		0 */
/* #define DEBUG_DSP2		0 */

#define ERROR			-1
#define COMPLETE		0
#define TRUE			1
#define	FALSE			0
#define MINDEGREE		0.005
#define AMAX			8

extern void rotate_fid(float *fidptr, double ph0, double ph1, int np, int datatype);

struct _dspar2Info
{
    double	*chargeup;	/* chargeup buffer */
    float	*databuffer;	/* temporary data storage */
    int		offset;		/* offset of data in databuffer */
    int		tshift;		/* tshift */
    int		fill_dsp;	/* dsp algorithm */
    int		show_fid;	/* show normal fid or downsampled fid in ft('noft') */
    int		fill_ph;
    int		fake_data;
    double	fake_alfa;
};
typedef struct _dspar2Info       dspar2Info;

static int asize;
static double afreq[AMAX];
static double awidth[AMAX];
static double aamp[AMAX];
static double aphase[AMAX];

static int readDFpar2(dsparInfo *, dspar2Info *);
static void fill_fakeoutput(dspar2Info *dspar2, int ncdpts, double *buffer);
int downsamp_initialize(int fn0, dsparInfo *dspar, dspar2Info *dspar2);
void ds_calcchargeup(double *dbuffer, double decfactor, int ntaps, int norm);
static void fill_fakeinput(dsparInfo *dspar, dspar2Info *dspar2,
                    int ncdpts, float *tmpdatabuffer);
static void fill_chargeup(float *buffer, int dataskip,
                   dsparInfo *dspar, dspar2Info *dspar2);
static void ds_filterfid(dsparInfo *dspar, float *buffer, double *data,
                         double *dfilter, int ncdatapts, int dataskip);
static void ds_filterfid_old(dsparInfo *dspar, float *data, double *buffer,
                      double *dfilter, int ncdatapts, int dataskip);
static void interp_phase(float *xpass, float *ypass);
static void interp2_phase(float *xpass, float *ypass);
static void interp3_phase(float *xpass, float *ypass);
static void interp4_phase(float *xpass, float *ypass);

/*-----------------------------------------------
|                                               |
|                  downsamp()/6                 |
|                                               |
+----------------------------------------------*/
int downsamp(dsparInfo *dspar, float *data, int ncdpts,
             int nclspts, int fn0, int realdata)
{
    int                  i,
			 ic,
			 databitc,
			 datatype;
    double		*dfilter,
			*buffer,
			 fpmult = 1;
    register float	*tmpdata, *tmpdatabuffer;
    register double	*tmpbuffer;
    dspar2Info		 dspar2;
    vInfo		 info;

#ifdef DEBUG_DSP2
    fprintf(stderr,"input data points:\n");
    for (i=0;i<30;i++)
      fprintf(stderr,"  %g\n",data[i]);
    fprintf(stderr,"  %g\n  %g  %g\n",data[2*ncdpts-2],data[2*ncdpts-1],data[2*ncdpts]);
#endif 

    if (downsamp_initialize(fn0 + 2*ncdpts, dspar, &dspar2) != 0)
    {
      dspar->dsflag = FALSE;
      releaseAllWithId("dspar2");
      return(ERROR);
    }

    if (fabs(dspar->dslsfrq) > MINDEGREE)
      rotate_fid(data, (double)0.0, dspar->dslsfrq, 2*(ncdpts-nclspts), COMPLEX);

    dfilter = dspar->filter;
    buffer = dspar->buffer;
    datatype = (realdata?REAL:COMPLEX);
    databitc = (realdata?2:1);
/* printf("datatype=%d databitc=%d ncdpts=%d finalnp=%d\n",datatype,databitc,ncdpts,dspar->finalnp); */

    tmpdatabuffer = dspar2.databuffer;
    for (i=0; i<dspar2.offset; i++)
	*tmpdatabuffer++ = 0.0;

/* data copy */
  if (dspar2.fake_data == 0)
  {
    tmpdatabuffer = dspar2.databuffer + dspar2.offset;
    tmpdata = data;
    for (i=0; i<2*ncdpts; i++)
	*tmpdatabuffer++ = *tmpdata++;

    for (i=0; i<dspar2.offset; i++)  /* pad the end with zeroes */
	*tmpdatabuffer++ = 0.0;
  }
  else
  {
    tmpdatabuffer = dspar2.databuffer + dspar2.offset;
    fill_fakeinput(dspar,&dspar2,ncdpts,tmpdatabuffer);
    fpmult = 1;
    if (!P_getVarInfo(CURRENT, "fpmult", &info))
    {
      if (info.active)
      {
        if (P_getreal(CURRENT, "fpmult", &fpmult, 1))
	{
          Werrprintf("warning: couldn't get parameter 'fpmult'");
	}
      }
    }
    else
      Werrprintf("warning: couldn't get parameter 'fpmult'");
  }

    tmpdatabuffer = dspar2.databuffer;
    fill_chargeup(tmpdatabuffer,datatype,dspar,&dspar2);

#ifdef DEBUG_DSP2
  tmpdatabuffer = dspar2.databuffer;
fprintf(stderr,"all input data:\n");
  for (i=0; i<240;)
  {
    fprintf(stderr,"%g  %g  %g  %g  %g  %g  %g  %g\n",
tmpdatabuffer[i+0],tmpdatabuffer[i+1],tmpdatabuffer[i+2],tmpdatabuffer[i+3],
tmpdatabuffer[i+4],tmpdatabuffer[i+5],tmpdatabuffer[i+6],tmpdatabuffer[i+7]);
    i += 8;
  }
#endif 

/************************************
*  Filter the real data points in   *
*  the time-domain FID.             *
************************************/

if (dspar2.show_fid < 2)
{
  if (dspar2.fake_data > 20)
    fill_fakeoutput(&dspar2,ncdpts,buffer);
  else
  {
    if (dspar2.fill_dsp == -3)
    {
      tmpdatabuffer = dspar2.databuffer + dspar2.offset;
      ds_filterfid_old(dspar,tmpdatabuffer,buffer,dfilter,databitc*ncdpts,datatype); /* data is input, buffer is output */
    }
    else
    {
      tmpdatabuffer = dspar2.databuffer;
      ds_filterfid(dspar,tmpdatabuffer,buffer,dfilter,databitc*ncdpts,datatype); /* data is input, buffer is output */
    }
    if (!realdata)
    {
    if (dspar2.fill_dsp == -3)
    {
      tmpdatabuffer = dspar2.databuffer + dspar2.offset;
      ds_filterfid_old(dspar,tmpdatabuffer+1,buffer+1,dfilter,databitc*ncdpts,datatype);
    }
    else
      ds_filterfid(dspar,tmpdatabuffer+1,buffer+1,dfilter,databitc*ncdpts,datatype);
    }
  }
}

if (dspar2.show_fid > 1)
{
    tmpdata = data;
    tmpdatabuffer = dspar2.databuffer;
    if (dspar2.fill_dsp == -3)
      tmpdatabuffer = dspar2.databuffer + dspar2.offset;
    if (dspar2.show_fid == 3) tmpdatabuffer++;
    for (i=0;i<datatype*ncdpts;i++)
    {
      *tmpdata = (float)(*tmpdatabuffer);
      tmpdatabuffer++;
      tmpdata++;
    }
}
else
{
    tmpdata = data;  /* initialize buffer */
    ic = databitc*ncdpts;
    for (i=0;i<ic;i++)
    {
      *tmpdata = 0.0;
      tmpdata++;
    }
    tmpbuffer = buffer;  /* copy data */
    tmpdata = data;
    ic = dspar->finalnp;
    if (ncdpts < ic) ic = ncdpts;
    ic = databitc*ic;
    if (dspar2.show_fid == 1) tmpbuffer++;
    for (i=0;i<ic*2;i++)
    {
      *tmpdata = (float)(*tmpbuffer);
      tmpbuffer++;
      tmpdata++;
    }
}
    if (dspar2.fake_data != 0)
    {
      tmpdata = data;
      *tmpdata++ *= fpmult;
      *tmpdata *= fpmult;
    }

/* printf("output data=%g %g",data[0],data[1]); */
fpmult = dspar2.fill_ph * (M_PI/180.0);
/* printf("  *exp(-iphi)=%g %g  mag=%g\n",(double)(data[0]*cos(fpmult)+data[1]*sin(fpmult)),
	(double)(-data[0]*sin(fpmult)+data[1]*cos(fpmult)),sqrt(data[0]*data[0]+data[1]*data[1]));
*/
#ifdef DEBUG_DSP2
    fprintf(stderr,"output data points:\n");
    for (i=0;i<30;i++)
      fprintf(stderr,"  %g\n",data[i]);
/*    fprintf(stderr,"buffer points:\n");
    for (i=0;i<30;i++)
      fprintf(stderr,"  %g\n",buffer[i]); */
#endif 

    releaseAllWithId("dspar2");
    return(COMPLETE);
}


int downsamp_initialize(int fn0, dsparInfo *dspar, dspar2Info *dspar2)
{
    int dscoeff;
/*
dspar2Info	dspar22, *dspar2;
    dspar2 = &dspar22;
*/

    if (readDFpar2(dspar,dspar2) != 0)
    {
      dspar2->fill_dsp = 5; /* new TBI algorithm */
      dspar2->fill_ph = 0;
      dspar2->show_fid = 0;
      dspar2->fake_data = 0;
      dspar2->fake_alfa = 0;
      dspar2->tshift = 1;
    }
    dscoeff = dspar->dscoeff;
    dspar2->offset = (dscoeff - 1) + 2 * dspar2->tshift;
    if (dspar2->fill_dsp != -3)
      dspar->lp = 0.0;

#ifdef DEBUG_DSP0
printf("downsamp_initialize: fn0=%d dsfiltfactor=%g dscoeff=%d tshift=%d offset=%d\n",
		fn0, dspar->dsfiltfactor, dscoeff, dspar2->tshift, dspar2->offset);
printf("  fill_dsp=%d fill_ph=%d show_fid=%d fake_data=%d dspar->lp=%g\n",
	dspar2->fill_dsp,dspar2->fill_ph,dspar2->show_fid,dspar2->fake_data,dspar->lp);
#endif 
    dspar2->chargeup = NULL;
    dspar2->databuffer = NULL;

/* allocate chargeup, define chargeup */
    dspar2->chargeup = (double *)allocateWithId(sizeof(double)*((dscoeff+1)/2),"dspar2");
    if (!dspar2->chargeup)
    {
#ifdef DEBUG_DSP0
      Werrprintf("Error allocating chargeup buffer");
#else 
      Werrprintf("Error allocating filter buffer");
#endif 
      return(-1);
    }
    ds_calcchargeup(dspar2->chargeup, dspar->dsfiltfactor, dscoeff, TRUE);

    dspar2->databuffer = (float *)allocateWithId(sizeof(float)*(fn0+dspar2->offset),"dspar2");
    if (!dspar2->databuffer)
    {
#ifdef DEBUG_DSP0
      Werrprintf("Error allocating temporary dsp buffer");
#else 
      Werrprintf("Error allocating digital filtering buffer");
#endif 
      return(-1);
    }

   return(0);
}


/*---------------------------------------
|                                       |
|           ds_digfilter()/4        |
|                                       |
+--------------------------------------*/
void ds_digfilter(double *dbuffer, double decfactor, int ntaps, int norm)
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

   if (norm)
   {
     tmpfilter = dbuffer;
     for (i=0;i<ntaps;i++)
       *tmpfilter++ /= sum;
   }
       
   *(dbuffer+ntaps) = sum;

#ifdef DEBUG_DSP
   fprintf(stderr,"filter points:\n");
   for (i=0;i<ntaps;i++)
     fprintf(stderr,"  %g\n",dbuffer[i]);
#endif 
}


/*---------------------------------------
|                                       |
|           ds_digfilter_new()/4            |
|                                       |
+--------------------------------------*/
void ds_digfilter_new(double *dbuffer, double decfactor, int ntaps, int norm)
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
			fsumi,
                        arg1,
                        argi,
			sixdblvl;
   int max;

   tmpfilter = dbuffer;
   tmpfilter2 = dbuffer+ntaps;
   sixdblvl = 1 / (decfactor) + 1.04 / ((double)ntaps);
   wc = M_PI *sixdblvl;
   sum = 0.0;
   nfullpts = ntaps;

   max=(ntaps+1)/2;
   for (i = 0; i < max; i++)         
   {
      j = i - max + 1;
      hd = ( j ? sin(wc * (double)j) / (M_PI * (double)j) :
		  sixdblvl );
      arg1 = (2 * M_PI) / (double) (nfullpts - 1);
      argi = arg1 * ((double)i);
      w = 0.42 - 0.5*cos(argi) + 0.08*cos(2 * argi);
      if (w < 1e-15) w=0;
      *tmpfilter = hd * w;
      *(--tmpfilter2) = *tmpfilter;
      sum += (*tmpfilter++);
      if (tmpfilter-1 != tmpfilter2)
        sum += (*tmpfilter2);
   }

   if (norm)
   {
     fsumi = 1/sum;
     tmpfilter = dbuffer;
     for (i=0;i<ntaps;i++)
       *tmpfilter++ *= fsumi;
   }

   *(dbuffer+ntaps) = sum;

#ifdef DEBUG_DSP
   fprintf(stderr,"filter points:\n");
   for (i=0;i<ntaps+1;i++)
     fprintf(stderr,"  %g\n",dbuffer[i]);
#endif 
}


/*---------------------------------------
|                                       |
|           ds_calcchargeup()/4         |
|                                       |
+--------------------------------------*/
void ds_calcchargeup(double *dbuffer, double decfactor, int ntaps, int norm)
{
   register int         i,
                        pts;
   register double      *tmpfilter;
   register double      val,
                        arg1,
                        argi;

   (void) decfactor;
   (void) norm;
   tmpfilter = dbuffer;
   pts = (ntaps / 2) + 1;
   arg1 = (double)((2.0 * M_PI) / ((double)(ntaps - 1)));

   for (i=0; i<(pts-1); i++)
   {
      argi = arg1 * ((double)i);
      val = 0.42 - 0.50*cos(argi) + 0.08*cos(2.0 * argi);
      if (val < 1e-15) val=0.0;
      *tmpfilter++ = (float) (val);
   }
#ifdef DEBUG_DSP
   fprintf(stderr,"chargeup points:\n");
   for (i=0;i<pts-1;i++)
     fprintf(stderr,"  %g\n",dbuffer[i]);
#endif 
}


/*---------------------------------------
|                                       |
|          fill_chargeup()/4            |
|                                       |
+---------------------------------------*/
static void fill_chargeup(float *buffer, int dataskip,
                   dsparInfo *dspar, dspar2Info *dspar2) 
{
   int			ntaps,
                        decfact;
   register int         i, k;
   int                  tshift, offset, fill_dsp;
   double		*tmpdfilter;
   float                *tmpbuffer,
                        *tmpbuffer2,
                        sum=0, sumi=1, ftmp;
   float                sumx, sumy,
                        sig, sigx, sigxx, sigy, sigxy, sigz, sigxz;
   double		*chargeup;

   ntaps = dspar->dscoeff;
   decfact = dspar->dsfactor;
   tmpbuffer = buffer;
   chargeup = dspar2->chargeup;
   tmpdfilter = chargeup;
   tshift = dspar2->tshift;
   offset = dspar2->offset;
   fill_dsp = dspar2->fill_dsp;

/* printf("calling fill_chargeup()\n"); */
  switch (fill_dsp)
  {
   case 0: default:
      k = tshift;
      tmpbuffer = buffer;       /* real */
      for (i = 0; i < k; i++)
      {
         *tmpbuffer = 0;
         tmpbuffer += dataskip;
      }
      sum = *(tmpbuffer + offset - dataskip * tshift);
      tmpdfilter = chargeup;
      k = ntaps/2;
      tmpbuffer = buffer + dataskip * tshift;
      for (i = 0; i < k; i++)
      {
         *tmpbuffer = sum * *tmpdfilter++;
         tmpbuffer += dataskip;
      }
      k = tshift;
      tmpbuffer = buffer;
      tmpbuffer++;              /* imag */
      for (i = 0; i < k; i++)
      {
         *tmpbuffer = 0;
         tmpbuffer += dataskip;
      }
      sumi = *(tmpbuffer + offset - dataskip * tshift);
      tmpdfilter = chargeup;
      k = ntaps/2;
      tmpbuffer = buffer + dataskip * tshift;
      tmpbuffer++;
      for (i = 0; i < k; i++)
      {
         *tmpbuffer = sumi * *tmpdfilter++;
         tmpbuffer += dataskip;
      }
      break;
   case -1: case -3:
/*      k = dataskip * (ntaps/2 + tshift); */
      k = offset;
      tmpbuffer = buffer;
      for (i = 0; i < k; i++)
      {
         *tmpbuffer = 0;
         tmpbuffer++;
      }
      tmpbuffer = buffer;
      sum = *(tmpbuffer + offset);
      sumi = *(tmpbuffer + 1 + offset);
      break;
   case 5: case 6:
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      if ( ! finite( (double) sumi) )
         sumi = 0.0;
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
#ifdef DEBUG_DSP
    printf("cos2p=%g sin2p=%g\n", sum, sumi);
#endif 
      tmpbuffer = buffer + 2 * (k + 1);
      if (fill_dsp==6) tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 7: case 8:
      k = tshift + ntaps/2;
      tmpdfilter = dspar->filter + (ntaps/2);
      tmpbuffer = buffer + 2 * k;
      sumx = 0.5 * *tmpdfilter * *tmpbuffer++;
      sumy = 0.5 * *tmpdfilter++ * *tmpbuffer++;
      for (i=1; i<ntaps/2-1; i++) 	/* not used for sum,sumi */
      {
	sumx += *tmpdfilter * *tmpbuffer++;
	sumy += *tmpdfilter++ * *tmpbuffer++;
      }
      dspar2->fill_ph *= 2.0;
      sum  = (float)cos((double)(dspar2->fill_ph*M_PI/180.0));
      sumi = (float)sin((double)(dspar2->fill_ph*M_PI/180.0));
      tmpbuffer = buffer + 2 * (k + 1);
      if (fill_dsp==8)
        tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         ftmp = *tmpbuffer; tmpbuffer++;
         *tmpbuffer2 = sum * (ftmp) + sumi * (*tmpbuffer);
         tmpbuffer2++;
         *tmpbuffer2 = sumi * (ftmp) - sum * (*tmpbuffer);
         tmpbuffer2 -= 3;
         tmpbuffer++;
      }
      break;
   case 9: case 10:
      tmpbuffer = buffer;
      for (i = 0; i < tshift; i++)
      {
         *tmpbuffer++ = 0;
         *tmpbuffer++ = 0;
      }
      k = tshift + ntaps/2;
      tmpdfilter = chargeup + ntaps/2 - 1;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
#ifdef DEBUG_DSP
    printf("cos2p=%g sin2p=%g\n", sum, sumi);
#endif 
      tmpbuffer = buffer + 2 * (k + 1);
      if (fill_dsp==10) tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = tshift; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1)) * *tmpdfilter;
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1)) * *tmpdfilter;
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
	 tmpdfilter--;
      }
      break;
   case 11: case 12:
      k = tshift + ntaps/2;
      tmpdfilter = dspar->filter + (ntaps/2);
      tmpbuffer = buffer + 2 * k;
      sumx=0;
      sumy=0;
      sumx = 0.5 * *tmpdfilter * *tmpbuffer++;
      sumy = 0.5 * *tmpdfilter++ * *tmpbuffer++;
/* printf("tmpb=%g %g\n",*tmpbuffer,*(tmpbuffer+1));
printf("tmpd=%g %g %g %g\n",(dspar->filter)[0],*tmpdfilter,(dspar->filter)[ntaps-1],(dspar->filter)[3*ntaps/4]);
*/
      for (i=1; i<ntaps/2-1; i++)
      {
	sumx += *tmpdfilter * *tmpbuffer++;
	sumy += *tmpdfilter++ * *tmpbuffer++;
      }
/* printf("sumx=%g sumy=%g\n",sumx,sumy); */
/*      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1)); */
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
      tmpbuffer = buffer + 2 * (k + 1);
      if (fill_dsp==12) tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 13:
      k = tshift + ntaps/2;
      tmpdfilter = dspar->filter + (ntaps/2);
      tmpbuffer = buffer + 2 * k;
/* printf("tmpd=%g %g %g\n", *(tmpdfilter-1),*tmpdfilter,*(tmpdfilter+1));
printf("tmpb=%g %g\n",*tmpbuffer,*(tmpbuffer+1));
*/
      sumx = 0.0;
      sumy = 0.0;
      for (i=0; i<ntaps/2-1; i++)
      {
	sumx += *tmpdfilter * *tmpbuffer++;
	sumy += *tmpdfilter++ * *tmpbuffer++;
      }
      sum=sumx; sumi=sumy;
/* printf("sumx0=%g sumy0=%g ",sumx,sumy); */
      tmpdfilter = dspar->filter + (ntaps/2) + 1;
      tmpbuffer = buffer + 2 * (k + 1);
      sumx = 0.0;
      sumy = 0.0;
      for (i=1; i<ntaps/2-1; i++)
      {
	sumx += *tmpdfilter * *tmpbuffer++;
	sumy += *tmpdfilter++ * *tmpbuffer++;
      }
/* printf("sumx1=%g sumy1=%g\n",sumx,sumy); */
      sumx=sum-sumx; sumy=sumi-sumy;
      dspar2->fill_ph = ((int) (180/M_PI*atan2(sumy,sumx) + 0.5));
/* printf("sumx0-1=%g sumy0-1=%g phase=%d\n",sumx,sumy,dspar2->fill_ph); */
      dspar2->fill_ph *= 2;
      sum  = (float)cos((double)(dspar2->fill_ph*M_PI/180.0));
      sumi = (float)sin((double)(dspar2->fill_ph*M_PI/180.0));
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         ftmp = *tmpbuffer; tmpbuffer++;
         *tmpbuffer2 = sum * (ftmp) + sumi * (*tmpbuffer);
         tmpbuffer2++;
         *tmpbuffer2 = sumi * (ftmp) - sum * (*tmpbuffer);
         tmpbuffer2 -= 3;
         tmpbuffer++;
      }
      break;
   case 15: /* calculate cos2p sin2p from if-then tree */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      sum=sumx; sumi=sumy;
      interp_phase(&sum, &sumi);
#ifdef DEBUG_DSP
    printf("cos2p=%g sin2p=%g\n", sum, sumi);
#endif 
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 16: /* calculate cos2p sin2p from arcsin bitmask */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      sum=sumx; sumi=sumy;
      interp2_phase(&sum, &sumi);
#ifdef DEBUG_DSP
    printf("cos2p=%g sin2p=%g\n", sum, sumi);
#endif 
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 17: /* calculate cos2p sin2p from approx 1/a */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      sum=sumx; sumi=sumy;
      interp3_phase(&sum, &sumi);
#ifdef DEBUG_DSP
    printf("cos2p=%g sin2p=%g\n", sum, sumi);
#endif 
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 18: /* calculate cos2p sin2p from approx 1/a, std float div */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      sum=sumx; sumi=sumy;
      interp4_phase(&sum, &sumi);
#ifdef DEBUG_DSP
    printf("cos2p=%g sin2p=%g\n", sum, sumi);
#endif 
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 21: case 22: case 23: case 24: /* linear fit */
/*      k = tshift + ntaps/2; */
      k = dspar2->offset / 2;
      tmpbuffer = buffer + 2 * k;
      sig=0; sigx=0; sigxx=0; sigy=0; sigxy=0; sigz=0; sigxz=0;
      for (i=0; i<4; i++)  /* not (i=2; i<6; i++) */
      {
         sig   +=  1;
         sigx  +=  (float)i;
         sigxx +=  (float)(i*i);
         sigy  +=  *(tmpbuffer + 2 * i);
         sigxy +=  *(tmpbuffer + 2 * i) * ((float)i);
         sigz  +=  *(tmpbuffer + 2 * i + 1);
         sigxz +=  *(tmpbuffer + 2 * i + 1) * ((float)i);
      }
      sumx = ((sigxx * sigy - sigx * sigxy) - 0 * (sig * sigxy - sigx * sigy));
      sumx /= (sig * sigxx - sigx * sigx);
      sumy = ((sigxx * sigz - sigx * sigxz) - 0 * (sig * sigxz - sigx * sigz));
      sumy /= (sig * sigxx - sigx * sigx);
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
      if (fill_dsp > 22)
      {
        *(tmpbuffer) = ((sigxx * sigy - sigx * sigxy) - 0 * (sig * sigxy - sigx * sigy));
        *(tmpbuffer) /= (sig * sigxx - sigx * sigx);
        *(tmpbuffer+1) = ((sigxx * sigz - sigx * sigxz) - 0 * (sig * sigxz - sigx * sigz));
        *(tmpbuffer+1) /= (sig * sigxx - sigx * sigx);
        *(tmpbuffer+2) = ((sigxx * sigy - sigx * sigxy) + 1 * (sig * sigxy - sigx * sigy));
        *(tmpbuffer+2) /= (sig * sigxx - sigx * sigx);
        *(tmpbuffer+3) = ((sigxx * sigz - sigx * sigxz) + 1 * (sig * sigxz - sigx * sigz));
        *(tmpbuffer+3) /= (sig * sigxx - sigx * sigx);
      }
      if (fill_dsp==22 || fill_dsp==24)
         tmpbuffer = buffer + 2 * k;
      else
         tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
  }
  if (fill_dsp > -1)
  {
    if (atan2((double)sumi, (double)sum))
      sig = (float)(180/M_PI*atan2((double)sumi, (double)sum));
    else
      sig = 0;
    if (sig < 0) sig = sig + 360;
    dspar2->fill_ph = sig / 2;
/*    printf("fill_chargeup: sumx=%g sumy=%g sum=%g sumi=%g\nphase angle %g  /2=%g\n", sumx,sumy,sum,sumi,sig,sig/2);
*/
  }
}


#define STSIZE 128
/* xpass, ypass;  x,y input; x=cos2p,y=sin2p output */
static void interp_phase(float *xpass, float *ypass)
{
  float sintable[STSIZE]; /* could declare as double or float */
  float x0, y0, xabs, yabs, xyabs, xsgn, ysgn, ytmp, flag45;
  int i, px, py;

/*	x0 = (double)(*xpass); y0 = (double)(*ypass); */
	x0 = *xpass; y0 = *ypass;
	/* standard value */
	*ypass = 1.0/(x0 * x0 + y0 * y0);
	*xpass = *ypass * (x0 * x0 - y0 * y0);
	*ypass = *ypass * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
/*	printf("    STANDARD: cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2); */

	/* double angle */
	xabs = x0 * x0 - y0 * y0;
	yabs = 2.0 * x0 * y0;
	xyabs = x0 * x0 + y0 * y0;
	x0 = xabs; y0 = yabs;

	for (i=0; i<STSIZE; i++)
	  sintable[i] = (float)(sin( 2.0 * M_PI * ((double)i)/((double)(4 * STSIZE)) ));

	px = 0; py = 0;
	if (y0 >= 0.0)
	{
	  yabs = y0; ysgn = 1;
	}
	else
	{
	  yabs = -y0; ysgn = -1;
	}
	if (x0 >= 0.0)
	{
	  xabs = x0; xsgn = 1;
	}
	else
	{
	  xabs = -x0; xsgn = -1;
	}

	if (yabs > xabs)
	{
	  flag45 = 2;
	  ytmp = yabs;
	  yabs = xabs;
	}
	else
	{
	  flag45 = 0;
	}

	  /* test on 90/64=1.4, assign 90/128, use flag45 to check octant */
	  if (yabs <= (xyabs * sintable[STSIZE/4])) /* lowest level is STSIZE/2, STSIZE/(STSIZE/2)=2 */
	  {
	    /* 2p=11.25=90/8, py = STSIZE/4-1; */
	    if (yabs <= (xyabs * sintable[8*STSIZE/64]))
	    {
	      if (yabs <= (xyabs * sintable[4*STSIZE/64]))
	      {
	        if (yabs <= (xyabs * sintable[2*STSIZE/64]))
	        {
	          if (yabs <= (xyabs * sintable[STSIZE/64]))
	          {
	            py = STSIZE/64-1; /* 2p=90/128 */
	          }
	          else
	          {
	            py = 2*STSIZE/64-1; /* 2p=3*90/128 */
	          }
	        }
	        else
	        {
	          if (yabs <= (xyabs * sintable[3*STSIZE/64]))
	          {
	            py = 3*STSIZE/64-1; /* 2p=5*90/128 */
	          }
	          else
	          {
	            py = 4*STSIZE/64-1; /* 2p=7*90/128 */
	          }
	        }
	      }
	      else
	      {
	        if (yabs <= (xyabs * sintable[6*STSIZE/64]))
	        {
	          if (yabs <= (xyabs * sintable[5*STSIZE/64]))
	          {
	            py = 5*STSIZE/64-1; /* 2p=9*90/128 */
	          }
	          else
	          {
	            py = 6*STSIZE/64-1; /* 2p=11*90/128 */
	          }
	        }
	        else
	        {
	          if (yabs <= (xyabs * sintable[7*STSIZE/64]))
	          {
	            py = 7*STSIZE/64-1; /* 2p=13*90/128 */
	          }
	          else
	          {
	            py = 8*STSIZE/64-1; /* 2p=15*90/128 */
	          }
	        }
	      }
	    }
	    else
	    {
	      if (yabs <= (xyabs * sintable[12*STSIZE/64]))
	      {
	        if (yabs <= (xyabs * sintable[10*STSIZE/64]))
	        {
	          if (yabs <= (xyabs * sintable[9*STSIZE/64]))
	          {
	            py = 9*STSIZE/64-1; /* 2p=17*90/128 */
	          }
	          else
	          {
	            py = 10*STSIZE/64-1; /* 2p=19*90/128 */
	          }
	        }
	        else
	        {
	          if (yabs <= (xyabs * sintable[11*STSIZE/64]))
	          {
	            py = 11*STSIZE/64-1; /* 2p=21*90/128 */
	          }
	          else
	          {
	            py = 12*STSIZE/64-1; /* 2p=23*90/128 */
	          }
	        }
	      }
	      else
	      {
	        if (yabs <= (xyabs * sintable[14*STSIZE/64]))
	        {
	          if (yabs <= (xyabs * sintable[13*STSIZE/64]))
	          {
	            py = 13*STSIZE/64-1; /* 2p=25*90/128 */
	          }
	          else
	          {
	            py = 14*STSIZE/64-1; /* 2p=27*90/128 */
	          }
	        }
	        else
	        {
	          if (yabs <= (xyabs * sintable[15*STSIZE/64]))
	          {
	            py = 15*STSIZE/64-1; /* 2p=29*90/128 */
	          }
	          else
	          {
	            py = 16*STSIZE/64-1; /* 2p=31*90/128 */
	          }
	        }
	      }
	    }
	  }
	  else
	  {
	    /* 2p=33.75=90*3/8, py = STSIZE/2-1; */ /* +32 */
	    if (yabs <= (xyabs * sintable[32+8*STSIZE/64]))
	    {
	      if (yabs <= (xyabs * sintable[32+4*STSIZE/64]))
	      {
	        if (yabs <= (xyabs * sintable[32+2*STSIZE/64]))
	        {
	          if (yabs <= (xyabs * sintable[32+STSIZE/64]))
	          {
	            py = 32+STSIZE/64-1; /* 2p=90/128 */
	          }
	          else
	          {
	            py = 32+2*STSIZE/64-1; /* 2p=3*90/128 */
	          }
	        }
	        else
	        {
	          if (yabs <= (xyabs * sintable[32+3*STSIZE/64]))
	          {
	            py = 32+3*STSIZE/64-1; /* 2p=5*90/128 */
	          }
	          else
	          {
	            py = 32+4*STSIZE/64-1; /* 2p=7*90/128 */
		  }
	        }
	      }
	      else
	      {
	        if (yabs <= (xyabs * sintable[32+6*STSIZE/64]))
	        {
	          if (yabs <= (xyabs * sintable[32+5*STSIZE/64]))
	          {
	            py = 32+5*STSIZE/64-1; /* 2p=9*90/128 */
	          }
	          else
	          {
	            py = 32+6*STSIZE/64-1; /* 2p=11*90/128 */
	          }
	        }
	        else
	        {
	          if (yabs <= (xyabs * sintable[32+7*STSIZE/64]))
	          {
	            py = 32+7*STSIZE/64-1; /* 2p=13*90/128 */
	          }
	          else
	          {
	            py = 32+8*STSIZE/64-1; /* 2p=15*90/128 */
	          }
	        }
	      }
	    }
	    else
	    {
	      if (yabs <= (xyabs * sintable[32+12*STSIZE/64]))
	      {
	        if (yabs <= (xyabs * sintable[32+10*STSIZE/64]))
	        {
	          if (yabs <= (xyabs * sintable[32+9*STSIZE/64]))
	          {
	            py = 32+9*STSIZE/64-1; /* 2p=17*90/128 */
	          }
	          else
	          {
	            py = 32+10*STSIZE/64-1; /* 2p=19*90/128 */
	          }
	        }
	        else
	        {
	          if (yabs <= (xyabs * sintable[32+11*STSIZE/64]))
	          {
	            py = 32+11*STSIZE/64-1; /* 2p=21*90/128 */
	          }
	          else
	          {
	            py = 32+12*STSIZE/64-1; /* 2p=23*90/128 */
	          }
	        }
	      }
	      else
	      {
	        if (yabs <= (xyabs * sintable[32+14*STSIZE/64]))
	        {
	          if (yabs <= (xyabs * sintable[32+13*STSIZE/64]))
	          {
	            py = 32+13*STSIZE/64-1; /* 2p=25*90/128 */
	          }
	          else
	          {
	            py = 32+14*STSIZE/64-1; /* 2p=27*90/128 */
	          }
	        }
	        else
	        {
	          if (yabs <= (xyabs * sintable[32+15*STSIZE/64]))
	          {
	            py = 32+15*STSIZE/64-1; /* 2p=29*90/128 */
	          }
	          else
	          {
	            py = 32+16*STSIZE/64-1; /* 2p=31*90/128 */
	          }
	        }
	      }
	    }
	  }

	if (flag45 > 1)
	{
	  py = STSIZE - py;
	  yabs = ytmp;
	}
	px = STSIZE - py;
/*	printf("xabs=%g yabs=%g xyabs=%g flag45=%g py=%d px=%d ", xabs,yabs,xyabs,flag45,py,px); */
	xabs = sintable[px];
	yabs = sintable[py];
	xabs *= xsgn;
	yabs *= ysgn;
	xyabs = 90.0*((double)py)/((double)STSIZE);
	if (xsgn < 0) xyabs = 180.0 - xyabs;
	if (ysgn < 0) xyabs = 360.0 - xyabs;
/*	printf("cos2p=%g sin2p=%g 2p=%g\n",xabs,yabs,xyabs); */
	*xpass = xabs; *ypass = yabs;
}

/* xpass, ypass;  x,y input; x=cos2p,y=sin2p output */
static void interp2_phase(float *xpass, float *ypass)
{
  float sintable[STSIZE]; /* could declare as double or float */
  float x0, y0, xabs, yabs, xyabs, xsgn, ysgn, ytmp, flag45, yy, aa;
  int i, px, py;

/*	x0 = (double)(*xpass); y0 = (double)(*ypass); */
	x0 = *xpass; y0 = *ypass;
	/* standard value */
	*ypass = 1.0/(x0 * x0 + y0 * y0);
	*xpass = *ypass * (x0 * x0 - y0 * y0);
	*ypass = *ypass * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
/*	printf("    STANDARD: cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2); */

	/* double angle */
	xabs = x0 * x0 - y0 * y0;
	yabs = 2.0 * x0 * y0;
	xyabs = x0 * x0 + y0 * y0;
	x0 = xabs; y0 = yabs;

	for (i=0; i<STSIZE; i++)
	  sintable[i] = (float)(sin( 2.0 * M_PI * ((double)i)/((double)(4 * STSIZE)) ));

	px = 0; py = 0;
	if (y0 >= 0.0)
	{
	  yabs = y0; ysgn = 1;
	}
	else
	{
	  yabs = -y0; ysgn = -1;
	}
	if (x0 >= 0.0)
	{
	  xabs = x0; xsgn = 1;
	}
	else
	{
	  xabs = -x0; xsgn = -1;
	}
	if (yabs > xabs)
	{
	  flag45 = 2;
	  ytmp = yabs;
	  yabs = xabs;
	}
	else
	{
	  flag45 = 0;
	}

	/* construct bitmask for py using approx to arcsin */
/*	printf("xyabs=%g yabs=%g",xyabs,yabs); */
	while (xyabs > 2) /* assume xyabs not less than 1 */
	{
	  xyabs *= 0.5;
	  yabs *= 0.5;
	}
/*	printf(" ... rescaled xyabs=%g yabs=%g\n",xyabs,yabs); */
	aa = xyabs * xyabs; ytmp = yabs * yabs;
/*	printf("aa=%g ytmp=%g ",aa,ytmp); */
	/* using 9th order gives 45+/-0.1 degrees */
/*	yy = yabs * (1.0*aa*aa*aa*aa + (1.0/2.0)*ytmp*((1.0/3.0)*aa*aa*aa + (3.0/4.0)*ytmp*((1.0/5.0)*aa*aa + (5.0/6.0)*ytmp*((1.0/7.0)*aa + (7.0/8.0)*ytmp*(1.0/9.0))))); */
	yy = (7.0/8.0)*ytmp*((1.0/9.0));
	yy = (5.0/6.0)*ytmp*((1.0/7.0)*aa + yy);
	yy = (3.0/4.0)*ytmp*((1.0/5.0)*aa*aa + yy);
	yy = (1.0/2.0)*ytmp*((1.0/3.0)*aa*aa*aa + yy);
	yy = yabs * (1.0*aa*aa*aa*aa + yy);
/*
	yy = 0.0972222*ytmp;
	yy = 0.833333*ytmp*(0.142857*aa + yy);
	yy = 0.75*ytmp*(0.2*aa*aa + yy);
	yy = 0.5*ytmp*(0.333333*aa*aa*aa + yy);
	yy = yabs * (aa*aa*aa*aa + yy);
*/
	ytmp = xyabs * xyabs * xyabs;
	aa = ytmp * ytmp * ytmp * M_PI / (2.0 * STSIZE);
/*	printf("yy=%g aa=%g aamax=%g\n", yy,aa,aa*STSIZE/2.0); */
	py=0;
	if (yy > (STSIZE/2.0) * aa)
	{
	  py +=  (STSIZE/2);
	  yy -=  (STSIZE/2.0) * aa;
	}
	if (yy > (STSIZE/4.0) * aa)
	{
	  py +=  (STSIZE/4);
	  yy -=  (STSIZE/4.0) * aa;
	}
	if (yy > (STSIZE/8.0) * aa)
	{
	  py +=  (STSIZE/8);
	  yy -=  (STSIZE/8.0) * aa;
	}
	if (yy > (STSIZE/16.0) * aa)
	{
	  py +=  (STSIZE/16);
	  yy -=  (STSIZE/16.0) * aa;
	}
	if (yy > (STSIZE/32.0) * aa)
	{
	  py +=  (STSIZE/32);
	  yy -=  (STSIZE/32.0) * aa;
	}
	if (yy > (STSIZE/64.0) * aa)
	{
	  py +=  (STSIZE/64);
	  yy -=  (STSIZE/64.0) * aa;
	}
	py += 1;

	if (flag45 > 1)
	{
	  py = STSIZE - py;
	  yabs = ytmp;
	}
	px = STSIZE - py;
/*	printf("xabs=%g yabs=%g xyabs=%g flag45=%g py=%d px=%d ", xabs,yabs,xyabs,flag45,py,px); */
	xabs = sintable[px];
	yabs = sintable[py];
	xabs *= xsgn;
	yabs *= ysgn;
	xyabs = 90.0*((double)py)/((double)STSIZE);
	if (xsgn < 0) xyabs = 180.0 - xyabs;
	if (ysgn < 0) xyabs = 360.0 - xyabs;
/*	printf("cos2p=%g sin2p=%g 2p=%g\n",xabs,yabs,xyabs); */
	*xpass = xabs; *ypass = yabs;
}

/* xpass, ypass;  x,y input; x=cos2p,y=sin2p output */
static void interp3_phase(float *xpass, float *ypass)
{
  float x0, y0, xyabs, xfactor, yy, aa, aa2, aa3;

/*	x0 = (double)(*xpass); y0 = (double)(*ypass); */
	x0 = *xpass; y0 = *ypass;
	/* standard value */
	*ypass = 1.0/(x0 * x0 + y0 * y0);
	*xpass = *ypass * (x0 * x0 - y0 * y0);
	*ypass = *ypass * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
/*	printf("    STANDARD: cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2); */

	/* approximate divide */
	xyabs = (x0 * x0 + y0 * y0);
	aa = xyabs;
/*	printf("aa=%g ",aa); */
	xfactor = 1;
	while (aa > 2)
	{
	  xfactor *= 0.5;
	  aa *= 0.5;
	}
	if (aa > 1.41421)
	{
	  xfactor *= 0.707109;
	  aa *= 0.707109; /* not exact 1/sqrt(2) */
	}
/*	printf("... reduced aa=%g xfactor=%g\n",aa,xfactor); */
	aa -= 1;
	aa2 = aa*aa;
	aa3 = aa*aa*aa;
	/* yy is approx of 1/(1+aa) */
/*	yy = 1 - aa + aa2 - aa3 + aa2*aa2 - aa2*aa3 + aa3*aa3 - aa2*aa2*aa3 + aa2*aa3*aa3 - aa3*aa3*aa3;
	yy += aa2*aa2*aa3*aa3 - aa2*aa3*aa3*aa3 + aa3*aa3*aa3*aa3;
*/
	yy = (-1 + aa);
	yy =  1 + aa * yy;
	yy = -1 + aa * yy;
	yy =  1 + aa * yy;
	yy = -1 + aa * yy;
	yy =  1 + aa * yy;
	yy = -1 + aa * yy;
	yy =  1 + aa * yy;
	yy = -1 + aa * yy;
	yy =  1 + aa * yy;
	yy = -1 + aa * yy;
	yy =  1 + aa * yy;

/*	printf("final yy=%g ",yy); */
	yy *= xfactor;
	*xpass = yy * (x0 * x0 - y0 * y0);
	*ypass = yy * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
/*	printf("cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2); */
}

/* xpass, ypass;  x,y input; x=cos2p,y=sin2p output */
static void interp4_phase(float *xpass, float *ypass)
{
  float x0, y0, xyabs, xfactor, yy, aa;

/*	x0 = (double)(*xpass); y0 = (double)(*ypass); */
	x0 = *xpass; y0 = *ypass;
	/* standard value */
	*ypass = 1.0/(x0 * x0 + y0 * y0);
	*xpass = *ypass * (x0 * x0 - y0 * y0);
	*ypass = *ypass * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
/*	printf("    STANDARD: cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2); */

	/* approximate divide */
	xyabs = (x0 * x0 + y0 * y0);
	aa = xyabs;
/*	printf("aa=%g ",aa); */
	xfactor = 1;
	while (aa >= 1)
	{
	  xfactor *= 0.5;
	  aa *= 0.5;
	}
/*	printf("... reduced aa=%g xfactor=%g\n",aa,xfactor); */
	aa = xyabs;
	yy = 1.0 * xfactor;
	yy = yy * (2.0 - (aa * yy));
	yy = yy * (2.0 - (aa * yy));
	yy = yy * (2.0 - (aa * yy));
	yy = yy * (2.0 - (aa * yy));
	yy = (yy * (1.0 - (aa * yy))) + yy;

	*xpass = yy * (x0 * x0 - y0 * y0);
	*ypass = yy * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
/*	printf("cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2); */
}

static void fill_fakeoutput(dspar2Info *dspar2, int ncdpts, double *buffer)
{
  double fake_alfa = dspar2->fake_alfa;
  int fake_data = dspar2->fake_data;
  int dsp_out_np = ncdpts;
  double ph0, ph1, ph2, ph3, ph4, phc, phs;
  register double *fdata;
  int i, j, ctval;

/* printf("fakeoutput: dscoeff=%d dsfactor=%d fake_data=%d dsp_out_np=%d\n",
	dspar->dscoeff, dspar->dsfactor, fake_data, dsp_out_np);
*/

  fdata = buffer;
  switch (fake_data)
  {
      case 21: case 22: default:
        ph1 = 1/((double)(dsp_out_np/2));
        ph2 = 0.5 * M_PI / ph1;
        ph2 /= 2;
        ph3 = 0.0065 / ph1;
        ph1 = 1/((double)(dsp_out_np/2));
/* printf("1/ph1=%g ph2=%g ph3=%g falfa=%g\n",1/ph1,2.0*ph2,ph3,fake_alfa); */
        ph0 = (fake_alfa) * ph1;
        ph4 = exp(-ph0 * ph3);
        phc = cos(ph2 * ph0); phs = sin(ph2 * ph0);
        *fdata = (float)( 100.0 * (phc + (phc * phc - phs * phs)) );
        *fdata *= ph4;
        if (fake_data==22) *fdata *= 0.5;
        fdata++;
        *fdata = (float)( 100.0 * (-phs + (2.0 * phs * phc)) );
        *fdata *= ph4;
        if (fake_data==22) *fdata *= 0.5;
        fdata++;
        for (i=1; i<dsp_out_np/2; i++)
        {
          ph0 = (((double)i) + fake_alfa) * ph1;
          ph4 = exp(-ph0 * ph3);
          phc = cos(ph2 * ph0); phs = sin(ph2 * ph0);
          *fdata = (float)( 100.0 * (phc + (phc * phc - phs * phs)) );
          *fdata *= ph4;
          fdata++;
          *fdata = (float)( 100.0 * (-phs + (2.0 * phs * phc)) );
          *fdata *= ph4;
          fdata++;
        }
        break;
      case 23:
        ctval = (int)fake_alfa;
        if (ctval < 0) ctval = 0;
        if (ctval > 0)
        {
          for (i=0; i<ctval; i++)
          {
            *fdata++ = 0.0;
            *fdata++ = 0.0;
          }
        }
        *fdata++ = 100.0;
        *fdata++ = 0.0;
        for (i=ctval+1; i<dsp_out_np/2; i++)
        {
          *fdata++ = 0.0;
          *fdata++ = 0.0;
        }
        break;
      case 24:
        ctval = (int)fake_alfa;
        if (ctval < 0) ctval = 0;
        ph1 = 1/((double)(dsp_out_np/2));
        ph2 = 0.5 * M_PI / ph1;
        ph2 /= 2;
        ph3 = 0.0065 / ph1;
        ph1 = 1/((double)(dsp_out_np/2));
/* printf("1/ph1=%g ph2=%g ph3=%g falfa=%g\n",1/ph1,2.0*ph2,ph3,fake_alfa); */
        if (ctval > 0)
        {
          for (i=0; i<ctval; i++)
          {
            ph0 = ((double)i) * ph1;
            ph4 = exp(-ph0 * ph3);
            phc = cos(ph2 * ph0); phs = sin(ph2 * ph0);
            *fdata = (float)( 100.0 * (phc + (phc * phc - phs * phs)) );
            *fdata *= ph4;
            fdata++;
            *fdata = (float)( 100.0 * (-phs + (2.0 * phs * phc)) );
            *fdata *= ph4;
            fdata++;
          }
        }
        ph0 = ((double)ctval) * ph1;
        ph4 = exp(-ph0 * ph3);
        phc = cos(ph2 * ph0); phs = sin(ph2 * ph0);
        *fdata = (float)( 50.0 * (phc + (phc * phc - phs * phs)) );
        *fdata *= ph4;
        fdata++;
        *fdata = (float)( 50.0 * (-phs + (2.0 * phs * phc)) );
        *fdata *= ph4;
        fdata++;
        for (i=ctval+1; i<dsp_out_np/2; i++)
        {
          ph0 = ((double)i) * ph1;
          ph4 = exp(-ph0 * ph3);
          phc = cos(ph2 * ph0); phs = sin(ph2 * ph0);
          *fdata = (float)( 100.0 * (phc + (phc * phc - phs * phs)) );
          *fdata *= ph4;
          fdata++;
          *fdata = (float)( 100.0 * (-phs + (2.0 * phs * phc)) );
          *fdata *= ph4;
          fdata++;
        }
        break;
      case 25:
        ph1 = 1/((double)(dsp_out_np/2));
	ph2 = -0.5;
        ph3 = 0.0065 / ph1;
	ph3 = 1/ph3;
	ph3 = 0.001;
/* printf("1/ph1=%g ph2=%g ph3=%g\n",1/ph1,ph2,ph3); */
        for (i=0; i<dsp_out_np/2; i++)
        {
          ph0 = ((double)(i-dsp_out_np/4)) * 2 * ph1;
	  ph4 = 100.0 / (ph3 * ph3 + (ph0 - ph2)*(ph0 - ph2));
	  *fdata++ = (float)(ph3 * ph4);
	  *fdata++ = -(float)((ph0 - ph2) * ph4);
        }
        break;
      case 26:
        ph1 = 1/((double)(dsp_out_np/2));
	ph2 = -0.25; /* -0.33 and 2*ph2 */
        ph3 = 0.0065 / ph1;
	ph3 = 1/ph3;
	ph3 = 0.001;
/* printf("1/ph1=%g ph2=%g ph3=%g\n",1/ph1,ph2,ph3); */
        for (i=0; i<dsp_out_np/2; i++)
        {
          ph0 = ((double)(i-dsp_out_np/4)) * 2 * ph1;
	  ph4 = 100.0 / (ph3 * ph3 + (ph0 - ph2)*(ph0 - ph2));
	  *fdata++ = (float)(ph3 * ph4);
	  *fdata++ = -(float)((ph0 - ph2) * ph4);
	  fdata -= 2;
	  ph4 = 100.0 / (ph3 * ph3 + (ph0 - 2.5*ph2)*(ph0 - 2.5*ph2));
	  *fdata++ += (float)(ph3 * ph4);
	  *fdata++ += -(float)((ph0 - 2.5*ph2) * ph4);
        }
        break;
      case 31:
        for (i=0; i<dsp_out_np; i++) *fdata++ = 0.0;
        for (j=0; j<asize; j++)  /* aphase[j] not used yet */
        {
	  fdata = buffer;
          ph1 = 1/((double)(dsp_out_np/2));
          ph2 = afreq[j] * M_PI / ph1;
          ph3 = awidth[j] / ph1;
          ph1 = 1/((double)(dsp_out_np/2));
	  aphase[j] *= (M_PI / 180);
          for (i=0; i<dsp_out_np/2; i++)
          {
            ph0 = (((double)i) + fake_alfa) * ph1;
            ph4 = exp(-ph0 * ph3);
            phc = cos(ph2 * ph0 + aphase[j]); phs = sin(ph2 * ph0 + aphase[j]);
            *fdata++ += ( aamp[j] * phc * ph4);
            *fdata++ += ( aamp[j] * phs * ph4);
          }
	}
	break;
  }
}


static void fill_fakeinput(dsparInfo *dspar, dspar2Info *dspar2,
                    int ncdpts, float *tmpdatabuffer)
{
  double fake_alfa = dspar2->fake_alfa;
  int fake_data = dspar2->fake_data;
  int npwords = 2 * ncdpts;
  int dsfactor = dspar->dsfactor;
  double ph0, ph1, ph2, ph3, ph4, phc, phs;
  register float *fdata;
  int i, j;

/* printf("fake: dscoeff=%d dsfactor=%d fake_data=%d npwords=%d\n",
	dspar->dscoeff, dspar->dsfactor, fake_data, npwords);
        ph1 = 1/((double)(npwords/2 / dsfactor));
printf("fake_alfa / np/2 = %g\n",(dspar2->fake_alfa * ph1));
*/

  fdata = tmpdatabuffer;
  switch (fake_data)
  {
    case -2:
        for (i=0; i<2*(dspar->dscoeff-1); i++)
        {
            *fdata++ = 0.0;
            *fdata++ = 0.0;
        }
        for (i=2*(dspar->dscoeff-1); i<npwords/2; i++)
        {
            *fdata++ = 100.0;
            *fdata++ = -100.0;
        }
        break;
    case -1: default: 	/* (case 21, 22, etc.) */
        for (i=0; i<npwords/2; i++)
        {
            *fdata++ = 100.0;
            *fdata++ = -100.0;
        }
        break;
    case 1:
/*
        ph1 = (double)(npwords/2)/10;
        ph2 = 1/((double)(npwords/2));
        ph3 = 0.01;
        for (i=0; i<npwords/2; i++)
        {
          ph0 = (((double)i) + fake_alfa) * ph2;
          *fdata++ = 100 * cos(ph1 * ph0 / M_PI) * exp(-ph0/ph3);
          *fdata++ = 100 * sin(ph1 * ph0 / M_PI) * exp(-ph0/ph3);
        }
*/
        ph1 = 1/((double)(npwords/2));
        ph2 = 800 * M_PI;
/*        ph2 = 2 * M_PI / (ph1 * 10 * dsfactor); */
        ph3 = 10 * ((double)dsfactor);
        for (i=0; i<npwords/2; i++)
        {
          ph0 = ((double)i) * ph1;
          *fdata++ = (float)( 100.0 * cos(ph2 * ph0) );
          *fdata++ = (float)( 100.0 * sin(ph2 * ph0) );
        }
        break;
    case 2:
        ph1 = 1/((double)(npwords/2));
        ph2 = 800 * M_PI;
/*        ph2 = 2 * M_PI / (ph1 * 10 * ((double)dsfactor)); */
        ph3 = 5 * ((double)dsfactor);
        for (i=0; i<npwords/2; i++)
        {
          ph0 = (((double)i) + fake_alfa) * ph1;
          ph4 = exp(-ph0 * ph3);
          *fdata++ = (float)( 100.0 * cos(ph2 * ph0) * ph4 );
          *fdata++ = (float)( 100.0 * sin(ph2 * ph0) * ph4 );
        }
        break;
    case 3:
        ph1 = 1/((double)(npwords/2 / dsfactor));
        ph2 = 0.5 * M_PI / ph1;
        ph2 /= 2;
        ph3 = 0.0065 / ph1;
        ph1 = 1/((double)(npwords/2));
/* printf("1/ph1=%g ph2=%g ph3=%g\n",1/ph1,2.0*ph2,ph3); */
        ph0 = (dspar2->fake_alfa) * ph1;
        ph4 = exp(-ph0 * ph3);
        phc = cos(ph2 * ph0); phs = sin(ph2 * ph0);
        *fdata = (float)( 100.0 * (phc + (phc * phc - phs * phs)) );
        *fdata *= ph4;
        fdata++;
        *fdata = (float)( 100.0 * (phs + (2.0 * phs * phc)) );
        *fdata *= ph4;
        fdata++;
        for (i=1; i<npwords/2; i++)
        {
          ph0 = (((double)i) + fake_alfa) * ph1;
          ph4 = exp(-ph0 * ph3);
          phc = cos(ph2 * ph0); phs = sin(ph2 * ph0);
/*          *fdata = (float)( 100.0 * cos(ph2 * ph0) );
          *fdata += (float)( 100.0 * cos(0.5 * ph2 * ph0) ); */
          *fdata = (float)( 100.0 * (phc + (phc * phc - phs * phs)) );
          *fdata *= ph4;
          fdata++;
/*          *fdata = (float)( 100.0 * sin(ph2 * ph0) );
          *fdata += (float)( 100.0 * sin(0.5 * ph2 * ph0) ); */
          *fdata = (float)( 100.0 * (phs + (2.0 * phs * phc)) );
          *fdata *= ph4;
          fdata++;
        }
        break;
    case 4:
        ph1 = 1/((double)(npwords/2 / dsfactor));
        ph2 = 0.5 * M_PI / ph1;
        ph2 /= 2;
        ph3 = 0.0065 / ph1;
        ph1 = 1/((double)(npwords/2));
/* printf("1/ph1=%g ph2=%g ph3=%g\n",1/ph1,2.0*ph2,ph3); */
        for (i=0; i<npwords/2; i++)
        {
          ph0 = (((double)i) + fake_alfa) * ph1;
          ph4 = exp(-ph0 * ph3);
          phc = cos(ph2 * ph0); phs = sin(ph2 * ph0);
          *fdata = (float)( 100.0 * (phc + (phc * phc - phs * phs)) );
          *fdata *= ph4;
          fdata++;
          *fdata = (float)( 100.0 * (-phs + (2.0 * phs * phc)) );
          *fdata *= ph4;
          fdata++;
        }
        break;
    case 5:
        ph1 = 1/((double)(npwords/2 / dsfactor));
        ph2 = 0.5 * M_PI / ph1;
        ph2 /= 2;
        ph3 = 0.0013 / ph1;
        ph1 = 1/((double)(npwords/2));
/* printf("1/ph1=%g ph2=%g ph3=%g\n",1/ph1,2.0*ph2,ph3); */
        for (i=0; i<npwords/2; i++)
        {
          ph0 = (((double)i) + fake_alfa) * ph1;
          ph4 = exp(-ph0 * ph3);
          phc = cos(ph2 * ph0); phs = sin(ph2 * ph0);
          *fdata = (float)( 100.0 * (phc + (phc * phc - phs * phs)) );
          *fdata *= ph4;
          fdata++;
          *fdata = (float)( 100.0 * (-phs + (2.0 * phs * phc)) );
          *fdata *= ph4;
          fdata++;
        }
        break;
    case 6:
        ph1 = 1/((double)(npwords/2 / dsfactor));
        ph2 = 0.5 * M_PI / ph1;
        ph2 /= 2;
        ph3 = 0.0026 / ph1;
        ph1 = 1/((double)(npwords/2));
/* printf("1/ph1=%g ph2=%g ph3=%g\n",1/ph1,2.0*ph2,ph3); */
        for (i=0; i<npwords/2; i++)
        {
          ph0 = (((double)i) + fake_alfa) * ph1;
          ph4 = exp(-ph0 * ph3);
          phc = cos(ph2 * ph0); phs = sin(ph2 * ph0);
          *fdata = (float)( 100.0 * (phc + (phc * phc - phs * phs)) );
          *fdata *= ph4;
          fdata++;
          *fdata = (float)( 100.0 * (-phs + (2.0 * phs * phc)) );
          *fdata *= ph4;
          fdata++;
        }
        break;
    case 11:
        for (i=0; i<npwords; i++) *fdata++ = 0.0;
        for (j=0; j<asize; j++)
        {
	  fdata = tmpdatabuffer;
          ph1 = 1/((double)(npwords/2 / dsfactor));
          ph2 = afreq[j] * M_PI / ph1;
          ph3 = awidth[j] / ph1;
          ph1 = 1/((double)(npwords/2));
          aphase[j] *= (M_PI / 180);
          for (i=0; i<npwords/2; i++)
          {
            ph0 = (((double)i) + fake_alfa) * ph1;
            ph4 = exp(-ph0 * ph3);
            phc = cos(ph2 * ph0 + aphase[j]); phs = sin(ph2 * ph0 + aphase[j]);
            *fdata++ += (float)( aamp[j] * phc * ph4);
            *fdata++ += (float)( aamp[j] * phs * ph4);
          }
	}
        break;
  }
/* printf("fakeinput: %g %g %g %g\n",tmpdatabuffer[0],tmpdatabuffer[1],tmpdatabuffer[2],tmpdatabuffer[3]); */
}


/*-----------------------------------------------
|                                               |
|                ds_filterfid_old()/6           |
|                                               |
+----------------------------------------------*/
static void ds_filterfid_old(dsparInfo *dspar, float *data, double *buffer,
                      double *dfilter, int ncdatapts, int dataskip)
{
   int			ntaps,
			decfact;
   register int         i,
                        j;
   register float       *tmpdata;
   register double	*tmpdfilter,
			*tmpbuffer,
                        sum;

   ntaps = dspar->dscoeff;
   decfact = dspar->dsfactor;
   tmpdata = data;
   tmpdfilter = dfilter;

/*   tmpbuffer = buffer;
   for (i = 0; i < dspar->finalnp/2; i++)
     *tmpbuffer++ = 0.0;
*/

   tmpbuffer = buffer;
   /* make sure "i" is set with all integer math! (Truncate all divides.) */
   for (i = ntaps/2-((ntaps/2)/decfact)*decfact; i < ntaps; i+=decfact)  {
     tmpdfilter = dfilter;
     tmpdata = data+dataskip*i;
     sum = 0.0;
     for (j=0;j<i+1;j++)  {
        sum += (*tmpdata) * (*(tmpdfilter++));
	tmpdata -= dataskip;
	}
     *tmpbuffer = sum;
     tmpbuffer += dataskip;
     }

   for (i = i; i < ncdatapts; i+=decfact)  {
     tmpdfilter=dfilter;
     tmpdata = data+dataskip*i;
     sum = 0.0;
     for (j=0;j<ntaps;j++)  {
        sum += (*tmpdata) * (*(tmpdfilter++));
	tmpdata -= dataskip;
	}
     *tmpbuffer = sum;
     tmpbuffer += dataskip;
     }
/* keeping the last ntaps-1/decfact points can sometimes be troublesome...
   if the FID doesn't decay (or lb) to zero, so don't do it unless
   dcrmv or dc is always done */
/*   for (i = i; i < ncdatapts + ntaps - 1; i+=decfact)  {
     tmpdfilter = dfilter+(i-ncdatapts)+1;
     tmpdata = data+dataskip*(ncdatapts-1);
     sum = 0.0;
     for (j=0;j<ntaps-(i-ncdatapts)-1;j++)  {
        sum += (*(tmpdata)) * (*(tmpdfilter++));
	tmpdata -= dataskip;
	}
     *tmpbuffer = sum;
     tmpbuffer += dataskip;
     num++;
     }*/
}


/*-----------------------------------------------
|                                               |
|                  ds_filterfid()/6             |
|                                               |
+----------------------------------------------*/
static void ds_filterfid(dsparInfo *dspar, float *buffer, double *data,
                         double *dfilter, int ncdatapts, int dataskip)
{
   int	 		i,
			ntaps,
                        decfact,
                        buffskip;
   register int         j, k;
   register float       *tmpbuffer;
   register double      *tmpdfilter,
                        *tmpdata;
   double               *tmpdfilter2;

   ntaps = dspar->dscoeff;
   decfact = dspar->dsfactor;
   tmpdata = data;
   tmpbuffer = buffer;
   tmpdfilter = dfilter;

   k = ncdatapts / decfact;
   i = ntaps;
   tmpbuffer = buffer + dataskip * i;
   tmpdfilter2 = dfilter;
   buffskip = dataskip * (ntaps + decfact);
   while (k--)
   {
     tmpdfilter=tmpdfilter2;
     *tmpdata = 0.0;
     j=ntaps;
     while (j--)
     {
        *tmpdata +=  *tmpbuffer  *  *tmpdfilter++;
        tmpbuffer -= dataskip;
     }
     tmpdata += dataskip;
/*     i += decfact; */
     tmpbuffer += buffskip;
   }
}


/*---------------------------------------
|                                       |
|       init_downsample_files()/1       |
|                                       |
+--------------------------------------*/
int init_downsample_files(ftparInfo *ftpar)
{
   char         path[MAXPATHL],
                filepath[MAXPATHL],
		proc_string[16],
                dp[16];
   int          r,
		res;
   double       sw,
		lsfrq,
		tmp_np,
		tmp_rfl,
		tmp_rfp;
   dfilehead	fidhead;
   vInfo	info;

   D_allrelease();

/* update the current acquisition parameters */
   if ( (r = P_copygroup(PROCESSED, CURRENT, G_ACQUISITION)) )
   {
      P_err(r, "ACQUISITION ", "copygroup:");
      return(ERROR);
   }

/* copy current parameters into temporary tree to be written out in new expt */
   P_treereset(TEMPORARY);
   P_copy(CURRENT,TEMPORARY);

/* change parameters np, sw, and dp; store parameters in other experiment */
   if ( (r = P_getreal(PROCESSED, "sw", &sw, 1)) )
   {
      P_err(r, "processed ", "sw: ");
      return(ERROR);
   }
   if ( (r = P_getreal(PROCESSED, "np", &tmp_np, 1)) )
   {
      P_err(r, "processed ", "np: ");
      return(ERROR);
   }

   if ( (r = P_getreal(TEMPORARY, "rfl", &tmp_rfl, 1)) )
   {
      P_err(r, "temporary ", "rfl: ");
      return(ERROR);
   }

   if ( (r = P_getreal(TEMPORARY, "rfp", &tmp_rfp, 1)) )
   {
      P_err(r, "temporary ", "rfp: ");
      return(ERROR);
   }

   if ( (r = P_getstring(TEMPORARY, "dp", dp, 1, 2)) )
   {
      P_err(r, "temporary ", "dp: ");
      return(ERROR);
   }
/* don't do this - lose dynamic range gained by DSP */
/*   if (dp[0] == 'n')
     ftpar->dspar.dp = FALSE;*/

   if ( (r = P_setreal(TEMPORARY, "sw", sw/ftpar->dspar.dsfactor, 1)) )
   {
      P_err(r, "temporary ", "sw: ");
      return(ERROR);
   }

   if ( (r = P_setreal(TEMPORARY, "np", (double)ftpar->dspar.finalnp, 1)) )
   {
      P_err(r, "temporary ", "np: ");
      return(ERROR);
   }

   if ( (r = P_setactive(TEMPORARY, "downsamp", ACT_OFF)) )
   {
      P_err(r, "temporary ", "downsamp: ");
      return(ERROR);
   }

   lsfrq=0;
   if (!P_getVarInfo(TEMPORARY, "dslsfrq", &info))
   {
     if (info.active)
     {
       if ( (r = P_getreal(TEMPORARY, "dslsfrq", &lsfrq, 1)) )
       {
         P_err(r, "temporary ", "dslsfrq: ");
         return(ERROR);
       }
     }
   }

   if ( (r = P_setreal(TEMPORARY,"rfl",tmp_rfl-(sw-sw/ftpar->dspar.dsfactor)/2.0+
					lsfrq, 1)) )
   {
      P_err(r, "temporary ", "rfl: ");
      return(ERROR);
   }

   P_setactive(TEMPORARY, "lsfid", ACT_OFF);
   P_setactive(TEMPORARY, "lb", ACT_OFF);
   P_setstring(TEMPORARY, "dcrmv", "n", 1);
   P_setactive(TEMPORARY, "gf", ACT_OFF);
   P_setactive(TEMPORARY, "gfs", ACT_OFF);
   P_setactive(TEMPORARY, "sb", ACT_OFF);
   P_setactive(TEMPORARY, "sbs", ACT_OFF);
   P_setactive(TEMPORARY, "awc", ACT_OFF);

   if ( (r = P_getstring(TEMPORARY, "proc", proc_string, 1, 10)) )
   {
     P_err(r, "temporary ", "proc: ");
     return(ERROR);
   }
   if (strncmp(proc_string,"lp",2) == 0)
   {
     if ( (r = P_setstring(TEMPORARY, "proc", "ft", 1)) )
     {
       P_err(r, "temporary ", "proc: ");
       return(ERROR);
     }
   }

/* setup parameters in target experiment */
   if ( (r = D_getparfilepath(CURRENT, path, ftpar->dspar.newpath)) )
   {
      D_error(r);
      return(ERROR);
   }
 
   if ( (r = P_save(TEMPORARY, path)) )
   {
      Werrprintf("problem storing parameters in %s", path);
      return(ERROR);
   }
 
   if ( (r = D_getparfilepath(PROCESSED, path, ftpar->dspar.newpath)) )
   {
      D_error(r);
      return(ERROR);
   }
 
   if ( (r = P_save(TEMPORARY, path)) )
   {
      Werrprintf("problem storing parameters in %s", path);
      return(ERROR);
   }

   P_treereset(TEMPORARY);
 
/* set the status of the phasefile in the target
   experiment to 0 */

   D_close(D_PHASFILE);

   if ( (r = D_getfilepath(D_PHASFILE, path, ftpar->dspar.newpath)) )
   {
      D_error(r);
      return(ERROR);
   }

   fidhead.status = 0;
   fidhead.vers_id = 0;
   fidhead.np = ftpar->fn0;
   fidhead.ntraces = 1;
   fidhead.nblocks = 1;
   fidhead.nbheaders = 1;
   fidhead.ebytes = sizeof(int);
   fidhead.tbytes = fidhead.np * sizeof(int);
   fidhead.bbytes = fidhead.nbheaders * sizeof(dblockhead) +
			fidhead.tbytes * fidhead.ntraces;

   if ( (r = D_newhead(D_PHASFILE, path, &fidhead)) )
   {
      D_error(r);
      return(ERROR);
   }

/* create new fid header in new experiment */

   D_close(D_USERFILE);

   if ( (res = D_getfilepath(D_USERFILE, path, curexpdir)) )
   {
      D_error(res);
      return(ERROR);
   }

   if ( (res = D_open(D_USERFILE, path, &fidhead)) )
   {
      D_error(res);
      return(ERROR);
   }

   D_close(D_USERFILE);

   /* if dp = 'y' use 32-bit floats in the file, otherwise 16-bit ints */
   fidhead.np = ftpar->dspar.finalnp;
   fidhead.ebytes = (ftpar->dspar.dp ? sizeof(float) : sizeof(short));
   fidhead.tbytes = fidhead.np * fidhead.ebytes;
   fidhead.bbytes = (fidhead.nbheaders & NBMASK) * sizeof(dblockhead) +
			fidhead.tbytes * fidhead.ntraces;
   if (ftpar->dspar.dp)
   {
     fidhead.status |= S_FLOAT;
     fidhead.status &= ~S_32;
   }

   if ( (res = D_getfilepath(D_USERFILE, filepath, ftpar->dspar.newpath)) )
   {
      D_error(res);
      return(ERROR);
   }
   if ( (r = D_newhead(D_USERFILE, filepath, &fidhead)) )
   {
      D_error(r);
      return(ERROR);
   }

   D_close(D_USERFILE);

   if ( (res = D_open(D_USERFILE, path, &fidhead)) )
   {
      D_error(res);
      return(ERROR);
   }
   
   strcpy(path,curexpdir);
   strcat(path,"/acqfil/log");
   if (!access(path,0))
   {
      strcpy(filepath,ftpar->dspar.newpath);
      strcat(filepath,"/acqfil/log");
      copy_file_verify(path,filepath);
   }
   strcpy(path,curexpdir);
   strcat(path,"/text");
   strcpy(filepath,ftpar->dspar.newpath);
   strcat(filepath,"/text");
   copy_file_verify(path,filepath);

   return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|               readDFcoefs()/0                 |
|						|
+----------------------------------------------*/
double *readDFcoefs(char *name, double decfactor, int *ntaps,
                    int min_ntaps, int max_ntaps)
{
   char		dfpath[MAXPATHL];
   int		i = 0,
		notempty = TRUE;
   double	*filter,
		*fptr,
		tmp,
		filtsum;
   FILE		*fdes;


   (void) decfactor;
   if ( strcmp(name, "") == 0 )
      return(NULL);
   if ( strcmp(name, "tbcfilter") == 0 )
      return(NULL);

   strcpy(dfpath, userdir);
   strcat(dfpath, "/filtlib/");
   strcat(dfpath, name);

   if ( (fdes = fopen(dfpath, "r")) == NULL )
   {
      Werrprintf("readDFcoefs():  cannot open DF file %s.", dfpath);
      return(NULL);
   }

   filtsum = 0.0;

   while ( notempty )
     if ( (notempty = ( fscanf(fdes, "%lf", &tmp) != EOF )) )
       i++;

   if (i < min_ntaps)
   {
      Werrprintf("readDFcoefs():  too few coefficients in file");
      fclose(fdes);
      return(NULL);
   }
   else if (i > max_ntaps)
   {
      Werrprintf("readDFcoefs():  too many coefficients in file");
      fclose(fdes);
      return(NULL);
   }
   else
   {
     rewind(fdes);
     if ((filter = (double *)allocateWithId(sizeof(double)*(i+1),"ft2d"))==NULL)
     {
       Werrprintf("readDFcoefs():  error allocating filter buffer");
       fclose(fdes);
       return(NULL);
     }
     notempty = TRUE;
     fptr = filter;
     while ( notempty )
     {
       *fptr = 0.0;
       if ( (notempty = ( fscanf(fdes, "%lf", fptr) != EOF)) )
	 filtsum += *fptr++;
     }
   }

   *fptr = filtsum;
   *ntaps = i;

   (void) fclose(fdes);

   return(filter);
}

/*-----------------------------------------------
|                                               |
|               readDFpar2()/0                  |
|						|
+----------------------------------------------*/
static int readDFpar2(dsparInfo  *dspar, dspar2Info  *dspar2)
{
   char		dfpath[256], jstr[256], filename[256];
   int		i = 0,
		j, iskip, itry = 0;
   int		fill_dsp, fill_ph, show_fid, fake_data, tshift;
   double	fake_alfa;
   float	fval, tmp;
   double	sw;
   vInfo        info;
   FILE		*fdes;

   filename[0] = '\0';

   if (!P_getVarInfo(CURRENT, "filtfile", &info))
     if (info.active)
       if (P_getstring(CURRENT, "filtfile", filename, 1, MAXPATHL))
         filename[0] = '\0';
/*
   if (!P_getVarInfo(PROCESSED, "filtfile", &info))
     if (info.active)
       if (P_getstring(PROCESSED, "filtfile", filename, 1, MAXPATHL))
         filename[0] = '\0';
*/

   if ( strcmp(filename, "" ) == 0)
      return(-1);
   strcpy(jstr,filename);
   if ( strcmp(jstr, "tbcfilter") != 0)
   {
#ifdef DEBUG_DSP0
      printf("readDF: using OLD fill_dsp\n");
#endif 
      return(-1);
   }
   if ( P_getreal(PROCESSED, "sw", &sw, 1) ) /* was done before, must be ok */
   {
      Werrprintf("cannot get sw for digital filtering");
      return(-1);
   }

   itry = 1;
   strcpy(dfpath, userdir);
   strcat(dfpath, "/filtlib/");
   strcat(dfpath, filename);

   if ( (fdes = fopen(dfpath, "r")) == NULL )
   {
      if (itry != 1)
         Werrprintf("readDFcoefs():  cannot open DF file %s", dfpath);
      return(-1);
   }
   if (itry == 1)
   {
      fill_dsp = -3;  /* Rosen algorithm */
      fill_ph = 0;
      show_fid = 0;
      fake_data = 0;
      fake_alfa = 0;
      tshift = 0;
      iskip = 0;
      while (iskip == 0)
      {
         if (fscanf(fdes, "%s", jstr) == EOF)
         {
            strcpy(jstr,"");
            iskip = 1;
         }
         if (strcmp(jstr,"filtertype")==0)
         {
            if (fscanf(fdes, "%s", jstr) == EOF)
            {
               strcpy(jstr,"");
               iskip = 1;
            }
            if ((strcmp(jstr,"phase")==0) || (strcmp(jstr,"phasea")==0) || (strcmp(jstr,"phaseb")==0))
            {
               fill_dsp = 7;
               if (strcmp(jstr,"phaseb")==0)
                  fill_dsp = 8;
               if (fscanf(fdes, "%s", jstr) == EOF)
                  iskip = 1;
               else
               {
                  /* if jstr is not a number, atof returns 0 */
                  fill_ph = atof(jstr);
               }
            }
            else if (strcmp(jstr,"try")==0)
            {
               fill_dsp = 0;
               if (fscanf(fdes, "%s", jstr) == EOF)
                  iskip = 1;
               else
               {
                  /* if jstr is not a number, atof returns 0 */
                  fval = atof(jstr);
                  if (fval < 0)
                     fill_dsp = (int)(fval-0.5);
                  else
                     fill_dsp = (int)(fval+0.5);
               }
            }
            else if ((strcmp(jstr,"alg1")==0) || (strcmp(jstr,"alg1a")==0))
               fill_dsp = 21;
            else if (strcmp(jstr,"alg1b")==0)
               fill_dsp = 22;
            else if ((strcmp(jstr,"alg2")==0) || (strcmp(jstr,"alg2a")==0))
               fill_dsp = 23;
            else if (strcmp(jstr,"alg2b")==0)
               fill_dsp = 24;
            else if ((strcmp(jstr,"alg0")==0) || (strcmp(jstr,"alg0a")==0))
               fill_dsp = 5;
            else if (strcmp(jstr,"alg0b")==0)
               fill_dsp = 6;
            else if ((strcmp(jstr,"alg3")==0) || (strcmp(jstr,"alg3a")==0))
               fill_dsp = 9;
            else if (strcmp(jstr,"alg3b")==0)
               fill_dsp = 10;
            else if ((strcmp(jstr,"alg4")==0) || (strcmp(jstr,"alg4a")==0))
               fill_dsp = 11;
            else if (strcmp(jstr,"alg4b")==0)
               fill_dsp = 12;
            else if (strcmp(jstr,"alg5")==0)
               fill_dsp = 13;
            else if (strcmp(jstr,"alg6")==0)
               fill_dsp = 15;
            else if (strcmp(jstr,"alg7")==0)
               fill_dsp = 16;
            else if (strcmp(jstr,"alg8")==0)
               fill_dsp = 17;
            else if (strcmp(jstr,"alg9")==0)
               fill_dsp = 18;
            else if ((strcmp(jstr,"chargeup")==0) || (strcmp(jstr,"wurl")==0))
               fill_dsp = 0;
            else if (strcmp(jstr,"zero")==0)
               fill_dsp = -1;
            else if (strcmp(jstr,"brutype")==0)
               fill_dsp = -2;
            else if (strcmp(jstr,"rosen")==0)
               fill_dsp = -3;
            else
               fill_dsp = 0;
         }
         else if (strcmp(jstr,"show")==0)
         {
            if (fscanf(fdes, "%s", jstr) == EOF)
            {
               strcpy(jstr,"");
               iskip = 1;
            }
            if ((strcmp(jstr,"dsreal")==0) || (strcmp(jstr,"dr")==0) || (strcmp(jstr,"d")==0))
               show_fid = 0;
            else if ((strcmp(jstr,"dsimag")==0) || (strcmp(jstr,"di")==0))
               show_fid = 1;
            else if ((strcmp(jstr,"osreal")==0) || (strcmp(jstr,"or")==0) || (strcmp(jstr,"o")==0))
               show_fid = 2;
            else if ((strcmp(jstr,"osimag")==0) || (strcmp(jstr,"oi")==0))
               show_fid = 3;
         }
         else if (strcmp(jstr,"lpshift")==0)
         {
            tshift = 0;
            if (fscanf(fdes, "%s", jstr) == EOF)
               iskip = 1;
            else
            {
               /* if jstr is not a number, atof returns 0 */
               fval = atof(jstr);
               if (fval < 0)
                  tshift = 0;
               else
                  tshift = (int)(fval+0.5);
            }
         }
         else if (strcmp(jstr,"lptime")==0)
         {
            tshift = 0;
            if (fscanf(fdes, "%s", jstr) == EOF)
               iskip = 1;
            else
            {
               /* if jstr is not a number, atof returns 0 */
               fval = atof(jstr);
               if (fval < 0)
                  tshift = 0;
               else
               {
                  tmp = (float) (dspar->dsfactor); /* fval in usec */
                  if (sw > 0.1) 
                     tshift = (int)(fval * 1.0e-6 * sw * tmp ); 
               }
            }
         }
         else if (strcmp(jstr,"data")==0)
         {
            if (fscanf(fdes, "%s", jstr) == EOF)
               iskip = 1;
            else
            {
               if (strcmp(jstr,"fakein1")==0) fake_data = 1; /* replace these with real names */
               else if (strcmp(jstr,"fakein2")==0) fake_data = 2;
               else if (strcmp(jstr,"fakein3")==0) fake_data = 3;
               else if (strcmp(jstr,"fakein4")==0) fake_data = 4;
               else if (strcmp(jstr,"fakein5")==0) fake_data = 5;
               else if (strcmp(jstr,"fakein6")==0) fake_data = 6;
               else if (strcmp(jstr,"fakein-1")==0) fake_data = -1;
               else if (strcmp(jstr,"fakein-2")==0) fake_data = -2;
               else if (strcmp(jstr,"fakeout1")==0) fake_data = 21;
               else if (strcmp(jstr,"fakeout2")==0) fake_data = 22;
               else if (strcmp(jstr,"fakeout3")==0) fake_data = 23;
               else if (strcmp(jstr,"fakeout4")==0) fake_data = 24;
               else if (strcmp(jstr,"fakeout5")==0) fake_data = 25;
               else if (strcmp(jstr,"fakeout6")==0) fake_data = 26;
               else if (strcmp(jstr,"fakeinput")==0) 
               {
		 fake_data = 11;
                 if (fscanf(fdes, "%s", jstr) == EOF)
                    iskip = 1;
                 else
                 {
                   asize = atoi(jstr);
		   if (asize < 0) asize = 1;
		   if (asize > AMAX) asize = AMAX;
/*		   printf("fakeinput asize=%d jstr= ",asize);
		   for (i=0; (i<4 && (fscanf(fdes, "%s", jstr) != EOF)); i++)
		     printf("%s ",jstr); printf("\n");
*/
		   for (j=0; j<asize; j++)
		   {
/* printf("j=%d ",j); */
		     for (i=0; (i<4 && (fscanf(fdes, "%s", jstr) != EOF)); i++)
		     {
		       switch (i)
		       {
			case 0: afreq[j] = atof(jstr); break;
			case 1: awidth[j] = atof(jstr); break;
			case 2: aamp[j] = atof(jstr); break;
			case 3: aphase[j] = atof(jstr); break;
		       }
/* printf("%g ",atof(jstr)); */
		     }
/* printf("\n"); */
		   }
                 }
               }
               else if (strcmp(jstr,"fakeoutput")==0) 
               {
		 fake_data = 31;
                 if (fscanf(fdes, "%s", jstr) == EOF)
                    iskip = 1;
                 else
                 {
                   asize = atoi(jstr);
		   if (asize < 0) asize = 1;
		   if (asize > AMAX) asize = AMAX;
/* 		   printf("fakeoutput asize=%d jstr= ",asize);
		   for (i=0; (i<4 && (fscanf(fdes, "%s", jstr) != EOF)); i++)
		     printf("%s ",jstr); printf("\n");
*/
		   for (j=0; j<asize; j++)
		   {
/* printf("j=%d ",j); */
		     for (i=0; (i<4 && (fscanf(fdes, "%s", jstr) != EOF)); i++)
		     {
		       switch (i)
		       {
			case 0: afreq[j] = atof(jstr); break;
			case 1: awidth[j] = atof(jstr); break;
			case 2: aamp[j] = atof(jstr); break;
			case 3: aphase[j] = atof(jstr); break;
		       }
/* printf("%g ",atof(jstr)); */
		     }
/* printf("\n"); */
		   }
                 }
               }
               else fake_data = 0;
            }
         }
         else if ((strcmp(jstr,"fakealfa")==0) && (fake_data != 0))
         {
            if (fscanf(fdes, "%s", jstr) == EOF)
               iskip = 1;
            else
            {
               /* if jstr is not a number, atof returns 0 */
               fval = atof(jstr);
               tmp = (float) (dspar->dsfactor); /* fval in usec */
               if (fake_data > 20) tmp = 1.0;
               if (sw > 0.1) 
                  fake_alfa = (double)(fval * 1.0e-6 * sw * tmp ); 
	       if ((fake_data==23) || (fake_data==24))
                  fake_alfa=(double)fval;
            }
         }
         else if (strcmp(jstr,"COMMENTS:")==0)
            iskip = 1;
         else if ((jstr[0] == '-') && (jstr[1] == '-'))
            iskip = 1;
      }
      if ((fake_data > 20) && (show_fid > 1)) show_fid=0;

      dspar2->fill_dsp = fill_dsp;
      dspar2->fill_ph = fill_ph;
      dspar2->show_fid = show_fid;
      dspar2->fake_data = fake_data;
      dspar2->fake_alfa = fake_alfa;
      dspar2->tshift = tshift;

   }
   (void) fclose(fdes);
   return(0);

}
