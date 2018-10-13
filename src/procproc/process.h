/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCprocessh
#define INCprocessh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* This one ExpInfo Structure */ 
typedef struct _expinfst {
		char 		ExpId[EXPID_LEN];
		SHR_MEM_ID  	ShrExpInfo; /* shared Memory via mmap of Exp info */
		SHR_EXP_INFO 	ExpInfo; /* start address of shared Exp. Info Structure */
		}  ExpInfoEntry;


#ifdef __cplusplus
}
#endif

#endif /* INCprocessh */

