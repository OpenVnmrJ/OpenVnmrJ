/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef BGVARS_H
#define BGVARS_H

extern void setupBlocking();
extern int P_sendTPVars(int tree, int fd);
extern void P_endPipe(int fd);
extern int P_sendGPVars(int tree, int group, int fd);
extern int P_sendVPVars(int tree, char *name, int fd);
extern int J_sendTPVars(int tree, int fd);
extern int J_sendGPVars(int tree, int group, int fd);
extern int J_sendVPVars(int tree, char *name, int fd);

#endif
