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
/* 
 * open a file with error checking 
 */

#include <stdio.h>
#include <stdlib.h>

FILE *efopen(char *progname, char *filename, char *status)
{
    FILE	*fname;

    if ( ( fname = fopen(filename,status) ) == NULL )
    {
	fprintf(stderr,"\n%s could not open file %s\n",progname,filename);
	exit(1);
    }
    return(fname);
}

/* 
 * read a string from a file, skipping comment lines (# in first column) 
 */

char *efgets(char *s, int n, FILE *stream)
{
    char	*status;

    do
	if ( ( status = fgets(s,n,stream) ) == NULL )
	{
	    fprintf(stderr,"\nerror while reading file\n");
	    exit(1);
	}
    while ( ( s[0] == '#' ) || ( s[0] == '\n' ) );

    return(status);
}

/* 
 * exit with a message 
 */

void exitm(char *message)
{
    fprintf(stderr,"\n%s\n\n",message);
    exit(1);
}

/* 
 * check arguments, exit if incorrect 
 */

void checkargs(char *progname, int argc, char *errmsg)
{
    if ( argc == 1 )
    {
	fprintf(stderr,"\n");
	fprintf(stderr,"usage: %s %s\n",progname,errmsg);
	fprintf(stderr,"\n");
	exit(1);
    }
}
