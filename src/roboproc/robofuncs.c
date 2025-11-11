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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <tcl.h>

#include "errLogLib.h"
#include "mfileObj.h"
#ifdef MAPEXP
#include "shrMLib.h"
#include "shrexpinfo.h"
#endif
#include "shrstatinfo.h"
#include "msgQLib.h"
#include "acquisition.h"
#include "acqcmds.h"
#include "errorcodes.h"
#include "rackObj.h"
#include "gilfuncs.h"
#include "gilsonObj.h"
#include "iofuncs.h"
#include "timerfuncs.h"
#include "hrm_errors.h"

extern char HostName[];
extern char systemdir[];
extern FILE *logFD;

/************************************************************************
 * Declarations for routines that have no include file for us to use.
 ************************************************************************/
#ifdef MAPEXP
extern int getStatAcqSample(void);                   /* statfuncs.c     */
extern void expStatusRelease(void);                  /* statfuncs.c     */
#endif
extern int InterpScript(char *tclScriptFile);        /* tclfuncs.c      */
extern int racksBeenRead(void);                      /* robofuncs.c     */
extern void shutdownComm(void);                      /* roboproc.c      */
extern int SendRabbit(char *cmd, char *rsp);         /* hrm_scheduler.c */
extern void gilsonDelete(GILSONOBJ_ID pGilId);       /* gilsonObj.c     */
extern int terminate();

/******************* TYPE DEFINITIONS AND CONSTANTS *************************/
#define  _POSIX_SOURCE 1
#define EOT  3
#define EOM  3
#define LF 10
#define CR 13
#define PERIOD 46
#define ASMCHANGER 0
#define SMSCHANGER 1
#define GILSON_215 3
#define NMSCHANGER 4
#define HRMCHANGER 5
#define AS4896CHANGER 6
#define ILLEGALCMD 13
#define FALSE  0
#define TRUE  1
#define WAIT_FOR_TIMEOUT    40000  /* in milisecond */
#define MSEC_IN_SEC          1000  /* msec in a second */
#define UNAMESIZE 80
/* AS 7510 is a little slower than AS 7600 */
#define LONG_MOTION_TIMEOUT 200
#define MOTION_TIMEOUT     90
/* NONMOTION_TIMEOUT needs enough time for system power-up */
#define NONMOTION_TIMEOUT  30

static char cr = (char) 13;     /* '\r' */
#ifdef XXX
static char lf = (char) 10;     /* '\n' */
static char eom = (char) 3;
static char period = (char) 46; /* '.'  */
#endif

/**************************** GLOBAL VARIABLES ******************************/

#ifdef MAPEXP
/* Start addr of shared Exp. Info Structure */
SHR_EXP_INFO expInfo = NULL;
/* Shared Memory Object */
static SHR_MEM_ID  ShrExpInfo = NULL;
#endif

/* if abort is active */
extern int abortActive;

/* SIGUSR2 will set this to abort sample change */
extern int AbortRobo;
extern char roboErrLog[];


/****************************** STATIC GLOBALS ******************************/

/* -99 = uninitialized, -1 = no samp in mag */
#ifdef XXX
static int sampeInMagnet = -99;
#endif

/* Type of sample changer we are hooked up to */
static int SMS_Changer = -1;

static char UserName[UNAMESIZE];

/*
 * This flag is used at present only for gilson operation
 * where a reportRobotStat(0) may be issued by the TCL script
 * thus allowing the gilson to work while acquisition continues.
 * This flag is tested to prevent multiple reportRobotStat() from
 * being sent.
 */
static int reportRobotStatDone = 0;


/*
 * Questions for completion.
 * Q.    The question now arises is how to handle the spinner controller 
 *
 * A.    Use talk2Acq like Vnmr does for sethw commands, talk2Acq doesn't 
 *       return till the command is complete, errors are returned via a
 *       char string.
 *
 * Q.    How to report spinner error & not continue ??? 
 *
 * A.    Errors of spinner or sample changer, result in an 'aa' sent to
 *       Expproc to abort the experiment, 
 *       Either aa contains an arg for the errorcode to use for Recvproc
 *       to get which is then sent to Procproc
 *     OR 
 *       the doneCode & errorCode are Queued directly to Procproc from
 *       Roboproc
 *
 *     Need to get confirmation that sample has been removed
 *        to determine if we can continue to change sample
 *        Equivalent to spinstat('C',) 
 */

static char *timeStamp()
{
   struct timeval tv;
   static char dateString[40];

   dateString[0] = '\0';
   if ( ! gettimeofday( &tv, NULL) )
   {
      strftime(dateString, 40, "%F %T",localtime((long*)&(tv.tv_sec)));
   }
   return(dateString);
}

/*****************************************************************************
                           S p i n n e r
    Description:

*****************************************************************************/
int Spinner(char cmd, int value)
{
    char msg4acq[ 80 ];
    char replymsg[ 80 ];
    int  replymsgsize;
    int talk2Acq();
    int fd;

    replymsgsize = 80;
    switch (cmd)
    {
        case 'E':
            sprintf(msg4acq,"1,%d",EJECT);
            break;
        case 'I':
            sprintf(msg4acq,"1,%d",INSERT);
            break;
        case 'X':
            sprintf(msg4acq,"1,%d",EJECTOFF);
            break;
        case 'B':
            sprintf(msg4acq,"1,%d",BEARON);
            break;
        case 'O':
            sprintf(msg4acq,"1,%d",BEAROFF);
            break;
        case 'S':
            sprintf(msg4acq,"1,%d,%d",SETSPD,value);
            break;
        case 'R':
            sprintf(msg4acq,"1,%d,%d",SETRATE,value);
            break;
        case 'L':
            sprintf(msg4acq,"2,%d,%d",SETLOC,value);
            break;
        case 'M':
            sprintf(msg4acq,"2,%d,%d",SETSTATUS,value);
            break;
        case 'P':
            break;
        default:
            printf("unknown command\n");
            break;
    }
    DPRINT4(1,"Spinner: host: '%s', user: '%s', cmd: %d, msge: '%s'\n",
               HostName, UserName, ACQHARDWARE, msg4acq);

    /* check for abort, if SO then just return */
    if (AbortRobo != 0) 
    {
        DPRINT(1,"Spinner: Sample Changer Aborted\n");
        return(-1);
    }

    /* Use Roboproc as user, acqHwSet in msgehandler.c checks this and 
     *  allows Roboproc to perform any commands it wishes
     */
    fd = talk2Acq(HostName,"Roboproc",ACQHARDWARE,msg4acq, 
                replymsg, replymsgsize );
    if (fd >= 0)
        close(fd);
    DPRINT1(1,"Spinner: Msge %s completed\n",msg4acq);
    if (logFD)
       fprintf(logFD,"%s SPINNER: %c %d\n",timeStamp(),cmd,value);

    /* now check replymsg for errors */

    return( 0 );
}

/**************************** I/O FUNCTIONS *********************************/
/*****************************************************************************
                             r e a d P o r t
    Description:

    cmdecho(aport, line14c, CMD)

    type = 0 - receives from the robot the expanded form of
               the command given till the EOM char is received

    type = 1 - wait for just an echoed character

*****************************************************************************/
int readPort( int type, int timeout )
{
    char charbuf1[128];
    int  rbyte=0, i, echo;
    char chr=0;
  
    echo = FALSE;
    while( chr != EOM && !echo )
    {
        timer_went_off = 0;
        setup_ms_timer(MSEC_IN_SEC * timeout);
        /*
         *  This is a BLOCKING read.
         *  Read will wait if there is nothing on the port.
         */
        rbyte = smsDevEntry->read(smsDev, charbuf1, 1);

        /*
         *  We returned so either we read something or
         *    the ALARM went off.
         *  If a timeout occured, an error is returned.
         */
        cleanup_from_timeout();
        if (timer_went_off)
        {
            errLogRet(LOGOPT, debugInfo,
                      "readPort: Sample Changer NOT Responsive\n");
            return (SMPTIMEOUT);
        }

        DPRINT1(2,"\nRECEIVED: %d chars\n", rbyte);
        if (type != 0)  
            echo = TRUE;

        for (i=0; i<rbyte; i++)
        {
            chr = charbuf1[i];
            DPRINT3(2,"char %d:0x%4x\t\t ==>  0x%3x\n",i+1,chr,chr);
        }
 
   } /* end of while */

    return (0);
}

/*****************************************************************************
                          w r i t e P o r t
    Description:
    Sending numerical part of commands to i/o channel
    then read the echo back from robot one digit at a time.

    ( echoval(aport, smpmum, line14c) )

*****************************************************************************/
int writePort( int smpnum )
{
    char  charbuf[20];
    char   *ptr;
    int   wbyte __attribute__((unused));
    int   stat;
    
    sprintf(charbuf,"%d", smpnum);
    ptr = charbuf;

    while (*ptr != '\000')
    {
        DPRINT1(2,"writePort Send: 0x%2x\n", *ptr);
        wbyte = smsDevEntry->write(smsDev, ptr, 1);
        ptr++;

        /* Read the echo back till end of text ("0x03") */
        stat = readPort(1, 6);
        if (stat == SMPTIMEOUT)
            return(stat);
    }

    return (0);
}

/*****************************************************************************
                              c m d A c k
    Description:
    Receives the sample changer response.
    Receives the character stream output from the
    sample changer and compiles any error numbers returned.
    This routine returns when the sample changer returns
    its prompt ( CRLF ) or error message ( -error EOM )

    Returns:
    0           - Success.
    -#          - Error, # == returned sample changer error.

    Jan 12 95

*****************************************************************************/
int cmdAck (int timeout)
{
    int  error, done, sign;
    char charbuf1[128];
    int  rbyte=0, i;
    char chr, prevchr;
  
    chr = prevchr = 0;
    error = 0;
    done  = FALSE;
    sign = 1;

    DPRINT1(1,"cmdAck: expecting CR NL . or (0xd 0xa 0x2e) WITHIN "
              "%d Seconds \n", timeout);
    DPRINT(1,"cmdAck: error expecting ## (0=0x30) EOM (0x3) "
             "then (0xd 0xa 0x2e)\n");
    while( chr != EOM && !done )
    {
        timer_went_off = 0;
        setup_ms_timer(MSEC_IN_SEC * timeout);

        rbyte = smsDevEntry->read(smsDev, charbuf1, 1);

        cleanup_from_timeout();
        if (timer_went_off)
        {
            errLogRet(LOGOPT, debugInfo,
                      "cmdAck: Sample Changer NOT Responsive");
            return (SMPTIMEOUT);
        }

        DPRINT1(1,"cmdAck: Received: %d chars\n", rbyte);
  
        for (i=0; i<rbyte; i++) 
        {
            chr = charbuf1[i];
            DPRINT3(1,"cmdAck: char %d:0x%4x\t\t ==>  0x%3x\n",i+1,chr,chr);
    
            if (chr == '-')
                sign = -1;
            if (chr >= '0' && chr <= '9')
                error = error*10 + (chr - '0');
            if (chr == PERIOD && ( prevchr == LF || prevchr == CR))
                done = TRUE;
            prevchr = chr;
        }
    }  /* end of while */

    if (chr == EOM )
    {
        DPRINT(1," ERROR\n ");
        DPRINT1(1," last chr = %d\n",chr);
        DPRINT1(1," error = %d\n",error);
        DPRINT1(1," sign = %d\n",sign);
        DPRINT1(1,"error return # %d\n",error*sign);
        return (error*sign);
    }

    return(error*sign);  
}

/*****************************************************************************
                         r o b o t C m p l t
    Description:
    Waits for the sample changer to complete a command.

    Returns:
    0           - Success.
    -#          - Error, # == returned sample changer error.

    Jan-12-95

*****************************************************************************/
int robotCmplt()
{
    int stat;

    DPRINT(1,"robotCmplt: Waiting for Cmd Ack\n");

    /* 3 min to timeout */
    if ( (stat = cmdAck(180)) )
        /* Return if stat != 0  ERROR */
        return(stat);

    /* At this point --> nicely DONE */
    DPRINT(1," robotCmplt: cmdAck D O N E\n\n");
    return (0);
}


/*************************** ROBOT FUNCTIONS ********************************/
/*****************************************************************************
                           s t a r t r o b o t
    Description:
    Send a command to the sample changer.
    Command is a single character.
    Number is usually the sample to Remove or Load.
    Returns immediately without waiting for the sample
    changer to complete.

    Returns:
    0           - Success.
    -#          - Error, # == returned sample changer error.

    Jan-12-95

*****************************************************************************/
int startRobot( char cmd, int smpnum )
{
    char *ptr;
    char  chrbuf[20];
    int   wbyte __attribute__((unused));
    int   stat;

    /* Check for abort, if so then just return */
    if (AbortRobo != 0) 
    {
        DPRINT(1,"startRobot: Sample Changer Aborted\n");
        return(90);
    }

    /* Clear i/o channel */
    smsDevEntry->ioctl(smsDev, IO_FLUSH);

    /*-- These commands require a parameter --*/
    sprintf(chrbuf,"%c", cmd);
    ptr = chrbuf;

    DPRINT2(1,"startRobot: cmd: %s, Arg: %d\n",chrbuf,smpnum);

    /* Sending alphabetical part of command to robot  */
    DPRINT1(2,"SENDcmd: 0x%2x\n",*ptr);
    wbyte= smsDevEntry->write(smsDev, ptr, 1);

    /* Read the echo back till end of text ("0x03") or timeout */
    stat = readPort(0, 6);
    if (stat == SMPTIMEOUT)
        return(stat);

    if (cmd == 'R' || cmd == 'L' || cmd == 'V' || cmd == 'W' || cmd
        == 'N' || cmd == 'Q' || cmd == 'J' || cmd == 'K' || cmd == 'Z')
    {
        DPRINT1(2,"\nSENDsmpnum: %2x Hex",smpnum);
        writePort(smpnum);
    }

    /* Sending "cr", The end of command  */
    /* DPRINT(2,"\nSEND 'cr'");  */
    smsDevEntry->write(smsDev, "\r", 1);

    return (0);
}

/*****************************************************************************
                                r o b o t
    Description:
    This routine does same things as "old robot".

*****************************************************************************/
int robot( char cmd, int smpnum )
{
    int stat;
  
    if ( (stat = startRobot( cmd, smpnum )) )
        /* Return if stat != 0  ERROR  */
        return (stat);

    return( robotCmplt() );
}

/*****************************************************************************
                            p a r k R o b o t
    Description:
    If error or some other problem happen put robot back to home position

*****************************************************************************/
void parkRobot()
{
    /* if NMS then don't bother with the eject air */
    if (SMS_Changer != NMSCHANGER) 
    {
        Spinner('I',0);
    }

    /* check for abort, if SO then just return */
    if (AbortRobo != 0) 
        return;

    /* Return Home */
    robot('I',0);
    if (SMS_Changer == ASMCHANGER)
    {
        /* Shutdown Robot */
        robot('S',0);
    }
}

/*****************************************************************************

                     s e t S a m p C h n g r T y p e
    Description:
    Called during initialization to set the SMS_Changer static
    global variable to the type of sample changer we are attached to.
    The possible input values and results are listed below.

        Input Parm     SMS_Changer         Comment
        "SMS_*"        SMSCHANGER      Zymark 50/100 sample handler
        "ASM_*"        ASMCHANGER      Carousel sample handler
        "NMS_*"        NMSCHANGER      Nano multi-sampler
        "GIL_*"        GILSON_215      VAST flow system w/Gilson 215
        "HRM_*"        HRMCHANGER      Hermes high-throughput system

*****************************************************************************/
void setSampChngrType(char *type)
{
    if ( (strncmp(type,"sms_",4) == 0) || (strncmp(type,"SMS_",4) == 0) )
    {
        SMS_Changer = SMSCHANGER;
    }
    else if ( (strncmp(type,"asm_",4) == 0) || (strncmp(type,"ASM_",4) == 0) )
    {
        SMS_Changer = ASMCHANGER;
    }
    else if ( (strncmp(type,"gil_",4) == 0) || (strncmp(type,"GIL_",4) == 0) )
    {
        SMS_Changer = GILSON_215;
    }
    else if ( (strncmp(type,"nms_",4) == 0) || (strncmp(type,"NMS_",4) == 0) )
    {
        SMS_Changer = NMSCHANGER;
    }
    else if ( (strncmp(type,"hrm_",4) == 0) || (strncmp(type,"HRM_",4) == 0) )
    {
        SMS_Changer = HRMCHANGER;
    }
    else if ( (strcmp(type,"as4896") == 0) || (strcmp(type,"AS4896") == 0) )
    {
        SMS_Changer = AS4896CHANGER;
    }
    else
    {
        SMS_Changer = -1;   /* unknown, will ask device */
    }
    DPRINT2(1,"setSampChngrType: Type: '%s', SMS/ASM/GILSON Flag: %d\n",
            type,SMS_Changer);
}

/*****************************************************************************

                     g e t S a m p C h n g r T y p e
    Description:
    Returns value of the static variable SMS_Changer.
    The possible input values and results are listed below.

        Input Parm     SMS_Changer         Comment
        "SMS"          SMSCHANGER      Zymark 50/100 sample handler
        "ASM"          ASMCHANGER      Carousel sample handler
        "NMS"          NMSCHANGER      Nano multi-sampler
        "GIL"          GILSON_215      VAST flow system w/Gilson 215
        "HRM"          HRMCHANGER      Hermes high-throughput system

*****************************************************************************/
int getSampChngrType()
{
    return(SMS_Changer);
}

void deliverMessageQ(char *interface, char *msg)
{
    MSG_Q_ID pMsgQ; 

    pMsgQ = openMsgQ(interface);
    if ( pMsgQ != NULL)
    {
        sendMsgQ(pMsgQ,msg,strlen(msg),MSGQ_NORMAL,
                                WAIT_FOREVER);
        closeMsgQ(pMsgQ);
        DPRINT2(1,"deliverMessageQ: to %s Msge Sent: '%s'\n",interface,msg);
    }
}


/*****************************************************************************
                     r e p o r t R o b o t S t a t
    Description:
    Send a roboack message to ExpProc using the message queue facility.
    The format of the message is: "roboack #" where # is equal to the
    robstat parameter.  Sets the static variable reportRobotStatDone
    to one when complete.

*****************************************************************************/
void reportRobotStat(int robstat)
{
    MSG_Q_ID pMsgQ; 
    char msg4acq[ 80 ];

    /* Check for abort, if so then just return */
    if (AbortRobo != 0) {
        DPRINT(1,"reportRobotStat: command aborted, do not return status");
        return;
    }

    sprintf(msg4acq,"roboack %d",robstat);
    pMsgQ = openMsgQ("Expproc");
    if ( pMsgQ != NULL)
    {
        sendMsgQ(pMsgQ,msg4acq,strlen(msg4acq),MSGQ_NORMAL,
                                WAIT_FOREVER);
        closeMsgQ(pMsgQ);
        reportRobotStatDone = 1;
        DPRINT1(1,"reportRobotStat: Ack Msge Sent: '%s'\n",msg4acq);
    }
    else
        DPRINT(0,"reportRobotStat: Unable to open Expproc msgQ");
}
    
/************************* INTERFACE FUNCTIONS ******************************/
/*****************************************************************************
                           c h g S m p N u m
    Description:
    Change sample number considered in magnet by the 
    SMS System V controller.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 9/21/94

*****************************************************************************/
int chgSmpNum(char *args)
{
    char *value;
    int newloc;

    value = strtok(NULL," ");
    newloc = atoi(value);
    DPRINT2(1,"chgSmpNum: %d, SMS_Changer= %d\n",newloc,SMS_Changer);

    /* If SMS system need to tell System V controller Too */
    if (SMS_Changer == SMSCHANGER )
        robot('K',newloc);

    return(0);
}

/*****************************************************************************
                           s e t S m p T r a y
    Description:
    Set number and size of sample trays(s) by the SMS
    System V controller.

    Valid output to SMS: 1050, 1100, 2050 & 2100

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 9/21/94

*****************************************************************************/
void setSmpTray(char *args)
{
    char *value;
    int num,size;
    int numnsiz;

    value = strtok(NULL," ");
    num = atoi(value);

    value = strtok(NULL," ");
    size = atoi(value);

    DPRINT2(1,"setSmpTray: %d - %d tray(s)\n", num,size);
    if (SMS_Changer == SMSCHANGER )
    {
        /* valid number of trays: 1 or 2 */
        numnsiz = num * 1000;

        /* valid tray sizes: 50 or 100 */
        numnsiz += size;

        robot('J',numnsiz);
    }
}

static int getErrFromRes(int res)
{
   int err;

   err = (res) ? -res - 2000 : 0;
   if ( (err < 0) || (err > 99) )
      err = 50;
   return(err);
}

int sendToAS(char *cmd, char *retMsg, int retLen, int timeout)
{
   int res;
   static int doFlush = 1;
   FILE *errLog;

   DPRINT1(1,"sendToAs cmd: %s\n", cmd);
   if (logFD)
   {
      fprintf(logFD,"%s CMD: %s\n",timeStamp(), cmd);
      fflush(logFD);
   }
   if (smsDev < 0)
   {
      doFlush = 1;
      smsDev = smsDevEntry->open(smsDevEntry->devName);
      if (smsDev < 0)
      {
         strcpy(retMsg,"Robot is non-responsive");
         if (logFD)
         {
            fprintf(logFD,"%s -2098 %s\n",timeStamp(), retMsg);
            fflush(logFD);
         }
         errLog = fopen(roboErrLog,"a");
         fprintf(errLog,"%s Failed to open Robot device %s\n",
                         timeStamp(),smsDevEntry->devName);
         fclose(errLog);
         return(-2098);
      }
   }
   if (doFlush)
   {
      if (logFD)
      {
         fprintf(logFD,"%s clear io channel\n", timeStamp());
         fflush(logFD);
      }
      res = read_lan_timed(smsDev, retMsg, retLen, 1000);
      doFlush = 0;
   }
   res = write(smsDev, cmd, strlen(cmd));
   if (res >= 0)
      res = write(smsDev, "\n", 1);
   if (res < 0)
   {
      if (errno == EPIPE) 
      {
         DPRINT(1,"sendToAs robot closed socket\n");
         close(smsDev);
         smsDev = smsDevEntry->open(smsDevEntry->devName);
         if (smsDev < 0)
         {
            doFlush = 1;
            strcpy(retMsg,"Robot is non-responsive");
            if (logFD)
            {
               fprintf(logFD,"%s -2098 %s\n",timeStamp(), retMsg);
               fflush(logFD);
            }
            errLog = fopen(roboErrLog,"a");
            fprintf(errLog,"%s Failed to open Robot device %s after failed write\n",
                            timeStamp(),smsDevEntry->devName);
            fclose(errLog);
            return(-2098);
         }
         else
         {
            res = write(smsDev, cmd, strlen(cmd));
            res = write(smsDev, "\n", 1);
         }
      }
   }
   if ( ! strncmp(cmd,"openubgrip", strlen("openubgrip")) )
      Spinner('I',0);
   res = read_lan_timed(smsDev, retMsg, retLen, timeout*1000);
   if ( (res < retLen) && (res > 0) )
     retMsg[res] = '\0';
   DPRINT2(1,"sendToAs result= %d ret: %s\n",res, retMsg);

   if (res <= 0)
   {
      doFlush = 1;
      if (logFD)
      {
         fprintf(logFD,"%s -2098 Robot is non-responsive\n",timeStamp());
         fflush(logFD);
      }
      errLog = fopen(roboErrLog,"a");
      fprintf(errLog,"%s Failed to get Robot reply\n", timeStamp() );
      fclose(errLog);
      return(-2098); 
   }
   sscanf(retMsg,"%d", &res); 
   DPRINT1(1,"sendToAs return= %d\n",res);
   if (logFD)
   {
      fprintf(logFD,"%s RES: %s   Console: %d\n", timeStamp(),
                     retMsg, getErrFromRes(res));
      fflush(logFD);
   }
   if (res)
   {
      errLog = fopen(roboErrLog,"a");
      fprintf(errLog,"%s %s\n",timeStamp(), retMsg);
      fclose(errLog);
   }
   return(res);
}

static int getAsIntStatus(char *stat, int *val)
{
   char returnMsg[1024];
   char cmd[1024];
   char arg[64];
   int res;
   int sres;

   sprintf(cmd,"getstatus 1 %s", stat);
   res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
   if (res)
      return(res);
   sres = sscanf(returnMsg,"%[^:]:%d;",arg, val);
   DPRINT3(1,"getAsIntStatus sres= %d arg is '%s'  val is %d\n",sres ,arg, *val);
   return(res);
}

static int checkAsStatus(int stat, int *val)
{
   char returnMsg[1024];
   char cmd[1024];
   char arg[64];
   int res;
   int sres;

   sprintf(cmd,"checkstatus 1 %d", stat);
   res = sendToAS(cmd,returnMsg, 1024, LONG_MOTION_TIMEOUT);
   if (res)
      return(res);
   sres = sscanf(returnMsg,"%[^:]:%d;",arg, val);
   DPRINT3(1,"checkAsStatus sres= %d arg is '%s'  val is %d\n",sres ,arg, *val);
   return(res);
}

#define ESTOP -2053
#define USERACCESS -2051
#define MAGNETMOTION -2065
#define NOT_HOMED -2067

static int checkAccess(int stat)
{
   char returnMsg[1024];
   char cmd[1024];
   int res = ESTOP;
   int timeout = 601;
   int msg = 1;
   struct timespec req;
   int val;
   int lastRes = 0;

   res = getAsIntStatus("status", &val);
   if (res)
   {
      return(res);
   }
   res = val;
   if (res == NOT_HOMED)
   {
      lastRes = res;
      Spinner('M',ACQ_HOMESMP);
      res = sendToAS("index",returnMsg, 1024, LONG_MOTION_TIMEOUT);
      if ( ! res)
      {
         res = getAsIntStatus("status", &val);
         if (res)
         {
            return(res);
         }
      }
   }
   while ( ((res == ESTOP) || (res == USERACCESS) || (res == MAGNETMOTION)) && (timeout > 0) )
   {
      res = getAsIntStatus("status", &val);
      if (res)
      {
         return(res);
      }
      res = val;
      if ((res == ESTOP) || (res == USERACCESS) || (res == MAGNETMOTION))
      {
         if (res != lastRes)
         {
            lastRes = res;
            if (res == ESTOP)
               Spinner('M',ACQ_ESTOPSMP);
            else if (res == USERACCESS)
               Spinner('M',ACQ_ACCESSSMP);
            else
               Spinner('M',ACQ_MMSMP);
         }
         if (msg == 1)
         {
            strcpy(cmd,"displaymessage 2 Automation halts");
            sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
            msg = 2;
         }
         else if (msg == 2)
         {
            sprintf(cmd,"displaymessage 2 in %d sec.", timeout);
            sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
            msg = 3;
         }
         else
         {
            if ( (timeout % 2) || (res != MAGNETMOTION) )
            {
               sprintf(cmd,"displaymessage 2 Halt in %d", timeout);
            }
            else
            {
               strcpy(cmd,"displaymessage 2 Magnet motion");
            }
            sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
            if ( (timeout % 30) == 2)
               msg = 1;
         }
         req.tv_sec=1;
         req.tv_nsec=0;
         nanosleep( &req, NULL);
         timeout--;
      }
      if (AbortRobo) 
      {
         return(0);
      }
   }
   if (lastRes)
      Spinner('M',stat);
   return(res);
}

/*****************************************************************************
                         g e t A s S a m p l e
    Description:
    Retrieves sample from the magnet on the AS 48 / 96
    sample handler.  Calls hermes_get_sample() routine to actually
    perform the operation.

    Returns:
    0           - Success
    -1          - Error

*****************************************************************************/
#define MAXPATHL 256
#define NO_SAMPLE_IN_MAGNET -99
int getAsSample(char *str)
{
    char *value;
    int sample;
    
    AbortRobo = 0;

    /* Get the sample destination location */
    value = strtok(NULL," ");
    sample = atoi(value);
    DPRINT1(1,"getAsSample: Get Sample %d from Magnet\n",sample);

    if (sample == NO_SAMPLE_IN_MAGNET)
    {
       struct timespec req;
       /* Done! */
       DPRINT(1, "getAsSample: console says magnet is empty, getsample skipped\n");
       /* small delay to allow console software time to set up semaphores to receive response */
       req.tv_sec=0;
       req.tv_nsec=200000000;
       nanosleep( &req, NULL);
       reportRobotStat( 0 );
       return 0;
    }
    if ( (sample >= 0) && (sample <=96) )
    {
       int res, spnstat;
       char cmd[64];
       char returnMsg[1024];
       int err;
       int val;

       if (sample == 0)
       {
          res = getAsIntStatus("lastsample", &val);
          if (res)
          {
             err = getErrFromRes(res);
             DPRINT2(1, "getAsSample: sample %d failed err= %d\n", sample, err);
             reportRobotStat( SMPERROR+err );
             return(-1);
          }
          if (val <= 0)
          {
             DPRINT(1, "getAsSample: invalid lastsample; abort\n");
             reportRobotStat(SMPERROR+66);
             return(-1);
          }
          sample = val;
       }
       res = checkAccess(ACQ_RETRIEVSMP);
       if (res)
       {
          err = getErrFromRes(res);
          DPRINT2(1, "getAsSample: sample %d failed err= %d\n", sample, err);
          reportRobotStat( SMPERROR+err );
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       if (AbortRobo) 
       {
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       sprintf(cmd,"displaymessage 2 Retrieve %d",sample);
       res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
       if (res)
       {
          err = getErrFromRes(res);
          DPRINT2(1, "getAsSample: sample %d failed err= %d\n", sample, err);
          reportRobotStat( SMPERROR+err );
          return(-1);
       }
       res = checkAsStatus(sample, &val);
       if (res)
       {
          err = getErrFromRes(res);
          DPRINT2(1, "getAsSample: sample %d failed err= %d\n", sample, err);
          reportRobotStat( SMPERROR+err );
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       if (val)
       {
          err = 61; /* Sample tray position not available */
          DPRINT2(1, "getAsSample: sample %d failed err= %d\n", sample, err);
          reportRobotStat( SMPERROR+err );
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       if (AbortRobo) 
       {
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       spnstat = Spinner('E',0);
       if (spnstat == -1)
       {
          reportRobotStat(SMPERROR+SMPRMFAIL);
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       sprintf(cmd,"waitforsampleinub 20");
       res = sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
       if (AbortRobo) 
       {
          AbortRobo = 0;
          sprintf(cmd,"openubgrip 30");
          sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
          Spinner('L',sample);  /*  Reset loc */
          AbortRobo = 1;
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       if (res)
       {
          struct timespec req;

          sprintf(cmd,"openubgrip 30");
          res = sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);

          /*  There is a random error where the turbine gets twisted and
           *  the gripper fails. Just try a second time.
           */
          /* Sample insert takes about 10 secs. */
          req.tv_sec=11;
          req.tv_nsec=0;
          nanosleep( &req, NULL);
          spnstat = Spinner('E',0);
          if (spnstat == -1)
          {
             reportRobotStat(SMPERROR+SMPRMFAIL);
             sprintf(cmd,"displaymessage 2 Ready");
             res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
             return(-1);
          }
          sprintf(cmd,"waitforsampleinub 20");
          res = sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
          if (AbortRobo) 
          {
             AbortRobo = 0;
             sprintf(cmd,"openubgrip 30");
             sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
             Spinner('L',sample);  /*  sample is put back in magnet */
             AbortRobo = 1;
             sprintf(cmd,"displaymessage 2 Ready");
             res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
             return(-1);
          }
          if (res)
          {
             err = getErrFromRes(res);
             sprintf(cmd,"openubgrip 30");
             res = sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
             reportRobotStat(SMPERROR+err);
             Spinner('L',sample);  /*  sample is put back in magnet */
             sprintf(cmd,"displaymessage 2 Ready");
             res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
             return(-1);
          }
       }
       spnstat = Spinner('X',0); // Turn off eject air w/o delay
       sprintf(cmd,"getsample %d",sample);
       res = sendToAS(cmd,returnMsg, 1024, LONG_MOTION_TIMEOUT);
       if (res)
       {
          err = getErrFromRes(res);
          res = getAsIntStatus("ubgrippresent", &val);
          if (res || (val == 0) ) /* No sample in UB */
          {
             Spinner('L',NO_SAMPLE_IN_MAGNET);
          }
          else /* Sample still in UB. Insert it */
          {
             spnstat = Spinner('E',0);
             sprintf(cmd,"openubgrip 30");
             res = sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
             Spinner('L',sample);  /*  sample is put back in magnet */
          }
          DPRINT2(1, "getAsSample: sample %d failed err= %d\n", sample, err);
          reportRobotStat( SMPERROR+err );
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return -1;
       }
       else
       {
          DPRINT1(1, "getAsSample: sample %d retrieved\n", sample);
          reportRobotStat( 0 );
          sprintf(cmd,"displaymessage 1 Magnet empty");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          if (AbortRobo) 
          {
             /* If aborted but get action succeeded,
              * tell Console sample is gone.
              */
             AbortRobo = 0;
             Spinner('L',NO_SAMPLE_IN_MAGNET);
             AbortRobo = 1;
          }
          return 0;
       }
    }
    else
    {
        DPRINT1(1, "getAsSample: sample number %d not recognized", sample);
        reportRobotStat(SMPERROR+RTVBADNUMBER);
        return(-1);
    }
}

/*****************************************************************************
                         f a i l P u t A s S a m p l e
    Description:

    This command is called if the putAsSample and reputAsSample failed
    to deliver a sample into the magnet. It turns on the eject air and
    reports an error.

    Returns:
    0           - Success
    -1          - Error

*****************************************************************************/
int failPutAsSample(char *str)
{
    char *value;
    int sample;
    int res;
    int spnstat;
    char cmd[64];
    char returnMsg[1024];
    int err;
    struct timespec req;

    /* SIGUSR2 will set this to abort sample change */
    AbortRobo = 0;

    /* Get the sample source location */
    value = strtok(NULL," ");
    sample = atoi(value);
    DPRINT1(1,"failPutAsSample: Sample %d into Magnet\n",sample);


    spnstat = Spinner('E',0);
    if (spnstat == -1)
    {
       reportRobotStat(SMPERROR+SMPRMFAIL);
       return(-1);
    }
    sprintf(cmd,"waitforsampleinub 20");
    res = sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
    if (AbortRobo) 
    {
       AbortRobo = 0;
       spnstat = Spinner('I',0);
       AbortRobo = 1;
       return(-1);
    }
    spnstat = Spinner('I',0);
    /* Insert takes about 10 secs */
    req.tv_sec=11;
    req.tv_nsec=0;
    nanosleep( &req, NULL);
    if (res)
    {
       /* This error if sample stuck in upper barrel */
       err = getErrFromRes(res);
       DPRINT2(1, "failputAsSample: sample %d failed err= %d\n",
                   sample, err);
       reportRobotStat( SMPERROR+err );
    }
    else
    {
       /* This error if sample grabbed by UB gripper */
       /* Return sample to hole but ignore errors from getsample */
       sprintf(cmd,"getsample %d",sample);
       sendToAS(cmd,returnMsg, 1024, LONG_MOTION_TIMEOUT);
       DPRINT1(1, "failputAsSample: sample %d \n", sample);
       reportRobotStat( SMPERROR+54 );
       Spinner('L',NO_SAMPLE_IN_MAGNET);  /*  Reset loc */
       sprintf(cmd,"displaymessage 1 Magnet empty");
       res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
    }
    return(-1);
}

/*****************************************************************************
                         r e p u t A s S a m p l e
    Description:

    This command is called if the putAsSample failed to deliver a sample
    into the magnet. It turns on the eject air and then reinserts, trying
    to dislodge the sample from the upper barrel.

    Returns:
    0           - Success
    -1          - Error

*****************************************************************************/
int reputAsSample(char *str)
{
    char *value;
    int sample;

    /* SIGUSR2 will set this to abort sample change */
    AbortRobo = 0;

    /* Get the sample source location */
    value = strtok(NULL," ");
    sample = atoi(value);
    DPRINT1(1,"reputAsSample: rePut Sample %d into Magnet\n",sample);

    if ( (sample >= 1) && (sample <= 96) )
    {
       int res;
       int spnstat;
       char cmd[64];
       char returnMsg[1024];
       struct timespec req;
       int err;

       sprintf(cmd,"displaymessage 2 Insert %d",sample);
       res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
       if (res)
       {
          err = getErrFromRes(res);
          DPRINT2(1, "reputAsSample: sample %d failed err= %d\n", sample, err);
          reportRobotStat( SMPERROR+err );
          return -1;
       }
       spnstat = Spinner('E',0);
       if (spnstat == -1)
       {
          reportRobotStat(SMPERROR+SMPRMFAIL);
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       sprintf(cmd,"waitforsampleinub 20");
       res = sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
       if (AbortRobo) 
       {
          AbortRobo = 0;
          sprintf(cmd,"openubgrip 30");
          sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
          Spinner('L',sample);  /*  Reset loc */
          AbortRobo = 1;
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       sprintf(cmd,"openubgrip 30");
       res = sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
       if (res)
       {
          err = getErrFromRes(res);
          DPRINT2(1, "reputAsSample: sample %d failed err= %d\n", sample, err);
          reportRobotStat( SMPERROR+err );
          Spinner('L',NO_SAMPLE_IN_MAGNET);  /*  Reset loc */
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return -1;
       }
       else
       {
          DPRINT1(1, "reputAsSample: sample %d put in magnet\n", sample);
          /* Insert takes about 10 secs */
          req.tv_sec=11;
          req.tv_nsec=0;
          nanosleep( &req, NULL);
          reportRobotStat( 0 );
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return 0;
       }
    }
    else
    {
        DPRINT1(1, "reputAsSample: sample number %d not recognized", sample);
        reportRobotStat(SMPERROR+INSBADNUMBER);
        return(-1);
    }
    return(-1);
}

/*****************************************************************************
                         p u t A s S a m p l e
    Description:

    This command is responsible for placing the specified sample
    into the magnet.

    Returns:
    0           - Success
    -1          - Error

*****************************************************************************/
int putAsSample(char *str)
{
    char *value;
    int sample;

    /* SIGUSR2 will set this to abort sample change */
    AbortRobo = 0;

    /* Get the sample source location */
    value = strtok(NULL," ");
    sample = atoi(value);
    DPRINT1(1,"putAsSample: Put Sample %d into Magnet\n",sample);

    if ( (sample >= 1) && (sample <= 96) )
    {
       int res;
       int spnstat __attribute__((unused));
       char cmd[64];
       char returnMsg[1024];
       struct timespec req;
       int err;

       res = checkAccess(ACQ_LOADSMP);
       if (res)
       {
          err = getErrFromRes(res);
          DPRINT2(1, "putAsSample: sample %d failed err= %d\n", sample, err);
          reportRobotStat( SMPERROR+err );
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       if (AbortRobo) 
       {
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       sprintf(cmd,"displaymessage 2 Insert %d",sample);
       res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
       if (res)
       {
          err = getErrFromRes(res);
          DPRINT2(1, "putAsSample: sample %d failed err= %d\n", sample, err);
          reportRobotStat( SMPERROR+err );
          return -1;
       }
       sprintf(cmd,"putsample %d",sample);
       res = sendToAS(cmd,returnMsg, 1024, LONG_MOTION_TIMEOUT);
       if ( AbortRobo )
       {
          sprintf(cmd,"displaymessage 2 Retrieve %d",sample);
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          sprintf(cmd,"getsample %d",sample);
          res = sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return(-1);
       }
       if (res)
       {
          err = getErrFromRes(res);
          DPRINT2(1, "putAsSample: sample %d failed err= %d\n", sample, err);
          /* small delay to allow console software time to set up semaphores to receive response */
          req.tv_sec=0;
          req.tv_nsec=200000000;
          nanosleep( &req, NULL);
          Spinner('L',NO_SAMPLE_IN_MAGNET);  /*  Reset loc */
          reportRobotStat( SMPERROR+err );
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return -1;
       }
       spnstat = Spinner('E',0);
       req.tv_sec=2;
       req.tv_nsec=0;
       nanosleep( &req, NULL);
       sprintf(cmd,"openubgrip 30");
       res = sendToAS(cmd,returnMsg, 1024, MOTION_TIMEOUT);
       if (res)
       {
          err = getErrFromRes(res);
          DPRINT2(1, "putAsSample: sample %d failed err= %d\n", sample, err);
          reportRobotStat( SMPERROR+err );
          Spinner('L',NO_SAMPLE_IN_MAGNET);  /*  Reset loc */
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return -1;
       }
       else
       {
          int rack;
          int val;

          if (sample > 48)
          {
             rack = '2';
             val = sample - 48;
          }
          else
          {
             rack = '1';
             val = sample;
          }
          DPRINT1(1, "putAsSample: sample %d put in magnet\n", sample);
          reportRobotStat( 0 );
          sprintf(cmd,"displaymessage 1 Tray %c: %c%c (%d)", rack,
                   'A' + ((val+5) / 6) - 1,  '1' + (int) (val+5) % 6, sample );
          
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          sprintf(cmd,"displaymessage 2 Ready");
          res = sendToAS(cmd,returnMsg, 1024, NONMOTION_TIMEOUT);
          return 0;
       }
    }
    else
    {
        DPRINT1(1, "putAsSample: sample number %d not recognized", sample);
        reportRobotStat(SMPERROR+INSBADNUMBER);
        return(-1);
    }
    return(-1);
}


/*****************************************************************************
                         g e t H r m S a m p l e
    Description:
    Retrieves sample from the magnet on the Hermes high throughput
    sample handler.  Calls hermes_get_sample() routine to actually
    perform the operation.

    Returns:
    0           - Success
    -1          - Error

*****************************************************************************/
#define MAXPATHL 256
int getHrmSample(char *str)
{
    char *value, rsp[32];
    int sample, argSample;
    extern int hermes_get_sample(int smp, int pcsSlot);
    
    /* SIGUSR2 will set this to abort sample change */
    //SDM-- fixes protune errors, but not sure about other ramifications
    AbortRobo = 0;
    /* Check for abort, if so then just return */
/*     if (AbortRobo != 0)  */
/*     { */
/*       DPRINT(0, "getHrmSample: User Aborted"); */
/*       return -1; */
/*     } */

    /* Get the sample destination location */
    value = strtok(NULL," ");
    argSample = atoi(value);
    DPRINT1(1,"getHrmSample: Get Sample %d from Magnet\n",argSample);

    /*
     * If sample == 0, Vnmr wants to get rid of the sample in
     *   the magnet (if any exists).  So go ahead and call the
     *   hermes_get_sample() routine to see if anything is in there.
     */

    sample = argSample;
    if ((sample == 0) || (sample > 999))
    {
        extern int rack768AS, zone768AS, well768AS;

        /* reportRobotStat() will set this to 1 */
        reportRobotStatDone = 0;

        rack768AS = (sample / 1000000L);
        zone768AS = (sample / 10000L % 100);
        well768AS = (sample % 1000);

        /* Check if there is a sample at the bottom of the magnet */
        if (SendRabbit("<UBSTAT1>\n", rsp) != 0) {
            reportRobotStat(SMPERROR+SMPRMFAIL);
            return(-1);
        }

        /* Yes - Tell Rabbit to Eject the Sample from the Magnet */
        if (strstr(rsp, "ON") != NULL) {
            if ((SendRabbit("<EJECT>\n", rsp) != 0) ||
                (strstr(rsp, "SUCCESS") == NULL)) {
                reportRobotStat(SMPERROR+SMPRMFAIL);
                return(-1);
            }
        }

        /* Now check for a sample at the top of the magnet */
        if (SendRabbit("<UBSTAT0>\n", rsp) != 0) {
            reportRobotStat(SMPERROR+SMPRMFAIL);
            return(-1);
        }

        /* Yes - call hermes_get_sample(), (if not, just return success) */
        if (strstr(rsp, "ON") != NULL) {
            int pcsSlot=0, stat=0;

            DPRINT(0, "Sample is at the top of the magnet.\n");

            /* TODO: Need to ask the gilson object to do this too */
            /* Clear i/o channel */
            smsDevEntry->ioctl(smsDev, IO_FLUSH);

            pcsSlot = 0;
            stat = hermes_get_sample(sample, pcsSlot);

            /* No Errors */
            if (stat == 0) {
                extern void schedule_new_samples(void);

                DPRINT(0, "getHrmSample: sample retrieved.\r\n");

                /* Schedule any free PCS slots if possible */
                schedule_new_samples();

                /*  Always Turn the Air Off */
                if ((SendRabbit("<AIR->\n", rsp) != 0) ||
                    (strstr(rsp, "ACK") == NULL)) {
                    reportRobotStat(SMPERROR+SMPRMFAIL);
                    return(-1);
                }

                /* Done! */
                reportRobotStat( 0 );
                return 0;
            }

            /* User Aborted */
            else if (stat == HRM_OPERATION_ABORTED) {
                DPRINT(0, "getHrmSample:  User Aborted");
                DPRINT(0, "getHrmSample: returning error code "
                          "(SMPERROR+SMPRMFAIL)\r\n");

                reportRobotStat(SMPERROR+SMPRMFAIL);
                return(-1);
            }

            /* Error */
            else {
                DPRINT1(0,"getHrmSample: returning error code %d\r\n",stat);
                reportRobotStat(stat);
                return(-1);
            }
        }
        else {
            DPRINT(0, "getHrmSample: magnet is empty, no retrieval needed\n");
            reportRobotStat( 0 );
            return 0;
        }
    }
    else {
        DPRINT1(0, "getHrmSample: sample number %d not recognized", sample);
        reportRobotStat(SMPERROR+SMPRMFAIL);
        return(-1);
    }
}

/*****************************************************************************
                         p u t H r m S a m p l e
    Description:

    This command is responsible for placing the specified sample
    into the magnet.  Calls hrm_put_sample() routine to actually
    perform the operation.

    Returns:
    0           - Success
    -1          - Error

*****************************************************************************/
int putHrmSample(char *str)
{
    char *value;
    int sample, argSample;
    extern int hermes_put_sample(int smp, int *pcsSlot);

    /* SIGUSR2 will set this to abort sample change */
    AbortRobo = 0;

    /* Get the sample source location */
    value = strtok(NULL," ");
    argSample = atoi(value);
    DPRINT1(1,"putHrmSample: Put Sample %d into Magnet\n",argSample);

    /* At this point argSample is more likely the right one */
    sample = argSample;
    if ( sample > 999)
    {
        extern int rack768AS, zone768AS, well768AS;
        int pcsSlot=0, stat=0;

        /* TODO: Need to ask the gilson object to do this too */
        /* Clear i/o channel */
        smsDevEntry->ioctl(smsDev, IO_FLUSH);

        /* reportRobotStat() will set this to 1 */
        reportRobotStatDone = 0;

        rack768AS = (sample / 1000000L);
        zone768AS = (sample / 10000L % 100);
        well768AS = (sample % 1000);

        stat = hermes_put_sample(sample, &pcsSlot);

        /* No Errors */
        if (stat == 0) {
            DPRINT1(0, "putHrmSample: sample is in turbine %d and "
                       "has been inserted into the magnet.\r\n", pcsSlot);

            /* Done! */
            if (reportRobotStatDone == 0)
                reportRobotStat( 0 );
            return 0;
        }

        /* User Aborted */
        else if (stat == HRM_OPERATION_ABORTED) {
            DPRINT(0, "putHrmSample:  User Aborted\n");
            DPRINT(0, "putHrmSample: returning error code "
                      "(SMPERROR+SMPINFAIL)\r\n");

            reportRobotStat(SMPERROR+SMPINFAIL);
            return(-1);
        }

        /* Error */
        else {
            DPRINT1(0,"putHrmSample: returning error code %d\r\n",stat);
            reportRobotStat(stat);
            return(-1);
        }
    }
    return 0;
}

/*****************************************************************************
                         g e t G i l S a m p l e
    Description:
    Gilson Liquid Handler Get Sample.
    Initializes i/o channel, resets and homes gilson if necessary.
    Reads the /vnmr/asm/racksetup file if needed.
    Calls the /vnmr/asm/tcl/get.tcl TCL script.

    Returns:
    0           - Success
    -1          - Error

*****************************************************************************/
int getGilSample(char *str)
{
    char Tcl_script[128];

    char *value;
    int status;
    int sample,argSample;
    int errorCode;

    /* SIGUSR2 will set this to abort sample change */
    AbortRobo = 0;

    /* Get the sample destination location */
    value = strtok(NULL," ");
    argSample = atoi(value);
    DPRINT1(1,"getGilSample: Get Sample %d from Magnet\n",argSample);

    /* Initialize i/o channel & home gilson */
    if (initGilson215(smsDevEntry->devName, 22, 29, 3) == -1)
    {
       errLogRet(LOGOPT, debugInfo,
                 "getGilSample: Gilson Initialization Failed\n");
       reportRobotStat(SMPERROR+SMPTIMEOUT);
       return(-1);
    }

    /* If any Axis or pump is not powered then reset and home gilson */
    gilPowerUp();

    if ( !racksBeenRead() )
    {
        DPRINT(1,"getGilSample: Read in racksetup file,"
                 " has not been read yet \n");
        if ( readRacks("/vnmr/asm/racksetup") )
        {
            errLogRet(LOGOPT, debugInfo,
                      "getGilSample: '/vnmr/asm/racksetup'  "
                      "Rack Initialization Failed\n");
            reportRobotStat(SMPERROR + RACKNSAMPFILEERROR);
            return(-1);
        }
    }

#ifdef MAPEXP
#ifndef STDALONE
    sample = getStatAcqSample();

    /* Get user name for Spinner() cmds */
    getStatUserId(UserName, UNAMESIZE);

    DPRINT2(1,"User: '%s', Get Sample %d from Magnet\n", UserName, sample);
#endif
#endif

    /* At this point, argSample is more likely the right one */
    sample = argSample;
 
    if ( sample > 999)
    {
        /* Build SampleInfoFile string from sample location components */
        if ( (status = gilSampleSetup((long)sample, "")) > 0)
        {
            reportRobotStat( status );
            return(-1);
        }

        /* Clear i/o channel */
        smsDevEntry->ioctl(smsDev, IO_FLUSH);

        /* reportRobotStat() will set this to 1 */
        reportRobotStatDone = 0;

        /* TCL Script controls the logic of the Get Operation    */
        /* Note that this script may be modified by the Customer */
        strcpy(Tcl_script,"/vnmr/asm/tcl/get.tcl");

        if ( (errorCode = InterpScript(Tcl_script)) != 0)
        {
            if (reportRobotStatDone == 0)
                reportRobotStat(errorCode);
            return(-1);
        }   
    }

    if (reportRobotStatDone == 0)
        reportRobotStat( 0 );

    return 0;
}

/*****************************************************************************
                         p u t G i l S a m p l e
    Description:
    Gilson Liquid Handler put Sample
    Initializes i/o channel, resets and homes gilson if necessary.
    Reads the /vnmr/asm/racksetup file if needed.
    Calls the /vnmr/asm/tcl/put.tcl TCL script.

    Returns:
    0           - Success
    -1          - Error

*****************************************************************************/
int putGilSample(char *str)
{
    char Tcl_script[128];

    char *value;
    int status;
    int sample,argSample;
    int errorCode;

    /* SIGUSR2 will set this to abort sample change */
    AbortRobo = 0;

    /* Get the sample source location */
    value = strtok(NULL," ");
    argSample = atoi(value);
    DPRINT1(1,"putGilSample: Put Sample %d into Magnet\n",argSample);

    /* Initialize i/o channel & home gilson */
    if (initGilson215(smsDevEntry->devName, 22, 29, 3) == -1)
    {
        errLogRet(LOGOPT, debugInfo,
                  "putGilSample: Gilson Initialization Failed\n");
        reportRobotStat(SMPERROR+SMPTIMEOUT);
        return(-1);
    }

    /* If any Axis or pump is not powered then reset and home gilson */
    gilPowerUp();

    if ( readRacks("/vnmr/asm/racksetup") )
    {
        errLogRet(LOGOPT, debugInfo,
                  "putGilSample: '/vnmr/asm/racksetup'  "
                  "Rack Initialization Failed\n");
        reportRobotStat(SMPERROR + RACKNSAMPFILEERROR);
        return(-1);
    }

#ifdef MAPEXP
#ifndef STDALONE
    sample = getStatAcqSample();

    /* Get user name for Spinner() cmds */
    getStatUserId(UserName, UNAMESIZE);

    DPRINT2(1,"User: '%s', Put Sample %d into Magnet\n", UserName, sample);
#endif
#endif

    /* At this point argSample is more likely the right one */
    sample = argSample;

    if ( sample > 999)
    {
        if ( (status = gilSampleSetup((long)sample, "")) > 0)
        {
            reportRobotStat( status );
            return(-1);
        }

        /* Clear i/o channel */
        smsDevEntry->ioctl(smsDev, IO_FLUSH);

        /* reportRobotStat() will set this to 1 */
        reportRobotStatDone = 0;

        /* TCL Script controls the logic of the Put Operation    */
        /* Note that this script may be modified by the Customer */
        strcpy(Tcl_script,"/vnmr/asm/tcl/put.tcl");

        if ( (errorCode = InterpScript(Tcl_script)) != 0)
        {
            if (reportRobotStatDone == 0)
                reportRobotStat(errorCode);
            return(-1);
        }   
    }

    if (reportRobotStatDone == 0)
        reportRobotStat( 0 );

    return 0;
}

/*****************************************************************************
                         g e t Z y S a m p l e
    Description:
    SM/SMS/Carousel get Sample

*****************************************************************************/
void getZySample(char *str)
{
    char *value;
    int robstat,spnstat,stat;
    int sample,argSample;

    /* SIGUSR2 will set this to abort sample change */
    AbortRobo = 0;

    /* Get the sample destination location */
    value = strtok(NULL," ");
    argSample = atoi(value);
    DPRINT1(1,"Get Sample %d from Magnet\n",argSample);

#ifdef MAPEXP
    sample = getStatAcqSample();

    /* Get user name for Spinner() cmds */
    getStatUserId(UserName, UNAMESIZE);
    DPRINT2(1,"User: '%s', Get Sample %d from Magnet\n", UserName, sample);
#endif

    /* At this point argSample is more likely the right one */
    sample = argSample;

#ifdef TESTING

    DPRINT(1,"getSample test: Eject, delay 3 seconds.\n");
    Spinner('E',0);  /*  Eject Sample */
    delayAwhile(3);  /* delay 3 sec */
    DPRINT(1,"getSample test: Insert\n");
    Spinner('I',0);  /*  Insert Sample */
    reportRobotStat(argSample);

#else
    /* Clear i/o channel */
    smsDevEntry->ioctl(smsDev, IO_FLUSH);

    /* If never use changer find out what type it is */
    DPRINT1(1,"SMS_Changer: %d\n",SMS_Changer);
    if (SMS_Changer == -1)
    {
        DPRINT(1,"Test For Sample Changer Type\n");

        /* Send CR to robot */
        smsDevEntry->write(smsDev, &cr, 1);

        /* Wait for all chars to be sent */
        smsDevEntry->ioctl(smsDev, IO_DRAIN);

        /* Timeout after 3 seconds */
        stat = cmdAck(3);

        /* Energize robot */
        if (stat == SMPTIMEOUT)
        {
            reportRobotStat(SMPERROR+stat);
            parkRobot();
            return;
        }

        SMS_Changer = (stat != 0) ? ASMCHANGER : SMSCHANGER;
        DPRINT1(1,"Tested for Changer Type, Type = '%s'\n",
                (SMS_Changer == ASMCHANGER) ? "ASM" : "SMS");

#ifdef VER22SMSDIC
        if ( SMS_Changer == SMSCHANGER )
        {
            int rtype;
            rtype = robot('Q',0);  /* Query changer for type */
            DPRINT1(1,">>>>>>>>> SMS changer type: %d\n",rtype);
        }
#endif
    } 
   
    if (SMS_Changer == ASMCHANGER) 
    { 
        /* Energize robot */
        if ( (robstat = robot('E',0)) )
        {
            reportRobotStat(SMPERROR+robstat);
            parkRobot();
            return;
        }

        /* Center over magnet */
        if ( (robstat = robot('C',0)) ) 
        {
            reportRobotStat(SMPERROR+robstat);
            parkRobot();
            return;    /* Center over Mag */
        }
    }


    /* If NMS then don't bother with the eject air */
    if (SMS_Changer != NMSCHANGER)
    {
        /* This would be equivalent to:               */
        /*    spinner('B',..,..) + spinner('E',..,..) */

        /*  Eject Sample */
        DPRINT(1,"getSample: Eject\n");
        spnstat = Spinner('E',0);
        if (spnstat == -1)
        {
            reportRobotStat(SMPERROR+SMPSPNFAIL);
            parkRobot();
            return;    /* Center over Mag */
        }
    }

    /* Ready to be implemented when test time is available */
#define NMS_SPINNER_CHANGE
#ifdef NMS_SPINNER_CHANGE
    else  /* (SMS_Changer == NMSCHANGER) */
    {
        /* Set Speed to Zero */
        spnstat = Spinner('S',0);
        if (spnstat == -1)
        {
            reportRobotStat(SMPERROR+SMPSPNFAIL);
            parkRobot();
            return;    /* Center over Mag */
        }
    }
#endif

    /* Now how to report spinner error & not continue ??? */
 
    /* Grasp position */
    DPRINT(1,"getSample: robot F\n");
    if ( (robstat = robot('F',0)) )
    {
         reportRobotStat(SMPERROR+robstat);
         parkRobot();
         return;    /* Center over Mag */
    }

    if (SMS_Changer == ASMCHANGER) 
    {
        /* Grasp sample */
        if ( (robstat = robot('G',0)) )
        {
            reportRobotStat(SMPERROR+robstat);
            parkRobot();
            return;    /* Center over Mag */
        }

        /* Delay 1 sec to grip */
        delayAwhile(1);
    }

    /* Goto sample # */
    DPRINT1(1,"getSample: robot V %d\n",sample);
    if ( (robstat = startRobot('V',sample)) )
    {
        reportRobotStat(SMPERROR+robstat);
        parkRobot();
        return;    /* Center over Mag */
    }

    /* if NMS then don't bother with the insert air */
    if (SMS_Changer != NMSCHANGER) 
    {
        /*  Turnoff Air */
        DPRINT(1,"getSample: Insert\n");
        spnstat = Spinner('I',0);
        if (spnstat == -1)
        {
            reportRobotStat(SMPERROR+SMPSPNFAIL);
            parkRobot();
            return;    /* Center over Mag */
        }
    }

    /* Goto sample # */
    DPRINT(1,"getSample: Wait for V to complete\n");
    if ( (robstat = robotCmplt()) )
    {
        reportRobotStat(SMPERROR+robstat);
        parkRobot();
        return;    /* Center over Mag */
    }
 
    /* Open Grasp */
    if (SMS_Changer == ASMCHANGER) 
    {
        if ( (robstat = robot('O',0)) )
        {
            reportRobotStat(SMPERROR+robstat);
            parkRobot();
            return;    /* Center over Mag */
        }
    }
 
    /* Confirmation that sample has been removed is done in console */

    DPRINT(1,"getSample: Park Robot\n");
    parkRobot();

    reportRobotStat( 0 );

#endif

    /* Return sample-changer error # or 94 if spinner error */

    return;
} 

/*****************************************************************************
                         p u t Z y S a m p l e
    Description:
    Command sample changer to load sample into magnet.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 9/21/94

*****************************************************************************/
void putZySample(char *str)
{
    char *value;
    int robstat,spnstat;
    int sample;

    /* SIGUSR2 will set this to abort sample change */
    AbortRobo = 0;

    /* Get the sample source location */
    value = strtok(NULL," ");
    sample = atoi(value);
    DPRINT1(1,"Put Sample %d into Magnet\n",sample);

#ifdef TESTING

    DPRINT(1,"putSample test: Eject, delay 3 seconds.\n");
    Spinner('E',0);  /*  Eject Sample */
    delayAwhile(3);  /* delay 3 sec */
    DPRINT(1,"putSample test: Insert\n");
    Spinner('I',0);  /*  Insert Sample */
    reportRobotStat(sample);

#else

    DPRINT1(1,"putSample: SMS_Changer: %d\n",SMS_Changer);

    /* Clear i/o channel */
    smsDevEntry->ioctl(smsDev, IO_FLUSH);

    if (SMS_Changer == ASMCHANGER)
    {
        /* Energize robot */
        if ( (robstat = robot('E',0)) )
        {
            reportRobotStat(SMPERROR +
                ((robstat < 80) ? robstat + 20 : robstat));
            parkRobot();
            return;
        }
    }

    /* Get New Sample */
    DPRINT1(1,"putSample: robot V %d\n",sample);
    if ( (robstat = robot('V',sample)) )
    {
        reportRobotStat(SMPERROR+((robstat < 80) ? robstat + 20 : robstat));
        parkRobot();
        return;    /* Center over Mag */
    }

    if (SMS_Changer == ASMCHANGER) 
    { 
        /* Grasp */
        if ( (robstat = robot('G',0)) )
        {
            reportRobotStat(SMPERROR +
                ((robstat < 80) ? robstat + 20 : robstat));
            parkRobot();
            return;
        }

        /* Center over magnet */
        if ( (robstat = robot('C',0)) )
        {
            reportRobotStat(SMPERROR +
                ((robstat < 80) ? robstat + 20 : robstat));
            parkRobot();
            return;    /* Center over Mag */
        }
    }

    /* if NMS then don't bother with the eject air */
    if (SMS_Changer != NMSCHANGER) 
    {
        /* Eject Sample */
        DPRINT(1,"GetSample: Eject\n");
        spnstat = Spinner('E',0);
        if (spnstat == -1)
        {
            reportRobotStat(SMPERROR+SMPSPNFAIL);
            parkRobot();
            return;
        }
    }

    /* Drop position  */
    DPRINT(1,"putSample: robot M\n");
    if ( (robstat = robot('M',0)) )
    {
        reportRobotStat(SMPERROR+((robstat < 80) ? robstat + 20 : robstat));
        parkRobot();
        return;    /* Center over Mag */
    }

    /* Open Grasp */
    if (SMS_Changer == ASMCHANGER) 
    {
        if ( (robstat = robot('O',0)) )
        {
            reportRobotStat(SMPERROR+robstat);
            parkRobot();
            return;    /* Center over Mag */
        }
    }
 
    DPRINT(1,"putSample: Park Robot\n");
    parkRobot();

    reportRobotStat( 0 );

#endif

    /* Return sample-changer error # or 94 if spinner error */

    return;
}


/*****************************************************************************
                           g e t S a m p l e
  
    Description:
    Command sample changer to retrieve sample from magnet.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 9/21/94

*****************************************************************************/
int getSample(char *str)
{
    if (SMS_Changer == GILSON_215)
        getGilSample(str);          /* Gilson 215 liquid handler (LC Probe) */
    else if (SMS_Changer == HRMCHANGER)
    {
        int stat;

        abortActive = 1;
        AbortRobo = 0;
        stat = getHrmSample(str); /* Hermes high-throughput robot */
        abortActive = 0;
        AbortRobo = 0;
        return (stat);
    }
    else if (SMS_Changer == AS4896CHANGER)
        return (getAsSample(str)); /* AS 48 / 96 robot */
    else
        getZySample(str);           /* ASM/SMS/Carousel */
    return 0;
}


/*****************************************************************************
                           p u t S a m p l e
    Description:
    Command sample changer to load sample into magnet.

    Returns:
    0           - Success
    -1          - Error

    Author:     Greg Brissey 9/21/94

*****************************************************************************/
int putSample(char *str)
{
    if (SMS_Changer == GILSON_215)
        putGilSample(str);          /* Gilson 215 liquid handler (LC Probe) */
    else if (SMS_Changer == HRMCHANGER)
    {
        int stat;

        abortActive = 1;
        AbortRobo = 0;
        stat = putHrmSample(str); /* Hermes high-throughput robot */
        abortActive = 0;
        AbortRobo = 0;
        return (stat);
    }
    else if (SMS_Changer == AS4896CHANGER)
        return (putAsSample(str)); /* AS 48 / 96 robot */
    else
        putZySample(str);           /* ASM/SMS/Carousel */
    return 0;
}

int reputSample(char *str)
{
    if (SMS_Changer == AS4896CHANGER)
        return (reputAsSample(str)); /* AS 48 / 96 robot */
    else
        return 0;
}

int failPutSample(char *str)
{
    if (SMS_Changer == AS4896CHANGER)
        return (failPutAsSample(str)); /* AS 48 / 96 robot */
    else
        return 0;
}

static int last_sample = 0;

/*****************************************************************************
                           q u e u e S a m p l e
  
    Description:
    Only applicable to the Hermes high-throughput sample changer.
    Responsible for queuing the given sample into the Hermes scheduler.

    Returns:
    0           - Success
    -1          - Error

*****************************************************************************/
int queueSample(char *str)
{
    int current_sample = 0;
    char *rack, *zone, *well, *exptime, *pcstime;
    extern int queue_sample(int rack, int zone, int well,
                            int exptime, int pcstime);

    if (SMS_Changer != HRMCHANGER) {
        DPRINT(1, "ROBOPROC RECEIVED QUEUESMP MESSAGE\n");
        return (0);
    }

    rack = strtok(NULL, " ");
    zone = strtok(NULL, " ");
    well = strtok(NULL, " ");
    exptime = strtok(NULL, " ");
    pcstime = strtok(NULL, "\n");

    current_sample = (atoi(rack) * 1000000L) + (atoi(zone) * 10000L) + atoi(well);
    if (current_sample == last_sample)
        return (0);
    else
        last_sample = current_sample;

    DPRINT5(1, "ROBOPROC RECEIVED QUEUESMP %s %s %s %s %s MESSAGE\n",
            rack, zone, well, exptime, pcstime);

    return (queue_sample(atoi(rack), atoi(zone), atoi(well),
                         atoi(exptime), atoi(pcstime)));
}

int userCmd(char *str)
{
    char *cmd;
    char returnMsg[1024];
    int ret;

    AbortRobo = 0;
    cmd = strtok(NULL,"\n");
    DPRINT1(1, "ROBOPROC userCmd MESSAGE = '%s'\n", str);
    DPRINT1(1, "ROBOPROC userCmd cmd = '%s'\n", cmd);
    if ( ! strncmp(cmd,"outfile",7) )
    {
       FILE *outFD;
       char outfile[1024];
       char outfile2[1024];
       char cmd2[1024];

       sscanf(cmd,"outfile %[^;]; %[^;];",outfile,cmd2);
       DPRINT1(1, "ROBOPROC userCmd outfile = %s\n", outfile);
       DPRINT1(1, "ROBOPROC userCmd cmd = %s\n", cmd2);
       if ( ! strcmp(cmd2,"term") )
          terminate(cmd2);
       ret = sendToAS(cmd2, returnMsg, 1024, LONG_MOTION_TIMEOUT);
       strcpy(outfile2,outfile);
       strcat(outfile2,".tmp");
       outFD = fopen(outfile2,"w");
       if (!ret)
       {
          fprintf(outFD,"%s\n",returnMsg);
       }
       else
       {
          int dummy;
          sscanf(returnMsg,"%d %[^\n]", &dummy, cmd2);
          fprintf(outFD,"%d %s\n",SMPERROR+getErrFromRes(ret), cmd2);
       }
       fclose(outFD);
       chmod(outfile2, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );
       unlink(outfile);
       ret = link(outfile2,outfile);
       unlink(outfile2);
    }
    else
    {
       if ( ! strcmp(cmd,"term") )
          terminate(cmd);
       ret = sendToAS(cmd, returnMsg, 1024, LONG_MOTION_TIMEOUT);
    }
    DPRINT1(1, "ROBOPROC userCmd resMsg = '%s'\n", returnMsg);
    return (0);
}


/*****************************************************************************
                            r o b o C l e a r
  
    Description:
    Only implemented on the Hermes high-throughput sample changer.
    Autoproc signals this operation when it detects the end of an
    automation run.  At this time, Roboproc should clear all samples
    out of the sample changer and clear any pending samples in the
    ready or wait queues.

    Returns:
    0           - Success
    -1          - Error

*****************************************************************************/
int roboClear(char *str)
{
    extern int hermes_clear_robot();

    DPRINT(1, "ROBOPROC RECEIVED ROBOCLEAR MESSAGE\n");
    last_sample = 0;

    if (SMS_Changer != HRMCHANGER)
        return (0);

    return hermes_clear_robot();
}

/* Initializing command sent by Expproc
 * until messageQ is ready. It is simply ignored
 * by roboproc
 */
int roboReady(char *str)
{
   DPRINT(1,"Roboproc: initializing roboReady message.\n");
   return(0);
}

/**************************** MISCELLANEOUS *********************************/
void ShutDownProc(void)
{
    extern GILSONOBJ_ID pGilObjId;

#ifdef MAPEXP
    expStatusRelease();
#endif
    shutdownComm();
    gilsonDelete(pGilObjId);
    /* resetRoboproc(char *str) */
}

int resetRoboproc(char *str)
{
    DPRINT(1,"Reset: Roboproc.\n");
    return 0;
}

int terminate(char *str)
{
    ShutDownProc();
    exit(0);
    return 0;
}

int debugLevel(char *str)
{
    char *value;
    int  val;
    extern int DebugLevel;

    value = strtok(NULL," ");
    val = atoi(value);

    DebugLevel = val;
    DPRINT1(1,"debugLevel: New DebugLevel: %d\n",val);

    return(0);
}

int mapIn(char *str)
{
#ifdef MAPEXP
    char* filename;

    filename = strtok(NULL," ");

    DPRINT1(1,"mapIn: map Shared Memory Segment: '%s'\n",filename);

    ShrExpInfo = shrmCreate(filename, 1, (unsigned long)
                            sizeof(SHR_EXP_STRUCT));
    if (ShrExpInfo == NULL)
    {
        errLogSysRet(LOGOPT, debugInfo, "mapIn: shrmCreate() failed:");
        return(-1);
    }
    if (ShrExpInfo->shrmem->byteLen < sizeof(SHR_EXP_STRUCT))
    {
        /* Hey, this file is not a shared Exp Info file */
        shrmRelease(ShrExpInfo); /* release shared Memory */
        unlink(filename);        /* remove filename that shared Mem created */
        ShrExpInfo = NULL;
        expInfo = NULL;
        return(-1);
    }

#ifdef DEBUG
    shrmShow(ShrExpInfo);
#endif

    expInfo = (SHR_EXP_INFO) shrmAddr(ShrExpInfo);
#else
    DPRINT(1,"mapIn: map Shared Memory Segment\n");
#endif

    return(0);
}

int mapOut(char *str)
{
/*
    char* filename;

    filename = strtok(NULL," ");

    DPRINT1(1,"mapOut: unmap Shared Memory Segment: '%s'\n",filename);
*/

#ifdef MAPEXP
    if (ShrExpInfo != NULL)
    {
        shrmRelease(ShrExpInfo);
        ShrExpInfo = NULL;
        expInfo = NULL;
    }
#else
    DPRINT(1,"mapOut: unmap Shared Memory Segment\n");
#endif
    return(0);
}

