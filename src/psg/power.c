/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include "ACQ_SUN.h"
#include "group.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "macros.h"
#include "power.h"

#define AP_REG_BIT	1
#define AP_BYTES_BIT	2
#define AP_MODE_BIT	4
#define AP_ADDR_BIT	8

#define RT_TABOFFSET	2

extern int      acqiflag;	/* for interactive acq. or not? */
extern int	ap_interface;
extern int	newacq;

struct pwrstruct
{
   double          value;
   double          units;
   int	   	   rtval;
   int		   type;		/* COARSE or FINE */
   int		   ap_reg;
   int		   ap_mode;
   int		   ap_bytes;
   int             scale;
   int             max;
   int             min;
   int             device;
   char           *label;
};

/*-------------------------------------------------------------------
|
|	G_Power(variable arg_list)/n
|
|				Author Frits Vosman  1/3/89
| 9/13/90  GMB
| Needs to be altered to handle channel then type of attenuator. 
| but for now it only handles fine attenuators so no harm in changing
| POWER_TO_LN -> TODEV and POWER_DO_LN -> DODEV. But beware, This
| routine is destined to fail with acqi if coarse atten. are attempted
| to be changed.
+------------------------------------------------------------------*/
/*VARARGS1*/
int 
G_Power(int firstkey, ...)
{
   int		   ap_mask, setrtval;
   int             counter;
   int             keyword;
   struct pwrstruct pwrs;
   va_list         ptr_to_args;

/* fill in the defaults */
   pwrs.value  = tpwrf;
   pwrs.rtval	= 0;
   pwrs.type	= FINE;
   pwrs.device = TODEV;  /* Was POWER_TO_LN */
   pwrs.label  = "";
   pwrs.scale  = 0;
   pwrs.max    = 4095;
   pwrs.min    = 0;
   pwrs.units  = 1.0;

   setrtval = FALSE;
   ap_mask = 0;

/* initialize for variable arguments */
   va_start(ptr_to_args, firstkey);
   keyword = firstkey;
/* now check if the user want it his own way */
   while (keyword && ok)
   {
      switch (keyword)
      {
	 case POWER_VALUE:
	    pwrs.value =  va_arg(ptr_to_args, double);
	    break;
	 case POWER_RTVALUE:
	    pwrs.rtval =   va_arg(ptr_to_args, int);
	    setrtval = TRUE;
	    break;
	 case POWER_TYPE:
	    pwrs.type =   va_arg(ptr_to_args, int);
	    setrtval = TRUE;
	    break;
	 case POWER_DEVICE:
	    pwrs.device = va_arg(ptr_to_args, int);
	    break;
	 case POWER_AP_ADDR:
	    pwrs.ap_bytes = va_arg(ptr_to_args, int);
	    ap_mask |= AP_ADDR_BIT;
	    break;
	 case POWER_AP_BYTES:
	    pwrs.ap_bytes = va_arg(ptr_to_args, int);
	    ap_mask |= AP_BYTES_BIT;
	    break;
	 case POWER_AP_MODE:
	    pwrs.ap_mode = va_arg(ptr_to_args, int);
	    ap_mask |= AP_MODE_BIT;
	    break;
	 case POWER_AP_REG:
	    pwrs.ap_reg = va_arg(ptr_to_args, int);
	    ap_mask |= AP_REG_BIT;
	    break;
	 case SLIDER_LABEL:
	    pwrs.label = va_arg(ptr_to_args, char *);
	    break;
	 case SLIDER_SCALE:
	    pwrs.scale = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_MAX:
	    pwrs.max = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_MIN:
	    pwrs.min = va_arg(ptr_to_args, int);
	    break;
	 case SLIDER_UNITS:
	    pwrs.units = va_arg(ptr_to_args, double);
	    break;
	 default:
	    fprintf(stdout, "wrong G_Power() keyword %d specified", keyword);
	    ok = FALSE;
	    break;
      }
      keyword = va_arg(ptr_to_args, int);
   }
   va_end(ptr_to_args);
   if (ap_mask != 0 && ap_mask != 15)
   {
      fprintf(stdout, "\n");
      fprintf(stdout,
       "Warning:  All or none of POWER_AP_ADDR, POWER_AP_BYTES,\n");
      fprintf(stdout,
       "          POWER_AP_MODE and POWER_AP_REG must be specified.\n");
      fprintf(stdout,
       "          Call to G_Power() ignored.\n");
      return;
   }
   /* check if  atten. device exists */
   if (pwrs.type == FINE)
   {
     if (fattn[pwrs.device] == 0)
     {
	fprintf(stdout, "\n");
	fprintf(stdout,
             "Warning:   No Fine Attenuators configured for RF channel: %d\n", 
		pwrs.device);
	fprintf(stdout,
                "          Call to G_Power() ignored.\n");
	return;
     }
   }
   else 	/* coarse */
   {
     if (cattn[pwrs.device] == 0)
     {
	fprintf(stdout, "\n");
	fprintf(stdout,
             "Warning:   No Fine Attenuators configured for RF channel: %d\n", 
		pwrs.device);
	fprintf(stdout,
                "          Call to G_Power() ignored.\n");
	return;
     }
     else 
     {
	if ((pwrs.max == 4095) && (newacq))
	{
	   pwrs.max = 63;
	   pwrs.min = -16;
	}
     }
   }
/* if there was an error, here or in other calls to Pulse, Offset, Gain, */
/* or Delay, return here, checking only the validity of the arguments    */
   if (!ok)
      return;

/* set initial power for inova.  Only for use with rtvar */
   if (setrtval)
   {
      if (pwrs.type == FINE)
	pwrf((c68int)pwrs.rtval,pwrs.device);
      else /* coarse */
	power((c68int)pwrs.rtval,pwrs.device);
   }

/* do we want interactive control? */
#ifdef DOIPA
   if ( strcmp(pwrs.label, "")  && acqiflag )
   {
      if (setrtval && newacq)
         write_to_acqi(pwrs.label, (double)get_acqvar(pwrs.rtval),  
		pwrs.units, pwrs.min, pwrs.max, TYPE_RTPARAM, 
		pwrs.scale, (int)(pwrs.rtval), pwrs.device);
      else
      {
	 if (pwrs.type == COARSE)
         {
	   fprintf(stdout, "\n");
	   fprintf(stdout,
                "Warning:   Coarse attenuators not supported.\n");
	   fprintf(stdout,
                "           Call to G_Power() ignored.\n");
         }
         else
 	 {
            write_to_acqi(pwrs.label, pwrs.value,  pwrs.units, pwrs.min,
		    pwrs.max, pwrs.device, pwrs.scale, 0, pwrs.device);
	 }
      }
   }
#endif
}
