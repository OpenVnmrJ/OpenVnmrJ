/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

/* Skeleton of Async Message Q, technique */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "hostMsgChannels.h"
#include "msgQLib.h"
#include "errLogLib.h"
#include "eventHandler.h"
#include "termhandler.h"
#include "iofuncs.h"
#include "acquisition.h"

/************************************************************************
 * Declarations for routines that have no include file for us to use.
 ************************************************************************/
extern int makeItaDaemon(int ignoreSigChld);         /* daemon.c        */
extern int initCmdParser(void);                      /* parser.c        */
extern int parser(char *str);                        /* parser.c        */
extern int setupexcepthandler(void);                 /* excepthandler.c */
extern void deliverMessageQ(char *interface, char *msg);
extern void initMas();

#define DEBUG_LEVEL 0
#define LOGMSG

#define TRUE 1
#define FALSE 0
#define FOR_EVER 1

/*
 * Roboproc reads the following file to determine which SUN 
 * comm port to use for the sample changer.
 * From Vnmr the config program allows the selection.
 * The config macro actually writes out the "/vnmr/smsport" 
 * file to contain one of the SMS device names.
 * If the smsport file is not present or an unrecognized
 * value is read, roboproc defaults to "SMS_TTYA" (serial port A)
 */

#define MAXPATHL 256

MSG_Q_ID pRecvMsgQ;
char ProcName[256];
char HostName[80];
char systemdir[MAXPATHL];       /* vnmr system directory */

/* Pertinent device table entry and file descriptor */
ioDev *smsDevEntry = NULL;
int   smsDev = -1;

void shutdownComm(void)
{
   smsDevEntry->close(smsDev);
   smsDev = -1;
   smsDevEntry = NULL;

   deleteMsgQ(pRecvMsgQ);
}

#ifdef LINUX
void ignoreSIGIO()
{
    sigset_t		qmask;
    struct sigaction	segquit;

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGIO );
    segquit.sa_handler = SIG_IGN;
    segquit.sa_mask = qmask;
    sigaction(SIGIO,&segquit,0L);
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGPIPE );
    segquit.sa_handler = SIG_IGN;
    segquit.sa_mask = qmask;
    sigaction(SIGPIPE,&segquit,0L);
}
#endif

int main(int argc, char *argv[])
{
    char *tmpptr;
    sigset_t    blockmask;
    void processMsge(void*);

    strncpy(ProcName,argv[0],256);
    ProcName[255] = '\0';

    gethostname(HostName,sizeof(HostName));

#ifdef LOGMSG
    ErrLogOp = LOGIT;
#else
    ErrLogOp = NOLOG;
#endif

    DebugLevel = DEBUG_LEVEL;
    if ( ! access("/vnmr/acqbin/Maslog",F_OK) )
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

    INIT_ACQ_COMM( systemdir );         /* to interact with Expproc */

#ifndef DEBUG
#ifndef LINUX
    /* Make this program a daemon */
    makeItaDaemon(1);  /* make this program a daemon */
#endif
#endif

    /* Lets use the syslog facility provided by the OS. */
    logSysInit("Roboproc", LOG_LOCAL0);

    /* initialize exception handlers */
    setupexcepthandler();
#ifdef LINUX
    ignoreSIGIO();
#endif

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

    /* setup key database, and Message Queue, facilities */
    pRecvMsgQ = createMsgQ("Roboproc", (int) ROBOPROC_MSGQ_KEY,
                           (int) ROBO_MSG_SIZE);
    if (pRecvMsgQ == NULL)
       exit(1);

    setMsgQAsync(pRecvMsgQ,processMsge);

    initMas();

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
    char MsgInbuf[ROBO_MSG_SIZE];

    /* Keep reading the Msg Q until no further Messages */
    do {

        /* Read msgQ, Don't block if none there */
        rtn = recvMsgQ(pRecvMsgQ, MsgInbuf, ROBO_MSG_SIZE, NO_WAIT);

        /* If we got a message then go ahead and parse it */
        if (rtn > 0)
        {
            DPRINT2(2, "received %d bytes, MsgInbuf len %d bytes\n",
                    rtn, strlen(MsgInbuf));
            parser(MsgInbuf);
            MsgInbuf[0] = '\0';
        }

    } while (rtn != -1); /* if no message we're finished */

    return;
}

