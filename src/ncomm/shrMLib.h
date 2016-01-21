/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCshrmlibh
#define INCshrmlibh


#include "mfileObj.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _shrmem {
	char    	*MemAddr;	/* mmap file starting address */
	char 		*MemPath;	/* File path that was mmapped */
	MFILE_ID	shrmem;		/* shared memory via mmap */
	int 		 mutexid;	/* Mutex Semaphhore id */
        	     } SHR_MEM_OBJ;

typedef SHR_MEM_OBJ *SHR_MEM_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)
 
extern SHR_MEM_ID  shrmCreate(char *filename,int keyid, unsigned long size);
extern void shrmRelease(SHR_MEM_ID shrmid);
extern char *shrmAddr(SHR_MEM_ID shrmid);
extern void shrmTake(SHR_MEM_ID shrmid);
extern void shrmGive(SHR_MEM_ID shrmid);
extern void  shrmShow(SHR_MEM_ID shrmid);
 
#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern SHR_MEM_ID  shrmCreate();
extern void shrmRelease();
extern char *shrmAddr();
extern void shrmTake();
extern void shrmGive();
extern void  shrmShow();
 
#endif

 
#ifdef __cplusplus
}
#endif
 
#endif
