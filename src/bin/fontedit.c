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

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
/*
#include <X11/IntrinsicP.h>
*/
#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>  

#define TOP_MARGIN 10
#define LEFT_MARGIN 10
#define BOTTOM_MARGIN 10
#define AROUND_RASTER_MARGIN 20
#define GRID_TO_COMMAND_MARGIN 5
#define RIGHT_MARGIN 5
#define BORDERWIDTH  5
#define ROW_GAP  8

#define MIN_SQUARE_SIZE 3
#define DEFAULT_SQUARE_SIZE 12

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

void _XtInherit() {};
void XtToolkitInitialize() {};

#define CHARNUM 256
static char *outdata[CHARNUM];
static char *indata[CHARNUM];
static char ch_status[34];
static int  use_var_font = 0; 
static int  firstch = 30;
static int  lastch = 128;
static int  curch = 30;
static int  out_first = 30;
static int  out_last = 128;
static int char_row_bytes;
static int  code_x = 0, code_y = 0;
static int  sizey1 = 0, sizey2 = 0 ;
static Window frame, grid_window, grid_window2;
static Window raster_window, raster2_window;
static XFontStruct *font;
static int   charWidth, charHeight, char_ascent;
static int   charWidth2, charHeight2, char_ascent2;
static int   charWidth3, charHeight3, char_ascent3;
static int  invert_flag = 0;
static int   base_x, base_y;
static int   screen, backup_num;
static int   reverse = 0;
static float multx, multy;
static Font  x_font = NULL;
XFontStruct  *newFontInfo;
Pixmap   plCharmap = NULL;
static   XImage *char_image = NULL;
Display *d;
GC       bit_gc, gc, raster_gc;
unsigned long foreground;
unsigned long background;
unsigned long invertcolor;
unsigned long bordercolor;


static Cursor exchange, pirate, cross, dot;
static char *backup_data = NULL;
static char *filename = NULL; /* name of input file */
static char *backup_filename;
static char *stripped_name; 
static char browse_start[7] = "Browse";
static char browse_stop[5] = "Stop";
static char *progname;
XColor whitecolor;


/* button callback function */
void  Browse(), ClearOrSetAll(), InvertAll(),
      MoveUD(), MoveLR(), IncrDecr(),
      DispChar(), CopyChar(), CopyAllChar(),
      CopyScaleChar(), CopyScaleAll(), UnDo(),
      alertHandler(), Quit();
int   WriteOutput();
int   InvertRasterBit();

struct message_data {
  Window window;
  char *name;
  int name_length;
  int name_width;  /* in pixels */
  int x_offset;
  };

struct alert_data {
  Window w;
  char *msg1, *msg2;
  int msg1_length, msg2_length;
  struct message_data *command_info;
  };

#define  BROWSE	  0
#define  NEXTCH   12
#define  PREVCH   13

static struct command_data {
  char *name;
  void (*proc)(); 
  int data; 
  Window window;
  int name_length;
  int x_offset;  /* so text is centered within command box */
  int inverted;
  } commands [] = {
	{browse_start,	Browse, 0},

	{"Clear",	ClearOrSetAll, 0},
/***
	{"Set",		ClearOrSetAll, 1},
	{"Invert",	InvertAll},
***/

	{"Copy",	CopyChar, 0},
	{"Copy All",	CopyAllChar, 0},

	{"Copy+Scale",	CopyScaleChar, 0},
	{"Cp+S All",	CopyScaleAll, 0},

	{"Move UP",	  MoveUD, 1},
	{"Move Down",	  MoveUD, 0},

	{"Move Left",	  MoveLR, 1},
	{"Move Right",	  MoveLR, 0},

	{"Next Char",	  DispChar, 1},
	{"Pre  Char",	  DispChar, 0},

	{"Undo",	  UnDo, 0},
	{"Write Output", (void (*)()) WriteOutput},
	{"Quit",	 Quit}
  };
#define N_COMMANDS (sizeof(commands)/sizeof(commands[0]))
static struct command_data *button_down_command = NULL;

static int square_size;  /* length of square's side, in pixels */
int    frame_width = 1, frame_height = 1;
static int right_side_bottom, right_side_width;

   /* has user changed bitmap since starting program or last write? */
static int changed = FALSE;

extern char *sys_errlist[];
#define min(x,y) ((x < y) ? x : y)
#define max(x,y) ((x < y) ? y : x)


main (argc, argv)
  int argc;
  char **argv;
{
  SetUp (argc, argv);
  curch = firstch -1;
  DispChar(1);
  while (TRUE)
  {
    XEvent event;
    XNextEvent(d, &event);
    ProcessEvent(&event);
  }
}


#define attr_fontsource 0
#define attr_starting 1
#define attr_ending 2
#define attr_fontwidth 3
#define attr_fontheight 4
#define attr_foreground 5
#define attr_background 6


struct attribs {
    char *argname;		/* command line argument name */
    char *resname;		/* resource name */
    char *value;
    char *desc;
} attributes[] = {
	{ "-fn", "9x15", NULL,
	  "-fn font               font used for reference" },
	{ "-s", "30", NULL,
	  "-s  number             starting code number of font" },
	{ "-e", "128", NULL,
	  "-e  number             ending code number of font" },
	{ "-fw", "fontWidth", NULL, 
	  "-fw number             font width in pixels" },
	{ "-fh", "fontHeight", NULL, 
	  "-fh number             font height in pixels" },
	{ "-fg", "Foreground", NULL,
	  "-fg color              foreground color" },
	{ "-bg", "Background", NULL,
	  "-bg color              background color" },
	{ NULL, NULL, NULL, NULL }};


usage ()
{
    struct attribs *attr;

    fprintf (stderr, 
	     "usage:  %s [-options ...] filename \n\n",
	     progname);
    fprintf (stderr, 
	 "where options include:\n");
    fprintf (stderr, 
	 "    -display host:display       X server to use\n");
    for (attr = attributes; attr->argname != NULL; attr++) {
	fprintf (stderr, "    %s\n", attr->desc);
    }
    fprintf (stderr, "\n");
    fprintf (stderr, " The default starting code is 30\n"); 
    fprintf (stderr, " The default ending code is 128\n"); 
    exit (1);
}

SetUp (argc, argv)
  int argc;
  char **argv;
{
  char *StripName(), *cify_name(), *BackupName();
  char *option, *fontname;
  char *geometry = NULL, *host = NULL;
  int i;
  int status;
  struct attribs *attr;
  char *var_name = NULL;
  struct  stat  f_stat;

  progname = argv[0];

  /* Parse command line */
  for (i = 1; i < argc; i++) {
    char *arg = argv[i];
    int len;

    if (arg[0] == '-') {

	for (attr = attributes; attr->argname != NULL; attr++) {
	    if (strcmp (arg, attr->argname) == 0) {
		if (++i >= argc) usage ();
		attr->value = argv[i];
		break;
	    }
	}
	if (attr->argname) continue;		/* got an arg */
	len = strlen (arg);
	switch (arg[1]) {
	    case 'd':				/* -display host:dpy */
		if ((arg[2] && strncmp ("-display", arg, len) != 0) ||
		    (++i >= argc))
		  usage ();
		host = argv[i];
		continue;
	    case 'g':				/* -geometry geom */
		if ((arg[2] && strncmp ("-geometry", arg, len) != 0) ||
		    (++i >= argc))
		  usage ();
		geometry = argv[i];
		continue;
	    default:
		usage ();
	}
    } else if (filename == NULL)
    	filename = argv[i];
    else 
      usage ();
  }

  if (filename == NULL) {
    fprintf (stderr, "%s:  no output file name specified\n", progname);
    usage ();
  }
  
  if (!(d = XOpenDisplay(host))) {
	fprintf(stderr, "%s:  unable to open display \"%s\"\n",
		argv[0], XDisplayName(host));
	exit (1);
    }

  if (geometry == NULL) geometry = XGetDefault (d, progname, "Geometry");

  screen = DefaultScreen(d);
  gc = DefaultGC (d, screen);
  XSetLineAttributes (d, gc, 0, LineSolid, CapNotLast, JoinMiter);

  for (attr = attributes; attr->argname != NULL; attr++) {
	if (attr->value != NULL) continue;	/* got from command line */
	attr->value = XGetDefault (d, progname, attr->resname);
  }


  /* get data */

  stripped_name = cify_name (var_name ? var_name : StripName (filename));
  backup_filename = BackupName (filename);

  for (attr = attributes; attr->argname != NULL; attr++)
  {
    if (strcmp ("-fn", attr->argname) == 0) 
    {
	if (attr->value != NULL)
	    fontname = attr->value;
	else
	    fontname = attr->resname;
	break;				/* out of deepest for loop */
    }
  }
  
   for( i = 0; i < CHARNUM; i++ )
   {
	outdata[i] = NULL;
	indata[i] = NULL;
   }
   for( i = 0; i < 34; i++ )
	ch_status[i] = 0;

   if (stat(fontname, &f_stat) == 0)
   {
	if (f_stat.st_size > 100)  /* 100 is any number */
	    ReadInputFont(1, fontname);
   }

   if (!use_var_font)
   {
       newFontInfo = XLoadQueryFont(d, fontname);
       if (newFontInfo == NULL)
       {
          fprintf(stderr, " font %s is not available\n", fontname);
          exit(0);
       }
       x_font = newFontInfo->fid;
       if (x_font == NULL)
          exit(0);

       charWidth = newFontInfo->max_bounds.width;
       charHeight = newFontInfo->max_bounds.ascent
                      + newFontInfo->max_bounds.descent;
       char_ascent = newFontInfo->max_bounds.ascent;
    }

   option = attributes[attr_fontwidth].value;
   charWidth2 = option ? atoi(option) : charWidth * 2;
   option = attributes[attr_fontheight].value;
   charHeight2 = option ? atoi(option) : charHeight * 2;
/*
   char_ascent2 = char_ascent * (charHeight2 / charHeight);
*/
   char_ascent2 =((float) char_ascent * (float)charHeight2) /(float) charHeight + 0.5;
  
   font = XLoadQueryFont(d, "9x15");
   if (font == NULL)
       font = XLoadQueryFont(d, "8x13");
   if (font == NULL)
   {
	fprintf(stderr, " font 8x13 and 9x15 are not found\n");
	exit(0);
   }
   charWidth3 = font->max_bounds.width;
   charHeight3 = font->max_bounds.ascent
                      + font->max_bounds.descent;
   char_ascent3 = font->max_bounds.ascent;
   x_font = font->fid;
   XSetFont (d, gc, x_font);

   multx = (float)charWidth2 /  (float)charWidth;
   multy =  (float)charHeight2 /  (float)charHeight;
   if ((option = attributes[attr_starting].value) != NULL)
   {
      firstch = atoi (option);
      if (firstch < 0) firstch = 30;
      if (firstch > 255) firstch = 30;
   }
   if ((option = attributes[attr_ending].value) != NULL)
   {
      lastch = atoi (option);
      if (lastch < 0) lastch = 128;
      if (lastch > 255) lastch = 255;
   }
   curch = firstch;
   out_first = firstch;
   out_last = lastch;
   if (stat(filename, &f_stat) == 0)
   {
	if (f_stat.st_size > 100)  /* 100 is any number */
	    ReadInputFont(2, filename);
   }
   char_row_bytes = (charWidth2 + 7) / 8;

  cross = XCreateFontCursor (d, XC_crosshair);
  dot = XCreateFontCursor (d, XC_dot);
/*
  exchange = XCreateFontCursor (d, XC_exchange);
*/
  exchange = XCreateFontCursor (d, XC_iron_cross);
  pirate = XCreateFontCursor (d, XC_circle);

  foreground = bordercolor = BlackPixel (d, screen);
  background = WhitePixel (d, screen);
  invertcolor = foreground ^ background;

  if (DisplayCells(d, screen) > 2) {
    Colormap cmap = DefaultColormap (d, screen);
    char *fore_color = attributes[attr_foreground].value;
    char *back_color = attributes[attr_background].value;
    char *high_color = "red";
    char *brdr_color = "black";
    char *mous_color = "blue";
    char *cross_color = "red";
    XColor fdef, bdef, hdef, mdef;
    unsigned long masks[2];
    int result = 0;
    int warn = FALSE;

    if (!(result = (fore_color && XParseColor(d, cmap, fore_color, &fdef)))) {
	if (fore_color)
	   fprintf (stderr, "%s:  invalid foreground color \"%s\"\n", 
	   	    progname, fore_color);
	fdef.pixel = foreground;
	XQueryColor(d, cmap, &fdef);
    }
    if (back_color && XParseColor(d, cmap, back_color, &bdef)) {
	result = 1;
    } else {
	if (back_color)
	   fprintf (stderr, "%s:  invalid background color \"%s\"\n", 
	   	    progname, back_color);
	bdef.pixel = background;
	XQueryColor(d, cmap, &bdef);
    }
    if (high_color && XParseColor(d, cmap, high_color, &hdef)) {
	result = 1;
    } else {
	if (high_color)
	   fprintf (stderr, "%s:  invalid highlight color \"%s\"\n", 
	   	    progname, high_color);
    }
    /* if background or foreground or highlight is found */
    if (result) {
	if (XAllocColorCells(d, cmap, FALSE, masks, high_color ? 2 : 1,
	 	&background, 1)) {
            bdef.pixel = background;
            XStoreColor(d, cmap, &bdef);
            invertcolor = masks[0];
            fdef.pixel = foreground = background | invertcolor;
            XStoreColor(d, cmap, &fdef);
	}
	else {
	    warn = TRUE;
	    fprintf (stderr, "%s:  unable to allocate color cells\n", 
	    	     progname);
	}
    }
	if (XParseColor(d, cmap, "red", &bdef)) {
	   if (XAllocColor(d, cmap, &bdef))
	      bordercolor = bdef.pixel;
	   else {
	      if (!warn)
		 fprintf (stderr, 
		 	  "%s:  unable to allocate border color \"%s\"\n",
			  progname, brdr_color);
	   }
	}
	else {
	   fprintf (stderr, "%s:  invalid border color \"%s\"\n", 
	   	    progname, brdr_color);
	}
    /* recoloring cursors is not done well */
    whitecolor.red = whitecolor.green = whitecolor.blue = ~0;
    if (mous_color) {
	if (XParseColor (d, cmap, mous_color, &mdef)) {
	   XRecolorCursor (d, cross, &mdef, &whitecolor);
	   XRecolorCursor (d, dot, &mdef, &whitecolor);
	   XRecolorCursor (d, pirate, &mdef, &whitecolor);
	   XRecolorCursor (d, exchange, &mdef, &whitecolor);
	}
	else {
	   fprintf (stderr, "%s:  invalid mouse color \"%s\"\n", 
	   	    progname, mous_color);
	}
    }
    if (XParseColor (d, cmap, cross_color, &mdef))
	   XRecolorCursor (d, cross, &mdef, &whitecolor);
  }    

  {
     XSizeHints hints;
     int geom_result;
     int display_width = DisplayWidth(d, screen);
     int display_height = DisplayHeight(d, screen);
     XSetWindowAttributes attrs;
     attrs.background_pixel = background;
     attrs.border_pixel = foreground;
     attrs.event_mask = StructureNotifyMask | KeyPressMask;
     attrs.cursor = cross;

     frame = XCreateWindow (d, RootWindow (d, screen),
     	0, 0, 1, 1, 
	BORDERWIDTH, CopyFromParent, CopyFromParent, CopyFromParent,
	CWBackPixel | CWBorderPixel | CWEventMask | CWCursor,
	&attrs);
     raster_gc = XCreateGC(d, frame, 0, 0);
     if (!use_var_font)
        XSetFont (d, raster_gc, newFontInfo->fid);
     LayoutStage1(); 
     OuterWindowDims (MIN_SQUARE_SIZE, right_side_width, right_side_bottom,
	&hints.min_width, &hints.min_height);
     hints.flags = PMinSize;
     if (geometry)
     {
	geom_result = XParseGeometry (geometry, &hints.x, &hints.y, &hints.width, &hints.height);
        if ((geom_result & WidthValue) && (geom_result & HeightValue)) {
           if (hints.width < hints.min_width) hints.width = hints.min_width;
           if (hints.height < hints.min_height) hints.height = hints.min_height;
           hints.flags |= USSize;
        }
        if ((geom_result & XValue) || (geom_result & YValue))
           hints.flags |= USPosition;
     }
     if (!(hints.flags & USSize))
     {
        OuterWindowDims (DEFAULT_SQUARE_SIZE, right_side_width, right_side_bottom,
            &hints.width, &hints.height);
        hints.flags |= PSize;
     }
     if (!(hints.flags & USPosition))
     {
	hints.x = min (200, display_width - hints.width - 2*BORDERWIDTH);
	hints.y = min (200, display_height - hints.height - 2*BORDERWIDTH);
	hints.flags |= PPosition;
     }
     if ((geom_result & XValue) && (geom_result & XNegative))
	hints.x += display_width - hints.width - BORDERWIDTH * 2;
     if ((geom_result & YValue) && (geom_result & YNegative))
	hints.y += display_height - hints.height - BORDERWIDTH * 2;
     XMoveResizeWindow (d, frame, hints.x, hints.y, hints.width, hints.height);
     XSetStandardProperties (d, frame, "Font Editor", "Fontedit", None, argv, argc, &hints);
  }

  XMapWindow (d, frame);
   plCharmap = XCreatePixmap(d, frame, charWidth, charHeight, 1);
   if (plCharmap == NULL)
   {
        fprintf(stderr, "  can not create pixmap\n");
        exit(0);
   }
   bit_gc = XCreateGC(d, plCharmap, 0, 0);
   if (!use_var_font)
        XSetFont (d, bit_gc, newFontInfo->fid);
   XSetForeground(d, bit_gc, 0);
   XFillRectangle(d, plCharmap, bit_gc, 0, 0, charWidth, charHeight);
   XSetForeground(d, bit_gc, 1);
}


ProcessEvent (event)
  register XEvent *event;
{
  register Window w = event->xany.window;
  register int i;

  if (event->type == Expose)
	disp_info();
  if (w == grid_window2)
    ProcessWindow2Event (event);
  else if (w == grid_window)
    ProcessResource (event);
  else if (w == frame)
    ProcessFrameEvent (event);
  else if (w == raster_window)
    RepaintRaster();
  else if (w == raster2_window)
    {
        if (event->type == Expose)
    	   RepaintRaster2();
    }
  else for (i=0;i<N_COMMANDS;i++)
    if (w == commands[i].window)
      ProcessButtonEvent (&commands[i], event, i);
}

ProcessResource (event)
  XEvent *event;
{
  switch (event->type) {
    case Expose: 
	DispChar(2);  /* redarw source */
	break;
    default:
	break;
   }
}

ProcessWindow2Event (event)
  XEvent *event;
{
  int x_square, y_square, set;
  static int x_square_prev, y_square_prev;
  static int raster_outdated;
  switch (event->type) {

    case Expose: {
#define this_event ((XExposeEvent *)event)
      int x1 = this_event->x;
      int y1 = this_event->y;
      int x2 = x1 + this_event->width;
      int y2 = y1 + this_event->height;
#undef this_event
      x1 /= square_size;
      x2 /= square_size;
      y1 /= square_size;
      y2 /= square_size;
      if (x2 > charWidth2)
        x2 = charWidth2;
      if (y2 > charHeight2)
        y2 = charHeight2;
      RepaintGridLinesPartially(x1,y1,x2,y2,TRUE, 2, 0);
      RepaintWindow2 (x1,y1,x2,y2,FALSE);
      break;
      }

    case ButtonPress:
      if (WhatSquare (event, &x_square, &y_square))
        return;  /* mouse outside window */
      if (curch < out_first)
	  out_first = curch;
      if (curch > out_last)
	  out_last = curch;
      switch (((XButtonPressedEvent *)event)->button) {
	case 1: /* Left button */
	  XDefineCursor (d, frame, dot);
	  PaintSquare2 (x_square, y_square, foreground);
	  SetRasterBit (x_square, y_square, 1);
	  break;
	case 2: /* Middle button */
	  XDefineCursor (d, frame, exchange);
	  set = InvertRasterBit (x_square, y_square);
	  InvertSquare (x_square, y_square, set);
	  break;
	case 3: /* Right button */
	  XDefineCursor (d, frame, pirate);
          XSetForeground (d, raster_gc, background);
	  PaintSquare2 (x_square, y_square, background);
	  SetRasterBit (x_square, y_square, 0);
	  break;
	}
      XDrawPoint(d, raster2_window, raster_gc, x_square + 3, y_square + 3);
      x_square_prev = x_square;
      y_square_prev = y_square;
      raster_outdated = FALSE;
      changed = TRUE;
      break;

    case ButtonRelease: 
      XDefineCursor (d, frame, cross);
      XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
      XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
      break;
    
    case MotionNotify:
      if (WhatSquare (event, &x_square, &y_square))
        return;  /* mouse outside grid; really shouldn't happen, but... */
      if ((x_square != x_square_prev) || (y_square != y_square_prev))
      {
       	  switch (((XMotionEvent *)event)->state) {
	    case Button1Mask: /* left button down */
	      PaintSquare2 (x_square, y_square, foreground);
	      SetRasterBit (x_square, y_square, 1);
	      changed = raster_outdated = TRUE;
	      break;
	    case Button2Mask: /* middle button down */
	      set = InvertRasterBit (x_square, y_square);
	      InvertSquare (x_square, y_square, set);
	      changed = raster_outdated = TRUE;
	      break;
	    case Button3Mask: /* right button down */
	      PaintSquare2 (x_square, y_square, background);
	      SetRasterBit (x_square, y_square, 0);
	      changed = raster_outdated = TRUE;
	      break;
	    default: /* ignore events with multiple buttons down */
	      break; 
	    }
	    XDrawPoint(d, raster2_window, raster_gc, x_square + 3, y_square + 3);
      }
      if (raster_outdated && !MouseMovedEventQueued()) {
/*
	RepaintRaster();
	RepaintRaster2();
*/
	raster_outdated = FALSE;
	}
      x_square_prev = x_square;
      y_square_prev = y_square;
      break;
  
    }
}

int MouseMovedEventQueued () 
{
  XEvent event;
  if (XPending(d) == 0) return (FALSE);
  XPeekEvent (d, &event);
  return (event.type == MotionNotify);
}


ProcessFrameEvent (event)
  XEvent *event;
{
  char keybuf[10];

  if (event->type == KeyPress &&
      XLookupString (event, keybuf, sizeof keybuf, NULL, NULL) == 1 &&
      keybuf[0] == 'q' || keybuf[0] == 'Q' || keybuf[0] == '\003') {
    Quit ();
    return;
  }
  if (event->type != ConfigureNotify)
    return;
  if ((frame_height == ((XConfigureEvent *)event)->height)
     && (frame_width == ((XConfigureEvent *)event)->width))
     return;

  /* the frame size has changed.  Must rearrange subwindows. */
  frame_height = ((XConfigureEvent *)event)->height;
  frame_width = ((XConfigureEvent *)event)->width;
  LayoutStage2 ();
  XMapSubwindows (d, frame);
}
  
ProcessButtonEvent (command, event, win_no)
  struct command_data *command;
  XEvent *event;
  int    win_no;
{
  
  switch (event->type) {
    case Expose:
      if (((XExposeEvent *)event)->count)
      	break;  /* repaint only when last exposure is received */
      if (command->inverted)
        XClearWindow (d, command->window);
      XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
      XDrawString (d, command->window, gc, command->x_offset, font->ascent,
	 command->name, command->name_length);
      if (command->inverted)
      	InvertCommandWindow (command);
      break;

    case ButtonPress:
      if (button_down_command != NULL)
        break;  /* must be a second button push--ignore */
      button_down_command = command;
      InvertCommandWindow (command);
      command->inverted = TRUE;
      if (win_no == NEXTCH || win_no == PREVCH)
          IncrDecr(win_no);
      break;

    case LeaveNotify:
      if (command == button_down_command) {
	InvertCommandWindow (command);
	command->inverted = FALSE;
	button_down_command = NULL;
	}
      XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
      break;

    case ButtonRelease:
      if (command == button_down_command) {
	(*command->proc)(command->data);
	button_down_command = NULL;
	InvertCommandWindow (command);
	command->inverted = FALSE;
	}
      XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
      break;
      
    }
  }


InvertCommandWindow (command)
  struct command_data *command;
  {
  XSetState (d, gc, 1L, 0L, GXinvert, invertcolor);
  XFillRectangle (d, command->window, gc, 0, 0, 1000, 1000);
  }

	  
/* WhatSquare returns TRUE if mouse is outside grid, FALSE if inside.
   If it returns FALSE, it assigns to *x_square and *y_square. */

int WhatSquare (event, x_square, y_square)
  register XEvent *event;
  register int *x_square, *y_square; /*RETURN*/
  {
  int x = ((XButtonEvent *)event)->x;
  int y = ((XButtonEvent *)event)->y;
  if ((x < 0) || (y < 0))
    return (TRUE);
  *x_square = x/square_size;
  *y_square = y/square_size;
  return ((*x_square >= charWidth2) || (*y_square >= charHeight2));
  }


RepaintGridLines(how, which)
  int  how, which;
{
  if (which == 1)
     RepaintGridLinesPartially (0, 0, charWidth, charHeight, TRUE, which, how);
  else
     RepaintGridLinesPartially (0, 0, charWidth2, charHeight2, TRUE, which, how);
}

RepaintGridLinesPartially (x1, y1, x2, y2, include_boundaries, which, white)
  int x1, y1, x2, y2, which, white;
  int include_boundaries;
{
  register int i;
  int px1, px2, py1, py2;
  Window  win;

  if( which == 1 )
  {
	win = grid_window;
  }
  else
  {
	win = grid_window2;
	XSetForeground(d, raster_gc, foreground);
        XDrawRectangle(d, grid_window2, raster_gc, 0, 0, charWidth2 * square_size, charHeight2 * square_size);
/*
	if (invert_flag)
	    return;
*/
  }

  if (reverse)
	XSetForeground(d, raster_gc, background);
  py1 = y1*square_size;
  py1 += (py1 & 1);  /* make sure pattern is aligned on even bit boundary */
  py2 = y2*square_size;
  if (!include_boundaries) {x1++;x2--;}
  px1 = x1*square_size;
  for (i=x1;i<=x2; i++) {
     XDrawLine (d, win, raster_gc, px1, py1, px1, py2);
     px1 += square_size;
     }
  if (!include_boundaries) {x1--;x2++;}

  /* draw horizontal grid lines */
  px1 = x1*square_size;
  px1 += (px1 & 1);  /* make sure pattern is aligned on even bit boundary */
  px2 = x2*square_size;
  if (!include_boundaries) {y1++;y2--;}
  py1 = y1*square_size;
  for (i=y1;i<=y2;i++) {
     XDrawLine (d, win, raster_gc, px1, py1, px2, py1);
     py1 += square_size;
  }
}

RepaintWindow (x1, y1, x2, y2, paint_background)
  int x1, y1, x2, y2;
  int paint_background;
{
  int   i, j, k, m;
  char		 *ch2;
  unsigned char  ch;

  if (outdata[curch] == NULL)
	return;
  k = x1 % 8;
  XSetForeground(d, raster_gc, foreground);
  for (i = y1; i < y2; i++)
  {
    ch2 = outdata[curch] + char_row_bytes * i + x1 / 8;
    ch = 0x80 >> k;
    for (j = x1; j < x2; j++) 
    {
	m = *ch2 & ch;
	if (m)
      	   PaintSquare (j, i, foreground);
	ch = ch >> 1;
        if (ch == 0)
	{
	   ch = 0x80;
	   ch2++;
	}
    }
  }
}


RepaintWindow2 (x1, y1, x2, y2, paint_background)
  int x1, y1, x2, y2;
  int paint_background;
{
  int   i, j, k, m;
  char   	 *ch2;
  unsigned char  ch;

  if (outdata[curch] == NULL)
	return;
  k = x1 % 8;
  XSetForeground(d, raster_gc, foreground);
  for (i = y1; i < y2; i++)
  {
    ch2 = outdata[curch] + char_row_bytes * i + x1 / 8;
    ch = 0x80 >> k;
    for (j = x1; j < x2; j++) 
    {
	m = *ch2 & ch;
	if (m)
      	   PaintSquare2 (j, i, foreground);
	ch = ch >> 1;
        if (ch == 0)
	{
	   ch = 0x80;
	   ch2++;
	}
    }
  }
}

PaintSquare(x, y, pixel)
  int x, y;
  unsigned long pixel;
{
  XFillRectangle (d, grid_window, raster_gc, x*square_size + 1, y*square_size + 1,
    square_size - 1, square_size - 1);
}

PaintSquare2 (x, y, pixel)
  int x, y;
  unsigned long pixel;
{
/*
  XSetState (d, gc, pixel, 0L, GXcopy, AllPlanes);
*/
  XFillRectangle (d, grid_window2, raster_gc, x*square_size + 1, y*square_size + 1,
    square_size-1, square_size-1);
}

InvertSquare(x, y, set)
  int x, y, set;
{
  if (set)
     XSetForeground (d, raster_gc, foreground);
  else
     XSetForeground (d, raster_gc, background);
  XFillRectangle (d, grid_window2, raster_gc, x*square_size + 1, y*square_size + 1,
    square_size-1, square_size-1);
}




int InvertRasterBit (x, y)
  int  x, y;
{
  char		 *ch2;
  unsigned char  ch;
  int   ret;

  ch2 = outdata[curch] + char_row_bytes * y + x / 8;
  x = x % 8;
  ch = 0x80 >> x;
  *ch2 = *ch2 ^ ch;
  ret = *ch2 & ch;
  return(ret);
}


SetRasterBit (x, y, set)
  int  x, y, set;
{
  char		 *ch2;
  unsigned char  ch;

  ch2 = outdata[curch] + char_row_bytes * y + x / 8;
  x = x % 8;
  ch = 0x80 >> x;
  if (set)
      *ch2 = *ch2 | ch;
  else
  {
      ch = ch ^ 0xff;
      *ch2 = *ch2 & ch;
  }
}

RepaintRaster() 
{
  char  str[2];
  int   m, k, bytes;
  char   	 *ch2;
  unsigned char  ch;

  str[0] = curch;
  XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
  if (!use_var_font)
  {
     XDrawString(d, raster_window, raster_gc, 3, 3 + char_ascent, str, 1);
/*
     disp_info();
*/
     disp_code(curch);
     return;
  }
  XClearWindow(d, raster_window);
  XDrawRectangle (d, raster_window, raster_gc, 0, 0, charWidth+6, charHeight+6);
  if (indata[curch] == NULL)
      return;
  bytes = (charWidth + 7) / 8;
  for(m = 0; m < charHeight; m++)
  {
      ch2 = indata[curch] + bytes * m;
      ch = 0x80;
      for( k = 0; k < charWidth; k++)
      {
	  if (*ch2 & ch)
	     XDrawPoint(d, raster_window, raster_gc, k + 3, m + 3);
	  ch = ch >> 1;
	  if (ch == 0)
	  {
	     ch = 0x80;
	     ch2++;
	  }
      }
   }
}


disp_info()
{
   char   str[16];

  XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
  sprintf(str, "code: %d", curch);
  XDrawString(d, frame, gc, code_x, code_y + char_ascent3, str, strlen(str));
  sprintf(str, "size: %dx%d", charWidth, charHeight);
  XDrawString(d, frame, gc, code_x, sizey1 + char_ascent3, str, strlen(str));
  sprintf(str, "size: %dx%d", charWidth2, charHeight2);
  XDrawString(d, frame, gc, code_x, sizey2 + char_ascent3, str, strlen(str));
  XDrawLine(d, frame, raster_gc, base_x, base_y, base_x + LEFT_MARGIN, base_y);
}

RepaintRaster2 (set) 
 int  set;
{
  int  m, k;
  char   	 *ch2;
  unsigned char  ch;

  XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
  XClearWindow(d, raster2_window);
  XDrawRectangle (d, raster2_window, raster_gc, 0, 0, charWidth2+6, charHeight2+6);
  if (outdata[curch] == NULL && invert_flag)
  {
      XFillRectangle (d, raster2_window, raster_gc, 3, 3, charWidth2, charHeight2);
      return;
  }
  for(m = 0; m < charHeight2; m++)
  {
      ch2 = outdata[curch] + char_row_bytes * m;
      ch = 0x80;
      for( k = 0; k < charWidth2; k++)
      {
	  if (*ch2 & ch)
	     XDrawPoint(d, raster2_window, raster_gc, k + 3, m + 3);
	  ch = ch >> 1;
	  if (ch == 0)
	  {
	     ch = 0x80;
	     ch2++;
	  }
      }
   }
/*
   disp_info();
*/
   disp_code(curch);
}
	  

ReadInputFont(which, file)
  int   which;
  char *file;
{
   int   outp, count, bits;
   int   x, y, bytes;
   int   width, height, ascent;
   int   first, last;
   unsigned char ch_in, ch_bit;
   char  *pp, data[80];
   char  **font_data;
   FILE  *inf;

   if ((inf = fopen(file, "r")) == NULL)
	return;
   fgets(data, 80, inf);
   if (strncmp(data, "varian font", 11) != 0)
	return;
   fgets(data, 80, inf);
   sscanf(data, "%d%d%d", &width, &height, &ascent);
   fgets(data, 80, inf);
   sscanf(data, "%d%d", &first, &last);
   bytes = (width + 7) / 8;
   if ( which == 1)
   {
	charWidth = width;
	charHeight = height;
	char_ascent = ascent;
	font_data = indata;
	firstch = first;
	lastch = last;
   }
   else
   {
	charWidth2 = width;
	charHeight2 = height;
	char_ascent2 = ascent;
	font_data = outdata;
	out_first = first;
	out_last = last;
   }

   for(;;)
   {
	ch_in = fgetc(inf);
	if ((int)ch_in == 255)
	   break;
   }
   for (count = first; count <= last; count++)
   {
      if ((font_data[count] = (char *)malloc (bytes * height)) == NULL)
      {
	 fclose(inf);
	 return;
      }
      ch_bit = 0;
      for( y = 0; y < height; y++)
      {
        pp = font_data[count] + bytes * y;
      	bits = 0;
	for( x = 0; x < width; x++)
	{
	    if (ch_bit == 0)
	    {
		ch_in = fgetc(inf);
	 	ch_bit = 0x80;
	    }
	    if (ch_in & ch_bit)
	        *pp = (*pp << 1) | 0x01;
	    else
		*pp = (*pp << 1) & 0xfe;
	    ch_bit = ch_bit >> 1;
	    bits++;
	    if (bits == 8)
	    {
		pp++;
		bits = 0;
	    }
	}
	if (bits > 0)
	    *pp = *pp << (8 - bits);
      }
   }
   for (x = 0; x < 34; x++)
      ch_status[x] = fgetc(inf);
   fclose(inf);
   if (which == 1)
      use_var_font = 1;

}


OutputToFile (outf)
  FILE *outf;
{
   int   outp, count;
   int   x, y, char_bytes;
   char  ch_out, *ch_in;
   unsigned char  ch_b;

   StoreStatus(curch);
   fprintf (outf, "varian font %dx%d\n", charWidth2, charHeight2);
   fprintf(outf, "%d %d %d\n", charWidth2, charHeight2, char_ascent2);
   fprintf(outf, "%d %d\n", out_first, out_last);
   fprintf(outf, "%c", 255);
   char_bytes = (charWidth2 * charHeight2 + 7) / 8;
   outp = out_first;
   while (outp <= out_last)
   {
        if (outdata[outp] == NULL)
	{
	    for( x = 0; x < char_bytes; x++)
		fprintf(outf, "%c", 0);
	    outp++;
	    continue;
	}
	ch_out = 0;
	count = 0;
        for (y = 0; y < charHeight2; y++)
	{
	    ch_in = outdata[outp] + char_row_bytes * y;
	    ch_b = 0x80;
	    for (x = 0; x < charWidth2; x++)
	    {
		if (ch_b == 0)
		{
		    ch_b = 0x80;
		    ch_in++;
		}
		if (*ch_in & ch_b)
		    ch_out = (ch_out << 1) | 0x01;
		else
		    ch_out = (ch_out << 1) & 0xfe;
	  	ch_b = ch_b >> 1;
		count++;
		if (count == 8)
		{
		    fprintf(outf, "%c", ch_out);
		    count = 0;
		    ch_out = 0;
		}
	    }
	}
	if (count != 0)
	{
	    ch_out = ch_out << (8 - count);
	    fprintf(outf, "%c", ch_out);
	}
	outp++;
    }
    for (x = 0; x < 34; x++)
	fprintf(outf, "%c", ch_status[x]);
}


char *cify_name (name)
    char *name;
{
    int length = name ? strlen (name) : 0;
    int i;

    for (i = 0; i < length; i++) {	/* strncpy (result, begin, length); */
	char c = name[i];
	if (!((isascii(c) && isalnum(c)) || c == '_')) name[i] = '_';
    }
    return name;
}

char *StripName(name)
  char *name;
{
  char *begin = (char *)rindex (name, '/');
  char *end, *result;
  int length;

  begin = (begin ? begin+1 : name);
  end = (char *)index (begin, '.'); /* change to rindex to allow longer names */
  length = (end ? (end - begin) : strlen (begin));
  result = (char *) malloc (length + 1);
  strncpy (result, begin, length);
  result [length] = '\0';
  return (result);
}


char *BackupName(name)
  char *name;
{
  int name_length = strlen (name);
  char *result = (char *) malloc (name_length+4);
  sprintf(result, "%s.bk", name);
  return (result);
}

char *TmpFileName(name)
  char *name;
{
  int  name_length, tmp_length, result_length;
  char *tmp = "/tmp/";
  char *begin, *result;

  begin = (char *)rindex (name, '/');
  if (begin)
    name = begin+1;
  name_length = strlen (name);
  tmp_length = strlen (tmp);
  result_length = name_length + tmp_length;
  result = (char *) malloc (result_length + 1);
  strncpy (result, tmp, tmp_length);
  strncpy (result+tmp_length, name, name_length);
  result [result_length] = '\0';
  return (result);
}



LayoutStage1 ()
  {
  int widths [N_COMMANDS];
  int command_width = 0;
  int ypos = TOP_MARGIN;
  int fontHeight = font->ascent + font->descent;
  register int i;
  XSetWindowAttributes attrs;

  /* first determine how wide the commands should be */
  for (i=0;i<N_COMMANDS;i++) {
    register struct command_data *command = &commands[i];
    command->name_length = strlen (command->name);
    widths[i] = XTextWidth (font, command->name, command->name_length);
    if (command_width < widths[i])
      command_width = widths[i];
    }

  command_width += 4; /* so even widest command has a little space around it */

  /* now create the command windows.  Command strings will be centered in them */
  /* x position of commands will be determined later */

  attrs.win_gravity = UnmapGravity;
  attrs.event_mask =
      ButtonPressMask | ButtonReleaseMask | LeaveWindowMask | ExposureMask;
  attrs.background_pixel = background;

  for (i=0;i<N_COMMANDS;i++) {
    register struct command_data *command = &commands[i];
    command->x_offset = (command_width - widths[i])/2;
    command->window = XCreateWindow (d, frame, 0, ypos,
	command_width, fontHeight, 1, CopyFromParent, CopyFromParent,
        CopyFromParent, CWBackPixel | CWWinGravity | CWEventMask, &attrs);
    ypos += fontHeight + 5;
    if (i == 0 || i == 3 || i == 5 || i == 7 || i == 9 || i == 11 || i == 13)
      ypos += fontHeight + 5;
      /* for gaps between command groups;  pretty random! */
    }
  
  /* set up raster window; x position to be determined later */
  attrs.event_mask = ExposureMask;
  ypos += AROUND_RASTER_MARGIN;
  code_y = ypos;
  sizey1 = ypos + charHeight + 4;
  ypos = sizey1 + charHeight + 4;
  raster_window = XCreateWindow (d, frame, 0, ypos,
	charWidth + 6, charHeight + 6, 1, CopyFromParent, CopyFromParent,
	CopyFromParent, CWBackPixel | CWWinGravity | CWEventMask, &attrs);
  
  /* set up raster invert window; x position to be determined later */
  ypos += charHeight + 8 + AROUND_RASTER_MARGIN;
  sizey2 = ypos;
  ypos += charHeight + 4;
  raster2_window = XCreateWindow (d, frame, 0, ypos,
	charWidth2 + 6, charHeight2 + 6, 1, CopyFromParent, CopyFromParent,
	CopyFromParent, CWBackPixel | CWWinGravity | CWEventMask, &attrs);

  XSetWindowBackground(d, raster2_window, background);
  /* set up the grid window; width and height to be determined later */
  attrs.event_mask =  Button1MotionMask | Button2MotionMask | Button3MotionMask
    | ExposureMask | ButtonPressMask | ButtonReleaseMask;
  grid_window = XCreateWindow (d, frame, LEFT_MARGIN, TOP_MARGIN,
	1, 1, 0, CopyFromParent, CopyFromParent,
	CopyFromParent, CWBackPixel | CWWinGravity | CWEventMask, &attrs);

  grid_window2 = XCreateWindow (d, frame, LEFT_MARGIN, TOP_MARGIN + 10,
	1, 1, 0, CopyFromParent, CopyFromParent,
	CopyFromParent, CWBackPixel | CWWinGravity | CWEventMask, &attrs);

  /* assign global variables based on this layout */

  right_side_bottom = ypos + charHeight2
     + 2 /* borders */ + AROUND_RASTER_MARGIN;
  right_side_width = 2 /* borders */ + max (
     command_width + GRID_TO_COMMAND_MARGIN + RIGHT_MARGIN,
     AROUND_RASTER_MARGIN + charWidth2);
}



LayoutStage2 ()
{
  int grid_width, grid_height;
  int  f_width, f_height;
  int x_room, y_room, i;
  XWindowChanges changes;
  
  x_room = frame_width - 1 - LEFT_MARGIN - right_side_width;
  y_room = frame_height - 1 - TOP_MARGIN - BOTTOM_MARGIN;
  x_room /= max (charWidth2, charWidth);
  y_room /= (charHeight + charHeight2 + 1);
  square_size = min (x_room, y_room);
  if (square_size < 1 || frame_height < right_side_bottom) {
     int done = FALSE;
     while (!done) {
	int result;
	static char *strings[2] = { "Yes", "No"};

	result = alert(frame,
		"The window is not big enough; abort?",
		"Yes will abort the program, no will continue",
		strings, 2, alertHandler); 
	if (result == 0)
	   Quit();
	else
	   done = TRUE;
     }
  }
  if (square_size < 1) square_size = 1;

  /* set the grid window's dimensions */
  grid_width = (charWidth * square_size) + 1;
  grid_height = charHeight * square_size + 1;
  XResizeWindow (d, grid_window, grid_width, grid_height);
  grid_width = (charWidth2 * square_size) + 3;
  XResizeWindow (d, grid_window2, grid_width, (charHeight2 * square_size) + 3);
  changes.x = LEFT_MARGIN;
  changes.y = TOP_MARGIN + grid_height + square_size;
  XConfigureWindow (d, grid_window2, CWX | CWY, &changes);
  base_y = changes.y + square_size * char_ascent2 + square_size / 2;
  base_x = LEFT_MARGIN + grid_width + 4;

  /* set x positions of command windows */
  changes.x = 2 * LEFT_MARGIN + grid_width + GRID_TO_COMMAND_MARGIN;
  for (i=0;i<N_COMMANDS;i++)
    XConfigureWindow (d, commands[i].window, CWX, &changes);
  f_width = changes.x + right_side_width + LEFT_MARGIN + 20;

  /* set x offsets for raster and raster-inverted windows */
  changes.x = LEFT_MARGIN + grid_width + AROUND_RASTER_MARGIN;
  code_x = changes.x;
  XConfigureWindow (d, raster_window, CWX, &changes);
  XConfigureWindow (d, raster2_window, CWX, &changes);
  if (f_width < frame_width)
  {
      XResizeWindow (d, frame, f_width, frame_height);
  }
}


OuterWindowDims (square_size, right_side_width,
  right_side_bottom, width, height)
  int square_size, right_side_width, right_side_bottom;
  int *width, *height; /* RETURN */
{

  *width = 2 * LEFT_MARGIN + charWidth2*square_size + 1 + right_side_width;
  *height = TOP_MARGIN + (charHeight+charHeight2+1)*square_size + 1 + BOTTOM_MARGIN;
  if (*height < right_side_bottom)
    *height = right_side_bottom;
}


StoreStatus(num)
 int  num;
{
   int   m, k;
   unsigned char  pp;

   if (backup_num != num)  /* not current active code */
	return;
   m = curch / 8;
   k = curch % 8;
   pp = 0x80 >> k;
   if (invert_flag)
      ch_status[m] = ch_status[m] | pp;
   else 
      ch_status[m] = ch_status[m] & (~pp);
}


BackUpData(num)
 int  num;
{
   int  m, k;
   char *ch, *ch2;
   unsigned char pp;

   if (backup_data == NULL)
   {
      if ((backup_data = (char *)malloc (char_row_bytes * charHeight2)) == NULL)
	   return;
   }
   if (outdata[num] == NULL)
	return;
   backup_num = num;
   ch = backup_data;
   ch2 = outdata[num];
   for (m = 0; m < char_row_bytes * charHeight2; m++)
   {
        *ch = *ch2;
 	ch++;
	ch2++;
   }
   m = num / 8;
   k = num % 8;
   pp = 0x80 >> k;
   invert_flag = ch_status[m] & pp;
}
   

void UnDo()
{
   int  m, k;
   char *ch, *ch2;
   unsigned char pp;

   if (backup_data == NULL)
	return;
   if (backup_num != curch)
	return;
   ch = backup_data;
   ch2 = outdata[curch];
   m = curch / 8;
   k = curch % 8;
   pp = 0x80 >> k;
   invert_flag = ch_status[m] & pp;
   for (m = 0; m < char_row_bytes * charHeight2; m++)
   {
        *ch2 = *ch;
 	ch++;
	ch2++;
   }
   RepaintRaster2(invert_flag);
   ClearSetWin2(invert_flag);
   RepaintWindow2(0, 0, charWidth2, charHeight2, foreground);
}
   



void MoveLR(left)
 int  left;  /* 0: move right, 1: move left  */
{
   int   x, y;
   unsigned char  ch, *ch2;
   int   x2, y2;
   int   h, w;

   if ( outdata[curch] == NULL )
	return;
   x2 = charWidth2 % 8;
   if (!left)
   {
	if (invert_flag)
        {
	     ch = 0xff;
	     while (x2 > 1)
	     {
		 ch = ch >> 1 & 0x7f;
		 x2--;
	     }
	}
	else
	{
	     ch = 0;
	     while (x2 > 1)
	     {
		 ch = ch >> 1 | 0x80;
		 x2--;
	     }
	}
   }
	     
   for (y = 0; y < charHeight2; y++)
   {
      if (left)
      {
        ch2 = outdata[curch] + char_row_bytes * y;
	for (x = 0; x < char_row_bytes - 1; x++)
	{
	    *ch2 = *ch2 << 1;
	    if (*(ch2+1) & 0x80)
		*ch2 |= 0x01;
	    ch2++;
	}
	*ch2 = *ch2 << 1;
	if (invert_flag)
	    *ch2 |= 0x01;
        else
	    *ch2 &= 0xfe;
      }
      else
      {
        ch2 = outdata[curch] + char_row_bytes * (y+1) - 1;
	if (invert_flag)
	    *ch2 |= ch;
	else
	    *ch2 &= ch;
	for (x = 0; x < char_row_bytes - 1; x++)
	{
	    *ch2 = *ch2 >> 1;
	    if (*(ch2-1) & 0x01)
		*ch2 |= 0x80;
	    ch2--;
	}
	*ch2 = *ch2 >> 1;
	if (invert_flag)
	    *ch2 |= 0x80;
	else
	    *ch2 &= 0x7f;
      }
   }
   XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
   if (left)
   {
	x2 = square_size + 1;
        h = charHeight2 * square_size;
	w = (charWidth2 - 1) *square_size - 1;
	XCopyArea(d, grid_window2, grid_window2, raster_gc,x2, 1, w, h, 1, 1);
        h = charHeight2;
	w = charWidth2 - 1;
	XCopyArea(d, raster2_window, raster2_window, raster_gc,4, 3, w, h, 3, 3);
	x = charWidth2 - 1;
	if (invert_flag)
            XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
	else
            XSetState (d, raster_gc, background, foreground, GXcopy, AllPlanes);
	for( y = 0; y < charHeight2; y++)
        {
	    PaintSquare2 (x, y, foreground);
	    XDrawPoint (d, raster2_window, raster_gc, x+3, y+3);
	}
   }
   else
   {
	x = square_size + 1;
        h = charHeight2 * square_size;
	w = (charWidth2 - 1) *square_size - 1;
	XCopyArea(d, grid_window2, grid_window2, raster_gc, 1, 1, w, h, x, 1);
        h = charHeight2;
	w = charWidth2 - 1;
	XCopyArea(d, raster2_window, raster2_window, raster_gc,3, 3, w, h, 4, 3);
	x = 0;
	if (invert_flag)
            XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
	else
            XSetState (d, raster_gc, background, foreground, GXcopy, AllPlanes);
	for( y = 0; y < charHeight2; y++)
	{
	    PaintSquare2 (x, y, foreground);
	    XDrawPoint (d, raster2_window, raster_gc, x+3, y+3);
	}
   }
	     
}
	
void MoveUD(up)
 int  up;  /* 0: move down, 1: move up  */
{
   int   x, y, x2, y2, w, h;
   unsigned char  *ch1, *ch2, data;

   if ( outdata[curch] == NULL )
	return;
   for (y = 0; y < charHeight2 - 1; y++)
   {
      if (up)
      {
	 ch1 = outdata[curch] + char_row_bytes * y;
	 ch2 = ch1 + char_row_bytes;
      }
      else
      {
	 ch1 = outdata[curch] + char_row_bytes * (charHeight2 - 1 - y);
	 ch2 = ch1 - char_row_bytes;
      }
      for (x = 0; x < char_row_bytes; x++)
      {
	    *ch1 = *ch2;
            ch1++;
	    ch2++;
      }
   }
   if (up)
       ch1 = outdata[curch] + char_row_bytes * (charHeight2 - 1);
   else
       ch1 = outdata[curch];
   if (invert_flag)
	data = 0xff;
   else
	data = 0;
   for (x = 0; x < char_row_bytes; x++)
   {
	*ch1 = data;
	ch1++;
   }
   XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
   if (up)
   {
	y2 = square_size + 1;
        h = (charHeight2-1) * square_size - 1;
	w = charWidth2 *square_size;
	XCopyArea(d, grid_window2, grid_window2, raster_gc,1, y2, w, h, 1, 1);
        h = charHeight2 - 1;
	w = charWidth2;
	XCopyArea(d, raster2_window, raster2_window, raster_gc,3, 4, w, h, 3, 3);
	y = charHeight2 - 1;
	if (invert_flag)
            XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
	else
            XSetState (d, raster_gc, background, foreground, GXcopy, AllPlanes);
	for( x = 0; x < charWidth2; x++)
        {
	    PaintSquare2 (x, y, foreground);
	    XDrawPoint (d, raster2_window, raster_gc, x+3, y+3);
	}
   }
   else
   {
	y = square_size + 1;
        h = (charHeight2-1) * square_size - 1;
	w = charWidth2 *square_size;
	XCopyArea(d, grid_window2, grid_window2, raster_gc, 1, 1, w, h, 1, y);
        h = charHeight2 - 1;
	w = charWidth2;
	XCopyArea(d, raster2_window, raster2_window, raster_gc,3, 3, w, h, 3, 4);
	y = 0;
	if (invert_flag)
            XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
	else
            XSetState (d, raster_gc, background, foreground, GXcopy, AllPlanes);
	for( x = 0; x < charWidth2; x++)
	{
	    PaintSquare2 (x, y, foreground);
	    XDrawPoint (d, raster2_window, raster_gc, x+3, y+3);
	}
   }
}


void IncrDecr(but_no)
  int   but_no;
{
    unsigned long   mask;
    XEvent   event;

    XFlush(d);
    XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
    mask = ButtonPressMask | ButtonReleaseMask | LeaveWindowMask | StructureNotifyMask | ExposureMask;
    usleep(200000);
    if(XCheckMaskEvent (d, mask, &event))
    {
	ProcessEvent (&event);
	return;
    }
    usleep(200000);
    while (TRUE)
    {
	 if(XCheckMaskEvent (d, mask, &event))
	 {
	      switch (event.type) {
	      case ButtonPress:
			break;
	      default:
			ProcessEvent (&event);
			return;
			break;
	      }
	 }
	 if (but_no == NEXTCH)
	 {
	      if (curch == lastch)
                  curch = firstch;
	      else
		  curch++;
	 }
	 else
	 {
	      if (curch == firstch)
                  curch = lastch;
	      else
		  curch--;
	 }
	 disp_code(curch);
	 usleep(200000);
    }
}
   
disp_code(num)
 int  num;
{
    int   x, w, h;
    char  str[6];

    x = code_x + charWidth3 * 5;
    w = charWidth3 * 5;
    h = charHeight3 + 2;
    XClearArea (d, frame, x, code_y, w, h, FALSE);
    sprintf(str, " %d", num);
    XDrawString(d, frame, gc, x, code_y + char_ascent3, str, strlen(str));
}



int CheckStop()
{
    int   break_flag;
    unsigned long   mask;
    XEvent event;

    mask = ButtonPressMask | ButtonReleaseMask | LeaveWindowMask |
		 StructureNotifyMask | ExposureMask;
     XFlush(d);
     while (XCheckMaskEvent (d, mask, &event))
     {
         break_flag = 1;
	 switch (event.type) {
	  case ButtonRelease:
		if (event.xany.window == commands[0].window)
		{
		    commands[0].name = browse_start;
		    button_down_command = NULL;
    		    XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
    		    XClearWindow (d, commands[0].window);
    		    XDrawString (d, commands[0].window, gc, commands[0].x_offset,
		        font->ascent, browse_start, strlen(browse_start));
    		    if (commands[0].inverted)
      			InvertCommandWindow (&commands[0]);
		    commands[0].inverted = FALSE;
		    BackUpData(curch);
		    return (1);
		}
		break;
	   case ButtonPress:
	   case LeaveNotify:
		break;
	   default:
		break_flag = 0;
		ProcessEvent (&event);
		break;
	 }
	 if (break_flag)
		break;
      }
      return(0);
}


void Browse()
{

    commands[0].name = browse_stop;
    XClearWindow (d, commands[0].window);
    XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
    XDrawString (d, commands[0].window, gc, commands[0].x_offset, font->ascent,
	 browse_stop, strlen(browse_stop));
    if (commands[0].inverted)
      	InvertCommandWindow (&commands[0]);
    XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
    XFlush(d);
    while (TRUE)
    {
 	 if (CheckStop())
	     return;
	 usleep(200000);
         if (curch == lastch)
	    curch = firstch;
	 else
	    curch++;
	 DispChar(3);
 	 if (CheckStop())
	     return;
	 usleep(300000);
 	 if (CheckStop())
	     return;
	 usleep(200000);
 	 if (CheckStop())
	     return;
	 usleep(200000);
    }
}
   



void DispChar(cond)
int cond;
{
  int   m, k;
  char   *ch2, str[16];
  unsigned char  pp;
  unsigned long  pixel;

   if (cond < 2)
       StoreStatus(curch);
   switch (cond) {
     case 0:   /*  display previous code */
	    if (curch > firstch)
		curch--;
	    else if (firstch > 0)
		 {
			firstch--;
			curch = firstch;
		 }
	    break;
     case 1:   /*  display next code */
	    if (curch < lastch)
		curch++;
	    else if (curch < CHARNUM - 1)
	    {
		lastch++;
		curch = lastch;
	    }
	    else
		curch = firstch;
	    break;
     default:   /*  display current code */
	    break;
   }

  XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
  XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
  ClearSource();
  if (outdata[curch] == NULL)
  {
    if ((outdata[curch] = (char *)malloc (char_row_bytes * charHeight2)) == NULL)
    {
	fprintf(stderr, " could not allocate memory...\n");
	exit(0);
    }
    ch2 = outdata[curch];
    for( m = 0; m < char_row_bytes * charHeight2; m++)
        *ch2++ = 0;
  }
  if (use_var_font)
      DrawVarChar();
  else
      DrawXChar();
  if (cond != 2)
  {
      m = curch / 8;
      k = curch % 8;
      pp = 0x80 >> k;
      invert_flag = ch_status[m] & pp;
  }
  RepaintRaster2(invert_flag);
  ClearSetWin2(invert_flag);
  RepaintWindow2(0, 0, charWidth2, charHeight2, foreground);
  if (cond < 2)
      BackUpData(curch);
}

DrawXChar()
{
  int    m, k;
  char   str[2];
  unsigned long  pixel;

  str[0] = curch;
  XSetForeground(d, bit_gc, 0);
  XFillRectangle(d, plCharmap, bit_gc, 0, 0, charWidth, charHeight);
  XSetForeground(d, bit_gc, 1);
  XDrawString(d, plCharmap, bit_gc, 0, char_ascent, str, 1);
  XDrawString(d, raster_window, raster_gc, 3, 3 + char_ascent, str, 1);
  if (char_image != NULL)
     XDestroyImage(char_image);
    
  char_image = XGetImage(d, plCharmap, 0, 0, charWidth, charHeight,
                         1, XYPixmap);
  for (m = 0; m < charHeight; m++)
  {
      for (k = 0; k < charWidth; k++)
      {
          pixel = XGetPixel(char_image, k, m);
          if (pixel)
	      PaintSquare(k, m, foreground);
      }
  }
}


DrawVarChar()
{
   int     x, y, bytes;
   unsigned char    ch_bit, *data;

   
   if (indata[curch] == NULL)
	return;
   bytes = (charWidth + 7) / 8;
   for( y = 0; y < charHeight; y++)
   {
	data = indata[curch] + bytes * y;
	ch_bit = 0x80;
        for(x = 0; x < charWidth; x++)
	{
	    if (*data & ch_bit)
		PaintSquare(x, y, foreground);
	    ch_bit = ch_bit >> 1;
	    if (ch_bit == 0)
	    {
		ch_bit = 0x80;
		data++;
	    }
	}
   }
   RepaintRaster();
}
   


void CopyAllChar()
{
   int    m, k, y;
   char   *ch;

   for( m = firstch; m <= lastch; m++)
   {
       if (outdata[m] == NULL)
       {
          if ((outdata[m] = (char *)malloc (char_row_bytes * charHeight2)) == NULL)
          {
	     fprintf(stderr, " could not allocate memory...\n");
             disp_code(curch);
	     return;
	  }
       }
       ch = outdata[m];
       for (k = 0; k < charHeight2 * char_row_bytes; k++)
	     *ch++ = 0;
       disp_code(m);
       if (use_var_font)
             CopyByBit(m);
       else
       {
  	   if (char_image != NULL)
      		  XDestroyImage(char_image);
   	   char_image = NULL;
           CopyByPix(m);
	}
   }
   if (char_image != NULL)
      	XDestroyImage(char_image);
   char_image = NULL;
   disp_code(curch);
   DispChar(3);
}
	


void CopyChar()
{
   int    m, k;
   char   *ch2;

   XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
   ClearSetWin2(0);
   for (m = 0; m < charHeight2; m++)
   {
	ch2 = outdata[curch] + char_row_bytes * m;
	for (k = 0; k < char_row_bytes; k++)
        {
	    *ch2 = 0;
	    ch2++;
	}
   }
   if (use_var_font)
        CopyByBit(curch);
   else
        CopyByPix(curch);
   RepaintRaster2(invert_flag);
   RepaintWindow2(0, 0, charWidth2, charHeight2, foreground);
}



CopyByBit(ch_num)
 int   ch_num;
{
   int    x, y, bytes;
   char   *ch1, *ch2;

   if (indata[ch_num] == NULL)
	return;
   ch2 = outdata[ch_num];
   bytes = (charWidth + 7) / 8;
   for (y = 0; y < charHeight && y < charHeight2; y++)
   {
      ch1 = indata[ch_num] + bytes * y;
      ch2 = outdata[ch_num] + char_row_bytes * y;
      for (x = 0; x < bytes && x < char_row_bytes; x++)
      {
	   *ch2 = *ch1;
	   ch1++;
	   ch2++;
      }
   }
}
	

CopyByPix(num)
 int   num;
{
   int    x, y, bytes;
   char   str[2], *ch2;
   unsigned char  ch;
   unsigned long  pixel;

   if (char_image == NULL)
   {
        str[0] = num;
	XSetForeground(d, bit_gc, 0);
	XFillRectangle(d, plCharmap, bit_gc, 0, 0, charWidth, charHeight);
	XSetForeground(d, bit_gc, 1);
	str[0] = num;
	XDrawString(d, plCharmap, bit_gc, 0, char_ascent, str, 1);
	XDrawString(d, raster_window, raster_gc, 3, 3 + char_ascent, str, 1);
	if (char_image != NULL)
     	   	XDestroyImage(char_image);
	char_image = XGetImage(d, plCharmap, 0, 0, charWidth, charHeight,
                        1, XYPixmap);
   }
   for (y = 0; y < charHeight && y < charHeight2; y++)
   {
      ch2 = outdata[num] + char_row_bytes * y;
      ch = 0x80;
      for (x = 0; x < charWidth && x < charWidth2; x++)
      {
          pixel = XGetPixel(char_image, x, y);
          if (pixel)
	      *ch2 = *ch2 | ch;
	  ch = ch >> 1;
	  if (ch == 0)
	  {
	      ch = 0x80;
	      ch2++;
	  }
      }
   }
}


void CopyScaleAll()
{
   int    m, k, y;
   char   *ch;

   for( m = firstch; m <= lastch; m++)
   {
       if (outdata[m] == NULL)
       {
          if ((outdata[m] = (char *)malloc (char_row_bytes * charHeight2)) == NULL)
          {
	     fprintf(stderr, " could not allocate memory...\n");
             disp_code(curch);
	     return;
	  }
       }
       ch = outdata[m];
       for (k = 0; k < charHeight2 * char_row_bytes; k++)
	     *ch++ = 0;
       disp_code(m);
       if (use_var_font)
	     CopyScaleByBit(m);
       else
       {
	     if (char_image != NULL)
     	   	XDestroyImage(char_image);
	     char_image = NULL;
	     CopyScaleByPix(m);
       }
   }
   if (char_image != NULL)
      	XDestroyImage(char_image);
   char_image = NULL;
   disp_code(curch);
   DispChar(3);
}
	

void CopyScaleChar()
{
   int    m, k;
   unsigned char  ch;
   char     *ch2;

   if (outdata[curch] == NULL)
	return;
   XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
   ClearSetWin2(0);
   for (m = 0; m < charHeight2; m++)
   {
	ch2 = outdata[curch] + char_row_bytes * m;
	for (k = 0; k < char_row_bytes; k++)
        {
	    *ch2 = 0;
	    ch2++;
	}
   }
   if (use_var_font)
	CopyScaleByBit(curch);
   else
        CopyScaleByPix(curch);
   RepaintRaster2(invert_flag);
   RepaintWindow2(0, 0, charWidth2, charHeight2, foreground);
}

CopyScaleByBit(num)
  int  num;
{
   int    x, y, x2, y2, countx, county;
   int    bytes;
   unsigned char   ch, ch3_bit, *ch1, *ch2, *ch3;

   if (indata[num] == NULL)
	return;
   y2 = 0;
   bytes = (charWidth + 7) / 8;
   for( y = 0; y < charHeight && y2 < charHeight2; y++)
   {
      county = multy * (y + 1);
      ch1 = indata[num] + bytes * y;
      while (y2 < county && y2 < charHeight2)
      {
         ch2 = outdata[num] + char_row_bytes * y2;
         ch = 0x80;
	 x2 = 0;
         for (x = 0; x < charWidth && x2 < charWidth2; x++)
         {
	    countx = multx * (x + 1);
	    ch3 = ch1 + x / 8;
            ch3_bit = 0x80 >> (x % 8);
	    while (x2 < countx && x2 < charWidth2)
	    {
	       if (*ch3 & ch3_bit)
		 *ch2 = *ch2 | ch;
	       ch = ch >> 1;
	       if (ch == 0)
	       {
		 ch = 0x80;
		 ch2++;
	       }
	       x2++;
	    }
	  }
	  y2++;
       }
    }
}

	    
	

CopyScaleByPix(num)
  int   num;
{
   int  m, k, m2, k2, countx, county;
   char    *ch2, str[2];
   unsigned char   ch;
   unsigned long  pixel;

   if (char_image == NULL)
   {
        str[0] = num;
	XSetForeground(d, bit_gc, 0);
	XFillRectangle(d, plCharmap, bit_gc, 0, 0, charWidth, charHeight);
	XSetForeground(d, bit_gc, 1);
	str[0] = num;
	XDrawString(d, plCharmap, bit_gc, 0, char_ascent, str, 1);
	XDrawString(d, raster_window, raster_gc, 3, 3 + char_ascent, str, 1);
	if (char_image != NULL)
     	   	XDestroyImage(char_image);
	char_image = XGetImage(d, plCharmap, 0, 0, charWidth, charHeight,
                        1, XYPixmap);
   }
   m2 = 0;
   for (m = 0; m < charHeight; m++)
   {
      county = multy * (m + 1);
      while (m2 < county)
      {
         ch2 = outdata[num] + char_row_bytes * m2;
         ch = 0x80;
	 k2 = 0;
         for (k = 0; k < charWidth; k++)
         {
	    countx = multx * (k + 1);
            pixel = XGetPixel(char_image, k, m);
	    while (k2 < countx)
	    {
	       if (pixel)
		 *ch2 = *ch2 | ch;
	       ch = ch >> 1;
	       if (ch == 0)
	       {
		 ch = 0x80;
		 ch2++;
	       }
	       k2++;
	    }
	  }
	  m2++;
       }
    }
}



ClearSource()
{
  XSetState (d, raster_gc, background, 0L, GXcopy, AllPlanes);
  XClearWindow (d, grid_window);
  XClearWindow (d, raster_window);
/*
  XFillRectangle (d, frame, raster_gc, code_x, code_y, charWidth3 * 10, charHeight3+ 2);
*/
  XSetState (d, raster_gc, foreground, 0L, GXcopy, AllPlanes);
  RepaintGridLines (0, 1);
}

void ClearOrSetAll(set)
  int set;  /* 0 for clear, 1 for set */
{
  register int x, y;
  char      new = (set ? ~0: 0);
  char     *ch2;

   XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
  for (y = 0; y < charHeight2; y++)
  {
      ch2 = outdata[curch] + char_row_bytes * y;
      for (x = 0; x < char_row_bytes; x++)
      {
	  *ch2 = new;
	  ch2++;
      }
  }
  reverse = set;
  ClearSetWin2(set);
  XSetForeground (d, raster_gc, foreground);
  RepaintRaster2(set);
  RepaintWindow2(0, 0, charWidth2, charHeight2, foreground);
}


ClearSetWin2(set)
int   set;
{
  register int x, y;
  char      new = (set ? ~0: 0);
  char     *ch2;

  changed = TRUE;
  XClearWindow(d, grid_window2);
  RepaintGridLines (set, 2);
}
   

void InvertAll() 
{
  int  x, y;
  char *ch2;

  for (y = 0; y < charHeight2; y++)
  {
	ch2 = outdata[curch] + char_row_bytes * y;
	for (x = 0; x < char_row_bytes; x++)
	{
	    *ch2 = *ch2 ^ 0xff;
	    ch2++;
	}
  }
  changed = TRUE;
  if (invert_flag)
	invert_flag = 0;
  else
	invert_flag = 1;
  if (reverse)
	reverse = 0;
  else
        reverse = 1;
fprintf(stderr, " reverse= %d\n", reverse);
  ClearOrSetAll(reverse);
/*
  XSetState (d, raster_gc, background, foreground, GXcopy, AllPlanes);
  XFillRectangle (d, grid_window2, raster_gc, 1, 1,
    charWidth2*square_size, charHeight2*square_size);
  XSetState (d, raster_gc, foreground, background, GXcopy, AllPlanes);
  if (invert_flag)
     XDrawRectangle(d, grid_window2, raster_gc, 0, 0, charWidth2 * square_size, charHeight2 * square_size);
  else
     RepaintGridLines (invert_flag, 2);
*/
  RepaintRaster2(invert_flag);
  RepaintWindow2(0, 0, charWidth2, charHeight2, foreground);
}




void alertHandler (event)
  XEvent *event;
{
  if (event->type == Expose || event->type == ConfigureNotify)
  	ProcessEvent (event);
}

enum output_error {e_rename, e_write};

/* WriteOutput returns TRUE if output successfully written, FALSE if not */

int WriteOutput() 
{
  FILE *file;

  if (rename (filename, backup_filename) && errno != ENOENT)
    return (HandleError(e_rename));
  file = fopen (filename, "w+");
  if (!file)
    return (HandleError(e_write));
  OutputToFile (file);
  fclose (file);
  changed = FALSE;
  return (TRUE);
}



int HandleError(e)
  enum output_error e;
{
  int result;
  static char *strings[] = {"Yes", "No"};
  char msg1[120], msg2[120];
  char *tmp_filename;

  if (e == e_rename)
    sprintf (msg1, "Can't rename %s to %s -- %s",
      filename, backup_filename, sys_errlist[errno]);
  else
    sprintf (msg1, "Can't write on file %s -- %s",
      filename, sys_errlist[errno]);
  tmp_filename = TmpFileName (filename);
  sprintf (msg2, "Do you want to write output to file %s?", tmp_filename);
  result = alert (frame, msg1, msg2, strings, 2, alertHandler);

  if (result == 0)  /* "yes" */
  {
    filename = tmp_filename;
    free (backup_filename);
    backup_filename = BackupName (filename);
    return (WriteOutput());
  }
  else   /* "no" */
  {
    free (tmp_filename);
    return (FALSE);
  }
}

    
void Quit()
{
  int result;
  static char *strings[3] = {"Yes", "No", "Cancel"};
  if (changed) {
    result = alert (frame, 
      "Save changes before quitting?", "", strings, 3, alertHandler);
      
    switch (result) {
      case 0:     /* "yes" */
      	if (WriteOutput())
	  exit(0);
	else return;
      case 1:    /* "no" */
        exit(0);
      default:  /* "cancel" */
      	return;
      }
    }

  exit(0);
}




int alert (w, msg1, msg2, labels, choices, input_handler)
  Window w;
  char *msg1, *msg2;
  char **labels;
  int choices;
  int (*input_handler) ();
{
  int x, y, gap;
  int msg1_width, msg2_width;
  int choice_width = 0;
  int height, width, min_width, i, result;
  struct alert_data data;
  XSetWindowAttributes attrs;
  

  data.msg1 = msg1;
  data.msg2 = msg2;
  data.msg1_length = strlen (msg1);
  data.msg2_length = strlen (msg2);
  data.command_info = (struct message_data *) malloc
    (choices*sizeof (struct message_data));

  msg1_width = XTextWidth (font, msg1, strlen(msg1));
  msg2_width = XTextWidth (font, msg2, strlen(msg2));
  for (i=0;i<choices;i++)
  {
    struct message_data *cmd = &data.command_info[i];
    cmd->name = labels[i];
    cmd->name_length = strlen (cmd->name);
    cmd->name_width = XTextWidth (font, cmd->name, cmd->name_length);
    if (cmd->name_width > choice_width)
      choice_width = cmd->name_width;
  }
  choice_width += 8;

  attrs.border_pixel = bordercolor;
  attrs.background_pixel = background;
  attrs.cursor = cross;
  attrs.event_mask = ExposureMask;
  attrs.override_redirect = 1;
  width = max (msg1_width, msg2_width) + LEFT_MARGIN + RIGHT_MARGIN;
  min_width = choices*choice_width + (choices+1)* 10;
  height = 3*charHeight3 + 2* ROW_GAP + TOP_MARGIN + BOTTOM_MARGIN;
  DeterminePlace (w, &x, &y);
  width = max (width, min_width);
  gap = (width - choices*choice_width)/(choices+1);
  data.w = XCreateWindow (d, RootWindow(d, screen), x, y, width, height,
    5, CopyFromParent, CopyFromParent, CopyFromParent,
    CWBorderPixel | CWBackPixel | CWOverrideRedirect | CWCursor | CWEventMask, &attrs);

  x = gap;
  y = TOP_MARGIN + 2*(charHeight3 +  ROW_GAP);
  for (i=0;i<choices;i++)
  {
    register struct message_data *command = &data.command_info[i];
    command->x_offset = (choice_width - command->name_width)/2;
    command->window = XCreateSimpleWindow (d, data.w, x, y, choice_width,
      charHeight3, 1, bordercolor, background);
    XSelectInput (d, command->window, 
       ButtonPressMask | ButtonReleaseMask | ExposureMask | LeaveWindowMask);
    x += (gap + choice_width);
  }
  
  XMapWindow (d, data.w);
  XMapSubwindows (d, data.w);
  
  while (1)
  {
    struct message_data *command = NULL;
    XEvent event;
    XNextEvent (d, &event);
    if (event.xany.window == data.w) {
      ProcessAlertEvent (&data, &event);
      continue;
      }
    for (i=0;i<choices;i++)
      if (event.xany.window == data.command_info[i].window) {
	command = &data.command_info[i];
	break;
	}
    if (command) {
       result = ProcessChoice (&data, command, &event);
       if (result >= 0)
          break;
       }
    else 
       /* event doesn't belong to any of the dialog box's windows.
          Send it back to the calling application. */
      (*input_handler) (&event);
  }
  
  XDestroyWindow (d, data.w);

  free ((char *)data.command_info);
  return (result);
}


static int ProcessChoice (data, command, event)
  struct alert_data *data;
  struct message_data *command;
  XEvent *event;
{
  static struct message_data *button_down_command = NULL;
  
  switch (event->type) {
    
    case Expose:
      if (event->xexpose.count == 0) {
	XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
	XDrawString (d, command->window, gc, command->x_offset,
	   font->ascent, command->name, command->name_length);
	}
      break;

    case ButtonPress:
      if (button_down_command != NULL)
        break;  /* must be second button press; ignore it */
      button_down_command = command;
      InvertChoice (command);
      break;

    case LeaveNotify:
      if (command == button_down_command) {
	InvertChoice (command);
	button_down_command = NULL;
	}
      break;

    case ButtonRelease:
      if (command == button_down_command) {
	button_down_command = NULL;
        return (command - data->command_info);
        }
      break;
  }

  return (-1);
}


ProcessAlertEvent (data, event)
  struct alert_data *data;
  XEvent *event;
{
   int  y;

   if (event->type == Expose && event->xexpose.count == 0) 
   {
       y = TOP_MARGIN + font->ascent;
       XSetState (d, gc, foreground, background, GXcopy, AllPlanes);
       XDrawString (d, data->w, gc, LEFT_MARGIN, y,
       	  data->msg1, data->msg1_length);
       y += font->descent +  ROW_GAP + font->ascent;
       XDrawString (d, data->w, gc, LEFT_MARGIN, y,
          data->msg2, data->msg2_length);
   }
}


InvertChoice (command)
  struct message_data *command;
{
    XSetState (d, gc, 1L, 0L, GXinvert, invertcolor);
    XFillRectangle (d, command->window, gc, 0, 0, 400, 400);
}


DeterminePlace (w, px, py)
  Window w;
  int *px, *py;
{
  int x, y;
  Window win; 

   XTranslateCoordinates (d, w, RootWindow (d, screen), 0, 0, &x, &y, &win);
   *px = max (0, x + 10);
   *py = y + frame_height / 2 - 40;
}

