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
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <netdb.h>

#include "acquisition.h"
#include "hostAcqStructs.h"

int
send2AcqWithAuth( int cmd_for_acq, char *msg_for_acq )
{
	char		*ourmsg;
	int		 ival, tlen, ulen;
	struct passwd	*getpwuid();
	struct passwd	*pasinfo;
 
	pasinfo = getpwuid((int) getuid());
	ulen = strlen( pasinfo->pw_name );
	tlen = ulen + 2 + 6 + strlen( msg_for_acq ) + 1;

	ourmsg = (char *) malloc( tlen );

	sprintf( ourmsg, "%s,,%d\n%s", pasinfo->pw_name, getpid(), msg_for_acq );

	ival = send2Acq( cmd_for_acq, ourmsg );
	free( ourmsg );
	return( ival );
}


static int
locateCdbFile( char *cdbFile )
{
	strcpy( cdbFile, getenv("vnmrsystem") );
	if (strlen( cdbFile ) < (size_t) 1)
	  strcpy( cdbFile, "/vnmr" );
	strcat( cdbFile, "/acqqueue/console.debug" );
}

static void
showStmDebug( STM_DEBUG *stmdbp, int index )
{
	printf( "STM %d Status register: 0x%04x\n", index, stmdbp->stmStatus );
	printf( "STM %d Tag register: %d\n", index, stmdbp->stmTag );
	printf( "STM %d NP counter: %d\n", index, stmdbp->stmNpReg );
	printf( "STM %d NT counter: %d\n", index, stmdbp->stmNtReg );
	printf( "STM %d Source address: 0x%08lx\n", index, stmdbp->stmSrcAddr );
	printf( "STM %d Destination address: 0x%08lx\n", index, stmdbp->stmDstAddr );
}

static void
showConsoleDebugStruct( CONSOLE_DEBUG *cdbp )
{
	int	iter;

	printf( "Time stamp: %s", ctime( (time_t *) &cdbp->timeStamp ) );

/*  ctime appends a new-line to the string it returns ...  */

	printf( "Magic number: %d\n", cdbp->magic );
	printf( "Revision number: %d\n", cdbp->revNum );
	printf( "Acquisition state: %d\n", cdbp->Acqstate );
	printf( "startup system configuration: 0x%08lx\n", cdbp->startupSysConf );
	printf( "current system configuration: 0x%08lx\n", cdbp->currentSysConf );

	printf( "FIFO status: 0x%08lx\n", cdbp->fifoStatus );
	printf( "FIFO Interrupt Mask: 0x%04x\n", cdbp->fifoIntrpMask );
	printf( "FIFO Last Word: 0x%08lx 0x%08lx 0x%08lx\n",
		 cdbp->lastFIFOword[ 0 ], cdbp->lastFIFOword[ 1 ], cdbp->lastFIFOword[ 2 ] );

	printf( "ADC Status: 0x%x\n", cdbp->adcStatus );
	printf( "ADC Interrupt Mask: 0x%x\n", cdbp->adcIntrpMask );

	printf( "Automation Status: 0x%x\n", cdbp->autoStatus );
	printf( "Automation H S & R Status: 0x%x\n", cdbp->autoHSRstatus );

	for (iter = 0; iter < MAX_STM_OBJECTS; iter++) {
		if (STM_PRESENT( iter ) & cdbp->currentSysConf)
		  showStmDebug( &cdbp->stmHwRegs[ iter ], iter );
		else
		  printf( "STM %d not present\n", iter );
	}
}

showConsoleDebug( char *cdbFile )
{
	int		cdb_fd;
	CDB_BLOCK	cdbBlock;

	cdb_fd = open( cdbFile, O_RDONLY );
	if (cdb_fd < 0) {
		fprintf( stderr, "cannot access console debug information\n" );
		exit( 1 );
	}
	read( cdb_fd, &cdbBlock.index, sizeof( cdbBlock ) - sizeof( cdbBlock.msg_type ) );
	close( cdb_fd );

	showConsoleDebugStruct( &cdbBlock.cdb );
}

static time_t
getFileMtime( char *file )
{
	int		ival;
	struct stat	tstat;

	ival = stat( file, &tstat );
	if (ival != 0)
	  return( (time_t) 0 );
	else
	  return( tstat.st_mtime );
}

main( int argc, char *argv[] )
{
	char	parameter[ 122 ], cdbFile[ 122 ], thisHostName[ MAXHOSTNAMELEN ];
	int	iter, ival, paramval;
	time_t	lastModify, curModify;

	if (argc < 2) {
		fprintf( stderr, "Usage %s <startup/abort>\n", argv[ 0 ] );
		exit( 1 );
	}
	else if (strncmp( argv[ 1 ], "startup", strlen( "startup" ) ) == 0) {
		paramval = SYSTEM_STARTUP;
	}
	else if (strncmp( argv[ 1 ], "abort", strlen( "abort" ) ) == 0) {
		paramval = SYSTEM_ABORT;
	}
	else {
		fprintf( stderr, "Choose startup or abort as an argument\n" );
		exit( 1 );
	}

	gethostname( &thisHostName[ 0 ], sizeof( thisHostName ) );
	INIT_VNMR_COMM( &thisHostName[ 0 ] );
	INIT_ACQ_COMM( getenv( "vnmrsystem" ) );

	locateCdbFile( &cdbFile[ 0 ] );
	lastModify = getFileMtime( &cdbFile[ 0 ] );

	sprintf( &parameter[ 0 ], "%d\n%d", GETCONSOLEDEBUG, paramval );
	ival = send2AcqWithAuth( TRANSPARENT, &parameter[ 0 ] );
	if (ival != 0) {
		fprintf( stderr, "Failed to contact acquisition system\n" );
		exit( 1 );
	}

	for (iter = 0; ; iter++ ) {
		curModify = getFileMtime( &cdbFile[ 0 ] );

/* Test succeeds if the console debug file has not been updated.  */

		if (curModify == (time_t) 0 || curModify <= lastModify) {
			if (iter > 2) {
				fprintf( stderr,
			   "Acqusition system never received debug block\n"
				);
				exit( 1 );
			}
			else
			  sleep( 1 );
		}
		else
		  break;
	}

	showConsoleDebug( &cdbFile[ 0 ] );
	exit( 0 );
}
