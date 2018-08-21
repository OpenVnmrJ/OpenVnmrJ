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
#ifndef memorybarrier_h
#define memorybarrier_h

#include <signal.h>
#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>

#include "errLogLib.h"
#include "mfileObj.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

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
typedef struct _membarrier {
    void *sharedData;
    pthread_mutex_t   mutex; /* Must lock and unlock when changing any shared variable between threads !! */
} membarrier_t, *pMembarrier_t;

extern int initMemBarrier(pMembarrier_t pMemBarrier, void *pSharedData);
extern void *lockSharedData(pMembarrier_t pMemBarrier);
extern int unlockSharedData(pMembarrier_t pMemBarrier);

#ifdef __cplusplus
}
#endif
 
#endif
