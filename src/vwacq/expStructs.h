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


#ifndef INCexph
#define INCexph

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
	int	num_acode_sets;
	int	cur_acode_set;
	unsigned short	*cur_acode_base;
	int	cur_acode_size;
	unsigned int	*cur_rtvar_base;
	int	cur_rtvar_size;
	int	num_tables;
	TBL_ID	table_ptr;
} EXP_OBJ;

typedef EXP_OBJ *EXP_ID;

/*

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)


#else
/* --------- NON-ANSI/C++ prototypes ------------  */


#endif

#ifdef __cplusplus
}
#endif

#endif
