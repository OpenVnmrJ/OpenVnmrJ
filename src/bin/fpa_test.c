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

/*  This program is part of the VNMR installation procedure.  It
    determines if a floating point accelerator is present on the
    SUN-3.  If no FPA is found, we assume a 68881 co-processor is
    present.  The program looks for the presence of /dev/fpa.
    If not found, it creates it and then tries to open the device.
    The call to open fails if the hardware is not present, even if
    the entry /dev/fpa is present.  If that happens and the program 
    had created /dev/fpa, it removes it.

    If an FPA is found, the program exits with a status of zero.

    If this is run on a SUN-4, it squawks and exits with a status
    of non-zero, as this program should not be started on the SUN-4.

    The process must have a user-ID of zero (root) to run this
    program.  If that is not the case, it exits with a status of
    non-zero.

    This program is designed to be used by a shell script, which
    uses the exit status to determined whether to configure VNMR
    for FPA or 68881 (if on a SUN-3).					*/

#include <stdio.h>

main()
{
	char	 architecture[ 122 ];
	int	 fpa_fd, made_entry;
	FILE	*pf;

	if (getuid() != 0) {
		fprintf( stderr, "You must be root to run fpa_test\n" );
		exit( 11 );
	}

/*  Process verified to be ROOT  */

	pf = popen( "arch", "r" );
	if (pf == NULL) {
		fprintf( stderr, "error starting 'arch' command\n" );
		exit( 12 );
	}

	if (fgets( &architecture[ 0 ], 120, pf ) == NULL) {
		fprintf( stderr, "error obtaining output from 'arch' command\n" );
		exit( 13 );
	}
	pclose( pf );

/*  Use strncmp to avoid problem with new-line character */

	if (strncmp( "sun4", &architecture[ 0 ], 4 )  == 0) {
		fprintf( stderr, "error:  'fpa_test' started on a SUN-4\n" );
		exit( 14 );
	}
	else if (strncmp( "sun3", &architecture[ 0 ], 4 ) != 0) {
		fprintf( stderr,
	    "error:  'fpa_test' started on unsupported architecture %s\n",
	    &architecture[ 0 ]
		);
		exit( 15 );
	}

/*  System verified to be a SUN-3.  Now try to access /dev/fpa.
    If not successful, create the entry.  Then try to open the
    device.  If the call to `open' is not successful, the system
    does not have an FPA and this program exits with a status of
    1.  If it is successful, then an FPA does exist and the
    program exits with a status of 0.

    A status of 0 is interpreted by UNIX as TRUE; a non-zero
    status us interpreted as FALSE.  See the corresponding
    script `setfloat'.						*/

	made_entry = 0;
	if (access( "/dev/fpa", 0 ) != 0 ) {
		system( "cd /dev; MAKEDEV fpa" );
		made_entry = 131071;
	}

	if ( (fpa_fd = open( "/dev/fpa", 0 )) < 0 ) {
		if (made_entry)
		  unlink( "/dev/fpa" );
		else {
			fprintf( stderr,
		    "Could not access preexisting entry /dev/fpa\n"
			);
		}
		exit( 1 );
	}
	close( fpa_fd );
	if (made_entry) {
		printf( "Created entry /dev/fpa\n" );
	}
	exit( 0 );
}
