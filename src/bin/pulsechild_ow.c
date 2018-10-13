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
 This is the child program of pulsetool.c, and controls everything
 having to do with directory listings, files, and editing of files.
 The on-line help manual is also contained in and controlled by this
 program.
************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <Xol/OpenLook.h>
#include <Xol/DrawArea.h>
#include <Xol/ControlAre.h>
#include <Xol/StaticText.h>
#include <Xol/OblongButt.h>
#include <Xol/RectButton.h>
#include <Xol/Form.h>
#include <Xol/Notice.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>
#include <Xol/Exclusives.h>
#include <Xol/TextEdit.h>
#include <Xol/ScrolledWi.h>
#include <Xol/RubberTile.h>
#include "pulsechild.h"

#define MAX_ENTRIES        800
#define MAX_ENTRY_LENGTH   100
#define CHAR_GAP 4

#ifdef sparc
#define BLINK_TIME         10
#else
#define BLINK_TIME         1
#endif sparc

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
        Widget ex, *but;
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

/*************************************************************************
*  init_window():  
*************************************************************************/
void init_window(argc,argv)
int argc;
char *argv[];
{
   void create_gc(), pipe_reader(); 
   int base_frame_event_proc();
   int n;
   Arg args[5];
 
   toplevel = OlInitialize("pulseTool","PulseTool",NULL,0,&argc,argv);
   n = 0;
   XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
   XtSetValues(toplevel,args,n);
   n = 0;
   XtSetArg(args[n], XtNheight, 1); n++;
   XtSetArg(args[n], XtNwidth, 1); n++;
   object[BASE_FRAME] = XtCreateManagedWidget("form",rubberTileWidgetClass,
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
    XtSetArg(args[n], XtNheight, &height); n++;
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
    Arg args[3];
 
    n = 0;
    XtSetArg(args[n], XtNlabel , label);  n++;
    XtSetArg(args[n], XtNtraversalOn , FALSE);  n++;
    object[which] = XtCreateManagedWidget("button",oblongButtonWidgetClass,
                        object[parent],args,n);
    XtAddCallback(object[which], XtNselect, notify_proc, NULL);
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
    Arg args[3];
 
    n = 0;
    XtSetArg(args[n], XtNlabel , label);  n++;
    XtSetArg(args[n], XtNtraversalOn , FALSE);  n++;
    object[which] = XtCreateManagedWidget("label",captionWidgetClass,
                        object[parent],args,n);
    n = 0;
    exclusives.ex = XtCreateManagedWidget("",exclusivesWidgetClass,
                        object[which],args,n);
    exclusives.but = (Widget *) malloc(sizeof(Widget)*num_strings);
    exclusives.num = num_strings;
    for (i=0;i<num_strings;i++)  {
      n = 0;
      XtSetArg(args[n], XtNlabel , strings[i]);  n++;
      exclusives.but[i] = XtCreateManagedWidget("button",rectButtonWidgetClass,
                        exclusives.ex,args,n);
      XtAddCallback(exclusives.but[i], XtNselect, notify_proc, NULL);
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
 
    n = 0;
    XtSetArg(args[n], XtNwidth, &width); n++;
    XtGetValues(object[which],args,n);
    return((int) width);
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
    XtSetArg(args[n], XtNx, (Dimension)x); n++;
    XtSetArg(args[n], XtNy, (Dimension)y); n++;
    XtSetArg(args[n], XtNwidth, (Dimension)width); n++;
    XtSetArg(args[n], XtNheight, (Dimension)height); n++;
    object[which] = XtCreateManagedWidget("panel",controlAreaWidgetClass,
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
    Arg args[7];
 
    n = 0;
    text_scroll = XtCreateManagedWidget("",scrolledWindowWidgetClass,
                        object[parent], args, n);
    XtSetArg(args[n], XtNcharsVisible, cols); n++;
    XtSetArg(args[n], XtNlinesVisible, rows); n++;
    XtSetArg(args[n], XtNeditType, OL_TEXT_READ); n++;
    object[which] = XtCreateManagedWidget("text",textEditWidgetClass,
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
   int n, argc=0;
   char *argv[2];
   Arg args[8];
 
   if (which == HELP_FRAME)  {
     n = 0;
     XtSetArg(args[n], XtNx, x); n++;
     XtSetArg(args[n], XtNy, y); n++;
     XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
     helptoplevel = XtCreateApplicationShell("pulseTool",
                        applicationShellWidgetClass, args, n);
     n = 0;
     object[which] = XtCreateManagedWidget("form",rubberTileWidgetClass,
                        helptoplevel,args,n);
     }
   if (which == FILES_FRAME)  {
     n = 0;
     XtSetArg(args[n], XtNx, x); n++;
     XtSetArg(args[n], XtNy, y); n++;
     XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
     filestoplevel = XtCreateApplicationShell("pulseTool",
                        applicationShellWidgetClass, args, n);
     n = 0;
     object[which] = XtCreateManagedWidget("form",rubberTileWidgetClass,
                        filestoplevel,args,n);
     }
   if (which == CANCEL_FRAME)  {
     n = 0;
     XtSetArg(args[n], XtNx, x); n++;
     XtSetArg(args[n], XtNy, y); n++;
     XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
     XtSetArg(args[n], XtNresizeCorners, FALSE); n++;
     canceltoplevel = XtCreateApplicationShell("pulseTool",
                        applicationShellWidgetClass, args, n);
     n = 0;
     object[which] = XtCreateManagedWidget("form",rubberTileWidgetClass,
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
  XtSetArg(args[n],XtNx, &parent_x); n++;
  XtSetArg(args[n],XtNy, &parent_y); n++;
  XtGetValues(object[parent],args,n);
  n = 0;
  XtSetArg(args[n], XtNwidth, (Dimension)width); n++;
  XtSetArg(args[n], XtNheight, (Dimension)height); n++;
  XtSetArg(args[n], XtNx, (Dimension)(x + parent_x)); n++;
  XtSetArg(args[n], XtNy, (Dimension)(y + parent_y)); n++;
  XtSetArg(args[n], XtNlayout, OL_MAXIMIZE); n++;
  object[which] = XtCreateManagedWidget("text",drawAreaWidgetClass,
                        object[parent],args,n);
   XtAddCallback(object[which], XtNexposeCallback, repaint_proc, NULL);
  XtAddEventHandler(object[which],ButtonPressMask|ConfigureNotify,
			 FALSE, event_proc, NULL);
  n=0;
  XtSetArg(args[n],XtNbackground, &back_pix); n++;
  XtSetArg(args[n],XtNforeground, &fore_pix); n++;
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
  XtSetArg(args[n],XtNyRefWidget, object[other]); n++;
  XtSetArg(args[n],XtNyAddHeight, TRUE); n++;
  XtSetArg(args[n],XtNyAttachBottom, TRUE); n++;
  XtSetValues(object[which],args,n);
}

/*****************************************************************************
*       Show object.
******************************************************************************/
void show_object(which)
{
    if (which == FILES_FRAME)
      if (!XtIsRealized(filestoplevel))
        XtRealizeWidget(filestoplevel);
      else
        XtMapWidget(filestoplevel);
    else if (which == HELP_FRAME)
      if (!XtIsRealized(helptoplevel))
        XtRealizeWidget(helptoplevel);
      else
        XtMapWidget(helptoplevel);
    else if (which == BASE_FRAME)
      if (!XtIsRealized(toplevel))
        XtRealizeWidget(toplevel);
      else
        XtMapWidget(toplevel);
    else if (which == CANCEL_FRAME)
      if (!XtIsRealized(canceltoplevel))
        XtRealizeWidget(canceltoplevel);
      else
        XtMapWidget(canceltoplevel);
    XtManageChild(object[which]);

}

/*****************************************************************************
*       Hide object.
******************************************************************************/
void hide_object(which)
{
    if (which == BASE_FRAME)
      if (!XtIsRealized(object[which]))  {}
      else
        XtUnmapWidget(toplevel);
    else if (which == HELP_FRAME)
      if (!XtIsRealized(object[which]))  {}
      else
        XtUnmapWidget(helptoplevel);
    else if (which == FILES_FRAME)
      if (!XtIsRealized(object[which]))  {}
      else
        XtUnmapWidget(filestoplevel);
    else if (which == CANCEL_FRAME)
      if (!XtIsRealized(object[which]))  {}
      else
        XtUnmapWidget(canceltoplevel);
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
      XtSetArg(args[n],XtNset, &set); n++;
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
    void init_notice(), hide_object();
    init_notice();
    XtRealizeWidget(toplevel);
    XtRealizeWidget(filestoplevel);
    XtRealizeWidget(helptoplevel);
    XtRealizeWidget(canceltoplevel);
    XChangeProperty(display, XtWindow(canceltoplevel), AtomDelete, XA_ATOM, 32,
                     PropModeAppend, &AtomHeader, 1);
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
*       Initialize notice window.
******************************************************************************/
void init_notice()
{
    int n;
    Arg args[4];
    Pixel back, focusColor = 1024;
    int    bcolor, gcolor, rcolor, cindex, gplanes;
    XColor xcolor;
    Colormap cmap;

    notice.shell = XtCreatePopupShell("notice",noticeShellWidgetClass, toplevel,
                                        NULL, 0);
    n=0;
    XtSetArg(args[n], XtNtextArea, &notice.txt);  n++;
    XtSetArg(args[n], XtNcontrolArea, &notice.ca);  n++;
    XtGetValues(notice.shell,args,n);
    n=0;
    XtSetArg(args[n], XtNtraversalOn , FALSE);  n++;
    notice.but1 = XtCreateManagedWidget("button",oblongButtonWidgetClass,
                        notice.ca, args, n);
    notice.but2 = XtCreateManagedWidget("button",oblongButtonWidgetClass,
                        notice.ca, args, n);

    gplanes = XDisplayPlanes(display, DefaultScreen(display));
    if (focusColor == 1024 && gplanes > 6)  {
      n=0;
      XtSetArg(args[n], XtNbackground, &back); n++;
      XtGetValues(notice.but1, args, n);
      cmap = XDefaultColormap(display, XDefaultScreen(display));
      xcolor.pixel = back;
      xcolor.flags = DoRed | DoGreen | DoBlue;
      XQueryColor(display, cmap, &xcolor);
      rcolor = xcolor.red >> 8;
      gcolor = xcolor.green >> 8;
      bcolor = xcolor.blue >> 8;
      if (rcolor > 120 || gcolor > 120 || bcolor > 120)
        cindex = -3;
      else
        cindex = 3;
      while(xcolor.pixel == back)  {
        if(rcolor > 3)
          rcolor += cindex;
        if(gcolor > 3)
          gcolor += cindex;
        if(bcolor > 3)
          bcolor += cindex;
        xcolor.red = rcolor << 8;
        xcolor.green = gcolor << 8;
        xcolor.blue = bcolor << 8;
        XAllocColor(display, cmap, &xcolor);
        }
      focusColor = xcolor.pixel;
      }
    if (gplanes > 6)  {
      n=0;
      XtSetArg(args[n], XtNinputFocusColor, focusColor); n++;
      XtSetValues(notice.but1, args, n);
      XtSetValues(notice.but2, args, n);
      }
    n=0;
}
 
/*****************************************************************************
*       Notice callback.
******************************************************************************/
int notice_callback(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    notice.done = TRUE;
    if (w == notice.but1)
      notice.val = 1;
    else
      notice.val = 0;
}
 
 
/*****************************************************************************
*       Pop up notice.  Don't return until we've got an answer from the
*       notice and it's gone away.
******************************************************************************/
popup_notice()
{
    int readfds, maxfds;
    XEvent event;
    Window root, child;
    int win_x, win_y, but_x, but_y, old_x, old_y;
    unsigned int k_b;
    Arg args[5];
    int n;
    Dimension x, y, width, height;
   
    XQueryPointer(display,XtWindow(toplevel),&root,&child,&old_x,&old_y,
                        &win_x,&win_y,&k_b);
    XtPopup(notice.shell, XtGrabExclusive);
/* to warp cursor to proper place (middle of notice.but1), must do some
   strange things.  Can't just warp to but1_width/2, but1_height/2 with
   warp dest_window being notice.but1.  This causes the warp to go to the
   correct position relative to the last position of the notice window (not
   the current position of the notice window).  (Don't ask me why....).
   To get the cursor to warp to the correct spot, you must have the
   dest_window be toplevel and calculate the x and y coordinates
   as shown below.  Why you have to subtract out toplevel's position, I
   don't know.  */
    n = 0;
    XtSetArg(args[n],XtNwidth,&width); n++;
    XtSetArg(args[n],XtNheight,&height); n++;
    XtGetValues(notice.but1,args,n);
    but_x = width/2; but_y = height/2;
    n = 0;
    XtSetArg(args[n],XtNx,&x); n++;
    XtSetArg(args[n],XtNy,&y); n++;
    XtGetValues(notice.ca,args,n);
    but_x += x;  but_y += y;
    n = 0;
    XtSetArg(args[n],XtNx,&x); n++;
    XtSetArg(args[n],XtNy,&y); n++;
    XtGetValues(notice.shell,args,n);
    but_x += x;  but_y += y;
    n = 0;
    XtSetArg(args[n],XtNx,&x); n++;
    XtSetArg(args[n],XtNy,&y); n++;
    XtGetValues(toplevel,args,n);
    but_x -= x;  but_y -= y;
    XWarpPointer(display,None,XtWindow(toplevel),0,0,0,0,but_x,but_y);
 
    notice.done = FALSE;
    while (!notice.done)  {
      if (XtPending())  {
        XtNextEvent(&event);
        XtDispatchEvent(&event);
        }
      else  {
        readfds = 0;
        maxfds = 1 + ConnectionNumber(XtDisplay(toplevel));
        readfds = 1 << ConnectionNumber(XtDisplay(toplevel));
        if (select(maxfds, &readfds, NULL, NULL, NULL) == -1)  {
          if (EINTR != errno)
            exit(1);
          }
        }
      }
    n = 0;
    XtSetArg(args[n],XtNx,&x); n++;
    XtSetArg(args[n],XtNy,&y); n++;
    XtGetValues(toplevel,args,n);
    old_x -= x;  old_y -= y;
    XWarpPointer(display,None,XtWindow(toplevel),0,0,0,0,old_x,old_y);
}

 
/*****************************************************************************
*       Pop up confirm box with "Yes" or "No" options.
******************************************************************************/
int notice_yn(which,str)
int which;
char *str;
{
    int n;
    Arg args[4];
 
    n = 0;
    XtSetArg(args[n], XtNemanateWidget, object[which]); n++;
    XtSetValues(notice.shell, args, n);
    n = 0;
    XtSetArg(args[n], XtNstring, str); n++;
    XtSetValues(notice.txt, args, n);
    n = 0;
    XtSetArg(args[n], XtNlabel, "Yes"); n++;
    XtSetValues(notice.but1, args, n);
    n = 0;
    XtSetArg(args[n], XtNlabel, "No"); n++;
    XtSetValues(notice.but2, args, n);
    XtAddCallback(notice.but1, XtNselect, notice_callback, NULL);
    XtAddCallback(notice.but2, XtNselect, notice_callback, NULL);
    popup_notice();
 
    return(notice.val);
}
 
/*****************************************************************************
*       Pop up informational box.
******************************************************************************/
void notice_prompt_ok(which,str)
int which;
char *str;
{
    int n;
    Arg args[4];
 
    n = 0;
    XtSetArg(args[n], XtNemanateWidget, object[which]); n++;
    XtSetValues(notice.shell, args, n);
    n = 0;
    XtSetArg(args[n], XtNstring, str); n++;
    XtSetValues(notice.txt, args, n);
    n = 0;
    XtSetArg(args[n], XtNlabel, "Ok"); n++;
    XtSetValues(notice.but1, args, n);
    XtUnmanageChild(notice.but2);
    XtAddCallback(notice.but1, XtNselect, notice_callback, NULL);
    XtAddCallback(notice.but2, XtNselect, notice_callback, NULL);
    popup_notice();
    XtManageChild(notice.but2);
}

/*****************************************************************
*  This routine reads data coming from the parent PulseTool,
*  in 512 byte blocks, taking the appropriate action based on
*  the string received from a successful read.
*****************************************************************/
static void
pipe_reader(client_data, fid, id)
XtPointer *client_data;
int   *fid;
XtInputId *id;
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
XtPointer client_data;
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
XtPointer client_data, call_data;
{
    int   obj, choice;

    obj = find_object(w);
    choice = (int)get_panel_value(HELP_CHOICE_BUTTONS);
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

    OlTextEditUpdate(object[which],FALSE);
    OlTextEditClearBuffer(object[which]);
    for (i=0;i<num;i++)
      OlTextEditInsert(object[which],txt[i],strlen(txt[i]));
    n = 0;
    XtSetArg(args[n], XtNdisplayPosition,0); n++;
    XtSetValues(object[which], args, n);
    OlTextEditUpdate(object[which],TRUE);
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
XtPointer client_data, call_data;
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
XtPointer client_data, call_data;
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
XtPointer client_data;
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
