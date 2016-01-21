/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef DDLSCANNER_H
#define DDLSCANNER_H

extern int ddlInitscanner();
extern void ddlClosescanner();
extern void ddlExecError(const char*, const char* token = 0);

#endif /* DDLSCANNER_H */
