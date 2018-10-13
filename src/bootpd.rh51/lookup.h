/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* lookup.h */

#include "bptypes.h"	/* for int32, u_int32 */

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern u_char *lookup_hwa P((char *hostname, int htype));
extern int lookup_ipa P((char *hostname, u_int32 *addr));
extern int lookup_netmask P((u_int32 addr, u_int32 *mask));

#undef P
