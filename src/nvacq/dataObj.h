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
#ifndef dataObj_h
#define dataObj_h

#include <msgQLib.h>
#include "hostAcqStructs.h"
#include "rngXBlkLib.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*  Do NOT define NO_TAG to be -1, for the DATA equipment
    uses this value at abort, halt and stop experiment.  */

#define NO_TAG	-2

#define NO_DATA 1
#define MAX_BUFFER_ALLOCATION (1024 * 2048)

/* DATA STATES From DATA */
#define DATA_RDY 1
#define DATA_RDY_MAXTRANS 2
#define DATAERROR_NP 3

/*  Defines for max DATA, etc. moved to hostAcqStruct.h  */

/* ------------------- DATA Object Structure ------------------- */
typedef struct {
	char* 	pIdStr; 	/* user identifier string */
  unsigned long dspMemAddr;	/* The local translated DATA Memory Address */
  unsigned long dspMemSize;	/* size in bytes of DATA Memory */
  unsigned long	maxFreeList;    /* present max of existing Tag Free List */
  unsigned long	maxFidBlkBuffered; /* maximum FID blocks buffered */
FID_STAT_BLOCK  *pStatBlkArray; /* ptr to array of FID StatBlk Entries */
    RINGXBLK_ID pTagFreeList;	/* avialable Tags */
	SEM_ID  pSemStateChg;   /* Semaphore for state changes of DATA */
	SEM_ID  pStmMutex;   	/* Mutex Semaphore for DATA Object */
        int     dataState;
	int	dspAppVersion;
} DATA_OBJ;

typedef DATA_OBJ *DATAOBJ_ID;

/*  currentTag and currentNt work together to help the console calculate
    the current CT as an experiment proceeds.  See monitor.c		*/

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern DATAOBJ_ID dataCreate(int channel,char *idStr);
extern int dataInitial(DATAOBJ_ID pDataId, ulong_t totalFidBlks, ulong_t fidSize);
extern int dataAllocWillBlock(DATAOBJ_ID pDataId);
extern int dataPeekAtNextTag(DATAOBJ_ID pStmId, int *nxtTag);
extern FID_STAT_BLOCK* dataAllocAcqBlk(DATAOBJ_ID pDataId, ulong_t fid_element, ulong_t np, ulong_t strtCt, ulong_t endCt, ulong_t nt, unsigned long fidSize, long *Tag, unsigned long* vmeAddr);
extern int 		dataFreeFidBlk(DATAOBJ_ID pDataId,long index);
extern int 		dataGetState(DATAOBJ_ID stmid, int mode, int secounds);
extern FID_STAT_BLOCK *dataGetStatBlk(DATAOBJ_ID pStmId,long dataTag);
extern int 		dataDelete(DATAOBJ_ID pStmId);
extern void 		dataShow(DATAOBJ_ID stmid,int level);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

#ifdef __cplusplus
}
#endif

#endif
#endif
