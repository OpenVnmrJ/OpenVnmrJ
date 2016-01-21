/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* EFL support routine to compare two character strings */

#include "f2c.h"

#ifdef KR_headers
extern integer s_cmp();
integer ef1cmc_(a, la, b, lb) ftnint *a, *b; ftnlen *la, *lb;
#else
extern integer s_cmp(char*,char*,ftnlen,ftnlen);
integer ef1cmc_(ftnint *a, ftnlen *la, ftnint *b, ftnlen *lb)
#endif
{
return( s_cmp( (char *)a, (char *)b, *la, *lb) );
}
