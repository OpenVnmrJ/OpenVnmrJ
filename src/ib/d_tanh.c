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
double tanh();
double d_tanh(x) doublereal *x;
#else
#undef abs
#include "math.h"
double d_tanh(doublereal *x)
#endif
{
return( tanh(*x) );
}
