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

/*   12-01-1994    pMsgesToAParser and pMsgesToXParser each modified to hold
		   50 messages of CONSOLE_MSGE_SIZE, since they receive stuff
		   directly from the monitor channel.				*/

#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <msgQLib.h>
#include <wdLib.h>
#include "commondefs.h"
#include "globals.h"
#include "hostAcqStructs.h"
#include "logMsgLib.h"
#include "hardware.h"
#include "qspiObj.h"
#include "spinObj.h"
#include "instrWvDefines.h"
#include "vtfuncs.h" 

/*
   System Initialization
   1. Make Message Qs required by the various Tasks
   2. Instanciate hardware Objects
   3. Open the needed Channels
   4. Start the needed tasks
*/

/* now in globals.h */
/* QSPI_ID pTheQspiObject - QSPI Object 14 chan shims/EEPROM */
/* SPIN_ID pTheSpinObject */
/* char      *pShareMemoryAddr;   /* address of beginning of share memory, just past SM backplane */
/* long      EndMemoryAddr;	/* the real end of memory, for heart beat value */
/* int	MSRType;	- MSR type MSRI or MSRII */

extern VT_ID   msr_VT;

static int tHBeatid;

int UsableMsrPorts = 3;

/* global debug flag */
extern int shimDebug;

shimDebugLevel()
{
	printf("Current shimDebug level = %d\n",shimDebug);
}

shimDebugSet(int level)
{
	printf("Old shimDebug level = %d, ",shimDebug);
	shimDebug = level;
	printf("New shimDebug level = %d\n",shimDebug);
}

postObj()
{
register QSPI_OBJ *pQspiObj;
register SPIN_OBJ *pSpinObj;

	pQspiObj = pTheQspiObject;
	pSpinObj = pTheSpinObject;

	printf("\n %s: vect=%lx, SEMid=%lx, spif=%d, count=%lx, config=%d\n",
	pQspiObj->pIdStr,pQspiObj->qspiItrVector,pQspiObj->pQspiSEMid,
	pQspiObj->qspispif,pQspiObj->qspicount,pQspiObj->qspiconfig);

	printf("\n %s: V_vectNum=%lx, MAS_vectNum=%lx\n",
	pSpinObj->pIdStr,pSpinObj->V_ItrVectNum,pSpinObj->MAS_ItrVectNum);

}

systemInit()
{
   int priority, stacksize, bigstacksize, medstacksize, taskoptions, 
       taskDelayTicks;
   int probetype = VERTICAL_PROBE;
   extern  char *usrGetSmEndAddr(void);

   DebugLevel = 0;
   shimDebug = 0;

   
   pShareMemoryAddr = usrGetSmEndAddr();
   printf("SmEndAddr: 0x%lx, SharedMemAddr: 0x%lx  :  ",pShareMemoryAddr,pShareMemoryAddr);
   if (pShareMemoryAddr < (char*) 0xbf7000)
   {
      EndMemoryAddr = 0x008FFFFF;
      MSRType = AUTO_BRD_TYPE_I;
   }
   else
   {
      EndMemoryAddr = 0x00bFFFFF;
      MSRType = AUTO_BRD_TYPE_II;
   }
   printf("MemoryEndAddr: 0x%lx\n",EndMemoryAddr);

 /* Add our System Delete Hook Routine */
 /*  addSystemDelHook(); */

 /* 1.   -------------  Message Qs */

 /* 2.   -------------  Channels */
  
 /* 3.   -------------  Hardware Objects */

	if( pTheQspiObject == NULL)
	{
		pTheQspiObject = qspiCreate();
		if( pTheQspiObject == NULL ) return(0x55);
	}
	else
		printf("\n qspiObj is already created! \n");

	if( pTheSpinObject == NULL)
	{
		if( (probetype != VERTICAL_PROBE) && (probetype != MAS_PROBE) )
			probetype = VERTICAL_PROBE;
		pTheSpinObject = spinCreate(probetype);
		if( pTheSpinObject == NULL ) return(0x55);
	}
	else
		printf("\n spinObj is already created! \n");

	postObj();

        mboxInit(pShareMemoryAddr);

        clearSharedDacValues();

 /* 4.   -------------  Start Tasks */
   /* Start communication link to Host */
   priority = 200;
   stacksize = 3000;
   medstacksize = 10000;
   bigstacksize = 20000;
   taskoptions = 0;

   startMBoxTasks();

   startChk(1);     /* Spin Regulation Task, 1 sec checking interval */

   setSpeed((long) 0L);  /* set initial speed to Zero */

   quartCreate(UsableMsrPorts,2);

   determineShimType();

   initAutoShim();	/* starts in autoShim.c */

   determineGpaType();

   startHeartBeat(EndMemoryAddr);   /* let main CPU know we are up and running */ 
   /* vt has to be after the quartCreate */
   if (msr_VT == NULL)
      vtCreate();

   printf("System Ready.....\n");
}


/* delete All Tasks and buffers and restart */
resetAll()
{
   int tid;

   if ((tid = taskNameToId("tIntrp")) != ERROR)
	taskDelete(tid);

   qspiDelete();
   pTheQspiObject = NULL;
	spinDelete();
   pTheSpinObject = NULL;

}

/* usage of a watch dog heart beat will avoid any time accumulation error as a 
   result of task preemption for a task based heart-beat
*/
#define WDOG_HEARTBEAT

#ifdef WDOG_HEARTBEAT
static WDOG_ID wdHeartBeat;
static u_long  pEndOfSharedMemory;
void HeartBeat()
{

    /* (*AUTO_HEARTBEAT(MPU332_RAM))++; */
    (*AUTO_HEARTBEAT_DYN(pEndOfSharedMemory))++;


    if ( wdStart(wdHeartBeat, (int) (sysClkRateGet() * 1), (FUNCPTR) HeartBeat, 0) == ERROR)
    {
	logMsg("HeartBeat: COuld not restart Watch-Dog Heart Beat\n");
    }

#ifdef INSTRUMENT
    wvEvent(EVENT_MSR_HEARTBEAT,NULL,NULL);
#endif

}
startHeartBeat(u_long endOfMemory)
{
   pEndOfSharedMemory = endOfMemory;
   printf("Heart Beat at: 0x%lx, new: 0x%lx\n",
	AUTO_HEARTBEAT(MPU332_RAM),AUTO_HEARTBEAT_DYN(endOfMemory));
   /* *AUTO_HEARTBEAT(MPU332_RAM) = 0L; */
   *AUTO_HEARTBEAT_DYN(endOfMemory) = 0L;

    if ((wdHeartBeat = wdCreate()) == NULL)
    {
        DPRINT(-1,"wdCreate Error\n");
	return(ERROR);
    }
    
    /* Set timer to go off every second */
    if (wdStart(wdHeartBeat, (int) (sysClkRateGet() * 1), (FUNCPTR) HeartBeat, 0) == ERROR)
    {
        DPRINT(-1,"wdStart Error\n");
       return(ERROR);
    }
}
prthb()
{
  /* DPRINT1(-1,"MSR332 HB: %lu\n",*AUTO_HEARTBEAT(MPU332_RAM)); */
  DPRINT2(-1,"MSR332 HB(0x%lx): %lu\n",
	AUTO_HEARTBEAT_DYN(pEndOfSharedMemory),*AUTO_HEARTBEAT_DYN(pEndOfSharedMemory));
}
#endif 

#ifdef INSTRUMENT
wvLog(int level)
{
     if ((level > 0) && (level < 4))
         wvEvtLogEnable(level);
     else
        printf("wvLog(level) level = 1,2, or 3\n");
}
wvStp(void)
{
     wvEvtLogDisable();
}
wvPri(int level)
{
  int tid;
  if ((tid = taskNameToId("tEvtTask")) != ERROR)
  {
     taskPrioritySet(tid,level);
  }  
}
#endif
