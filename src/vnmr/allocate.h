/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------------
|
|	allocate.h
|
|	This include file contains definitions used for chained memory
|	allocation...
|
+-----------------------------------------------------------------------------*/
#ifndef ALLOCATE_H
#define ALLOCATE_H

#include <sys/types.h>

#if defined(__INTERIX)
#include <inttypes.h>
#endif

extern void *allocateWithId(register size_t n, const char *id);
extern void *allocate(size_t n);
extern int blocksAllocated(int n);
extern int blocksAllocatedTo(const char *id);
extern int blocksAllocatedToOfSize(const char *id, int n);
extern int charsAllocated(int n);
extern int charsAllocatedTo(const char *id);
extern int charsAllocatedToOfSize(const char *id, int n);
extern void release(void *p);
extern void releaseAll();
extern void releaseAllWithId(const char *id);
extern void *scanFor(const char *id, char **p, int *n, const char **i);
extern void *skyallocateWithId(size_t n, const char *id);
extern void skyrelease(void *p);
extern void skyreleaseAll();
extern void skyreleaseAllWithId(const char *id);
extern void renameAllocation(const char *old, const char *newName);
extern void releaseWithId(const char *id);

#endif
