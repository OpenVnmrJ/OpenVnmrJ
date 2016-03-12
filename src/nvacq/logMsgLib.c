/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif

/* 
 */
#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <vxWorks.h>
#include <taskLib.h>
#include <stdio.h>
#include <ioLib.h>
#include <socket.h>
#include <types.h>
#include <in.h>
#include <bootLib.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <pipeDrv.h>
#include <msgQLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <sockLib.h>
#include <intLib.h>

/* #include "/usr/include/syslog.h"	 UNIX syslog defines */
/* #include <syslog.h>	 UNIX syslog defines */

#include "commondefs.h"
#include "logMsgLib.h"
#include "rngLLib.h"
#include "taskPriority.h"

/*
modification history
--------------------
8-24-93,gmb  created 
*/


/*
DESCRIPTION

This library contain the message logging facility. The usage of this facility
is to prevent the typical "out of sequence" messages and task blocking due to the
printing and preemptive scheduling interaction. This interaction typically
results in tasks blocking that would of normally not done so. 
Resulting in erroneous diagnostic output and intertask behavior.

 Three classes of Messages
 1. Informational Application Specific (i.e. I am doing this now, etc. )
 2. Diagnostic Messages for debugging, application specific
 3. Errors:
    A. Application Errors
    B. System Call Errors
 
   Class 1. maybe be just printf(s) or they maybe wish to be logged

   Class 2. Present for debugging but gone in non-debugging version,
            are just fprintf(s).

   Clase 3. A. Should be logged
            B. Most Definitely should be logged.

 This Library provides facilities for Class 2 & 3 messages.

 Debugging messages use the DPRINT style macros to allow
 conditional compiling of print statment and also provides
 a level of verbosity in debug printing.
 
 Logging is provided via the Boot Hosts syslogd UNIX facility.
 The Error routines may log or not log based on calls to
 sysLogEnable() or sysLogDisable().
 The messages are printed on console's stderr. Log messages
 are log and printed according the /etc/syslog.conf file
 and the syslog facility parameter.

 The acquisition console should be consider to be LOG_LOCAL7:
 
   For LOG_LOCAL7 modify /etc/syslog.conf to include
   local7.err;local7.info;local7.emerg;local7.debug        /var/log/console
   local7.err;local7.info;local7.emerg;local7.debug        /dev/console
 
   This tells syslogd to write your messages to the console as well
as the logfile /var/log/vnmr.
 
  After modifing syslog.conf execute a "kill -HUP  syslog's pid"
(found in /etc/syslog.pid)
This will result in syslogd reading the modified syslog.conf file.
 

Note: if the logMsg() facility does not use a large enough queue, since if
 (i.e. messages are being lost. ). Then it's MessageQ size can be increased
 in usrConfig.c in vw/config/all .

 Usage:
    DPRINT(level,"This is a test, No args....\n");
    DPRINT1(level,"This is a test, 1 args..%d..\n",val);
    DPRINT2(level,"This is a test, 2 args..%d.%s.\n",val,str);
    DPRINT3(level,"This is a test, 3 args..%d.%s.%d\n",val,str,val);
    ......
    DPRINT5(level,........)

   or
    diagPrint(debugInfo, format_string, arg1, arg2, ....)

    Note: debugInfo expands to fileNline(__FILE__,__LINE__)
          if DEBUG is defined, otherwise it is NULL.

  OutPut:
 0x326c58 (tRdbRun): 10:07:20 'tstlog.c' line: 57, This is a test, No args....
 0x326c58 (tRdbRun): 10:07:29 'tstlog.c' line: 58, This is a test, 1 args..1..
 0x326c58 (tRdbRun): 10:07:51 'tstlog.c' line: 59, This is a test, 2 args..1.2.
 0x326c58 (tRdbRun): 10:08:47 'tstlog.c' line: 61, This is a test, 3 args..1.2.1

   (the filename and line number are outputted only if DEBUG was defined during
    compilation)

    if (msgQDelete(1L) == ERROR)
    {
      errLogSysRet(LOGIT,debugInfo,"Oh OOOhh: ");
    }

OutPut:
  Console's stdout

0x326c58 (tRdbRun): 10:09:13 'tstlog.c' line: 65, Oh OOOhh:  S_smObjLib_NO_OBJECT_DESTROY
    
  Boot Hosts Console window via syslogd

Jul 29 10:09:55 mv162  0x326c58 (tRdbRun): 10:09:55: 'tstlog.c' line: 70, Oh OOOhh:  S_smObjLib_NO_OBJECT_DESTROY


*/

/*
 *  Priorities (these are ordered) according to UNIX SUNOS 4.1.3
*/
/* #define LOG_EMERG       0        system is unusable */
/* #define LOG_ALERT       1        action must be taken immediately */
/* #define LOG_CRIT        2        critical conditions */
/* #define LOG_ERR         3        error conditions */
/* #define LOG_WARNING     4        warning conditions */
/* #define LOG_NOTICE      5        normal but signification condition */
/* #define LOG_INFO        6        informational */
/* #define LOG_DEBUG       7        debug-level messages */

#define LOG_PRIORTY 100
#define LOG_TASK_OPTIONS 0
#define LOG_STACK_SIZE 2048

#define MAXLINE         1024            /* maximum line length */
 
#define ADDDATE         0x004   /* add a date to the message */
#define DEFPRI          0x1     /* Set default priority */
 
#define SYSLOGPORT      514     /* Hard coded port number - yech*/


#define BOOT_LINE_ADRS  0x700   /* Hard Coded Address, VW told me to do this */

static int lpifd;      /* Log pipe input */
static int lpofd;      /* Log pipe output */
static int SysLogFd = -1;   /* To syslogd */
static struct sockaddr_in logserv;     /* Used for connection to syslogd */


void syslogTask();

int DebugLevel = 0;
int acqerrno;	/* acquisition error number , like unix errno */

static int hiMsgQMark;

static int msgPipe = -1;
static int logTaskId;
static int msgLost = 0;
static int sysLogFd = -1;

static SEM_ID pVaMutex = NULL;	/* mutex to protect va_start, va_arg, va_end usage */
static MSG_Q_ID pMsgLogMsgQ = NULL;

static int logIt(char*);
static int errPrtMsg(char*,int);

#define FILENLINEBUFSIZE 64
#define FILENLINEBUFSIZEM1 63

static char *fileNlineBufPtr = NULL;
static RINGL_ID pLRngId = NULL;
/* extern MSG_Q_ID pMsgLogQ; */

/**************************************************************
*
*  logMsgCreate - Creates a Message logging facility 
*
*
*
* RETURNS:
*  OK or ERROR 
*
*/
int logMsgCreate(int maxEntrys)
/* entrys	Max number of entrys queued */
{
   void msgLogTask();
   int tid;

   /* hiMsgQMark = 0; */
   msgLost = 0;

/*  We create this pipe so we can be like the VxWorks logTask
    pipe it's output to this pipe which can then be transfered
    to the boot Host's syslogd */

    if (pMsgLogMsgQ == NULL)
    {
        pMsgLogMsgQ = msgQCreate(maxEntrys,255,MSG_Q_FIFO);
    }
        /* pMsgLogMsgQ = msgQCreate(500,255,MSG_Q_FIFO);  removed 11/8/05 */
        /* pMsgLogMsgQ = msgQCreate(200,255,MSG_Q_FIFO); */

    /* printf("logMsgCreate: pMsgLogMsgQ: 0x%lx\n",pMsgLogMsgQ); */
    if (pMsgLogMsgQ == NULL)
    {
      perror("msgQCreate()");
      return -1;
    }

    if ( fileNlineBufPtr == NULL)
    {
        int i;
        register int toP;
        char *addrPtr;
        fileNlineBufPtr = (char*) malloc(maxEntrys*FILENLINEBUFSIZE);
        if (pMsgLogMsgQ == NULL)
        {
          perror("msgQCreate()");
          return -1;
        }
        pLRngId = rngLCreate(maxEntrys,"fileNlineBufAddrs"); 
        if (pLRngId == NULL)
        {
          perror("msgQCreate()");
          return -1;
        }
     
        addrPtr = fileNlineBufPtr;
        for(i=0; i < maxEntrys; i++)
        {
            /* printf("msgQCreate: nfilebuf addrs: 0x%lx\n",addrPtr); */
            RNG_LONG_PUT(pLRngId, (long) addrPtr, toP);
            addrPtr += FILENLINEBUFSIZE;
        }
    }

    if (pVaMutex == NULL)
    {
       pVaMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
			     SEM_DELETE_SAFE);
    }

#ifdef USE_PIPE_IS_NOLONG_RECOMMENDED_BY_WRS_4_ISR    /* GMB 2/01/2005 */
    if (ERROR == pipeDevCreate("/pipe/msgLog",200,255)) {
        perror("pipeDevCreate");
        exit(1);
    }
 
    if (ERROR == (msgPipe = open("/pipe/msgLog",WRITE,0))) {
        perror("open");
        exit(1);
    }
#endif

   /* Spawn the Logging Task */
   if ((tid = taskNameToId("tMsgLogd")) == ERROR)
   {

      /* logTaskId = taskSpawn("tMsgLogd",0,VX_UNBREAKABLE|VX_DEALLOC_STACK,3000, */
      logTaskId = taskSpawn("tMsgLogd",MSGLOG_TASK_PRIORITY,VX_UNBREAKABLE|VX_DEALLOC_STACK,3000,
                 (FUNCPTR)msgLogTask,(int) pMsgLogMsgQ,0, 0,0,0,0,0,0,0,0);
      if (logTaskId == ERROR)
      {
        perror("msgLogCreate: ");
        return(ERROR);
      }
   }
   return( OK );
}
/**************************************************************
*
*  msgLogTask - Waits on the Message Log Queue 
*
*  At present just prints messages to stdout
*
* RETURNS:
*   OK, ERROR - if message lost
*
*/
void
msgLogTask(MSG_Q_ID pMsgQ)
{
   int lpofd,bytes;
   char msgStr[256];

#ifdef USE_PIPE_IS_NOLONG_RECOMMENDED_BY_WRS_4_ISR
   /* Open connection to pipe */
   if (ERROR == (lpofd = open("/pipe/msgLog",READ,0))) {
        perror("open");
        exit(1);
   }
#endif

   /* Wait for data on pipe */
    /* printf("msgLogTask: Passed pMsgQ: 0x%lx, Global pMsgLogQ: 0x%lx\n",pMsgQ,pMsgLogQ); */

    FOREVER 
    {
#ifdef USE_PIPE_IS_NOLONG_RECOMMENDED_BY_WRS_4_ISR
       if (ERROR == (bytes = read(lpofd,msgStr,255))) 
       {
	    perror("msgLogTask: read");
	    exit(1);
       }
#endif
       bytes = msgQReceive(pMsgLogMsgQ, msgStr,255,WAIT_FOREVER);
       /* printf("msgQReceive: %d bytes\n",bytes); */
       msgStr[255] = '\0';  /* just to be sure its null terminated */
       printf("%s",msgStr);
       if (sysLogFd != -1)
	write(sysLogFd,msgStr,bytes);
    }
}

/**************************************************************
*
*  sysLogInit - Initializes to DGRAM socket connect to the syslogd port
*
*
* RETURNS:
* void
*
*       Author Greg Brissey 7/18/94
*/
void sysLogInit()
{
   u_long inetadr;
   struct sockaddr_in sin;
   BOOT_PARAMS *bootparams;
 
   char *bootline = (char *)BOOT_LINE_ADRS; /* memory location of bootline */
 
   msgLost = 0;

    /* Allocate memory to hold values from bootline */
    if (NULL == (bootparams = (BOOT_PARAMS *)malloc(sizeof(BOOT_PARAMS)))) {

        perror("syslogInit");                                                
        exit(1);
    }
 
   /* Parse bootline */
   (void)bootStringToStruct(bootline,bootparams);
 
   /* get boot host inet address, if not in boot Host Address (had[]) then
      try default bootp hostname "bootHost" */
   if (strlen(bootparams->had) != 0)
   {
      inetadr = inet_addr(bootparams->had);
   }
   else if ((inetadr = hostGetByName("bootHost")) == ERROR)
   {
        perror("syslogInit: hostGetByName");
        exit(1);
   }
 
   /* Free up space */
   free(bootparams);
 
/*  We create this pipe so we can let the VxWorks logTask
    pipe it's output to this pipe which can then be transfered
    to the boot Host's syslogd */

    if (ERROR == pipeDevCreate("/pipe/log",200,255)) {
        perror("pipeDevCreate");
        exit(1);
    }
 
    if (ERROR == (lpifd = open("/pipe/log",WRITE,0))) {
        perror("open");
        exit(1);
    }

    /* Clear out the structures for communication */
    memset(&logserv, 0, sizeof(logserv));
    memset(&sin, 0, sizeof(sin));
 
    /* determine the INET address of the server */
    logserv.sin_addr.s_addr = inetadr;
    logserv.sin_family = AF_INET;
    logserv.sin_port = SYSLOGPORT;
 
    /* Get socket for connection */
    SysLogFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (SysLogFd >= 0) {
            sin.sin_family = AF_INET;
            sin.sin_port = 0;
            if (bind(SysLogFd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
                    perror("bind");
                    exit(0);
            }
    }   
        
/*  this is where we start the task that would transfer logTask
    output to the syslogd
*/

    logTaskId = taskSpawn("tsyslogTask",0,VX_UNBREAKABLE|VX_DEALLOC_STACK,3000,
                (FUNCPTR)syslogTask,
                0,0,0,0,0,0,0,0,0,0);
 
}
 
/**************************************************************
*
*  sysLogEnable - Enable syslogging mechinism 
*	
*	This call added the syslogging pipe fd to which logged 
*       will be sent. This means any use of logMsg() will also
*	be sent to the boot host's syslogd. This includes all
*       the DPRINTX, daigPrint, and errLogXXXXXX utilities.
*
* RETURNS:
* void
*
*       Author Greg Brissey 7/18/94
*/
void sysLogEnable(void)
{
    sysLogFd = lpifd;
/*
    if (ERROR == logFdAdd(lpifd)) 
    {
        perror("logFdAdd");
    }
*/
}
/**************************************************************
*
*  sysLogDisable - Disable syslogging mechinism 
*	
*	This call deletes the syslogging pipe fd to which logged 
*       will be sent. This means any use of logMsg() will also
*	be no longer sent to the boot host's syslogd. This includes 
*       all the DPRINTX, daigPrint, and errLogXXXXXX utilities.
*
* RETURNS:
* void
*
*       Author Greg Brissey 7/18/94
*/
void sysLogDisable(void)
{
    sysLogFd = -1;
/*
    if (ERROR == logFdDelete(lpifd)) 
    {
        perror("logFdAdd");
    }
*/
}
/**************************************************************
*
*  logMsgShow - Displays state of Message logging facility 
*
* Output:
*
* msgLogTask: tMsgLogd(0x393890)
*
*   NAME        ENTRY       TID    PRI   STATUS      PC       SP     ERRNO  DELAY
* ---------- ------------ -------- --- ---------- -------- -------- ------- -----
* tMsgLogd   3927bc         393890 100 READY          8320   393868       0     0
*
* Messsaged LOST: 0
*
*
* RETURNS:
*  void 
*
*/
void logMsgShow(void)
{
   printf("msgLogTask: %s(0x%x)\n",taskName(logTaskId),logTaskId);
   taskShow(logTaskId,0);
   /* printf("\nMax Messsaged Queued: %d\n",hiMsgQMark); */
   printf("Messsaged LOST: %d\n",msgLost);
   /* msgQShow(pMsgLogMsgQ,1); */
}


int msgLogIt(char *msg)
{
   return( logIt(msg) );
}

tstdiagp(char *msg)
{
     diagPrint(NULL, msg);
     diagPrint(fileNline(__FILE__,__LINE__), msg);
}

/**************************************************************
*
*  diagPrint - Diagnostic Print routine not typically called directly but used via DPRINT1-5(...) cpp Macros.
*
* This routine prints the given string on the standard output via logMsg().
*  
* Typical usage:
*   DPRINT1(level,"Message String %d\\n",arg1);
*   if DEBUG is defined during compilation then the print statement
*is expanded to the form:
*   if(DebugLevel>=level)diagPrint(debugInfo, string, arg1)
*  otherwise the print statement is not present at all.
*  Level is a debugging level at which one wishes the message to be printed.
*This is compared to the global debug level parameter "DebugLevel".
*It is up to the user to set DebugLevel to some level (0-#).
*Thus if DebugLevel = 0, level = 1 then no message is printed,
*however if DebugLevel = 1 or greater then the Message would be printed.
*
* RETURNS:
* void
*
*       Author Greg Brissey 7/18/94
*/
void diagPrint(char* fileNline, ...)
/* char* fileNline - String containing file name & line number */
{
   va_list vargs;
   char *fmt;
   int len;
   int ItrContext,semResult;
   register int toP;

   char DiagMsgStr[MAX_LOGMSG_LENGTH];
 
   len = 0;
   ItrContext = intContext();     /* at interrupt level? */
   /* printf("ItrContext: %d\n",ItrContext); */
   if (ItrContext == 0 )     /* at interrupt level? */
   {  /* NO */
     if (fileNline != NULL)
     {
       sprintf(DiagMsgStr,"%s",fileNline);
        /* printf("DiagMsgStr: '%s'\n",DiagMsgStr); */
       len = strlen(DiagMsgStr);
       if (! (intContext()))     /* at interrupt level? */
       {
	 /* free(fileNline);	/* if not, free malloc space from fileNline() */
         /* printf("diagPrint: return: 0x%lx\n", fileNline); */
         RNG_LONG_PUT(pLRngId, (long) fileNline, toP);
       } 
     }
   }
 
   if (ItrContext != 1) /* if a task context then use semaphore */
   {
      semResult = semTake(pVaMutex,calcSysClkTicks(1000));  /* wait up to 1 sec then just continue */
      if (semResult != OK)
      {
        printf("diagPrint(): WARNING: semTake of va_start,va_arg,va_end Mutex timed out.\n");
      }
   }

   va_start(vargs, fileNline);
   fmt = va_arg(vargs, char *);
   vsprintf((DiagMsgStr + len), fmt, vargs);
   va_end(vargs);

   if (ItrContext != 1)
   {
      if (semResult == OK)  /* give back if take did not timeout */
      {
         semGive(pVaMutex);
      }
   }
 
   /* printf("diagPrint(): strlen: %d\n",strlen(DiagMsgStr)); */
   if (strlen(DiagMsgStr) >= MAX_LOGMSG_LENGTH)
      fprintf(stderr,"diagPrint(): overran DiagMsgStr length\n");

   /* safety in call logMsg() in interrupt context, for spped one should call logMsg() directly
      4 us (logMsg)  us vs 34 us on a 333 MHz 405 */
   if ( ItrContext == 0 )
   {
      logIt(DiagMsgStr);
   }
   else
   {
      logMsg(DiagMsgStr,1,2,3,4,5,6);
   }
 
   return;
}
 
/**************************************************************
*
*  errLogRet - Print/Log Application Error
*
* This routine prints and optionally logs an application error
*  then returns to the calling function.
*
* RETURNS:
* void
*
*       Author Greg Brissey 7/18/94
*/
void errLogRet(int prtOpt, char *fileNline, ...)
/* int prtOpt - Unix Version compatibility, No function */
/* char* fileNline - String containing file names & line number */
{
   va_list vargs;
   char *fmt;
   int len;
   char ErrMsgStr[MAX_LOGMSG_LENGTH];
   register int toP;
 
   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      if (! (intContext()))     /* at interrupt level? */
      {
	/* free(fileNline);	/* if not, free malloc space from fileNline() */
        RNG_LONG_PUT(pLRngId, (long) fileNline, toP);
      } 
      len = strlen(ErrMsgStr);
   }
 
   va_start(vargs, fileNline);
   fmt = va_arg(vargs, char *);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);
 
   if (strlen(ErrMsgStr) >= MAX_LOGMSG_LENGTH)
      fprintf(stderr,"errLogRet(): overran ErrMsgStr length\n");

   logIt(ErrMsgStr);
       
   return;
}

/**************************************************************
*
*  errLogQuit - Print/Log Application Error
*
* This routine prints and optionally logs an application error
*  then quits the application via exit(1).
*
* RETURNS:
* void
*
*       Author Greg Brissey 7/18/94
*/
void errLogQuit(int prtOpt, char *fileNline, ...)
/* int prtOpt - Unix Version compatibility, No function */
/* char* fileNline - String containing file names & line number */
{
    va_list vargs;
    char *fmt;
    int len;
    register int toP;
    char ErrMsgStr[MAX_LOGMSG_LENGTH];
 
   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      if (! (intContext()))     /* at interrupt level? */
      {
	/* free(fileNline);	/* if not, free malloc space from fileNline() */
        RNG_LONG_PUT(pLRngId, (long) fileNline, toP);
      } 
      len = strlen(ErrMsgStr);
   }
 
   va_start(vargs, fileNline);
   fmt = va_arg(vargs, char *);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);
 
   if (strlen(ErrMsgStr) >= MAX_LOGMSG_LENGTH)
      fprintf(stderr,"errLogQuit(): overran ErrMsgStr length\n");

   logIt(ErrMsgStr);

   exit(1);
}
/**************************************************************
*
*  errLogSysRet - Print/Log Application Error
*
* This routine prints and optionally logs a system error,
*the system errno message is appended, then returns to the
*calling function.
*
* RETURNS:
* void
*
*       Author Greg Brissey 7/18/94
*/
void errLogSysRet(int prtOpt, char *fileNline, ...)
/* int prtOpt - Unix Version compatibility, No function */
/* char* fileNline - String containing file names & line number */
{
   va_list vargs;
   char *fmt;
   int len;
   register int toP;
   char ErrMsgStr[MAX_LOGMSG_LENGTH];
 
   taskDelay(10);
   ErrMsgStr[0] = 0;
   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      if (! (intContext()))     /* at interrupt level? */
      {
	/* free(fileNline);	/* if not, free malloc space from fileNline() */
        RNG_LONG_PUT(pLRngId, (long) fileNline, toP);
      } 
      len = strlen(ErrMsgStr);
   }
   /* printf("errLogSysRet: msg: '%s'\n", ErrMsgStr); */
 
   va_start(vargs, fileNline);
   fmt = va_arg(vargs, char *);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);
 
   errPrtMsg(ErrMsgStr,MAX_LOGMSG_LENGTH-1);         /* appends system Error to message */

   /* printf("errPrtMsg return: '%s'\n",ErrMsgStr); */
   /* printf("ErrMsgStr: '%s', len: %d, maxlen: %d\n", ErrMsgStr,strlen(ErrMsgStr), MAX_LOGMSG_LENGTH); */
   if (strlen(ErrMsgStr) >= MAX_LOGMSG_LENGTH)
      fprintf(stderr,"errLogSysRet(): overran ErrMsgStr length\n"); 

   logIt(ErrMsgStr);
       
   return;
}

/**************************************************************
*
*  errLogSysQuit - Print/Log Application Error
*
* This routine prints and optionally logs a System error,
*the system errno message is appended,
*then quits the application via exit(1).
*
* RETURNS:
* void
*
*       Author Greg Brissey 7/18/94
*/
void errLogSysQuit(int prtOpt, char *fileNline, ...)
/* int prtOpt - Unix Version compatibility, No function */
/* char* fileNline - String containing file names & line number */
{
    va_list vargs;
    char *fmt;
    int len;
    register int toP;
    char ErrMsgStr[MAX_LOGMSG_LENGTH];
 
   len = 0;
   if (fileNline != NULL)
   {
      sprintf(ErrMsgStr,"%s",fileNline);
      if (! (intContext()))     /* at interrupt level? */
      {
	/* free(fileNline);	/* if not, free malloc space from fileNline() */
        RNG_LONG_PUT(pLRngId, (long) fileNline, toP);
      } 
      len = strlen(ErrMsgStr);
   }
 
   va_start(vargs, fileNline);
   fmt = va_arg(vargs, char *);
   vsprintf((ErrMsgStr + len), fmt, vargs);
   va_end(vargs);

   errPrtMsg(ErrMsgStr,MAX_LOGMSG_LENGTH-1);         /* appends system Error to message */
 
   if (strlen(ErrMsgStr) >= MAX_LOGMSG_LENGTH)
      fprintf(stderr,"errLogSysQuit(): overran ErrMsgStr length\n");

   logIt(ErrMsgStr);
/*
   if (prtOpt == LOGIT)
      sysLog(LOG_LOCAL7 | LOG_ERR, ErrMsgStr);
*/
       
   exit(1);
}

/**************************************************************
*
*  errPrtMsg - appends errno system message to end of Error String
*
* This routine is for internal library use.
*
* RETURNS:
* void
*
*       Author Greg Brissey 7/18/94
*/
static int errPrtMsg(char *buffer, int maxSize)
/* char *buffer - buffer to add system error message to */
{
   int len;
   int slen,result;
   char errorstr[200];
 
   errorstr[0]=0;
   len = strlen(buffer);
   /* taskDelay(10); */
   /* printf("errPrtMsg: str: '%s', len: %d\n",buffer,len); */
   result = strerror_r(errno,errorstr); /* POSIX reentrient version */
   slen = strlen(errorstr);
   /* printf("result: %d\n",result); */
   if (result == OK)
   {
      slen = strlen(errorstr);
      if ( (slen + len) <= maxSize)
      {
          result = sprintf(buffer + len, " %s", errorstr);
          /* printf("OK - sprintf copy len: %d\n",result);
           * printf("sysErrMsg: '%s'\n",errorstr);
           * printf("Msg: '%s'\n",buffer);
           */
      }
      else
      {
         printf("length: %d Too Big, max: %d\n",slen + len,maxSize);
      }
   }
   else
   {
      result =  sprintf(buffer + len, "unknown errno: %d",errno);
      /* printf("ERROR - sprintf copy len: %d\n",result); */
   }
   /* printf("return: Msg: '%s'\n",buffer); */
   /* Note: for some reason buring boot if this routine called, returning will from
    *  this routine hangs the boot up, can't seem to figure what is going on
    *   GMB  1/26/07 */
   return 0;
}

/**************************************************************
*
*  logIt - generates timestamp, prepends to message
*	    and sends it down via logMsg() 
*
*  Not a User callable function
*
*
* RETURNS:
* OK, ERROR - if message lost index index 
*
*	Author Greg Brissey 8/5/93
*/
static int logIt(char *pMsgStr)
/* char *pMsgStr - Message String to log */
{
   int size,bytes;
   int len;
   time_t  timet;
   time_t  timet2;
   char timestamp[40];
   char pFullMsg[MAX_LOGMSG_LENGTH];
   /* struct tm *tmtime; */
   struct tm tmtime;

/* -------- ANSI way of getting time of day */
   size = sizeof(timestamp);
   timet2 = time(&timet);
   /* timstr = ctime(&timet); */
   /* ctime_r(&timet,timestamp,&size); */
   /* tmtime = localtime(&timet); */
   /* strftime(timestamp,sizeof(timestamp),"%H:%M:%S",tmtime); */
   localtime_r(&timet,&tmtime);
   strftime(timestamp,sizeof(timestamp),"%H:%M:%S",&tmtime);
/* ------------------------------------ */

/*----- concatinate the various string to form message ----- */

   strcpy(pFullMsg,timestamp);	/* Time Stamp */
   strcat(pFullMsg,pMsgStr);	/* Task Name, file, line, message  */

   /* if not carrage return add one */
   len = strlen(pFullMsg);
   if ((char)(pFullMsg[len-1]) != (char) '\n')
   {
      strcat(pFullMsg,"\n");
   }

   size = strlen(pFullMsg);
#ifdef USE_PIPE_IS_NOLONG_RECOMMENDED_BY_WRS_4_ISR
   bytes = write(msgPipe,pFullMsg,size+1);
   if (bytes != size+1)
     msgLost++;
#endif

   /* printf("pMsgLogMsgQ: 0x%lx\n",pMsgLogMsgQ); */
   if (pMsgLogMsgQ != NULL)
   {
      /* printf("msgQSend: size: %d\n",size+1); */
      msgQSend(pMsgLogMsgQ, pFullMsg, (size+1), NO_WAIT, MSG_PRI_NORMAL);
   }

#ifdef XXXX
   else
   {
      /* adjust high level mark of Queued Messages */
     nQd= msgQNumMsgs(pMsgLogMsgQ);
     hiMsgQMark = (nQd > hiMsgQMark) ? nQd : hiMsgQMark;
   }
#endif

   return(OK);
}
/**************************************************************
*
*  fileNline - returns pointer to File name & Line Number string
*
* This routine is used via the infoDebug cpp macro and is
* not typically used directly by the user.
*
* RETURNS:
* char*
*
*       Author Greg Brissey 7/18/94
*/
char *fileNline(char* filename, int linenum)
/* char *filename - filename of fil __FILE__ */
/* char linenum - line number within file __LINE__ */
{
   /* char *pTaskName; */

   static char stbuffer[100];
   int stat;
   char tmpbuffer[128];
   char *mbuffer;
   register int toP;

   if (intContext())     /* at interrupt level? */
   {
     sprintf(stbuffer," [IntContxt]: '%s' line: %d, ", filename,linenum);
     return(stbuffer);
   }  
   else
   {
     /*
     Since we are using logMsg, don't need taskName, logMsg does that 
     pTaskName = (char *) taskName(taskIdSelf());
     sprintf(mbuffer," [%s]: '%s' line: %d, ",pTaskName, filename,linenum);
     */
     /* if ( (mbuffer = (char *) malloc(128)) == NULL)
	return(NULL); */
     stat = RNG_LONG_GET(pLRngId, (long*) &mbuffer,toP);
     /* printf("fileNline: stat %d, Addr: 0x%lx\n",stat, mbuffer); */
     if ( stat != 0)
     {
        sprintf(tmpbuffer,"  '%s' line: %d, ",filename,linenum);
        strncpy(mbuffer,tmpbuffer,FILENLINEBUFSIZEM1);  /* one less than size to insure NULL termination */
     }
     else
     {
        mbuffer = NULL;
     }
   } 
   return(mbuffer);
}

/**************************************************************
*
*  sysLog - log message directly to the boot Host's syslogd port
*	
*	This call may result in unanticipated blocking due to
*	the sendto() socket fd. The prefered method is to use
*	DPRINT, daigPrint, or one of the errLogXXX calls
*       with the syslogging enabled.
*
* RETURNS:
* void
*
*       Author Greg Brissey 7/18/94
*/
void sysLog(int pri,char* message)
/* int pri - priority of syslog message */
/* char *message - message to log */
{
   int l;
   char line[MAXLINE];             /* Holds the formatted message to be sent */
 
   extern int SysLogFd;
   extern struct sockaddr_in logserv;
 
   if ( SysLogFd == -1 )
       return;

   /* Add the date */
   pri |= ADDDATE;
 
   /* Compose the message */
   l = sprintf(line, "<%d> %s",pri,message);
   if (l > MAXLINE)
       l = MAXLINE;
 
   /* send the message */
   if (sendto(SysLogFd, line, l, 0,
       (struct sockaddr *)&logserv, sizeof (logserv)) != l) 
   {
                perror("sendto");
   }
   return;
} /* syslog */


/**************************************************************
*
*  syslogTask - Waits on the log pipe
*
*   syslogTask deverts any incomming messages to the syslogd
*   DataGram port of the boot hosts syslogd.  There must be a 
*   carrage return end the message to allow this task to identify the 
*   the end of the message.
*
* RETURNS:
*   OK, ERROR - if message lost
*
*/
void syslogTask()
{
    int il,ol;
    char buf[255];
    int pri;
    char buf2[255];
    char line[MAXLINE];
    register int i;
    int pos = 0;

    extern struct sockaddr_in logserv;

    /* Initialize buffer */
    memset(buf,0,sizeof(buf));

    /* Set default priority */
    pri = LOG_LOCAL7 | LOG_ERR | ADDDATE;


    /* Open connection to pipe */
    if (ERROR == (lpofd = open("/pipe/log",READ,0))) {
        perror("open");
        exit(1);
    }

    /* Wait for data on pipe */
    FOREVER 
    {
        /* read incomming message, note it may not come in one read, 
	   so we must keep reading untill the carrage return is found.
        */
	if (ERROR == (il = read(lpofd,buf,255))) 
        {
	    perror("read");
	    exit(1);
	}

	/* printf("sysLogD(%d): '%s'\n",il,buf); */

        /* check each character for carrage return */
	for (i = 0; i < il; i++) 
        {
	    if (buf[i] == '\n')   /* carrage return found the send message */
            {
		/* Compose the message */
		buf2[pos] = '\0';

		/* added syslog priority, etc */
		ol = sprintf(line, "<%d> %s",pri,buf2);
		if (ol > MAXLINE)
		    ol = MAXLINE;

		/* send the message */
		if (sendto(SysLogFd, line, ol, 0, (struct sockaddr *)&logserv, sizeof (logserv)) != ol) 
		    perror("sendto");

	       /* Reset buffer and pointer */
	       bzero(buf2,sizeof(buf2));
	       pos = 0;
	    }
	    else buf2[pos++] = buf[i];
	}
    }
} /* syslogTask */

void logMsgLibShowSCCSId()
{
     printf("%s\n",SCCSid);
}
