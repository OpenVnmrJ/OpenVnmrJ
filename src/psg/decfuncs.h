/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef DECFUNCS_H
#define DECFUNCS_H

extern void declvlon();
extern void declvloff();
extern void decpwr(double level);
extern void spareon();
extern void spareoff();
extern void decblankon();
extern void decblankoff();
extern void initsisdecblank();
extern void sisdecblank(int action);

#endif
