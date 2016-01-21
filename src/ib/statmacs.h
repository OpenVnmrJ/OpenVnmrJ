/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
// @(#)statmacs.h 18.1 03/21/08 (c)1992 SISCO

#ifndef STATMACS_H
#define STATMACS_H

#include <sys/stat.h>

#ifndef S_ISBLK
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISCHR
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)
#endif
#ifndef S_ISREG
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#endif

#ifndef _POSIX_SOURCE

#ifndef S_ISLNK
#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)
#endif

#endif  /* _POSIX_SOURCE */

#endif  /* STATMACS_H */
