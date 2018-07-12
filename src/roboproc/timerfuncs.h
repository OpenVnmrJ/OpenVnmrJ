/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef __TIMERFUNCS_H__
#define __TIMERFUNCS_H__

extern int timer_went_off;

extern int setup_ms_timer( int ms_interval );
extern int cleanup_from_timeout(void);
extern void delayAwhile(int time /* seconds */);
extern void delayMsec(int time /* milliseconds */);

#endif
