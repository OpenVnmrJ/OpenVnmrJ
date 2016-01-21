/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef SSHA_H
#define SSHA_H

extern int initSSHA();
extern void initSSHAshimmethod();
extern int translateSSHAmethod(char hdwshimmethod[] );
extern void hdwshiminit();
extern void hdwshiminitPresat();
extern void setSSHAdelayTooShort();
extern void setSSHAdelayNotTooShort();
extern void setSSHAdisable();
extern void setSSHAreenable();
extern void setSSHAoff();
extern void turnOnSSHA(double delaytime);
extern void turnOffSSHA();
extern int isSSHAselected();
extern int isSSHAPselected();
extern int isSSHAactive();
extern int isSSHAstillDoItNow();
extern int isSSHAdelayTooShort();
extern int isSSHAPresatInit();
#endif
