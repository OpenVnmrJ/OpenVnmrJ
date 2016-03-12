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
#ifndef INCcntlrstatesh
#define INCcntlrstatesh

#include <semLib.h>

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
 * WARNING....
 * BESURE to add ANY NEW States to the cntlrStateStr in the
 *   the C file.
 *
 *  DON't FORGET to Increase MAX_CNTLR_STATES   
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* ---------- State defines ------------*/
#define NOT_READY  0
#define CNTLR_NOT_READY  0
#define CNTLR_READYnIDLE 1
#define CNTLR_READY4SYNC 2
#define CNTLR_EXCPT_CMPLT 3
#define CNTLR_EXP_CMPLT 4
#define CNTLR_SU_CMPLT 5
#define CNTLR_FLASH_UPDATE_CMPLT 6
#define CNTLR_FLASH_UPDATE_FAIL 7
#define CNTLR_FLASH_COMMIT_CMPLT 8
#define CNTLR_FLASH_COMMIT_FAIL 9
#define CNTLR_TUNE_ACTION_CMPLT 10
#define CNTLR_EXCPT_INITIATED  11

#define MAX_CNTLR_STATES 12

#define MAX_CNTLR_PERTYPE 64   /* i.e. 64 rf and/or 64 ddr, etc. */

/* Used States */
#define CNTLR_INUSE_STATE   1
#define CNTLR_UNUSED_STATE 2

#define RF_CONFIG_UNSET -1
#define RF_CONFIG_STD  0
#define RF_CONFIG_ICAT 1

#define MASTER_CONFIG_STD 0

#define PFG_CONFIG_UNSET -1
#define PFG_CONFIG_STD 0
#define PFG_CONFIG_LOCK 1

/* sysFlags Object State */

typedef struct {
                 char cntlrID[16];
                 int state;
                 int errorcode;
                 int usedInPS;  /* controller (rf2,pfg1,etc.)  use in Pulse Sequence */
                 int configInfo;   /* any special configurational information */
                 long long fifoTicks;
               } CntlrState;

typedef struct {
                SEM_ID  pSemStateChg; /* Semaphore for state change of FIFO */
                SEM_ID  pSemMutex;       /* Mutex Semaphore for Cntroller State Object */
                int     TotalCntlrs;  /* master1,rf1,ddr1, etc. */
                int     totalTypes;   /* total number of types */
                int     totalOfType[8];  /* total of each type */
                CntlrState CntlrInfo[8][MAX_CNTLR_PERTYPE+1];      // start at one not zero thus need +1
                int	stateFlags;	 /* state Flag Bits */
                int     cntlrsReponded; /* # of controllers that have reported back after initial action that expects reponses */
} CNTRLSTATE_OBJ;

typedef CNTRLSTATE_OBJ *CNTRLSTATE_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern CNTRLSTATE_ID  cntrlStatesCreate();
// extern int cntlrStatesAdd(char *id,int state);
extern int cntlrStatesAdd(char *id,int state,int ConfigInfo);
extern int cntrlStatesDelete(CNTRLSTATE_ID pSysFlags);
/* extern int  cntlrSetState(char *cntrlID,int state); */
extern int  cntlrSetState(char *cntrlId,int state,int errorcode);
extern int  cntlrSetFifoTicks(char *cntrlId,long long ticks);
extern int  cntlrSetStateAll(int state);
extern int cntlrStatesCmp(int state, int timeout);
extern void InitCntlrStates(void);
extern void cntlrStatesRemoveAll(void);
extern int wait4CntlrsReady(void);
extern int cntlrPresentsVerify(char *cntlrList, char *missingList);
extern int cntlrSet2UsedInPS(char *cntlrList, char *missingList);
extern char *getCntlrStateStr(int state);
extern char *getCntlrUseStateStr(int state);
extern int cntlrPresentGet(char *cntlrList);
extern int cntlrConfigGet(char *cntlrType, int configType, char *cntlrList);
extern int cntlrStatesCmpList(int cmpstate, int timeout, char *notReadyList);
extern int cntlrStatesCmpListUntilAllCtlrsRespond(int cmpstate, int timeout, char *notReadyList);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */


#endif

#ifdef __cplusplus
}
#endif

#endif
