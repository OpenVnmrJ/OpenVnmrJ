/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Define consecutive integers each greater
    than 0 to represent the current mode of VNMR

    Please note well that locksys.c has a character table
    that assumes the definitions specified below.  */

#define NUM_LOCK_MODES	4

#define FIRST_MODE	1
#define ACQUISITION	1
#define BACKGROUND	2
#define FOREGROUND	3
#define AUTOMATION	4
