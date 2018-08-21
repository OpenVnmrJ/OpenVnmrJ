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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

/**************************************************************************
    fileowner  returns the full name of the owner of the specified file.
               The only reason this is needed is because ls -l sometimes
               truncated the owner in the listing.  Several scripts use
               the owner of a system file to determine who the admin is.

    Usage: fileowner filename

*************************************************************************/

int main(int argc, char *argv[])
{
    struct passwd  *pwbuf;
    struct stat     stbuf;
    char namebuf[256];

    if(argc == 2)
    {

        /* Open the output file. */
        if ( stat(argv[1], &stbuf ) == -1)
        {
           printf("\n" );
           exit(0);
        }
        pwbuf = getpwuid( stbuf.st_uid);
        if (pwbuf == NULL)
        {
           printf("\n" );
           exit(0);
        }
        strcpy(namebuf, pwbuf->pw_name);
        printf("%s\n",namebuf );
    }
    else
    {
        printf("\n");
    }
    exit(0);
}
