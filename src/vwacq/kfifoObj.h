/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* #define _POSIX_SOURCE /* defined when source is waited tobe POSIX-compliant */
/* #define _SYSV_SOURCE /* defined when source is System V */
/* #ifdef __STDC__ /* used to determine if using an ANSI compiler */


#ifndef INCfifoh
#define INCfifoh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#define FIFO_ALMOST_FULL 1
#define FIFO_EMPTY 0
#define FIFO_STOPPED -1
#define NO_VALUE 0
#define FIFO_ERROR -1
#define FIFO_UNDERFLOW -2
#define FIFO_START_ON_HALT -3
#define FIFO_START_ON_EMPTY -4
#define FIFO_WORD_BUF_SIZE   4096

/* see vmeIntrp.h for interrupt vector Numbers */

/* Fifo Status Modes */


typedef struct {
		char *  fifoBaseAddr;
		int	vmeItrVector;
		int	vmeItrLevel;
		int	vmeItrMask;
		int	fifoBrdVersion;
		int fifoState;	/* FIFO_ACTIVE, FIFO_STOPPED, FIFO_ERROR */
		int lastError;  /* FIFO_NP_ERROR, FIFO_UNDERFLOW, FIFO_START_ON_EMPTY
				   FIFO_START_ON_HALT */
		int apValue;	/* AP Buss read back value */
		RINGXBLK_ID pFifoWordBuf;
		SEM_ID  pSemFifoStateChg; /* Semiphore for state change of FIFO */
		SEM_ID  pSyncOk2Stuff;    /* Semiphore to block stuffing FIFO */
		SEM_ID  pFifoMutex;       /* Mutex Semiphore for FIFO Object */
		PFI	pStuffIt;	  /* ptr to stuffing function */
		PFI	pUserFunc;	  /* ptr to User Function */
		int	pUserArg;	  /* ptr to User Function Arg */
    		char*	pIdStr;	  	  /* Identifier String */
} FIFO_OBJ;

typedef FIFO_OBJ *FIFO_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern FIFO_ID fifoCreate(char* baseAddr, int vector, int level, char* idstr);
extern int getFifoState(FIFO_ID pFifoId, int mode, int secounds);
extern int fifoGetApValue(FIFO_ID pFifoId, int mode, int secounds);
extern int fifoStart(FIFO_ID pFifoId);
extern int fifoStartSync(FIFO_ID pFifoId, int select);
extern void fifoReset(FIFO_ID pFifoId, int options);
extern void fifoItrpEnable(FIFO_ID pFifoId, int mask);
extern void fifoItrpDisable(FIFO_ID pFifoId, int mask);
extern void fifoStuffIt(FIFO_ID pFifoId, long code);
extern void fifoHaltop(FIFO_ID pFifoId);
extern void fifoStuffCode(FIFO_ID pFifoId, long code);
extern int fifoStatReg(FIFO_ID pFifoId);
extern int fifoRunning(FIFO_ID pFifoId);
extern int fifoEmpty(FIFO_ID pFifoId);
extern int fifoPreAEmpty(FIFO_ID pFifoId);
extern int fifoPreAFull(FIFO_ID pFifoId);
void fifoWait4Stop(FIFO_ID pFifoId);
extern void fifoBlockStuff(void);
extern void fifoUnblockStuff(void);
extern int initFifoLog(int bufsize);
extern void fifoLogStart(void);
extern void fifoLogStop(void);
extern void fifoLogReset(void);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern int createFifoObj();
extern int fifoGetState();
extern int fifoGetApValue();
extern int fifoStart();
extern int fifoStartSync();
extern void fifoReset();
extern void fifoItrpEnable();
extern void fifoItrpDisable();
extern void fifoHaltop();
extern void fifoStuffCode();
extern void fifoBlockStuff();
extern void fifoUnblockStuff();
extern int initFifoLog();
extern void fifoLogStart();
extern void fifoLogStop();
extern void fifoLogReset();

#endif

#ifdef __cplusplus
}
#endif

#endif
