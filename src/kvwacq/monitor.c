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
/*
DESCRIPTION


*/

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <sysLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>

#include "taskPrior.h"
#include "hostAcqStructs.h"
#include "consoleStat.h"
#include "hostMsgChannels.h"
#include "expDoneCodes.h"
#include "acqcmds.h"
#include "stmObj.h"
#include "autoObj.h"
#include "fifoObj.h"
#include "logMsgLib.h"
#include "tuneObj.h"
#include "AParser.h"
#include "hardware.h"
#include "errorcodes.h"
#include "vtfuncs.h"
#include "instrWvDefines.h"


#ifndef DEBUG_CT_ELEM
#define DEBUG_CT_ELEM  (9)
#endif


#define LOCK_ADC_RSHIFT 2

extern MSG_Q_ID pMsgesToHost;
extern MSG_Q_ID pMsgesToAParser;
extern MSG_Q_ID pMsgesToAupdt;
extern MSG_Q_ID pMsgesToPHandlr;
extern MSG_Q_ID pMsgesToXParser;
extern MSG_Q_ID pMsgesToHost;
extern EXCEPTION_MSGE GenericException;

extern STMOBJ_ID	pTheStmObject;
extern AUTO_ID   	pTheAutoObject;
extern FIFO_ID   	pTheFifoObject;
extern TUNE_ID		pTheTuneObject;

extern ACODE_ID  pTheAcodeObject; /* Acode object */
extern RING_ID  pSyncActionArgs;  /* rngbuf for sync action function args */

extern int SA_Criteria; /* criteria for SA, EXP_FID_CMPLT, BS_CMPLT, IL_CMPLT */
extern unsigned long SA_Mod; /* modulo criteria for SA, ie which fid to stop at 'il'*/
extern unsigned long SA_CTs;  /* Completed Transients for SA */

extern int calc_new_interval( int interval );
extern void set_new_interval( int interval );


unsigned long
htol( char *hexascii )
{
	int		iter;
	unsigned long	hexval;
	unsigned char	hexdigit;

	hexval = 0;

/* Skip past 0x ... */

	if (*hexascii == '0' && (*(hexascii+1) == 'x' || *(hexascii+1) == 'X'))
	  hexascii += 2;

/* Skip past leading zeros ... */

	while (*hexascii == '0')
	  hexascii++;

/*  You are allowed 8 hex digits ... */

	for (iter = 0; iter < 8; iter++) {
		hexdigit = (unsigned char) (*(hexascii++));

		if (hexdigit == '\0')
		  break;

		if (hexdigit >= 'a' && hexdigit <= 'z') {
			hexdigit = hexdigit - 0x20 - 'A' + 10;
			hexval = (hexval << 4) + hexdigit;
		}
		else if (hexdigit >= '0' && hexdigit <= '9')
		  hexval = (hexval << 4) + hexdigit - '0';
		else if (hexdigit >= 'A' && hexdigit <= 'Z')
		  hexval = (hexval << 4) + hexdigit - 'A' + 10;
		else
		  break;
	}

	return( hexval );
}

/*  These programs help implement interactive acquires  */

#define  NO_DISPLAY	0
#define  LOCK_DISPLAY	1
#define  FID_DISPLAY	2

static int	typeInteractiveAcq = NO_DISPLAY;

static
startLock()
{
	char	msg4AParser[ 40 ];
	int	len;

	setup_for_lock();

	sprintf( &msg4AParser[ 0 ], "ACQI_PARSE,%s,1,0;", "lock" );
	len = strlen( &msg4AParser[ 0 ] );
	msgQSend(pMsgesToAParser, &msg4AParser[ 0 ], len+1, NO_WAIT, MSG_PRI_NORMAL);

	typeInteractiveAcq = LOCK_DISPLAY;
}

static
getInteract()
{
	ITR_MSG intrmsg;

	DPRINT( 2, "get interact program called\n" );
	intrmsg.tag = 0;
	intrmsg.count = 1;
	intrmsg.donecode = USER_FLAG;
	intrmsg.errorcode = 0;
	intrmsg.msgType = SEND_DATA_NOW;
	msgQSend(pTheStmObject->pIntrpMsgs,(char *) &(intrmsg),sizeof(ITR_MSG),WAIT_FOREVER,MSG_PRI_NORMAL);
}

startFidScope()
{
	ITR_MSG intrmsg;

	DPRINT( 2, "start FID scope program called\n" );
	intrmsg.tag = 0;
	intrmsg.count = 1;
	intrmsg.donecode = USER_FLAG;
	intrmsg.errorcode = 0;
	intrmsg.msgType = START_INTERVALS;
	msgQSend(pTheStmObject->pIntrpMsgs,(char *) &(intrmsg),sizeof(ITR_MSG),WAIT_FOREVER,MSG_PRI_NORMAL);
}

stopFidScope()
{
	ITR_MSG intrmsg;

	DPRINT( 2, "stop FID scope program called\n" );
	intrmsg.tag = 0;
	intrmsg.count = 1;
	intrmsg.donecode = USER_FLAG;
	intrmsg.errorcode = 0;
	intrmsg.msgType = STOP_INTERVALS;
	msgQSend(pTheStmObject->pIntrpMsgs,(char *) &(intrmsg),sizeof(ITR_MSG),WAIT_FOREVER,MSG_PRI_NORMAL);
}

static
stopInteract()
{
	int			len, tid;
	char			msg4Aupdt[ 32 ];
	ITR_MSG 		intrmsg;
	extern MSG_Q_ID		pUpLinkIMsgQ;

	DPRINT1( 1, "stopInteract program starts with type of acq = %d\n", typeInteractiveAcq );

	stopFidScope();
	if (typeInteractiveAcq == LOCK_DISPLAY) {
		sprintf( &msg4Aupdt[ 0 ], "1;%d,%d,1,%d;", FIX_RTVARS, getstopoffset(), 1 );
		len = strlen( &msg4Aupdt[ 0 ] );
		msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], len+1, NO_WAIT, MSG_PRI_NORMAL);
		typeInteractiveAcq = NO_DISPLAY;
		DPRINT(0,"stop FID display\n");
		GenericException.exceptionType = EXP_ABORTED;
		GenericException.reportEvent = EXP_ABORTED;
	   /* send Abort to exception handler task, it knows what to do */

		msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
				sizeof(EXCEPTION_MSGE), 
				NO_WAIT, MSG_PRI_URGENT);

		update_acqstate( ACQ_IDLE );
	}
	else if (typeInteractiveAcq == FID_DISPLAY) {
		DPRINT(0,"stop FID display\n");
		GenericException.exceptionType = EXP_ABORTED;
		GenericException.reportEvent = EXP_ABORTED;
	   /* send Abort to exception handler task, it knows what to do */

		msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
				sizeof(EXCEPTION_MSGE), 
				NO_WAIT, MSG_PRI_URGENT);

		typeInteractiveAcq = NO_DISPLAY;
		update_acqstate( ACQ_IDLE );
	}

	DPRINT( 1, "stopInteract program completes\n" );
}

/*  End of interactive / lock signal programs  */


startComLnk(int priority,int options, int stacksize)
{
   int strtHostComLk();

   if (taskNameToId("tComLink") == ERROR)
     taskSpawn("tComLink",priority,options,stacksize,strtHostComLk,
		pMsgesToHost,priority, options,stacksize,5,6,7,8,9,10);

}

restartComLnk()
{
   int tid,stat,priority,stacksize,taskoptions;
   /* if tComLink still around then Connection was never made */
   DPRINT(1,"Time to Restart Exppproc Link.");
   if ((tid = taskNameToId("tComLink")) == ERROR)
   {
    DPRINT(1,"tComLink is not running so delete tasks.");
    if ((tid = taskNameToId("tMonitor")) != ERROR)
    {
       stat = taskDelete(tid);
    }
    if ((tid = taskNameToId("tHostAgent")) != ERROR)
    {
       stat = taskDelete(tid);
    }
    if ((tid = taskNameToId("tStatAgent")) != ERROR)
    {
       stat = taskDelete(tid);
    }
    if ((tid = taskNameToId("tStatMon")) != ERROR)
    {
       stat = taskDelete(tid);
    }
    priority = 200;
    stacksize = 3000;
    taskoptions = 0;
    DPRINT(0,"restart ComLnk.");
    startComLnk(MONITOR_TASK_PRIORITY,STD_TASKOPTIONS,STD_STACKSIZE);
   }
}

killComLnk()
{
   int tid;

   if ((tid = taskNameToId("tComLink")) != ERROR)
       taskDelete(tid);
   if ((tid = taskNameToId("tMonitor")) != ERROR)
       taskDelete(tid);
   if ((tid = taskNameToId("tHostAgent")) != ERROR)
       taskDelete(tid);
   if ((tid = taskNameToId("tStatAgent")) != ERROR)
       taskDelete(tid);
   if ((tid = taskNameToId("tStatMon")) != ERROR)
       taskDelete(tid);
}


#define  MIN_INTERVAL	10
#define  MAX_INTERVAL	5000
#define  DEFAULT_INTERVAL  MAX_INTERVAL
#define  UNITS_PER_SEC	1000

extern STATUS_BLOCK	currentStatBlock;

static struct {
	int	currentTicks;
	SEM_ID	newValueTicks;
} statblock_coordinator;


startStatMonitor( int taskpriority, int taskoptions, int stacksize )
{
   int statmonitor();

   statblock_coordinator.currentTicks = calc_new_interval( DEFAULT_INTERVAL );
   if (taskNameToId("tStatMon") == ERROR)
     taskSpawn("tStatMon",taskpriority,taskoptions,stacksize,statmonitor,
		1,2,3,4,5,6,7,8,9,10);
}


/*************************************************************
*
*  strtHostComLk - establich connection with expproc then spawn
*  monitor & hostAgent tasks
*
*					Author Greg Brissey 10-6-94
*/
strtHostComLk(MSG_Q_ID pMsgesToHost,int taskpriority,int taskoptions,int stacksize)
{
   int monitor();
   int hostagent();
   int statusagent();

   int chan_no;

   chan_no = EXPPROC_CHANNEL;
   stacksize = 3000;

   DPRINT(1,"HostComLink: Establish Connection to Host.\n");
   EstablishChannelCon(chan_no);
   DPRINT(1,"HostComLink: Connection Established.\n");

   if (taskNameToId("tMonitor") == ERROR)
     taskSpawn("tMonitor",MONITOR_TASK_PRIORITY,taskoptions,MED_STACKSIZE,monitor,
		    chan_no,pMsgesToHost,3,4,5,6,7,8,9,10);

   if (taskNameToId("tHostAgent") == ERROR)
     taskSpawn("tHostAgent",HOSTAGENT_TASK_PRIORITY,taskoptions,STD_STACKSIZE,
               hostagent, chan_no,pMsgesToHost,3,4,5,6,7,8,9,10);

   if (taskNameToId("tStatAgent") == ERROR)
     taskSpawn("tStatAgent",STATAGENT_TASK_PRIORITY,taskoptions,STD_STACKSIZE,
		statusagent, 1,2,3,4,5,6,7,8,9,10);

   startStatMonitor(STATMON_TASK_PRIORITY,taskoptions,STD_STACKSIZE);
}

/*
    All reads from and writes to the host computer should go through
    phandlerReadChannel and phandlerWriteChannel.  These two programs
    notify the problem handler if an error occurs.  The problem
    handler then resets the console software and prepares those tasks
    that interface with the host for a reconnection.

    At this time the monitor, host agent, uplinker and downlinker are
    the tasks that communicate with the host computer.

    rReadChannel and rWriteChannel are now obsolete.

    phandlerReadChannel - reads channel, if connection is closed by Host
    then the LOST_CONN is sent to phandler & task is suspended
*/

phandlerReadChannel(int chan_no, char *buffer, int size)
{
   int bytes;

    bytes = readChannel(chan_no,buffer, size);
    if (bytes == 0)  /* lost connection report to phandler */
    {
       errLogSysRet(LOGIT,debugInfo,
	 "phandlerReadChannel, Channel: %d,  Host Closed Connection to Console.\n",chan_no);
       GenericException.exceptionType = LOST_CONN;
       GenericException.reportEvent = LOST_CONN;
       /* send Lost Connection to exception handler task, it knows what to do */
       msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
				sizeof(EXCEPTION_MSGE), 
					NO_WAIT, MSG_PRI_URGENT);
       errLogRet(LOGIT,debugInfo,
	 "phandlerReadChannel, Suspending Task: '%s'\n",taskName(taskIdSelf()));
       taskSuspend(0);
    }
    return(bytes);
}

/*
    phandlerWriteChannel - writes channel, if connection is closed by Host
    then the LOST_CONN is sent to phandler & task is suspended
*/

phandlerWriteChannel(int chan_no, char *buffer, int size)
{
   int bytes;

    bytes = writeChannel(chan_no,buffer, size);
    if (bytes == -1)  /* lost connection report to phandler */
    {
       errLogSysRet(LOGIT,debugInfo,
         "phandlerWriteChannel, Channel: %d,  Host Closed Connection to Console.\n",chan_no);
       GenericException.exceptionType = LOST_CONN;
       GenericException.reportEvent = LOST_CONN;
       /* send Lost Connection to exception handler task, it knows what to do */
       msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
				sizeof(EXCEPTION_MSGE), 
					NO_WAIT, MSG_PRI_URGENT);
       errLogRet(LOGIT,debugInfo,
	 "phandlerReadChannel, Suspending Task: '%s'\n",taskName(taskIdSelf()));
       taskSuspend(0);
    }
    return(bytes);
}


/*************************************************************
*
*  monitor - Wait for Message from Host 
*   Receives Messages from Host and routes them to the proper
*   task msgQ. 
*
*					Author Greg Brissey 10-6-94
*/
monitor(int chanId, MSG_Q_ID msges)
{
   void decode();
   char  msge[CONSOLE_MSGE_SIZE];
   int bytes;

   DPRINT(1,"Monitor 1:Server LOOP Ready & Waiting.\n");
   FOREVER
   {
     /* if connection lost this routine send msge to phandler LOST_CONN */
     bytes = phandlerReadChannel(chanId, msge, CONSOLE_MSGE_SIZE);
     DPRINT1(2,"Monitor: got %d bytes\n",bytes);

     if (bytes < 0)
     {
         errLogSysRet(LOGIT,debugInfo, "read from Host Msge failed\n");
         break;
     }
     decode( &msge[ 0 ], chanId );
   } 
/*
   closeChannel( chan_no );
*/
}

/*********************************************************************
*
* decode base on the cmd code decides what msgeQ to put the message into
*  then it returns
*
*					Author Greg Brissey 10-6-94
*/
void decode(char *msge, int chanId)
{
   char *token;
   int len;
   int cmd;

   DPRINT2(2,"decode message: %s, length: %d\n",msge, strlen( msge ));

   cmd = atoi( msge );
   DPRINT1(2,"decode command: %d\n",cmd);
   switch( cmd )
   {
     case ECHO:
	  {
		long	ival;
		long	imsg[CONSOLE_MSGE_SIZE/sizeof( long )];
		
   	        DPRINT(1,"ECHO\n");
		token = strchr( msge, '\n' );
		if (token == NULL) {
			errLogRet( LOGIT, debugInfo, "ECHO command but no message\n" );
			break;
		}

		token++;			/*  Move address past the new-line */
		len = strlen( token );
		if (len < 1) {
			errLogRet( LOGIT, debugInfo, "ECHO message of 0 length\n" );
			break;
		}

	/*  Translate the ASCII message into binary format.  We expect
	    the first token to be a number.  Use atoi to get its value.
	    Then advance the address (token) past this number.  We assume
	    the next character will be the separator, so if it is not
	    '\0', we advance the address past it too.  We then send a
	    message consisting of the binary value of the first token as
	    the first integer, followed by the rest of the string.	*/
	    
		ival = atoi( token );
		while ('0' <= *token && *token <= '9') {
			token++;
			len--;
		}
		if (*token != '\0') {
			token++;
			len--;
		}
		imsg[ 0 ] = ival;
#ifdef INSTRUMENT
     		wvEvent(EVENT_MONITOR_CMD_ECHO,NULL,NULL);
#endif
		strcpy( (char *) &imsg[ 1 ], token );
		msgQSend(pMsgesToHost, (char *) &imsg[ 0 ], len + sizeof(int) + 1,
			 NO_WAIT, MSG_PRI_NORMAL);
	  }
	  break;

     case XPARSER:
   	   DPRINT(1,"XPARSER\n");
	   token = strchr( msge, '\n' );
	   if (token == NULL) {
		errLogRet( LOGIT, debugInfo, "XPARSER command but no message\n" );
		break;
	   }

	   token++;
	   len = strlen( token );
	   if (len < 1) {
		errLogRet( LOGIT, debugInfo, "XPARSER message of 0 length\n" );
		break;
	   }
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_XPARSER,NULL,NULL);
#endif
	   msgQSend(pMsgesToXParser, token, len+1, NO_WAIT, MSG_PRI_NORMAL);
	  break;

     case APARSER:
   	   DPRINT(1,"APARSER\n");
	   token = strchr( msge, '\n' );
	   if (token == NULL) {
		errLogRet( LOGIT, debugInfo, "APARSER command but no message\n" );
		break;
	   }

	   token++;
	   len = strlen( token );
	   if (len < 1) {
		errLogRet( LOGIT, debugInfo, "APARSER message of 0 length\n" );
		break;
	   }
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_APARSER,NULL,NULL);
#endif
     	   msgQSend(pMsgesToAParser, token, len+1, NO_WAIT, MSG_PRI_NORMAL);
	  break;

/*  STARTINTERACT is identical to APARSER, with the exception of
    the error messages.  The message from the host computer tells
    the A-code parser this is an interactive operation.  We mark
    this as a FID display interactive acquisition.  See stopInteract
    for the reason why.							*/

     case STARTINTERACT:
	   token = strchr( msge, '\n' );
	   if (token == NULL) {
		errLogRet( LOGIT, debugInfo, "STARTINTERACT command but no message\n" );
		break;
	   }

	   token++;
	   len = strlen( token );
	   if (len < 1) {
		errLogRet( LOGIT, debugInfo, "STARTINTERACT message of 0 length\n" );
		break;
	   }

	   DPRINT1( 1, "STARTINTERACT will send %s to the A-Parser\n", token );
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_INTERACT,NULL,NULL);
#endif
     	   msgQSend(pMsgesToAParser, token, len+1, NO_WAIT, MSG_PRI_NORMAL);

	   typeInteractiveAcq = FID_DISPLAY;
	  break;

     case AUPDT:
   	   DPRINT(1,"AUPDT\n");
	   token = strchr( msge, '\n' );
	   if (token == NULL) {
		errLogRet( LOGIT, debugInfo, "AUPDT command but no message\n" );
		break;
	   }

	   token++;
	   len = strlen( token );
	   if (len < 1) {
		errLogRet( LOGIT, debugInfo, "AUPDT message of 0 length\n" );
		break;
	   }
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_AUPDT,NULL,NULL);
#endif
     	   msgQSend(pMsgesToAupdt, token, len+1, NO_WAIT, MSG_PRI_NORMAL);
	  break;

     case HALTACQ:    /* equivilent of SA on CT */
   	   DPRINT(1,"HALTACQ\n");
#ifdef XXX
           /* Enable End-of-Transient SW2 interrupt so that lastest data will go to Host */
	   SA_Mod = 1L;
	   SA_Criteria = EXP_HALTED;
	   fifoItrpEnable(pTheFifoObject, SW2_I );
#endif
	   GenericException.exceptionType = EXP_HALTED;
	   GenericException.reportEvent = EXP_HALTED;
	   /* send Abort to exception handler task, it knows what to do */
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_HALTACQ,NULL,NULL);
#endif
	   msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
				sizeof(EXCEPTION_MSGE), 
					NO_WAIT, MSG_PRI_URGENT);
	  break;

     case ABORTACQ:
   	   DPRINT(1,"ABORTACQ\n");
	   GenericException.exceptionType = EXP_ABORTED;
	   GenericException.reportEvent = EXP_ABORTED;
	   /* send Abort to exception handler task, it knows what to do */
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_ABORTACQ,NULL,NULL);
#endif
	   msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
				sizeof(EXCEPTION_MSGE), 
					NO_WAIT, MSG_PRI_URGENT);
		break;

     case STATINTERV:
	   {
		int	interval;

		token = strchr( msge, '\n' );
		if (token == NULL) {
			errLogRet( LOGIT, debugInfo, "STATINTERV command but no message\n" );
			break;
		}

		token++;
		len = strlen( token );
		if (len < 1) {
			errLogRet( LOGIT, debugInfo, "STATINTERV message of 0 length\n" );
			break;
		}

		interval = atoi( token );
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_STATINTERV,NULL,NULL);
#endif
		set_new_interval( interval );
	   }
	  break;

     case STARTLOCK:
	   {
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_STARTLOCK,NULL,NULL);
#endif
		startLock();
	   }
	  break;

     case GETINTERACT:
	   {
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_XPARSER,NULL,NULL);
#endif
		getInteract();
	   }
	  break;

     case STOPINTERACT:
	   {
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_XPARSER,NULL,NULL);
#endif
		stopInteract();
	   }
	  break;

     case STOP_ACQ:
	   {
	     token = strchr( msge, '\n' );
	     if (token == NULL) 
	     {
		errLogRet( LOGIT, debugInfo, 
			"STOP_ACQ command but no message\n" );
			break;
	     }

	     token++;
	     len = strlen( token );
	     if (len < 1) {
		errLogRet( LOGIT, debugInfo, "STOP_ACQ message of 0 length\n" );
		break;
	     }

	     SA_Criteria = atoi( token );

	     token = strchr( token, ' ' );
	     token++;

	     SA_Mod = atol( token );

#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_STOPACQ,NULL,NULL);
#endif
	     /* if EOS SA then enable End-of-Transient SW2 interrupt */
	     if (SA_Criteria == CT_CMPLT)
	     {
	          fifoItrpEnable(pTheFifoObject, SW2_I );
             }
   	     DPRINT2(1,"STOP_ACQ, SA Criteria: %d, Mod: %lu\n",
		SA_Criteria,SA_Mod);
	   }
	  break;

     case ACQDEBUG:
	  {
             int debugval;

	     token = strchr( msge, '\n' );

	     if (token == NULL) 
	     {
		errLogRet( LOGIT, debugInfo, 
			"ACQDEBUG command but no message\n" );
			break;
	     }

	     token++;
	     len = strlen( token );
	     if (len < 1) {
		errLogRet( LOGIT, debugInfo, "ACQDEBUG message of 0 length\n" );
		break;
	     }
	     debugval = atoi( token );
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_ACQDEBUG,NULL,NULL);
#endif
	     if (debugval > 9)  /* acqdebug(2x) for autoamtion brd */
	     {
		debugval -= 10;	/* 10 - 19 -> 0 - 9 for automation */
	        autoSetDebugLevel(pTheAutoObject,debugval);
	     }
	     else
	     {
	        DebugLevel = debugval;
	     }
   	     DPRINT1(-1,"ACQDEBUG: DebugLevel = %d\n",DebugLevel);
	  }
	  break;

     case HEARTBEAT:
	  {
	    long	ival,ival2;
	    long	imsg[CONSOLE_MSGE_SIZE/sizeof( long )];
		
            DPRINT(2,"HEARTBEAT:");
	    token = strchr( msge, '\n' );
	    if (token == NULL) {
		errLogRet( LOGIT, debugInfo, "ECHO command but no message\n" );
		break;
	    }

	    token++;			/*  Move address past the new-line */
	    len = strlen( token );
	    if (len < 1) {
		errLogRet( LOGIT, debugInfo, "ECHO message of 0 length\n" );
		break;
	    }

	    ival = atoi( token );
	    while ('0' <= *token && *token <= '9') {
			token++;
			len--;
	    }

            token++;                    /*  Move address past the new-line */
	    ival2 = atoi( token );
	    imsg[ 0 ] = ival;
	    imsg[ 1 ] = ival2;
	    imsg[ 2 ] = 0;
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_HEARTBEAT,NULL,NULL);
#endif
	    msgQSend(pMsgesToHost, (char *) &imsg[ 0 ], 3 * sizeof(int) + 1,
			 NO_WAIT, MSG_PRI_URGENT);
	  }
	  break;

     case GETSTATBLOCK:
	  {
	    char	xmsg[ 12 ];
	    int		len;

	    sprintf( &xmsg[ 0 ], "1,%d", GETSTATUS );
	    len = strlen( &xmsg[ 0 ] );
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_GETSTATBLOCK,NULL,NULL);
#endif
	    msgQSend(pMsgesToXParser, &xmsg[ 0 ], len+1, NO_WAIT, MSG_PRI_NORMAL);
	  }
	  break;

     case ABORTALLACQS:
	  {

    /*  BOOT_CLEAR provides for an automatic restart with memory cleared.
        The system attempts to reload the VxWorks kernel and execute any
        startup script, as provided for in the PROM parameters.

#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_ABORTALLACQS,NULL,NULL);
#endif
	     /* send reset to automation card, rebooting it */
	     /* autoReset(pTheAutoObject, AUTO_RESET_ALL); */
	     autoReset(pTheAutoObject, AUTO_RESET_332 | AUTO_RESET_AP);
	     reboot( BOOT_CLEAR );
	  }
	  break;

     case OK2TUNE:
	  {
             DPRINT(1, "Host says it's OK to tune\n" );
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_OK2TUNE,NULL,NULL);
#endif
	     if (pTheTuneObject != NULL)
	       semGive( pTheTuneObject->pSemOK2Tune );
	  }
	 break;

     case ROBO_CMD_ACK:
	   {
             int cmdAck;
	     token = strchr( msge, '\n' );
	     if (token == NULL) 
	     {
		errLogRet( LOGIT, debugInfo, 
			"ROBO_CMD_ACK command but no status value, report Timeout\n" );
			break;
                cmdAck = SMPERROR + SMPTIMEOUT;  /* default to timeout */
	     }
	     else
	     {
	       token++;			/*  Move address past the new-line */
	       len = strlen( token );
               /* DPRINT1(0,"ROBO_CMD_ACK: status length: %d\n",len); */
	       if (len < 1) 
               {
		   errLogRet( LOGIT, debugInfo, 
		      "ROBO_CMD_ACK: status  of 0 length, report Timeout\n" );
                   cmdAck = SMPERROR + SMPTIMEOUT;  /* default to timeout */
	       }
	       else
               {
                  /* DPRINT1(0,"ROBO_CMD_ACK: status str value: '%s' \n",token); */
	     	  cmdAck  = atoi( token );
	       }
	    }

            DPRINT1(0,"ROBO_CMD_ACK: status value: %d \n",cmdAck);
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_ROBO_ACK,NULL,NULL);
#endif
	    rngBufPut(pSyncActionArgs,(char*) &cmdAck,sizeof(cmdAck));
	    semGive(pTheAcodeObject->pSemParseSuspend);
           }
	 break;

     case CONSOLEINFO:
	  {
	   int subCmd;

   	   DPRINT(1,"CONSOLEINFO\n");
	   token = strchr( msge, '\n' );
	   if (token == NULL) {
		errLogRet( LOGIT, debugInfo, "CONSOLEINFO command but no message\n" );
		break;
	   }

	   token++;
	   len = strlen( token );
	   if (len < 1) {
		errLogRet( LOGIT, debugInfo, "CONSOLEINFO message of 0 length\n" );
		break;
	   }

           errLogRet( LOGIT, debugInfo, "console info sub-command is %s\n", token );
	   subCmd = atoi( token );
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_CONSOLEINFO,NULL,NULL);
#endif
	   if (subCmd == 1)
	     checkStack( 0 );
	   else
	     i( 0 );
           errLogRet( LOGIT, debugInfo, "console info command completes\n" );
	  }
         break;

     case DOWNLOAD:
	  {
           char *tptr, *argptr, *vmeaddr;
	   int   dwnldsize;

   	   DPRINT(1,"DOWNLOAD\n");
	   token = strchr( msge, '\n' );
	   if (token == NULL) {
		errLogRet( LOGIT, debugInfo, "DOWNLOAD command but no message\n" );
		break;
	   }

	   token++;
	   len = strlen( token );
	   if (len < 1) {
		errLogRet( LOGIT, debugInfo, "DOWNLOAD message of 0 length\n" );
		break;
	   }

           tptr=strtok_r(token,",",&argptr);
	   vmeaddr = (char *) htol( tptr );
           tptr=strtok_r(argptr,",",&argptr);
	   dwnldsize = atol( tptr );

#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_DOWNLOAD,NULL,NULL);
#endif
	   receiveVmeDownload( chanId, vmeaddr, dwnldsize );
	  }
         break;

/* Currently if the STARTFIDSCOPE command is received before the interactive
   acquisition has proceeded to the point that the STM object references the
   interactive uplinker message queue, the non-interactive message queue will
   receive the message and the start FIDscope command will be lost.		*/

     case STARTFIDSCOPE:
	  {
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_STARTFIDSCOPE,NULL,NULL);
#endif
		startFidScope();
	  }
         break;

     case STOPFIDSCOPE:
	  {
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_STOPFIDSCOPE,NULL,NULL);
#endif
		stopFidScope();
	  }
         break;

     case GETCONSOLEDEBUG:
	  {
	   int index;

   	   DPRINT(1,"CONSOLEINFO\n");
	   token = strchr( msge, '\n' );
	   if (token == NULL) {
		errLogRet( LOGIT, debugInfo, "GETCONSOLEDEBUG command but no message\n" );
		break;
	   }

	   token++;
	   len = strlen( token );
	   if (len < 1) {
		errLogRet( LOGIT, debugInfo, "GETCONSOLEDEBUG message of 0 length\n" );
		break;
	   }

	   index = atoi( token );
#ifdef INSTRUMENT
     	   wvEvent(EVENT_MONITOR_CMD_GETCONSOLEDEBUG,NULL,NULL);
#endif
	   sendConsoleDebug( index );
	  }
         break;

     default:
         errLogRet(LOGIT,debugInfo, "Host Cmd: %d Unknown.\n",cmd);
         break;
   }
   return;
}

/*************************************************************
*
*  hostagent - Wait for Messages to be routed to the Host 
*   Receives Messages from its well known msgQ and routes it
*   to the Host Computer  (Expproc)
*
*					Author Greg Brissey 10-6-94
*/
hostagent(int chanId, MSG_Q_ID msges)
{
   char  msge[CONSOLE_MSGE_SIZE];
   int *val;
   int bytes;

   DPRINT(1,"HostAgent 1: Server LOOP Ready & Waiting.\n");

/*   Host Agent waits for the Sendproc channel to complete its connection.
     Thus when the host agent sends a console configuration block requesting
     a DSP download, the Sendproc channel will be ready to download DSP.    */

   bytes = wait4ChanConnect( SENDPROC_CHANNEL );
   FOREVER
   {
     memset( &msge[ 0 ], 0, sizeof( msge ) );
     bytes = msgQReceive(pMsgesToHost, &msge[ 0 ], CONSOLE_MSGE_SIZE, WAIT_FOREVER);
     DPRINT1(2,"hostAgent: recv: %d bytes.\n",bytes);
     bytes = phandlerWriteChannel(chanId,&msge[ 0 ],CONSOLE_MSGE_SIZE);
     DPRINT1(2,"hostAgent: wrote: %d bytes.\n",bytes);
     if (bytes < 0)
     {
         errLogSysRet(LOGIT,debugInfo, "write to Host failed");
         break;
     }
   } 
/*
   closeChannel( chanId );
*/
}


int
calc_new_interval( int interval )
{
	return( (interval * sysClkRateGet() + UNITS_PER_SEC - 1) / UNITS_PER_SEC );
}

void
set_new_interval( int interval )
{
	statblock_coordinator.currentTicks =
		 (interval * sysClkRateGet() + UNITS_PER_SEC - 1) / UNITS_PER_SEC;
	DPRINT1( 0, "New number of ticks in the stat block coordinator: %d\n",
		 statblock_coordinator.currentTicks
	);
	semGive( statblock_coordinator.newValueTicks );
}

update_acqstate( int acqstate)
{
	currentStatBlock.stb.Acqstate = acqstate;
}

get_acqstate()
{
	return( (int) currentStatBlock.stb.Acqstate );
}

getstatblock()
{
   /* Giving this semaphore unblocks the status agent task, which will then
      send the current status block to the host comuter immediately.        */

	semGive( statblock_coordinator.newValueTicks );
}


static void
updateCtFidCnt()
{
	short   tagReg;
	int	ctCntValid;
	long	ntCnt;

	ctCntValid = 1;
	tagReg = stmTagReg( pTheStmObject );
	DPRINT2( DEBUG_CT_ELEM, "current tag: %d, last tag: %d\n",
		      tagReg, pTheStmObject->currentTag );
	if (tagReg != pTheStmObject->currentTag) {
		FID_STAT_BLOCK	*curFidBlock;

		curFidBlock = stmTag2StatBlk( pTheStmObject, tagReg );
		if (curFidBlock == NULL) {
			DPRINT1( DEBUG_CT_ELEM,
			        "For tag %d, no FID status block\n", tagReg );
			currentStatBlock.stb.AcqCtCnt = 0;
			ctCntValid = 0;
			pTheStmObject->currentTag = -1;
		}
		else if (curFidBlock->fidAddr != NULL &&
			 curFidBlock->doneCode != NOT_ALLOCATED) {
			pTheStmObject->currentTag = tagReg;
			pTheStmObject->currentNt = curFidBlock->nt;
			currentStatBlock.stb.AcqFidCnt = curFidBlock->elemId;
			DPRINT1( DEBUG_CT_ELEM,
			        "setting FID element to %d\n",
				 currentStatBlock.stb.AcqFidCnt );
		}
		else {
			DPRINT1( DEBUG_CT_ELEM,
			        "For tag %d, FID status block no longer valid\n",
				 tagReg );
			currentStatBlock.stb.AcqCtCnt = 0;
			ctCntValid = 0;
			pTheStmObject->currentTag = -1;
		}
	}

	if (currentStatBlock.stb.AcqFidCnt > 0 && ctCntValid) {
		ntCnt = stmNtCntReg( pTheStmObject );
		currentStatBlock.stb.AcqCtCnt = pTheStmObject->currentNt - ntCnt;
	}
	else
	  currentStatBlock.stb.AcqCtCnt = 0;

	DPRINT2( DEBUG_CT_ELEM, "CT count is %d, FID # is %d\n",
		      currentStatBlock.stb.AcqCtCnt,
		      currentStatBlock.stb.AcqFidCnt );
}

statusagent()
{
	int	curAcqState;
	int	lklevel;

	currentStatBlock.stb.AcqOpsComplCnt = 0;
	currentStatBlock.stb.AcqCtCnt = -1;
	statblock_coordinator.newValueTicks = semBCreate( SEM_Q_PRIORITY, SEM_EMPTY );
	set_new_interval( DEFAULT_INTERVAL );
	DPRINT(1, "Stat Block Agent, server LOOP Ready and Waiting.\n");

	FOREVER {
		semTake(
		    statblock_coordinator.newValueTicks, 
		    statblock_coordinator.currentTicks
		);
		/*errLogRet( LOGIT, debugInfo, "semTake returned in the status agent\n" );*/

           /* The programs between her and the msgQSend statement fill in those
	      parts of the stat block we want filled in at the same time the stat
	      block is sent.  Most fields are computed or filled in by a separate
	      task, the stat monitor.  The Fid or element count is set by the
	      uplinker.  The CT count must be filled in here.  This is required
              because effectively the host computer gets it from two sources, here
	      and from the uplinker.  For as each element completes, the host
	      computes a value for the current CT count.  Prior to this change,
              an incorrect CT count could subsequently be displayed, since the
	      status agent could send a stat block with an out-of-date CT count.  */

	   /* I get the acquisition state into a local variable in case we want
	      to compare it with more than one value.

	      At this time we only obtain the CT count if a non-interactive acquisition
 	      is in progress.  If lock display or FID display from ACQI are running,
	      the state is ACQ_INTERACTIVE.						*/

		curAcqState = get_acqstate();
		if (curAcqState == ACQ_ACQUIRE) {
			updateCtFidCnt();
		}
		else
		  currentStatBlock.stb.AcqCtCnt = 0;
		lklevel = autoLkValueGet( pTheAutoObject );
		lklevel = lklevel >> LOCK_ADC_RSHIFT;
		if (lklevel < 0)
		  lklevel = 0;
		DPRINT1( 2, "locklevel is %d\n", lklevel );
		setLockLevel( lklevel );

	   /* A value of -1 for the AcqCtCnt tells the host the value is
	      no good and should be ignored.  A value of 0 confuses the
	      host; it thinks CT == NT and the element has completed.	*/

		currentStatBlock.dataTypeID = STATBLK;
		msgQSend(pMsgesToHost, (char *) &currentStatBlock, sizeof( currentStatBlock ),
			 NO_WAIT, MSG_PRI_NORMAL);
	}
}

#define STATMONITOR_DELAY_SECONDS	5

#define HSL_DEC_MASK	0x0080
#define LSDV_DEC_MASK	0x0600
#define LSDV_DEC_SHIFT	9

/*  Naive version.

    The currentTicks field in the statblock coordinator is initally 0.
    The obtaining of lock level is not coordinated with the sending of
    the status block to the console.  Under the worst scenerio, when
    you start the ACQI shim display, it could take upto 5 seconds for 
    the lock level to start updating more quickly.

    We do not call autoLkLevelGet in the statusagent, since the former
    takes semaphores and waits for interrupts and could thus disrupt
    the timing of the status agent, which is expected to send back stat
    blocks quickly at times, for example, during shim display.  Use of
    a separate task allows autoLkLevelGet to block it without disrupting
    the rest of the console software.					*/

/* automation heart beat values, should be change is MSR is alive */
static unsigned long autoHB1 = 0L;	
static unsigned long autoHB2 = 0L;

statmonitor()
{
	short	LSDVword;
	int	spinactive, curHSLinesDec;
	int	curAcqState;

	setLSDVword( 0 );

	FOREVER {
		/*taskDelay( statblock_coordinator.currentTicks );*/
		taskDelay( STATMONITOR_DELAY_SECONDS * sysClkRateGet()  );

/*  Automation board returns spin rate in 10ths of a Hertz.
    Convert value in stat block to Hertz.			*/

	if (getSpinType() == -1)
        {
           setSpinAct( -2 );
        }
        LSDVword = autoLSDVget( pTheAutoObject );

        setLSDVword( LSDVword );

/*    Now VT task takes care of this 7/16/97 */
#ifdef XXXX
		/* Handle VT status & Interlock Failure */
		if ( getVTtype() != -1)   /* if we have a VT,  check it */
	        {
		   int temp;
		   /* Getting the Temp takes a large amount ot time (100s of msec) */
                   temp = getVTtemp();
                   /* DPRINT1(0,"statmonitor: temp: %d\n",temp); */
                   if ( (temp == 98 /* TIMEOUT */ ) || (temp == -10000 /*CMDTIMEOUT*/) )
                   {
                     currentStatBlock.stb.AcqVTAct = (short) 30000 /* VTOFF */;
                   }
	           else
		   {
                     currentStatBlock.stb.AcqVTAct = (short) temp;
		   }
		   setVT_LSDVbits();
		   /* should we report VT out of Reg. ? */
		   if (getVTinterLk() == 2 /* Ready to Check */) 
		   {
                     DPRINT1(0,"statmonitor(): VTchk: %d\n",VTchk());
		     if ( VTchk() != 1 )
                     {
		        /* disable just VT interlock tests if it is a Warning */
			setVTinterLk(0);
			if (getVTErrMode() == HARD_ERROR)  /*(15)  Error */
		        {
		          /* disable all interlock tests, since this will stop the exp (for Error), don't want
			     continue error coming back while no exp. is running */
			  setLKInterlk(0);
			  setSpinInterlk(0);
       			  GenericException.exceptionType = HARD_ERROR;  
			}
			else  /* WARNING */
			{
       			  GenericException.exceptionType = WARNING_MSG;  
			}
       			GenericException.reportEvent = acqerrno; /* VTERROR + VTREGFAIL; */
    			/* send error to exception handler task */
    			msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   		sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
		     }
		   }
		}
	        else
                {
                     /* DPRINT(0,"statmonitor: No VT Object\n"); */
		     setVT_LSDVbits();
                     currentStatBlock.stb.AcqVTAct = (short) 30000 /* VTOFF */;
                     currentStatBlock.stb.AcqVTSet = (short) 30000 /* VTOFF */;
                }
#endif

	        /* should we report Lock out of Reg. ?, i.e. in='y' */
		if (getLKInterlk() != 0) 
	        {
		  /*
		   DPRINT1(0,"Check lock: %s\n",
			(autoLockSense( pTheAutoObject ) == 1) ? "LOCKED" : "NOT LOCKED");
		  */
		   if ( autoLockSense( pTheAutoObject ) != 1)
	           {
		        /* disable just Lock interlock tests if it is a Warning */
			setLKInterlk(0);

			if (getLkErrMode() == HARD_ERROR)  /*(15) Error */
		        {
		          /* disable all interlock tests, since this will stop the exp, don't want
			     continue error coming back while no exp. is running */
			  setVTinterLk(0);
			  setSpinInterlk(0);
       			  GenericException.exceptionType = HARD_ERROR;  
			}
			else  /* WARNING */
			{
       			  GenericException.exceptionType = WARNING_MSG;  
			}
       			GenericException.reportEvent = SFTERROR + LOCKLOST;
    			/* send error to exception handler task */
    			msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   		sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
	           }
		}
	        /* should we report Spinner out of Reg. ?, i.e. in='y' */
		if (getSpinInterlk() == 2) 
	        {
		  /*
		   DPRINT1(0,"Check spin: %s\n",
			(autoSpinReg( pTheAutoObject ) == 1) ? "REGULATED" : "NOT REGULATED");
		  */
		   if (autoSpinReg( pTheAutoObject ) != 1)
	           {
			setSpinInterlk(0);
			DPRINT1(0,"setSpinErrMode: %d, (1-Err,2-Warning)\n",getSpinErrMode());
			if (getSpinErrMode() == HARD_ERROR)  /* (15) Error */
		        {
		          /* disable all interlock tests, since this will stop the exp, don't want
			     continue error coming back while no exp. is running */
			  setVTinterLk(0);
			  setLKInterlk(0);
       			  GenericException.exceptionType = HARD_ERROR;  
			}
			else  /* WARNING */
			{
       			  GenericException.exceptionType = WARNING_MSG;  
			}
       			GenericException.reportEvent = SPINERROR + SPINOUT;
    			/* send error to exception handler task */
    			msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   	    sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
	           }
	        }

		curHSLinesDec = (fifoGetHsl( pTheFifoObject, STD_HS_LINES ) & HSL_DEC_MASK);
		clearLSDVbits( LSDV_DEC_MASK );
		if (curHSLinesDec != 0)
		  setLSDVbits( 1 << LSDV_DEC_SHIFT );

                /* check MSR Heart Beat, but only if MSR is there and the check interval >= 2 sec */ 
                if ( (pTheAutoObject != NULL) && (pTheAutoObject->autoBaseAddr != 0xFFFFFFFF) && 
		     (statblock_coordinator.currentTicks > 120) )
                {
		  /* Time to check Heart Beat of MSR, is it Alive or Dead ? */
	          if (autoHB1 == 0L)
		  {
		    autoHB1 = autoGetHeartBeat(pTheAutoObject);
		  }
		  else
		  {
		    autoHB2 = autoGetHeartBeat(pTheAutoObject);
		    if (autoHB1 != autoHB2) /* Hey MSR is OK */
		    {
			  autoHB1 = autoHB2;
		    }
		    else	/* R.I.P. */
		    {
       			GenericException.exceptionType = WARNING_MSG;  
       			GenericException.reportEvent = WARNINGS + AUTO_NOTBOOTED;
    			/* send error to exception handler task */
    			msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   	    sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
			DPRINT2(-1,"WARNING: MSR Heart Beat Lost: HB1: %lu, HB2: %lu\n",autoHB1,autoHB2);
		    }
		  }
	        }

	}
}
