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
extern double exp(), cos(), sin();

 VOID c_exp(r, z) complex *r, *z;
#else
#undef abs
#include "math.h"

void c_exp(complex *r, complex *z)
#endif
{
double expx;

expx = exp(z->r);
r->r = expx * cos(z->i);
r->i = expx * sin(z->i);
}
