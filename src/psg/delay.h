/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef DELAY_H
#define DELAY_H

extern void G_Delay(int firstkey, ...);
extern void delayer(double time, int do_0_delay);
extern int create_delay(int tword1, int tword2, int do_0_delay);
extern void G_RTDelay(int firstkey, ...);
extern int gdelayer(int gtable, double time, int do_0_delay);

#endif
