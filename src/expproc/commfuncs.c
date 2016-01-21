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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdarg.h>

#define  DELIMITER_2      '\n'

#include "sockets.h"
#include "hostMsgChannels.h"
#include "hostAcqStructs.h"
#include "msgQLib.h"
#include "eventHandler.h"
#include "chanLib.h"
#include "commfuncs.h"
#include "errLogLib.h"

#ifndef DEBUG_HEARTBEAT
#define DEBUG_HEARTBEAT	(9)
#endif

#define TRUE 1
#define FALSE 0
#define FOR_EVER 1
#define HEARTBEAT_TIMEOUT_INTERVAL	(2.8)

extern MSG_Q_ID pRecvMsgQ;

int chanId;
int chanIdSync;		/* Expproc's updating socket to console */

static int Chan_Num = - 1;
static int Heart1stTime = 1;
static int HeartBeatReply = -1;
static int doHeartBeat = 0;
void setupHeartBeat();

#define WALLNUM 2
static char *wallpaths[] = { "/usr/sbin/wall", "/usr/bin/wall" };
static char wallpath[25] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static int shpid;

static struct timeval  hbtime;		/* time that the last heartbeat */
					/* message sent to the console  */
static double
diff_hbtime( struct timeval *newtime )
{
	int	diffsec, diffusec;
	double	dval;

	diffsec = newtime->tv_sec - hbtime.tv_sec;
	diffusec = newtime->tv_usec - hbtime.tv_usec;
	if (diffusec < 0) {
		diffsec--;
		diffusec += 1000000;
	}
	dval = (double) (diffsec) + ((double) (diffusec)) / 1.0e6;

	return( dval );
}

/*  print_hbtime_diff should only be called if DebugLevel is high
    enough that the debug output is to be printed.  Therefore we
    can use -1 as the "debug level" in the DPRINTn macro	*/

void print_hbtime_diff( char *msg, struct timeval *newtime )
{
	DPRINT2( -1, "%s: heartbeat time difference: %g\n",
		      msg, diff_hbtime( newtime )  );
}

/**************************************************************
*
*  shutdownComm - Close the Message Q , DataBase & Channel 
*
* This routine closes the message Q , DataBase and Channel
*  
* RETURNS:
* MSG_Q_ID , or NULL on error. 
*
*       Author Greg Brissey 8/4/94
*/
void shutdownComm(void)
{
     deleteMsgQ(pRecvMsgQ);
     closeChannel(chanId);
     if (chanIdSync != -1)
	closeChannel(chanIdSync);
}

/**************************************************************
*
*  chanConItrp - SIGALRM interrupt routine, re-attempts connect
*
*  
* RETURNS:
* void 
*
*       Author Greg Brissey 9/14/94
*/
static void
chanConItrp(int signal)
{

  /* Place the SIGUSR1 interrupt & msgID onto the eventQ, the non-
    interrupt function (processMsge) will be called with msgId as an argument */

  processNonInterrupt( SIGALRM, (void*) Chan_Num );
  return;
}
static void
chanErrItrp(int signal)
{

  /* Place the SIGUSR1 interrupt & msgID onto the eventQ, the non-
    interrupt function (processMsge) will be called with msgId as an argument */

  processNonInterrupt( SIGPIPE, (void*) Chan_Num );
  return;
}

/**************************************************************
*
*  chanConRetry - Routine retry connecting to channel 
*  In responce to SIGALRM this routine continues to attempt
*  to connect to a channel. When connection is achieved
*  the SIGALRM is turned off and unregistered with the eventHandler
*
*  
* RETURNS:
* void 
*
*       Author Greg Brissey 9/14/94
*/
static void
chanConRetry(int chan_no)
{
  int result;
  result = connectChannel( chan_no );
  if (result != -1)
  {
    setRtimer(0.0,0.0);
    unregisterAsyncHandlers(SIGALRM);      /* timer signal */
    chanId = chan_no;			   /* update chanId */
  }
  return;
}
/**************************************************************
*
*  initiateChan - Initialize a Channel to the Console
*
* This routine initializes a Channel to the Console.
* If no reponce from console, keep trying several times before
* giving up.
*  
* RETURNS:
* MSG_Q_ID , or NULL on error. 
*
*/
int initiateChan(int chan_no)
/* int chan_no - channel to open */
/* int xloops - number of trys before returning failure */
{
  int ival;
  closeChannel( chan_no );   /* just to be sure */
  Chan_Num = chan_no;
  ival = openChannel( chan_no, 0, 0 );
  if (ival != chan_no)
  {
     DPRINT2(1,"initiateChan :openChannel() on channel %d returned %d\n",chan_no,ival);
     closeChannel(chan_no);
     return(-1);
  }

  ival = connectChannel( chan_no );
  if (ival != 0)
  {
    DPRINT1(1,"initiateChan :initial connect failed, backgrounding connect for chan %d\n",
	chan_no);
    /* here we register both the signal handler to be called
       and the non-interrupt function to handle re-establishing 
       connection */

    registerAsyncHandlers(
                          SIGALRM,      /* timer signal */
			  chanConItrp,
			  chanConRetry
                         );
    setRtimer(5.0,5.0);
    chan_no = -1;
  }
  return(chan_no);
}  


/**************************************************************
*
*  connectChan - connect a Channel to the Console
*
* This routine connects a Channel to the Console.
*  
* RETURNS:
* Channel fd , or -1 on error. 
*
*/
int connectChan(int chan_no)
/* int chan_no - channel to open */
/* int xloops - number of trys before returning failure */
{
  int ival;
  closeChannel( chan_no );   /* just to be sure */
  /* Chan_Num = chan_no; */
  ival = openChannel( chan_no, 0, 0 );
  if (ival != chan_no)
  {
     DPRINT2(1,"connectChan :openChannel() on channel %d returned %d\n",chan_no,ival);
     closeChannel(chan_no);
     return(-1);
  }

  ival = connectChannel( chan_no );
  if (ival != 0)
  {
    DPRINT1(1,"connectChan : connect failed for chan %d\n",
	chan_no);
    chan_no = -1;
  }
  return(chan_no);
}  

/* define this dummy for all other processes except Expproc */
#ifndef NODUMMY
void processChanMsge()
{
}
void resetExpproc()
{
}
int isExpActive()
{
   return(1);
}
#endif

/**************************************************************
*
*  chanAsyncConRetry - Routine retry connecting to channel 
*  In responce to SIGALRM this routine continues to attempt
*  to connect to a channel. When connection is achieved
*  the SIGALRM is turned off and unregistered with the eventHandler
*  Then the channel is set to be Async.
*
*  
* RETURNS:
* void 
*
*       Author Greg Brissey 10/4/94
*/
#ifdef TODO
static void
chanAsyncConRetry(int chan_no)
{
  extern void processChanMsge();
  int result;
  result = connectChannel( chan_no );
  if (result != -1)
  {
    setRtimer(0.0,0.0);
    unregisterAsyncHandlers(SIGALRM);      /* timer signal */
    chanId = chan_no;			   /* update chanId */
    result =  registerChannelAsync( chan_no, processChanMsge);
    if (result != 0)
    {
       errLogSysRet(ErrLogOp,debugInfo,
	    "initiateChan: registerChannelAsync() failed. Terminating Expproc");
       chan_no = -1;
    }
    Heart1stTime = 1;
    setupHeartBeat();
  }
  return;
}
#endif

static void ignoreSigalrm(int signal)
{
}

/**************************************************************
*
*  initiateAsyncChan - Initialize a Async Channel to the Console
*
* This routine initializes a Channel to the Console.
* If no reponce from console, keep trying several times before
* giving up.
*  
* RETURNS:
* MSG_Q_ID , or NULL on error. 
*
*/
int initiateAsyncChan(int chan_no)
/* int chan_no - channel to open */
{
  extern void processChanMsge();
  void ignoreSigalrm(int);
  int retry, ival;
  retry = 1;
  Chan_Num = chan_no;

  unregisterAsyncHandlers(SIGALRM);      /* timer signal */
  registerAsyncHandlers( SIGALRM,      /* timer signal */
			  ignoreSigalrm,
			  ignoreSigalrm
                         );

  while (retry)
  {
     closeChannel( chan_no );   /* just to be sure */
     ival = openChannel( chan_no, 0, 0 );
     if (ival != chan_no)
     {
        DPRINT2(1,"initiateAsyncChan :openChannel() on channel %d returned %d\n",
		chan_no,ival);
        closeChannel(chan_no);
        sleep(5);
     }
     else
     {
        ival = connectChannel( chan_no );
        if (ival != 0)
        {
           DPRINT1(1,"initiateChan :initial connect failed for chan %d\n",
	          chan_no);
           sleep(5);
        }
        else
        {
            ival =  registerChannelAsync( chan_no, processChanMsge);
            if (ival != 0)
            {
               errLogSysRet(ErrLogOp,debugInfo,
                     "initiateChan: registerChannelAsync() failed.");
            }
            else
            {
               retry = 0;
            }
         }
      }
   }
  Heart1stTime = 1;
  unregisterAsyncHandlers(SIGALRM);      /* timer signal */
  return(chan_no);
}  

int consoleConn()
{
   return( ((chanId > 0) ? 1 : 0) );
}

/*-------------------------------------------------------------------------
|
|   Setup the timer interval alarm
|
+--------------------------------------------------------------------------*/
int setRtimer(double timsec, double interval)
{
    long sec,frac;
    struct itimerval timeval,oldtime;

    sec = (long) timsec;
    frac = (long) ( (timsec - (double)sec) * 1.0e6 ); /* usecs */
    DPRINT2(3,"setRtimer(): sec = %ld, frac = %ld\n",sec,frac);
    timeval.it_value.tv_sec = sec;
    timeval.it_value.tv_usec = frac;
    sec = (long) interval;
    frac = (long) ( (interval - (double)sec) * 1.0e6 ); /* usecs */
    DPRINT2(3,"setRtimer(): sec = %ld, frac = %ld\n",sec,frac);
    timeval.it_interval.tv_sec = sec;
    timeval.it_interval.tv_usec = frac;
    if (setitimer(ITIMER_REAL,&timeval,&oldtime) == -1)
    {
         perror("setitimer error");
         return(-1);
    }
    return(0);
}

/**************************************************************
*
*   deliverMessage - send message to a named message queue or socket
*
*   Use this program to send a message to a message queue
*   using the name of the message queue.  It opens the
*   message queue, sends the message and then closes the
*   message queue.  The return value is the result from
*   sending the message, unless it cannot access the message
*   queue, in which case it returns -1.  Its two arguments
*   are the name of the message queue and the message, both
*   the address of character strings.
*   
*   
*   March 1998:  deliverMessage was extended to send the message
*   to either a message queue or a socket.  It selects the system
*   interface based on the format of the interface argument.
*
*   A message queue will have an interface either like "Expproc"
*   (single word) or "Vnmr 1234" (word, space, number - note
*   this latter form is now obsolete).
*
*   A socket will have an interface like "inova400 34567 1234"
*   (word, space, number, space, number - the word is the hostname,
*   the first number is the port address of the socket to connect
*   to, the second is the process ID).
*
*   Using a format specifier of "%s %d %d" (string of non-space
*   characters, space, number, space, number) it scans this
*   interface argument.  If it can convert three fields, then
*   the interface is a socket; otherwise it is a message queue.
*/

int
deliverMessage( char *interface, char *message )
{
	char		tmpstring[ 256 ];
	int		ival1, ival2;
	int		ival, mlen;
	MSG_Q_ID	tmpMsgQ;

	if (interface == NULL)
	  return( -1 );
	if (message == NULL)
	  return( -1 );
	mlen = strlen( message );
	if (mlen < 1)
	  return( -1 );

    	ival = sscanf( interface, "%s %d %d\n", &tmpstring[ 0 ], &ival1, &ival2 );

/*
*        diagPrint(debugInfo,"Procproc deliverMessage  ----> host: '%s', port: %d,(%d,%d)  pid: %d\n",
*           tmpstring,ival1,0xffff & ntohs(ival1), 0xffff & htons(ival1), ival2);
*/  

	if (ival >= 3) {
		int	 replyPortAddr;
		char	*hostname;
		Socket	*pReplySocket;

		replyPortAddr = ival1;
		hostname = &tmpstring[ 0 ];

		pReplySocket = createSocket( SOCK_STREAM );
		if (pReplySocket == NULL)
		  return( -1 );
		ival = openSocket( pReplySocket );
		if (ival != 0)
                {
		  free( pReplySocket );
		  return( -1 );
                }
                /* vnm port is already in network order, so switch to host order since
                 * sockets.c will switch to network order
                 */
		ival = connectSocket( pReplySocket, hostname, 0xFFFF & htons(replyPortAddr) );
		if (ival == 0)
                {
		   writeSocket( pReplySocket, message, mlen );
                }
		closeSocket( pReplySocket );
		free( pReplySocket );
		return( ival );
	}

	tmpMsgQ = openMsgQ( interface );
	if (tmpMsgQ == NULL)
	  return( -1 );

        ival = sendMsgQ( tmpMsgQ, message, mlen, MSGQ_NORMAL, NO_WAIT );

        closeMsgQ( tmpMsgQ );
	return( ival );
}

/*  See deliverMessage for an explanation of how this works  */

int
verifyInterface( char *interface )
{
	char		tmpstring[ 256 ];
	int		ival1, ival2;
	int		ival;
	MSG_Q_ID	tmpMsgQ;

	if (interface == NULL)
	  return( 0 );

    	ival = sscanf( interface, "%s %d %d\n", &tmpstring[ 0 ], &ival1, &ival2 );

	if (ival >= 3) {
		int	 peerProcId;

		peerProcId = ival2;
		ival = kill( peerProcId, 0 );
		if (ival != 0) {
			if (errno != ESRCH)
			  DPRINT1( -1, "Error accessing PID %d\n", peerProcId );
			return( 0 );
		}
		else
		  return( 1 );
	}
	else {
		tmpMsgQ = openMsgQ( interface );
		if (tmpMsgQ == NULL)
		  return( 0 );

	        closeMsgQ( tmpMsgQ );
		return( 1 );
	}
}

void controlHeartBeat(int val)
{
   doHeartBeat = val;
}

void setHeartBeat()
{
   HeartBeatReply = 1;
}

/**************************************************************
*
*  HeartBeat - Routine to send heartbeat reply message to console
*  In responce to SIGALRM this routine sends message to console 
*  to which it replys. If This routine is called prior to the reponce
*  of the preceding heartbeat reply message the console is assumed off-line
*
*  
* RETURNS:
* void 
*
*/
static void
HeartBeat(int chan_no)
{
  char Msge[CONSOLE_MSGE_SIZE];
  if ( doHeartBeat && consoleConn() )
  {
     if ( ! isExpActive() )
     {
	DPRINT2(2,"Check HeartBeat Reply: 1st time = %d, HBReply = %d\n",
			Heart1stTime,HeartBeatReply);
        if ( (HeartBeatReply != 1) && (Heart1stTime != 1) )
        {
	   struct timeval newtime;

	   gettimeofday( &newtime, NULL );
	   if (diff_hbtime( &newtime ) < HEARTBEAT_TIMEOUT_INTERVAL)
	   {
	      return;
	   }
	   if (DebugLevel >= DEBUG_HEARTBEAT)
	     print_hbtime_diff( "No heartbeat reply", &newtime );
	   DPRINT(0,
	    "No HeartBeat Reply, Restarting Expproc, Sendproc, Recvproc\n");
	   resetExpproc();
        }
	else
	{
	   HeartBeatReply = 0;
	   sprintf( &Msge[ 0 ], "%d%c%d%c%d",
			   HEARTBEAT, DELIMITER_2, CASE, DELIMITER_2, HEARTBEAT_REPLY );
	   gettimeofday( &hbtime, NULL );
           writeChannel(chanId, &Msge[ 0 ], CONSOLE_MSGE_SIZE);
	   Heart1stTime = 0;
	}
     }
  }
  return;
}
static void
FixPipe(int chan_no)
{
   DPRINT(0, "Failure communicating with acquisition\n");
   resetExpproc();
  return;
}
/**************************************************************
*
*  setupHeartBeat - Initialize the heart beat 
*
***************************************************************/
void setupHeartBeat()
{
   /* here we register both the signal handler to be called
      and the non-interrupt function to handle re-establishing 
      connection */
     registerAsyncHandlers(
                          SIGALRM,      /* timer signal */
			  chanConItrp,
			  HeartBeat
                         );
      setRtimer(3.0,3.0);
   /* here we register both the signal handler to be called
      and the non-interrupt function to handle re-establishing 
      connection in case of error */
     registerAsyncHandlers(
                          SIGPIPE,      /* socket error signal */
			  chanErrItrp,
			  FixPipe
                         );
}

/*-----------------------------------------------------------------------
|
| findwall - search several pathes to find the wall command
|
+------------------------------------------------------------------------*/
static int findwall(path)
char *path;
{
    int i;
 
    for (i=0;i<WALLNUM;i++)
    {
        if (access(wallpaths[i],X_OK) != -1)
        {
            strcpy(path,wallpaths[i]);
            return(0);
        }   
    }    
    strcpy(path,"");
    return(-1);
}
 

/***********************************************************************
* wallMsge
*   Execute the UNIX wall command with the given message
*
* WARNING: 
*   Processes using this function must catch the SIGCHLD of this forked shell
*/
int wallMsge(char *fmt, ...)
{
   va_list  vargs;
   char msge[512];
   char cmd[1024];

   va_start(vargs, fmt);
   vsprintf(msge, fmt, vargs);
   va_end(vargs);

   if ((int)strlen(wallpath) < 2)
   {
      findwall(wallpath);
      DPRINT1(3,"wallMsge: find path = '%s'\n",wallpath);
      if ((int)strlen(wallpath) < 2)
         return(0);
   }
#ifdef LINUX
   sprintf(cmd,"echo '%s' | %s",msge,wallpath); /* /usr/bin/wall "echo 'msge'" */
#else
   sprintf(cmd,"echo '%s' | %s -a",msge,wallpath); /* /usr/sbin/wall -a "echo 'msge'" */
#endif

   DPRINT1(1,"wallMsge: cmd = '%s'\n",cmd);
   shpid = vfork();
   if (shpid == 0)   /* if we are the child then exec the sh */
   {
       execl("/bin/sh","sh","-c",cmd,NULL);
       exit(1);
   }
   return 0;
}
