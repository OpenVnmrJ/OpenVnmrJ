/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include "f2c.h"

 integer
#ifdef KR_headers
lbit_shift(a, b) integer a; integer b;
#else
lbit_shift(integer a, integer b)
#endif
{
	return b >= 0 ? a << b : (integer)((uinteger)a >> -b);
	}
