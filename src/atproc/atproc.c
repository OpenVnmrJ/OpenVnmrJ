/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* Skeleton of Async Message Q, technique */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "msgQLib.h"
#include "errLogLib.h"
#include "hostMsgChannels.h"
#include "eventHandler.h"

/************************************************************************
 * Declarations for routines that have no include file for us to use.
 ************************************************************************/
extern int makeItaDaemon(int ignoreSigChld);         /* daemon.c        */
extern int initCmdParser(void);                      /* parser.c        */
extern int parser(char *str);                        /* parser.c        */
extern int setupexcepthandler(void);                 /* excepthandler.c */
extern void atcmd2(int sig);

#define DEBUG_LEVEL 0
#define LOGMSG

#define TRUE 1
#define FALSE 0
#define FOR_EVER 1

/*
 * Atproc reads the following file to determine which atcmd's
 * to perform.
 */

#define MAXPATHL 256
char atCmdPath[MAXPATHL];

MSG_Q_ID pAtMsgQ;
char ProcName[256];
char systemdir[MAXPATHL];       /* vnmr system directory */

void shutdownComm()
{
   deleteMsgQ(pAtMsgQ);
}

int main(int argc, char *argv[])
{
    char *tmpptr;
    sigset_t blockmask;
    void processMsge(void*);

    strncpy(ProcName,argv[0],256);
    ProcName[255] = '\0';

#ifdef LOGMSG
    ErrLogOp = LOGIT;
#else
    ErrLogOp = NOLOG;
#endif

    DebugLevel = DEBUG_LEVEL;
    if ( ! access("/vnmr/acqbin/Atlog",F_OK) )
    {
       DebugLevel = 1;
    }
    DPRINT1(1,"Starting: '%s'\n", ProcName);

    /* clear file creation mode mask,so that open has control */
    umask(000);

    /* initialize environment parameter vnmrsystem value */
    tmpptr = getenv("vnmrsystem");      /* vnmrsystem */
    if (tmpptr != (char *) 0) {
        strcpy(systemdir,tmpptr);       /* copy value into global */
    }
    else {   
        strcpy(systemdir,"/vnmr");      /* use /vnmr as default value */
    }

    strcpy(atCmdPath,systemdir);      /* typically /vnmr/smsport */
    strcat(atCmdPath,"/acqqueue/atcmd");
    DPRINT1(1,"atCmdPath: '%s'\n",atCmdPath);


#ifdef XXX
#ifndef DEBUG
    /* Make this program a daemon */
    makeItaDaemon(1);  /* make this program a daemon */
#endif
#endif

    /* Lets use the syslog facility provided by the OS. */
    logSysInit("Atproc", LOG_LOCAL0);

    /* initialize exception handlers */
    setupexcepthandler();

    /* initialize command parser */
    initCmdParser();

    /* Initialize the Event Handlers Queue */
    /* Block SIGALRM, SIGIO and SIGCHLD interrupts during Event handling */
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
    setupForAsync(20, &blockmask);

    DPRINT(1,"setup message queue\n");
    /* setup key database, and Message Queue, facilities */
    pAtMsgQ = createMsgQ("Atproc", (int) ATPROC_MSGQ_KEY,
                           (int) AT_MSG_SIZE);
    if (pAtMsgQ == NULL)
       exit(1);

    setMsgQAsync(pAtMsgQ,processMsge);

    DPRINT(1,"call atcmd\n");
    atcmd2(0);
    asyncMainLoop();

    /* Orderly shutdown of the comm port */
    shutdownComm();

    exit(0);
}

/************************************************************************
 *
 *  processMsge - Routine envoked to read message Q and call parser
 *
 *   This Function is the Non-interrupt function called to handle the
 *   msgeQ interrupt as registered in setupMsgSig() via
 *   registerAsyncHandlers().
 *
 * RETURNS:
 *   void 
 *
 * Author Greg Brissey 9/6/94
 *
 ************************************************************************/
void processMsge(void *notin)
{
    int rtn;
    char MsgInbuf[AT_MSG_SIZE];

    /* Keep reading the Msg Q until no further Messages */
    do {

        /* Read msgQ, Don't block if none there */
        rtn = recvMsgQ(pAtMsgQ, MsgInbuf, AT_MSG_SIZE, NO_WAIT);

        /* If we got a message then go ahead and parse it */
        if (rtn > 0)
        {
            DPRINT2(2, "received %d bytes, MsgInbuf len %zu bytes\n",
                    rtn, strlen(MsgInbuf));
            parser(MsgInbuf);
            MsgInbuf[0] = '\0';
        }

    } while (rtn != -1); /* if no message we're finished */

    return;
}

