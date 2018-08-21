/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "oopc.h"
#include "acodes.h"
#include "acqparms.h"
#include "macros.h"
#include "rfconst.h"

extern int HSlines;
extern int curfifocount;
extern int acqiflag;
extern int bgflag;

extern FILE *sliderfile;

static int init_frqswp_offset[MAX_RFCHAN_NUM+1] = { 0, 0, 0, 0 };

/* frequency sweep structure */
struct freqstruct
{
   double          centerfreq;
   double          sweepwidth;
   double          units;
   double          np;
   double          mode;
   int             scale;
   double          max;
   double          min;
   int             device;
   char           *label;
};

/*-------------------------------------------------------------------
|
|	G_Sweep(variable arg_list)/n
|
|	Generate a Freqency sweep 
|
|				Author Greg Brissey 2/28/91
+------------------------------------------------------------------*/
/*VARARGS1*/
int 
G_Sweep(int firstkey, ...)
{
   struct freqstruct freqs;
   va_list         ptr_to_args;
   int             counter;
   int             keyword;
   int             initswp;

/* fill in the defaults */
   freqs.centerfreq = sfrq;
   freqs.sweepwidth = 4e6;	/* 4 MHz sweep width */
   freqs.np =  1024;		/* 1024 data points */
   freqs.device = TODEV;
   freqs.mode = 0.0;
   freqs.label  = "";
   freqs.scale  = 1;
   freqs.max    = sfrq+10.0;
   freqs.min    = sfrq-10.0;
   freqs.units  = 1;

/* initialize for variable arguments */
   va_start(ptr_to_args, firstkey);
   keyword = firstkey;
/* now check if the user want it his own way */
   while (keyword && ok)
   {
      switch (keyword)
      {
	 case FREQSWEEP_CENTER:
	    freqs.centerfreq =  va_arg(ptr_to_args, double);  /* MHz */
	    if (freqs.centerfreq < 0.0)
	       freqs.centerfreq = 0.0;
            initswp = 1;
	    break;
	 case FREQSWEEP_WIDTH:
	    freqs.sweepwidth =  va_arg(ptr_to_args, double);	/* Hz */
	    if (freqs.sweepwidth < 20000.0)
	       freqs.sweepwidth = 20000.0;
	    if (freqs.sweepwidth > 200000000.0)
	       freqs.sweepwidth = 200000000.0;
	    break;
	 case FREQSWEEP_NP:
	    freqs.np = va_arg(ptr_to_args, double);
	    break;
	 case FREQSWEEP_MODE:
	    freqs.mode = va_arg(ptr_to_args, double);
	    break;
	 case FREQSWEEP_INCR:
	    initswp = va_arg(ptr_to_args, int);
            initswp = 0;
	    break;
	 case FREQSWEEP_DEVICE:
	    freqs.device = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_LABEL:
	    freqs.label = va_arg(ptr_to_args, char *); /* Obs.,Dec.,2nd Dec. */
	    break;
	 case SLIDER_SCALE:
	    freqs.scale = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_MAX:
	    freqs.max = va_arg(ptr_to_args, double);
	    break;
	 case SLIDER_MIN:
	    freqs.min = va_arg(ptr_to_args, double);
	    break;
	 case SLIDER_UNITS:
	    freqs.units = va_arg(ptr_to_args, double);
	    break;
	 default:
	    fprintf(stdout, "wrong G_Pulse() keyword %d specified", keyword);
	    ok = FALSE;
	    break;
      }
      keyword = va_arg(ptr_to_args, int);
   }
   va_end(ptr_to_args);
/* if there was an error, here or in other calls to G_Pulse, G_Offset,	*/
/* G_Gain, or G_Delay, return here, checking only the validity of the	*/
/*  arguments								*/
   if (!ok)
      return;

   counter = (int) (Codeptr - Aacode);

   if (initswp)
   {
     SetRFChanAttr(RF_Channel[freqs.device],
           SET_SWEEPCENTER,	freqs.centerfreq, 
	   SET_SWEEPWIDTH, 	freqs.sweepwidth, 
	   SET_SWEEPNP,		freqs.np,
           SET_SWEEPMODE, 	0.0,
	   SET_INITSWEEP,	0.0,
	   NULL );

     /* Center Freq. needs only change the INITFREQ acode */
     if ( strcmp(freqs.label, "")  && acqiflag )
     {
        write_to_acqi(freqs.label, freqs.centerfreq, freqs.units, 
		      (int) freqs.min, (int) freqs.max, 
		      TYPE_FREQSWPCENTER,(int) freqs.scale, counter*2,
		      (int) freqs.device);
        write_to_acqi_swp(freqs.centerfreq, freqs.sweepwidth, freqs.np, 0.0);
     }
     init_frqswp_offset[freqs.device] = counter;
   }
   else
   {
     SetRFChanAttr(RF_Channel[freqs.device],
           SET_INCRSWEEP,	0.0, 
	   NULL );

     if ( strcmp(freqs.label, "")  && acqiflag )
     {
        Msg_Set_Param param;
        Msg_Set_Result result;
        int error;
        char msge[256];
	double swidthval, maxswidth;

        /* Get RF sweep width */
        param.setwhat=GET_SWEEPWIDTH;
        error = Send(RF_Channel[freqs.device],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
        if (error < 0)
        {
          sprintf(msge,"%s : %s\n",
		RF_Channel[freqs.device]->objname,ObjError(error));
          text_error(msge);
        }
	swidthval = result.DBreqvalue;

        /* Get max sweep width */
        param.setwhat=GET_SWEEPMAXWIDTH;
        error = Send(RF_Channel[freqs.device],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
        if (error < 0)
        {
          sprintf(msge,"%s : %s\n",
		RF_Channel[freqs.device]->objname,ObjError(error));
          text_error(msge);
        }
	maxswidth = result.DBreqvalue;
        printf("max sweep width = %lf, req. swpwidth = %lf\n",
		maxswidth,swidthval);

	/* check for values exceeding the maximum Sweep Width */ 
        if (swidthval  > maxswidth)
	   swidthval = maxswidth;
        if ((freqs.max * freqs.units) > maxswidth)
	   freqs.max = maxswidth / freqs.units;
        /* A change in sweep width must change both the INITFREQ & INCRFREQ acodes */
        write_to_acqi(freqs.label, swidthval, freqs.units, 
		      (int)freqs.min, (int)freqs.max, TYPE_FREQSWPWIDTH, 
		      (int)freqs.scale, init_frqswp_offset[freqs.device]*2,
		      (int) freqs.device);
        write_to_acqi(freqs.label, swidthval, freqs.units, 
		      (int)freqs.min, (int)freqs.max, TYPE_FREQSWPWIDTH, 
		      (int)freqs.scale, counter*2, (int) freqs.device );
     }
   }
}

/*-------------------------------------------------------------------
|
| write_to_acqi_swp(center, width, np , mode)/4
|
|				Author Greg Brissey 4/3/91
+------------------------------------------------------------------*/
write_to_acqi_swp(center, width, np, mode)
double center,width,np,mode;
{
FILE	*fopen();
char    filename[80];
char   *getenv();

   if (ix != 1) return;
   
   if (!sliderfile)  /* if not already open, open it */
   {
      strcpy(filename,getenv("vnmrsystem") );
      strcat(filename,"/acqqueue/acqi.IPA");
      sliderfile=fopen(filename,"w");
   }
   fprintf(sliderfile,"%.12lg %.12lg %lg %lg\n", center, width, np, mode);
}

/*-------------------------------------------------------------------
|
| sweepstart(center, width, rtvar1, rtvar2)/4
|
|				Author Greg Brissey 7/3/91
+------------------------------------------------------------------*/
sweepstart(center,width,rtvar1,rtvar2)
double center,width;
int rtvar1,rtvar2;
{
   sweep_setup(TODEV,center,width,np/2.0,"Center");
   initval((np/2.0),rtvar1);
   loop(rtvar1,rtvar2);
}

/*-------------------------------------------------------------------
|
| sweepend(rtvar2)/1
|
|				Author Greg Brissey 7/3/91
+------------------------------------------------------------------*/
sweepend(rtvar2)
int rtvar2;
{
   acquire(2.0,1.0e-6);
   stepfreq(TODEV);
   endloop(rtvar2);
}

/*-------------------------------------------------------------------
|
| sweep_setup(device,center,width,frqsteps,string)/5
|
|				Author Greg Brissey 7/3/91
+------------------------------------------------------------------*/
sweep_setup(device,center,width,frqsteps,string)
int device;
double center,width,frqsteps;
char *string;
{
   if ( (device > 0) && (device <= NUMch) ) 
   {
      /* initialize tune sweep parameters */
      G_Sweep( 	FREQSWEEP_DEVICE, device,
      		FREQSWEEP_CENTER, center,	/* MHz */
      		FREQSWEEP_WIDTH, width,		/* Hz  */
      		FREQSWEEP_NP, frqsteps,
      		SLIDER_LABEL, string,
      		NULL );
   }
   else
   {
      char msge[128];
      sprintf(msge,"sweep_setup: device #%d is not within bounds 1 - %d\n",
                  device, NUMch);
      abort_message(msge);
   }
}

/*-------------------------------------------------------------------
|
| stepfreq(device)/1
|
|				Author Greg Brissey 7/3/91
+------------------------------------------------------------------*/
stepfreq(device)
int device;
{
   if ( (device > 0) && (device <= NUMch) )
   {
      /* change frequency */
      G_Sweep( FREQSWEEP_DEVICE, device,
               FREQSWEEP_INCR, 0,
               SLIDER_LABEL, "Width",
               SLIDER_SCALE, 1 ,
               SLIDER_MAX, 20000.0,
               SLIDER_MIN, 1.0,
               SLIDER_UNITS, 1.0E3,
               NULL );
   }
   else
   {
      char msge[128];
      sprintf(msge,"stepfreq: device #%d is not within bounds 1 - %d\n",
                     device, NUMch);
      abort_message(msge);
   }
}
