/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------------------
|
|    shellcmds
|
|    This module contains some basic shell commands 
|
|    Includes modifications for VMS.  No effect on SUN function
|    Because some arrays were added for VMS, the object code is
|    different.    RL  09/05/88
|
|    Modified the `Shell' subroutine to redirect standard input to
|    /dev/null (on UNIX) if the VNMR command contained arguments.
|    Fixes problem with VNMR command  shell('cat')   RL  11/15/88
|
|    Added `fgets_nointr' static subroutine    RL  08/09/1994
|
+----------------------------------------------------------------------------*/

#include "vnmrsys.h"
#include "group.h"
#include "tools.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>
#define __USE_XOPEN 1
#include <time.h>

#include "pvars.h"
#include "wjunk.h"

#ifdef UNIX
#include <sys/file.h>
#else 
#define F_OK	0
#define W_OK	2
#define R_OK	4
#endif 

#ifdef SUN
#define TOOL	"vnmrshell"
#endif 

#define MAXSHSTRING	1024
#define GRAPHONSIZE	19
#define SUNSIZE		16
#define CATT 	0
#define CD	1
#define CP	2 
#define LF	3
#define MKDIR	4 
#define MV	5 
#define PWD	6
#define RM	7
#define RMANY	8
#define RMDIR	9
#define SHELL	10
#define W	12
#define SHELLI	13
#define RMFILE	14
#define UNAME  15
#define HNAME  16

#ifdef  DEBUG
extern int Tflag;
#define TPRINT0(str) \
	if (Tflag) fprintf(stderr,str)
#define TPRINT1(str, arg1) \
	if (Tflag) fprintf(stderr,str,arg1)
#define TPRINT2(str, arg1, arg2) \
	if (Tflag) fprintf(stderr,str,arg1,arg2)
#else 
#define TPRINT0(str) 
#define TPRINT1(str, arg2) 
#define TPRINT2(str, arg1, arg2) 
#endif 

static char t_format[] = "%Y\%m\%dT%H\%M\%S";

#ifdef X11
extern  char  Xserver[];
#endif 
#ifdef VNMRJ
extern  char  VnmrJHostName[];
#endif 

static char  chkbuf[MAXPATH+2];

extern FILE *popen_call(char *cmdstr, char *mode);
extern int   pclose_call(FILE *pfile);
extern char *check_spaces(char *s1, char *s2, int len);
extern int   sendTripleEscToMaster(char code, char *string_to_send );
extern void  Wturnoff_buttons();
extern char *W_getInput(char *prompt, char *s, int n);
extern void  restoreInput();
extern void  setInputRaw();
extern void  system_call(char *s);
static int   getcmd(char *cmd);
static void  Shellret(FILE *stream, int retc, char *retv[]);
int More(FILE *stream, int screenLength);

/*---------------------------------------------------------------------------
|
|    shellcmds
|
|    This module contains some basic shell commands 
|
+----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------
|
|    getcmd
|
|	This routine returns a integer corresonding to a shell command.
|	This integer is used by a case statement.
|
/+---------------------------------------------------------------------*/

static int getcmd(char *cmd)
{   if ( ! strcmp(cmd,"cat") )
	return CATT;
    if ( ! strcmp(cmd,"cd") )
	return CD;
    if ( ! strcmp(cmd,"cp") || ! strcmp(cmd,"copy") )
	return CP;
    if ( ! strcmp(cmd,"lf") || ! strcmp(cmd,"ls")
                      || ! strcmp(cmd,"dir") )
	return LF;
    if ( ! strcmp(cmd,"mkdir") )
	return MKDIR;
    if ( ! strcmp(cmd,"mv") || ! strcmp(cmd,"rename") )
	return MV;
    if ( ! strcmp(cmd,"pwd") )
	return PWD;
    if ( ! strcmp(cmd,"rm") )
	return RM;
    if ( ! strcmp(cmd,"delete") )
	return RMANY;
    if ( ! strcmp(cmd,"rmfile") )
	return RMFILE;
    if ( ! strcmp(cmd,"rmdir") )
	return RMDIR;
    if ( ! strcmp(cmd,"shell") )
	return SHELL;
    if ( ! strcmp(cmd,"shelli") )
	return SHELLI;
    if ( ! strcmp(cmd,"uname") )
	return UNAME;
    if ( ! strcmp(cmd,"host") )
	return HNAME;
    if ( ! strcmp(cmd,"w") )
	return W;
    return -1;
}

/*  This little program helps work around mystery failures of the shell
    command.  For example, shell('date'):n1 would return no output to
    the Magical varaible n1, instead of the expected data/time string.
    It appears that fgets sometimes returned NULL, with errno set to
    EINTR.  (Perhaps a child process exited, sending a SIGCHLD signal.)
    If fgets returns NULL here, the program will try again, unless errno
    is set to something other that EINTR or EOF has been raise on the
    file pointer passed as an argument to the program.   08/09/1994	*/

char *
fgets_nointr(char *datap, int count, FILE *filep )
{
	char	*qaddr;

	while ( (qaddr = fgets( datap, count, filep )) == NULL) {
                if (errno != EINTR || (feof(filep)))
		  return( NULL );
	}

	return( qaddr );
}

// execute thread-safe shell command
int exec_shell(char *cmd) {
	FILE *stream;
	int num_chars_out=0;
	char data[1024];
	data[0] = 0;
	if ((stream = popen_call(cmd, "r")) != NULL) {
		// wait for shell to exit
		char *p = fgets_nointr(data, 1024, stream);
		while (p != NULL) {
			int len = strlen(data);
			if (len > 0) {
				// display any echo or print statement issued from shell
				if (data[len - 1] == '\n')
					data[len - 1] = '\0';
				Winfoprintf("%s", data);
			}
			num_chars_out+=len;
			p = fgets_nointr(data, 1024, stream);
		}
		pclose_call(stream);
	}
	return num_chars_out; // return number of chars printed from shell
}
char  *get_cwd()
{
    static char    data[4096];

     return( getcwd(data,4096) );
}

/*---------------------------------------------------------------------------
|
|    chDir
|
|    This module changes the current directory 
|
+----------------------------------------------------------------------------*/

int ch_dir(int argc, char *argv[], int retc, char *retv[])
{    char        *p;

    if ( argc == 1) /* change to home directory */
    {
#ifdef UNIX
	if ( (p = getenv("HOME")) )
	{   if (chdir(p) == -1)
	    {   Werrprintf("Error changing directory to \"HOME\"");
		ABORT;
	    }
	    else
	    {
               if (retc < 1)
                  Winfoprintf("Directory now \"%s\"",p);
               else
	          retv[ 0 ] = newString( p );
		RETURN;
	    }
	}
	else
	{   Werrprintf("Problem finding HOME variable, may not exist");
	    ABORT;
	}
#else 
	if (chdir( "SYS$LOGIN:" ) == -1 )
	{
	    Werrprintf( "Error changing directory to SYS$LOGIN:" );
	    ABORT;
	}
	else
	{
	    Winfoprintf( "Current default directory now SYS$LOGIN:" );
	    RETURN;
	}
#endif 
    }
    else
    {   if (chdir(argv[1]) == -1)
	{   Werrprintf("Problem changing directory to \"%s\"",argv[1]);
	    RETURN;
	}
	else
	{
	    p = get_cwd();
            if (retc < 1)
               Winfoprintf("Directory now \"%s\"",p);
            else
	       retv[ 0 ] = newString( p );
	    RETURN;
	}
    }
}

/*---------------------------------------------------------------------------
|
|    Whoinfo
|
|    This routine does a standard w command 
|
+----------------------------------------------------------------------------*/

int Whoinfo(int argc, char *argv[], int retc, char *retv[])
{
    char	*cmdstr;
    FILE	*stream;

    Wshow_text();
#ifdef UNIX
    cmdstr = "/usr/bin/who";
#else 
    cmdstr = "SHOW USERS";
#endif 
    if ((stream = popen_call( cmdstr, "r")) == NULL)
    {  Werrprintf("Problem with creating shell command with popen");
       ABORT;
    }
    More(stream,WscreenSize());   /* more it out to the screen */
    RETURN;
}

/*---------------------------------------------------------------------------
|
|    Uname
|
|    This routine returns a uname string
|
+----------------------------------------------------------------------------*/
int Uname(int argc, char *argv[], int retc, char *retv[]) {
	static char osname[256] = { 0 };
	if (strlen(osname) == 0) {
                struct utsname buf;

                uname( &buf );
                strcpy(osname,buf.sysname);
	}
	if (retc > 0) {
		retv[0] = newString(osname);
	} else {
		Winfoprintf("uname = %s", osname);
	}
	RETURN;
}

/*---------------------------------------------------------------------------
|
|    Uname
|
|    This routine returns a a host name string
|
+----------------------------------------------------------------------------*/
int Hname(int argc, char *argv[], int retc, char *retv[]) {
	static char host[256] = { 0 };
	if (strlen(host) == 0) {
                struct utsname buf;

                uname( &buf );
                strcpy(host,buf.nodename);
	}
	if (retc > 0) {
		retv[0] = newString(host);
	} else {
		Winfoprintf("host = %s", host);
	}
	RETURN;
}

/*---------------------------------------------------------------------------
|
|    Pwd
|
|    This module shows the present working directory 
|
+----------------------------------------------------------------------------*/

int Pwd(int argc, char *argv[], int retc, char *retv[])
{
    char *tmpptr;

   /*
    tmpptr = getcwd( &path[ 0 ], sizeof( path ) - 1 );
   */
    tmpptr = get_cwd();
    if (tmpptr == NULL) {
	Werrprintf( "%s:  no current directory  (did you remove it?)", argv[ 0 ] );
        if (retc)
	   retv[ 0 ] = newString( "" );
	RETURN;
    }

    if (retc < 1) {
	Winfoprintf( "Current directory = \"%s\"", tmpptr );
    }
    else {
	retv[ 0 ] = newString( tmpptr );
    }

    RETURN;
}


/*------------------------------------------------------------------------------
|
|	Shell
|
|	This procedure resets the terminal, and execs /bin/csh.  When
|	the shell dies, it will reset up the vnmr screens
|
+-----------------------------------------------------------------------------*/

#define MAXSHSTRING	1024

/*
static char *my_argv[] = { 0 , 0 };
*/

int Shell(int argc, char *argv[], int retc, char *retv[])
{  char  cmdstr[1024];   
   int   i;
   int   interactive;
   int   screenLength;
   FILE *stream;
   int   background;

/*  shell command is interactive if:
    1) argc is 1, implying no arguments OR
    2) command is shelli, retc is 0 (implying no return values), and
       Vnmr is not running in a GUI environment.

    2nd condition added for editing on display terminals:

      shelli('vnmredit my.new.file')
*/

   interactive = (argc == 1 ||
                  (strcmp( argv[ 0 ], "shelli" ) == 0 && !Wissun() && retc == 0));
   if (interactive)			  /* interactive shell */
   {
      Wturnoff_buttons();
#ifdef SUN
      if (Wissun())
      {
#ifdef X11
         strcpy(cmdstr,TOOL);
         if (Xserver[0] != '\0')
         {
              strcat(cmdstr," -display ");
              strcat(cmdstr,Xserver);
         }
         strcat(cmdstr," -name VNMR -T UNIX ");
         system_call(cmdstr);
#else 
	 system_call(TOOL);
#endif 
         setTtyInputFocus();
      
         /*my_argv[0] = "csh";
         popUpTty(argc,my_argv,NULL);
	 */
      } 
      else
#endif 
      {
         if (argc > 1)
         {
            cmdstr[ 0 ] = '\0';
            for (i=1; i<argc; i++)
            {  if (i > 1)				/* added 03/28/90 */
                 strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
               strncat(cmdstr,argv[i],MAXSHSTRING - strlen(cmdstr) -1);
            }
         }
         Wshow_text();
         Wresterm("Setting up Shell,  please wait....");
         restoreInput(); /* set tty driver back to cooked mode */
         if (argc > 1)
           system_call( &cmdstr[ 0 ] );
         else
#ifdef __INTERIX
             system_call("/bin/ksh");
#else
           system_call("/bin/csh");
#endif
         Wsetupterm();
         setInputRaw();  /* return tty driver to raw mode */
         Winfoprintf("Resuming session");
         RETURN;
      }
   }
   else  /* non-interactive shell */
   {
#ifdef UNIX

/*  If `shell' is called with arguments, it appears preferable
    to pass the arguments directly to system_call or popen rather
    than start up a separate copy of /bin/csh.  Thus the next
    3 lines were removed 09/07/90.				*/

/*    strcpy(cmdstr,"/bin/csh -f ");
      if ( 1 <= argc)
         strncat(cmdstr," -c \"",MAXSHSTRING - strlen(cmdstr) -1);	*/

      cmdstr[ 0 ] = '\0';
      for (i=1; i<argc; i++)
      {  if (i > 1)					/* added 03/28/90 */
           strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
         strncat(cmdstr,argv[i],MAXSHSTRING - strlen(cmdstr) -1);
      }

/*  Redirect standard input to come from /dev/null, so the command will
    receive EOF if it attempts to read anything.  Keeps VNMR commands
    such as shell('cat') from locking up the VNMR program.		*/

      background = (cmdstr[strlen(cmdstr) - 1] == '&'); /* background shell */
      if (background)
         cmdstr[strlen(cmdstr) - 1] = ' ';

/*  It is also necessary to remove the terminating double quote.  09/07/90  */

/*    strncat(cmdstr,"\" </dev/null",MAXSHSTRING - strlen(cmdstr) -1);  */
      strncat(cmdstr," </dev/null",MAXSHSTRING - strlen(cmdstr) -1);
#ifdef __INTERIX
      background=0; // can't fork shells in windows (causes viewport crash)
#endif
      if (background)
         strncat(cmdstr," &",MAXSHSTRING - strlen(cmdstr) -1);
#else 
      cmdstr[ 0 ] = '\0';
      for (i=1; i<argc; i++)
      {
	strcat(cmdstr,argv[i]);
	if (i+1 < argc) strcat(cmdstr," ");
      }
      background = (cmdstr[strlen(cmdstr) - 1] == '&'); /* background shell */
#endif 

      if (background)  /* background shell */
      {
         system_call(cmdstr);
      }
      else
      {
         /* Startup the shell command and pipe in the output */
         if ((stream = popen_call(cmdstr,"r"))  == NULL)
         {  Werrprintf("Problem with creating shell command with popen");
            ABORT;
         }
         if (retc == 0)
         {
            Wscrprintf("\n"); /* do a CRLF for seperation */
            screenLength = WscreenSize();
            if (More(stream,screenLength))  /* more it out to screen */
               Wshow_text();
         }
         else
         {
            Shellret(stream,retc,retv);
         }
      }
   }
   RETURN;
}

/*------------------------------------------------------------------------------
|
|	lf
|
|	This procedure  does a standard lf type command.
|
+-----------------------------------------------------------------------------*/

int Lf(int argc, char *argv[], int retc, char *retv[])
{   char  cmdstr[1024];   
    int   i,len;
    int   screenLength;
    FILE *stream;

#ifdef UNIX
   strcpy(cmdstr,"/bin/ls -C -F");
#else 
   strcpy(cmdstr,"dir ");
#endif 
   for (i=1; i<argc; i++)
   {  strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
      len = MAXSHSTRING - strlen(cmdstr) -1;
      strncat(cmdstr,check_spaces(argv[i],chkbuf,len),len);
   }
   Wshow_text();
   screenLength = WscreenSize();
   /* startup the lf command and pipe in the output */
   TPRINT1("Lf:cmdstr is '%s'\n",cmdstr);
   if ((stream = popen_call(cmdstr,"r"))  == NULL)
   {  Werrprintf("Problem with creating lf command with popen");
      ABORT;
   }
   Wscrprintf("\n"); /* do a CRLF for seperation */
   if (!More(stream,screenLength)) /* more it out to screen */
   {  Werrprintf("No such files. Sorry!!!");
      ABORT;
   }
   else
      RETURN;
}

/*------------------------------------------------------------------------------
|
|	Cp
|
|	This procedure  does a standard cp command.
|
+-----------------------------------------------------------------------------*/

int Cp(int argc, char *argv[], int retc, char *retv[])
{   char cmdstr[MAXSHSTRING];
    int  i,len;
    int res;

#ifdef UNIX
    strcpy(cmdstr,"/bin/cp ");
#else 
    if (argc != 3) {
      Werrprintf( "%s:  use exactly 2 arguments", argv[ 0 ] );
      ABORT;
    }
    strcpy(cmdstr,"copy ");
#endif 
    for (i=1; i<argc; i++)
    {
	if (verify_fname( argv[ i ] ) != 0)
	{
	    Werrprintf( "%s:  cannot use '%s' as a file name", argv[ 0 ], argv[ i ] );
#ifdef VNMRJ
	    writelineToVnmrJ("invalidfile",argv[i]);
#endif 
	    ABORT;
	}
	strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
	len = MAXSHSTRING - strlen(cmdstr) -1;
	strncat(cmdstr,check_spaces(argv[i],chkbuf,len),len);
    }
    res = access(argv[ argc - 2 ],R_OK);
    TPRINT1("cp: command string \"%s\"\n",cmdstr);
    if ( !res )
    {
       strncat(cmdstr," 2> /dev/null", MAXSHSTRING - strlen(cmdstr) - 14);
       system_call(cmdstr);
    }
    else if ( !retc )
    {
       Werrprintf( "%s: input file '%s' does not exist", argv[ 0 ], argv[ argc - 2 ] );
    }
    if (retc)
    {
       if (res)
       {
	  retv[ 0] = realString((double) 0);
       }
       else
       {
          res = access(argv[ argc - 1 ],R_OK);
	  retv[ 0] = realString( (res) ? (double) 0 : (double) 1);
       }
    }
    else if ( !res )
    {
       res = access(argv[ argc - 1 ],R_OK);
       if (res)
          Werrprintf( "%s: could not write to file '%s'", argv[ 0 ], argv[ argc - 1 ] );
    }
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	CpTree
|
|	This procedure  does a standard cp -r command.
|
+-----------------------------------------------------------------------------*/

int CpTreE(int argc, char *argv[], int retc, char *retv[])
{   char cmdstr[1024];   
    int  i;

#ifdef UNIX
    strcpy(cmdstr,"/bin/cp -r ");
    for (i=1; i<argc; i++)
    {	strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
     	strncat(cmdstr,argv[i],MAXSHSTRING - strlen(cmdstr) -1);
    }
    TPRINT1("CpTreE: command string \"%s\"\n",cmdstr);
    system_call(cmdstr);
    RETURN;
#else 
    Werrprintf( "%s:  command not supported, sorry!", argv[ 0 ] );
#endif 
}

/*------------------------------------------------------------------------------
|
|	Cat
|
|	This procedure  does a standard cat command.
|
+-----------------------------------------------------------------------------*/

int Cat(int argc, char *argv[], int retc, char *retv[])
{  char  cmdstr[1024];   
   int   i;
   int   screenLength;
   FILE *stream;
   char *tptr1;
   char  tfilepath[MAXPATH];
   extern int printOn;

    if (argc == 1) /* This is a no no, must have arguments for cat */
    {	Werrprintf("Error: no arguments. Usage--cat('file')");
	ABORT;
    }
   /* determine if files exists and is readable by user */
   for (i=1; i<argc; i++)
      if (access(argv[i],F_OK|R_OK) == -1 || strlen(argv[i]) < 1)
      {  Werrprintf("File '%s' not accessible",argv[i]);
         ABORT;
      }
#ifdef UNIX
    strcpy(cmdstr,"/bin/cat ");
#else 
    strcpy(cmdstr,"typ ");
#endif 
    for (i=1; i<argc; i++)
    {
#ifdef UNIX
	strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);  /* why??? */
     	strncat(cmdstr,argv[i],MAXSHSTRING - strlen(cmdstr) -1);
#else 
        char *tptr2;

	strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1); /* why??? */
     	strncat(cmdstr,argv[i],MAXSHSTRING - strlen(cmdstr) -1);
        tptr1 = strrchr(argv[i],']');
	if (tptr1 == NULL)
	  tptr1 = argv[i];
	tptr2 = strrchr(tptr1,'.');
	if (tptr2 == NULL)
	  strncat(cmdstr,".",MAXSHSTRING - strlen(cmdstr) -1);
	if (i+1<argc)
	  strncat(cmdstr,",",MAXSHSTRING - strlen(cmdstr) -1);
#endif 
    }
    Wsettextdisplay("Cat");
    Wshow_text();
    screenLength = WscreenSize();

   /* Startup the cat command and pipe in the output */
   if (Wissun() && !printOn)
   {
        for (i=1; i<argc; i++)
        {
	    if (argv[i][0] != '/')
            {
		/*
                if (getcwd( &tfilepath[ 0 ], sizeof( tfilepath ) - 1 ) == NULL)
		*/
		if (( tptr1 = get_cwd()) == NULL)
                {
                     Werrprintf( "Cannot obtain current working directory" );
                     ABORT;
                }
		sprintf(tfilepath, "%s/", tptr1);
                strcat(tfilepath, argv[i]);
            }
            else
                strcpy(tfilepath, argv[i]);
            sendTripleEscToMaster( 'P', tfilepath);
        }
        RETURN;
   }

   if ((stream = popen_call(cmdstr,"r"))  == NULL)
   {  Werrprintf("Problem with creating cat command with popen");
      ABORT;
   }
   Wscrprintf("\n"); /* do a CRLF for seperation */
   More(stream,screenLength);  /* more it out to screen */
   RETURN;
}

/*------------------------------------------------------------------------
|
|	Shellret
|
|	This procedure reads from a pipe that has been created by popen
|       and returns the output to a macro.
|
+-----------------------------------------------------------------------------*/
static void Shellret(FILE *stream, int retc, char *retv[])
{  char      *p;
   char       string[1024];
   int        args;

   args = retc;
   while ( (p = fgets_nointr(string,1024,stream)) )
   {
      int len;

      len = strlen(string);
      if (string[len - 1] == '\n')
          string[len - 1] = '\0';
      if (retc)
      {
	 retv[ args - retc ] = newString( p );
         retc--;
      }
   } 

   pclose_call(stream);
}

/*------------------------------------------------------------------------
|
|	More
|
|	This procedure produces a more like feature.  It reads from a pipe
|	that has been created by popen and controls the output. More
|       returns the number of lines it has displayed.
|
+-----------------------------------------------------------------------------*/
int More(FILE *stream, int screenLength)
{  char      *p;
   char       string[1024];
   extern int printOn;
   int        line_num;
   int        num;
   int        something;

   something = 0; /* set something to  nothing */
   line_num = 1;
   p = fgets_nointr(string,1024,stream);
   if (p)
   {
      something = 1;
#ifdef SUN
      if (Wissun() && !printOn) setTextAtBottom();
#endif 
   }
   while (p)
   {  Wscrprintf("%s",p);
      line_num++;
      /* This will not do more if print is on */
      if (line_num > screenLength && !printOn && !Wissun() && !Bnmr)
      {  W_getInput("More[rows/y/n]?>",string,sizeof(string)); 
         if (*string == 'Y' || *string == 'y') 
	    line_num = 1;
         else
	 if ((num = atoi(string)) > 0)
	       line_num = screenLength - num + 1;
         else
         {  pclose_call(stream);
	    return (something);
         }
      }
      p = fgets_nointr(string,1024,stream);
   } 
   pclose_call(stream);
   return (something);
}

/*------------------------------------------------------------------------------
|
|	mv
|
|	This procedure  does a standard mv command.
|
+-----------------------------------------------------------------------------*/

int Mv(int argc, char *argv[], int retc, char *retv[])
{   char cmdstr[1024];   
    int  i,len;

#ifdef UNIX
    strcpy(cmdstr,"/bin/mv");
#else 
    if (argc != 3) {
      Werrprintf( "%s:  use exactly 2 arguments", argv[ 0 ] );
      ABORT;
    }
    strcpy(cmdstr,"rename ");
#endif 
    for (i=1; i<argc; i++)
    {
	if (verify_fname( argv[ i ] ) != 0)
	{
	    Werrprintf( "%s:  cannot use '%s' as a file name", argv[ 0 ], argv[ i ] );
#ifdef VNMRJ
	    writelineToVnmrJ("invalidfile",argv[i]);
#endif 
	    ABORT;
	}
	strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
	len = MAXSHSTRING - strlen(cmdstr) -1;
	strncat(cmdstr,check_spaces(argv[i],chkbuf,len),len);
    }
    TPRINT1("mv: command string \"%s\"\n",cmdstr);
    system_call(cmdstr);
    RETURN;
}

/* convert string to octal */
static int str_to_octal(char *ptr )
{
  int i, fac=1, len, ret=0;
  char *cptr;
  len = strlen( ptr );
  if ((len < 1) || (len > 4))
    ABORT;
  for (i=1; i<len; i++)
    fac *= 8;
  for (i=0; i<len; i++)
  {
    cptr = ptr + i;
    switch (*cptr)
    {
	case '0': case ' ': break;
	case '1': ret += fac * 1; break;
	case '2': ret += fac * 2; break;
	case '3': ret += fac * 3; break;
	case '4': ret += fac * 4; break;
	case '5': ret += fac * 5; break;
	case '6': ret += fac * 6; break;
	case '7': ret += fac * 7; break;
	default:
	  Werrprintf("str_to_octal: invalid character '%c'\n", *cptr);
	  ABORT;
	  break;
    }
    fac /= 8;
  }
  return( ret );
}

/*------------------------------------------------------------------------------
|
|	mk_dir_args - process -p -m mode arguments for mk_dir
|
+-----------------------------------------------------------------------------*/
static int mk_dir_args(int *mode, int *psub, int *ict, int argc, char *argv[])
{
  int i;

  if ((argc<3) || (argv[1][0] != '-'))
    RETURN;
  for (i=1; i<argc; i++)
  {
    *ict = i;
    if (argv[i][0] != '-')
      RETURN;
    if (strcmp(argv[i], "-p") == 0)
      *psub = 1;
    if (argv[i][1] == 'm')
    {
      int len, tmpmode;
      len = strlen(argv[i]);
      if (len > 3)
      {
	tmpmode = str_to_octal(argv[i] + 3);
        if ((tmpmode < 0500) || (tmpmode > 0777))
/*        if ((tmpmode < 320) || (tmpmode > 511)) */
	  Werrprintf("mkdir: invalid mode '%s', reset to 777\n",argv[i] + 3);
        else
	  *mode = tmpmode;
      }
    }
  }
  RETURN;
}

/*------------------------------------------------------------------------------
|
|	mkdir
|
|	This procedure  does a standard mkdir command.
|
+-----------------------------------------------------------------------------*/

int mk_dir(int argc, char *argv[], int retc, char *retv[])
{   char cmdstr[1024];   
    int  i, ict=1, mode=0, psub=0;

    mk_dir_args(&mode, &psub, &ict, argc, argv);
    for (i=ict; i<argc; i++)
    {
	int	ival;

	if (verify_fname( argv[ i ] ) != 0)
	{
	    Werrprintf(
		"%s:  cannot use '%s' as a directory name", argv[ 0 ], argv[ i ]
	    );
#ifdef VNMRJ
	    writelineToVnmrJ("invalidfile",argv[i]);
#endif 
	    ABORT;
	}
/*	ival = mkdir( argv[ i ], 0777 ); */
	if (psub == 1)
	{
          if (mode)
	     sprintf(cmdstr, "mkdir -m %o -p %s", mode, check_spaces(argv[i],chkbuf,1005));
          else
	     sprintf(cmdstr, "mkdir -p %s", check_spaces(argv[i],chkbuf,1005));
	  system_call( cmdstr );
          ival = access( argv[i], F_OK );
	}
	else
	  ival = mkdir( argv[ i ], 0777 );
        if (retc)
        {
	   retv[ 0 ] = intString( (ival) ? 0 : 1 );
        }
	else if (ival != 0)
        {
           Werrprintf( "Cannot create '%s' subdirectory", argv[ i ] );
	}
    }
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	rm
|
|	This procedure  does a standard rm command.
|
+-----------------------------------------------------------------------------*/

int Rm(int argc, char *argv[], int retc, char *retv[])
{   char cmdstr[1024];   
#ifdef VMS
    char *ival;
#endif
    int  i,len;

#ifdef UNIX
    strcpy(cmdstr,"/bin/rm");
#else 
    strcpy(cmdstr,"delete ");
#endif 
    for (i=1; i<argc; i++)
    {
        strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
	len = MAXSHSTRING - strlen(cmdstr) -1;
	strncat(cmdstr,check_spaces(argv[i],chkbuf,len),len);
#ifdef VMS
	ival = strchr( argv[ i ], ';' );
	if (ival == NULL)
	  strncat(cmdstr,";",MAXSHSTRING - strlen(cmdstr) -1 );
#endif 
    }
    TPRINT1("rm: command string \"%s\"\n",cmdstr);
    system_call(cmdstr);
    for (i=1; i<argc; i++)
      if (argv[i][0] != '-')
      {
          if ( access(argv[i],F_OK) == 0)
	  {
             Werrprintf("cannot delete %s",argv[i]);
#ifdef VNMRJ
	     writelineToVnmrJ("nodelete",argv[i]);
#endif 
	  }
	  else if (!retc)
          {
	     Winfoprintf("rm: %s",argv[i]);
          }
      }
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	RmTree
|
|	This procedure  does a standard rm -r  command.
|       It removes the directory and all files under it.
|
+-----------------------------------------------------------------------------*/

int RmTree(int argc, char *argv[], int retc, char *retv[])
{   char cmdstr[1024];   
    int  i;

#ifdef UNIX
    strcpy(cmdstr,"/bin/rm -r ");
    for (i=1; i<argc; i++)
    {	strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
     	strncat(cmdstr,argv[i],MAXSHSTRING - strlen(cmdstr) -1);
    }
    TPRINT1("rm: command string \"%s\"\n",cmdstr);
    system_call(cmdstr);
    for (i=1; i<argc; i++)
      if (argv[i][0] != '-')
          if ( access(argv[i],F_OK) == 0)
	  {
             Werrprintf("cannot delete %s",argv[i]);
#ifdef VNMRJ
	     writelineToVnmrJ("nodelete",argv[i]);
#endif 
	  }
    RETURN;
#else 
    Werrprintf( "%s:  command not supported, sorry!", argv[ 0 ] );
#endif 
}

/*------------------------------------------------------------------------------
|
|	rmdir
|
|	This procedure  does a standard rmdir command.
|
+-----------------------------------------------------------------------------*/

int rm_dir(int argc, char *argv[], int retc, char *retv[])
{   char cmdstr[1024];   
    int  i,len;

#ifdef UNIX
    strcpy(cmdstr,"/bin/rmdir");
    for (i=1; i<argc; i++)
    {
        strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
	len = MAXSHSTRING - strlen(cmdstr) -1;
	strncat(cmdstr,check_spaces(argv[i],chkbuf,len),len);
    }
    TPRINT1("rmdir: command string \"%s\"\n",cmdstr);
    system_call(cmdstr);
#else 
    for (i=1; i<argc; i++)
      rmdir( argv[i] );
#endif 
    for (i=1; i<argc; i++)
      if (argv[i][0] != '-')
      {
          if ( access(argv[i],F_OK) == 0)
	  {
             Werrprintf("cannot delete %s",argv[i]);
#ifdef VNMRJ
	     writelineToVnmrJ("nodelete",argv[i]);
#endif 
	  }
	  else if (!retc)
          {
	    Winfoprintf("rmdir: %s",argv[i]);
          }
       }
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	del_file2
|
|	This procedure deletes any directory or file
|	(.fid or .par must be specified)
|
+-----------------------------------------------------------------------------*/

int del_file2(int argc, char *argv[], int retc, char *retv[])
{   char cmdstr[MAXPATH+20], jstr[3*MAXPATH];
    char filename[MAXPATH];
#ifdef VNMRJ
    char hostarg[MAXPATH], objtype[32];
#endif 
    int  i,j;

    if (argc < 2)
	RETURN;
#ifdef VNMRJ
    else if (argc > 3)
    {
	strcpy(hostarg,argv[1]); /* 'ultra30' */
	strcpy(objtype,argv[2]); /* 'vnmr_data' */
	strcpy(filename,argv[3]); /* '/export/home/vnmr1/data/dd.fid' */
    }
#endif 
    else
    {
	strcpy(filename,argv[1]);
#ifdef VNMRJ
	strcpy(objtype,"vnmr_data");
	strcpy(hostarg,VnmrJHostName);
#endif 
    }
    if (filename[0] != '/')
    {
	if (getcwd(jstr,256) == NULL)
	    strcpy(jstr,"");
	strcat(jstr,"/");
	strcat(jstr,filename);
	strncpy(filename,jstr,MAXPATH);
	i = strlen(filename);
	if (filename[i] != '\0')
	    filename[i] = '\0';
    }

       for (j=0; j<strlen(filename); j++)
          if (filename[j] == '*')
          {
             Werrprintf("No wildcards are permitted in the %s command", argv[0]);
#ifdef VNMRJ
	     sprintf(jstr,"noWildcards %s %s %s",hostarg,objtype,filename);
	     writelineToVnmrJ("noRmFile",jstr);
#endif 
             ABORT;
          }
       if ( access(filename,F_OK) == 0)
       {
          if ( access(filename,W_OK|R_OK) == 0)
          {
#ifdef UNIX
             sprintf(cmdstr,"/bin/rm -r %s",check_spaces(filename,chkbuf,MAXPATH+8));
#else 
	     if (strchr(filename,';') != NULL)
	      sprintf(cmdstr,"delete %s",filename);
	     else
	      sprintf(cmdstr,"delete %s;*",filename);
#endif 
             TPRINT1("rmfile: command string \"%s\"\n",cmdstr);
             system_call(cmdstr);
          }
#ifdef VNMRJ
          sprintf(jstr,"%s %s %s",hostarg,objtype,filename);
#endif 
          if ( access(filename,F_OK) == 0)
	  {
             Werrprintf("cannot remove %s",filename);
#ifdef VNMRJ
	     writelineToVnmrJ("noRmFile",jstr);
#endif 
	  }
#ifdef VNMRJ
	  else
	  {
	     writelineToVnmrJ("RmFile",jstr);
	  }
#endif 
       }
       else
       {
	  Werrprintf("file not found: cannot remove %s",filename);
#ifdef VNMRJ
	  sprintf(jstr,"noFind %s %s %s",hostarg,objtype,filename);
	  writelineToVnmrJ("noRmFile",jstr);
#endif 
       }
    RETURN;
}

/*------------------------------------------------------------------------------
|
|	del_file
|
|	This procedure  deletes either a .fid or .par directory or a file
|
+-----------------------------------------------------------------------------*/

int del_file(int argc, char *argv[], int retc, char *retv[])
{   char cmdstr[MAXPATH+20];   
#ifdef VNMRJ
    char jstr[ 3*MAXPATH ], jstrn[ MAXPATH ];
#endif 
    int  i,j;

    for (i=1; i<argc; i++)
       for (j=0; j<strlen(argv[i]); j++)
          if (argv[i][j] == '*')
          {
             Werrprintf("No wildcards are permitted in the delete command");
#ifdef VNMRJ
	     writelineToVnmrJ("noRmFile","nowildcards");
#endif 
             ABORT;
          }
    for (i=1; i<argc; i++)
       if ( access(argv[i],F_OK) == 0)
       {
          if ( access(argv[i],W_OK|R_OK) == 0)
          {
#ifdef UNIX
             sprintf(cmdstr,"/bin/rm %s",check_spaces(argv[i],chkbuf,MAXPATH));
#else 
	     if (strchr(argv[i],';') != NULL)
	      sprintf(cmdstr,"delete %s",argv[i]);
	     else
	      sprintf(cmdstr,"delete %s;*",argv[i]);
#endif 
             TPRINT1("delete: command string \"%s\"\n",cmdstr);
             system_call(cmdstr);
          }
#ifdef VNMRJ
          if (argv[i][0] != '/')
          {
              if (getcwd(jstr,256) == NULL)
                  strcpy(jstr,"");
              strcat(jstr,"/");
              strcat(jstr,argv[i]);
              strncpy(jstrn,jstr,MAXPATH);
              i = strlen(jstrn);
              if (jstrn[i] != '\0')
                  jstrn[i] = '\0';
              sprintf(jstr,"%s vnmr_data %s",VnmrJHostName,jstrn);
          }
          else
              sprintf(jstr,"%s vnmr_data %s",VnmrJHostName,argv[i]);
#endif 
          if ( access(argv[i],F_OK) == 0)
	  {
             Werrprintf("cannot delete %s",argv[i]);
#ifdef VNMRJ
	     writelineToVnmrJ("noRmFile",jstr);
#endif 
	  }
#ifdef VNMRJ
	  else
	     writelineToVnmrJ("RmFile",jstr);
#endif 
       }
#ifdef UNIX
       else
       {  char tmp[MAXPATH];

          sprintf(tmp,"%s.fid",argv[i]);
          if ( access(tmp,F_OK) == 0)
          {
             if ( access(tmp,W_OK|R_OK) == 0)
             {
                sprintf(cmdstr,"/bin/rm -r %s",check_spaces(tmp,chkbuf,MAXPATH));
                TPRINT1("delete: command string \"%s\"\n",cmdstr);
                system_call(cmdstr);
#ifdef VNMRJ
                if (tmp[0] != '/')
                {
                    if (getcwd(jstr,256) == NULL)
                        strcpy(jstr,"");
                    strcat(jstr,"/");
                    strcat(jstr,tmp);
                    strncpy(jstrn,jstr,MAXPATH);
                    i = strlen(jstrn);
                    if (jstrn[i] != '\0')
                        jstrn[i] = '\0';
                    sprintf(jstr,"%s vnmr_data %s",VnmrJHostName,jstrn);
                }
                else
                    sprintf(jstr,"%s vnmr_data %s",VnmrJHostName,tmp);
#endif 
                if ( access(tmp,F_OK) == 0)
		{
                   Werrprintf("cannot delete %s",argv[i]);
#ifdef VNMRJ
		   writelineToVnmrJ("noRmFile",jstr);
#endif 
		}
#ifdef VNMRJ
		else
		   writelineToVnmrJ("RmFile",jstr);
#endif 
             }
          }
          else
          {
             sprintf(tmp,"%s.par",argv[i]);
             if ( access(tmp,F_OK) == 0)
             {
                if ( access(tmp,W_OK|R_OK) == 0)
                {
                   sprintf(cmdstr,"/bin/rm -r %s",check_spaces(tmp,chkbuf,MAXPATH));
                   TPRINT1("delete: command string \"%s\"\n",cmdstr);
                   system_call(cmdstr);
#ifdef VNMRJ
                   if (tmp[0] != '/')
                   {
                      if (getcwd(jstr,256) == NULL)
                          strcpy(jstr,"");
                      strcat(jstr,"/");
                      strcat(jstr,tmp);
                      strncpy(jstrn,jstr,MAXPATH);
                      i = strlen(jstrn);
                      if (jstrn[i] != '\0')
                          jstrn[i] = '\0';
                      sprintf(jstr,"%s vnmr_data %s",VnmrJHostName,jstrn);
                   }
                   else
                      sprintf(jstr,"%s vnmr_data %s",VnmrJHostName,tmp);
#endif 
                   if ( access(tmp,F_OK) == 0)
		   {
                      Werrprintf("cannot delete %s",argv[i]);
#ifdef VNMRJ
		      writelineToVnmrJ("noRmFile",jstr);
#endif 
		   }
#ifdef VNMRJ
		   else
		      writelineToVnmrJ("RmFile",jstr);
#endif 
                }
             }
             else
	     {
                Werrprintf("file %s not found",argv[i]);
#ifdef VNMRJ
                if (argv[i][0] != '/')
                {
                   if (getcwd(jstr,256) == NULL)
                       strcpy(jstr,"");
                   strcat(jstr,"/");
                   strcat(jstr,argv[i]);
                   strncpy(jstrn,jstr,MAXPATH);
                   i = strlen(jstrn);
                   if (jstrn[i] != '\0')
                       jstrn[i] = '\0';
                   sprintf(jstr,"%s vnmr_data %s",VnmrJHostName,jstrn);
                }
                else
                   sprintf(jstr,"%s vnmr_data %s",VnmrJHostName,argv[i]);
		writelineToVnmrJ("noRmFile",jstr);
#endif 
	     }
          }
       }
#else 
       else
       {
          char tmp[MAXPATH];

          sprintf( tmp, "%s_fid.dir", argv[ i ] );
          if (access( tmp, F_OK ) == 0)
          {
             sprintf( cmdstr, "rm_recur %s", tmp );
             system_call( cmdstr );
             if (access( tmp, F_OK ) == 0)
               Werrprintf( "cannot delete %s", argv[ i ] );
          }
          else
          {
             sprintf( tmp, "%s_par.dir", argv[ i ] );
             if (access( tmp, F_OK ) == 0)
             {
                sprintf( cmdstr, "rm_recur %s", tmp );
                system_call( cmdstr );
                if (access( tmp, F_OK ) == 0)
                  Werrprintf( "cannot delete %s", argv[ i ] );
             }
             else
               Werrprintf( "file %s not found", argv[ i ] );
          }
       }
#endif 
    RETURN;
}

int shellcmds(int argc, char *argv[], int retc, char *retv[])
{
    // Wturnoff_buttons();
    switch(getcmd(argv[0]))
    { case CATT:	return (Cat(argc,argv,retc,retv));
      case CD:	 	return (ch_dir(argc,argv,retc,retv));
      case CP:		return (Cp(argc,argv,retc,retv));
      case LF:	 	return (Lf(argc,argv,retc,retv));
      case MKDIR:	return (mk_dir(argc,argv,retc,retv));
      case MV:		return (Mv(argc,argv,retc,retv));
      case PWD:	 	return (Pwd(argc,argv,retc,retv));
      case RM:		return (Rm(argc,argv,retc,retv));
      case RMANY:	return (del_file(argc,argv,retc,retv));
      case RMFILE:	return (del_file2(argc,argv,retc,retv));
      case RMDIR:	return (rm_dir(argc,argv,retc,retv));
      case SHELL: 	return (Shell(argc,argv,retc,retv));
      case SHELLI: 	return (Shell(argc,argv,retc,retv));
      case W:		return (Whoinfo(argc,argv,retc,retv));
      case UNAME:	return (Uname(argc,argv,retc,retv));
      case HNAME:	return (Hname(argc,argv,retc,retv));
      default:		fprintf(stderr,"shellcmd: (%d) unknown command\n",
				getcmd(argv[0]));
			ABORT;
    }
}

void currentDate(char *cstr, int len )
{
  char tmpstr[MAXPATH];
  struct timeval clock;

  gettimeofday(&clock, NULL);
  strftime(tmpstr, MAXPATH, t_format,  localtime((long*)&(clock.tv_sec)));
  strncpy(cstr, tmpstr, len);
}

char *currentDateLocal(char *cstr, int len )
{
  char tmpstr[MAXPATH];
  struct timeval clock;

  gettimeofday(&clock, NULL);
  strftime(tmpstr, MAXPATH, "%Y\%m\%dT%H\%M\%SX%I\%p\%a%b",  localtime((long*)&(clock.tv_sec)));
  strncpy(cstr, tmpstr, len);
  return(cstr);
}

char *currentDateSvf(char *cstr, int len )
{
  char tmpstr[MAXPATH], timestr[MAXPATH];
  struct timeval clock;

  if (P_getstring(GLOBAL, "svfdate", timestr, 1, MAXPATH) == 0)
  {
    if (timestr[0]!='%')
    {
      strcpy(timestr, t_format);
    }
  }
  else
     strcpy(timestr, t_format);
  gettimeofday(&clock, NULL);
  if (strftime(tmpstr, MAXPATH, timestr,  localtime((long*)&(clock.tv_sec))) == 0)
    strftime(tmpstr, MAXPATH, t_format,  localtime((long*)&(clock.tv_sec)));
  strncpy(cstr, tmpstr, len);
  return(cstr);
}

static double
double_of_timeval(struct timeval tv )
{
	return( (double) (tv.tv_sec) + ((double) (tv.tv_usec)) / 1.0e6 );
}

/*---------------------------------------------------------------------------
|
|	unixtime -  make underlying basic UNIX / POSIX time
|                   available to Magical
|
---------------------------------------------------------------------------*/

int unixtime(int argc, char *argv[], int retc, char *retv[])
{
	double		dtime;
	struct timeval	clock;

   if (argc == 1)
   {
	gettimeofday( &clock, NULL );
	dtime = double_of_timeval( clock );
	if (retc < 1)
        {
		  Winfoprintf( "UNIX time: %d\n", (int) dtime );
	}
	else if (retc == 2)
        {
		char	tstr[ 30 ];

		sprintf( &tstr[ 0 ], "%ld", clock.tv_sec );
		retv[ 0 ] = newString( &tstr[ 0 ] );
		sprintf( &tstr[ 0 ], "%d", (int) clock.tv_usec );
		retv[ 1 ] = newString( &tstr[ 0 ] );
	}
	else
        {
		char	tstr[ 30 ];

		sprintf( &tstr[ 0 ], "%.16g", dtime );
		retv[ 0 ] = newString( &tstr[ 0 ] );
	}
   }
   else if (argc == 2)
   {
        char tmpstr[MAXPATH];

	gettimeofday( &clock, NULL );
        strftime(tmpstr, MAXPATH, argv[1],  localtime((long*)&(clock.tv_sec)));
	if (retc)
	   retv[ 0 ] = newString( tmpstr );
        else
	   Winfoprintf( "UNIX time: %s\n", tmpstr );
   }
   else if (argc == 3)
   {
        char tmpstr[MAXPATH];
        if (isReal(argv[2]) )
        {
           clock.tv_sec = atoi(argv[2]);
           strftime(tmpstr, MAXPATH, argv[1],  localtime((long*)&(clock.tv_sec)));
	   if (retc)
	      retv[ 0 ] = newString( tmpstr );
           else
	      Winfoprintf( "UNIX time: %s\n", tmpstr );
        }
        else
        {
           struct tm timeptr;

           strptime(argv[2], argv[1], &timeptr);
           timeptr.tm_isdst = -1;
           if (retc)
              retv[0] = intString( (int) mktime( &timeptr) );
           else
	      Winfoprintf( "UNIX time: %d\n", (int) mktime( &timeptr) );
        }

   }
   RETURN;
}
