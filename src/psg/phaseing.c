/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "oopc.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "aptable.h"
#include "macros.h"
#include "ssha.h"
#include "abort.h"

extern int	bgflag,			/* debug flag */
		ap_ovrride;		/* ap delay override flag */
extern int      safety_check();         /* safety checking for max atten value */
extern char *ObjError(int wcode);
extern int putcode();


/*-----------------------------------------------------------------
| 
|The psg element POWER may be called from any pulse sequence.
|It requires two parameters to be passed as shown below:
|
|	power(power_level,device)/2
|	
|	power_level:
|		the name of a real time variable (v1-v14).
|		The value of this acquisition time parameter must
|		be positive between 0 - 63.
|	
|	device:
|		specifies if the transmitter or the decoupler power level
|		will be changed. Values may be TODEV or DODEV.
|	
|	For example:   POWER(V1,TODEV)
|	
|	The procedure will output three integer values, an acode (SETPOWER)
|	then the relative pointer to the power_level, and finally the device 
|	(TODEV or DODEV).   In addition, a 0.2 us delay has been inserted
|       prior to actually setting the attenuator over the AP bus.  This is
|	to allow the high-speed lines to be reset before the AP bus call
|	instead of after the AP bus call (which takes 2 us).
|
|	
|				Author Greg Brissey  5/19/86
|                             Modified Sandy Farmer  6/2/88
|                             Modified Greg Brissey  9/15/88
|   Modified   Author     Purpose
|   --------   ------     -------
|   12/13/88   Greg B.    1. Changed param.setwhat to SET_RTPARAM from SET_VALUE
|			     to handle new capabilities of Attn Object. 
|   1/6/89     Greg B.    1. Be warn that this routine is being replaced by
|			     SetAttnAttr().
|   6/25/90     Greg B.    1. Changed to use Channel Objects 
+---------------------------------------------------------------*/
void power(int value, int device)
{
	if ( (device > 0) && (device <= NUMch) )
	{
	   SetRFChanAttr(RF_Channel[device], SET_RTPWR,value, 0);
	}
	else
	{
           abort_message("power: device #%d is not within bounds 1 - %d\n",
			device,NUMch);
	}
}

/*-----------------------------------------------------------------
|
|	rlpower()/2  -  rlpower(power_level, rfdevice)
|
|			power_level is a double precision variable
|			rfdevice is TODEV, DODEV, or DO2DEV
|
+----------------------------------------------------------------*/
void rlpower(double value, int device)
{
   int	attnval;

   if ( (device > 0) && (device <= NUMch) )
   {
      attnval = (int) ( (value < 0.0) ? (value - 0.5) : (value + 0.5) );

      if (safety_check("MAXATTEN", "psg: rlpower()", (double)attnval, device, "not used") == 0 )
      {
        psg_abort(1);
      }

      SetRFChanAttr(RF_Channel[device], SET_PWR, attnval, 0);
      if (isSSHAPselected())
	hdwshiminitPresat();
   }
   else
   {
      abort_message("rlpower: device #%d is not within bounds 1 - %d\n",
		 device, NUMch);
   }
}

/*-----------------------------------------------------------------
| The functions:
|   txphase(phaseptr)  {};
|   xmtrphase(phaseptr) {};
|   decphase(phaseptr) {};
|   dcplrphase(phaseptr) {};
|   
| have been replace with the approriate calls to the Channel Object
|
|  	dcphase()/1 is no longer supported.
|
+-----------------------------------------------------------------*/

/*-----------------------------------------------------------------
| 
|The psg element PWRF may be called from any pulse sequence.
|It requires two parameters to be passed as shown below:
|
|	pwrf(power_level,device)/2
|	
|	power_level:
|		the name of a real time variable (v1-v14).
|		The value of this acquisition time parameter must
|		be positive between 0 - 63.
|	
|	device:
|		specifies if the transmitter or the decoupler power level
|		will be changed. Values may be TODEV or DODEV or DO2DEV.
|	
|	For example:   PWRF(V1,TODEV)
|	
|	Was a macro but now a function call.
|
|	
|				Author Greg Brissey  6/19/90
+----------------------------------------------------------------*/
void pwrf(int value, int device)
{
	if ( (device > 0) && (device <= NUMch) )
	{
	   SetRFChanAttr(RF_Channel[device], SET_RTPWRF, value, 0);
	}
	else
	{
           abort_message("pwrf: device #%d is not within bounds 1 - %d\n",
			device,NUMch);
	}
}

/*-----------------------------------------------------------------
|
|	 rlpwrf()/2  -  rlpwrf(power_level, rfdevice)
|
|			power_level is a double precision variable
|			rfdevice is TODEV, DODEV, or DO2DEV
|
+----------------------------------------------------------------*/
void rlpwrf(double value, int device)
{
   int	attnval;

   if ( (device > 0) && (device <= NUMch) )
   {
      attnval = (int) ( (value < 0.0) ? (value - 0.5) : (value + 0.5) );
      SetRFChanAttr(RF_Channel[device], SET_PWRF, attnval, 0);
   }
   else
   {   
      abort_message("rlpwrf: device #%d is not within bounds 1 - %d\n",
                        device,NUMch);
   }
}

/*--------------------------------------------------------------------
|
|  W A R N I N G:  Only valid for C type RF
|
|   The psg element STEPSIZE may be called from any pulse sequence.
|   It requires two parameters to be passed as shown below:
|
|   stepsize(phase_incremental_value,device)/2
|
|   phase_incremental_value:
|	a real number, expression or variable
|	It represents the base phase shift in degrees.
|	Any Value is acceptable.
|
|   device:
|	specifies if the trransmitter or the decoupler will
|	be phase shifted. Values may be TODEV or DODEV.
|
|   For example:   STEPSIZE(60.0,TODEV);
|
|   The base is multiplied by 2 so that angles down to 0.5 can be specified.
|   These numbers are interpreted by APINT to set the correct phase.
|
|				Author Greg Brissey  5/19/86
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/25/90     Greg B.    1. Changed to use Channel Objects 
|  double stepsiz;	 stepsize of phaseshifter .5-360
|  int device;		 device trans or decoupler
+--------------------------------------------------------------*/
void stepsize(double stepsiz, int device)
{
	if ( (device > 0) && (device <= NUMch) )
	{
	   SetRFChanAttr(RF_Channel[device], SET_PHASESTEP,stepsiz, 0);
	}
	else
	{
           abort_message("stepsize: device #%d is not within bounds 1 - %d\n",
			device,NUMch);
	}
}

/*-------------------------------------------------------------------
|
|	apovrride()
|
|	The PSG element apovrride sets the global flag ap_ovrride to
|	TRUE so that the next funciton which uses the AP bus does not
|	incur the 0.2 us delay.
|
|					Author  Sandy Farmer  6/27/88
|
-------------------------------------------------------------------*/
void apovrride()
{
   ap_ovrride = 1;	/* ap delay override flag is set to TRUE */
}
/*-------------------------------------------------------------------
|
|	phaseshift()
|
|	The PSG element phaseshift may be called from any pulse sequence.
|       It requires three elements to be passed.  Each element is described
|	below.  The function generates the necessary Acodes either to produce
|	software small-angle phaseshift.
|
|
|	phaseshift(basephase,multiplier,device)/3
|
|
|	basephase:
|		the step size in degrees for the small-angle phaseshifts.
|		The step size is generally chosen to be the largest common
|		denominator for all desired small-angle phaseshifts.
|		The variable "basephase" is a real number.
|	
|	multiplier:
|		the name of a real time variable (ct, v1-v14, etc).
|		The value of this acquisition time parameter must
|		be positive. The actual phase shift will be:
|
|		( phase_incremental_value * multiplier ) mod 360.
|
|	device:
|		the RF device which is to be phaseshifted.  The only
|		two supported RF devices are the transmitter (TODEV)
|		and the decoupler (DODEV).  The variable "device"
|		is an integer.
|	
|	
|				Author Greg Brissey  5/19/86
|				Modified Sandy Farmer  6/24/88
|
+------------------------------------------------------------------*/
void phaseshift(basephase,mult,device)
double basephase;
codeint mult;
int device;
{
   codeint iminshift;    /*  100 * minimum shift */
   double deltafreq;	/*  delta freq use to shift phase */
   double Hzoffset;	/* freq of the offset word */
   double min_delay;	/* minimum delay possible */
   double min_shift;	/* minimum phase shift possible */
   double rphase;
   int incr;		/* change in the offset word multiplier */
   int digit;		/* 10kHz digit of the offset word */
   int digitval;
   int apword;
   int i,ok_device;
   Msg_Set_Param param;
   Msg_Set_Result result;
   int error;

   incr = 4;		/* change freq by four of base freq */
   digit = 5;		/* base freq, 10kHz */
   
/* Check device arguments and RF consistency. */
   ok_device = (device == TODEV) || (device == DODEV);
   if (!ok_device)
   {
       text_error("Incorrect device for PHASESHIFT\n");
       text_error("Use TODEV or DODEV\n");
       psg_abort(1);
   }
   if ((device == TODEV) && (newtrans))
   {
        text_error("PHASESHIFT:  requires old-style RF on xmitter\n");
	text_error("Use XMTRPHASE for small-angle phaseshifts with direct syn RF on xmitter\n");
	psg_abort(1);
   }
   if ((device == DODEV) && (newdec))
   {
	text_error("PHASESHIFT: requires old-style RF on decoupler\n");
	text_error("Use DCPLRPHASE for small-angle phaseshifts with direct syn RF on decoupler\n");
	psg_abort(1);
   }

/* Calculate parameters for software small-angle phaseshift. */
   if (fifolpsize < 70)
       min_delay = 100.0e-9; /* 100 nsec */
   else
       min_delay = 25.0e-9; /* new ouput board 25 nsec */

   for (Hzoffset = 1.0,i=1; i<digit; i++)
     Hzoffset *= 10.0;

   deltafreq = Hzoffset * incr;         /* e.g., digit=5 incr=4, dfreq=40k*/

   min_shift = 360.0 * min_delay * deltafreq; /* 360*40k*100nsec= 1.44degrees*/
   iminshift = (codeint) ((min_shift * 100.0) + 0.05); /* 1.44 -> 144 */

   
    /* Get tbo,dbo of channel */
    param.setwhat=GET_OF_FREQ;
    error = Send(RF_Channel[device],MSG_GET_RFCHAN_ATTR_pr,&param,&result);
    if (error < 0)
    {
      text_error("%s : %s\n",RF_Channel[device]->objname,ObjError(error));
    }

   digitval = ((int)( result.DBreqvalue / Hzoffset )) % 10;
   apword = 0x8000 | (device << DEVPOS) | (digit << DIGITPOS) | digitval;

   if ( (digitval + incr) >= 10 )
      incr = -incr;

   rphase = basephase;

   /* Force phase shift between 0 to 360 */
   while (rphase < 0.0) 
     rphase += 360.0;
   while (rphase > 360.0) 
     rphase -= 360.0;

   if ((mult >= t1) && (mult <= t60))
   {
      mult = tablertv(mult);
   }

   putcode((codeint) PHASESHIFT);
   putcode(iminshift);
   putcode((codeint) (rphase + 0.5));
   putcode(mult);
   putcode((codeint) incr);
   putcode((codeint) apword);
}
