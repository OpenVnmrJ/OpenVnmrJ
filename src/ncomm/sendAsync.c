/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
modification history
--------------------
10-6-94,gmb  created 
*/


/*
DESCRIPTION

*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "errLogLib.h"
#include "sockets.h"

/**************************************************************
*
*  sendAsync - Send a Message to a Vnmr style Async Message Socket
*
* RETURNS:
* 1 on success or -1 on failure 
*
*	Author Greg Brissey 10/6/94
*/
int sendAsync(char *machine,int port,char *message)
{
 Socket *SockId;
 int i,result;

  if ( (SockId = createSocket( SOCK_STREAM )) == NULL) 
  {
      return( -1 );
  }

 /* --- try several times then fail --- */
  for (i=0; i < 5; i++)
  {
     result = openSocket(SockId); /* open * setup */
     if (result != 0)
     {
         /* perror("sendacq(): socket"); */
         errLogSysRet(ErrLogOp,debugInfo,"sendAsync(): Error, could not create socket\n");
         return(-1);
     }

     if ((result = connectSocket(SockId, machine, port)) != 0)
     {
          /* errLogSysRet(ErrLogOp,debugInfo,"connect: "); */
          /* --- Is Socket queue full ? --- */
          if (errno != ECONNREFUSED && errno != ETIMEDOUT)
          {                             /* NO, some other error */
              errLogSysRet(ErrLogOp,debugInfo,"sendAsync():aborting,  connect error");
              return(-1);
          }
     }
     else     /* connection established */
     {
        break;
     }
     errLogRet(ErrLogOp,debugInfo,"sendAsync(): Socket queue full, will retry %d\n",i);
     closeSocket(SockId);
     sleep(5);
  }
  if (result != 0)    /* tried MAXRETRY without success  */
  {
      errLogRet(ErrLogOp,debugInfo,"sendAsync(): Max trys Exceeded, aborting send\n");
      return(-1);
  }

    writeSocket(SockId,message,strlen(message));
    closeSocket(SockId);
    return(1);
}


