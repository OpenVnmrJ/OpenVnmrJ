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
double asin();
double r_asin(x) real *x;
#else
#undef abs
#include "math.h"
double r_asin(real *x)
#endif
{
return( asin(*x) );
}
