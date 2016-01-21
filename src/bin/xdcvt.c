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

/*  Note on values returned by routines:

    When performing a task or checking for errors, the routine
    follows the UNIX tradition of returning 0 if successful.
    A negative value implies an error has occurred.  A value of -1
    is used when the routine takes care of reporting the problem;
    thus the calling program can simply abort.  Other negative
    values are used to report on situations that may occur during
    normal operation.  The calling program must report the error.	*/

#define  IS_PAR  1
#define  IS_FID  2

#define  NOT_A_DIR	-2
#define  NO_PROCPAR	-3
#define  NO_FID		-4
#define  CANT_WRITE	-5
#define  NOSUCH_VAR	-2		/* from /jaws/sysvnmr/pvars.c */

#define  MAXPATHL	128

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>

#ifdef UNIX
#define  BAD_EXIT	1
#define  NORMAL_EXIT	0
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#else
#include  stat
#include  file
#define  SEEK_CUR	1
#define  R_OK		4
#define  W_OK		2
#define  BAD_EXIT	0
#define  NORMAL_EXIT	1
#include "unix_io.h"
#endif

#include "group.h"		/* VNMR source file */
#include "params.h"		/* VNMR source file */
#include "variables.h"		/* VNMR source file */
#include "vfh.h"		/* Varian File Header */

main( argc, argv )
int argc;
char *argv[];
{
	char	fullname[ MAXPATHL ];
	int	iter, ival;

	if (argc < 2) {
		printf( "you must provide an argument\n" );
		exit( BAD_EXIT );
	}

	for (iter = 1; iter <argc; iter++) {
		ival = find_vnmr_ds( argv[ iter ], &fullname[ 0 ], MAXPATHL );
		if (ival == 0) {
			printf(
	    "No vnmr dataset with name %s\n", argv[ iter ]
			);
			continue;
		}

		if (ival == IS_PAR) {
			printf(
	    "Found %s, a parameter set\n", &fullname[ 0 ]
			);
			continue;
		}
		ival = verify_vnmr_ds( &fullname[ 0 ] );
		if (ival != 0) {
			vnmr_ds_error( ival, &fullname[ 0 ] );
			continue;
		}

		ival = convert_vnmr_ds( &fullname[ 0 ] );
	}
	exit( NORMAL_EXIT );
}

static vnmr_ds_error( errorval, dsname )
int errorval;
char *dsname;
{
	printf( "Cannot convert %s:  ", dsname );
	if (errorval == NOT_A_DIR)
	  printf( "not a directory\n" );
	else if (errorval == NO_PROCPAR)
	  printf( "does not contain 'procpar'\n" );
	else if (errorval == NO_FID)
	  printf( "does not contain 'fid'\n" );
	else if (errorval == CANT_WRITE)
	  printf( "no write access\n" );
}

/*  This table contains the 'correct' byte order values for the
    local computer; thus its definition depends on which machine
    the program is compiled on.  In either case, the table is
    terminated with NULL; this seems the most flexible way of
    letting the program know when it reaches the end of the table.	*/

static char			*byte_order_vals[] = {
#ifdef VAX
	"vax",
	"vms",
#else
	"68000",
	"sun",
#endif
	NULL
};

static struct file_header	 fh;
static struct block_header	 bh;

static int convert_vnmr_ds( ds_path )
char *ds_path;
{
	char	 tempbuf[ 12 ], tempfile[ MAXPATHL ];
	int	 do_it, ds_len, iter, ival;

	ds_len = strlen( ds_path );
	if (ds_len > MAXPATHL-8)
	  return( -1 );

	ival = P_treereset( TEMPORARY );
	if (ival != 0) {
		printf( "P_treereset returned %d\n", ival );
		return( -1 );
	}
	strcpy( &tempfile[ 0 ], ds_path );
#ifdef UNIX
	strcat( &tempfile[ 0 ], "/procpar" );
#else
	strcat( &tempfile[ 0 ], "procpar" );
#endif
	ival = P_read( TEMPORARY, &tempfile[ 0 ] );
	if (ival != 0) {
		printf( "error reading parameters, error code = %d\n", ival );
		return( -1 );
	}

/*  do_it gets set if we decide to convert the data.  This happens
    if the byte order parameter is absent, or if its value is not
    correct for the local computer.					*/

	do_it = 0;
	ival = P_getstring( TEMPORARY, "byteorder", &tempbuf[ 0 ], 1, 10 );
	if (ival == NOSUCH_VAR) {
		ival = P_creatvar( TEMPORARY, "byteorder", T_STRING );
		if (ival != 0) {
			printf(
	    "error creating 'byteorder' parameter, error code = %d\n", ival
			);
			return( -1 );
		}
		do_it = 131071;
	}
	else if (ival != 0) {
		printf(
	    "error locating 'byteorder' parameter, error code = %d\n", ival
		);
		return( -1 );
	}

/*  At this point 'do_it' is set if the byte order parameter was not
    found in the parameter set.  If it is present, see if its value
    is appropriate for the local computer.				*/

	if (do_it == 0)
	  for (iter = 0; byte_order_vals[ iter ] != NULL; iter++)
	    if (strcmp_nocase(
			byte_order_vals[ iter ], &tempbuf[ 0 ]
	    ) == 0) {
		printf( "data set %s is in the correct format\n", ds_path );
		return( 0 );
	    }

/*  If the program exits the loop above without returning,
    we can assume the data needs to be converted.		*/

	printf( "converting %s to %s format\n", ds_path, byte_order_vals[ 0 ] );
	tempfile[ ds_len ] = '\0';
#ifdef UNIX
	strcat( &tempfile[ 0 ], "/fid" );
#else
	strcat( &tempfile[ 0 ], "fid" );
#endif
	if (open_file( &tempfile[ 0 ] ))
	  return( -1 );

	if (get_file_header())
	  return( -1 );

	cvt_file_header();

	if (check_file_header( &fh ) != 0)
	  return( -1 );
	
	if (get_cvt_buffer() != 0)
	  return( -1 );

	if (put_file_header() != 0)
	  return( -1 );

	for (iter = 0; iter <fh.nblock; iter++)
	  cvt_block( iter );

/*  Set the "byteorder" parameter; set its protection
    so the user cannot mess with it.			*/

	ival = P_setstring( TEMPORARY, "byteorder", byte_order_vals[ 0 ], 1 );
	ival = P_setprot( TEMPORARY, "byteorder",
		P_ARR | P_VAL | P_DEL | P_PRO
	);
	tempfile[ ds_len ] = '\0';
#ifdef UNIX
	strcat( &tempfile[ 0 ], "/procpar" );
#else
	strcat( &tempfile[ 0 ], "procpar" );
#endif
	ival = P_save( TEMPORARY, &tempfile[ 0 ] );

	return( 0 );
}

static cvt_block( iblock )
int iblock;
{
	if (get_block_header() != 0)
	  return( -1 );
	cvt_block_header();
	if (put_block_header() != 0)
	  return( -1 );

	cvt_data_block( iblock );
	return( 0 );
}

static cvt_file_header()
{
	cvt32( &fh.nblock );
	cvt32( &fh.ntrace );
	cvt32( &fh.npts );
	cvt32( &fh.ebytes );
	cvt32( &fh.tbytes );
	cvt32( &fh.bbytes );
	cvt16( &fh.transf );
	cvt16( &fh.status );
	cvt32( &fh.spare1 );
}

static cvt_block_header()
{
	cvt16( &bh.iscal );
	cvt16( &bh.status );
	cvt16( &bh.index );
	cvt16( &bh.spare3 );
	cvt32( &bh.ct );
	cvtf( &bh.lpval );
	cvtf( &bh.rpval );
	cvtf( &bh.lvl );
	cvtf( &bh.tlt );
}

static int	 fidfd;
static char	*bufaddr;
static int	 bufsize;

static check_file_header( fhptr )
struct file_header *fhptr;
{
	int		ival, new_nblocks, old_nblocks;
	struct stat	fidstat;

	ival = fstat( fidfd, &fidstat );
	if (ival != 0) {
		printf( "error obtaining size of FID file\n" );
		printf( "UNIX error" );
		return( -1 );
	}

/*	printf( "nblock:  %d\n", fh.nblock );
	printf( "ntrace:  %d\n", fh.ntrace );
	printf( "npts:    %d\n", fh.npts );
	printf( "ebytes:  %d\n", fh.ebytes );
	printf( "tbytes:  %d\n", fh.tbytes );
	printf( "bbytes:  %d\n", fh.bbytes );
	printf( "transf:  %d\n", fh.transf );
	printf( "status:  %d\n", fh.status );	*/

	if (fhptr->ebytes != 2 && fhptr->ebytes != 4) {
		printf( "Error in file header, ebytes = %d\n", fhptr->ebytes );
		return( -1 );
	}
	if (fhptr->ebytes == 2 && (fhptr->status & S_32) != 0) {
		printf( "Error in file header, S_32 is set and ebytes = 2\n" );
		return( -1 );
	}
	if (fhptr->ebytes == 4 && (fhptr->status & S_32) == 0) {
		printf( "Error in file header, S_32 is clear and ebytes = 4\n" );
		return( -1 );
	}

/*  Number of bytes in a trace must equal the number of points
    times the number of bytes per element (point)		*/

	if (fhptr->tbytes != fhptr->npts*fhptr->ebytes) {
		printf( "Inconsistent fields in file header:\n" );
		printf( "tbytes:  %d\n", fhptr->tbytes );
		printf( "ntrace:  %d, npts:  %d, ebytes:  %d\n",
			fhptr->ntrace, fhptr->npts, fhptr->ebytes );
		return( -1 );
	}

/*  Correct number of blocks if not consistent with the file size.  */

	if (fidstat.st_size != fhptr->nblock*fhptr->bbytes + sizeof( *fhptr )) {

	/*  The VMS file may be slightly larger than implied by the file
	    header, since VMS file size is in units of blocks.   First
	    compute the new number of blocks.  If it really is different,
	    report the change.  Because the reported file size may be
	    slighly larger, the correct number of blocks is obtained by
	    truncating the result of the division, so no special steps
	    are required.						*/

		old_nblocks = fhptr->nblock;
		new_nblocks = (fidstat.st_size - sizeof( *fhptr )) / fhptr->bbytes;
		if (old_nblocks != new_nblocks) {
			fhptr->nblock = new_nblocks;
			printf(
	   "Changing number of blocks in data set from %d to %d\n",
	    old_nblocks, new_nblocks
			);
		}
	}

	return( 0 );
}

/* Routine to allocate buffer space */

static int get_cvt_buffer()
{
	extern char		*allocate();

	bufsize = fh.ntrace*fh.npts*fh.ebytes;
	bufaddr = allocate( bufsize );
	if (bufaddr == NULL) {
		printf(
	    "Cannot allocate buffer for conversion, need %d bytes\n", bufsize
		);
		return( -1 );
	}
	return( 0 );
}

/* Routine to convert a single block */

static int cvt_data_block( iblock )
int iblock;
{
	int	 iter, num_to_convert;
	short	*sptr;
	int	*lptr;

	if (fh.ebytes != 2 && fh.ebytes != 4) {
		printf(
	    "logic error in convert data block, convert size not defined\n"
		);
		exit( BAD_EXIT );
	}

	num_to_convert = fh.ntrace * fh.npts;
	if (fh.ebytes == 2)
	  sptr = (short *) bufaddr;
	else
	  lptr = (long *) bufaddr;

	if (get_data_block( iblock ) != 0)
	  return( -1 );
	for (iter = 0; iter < num_to_convert; iter++)
	  if (fh.ebytes == 2)
	    cvt16( sptr++ );
	  else
	    cvt32( lptr++ );
	if (put_data_block( iblock ) != 0)
	  return( -1 );

	return( 0 );
}

/* Routines to access the binary FID file */

static int open_file( fname )
char *fname;
{
	fidfd = open( fname, O_RDWR );
	if (fidfd < 0) {
		printf( "error opening %s\n", fname );
		perror( "UNIX error" );
		return( -1 );
	}
	return( 0 );
}

/*  All routines assume the file pointer is positioned at the
    start of the file header (i. e., the start of the file).

    The "get" routines use lseek to position the file pointer at
    the position it was at when the program entered the routine.

    The file and block headers are global data structures.	*/

static int get_file_header()
{
	int	eval, ival;

	eval = sizeof( fh );			/* expected value */
	ival = read( fidfd, &fh, eval );
	if (ival != eval) {
		printf(
	    "error reading file header, expected %d, found %d\n", eval, ival
		);
		perror( "UNIX error" );
		return( -1 );
	}
	ival = lseek( fidfd, -eval, SEEK_CUR );

	return( 0 );
}

static int put_file_header()
{
	int	eval, ival;

	eval = sizeof( fh );			/* expected value */
	ival = write( fidfd, &fh, eval );
	if (ival != eval) {
		printf(
	    "error writing file header, expected %d, wrote %d\n", eval, ival
		);
		perror( "UNIX error" );
		return( -1 );
	}

	return( 0 );
}

static int get_block_header()
{
	int	eval, ival;

	eval = sizeof( bh );			/* expected value */
	ival = read( fidfd, &bh, eval );
	if (ival != eval) {
		printf(
	    "error reading block header, expected %d, found %d\n", eval, ival
		);
		perror( "UNIX error" );
		return( -1 );
	}
	ival = lseek( fidfd, -eval, SEEK_CUR );

	return( 0 );
}

static int put_block_header()
{
	int	eval, ival;

	eval = sizeof( bh );			/* expected value */
	ival = write( fidfd, &bh, eval );
	if (ival != eval) {
		printf(
	    "error writing block header, expected %d, wrote %d\n", eval, ival
		);
		perror( "UNIX error" );
		return( -1 );
	}

	return( 0 );
}

/*  The size and the address of the data block are static global variables.  */

static int get_data_block( n )
int n;
{
	int	ival;

	ival = read( fidfd, bufaddr, bufsize );
	if (ival != bufsize) {
		printf(
	    "error reading block %d, expected %d, found %d\n", n, bufsize, ival
		);
		perror( "UNIX error" );
		return( -1 );
	}
	ival = lseek( fidfd, -bufsize, SEEK_CUR );
	return( 0 );
}

static int put_data_block( n )
int n;
{
	int	ival;

	ival = write( fidfd, bufaddr, bufsize );
	if (ival != bufsize) {
		printf(
	    "error writing block %d, expected %d, wrote %d\n", n, bufsize, ival
		);
		perror( "UNIX error" );
		return( -1 );
	}
	return( 0 );
}

/*  Elementary data conversion routines.

    The routines for short integer and long integer are symmetric.
    The program does not need to know whether it is on a SUN or a VAX.
    The same operation repeated twice produces the original data.

    The routine for floating point numbers are NOT symmetric; thus
    the contents depend on the compiler switch VAX.  Fortunately VAX
    format is obtained by multiplying the IEEE format by 4 (the IEEE
    format as a value in the VAX) and vice-versa.

    The compiler switch VAX is defined by the VMS C compiler.  Obviously
    this switch should not be defined on the SUN.
    
    Each routine is written to access the data directly via its
    address, rather than returning a value.				*/

static cvt16( sptr )
short *sptr;
{
	register char	*tptr, tval;

	tptr = (char *) sptr;

	tval = tptr[ 1 ];
	tptr[ 1 ] = tptr[ 0 ];
	tptr[ 0 ] = tval;
}

static cvt32( iptr )
int *iptr;
{
	register char	*tptr, tval;

	tptr = (char *) iptr;

	tval = tptr[ 3 ];
	tptr[ 3 ] = tptr[ 0 ];
	tptr[ 0 ] = tval;
	tval = tptr[ 2 ];
	tptr[ 2 ] = tptr[ 1 ];
	tptr[ 1 ] = tval;
}

static cvtf( fptr )
float *fptr;
{
	char	 tmp_char;
	int	 iter;
	char	*tptr;
	union {
		char	cval[ 4 ];
		float	fval;
	} x;

/*  First copy the value, byte-wise to avoid VAX floating-point operand faults.  */

	tptr = (char *) fptr;
	for (iter = 0; iter < 4; iter++)
	  x.cval[ iter ] = *(tptr+iter);

/*  Now swap successive pairs of bytes.  */

	tmp_char = x.cval[ 0 ];
	x.cval[ 0 ] = x.cval[ 1 ];
	x.cval[ 1 ] = tmp_char;
	tmp_char = x.cval[ 2 ];
	x.cval[ 2 ] = x.cval[ 3 ];
	x.cval[ 3 ] = tmp_char;

#ifdef VAX
	x.fval *= 4.0;
#else
	x.fval *= 0.25;
#endif

	*fptr = x.fval;
}

static int verify_vnmr_ds( candidate )
char *candidate;
{
	char	tempbuf[ MAXPATHL ];
	int	len;

	len = strlen( candidate );
	if (len < 0 || len > MAXPATHL-10)
	  return( -1 );

/*  On VMS we assume the candidate is a valid Files-11 directory
    or some previous routine has constructed the appropriate
    directory tree.  If it isn't, the search for PROCPAR or FID
    will surely fail.

    On UNIX, we cannot tell if a file is a directory without
    consulting with the "stat" system call.			*/

#ifdef UNIX
	if ( isDirectory( candidate ) == 0 )
	  return( NOT_A_DIR );
#endif

	strcpy( &tempbuf[ 0 ], candidate );
#ifdef UNIX
	strcat( &tempbuf[ 0 ], "/procpar" );
#else
	strcat( &tempbuf[ 0 ], "procpar" );
#endif
	if (access( &tempbuf[ 0 ], 0 ) != 0)
	  return( NO_PROCPAR );
	if (access( &tempbuf[ 0 ], R_OK | W_OK ) != 0)
	  return( CANT_WRITE );

	tempbuf[ len ] = '\0';
#ifdef UNIX
	strcat( &tempbuf[ 0 ], "/fid" );
#else
	strcat( &tempbuf[ 0 ], "fid" );
#endif
	if (access( &tempbuf[ 0 ], 0 ) != 0)
	  return( NO_FID );
	if (access( &tempbuf[ 0 ], R_OK | W_OK ) != 0)
	  return( CANT_WRITE );

	return( 0 );
}

/*  Below are various symbols required by routines in magiclib.a or
    unmrlib.a which are needed by the ``xdcvt'' program.  They are
    defined here to prevent the ``ld'' program from bringing in the
    VNMR module which defines them, as the VNMR version includes
    references to SUN libraries which we would rather not load.    */

int	Dflag = 0;
int	Eflag = 0;

/*  The next two actually do something.  */

char *skymalloc(memsize)
int memsize;
{
	return( (char *) malloc(memsize) );
}

skyfree(memptr)
unsigned *memptr;
{
	return( free(memptr) );
}

void unsetMagicVar(addr)
int addr;
{
}

/*  Under normal operation the window routines below should not
    be called.  At this time each routine just announces itself.  */

WerrprintfWithPos()
{
	printf( "WerrprintfWithPos called\n" );
}

Wscrprintf()
{
	printf( "Wscrprintf called\n" );
}

Wprintfpos()
{
	printf( "Wprintfpos called\n" );
}

Werrprintf()
{
	printf( "Werrprintf called\n" );
}

Winfoprintf()
{
	printf( "Winfoprintf called\n" );
}
