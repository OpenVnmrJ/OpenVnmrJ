/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "vnmrsys.h"

#include <stdio.h>
#include <ctype.h>			/* for isascii, etc. */
#include <string.h>
#ifdef UNIX
#include <unistd.h>			/* to define R_OK, etc. for Solaris  */
#include <sys/file.h>			/* to define R_OK, etc. */
#include <sys/types.h>			/* for stat.h, dir.h */
#include <sys/stat.h>			/* to define stat data structure */
#include <dirent.h>			/* to define directory data structures */
#else 
#define R_OK	4
#include "dirent.h"
#include stat
#endif 

#include "element.h"
#include "tools.h"

extern char	*getcwd();
extern char	*get_cwd();
extern int	 Tflag;
extern int	 Bnmr;

#ifndef VNMRJ
static Elist	*elem_list_base = NULL;
static char	 cur_working_dir[ MAXPATH ] = {'\0'};
#endif

/*----------------------------------------------------------------------
|       fileExists
|
|       This routine determines if the file exists, returns non 0 if does.
+------------------------------------------------------------------------*/
int fileExists(char *s)
{
	if (!access(s,R_OK)) {			/* is it readable */
		if (Tflag)
		  fprintf(stderr,"fileExists: %s is readable\n",s);
		return 1;
	}
	else {
		if (Tflag)
		  fprintf(stderr,"fileExists: %s is not readable",s);
		return 0;
	}
}

/*----------------------------------------------------------------------
|   isFDARec  
|
|   This routine determines if the directory is a REC diredtory.
|   It returns 1 if it is, a 0 if it is not.  
+-----------------------------------------------------------------------*/
int isFDARec(char *s)
{
	char *ptr;

/*   directory path has the form  /abc/xxx.REC   */

	if (!(ptr = strrchr(s,'.'))) {		/* find last dot in last field */  
		if (Tflag)
		  fprintf(stderr,"isFDARec: Could not find a dot in %s\n",s);
		return 0;
	}
	if (strncmp(".REC",ptr,4) == 0  && strlen(ptr) == 4) {
		if (Tflag)
		  fprintf(stderr,"isFDARec: %s is a REC file\n",s);
		return 1;
	}
	else {
		if (Tflag)
		  fprintf(stderr,"isFDARec: %s is not a REC file\n",s);
		return 0;
	}
}

/*----------------------------------------------------------------------
|   isRec  
|
|   This routine determines if the directory is a rec diredtory.
|   It returns 1 if it is, a 0 if it is not.  
+-----------------------------------------------------------------------*/
int isRec(char *s)
{
	char *ptr;

/*   directory path has the form  /abc/xxx.rec   */

	if (!(ptr = strrchr(s,'.'))) {		/* find last dot in last field */  
		if (Tflag)
		  fprintf(stderr,"isRec: Could not find a dot in %s\n",s);
		return 0;
	}
	if (strncmp(".rec",ptr,4) == 0  && strlen(ptr) == 4) {
		if (Tflag)
		  fprintf(stderr,"isRec: %s is a rec file\n",s);
		return 1;
	}
	else {
		if (Tflag)
		  fprintf(stderr,"isRec: %s is not a rec file\n",s);
		return 0;
	}
}

/*----------------------------------------------------------------------
|   isFid  
|
|   This routine determines if the directory is a fid diredtory.
|   It returns 1 if it is, a 0 if it is not.  
+-----------------------------------------------------------------------*/
int isFid(char *s)
{
	char *ptr;

/*   directory path has the form  /abc/xxx.fid   */

	if (!(ptr = strrchr(s,'.'))) {		/* find last dot in last field */  
		if (Tflag)
		  fprintf(stderr,"isFid: Could not find a dot in %s\n",s);
		return 0;
	}
	if (strncmp(".fid",ptr,4) == 0  && strlen(ptr) == 4) {
		if (Tflag)
		  fprintf(stderr,"isFid: %s is a fid file\n",s);
		return 1;
	}
	else {
		if (Tflag)
		  fprintf(stderr,"isFid: %s is not a fid file\n",s);
		return 0;
	}
}

/*----------------------------------------------------------------------
|   isPar  
|
|   This routine determines if the directory is a parameter diredtory.
|   It returns 1 if it is, a 0 if it is not.  
+-----------------------------------------------------------------------*/
int isPar(char *s)
{
	char *ptr;

/*   directory path has the form  /abc/xxx.par   */

	if (!(ptr = strrchr(s,'.'))) {		/* find last dot in last field */  
		if (Tflag)
		  fprintf(stderr,"isPar: Could not find a dot in %s\n",s);
		return 0;
	}
	if (strncmp(".par",ptr,4) == 0  && strlen(ptr) == 4) {
		if (Tflag)
		  fprintf(stderr,"isPar: %s is a par file\n",s);
		return 1;
	}
	else {
		if (Tflag)
		  fprintf(stderr,"isPar: %s is not a par file\n",s);
		return 0;
	}
}

/*------------------------------------------------------------------------
|       isDirectory
|
|       This routine determines if name is a directory
+-----------------------------------------------------------------------*/
int isDirectory(char *filename)
{
	int		ival, retval;
	struct stat	buf;

	ival = stat(filename,&buf);
	if (ival != 0)
	  retval = 0;
	else
	  if (buf.st_mode & S_IFDIR)
	    retval = 1;
	  else
	    retval = 0;

	return( retval );
}

/*----------------------------------------------------------------------
|       isFileAscii/1
|
|       This routine is a crude determination if the file is an ascii
|       file.  It returns a 1 if it is.  This program looks at the first
|       512 bytes of a file and checks if they are all ascii.
|
|	Extended to verify file is not a directory before proceeding.
|	This is the only way we can eliminate directories as possible
|	ascii files, as one cannot read any characters from a directory
|	with fgetc.
|	Extended to test for NUL characters ( \0 ), which should not
|	appear in ascii files; for example, vi removes any NUL characters.
+------------------------------------------------------------------------*/
int isFileAscii(char *s)
{
	FILE	*stream;
	int	 c;
	int	 i;

	if (isDirectory(s))
	  return( 0 );

	i = 0;
	if ( (stream = fopen(s,"r")) ) {
		if (Tflag)
		  fprintf(stderr,"isFileAscii: opened file %s\n",s);
		while ((c=fgetc(stream)) != EOF && i<512) {
			if (!isascii(c) || c == 0) {
				if (Tflag)
				  fprintf(stderr,
			    "isFileAscii:A non ascii char found\n"
				  );
				fclose(stream);
				return 0; 
			}             
		}          
		if (Tflag)
		  fprintf(stderr,"isFileAscii:File is ascii\n");
		fclose(stream);
		return 1;
	}
	else {
		if (Tflag)
		  fprintf(stderr,"isFileAscii: problems opening %s\n",s);
		return 0;
	}
} 

#ifndef VNMRJ
/*------------------------------------------------------------------------
|       directoryLoad/1
|
|  This routine loads up the element structure with the files from the
|  requested directory.  It returns a 0 if everything was ok, 1 if not.     
+--------------------------------------------------------------------------*/
static
directoryLoad( dirpath )
char *dirpath;
{
	DIR		*dirp;
	struct dirent	*dp;
 
/* Open the directory and add file names to the element structure */

	if (dirp = opendir(dirpath)) {

	/* must clear out old elist if one already exists */

		if (elem_list_base) {
			releaseElementStruct(elem_list_base);
			elem_list_base = NULL;
		} 
		if (!(elem_list_base = initElementStruct())) {
			if (Tflag)
			  fprintf(stderr,"directoryLoad:out of memory\n");
			closedir(dirp);
			return 1;
		} 
		if (Tflag)
		  fprintf(stderr,"directoryLoad: elist created\n");
		for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {

		/* no . files in element structure*/

			if (*dp->d_name != '.') {
				char newname[MAXPATH];

				sprintf(newname,"%s/%s",dirpath,dp->d_name);
				if (isDirectory(newname)) {
					addElement(elem_list_base,dp->d_name,"d");
				}
				else {
					addElement(elem_list_base,dp->d_name,"n");
				}
			}
		}   

		if (Tflag)
		  fprintf(stderr,"directoryLoad: returning normally\n");
		closedir(dirp);
		return 0;
	}   
	else {
		if (Tflag)
		  fprintf(stderr,"directoryLoad:trouble opening \"%s\"\n",dirpath);
		return 1;
	}
}

/*----------------------------------------------------------------------
|       CleanUp
|
|       This routine clears everything up. 
+------------------------------------------------------------------------*/
static
CleanUp()
{
	extern int terminalElementActive;
   
	if (Wissun())
	  Wturnoff_mouse();
	else
	  if (Wisgraphon()) {
		WblankCanvas();
		terminalElementActive = 0; /* make sure graphon elements not active */
	}
	if (elem_list_base) {
		releaseElementStruct(elem_list_base);
		elem_list_base = NULL;
	}
	if (WgraphicsdisplayValid( "files" ))
	  Wsetgraphicsdisplay( "" );
}

/*------------------------------------------------------------------------*/

static int
display_the_files( cwd_addr )
char *cwd_addr;
{
	int	ival;

	if (elem_list_base != NULL) {
		releaseElementStruct( elem_list_base );
		elem_list_base = NULL;
	}

	ival = directoryLoad( cwd_addr );
	if (ival != 0) {
		Werrprintf( "cannot read files from %s", cwd_addr );
		return( -1 );
	}
	sortElementByName( elem_list_base );
	displayElements( elem_list_base );

	return( 0 );
}

static int
display_files_menu( menu_name, menu_path, turnoff_routine )
char *menu_name;
char *menu_path;
int (*turnoff_routine)();
{
	int	ival;

	ival = read_menu_file( menu_path );
	if (ival != 0) {
		Werrprintf( "cannot open file menu %s", menu_path );
		return( -1 );
	}

	exec_menu_buf();
	display_menu( menu_name, turnoff_routine );

	return( 0 );
}

int
files( argc, argv )
int argc;
char *argv[];
{
	int	ival, plan_to_display_files, plan_to_display_menu,
		turnoff_flag;
	char	new_menu_name[ MAXPATH ], new_menu_path[ MAXPATH ],
		new_working_dir[ MAXPATH ];
	char    *ptr;

static char	cur_menu_name[ MAXPATH ] = {'\0'};
static char	cur_menu_path[ MAXPATH ] = {'\0'};

	if (Bnmr) {
		Werrprintf(
	    "%s:  not available in a background mode of VNMR", argv[ 0 ]
		);
		ABORT;
	}
	else if (Wistek4x05()) {
		Werrprintf(
	    "%s:  not available on a Tektronix 4105 or 4205", argv[ 0 ]
		);
		ABORT;
	}

	plan_to_display_files = 131071;
	plan_to_display_menu  = 131071;
	turnoff_flag          = 131071;

	ival = verify_menu_params();
/*
	if (getcwd( &new_working_dir[ 0 ], sizeof( new_working_dir ) - 1 ) == NULL) {
*/
	if ((ptr = get_cwd()) == NULL)
	{
		Werrprintf( "Cannot obtain current working directory" );
		ABORT;
	}
	strcpy(new_working_dir, ptr);
	if (argc > 1)
	  strcpy( &new_menu_name[ 0 ], argv[ 1 ] );
	else {
		if (strlen( &cur_menu_name[ 0 ] ) < 1)
		  strcpy( &new_menu_name[ 0 ], "files_main" );
		else
		  strcpy( &new_menu_name[ 0 ], &cur_menu_name[ 0 ] );
	}

	ival = locate_menufile( &new_menu_name[ 0 ], &new_menu_path[ 0 ] );
	if (ival != 0) {
		Werrprintf(
	    "%s:  menu %s does not exist", argv[ 0 ], &new_menu_name[ 0 ] );
		ABORT;
	}

	if ( !WgraphicsdisplayValid( argv[ 0 ] ) ) {
		plan_to_display_files = 131071;
		plan_to_display_menu  = 131071;
		turnoff_flag          = 131071;
	}
	else {
		turnoff_flag = 0;
		if (strcmp( &new_menu_path[ 0 ], &cur_menu_path[ 0 ] ) != 0)
		  plan_to_display_menu = 131071;
		else
		  plan_to_display_menu = 0;
		if (strcmp( &new_working_dir[ 0 ], &cur_working_dir[ 0 ] ) != 0)
		  plan_to_display_files = 131071;
		else
		  plan_to_display_files = 0;
	}

/*  First turnoff the buttons (from previous VNMR command)
    Then display the files (this can take a while if many files are present)
    Then display the menu
    Necessary to turn off the buttons BEFORE displaying the files.	*/

	if (turnoff_flag)
	  Wturnoff_buttons();
	if (plan_to_display_files) {
		ival = display_the_files( &new_working_dir[ 0 ] );
		if (ival != 0)
		  ABORT;
	}
	if (plan_to_display_menu) {
		ival = display_files_menu(
		    &new_menu_name[ 0 ], &new_menu_path[ 0 ], CleanUp
		);
		if (ival != 0)
		  ABORT;
		if (turnoff_flag == 0)
		  resetButtonPushed();
	}

	strcpy( &cur_working_dir[ 0 ], &new_working_dir[ 0 ] );
	strcpy( &cur_menu_path[ 0 ], &new_menu_path[ 0 ] );
	strcpy( &cur_menu_name[ 0 ], &new_menu_name[ 0 ] );
	Wsetgraphicsdisplay( argv[ 0 ] );

	RETURN;
}
#endif

char *
get_all_active( elist )
Elist *elist;
{
	char		*first_one, *current_one;
	extern char	*newString(), *newCat();

	first_one = getFirstSelection( elist );
	if (first_one == NULL)
	  return( NULL );

/* getFirstSelection returns address from the Element List structure.
   Use newString to make a separate, private copy of the element.
   Since C passes arguments by value, not by reference, you can store
   the value that the function returns in the variable used as an
   argument, provided you are no longer interested in the original
   value passed to the function.  Since the original value in
   `first_one' is the address from the Element List structure, it can
   be discarded once we have a private copy of that string.		*/

	first_one = newString( first_one );
	current_one = first_one;

/* newCat assumes the first argument is an address into the dynamic
   heap and releases the memory it addresses.  So when newCat returns,
   we do not have to keep the value passed as an argument to newCat.	*/

	while ((current_one = getNextSelection( elist, current_one )) != NULL) {
		first_one = newCat( first_one, " " );
		first_one = newCat( first_one, current_one );
	}

/*  The calling routine must release the memory
    addressed by the value returned by this function.	*/

	return( first_one );
}

char *
get_one_active( elist, elem_num )
Elist *elist;
int elem_num;
{
	char	*current_one;
	int	 iter;

	current_one = getFirstSelection( elist );
	if (current_one == NULL)
	  return( NULL );
	iter = 1;

	while (iter < elem_num) {
		iter++;
		current_one = getNextSelection( elist, current_one );
		if (current_one == NULL)
		  return( NULL);
	}

/*  Copy the file name to a separate location on the heap.
    Get first/next selection returns an address from the
    elist data structure.  The calling routine expects to
    release the memory address by this return value, and
    that simply will not do for the elist data structure.  */

	current_one = newString( current_one );
	return( current_one );
}
#ifndef VNMRJ

/*  These defines allow process args of files elements to communicate
    the function the argument specifies to the files elements command.	*/

#define  ERROR	-1
#define  RESET	0
#define  NUMBER 1
#define  VALUE  2

static int
process_args_filesinfo( argc, argv, retc )
int argc;
char *argv[];
int retc;
{
	int	retval;

	if (argc < 2) {
		Werrprintf( "%s:  this command requires an argument", argv[ 0 ] );
		return( ERROR );
	}

	if (strcmp( argv[ 1 ], "redisplay" ) == 0)
	  retval = RESET;
	else {
		if (strcmp( argv[ 1 ], "number" ) == 0)
		  retval = NUMBER;
		else if (strcmp( argv[ 1 ], "name" ) == 0 ||
			 strcmp( argv[ 1 ], "names" ) == 0)
		  retval = VALUE;
		else {
			Werrprintf(
		    "%s: argument of %s not recognized", argv[ 0 ], argv[ 1 ]
			);
			return( ERROR );
		}

		if (retc < 1) {
			Werrprintf(
	    "%s: you must return a value with the %s argument", argv[ 0 ], argv[ 1 ]
			);
			return( ERROR );
		}
	}

	return( retval );
}

int
filesinfo( argc, argv, retc, retv )
int argc;
char *argv[];
int retc;
char *retv[];
{
	int	 cur_function, elem_num, ival, num_active;
	char	*active_list;

	if (!WgraphicsdisplayValid( "files" )) {
		Werrprintf( "%s:  files program is not active", argv[ 0 ] );
		ABORT;
	}

	cur_function = process_args_filesinfo( argc, argv, retc );
	if (cur_function == ERROR)
	  ABORT;

	if (cur_function == RESET) {
		if (elem_list_base != NULL) {
			releaseElementStruct( elem_list_base );
			elem_list_base = NULL;
		}
		ival = directoryLoad( &cur_working_dir[ 0 ] );
		if (ival != 0) {
			Werrprintf(
		    "cannot read files from %s", &cur_working_dir[ 0 ]
			);
			ABORT;
		}
		sortElementByName( elem_list_base );
		displayElements( elem_list_base );
	}
	else if (cur_function == NUMBER) {
		num_active = getNumberOfSelect( elem_list_base );
                retv[ 0 ] = realString( (double) num_active );
        }
	else if (cur_function == VALUE) {
		active_list = NULL;
		if (argc < 3)
		  active_list = get_all_active( elem_list_base );
		else {

	   /*  Three ways the command can abort here:
	       1.  The 2nd argument is not a number
	       2.  The 2nd argument is a number but is out of range
	       3.  Something goes wrong in `get_one_active' and it
		   fails to return a file name.

	       In the second case we set the value of the return
	       argument to the 0-length string.  In the other cases
	       we do nothing to the return argument.  The first one
	       is likely from a problem in the user's programming;
               the last one would be from a problem in our programs.	*/

			if (!isReal( argv[ 2 ] )) {
				Werrprintf(
		    "%s:  must use a number to select a file", argv[ 0 ]
				);
				ABORT;
			}

			elem_num = (int) stringReal( argv[ 2 ] );
			if (elem_num < 1) {
				Werrprintf(
	   "%s:  you must use an index greater than zero to select a file", argv[ 0 ]
				);
				retv[ 0 ] = newString( "" );
				ABORT;
			}
			num_active = getNumberOfSelect( elem_list_base );
			if (elem_num > num_active) {
				Werrprintf(
		   "%s:  you have not selected %d files", argv[ 0 ], elem_num
				);
				retv[ 0 ] = newString( "" );
				ABORT;
			}
			active_list = get_one_active( elem_list_base, elem_num );
			if (active_list == NULL) {
				Werrprintf(
		   "%s:  selection %d does not exist", argv[ 0 ], elem_num
				);
				ABORT;
			}
		}

		if (active_list) {
			retv[ 0 ] = newString( active_list );
			release( active_list );
		}
		else
		  retv[ 0 ] = newString( "" );
	}

	RETURN;
}
#endif
