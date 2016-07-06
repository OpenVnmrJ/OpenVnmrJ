/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 *
 *  Author:  Greg Brissey   11/05/2007
 */
#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <sysLib.h>
#include <semLib.h>
#include <rngLib.h>
#include <msgQLib.h>

/* only for NDDS 4x version */
#ifdef RTI_NDDS_4x

#include "errorcodes.h"
#include "instrWvDefines.h"
#include "taskPriority.h"

#include "logMsgLib.h"

#include "NDDS_Obj.h"
#include "NDDS_PubFuncs.h"
#include "NDDS_SubFuncs.h"

#include "Console_ConfPlugin.h"
#include "Console_ConfSupport.h"

extern int DebugLevel;
extern NDDS_ID NDDS_Domain;
extern char *nddsverstr(char *str);


/* Console  status Pub */
NDDS_ID pConsoleConfPub = NULL;

static SEM_ID pPubSem = NULL;

int publishConf(NDDS_ID pNDDS_Obj)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Console_ConfDataWriter *ConsoleConf_writer = NULL;

   ConsoleConf_writer = Console_ConfDataWriter_narrow(pNDDS_Obj->pDWriter);
   if (ConsoleConf_writer == NULL) {
        errLogRet(LOGIT,debugInfo, "Console_Conf: DataReader narrow error.\n");
        return -1;
   }
   result = Console_ConfDataWriter_write(ConsoleConf_writer,
                pNDDS_Obj->instance,&instance_handle);
   if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "publishStat: write error %d\n",result);
   }
   return 0;
}

/*
 *   Task waits for the semaphore to be given then publishes the Status
 *   This is done to avoid doing the publishing from within a NDDS task.
 *
 *     Author:  Greg Brissey 11/05/07
 */
pubConsoleConf()
{
   int countdown = 60;
   int autoUpdateRate = calcSysClkTicks(2000); /*  2 sec update rate no matter what */

   FOREVER
   {
       semTake(pPubSem, autoUpdateRate);  /* every 2 sec return and publish */
       if (pConsoleConfPub != NULL)
       {
          publishConf(pConsoleConfPub);
       }
       DPRINT1(1,"pubConsoleConf: countdown: %d\n",countdown);
       if (--countdown <= 0)
          break;
   }
}

 
void initialConfComm()
{
   extern char *runtimeVersion;
   extern int ConsoleTypeFlag, SystemRevId, InterpRevId;
   extern char fpgaLoadStr[40];

   char nddsProdVer[120];
   Console_Conf *pConsoleConfIssue;
   int startConsoleConfPub(int priority, int taskoptions, int stacksize);
   NDDS_ID createConsoleConfPub(NDDS_ID nddsId, char *topic);

   pConsoleConfPub = createConsoleConfPub(NDDS_Domain,(char*) CNTLR_PUB_CONF_TOPIC_FORMAT_STR);
   pConsoleConfIssue = pConsoleConfPub->instance;

   //  structure version
   pConsoleConfIssue->structVersion = CONSOLE_CONF_STRUCT_VERSION;

   /* maximum length = ((CONSOLE_CONF_MAX_STR_LEN)) */

   //  VxWorks version 
   strcpy(pConsoleConfIssue->VxWorksVersion,runtimeVersion);

   // Console Type, VNMRS, 400MR
   pConsoleConfIssue->ConsoleTypeFlag = ConsoleTypeFlag; 

   // System Rev
   pConsoleConfIssue->SystemRevId = SystemRevId;

   // fpga load string
   strncpy(pConsoleConfIssue->fpgaLoadStr,fpgaLoadStr,CONSOLE_CONF_MAX_STR_LEN);

   // nddsVersion = NDDS_Config_Version_get_api_version();
   // printf("major: %ld, minor: %ld, release: '%c', Build: %ld\n",
   //    nddsVersion->major, nddsVersion->minor, nddsVersion->release, nddsVersion->build);

   //  NDDS version 
   strcpy(pConsoleConfIssue->RtiNddsVersion,nddsverstr(nddsProdVer));
   // strcpy(pConsoleConfIssue->RtiNddsVersion,"4.1e rev 00");

   //  PSG Interpeter version 
   sprintf(pConsoleConfIssue->PsgInterpVersion,"%d",InterpRevId); 

   //  Compilation Timestamp 
   sprintf(pConsoleConfIssue->CompileDate,"%s %s",__DATE__,__TIME__); 

   //  file md5 signitures 
   if (ffmd5("ddrexec.o",pConsoleConfIssue->ddrmd5) != 0) 
       strcpy(pConsoleConfIssue->ddrmd5,"Not Present");
   if (ffmd5("gradientexec.o",pConsoleConfIssue->gradientmd5) !=0 )
       strcpy(pConsoleConfIssue->gradientmd5,"Not Present");
   if (ffmd5("lockexec.o",pConsoleConfIssue->lockmd5) !=0 )
       strcpy(pConsoleConfIssue->lockmd5,"Not Present");
   if (ffmd5("masterexec.o",pConsoleConfIssue->mastermd5) !=0 )
       strcpy(pConsoleConfIssue->mastermd5,"Not Present");
   if (ffmd5("nvlib.o",pConsoleConfIssue->nvlibmd5) !=0 )
       strcpy(pConsoleConfIssue->nvlibmd5,"Not Present");
   if (ffmd5("nvScript",pConsoleConfIssue->nvScriptmd5) !=0 )
       strcpy(pConsoleConfIssue->nvScriptmd5,"Not Present");
   if (ffmd5("pfgexec.o",pConsoleConfIssue->pfgmd5) !=0 )
       strcpy(pConsoleConfIssue->pfgmd5,"Not Present");
   if (ffmd5("lpfgexec.o",pConsoleConfIssue->lpfgmd5) !=0 )
       strcpy(pConsoleConfIssue->lpfgmd5,"Not Present");
   if (ffmd5("rfexec.o",pConsoleConfIssue->rfmd5) !=0 )
       strcpy(pConsoleConfIssue->rfmd5,"Not Present");
   if (ffmd5("vxWorks405gpr.bdx",pConsoleConfIssue->vxWorksKernelmd5) !=0 )
       strcpy(pConsoleConfIssue->vxWorksKernelmd5,"Not Present");

   startConsoleConfPub(STATMON_TASK_PRIORITY, STD_TASKOPTIONS, STD_STACKSIZE);
}
#ifdef XXXX
// gives 3.1c rev 10 as an answer even on 4.1e, go figure....
prtnv()
{
   int vernum,major,minor,build;
   char verletter;
   char str[128];

   vernum = NddsVersionGet();
   major = (vernum >> 24) & 0xff;
   minor = (vernum >> 16) & 0xff;
   verletter = ((vernum >> 8) & 0xff) + 0x61;   /* encode letter starting at zero, 61+0 = 'a' */
   build = vernum & 0xff;

   printf("major: %d minor: %d char: '%c' rev: %d\n",major,minor,verletter,build);
   printf("ndds%d.%d%c_rev%d",major,minor,verletter,build);
   // sprintf(str,"ndds%d.%d%c_rev%d",major,minor,verletter,build);
}
#endif

prtConfInfo()
{
   Console_Conf *recvIssue;
   recvIssue = pConsoleConfPub->instance;
     printf("VxWorks Version: '%s'\n",recvIssue->VxWorksVersion);
     printf("RTI NDDS Version: '%s'\n",recvIssue->RtiNddsVersion);
     printf("PSG/Interpreter Version: '%s'\n",recvIssue->PsgInterpVersion);
     printf("Console Type: %d\n", recvIssue->ConsoleTypeFlag);
     printf("System Rev Id: %d\n", recvIssue->SystemRevId);
     printf("FPGA Loaded With: '%s'\n", recvIssue->fpgaLoadStr);
     printf("Compile Data Time: '%s'\n",recvIssue->CompileDate);
     printf("MD5 Signitures for:\n");
     printf("         ddr: '%s'\n",recvIssue->ddrmd5);
     printf("    gradient: '%s'\n",recvIssue->gradientmd5);
     printf("        lock: '%s'\n",recvIssue->lockmd5);
     printf("      master: '%s'\n",recvIssue->mastermd5);
     printf("       nvlib: '%s'\n",recvIssue->nvlibmd5);
     printf("    nvScript: '%s'\n",recvIssue->nvScriptmd5);
     printf("         pfg: '%s'\n",recvIssue->pfgmd5);
     printf("        lpfg: '%s'\n",recvIssue->lpfgmd5);
     printf("          rf: '%s'\n",recvIssue->rfmd5);
     printf("     vxWorks: '%s'\n",recvIssue->vxWorksKernelmd5);
}

startConsoleConfPub(int priority, int taskoptions, int stacksize)
{
   if (pPubSem == NULL)
   {
      pPubSem = semCCreate(SEM_Q_FIFO,SEM_EMPTY);
      if ( (pPubSem == NULL) )
      {
        errLogSysRet(LOGIT,debugInfo,
	   "startConsoleConfPub: Failed to allocate pPubSem Semaphore:");
        return(ERROR);
      }
   }
   
   if (taskNameToId("tConfPub") == ERROR)
      taskSpawn("tConfPub",priority,0,stacksize,pubConsoleConf,pPubSem,
						2,3,4,5,6,7,8,9,10);
}

killConsoleConfPub()
{
   int tid;
   if ((tid = taskNameToId("tConfPub")) != ERROR)
      taskDelete(tid);
}


/*
 * Create a Best Effort Publication Topic to communicate the Lock Status
 * Information
 *
 *					Author Greg Brissey 9-29-04
 */
NDDS_ID createConsoleConfPub(NDDS_ID nddsId, char *topic)
{
     int result;
     NDDS_ID pPubObj;
     char pubtopic[128];

    /* Build Data type Object for both publication and subscription to Expproc */
    /* ------- malloc space for data type object --------- */
    if ( (pPubObj = (NDDS_ID) malloc( sizeof(NDDS_OBJ)) ) == NULL )
      {  
        return(NULL);
      }  

    /* zero out structure */
    memset(pPubObj,0,sizeof(NDDS_OBJ));
    memcpy(pPubObj,nddsId,sizeof(NDDS_OBJ));

    strcpy(pPubObj->topicName,topic);
    pPubObj->pubThreadId = STATMON_TASK_PRIORITY; /* DEFAULT_PUB_THREADID; taskIdSelf(); */
         
    /* fills in dataTypeName, TypeRegisterFunc, TypeAllocFunc, TypeSizeFunc */
    getConsole_ConfInfo(pPubObj);
         
    DPRINT1(-1,"Create Pub topic: '%s' \n",pPubObj->topicName);
    initBEPublication(pPubObj);
    createPublication(pPubObj);
    return(pPubObj);
}        

/*
sendConsoleStatus()
{
    if (pPubSem != NULL)
      semGive(pPubSem);
}
*/

#endif  /*  not RTI_NDDS_4x */
