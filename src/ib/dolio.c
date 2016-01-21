/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include "f2c.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef KR_headers
extern int (*f__lioproc)();

integer do_lio(type,number,ptr,len) ftnint *number,*type; char *ptr; ftnlen len;
#else
extern int (*f__lioproc)(ftnint*, char*, ftnlen, ftnint);

integer do_lio(ftnint *type, ftnint *number, char *ptr, ftnlen len)
#endif
{
	return((*f__lioproc)(number,ptr,len,*type));
}
#ifdef __cplusplus
	}
#endif
