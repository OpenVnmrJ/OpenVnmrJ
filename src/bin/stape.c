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

	/*   Product version of streaming tape software.  */
      /* stape.c - used with tape_main.c to assemble 'tape' */

#include  <stdio.h>
#include  <sys/types.h>
#include  <sys/file.h>

#define  MAXVXRDIRSIZ	432
#define  MNENTRYLEN	36
#define  STAPE_BUFSIZE	512
#define  DIRECT_ID	1
#define  DATA_ID	2
#define  PROG_ID	3
#define  ARRAY_ID	4
#define  UNIT_ID	8

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

struct stape_header {
	short		softnum;
	short		htype;
	short		date[ 3 ];
	char		fname[ 6 ];
	struct fcb	ffcb;
	int		size;
	int		hlad;
	char		txt[ 400 ];
};

/*  End of data structure definitions.  Start of memory allocations.

    STAPE_DIR contains the VXR directory, as read from the tape.
    STAPE_BUF contains the current streaming tape record, including the
    tape header.
    SPACELIST is a bitmask to isolate spacefilling files.
    NSTAPE_REC keeps track of the number of records that have been read.
    Used with the I3 value in the VXR directory, it allows individual
    VXR files to be located.						*/

static char		stape_dir[ MAXVXRDIRSIZ/MNENTRYLEN * STAPE_BUFSIZE ];
static char		stape_buf[ STAPE_BUFSIZE ];
static int		nstape_rec;

/*  SPACELIST is a bitmask to isolate spacefilling files.  */

static int		spacelist;

/*  If ORGANIZE_DIR is non-zero, then sort the VXR directory by I3 value  */

static int		organize_dir;

/*  NUM_ENTRIES is the total number of VXR entries.
    NS_ENTRIES is the number of spacefilling entries.			*/

static int		num_entries;
static int		ns_entries;


/*  A streaming tape, as written on the VXR system, contains only two files.
    The first file is expected to be 512 bytes long and is the streaming tape
    header.  The second file contains the information copied from the VXR
    system.  The first few blocks are expected to be a VXR directory, as
    specified in the streaming tape header.  The size of this file is also
    determined from the header using the SIZE field.			*/

/*  Main routine for streaming tapes.  */

stape_main( argc, argv )
int argc;
char *argv[];
{

/*  Define the bit map for spacefilling VXR files  */

	spacelist = (1 << DIRECT_ID) | (1 << DATA_ID) |
		    (1 << PROG_ID) | (1 << ARRAY_ID);
	nstape_rec = 0;

/*  Verify the command.  Provide help if indicated.  The STAPE_HELP()
    routine exits when complete; thus it does not return.		*/

	if (argc < 1) stape_help();
	if (strcmp( argv[ 0 ], "help" ) == 0 || strcmp( argv[ 0 ], "?" ) == 0)
	 stape_help();

	stape_cmd_verify( argc, argv );

/*  Now verify the streaming tape header.  */

	stape_header_verify();

/*  Load the VXR directory.  If necessary, sort the directory.  */

	load_vxrdir();
	if (organize_dir) ns_entries = sort_vxrdir();

/*  Process command.  */

	if (strcmp( argv[ 0 ], "dir" ) == 0 || strcmp( argv[ 0 ], "cat" ) == 0)
	 stape_cat();
	else if (strcmp( argv[ 0 ], "read" ) == 0)
	 stape_load( argc-1, argv+1 );
}

stape_help()
{
    printf( "You have chosen the streaming tape interface\n\n" );
    printf( "Options include:\n" );
    printf( "    cat                   display tape header and directory\n" );
    printf( "    read <file names>     extract named files from the tape\n\n" );
    printf( "You can supply a wildcard by using the '*' character.  Don't\n" );
    printf( "forget to used double quotes to prevent intervention by the\n" );
    printf( "UNIX shell.\n\n" );
    printf( "Files are always created in your current working directory.\n" );
    printf( "If a file already exists, it will NOT be overwritten\n\n" );

    exit();
}

/*  Verify the streaming tape command.  If valid, check for the necessity
    of organizing the VXR directory, once it has been loaded.		*/

stape_cmd_verify( argc, argv )
int argc;
char *argv[];
{
	if (argc < 1) {
		printf( "stape_cmd_verify called incorrectly\n" );
		exit();
	}

/*  Usually it is necessary to sort the VXR directory.  */

	organize_dir = 131071;
	if (strcmp( argv[ 0 ], "read" ) == 0) {
		if (argc < 2) {
			printf(
	    "You must specify at least one file name with the read command\n"
			);
			exit();
		}
		else if (argc == 2)
		 if (
			(rindex( argv[ 1 ], '*' ) == NULL) &&
			(rindex( argv[ 1 ], '%' ) == NULL) &&
			(rindex( argv[ 1 ], '#' ) == NULL)
		  )
		  organize_dir = 0;
	}
	else if (strcmp( argv[ 0 ], "cat" ) != 0 &&
		 strcmp( argv[ 0 ], "dir" ) != 0) {
		printf( "Invalid command for streaming tape interface\n" );
		exit();
	}
}

/*  The streaming tape device is first accessed by this routine.  */

stape_header_verify()
{
	int			etype, ival;
	struct stape_header	*hp;

	open_stape();				/*  Exits if failure  */
	rewind_stape();
	ival = stape_read( &stape_buf[ 0 ] );
	if (ival != STAPE_BUFSIZE) {
		perror( "Error reading initial streaming tape header" );
		exit();
	}

/*  The streaming tape header is not included in the count in NSTAPE_REC  */

	hp = (struct stape_header *) &stape_buf[ 0 ];

	if (hp->size < 1) {
		printf( "No information on the tape!\n" );
		exit();
	}
	etype = hp->ffcb.i1[ 0 ] & 0x7f;
	if ( etype != DIRECT_ID  && etype != UNIT_ID ) {
		printf( "Streaming tape must contain a VXR directory\n" );
		exit();
	}
	if (hp->size != hp->ffcb.i4) {
		printf( "Mismatch between size of tape and I4 value in FCB\n" );
		exit();
	}

	num_entries = hp->ffcb.i2;
	if (num_entries > MAXVXRDIRSIZ) {
		printf(
    "Streaming tape must have %d or fewer entries in its VXR directory\n",
			MAXVXRDIRSIZ
		);
		exit();
	}
	else if (num_entries < 1) {
		printf( "No entries in VXR directory\n" );
		exit();
	}
}

load_vxrdir()
{
	char		*cp;
	int		dsize, iter, ival;

	dsize = (num_entries+MNENTRYLEN-1)/MNENTRYLEN;
	skip_stape_file();
	cp = &stape_dir[ 0 ];

	for (iter = 0; iter < dsize; iter++) {
		ival = stape_read( cp );
		if (ival != STAPE_BUFSIZE) {
			printf( "Streaming tape read error at block %d\n",
				iter+1
			);
			perror( "unix error" );
			exit();
		}
		cp += ival;
	}

/*  Always remove the parity bit from the name of each VXR entry.  */

	cvt_names();
	nstape_rec = dsize;
}

static int	i3index[ MAXVXRDIRSIZ ];

int compress_index( ia, n )
int ia[];
int n;
{
	int		i2, iter, ival;

	i2 = 0;
	for (iter = 0; iter < n; iter++)
	 if ( (ival = ia[ iter ]) >= 0) {
		if (i2 < iter) ia[ i2 ] = ia[ iter ];
		i2++;
	 }

	return( i2 );
}

/*  Sorts the array IA[ * ], based on values in VA[ * ].

    This routine is deliberately loaded with traps, so that if
    either IA[ * ] or VA[ * ] have invalid values the process
    will terminate.  It is the task of the programmer to see
    that this is not so!					*/

sort_index( ia, va, n )
int ia[];
int va[];
int n;
{
	int		finished, in, iter, ival, jn, jter, jval;

	if (n < 1) return;				/*  Trivial  */

	finished = 131071;
	do
	 for (iter = 1; iter < n; iter++) {
		if ( (in = ia[ iter ]) < 0 ) {
			printf( "programmer error in sort, %d  %d\n",
				iter, in );
			exit();
		}
		jter = iter - 1;
		if ( (jn = ia[ jter ]) < 0 ) {
			printf( "programmer error in sort, %d  %d\n",
				jter, jn );
			exit();
		}

		if ( (ival = va[ in ]) < 1) {
			printf( "programmer error in sort, %d  %d  %d\n",
				iter, in, ival );
			exit();
		}
		if ( (jval = va[ jn ]) < 1) {
			printf( "programmer error in sort, %d  %d  %d\n",
				jter, jn, jval );
			exit();
		}

	/*  Now swap indices if values are out of order.  */

		if (ival < jval) {
			ia[ iter ] = jn;
			ia[ jter ] = in;
			finished = 0;
		}
	 }
	while ( !finished );
}

/*  Remove parity bit from each name in the VXR directory  */

cvt_names( )
{
	char		*cp;
	int		in, ip, iter, jter, n;
	struct mnempage *bp;

	bp = (struct mnempage *) &stape_dir[ 0 ];
	n = num_entries;

	in = 0;
	ip = 0;
	for (iter = 0; iter < n; iter++) {
		cp = &( (bp+ip)->mnemarray[ in ].n[ 0 ] );
		for (jter = 0; jter < 6; jter++) {
			*cp &= 0x7f;
			cp++;
		}

		if (++in >= MNENTRYLEN) {
			in = 0;
			ip++;
		}
	}
}

/*  This procedure determines if an FCB is that of a spacefilling entry
    In addition, it verifies that both I3 and I4 are greater than zero.	*/

int valid_fcb( fcbptr )
struct fcb *fcbptr;
{
	if ( (1 << fcbptr->i1[ 0 ]) & spacelist )
	 return( fcbptr->i3 > 0 && fcbptr->i4 > 0 );
	else
	 return( 0 );
}

/*  If you need those non-spacefilling entries, you will have to modify
    the initial loop and COMPRESS_INDEX appropriately.  Currently they
    are discarded.							*/

int sort_vxrdir( )
{
	char		*cp;
	int		fcb_index, ip, in, iter, jter, nentries, ns;
	int		i3value[ MAXVXRDIRSIZ ];
	struct fcb	*fcbptr;
	struct mnentry	*mnenptr;
	struct mnempage *bp;

	bp = (struct mnempage *) &stape_dir[ 0 ];
	nentries = num_entries;

	ip = 0;					/*  Page offset  */
	in = 0;					/*  Index into page  */
	for (iter = 0; iter < nentries; iter++) {
		fcbptr = &( (bp+ip)->mnemarray[ in ].v );
		if ( valid_fcb( fcbptr ) ) {
			i3index[ iter ] = iter;
			i3value[ iter ] = fcbptr->i3;
		}
		else {
			i3index[ iter ] = -1;
			i3value[ iter ] = -1;
		}
		in++;
		if (in >= MNENTRYLEN) {
			in = 0;
			ip++;
		}
	}

/*  NS is the number of spacefilling entries.  After COMPRESS_INDEX() has
    been called, the spacefilling entries should be at the beginning of
    I3INDEX[ * ]; if a hole is found now, it is a programming error.	*/

	ns = compress_index( &i3index[ 0 ], nentries );
	sort_index( &i3index[ 0 ], &i3value[ 0 ], ns );

	return( ns );
}

/*  This procedure documents itself  */

dump_header()
{
	char			tchr;
	int			iter;
	struct stape_header	*bp;

/*  Assumes that STAPE_BUF still holds the streaming tape header.  */

	bp = (struct stape_header *) &stape_buf[ 0 ];
	printf( "Software revision: %x\n", bp->softnum  );
	printf( "Date of storage: %d-%d-%d\n",
		bp->date[ 0 ], bp->date[ 1 ], bp->date[ 2 ]
	);
	printf( "Name of file: " );
	for (iter = 0; iter < 6; iter++)
	 printf( "%c", bp->fname[ iter ] & 0x7f );
	printf( "\n" );
	printf( "FCB of file:  %d  %d  %d\n",
		bp->ffcb.i2, bp->ffcb.i3, bp->ffcb.i4
	);
	printf( "Size of file: %d\n", bp->size );
	printf( "Comments:  " );
	iter = 0;
	while ( (tchr = bp->txt[ iter ] & 0x7f) && iter < 72 ) {
		printf( "%c", tchr );
		iter++;
	}
	printf( "\n" );
	printf( "\n" );
}

/*  List a sorted VXR directory.  Expects I3INDEX[ * ] to be filled.  */

static char	*entry_type[] = {
	"DR", "DA", "PR", "AR"
};

list_cat()
{
	char		*cp;
	int		e_type, fcb_index, in, ip, iter, jter, n;
	struct mnentry	*mnemptr;
	struct mnempage *bp;

	bp = (struct mnempage *) &stape_dir[ 0 ];
	n = ns_entries;

	for (iter = 0; iter < n; iter++) {
		if ( (fcb_index = i3index[ iter ]) < 0 ) {
			printf( "programmer error in stape_cat, %d  %d\n",
				iter, fcb_index );
			exit();
		}
		in = fcb_index % MNENTRYLEN;
		ip = fcb_index / MNENTRYLEN;
		mnemptr = &( (bp+ip)->mnemarray[ in ] );
		cp = mnemptr->n;

		for (jter = 0; jter < 6; jter++)
		 printf( "%c", *(cp++) );
		e_type = (unsigned char) (mnemptr->v.i1[ 0 ]) & 0x7f;
		
		printf( " %s", entry_type[ e_type-1 ] );
		printf( "  %-5d  %-5d", mnemptr->v.i2, mnemptr->v.i4 );
		if (iter % 3 == 2)
		 printf( "\n" );
		else printf( "     " );
	}
	printf( "\n" );
}

stape_cat()
{
	if ( !organize_dir ) {
		printf( "programmer error, ORGANIZE_DIR false with DIR\n" );
		exit();
	}

	dump_header();
	list_cat();
}

load_vxrfile( fptr, nptr )
struct fcb *fptr;
char *nptr;
{
	char		ext[ 10 ], final_name[ 20 ], tchr;
	int		fd, fsize, i1val, i2val, i3val, iter, ival;

	i3val = fptr->i3;
	if (nstape_rec > i3val) {
		printf( "Programming error:  I3 value: %d, have read %d\n",
			i3val, nstape_rec
		);
		exit();
	}
	i2val = fptr->i2;
	i1val = (int) (fptr->i1[ 0 ] & 0x7f);
	fsize = fptr->i4;

/*  Advance the tape to the start of the file.  You MUST keep track of the
    tape position with NSTAPE_REC.					*/

	while (nstape_rec < i3val)
	 if (stape_read( &stape_buf[ 0 ] ) != STAPE_BUFSIZE) {
		perror( "Error advancing streaming tape" );
		exit();
	 }
	 else nstape_rec++;

/*  Make the UNIX file name.  We start with the original VXR file name,
    which is six characters or less.  Add an extension, based on the I1
    and I2 values to complete the name.  Use access to check if the UNIX
    file already exists and do not overwrite it if so.			*/

	make_ext( &ext[ 0 ], i1val, i2val );
	iter = 0;
	while ( ((tchr = *(nptr++)) >= '0') &&  iter < 6 ) {
		if (tchr >= 'A' && tchr <= 'Z') tchr |= 0x20;
		final_name[ iter++ ] = tchr;
	}
	final_name[ iter ] = '\0';
	strcat( &final_name[ 0 ], &ext[ 0 ] );
	if (access( &final_name[ 0 ], 0 ) == 0) {
		printf( "%s exists, not overwritten\n", &final_name[ 0 ] );
		return;
	}

	fd = open( &final_name[ 0 ],
		O_CREAT | O_WRONLY | O_APPEND | O_TRUNC,
		0644
	);
	if (fd < 0) {
		perror( "Failed to create UNIX file" );
		exit();
	}

/*  Report to the user.  */

	printf( "Reading %s, %d blocks\n", &final_name[ 0 ], fsize );

	for (iter = 0; iter < fsize; iter++) {
		ival = stape_read( &stape_buf[ 0 ] );
		if (ival != STAPE_BUFSIZE) {
			perror( "Error reading streaming tape" );
			exit();
		}
		else nstape_rec++;

		ival = write(fd, &stape_buf[ 0 ], STAPE_BUFSIZE );
		if (ival != STAPE_BUFSIZE) {
			perror( "Error writing UNIX file" );
			exit();
		}
	}
}

/*  Locate exactly 1 FCB given a name to search for.  The VXR directory
    does not have to have been sorted.  The names in the VXR directory
    are assumed to have had the parity bit cleared.			*/

int locate_1fcb( nptr )
char *nptr;
{
	char		*cp, vxrname[ 8 ];
	int		in, ip, iter, jter;
	struct mnempage *dp;

	dp = (struct mnempage *) &stape_dir[ 0 ];
	for (iter = 0; iter < num_entries; iter++) {
		ip = iter / MNENTRYLEN;
		in = iter % MNENTRYLEN;
		cp = (dp+ip)->mnemarray[ in ].n;

/*  You have to copy the VXR name because names with 6 characters will NOT
    be terminated with a NUL character.  Stop when a NUL character or a
    blank is found.							*/

		jter = 0;
		while (jter < 6)
		 if ( *cp != ' ' && *cp != '\0' )
		  vxrname[ jter++ ] = *(cp++);
		 else break;
		vxrname[ jter ] = '\0';

		if (nscmp( nptr, &vxrname[ 0 ] )) return( iter );
	}

	return( -1 );					/*  Never found  */
}

load_1file( np )
char *np;
{
	int		fsize, i3val, in, ip, fcb_index;
	struct fcb	*fptr;
	struct mnempage *bp;

/*  Locate the entry in the VXR directory.  */

	fcb_index = locate_1fcb( np );
	if (fcb_index < 0) {
		printf( "%s not found\n", np );
		return;
	}

/*  Locate the corresponding FCB; verify the entry is a spacefilling file.  */

	ip = fcb_index / MNENTRYLEN;
	in = fcb_index % MNENTRYLEN;
	bp = (struct mnempage *) &stape_dir[ 0 ];
	fptr = &( (bp+ip)->mnemarray[ in ].v );
	if ( !valid_fcb( fptr ) ) {
		printf( "%s is not a spacefilling file\n" );
		return;
	}
	else load_vxrfile( fptr, np );
}

load_nfiles( argc, argv )
int argc;
char *argv[];
{
	char		*cp, vxrname[ 8 ];
	int		fcb_index, found, in, ip, iter, jter;
	struct fcb	*fcbptr;
	struct mnentry	*mnemptr;
	struct mnempage *bp;

	bp = (struct mnempage *) &stape_dir[ 0 ];
	for (iter = 0; iter < ns_entries; iter++) {
		fcb_index = i3index[ iter ];
		if (fcb_index < 0 || fcb_index >= num_entries) {
			printf( "programmer error in load_nfiles, %d  %d\n",
				iter, fcb_index
			);
			exit();
		}

		in = fcb_index % MNENTRYLEN;
		ip = fcb_index / MNENTRYLEN;
		mnemptr = &( (bp+ip)->mnemarray[ in ] );
		cp = mnemptr->n;

/*  See note in LOCATE_1FCB  */

		jter = 0;
		while (jter < 6)
		 if ( *cp != ' ' && *cp != '\0' )
		  vxrname[ jter++ ] = *(cp++);
		 else break;
		vxrname[ jter ] = '\0';

/*  Now search the argument list for this entry  */

		found = 0;
		jter = 0;
		while (jter < argc && !found)
		 if (nscmp( argv[ jter ], &vxrname[ 0 ] )) {
			fcbptr = &mnemptr->v;
			load_vxrfile( fcbptr, &vxrname[ 0 ] );
			found = 131071;
		 }
		 else jter++;
	}
}

stape_load( argc, argv )
int argc;
char *argv[];
{
	if (organize_dir) load_nfiles( argc, argv ); 
	else		  load_1file( argv[ 0 ] );
}
