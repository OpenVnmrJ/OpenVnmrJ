/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  This file contains VNMR commands for automated operation of the
    spectrometer.							*/

#include  "vnmrsys.h"
#include  "group.h"
#include  <stdio.h>
#include  <ctype.h>
#include  <unistd.h>
#include  <stdlib.h>
#include  <string.h>
#include  <sys/file.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  "acquisition.h"
#include  "pvars.h"
#include  "wjunk.h"

#define  OK		0

#define  BAD_ENTRY	-2
#define  INCOMPLETE	-3
#define  SAMPLE_NUM_ERR	-4

#ifdef  DEBUG
extern int      debug1;
#define DPRINT0(str)		if (debug1) Wscrprintf(str)
#define DPRINT1(str, arg1)	if (debug1) Wscrprintf(str,arg1)
#else 
#define DPRINT0(str)
#define DPRINT1(str, arg1)
#endif 

extern char *get_cwd();
extern char *W_getInput(char *prompt, char *s, int n);
extern void doingAutoExp();
extern int  cexpCmd(int argc, char *argv[], int retc, char *retv[]);
extern int ok_to_acquire();
extern void report_acq_perm_error(char *cmdname, int errorval);
extern int setAutoDir(char *str);
extern int send2Acq(int cmd, char *msg_for_acq);
extern int isDirectory(char *filename);
extern int release_console2();

static int create_autoexps(char *);

/*  The name "auto" cannot be used because that is a word reserved
    for use by the C compiler.
    This command does NOT have to be entered on the SUN console.	*/

int autocmd(int argc, char *argv[], int retc, char *retv[])
{
	char	autodir[ MAXPATH+2 ], autoinfofil[ MAXPATH ],
		inputstr[ MAXPATH ];
	char    *pathptr;
	int	autodir_defined;
	int	l, r;
	FILE	*autoinfoptr;

/*  This process must be able to write to $vnmrsystem/conpar...  */

	disp_status( "AUTO    " );

/*  Establish current value of AUTODIR  */

	autodir[ 0 ] = '\0';		/*  Start with 0-length string */
	if (argc < 2) 			/*  No arguments, just command  */
	{
		r =  P_getstring(
		    GLOBAL, "autodir", &autodir[ 0 ], 1, MAXPATH
		);
		autodir_defined = (r == 0);
		if ( autodir_defined )
		  autodir_defined = !(strlen( &autodir[ 0 ] ) < 1);

		if ( !autodir_defined )
		  W_getInput( "Enter automation directory name: ",
			&inputstr[ 0 ], MAXPATH
		  );
		else
		{
                        /*
			 * Pass a separate string to W_getInput so as to avoid 
	    		 * overwriting AUTODIR if a <CR> gets typed.
                         */
			Winfoprintf(
			        "Automation directory is '%s'", &autodir[ 0 ] );
			W_getInput( "Enter new name, or <CR> to keep current: ",
				&inputstr[ 0 ], MAXPATH
			);
			if (inputstr[ 0 ] == '\0')
			   strcpy(&inputstr[ 0 ], &autodir[ 0 ]);
		}
	}
	else 				/*  Argument is value for AUTODIR  */
	{
		strcpy( &inputstr[ 0 ], argv[ 1 ] );
	}
	l = strlen( &inputstr[ 0 ] );
	if ( l <= MAXPATH-1)
        {
		if (inputstr[ 0 ] != '/')
		{
			pathptr = get_cwd();
			l += strlen( pathptr ) + 1;
			if (l < MAXPATH-1 )
			{  strcpy(&autodir[ 0 ], pathptr);
			   strcat(&autodir[ 0 ], "/");
			   strcat(&autodir[ 0 ], &inputstr[ 0 ]);
			}
		}
		else
		   strcpy( &autodir[ 0 ], &inputstr[ 0 ] );
        }
	if ( l > MAXPATH-1)
	{
		Werrprintf(
	    "Too many characters in full pathname of automation directory"
		);
		disp_status( "        " );
		ABORT;
	}
/*  Verify that 'autodir' is not the 0-length string; verify
    it is an absolute UNIX path name  */

	else if (l < 1)
	{
		Werrprintf("Error: value of automation directory not defined" );
		disp_status( "        " );
		ABORT;
	}

/*  Make the automation directory if not present.  All users get full
    access, barring umask.  Abort if a problem.  If using the SUN console,
    the PERROR output will go to the (hidden or closed) shelltool
    window - too bad!!	*/

	if (access( &autodir[ 0 ], 0 ) != 0)
	  if ( (r = mkdir( &autodir[ 0 ], 0777 )) )
	  {
		Werrprintf( "Error creating automation directory" );
		perror( "make automation directory" );
		disp_status( "        " );
		ABORT;
	  }

/*  Now this process must be able to read, write and execute (search)
    the automation directory.						*/

	if (access( &autodir[ 0 ], R_OK | W_OK | X_OK ))
	{
		Werrprintf( "Insufficent access to automation directory" );
		disp_status( "        " );
		ABORT;
	}

/*  Create special automation experiments, exp1 through exp4  */

	if (create_autoexps( &autodir[ 0 ] ) != 0) ABORT;

/*  Displayed status may be changed by creating experiments.  */

	disp_status( "AUTO    " );
	strcpy( &autoinfofil[ 0 ], &autodir[ 0 ] );
	strcat( &autoinfofil[ 0 ], "/autoinfo" );
	autoinfoptr = fopen( &autoinfofil[ 0 ], "w+" );
	if (autoinfoptr == NULL )
	{
		Werrprintf( "Error creating %s", &autoinfofil[ 0 ] );
		disp_status( "        " );
		ABORT;
	}
	fclose( autoinfoptr );			/* Someday this file will  */
						/* contain useful information */
/*  Ignore error from setstring, for now.  */

	r = P_setstring( GLOBAL, "autodir", &autodir[ 0 ], 1 );
	disp_status( "        " );
	RETURN;
}

/*  Note that the following routine checks for error conditions
 *  and produce a message if an error occurs.
 */

/*  I made this a separate routine so as to allow the automation
    experiments to be created without having to call the "auto"
    command, which will interrogate the user for the pathname to
    the automation root directory.					*/

static int create_autoexps( char *autodir )
{
   char	digit;
   char	autoexp[ MAXPATH ], cexp_arg1[ 2 ], saved_userdir[ MAXPATH ];
   char	*cexp_argvec[ 3 ];
   int	iter, l, r;

/*  Create special automation experiments, exp1 through exp4  */

   strcpy( &saved_userdir[ 0 ], userdir );		/* Prepare... */
   strcpy( userdir, autodir );			/* Sinful... */

/*  Avoid wasting stack space, time in the executive, etc. by calling
    CEXP directly.  Note that we are screwing around (sinning) by
    changing USERDIR to point to the automation directory so cexp
    roots the special experiments in the automation directory...

    Check if the automation experiment exists and silently continue if so.

    The cexp command may fail for a variety of reasons, including disk
    partition full.  Restore correct value for USERDIR before aborting.
    We assume the CEXP routine squawks if it fails.			*/

   strcpy( &autoexp[ 0 ], &autodir[ 0 ] );
   strcat( &autoexp[ 0 ], "/expX" );
   l = strlen( &autoexp[ 0 ] );

   cexp_argvec[ 0 ] = "cexp";
   cexp_argvec[ 1 ] = &cexp_arg1[ 0 ];
   cexp_argvec[ 2 ] = NULL;
   for (iter = 0; iter < 4; iter++)
   {
      digit = iter+'1';
      autoexp[ l-1 ] = digit;
      if (access( &autoexp[ 0 ], 0 ) != 0)
      {
          /*
           * doingAutoExp sets a flag in cexp which tells it
           * not to add the experiment to the shuffler
           */
         doingAutoExp();
	 cexp_arg1[ 0 ] = digit;
	 cexp_arg1[ 1 ] = '\0';
	 if ( (r = cexpCmd( 2, &cexp_argvec[ 0 ], 0, NULL )) )
	 {
	    strcpy( userdir, &saved_userdir[ 0 ] );
	    ABORT;
	 }
      }

	/*  Remain silent if the automation experiment exists...
	    But set protection to 0777 (barring umask) in either case.	*/

      chmod( &autoexp[ 0 ], 0777 );
   }
   strcpy( userdir, &saved_userdir[ 0 ] );		/* Sin no more... */
   RETURN;
}

int autora(int argc, char *argv[], int retc, char *retv[])
{
        char   tmpPath[MAXPATH];
        int    ival;

	DPRINT0("autora command invoked\n"); /* resume is implicit */
	ival = ok_to_acquire();
        if (ival != -2)
           release_console2();
	if ( (ival != 0) && (ival != -4))
        {
           /* Acquisition needs to be Idle or in automation mode */
	   report_acq_perm_error( argv[0], ival );
	   ABORT;
        }
        if (argc == 1)
        {
           strcpy(tmpPath,userdir);
           strcat(tmpPath,"/global");
           DPRINT1("copying Global parameters to '%s' file.\n",tmpPath);
           if (P_save(GLOBAL,tmpPath))
           {   Werrprintf("Problem saving global parameters in '%s'.",tmpPath);
           }
        }

	P_getstring(GLOBAL,"autodir",&tmpPath[ 0 ], 1, MAXPATH);
        setAutoDir(tmpPath);
        if (send2Acq(11, tmpPath) < 0)
                Werrprintf("autora message was not sent");
	RETURN;
}

int resume(int argc, char *argv[], int retc, char *retv[])
{
        char   resume_arg[MAXPATH];
	DPRINT0("resume command invoked\n");
        GET_ACQ_ID(resume_arg);
   	if (send2Acq(13, resume_arg) < 0)
		Werrprintf("resume message was not sent");
	RETURN;
}

/*  Dummy autosa command  */

int autosa(int argc, char *argv[], int retc, char *retv[])
{
	DPRINT1( "%s command invoked\n", argv[ 0 ] );
        if (argc > 1)
        {
           if (send2Acq(12, "1") < 0)
                Werrprintf("autosa message was not sent");
        }
        else
        {
           if (send2Acq(12, "0") < 0)
                Werrprintf("autosa message was not sent");
        }
        RETURN;
}

/*  See the comments at the top of this file where the values returned
    by this routine are given symbolic definitions.			*/

/*  Verify the existence of the automation queue file.  First argument of
    check_autoqueue is so VNMR command can identify itself.		*/

static int check_autoqueue(char *cmdptr, char *autoqptr, char *macdirptr )
{
	FILE	*quein;
        char basename[MAXPATH];
        char *bPtr;

        macdirptr[0] = '\0';
        if (isDirectory(autoqptr))
        {
           bPtr = strrchr(autoqptr, '/');
           if (bPtr == NULL)
           {
              strcpy(basename,autoqptr);
           }
           else
           {
              bPtr++;
              strcpy(basename,bPtr);
           }
           sprintf(macdirptr,"%s/%s.macdir",autoqptr,basename);
           strcat(autoqptr,"/");
           strcat(autoqptr,basename);
        }
	quein = fopen( autoqptr, "r" );
	if (quein == NULL) {
		Werrprintf( "%s:  cannot open queue file %s",
			cmdptr, autoqptr );
		ABORT;
	}
	fclose( quein );	/* File needs to be closed, error or not */
	RETURN;
}

int autogo(int argc, char *argv[], int retc, char *retv[])
{
/*  The argument vector and return vector are used to call the
    "auto" and "autora" commands of VNMR.				*/

	char	enterfile[ MAXPATH ], newautodir[ MAXPATH ],
		macdir[MAXPATH], cp_autoq_cmd[ MAXPATH*2+4 ];
	char	*cmd_argvec[ 3 ];

	disp_status( "AUTOGO  " );

/*  Establish if path to automation queue is specified in the command (the
    first argument).  If not, ask for it.  Abort if none provided.	*/

	if (argc > 1) {
		if (strlen(argv[1]) > MAXPATH-1)
                {
			Werrprintf(
		    "%s: automation queue pathname too long", argv[ 0 ]
			);
			disp_status( "        " );
			ABORT;
		}
		else
		  strcpy( &enterfile[ 0 ], argv[ 1 ] );
	}
	else {
		enterfile[ 0 ] = '\0';	/* In case nothing is entered */
		W_getInput( "Location of automation queue: ",	
			&enterfile[ 0 ], MAXPATH
		);
	}
	if (strlen( &enterfile[ 0 ] ) < 1) {
		Werrprintf(
		    "%s:  automation queue pathname not specified", argv[ 0 ]
		);
		disp_status( "        " );
		ABORT;
	}

/*  This routine produces an error message if it detects an error.	*/

	if (check_autoqueue( argv[ 0 ], &enterfile[ 0 ], &macdir[0] ))
           ABORT;

/*  Now check if the command specifies the automation directory  */

	if (argc > 2) {
		cmd_argvec[ 0 ] = "auto";
		cmd_argvec[ 1 ] = argv[ 2 ];
		cmd_argvec[ 2 ] = NULL;
                /*
		 *  Command is:  auto('/usr2/auto')
    		 *  It will complain if it encounters a problem.
                 */
	}
	else {
		cmd_argvec[ 0 ] = "auto";
		cmd_argvec[ 1 ] = NULL;
	}

/*  The "auto" command clears the displayed status.  */

	if (autocmd( argc-1, &cmd_argvec[ 0 ], 0, NULL))
		ABORT;
	disp_status( "AUTOGO  " );

/*  Copy the enter queue file into the automation directory.  Borrow the string
    "newautodir" to store the value of this directory.  Its value should be
    explicitly retrieved now, since it may not have been defined earlier, or
    its value may have changed during the execution of the "autogo" command. */

	P_getstring(
		GLOBAL, "autodir", &newautodir[ 0 ], 1, MAXPATH
	);

/*  Borrow string "cp_autoq_cmd" to construct system enter queue.  Abort if
    the file exists and is of non-zero length.				*/

	sprintf( &cp_autoq_cmd[ 0 ], "%s/enterQ", &newautodir[ 0 ] );
	if (access( &cp_autoq_cmd[ 0 ], 0 ) == 0)
	{
		struct stat	buf;

		if (stat( &cp_autoq_cmd[ 0 ], &buf ) != 0)
		{
			Werrprintf(
	    "%s:  cannot access existing automation queue", argv[ 0 ]
			);
			disp_status( "        " );
			ABORT;
		};
		if (buf.st_size > 0)
		{
			Werrprintf(
	    "%s:  current automation queue is not empty", argv[ 0 ]
			);
			disp_status( "        " );
			ABORT;
		}
	}

	sprintf( &cp_autoq_cmd[ 0 ], "%s/gQ", &newautodir[ 0 ] );
	if (access( &cp_autoq_cmd[ 0 ], F_OK ) == 0)
	{
		if ( unlink( &cp_autoq_cmd[ 0 ] ) )
		{
			Werrprintf( "%s:  cannot remove %s", argv[ 0 ], &cp_autoq_cmd[ 0 ] );
			disp_status( "        " );
			ABORT;
		}
	}

	sprintf( &cp_autoq_cmd[ 0 ], "cp %s %s/enterQ",
		&enterfile[ 0 ], &newautodir[ 0 ]);
	system( &cp_autoq_cmd[ 0 ] );
        if (macdir[0] != '\0')
        {
	   sprintf( &cp_autoq_cmd[ 0 ], "cp -rf %s %s/enterQ.macdir",
		&macdir[ 0 ], &newautodir[ 0 ]);
	   system( &cp_autoq_cmd[ 0 ] );
        }

	cmd_argvec[ 0 ] = "autogo";
	cmd_argvec[ 1 ] = NULL;

	if (autora( 1, &cmd_argvec[ 0 ], 0, NULL )) ABORT;

	disp_status( "        " );
	RETURN;
}
