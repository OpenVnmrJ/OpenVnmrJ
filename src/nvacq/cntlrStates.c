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
#include <vxWorks.h>
#include <stdlib.h>
#include "logMsgLib.h"
#include "cntlrStates.h"
#include "instrWvDefines.h"
#include "ACode32.h"

static char *typetable[8] = { "master", "rf", "pfg", "grad", "lock", "ddr", "lock+pfg", "NA" };
static CNTRLSTATE_ID pTheCntlrStateObj;
#define CNTLR_TYPE_MAX 7

static char *cntlrStateStr[12] = { "Not Ready", "Ready & Idle", "Ready For Sync",
		"Exception Complete", "Exp. Complete", "Setup Complete", 
                "Flash Update Complete", "Flash Update Failed",
		"Flash Commit Complete", "Flash Commit Failed",
                "Tune Action Complete" ,"Exception Initiated" };

static char *cntlrInUse[3] = { "Unknown", "In Use", "UnUsed" };

static char *cntrlConfigStr[2] = { "Standard", "iCAT" };

/**************************************************************
*
*  cntrlStatesCreate - create the Controller's States Object Data Structure & Semaphore
*
*
* RETURNS:
* OK - if no error, NULL - if mallocing or semaphore creation failed
*
*/ 

CNTRLSTATE_ID  cntrlStatesCreate()
{
  CNTRLSTATE_ID pCntlrStatesId;
   char tmpstr[80];

  /* ------- malloc space for FIFO Object --------- */
  if ( (pCntlrStatesId = (CNTRLSTATE_ID) malloc( sizeof(CNTRLSTATE_OBJ)) ) == NULL )
  {
    errLogSysRet(LOGIT,debugInfo,"fifoCreate: Could not Allocate Space:");
    return(NULL);
  }

  /* zero out structure so we don't free something by mistake */
  memset(pCntlrStatesId,0,sizeof(CNTRLSTATE_OBJ));

  pCntlrStatesId->pSemStateChg = semBCreate(SEM_Q_FIFO,SEM_EMPTY);
  /* pCntlrStatesId->pSemStateChg = semCCreate(SEM_Q_FIFO,0); */
  pCntlrStatesId->pSemMutex =  semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                        SEM_DELETE_SAFE);

  if ( (pCntlrStatesId->pSemMutex == NULL) || 
          (pCntlrStatesId->pSemStateChg == NULL) 
     )
     {
        cntrlStatesDelete(pCntlrStatesId);
        errLogSysRet(LOGIT,debugInfo,
	   "sysFlagsCreate: Failed to allocate some resource:");
        return(NULL);
     }


  pTheCntlrStateObj = pCntlrStatesId;

 return(pCntlrStatesId);
}

/**************************************************************
*
*  cntrlStatesDelete - Deletes Controller States Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 8/9/2004
*/
int cntrlStatesDelete(CNTRLSTATE_ID pSysFlags)
/* CNTRLSTATE_ID pSysFlags - controller states  Object identifier */
{

   if (pSysFlags != NULL)
   {
      if (pSysFlags->pSemStateChg != NULL)
         semDelete(pSysFlags->pSemStateChg);
      if (pSysFlags->pSemMutex != NULL)
         semDelete(pSysFlags->pSemMutex);

      free(pSysFlags);
   }
}

static int getType(char *id, int *type, int *num)
{
    char name[16], numstr[16];
    int i,j,k,len;
    len = strlen(id);
    j = k = 0;
    for (i=0; i< len; i++)
    {
       if (!isdigit(id[i]))
       {
         name[j++] = id[i]; 
       }
       else
       {
	 numstr[k++] = id[i]; 
       }
    }
    name[j] = numstr[k] = 0;
    *num = atoi(numstr);
    for (i=0; i < CNTLR_TYPE_MAX; i++)
    {
      if (strcmp(name,typetable[i]) == 0)
         break;
    }
    *type = i;
    
    return(0);
}

/*
 * chkStateList differs from the standard one above by
 * 1. instead of returning 0 - if not already it returns the number
 *    of controllers not in this state. Along with a list of them
 * 2. instead of return 1 if all the controllers are the requested state 
 *    returns 0 
 *
 *	Author Greg Brissey  12/3/2004   
 *
 * added if the state waiting for is not Exception complete, and a controller
 * report exception initiated, then return -1, with the controller generated the exception 
 * added errocode to struct, so now a list of controllers return an errorcode is also generated.
 */
static int chkStateList(int state, char *notReadyList)
{
   int type,num,j;
   int numNotReady;
   int numErrors;

   DPRINT1(1,"chkStateList: state: %d\n",state);
   numNotReady = numErrors = 0;
   notReadyList[0] = 0;
   /* errorList[0] = 0; */

   // don't allow cntlrSetSate to alter states while checking them..
   DPRINT(2,"chkStateList: Take Mutex\n");
   semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);
   DPRINT(2,"chkStateList: Mutex taken\n");

   for(type=0; type < CNTLR_TYPE_MAX; type++)
   {
      num = pTheCntlrStateObj->totalOfType[type];
      for(j=1; j <= num; j++)
      {
         DPRINT4(2,"chkStateList: '%s': State: %d, CmpState: %d, usedInPS: %d\n",pTheCntlrStateObj->CntlrInfo[type][j].cntlrID,
		 pTheCntlrStateObj->CntlrInfo[type][j].state, state, pTheCntlrStateObj->CntlrInfo[type][j].usedInPS);

         if ( (pTheCntlrStateObj->CntlrInfo[type][j].usedInPS == CNTLR_INUSE_STATE) )
         {
            if ( (state != CNTLR_EXCPT_CMPLT) && 
                 ( (pTheCntlrStateObj->CntlrInfo[type][j].state == CNTLR_EXCPT_INITIATED) ||
                   (pTheCntlrStateObj->CntlrInfo[type][j].state == CNTLR_EXCPT_CMPLT) ) )
            {
                notReadyList[0] = 0;
                strcat(notReadyList,pTheCntlrStateObj->CntlrInfo[type][j].cntlrID);
                DPRINT(2,"chkStateList: Give Mutex\n");
                semGive(pTheCntlrStateObj->pSemMutex);
                return(-1);   /* exception has occurred while waiting for another state, return immediately */
            }
            else if (pTheCntlrStateObj->CntlrInfo[type][j].state != state) 
            {
               if (numNotReady != 0)
                  strcat(notReadyList," ");
               strcat(notReadyList,pTheCntlrStateObj->CntlrInfo[type][j].cntlrID);
	       numNotReady++;
            }
         }
      }
   }
   
   DPRINT(2,"chkStateList: Give Mutex\n");
   semGive(pTheCntlrStateObj->pSemMutex);
   return(numNotReady);
}

char *getCntlrStateStr(int state)
{
    char *strptr;
    if ((state >= 0) && (state < MAX_CNTLR_STATES))
    {
        strptr = cntlrStateStr[state];
    }
    else
        strptr = "Unknown";
    return strptr;
}
char *getCntlrUseStateStr(int state)
{
    char *strptr;
    if ((state >= 0) && (state < 3))
    {
        strptr = cntlrInUse[state];
    }
    else
        strptr = "Unknown";
    return strptr;
}

char *getCntlrConfigStr(int config)
{
    char *strptr;
    if ((config >= 0) && (config < 2))
    {
        strptr = cntrlConfigStr[config];
    }
    else
        strptr = "Unknown";

    return strptr;
}

#ifdef NOTYET
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

#endif

/*************************************************************
*  cntlrSetState - set a bit, then give flag semaphore
*
*			Author Greg Brissey
*
*/
int  cntlrSetState(char *cntrlId,int state,int errorcode)
{
   int error,type,num;

   if (pTheCntlrStateObj == NULL)
     return(-1);

   error = getType(cntrlId, &type,&num);
   if (error != 0)
     return(-1);

   DPRINT(2,"cntlrSetState: Take Mutex\n");
   semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);
   DPRINT(2,"cntlrSetState: Mutex taken\n");

      DPRINT4(1,"cntlrSetState: changed '%s' state to: '%s', [type=%d, num=%d]\n",cntrlId,getCntlrStateStr(state),type,num);
      /* CNTLR_READY4SYNC 2 CNTLR_EXCPT_CMPLT 3 CNTLR_EXP_CMPLT 4 CNTLR_SU_CMPLT 5 CNTLR_FLASH_UPDATE_CMPLT 6 */

      /* to avoid the problem where an controller send exception complete, then exp complete, we prevent
         a state changed from CNTLR_EXCPT_CMPLT, unless it to set it back to IDLE state, 3/14/05  GMB */
      if ( (pTheCntlrStateObj->CntlrInfo[type][num].state == CNTLR_EXCPT_CMPLT) &&
	   ( state >= CNTLR_READY4SYNC ) ) 
      {
          if (state != CNTLR_EXCPT_CMPLT)
          {
            DPRINT1(-1,"cntlrSetState: '%s': present state is CNTLR_EXCPT_CMPLT, won't change until reset\n",cntrlId);
          }
          else
          {
            DPRINT1(-1,"cntlrSetState: '%s': Logic Error, state was not changed\n",cntrlId);
          }
      }
      else
      {
         pTheCntlrStateObj->CntlrInfo[type][num].state = state;
         /* do not set the errorcode for CNTLR_EXCPT_CMPLT, otherwise the errorcode will be set to zero */
         if ( state != CNTLR_EXCPT_CMPLT)
            pTheCntlrStateObj->CntlrInfo[type][num].errorcode = errorcode;
      }

      pTheCntlrStateObj->cntlrsReponded++; /* increment number of controllers responding */

      DPRINT(1,"cntlrSetState: give & flush Semaphore\n");
      semGive(pTheCntlrStateObj->pSemStateChg);
      semFlush(pTheCntlrStateObj->pSemStateChg);

   DPRINT(2,"cntlrSetState: Give Mutex\n");
   semGive(pTheCntlrStateObj->pSemMutex);

   return(OK);
}

/*************************************************************
*  cntlrSetFifoTicks - set fifo tick from reporting controller 
*
*			Author Greg Brissey
*
*/
int  cntlrSetFifoTicks(char *cntrlId,long long ticks)
{
   int error,type,num;

   if (pTheCntlrStateObj == NULL)
     return(-1);

   error = getType(cntrlId, &type,&num);
   if (error != 0)
     return(-1);

   semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);

      DPRINT3(+1,"cntlrSetFifoTicks: changed '%s' Fifo ticks to: 0x%llx, %lld\n",cntrlId,ticks,ticks);

      pTheCntlrStateObj->CntlrInfo[type][num].fifoTicks = ticks;

   semGive(pTheCntlrStateObj->pSemMutex);

   return(OK);
}

/*************************************************************
*  cntlrSetStateAll - set the state of all controller but master 
*                     then give flag semaphore
*
*			Author Greg Brissey
*
*/
int  cntlrSetStateAll(int state)
{
   int errorcode,type,j,num;

   if (pTheCntlrStateObj == NULL)
     return(-1);

   semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);

   /* DPRINT1(-1,"cntlrSetStateAll: set state: %d\n",state); */
   for(type=0; type < CNTLR_TYPE_MAX; type++)
   {

       // never set lock to be  'in use', there has been instances where that prior to Acode roll call 
       // an abort could happen, prior to this change the
       // the system timed out waiting on the lock to send exception complete, 
       if ( type == 4 )
          continue;

      num = pTheCntlrStateObj->totalOfType[type];
      /* DPRINT2(-1,"cntlrSetStateAll: type: %d, num: %d\n",type,num); */
      if (num == 0)
	 continue;

      for(j=1; j <= num; j++)
      {
          /* DPRINT3(-1,"cntlrSetStateAll: '%s': State: %d, Set State: %d\n",
                pTheCntlrStateObj->CntlrInfo[type][j].cntlrID,
	     	pTheCntlrStateObj->CntlrInfo[type][j].state, state);  */
      /*   if ( strcmp(pTheCntlrStateObj->CntlrInfo[type][j].cntlrID,"master1") != 0) */

            pTheCntlrStateObj->CntlrInfo[type][j].state = state;
      }
   }

   pTheCntlrStateObj->cntlrsReponded=0; /* initialize controllers reponded to zero */

   semGive(pTheCntlrStateObj->pSemMutex);

   return(OK);
}

/*************************************************************
*  cntlrSetInUseAll - set the inuse state of all controller but master 
*                     then give flag semaphore
*
*			Author Greg Brissey
*
*/
int  cntlrSetInUseAll(int state)
{
   int errorcode,type,j,num;

   if (pTheCntlrStateObj == NULL)
     return(-1);

   semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);

   /* DPRINT1(-1,"cntlrSetStateAll: set state: %d\n",state); */
   for(type=0; type < CNTLR_TYPE_MAX; type++)
   {

      num = pTheCntlrStateObj->totalOfType[type];
      /* DPRINT2(-1,"cntlrSetStateAll: type: %d, num: %d\n",type,num); */
      if (num == 0)
	 continue;

      for(j=1; j <= num; j++)
      {
          /* DPRINT3(-1,"cntlrSetStateAll: '%s': State: %d, Set State: %d\n",
                pTheCntlrStateObj->CntlrInfo[type][j].cntlrID,
	     	pTheCntlrStateObj->CntlrInfo[type][j].state, state);  */
            pTheCntlrStateObj->CntlrInfo[type][j].usedInPS = state;
      }
   }

   semGive(pTheCntlrStateObj->pSemMutex);

   return(OK);
}

/*
 * Add controllers to the list, default the controllers to being 'Used'
 *   except for lock which is always unused (in PS)
 */
int cntlrStatesAdd(char *id, int state, int ConfigInfo)
{
   int type, num, errorcode;

   if (pTheCntlrStateObj == NULL)
      return(-1);
   errorcode = getType(id, &type, &num);
   if (errorcode != 0)
     return(-1);
   
   DPRINT3(2,"'%s': type: %d, ordinal#: %d\n",id, type, num); 
   
   DPRINT2(2,"cntlrStatesAdd: '%s' == '%s'\n", pTheCntlrStateObj->CntlrInfo[type][num].cntlrID,id); 
   if (strcmp(pTheCntlrStateObj->CntlrInfo[type][num].cntlrID,id) != 0)
   {
      errorcode = 0;
      semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);
       strncpy(pTheCntlrStateObj->CntlrInfo[type][num].cntlrID,id,16);
       pTheCntlrStateObj->CntlrInfo[type][num].state = state;
       // if it's a lock controller then do not set it as inuse
       pTheCntlrStateObj->CntlrInfo[type][num].usedInPS = (type != 4) ? CNTLR_INUSE_STATE : CNTLR_UNUSED_STATE;
       pTheCntlrStateObj->CntlrInfo[type][num].configInfo = ConfigInfo;
       pTheCntlrStateObj->TotalCntlrs++;
       (pTheCntlrStateObj->totalOfType[type])++;
       DPRINT5(2,"id: '%s', state: %d, type: %d, ord: %d, tnum: %d\n",
	      pTheCntlrStateObj->CntlrInfo[type][num].cntlrID, pTheCntlrStateObj->CntlrInfo[type][num].state,
	      type, num, pTheCntlrStateObj->totalOfType[type]); 
      semGive(pTheCntlrStateObj->pSemMutex);
   }
   else
      errorcode = -1;

   DPRINT3(2,"cntlrStatesAdd: Type (%d) total: %d, Cntlr Total: %d\n", 
	   type, pTheCntlrStateObj->totalOfType[type],pTheCntlrStateObj->TotalCntlrs);
   return(errorcode);
}

void cntlrStatesRemoveAll()
{
   int type,num,j;

   DPRINT(2,"cntlrStatesRemoveAll\n");
   semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);
   for(type=0; type < CNTLR_TYPE_MAX; type++)
   {

      num = pTheCntlrStateObj->totalOfType[type];
      if (num == 0)
         continue;

      // for(j=1; j <= num; j++), resulted in non-deterministic behavior, 
      // depending on if a controller didn't respond, but was within the 
      // structure from previous attempts, e.g. iCAT rf5 showed this error
      // the problem showed itself, when RemoveAll state the total number 
      // of RFs was 4, thus only rf1-rf4 were cleared, however upon a rollcall
      // when rf5 was attempted to be added the cntrl name was still in the
      // state structure and thus believed to be all ready present, however the
      // type total number remained at 4 thus making the structure inconsistent
      // resulting the flash update error. Only rf1-rf4 were check for state
      // changes. Thus when rf5 reported a state change though the total number
      // of controllers reponding increased, the total in a particular state
      // did not change, resulting in cntlrStatesCmpListUntilAllCtlrsRespond()
      // returning prematurely with an error.
      //   GMB 10/10/2010
      for(j=1; j <= MAX_CNTLR_PERTYPE ; j++)
      {
         DPRINT4(2,"%d '%s': State: %d, Inuse: %d, removed.\n",j,
                        pTheCntlrStateObj->CntlrInfo[type][j].cntlrID,
                        pTheCntlrStateObj->CntlrInfo[type][j].state,
                        pTheCntlrStateObj->CntlrInfo[type][j].usedInPS);
         pTheCntlrStateObj->CntlrInfo[type][j].cntlrID[0] =  0;
         pTheCntlrStateObj->CntlrInfo[type][j].state = 0;
         pTheCntlrStateObj->CntlrInfo[type][j].usedInPS = 0;
      }
      pTheCntlrStateObj->totalOfType[type] = 0;
   }
   pTheCntlrStateObj->TotalCntlrs = 0;
   semGive(pTheCntlrStateObj->pSemMutex);
   return;
}

/*
 * return Zero if All ready
 *        number of Controllers not ready, with the char array filled with the controller
 *        names that are not ready
 *
 */
int cntlrStatesCmpList(int cmpstate, int timeout, char *notReadyList)
/* int state 	state - state to test for */
/* int timeout;	 timeout of call, see above */
/* char *notreadyList;	 list of controller not ready */
{
    int state;
    if (pTheCntlrStateObj == NULL)
      return(-1);

    DPRINT2(+1,"cntlrStatesCmpList: state: %d, timeout: %d\n",cmpstate,timeout);
    switch(timeout)
    {
     case NO_WAIT:
          state =  chkStateList(cmpstate,notReadyList);
	  break;

     case WAIT_FOREVER: /* block if state has not changed */
          while( (state = chkStateList(cmpstate,notReadyList)) > 0  )
          {
#ifdef INSTRUMENT
            wvEvent(EVENT_SYSFLAG_SEMTAKE,NULL,NULL);
#endif
	    semTake(pTheCntlrStateObj->pSemStateChg, WAIT_FOREVER);  
          }
	  break;

     default:     /* block if state has not changed, until timeout */
          while( (state = chkStateList(cmpstate,notReadyList)) > 0 )
          {
#ifdef INSTRUMENT
            wvEvent(EVENT_SYSFLAG_SEMTAKE,NULL,NULL);
#endif
            DPRINT(+1,"cntlrStatesCmpList: chkStates failed, Take Semaphore.\n");
            if ( semTake(pTheCntlrStateObj->pSemStateChg, (sysClkRateGet() * timeout) ) != OK )
	    {
                 DPRINT(-1,"cntlrStatesCmpList: Take of Semaphore Timed Out.\n");
	         /* state = ERROR;		/* timed out */
	         break;
	    }
            DPRINT(+1,"cntlrStatesCmpList: Semaphore was Given, Try chkStates again.\n");
          }
          break;
    }
   return(state);
}


/*
 * return Zero if All ready
 *        number of Controllers not ready, with the char array filled with the controller
 *        names that are not ready
 *
 */
int cntlrStatesCmpListUntilAllCtlrsRespond(int cmpstate, int timeout, char *notReadyList)
/* int state 	state - state to test for */
/* int timeout;	 timeout of call, see above */
/* char *notreadyList;	 list of controller not ready */
{
   int NotAtState;
   int cntlrsReponded;

   if (pTheCntlrStateObj == NULL)
     return(-1);

   DPRINT2(1,"cntlrStatesCmpListUntilAllCtlrsRespond: state: %d, timeout: %d\n",cmpstate,timeout);
    switch(timeout)
    {
     case NO_WAIT:
          NotAtState =  chkStateList(cmpstate,notReadyList);
	  break;

     case WAIT_FOREVER: /* block if state has not changed */
          while( (NotAtState = chkStateList(cmpstate,notReadyList)) > 0  )
          {
#ifdef INSTRUMENT
            wvEvent(EVENT_SYSFLAG_SEMTAKE,NULL,NULL);
#endif
	         semTake(pTheCntlrStateObj->pSemStateChg, WAIT_FOREVER);  
          }
	  break;

     default:     /* block if state has not changed, until timeout */

          while(1)
          {
             DPRINT2(1,"cntlrStatesCmpListUntilAllCtlrsRespond: reportedIn: %d, total cntlrs: %d\n", 
                         pTheCntlrStateObj->cntlrsReponded,pTheCntlrStateObj->TotalCntlrs);

             DPRINT(2,"cntlrStatesCmpListUntilAllCtlrsRespond: Take Mutex\n");
             semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);
             DPRINT(2,"cntlrStatesCmpListUntilAllCtlrsRespond: Mutex taken\n");

               NotAtState = chkStateList(cmpstate,notReadyList);
               // get responding controller value while mutex is locked, 
               // so that it doesn't change during following testing
               cntlrsReponded = pTheCntlrStateObj->cntlrsReponded;

             DPRINT(2,"cntlrStatesCmpListUntilAllCtlrsRespond: Give Mutex\n");
             semGive(pTheCntlrStateObj->pSemMutex);
             DPRINT3(1,"cntlrStatesCmpListUntilAllCtlrsRespond: AtState: %d, NotAtState: %d, Total reported in: %d\n", 
                     (pTheCntlrStateObj->TotalCntlrs - NotAtState),NotAtState,cntlrsReponded);
#ifdef INSTRUMENT
            wvEvent(EVENT_SYSFLAG_SEMTAKE,NULL,NULL);
#endif
            /* if all controllers have reported in then break out of loop */
            if ( cntlrsReponded == pTheCntlrStateObj->TotalCntlrs)
               break;

            DPRINT1(+1,"cntlrStatesCmpListUntilAllCtlrsRespond: %d Cntlrs not at State, Take Semaphore.\n",NotAtState);
            if ( semTake(pTheCntlrStateObj->pSemStateChg, (sysClkRateGet() * timeout) ) != OK )
	    {
                 DPRINT(-1,"cntlrStatesCmpListUntilAllCtlrsRespond: Take of Semaphore Timed Out.\n");
	         break;
	    }
            DPRINT(+1,"cntlrStatesCmpListUntilAllCtlrsRespond: Semaphore was Given, Try chkStates again.\n");
          }

          break;
    }
   return(NotAtState);
}


/*
 * Give an array of controllers (e.g. master1,rf1,rf2,rf3,pfg1,lock1,ddr1)
 * Set their usedInPS state to true.
 * 
 *  return:
 *    0 - if all were changed
 *    ERROR - number of missing controllers from list 
 *
 *   Author: Greg Brissey 9/14/04
 */
int cntlrSet2UsedInPS(char *cntlrList, char *missingList)
{
   int errorcode,type,num,numMissing;
   int result,j,found;
   char CList[MAX_ROLLCALL_STRLEN];
   char *pCList;
   char *cntlrName;

   if (pTheCntlrStateObj == NULL)
     return(-1);

   strncpy(CList,cntlrList,MAX_ROLLCALL_STRLEN);
   pCList = CList;
   /* DPRINT1(-1,"pCList: '%s'\n",pCList); */
   numMissing = 0;

   /* 1st set all controllers to not used, then we go through the list */
   cntlrSetInUseAll(CNTLR_UNUSED_STATE);

   for (cntlrName = (char*) strtok_r(CList," ,",&pCList);
         cntlrName != NULL;
           cntlrName = (char*) strtok_r(NULL," ,",&pCList))
   {
      /* extract the controller type and instance number from the requested controller */
      errorcode = getType(cntlrName, &type, &num);
      DPRINT3(+4,"cntlrSet2UsedInPS: cntlr: '%s', type: %d, ordinal#: %d\n",cntlrName,type,num);
      if (errorcode != 0)
      {
        DPRINT1(-1,"Unknown controller type: '%s'\n",cntlrName);
        return(-1);
      }

      DPRINT1(+4,"cntlrSet2UsedInPS: Cntlr: '%s' \n",pTheCntlrStateObj->CntlrInfo[type][num].cntlrID);
      semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);
      result = strcmp(pTheCntlrStateObj->CntlrInfo[type][num].cntlrID,cntlrName);
      semGive(pTheCntlrStateObj->pSemMutex);
   
      if ( result == 0)
      {
	  pTheCntlrStateObj->CntlrInfo[type][num].usedInPS = CNTLR_INUSE_STATE;
      }    
      else
      {
        if (numMissing != 0)
           strcat(missingList," ");
        strcat(missingList,cntlrName);
        numMissing++;
      }
   }

   return(numMissing);
}


/*
 * Give an array of controllers (e.g. master1,rf1,rf2,rf3,pfg1,lock1,ddr1)
 * Check to verify that these are present within the list.
 * 
 *  return:
 *    0 - if all preset
 *    number of missing controllers;
 *    fills in passed string with space separated list of missing controllers
 *
 *   Author: Greg Brissey 9/14/04
 */
int cntlrPresentsVerify(char *cntlrList, char *missingList)
{
   int errorcode,type,num,numMissing;
   int j,found;
   char CList[MAX_ROLLCALL_STRLEN];
   char *pCList;

   char *cntlrName;

   if (pTheCntlrStateObj == NULL)
     return(-1);

   numMissing = 0;

   strncpy(CList,cntlrList,MAX_ROLLCALL_STRLEN);
   pCList = CList;
   /* DPRINT1(-1,"pCList: '%s'\n",pCList); */

   // while ( (cntlrName = (char*) strtok_r(pCList," ,",&pCList)) != NULL)
   for (cntlrName = (char*) strtok_r(CList," ,",&pCList);
         cntlrName != NULL;
            cntlrName = (char*) strtok_r(NULL," ,",&pCList))
   {


      errorcode = getType(cntlrName, &type,&num);
      /* DPRINT3(-1,"cntlrPresentsVerify: cntlr: '%s', type: %d, ordinal#: %d\n",
                    cntlrName,type,num); */
      if (errorcode != 0)
      {
        DPRINT1(-1,"Unknow controller type: '%s'\n",cntlrName);
        return(-1);
      }


      semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);
        num = pTheCntlrStateObj->totalOfType[type];
      semGive(pTheCntlrStateObj->pSemMutex);
      if (num == 0)
      {
        if (numMissing != 0)
           strcat(missingList," ");
        strcat(missingList,cntlrName);
        numMissing++;
        continue;
      }


      found = 0;
      for(j=1; j <= num; j++)
      {
          /* DPRINT3(-1,"cntlrSetStateAll: '%s': State: %d, Set State: %d\n",
                pTheCntlrStateObj->CntlrInfo[type][j].cntlrID,
	     	pTheCntlrStateObj->CntlrInfo[type][j].state, state);  */
         if ( strcmp(pTheCntlrStateObj->CntlrInfo[type][j].cntlrID,cntlrName) == 0)
         {
            found = 1;
            break;
         }    

      }
      if (!found)
      {
        if (numMissing != 0)
           strcat(missingList," ");
        strcat(missingList,cntlrName);
        numMissing++;
      }
   }

   return(numMissing);

}
/*
 * return a string of controller that are present (e.g. master1,rf1,rf2,rf3,pfg1,lock1,ddr1)
 * 
 *  return:
 *    0 - if all preset
 *    number of missing controllers;
 *    fills in passed string with space separated list of missing controllers
 *
 *   Author: Greg Brissey 9/14/04
 */
int cntlrPresentGet(char *cntlrList)
{
   int errorcode,type,num,numPresent;
   int j,found;

   numPresent = 0;
   cntlrList[0] = 0;
   semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);
   for(type=0; type < CNTLR_TYPE_MAX; type++)
   {

      num = pTheCntlrStateObj->totalOfType[type];
      if (num == 0)
         continue;

      for(j=1; j <= num; j++)
      {
        if (numPresent != 0)
           strcat(cntlrList," ");
        strcat(cntlrList,pTheCntlrStateObj->CntlrInfo[type][j].cntlrID);
        numPresent++;
      }
   }
   semGive(pTheCntlrStateObj->pSemMutex);

   return(numPresent);
}

int cntlrConfigGet(char *cntlrType, int configType, char *cntlrList)
{
   int type,num,error,i,total;
   cntlrList[0] = 0;
   error =  getType(cntlrType, &type, &num);
   if (error != 0)
     return(-1);

   total = 0;
   semTake(pTheCntlrStateObj->pSemMutex,WAIT_FOREVER);
     num = pTheCntlrStateObj->totalOfType[type];
   semGive(pTheCntlrStateObj->pSemMutex);

   if (num == 0)
      return 0;

   // printf("number of '%s' (%d): %d\n", cntlrType, type, num);
   for(i=1; i <= num; i++)
   {
       /* printf("%d: '%s': config: %d  vs %d\n",i,pTheCntlrStateObj->CntlrInfo[type][i].cntlrID,
        *                      pTheCntlrStateObj->CntlrInfo[type][i].configInfo,configType); */
       if ( pTheCntlrStateObj->CntlrInfo[type][i].configInfo == configType )
       {
           if (total != 0)
              strcat(cntlrList," ");
           strcat(cntlrList,pTheCntlrStateObj->CntlrInfo[type][i].cntlrID);
           total++;
       }
   }
   return total;
   
}

void InitCntlrStates()
{
   pTheCntlrStateObj = cntrlStatesCreate();
}

struct __report
{
   int completes;
   int curcnt;
   int fails;
   int errLog[80];
} tSn;

printSnoop()
{
  int i,k;
  printf("runs = %d  runs with errors = %d\n",\
     tSn.completes,tSn.fails);
  if (tSn.completes > 0) printf("%% run failure=%f\n",100.0*tSn.fails/tSn.completes);
  if (tSn.fails > 80) k = 80; else k = tSn.fails;
  printf("fail run counters\n");
  for (i=0; i < k; i++)
  {
    printf("%d\n",tSn.errLog[i]);
  }
} 

int ilock = 1;

clearSnoop()
{
  int i;
  tSn.completes = tSn.curcnt = tSn.fails = 0;
  for (i = 0; i < 80; i++)  tSn.errLog[i] = 0;
  ilock = 0;
}
/*
 * report the report fifo ticks, and display a delta from the master
 *
 *           Author:  Greg Brissey  9/7/05
 *
 */
int cntlrStateReportFifoTicks()
{
   int type,j,num;
   int state, used, errorcode;
   long long ticks, masterTicks;
   double masterDuration,duration;
   int cntrtag,sutrap;

    if (pTheCntlrStateObj == NULL)
       return (-1);

   if (ilock == 1) clearSnoop();
   /* for easier comparison the additional 8 ticks the master has is subtracted */
   masterTicks = pTheCntlrStateObj->CntlrInfo[0][1].fifoTicks - 8LL;    /* master1 */
   masterDuration = (((double) masterTicks) * .0125);
   diagPrint(NULL," %8s: Duration: 0x%llx, %llu (ticks) or %lf us \n",
         pTheCntlrStateObj->CntlrInfo[0][1].cntlrID,masterTicks, masterTicks, masterDuration);

   /* start at rf */
   cntrtag = 0;  
   sutrap = 0;
   for(type=1; type < CNTLR_TYPE_MAX; type++)
   {

      num = pTheCntlrStateObj->totalOfType[type];
      if (num == 0)
	 continue;

      for(j=1; j <= num; j++)
      {
	 used = pTheCntlrStateObj->CntlrInfo[type][j].usedInPS;
         if (used != CNTLR_INUSE_STATE)
           continue;

	 ticks = pTheCntlrStateObj->CntlrInfo[type][j].fifoTicks;
         duration = (((double) ticks) * .0125);
         if (duration < 120000.0) sutrap=1;
         diagPrint(NULL," %8s: Duration: 0x%llx, %llu (ticks) or %lf us, Delta: %lld (ticks), %lf us \n",
			pTheCntlrStateObj->CntlrInfo[type][j].cntlrID,ticks, ticks, duration, ticks - masterTicks, duration - masterDuration);
         if (duration != masterDuration)
         {  
           cntrtag = 1; 
         }
      }
   }
   if (sutrap == 0) {
   tSn.completes++;
   tSn.curcnt++;
   tSn.fails += cntrtag;
   diagPrint(NULL,".....run number %d  (%d,%d)",tSn.curcnt,tSn.fails,tSn.completes);
   if (tSn.fails < 80) 
   {
     tSn.errLog[tSn.fails] = tSn.curcnt;
   }
   }
   return(cntrtag);
}


cntlrStateShow()
{
   int type,j,num;
   int state, used, config, errorcode;

    if (pTheCntlrStateObj == NULL)
       return (-1);

   diagPrint(NULL," Controller States:   %d Controllers Present\n",pTheCntlrStateObj->TotalCntlrs);
   for(type=0; type < CNTLR_TYPE_MAX; type++)
   {

      num = pTheCntlrStateObj->totalOfType[type];
      if (num == 0)
	      continue;

      for(j=1; j <= num; j++)
      {
	     state = pTheCntlrStateObj->CntlrInfo[type][j].state;
	     used = pTheCntlrStateObj->CntlrInfo[type][j].usedInPS;
	     config = pTheCntlrStateObj->CntlrInfo[type][j].configInfo;
	     errorcode = pTheCntlrStateObj->CntlrInfo[type][j].errorcode;
        diagPrint(NULL," '%s': Ctlr State: '%s', PS State: '%s', Config: '%s', errorcode: %d, Fifo Duration: %llu, %lf us\n",
                  pTheCntlrStateObj->CntlrInfo[type][j].cntlrID,
                  getCntlrStateStr(state), getCntlrUseStateStr(used),
                  getCntlrConfigStr(config),errorcode, pTheCntlrStateObj->CntlrInfo[type][j].fifoTicks,
			(((double) pTheCntlrStateObj->CntlrInfo[type][j].fifoTicks) * .0125));
      }
   }
}

#ifdef TESTING
static char *names = "master1 rf1 rf2 rf3 rf4 pfg1 lock1 ddr1 ddr2 lpfg1";
qtst()
{
   char *Vnames;
   char mcntlrs[MAX_ROLLCALL_STRLEN];
   int nmissing;
   Vnames = names;

   mcntlrs[0] = 0;

   nmissing = cntlrPresentsVerify(Vnames, mcntlrs);
   DPRINT1(-1,"num missing: %d\n",nmissing);
   DPRINT1(-1,"MissingList: '%s'\n",mcntlrs);

}

qintst()
{
   char *Vnames;
   char mcntlrs[MAX_ROLLCALL_STRLEN];
   int nmissing;
   Vnames = names;

   mcntlrs[0] = 0;

   nmissing = cntlrSet2UsedInPS(Vnames, mcntlrs);
   DPRINT1(-1,"num missing: %d\n",nmissing);
   DPRINT1(-1,"MissingList: '%s'\n",mcntlrs);
}
#endif
