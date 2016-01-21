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

#include  <unistd.h>
#include  <stdio.h>
#include  <fcntl.h>

#include  "group.h"			/*  VNMR include file */
#include  "symtab.h"                    /*  VNMR include file */
#include  "variables.h"
#include  "config.h"

/*  This file is modified from vconfig.c  */

#ifndef VMS
#include  <sys/file.h>
#define F_OK            0       /* does file exist */
#define X_OK            1       /* is it executable by caller */
#define W_OK            2       /* writable by caller */
#define R_OK            4       /* readable by caller */
#endif

#define  SYSTEMDIR   (1<<0)
#define  CONPARFILE  (1<<1)
#define  STDPARFILE  (1<<2)
#define  TESTSFILE   (1<<3)

#define  MAXPATHL    128

/*  This define lets the program developer work with an arbitrary CONPAR
    file.  Since CONPAR would now be defined by the programmer, defining
    this symbol causes the program to assume you are debugging.  Thus
    you can access interactive mode without all the permission tests.
    Use the `display' argument to access display mode.			*/


/*  Global structures of builtin type.  */

int	interact_flag;
int	bg_flag;
int	fileStatus;
int	makestdparlink;
int	maketestslink;
int	display_flag;
char	systemdir[ MAXPATHL ] = "";
char	userdir[ MAXPATHL ] = "";	/* vfilesys.c (follow_link) */
				      /* requires the userdir array */

/*  Store the name of the CONPAR file here.	*/

char      conparfil[ MAXPATHL ];
char      conFile[ MAXPATHL ];

struct _hw_config   console_data;

char	*cwb_file;

extern char RevID[];
extern char RevDate[];
extern char CDNT[];

#ifndef VMS
	extern char	*getenv();
#else
	extern char	*vms_getenv();
#endif

/************************************************************************/


#ifndef VMS

/*  These programs might get called when VNMR config exits.
    The public program is configExit.

    configExit is called from the GUI programs.  It does not
    itself call exit, but returns to the GUI and assumes the
    GUI calls the exit program.

    Unlike the GUI version (configExit_sunview, etc.), this
    program assumes the current data should be written out.	*/

putSysGlobalVals( fnptr )
char *fnptr;
{
	int	r;

	r = P_save(SYSTEMGLOBAL, fnptr);
	if (r != 0)
	  fprintf( stderr,
		"failed to write system global parameters to %s\n", fnptr
	  );
}

int
makelink( link_name )
char *link_name;
{
	char	base_path[ MAXPATHL ], link_path[ MAXPATHL ], h1freq_str[ 14 ];
	int	result;
	double	h1freq_val;

	result = P_getreal( SYSTEMGLOBAL, "h1freq", &h1freq_val, 1 );
	if (result != 0) {
		fprintf( stderr, "cannot access `h1freq' parameter\n" );
		return( -1 );
	}

/*  Use %.3d format convertor to guarantee at least
    3 non-blank characters in the resulting string.	*/

	sprintf( &h1freq_str[ 0 ], "%.3d", (int) (h1freq_val+0.001) );

	strcpy( &base_path[ 0 ], &systemdir[ 0 ] );
	strcat( &base_path[ 0 ], "/par" );
	strcat( &base_path[ 0 ], &h1freq_str[ 0 ] );
	strcat( &base_path[ 0 ], "/" );
	strcat( &base_path[ 0 ], link_name );

/*  Verify the base path exists before proceeding...  */

	if (access( &base_path[0], F_OK) != 0) {
		fprintf( stderr, "entry %s doesn't exist\n", &base_path[ 0 ] );
		return( -1 );
	}

	strcpy( &link_path[ 0 ], &systemdir[ 0 ] );
	strcat( &link_path[ 0 ], "/" );
	strcat( &link_path[ 0 ], link_name );

/*  Only unlink if the link is present.  */

	if (access( &link_path[ 0 ], F_OK ) == 0)
	  if (unlink( &link_path[ 0 ])) {
		perror("unlink failure in VNMR config");
		fprintf( stderr, "failed to unlink %s\n", &link_path[ 0 ] );
		return( -1 );
	  }

	if (symlink( &base_path[ 0 ], &link_path[ 0 ] )) {
		perror("link failure in config");
		fprintf( stderr,
	    "Failed to link %s to %s", &link_path[ 0  ], &base_path[ 0 ]
		);
		return( -1 );
	}

	return( 0 );
}

#endif


/************************************************************************/

static int
process_args( argc, argv)
int argc;
char *argv[];
{
	int	iter;

	display_flag = 0;
	bg_flag = 0;
	cwb_file = NULL;
	if (argc < 2)
	  return( 0 );

	iter = 0;						/* Start at 1 */
	while (++iter < argc ) {
		if (strcmp( argv[ iter ], "display" ) == 0 ||
		    strcmp( argv[ iter ], "print" ) == 0)
		  display_flag = 1;
		else if (strcmp( argv[ iter ], "-background") == 0 ||
		    strcmp( argv[ iter ], "-bg") == 0)
		  	bg_flag = 1;
		else if (strcmp( argv[ iter ], "cwbf" ) == 0)
		{
                    iter++;
                    if (argc <= iter) {
                        fprintf( stderr, "Console writeback file requires a file name\n");
			return(-1);
		    }
		    if (cwb_file) {
                	free( cwb_file );
                	cwb_file = NULL;
        	    }
		    if ((int)strlen(argv[iter]) > 0)
		    {
			cwb_file = (char *) malloc(strlen(argv[iter]) + 1);
			if (cwb_file != NULL)
			    strcpy(cwb_file, argv[iter]);
		    }
		}
	}

	return( 0 );
}

#ifndef VMS

/*  VNMR config can run interactively only if some conditions are met:
	    write access to the VNMR system directory
	    write access to the CONPAR file
	    write access to the link "stdpar" in the VNMR system directory
	    write access to the link "tests" in the VNMR system directory
*/

static int
check_interactive_access( report_flag, cmd_name )
int report_flag;
char  *cmd_name;
{
	int	retval;
	char	conparlink[ MAXPATHL ];
	int	ival;
	
	retval = 0;
	if (access(&systemdir[ 0 ], W_OK) != 0) {
		if (report_flag)
		  fprintf( stderr,
    "cannot run %s interactively, no write access to %s\n",
     cmd_name, &systemdir[ 0 ]
		  );
		fileStatus |= SYSTEMDIR;
		retval = -1;
	}

	if (access( &conparfil[ 0 ], W_OK) != 0) {
		if (report_flag)
		  fprintf( stderr,
    "cannot run %s interactively, no write access to %s\n",
     cmd_name, &conparfil[ 0 ]
		  );
		fileStatus |= CONPARFILE;
		retval = -1;
	}

	return( retval );
}

#endif


int
main( argc, argv )
int argc;
char *argv[];
{
	int		ival, report_flag;
	char		*quick_ptr;

#ifndef VMS
	quick_ptr = getenv( "vnmrsystem" );
#else
	quick_ptr = vms_getenv( "vnmrsystem" );
#endif

	process_args(argc, argv);
	if (bg_flag)
	{
	    freopen("/dev/null","a", stdout);
            freopen("/dev/null","a", stderr);
	}
	if (quick_ptr == NULL)
	{
	    fprintf( stderr, "Error: env 'vnmrsystem' is not set. \n");
	    exit( 1 );
	}

	if (strlen( quick_ptr ) >= MAXPATHL-30) {
		fprintf( stderr,
	    "error starting VNMR CONFIG, value for 'vnmrsystem' is too long\n"
		);
		exit( 1 );
	}
	strcpy( &systemdir[ 0 ], quick_ptr );
	read_console_data();
        interact_flag = 1;
	report_flag = 0;

#ifdef CONPAR_FILE
	strcpy( &conparfil[ 0 ], CONPAR_FILE );
#else
#ifndef VMS
	makestdparlink = 131071;		/* Normally these links are */
	maketestslink = 131071; 		/* to be made if the command */
						/* is used interactively. */
	strcpy( &conparfil[ 0 ], &systemdir[ 0 ] );
	strcat( &conparfil[ 0 ], "/conpar" );

        ival = check_interactive_access( report_flag, "VNMR config" );
	if (ival != 0)
	    interact_flag = 0;
#else
	strcpy( &conparfil[ 0 ], &systemdir[ 0 ] );
	strcat( &conparfil[ 0 ], "conpar" );

	interact_flag = 0;		/* No interactive mode on VMS */
#endif
#endif
	getSysVals( &conparfil[ 0 ] );

/* ---  print revision ID and Date, Compiled within revdate.c --- */
        P_setstring( SYSTEMGLOBAL, "rev", &RevID[ 0 ], 1);
        P_setstring( SYSTEMGLOBAL, "revdate", &RevDate[ 0 ], 1);
	if (display_flag)
	{
	    disp_conpar(stdout);
	    if (!bg_flag)
		return;
	}

	if (bg_flag)
	{
	    if (interact_flag)
	    {
		restore_console_data();
		vconfigSaveExit();
	    }
	}
	else
	{
	   makeVconfigPanels( argc, argv );
	   vconfigLoop();
	}
}



extern symbol   **getTreeRoot();
extern varInfo   *rfindVar();
extern varInfo   *RcreateVar();

int
config_setreal( name, value, index )
char *name;
double value;
int index;
{
        symbol  **root;
        varInfo *v;

        if (root = getTreeRoot(getRoot( SYSTEMGLOBAL )))
          if (v = rfindVar( name, root )) {
                if (assignReal( value, v, index ))
                  return( 0 );
                else
                  return( -99 );
          }

/*  Create the real number parameter if it does not exist.  */

          else {
                v = RcreateVar( name, root, T_REAL );
                if (assignReal( value, v, index ))
                  return( 0 );
                else
                  return( -99 );
          }
        else
          return( -1 );                 /* No such tree (should not happen) */
}

int config_setstring( name, value, index )
char *name;
char *value;
int index;
{
        symbol  **root;
        varInfo *v;

        if (root = getTreeRoot(getRoot( SYSTEMGLOBAL )))
          if (v = rfindVar( name, root )) {
                if (assignString( value, v, index ))
                  return( 0 );
                else
                  return( -99 );
          }


/*  Create the string parameter if it does not exist.  */

          else {
                v = RcreateVar( name, root, T_STRING );
                if (assignString( value, v, index ))
                  return( 0 );
                else
                  return( -99 );
          }
        else
          return( -1 );                 /* No such tree (should not happen) */
}


