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
/* tape_util.c - used with tape_main.c to assemble 'tape' */
#include  <stdio.h>
#include  <sys/types.h>
#include  <sys/file.h>
#include  <sys/ioctl.h>
#include  <sys/mtio.h>

#define  STAPE_BUFSIZE	512

static int	stp;			/*  Descriptor for streaming tape  */

open_stape()
{
	int		ival;

	ival = open( "/dev/rst8", 0, 0 );
	if (ival < 0) {
		perror( "Error opening /dev/rst8" );
		exit();
	}
	else stp = ival;
}

int stape_read( buf )
char *buf;
{
	int		ival;

	ival = read( stp, buf, STAPE_BUFSIZE );
	return( ival );
}

int skip_stape_file()
{
	int		ival;
	struct mtop	skipit;

	skipit.mt_op = MTFSF;			/*  Forward Space File  */
	skipit.mt_count = 1;			/*  1 file  */
	ival = ioctl( stp, MTIOCTOP, &skipit );
	if (ival != 0) {
		perror( "Error skipping file on streaming tape" );
		exit();
	}
}

int rewind_stape()
{
	int		ival;
	struct mtop	rewindit;

	rewindit.mt_op = MTREW;			/*  Rewind  */
	rewindit.mt_count = 1;			/*  Only once!  */
	ival = ioctl( stp, MTIOCTOP, &rewindit );
	if (ival != 0) {
		perror( "Error rewinding file on streaming tape" );
		exit();
	}
}
