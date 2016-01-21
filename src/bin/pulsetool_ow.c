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
************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <sys/signal.h>
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
#include <Xol/MenuButton.h>
#include <Xol/AbbrevMenu.h>
#include <Xol/PopupWindo.h>
#include <Xol/Form.h>
#include <Xol/Notice.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>
#include <Xol/RubberTile.h>
#include "pulsetool.h"

#define NUM_COLORS         32         /* color map size */
#define MAX_SEG_SIZE       250

typedef struct {
	unsigned char red, green, blue;
	} color_struct;

static color_struct   canvas_colors[NUM_COLORS] = {{  0,  0,  0},
                                                {255,  0,  0},
                                                {255,255,255},
                                                {230,  0,170},
                                                {190, 50,230},
                                                {150, 80,250},
                                                { 80,110,250},
                                                {  0,140,250},
                                                {  0,255,255},
                                                {255,180,  0},
                                                {255,255,  0},
                                                {255,  0,255},
                                                {  0,200,255},
                                                {  0,255,  0},
                                                {  0,  0,255},
                                                {  0,  0,255},
                                                {  0, 95,  0},
                                                {  0,105,  0},
                                                {  0,115,  0},
                                                {  0,125,  0},
                                                {  0,135,  0},
                                                {  0,145,  0},
                                                {  0,155,  0},
                                                {  0,165,  0},
                                                {  0,175,  0},
                                                {  0,185,  0},
                                                {  0,195,  0},
                                                {  0,210,  0},
                                                {  0,225,  0},
                                                {  0,240,  0},
                                                {  0,255,  0},
                                                {  0,  0,  0} };
static Colormap colormap;
static XColor	colors[NUM_COLORS];
static unsigned long sun_colors[NUM_COLORS];
static int gplanes;
static Pixel focusColor = 1024;
static Colormap cmap;

static Display          *display;
static int              *display_name=NULL;
static int		depth, screen;
static int		xor_flag=FALSE;

static unsigned char icon_data[] = {
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0x7f,0xfe,0xff,0xff,0xff,
0xff,0xff,0xff,0x3f,0xfd,0xff,0xff,0xff,
0xff,0xff,0xff,0xbf,0xfd,0xff,0xff,0xff,
0xff,0xff,0xff,0x5f,0xfb,0xff,0xff,0xff,
0xff,0xff,0xff,0xdf,0xf9,0xff,0xff,0xff,
0xff,0xff,0xff,0x6f,0xf7,0xff,0xff,0xff,
0x0f,0x00,0x00,0x00,0xf4,0xff,0xff,0xff,
0xf7,0xff,0xff,0xff,0xf3,0xff,0xff,0xff,
0xfb,0xff,0xff,0xff,0xe7,0xff,0xff,0xff,
0x1b,0xb7,0xfd,0x1c,0xe6,0xff,0xff,0xff,
0xdb,0xb6,0x7d,0xdb,0xe7,0xff,0xff,0xff,
0xdb,0xb6,0xfd,0x1e,0xe7,0xff,0xff,0xff,
0x1b,0xb7,0xfd,0xdd,0xd7,0xff,0xff,0xff,
0xdb,0xb7,0x7d,0xdb,0xd7,0xff,0xff,0xff,
0xdb,0xcf,0xe1,0x1c,0xd6,0xff,0xff,0xff,
0xfb,0xff,0xff,0xff,0x97,0xff,0xff,0xff,
0xf7,0xff,0xff,0xff,0xb3,0xff,0xff,0xff,
0x0f,0x00,0x00,0x00,0x9c,0xff,0xff,0xff,
0xff,0xff,0xff,0x75,0xb7,0xff,0xff,0xff,
0xff,0xff,0xff,0xdc,0x5d,0xff,0xff,0xff,
0xff,0xff,0xfc,0x76,0x77,0x3f,0xff,0xff,
0xff,0x7f,0xf9,0xdc,0x5d,0xdf,0xfe,0xff,
0xff,0x3f,0xfb,0x76,0x77,0x5f,0xfd,0xff,
0x1f,0xbf,0x75,0xdd,0xdd,0xce,0xfd,0xf8,
0xcf,0x5e,0x77,0x77,0x77,0x6e,0x7b,0xf7,
0x77,0xdd,0x75,0xdd,0xdd,0xce,0xb9,0xed,
0xdb,0x6d,0x27,0x77,0x77,0x75,0x37,0xd7,
0x73,0xc9,0xad,0xdd,0xdd,0xd5,0xd5,0xdd,
0xdd,0x75,0x57,0x77,0x77,0x71,0x67,0xb7,
0x75,0xd7,0xdd,0xdd,0xdd,0xd1,0xcd,0x9d,
0xdb,0x69,0x27,0x77,0x77,0x75,0x57,0xd7,
0x73,0xcd,0xad,0xdd,0xdd,0xd5,0xb5,0xdd,
0xd7,0x5d,0x77,0x77,0x77,0x6e,0x3b,0xe7,
0x6f,0xde,0x75,0xdd,0xdd,0xce,0x79,0xf5,
0x1f,0x3f,0x77,0x77,0x77,0x6e,0xfd,0xf8,
0xff,0xbf,0xf9,0xdc,0x5d,0xdf,0xfd,0xff,
0xff,0x7f,0xfb,0x76,0x77,0x5f,0xfe,0xff,
0xff,0xff,0xfc,0xdc,0x5d,0x3f,0xff,0xff,
0xff,0xff,0xff,0x76,0x77,0xff,0xff,0xff,
0xff,0xff,0xff,0xdd,0x9d,0xff,0xff,0xff,
0xff,0xff,0xff,0x75,0x07,0x00,0x00,0xf0,
0xff,0xff,0xff,0xdd,0xf5,0xff,0xff,0xef,
0xff,0xff,0xff,0x75,0xfb,0xff,0xff,0xdf,
0xff,0xff,0xff,0xdb,0x19,0xcc,0x73,0xdf,
0xff,0xff,0xff,0x73,0x7b,0xb7,0x6d,0xdf,
0xff,0xff,0xff,0xdb,0x79,0xb7,0x6d,0xdf,
0xff,0xff,0xff,0x77,0x7b,0xb7,0x6d,0xdf,
0xff,0xff,0xff,0xd7,0x79,0xb7,0x6d,0xdf,
0xff,0xff,0xff,0x77,0x7b,0xcf,0x73,0xd8,
0xff,0xff,0xff,0xd7,0xf9,0xff,0xff,0xdf,
0xff,0xff,0xff,0x6f,0xf7,0xff,0xff,0xef,
0xff,0xff,0xff,0xcf,0x0d,0x00,0x00,0xf0,
0xff,0xff,0xff,0x6f,0xf7,0xff,0xff,0xff,
0xff,0xff,0xff,0xdf,0xf9,0xff,0xff,0xff,
0xff,0xff,0xff,0x5f,0xfb,0xff,0xff,0xff,
0xff,0xff,0xff,0xbf,0xfd,0xff,0xff,0xff,
0xff,0xff,0xff,0x3f,0xfd,0xff,0xff,0xff,
0xff,0xff,0xff,0x7f,0xfe,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

Pixmap icon_pixmap;

typedef void (*funct_ptr)();

typedef struct {
	Widget label, button, menu, preview, *menu_buttons;
	funct_ptr call_back;
	int index, max;
	} cycle_struct;

static cycle_struct	cycle[LAST_CYCLE-FIRST_CYCLE+1];

static Widget		textfield[LAST_TEXTFIELD-FIRST_TEXTFIELD+1];

static Widget		*menu_buttons;

static Widget		toplevel, createtoplevel, 
			simulatetoplevel, object[NUM_OBJECTS];

typedef struct {
	Widget shell, txt, ca, but1, but2;
	int val, done;
	} notice_struct;

static notice_struct	notice;

static Widget		titleca[6], titles[6];

static XtInputId	child_input_des;

static GC		gc;
static XFontStruct	*xstruct, *xstruct_b;

int			color_display=0;
extern int		char_width,
			char_ascent,
			char_descent,
			char_height;

char            *pulse_frame_label[12] = {"Amplitude","Phase","Frequency",
                                         "Real","Imaginary","Fourier Transform",
                                         "Mx","My","Mz","Mxy","Phase",
                                         "Fourier Transform"};


static int   ME;
static int   *me = &ME;

/*****************************************************************************
*		EXTERNAL VARIABLES AND FUNCTIONS
******************************************************************************/
extern int tochild,
	   fromchild,
	   childpid,
	   interrupt;
/*****************************************************************************
*		WINDOWING FUNCTIONS
******************************************************************************/

/*****************************************************************************
*	init window system and set display and depth variables 
******************************************************************************/
void init_window(argc,argv)
int argc;
char *argv[];
{
   void create_gc();
   int n;
   Arg args[5];

   toplevel = OlInitialize("pulseTool","PulseTool",NULL,0,&argc,argv);
   object[BASE_FRAME] = XtCreateManagedWidget("form",formWidgetClass,
			toplevel,NULL,0);
   if (!(display = XtDisplay(toplevel)))  {
        (void)fprintf(stderr,"Cannot connect to Xserver\n");
        exit(-1);
        }
   screen = DefaultScreen(display);
   depth = DefaultDepth(display,screen);
}

/*****************************************************************************
*	Set up font and return character string with font name 
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
    xstruct = XLoadQueryFont(display,str);
    if (!xstruct)  {
      str = "8x16";
      xstruct = XLoadQueryFont(display,str);
      }
    if (xstruct)  {
      *font_name = (char *)malloc(sizeof(char)*(strlen(str)+1));
      strcpy(*font_name,str);
      }
    char_width = xstruct->max_bounds.width;
    char_ascent = xstruct->max_bounds.ascent;
    char_descent = xstruct->max_bounds.descent;
    char_height = char_ascent+char_descent;
    create_gc();
}

/*****************************************************************************
*	Set up and attach icon 
******************************************************************************/
void setup_icon()
{
    int n;
    Arg args[2];

    icon_pixmap = XCreateBitmapFromData(display,DefaultRootWindow(display),
			icon_data,64,64);
    n = 0;
    XtSetArg(args[n], XtNiconPixmap, icon_pixmap); n++;
    XtSetValues(toplevel, args, n);
}

/*****************************************************************************
*	Set up colormaps - cms for all objects except canvases, and canvas_cms
*	for canvases.
******************************************************************************/
void setup_colormap()
{
    int n, i;

    gplanes = XDisplayPlanes(display, DefaultScreen(display));
    colormap = XDefaultColormap(display, screen);
    for (i=0; i < NUM_COLORS; i++)
    {
      colors[i].red = canvas_colors[i].red << 8;
      colors[i].green = canvas_colors[i].green << 8;
      colors[i].blue = canvas_colors[i].blue << 8;
      colors[i].flags = DoRed | DoGreen | DoBlue;;
      colors[i].pixel = sun_colors[i];
    }
    if (gplanes > 6)
    {
        for(n=0; n < NUM_COLORS; n++)
        {
          XAllocColor(display, colormap, &colors[n]);
          sun_colors[n] = colors[n].pixel;
        }
    }
    else
    {
      for(n=0; n < NUM_COLORS; n++)
        sun_colors[n] = 0;
    }
}

/*****************************************************************************
*	Create gc for object which.
******************************************************************************/
void create_gc()
{
    unsigned long  valuemask;
    XGCValues values;

    valuemask = GCBackground|GCForeground|GCFont;
    values.foreground = WhitePixel(display,screen);
    values.background = BlackPixel(display,screen);
    values.font = xstruct->fid;
    gc = XCreateGC(display,DefaultRootWindow(display),valuemask, &values);
}

/*****************************************************************************
*	Create button at (x,y) with label and notify proc attached.
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
*	Given column number, return pixel value.
******************************************************************************/
int column(which,col)
int which, col;
{
    return(0);		/* not used */
}

/*****************************************************************************
*	Given row number, return pixel value.
******************************************************************************/
int row(which,r)
int which, r;
{
    return(0);		/* not used */
}

/*****************************************************************************
*	Return width of OL object.
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
*	Create menu with "num_strings" objects with labels in "strings"
*	and notify proc for menu selection "notify_proc".  Attach menu to
*	button "which".
******************************************************************************/
void attach_menu_to_button(which,menu,num_strings,strings,notify_proc)
int which, menu, num_strings;
char *strings[];
int (*notify_proc)();
{
    int i;
    int n;
    Arg args[5];
    String str;
    Widget parent;
    char label[100];
    Pixel back;
    int    bcolor, gcolor, rcolor, cindex;
    XColor xcolor;

    n=0;
    XtSetArg(args[n],XtNlabel, &str); n++;
    XtGetValues(object[which],args,n);
    strcpy(label,str);
    parent = XtParent(object[which]);
    XtDestroyWidget(object[which]);
    n=0;
    XtSetArg(args[n], XtNlabel , label);  n++;
    XtSetArg(args[n], XtNtraversalOn , FALSE);  n++;
    object[which] = XtCreateManagedWidget("button",menuButtonWidgetClass,
			parent,args,n);
    if (focusColor == 1024 && gplanes > 6)  {
      n=0;
      XtSetArg(args[n], XtNbackground, &back); n++;
      XtGetValues(object[which], args, n);
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
      XtSetValues(object[which], args, n);
      }
    n=0;
    XtSetArg(args[n],XtNmenuPane,&object[menu]); n++;
    XtGetValues(object[which],args,n);
    menu_buttons = (Widget *)malloc(sizeof(Widget)*num_strings);
    for (i=0;i<num_strings;i++)  {
        n=0;
        XtSetArg(args[n], XtNlabel , strings[i]);  n++;
        XtSetArg(args[n], XtNtraversalOn , FALSE);  n++;
        if (gplanes > 6)  {
          XtSetArg(args[n],XtNinputFocusColor,focusColor); n++;
          }
	menu_buttons[i] = XtCreateManagedWidget("menuButton",
				oblongButtonWidgetClass,object[menu],args,n);
	XtAddCallback(menu_buttons[i],XtNselect, notify_proc,
		strings[i]);
	}
}

/*****************************************************************************
*	Create cycle at (x,y) with label and strings as cycle optons.
*	Attach notify proc.
******************************************************************************/
void create_cycle(which,parent,x,y,label,num_strings,strings,def,
			notify_proc)
int which,parent;
int x, y;
char *label;
int num_strings;
char *strings[];
int def;
funct_ptr notify_proc;
{
    int i, n;
    Arg args[6];
    Dimension parent_x, parent_y, parent_width;
    Pixel back;
    int    bcolor, gcolor, rcolor, cindex;
    XColor xcolor;

    void cycle_proc();

    n = 0;
    XtSetArg(args[n], XtNlayoutType, OL_FIXEDCOLS); n++;
    XtSetArg(args[n], XtNmeasure, 3); n++;
    object[which] = XtCreateManagedWidget("panel",controlAreaWidgetClass,
			object[parent], args, n);
    n=0;
    XtSetArg(args[n], XtNstring, label); n++;
    cycle[which-FIRST_CYCLE].label = XtCreateManagedWidget("label",
			staticTextWidgetClass, object[which], args, n);
    n=0;
    XtSetArg(args[n], XtNtraversalOn, FALSE); n++;
    cycle[which-FIRST_CYCLE].button = XtCreateManagedWidget("button", 
			abbrevMenuButtonWidgetClass, object[which], args, n);
    n=0;
    XtSetArg(args[n], XtNmenuPane, &(cycle[which-FIRST_CYCLE].menu)); n++;
    XtGetValues(cycle[which-FIRST_CYCLE].button, args, n);
    n=0;
    XtSetArg(args[n], XtNstring, strings[def]); n++;
    cycle[which-FIRST_CYCLE].preview = XtCreateManagedWidget("text",
		staticTextWidgetClass, object[which], args, n);
    n = 0;
    XtSetArg(args[n],XtNpreviewWidget,cycle[which-FIRST_CYCLE].preview); n++;
    XtSetValues(cycle[which-FIRST_CYCLE].button, args, n);
    cycle[which-FIRST_CYCLE].menu_buttons = (Widget *)malloc(sizeof(Widget)*
						num_strings);
    cycle[which-FIRST_CYCLE].call_back = notify_proc;
    if (focusColor == 1024 && gplanes > 6)  {
      n=0;
      XtSetArg(args[n], XtNbackground, &back); n++;
      XtGetValues(cycle[which-FIRST_CYCLE].button, args, n);
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
      XtSetValues(cycle[which-FIRST_CYCLE].button, args, n);
      }
    for (i=0;i<num_strings;i++)  {
      n = 0;
      XtSetArg(args[n], XtNlabel, strings[i]); n++;
      XtSetArg(args[n], XtNtraversalOn , FALSE);  n++;
      if (gplanes > 6)  {
        XtSetArg(args[n],XtNinputFocusColor,focusColor); n++;
        }
      cycle[which-FIRST_CYCLE].menu_buttons[i] =
		XtCreateManagedWidget("menuButton",oblongButtonWidgetClass,
		cycle[which-FIRST_CYCLE].menu,args,n);
      XtAddCallback(cycle[which-FIRST_CYCLE].menu_buttons[i], XtNselect,
			cycle_proc, &(cycle[which-FIRST_CYCLE]));
      }
    cycle[which-FIRST_CYCLE].index = 0;
    cycle[which-FIRST_CYCLE].max = num_strings;
    XtAddCallback(cycle[which-FIRST_CYCLE].button, XtNconsumeEvent,
		cycle_proc,&(cycle[which-FIRST_CYCLE]));
}

/*****************************************************************************
*	Function to intercept SELECT button presses on abbrevMenuButtons,
*	to allow cycling of menu items with SELECT button in addition to
*	selection of items with menu.
******************************************************************************/
void cycle_proc(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    int which, found, n;
    Arg args[2];
    char *str;
    OlVirtualEvent o_event;
    XEvent *x_event;
    cycle_struct *cycle = (cycle_struct *)client_data;

  if (w == cycle->button)  {
    o_event = (OlVirtualEvent) call_data;
    x_event = (XEvent *)o_event->xevent;
    if ((o_event->virtual_name==OL_SELECT) && (x_event->type==ButtonPress))  {
      o_event->consumed = TRUE;
      n=0;
      XtSetArg(args[n], XtNlabel, &str); n++;
      if (++(cycle->index) == cycle->max)
	cycle->index = 0;
      XtGetValues(cycle->menu_buttons[cycle->index], args, n);
      n=0;
      XtSetArg(args[n],XtNstring,str); n++;
      XtSetValues(cycle->preview,args,n);
      if (cycle->call_back != NULL)
        (*cycle->call_back)(cycle->button,str,NULL);
      }
    else if (o_event->virtual_name==OL_SELECT) {
      o_event->consumed = TRUE;
      }
    }
  else  {
    which = 0; found = FALSE;
    while (!found)  {
      if (w == cycle->menu_buttons[which])
        found = TRUE;
      else
        which++;
      }
    n=0;
    XtSetArg(args[n], XtNlabel, &str); n++;
    XtGetValues(cycle->menu_buttons[which], args, n);
    cycle->index = which;
    n=0;
    XtSetArg(args[n],XtNstring,str); n++;
    XtSetValues(cycle->preview,args,n);
    if (cycle->call_back != NULL)
      (*cycle->call_back)(cycle->menu_buttons[which],str,NULL);
    }
}

/*****************************************************************************
*	Create type-in text object at (x,y) with label and a type-in area of
*	len characters and default string of value.  Attach notify proc.
******************************************************************************/
void create_panel_text(which,parent,x,y,label,value,len,notify_proc)
int which,parent;
int x, y;
char *label, *value;
int len;
int (*notify_proc)();
{
    int n;
    Arg args[5];

    n=0;
    XtSetArg(args[n], XtNlabel, label); n++;
    XtSetArg(args[n], XtNx, x); n++;
    XtSetArg(args[n], XtNy, y); n++;
    object[which] = XtCreateManagedWidget("label", captionWidgetClass,
		object[parent], args, n);
    n=0;
    XtSetArg(args[n], XtNcharsVisible, len); n++;
    XtSetArg(args[n], XtNstring, value); n++;
    textfield[which-FIRST_TEXTFIELD] = XtCreateManagedWidget("text",
		textFieldWidgetClass, object[which], args, n);
    if (notify_proc != NULL)
      XtAddCallback(textfield[which-FIRST_TEXTFIELD], XtNverification,
			notify_proc, NULL);
}


/*****************************************************************************
*	Create panel message object at (x,y) with message label.
******************************************************************************/
void create_panel_message(which,parent,x,y,label)
int which,parent;
int x, y;
char *label;
{
    int n;
    Arg args[2];

    n=0;
    XtSetArg(args[n], XtNstring, label); n++;
    object[which] = XtCreateManagedWidget("text", staticTextWidgetClass,
		object[parent], args, n);
}

/*****************************************************************************
*	Create panel area at (x,y) of size width by height.
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
    if (which == MAIN_PARAMETER_PANEL)  {
      XtSetArg(args[n], XtNlayoutType, OL_FIXEDROWS); n++;
      XtSetArg(args[n], XtNmeasure, 3); n++;
      }
    if (which == CREATE_PANEL)  {
      XtSetArg(args[n], XtNlayoutType, OL_FIXEDCOLS); n++;
      XtSetArg(args[n], XtNmeasure, 1); n++;
      }
    if (which == SIMULATE_PANEL)  {
      XtSetArg(args[n], XtNlayoutType, OL_FIXEDROWS); n++;
      XtSetArg(args[n], XtNmeasure, 7); n++;
      }
    object[which] = XtCreateManagedWidget("panel",controlAreaWidgetClass,
			object[parent], args, n);
}

/*****************************************************************************
*	Create frame at (x,y) of size width by height.
******************************************************************************/
void create_frame(which,parent,label,x,y,width,height)
int which, parent;
char *label;
int x, y, width, height;
{
   int n, argc=0;
   char *argv[2];
   Arg args[8];

   if (which == CREATE_FRAME)  {
     n = 0;
     XtSetArg(args[n], XtNx, x); n++;
     XtSetArg(args[n], XtNy, y); n++;
     XtSetArg(args[n], XtNtitle, "Create"); n++;
     createtoplevel = XtCreateApplicationShell("pulseTool",
			applicationShellWidgetClass, args, n);
     n = 0;
     object[which] = XtCreateManagedWidget("form",formWidgetClass,
			createtoplevel,args,n);
     }
   if (which == SIMULATE_FRAME)  {
     n = 0;
     XtSetArg(args[n], XtNx, x); n++;
     XtSetArg(args[n], XtNy, y); n++;
     XtSetArg(args[n], XtNtitle, "Simulate"); n++;
     simulatetoplevel = XtCreateApplicationShell("pulseTool",
			applicationShellWidgetClass, args, n);
     n = 0;
     object[which] = XtCreateManagedWidget("form",formWidgetClass,
			simulatetoplevel,args,n);
     }
}

/*****************************************************************************
*	Create canvas at (x,y) of size width by height.  Attach an event
*	handler and a repaint process.  Specify if border is to be shown (TRUE)
*	or hidden (FALSE).
******************************************************************************/
void create_canvas(which,parent,x,y,width,height,repaint_proc,event_proc,border)
int which, parent, x, y, width, height;
int (*repaint_proc)(), (*event_proc)();
int border;
{
  int n;
  Dimension  parent_x, parent_y, parent_width, parent_height;
  Arg args[9];
  Pixel pixel_col;

  if (which <= SM_WIN_6 && which >= SM_WIN_1)  {
    n=0;
    XtSetArg(args[n],XtNlayoutType, OL_FIXEDCOLS); n++;
    XtSetArg(args[n],XtNhPad, 0); n++;
    XtSetArg(args[n],XtNvPad, 0); n++;
    XtSetArg(args[n],XtNmeasure, 1); n++;
    XtSetArg(args[n],XtNcenter, True); n++;
    titleca[which-SM_WIN_1] = XtCreateManagedWidget("title",
			controlAreaWidgetClass,object[parent],args,n);
    n = 0;
    XtSetArg(args[n],XtNstring, pulse_frame_label[which-SM_WIN_1]); n++;
    titles[which-SM_WIN_1] = XtCreateManagedWidget("title",
			staticTextWidgetClass,titleca[which-SM_WIN_1],args,n);
    }

  if (which == LG_WIN)
    width += 9;
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
  XtSetArg(args[n], XtNbackground, BlackPixel(display,screen)); n++;
  if (border)  {
    XtSetArg(args[n], XtNborderWidth, (Dimension)1); n++;
    }
  if (which <= SM_WIN_6 && which >= SM_WIN_1)
    object[which] = XtCreateManagedWidget("canvas",drawAreaWidgetClass,
			titleca[which-SM_WIN_1],args,n);
  else
    object[which] = XtCreateManagedWidget("canvas",drawAreaWidgetClass,
			object[parent],args,n);
  XtAddCallback(object[which], XtNexposeCallback, repaint_proc, NULL);
  XtAddEventHandler(object[which],ButtonPressMask|ButtonReleaseMask |
			ButtonMotionMask, FALSE, event_proc, NULL);
  if (which == PLOT_WIN)  {
    n=0;
    XtSetArg(args[n],XtNbackground, &pixel_col); n++;
    XtGetValues(object[LG_WIN],args,n);
    sun_colors[0] = pixel_col;
    }
}

/*****************************************************************************
*	Set object which to the right of other.
******************************************************************************/
#define GAP 4
void set_win_right_of(which, other)
int which, other;
{
  int n;
  Arg args[4];

  n = 0;
  if (other <= SM_WIN_6 && other >= SM_WIN_1)  {
    XtSetArg(args[n],XtNxRefWidget, titleca[other-SM_WIN_1]); n++;
    }
  else  {
    XtSetArg(args[n],XtNxRefWidget, object[other]); n++;
    }
  XtSetArg(args[n],XtNxAddWidth, TRUE); n++;
  XtSetArg(args[n],XtNxAttachRight, TRUE); n++;
  if (which <= SM_WIN_6 && which >= SM_WIN_1)
    XtSetValues(titleca[which-SM_WIN_1],args,n);
  else
    XtSetValues(object[which],args,n);
}

/*****************************************************************************
*	Set object which below other.
******************************************************************************/
void set_win_below(which, other)
int which, other;
{
  int n;
  Arg args[4];

  n = 0;
  if (other <= SM_WIN_6 && other >= SM_WIN_1)  {
    XtSetArg(args[n],XtNyRefWidget, titleca[other-SM_WIN_1]); n++;
    }
  else  {
    XtSetArg(args[n],XtNyRefWidget, object[other]); n++;
    }
  XtSetArg(args[n],XtNyAddHeight, TRUE); n++;
  XtSetArg(args[n],XtNyAttachBottom, TRUE); n++;
  if (which <= SM_WIN_6 && which >= SM_WIN_1)  {
    XtSetValues(titleca[which-SM_WIN_1],args,n);
    }
  else  {
    XtSetValues(object[which],args,n);
    }
}

/*****************************************************************************
*	Set panel text value.
******************************************************************************/
void set_panel_value(which,value)
int which;
win_val value;
{
  int n;
  Arg args[4];
  char *str;

  if ((which >= FIRST_CYCLE) && (which <= LAST_CYCLE))  {
    n = 0;
    XtSetArg(args[n],XtNlabel, &str); n++;
    XtGetValues(cycle[which-FIRST_CYCLE].menu_buttons[value],args,n);
    cycle[which-FIRST_CYCLE].index = value;
    n = 0;
    XtSetArg(args[n],XtNstring, str); n++;
    XtSetValues(cycle[which-FIRST_CYCLE].preview,args,n);
    }
  else  {
    n = 0;
    XtSetArg(args[n],XtNstring, value); n++;
    XtSetValues(textfield[which-FIRST_TEXTFIELD],args,n);
    }
}

/*****************************************************************************
*	Get panel text value.
******************************************************************************/
win_val get_panel_value(which)
int which;
{
  int n, i, done;
  Arg args[4];
  char *str, *str2;

  if ((which >= FIRST_CYCLE) && (which <= LAST_CYCLE))  {
    n = 0;
    XtSetArg(args[n],XtNstring, &str); n++;
    XtGetValues(cycle[which-FIRST_CYCLE].preview,args,n);
    i = 0;   done = FALSE;
    while (!done)  {
      n = 0;
      XtSetArg(args[n],XtNlabel, &str2); n++;
      XtGetValues(cycle[which-FIRST_CYCLE].menu_buttons[i],args,n);
      if (strcmp(str,str2) == 0)
	done = TRUE;
      else 
	i++;
      }
    return((win_val)i);
    }
  else  {
    n = 0;
    XtSetArg(args[n],XtNstring, &str); n++;
    XtGetValues(textfield[which-FIRST_TEXTFIELD],args,n);
    }
  return((win_val)str);
}

/*****************************************************************************
*	Set panel label value.
******************************************************************************/
void set_panel_label(which,str)
int which;
char *str;
{
  int n;
  Arg args[4];

  if ((which >= FIRST_CYCLE) && (which <= LAST_CYCLE))  {
    n = 0;
    XtSetArg(args[n],XtNstring, str); n++;
    XtSetValues(cycle[which-FIRST_CYCLE].preview,args,n);
    }
  else  {
    n = 0;
    XtSetArg(args[n],XtNlabel, str); n++;
    XtSetArg(args[n],XtNstring, str); n++;
    XtSetValues(object[which],args,n);
    }
}

/*****************************************************************************
*	Set frame label value.
******************************************************************************/
void set_frame_label(which,value)
int which;
win_val value;
{
  int n, i;
  Arg args[4];

  if (which == BASE_FRAME)  {
    for (i=0;i<6;i++)  {
      n = 0;
      if (value == 0)  {
        XtSetArg(args[n],XtNstring, pulse_frame_label[i]); n++;
	}
      else  {
        XtSetArg(args[n],XtNstring, pulse_frame_label[i+6]); n++;
	}
      XtSetValues(titles[i],args,n);
      }
    }
  else if (which == CREATE_FRAME)  {
    n = 0;
    XtSetArg(args[n],XtNtitle, (String)value); n++;
    XtSetValues(createtoplevel,args,n);
    }
  else if (which == SIMULATE_FRAME)  {
    n = 0;
    XtSetArg(args[n],XtNtitle, (String)value); n++;
    XtSetValues(simulatetoplevel,args,n);
    }
}

/*****************************************************************************
*	Set width.
******************************************************************************/
void set_width(which,width)
int which, width;
{
}

/*****************************************************************************
*	Set height.
******************************************************************************/
void set_height(which,height)
int which, height;
{
}

/*****************************************************************************
*	Show object.
******************************************************************************/
void show_object(which)
{
    if (which == CREATE_FRAME)
      if (!XtIsRealized(createtoplevel))
        XtRealizeWidget(createtoplevel);
      else
        XtMapWidget(createtoplevel);
    else if (which == SIMULATE_FRAME)
      if (!XtIsRealized(simulatetoplevel))
        XtRealizeWidget(simulatetoplevel);
      else
        XtMapWidget(simulatetoplevel);
    XtManageChild(object[which]);
}

/*****************************************************************************
*	Hide object.
******************************************************************************/
void hide_object(which)
{
    if (which == CREATE_FRAME)
      if (!XtIsRealized(object[which]))  {}
      else 
        XtUnmapWidget(createtoplevel);
    else if (which == SIMULATE_FRAME)
      if (!XtIsRealized(object[which]))  {}
      else 
        XtUnmapWidget(simulatetoplevel);
    else
      XtUnmanageChild(object[which]);
}

/*****************************************************************************
*	Given a window object, find it's index in "objects" array.
******************************************************************************/
int find_object(item)
Widget item;
{
    int i;

    i = 0;
    while (i<NUM_OBJECTS)  {
      if (object[i] == item)
        return(i);
      i++;
      }
    return(-1);  /* error */
}

/*****************************************************************************
*	Given a window object, find it's index in "objects" array.
******************************************************************************/
int find_text_object(item)
Widget item;
{
    int i;

    i = 0;
    while (i<(LAST_TEXTFIELD-FIRST_TEXTFIELD+1))  {
      if (textfield[i] == item)
        return(i+FIRST_TEXTFIELD);
      i++;
      }
    return(-1);  /* error */
}

/*****************************************************************************
*	Do frame fit operation to resize frame to fit contents.
******************************************************************************/
void fit_frame(which)
int which;
{
}

/*****************************************************************************
*	Do frame fit operation to resize frame to fit contents in vertical
*	direction.
******************************************************************************/
void fit_height(which)
int which;
{
}

/*****************************************************************************
*	Set up signal handler for interrupt signal from pulsechild and
*	loop on BASE_FRAME.
******************************************************************************/
void window_start_loop(which)
int which;
{
    void signal1_handler();
    signal(SIGUSR1,signal1_handler);
    init_notice();
    XtRealizeWidget(toplevel);
    XtMainLoop();
}

/*****************************************************************************
*	Start graphics
******************************************************************************/
void start_graphics(which)
int which;
{
}

/*****************************************************************************
*	Tell window system to do all pending draw requests NOW.
******************************************************************************/
void flush_graphics(which)
int which;
{
    XFlush(display);
}

/******************************************************************************
*		EVENT HANDLERS
******************************************************************************/

/******************************************************************************
*	Process file button events.
******************************************************************************/
int files_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    int obj;
    if ((obj = find_object(w)) == -1)
      obj = find_text_object(w);
    do_files_proc(obj);
}

/******************************************************************************
*	Process Thresh/Scale button events.
******************************************************************************/
int horiz_notify_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_horiz_notify_proc();
}

/******************************************************************************
*	Process Expand/Full button events.
******************************************************************************/
int expand_notify_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_expand_notify_proc();
}

/******************************************************************************
*	Process Simulate button events.
******************************************************************************/
int simulate_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    int obj;
    if ((w == cycle[SIMULATE_FORMAT_CYCLE-FIRST_CYCLE].menu_buttons[0]) ||
        (w == cycle[SIMULATE_FORMAT_CYCLE-FIRST_CYCLE].menu_buttons[1]) ||
        (w == cycle[SIMULATE_FORMAT_CYCLE-FIRST_CYCLE].menu_buttons[2]) ||
	(w == cycle[SIMULATE_FORMAT_CYCLE-FIRST_CYCLE].button))  {
      do_simulate_proc(SIMULATE_FORMAT_CYCLE);
      }
    else if ((w == cycle[SIMULATE_INIT_CYCLE-FIRST_CYCLE].menu_buttons[0]) ||
        (w == cycle[SIMULATE_INIT_CYCLE-FIRST_CYCLE].menu_buttons[1]) ||
        (w == cycle[SIMULATE_INIT_CYCLE-FIRST_CYCLE].menu_buttons[2]) ||
	(w == cycle[SIMULATE_INIT_CYCLE-FIRST_CYCLE].button))  {
      }
    else  {
      if ((obj = find_object(w)) == -1)
        obj = find_text_object(w);
      do_simulate_proc(obj);
      }
}

/******************************************************************************
*	Process Pulse/Simulate cycle events.
******************************************************************************/
int display_cycle_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_display_cycle_proc();
}

/******************************************************************************
*	Process Grid  cycle events.
******************************************************************************/
int grid_cycle_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_grid_cycle_proc();
}

/******************************************************************************
*	Process Create button events.
******************************************************************************/
int create_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_create_proc(find_object(w));
}

/******************************************************************************
*	Process events when create menu item is selected.
******************************************************************************/
int create_menu_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_create_menu_proc((String)client_data);
}


/******************************************************************************
*	Process events when create cycle item is selected.
******************************************************************************/
int create_cycle_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
}

/*****************************************************************
*  Activate the Help window when Help button pressed (controlled by pulsechild)
*****************************************************************/
int help_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    void send_to_pulsechild();

    if (w == object[MAIN_HELP_BUTTON]) {
	send_to_pulsechild("help");
    }
}


/*****************************************************************
* Destroy the parent window system, and kill the child process when Quit button
* pressed.  A "quit" signal is sent to the child only if the xv_destroy
* is successful, i.e., if the user does not cancel with the
* right mouse button.
*****************************************************************/
int quit_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    int resp;

    resp = notice_yn(MAIN_QUIT_BUTTON,"Do you really want to quit?");
    if (resp)  {
      XtRemoveInput(child_input_des);
      send_to_pulsechild("quit");
      }
}


/******************************************************************************
*	Process Save button events.
******************************************************************************/
int save_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_save_proc(find_object(w));
}

/******************************************************************************
*	Process Print button events.
******************************************************************************/
int print_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_print_proc(find_object(w));
}

/******************************************************************************
*	Repaint six small canvases on top of window.
******************************************************************************/
int repaint_small_canvases(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    int obj;

    /* Figure out which canvas prompted this request and redraw it */
    if (w == object[SM_WIN_1])
      obj = SM_WIN_1;
    else if (w == object[SM_WIN_2])
      obj = SM_WIN_2;
    else if (w == object[SM_WIN_3])
      obj = SM_WIN_3;
    else if (w == object[SM_WIN_4])
      obj = SM_WIN_4;
    else if (w == object[SM_WIN_5])
      obj = SM_WIN_5;
    else if (w == object[SM_WIN_6])
      obj = SM_WIN_6;
    do_repaint_small_canvases(obj);
}

/******************************************************************************
*	Process events in six small canvases on top of window.
******************************************************************************/
int select_large_state(w, client_data, event)
Widget w;
XtPointer client_data;
XEvent *event;
{
    int obj, button;

    button = -1;  obj = -1;
    if ((event->type==ButtonPress) && (event->xbutton.button==Button1))  {
      button = 1;
      }
    if ((event->type==ButtonPress) && (event->xbutton.button==Button2))  {
      button = 2;
      }
    /* if button was pressed, figure out which canvas it was pressed in */
    if (button > 0)  {
      if (w == object[SM_WIN_1])
        obj = SM_WIN_1;
      else if (w == object[SM_WIN_2])
        obj = SM_WIN_2;
      else if (w == object[SM_WIN_3])
        obj = SM_WIN_3;
      else if (w == object[SM_WIN_4])
        obj = SM_WIN_4;
      else if (w == object[SM_WIN_5])
        obj = SM_WIN_5;
      else if (w == object[SM_WIN_6])
        obj = SM_WIN_6;
      if (obj > 0)
	do_select_large_state(obj, button);
      }
}

/******************************************************************************
*	Repaint large canvas.
******************************************************************************/
int repaint_large_canvas(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_repaint_large_canvas();
}

int large_canvas_event_handler(w, client_data, event)
Widget w;
XtPointer client_data;
XEvent *event;
{
}

/******************************************************************************
*	Repaint plot canvas.
******************************************************************************/
int repaint_plot_canvas(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_repaint_plot_canvas();
}

/******************************************************************************
*	Process events in plot canvas.
******************************************************************************/
int plot_canvas_event_handler(w, client_data, event)
Widget w;
XtPointer client_data;
XEvent *event;
{
    int button, down, up, drag, ctrl, x, y;
    button = -1;
    drag = FALSE;
    /* figure out if button or drag event */
    if (event->type==ButtonPress)  {
      if (event->xbutton.button==Button1)
        button = 1;
      else if (event->xbutton.button==Button2)
        button = 2;
      else if (event->xbutton.button==Button3)
        button = 3;
      x = event->xbutton.x;
      y = event->xbutton.y;
      ctrl = (event->xbutton.state & ControlMask);
      up = FALSE;
      down = TRUE;
      }
    else if (event->type==ButtonRelease)  {
      if (event->xbutton.button==Button1)
        button = 1;
      else if (event->xbutton.button==Button2)
        button = 2;
      else if (event->xbutton.button==Button3)
        button = 3;
      x = event->xbutton.x;
      y = event->xbutton.y;
      ctrl = (event->xbutton.state & ControlMask);
      up = TRUE;
      down = FALSE;
      }
    else if (event->type == MotionNotify)  { 
      drag = TRUE;
      if (event->xmotion.state&Button1MotionMask)
	button = 1;
      else if (event->xmotion.state&Button2MotionMask)
	button = 2;
      else if (event->xmotion.state&Button3MotionMask)
	button = 3;
      x = event->xmotion.x;
      y = event->xmotion.y;
      ctrl = (event->xmotion.state & ControlMask);
      up = FALSE;
      down = TRUE;
      }
    /* if button or drag event, figure out if button up or down and whether
	control key also pressed */
    if (button > 0)  {
      do_plot_canvas_event_handler(button,down,up,drag,ctrl, x, y);
      }
}

/******************************************************************************
*	Process events when one of parameters at bottom of main window is
*	modified.
******************************************************************************/
int param_notify_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_param_notify_proc(find_text_object(w));
}

/******************************************************************************
*	Process Simulate 3D button events.
******************************************************************************/
int simulate_3d_proc(w, client_data, call_data)
Widget w;
XtPointer client_data, call_data;
{
    do_simulate_3d_proc();
}

/*****************************************************************************
*	Initialize notice window.
******************************************************************************/
int init_notice()
{
    int n;
    Arg args[4];

    notice.shell = XtCreatePopupShell("notice",noticeShellWidgetClass,
			toplevel, NULL, 0);
    n=0;
    XtSetArg(args[n], XtNtextArea, &notice.txt);  n++;
    XtSetArg(args[n], XtNcontrolArea, &notice.ca);  n++;
    XtGetValues(notice.shell,args,n);
    n=0;
    XtSetArg(args[n], XtNtraversalOn , FALSE);  n++;
    if (gplanes > 6)  {
      XtSetArg(args[n], XtNinputFocusColor, focusColor); n++;
      }
    notice.but1 = XtCreateManagedWidget("button",oblongButtonWidgetClass,
			notice.ca, args, n);
    notice.but2 = XtCreateManagedWidget("button",oblongButtonWidgetClass,
			notice.ca, args, n);
}

/*****************************************************************************
*	Notice callback.
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
*	Pop up notice.  Don't return until we've got an answer from the
*	notice and it's gone away.
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
*	Pop up confirm box with "Yes" or "No" options.
******************************************************************************/
int notice_yn(which,str)
int which;
char *str;
{
    int n;
    Arg args[4];
    XEvent event;

    n = 0;
    if (which == MAIN_DISPLAY_CYCLE)  {
      XtSetArg(args[n], XtNemanateWidget, cycle[which-FIRST_CYCLE].button); n++;
      }
    else  {
      XtSetArg(args[n], XtNemanateWidget, object[which]); n++;
      }
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
*	Pop up informational box.
******************************************************************************/
void notice_ok(which,str)
int which;
char *str;
{
    int n;
    Arg args[4];
    XWindowAttributes attr;

    if (which == MAIN_DISPLAY_CYCLE)  {
      /* this is called when you try to select "Simulation" when no
	 simulation is active */
      /* for some reason, you have to remove the grab by the display_cycle
	  menu shell before doing another grab with the notice shell.
	  If you don't, one of two problems occurs : 1) if the notice shell
	  does a GrabNonexclusive, an error message is printed about trying
	  to remove a non-existent grab, or  2) if the notice shell does a
	  GrabExclusive, the program is permanently "Grabbed" and cannot
	  be used anymore. */
	if (XtIsRealized(cycle[which-FIRST_CYCLE].menu))  {
	  XGetWindowAttributes(display,XtWindow(cycle[which-FIRST_CYCLE].menu),
		&attr);
	  if (attr.map_state == IsViewable)
	    XtRemoveGrab(XtParent(XtParent(cycle[which-FIRST_CYCLE].menu)));
	  }
      }
    n = 0;
    if (which == MAIN_DISPLAY_CYCLE)  {
      XtSetArg(args[n], XtNemanateWidget, cycle[which-FIRST_CYCLE].button); n++;
      }
    else  {
      XtSetArg(args[n], XtNemanateWidget, object[which]); n++;
      }
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
    if (which == MAIN_DISPLAY_CYCLE)  {
      n = 0;
      XtSetArg(args[n], XtNset, FALSE); n++;
      XtSetArg(args[n], XtNbusy, FALSE); n++;
      XtSetValues(cycle[which-FIRST_CYCLE].button, args, n);
      }
    XtManageChild(notice.but2);
}


/*****************************************************************
*  Display "string" as a flashing message in specified frame's
*  label area.
*****************************************************************/
void frame_message(string, win)
char   *string;
int  win;
{
    notice_ok(win,string);
}


/******************************************************************************
*		INTERPROCESS COMMUNICATION
*
*	This stuff depends on the window system, too, or at least for 
*	sending and receiving signals....
******************************************************************************/

/*****************************************************************
*  This routine catches the SIGUSR1 signal sent by pulsechild
*  when a simulation is cancelled with the Cancel button.  The
*  Cancel button lives in pulsechild, and will therefore
*  eventually be acknowledged even though pulsetool is working
*  100% in the bloch_simulate routine.  The variable "interrupt"
*  is periodically checked during a simulation.
*****************************************************************/
void
signal1_handler()
{
    interrupt = 1;
}

/*****************************************************************
*  This routine writes "string" to pulsechild via the pipe 
*  "tochild".
*****************************************************************/
void send_to_pulsechild(string)
char     *string;
{
    int    returnval;
    strcat(string, "\0");
    returnval = write(tochild, string, strlen(string)+1);
}


/*****************************************************************
*  Fork a child process and exec pulsechild.  
*****************************************************************/
void
start_pulsechild(argc, argv)
int argc;
char *argv[];
{
    int      i, numfds, pipeto[2], pipefrom[2];
    extern char *font_name;
    void pipe_reader(), dead_child();

    if (pipe(pipeto) < 0  ||  pipe(pipefrom) < 0) {
        perror("pipe");
        exit(1);
    }
    switch (childpid = fork()) {
        case -1:
            perror("fork");
            exit(1);
        case  0:
	    /****
	    * This is the child.  The pipe to parent pulsetool is via
	    * stdout, and stdin serves as the pipe from pulsetool.
	    ****/
            dup2(pipeto[0], 0);
            dup2(pipefrom[1], 1);
            
            numfds = getdtablesize();
            for (i=3; i<numfds; i++)
                close(i);

            execvp("pulsechild", argv);
            perror("execvp");
            exit(1);
        default:
	    /****
	    * This is the parent.
	    ****/
            close(pipeto[0]);
            close(pipefrom[1]);
            tochild = pipeto[1];
            fromchild = pipefrom[0];
            break;
    }
    child_input_des = XtAddInput(fromchild, XtInputReadMask,
			pipe_reader, NULL);
    signal(SIGCHLD,dead_child);
}


/*****************************************************************
*  This is the communications link between pulsetool and
*  pulsechild.  It reads 512 byte blocks from the "fromchild"
*  pipe, and acts on the received string appropriately.
*****************************************************************/
void pipe_reader(client_data, fid, id)
XtPointer *client_data;
int *fid;
XtInputId *id;
{
    char     buf[512];
    int      i, nread;
    extern char filename[], directory_name[];
    extern char pattern_name[], *font_name;
    void frame_message();

    nread = read(*fid, buf, 512);
    if (nread > 0)
       buf[nread] = '\0';
    switch (nread) {
        case -1:
            perror("(parent)pipe_read: read");
            exit(1);
        case  0:
            perror("(parent)pipe_read: read(EOF)");
            exit(1);
        default:
            if (!strncmp(buf, "childok", 7)) {
            }
	    /****
	    * Load a new file.
	    ****/
            else if (!strncmp(buf, "load", 4)) {
                strcpy(pattern_name, &buf[4]);
			set_panel_value(PARAM_FILE_NAME,pattern_name);
			read_pattern_file(0);
	      }
	    /****
	    * Change the working directory.
	    ****/
	    else if (!strncmp(buf, "dir", 3)) {
		strcpy(directory_name, &buf[3]);
		if (chdir(directory_name) == -1) {
		    frame_message("Error:  Unable to change directory.", PARAM_DIRECTORY_NAME);
		}
#ifdef SOLARIS
	        getcwd(directory_name, 256 );
#else
		getwd(directory_name); /* check its declaration in pulsetool.c */
#endif
		set_panel_value(PARAM_DIRECTORY_NAME,directory_name);
            }
	    /****
	    * Load the file written by the editor in pulsechild.  A delay
	    * is inserted by means of the for-loop and the frame_message to
	    * allow the :w in the editor to be completed before reading.
	    ****/
            else if (!strncmp(buf, "view", 4)) {
                strcpy(pattern_name, &buf[4]);
                for (i=0; i<5e4; i++) {}
		set_panel_value(PARAM_FILE_NAME,pattern_name);
                read_pattern_file(0);
            }
    }
}


/*****************************************************************
*  Reap the child process' death, in its unlikely event.
*****************************************************************/
void dead_child()
{
    close(tochild);
    close(fromchild);
/*    fprintf(stderr,"The program \"pulsechild\" cannot be found, or has died.\n");*/
    exit(-1);
}

/******************************************************************************
*		DRAWING ROUTINES
******************************************************************************/

/*****************************************************************
*   Returns the pixel value of a color for the plot canvas,
*   given its index in the plot_canvas colormap.
*****************************************************************/
unsigned long pixel(col)
int col;
{
    return(sun_colors[col]);
}

/****************************************************************************
*   Draw a line to a canvas in color
*****************************************************************************/

void win_draw_line(which, x1, y1, x2, y2, color)
int which, x1, y1, x2, y2, color;
{
    if (xor_flag)  {
      XSetForeground(display, gc, pixel(color)^sun_colors[0]);
      }
    else
      XSetForeground(display, gc, pixel(color));
    XDrawLine(display, XtWindow(object[which]), gc,
		 x1, y1, x2, y2);
}

/****************************************************************************
*   Draw segments to a canvas in color
*****************************************************************************/

#define MAX_SEG_SIZE 250

void win_draw_segments(which, data, num, color)
int which;
int *data;
int num, color;
{
    XSegment seg[MAX_SEG_SIZE];
    int i, counter;

    XSetForeground(display, gc, pixel(color));
    counter = 0;
    for (i=0;i<num-1;i++)  {
      seg[counter].x1 = (short)data[2*i];
      seg[counter].y1 = (short)data[2*i+1];
      seg[counter].x2 = (short)data[2*(i+1)];
      seg[counter].y2 = (short)data[2*(i+1)+1];
      counter++;
      if (counter == MAX_SEG_SIZE)  {
	XDrawSegments(display, XtWindow(object[which]),
				gc,
				seg,counter);
	counter = 0;
	}
      }
    if (counter != 0)
      XDrawSegments(display, XtWindow(object[which]), gc,
		 seg,counter);
}

/****************************************************************************
*   Draw a point to a canvas in color
*****************************************************************************/

void win_draw_point(which, x, y, color)
int which, x, y, color;
{
    XSetForeground(display, gc, pixel(color));
    XDrawPoint(display, XtWindow(object[which]),
			gc, x, y);
}

/****************************************************************************
*   Draw points to a canvas in color
*****************************************************************************/

void win_draw_points(which, data, num, color)
int which;
int *data;
int num, color;
{
    XPoint points[MAX_SEG_SIZE];
    int i, counter;

    XSetForeground(display, gc, pixel(color));
    counter = 0;
    for (i=0;i<num;i++)  {
      points[counter].x = data[2*i];
      points[counter].y = data[2*i+1];
      counter++;
      if (counter == MAX_SEG_SIZE)  {
	XDrawPoints(display, XtWindow(object[which]),
				gc,
				points,counter,CoordModeOrigin);
	counter = 0;
	}
      }
    if (counter != 0)
      XDrawPoints(display, XtWindow(object[which]),
				gc,
				points,counter,CoordModeOrigin);
}

/****************************************************************************
*   Write a string to a canvas in color
*****************************************************************************/

void win_draw_string(which, x, y, str, color)
int which, x, y;
char *str;
int color;
{
    XSetForeground(display, gc, pixel(color));
    XDrawString(display, XtWindow(object[which]),
			 gc, x, y, str, strlen(str));
}

/****************************************************************************
*   Write a string to a canvas in reverse video
*****************************************************************************/

void draw_inverse_string(which, x, y, str, color)
int which, x, y;
char *str;
int color;
{
    unsigned long inv_color;
    int width;

    XSetForeground(display, gc, pixel(color));
    width = XTextWidth(xstruct,str,strlen(str));
    XFillRectangle(display, XtWindow(object[which]),
			gc, x, y - char_ascent - 1, width, char_height+2);
    XSetForeground(display, gc, pixel(0));
    XDrawString(display, XtWindow(object[which]),
			 gc, x, y, str, strlen(str));
}

/****************************************************************************
*   Clear entire canvas
*****************************************************************************/

void clear_window(which)
int which;
{
    XClearWindow(display,XtWindow(object[which]));
}

/****************************************************************************
*   Clear area of a canvas
*****************************************************************************/

void clear_area(which, x1, y1, x2, y2)
int which, x1, y1, x2, y2;
{
    XSetForeground(display, gc, pixel(0));
    XFillRectangle(display, XtWindow(object[which]),
			gc, x1, y1, x2, y2); 
}

/******************************************************************************
*	Change GC to draw in XOR mode.
******************************************************************************/
void xor_mode(which)
int which;
{
    xor_flag=TRUE;
    XSetFunction(display,gc,GXxor);
}

/******************************************************************************
*	Change GC to draw in Normal mode.
******************************************************************************/
void normal_mode(which)
int which;
{
    xor_flag=FALSE;
    XSetFunction(display,gc,GXcopy);
}

void screen_dims(width, height)
int *width, *height;
{
    *width = DisplayWidth(display, screen);
    *height = DisplayHeight(display, screen);
}

#ifdef SOLARIS
#include <sys/resource.h>

static int
getdtablesize()
{
	int		 ival;
	struct rlimit	 numfiles;

	ival = getrlimit( RLIMIT_NOFILE, &numfiles );
	if (ival < 0) {
		perror( "getrlimit" );
		_exit( 1 );
	}

	return( numfiles.rlim_cur );
}
#endif
