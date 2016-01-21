/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#define NO_METHOD	0
#define USE_Z0		1
#define USE_LOCKFREQ	2

typedef struct {
	long	method;
	long	smallest;
	long	largest;
	long	middle;
	long	initial;
	long	current;
	long	best;
} FIND_LOCK_OBJ;

typedef FIND_LOCK_OBJ *FIND_LOCK_ID;
