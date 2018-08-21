/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifdef HEADER_ID

#ifndef lint
	static char DEBUG_ALLOC_H_ID[] = "@(#)debug_alloc.h 18.1 03/21/08 20:00:57 Copyright 1992 Spectroscopy Imaging Systems";
#endif (not) lint

#else (not) HEADER_ID

#ifndef DEBUG_ALLOC_H
#define DEBUG_ALLOC_H

#ifndef REAL_ALLOC
#define calloc  DEBUG_CALLOC
#define malloc  DEBUG_MALLOC
#define realloc DEBUG_REALLOC
#define free    DEBUG_FREE
#endif

/* functions defined in this module */

char *DEBUG_CALLOC( /* unsigned nelem, unsigned elsize */ );
char *DEBUG_MALLOC( /* unsigned size */ );
char *DEBUG_REALLOC( /* char *ptr, unsigned size */ );
int   DEBUG_FREE( /* char *p */ );
int   check_list( /* void */ );

#endif (not) DEBUG_ALLOC_H

#endif HEADER_ID
