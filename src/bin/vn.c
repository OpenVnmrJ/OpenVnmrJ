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

/*---------------------------------------------------------------------------
|
|	vn   Varian Nmr Startup Program
|
|	This is the startup program of the varian unix nmr system.
|	It determines if we are on the sun or a graphics terminal
|	and starts the appropriate code.
|
|       When Vnmr has exited or crashed, this program attempts to
|       set the terminal back to a normal state (especially in the
|       case of a crash)
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   11/27/89   Greg B.    1. change main to find both master and Vnmr then
|			     execute the appropriate one. Prior to this
|			     only the Vnmr was used. This caused  problems
|			     when separate executables are needed for DBX
|			     or profiling.
|
/+-----------------------------------------------------------------------*/

#define  MAXEXPS	9
#define  MAXPATHL	128

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <varargs.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#if defined (IRIX) || (SOLARIS)
#include <termios.h>
#else
#include <sys/ttydev.h>
#endif

char           *terminal;
extern char    *getenv();
int             debug;
int             Normal;
int             bgmode;
int             argc2;
char           **argv2;
char           *Master = "master";
char           *Vnmr = "Vnmr";
char           *Debug = "-d";
#if defined (IRIX) || (SOLARIS)
static struct termios   tbufsave; /* Original terminal characteristics */
#else
static struct sgttyb    tbufsave;
#endif


main(argc, argv)
int             argc;
char           *argv[];

{
   char           *sys;
   char            vnmrstr[256];
   int             pid;

   fprintf(stderr,"VN: starting VNMR\n");
   argv2 = (char **)calloc(1, sizeof(char *) * (argc + 3));
   argv2[0] = Master;
   argv2[1] = Vnmr;
   argc2 = 2;
   parse_argv(argc, argv);

/* Determine if we are on the sun or a graphics terminal */

   terminal = getenv("graphics");
   if (terminal == 0)
   {
      printf("error: enviroment variable 'graphics' undefined'\n");
      exit( 1 );
   }
   if (strcmp(terminal, "sun") == 0 || strcmp(terminal, "suncolor") == 0)
   {
      /* Fork a child task */
      switch (fork())
      {
	 case -1:		/* some problem */
	    fprintf(stderr, "Problem creating child process\n");
	    break;

	    /* Note: the vnmrstr is the Vnmr that master will start. */
	 case 0:		/* The child */
	    execvp("Vnmr", argv2);
	    fprintf(stderr, "VN:  execv failed, something is really wrong\n");
	    break;

	 default:		/* the parent */
	    /* Wait for Vnmr to exit or crash */
	    signal(SIGINT, SIG_IGN);	/* parent ignore contro_c */
	    if (wait(NULL) == -1)
	       fprintf(stderr, "Problem with waiting for child\n");
      }
   }
   else
   {
      saveTerminal();
      /* Fork a child task */
      switch (fork())
      {
	 case -1:		/* some problem */
	    fprintf(stderr, "Problem creating child process\n");
	    break;

	 case 0:		/* The child */
	    execlp("Vnmr", "Vnmr", 0);
	    fprintf(stderr, "VN:  execv failed, something is really wrong\n");
	    break;

	 default:		/* the parent */
	    /* Wait for Vnmr to exit or crash */
	    signal(SIGINT, SIG_IGN);	/* parent ignore contro_c */
	    pid = wait(NULL);
	    if (pid == -1)
	       fprintf(stderr, "Problem with waiting for child\n");
	    else
	    {
	       unlockAllExp(getenv("vnmruser"), pid, 'f');
	    }
      }
      if (Normal)
	 normal();		/* reset terminal to normal conditions */
   }
   /* remove core, if one exists */
   if (access("core", F_OK | R_OK) == 0)
	 system("rm core");
}

/*  Find copy of master or Vnmr */

find_vnmr(basename, vnmrstr)
char           *basename;	/* basename for file (Vnmr, master) */
char           *vnmrstr;	/* Assumes the next 128 characters */
{				/* following this address are writeable */
   char           *cwd,
                  *getcwd();
   char           *sys;
   char            localstr[MAXPATHL];

/* get system directory */

   sys = getenv("vnmrsystem");
   if (sys == NULL)
   {
      printf("error: enviroment variable 'vnmrsystem' undefined'\n");
      exit( 1 );
   }
   if (strlen(sys) > MAXPATHL - 12)
   {
      printf("error: value of environment variable 'vnmrsystem' ");
      printf("has too many characters!!\n");
      exit( 1 );
   }

/*  Vn first looks in current working directory for the file Vnmr.    */
/*  If it doesn't find it, it looks in the system bin directory       */
/*  If user is on the SUN, it executes the Vnmr task with argv[0] set */
/*  to master.  The master task will then execute Vnmr.  If user is   */
/*  not on the Sun, this routine just executes Vnmr.		      */

   /* obtain absolute path of current working directory */
   if ((cwd = getcwd((char *) NULL, 130)) == NULL)
   {
      perror("find_vnmr: getcwd");
      exit(1);
   }
   strcpy(&localstr[0], cwd);
   strcat(&localstr[0], "/");
   strcat(&localstr[0], basename);
   if (access(&localstr[0], F_OK | R_OK) == -1)
   {
      if (debug)
	 fprintf(stderr, "vn: %s is not in current directory\n", basename);
      if (sys)
      {
	 strcpy(&localstr[0], sys);	/* copy over systemdir */
	 strcat(&localstr[0], "/bin/");
	 strcat(&localstr[0], basename);
      }
      if (access(&localstr[0], F_OK | R_OK) == -1)
      {
	 fprintf(stderr, "Cannot access '%s'.  It is in neither the\n", basename);
	 fprintf(stderr, "current directory or %s/bin\n", sys);
	 exit( 1 );
      }
   }

/*  Found it, somewhere.  */

   strcpy(vnmrstr, &localstr[0]);
}

/*  This routine serves to isolate the parsing of the command
    argument vector.  New feature allows background startup mode.  */

parse_argv(argc, argv)
int             argc;
char           *argv[];

{
   char           *p;
   int             iter;

/* If the first character of the command name is 'b' or 'B',
   assume background mode.  */

   if (*argv[0] == 'b' || *argv[0] == 'B')
      bgmode = 131071;
   else
      bgmode = 0;

   if (argc < 2)
      return;			/* No extra arguments */

   /* The following was copied from the main routine  12/16/87 */

   /* check for debug option */
   if (argc > 1 && *argv[1] == 'd')	/* The check of argc is */
      debug = 1;		/* no longer needed but */
   else				/* is being kept to     */
      debug = 0;		/* maintain continuity  */

   if (argc > 1 && *argv[1] == 'n')
      Normal = 0;
   else
      Normal = 1;

   /*
    * End of stuff copied from main routine. Now check for background
    * processing or anything else which is preceded with a '-' character
    */

   for (iter = 1; iter < argc; iter++)
   {
      if (strcmp(argv[iter],"d")== 0)
      {
	    debug = 1;
            argv2[argc2++] = Debug;
      }
      else
            argv2[argc2++] = argv[iter];
   }
   argv2[argc2] = NULL;
}


saveTerminal()
{
	int  stdinFd;

        stdinFd = fileno( stdin );
        if (stdinFd < 0) {
                perror( "error getting file descriptor for stdin" );
        }
#if defined (IRIX) || (SOLARIS)
        if (ioctl( stdinFd, TCGETA, &tbufsave ) != 0)
#else
        if (ioctl( stdinFd, TIOCGETP, &tbufsave ) != 0)
#endif
            perror( "Error reading terminal characteristics" );
}

/*-----------------------------------------------------------
|
|   Normal
|
|	This routine restores graphon and hds terminals to
|	hopefully their original state (what ever that is)
|	(may be New Jersey). It also resotres -cbreak and
|	echo to the terminal driver.
|+---------------------------------------------------------*/
char           *graphics;

normal()
{
#if !defined (IRIX) && !defined (SOLARIS)
   struct sgttyb   tbuf;
#endif

   graphics = getenv("graphics");

#if defined (IRIX) || (SOLARIS)
   if (ioctl(0,TCSETA,&tbufsave)==-1)  /* restore tty configuration */
      perror("ioctl2");
#else
   if (ioctl(0, TIOCGETP, &tbuf) == -1)
      perror("ioctl");
   tbuf.sg_flags &= ~CBREAK;
   tbuf.sg_flags |= ECHO;
   if (ioctl(0, TIOCSETN, &tbuf) == -1)
      perror("ioctl2");
#endif
   Wresterm("");
}





int             active = 3;	/* keep track of active window */

/*----------------------------------------------------------------------
|
|	Wisgraphon/0    Wishds/0  Wissun/0
|
|	These procedures return a 1 if the terminal is set to the
|	particular brand (graphon or hds).
|
+----------------------------------------------------------------------*/


Wisgraphon()
{
   if (strcmp(graphics, "graphon") == 0)
      return 1;
   else
      return 0;
}

Wishds()
{
   if (strcmp(graphics, "hds") == 0)
      return 1;
   else
      return 0;
}

/* delete a window from the terminal */
static 
killwindow(window)
int             window;
{
   printf("\033[%d;1;0;629;0;0;0\"w", window);	/* kill window  */
   fflush(stdout);		/* Send this out right away */
}

/* create a window in the terminal */
/* setup window. plane=1 or 2, window 1-4, firstrow,lastrow 1-26
     Note: windows cannot overlap in planes  */
static 
setwindow(plane, window, firstrow, lastrow)
int             plane,
                window,
                firstrow,
                lastrow;
{
   printf("\033[%d;%d;1;%d;%d;0;0\"w", window, plane,
	  779 - (firstrow - 1) * 30, 779 - (lastrow - 1) * 30);
   fflush(stdout);		/* Send this out right away */
}

Wsetactive(window)		/* selects active window, window=1-4 */
int             window;		/* if window=2, graphics plane is turned on,
				 * alpha turned off */
{
   /* if(window==2) /* turn on graphic window */
   /* Wgmode(); */
   /* else */
   {
      if (Wisgraphon())
      {
	 printf("\033[%dw", window);	/* select window */
      }
      if (Wishds())
      {				/* if(active == 2) /* if we were in graphics */
	 printf("\030");	/* goto alpha */
	 printf("\033[%d;1!w", window);	/* select the window */
	 printf("\033[%d;9!w", window);	/* move cursor to window */
      }
   }
   /* active = window; /* keep track of window */
   fflush(stdout);		/* Send this out right away */
}
static 
setjump()
{
   printf("\033[?4l");		/* turn off smooth scroll */
}

static 
setwrap()
{
   printf("\033[?7h");		/* turn autowrap on */
}

/* setup window for hds */
static 
hds_setwindow(window, top, bot)
int             window;
int             top;
int             bot;
{
   Wsetactive(window);		/* select window first */
   printf("\033[%d;%d;1;80w", top, bot);
}


Wclear(window)
int             window;
{
   if (window < 1 || 4 < window)
      return;
   if (Wisgraphon())
   {
      printf("\033[%dw", window);	/* make window active temporarily */
      if (window != 2)
	 printf("\033[2J");
      else
	 printf("\033\014");	/* clear screen (ESC FF or ESC ctrl-L) */
      fflush(stdout);		/* Send this out right away */
   }
   if (Wishds())
   {
      if (window != 2)
      {
	 printf("\033[%d;0!w", window);	/* make window active temporarily */
	 printf("\033[2J");
      }
      else
      {
	 printf("\033[1+z");
	 printf("\030");
      }
      fflush(stdout);
   }
}



/*----------------------------------------------------------------------
|
|	Wresterm/1
|
|	This procedure trys to reset terminal to original setting before
|	the interpreter was executed.
|
+----------------------------------------------------------------------*/

/* retore terminal to original setting */
Wresterm(mes)
char           *mes;
{
   if (Wisgraphon())
   {
      Wclear(2);
      killwindow(1);
      killwindow(3);
      killwindow(4);
      setwindow(1, 1, 1, 24);
      Wsetactive(1);
      setwrap();		/* set autowrap on */
      setjump();		/* set jump scroll */
      fprintf(stderr, "%s\n", mes);
   }
   else if (Wishds())
   {
      hds_setwindow(1, 1, 24);	/* reset windows to window 1, 1-24 rows */
      printf("\033[=125h");	/* Blank out inactive display memory */
      printf("\033[3+|");	/* Turn off graphics */
      printf("\033[0+|");	/* Turn on alpha */
      Wclear(2);		/* clear the screen */
      printf("\030");		/* go to alpha mode if necessary */
      fprintf(stderr, "%s\n", mes);
      Wclear(1);
   }
   else
      fprintf(stderr, "%s\n", mes);
}

/* This routine is to be called by the parent process when its VNMR child
   exits.  It users the path contained in userptr to locate each experiment
   and the pid and mode arguments to construct the lock file names.	*/

int 
unlockAllExp(userptr, pid, mode)
char           *userptr;
int             pid;		/* If mode == 'a', this is the PID of Acqproc */
char            mode;
{
   char            fname[MAXPATHL];
   int             iter,
                   ulen;

   ulen = strlen(userptr);
   if (ulen < 1 || ulen > MAXPATHL - 10)
      return (1);

   strcpy(&fname[0], userptr);
   for (iter = 1; iter <= MAXEXPS; iter++)
   {
      sprintf(&fname[0], "%s/exp%d/%c.%d", userptr, iter, mode, pid);
      unlink(&fname[0]);
   }
}
