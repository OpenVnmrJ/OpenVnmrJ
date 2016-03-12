/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _FLAGS_H
#define _FLAGS_H

#define FLAGS_FORCE_GEOID(x)   ((x) & 0x010000)
#define FLAGS_SAFEGATES(x)   (!((x) & 0x0F0000))
#define FLAGS_IGNORE_MD5(x)    ((x) & 0x020000)
#define FLAGS_NOHW(x)          ((x) & 0x040000)
#define FLAGS_EXIT_IF(x)     ((((x) & 0xFF0000) == 0xF00000) ? (x) & 0x0000FF : 0)
#define EXIT_IF_FLAGGED(x) if (prtFlag(x)) return 0
#define SKIP_IF_FLAGGED(x) if (prtFlag(x)) 
//int prtFlag(int);

#endif
