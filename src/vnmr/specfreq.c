/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#define TRUE 1
#include "vnmrsys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "variables.h"
#include "group.h"
#include "params.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"

#ifdef SUN
#include "ACQ_HAL.h"
#include "ACQ_SUN.h"
#define  SUN_HAL		/* to keep oopc.h happy */
#endif 

extern int	Gflag;

#define COMPLETE 0
#define ERROR 1
#define CALLNAME 0
#define TRUE 1
#define FALSE 0
#define CALC 1
#define SHOW 0
#define MAXSTR 256

#ifdef  DEBUG
extern int      Gflag;

#define GPRINT0(level, str) \
	if (Gflag >= level) Wscrprintf(str)
#define GPRINT1(level, str, arg1) \
	if (Gflag >= level) Wscrprintf(str,arg1)
#define GPRINT2(level, str, arg1, arg2) \
	if (Gflag >= level) Wscrprintf(str,arg1,arg2)
#define GPRINT3(level, str, arg1, arg2, arg3) \
	if (Gflag >= level) Wscrprintf(str,arg1,arg2,arg3)
#define GPRINT4(level, str, arg1, arg2, arg3, arg4) \
	if (Gflag >= level) Wscrprintf(str,arg1,arg2,arg3,arg4)
#else 
#define GPRINT0(level, str)
#define GPRINT1(level, str, arg2)
#define GPRINT2(level, str, arg1, arg2)
#define GPRINT3(level, str, arg1, arg2, arg3)
#define GPRINT4(level, str, arg1, arg2, arg3, arg4)
#endif 

#undef ACTIVE
#include "oopc.h"       /* Object Oriented Programing Package */
#include "rfconst.h"

extern int  SetFreqAttr(Object, ...);
extern int  nvAcquisition();
extern int  getparm(char *varname, char *vartype, int tree,
                   void *varaddr, int size);
extern void get_solv_factor(double *solvfactor);
extern void get_lock_factor(double *lkfactor, double h1freq, char rftype);
extern int  getnucinfo(double h1freq, char rftype, char *nucname, double *freq,
               double *basefreq, char *freqband, char *syn, double *reffreq,
               int verbose);
extern int  send_transparent_data( c68int buf[], int len );
extern int  tunecmd_convert(c68int *buffer, int bufferlen);
extern char *ObjError(int wcode);
extern int setparm(char *varname, char *vartype, int tree, void *varaddr, int index);

int  bgflag;
int  curfifocount;
double dfrq;

static char	rftype[MAXSTR];
static char	rfband[MAXSTR];
static int	h1freq;
static int	ap_interface;

static double getdblattr(Object obj, int attr);
static double whatfrqstepsize(char *param);
static int whatrftype(int chan);
static int whatrfband(int chan);
static double whatiffreq(int rfchan);
static double whatbasefreq(int chan);
static double whatconstfreq(int chan, int h1freq);
static double whatptsoptions(int device);

/*#define MAX_RFCHAN_NUM 6	defined in rfconst.h */
#define  MAX_TUNE_CHAN  7       /* 3 control lines for tune switch 
                                     switch position #0 for normal operation
                                                     #1 - 7 for tunning      */

static int
init_statics()
{
	double	tmpval;

/*  Special note:  getparm writes an error message if it is not successful  */

	if (getparm("rftype", "STRING", SYSTEMGLOBAL, &rftype[0], MAX_RFCHAN_NUM+1))
	  return( -1 );
	if (getparm("rfband", "STRING", CURRENT, &rfband[0], MAX_RFCHAN_NUM+1))
	  return( -1 );
	if (getparm("h1freq", "REAL", SYSTEMGLOBAL, &tmpval, 1))
	  return( -1 );
	h1freq = (int) tmpval;

/*--------------------------------------------------------------*/
/*     determine type of ap_interface board that is present
   type 1 is 500 style attenuators,etc
   type 2 is ap interface introduced with the AMT amplifiers
   type 3 is 'type 2' with the additional fine linear attenuators
          for the solids application.				*/
/*--------------------------------------------------------------*/

	if ( P_getreal(SYSTEMGLOBAL,"apinterface",&tmpval,1) >= 0 ) {
		ap_interface = (int) (tmpval + 0.0005);
		if (( ap_interface < 1) || (ap_interface > 3)) {
			Werrprintf(
            "AP interface specified '%d' is not valid.. PSG Aborted.", ap_interface
			);
			return( -1 );
		}
	}
	else
          ap_interface = 1;       /* if not found assume ap_interface type 1 */

	return( 0 );
}

/*  The specfreq program puts the lock frequency and the solvent factor
    on the stack rather than allocating static space for them.  Thus the
    separate program and the 2 addresses as arguments.			*/

#ifdef XXX
static int
init_special_values(int *numch_addr )
{
	int	tnumch;
	double	tmpval;

	if ( P_getreal(SYSTEMGLOBAL,"numrfch",&tmpval,1) >= 0 ) {
		tnumch = (int) (tmpval + 0.0005);
		if (( tnumch < 1) || (tnumch > MAX_RFCHAN_NUM)) {
			Werrprintf(
		   "Number of RF Channels specified '%d' is too large..", tnumch
			);
			return( -1 );
		}
	}
	else
	  tnumch = 2;

	if (tnumch > MAX_RFCHAN_NUM) {
		Werrprintf(
	   "Number of Channels configured (%d) beyond the max (%d).",tnumch, MAX_RFCHAN_NUM
		);
		return( -1 );
	}
	else
	  *numch_addr = tnumch;

	return( 0 );
}
#endif

static
void freeObject(Object object )
{
	free( object->objname );
	free( object );
}

/*-------------------------------------------------------------------
|    SPECFREQ, DECFREQ, DEC2FREQ:
|
|		Usage:	spcfrq(frequency MHz)   or spcfrq
|			decfreq(frequency MHz)  or decfreq
|			dec2freq(frequency MHz) or dec2freq
|			dec3freq(frequency MHz) or dec3freq
|
|   Purpose:
|  	This module displays or sets the spectrometer frequency
| 	(Transmitter or Decoupler)
|
|   Parameters altered:
|	{tn,tbo,tof,sfrq}, {dn,dbo,dof,dfrq}, or {dn2,dof2,dfrq2}
|
|   Routines:
| 	specfreq  -  specfreq main function.
|	calcFrequency - returns calc PTS synthesizer frequency,
|			w/ cflag set then Base Frequency Offset
|				synthesizer frequency is calc and altered.
|			        frequency Offset is set to Zero.
|	showfrequency - displays the spectrometer frequency, Base Offset
|			frequency, and PTS synthesizer freqency.
|	getparm - retrieves the values of REAL or STRING parameters.
|	setparm - stores the values of REAL or STRING parameters.
|	Author   Greg Brissey   4/28/86
|	Modified for no tbo,dbo  Greg B. 8/27/87
|	Modified for 2nd decoupler S. Farmer 1/11/90
|       Modified to use Freq Objects Greg Brissey 7/24/90
/+-------------------------------------------------------------------*/

/*~~~~~~~~~~~~~~~~~~~~~~  Main Program ~~~~~~~~~~~~~~~~~~~~~~*/
int specfreq(int argc, char *argv[], int retc, char *retv[])
{
   char            nucname[20];
   char		   msge[128];
   extern double   calcFrequency();
   extern int      Freq_Device();
   extern double   whatbasefreq(), whatconstfreq();
   extern double   whatptsoptions();
   extern double   whatiffreq();
   extern double   whatamphibandmin();
   extern double   whatfrqstepsize();
   extern Object   ObjectNew();
   double          tbo;		/* transmitter base offset frequency */
   double          bfrq;	/* transmitter frequency */
   double          offrq;	/* transmitter offset frequency */
   double          reqfrq = 0.0;	/* requested frequency */
   double          pts;		/* pts frequency */
   double          ptsval;	/* PTS type identifier */
   double          spcfrq;	
   int             calcsyn;
   Object	   FreqObj;	/* frequency channel selected */
   int		   rfchan;
   double 	   tmpval;	
   int		   NUMch; 	/* number of channels configured */
   char		   frqname[64],ofname[64],tnname[64],tboname[64],channame[64];
   double	   getdblattr();


   if (2 < argc)
   {
      Werrprintf("Usage - spcfrq(<freq>), decfrq(<freq>), dec2frq(<freq>) or dec3freq(,freq>)");
      ABORT;
   }

   bgflag = Gflag;  		/* debug flag in Objects */

   tbo = bfrq = offrq = 0.0;	/* initialize to zero for observe */

/* --- spcfrq, decfrq, or dec2frq called? --- */
   if (strcmp(argv[CALLNAME], "spcfrq") == 0)
   {
      strcpy(frqname,"sfrq");
      strcpy(ofname,"tof");
      strcpy(tnname,"tn");
      strcpy(tboname,"Transmitter Offset");
      strcpy(channame,"Spectrometer");
      rfchan = TODEV;
   }
   else if (strcmp(argv[CALLNAME], "decfrq") == 0)
   {
      strcpy(frqname,"dfrq");
      strcpy(ofname,"dof");
      strcpy(tnname,"dn");
      strcpy(tboname,"Decoupler Offset");
      strcpy(channame,"Decoupler");
      rfchan = DODEV;
   }
   else if (strcmp(argv[CALLNAME], "dec2frq") == 0)
   {
      strcpy(frqname,"dfrq2");
      strcpy(ofname,"dof2");
      strcpy(tnname,"dn2");
      strcpy(channame,"2nd Decoupler");
      rfchan = DO2DEV;
   }
   else if (strcmp(argv[CALLNAME], "dec3frq") == 0)
   {
      strcpy(frqname,"dfrq3");
      strcpy(ofname,"dof3");
      strcpy(tnname,"dn3");
      strcpy(channame,"3rd Decoupler");
      rfchan = RFCHAN4;
   }
   else
   {
      Werrprintf("Illegal Alias: %s", argv[CALLNAME]);
      return ERROR;
   }

   if ( P_getreal(SYSTEMGLOBAL,"numrfch",&tmpval,1) >= 0 )
   {
       NUMch = (int) (tmpval + 0.0005);
       if (( NUMch < 1) || (NUMch > MAX_RFCHAN_NUM))
       {
           Werrprintf(
           "Number of RF Channels specified '%d' is too large..",
              NUMch);
            return ERROR;
        }
   }
   else
      NUMch = 2;

   if (NUMch > MAX_RFCHAN_NUM)
   {
      Werrprintf(
	"Number of Channels configured (%d) beyond the max (%d).",NUMch,
          MAX_RFCHAN_NUM);
      return ERROR;
   }

   if (rfchan > NUMch)
   {
      Werrprintf(
	"%s: Channel is not configured.",argv[CALLNAME]);
      return ERROR;
   }

    /* --- determine type of ap_interface board that is present --- 
     * type 1 is 500 style attenuators,etc                         
     * type 2 is ap interface introduced with the AMT amplifiers  
     * type 3 is 'type 2' with the additional fine linear attenuators
     *        for the solids application.
     *-----------------------------------------------------------------
     */
    if ( P_getreal(SYSTEMGLOBAL,"apinterface",&tmpval,1) >= 0 )
    {
        ap_interface = (int) (tmpval + 0.0005);
        if (( ap_interface < 1) || (ap_interface > 3))
        {
            Werrprintf(
                "AP interface specified '%d' is not valid.. PSG Aborted.",
              ap_interface);
	    return ERROR;
        }
    }
    else
        ap_interface = 1;       /* if not found assume ap_interface type 1 */
 
/*  "rftype", "rfband", etc. are strings with 1 char for each channel.
    The string-length parameter to the getparm program must include
    space for the '\0' character; thus it is expected string size + 1.	*/

   if (getparm(tnname, "STRING", CURRENT, nucname, 6))
	 return ERROR;
   if (getparm("rftype", "STRING", SYSTEMGLOBAL, &rftype[0], MAX_RFCHAN_NUM+1))
      return COMPLETE;
   if (getparm("rfband", "STRING", CURRENT, &rfband[0], MAX_RFCHAN_NUM+1))
      return COMPLETE;
   if (getparm("h1freq", "REAL", SYSTEMGLOBAL, &tmpval, 1))
      return COMPLETE;
   h1freq = (int) tmpval;
   if (getparm(frqname, "REAL", CURRENT, &bfrq, 1))
      return ERROR;
   if (getparm(ofname, "REAL", CURRENT, &offrq, 1))
      return ERROR;

   /*--- obtain PTS values for channel configuration ---*/
   if ( P_getreal(SYSTEMGLOBAL,"ptsval",&tmpval,rfchan) >= 0 )
     ptsval = (int) (tmpval + .0005);
   else
     ptsval = 0;                /* no pts for decoupler */


   GPRINT3(1, "rftype = %s, rfband = %s, h1freq = %8.4lf\n",
	   rftype, rfband, h1freq);

/* --- calc freq or just display --- */
   if (argc <= 1)
   {
      calcsyn = FALSE;		/* a freq passed ? */
   }
   else
   {
      calcsyn = TRUE;
      GPRINT1(1, "Param String: %s \n", argv[1]);
      if (isReal(argv[1]))
      {
	 reqfrq = stringReal(argv[1]);
	 offrq = 0.0;
      }
      else
      {
	 Werrprintf("Parameter is Not a Real Type.");
	 return ERROR;
      }
   }


   sprintf(msge,"Channel %d Transmitter",rfchan);
   FreqObj = ObjectNew(Freq_Device,msge);
   if ( SetFreqAttr(FreqObj,SET_DEVCHANNEL,      (double) rfchan,
               SET_H1FREQ,            (double) h1freq,
               SET_PTSVALUE,          (double) ptsval,
               SET_IFFREQ,            (double) whatiffreq(rfchan),
               SET_FREQSTEP,          (double) whatfrqstepsize(ofname),
               SET_RFTYPE,            (double) whatrftype(rfchan),
               SET_RFBAND,            (double) whatrfband(rfchan),
               SET_OSYNBFRQ,          (double) whatbasefreq(rfchan),
               SET_OSYNCFRQ,          (double) whatconstfreq(rfchan,h1freq),
               SET_PTSOPTIONS,        (double) whatptsoptions(rfchan),
               SET_OVRUNDRFLG,        (double) 0,
	       SET_SPECFREQ,	      (double) bfrq,  /* sfrq,dfrq,dfrq2,... */
	       SET_INIT_OFFSET,	      (double) offrq, /* tof,dof,dof2,.....  */
               NULL)
       < 0 )
	return ERROR;

   if (calcsyn == TRUE) /* add in corrections, so when removed get right freq */
   {
      bfrq = reqfrq;
      if ( SetFreqAttr(FreqObj, SET_SPECFREQ, (double) bfrq, NULL)
           < 0 )
	  return ERROR;
   }


    /* rfbandval = getdblattr(FreqObj,GET_RFBAND); */
    spcfrq = ( getdblattr(FreqObj,GET_SPEC_FREQ) * 1.0e-6);
    tbo = ( getdblattr(FreqObj,GET_OF_FREQ) * 1.0e-6 );
    if ( (rftype[rfchan-1] == 'b') || (rftype[rfchan-1] == 'B') )
       pts = getdblattr(FreqObj,GET_PTS_FREQ);
    else
       pts = ( getdblattr(FreqObj,GET_PTS_FREQ) * 1.0e-6 );

   if (calcsyn == TRUE)
   {
      rfband[rfchan-1] = 'c';   /* set rfband to calculate */
      setparm("rfband", "STRING", CURRENT, rfband, 1);
      setparm(frqname, "REAL", CURRENT, &bfrq, 1);
      setparm(ofname, "REAL", CURRENT, &offrq, 1);
      /* setparm("dbo", "REAL", CURRENT, &dbo, 1); tbo,dbo are not used */
   }

   if (retc)
      retv[0] = realString(spcfrq);
   else
   {
      Wscrprintf("%s Frequency = %11.7lf MHz,   Nucleus: %s \n",
		 channame, spcfrq, nucname);

      if (rftype[rfchan-1] != 'c' && rftype[rfchan-1] != 'd')
	 Wscrprintf("%s = %11.7lf MHz\n", tboname, tbo);
      if (rftype[rfchan-1] != 'a')
      {
	 Wscrprintf("Synthesizer Frequency = %11.7lf MHz\n", pts);
      }
   }

   free( FreqObj->objname );
   free( FreqObj );

   return COMPLETE;
}


/*  Special note:  programs send error messages to stderr as well as
                   using Werrprintf.  While this is not a perfect
                   solution to the problem of multiple error messages
                   each overwriting the previous, it does allow all
                   to be seen, provided one opens the window from
                   which this program was started.			*/

/*--------------------------------------------------------------------
|  getdblattr( object, attribute )
|      get double precision attribute of an object
+--------------------------------------------------------------------*/
static double getdblattr(Object obj, int attr)
{
    Msg_Set_Param   param;
    Msg_Set_Result  result;
    int		    error;

    param.setwhat=attr;
    error = Send(obj,MSG_GET_FREQ_ATTR_pr,&param,&result);
    if (error < 0)
    {
      Werrprintf("%s : %s",obj->objname,ObjError(error));
      fprintf(stderr, "%s : %s\n",obj->objname,ObjError(error));
      return ERROR;
    }
    return(result.DBreqvalue);
}

/*--------------------------------------------------------------------
|  getintattr( object, attribute, result_addr )
|      get integer attribute of an object
|      unlike get double precisiion attribute, this program
|      returns a status.  The requested value is returned
|      via the argument list
+--------------------------------------------------------------------*/
static int
getintattr(Object obj, int attr, int *raddr)
{
    Msg_Set_Param   param;
    Msg_Set_Result  result;
    int		    error;

    param.setwhat=attr;
    error = Send(obj,MSG_GET_FREQ_ATTR_pr,&param,&result);
    if (error < 0)
    {
      Werrprintf("%s : %s",obj->objname,ObjError(error));
      fprintf(stderr, "%s : %s\n",obj->objname,ObjError(error));
      return ERROR;
    }

    *raddr = result.reqvalue;
    return COMPLETE;
}


/*--------------------------------------------------------------------*/

static c68int	*putcode_base = NULL;
static c68int	*putcode_addr = NULL;
static int	 putcode_size = 0;

static int
start_putcode(c68int *bufaddr, int bufsize )
{
	putcode_base = bufaddr;
	putcode_addr = bufaddr;
	putcode_size = bufsize;

	return( 0 );
}

static void stop_putcode()
{
	putcode_addr = NULL;
	putcode_base = NULL;
	putcode_size = 0;
}


/* PSG functions that have been altered or dummied for vnmr */

void abort_message(const char *format, ...)
{
   va_list vargs;
   char msge[MAXSTR];

   va_start(vargs, format);
   vsprintf(msge,format,vargs);
   va_end(vargs);
   Werrprintf("%s",msge);
   fprintf(stderr,"%s\n",msge);
}
void text_error(const char *format, ...)
{
   va_list vargs;
   char msge[MAXSTR];

   va_start(vargs, format);
   vsprintf(msge,format,vargs);
   va_end(vargs);
   Werrprintf("%s",msge);
   fprintf(stderr,"%s\n",msge);
}

void putcode(c68int arg)
{
	int	space_taken;

	if (putcode_addr == NULL)
	  return;
	space_taken = putcode_addr - putcode_base;
	if (space_taken >= putcode_size || space_taken < 0)
	  return;

	*putcode_addr = arg;
	putcode_addr++;
}
void formPTSwords()
{
}
void formXLwords()
{
}
/*--------------------------------------------------------------------
| whatfrqstepsize() obtains frequency step size of the channel
|  		    using the stepsize of the freq. offset parameter
|		    (e.g. tof,dof,dof2)
+---------------------------------------------------------------------*/
static double whatfrqstepsize(char *param)
{
   double stepsize;
   int   pindex;
   vInfo  varinfo;

    /*--- determine frequency step size of channel tof, dof, dof2  etc. ---*/
    if ( P_getVarInfo(CURRENT,param,&varinfo) )
    {   
	Werrprintf("whatfrqstepsize: Cannot find the variable: '%s'",
	     param);
	 return ERROR;
    }
    if (varinfo.prot & P_MMS) {     /* stepsize an index into parstep ? */
        pindex = (int) (varinfo.step+0.1);
        if (P_getreal( SYSTEMGLOBAL, "parstep", &stepsize, pindex ))
        {
            Werrprintf("whatfrqstepsize: Could not find parstep[%d]",pindex);
	    return ERROR;
        }
        if (Gflag)
           fprintf(stdout,
		"whatfrqstepsize: parstep[%d] = %6.1f (stepsize Hz) for '%s'\n",
                pindex,stepsize,param);
    }
    else {
        stepsize = varinfo.step;
        if (Gflag)
           fprintf(stdout,"whatfrqstepsize: vInfo.step = %6.1f (stepsize Hz) for '%s' \n",
		stepsize,param);
    }
    return(stepsize);
}
/*--------------------------------------------------------------------
| whatrftype() obtains RF type for the channel from rftype parameter
+---------------------------------------------------------------------*/
static int whatrftype(int chan)
{
    char type;
    type = rftype[chan-1];
/*    fprintf(stderr,"whatrftype(): type=%c\n",type); */
    if ( (type == 'd') || (type == 'D') )
    {
      return(DIRECT_NON_DBL);
    }
    else if ( (type == 'c') || (type == 'C') )
    {
      return(DIRECTSYN);
    }
    else if ( (type == 'b') || (type == 'B') )
    {
      return(OFFSETSYN);
    }
    else if ( (type == 'a') || (type == 'A') )
    {
      return(FIXED_OFFSET);
    }
    else if ( (type == 'm') || (type == 'M') )
    {
      return(IMAGE_OFFSETSYN);
    }
    else
      return(0);
}
/*--------------------------------------------------------------------
| whatrfband() obtains RF band for the channel from rfband parameter
+---------------------------------------------------------------------*/
static int whatrfband(int chan)
{
    char type;
    type = rfband[chan-1];
    if ( (type == 'h') || (type == 'H') )
    {
      return(RF_HIGH_BAND);
    }
    else if ( (type == 'l') || (type == 'L') )
    {
      return(RF_LOW_BAND);
    }
    else if ( (type == 'c') || (type == 'C') || (type == 'd') || (type == 'D') )
    {
      return(RF_BAND_AUTOSELECT);
    }
    else
      return(0);
}
/*--------------------------------------------------------------------
| whatiffreq() obtains iffreq of the channel, assumes iffreq is an array
|  		having values for each channel, if no value defaults 
|		defaults to 10.5
+---------------------------------------------------------------------*/
static double whatiffreq(int rfchan)
{
    double iffreq;

    if ( P_getreal(SYSTEMGLOBAL,"iffreq",&iffreq,rfchan) < 0 )
        iffreq = 10.5;               /* if not found assume IF Freq 10.5 MHz */
    return( iffreq );
}
/*--------------------------------------------------------------------
| whatbasefreq() obtains Offset Synthesizer Base frequency based on 
|		 rftype parameter. (differs only between SISCO & VNMRID)
+---------------------------------------------------------------------*/
static double whatbasefreq(int chan)
{
    char type;
    type = rftype[chan-1];
    if ( (type == 'm') || (type == 'M') )
    {
      return(1.50);	/* SISCO 1.50 Mhz for offset syn */
    }
    else
      return(1.45);	/* 1.45 MHz offset syn */
}
/*--------------------------------------------------------------------
| whatconstfreq() obtains broad band of Fix frequency Base from 
|		 rftype & h1freq parameters. 
+---------------------------------------------------------------------*/
static double whatconstfreq(int chan, int h1freq)
{
  char type;
  type = rftype[chan-1];
  if ( (type != 'a') && (type != 'A') )
  {
    if ( (type == 'm') || (type == 'M') )
    {
      return(20.5);	/* SISCO 20.5 MHz  */
    }
    else if  (h1freq > 360)
    {
      return(205.5);
    }
    else
      return(158.5);
   }
  else
  {
    if (h1freq < 210)   /* instrument proton frequency, selects IF freq for 'a' */
      return(198.5);
    else if (h1freq < 310)
      return(298.5);
    else if (h1freq < 410)
      return(398.5);
    else
    {
       Werrprintf("No fix frequency board for h1freq greater than 400");
       return ERROR;
    }
  }
}
/*--------------------------------------------------------------------
| whatptsoptions() obtains the 2 options of the PTS (latching,over/under ranging)
|		   also determine type of control required by rftype and
|		   ap_interface parameters. 
+---------------------------------------------------------------------*/
static double whatptsoptions(int device)
{
    char latch[MAXSTR];
    char overundr[MAXSTR];
    int latchlen,overundrlen;
    int ptsoptmask;

    ptsoptmask = 0;

    if (P_getstring(SYSTEMGLOBAL, "latch", latch, 1, MAXSTR) < 0)
       strcpy(latch,"");
    latchlen = strlen(latch);
    if (P_getstring(SYSTEMGLOBAL, "overrange", overundr, 1, MAXSTR) < 0)
       strcpy(overundr,"");
    overundrlen = strlen(overundr);

   if ((rftype[device-1] == 'c') && (ap_interface < 2))
   {
      ptsoptmask = (1 << USE_SETPTS);
     return( (double) ptsoptmask );	/* Use SETPTS else do not */
   }
   else
   {
     if (latchlen >= device)		/* no entry for dev, equiv to 'n' */
     {
       if (latch[device-1] == 'y')	/* latching pts ? */
         ptsoptmask |= (1 << LATCH_PTS);
     }
     if (overundrlen >= device)		/* no entry for dev, equiv to 'n' */
     {
       if (overundr[device-1] == 'y')	/* over/under ranging pts ? */
         ptsoptmask |= (1 << OVR_UNDR_RANGE);
     }
     return( (double) ptsoptmask );
   }
}

/*-------------------------------------------------------------------
| whatptsval(rfchan) -  obtain the PTS synthesizer value for this
|			RF channel.  New for the tune program.
-------------------------------------------------------------------*/
static double whatptsval(int rfchan)
{
	double	ptsval;

   /*--- obtain PTS values for channel configuration ---*/

	if ( P_getreal(SYSTEMGLOBAL,"ptsval",&ptsval,rfchan) < 0 )
	  ptsval = 0.0;                /* no pts for decoupler */

	return( ptsval );
}

static char *whatoffsetparam(int rfchan)
{
	char	*romstr;	/* you are not supposed to write to this address */
   romstr = "tof";    /** put in by chin for the error "can not find the variable 'dof2'
                                                        of whatfrqstepsize    **/

        (void) rfchan;
/*	if (rfchan == TODEV)
	  romstr = "tof";
	else if (rfchan == DODEV)
	  romstr = "dof";
	else if (rfchan == DO2DEV)
	  romstr = "dof2";
	else if (rfchan == RFCHAN4)
	  romstr = "dof3";
	else
	  romstr = "";
*/

	return( romstr );
}


/*-------------------------------------------------------------------
    Special entry point called from freq_device.c
    In PSG it is present in initfreqobj and programs PTS 1
    to tune on this channel.  In VNMR it is a dummy routine
    which displays an error message since it should not be
    called from VNMR.  The message shows how it would happen.
-------------------------------------------------------------------*/

/*  FreqObj actually the address of a Frequency Object */
void tune_from_freq_obj(char *FreqObj, int dev_channel )
{
   (void) FreqObj;
   Werrprintf( "SET_VALUE called for Frequency Object for channel %d", dev_channel );
}

int mapRF(int index )
{
   return(index);
}

int
validate_imaging_config(char *callname)
{
   (void) callname;
   return(1);
}

/*-------------------------------------------------------------------
    Tune programs 
-------------------------------------------------------------------*/

static char	*tunechan_names[][ MAX_TUNE_CHAN ] = {
	{ "todev", "dodev", "do2dev", "do3dev", "", "", "" },
	{ "ch1", "ch2", "ch3", "ch4", "ch5", "ch6", "ch7" }
};

/***** the below array def. being used anywhere ? if not, it should be removed *****/
/* static char	*offset_param[ MAX_TUNE_CHAN ] = {
	"tof",
	"dof",
	"dof2",
	"dof3",
	"",
	""
};
*/

/** Converts tune channel names such as ch1, ch2, ch3... to 1, 2, 3... accordingly **/
static int tune_channame_to_number(char *name )
{
	int	iter, iter_2, tsize;

	tsize = sizeof( tunechan_names ) / (sizeof( char * ) * MAX_TUNE_CHAN);

	for (iter = 0; iter < tsize; iter++)
	  for (iter_2 = 0; iter_2 < MAX_TUNE_CHAN; iter_2++)
	    if (strcmp( name, tunechan_names[ iter ][ iter_2 ] ) == 0)
	      return( iter_2+1 );
					/* channels are numbered starting at 1 */
	return( -1 );			/* come here if never found */
}

static int do_tune_with_freq(int channel, char *freqstr )
{
	char		*ofname;
	c68int		*tunebuf;
 	char		 msge[128];
	int		 bufsize, tunesize, tunebufsize, non_nessie_size;
	double		 frequency;
	Object	   	 FreqObj;
	extern int       Freq_Device();
        extern Object   ObjectNew();

	frequency = stringReal( freqstr );
	ofname = whatoffsetparam(channel);
	sprintf(msge,"Channel %d Transmitter",channel);
	FreqObj = ObjectNew(Freq_Device,msge);

	if (SetFreqAttr( FreqObj,
		SET_APADR,		(double) 0x7,	/* Hydra and non-Hydra */
		SET_APREG,		(double) 0x21,	/*  ***Hydra ONLY***  */
		NULL
	) < 0) {
		freeObject( FreqObj );
		return( -1 );
	}

/*  If tuning with (numeric) frequency, the lock frequency, the solvent factor
    and the offset frequency are specified to be 0.

    The h1freq number was set in init_statics().				*/
    
	if ( SetFreqAttr(FreqObj,
	       SET_DEVCHANNEL,        (double) channel,
               SET_H1FREQ,            (double) h1freq,
               SET_PTSVALUE,                   whatptsval(TODEV),
               SET_IFFREQ,            (double) whatiffreq(TODEV),
               SET_FREQSTEP,                   whatfrqstepsize(ofname),
               SET_RFTYPE,            (double) whatrftype(TODEV),
               SET_RFBAND,            (double) whatrfband(TODEV),
               SET_OSYNBFRQ,          (double) whatbasefreq(TODEV),
               SET_OSYNCFRQ,          (double) whatconstfreq(TODEV,h1freq),
               SET_PTSOPTIONS,        (double) whatptsoptions(TODEV),
               SET_OVRUNDRFLG,                 0.0,
	       SET_SPECFREQ,	               frequency,
	       SET_INIT_OFFSET,	               0.0,
               NULL
	) < 0 ) {
		freeObject( FreqObj );
		return( -1 );
	}

	if (getintattr( FreqObj, GET_SIZETUNE, &tunesize ) != COMPLETE) {
		freeObject( FreqObj );
		return( -1 );
	}

/*  Bufsize equals original size for the tune frequency
      + 1 for the count
      + 1 for the channel
      + 1 for the SET TUNE FREQUENCY console command	*/

     	non_nessie_size = tunesize + 3; /* this size will be sent to non_nessie
                 tunecmd_convert() for doing nothing, same size be returned  */

        bufsize = tunesize + 11; /* make buffer larger to accommodate nessie codes */
	tunebuf = (c68int *) malloc( bufsize * sizeof( c68int ) );

/*  Send address of 2nd word in the buffer to the putcode system.
    This program will place SET TUNE FREQUENCY in the first word.  */

	start_putcode( &tunebuf[ 1 ], bufsize-1 );
	SetFreqAttr( FreqObj, SET_TUNE_FREQ, 0.0, NULL );
	stop_putcode();
	tunebuf[ 0 ] = SET_TUNE;

/* tunecmd_convert() is in socket.c . If NESSIE defined the pts codes will
   be converted to nessie codes and new tunebufsize be returned , else the
   same old codes and size be returned   */
        tunebufsize = tunecmd_convert(tunebuf, non_nessie_size);

	send_transparent_data( tunebuf, tunebufsize );

	free( tunebuf );
	freeObject( FreqObj );

	return( 0 );
}

static int
do_tune_with_nucleus(int channel, char *nucleus )
{
	char		*ofname;
	char		 frqband[ 10 ], synthesizer[ 10 ], msge[128];
	c68int		*tunebuf;
	int		 bufsize, tunesize;
	int		 tunebufsize, non_nessie_size;
	double		 lkfactor, nucfreq, nucbaseoffset, reffreq, solvfactor;
	Object	   	 FreqObj;
	extern int       Freq_Device();
        extern Object   ObjectNew();

	/* use channel 1 for tuning */
	/* this was checked earlier; we expect it to work now  */
	if (getnucinfo( (double) h1freq, rftype[0], nucleus, &nucfreq,
                &nucbaseoffset, &frqband[0], &synthesizer[0], &reffreq, 1) != 0)
	  return( -1 );

	ofname = whatoffsetparam(channel);		/* "tof", "dof", etc. */
	sprintf(msge,"Channel %d Transmitter",channel);
	FreqObj = ObjectNew(Freq_Device,msge);

	if (SetFreqAttr( FreqObj,
		SET_APADR,		(double) 0x7,	/* Hydra and non-Hydra */
		SET_APREG,		(double) 0x21,	/*  ***Hydra ONLY***  */
		NULL
	) < 0) {
		freeObject( FreqObj );
		return( -1 );
	}

	get_lock_factor( &lkfactor, (double) h1freq, 'd' );
	get_solv_factor(&solvfactor);

	nucfreq *= 1.0e6;				/* put frequency into Hz */
	nucfreq += (nucfreq * lkfactor);
	GPRINT1(1, "lock corrected freq= %14.8f MHz\n", nucfreq * 1e-6);
	nucfreq -= (nucfreq * solvfactor);
	GPRINT1(1, "solvent corrected freq= %14.8f MHz\n", nucfreq * 1e-6);
	nucfreq *= 1e-6;				  /* convert back to MHz */
    
	if ( SetFreqAttr(FreqObj,
	       SET_DEVCHANNEL,        (double) channel,
               SET_H1FREQ,            (double) h1freq,
               SET_PTSVALUE,                   whatptsval(TODEV),
               SET_IFFREQ,            (double) whatiffreq(TODEV),
               SET_FREQSTEP,                   whatfrqstepsize(ofname),
               SET_RFTYPE,            (double) whatrftype(TODEV),
               SET_RFBAND,            (double) whatrfband(TODEV),
               SET_OSYNBFRQ,          (double) whatbasefreq(TODEV),
               SET_OSYNCFRQ,          (double) whatconstfreq(TODEV,h1freq),
               SET_PTSOPTIONS,        (double) whatptsoptions(TODEV),
               SET_OVRUNDRFLG,                 0.0,
	       SET_SPECFREQ,	               nucfreq,
	       SET_INIT_OFFSET,	               0.0,
               NULL
	) < 0 ) {
		freeObject( FreqObj );
		return( -1 );
	}

	if (getintattr( FreqObj, GET_SIZETUNE, &tunesize ) != COMPLETE) {
		freeObject( FreqObj );
		return( -1 );
	}

        non_nessie_size = tunesize + 3; /* this size will be sent to non_nessie
                 tunecmd_convert() for doing nothing, same size be returned  */

	bufsize = tunesize + 11; /* make buffer larger to accommodate nessie codes */
	tunebuf = (c68int *) malloc( bufsize * sizeof( c68int ) );

/*  Send address of 2nd word in the buffer to the putcode system.
    This program will place SET TUNE FREQUENCY in the first word.  */

	start_putcode( &tunebuf[ 1 ], bufsize-1 );
	SetFreqAttr( FreqObj, SET_TUNE_FREQ, 0.0, NULL );
	stop_putcode();
	tunebuf[ 0 ] = SET_TUNE;

/* tunecmd_convert() is in socket.c . If NESSIE defined the pts codes will
   be converted to nessie codes and new tunebufsize be returned , else the
   same old codes and size be returned   */
        tunebufsize = tunecmd_convert(tunebuf, non_nessie_size);

	send_transparent_data( tunebuf, tunebufsize );

	free( tunebuf );
	freeObject( FreqObj );

	return( 0 );
}


/*  Description of tune describes 2 formats.  The tune program decides
    which one to select and calls the corresponding program.  The tune
    program itself is but a Shell.					*/

/*  An official VNMR command has either RETURN or ABORT to exit.
    I don't like a VNMR command returning a value from a subprogram.
    One should read the VNMR command program and decide there what
    works and what aborts.

    A method returns 0 if successful and -1 if it fails.  I have
    written the 2 working tune programs as methods.  If either
    becomes a VNMR command, replace return( 0 ) with RETURN and
    return( -1 ) with ABORT.

    Each working tune program however is written to work by itself.
    Thus you may see a few additional checks that it would appear
    the tune command itself already did.				*/

/*  The 1st format assigns frequecies to channels implicitly.  Each
    argument represents a frequency.  Channel 1 receives the 1st;
    channel 2 receives the 2nd; etc.  Further channels are not affected.*/

static int
tune_1st_format(int argc, char *argv[], int retc, char *retv[])
{
   int	iter;

   (void) retc;
   (void) retv;
   if (argc < 2)
   {
      Werrprintf( "%s:  command requires argument(s).", argv[ CALLNAME ] );
      return( -1 );
   }

/*  Program assumes 1st channel is numbered 1; we therefore
    pass iter as the channel number to lookup nucleus.
    If the parameter is a real number, we assume it is OK.
    If not, we look it up in the nucleus tables. */


/*  Real work starts here.  */

   for (iter = 1; iter < argc; iter++)
   {
      if (isReal( argv[ iter ] ))
         do_tune_with_freq( iter, argv[ iter ] );
      else
         do_tune_with_nucleus( iter, argv[ iter ] );
   }

   return( 0 );
}


/*****  The 2nd format assigns frequencies directly.  Arguments
        are pairs of channel name, and follow by either nucleus or frequency.  *****/

static int
tune_2nd_format(int argc, char *argv[], int retc, char *retv[])
{
   int	channel, iter;

   (void) retc;
   (void) retv;
   if (argc < 2)
   {
      Werrprintf( "%s:  command requires argument(s).", argv[ CALLNAME ] );
      return( -1 );
   }
   else if ((argc % 2) != 1)
        {
           Werrprintf(
	      "%s:  its parameters must be channel/frequency pairs", argv[ CALLNAME ]
		     );
           Wscrprintf(
	      "Number of parameters do not imply this, use an even number\n"
		);
	   return( -1 );
	}

   for (iter = 1; iter < argc; iter += 2)
   { 
      channel = tune_channame_to_number( argv[ iter ] );
      if (channel < 1)
      {
         Werrprintf(
            "%s:  %s is not the name of an RF channel", argv[ CALLNAME ], argv[ iter ]
	           );
         return( -1 );
      }
      else if (channel > MAX_TUNE_CHAN)
           {
              Werrprintf(
                 "%s:   %s is not valid for tune command", argv[ CALLNAME ], argv[iter]
	                );
	      return( -1 );
            }
   }

/*  Real work starts here.  */
/* This is the 2nd time the same 'for' loop to be performed, because the first one 
   only to making sure that the tune command is entered correctly before proceeding */

   for (iter = 1; iter < argc; iter += 2)
   {
      channel = tune_channame_to_number( argv[ iter ] );

      if (isReal( argv[ iter+1 ] ))
         do_tune_with_freq( channel, argv[ iter+1 ] );
      else
         do_tune_with_nucleus( channel, argv[ iter+1 ] );
   }
   return( 0 );
}


/**********************************************************************************
   There are two formats to enter the tune command:
   1st format ==>  tune(H1,C13,200)
                               tune switch #1 will output H1 frequency
                                           #2     "       C13   "               
                                           #3     "       200 Mhz
   2nd format ==> tune('ch1','H1','ch2',200,'ch3','N15')
                               tune switch #1 will output H1 frequency
                                           #2     "       200 Mhz
                                           23     "       N15 frequency
   Total of 7 switch positions are available for tunning ( 1 - 7 ), so  the
   user can enter from ch1 -ch7 at once, if One like to do so .
   Tune switch position "0" will place system in normal operation (observe mode)
                        "8" is hard-wired to "0" (no software options) 
                        "9" is hard-wired to "1" (no software options)
***********************************************************************************/
int tune(int argc, char *argv[], int retc, char *retv[])
{
   int	ival;

   if ( nvAcquisition() )
   {
      Werrprintf(
        "This system is not applicable for using the tune command.");
      return( -1 );
   }
   if (argc < 2)
   {
      Werrprintf( "%s:  command requires argument(s).", argv[ CALLNAME ] );
         ABORT;
   }
   if (init_statics() != 0)   /* get rftype, rfband, h1freq ... */
      return( -1 );

   if (whatrftype( 1 ) != DIRECT_NON_DBL)   /* interested only in channel #1 */
   {
      Werrprintf(
        "This system is not applicable for using the tune command . UNITY+ and INOVA only"
                );
      return( -1 );
   }

   ival = tune_channame_to_number( argv[ 1 ] );
   if (ival > -1)
   {
      ival = tune_2nd_format( argc, argv, retc, retv );
      if (ival != 0)
         ABORT;
      else
         RETURN;
   }
   else
   {
      ival = tune_1st_format( argc, argv, retc, retv );
      if (ival != 0)
         ABORT;
      else
         RETURN;
   }
}

/*----------------------------------------------------------------------*/
/* putgtab - dummy routine for now.					*/
/*----------------------------------------------------------------------*/
void putgtab(int table, short word)
{
   (void) table;
   (void) word;
}
/*----------------------------------------------------------------------*/
/* select_sisunity_rfband - dummy routine for now.			*/
/*----------------------------------------------------------------------*/
void select_sisunity_rfband(int band_select, c68int dev_channel)
{
   (void) band_select;
   (void) dev_channel;
}
/*----------------------------------------------------------------------*/
/* set_sisunity_rfband - dummy routine for now.			*/
/*----------------------------------------------------------------------*/
void set_sisunity_rfband(int setwhat, double value, c68int dev_channel)
{
   (void) setwhat;
   (void) value;
   (void) dev_channel;
}
