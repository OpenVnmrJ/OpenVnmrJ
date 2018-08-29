/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* dspfuncs.c  DSP functions */

/*
 */

/*----------------------------------------------+
|						|
|		    dspfuncs.c			|
|						|
|   This source file contains the interface	|
|   to a selection of DSP routines which can	|
|   be applied to the FID data prior to its	|
|   being written to disk.			|
|						|
+----------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>
#include <math.h>

#include "errLogLib.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "shrexpinfo.h"
#include "shrstatinfo.h"
#include "dspfuncs.h"


/* #include <sys/time.h> */

#define DEBUG0	-1
/* #define DEBUG_DSP	0 */
/* #define DSPTIME 0 */

#define MAXPATHL	128
#define FALSE		0
#define TRUE		1
#define COMPLETE	0
#define ERROR		1
#define COMPLEX		2
#define AMAX		8

int		dspflag = FALSE;	/* initialization */
static int      dsp_out_np;
static int	dpf_state;
static int	fill_dsp = 5;
static float	fill_ph = 0;
static int	show_fid = 0;
static int	fake_data = 0;
static double	fake_alfa = 0;
static double	rp_add = 0;
static double	lp_add = 0;
/* extern int	Acqdebug; */ 

int asize;
double afreq[AMAX];
double awidth[AMAX];
double aamp[AMAX];
double aphase[AMAX];

dspInfo dspinfo;

static void rotate2(float *spdata, int nelems, double lpval, double rpval);

#ifdef DSPTIME
/*---------------------------------------
|					|
|	       diff_time()/3		|
|					|
|   This function prints the time	|
|   differences.			|
|					|
+--------------------------------------*/
diff_time(xx, tp1, tp2)
char		*xx;
struct timeval	tp1,
		tp2; 
{
   double	acc1,
		acc2;
   printf("%s ", xx);
   acc1 = 1e6*tp1.tv_sec + tp1.tv_usec;
   acc2 = 1e6*tp2.tv_sec + tp2.tv_usec;
   acc2 -= acc1;
   acc2 *= 1e-6;
   printf("duration is %g\n", acc2);
}
#endif

/*-----------------------------------------------
|                                               |
|               readDFcoefs()/0                 |
|						|
+----------------------------------------------*/
/* static int readDFcoefs(coefs,userpath) */
static int readDFcoefs(coefs,ExpInfo)
int coefs;
SHR_EXP_INFO ExpInfo;
/* char *userpath; */
{
   char		dfpath[256], jstr[256];
   int		i = 0,
		j, jval = 0, iskip, itry = 0,
		notempty = TRUE;
   float	*fptr, fval,
		tmp;
   FILE		*fdes,
		*fopen();


/*   fill_dsp = 0;  /* orig algorithm */
   fill_dsp = 5;    /* new algorithm */
   fill_ph = 0;
   show_fid = 0;
   fake_data = 0;
   if ( strcmp(dspinfo.name, "" ) == 0)
      return(ERROR);
   strcpy(jstr,dspinfo.name);
   if (strncmp(jstr,"tbcfilter",9) == 0)
      itry = 1;
   if (strcmp(jstr,"chargeup") == 0)
   {
      fill_dsp = 0;
      itry = 1;
   }

   strcpy(dfpath, ExpInfo->UsrDirFile);
   strcat(dfpath, "/filtlib/");
   strcat(dfpath, dspinfo.name);

   if ( (fdes = fopen(dfpath, "r")) == NULL )
   {
      if (itry != 1)
         fprintf(stderr, "readDFcoefs():  cannot open DF file %s.\n", dfpath);
      return(ERROR);
   }
   if (itry == 1)
   {
      show_fid = 0;
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
            if ((strcmp(jstr,"chargeup")==0) || (strcmp(jstr,"wurl")==0))
               fill_dsp = 0;
            else if ((strcmp(jstr,"phase")==0) || (strcmp(jstr,"phasea")==0) || (strcmp(jstr,"phaseb")==0))
            {
               fill_dsp = 7;
               if (strcmp(jstr,"phaseb")==0)
                  fill_dsp = 8;
               if (fscanf(fdes, "%s", jstr) == EOF)
                  iskip = 1;
               else
               {
                  /* atof returns 0 on failure */
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
            else if (strcmp(jstr,"alg3a")==0)
               fill_dsp = 31;
            else if (strcmp(jstr,"alg3b")==0)
               fill_dsp = 32;
            else if (strcmp(jstr,"alg3c")==0)
               fill_dsp = 33;
            else if ((strcmp(jstr,"alg3")==0) || (strcmp(jstr,"alg3d")==0))
               fill_dsp = 34;
            else if ((strcmp(jstr,"alg0")==0) || (strcmp(jstr,"alg0a")==0))
               fill_dsp = 5;
            else if (strcmp(jstr,"alg0b")==0)
               fill_dsp = 6;
            else if (strcmp(jstr,"alga")==0)
               fill_dsp = 51;
            else if (strcmp(jstr,"algb")==0)
               fill_dsp = 50;
            else if (strcmp(jstr,"algc")==0)
               fill_dsp = 52;
            else if (strcmp(jstr,"algd")==0)
               fill_dsp = 40;
            else if (strcmp(jstr,"alge")==0)
               fill_dsp = 49;
            else if (strcmp(jstr,"algf4")==0)
               fill_dsp = 41;
            else if (strcmp(jstr,"algf3")==0)
               fill_dsp = 42;
            else if (strcmp(jstr,"algf2")==0)
               fill_dsp = 43;
            else if (strcmp(jstr,"algf1")==0)
               fill_dsp = 44;
            else if (strcmp(jstr,"algg4")==0)
               fill_dsp = 45;
            else if (strcmp(jstr,"algg3")==0)
               fill_dsp = 46;
            else if (strcmp(jstr,"algg2")==0)
               fill_dsp = 47;
            else if (strcmp(jstr,"algg1")==0)
               fill_dsp = 48;
            else if (strcmp(jstr,"zero")==0)
               fill_dsp = -1;
            else if ((strcmp(jstr,"brutype")==0) || (strcmp(jstr,"bruker")==0))
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
            if ((strcmp(jstr,"osfid")==0) || (strcmp(jstr,"o")==0))
               show_fid = 1;
            else if ((strcmp(jstr,"dsfid")==0) || (strcmp(jstr,"d")==0))
               show_fid = 0;
         }
         else if (strcmp(jstr,"lpshift")==0)
         {
            dspinfo.tshift = 0;
            if (fscanf(fdes, "%s", jstr) == EOF)
               iskip = 1;
            else
            {
                  fval = atof(jstr);
                  if (fval < 0)
                     dspinfo.tshift = 0;
                  else
                     dspinfo.tshift = (int)(fval+0.5);
            }
         }
         else if (strcmp(jstr,"lptime")==0)
         {
            dspinfo.tshift = 0;
            if (fscanf(fdes, "%s", jstr) == EOF)
               iskip = 1;
            else
            {
                  fval = atof(jstr);
                  if (fval <= 0)
                     dspinfo.tshift = 0;
                  else
                  {
                     tmp = (float) (ExpInfo->DspOversamp); /* fval in usec */
                     if (ExpInfo->DspSw > 0.1) 
                        dspinfo.tshift = (int)(fval * 1.0e-6 * (ExpInfo->DspSw) * tmp ) + 1;
                  }
            }
         }
	 else if (strcmp(jstr,"rp_add")==0)
	 {
	    rp_add = 0.0;
            if (fscanf(fdes, "%s", jstr) == EOF)
               iskip = 1;
            else
            {
                  fval = atof(jstr);
		  rp_add = fval;
            }
	 }
	 else if (strcmp(jstr,"lp_add")==0)
	 {
	    lp_add = 0.0;
            if (fscanf(fdes, "%s", jstr) == EOF)
               iskip = 1;
            else
            {
                  fval = atof(jstr);
		  lp_add = fval;
            }
	 }
         else if (strcmp(jstr,"data")==0)
         {
            if (fscanf(fdes, "%s", jstr) == EOF)
               iskip = 1;
            else
            {
               if (strcmp(jstr,"fakein1")==0) fake_data = 1;
               else if (strcmp(jstr,"fakein2")==0) fake_data = 2;
               else if (strcmp(jstr,"fakein3")==0) fake_data = 3;
               else if (strcmp(jstr,"fakein4")==0) fake_data = 4;
               else if (strcmp(jstr,"fakein5")==0) fake_data = 5;
               else if (strcmp(jstr,"fakein-1")==0) fake_data = -1;
               else if (strcmp(jstr,"fakein-2")==0) fake_data = -2;
               else if (strcmp(jstr,"fakeout1")==0) fake_data = 21;
               else if (strcmp(jstr,"fakeout2")==0) fake_data = 22;
               else if (strcmp(jstr,"fakeout3")==0) fake_data = 23;
               else if (strcmp(jstr,"fakeout4")==0) fake_data = 24;
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
                   for (i=0; (i<4 && (fscanf(fdes, "%s", jstr) != EOF)); i++)
                     ;
                   for (j=0; j<asize; j++)
                   {
                     for (i=0; (i<4 && (fscanf(fdes, "%s", jstr) != EOF)); i++)
                     {
                       switch (i)
                       {
                        case 0: afreq[j] = atof(jstr); break;
                        case 1: awidth[j] = atof(jstr); break;
                        case 2: aamp[j] = atof(jstr); break;
                        case 3: aphase[j] = atof(jstr); break;
                       }
                     }
#ifdef DEBUG_DSP
     DPRINT4(DEBUG0,"%g %g %g %g\n",afreq[j],awidth[j],aamp[j],aphase[j]);
#endif
                   }
                 }
               }
               else if (strcmp(jstr,"fakeoutput")==0)
               {
                 fake_data = 25;
                 if (fscanf(fdes, "%s", jstr) == EOF)
                    iskip = 1;
                 else
                 {
                   asize = atoi(jstr);
                   if (asize < 0) asize = 1;
                   if (asize > AMAX) asize = AMAX;
                   for (i=0; (i<4 && (fscanf(fdes, "%s", jstr) != EOF)); i++)
                     ;
                   for (j=0; j<asize; j++)
                   {
                     for (i=0; (i<4 && (fscanf(fdes, "%s", jstr) != EOF)); i++)
                     {
                       switch (i)
                       {
                        case 0: afreq[j] = atof(jstr); break;
                        case 1: awidth[j] = atof(jstr); break;
                        case 2: aamp[j] = atof(jstr); break;
                        case 3: aphase[j] = atof(jstr); break;
                       }
                     }
#ifdef DEBUG_DSP
     DPRINT4(DEBUG0,"%g %g %g %g\n",afreq[j],awidth[j],aamp[j],aphase[j]);
#endif
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
                  fval = atof(jstr);
                  tmp = (float) (ExpInfo->DspOversamp); /* fval in usec */
                  if (fake_data > 20) tmp = 1.0;
                  if (ExpInfo->DspSw > 0.1) 
                     fake_alfa = (double)(fval * 1.0e-6 * (ExpInfo->DspSw) * tmp ); 
		  if ((fake_data==23) || (fake_data==24)) fake_alfa=(double)fval;
            }
         }
         else if (strcmp(jstr,"COMMENTS:")==0)
            iskip = 1;
         else if ((jstr[0] == '-') && (jstr[1] == '-'))
            iskip = 1;
      }
      if ((fake_data > 20) && (show_fid==1)) show_fid=0;
      (void) fclose(fdes);
      return(ERROR);
   }

   dspinfo.filtsum = 0.0;
   i = 0;
   while ( notempty )
     if ( notempty = ( fscanf(fdes, "%f", &tmp) != EOF ) )
       i++;

   if (i != coefs)
   {
      fprintf(stderr,
       "number of coefficients in file %s does not match oscoef parameter\n",
        dspinfo.name);
      return(ERROR);
   }
   else
   {
     rewind(fdes);
     if ((dspinfo.filter = (float *)malloc(sizeof(float)*(i+1)))==NULL)
     {
       fprintf(stderr,"readDFcoefs():  error allocating filter buffer\n");
       return(ERROR);
     }
     notempty = TRUE;
     fptr = dspinfo.filter;
     while ( notempty )
     {
       *fptr = 0.0;
       if ( notempty = ( fscanf(fdes, "%f", fptr) != EOF ) )
	 dspinfo.filtsum += *fptr++;
     }
   }

   *fptr = dspinfo.filtsum;
   dspinfo.oscoeff = i;

   (void) fclose(fdes);
   return(COMPLETE);
}


/*-----------------------------------------------
|                                               |
|                 initDSP()/1                   |
|						|
+----------------------------------------------*/
int initDSP(ExpInfo)
SHR_EXP_INFO ExpInfo;
{
   char		dspfilt[MAXPATHL];
   int		npdsp;
   int		osfactor = 0,
		oscoeff = 0;
   double	np,
   		tmp;
   void		calc_digfilter(),
        	calc_chargeup(),
		clearDSP();
   int		readDFcoefs(); 


/********************************
*  Do not continue with DSP if  *
*  "dspflag" is FALSE.          *
********************************/

   clearDSP();		/* sets `dspflag` to FALSE */

   if (ExpInfo->DspOversamp > 1)
   {
      osfactor = ExpInfo->DspOversamp;
      dspflag = TRUE;
   }

   if (!dspflag)
   {
      dspinfo.osfactor = 1;
      return(COMPLETE);
   }

/*******************************************
*  Get other relevant parameters for DSP.  *
*******************************************/

   np = ExpInfo->NumDataPts;
   oscoeff =  ExpInfo->DspOsCoef;
   if (oscoeff < OS_MIN_COEFF)
     oscoeff = 31;
   if (oscoeff % 2 == 0)
     oscoeff++;

   if (strlen(ExpInfo->DspFiltFile))
         strcpy(dspinfo.name, ExpInfo->DspFiltFile);
/*   else
         strcpy(dspinfo.name, "\0"); */

   dspinfo.oscoeff = oscoeff;
   dspinfo.osfactor = osfactor;
   dspinfo.osfiltfactor = osfactor;
   if ((ExpInfo->DspFb > 0.0) && (ExpInfo->DspSw > 0.0))
      dspinfo.osfiltfactor = osfactor*ExpInfo->DspSw/(2.0*ExpInfo->DspFb);
   if (dspinfo.osfiltfactor <= 1.0)
     dspinfo.osfiltfactor = osfactor;

   tmp = (double)(osfactor);
   if (ExpInfo->DspSw > 0.1) 
   {
      if (ExpInfo->DspOsskippts >= 1)
      {
        dspinfo.tshift = ExpInfo->DspOsskippts;
      }
      else
      {
        dspinfo.tshift = (int)(1.0e-5 * (ExpInfo->DspSw) * tmp ) + 1;
      }
   }
   dpf_state = (int)(ExpInfo->DataPtSize);
/*
   DPRINT3(DEBUG0,"initDSP():  OversampSw=%g  tshift=%d  dpf=%d\n",
		(ExpInfo->DspSw) * tmp, dspinfo.tshift, dpf_state);
   DPRINT2(DEBUG0,"initDSP(): Oversamp=%d  Oscoef=%d\n",
		osfactor, oscoeff);
*/

/*******************************************
*  Calculate the subsampling factor, also  *
*  called the DSP scaling factor, for the  *
*  number of points to be acquired.        *
*******************************************/

   if (dspinfo.osfactor < 2)
   {
      clearDSP();
      fprintf(stderr, "initDSP():  osfactor too small.\n");
      return(ERROR);
   }

/*********************************************
*  Re-initialize DSP pointers if necessary.  *
*********************************************/
/* should already be done by clearDSP() */

/*
   if (dspinfo.filter != NULL)
   {
      free( (char *)dspinfo.filter );
      dspinfo.filter = NULL;
   }
   if (dspinfo.chargeup != NULL)
   {
      free( (char *)dspinfo.chargeup );
      dspinfo.chargeup = NULL;
   }
   if (dspinfo.buffer != NULL)
   {
      free( (char *)dspinfo.buffer );
      dspinfo.buffer = NULL;
   }
   if (dspinfo.data != NULL)
   {
      free( (char *)dspinfo.data );
      dspinfo.data = NULL;
   }
*/

/*   if (readDFcoefs(dspinfo.oscoeff,ExpInfo->UsrDirFile)) */
   if (readDFcoefs(dspinfo.oscoeff,ExpInfo))
   {
     if ( (dspinfo.filter = (float *) malloc( (unsigned) ((dspinfo.oscoeff+1) *
			sizeof(float)) )) == NULL )
     {
       fprintf(stderr,
		"initDSP():  cannot allocate memory for digital filter.\n");
       clearDSP();
       return(ERROR);
     }
     calc_digfilter();
   }
/*
   DPRINT4(DEBUG0,"initDSP(): OversampSw=%g tshift=%d Oversamp=%d Oscoef=%d\n",
	(ExpInfo->DspSw) * tmp, dspinfo.tshift, osfactor, oscoeff);
*/

/******************************************
*  Allocate memory for digital filter,    *
*  DSP buffer, and filtered FID and then  *
*  read in filter coefficients.           *
******************************************/

   dsp_out_np = (int) (np + 0.5);
   npdsp = dsp_out_np * dspinfo.osfactor + 2 * dspinfo.oscoeff + 2 * dspinfo.tshift;

   if ( (dspinfo.buffer = (float *) malloc( (unsigned) ( npdsp *
		sizeof(float) ) )) == NULL )
   {
      fprintf(stderr, 
                "initDSP():  cannot allocate memory for DSP buffer.\n"); 
      clearDSP();
      return(ERROR); 
   }
   if ( (dspinfo.chargeup = (float *) malloc( (unsigned) ( dspinfo.oscoeff *
		sizeof(float) ) )) == NULL )
   {
      fprintf(stderr, 
                "initDSP():  cannot allocate memory for DSP buffer.\n"); 
      clearDSP();
      return(ERROR); 
   }
   if ( (dspinfo.data = (float *) malloc( (unsigned) (dsp_out_np * sizeof(float) ) )) == NULL )
   {
      fprintf(stderr, 
                "initDSP():  cannot allocate memory for DSP buffer.\n"); 
      clearDSP();
      return(ERROR); 
   }

   calc_chargeup();
   if ((ExpInfo->DspSw > 0.0) &&
       ((ExpInfo->DspOslsfrq > 0.1) || (ExpInfo->DspOslsfrq < -0.1) ))
      dspinfo.oslsfrq =
           -180.0*(ExpInfo->DspOslsfrq/(dspinfo.osfactor*ExpInfo->DspSw));
/* multiplied by -2 later on -> 360.0 */

/*********************
*  Debug statements  *
*********************/

   return(COMPLETE);
}


/*-----------------------------------------------
|						|
|		  clearDSP()/0			|
|						|
+----------------------------------------------*/
void clearDSP()
{
   if (dspinfo.filter)
     free( (char *)dspinfo.filter );
   if (dspinfo.chargeup)
     free( (char *)dspinfo.chargeup );
   if (dspinfo.buffer)
     free( (char *)dspinfo.buffer );
   if (dspinfo.data)
     free( (char *)dspinfo.data );

   dspinfo.filter         = NULL;
   dspinfo.chargeup       = NULL;
   dspinfo.buffer         = NULL;
   dspinfo.data           = NULL;
   dspinfo.filtsum        = 1.0;
   dspinfo.oscoeff        = 0;
   dspinfo.osfactor       = 1;
   dspinfo.osfiltfactor   = 1.0;
   dspinfo.oslsfrq        = 0;
   dspinfo.lvl            = 0;
   dspinfo.tlt            = 0;
   dspinfo.offset         = 0;
   dspinfo.tshift	  = 0; 
   dspflag                = FALSE;
   strcpy(dspinfo.name, "");

}


/*-----------------------------------------------
|						|
|		calc_chargeup()/0		|
|						|
+----------------------------------------------*/
void calc_chargeup()
{
   register int         i,
                        pts;
   register float       *tmpfilter;
   register double      val,
   			arg1,
			argi;

   tmpfilter = dspinfo.chargeup;
   pts = (dspinfo.oscoeff / 2) + 1;
   arg1 = (double)((2.0 * M_PI) / ((double)(dspinfo.oscoeff - 1)));

   for (i=0; i<(pts-1); i++)
   {
      argi = arg1 * ((double)i);
      val = 0.42 - 0.50*cos(argi) + 0.08*cos(2.0 * argi);
      if (val < 1e-15) val=0.0;
      *tmpfilter++ = (float) (val);
   }


}

/*-----------------------------------------------
|						|
|		calc_digfilter()/0		|
|						|
+----------------------------------------------*/
void calc_digfilter()
{
   register int         i,
                        j,
			nfullpts,
			max;
   register float       *tmpfilter,
			*tmpfilter2,
			fsum,
			fsumi;
   register double      wc,
                        hd,
                        w, 
                        arg1,
			argi,
			sixdblvl;

   tmpfilter = dspinfo.filter;
   tmpfilter2 = dspinfo.filter+dspinfo.oscoeff;
/*   sixdblvl = 1 / (dspinfo.osfiltfactor); */
   sixdblvl = 1 / (dspinfo.osfiltfactor) + 1.04 / (dspinfo.oscoeff); 
   wc = M_PI * sixdblvl;
   fsum = 0.0;
   nfullpts = dspinfo.oscoeff;

   arg1 = (2 * M_PI) / (double) (nfullpts - 1);
   max=(dspinfo.oscoeff+1)/2;
   for (i = 0; i < max; i++)         
   {
      j = i - max + 1;
      hd = ( j ? sin(wc * (double)j) / (M_PI * (double)j) :
		  sixdblvl );
      argi = arg1 * ((double)i);
      w = 0.42 - 0.5*cos(argi) + 0.08*cos(2.0 * argi);
      if (w < 1e-15) w=0;
      *tmpfilter = (float) (hd * w);
      *(--tmpfilter2) = *tmpfilter;
      fsum += (*tmpfilter++);
      if (tmpfilter-1 != tmpfilter2)
        fsum += (*tmpfilter2);
   }

   dspinfo.filtsum = fsum;
   fsumi = 1/fsum;
   tmpfilter = dspinfo.filter;
   for (i=0;i<dspinfo.oscoeff;i++)
       *tmpfilter++ *= fsumi;
   tmpfilter = dspinfo.filter+dspinfo.oscoeff;
   *tmpfilter = fsum;
 
}


/*-----------------------------------------------
|						|
|		   dspExec()/4			|
|						|
+----------------------------------------------*/
int dspExec(dataPtr, outPtr, np_os, fidsize, ct_os)
char	*dataPtr,	/* data pointer */
        *outPtr;	/* output data pointer */
unsigned long  	 np_os,		/* number of oversampled data points */
	 fidsize,	/* fid size in bytes */
	 ct_os;		/* number of transients */
{
   int			npbytes = (int)fidsize; 	/* number of bytes in acquired FID */
   int			ctval; 				/* number of transients */
   int			dpf = dpf_state; 		/* FID double precision flag */

   register short 	*sdata;
   register int		i, j,
			npwords,
			*ldata,
			ntaps;
   register float	*buffer,
			*fdata,
			fsum,
			sscale;
   int			ndpts;		/* real points */
   double		ph4=0, ph3=0, ph1=0, ph0=0, ph2=0, radtodeg = 180.0/M_PI,
			phc, phs;
   void			os_filterfid(),
			os_filterfid_old(),
			fill_chargeup(),
                        sclbuf(),
			dcrmv(),
			clearDSP();

   int			tmptmp_np;
#ifdef DSPTIME
   struct timeval	tp1;
   struct timezone	tzp1;

/*   if (Acqdebug > DEBUG0) */
      gettimeofday(&tp1, &tzp1);
#endif

    ctval = (int)ct_os; 

/**************************
*  load data into buffer  *
**************************/

    dspinfo.offset = (dspinfo.oscoeff - 1) + 2 * dspinfo.tshift;
    fdata = dspinfo.buffer + dspinfo.offset;

if (fake_data == 0)
{
    if (dpf == 4)
    {
	ldata = (int *)dataPtr;
	npwords = npbytes / sizeof(int); 
	for (i=0; i<npwords; i++)
	    *fdata++ = (float) (*ldata++);
    }
    else
    {
	sdata = (short *)dataPtr;
	npwords = npbytes / (sizeof(short));
	for (i=0; i<npwords; i++)
	    *fdata++ = (float) (*sdata++);
    }
}
else
{
/*    ndpts  = npbytes / dpf; */
    if (dpf==4)
	npwords = npbytes / sizeof(int); 
    else
	npwords = npbytes / sizeof(short); 
    switch (fake_data)
    {
    case -2: case 21: case 22: case 23: case 24: case 25:
        for (i=0; i<2*(dspinfo.oscoeff-1); i++)
        {
            *fdata++ = 0.0;
            *fdata++ = 0.0;
        }
        for (i=2*(dspinfo.oscoeff-1); i<npwords/2; i++)
        {
            *fdata++ = 100.0;
            *fdata++ = -100.0;
        }
        break;
    case -1:
        for (i=0; i<npwords/2; i++)
        {
            *fdata++ = 100.0;
            *fdata++ = -100.0;
        }
        break;
    case 1:
        ph1 = 1/((double)(npwords/2));
        ph2 = 800 * M_PI;
        ph3 = 10 * ((double)dspinfo.osfactor);
        for (i=0; i<npwords/2; i++)
        {
          ph0 = ((double)i) * ph1;
          *fdata++ = (float)( 100.0 * cos(ph2 * ph0) );
          *fdata++ = (float)( 100.0 * sin(ph2 * ph0) );
        }
        break;
    case 2: default:
        ph1 = 1/((double)(npwords/2));
        ph2 = 800 * M_PI;
        ph3 = 5 * ((double)dspinfo.osfactor);
        for (i=0; i<npwords/2; i++)
        {
          ph0 = (((double)i) + fake_alfa) * ph1;
          ph4 = exp(-ph0 * ph3);
          *fdata++ = (float)( 100.0 * cos(ph2 * ph0) * ph4 );
          *fdata++ = (float)( 100.0 * sin(ph2 * ph0) * ph4 );
        }
        break;
    case 3:
        ph1 = 1/((double)(npwords/2 / dspinfo.osfactor));
        ph2 = 0.5 * M_PI / ph1;
        ph2 /= 2;
        ph3 = 0.0065 / ph1;
        ph1 = 1/((double)(npwords/2));
        ph0 = (fake_alfa) * ph1;
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
          *fdata = (float)( 100.0 * (phc + (phc * phc - phs * phs)) );
          *fdata *= ph4;
	  fdata++;
          *fdata = (float)( 100.0 * (phs + (2.0 * phs * phc)) );
          *fdata *= ph4;
	  fdata++;
        }
        break;
    case 4:
        ph1 = 1/((double)(npwords/2 / dspinfo.osfactor));
        ph2 = 0.5 * M_PI / ph1;
        ph2 /= 2;
        ph3 = 0.0065 / ph1;
        ph1 = 1/((double)(npwords/2));
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
        ph1 = 1/((double)(npwords/2 / dspinfo.osfactor));
        ph2 = 0.5 * M_PI / ph1;
        ph2 /= 2;
        ph3 = 0.0013 / ph1;
        ph1 = 1/((double)(npwords/2));
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
	  fdata = dspinfo.buffer + dspinfo.offset;
          ph1 = 1/((double)(npwords/2 / dspinfo.osfactor));
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
}

#ifdef DEBUG_DSP
    DPRINT(DEBUG0,"dspExec initial data points\n");
    fdata = dspinfo.buffer + dspinfo.offset;
    for (i=0; i<12; i++)
        DPRINT2(DEBUG0,"  input_data[%d]=%g\n",i,fdata[i]);
#endif

/***************************************
*  Initialize variables and pointers.  *
***************************************/

   fsum = dspinfo.filtsum;
   ntaps = dspinfo.oscoeff;
   ndpts  = npbytes / dpf; 
   buffer = dspinfo.buffer + dspinfo.offset;

/*********************************
*  Apply the digital filter and  *
*  decimate the filtered FID.    *
*********************************/

/* do dcrmv if ct == 1 */
   if ((fabs(dspinfo.lvl) > 0.00001 || fabs(dspinfo.tlt) > 0.00001) && (ctval == 1))
   { 
     if (fake_data == 0)
       dcrmv(buffer,ndpts/2,dspinfo.lvl,dspinfo.tlt);
   }

/*
   if ( (dpf==4) && (ctval > 1) )
   {
	if (ct_os >= dsp_numtrans) {
     sclbuf(buffer,ndpts/2,(float) 1.0 / (float) ctval); }
	else {
     sclbuf(buffer,ndpts/2,(float) 0.0); } 
   } 
*/
/*
   if ( (ctval > 1) )
   {
     sclbuf(buffer,ndpts/2,(float) 1.0 / (float) ctval); 
   } 
*/

/* rotate if oslsfrq requested */
/*
   if (fabs(dspinfo.oslsfrq) > 0.0001)
   {
     buffer = dspinfo.buffer;
     ph1 = dspinfo.oslsfrq*2.0*( ((double)(ndpts/2))-1.0 );
     ph0 = dspinfo.oslsfrq*2.0*( ((double)((ndpts-ntaps)/2))-1.0 );
     rotate2(buffer, ndpts/2, ph1, -ph0);
   }
*/
   if (fabs(dspinfo.oslsfrq) > 0.0001)
   {
     tmptmp_np = dsp_out_np * dspinfo.osfactor + 2 * dspinfo.oscoeff + 2 * dspinfo.tshift;
     buffer = dspinfo.buffer;

     ph1 = -2.0 * dspinfo.oslsfrq;
     ph0 = 0.0;
     ph0 = -ph1 * dspinfo.osfactor * 4.0; /* empirical number */

     ph1 += lp_add;
     ph0 += rp_add;

/*
DPRINT3(DEBUG0,"rotate2(): ndpts=%d ph1=%16.8f ph0=%16.8f\n", 
	ndpts, ph1, ph0);
*/
     rotate2(buffer, tmptmp_np/2, ph1, ph0);
   }

/*******************************************************
*  Perform filtering.                                  *
*  Recast the filtered FID data into single-precision  *
*  integer form if and only if dp = 'n'.  Otherwise,   *
*  the filtered FID data should stay floating point.   *
*******************************************************/
   if (dpf == 4)
   {

      if ((fake_data==21) || (fake_data==22))
      {
        fdata = (float *)outPtr;
        ph1 = 1/((double)(dsp_out_np/2));
        ph2 = 0.5 * M_PI / ph1;
        ph2 /= 2;
        ph3 = 0.0065 / ph1;
        ph1 = 1/((double)(dsp_out_np/2));
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
      }
      else if (fake_data==23)
      {
        fdata = (float *)outPtr;
        ctval = (int)(fake_alfa + 0.5);
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
      }
      else if (fake_data==24)
      {
        fdata = (float *)outPtr;
        ctval = (int)(fake_alfa + 0.5);
        if (ctval < 0) ctval = 0;
        ph1 = 1/((double)(dsp_out_np/2));
        ph2 = 0.5 * M_PI / ph1;
        ph2 /= 2;
        ph3 = 0.0065 / ph1;
        ph1 = 1/((double)(dsp_out_np/2));
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
      }
      else if (fake_data == 25)
      {
        fdata = (float *)outPtr;
        ctval = (int)(fake_alfa + 0.5);
        if (ctval < 0) ctval = 0;
        for (i=0; i<dsp_out_np; i++) *fdata++ = 0.0;
        for (j=0; j<asize; j++)
        {
          fdata = (float *)outPtr;
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
      }
      else
      {

   fill_chargeup(dspinfo.buffer, COMPLEX, &ph3,&ph4);
#ifdef DEBUG_DSP
   DPRINT(DEBUG0,"  initial buffer points:\n");
   for (i=0; i<ntaps+12; i++)
        DPRINT2(DEBUG0,"  buffer[%d]=%g\n",i,dspinfo.buffer[i]);
#endif
/* try rotation after fill_chargeup() ?? */
/*
   if (fabs(dspinfo.oslsfrq) > 0.0001)
   {
     ph1 = dspinfo.oslsfrq;
     ph1 *= 2 * (ndpts/2 - 1);
     tmptmp_np = dsp_out_np * dspinfo.osfactor + 2 * dspinfo.oscoeff + 2 * dspinfo.tshift;
     ph0 = -ph1;
     buffer = dspinfo.buffer;
     ph0 += ph1;
     ph0 += rp_add;
     ph1 *= ( -1.0/((double) (ndpts/2 - 1)) );
     ph1 += lp_add;

DPRINT4(DEBUG0,"rotate2(): ndpts=%d npdsp_calc=%d ph1=%16.8f ph0=%16.8f\n", 
	ndpts, tmptmp_np, ph1, ph0);

     rotate2(buffer, ndpts/2, ph1, ph0);
   }
*/
if (fill_dsp == -3)
{
/* output goes into outPtr, not dspinfo.data */
/* real data */
   os_filterfid_old((float *)outPtr,dspinfo.buffer,dspinfo.filter,COMPLEX); 
/* imaginary data */
   os_filterfid_old((float *)(outPtr+sizeof(float)),dspinfo.buffer+1,dspinfo.filter,COMPLEX); 
}
else
{
/* output goes into outPtr, not dspinfo.data */
/* real data */
   os_filterfid((float *)outPtr,dspinfo.buffer,dspinfo.filter,COMPLEX); 
/* imaginary data */
   os_filterfid((float *)(outPtr+sizeof(float)),dspinfo.buffer+1,dspinfo.filter,COMPLEX); 
}
      }

#ifdef DEBUG_DSP
    DPRINT(DEBUG0,"dspExec final data points\n");
    fdata = (float *)(outPtr);
    for (i=0; i<12; i++)
        DPRINT2(DEBUG0,"  output_data[%d]=%g\n",i,fdata[i]);
#endif

   }
   else		/* dpf==2 */
   {
      fill_chargeup(dspinfo.buffer, COMPLEX, &ph3,&ph4);
#ifdef DEBUG_DSP
   DPRINT(DEBUG0,"  initial buffer points:\n");
   for (i=0; i<ntaps+12; i++)
        DPRINT2(DEBUG0,"  buffer[%d]=%g\n",i,dspinfo.buffer[i]);
#endif
if (fill_dsp == -3)
{
/* output goes into outPtr, not dspinfo.data */
      /* output goes into dspinfo.data, then cast into outPtr as short */
      /* real data */
      os_filterfid_old(dspinfo.data,dspinfo.buffer,dspinfo.filter,COMPLEX); 
      /* imag data */
      os_filterfid_old(dspinfo.data+1,dspinfo.buffer+1,dspinfo.filter,COMPLEX); 
}
else
{
/* output goes into outPtr, not dspinfo.data */
      /* output goes into dspinfo.data, then cast into outPtr as short */
      /* real data */
      os_filterfid(dspinfo.data,dspinfo.buffer,dspinfo.filter,COMPLEX); 
      /* imag data */
      os_filterfid(dspinfo.data+1,dspinfo.buffer+1,dspinfo.filter,COMPLEX); 
}
/*
      fsum = 0.0;
      buffer = dspinfo.data;
      for (i = 0; i < dsp_out_np; i++)
      {
         if (fabs(*buffer) > fsum) 
            fsum = fabs(*buffer);
         buffer++;
      }
*/

      buffer = dspinfo.data;
      sdata = (short *)outPtr;
      for (i = 0; i < dsp_out_np; i++)
         *sdata++ = (short) (*buffer++); 

#ifdef DEBUG_DSP
    DPRINT(DEBUG0,"dspExec final data points\n");
    fdata = dspinfo.data;
    for (i=0; i<12; i++)
        DPRINT2(DEBUG0,"  output_data[%d]=%g\n",i,fdata[i]);
#endif
   }
   ph2 = radtodeg * atan2(ph4,ph3);
   if (ph2 < 0) ph2 = ph2 + 360.0;
/*   DPRINT4(DEBUG0, "dspExec(): fill_dsp=%d cos2p=%g sin2p=%g 2p=%g\n", fill_dsp, ph0, ph1, ph2); */
/*   DPRINT2(DEBUG0, "dspExec(): fill_dsp=%d  two_phi=%g\n", fill_dsp, ph2); */
   return( dpf * dsp_out_np );
}

/*---------------------------------------
|					|
|		   dcrmv()/4		|
|					|
+---------------------------------------*/
void dcrmv(buffer, ncpts, lvl, tlt)
register float *buffer;
register int ncpts;
register float lvl, tlt;
{
    register int i;

    for (i=0;i<ncpts;i++)
    {
      *buffer++ -= lvl;
      *buffer++ -= tlt;
    }
}

/*---------------------------------------
|					|
|		   sclbuf()/3		|
|					|
+---------------------------------------*/
void sclbuf(buffer, ncpts, scl)
register float *buffer;
register int ncpts;
register float scl;
{
    register int i;

    for (i=0;i<ncpts;i++)
    {
      *buffer++ *= scl;
      *buffer++ *= scl;
    }
}

/*---------------------------------------
|					|
|	   fill_chargeup()/4		|
|					|
+---------------------------------------*/
void fill_chargeup(buffer, dataskip, ph0, ph1)
register int    dataskip;
float  		*buffer;
double		*ph0, *ph1;
{
   register int		ntaps,
			decfact,
			buffskip;
   register int         i, j, k;
   int			tshift;
   register float       *tmpdfilter,
			*tmpbuffer,
			*tmpbuffer2,
			sum=0, sumi=0, ftmp;
   float		sumx, sumy,
			sig, sigx, sigxx, sigy, sigxy, sigd, sigz, sigxz;
   void interp_phase(), interp2_phase(), interp3_phase(), interp4_phase();

   ntaps = dspinfo.oscoeff;
   decfact = dspinfo.osfactor;
   tmpbuffer = buffer;
   tmpdfilter = dspinfo.chargeup;
   tshift = dspinfo.tshift;
#ifdef DEBUG_DSP
    DPRINT1(DEBUG0,"dspExec: calling fill_chargeup(), fill_dsp=%d\n",fill_dsp);
#endif

   switch (fill_dsp)
   {
   case 0: case -3: default:	/* could simplify */
      k = tshift;
      tmpbuffer = buffer;	/* real */
      for (i = 0; i < k; i++)
      {
         *tmpbuffer = 0;
         tmpbuffer += dataskip;
      }
/*      sum = *(tmpbuffer + dspinfo.offset); */
      sum = *(tmpbuffer + dspinfo.offset - dataskip * tshift);
      tmpdfilter = dspinfo.chargeup;
      k = ntaps/2;
      tmpbuffer = buffer + dataskip * tshift;
      for (i = 0; i < k; i++)
      {
         *tmpbuffer = sum * *tmpdfilter++;
         tmpbuffer += dataskip;
      }
      k = tshift;
      tmpbuffer = buffer;
      tmpbuffer++;		/* imag */
      for (i = 0; i < k; i++)
      {
         *tmpbuffer = 0;
         tmpbuffer += dataskip;
      }
/*      sum = *(tmpbuffer + dspinfo.offset); */
      sum = *(tmpbuffer + dspinfo.offset - dataskip * tshift); 
      tmpdfilter = dspinfo.chargeup;
      k = ntaps/2;
      tmpbuffer = buffer + dataskip * tshift;
      tmpbuffer++;
      for (i = 0; i < k; i++)
      {
         *tmpbuffer = sum * *tmpdfilter++;
         tmpbuffer += dataskip;
      }
      break;
   case -1:	/* Bruker style ?? not really, don't need lp<>0 */
      k = tshift + ntaps/2;
      tmpbuffer2 = buffer;
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2++ = 0.0;
         *tmpbuffer2++ = 0.0;
      }
      break;
   case -2:	/* Bruker style, need big lp>0 */
      k = tshift + ntaps/2;
      tmpbuffer2 = buffer;
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2++ = 0.0;
         *tmpbuffer2++ = 0.0;
      }
      break;
   case 1:
      k = tshift + ntaps/2;
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2++ = *tmpbuffer++;
         *tmpbuffer2 = (*tmpbuffer++);
          tmpbuffer2 -= 3;
      }
      break;
   case 2:
      k = tshift + ntaps/2;
      tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2++ = *tmpbuffer++;
         *tmpbuffer2 = (*tmpbuffer++);
          tmpbuffer2 -= 3;
      }
      break;
   case 3:
      k = tshift + ntaps/2;
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2++ = *tmpbuffer++;
         *tmpbuffer2 = -(*tmpbuffer++);
          tmpbuffer2 -= 3;
      }
      break;
   case 4:
      k = tshift + ntaps/2;
      tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2++ = *tmpbuffer++;
         *tmpbuffer2 = -(*tmpbuffer++);
          tmpbuffer2 -= 3;
      }
      break;
   case 360: /* same as case 3 */
      k = tshift + ntaps/2;
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = *tmpbuffer;
         *(tmpbuffer2+1) = -(*(tmpbuffer+1));
          tmpbuffer2 -= dataskip;
          tmpbuffer += dataskip;
      }
      break;
   case 180:
      k = tshift + ntaps/2;
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = -(*tmpbuffer);
         *(tmpbuffer2+1) = (*(tmpbuffer+1));
          tmpbuffer2 -= dataskip;
          tmpbuffer += dataskip;
      }
      break;
   case 90:
      k = tshift + ntaps/2;
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = *(tmpbuffer+1);
         *(tmpbuffer2+1) = -(*(tmpbuffer));
          tmpbuffer2 -= dataskip;
          tmpbuffer += dataskip;
      }
      break;
   case 270:
      k = tshift + ntaps/2;
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = -(*(tmpbuffer+1));
         *(tmpbuffer2+1) = (*(tmpbuffer));
          tmpbuffer2 -= dataskip;
          tmpbuffer += dataskip;
      }
      break;
   case 5:
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT4(DEBUG0, "sumx=%g sumy=%g cos2p=%g sin2p=%g\n", sumx, sumy, sum, sumi);
#endif
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
/*
         ftmp = *tmpbuffer; tmpbuffer++;
         *tmpbuffer2 = sum * (ftmp) + sumi * (*tmpbuffer);
         tmpbuffer2++;
         *tmpbuffer2 = sumi * (ftmp) - sum * (*tmpbuffer);
         tmpbuffer2 -= 3;
         tmpbuffer++;
*/
      }
      break;
   case 6:
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
#ifdef DEBUG_DSP
    DPRINT2(DEBUG0, "cos2p=%g sin2p=%g\n", sum, sumi);
#endif
      tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = sum * *tmpbuffer + sumi * *(tmpbuffer+1);
         *(tmpbuffer2+1) = sumi * *tmpbuffer - sum * *(tmpbuffer+1);
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 7:	/* filtfile='tryaNNN' */
      k = tshift + ntaps/2;
      sum = (float)cos((double)(fill_ph*M_PI/180.0));
      sumi = (float)sin((double)(fill_ph*M_PI/180.0));
      tmpbuffer = buffer + 2 * (k + 1);
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
/*
         *tmpbuffer2 = (float)((double)sum * (double)(*tmpbuffer) + (double)sumi * (double)(*(tmpbuffer+1)));
         *(tmpbuffer2+1) = (float)((double)sumi * (double)(*tmpbuffer) - (double)sum * (double)(*(tmpbuffer+1)));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
*/
         ftmp = *tmpbuffer; tmpbuffer++;
         *tmpbuffer2 = sum * (ftmp) + sumi * (*tmpbuffer);
         tmpbuffer2++;
         *tmpbuffer2 = sumi * (ftmp) - sum * (*tmpbuffer);
         tmpbuffer2 -= 3;
         tmpbuffer++;
      }
      break;
   case 8:	/* filtfile='trybNNN' */
      k = tshift + ntaps/2;
      sum = (float)cos((double)(fill_ph*M_PI/180.0));
      sumi = (float)sin((double)(fill_ph*M_PI/180.0));
      tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = sum * *tmpbuffer + sumi * *(tmpbuffer+1);
         *(tmpbuffer2+1) = sumi * *tmpbuffer - sum * *(tmpbuffer+1);
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 9:
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 4)));
      sumy = (float)(*(buffer + 2 * (k + 4) + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
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
   case 10:
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 4)));
      sumy = (float)(*(buffer + 2 * (k + 4) + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
      tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 11:
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 3)));
      sumy = (float)(*(buffer + 2 * (k + 3) + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
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
   case 12:
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 3)));
      sumy = (float)(*(buffer + 2 * (k + 3) + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
      tmpbuffer = buffer + 2 * k;
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
      sumx = (float)(*(buffer + 2 * (k + 2)));
      sumy = (float)(*(buffer + 2 * (k + 2) + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
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
   case 14:
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 2)));
      sumy = (float)(*(buffer + 2 * (k + 2) + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
      tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 15:
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 1)));
      sumy = (float)(*(buffer + 2 * (k + 1) + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
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
   case 16:
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 1)));
      sumy = (float)(*(buffer + 2 * (k + 1) + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
      tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 17: /* same as case 5 */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 0)));
      sumy = (float)(*(buffer + 2 * (k + 0) + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
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
   case 18: /* same as case 6 */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 0)));
      sumy = (float)(*(buffer + 2 * (k + 0) + 1));
      sumi = 1.0/(sumx * sumx + sumy * sumy);
      sum  = sumi * (sumx * sumx - sumy * sumy);
      sumi = sumi * 2.0 * sumx * sumy;
      tmpbuffer = buffer + 2 * k;
      tmpbuffer2 = buffer + 2 * (k - 1);
      for (i = 0; i < k; i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 21: case 22: /* linear fit */
/*      k = tshift + ntaps/2; */
      k = dspinfo.offset / 2;
      tmpbuffer = buffer + 2 * k;
      sig=0; sigx=0; sigxx=0; sigy=0; sigxy=0; sigz=0; sigxz=0;
      for (i=2; i<6; i++)
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
      if (fill_dsp==22)
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
   case 23: case 24: /* linear fit, subst pts 0 and 1 */
      k = tshift + ntaps/2;
      tmpbuffer = buffer + 2 * k;
      sig=0; sigx=0; sigxx=0; sigy=0; sigxy=0; sigz=0; sigxz=0;
      for (i=2; i<6; i++)
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
      *(tmpbuffer) = ((sigxx * sigy - sigx * sigxy) - 0 * (sig * sigxy - sigx * sigy));
      *(tmpbuffer) /= (sig * sigxx - sigx * sigx);
      *(tmpbuffer+1) = ((sigxx * sigz - sigx * sigxz) - 0 * (sig * sigxz - sigx * sigz));
      *(tmpbuffer+1) /= (sig * sigxx - sigx * sigx);
      *(tmpbuffer+2) = ((sigxx * sigy - sigx * sigxy) + 1 * (sig * sigxy - sigx * sigy));
      *(tmpbuffer+2) /= (sig * sigxx - sigx * sigx);
      *(tmpbuffer+3) = ((sigxx * sigz - sigx * sigxz) + 1 * (sig * sigxz - sigx * sigz));
      *(tmpbuffer+3) /= (sig * sigxx - sigx * sigx);
      if (fill_dsp==24)
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
   case 31: /* calculate cos2p sin2p from if-then tree */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      interp_phase(&sumx, &sumy);
      sum=sumx; sumi=sumy;
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
   case 32: /* calculate cos2p sin2p from arcsin bitmask */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      interp2_phase(&sumx, &sumy);
      sum=sumx; sumi=sumy;
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
   case 33: /* calculate cos2p sin2p from 1/v approx */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      interp3_phase(&sumx, &sumy);
      sum=sumx; sumi=sumy;
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
   case 34: /* calculate cos2p sin2p from 1/v approx, std dsp inverse */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * k));
      sumy = (float)(*(buffer + 2 * k + 1));
      interp4_phase(&sumx, &sumy);
      sum=sumx; sumi=sumy;
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
   case 40: /* 41-48 similar to 5, except use a sliding pt for phase calc */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + tshift)));
      sumy = (float)(*(buffer + 2 * (k + tshift) + 1));
      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT2(DEBUG0, "   cos2p=%g sin2p=%g\n", sum, sumi);
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
   case 41: /* 41-48 similar to 5, except use a diff pt for phase calc */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 4)));
      sumy = (float)(*(buffer + 2 * (k + 4) + 1));
      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT2(DEBUG0, "cos2p=%g sin2p=%g\n", sum, sumi);
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
   case 42: /* 41-48 similar to 5, except use a diff pt for phase calc */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 3)));
      sumy = (float)(*(buffer + 2 * (k + 3) + 1));
      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT2(DEBUG0, "cos2p=%g sin2p=%g\n", sum, sumi);
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
   case 43: /* 41-48 similar to 5, except use a diff pt for phase calc */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 2)));
      sumy = (float)(*(buffer + 2 * (k + 2) + 1));
      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT2(DEBUG0, "cos2p=%g sin2p=%g\n", sum, sumi);
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
   case 44: /* 41-48 similar to 5, except use a diff pt for phase calc */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 1)));
      sumy = (float)(*(buffer + 2 * (k + 1) + 1));
      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT2(DEBUG0, "cos2p=%g sin2p=%g\n", sum, sumi);
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
   case 45: /* 41-48 similar to 5, except use a diff pt for phase calc, replace pts */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 4)));
      sumy = (float)(*(buffer + 2 * (k + 4) + 1));
      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT2(DEBUG0, "cos2p=%g sin2p=%g\n", sum, sumi);
#endif
      tmpbuffer = buffer + 2 * ((k+4) + 1);
      tmpbuffer2 = buffer + 2 * ((k+4) - 1);
      for (i = 0; i < (k+4); i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 46: /* 41-48 similar to 5, except use a diff pt for phase calc, replace pts */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 3)));
      sumy = (float)(*(buffer + 2 * (k + 3) + 1));
      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT2(DEBUG0, "cos2p=%g sin2p=%g\n", sum, sumi);
#endif
      tmpbuffer = buffer + 2 * ((k+3) + 1);
      tmpbuffer2 = buffer + 2 * ((k+3) - 1);
      for (i = 0; i < (k+3); i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 47: /* 41-48 similar to 5, except use a diff pt for phase calc, replace pts */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 2)));
      sumy = (float)(*(buffer + 2 * (k + 2) + 1));
      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT2(DEBUG0, "cos2p=%g sin2p=%g\n", sum, sumi);
#endif
      tmpbuffer = buffer + 2 * ((k+2) + 1);
      tmpbuffer2 = buffer + 2 * ((k+2) - 1);
      for (i = 0; i < (k+2); i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 48: /* 41-48 similar to 5, except use a diff pt for phase calc, replace pts */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + 1)));
      sumy = (float)(*(buffer + 2 * (k + 1) + 1));
      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT2(DEBUG0, "cos2p=%g sin2p=%g\n", sum, sumi);
#endif
      tmpbuffer = buffer + 2 * ((k+1) + 1);
      tmpbuffer2 = buffer + 2 * ((k+1) - 1);
      for (i = 0; i < (k+1); i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 49: /* 41-48 similar to 5, except use a shifting pt for phase calc, replace pts */
      k = tshift + ntaps/2;
      sumx = (float)(*(buffer + 2 * (k + tshift)));
      sumy = (float)(*(buffer + 2 * (k + tshift) + 1));
      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT2(DEBUG0, "cos2p=%g sin2p=%g\n", sum, sumi);
#endif
      tmpbuffer = buffer + 2 * ((k+tshift) + 1);
      tmpbuffer2 = buffer + 2 * ((k+tshift) - 1);
      for (i = 0; i < (k+tshift); i++)
      {
         *tmpbuffer2 = (float)(sum * *tmpbuffer + sumi * *(tmpbuffer+1));
         *(tmpbuffer2+1) = (float)(sumi * *tmpbuffer - sum * *(tmpbuffer+1));
         tmpbuffer2 -= dataskip;
         tmpbuffer += dataskip;
      }
      break;
   case 50: /* first filtered downsampled point phase calc, Sf & Sb */
/* why are all buffer points scaled down from input points? */
      k = ntaps/2 + tshift;
      j = ntaps/2 + 1;

      sumx = 0.0; /* X forward */
      sumy = 0.0; /* Y forward */
      sigx = 0.0; /* X backward */
      sigy = 0.0; /* Y backward */
      tmpbuffer = buffer + (2 * (k + 1));
      tmpdfilter = dspinfo.filter + (j);
#ifdef DEBUG_DSP
/* tmpbuffer output doesn't depend on k, extra memory for skip pts */
      DPRINT5(DEBUG0," buffer[%d]=%g,%g,%g,%g\n",k,(*tmpbuffer),(*(tmpbuffer+1)),(*(tmpbuffer+2)),(*(tmpbuffer+3)) );
      DPRINT5(DEBUG0," dfilter[%d]=%g,%g,,%g,%g\n",j,(*tmpdfilter),(*(tmpdfilter+1)),(*(tmpdfilter+(j-2))),(*(tmpdfilter+(j-1))) );
#endif
      for (i = 1; i < j; i++)
      {
         sumx += (float)((*tmpbuffer) * (*tmpdfilter));
         sumy += (float)(*(tmpbuffer+1) * (*tmpdfilter));
         tmpbuffer += dataskip;
         tmpdfilter++;
      }
      sigx =  sumx;
      sigy = -sumy;
      tmpbuffer = buffer + (2 * k);
      tmpdfilter = dspinfo.filter + (j - 1);
#ifdef DEBUG_DSP
      DPRINT3(DEBUG0," last buffer=%g,%g dfilter=%g\n",(*tmpbuffer),(*(tmpbuffer+1)),(*(tmpdfilter)) );
#endif
      sumx += (float)((*tmpbuffer) * (*tmpdfilter));
      sumy += (float)(*(tmpbuffer+1) * (*tmpdfilter));

      sumi = sqrt((sumx*sumx + sumy*sumy) * (sigx*sigx + sigy*sigy));
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sigx + sumy * sigy);
         sumi = sumi * (sumy * sigx - sumx * sigy);
      }
#ifdef DEBUG_DSP
    DPRINT4(DEBUG0, "sumx=%g sumy=%g cos2p=%g sin2p=%g\n", sumx, sumy, sum, sumi);
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
   case 51: /* first filtered downsampled point as rcvr phase */
      k = ntaps/2 + tshift;
      j = ntaps/2 +1;

      sumx = 0.0; /* X forward */
      sumy = 0.0; /* Y forward */
      tmpbuffer = buffer + 2 * k;
      tmpdfilter = dspinfo.filter + (ntaps/2);
#ifdef DEBUG_DSP
/* tmpbuffer output doesn't depend on k, extra memory for skip pts */
      DPRINT5(DEBUG0," buffer[%d]=%g,%g,%g,%g\n",k,(*tmpbuffer),(*(tmpbuffer+1)),(*(tmpbuffer+2)),(*(tmpbuffer+3)) );
      DPRINT5(DEBUG0," dfilter[%d]=%g,%g,,%g,%g\n",j,(*tmpdfilter),(*(tmpdfilter+1)),(*(tmpdfilter+(j-1))),(*(tmpdfilter+j)) );
#endif
      for (i = 0; i < j; i++)
      {
         sumx += (float)((*tmpbuffer) * (*tmpdfilter));
         sumy += (float)(*(tmpbuffer+1) * (*tmpdfilter));
         tmpbuffer += dataskip;
         tmpdfilter++;
      }

      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT4(DEBUG0, "sumx=%g sumy=%g cos2p=%g sin2p=%g\n", sumx, sumy, sum, sumi);
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
   case 52: /* first filtered downsampled point, w/o 1st os pt, as rcvr phase */
      k = ntaps/2 + tshift;
      j = ntaps/2;

      sumx = 0.0; /* X forward */
      sumy = 0.0; /* Y forward */
      tmpbuffer = buffer + (2 * (k + 1));
      tmpdfilter = dspinfo.filter + (j + 1);
#ifdef DEBUG_DSP
/* tmpbuffer output doesn't depend on k, extra memory for skip pts */
      DPRINT5(DEBUG0," buffer[%d]=%g,%g,%g,%g\n",k,(*tmpbuffer),(*(tmpbuffer+1)),(*(tmpbuffer+2)),(*(tmpbuffer+3)) );
      DPRINT5(DEBUG0," dfilter[%d]=%g,%g,,%g,%g\n",j,(*tmpdfilter),(*(tmpdfilter+1)),(*(tmpdfilter+(j-1))),(*(tmpdfilter+j)) );
#endif
      for (i = 1; i < j; i++)
      {
         sumx += (float)((*tmpbuffer) * (*tmpdfilter));
         sumy += (float)(*(tmpbuffer+1) * (*tmpdfilter));
         tmpbuffer += dataskip;
         tmpdfilter++;
      }

      sumi = (sumx * sumx + sumy * sumy);
      if (sumi < 1e-6)
      {
         sum = 1.0; sumi = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
      }
      else
      {
         sumi = 1.0/sumi;
         sum  = sumi * (sumx * sumx - sumy * sumy);
         sumi = sumi * 2.0 * sumx * sumy;
      }
#ifdef DEBUG_DSP
    DPRINT4(DEBUG0, "sumx=%g sumy=%g cos2p=%g sin2p=%g\n", sumx, sumy, sum, sumi);
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
   }
   *ph0 = (double)sum; *ph1=(double)sumi;
}

/*-----------------------------------------------
|						|
|		   os_filterfid()/6		|
|						|
+----------------------------------------------*/
void os_filterfid(data, buffer, dfilter, dataskip)
register int    dataskip;
float  		*data,
		*buffer,
		*dfilter;
{
   register int		ntaps,
			decfact,
			buffskip;
   register int         i, j, k;
   int			tshift;
   register float       *tmpdata,
			*tmpdfilter,
			*tmpdfilter2,
			*tmpbuffer,
			sum;

   ntaps = dspinfo.oscoeff;
   decfact = dspinfo.osfactor;
   tmpdata = data;
   tmpbuffer = buffer;
   tmpdfilter = dspinfo.chargeup;
   tshift = dspinfo.tshift;

/* if (fill_dsp == 0)
{
   k = tshift;
   tmpbuffer = buffer;
   for (i = 0; i < k; i++)
   {
      *tmpbuffer = 0;
      tmpbuffer += dataskip;
   }

   sum = *(tmpbuffer + dspinfo.offset); 
   tmpdfilter = dspinfo.chargeup;
   k = ntaps/2;
   tmpbuffer = buffer + dataskip * tshift;
   for (i = 0; i < k; i++)
   {
     *tmpbuffer = sum * *tmpdfilter++;
     tmpbuffer += dataskip;
   }
} */

 if (show_fid == 1)
 {
   k = dsp_out_np /2;
   tmpbuffer = buffer;
   while (k--)
   {
     *tmpdata = *tmpbuffer;
     tmpdata += dataskip;
     tmpbuffer += dataskip;
   }
 }
 else
 {
   k = dsp_out_np /2;
   i = ntaps;
   tmpbuffer = buffer + dataskip * i;
   tmpdfilter2 = dfilter;
   buffskip = dataskip * (ntaps + decfact); 
   if (fill_dsp == -2) tmpbuffer += dataskip * (i + tshift);
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
     i += decfact;
     tmpbuffer += buffskip;
   }
 }

}


/*----------------------------------------------+
|                                               |
|                os_filterfid_old()/6           |
|                                               |
+----------------------------------------------*/
/* void os_filterfid_old(data, buffer, dfilter, dataskip) */
void os_filterfid_old(buffer, data, dfilter, dataskip)
int             dataskip;
float           *data;
float           *buffer,
                *dfilter;
{
   int                  ntaps,
			ncdatapts = dsp_out_np/2,
                        decfact;
   register int         i,
                        j;
   register float       *tmpdata;
   register float       *tmpdfilter,
                        *tmpbuffer,
                        sum;

   ntaps = dspinfo.oscoeff;
   decfact = dspinfo.osfactor;
   tmpdata = data;
   tmpdfilter = dfilter;

/*   tmpbuffer = buffer;
   for (i = 0; i < dspar->finalnp/2; i++)
     *tmpbuffer++ = 0.0;
*/
   tmpbuffer = buffer;
   data += dspinfo.offset;
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

/* i = 0; /* i = ntaps/2-((ntaps/2)/decfact)*decfact; */
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

/*
dspar->lp = ((dspar->dscoeff/2)/(dspar->dsfactor))*360.0;
printf("downsamp_par: OLD dspar->lp=%g\n",dspar->lp);
*/
 

/*-----------------------------------------------
|						|
|	           rotate2()/4			|
|						|
|   This function performs a one-dimensional	|
|   phase rotation on a single 1D spectrum or	|
|   2D trace.  The phase rotation vector is	|
|   calculated on the fly.			|
|						|
| nelems	 number of complex points	|
| spdata	 pointer to spectral data	|
| lpval		 first-order phasing constant	|
| rpval		 zero-order phasing constant	|
+----------------------------------------------*/
static void rotate2(float *spdata, int nelems, double lpval, double rpval)
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
/*   phi = (rpval + lpval)*conphi;
   lpval *= ( -conphi/((double) (nelems - 1)) );
*/
   phi = rpval * conphi;
   lpval *= conphi;

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

int dspSize(np,bytes)	/* fid size in bytes */
double np,bytes;
{
   int npdsp;

   npdsp = ((int)(np + 0.5)) * dspinfo.osfactor + (dspinfo.oscoeff - 1);
   npdsp *= (int) bytes;
   return( (int) npdsp);
}

void dsp_dcset(lvl, tlt)
float lvl,
      tlt;
{
    dspinfo.lvl = lvl;
    dspinfo.tlt = tlt;
}

#define STSIZE 128
void interp_phase(xpass, ypass)
float *xpass, *ypass; /* x,y input; x=cos2p,y=sin2p output */
{
  float sintable[STSIZE]; /* could declare as double or float */
  float x0, y0, xabs, yabs, xyabs, xyquadrant, xsgn, ysgn, ytmp, flag45, yy, aa;
  int i, px, py;

/*	x0 = (double)(*xpass); y0 = (double)(*ypass); */
	x0 = *xpass; y0 = *ypass;
	/* standard value */
	*ypass = 1.0/(x0 * x0 + y0 * y0);
	*xpass = *ypass * (x0 * x0 - y0 * y0);
	*ypass = *ypass * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
	printf("    STANDARD: cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2);

	/* double angle */
	xyabs = (x0 * x0 + y0 * y0);
        if (xyabs < 0.5)
        {
          *xpass = 1.0; *ypass = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
	  return;
        }
	xabs = x0 * x0 - y0 * y0;
	yabs = 2.0 * x0 * y0;
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

/*	if (yabs <= (xyabs * sintable[STSIZE/2])) /* test on 22.5, assign 11.25, STSIZE=8 */
/*	{
/*	  if (yabs <= (xyabs * sintable[STSIZE/4]))
/*	  {
/*	    /* 2p=11.25 */
/*	    py = STSIZE/4-1;
/*	  }
/*	  else
/*	  {
/*	    /* 2p=33.75 */
/*	    py = STSIZE/2-1;
/*	  }
/*	}
/*	else
/*	{
/*	  if (yabs <= (xyabs * sintable[3*STSIZE/4]))
/*	  {
/*	    /* 2p=56.25 */
/*	    py = 3*STSIZE/4-1;
/*	  }
/*	  else
/*	  {
/*	    /* 2p=78.75 */
/*	    py = STSIZE-1;
/*	  }
/*	}    */

/*	if (yabs <= (xyabs * sintable[STSIZE/2-1])) /* test on 11.25, assign on 22.5, STSIZE=8 */
/*	{
/*	  if (yabs <= (xyabs * sintable[STSIZE/4-1]))
/*	  {
/*	    /* 2p=0 */
/*	    py = 0;
/*	  }
/*	  else
/*	  {
/*	    /* 2p=22.5 */
/*	    py = STSIZE/4;
/*	  }
/*	}
/*	else
/*	{
/*	  if (yabs <= (xyabs * sintable[3*STSIZE/4-1]))
/*	  {
/*	    /* 2p = 45 */
/*	    py = 2*STSIZE/4;
/*	  }
/*	  else
/*	    if (yabs <= (xyabs * sintable[4*STSIZE/4-1]))
/*	    {
/*	      /* 2p=67.5 */
/*	      py = 3*STSIZE/4;
/*	    }
/*	    else
/*	    {
/*	      /* 2p=90 */
/*	      py = 4*STSIZE/4;
/*	    }
/*	}      */

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
	printf("xabs=%g yabs=%g xyabs=%g flag45=%g py=%d px=%d ", xabs,yabs,xyabs,flag45,py,px);
	xabs = sintable[px];
	yabs = sintable[py];
	xabs *= xsgn;
	yabs *= ysgn;
	xyabs = 90.0*((double)py)/((double)STSIZE);
	if (xsgn < 0) xyabs = 180.0 - xyabs;
	if (ysgn < 0) xyabs = 360.0 - xyabs;
	printf("cos2p=%g sin2p=%g 2p=%g\n",xabs,yabs,xyabs);
	*xpass = xabs; *ypass = yabs;
}

void interp2_phase(xpass, ypass)
float *xpass, *ypass; /* x,y input; x=cos2p,y=sin2p output */
{
  float sintable[STSIZE]; /* could declare as double or float */
  float x0, y0, xabs, yabs, xyabs, xyquadrant, xsgn, ysgn, ytmp, flag45, yy, aa;
  int i, px, py;

/*	x0 = (double)(*xpass); y0 = (double)(*ypass); */
	x0 = *xpass; y0 = *ypass;
	/* standard value */
	*ypass = 1.0/(x0 * x0 + y0 * y0);
	*xpass = *ypass * (x0 * x0 - y0 * y0);
	*ypass = *ypass * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
	printf("    STANDARD: cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2);

	/* double angle */
	xyabs = x0 * x0 + y0 * y0;
        if (xyabs < 0.5)
        {
          *xpass = 1.0; *ypass = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
	  return;
        }
	xabs = x0 * x0 - y0 * y0;
	yabs = 2.0 * x0 * y0;
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
	printf("xyabs=%g yabs=%g",xyabs,yabs);
	while (xyabs > 2) /* assume xyabs not less than 1 */
	{
	  xyabs *= 0.5;
	  yabs *= 0.5;
	}
	printf(" ... rescaled xyabs=%g yabs=%g\n",xyabs,yabs);
	aa = xyabs * xyabs; ytmp = yabs * yabs;
	printf("aa=%g ytmp=%g ",aa,ytmp);
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
	printf("yy=%g aa=%g aamax=%g\n", yy,aa,aa*STSIZE/2.0);
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
	printf("xabs=%g yabs=%g xyabs=%g flag45=%g py=%d px=%d ", xabs,yabs,xyabs,flag45,py,px);
	xabs = sintable[px];
	yabs = sintable[py];
	xabs *= xsgn;
	yabs *= ysgn;
	xyabs = 90.0*((double)py)/((double)STSIZE);
	if (xsgn < 0) xyabs = 180.0 - xyabs;
	if (ysgn < 0) xyabs = 360.0 - xyabs;
	printf("cos2p=%g sin2p=%g 2p=%g\n",xabs,yabs,xyabs);
	*xpass = xabs; *ypass = yabs;
}

void interp3_phase(xpass, ypass)
float *xpass, *ypass; /* x,y input; x=cos2p,y=sin2p output */
{
  float x0, y0, xabs, yabs, xyabs, xfactor, yy, aa, aa2, aa3;
  int i, px, py;

/*	x0 = (double)(*xpass); y0 = (double)(*ypass); */
	x0 = *xpass; y0 = *ypass;
	/* standard value */
	*ypass = 1.0/(x0 * x0 + y0 * y0);
	*xpass = *ypass * (x0 * x0 - y0 * y0);
	*ypass = *ypass * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
	printf("    STANDARD: cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2);

	/* approximate divide */
	xyabs = (x0 * x0 + y0 * y0);
        if (xyabs < 0.5)
        {
          *xpass = 1.0; *ypass = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
	  return;
        }
	aa = xyabs;
	printf("aa=%g ",aa);
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
	printf("... reduced aa=%g xfactor=%g\n",aa,xfactor);
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

	printf("final yy=%g ",yy);
	yy *= xfactor;
	printf("scaledup yy=%g true 1/a=%g\n",yy,1.0/xyabs); /* problem if a=0!! */
	*xpass = yy * (x0 * x0 - y0 * y0);
	*ypass = yy * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
	printf("cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2);
}

void interp4_phase(xpass, ypass)
float *xpass, *ypass; /* x,y input; x=cos2p,y=sin2p output */
{
  float x0, y0, xabs, yabs, xyabs, xfactor, yy, aa, aa2, aa3;
  int i, px, py;

/*	x0 = (double)(*xpass); y0 = (double)(*ypass); */
	x0 = *xpass; y0 = *ypass;
	/* standard value */
	*ypass = 1.0/(x0 * x0 + y0 * y0);
	*xpass = *ypass * (x0 * x0 - y0 * y0);
	*ypass = *ypass * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
	printf("    STANDARD: cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2);

	/* approximate divide */
	xyabs = (x0 * x0 + y0 * y0);
        if (xyabs < 0.5)
        {
          *xpass = 1.0; *ypass = 0.0;
DPRINT(DEBUG0,"dspExec(): WARNING!!!  first data point is zero\n");
	  return;
        }
	aa = xyabs;
	printf("aa=%g ",aa);
	xfactor = 1;
	while (aa >= 1)
	{
	  xfactor *= 0.5;
	  aa *= 0.5;
	}
	printf("... reduced aa=%g xfactor=%g\n",aa,xfactor);
	aa = xyabs;
	yy = 1.0 * xfactor;
	yy = yy * (2.0 - (aa * yy));
	yy = yy * (2.0 - (aa * yy));
	yy = yy * (2.0 - (aa * yy));
	yy = yy * (2.0 - (aa * yy));
	yy = (yy * (1.0 - (aa * yy))) + yy;

	printf("scaledup yy=%g true 1/a=%g\n",yy,1.0/xyabs); /* problem if a=0!! */
	*xpass = yy * (x0 * x0 - y0 * y0);
	*ypass = yy * 2.0 * x0 * y0;
	xyabs = (float)(180/M_PI*atan2((double)(*ypass), (double)(*xpass)));
	if (xyabs < 0.0) xyabs += 360.0;
	printf("cos2p=%g sin2p=%g 2p=%g p=%g\n", *xpass,*ypass,xyabs,xyabs/2);
}
