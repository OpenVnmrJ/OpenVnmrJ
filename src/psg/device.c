/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  All Device functions are in this file  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

extern char    *ObjError(), *ObjCmd();
extern void abort_message(char *format, ...);

#define DPRTLEVEL 2

typedef struct
{
#include "device.p"
}               Dev_Object;

/* base class dispatcher */
#define Base(this,msg,par,res)  ILLEGAL_MSG;

/*----------------------------------------------------------
| set_attr()/3 - static routine that has access to device
|		 attributes, state,control,channel
|			Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int 
set_attr(Dev_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case SET_DEVSTATE:
	 DPRINT3(DPRTLEVEL,
		 "set_dev_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->dev_state = param->value;
	 break;
      case SET_DEVCNTRL:	/* obsserve, decoupler, etc */
	 DPRINT3(DPRTLEVEL,
		 "set_dev_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->dev_cntrl = param->value;
	 break;
      case SET_DEVCHANNEL:
	 DPRINT3(DPRTLEVEL,
		 "set_dev_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->dev_channel = param->value;
	 break;
      default:
	 error = UNKNOWN_DEV_ATTR;
	 result->resultcode = UNKNOWN_DEV_ATTR;
	 break;
   }
   return (error);
}

/*----------------------------------------------------------
| get_attr()/3 - static routine that has access to device
|		 attributes, state,control,channel
|			Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int 
get_attr(Dev_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case GET_STATE:
	 result->reqvalue = this->dev_state;
	 DPRINT3(DPRTLEVEL,
		 "get_dev_attr: Cmd: '%s', is  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;
      case GET_CNTRL:		/* observe, decoupler, etc */
	 result->reqvalue = this->dev_cntrl;
	 DPRINT3(DPRTLEVEL,
		 "get_dev_attr: Cmd: '%s', is  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;
      case GET_CHANNEL:
	 result->reqvalue = this->dev_channel;
	 DPRINT3(DPRTLEVEL,
		 "get_dev_attr: Cmd: '%s', is  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;
      default:
	 error = UNKNOWN_DEV_ATTR;
	 result->resultcode = UNKNOWN_DEV_ATTR;
	 break;
   }
   return (error);
}

/*-------------------------------------------------------------
| Device()/4 - Message Handler for devices.
|			Author: Greg Brissey  8/18/88
+-------------------------------------------------------------*/
int Device(Dev_Object *this, Message msg, void *param, void *result)
{
   int             error = 0;

   switch (msg)
   {
      case MSG_SET_DEV_ATTR_pr:
	 error = set_attr(this, param, result);
	 break;
      case MSG_GET_DEV_ATTR_pr:
	 error = get_attr(this, param, result);
	 break;
      default:
	 error = UNKNOWN_MSG;
	 break;
   }
   return (error);
}

/*----------------------------------------------------------
|  ObjectNew(dispatch,obj_name)/2 - generate a new object
|			Author: Greg Brissey 8/18/88
+----------------------------------------------------------*/
Object
ObjectNew(Functionp dispatch, char *name)
{
   int             error;
   Msg_New_Result  new;

   if (*dispatch == NULL)
   {
      abort_message("%s : No dispatcher routine specified at creation time.\n",
                    name);
      return(NULL);
   }
   error = (*dispatch) (NULL, MSG_NEW_DEV_r, NULL, &new);
   if (error < 0)
   {
      abort_message("%s : %s\n", name, ObjError(error));
      return(NULL);
   }

   new.object->objname = (char *) malloc(strlen(name) + 2);
   if (new.object->objname == (char *) 0)
   {
      fprintf(stdout, "Insuffient memory for New Device Name!!");
      abort_message("%s : Insuffient Memory for New Device Name!!\n", name);
      return(NULL);
   }
   strcpy(new.object->objname, name);
   DPRINT1(DPRTLEVEL, "Created: '%s' Object.\n", new.object->objname);
   return (new.object);
}

/*--------------------------------------------------------------------
| ClearTable()
|       zero out array or structure
|       tableptr - pointer to array or structure
|       tablesize - size of array or structure in Bytes.
|                               Author Greg Brissey 8/9/88
+-------------------------------------------------------------------*/
int ClearTable(void *ptr, size_t tablesize)
{
   size_t   i;
   char *tableptr;

   tableptr = (char *) ptr;

   for (i = 0; i < tablesize; i++)
      *tableptr++ = 0;
   return(0);
}
