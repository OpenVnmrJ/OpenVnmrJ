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

#include <pthread.h>

/*
 * Barrier structure for threads
 */
typedef struct barrier_tag {
   pthread_mutex_t   mutex;  /* control access to barrier */
   pthread_cond_t    cv;    /* wait for barrier */
   int                valid; /* set to magic number when valid */
   int    threshold;        /* number thread require to continue */
   int     counter;         /* current number of threads waiting */
   int    resetThreshold;
   unsigned int cycle;            /* cycle count */
   int    forcedRelease;    /* abnormal release from barrier, i.e. abort condition */
 } barrier_t;

#define BARRIER_VALID  0xcafebf

/*
* Static init
*/

/*
 *  functions
 */
extern int barrrierInit( barrier_t *barrier, int count);
extern int barrierSetCount(barrier_t *barrier,int count);
extern int barrierDestroy(barrier_t *barrier);
extern int barrierDecThreshold(barrier_t *barrier);
extern int barrierResetThreshold(barrier_t *barrier);
extern int barrierWaitAbort(barrier_t *barrier);
extern int barrierWait(barrier_t *barrier);
