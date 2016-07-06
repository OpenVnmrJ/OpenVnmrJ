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

#include "errorcodes.h"
#include "instrWvDefines.h"
#include "taskPriority.h"

#include "logMsgLib.h"

#include "NDDS_Obj.h"
#include "NDDS_PubFuncs.h"
#include "NDDS_SubFuncs.h"

#include "Console_Stat.h"

#ifdef RTI_NDDS_4x
#include "Console_StatPlugin.h"
#include "Console_StatSupport.h"
#endif  /* RTI_NDDS_4x */

#include "masterAux.h"

extern int DebugLevel;
extern NDDS_ID NDDS_Domain;


extern int globalLocked;
extern int auxLockByte;

/* Console  status Pub */
NDDS_ID pConsoleStatusPub = NULL;

Console_Stat *pCurrentStatBlock = NULL;

static SEM_ID pPubSem = NULL;

#ifdef RTI_NDDS_4x
int publishStat(NDDS_ID pNDDS_Obj)
{
   DDS_ReturnCode_t result;
   DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
   Console_StatDataWriter *ConsoleStat_writer = NULL;

   ConsoleStat_writer = Console_StatDataWriter_narrow(pNDDS_Obj->pDWriter);
   if (ConsoleStat_writer == NULL) {
        errLogRet(LOGIT,debugInfo, "Console_Stat: DataReader narrow error.\n");
        return -1;
   }
   result = Console_StatDataWriter_write(ConsoleStat_writer,
                pNDDS_Obj->instance,&instance_handle);
   if (result != DDS_RETCODE_OK) {
            errLogRet(LOGIT,debugInfo, "publishStat: write error %d\n",result);
   }
   return 0;
}
#endif  /* RTI_NDDS_4x */

/*
 *   Task waits for the semaphore to be given then publishes the Status
 *   This is done to avoid doing the publishing from within a NDDS task.
 *
 *     Author:  Greg Brissey 9/29/04
 */
pubConsoleStatus()
{
   int autoUpdateRate = calcSysClkTicks(2000); /*  2 sec update rate no matter what */

   FOREVER
   {
       semTake(pPubSem, autoUpdateRate);  /* every 2 sec return and publish */
       if (pConsoleStatusPub != NULL)
       {
          if (DebugLevel >= 4)
             prtStatus();
#ifndef RTI_NDDS_4x
          nddsPublishData(pConsoleStatusPub);
#else  /* RTI_NDDS_4x */
          publishStat(pConsoleStatusPub);
#endif  /* RTI_NDDS_4x */
       }
   }
}

 
void initialStatComm()
{
   int startConsoleStatPub(int priority, int taskoptions, int stacksize);
   NDDS_ID createConsoleStatusPub(NDDS_ID nddsId, char *topic);
   pConsoleStatusPub = createConsoleStatusPub(NDDS_Domain,(char*) CNTLR_PUB_STAT_TOPIC_FORMAT_STR);
   pCurrentStatBlock = pConsoleStatusPub->instance;

   pCurrentStatBlock->dataTypeID = 3; /* STATBLOCK */
   /*
    * Set to ACQ_REBOOT on bootup. Procs use this to decide if shims, lock, should be updated.
    * Procs then set this to ACQ_INACTIVE
    * When ddr publishes CT, it is set to ACQ_IDLE
    */
   pCurrentStatBlock->Acqstate = ACQ_REBOOT;
   pCurrentStatBlock->AcqVTSet = 30000;  /* OFF */
   pCurrentStatBlock->AcqVTAct = 30000;  /* OFF */
   pCurrentStatBlock->AcqLSDVbits |= LSDV_EJECT;  /* Inserted */

#ifdef SILKWORM
   pCurrentStatBlock->consoleID = -1; /* ConsoleID undetermined */
#endif

   /* taskDelay(10); */
   startConsoleStatPub(STATMON_TASK_PRIORITY, STD_TASKOPTIONS, (STD_STACKSIZE + 1024));
}

startConsoleStatPub(int priority, int taskoptions, int stacksize)
{
   if (pPubSem == NULL)
   {
      pPubSem = semCCreate(SEM_Q_FIFO,SEM_EMPTY);
      if ( (pPubSem == NULL) )
      {
        errLogSysRet(LOGIT,debugInfo,
	   "startConsoleStatPub: Failed to allocate pPubSem Semaphore:");
        return(ERROR);
      }
   }
   
   if (taskNameToId("tStatPub") == ERROR)
      taskSpawn("tStatPub",priority,0,stacksize,pubConsoleStatus,pPubSem,
						2,3,4,5,6,7,8,9,10);
}

killConsolePub()
{
   int tid;
   if ((tid = taskNameToId("tStatPub")) != ERROR)
      taskDelete(tid);
}


/*
 * Create a Best Effort Publication Topic to communicate the Lock Status
 * Information
 *
 *					Author Greg Brissey 9-29-04
 */
NDDS_ID createConsoleStatusPub(NDDS_ID nddsId, char *topic)
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
    getConsole_StatInfo(pPubObj);
         
    DPRINT1(-1,"Create Pub topic: '%s' \n",pPubObj->topicName);
#ifndef RTI_NDDS_4x
    createBEPublication(pPubObj);
#else  /* RTI_NDDS_4x */
    initBEPublication(pPubObj);
    createPublication(pPubObj);
#endif  /* RTI_NDDS_4x */
    return(pPubObj);
}        

sendConsoleStatus()
{
    pCurrentStatBlock->AcqLSDVbits &= ~LSDV_LKMASK;
    pCurrentStatBlock->AcqOpsComplCnt ++;
    if ( ! (auxLockByte & AUX_LOCK_LOCKON) )  // zero = loop closed
    {
       if (globalLocked)
          pCurrentStatBlock->AcqLSDVbits |= LSDV_LKREGULATED; /* REG   =0x04 */
       else
          pCurrentStatBlock->AcqLSDVbits |= LSDV_LKNONREG;    /* NONREG=0x08*/
    }							      /* OFF   =0x00*/
    if ( detectSample() )
       pCurrentStatBlock->AcqLSDVbits |= LSDV_DETECTED;
    else
       pCurrentStatBlock->AcqLSDVbits &= ~LSDV_DETECTED;
    DPRINT2(+3,"Stat Ops= %d LSDV=0x%x\n", pCurrentStatBlock->AcqOpsComplCnt, pCurrentStatBlock->AcqLSDVbits);
    if (pPubSem != NULL)
      semGive(pPubSem);
}

setAcqState( int acqstate)
{
   pCurrentStatBlock->Acqstate = (short) acqstate;
   sendConsoleStatus();
}

getAcqState()
{
    return( (int) pCurrentStatBlock->Acqstate );
}

setFidCtState(int fidnum, int ct)
{
   pCurrentStatBlock->AcqFidCnt = fidnum;
   pCurrentStatBlock->AcqCtCnt = ct;
   sendConsoleStatus();
}

prtStatus()
{
   DPRINT1(-5,"Acqstat: %d\n", pCurrentStatBlock->Acqstate);
   DPRINT2(-5,"FID: %d, CT: %d\n", pCurrentStatBlock->AcqFidCnt, pCurrentStatBlock->AcqCtCnt);
   DPRINT2(-5,"VT Set: %d, Actual: %d\n", pCurrentStatBlock->AcqVTSet, pCurrentStatBlock->AcqVTAct);
   DPRINT1(-5,"Stat LSDV=%d\n",pCurrentStatBlock->AcqLSDVbits);
}

testStatChange(int type, int value){
    switch(type){
    case 0:
        pCurrentStatBlock->AcqVTAct=value;
        break;
    case 1:
        pCurrentStatBlock->AcqVTSet=value;
        break;
    case 2:
        pCurrentStatBlock->AcqSpinAct=value;
        break;
    case 3:
        pCurrentStatBlock->AcqSpinSet=value;
        break;
    }
    sendConsoleStatus();
}
#ifdef FOR_DEBBIE_QUESTION
tstsize()
{
   unsigned long long my64bit;
   short my16bit;

   printf("sizeof long long: %d\n",sizeof(my64bit));
   printf("sizeof short: %d\n",sizeof(my16bit));
   my64bit = 0x1234567887654321LL;
   my16bit = 0x1234;
   printf("64: %llu, %llx, 16: %hd, %d, %hx,%x\n", my64bit,my64bit,my16bit,my16bit,my16bit,my16bit);
}
#endif

