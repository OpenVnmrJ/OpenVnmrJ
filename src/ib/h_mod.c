/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include "f2c.h"

#ifdef KR_headers
shortint h_mod(a,b) short *a, *b;
#else
shortint h_mod(short *a, short *b)
#endif
{
return( *a % *b);
}
