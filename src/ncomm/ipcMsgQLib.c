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
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>

#ifdef LINUX
#define  __USE_GNU
#endif

#include <sys/msg.h>
#if (defined(__INTERIX) || defined(__CYGWIN__) || defined(MACOS))
struct msgbuf
{
  long int mtype;
  char mtext[1];
};

#endif

#include <errno.h>

#include "errLogLib.h"
#include "ipcMsgQLib.h"

/*
modification history
--------------------
6-27-94,gmb  created
*/


/*
DESCRIPTION
This library provide a VxWorks style interface for the 
System V message queue routines. 

Interface provided:

   IPC_MSG_Q_ID msgId = msgQCreate(key_t MSGKEY, int maxMsgLength)
   msgQSend(msgId,char *buffer, int nBytes,int timeout, int priorty)
   msgQReceive(msgId,char *buffer, int maxBytes, int timeout)
   msgQDelete(msgId)
   msgQClose(msgId)

System V places several contraints on Messages Queues at the kernel
level these are:  (SUNOS 4.1.3)
	msgmni - maximum # of message queues, systemwide ~50
	msgmax - maximum # of bytes per message  ~8192
	msgmnb - maximum # of bytes on any one msge queue ~2048
	msgtql - maximum # of messages systemwide ~50
	
These limits can be raised by rebuilding the kernel.
	
    Note: At present the acutal timeout feature has not been
          implemented. This maybe a future enhancement 
	  if required.
*/

/* for SunOS */
#ifndef MSG_W 
#define MSG_W 0200
#endif
#ifndef MSG_R
#define MSG_R 0400
#endif

#define MSG_OWNR_PERM (MSG_W | MSG_R)
#define MSG_GRP_PERM  ((MSG_W >> 3) | (MSG_R >> 3))
#define MSG_WRLD_PERM ((MSG_W >> 6) | (MSG_R >> 6))
#define MSG_ALL_PERM  (MSG_OWNR_PERM | MSG_GRP_PERM | MSG_WRLD_PERM)
 
/**************************************************************
*
*  msgQCreate - Create and initialize Message Queue.
*
* This routine creates a System V Message Queue. This message
* queue has priority ordering. The maximum number of messages
* and message size is limited by the kernel. So keepem short.
* If it exits, we don't initialize it.
*  
*   Note: maxMsgLength is a user maximum not system max. Since
* default maximum total bytes on a queue is 2048. The bigger
* the messages the few of them you can queue up. I.E.
* maxMsgLength = 1024 only gives you 2 messages in the worst
* case. The lesson is message Qs are not for big messages.
*
* RETURNS:
* Message Queue ID, else NULL 
*
*       Author Greg Brissey 6/24/94
*/
IPC_MSG_Q_ID  msgQCreate(key_t key, int maxMsgLength )
/* key_t key - key to semaphore */
/* int maxMsgLength - max message length user will send/receive */
{
   register int id;
   struct msqid_ds msgQStats;
   IPC_MSG_Q_ID msgqid;

   if ( (key == IPC_PRIVATE) || (key == (key_t) -1) )
   {
     errLogRet(ErrLogOp,debugInfo,"msgQCreate: Invalid Key given. %d\n",key);
     return(NULL); /* not for private sems or invalid key */
   }

   if ( (id = msgget(key, (MSG_ALL_PERM | IPC_CREAT))) < 0)
   {
      errLogSysRet(ErrLogOp,debugInfo,"msgQCreate: msgget(perm=0%o)error",
		(MSG_ALL_PERM | IPC_CREAT));
      return(NULL);  /* permission problem or kernel table full */
   }

   DPRINT1(1,"msgQCreate: created msgId = %d\n",id);

   if ( msgctl(id, IPC_STAT, &msgQStats) < 0 )
   {
      errLogSysRet(ErrLogOp,debugInfo,"msgQCreate: msgid=%d, msgctl IPC_STAT error",
	id);
      return(NULL);  /* permission problem or kernel table full */
   }

   /* malloc space for IPC_MSG_Q_ID struct and message size */
   if ( (msgqid = (IPC_MSG_Q_ID) (malloc(sizeof(IPC_MSG_Q_OBJ)))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"msgQCreate: IPC_MSG_Q_OBJ malloc error");
      msgctl(id, IPC_RMID, (struct msqid_ds *) 0);
   }

   if ( (msgqid->msgop = (struct msgbuf *) 
		(malloc(sizeof(struct msgbuf) + maxMsgLength + 1))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"msgQCreate: msgbuf malloc error");
      msgctl(id, IPC_RMID, (struct msqid_ds *) 0);
   }
   msgqid->msgid = id;
   msgqid->msgmax = maxMsgLength;

   /* maximum bytes allowed in message queue by the OS */
   msgqid->msgqmax = msgQStats.msg_qbytes;

   if (DebugLevel >= 2) 
      msgQShow(msgqid);

   return(msgqid);
}

/**************************************************************
*
*  msgQClose - Close a MsgQ.
*
* This routine logically closes a System V Message Queue that exist. 
* And frees memory resources allocated.
* It does not remove the Queue from the OS
*
*
* RETURNS:
* VOID 
*
*       Author Greg Brissey 10/13/94
*/
void msgQClose(IPC_MSG_Q_ID mid)
/* IPC_MSG_Q_ID mid - Message Q ID */
{
   if (mid != NULL)
   {
     free((char*)mid->msgop);
     free((char*)mid);
   }
   return;
}


/**************************************************************
*
*  msgQDelete - Removes the System V Message Queue.
*
* This routine removes a System V Message Queue that exist. 
* Any other process pending on this Msg Q will be Unblocked.  
*  Note: System V message Queues are persistent across 
* process termination. Thus to remove the message queue
* from the OS system one must call this routine.
*
* RETURNS:
* VOID 
*
*       Author Greg Brissey 6/24/94
*/
void msgQDelete(IPC_MSG_Q_ID mid)
/* IPC_MSG_Q_ID mid - Message Q ID */
{

   if (mid != NULL)
   {
     if(msgctl(mid->msgid, IPC_RMID, (struct msqid_ds *) 0) < 0 )
        errLogSysRet(ErrLogOp,debugInfo,"msgQDelete: msgid=%d, can't remove msgQ",
	mid->msgid);
     free((char*)mid->msgop);
     free((char*)mid);
   }
   return;
}


/**************************************************************
*
*  msgQSend - send a message to a message queue.
*
* This routine sends a message to a System V Message Queue.
* Valid values for timeout are NO_WAIT or WAIT_FOREVER.
* If there is no room for the message on the queue the
* process will block unless NO_WAIT has been specified.
* Then msgQSend will return with errno of EAGAIN.
*   Note: timeout is not implemented yet. 
*
* 
* RETURNS:
* 0 - on success, -1 on Error
*
*       Author Greg Brissey 6/24/94
*/
int msgQSend(IPC_MSG_Q_ID msgId, char *buffer, int nBytes, int timeout, long priority)
/* IPC_MSG_Q_ID msgId - message queue id */
/* char *buffer - message to send */
/* int nBytes - length of message */
/* int timeout - NO_WAIT, WAIT_FOREVER, or seconds to timeout */
/* long priority - MSGQ_NORMAL, MSGQ_URGENT */
{
	int flags = 0;

	if (timeout == NO_WAIT)
	{
	  flags = IPC_NOWAIT;
	}
	else if (timeout == WAIT_FOREVER)
	{
	   flags = 0;
	}

	if ( nBytes > msgId->msgmax )
	{
         errLogRet(ErrLogOp,debugInfo,
	  "msgQSend: The requested %d bytes to send is larger than message queue size of %d\n",
		 	nBytes,msgId->msgmax);
	 return(-1);
	}

	msgId->msgop->mtype = priority;
        memcpy(msgId->msgop->mtext,buffer,nBytes);
	if ( msgsnd(msgId->msgid, msgId->msgop, nBytes, flags) < 0 )
	{
	   if ( (flags == IPC_NOWAIT) && ( errno == EAGAIN ) )
           {
		return(-1);
           }
	   else
           {
             errLogSysRet(ErrLogOp,debugInfo,
               "msgQSend: msgid = %d, msgsnd() error in sending message",msgId->msgid);
	     return(-1);
	   }
	   
	}
    return(0);
}



/**************************************************************
*
*  msgQReceive - receive a message from a message queue
*
* This routine receives a message from a message queue.
* The only valid value of timeout are NO_WAIT or WAIT_FOREVER.
*  If the message receive is larger than maxNBytes the message
* will be truncated to size maxNBytes. However the returned
* size will be of the actual message.
*  If NO_WAIT is specified and no message is present 
* msgQReceive returns with errno of ENOMSG. 
*
*   Note: timeout is not implemented yet. 
*
* RETURNS:
*  Number of Bytes Receive, or -1 for error
*
*       Author Greg Brissey 6/24/94
*/
int msgQReceive(IPC_MSG_Q_ID msgId, char* buffer, int maxNBytes, int timeout)
/* IPC_MSG_Q_ID msgId - message queue id */
/* char *buffer - buffer to receive message */
/* int maxNBytes - length of buffer */
/* int timeout - NO_WAIT, WAIT_FOREVER, or seconds to wait */
{
    int bytesRcv;
	int flags = 0;

	if (timeout == NO_WAIT)
	{
	  flags = IPC_NOWAIT | MSG_NOERROR;
	}
	else if (timeout == WAIT_FOREVER)
	{
	   flags = MSG_NOERROR;
	}

	if ( maxNBytes > msgId->msgmax )
	{
         errLogRet(ErrLogOp,debugInfo,
	     "The requested %d bytes is larger than message queue size of %d\n",
		 	maxNBytes,msgId->msgmax);
	}

	msgId->msgop->mtype = -MSGQ_NORMAL;
	if ( (bytesRcv = msgrcv(msgId->msgid, msgId->msgop, maxNBytes, 
				            -MSGQ_NORMAL, flags)) < 0 )
        {
	   if ( (timeout == NO_WAIT) && ( errno == ENOMSG) )
	   {
		return(-1);
    	   }
	   else
	   {
              errLogSysRet(ErrLogOp,debugInfo,
	        "msgQReceive: msgid=%d, msgrcv() error in receiving message", 
		  msgId->msgid);
	      return(-1);
    	   }
        }
	if ( bytesRcv <= maxNBytes )
        {
          memcpy(buffer,msgId->msgop->mtext,bytesRcv);
          buffer[bytesRcv] = '\0';
	}
	else
	{
	  /* WARNING maxNBytes exceeded truncations */
            errLogRet(ErrLogOp,debugInfo,
		"msgQReceive: WARNING bytes received %d exceeded max. %d; Msge truncated\n",bytesRcv,maxNBytes); 
          memcpy(buffer,msgId->msgop->mtext,maxNBytes);
          buffer[maxNBytes] = '\0';
	}
    return(bytesRcv);
}
/**************************************************************
*
*  msgQNumMsgs - get the number of messages queued in a message queue
*
* This routine returns the number of messages currently queued in a 
* specified message queue.
*  
*
* RETURNS:
* number of messages queued, or ERROR -1 
*
*       Author Greg Brissey 6/24/94
*/
int  msgQNumMsgs(IPC_MSG_Q_ID msgId)
/* IPC_MSG_Q_ID msgId - Message Queue Id */
{
   struct msqid_ds msgQStats;

   if ( msgctl(msgId->msgid, IPC_STAT, &msgQStats) < 0 )
   {
      errLogSysRet(ErrLogOp,debugInfo,
	"msgQNumMsgs: msgid=%d, msgctl() can't obtain msg Q stats",msgId->msgid);
      return(-1);  /* permission problem or kernel table full */
   }

   return(msgQStats.msg_qnum);
}
/**************************************************************
*
*  msgQShow - show information about message queue
*
* This routine display the system state information of 
* the message queue.
*
* RETURNS:
* void
*
*       Author Greg Brissey 6/24/94
*/
void  msgQShow(IPC_MSG_Q_ID msgId)
/* IPC_MSG_Q_ID msgId - Message Queue Id */
{
   struct msqid_ds msgQStats;

   if ( msgctl(msgId->msgid, IPC_STAT, &msgQStats) < 0 )
   {
      perror("msgQNumMsgs: can't obtain msg Q stats, msgctl");
      return;  /* permission problem or kernel table full */
   }

#if ( !defined(LINUX) && !defined(__INTERIX) )
   DPRINT3(3,"\n\nMsg Q Obj: 0x%lx, Msg Id: %d   Key: 0x%lx\n",
	   msgId,msgId->msgid,msgQStats.msg_perm.key);
#endif
   DPRINT3(3,"%d Queued, %u Bytes in Q, %u Max Allowed Bytes\n",
	   msgQStats.msg_qnum, msgQStats.msg_cbytes, msgQStats.msg_qbytes);
   DPRINT2(3,"Proc pid last send: %d, Proc pid last receive: %d\n",
	   msgQStats.msg_lspid, msgQStats.msg_lrpid);
   DPRINT1(3,"Last Send: %s", ctime(&(msgQStats.msg_stime)));
   DPRINT1(3,"Last Receive: %s", ctime(&(msgQStats.msg_rtime)));
   DPRINT(3,"Msg Q Permissions: \n");
   DPRINT2(3,"                   Owner's uid: %d, Group gid: %d\n",
	msgQStats.msg_perm.uid, msgQStats.msg_perm.gid);
   DPRINT2(3,"                   Creator's uid: %d, Group gid: %d\n",
	msgQStats.msg_perm.cuid, msgQStats.msg_perm.cgid);
#if ( !defined(LINUX) && !defined(__INTERIX) )
   DPRINT2(3,"                   Mode: %d, Slot Usage Sequence #: %d\n",
	msgQStats.msg_perm.mode, msgQStats.msg_perm.seq);
#endif

   return;
}

