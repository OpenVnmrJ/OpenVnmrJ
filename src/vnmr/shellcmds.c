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
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>
#define __USE_XOPEN 1
#include <time.h>
#include <libgen.h>

#include "pvars.h"
#include "wjunk.h"
#include "allocate.h"
#include "variables.h"
extern int assignString(const char *s, varInfo *v, int i);

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
#define TOUCH  17

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
extern void  trLine(char inLine[], char subLine[], char newChar[], char outLine[] );
extern int   Rmdir(char *dirname, int rmParent);
extern int   Rmfiles(char *dirname, char *fileRegex, int testOnly);
extern int   isDirectory(char *filename);
static int   getcmd(char *cmd);
static void  Shellret(FILE *stream, int retc, char *retv[]);
int More(FILE *stream, int screenLength);
extern int mvDir( char *indir, char *outdir );

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
    if ( ! strcmp(cmd,"touch") )
	return TOUCH;
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

    (void) argc;
    (void) argv;
    (void) retc;
    (void) retv;
    Wshow_text();
    cmdstr = "/usr/bin/who";
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
        (void) argc;
        (void) argv;
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

        (void) argc;
        (void) argv;
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
|    Touch
|
|    This routine does equivalent of touch
|
+----------------------------------------------------------------------------*/
int Touch(int argc, char *argv[], int retc, char *retv[]) {
	int res = 0;
     
   if (argc != 2)
   {
      Werrprintf( "%s: file name must be passed as an argument", argv[ 0 ] );
      ABORT;
   }
   res = open(argv[1], O_WRONLY|O_CREAT|O_NOCTTY|O_NONBLOCK, 0666);
   if (res < 0)
   {
      res = 0;
   }
   else
   {
#ifdef MACOS
      futimes(res,NULL);
#else
      futimens(res,NULL);
#endif
      close(res);
      res = 1;
   }
   if (retc > 0) {
      retv[0] = intString(res);
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

    (void) argc;
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

   (void) retc;
   (void) retv;
   strcpy(cmdstr,"/bin/ls -C -F");
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
    int  i,j;
    int res;

    if ( (argc == 3) || ( (argc == 4) && ! strcmp(argv[1],"-p") ) )  // simple copy
    {
       mode_t mode;
       char *fromFile;
       char toFile[MAXPATH];
       struct stat buf;
       ino_t inode;
       nlink_t nlink;

       if (argc == 3)
       {
          fromFile = argv[1];
       }
       else
       {
          fromFile = argv[2];
       }
       if (stat(fromFile, &buf))
       {
          if (retc)
          {
	     retv[ 0] = realString((double) 0);
             RETURN;
          }
          Werrprintf ( "%s: error for %s: %s", argv[0], fromFile, strerror(errno) );
          ABORT;
       }
       if (S_ISDIR(buf.st_mode))
       {
          if (retc)
          {
	     retv[ 0] = realString((double) 0);
             RETURN;
          }
          Werrprintf ( "%s: cannot copy directory %s", argv[0], fromFile );
          ABORT;
       }
       if (argc == 3)
       {
          strcpy(toFile,argv[2]);
          mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
       }
       else
       {
          strcpy(toFile,argv[3]);
          mode = buf.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO);
       }
       inode = buf.st_ino;
       nlink = buf.st_nlink;
       buf.st_ino = 0;
       if ( (stat(toFile, &buf) == 0) && (S_ISDIR(buf.st_mode)) )
       {
          char tmpFile[MAXPATH];
          char *bname;

          strcpy(tmpFile,fromFile);
          bname = basename(tmpFile);
          strcat(toFile,"/");
          strcat(toFile,bname);
          if (stat(toFile, &buf))
             buf.st_ino = 0;
       }
       if ((inode == buf.st_ino) && (nlink == buf.st_nlink) )
       {
          if (retc)
          {
	     retv[ 0] = realString((double) 0);
             RETURN;
          }
          Werrprintf ( "%s: %s and %s are the same file", argv[0], fromFile, toFile );
          RETURN;
       }
       if ( (res = copyFile(fromFile, toFile, mode)) )
       {
          if (retc)
          {
	     retv[ 0] = realString((double) 0);
             RETURN;
          }
          if (res == NO_FIRST_FILE)
             Werrprintf("%s: cannot copy file %s", argv[0],  fromFile);
          else if (res == NO_SECOND_FILE)
             Werrprintf("%s: could not open %s for writing", argv[0],  toFile );
          else
             Werrprintf("%s: error writing to file: %s", argv[0], strerror(errno) );
          ABORT;
       }
       if (retc)
       {
	  retv[ 0] = realString((double) 1);
       }
       RETURN;
    }
    if ( (argc == 4) && ! strcmp(argv[3],"symlink") )
    {
       res = 1;
       // symlink does not care if argv[1] exists
       if (symlink(argv[1],argv[2]))
       {
          res = 0;
       }
       if (retc)
       {
	  retv[ 0] = realString((double) res);
       }
       else if ( ! res)
       {
          Werrprintf("cannot symlink file %s", argv[1]);
          ABORT;
       }
       RETURN;
    }
    if ( (argc == 4) && ! strcmp(argv[3],"relsymlink") )
    {
       char tpath[MAXPATH];
       char argv2[PATH_MAX];
       char path1[PATH_MAX];
       char path2[PATH_MAX];
       char *path1Ptr;
       char *path2Ptr;
       char *ptr;
       int len;
       int i;
       int ret;
          
       strcpy(tpath,"");
       path1Ptr = realpath(argv[1], path1);
       strcpy(argv2,argv[2]);
       ptr = dirname(argv2);
       path2Ptr = realpath(ptr, path2);
       if (( path1Ptr == NULL ) || (path2Ptr == NULL))
       {
          if (retc)
          {
	     retv[ 0] = realString((double) 0);
             RETURN;
          }
          else
          {
             if (path1Ptr == NULL )
                Werrprintf("cannot relsymlink from %s", argv[1]);
             if (path2Ptr == NULL )
                Werrprintf("cannot relsymlink to %s", argv[2]);
             ABORT;
          }
       }
       ret = 0;
       i = 0;
       while ( *path1Ptr && *path2Ptr )
       {
          if (*path1Ptr != *path2Ptr)
             break;
          if (*path1Ptr == '/')
             ret = i + 1;
          path1Ptr++;
          path2Ptr++;
          i++;
       }
       if ((!*path1Ptr && !*path2Ptr) ||
           (!*path1Ptr && *path2Ptr == '/') ||
           (!*path2Ptr && *path1Ptr == '/'))
       {
          ret = i;
          path1Ptr = &path1[ret];
          path2Ptr = &path2[ret];
          if  (!*path2Ptr && *path1Ptr == '/')
             path1Ptr = &path1[ret+1];
          if  (!*path1Ptr && *path2Ptr == '/')
             path2Ptr = &path2[ret+1];
       }
       else
       {
          path1Ptr = &path1[ret];
          path2Ptr = &path2[ret];
       }
       len = (int) strlen(path2Ptr);
       for (i=0; i<len; i++)
       {
          if (path2Ptr[i] == '/')
             strcat(tpath,"../");
       }
       if (*path2Ptr)
          strcat(tpath,"../");
       strcat(tpath,path1Ptr);
       res = 1;
       if (symlink(tpath,argv[2]))
       {
          res = 0;
       }
       if (retc)
       {
	  retv[ 0] = realString((double) res);
       }
       else if ( ! res)
       {
          Werrprintf("cannot relsymlink file %s", argv[1]);
          ABORT;
       }
       RETURN;
    }
    if ( (argc == 4) && ! strcmp(argv[3],"link") )
    {
       res = 0;
       if (!access(argv[1],F_OK))
       {  
          res = 1;
          if (link(argv[1],argv[2]))
          {
            res = 0;
          }
       }
       if (retc)
       {
	  retv[ 0] = realString((double) res);
       }
       else if ( ! res)
       {
          Werrprintf("cannot link file %s", argv[1]);
          ABORT;
       }
       RETURN;
    }
    strcpy(cmdstr,"cp ");
    j = 2;
    if (argc < 3)
    {
       Werrprintf("%s command requires at least 2 arguments", argv[0]);
       ABORT;
    }
    for (i=1; i<argc; i++)
    {
       int k = 0;
       cmdstr[j++] = ' ';
       if (j + strlen(argv[i]) > MAXSHSTRING - 18)
       {
          Werrprintf("%s arguments exceed %d characters",
                     argv[0], MAXSHSTRING - 18);
          ABORT;
       }
       while (k < strlen(argv[i]))
       {
          if (argv[i][k] == ' ')
             cmdstr[j++] = '\\';
          cmdstr[j++] = argv[i][k];
          k++;
       }
    }
    cmdstr[j] = '\0';
    
    TPRINT1("cp: command string \"%s\"\n",cmdstr);
    strncat(cmdstr," 2> /dev/null", MAXSHSTRING - strlen(cmdstr) - 14);
    system_call(cmdstr);
    res = access(argv[ argc - 1 ],R_OK);
    if (retc)
    {
       retv[ 0] = realString( (res) ? (double) 0 : (double) 1);
    }
    else if ( res )
    {
       Werrprintf( "%s: could not write to file '%s'", argv[ 0 ], argv[ argc - 1 ] );
    }
    RETURN;
}

#ifdef XXX
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

    strcpy(cmdstr,"/bin/cp -r ");
    for (i=1; i<argc; i++)
    {	strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
     	strncat(cmdstr,argv[i],MAXSHSTRING - strlen(cmdstr) -1);
    }
    TPRINT1("CpTreE: command string \"%s\"\n",cmdstr);
    system_call(cmdstr);
    RETURN;
}
#endif

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

    (void) retc;
    (void) retv;
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
    sync();
    strcpy(cmdstr,"/bin/cat ");
    for (i=1; i<argc; i++)
    {
	strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);  /* why??? */
     	strncat(cmdstr,argv[i],MAXSHSTRING - strlen(cmdstr) -1);
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
    int mvErr = 0;

    (void) retc;
    (void) retv;
    if ( !((argc == 3) || ( (argc == 4) && ! strcmp(argv[1],"-f") )) )
    {
      Werrprintf( "%s:  use exactly 2 arguments", argv[ 0 ] );
      ABORT;
    }
    i = 1;
    if (argc == 4)
       i = 2;
    len = strlen(argv[i]);
    if ( isDirectory(argv[i+1]) &&
         (argv[i][len-1] == '*') &&
         (argv[i][len-2] == '/') )
    {

         strcpy(cmdstr, argv[i]);
         cmdstr[len-2] = '\0';
         if ( isDirectory(cmdstr) )
            mvErr = mvDir( cmdstr, argv[i+1] );
         else
         {
            mvErr = -1;
         }
    }
    else
    {
         mvErr = rename(argv[i], argv[i+1]);
    }
    if ( mvErr )
    {
       strcpy(cmdstr,"/bin/mv");
       for (i=1; i<argc; i++)
       {
	strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
	len = MAXSHSTRING - strlen(cmdstr) -1;
	strncat(cmdstr,check_spaces(argv[i],chkbuf,len),len);
       }
       TPRINT1("mv: command string \"%s\"\n",cmdstr);
       system_call(cmdstr);
    }
    RETURN;
}

/* convert string to octal */
mode_t str_to_octal(char *ptr )
{
  int i, len;
  mode_t fac=1, ret=0;
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
static int mk_dir_args(mode_t *mode, int *psub, int *ict, int argc, char *argv[])
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
      int len;
      mode_t tmpmode;
      len = strlen(argv[i]);
      if (len > 3)
      {
	tmpmode = str_to_octal(argv[i] + 3);
        if ((tmpmode < 0500) || (tmpmode > 0777))
/*        if ((tmpmode < 320) || (tmpmode > 511)) */
	  Werrprintf("mkdir: invalid mode '%s', reset to 777",argv[i] + 3);
        else
	  *mode = tmpmode;
      }
    }
  }
  RETURN;
}

int do_mkdir(const char *dir, int psub, mode_t mode)
{
   int	ival=0;
   if (psub == 1)
   {
      char path[MAXPATH*2];   
      char lastpath[MAXPATH*2];   
      char tmppath[MAXPATH*2];   
      char dirpath[MAXPATH*2];   
      char *ptr;
      strcpy(path,dir);
      while ( access(path, F_OK) && (ival == 0) )
      {
         strcpy(tmppath,path);
         ptr = tmppath;
         while ( access(ptr, F_OK) )
         {
            strcpy(lastpath,ptr);
            strcpy(dirpath,ptr);
            ptr = dirname(dirpath);
         }
         ival = mkdir( lastpath, mode );
      }
      ival = access( dir, F_OK );
   }
   else
      ival = mkdir( dir, mode );
   return(ival);
}
/*------------------------------------------------------------------------------
|
|	mkdir
|
|	This procedure  does a standard mkdir command.
|
+-----------------------------------------------------------------------------*/

int mk_dir(int argc, char *argv[], int retc, char *retv[])
{
    int  i, ict=1, psub=0;
    mode_t  mode=0777;

    mk_dir_args(&mode, &psub, &ict, argc, argv);
    for (i=ict; i<argc; i++)
    {
	int	ival=0;

	if (verify_fname2( argv[ i ] ) != 0)
	{
	    Werrprintf(
		"%s:  cannot use '%s' as a directory name", argv[ 0 ], argv[ i ]
	    );
#ifdef VNMRJ
	    writelineToVnmrJ("invalidfile",argv[i]);
#endif 
	    ABORT;
	}
        ival = do_mkdir(argv[i], psub, mode);
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
    int  i,len;

    (void) retv;
    if ( (argc == 2) || ( (argc == 3) && ! strcmp(argv[1],"-f") ) )
    {
       int argnum = (argc == 2) ? 1 : 2;
       
       if ( ! access(argv[argnum],F_OK))
       {
          unlink(argv[argnum]);
          if ( access(argv[argnum],F_OK))
             RETURN;
       }
    }
    else if ( (argc == 3) &&
              ( !strcmp(argv[1],"-rf") || !strcmp(argv[1],"-r") || !strcmp(argv[1],"-R") ) )
    {
       int noWildCard = 1;
       for (i=0; i<(int)strlen(argv[2]); i++)
          if (argv[2][i] == '*')
          {
             noWildCard = 0;
          }
       if (noWildCard && ! access(argv[2],F_OK|W_OK|R_OK) )
       {
          if ( !strcmp(argv[1],"-R") )
          {
             if ( ! Rmdir(argv[2],0) )
                RETURN;
          }
          else
          {
             if ( ! Rmdir(argv[2],1) )
                RETURN;
          }
       }
       if ( !strcmp(argv[1],"-R") )
       {
          // Don't let it continue with system command since that will
          // also remove the parent directory
          if (!retc)
          {
             if ( noWildCard == 0 )
                Werrprintf("%s: cannot use -R with wildcards (*) in the path name %s",argv[0],argv[2]);
             else if ( access(argv[2],F_OK|W_OK|R_OK) )
                Werrprintf("%s: cannot use -R with non-accessible directory %s",argv[0],argv[2]);
             else
                Werrprintf("%s: cannot remove contents of %s",argv[0],argv[2]);
             ABORT;
          }
          else
          {
             RETURN;
          }
       }
    }
    else if ( (argc == 4) && (  !strcasecmp(argv[1],"regex") || !strcasecmp(argv[1],"regextest")  ) )
    {
       // in ths case, argv[2] is directory and argv[3] is filename with wildcard
       if ( ! access(argv[2],X_OK))
       {
          int testOnly = (!strcasecmp(argv[1],"regextest"));
          if ( ! Rmfiles(argv[2], argv[3], testOnly) )
             RETURN;
       }
       RETURN;
    }
    strcpy(cmdstr,"/bin/rm");
    for (i=1; i<argc; i++)
    {
        strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
	len = MAXSHSTRING - strlen(cmdstr) -1;
	strncat(cmdstr,check_spaces(argv[i],chkbuf,len),len);
    }
    TPRINT1("rm: command string \"%s\"\n",cmdstr);
    system_call(cmdstr);
    for (i=1; i<argc; i++)
      if (argv[i][0] != '-')
      {
          if ( access(argv[i],F_OK) == 0)
	  {
             if (!retc)
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

#ifdef XXX
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
}
#endif

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

    (void) retv;
    strcpy(cmdstr,"/bin/rmdir");
    for (i=1; i<argc; i++)
    {
        strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
	len = MAXSHSTRING - strlen(cmdstr) -1;
	strncat(cmdstr,check_spaces(argv[i],chkbuf,len),len);
    }
    TPRINT1("rmdir: command string \"%s\"\n",cmdstr);
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

    (void) retc;
    (void) retv;
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

       for (j=0; j<(int) strlen(filename); j++)
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
{
    char jstr[ 3*MAXPATH ], jstrn[ MAXPATH ];
    int  i,j;
    int isDir;

    (void) retv;
    for (i=1; i<argc; i++)
       for (j=0; j<(int) strlen(argv[i]); j++)
          if (argv[i][j] == '*')
          {
             Werrprintf("No wildcards are permitted in the delete command");
	     writelineToVnmrJ("noRmFile","nowildcards");
             ABORT;
          }
    for (i=1; i<argc; i++)
    {
       if ( !strcmp(argv[i],"") )
          continue;
       isDir = (strstr(argv[i],".fid") || strstr(argv[i],".par") );
       if ( !isDir &&  access(argv[i],F_OK) == 0)
       {
          if ( access(argv[i],W_OK|R_OK) == 0)
          {
             unlink(argv[i]);
          }
          if (!Bnmr)
          {
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
             if ( access(argv[i],F_OK) == 0)
	     {
                Werrprintf("cannot delete %s",argv[i]);
	        writelineToVnmrJ("noRmFile",jstr);
	     }
	     else
	        writelineToVnmrJ("RmFile",jstr);
          }
          else
          {
             if ( !retc &&  access(argv[i],F_OK) == 0)
	     {
                Werrprintf("cannot delete %s",argv[i]);
	     }
          }
       }
       else if ( !isDir &&  (i+1 < argc) && strcmp(argv[i+1],"") == 0)
       {
          // don't check .par and .fid if file name followed by null string
          i++;
          continue;
       }
       else
       {  char tmp[MAXPATH];

          if (isDir)
             strcpy(tmp,argv[i]);
          else
             sprintf(tmp,"%s.fid",argv[i]);
          if ( access(tmp,F_OK) == 0)
          {
             if ( access(tmp,W_OK|R_OK) == 0)
             {
                int res;
                res = Rmdir(tmp,1);
                if (!Bnmr)
                {
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
                   if ( res )
		   {
                      if (!retc)
                         Werrprintf("cannot delete %s",argv[i]);
		      writelineToVnmrJ("noRmFile",jstr);
		   }
		   else
		      writelineToVnmrJ("RmFile",jstr);
                }
                else
                {
                   if ( !retc &&  res )
	           {
                      Werrprintf("cannot delete %s",argv[i]);
	           }
                }
             }
             else if ( !retc )
	     {
                Werrprintf("no permission to delete %s",argv[i]);
             }
          }
          else
          {
             sprintf(tmp,"%s.par",argv[i]);
             if ( access(tmp,F_OK) == 0)
             {
                if ( access(tmp,W_OK|R_OK) == 0)
                {
                   int res;
                   res = Rmdir(tmp,1);
                   if (!Bnmr)
                   {
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
                      if ( res )
		      {
                         if (!retc)
                            Werrprintf("cannot delete %s",argv[i]);
		         writelineToVnmrJ("noRmFile",jstr);
		      }
		      else
		         writelineToVnmrJ("RmFile",jstr);
                   }
                   else
                   {
                      if ( !retc &&  res )
	              {
                         Werrprintf("cannot delete %s",argv[i]);
	              }
                   }
                }
                else if ( !retc )
	        {
                   Werrprintf("no permission to delete %s",argv[i]);
	        }
             }
             else
	     {
                if (!retc)
                   Werrprintf("file %s not found",argv[i]);
                if (!Bnmr)
                {
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
                }
	     }
          }
       }
    }
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
      case TOUCH:	return (Touch(argc,argv,retc,retv));
      default:		Werrprintf("unknown shell command %s",argv[0]);
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
   else if (argc >= 3)
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

           timeptr.tm_sec = 0;
           timeptr.tm_min = 0;
           timeptr.tm_hour = 0;
           timeptr.tm_mday = 0;
           timeptr.tm_mon = 0;
           timeptr.tm_year = 0;
           timeptr.tm_wday = 0;
           timeptr.tm_yday = 0;
           timeptr.tm_isdst = 0;
           strptime(argv[2], argv[1], &timeptr);
           timeptr.tm_isdst = -1;
           if (argc == 3)
           {
              if (retc)
                 retv[0] = intString( (int) mktime( &timeptr) );
              else
      	         Winfoprintf( "UNIX time: %d\n", (int) mktime( &timeptr) );
           }
           else
           {
              clock.tv_sec = mktime( &timeptr );
              strftime(tmpstr, MAXPATH, argv[3],
                       localtime(&(clock.tv_sec)));
              if (retc)
                 retv[ 0 ] = newString( tmpstr );
              else
                 Winfoprintf( "UNIX time: %s\n", tmpstr );
           }

        }

   }
   RETURN;
}

static char *getLine(char *inLine, size_t len, char *fromStrPtr)
{
   char *ptr;

   if ( *fromStrPtr == '\0')
      return(NULL);
   ptr = fromStrPtr;
   while ( ( *ptr != '\0' ) && (len > 1) )
   {
      *inLine++ = *ptr;
      if ( *ptr == '\n' )
      {
         ptr++;
         break;
      }
      ptr++;
      len--;
   }
   *inLine = '\0';
   return(ptr);
}

static int countWords(const char* str)
{
   if (str == NULL)
      return(0);

   int inSpaces = 1;
   int numWords = 0;

   while (*str != '\0')
   {
      if (isspace(*str))
      {
         inSpaces = 1;
      }
      else if (inSpaces)
      {
         numWords++;
         inSpaces = 0;
      }
      ++str;
   }
   return(numWords);
}

#define APPENDLINE 2048
int appendCmd(int argc, char *argv[], int retc, char *retv[])
{
   FILE *inFile = NULL;
   FILE *outFile = NULL;
   int lineCount = 0;
   int wordCount = -1;
   int headCount = 0;
   int tailCount=0;
   int skipCount = 0;
   int tailArg = 0;
   int headArg = 0;
   int tailStage = -1;
   int headOK = 1;
   int lineOK = 0;
   int fromStr;
   char *fromStrPtr = NULL;
   varInfo *v1 = NULL;
   char inLine[APPENDLINE];

   if (argc < 3)
   {
      Werrprintf("%s: at least two arguments must be provided",argv[0]);
      ABORT;
   }
   fromStr = (  ! strcmp(argv[0],"appendstr") || ! strcmp(argv[0],"copystr") );
   if ( !fromStr &&  ! strcmp(argv[1],argv[argc-1]) )
   {
      Werrprintf("%s: cannot append to the same file as the source file",argv[0]);
      ABORT;
   }
   if (argc > 3)
   {
      int index;
      for (index=2; index<argc-1; index += 2)
      {
         if ( !strcmp(argv[index],"head") )
         {
            headCount++;
            headArg = atoi(argv[index+1]);
         }
         else if ( !strcmp(argv[index],"tail") )
         {
            tailCount++;
            tailArg = atoi(argv[index+1]);
         }
// sed, sed g, and tr require two arguments
         else if ( !strcmp(argv[index],"sed") ||
                   !strcmp(argv[index],"sed g") ||
                   !strcmp(argv[index],"tr") )
            index++;
      }
      if (headCount > 1)
      {
         Werrprintf("%s: can only use the 'head' option once",argv[0]);
         ABORT;
      }
      if (tailCount > 1)
      {
         Werrprintf("%s: can only use the 'tail' option once",argv[0]);
         ABORT;
      }
      headCount = 0;
      tailCount = 0;
   }
   if ( !strcmp(argv[argc-1],"|wc w") || ! strcmp(argv[argc-1],"| wc w") ||
        !strcmp(argv[argc-1],"|wc -w") || ! strcmp(argv[argc-1],"| wc -w"))
      wordCount = 0;
   if ( strcmp(argv[argc-1],"|wc") && strcmp(argv[argc-1],"| wc") &&
        (wordCount == -1) )
   {
      if (strstr(argv[argc-1],"|"))  // probably a bad |wc argument
      {
         Werrprintf("%s: final argument '%s' is wrong",argv[0],argv[argc-1]);
         ABORT;
      }
      if ( argv[argc-1][0] == '$' )
      {
         symbol **root;
         if ((root=selectVarTree(argv[argc-1])) == NULL)
         {
            Werrprintf("%s: local variable \"%s\" doesn't exist",argv[0],argv[argc-1]);
            ABORT;
         }
         if ((v1 = rfindVar(argv[argc-1],root)) == NULL)
         {   Werrprintf("%s: variable \"%s\" doesn't exist",argv[0],argv[argc-1]);
             ABORT;
         }
         if (v1->T.basicType == T_REAL)
         {   Werrprintf("%s: variable \"%s\" must be of string type",argv[0],argv[argc-1]);
             ABORT;
         }
         assignString("",v1,0);
      }
      else
      {
         if ( !strncmp(argv[0], "app", 3) ) 
            outFile = fopen(argv[argc-1],"a");
         else
            outFile = fopen(argv[argc-1],"w");
         if (outFile == NULL)
         {
            if (retc)
            {
               retv[ 0 ] = intString(0);
               RETURN;
            }
            Werrprintf("%s: cannot append to file %s",argv[0],argv[argc-1]);
            RETURN;
         }
      }
   }
   if ( ! fromStr)
   {
      if ( isDirectory(argv[1]) )
      {
         Werrprintf("%s: Input file %s is a directory",argv[0],argv[1]);
         if (outFile)
            fclose(outFile);
         ABORT;
      }
      inFile = fopen(argv[1],"r");
      if (inFile == NULL)
      {
         if (outFile)
            fclose(outFile);
         if (retc)
         {
            retv[ 0 ] = intString(0);
            RETURN;
         }
         Werrprintf("%s: Input file %s not found",argv[0],argv[1]);
         RETURN;
      }
   }
   else
   {
      fromStrPtr = argv[1];
   }

   while ( headOK )
   {
      size_t len;
      int ret;

      inLine[0] = '\0';
      if (fromStr)
      {
         ret = ( (fromStrPtr = getLine(inLine, sizeof(inLine), fromStrPtr)) == NULL);
         // When appending a null string, make sure carriage return is appended
         lineOK = 1;
      }
      else
      {
         ret = (fgets(inLine, sizeof(inLine), inFile) == NULL);
         lineOK = 0;
      }
      if (ret)
      {
         if (tailStage)
            break;
         if (tailStage == 0)
         {
            tailStage = 1;
            if (fromStr)
            {
               fromStrPtr = argv[1];
               if  ( (fromStrPtr = getLine(inLine, sizeof(inLine), fromStrPtr)) == NULL)
                  break;
            }
            else
            {
               rewind(inFile);
               if (fgets(inLine, sizeof(inLine), inFile) == NULL)
                  break;
            }
            lineCount = 0;
            headCount = 0;
            skipCount = 0;
            wordCount = 0;
         }
      }
      len = strlen(inLine);          /* get buf length */
      if (len && inLine[len-1] == '\n')      /* check last char is '\n' */
         inLine[--len] = '\0'; 
      if (len && inLine[len-1] == '\r')      /* check last char is '\r' */
         inLine[--len] = '\0'; 
      lineOK = 1;
      if (argc > 3)
      {
         int index;
         for (index=2; index<argc-1; index += 2)
         {
            if ( !strcmp(argv[index],"sed") ||
                 !strcmp(argv[index],"sed g") )
            {
               regex_t regex;
               regmatch_t matches;
               int reti;
               char newline[APPENDLINE];
               char tmpline[APPENDLINE];
               int doGlobal;
               int sindex;
               int found = 0;

               if (index + 2 >= argc-1)
               {
                  Werrprintf("%s: sed options require two arguments",argv[0]);
                  continue;
               }
               doGlobal = (strcmp(argv[index],"sed g") == 0) ? 1 : 0;
               index++;

               
/* Compile regular expression */
               reti = regcomp(&regex, argv[index], REG_EXTENDED);
               if (reti)
               {
                  Werrprintf("%s: Could not compile regex",argv[0]);
                  continue;
               }
/* Execute regular expression */
               strcpy(tmpline,inLine);
               newline[0] = '\0';
               sindex = 0;
               while ( (reti = regexec(&regex, &tmpline[sindex], 1, &matches, 0)) == 0)
               {
                   found = 1;
                   if (doGlobal)
                   {
                      strncat(newline, &tmpline[sindex], matches.rm_so);
                      strcat(newline, argv[index+1]);
                      sindex += matches.rm_eo;
                      if (tmpline[sindex] == '\0')
                         break;
                   }
                   else
                   {
                      tmpline[matches.rm_so] = '\0';
                      sprintf(inLine,"%s%s%s",tmpline, argv[index+1], &tmpline[matches.rm_eo]);
                      break;
                   }
               }
               if ( found && doGlobal )
               {
                  strcpy(inLine,newline);
                  strcat(inLine, &tmpline[sindex]);
               }

/* Free memory allocated to the pattern buffer by regcomp() */
               regfree(&regex);
            }
            else if ( !strcmp(argv[index],"awk") )
            {
               char tmpLine[APPENDLINE];
               size_t lIndex;
               int wordNumber;
               char *awkPtr;

               strcpy(tmpLine,inLine);
               awkPtr = argv[index+1];
               len = strlen(argv[index+1]);
               inLine[0] = '\0';
               lIndex=0;
               while ( *awkPtr != '\0' )
               {
                  if (*awkPtr == '$')
                  {
                     if ((*(awkPtr+1) >= '0') && (*(awkPtr+1) <= '9'))
                     {
                        char *ptr;
                        int iWord;

                        awkPtr++;
                        wordNumber=atoi(awkPtr);
                        while ((*awkPtr >= '0') && (*awkPtr <= '9'))
                           awkPtr++;
                        ptr = tmpLine;
                        // skip initial white space unless $0 requested
                        if (wordNumber)
                           while ( (*ptr != '\0') && ((*ptr == ' ') || (*ptr == '\t')) )
                              ptr++;
                        iWord = 1;
                        while (iWord < wordNumber)
                        {
                           while ( (*ptr != '\0') && (*ptr != ' ') && (*ptr != '\t') )
                              ptr++;
                           iWord++;
                           while ( (*ptr != '\0') && ((*ptr == ' ') || (*ptr == '\t')) )
                              ptr++;
                        }
                        if (wordNumber)
                        {
                           while ( (*ptr != '\0') && (*ptr != ' ') && (*ptr != '\t') )
                              inLine[lIndex++] = *ptr++;
                        }
                        else
                        {
                           while (*ptr != '\0')
                              inLine[lIndex++] = *ptr++;
                        }
                     }
                     else
                     {
                        inLine[lIndex++] = *awkPtr++;
                     }
                  }
                  else
                  {
                     inLine[lIndex++] = *awkPtr++;
                  }
               }
               inLine[lIndex]= '\0';
            }
            else if ( !strcmp(argv[index],"tr") )
            {
               char tmpLine[APPENDLINE];
               
               trLine(inLine, argv[index+1], argv[index+2], tmpLine );
               strcpy(inLine,tmpLine);
               index++;
            }
            else if ( !strcmp(argv[index],"head") )
            {
               if (lineOK)
                  headCount++;
               if (headArg < headCount)
               {
                  lineOK = 0;
                  if (tailStage)
                     headOK = 0;
               }
            }
            else if ( !strcmp(argv[index],"tail") )
            {
               if (lineOK && (tailStage <= 0))
                  tailCount++;
               if (argv[index+1][0] == '+')
               {
                  if (tailCount < tailArg)
                     lineOK = 0;
               }
               else
               {
                  if (tailStage == -1)
                     tailStage = 0;
                  if (tailStage == 0)
                  {
                     headOK = 1;
                  }
                  else if (tailStage == 1)
                  {
                     if (lineOK)
                             skipCount++;
                     if (tailCount - skipCount >= tailArg)
                        lineOK = 0;
                  }
               }
            }
            else if (argv[index+1][0] == '^')
            {
               int res;
               size_t len = strlen(argv[index+1])-1;
               res=strncmp(inLine,&argv[index+1][1], len);
               if ( !strcmp(argv[index],"grep") )
               {
                  if (res)
                  {
                    lineOK = 0;
                    break;
                  }
               }
               else if ( !strcmp(argv[index],"grep -v") )
               {
                  if (!res)
                  {
                    lineOK = 0;
                    break;
                  }
               }
               else if ( !strcmp(argv[index],"grep -w") )
               {
                  if ( (res) ||  
                        ( (*(inLine+len) != ' ') &&
                          (*(inLine+len) != '\t') &&
                          (*(inLine+len) != '\0')) )
                  {
                    lineOK = 0;
                    break;
                  }
               }
               else if ( !strcmp(argv[index],"grep -vw") || !strcmp(argv[index],"grep -wv") )
               {
                  if ( (!res) &&
                         ((*(inLine+len) == ' ') ||
                          (*(inLine+len) == '\t') ||
                          (*(inLine+len) == '\0')))
                  {
                    lineOK = 0;
                    break;
                  }
               }
               else if ( !strcmp(argv[index],"grep -iw") || !strcmp(argv[index],"grep -wi") )
               {
                  res=strncasecmp(inLine,&argv[index+1][1], len);
                  if ( (res) ||  
                        ( (*(inLine+len) != ' ') &&
                          (*(inLine+len) != '\t') &&
                          (*(inLine+len) != '\0')) )
                  {
                    lineOK = 0;
                    break;
                  }
               }
               else if ( !strcmp(argv[index],"grep -i") )
               {
                  res=strncasecmp(inLine,&argv[index+1][1], len);
                  if (res)
                  {
                    lineOK = 0;
                    break;
                  }
               }
               else
               {
                  Werrprintf("%s: unknown filter '%s'",argv[0],argv[index]);
                  if (inFile)
                     fclose(inFile);
                  if (outFile)
                     fclose(outFile);
                  ABORT;
               }
            }
            else
            {
               if ( !strcmp(argv[index],"grep") )
               {
                  if ( ! strstr(inLine,argv[index+1]) )
                  {
                     lineOK = 0;
                     break;
                  }
               }
               else if ( !strcmp(argv[index],"grep -v") )
               {
                  if ( strstr(inLine,argv[index+1]) )
                  {
                     lineOK = 0;
                     break;
                  }
               }
               else if ( !strcmp(argv[index],"grep -w") )
               {
                  char *ptr;
                  if ( (ptr = strstr(inLine,argv[index+1])) == NULL )
                  {
                     lineOK = 0;
                     break;
                  }
                  else
                  {
                     char *ptr2;
                     len = strlen(argv[index+1]);
                     ptr2 = ptr;
 
                     while (((ptr == ptr2) && (strlen(ptr2) >= len) &&
                            (*(ptr+len) != ' ') &&
                            (*(ptr+len) != '\t') &&
                            (*(ptr+len) != '\0') ) ||
                            ( (ptr != inLine) &&
                              ( *(ptr-1) != ' ') &&
                              ( *(ptr-1) != '\t')
                            )
                           )
                     {
                        ptr2++;
                        if ( (ptr  = strstr(ptr2,argv[index+1])) == NULL )
                        {
                            lineOK = 0;
                            break;
                        }
                        ptr2 = ptr;
                     }
                  }
               }
               else if ( !strcmp(argv[index],"grep -vw") || !strcmp(argv[index],"grep -wv") )
               {
                  char *ptr;
                  if ( (ptr = strstr(inLine,argv[index+1])) != NULL )
                  {
                     char *ptr2;
                     len = strlen(argv[index+1]);
                     ptr2 = ptr;
 
                     while (((ptr == ptr2) && (strlen(ptr2) >= len) &&
                            (*(ptr+len) != ' ') &&
                            (*(ptr+len) != '\t') &&
                            (*(ptr+len) != '\0') ) ||
                            ( (ptr != inLine) &&
                              ( *(ptr-1) != ' ') &&
                              ( *(ptr-1) != '\t')
                            )
                           )
                     {
                         ptr2++;
                         if ( (ptr  = strstr(ptr2,argv[index+1])) == NULL )
                         {
                             break;
                         }
                         ptr2=ptr;
                     }
                     if (ptr != NULL)
                        lineOK = 0;
                  }
               }
               else if ( !strcmp(argv[index],"grep -i") )
               {
                  char needle[APPENDLINE];
                  char haystack[APPENDLINE];
                  char *tptr, *fptr;

                  fptr=inLine;
                  tptr=haystack;
                  while ( *fptr )
                     *tptr++ = (isupper( *fptr )) ? *fptr++ + 'a' - 'A' : *fptr++;
                  *tptr = '\0';
                  fptr=argv[index+1];
                  tptr=needle;
                  while ( *fptr )
                     *tptr++ = (isupper( *fptr )) ? *fptr++ + 'a' - 'A' : *fptr++;
                  *tptr = '\0';
                  if ( ! strstr(haystack,needle) )
                  {
                     lineOK = 0;
                     break;
                  }
               }
               else if ( !strcmp(argv[index],"grep -iw") || !strcmp(argv[index],"grep -wi") )
               {
                  char needle[APPENDLINE];
                  char haystack[APPENDLINE];
                  char *tptr, *fptr;
                  char *ptr;

                  fptr=inLine;
                  tptr=haystack;
                  while ( *fptr )
                     *tptr++ = (isupper( *fptr )) ? *fptr++ + 'a' - 'A' : *fptr++;
                  *tptr = '\0';
                  fptr=argv[index+1];
                  tptr=needle;
                  while ( *fptr )
                     *tptr++ = (isupper( *fptr )) ? *fptr++ + 'a' - 'A' : *fptr++;
                  *tptr = '\0';

                  if ( (ptr = strstr(haystack,needle)) == NULL )
                  {
                     lineOK = 0;
                     break;
                  }
                  else
                  {
                     char *ptr2;
                     len = strlen(needle);
                     ptr2 = ptr;
 
                     while (((ptr == ptr2) && (strlen(ptr2) >= len) &&
                            (*(ptr+len) != ' ') &&
                            (*(ptr+len) != '\t') &&
                            (*(ptr+len) != '\0') ) ||
                            ( (ptr != haystack) &&
                              ( *(ptr-1) != ' ') &&
                              ( *(ptr-1) != '\t')
                            )
                           )
                     {
                        ptr2++;
                        if ( (ptr  = strstr(ptr2,needle)) == NULL )
                        {
                            lineOK = 0;
                            break;
                        }
                        ptr2 = ptr;
                     }
                  }
               }
               else
               {
                  Werrprintf("%s: unknown filter '%s'",argv[0],argv[index]);
                  if (inFile)
                     fclose(inFile);
                  if (outFile)
                     fclose(outFile);
                  ABORT;
               }
            }
         }
      }
      if (lineOK)
      {
         lineCount++;
         if (wordCount >= 0)
            wordCount += countWords(inLine);
         if (outFile && tailStage)
            fprintf(outFile,"%s\n",inLine);
         else if (tailStage && (lineCount < retc) )
         {
            retv[lineCount] = newString(inLine);
         }
         if (tailStage && v1)
         {
            assignString(inLine,v1,lineCount);
         }
         lineOK = 0;
      }
   }

   // Handle case where only a null string is given
   if (lineOK && !lineCount)
   {
      lineCount++;
      if (outFile && tailStage && fromStr)
            fprintf(outFile,"%s\n",inLine);
      else if (tailStage && (lineCount < retc) )
      {
         retv[lineCount] = newString(inLine);
      }
   }
   if (outFile != NULL)
   {
      /* when appending to a file, not just counting lines, just indicate success */
      lineCount = 1;
      fclose(outFile);
   }
   if (inFile)
      fclose(inFile);
   if (retc)
      retv[ 0 ] = (wordCount >= 0) ? intString(wordCount) : intString(lineCount);
   RETURN;
}

static char *kflags(char *in, int k1Flag, int k2Flag)
{
   static char *wPtr;
   wPtr = in;
   if (k1Flag)
   {
      int index;

      for (index=1; index<k1Flag; index++)
      {
         /* skip white space */
         while (( (*wPtr == ' ') || (*wPtr == '\t')) && (*wPtr != '\0'))
            wPtr++;
         /* and skip word */
         while ((*wPtr != ' ') && (*wPtr != '\t') && (*wPtr != '\0'))
            wPtr++;
      }
      if (k2Flag != 0)
      {
         char *ePtr = wPtr;
            
         for (index=k1Flag; index<=k2Flag; index++)
         {
            /* skip white space */
            while (( (*ePtr == ' ') || (*ePtr == '\t')) && (*ePtr != '\0'))
               ePtr++;
            /* and skip word */
            while ((*ePtr != ' ') && (*ePtr != '\t') && (*ePtr != '\0'))
               ePtr++;
         }
         *ePtr = '\0';
      }
      /* skip white space */
      while (( (*wPtr == ' ') || (*wPtr == '\t')) && (*wPtr != '\0'))
         wPtr++;
   }
   return(wPtr);
}

#define SORTSIZE 100

int sortCmd(int argc, char *argv[], int retc, char *retv[])
{
   FILE *inFile;
   FILE *outFile = NULL;
   int uFlag = 0;
   int rFlag = 0;
   int nFlag = 0;
   int k1Flag = 0;
   int k2Flag = 0;
   char inLine1[2048];
   char inLine2[2048];
   char *ptr1 = NULL;
   char *ptr2 = NULL;
   long *posPtr = NULL;
   double *valPtr = NULL;
   long   pos[SORTSIZE];
   double val[SORTSIZE];
   long ipos;
   int numLines = 0;
   int i,j;
   char *ret __attribute__((unused));
   

   if (argc < 3)
   {
      Werrprintf("%s: at least two arguments must be provided",argv[0]);
      ABORT;
   }
   if (argc == 4)
   {
      char *kPtr;
      if (strstr(argv[2],"u"))
         uFlag=1;
      if (strstr(argv[2],"r"))
         rFlag=1;
      if (strstr(argv[2],"n"))
         nFlag=1;
      if ( (kPtr = strstr(argv[2],"k")) )
      {
         k1Flag=atoi(kPtr+1);
         if ( (kPtr = strstr(argv[2],",")) )
            k2Flag=atoi(kPtr+1);
      }
   }
   if ( ! strcmp(argv[1],argv[argc-1]) )
   {
      Werrprintf("%s: cannot append to the same file as the source file",argv[0]);
      ABORT;
   }
   outFile = fopen(argv[argc-1],"w");
   if (outFile == NULL)
   {
      if (retc)
      {
         retv[ 0 ] = intString(0);
         RETURN;
      }
      Werrprintf("%s: cannot write to file %s",argv[0],argv[argc-1]);
      ABORT;
   }
   inFile = fopen(argv[1],"r");
   if (inFile == NULL)
   {
      fclose(outFile);
      if (retc)
      {
         retv[ 0 ] = intString(0);
         RETURN;
      }
      Werrprintf("%s: Input file %s not found",argv[0],argv[1]);
      ABORT;
   }

   ipos = ftell(inFile);
   while (fgets(inLine1, sizeof(inLine1), inFile) != NULL)
   {
      if (numLines < SORTSIZE)
      {
         pos[numLines] = ipos;
         if (nFlag)
         {
            ptr1=kflags(inLine1,k1Flag,k2Flag);
            val[numLines] = strtod(ptr1,NULL);
         }
      }
      numLines++;
      ipos = ftell(inFile);
   }
   if (numLines > SORTSIZE)
   {
      posPtr = (long *) allocateWithId(numLines*sizeof(long),"sortCmd");;
      if (nFlag)
      {
         valPtr = (double *) allocateWithId(numLines*sizeof(double),"sortCmd");;
      }
      rewind(inFile);
      ipos = ftell(inFile);
      numLines = 0;
      while (fgets(inLine1, sizeof(inLine1), inFile) != NULL)
      {
         *(posPtr+numLines) = ipos;
         if (nFlag)
         {
            ptr1=kflags(inLine1,k1Flag,k2Flag);
            *(valPtr+numLines) = strtod(ptr1,NULL);
         }
         numLines++;
         ipos = ftell(inFile);
      }
   }
   else
   {
      posPtr = &pos[0];
      valPtr = &val[0];
   }
   rewind(inFile);
   for (i=1; i<numLines; i++)
   {
      double dval = 0.0;
      size_t len;

      ipos = *(posPtr+i);
      if ( !nFlag )
      {
         fseek(inFile, *(posPtr+i), SEEK_SET);
         ret = fgets(inLine1, sizeof(inLine1), inFile);
         len = strlen(inLine1);          /* get buf length */
         if (len && inLine1[len-1] == '\n')      /* check last char is '\n' */
            inLine1[--len] = '\0'; 
         ptr1=kflags(inLine1,k1Flag,k2Flag);
      }
      else
      {
         dval = *(valPtr+i);
      }
      j = i - 1;
      while (j>=0)
      {
         if (*(posPtr+j) >= 0)
         {
            if ( !nFlag )
            {
               fseek(inFile, *(posPtr+j), SEEK_SET);
               ret = fgets(inLine2, sizeof(inLine2), inFile);
               len = strlen(inLine2);          /* get buf length */
               if (len && inLine2[len-1] == '\n')      /* check last char is '\n' */
                  inLine2[--len] = '\0'; 
               ptr2=kflags(inLine2,k1Flag,k2Flag);
               if (uFlag && (strcmp(ptr2,ptr1) == 0) )
               {
                  ipos = *(posPtr+j+1) = -1;
                  break;
               }
               else if (strcmp(ptr2,ptr1) > 0)
               {
                  *(posPtr+j+1) = *(posPtr+j);
                  j--;
               }
               else
               {
                  break;
               }
            }
            else
            {
               if (uFlag && (*(valPtr+j) == dval))
               {
                  ipos = *(posPtr+j+1) = -1;
                  break;
               }
               else if (*(valPtr+j) > dval)
               {
                  *(posPtr+j+1) = *(posPtr+j);
                  *(valPtr+j+1) = *(valPtr+j);
                  j--;
               }
               else
               {
                  break;
               }
            }
         }
         else
         {
            *(posPtr+j+1) = *(posPtr+j);
            if (nFlag)
               *(valPtr+j+1) = *(valPtr+j);
            j--;
         }
      }
      *(posPtr+j+1) = ipos;
      if (nFlag)
      {
         *(valPtr+j+1) = dval;
      }
   }
   if (rFlag)
   {
      for (j=numLines-1; j>=0; j--)
      {
         if (*(posPtr+j) == -1)
            continue;
         fseek(inFile, *(posPtr+j), SEEK_SET);
         ret = fgets(inLine1, sizeof(inLine1), inFile);
         fprintf(outFile,"%s",inLine1);
      }
   }
   else
   {
      for (j=0; j<numLines; j++)
      {
         if (*(posPtr+j) == -1)
            continue;
         fseek(inFile, *(posPtr+j), SEEK_SET);
         ret = fgets(inLine1, sizeof(inLine1), inFile);
         fprintf(outFile,"%s",inLine1);
      }
   }
   fclose(inFile);
   fclose(outFile);
   releaseAllWithId("sortCmd");
   if (retc)
   {
      retv[ 0 ] = intString(1);
   }
   RETURN;
}
