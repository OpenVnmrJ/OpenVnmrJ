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
/* #define _POSIX_SOURCE /* defined when source is waited tobe POSIX-compliant */
/* #define _SYSV_SOURCE /* defined when source is System V */
/* #ifdef __STDC__ /* used to determine if using an ANSI compiler */


#ifndef INCaparserh
#define INCaparserh

#include <wdLib.h>
#include <semLib.h>
#include "lc.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* Table Header Structure */
/* The entry element the first in a list. There are four for 	*/
/* longword alignment.						*/

extern Acqparams acqReferenceData;
extern autodata *autoDP;

typedef struct {
	int	num_entries;
	int	size_entry;
	int	mod_factor;
	char	entry[4];
} TBL_OBJ;

typedef TBL_OBJ *TBL_ID;

#define EXP_BASE_NAME_SIZE  32

typedef struct {
        int parser_cmd;
	unsigned long NumAcodes;
	unsigned long NumTables;
	unsigned long startFID;
        int      spare1;
	char AcqBaseBufName[EXP_BASE_NAME_SIZE];
	} PARSER_MSG;
	

/* Experiment Data Structure */
/* The table_ptr is the first entry in a list. */


typedef struct {
   	char		id[EXP_BASE_NAME_SIZE];
	int		interactive_flag;
	int		num_acode_sets;
	int		cur_acode_set;
	unsigned int	*cur_acode_base;
	unsigned int	*cur_jump_base;
	int		cur_acode_size;
        Acqparams       *pLcStruct;
        autodata        *pAutodataStruct;
	unsigned int	*cur_rtvar_base;
	int		cur_rtvar_size;
	unsigned long 	cur_scan_data_adr;
	unsigned long 	prev_scan_data_adr;
	long		initial_scan_num;
	long		tag2snd;
	int		num_tables;
        int		dspSrcBuf;
        int		dspDstBuf;
#ifdef VXWORKS
	TBL_ID		*table_ptr;
	SEM_ID		pAcodeControl;     /* Mutex Semaphore for Acode/Table updt */
	SEM_ID		pSemParseUpdt;     /* Binary Semaphore  for Acode/Table updt */
	SEM_ID		pSemParseSuspend;  /* 4 Sync Ops needing fifo (AUTOLOCK & AUTOSHIM) */
	WDOG_ID 	wdFifoStart;
#endif
} ACODE_OBJ;

typedef ACODE_OBJ *ACODE_ID;

/* defines for interactive_flag in ACODE_OBJ  */
#define ACQI_INTERACTIVE 1
#define ACQ_AUTOGAIN	 2

/* ---   Parse Ahead Constants --- */
#define PARSE_AHEAD_COUNT 64  /* 32 increased to 64 to help EPI exp on DDR */
#define PARSE_AHEAD_ITR_LOCATION 48  /* (PARSE_AHEAD_COUNT/2) */
#define PARSE_AHEAD_ITR_INTERVAL 4  /*  Interval to add interrupts */

/*

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

ACODE_OBJ *acodeCreate(char *expname);


unsigned int *getAcodeSet(char *expname, int acode_index, unsigned int *size, int timeVal);
int getPattern(int pat_index,int* *patternList,int *patternSize);
int *TableElementPntrGet(int table_index, int element, int *errorcode, char *emssg);
/* int sendCntrlFifoList(int* list, int size, int ntimes , int rem, int startfifo); */
int sendCntrlFifoList(int* list, int size, int ntimes);
int writeCntrlFifoWord(int word);
int writeCntrlFifoBuf(int *list, int size);
int flushCntrlFifoRemainingWords();
int startCntrlFifo();
void wait4CntrlFifoStop();
int rmAcodeSet(char *expname, int cur_acode_set);
int markAcodeSetDone(char *expname, int cur_acode_set);

void acodeDelete(ACODE_ID pAcodeId);
void acodeStartFifoWD(ACODE_ID pAcodeId);
void acodeCancelFifoWD(ACODE_ID pAcodeId);
unsigned short *getWFSet(char *expname);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */


ACODE_OBJ *acodeCreate();
void acodeDelete();
void acodeStartFifoWD();
void acodeCancelFifoWD();
unsigned short *getWFSet();

#endif

#ifdef __cplusplus
}
#endif

#endif
