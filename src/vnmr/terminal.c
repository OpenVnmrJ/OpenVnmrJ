/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------------------------------------
|	terminal.c
|
|	This module contains most of the code to handle  
|	the graphon terminal interface. It processes characters 
|	sent from the graphon and determines mouse click  
|	position and simulated buttons pushed. It also  
|	contains code to set raw and cooked modes of the 
|	terminal. 
|
|       Major change to terminal_main_loop and doFunction.
|       The latter routine now receives the index into the
|       escape sequence table, rather than a character as
|       its argument.  The terminal_main_loop is now table-
|       driven.  No change in the user-interface is intended.
|
|	Major change introduces capability of querying the
|	system to determine if any characters have been typed
|	without blocking the process or reading more than one
|	unsolicited character.  This is necessary to emulate
|	the mouse in the host computer software.  Related
|	change introduces a routine getOneChar(), to be used
|	as the standard terminal input routine.  This routine
|	will move the emulated mouse icon if the user pauses
|	while typing.  No change in the interface is intended
|	for the GraphOn terminal.
+---------------------------------------------------------------------------*/
#include "vnmrsys.h"
#include "wjunk.h"
#include "buttons.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>				/* Need value of EWOULDBLOCK */
#include <fcntl.h>
#ifdef LINUX
#include <sys/ioctl.h>
#endif
#include <unistd.h>
#include <termios.h>
#include <setjmp.h>

#define BOTTOM_LABEL_PIXEL	740
#define BUTTONS			0
#define GRAPHON_COL_WIDTH	13
#define GRAPHON_ROW_HEIGHT	30
#define MAX_NUM_BUTS 		8
#define MOUSE			1
#define TOP_ELEMENT_WINDOW	625

extern int           Tflag;
extern int           Aflag;
extern char 	     vnMode[];
extern int           tekCurSor;
extern int           curSor;
extern int	     terminalElementActive;
extern int	     hasMouse;
extern int	     icon_moving;
extern int           interuption;
extern int           working;
jmp_buf		     jmpEnvironment;
static int	     button_pushed;  /* panel button */
extern int (*funcs[])();
extern void nmr_exit(char *modeptr);
extern void Wsetcommand();
extern int Wputchar(char c);
extern void Wsetscroll();
extern void Wclearerr();
extern int loadAndExec(char *buffer);
extern void autoRedisplay();


/*  Start of variable definitions specific to either UNIX or VMS */

static int		stdinFd = -1;	/* File descriptor for stdin */
static struct termios   tbufsave;

static int		blockf;
static int		nblokf;
static char		cbuf = '\0';

#ifdef OLDCODE
static void processGraphOnCursor(char *s);
static int processTekCursor(char *s );
static void decodeTekXY(int rptarr[], int *xptr, int *yptr );
static int processSimulCursor(int knum );
static void processTekElementFiles(int x, int y );
#endif
static void doFunction(int c);

/*  The UNIX and VMS versions of initTerminal are completely different  */

void initTerminal()
{

/*  This routine is designed to exit if stdin has been redirected 
    to a non-tty device, for example, a disk file.		*/

	int		tflags;
        struct termios  tbuf;

	if (-1 < stdinFd) return;		/* Escape if already called */
	stdinFd = fileno( stdin );
	if (stdinFd < 0) {
		perror( "error getting file descriptor for stdin" );
		exit(0);
	}

/*  Get terminal characteristics with ioctl  */

        if (tcgetattr(stdinFd, &tbuf)) {
		perror( "Error reading terminal characteristics" );
		exit(0);
	}
	tbufsave = tbuf;

/*  Get "file characteristics" of the controlling terminal with fcntl  */

	if ((tflags = fcntl( stdinFd, F_GETFL, 0 )) == -1) {
		perror( "fcntl error with F_GETFL" );
		exit(0);
	}
	blockf = tflags & ~O_NDELAY;
	nblokf = tflags | O_NDELAY;
}

/*  query() returns TRUE if a Character is waiting; FALSE if not.
    Note that input can get at most one character ahead, that is,
    at most one character can be read by calling query() repeatedly.  
    For this reason, it is best to read input with get1char(), defined
    below, if you have called query() previously.			*/

int query()
{
	int		ival;
	extern int	errno;			/* NOT DEFINED IN errno.h!!  */

/*  initTerminal() must have been called previously...  */

	if (stdinFd < 0) {
		printf( "Programmer error, initTerminal not called\n" );
		exit(0);
	}

/*  Check if a previous call to query() already read a character.	*/

	if (cbuf != '\0')
	  return( 131071 );

/*  Set the file descriptor associated with stdin to be non-blocking,
    then try to read a character.  If the operation returns EWOULDBLOCK
    as an error, then no character is waiting.  Before returning, set
    the terminal back sa as to block the process until a character is
    typed if a read is performed.  This is done because if the current
    process exits with the terminal left non-blocking, UNIX will log
    the user out!!							*/

	if (fcntl( stdinFd, F_SETFL, nblokf ) != 0)
	  perror( "fcntl failed to set terminal to non-blocking" );
	ival = read( stdinFd, &cbuf, 1 );
	if (fcntl( stdinFd, F_SETFL, blockf ) != 0)
	  perror( "fcntl failed to set terminal to block" );

	if (ival < 0) {
		if (errno == EWOULDBLOCK)
		  return( 0 );
		perror( "read failure on stdin in query" );
		exit(0);
	}
	else if (ival == 0)
	  return( 0 );
	else
	  return( 131071 );
}

int get1char()
{
	char	c;
	int	ival;

/*  initTerminal() must have been called previously...  */

	if (stdinFd < 0) {
		printf( "Programmer error, initTerminal not called\n" );
		exit(0);
	}

	if (cbuf != '\0') {
		c = cbuf;
		cbuf = '\0';
		return( c & 0377 );
	}

/*  This routine expects the terminal to be configured to block
    the process until a character is available.			*/

	ival = read( stdinFd, &c, 1 );
	if (ival < 0) {
		perror( "read failure on stdin in get1char()" );
		exit(0);
	}
	else if (ival == 0)
	  return( -1 );				/* EOF */
	else
	  return( c & 0377 );
}

void resetButtonPushed()
{   button_pushed = 0;
}

/*-----------------------------------------------------------------------------
|
|	setInputRaw   sets the input stty as raw, no echoing, no kill
|	no nothing.  The program is responsible for everything.  Note
|	we must restore terminal or else it will be lost.
|
|	restoreInput  restores terminal back to original state
|
|----------------------------------------------------------------------------*/
void
setInputRaw()
{
    struct termios tbuf;

    if (!Wissun() && Tflag)
	fprintf(stderr,"setting raw input tty\n");
    if (!Wissun())
    {
        if (tcgetattr(0, &tbuf) == -1)
            perror("ioctl");
        tbufsave = tbuf;
        tbuf.c_lflag  &=  ~(ECHO | ICANON); /* turn off echo */
        tbuf.c_cc[4] = 1;   /* MIN  */
        tbuf.c_cc[5] = 0;   /* TIMER */
        if (tcsetattr(0, TCSANOW,  &tbuf) == -1)
            perror("ioctl2");
    }
}

void restoreInput()
{
    if (!Wissun() && Tflag)
	fprintf(stderr,"restoring input tty\n");
    if (!Wissun()) 
        if (tcsetattr(0, TCSANOW,  &tbufsave) == -1)
	    perror("ioctl3");
}

#define BACKSPACE       '\010'  /***/
#define CONTROL_C       '\003'  /***/
#define CONTROL_D       '\004'  /***/
#define DELETE          '\177'  /***/
#define ESC		'\033'
#define NEWLINE         10
#define CONTROL_M	13

/*  This table stores the escape sequences recognized by VNMR.
    More can be added TO THE END.  Try to keep the number of
    sequences less than 32, as you will find it much more
    complex to keep track of which sequences are still active.  */

static char *seq_table[] = {
        "1",				/*  Function Key 1 */
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",				/*  Function Key 8 */
        "9",				/*  GraphOn Cursor Report */
        "[9",				/*    "       "      "    */
	"a",
	"b",
	"c",
	"d",
	"q",				/*  Left mouse key on TEK   */
	"w",				/*  Middle mouse key on TEK */
	"e",				/*  Right mouse key on TEK  */
	"[A",				/*  Up-arrow    */
	"[B",				/*  Down-arrow  */
	"[C",				/*  Right-arrow */
	"[D",				/*  Left-arrow  */
	NULL
};

/*  Specify some indices into the table as parameters.  Note that
    one has been added to the C-style index.			*/

#define  GRAPHON_CR1	9		/*  Escape 9  */
#define  GRAPHON_CR2	10		/*  Escape [9  */
#define  ESCAPE_A	11		/*  doFunction() expects these to */
#define  ESCAPE_B	12		/*  be contigious and in order with */
#define  ESCAPE_C	13		/*  ESCAPE_A first and ESCAPE_D */
#define  ESCAPE_D	14		/*  last.  */
#define  LEFT_TEK_MKEY	15
#define  MID_TEK_MKEY	16
#define  RIGHT_TEK_MKEY	17
#define  UP_ARROW	18
#define  LEFT_ARROW	21

/*  Suggestion for future improvement.  If an initialization routine
    is ever required, have it define init_mask dynamically by counting
    the number of non-NULL addresses in the sequence table.	*/

static int	init_mask = 0x1fffff;	/* Initial mask; do not change.  */
static int	cur_mask = 0;		/* Mask of those currently active */
static int	esc_seen = 0;		/* Set when an escape is received */
static int	seq_started = 0;	/* Set when one character is matched */
static int	cur_index;		/* Index into the escape sequences;
					   equal to number of characters
					   matched minus one		*/
static char oldbuffer[1024];

/*  Why this function to read a character?  If the Mouse Icon is
    emulated in software, we want to update its position on the
    screen if the user pauses for a moment while typing.	*/

int getOneChar()
{
#ifdef OLDCODE
	if (icon_moving)
	 if (query()) return( get1char() );
	 else
	 {
		Gmove_icon();
		icon_moving = 0;	/*  Fall through  */
	 }
#endif
	return( get1char() );
}

/*-----------------------------------------------------------------------------
|
|	terminal_main_loop  is the equivalent to window_main_loop for
|	tty terminals.   It loops around waiting for a nice buffer of 
|	character to send to the parser.  It also looks for function
|	key sequences.	
|
|----------------------------------------------------------------------------*/

void terminal_main_loop()
{  static char buffer[1024];
   static int  bp = 0;
   int cur_char, seq_index;
   char *cur_seq;

    if ( Tflag)
	fprintf(stderr,"terminal_main_loop:starting\n");
    oldbuffer[0] = '\0';
    setInputRaw();  /* set input to raw */
    Wsetcommand(); /* activate command window */
    Wputchar('>');  /* Give the input prompt */
    while(1)
    {
	cur_char = getOneChar();	
	if (Aflag) Wscrprintf( "Got %d  %x\n", cur_char, cur_char );

/*  If we haven't seen an escape sequence, then check for current
    character being the escape character.  Specify that an escape
    sequence hasn't started either.					*/

	cur_seq = NULL;				/*  Indicate no match */
	if ( !esc_seen )
	{
	    esc_seen = cur_char == ESC;
	    seq_started = 0;
	}

/*  Otherwise, we HAVE seen the escape character.  Now the question is
    whether an escape sequence has started.  If not, reset the mask of
    currently valid escape sequences to include all defined.		*/

	else 
	{
	    if ( !seq_started )
	    {
		cur_mask = init_mask;
		seq_index = 0;
		cur_index = 0;
		seq_started = 131071;
	    }

/*  If the current character matches the current character in an escape
    sequence, then check the next character, for if it is NUL, then we
    have matched an escape sequence.

    Do not check a sequence unless its bit is still set in the mask.  */

	    seq_index = 0;
	    while ( (cur_seq = seq_table[ seq_index ]) != NULL )
	    {
		if ( (1 << seq_index) & cur_mask )
                {
		 if (cur_char != *(cur_seq+cur_index))
		  cur_mask &= ~(1 << seq_index);
		 else if (*(cur_seq+cur_index+1) == '\0')
		  break;
		 else ;
                }
		seq_index++;
	    }

/*  Report if a character doesn't match any escape sequences.	*/

	    if (cur_mask == 0)
	    {
		esc_seen = 0;
		seq_started = 0;
		Werrprintf( "'%c'    No such function key", cur_char );
		continue;
	    }
	    else
	     cur_index++;
	}

/*  Do we have a match?? */

	if (cur_seq != NULL)
	{
	    esc_seen = 0;
	    seq_started = 0;
	    doFunction( seq_index+1 );
	}

/*  Do not store the Character (or otherwise process it) if it
    could be part of an escape sequence.			*/

	else if ( !esc_seen )
	{
#ifdef OLDCODE
	    if (icon_moving) {
		Gmove_icon();
		icon_moving = 0;
	    }
#endif
	    switch (cur_char)
	    {
	  	case NEWLINE:
	  	case CONTROL_M:  if (1 <Tflag)
	        	fprintf(stderr,"main_loop: got Return\n"); 
  			buffer[bp++] = '\n';
			buffer[bp++] = '\0';
			Wputchar('\n');                        
			Wsetscroll(); /* activate scroll window */
                        if (strncmp(buffer,"!!",2)== 0) {
#ifdef OLDCODE
                            if (Wistek42xx()) {
                                disableTekGin();
                                tekCurSor = 0;
                            }
#endif
		    	    loadAndExec(oldbuffer);
#ifdef OLDCODE
                            if (Wistek42xx()) {
                                tekCurSor = 1;
                                if (curSor) enableTekGin();
                            }
#endif
                        }
			else {
#ifdef OLDCODE
                            if (Wistek42xx()) {
                                disableTekGin();
                                tekCurSor = 0;
                            }
#endif
			    loadAndExec(buffer);
#ifdef OLDCODE
                            if (Wistek42xx()) {
                                tekCurSor = 1;
                                if (curSor) enableTekGin();
                            }
#endif
			    strcpy(oldbuffer,buffer);
			}
			Wsetcommand(); /* activate command window */;
			Wputchar('>');  /* Give the input prompt */
			bp = 0;
			break;
	  case BACKSPACE:  
	  case DELETE:  if (bp > 0)     /* Modified so as to not */
			{		/* overwrite the prompt  */
			    bp--;  
                            Wputchar(BACKSPACE);
                            Wputchar(' ');
                            Wputchar(BACKSPACE); 
			}
			break;  
          case EOF:
	  case CONTROL_D: nmr_exit(vnMode);
			break;

	  default:	if (1 < Tflag)
			    fprintf(stderr,"terminal_mail_loop: got %c\n",
					cur_char);
			buffer[bp++] = cur_char;
                        Wputchar(cur_char); 
			break;
	    }
	}
    }
}
    
/*-------------------------------------------------------------------------
|	executeFunction/1
|
|	This routine takes the function number gotton when the function
|	key has been pushed or mouse has been clicked on the label of
|	the graphon, and attemps to execute the function associated with
|	it.
|
+------------------------------------------------------------------------*/

void executeFunction(int num)
{

    if (Tflag)
	fprintf(stderr,"executeFunction: executing function %d\n",num);
    if (funcs[num]) /* if a function really exists */
    {	Wclearerr(); /* clear error line */
	if (setjmp(jmpEnvironment) == 0)
	{   if (1 < Tflag) 
		fprintf(stderr,"executeFunction:saving environment\n");
	    working = 1;
	    (*funcs[num])(num+1); /* execute it */
	    autoRedisplay(); /* check if redisplay of anything needed */
	    Buttons_off(); /* Turn buttons off if necessary */
	    working = 0;
	    interuption = 0;
	    Wsetcommand();
	    if (1 < Tflag) 
		fprintf(stderr,"executeFunction: Back again (normally)\n");
	}
	else
	{  if (1 < Tflag) 
		fprintf(stderr,"executeFunction: Back at square one!\n");
	    Buttons_off(); /* Turn buttons off if necessary */
	    working     = 0;
	    interuption = 0;
	    Wsetcommand();
	}
    }
    else
    {	Wclearerr(); /* clear error line */
	Werrprintf("Function %d is not defined",num+1);
	Buttons_off(); /* Turn buttons off if necessary */
    }
}

/*  Completely rewritten.  Argument is the index (plus 1) into the
    escape sequence table SEQ_TABLE defined above.			*/

static void doFunction(int c)
{
#ifdef OLDCODE
    char curSeq[14];
    int curSeqptr;
#endif
    int num;

    if (Tflag)
	fprintf(stderr,"doFunction: sequence %d\n",c);

#ifdef OLDCODE
    if ( !hasMouse )
     if (UP_ARROW <= c && c <= LEFT_ARROW) {
	shiftIconPos( c-UP_ARROW );
	return;
     }
     else if (icon_moving) {
	Gmove_icon();
	icon_moving = 0;
     }

    /* if cursor is on, check for graphon cursor sequence */
    if (curSor && (c == GRAPHON_CR1 || c == GRAPHON_CR2))
    {   curSeqptr = 0;
     	while ((c = get1char()) != '\n' && c != 'v')
	{   curSeq[curSeqptr++] = c;
	}
        curSeq[curSeqptr] = '\0';
	processGraphOnCursor(curSeq);
	/* processCursor(curSeq+1); */
        return;
    }
    else if (LEFT_TEK_MKEY <= c && c <= RIGHT_TEK_MKEY)
     if (curSor)
      if (Wistek42xx())
      {
	curSeqptr = 0;
	curSeq[curSeqptr++] = c;	/* Tells which key was pressed */

/*  The next five characters make up the GIN report.  */

     	while (curSeqptr < 6)
	{
            c = get1char();
            if (Aflag) Wscrprintf( "Got %d %d (xy report)\n", c, c );
            curSeq[curSeqptr++] = c;
	}
        curSeq[curSeqptr] = '\0';
	processTekCursor(curSeq);
        return;
      }
      else if ( !hasMouse )
      {
	processSimulCursor( c - LEFT_TEK_MKEY );
	return;
      }
#endif

    /* check if we have function key 1-8 or a-d */

    if (c > 0 && c < 9)
    {
	num = c-1;
	executeFunction(num);
    }
    else if (c >= ESCAPE_A && c <= ESCAPE_D)
    {
	    switch (c)
	    {  case ESCAPE_A: /* Exit somehow */
		    execString("exit\n");
		    break;
	       case ESCAPE_B:  /* not used */
		    break;
	       case ESCAPE_C: /* menu */
		    execString("menu\n");
		    break;
	       case ESCAPE_D: /* menu ('main') */
		    execString("menu('main')\n");
		    break;
	    }
	    Wsetcommand();
    }
    else
    { 
     	   Werrprintf("'%d'  No such function key",c);
    }
}	

#ifdef OLDCODE
static void processGraphOnCursor(char *s)
{   char        *ptr;
    char        *xp;
    char        *yp;
    extern int (*mouseButton)();
    extern int (*mouseMove)();
    extern int ElementBotHead;
    extern int ElementTopHead;
    int          butNum;
    static int   butNumPushed;  /* mouse button */
    static int   graphonButtonDown = 0;
    int          x;
    int          y;

    if ( 2<=Tflag)
	 fprintf(stderr,"processGraphOnCursor: got \"%s\"\n",s);
    xp = ptr = &s[1];
    while (*ptr != ';' && *ptr)
	ptr++;
    *ptr++= '\0';
    yp = ptr;
    while (*ptr != ';' && *ptr)
	ptr++;
    if (!*ptr)
    {	butNum = 0;
    }
    else
    {	*ptr++= '\0';
	butNumPushed = butNum = *ptr - '0';
	switch (butNumPushed)
	{  case 4:  butNumPushed = 0;
		break;
	   case 2:  butNumPushed = 1;
		break;
           case 1:  butNumPushed = 2;
		break;
           default: fprintf(stderr,"processGraphOnCursor: bad button pushed %d\n",butNumPushed);
		break;
        }
    }

    x = atoi(xp);
    y = atoi(yp);
    if (Tflag)
      fprintf(stderr,"processCursor:x=%d y=%d MousebutNum = %d\n",x,y,butNum);
    if (y > BOTTOM_LABEL_PIXEL) /* in button area? */
    {  if ( button_pushed = getButtonPushed(x,y))
         if (butNum == 0)  /* button going down */
         {  inVert(button_pushed); 
	    executeFunction(button_pushed-1);
         }
         if (Tflag)
           fprintf(stderr,"processCursor:In buttons,%d pushd\n",button_pushed);
         if (button_pushed) /* Lets uninvert button after button is finshed */
            if (butNum == 0)  /* button coming up */
               reInVert(button_pushed);
    }
    else /* are we trying to select an element ?  */
      if (terminalElementActive && y <= TOP_ELEMENT_WINDOW)  
      {   int cCol;
          int cRow;

          if (!graphonButtonDown)
	  {  cRow = (TOP_ELEMENT_WINDOW - y)/GRAPHON_ROW_HEIGHT;
	     cCol = x/GRAPHON_COL_WIDTH;
	     if (Tflag)
                Wscrprintf("Button going down at x=%d  y=%d line %d char %d\n",
	           x,y,cRow,cCol);
	     graphonButtonDown = 1;
	     if (ElementTopHead) /* check if we are at top header */
	        if (cRow == 0)
		{  DisplayElementScreen(-1);
		   return;
                }
	     if (ElementBotHead) /* check if we are at bottom header */
	        if (cRow == 19  || cRow == 20)
                {  DisplayElementScreen(1);
		   return;
                }
	     /* call routine to figure out what idem we pushed and 
		set it */
	     setItNow(x,y,cRow,cCol);

          }
	  else
	  {  if (Tflag)
                Wscrprintf("Button coming up at x=%d  y=%d line %d char %d\n",
	         x,y,(TOP_ELEMENT_WINDOW - y)/GRAPHON_ROW_HEIGHT
	        ,x/GRAPHON_COL_WIDTH);
	     graphonButtonDown = 0;
          }
       }
    else /* No we are just hitting a mouse button */
    {  if (mouseButton) /* if button routine defined */
      {  (*mouseButton)(butNumPushed,butNum ? 0 : 1, x,y);
         Wsetcommand(); /* activate command window */
      }
      else   /* if button routine not defined, check if mouseMove defined */
         if (mouseMove) 
            (*mouseMove)(x,y);
    }
}

#define TEK_LABEL_BOT	351
#define TEK_LABEL_TOP	359
#define MAX_TEK_REPORT	5		/*  Maximum number of characters */
					/*  in X, Y position report  */
static int processTekCursor(char *s )
{
	int		butNum, iter, report[ 6 ], x, y;
	extern int	(*mouseButton)();
	extern int	(*mouseMove)();

/*  Determine which mouse button was pushed.  */

	switch ( *s )
	{
	 case LEFT_TEK_MKEY:
		butNum = 0;
		break;
	 case MID_TEK_MKEY:
		butNum = 1;
		break;
	 case RIGHT_TEK_MKEY:
		butNum = 2;
		break;
	 default:
		ABORT;
	}

	for (iter = 0; iter < MAX_TEK_REPORT; iter++)
	 report[ iter ] = 32;
	iter = 0;
	while ( iter < MAX_TEK_REPORT )
	{
		report[ iter ] = *(s+iter+1);
		iter++;
	}
	decodeTekXY( &report[ 0 ], &x, &y );

/*  Buttons are on the top line of the alphanumeric display.  You can
    move the cursor off the top of the screen, in which case the Y
    value will be too large.  This situation causes the operation to
    be aborted.  Note that BUTTON_PUSHED is a static variable, and the
    value is modified by resetButtonPushed(), above.

    By scaling the GIN report to the pixel space of the 05, we require
    only one set of constants to define the label area.  However, the
    X coordinate must be rescaled if the program is driving an 07.	*/

	if (y > TEK_LABEL_BOT)
	{
		if (y > TEK_LABEL_TOP) ABORT;
		if (Wistek4x07()) x = (x*4)/3;
		if ( button_pushed = getButtonPushed(x, y) )
		{
			disableTekGin();
			tekCurSor = 0;
			inVert(button_pushed);
			executeFunction(button_pushed-1);
			if (button_pushed) reInVert(button_pushed);
			tekCurSor = 1;
			if (curSor) enableTekGin();
		}
	}

/*  The GraphOn sends TWO reports for each press/release (click) of
    its mouse.  Thus processGraphOnCursor normally gets called twice.
    At this time the Textronix is programmed to send only one report
    per click.  Thus the routines below must be called twice.		*/

	else {
		disableTekGin();
		tekCurSor = 0;
		if (Wistek4x07()) {			/* Convert to 07  */
			x = (x*4)/3;			/* pixel space if */
			y = (y*4)/3;			/* that's what we */
		}					/* have.  */
		if (mouseButton)
		{
			(*mouseButton)(butNum, 0, x, y);
			if (mouseButton) (*mouseButton)(butNum, 1, x, y);
			Wsetcommand();
		}
		else if (mouseMove)
		{
			(*mouseMove)(x,y);
			if (mouseMove) (*mouseMove)(x,y);
		}
		else if (terminalElementActive)
		{
			processTekElementFiles(x, y );
		}
		if (curSor) enableTekGin();
		tekCurSor = 1;
	}
        RETURN;
}

/*  Index of report parameters.  */

#define  HI_Y	0
#define  EXTRA	1
#define  LO_Y	2
#define  HI_X	3
#define  LO_X	4

/*  Decode the GIN report sent by the Tek 42xx series terminal.

    The initial X, Y coordinates represent vector or screen space,
    which range from 0 to 4095 in both axes on all screen sizes.
    This routine then scales them to the pixel space of the 05.
    Thus these values are not valid for the 07.			*/

static void decodeTekXY(int rptarr[], int *xptr, int *yptr )
{
	int	highy, extra, lowy, highx, lowx, xval, yval;

	highy = (rptarr[ HI_Y ] & 127) - 32;
	extra = (rptarr[ EXTRA ] & 127) - 32;
	lowy  = (rptarr[ LO_Y ] & 127) - 32;
	highx = (rptarr[ HI_X ] & 127) - 32;
	lowx  = (rptarr[ LO_X ] & 127) - 32;

	yval = (highy << 7) + (lowy << 2) + ((extra >> 2) & 3);
	xval = (highx << 7) + (lowx << 2) + (extra & 3);
	yval = ((yval*45) / 49) >> 3;			/* 49*8=392 */
	xval = (xval*15) >> 7;

	*xptr = xval;
	*yptr = yval;
}

static int processSimulCursor(int knum )
{
	int		butNum, labelTop, x, y;
	extern int	(*mouseButton)();
	extern int	(*mouseMove)();

	if (getSimulCursorPos( &x, &y ))
	  ABORT;
	if ( knum < 0 || 2 < knum )
	  ABORT;
	else
	  butNum = knum;

	if (Wistek4x05())
	  labelTop = 351;
	else if (Wistek4x07())
	  labelTop = 471;
	else
	  ABORT;			/*  No HDS for now, sorry.  */

/*  Please see the comments in processTekCursor, above.  */

	if (y > labelTop)
	{
		if ( button_pushed = getButtonPushed(x, y) )
		{
			inVert(button_pushed);
			executeFunction(button_pushed-1);
			if (button_pushed) reInVert(button_pushed);
		}
	}
	else if (mouseButton)
	{
		(*mouseButton)(butNum, 0, x, y);
		if (mouseButton) (*mouseButton)(butNum, 1, x, y);
		Wsetcommand();
	}
	else if (mouseMove)
	{
		(*mouseMove)(x,y);
		if (mouseMove) (*mouseMove)(x,y);
	}
	else if (terminalElementActive)
	{
		processTekElementFiles(x, y );
	}
        RETURN;
}

/*  4x07 version only !!!!  */

static void processTekElementFiles(int x, int y )
{
	int		cCol, cRow;
	extern int	ElementBotHead;
	extern int	ElementTopHead;

	cRow = (405 - y)/15;
	cCol = x / 8;

/*  Winfoprintf(
	"tek mouse at x= %d, y = %d, col = %d, row = %d", x, y, cCol, cRow
    );  */

/*  Previous or next screen selected ?  */

	if (ElementTopHead)
	  if (cRow == 0) {
		DisplayElementScreen( -1 );
		return;
	  }
	if (ElementBotHead)
	  if (cRow == 25 || cRow == 26) {
		DisplayElementScreen( 1 );
		return;
	  }

	setItNow(x, y, cRow, cCol );
}
#endif
