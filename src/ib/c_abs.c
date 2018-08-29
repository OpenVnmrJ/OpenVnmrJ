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
extern double f__cabs();

double c_abs(z) complex *z;
#else
extern double f__cabs(double, double);

double c_abs(complex *z)
#endif
{
return( f__cabs( z->r, z->i ) );
}
