/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCstmh
#define INCstmh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#define PART_OVRHD     16
#define ALLOC_OVERHEAD 8
#define BYTES_PER_NP   4

#define FREE 0
#define NO_DATA 1

/* --------- STM interupt vector numbers see vmeIntrp.h ------------ */

/* ------ IST VxWorks Task Priority & Stack Size  see taskPrior.h ---- */

/* ------------- interrupt Message struct ----------------- */
typedef struct {
		char* memaddr;
	        unsigned long count;
	       }  ITR_MSG;

/* ----------------- FID Block Element --------------------- */
typedef struct {
		int pageState; 		/* FREE,ACQ,Complete,Xfering */
		char*  	      elemAddr; /* STM address of FID Data */
		unsigned long elemId;	/* FID # */
                unsigned long startct;	/* logical starting CT of this FID Block */
		unsigned long ct;	/* Completed Transients of FID */
		unsigned long nt;	/* Completed Transients of FID */
		unsigned long np;	/* Completed Transients of FID */
		int acqState;		/* State of Data, READY, MAXTRIANS, ERROR */
} FIDBLK_ELEMENT;

typedef FIDBLK_ELEMENT FIDBLK_ENTRY;

/* ------------------- STM Object Structure ------------------- */
typedef struct {
	char *  stmBaseAddr;
	int	vmeItrVector;
	int	vmeItrLevel;
	int	stmBrdVersion;
	int 	stmState;	/* OK, DATA_RDY, DATA_RDY_MAXTRANS, DATAERROR_NP */
	int 	npOvrRun;   /* # data points under or over acquired for 
				     DATAERROR_NP error */
	char* 	pStmIdStr; 	/* user identifier string */
	char* 	pStmLocAdr; 	/* Local Addres of STM memory */
	long 	lastPage;  	/* Last Page Transfered to Host Computer */
  FIDBLK_ENTRY *pFidEntry;/* ptr to array of FID Entries */
      PART_ID   pStmMemPool; 	/* Pointer to the STMs memory map */
	char*	pmemPoolAdr;	/* used in memPartDestroy */
  unsigned long	memPoolSize;	/* used in memPartDestroy */
       MSG_Q_ID pMsgRdyLst; 	/* Msg Q of Ready STM memory addresses */
       MSG_Q_ID pMsgMaxLst; 	/* Msg Q of MaxTran STM memory addresses */
       MSG_Q_ID pMsgErrLst; 	/* Msg Q of Error STM memory addresses */
    RINGBLK_ID  pFidBlkFreeLst; /* Ring Buffer of free FidEntry */
       RING_ID  pRngRdyLst; 	/* Ring Buffer of Ready STM memory addresses*/
	HASH_ID pHashStmAddr;  /* hash table of address to FID_ELEM */
	SEM_ID  pSemStateChg;   /* Semiphore for state changes of STM */
	SEM_ID  pStmMutex;   	/* Mutex Semiphore for STM Object */
  unsigned long	maxFidBlkBuffered; /* maximum FID blocks buffered */
} STM_OBJ;

typedef STM_OBJ *STMOBJ_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)
extern STMOBJ_ID 	stmCreate(char* baseAddr, int vector, int level, char* idstr);
extern int 		stmInitial(STMOBJ_ID stmid, long np, long totalFidBlks, 
				   PFL hashFunc, PFI cmpFunc);
extern char* 		stmAllocAcqBlk(STMOBJ_ID stmid, long fid, 
				       long np, long strtCt, long nt);
extern int 		stmFreeFidBlk(STMOBJ_ID pStmId,char* memAdr);
extern int 		stmGetState(STMOBJ_ID stmid, int mode, int secounds);
extern int 		stmGetNxtFid(STMOBJ_ID stmid,FIDBLK_ENTRY *fidblk);
extern int 		stmFreeAllRes(STMOBJ_ID pStmId);
extern int 		stmDelete(STMOBJ_ID pStmId);
extern void 		stmShow(STMOBJ_ID stmid,int level);
#else
/* --------- NON-ANSI/C++ prototypes ------------  */
extern void initStmMap();
extern void stmCreate();
extern int getStmState();
#endif

#ifdef __cplusplus
}
#endif

#endif

