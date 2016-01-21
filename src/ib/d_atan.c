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
double atan();
double d_atan(x) doublereal *x;
#else
#undef abs
#include "math.h"
double d_atan(doublereal *x)
#endif
{
return( atan(*x) );
}
