/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCexpentrystructsh
#define INCexpentrystructsh

#include "expQfuncs.h"
#include "shrMLib.h"
#include "shrexpinfo.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* This one Exp Entry Info Structure */ 
typedef struct _expentryinfo {
		int 		ExpPriority;
		char 		ExpId[EXPID_LEN];
		SHR_MEM_ID  	ShrExpInfo; /* shared Memory via mmap of Exp info */
		SHR_EXP_INFO 	ExpInfo; /* start address of shared Exp. Info Structure */
		SHR_MEM_ID  	ShrExpStatus; /* shared Memory via mmap of Exp info */
		SHR_EXP_INFO 	ExpStatus; /* start address of shared Exp. Info Structure */
		}  ExpEntryInfo;


#ifdef __cplusplus
}
#endif

#endif /* INCexpentrystructsh */

