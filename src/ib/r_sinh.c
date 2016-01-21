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
double sinh();
double r_sinh(x) real *x;
#else
#undef abs
#include "math.h"
double r_sinh(real *x)
#endif
{
return( sinh(*x) );
}
