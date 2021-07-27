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
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <strings.h>
#include <ctype.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/param.h>
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
#include <Xm/TextF.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
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
static Colormap cmap;

static Display          *display;
static int              *display_name=NULL;
static int		depth, screen;
static int		xor_flag=FALSE;

static char icon_data[] = {
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
	Widget menu, *menu_buttons;
	} cycle_struct;

typedef struct {
	Widget label, field;
	} textfield_struct;

static cycle_struct	cycle[LAST_CYCLE-FIRST_CYCLE+1];

static textfield_struct		textfield[LAST_TEXTFIELD-FIRST_TEXTFIELD+1];

static Widget		*menu_buttons;

static Widget		toplevel, createtoplevel, 
			simulatetoplevel, object[NUM_OBJECTS];

typedef struct {
	Widget shell, txt, ca, but1, but2;
	int val, done;
	} notice_struct;

static Widget		titlebar=NULL, titles[6];

static notice_struct	notice;

static XtInputId	child_input_des;

static GC		gc;
static XFontStruct	*xstruct, *xstruct_b;

int			color_display=0;
extern int		char_width,
			char_ascent,
			char_descent,
			char_height;

char		*pulse_frame_label[12] = {"Amplitude","Phase","Frequency",
					 "Real","Imaginary","Fourier Transform",
					 "Mx","My","Mz","Mxy","Phase",
					 "Fourier Transform"};

static int   ME;
static int   *me = &ME;

static XmStringCharSet  charset = (XmStringCharSet) XmSTRING_DEFAULT_CHARSET;
void create_gc();
void send_to_pulsechild(char *string);

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
   int n;
   Arg args[5];
   char **argv2;
   int argc2, i;

   argv2 = (char **) malloc(sizeof(char *)*(argc+3));
   for (i=0;i<argc;i++)  {
     argv2[i] = argv[i];
     }
   argv2[argc] = "-geometry";
   argv2[argc+1] = "+0+0";
   argc2 = argc+2;
   n = 0;
/*   XtSetArg(args[n], XtNallowShellResize, True); n++;*/
   XtSetArg(args[n], XmNmwmDecorations, MWM_DECOR_BORDER); n++;
   toplevel = XtInitialize("pulseTool","PulseTool",args, n,
			&argc2,argv2);
   n = 0;
   object[BASE_FRAME] = XtCreateManagedWidget("form",xmFormWidgetClass,
			toplevel,args,n);
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
void setup_font(char **font_name)
{
    int i;
    char *str;
    static XtResource resources[] =  {
	{ "font","Font",XtRString, sizeof(String),
		 0, XtRString, "8x16"}};

    XtGetSubresources(toplevel,&str,"canvas","DrawingArea",resources,
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
    else
    {
       fprintf(stderr,"Font 8x16 is not installed\n");
       exit(-1);
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
    Arg args[6];

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
*	Return width of object.
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

arm_proc(w,client_data,call_data)
Widget w;
caddr_t client_data;
caddr_t call_data;
{
    Arg args[2];
    int n;
 
    n = 0;
    XtSetArg(args[n], XmNlabelString , client_data);  n++;
    XtSetValues(w,args,n);
}

void PostIt(w,popup,event)
Widget w, popup;
XButtonEvent *event;
{
   if (event->button == Button2)
     return;
   XmMenuPosition(popup,event);
   XtManageChild(popup);
}

XmString arm_str, disarm_str;

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
    XmString str;

    object[menu] = XmCreatePopupMenu(object[which],"menupane",NULL,0);
    XtAddEventHandler(object[which],ButtonPressMask,False,PostIt,object[menu]);
    arm_str = XmStringCreate(strings[0],charset);
    XtAddCallback(object[which],XmNarmCallback, arm_proc, arm_str);
    n=0;
    XtSetArg(args[n],XmNlabelString,&str); n++;
    XtGetValues(object[which],args,n);
    disarm_str = XmStringCopy(str);
    XtAddCallback(object[which],XmNdisarmCallback, arm_proc, disarm_str);
    XtAddCallback(object[which],XmNactivateCallback, notify_proc, strings[0]);
    n=0;
    XtSetArg(args[n],XmNsubMenuId,object[menu]); n++;
    XtSetValues(object[which],args,n);
    menu_buttons = (Widget *)malloc(sizeof(Widget)*num_strings);
    for (i=0;i<num_strings;i++)  {
        n=0;
        str = XmStringCreate(strings[i],charset);
        XtSetArg(args[n], XmNlabelString , str);  n++;
        XtSetArg(args[n], XmNtraversalOn , False);  n++;
        menu_buttons[i] = XtCreateManagedWidget("menuButton",
                            xmPushButtonWidgetClass,object[menu],args,n);
        XmStringFree(str);
        XtAddCallback(menu_buttons[i],XmNactivateCallback, notify_proc,
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
    XmString string;
 
    cycle[which-FIRST_CYCLE].menu = 
		XmCreatePulldownMenu(object[parent],"menupane",NULL,0);
    string = XmStringCreate(label,charset);
    n=0;
    XtSetArg(args[n], XmNlabelString , string);  n++;
    XtSetArg(args[n], XmNsubMenuId , cycle[which-FIRST_CYCLE].menu);  n++;
    XtSetArg(args[n], XmNtraversalOn , False);  n++;
    object[which] = XmCreateOptionMenu(object[parent],"optionbutton",args,n);
    XmStringFree(string);
    XtManageChild(object[which]);
    cycle[which-FIRST_CYCLE].menu_buttons = 
		(Widget *)malloc(sizeof(Widget)*num_strings);
    for (i=0;i<num_strings;i++)  {
        n=0;
        string = XmStringCreate(strings[i],charset);
        XtSetArg(args[n], XmNlabelString , string);  n++;
        XtSetArg(args[n], XmNtraversalOn , False);  n++;
        cycle[which-FIRST_CYCLE].menu_buttons[i] =
		XtCreateManagedWidget("menuButton", xmPushButtonWidgetClass,
				cycle[which-FIRST_CYCLE].menu,args,n);
        XmStringFree(string);
	if (notify_proc != NULL)
	  XtAddCallback(cycle[which-FIRST_CYCLE].menu_buttons[i],
			XmNactivateCallback, notify_proc,strings[i]);
	}
    n=0;
    XtSetArg(args[n],XmNmenuHistory,
		cycle[which-FIRST_CYCLE].menu_buttons[0]); n++;
    XtSetValues(object[which],args,n);
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
    XmString string;
    Dimension entry_height, margin_height, shadow_thickness, rc_margin_height;

    n=0;
    XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
    object[which] = XtCreateManagedWidget("form", xmRowColumnWidgetClass,
		object[parent], args, n);
    string = XmStringCreate(label,charset);
    n=0;
    XtSetArg(args[n], XmNlabelString, string); n++;
    textfield[which-FIRST_TEXTFIELD].label = XtCreateManagedWidget("text",
		xmLabelWidgetClass, object[which], args, n);
    XmStringFree(string);
    n=0;
    XtSetArg(args[n], XmNcolumns, len); n++;
    XtSetArg(args[n], XmNvalue, value); n++;
    XtSetArg(args[n], XmNmarginHeight, (Dimension)2); n++;
    textfield[which-FIRST_TEXTFIELD].field = XtCreateManagedWidget("text",
		xmTextFieldWidgetClass, object[which], args, n);
    if (notify_proc != NULL)
      XtAddCallback(textfield[which-FIRST_TEXTFIELD].field,
		XmNactivateCallback,notify_proc, NULL);
    if (which == PARAM_PULSE_LENGTH)  {
      n=0;
      XtSetArg(args[n], XmNheight, &entry_height); n++;
      XtSetArg(args[n], XmNmarginHeight, &margin_height); n++;
      XtSetArg(args[n], XmNshadowThickness, &shadow_thickness); n++;
      XtGetValues(textfield[which-FIRST_TEXTFIELD].field,args,n);
      n=0;
      XtSetArg(args[n], XmNmarginHeight, &rc_margin_height); n++;
      XtGetValues(object[which],args,n);
      n=0;
      XtSetArg(args[n], XmNheight, (Dimension)
	(3*(2*(margin_height+shadow_thickness+rc_margin_height)+
	 entry_height))); n++;
      XtSetArg(args[n], XmNresizeHeight, False); n++;
      XtSetValues(object[parent],args,n);
      }
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
    XmString string;

    string = XmStringCreate(label,charset);
    n=0;
    XtSetArg(args[n], XmNlabelString, string); n++;
    object[which] = XtCreateManagedWidget("text", xmLabelWidgetClass,
		object[parent], args, n);
    XmStringFree(string);
}

/*****************************************************************************
*	Create panel area at (x,y) of size width by height.
******************************************************************************/
void create_panel(which,parent,x,y,width,height)
int which,parent;
int x, y, width, height;
{
    int n;
    Arg args[12];

    n = 0;
    XtSetArg(args[n], XmNx, (Dimension)x); n++;
    XtSetArg(args[n], XmNy, (Dimension)y); n++;
    XtSetArg(args[n], XmNwidth, (Dimension)width); n++;
    XtSetArg(args[n], XmNheight, (Dimension)height); n++;
    XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_CENTER); n++;
    if (which == MAIN_PARAMETER_PANEL)  {
      XtSetArg(args[n], XmNresizeWidth, False); n++;
      XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); n++;
      XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
      XtSetArg(args[n], XmNnumColumns, 3); n++;
      }
    else if (which == MAIN_BUTTON_PANEL)  {
      XtSetArg(args[n], XmNresizeWidth, False); n++;
      XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
      XtSetArg(args[n], XmNnumColumns, 1); n++;
      }
    else if (which == CREATE_PANEL)  {
      XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
      XtSetArg(args[n], XmNnumColumns, 1); n++;
      }
    else if (which == SIMULATE_PANEL)  {
      XtSetArg(args[n], XmNpacking, XmPACK_COLUMN); n++;
      XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
      XtSetArg(args[n], XmNnumColumns, 2); n++;
      }
    else  {
      XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
      }
    object[which] = XtCreateManagedWidget("panel",xmRowColumnWidgetClass,
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
   int n;
   Arg args[8];

   if (which == CREATE_FRAME)  {
     n = 0;
     XtSetArg(args[n], XmNx, x); n++;
     XtSetArg(args[n], XmNy, y); n++;
     XtSetArg(args[n], XtNtitle, "Create"); n++;
     XtSetArg(args[n], XtNallowShellResize, True); n++;
     createtoplevel = XtCreatePopupShell("pulseTool",
			transientShellWidgetClass, toplevel,args, n);
     n = 0;
     object[which] = XtCreateManagedWidget("form",xmFormWidgetClass,
			createtoplevel,args,n);
     }
   if (which == SIMULATE_FRAME)  {
     n = 0;
     XtSetArg(args[n], XmNx, x); n++;
     XtSetArg(args[n], XmNy, y); n++;
     XtSetArg(args[n], XtNtitle, "Simulate"); n++;
     XtSetArg(args[n], XtNallowShellResize, True); n++;
     simulatetoplevel = XtCreatePopupShell("pulseTool",
			transientShellWidgetClass, toplevel, args, n);
     n = 0;
     object[which] = XtCreateManagedWidget("form",xmFormWidgetClass,
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
  Dimension  parent_x, parent_y, parent_width, parent_height, bar_y=0;
  Arg args[9];
  Pixel pixel_col;
  XmString string;

  if (which <= SM_WIN_6 && which >= SM_WIN_1)  {
    if (titlebar == NULL)  {
      n = 0;
      XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
      XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_CENTER); n++;
      titlebar = XtCreateManagedWidget("form", xmRowColumnWidgetClass,
			object[parent], args, n);
      }
    string = XmStringCreate(pulse_frame_label[which-SM_WIN_1],charset);
    n = 0;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++;
    XtSetArg(args[n], XmNlabelString, string); n++;
    XtSetArg(args[n], XmNrecomputeSize, False); n++;
    if ((which==SM_WIN_1) || (which==SM_WIN_6))  {
      XtSetArg(args[n], XmNwidth, (Dimension)(width-2)); n++;
      }
    else  {
      XtSetArg(args[n], XmNwidth, (Dimension)(width-1)); n++;
      }
    titles[which-SM_WIN_1] = XtCreateManagedWidget("title", xmLabelWidgetClass,
			titlebar, args, n);
    XmStringFree(string);
    n = 0;
    XtSetArg(args[n], XmNy, &parent_y); n++;
    XtSetArg(args[n], XmNheight, &bar_y); n++;
    XtGetValues(titlebar,args,n);
    bar_y += parent_y+8;
    parent_y = 0;
    }

  if (which == LG_WIN)
    width += 9;
  n = 0;
  XtSetArg(args[n],XmNx, &parent_x); n++;
  XtSetArg(args[n],XmNy, &parent_y); n++;
  XtGetValues(object[parent],args,n);
  n = 0;
  XtSetArg(args[n], XmNwidth, (Dimension)width); n++;
  XtSetArg(args[n], XmNheight, (Dimension)height); n++;
  XtSetArg(args[n], XmNx, (Dimension)(x + parent_x)); n++;
  XtSetArg(args[n], XmNy, (Dimension)(y + parent_y + bar_y)); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  XtSetArg(args[n], XmNbackground, BlackPixel(display,screen)); n++;
  if (border)  {
    XtSetArg(args[n], XmNborderWidth, (Dimension)1); n++;
    }
  object[which] = XtCreateManagedWidget("canvas",xmDrawingAreaWidgetClass,
			object[parent],args,n);
  XtAddCallback(object[which], XmNexposeCallback, repaint_proc, NULL);
  XtAddEventHandler(object[which],ButtonPressMask|ButtonReleaseMask |
			ButtonMotionMask, FALSE, event_proc, NULL);
  if (which == PLOT_WIN)  {
    n=0;
    XtSetArg(args[n],XmNbackground, &pixel_col); n++;
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

/*  n = 0;
  XtSetArg(args[n],XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNrightWidget, object[which]); n++;
  XtSetValues(object[other],args,n);*/
  n = 0;
  XtSetArg(args[n],XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNleftWidget, object[other]); n++;
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

/*  n = 0;
  XtSetArg(args[n],XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNbottomWidget, object[which]); n++;
  XtSetValues(object[other],args,n);*/
  n = 0;
  XtSetArg(args[n],XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNtopWidget, object[other]); n++;
  XtSetValues(object[which],args,n);
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

  if ((which >= FIRST_CYCLE) && (which <= LAST_CYCLE))  {
    n = 0;
    XtSetArg(args[n],XmNmenuHistory,
		cycle[which-FIRST_CYCLE].menu_buttons[value]); n++;
    XtSetValues(object[which],args,n);
    }
  else  {
    n = 0;
    XtSetArg(args[n],XmNvalue, value); n++;
    XtSetValues(textfield[which-FIRST_TEXTFIELD].field,args,n);
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
  XmString string1, string2;
  String str;

  if ((which >= FIRST_CYCLE) && (which <= LAST_CYCLE))  {
    n = 0;
    XtSetArg(args[n],XmNlabelString, &string1); n++;
    XtGetValues(XmOptionButtonGadget(object[which]),args,n);
    i = 0;   done = FALSE;
    while (!done)  {
      n = 0;
      XtSetArg(args[n],XmNlabelString, &string2); n++;
      XtGetValues(cycle[which-FIRST_CYCLE].menu_buttons[i],args,n);
      if (XmStringCompare(string1,string2))
	done = TRUE;
      else 
	i++;
      }
    return((win_val)i);
    }
  else  {
    n = 0;
    XtSetArg(args[n],XmNvalue, &str); n++;
    XtGetValues(textfield[which-FIRST_TEXTFIELD].field,args,n);
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
  XmString string;

  if ((which >= FIRST_CYCLE) && (which <= LAST_CYCLE))  {
    }
  else if ((which >= FIRST_TEXTFIELD) && (which <= LAST_TEXTFIELD))  {
    string = XmStringCreate(str,charset);
    n = 0;
    XtSetArg(args[n],XmNlabelString, string); n++;
    XtSetValues(textfield[which-FIRST_TEXTFIELD].label,args,n);
    XmStringFree(string);
    }
  else  {
    string = XmStringCreate(str,charset);
    n = 0;
    XtSetArg(args[n],XmNlabelString, string); n++;
    XtSetValues(object[which],args,n);
    XmStringFree(string);
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
  XmString string;

  if (which == BASE_FRAME)  {
    for (i=0;i<6;i++)  {
      if (value == 0)
	string = XmStringCreate(pulse_frame_label[i],charset);
      else
	string = XmStringCreate(pulse_frame_label[i+6],charset);
      n = 0;
      XtSetArg(args[n],XmNlabelString, string); n++;
      XtSetValues(titles[i],args,n);
      XmStringFree(string);
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
    if (which == CREATE_FRAME)  {
      if (!XtIsRealized(createtoplevel))
        XtRealizeWidget(createtoplevel);
      XtPopup(createtoplevel,XtGrabNone);
      }
    else if (which == SIMULATE_FRAME)  {
      if (!XtIsRealized(simulatetoplevel))
        XtRealizeWidget(simulatetoplevel);
      XtPopup(simulatetoplevel,XtGrabNone);
      }
    else
      XtManageChild(object[which]);
}

/*****************************************************************************
*	Hide object.
******************************************************************************/
void hide_object(which)
{
    if (which == CREATE_FRAME)  {
      if (XtIsRealized(object[which]))
	XtPopdown(createtoplevel);
      }
    else if (which == SIMULATE_FRAME)  {
      if (XtIsRealized(object[which]))
	XtPopdown(simulatetoplevel);
      }
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
      if (textfield[i].field == item)
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
caddr_t client_data, call_data;
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
caddr_t client_data, call_data;
{
    do_horiz_notify_proc();
}

/******************************************************************************
*	Process Expand/Full button events.
******************************************************************************/
int expand_notify_proc(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    do_expand_notify_proc();
}

/******************************************************************************
*	Process Simulate button events.
******************************************************************************/
int simulate_proc(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    int obj;
    if ((w == cycle[SIMULATE_FORMAT_CYCLE-FIRST_CYCLE].menu_buttons[0]) ||
        (w == cycle[SIMULATE_FORMAT_CYCLE-FIRST_CYCLE].menu_buttons[1]) ||
        (w == cycle[SIMULATE_FORMAT_CYCLE-FIRST_CYCLE].menu_buttons[2]) ||
        (w == object[SIMULATE_FORMAT_CYCLE]))  {
      do_simulate_proc(SIMULATE_FORMAT_CYCLE);
      }
    else if ((w == cycle[SIMULATE_INIT_CYCLE-FIRST_CYCLE].menu_buttons[0]) ||
        (w == cycle[SIMULATE_INIT_CYCLE-FIRST_CYCLE].menu_buttons[1]) ||
        (w == cycle[SIMULATE_INIT_CYCLE-FIRST_CYCLE].menu_buttons[2]) ||
        (w == object[SIMULATE_INIT_CYCLE]))  {
      }
    else  {
      obj = find_object(w);
      do_simulate_proc(obj);
      }
}

/******************************************************************************
*	Process Pulse/Simulate cycle events.
******************************************************************************/
int display_cycle_proc(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    do_display_cycle_proc();
}

/******************************************************************************
*	Process Grid  cycle events.
******************************************************************************/
int grid_cycle_proc(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    do_grid_cycle_proc();
}

/******************************************************************************
*	Process Create button events.
******************************************************************************/
int create_proc(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    do_create_proc(find_object(w));
}

/******************************************************************************
*	Process events when create menu item is selected.
******************************************************************************/
int create_menu_proc(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    do_create_menu_proc((String)client_data);
}


/******************************************************************************
*	Process events when create cycle item is selected.
******************************************************************************/
int create_cycle_proc(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
}

/*****************************************************************
*  Activate the Help window when Help button pressed (controlled by pulsechild)
*****************************************************************/
int help_proc(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{

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
caddr_t client_data, call_data;
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
caddr_t client_data, call_data;
{
    do_save_proc(find_object(w));
}

/******************************************************************************
*	Process Print button events.
******************************************************************************/
int print_proc(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    do_print_proc(find_object(w));
}

/******************************************************************************
*	Repaint six small canvases on top of window.
******************************************************************************/
int repaint_small_canvases(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    int obj= -1;

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
    if (obj > 0)
       do_repaint_small_canvases(obj);
}

/******************************************************************************
*	Process events in six small canvases on top of window.
******************************************************************************/
int select_large_state(w, client_data, event)
Widget w;
caddr_t client_data;
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
caddr_t client_data, call_data;
{
    do_repaint_large_canvas();
}

int large_canvas_event_handler(w, client_data, event)
Widget w;
caddr_t client_data;
XEvent *event;
{
}

/******************************************************************************
*	Repaint plot canvas.
******************************************************************************/
int repaint_plot_canvas(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    do_repaint_plot_canvas();
}

/******************************************************************************
*	Process events in plot canvas.
******************************************************************************/
int plot_canvas_event_handler(w, client_data, event)
Widget w;
caddr_t client_data;
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
caddr_t client_data, call_data;
{
    do_param_notify_proc(find_text_object(w));
}

/******************************************************************************
*	Process Simulate 3D button events.
******************************************************************************/
int simulate_3d_proc(w, client_data, call_data)
Widget w;
caddr_t client_data, call_data;
{
    do_simulate_3d_proc();
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
    notice.shell = XmCreateErrorDialog(toplevel,"Error",args,n);
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
*	Pop up confirm box with "Yes" or "No" options.
******************************************************************************/
int notice_yn(which,str)
int which;
char *str;
{
    XmString string, ok_string, cancel_string;
    int n;
    Dimension x,y;
    Arg args[8];
    Widget cancel;
    string = XmStringCreate(str,charset);
    ok_string = XmStringCreate("Yes",charset);
    cancel_string = XmStringCreate("No",charset);
    XtManageChild(notice.but2);
    n = 0;
    XtSetArg(args[n], XmNmessageString, string); n++;
    XtSetArg(args[n], XmNokLabelString, ok_string); n++;
    XtSetArg(args[n], XmNcancelLabelString, cancel_string); n++;
    XtSetArg(args[n], XmNdefaultButtonType, XmDIALOG_OK_BUTTON); n++;
    XtSetArg(args[n], XmNdefaultButton, notice.but1); n++;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_QUESTION); n++;
    XtSetValues(notice.shell, args, n);
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
    XmString string, ok_string;
    int n;
    Dimension x,y;
    Arg args[8];
    Widget cancel;
    string = XmStringCreate(str,charset);
    ok_string = XmStringCreate("Ok",charset);
    XtUnmanageChild(notice.but2);
    n = 0;
    XtSetArg(args[n], XmNmessageString, string); n++;
    XtSetArg(args[n], XmNokLabelString, ok_string); n++;
    XtSetArg(args[n], XmNdefaultButtonType, XmDIALOG_OK_BUTTON); n++;
    XtSetArg(args[n], XmNdefaultButton, notice.but1); n++;
    XtSetArg(args[n], XmNdialogType, XmDIALOG_ERROR); n++;
    XtSetValues(notice.shell, args, n);
    popup_notice();
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
    signal(SIGUSR1,signal1_handler);
}

/*****************************************************************
*  This routine writes "string" to pulsechild via the pipe 
*  "tochild".
*****************************************************************/
void send_to_pulsechild(char *string)
{
    int    returnval;
    char str[512];

    strcpy(str,string);
    strcat(str, "\0");
    returnval = write(tochild, str, strlen(str));
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
caddr_t *client_data;
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
    switch(nread) {
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
                getcwd(directory_name, 256);
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
void normal_mode(int which)
{
    xor_flag=FALSE;
    XSetFunction(display,gc,GXcopy);
}

void screen_dims(width, height)
int *width, *height;
{
    *width = DisplayWidth(display,screen);
    *height = DisplayHeight(display,screen);
}
