/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------------+
|                                                                             |
|       tempstuff.c                                                           |
|                                                                             |
|       This file contains code for dealing with temp IDs.  The parser        |
|       requests allocation of memory utilizing unique temporary IDs.         |
|       Each invocation of the parser must use it's own to ensure proper      |
|       nesting and release.                                                  |
|                                                                             |
+----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *tempID = NULL;			/* Global pointer for temp IDs       */

/*----------------------------------------------------------------------------+
|                                                                             |
|       newTempName/1                                                         |
|                                                                             |
|       This function returns a string consisting of a given prefix string    |
|       followed by a number.  Each call will result in a new number being    |
|       issued.  The returned string resides in malloc'd storage, and         |
|       should be free'd when no longer needed.                               |
|                                                                             |
+----------------------------------------------------------------------------*/

char *newTempName(prefix)		char *prefix;
{  char       *s;
   char        buffer[32];
   static int  count = 0;

   count += 1;
   sprintf(buffer,"%s%d",prefix,count);
   if ( (s=(char *)malloc(strlen(buffer)+1)) )
   {  strcpy(s,buffer);
      return(s);
   }
   else
   {  fprintf(stderr,"out of memory\n");
      abort();
   }
}
