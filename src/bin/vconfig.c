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

#include  "vnmrsys.h"			/*  VNMR include file */
#include  "group.h"			/*  VNMR include file */

/*  Three complier symbols allow the same program to be compiled for
    either the SUN or VMS with sections isolated by ifdef, etc.

    The symbol SUN isolates those parts that create and control the
    window system in the config program.

    The symbol UNIX isolates include files with a path syntax limited
    to UNIX-like file systems, for example, <sys/file.h>.  The
    alternate section has the VMS equivalent.

    The symbol VMS is normally referenced by its absence, as in
    ifndef.  It isolates those areas where we make changes (or prepare
    to make changes) to the system global parameter tree which are
    otherwise not isolated.						*/

#ifdef UNIX
#include  <sys/file.h>
#else
#define F_OK            0       /* does file exist */
#define X_OK            1       /* is it executable by caller */
#define W_OK            2       /* writable by caller */
#define R_OK            4       /* readable by caller */
#endif

#include  "vconfig.h"

/*  This define lets the program developer work with an arbitrary CONPAR
    file.  Since CONPAR would now be defined by the programmer, defining
    this symbol causes the program to assume you are debugging.  Thus
    you can access interactive mode without all the permission tests.
    Use the `display' argument to access display mode.			*/

/*#define    CONPAR_FILE	"conpar"*/


/*  Global structures of builtin type.  */

int		cur_line_item_panel;
char		systemdir[ MAXPATHL ] = "";
char		userdir[ MAXPATHL ] = "";	/* vfilesys.c (follow_link) */
					      /* requires the userdir array */

/*  Several buttons have 2 choices:  Not Present and Present.
    Use Not Present as first choice, for if an item is not
    configurable, it is Not Present.  Thus this can be the 
    sole choice when actually no choice exists.	  Notice
    that NOT_PRESENT is used in places other than simple
    yes / no choices, for example with the RF Synthesizer
    Frequency.  See `rf_panel.c'

    Following it is the corresponding parameter value table
    used when the value is 'n' or 'y'.  The NO value comes
    first, becuase the NOT_PRESENT index is defined to be 0.	*/

char	*present_not_present[ 3 ] = {
	"Not Present", "Present", NULL
};
char		 no_yes[] = { 'n', 'y' };

/*  RF channel labels.  This table is NOT terminated with a NULL address.  */

char	*rf_chan_name[ CONFIG_MAX_RF_CHAN ];

/*  Here is the storage addressed by the previous array.
    The only public access is via the table defined above.	*/

static char	 line_item_label[ CONFIG_MAX_RF_CHAN ][ 42 ];

/*  Store the name of the CONPAR file here.	*/

static char      conparfil[ MAXPATHL ];

#ifndef VMS
static int	 makestdparlink;
static int	 maketestslink;
#endif

extern char RevID[];
extern char RevDate[];
extern char CDNT[];

#ifndef VMS
	extern char	*getenv();
#else
	extern char	*vms_getenv();
#endif

/************************************************************************/

static make_line_item_labels()
{
	int	cur_index, iter, result;
	char	*cur_addr;

	for (iter = 0; iter < CONFIG_MAX_RF_CHAN; iter++) {
		cur_index = iter;
		cur_addr = &line_item_label[ cur_index ][ 0 ],
		result = P_getstring(
			 SYSTEMGLOBAL,
			"rfchlabel",
			 cur_addr,
			 iter+1,		/* arrayed parameters are */
			 40			/* indexed starting at 1  */
		);

	/*  Default to "RF Channel N" if no label in conpar.	*/

		if (result != 0)
		  sprintf( cur_addr, "RF Channel %d (Dec%d)", iter+1, iter );
		rf_chan_name[ iter ] = cur_addr;
	}
}


static int
getSysGlobalVals(fnptr)
char           *fnptr;
{
	int	r;

	r = P_read( SYSTEMGLOBAL, fnptr );
	if (r != 0) {
                fprintf( stderr,
	    "failed to read system global parameters from %s\n", fnptr
                );
                return( -1 );
        }
	init_rf_choices();
	init_grad_choices();

/*  Call this one last; its results depend on the results of Gradient choices.  */

	init_generic_choices();

	make_line_item_labels();
	return( 0 );
}

#ifndef VMS

/*  These programs might get called when VNMR config exits.
    The public program is configExit.

    configExit is called from the GUI programs.  It does not
    itself call exit, but returns to the GUI and assumes the
    GUI calls the exit program.

    Unlike the GUI version (configExit_sunview, etc.), this
    program assumes the current data should be written out.	*/

static
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

static int
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

	/***  h1freq_val variable is being reset in order to link      ***/
	/***  parameters for 85 and 100 Mhz systems to 200 Mhz.	       ***/
	if (h1freq_val < 200.0) h1freq_val = 200.0;

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

int
configExit()
{

/*  Write out configuration stuff to be sent to the console
    if it is a Hydra (UnityPlus) console.			*/

	if (is_cwb_file() && is_system_hydra()) {
		storeSystemPanelInConsole();
		storeRFPanelsInConsole();
		write_cwb_file();
	}

	update_generic_conpar();
	update_rf_conpar();
	update_grad_conpar();

/*  Since defining CONPAR_FILE implies debug mode, do not make these
    links in the VNMR system directory if CONPAR_FILE is defined.	*/

#ifndef CONPAR_FILE
	if (makestdparlink)
	  makelink("stdpar");
	if (maketestslink)
	  makelink("tests");
#endif
	do_final_fixes();
	putSysGlobalVals(conparfil);
}
#endif


/************************************************************************/

static int
display_params()
{
	display_generic_params( stdout );
	display_rf_params( stdout );
	display_grad_params( stdout );
}

static int
process_args( argc, argv, display_flag_addr, interact_addr, report_addr )
int argc;
char *argv[];
int *display_flag_addr;
int *interact_addr;
int *report_addr;
{
	int	iter;

	*display_flag_addr = 0;
	*interact_addr = 0;
	*report_addr = 131071;
	if (argc < 2)
	  return( 0 );

	iter = 0;						/* Start at 1 */
	while (++iter < argc ) {
		if (strcmp( argv[ iter ], "display" ) == 0 ||
		    strcmp( argv[ iter ], "print" ) == 0)
		  *display_flag_addr = 1;
		else if (strcmp( argv[ iter ], "interact" ) == 0 ||
			 strcmp( argv[ iter ], "interactive" ) == 0)
		  *interact_addr = 1;
		else if (strcmp( argv[ iter ], "interactive_try" ) == 0) {
			*interact_addr = 1;
			*report_addr = 0;
		}
		else if (strcmp( argv[ iter ], "cwbf" ) == 0) {
			iter++;
			if (argc <= iter) {
				fprintf( stderr,
				   "Console writeback file requires a file name\n"
				);
				fprintf( stderr,
				   "No configuration data will be written to the console\n"
				);
				return( -1 );
			}

			set_cwb_file( argv[ iter ] );
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

    A report on why it can't is optional; see the calling program.	*/

static int
check_interactive_access( report_flag, cmd_name )
int report_flag;
{
	char	conparlink[ MAXPATHL ];
	int	ival;

	if (access(&systemdir[ 0 ], W_OK) != 0) {
		if (report_flag)
		  fprintf( stderr,
    "cannot run %s interactively, no write access to %s\n",
     cmd_name, &systemdir[ 0 ]
		  );
		return( -1 );
	}

	if (access( &conparfil[ 0 ], W_OK) != 0) {
		if (report_flag)
		  fprintf( stderr,
    "cannot run %s interactively, no write access to %s\n",
     cmd_name, &conparfil[ 0 ]
		  );
		return( -1 );
	}
	return( 0 );
}

#endif


int
main( argc, argv )
int argc;
char *argv[];
{
	int		 display_flag, interact_flag, ival, report_flag;
	char		*quick_ptr;
	extern char	*getenv();

#ifndef VMS
	quick_ptr = getenv( "vnmrsystem" );
#else
	quick_ptr = vms_getenv( "vnmrsystem" );
#endif

	if (strlen( quick_ptr ) >= MAXPATHL-30) {
		fprintf( stderr,
	    "error starting VNMR CONFIG, value for 'vnmrsystem' is too long\n"
		);
		exit( 1 );
	}
	strcpy( &systemdir[ 0 ], quick_ptr );
	process_args( argc, argv, &display_flag, &interact_flag, &report_flag );

#ifdef CONPAR_FILE
	strcpy( &conparfil[ 0 ], CONPAR_FILE );
#else
#ifndef VMS
	makestdparlink = 131071;		/* Normally these links are */
	maketestslink = 131071; 		/* to be made if the command */
						/* is used interactively. */
	strcpy( &conparfil[ 0 ], &systemdir[ 0 ] );
	strcat( &conparfil[ 0 ], "/conpar" );

/*  If interactive mode was requested, verify necessary access exists.
    If the report flag is set the program is to report why it can't
    run interactively and then exit should this access not exist.

    If the report flag is clear, the program tries for interactive
    access.  If it is not successful, it displays parameter values.

    The default is display mode; print current values and exit.		*/

	if (interact_flag) {
		ival = check_interactive_access( report_flag, "VNMR config" );
		if (ival != 0)
		  if (report_flag)
		    exit( 1 );
		  else
		    interact_flag = 0;
	}
#else
	strcpy( &conparfil[ 0 ], &systemdir[ 0 ] );
	strcat( &conparfil[ 0 ], "conpar" );

	interact_flag = 0;		/* No interactive mode on VMS */
#endif
#endif
	if (getSysGlobalVals( &conparfil[ 0 ] ))
	  exit( 1 );

/* ---  print revision ID and Date, Compiled within revdate.c --- */
        P_setstring( SYSTEMGLOBAL, "rev", &RevID[ 0 ], 1);
        P_setstring( SYSTEMGLOBAL, "revdate", &RevDate[ 0 ], 1);

	if (display_flag || !interact_flag) {
		display_params();
	}
#ifdef SUN
	else {
		do_prelim_fixes();
		makeVconfigPanels( argc, argv );
		vconfigLoop();
	}
#endif
}
