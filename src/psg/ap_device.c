/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* for SFU build, stub code for AP functions is in table.c */
#ifndef __INTERIX

/*  All AP functions are in this file  */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "acodes.h"
#include "oopc.h"

#if defined(PSG_LC) || defined(INTERACT)
#include <stdarg.h>
#endif
#ifdef PSG_LC
#include "abort.h"
#endif

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

#if defined __GNUC__ && __GNUC__ >= 14
#pragma GCC diagnostic warning "-Wimplicit-function-declaration"
#endif

extern char    *ObjError(), *ObjCmd();

#define DPRTLEVEL 2

typedef struct
{
#include "ap_device.p"
}               AP_Object;

/* base class dispatcher */
#define Base(this,msg,par,res)    Device(this,msg,par,res)
static int init_apbyte(Msg_New_Result *result);
static int get_attr(AP_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);
static int set_attr(AP_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);

/*-------------------------------------------------------------
| AP_Device()/4 - Message Handler for ap_devices.
|			Author: Greg Brissey  8/18/88
+-------------------------------------------------------------*/
int 
AP_Device(this, msg, param, result)
AP_Object      *this;
Message         msg;
caddr_t         param;
caddr_t         result;
{
   int             error = 0;

   switch (msg)
   {
      case MSG_NEW_DEV_r:
	 error = init_apbyte( (Msg_New_Result *) result);
	 break;
      case MSG_SET_AP_ATTR_pr:
	 error = set_attr(this, (Msg_Set_Param *) param,  (Msg_Set_Result *) result);
	 break;
      case MSG_GET_AP_ATTR_pr:
	 error = get_attr(this, (Msg_Set_Param *) param,  (Msg_Set_Result *) result);
	 break;
      default:
	 error = Base(this, msg, param, result);
	 break;
   }
   return (error);
}

static int init_apbyte(Msg_New_Result *result)
{
   result->object = (Object) malloc(sizeof(AP_Object));
   if (result->object == (Object) 0)
   {
      fprintf(stderr, "Insuffient memory for New AP Device.");
      return (NO_MEMORY);
   }
   ClearTable(result->object, sizeof(AP_Object));	/* besure all clear */
   result->object->dispatch = (Functionp) AP_Device;
   return (0);
}

/*----------------------------------------------------------
| set_attr()/3 - static routine that has access to ap device
|		 attributes, address,register,bytes,mode
|			Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int set_attr(AP_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;
   int             mask;

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case SET_APADR:
	 DPRINT3(DPRTLEVEL,
		 "set_ap_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->ap_adr = param->value;
	 break;
      case SET_APREG:		/* obsserve, decoupler, etc */
	 DPRINT3(DPRTLEVEL,
		 "set_ap_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->ap_reg = param->value;
	 break;
      case SET_APBYTES:
	 DPRINT3(DPRTLEVEL,
		 "set_ap_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->ap_bytes = param->value;
	 break;
      case SET_APMODE:
	 DPRINT3(DPRTLEVEL,
		 "set_ap_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->ap_mode = param->value;
	 break;
      case SET_MASK:
	 DPRINT3(DPRTLEVEL,
		 "set_apbit: Cmd: '%s' %d to 1, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 DPRINT1(DPRTLEVEL, "Prior mask: 0x%x, ", this->relaymask);
	 this->relaymask = param->value;
	 DPRINT1(DPRTLEVEL, "New mask: 0x%x\n", this->relaymask);
	 break;
      case SET_VALUE:		/* Output Acodes */
	 DPRINT2(DPRTLEVEL,
		 "set_apbit: Cmd: '%s' , for device: '%s'\n",
		 ObjCmd(param->setwhat), this->objname);
	 DPRINT1(DPRTLEVEL, "APBOUT for device: '%s'\n", this->objname);
	 DPRINT1(DPRTLEVEL, "n-1=%d\n", this->ap_bytes);
	 DPRINT1(DPRTLEVEL, "APSELECT=0x%x\n", APSELECT | ((this->ap_adr << 8) & 0x0f00) |
		 (this->ap_reg & 0xff));
	 putcode((c68int) APBOUT);
	 putcode((c68int) this->ap_bytes);	/* fifoword count - 1 */
	 putcode((c68int) APSELECT | ((this->ap_adr << 8) & 0x0f00) |
		 (this->ap_reg & 0xff));
	 if (this->ap_mode == -1)
	    mask = ~(this->relaymask);
	 else
	    mask = this->relaymask;
	 DPRINT2(DPRTLEVEL, "relay mask = 0x%x, conv mask = 0x%x\n",
			this->relaymask, mask);
	 putcode((c68int) APWRITE | ((this->ap_adr << 8) & 0x0f00) |
		 	(mask & 0xff));
	 DPRINT1(DPRTLEVEL, "APWRITE=0x%x\n", APWRITE | ((this->ap_adr << 8) &
			0x0f00) | (mask & 0xff));
         if (this->ap_bytes == 2)
         {
            putcode( (c68int) APPREINCWR | ((this->ap_adr<<8) & 0x0f00) |
                 	( (mask &0xff00) >> 8) );
	 DPRINT1(DPRTLEVEL, "APPREINCWR=0x%x\n", APPREINCWR |
			((this->ap_adr << 8) & 0x0f00) | (mask & 0xff));
         }
	 result->genfifowrds = this->ap_bytes + 1;/* words stuffed in acquisition acode */
	 break;
      case SET_GTAB:
	 DPRINT2(DPRTLEVEL,"set_apbit: Cmd: '%s' , for device: '%s'\n",
		 ObjCmd(param->setwhat), this->objname);
	 DPRINT1(DPRTLEVEL, "APBOUT for device: '%s'\n", this->objname);
	 DPRINT1(DPRTLEVEL, "n-1=%d\n", this->ap_bytes);
	 DPRINT1(DPRTLEVEL, "APSELECT=0x%x\n", APSELECT | ((this->ap_adr << 8) 
		& 0x0f00) | (this->ap_reg & 0xff));

	 putgtab( param->value, (c68int) APBOUT);
	 putgtab( param->value, (c68int) this->ap_bytes); /* fifoword count - 1 */
	 putgtab( param->value, (c68int) APSELECT | ((this->ap_adr << 8) 
				& 0x0f00) |(this->ap_reg & 0xff));
	 if (this->ap_mode == -1)
	    mask = ~(this->relaymask);
	 else
	    mask = this->relaymask;
	 DPRINT2(DPRTLEVEL, "relay mask = 0x%x, conv mask = 0x%x\n",
			this->relaymask, mask);
	 putgtab( param->value, (c68int) APWRITE | ((this->ap_adr << 8) 
			& 0x0f00) | (mask & 0xff));
	 DPRINT1(DPRTLEVEL, "APWRITE=0x%x\n", APWRITE | ((this->ap_adr << 8) &
			0x0f00) | (mask & 0xff));
         if (this->ap_bytes == 2)
         {
	    DPRINT1(DPRTLEVEL, "APPREINCWR=0x%x\n", APPREINCWR |
			((this->ap_adr << 8) & 0x0f00) | (mask & 0xff));
            putgtab( param->value, (c68int) APPREINCWR | ((this->ap_adr<<8) 
			& 0x0f00) | ( (mask &0xff00) >> 8) );
         }
	 result->genfifowrds = this->ap_bytes + 1; /* # words in acq acode */
         break;
      case SET_RTPARAM:
   	DPRINT3(DPRTLEVEL,
	  "set_apbit: Cmd: '%s' to %d, for device: '%s'\n",
	      ObjCmd(param->setwhat),param->value,this->objname);
           putcode((c68int)APCOUT);
	   putcode(((this->ap_adr << 8) & 0xff00) |
                             (this->ap_reg & 0xff) );
	   putcode((c68int) (RTVAR_BIT | this->ap_bytes));
           putcode((c68int) 0xffff);	/* max value */
           putcode((c68int) 0);	/* offset */
	   putcode((c68int) (c68long) param->value); 
	break;
      default:
	 error = UNKNOWN_AP_ATTR;
	 break;
   }
   return (error);
}

/*----------------------------------------------------------
| get_attr()/3 - static routine that has access to ap device
|		 attributes, address,register,bytes,mode
|			Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int get_attr(AP_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case GET_ADR:
	 result->reqvalue = this->ap_adr;
	 DPRINT3(DPRTLEVEL,
		 "get_ap_attr: Cmd: '%s', is  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;
      case GET_REG:		/* obsserve, decoupler, etc */
	 result->reqvalue = this->ap_reg;
	 DPRINT3(DPRTLEVEL,
		 "get_ap_attr: Cmd: '%s', is  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;
      case GET_BYTES:
	 result->reqvalue = this->ap_bytes;
	 DPRINT3(DPRTLEVEL,
		 "get_ap_attr: Cmd: '%s', is  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;
      case GET_MODE:
	 result->reqvalue = this->ap_mode;
	 DPRINT3(DPRTLEVEL,
		 "get_ap_attr: Cmd: '%s', is  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;
      default:
	 error = UNKNOWN_AP_ATTR;
	 result->resultcode = UNKNOWN_AP_ATTR;
   }
   return (error);
}

#if defined(PSG_LC) || defined(INTERACT)
/*---------------------------------------------------------------------
| SetAPAttr  -  Set AP Device to List of Attributes
|			Author: Greg Brissey  8/18/88
+---------------------------------------------------------------------*/
int SetAPAttr(Object attnobj, ...)
{
   va_list         vargs;
   int             error = 0;
   Msg_Set_Param   param;
   Msg_Set_Result  result;

   va_start(vargs, attnobj);

/* See freq_device.c for explanation of code conditionally compiled for VMS */

#ifdef VMS
   error = va_arg(vargs, int);
   error = 0;
#endif 

   /*
    * Options always follow the format
    * 'Attribute,(value),Attribute,(value),etc'
    */
   while ((param.setwhat = va_arg(vargs, int)) != 0)
   {
      if ((param.setwhat != SET_DEFAULTS) && (param.setwhat != SET_VALUE))
			/* default  and value have no value parameter */
	 param.value = (c68int) va_arg(vargs, int);
      error = Send(attnobj, MSG_SET_AP_ATTR_pr, &param, &result);
      if (error < 0)
      {
#ifdef PSG_LC
	 abort_message("%s : %s  '%s'\n", attnobj->objname, ObjError(error),
		 ObjCmd(param.setwhat));
#else
	 Werrprintf("%s : %s  '%s'\n", attnobj->objname, ObjError(error),
		 ObjCmd(param.setwhat));
         abort();
#endif
      }
   }
   va_end(vargs);
   return (error);
}
#endif

#endif // __INTERIX

