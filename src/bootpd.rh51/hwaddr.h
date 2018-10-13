/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* hwaddr.h */
#ifndef	HWADDR_H
#define HWADDR_H

#define MAXHADDRLEN		8	/* Max hw address length in bytes */

/*
 * This structure holds information about a specific network type.  The
 * length of the network hardware address is stored in "hlen".
 * The string pointed to by "name" is the cononical name of the network.
 */
struct hwinfo {
    unsigned int hlen;
    char *name;
};

extern struct hwinfo hwinfolist[];
extern int hwinfocnt;

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern void setarp P((int, struct in_addr *, int, u_char *, int));
extern char *haddrtoa P((u_char *, int));
extern void haddr_conv802 P((u_char *, u_char *, int));

#undef P

/*
 * Return the length in bytes of a hardware address of the given type.
 * Return the canonical name of the network of the given type.
 */
#define haddrlength(type)	((hwinfolist[(int) (type)]).hlen)
#define netname(type)		((hwinfolist[(int) (type)]).name)

#endif	/* HWADDR_H */
