/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* recvproc.c - Recvproc Process */
/*
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>


#include <errno.h>

#include "hostMsgChannels.h"
#include "msgQLib.h"
#include "errLogLib.h"
#include "procQfuncs.h"
#include "eventHandler.h"
#include "commfuncs.h"
#include "shrstatinfo.h"

MSG_Q_ID pRecvMsgQ;

char ProcName[256];

main(argc,argv)
int argc;
char *argv[];
{
   int rtn;
   char MsgInbuf[RECV_MSG_SIZE];
   sigset_t   blockmask;
   MSG_Q_ID msgId;
   void processMsge(void*);

   strncpy(ProcName,argv[0],256);
   ProcName[255] = '\0';

   sigemptyset( &blockmask );
   sigaddset( &blockmask, SIGALRM );
   sigaddset( &blockmask, SIGIO );
   sigaddset( &blockmask, SIGCHLD );
   sigaddset( &blockmask, SIGQUIT );
   sigaddset( &blockmask, SIGPIPE );
   sigaddset( &blockmask, SIGALRM );
   sigaddset( &blockmask, SIGTERM );
   sigaddset( &blockmask, SIGUSR1 );
   sigaddset( &blockmask, SIGUSR2 );

   DebugLevel = 0;
   DPRINT1(1,"Starting: '%s'\n",ProcName);

   umask(000); /* clear file creation mode mask,so that open has control */

#ifndef DEBUG
   /* Make this program a daemon */
   makeItaDaemon(1);  /* make this program a daemon */
#endif

   /* Lets use the syslog facility provide by the OS. */
   logSysInit("Recvproc",RECVLOG_FACILITY);

   /* initialize exception handlers */
   setupexcepthandler();
 
   /* Map in shared Exp Status */
   initExpStatus(0); 

   /* Map in Active Exp Q */
   initActiveExpQ(0);

   /* initialize command parser */
   initCmdParser();

   /* Initialize the Event Handlers Queue */
   setupForAsync(20, &blockmask);

   /* setup key database, and Message Queue, facilities */
   pRecvMsgQ = createMsgQ("Recvproc", (int) RECVPROC_MSGQ_KEY, (int) RECV_MSG_SIZE);
   if (pRecvMsgQ == NULL)
      exit(1);

   setMsgQAsync(pRecvMsgQ,processMsge);

   initProcQs(1);   /* zero out processing Queue */

   /* Open channel to console, if initial connect fails then backgound
      the connection task untill connection is achieved */
   chanId = initiateChan(RECVPROC_CHANNEL); 

   asyncMainLoop();

   exit(0);
}

/**************************************************************
*
*  processMsge - Routine envoked to read message Q and call parser
*
*   This Function is the Non-interrupt function called to handle the
*   msgeQ interrupt as register in setupMsgSig() via registerAsyncHandlers()
*   (proccomm.c)
*
* RETURNS:
* void
*
*       Author Greg Brissey 9/6/94
*/
void processMsge(void *notin)
{
  char MsgInbuf[RECV_MSG_SIZE];
  int rtn;
 
 /* Keep reading the Msg Q until no further Messages */
  do {
       /* read msgQ don't block if none there */
       rtn = recvMsgQ(pRecvMsgQ, MsgInbuf, RECV_MSG_SIZE, NO_WAIT);
       /* if we got a message then go ahead and parse it */
       if (rtn > 0)
       {
         DPRINT2(1,"received %d bytes, MsgInbuf len %d bytes\n",rtn,strlen(MsgInbuf));
         parser(MsgInbuf);
         MsgInbuf[0] = '\0';
       }
     }
     while(rtn != -1);       /* if no message continue on */
         
  return;
}
