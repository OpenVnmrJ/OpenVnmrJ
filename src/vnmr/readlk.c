/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------------------
|	readlk.c
|
|  close the statusSocket before exiting.		08/31/88  RL 
|
|  abort gracefully if interactive acquisition in progress  11/15/88  RL
|
|  always get current Acqproc parameters		06/22/89  RL
|
+---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "vnmrsys.h"
#include "tools.h"
#include "acquisition.h"
#include "wjunk.h"

/*-----------------------------------------------------------------------
|
|       readlk
|
|	Obtain current lock level, as reported by AcqProc
|
|       This command works on the display terminals as well as
|       the SUN console windows.  Checked 11/11/88,  RL
|
+-----------------------------------------------------------------------*/

int readlk(int argc, char *argv[], int retc, char *retv[])
{
    int                 ival;
    double              lockLevel;

/*  Check on interactive acquisition...  */

    if (interact_is_connected("")) {
        Werrprintf( "Cannot run %s, interactive acquisition in progress",
		   argv[ 0 ] );
        ABORT;
    }

    if (ACQOK(HostName) == 0) {
	disp_acq("");
        Werrprintf("Acquisition is not available!");
	ABORT; 
    }

    if (GETACQSTATUS(HostName,UserName) < 0)
    {
        Werrprintf( "%s:  failed to obtain acquisition status", argv[ 0 ] );
        ABORT;
    }

    ival = getAcqStatusValue(LOCKLEVEL, &lockLevel);

    if (retc == 0)
      Winfoprintf( "readlk:  current lock level = %g", lockLevel );
    else if (retc > 0)
    {
      retv[ 0 ] = realString( lockLevel );
      if (retc > 1)
      {
         ival = getAcqStatusValue(LOCKLEVEL+1, &lockLevel);
         retv[ 1 ] = realString( lockLevel );
      }
    }

    RETURN;
}
