/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef VFILESYS_H
#define VFILESYS_H

#ifndef DPS
#include <unistd.h>
#endif

extern int  appdirFind(const char *filename, const char *lib, char *fullpath,
               const char *suffix, int perm);
extern int  appdirAccess(const char *lib, char *fullpath);
extern char *getAppdirValue();
extern void setAppdirValue(char *path);

extern int  follow_link(char *start, char *final, int flen );
extern int  isDiskFullFile(char *path, char *path_2, int *resultadr );
extern int  isDiskFullSize(char *path, int size, int *resultadr );

#endif
