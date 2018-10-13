/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------------------------------------
|
|	help.c
|
|	This module contains various routines to help the poor user.
|	It contains the help command that provides relavent information
|	about the current buttons and mouse keys.
|
+-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "vnmrsys.h"
#include "vfilesys.h"
#include "buttons.h"
#include "wjunk.h"

#ifdef UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#else 
#define  R_OK	4
#endif 

#define GRAPHONSIZE	19
#define MAXSHSTRING     1024

extern int More(FILE *stream, int screenLength);
extern int default_search_seq[];

/*---------------------------------------------------------------------------
|
|	help
|
|	This command displays a file that contains definitions of the screen 
|	buttons and the mouse buttons that are active at the time help 
|	is executed.
|	On the sun, a scrollable pop-up window is used, on the graphon,
|	the file is displayed with popen and More. 
|
|	The file that is displayed is determined by retrieving the help
|	file name from the Wactivate_button routine help file name variable
|	help_name.  If there is nothing in help_name, use the name "default".
|	The file is searched for as determined by appdir.
|
+---------------------------------------------------------------------------*/

int help(int argc, char *argv[], int retc, char *retv[])
{
   char	        fileName[MAXPATHL];
   char	        filePath[MAXPATHL];
   char		cmdstr[MAXSHSTRING];
   extern char  help_name[];
   FILE	       *stream, *popen_call();

      (void) argc;
      (void) argv;
      (void) retc;
      (void) retv;
      if (help_name[0] == '\0')		/* use a default name */
	 strcpy(fileName, "default" );
      else				/* use help_name name */
	 strcpy(fileName,help_name);

      filePath[ 0 ] = '\0';
      appdirFind(fileName,"help",filePath,NULL,R_OK);
      if (strlen(filePath) < 1 || access(filePath,R_OK) < 0)
      {
	 Werrprintf("help file %s does not exist",fileName);
	 ABORT;
      }
      else
      {   

/*  VMS requires an extension to the file name, even if it is null;
    otherwise, the TYPE command assumes .LIS, which causes problems.  */

#ifdef VMS
         strcat(filePath,".");
#endif 
	 if ( !Wissun() )			/* If using a terminal, stop */
           Wturnoff_buttons();			/* interactive display. */
         Wsettextdisplay("Help");
	 Wshow_text();
	 Wclear_text();
#ifdef UNIX
	 strcpy(cmdstr,"/bin/cat ");
#else 
	 strcpy(cmdstr,"type ");
#endif 
	 strcat(cmdstr,filePath);                    /* Start shell cmd */
         if ((stream = popen_call(cmdstr,"r"))  == NULL)  /* & pipe output */
         {  Werrprintf("Problem with creating shell command with popen");
            ABORT;
         }
	 else
	 {  More(stream,WscreenSize());  /* more it out to screen */
	    RETURN;
	 }
      }		/* found a help file */
}

