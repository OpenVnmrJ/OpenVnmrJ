/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* Skeletion of Async Message Q, technique */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>


#include <errno.h>

#include "hostMsgChannels.h"
#include "ipcKeyDbm.h"
#include "msgQLib.h"
#include "errLogLib.h"

#define TRUE 1
#define FALSE 0
#define FOR_EVER 1

static int sigioFlag = 0;

static MSG_Q_ID msgId;
static IPC_KEY_DBM_ID keyId;
static IPC_KEY_DBM_DATA roboEntry;
static IPC_KEY_DBM_DATA expEntry;

MSG_Q_ID setupMsgQ(int keyindex,int msgSize)
{
   static void setupMsgSig();

   /* Create/Open  MSG Q Database */
   keyId = ipcKeyDbmCreate(MSG_Q_DBM_PATH, MSG_Q_DBM_SIZE);

   /* Update present Process Key,pid,etc. in database   */
   ipcKeySet(keyId, "Roboproc", getpid(), keyindex, NULL);

   ipcKeyGet(keyId, "Roboproc",&roboEntry);  

   /* create message queue with returned key_t key */
   msgId = msgQCreate(roboEntry.ipcKey, msgSize);

   /* 
      Setup the SIGUSR1 signal handler that is used to signal
      that there is a message in the Message Q.
   */
   setupMsgSig();

#ifdef DEBUG
   ipcKeyDbmShow(keyId);     /* display entries    */
   msgQShow(msgId);
#endif

   return(msgId);
}

/* SIGUSR1, to signal  Message Queue I/O*/
static void
setupMsgSig()
{
    struct sigaction    intserv;
    sigset_t            qmask;
    static void         msgQitrp();

    /* --- set up signal handler --- */

    sigemptyset( &qmask );
    /* Block all these signal after SIGUSR1 recieved */
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGCHLD );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGUSR1 );
    sigaddset( &qmask, SIGUSR2 );
    intserv.sa_handler = msgQitrp;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;
 
    sigaction( SIGUSR1, &intserv, NULL );

    return;
}

static void
msgQitrp()
{
    sigioFlag = TRUE;  /* set this flag */
		       /* However in this example we don't even use it */
    setupMsgSig();
    return;
}
