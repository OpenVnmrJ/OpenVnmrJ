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

/*  Initial version of a programmer's library for Varian PAID.
    Contents:
	find_vnmr_ds
	is_vnmr_ds
	quik_strncpy
	strcmp_nocase

    For SUN UNIX the compiler switch UNIX must be defined or
    the program will not compile.  Thus:
	cc -DUNIX -c proglib.c
								*/

#include <stdio.h>
#include <ctype.h>

#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#else
#include  stat
#include  "dir.h"
#endif

#define  IS_PAR 	1
#define  IS_FID 	2
#define  MAXPATHL	128

/*  The value returned by this routine is the same as that returned by
    "is_vnmr_ds", below.  This routine interacts with the file system.

    If the routine is successful, ``finalpath'' contains the complete
    name of the VNMR data set.  Furthermore, it is known to be a
    directory.

    The VMS version returns ``finalpath'' as the Files-11 directory
    specification, e. g. [VNMR.FIDLIB.FID1D_FID]			*/

int find_vnmr_ds( startpath, finalpath, final_len, fp_switch )
char *startpath;
char *finalpath;
int final_len;
int fp_switch;
{
	char	tempbuf[ MAXPATHL ];
	int	ival, tlen;

/*  Default is to look for FID first.  */

	if (fp_switch != IS_PAR)
	  fp_switch = IS_FID;

/*  Start by verifying the length of the string  */

	tlen = strlen( startpath );
	if (tlen < 0 || tlen > MAXPATHL-14)
	  return( 0 );

#ifdef VMS
#define SS$_NOTRAN	1577		/* Why does this have to appear
					   like success from VMS???	*/
	if ( *(startpath+tlen-1) == ':' ) {
		ival = get_logical_symbol(
			startpath, tlen, &tempbuf[ 0 ], MAXPATHL-2
		);
		if ( (ival & 3) != 1 )
		  return( 0 );
		if (ival == SS$_NOTRAN)
		  return( 0 );
		tlen = strlen( &tempbuf[ 0 ] );
		if (tlen < 0 || tlen > MAXPATHL-14)
		  return( 0 );
	}
	else
	  strcpy( &tempbuf[ 0 ], startpath );

/*  If the string contains a file name and is not a bare directory,
    then we want to search for "_fid" and "_par".  If either are
    successful, then construct the VMS directory from this.

    If it is rather a bare directory, the last level must end in
    either "_fid" or "_par".					*/

	ival = is_fname( &tempbuf[ 0 ], NULL, 0 );
	if (ival) {
		ival = is_vnmr_ds( &tempbuf[ 0 ] );
		if (ival != 0)
		  goto l1;

		ival = fp_switch;
		if (ival == IS_FID)
		  strcat( &tempbuf[ 0 ], "_fid.dir" );
		else
		  strcat( &tempbuf[ 0 ], "_par.dir" );
		if (access( &tempbuf[ 0 ], 0 ) == 0)
		  goto l1;

/*  Now look for the other kind of file, the opposite
    of that suggested by ``fp_switch''			*/

		ival = (fp_switch == IS_FID) ? IS_PAR : IS_FID;
		tempbuf[ tlen ] = '\0';
		if (ival == IS_FID)
		  strcat( &tempbuf[ 0 ], "_fid.dir" );
		else
		  strcat( &tempbuf[ 0 ], "_par.dir" );
		if (access( &tempbuf[ 0 ], 0 ) != 0)
		  return( 0 );

/*  If control comes to label l1, we have a file that appears to
    be a directory.  Verify that it indeed is before proceeding.	*/

	      l1:
		if (isDirectory( &tempbuf[ 0 ] ) == 0)
		  return( 0 );
		make_vmstree( &tempbuf[ 0 ] , &tempbuf[ 0 ], MAXPATHL-2 );
		quik_strncpy( finalpath, &tempbuf[ 0 ], final_len );
		return( ival );
	}
	else {
		ival = is_vnmr_ds( &tempbuf[ 0 ] );
		if (ival != 0)
		  quik_strncpy( finalpath, &tempbuf[ 0 ], final_len );
		return( ival );
	}
#else

/*  If the string is already a complete VNMR dataset name,
    verify it exists.  If so, copy startpath to finalpath
    (imposing the restriction of length) then return its
    type; else return 0						*/

	ival = is_vnmr_ds( startpath );
	if (ival != 0)
	  if (access( startpath, 0 ) == 0)
	    goto l1;
	  else
	    return( 0 );

/*  Use value of ``fp_switch'' to establish whether
    to search for .par or .fid first.			*/

	ival = fp_switch;
	strcpy( &tempbuf[ 0 ], startpath );
	if (ival == IS_FID)
	  strcat( &tempbuf[ 0 ], ".fid" );
	else
	  strcat( &tempbuf[ 0 ], ".par" );
	if (access( &tempbuf[ 0 ], 0 ) == 0)
	  goto l1;

	ival = (fp_switch == IS_FID) ? IS_PAR : IS_FID;
	tempbuf[ tlen ] = '\0';
	if (ival == IS_PAR)
	  strcat( &tempbuf[ 0 ], ".par" );
	else
	  strcat( &tempbuf[ 0 ], ".fid" );
	if (access( &tempbuf[ 0 ], 0 ) != 0)
	  return( 0 );

/* Verify UNIX file is a directory */

      l1:
	if (isDirectory( &tempbuf[ 0 ] ) == 0)
	  return( 0 );
	quik_strncpy( finalpath, &tempbuf[ 0 ], final_len );
	return( ival );
#endif
}

/*  Returns:
	IS_PAR		if cptr is a string ending in ".par"
	IS_FID		if cptr is a string ending in ".fid"
	0		otherwise

    VMS version replaces dot with underscore and handles the
    case where the string in a directory specification.  Thus:

	[VNMR1.FIDLIB]FID1D_FID
	[VNMR1.FIDLIB.FID1D_FID]

    both are accepted as FID data sets; but

	[VNMR1.FIDLIB]FID1D_FID.DIR

    is NOT accepted as a VNMR data set.

    This routine does not interact with the file system.	*/

int is_vnmr_ds( cptr )
char *cptr;
{
	char	*tptr;
	int	 len;

	if (cptr == NULL)
	  return( 0 );
	len = strlen( cptr );
	if (len < 4)			/* check for string too short */
	  return( 0 );

/*  Note this routine exploits the fact that both
    ".par" and ".fid" are each 4-character strings  */

#ifdef UNIX
	tptr = cptr + (len-4);
	if (*tptr != '.')
	  return( 0 );

	tptr++;
	if (strcmp_nocase( tptr, "par" ) == 0)
	  return( IS_PAR );
	else if (strcmp_nocase( tptr, "fid" ) == 0)
	  return( IS_FID );
	else
	  return( 0 );
#else
	if ( *(cptr+len-1) == ']' ) {
		if (len < 6)
		  return( 0 );
		tptr = cptr + (len-5);
	}
	else
	  tptr = cptr + (len-4);

	if (*tptr != '_')
	  return( 0 );

/*  The string comparisons are a bit awkward, but we do not want to
    overwrite the right bracket if it exists.  The alternative is to
    copy the string argument into a local buffer and zap the bracket
    in the local buffer.						*/

	tptr++;
	if (strcmp_nocase( tptr, "par" ) == 0 ||
	    strcmp_nocase( tptr, "par]" ) == 0)
	  return( IS_PAR );
	else if (strcmp_nocase( tptr, "fid" ) == 0 ||
		 strcmp_nocase( tptr, "fid]" ) == 0)
	  return( IS_FID );
	else
	  return( 0 );
#endif
}

/*  Taken from /jaws/sysvnmr/builtin.c
    UNIX version returns 0 if the file itself doesn't exist.	*/

/*------------------------------------------------------------------------
|	isDirectory
|	
|	This routine determines if name is a directory
|	return non-zero if so; zero if error or not-a-directory
+-----------------------------------------------------------------------*/

int isDirectory(dirname)
char *dirname;
{
#ifdef VMS

/*  The VMS version is complex because several different formats
    for expressing the name of a directory must be supported.
    These are best illustrated by example:

    Assume that the following directory exists on VMS:
	[ROBERT.VNMR.EXP1]
    and the current working directory is:
	[ROBERT.VNMR]
    Then the following arguments to isDirectory should return a
    non-zero result:
	[ROBERT.VNMR.EXP1]
	[ROBERT.VNMR]EXP1.DIR
	[ROBERT.VNMR]EXP1
	EXP1.DIR
	EXP1							*/

#define  NAM$C_MAXRSS	256			/* From RMS */
    char	workspace[ NAM$C_MAXRSS+2 ];	/* Room for NUL char */
    char	*bptr, *dptr, *sptr;
    int 	dlen, ival;
    extern char *rindex();
    struct stat buf;

    if (strlen( dirname ) > NAM$C_MAXRSS) return( 0 );
    strcpy( &workspace[ 0 ], dirname );
    sptr = rindex( &workspace[ 0 ], ';' );	/* Pointer to semi-colon */
    dptr = rindex( &workspace[ 0 ], '.' );	/* Pointer to dot */
    bptr = rindex( &workspace[ 0 ], ']' );	/* Pointer to right-bracket */

    if (sptr != NULL) *sptr = '\0';		/* No version numbers, please */
    dlen = strlen( &workspace[ 0 ] );

/*  First case:  no dot and no right bracket.  Append ".dir"
    to the file name.						*/

    if (dptr == NULL && bptr == NULL)
      strcat( &workspace[ 0 ], ".dir" );

/*  Check for three cases if a right-bracket is present:
    1.  If the right bracket is the last character, use
	GET_PARENTDIR() to obtain the actual name of the
	parent directory.

    2.  If no dot, or the dot is part of the directory tree
	and thus its address is less than that of the bracket,
	append ".dir" to the file name.

    3.  Otherwise, use the name "as is".			*/

    else if (bptr != NULL) {
	if (bptr - &workspace[ 0 ] == dlen-1)
	  get_parentdir( &workspace[ 0 ], &workspace[ 0 ], NAM$C_MAXRSS );
	else if (dptr == NULL || ( dptr != NULL && dptr < bptr ))
	  strcat( &workspace[ 0 ], ".dir" );
    }

/*  If no right bracket present, but there is a dot, use as is.  */

    ival = stat( &workspace[ 0 ], &buf );	/* Returns 0 if successful */
    if (ival) return( 0 );			/* No directory if error */
    else      return(buf.st_mode & S_IFDIR);
#else
    struct stat buf;				/* UNIX version much simpler */

    if (stat(dirname, &buf) != 0)
      return( 0 );
    else
      return(buf.st_mode & S_IFDIR);
#endif
}

/*  Given source string b, destination string a and size n,
    this routine will copy all of b to a, if the length of
    b is shorter than n; or copy n-1 characters of b to a,
    setting the n'th character of a to '\0'.			*/

quik_strncpy( a, b, n )
char *a;
char *b;
int n;
{
	int	b_len;

	if (n < 2) return;		/* don't f**k with me!! */

	b_len = strlen( b );
	if (b_len < 0) {
		*a = '\0';
		return;
	}

	if (b_len < n) {
		strcpy( a, b );
	}
	else {
		strncpy( a, b, n-1 );
		a[ n-1 ] = '\0';
		return;
	}
}

/*  Use this routine for case-independent string comparisons.	*/

int strcmp_nocase( s1, s2 )
char *s1;
char *s2;
{
	char c1, c2;

	do {
		c1 = *(s1++);
		c2 = *(s2++);
		if (isupper( c1 ))
		  c1 = tolower( c1 );
		if (isupper( c2 ))
		  c2 = tolower( c2 );
		if (c1 != c2)
		  return( (int) (c1 - c2) );
	}
	  while (c1 != '\0');

	return( 0 );
}
