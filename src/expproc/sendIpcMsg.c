/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <signal.h>
#include "msgQLib.h"
#include "ipcKeyDbm.h"
#include "errLogLib.h"
#include "hostMsgChannels.h"

/* 
*  record structure for process with msgQs that this process 
*  may wish to talk to.
*/
typedef struct _outmsg {
			  MSG_Q_ID procMsgQ;
			  char *procName;
			  int msgSize;
			} IPC_PROC_RECORD;

/*
*  Initialized data base for all process that msgQs
*  that this process may wish to talk to.
*  the record define can be found in hostMsgChannels.h
*  and their order is important.
*/
static IPC_PROC_RECORD msgQs[MSG_Q_DBM_SIZE] = { 
		EXP_RECORD,
		RECV_RECORD,
		SEND_RECORD,
		PROC_RECORD,
		ROBO_RECORD,
		AUTO_RECORD,
		IAUL_RECORD,
		SPUL_RECORD,
		NULL_RECORD,
		NULL_RECORD
           };

/**************************************************************
*
*  sendMsg - Send message via Message Q in dataBase.
*
* 1. Obtain process msgQ info from the IPC Key Database
* 2. Check that the request process exists & running
* 3. If messageQ has not been open then open a messageQ 
*    based on the ipc key to process
* 4. Send Message
* 5. Send Signal to process indicating message sent.
*  
* RETURNS:
* 0 , -1 on error. 
*
*       Author Greg Brissey 8/31/94
*/
sendMsg(IPC_KEY_DBM_ID keyId, int process,char *message, int priority, int waitflg)
{
   IPC_KEY_DBM_DATA msgeEntry;
   MSG_Q_ID msgId;
   int len,stat;

   /* using process name get ipcKey entry */
   if (stat = ipcKeyGet(keyId, msgQs[process].procName, &msgeEntry) == -1)  
   {
      errLogRet(ErrLogOp,debugInfo,
	"ipcSendMsg: '%s' is not contained in the IPC database\n",
	msgQs[process].procName);
      return(-1);
   }

   /* if process not present give error */
   if (msgeEntry.pidActive != 1)
   {
      errLogRet(ErrLogOp,debugInfo,
	"ipcSendMsg: '%s' is not running\n",msgQs[process].procName);
      return(-1);
   }

   /* if msgQ has not already been created, make one  
   * the assumption here is that the ipc key will not change 
   * over different executions of the process. This is true
   * at the present time.
   */
   if (msgQs[process].procMsgQ == NULL)
   {
      /* create message queue with returned key_t key */
      msgId = msgQCreate(msgeEntry.ipcKey, msgQs[process].msgSize);
      if (msgId == NULL)
      {
        errLogRet(ErrLogOp,debugInfo,"sendMsg: msgQCreate() failed.\n");
	return(-1);
      }
      msgQs[process].procMsgQ = msgId;
   }

   /* Put the message into the Queue */
   len = strlen(message);
   if (msgQSend(msgQs[process].procMsgQ, message, len, waitflg, priority) == -1)
   {
	return(-1);
   }
   /* send the async signal to process to inform it a message has been sent */
   if (kill(msgeEntry.pid,SIGUSR1) == -1)
   {
      errLogSysRet(ErrLogOp,debugInfo,
	"sendMsg: msgQ async Signal could not be sent to; '%s',pid: %d",
		msgQs[process].procName, msgeEntry.pid);
      return(-1);
   }
   return(0);
}
/**************************************************************
*
*  sendMsgByName - Send message via Message Q in dataBase.
*
* 1. Obtain process msgQ info from the IPC Key Database
* 2. Check that the request process exists & running
* 3. If messageQ has not been open then open a messageQ 
*    based on the ipc key to process
* 4. Send Message
* 5. Send Signal to process indicating message sent.
*  
* RETURNS:
* 0 , -1 on error. 
*
*       Author Greg Brissey 8/31/94
*/
sendMsgByName(IPC_KEY_DBM_ID keyId, char *procname,char *message, int priority, int waitflg)
{
   IPC_KEY_DBM_DATA msgeEntry;
   MSG_Q_ID msgId;
   int len,stat,i,index;

   /* using process name get ipcKey entry */
   if (stat = ipcKeyGet(keyId, procname, &msgeEntry) == -1)  
   {
      errLogRet(ErrLogOp,debugInfo,
	"sendMsgByName: '%s' is not contained in the IPC database\n",
	   procname);
      return(-1);
   }

   /* if process not present give error */
   if (msgeEntry.pidActive != 1)
   {
      errLogRet(ErrLogOp,debugInfo,
	"sendMsgByName: '%s' is not running\n",procname);
      return(-1);
   }

   /*
   * if msgQ has not already been created, make one  
   * the assumption here is that the ipc key will not change 
   * over different executions of the process. This is true
   * at the present time.
   */
   index = -1;
   for(i=0; i < MSG_Q_DBM_SIZE; i++)
   {
      if (strcmp(procname,msgQs[i].procName) == 0)
      {
	 index = i;
      }
   }
   if (index == -1)
   {
      errLogRet(ErrLogOp,debugInfo,
	"sendMsgByName: %s not found in  msgQs.\n",procname);
      return(-1);
   }

   if (msgQs[index].procMsgQ == NULL)
   {
      /* create message queue with returned key_t key */
      msgId = msgQCreate(msgeEntry.ipcKey, msgQs[index].msgSize);
      if (msgId == NULL)
      {
        errLogRet(ErrLogOp,debugInfo,"sendMsg: msgQCreate() failed.\n");
	return(-1);
      }
      msgQs[index].procMsgQ = msgId;
   }

   /* Put the message into the Queue */
   len = strlen(message);
   if (msgQSend(msgQs[index].procMsgQ, message, len, waitflg, priority) == -1)
   {
	return(-1);
   }
   /* send the async signal to process to inform it a message has been sent */
   if (kill(msgeEntry.pid,SIGUSR1) == -1)
   {
      errLogSysRet(ErrLogOp,debugInfo,
	"sendMsg: msgQ async Signal could not be sent to; '%s',pid: %d",
		msgQs[index].procName, msgeEntry.pid);
      return(-1);
   }
   return(0);
}
