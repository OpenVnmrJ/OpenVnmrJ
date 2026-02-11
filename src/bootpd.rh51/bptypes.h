/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* bptypes.h */

#ifndef	BPTYPES_H
#define	BPTYPES_H

/*
 * 32 bit integers are different types on various architectures
 */

#ifndef	int32
#define int32 int
#endif
typedef unsigned int32 u_int32;

/*
 * Nice typedefs. . .
 */

typedef int boolean;
typedef unsigned char byte;


#endif	/* BPTYPES_H */
