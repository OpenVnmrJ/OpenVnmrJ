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

/* --- define the different byte sizes in char,int,long for SUN or VM02 --- */
#ifdef SUN
typedef char c68char;
typedef short c68int;
typedef long c68long;
#else
typedef char c68char;
typedef int c68int;
typedef long c68long;
#endif
