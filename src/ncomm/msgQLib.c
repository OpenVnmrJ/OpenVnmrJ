/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef VNMRS_WIN32   /* The Native Windows Compilation Define */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

#include <errno.h>

#include "errLogLib.h"
#include "hostMsgChannels.h"

#ifndef NIRVANA
#include "chanLib.h"
#include "eventHandler.h"
#endif

#include "mfileObj.h"

#else  /* VNMRS_WIN32 */

#include "Windows.h"
#include "errLogLib.h"

#endif  /* VNMRS_WIN32 */

#include "msgQLib.h"

/*
modification history
--------------------
3-11-98  gmb major changes , no longer ipcDbm, only the proc family uses the
	 msgQs now, Vnmr,PSG, etc uses sockets. Thus only a same share memory
	 structure is now shared.

10-12-94,gmb  created
*/

#ifndef VNMRS_WIN32

/*
DESCRIPTION
This library provide a Message Queue Object Library 

Interface provided:

   MSG_Q_ID pMsgQ = createMsgQ()
   MSG_Q_ID pMsgQ = openMsgQ()
   sendMsgQ(pMsgQ,char *buffer, int nBytes,int timeout, int priorty)
   recvMsgQ(pMsgQ,char *buffer, int maxBytes, int timeout)
   closeMsgQ(pMsgQ)
   deleteMsgQ(pMsgQ)

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

/*
*  Initialized data base for all msgQ process that msgQs
*  that this process may wish to talk to.
*  the record define can be found in hostMsgChannels.h
*  and their order is important.
*/
static struct { char *procName;
		key_t ipcKey;
		pid_t pid;
		int maxSize;
	       } procList[PROCIPC_DB_SIZE] = {
                EXPPROC_IPC_DB,
                RECVPROC_IPC_DB,
                SENDPROC_IPC_DB,
                PROCPROC_IPC_DB,
                ROBOPROC_IPC_DB,
                AUTOPROC_IPC_DB,
                ATPROC_IPC_DB,
                SPULPROC_IPC_DB,
                PROC_NULL_RECORD,
           };

#ifndef NIRVANA
static void msgQitrp(int signal);
#endif

static int findProcIndex(char *name)
{
   int i;
    for(i=0; i < PROCIPC_DB_SIZE; i++)
    {
	if (strcmp(name,procList[i].procName) == 0)
	{
	   return(i);
	}
    }
    return(-1);
}
 
/**************************************************************
*
*  createMsgQ - Initialize an Message Q to Receive Messages
*
* This routine initializes a message queue and DataBase
* on the given String ID and IPC Key seed. The calling process 
* receives ownership of the message queue created.
*  
* RETURNS:
* MSG_Q_ID , or NULL on error. 
*                    malloc failure
*                    msgQ is already owned by active process
*
*       Author Greg Brissey 10/12/94
*/
MSG_Q_ID createMsgQ(char *myProcName, int keyindex, int msgSize)
/* myProcName - unique id string for Message Queue */
/* keyindex - ipc key seed */
/* msgSize - Maximum Size of message in Bytes */
{
   MSG_Q_ID pMsgQ;
   MSG_Q_DBM_ID pMsgInfo;
   int result,index,nMsges;
   char buffer[8192];

   (void) keyindex;
   /* malloc space for IPC_KEY_DBM_ID object */
   if ( (pMsgQ = (MSG_Q_ID) (malloc(sizeof(MSG_Q_OBJ)))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"createMsgQ: MSG_Q_OBJ malloc error\n");
      return((MSG_Q_ID) NULL);
   }

   memset((void *)pMsgQ,0,sizeof(MSG_Q_OBJ));

   if ( (pMsgQ->msgQprocName = (char*) (malloc(strlen(myProcName)+1))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"createMsgQ: MSG_Q_OBJ malloc error\n");
      return((MSG_Q_ID) NULL);
   }
   strcpy(pMsgQ->msgQprocName,myProcName);
  
   pMsgQ->shrmMsgQInfo = shrmCreate(MSGQ_DBM_PATH,MSGQ_DBM_KEY, (unsigned long) MSQ_Q_DBM_SIZE);

   index = findProcIndex(myProcName);

   if (index == -1)
   {
     errLogRet(ErrLogOp,debugInfo,"createMsgQ() failed.\n");
     free(pMsgQ->msgQprocName);
     free(pMsgQ);
     return(NULL);
   }

   pMsgQ->shrmMsgQInfo = shrmCreate(MSGQ_DBM_PATH,MSGQ_DBM_KEY, (unsigned long) MSQ_Q_DBM_SIZE);
   if (pMsgQ->shrmMsgQInfo == NULL)
   {
     errLogRet(ErrLogOp,debugInfo,"createMsgQ() shrmCreate failed.\n");
     free(pMsgQ->msgQprocName);
     free(pMsgQ);
     return(NULL);
   }
   pMsgInfo = (MSG_Q_DBM_ID) ( shrmAddr(pMsgQ->shrmMsgQInfo) + (MSQ_Q_DBM_ESIZE * index));
 
   /* Fill in share memory entry for this created msgQ at the bottom 
	so no one will try to use it till we are done
   */

   pMsgQ->ipcKey = procList[index].ipcKey;
   pMsgQ->maxSize =  procList[index].maxSize;
   pMsgQ->pid = getpid();
   pMsgQ->AsyncFlag = 1;
  

   DPRINT4(2,"createMsgQ: name: '%s', pid: %d, maxSize: %d, Async: %d\n",
       pMsgQ->msgQprocName, pMsgQ->pid, pMsgQ->maxSize,pMsgQ->AsyncFlag);

   pMsgQ->msgId = msgQCreate(procList[index].ipcKey, msgSize);
   if (pMsgQ->msgId == NULL)
   {
     errLogRet(ErrLogOp,debugInfo,"createMsgQ() failed.\n");
     shrmRelease(pMsgQ->shrmMsgQInfo);
     free(pMsgQ->msgQprocName);
     free(pMsgQ);
     return(NULL);
   }
   /* are there any left over Messages in this msgQ
      if so then clear them all out
   */
   nMsges = msgQNumMsgs(pMsgQ->msgId);
   if (nMsges)
   {
     result = 0;
     DPRINT2(2,
      "createMsgQ: name: '%s', Has %d Messages Pending, Deleting Them.\n",
	pMsgQ->msgQprocName, nMsges);
     while ( result != -1 ) 
     {
	  result = msgQReceive(pMsgQ->msgId, buffer, msgSize, NO_WAIT);
     }
     nMsges = msgQNumMsgs(pMsgQ->msgId);
     if (nMsges)
       errLogRet(ErrLogOp,debugInfo,
        "createMsgQ: name: '%s', WARNING  Unable to Delete Pending Messages, still %d Pending.\n",
	pMsgQ->msgQprocName, nMsges);
     
   }

   /* contruct share memory entry for this create msgQ */
   strcpy(pMsgInfo->procName,procList[index].procName);
   pMsgInfo->ipcKey = procList[index].ipcKey;
   pMsgInfo->maxSize =  procList[index].maxSize;
   pMsgInfo->AsyncFlag = 1;

   pMsgInfo->pid = getpid();  /* last thing into shared memory, now msgQ is ready to recv */


   return(pMsgQ);
}

/**************************************************************
*
*  recvMsgQ - receive a message from a message queue
*
* This routine receives a message from a message queue.
* The only valid value of timeout are NO_WAIT or WAIT_FOREVER.
*  If the message receive is larger than "len" the message
* will be truncated to size "len". However the returned
* size will be of the actual message.
*  If NO_WAIT is specified and no message is present 
* recvMsgQ returns with errno of ENOMSG. 
*
*   Note: timeout is not implemented yet. 
*
* RETURNS:
*  Number of Bytes Receive, or -1 for error
*
*       Author Greg Brissey 10/12/94
*/
int recvMsgQ(MSG_Q_ID pMsgQ, char *buffer, int len, int mode)
/* MSG_Q_ID pMsgQ - message queue object pointer */
/* char *buffer - buffer to receive message */
/* int len - length of buffer */
/* int timeout - NO_WAIT, WAIT_FOREVER, or seconds to wait */
{
   if ( pMsgQ == NULL)
   {
        errno = EINVAL;
        return( -1 );
   }

   return(msgQReceive(pMsgQ->msgId, buffer, len, mode));
}

/**************************************************************
*
*  msgesInMsgQ - returns # msges in Msge Queue 
*
* This routine returns the number of messages queued within 
* this Message Queue
*  
* RETURNS:
*  # of msge queued
*
*       Author Greg Brissey 9/28/95
*/
int msgesInMsgQ(MSG_Q_ID pMsgQ)
{
 return(msgQNumMsgs(pMsgQ->msgId));
}

/**************************************************************
*
*  deleteMsgQ - Completely Deletes a Message Q from the System 
*
* This routine deletes the underlying System V message queue 
* from the OS. All pending message will be lost.
* Only the process calling createMsgQ should call this routine.
*  
* RETURNS:
* 0 , or -1 on error. 
*
*       Author Greg Brissey 10/12/94
*/
int deleteMsgQ(MSG_Q_ID pMsgQ)
{

   if ( pMsgQ == NULL)
   {
        errno = EINVAL;
        return( -1 );
   }

   if (pMsgQ->msgQprocName != NULL)
   {
     free(pMsgQ->msgQprocName);
   }

   shrmRelease(pMsgQ->shrmMsgQInfo);
   msgQDelete(pMsgQ->msgId);
   free(pMsgQ);
   return(0);
}

/**************************************************************
*
*  openMsgQ - opens a Message Q to Send Messages
*
* This routine opens a message Q previously created (createMsgQ())
* for sending messages base on the ID string.
*  
* RETURNS:
* MSG_Q_ID , or NULL on error. 
*
*       Author Greg Brissey 10/12/94
*/
MSG_Q_ID openMsgQ(char *msgQIdStr)
{
   MSG_Q_ID pMsgQ;
   MSG_Q_DBM_ID pMsgInfo;
   int mSize,index;

   /* malloc space for IPC_KEY_DBM_ID object */
   if ( (pMsgQ = (MSG_Q_ID) (malloc(sizeof(MSG_Q_OBJ)))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"openMsgQ: MSG_Q_OBJ malloc error\n");
      return((MSG_Q_ID) NULL);
   }

   memset((void *)pMsgQ,0,sizeof(MSG_Q_OBJ));

   index = findProcIndex(msgQIdStr);
   if (index == -1)
   {
     errLogRet(ErrLogOp,debugInfo,"openMsgQ() failed.\n");
     free(pMsgQ);
     return(NULL);
   }

   pMsgQ->shrmMsgQInfo = shrmCreate(MSGQ_DBM_PATH,MSGQ_DBM_KEY, (unsigned long) MSQ_Q_DBM_SIZE);
   if (pMsgQ->shrmMsgQInfo == NULL)
   {
     errLogRet(ErrLogOp,debugInfo,"createMsgQ() shrmCreate failed.\n");
     free(pMsgQ->msgQprocName);
     free(pMsgQ);
     return(NULL);
   }
   pMsgInfo = (MSG_Q_DBM_ID) ( shrmAddr(pMsgQ->shrmMsgQInfo) + (MSQ_Q_DBM_ESIZE * index));

   /* contruct from share memory entry for this create msgQ */
   pMsgQ->ipcKey = pMsgInfo->ipcKey;
   pMsgQ->pid = pMsgInfo->pid;
   pMsgQ->maxSize = pMsgInfo->maxSize;
   pMsgQ->AsyncFlag = pMsgInfo->AsyncFlag;

   DPRINT4(2,"openMsgQ: name: '%s', pid: %d, maxSize: %d, async: %d\n",
       msgQIdStr, pMsgQ->pid, pMsgQ->maxSize, pMsgQ->AsyncFlag);

   mSize = pMsgInfo->maxSize;

   pMsgQ->msgId = msgQCreate(pMsgQ->ipcKey, mSize);
   if (pMsgQ->msgId == NULL)
   {
      errLogSysRet(ErrLogOp,debugInfo,"openMsgQ: msgQCreate() failed.\n");
      shrmRelease(pMsgQ->shrmMsgQInfo);
      free(pMsgQ);
      return(NULL);
   }

   return(pMsgQ);
}


/**************************************************************
*
*  sendMsgQ - Send message via Message Q.
*
* This routine sends a message.
* Valid values for timeout are NO_WAIT or WAIT_FOREVER.
* If there is no room for the message on the queue the
* process will block unless NO_WAIT has been specified.
* Then sendMsgQ will return with errno of EAGAIN.
*   Note: timeout is not implemented yet. 
*
*  
* RETURNS:
* 0 - on success, -1 on Error
*
*       Author Greg Brissey 10/12/94
*/
int sendMsgQ(MSG_Q_ID pMsgQ, char *message, int len, int priority, int waitflg)
/* MSG_Q_ID pMsgQ - message queue Object Ptr */
/* char *message - message to send */
/* int len - length of message */
/* int priority - MSGQ_NORMAL, MSGQ_URGENT */
/* int waitflg - NO_WAIT, WAIT_FOREVER, or seconds to timeout */
{
   int index,alive;

   if ( pMsgQ == NULL)
   {
        errno = EINVAL;
        return( -1 );
   }

   index = pMsgQ->ipcKey & 0xff;
   DPRINT4(2,"sendMsgQ: Name: '%s', pid: %d, maxSize: %d, Asyncflag: %d\n",
	 procList[index].procName,pMsgQ->pid,pMsgQ->maxSize, pMsgQ->AsyncFlag);

  if (pMsgQ->pid != -1)
  {
     alive = kill(pMsgQ->pid,0); /* are you out there */
  }
  else
  {
     alive = -1;
     errno = ESRCH;
  }
   /* if process not present give error */
   if (alive == -1)
   {
       switch(errno)
	{
	  case ESRCH:
          /* No message */
	  break;

	case EPERM:
      	errLogRet(ErrLogOp,debugInfo,
	"sendMsgQ: Permission Error between Sending Process %d and Receiving Process %d  waiting on MsgQ Id: '%s'\n",
	  	getpid(), pMsgQ->pid,procList[index].procName);
	break;
	
	default:
        errLogRet(ErrLogOp,debugInfo,
	"sendMsgQ: Sending Process %d used an invalid signal on MsgQId: '%s'\n",
	  	getpid(), procList[index].procName);
	break;
      }
      return(-1);
   }


   /* Put the message into the Queue */
   if (msgQSend(pMsgQ->msgId, message, len, waitflg, priority) == -1)
   {
	return(-1);
   }
   /* send the async signal to process to inform it a message has been sent */
   if (pMsgQ->AsyncFlag)
   {
     DPRINT1(2,"sendMsgQ: SIGUSR1 to PID: %d\n",pMsgQ->pid);
     if (kill(pMsgQ->pid,SIGUSR1) == -1)
     {
        errLogSysRet(ErrLogOp,debugInfo,
	  "sendMsgQ: msgQ async Signal could not be sent to; '%s',pid: %d",
		procList[index].procName, pMsgQ->pid);
        return(-1);
     }
   }
   return(0);
}

#ifndef NIRVANA
/**************************************************************
*
*   setMsgQAsync - set msgQ to Asynchronous Operation
*
*   This program setups the Msg Q as Asynchronous.
*   You must provide a "callback",
*   a program which will be called when data arrives
*   on this msgQ.  You must also call asyncMainLoop
*   before your callback program can be called.  The
*   callback will receive a NULL argument.
*
*   returns:  0 if successful
*             -1 if not successful, with `errno' set
*                EINVAL       MSG_Q_ID NULL 
*                             or no callback specified
*
*       Author Greg Brissey 10/12/94
*/
int setMsgQAsync( MSG_Q_ID pMsgQ, void (*callback)() )
/* pMsgQ -        Pointer to Message Queue Object*/
/* void (*callback)() - routine to be called when data is ready on the MsgQ */
{

   MSG_Q_DBM_ID pMsgInfo;
   int index;
   
   if ( pMsgQ == NULL)
   {
        errno = EINVAL;
        return( -1 );
   }

   if ( callback == NULL)
   {
        errno = EINVAL;
        return( -1 );
   }

   /* 
      Setup the SIGUSR1 signal handler that is used to signal
      that there is a message in the Message Q.
   */
    registerAsyncHandlers(
                          SIGUSR1,      /* message Q Signal */
                          msgQitrp,     /* this puts the event on the eventQ */
                          callback
                         );

    index = pMsgQ->ipcKey & 0xff;
    pMsgInfo = (MSG_Q_DBM_ID) ( shrmAddr(pMsgQ->shrmMsgQInfo) + (MSQ_Q_DBM_ESIZE * index));
    pMsgQ->AsyncFlag = 1;
    pMsgInfo->AsyncFlag = 1;
    return(0);
}
#endif

/**************************************************************
*
*   closeMsgQ - closes the msgQ opened via openMsgQ 
*
*   This routine closes the msgQ opened via openMsgQ 
*   and free the allocated memory resourced used.
*
*   returns:  0 if successful
*             -1 if not successful, with `errno' set
*                EINVAL       MSG_Q_ID NULL 
*
*       Author Greg Brissey 10/12/94
*/
int closeMsgQ(MSG_Q_ID pMsgQ)
{
    if (pMsgQ != NULL)
    {
     msgQClose(pMsgQ->msgId);
     shrmRelease(pMsgQ->shrmMsgQInfo);
     free(pMsgQ);
     return(0);
    }
    else
    {
        errno = EINVAL;
        return( -1 );
    }
}

#ifndef NIRVANA
/**************************************************************
*
*  msgQitrp - Routine envoked on receiving the Message Q Signal
*
*  This catches the Message Q Signal
*  
* RETURNS:
* void 
*
*       Author Greg Brissey 10/12/94
*/
static void msgQitrp(int signal)
{
  (void) signal;
  /* Place the SIGUSR1 interrupt & msgID onto the eventQ, the non-
    interrupt function (processMsge) will be called with msgId as an argument */
  processNonInterrupt( SIGUSR1, (void*) NULL );
  return;
}
#endif


/* ###################################################################### */
/* ###################################################################### */
#else   /* VNMRS_WIN32 */
/* ###################################################################### */
/* ###################################################################### */

#include "rngBlkLib.h"

/* Build a Windows native msgQ */

static wincnt = 0;
static MSG_Q_ID pRecvMsgQId = NULL;   /* used by msgCallBack() function */

/* find_MsgQ_WindowHandle function continously called by 'EnumWindows()' until 
   FALSE is return or all Top Windows have been interated through
*/
BOOL CALLBACK find_MsgQ_WindowHandle(HWND h,LPARAM l)
{	char temp[1000];
    MSG_Q_ID pMsgQId;
    pMsgQId = (MSG_Q_ID) l;

	GetClassName(h,temp,1000);
	wincnt++;
	// DPRINT2(-2,"EnumWIN: %d - '%s'\n",wincnt,temp);

	if (!strcmp(temp,pMsgQId->msgQWindowName))
	{	pMsgQId->msgWindowHandle = h;
	    // DPRINT1(-2," ===== FOUND IT ==== '%s'\n",pMsgQId->msgQWindowName);
		return(FALSE);
	}
	return(TRUE);
}

#define MAX_DATA_LEN  256

/* this routine is called when DispatchMessage is called 
   or when SendMessage is called 
 */
LRESULT CALLBACK msgCallBack(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
    char* s;    
//	char msg[128];
    PCOPYDATASTRUCT msgData;

    switch (message) 
	{
        case WM_DESTROY:
            PostQuitMessage(0);
	    ExitThread(0);
            break;

        case WM_COPYDATA:
	  {
	   char *bptr;
	   if (InSendMessage())
	      ReplyMessage(TRUE);
	   msgData = (PCOPYDATASTRUCT)lParam;
           s = (char *)((PCOPYDATASTRUCT)lParam)->lpData;
	   if (pRecvMsgQId != NULL)
	   {
	      bptr = (char*) malloc(MAX_DATA_LEN);
              if (msgData->cbData+1 <= MAX_DATA_LEN)
	         memcpy(bptr,s,msgData->cbData+1);
              else
	         memcpy(bptr,s,MAX_DATA_LEN);
	      DPRINT2(-1,"size: %d, str: '%s'\n",msgData->cbData,(char*) msgData->lpData);
	      rngBlkPut(pRecvMsgQId->pMsgRingBuf,(long*) &bptr,1);
	   }
	  }
            break;

        case WM_QUIT:
            PostQuitMessage(0);
	    ExitThread(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
   }
   
   return 0;
}

/* The Classic Windows Mesage Loop Thread Routine  Gets & Dispatches messages */
DWORD WINAPI msgLoop(LPVOID msgId) {
    MSG msg;
    MSG_Q_ID pMsgQId;
    pMsgQId = (MSG_Q_ID) msgId;
    pMsgQId->msgWindowHandle = CreateWindow(pMsgQId->msgQWindowName /* "Expproc_MSG_WINDOW"*/ , "", (DWORD) 0,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, NULL, NULL, NULL, NULL);
   
    while ( GetMessage(&msg, NULL, 0, 0) ) {
        // TranslateMessage(&msg);
        DispatchMessage(&msg);  /* send msg along to the registered callback routine in WindowClass */
    }

    return (DWORD) 0;
}


/* Create the Window Class and Register it for the Message Queue */
BOOL CreateMsgWindowClass(MSG_Q_ID pMsgQId, void*  callBackFunc)
{
    WNDCLASSEX wc;
    BOOL retVal;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_GLOBALCLASS;
    wc.lpfnWndProc = (WNDPROC) callBackFunc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = pMsgQId->hInstance;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = pMsgQId->msgQWindowName; // "Expproc_MSG_WINDOW";
    wc.hIconSm = NULL;

    pMsgQId->registeredWindowClass = RegisterClassEx(&wc);  // returns the Class ATOM
    if ( !pMsgQId->registeredWindowClass ) 
    {
       pMsgQId->registeredWindowClass = (ATOM) NULL;
       retVal = 0;
    } else {
       retVal = 1;
    }
    return retVal;
}



static void freeResources(MSG_Q_ID msgQId)
{
	if (msgQId == NULL)
		return;
	if (msgQId->msgQprocName != NULL)
		free(msgQId->msgQprocName);
	if (msgQId->msgQWindowName != NULL)
		free(msgQId->msgQWindowName);
	if (msgQId->pMsgRingBuf != NULL)
        rngBlkDelete(msgQId->pMsgRingBuf);
	free(msgQId);

}

/**************************************************************
*
*  createMsgQ - Initialize a Windows Message Q to Receive Messages
*
* This routine creates and registers a Window Class for receiving 
* Window Messages
* A separated thread is create which Creates the unseen Window and 
* then goes into the classic Windows message loop
* 
* RETURNS:
* MSG_Q_ID , or NULL on error. 
*                    malloc failure
*
*       Author Greg Brissey 6/26/06
*/
MSG_Q_ID createMsgQ(char *myProcName, int keyindex, int msgSize)
/* myProcName - unique id string for Message Queue */
/* keyindex - ipc key seed */
/* msgSize - Maximum Size of message in Bytes */
{
   MSG_Q_ID pMsgQ;
   size_t len;
   HANDLE threadHandle;
   DWORD threadId;

   /* malloc space for MSG_Q object */
   if ( (pMsgQ = (MSG_Q_ID) (malloc(sizeof(MSG_Q_OBJ)))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"createMsgQ: MSG_Q_OBJ malloc error\n");
      return((MSG_Q_ID) NULL);
   }

   memset((void *)pMsgQ,0,sizeof(MSG_Q_OBJ));

   if ( (pMsgQ->msgQprocName = (char*) (malloc(strlen(myProcName)+1))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"createMsgQ: Key Name malloc error\n");
      return((MSG_Q_ID) NULL);
   }
   strcpy(pMsgQ->msgQprocName,myProcName);

   len = strlen(myProcName) + strlen(WINDOWS_NAME_SUFIX) + 1;
   if ( (pMsgQ->msgQWindowName = (char*) (malloc(len))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"createMsgQ: Msg Name malloc error\n");
	  freeResources(pMsgQ);
      return((MSG_Q_ID) NULL);
   }
   strcpy(pMsgQ->msgQWindowName,myProcName);
   strcat(pMsgQ->msgQWindowName,WINDOWS_NAME_SUFIX);  /* _Msg_Window */

   /* make sure this window has not already been registered.
      wrt Procs this means there already is the same proc running 
  */
   wincnt = 0;
   EnumWindows(find_MsgQ_WindowHandle,(LPARAM) pMsgQ);
   // DPRINT1(-1,"%d enum windows\n",wincnt);
   if (pMsgQ->msgWindowHandle != NULL)
   {
	   errLogSysRet(ErrLogOp,debugInfo,"createMsgQ: MsgQ Windows Exists for '%s'\n",pMsgQ->msgQWindowName);
       freeResources(pMsgQ);
       return((MSG_Q_ID) NULL);
   }

   pMsgQ->pMsgRingBuf = rngBlkCreate(128,pMsgQ->msgQWindowName, 1);

   pMsgQ->hInstance = GetModuleHandle(NULL); 

   /* Create and register the Window Class, 1st step in getting a Windows msgQ */
   CreateMsgWindowClass(pMsgQ, msgCallBack);

   pRecvMsgQId = pMsgQ;

   /* use a separate thread to create the actual Window and then go into the msg loop */
   threadHandle = CreateThread(NULL, 0, msgLoop,
                                    pMsgQ, 0, &threadId);
   
   if (threadHandle == NULL)
   {
     errLogRet(ErrLogOp,debugInfo,"createMsgQ() failed.\n");
	 freeResources(pMsgQ);
     return(NULL);
   }

   return(pMsgQ);
}
/**************************************************************
*
*  deleteMsgQ - Completely Deletes a Message Q from the System 
*
* This routine deletes the underlying Windows Class & Message Loop Thread
* All pending message will be lost.
* Only the process calling createMsgQ should call this routine.
*  
* RETURNS:
* 0 , or -1 on error. 
*
*       Author Greg Brissey 10/12/94
*/
int deleteMsgQ(MSG_Q_ID pMsgQ)
{

   if ( pMsgQ == NULL)
   {
        return( -1 );
   }
   
   /* terminate message loop threae */
   SendMessage(pMsgQ->msgWindowHandle,WM_DESTROY,0,0);

   /* Unregister Window Message Class */
   UnregisterClass((LPCTSTR) pMsgQ->registeredWindowClass,pMsgQ->hInstance);
   
   freeResources(pMsgQ);

   return(0);
}


/**************************************************************
*
*  recvMsgQ - receive a message from a message queue
*
* This routine receives a message from a message queue.
* The only valid value of timeout are NO_WAIT or WAIT_FOREVER.
*  If the message receive is larger than "len" the message
* will be truncated to size "len". However the returned
* size will be of the actual message.
*  If NO_WAIT is specified and no message is present 
* recvMsgQ returns with errno of ENOMSG. 
*
*   Note: timeout is not implemented yet. 
*
* RETURNS:
*  Number of Bytes Receive, or -1 for error
*
*       Author Greg Brissey 10/12/94
*/
int recvMsgQ(MSG_Q_ID pMsgQ, char *buffer, int len, int mode)
/* MSG_Q_ID pMsgQ - message queue object pointer */
/* char *buffer - buffer to receive message */
/* int len - length of buffer */
/* int timeout - NO_WAIT, WAIT_FOREVER, or seconds to wait */
{
	int status;
	char *strptr;
   if ( pMsgQ == NULL)
   {
        return( -1 );
   }
   status = rngBlkGet(pMsgQ->pMsgRingBuf,(long*) &strptr,1);
   memcpy(buffer,strptr,len);

   return(0);
}

/**************************************************************
*
*  openMsgQ - opens a Message Q to Send Messages
*
* This routine opens a message Q previously created (createMsgQ())
* for sending messages base on the ID string.
*  
* RETURNS:
* MSG_Q_ID , or NULL on error. 
*
*       Author Greg Brissey 10/12/94
*/
MSG_Q_ID openMsgQ(char *msgQIdStr)
{
   MSG_Q_ID pMsgQ;
   size_t len;

   /* malloc space for MSG_Q_ID object */
   if ( (pMsgQ = (MSG_Q_ID) (malloc(sizeof(MSG_Q_OBJ)))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"openMsgQ: MSG_Q_OBJ malloc error\n");
      return((MSG_Q_ID) NULL);
   }

   memset((void *)pMsgQ,0,sizeof(MSG_Q_OBJ));

   if ( (pMsgQ->msgQprocName = (char*) (malloc(strlen(msgQIdStr)+1))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"createMsgQ: MSG_Q_OBJ malloc error\n");
      return((MSG_Q_ID) NULL);
   }
   strcpy(pMsgQ->msgQprocName,msgQIdStr);

   len = strlen(msgQIdStr) + strlen(WINDOWS_NAME_SUFIX) + 1;
   if ( (pMsgQ->msgQWindowName = (char*) (malloc(len))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"createMsgQ: MSG_Q_OBJ malloc error\n");
	  freeResources(pMsgQ);
      return((MSG_Q_ID) NULL);
   }
   strcpy(pMsgQ->msgQWindowName,msgQIdStr);
   strcat(pMsgQ->msgQWindowName,WINDOWS_NAME_SUFIX);  /* _MSG_WINDOW */
   wincnt = 0;
   EnumWindows(find_MsgQ_WindowHandle,(LPARAM) pMsgQ);
   // DPRINT1(-1,"%d enum windows\n",wincnt);
   if (pMsgQ->msgWindowHandle == NULL)
   {
	   errLogSysRet(ErrLogOp,debugInfo,"createMsgQ: MsgQ Windows Exists for '%s'\n",pMsgQ->msgQWindowName);
       freeResources(pMsgQ);
       return((MSG_Q_ID) NULL);
   }


   return(pMsgQ);
}

/**************************************************************
*
*   closeMsgQ - closes the msgQ opened via openMsgQ 
*
*   This routine closes the msgQ opened via openMsgQ 
*   and free the allocated memory resourced used.
*
*   returns:  0 if successful
*             -1 if not successful, with `errno' set
*                EINVAL       MSG_Q_ID NULL 
*
*       Author Greg Brissey 10/12/94
*/
closeMsgQ(MSG_Q_ID pMsgQ)
{
    if (pMsgQ != NULL)
    {
	  freeResources(pMsgQ);
      return(0);
    }
    else
    {
      return( -1 );
    }
}


/**************************************************************
*
*  sendMsgQ - Send message via Message Q.
*
* This routine sends a message.
* Valid values for timeout are NO_WAIT or WAIT_FOREVER.
* If there is no room for the message on the queue the
* process will block unless NO_WAIT has been specified.
* Then sendMsgQ will return with errno of EAGAIN.
*   Note: timeout is not implemented yet. 
*
*  
* RETURNS:
* 0 - on success, -1 on Error
*
*       Author Greg Brissey 10/12/94
*/
sendMsgQ(MSG_Q_ID pMsgQ, char *message, int len, int priority, int waitflg)
/* MSG_Q_ID pMsgQ - message queue Object Ptr */
/* char *message - message to send */
/* int len - length of message */
/* int priority - MSGQ_NORMAL, MSGQ_URGENT */
/* int waitflg - NO_WAIT, WAIT_FOREVER, or seconds to timeout */
{
//    MSG_Q_ID msgId;
//    int stat,i,index,alive;
 //   LPARAM msgarg;
   COPYDATASTRUCT msgData;
   LRESULT result;

   if ( pMsgQ == NULL)
   {
    //     errno = EINVAL;
        return( -1 );
   }
   
   if (pMsgQ->msgWindowHandle == NULL)
   {
       errLogSysRet(ErrLogOp,debugInfo,"createMsgQ: MsgQ Windows Does Not Exists for '%s'\n",pMsgQ->msgQWindowName);
       return(-1);
   }
   
   if (len > MAX_DATA_LEN)
   {
       errLogRet(ErrLogOp,debugInfo,"createMsgQ: len %d, > than Max. %d\n",len,MAX_DATA_LEN);
       return(-1);
   }
 
   msgData.dwData = 0;  // data to be passed to the receiving application
   msgData.cbData = len; // size, in bytes, of the data pointed to by the lpData member
   msgData.lpData = message;  // ointer to data to be passed to the receiving application

   /* Put the message into the Queue */
   // PostMessage(pMsgQ->msgWindowHandle,WM_COPYDATA,0,(LPARAM) &msgData);
   result =  SendMessage(pMsgQ->msgWindowHandle,WM_COPYDATA,0,(LPARAM) &msgData);
   
   return( ((result == TRUE) ? 0 : -1) );
}
/**************************************************************
*
*  msgesInMsgQ - returns # msges in Msge Queue 
*
* This routine returns the number of messages queued within 
* this Message Queue
*  
* RETURNS:
*  # of msge queued
*
*       Author Greg Brissey 9/28/95
*/
int msgesInMsgQ(MSG_Q_ID pMsgQ)
{
  if (pMsgQ == NULL)
     return -1;
  if (pMsgQ->pMsgRingBuf == NULL)
     return -1;

  return(rngBlkNElem(pMsgQ->pMsgRingBuf));
}



 // #define TEST_MAIN
#ifdef TEST_MAIN

DWORD WINAPI msgQGetThread(LPVOID msgId) {
	char msge[256];
    MSG_Q_ID pMsgQId;
	int stat;
    pMsgQId = (MSG_Q_ID) msgId;
    
    while ( 1 ) {
        stat = recvMsgQ(pMsgQId, msge, 128, 0);
		DPRINT1(-1,"msge: '%s'\n",msge);
    }

    return (DWORD) 0;
}
/* test main1 for testing basic operation of msgQ, one process messaging to itself
   Best to use a source debugger for to allow proper thread execution for proper operations
   Sleep are there to allow the other thread time to execute.
*/
#define SELFTEST
#ifdef SELFTEST

 main()
// int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE prevhInstance, LPSTR str1, int cmdshow)
{
  MSG_Q_ID pMsgQ, pMsgOpen;
    HANDLE threadHandle;
   DWORD threadId;
  char *mqname = "MyTestMQ";
  char *msg = "Test String to Send";

 
  pMsgQ = createMsgQ(mqname, 1234, 256);
  Sleep(200);
  pMsgOpen = openMsgQ(mqname);

  threadHandle = CreateThread(NULL, 0, msgQGetThread,
                                   pMsgQ, 0, &threadId);
  Sleep(100);
  sendMsgQ(pMsgOpen,msg, (int) (strlen(msg)+1), 0, 0);

  Sleep(200);
  closeMsgQ(pMsgOpen);
  Sleep(200);

  deleteMsgQ(pMsgQ);
   Sleep(200);
 
}

#else /* TWO_PROCESS_TEST */

/* two process test, ther is the receive process and sendin process
   with VS you will need two VS running to execute these two main together
*/
#define RECEIVE_PROC
#ifdef RECEIVE_PROC
 /* receive side */
 main()
{
  MSG_Q_ID pMsgQ;
  int stat;
  char msge[256];
  char *mqname = "MyTestMQ";
  char *msg = "Test String to Send";

 
  pMsgQ = createMsgQ(mqname, 1234, 256);
  Sleep(200);
  while ( 1 ) {
        stat = recvMsgQ(pMsgQ, msge, 128, 0);
		DPRINT1(-1,"msge: '%s'\n",msge);
    }
}
#else
/* SEND Side */
 main()
// int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE prevhInstance, LPSTR str1, int cmdshow)
{
  MSG_Q_ID pMsgOpen;
  int len;
  char *mqname = "MyTestMQ";
  char *msg = "Test String to Send from Another Process";

  
  pMsgOpen = openMsgQ(mqname);

  len = (int) strlen(msg)+1;
  sendMsgQ(pMsgOpen, msg, len, 0, 0);
  sendMsgQ(pMsgOpen, msg, len, 0, 0);
  Sleep(200);
  closeMsgQ(pMsgOpen);
  Sleep(200);
}
#endif
#endif
#endif


#endif  /* VNMRS_WIN32 */
