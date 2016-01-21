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
#include "objerror.h"

static char buf[64];
static char cmdbuf[64];

/*------------------------------------------------------------------------
|
|       ObjError/1
|
|       Return a a pointer to the error message corresponding to the
|       error code passed.
+-----------------------------------------------------------------------*/
char *ObjError(int wcode)
{
   int             iter;

   iter = 0;
   while (objerrMsgTable[iter].code != 0)
   {
      if (objerrMsgTable[iter].code == wcode)
      {
	 if (objerrMsgTable[iter].errmsg == (char *) 0)
	 {
	    sprintf(&buf[0], "Error code = %d", wcode);
	    return ((char *) buf);
	 }
	 return (objerrMsgTable[iter].errmsg);
      }
      else
	 iter++;
   }

   sprintf(&buf[0], "Error code = %d", wcode);
   return ((char *) buf);
}

/*------------------------------------------------------------------------
|
|       ObjCmd/1
|
|       Return  a pointer to the message corresponding to the
|       object Attribute command code passed.
+-----------------------------------------------------------------------*/
char *ObjCmd(int wcode)
{
   int             iter;

   iter = 0;
   while (objCmdMsgTable[iter].code != -1)
   {
      if (objCmdMsgTable[iter].code == wcode)
      {
	 if (objCmdMsgTable[iter].errmsg == (char *) 0)
	 {
	    sprintf(&cmdbuf[0], "Command code = %d", wcode);
	    return ((char *) cmdbuf);
	 }
	 return (objCmdMsgTable[iter].errmsg);
      }
      else
	 iter++;
   }

   sprintf(&cmdbuf[0], "Command code = %d", wcode);
   return ((char *) cmdbuf);
}
