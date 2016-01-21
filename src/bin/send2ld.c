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


main(argc, argv)
int	argc;
char    **argv;
{
	int   pipe;
	char  mess[256];

	if (argc < 3)
	     exit(0);
	pipe = atoi(argv[1]);
	sprintf(mess, "%s $  %s\n", argv[2], argv[3]);
	if (pipe >= 0)
	     write(pipe, mess, strlen(mess));
}

