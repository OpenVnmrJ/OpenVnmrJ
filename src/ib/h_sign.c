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
shortint h_sign(a,b) shortint *a, *b;
#else
shortint h_sign(shortint *a, shortint *b)
#endif
{
shortint x;
x = (*a >= 0 ? *a : - *a);
return( *b >= 0 ? x : -x);
}
