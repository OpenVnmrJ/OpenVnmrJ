/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCipcmsgqlibh
#define INCipcmsgqlibh


#define MSGQ_NORMAL ((long) 500)
#define MSGQ_URGENT ((long) 100)
#define NO_WAIT		0
#define WAIT_FOREVER	-1

/* Msg Q read/write permision for own & group & other */
#define MSGQ_PERMS (0666)

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* System V msgsnd/msgrcv cmd & message structure */

typedef struct _msgQ {
			int msgid;	  /* System V message queue id */
			int msgmax;	  /* maximum size of message */
			int msgqmax;	  /* system max bytes in queue */
			struct msgbuf *msgop; /* msgbuf for msgsnd & msgrcv */
        	     } IPC_MSG_Q_OBJ;

typedef IPC_MSG_Q_OBJ *IPC_MSG_Q_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)

#include <sys/types.h>
 
extern IPC_MSG_Q_ID  msgQCreate(key_t key, int maxMsgLength);
extern void msgQClose(IPC_MSG_Q_ID mid);
extern void msgQDelete(IPC_MSG_Q_ID mid);
extern int msgQSend(IPC_MSG_Q_ID msgId, char *buffer, int nBytes, int timeout, long priority);
extern int msgQReceive(IPC_MSG_Q_ID msgId, char* buffer, int maxNBytes, int timeout);
extern int  msgQNumMsgs(IPC_MSG_Q_ID msgId);
extern void  msgQShow(IPC_MSG_Q_ID msgId);
 
#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern IPC_MSG_Q_ID  msgQCreate();
extern void msgQClose();
extern void msgQDelete();
extern int msgQSend();
extern int msgQReceive();
extern int  msgQNumMsgs();
extern void  msgQShow();

#endif

 
#ifdef __cplusplus
}
#endif
 
#endif
