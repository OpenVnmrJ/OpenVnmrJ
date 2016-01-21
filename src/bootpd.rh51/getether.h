/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* getether.h */

#ifdef	__STDC__
extern int getether(char *ifname, char *eaptr);
#else
extern int getether();
#endif
