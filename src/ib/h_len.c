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
shortint h_len(s, n) char *s; ftnlen n;
#else
shortint h_len(char *s, ftnlen n)
#endif
{
return(n);
}
