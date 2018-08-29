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
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

static int
copyFile(infile, outfile)
     char *infile;
     char *outfile;
{
    int fdi;
    int fdo;
    int len;
    char buf[4096];

    fdi = open(infile, O_RDONLY);
    if (fdi == -1) {
	perror("cptoconpar: infile open()");
	return 1;
    }
    fdo = open(outfile, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fdo == -1) {
	perror("cptoconpar: outfile open()");
	return 1;
    }
    while (len=read(fdi, buf, sizeof(buf))) {
	write(fdo, buf, len);
    }
    return 0;
}

/*-----------------------------------------------------------------------------
|  cptoconpar - This is a specialized program that copies the file
|		/tmp/conpartmp/ to $vnmrsystem/conpar using setuid permission.
|		A copy of the old conpar file is left in
|		"/tmp/conpar.<username>.cptoconpar".
|		Its purpose is to let users other than vnmr1 modify conpar.
|		
|	     Successful Status = 0.
|	M. Howitt 10/29/92
|	Modified: C. Price 3/18/98: Eliminate calls to Unix shell; they
|		do not work in setuid mode.
+-----------------------------------------------------------------------------*/
int
main(argc,argv)
     int argc;
     char *argv[];
{
    char *vnmrsystem;
    char conpar[PATH_MAX];
    char userName[LOGIN_NAME_MAX]; // L_cuserid=9 depricated, use _POSIX_LOGIN_NAME_MAX=9, or LOGIN_NAME_MAX=256
    char oldConpar[PATH_MAX];
    char newConpar[] = "/tmp/conpartmp";

    if ( (vnmrsystem = getenv("vnmrsystem")) == NULL) {
	vnmrsystem = "/vnmr";
    }
    sprintf(conpar, "%.1000s/conpar", vnmrsystem);
    cuserid(userName);
    sprintf(oldConpar,"/tmp/conpar.%s.cptoconpar", userName);

    if (copyFile(conpar, oldConpar)) {
	fprintf(stderr,"%s: Cannot copy %s to %s\n",
		argv[0], conpar, oldConpar);
	return 1;
    }
    if (copyFile(newConpar, conpar)) {
	fprintf(stderr,"%s: Cannot copy %s to %s\n",
		argv[0], newConpar, conpar);
	copyFile(oldConpar, conpar);
	return 1;
    }

    return 0;
}
