/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/******************************************************************************
* File vnmrclock.c:  Contains "adapters" between vnmr and the clocktime       *
*      -----------   module.  The "clocktime" modules may be called from vnmr *
*		     using these routines.  Note that it does not make much   *
*		     sense to use "clock_start" and "clock_stop" unless the   *
*		     event to be timed is rather long, on the order of        *
*		     several seconds or more.				      *
******************************************************************************/

#include <string.h>
#include <stdio.h>
#include "vnmrsys.h"

char *malloc();

/***************
* See bootup.c *
***************/
extern int psg_pid;

/***********************************************************************
* "Convenience" variables associated with the vnmr clocktime interface *
***********************************************************************/

/* Timer number used for ft, typically 1. */
int ft_timer_no;
/* Timer number used for ds, typically 4. */
int ds_timer_no;
/* Timer number used for dcon, typically 5. */
int dcon_timer_no;
/* Timer number used for fitspec, typically 6. */
int fitspec_timer_no;
/* Timer number used for spins, typically 7. */
int spins_timer_no;
/* Timer number used for jexp, typically 8 or 9. */
int jexp_timer_no;
/* Timer used for go, typically 10-14 */
int go_timer_no;
/* Timer number for the dummy (short) ft package timeft, typically 16 or 17 */
int dummyft_timer_no;

/* Default period (in seconds) between tests for process existence */
/* Used by wait_on_psg						   */
#define DEFAULT_PERIOD 2

/******************************************************************************/
int clock_init ( argc, argv )
/* 
Purpose:
-------
     Routine clock_init is an adapter routine, used to call clocktime function
init_timer, which in turn initializes the clocktime system.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc  :  (I  )  Argument count.  Should be two.
argv  :  (I  )  Command line arguments.  Should contain the name of this 
		routine, followed by the number of timers to initialize.
*/
int argc;
char *argv[];

{ /* Begin function clock_init */
   /*
   Local Variables:
   ---------------
   no_timers  :  Number of timers to initialize.
   retval     :  Return value of the clocktime routine called.
   */
   int no_timers, retval;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Check arguments for validity */
   if ( argc != 2 )
   {  Werrprintf ( "Usage:  %s no_timers'", argv[0] );
      ABORT;
   }
   if ( sscanf ( argv[1], "%d", &no_timers ) != 1 )
   {  Werrprintf ( "%s: Bad numeric field '%s'", argv[0], argv[1] );
      ABORT;
   }

   /* Call the clocktime routine to do the work */
   if ( ( retval = init_timer ( no_timers ) ) != 0 )
   {  Werrprintf ( "%s: clocktime error %d", argv[0], retval );
      ABORT;
   }

   /* Normal successful return */
   RETURN;

} /* End function clock_init */

/******************************************************************************/
int clock_report ( argc, argv )
/* 
Purpose:
-------
     Routine clock_report is an adapter routine, used to call clocktime 
function report_timer, which in turn writes a report and terminates the 
clocktime system.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc  :  (I  )  Argument count.  Should be two.
argv  :  (I  )  Command line arguments.  Should contain the name of this 
		routine, followed by the name of an output file, followed by
		a title variable (see clocktime document for explanation).
*/
int argc;
char *argv[];

{ /* Begin function clock_report */
   /*
   Local Variables:
   ---------------
   output_file  :  Name of output file for clocktime routine.
   title        :  Title variable for clocktime routine.
   retval       :  Return value of the clocktime routine called.
   */
   char *output_file, *title;
   int retval;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Check arguments for validity */
   if ( argc > 3 )
   {  Werrprintf ( "Usage:  %s [output_file] [title]'", argv[0] );
      ABORT;
   }
   if ( argc < 3 || strcmp(argv[2],"0") == 0 )
      title = (char *)0;
   else
      title = argv[2];
   if ( argc < 2 || strcmp(argv[1],"0") == 0 )
      output_file = "clocktime.out";
   else
      output_file = argv[1];

   /* Call the clocktime routine to do the work */
   if ( ( retval = report_timer ( output_file, title ) ) != 0 )
   {  Werrprintf ( "%s: clocktime error %d", argv[0], retval );
      ABORT;
   }

   /* Normal successful return */
   RETURN;

} /* End function clock_report */

/******************************************************************************/
int clock_start ( argc, argv )
/* 
Purpose:
-------
     Routine clock_start is an adapter routine, used to call clocktime function
start_timer, which in turn starts a timer.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc  :  (I  )  Argument count.  Should be two.
argv  :  (I  )  Command line arguments.  Should contain the name of this 
		routine, followed by the number of the timer to start.
*/
int argc;
char *argv[];

{ /* Begin function clock_start */
   /*
   Local Variables:
   ---------------
   itimer  :  Number of the timer to start.
   retval  :  Return value of the clocktime routine called.
   */
   int itimer, retval;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Check arguments for validity */
   if ( argc != 2 )
   {  Werrprintf ( "Usage:  %s itimer'", argv[0] );
      ABORT;
   }
   if ( sscanf ( argv[1], "%d", &itimer ) != 1 )
   {  Werrprintf ( "%s: Bad numeric field '%s'", argv[0], argv[1] );
      ABORT;
   }

   /* Call the clocktime routine to do the work */
   if ( ( retval = start_timer ( itimer ) ) != 0 )
   {  Werrprintf ( "%s: clocktime error %d", argv[0], retval );
      ABORT;
   }

   /* Normal successful return */
   RETURN;

} /* End function clock_start */

/******************************************************************************/
int clock_stop ( argc, argv )
/* 
Purpose:
-------
     Routine clock_stop is an adapter routine, used to call clocktime function
stop_timer, which in turn stops a timer.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc  :  (I  )  Argument count.  Should be two.
argv  :  (I  )  Command line arguments.  Should contain the name of this 
		routine, followed by the number of the timer to stop.
*/
int argc;
char *argv[];

{ /* Begin function clock_stop */
   /*
   Local Variables:
   ---------------
   itimer  :  Number of the timer to stop.
   retval  :  Return value of the clocktime routine called.
   */
   int itimer, retval;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Check arguments for validity */
   if ( argc != 2 )
   {  Werrprintf ( "Usage:  %s itimer'", argv[0] );
      ABORT;
   }
   if ( sscanf ( argv[1], "%d", &itimer ) != 1 )
   {  Werrprintf ( "%s: Bad numeric field '%s'", argv[0], argv[1] );
      ABORT;
   }

   /* Call the clocktime routine to do the work */
   if ( ( retval = stop_timer ( itimer ) ) != 0 )
   {  Werrprintf ( "%s: clocktime error %d", argv[0], retval );
      ABORT;
   }

   /* Normal successful return */
   RETURN;

} /* End function clock_stop */

/******************************************************************************/
int set_clock_var ( argc, argv )
/* 
Purpose:
-------
     Routine set_clock_var allows convenience variables associated with the Vnmr
clocktime interface to be changed.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc  :  (I  )  Argument count.
argv  :  (I  )  Command line arguments.
*/
int argc;
char *argv[];

{ /* Begin function set_clock_var */
   /*
   Local Variables:
   ---------------
   number    :  Value to assign to one of the convenience variables.
   variable  :  The convenience variable to assign to.
   argvi     :  One of the input araguments argv[i].
   name_end  :  Index into argument indicating end of name of convenience
		variable to be set.
   */
   char *argvi;
   int number, name_end, *variable;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* If user just wants list of convenience variables */
   if ( argc < 2 )
   {
      Wscrprintf ( "ft_timer_no=%d ds_timer_no=%d dcon_timer_no=%d\n",
		   ft_timer_no, ds_timer_no, dcon_timer_no );
      Wscrprintf ( "fitspec_timer_no=%d spins_timer_no=%d jexp_timer_no=%d\n",
		   fitspec_timer_no, spins_timer_no, jexp_timer_no );
      Wscrprintf ( "go_timer_no=%d, dummyft_timer_no=%d\n", 
		   go_timer_no, dummyft_timer_no );
   }
   /* Else, user wants to set some variables */
   else
   {
      /* Loop over the arguments supplied */
      while  ( --argc > 0 )
      {
	 /* Search for the equals sign in the assignment string */
	 argvi = *(++argv);
	 for ( name_end = 0 ; argvi[name_end] != 0 && argvi[name_end] != '=' ; 
	    name_end++ )
	 ;
	 if ( argvi[name_end] == 0 )
	 {  Werrprintf ( "Usage:  %s <varname>=<number>", argv[0] );
	    ABORT;
	 }

	 /* Choose the convenience variable to assign to */
	 if ( strncmp ( "jexp_timer_no", argvi, name_end ) == 0 )
	    variable = &jexp_timer_no;
	 else if ( strncmp ( "ft_timer_no", argvi, name_end ) == 0 )
	    variable = &ft_timer_no;
	 else if ( strncmp ( "ds_timer_no", argvi, name_end ) == 0 )
	    variable = &ds_timer_no;
	 else if ( strncmp ( "dcon_timer_no", argvi, name_end ) == 0 )
	    variable = &dcon_timer_no;
	 else if ( strncmp ( "fitspec_timer_no", argvi, name_end ) == 0 )
	    variable = &fitspec_timer_no;
	 else if ( strncmp ( "spins_timer_no", argvi, name_end ) == 0 )
	    variable = &spins_timer_no;
	 else if ( strncmp ( "go_timer_no", argvi, name_end ) == 0 )
	    variable = &go_timer_no;
	 else if ( strncmp ( "dummyft_timer_no", argvi, name_end ) == 0 )
	    variable = &dummyft_timer_no;
	 else
	 {  Werrprintf ( "Unrecognized vnmrclock variable in expression %s", 
			 argvi );
	    ABORT;
	 }

	 /* Decode the number value to assign */
	 if ( sscanf ( argvi+(name_end+1), "%d", &number ) != 1 )
	 {  Werrprintf ( "Illegal numerical expression %s", 
			 argvi+(name_end+1) );
	    ABORT;
	 }

	 /* Make the variable assignment */
	 *variable = number;

      }  /* End loop over the arguments supplied */

   }  /* End if user just wants a list of convenience variables */

   /* Normal successful return */
   RETURN;

} /* End function set_clock_var */

/******************************************************************************/
int wait_on_psg ( argc, argv )
/* 
Purpose:
-------
     Routine wait_on_psg shall wait for psg to finish.  If psg is no longer 
active, it returns immediately.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc  :  (I  )  Argument count.
argv  :  (I  )  Command line arguments.  There is one (optional) argument:  The
		number of seconds to sleep between checks for the existence of
		the psg process.
*/
int argc;
char *argv[];

{ /* Begin function wait_on_psg */
   /*
   Local Variables:
   ---------------
   period   :  Amount of time to wait (in seconds) between tests, for continued
               existence of process to wait upon.
   */
   int period;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Check number of arguments */
   if ( argc > 2 )
   {  Werrprintf ( "Usage:  wait_on_psg [test-period]" );
      ABORT;
   }
   /* Obtain waiting period between tests */
   if ( argc == 2 )
   {  if ( sscanf ( argv[1], "%d", &period ) != 1 || period <= 0 )
      {  period = DEFAULT_PERIOD;
         Werrprintf ( "Bad test period '%s', using default of %d seconds",
		      argv[2], period );
      }
   }  
   else
      period = DEFAULT_PERIOD;
      
   /* Check to see if psg really exists, using "kill" */
   if ( kill ( psg_pid, 0 ) != 0 )
   {  Werrprintf ( "Process %d does not exist", psg_pid );
      ABORT;
   }
   /* Message to user */
   disp_status ( "WAIT_PSG" );

   /* Wait on the process */
   top:
      sleep ( (unsigned)period );
      if ( kill ( psg_pid, 0 ) == 0 )
         goto top;
 
   /* Parting message to user */
   /* Winfoprintf ( "Process %d finished", psg_pid ); */

   /* Clear message to user */
   disp_status ( "        " );

   /* Normal successful return */
   RETURN;

} /* End function wait_on_psg */

/******************************************************************************/
int set_env_var ( argc, argv )
/* 
Purpose:
-------
     Routine set_env_var shall place a value in the Unix process environment 
table.

David Arnstein
Spectroscopy Imaging Systems Corporation
Fremont, California

Arguments:
---------
argc  :  (I  )  Argument count.
argv  :  (I  )  Command line arguments.  One for each environment variable to 
		set.
*/
int argc;
char **argv;

{ /* Begin function set_env_var */
   /*
   Local Variables:
   ---------------
   space_ptr  :  Pointer to new space to store environment variable.
   */
   char *space_ptr;
   /*
   Begin Executable Code:
   ---------------------
   */
   /* Loop over command line arguments */
   while ( --argc > 0 )
   {
      /* Allocate space */
      if ( ( space_ptr = malloc ( (unsigned)(strlen(*(++argv))+1) ) ) == 0 )
      {  Werrprintf ( "set_env_var: malloc fails" );
	 ABORT;
      }
      /* copy the argument over */
      (void)strcpy ( space_ptr, *argv );

      /**********************************************************************
      * Note:  The above string copy is necessary because the command line  *
      * ----   arguments may disappear, and putenv (below) causes its 	    *
      *        argument to become part of the process environment, thus, it *
      *	       should not disappear!					    *
      **********************************************************************/

      /* Change the environment table */
      if ( putenv ( space_ptr ) != 0 )
      {  Werrprintf ( "set_env_var: putenv fails" );
	 ABORT;
      }

   }  /* End loop over command line arguments */

   /* Normal successful return */
   RETURN;

} /* End function set_env_var */
