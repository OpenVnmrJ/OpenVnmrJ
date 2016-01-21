/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <strings.h>

#define	CURRENT	1

/**************************
*  Objects and Variables  *
**************************/

Object		SpclAttn;	/* Special "third" attenuator */


createspecial()
{
   extern int		Attn_Device();
   extern Object	ObjectNew();

/**************************************************************
*  First, create the hardware object to be controlled:  we    *
*  specify it to be an attenuator.  Then, set the attributes  *
*  for this attenuator:  we specify the attributes to be      *
*  equivalent to those for the other VXR500-style atten-      *
*  uators.  Finally, set the AP address characteristics for   *
*  this attenuator.                                           *
**************************************************************/

   SpclAttn = ObjectNew(Attn_Device,"Special Third Attenuator");
   (void) SetAttnAttr(SpclAttn,SET_DEFAULTS,SET_MAXVAL,OBS_ATTN_MAXVAL,
                         SET_MINVAL,OBS_ATTN_MINVAL,NULL);
   (void) SetAPAttr(SpclAttn,SET_APREG,8,NULL);
}
 

spclpower(powerptr,object)
codeint	powerptr;
Object	object;
{
    char msge[128];
    int error;
    Msg_Set_Param param;
    Msg_Set_Result result;

    /*  Object type call for an attenuator */
    param.setwhat = SET_VALUE;
    param.value = powerptr;
    result.genfifowrds = 0;
    error = Send(object,MSG_SET_ATTN_ATTR_pr,&param,&result);
    if (error < 0 )
    {
          sprintf(msge,"%s : %s\n",object->objname,ObjError(error));
          text_error(msge);
	  abort(1);
    }
    curfifocount += result.genfifowrds;/* words stuffed in acquisition acode */
    return;
}


double init3rdattn(pname,initptr,defaultval)	/* initialize 3rd attenuator */
char	pname[MAXSTR];
codeint	initptr;
int	defaultval;
{
   double	tmp;

   if (P_getreal(CURRENT, pname, &tmp, 1) < 0)
   {
      tmp = (double) (defaultval);
   }
   else
   {
      tmp = getval(pname);
   }
      
   initval(tmp, initptr);
   createspecial();
   spclpower(initptr, SpclAttn);

   return(tmp);
}
