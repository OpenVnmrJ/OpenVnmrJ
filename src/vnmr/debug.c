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
|	debug.c
|
|	This module contains various routines to set or clear the various
|	debugging flags.
|
+-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "vnmrsys.h"
#include "wjunk.h"

#define GRAPHONSIZE	19
#define MAXSHSTRING     1024

#ifdef DEBUG
extern int          Aflag;
extern int          Dflag;
extern int          Eflag;
extern int          Gflag;
extern int          Lflag;
extern int          Pflag;
extern int          Rflag;
extern int          Tflag;
extern int          Vflag;
#endif

int		    debug1 = 0;  /* debug flag for Rene */
extern int          M0flag; /* flag for reporting command abort */
extern int          Mflag; /* flag for tracing macros and commands */
extern int          M1flag; /* flag for tracing macros and commands */
extern int          M2flag; /* flag for tracing macros and commands */
extern int          M3flag; /* flag for tracing macros and commands */
extern int          MflagIndent; /* flag for tracing macros and commands */
extern int          MflagVJcmd;
extern int 			shelldebug(int t);
/*-----------------------------------------------------------------------------
|
|	debug
|
|	This routine turns on the debug flags
|
+-----------------------------------------------------------------------------*/

int debuger(int argc, char *argv[], int retc, char *retv[])
{   int i;

    (void) retc;
    (void) retv;
    if (argc == 1)
    {	Werrprintf("Usage -- debug(flag1,flag2,...)");
	return(0);
    }
    else
    {	for (i=1; i<argc ;i++)
	{   switch (*argv[i])
	    {
	      case 'c':
			if ( ! strcmp(argv[i],"ca"))
			{
			   MflagVJcmd = 1;
			}
			else if ( ! strcmp(argv[i],"c3"))
			{
			   Mflag = 0;
			   M1flag = M2flag = M3flag = 1;
			}
			else if ( ! strcmp(argv[i],"c2"))
			{
			   Mflag = 0;
			   M1flag = M2flag = 1;
			   M3flag = 0;
			}
			else if ( ! strcmp(argv[i],"c1"))
			{
			   Mflag = 0;
			   M1flag = 0;
			   M2flag = 1;
			   M3flag = 0;
			}
			else if ( ! strcmp(argv[i],"c0"))
			{
			   M0flag = 1;
			   M1flag = 0;
			   Mflag = 0;
			   M2flag = 0;
			   M3flag = 0;
			}
			else
			{
			   Mflag = 1;
			   M0flag = M1flag = M2flag = M3flag = 0;
			}
			MflagIndent = 0;
			Wscrprintf("Command and macro tracing turned on\n");
			Wscrprintf("Output is displayed in shell window\n");
			break;
	      case 'C':	M0flag = Mflag = M1flag = M2flag = M3flag = MflagVJcmd = 0;
			Wscrprintf("Command and macro tracing turned off\n");
			break;
	      case 't':
	         {
	        	 int t=0;
	    	     if(argv[i][1])
	    		     t=argv[i][1]-'0';
	    	     shelldebug(t);
	         }
	      	  break;
	      case 'T':
	    	  shelldebug(0);
	          break;
#ifndef DEBUG
	      default:  Wscrprintf("Flag '%s' not available. Only c and C are valid for debug\n",argv[i]);
#endif 
#ifdef DEBUG
                        /* debug1 flag - used for all sorts of VNMR routines debugging
			such as convert,dg,dli,dpcon,ds,fold,ft2d,
			init,integ,proc2d, and sky                 */
	      case '1':	if (debug1)
			{   debug1 = 0;
			    Wscrprintf("debug1 OFF\n");
			}
			else
			{   debug1 = 1;
			    Wscrprintf("debug1 ON\n");
			}
			break;

              /* Aflag is used to debug the acqhandler.  RL  06/01/87  */

	      case 'a':	Aflag += 1;
			Wscrprintf("Aflag level %d\n",Aflag);
			break;
	      case 'A':	Aflag -= 1;
			Wscrprintf("Aflag level %d\n",Aflag);
			break;
	      /* Dflag used for various interpreter debugging especially
			Exec, some variables, some io, and ops (operations
			such as +, - , etc.)				*/
	      case 'd':	Dflag += 1;
			Wscrprintf("Dflag level %d\n",Dflag);
			break;
	      case 'D':	Dflag -= 1;
			Wscrprintf("Dflag level %d\n",Dflag);
			break;
	      /* Eflag  Doesn't seem to be used at the moment */
	      case 'e':	Eflag += 1;
			Wscrprintf("Eflag level %d\n",Eflag);
			break;
	      case 'E':	Eflag -= 1;
			Wscrprintf("Eflag level %d\n",Eflag);
			break;
	      /* Greg Brissey's debug flag */
	      case 'g':	Gflag += 1;
			Wscrprintf("Gflag level %d\n",Gflag);
			break;
	      case 'G':	Gflag -= 1;
			Wscrprintf("Gflag level %d\n",Gflag);
			break;
	      /* Lflag is used for debugging of the lexer code	*/
	      case 'l':	Lflag += 1;
			Wscrprintf("Lflag level %d\n",Lflag);
			break;
	      case 'L':	Lflag -= 1;
			Wscrprintf("Lflag level %d\n",Lflag);
			break;
	      /* Pflag is used for debugging of the parser code	*/
	      case 'p':	Pflag += 1;
			Wscrprintf("Pflag level %d\n",Pflag);
			break;
	      case 'P':	Pflag -= 1;
			Wscrprintf("Pflag level %d\n",Pflag);
			break;
	      /* Rene Richarz's debug flag */
	      case 'r':	Rflag += 1;
			Wscrprintf("Rflag level %d\n",Rflag);
			break;
	      case 'R':	Rflag -= 1;
			Wscrprintf("Rflag level %d\n",Rflag);
			break;
	      /* Rick's debug flag used for debugging various routines
			such as buttons, builtin commands, handlers,
			macros, variables, shellcommands, sockets,
			terminal io, and wjunk (graphics)	*/
	      case 't':	Tflag += 1;
			Wscrprintf("Tflag level %d\n",Tflag);
			break;
	      case 'T':	Tflag -= 1;
			Wscrprintf("Tflag level %d\n",Tflag);
			break;
	      /* Vflag - Verbose mode. Show preloaded macros */
	      case 'v':	Vflag += 1;
			Wscrprintf("Vflag level %d\n",Vflag);
			break;
	      case 'V':	Vflag -= 1;
			Wscrprintf("Vflag level %d\n",Vflag);
			break;
	      default:  Wscrprintf( "There is no such \"%s\" flag, valid flags are 1,D,E,G,L,P,R,T,V\n",argv[i]);
#endif 
	    }
	}
    }
    RETURN;
}
