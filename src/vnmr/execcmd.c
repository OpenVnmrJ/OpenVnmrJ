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
#include "vnmrsys.h"
#include "allocate.h"
#include "tools.h"
#include "wjunk.h"

extern int expdir_to_expnum(char *expdir);
extern int showFlushDisp;

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
         Werrprintf ( "Usage:  exec(cmd)" );
	 ABORT;
      }
   }
}

int execexp(int argc, char *argv[], int retc, char *retv[] )
{
   int ival;
   char *cmd;

   if ( argc == 3 )
   {
      int newExp;
      int oldExp;
      char cmdLine[MAXPATH * 3];
      int tmp;

      tmp=showFlushDisp;
      showFlushDisp = 0;

/*  Use the Dont Touch version of newCat
    so argv[ iter ] is not freed prematurely

    Use newCat because the execString
    parser requires a new-line character
    to terminate its input.			*/
      newExp= atoi(argv[1]);
      oldExp = expdir_to_expnum(curexpdir);
      sprintf(cmdLine,"jexp(%d) %s jexp(%d)",newExp, argv[2], oldExp);

      cmd = newCatIdDontTouch( cmdLine, "\n", "exec" );
      ival = execString( cmd );
      release( cmd );
      showFlushDisp = tmp;
      if (ival != 0)
      {
         if (retc)
         {
            retv[0] = intString( 0 );
            RETURN;
         }
         else
         {
            Werrprintf ( "execexp: '%s' aborted", argv[2] );
            ABORT;
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
         Werrprintf ( "Usage:  execexp(expNumber,cmd)" );
	 ABORT;
      }
   }
}
