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

/************************************************************************
 This is the child program of pulsetool.c.
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#ifndef SOLARIS
#include <sys/dir.h>
#endif
#ifdef AIX
#include <sys/select.h>
#endif
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <Xm/BulletinB.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/MessageB.h>
#include <Xm/ToggleB.h>
/**
#include <Xm/VendorE.h>
**/
#include <Xm/MwmUtil.h>
#include "pulsechild.h"

#define MAX_ENTRIES        800
#define MAX_ENTRY_LENGTH   100
#define CHAR_GAP 4

#ifdef sparc
#define BLINK_TIME         10
#else
#define BLINK_TIME         1
#endif

#define FILE_WIN_ROWS	15   /* number of text rows in the file browser win */

static Widget		object[NUM_OBJECTS];
static Widget		toplevel, helptoplevel, filestoplevel, canceltoplevel,
			text_scroll;

typedef struct {
        Widget shell, txt, ca, but1, but2;
        int val, done;
        } notice_struct;
static notice_struct    notice;

typedef struct {
        Widget label, radio_rc, *but;
	int num;
	} exclusives_struct;
static exclusives_struct exclusives;

static XtInputId	input_des;
 
static GC		gc;
static XFontStruct	*font;
static Atom		AtomHeader;
static Atom		AtomDelete;

Display			*display;	/* X11 ID of display */
int			depth,		/* pixel depth for display */
			screen;		/* screen number */
char			*display_name=NULL;/* name of display */

extern int              char_width,
                        char_ascent,
                        char_descent,
                        char_height;


typedef struct {
        unsigned char red, green, blue;
        } color_struct;
 
static Pixel   fore_pix;
static Pixel   back_pix;

static XmStringCharSet  charset = (XmStringCharSet) XmSTRING_DEFAULT_CHARSET;
static void pipe_reader(caddr_t *client_data, int *fid, XtInputId *id);
void create_gc();

/*************************************************************************
*  init_window():  
*************************************************************************/
void init_window(argc,argv)
int argc;
char *argv[];
{
   int base_frame_event_proc();
   int n;
   Arg args[5];
 
   n = 0;
   XtSetArg(args[n], XtNallowShellResize, True); n++;
   toplevel = XtInitialize("pulseTool","PulseTool",args,n,&argc,argv);
   n = 0;
   XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
   XtSetValues(toplevel,args,n);
   n = 0;
   XtSetArg(args[n], XtNheight, 1); n++;
   XtSetArg(args[n], XtNwidth, 1); n++;
   object[BASE_FRAME] = XtCreateManagedWidget("form",xmFormWidgetClass,
                        toplevel,args,n);
   XtAddEventHandler(object[BASE_FRAME],ConfigureNotify,FALSE,
			base_frame_event_proc, NULL);
   if (!(display = XtDisplay(toplevel)))  {
        (void)fprintf(stderr,"Cannot connect to Xserver\n");
        exit(-1);
        }
   screen = DefaultScreen(display);
   depth = DefaultDepth(display,screen);
   input_des = XtAddInput(0, XtInputReadMask,
                        pipe_reader, NULL);
}

/*****************************************************************************
*       Set up font and return character string with font name
******************************************************************************/
void setup_font(font_name)
char **font_name;
{
    int i;
    char *str;
    static XtResource resources[] =  {
        { "font","Font",XtRString, sizeof(String),
                 0, XtRString, "8x16"}};
 
    XtGetSubresources(toplevel,&str,"canvas","DrawArea",resources,
                        XtNumber(resources), NULL,0);
    font = XLoadQueryFont(display,str);
    if (!font)  {
      str = "8x16";
      font = XLoadQueryFont(display,str);
      }
    if (font)  {
      *font_name = (char *)malloc(sizeof(char)*(strlen(str)+1));
      strcpy(*font_name,str);
      }
    char_width = font->max_bounds.width;
    char_ascent = font->max_bounds.ascent;
    char_descent = font->max_bounds.descent;
    char_height = char_ascent+char_descent;
    create_gc();
}

int get_canvas_width(which)
int which;
{
    return(object_width(which));
}

int get_canvas_height(which)
int which;
{
    int n;
    Arg args[2];
    Dimension height;

    n = 0;
    XtSetArg(args[n], XmNheight, &height); n++;
    XtGetValues(object[which],args,n);
    return((int) height);
}

/*****************************************************************************
*       Set up colormap
******************************************************************************/
void setup_colormap()
{
}

/*****************************************************************************
*       Create gc 
******************************************************************************/
void create_gc()
{
    unsigned long  valuemask;
    XGCValues values;
 
    valuemask = GCBackground|GCForeground|GCFont;
    values.foreground = WhitePixel(display,screen);
    values.background = BlackPixel(display,screen);
    values.font = font->fid;
    gc = XCreateGC(display,DefaultRootWindow(display),valuemask, &values);
}

/*****************************************************************************
*       Create button at (x,y) with label and notify proc attached.
******************************************************************************/
void create_button(which,parent,x,y,label,notify_proc)
int which, parent, x, y;
char *label;
int (*notify_proc)();
{
    int n;
    Arg args[5];
 
    XmString string;
 
    n = 0;
    string = XmStringCreate(label,charset);
    XtSetArg(args[n], XmNlabelString , string);  n++;
    XtSetArg(args[n], XmNtraversalOn , False);  n++;
    XtSetArg(args[n], XmNmarginTop , (Dimension) 3);  n++;
    XtSetArg(args[n], XmNmarginBottom , (Dimension) 3);  n++;
    object[which] = XtCreateManagedWidget("button",xmPushButtonWidgetClass,
                        object[parent],args,n);
    XmStringFree(string);
    if (notify_proc != NULL)
      XtAddCallback(object[which], XmNactivateCallback, notify_proc, NULL);
}

/*****************************************************************************
*       Create choice buttons at (x,y) with label strings and notify_proc
*	attached.
******************************************************************************/
void create_choice_button(which,parent,x,y,label,num_strings,strings,
                        notify_proc)
int which,parent;
int x, y;
char *label;
int num_strings;
char *strings[];
int (*notify_proc)();
{
    int n, i;
    Arg args[5];
    XmString string;
 
    n = 0;
    XtSetArg(args[n], XmNtraversalOn , FALSE);  n++;
    XtSetArg(args[n], XmNorientation , XmHORIZONTAL);  n++;
    object[which] = XtCreateManagedWidget("form",xmRowColumnWidgetClass,
                        object[parent],args,n);
    string = XmStringCreate(label,charset);
    n = 0;
    XtSetArg(args[n], XmNlabelString , string);  n++;
    exclusives.label = XtCreateManagedWidget("label",xmLabelWidgetClass,
                        object[which],args,n);
    XmStringFree(string);
    n = 0;
    XtSetArg(args[n], XmNtraversalOn , FALSE);  n++;
    XtSetArg(args[n], XmNorientation , XmHORIZONTAL);  n++;
    XtSetArg(args[n], XmNradioBehavior , True);  n++;
    XtSetArg(args[n], XmNradioAlwaysOne , True);  n++;
    exclusives.radio_rc = XtCreateManagedWidget("form",xmRowColumnWidgetClass,
                        object[which],args,n);
    n = 0;
    exclusives.but = (Widget *) malloc(sizeof(Widget)*num_strings);
    exclusives.num = num_strings;
    for (i=0;i<num_strings;i++)  {
      string = XmStringCreate(strings[i],charset);
      n = 0;
      XtSetArg(args[n], XmNlabelString , string);  n++;
      XtSetArg(args[n], XmNtraversalOn , False);  n++;
      if (i == 0)  {
        XtSetArg(args[n], XmNset , True);  n++;
	}
      exclusives.but[i] = XtCreateManagedWidget("button",
		xmToggleButtonWidgetClass,exclusives.radio_rc,args,n);
      XtAddCallback(exclusives.but[i], XmNarmCallback, notify_proc, NULL);
      }
}

/*****************************************************************************
*       Given column number, return pixel value.
******************************************************************************/
int column(which,col)
int which, col;
{
    return(0);
}

/*****************************************************************************
*       Given row number, return pixel value.
******************************************************************************/
int row(which,r)
int which, r;
{
    return(0);
}

/*****************************************************************************
*       Return width of XView object.
******************************************************************************/
int object_width(which)
int which;
{
    int n;
    Arg args[2];
    Dimension width;
 
    if (XtIsRealized(object[which]))  {
      n = 0;
      XtSetArg(args[n], XmNwidth, &width); n++;
      XtGetValues(object[which],args,n);
      return((int) width);
      }
    else
      return(0);
}

/*****************************************************************************
*       Create panel area at (x,y) of size width by height.
******************************************************************************/
void create_panel(which,parent,x,y,width,height)
int which,parent;
int x, y, width, height;
{
    int n;
    Arg args[7];
 
    n = 0;
    XtSetArg(args[n], XmNx, (Dimension)x); n++;
    XtSetArg(args[n], XmNy, (Dimension)y); n++;
    XtSetArg(args[n], XmNwidth, (Dimension)width); n++;
    XtSetArg(args[n], XmNheight, (Dimension)height); n++;
    XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
    object[which] = XtCreateManagedWidget("panel",xmRowColumnWidgetClass,
                        object[parent], args, n);
}

void null_proc()
{
}

/*****************************************************************************
*       Create textsw
******************************************************************************/
void create_sw(which,parent,rows,cols,filename)
int which, parent;
int rows, cols;
char *filename;
{
    int n;
    Arg args[8];
 
    n = 0;
    XtSetArg(args[n], XmNscrollingPolicy, XmAPPLICATION_DEFINED); n++;
    text_scroll = XtCreateManagedWidget("",xmScrolledWindowWidgetClass,
                        object[parent], args, n);
    n = 0;
    XtSetArg(args[n], XmNcolumns, cols); n++;
    XtSetArg(args[n], XmNrows, rows); n++;
    XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
    XtSetArg(args[n], XmNscrollHorizontal, False); n++;
    XtSetArg(args[n], XmNscrollVertical, True); n++;
    XtSetArg(args[n], XmNeditable, False); n++;
    XtSetArg(args[n], XmNautoShowCursorPosition, False); n++;
    object[which] = XtCreateManagedWidget("text",xmTextWidgetClass,
                        text_scroll, args, n);
}

/*****************************************************************************
*       Create frame at (x,y) of size width by height.
******************************************************************************/
void create_frame(which,parent,label,x,y,width,height)
int which, parent;
char *label;
int x, y, width, height;
{
   int n;
   Arg args[9];
 
   if (which == HELP_FRAME)  {
     n = 0;
     XtSetArg(args[n], XmNx, x); n++;
     XtSetArg(args[n], XmNy, y); n++;
     XtSetArg(args[n], XtNtitle, "PulseTool Help"); n++;
     XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
     XtSetArg(args[n], XtNallowShellResize, True); n++;
     helptoplevel = XtCreatePopupShell("pulseTool",
                        transientShellWidgetClass, toplevel, args, n);
     n = 0;
     object[which] = XtCreateManagedWidget("form",xmRowColumnWidgetClass,
                        helptoplevel,args,n);
     }
   if (which == FILES_FRAME)  {
     n = 0;
     XtSetArg(args[n], XmNx, x); n++;
     XtSetArg(args[n], XmNy, y); n++;
     XtSetArg(args[n], XtNtitle, "PulseTool Files"); n++;
     XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
     XtSetArg(args[n], XtNallowShellResize, True); n++;
     filestoplevel = XtCreatePopupShell("pulseTool",
                        transientShellWidgetClass, toplevel, args, n);
     n = 0;
     object[which] = XtCreateManagedWidget("form",xmFormWidgetClass,
                        filestoplevel,args,n);
     }
   if (which == CANCEL_FRAME)  {
     n = 0;
     XtSetArg(args[n], XmNx, x); n++;
     XtSetArg(args[n], XmNy, y); n++;
     XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
     XtSetArg(args[n], XmNmwmDecorations, MWM_DECOR_BORDER); n++;
     canceltoplevel = XtCreatePopupShell("pulseTool",
                        transientShellWidgetClass, toplevel, args, n);
     n = 0;
     object[which] = XtCreateManagedWidget("form",xmRowColumnWidgetClass,
                        canceltoplevel,args,n);
     AtomHeader = XInternAtom(display, "_OL_DECOR_HEADER" , False);
     AtomDelete = XInternAtom(display, "_OL_DECOR_DEL" , False);
     }
}

/*****************************************************************************
*       Create frame at (x,y) of size width by height.
******************************************************************************/
void create_frame_with_event_proc(which,parent,label,x,y,width,height,ev_proc)
int which, parent;
char *label;
int x, y, width, height;
int (*ev_proc)();
{
/*   int n, argc=0;
   char *argv[2];
   Arg args[8];
 
     n = 0;
     XtSetArg(args[n], XtNx, x); n++;
     XtSetArg(args[n], XtNy, y); n++;
     canceltoplevel = XtCreateApplicationShell("pulseTool",
                        applicationShellWidgetClass, args, n);
     n = 0;
     object[which] = XtCreateManagedWidget("form",formWidgetClass,
                        canceltoplevel,args,n);*/
}

/*****************************************************************************
*       Create canvas at (x,y) of size width by height.  Attach an event
*       handler and a repaint process.
******************************************************************************/
void create_canvas(which,parent,x,y,width,height,repaint_proc,event_proc)
int which, parent, x, y, width, height;
int (*repaint_proc)(), (*event_proc)();
{
  int n;
  Dimension  parent_x, parent_y, parent_width, parent_height;
  Arg args[9];
 
  n = 0;
  XtSetArg(args[n],XmNx, &parent_x); n++;
  XtSetArg(args[n],XmNy, &parent_y); n++;
  XtGetValues(object[parent],args,n);
  n = 0;
  XtSetArg(args[n], XmNwidth, (Dimension)width); n++;
  XtSetArg(args[n], XmNheight, (Dimension)height); n++;
  XtSetArg(args[n], XmNx, (Dimension)(x + parent_x)); n++;
  XtSetArg(args[n], XmNy, (Dimension)(y + parent_y)); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  object[which] = XtCreateManagedWidget("text",xmDrawingAreaWidgetClass,
                        object[parent],args,n);
  XtAddCallback(object[which], XmNexposeCallback, repaint_proc, NULL);
  XtAddEventHandler(object[which],ButtonPressMask|ConfigureNotify,
			 FALSE, event_proc, NULL);
  n=0;
  XtSetArg(args[n],XmNbackground, &back_pix); n++;
  XtSetArg(args[n],XmNforeground, &fore_pix); n++;
  XtGetValues(object[which],args,n);
}

/*****************************************************************************
*       Set object which below other.
******************************************************************************/
void set_win_below(which, other)
int which, other;
{
  int n;
  Arg args[4];
 
  n = 0;
  XtSetArg(args[n],XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNtopWidget, object[other]); n++;
  XtSetValues(object[which],args,n);
}

/*****************************************************************************
*       Show object.
******************************************************************************/
void show_object(which)
{
    if (which == FILES_FRAME)  {
      if (!XtIsRealized(filestoplevel))
        XtRealizeWidget(filestoplevel);
      XtPopup(filestoplevel,XtGrabNone);
      }
    else if (which == HELP_FRAME)  {
      if (!XtIsRealized(helptoplevel))
        XtRealizeWidget(helptoplevel);
      XtPopup(helptoplevel,XtGrabNone);
      }
    else if (which == BASE_FRAME)  {
      if (!XtIsRealized(toplevel))
        XtRealizeWidget(toplevel);
      else
        XtMapWidget(toplevel);
      }
    else if (which == CANCEL_FRAME)  {
      if (!XtIsRealized(canceltoplevel))
        XtRealizeWidget(canceltoplevel);
      XtPopup(canceltoplevel,XtGrabNone);
      }
    else 
      XtManageChild(object[which]);

}

/*****************************************************************************
*       Hide object.
******************************************************************************/
void hide_object(which)
{
    if (which == BASE_FRAME)  {
      if (!XtIsRealized(object[which]))  {}
      else
        XtUnmapWidget(toplevel);
      }
    else if (which == HELP_FRAME)  {
      if (XtIsRealized(object[which]))
        XtPopdown(helptoplevel);
      }
    else if (which == FILES_FRAME)  {
      if (XtIsRealized(object[which]))
        XtPopdown(filestoplevel);
      }
    else if (which == CANCEL_FRAME)  {
      if (XtIsRealized(object[which]))
        XtPopdown(canceltoplevel);
      }
    else
      XtUnmanageChild(object[which]);
}

/*****************************************************************************
*       Hide frame label.
******************************************************************************/
void hide_frame_label(which)
{
}

/*****************************************************************************
*       Get panel value.
******************************************************************************/
win_val get_panel_value(which)
int which;
{
  int n, i;
  Arg args[2];
  Boolean set;
 
  if (which == HELP_CHOICE_BUTTONS)  {
    set = FALSE; i = 0;
    while (!set && i < exclusives.num)  {
      n = 0;
      XtSetArg(args[n],XmNset, &set); n++;
      XtGetValues(exclusives.but[i],args,n);
      if (!set)
	i++;
      }
    }
  return((win_val)i);
}

/*****************************************************************************
*       Given a window object, find it's index in "objects" array.
******************************************************************************/
int find_object(item)
Widget item;
{
    int i;

    i = 0;
    while (i<exclusives.num)  {
      if (exclusives.but[i] == item)
        return(HELP_CHOICE_BUTTONS);
      i++;
      }
    i = 0;
    while (i<NUM_OBJECTS)  {
      if (object[i] == item)
        return(i);
      i++;
      }
    return(-1);  /* error */
}

/*****************************************************************************
*       Do frame fit operation to resize frame to fit contents.
******************************************************************************/
void fit_frame(which)
int which;
{
}

/*****************************************************************************
*       Do frame fit operation to resize frame to fit contents in vertical
*       direction.
******************************************************************************/
void fit_height(which)
int which;
{
}

/*****************************************************************************
*       loop on BASE_FRAME.
******************************************************************************/
void window_start_loop(which)
int which;
{
    int init_notice();
    void hide_object();

    XtRealizeWidget(toplevel);
    XtRealizeWidget(filestoplevel);
    XtRealizeWidget(helptoplevel);
    XtRealizeWidget(canceltoplevel);
    XChangeProperty(display, XtWindow(canceltoplevel), AtomDelete, XA_ATOM, 32,
                     PropModeAppend, &AtomHeader, 1);
    init_notice();
    XtMainLoop();
}

/*****************************************************************************
*       Tell window system to do all pending draw requests NOW.
******************************************************************************/
void flush_graphics()
{
    XFlush(display);
}

/*****************************************************************************
*	Initialize notice window.
******************************************************************************/
int init_notice()
{
    int n;
    Arg args[8];
    Widget help;
    int notice_yes_callback(), notice_no_callback();
 
    n=0;
    XtSetArg(args[n], XmNdialogStyle, XmDIALOG_SYSTEM_MODAL);  n++;
    XtSetArg(args[n], XmNtraversalOn, False);  n++;
    notice.shell = XmCreateErrorDialog(filestoplevel,"Error",args,n);
    help = XmMessageBoxGetChild(notice.shell,XmDIALOG_HELP_BUTTON);
    XtUnmanageChild(help);
    notice.but1 = XmMessageBoxGetChild(notice.shell,XmDIALOG_OK_BUTTON);
    notice.but2 = XmMessageBoxGetChild(notice.shell,XmDIALOG_CANCEL_BUTTON);
    XtAddCallback(notice.but1,XmNactivateCallback,notice_yes_callback,NULL);
    XtAddCallback(notice.but2,XmNactivateCallback,notice_no_callback,NULL);
    n=0;
    XtSetArg(args[n], XmNtraversalOn, False);  n++;
    XtSetValues(notice.but1,args,n);
    XtSetValues(notice.but2,args,n);
    notice.done = FALSE;
    notice.val = 0;
}

/*****************************************************************************
*	Notice callback.
******************************************************************************/
int notice_yes_callback(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    notice.done = TRUE;
    notice.val = 1;
}

/*****************************************************************************
*	Notice callback.
******************************************************************************/
int notice_no_callback(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    notice.done = TRUE;
    notice.val = 0;
}

/*****************************************************************************
*	Pop up notice.  Don't return until we've got an answer from the
*	notice and it's gone away.
******************************************************************************/
popup_notice()
{
    int maxfds;
    fd_set readfds;
    XEvent event;

    XtManageChild(notice.shell);

    notice.done = FALSE;
    while (!notice.done)  {
      if (XtPending())  {
	XtNextEvent(&event);
	XtDispatchEvent(&event);
        }
      else  {
	maxfds = 1 + ConnectionNumber(XtDisplay(toplevel));
        FD_ZERO(&readfds);
        FD_SET(ConnectionNumber(XtDisplay(toplevel)),&readfds);
	if (select(maxfds, &readfds, NULL, NULL, NULL) == -1)  {
	  if (EINTR != errno)
	    exit(1);
	  }
	}
      }
}

/*****************************************************************************
*	Pop up informational box.
******************************************************************************/
void notice_prompt_ok(which,str)
int which;
char *str;
{
    XmString string, ok_string;
    int n;
    Dimension x,y;
    Arg args[6];
    Widget cancel;

    string = XmStringCreate(str,charset);
    ok_string = XmStringCreate("Ok",charset);
    XtUnmanageChild(notice.but2);
    n=0;
    XtSetArg(args[n], XmNmessageString, string); n++;
    XtSetArg(args[n], XmNokLabelString, ok_string); n++;
    XtSetArg(args[n], XmNdefaultButtonType, XmDIALOG_OK_BUTTON); n++;
    XtSetArg(args[n], XmNdefaultButton, notice.but1); n++;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_ERROR); n++;
    XtSetValues(notice.shell, args, n);
    popup_notice();
}


/*****************************************************************
*  This routine reads data coming from the parent PulseTool,
*  in 512 byte blocks, taking the appropriate action based on
*  the string received from a successful read.
*****************************************************************/
static void pipe_reader(caddr_t *client_data, int *fid, XtInputId *id)
{
    char  buf[512];
    int   nread;

    nread = read(*fid, buf, 512);
    if (nread > 0)
	buf[nread] = '\0';
    switch(nread) {
	case -1:
	    perror("(child)pipe_read: read");
	    exit(1);
        case  0:
	    perror("(child)pipe_read: read(EOF)");
	    exit(1);
        default:
	    do_pipe_reader(buf);
	}
}

/*****************************************************************************
*	process events on base frame - just hide it when the program starts 
*****************************************************************************/
int base_frame_event_proc(w, client_data, event)
Widget w;
caddr_t client_data;
XEvent *event;
{
}

/*****************************************************************
*  This routine displays a window containing help information
*  about all of the PulseTool capabilities.  It is initially
*  called from pipe_reader, and then by either the "Done" button
*  in the help window, or by one of the help choices in the 
*  window.  Each time a new help selection is made, the old 
*  message items used to display the previous text are destroyed,
*  and new ones created for the new text.  Each block of text
*  must end with the line "END".
*****************************************************************/
int help_proc(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    int   obj, choice, found;

    obj = find_object(w);
    choice = 0;  found = FALSE;
    while ((!found) && (choice < exclusives.num))  {
      if (w == exclusives.but[choice])
	found = TRUE;
      else
	choice++;
      }
/*    choice = (int)get_panel_value(HELP_CHOICE_BUTTONS);*/
    do_help_proc(obj,choice);
}

/********************************************************************
*	show text for new help catagory
********************************************************************/
void show_text(which,num,txt)
int which, num;
char *txt[];
{
    int i, n;
    Arg args[4];
    XmTextPosition first, last;

    first = 0;
    last = XmTextGetLastPosition(object[which]);
    XmTextSetSelection(object[which],first,last,CurrentTime);
    XmTextRemove(object[which]);
    for (i=0;i<num;i++)
      XmTextInsert(object[which],XmTextGetLastPosition(object[which]),txt[i]);
    XmTextSetTopCharacter(object[which],0);
}

/*****************************************************************
*	kills all windows
*****************************************************************/
void quit_proc()
{
    XtRemoveInput(input_des);
    exit(0);
}

/*****************************************************************
*  This routine is called by the "Done", "Chdir", "Parent",
*  and "Load" buttons, and also when the "Directory" and
*  "Pulse Name" text items are modified in the parent PulseTool.
*  In the case of the first three, all of the PANEL_TEXT items
*  are first destroyed, so that the next directory listing does
*  not continue to create more and more items.
*****************************************************************/
void file_control(w, client_data, call_data)
Widget   w;
caddr_t client_data, call_data;
{
    do_file_control(find_object(w));
}

/************************************************************************
*  Display the directory listing in the "files_panel" popup.
*  Each time this routine is called, "entry_counter" filenames
*  are written row by row to a canvas.  If all rows don't fit
*  on one screen, allows paging forward or back to see all filenames.
*************************************************************************/
int display_directory_list(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    do_display_directory_list();
}

/****************************************************************************
*  Process events in the file browser window.  Only events we need to consider
*  are configuration events and mouse down events.  
*  Call display_directory_list() to redraw the window.
*****************************************************************************/

int directory_event_proc(w, client_data, event)
Widget w;
caddr_t client_data;
XEvent *event;
{
    int button_down = FALSE,
	resize = FALSE,
 	x = 0, y = 0;

    if (event->type==ConfigureNotify)  {
      resize = TRUE;
      }
    if (event->type==ButtonPress)  {
      if (event->xbutton.button==Button1)
        button_down = True;
	x = event->xbutton.x;
	y = event->xbutton.y;
	}
    do_directory_event_proc(button_down, x, y, resize);
}

/****************************************************************************
*   Write a string to a canvas
*****************************************************************************/

void draw_string(which, x, y, str, color)
int which, x, y;
char *str;
int color;
{
    int width;

    width = XTextWidth(font,str,strlen(str));
    XSetForeground(display, gc, back_pix);
    XFillRectangle(display, XtWindow(object[which]), gc, x, y - char_ascent-1, width,
                        char_height + 2);
    XSetForeground(display, gc, fore_pix);
    XDrawString(display, XtWindow(object[which]), gc, x, y, str, strlen(str));
}

/****************************************************************************
*   Write a string to a canvas in reverse video
*****************************************************************************/

void draw_inverse_string(which, x, y, str, color)
int which, x, y;
char *str;
int color;
{
    int width;

    width = XTextWidth(font,str,strlen(str));
    XSetForeground(display, gc, fore_pix);
    XFillRectangle(display, XtWindow(object[which]), gc, x, y - char_ascent - 1,
                        width,char_height + 2);
    XSetForeground(display, gc, back_pix);
    XDrawString(display, XtWindow(object[which]), gc, x, y, str, strlen(str));
}

/****************************************************************************
*   Get width of a string
*****************************************************************************/

int get_string_width(str)
char *str;
{
    int width;
    width=XTextWidth(font,str,strlen(str));
    return(width);
}

/****************************************************************************

*   Clear area of a canvas
*****************************************************************************/

void clear_area(which, x1, y1, x2, y2)
int which, x1, y1, x2, y2;
{
    XSetForeground(display, gc, back_pix);
    XFillRectangle(display, XtWindow(object[which]), gc, x1, y1, x2, y2);
}

/****************************************************************************
*   Clear entire canvas
*****************************************************************************/

void clear_window(which)
int which;
{
    XClearWindow(display,XtWindow(object[which]));
}
