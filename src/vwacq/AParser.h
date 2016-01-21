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


#ifndef INCaparserh
#define INCaparserh

#include <wdLib.h>

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* Table Header Structure */
/* The entry element the first in a list. There are four for 	*/
/* longword alignment.						*/

typedef struct {
	int	num_entries;
	int	size_entry;
	int	mod_factor;
	char	entry[4];
} TBL_OBJ;

typedef TBL_OBJ *TBL_ID;


/* Experiment Data Structure */
/* The table_ptr is the first entry in a list. */


typedef struct {
   	char	id[32];
	int	interactive_flag;
	SEM_ID	pAcodeControl;       /* Mutex Semaphore for Acode/Table updt */
	SEM_ID	pSemParseUpdt;     /* Binary Semaphore  for Acode/Table updt */
	SEM_ID	pSemParseSuspend;  /* 4 Sync Ops needing fifo (AUTOLOCK & AUTOSHIM) */
	WDOG_ID wdFifoStart;
	int	num_acode_sets;
	int	cur_acode_set;
	unsigned short	*cur_acode_base;
	unsigned short	*cur_jump_base;
	int	cur_acode_size;
	unsigned int	*cur_rtvar_base;
	int	cur_rtvar_size;
	unsigned long cur_scan_data_adr;
	long	initial_scan_num;
	long	tag2snd;
	int	num_tables;
	TBL_ID	*table_ptr;
} ACODE_OBJ;

typedef ACODE_OBJ *ACODE_ID;

/* defines for interactive_flag in ACODE_OBJ  */
#define ACQI_INTERACTIVE 1
#define ACQ_AUTOGAIN	 2

/*

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

ACODE_OBJ *acodeCreate(char *expname);
void acodeDelete(ACODE_ID pAcodeId);
void acodeStartFifoWD(ACODE_ID pAcodeId, FIFO_ID pFifoId);
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
