/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/file.h>
#ifdef SOLARIS
#include <sys/filio.h>
#endif
#ifdef LINUX
#include <sys/ioctl.h>
#endif
#include <signal.h>
#include <sys/time.h>

#include <sys/socket.h>

#ifdef __INTERIX
#include <arpa/inet.h>
#else
#ifdef SOLARIS
#include <sys/types.h>
#endif
#include <netinet/in.h>
#endif

#include "locksys.h"
#include "acquisition.h"
#include "allocate.h"
#include "sockets.h"
#include "sockinfo.h"
#include "smagic.h"
#include "master_win.h"
#include "wjunk.h"

int     debug = 0;

#ifdef  DEBUG
#define DPRINT0(str) \
	if (debug) fprintf(stderr,str)
#define DPRINT1(str, arg1) \
	if (debug) fprintf(stderr,str,arg1)
#define DPRINT2(str, arg1, arg2) \
	if (debug) fprintf(stderr,str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
	if (debug) fprintf(stderr,str,arg1,arg2,arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4) \
	if (debug) fprintf(stderr,str,arg1,arg2,arg3,arg4)

#else 
#define DPRINT0(str)
#define DPRINT1(str, arg2)
#define DPRINT2(str, arg1, arg2)
#define DPRINT3(str, arg1, arg2, arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4)
#endif 

/*  Definition of the acquisition message entry uses trickery in
    that the actual entry may be smaller than 256+4 bytes.  The
    actual size is determined by the length of the message received
    from Acqproc.  The structure definition is only referenced by
    pointers and never explicitly allocated.			*/

struct acqmsgentry
{
   char           *nextentry;
   char            curentry[256];
};


#define N_MASTER_STATUS_WINDOWS  9

struct msgwindow
{
   char	 *name;
   int    startCol;
   int    width;
   int    lineNum;
};


struct msgwindow  masterStatusField[ N_MASTER_STATUS_WINDOWS ] =
{
	{ "print",           1,  5, 0, },
	{ "plot",           -1,  4, 0, },
	{ "sequence",       -1, 16, 0, },
	{ "experiment",     -1,  8, 0, },
	{ "spectrum index", -1, 11, 0, },
	{ "acquisition",    -1,  8, 0, },
	{ "status",         -1,  8, 0, },
	{ "index",          -1,  5, 0, },
	{ "plane",          42,  8, 1, },
};


extern char     HostName[];
extern pid_t    HostPid;
extern char    *graphics;
extern char     userdir[];
extern char     systemdir[];

int             forgroundFdW;
int             masterWindowHeight2 = 0;
int             masterWindowWidth2 = 0;
int             textWindowHeight = 0;
int             textWindowWidth = 0;
int             resizeBorderWidth = 10;

int  VnmrJPortId;
int  VnmrJViewId = 1;
char VnmrJHostName[128];
char Jvbgname[64] = "/tmp/vbgtmp";
int  jInput = 0;

extern Socket sVnmrJaddr; /* vnmraddr */
extern Socket sVnmrJcom; /* common text */
extern Socket sVnmrJAl;  /* alpha text window */
extern Socket sVnmrGph;  /* graphics */
extern int Gphsd; /* graphics socket descriptor */
extern int Bserver;
extern int noUI;

/*  Counter to indicate number of commands that the master has
    sent to the child.  Only when this counter is 0 can messages
    from Acqproc be sent to the child.				*/

int             fgBusy;		/* Used by socket1.c    */
extern struct acqmsgentry *baseofqueue;	/* Defined in socket1.c */
extern struct acqmsgentry *removeAcqMsgEntry();	/* Defined in socket1.c */

extern void setupdirs(char *cptr );
extern int unlockAllExp(char *userptr, int target_pid, int mode );
extern int  clear_acq();
extern int locklc_(char *cmd, int pid);
extern int createVSocket(int type, Socket *tSocket );

static void setOnCancel(char *txt);

/*  Note that "smagic.c" has an identical static variable
    "default_graphics".  The two separate strings exist
    because Vnmr can run in foreground or background mode.
    In the latter we want the global pointer "graphics" to
    not point to the value of the environment variable
    "graphics" so that the window output routines will dump
    their output to stdout without any extra stuff.

    By assumption, master runs in foreground mode so the
    default value of graphics should point to the string
    "sun".  Thus the two distinct variables and the two
    separate definitions of the global pointer "graphics".	*/

#ifdef MOTIF
static char     bgReaderBuffer[256];		/* bgReaderBuffer isn't used */
#endif
#define MAXREAD 1024
static char     fgReaderBuffer[MAXREAD+4];	/* prevent overflow from long strings */
static int      data_station = 1;       /* parameter to distinguish data station and spectrometer */
                                        /* only used by AbortAcq */
int      backgroundFdR;
int      backgroundPid;
int      bgEscapeMode = 0;
int      fgEscapeMode = 0;
int      forgroundFdR;
int      forgroundPid;
int      pipeSuspended = 0;
int      bgReaderBp = 0;
int      fgReaderBp = 0;
int      MainPort = -1;
FILE     *textTypeIsTcl = NULL;
char     TclFile[256];
char     TclDefaultCmd[256];
int      TclPid = 0;
int      TclPort = 0;

char     Xserver[64], Xdisplay[10]; /* prepared for X-Window */
char     fontName[120], textFrameFds[12];
char     mainFrameFds[12];
char     buttonWinFds[12];
char     vjXInfo[120];

static int maxbuttonchars = 80;
static int buttonspacechars = 2;
static int def_font = 0;
static int inputPort = 0;  /* the input socket port */
static int bBack = 0;  /* in background mode */

static FILE   *xfd = NULL;

static void
setupMasterStatusFields()
{
	int	currentStartingColumn, currentLine;
	int	iter;

	currentStartingColumn = 1;
	currentLine = -1;

	for (iter = 0; iter < N_MASTER_STATUS_WINDOWS; iter++)
	{
		if (currentLine != masterStatusField[ 0 ].lineNum)
		{
			currentLine = masterStatusField[ iter ].lineNum;
			currentStartingColumn = masterStatusField[ iter ].startCol;
			if (currentStartingColumn < 1)
			  currentStartingColumn = 1;	/* default to column 1 */
		}

		if (masterStatusField[ iter ].startCol < 0)
		  masterStatusField[ iter ].startCol = currentStartingColumn;

		currentStartingColumn += (masterStatusField[ iter ].width + 2);
	}

#ifdef DEBUG_STATUS_FIELDS
	for (iter = 0; iter < N_MASTER_STATUS_WINDOWS; iter++)
	{
		fprintf( stderr, "field %d: %s, %d, %d, %d\n",
			 iter,
			 masterStatusField[ iter ].name,
			 masterStatusField[ iter ].startCol,
			 masterStatusField[ iter ].width,
			 masterStatusField[ iter ].lineNum
		);
	}
#endif 
}

/*
 *  Use this routine to write newline characters to the child process.
 */
void sendChildNewLine()		/* Not a static for it   */
{				/* is used by socket1.c  */
   write(forgroundFdW, "\n", 1);
   fgBusy++;
   DPRINT0("child now busy\n");
}

/*
 *  This routine is the same as above except line 3 is not cleared
 *  See processChar, which examines NEWLINE and RETURN
 */
void sendChildNewLineNoClear()
{
   write(forgroundFdW, "\r", 1);
   fgBusy++;
   DPRINT0("child now busy\n");
}

/*------------------------------------------------------------------------------
|
|	We have got an escape sequence from a child, deal with it!  A child
|	sends a message by prefixing it with exactly THREE escapes (\033) and
|	terminates it with a newline (\n).  Usually the first 7 characters
|	are the pid (in %7u format) of the sender.  For example...
|
|		printf("\033\033\033%7uTActive\n",HostPid);
|
|	A good way to make this fairly clean is to make a message sender
|	function...
|
|		sendStatus(m)		char *m;
|		{  printf("\033\033\033%7uT%s\n",HostPid,m);
|		}
|
|	Which is called passing in a char * pointing to the null terminated
|	message.
|
|		   .
|		   .
|		sendMessage("Active");
|		   .
|		   .
|
|	For complex messages with numbers and such, sprintf is useful...
|
|		{  char buffer[256];
|
|		   sendMessage(sprintf(buffer,"Count is %d",count));
|		}
|
+-----------------------------------------------------------------------------*/

static char       *
printable(char *msg)
{
   char           *p,
                  *q;
   static char     buffer1[122];
   static char     buffer2[122];
   static char     buffer3[122];
   static int      select = 1;

   switch (select)
   {
      case 1:
	 p = buffer1;
	 q = buffer1;
	 select = 2;
	 break;
      case 2:
	 p = buffer2;
	 q = buffer2;
	 select = 3;
	 break;
      case 3:
	 p = buffer3;
	 q = buffer3;
	 select = 1;
	 break;
      default:
	 p = buffer1;
	 q = buffer1;
	 select = 2;
	 break;
   }
   while (*msg && (p-q) < (int) sizeof(buffer1) - 3)
   {
      if ((' ' <= *msg) && (*msg <= '~'))
	 *p++ = *msg;
      else
	 switch (*msg)
	 {
	    case '\b':
	       *p++ = '\\';
	       *p++ = 'b';
	       break;
	    case '\f':
	       *p++ = '\\';
	       *p++ = 'f';
	       break;
	    case '\n':
	       *p++ = '\\';
	       *p++ = 'n';
	       break;
	    case '\r':
	       *p++ = '\\';
	       *p++ = 'r';
	       break;
	    case '\t':
	       *p++ = '\\';
	       *p++ = 't';
	       break;
	    default:
	       *p++ = '^';
	       *p++ = *msg + '@';
	 }
      msg += 1;
   }
   *p = '\0';
   return (q);
}

void showDispPrint(char *msg)
{
    dispMessage(1, printable(msg), 0, 0, 8);
}

void showDispPlot(char *msg)
{
   dispMessage(2, printable(msg), 8, 0, 8);
}

void showDispBg(char *msg)
{
   (void) msg;
   fprintf(stderr, "master: reference made to non-existant background field!\n");
}

void showDispSeq(char *msg)
{
   dispMessage(4, printable(msg), 23, 0, 14);
}

void showDispExp(char *msg)
{
   dispMessage(5, printable(msg), 37, 0, 8);
}

void showDispSpecIndex(char *msg)
{
   dispMessage(6, printable(msg), 44, 0, 12);
}

void showDispAcq(char *msg)
{
   dispMessage(7, printable(msg), 56, 0, 10);
}

void showDispStatus(char *msg)
{
   dispMessage(8, printable(msg), 66, 0, 10);
}

void showDispIndex(char *msg)
{
   dispMessage(9, printable(msg), 76, 0, 8);
}

void showDispPlane(char *msg)
{
   dispMessage(10, printable(msg), 44, 1, 8);
}

void showError(char *msg)
{
   DPRINT1( "M: showError: %s\n", msg );
   dispMessage(0, printable(msg), 0, 2, 80);
}

void
showStatus(char *tex)
{
   int             c_index, field;
   int             startingColumn, lineNumber, width;

   sscanf(tex, "%2d", &field);
   DPRINT2( "M, showStatus: %d, %s\n", field, tex+2 );

   if (field < 1 || field > N_MASTER_STATUS_WINDOWS)
   {
      fprintf(stderr,
          "no such status message field %d message %s\n", field, tex + 2
      );
      return;
   }

   c_index = field - 1;
   startingColumn = masterStatusField[ c_index ].startCol;
   width = masterStatusField[ c_index ].width;
   lineNumber = masterStatusField[ c_index ].lineNum;
   dispMessage( field, printable(tex + 2), startingColumn, lineNumber, width );
}


void showButton(char *cptr)
{
   int             blen,
                   bnum;
   static int      bn_saved,
                   curlen;

   DPRINT1("entered showButton with message: %s\n", cptr );
   bnum = *cptr - '0';
   if (bnum == 0)
   {
      removeButtons();
      bn_saved = 0;
      curlen = 0;
   }
   else
   {
      blen = strlen(cptr+1);
      if (*(cptr + blen) == '\n')
      {
	 *(cptr + blen) = '\0';
	 blen--;
      }
/* this is java's problem now! */
/*
 *   if (blen + curlen > maxbuttonchars)
 *   {
 *       Werrprintf("Too many characters in menu labels: %d %d",blen,curlen);
 *       return;
 *   }
 */

#ifdef VNMRJ
      if (strcmp("doneButtons",cptr) == 0)
      {
         writelineToVnmrJ("button","-2 doneButtons");
         return;
      }
#endif 
      DPRINT2("old button: %d   new: %d\n", bn_saved, bnum);
      if (bn_saved + 1 != bnum)
      {
	 fprintf(stderr, "button number mismatch in master\n");
	 return;
      }
      bnum--;			/* Button number to C-style index */

      buildButton(bnum, cptr+1);
      curlen += blen + buttonspacechars;
      bn_saved = bnum + 1;	/* bn_saved is a button number  */
   }
}

/* disable Tcl for vnmrj, gives "Connect error" */
void initTcl(char *tex)
{
   (void) tex;
/*   extern int explicitFocus;
   if (strlen(tex))
   {
      TclPort = atoi(tex);
      initToTclDg(TclPort);
      strcpy(TclDefaultCmd,"");
      if (explicitFocus == 0)
      {
         sendToTclDg("pointerFocus");
      }
   }
*/
}

void initTclCmd(char *tex)
{
   (void) tex;
/*   if (strlen(tex))
      strcpy(TclDefaultCmd,tex);
   else
      strcpy(TclDefaultCmd,"");
*/
}

void sendTcl(char *tex)
{
   (void) tex;
/*   if (TclPid && TclPort) */
/*   {
      if (strlen(tex))
      {
         sendToTclDg(tex);
      }
      else if (strlen(TclDefaultCmd))
      {
         sendToTclDg(TclDefaultCmd);
      }
   }
*/
}

/*
 * static void Acqi()
 * {
 *    write(forgroundFdW, "acqi", 4);
 *    sendChildNewLine();
 * }
 */

static void execCmd(char *tex)
{
   write(forgroundFdW, tex, strlen(tex));
   sendChildNewLine();
}

void gotEscSeq(char *msg)
{
   char            type;
   char           *tex;
   int             len,
                   pid;
   struct acqmsgentry *curptr;

   sscanf(msg + 3, "%7u", &pid);
   type = msg[10];
   tex = &(msg[11]);
   DPRINT3("M: Message from pid %u, type %c, \"%s\"\n", pid, type, printable(tex));
   switch (type)
   {
      /*
      case 'A':
	 buildMainButton(-1, "Acqi", Acqi);
	 break;
      case 'a':
	 set_vnmr_button_by_name("Acqi", 0);
	 break;
       */
      case 'B':
	 ring_bell();
	 break;
      case 'b':
	 showButton(tex);
	 break;
      case 'C':
	 execCmd(tex);
	 break;
      case 'd':
	 data_station = 1;
	 break;
      case 'D':
	 data_station = 0;
	 break;
      case 'e':
	 setErrorwindow(tex);
	 break;
      case 'E':
	 showError(tex);
	 break;
	 /* insert text in command window */
         /* vnmrj sending input command */
/*
 *    case 'i':
 *	 jInput = 1;
 *	 break;
 */
      case 'I':
#ifdef VNMRJ
	 jInput = 1;
#endif 
/*
	 ttyInput(tex, strlen(tex));
*/
	 break;
      case 'f':
/*
	 ttyFocus(tex);
*/
	 break;
      case 'F':		/*
                         *  do textsw editing functions Two functions:
			 *  clear the text subwindow or position the
			 *  cursor at the bottom
                         */
	 setTextwindow(tex);
	 break;
      case 'g':
	 setOnCancel(tex); /* alternate events when Cancel command is sent */
	 break;
      case 'j':
         //  jFunc(2,tex); /* close java fd's etc */
	 resetMasterSockets(); /* close java fd's etc */
	 break;
      case 'J': 	/* click menu button from java */
	 but_interpos_handler_jfunc(tex);
	 break;
      case 'l':		/* Make small/large button large */
	 setLargeSmallButton("l");
	 break;
      case 'p':
	 show_vnmrj_alphadisplay();
	 break;
      case 'P':
         fileDump(tex);
         break;
      case 'r':
/*
	 raise_text_window();
*/
	 break;
      case 'R':		/* Child is ready for input from Acqproc */

	 DPRINT2("child complete, %d  %p\n", fgBusy, baseofqueue);
	 if (fgBusy > 0)
	    fgBusy--;
         setOnCancel("");
	 if (baseofqueue && fgBusy == 0)
	 {
	    curptr = removeAcqMsgEntry();
	    DPRINT1("new base of queue: %p\n", baseofqueue);
	    len = strlen(&curptr->curentry[0]);
	    write(forgroundFdW, &curptr->curentry[0], len);
	    sendChildNewLineNoClear();	/* Increments fgBusy */
	    release(curptr);
	 }
	 DPRINT1("fgBusy now %d\n", fgBusy);

	 break;
      case 's':		/* Make small/large button small */
	 setLargeSmallButton("s");
	 break;
      case 'S':
	 showStatus(tex);
	 break;
/* disable Tcl for vnmrj, gives "Connect error" */
/*    case 'T':
	 sendTcl(tex);
	 break;
      case 'U':
	 initTcl(tex);
	 break;
      case 'V':
	 initTclCmd(tex);
	 break;
*/
      case 'X':
         if (xfd != NULL) {
            fclose(xfd);
            xfd = NULL;
         }
        if (strcmp("null", tex) == 0)
            return;
        if (strncmp(tex, "/dev/", 5) == 0) {
            if (access(tex, W_OK) != 0)
               return;
        }
        xfd = fopen(tex, "w");
        if (xfd != NULL) {
            dup2(fileno(xfd), 2);
        }
         break;
      default:
	 DPRINT1("M: Don't know about message type '%c'\007\n", type);
   }
}

void chewEachChar(char *buffer, int size)
{
   char            c;

   while (0 < size)
   {
      c = *buffer++;
      switch (fgEscapeMode)
      {
	 case 0:
	    switch (c)
	    {
	       case '\033':
		  if (fgReaderBp)
		  {
		     fgReaderBuffer[fgReaderBp] = '\0';
		     textprintf(fgReaderBuffer, fgReaderBp);
		     fgReaderBp = 0;
		  }
		  fgReaderBuffer[fgReaderBp++] = c;
		  fgEscapeMode = 1;
		  break;
	       case '\n':

	       case '\t':
		  fgReaderBuffer[fgReaderBp++] = c;
		  break;
	       default:
		  if (c < ' ')
		     fgReaderBuffer[fgReaderBp++] = '^';
		  fgReaderBuffer[fgReaderBp++] = c;
	    }
	    break;
	 case 1:
	 case 2:
	    fgReaderBuffer[fgReaderBp++] = c;
	    switch (c)
	    {
	       case '\033':
		  fgEscapeMode += 1;
		  break;
	       default:
		  fgReaderBuffer[fgReaderBp] = '\0';
		  textprintf(fgReaderBuffer, fgReaderBp);
		  fgReaderBp = 0;
		  fgEscapeMode = 0;
	    }
	    break;
	 case 3:
	    switch (c)
	    {
	       case '\n':
		  fgReaderBuffer[fgReaderBp] = '\0';
		  gotEscSeq(fgReaderBuffer);
		  fgReaderBp = 0;
		  fgEscapeMode = 0;
		  break;
	       default:
		  fgReaderBuffer[fgReaderBp++] = c;
	    }
      }
      size -= 1;
   }
   if (fgEscapeMode == 0 && fgReaderBp)
   {
      fgReaderBuffer[fgReaderBp] = '\0';
      textprintf(fgReaderBuffer, fgReaderBp);
      fgReaderBp = 0;
   }
}

/*------------------------------------------------------------------------------
|
|	The various handlers...
|
+-----------------------------------------------------------------------------*/

static char cancelCommand[512];

static void setOnCancel(char *buffer)
{
   strcpy(cancelCommand, buffer);
}

void cancelCmd()
{
   int             code;
   int             n;

/* could make special cancelCmd for W_getInputCR to not showError, don't toss chars? */

   showError("Cancel sent, waiting for acknowledge");
#ifdef __INTERIX
   code = fcntl( forgroundFdR, F_GETFL, 0 );
   code |= O_NONBLOCK;
   fcntl( forgroundFdR, F_SETFL, code );
   n = 1;
   while (0 < n)
   {
      char buffer[1024];

      n = read(forgroundFdR, buffer, 1024);
   }
   kill(forgroundPid, SIGINT);
#else
   code = ioctl(forgroundFdR, FIONREAD, &n);
   kill(forgroundPid, SIGINT);
   if (0 <= code)
   {
      DPRINT2("M: tossing %d character(s) on fd %d\n", n, forgroundFdR);
      while (0 < n)
      {
	 char            buffer[1024];

	 if (1024 < n)
	 {
	    read(forgroundFdR, buffer, 1024);
	    n -= 1024;
	 }
	 else
	 {
	    read(forgroundFdR, buffer, n);
	    n = 0;
	 }
      }
   }

   if (cancelCommand[0] != '\0')
      system(cancelCommand);

   // Kill recon if running
   // Get the path to xrecon_fid_file in users home directory and open it
   char *filepath;
   FILE *fd;
   char pidfilename[200];
   int ipid;


   filepath = getenv("HOME");

	strcpy(pidfilename, filepath);
	strcat(pidfilename, "/xrecon_pid_file");


   fd = fopen(pidfilename, "r");
   if(fd != NULL) {
      ssize_t retsize = fscanf(fd,"%d",&ipid);
      fclose(fd);
      if (retsize > 0) {
         if (ipid > 0) {
            kill(ipid, SIGUSR1);
         }
      }
   }

   

#endif
}

void
gotInterupt2()
{
   DPRINT1("M: Kill the kid! (pid= %u)\n", forgroundPid);
   kill(forgroundPid, SIGKILL);
}

void
gotBadInterupt()
{
   DPRINT1("M: The shame is to much! Kill the kid (pid %u)\n", forgroundPid);
   kill(forgroundPid, SIGKILL);
   DPRINT0("M: ...and myself!\n");
   exit(-1);
}



#ifdef MOTIF
void
bgPipeIsReady(fd)
int             fd;
{
   int             n;
#ifdef __INTERIX
   struct stat     sinfo;
#endif

   DPRINT0("bgPipeIsReady:starting\n");
#ifdef __INTERIX
   if (fstat(fd, &sinfo) >= 0)
#else
   if (0 <= ioctl(fd, FIONREAD, &n))
#endif
   {
#ifdef __INTERIX
      n = sinfo.st_size;
#endif
      while (0 < n)
      {
	 char            c;

	 read(fd, &c, 1);
	 switch (bgEscapeMode)
	 {
	    case 0:
	       switch (c)
	       {
		  case '\033':
		     bgReaderBp = 0;
		     DPRINT2("M: bg/0 -- \\%03o at %d\n", c, bgReaderBp);
		     bgReaderBuffer[bgReaderBp++] = c;
		     bgEscapeMode = 1;
		     break;
		  default:
		     bgReaderBp = 0;
	       }
	       break;
	    case 1:
	    case 2:
	       switch (c)
	       {
		  case '\033':
		     DPRINT3("M: bg/%d -- \\%03o at %d\n", bgEscapeMode, c, bgReaderBp);
		     bgReaderBuffer[bgReaderBp++] = c;
		     bgEscapeMode += 1;
		     break;
		  default:
		     bgReaderBp = 0;
		     bgEscapeMode = 0;
	       }
	       break;
	    case 3:
	       switch (c)
	       {
		  case '\n':
		     DPRINT2("M: bg/3 -- \\%03o at %d\n", '\0', bgReaderBp);
		     bgReaderBuffer[bgReaderBp++] = '\0';
		     DPRINT0("M: SHIP IT!\n");
		     gotEscSeq(bgReaderBuffer);
		     bgReaderBp = 0;
		     bgEscapeMode = 0;
		     break;
		  default:
		     DPRINT2("M: bg/3 -- \\%03o at %d\n", c, bgReaderBp);
		     bgReaderBuffer[bgReaderBp++] = c;
	       }
	 }
	 n -= 1;
      }
   }
}
#endif

/*  Routine called when the foreground child (Vnmr) writes to
    what he thinks is 'stdout'.  Key trick is to 'suspend' the
    pipe if too many characters exist to process at once.  In
    that situation 'checkPipe' will be called every second.
    That routine then calls this routine.  Eventually, all the
    data in the pipe will be consumed, at which point the
    interval time will be turned off.

    Otherwise, this routine checks for errors and sends valid
    data to be processsed by 'chewEachChar'.			*/

void
fgPipeIsReady(fd)
int             fd;
{
   char            IObuffer[MAXREAD];
   int            n, len;

   DPRINT0("fgPipeIsReady:starting\n");
   if (0 <= ioctl(fd, FIONREAD, &n))
   {
      while (n > 0)
      {
	 len = n;
	 DPRINT2("M: %ld char(s) availible on fd %d\n", n, fd);
	 if (n > MAXREAD)
	 {
	    len = MAXREAD;
	    pipeSuspended = 1;
            DPRINT1("M: ...must suspend (activate timer and read only %ld of them)\n", n);
/*
            setCallbackTimer(100);
*/
	 }
	 else if (pipeSuspended)
	 {
	    pipeSuspended = 0;
	    DPRINT0("M: ...don't suspend anymore (kill timer)\n");
	 }
	 else
	    DPRINT0("M: ...read all of them\n");
	 read(fd, IObuffer, len);
	 n -= len;
	 chewEachChar(IObuffer, len);
      }
   }
}

void
checkPipe()
{
   if (pipeSuspended)
   {
      DPRINT0("M: Go check suspended pipe...\n");
      fgPipeIsReady(forgroundFdR);
   }
}


void
forgroundExpired(int pid)
{

   DPRINT1("M: forground task (pid %u) killed\n", pid);

   clearForground(pipeSuspended);

   close(forgroundFdR);
   close(forgroundFdW);

   DPRINT0("M: exit -- forgroundExpired\n");
   unlockAllExp(&userdir[0], pid, FOREGROUND);
   clear_acq();
   locklc_("unlock", pid);
   exit(0);
}

/*--------------------------------------------------------------------------
|	AbortAcq
|
|	This routine sends an abort command string to the Acquisition
|	task.
|
|
+-------------------------------------------------------------------------*/
#define ACQABORT 4

extern char     UserName[];

void
AbortAcq()
{
   char            message[128];

   DPRINT0("AbortAcq: starting\n");
   if (!data_station)
   {
      if ( ACQOK(HostName) )
      {
         sprintf(message, "%s %s", UserName, "junk");
         send2Acq(ACQABORT, message);
      }
      else
      {
         Werrprintf("Acquisition process not active, no abort");
      }
   }
   else
   {
      Werrprintf("Abort Acq not active on a data station");
   }
}

/*  routine is not used anymore, used to be used with EXIT command */
/*  This is left here just incase we need to bring it back         */
/* send exit command to vnmr */
/*
 *  quit()
 * {   DPRINT0("M: exit (via quit)\n");
 *     write(forgroundFdW,"exit",4);
 *     sendChildNewLine();
 *     sleep(5);
 * }
 */

/*--------------------------------------------------------------------------
|	Menu    MainMenu
|
|	These three routines shove ascii commands down the pipe as if
|	Rene had just typed them on the keyboard
|
+-------------------------------------------------------------------------*/

void menU()
{
   DPRINT0("M: executing Menu\n");
   write(forgroundFdW, "menu", 4);
   sendChildNewLine();
}

void mainMenu()
{
   DPRINT0("M: executing Menu\n");
   write(forgroundFdW, "menu('main')", 12);
   sendChildNewLine();
}

void l_exit_vnmr()
{
   DPRINT0("M: executing Exit\n");
   write(forgroundFdW, "exit", 4);
   sendChildNewLine();
}

void l_usermac1()
{
   DPRINT0("M: executing usermacro1\n");
   write(forgroundFdW, "usermacro1", 10);
   sendChildNewLine();
}

void l_usermac2()
{
   DPRINT0("M: executing usermacro2\n");
   write(forgroundFdW, "usermacro2", 10);
   sendChildNewLine();
}

void l_usermac3()
{
   DPRINT0("M: executing usermacro3\n");
   write(forgroundFdW, "usermacro3", 10);
   sendChildNewLine();
}

void l_usermac4()
{
   DPRINT0("M: executing usermacro4\n");
   write(forgroundFdW, "usermacro4", 10);
   sendChildNewLine();
}

void l_usermac5()
{
   DPRINT0("M: executing usermacro5\n");
   write(forgroundFdW, "usermacro5", 10);
   sendChildNewLine();
}

void l_usermac6()
{
   DPRINT0("M: executing usermacro6\n");
   write(forgroundFdW, "usermacro6", 10);
   sendChildNewLine();
}

void l_usermac7()
{
   DPRINT0("M: executing usermacro7\n");
   write(forgroundFdW, "usermacro7", 10);
   sendChildNewLine();
}

void l_usermac8()
{
   DPRINT0("M: executing usermacro8\n");
   write(forgroundFdW, "usermacro8", 10);
   sendChildNewLine();
}

/*--------------------------------------------------------------------------
|	Flip
|
|	This routine sends the flip command to Vnmr.
|
+-------------------------------------------------------------------------*/
void Flip()
{
   write(forgroundFdW, "flip", 4);
   sendChildNewLine();
}

/*--------------------------------------------------------------------------
|	Help
|
|	This routine sends the help command to Vnmr.
|
+-------------------------------------------------------------------------*/
void Help()
{
   write(forgroundFdW, "help", 4);
   sendChildNewLine();
}


void suicide()
{
   while (0 <= kill(forgroundPid, 0))
   {
      kill(forgroundPid, SIGKILL);
      DPRINT0("M: ...wait a moment\007\r");
      sleep(1);
   }
   DPRINT1("\nM: ...%u seems dead (or dying)\n", forgroundPid);
}

/*------------------------------------------------------------------------
|   addChar
|
|   This routine adds a character to a buffer at the cursor position
|    cp and shifts everything  to the right.
|
+------------------------------------------------------------------------*/
void addChar(char c, char *buf, int cp, int bp)
{
   int             ip;

   for (ip = bp; ip > cp; ip--)
      buf[ip] = buf[ip - 1];
   buf[cp] = c;
}

/*------------------------------------------------------------------------
|   delChar
|
|   This routine deletes a character from the buffer at the cursor position
|    cp and shifts everything  to the left.
|
+------------------------------------------------------------------------*/
void delChar(char *buf, int cp, int bp)
{
   int             ip;

   for (ip = cp; ip < bp; ip++)
      buf[ip] = buf[ip + 1];
}

/*------------------------------------------------------------------------------
|
|	Startup the forground job (must work!).
|
+-----------------------------------------------------------------------------*/

void startForground(char *name, char *vnmrj_portid,
                    char *vnmrj_viewid, char *vnmrj_hostname)
{
   int             fdR[2];
   int             fdW[2];
   char            jParentName[20];

   DPRINT1("M: Starting forground job \"%s\"\n", name);
   sprintf(jParentName, "-P%d", getppid() );
   if (0 <= pipe(fdR))
      if (0 <= pipe(fdW))
      {
	 DPRINT4("pipes fdR 0=%d 1=%d fdW 0=%d 1=%d\n",
		 fdR[0], fdR[1], fdW[0], fdW[1]);
#ifdef __INTERIX
         /* For some reason, fork works on some, but not all Windows XP
          * systems.  vfork() works on all the XP systems.
          */
	 if ((forgroundPid = vfork()) < 0)
#else
	 if ((forgroundPid = fork()) < 0)
#endif
	 {
	    fprintf(stderr, "master: Could not fork forground process!\n");
	    close(fdR[0]);
	    close(fdR[1]);
	    close(fdW[0]);
	    close(fdW[1]);
	    exit(1);
	 }
	 else if (forgroundPid)	/* we are the parent here */
	 {
	    forgroundFdR = fdR[0];
	    close(fdR[1]);
	    close(fdW[0]);
	    DPRINT2("M: ...closed fd %d and %d\n", fdR[1], fdW[0]);
	    forgroundFdW = fdW[1];
	    waitForgroundChild(forgroundFdR, forgroundPid);
	    DPRINT3("M: ...forground pid= %u, reader= %d, writer=%d\n", forgroundPid, forgroundFdR, forgroundFdW);
	 }
	 else			/* child process */
	 {
	    int i;

	    DPRINT2(" M: fd fdW[0]=%d fdR[1]=%d \n", fdW[0], fdR[1]);
	    close(fdR[0]);
	    close(fdW[1]);
	    if (dup2(fdW[0], 0) < 0)
	       perror("dup2(_,0)");
	    close(fdW[0]);
	    if (dup2(fdR[1], 1) < 0)
	       perror("dup2(_,1)");
/*	       if (dup2(fdR[1],2) < 0)  lets keep stderr intact for now */
/*		  perror("dup2(_,2)"); */
	    close(fdR[1]);
	    /* lets use file descriptor 4 for textFrameFd */
#ifndef X11
	    if (dup2(i, 4) < 0)
	    {
	       perror("dup2(_,4)");
	    }
#endif 
	    for (i = 5; i < 30; ++i)
	       close(i);
	    {
	       char            mserver[20];
	       char            mback[20];
	       char            addr[257];
               char            font_name[122];
               char            port_name[20];
               char            progName[512];

	       /* Pass the heigth and width of master screen */
               if (Bserver)
	           sprintf(mserver, "-xserver");
               else 
	           sprintf(mserver, "-xnull");
               if (bBack)
	           sprintf(mback, "-mback");
               else 
	           sprintf(mback, "-xnull");
	       if (def_font)
                   sprintf(font_name, "-Z%s", fontName);
	       else
                   sprintf(font_name, "-Z");
	       sprintf(port_name, "-xport_%d", htons(inputPort));
               strcpy(addr,"-h");
               GET_VNMR_ADDR(&addr[2]);
#ifdef __CYGWIN__
               sprintf(progName,"%s/bin/Vnmrbg.exe",systemdir);
#else
               strcpy(progName,name);
#endif

#ifdef DEBUG
	       if (debug)
	       {
		  if (strlen(Xserver) > 0)
                    execlp(progName, "Vnmrch", "-t", "-mforeground", addr, mserver, port_name, mback, Xdisplay, Xserver, font_name,"-port",vnmrj_portid,"-view",vnmrj_viewid,"-host",vnmrj_hostname, jParentName, vjXInfo, NULL);
		  else
                    execlp(progName, "Vnmrch", "-t", "-mforeground", addr, mserver, port_name, mback, font_name,"-port",vnmrj_portid,"-view",vnmrj_viewid,"-host",vnmrj_hostname, jParentName, vjXInfo,NULL);
	       }
	       else
#endif 
	       {
		  if (strlen(Xserver) > 0)
                     execlp(progName, "Vnmrch", "-mforeground", addr, mserver,port_name, mback, Xdisplay, Xserver,font_name, "-port",vnmrj_portid,"-view",vnmrj_viewid,"-host",vnmrj_hostname, jParentName, vjXInfo, NULL);
		  else
                     execlp(progName, "Vnmrch", "-mforeground", addr, mserver, port_name, mback, font_name, "-port",vnmrj_portid,"-view",vnmrj_viewid,"-host",vnmrj_hostname, jParentName, vjXInfo, NULL);
	       }
	       fprintf(stderr, "master: execl(%s,Vnmrch,0) failed!\n", progName);
	       exit(1);
	    }
	 }
      }
      else
      {
	 fprintf(stderr, "master: Can't open pipe to forground or more\n");
	 close(fdR[0]);
	 close(fdR[1]);
	 exit(1);
      }
   else
   {
      fprintf(stderr, "master: Can't open pipe from forground\n");
      exit(1);
   }
}


void master(int argc, char *argv[])
{
   char           *fgName;
   char           *p;
   int             i;
   char		   vnmrj_hostname[128];
   char		   vnmrj_portid[128] = "0";
   char		   vnmrj_viewid[4] = "1";
   FILE            *fj;

   setbuf(stderr, NULL);

   createVSocket( SOCK_STREAM, &sVnmrJaddr );
   createVSocket( SOCK_STREAM, &sVnmrJcom );
   createVSocket( SOCK_STREAM, &sVnmrJAl );
   createVSocket( SOCK_STREAM, &sVnmrGph );

   fgReaderBuffer[0] = '\0';
   Xserver[0] = '\0';
   Xdisplay[0] = '\0';
   fontName[0] = '\0';
   vnmrj_hostname[0] = '\0';
   Bserver = 0; 
   noUI = 0; 
   fgBusy = 0;
   fgName = NULL;
   if (argc > 0)
       fgName = argv[0];
   sprintf(vjXInfo, "-jxid 0 ");
   strcpy(vnmrj_hostname, HostName);

   setupMasterStatusFields();
   setOnCancel("");

/*  Define graphics so window routines will not crash...  */

   textTypeIsTcl = NULL;
   graphics = "sun";
   if ( !strcmp(graphics,"sun") )
   {
      maxbuttonchars = 128;
      buttonspacechars = 0;
   }

#ifndef DBXTOOL

   for (i = 1; i < argc; ++i)
   {
      p = argv[i];
      if (*p == '-')
      {
         if (!strcmp(argv[i], "-display"))
         {      i++;
                sprintf(Xdisplay, "-display");
                if (strlen(argv[i]) > 0)
                   sprintf(Xserver, "%s", argv[i]);
                continue;
         }
         if (!strcmp(argv[i], "-fn"))
         {      i++;
                if (strlen(argv[i]) > 0)
		{
                   sprintf(fontName, "%s", argv[i]);
		   def_font = 1;
		}
                continue;
         }
         if (!strcmp(argv[i], "-host"))
         {      i++;
                if (strlen(argv[i]) > 0)
		{
		   strcpy(vnmrj_hostname,argv[i]);
                }
                continue;
         }
         if (!strcmp(argv[i], "-port"))
         {      i++;
                if (strlen(argv[i]) > 0)
		{
		   strcpy(vnmrj_portid,argv[i]);
                }
                continue;
         }
         if (!strcmp(argv[i], "-view"))
         {      i++;
                if (strlen(argv[i]) > 0)
		{
		   strcpy(vnmrj_viewid,argv[i]);
                }
                continue;
         }
         if (!strncmp(argv[i], "-jx", 3))
         { 
		strcat(vjXInfo,argv[i]); 
		i++;
		if (i < argc) {
		   strcat(vjXInfo," "); 
		   strcat(vjXInfo,argv[i]); 
		}
		strcat(vjXInfo," "); 
                continue;
         }
         if (!strcmp(argv[i], "-p"))
         {      i++;
                if (i < argc && strlen(argv[i]) > 0)
		{
                   inputPort = atoi(argv[i]);
                }
                continue;
         }

         if (!strcmp(argv[i], "-mserver"))
         {
                Bserver = 1;
                noUI = 1;
                continue;
         }
         if (!strcmp(argv[i], "-mback"))
         {
                bBack = 1;
                continue;
         }
         if (!strcmp(argv[i], "-exec"))
         {      i++;
                if (i < argc && strlen(argv[i]) > 0)
                   insertAcqMsgEntry(argv[i]);
                continue;
         }

	 while (*++p)
	 {
	    switch (*p)
	    {
	       case 'd':
		  debug += 1;
		  break;
	       default:
		  fprintf(stderr, "master: unknown option -%c\n", *p);
                  {
                     fj = fopen(Jvbgname,"a");
                     if (fj != NULL) {
                        fprintf(fj,"master: unknown option -%c\n", *p);
                        fclose(fj);
                     }
                  }
		  /* exit(1);   may want to exit if background mode? */
	    }
	 }
      }
      else if (fgName == NULL)
	 fgName = p;
      else
      {
/*
	 fprintf(stderr, "master: too many names\n");
         fj = fopen(Jvbgname,"a");
         if (fj != NULL) {
             fprintf(fj,"master: too many names\n");
             fclose(fj);
         }
*/
/*	 exit(1); */ /* may want to exit if background mode? */
      }
   }

#else 
    fgName = "Vnmrbg";
#endif 
   if (debug) {
       fprintf(stderr, " Vnmrbg args: ");
       for (i = 0; i < argc; ++i)
          fprintf(stderr, "  %d: %s \n", i, argv[i]);
   }

   if (fgName == NULL) {
      if (argc > 0)
         fgName = argv[0];
      else
         fgName = "Vnmrbg";
   }

      VnmrJPortId = atoi(vnmrj_portid);
      VnmrJViewId = atoi(vnmrj_viewid);
      strcpy(VnmrJHostName,vnmrj_hostname); /* same as in main.c */
      sprintf(Jvbgname, "%s/tmp/vbgtmp%d", systemdir, VnmrJPortId);

   if (fgName)
   {

      close(1);			/* we never use stdout */
      INIT_VNMR_COMM(HostName);	/* To receive stuff from acq process */
      inputPort = init_vnmrj_comm(vnmrj_hostname, inputPort); /* same as VnmrJHostName, HostName */
			/* do I want local hostname or one vnmrj starts from? */

      setupdirs("master");

      /* create the Acqisition Process socket (return port number) */
#ifndef NOACQ
      INIT_ACQ_COMM(systemdir);/* To send stuff to acq process */
#endif
      createWindows2( 1, noUI );
      readyCatchers();
      if (smagicSendJvnmraddr(inputPort))
      {
	 fprintf(stderr, "master: failed to communicate with GUI\n");
         exit(0);
      }
      DPRINT1("M: startForground for %s \n", fgName);
      startForground(fgName,vnmrj_portid,vnmrj_viewid,vnmrj_hostname); /* which port_id do I send? */
      DPRINT0("M: Into the notifier loop...\n");
/* but loop must work */
      MasterLoop(); /* infinite loop, never exits unless vnmrexit is called */

      DPRINT0("M: ...the notifier loop has exited!\n");
/*
      exit(0);
*/
   }
   else
   {
      fprintf(stderr, "master: No forground requested\n");
      exit(1);
   }
}
