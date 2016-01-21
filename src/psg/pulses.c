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
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include "group.h"
#include "cps.h"
#include "acodes.h"
#include "oopc.h"
#include "rfconst.h"
#include "acqparms.h"
#include "macros.h"
#include "apdelay.h"
#include "pvars.h"
#include "decfuncs.h"
#include "delay.h"
#include "ssha.h"
#include "abort.h"

#define MINDELAY  0.195e-6
#define INOVAMINDELAY 0.0995e-6

extern int      acqiflag;	/* for interactive acq. or not? */
extern int      bgflag;		/* debug flag */
extern int      rcvroff_flag;	/* global receiver flag */
extern int	rcvr_hs_bit;
extern int      ap_interface;
extern int	rfp270_hs_bit;
extern int	dc270_hs_bit;
extern int	dc2_270_hs_bit;
extern int	newacq;
extern int      dps_flag;

extern codeint	t60;
extern int okinhwloop();
extern int SetRFChanAttr(Object obj, ...);
extern void HSgate(int ch, int state);
extern int putcode();
extern void notinhwloop(char *name);
extern void rgradient(char axis, double value);
extern int isBlankOn(int device);

void write_to_acqi(char *label, double value, double units,
                   int min, int max, int type, int scale, codeint counter, int device);

FILE	*sliderfile = 0;

struct pulsestruct
{
   double          pw;
   double          rof1;
   double          rof2;
   double          units;
   int             scale;
   int             max;
   int             min;
   int             device;
   int             phasetype;
   union {
           int     ival;
           char   *cval;
         } phval;
   char           *label;
};

static int pulser(struct pulsestruct *pulses, int do_0_pulse);

int pulse_phase_type(val)
int val;
{
   return((0 <= val && val <= t60) ? PULSE_PHASE : PULSE_PHASE_TABLE);
}
int phase_var_type(val)
int val;
{
   return((0 <= val && val <= t60) ? SET_RTPHASE90 : SET_PHASE90);
}
/*-------------------------------------------------------------------
|
|	G_Pulse(variable arg_list)/n
|
|	Generate a pulse with phase with receiver being gated off
|       rx2 sec. before pulseand receiver turned on rx2 sec. after
|       the  pulse.
|
|				Author Frits Vosman  1/3/89
+------------------------------------------------------------------*/
/*VARARGS1*/
void 
G_Pulse(int firstkey, ...)
{
   struct pulsestruct pulses;
   va_list         ptr_to_args;
   int             counter;
   int             keyword;

/* fill in the defaults */
   pulses.pw     = pw;
   pulses.rof1   = rof1;
   pulses.rof2   = rof2;
   pulses.device = TODEV;
   pulses.label  = "";
   pulses.scale  = 1;
   pulses.max    = 100;
   pulses.min    = 0;
   pulses.units  = 1e-6;
   pulses.phasetype  = SET_RTPHASE90;
   pulses.phval.ival  = oph;

/* initialize for variable arguments */
   va_start(ptr_to_args, firstkey);
   keyword = firstkey;
/* now check if the user want it his own way */
   while (keyword && ok)
   {
      switch (keyword)
      {
	 case PULSE_WIDTH:
	    pulses.pw =  va_arg(ptr_to_args, double);
	    if (pulses.pw < 0.0)
	       pulses.pw = 0.0;
	    break;
	 case PULSE_PRE_ROFF:
	    pulses.rof1 = va_arg(ptr_to_args, double);
	    break;
	 case PULSE_POST_ROFF:
	    pulses.rof2 = va_arg(ptr_to_args, double);
	    break;
	 case PULSE_DEVICE:
	    pulses.device = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_LABEL:
	    pulses.label = va_arg(ptr_to_args, char *);
	    break;
	 case SLIDER_SCALE:
	    pulses.scale = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_MAX:
	    pulses.max = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_MIN:
	    pulses.min = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_UNITS:
	    pulses.units = va_arg(ptr_to_args, double);
	    break;
	 case PULSE_PHASE:
            pulses.phasetype  = SET_RTPHASE90;
	    pulses.phval.ival = va_arg(ptr_to_args, int);
	    break;
	 case PULSE_PHASE_TABLE:
            pulses.phasetype  = SET_PHASE90;
	    pulses.phval.cval = va_arg(ptr_to_args, char *);
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
   if (isSSHAPselected())
   {
      if (isSSHAstillDoItNow())
      {
	 turnOnSSHA(pulses.pw);
	 if (pulses.pw >= 0.1)
	 {
	    turnOnSSHA(pulses.pw);
	    setSSHAdelayNotTooShort();
	 }
	 else
	 {
	    text_error( "WARNING: pulse of %g is too short for hdwshim\n", pulses.pw );
	    setSSHAoff();
	 }
      }
   }
/* using the old pulse subroutines already present in PSG */
   counter = pulser (&pulses, (strcmp(pulses.label,"") && acqiflag) );
/* do we want interactive control? */
   if ( strcmp(pulses.label, "")  && acqiflag )
   {
      write_to_acqi(pulses.label, pulses.pw, pulses.units, pulses.min,
		    pulses.max, TYPE_PULSE, pulses.scale, counter*2,
		    pulses.device );
   }
   if (isSSHAPselected() && isSSHAactive())
   {
      turnOffSSHA();
   }
}


/*-------------------------------------------------------------------
|
|	simpulse(transpw,decpw,transphase,decupphase,rx1,rx2)/6
|
|	generate a simultaneous transmitter and decoupler pulse
|	The shorter of the two pulse widths is centered in the other.
|	with receiver being gated of rx1 sec before pulse
|	and receiver turned on rx2 sec after pulse.
|				Author Greg Brissey  6/20/86
| replace with a macro
+------------------------------------------------------------------*/
/*-------------------------------------------------------------------
|
|	pulser(pulsewidth,phaseptr,rx1,rx2)/4
|
|	Generate a pulse of width 'pulsewidth', phase of phaseptr
|	with receiver being gated off rx1 sec before pulse
|	and receiver turned on rx2 sec after pulse on device device.
|	Combine rgpulse/decrgpulse for pulser.
|
|				Author Sandy Farmer  6/26/88
|				Combiner Frits Vosman 1/3/89
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/15/89   Greg B.     1. Use new global parameters to calc acode offsets 
|   1/18/90   Greg B.     1. Use new RF Channel Objects 
+------------------------------------------------------------------*/
static int pulser(struct pulsestruct *pulses, int do_0_pulse)
{
double          pulsewidth = pulses->pw,
                rx1 = pulses->rof1,
                rx2 = pulses->rof2;
double 		mindelay;
int             device = pulses->device;
   int		   count;
   char           msge[128];
   extern int      rfchan_getphsbits();
   extern char*    rfchan_getdevname();
   count = 0;

    if (newacq)
	mindelay = INOVAMINDELAY;
    else
	mindelay = MINDELAY;

    if ((device < 1) || (device > NUMch))
    {
      sprintf(msge,"pulser: device #%d is not within bounds 1 - %d\n",
        device,NUMch);
      text_error(msge);
      psg_abort(1);
    }

    if ( RF_Channel[device] == NULL )
    {
      sprintf(msge,"pulser: Warning RF Channel device #%d is not present.\n",
        device);
      text_error(msge);
      psg_abort(1);
    }

   if (pulsewidth < 0.0)
   {
      if (ix == 1)
      {
         sprintf(msge, "Warning for %s:  improper pulse value set to zero.\n",
		  RF_Channel[device]->objname);
         text_error(msge);
         text_error("A negative pulse width cannot be executed.\n");
      }
   }
   else if ((pulsewidth > 0.0) && (pulsewidth < mindelay))
   {
      if (ix == 1)
      {
         sprintf(msge, "Warning for %s:  improper pulse value set to zero.\n",
		   RF_Channel[device]->objname);
         text_error(msge);
         text_error(
	      "A non-zero pulse width less than 0.2 us cannot be executed!\n");
      }
   }
   else if (pulsewidth >= mindelay || do_0_pulse)
   {
      /* phsbits = rfchan_getphsbits(device); */
      okinhwloop();

      /* --- set phase bits in rfphpt or curdec --- */
      if (pulses->phasetype == SET_RTPHASE90)
         SetRFChanAttr(RF_Channel[device], SET_RTPHASE90,pulses->phval.ival, 0);
      else
         SetRFChanAttr(RF_Channel[device], SET_PHASE90, pulses->phval.cval, 0);

      if (ap_interface < 4)
      {
	 if (device == DODEV)
	    sisdecblank(OFF);
         HSgate(rcvr_hs_bit, TRUE);	/* turn receiver off */
      }
      else
      {  
	 if (newacq)
	    HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
	 else
	    SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, OFF, 0);
         SetRFChanAttr(RF_Channel[device], SET_RCVRGATE, OFF, 0);
      }

      delayer(rx1,FALSE);		/* rx1  (e.g., rof1)  */
      SetRFChanAttr(RF_Channel[device], SET_XMTRGATE, ON, 0);
      count = (int) (Codeptr - Aacode);	/* offset into Acodes */
      if (newacq && acqiflag)
	 insertIPAcode(count);
      delayer(pulsewidth,do_0_pulse);	/* pulsewidth  (e.g., pw)  */
      SetRFChanAttr(RF_Channel[device], SET_XMTRGATE, OFF, 0);
      delayer(rx2,FALSE);		/* wait rx2 (e.g., rof2) */
      if (ap_interface < 4)
      {  if (!rcvroff_flag)		/* turn receiver back on only if */
	   HSgate(rcvr_hs_bit, FALSE);	/* receiver is not globally turned */
					/* off */
	 if (device == DODEV)
	    sisdecblank(ON);
      }		
      else
      {
	 if (newacq) {
	    if (!rcvroff_flag)		/* turn receiver back on only if */
	   	HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
	 }
	 else {
            if ( isBlankOn(OBSch) )
            	SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, ON, 0);
	 }
         if ( isBlankOn(device) )
            SetRFChanAttr(RF_Channel[device], SET_RCVRGATE, ON, 0);
      }
   }
   return(count);
}

/*-------------------------------------------------------------------
|
|       gensim2pulse(pw1, pw2, phase1, phase2, rx1, rx2, device1,
|                       device2)/8
|
|       It generates a simultaneous pulse on two RF channels.
|       The two pulse widths are centered with respect to one
|       another.  The receiver is gated of rx1 sec before the
|       pulse and turned back on rx2 sec after the pulse.
|	Either or both pulse times can have a value of 0.0
|
|                               Author Greg Brissey  6/20/86
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   1/18/90   Greg B.     1. Use new RF Channel Objects 
+------------------------------------------------------------------*/
void gensim2pulse(pw1, pw2, phase1, phase2, rx1, rx2, device1, device2)
codeint	phase1,		/* phase for first RF channel			*/
	phase2;		/* phase for second RFchannel			*/
int	device1,	/* name of first RF channel, e.g., TODEV	*/
	device2;	/* name of second RF channel			*/
double	pw1,		/* pulse length for first RF channel		*/
	pw2,		/* pulse length for second RF channel		*/
	rx1,		/* pre-pulse delay				*/
	rx2;		/* post-pulse delay				*/
{
   char		msge[128];
   int		devshort,
		devlong;
   double	centertime,  /* delay time that will center other pulse */
		pwlong,
		pwshort;
   double	mindelay;


    if (newacq)
	mindelay = INOVAMINDELAY;
    else
	mindelay = MINDELAY;

   if ( (device1 < 1) || (device1 > NUMch) ||
        (device2 < 1) || (device2 > NUMch))
   {
      sprintf(msge,
		"sim2pulse: device # %d or %d is not within bounds 1 - %d\n",
			device1, device2, NUMch);
      text_error(msge);
      psg_abort(1);
   }
   else if (device1 == device2)
   {
      text_error("sim2pulse:  RF devices must be different\n");
      psg_abort(1);
   }
       
    
   if ( (RF_Channel[device1] == NULL) || (RF_Channel[device2] == NULL) )
   {
      sprintf(msge,
		"sim2pulse:  RF Channel device # %d or %d is not present.\n",
			device1, device2);
      text_error(msge);
      psg_abort(1);
   }

   if ( (pw1 < 0.0) || (pw2 < 0.0) )
   {
      if (ix == 1)
      {
         text_error("\nWarning:  gensim2pulse() pulse value set to zero.\n");
         text_error("A negative delay is not executable.\n");
      }

      pw1 = ( (pw1 < 0.0) ? 0.0 : pw1 );
      pw2 = ( (pw2 < 0.0) ? 0.0 : pw2 );
   }
   else if ( ((pw1 > 0.0) && (pw1 < mindelay)) ||
	     ((pw2 > 0.0) && (pw2 < mindelay)) )
   {
      if (ix == 1)
      {
         text_error("\nWarning:  gensim2pulse() pulse value set to zero.\n");
         text_error("A non-zero pulse width < 0.2 us is not executable!\n");
      }

      pw1 = ( ((pw1 > 0.0) && (pw1 < mindelay)) ? 0.0 : pw1 );
      pw2 = ( ((pw2 > 0.0) && (pw2 < mindelay)) ? 0.0 : pw2 );
   }


   if ((pw1 >= mindelay) || (pw2 >= mindelay))
   {
      if ( (fabs(pw1 - pw2) <= (2 * mindelay)) &&
		(fabs(pw1 - pw2) > 0.0) )
      {
	 if (pw1 > pw2)
	 {
	    pw1 += 2.1 * mindelay;
	 }
	 else if (pw2 > pw1)
	 {
	    pw2 += 2.1 * mindelay;
	 }
      }

      if (pw2 > pw1)
      {
         pwshort = pw1;
         pwlong = pw2;
         devshort = device1;
         devlong = device2;
      }
      else
      {
         pwshort = pw2;
         pwlong = pw1;
         devshort = device2;
         devlong = device1;
      }

      okinhwloop();
      centertime = (pwlong - pwshort) / 2.0;

      /* --- set phase bits in rfphpt & curdec --- */
      SetRFChanAttr(RF_Channel[device1], SET_RTPHASE90, phase1, 0);
      SetRFChanAttr(RF_Channel[device2], SET_RTPHASE90, phase2, 0);

      if (ap_interface < 4)
      {
	 sisdecblank(OFF);
         HSgate(rcvr_hs_bit, TRUE);		 /* gate reeiver off       */
      }
      else
      {  
	 if (newacq)
	    HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
	 else
	    SetRFChanAttr(RF_Channel[OBSch],    SET_RCVRGATE, OFF, 0);

         SetRFChanAttr(RF_Channel[devlong],  SET_RCVRGATE, OFF, 0);
         SetRFChanAttr(RF_Channel[devshort], SET_RCVRGATE, OFF, 0);
      }
      delayer(rx1, FALSE);			 /* pre-pulse delay        */

      SetRFChanAttr(RF_Channel[devlong], SET_XMTRGATE, ON, 0);
      delayer(centertime, FALSE);
      SetRFChanAttr(RF_Channel[devshort], SET_XMTRGATE, ON, 0);
      delayer(pwshort, FALSE);

      SetRFChanAttr(RF_Channel[devshort], SET_XMTRGATE, OFF, 0);
      delayer(centertime, FALSE);
      SetRFChanAttr(RF_Channel[devlong], SET_XMTRGATE, OFF, 0);

      delayer(rx2, FALSE);			 /* post-pulse delay	   */
      if (ap_interface < 4)
      {  if (!rcvroff_flag)			 /* reset receiver gate     */
            HSgate(rcvr_hs_bit, FALSE);
	 sisdecblank(ON);
      }
      else
      {
	 if (newacq) {
	    if (!rcvroff_flag)		/* turn receiver back on only if */
	   	HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
	 }
	 else {
            if ( isBlankOn(OBSch) )
            	SetRFChanAttr(RF_Channel[OBSch],    SET_RCVRGATE, ON, 0);
	 }
         if ( isBlankOn(devlong) )
            SetRFChanAttr(RF_Channel[devlong],  SET_RCVRGATE, ON, 0);
         if ( isBlankOn(devshort) )
            SetRFChanAttr(RF_Channel[devshort], SET_RCVRGATE, ON, 0);
      }
   }
}

/*-------------------------------------------------------------------
|
|	gensim3pulse(pw1, pw2, pw3, phase1, phase2, phase3, rx1,
|			rx2, device1, device2, device3)/11
|
|	It generates a simultaneous pulse on three RF channels.
|	The three pulse widths are centered with respect to one
|	another.  The receiver is gated of rx1 sec before the
|	pulse and turned back on rx2 sec after the pulse. Any
|	or all of the pulse times cna have values of 0.0.
|
|				Author Greg Brissey  1/01/90
|   Modified   Author     Purpose
|   --------   ------     -------
|   1/18/90   Greg B.     1. Use new RF Channel Objects 
+------------------------------------------------------------------*/
void gensim3pulse(pw1, pw2, pw3, phase1, phase2, phase3, rx1, rx2,
		device1, device2, device3)
codeint	phase1,		/* phase for first RF channel			*/
	phase2,		/* phase for second RFchannel			*/
	phase3;		/* phase for third RF channel			*/
int	device1,	/* name of first RF channel, e.g., TODEV	*/
	device2,	/* name of second RF channel			*/
	device3;	/* name of third RF channel			*/
double	pw1,		/* pulse length for first RF channel		*/
	pw2,		/* pulse length for second RF channel		*/
	pw3,		/* pulse length for third RF channel		*/
	rx1,		/* pre-pulse delay				*/
	rx2;		/* post-pulse delay				*/
{
   char		msge[128];
   int		devshort,
		devmed,
		devlong;
   double	centertime1,  /* time that centers longest two pulses	*/
		centertime2,  /* time that centers shortest two pulses	*/
		pwshort,
		pwmed,
		pwlong;
   double	mindelay;


    if (newacq)
	mindelay = INOVAMINDELAY;
    else
	mindelay = MINDELAY;

   if ( (device1 < 1) || (device1 > NUMch) ||
        (device2 < 1) || (device2 > NUMch) ||
        (device3 < 1) || (device3 > NUMch))
   {
      sprintf(msge,
	"gensim3pulse(): device # %d, %d, or %d is not within bounds 1 - %d\n",
        		device1, device2, device3, NUMch);
      text_error(msge);
      psg_abort(1);
   }
   else if ( (device1 == device2) || (device1 == device3) ||
		(device2 == device3) )
   {
      text_error("gensim3pulse():  all three RF devices must be different\n");
      psg_abort(1);
   }

   if ( (RF_Channel[device1] == NULL) || 
        (RF_Channel[device2] == NULL) ||
        (RF_Channel[device3] == NULL) )
   {
      sprintf(msge,
	   "sim3pulse:  RF Channel device # %d, %d, or %d is not present.\n",
		device1, device2, device3);
      text_error(msge);
      psg_abort(1);
   }


   if ( (pw1 < 0.0) || (pw2 < 0.0) || (pw3 < 0.0) )
   {
      if (ix == 1)
      {
         text_error("\nWarning:  gensim3pulse() pulse value set to zero.\n");
         text_error("A negative delay is not executable.\n");
      }

      pw1 = ( (pw1 < 0.0) ? 0.0 : pw1 );
      pw2 = ( (pw2 < 0.0) ? 0.0 : pw2 );
      pw3 = ( (pw3 < 0.0) ? 0.0 : pw3 );
   }
   else if ( ((pw1 > 0.0) && (pw1 < mindelay)) ||
	     ((pw2 > 0.0) && (pw2 < mindelay)) ||
	     ((pw3 > 0.0) && (pw3 < mindelay)) )
   {
      if (ix == 1)
      {
         text_error("\nWarning:  gensim3pulse() pulse value set to zero.\n");
         text_error("A non-zero pulse width < 0.2 us is not executable!\n");
      }

      pw1 = ( ((pw1 > 0.0) && (pw1 < mindelay)) ? 0.0 : pw1 );
      pw2 = ( ((pw2 > 0.0) && (pw2 < mindelay)) ? 0.0 : pw2 );
      pw3 = ( ((pw3 > 0.0) && (pw3 < mindelay)) ? 0.0 : pw3 );
   }


   if ((pw1 >= mindelay) || (pw2 >= mindelay) || (pw3 >= mindelay))
   {
      if (pw1 > pw2)
      {
         if (pw1 > pw3)
         {
            pwlong = pw1;
            devlong = device1;

            if (pw3 > pw2)
            {
               pwshort = pw2;
               pwmed = pw3;
               devshort = device2;
               devmed = device3;
            }
            else
            {
               pwshort = pw3;
               pwmed = pw2;
               devshort = device3;
               devmed = device2;
            }
         }
         else
         {
            pwlong = pw3;
            pwmed = pw1;
            pwshort = pw2;
            devlong = device3;
            devmed = device1;
            devshort = device2;
         }
      }
      else
      {
         if (pw2 > pw3)
         {
            pwlong = pw2;
            devlong = device2;

            if (pw3 > pw1)
            {
               pwshort = pw1;
               pwmed = pw3;
               devshort = device1;
               devmed = device3;
            }
            else
            {
               pwshort = pw3;
               pwmed = pw1;
               devshort = device3;
               devmed = device1;
            }
         }
         else
         {
            pwlong = pw3;
            pwmed = pw2;
            pwshort = pw1;
            devlong = device3;
            devmed = device2;
            devshort = device1;
         }
      }

      if ( (fabs(pwlong - pwmed) <= (2 * mindelay)) &&
		(fabs(pwlong - pwmed) > 0.0) )
      {
         pwlong += 2.1 * mindelay;
      }
      else if ( (fabs(pwmed - pwshort) <= (2 * mindelay)) &&
		(fabs(pwmed - pwshort) > 0.0) )
      {
         pwshort -= 2.1 * mindelay;
         if (pwshort < mindelay)
            pwshort = 0.0;
      }

      okinhwloop();
      centertime1 = (pwlong - pwmed) / 2.0;
      centertime2 = (pwmed - pwshort) / 2.0;

      /* --- set phase bits in rfphpt & curdec --- */
      SetRFChanAttr(RF_Channel[device1], SET_RTPHASE90, phase1, 0);
      SetRFChanAttr(RF_Channel[device2], SET_RTPHASE90, phase2, 0);
      SetRFChanAttr(RF_Channel[device3], SET_RTPHASE90, phase3, 0);

      if (ap_interface < 4)
         HSgate(rcvr_hs_bit, TRUE);		/* gate receiver off        */
      else
      {  
	 if (newacq)
	    HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
	 else
	    SetRFChanAttr(RF_Channel[OBSch],    SET_RCVRGATE, OFF, 0);

         SetRFChanAttr(RF_Channel[devlong],  SET_RCVRGATE, OFF, 0);
         SetRFChanAttr(RF_Channel[devmed],   SET_RCVRGATE, OFF, 0);
         SetRFChanAttr(RF_Channel[devshort], SET_RCVRGATE, OFF, 0);
      }
      delayer(rx1, FALSE);			/* pre-pulse delay	    */

      SetRFChanAttr(RF_Channel[devlong], SET_XMTRGATE, ON, 0);
      delayer(centertime1, FALSE);
      SetRFChanAttr(RF_Channel[devmed], SET_XMTRGATE, ON, 0);
      delayer(centertime2, FALSE);
      SetRFChanAttr(RF_Channel[devshort], SET_XMTRGATE, ON, 0);
      delayer(pwshort, FALSE);

      SetRFChanAttr(RF_Channel[devshort], SET_XMTRGATE, OFF, 0);
      delayer(centertime2, FALSE);
      SetRFChanAttr(RF_Channel[devmed], SET_XMTRGATE, OFF, 0);
      delayer(centertime1, FALSE);
      SetRFChanAttr(RF_Channel[devlong], SET_XMTRGATE, OFF, 0);

      delayer(rx2, FALSE);			/* post-pulse delay	    */
      if (ap_interface < 4)
      {  if (!rcvroff_flag)			/* reset receiver gate	    */
            HSgate(rcvr_hs_bit, FALSE);
      }
      else
      {
	 if (newacq) {
	    if (!rcvroff_flag)		/* turn receiver back on only if */
	   	HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
	 }
	 else {
            if ( isBlankOn(OBSch) )
            	SetRFChanAttr(RF_Channel[OBSch],    SET_RCVRGATE, ON, 0);
	 }
         if ( isBlankOn(devlong) )
            SetRFChanAttr(RF_Channel[devlong],  SET_RCVRGATE, ON, 0);
         if ( isBlankOn(devmed) )
            SetRFChanAttr(RF_Channel[devmed],   SET_RCVRGATE, ON, 0);
         if ( isBlankOn(devshort) )
            SetRFChanAttr(RF_Channel[devshort], SET_RCVRGATE, ON, 0);
      }
   }
}

#define MAXACQISTR	6
/*-------------------------------------------------------------------
|
|  write_to_acqi(label, value, units, min, max, type, scale, counter, device)/9
|
|				Author Frits Vosman 1/3/89
|
| Mod. 4/16/91 to include device or rf channel  Greg B.
+------------------------------------------------------------------*/
void write_to_acqi(char *label, double value, double units,
                   int min, int max, int type, int scale, codeint counter, int device)
{
char    filename[80];
char	tmplabel[8];

   if (ix != 1) return;
   
   if (!sliderfile)
   {
      strcpy(filename,getenv("vnmrsystem") );
      strcat(filename,"/acqqueue/acqi.IPA");
      sliderfile=fopen(filename,"w");
   }
   if ((int)strlen(label) > MAXACQISTR)
   {
      strncpy(tmplabel,label,MAXACQISTR);
 	tmplabel[MAXACQISTR] = '\0';
   }
   else
   {
      strcpy(tmplabel,label);
   }
   if (newacq)
	add_to_ipalist(tmplabel, value, units, min, max, type, scale,
							 counter, device);
   else
	fprintf(sliderfile,"%6s %g %g %d %d %d %d %d %d\n", tmplabel, value,
				units, min, max, type, scale, counter, device);
}

#define MAX_RFCHAN_NUM	6
void 
G_Simpulse(int firstkey, ...)
{
va_list	ptr_to_args;
char	msg[80];
codeint	phase[MAX_RFCHAN_NUM+1];
double	pw[MAX_RFCHAN_NUM+1],
	d_tmp,
	rx1 = 0.0,
	rx2 = 0.0;
double	mindelay;
int	device[MAX_RFCHAN_NUM+1],
	ndevice,
	nphase,
	npulse,
	i_tmp,
	i,
        keyword,
	j;
/* initialize for variable arguments */
   va_start(ptr_to_args, firstkey);
   keyword = firstkey;
   ndevice = nphase = npulse = 0;
/* now check if the user want it his own way */
   while (keyword && ok)
   {
      switch (keyword)
      {
	 case PULSE_PHASE:
	    i = 0;
	    do
	    {  phase[i] = va_arg(ptr_to_args, int);
	       i++;
	    } while (phase[i-1] != 0 && i<6);
	    nphase = i-1;
	    break;
	 case PULSE_DEVICE:
	    i = 0;
	    do
	    {  device[i] = va_arg(ptr_to_args, int);
	       i++;
	    } while (device[i-1] != 0 && i<6);
	    ndevice = i-1;
	    break;
	 case PULSE_WIDTH:
	    i = 0;
	    if (ndevice != 0)
            {  i_tmp = ndevice;
            }
	    else if (nphase != 0)
	       {  i_tmp = nphase;
               }
               else
	       { fprintf(stdout,"G_Simpulse: PULSE_WIDTH must be used after\n");
		 fprintf(stdout,"            PULSE_PHASE or PULSE_DEVICE\n\n");
		 psg_abort(1);
               }
	    do
	    { pw[i] =  va_arg(ptr_to_args, double);
	      i++;
	    } while (i<i_tmp);
	    npulse = i_tmp;
	    pw[i_tmp+1] =  va_arg(ptr_to_args, double); /* read the last 0.0 */
	    break;
	 case PULSE_PRE_ROFF:
	    rx1 = va_arg(ptr_to_args, double);
	    break;
	 case PULSE_POST_ROFF:
	    rx2 = va_arg(ptr_to_args, double);
	    break;
	 default:
	    fprintf(stdout,"G_Simpulse(): wrong keyword %d specified\n", keyword);
	    ok = FALSE;
	    break;
      }
      keyword = va_arg(ptr_to_args, int);
   }
   va_end(ptr_to_args);

   if (newacq)
	mindelay = INOVAMINDELAY;
   else
	mindelay = MINDELAY;

/* the # of pw-s, devices and phases must be the same */
   if (npulse!=nphase || npulse!=ndevice)
   {  text_error("G_Simpulse: # of pw-s does not match # of phases or # of devices");
      psg_abort(1);
   }
/* next we do some error checking, resetting of pw, etc */
   for (i=0; i<ndevice; i++)
   {  if ( pw[i] < 0.0 )
      {  if (ix == 1)
         {  text_error("\nWarning:  G_Simpulse() pulse value set to zero.\n");
            text_error("            A negative delay is not executable.\n");
         }
         pw[i] = 0.0;
	 npulse -= 1;
      }
      else if ( (pw[i] > 0.0) && (pw[i] < mindelay) )
      {  if (ix == 1)
         {  text_error("\nWarning:  G_Simpulse() pulse value set to zero.\n");
            text_error("            A non-zero pulse width < 0.2 us is\n");
	    text_error("            not executable!\n");
         }
         pw[i] = 0.0;
	 npulse -= 1;
      }
      else if (pw[i] == 0.0 )
      {  npulse -= 1;
      }
   }
   if (npulse == 0) return;
/* now sort pw, phase, device by pw-length	*/
/* this could be fancier, but there will never	*/
/* be mode than 6				*/
/* this should sort in descending order		*/
/* we use ndevice as maximum index, that way	*/
/* all pulses that were set to 0.0 will be at	*/
/* the end and can be ignored (note that npulse	*/
/* is already decremented accordingly)		*/
   for (i=0; i<ndevice; i++)
      for (j=0; j<(ndevice-1); j++)
      {  if (pw[j] < pw[j+1])
	 {  d_tmp   = pw[j+1];
	    pw[j+1] = pw[j];
	    pw[j]   = d_tmp;
	    i_tmp       = device[j+1];
	    device[j+1] = device[j];
	    device[j]   = i_tmp;
	    i_tmp      = phase[j+1];
	    phase[j+1] = phase[j];
	    phase[j]   = i_tmp;
	 }
      }
   for (i=0; i<npulse; i++)
   {  if ( (device[i] <= 0) || (device[i] > NUMch) )
      {  sprintf(msg,"G_Simpulse(): device #%d not with in bounds of 1 - %d",
		device[i],NUMch);
	 text_error(msg);
	 psg_abort(1);
      }
      for (j=0; j<npulse; j++)
      {  if (i!=j)
	    if (device[i] == device[j])
	    {  text_error("G_Simpulse(): all RF devices numst be different");
	       psg_abort(1);
	    }
      }
      if (RF_Channel[device[i]] == NULL) 
      {  sprintf(msg,
	   "G_Simpulse():  RF Channel device #%d is not present.\n",device[i]);
         text_error(msg);
         psg_abort(1);
      }
   }
   okinhwloop();
/*   fprintf(stdout,"DEV\tPHASE\tPW\n");
 *   for (i=0;i<ndevice;i++)
 *      fprintf(stdout," %d\t%d\t%f\n",device[i],phase[i],pw[i]);
 */
/* set all devices anyway, the next element may count on this phase */
   for (i=0; i<npulse; i++)
   {  SetRFChanAttr(RF_Channel[device[i]], SET_RTPHASE90, phase[i], 0);
   }
   
   if (newacq)
	HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
   else
   	SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, OFF, 0);
   if (ap_interface < 4)
   {
      sisdecblank(OFF);
      HSgate(rcvr_hs_bit, TRUE);		/* gate receiver off        */
   }
   else
   {  for (i=0; i<npulse; i++)
         SetRFChanAttr(RF_Channel[device[i]], SET_RCVRGATE, OFF, 0);
   }
   delayer(rx1, FALSE);			/* pre-pulse delay	    */

   for (i=0; i<(npulse-1); i++)
   {  SetRFChanAttr(RF_Channel[device[i]], SET_XMTRGATE, ON, 0);
      d_tmp = (pw[i] - pw[i+1])/2.0;
      /* if (0 < delta < mindelay/2) make these pulses the same */
      if (d_tmp >= mindelay/2.0)
      {
         if (d_tmp < mindelay)
            delayer(mindelay,FALSE);
         else
            delayer(d_tmp, FALSE);
      }
   }
   SetRFChanAttr(RF_Channel[device[npulse-1]], SET_XMTRGATE, ON, 0);
   delayer(pw[npulse-1],FALSE);
   for (i=(npulse-1); i>0; i--)
   {  SetRFChanAttr(RF_Channel[device[i]], SET_XMTRGATE, OFF, 0);
      d_tmp = (pw[i-1] - pw[i])/2.0;
      /* if (0 < delta < mindelay/2) make these pulses the same */
      if (d_tmp >= mindelay/2.0)
      {
         if (d_tmp < mindelay)
            delayer(mindelay,FALSE);
         else
            delayer(d_tmp, FALSE);
      }
   }
   SetRFChanAttr(RF_Channel[device[0]], SET_XMTRGATE, OFF, 0);

   delayer(rx2, FALSE);			/* post-pulse delay	    */
   if (ap_interface < 4)
   {  if (!rcvroff_flag)		/* reset receiver gate	    */
         HSgate(rcvr_hs_bit, FALSE);
      sisdecblank(ON);
   }
   else
   {
      if (newacq) {
	 if (!rcvroff_flag)		/* turn receiver back on only if */
	   	HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
	 }
      else {
         if ( isBlankOn(OBSch) )
            SetRFChanAttr(RF_Channel[OBSch], SET_RCVRGATE, ON, 0);
      }
      for (i=0; i<npulse; i++)
         if ( isBlankOn(device[i]) )
            SetRFChanAttr(RF_Channel[device[i]], SET_RCVRGATE, ON, 0);
   }
}

/* Prototype Phase Table */

#ifndef PSG_LC
#define PSG_LC
#endif
#include "lc.h"
#include "aptable.h"

#ifdef DEBUG
#define DPRINT0(str) \
        fprintf(stderr,str)
#define DPRINT1(str, arg1) \
        fprintf(stderr,str,arg1)
#define DPRINT2(str, arg1, arg2) \
        fprintf(stderr,str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
        fprintf(stderr,str,arg1,arg2,arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4) \
        fprintf(stderr,str,arg1,arg2,arg3,arg4)
#define DPRINT7(str, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
        fprintf(stderr,str,arg1,arg2,arg3,arg4, arg5, arg6, arg7)
#else
#define DPRINT0(str)
#define DPRINT1(str, arg2)
#define DPRINT2(str, arg1, arg2)
#define DPRINT3(str, arg1, arg2, arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4)
#define DPRINT7(str, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#endif

extern Acqparams *Alc;

#define TBLERROR		-1
#define NONE   -1
#define PHASE1 11
#define PHASE2 12
#define PHASE3 13
#define CYCLE  21
#define CYCLE2 22
#define FAD    31

static int  cy0123[] = {0, 1, 2, 3 };
static int  cy0213[] = {0, 2, 1, 3 };

static void add_fad(int main_phase[], int len, char *parname)
{
   int i;
   int phase = getval(parname);

   DPRINT2("adding fad for %s = %d value\n",parname,phase);
   if ((phase == 1) || (phase == 2))
   {
      int idn = Alc->acqid2;

      DPRINT2("fad value for increment %d is %s\n",idn,
                      (idn % 2) ? "two" : "zero");
      if (idn % 2)
         for (i=0; i < len; i++)
            main_phase[i] += 2;
   }
}

static void add_phase(int main_phase[], int len, char *parname)
{
   int i;
   int phase = getval(parname);

   DPRINT2("adding %s = %d value\n",parname,phase);
   if (phase == 2)
   {
      DPRINT0("increment value is one\n");
      for (i=0; i < len; i++)
         main_phase[i] += 1;
   }
   else if (phase == 3)
   {
      int idn = 0;

      if (strcmp(parname,"phase") == 0)
         idn = Alc->acqid2;
      else if (strcmp(parname,"phase2") == 0)
         idn = Alc->acqid3;
      else if (strcmp(parname,"phase3") == 0)
         idn = Alc->acqid4;
      DPRINT1("increment value is %d\n",idn);
      for (i=0; i < len; i++)
         main_phase[i] += idn;
   }
}
static void add_cycle(int main_phase[], int *len, int cycle[])
{
   int i,j;

   for (i=0; i < 4; i++)
      for (j=0; j< *len; j++)
      {
         main_phase[*len * i + j] = main_phase[j] + cycle[i];
         DPRINT7("i= %d j= %d ph[%d](%d) = ph[%d](%d) + %d\n",i,j, *len * i + j,
              main_phase[*len * i + j], j, main_phase[j], cycle[i]);
      }
   *len *= 4;
   DPRINT1("after cycle main phase elements is %d\n",*len);
}

void make_table(int main_phase[], int *phase_len, int mod_list[], int mod_count)
{
   int i=0;
   int j;

#ifdef DEBUG
   DPRINT1("main phase elements is %d\n",*phase_len);
   for (j=0; j< *phase_len; j++)
      DPRINT2("main phase element %d is %d\n",j+1, main_phase[j]);
   DPRINT1("%d modifiers supplied \n",mod_count);
#endif
   while (i < mod_count)
   {
      j = mod_list[i];
      switch (j)
      {
         case PHASE1:
                  DPRINT2("modifier %d is %s \n",i+1, "PHASE1");
                  add_phase(main_phase, *phase_len,"phase");
                  break;
         case PHASE2:
                  DPRINT2("modifier %d is %s \n",i+1, "PHASE2");
                  add_phase(main_phase, *phase_len,"phase2");
                  break;
         case PHASE3:
                  DPRINT2("modifier %d is %s \n",i+1, "PHASE3");
                  add_phase(main_phase, *phase_len,"phase3");
                  break;
         case CYCLE:
                  DPRINT2("modifier %d is %s \n",i+1, "CYCLE");
                  add_cycle(main_phase, phase_len,cy0123);
                  break;
         case CYCLE2:
                  DPRINT2("modifier %d is %s \n",i+1, "CYCLE2");
                  add_cycle(main_phase, phase_len,cy0213);
                  break;
         case FAD:
                  DPRINT2("modifier %d is %s \n",i+1, "FAD");
                  add_fad(main_phase, *phase_len,"phase");
                  break;
         default:
                  DPRINT3("modifier %d is %s (val = %d) \n",i+1, "NONE",j);
                  break;
      }
      i++;
   }
   for (j=0; j< *phase_len; j++)
   {
      DPRINT3("phase element[%d] is %d mod 4 = %d\n",j+1,
                      main_phase[j], main_phase[j] % 4);
      main_phase[j] %= 4;
   }
}

int modtype(char *modifier)
{
   if ( (strcmp("phase",modifier) == 0) ||
        (strcmp("phase1",modifier) == 0) )
      return(PHASE1);
   else if (strcmp("phase2",modifier) == 0)
      return(PHASE2);
   else if (strcmp("phase3",modifier) == 0)
      return(PHASE3);
   else if (strcmp("cycle",modifier) == 0)
      return(CYCLE);
   else if (strcmp("cycle2",modifier) == 0)
      return(CYCLE2);
   else if ( (strcmp("fad",modifier) == 0) ||
             (strcmp("fad1",modifier) == 0) )
      return(FAD);
   else
      return(NONE);
}
int get_mod(arg,index,modifier)
char arg[];
int *index;
int *modifier;
{
    int i = *index;
    int j = 0;
    int c;
    char mod[1024];

    while (((c = arg[i]) != '\0') && ((c == ' ') || (c == '\t') || (c == '+')))
       i++;   /* skip white space */
    while (((c = arg[i]) != '\0') && (c != ' ') && (c != '\t') && (c != '+') )
       mod[j++] = arg[i++];
    mod[j] = '\0';
    *index = i;
    DPRINT1("check modifier %s is ",mod);
    *modifier = modtype(mod);
    DPRINT1("%s\n",(*modifier == NONE) ? "not ok" : "ok");
    return(*modifier );
}
int
get_main_phase(arg,index,val)
char arg[];
int *index;
int val[];
{
   int i=0;
   int j=0;
   int length = strlen(arg);

   while ( (i < length) && (arg[i] != '\0') &&
           (arg[i] != '+') )
   {
      if ((arg[i] == '0') || (arg[i] == '1') ||
          (arg[i] == '2') || (arg[i] == '3') )
         val[j++] = arg[i] - '0';
      i++;
   }
   *index = i;
   DPRINT1("main phase elements is %d\n",j);
   return(j);
}

void settable90(int device, char arg[])
{
   int main_phase[1024];
   int modifier;
   int mod_list[256];
   int index;
   int phaselen;
   int mod_count;
   int i,j,len;

   mod_count = 0;
   DPRINT2("use phase table on channel %d of %s\n",device,arg);
   phaselen = get_main_phase(arg,&index,main_phase);
   while (get_mod(arg,&index,&modifier) != NONE)
      mod_list[mod_count++] = modifier;
   make_table(main_phase,&phaselen,mod_list,mod_count);
   putcode(SETPHAS90);	/* replaces TXPHASE, DECPHASE */
   putcode((device & 0x0007) | 0x0008 | ((phaselen << 4) & 0xfff0) );	/* table value */
   len = (phaselen + 7 ) / 8;
   j=0;
   for (index=0; index < len; index++)
   {
      modifier = i=0;
      while ((j<phaselen) && (i<8))
      {
         modifier |= (main_phase[j++] << i*2);
         i++;
      }
      DPRINT2("compressed element %d is 0x%x\n",index,modifier);
      putcode( (codeint) modifier);
   }
}

void tabsetreceiver(char arg[])
{
   int main_phase[1024];
   int modifier;
   int mod_list[256];
   int index;
   int phaselen;
   int mod_count;

   notinhwloop("setreceiver()");
   mod_count = 0;
   DPRINT1("use phase table on receiver of %s\n",arg);
   phaselen = get_main_phase(arg,&index,main_phase);
   while (get_mod(arg,&index,&modifier) != NONE)
      mod_list[mod_count++] = modifier;
   make_table(main_phase,&phaselen,mod_list,mod_count);
   if (phaselen >= 1)
   {
      codeint acodeloc;
      codeint indexloc;

      putcode(TABLE);
      acodeloc = Codeptr - Aacode;
      putcode((codeint) phaselen);	/* size of table */
      putcode((codeint) 0);		/* auto-inc flag */
      putcode((codeint) 1);		/* divn-rtrn factor */
      putcode(0);			/* auto-inc ct loc */
      for (index = 0; index < phaselen; index++)
         putcode((codeint) main_phase[index]);

      putcode(TASSIGN);
      putcode(acodeloc);
      indexloc = (codeint) ctss;
      if (!newacq)
        indexloc += 1;
      putcode(indexloc);
      putcode(oph);
   }
}

void setreceiver(void *phaseptr)
{
   if (phase_var_type((int) phaseptr) == SET_RTPHASE90)
      APsetreceiver((int) phaseptr);
   else
      tabsetreceiver((char *)phaseptr);
}


void zgradpulse(double gval, double gdelay)
{
   char sgradientshaping[4];

   if (gdelay > 2e-7)
   {

     /*
     P_getstring(GLOBAL,"gradientshaping",&sgradientshaping,1,2);
     printf("gradientshaping = %s\n",sgradientshaping);
     */
     if ( P_getstring(GLOBAL,"gradientshaping",sgradientshaping,1,2) == 0 )
     {
       if ((sgradientshaping[0] != 'y'))
       {
          if ((sgradientshaping[0] == 's'))
          {
             DPRINT0("no gradients applied for zgradpulse. gradientshaping='s'\n");
             if ( bgflag && (ix == 1) ) printf("no gradients applied for zgradpulse. gradientshaping='s'\n");
             if (ix == 1) warn_message("warning: no gradients applied for zgradpulse. gradientshaping='s'\n");
             delay(gdelay+GRADIENT_DELAY);
          }
          else
          {
             DPRINT0("standard (rectangular) gradients used for zgradpulse. gradientshaping='n'\n");
             if ( bgflag && (ix == 1) ) printf("standard (rectangular) gradients used for zgradpulse. gradientshaping='n'\n");
             rgradient('z',gval);
             delay(gdelay);
             rgradient('z',0.0);
          }
       }
       else
       {
        /* WURST-40 gradient shape */
        static double mgrad[21]= {0.0,0.07,0.24,0.42,0.68,0.94,1.0,
                             1.0,1.0,1.0,1.0,1.0,1.0,1.0,
                             1.0,0.94,0.68,0.42,0.24,0.07,0.0};
        double gstep,gampl;
        int jcnt;
        DPRINT0("shaped gradients (WURST) used for zgradpulse. gradientshaping='y'\n");
        if ( bgflag && (ix == 1) ) printf("shaped gradients (WURST) used for zgradpulse. gradientshaping='y'\n");
        if ((gdelay/21.0) < (8.65e-6))
        {
          text_error("gradient duration %g too short for shaping. set duration to %g \n",gdelay,(22.0*8.65e-6));
          psg_abort(1);
        }
        else
          gstep = ((gdelay)/21.0);
        for (jcnt=0;jcnt<=20;jcnt++)
          { gampl=(mgrad[jcnt]*gval);
           rgradient('Z',gampl);
           delay(gstep - GRADIENT_DELAY); }
        rgradient('Z',0.0);
       }
     }
     else
     {
        DPRINT0("standard (rectangular) gradients used for zgradpulse. gradientshaping does not exist in global tree\n");
        if ( bgflag && (ix == 1) ) printf("standard (rectangular) gradients used for zgradpulse. gradientshaping does not exist in global tree\n");
        rgradient('z',gval);
        delay(gdelay);
        rgradient('z',0.0);
     }


   }
   else
   {
      delay(2*GRADIENT_DELAY);
   }

}

