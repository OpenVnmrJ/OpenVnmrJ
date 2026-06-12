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
extern FILE *efopen(char *progname, char *filename, char *status);
extern char *efgets(char *s, int n, FILE *stream);
extern void exitm(char *message);
extern void checkargs(char *progname, int argc, char *errmsg);
