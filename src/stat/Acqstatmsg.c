/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include  <stdio.h>
#include  <suntool/sunview.h>
#include  <suntool/panel.h>

static char *valid_args[] = {
	"noacqproc",
	"statpresent",
	NULL
};

static char *user_report[] = {
	"The acquisition process is not active.  Please read the manual for details on how to start it.  Type any key to continue.",
        "The acquisition status window is presently active.  Type any key to continue.",
	NULL
};

main( argc, argv )
int argc;
char *argv[];
{
	int	iter, ival, wfd;
	Frame	base_frame;

/*  Verify manner in which called.  Silently exit if not right.  */

	if (argc < 2) exit();
	ival = -1;
	for (iter = 0; valid_args[ iter ] != NULL; iter++)
	  if (strcmp(argv[ 1 ], valid_args[ iter ] ) == 0) {
		ival = iter;
		break;
	  }
	if (ival < 0) exit();			/* Not found */

	base_frame = window_create( NULL, FRAME, 0 );
	if (base_frame == NULL) {
		fprintf( stderr, "SUNTOOLS NOT ACTIVE!!!\n" );
		exit();
	}
	wfd = (int) window_get( base_frame, WIN_FD );
	ival = wmgr_confirm( wfd, user_report[ ival ] );
	printf( "wmgr_confirm returned %d\n", ival );
}
