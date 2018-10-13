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
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "locksys.h"
#include "acquisition.h"

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

#else   DEBUG
#define DPRINT0(str)
#define DPRINT1(str, arg2)
#define DPRINT2(str, arg1, arg2)
#define DPRINT3(str, arg1, arg2, arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4)
#endif  DEBUG

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
extern char    *graphics;
extern char     userdir[];
extern char     systemdir[];
int             forgroundFdW;
int             masterWindowHeight2 = 0;
int             masterWindowWidth2 = 0;
int             textWindowHeight = 0;
int             textWindowWidth = 0;
int             resizeBorderWidth = 10;
extern long     textFrameFd;

/*  Counter to indicate number of commands that the master has
    sent to the child.  Only when this counter is 0 can messages
    from Acqproc be sent to the child.				*/

int             fgBusy;		/* Used by socket1.c    */
extern struct acqmsgentry *baseofqueue;	/* Defined in socket1.c */
extern struct acqmsgentry *removeAcqMsgEntry();	/* Defined in socket1.c */


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

static char     default_graphics[4] = "sun";
static char     bgReaderBuffer[256];		/* bgReaderBuffer isn't used */
static char     fgReaderBuffer[1028];	/* prevent overflow from long strings */
static int      data_station = 1;       /* parameter to distinguish data station and spectrometer */
                                        /* only used by AbortAcq */
static int      def_font = 0;
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
int      TclPid;
int      TclPort = 0;

char     Xserver[64], Xdisplay[10]; /* prepared for X-Window */
char     fontName[120], textFrameFds[12];
char     mainFrameFds[12];
char     buttonWinFds[12];

static int maxbuttonchars = 80;
static int buttonspacechars = 2;


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
sendChildNewLine()		/* Not a static for it   */
{				/* is used by socket1.c  */
   write(forgroundFdW, "\n", 1);
   fgBusy++;
   DPRINT0("child now busy\n");
}

/*
 *  This routine is the same as above except line 3 is not cleared
 *  See processChar, which examines NEWLINE and RETURN
 */
sendChildNewLineNoClear()
{
   write(forgroundFdW, "\r", 1);
   fgBusy++;
   DPRINT0("child now busy\n");
}

/*------------------------------------------------------------------------------
|
|	We have got an escape sequence from a child, deal with it!  A child
|	sends a message by prefixing it with exactly THREE escapes (\033) and
|	terminates it with a newline (\n).  Usually the first 6 characters
|	are the pid (in %6u format) of the sender.  For example...
|
|		printf("\033\033\033%6uTActive\n",getpid());
|
|	A good way to make this fairly clean is to make a message sender
|	function...
|
|		sendStatus(m)		char *m;
|		{  printf("\033\033\033%6uT%s\n",getpid(),m);
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
printable(msg)
char           *msg;
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
   }
   while (*msg && (p-q) < sizeof(buffer1) - 3)
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

showDispPrint(msg)
char           *msg;
{
    dispMessage(1, printable(msg), 0, 0, 8);
}

showDispPlot(msg)
char           *msg;
{
   dispMessage(2, printable(msg), 8, 0, 8);
}

showDispBg(msg)
char           *msg;
{
   fprintf(stderr, "master: reference made to non-existant background field!\n");
}

showDispSeq(msg)
char           *msg;
{
   dispMessage(4, printable(msg), 23, 0, 14);
}

showDispExp(msg)
char           *msg;
{
   dispMessage(5, printable(msg), 37, 0, 8);
}

showDispSpecIndex(msg)
char           *msg;
{
   dispMessage(6, printable(msg), 44, 0, 12);
}

showDispAcq(msg)
char           *msg;
{
   dispMessage(7, printable(msg), 56, 0, 10);
}

showDispStatus(msg)
char           *msg;
{
   dispMessage(8, printable(msg), 66, 0, 10);
}

showDispIndex(msg)
char           *msg;
{
   dispMessage(9, printable(msg), 76, 0, 8);
}

showDispPlane(msg)
char           *msg;
{
   dispMessage(10, printable(msg), 44, 1, 8);
}

showError(msg)
char           *msg;
{
   DPRINT1( "M: showError: %s\n", msg );
   dispMessage(0, printable(msg), 0, 2, 80);
}

void
showStatus(tex)
char           *tex;
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


showButton(cptr)
char           *cptr;
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
      if (blen + curlen > maxbuttonchars)
      {
         Werrprintf("Too many characters in menu labels");
	 return;
      }
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

initTcl(tex)
char *tex;
{
   extern int explicitFocus;
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
}

initTclCmd(tex)
char *tex;
{
   if (strlen(tex))
      strcpy(TclDefaultCmd,tex);
   else
      strcpy(TclDefaultCmd,"");
}

sendTcl(tex)
char *tex;
{
   if (TclPid && TclPort)
   {
      if (strlen(tex))
      {
         sendToTclDg(tex);
      }
      else if (strlen(TclDefaultCmd))
      {
         sendToTclDg(TclDefaultCmd);
      }
   }
}

static Acqi()
{
   write(forgroundFdW, "acqi", 4);
   sendChildNewLine();
}

static execCmd(tex)
char *tex;
{
   write(forgroundFdW, tex, strlen(tex));
   sendChildNewLine();
}

gotEscSeq(msg)
char           *msg;
{
   char            type;
   char           *tex;
   int             len,
                   pid;
   struct acqmsgentry *curptr;

   sscanf(msg + 3, "%6u", &pid);
   type = msg[9];
   tex = &(msg[10]);
   DPRINT3("M: Message from pid %u, type %c, \"%s\"\n", pid, type, printable(tex));
   switch (type)
   {
      case 'A':
	 buildMainButton(-1, "Acqi", Acqi);
	 break;
      case 'a':
	 set_vnmr_button_by_name("Acqi", 0);
	 break;
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
      case 'I':
	 ttyInput(tex, strlen(tex));
	 break;
      case 'f':
	 ttyFocus(tex);
	 break;
      case 'F':		/*
                         *  do textsw editing functions Two functions:
			 *  clear the text subwindow or position the
			 *  cursor at the bottom
                         */
	 setTextwindow(tex);
	 break;
      case 'l':		/* Make small/large button large */
	 setLargeSmallButton("l");
	 break;
      case 'P':
         fileDump(tex);
         break;
      case 'r':
	 raise_text_window();
	 break;
      case 'R':		/* Child is ready for input from Acqproc */

	 DPRINT2("child complete, %d  %x\n", fgBusy, baseofqueue);
	 if (fgBusy > 0)
	    fgBusy--;
	 if (baseofqueue && fgBusy == 0)
	 {
	    curptr = removeAcqMsgEntry();
	    DPRINT1("new base of queue: %x\n", baseofqueue);
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
      case 'T':
	 sendTcl(tex);
	 break;
      case 'U':
	 initTcl(tex);
	 break;
      case 'V':
	 initTclCmd(tex);
	 break;
      default:
	 DPRINT1("M: Don't know about message type '%c'\007\n", type);
   }
}


chewEachChar(buffer, size)
char           *buffer;
int             size;
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

cancelCmd()
{
   int             code;
   int             n;
   struct stat     sinfo;

   showError("Cancel sent, waiting for acknowledge");
/*
   code = ioctl(forgroundFdR, FIONREAD, &n);
*/
   code = fstat(forgroundFdR, &sinfo);
   kill(forgroundPid, SIGINT);
   if (0 <= code)
   {
      n = sinfo.st_size;
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
#ifdef IRIX
   focusOfTty(1);
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


#define maxRead 256

void
bgPipeIsReady(fd)
int             fd;
{
   int             n;
   struct stat     sinfo;

   DPRINT0("bgPipeIsReady:starting\n");
/*
   if (0 <= ioctl(fd, FIONREAD, &n))
*/
   if (fstat(fd, &sinfo) >= 0)
   {
      n = sinfo.st_size;
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
   char            IObuffer[maxRead];
   long            n;
   struct stat     sinfo;

   DPRINT0("fgPipeIsReady:starting\n");
/*
   if (0 <= ioctl(fd, FIONREAD, &n))
*/
   if (fstat(fd, &sinfo) >= 0)
   {
      n = sinfo.st_size;
      if (0 < n)
      {
	 DPRINT2("M: %d char(s) availible on fd %d\n", n, fd);
	 if (maxRead < n)
	 {
	    n = maxRead;
#ifndef  MOTIF
	    if (pipeSuspended == 0)
	    {
	       pipeSuspended = 1;
	       DPRINT1("M: ...must suspend (activate timer and read only %d of them)\n", n);
	       setCallbackTimer(1);
	    }
	    else
	       DPRINT1("M: ...remain suspended (read only %d of them)\n", n);
#else    MOTIF
	    pipeSuspended = 1;
            DPRINT1("M: ...must suspend (activate timer and read only %d of them)\n", n);
            setCallbackTimer(100);
#endif
	 }
	 else if (pipeSuspended)
	 {
	    pipeSuspended = 0;
	    DPRINT0("M: ...don't suspend anymore (kill timer)\n");
#ifndef  MOTIF
	    clearCallbackTimer();
#endif
	 }
	 else
	    DPRINT0("M: ...read all of them\n");
	 read(fd, IObuffer, n);
	 chewEachChar(IObuffer, n);
      }
      else
	 DPRINT1("M: but no chars on fd %d!\n", fd);
   }
   else
   {
      DPRINT1("M: ...notified but the ioctl on %d failed\n", fd);
      {
	 char            buffer[56];

	 sprintf(buffer, "M: ioctl(%d,FIONREAD,_):", fd);
	 perror(buffer);
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
forgroundExpired(pid)
int	pid;
{

   DPRINT1("M: forground task (pid %u) killed\n", pid);

   clearForground(pipeSuspended, pid, forgroundFdR);

#ifdef DEBUG
   if (debug)
   {
      struct stat     sinfo;
      if (fstat(forgroundFdR, &sinfo) >= 0)
      {
	 if (0 < sinfo.st_size)
	    DPRINT2("M: ...%d char(s) remaining on fd %d are lost\n", sinfo.st_size, forgroundFdR);
	 else
	    DPRINT1("M: ...and no chars remain on fd %d\n", forgroundFdR);
      }
      else
      {
	 DPRINT1("M: ...and the ioctl on %d failed\n", forgroundFdR);
	 {
	    char            buffer[256];

	    sprintf(buffer, "M: ioctl(%d,FIONREAD,_):", forgroundFdR);
	    perror(buffer);
	 }
      }
   }
#endif DEBUG

   /*if (0 <= ioctl(forgroundFdR, TIOCFLUSH, 0))
   {
      DPRINT1("M: Flushed fd %d\n", forgroundFdR);
   }
   else
   {
      DPRINT1("M: Can't flush fd %d\n", forgroundFdR);
   }*/

   close(forgroundFdR);
   close(forgroundFdW);

   DPRINT0("M: exit -- forgroundExpired\n");
   unlockAllExp(&userdir[0], pid, FOREGROUND);
   clear_acq();
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
#ifdef IRIX
   focusOfTty(1);
#endif
}

/*-----------------------------------------------------------------------*/
/*  routine is not used anymore, used to be used with EXIT command */
/*  This is left here just incase we need to bring it back         */
/* quit()
/* {   DPRINT0("M: exit (via quit)\n");
    /* send exit command to vnmr */
/*     write(forgroundFdW,"exit",4);
/*     sendChildNewLine();
/*     sleep(5);
/* }
/*-----------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
|	Menu    MainMenu
|
|	These three routines shove ascii commands down the pipe as if
|	Rene had just typed them on the keyboard
|
+-------------------------------------------------------------------------*/

menU()
{
   DPRINT0("M: executing Menu\n");
   write(forgroundFdW, "menu", 4);
   sendChildNewLine();
#ifdef IRIX
   focusOfTty(1);
#endif
}

mainMenu()
{
   DPRINT0("M: executing Menu\n");
   write(forgroundFdW, "menu('main')", 12);
   sendChildNewLine();
#ifdef IRIX
   focusOfTty(1);
#endif
}

l_exit_vnmr()
{
   DPRINT0("M: executing Exit\n");
   write(forgroundFdW, "exit", 4);
   sendChildNewLine();
}

l_usermac1()
{
   DPRINT0("M: executing usermacro1\n");
   write(forgroundFdW, "usermacro1", 10);
   sendChildNewLine();
}

l_usermac2()
{
   DPRINT0("M: executing usermacro2\n");
   write(forgroundFdW, "usermacro2", 10);
   sendChildNewLine();
}

l_usermac3()
{
   DPRINT0("M: executing usermacro3\n");
   write(forgroundFdW, "usermacro3", 10);
   sendChildNewLine();
}

l_usermac4()
{
   DPRINT0("M: executing usermacro4\n");
   write(forgroundFdW, "usermacro4", 10);
   sendChildNewLine();
}

l_usermac5()
{
   DPRINT0("M: executing usermacro5\n");
   write(forgroundFdW, "usermacro5", 10);
   sendChildNewLine();
}

l_usermac6()
{
   DPRINT0("M: executing usermacro6\n");
   write(forgroundFdW, "usermacro6", 10);
   sendChildNewLine();
}

l_usermac7()
{
   DPRINT0("M: executing usermacro7\n");
   write(forgroundFdW, "usermacro7", 10);
   sendChildNewLine();
}

l_usermac8()
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
Flip()
{
   write(forgroundFdW, "flip", 4);
   sendChildNewLine();
#ifdef IRIX
   focusOfTty(1);
#endif
}

/*--------------------------------------------------------------------------
|	Help
|
|	This routine sends the help command to Vnmr.
|
+-------------------------------------------------------------------------*/
Help()
{
   write(forgroundFdW, "help", 4);
   sendChildNewLine();
#ifdef IRIX
   focusOfTty(1);
#endif
}


suicide()
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
addChar(c, buf, cp, bp)
char            c;
char           *buf;
int             cp,
                bp;
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
delChar(buf, cp, bp)
char           *buf;
int             cp,
                bp;
{
   int             ip;

   for (ip = cp; ip < bp; ip++)
      buf[ip] = buf[ip + 1];
}


/*------------------------------------------------------------------------------
|
|	Startup a background job (if possible).
|
+-----------------------------------------------------------------------------*/

static int 
startTclBackground(filename,addr,fname)
char           *filename;
char           *addr;
char           *fname;
{
      int Pid;
      sigset_t omask, qmask;
      DPRINT0("M: Background \n");
      if ((Pid = fork()) < 0)
      {
	 showError("Could not fork background process!");
      }
      else if (Pid)
      {
	 DPRINT1("M: ...background pid= %u\n", Pid);
      }
      else
      {
         int i;
         char cmd[256];
	 for (i = 3; i < 20; ++i)
	    close(i);
         sigemptyset( &qmask );
         sigaddset( &qmask, SIGUSR1 );
         sigaddset( &qmask, SIGUSR2 );
         sigprocmask( SIG_BLOCK, &qmask, &omask );
         sprintf(cmd,"%s/tcl/bin/vnmrwish",systemdir);
         execlp(cmd, "vnmrWish", filename, addr, fname, 0);
	 sleep(5);
	 exit(1);
      }
      return(Pid);
}

/*------------------------------------------------------------------------------
|
|	Startup the forground job (must work!).
|
+-----------------------------------------------------------------------------*/

startForground(name)
char           *name;
{
   int             fdR[2];
   int             fdW[2];

   DPRINT1("M: Starting forground job \"%s\"\n", name);
   if (0 <= pipe(fdR))
      if (0 <= pipe(fdW))
      {
	 DPRINT4("pipes fdR 0=%d 1=%d fdW 0=%d 1=%d\n",
		 fdR[0], fdR[1], fdW[0], fdW[1]);
	 if ((forgroundPid = fork()) < 0)
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
	 else			/* were in child */
	 {
	    char            pid[12];
	    char            port[10];
	    int             i;

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
	       textFrameFd = 0;
	    }
	    else
		textFrameFd = i;
#endif
	    for (i = 5; i < 30; ++i)
	       close(i);
	    {
	       char            masterWidth[20];
	       char            masterHeight[20];
	       char            addr[257];
	       char            textHeight[20];
               char            font_name[122];
               char            resize_width[6];

	       /* Pass the heigth and width of master screen */
	       sprintf(masterWidth, "-S%d", masterWindowWidth2);
	       sprintf(masterHeight, "-H%d", masterWindowHeight2);
	       sprintf(textHeight, "-K%d", textWindowHeight);
	       if (def_font)
                   sprintf(font_name, "-Z%s", fontName);
	       else
                   sprintf(font_name, "-Z");
	       sprintf(resize_width, "-B%d", resizeBorderWidth);
               strcpy(addr,"-h");
               GET_VNMR_ADDR(&addr[2]);

#ifdef DEBUG
	       if (debug)
	       {
		  if (strlen(Xserver) > 0)
                    execlp(name, "Vnmr", "-t", "-mforeground", addr, masterWidth, masterHeight, textFrameFds,mainFrameFds,buttonWinFds,textHeight, resize_width, Xdisplay, Xserver, font_name,0);
		  else
                    execlp(name, "Vnmr", "-t", "-mforeground", addr, masterWidth, masterHeight, textFrameFds,mainFrameFds,buttonWinFds,textHeight, resize_width, font_name,0);
	       }
	       else
#endif DEBUG
	       {
		  if (strlen(Xserver) > 0)
                     execlp(name, "Vnmr", "-mforeground", addr, masterWidth, masterHeight, textFrameFds,mainFrameFds,buttonWinFds, textHeight, resize_width, Xdisplay, Xserver,font_name, 0);
		  else
                     execlp(name, "Vnmr", "-mforeground", addr, masterWidth, masterHeight, textFrameFds,mainFrameFds,buttonWinFds, textHeight, resize_width,font_name, 0);
	       }
	       fprintf(stderr, "master: execl(%s,Vnmr,0) failed!\n", name);
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



master(argc, argv)
int             argc;
char           *argv[];

{
   char           *fgName;
   char           *p;
   int             i;

   setbuf(stderr, NULL);

   Xserver[0] = '\0';
   Xdisplay[0] = '\0';
   fontName[0] = '\0';

   setupMasterStatusFields();

/*  Define graphics so window routines will not crash...  */

   textTypeIsTcl = NULL;
   graphics = (char *)getenv("graphics");
   if (graphics == NULL)
      graphics = &default_graphics[0];
   if ( !strcmp(graphics,"sun") )
   {
      maxbuttonchars = 128;
      buttonspacechars = 0;
   }
   fgName = NULL;

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

	 while (*++p)
	 {
	    switch (*p)
	    {
	       case 'd':
		  debug += 1;
		  break;
	       default:
		  fprintf(stderr, "master: unknown option -%c\n", *p);
		  exit(1);
	    }
	 }
      }
      else if (fgName == NULL)
	 fgName = p;
      else
      {
	 fprintf(stderr, "master: too many names\n");
	 exit(1);
      }
   }

#else
    fgName = "Vnmr";
#endif

   if (fgName)
   {
      char *dgtext;

      close(1);			/* we never use stdout */
      /* create the Acqisition Process socket (return port number) */
      INIT_VNMR_COMM(HostName);	/* To receive stuff from acq process */

      setupdirs("master");

      dgtext = (char *)getenv("vnmrtext");
      if ( (dgtext != NULL) && (strlen(dgtext) > 1) )
      {
         if (access(dgtext,R_OK) == 0)
         {
         char addr[257];
         extern void killBG();

         GET_VNMR_ADDR(addr);
         getTclInfoFileName(TclFile,addr);
         strcat(TclFile,"_text");
         if ((textTypeIsTcl = fopen(TclFile, "w")) != NULL)
         {
            TclPid = startTclBackground(dgtext,addr,TclFile);
            register_child_exit_func(SIGCHLD, TclPid, NULL, killBG);
         }
         }
      }
      createWindows2((textTypeIsTcl != NULL));
      INIT_ACQ_COMM(systemdir);/* To send stuff to acq process */
      readyCatchers();
      DPRINT1("M: startForground for %s \n", fgName);
      startForground(fgName);
      DPRINT0("M: Into the notifier loop...\n");
      MasterLoop();
      DPRINT0("M: ...the notifier loop has exited!\n");
      exit(0);
   }
   else
   {
      fprintf(stderr, "master: No forground requested\n");
      exit(1);
   }
}
