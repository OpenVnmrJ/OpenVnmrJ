/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#ifndef MOTIF
#include <sys/time.h>
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <ctype.h>

#ifdef SOLARIS
#include <sys/filio.h>
#endif 

#include "vnmrsys.h"
#include "locksys.h"
#include "tools.h"

#ifdef  MOTIF
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

extern Font  x_font;
extern Font  org_x_font;
static XrmDatabase  dbase = NULL;

#else
#include "vjXdef.h"
#endif 

Widget       canvasShell;
XtInputId    inputId;

extern int   charWidth, charHeight;
extern int   graf_width, graf_height;
extern int   inputfd;

extern char  Xserver[];
extern char  Xdisplay[];
extern char  fontName[];
extern XID   xid;
extern Display *xdisplay;

extern FILE  *popen_call();
extern void  ignoreSigpipe();
extern void nmr_exit(char *modeptr);
extern void processChar(char c);
extern void save_imagefile_geom();
extern int nmrExit(int argc, char *argv[], int retc, char *retv[]);

extern char  Jvbgname[];
extern char  vnMode[];

char   *get_x_resource();

#ifdef  MOTIF
static int  xwin = 0;
static char *font_names[] = {"6x12", "7x13", "8x13", "9x15", "10x20"};
#endif


void
setUpWin(int windowRetain, int noUi)
{

#ifdef  MOTIF
   int 		   n;
   int             argc;
   char           *argv[6];
   char           *name = "Vnmr";
   char 	*font_name;
   XFontStruct  *newFontInfo = NULL;

   
   if (noUi) {
        xwin = 0;
        return;
   }
   xwin = 1;
   argv[0] = name;
   argc = 1;
   if (Xserver[0] != '\0')
   {
        argv[argc++] = Xdisplay;
        argv[argc++] = Xserver;
   }

   canvasShell = XtInitialize("Vnmr", "Vnmr", NULL, 0, &argc, argv);
   xdisplay = XtDisplay(canvasShell);
   if (!xdisplay)
   {
        fprintf(stderr, " Error: Can't open display %s, exiting...\n", Xserver);
        exit (0);
   }
   graf_width = 0;
   graf_height = 0;
   x_font = (Font) NULL;
   if (fontName[0] == '\0')
   {
	if ((font_name = get_x_resource("Vnmr", "fontList")) == NULL)
           strcpy(fontName, "9x15");
	else
           strcpy(fontName, font_name);
   }
   if (fontName[0] != '\0')
   {
	 if ((newFontInfo = XLoadQueryFont(xdisplay, fontName))!=NULL)
                x_font = newFontInfo->fid;
   }
   if (x_font == (Font) NULL)
   {
	for (n = 0; n < 5; n++)
	{
	   if ((newFontInfo = XLoadQueryFont(xdisplay, font_names[n]))!=NULL)
	   {
                x_font = newFontInfo->fid;
		strcpy(fontName, font_names[n]);
		break;
	   }
	}
   }
   if (x_font)
   {
        charWidth = newFontInfo->max_bounds.width;
        charHeight = newFontInfo->max_bounds.ascent
                      + newFontInfo->max_bounds.descent;
   }
   else
   {
        charWidth = 9;
        charHeight =  15;
   }
   org_x_font = x_font;
   xid = 0;
/*
   sun_window();
*/
#else
   xid = 0;
   xdisplay = NULL;
   canvasShell = 0;
#endif
}

char *
get_x_resource(class, value)
char  *class, *value;
{
#ifdef  MOTIF
	char       info_str[128];
        char       *str_type;
        XrmValue   rval;

	if (dbase == NULL)
           return((char *) NULL);
        sprintf(info_str, "%s*%s", class, value);
        if (XrmGetResource(dbase, info_str, class, &str_type, &rval))
           return((char *) rval.addr);
        if (XrmGetResource(dbase, value, class, &str_type, &rval))
           return((char *) rval.addr);
#endif
        return((char *) NULL);
}


void
release_canvas_event_lock()
{
}

/*  If using the SUN console, this routine is called each time a
    character is typed.  The master actually reads the character;
    then sends it down the pipe that serves as standard input for
    the child.  The child (this process) arranges with the notifier
    to have this routine called whenver there is activity on stdin.     */

#ifdef MOTIF
static
void pipeIsReady(XtPointer  client_data, int *fd, XtInputId *id)
{
    int   n;
    char  c;
    static int badCount = 0;

    if (0 <= ioctl(*fd,FIONREAD,&n))
    {
        if (n == 0)
        {
           badCount++;
           if (badCount > 100)
           {
              pid_t ppid;
              ppid = getppid();
              if (ppid == 1)
              {
                 ignoreSigpipe();
                 nmr_exit(vnMode);
              }
           }
        }
        else
        {
           while( n > 0 )
           {
              read(*fd,&c,1);
              processChar(c);
              n -= 1;
           }
           badCount = 0;
        }
    }
}
#endif

static
void readInput(int fd)
{
    int  n;
    int  ok = 0;
    int  index;
    char *d;
    char buf[1024];

/*
#ifdef MOTIF
    if (0 <= ioctl(fd,FIONREAD,&n))
    {
        while( n > 0 )
        {
           read(fd,&c,1);
           processChar(c);
           n -= 1;
        }
    }

#else
*/
    while ( (n = read(  fd, buf, 1020)) > 0)
    {
       ok = 1;
       index = 0;
       buf[n] = '\0';
       d = buf;
       while (index < n)
       {
           processChar(*d);
           d++;
           index++;
       }
    }
    if ( ! ok) /* check if parent is still there */
    {
       int stat;
       stat = kill( getppid(), 0);  /* If java program is gone, exit */
       if (stat)
       {
          nmrExit(0,NULL,0,NULL);
       }
    }
/*
#endif
*/
}


void
set_input_func()
{
#ifdef  MOTIF
    if (xwin)
       inputId = XtAddInput(inputfd, (XtPointer) XtInputReadMask, pipeIsReady, NULL);
#endif
}

void
smagicLoop()
{
    int  res;
    fd_set  rfds;

#ifdef  MOTIF
    if (xwin) {
        XtMainLoop();
        exit(0);
    }
#endif

    while (1)
    {
       FD_ZERO( &rfds );
       FD_SET( inputfd, &rfds);
       res = select(inputfd+1, &rfds, 0, 0, 0);
       if (FD_ISSET(inputfd, &rfds) )
          readInput(inputfd);
    }
}

void
graphicsOnTop()
{ }

void
textOnTop()
{ }

int 
is_small_window()
{
    return (1);
}

void
largeWindow()
{ }

void
smallWindow()
{ }

void
save_vnmr_geom()
{ 
    save_imagefile_geom(); /* in graphics_util.c */
}

