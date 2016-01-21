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

/*  This version is compatible with VMS.  The presence of UNIX in
    the names of variables is now due simply to historical reasons.
    The subroutines UNIX_FSPLIT and UNIX_DIRSEARCH perform the
    apprpriate functions under VMS.

    Two versions of `is_ext', one for UNIX, one for VMS.		*/

#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>

#ifdef UNIX
#include  <sys/types.h>
#include  <fcntl.h>
#include  <dirent.h>
#include  <sys/stat.h>
#include  <sys/file.h>
#else
#include  <file.h>
#include  <stat.h>
#include  "dirent.h"
#include  "unix_io.h"
#endif

#ifdef VAX
#define  ZERO	1
#define  ONE	0
#else
#define  ZERO	0
#define  ONE	1
#endif

#define  DIRECTORY_ID	1
#define  DATA_ID	2
#define  PROG_ID	3
#define  ARRAY_ID	4
#define  MAXVXRDIRSIZ	432
#define  MNENTRYLEN	36
#define  BLOCKSIZE	512

struct fcb {
	char		i1[ 2 ];
	short		i2;
	short		i3;
	short		i4;
};

struct mnentry {
	char		n[ 6 ];
	struct fcb	v;
};

struct mnempage {
	short		next, num;
	struct mnentry	mnemarray[ MNENTRYLEN ];
	short		spare1, spare2;
};

/*  End of data structure templates; start of static memory allocation.  */

static int	verbose;
static int	VXRfd;
static int	VXRfsize;
static int	numIsCertain;
static int	numVXRentries;
static int	numVXRdirblocks;
static int	spacelist;
static short	entryBitMask[ MAXVXRDIRSIZ/16 ];
static char	VXRpname[ 256 ];
static char	VXRext[ 20 ];
static char	unixDir[ 8 ];
static char	vxrdir[ BLOCKSIZE * MAXVXRDIRSIZ/MNENTRYLEN ];
static char	vxrbuf[ BLOCKSIZE ];

main( argc, argv )
int argc;
char *argv[];
{
	int		ival;

#ifndef UNIX
	vf_init();
#endif
	if (argc < 2) {
		printf( "Supply a file name\n" );
		exit();
	}

	verbose = 131071;
	spacelist = (1 << DIRECTORY_ID) | (1 << DATA_ID) |
		    (1 << PROG_ID) | (1 << ARRAY_ID);

	if (verifyVxrlib( argv[ 1 ] ) != 0) exit( 2 );
	if (numVXRentries < 1) {
		printf( "File contains no VXR entries!\n" );
		exit();
	}

	if (setupUnixDir() != 0) exit( 2 );

	loadVXRfiles();
}

/*  Rename current file, if necessary.
    Create directory to receive VXR entries.	*/

setupUnixDir()
{
	char		*dptr, uext[ 20 ], newDir[ 12 ], newName[ 30 ];
	int		inCurDir, mustRename, subDirExists;

/*  No need to rename the VXR file on VMS, as the subdirectory
    will be named something like FIDLIB.DIR, while the VXR file
    will be named FIDLIB.36.

    The VMS version does address the issue of the subdirectory
    already existing.  The error is detected at one point and
    reported later, after we have built the subdirectory name
    in the tree syntax of Files-11.				*/

	subDirExists = 0;

#ifdef UNIX
	inCurDir = (strrchr( &VXRpname[ 0 ], '/' ) == NULL);
	mustRename = inCurDir && strlen( &VXRext[ 0 ] ) == 0 &&
		strlen( &VXRpname[ 0 ] ) <= 6;
	if (mustRename) {
		strcpy( &newName[ 0 ], &VXRpname[ 0 ] );
		make_ext( &uext[ 0 ], DIRECTORY_ID, numVXRentries );
		strcat( &newName[ 0 ], &uext[ 0 ] );
		printf( "Renaming '%s' to '%s'\n",
			&VXRpname[ 0 ], &newName[ 0 ]
		);
		if (rename( &VXRpname[ 0 ], &newName[ 0 ] ) != 0)
		{
			perror( "Error renaming VXR file" );
			return( 1 );
		}
		strcpy( &VXRpname[ 0 ], &newName[ 0 ] );
	}
#else
	strncpy( &newDir[ 0 ], &VXRpname[ 0 ], 6 );
	newDir[ 6 ] = '\0';
	strcat( &newDir[ 0 ], ".dir" );
	if (access( &newDir[ 0 ], 0 ) == 0)
	  subDirExists = 131071;
#endif

/*  Now create the UNIX directory  */

#ifdef UNIX
	strncpy( &newDir[ 0 ], &VXRpname[ 0 ], 6 );
	newDir[ 6 ] = '\0';
	if ( (dptr = strrchr( &newDir[ 0 ], '.')) != NULL ) *dptr = '\0';
#else
	strcpy( &newDir[ 0 ], "[." );
	strncpy( &newDir[ 2 ], &VXRpname[ 0 ], 6 );
	newDir[ 8 ] = '\0';
	if ( (dptr = strrchr( &newDir[ 2 ], '.')) != NULL ) *dptr = '\0';
	strcat( &newDir[ 0 ], "]" );
#endif

	if (subDirExists) {
		printf( "Error, subdirectory %s exists\n", &newDir[ 0 ] );
		printf( "You must remove it before continuing\n" );
		exit();
	}

	if (mkdir( &newDir[ 0 ], 0755 ) != 0) {
		perror( "Error creating subdirectory" );
		return( 1 );
	}
	strcpy( &unixDir[ 0 ], &newDir[ 0 ] );
	return( 0 );
}

loadVXRfiles()
{
	char		*cptr, *uptr, tchar, unixExt[ 10 ], unixName[ 30 ],
			 cposName[ 8 ];
	int		bitndx, bitnum, fd, i1mask, in, ip, iter, jter, udl;
	struct fcb	*fcbptr;
	struct mnempage	*bptr;

/*  Set up the directory part of the UNIX file name.  It never changes.  */

	udl = strlen( &unixDir[ 0 ] );
#ifdef UNIX
	strcpy( &unixName[ 0 ], &unixDir[ 0 ] );
	unixName[ udl ] = '/';
	udl++;
	unixName[ udl ] = '\0';
#else
	strcpy( &unixName[ 0 ], &unixDir[ 0 ] );
#endif

	bptr = (struct mnempage *) &vxrdir[ 0 ];
	for (iter = 0; iter < numVXRentries; iter++)
	{
		bitnum = iter % 16;
		bitndx = iter / 16;
		if ( ((1 << bitnum) & entryBitMask[ bitndx ]) == 0 ) continue;

		in = iter % MNENTRYLEN;
		ip = iter / MNENTRYLEN;

		fcbptr = &( (bptr+ip)->mnemarray[ in ].v );
		cptr = (bptr+ip)->mnemarray[ in ].n;
		cpos_to_unix( &cposName[ 0 ], cptr, 6 );
		cptr = &cposName[ 0 ];
		uptr = &unixName[ udl ];
		for (jter = 0; jter < 6; jter++)
		{
			tchar = *(cptr++) & 0x7f;
			if (tchar < '0') break;
			else if (tchar >= 'A' && tchar <= 'Z') tchar |= 0x20;
			*(uptr++) = tchar;
		}
		*uptr = '\0';
		make_ext( &unixExt[ 0 ], fcbptr->i1[ ZERO ] & 0x7f, fcbptr->i2 );
		strcat( uptr, &unixExt[ 0 ] );

		if (loadOneFile( &unixName[ 0 ], fcbptr )) return;
	}
}

int loadOneFile( uname, fcbptr )
char *uname;
struct fcb *fcbptr;
{
	char		*eptr, errMsg[ 80 ];
	int		fd, iter, spos;

/*  Verify that Robert hasn't f****d up.  */

	if (fcbptr->i3 < 1 || fcbptr->i4 < 1)
	{
#ifdef UNIX
		eptr = strrchr( uname, '/' );
#else
		eptr = strrchr( uname, ']' );
#endif
		if (eptr == NULL) eptr = uname;
		printf( "Programming error, entry %s has an invalid FCB\n",
			eptr
		);
		return( 1 );
	}
	if (verbose)
	 printf( "Loading %s, %d blocks\n", uname, fcbptr->i4 );

	spos = fcbptr->i3 * BLOCKSIZE;
	if (lseek( VXRfd, spos, 0 ) != spos)
	{
		perror( "seek error with VXR file" );
		return( 1 );
	}

	fd = open( uname, O_CREAT | O_WRONLY | O_TRUNC, 0644 );
	if (fd < 0)
	{
		sprintf( &errMsg[ 0 ], "error creating %s", uname );
		perror( &errMsg[ 0 ] );
		return( 1 );
	}

	for (iter = 0; iter < fcbptr->i4; iter++)
	{
		if (read( VXRfd, &vxrbuf[ 0 ], BLOCKSIZE ) != BLOCKSIZE)
		{
			sprintf( &errMsg[ 0 ],
	    "error reading block %d from VXR file", 1+iter+fcbptr->i3
			);
			perror( &errMsg[ 0 ] );
			return( 1 );
		}
		if (write( fd, &vxrbuf[ 0 ], BLOCKSIZE ) != BLOCKSIZE)
		{
			sprintf( &errMsg[ 0 ],
	    "error writing block %d to %s", 1+iter, uname
			);
			perror( &errMsg[ 0 ] );
			return( 1 );
		}
	}

	close( fd );
	return( 0 );
}

int verifyVxrlib( sname )
char *sname;
{
	char		*bptr, fext[ 20 ], pname[ 256 ];
	short		i1, i2;
	int		fd, iter, ival, nblocks, nchk, reject_it, startnum;

/*  Use find1file() to find the file implied by the argument.  */

	if (strlen( sname ) < 1) {
		printf( "Null file name not allowed\n" );
		return( 1 );
	}
	if ((fd = find1file( sname, &pname[ 0 ], 254 )) < 0)
	{
		if (fd == -3)
		 printf( "File cannot be found\n" );
		else if (fd == -5)
		 printf( "Too many files match\n" );
		else
		 printf( "You made a big mistake, fd = %d\n", fd );

		return( 1 );
	}
	else VXRfd = fd;

	is_ext( &pname[ 0 ], &fext[ 0 ], 20 );
	startnum = verifyExtension( &fext[ 0 ] );
	if (startnum < 0)
	{
#ifdef UNIX
		printf( "UNIX file name implies an invalid VXR file type\n" );
#else
		printf( "VMS file name implies an invalid VXR file type\n" );
#endif
		return( 1 );
	}
	else numIsCertain = (startnum > 0);

/*  At this time, insist on numIsCertain being TRUE.  */

	if (numIsCertain == 0)
	{
		printf(
	    "File name does not imply it represents a VXR directory\n"
		);
		return( 1 );
	}

	VXRfsize = get_fsize_fd( fd );

/*	if (startnum == 0)
	 startnum = (VXRfsize > 8) ? MAXVXRDIRSIZ :
			(VXRfsize-1) * MNENTRYLEN;  */
	if (startnum < 1 || startnum > MAXVXRDIRSIZ) {
		printf( "Invalid number of entries (%d) in VXR directory\n",
			startnum
		);
		return( 1 );
	}
	if ( (VXRfsize-1) * MNENTRYLEN < startnum ) {
		printf( "UNIX file too small for %d entries\n", startnum );
		return( 1 );
	}

	numVXRentries = 0;
	nblocks = (startnum+MNENTRYLEN-1)/MNENTRYLEN;
	if (loadVXRdir( nblocks )) return( 1 );
	bptr = &vxrdir[ 0 ];
	for (iter = 0; iter < nblocks; iter++)
	{
		if (iter == nblocks-1) nchk = startnum - iter*MNENTRYLEN;
		else		      nchk = MNENTRYLEN;
		ival = verifyMnempage( bptr, nchk, iter );
		numVXRentries += ival;
		if (ival != nchk)
		 if (numIsCertain)
		 {
			printf( "Invalid VXR entry found in block %d\n",
				iter+1
			);
			return( 1 );
		 }
		 else break;

		bptr += BLOCKSIZE;
	}

	numVXRdirblocks = (numVXRentries+MNENTRYLEN-1)/MNENTRYLEN;
	strcpy( &VXRpname[ 0 ], &pname[ 0 ] );
	strcpy( &VXRext[ 0 ], &fext[ 0 ] );
	return( 0 );
}

int verifyMnempage( mnemptr, num_check, p_num )
struct mnempage *mnemptr;
int num_check;				/*  Number of entries to check  */
int p_num;				/*  Page number in VXR directory  */
{
	char		tchr, *tptr;
	int		bitnum, bitndx, i1mask, i1val, iter, jter, limit;
	struct fcb	*fcbptr;

	if (num_check < 1) return( -1 );			/*  Error  */
	limit = (num_check > MNENTRYLEN) ? MNENTRYLEN : num_check;
	for (iter=0; iter < limit; iter++) {
		tptr = mnemptr->mnemarray[ iter ].n;
		for (jter = 0; jter < 6; jter++)
		 if (( *(tptr+jter) & 0x80 ) == 0) return( iter );

/*  Check the I1 value.  Skip entry if not spacefilling.  */

		fcbptr = &mnemptr->mnemarray[ iter ].v;
		i1val = fcbptr->i1[ ZERO ] & 0x7f;
		if (i1val < 0 || i1val > 31) continue;
		i1mask = (1 << i1val);
		if ( (i1mask & spacelist) == 0 ) continue;

/*  Current entry is spacefilling.  Verify I3 and I4 are valid.  */

		if (fcbptr->i3 < 1) return( iter );
		if (fcbptr->i4 < 1) return( iter );
		if (fcbptr->i3 + fcbptr->i4 > VXRfsize) return( iter );

/*  Mark this as a valid entry by turning on its bit in the entry bit mask  */

		bitnum = p_num*MNENTRYLEN+iter;
		bitndx = bitnum / 16;
		bitnum = bitnum % 16;
		entryBitMask[ bitndx ] |= (1 << bitnum);
	}
	return( limit );
}

/*  Verifies the UNIX file extension.
    Returns -1 if unacceptable.
    Returns 0 if acceptable, but the number of directory entries
cannot be determined from the file extension.
    Otherwise, the file extension is the decimal representation of
the number of VXR entries and the function returns this value.  Note
that a negative value will not be returned in I2 by EXTR_I1_I2().	*/

int verifyExtension( extp )
char *extp;
{
	short		i1, i2;
	int		iter;

/*  Extract the extension; convert to I1 and I2 values  */

	extr_i1_i2( extp, &i1, &i2 );
	if (i1 == ARRAY_ID || i1 == PROG_ID) return( -1 );
	else if (i1 == DIRECTORY_ID) return( i2 );
	else if (strlen( &extp[ 0 ] ) < 3) return( 0 );

/*  DATA_ID is used as the default VXR file type.  We want to reject
    the file if its extension starts with ".DAT" or some variation
    with lower-case letters.						*/

	else {
		for (iter = 0; iter < 3; iter++)
		 if ( (*(extp++) & ~0x20) != "DAT"[ iter ] )
		 return( 0 );
		return( -1 );
	}
}

int loadVXRdir( nblks )
int nblks;
{
	char		*bptr;
	int		iter;

	lseek( VXRfd, 0, 0 );
	bptr = &vxrdir[ 0 ];
	for (iter = 0; iter < nblks; iter++)
	{
		if (read( VXRfd, bptr, BLOCKSIZE ) != BLOCKSIZE)
		{
			printf( "Error reading block %d in VXR file\n",
				iter+1
			);
			return( 1 );
		}
		bptr += BLOCKSIZE;
	}
	return( 0 );
}

/*  End of decompose program.  */

/*  Start of fileops subroutines.  File operations on UNIX.  */

#define  DIRECTORY_ID	1
#define  DATA_ID	2
#define  PROG_ID	3
#define  ARRAY_ID	4

/*  New feature -  arrays are supported.  Extension is ".annn" where
    "nnn" is the number of entries in the array (the I2 value). 	*/

make_ext( ext_ptr, i1_val, i2_val )
char *ext_ptr;
int i1_val, i2_val;
{
	*ext_ptr = '\000';
	if (i1_val == PROG_ID)
	 strcat( ext_ptr, ".prg" );
	else if (i1_val == DATA_ID)
	 strcat( ext_ptr, ".dat" );
	else if (i1_val == ARRAY_ID) {
		if (i2_val < 1) return;
		sprintf( ext_ptr, ".a%d", i2_val );
	}
	else {
		if (i2_val < 1) return;
		sprintf( ext_ptr, ".%d", i2_val );
	}
}

/*  Following procedure finds exactly 1 file, using this procedure.

    The input name is searched for.  If successful, that file is opened.
    Otherwise, the wildcard specification ".*" is appended and a file
    matching that specification is searched for. Only ONE file can match
    the latter specification; if two or more match, an error is reported.  */

int find1file( input_pname, final_pname, final_plen )
char *input_pname, *final_pname;
int final_plen;
{
	char		local_pname[ 256 ];
	int		input_plen, ival, tfd, want_pname;

	if ( (input_plen = strlen( input_pname )) < 1 ) return( -3 );
	else if ( input_plen > 255 ) 			return( -1 );

	want_pname = (final_pname != NULL && final_plen > 0);

/*  Try to open the file.  If successful, return the file descriptor.  */

	tfd = open( input_pname, 0 );
	if (tfd >= 0) {
		if (want_pname)
		 strlcpy( input_pname, final_pname, final_plen );
		return( tfd );
	}

/*  If the file was not found and the path name has an extension,
    return error code indicating not found.				*/

	if (is_ext( input_pname, NULL, 0 )) return( -3 );

/*  If no extension, append ".*" to the name and use UNIX_SEARCH.  */

	strlcpy( input_pname, &local_pname[ 0 ], 256 );
	strcat( &local_pname[ input_plen ], ".*" );
	ival = unix_search( &local_pname[ 0 ], final_pname, final_plen );
	if (ival == 0)
	 return( open( final_pname, 0 ) );
	else
	 return( ival );
}

/*  Searches for a UNIX file.  If successful, returns 0.  Accepts ordinary
    UNIX path names and wild card specifications of the form "X.*", where
    X is a valid UNIX file name.

    Error returns:
	-1		Original path name too long
	-2 		Invalid wild card operation, e. g. "*.c"
	-3		File not found
	-4		Directory not found (wildcard operations only)
	-5		More than 1 file matches the wildcard spec	*/

int unix_search( input_ptr, output_ptr, output_len )
char *input_ptr;
char *output_ptr;
int output_len;
{
	char		dname[ 256 ], fname[ 80 ], res_name[ 80 ], *tptr;
	int		dlen, flen, ilen, ival, rval, star_present, tfd,
			want_output;
	DIR		*dstream_ptr;

	ilen = strlen( input_ptr );
	if (ilen < 1) return( -3 );		/*  No null file names  */
	else if (ilen > 255) return( -1 );

	want_output = (output_ptr != NULL && output_len > 0);
	ival = unix_fsplit( input_ptr, &dname[ 0 ], 256, &fname[ 0 ], 80 );

/*  Look for the wildcard charater.  If found, it must be the last
    character in the string, and the previous character must be a
    dot.  If either is not true, return an error condition.		*/

	flen = strlen( tptr = &fname[ 0 ] );
	star_present = 0;
	while (*tptr) {
		if (*tptr == '*') {
			if ( *(tptr+1) != '\000' )	return( -2 );
			else if ( tptr == &fname[ 0 ] ) return( -2 );
			else if ( *(tptr-1) != '.' )	return( -2 );
			else star_present = 131071;
		}

/*  If the wildcard character was found, the next character in the string
    has to be the null character.  Thus incrementing the pointer will
    cause the WHILE loop to exit, just by checking the current character.  */
	
		tptr++;
	}

/*  If no wildcard character was found, complete the search by attempting
    to open the original file specification.				*/

	if ( !star_present ) {
		tfd = open( input_ptr, 0 );
		if (tfd < 0) return( -3 );

		close( tfd );
		if (want_output)
		 strlcpy( input_ptr, output_ptr, output_len );
		return( 0 );
	}

/*  Wildcard found, presumably valid.

    The UNIX version needs a dot appended to the end of the
    directory so as to search the correct directory.		*/

#ifdef UNIX
	strcat( &dname[ 0 ], "." );
#endif
	dstream_ptr = opendir( &dname[ 0 ] );
	if (dstream_ptr == NULL) return( -4 );

/*  Search the directory in a separate routine.  */

	rval = unix_dirsearch( dstream_ptr, &fname[ 0 ], &res_name[ 0 ], 80 );
	if (rval != 0 || !want_output) return( rval );

/*  Before copying the directory name, erase the "." we added earlier.  */

	if ( (dlen = strlen( &dname[ 0 ] )) > 1 ) {
		dname[ --dlen ] = '\000';
		strlcpy( &dname[ 0 ], output_ptr, output_len );
		output_len = output_len - dlen;
		if (output_len < 1) return( rval );
		else output_ptr += dlen;
	}

	strlcpy( &res_name[ 0 ], output_ptr, output_len );
	return( 0 );
}

int unix_dirsearch( ds_ptr, fs_ptr, fn_ptr, fn_len )
DIR *ds_ptr;
char *fs_ptr;
char *fn_ptr;
int fn_len;
{
	char		*iptr, *cptr, safe_place[ 256 ];
	int		fs_len, matched, ret_val, star_present, want_match;
	struct dirent	*cur_entry;

	fs_len = strlen( iptr = fs_ptr );
	if (fs_len < 1) return( -3 );
	else if (fs_len > 255) return( -1 );

/*  Yes I know the following has most likely already been done.
    In fact, I took this code from UNIX_SEARCH, above.  However,
    what follows is less restrictive, and only demands that the
    wild card character be the last one in the file specification.

    Thus we can call UNIX_DIRSEARCH from routines other than UNIX_SEARCH.

    It's called "defensive programming".				*/

	star_present = 0;
	while (*iptr) {
		if (*iptr == '*') {
			if ( *(iptr+1) != '\000' ) return( -2 );
			else star_present = 131071;
		}
		iptr++;
	}

	want_match = (fn_ptr != NULL && fn_len > 0);
	ret_val = -3;					/*  not found  */
	do {
		cur_entry = readdir( ds_ptr );
		if (cur_entry == NULL) break;

		iptr = fs_ptr;
		cptr = &cur_entry->d_name[ 0 ];

/*  Compare the current entry with the input specification.  The
    instructions below assume there is only one wild card character
    in the specification.

    Remember that we verified the file specification.  Under VMS, if
    the two characters are not the same, check for upper vs. lower
    case letters.							*/

		matched = 0;
		do {
			if (*iptr == '*')
			 matched = 131071;
			else if (*iptr == '\000' && *cptr == '\000')
			 matched = 131071;
#ifdef UNIX
			else if (*iptr != *cptr) break;
#else
			else if ( (*iptr != *cptr) &&
				(*iptr & ~0x20) != *cptr )  break;
#endif
		} while (*(iptr++) && *(cptr++) && !matched);	

/*  More than one match?  Return -5 if so.  */

		if (matched) {
			if (ret_val == -3) ret_val = 0;
			else		   ret_val = -5;
			if (ret_val == 0 && want_match)
			 strlcpy(
				&cur_entry->d_name[ 0 ],
				&safe_place[ 0 ], 256
			 );
		}

/*  CUR_ENTRY == NULL was checked earlier...  */

	} while (ret_val != -5);

	closedir( ds_ptr );
	if (ret_val == 0 && want_match)
	 strlcpy( &safe_place[ 0 ], fn_ptr, fn_len );

	return( ret_val );
}

/*  Take the UNIX path name in PNAME and splits it into the directory
    and file names.  Length of the latter strings is governed by DLEN
    and FLEN.  Thus:

	Input:      /usr2/nmr/robert/test01.c
        directory:  /usr2/nmr/robert/
        file name:  test01.c

    This routine follows the UNIX convention by returning 0 if successful
    and a negative value if an error occurs.  Either DNAME or FNAME can
    be NULL, provided the corrsponding length value is 0.

    VMS version:
	Input:      DUA0:[ROBERT]TEST01.C
        directory:  DUA0:[ROBERT]
        file name:  TEST01.C
									*/

int unix_fsplit( pname, dname, dlen, fname, flen )
char *pname;
char *dname;
int dlen;
char *fname;
int flen;
{
	char		*tptr;
	int		finished, index, plen;

	plen = strlen( pname );
	if (plen < 1) return( 0 );
	else if (plen > 255) return( -1 );		/*  Too long !!  */

/*  Locate a slash, if it is present.  */

	tptr = pname + (plen-1);
	finished = 0;
	while (tptr >= pname && !finished)
#ifdef UNIX
	 if (*tptr == '/') finished = 131071;
#else
	 if (*tptr == ']' || *tptr == ':') finished = 131071;
#endif
	 else tptr--;

/*  Copy the directory path, including the slash.  If no slash is present,
    a null character is placed in the directory name.			*/

	tptr++;
	if (dname != NULL && dlen > 0)			/*  I promised !!  */
	 if (finished) {
		index = 1;
		while (index < dlen && pname < tptr) {
			*(dname++) = *(pname++);
			index++;
		}
		*dname = '\000';
	 }
	 else *dname = '\000';

/*  Copy the file name.  */

	if (fname != NULL && flen > 0)
	 strlcpy( tptr, fname, flen );

	return( 0 );
}

/*  UNIX version has to account for distinction between upper and lower
    case characters.  "prg", "Prg", "PRG", etc are all accepted as
    program files.  Avoid inconsistancy by skipping the "." if it is
    the 1st character in the extension.

    Note:  The SUN version of "toupper" is completely useless and I am
           surprised it was even released in its current form.		*/

extr_i1_i2( ext_ptr, i1_addr, i2_addr )
char *ext_ptr;
short *i1_addr, *i2_addr;
{
	char		cur_char;
	int		array_flag, i1_val, iter, ival;

	if ( *ext_ptr == '.' ) ext_ptr++;

	cur_char = *ext_ptr;
	if (cur_char >= 'a' && cur_char <= 'z') cur_char = cur_char - 0x20;
	if ( 'A' == cur_char ) {
		array_flag = 131071;
		ext_ptr++;
	}
	else
	 array_flag = 0;

	if (isdigit( *ext_ptr )) {
		ival = 0;
		do
		 ival = ival*10 + (int) (*(ext_ptr++) - '0');
		while (isdigit( *ext_ptr ) && ( *ext_ptr ));

		*i1_addr = (array_flag) ? ARRAY_ID : DIRECTORY_ID;
		*i2_addr = (short) ival;
	}

/*  If we make it here and ARRAY_FLAG was set, the extension is not
    supported and defaults to DATA.					*/

	else {
		*i2_addr = 0;
		if (array_flag) {
			*i1_addr = DATA_ID;
			return;
		}
		i1_val = PROG_ID;
		for (iter = 0; iter < 3; iter++) {
			cur_char = *ext_ptr;
			if (cur_char >= 'a' && cur_char <= 'z')
			 cur_char = cur_char - 0x20;
			if ( "PRG"[ iter ] == cur_char )
		 	 ext_ptr++;
			else {
				i1_val = DATA_ID;
				break;
			}
		}

		*i1_addr = (short) i1_val;
	}
}

int is_ext( fil_nptr, ext_ptr, ext_len )
char *fil_nptr, *ext_ptr;
int ext_len;
{
#ifdef UNIX

/*  This version is for UNIX only  (really!!)

    The VMS version stops at the semicolon, as the version number
    is not part of the file extenstion. 

    (FIDLIB.36;1  => extension is 36)			*/

	char		*t_ptr;
	int		fil_nlen, finished, index;

/*  If the file name is not NULL, then T_PTR is set to the address of the
    the last non-null character in the name.  The routine then uses this
    pointer to scan the file name backwards until a '/' or a '.' is found,
    or until the process reaches the beginning of the string.		*/

	if ( (fil_nlen = strlen( fil_nptr )) > 0) {
		t_ptr = fil_nptr + (fil_nlen-1);
		finished = 0;
		while ( t_ptr >= fil_nptr && !finished )
		 if (*t_ptr == '/' || *t_ptr == '.') finished = 131071;
		 else t_ptr--;

/*  If a '.' was found, copy all the trailing characters in the file
    name to the extension, but do not overwrite the end of this buffer.

    Note:  "test01." has an extension; "test01" does not.		*/

		if (finished)
		 if ( *t_ptr == '.' ) {
			if (ext_len < 1 || ext_ptr == NULL) return( 1 );
			t_ptr++;		/*  Skip the dot  */
			index = 1;		/*  Leave room for null  */
			while ( index < ext_len && (*t_ptr) ) {
				*(ext_ptr++) = *(t_ptr++);
				index++;
			}
			*ext_ptr = '\000';	/* Terminate extension  */
			return( 1 );
		 }
	}

/*  Otherwise, place a null character in the extension buffer  */

	if (ext_len > 0) *ext_ptr = '\000';
	return( 0 );

#else
	int	fil_nlen, finished, index;
	char	*t_ptr;

	if ( (fil_nlen = strlen( fil_nptr )) > 0 ) {
		t_ptr = fil_nptr + (fil_nlen-1);
		finished = 0;
		while (t_ptr >= fil_nptr && !finished)
		 if (*t_ptr == ']' || *t_ptr == ':' || *t_ptr == '.') finished = 1;
		 else t_ptr--;
		if (finished)
		 if (*t_ptr == '.') {
			t_ptr++;			/* Skip the dot */
			index = 1;			/* Leave room for null */
			finished = 0;
			while ( (index < ext_len) && (*t_ptr) && (!finished) )
			 if (*t_ptr != '.' && *t_ptr != ';') {
				*(ext_ptr++) = *(t_ptr++);
				index++;
			 }
			 else finished = 1;
			*ext_ptr = '\000';
			return( 1 );
		}
	}
	if (ext_ptr)
	 if(ext_len > 0)
	  *ext_ptr = '\000';				/* Return null string */
	return( 0 );
#endif
}

#define  BYTES_PER_BLOCK	512

int get_fsize_fd( fd )
int fd;
{
	int		ival;
	struct stat	unix_fab;

	ival = fstat( fd, &unix_fab );
	if (ival != 0) {
		perror( "FSTAT call failed" );
		exit( 3 );
	}

	return( (unix_fab.st_size+BYTES_PER_BLOCK-1) >> 9 );
}

/*  STRLCPY -  String copy, limited.

    This subroutine is like STRNCPY, except that copying stops when
    either the output limit is reached or the entire input string is
    copied, whichever occurs first.  Since a a NUL character is always
    appended to the end of the output, a maximum of LEN-1 non-NUL
    characters can be copied.  The output is NOT padded with additional
    NUL characters, nor is any value returned.				*/

strlcpy( p1, p2, l )
char *p1, *p2;
int l;
{
	int		index;

/*  Defensive programming...  */

	if (p1 == NULL) return;
	if (p2 == NULL) return;
	if (l < 1) return;

	index = 1;				    /*  Leave room for NUL  */
	while (index < l && *p1) {
		*(p2++) = *(p1++);
		index++;
	}
	*p2 = '\000';
}

/*  UNIX provides rename() as part of the system library  */

#ifndef UNIX
int rename( oldname, newname )
char *oldname;
char *newname;
{
	char	opsys_cmd[ 520 ];

	sprintf( &opsys_cmd[ 0 ], "rename %s %s", oldname, newname );
	system( &opsys_cmd[ 0 ] );
	return( 0 );
}
#endif

cpos_to_unix( target, source, length )
char *target;
char *source;
int length;
{
#ifdef VAX
	int	iter;
	
	for (iter = 0; iter < length; iter += 2) {
		*target++ = *(source+ZERO) & 0x7f;
		*target++ = *(source+ONE)  & 0x7f;
		source += 2;
	}
	*target = '\0';
#else
	strncpy( target, source, length );
	*(target+length) = '\0';
#endif
}
