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
#include "oopc.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"

#ifdef DEBUG
extern int      bgflag;

#define DPRINT(level, str) \
        if (bgflag >= level) fprintf(stdout,str)
#define DPRINT1(level, str, arg1) \
        if (bgflag >= level) fprintf(stdout,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2)
#define DPRINT3(level, str, arg1, arg2, arg3) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2,arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2,arg3,arg4)
#else
#define DPRINT(level, str)
#define DPRINT1(level, str, arg2)
#define DPRINT2(level, str, arg1, arg2)
#define DPRINT3(level, str, arg1, arg2, arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4)
#endif

extern char *ObjError(),*ObjCmd(); 

extern int NUMch;	/* number of defined channels */

#define DPRTLEVEL 1

/*---------------------------------------------------------------------
| sync_on_event  -  Set the appropriate event op for proper external
|		    syncing
|                       Author: Greg Brissey  11/14/88
+---------------------------------------------------------------------*/
/*VARARGS2*/
sync_on_event(Object dev_obj, ...)
{
   char		   msge[128];
   va_list         vargs;
   int             error = 0;
   Msg_Set_Param   param;
   Msg_Set_Result  result;

   va_start(vargs, dev_obj);
   if (dev_obj->dispatch == NULL)
   {
      abort_message("sync_on_event: Uninitialized Device...\n");
   }

   /* Options allways follow the format 'Option,(value),Option,(value),etc' */
   while ((param.setwhat = va_arg(vargs, int)) != 0)
   {
      switch (param.setwhat)
      {
	 case SET_RTPARAM:
	    param.value = (c68int) va_arg(vargs, int);
	    DPRINT3(DPRTLEVEL,"sync_on_event: Cmd: '%s' to %d, for device: '%s'\n",
		      ObjCmd(param.setwhat),param.value,dev_obj->objname);
	    error = Send(dev_obj, MSG_SET_EVENT_VAL_pr, &param, &result);
	    break;
	 case SET_DBVALUE:
   	    param.setwhat = SET_VALUE;
	    param.value = (c68int) va_arg(vargs, double);
	    DPRINT3(DPRTLEVEL,"sync_on_event: Cmd: '%s' to %d, for device: '%s'\n",
		      ObjCmd(param.setwhat),param.value,dev_obj->objname);
	    error = Send(dev_obj, MSG_SET_EVENT_VAL_pr, &param, &result);
	    break;
	 default:
            sprintf(msge,"%s : Unknown Command '%s'.\n",
		dev_obj->objname,ObjCmd(param.setwhat));
            abort_message(msge);
	    break;
      }
   }
   va_end(vargs);
   return (error);
}
/*---------------------------------------------------------------------
| hardware_get  -  Set the appropriate event op for proper external
|		    syncing
|                       Author: Greg Brissey  11/14/88
+---------------------------------------------------------------------*/
hardware_get(Object dev_obj, ...)
{
   char		   msge[128];
   va_list         vargs;
   int             error = 0;
   Msg_Set_Param   param;
   Msg_Set_Result  result;

   va_start(vargs, dev_obj);
   if (dev_obj->dispatch == NULL)
   {
      abort_message("hardware_get: Uninitialized Device...\n");
   }

   /* Options allways follow the format 'Option,(value),Option,(value),etc' */
   while ((param.setwhat = va_arg(vargs, int)) != 0)
   {
      switch (param.setwhat)
      {
	 case GET_RTVALUE:
	    param.value = (c68int) va_arg(vargs, int);
	    DPRINT3(DPRTLEVEL,"hardware_get: Cmd: '%s' to %d, for device: '%s'\n",
		      ObjCmd(param.setwhat),param.value,dev_obj->objname);
	    error = Send(dev_obj, MSG_GET_EVENT_VAL_pr, &param, &result);
	    break;
	 default:
            sprintf(msge,"%s : Unknown Command '%s'.\n",
		dev_obj->objname,ObjCmd(param.setwhat));
            abort_message(msge);
	    break;
      }
   }
   va_end(vargs);
   return (error);
}


/*-------------------------------------------------------------------
|
|	verify_rfchan()/1
|	 verify the RF channel is valid for this system
|	 verifies device is in range 1 <= Numch
|	 verifies the RF_Channel exists for device
|	 facility is used to identify who called this program
|
+------------------------------------------------------------------*/
static int
verify_rfchan( device, facility )
int device;
{
	char	msge[ 128 ];

	if ((device < 1) || (device > NUMch))
	{
		sprintf(msge,"%s: device #%d is not within bounds 1 - %d\n",
			      facility,device,NUMch);
		abort_message(msge);
	}
	if ( RF_Channel[device] == NULL )
	{
		sprintf(msge,"%s: Warning RF Channel device #%d is not present.\n",
			      facility,device);
		text_error(msge);
		return(-1);
	}

	return( 0 );
}

/*-------------------------------------------------------------------
|
|	rfchan_getrfband()/1 
|        get the rfband the channel is in. 
|	 
|				Author Greg Brissey  1/16/90
|   Modified   Author     Purpose
|   --------   ------     -------
+------------------------------------------------------------------*/
rfchan_getrfband(device)
int device;
{
    int error;
    char msge[128];
    Msg_Set_Param param;
    Msg_Set_Result result;

    error = verify_rfchan( device, "chan_getrfband" );
    if (error != 0)
      return( error );

    /* Get RF band of Observe Transmitter  */

    param.setwhat=GET_RFBAND;
    error = Send(RF_Channel[device],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
    if (error < 0)
    {
      sprintf(msge,"%s : %s\n",RF_Channel[device]->objname,ObjError(error));
      text_error(msge);
    }
    return(result.reqvalue);
}
/*-------------------------------------------------------------------
|
|	rfchan_getlbandmax()/1 
|        get the low band frequency max  
|	 
|				Author Greg Brissey  8/5/91
|   Modified   Author     Purpose
|   --------   ------     -------
+------------------------------------------------------------------*/
double
rfchan_getlbandmax(device)
int device;
{
    int error;
    char msge[128];
    Msg_Set_Param param;
    Msg_Set_Result result;

    error = verify_rfchan( device, "chan_getlbandmax" );
    if (error != 0)
      return( error );

    /* Get RF band of Observe Transmitter  */

    param.setwhat=GET_LBANDMAX;
    error = Send(RF_Channel[device],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
    if (error < 0)
    {
      sprintf(msge,"%s : %s\n",RF_Channel[device]->objname,ObjError(error));
      text_error(msge);
    }
    return(result.DBreqvalue);
}
/*-------------------------------------------------------------------
|
|	rfchan_getfreqmax()/1 
|        get the frequency max of channel 
|	 
|				Author Greg Brissey  8/5/91
|   Modified   Author     Purpose
|   --------   ------     -------
+------------------------------------------------------------------*/
double
rfchan_getfreqmax(device)
int device;
{
    int error;
    char msge[128];
    Msg_Set_Param param;
    Msg_Set_Result result;

    error = verify_rfchan( device, "chan_getfreqmax" );
    if (error != 0)
      return( error );

    /* Get RF band of Observe Transmitter  */

    param.setwhat=GET_MAXXMTRFREQ;
    error = Send(RF_Channel[device],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
    if (error < 0)
    {
      sprintf(msge,"%s : %s\n",RF_Channel[device]->objname,ObjError(error));
      text_error(msge);
    }
    return(result.DBreqvalue);
}
/*-------------------------------------------------------------------
|
|	rfchan_getampband()/1 
|        get the amplifier band the channel is in. 
|	 
|				Author Greg Brissey  1/25/90
|   Modified   Author     Purpose
|   --------   ------     -------
+------------------------------------------------------------------*/
rfchan_getampband(device)
int device;
{
    int error;
    char msge[128];
    Msg_Set_Param param;
    Msg_Set_Result result;

    error = verify_rfchan( device, "chan_getampband" );
    if (error != 0)
      return( error );

    /* Get RF band of Observe Transmitter  */

    param.setwhat=GET_AMPHIBAND;
    error = Send(RF_Channel[device],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
    if (error < 0)
    {
      sprintf(msge,"%s : %s\n",RF_Channel[device]->objname,ObjError(error));
      text_error(msge);
    }
    return(result.reqvalue);
}
/*-------------------------------------------------------------------
|
|	rfchan_getbasefreq()/1 
|        get the base frequency for the channel. 
|	 
|				Author Greg Brissey  1/25/90
|   Modified   Author     Purpose
|   --------   ------     -------
+------------------------------------------------------------------*/
double
rfchan_getbasefreq(device)
int device;
{
    int error;
    char msge[128];
    Msg_Set_Param param;
    Msg_Set_Result result;

    error = verify_rfchan( device, "chan_getbasefreq" );
    if (error != 0)
      return( error );

    /* Get RF band of Observe Transmitter  */

    param.setwhat=GET_BASE_FREQ;
    error = Send(RF_Channel[device],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
    if (error < 0)
    {
      sprintf(msge,"%s : %s\n",RF_Channel[device]->objname,ObjError(error));
      text_error(msge);
    }
    return(result.DBreqvalue);
}
/*-------------------------------------------------------------------
|
|       rfchan_getfreqstep()/1
|        get the frequency step size for this channel.
|
+------------------------------------------------------------------*/
double
rfchan_getfreqstep(device)
int device;
{
    int error;
    char msge[128];
    Msg_Set_Param param;
    Msg_Set_Result result;

    error = verify_rfchan( device, "chan_getfreqstep" );
    if (error != 0)
      return( error );

/* Get frequency step size in requested channel  */

    param.setwhat=GET_FREQSTEP;
    error = Send(RF_Channel[device],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
    if (error < 0)
    {
      sprintf(msge,"%s : %s\n",RF_Channel[device]->objname,ObjError(error));
      text_error(msge);
    }
    return(result.DBreqvalue);
}

/*-------------------------------------------------------------------
|
|       rfchan_getHSlines()/1
|        get current state of HS lines for this channel.
|
|	 returns ON (1) or OFF (0)
|
+------------------------------------------------------------------*/
int
rfchan_getxmtrstate(device)
int device;
{
	int		HSlines, HSmask;
	int		error;
	char		msge[128];
	Msg_Set_Param	param;
	Msg_Set_Result	result;

	error = verify_rfchan( device, "chan_getfreqstep" );
	if (error != 0)
	  return( error );

/*  Get current state of HS lines in requested channel  */

	param.setwhat=GET_XMTRGATE;
	error = Send(RF_Channel[device],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
	if (error < 0)
	{
		sprintf(msge,"%s : %s\n",RF_Channel[device]->objname,ObjError(error));
		text_error(msge);
	}
	HSlines = result.reqvalue;

/*  Get mask for the transmitter for this channel */

	param.setwhat=GET_XMTRGATEBIT;
	error = Send(RF_Channel[device],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
	if (error < 0)
	{
		sprintf(msge,"%s : %s\n",RF_Channel[device]->objname,ObjError(error));
		text_error(msge);
	}
	HSmask = result.reqvalue;

	return( (HSmask & HSlines) != 0 );
}

/*-----------------------------------------------------------------
|      rfchan_getpower()/1
|
|	return current power level for this channel
|
+----------------------------------------------------------------*/
double
rfchan_getpower( channel )
int	channel;
{
	char		msge[ 128 ];
	int		error;
	Msg_Set_Param	param;
	Msg_Set_Result	result;

	error = verify_rfchan( channel, "chan_getpower" );
	if (error != 0)
	  return( error );

	param.setwhat = GET_PWR;
	error = Send(
		 RF_Channel[ channel ],
		 MSG_GET_RFCHAN_ATTR_pr,
		&param,
		&result
	);
	if (error < 0) {
		sprintf( msge,
			"%s : %s\n", RF_Channel[channel]->objname, ObjError(error)
		);
		text_error(msge);
	}

	return( (double) (result.reqvalue) );
}

/*-----------------------------------------------------------------
|      rfchan_getpwrf()/1
|
|	return current fine power level for this channel
|
+----------------------------------------------------------------*/
double
rfchan_getpwrf( channel )
int	channel;
{
	char		msge[ 128 ];
	int		error;
	Msg_Set_Param	param;
	Msg_Set_Result	result;

	error = verify_rfchan( channel, "chan_getpower" );
	if (error != 0)
	  return( (double) 4095.0 );

	param.setwhat = GET_PWRF;
	error = Send(
		 RF_Channel[ channel ],
		 MSG_GET_RFCHAN_ATTR_pr,
		&param,
		&result);
	return( (error < 0) ? (double) 4095.0 : (double) (result.reqvalue) );
}
