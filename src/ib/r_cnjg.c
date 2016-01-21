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
VOID r_cnjg(r, z) complex *r, *z;
#else
VOID r_cnjg(complex *r, complex *z)
#endif
{
r->r = z->r;
r->i = - z->i;
}
