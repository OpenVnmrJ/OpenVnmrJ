/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Expanded to allow interactive modification of frequencies in ACQI.
    Main entry point is G_Offset.  Offset now sets up and calls G_Offset.  */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "group.h"
#include "oopc.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "abort.h"
#include "cps.h"
#include "macros.h"

extern double   getval();
extern double   setoffsetsyn();
extern double   rfchan_getfreqstep();
#ifdef DOIPA
extern void write_to_acqi(char *label, double value, double units,
                   int min, int max, int type, int scale,
                   codeint counter, int device);
extern void insertIPAcode();
extern int	acqiflag;
#endif
extern int      ap_ovrride;	/* ap delay overide flag */
extern int	newacq;
extern char *ObjError(int wcode);

int offsetter(double offset_freq, int device);
int G_Offset( int firstkey, ... );

struct offset_struct {
	double		 freq;
	double		 units;
	char		*label;
	int		 max;
	int		 min;
	int		 scale;
	int		 device;
};

static double
power_of_ten( ival )
int ival;
{
	int	iter;
	double	retval, tempval;

	if (ival == 0)
	  return( 1.0 );
	else if (ival < 0) {
		tempval = 0.1;
		ival = -ival;
	}
	else
	  tempval = 10.0;

	retval = 1.0;
	for (iter = 0; iter < ival; iter++)
	  retval = retval * tempval;

	return( retval );
}

/*  Get frequency offset from device, using the corresponding parameter.  */

static int
get_offset_from_device( device, freq_addr )
int device;
double *freq_addr;
{
	char	*param_name;
	double	 tval;

	if (device == TODEV)
	  param_name = "tof";
	else if (device == DODEV)
	  param_name = "dof";
	else
	  param_name = "dof2";

	if (getparm( param_name, "real", CURRENT, &tval, 1) != 0) {
		fprintf( stdout,
	    "Cannot obtain value for %s in G_Offset\n", param_name
		);
		return( -1 );
	}

	*freq_addr = tval;
	return( 0 );
}

/*  Compute upper limit for a range of twice `scale' that includes `val'  */

static double
upper_limit( val, scale )
double val;
double scale;
{

/*  Routine must distinguish between negative and postive input because
      int( 0.5 ) = 0
      int( -0.5 ) = 0
    and this causes an algorithm that ignored the sign to not work
    correctly for negative numbers.  If you do not believe it, work
    it out yourself !!                                                  */

        int     ival;
        double  retval, signval;

        if (val < 0.0) {
                signval = -1.0;
                ival = (int) ( -val / scale + 0.5 );
        }
        else {
                signval = 1.0;
                ival = (int) ( val / scale + 0.5 );
        }
        if (signval < 0.0)
          retval = ((double) (ival-1)) * scale;  /* Move towards the origin if < 0 */
        else
          retval = ((double) (ival+1)) * scale;  /* Move away from the origin if > 0 */
        return( signval * retval );
}

int
offset( frequency, device )
double frequency;
int device;
{
	int	retval;

	retval = G_Offset( OFFSET_DEVICE,	device,
			   OFFSET_FREQ,		frequency,
			   0 );
	return( retval );
}

int
G_Offset( int firstkey, ... )
{
	struct offset_struct	offset_s;
	double			scale_val;
	va_list			ptr_to_args;
	int			no_freq, no_min_or_max, keyword;
	int			device_type __attribute__((unused));
	int	counter;	/* Offset into A-code sequence where
				   the desired frequency is set.     */

	va_start( ptr_to_args, firstkey );
        keyword = firstkey;

	offset_s.freq   = 0.0;
	offset_s.units  = 1.0;
	offset_s.max    = 1000;
	offset_s.min    = -1000;
	offset_s.scale  = 0;
	offset_s.label  = "";
	offset_s.device = -1;			/* To show none selected  */

	no_freq = 1;				/* No frequency specified */
	no_min_or_max = 1;		/* No minimum or maximum specified */

/*  `ok' is a global variable defined in psg.c  */

	while( (keyword != 0) && (ok != FALSE) ) {
		switch (keyword) {
		  case OFFSET_FREQ:
			offset_s.freq = va_arg( ptr_to_args, double );
			no_freq = 0;
			break;

		  case OFFSET_DEVICE:
			offset_s.device = va_arg( ptr_to_args, int );
			break;

		  case SLIDER_LABEL:
			offset_s.label = va_arg( ptr_to_args, char * );
			break;

		  case SLIDER_SCALE:
			offset_s.scale = va_arg( ptr_to_args, int );
			break;

		  case SLIDER_MAX:
			offset_s.max = va_arg( ptr_to_args, int );
			no_min_or_max = 0;
			break;

		  case SLIDER_MIN:
			offset_s.min = va_arg( ptr_to_args, int );
			no_min_or_max = 0;
			break;

		  case SLIDER_UNITS:
			offset_s.units = va_arg( ptr_to_args, double );
			break;

		  default:
			fprintf( stdout,
		    "Invalid G_Offset keyword %d specified\n", keyword
			);
			ok = FALSE;
			break;
		}
	        keyword = va_arg( ptr_to_args, int );
	}

	va_end( ptr_to_args );
	if (ok == FALSE)
	  return( -1 );
	if (offset_s.device == -1) {			/* No device selected? */
		offset_s.device = TODEV;
	}

	if (offset_s.device == TODEV)
	  device_type = TYPE_TOF;
	else if (offset_s.device == DODEV)
	  device_type = TYPE_DOF;
	else if (offset_s.device == DO2DEV)
	  device_type = TYPE_DOF2;
	else if (offset_s.device == DO3DEV)
	  device_type = TYPE_DOF3;
	else {
		fprintf( stdout,
	    "Invalid RF device %d selected in G_Offset\n", offset_s.device
		);
		return( -1 );
	}

	if (no_freq) {
		if (get_offset_from_device( offset_s.device, &offset_s.freq ) != 0)
		  return( -1 );
	}

	if (no_min_or_max) {
		scale_val = power_of_ten( offset_s.scale ) * 1000.0;
		offset_s.max = upper_limit( offset_s.freq, scale_val );
		offset_s.min = offset_s.max - 2.0 * scale_val;
	}
	else {
		if (offset_s.freq > offset_s.max)
		  fprintf( stdout,
	    "Warning:  offset value larger than maximum for interactive slider\n"
		  );
		else if (offset_s.freq < offset_s.min)
		  fprintf( stdout,
	    "Warning:  offset value smaller than minimum for interactive slider\n"
		  );
	}

	counter = offsetter( offset_s.freq, offset_s.device );
    if (counter < 0)
    {
        return(-1);
    }

#ifdef DOIPA
	if (strcmp( offset_s.label, "" ) != 0 && acqiflag ) {
		write_to_acqi( offset_s.label,
			       offset_s.freq,
			       offset_s.units,
			       offset_s.min,
			       offset_s.max,
			       device_type,
			       offset_s.scale,
			       counter*sizeof (codeint),
			       offset_s.device
		);
	}
#endif
	return( 0 );
}

/*------------------------------------------------------------
|
|	offsetter(freq,device)
|
|	change frequency of either trans or decoupler by
|	freq (-1000.0 to 1000.0 Hz)
|       device is either TODEV or DODEV
|
|				Author Greg Brissey  6/23/86
|   Modified   Author     Purpose
|   --------   ------     -------
|    7/27/89   Greg B.    1. Change routine to use Frequency Objects
|    1/16/90   Greg B.    2. Change routine to use RF Channel Objects
|    12/11/90  Robert L.  3. renamed offsetter; offset now interface
|			     for G_Offset routine
+------------------------------------------------------------*/
int offsetter(double offset_freq, int device)
{
   int             error;
   Msg_Set_Param   param;
   Msg_Set_Result  result;
   extern void	   setHSLdelay();
   int		counter;	/* Offset into A-code sequence where
				   the desired frequency is set.     */

   if ((device < 1) || (device > NUMch))
   {
      abort_message("offsetter: device #%d is not within bounds 1 - %d\n",
	      device, MAX_RFCHAN_NUM);
   }
   if (RF_Channel[device] == NULL)
   {
      if (ix < 2)
      {
	     text_error("offsetter: Warning RF Channel device #%d is not present.\n",
		 device);
      }
      return(-1);
   }

   okinhwloop();

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   setHSLdelay();	/* allows HSL's to be reset if necessary */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   counter = (int) (Codeptr - Aacode);
#ifdef DOIPA
   if (newacq && acqiflag)
	 insertIPAcode(counter);
#endif

   SetRFChanAttr(
	RF_Channel[device],
	SET_OFFSETFREQ,			/* include value of offset frequency */
	offset_freq,
	SET_FREQVALUE,			/* calculate A-codes to set frequency */
	0.0,
	NULL
   );


   /* Get RF band of Observe Transmitter  */
   param.setwhat = GET_XMTRTYPE;
   error = Send(RF_Channel[device], MSG_GET_RFCHAN_ATTR_pr, &param, &result);
   if (error < 0)
   {
      text_error("%s : %s\n", RF_Channel[device]->objname, ObjError(error));
   }

   /* direct syn xmtr type ? */
   if (result.reqvalue != TRUE)
   {
      G_Delay(DELAY_TIME, 1.0e-4, 0);	/* ?? */
   }

   return(counter);
}

/*------------------------------------------------------------
|
|	init_spare_offset(freq,device)
|
|	Init frequency of spare freq register
|
+------------------------------------------------------------*/
void init_spare_offset(double offset_freq, int device)
{
   extern void	   setHSLdelay();

   if ((device < 1) || (device > NUMch))
   {
      abort_message("init_spare_offset: device #%d is not within 1 - %d\n",
	      device, MAX_RFCHAN_NUM);
   }
   if (RF_Channel[device] == NULL)
   {
      if (ix < 2)
      {
	     text_error("init_spare_offset: Warning RF Channel device #%d is not present.\n",
		 device);
      }
      return;
   }

   okinhwloop();

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   setHSLdelay();	/* allows HSL's to be reset if necessary */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   SetRFChanAttr(
	RF_Channel[device],
	SET_OFFSETFREQ_STORE,		/* include value of offset frequency */
	offset_freq,
	SET_FREQVALUE,			/* calculate A-codes to set frequency */
	0.0,
	NULL
   );


}
/*------------------------------------------------------------
|
|	init_spare_freq(freq,device)
|
|	Init frequency of spare freq register
|
+------------------------------------------------------------*/
void init_spare_freq(double freq, int device)
{
   extern void	   setHSLdelay();

   if ((device < 1) || (device > NUMch))
   {
      abort_message("init_spare_freq: device #%d is not within 1 - %d\n",
	      device, MAX_RFCHAN_NUM);
   }
   if (RF_Channel[device] == NULL)
   {
      if (ix < 2)
      {
	     text_error("init_spare_freq: Warning RF Channel device #%d is not present.\n",
		 device);
      }
      return;
   }

   okinhwloop();

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   setHSLdelay();	/* allows HSL's to be reset if necessary */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   SetRFChanAttr(
	RF_Channel[device],
	SET_OVRUNDRFLG,	(double) 0.0,	
	SET_SPECFREQ, freq,
	SET_OFFSETFREQ_STORE,	0.0,	/* include value of offset frequency */
	SET_FREQVALUE,			/* calculate A-codes to set frequency */
	0.0,
	NULL
   );
}

/*------------------------------------------------------------
|
|	set_spare_freq(device)
|
|	Set frequency in spare freq register.
|
+------------------------------------------------------------*/
void set_spare_freq(int device)
{
   extern void	   setHSLdelay();

   if ((device < 1) || (device > NUMch))
   {
      abort_message("init_spare_offset: device #%d is not within 1 - %d\n",
	      device, MAX_RFCHAN_NUM);
   }
   if (RF_Channel[device] == NULL)
   {
      if (ix < 2)
      {
	     text_error("set_spare_offset: Warning RF Channel device #%d is not present.\n",
		 device);
      }
      return;
   }

   okinhwloop();

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   setHSLdelay();	/* allows HSL's to be reset if necessary */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   SetRFChanAttr(
	RF_Channel[device],
	SET_FREQ_FROMSTORAGE,  /* set frequency in spare register */
	1.0,
	SET_FREQVALUE,		/* output A-codes to set frequency */
	0.0,
	NULL
   );
}

/*------------------------------------------------------------
|
|	set_std_freq(device)
|
|	Set frequency in standard freq register.
|	NOTE!!! The user needs to make sure that the last "standard"
|		frequency or offset sent down was to this device.
|
+------------------------------------------------------------*/
void set_std_freq(int device)
{
   extern void	   setHSLdelay();

   if ((device < 1) || (device > NUMch))
   {
      abort_message("init_spare_offset: device #%d is not within 1 - %d\n",
	      device, MAX_RFCHAN_NUM);
   }
   if (RF_Channel[device] == NULL)
   {
      if (ix < 2)
      {
	     text_error("set_spare_offset: Warning RF Channel device #%d is not present.\n",
		 device);
      }
      return;
   }

   okinhwloop();

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   setHSLdelay();	/* allows HSL's to be reset if necessary */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   SetRFChanAttr(
	RF_Channel[device],
	SET_FREQ_FROMSTORAGE,  /* set frequency in spare register */
	0.0,
	SET_FREQVALUE,		/* output A-codes to set frequency */
	0.0,
	NULL
   );
}
