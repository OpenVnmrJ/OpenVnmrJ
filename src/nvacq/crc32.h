/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

#ifndef CRC_H_DEFINED
#define CRC_H_DEFINED

typedef unsigned int tcrc;      /* type of crc value -- same as in brik.c */

#define INITCRC 0xFFFFFFFF

#if defined(__STDC__) && ! defined (__cplusplus) && ! defined (c_plusplus)

extern tcrc addbfcrc (char *buf, int size);
extern tcrc addbfcrcinc (char *buf, int size, tcrc *prevcrc);

#elif defined(__cplusplus) || defined(c_plusplus)

extern "C" tcrc addbfcrc (char *buf, int size);
extern "C" tcrc addbfcrc (char *buf, int size, tcrc *prevcrc);

#else

extern tcrc addbfcrc();

#endif

#endif
