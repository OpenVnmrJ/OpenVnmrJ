/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-------------------------------------------------------------------------
|
|       vnmr  main routine
|
|	This module contains the main() procedure of the
|	vnmr program. This main procedure figures out whether
|	we are running on a sun or a graphon terminal and
|	creates the appropriate windows.
|	This module also contains code to handle mouse
|	execution on the sun, creating the special cursor
|	on the sun, and sets up environmental variables.
|
|	NOTE WELL THAT ON THE SUN CONSOLE THIS IS THE BASE ROUTINE OF
|	THE CHILD AND IS NOT CALLED BY MASTER
|
|	Important note:  Prior to this version, the child owned the
|	numbered buttons.  This was not acceptable, as the master had
|	no way of knowing synchronously that the child was preoccupied
|	with a Button event and thus Acquisition messages should be
|	queued until the child is no longer busy.
|
|	Thus the code to create the panel and the numbered buttons
|	has been removed from this module and placed in master.c
|
/+-----------------------------------------------------------------------*/
#include "vnmrsys.h"
#include "locksys.h"
#include <stdio.h>

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include "acquisition.h"

char         datadir[MAXPATH];	 	/* path to data in automation mode */
char        *initCommand = NULL;        /* inital command */
char        *graphics;			/* Type of graphical display */
char         psgaddr[MAXSTR];

char         vnMode[22];	/* can be set to background, acq, etc */
char        *vnmruser = NULL;		


extern int   ignoreEOL;
extern int   textFrameFd;
extern int   mainFrameFd;
extern int   buttonWinFd;
extern int   textTypeIsTcl;
extern int   resizeBorderWidth;
extern int   Eflag;
extern int   Rflag;
extern char  textFrameFds[];
extern char  mainFrameFds[];
extern char  buttonWinFds[];
extern char  Xserver[];
extern char  Xdisplay[];
extern char  fontName[];
extern char  HostName[];

extern void  showMacros();
int          Bnmr;
int          Aflag;
int          Dflag;
int          Gflag;
int          Lflag;
int	     masterWinHeight = 1;
int	     masterWinWidth = 1;
int          textWinHeight = 1;
int	     mode_of_vnmr;
int	     moreFd = 0;
int          Pflag;
int          Tflag;
int          Vflag;
int          Wretain;
int          inputfd;
int          outputfd;
int 	     interuption = 0;
int 	     working     = 0;

static int  expnum;
static int  graphics_window;
static char default_graphics[ 2 ] = "\0";

#define NEWLINE         10

/*-------------------------------------------------------------------------
|
|     Main routine of magic
|
/+-----------------------------------------------------------------------*/

static
catch_sigint()
{
    struct sigaction	intserv;
    sigset_t		qmask;
    void		gotInterupt();
 
    /* --- set up signal handler --- */

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGINT );
    intserv.sa_handler = gotInterupt;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;

    sigaction( SIGINT, &intserv, NULL );
}

void
gotInterupt()
{  if (working)
      if (interuption == 0)
      {  Werrprintf("command canceled");
         interuption = 1;
      }
      else
	 Werrprintf("already canceled");
   else
      Werrprintf("nothing to cancel");

   catch_sigint();
}

/*  This routine process the command line used to start Vnmr.  */

vnmr_argvec( argc, argv )
int argc;
char *argv[];
{
    char *p;
    char *vnmode;
    char *moreFds;				/*  Used???  */
    int i;

/*  Parsing loop changed so that only one option can be
    specified per item in the argument vector.			*/

    sprintf(textFrameFds, "0");
    textFrameFd = 0;
    mainFrameFd = 0;
    buttonWinFd = 0;
    vnmode = NULL;
    psgaddr[0] = '\0';
    for (i=1; i<argc; ++i)
    {
	p = argv[ i ];
	if (*p == '-')
	    switch (*(++p))
	    {
		  case 'B':     resizeBorderWidth = atoi(p+1);
                                break;
		  case 'D':	Dflag -= 1;
				break;
		  case 'd':	
                                if (!strcmp(argv[i], "-display"))
                                {
                                     sprintf(Xdisplay, "-display");
                                     i++;
                                     strcpy(Xserver, argv[i]);
                                }
				else
                                {
                                     int len;

                                     strcpy( &datadir[ 0 ], p+1 );
                                     len = strlen(datadir);
                                     if ((len > 4) && ( strcmp( &datadir[len - 4], ".fid") == 0) )
                                        datadir[len-4] = '\0';
                                }
				break;
		  case 'E':	Eflag -= 1;
				break;
		  case 'e':	Eflag += 1;
				break;
		  /* option f Does not need to be used */
		  case 'f':     moreFds = p+1;
				moreFd = atoi(moreFds);
				break;
		  /* The textsw file descriptor */
                  case 'F':
                                sprintf(textFrameFds, "%s", p+1);
				textFrameFd = atoi(textFrameFds);
                                textTypeIsTcl = (textFrameFd <= 0);
                                break;
		  case 'G':	Gflag -= 1;
				break;
		  case 'g':	Gflag += 1;
				break;
		  case 'h':     SET_VNMR_ADDR(p+1);
                                strcpy(psgaddr, p+1);
				break;
		  case 'H':     masterWinHeight = atoi(p+1);
				break;
		  case 'i':     SET_ACQ_ID(p+1);
				break;
		  case 'K':     textWinHeight = atoi(p+1);
                                break;
		  case 'L':	Lflag -= 1;
				break;
		  case 'l':	Lflag += 1;
				break;
                  case 'M':
                                sprintf(mainFrameFds, "%s", p-1);
				mainFrameFd = atoi(p+1);
                                break;
		  case 'm':     vnmode= p+1;
				break;
		  case 'n':     expnum = atoi(p+1);
				break;
                  case 'O':
                                sprintf(buttonWinFds, "%s", p-1);
				buttonWinFd = atoi(p+1);
                                break;
		  case 'P':	Pflag -= 1;
				break;
		  case 'p':	Pflag += 1;
				break;
		  case 'R':	Rflag -= 1;
				break;
		  case 'r':	Rflag += 1;
				break;
		  case 'S':     masterWinWidth = atoi(p+1);
				break;
		  case 'T':	Tflag -= 1;
				break;
		  case 't':	Tflag += 1;
				break;
		  case 'u':	strcpy(&userdir[ 0 ],p+1);
				break;
		  case 'V':	Vflag -= 1;
				break;
		  case 'v':	Vflag += 1;
				break;
		  case 'W':	Wretain -= 1;
				break;
		  case 'w':	Wretain += 1;
				break;
                  case 'Z':
                                if (strlen(argv[i]) > 2)
                                   strcpy(&fontName[ 0 ],p+1);
                                break;
		  default:	fprintf(stderr,"magic: unknown option '%c' available options DdEeGgLlPpRrTtuVvWw\n",*p);
				break;
	}
	else
	{   initCommand = p;
	    if (Tflag)
		fprintf(stderr,"Command line %s passed to vnmr\n",initCommand);
	}
    }			/*  End of for each argument loop */

    setUpEnv(vnmode);	/* setup environmental variables */
}

vnmr(argc,argv)			int argc; char *argv[];
{
    setbuf(stdout,NULL);
    setbuf(stderr,NULL);
#ifdef VMS
    vf_init();				/* Initialize binary disk I/O system */
#endif
    /*signal(SIGINT,gotInterupt);*/
    catch_sigint();
    Dflag     = 0;
    Eflag     = 0;
    Gflag     = 0;
    ignoreEOL = 0;
    Aflag     = 0;
    Lflag     = 0;
    Pflag     = 0;
    Rflag     = 1;
    Tflag     = 0;
    Vflag     = 0;
    Wretain   = 1;   /* default retained */
    expnum    = 1;
    graphics  = &default_graphics[ 0 ];
    datadir[0]= '\0';				/* No path to data */
    fontName[0] = '\0';
    Xserver[0] = '\0';
    Xdisplay[0] = '\0';

     merge_command_table(NULL);
    INIT_VNMR_ADDR();
    vnmr_argvec( argc, argv );
    VNMR_ADDR_OK();

    if (Tflag)
    {
        if (Bnmr)
        {
            fprintf(stderr,"Background VNMR sys =%s user=%s moreFd = %d expnum=%d vnMode =%s\n",systemdir,userdir,moreFd,expnum,vnMode);
        }
        else
        {
            fprintf(stderr,"Forground VNMR sys =%s user=%s moreFd = %d expnum=%d vnMode=%s\n",systemdir,userdir,moreFd,expnum,vnMode);
        }
    }

    INIT_ACQ_COMM(systemdir);

    if (Vflag)
	showMacros();

    if (Bnmr)  /* background vnmr */
    {
        INIT_VNMR_COMM(HostName);	/* To receive stuff from acq process */
	setupdirs( "background" );
	bootup(vnMode,expnum);
	if (initCommand)
	{
            char tmpStr[2048];
            if (Tflag)
		fprintf(stderr,"executing command line %s\n",initCommand);
            strcpy(tmpStr,initCommand);
            strcat(tmpStr,"\n");
	    loadAndExec(tmpStr);
	}
	nmr_exit(vnMode);
	exit(0);
    }

    graphics = (char *)getenv( "graphics" );

#ifdef SUN
    if (Wissun())  /* sun vnmr */
    {	setupdirs( "child" );
	graphics_window = 1;
        setUpWin(Wretain); /* setup frame */
	if (Tflag)
	  fprintf(stderr,"before notify set input func\n");
    /* set to yell at fd 0 */
        inputfd = 0;
        set_input_func();
	if (Tflag)
	  fprintf(stderr,"after notify set input func\n");
	set_bootup_gfcn_ptrs();
	Wclear_graphics();
	bootup(vnMode,expnum);
        setTtyInputFocus();
        smagicLoop();
	if (Tflag)
	  fprintf(stderr,"after window_main_loop \n");
    }
    else  /* terminal vnmr */
#endif
    {	setupdirs( "terminal" );
	graphics_window = 0;
	Wsetupterm(); /* setup windows */
	bootup(vnMode,expnum);
	terminal_main_loop();
    }
}

#ifdef SUN
/*--------------------------------------------------------------------
|    getInputBuf/2
|
|	This routine grabs a string from the input pipe and  
|	passes it back to the caller
|
/+--------------------------------------------------------------------*/
char *
getInputBuf(buf,len)	char *buf; int len;
{   char c;
    int  i = 0;
    int  res;

    if (!len)
	return ('\0');  /* why would anyone want 0 length */
   
    res = read(0,&c,1); /* read first character */
    if (Tflag)
    {	if (res == -1)
	    perror("getInputBuf:Read");
	fprintf(stderr,"getInputBuf:res = %d read %c\n",res,c);
    }
    while (i < len)
    {	if (c == NEWLINE)
	{   buf[i] = '\0';
	    return(buf);   
	}
	buf[i++] = c;
     	res = read(0,&c,1);
	if (Tflag)
	{   if (res == -1)
		perror("getInputBuf:Read");
	    fprintf(stderr,"getInputBuf:res = %d read %c\n",res,c);
	}
    }
    buf[len-1] = '\0';
    return (buf);
}

#endif


/*
 * This prevents mouse movements over the graphics canvas from blocking
 * all input to the Sun windows.  This is a problem which seems to be
 * new in SunOS 4.x.  This procedure is called when spectra or FIDs are
 * accessed.  (calc_spec(),  gettrace(),  getfid(),  get_one_fid(), and
 * secondft().)
 */
long_event()
{
#ifdef SUN
   if (graphics_window)
      release_canvas_event_lock();
#endif
}


/*--------------------------------------------------------------------
|    macro_main_loop
|
|	This opens macro file and executes from it 
|
/+--------------------------------------------------------------------*/
macro_main_loop(file)	char *file;
{   char  buf[1024];
    FILE *stream;

    if (stream = fopen(file,"r"))
    {	while (fgets(buf,sizeof(buf),stream))
	    loadAndExec(buf);
    }
    else
    {	fprintf(stderr,"Errors opening file %s\n",file);
	exit(1);
    }
}

/*--------------------------------------------------------------------
|    setUpEnv
|
|	This routine checks and reads in the environmental variables
|	for vnmr.  It also checks if userdir/maclib exists and 
|	creates one if it doesn't.
|
/+--------------------------------------------------------------------*/

setUpEnv(modeptr)
char *modeptr;
{
    int  mode_err;

    Bnmr = 0;
    mode_err = 0;

    /* setup vnMode */

    if (modeptr == NULL) /* if not predefined by option */
	modeptr = "foreground";
    else if (strlen(modeptr) > 20)
    {
    	fprintf(stderr,"Error: value of mode option too long\n");
	exit( 1 );
    }

/*  Mode can be:
	foreground
	background
	acquisition
	automation

    Only the minimum number of characters are required.
    If mode is not recognized, mode_of_vnmr will remain 0.	*/

    mode_of_vnmr = 0;
    if (*modeptr == 'b')
      mode_of_vnmr = BACKGROUND;
    else if (*modeptr == 'a')
    {
	modeptr++;
	if (*modeptr == 'c')
	  mode_of_vnmr = ACQUISITION;
	else if (*modeptr == 'u')
	  mode_of_vnmr = AUTOMATION;
	modeptr--;
    }
    else if (*modeptr == 'f')
      mode_of_vnmr = FOREGROUND;

    if (mode_of_vnmr == 0)
    {
	fprintf( stderr, "invalid mode option %s\n", modeptr );
	exit( 1 );
    }
    strcpy(vnMode,modeptr);
    Bnmr = (mode_of_vnmr != FOREGROUND);

    if (Tflag)
	fprintf(stderr,"setUpEnv systemdir =%s userdir=%s expnum = %d\n",systemdir,userdir,expnum);
}

#define  EOT 	04
#define ACQMBUFSIZE 1024
struct acqmsgentry {
	char	*nextentry;
	char	curentry[ ACQMBUFSIZE ];
};

extern int         fgBusy;
extern int         debug;
extern int         forgroundFdW;

struct acqmsgentry  *baseofqueue = NULL;
static char        acqProcBuf[ACQMBUFSIZE];
static int         acqProcBufPtr = 0;

#ifdef  DEBUG
extern int Gflag;
#define GPRINT0(str) \
        if (Gflag) fprintf(stderr,str)
#define GPRINT1(str, arg1) \
        if (Gflag) fprintf(stderr,str,arg1)
#else   DEBUG
#define GPRINT0(str)
#define GPRINT1(str, arg1)
#endif  DEBUG
/*---------------------------------------------------------------
|
|   exec_message()/0
|	send acquisition message to Vnmr
|
+--------------------------------------------------------------*/
static void
exec_message()
{
   acqProcBuf[acqProcBufPtr] = '\0';
   GPRINT1("M: Got '%s'\n",acqProcBuf);
   if (strcmp(acqProcBuf," "))
   {
      if (Wissun())
      {
         GPRINT1("fgBusy = %d\n", fgBusy);
         if (fgBusy)
            insertAcqMsgEntry( &acqProcBuf[ 0 ] );
         else
         {
            write(forgroundFdW,acqProcBuf,strlen(acqProcBuf));
            sendChildNewLineNoClear();
         }
      }
      else
      {
         strcat(acqProcBuf,"\n");
         execString( &acqProcBuf[ 0 ] );
      }
   }
   acqProcBufPtr = 0;
}

/*-------------------------------------------------------------
|  AcqSocketIsRead(me,fd)
|
|  Modified  for added robustness see above for comments  .
|				  12/13/90  Greg Brissey
|  Modified  for elimination of EOT.
|				  2/08/91  Greg Brissey
+-------------------------------------------------------------*/
AcqSocketIsRead(reader)
int (*reader)();
{  
   char tbuff[ACQMBUFSIZE];
   int  index,readchars,ovrrun;
   int  read_again, do_read;
   char c;
 
   do_read = 1;
   read_again = 0;
   while (do_read &&
          ((readchars = (*reader)(tbuff, sizeof(tbuff), &read_again)) > 0) )
   {
      index = ovrrun = 0;
      do_read = read_again;
      while (index < readchars)
      {
         c = tbuff[index++];
         switch (c)
         {
         case '\n': 
                exec_message();
		break;
         case EOT:   /* skip any EOT char, for compatiblity with previous routines */
		break;
	 default:   
		/* make sure we don't overrun buffer */
                if (acqProcBufPtr < ACQMBUFSIZE)
                {
                   acqProcBuf[acqProcBufPtr++] = c;
                }
                else
                {
                   acqProcBuf[acqProcBufPtr++] = '\0';
		   ovrrun = 1;
                   fprintf(stderr, "Acq message exceeded buffer size\n");
                   fprintf(stderr, "message fragment: '%s'\n",acqProcBuf);
                   break;
                }
         }
         if (ovrrun)
            break;	/* jump out of while if overran buffer */
      }   
   }
   if (readchars <= 0)
   {
      if (acqProcBufPtr)
      {
         exec_message();
      }
   }
}

/*  No concern about concurrent execution so long as both the insert
    and the remove routines are called from the Notify dispatcher (if
    indirectly) AND the Notify dispatcher does not interrupt itself	*/

insertAcqMsgEntry( acqmptr )
char *acqmptr;
{
	register int			 esize, finished;
	register char			*tmpptr;
	register struct acqmsgentry	*curptr, *newptr;
	extern char			*allocateWithId();

/*  Be careful to leave curptr pointing to something useful, not NULL
    (unless, of course, the queue is empty)				*/

	curptr = baseofqueue;
	finished = (curptr == NULL);
	while ( !finished ) {
		tmpptr = curptr->nextentry;
		if (tmpptr != NULL)
		  curptr = (struct acqmsgentry *) tmpptr;
		else
		  finished = 131071;
	}

/*  Round entry size up to next multiple of 4.  Do not forget
    to include space for the next entry, or for the nul byte.   */

	esize = (strlen( acqmptr ) + 1 + sizeof( char * ) + 3) & ~3;
	if ( (tmpptr = allocateWithId( esize, "acqmsgqueue" )) == NULL ) {
		GPRINT0("BUG! out of memory for acqmsg queue\n" );
		return;
	}

	newptr = (struct acqmsgentry *) tmpptr;
	newptr->nextentry = NULL;
	strcpy( &newptr->curentry[ 0 ], acqmptr );

/*  An empty queue is a special case.  */

	if (baseofqueue == NULL) baseofqueue = newptr;
	else curptr->nextentry = (char *) newptr;
}

/*  It is the duty of the routine that calls "removeAcqMsgEntry"
    to deallocate the space used by the queue entry.		*/

struct acqmsgentry *removeAcqMsgEntry()
{
	register struct acqmsgentry	*curptr;

	if (baseofqueue == NULL) return( NULL );	/* empty queue */

	curptr = baseofqueue;
	baseofqueue = (struct acqmsgentry *) curptr->nextentry;
	return( curptr );
}

jExpressionError() /* dummy function used by vnmrj */
{
}

jAutoRedisplayList( jstr ) /* dummy function used by vnmrj */
char *jstr;
{
}

/*
 *  Function used by vnmrj.  Defined here so that macros do not need to decide
 *  whether this command is available
 */
int vnmrjcmd(argc, argv, retc, retv)
int  argc, retc;
char *argv[], *retv[];
{
   RETURN;
}
