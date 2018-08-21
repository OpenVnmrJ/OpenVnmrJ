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
|	wjunk.c 
|
|	This module contains the windowing subroutines used to manipulate the
|	windows on the GraphOn terminal.  It also contain codes to write status
|	messages in the correct places in the status panel on both the Sun and
|	the graphon. It contains code to clear screens, codes to print error
|	string, positional strings,  strings in the scrolling area, clear parts
|	of strings, go into graphics or alpha mode on the Graphon, clear   
|	graphics, terminal window setup and restoral, etc...
|
+-----------------------------------------------------------------------------*/

#define BUTTONS	2
#define MOUSE	1
#define GRAPHONSIZE	19
#define TEK4105SIZE	23
#define TEK4107SIZE	25
#define SUNSIZE		16

#define NDIALOGLINES	2

#include "vnmrsys.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "graphics.h"
#include "node.h"
#include "stack.h"
#include "group.h"

#ifdef  DEBUG
extern int      Tflag;
#define TPRINT0(str) \
	if (Tflag) fprintf(stderr,str)
#define TPRINT1(str, arg1) \
	if (Tflag) fprintf(stderr,str,arg1)
#else   DEBUG
#define TPRINT0(str) 
#define TPRINT1(str, arg2) 
#endif  DEBUG

extern char   *graphics;
extern char   *newString();
extern char   *realString();
int     beepOn = 1;
extern int     Bnmr;
extern int     Mflag;
extern node   *doingNode;
extern int interuption;
void Wprintfpos(int, int, int, char *, ...);
void Wprintf(char *, ...);


char           screenDisplay[20] = "bad";/* temporary parameter */
char           textDisplay[20] = "";     /* keep track of text display */
char           graphicsDisplay[20] = ""; /* keep track of graphics display */
int            active = 3; 		 /* keep track of active window */
int	       graphicsOn = 0;
int	       hasMouse = 0;		 /* Has a hardware mouse if non-zero */

static int     error_line_dirty = 0;  /* error line starts out clean */
static int     MoreCount = 0;  /* more counter */
static int     status_line_one_dirty = 0;  /* status line starts out clean */
static char    status[9];

/*  VT-100 style terminals do not have separate scrolling windows.
    Thus the 3 windows of VNMR must be emulated in software.  The
    following data structure tracks essential information.	*/

#define  NUM_WINDOWS	4

static struct windowData {
	int	plane;
	int	firstLine;
	int	numLines;
	int	curLine;
	int	curCol;
} SoftWindow[ NUM_WINDOWS ];

static char	screenDbase[ 80*NDIALOGLINES ];

/*------------------------------------------------------------------
|	Status display procedures. 
|
|	These procedures tell the master to display status information 
|	in various fields on the sun status window or displays the
|	status information on the graphic terminal.
|       Some procedures are passed integer, other are passed ascii
|       strings. See description below for information.
|	The status line has been broken up into 9 fields. 
|
| LINE1	 1    2      3      4      5      6      7      8
|	1-5  8-11  14-29  32-39  42-52  55-62  65-72  75-79
|	
|	1. disp_print     - display 5 byte string in field 1
|	2. disp_plot	  - display 4 byte string in field 2
|	3. disp_seq	  - display Seq: and 12 byte string in field 4
|       4. disp_exp	  - display Exp:iiii   where 0 < iiii <= 9999
|	5. disp_specIndex - displays Index:iiii  where 0 < iiii <= 9999  
|	6. disp_acq       - display 8 byte acqproc message 
|	7. disp_status    - displays 8 byte status word
|	8. disp_index     - displays 4 byte index integer
|
|	All display routines now send output to stdout if process
|	is running in background (Bnmr non-zero).  Except for scrolling
|	routines, each appends a newline character.
+----------------------------------------------------------------------*/

/*------------------------------------------------------------------
|
|	makeSmallBut  makeLargeBut
|
|	This procedure sends a message to the master to make the
|       small/large button display small or large.
|
+----------------------------------------------------------------------*/
/***********/
makeSmallBut()
/***********/
{   
    if (!Bnmr)   /* if background, just ignore */
    {
#ifdef SUN
	if (Wissun())
	    sendCodeToMaster("s");  /* send to master */
#endif
    }
}

/***********/
makeLargeBut()
/***********/
{   
    if (!Bnmr)   /* if background, just ignore */
    {
#ifdef SUN
	if (Wissun())
	    sendCodeToMaster("l");  /* send to master */
#endif
    }
}

/***********/
set_datastation(val)
/***********/
int val;
{   
    if (!Bnmr)   /* if background, just ignore */
    {
#ifdef SUN
	if (Wissun())
        {
	    sendCodeToMaster( (val == 0) ? "D" : "d" );  /* send to master */
        }
#endif
    }
}

/*------------------------------------------------------------------
|
|	dispPrompt/1
|
|	This procedure displays a character string in the command line.
|	This is for use in entering variables. Lets limit it to 70
|	characters.
|
+----------------------------------------------------------------------*/
/***********/
dispPrompt(t)		char *t;
/***********/
/* display Prompt in the command window status characters */
{   char s[70];

    if (Bnmr) return;		/* Should not be called from background */

    strncpy(s,t,69);
    s[69] = 0;
    if (Bnmr) return;
#ifdef SUN
    if (Wissun())
      sendStatusToMaster(s,1);  /* send to master */
    else
#endif
      Wprintfpos(1,2,1,"%s",s);  /* send to terminal */
}

/*------------------------------------------------------------------
|
|	disp_print/1
|
|	This procedure displays 5 byte string line 1 1-5
|
+----------------------------------------------------------------------*/
/***********/
disp_print(t)		char *t;
/***********/
/* display the processing status characters */
{   char s[6];

    strncpy(s,t,5); /* copy only 5 characters */
    s[5] = '\0';    /* set last place to null */
    if (Bnmr)       /* send out stdout if background vnmr */
	printf("%s\n",s);
    else
#ifdef SUN
	if (Wissun())
	    sendStatusToMaster(s,1);  /* send to master */
	else
#endif
	    Wprintfpos(1,2,1,"%s",s);  /* send to terminal */
}

/*------------------------------------------------------------------
|
|	disp_plot/1
|
|	This procedure displays 4 byte string line 1 8-11
|
+----------------------------------------------------------------------*/
/***********/
disp_plot(t)		char *t;
/***********/
/* display the processing status characters */
{   char s[5];

    strncpy(s,t,4); /* copy only 4 characters */
    s[4] = '\0';    /* set last place to null */
    if (Bnmr)       /* send out stdout if background vnmr */
	printf("%s\n",s);
    else
#ifdef SUN
	if (Wissun())
	    sendStatusToMaster(s,2);  /* send to master */
	else
#endif
	    Wprintfpos(1,2,8,"%s",s);  /* send to terminal */
}

/*------------------------------------------------------------------
|
|	disp_seq/1
|
|	This procedure displays Seq:xxxxxxxx line 1 14-29
|
+----------------------------------------------------------------------*/
/***********/
disp_seq(t)		char *t;
/***********/
/* display a processing index, 0 clears field */
{   char s[20];
    int  l;

    if (t) {
	l = strlen( t );
	if (l <= 8)
	  sprintf(s,"Seq:%8s",t);
	else if (l > 12)
	  sprintf(s,"Seq:%*s",12,t);
	else 
	  sprintf(s,"Seq:%*s",l,t);
    }
    else
	sprintf(s,"            ");
    if (Bnmr)      /* send out stdout if background vnmr */
	printf("%s\n",s);
    else
#ifdef SUN
	if (Wissun())
	    sendStatusToMaster(s,3);  /* send to master */
	else
#endif
	    Wprintfpos(1,2,14,"%s",s);  /* send to terminal */
}

/*------------------------------------------------------------------
|
|	disp_exp/1
|
|	This procedure displays Exp:xxxx line 1 32-39
|
+----------------------------------------------------------------------*/
/***********/
disp_exp(i)		int i;
/***********/
/* display a processing index, 0 clears field */
{   char s[20];

    if ( (i > -1) && (i < 10000) )
    {
	sprintf(s,"Exp:%-4d",i);   /* use %-4d to left justify the exp number */
    }
    else
    {
	sprintf(s,"        ");
    }

    if (Bnmr)      /* send out stdout if background vnmr */
	printf("%s\n",s);
    else
#ifdef SUN
	if (Wissun())
	    sendStatusToMaster(s,4);  /* send to master */
	else
#endif
	    Wprintfpos(1,2,32,"%s",s);  /* send to terminal */
}

/*------------------------------------------------------------------
|
|	disp_planeIndex/0
|
|	This procedure displays Index2:xxxx  line 2  42-51
|       Note that this procedure is a static and only called via
|       disp_specIndex()
|
|       Moved back to start in col 42, so it lines up with the
|	spectrum index (disp_specIndex)   04/1998
+----------------------------------------------------------------------*/
/***********/
static disp_planeIndex(disp)
/***********/
/* display a plane index, 0 clears field */
int disp;
{   char s[20];
    int i;

    if (disp && (getnd() >= 3) && ((i = getplaneno()) >= 0))
	if (i)
           sprintf(s,"Index2:%4d",i);
        else
           sprintf(s,"Index2:proj");
    else
	sprintf(s,"           ");
    if (Bnmr)   /* send out stdout if background vnmr */
	printf("%s\n",s);
    else
#ifdef SUN
	if (Wissun())
	    sendStatusToMaster(s,9);  /* send to master */
	else
#endif
	    Wprintfpos(1,3,41,"%s",s);  /* send to terminal */
}

/*------------------------------------------------------------------
|
|	disp_specIndex/1
|
|	This procedure displays Index: xxxxx  line 1  42-52
|
+----------------------------------------------------------------------*/
/***********/
disp_specIndex(i)		int i;
/***********/
/* display a processing index, 0 clears field */
{   char s[20];

    if (i>9999)
	sprintf(s,"Index:%-5d",i);
    else if (i>0)
	sprintf(s,"Index: %-4d",i);
    else
	sprintf(s,"            ");
    if (Bnmr)   /* send out stdout if background vnmr */
	printf("%s\n",s);
    else
#ifdef SUN
	if (Wissun())
	    sendStatusToMaster(s,5);  /* send to master */
	else
#endif
	    Wprintfpos(1,2,41,"%s",s);  /* send to terminal */
    disp_planeIndex(i);
}

/*------------------------------------------------------------------
|
|	disp_acq/1
|
|	This procedure displays an 8 byte string line 1 55-62
|
+----------------------------------------------------------------------*/
/************/
disp_acq(t)		char *t;
/************/
/* display the processing status characters */
{   char s[9];

    strncpy(s,t,8);
    s[8] = 0;
    if (Bnmr)   /* send out stdout if background vnmr */
	printf("%s\n",s);
    else
#ifdef SUN
	if (Wissun())
	    sendStatusToMaster(s,6);  /* send to master */
	else
#endif
	    Wprintfpos(1,2,55,"%s",s);  /* send to terminal */
}

/*------------------------------------------------------------------
|
|	disp_status/1
|
|	This procedure displays an eight byte status word  line 1 65-72
|
+----------------------------------------------------------------------*/
/************/
disp_status(t)		char *t;
/************/
/* display the processing status characters */
{
    strncpy(status,t,8);
    status[8] = 0;
    if (Bnmr)   /* send out stdout if background vnmr */
    {
        if (status[0] != ' ')
	   printf("%s\n",status);
    }
    else
#ifdef SUN
	if (Wissun())
	    sendStatusToMaster(status,7);  /* send to master */
	else
#endif
	    Wprintfpos(1,2,65,"%s",status);  /* send to terminal */
}

/*************/
getstatus(argc,argv,retc,retv)
/*************/
int argc,retc;
char *argv[],*retv[];
{
    if (retc > 0) retv[0] = newString(status);
    RETURN;
}

/*------------------------------------------------------------------
|
|	disp_index/1
|
|	This procedure displays an 4 byte index integer line 1 75-79
|
+----------------------------------------------------------------------*/
/***********/
disp_index(i)		int i;
/***********/
/* display a processing index, 0 clears field */
{   char s[5];

    if (i)
	sprintf(s,"%5d",i);
    else
        sprintf(s,"    ");
    if (Bnmr)   /* send out stderr if background vnmr */
	printf("%s\n",s);
    else
#ifdef SUN
	if (Wissun())
	    sendStatusToMaster(s,8);  /* send to master */
	else
#endif
	    Wprintfpos(1,2,75,"%s",s);  /* send to terminal */
}

/*********************************************

graphicsOnTop()
{
#ifdef SUN
    extern int parentfd;
    extern int textWindowVisable;
    int        graphicsFrameFd;

    if (textWindowVisable)
    	if (Wissun())
	{   graphicsFrameFd = (int)window_get(Wframe,WIN_FD,0); 
	    wmgr_top(graphicsFrameFd,parentfd);
	    textWindowVisable = 0; 
	}
#endif
}
 
textOnTop()
{
#ifdef SUN
   extern int parentfd;

   if (!textWindowVisable)
      if (Wissun())
      {  wmgr_top(4,parentfd);
	 textWindowVisable = 1; 
      }
#endif
}
***********************************************/

static char  proc_name[32];
static int   (*dismiss_proc)() = NULL;

register_dismiss_proc(name, func)
char  *name;
int   (*func)();
{
        strcpy(proc_name, name);
        dismiss_proc = func;
}


/*------------------------------------------------------------------
|
|	Wsetgraphicsdisplay/1
|
|	This procedures stores the current graphics display
|
+----------------------------------------------------------------------*/

Wsetgraphicsdisplay(n)	char *n;
{   int i;
	
    if (dismiss_proc != NULL)
    {
        if (strcmp(proc_name, n) != 0)
        {
                dismiss_proc();
                dismiss_proc = NULL;
        }
    }

    for(i=0;i<20;i++)
	graphicsDisplay[i] = NULL;
    strncpy(graphicsDisplay,n,19);
    if (!Wissun())
    {
      for(i=0;i<20;i++)
	textDisplay[i] = NULL;
      strncpy(textDisplay,n,19);
    }
}

/*------------------------------------------------------------------
|
|	Wsettextdisplay/1
|
|	This procedures stores the current text display
|
+----------------------------------------------------------------------*/

Wsettextdisplay(n)	char *n;
{  int i;
   extern int printOn;
	
   if (!printOn)
   {
      for(i=0;i<20;i++)
	textDisplay[i] = NULL;
      strncpy(textDisplay,n,19);
      if (!Wissun())
      {
        for(i=0;i<20;i++)
	  graphicsDisplay[i] = NULL;
        strncpy(graphicsDisplay,n,19);
      }
   }
}

/*------------------------------------------------------------------
|
|	Wgetgraphicsdisplay/1
|
|	This procedures gets the current graphics display
|
+----------------------------------------------------------------------*/

char *
Wgetgraphicsdisplay(char *buf, int max1)
{   
    strncpy(buf,graphicsDisplay,max1);
    buf[max1-1] = NULL;
    return(buf);
}

/*------------------------------------------------------------------
|
|	Wgettextdisplay/1
|
|	This procedures gets the current text display
|
+----------------------------------------------------------------------*/

char *
Wgettextdisplay(buf,max1)	char *buf;  int max1;
{   
    strncpy(buf,textDisplay,max1);
    buf[max1-1] = NULL;
    return(buf);
}

/*------------------------------------------------------------------
|
|	WgraphicsdisplayValid/1
|
|	This procedures returns a 1 if graphics display is still on
|	the screen and can be updated.  It returns 0, if it is not
|
+----------------------------------------------------------------------*/

WgraphicsdisplayValid(n)	char *n;
{   if (strcmp(n,graphicsDisplay))
	return 0;
    else
	return 1;
}

/*------------------------------------------------------------------
|
|	WtextdisplayValid/1
|
|	This procedures returns a 1 if text display is still on
|	the screen and can be updated.  It returns 0, if it is not
|
+----------------------------------------------------------------------*/

WtextdisplayValid(n)	char *n;
{   if (strcmp(n,textDisplay))
	return 0;
    else
	return 1;
}

/*----------------------------------------------------------------------
|
|	Wisgraphon/0    Wishds/0       Wissun/0       Wistek/0
|	Wistek41xx/0    Wistek42xx/0   Wistek4x05/0   Wistek4x07/0
|
|	These procedures return a 1 if the terminal is set to the	
|	particular type (graphon, sun, hds, tek, or color sun). 
|
|       Distinction necessary between Tektronix model numbers and
|	also between Tektronix terminal series.  The former governs
|	the size of the graphics screen; the latter determines
|	whether a Mouse is present.
|
+----------------------------------------------------------------------*/

Wissun()
{
#ifdef SUN
    static int retval = -1;
    if ( retval == -1 )
       if (strcmp(graphics,"sun")==0 || strcmp(graphics,"suncolor")==0)
	  retval = 1;
       else
	  retval = 0;
    return retval;
#else
    return 0;
#endif
}

WisSunColor()
{
#ifdef SUN
    static int retval = -1;

    if ( retval == -1 )
       if (Wissun()) /* make sure its a sun before doing sun calls */
       {
           if (depthOfWindow() > 6)
	       retval = 1; /* yes it is */
	   else
	       retval = 0;
       }
       else
	  retval = 0;
    return retval;
#else
    TPRINT0("WisSunColor: terminal not a sun\n");
    return 0;
#endif
}

/*  Determine if terminal is a Tektronix model.   */

Wistek()
{
	static int retval = -1;
	if ( retval == -1 )
	   retval = ( strncmp( graphics, "tek", 3 ) == 0 );
	return retval;
}

Wistek41xx()
{
	static int retval = -1;
	if ( retval == -1 )
	   retval = ( strncmp( graphics, "tek41", 5 ) == 0 );
	return retval;
}

Wistek42xx()
{
	static int retval = -1;
	if ( retval == -1 )
	   retval = ( strncmp( graphics, "tek42", 5 ) == 0 );
	return retval;
}

Wistek4x05()
{
	static int retval = -1;
	if ( retval == -1 )
	{
	   if ( strncmp( graphics, "tek", 3) != 0 ) 
	      retval = 0;
	   else
	      retval = ( strncmp( graphics+5, "05", 2 ) == 0 );
	}
	return retval;
}

Wistek4x07()
{
	static int retval = -1;
	if ( retval == -1 )
	{
	   if ( strncmp( graphics, "tek", 3) != 0 ) 
	      retval = 0;
	   else
	      retval = ( strncmp( graphics+5, "07", 2 ) == 0 );
	}
	return retval;
}

Wisgraphon()
{
    static int retval = -1;
    if ( retval == -1 )
       if (strcmp(graphics,"graphon")==0)
	   retval = 1;
       else
	   retval = 0;
    return retval;
}

Wishds()
{   
    static int retval = -1;
    if ( retval == -1 )
       if (strcmp(graphics,"hds")==0)
	   retval = 1;
       else
	   retval = 0;
    return retval;
}

/*----------------------------------------------------------------------
|
|	Wsetstatus/0    Wsetcommand/0    Wsetscroll/0   Wsetgraphics/0
|
|	These procedures set the active window.  The setgraphics
|	command turns on 4014 graphics
|
+----------------------------------------------------------------------*/

Wsetstatus() /* set status screen (1) active */
{    Wscreen(1);
} 

Wsetcommand() /* set command screen (3) active */
{    Wscreen(3);
}

Wsetscroll() /* set scroll screen (4) active */
{    Wscreen(4);
}

Wsetgraphics() /* set graphics screen (2) active */
{    Wgmode();
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
Wresterm(mes)		char *mes;
{   if (Wisgraphon())
    {	killwindow(1);
	killwindow(3);
	killwindow(4);
	setwindow(1,1,1,24);
	Wsetactive(1);
	setwrap(); /* set autowrap on */
	setjump(); /* set jump scroll */
	fprintf(stderr,"%s\n",mes);
    }
    else if (Wishds())
    {   hds_setwindow(1,1,24); /* reset windows to window 1, 1-24 rows */
        printf("\033[=125h"); /* Blank out inactive display memory */
        printf("\033[3+|"); /* Turn off graphics */
        printf("\033[0+|"); /* Turn on alpha */
        Wclear(2); /* clear the screen */
        printf("\030"); /* go to alpha mode if necessary */
        fprintf(stderr,"%s\n",mes);
        Wclear(1);
    }
    else if (Wistek())
    {
        if (Wistek42xx())
        {
            printf( "\033%%!0" );			/*  Require TEK mode */
            printf( "\033IDD0" );			/*  Disable GIN */
            printf( "\033%%!1" );			/*  Back to ANSI mode */
        }
        Wsetactive(4);
        printf( "%s", mes );
    }
    else
        fprintf(stderr,"%s\n",mes);
}
   
/*----------------------------------------------------------------------
|
|	Wsetupterm/0
|
|	These procedures setup the environment for the terminal.
|
+----------------------------------------------------------------------*/

/* setup graphon terminal for vnmr program */
Wsetupterm()
{
     initTerminal();

     if (Wisgraphon())
           graphon();
     else if (Wishds())
           hds();
     else if (Wistek())
	   tek();
}

static setjump()
{   printf("\033[?4l"); /* turn off smooth scroll */
}

static setwrap()
{    printf("\033[?7h"); /* turn autowrap on */
}
    
static hds()
{    printf("\033[=125l"); /* Don't blank out inactive display memory */
     hds_setwindow(1,1,3); /* setup window 1  row 1-3 */
     printf("\033[2J"); /* clear alpha screen */
     hds_setwindow(4,6,24); /* setup window 4  row 6-24 */
     printf("\033[2J"); /* clear alpha screen */
     hds_setwindow(3,4,5); /* setup window 3  row 4-5 */
     printf("\033[2J"); /* clear alpha screen */
     printf("\033[3+|"); /* turn off graphic screen */

     hasMouse = 0;
}

static graphon()
/* setup window 1 1x3, window 3 4x2, window 4 6x20  */
{    killwindow(1);
     killwindow(3);
     killwindow(4);
     setwindow(1,1,1,3);
     setwindow(1,3,4,5);
     setwindow(1,4,6,26);
     Wsetactive(4);
     Wclear_graphics();
     setjump(); /* lets turn on jump scroll */
     setwrap(); /* lets set auto rap for this window */
     Wsetactive(3);

     hasMouse = 131071;
}

/* delete a window from the terminal */
static killwindow(window)   int window;
{   printf("\033[%d;1;0;629;0;0;0\"w",window); /* kill window  */
    fflush(stdout); /* Send this out right away */
}

/* create a window in the terminal */
/* setup window. plane=1 or 2, window 1-4, firstrow,lastrow 1-26
     Note: windows cannot overlap in planes  */
static setwindow(plane,window,firstrow,lastrow)
int plane,window,firstrow,lastrow;
{    
     printf("\033[%d;%d;1;%d;%d;0;0\"w",window,plane,
                779-(firstrow-1)*30,779-(lastrow-1)*30);
     fflush(stdout); /* Send this out right away */
}

/* setup window for hds */
static hds_setwindow(window,top,bot)
int window;
int top;
int bot;
{    Wsetactive(window);  /* select window first */
     printf("\033[%d;%d;1;80w",top,bot);
}

/*----------------------------------------------------------------------
|
|	Walpha/1
|
|	This procedure turns on alpha mode for the requested window.
|	It makes window active.  It will not work for window 2 since
|	window 2 is the graphics window.  If 0 is chosen, it attemps
|	to turn off graphics mode and make the previous window active
|
+----------------------------------------------------------------------*/

Walpha(window)
int window;
{    if(Wisgraphon())
     {   if(window)
         {    Wsetactive(window);
              dis_plane(0);
         }
         else
             dis_plane(0);
     }
     else if(Wishds())
     {    printf("\033[3+|"); /* Turn off graphics */
          Wsetactive(window);
     }
     else if(Wistek())
     {
	Wsetactive(window);		/* Turns graphics on if required */
     }
     if (window != 2)
	graphicsOn = 0;
}

Wsetactive(window)  /* selects active window, window=1-4 */
int window;    /* if window=2, graphics plane is turned on, alpha turned off */
{    
     if(window==2) /* turn on graphic window */
         Wgmode();
     else
     {    if(Wisgraphon())
          {   printf("\033[%dw",window); /* select window */
          }
          else if(Wishds())
          {   if(active == 2) /* if we were in graphics */
                 printf("\030"); /* goto alpha */
              printf("\033[%d;1!w",window); /* select the window */
              printf("\033[%d;9!w",window); /* move cursor to window */
          }

/*  For Tekronix, move the cursor to the current position in the
    specified window.  Future calls to printf, putchar, etc., then
    will display text appropriately.				*/

	  else if(Wistek())
	  {
	     if (active == 2)
	     {
		printf( "\033%%!1" );			/* Must be in ANSI */
		graphicsOn = 0;
	     }
	     printf("\033[%d;%dH",
		SoftWindow[ window-1 ].curLine,
		SoftWindow[ window-1 ].curCol
	     );
	  }
     }
     active = window; /* keep track of window */
     fflush(stdout); /* Send this out right away */
}

/* same as setactive, but does a fflush to send it out right away */

Wscreen(num)
int num;
{     Wsetactive(num);
      fflush(stdout); /* Send this out right away */
}

static dis_plane(option)
int option;
/* 0 - display only active window plane, 2 - display both planes */
{    printf("\033[%d#z",option);
     fflush(stdout);
}


Wend_graf() /* if visual, turn back alpha */
{   }

/*----------------------------------------------------------------------
|
|	Wshow_text/0
|
|	This procedure puts text in forground.  On the sun, it
|	makes sure that the text frame is on the front. On the terminals,
|	it just turns on alpha (without clearing graphics). IT DOES NOW!
|
+----------------------------------------------------------------------*/
Wshow_text()
{
   extern int   printOn;

   if (!printOn)
   {
    if (Wisgraphon())
    {	printf("\033[0#z");
	Walpha(4);
	Wclear(2);
	fflush(stdout);
    }
    else if(Wishds())
    {	printf("\033[3+|"); /* Turn off graphics */
	fflush(stdout); /* Send this out right away */
    }
    else if (Wistek())
    {
	Walpha(4);
	Wclear(2);
	fflush(stdout);
    }
    else if (Wissun())
	textOnTop();
    active = 4;
   }
}

/*----------------------------------------------------------------------
|
|	Wshow_graphics/0
|
|	This procedure puts graphics in forground.  On the sun, it
|	does nothing.  On the terminals, it just turns on graphics 
|	and clears the text screen.
|
+----------------------------------------------------------------------*/
Wshow_graphics()
{   if (Wisgraphon())
    {   
     	printf("\033[2#z");
	fflush(stdout);
        Wclear(4);
    }
    else if(Wishds())
    {	printf("\035");  /* switch to graphics mode */
	printf("\033[2+|"); /* Turn on graphics screen */
	printf("\033/1h"); /* non-scaled addressing */
	fflush(stdout);
    }
    else if(Wistek())
    {
        Wclear(4);
	if(active != 2)
	{
	    printf( "\033%%!0" );		/* TEK mode for graphics */
	    active = 2;
	}
    }
    /* if (Wissun())
	graphicsOnTop(); */
}

/*----------------------------------------------------------------------
|
|	Woverlap/0
|
|	This procedure turns on both graphics and alpha screens for
|	graphon terminal. It does nothing for hds since hds has a
|	different method for overlap.  It does not change active screen.
|
+----------------------------------------------------------------------*/

Woverlap()
{    if(Wisgraphon())
         dis_plane(2);
}

/*----------------------------------------------------------------------
|
|	WblankCanvas
|
|	If the terminal is a Sun, this procedure clears the screen
|       and sets the background to white.  If it is a graphon, this
|       routine clears both the graphics and scroll sections of
|       the screen.
|
+----------------------------------------------------------------------*/
WblankCanvas()
{
   if (Wisgraphon() || Wistek())
   {  Wclear(2);
      Wclear(4);
   }
#ifdef SUN
   else
   { 
      whiteCanvas();
   }
#endif SUN
}

/*----------------------------------------------------------------------
|
|	Wclear/1
|
|	This procedure clears requested window and leaves previous 
|	screen active.
|
+----------------------------------------------------------------------*/

/* clears window without changing active status */
Wclear(window)
int window;
{
    int 	iter;

    if (window < 1 || 5 < window)
	return;

    if(Wistek())
    {
    	if (window < 1 || 4 < window)
	    return;
	if(window != active)
	 if (window == 2) 	printf( "\033%%!0" );	/* Select TEK mode */
	 else if (active == 2)	printf( "\033%%!1" );	/* Select ANSI mode*/
	if (window == 2)
	 printf( "\033\014" );				/* G Erase */
	else
	{
	    window--;					/* Convert to index */
	    printf( "\033[%d;1H",
		SoftWindow[ window ].firstLine
	    );
	    if (window == 3 )				/* Scrolling window */
	     printf( "\033[0J" );
	    else
	     for (iter = 0; iter < SoftWindow[ window ].numLines; iter++)
	     {
		printf( "\033[2K" );
		if (iter < SoftWindow[ window ].numLines-1)
		 printf( "\033[%d;1H", SoftWindow[ window ].firstLine+iter+1 );
	     }
	    SoftWindow[ window ].curLine = SoftWindow[ window ].firstLine;
	    SoftWindow[ window ].curCol = 1;
	    window++;
	}
	if(window != active)
	 if (window == 2) 	printf( "\033%%!1" );	/* Back to ANSI */
	 else if (active == 2)	printf( "\033%%!0" );	/* Back to TEK */
    }
    else if (Wisgraphon())
    {   
	if (window < 1 || 4 < window)
	    return;
 	if(window != active)
            printf("\033[%dw",window); /* make window active temporarily */
        if(window!=2)
            printf("\033[2J");
        else
            printf("\033\014"); /* clear screen (ESC FF or ESC ctrl-L) */ 
        if(window != active)
            printf("\033[%dw",active); /* set previous window active */
        fflush(stdout); /* Send this out right away */
    }
    else if (Wishds())
    {   
	if (window < 1 || 4 < window)
	    return;
        if(window!=2)
        {   printf("\033[%d;0!w",window); /* make window active temporarily */
            printf("\033[2J");
            printf("\033[%d;0!w",active); /* make window active temporarily */
        }
        else
        {   printf("\033[1+z");
            if(active !=2)
                 printf("\030");
        }
        fflush(stdout);
    }
#ifdef SUN
    else if (Wissun())
    {	if (window == 2)
	    Wclear_graphics();
	else if (window == 4)
	    Wclear_text();
	else if (window == 5)
	    sendClearerrToMaster();
    }
#endif
}

/*----------------------------------------------------------------------
|
|	Wclear_text/0
|
|	This clears the text screen on the sun, graphon or hds terminal.
|
+----------------------------------------------------------------------*/
#define CNTR_L 	0x0c

Wclear_text()
{
   extern int printOn;

   if (!printOn)
   {
    if(Wisgraphon())
    {   if(4 != active)
            printf("\033[%dw",4); /* make window active temporarily */
	printf("\033[2J");
	printf("\033[H");
        if(4 != active)
            printf("\033[%dw",active); /* set previous window active */
        fflush(stdout); /* Send this out right away */
    }
    else if(Wishds())
    {   
        printf("\033[%d;0!w",4); /* make window active temporarily */
	printf("\033[2J");
	printf("\033[H");
	printf("\033[%d;0!w",active); /* make active window active */
        fflush(stdout);
    }
    else if(Wistek())
    {
	if(active==2) {
	    printf( "\033%%!1" );
	}
	printf( "\033[%d;1H", SoftWindow[ 3 ].firstLine );
	printf( "\033[0J" );
	SoftWindow[ 3 ].curLine = SoftWindow[ 3 ].firstLine;
	SoftWindow[ 3 ].curCol = 1;
	if(active==2) {
	    printf( "\033%%!0" );
	}
    }
#ifdef SUN
    else if (Wissun())
    {   sendClearToMaster();
/*      printf("%c",CNTR_L); /* clear the text screen */
/* 	printf("\033[1;0;0;H"); */
        fflush(stdout);
    }
#endif
   }
}

/*----------------------------------------------------------------------
|
|	Wclear_graphics/0
|
|	This clears the graphics screen on the sun, graphon or hds terminal.
|
+----------------------------------------------------------------------*/
Wclear_graphics()
{
    if(Wisgraphon())
    {   /*if(4 != active)
            printf("\033[%dw",4); /* make window active temporarily */
	printf("\033\014"); /* clear screen (ESC FF or ESC ctrl-L) */ 
	if(2 != active)
            printf("\033[%dw",active); /* set previous window active */
        fflush(stdout); /* Send this out right away */
    }
    else if(Wishds())
    {   printf("\033[1+z");
	if(active !=2)
	    printf("\030");
        fflush(stdout);
    }
    else if(Wistek())
    {
	if (active != 2)
	 printf( "\033%%!0" );
	printf( "\033\014" );
	if (active != 2)
	 printf( "\033%%!1" );
    }
#ifdef SUN
    else if (Wissun()) /* if we have a sun, clear the screen */
    {	
        sunGraphClear();
    }
#endif
}

/*----------------------------------------------------------------------
|
|	Wgmode/0
|
|	This procedure sets terminal to tek 4014 mode it it can.
|
+----------------------------------------------------------------------*/

Wgmode()  /* tek grpahic mode */
{   graphicsOn = 1;
    if (Wisgraphon()) 
    {	printf("\035");
    }
    else if (Wistek())
    {
	printf( "\033%%!0" );			/* TEK mode */
        if (GisXORmode())
          printf( "\033RU!74" );
        else
          printf( "\033RU!;4" );
    }
    else if(Wishds())
    {	printf("\035");  /* switch to graphics mode */
	printf("\033[2+|"); /* Turn on graphics screen */
	printf("\033/1h"); /* non-scaled addressing */
    }
    else if (Wissun())
	graphicsOnTop(); /* make sure child is on top */
    active = 2;
    fflush(stdout); /* Send this out right away */
}

/*----------------------------------------------------------------------
|
|	Wamode/0
|
|	This procedure sets terminal to tek alpha mode if it can.  This
|	alpha mode if different than the alpha mode set by the Walpha
|	command.
|
+----------------------------------------------------------------------*/

Wamode() /* tek alpha mode */
{    if(Wisgraphon() || Wishds())
     {    printf("\037");
          fflush(stdout); /* Send this out right away */
     }
     active = 2;
}

/*----------------------------------------------------------------------
|
|	Wstprintf/1+
|
|	This procedure is like a regular printf except it prints the
|	message in the status area if terminal is a graphon or hds.
|	If not, it just prints it where ever.
|
+----------------------------------------------------------------------*/
Wstprintf(char *format, ...)
{
    va_list vargs;

    va_start(vargs, format);
    if(Wisgraphon()) /* if we have graphon */
    {   if(active != 1)  /* if status window not active */
            printf("\033[1w"); /* make status window active temporarily */
        vfprintf(stdout,format,vargs);
        if(active != 1) /* set previous window active */
            printf("\033[%dw",active); /* set previous window active */
        fflush(stdout); /* Send this out right away */
    }
    else if (Wishds())
    {	if (active != 1)
	{   if (active == 2) /* if we were in graphics */
		printf("\030"); /* goto alpha */
	    printf("\033[%d;1!w",1); /* select the window */
	}
        vfprintf(stdout,format,vargs);
	if (active != 1) 
	{   if (active == 2)
		Wgmode(); /* go back to grahpics */
	    printf("\033[%d;1!w",active); /* select the window */
	}
    }
    else if (Wistek())
    {
        char	tbuf[ 80 ];

	if (active == 2)
	{
	    printf( "\033%%!1" );			/* Must be in ANSI */
	}
	vsprintf( &tbuf[ 0 ], format, vargs );
	tekOutputChars( 1, &tbuf[ 0 ] );
/*	ival = strlen( &tbuf[ 0 ] );
	printf( "\033[%d;%dH",
		SoftWindow[ 0 ].curLine,
		SoftWindow[ 0 ].curCol
	);
	printf( "%s", &tbuf[ 0 ] );
	SoftWindow[ 0 ].curCol += ival;		   Assume no CR in string */
	if (active == 2)
	{
	    printf( "\033%%!0" );			/* Back to TEK mode */
	}
    }
    else
    {
        vfprintf(stdout,format,vargs);
	fprintf(stdout,"\n");
    }
    va_end(vargs);
}

/*----------------------------------------------------------------------
|
|	Wclearpos/4
|
|	This procedure clears any length line starting at any column in
|	any line in any window.
|
+----------------------------------------------------------------------*/
/* clear a string in a window at line,col */
Wclearpos(window,line,col,len)
int window;
int line;
int col;
int len;
{
    int i;
    int need_to_clean;

    need_to_clean = 1; /* always clean except if line is not dirty */
    switch (window)
    { case 1:
	switch (line)
	{ case 1: 
	    if (status_line_one_dirty)
		status_line_one_dirty = 0;
	    else
		need_to_clean = 0;
	    break;
	}
	break;
    }
    TPRINT1("Wclearpos: starting  statusline1 =%d\n",status_line_one_dirty);
    if (need_to_clean)
    {	if(Wisgraphon()) /* if we have graphon */
	{   if(active != window)  /* if window not active */
		printf("\033[%dw",window); /* make window active temporarily */
	    printf("\033[%d;%dH",line,col); /* go to the poisition */
	    for(i=0;i<len;i++) /*  blank out string */ 
		putchar(' ');
	    if(active != window) /* set previous window active */
		printf("\033[%dw",active); /* set previous window active */
	    fflush(stdout); /* Send this out right away */
	}
	else if(Wishds()) /* if we have hds */
	{   if(active != window)  /* if window not active */
	    {   if(active == 2)
		    printf("\030"); /* goto alpha mode */
		printf("\033[%d;0!w",window); /* make window active */
	    }
	    printf("\033[%d;%dH",line,col); /* go to the poisition */
	    for(i=0;i<len;i++) /*  blank out string */ 
		putchar(' ');
	    if(active != window) /* set previous window active */
	    {   if(active == 2)
		    Wgmode();
		else
		    printf("\033[%d;0!w",active); /*set previous window active*/
	    }
	    fflush(stdout); /* Send this out right away */
	}
	else if (Wistek())
	{
	    if (active == 2)
	    {
		printf( "\033%%!1" );			/* Must be in ANSI */
	    }
	    window--;
	    line += (SoftWindow[ window ].firstLine - 1);
	    printf( "\033[%d;%dH", line, col );
	    for (i=0;i<len;i++)
		putchar( ' ' );
	    SoftWindow[ window ].curCol = col+len;
	    SoftWindow[ window ].curLine = line;
	    if (active == 2)
	    {
		printf( "\033%%!0" );			/* Back to TEK mode */
	    }
	}
    }
}

/*----------------------------------------------------------------------
|
|	Wprintfpos/4+
|
|	This procedure positions a cursor to a position and then printfs.
|	The  user is responsible for any carriage returns.	
|	The cursor is positioned to a line relative to the window screen
|	chosen.  Lines start from line 1. Columns also start at 1.
|	If a negative column is passed, Wprintfpos will print from the
|	cursor position onwards. 
|	If the SUN is being used, Wprintfpos will attempt to create an
|	panel_item or canvas_item and then display the item in the
|	correct window.
|
+----------------------------------------------------------------------*/

void Wprintfpos(int window, int line, int col, char *format, ...)
{
    va_list vargs;

    va_start(vargs, format);
    switch (line)
    { case 1:
	if (window == 1)
	    status_line_one_dirty = 1;
	break;
      default:
	break;
    }
    if(Wisgraphon()) /* if we have graphon */
    {   if(active != window)  /* if window not active */
            printf("\033[%dw",window); /* make window active temporarily */
	if ( -1 < col) /* if we do not want positioning */
            printf("\033[%d;%dH",line,col); /* go to the poisition */
        vfprintf(stdout,format,vargs); /* print out the number */
        if(active != window) /* set previous window active */
            printf("\033[%dw",active); /* set previous window active */
        fflush(stdout); /* Send this out right away */
    }
    else if(Wishds()) /* if we have hds */
    {    /* printf("Wprintfpos:active=%d window = %d\n",active,window); */
        if(active != window)  /* if window not active */
        {   if(active == 2)
                printf("\030"); /* goto alpha mode */
             printf("\033[%d;0!w",window); /* make window active */
        }
	if ( -1 < col) /* if we do not want positioning */
            printf("\033[%d;%dH",line,col); /* go to the poisition */
        vfprintf(stdout,format,vargs); /* print out the number */
        if(active != window) /* set previous window active */
        {   if(active == 2)
                 Wgmode();
            else
                 printf("\033[%d;0!w",active); /* set previous window active */
        }
        fflush(stdout); /* Send this out right away */
    }
    else if (Wistek())
    {
        char	tbuf[ 1024 ];

	if (active == 2)
	{
	    printf( "\033%%!1" );			/* Must be in ANSI */
	}
	vsprintf( &tbuf[ 0 ], format, vargs );
	window--;					/*  Cvt to C index */
	if (col == 0) col++;
	if (-1 < col) {
		SoftWindow[ window ].curCol = col;
		SoftWindow[ window ].curLine =
			line + SoftWindow[ window ].firstLine - 1;
	}
	window++;
	tekOutputChars( window, &tbuf[ 0 ] );
	if (active == 2)
	{
	    printf( "\033%%!0" );			/* Back to TEK mode */
	}
	else if (active != window)
	  printf( "\033[%d;%dH",
		SoftWindow[ active-1 ].curLine,
		SoftWindow[ active-1 ].curCol);
    }
    else
    {
        vfprintf(stdout,format,vargs); /* send out the string */
    }        
    va_end(vargs);
}

/*---------------------------------------------------------------------
|
|   WinverseVideo    WnormalVideo
|
|   These two routines set the terminal to inverse or normal video
|   respectively.  They are necessary so as to not confuse the
|   Tektronix driver with escape sequences that do not move the
|   cursor.
|
|   These routines assume the terminal is in text mode, i. e.,
|   windows 1, 3 or 4 are active, but not window 2.
|
+----------------------------------------------------------------------*/
WinverseVideo()
{
    printf( "\033[7m" );
}

WnormalVideo()
{
    printf( "\033[27m" );
}

/*---------------------------------------------------------------------
|
|   WprintfposInv 
|
|   This routine prints out a string in inverse video on the graphon
|
+----------------------------------------------------------------------*/
WprintfposInv(int window, int line, int col, char *format, ...)
{
   va_list vargs;
   char newFormat[256];
   char buffer[1024];

   va_start(vargs, format);
   strcpy(newFormat,"\033[7m");
   strcat(newFormat,&format[0]);
   strcat(newFormat,"\033[27m");
   vsprintf(&buffer[ 0 ],newFormat,vargs);
   Wprintfpos(window,line,col,"%s",&buffer[ 0 ]);
   va_end(vargs);
}

/*----------------------------------------------------------------------
|
|	Wsprintfpos/4+
|
|	This procedure is similar to Wprintfpos except it gets a pointer
|	to a pointer for the args parameter.  This is so Werrprintf
|	will work.
|
+----------------------------------------------------------------------*/

Wsprintfpos(int window, int line, int col, char *format, ...)
{
    va_list vargs;

    va_start(vargs, format);
    if(Wisgraphon()) /* if we have graphon */
    {   if(active != window)  /* if window not active */
            printf("\033[%dw",window); /* make window active temporarily */
	if ( -1 < col) /* if we do not want positioning */
            printf("\033[%d;%dH",line,col); /* go to the poisition */
        vfprintf(stdout,format,vargs);
        if(active != window) /* set previous window active */
            printf("\033[%dw",active); /* set previous window active */
        fflush(stdout); /* Send this out right away */
    }
    else if(Wishds()) /* if we have hds */
    {    /* printf("Wprintfpos:active=%d window = %d\n",active,window); */
        if(active != window)  /* if window not active */
        {   if(active == 2)
                printf("\030"); /* goto alpha mode */
             printf("\033[%d;0!w",window); /* make window active */
        }
	if ( -1 < col) /* if we do not want positioning */
            printf("\033[%d;%dH",line,col); /* go to the poisition */
        vfprintf(stdout,format,vargs); /* print out the number */
        if(active != window) /* set previous window active */
        {   if(active == 2)
                 Wgmode();
            else
                 printf("\033[%d;0!w",active); /* set previous window active */
        }
        fflush(stdout); /* Send this out right away */
    }
    else if (Wistek())
    {
        char	tbuf[ 1024 ];

	if (active == 2)
	{
	    printf( "\033%%!1" );			/* Must be in ANSI */
	}
	vsprintf( &tbuf[ 0 ], format, vargs );
	window--;					/*  Cvt to C index */
	if (col == 0) col++;
	if (-1 < col) {
		SoftWindow[ window ].curCol = col;
		SoftWindow[ window ].curLine =
			line + SoftWindow[ window ].firstLine - 1;
	}
	window++;
	tekOutputChars( window, &tbuf[ 0 ] );
	if (active == 2)
	{
	    printf( "\033%%!0" );			/* Back to TEK mode */
	}
	else if (active != window)
	  printf( "\033[%d;%dH",
		SoftWindow[ active-1 ].curLine,
		SoftWindow[ active-1 ].curCol);
    }
    else
    {
        vfprintf(stdout,format,vargs); /* send out the string */
    }        
    va_end(vargs);
}

/*----------------------------------------------------------------------
|
|	Wclearsta/3
|
|	This procedure clears the status line starting at a line, col,
|	and length to clear.
|
+----------------------------------------------------------------------*/

Wclearsta(line,col,len)		int line,col,len;
{

    Wclearpos(1,line,col,len);
}

/*----------------------------------------------------------------------
|
|	Wscrprintf/1+
|
|	This procedure is like a regular printf except it prints the
|	message in the scroll area if terminal is a graphon or hds.
|	If not, it just prints it where ever.
|
+----------------------------------------------------------------------*/

void Wscrprintf(char *format, ...)
{
    va_list vargs;
    extern int   printOn;
    extern FILE *printfile;

    va_start(vargs, format);
    if (printOn) /* send to file if print in on */
    {
       vfprintf(printfile,format,vargs);
    }
    else if (Bnmr) /* if this is a background command */
    {
        vfprintf(stdout,format,vargs);
    }
    else
    {
    /* Since this command, could scroll a screen, mark text screen bad */
       Wsettextdisplay("write");
       if(Wisgraphon()) /* if we have graphon */
       {   if(active != 4)  /* if window 4 not active */
               printf("\033[4w"); /* make window active temporarily */
           vfprintf(stdout,format,vargs); /* print out the number */
           if(active != 4) /* set previous window active */
               printf("\033[%dw",active); /* set previous window active */
           fflush(stdout); /* Send this out right away */
       }
       else if(Wishds()) /* if we have hds */
       {   if(active != 4)  /* if window 4 not active */
           {   if(active == 2)
                   printf("\030"); /* goto alpha mode */
               printf("\033[4;0!w"); /* make window active temporarily */
           }
           vfprintf(stdout,format,vargs); /* print out the number */
           if(active != 4) /* set previous window active */
           {   if(active == 2)
                    Wgmode();
               else
                    printf("\033[%d;0!w",active);/*set previous window active */
           }
           fflush(stdout); /* Send this out right away */
       }
       else if (Wissun())
       {
	   textOnTop();
           vfprintf(stdout,format,vargs);
       }
       else if (Wistek())
       {
           char	tbuf[ 1024 ];

	   if (active == 2)
	   {
	       printf( "\033%%!1" );			/* Must be in ANSI */
	   }
	   vsprintf( &tbuf[ 0 ], format, vargs );
	   tekOutputChars( 4, &tbuf[ 0 ] );
	   if (active == 2)
	   {
	       printf( "\033%%!0" );			/* Back to TEK mode */
	   }
	   else if (active != 4)
	     printf( "\033[%d;%dH",
		SoftWindow[ active-1 ].curLine,
		SoftWindow[ active-1 ].curCol);
       }
       else
           vfprintf(stdout,format,vargs);
    }
    va_end(vargs);
}

/*----------------------------------------------------------------------
|
|	sendErrorToMaster/1   sendStatusToMaster/2  sendBeepToMaster/0
|       setTtyInputFocus/0
|
|	These procedures  sends error and status messages to
|	the master. sendBeepToMaster does just that.  setTtyInputFocus
|	makes the input tty subwindow the focus.
|
+----------------------------------------------------------------------*/
#ifdef SUN
extern int   isMaster;
#endif SUN

/* set focus of tty input window */
setTtyInputFocus()
{
#ifdef SUN
   if (isMaster)  /* if we are the master, do not send down stdout */
      focusOfTty(1);
   else
      sendTripleEscToMaster( 'f', "IN" );
#endif SUN
}

resetTtyInputFocus()
{
#ifdef SUN
   if (isMaster)  /* if we are the master, do not send down stdout */
      focusOfTty(0);
   else
      sendTripleEscToMaster( 'f', "OUT" );
#endif SUN
}

dgSetTtyFocus()
{
#ifdef SUN
   if (isMaster)  /* if we are the master, do not send down stdout */
      focusOfTty(1);
   else
      sendTripleEscToMaster( 'f', "IN2" );
#endif SUN
}

#ifdef SUN
sendErrorToMaster(m)		char *m;
{  if (isMaster)  /* if we are the master, do not send down stdout */
      showError(m);
   else
      sendTripleEscToMaster( 'E', m );
}

/*  Sends message to master instructing it to position the cursor
    of the text subwindow close to the bottom so that additional
    output will be visible.					*/

setTextAtBottom()
{
    if (Wissun())
	sendTripleEscToMaster( 'F', "bottom" );
}

sendBeepToMaster()
{  if (isMaster)  /* if we are the master, do not send down stdout */
      ring_bell();
   else
      sendTripleEscToMaster( 'B', "" );
}

sendCodeToMaster(m)		char *m;  
{
   sendTripleEscToMaster( *m, "" );
}

sendStatusToMaster(m,fieldnum)		char *m;  int fieldnum;
{
   char	tempstr[ 122 ];

   sprintf( &tempstr[ 0 ], "%2d %s", fieldnum, m );
   sendTripleEscToMaster( 'S', &tempstr[ 0 ] );
}
#endif

/*----------------------------------------------------------------------
|
|	sendPromptToMaster/1 
|
|	These procedures sends a message to master  to display in
|	the command textwindow a prompt
|
+----------------------------------------------------------------------*/

sendPromptToMaster(m)		char *m;
{
#ifdef SUN
    if (Wissun())
	sendTripleEscToMaster( 'I', m );
    else
#endif
	Wcomprint(m);	
}

/*----------------------------------------------------------------------
|
|	sendClearToMaster/0, sendClearerrToMaster/0
|
|	These procedures send a message to clear the bottom
|	textsubwindow, or the error window.
|
+----------------------------------------------------------------------*/

sendClearToMaster()
{
#ifdef SUN
    if (Wissun())
	sendTripleEscToMaster( 'F', "clear");
    else
#endif
	Wclear(4);	
}

sendClearerrToMaster()
{
#ifdef SUN
    if (Wissun())
	sendTripleEscToMaster( 'e', "clear");
#endif
}

/*----------------------------------------------------------------------
|
|	sendMoreMessageToMaster/0
|
|	These procedures sends a more message to master to start the 
|	more interaction.
|
+----------------------------------------------------------------------*/

sendMoreMessageToMaster()
{
#ifdef SUN
    if (Wissun())
	sendTripleEscToMaster( 'M', "More[y/n]?" );
#endif
}

/*----------------------------------------------------------------------
|
|	Wcomprint/1+
|
|	This procedure  prints a string in the command line.
|	Used for printing prompts
|
+----------------------------------------------------------------------*/
Wcomprint(string)	char *string;
{    
    Wsetactive(3);
    Wprintf("%s",string);
    fflush(stdout);
}

/*----------------------------------------------------------------------
|
|	Winfoprintf/1+
|
|	This procedure  prints a message in the error line.
|       It is based on Werrprintf. It does not make a bell or noise.
|
+----------------------------------------------------------------------*/
void Winfoprintf(char *format, ...)
{   char    str[1024];
    va_list vargs;

    va_start(vargs, format);
    TPRINT1("Winfoprintf: format ='%s'\n",format);
    Wclearerr();
    if (Wisgraphon() || Wishds() || Wistek())
    {
        vsnprintf(str, 1023, format,vargs);
        Wsprintfpos(1,3,1,str);
    }
#ifdef SUN
    else if (Wissun()) /* if we are on the sun */
    {
        vsnprintf(str, 1023, format,vargs);
        sendErrorToMaster(str);
    }
#endif SUN
    else
    {
        vfprintf(stdout,format,vargs);
        fprintf(stdout,"\n");
    }
    if (Mflag)
    {
        vfprintf(stderr,format,vargs);
        fprintf(stderr,"\n");
    }
    va_end(vargs);
    error_line_dirty = 1;
    TPRINT0("Winfoprintf: returning\n");
}

/*----------------------------------------------------------------------
|
|	getErrorPosition/2
|
|	This procedure creates a string containing the error position
|	and filename. If there is no file name, this routine returns
|	a null in the buf. 
|
+----------------------------------------------------------------------*/
char *getErrorPosition(buf,size)	char *buf;  int size;
{   char tmp[128];

/* For VMS, locate final ']' instead of '/'.  Rest works just line UNIX */
    
    if (doingNode && 1 <= doingNode->location.line && doingNode->location.file)
    {   char  tempFilename[64];
        char *ptr;

        /*  remove path prefix and .suffix from filename */
#ifdef UNIX
        if (ptr = strrchr(doingNode->location.file,'/'))
#else
        if (ptr = rindex(doingNode->location.file,']'))
#endif
        {   strncpy(tempFilename,ptr+1,64);
	    tempFilename[63] = '\0';
	    if (ptr = (char *)strrchr(tempFilename,'.'))
		*ptr = '\0';			 /* get rid of . part */
	}
	else
	{   strncpy(tempFilename,doingNode->location.file,64);
	    tempFilename[63] = '\0';
	}
        sprintf(tmp," at line %d (col %d) in %s"
		   ,doingNode->location.line
		   ,doingNode->location.column
		   ,tempFilename
	       );
    }
    else
	tmp[0] = '\0';
    strncpy(buf,tmp,size);
    buf[size-1] = '\0';			/* Index was `size', changed 09/07/90 */
    return(buf);
}

/*----------------------------------------------------------------------
|
|       Error Logging Facility
|
|       For Werrprintf and WerrprintfWithPos it is desired to store
|	the last few lines for future review.  The parameter errloglen
|       defines the number of lines we keep.  Default value is 10.
|
|	The error logging facility relies on a list-of-strings facility,
|	found in lstring.c
+----------------------------------------------------------------------*/

#define  DEFAULT_ERROR_LOG_LEN  10

static char	*baseOfErrorLog;

static void
logErrorLine( string )
char *string;
{
	int	iter, i_errorloglen, numberOfStrings, r;
	double	d_errorloglen;

	if (r = P_getreal(GLOBAL, "errloglen", &d_errorloglen, 1) != 0)
	  i_errorloglen = DEFAULT_ERROR_LOG_LEN;
	else {
		i_errorloglen = (int) (d_errorloglen + 0.5);
		if (i_errorloglen < 1)
		  i_errorloglen = DEFAULT_ERROR_LOG_LEN;
	}

	addStringToEnd( &baseOfErrorLog, string, "errorLog" );

	numberOfStrings = getNumberOfStrings( baseOfErrorLog );
	if (numberOfStrings > i_errorloglen) {
		for (iter = i_errorloglen; iter < numberOfStrings; iter++)
		  deleteFirstString( &baseOfErrorLog );
	}
}

/****************************/
int errlog(argc,argv,retc,retv)
/****************************/
int argc;
char *argv[];
int retc;
char *retv[];
{
	printEachString( baseOfErrorLog);
	RETURN;
}

/*----------------------------------------------------------------------
|
|	WerrprintfWithPos/1+
|
|	This procedure  prints a message in the error line.
|	and beeps the bell and/or panel. It includes the position in
|       the macro file where error has occured.
|
+----------------------------------------------------------------------*/
void WerrprintfWithPos(char *format, ...)
{
    va_list      vargs;
    char         str[1024];
    char         tmp[128];

    va_start(vargs, format);
    TPRINT1("WerrprintfWithPos: format ='%s'\n",format);
    getErrorPosition(tmp,128);
    Wclearerr();
    vsprintf( &str[ 0 ], format, vargs );
    if (Wisgraphon() || Wishds() || Wistek())
    {
        Wsprintfpos(1,3,1,str);
	Wprintfpos(1,3,-1,"%s",tmp);
	if (beepOn)
	{   if ((active == 2) && !Wistek())  /* if we are in graphics */
		printf("\033[3w"); /* make error window active temporarily */
	    putchar('\007');
	    if ((active == 2) && !Wistek())
		printf("\033[2w"); /* make graphics window active */
	}
    }
#ifdef SUN
    else if (Wissun()) /* if we are on the sun */
    {
        strcat(str,tmp);
        if (glide_active())
          Wscrprintf("%s\n",str);
        else
          sendErrorToMaster(str);
        if (beepOn)
    	    sendBeepToMaster();
    }
#endif
    else
    {
        fprintf(stdout,str);
        fprintf(stdout,tmp);
        fprintf(stdout,"\n");
    }
    error_line_dirty = 1;
    if (Mflag)
    {
        fprintf(stderr,str);
        fprintf(stderr,"\n");
    }

    if (!Bnmr)
      logErrorLine( str );

    va_end(vargs);
    TPRINT0("WerrprintfWithPos: returning\n");
}

/*----------------------------------------------------------------------
|
|	Werrprintf/1+
|
|	This procedure  prints a message in the error line.
|	and beeps the bell and/or panel. It does not give position
|       information.
|
+----------------------------------------------------------------------*/
void Werrprintf(char *format, ...)
{   char         str[1024];
    va_list      vargs;

    va_start(vargs, format);
    TPRINT1("Werrprintf: format ='%s'\n",format);
    Wclearerr();
    vsprintf(str,format,vargs);
    if (Wisgraphon() || Wishds() || Wistek())
    {
        Wsprintfpos(1,3,1,str);
	if (beepOn)
	{   if ((active == 2) && !Wistek())  /* if we are in graphics */
		printf("\033[3w"); /* make error window active temporarily */
	    putchar('\007');
	    if ((active == 2) && !Wistek())
		printf("\033[2w"); /* make graphics window active */
	}
    }
#ifdef SUN
    else if (Wissun()) /* if we are on the sun */
    {
        if (glide_active())
          Wscrprintf("%s\n",str);
        else
          sendErrorToMaster(str);
        if (beepOn)
    	    sendBeepToMaster();
    }
#endif
    else
    {
        fprintf(stdout,str);
        fprintf(stdout,"\n");
    } 
    error_line_dirty = 1;
    if (Mflag)
    {
        fprintf(stderr,str);
        fprintf(stderr,"\n");
    }

    if (!Bnmr)
      logErrorLine( str );

    va_end(vargs);
    TPRINT0("Werrprintf: returning\n");
}

/*----------------------------------------------------------------------
|
|	Wclearerr/0
|
|	This procedure  clears the error line.
|
+----------------------------------------------------------------------*/
/* clear error line */
Wclearerr()
{
    if (error_line_dirty)
    {
#ifdef SUN
        if (Wissun())
	    sendErrorToMaster("");
	else
#endif
	{
	    Wclearpos(1,3,1,80);  
	}
	error_line_dirty = 0;
    }
}

/*----------------------------------------------------------------------
|
|	Wgraphics_large/0  Wgraphics_small/0
|
|	This procedure make the graphics large or small
|
+----------------------------------------------------------------------*/
static int graphicsLarge = 0;

Wgraphics_large()
{
#ifdef SUN
    if (Wissun())
    {   if (!graphicsLarge)

/*  Large:
      Set the width of the graphics frame to be the width of the entire screen.
      Set the height to be the height of the entire screen less the height of
      the VNMR status and input windows (so they will remain visible).  Use the
      WIN_EXTEND_TO_EDGE option to set the size of the graphics canvas to fill
      its frame but no overlap the margins of the parent frame.  Do the latter
      operations now so the rest of VNMR (graphics.c) will know immediately the
      new size of the graphics canvas.  To let the notifier adjust the canvas
      size is not acceptable as another VNMR command may execute (ds) before
      the notifier has a change to resize the graphics canvas.			*/
      largeWindow();
    }
    graphicsLarge = 1;
#endif
}

Wgraphics_small()
{
#ifdef SUN
    if (Wissun())
    {   if (graphicsLarge)

/*  Small:
      Set the width and the height of the graphics frame to values established
      when VNMR started.  Use the WIN_EXTEND_TO_EDGE option to set the size of
      the graphics canvas to fill its frame but not overlap the margins of the
      parent frame.  See comments in Wgraphics_large also.			*/
       smallWindow();
    }
    graphicsLarge = 0;
#endif
}

/*  Both large and small now turn off the buttons to stop any interactive
    graphical display programs (ds, dfid, etc.)		05/13/91	*/

/****************************/
int large(argc,argv,retc,retv)
/****************************/
int argc; char *argv[]; int retc; char *retv[];
{
   if (Wissun())
   {  Wgraphics_large();
      makeSmallBut(); /* tell master to make button label small */
/**
      Wturnoff_buttons();
      Wclear_graphics();
      Wsetgraphicsdisplay( "large" );
**/
   }
  else
    Werrprintf("large: command available only on Sun screen");
  RETURN;
}

/****************************/
int small(argc,argv,retc,retv)
/****************************/
int argc; char *argv[]; int retc; char *retv[];
{
   if (Wissun())
   {  Wgraphics_small();
      makeLargeBut();  /* tell master to make button label large */
/**
      Wturnoff_buttons();
      Wclear_graphics();
      Wsetgraphicsdisplay( "small" );
**/
   }
   else
     Werrprintf("small: command available only on Sun screen");
   RETURN;
}

/*-------------------------------------------------------------------------
|	WrestoreTerminal/0
|
|	This routine returns terminal back to inital state, called only
|	when exiting from a foreground vnmr.
|
+-------------------------------------------------------------------------*/
WrestoreTerminal()
{   cursorOff(MOUSE);
    cursorOff(BUTTONS);
    Wsetcommand();
    Wshow_text();  /* make text the last thing that is shown */
    restoreInput(); /* restore input */
    Wresterm("bye");
}

static char *
fgets_withintr( datap, count, filep )
char *datap;
int count;
FILE *filep;
{
	char	*qaddr;

	while ( (qaddr = fgets( datap, count, filep )) == NULL) {
		if (errno != EINTR || (feof(filep)))
		  return( NULL );
          if (interuption)
	    return( NULL );
	}

	return( qaddr );
}

/*-------------------------------------------------------------------------
|	W_getInput/3
|
|	This routine prints out an optional prompt onto the command line
|	and reads in a buffer full of data.  It is up to the programmer
|	to digest this buffer full of data. W_getInput will not figure
|	out if the data is a string, integer, etc.  Note we use a
|	standard fgets (how clever jim was to dup the pipe to stdin).
|	Thus we will get the same result as doing a fgets which is 
|	fgets reads n-1 characters, or up to a newline character which
|	ever comes first into the string s. The last character read
|	into s is followed by a null character. fget returns its first
|	argument. If fget returns NULL, we have end of file or error
|	We have two versions of getInput
|	W_getinputCR - returns the entire string with <cr> if any
|	W_getinput - returns entire string Without <cr> if any
|	
+-------------------------------------------------------------------------*/
char *
W_getInput(prompt,s,n)	char *prompt; char *s; int n;
{   char *rets;
    char *W_getInputCR();
    int   len;

    rets = W_getInputCR(prompt,s,n);
    len = strlen(s); /* get string length */
    if(s[len-1] == '\n') /* if the last character is a \n */
	s[len-1] = '\0'; /* set it to null */
    return (rets);
}

char *
W_getInputCR(prompt,s,n)	char *prompt; char *s; int n;
{   extern char *fgets_nointr();
    char *rets;

/*  Prevent user input from background mode.  */

    if (Bnmr)
    {
	Wscrprintf( "No user input from Background mode\n" );
	*s = '\0';			/* render input string the 0-length string */
	return( s );
    }

    if (prompt)
	sendPromptToMaster(prompt);
    else
	sendPromptToMaster("?");
    restoreInput(); /* let fgets work */
/*  WunlockPanel();  unlock panel if necessary */
    rets = fgets_withintr(s,n,stdin);

/*  Master sent us a new-line character.  Thus he incremented the fgBusy
    count.  Now that we have received the character, let the master know
    it has been processed.  This should NOT reduce the busy count to 0,
    for the master surely sent us a new-line command earlier to get the
    child here!!
 */
    sendReadyToMaster();
    if (Wistek()) scrollWindow(3);
    setInputRaw(); /* set it back */
    if (interuption)
    {
       s[0] = '\0'; /* set it to null */
       sendPromptToMaster("\r");
       return (NULL);
    }
    return (rets);
}

/*-------------------------------------------------------------------------
|	WunlockPanel/0
|
|	This routine unlocks the input panel after a button push. This must
|	be done if input is to be accepted by other button pushes
|
+-------------------------------------------------------------------------*/
WunlockPanel()
{
#ifdef SUN
   if (Wissun())
       unlockPanel();
#endif SUN
}

/*-------------------------------------------------------------------------
|	WmoreStart/0
|
|	This routine sets the more line counter to one initializing
|	the more counter.
|
+-------------------------------------------------------------------------*/
WmoreStart()
{  MoreCount = 1;
}

/*-------------------------------------------------------------------------
|	WmoreEnd/0
|
|	This routine sets the more line counter to zero turning off
|	the more counter.
|
+-------------------------------------------------------------------------*/
WmoreEnd()
{  MoreCount = 0;
}

/*----------------------------------------------------------------------
|
|	Wmoreprintf/1+
|
|	This procedure is like a regular printf except it 
|	has a more like feature.  Once initialized by WmoreStart,
|	each Wmoreprintf will check if it has filled up the screen.
|	If it has it will send the more prompt.  If yes or a number
|	of rows is not entered, the routine goes into ignore mode
|	and will ignore (not print) any other Wmoreprintf until
|	WmoreEnd or WmoreStart is called.  WmoreEnd turns off this
|	routine making Wmoreprintf just like a Wscrprintf.
|
+----------------------------------------------------------------------*/
void Wmoreprintf(char *format, ...)   /* vargs is format,args */
{  char string[1024];	/* ``string'' holds the answer to the More query  */
   int  num;		/* and the output from the format conversion.     */
   int  screenLength;
   int  do_print;
   extern int   printOn;
   va_list      vargs;


/* more is disabled on printOn (output going to printer) or bg or SUN console */

   va_start(vargs, format);
   do_print = 0;
   if (MoreCount == 0 || printOn || Bnmr || Wissun()) 
      do_print = 131071;
   else if (MoreCount > 0)
   {
      screenLength = WscreenSize();
      if (++MoreCount > screenLength)
      {  W_getInput("More[rows/y/n]?>",string,sizeof(string));
         if (*string == 'Y' || *string == 'y')
            MoreCount = 1;
         else if ((num = atoi(string)) > 0)
	    MoreCount = screenLength - num +1;
         else
            MoreCount = -1;  /* do not print */ 
      }
      if (MoreCount >= 0)
         do_print = 131071;
   }

/*  Do the output conversion here, not in Wscrprintf.  Very difficult
    to pass a variable argument list from one subroutine to another.	*/

   if (do_print)
   {
      vsprintf( &string[ 0 ], format, vargs );
      Wscrprintf( "%s", &string[ 0 ] );
   }
      
   va_end(vargs);
}

/*----------------------------------------------------------------------
|
|	WscreenSize()
|
|	This function returns the size of the scrolling window
|
+-----------------------------------------------------------------------*/
int WscreenSize()
{
	if (Wisgraphon() || Wishds())
	  return( GRAPHONSIZE );
	else if (Wistek4x05())
	  return( TEK4105SIZE );
	else if (Wistek4x07())
	  return( TEK4107SIZE );
	else					/* Assume sun screen */
	  return( SUNSIZE );
}

/*----------------------------------------------------------------------
|
|	text_is
|
|	This procedure determines what is displayed on the text screen.
|
+----------------------------------------------------------------------*/
int text_is(argc,argv,retc,retv)
int argc; char *argv[]; int retc; char *retv[];
{
  char           textcmd[20];
  static char resetDisp[20] = "";
  static char resetKey[20] = "";

  if (argc == 1)
    if (retc>0)
      retv[0] = newString(Wgettextdisplay(textcmd,20));
    else
      Winfoprintf("the text window currently displays %s",
                   Wgettextdisplay(textcmd,20));
  else if (argc == 2)
    if (retc>0)
      retv[0] = realString((double) WtextdisplayValid(argv[1]));
    else
      Winfoprintf("the text window %s displaying %s",
                   (WtextdisplayValid(argv[1])) ? "is" : "is not",argv[1]);
  else if ((argc == 3) && (strcmp(argv[1],"set") == 0))
  {
    Wgettextdisplay(resetDisp,20);
    strncpy(resetKey,argv[2],20);
    Wsettextdisplay(argv[2]);
  }
  else if ((argc == 3) && (strcmp(argv[1],"reset") == 0))
  {
     if (strcmp(resetKey,Wgettextdisplay(textcmd,20)) == 0)
     {
         Wsettextdisplay(resetDisp);
         appendMagicVarList();
         strcpy(resetKey,"");
     }
  }
  else
  {
    Werrprintf("Usage: %s<('command')>",argv[0]);
    ABORT;
  }
  RETURN;
}

/*----------------------------------------------------------------------
|
|	graph_is
|
|	This procedure determines what is displayed on the graphics screen.
|
+----------------------------------------------------------------------*/
int graph_is(argc,argv,retc,retv)
int argc; char *argv[]; int retc; char *retv[];
{
  char           graphcmd[20];

  if (argc == 1)
    if (retc>0)
      retv[0] = newString(Wgetgraphicsdisplay(graphcmd,20));
    else
      Winfoprintf("the graphics window currently displays %s",
                   Wgetgraphicsdisplay(graphcmd,20));
  else if (argc == 2)
    if (retc>0)
      retv[0] = realString((double) WgraphicsdisplayValid(argv[1]));
    else
      Winfoprintf("the graphics window %s displaying %s",
                   (WgraphicsdisplayValid(argv[1])) ? "is" : "is not",argv[1]);
  else
  {
    Werrprintf("Usage: %s<('command')>",argv[0]);
    ABORT;
  }
  RETURN;
}

/*----------------------------------------------------------------------
|
|       bgmode_is
|
|       This procedure determines whether Vnmr is in background mode
|
+----------------------------------------------------------------------*/
int bgmode_is(argc,argv,retc,retv)
int argc; char *argv[]; int retc; char *retv[];
{
  char           graphcmd[20];

    if (retc>0)
      retv[0] = realString((double) Bnmr);
    else
      if (!Bnmr)
        Winfoprintf("Vnmr is in foreground mode");
/* can't print to Winfoprintf if it's in background mode */
  RETURN;
}

/*--------------------------------------------------------------
|   The following routines were added to support the Tektronix	|
|   series of terminals.					|
 --------------------------------------------------------------*/

static killSoftWindow( wnum )
int wnum;
{
	if (wnum < 1 || wnum > NUM_WINDOWS) ABORT;

	SoftWindow[ wnum-1 ].firstLine = 0;
	RETURN;
}

static setSoftWindow( plane, wnum, firstl, lastl )
int plane;
int wnum;
int firstl;
int lastl;
{
	int		cval, iter;

	if (wnum < 1 || wnum > NUM_WINDOWS) ABORT;
	wnum--;						/* Cvt to C index */
	if (SoftWindow[ wnum ].firstLine > 0) ABORT;

/*  Note that the window being specified is not defined.  */

	for (iter = 0; iter < NUM_WINDOWS; iter++)	/* Verify no overlap */
	 if (( cval = SoftWindow[ iter ].firstLine ) > 0) {
		if (cval == firstl) ABORT;
		else if (cval < firstl)
		 if (cval + SoftWindow[ iter ].numLines - 1 >= firstl) ABORT;
		 else ;
		else
		 if (lastl >= cval) ABORT;
	 }

	SoftWindow[ wnum ].plane = plane;
	SoftWindow[ wnum ].firstLine = firstl;
	SoftWindow[ wnum ].numLines = lastl-firstl+1;
	SoftWindow[ wnum ].curLine = firstl;
	SoftWindow[ wnum ].curCol = 1;
}

static tek()
{
	int		iter;

/*  Verify the program supports this model of Tektronix  */

	if (Wistek42xx() == 0 && Wistek41xx() == 0) {
		printf(
	    "Tektronix model %s not supported, program exits\n", graphics
		);
		exit();
	}
	if (Wistek4x05() == 0 && Wistek4x07() == 0) {
		printf(
	    "Tektronix screen %s not supported, program exits\n", graphics
		);
		exit();
	}

	for (iter = 0; iter < NUM_WINDOWS; iter++)
	 killSoftWindow( iter+1 );
	setSoftWindow(1, 1, 1, 3);
	setSoftWindow(1, 3, 4, 4+NDIALOGLINES-1);
	if (Wistek4x05())
	 setSoftWindow(1, 4, 4+NDIALOGLINES, 4+NDIALOGLINES+TEK4105SIZE+1);
	else
	 setSoftWindow(1, 4, 4+NDIALOGLINES, 4+NDIALOGLINES+TEK4107SIZE+1);
	for (iter = 0; iter < 80*NDIALOGLINES; iter++)
	 screenDbase[ iter ] = ' ';

/*  Put terminal in ANSI mode, in preparation for <ESC><LF>  */

	printf("\033%%!1");
	printf("\033\012\033c");

/*  For the 4200 series, define all mouse keys to null, so that either
    press or release produces nada.  The routine to enable TEK GIN must
    program -155, -157 and -159 to generate "q", "w", and "e".

    Since defineTekKey now expects the terminal to be in TEK mode (and
    leaves it in TEK mode too), set the terminal to that mode first	*/

	if (Wistek42xx())
	{
	    printf("\033%%!0");
	    defineTekKey( -155, "" );
	    defineTekKey( -156, "" );
	    defineTekKey( -157, "" );
	    defineTekKey( -158, "" );
	    defineTekKey( -159, "" );
	    defineTekKey( -160, "" );

	    printf("\033IS");		/* Set Report Signature Character */
	    encodeTekInt( 64 );		/* 64 is simplest Mouse report */
	    encodeTekInt( 27 );		/* Use <ESC> to introduce reports */
	    printf("\033NT");		/* No terminating character */
	    printf("\033%%!1"); 	/* Back to ANSI mode */
	    hasMouse = 131071;
	}				/* End of Tek 4200 series stuff */
	else {
	    setIconLimits();
	    Ginit_icon( 200, 200 );
	    hasMouse = 0;
	}
	Wsetactive(4);
	Wclear_graphics();
	setjump();
	setwrap();
	Wsetactive(3);
}

/*  The following routines allow text output while keeping track of the
    position of the Cursor, necessary for emulating display windows.	*/

void Wprintf(char *format, ...)
{
    va_list      vargs;
    char	 tbuf[ 1026 ];

    va_start(vargs, format);
    if (Wistek())
    {
	vsprintf( &tbuf[ 0 ], format, vargs );
	tekOutputChars( active, &tbuf[ 0 ] );
    }
    else
        vfprintf(stdout,format,vargs);
    va_end(vargs);
}

Wputchar( c )
char c;
{
	int	col, cpos;

	if ( !Wistek() )
	  putchar( c );
	else {
		col = SoftWindow[ active-1 ].curCol;
		if (active == 2) ABORT;
		if (c == '\033') ABORT;
		if (c == '\n')
		  scrollWindow( active );
		else if (c == '\b') {
			if (col > 1) {
				SoftWindow[ active-1 ].curCol--;
				putchar( c );
			}
		}
		else {
			if (col > 79)
			  printf( "\033[%d;80H",
				SoftWindow[ active-1 ].curLine
			  );
			putchar( c );
			if (active == 3) {
				cpos = (SoftWindow[ 2 ].curLine-
					SoftWindow[ 2 ].firstLine)*80 +
					SoftWindow[ 2 ].curCol-1;
				screenDbase[ cpos ] = c;
			}
			if (col < 80) SoftWindow[ active-1 ].curCol++;
		}
	}
}


/*  The child (Vnmr) effects many functions in the master by sending
    it a triple escape sequence.  Each sequence of 3 <ESC> characters
    is followed by the process ID of the child, a single character
    code and a string of characters.					*/

#define STDOUT_FD	1

int
sendTripleEscToMaster( code, string_to_send )
char code;
char *string_to_send;
{
#ifdef SUN
	char	tempstr[ 2048 ];

	sprintf( &tempstr[ 0 ],
	    "\033\033\033%6u%c%s\n", getpid(), code, string_to_send
	);
	fflush( stdout );

/*  Use the write system call rather than (say) printf because these
    messages must be delivered to Master in an atomic fashion; that is,
    no intervening output (say from a Child process, such as PSG) can
    occur.  The preceding call to fflush is required so we can bypass
    the library output scheme in this fashion.  Otherwise output from
    prior calls to printf could still be in the buffer when this call
    to write occurs, causing behavoir beyond rational prediction.

    This should help correct the Button Mismatch in Master bug, the
    mysterious disappearance of Buttons and other intermittent bugs
    found in the course of running VNMR, especially when running PSG.	*/

	write( STDOUT_FD, &tempstr[ 0 ], strlen( &tempstr[ 0 ] ) );

#endif
	return( 0 );
}

void output(c)				int c;
{
#ifdef SUN
   char one;

   one = c;
   textprintf(&one,1);
#endif
}

/*  The windows are emulated in software on the Tektronix series.
/*    To judge what effect an operation will have on the position of
/*    the cursor, the number of \n and \b characters must be measured.
/*    Thus the characters to be output have to be stored in a buffer.
/*    The following routine serves to emulate SPRINTF where the exact
/*    argument list has to be obtained from the arguments passed to
/*    Wscrprintf(), etc.					*/
/*
/*char *do_sprintf( buf, fmt, args )
/*char *buf;
/*char *fmt;
/*char **args;
/*{
/*#ifdef VMS					/* Taken from VMS sprintf */
/*	int	ival;
/*
/*	ival = C$$DOPRINT( buf, fmt, args );
/*	buf[ ival ] = '\0';
/*#else
/*	struct _iobuf	_strbuf;
/*
/*	_strbuf._flag = _IOWRT + _IOSTRG;
/*	_strbuf._ptr  = (unsigned char *) buf;
/*	_strbuf._cnt  = 1024;			/* Size of all buffers this */
/*	_doprnt( fmt, args, &_strbuf ); 	/* routine is called with. */
/*	putc( '\0', &_strbuf );			/* Terminate string. */
/*#endif
/*
/*	return( buf );
/*}
 */

/*  Scroll a window, using software.  Window #4 extends (by definition)
    to the bottom of the screen; thus it is a little easier to scroll.  */

static scrollWindow( wnum )
int wnum;
{
	char		tempBuf[ 82 ];
	char		*cptr1, *cptr2;
	int		iter, nchrs;

	if (wnum < 3 || 4 < wnum) ABORT;

/*  Check for current line less than the botton line of the window
    and output a newline character if so.				*/

	if (SoftWindow[ wnum-1 ].curLine <
	  SoftWindow[ wnum-1 ].firstLine + SoftWindow[ wnum-1 ].numLines-1) {
		putchar( '\n' );
		SoftWindow[ wnum-1 ].curLine++;
		SoftWindow[ wnum-1 ].curCol = 1;
		return;
	}
	SoftWindow[ wnum-1 ].curCol  = 1;
	SoftWindow[ wnum-1 ].curLine = SoftWindow[ wnum-1 ].firstLine +
		SoftWindow[ wnum-1 ].numLines - 1;
	if (wnum == 4) {
		printf( "\033[%d;1H\033[1M", SoftWindow[ 3 ].firstLine );
		printf( "\033[%d;1H", SoftWindow[ 3 ].curLine );
	}
	else {

	  /*  cptr1 points to start of 2nd line in dialog window  */

		cptr1 = &screenDbase[ 80 ];
		for (iter = 1; iter < SoftWindow[ 2 ].numLines; iter++) {
			strncpy( &tempBuf[ 0 ], cptr1, 80 );
			cptr1 += 80;
			tempBuf[ 80 ] = '\0';
			printf( "\033[%d;1H%s",
				SoftWindow[ 2 ].firstLine+iter-1,
				&tempBuf[ 0 ]
			);
		}
		printf( "\033[%d;1H\033[2K", SoftWindow[ 2 ].curLine );

	  /*  Now shuffle dialog lines in the screen database  */

		cptr1 = &screenDbase[ 0 ];
		cptr2 = cptr1+80;
		nchrs = (SoftWindow[ 2 ].numLines-1)*80;
		for (iter = 0; iter < nchrs; iter++)
		  *(cptr1++) = *(cptr2++);
		for (iter = 0; iter < 80; iter++)
		  *(cptr1++) = ' ';
	}
}

/*  Output characters to the Tektronix, using our software windows.  */

static tekOutputChars( wnum, sptr )
int wnum;
char *sptr;
{
	char		tekBuf[ 82 ];
	char		*nlptr;
	int		col, cpos, finished, len, line, lastLine;

/*  Check for invalid windows.  Window number 2 represents graphical
    display and cannot be access by this routine.		    */

	if (wnum < 1 || 4 < wnum) ABORT;
	if (wnum == 2) ABORT;

/*  Subtract 1 from window number to facilitate indexing  */

	wnum--;
	nlptr = strchr(sptr, '\n' );
	line = SoftWindow[ wnum ].curLine;
	col  = SoftWindow[ wnum ].curCol;
	printf( "\033[%d;%dH", line, col );		/* Position cursor */

/*  At this time, no scrolling is allowed in Window #1.  */

	if (wnum == 0) {				/* Actually #1 */
		len = (nlptr != NULL) ?
		    nlptr - sptr :
		    strlen( sptr );
		if (col+len > 80+1)
		  len = 80+1-col;
		strncpy( &tekBuf[ 0 ], sptr, len );
		tekBuf[ len ] = '\0';
		printf( "%s", &tekBuf[ 0 ] );
		SoftWindow[ 0 ].curCol += len;
		if (SoftWindow[ 0 ].curCol > 80) SoftWindow[ 0 ].curCol = 1;
		RETURN;
	}

	lastLine = SoftWindow[ wnum ].firstLine +
	  SoftWindow[ wnum ].numLines - 1;
	finished = 0;
	do {
		len = (nlptr != NULL) ?
		    nlptr - sptr :
		    strlen( sptr );
		if (col+len > 80+1)
		  len = 80+1-col;
		strncpy( &tekBuf[ 0 ], sptr, len );
		tekBuf[ len ] = '\0';
		printf( "%s", &tekBuf[ 0 ] );
		if (wnum == 2) {
			cpos = (line-SoftWindow[ 2 ].firstLine)*80 + col-1;
			strncpy( &screenDbase[ cpos ], &tekBuf[ 0 ], len );
		}
		if (nlptr != NULL) {
			line++;
			if (line > lastLine) {
				scrollWindow( wnum+1 );
				line = lastLine;
			}
			else
			  printf( "\033[%d;1H", line );
			col = 1;
			sptr = nlptr+1;
			nlptr = strchr( sptr, '\n' );
		}
		else {
			finished = 131071;
			col += len;
		}
	} while ( !finished );

	SoftWindow[ wnum ].curLine = line;
	SoftWindow[ wnum ].curCol  = col;
	if (SoftWindow[ wnum ].curCol > 80) 
	 SoftWindow[ wnum ].curCol = 80;
}

defineTekKey( knum, kstr )
int knum;
char *kstr;
{
	int	klen;

	klen = strlen( kstr );
	if (klen < 0) return;
	printf( "\033KD" );
	encodeTekInt( knum );
	encodeTekInt( klen );
	while ( *kstr )
	 encodeTekInt( (int) *(kstr++) );
}

static encodeTekInt( ival )
{
	int	i1, i2, i3, jval;

	jval = (ival < 0) ? -ival : ival;
	i1 = (jval >> 10) + 64;
	i2 = ((jval >> 4) % 64) + 64;
	i3 = jval % 16 + 32;
	if ( ival >= 0 ) i3 += 16;

	if ( i1 != 64 ) printf( "%c", i1 );
	if ( i2 != 64 ) printf( "%c", i2 );
	printf( "%c", i3 );
}
