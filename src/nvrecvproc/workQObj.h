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

#ifndef INCworkQObjh
#define INCworkQObjh

#include "fileObj.h"
#include "mfileObj.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#include "hostAcqStructs.h"
#include "data.h"

/* HIDDEN */

/* typedefs */

#define FALSE 0
#define TRUE 1

#define ERRSTATBLK 1
#define WRNSTATBLK 2
#define FIDSTATBLK 4
#define SU_STOPSTATBLK 8

typedef struct _mfileWapper_ {
     FILE_ID  pFile;		/* regular File IO usage */
     MFILE_ID  pMapFile;	/* MMAP File IO usage */
     pthread_mutex_t   *pMutex;   /* access protection in a threaded environment */
} MFILE_WRAPPER, *MFILE_ID_WRAPPER;


/* The Sum-To-Memory Function for summing the data on the Host Computer */
typedef int (*PSTMF)(void* dstadr, void* srdadr, unsigned long np);

#define USE_MALLOC_BUFS

/*************************************************************/
typedef struct _Work_Q_Invarients {
       long  		NumFids;  /* NF */
       int  		rcvrPos;	/* the positional order of this multiple recvr FID with the FID file */
       int  		numActiveRcvrs; /* the positional order of this multiple recvr FID with the FID file */
       RINGBLK_ID 	pFidBufIndices; /* block ring buffer of free data buffers indices, for buffering fid data  */
       MFILE_ID_WRAPPER indatafile;
       MFILE_ID_WRAPPER fiddatafile;
       PSTMF            pStmFunc;
       void*            pRcvrDesc;
   } WORKQ_INVARIENT, *WORKQINVARIENT_ID;

/*************************************************************/

typedef struct _Work_Q_Entry {
       struct _workqobj_ 	*pWorkQObj; 
       struct _Work_Q_Invarients *pInvar;
       int 			statBlkType;		/* FID, Warning, Error */
       unsigned long  		statBlkCRC;
       FID_STAT_BLOCK* 		pFidStatBlk;
       unsigned long  		dataCRC;
       int      		bufferIndex;
       long 			cf;		/* current fid trace */
       long 			trueElemId;
       char 			*FidStrtAddr; /* used only by NDDS Data_Upload.c  Custom Deserializer */
       char 			*pFidData;	 /* data buffer */
       long long		RW_FileOffset;   /* offset into file, used for regular file I/O */
   } WORKQ_ENTRY;

/*************************************************************/

typedef WORKQ_ENTRY  *WORKQ_ENTRY_ID;
       

typedef struct _workqobj_   /* FID Stat Block Buffer Object */
    {
       int 		numWorkQs;		/* this can dynamicly change  */
       RINGBLK_ID 	pBlkRng;	/* block ring buffer of addresses */
       int 		maxWorkQs;		/* maximum allowable, i.e. size of ring buffer */
       long long        maxFidDataMemUsage; /* maximum memory allowed to be used for data buffers in bytes */
       WORKQ_ENTRY_ID   pWrkQBuffs;
       FID_STAT_BLOCK*  pFidStatBufs;
       char 		*pFidDataBufs;
       unsigned long 	FidDataBufSize;
       struct _Work_Q_Invarients *pInvar;
       char             *pFidSummingBuf;  /* for IL & RA Fid summing when using regular File IO rather than MMAP */
    } WORKQ_OBJECT;


typedef WORKQ_OBJECT  *WORKQ_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

 
extern WORKQ_ID workQCreate(void *pWorkDesc, int maxWorkQentries);
extern int setMaxWorkQMemoryUsage(WORKQ_ID pWorkQ, long long maxMemorySize);
extern int workQDataBufsInit(WORKQ_ID pWorkQ, MFILE_ID_WRAPPER fiddata,  long fidSizeBytes, long nf, int ddrPos, int numActiveDDRs,PSTMF stmFunc);
extern WORKQ_ENTRY_ID workQGet(WORKQ_ID pWorkQ);
extern int workQReset(WORKQ_ID pWorkQ);
extern int workQFree(WORKQ_ID pWorkQ, WORKQ_ENTRY_ID workQEntry);
extern char *getWorkQNewFidBufferPtr(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry);
extern char *getWorkQFidBufferPtr(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry);
extern char *getWorkQFidBlkHdrPtr(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry);
extern char *getWorkQFidPtr(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry);
extern int writeWorkQFidHeader2File(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry, void *pHeader, int bytelen);
extern int readWorkQFidHeaderFromFile(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry, void *pHeader, int bytelen);
extern int writeWorkQFid2File(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry, int bytelen);
extern int readWorkQFidFromFile(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry, char* pFidBUffer, int bytelen);
extern int sumWorkQFidData(WORKQ_ID pWorkQ,WORKQ_ENTRY_ID pWorkQEntry,int bytelen,int np);
 
#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
