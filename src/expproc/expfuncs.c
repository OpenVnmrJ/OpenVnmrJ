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
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

#include "errLogLib.h"
#include "msgQLib.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "shrexpinfo.h"
#include "hostMsgChannels.h"
#include "expentrystructs.h"


extern void killEmAll(void);
extern void shutdownComm(void);
extern void expQRelease(void);
extern void resetState(void);
extern void showAuthRecord(void);
extern void expStatusRelease(void);
extern int expQshow(void);

extern int chanId;	/* Channel Id */

SHR_EXP_INFO expInfo = NULL;   /* start address of shared Exp. Info Structure */

extern ExpEntryInfo ActiveExpInfo;

static SHR_MEM_ID  ShrExpInfo = NULL;

int strtExp(char *argstr)
{
    char *filename;
    filename = strtok(NULL," ");
    DPRINT1(1,"strtExp: starting Exp: '%s'\n",filename);
    return(0);
}

int abortCodes(char *str)
{
   DPRINT(1,"abortCodes: \n");
   return(0);
}

int echoCmd(char *str)
{
   int stat;
   MSG_Q_ID pMsgId;
   char *procname;
   int len;

  procname = strtok(NULL," ");
  pMsgId = openMsgQ(procname);
  if (pMsgId == NULL)
    return(-1);
  
  len = strlen(str);
  if ((stat = sendMsgQ(pMsgId,str,len,MSGQ_NORMAL,WAIT_FOREVER)) != 0)
  {
     errLogRet(ErrLogOp,debugInfo, "echoCmd: Message to %s not sent\n",procname);
  }
  closeMsgQ(pMsgId);
  return(0);
}

void terminate(char *str)
{
   DPRINT(1,"terminate: \n");
   killEmAll();		/* terminate the rest of the Proc Family */
   shutdownComm();
   expQRelease();
   activeExpQRelease();
   expStatusRelease();
   killEmAll();
   exit(EXIT_SUCCESS);
}

/* this is invoked by the exception handler (excepthandler.c), 
   when process is sent a terminate signal */
void ShutDownProc()
{
   resetState();
   killEmAll();		/* terminate the rest of the Proc Family */
   shutdownComm();
   expQRelease();
   activeExpQRelease();
   expStatusRelease();
}

int debugLevel(char *str)
{
    extern int DebugLevel;
    char *value;
    int  val;
    value = strtok(NULL," ");
    val = atoi(value);
    DPRINT1(1,"debugLevel: New DebugLevel: %d\n",val);
    DebugLevel = val;
    printf("Expproc states: \n");
    showAuthRecord();
    expQshow();
    activeExpQshow();
    if (strlen(ActiveExpInfo.ExpId) > (size_t) 1)
    {
      DPRINT(0,"Running Experiment\n");
      DPRINT2(0,"Id: '%s', Priority: %d\n",
		ActiveExpInfo.ExpId,ActiveExpInfo.ExpPriority);
      shrmShow(ActiveExpInfo.ShrExpInfo);
    }
    return(0);
}

int mapInExp(ExpEntryInfo *expid)
{
    DPRINT1(2,"mapInExp: map Shared Memory Segment: '%s'\n",expid->ExpId);

    expid->ShrExpStatus = NULL;
    expid->ExpStatus = NULL;
    expid->ShrExpInfo = shrmCreate(expid->ExpId,SHR_EXP_INFO_RW_KEY,(unsigned long)sizeof(SHR_EXP_STRUCT)); 
    if (expid->ShrExpInfo == NULL)
    {
       errLogSysRet(LOGOPT,debugInfo,"mapInExp: shrmCreate() failed:");
       return(-1);
    }
    if (expid->ShrExpInfo->shrmem->byteLen < sizeof(SHR_EXP_STRUCT))
    {
       errLogRet(LOGOPT,debugInfo,
        "mapInExp: File: '%s'  is not the size of the shared Exp Info Struct: %zu(correct size) bytes\n",
     	    expid->ExpId, sizeof(SHR_EXP_STRUCT));
        /* hey, this file is not a shared Exp Info file */
       shrmRelease(expid->ShrExpInfo);         /* release shared Memory */
       unlink(expid->ExpId);        /* remove filename that shared Mem created */
       expid->ShrExpInfo = NULL;
       return(-1);
    }


#ifdef DEBUG
    if (DebugLevel >= 3)
       shrmShow(expid->ShrExpInfo);
#endif

    expid->ExpInfo = (SHR_EXP_INFO) shrmAddr(expid->ShrExpInfo);

    /* Should open the shared Exp status shared structure here */

    return(0);
}

int mapOutExp(ExpEntryInfo *expid)
{
    DPRINT1(2,"mapOutExp: unmap Shared Memory Segment: '%s'\n",expid->ExpId);

    if (expid->ShrExpInfo != NULL)
      shrmRelease(expid->ShrExpInfo); 
    
    if (expid->ShrExpStatus != NULL)
      shrmRelease(expid->ShrExpStatus); 

    memset((char*) expid,0,sizeof(ExpEntryInfo));  /* clear struct */

    return(0);
}

int mapIn(char *str)
{
    char* filename;

    filename = strtok(NULL," ");

    DPRINT1(1,"mapIn: map Shared Memory Segment: '%s'\n",filename);

    ShrExpInfo = shrmCreate(filename,1,(unsigned long)sizeof(SHR_EXP_STRUCT)); 
    if (ShrExpInfo == NULL)
    {
       errLogSysRet(LOGOPT,debugInfo,"mapIn: shrmCreate() failed:");
       return(-1);
    }

#ifdef DEBUG
    if (DebugLevel >= 3)
      shrmShow(ShrExpInfo);
#endif

    expInfo = (SHR_EXP_INFO) shrmAddr(ShrExpInfo);

    return(0);
}

int mapOut(char *str)
{
    char* filename;

    filename = strtok(NULL," ");

    DPRINT1(1,"mapOut: unmap Shared Memory Segment: '%s'\n",filename);

    shrmRelease(ShrExpInfo);
    ShrExpInfo = NULL;
    expInfo = NULL;
    return(0);
}

void expQTask()
{
}
