/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* readfile.h */

#include "bptypes.h"
#include "hash.h"

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern boolean hwlookcmp P((hash_datum *, hash_datum *));
extern boolean iplookcmp P((hash_datum *, hash_datum *));
extern boolean nmcmp P((hash_datum *, hash_datum *));
extern void readtab P((int));
extern void rdtab_init P((void));

#undef P

