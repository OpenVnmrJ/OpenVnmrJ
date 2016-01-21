/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * strerror() - for those systems that don't have it yet.
 * Doing it this way avoids yet another ifdef...
 */

/* These are part of the C library. (See perror.3) */
extern char *sys_errlist[];
extern int sys_nerr;

static char errmsg[80];

char *
strerror(en)
	int en;
{
	if ((0 <= en) && (en < sys_nerr))
		return sys_errlist[en];

	sprintf(errmsg, "Error %d", en);
	return errmsg;
}
