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
double erf();
double derf_(x) doublereal *x;
#else
extern double erf(double);
double derf_(doublereal *x)
#endif
{
return( erf(*x) );
}
