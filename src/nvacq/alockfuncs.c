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
/*
DESCRIPTION

   Auto Task 

*/

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <msgQLib.h>
/* #include <arch/ppc/ivPpc.h> */
 
#include "logMsgLib.h"
#include "taskPriority.h"
#include "nvhardware.h"
#include "Cntlr_Comm.h"
#include "Console_Stat.h"
#include "Lock_FID.h"
#include "Lock_Stat.h"
#include "Lock_Cmd.h"
#include "autolock.h"

/* DO NOT Alter these pointers !!!!!! */

MSG_Q_ID pMsgesToAutoLock = NULL;

extern Console_Stat     *pCurrentStatBlock;     /* Acqstat-like status block */


/*  in moving the autolock from lock controller to the Master we no longer
    need a separate task, autolock can run in the context of shandler
*/

#ifdef NOT_ON_LOCKCNTLR

/*
 * Autolock Task, this is here so that the autolock algorithym is perform in 
 *     this task context and not anothers
 *
 *      Author:  Greg Brissey    1/6/2005
 */
void AutoLock()
{
   ALOCK_MSG alockMsg;
   int bytes,errorcode;
   int doAlock(ALOCK_MSG_ID msg);

   DPRINT(-1,"AutoLock:Server LOOP Ready & Waiting.\n");
   FOREVER
   {
      /* get msg from lockcomm */
      bytes = msgQReceive(pMsgesToAutoLock, (char *) &alockMsg, sizeof(ALOCK_MSG), WAIT_FOREVER);
      errorcode = doAlock(&alockMsg);
   } 

}

int doAlock(ALOCK_MSG_ID msg)
{
     DPRINT3(-1,"doAlock: mode: %d, maxpower: %d, maxgain: %d\n",
	msg->mode,msg->maxpwr, msg->maxgain);
}

void startAutoLock(int priority, int taskOptions, int stackSize)
{
   int AutoLockTid;

   if (pMsgesToAutoLock == NULL)
      pMsgesToAutoLock = msgQCreate(10,sizeof(ALOCK_MSG), MSG_Q_FIFO);

   if (taskNameToId("tALockSvc") == ERROR)
   {
     AutoLockTid = taskSpawn("tALockSvc",priority, taskOptions, stackSize,
		       (void *) AutoLock,0,0, 0,0,0,0,0,0,0,0);
     if (AutoLockTid == ERROR)
     {
       perror("taskSpawn");
       errLogRet(LOGIT,debugInfo, "startAutoLock():  ****** task Spawn failed\n");
     }
   }
   return;
}
#endif


/*  Routines used by autolock, found in X_interp.c */
/*
chlock() {};     check if locked 
lk2kcs() {};
lk2kcf() {};
lk20Hz() {};
getmode() {};
setmode() {};
set_lock_offset() {};
get_lock_offset() {};
*/

/*
 * doAutoLock  - stop over on the way to do_autolock
 *               start the Lock Subscription is not already established
 *
 *  alock='n' = 0,  alock='y' = 1, alock='a' = 3, alock='s' = 4, alock='u' = 5, alock='f' = 3+8 
 *     Author: Greg Brissey
 */
int doAutoLock(int lkmode,int maxpwr,int maxgain, int sampleHasChanged)
{
    extern int initLockFidSub(int updateRateInHz);
    initLockFidSub(2); /* 2 Hz */
    return(  do_autolock(lkmode, maxpwr, maxgain, sampleHasChanged) );
}

/*
 *
 * getlkfid()
 *
 * getlkfid(lkdata,SCANS,LK20HZ) 
 * returns 1 = error, 0 = OK 
 *
 * Author: Greg Brissey  1/10/2005
 */
int getlkfid(int *data,int nt,int filter)
{
   int len,result,i;
   /* ignore nt and filter */
   /* 1. are we in the right filter setting,  
	LK20HZ == locktc(),   fast filter
        lockacqtc    slow filter
   */

    result = 0;
    len = getLkData(data);
    DPRINT1( 4,"len = %d\n",len);
    if ((DebugLevel > 1) && ( len != -1) )
    {
       for(i=0; i < len; i++)
       {
           printf(" %d - %d, ",i,data[i]);
           if ((i % 10 ) == 0)  printf("\n");
       }
    }
    if (len == -1)
    {
      DPRINT(-1,"getLkData() timeout waiting for Lock FID data\n");
      result = 1;
    }
      
    return(result); 
}

/*
*initlksub()
*{
*   initLockFidSub(2);
*}
*tstlkfid()
*{
*    int lkdata[512];
*    getlkfid(lkdata,512,16);
*}
*
*tstLkFid()
*{
*    int lkdata[512];
*    int len;
*    len = getLkData(lkdata);
*}
*/


/* just a stub this need to be altered since 
   lock frequency are doubles now 
*/
double  getLockFreqAP() 
{ 
   return (get_lkfreq_ap());
}

/* init_dac() { does absolutely nothing }; */

/*
 * Obtain the lock level from the update status block 
 *
 */
int getLockLevel() 
{ 
   return((int) (pCurrentStatBlock->AcqLockLevel) );
}

int getSpinSet() 
{ 
   return((int) (pCurrentStatBlock->AcqSpinSet) );
}

void get_all_dacs(int arr[], int len)
{
   int index;

   for (index=0; index < len; index++)
      arr[index] = (int) pCurrentStatBlock->AcqShimValues[ index ];
}

/*----------------------------------------------------------------------*/
/* time_t secondClock(&chkval,mode) - mode = 1 - start clock		*/
/*				   mode = 0 - elasped time in seconds	*/
/*----------------------------------------------------------------------*/
time_t secondClock(time_t *chkval,int mode)
{
   if (mode)
   {
     *chkval =  time(NULL);
     return(0);
   }
   else
   {
      return(time(NULL) - *chkval);
   }
}

/*
 * Set the hardware lock On or Off
 */
int setLockMode(int newMode)
{
    setmode(newMode);
    return 0;
}

int getLockMode()
{
    return( getmode() );
}


/*
 * setLockPower()
 * Set the Lock Power
 * Then return when the lock controller updated the status to show the
 * the power is set to what was sent.
 * if it does change within a second the routines -1 as error.
 *
 *   Author: Greg Brissey 1/7/05
 */
int setLockPower(int newPower)
{
     int retries, presentPower, result;
   
     presentPower = getpower();
     DPRINT2(1,"setLockPower: Power: %d, Present Power: %d\n",newPower,presentPower);
     if (newPower == presentPower) /* no since in changing what is already set */
        return 0;

     setpower(newPower);  /* master & lock controllers both involved in setting this */

     /* wait for number of tries in one second base on clock rate */
     /* clock is 60 ticks per sec, then try 60 times with pause of 1 tick between tries */
     result = -1;  /* default failure */
     for (retries = sysClkRateGet() ; retries > 0; retries--)
     {
       taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
       presentPower = getpower();
       /* DPRINT2(-1,"setLockPower: Power: %d, Present Power: %d\n",newPower,presentPower); */
       if (presentPower == newPower)
       {
          result = 0;   /* success */
          break;
       }
     }
     return( result );
}

int getLockPower() 
{
    return ( getpower() );
}



/*
 * setLockGain()
 * Set the Lock Gain
 * Then return when the lock controller updated the status to show the
 * the gain is set to what was sent.
 * if it does change within a second the routines -1 as error.
 *
 *   Author: Greg Brissey 1/7/05
 */
int setLockGain(int newGain)
{
     int retries, presentGain, result;
   
     presentGain = getgain();
     DPRINT2(1,"setLockGain: Gain: %d, Present Gain: %d\n",newGain,presentGain);
     if (newGain == presentGain) /* no since in changing what is already set */
        return 0;

     setgain(newGain);  /* lock controller involved in setting this */

     /* wait for number of tries in one second base on clock rate */
     /* clock is 60 ticks per sec, then try 60 times with pause of 1 tick between tries */
     result = -1;  /* default failure */
     for (retries = sysClkRateGet() ; retries > 0; retries--)
     {
       taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
       presentGain = getgain();
       DPRINT2(-1,"setLockGain: Gain: %d, Present Gain: %d\n",newGain,presentGain); 
       if (presentGain == newGain)
       {
          result = 0;   /* success */
          break;
       }
     }
     return( result );
}

int getLockGain() 
{
    return ( getgain() );
}


/*
 * setLockPhase()
 * Set the Lock Phase
 * Then return when the lock controller updated the status to show the
 * the phase is set to what was sent.
 * if it does change within a second the routines -1 as error.
 *
 *   Author: Greg Brissey 1/7/05
 */
int setLockPhase(int newPhase)
{
     int retries, presentPhase, result;
   
     presentPhase = getphase();
     DPRINT2(1,"setLockPhase: Phase: %d, Present Phase: %d\n",newPhase,presentPhase);
     if (newPhase == presentPhase) /* no since in changing what is already set */
        return 0;

     setphase(newPhase);  /* master & lock controllers both involved in setting this */

     /* wait for number of tries in one second base on clock rate */
     /* clock is 60 ticks per sec, then try 60 times with pause of 1 tick between tries */
     result = -1;  /* default failure */
     for (retries = sysClkRateGet() ; retries > 0; retries--)
     {
       taskDelay(calcSysClkTicks(17));  /* taskDelay(1); */
       presentPhase = getphase();
       /* DPRINT2(-1,"setLockPhase: Phase: %d, Present Phase: %d\n",newPhase,presentPhase); */
       if (presentPhase == newPhase)
       {
          result = 0;   /* success */
          break;
       }
     }
     return( result );
}

int getLockPhase() 
{
    return ( getphase() );
}


updateAcqState(int state)
{
   setAcqState(state);
}

getUpdateAcqState()
{
  return ( getAcqState() );
}

set_ShimZ0(int Z0_Value)
{
   /* send2Master(CNTLR_CMD_SET_SHIMDAC, Z0_Value); */
}

set_LockFreq( int value )
{
   /* Hmmmm, now what? */
   /* setDLockFreq(int a, int b); */
}

int get_LockPower()
{
  /* return( pLkStatIssue->lkpower ); */
}
 
int get_LockLevel()
{
  /* return ( pLkStatIssue->lkLevelR ); */
}

/*--------------------------------------------------------------*/
/* calcgain(level,requested level)				*/
/*   Calculate a new lock gain change for a new level		*/
/*   (long) level - present signal level			*/
/*   (long) requested level - level desired			*/
/*								*/
/*   First the level is divide by 2 untill the level is close   */
/*   to the requested level. For each division or mult by 2     */
/*   a corresponding 6db in gain is needed.			*/
/*   After division or mult by two the percent of the remainder */
/*   to the requested level is obtained. For each 16% off       */
/*   1db of gain change is needed.				*/
/*--------------------------------------------------------------*/
int calcgain(long level,long reqlevel)
{
   long value,diff,percnt,percnt1;
   int cnt,coarse,fine,sign;

   value = level;
   sign = cnt = 0;

   diff = reqlevel - value;
   if (diff < 0L)
      diff = -diff;
   percnt1 = (diff*1000L) / reqlevel;	/* % diff of level to req. level */

  /*--- count number of times divided or mult by 2 to reach target value --*/
   if (level > reqlevel) 
   {
      while(level > reqlevel)
      {
         level >>= 1;
         cnt--;
      }
      sign = 1;	/* gain change in positive direction */
   }
   else
   {
      if (level < reqlevel)
      {
         while(level < reqlevel)
         {
            level <<= 1;
            cnt++;
         }
         sign = -1; /* gain change in negative direct */
      }
   }
   coarse = 6 * cnt;	/* course db adjustment (6db double signal)  */

 /*-- calc percent diff from value to reqlevel for each 16% its 1db  */

   /*fine = ((((reqlevel - level)*1000L) / level) / 16L) / 10L;*/
   diff = reqlevel - level;
   if (diff < 0L)
      diff = -diff;
   percnt = (diff*1000L) / level;
   fine = ((percnt / 16L) + 5L) / 10L;
   fine *= sign;
   DPRINT7(1,
    "Calcgain: value=%ld req=%ld %%dif=%ld, coarse=%d, dif=%ld, %%=%ld, fine=%d\n",
       value,reqlevel,percnt1/10,coarse,diff,percnt,fine);

   if (percnt1 < 160L)
      return(0);	/* % < 16% no gain change is needed */
   else
      return(coarse + fine);
}

