/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCnamebufsh
#define INCnamebufsh

#include <vxWorks.h>
#include <semLib.h>
#include "hostAcqStructs.h"
#include "fBufferLib.h"
#include "hashLib.h"

/* Buffer errno's i, vxWorks style of errno codes */
#define   S_namebufs_NAMED_BUFFER_PTR_NULL 	0x1001
#define   S_namebufs_NAMED_BUFFER_ALREADY_INUSE 0x1002
#define   S_namebufs_NAMED_BUFFER_MALLOC_FAILED 0x1003
#define   S_namebufs_NAMED_BUFFER_TIMED_OUT 	0x1004

/* BUFFER TYPE */
#define FIXED_FAST_BUF  1
#define DYNAMIC_BUF	2

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
   			FBUFFER_ID pDLBfBufId;
   			int dataBufNum;	  /* fix buffer number */
   			int dataBufSize;  /* fix buffer size */
			int findEntryPended;
			SEM_ID pDlbMutex;
			SEM_ID pDlbFindPendSem;
			SEM_ID pDlbMakePendSem;
		}  DLB_OBJECT;

typedef DLB_OBJECT *DLB_ID;

typedef struct _DownLinkBuffer_
		{
   			char id[32];
   			int size;
   			int status;
			DLB_ID  nambufptr;
   			unsigned long *data_array;
   			unsigned long *data_end;
		} DLB;

typedef DLB *DLBP;

typedef struct {
	unsigned long curqui;
	int np,
	    prec,
	    ct, nt,
	    bs,
	    /* these are abstracted from the ni... array */
	    c1, dim1, 
	    c2, dim2,
	    c3, dim3, 
	    c4, dim4, 
	    c5, dim5, 
	    cum_inc, total_incs,
	    rtphase, 
	    jumps[16],
	    vvar[NUM_RT_VARS];
} XP_CNTRS_OBJ;

extern DLB_ID  dlb_Pool;

/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)
 
extern DLB_ID 	dlbCreate(int buftype, int numbuf, int bufsize);
extern void 	dlbDelete(DLB_ID x);
extern DLBP 	dlbMakeEntry(DLB_ID Pool,char *tag,int size);
extern DLBP 	dlbMakeEntryP(DLB_ID Pool,char *tag,int size,int timeout);
extern DLBP 	dlbRename(DLB_ID dlbId, char* presentlabel,char* newlabel);
extern int 	dlbReuse(DLB_ID xx, int nBuffers, int bufSize);
extern int 	dlbFree(DLBP xx);
extern int 	dlbFreeByName(DLB_ID xx, char* label);
extern DLBP 	dlbFindEntry(DLB_ID xx, char *label);
extern DLBP 	dlbFindEntryP(DLB_ID xx, char *label, int timeout);
extern void 	dlbFreeAll(DLB_ID xx);
extern int  	dlbFreeBufs(DLB_ID x);
extern int	dlbUsedBufs (DLB_ID x);
extern long     dlbMaxSingleBuf (DLB_ID x);
extern void 	dlbSetReady(DLBP xx);
extern int 	dlbRelease(DLBP xx);
extern void    *dlbGetPntr(DLBP xx);
extern void 	dlbPrintBuffers(DLB_ID xx);
extern void 	dlbShow(DLB_ID xx,int level);

extern XP_CNTRS_OBJ *create_xp_counters();
extern int reset_xp_counters( XP_CNTRS_OBJ *arg,int np,int bs,int nt,int dim1, int dim2, int dim3, int dim4, int dim5);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern DLB_ID 	dlbCreate();
extern void 	dlbDelete();
extern DLBP 	dlbMakeEntry();
extern DLBP 	dlbMakeEntryP();
extern int 	dlbRename();
extern int 	dlbReuse();
extern int 	dlbFree();
extern int 	dlbFreeByName();
extern DLBP 	dlbFindEntry();
extern DLBP 	dlbFindEntryP();
extern void 	dlbFreeAll();
extern int  	dlbFreeBufs();
extern int	dlbUsedBufs ();
extern long     dlbMaxSingleBuf ();
extern void 	dlbSetReady();
extern int 	dlbRelease();
extern void    *dlbGetPntr();
extern void 	dlbPrintBuffers();
extern void 	dlbShow();

extern XP_CNTRS_OBJ *create_xp_counters();
extern int reset_xp_counters();

#endif
 
#ifdef __cplusplus
}
#endif

#endif
