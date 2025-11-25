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
 *   DDR Controller Descriptor Structure
 * this struct contains the information on the DDR controller Publication/Subscription
 * responcable thread and pipes for handling the data from this controller
 *
 *		Author   Greg Brissey 8/20/05
 */
#ifndef rcvrDesc_h
#define rcvrDesc_h

#ifndef LINUX
#include <thread.h>
#endif
#include <pthread.h>
#include <sys/time.h>

#include "NDDS_Obj.h"
#include "mfileObj.h"
#include "rngBlkLib.h"
#include "workQObj.h"


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* typedef int (*PFIV)(void*, void *arg); */
typedef int (*PFIV)(void *arg);

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
typedef struct _shared_data_ {
    int AbortFlag;
    int discardIssues;
    MFILE_ID   pMapBufFile;
    MFILE_ID   pMapFidFile;	/* fid data MMAP file */
    FILE_ID    pFidFile;        /* Fid data File, using regular file I/O */
} SHARED_DATA, *SHARED_DATA_ID;

typedef struct _diagnostc_data_ {
      struct timeval tp;
      int    pipeHighWaterMark;
      int    workQLowWaterMark;
} DIAG_DATA, *DIAG_DATA_ID;

/* structure passed to routines to do work on data, etc. */
typedef struct _rcvrDesc_ {
    char           cntlrId[32];     /* ddr controller name,  ddr1, ddr2, ddr3, etc. */
    pthread_t      threadID;        /* thread Id of thread dedicated to handling the FID traffic */
    NDDS_ID        PubId;	    /* Publication to the DDR */
    NDDS_ID        SubId;           /* Subscription to the DDR data stream */
    NDDS_ID	   SubHBId;         /* Heart Beat Sub from node the thread is servicing */
    RINGBLK_ID     pInputQ;	    /* Work Input Q for the thread handling this DDR's data */
    RINGBLK_ID     pOutputQ;	    /* Work Output Q for the thread handling this DDR's data */
    WORKQ_ID       pWorkQObj;	    /* pointer to work Object */
    WORKQ_ENTRY_ID activeWrkQEntry; /* A work Q entry used only be the NDDS callback, work q entries are then passed along */
    unsigned int  prevElemId;      /* used to checking lost issues */
    void           *pParam;         /* shared experiment info structure */
    void           *pFidBlockHeader;/* point to aunqiue fid block head for this DDR */
    PFIV           pCallbackFunc;   /* routine that thread calls to do actually work of this stage */
    pthread_mutex_t   *globalMemMutex; /* Must lock and unlock when changing any shared variable between threads !! */
    DIAG_DATA     *p4Diagnostics;   /* useful for diagnostic to keep unique info on a per rcvr basis */
    void          *pPrivateReserved;
} RCVR_DESC, *RCVR_DESC_ID; 

 
#ifdef __cplusplus
}
#endif
 
#endif



