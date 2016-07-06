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
#ifndef INCnameClBufsh
#define INCnameClBufsh

#include <vxWorks.h>
#include <semLib.h>
#include "hostAcqStructs.h"
#include "fBufferLib.h"
#include "hashLib.h"
#include "mBufferLib.h"

/* Buffer errno's i, vxWorks style of errno codes */
#define   S_namebufs_NAMED_BUFFER_PTR_NULL 	0x1001
#define   S_namebufs_NAMED_BUFFER_ALREADY_INUSE 0x1002
#define   S_namebufs_NAMED_BUFFER_MALLOC_FAILED 0x1003
#define   S_namebufs_NAMED_BUFFER_TIMED_OUT 	0x1004

/* DOWN LOAD */

/* buffer status */
#define MT	(1)
#define READY   (2)
#define DONE	(4)
#define LOADING	(8)
#define PARSING (16)

/* UP LINK */

#define NUM_AHEAD		(100)

/* UP LINK EXCEPTION STATE  */

#define U_CLEAR 0		/* pend for input */
#define U_DONE	1		/* data is good */
#define U_ABORT 2		/* discard data */


#define NUM_RT_VARS 16

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


typedef struct _DownLink_
		{
   			int number;
   			HASH_ID pNamTbl;
                        MBUFFER_ID pMBufferId;
   			FBUFFER_ID pDLBfBufId;
			int findEntryPended;
			SEM_ID pDlbMutex;
			SEM_ID pDlbFindPendSem;
			SEM_ID pDlbMakePendSem;
			SEM_ID pDlbFreeBufsPendSem;
		}  NCLB_OBJECT;

typedef NCLB_OBJECT *NCLB_ID;

typedef struct _DownLinkBuffer_
		{
   			char id[32];
   			int size;
   			int status;
			NCLB_ID  nambufptr;
                        int bufExtern;
   			unsigned long *data_array;
   			unsigned long *data_end;
		} NCLB;

typedef NCLB *NCLBP;

extern NCLB_ID  clb_Pool;

/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)
 
extern NCLB_ID nClbCreate(MBUFFER_ID mBufferId, int numbuf);
extern void 	nClbDelete(NCLB_ID x);
extern NCLBP nClbMakeEntry(NCLB_ID Pool,char *tag,int size, char *bufAddr);
extern NCLBP nClbMakeEntryP(NCLB_ID Pool,char *tag,int size,char *bufAddr, int timeout);
extern NCLBP 	nClbRename(NCLB_ID dlbId, char* presentlabel,char* newlabel);
extern int 	nClbReuse(NCLB_ID xx, int nBuffers, int bufSize);
extern int 	nClbFree(NCLBP xx);
extern int 	nClbFreeByName(NCLB_ID xx, char* label);
extern int      nClbFreeByRootName(NCLB_ID xx,char *rootLabel);
extern NCLBP 	nClbFindEntry(NCLB_ID xx, char *label);
extern NCLBP 	nClbFindEntryP(NCLB_ID xx, char *label, int timeout);
extern void 	nClbFreeAll(NCLB_ID xx);
extern int  	nClbFreeBufs(NCLB_ID x);
extern int	nClbUsedBufs (NCLB_ID x);
extern long     nClbMaxSingleBuf (NCLB_ID x);
extern void 	nClbSetReady(NCLBP xx);
extern int 	nClbRelease(NCLBP xx);
extern void    *nClbGetPntr(NCLBP xx);
extern void 	nClbPrintBuffers(NCLB_ID xx);
extern void 	nClbShow(NCLB_ID xx,int level);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern NCLB_ID 	nClbCreate();
extern void 	nClbDelete();
extern NCLBP 	nClbMakeEntry();
extern NCLBP 	nClbMakeEntryP();
extern int 	nClbRename();
extern int 	nClbReuse();
extern int 	nClbFree();
extern int 	nClbFreeByName();
extern NCLBP 	nClbFindEntry();
extern NCLBP 	nClbFindEntryP();
extern void 	nClbFreeAll();
extern int  	nClbFreeBufs();
extern int	nClbUsedBufs ();
extern long     nClbMaxSingleBuf ();
extern void 	nClbSetReady();
extern int 	nClbRelease();
extern void    *nClbGetPntr();
extern void 	nClbPrintBuffers();
extern void 	nClbShow();

#endif
 
#ifdef __cplusplus
}
#endif

#endif
