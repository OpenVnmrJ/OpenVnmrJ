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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include "errLogLib.h"
#include "msgQLib.h"
#include "acquisition.h"
#include "acqcmds.h"
#include "errorcodes.h"
#include "iofuncs.h"

extern char HostName[];
extern char systemdir[];

/************************************************************************
 * Declarations for routines that have no include file for us to use.
 ************************************************************************/
extern int getStatAcqSample(void);                   /* statfuncs.c     */
extern void expStatusRelease(void);                  /* statfuncs.c     */
extern int racksBeenRead(void);                      /* robofuncs.c     */
extern void shutdownComm(void);                      /* roboproc.c      */
extern void masSpinner(int *paramvec, int *index, int count);
extern int getMasSpeed();
extern int masParsChanged(char *cmd);

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

/**************************** GLOBAL VARIABLES ******************************/
/* SIGUSR2 will set this to abort sample change */
int AbortRobo;


/****************************** STATIC GLOBALS ******************************/

/* Type of sample changer we are hooked up to */
static int SMS_Changer = -1;
static char portPath[1024];



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

static void
alarmItrp(int signal)
{
  processNonInterrupt( SIGALRM, NULL );
  return;
}


void masUpdate(int *sig)
{
   int speed;
   static int lastspeed = -1;
   char cmd[256];
   DPRINT(1,"masUpdate called\n");
   if (access(portPath, R_OK))
   {
      DPRINT(1,"portPath removed; exiting\n");
      deliverMessageQ("Expproc", "masspeed -1\n");
      terminate(NULL);
   }
   speed = getMasSpeed();
   if (speed != lastspeed)
   {
      sprintf(cmd,"masspeed %d\n",speed);
      deliverMessageQ("Expproc", cmd);
      lastspeed = speed;
   }
   checkModuleChange();
   if ( masParsChanged(cmd))
   {
      deliverMessageQ("Expproc", cmd);
   }
   alarm(3);
}

void initMas()
{
   static int init = 0;
#ifdef XXX
   char str[1024];
   char *cmdstr;
#endif

   if (init)
      return;
   strcpy(portPath,systemdir);      /* typically /vnmr/smsport */
   strcat(portPath,"/masport");
   DPRINT1(1,"portPath: '%s'\n",portPath);
   if (access(portPath, R_OK))
   {
      DPRINT(1,"portPath missing; exiting\n");
      terminate(NULL);
   }
   /* Setup Device Communication */
   smsDevEntry = &devTable[2];
   DPRINT1(1,"devName: '%s'\n",smsDevEntry->devName);
   SMS_Changer = SMSCHANGER;
   if ((smsDev = smsDevEntry->open(smsDevEntry->devName)) < 0) {
       errLogSysRet(ErrLogOp, debugInfo, "open comm device");
   }
   setMasSpeedPort(smsDev);

   registerAsyncHandlers( SIGALRM, alarmItrp, masUpdate);
#ifdef XXX
strcpy(str,"user start\n");
   cmdstr = strtok(str," ");
userCmd(str);
strcpy(str,"user speed 1000\n");
   cmdstr = strtok(str," ");
userCmd(str);
#endif
   alarm(3);
}

int userCmd(char *str)
{
    char *cmd, *arg;
    int index;
    int pars[3];
    

    pars[2] = 0;
    cmd = strtok(NULL," \n");
    index = 0;
    DPRINT1(1, "ROBOPROC userCmd MESSAGE = '%s'\n", str);
    DPRINT1(1, "ROBOPROC userCmd cmd = '%s'\n", cmd);
#ifdef XXX
    if ( ! strncmp(cmd,"outfile",7) )
    {
       char outfile[1024];
       char cmd2[128];

       sscanf(cmd,"outfile %[^;]; %[^;];",outfile,cmd2);
       DPRINT1(1, "ROBOPROC userCmd outfile = %s\n", outfile);
       DPRINT1(1, "ROBOPROC userCmd cmd2= %s\n", cmd2);
       cmd = strtok(cmd2," \n");
       DPRINT1(1, "ROBOPROC userCmd cmd = '%s'\n", cmd);
    }
#endif
    if ( ! strcmp(cmd,"start") )
    {
       pars[0] = MASON;
       pars[1] = 0;
    DPRINT1(1, "ROBOPROC start par[0] = %d\n", pars[0]);
       masSpinner(pars, &index, 1);
    }
    else if ( ! strcmp(cmd,"stop") )
    {
       pars[0] = MASOFF;
       pars[1] = 0;
       masSpinner(pars, &index, 1);
    }
    else if ( ! strcmp(cmd,"speed") )
    {
       arg = strtok(NULL," \n");
       pars[0] = SETSPD;
       pars[1] = atoi(arg);
    DPRINT2(1, "ROBOPROC speed par[0] = %d par[1]= %d\n", pars[0], pars[1]);
       masSpinner(pars, &index, 2);
    }
    else if ( ! strcmp(cmd,"bearmax") )
    {
       arg = strtok(NULL," \n");
       pars[0] = SETBEARMAX;
       pars[1] = atoi(arg);
       masSpinner(pars, &index, 2);
    }
    else if ( ! strcmp(cmd,"bearspan") )
    {
       arg = strtok(NULL," \n");
       pars[0] = SETBEARSPAN;
       pars[1] = atoi(arg);
       masSpinner(pars, &index, 2);
    }
    else if ( ! strcmp(cmd,"bearadj") )
    {
       arg = strtok(NULL," \n");
       pars[0] = SETBEARADJ;
       pars[1] = atoi(arg);
       masSpinner(pars, &index, 2);
    }
    else if ( ! strcmp(cmd,"asp") )
    {
       arg = strtok(NULL," \n");
       pars[0] = SETASP;
       pars[1] = atoi(arg);
       masSpinner(pars, &index, 1);
    }
    else if ( ! strcmp(cmd,"profile") )
    {
       arg = strtok(NULL," \n");
       pars[0] = SETPROFILE;
       pars[1] = atoi(arg);
       masSpinner(pars, &index, 1);
    }
    else
    {
       DPRINT1(0,"Unknown command: '%s'\n",cmd);
    }
       
    return (0);
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
    expStatusRelease();
    shutdownComm();
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
