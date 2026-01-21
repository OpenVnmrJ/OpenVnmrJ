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
#include <stdarg.h>

#include "oopc.h"

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

#define DPRTLEVEL 2

typedef struct
{
#include "apbit_device.p"
}               APBit_Object;

extern int AP_Device(void *thisv, Message msg, void *param, void *result);
extern int ClearTable(void *ptr, size_t tablesize);
extern char *ObjError(int wcode);
extern char *ObjCmd(int wcode);

/* base class dispatcher */
#define Base(this,msg,par,res)    AP_Device(this,msg,par,res)

static int init_relay(Msg_New_Result *result);
static int get_attr(APBit_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);
static int set_attr(APBit_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);
static int set_value(APBit_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);

/*-------------------------------------------------------------
| APBit_Device()/4 - Message Handler for apbit_devices.
|			Author: Greg Brissey  8/18/88
+-------------------------------------------------------------*/
int APBit_Device(APBit_Object *this, Message msg, void *param, void *result)
{
   int             error = 0;

   switch (msg)
   {
      case MSG_NEW_DEV_r:
	 error = init_relay( (Msg_New_Result * ) result);
	 break;
      case MSG_SET_APBIT_ATTR_pr:
	 error = set_attr(this, (Msg_Set_Param *) param, (Msg_Set_Result *)result);
	 break;
      case MSG_SET_APBIT_MASK_pr:
	 error = set_value(this, (Msg_Set_Param *) param, (Msg_Set_Result *)result);
	 break;
      case MSG_GET_APBIT_MASK_pr:
         error = get_attr(this, (Msg_Set_Param *) param, (Msg_Set_Result *)result);
         break;
      default:
	 error = Base(this, msg, param, result);
	 break;
   }
   return (error);
}

static int init_relay(Msg_New_Result *result)
{
   result->object = (Object) malloc(sizeof(APBit_Object));
   if (result->object == (Object) 0)
   {
      fprintf(stderr, "Insuffient memory for New APBit Device.");
      return (NO_MEMORY);
   }
   ClearTable(result->object, sizeof(APBit_Object));	/* besure all clear */
   result->object->dispatch = (Functionp) APBit_Device;
   return (0);
}

/*----------------------------------------------------------
| set_value()/3 - static routine that has access to apbit device
|		 bit values
|			Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int set_value(APBit_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;
   int             mask;
   Msg_Set_Param   iparam;
   Msg_Set_Result  iresult;

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case SET_BIT:
	 DPRINT3(DPRTLEVEL,
		 "set_apbit: Cmd: '%s' %d to 1, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 DPRINT1(DPRTLEVEL, "Prior mask: 0x%x, ", this->relaymask);
	 this->relaymask |= 1 << param->value;
	 DPRINT1(DPRTLEVEL, "New mask: 0x%x\n", this->relaymask);
	 break;
      case CLEAR_BIT:
	 DPRINT3(DPRTLEVEL,
		 "set_apbit: Cmd: '%s' %d to 0, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 DPRINT1(DPRTLEVEL, "Prior mask: 0x%x, ", this->relaymask);
	 mask = ~(1 << param->value);
	 this->relaymask &= mask;
	 DPRINT1(DPRTLEVEL, "New mask: 0x%x\n", this->relaymask);
	 break;
      case SET_TRUE:
	 DPRINT3(DPRTLEVEL,
		 "set_apbit: Cmd: '%s' bit %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 iparam.value = param->value;
	 if (this->true_eqs[param->value] == ZERO)
	 {
	    iparam.setwhat = CLEAR_BIT;
	 }
	 else if (this->true_eqs[param->value] == ONE)
	 {
	    iparam.setwhat = SET_BIT;
	 }
	 else
	 {
	    error = ILLEGAL_APBIT_BIT_ATTR;
	    return (error);
	 }
	 error = Send(this, MSG_SET_APBIT_MASK_pr, &iparam, &iresult);
	 break;
      case SET_FALSE:
	 DPRINT3(DPRTLEVEL,
		 "set_apbit: Cmd: '%s' bit %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 iparam.value = param->value;
	 if (this->true_eqs[param->value] == ZERO)
	 {
	    iparam.setwhat = SET_BIT;
	 }
	 else if (this->true_eqs[param->value] == ONE)
	 {
	    iparam.setwhat = CLEAR_BIT;
	 }
	 else
	 {
	    error = ILLEGAL_APBIT_BIT_ATTR;
	    return (error);
	 }
	 error = Send(this, MSG_SET_APBIT_MASK_pr, &iparam, &iresult);
	 break;
      default:
	 error = UNKNOWN_APBIT_ATTR;
	 break;
   }
   return (error);
}

/*----------------------------------------------------------
| get_attr()/3 - static routine that has access to apbit device
|                attributes
|                       Author: Sandy Farmer  5/17/90
+----------------------------------------------------------*/
static int get_attr(APBit_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;
 
   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case GET_PREAMP_SELECT:
         result->reqvalue = ( (this->relaymask >> PREAMP_SELECT) & 1 );
         DPRINT3(DPRTLEVEL,
                 "get_attr: Cmd: '%s', is %d for device: '%s'\n",
                 ObjCmd(param->setwhat), result->reqvalue, this->objname);
         break;
      case GET_VALUE:
         result->reqvalue = this->relaymask ;
         DPRINT3(DPRTLEVEL,
                 "get_attr: Cmd: '%s', is %d for device: '%s'\n",
                 ObjCmd(param->setwhat), result->reqvalue, this->objname);
         break;
      default:
         error = UNKNOWN_APBIT_ATTR;
         break;
   }

   return(error);
}

/*----------------------------------------------------------
| set_attr()/3 - static routine that has access to apbit device
|		 attributes,
|			Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int set_attr(APBit_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case SET_DEFAULTS:
	 DPRINT2(DPRTLEVEL,
		 "set_apb_attr: Cmd: '%s' for device: '%s'\n",
		 ObjCmd(param->setwhat), this->objname);
	 this->dev_state = ACTIVE;
	 this->dev_cntrl = BITMASK;
	 this->dev_channel = OBSERVE;
	 this->ap_adr = RF_RELAY_AP_ADR;
	 this->ap_reg = RF_RELAY_AP_REG;
	 this->ap_bytes = RF_RELAY_BYTES;
	 this->ap_mode = RF_RELAY_MODE;
	 this->relaymask = 0;	/* present Value device is set to */
	 this->true_eqs[0] = 1;	/* default logical true equal to bit set */
	 this->true_eqs[1] = 1;	/* */
	 this->true_eqs[2] = 1;	/* */
	 this->true_eqs[3] = 1;	/* */
	 this->true_eqs[4] = 1;	/* */
	 this->true_eqs[5] = 1;	/* */
	 this->true_eqs[6] = 1;	/* */
	 this->true_eqs[7] = 1;	/* */
	 break;
      case SET_TRUE_EQ_ONE:
	 DPRINT3(DPRTLEVEL,
		 "set_apb_attr: Cmd: '%s' for bit %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->true_eqs[param->value] = 1;	/* */
	 break;
      case SET_TRUE_EQ_ZERO:
	 DPRINT3(DPRTLEVEL,
		 "set_apb_attr: Cmd: '%s' for bit %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->true_eqs[param->value] = 0;	/* */
	 break;
      default:
	 error = UNKNOWN_APBIT_ATTR;
	 result->resultcode = UNKNOWN_APBIT_ATTR;
	 break;
   }
   return (error);
}


/*---------------------------------------------------------------------
| SetAPBit  -  Set AP Bit Device to List of Attributes or Values
|			Author: Greg Brissey  8/18/88
+---------------------------------------------------------------------*/
int SetAPBit(Object obj, ...)
{
   va_list         vargs;
   int             error = 0;
   Msg_Set_Param   param;
   Msg_Set_Result  result;

   va_start(vargs, obj);
	
   /* Options allways follow the format 'Option,(value),Option,(value),etc' */
   while ((param.setwhat = va_arg(vargs, int)) != 0)
   {
      switch (param.setwhat)
      {
	 case SET_BIT:
	 case CLEAR_BIT:
	 case SET_TRUE:
	 case SET_FALSE:
	    param.value = va_arg(vargs, int);
	    error = Send(obj, MSG_SET_APBIT_MASK_pr, &param, &result);
	    break;
         case SET_MASK:
	    param.value = va_arg(vargs, int);
	    error = Send(obj, MSG_SET_AP_ATTR_pr, &param, &result);
	    break;
	 case SET_VALUE:
	    error = Send(obj, MSG_SET_AP_ATTR_pr, &param, &result);
	    break;
	 case SET_DEFAULTS:
	    error = Send(obj, MSG_SET_APBIT_ATTR_pr, &param, &result);
	    break;
	 case SET_TRUE_EQ_ONE:
	 case SET_TRUE_EQ_ZERO:
	    param.value = va_arg(vargs, int);
	    error = Send(obj, MSG_SET_APBIT_ATTR_pr, &param, &result);
	    break;
	 case SET_GTAB:
	    param.value = va_arg(vargs, int);
	    error = Send(obj, MSG_SET_AP_ATTR_pr, &param, &result);
	    break;
	 default:
	    error = UNKNOWN_APBIT_ATTR;
	    break;
      }
      if (error < 0)
	 break;
   }
   va_end(vargs);
   return (error);
}
