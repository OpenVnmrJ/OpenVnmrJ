/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
/* 
 */

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  MAIN processing window.  It creates canvas, user-interface for a 	*
*  specific processing, and build a communication link with the 	*
*  main user-interface process.						*
*									*
*************************************************************************/
#include <stdio.h>
#include <signal.h>
#include <stream.h>
#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#include <X11/Intrinsic.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <new.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>

/* Note that xview.h uses K&R C varargs.h which        */
/* is not compatible with stdarg.h for C++ or ANSI C.	*/

#ifdef __OBJECTCENTER__
#ifdef va_start
#undef va_start
#undef va_end
#undef va_arg
#endif
#endif

#include <stdarg.h>
#ifdef LINUX
#include "generic.h"
#else
#include <generic.h>
#endif
#include "initstart.h"
#include "message.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "msgprt.h"
#include "inputwin.h"
#include "filelist_id.h"
#include "contrast.h"
#include "win_process.h"
#include "win_stat.h"
#include "ibcursors.h"
#include "file_format.h"
#include "macroexec.h"
#include "magicfuncs.h"
#include "win_stat.h"
#include "vs.h"
#include "vscale.h"
#include "voldata.h"
#include "interrupt.h"
#include "imagelist.h"

#define R_OK 4		/* Not defined in Saber version of unistd.h */

#define debug name2(/,/)

// ID identification of the caller used in updating current time
#define	TIME_ID	1
#define MOVIE_TICK 2  

// This program version number.  It is used to set up communication
// (message) link with the ib_ui process
#define	VERSNUMBER	1

// The following defines the number of window message buffers
#define WIN_MSGBUF_LIMIT 4
#define MSGBUF_SIZE 128

char win_msgbuf[WIN_MSGBUF_LIMIT][MSGBUF_SIZE];	// buffer for whole message
char msgbuf[(MSGBUF_SIZE+1)*WIN_MSGBUF_LIMIT];  // buffer for sub messages
int msgbuf_stack_index = 0;

void show_msgbuf();
void init_msgbuf();
void push_msg(char *format, ...);
void pop_msg();

extern void win_graphics_redraw();

extern "C" 
{
/*   strftime(char *, int, char *, struct tm *);*/
#ifndef LINUX
   gethostname(char *, int);
#endif
   int rtu_ok(char *);
}

/* this stuff added so fullscreen zoom can know them  DKL 12-2-91 */
int canvas_width = 0 ;
int canvas_height = 0 ;
int canvas_stx = 0 ;
int canvas_sty = 0 ;
static char startupfile[256];

/* MAIN Window handlers */
typedef struct _wingraph
{
   Frame frame;
   Canvas canvas;
} Wingraph;

static Wingraph *wgraph;
static Gdev *gdev;

static ImagelistList startup_images;

/* Functions in this file */
static void window_create_objects(int);
static Canvas window_create_canvas(Canvas);
static Frame window_create_frame(Frame, int);
#ifdef BASEFRAME_MENU
static void baseframe_menu_notify(void);
#endif BASEFRAME_MENU
static void window_create_file_browser(void);
static void window_create_msgprt(void);
static void window_create_contrast(void);
static void window_create_vscale(void);
static void quit_proc(void);
static void win_canvas_handler(Xv_window win, Event *e);
void win_print_msg(char *format, ...);
void canvas_repaint_proc(void);
static void win_handler_msg(Ipgmsg *msg);
static void canvas_resize_proc(void);
static void time_func(int);
static void run_out_memory_handler(void);
static void write_err_message(char *, char *);
static void write_info_message(char *, char *);
static void push_imagelist_from_file(ImagelistList *ilist, char *ifile);
static void usage(char *progname);

/************************************************************************
*                                                                       *
*  Main Program.                                                        *
*                                                                       */
int
main(int argc, char **argv)
{
   extern void init_msg_handler(char *);
   char receiver_host[128];
   char *argstr;
   char *progname;
   int groupid;
   int i;
   int gotmacro = FALSE;

   // Use BROWSERDIR as image-browser environment name.  It is only called
   // once
   init_set_env_name("BROWSERDIR");

   // Delete last argument from command line
   groupid = atoi(argv[--argc]); // Master window ID
   if (!groupid){
       fprintf(stderr,"%s should not be called directly\n", argv[0]);
       exit(1);
   }
   // Delete the next-to-last argument too
   progname = argv[--argc];	// Original calling name
   
   gethostname(receiver_host, sizeof(receiver_host)); // Hostname

   // Strip off Xview command line arguments (and initialize Xview)
   xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);

   // Parse argument list
   strcpy(startupfile,"startup");// Default startup file
   argc--; argv++;		// Skip prog name
   while (argc){
       argc--;
       argstr = *argv++;
       if (strcmp(argstr, "-image") == 0){
	   if (argc < 1){
	       usage(progname);
	       exit(-1);
	   }else{
	       Imagelist *il = new Imagelist(*argv);
	       startup_images.Push(il);
	       argv++; argc--;
	   }
       }else if (strcmp(argstr, "-imagelist") == 0){
	   if (argc < 1){
	       usage(progname);
	       exit(-1);
	   }else{
	       push_imagelist_from_file(&startup_images, *argv);
	       argv++; argc--;
	   }
       }else if (!gotmacro && *argstr != '-'){
	   // A "bare" argument should be a macro name
	   sprintf(startupfile,"%.255s", argstr);
	   char path[MAXPATHLEN];
	   char filepath[MAXPATHLEN];
	   init_get_env_name(path);
	   sprintf(filepath,"%s/macro/%s", path, startupfile);
	   if (access(filepath, R_OK)){
	       fprintf(stderr,"IB error: macro \"%s\" not found in %s/macro/\n",
		       startupfile, path);
	       usage(progname);
	       exit(-1);
	   }
       }else{
	   usage(progname);
	   exit(-1);
       }
   }

   // Create window objects
   window_create_objects(groupid);

   // Create server images for color-choice icons
   initialize_color_chips(gdev);

   // Create error message window and informative message window
   window_create_msgprt();

   // Notify to call quit_proc when window object is destroyed in unusual manner
   notify_interpose_destroy_func(wgraph->frame, (Notify_func)quit_proc);

   // Create file browser to browse the file
   window_create_file_browser();

   // Create graphics tool for user-interactive
   Gtools::create(wgraph->frame, gdev);

   // Create communication link between this process and ib_ui process
   Message::create((u_long)win_handler_msg, VERSNUMBER, "MESSAGE_KEY", 
	      receiver_host);

   // Send our PID to the ib_ui so that this process can be
   // interrupted by a signal SIGUSR1 (sent by ib_ui)
   // Catch the interrupt signal (sent by ib_ui process)
   /* This requires the Message handler to have been created! */
   interrupt_register(wgraph->frame, UI_COMMAND, CANCEL, SIGUSR1);

   // Register callback function to call 'time_func' every 1 second
   ipgwin_register_user_func(TIME_ID, 1, 0, (void(*)(int))time_func);

   // Callback routine for run out memory (used by command 'new')
   set_new_handler(run_out_memory_handler);

   // Create contrast tool for adjusting colormap
   window_create_contrast();
   Vscale::contrast_init();

   // Create the intensity scaling window (must be after Gtools creation)
   window_create_vscale();
   Vscale::vscale_init();

   // Create the statistics window
   Win_stat::create();

   // Create user on-line input window (for miscellaneous uses)
   inputwin_create(wgraph->frame, "INPUT_WIN");

   browser_loop();

   /*NOTREACHED*/
   return 0;
}

/************************************************************************
*									*
*  Start up the image browser looping
*									*/
void
browser_loop()
{
    // fprintf(stderr,"browser_loop()\n");
    ipgwin_main_loop(wgraph->frame);
    /*NOTREACHED*/
}

/************************************************************************
*									*
*  Create window objects with its graphics device.			*
*									*/
void
window_create_objects(int groupid)
{
   Siscms *siscms;
   char initname[128];	// init file name

   // Allocate memory for window object
   if ((wgraph = (Wingraph *)calloc(1, sizeof(Wingraph))) == NULL)
   { 
      STDERR("window_create_objects(): cannot allocate memory"); 
      exit(1); 
   }

   // Create frame
   if ((wgraph->frame = window_create_frame(NULL, groupid)) == NULL)
   {
      STDERR("window_create_objects(): cannot create frame");
      exit(1);
   }

   // Create canvas
   if ((wgraph->canvas = window_create_canvas(wgraph->frame)) == NULL)
   {
      STDERR("window_create_objects: cannot create canvas");
      exit(1);
   }

   // Get initialized colormap file
   (void)init_get_cmp_filename(initname); 

   // Create Siscms colormap structure and load the colormap from 
   // initialized file.
   // Note that the order of colorname is important because that is the 
   // order the colormap (red/gren/blue) will be loaded.                
   if ((siscms = (Siscms *)siscms_create(initname, "mark-color",
       "gray-color", "false-color")) == NULL)
   {
      STDERR("window_create_objects(): cannot create siscms");
      exit(1);
   }

   // Create graphics device (bind it to this canvas).  This gdev will be
   // used in every graphics drawing into this canvas
   if ((gdev = (Gdev *)g_device_create(wgraph->canvas, siscms))
       == NULL)
   {
      STDERR("window_create_objects(): cannot create graphics device");
      exit(1);
   }
}

static void
done_proc()
{
    xv_set(wgraph->frame, XV_SHOW, FALSE, NULL);
}

/************************************************************************
*									*
*  Create frame for canvas.						*
*									*/
static Frame
window_create_frame(Frame owner, int groupid)
{
   Frame win;           // frame handler
   int xpos, ypos;      // window position
   int wd, ht;          // window size
   char initname[128];	// init file
   int i;

   // Get the initialized file for graphics window position
   (void)init_get_win_filename(initname); 

   // Get the position of the graphics window
   if (init_get_val(initname, "MAIN_GRAPHICS", "dddd",
		    &xpos, &ypos, &wd, &ht) == NOT_OK)
   {
      // Defaults
      xpos = 0;
      ypos = 60;
      wd = 740;
      ht = 740;
   }

   win = xv_create(owner,       FRAME_CMD,
		   XV_X,           xpos,
		   XV_Y,           ypos,
		   XV_WIDTH,       wd,
		   XV_HEIGHT,      ht,
		   FRAME_DONE_PROC, done_proc,
		   FRAME_SHOW_LABEL,	FALSE,
		   FRAME_SHOW_FOOTER,  	TRUE,
		   FRAME_LEFT_FOOTER,	"Informative messages.......",
		   FRAME_SHOW_RESIZE_CORNER, TRUE,
		   XV_VISUAL_CLASS, PseudoColor,
		   NULL);
   Display *display = (Display *)xv_get(win, XV_DISPLAY);
   int xid = (int)xv_get(win, XV_XID);
   if (groupid){
       XSetTransientForHint(display, xid, groupid);
   }
   int atom = XInternAtom(display, "_MOTIF_WM_HINTS", FALSE);
   if (atom != None){
       // Set Motif hint for no header bar on frame.
       int props[4] = {2, 0, 6, 0};
       XChangeProperty(display, xid, atom, atom, 32, PropModeReplace,
		       (unsigned char *)props, 4);
   }

   // Set the icon
   /* Maybe do this if no groupid? */
   /*char iconname[1024];
   init_get_env_name(iconname);
   strcat(iconname, "/graphics.bm");
   if (access(iconname, R_OK) != 0){
       char msg[1024];
       sprintf(msg,"Warning: icon image file not found: \"%s\"", iconname);
       STDERR(msg);
   }else{
       Server_image icon_image = (Server_image)xv_create(NULL, SERVER_IMAGE,
				   SERVER_IMAGE_BITMAP_FILE, iconname,
				   NULL);
       Icon icon = (Icon)xv_create(win, ICON,
				   ICON_IMAGE, icon_image,
				   XV_WIDTH, 65,
				   XV_HEIGHT, 65,
				   NULL);
       xv_set(win, FRAME_ICON, icon, NULL);
       }*/

   return(win);
}

/************************************************************************
*									*
*  Create main canvas.							*
*									*/
static Canvas
window_create_canvas(Frame owner)
{
    Canvas win;		// canvas handler
    char initname[128];	// initialization start-up filename

    (void)init_get_win_filename(initname);

    win = xv_create(owner,			CANVAS,
		    XV_X,			0,
		    XV_Y,			0,
		    XV_WIDTH,			WIN_EXTEND_TO_EDGE,
		    XV_HEIGHT,			WIN_EXTEND_TO_EDGE,
		    WIN_BORDER,			FALSE,
		    CANVAS_RESIZE_PROC,		canvas_resize_proc,
		    CANVAS_REPAINT_PROC,	canvas_repaint_proc,
		    OPENWIN_SHOW_BORDERS,	FALSE,
		    NULL);

   // Register canvas event to handler mouse
   xv_set(canvas_paint_window(win),
	  WIN_CONSUME_EVENTS,
	  /*WIN_NO_EVENTS,*/
	  LOC_DRAG,
	  LOC_MOVE,
	  WIN_MOUSE_BUTTONS,
	  WIN_ASCII_EVENTS,    
	  NULL,
	  WIN_EVENT_PROC,    win_canvas_handler,
	  NULL);
   return(win);
}

/************************************************************************
*									*
*  Create filebrowser.							*
*									*/
static void
window_create_file_browser(void)
{
   int x_pos, y_pos;	// position of window
   int name_width;	// width of filename (in pixel) 
   int num_names;	// number of filenames showed at one time 
   char initname[128];	// initialization start-up filename

   (void)init_get_win_filename(initname);

   // Get the initialized file-browser window position and its size
   if (init_get_val(initname, "FILE_BROWSER", "dddd", &x_pos, &y_pos,
       &name_width, &num_names) == NOT_OK)
   {
      // Default
      x_pos = y_pos = 0;
      name_width = 0;
      num_names = 0;
   }

   // Create file-browser
   if (filelist_win_create(wgraph->frame, x_pos, y_pos, name_width,
       num_names)  == NOT_OK)
   {
      STDERR("window_create_file_browser:filelist_win_create");
      exit(1);
   }

   // Register the notify functions for saving and loading images.
   filelist_notify_func(FILELIST_WIN_ID, FILELIST_NEW,
			(long)&Frame_routine::load_data_file,
			(long)&Gframe::menu_save_image,
			(long)&Frame_routine::load_data_all);

   // Register the notify functions for saving error and info message
   // into a file
   filelist_notify_func(FILELIST_ERRMSG_ID, FILELIST_NEW, NULL,
	(long)write_err_message);
   filelist_notify_func(FILELIST_INFOMSG_ID, FILELIST_NEW, NULL,
	(long)write_info_message);
}

/************************************************************************
*									*
*  Create contrast tool to adjust colormap.				*
*									*/
static void
window_create_contrast(void)
{
   int x_pos, y_pos;	// position of window
   int width, height;	// width and height of window 
   char initname[128];	// initialization start-up filename

   // Get the initialized filename
   (void)init_get_win_filename(initname);

   // Get the contrast window position
   if (init_get_val(initname, "CONTRAST_WIN", "dddd", &x_pos, &y_pos,
       &width, &height) == NOT_OK)
   {
      // Default
      x_pos = 400;
      y_pos = 40;
      width = 140;
      height = 140;
   }

   // Create contrast window
   contrast_win_create(wgraph->frame, x_pos, y_pos, width, height, 
      gdev->siscms, G_Get_Stcms2(gdev), G_Get_Sizecms2(gdev));
}

/************************************************************************
*									*
*  Create vscale tool to adjust intensity mapping.
*									*/
static void
window_create_vscale(void)
{
    int x_pos, y_pos;	// position of window
    int width, height;	// width and height of window 
    char initname[128];	// initialization start-up filename

    // Get the initialized filename
    (void)init_get_win_filename(initname);

    // Get the vscale window position
    if (init_get_val(initname, "VSCALE_WIN", "dddd", &x_pos, &y_pos,
		     &width, &height) == NOT_OK)
    {
	// Default
	x_pos = 400;
	y_pos = 40;
	width = 350;
	height = 350;
    }

    // Create vscale window
    vs_win_create(wgraph->frame, x_pos, y_pos, width, height, 
		  gdev->siscms,
		  G_Get_Stcms2(gdev), G_Get_Sizecms2(gdev));
}

/************************************************************************
*									*
*  Create msgerr and msginfo.						*
*									*/
static void
window_create_msgprt(void)
{
   int x_pos, y_pos;	// position of window
   int width, height;	// width and height of window 
   char initname[128];	// initialization start-up filename

   // Get the initialized file
   (void)init_get_win_filename(initname);

   // Get the info-message window position
   if (init_get_val(initname, "INFOMSG_WIN", "dddd", &x_pos, &y_pos,
       &width, &height) == NOT_OK)
   {
      // Default
      x_pos = y_pos = 0;
      width = 0;
      height = 0;
   }

   // Create info-message window
   msginfo_win_create(wgraph->frame, x_pos, y_pos, width, height);

   // Create macro-display window
   msgmacro_win_create(wgraph->frame, x_pos, y_pos, width, height);

   // Get the error-message window position
   if (init_get_val(initname, "ERRMSG_WIN", "dddd", &x_pos, &y_pos,
       &width, &height) == NOT_OK)
   {
      // Default
      x_pos = y_pos = 0;
      width = 0;
      height = 0;
   }

   // Create error-message window
   msgerr_win_create(wgraph->frame, x_pos, y_pos, width, height);
}

/************************************************************************
*									*
*  Exit the program.							*
*									*/

void
quit_proc(void)
{
   // Destroy all message object. This routine MUST be called 
   // before exiting.                                        
   Message::destroy();

   exit(0);
}

/************************************************************************
*                                                                       *
*  Internal routine to show contents of msgbuf in frame left footer area*
*									*/

void
show_msgbuf() {
  
  msgbuf[0] = 0;
  for (int i = 0; i < WIN_MSGBUF_LIMIT; i++) {
    strcat(msgbuf, win_msgbuf[i]);
  }
  xv_set(wgraph->frame, FRAME_LEFT_FOOTER, msgbuf, NULL);
}

void
push_msg(char *format, ...)
{
   va_list vargs;	// variable argument pointer

   msgbuf_stack_index++;
   if (msgbuf_stack_index >= WIN_MSGBUF_LIMIT ||
       msgbuf_stack_index < 0) {
     return;
   }
   
   va_start(vargs, format);
   (void)vsprintf(win_msgbuf[msgbuf_stack_index], format, vargs);
   va_end(vargs);
   show_msgbuf();
}

void
init_msgbuf() {
  
  for (int i = 0; i < WIN_MSGBUF_LIMIT; i++) {
    win_msgbuf[i][0] = 0;
  }
  msgbuf[0] = 0;
  msgbuf_stack_index = -1;
  xv_set(wgraph->frame, FRAME_LEFT_FOOTER, msgbuf, NULL);
}

void
pop_msg()
{

  msgbuf_stack_index--;
  if (msgbuf_stack_index >= WIN_MSGBUF_LIMIT || msgbuf_stack_index < 0) {
    return;
  }
   
  win_msgbuf[msgbuf_stack_index][0] = 0;
  show_msgbuf();
}

/************************************************************************
*                                                                       *
*  Output the message in indexed window footer.				*
*									*/

void
win_print_msg(char *format, ...)
{
   va_list vargs;	// variable argument pointer

   va_start(vargs, format);
   (void)vsprintf(win_msgbuf[0], format, vargs);
   va_end(vargs);
   show_msgbuf();
}


/************************************************************************
*                                                                       *
*  Output the message in the window footer.				*
*									*/

void
win_print_msg_field(int index, char *format, ...)
{
   va_list vargs;	// variable argument pointer

   if (index < 0 || index >= WIN_MSGBUF_LIMIT) return;

   va_start(vargs, format);
   (void)vsprintf(win_msgbuf[index], format, vargs);
   va_end(vargs);

   show_msgbuf();
}

/************************************************************************
*                                                                       *
*  Move the pointer (cursor) to a given location
*									*/
void
warp_pointer(int x, int y)
{
    Display *disp = (Display *)xv_get(wgraph->canvas, XV_DISPLAY);
    Window win = (Window)xv_get(wgraph->canvas, XV_XID);
    XWarpPointer(disp, None, win, 0, 0, 0, 0, x, y);
}

/************************************************************************
*									*
*  Set the cursor shape
*  Choices are defined in "ibcursors.h"
*  Returns the previous cursor shape.
*									*/
int
set_cursor_shape(int curs)
{
    static int current_cursor = IBCURS_SELECT_POINT;

    Canvas win = wgraph->canvas;

    int previous_cursor = current_cursor;
    current_cursor = curs;
    
    // Get some stuff for Xlib calls
    Display *display = (Display *)xv_get(win, XV_DISPLAY);
    
    // Set the cursor from the predefined cursor font.
    // This works fine, but the choices are limited.
    Cursor curs_draw = XCreateFontCursor(display, curs);
    XDefineCursor(display,
		  xv_get(xv_get(win, CANVAS_NTH_PAINT_WINDOW, 0), XV_XID),
		  curs_draw);
    XFlush(display);

    return previous_cursor;
}

/************************************************************************
*                                                                       *
*  Canvas event handler.						*
*									*/
static void
win_canvas_handler(Xv_window win, Event *event)
	// Note that it actually takes 4 arguments, but the last
	// two are not used (Notify_arg arg, Noitify_event_type type)
	// Note tha win is also not used.
{
  WARNING_OFF(win);
 Gtools::handler_event(event);
}



void
FlushGraphics(void)
{
    XFlush(gdev->xdpy);
}


/************************************************************************
*									*
*  canvas_repaint_proc gets called in response to a "Redisplay" request *
*  either from the user or from the window manager (xview REPAINT)      *
*									*
*************************************************************************/

void
canvas_repaint_proc(void)
{
  extern void redraw_all_roitools();

  char fname[1024];
  static char *mac_path[] = {"", NULL};
  
  Frame_routine::redraw_frame();

  static int firsttime = TRUE;
  if (firsttime){
      firsttime = FALSE;

      /* this stuff added so fullscreen zoom can know them  DKL 12-2-91 */
      canvas_width = (int) xv_get(wgraph->canvas, XV_WIDTH);
      canvas_height = (int) xv_get(wgraph->canvas, XV_HEIGHT);
      canvas_stx = (int) xv_get(wgraph->canvas, XV_X);
      canvas_sty = (int) xv_get(wgraph->canvas, XV_Y);

      init_get_env_name(fname);
      strcat(fname, "/macro");
      mac_path[0] = strdup(fname);
      magical_register_user_func_finder(MacroExec::getFunc);
      magical_set_macro_search_path(mac_path);
      magical_set_error_print(msgerr_print_string);
      magical_set_info_print(msginfo_print_string);
      char *mcmd = (char *)malloc(strlen(startupfile) + 2);
      sprintf(mcmd,"%s\n", startupfile);
      loadAndExec(mcmd);	/* Causes problems w/ Purify! -- CMP*/
      free(mcmd);

      // Load images passed on command line
      // This may overwrite images loaded by the "startupfile"
      ImagelistIterator ilist(&startup_images);
      Imagelist *item;
      while (item = ++ilist){
	  Frame_routine::load_data_file("", item->Filename());
      }
  }
}


/************************************************************************
*									*
*  canvas_resize_proc gets called in response to an XVIEW resize request*
*									*
*************************************************************************/
void
canvas_resize_proc(void)
{
    // debug << "canvas_resize_proc()\n" ;
    if (wgraph && wgraph->canvas) {
	/* this stuff added so fullscreen zoom can know them  DKL 12-2-91 */
	canvas_width = (int) xv_get(wgraph->canvas, XV_WIDTH);
	canvas_height = (int) xv_get(wgraph->canvas, XV_HEIGHT);
	canvas_stx = (int) xv_get(wgraph->canvas, XV_X);
	canvas_sty = (int) xv_get(wgraph->canvas, XV_Y);
    }
    Roitool::allocate_bkg();
}

/************************************************************************
*                                                                       *
*  Handle messages sent by window user-interface.			*
*									*/
static void
win_handler_msg(Ipgmsg *msg)
{
  extern void winpro_movie_show();
  extern void winpro_line_show();
  extern void winpro_point_show();

  win_print_msg(msg->msgbuf);
  switch ((Define_ui)msg->minor_id)  {
  case FILE_LOAD:
    filelist_win_show(FILELIST_WIN_ID, FILELIST_LOAD_ALL, NULL, 
		      "File Browser: Data");
    break;
  case FILE_LOAD_ALL:
    msgerr_print("Function combined with FILE_LOAD");
    break;
  case FILE_SAVE:
    filelist_win_show(FILELIST_WIN_ID, FILELIST_SAVE, NULL,
		      "File Browser: Data");
    break;

  case FILE_FORMAT:
    Fileformat::show_window();
    break;

  case FILE_RESTART:
    system("ib_graphics&");
    quit_proc();
    
  case FILE_EXIT:
    quit_proc();
    break;

  case VIEW_REDISPLAY:
    xv_set(wgraph->frame, XV_SHOW, TRUE, NULL);
    canvas_repaint_proc();
    break;
    
  case PROCESS_STATISTICS:
    Win_stat::win_statistics_show();
    break;
  case PROCESS_ROTATION:
    winpro_rotation_show();
    break;
  case PROCESS_ARITHMETICS:
    winpro_arithmetic_show();
    break;
  case PROCESS_MATH:
    winpro_math_show();
    break;
  case PROCESS_HISTOGRAM:
    winpro_histenhance_show();
    break;
  case PROCESS_FILTERING:
    winpro_filter_show();
    break;
  case PROCESS_CURSOR:
    winpro_point_show();
    break;
  case PROCESS_LINE:
    winpro_line_show();
    break;
  case MOVIE:
    winpro_movie_show();
    break;
  case TOOLS_GRAPHICS:
    Gtools::show_tool();
    break;
  case TOOLS_COLORMAP:
    contrast_win_show();
    break;
  case TOOLS_SLICER:
    VolData::extract_slices(NULL);
    break;
  case MISC_INFO_MSG:
  case MISC_INFO_MSG_SHOW:
    msginfo_print((char *)NULL);
    break;
  case MISC_INFO_MSG_CLEAR:
    msginfo_write(NULL);
    break;
  case MISC_INFO_MSG_WRITE:
    filelist_win_show(FILELIST_INFOMSG_ID, FILELIST_SAVE, NULL,
		      "File Browser: Info Messages");
    break;
  case MISC_ERR_MSG:
  case MISC_ERR_MSG_SHOW:
    msgerr_print((char *)NULL);
    break;
  case MISC_ERR_MSG_CLEAR:
    msgerr_write((char *)NULL);
    break;
  case MISC_ERR_MSG_WRITE:
    filelist_win_show(FILELIST_ERRMSG_ID, FILELIST_SAVE, NULL,
		      "File Browser: Error Messages");
    break;
  case MISC_TIME:
    time_func(0);
    break;
  case MACRO_EXECUTE:
    MacroExec::show_window();
    /*MacroExec::execMacro("testmac");*/
    break;
  case MACRO_RECORD:
    fprintf(stderr,"MACRO_RECORD\n");
    break;
  case MACRO_SHOW:
    msgmacro_print((char *)NULL);
    break;
  case MACRO_SAVE:
    fprintf(stderr,"MACRO_SAVE\n");
    break;
  }
}


/************************************************************************
*                                                                       *
*  Get the x,y,width and height of a given canvas (default is main)     *
*									*/
static void
get_canvas_size(int& x, int& y, int& width, int& height, Canvas canvas)
{
  x = (int)xv_get(canvas, XV_X);
  y = (int)xv_get(canvas, XV_Y);
  width = (int)xv_get(canvas, XV_WIDTH);
  height = (int)xv_get(canvas, XV_HEIGHT);
}

/************************************************************************
*                                                                       *
*  This routine displays a time in the Right bottom corner of a frame.	*
*  It also sets/resets the time.					*
*  (This function is registered to be called every 1 second.)		*
*									*/
static void
time_func(int id)	// id = 0: set/reset time
			// id = TIME_ID: update time
{
   char buf[128];		// buffer for message date
   static int toggle=TRUE;	// toggle to turn on/off the time
   struct tm *tmm;		// tm structure pointer

   if (id == 0){
      // Toggle between set and reset
      if (toggle){
	 toggle = FALSE;
         ipgwin_register_user_func(TIME_ID, 1, 0, (void (*)(int))NULL);
         xv_set(wgraph->frame, FRAME_RIGHT_FOOTER, "", NULL);
      }else{
	 toggle = TRUE;
         ipgwin_register_user_func(TIME_ID, 1, 0, (void (*)(int))time_func);
      }
   }else{
      // id = TIME_ID
      time_t temp = time(NULL);
      tmm = localtime(&temp);
      strftime(buf,128,"%Y %h %e %T %Z", tmm);
      xv_set(wgraph->frame, FRAME_RIGHT_FOOTER, buf, NULL);
   }
}


/************************************************************************
*                                                                       *
*  This routine will be called if the process runs out of memory for	*
*  'new'.								*
*									*/
static void
run_out_memory_handler()
{
   STDERR("run_out_memory_handler:The process runs out of memory");
   alarm(1);
   sleep(10);
   quit_proc();
}

/************************************************************************
*									*
*  Save the info window to a file.
*  [MACRO interface]
*  argv[0]: (char *) Optional file path
*  [STATIC Function]							*
*									*/
int
info_write(int argc, char **argv, int, char **)
{
    argc--;
    argv++;

    char *dir;
    char *fname = NULL;
    if (argc > 1){
	ABORT;
    } else if (argc == 1) {
	fname = argv[0];
	if (fname[0] == '/') {
	    // Absolute path
	    dir = "";
	} else {
	    dir = get_filelist_dir(FILELIST_INFOMSG_ID);
	}
	write_info_message(dir, fname);
    } else {
	filelist_win_show(FILELIST_INFOMSG_ID, FILELIST_SAVE, NULL,
			  "File Browser: Info Messages");
    }
    return PROC_COMPLETE;
}

/************************************************************************
*                                                                       *
*  Write info message from Textsw into a file.				*
*									*/
static void
write_info_message(char *dirpath, char *filename)
{
   char fullname[128];

   if (*filename == NULL)
   {
       msgerr_print("write_info_message:Must specify a filename to be saved");
       return;
   }

   (void)sprintf(fullname, "%s/%s", dirpath, filename);
   if (msginfo_write(fullname) == NOT_OK)
      msgerr_print("write_info_message:Couldn't save info message into %s",
      fullname);
   macroexec->record("info_save('%s')", fullname);
}

/************************************************************************
*                                                                       *
*  Write error message from Textsw into a file.				*
*									*/
static void
write_err_message(char *dirpath, char *filename)
{
   char fullname[128];

   if (*filename == NULL)
   {
       msgerr_print("write_err_message:Must specify a filename to be saved");
       return;
   }

   (void)sprintf(fullname, "%s/%s", dirpath, filename);
   if (msgerr_write(fullname) == NOT_OK)
      msgerr_print("write_err_message:Couldn't save error message into %s",
      fullname);
}

Frame
get_base_frame()
{
    return wgraph->frame;
}

/************************************************************************
*                                                                       *
*  Read a list of image filepaths from the file "ifile" and
*  load them into the list "ilist".
*									*/
static void
push_imagelist_from_file(ImagelistList *ilist, char *ifile)
{
    char buf[MAXPATHLEN];
    char iname[MAXPATHLEN];
    Imagelist *il;

    FILE *fd = fopen(ifile, "r");
    if (!fd){
	sprintf(buf,"Cannot read image list file \"%s\"", ifile);
	msgerr_print(buf);
    }
    while (fgets(buf, sizeof(buf), fd)){
	if (*buf != '#' && strspn(buf," \t\n") < strlen(buf)){
	    sscanf(buf,"%s", iname);
	    il = new Imagelist(iname);
	    ilist->Push(il);
	}
    }
    fclose(fd);
}

/************************************************************************
*                                                                       *
*  Print out a usage message and abort.
*									*/
static void
usage(char *progname)
{
    fprintf(stderr, USAGE_MSG, progname);
}
