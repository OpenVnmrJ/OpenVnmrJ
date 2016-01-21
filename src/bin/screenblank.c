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

/* screenblank.c - screen blanking daemon
**
** Comments to:
**   jef@netcom.com
**   jef@well.sf.ca.us
**
** Copyright (C) 1989, 1990, 1991, 1993 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Changes made at Varian Associates, NMR
**  compiles on Solaris or SunOS.  For Solaris, enter:
**          make -f Makefile CFLAGS=-DSOLARIS
**  becomes a Daemon process.  See become_a_daemon
*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef SOLARIS
#include <sys/fbio.h>
#else
#include <sun/fbio.h>
#endif


/* Definitions. */

#define CONSOLE_NAME "/dev/console"
#define KEYBOARD_NAME "/dev/kbd"
#define MOUSE_NAME "/dev/mouse"

#define DEFAULT_WAITING_SECS 600	/* ten minutes */
#define BLANKING_USECS 250000		/* four times a second */

#define MAX_FBS 100

#define min(a,b) ( (int) (a) < (int) (b) ? (a) : (b) )
#define max(a,b) ( (int) (a) > (int) (b) ? (a) : (b) )

#ifndef NOFILE
#define NOFILE 20
#endif

/* Forward routines. */

static void blank();
static void find_fbs();
static unsigned idle_time();
static void my_stat();
static void set_video();
static void sig_handle();
static void unblank();
static void usleep();


/* Variables. */

static char* argv0;
static int ign_mouse;
static int ign_keyboard;
static unsigned waiting_secs;
static int num_fbs;
static char* fb_name[MAX_FBS];
static int video[MAX_FBS];
static time_t curr_time;


/* Routines. */

/*  This program is a effort at a generic become-a-daemon program.
    When it returns, the initial or parent process has exited and
    a Child process now has control.  It belongs to its own process
    group.  It has disassociated from any controlling terminal and
    redirected output to the console.					*/

static int
become_a_daemon()
{
	int	iter, ival, tty_fd;

/* Disassociate from Control Terminal */

	ival = fork();
	if (ival > 0)
	  exit( 0 );		/* parent process */
	else if (ival < 0) {
		printf( "limNET:  Error creating daemon process\n\r" );
		exit( 1 );
	}

/*  This is the Child process now */

	freopen("/dev/null","r",stdin);
	freopen("/dev/console","a",stdout);
	freopen("/dev/console","a",stderr);

	for (iter = 3; iter < NOFILE; iter++)
	  close(iter);			/* close any inherited open file descriptors */

	ival = setsid();		/* the setsid program will disconnect from */
					  /* controlling terminal and create a new */
				 /* process group, with this process as the leader */
#ifdef SIGTTOU
	signal(SIGTTOU, SIG_IGN);
#endif
}

void
main( argc, argv )
    int argc;
    char* argv[];
    {
    int argn;
    unsigned idle;
    char* usage = "usage:  %s [-mouse] [-keyboard] [-delay <seconds>] [-fb <framebuffer> ...]\n";

    argv0 = *argv;

    /* Parse flags. */
    argn = 1;
    ign_mouse = 0;
    ign_keyboard = 0;
    waiting_secs = DEFAULT_WAITING_SECS;
    num_fbs = 0;
    while ( argn < argc && argv[argn][0] == '-' )
	{
	if ( strncmp(argv[argn],"-delay",max(strlen(argv[argn]),2)) == 0 )
	    {
	    ++argn;
	    if ( argn >= argc ||
		 sscanf( argv[argn], "%d", &waiting_secs ) != 1 )
		{
		(void) fprintf( stderr, usage, argv0 );
		exit( 1 );
		}
	    }
	else if ( strncmp(argv[argn],"-mouse", max(strlen(argv[argn]),2)) == 0 )
	    ign_mouse = 1;
	else if ( strncmp(argv[argn],"-keyboard",max(strlen(argv[argn]),2))==0 )
	    ign_keyboard = 1;
	else if ( strncmp(argv[argn],"-fb",max(strlen(argv[argn]),2)) == 0 )
	    {
	    ++argn;
	    if ( argn >= argc )
		{
		(void) fprintf( stderr, usage, argv0 );
		exit( 1 );
		}
	    fb_name[num_fbs] = argv[argn];
	    video[num_fbs] = 1;
	    ++num_fbs;
	    }
	else
	    {
	    (void) fprintf( stderr, usage, argv0 );
	    exit( 1 );
	    }
	++argn;
	}

    if ( argn != argc )
	{
	(void) fprintf( stderr, usage, argv0 );
	exit( 1 );
	}

    become_a_daemon();

    /* If no fb's specified, search through /dev and find them all. */
    if ( num_fbs == 0 )
	find_fbs();
    
    (void) signal( SIGHUP, sig_handle );
    (void) signal( SIGINT, sig_handle );
    (void) signal( SIGTERM, sig_handle );
    (void) nice( 15 );

    /* Main loop. */
    for (;;)
	{
	/* Wait for idleness to be reached. */
	idle = 0;
	do
	    {
	    (void) sleep( waiting_secs - idle );
	    }
	while ( ( idle = idle_time() ) < waiting_secs );

	/* Ok, we're idle.  Blank it. */
	blank();

	/* Watch for non-idleness. */
	do
	    {
	    usleep( BLANKING_USECS );
	    }
	while ( idle_time() >= waiting_secs );

	/* No longer idle.  Clean up. */
	unblank();
	}
    }


static void
find_fbs()
    {
    static char* fb_names[] = {
        "/dev/fb",
        "/dev/bwone0", "/dev/bwone1",
        "/dev/bwtwo0", "/dev/bwtwo1",
        "/dev/cgone0", "/dev/cgone1",
        "/dev/cgtwo0", "/dev/cgtwo1",
        "/dev/cgthree0", "/dev/cgthree1",
        "/dev/cgfour0", "/dev/cgfour1",
        "/dev/cgfive0", "/dev/cgfive1",
        "/dev/cgsix0", "/dev/cgsix1",
        "/dev/cgeight0", "/dev/cgeight1",
        "/dev/cgnine0", "/dev/cgnine1",
        "/dev/gpone0a", "/dev/gpone1a",
	(char*) 0
	};
    char** fbn;
    int fd;

    for ( fbn = fb_names; *fbn != (char*) 0; ++fbn )
	{
	fd = open( *fbn, O_RDWR );
	if ( fd >= 0 )
	    {
	    (void) close( fd );
	    fb_name[num_fbs] = *fbn;
	    video[num_fbs] = 1;
	    ++num_fbs;
	    }
	}
    }


/*ARGSUSED*/
static void
sig_handle()
    {
    unblank();
    exit( 0 );
    }


static void
blank()
    {
    int fb_num;

    for ( fb_num = 0; fb_num < num_fbs; ++fb_num )
	set_video( fb_num, 0 );
    }


static void
unblank()
    {
    int fb_num;

    for ( fb_num = 0; fb_num < num_fbs; ++fb_num )
	if ( ! video[fb_num] )
	    set_video( fb_num, 1 );
    }


static unsigned
idle_time()
    {
    struct stat sb;
    long mintime;

    curr_time = time( (long*) 0 );

    my_stat( CONSOLE_NAME, &sb );
    mintime = curr_time - sb.st_atime;
    mintime = min( mintime, curr_time - sb.st_mtime );

    if ( ! ign_mouse )
	{
	my_stat( MOUSE_NAME, &sb );
	mintime = min( mintime, curr_time - sb.st_atime );
	mintime = min( mintime, curr_time - sb.st_mtime );
	}

    if ( ! ign_keyboard )
	{
	my_stat( KEYBOARD_NAME, &sb );
	mintime = min( mintime, curr_time - sb.st_atime );
	mintime = min( mintime, curr_time - sb.st_mtime );
	}

    return mintime;
    }


static void
my_stat( dev, sbP )
    char* dev;
    struct stat* sbP;
    {
    char quickstr[ 122 ];

    if ( stat( dev, sbP ) < 0 )
	{
	(void) sprintf(
	    &quickstr[ 0 ], "%s: stat %s", argv0, dev );
	perror( &quickstr[ 0 ] );
	exit( 1 );
	}
    }


static void
set_video( fb_num, b )
    int fb_num, b;
    {
    int fd, fbv;

    fd = open( fb_name[fb_num], O_RDWR );
    if ( fd < 0 )
	{
	(void) fprintf(
	    stderr, "%s: error opening fb %s\n", argv0, fb_name[fb_num] );
	exit( 1 );
	}
    fbv = b ? FBVIDEO_ON : FBVIDEO_OFF;
    if ( ioctl( fd, FBIOSVIDEO, &fbv ) < 0 )
	{
	(void) fprintf(
	    stderr, "%s: error setting video on fb %s\n", argv0,
	    fb_name[fb_num] );
	exit( 1 );
	}
    (void) close( fd );
    video[fb_num] = b;
    }


static void
usleep( usecs )
    unsigned long usecs;
    {
    struct timeval timeout;

    timeout.tv_sec = usecs / 1000000L;
    timeout.tv_usec = usecs - timeout.tv_sec * 1000000L;

    (void) select( 0, (fd_set*) 0, (fd_set*) 0, (fd_set*) 0, &timeout );
    }
