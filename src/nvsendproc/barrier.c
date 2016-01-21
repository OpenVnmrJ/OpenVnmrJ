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


#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>

#include "barrier.h"
#include "errLogLib.h"

/*
 *
 */
int barrierInit(barrier_t *barrier, int count)
{
   int status;
   
   barrier->threshold = barrier->counter = count;
   barrier->cycle = 0L;
   barrier->forcedRelease = 0;
   status = pthread_mutex_init( &barrier->mutex, NULL );
   if (status != 0)
       return status;

   status = pthread_cond_init( &barrier->cv, NULL);
   if (status != 0)
   {
      pthread_mutex_destroy( &barrier->mutex );
      return status;
   }
   barrier->valid = BARRIER_VALID;
   return 0;
}

/*
 * change the membership number, must be done prior to usage however
 */
int barrierSetCount(barrier_t *barrier,int count)
{
    int status, cancel, temp, cycle;

    if (barrier->valid != BARRIER_VALID)
          return -42;

    status = pthread_mutex_lock( &barrier->mutex );
    if (status != 0)
        return status;

   barrier->threshold = barrier->counter = barrier->resetThreshold = count;
   barrier->forcedRelease = 0;
   DPRINT1(+2,"barrierSetCount: new count = %d\n",barrier->threshold);

   status = pthread_mutex_unlock( &barrier->mutex);
   return status;
}

int barrierDecThreshold(barrier_t *barrier)
{
    int status;
    if (barrier->valid != BARRIER_VALID)
          return -42;

    status = pthread_mutex_lock( &barrier->mutex );
    if (status != 0)
        return status;

   --barrier->threshold;
   DPRINT1(+2,"barrierDecThreshold: decrement threashold to %d\n",barrier->threshold);
   if (--barrier->counter == 0)
    {
       DPRINT(+2,"barrierDecThreshold: count Zero, broadcast to restart threads\n");
       barrier->cycle++;
       barrier->counter = barrier->threshold;
       status = pthread_cond_broadcast( &barrier->cv );

       /*
        * Last thread to reach the barrier will be give a special return status
        * of -1 instead of 0. Allow last thread to do anything special that is required.
        */
        if (status == 0)
           status =  -1;
    }
   DPRINT1(+2,"barrierDecThreshold: new threshold = %d\n",barrier->threshold);

   status = pthread_mutex_unlock( &barrier->mutex);
   return status;
}

int barrierResetThreshold(barrier_t *barrier)
{
    int status;
    if (barrier->valid != BARRIER_VALID)
          return -42;

    status = pthread_mutex_lock( &barrier->mutex );
    if (status != 0)
        return status;

   barrier->threshold = barrier->resetThreshold;
   barrier->forcedRelease = 0;
   DPRINT1(+2,"barrierResetThreshold: reset threshold = %d\n",barrier->threshold);

   status = pthread_mutex_unlock( &barrier->mutex);
   return status;
}

int barrierWaitAbort(barrier_t *barrier)
{
    int status;
    if (barrier->valid != BARRIER_VALID)
          return -42;

    status = pthread_mutex_lock( &barrier->mutex );
    if (status != 0)
        return status;

    /* Not even going to bother with this test, just always braodcast it */
    /* if (barrier->counter != barrier->threshold)  /* somebody is waiting */

    DPRINT(+2,"barrierAbortWait: force waiting threads to continue, broadcast to restart thread\n");
    barrier->forcedRelease = 1;
    barrier->cycle++;
    barrier->counter = barrier->threshold;
    status = pthread_cond_broadcast( &barrier->cv );

    pthread_mutex_unlock( &barrier->mutex);
    return status;     /* error, -1 for waker, or 0 */
}

int barrier_destory(barrier_t *barrier)
{
}

/*
 * forces thread to wait until all members of a barrier, reach the barrier.
 * When the count (remaining memebers) reaches 0, wake up all threads waiting
 */
int barrierWait(barrier_t *barrier)
{
    int status, cancel, temp, cycle;

    if (barrier->valid != BARRIER_VALID)
          return -42; /* EINVAL; */


    status = pthread_mutex_lock( &barrier->mutex );
    if (status != 0)
        return status;

    cycle = barrier->cycle;  /* note which cycle we are on */

    if (--barrier->counter == 0)
    {
       DPRINT(+2,"barrierWait: count Zero, broadcast to restart thread\n");
       barrier->cycle++;
       barrier->counter = barrier->threshold;
       status = pthread_cond_broadcast( &barrier->cv );

       /*
        * Last thread to reach the barrier will be give a special return status
        * of -1 instead of 0. Allow last thread to do anything special that is required.
        */
        if ((status == 0) && (barrier->forcedRelease == 0))
           status =  -1;
        else
           status = -99;
    }
    else
    {
       /* not a cancellation point, disable it */
       pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &cancel);

       /*
        * Wait until barrier cycle has changes, which means that it
        * has been boradcast to continue
        */
       while( cycle == barrier->cycle )
       {
          status = pthread_cond_wait( &barrier->cv, &barrier->mutex);
          if (status != 0)  break;
       }

       if (barrier->forcedRelease == 1)
         status = -99;
       pthread_setcancelstate( cancel, &temp);
    }
    pthread_mutex_unlock( &barrier->mutex);
    return status;     /* error, -1 for waker, or 0, abort = -99 */
}
