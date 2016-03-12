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
#ifndef INCstmh
#define INCstmh

#include <msgQLib.h>
#include "hostAcqStructs.h"
#include "rngXBlkLib.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*  Do NOT define NO_TAG to be -1, for the STM equipment
    uses this value at abort, halt and stop experiment.  */

#define NO_TAG	-2

#define NO_DATA 1
#define MAX_BUFFER_ALLOCATION (1024 * 32)

/* DATA STATES From STM */
#define DATA_RDY 1
#define DATA_RDY_MAXTRANS 2
#define DATAERROR_NP 3

/*  Defines for max STMs, etc. moved to hostAcqStruct.h  */

/* --------- STM interupt vector numbers see vmeIntrp.h ------------ */

/* ------------------- STM Object Structure ------------------- */
typedef struct {
  unsigned long stmBaseAddr;	/* The local translated STM Base Address */
  unsigned long stmMemAddr;	/* The local translated STM Memory Address */
  unsigned long stmMemSize;	/* size in bytes of STM Memory */
	int     stmApBusAddr;	/* The STM Apbus Address */
	int	vmeItrVector;
	int	vmeItrLevel;
	int	stmCntrlMask;	/* control register setting, can't read it */
	int	stmBrdVersion;
	int 	stmState;    /* OK, DATA_RDY, DATA_RDY_MAXTRANS, DATAERROR_NP */
	int     adcOvldFlag;	/* HS DTM Adc OverFlow Flag (a knock-off of the ADC's flag) */
	int 	npOvrRun;    /* # data points under or over acquired for 
				     DATAERROR_NP error */
        int	activeIndex;	/* n'th active rcvr channel */
        int	bsPending;	/* Flag storage for FIFO stuffer */
        int	nSkippedAcqs;	/* Info storage for FIFO stuffer */
	char* 	pStmIdStr; 	/* user identifier string */
	char* 	pSID; 		/* SCCS ID string */
  unsigned long	maxFreeList; /* present max of existing Tag Free List */
  unsigned long	maxFidBlkBuffered; /* maximum FID blocks buffered */
	long 	lastPage;  	/* Last Page Transfered to Host Computer */
	long	currentNt;	/* NT for the current element */
FID_STAT_BLOCK  *pStatBlkArray;/* ptr to array of FID StatBlk Entries */
    RINGXBLK_ID pTagFreeList;	/* avialable Tags */
       MSG_Q_ID pIntrpMsgs; 	/* Msg Q of Ready STM memory addresses */
       MSG_Q_ID pIntrpDefaultMsgs; /* Msg Q assign in stmInit, so that it can be restored */
	SEM_ID  pSemStateChg;   /* Semaphore for state changes of STM */
	SEM_ID  pStmMutex;   	/* Mutex Semaphore for STM Object */
        short	currentTag;	/* Used to track the start of each element */
	unsigned long cur_scan_data_adr;
	unsigned long prev_scan_data_adr;
	long	initial_scan_num;
	long	tag2snd;
        void	*adcHandle;	/* Which ADC this STM is connected to */
	int	inputSwitchFlag; /* for 1MHz STM/ADC Liquids or Wideline signal inputs */
} STM_OBJ;

typedef STM_OBJ *STMOBJ_ID;

/*  currentTag and currentNt work together to help the console calculate
    the current CT as an experiment proceeds.  See monitor.c		*/

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)
extern STMOBJ_ID stmCreate(unsigned long baseAddr,unsigned long memAddr, int apBusAddr, int vector, int level, char* idstr);
extern int stmInitial(STMOBJ_ID pStmId, ulong_t totalFidBlks, ulong_t fidSize, MSG_Q_ID upLnkMsgQ, int activeIndex);
extern int stmAllocWillBlock(STMOBJ_ID pStmId);
extern FID_STAT_BLOCK* stmAllocAcqBlk(STMOBJ_ID pStmId, ulong_t fid_element, ulong_t np, ulong_t strtCt, ulong_t endCt, ulong_t nt, ulong_t fidSize, long *Tag, ulong_t* vmeAddr);
extern int 		stmFreeFidBlk(STMOBJ_ID pStmId,int index);
extern int 		stmGetState(STMOBJ_ID stmid, int mode, int secounds);
extern int 		stmGetNxtFid(STMOBJ_ID pStmId,ITR_MSG *itrmsge, FID_STAT_BLOCK* *pStatBlk,long* *dataAddr,int *stat);
extern int 	      stmHaltCode(STMOBJ_ID pStmId,int Donecode, int Errorcode);
extern MSG_Q_ID 	stmChgMsgQ(STMOBJ_ID pStmId, MSG_Q_ID upLnkMsgQ );
extern int 		stmRestoreMsgQ(STMOBJ_ID pStmId);
extern int 		stmSA(STMOBJ_ID pStmId);
extern int 		stmAA(STMOBJ_ID pStmId);
extern short 		stmStatReg(STMOBJ_ID pStmId);
extern long 		stmNtCntReg(STMOBJ_ID pStmId);
extern long 		stmNpCntReg(STMOBJ_ID pStmId);
extern long 		stmSrcAddrReg(STMOBJ_ID pStmId);
extern long 		stmDstAddrReg(STMOBJ_ID pStmId);
extern long 		stmMaxSumReg(STMOBJ_ID pStmId);
extern short 		stmTagReg(STMOBJ_ID pStmId);
extern long 	       *stmTag2DataAddr(STMOBJ_ID pStmId,short tag);
extern FID_STAT_BLOCK  *stmTag2StatBlk( STMOBJ_ID pStmId, short tag );
extern void 		stmReset(STMOBJ_ID pStmId);
extern void 		stmItrpDisable(STMOBJ_ID pStmId, int mask);
extern void 		stmItrpEnable(STMOBJ_ID pStmId, int mask);
extern int 		stmGenCntrlCodes(STMOBJ_ID pStmId, unsigned long cntrl, unsigned long *pCodes);
extern int 		stmGenTagCodes(STMOBJ_ID pStmId, unsigned long tag, unsigned long *pCodes);
extern int 		stmGenSrcAdrCodes(STMOBJ_ID pStmId, unsigned long srcAdr, unsigned long *pCodes);
extern int 		stmGenDstAdrCodes(STMOBJ_ID pStmId, unsigned long dstAdr, unsigned long *pCodes);
extern int 		stmGenNpCntCodes(STMOBJ_ID pStmId, unsigned long count, unsigned long *pCodes);
extern int 		stmGenNtCntCodes(STMOBJ_ID pStmId, unsigned long count, unsigned long *pCodes);
extern int 		stmGenMaxSumCodes(STMOBJ_ID pStmId, unsigned long maxsum, unsigned long *pCodes);
extern int 		stmFreeAllRes(STMOBJ_ID pStmId);
extern int 		stmDelete(STMOBJ_ID pStmId);
extern void 		stmShow(STMOBJ_ID stmid,int level);
extern int		instantiateSTMs(void);
extern void 		stmAdcOvldClear(STMOBJ_ID pStmId);
extern int 		stmGetHsStmIdx();
extern int 		stmGetHsStmObj(STMOBJ_ID *pStmId);
extern STMOBJ_ID	stmGetSelectedHsStmObj();
extern STMOBJ_ID 	stmGetStdStmObj();
extern int 		stmIsHsStmObj(STMOBJ_ID pStmId);
extern void		stmUseThisOne(STMOBJ_ID pStmId);
extern STMOBJ_ID	stmGetStmObjByIndex( int index );
extern STMOBJ_ID	stmGetNextStmObj(int *index);
extern STMOBJ_ID	stmGetActive(ulong_t *activeRcvrs, int *pindex);
extern void		stmSetNActive(int n);
extern int		stmGetNActive(void);
extern void		stmSetAdcPointers(void);
extern void* 		stmGetAttachedADC(STMOBJ_ID pStmId);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern STMOBJ_ID stmCreate();
extern int stmInitial();
extern int stmAllocWillBlock();
extern FID_STAT_BLOCK* stmAllocAcqBlk();
extern int 		stmFreeFidBlk();
extern int 		stmGetState();
extern int 		stmGetNxtFid();
extern int 	        stmHaltCode();
extern MSG_Q_ID 	stmChgMsgQ();
extern int 		stmRestoreMsgQ();
extern int 		stmSA();
extern int 		stmAA();
extern short 		stmStatReg();
extern long 		stmNtCntReg();
extern long 		stmNpCntReg();
extern long 		stmSrcAddrReg();
extern long 		stmDstAddrReg();
extern long 		stmMaxSumReg();
extern short 		stmTagReg();
extern long 	       *stmTag2DataAddr();
extern FID_STAT_BLOCK  *stmTag2StatBlk();
extern void 		stmReset();
extern void 		stmItrpDisable();
extern void 		stmItrpEnable();
extern int 		stmGenCntrlCodes();
extern int 		stmGenTagCodes();
extern int 		stmGenSrcAdrCodes();
extern int 		stmGenDstAdrCodes();
extern int 		stmGenNpCntCodes();
extern int 		stmGenNtCntCodes();
extern int 		stmFreeAllRes();
extern int 		stmDelete();
extern void 		stmShow();
extern int		instantiateSTMs();
extern void 		stmAdcOvldClear();
extern int 		stmGetHsStmIdx();
extern int 		stmGetHsStmObj();
extern STMOBJ_ID 	stmGetStdStmObj();
extern int 		stmIsHsStmObj();
extern void		stmUseThisOne();
extern STMOBJ_ID	stmGetStmObjByIndex();
extern STMOBJ_ID	stmGetNextStmObj();
extern STMOBJ_ID	stmGetActive();
extern void		stmSetNActive();
extern int		stmGetNActive();
extern void		stmSetAdcPointers();
extern void* 		stmGetAttachedADC();
#endif

#ifdef __cplusplus
}
#endif

#endif

