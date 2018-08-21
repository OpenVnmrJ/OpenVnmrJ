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

#ifndef MAGICAL_CMDS_H
#define MAGICAL_CMDS_H



/*-------------------------------------------------------------------------
|	commands.h
|
|	This include file contains the names, addresses, and other
|	information about all commands.
+-------------------------------------------------------------------------*/
#include <stdio.h>

struct _cmd {
    char   *name;
    int   (*func)();
};
typedef struct _cmd cmd;

extern int (*getBuiltinFunc())();

#endif /* MAGICAL_CMDS_H */
