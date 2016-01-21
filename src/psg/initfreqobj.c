/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include "group.h"
#include "variables.h"
#include "params.h"
#include "acodes.h"
#include "rfconst.h"
#include "oopc.h"	/* Object Oriented Programing Package */
#include "acqparms.h"
#include "chanstruct.h"

#define  COMPLETE	 0
#define  ERROR		-1

extern int			ap_interface;	/* AP Interface Type */
extern char			amptype[];
extern struct _chanconst	rfchanconst[];

/*--------------------------------------------------------------------
| whatfrqstepsize() obtains frequency step size of the channel
|  		    using the stepsize of the freq. offset parameter
|		    (e.g. tof,dof,dof2)
+---------------------------------------------------------------------*/
static double
whatfrqstepsize(chan)
int chan;
{
   char msge[128];
   double stepsize;
   int ret;
   extern char *getoffsetname();
   char *offsetname;

   offsetname = getoffsetname(chan);
   ret = rf_stepsize(&stepsize, offsetname, GLOBAL);
    /*--- determine frequency step size of channel tof, dof, dof2  etc. ---*/
   if ( ret == -1 )
   {   
      sprintf(msge,"Cannot find the variable: '%s'\n", offsetname);
      text_error(msge);
      psg_abort(1);
   }
   if ( ret )
   {
      sprintf(msge,"Cannot find parstep[%d] value\n", ret);
      text_error(msge);
      psg_abort(1);
   }
   return(stepsize);
}
/*--------------------------------------------------------------------
| whatrftype() obtains RF type for the channel from rftype parameter
+---------------------------------------------------------------------*/
whatrftype(chan)
int chan;
{
    char type;
    type = rftype[chan-1];
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
    else if ( (type == 'l') || (type == 'L') )
    {
      return(LOCK_DECOUP_OFFSETSYN);
    }
    else
       return(0);
}
/*--------------------------------------------------------------------
| whatrfband() obtains RF band for the channel from rfband parameter
+---------------------------------------------------------------------*/
static
whatrfband(chan)
int chan;
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
double
whatiffreq(rfchan)
int rfchan;
{
    double iffreq;

    if ( P_getreal(GLOBAL,"iffreq",&iffreq,1) < 0 )
        iffreq = 10.5;               /* if not found assume IF Freq 10.5 MHz */
    return( iffreq );
}

/*--------------------------------------------------------------------
| whatbasefreq() obtains Offset Synthesizer Base frequency based on 
|		 rftype parameter. (differs only between SISCO & VNMRID)
+---------------------------------------------------------------------*/
static
double
whatbasefreq(chan)
int chan;
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
static
double
whatconstfreq(chan,h1freq)
int chan;
int h1freq;
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
       text_error("No fix frequency board for h1freq greater than 400");
       psg_abort(1);
    }
  }
}
/*--------------------------------------------------------------------
| whatoverrange() obtains overrange value of the channel, assumes overrange
|  	is an array having values for each channel, if no value defaults 
|	 to 0.0
+---------------------------------------------------------------------*/
static
double
whatoverrange(rfchan)
int rfchan;
{
    double overundr;
    if ( P_getreal(GLOBAL,"overrange",&overundr,rfchan) < 0 )
    {
       overundr = 0.0;        /* if not found assume 0.0 Hz overrange */
    }
    return( overundr );
}

/*--------------------------------------------------------------------
| whatptsoptions() obtains the 2 options of the PTS (latching,over/under ranging)
|		   also determine type of control required by rftype and
|		   ap_interface parameters. 
+---------------------------------------------------------------------*/
static
double
whatptsoptions(device)
int device;
{
    double overundr;
    char latch[MAXSTR];
    int latchlen;
    int ptsoptmask;

    ptsoptmask = 0;

    if (P_getstring(GLOBAL, "latch", latch, 1, MAXSTR) < 0)
       strcpy(latch,"");
    latchlen = strlen(latch);
    if (P_getreal(GLOBAL, "overrange", &overundr, device) < 0)
       overundr = 0.0;

   if ( (rftype[device-1] == 'c') || (rftype[device-1] == 'C') ||
        (rftype[device-1] == 'd') || (rftype[device-1] == 'D') )
   { /* only direct synthesis RF has PTS */
      if (ap_interface < 2)
      {
	 if ((latchlen >= device) && (overundr > 0.5))
	 {
	    /* SIS system */
            if (latch[device-1] == 'y')	/* latching pts ? */
               ptsoptmask |= (1 << LATCH_PTS);
            if (overundr > 0.5)	/* over/under ranging pts ? */
               ptsoptmask |= (1 << OVR_UNDR_RANGE);
	 }
         else 
	   ptsoptmask = (1 << USE_SETPTS);
      }
      else
      {
         if (latchlen >= device)	  /* no entry for dev, equiv to 'n' */
         {
            if (latch[device-1] == 'y')	/* latching pts ? */
               ptsoptmask |= (1 << LATCH_PTS);
         }
         if (overundr > 0.5)	/* over/under ranging pts ? */
             ptsoptmask |= (1 << OVR_UNDR_RANGE);
      }
   }
   if ( (rftype[device-1] == 'm') || (rftype[device-1] == 'M') )
   {
      if (overundr > 0.5)	/* PTS Contains offsetsyn. */
          ptsoptmask |= (1 << SIS_PTS_OFFSETSYN );
   }
   return( (double) ptsoptmask );	  /* Use SETPTS else do not */
}

setup_freq_obj( FreqObj, rfchan )
Object FreqObj;
int rfchan;
{
        int 	error;

	error =
	SetFreqAttr(FreqObj,SET_DEVCHANNEL,	(double) rfchan,
		         SET_APADR,		(double) rfchanconst[rfchan].ptsapadr,
			 SET_APREG,		(double) rfchanconst[rfchan].ptsapreg,
                         SET_APBYTES,		(double) rfchanconst[rfchan].ptsapbytes,
			 SET_H1FREQ, 		(double) H1freq, 
			 SET_PTSVALUE, 		(double) ptsval[rfchan-1], 
			 SET_OVERRANGE, 	(double) whatoverrange(rfchan), 
			 SET_IFFREQ, 		(double) whatiffreq(rfchan), 
			 SET_FREQSTEP, 		(double) whatfrqstepsize(rfchan), 
			 SET_RFTYPE, 		(double) whatrftype(rfchan), 
			 SET_RFBAND, 		(double) whatrfband(rfchan), 
			 SET_OSYNBFRQ, 		(double) whatbasefreq(rfchan),
			 SET_OSYNCFRQ,		(double) whatconstfreq(rfchan,H1freq),
			 SET_PTSOPTIONS,	(double) whatptsoptions(rfchan),
			 SET_OVRUNDRFLG,	(double) 0.0,
               NULL);

	if (ap_interface == 4)
	{
	  SetFreqAttr(FreqObj, SET_APREG, (double) 0x21, NULL);

          if (whatrftype(rfchan) == LOCK_DECOUP_OFFSETSYN)
          {
	     /*
	     fprintf(stderr,"setup_freq_obj: LOCK_DECOUP_OFFSETSYN, apreg: 0x%x\n",
			0x9c + (rfchan-1)*16);
	    */
	     SetFreqAttr(FreqObj,
		         SET_APADR,		(double) 0xb,
			 SET_APREG,		(double) 0x9c + (rfchan-1)*16,
                         SET_APBYTES,		(double) 3,
               NULL);
          }
	}

	if (error < 0)
	   psg_abort(1);
}


/*------------------------------------------------------------------------
|  special programs for tune.
------------------------------------------------------------------------*/

/*  Special note:  getdblattr and getintattr were STOLEN from specfreq.c
		   SCCS category VNMR.  The PSG versions are identical
		   except I took out all the calls to Werrprintf and 
		   each program returns a status, with the attribute
		   returned using the argument list.			*/

/*--------------------------------------------------------------------
|  getdblattr( object, attribute )
|      get double precision attribute of an object
+--------------------------------------------------------------------*/
static double
getdblattr(obj,attr,raddr)
Object obj;
int    attr;
double *raddr;
{
    Msg_Set_Param   param;
    Msg_Set_Result  result;
    int		    error;

    param.setwhat=attr;
    error = Send(obj,MSG_GET_FREQ_ATTR_pr,&param,&result);
    if (error < 0)
    {
      fprintf(stderr, "%s : %s\n",obj->objname,ObjError(error));
      return ERROR;
    }
    *raddr = result.DBreqvalue;

    return COMPLETE;
}

/*--------------------------------------------------------------------
|  getintattr( object, attribute, result_addr )
|      get integer attribute of an object
|      unlike get double precisiion attribute, this program
|      returns a status.  The requested value is returned
|      via the argument list
+--------------------------------------------------------------------*/
static int
getintattr(obj,attr,raddr)
Object obj;
int    attr;
int   *raddr;
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


/* place any attribute that must be saved/restored when
   computing A-codes for tune as opposed to regular usage.	*/

static struct _attr_for_tune {
	double	old_basefreq;
	double	old_constfreq;
	double	old_iffreq;
	int	old_ptsoptions;
	int	old_ptsval;
	int	old_rfband;
	int	old_rftype;
	int	old_channel;
} attr_for_tune;

int
tune_from_freq_obj( FreqObj, channel )
Object FreqObj;
int channel;
{
	int	eval;

	if (channel != OBSERVE) {
		eval = getintattr( FreqObj, GET_PTSOPTIONS, &attr_for_tune.old_ptsoptions );
		if (eval != COMPLETE)
		  return( -1 );

		eval = getintattr( FreqObj, GET_PTSVALUE, &attr_for_tune.old_ptsval );
		if (eval != COMPLETE)
		  return( -1 );

		eval = getintattr( FreqObj, GET_RFBAND, &attr_for_tune.old_rfband );
		if (eval != COMPLETE)
		  return( -1 );

		eval = getintattr( FreqObj, GET_RFTYPE, &attr_for_tune.old_rftype );
		if (eval != COMPLETE)
		  return( -1 );

		eval = getdblattr( FreqObj, GET_OSYNBFRQ, &attr_for_tune.old_basefreq );
		if (eval != COMPLETE)
		  return( -1 );

		eval = getdblattr( FreqObj, GET_OSYNCFRQ, &attr_for_tune.old_constfreq );
		if (eval != COMPLETE)
		  return( -1 );

		eval = getdblattr( FreqObj, GET_IFFREQ, &attr_for_tune.old_iffreq );
		if (eval != COMPLETE)
		  return( -1 );
    
		attr_for_tune.old_channel = channel;

		eval = SetFreqAttr(FreqObj,
			SET_DEVCHANNEL,		(double) TODEV,
			SET_PTSVALUE,		(double) ptsval[ 0 ], 
			SET_OVERRANGE,		(double) whatoverrange(TODEV),
			SET_IFFREQ,		(double) whatiffreq(TODEV),
			SET_RFTYPE,		(double) whatrftype(TODEV),
			SET_RFBAND,		(double) whatrfband(TODEV),
			SET_OSYNBFRQ,		(double) whatbasefreq(TODEV),
			SET_OSYNCFRQ,		(double) whatconstfreq(TODEV,H1freq),
			SET_PTSOPTIONS,		(double) whatptsoptions(TODEV),
			NULL
		);
		if (eval < 0 )
		  return( -1 );
	}

/*  Look in freq_device.c to see what we do not need
    to pass (back) the channel argument.		*/

	do_tune_acodes( FreqObj );

	if (channel != OBSERVE) {
		eval = SetFreqAttr(FreqObj,
			SET_DEVCHANNEL,		(double) attr_for_tune.old_channel,
			SET_PTSVALUE,		(double) attr_for_tune.old_ptsval,
			SET_IFFREQ,			 attr_for_tune.old_iffreq,
			SET_RFTYPE,		(double) attr_for_tune.old_rftype,
			SET_RFBAND,		(double) attr_for_tune.old_rfband,
			SET_OSYNBFRQ,			 attr_for_tune.old_basefreq,
			SET_OSYNCFRQ,			 attr_for_tune.old_constfreq,
			SET_PTSOPTIONS,		(double) attr_for_tune.old_ptsoptions,
			NULL
		);
		if (eval < 0 )
		  return( -1 );

		fixup_after_tune( FreqObj, channel );
	}
}
