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

#define FIFOBUFOBJ

#include "commondefs.h"

#include "fifoBufObj.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* ----------- FIFO Status ---------- */
#define FIFO_ERROR -1
#define FIFO_ALMOST_FULL 1
#define FIFO_EMPTY 0
#define FIFO_STOPPED -1

/* ----------- FIFO Error Codes, start at 20 so that it can be append to those
	 codes in errorcodes.h in vnmr
*/
#define FIFO_UNDERFLOW 		20
#define FIFO_START_ON_HALT 	21
#define FIFO_START_ON_EMPTY 	22
#define FIFO_NETBL 		23
#define FIFO_FORP 		25
#define FIFO_NETBAP 		26

#define FIFO_WORD_BUF_SIZE   8192

#define FIFO_SW_INTRP1		2048
#define FIFO_SW_INTRP2		1024
#define FIFO_SW_INTRP3		512
#define FIFO_SW_INTRP4		256

#define STD_HS_LINES 0
#define EXT_HS_LINES 1

/* see vmeIntrp.h for interrupt vector Numbers */

/* Fifo Object State */


typedef struct {
		unsigned long fifoBaseAddr;
		int	vmeItrVector;
		int	vmeItrLevel;
		short   timeToBlock; /* Flag to indicate when 
				        to Block Stuffing FIFO */
                short   optionsPresent; /* extend HSlines, rotorsync */
		int	fifoBrdVersion;
       unsigned long	HSLines;	/* Standard High Speed Lines */
       unsigned long	HSLinesExt;     /* Extended High Speed Lines */
       unsigned long	SafeHSLines;	/* Safe Standard High Speed Lines */
       unsigned long	SafeHSLinesExt; /* Safe Extended High Speed Lines */
		int fifoState;	/* FIFO_ACTIVE, FIFO_STOPPED, FIFO_ERROR */
		int lastError;  /* FIFO_NP_ERROR, FIFO_UNDERFLOW, FIFO_START_ON_EMPTY
				   FIFO_START_ON_HALT */
		int 	NoStartFlag;	  /* When set do not allow fifo to be started */
		int 	NoStuffFlag;	  /* When set Stuffer task suspends it's self, waiting to be restart (phandler) */
                int	fifoStarted4Exp;  /* The Initial Start of the FIFO (via parseri, stuffer,etc.) 4 the Exp. Has Happened */
		FIFOBUF_ID  pFifoWordBufs;
		SEM_ID  pSemFifoStateChg; /* Semaphore for state change of FIFO */
		SEM_ID  pSyncOk2Stuff;    /* Semaphore to block stuffing FIFO */
		SEM_ID  pFifoMutex;       /* Mutex Semaphore for FIFO Object */
		SEM_ID pSemAMMT;	  /* Almost Empty Semaphore */
		SEM_ID pSemFifoStop;	  /* Interrupt given Semaphore */
		SEM_ID pSemFifoError;     /* Interrupt given Semaphore */
		SEM_ID pSemApBus; 	/* Interrupt given Semaphore */
		SEM_ID pSemFifoSW;  	  /* Interrupt given Semaphore */
	        /* MSG_Q_ID pTagMsgQ;	  /* MsgQ for Tags */
	        /* MSG_Q_ID pApMsgQ;	  /* MsgQ for AP ReadBack */
		PFI	pUserFunc;	  /* ptr to User Function */
		int	pUserArg;	  /* ptr to User Function Arg */
		int	fifoFd;		  /* File Descriptor for fifo output */
		int	fifoLoopFlag;	  /* Flag for stuffing HW loop codes */
		int	fifoLoopCnt;	  /* Hardware loop count */
		int	tAMMTid;  	  /* AMMT task id */
		int	tSTOPid;  	  /* FIFO stopped task id */
		int	tAPid;  	  /* AP task id */
		int	tSTUFid;  	  /* Stuffer task id */
		int	tERRid;  	  /* Error task id */
       unsigned long 	hwlFifoWords[3];  /* Store for hardware looping */
    		char*	pIdStr;	  	  /* Identifier String */
    		char*	pSID;	  	  /* SCCS Identifier String */
	        void*   pPChanObj;	  /* Parallel Channel Object ID */
} FIFO_OBJ;

typedef FIFO_OBJ *FIFO_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern FIFO_ID fifoCreate(unsigned long baseAddr, int vector, int level, char* idstr);
extern long  fifoApReadBk(FIFO_ID pFifoId);
extern void  fifoBlockStuff(FIFO_ID pFifoId);
extern int   fifoGetState(FIFO_ID pFifoId, int mode, int secounds);
extern int   fifoGetApValue(FIFO_ID pFifoId, int mode, int secounds);
extern int   fifoEmpty(FIFO_ID pFifoId);
extern void  fifoFlushBuf(FIFO_ID pFifoId);
extern void  fifoClrHsl(FIFO_ID pFifoId,int whichone);
extern long  fifoDiagTimer(FIFO_ID pFifoId);
extern int   fifoGetHsl( FIFO_ID pFifoId, int whichone );
extern void  fifoLastWord(FIFO_ID pFifoId,long *pValues);
extern int   fifoReadWord(FIFO_ID pFifoId,long *pValues);
extern void  fifoLoadHsl(FIFO_ID pFifoId,int whichone, unsigned long value);
extern void  fifoMaskHsl(FIFO_ID pFifoId,int whichone, unsigned long bitsToTurnOn);
extern void  fifoUnMaskHsl(FIFO_ID pFifoId,int whichone, unsigned long BitsToClear);
extern void  fifoSafeHsl(FIFO_ID pFifoId,int whichone, unsigned long value);
extern void  fifoHaltop(FIFO_ID pFifoId);
extern short fifoIntrpMask(FIFO_ID pFifoId);
extern void  fifoItrpEnable(FIFO_ID pFifoId, int mask);
extern void  fifoItrpDisable(FIFO_ID pFifoId, int mask);
extern int   fifoPreAEmpty(FIFO_ID pFifoId);
extern int   fifoPreAFull(FIFO_ID pFifoId);
extern void  fifoPrtStatus(FIFO_ID pFifoId);
extern void  fifoPrtImask(FIFO_ID pFifoId);
extern void  fifoSetNoStart(FIFO_ID pFifoId);
extern void  fifoClrNoStart(FIFO_ID pFifoId);
extern int   fifoExternGate(FIFO_ID pFifoId,int count);
extern int   fifoRotorSync(FIFO_ID pFifoId, int count);
extern int   fifoRotorRead(FIFO_ID pFifoId, long *value);
extern void  fifoSetNoStuff(FIFO_ID pFifoId);
extern int   fifoStart(FIFO_ID pFifoId);
extern int   fifoStartSync(FIFO_ID pFifoId);
extern void  fifoStuffIt(FIFO_ID pFifoId, long *pCodes, int num);
extern void  fifoReset(FIFO_ID pFifoId, int options);
extern void  fifoResetStuffing(FIFO_ID pFifoId);
extern int   fifoRunning(FIFO_ID pFifoId);
extern long  fifoStatReg(FIFO_ID pFifoId);
extern void  fifoStuffCmd(FIFO_ID pFifoId, unsigned long cntrlBits, unsigned long DataField);
extern void  fifoStuffCode(FIFO_ID pFifoId, long *pCodes, int num);
extern  int  fifoSwIsrREG(FIFO_ID pFifoId, int sw_itrp, PFI pISR, int isrArg);
extern long  fifoTagValue(FIFO_ID pFifoId);
extern void  fifoWait4Stop(FIFO_ID pFifoId);
extern void  fifoUnblockStuff(FIFO_ID pFifoId);
extern int fifoInitLog(FIFO_ID pFifoId);
extern void  fifoBeginHardLoop(FIFO_ID pFifoId);
extern void  fifoEndHardLoop(FIFO_ID pFifoId);
extern void  fifoLogStart(void);
extern void  fifoLogStop(void);
extern void  fifoLogReset(void);
extern int   fifoStufferAA(FIFO_ID pFifoId);
extern unsigned long fifoGetOpReg( FIFO_ID pFifoId );
extern void fifoStart4Exp(FIFO_ID pFifoId);
extern void fifoClrStart4Exp(FIFO_ID pFifoId);
extern int fifoGetStart4Exp(FIFO_ID pFifoId);
extern void fifoSetStart4Exp(FIFO_ID pFifoId,int state);
#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern int createFifoObj();
extern int fifoGetState();
extern int fifoGetApValue();
extern void  fifoSetNoStart();
extern void  fifoClrNoStart();
extern void  fifoSetNoStuff();
extern int fifoStart();
extern int fifoStartSync();
extern void fifoReset();
extern void fifoItrpEnable();
extern void fifoItrpDisable();
extern short fifoIntrpMask();
extern long fifoDiagTimer();
extern long fifoApReadBk();
extern long fifoTagValue();
extern int  fifoGetHsl();
extern void  fifoSafeHsl();
extern void fifoLastWord();
extern int   fifoReadWord();
extern void fifoPrtStatus();
extern void fifoPrtImask();
extern void fifoHaltop();
extern void fifoStuffCode();
extern int   fifoExternGate();
extern int   fifoRotorSync();
extern int   fifoRotorRead();
extern void fifoBlockStuff();
extern void fifoUnblockStuff();
extern int initFifoLog();
extern void  fifoBeginHardLoop();
extern void  fifoEndHardLoop();
extern void fifoLogStart();
extern void fifoLogStop();
extern void fifoLogReset();
extern int   fifoStufferAA();
extern unsigned long fifoGetOpReg();
extern void fifoStart4Exp();
extern void fifoClrStart4Exp();
extern int fifoGetStart4Exp();
extern void fifoSetStart4Exp();

#endif

#ifdef __cplusplus
}
#endif

#endif
