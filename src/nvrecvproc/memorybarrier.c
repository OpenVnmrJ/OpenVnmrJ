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
#include <string.h>
#include <signal.h>
#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>

#include "memorybarrier.h"

/* --------- WARNING --------- */
/* shared memory between pthread requires a memory barrier
   do to the memory visibility rules ...
   1. values a thread can see when it call pthread_create can be seen by the new pthread
   2. values a thread can see when it unlock a mutex, can be seen by a thread that locks
      the SAME mutex. 
      DANGER: Values changed After the unlock are NOT GUARANTEE to be seen by other threads.
   3.
   4.
 */

/*
 *  Initialize the shared memory barrier construct
 *
 *   Author: Greg Brissey   8/25/2005
 */
int initMemBarrier(pMembarrier_t pMemBarrier, void *pSharedData)
{
   int stat;

   DPRINT1(+1,"Shared Struct Addr: 0x%lx\n",pSharedData);

   /* clear complete structure */
   memset(pMemBarrier,0,sizeof(membarrier_t));
   stat = pthread_mutex_init(&pMemBarrier->mutex,NULL);  /* assign defaults to mutex */
   pMemBarrier->sharedData = pSharedData;
   return 0;
}
 
/*
 * convenience routines to lock and unlock the shared data mutex. 
 * It is up to the App. to loc, thenk change parameters within the shared data structure
 * then unlock.   Failure to do so, WILL result in unpredictable results!
 *
 *       Author:  Greg Brissey     8/25/05
 */
void *lockSharedData(pMembarrier_t pMemBarrier)
{
    int status;
    status = pthread_mutex_lock (&pMemBarrier->mutex);
    if (status != 0)
       return (NULL);     /* errLogSysQuit(LOGOPT,debugInfo,"Could not lock memory barrier mutex"); */
    else    
      return (pMemBarrier->sharedData);
}

int unlockSharedData(pMembarrier_t pMemBarrier)
{
    int status;
    status = pthread_mutex_unlock (&pMemBarrier->mutex);
    if (status != 0)
       errLogSysRet(LOGOPT,debugInfo,"Could not unlock memory barrier mutex");
    return (status);
}
