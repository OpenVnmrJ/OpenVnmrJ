/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* getif.h */

#ifdef	__STDC__
extern struct ifreq *getif(int, struct in_addr *);
#else
extern struct ifreq *getif();
#endif
