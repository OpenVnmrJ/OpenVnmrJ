/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef SOCKET1_H
#define SOCKET1_H
extern int receive(char buffer[], int bufsize, int *read_more);
extern void setupVnmrAsync(void (*func)());
extern int setMasterInterfaceAsync(void (*func)());
#endif
