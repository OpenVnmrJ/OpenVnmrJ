/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCsysflagsh
#define INCsysflagsh

#include <semLib.h>
#include "commondefs.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* ----------- Task Defines ---------- */
#define DOWNLINKER_FLAGBIT 1
#define APARSER_FLAGBIT    2
#define UPLINKER_FLAGBIT   4
#define PHANDLER_FLAGBIT   8

/* sysFlags Object State */


typedef struct {
		SEM_ID  pSemFlagsChg; /* Semaphore for state change of FIFO */
		SEM_ID  pSemMutex;       /* Mutex Semaphore for FIFO Object */
		int	stateFlags;	 /* state Flag Bits */
    		char*	pIdStr;	  	  /* Identifier String */
    		char*	pSID;	  	  /* SCCS Identifier String */
} SYSFLAGS_OBJ;

typedef SYSFLAGS_OBJ *SYSFLAGS_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern SYSFLAGS_ID  sysFlagsCreate(char* idstr);
extern int 	sysFlagsDelete(SYSFLAGS_ID pSysFlags);
extern int	sysFlagsMaskBit(SYSFLAGS_ID pTheSysFlags,int task);
extern int   	sysFlagsClearBit(SYSFLAGS_ID pTheSysFlags,int task);
extern int   	sysFlagsClearAll(SYSFLAGS_ID pTheSysFlags);
extern int 	sysFlagsCmpMask(SYSFLAGS_ID pSysFlags, int mask, int timeout);
extern int	InitSystemFlags(void);
extern int	wait4SystemReady(void);
extern int	wait4DownLinkerReady(void);
extern int	wait4ParserReady(void);
extern int	phandlerBusy(void);
extern int	downLinkerBusy(void);
extern int	parserBusy(void);
extern int	markReady(int task);
extern int	markBusy(int task);
extern int	sysShow(void);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */


#endif

#ifdef __cplusplus
}
#endif

#endif
