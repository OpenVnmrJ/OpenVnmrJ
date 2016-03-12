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
#include <stdlib.h>
#include "commondefs.h"
#include "logMsgLib.h"
#include "sysflags.h"
#include "instrWvDefines.h"

static char *IDStr ="Fifo Object";
static int  IdCnt = 0;

static SYSFLAGS_ID pTheSysFlagsObj;


/**************************************************************
*
*  sysFlagsCreate - create the sysFlags Object Data Structure & Semaphore
*
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semaphore creation failed
*
*/ 

SYSFLAGS_ID  sysFlagsCreate(char* idstr)
/* char* idstr - user indentifier string */
{
  SYSFLAGS_ID pSysFlagsObj;
   char tmpstr[80];

  /* ------- malloc space for FIFO Object --------- */
  if ( (pSysFlagsObj = (SYSFLAGS_OBJ *) malloc( sizeof(SYSFLAGS_OBJ)) ) == NULL )
  {
    errLogSysRet(LOGIT,debugInfo,"fifoCreate: Could not Allocate Space:");
    return(NULL);
  }

  /* zero out structure so we don't free something by mistake */
  memset(pSysFlagsObj,0,sizeof(SYSFLAGS_OBJ));


  IdCnt++;
  /* ------ Create Id String ---------- */
  if (idstr == NULL) 
  {
     sprintf(tmpstr,"%s %d\n",IDStr,IdCnt);
     pSysFlagsObj->pIdStr = (char *) malloc(strlen(tmpstr)+2);
  }
  else
  {
     pSysFlagsObj->pIdStr = (char *) malloc(strlen(idstr)+2);
  }
  pSysFlagsObj->pSID = SCCSid;	/* SCCS ID */

  if (pSysFlagsObj->pIdStr == NULL)
  {
     sysFlagsDelete(pSysFlagsObj);
     errLogSysRet(LOGIT,debugInfo,
	"sysFlagsCreate: IdStr - Could not Allocate Space:");
     return(NULL);
  }

  if (idstr == NULL) 
  {
     strcpy(pSysFlagsObj->pIdStr,tmpstr);
  }
  else
  {
     strcpy(pSysFlagsObj->pIdStr,idstr);
  }

  pSysFlagsObj->pSemFlagsChg = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  pSysFlagsObj->pSemMutex =  semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                        SEM_DELETE_SAFE);

  if ( (pSysFlagsObj->pSemMutex == NULL) || 
          (pSysFlagsObj->pSemFlagsChg == NULL) 
     )
     {
        sysFlagsDelete(pSysFlagsObj);
        errLogSysRet(LOGIT,debugInfo,
	   "sysFlagsCreate: Failed to allocate some resource:");
        return(NULL);
     }

  pSysFlagsObj->stateFlags = 0;

 return(pSysFlagsObj);
}

/**************************************************************
*
*  sysFlagsDelete - Deletes sysFlags Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 10/1/93
*/
int sysFlagsDelete(SYSFLAGS_ID pSysFlags)
/* SYSFLAGS_ID 	pSysFlags - sysFlags Object identifier */
{

   if (pSysFlags != NULL)
   {
      if (pSysFlags->pIdStr != NULL)
	 free(pSysFlags->pIdStr);
      if (pSysFlags->pSemFlagsChg != NULL)
         semDelete(pSysFlags->pSemFlagsChg);
      if (pSysFlags->pSemMutex != NULL)
         semDelete(pSysFlags->pSemMutex);

      free(pSysFlags);
   }
}

/*----------------------------------------------------------------------*/
/* sysFlagsShwResrc							*/
/*     Show system resources used by Object (e.g. semaphores,etc.)	*/
/*	Useful to print then related back to WindView Events		*/
/*----------------------------------------------------------------------*/
void sysFlagsShwResrc(SYSFLAGS_ID pSysFlags, int indent )
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';

   printf("\n%s SysFlags Obj: '%s', 0x%lx\n",spaces,pSysFlags->pIdStr,pSysFlags);
   printf("%s   Binary Sems: pSemFlagsChg - 0x%lx\n",spaces,pSysFlags->pSemFlagsChg);
   printf("%s   Mutex:       pSemMutex  --- 0x%lx\n",spaces,pSysFlags->pSemMutex);
}


/*************************************************************
*  sysFlagsMaskBit - set a bit, then give flag semaphore
*
*			Author Greg Brissey
*
*/
int  sysFlagsMaskBit(SYSFLAGS_ID pTheSysFlags,int task)
{
   if (pTheSysFlags == NULL)
     return(-1);

   semTake(pTheSysFlags->pSemMutex,WAIT_FOREVER);
      pTheSysFlags->stateFlags |= task;
      semGive(pTheSysFlags->pSemFlagsChg);
      semFlush(pTheSysFlags->pSemFlagsChg);
   semGive(pTheSysFlags->pSemMutex);

   return(OK);
}

/*************************************************************
*  sysFlagsClearBit - clear a bit
*
*			Author Greg Brissey
*
*/
int   sysFlagsClearBit(SYSFLAGS_ID pTheSysFlags,int task)
{
   if (pTheSysFlags == NULL)
     return(-1);

   pTheSysFlags->stateFlags &= ~(task);

   return(OK);
}

/*************************************************************
*  sysFlagsClearAll - clear all bits
*
*			Author Greg Brissey
*
*/
int   sysFlagsClearAll(SYSFLAGS_ID pTheSysFlags)
{
   if (pTheSysFlags == NULL)
     return(-1);

   pTheSysFlags->stateFlags = 0;

   return(OK);
}

int sysFlagsCmpMask(SYSFLAGS_ID pSysFlags, int mask, int timeout)
/* FLAGS_ID 	pSysFlags - Flags Object identifier */
/* int mask 	mask - bits to test for */
/* int timeout;	 timeout of call, see above */
/* int secounds;    number of secounds to wait before timing out */
{
   int state;
   if (pSysFlags == NULL)
     return(-1);

    switch(timeout)
    {
     case NO_WAIT:
          state =  ((pSysFlags->stateFlags & mask) == mask);
	  break;

     case WAIT_FOREVER: /* block if state has not changed */
          while( ((pSysFlags->stateFlags & mask) != mask) )
          {
#ifdef INSTRUMENT
            wvEvent(EVENT_SYSFLAG_SEMTAKE,NULL,NULL);
#endif
	    semTake(pSysFlags->pSemFlagsChg, WAIT_FOREVER);  
          }
	  state = 1;
	  break;

     default:     /* block if state has not changed, until timeout */
          state = 0;
          while(((pSysFlags->stateFlags & mask) != mask))
          {
#ifdef INSTRUMENT
            wvEvent(EVENT_SYSFLAG_SEMTAKE,NULL,NULL);
#endif
            /* if ( semTake(pSysFlags->pSemFlagsChg, (sysClkRateGet() * timeout) ) != OK ) */
            if ( semTake(pSysFlags->pSemFlagsChg,  timeout ) != OK )
	    {
	         state = ERROR;		/* timed out */
	         break;
	    }
          }
          if (state != ERROR)
	     state = 1;
          break;
    }
   return(state);
}

InitSystemFlags()
{
   pTheSysFlagsObj = sysFlagsCreate("Console System Flags");
}

int wait4SystemReady()
{
#ifdef INSTRUMENT
            wvEvent(EVENT_SYSFLAG_WAIT4SYSTEM,NULL,NULL);
#endif
   return(
    sysFlagsCmpMask(pTheSysFlagsObj,(DOWNLINKER_FLAGBIT | APARSER_FLAGBIT),WAIT_FOREVER)
         );
}

int wait4DownLinkerReady()
{
#ifdef INSTRUMENT
     wvEvent(EVENT_SYSFLAG_WAIT4DWNLKR,NULL,NULL);
#endif
   return(
    sysFlagsCmpMask(pTheSysFlagsObj,DOWNLINKER_FLAGBIT,WAIT_FOREVER)
         );
}

int wait4ParserReady(int ticks)
{
#ifdef INSTRUMENT
     wvEvent(EVENT_SYSFLAG_WAIT4PARSER,NULL,NULL);
#endif
   return(
    sysFlagsCmpMask(pTheSysFlagsObj,APARSER_FLAGBIT,ticks)
         );
}

int phandlerBusy()
{
   return(
      ! sysFlagsCmpMask(pTheSysFlagsObj,PHANDLER_FLAGBIT,NO_WAIT)
         );
}

int downLinkerBusy()
{
   return(
      ! sysFlagsCmpMask(pTheSysFlagsObj,DOWNLINKER_FLAGBIT,NO_WAIT)
         );
}

int parserBusy()
{
   return(
      ! sysFlagsCmpMask(pTheSysFlagsObj,APARSER_FLAGBIT,NO_WAIT)
         );
}

markReady(int task)
{
   return(sysFlagsMaskBit(pTheSysFlagsObj,task));
}

markBusy(int task)
{
   return(sysFlagsClearBit(pTheSysFlagsObj,task));
}

sysShow()
{
   int state;

   state = pTheSysFlagsObj->stateFlags;
   printf("DownLinker: %s\n", ( (state & DOWNLINKER_FLAGBIT) ? "READY" : "BUSY" ));
   printf("AParser: %s\n", ( (state & APARSER_FLAGBIT) ? "READY" : "BUSY" ));
   printf("PHandlr: %s\n", ( (state & PHANDLER_FLAGBIT) ? "READY" : "BUSY" ));
}

sysflagsresrc()
{
    sysFlagsShwResrc(pTheSysFlagsObj, 1 );
}
