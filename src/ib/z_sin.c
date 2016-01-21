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
double sin(), cos(), sinh(), cosh();
VOID z_sin(r, z) doublecomplex *r, *z;
#else
#undef abs
#include "math.h"
void z_sin(doublecomplex *r, doublecomplex *z)
#endif
{
r->r = sin(z->r) * cosh(z->i);
r->i = cos(z->r) * sinh(z->i);
}
