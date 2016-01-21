/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "vnmrsys.h"
#include "allocate.h"
#include "tools.h"
#include "wjunk.h"

int exec(int argc, char *argv[], int retc, char *retv[] )
{
   int iter, ival;
   char *cmd;

   if ( argc > 1 )
   {
      for (iter = 1; iter < argc; iter++) {

/*  Use the Dont Touch version of newCat
    so argv[ iter ] is not freed prematurely

    Use newCat because the execString
    parser requires a new-line character
    to terminate its input.			*/

         cmd = newCatIdDontTouch( argv[ iter ], "\n", "exec" );
         ival = execString( cmd );
         release( cmd );
         if (ival != 0)
         {
            if (retc)
            {
               retv[0] = intString( 0 );
               RETURN;
            }
            else
            {
               Werrprintf ( "exec: '%s' aborted", argv[iter] );
               ABORT;
            }
         }
      }
      if (retc)
      {
	 retv[0] = intString( 1 );
      }
      RETURN;
   }
   else
   {
      if (retc)
      {
	 retv[0] = intString( 0 );
         RETURN;
      }
      else
      {
         Werrprintf ( "Usage:  exec(arg1,arg2,...)" );
	 ABORT;
      }
   }
}
