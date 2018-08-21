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
*  Routine related to create user-interface window and its controller.	*
*									*
*************************************************************************/

#include <stream.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "message.h"

#define R_OK 4		/* Not defined in Saber version of unistd.h */

// Font used for panel name
#define	MY_FONT_FAMILY	FONT_FAMILY_LUCIDA

// The gap pixel value between consecutive items
#define	PANEL_COMMAND_GAP	11

// Program version number (used by RPC message)
#define	VERSNUMBER	1

class Winui
{
   private:
      static int key_data;		// window unique key data
      static Xv_font font;
      static Panel panel;
      static Panel_item file;
//      static Panel_item parameters;
      static Panel_item view;
      static Panel_item process;
      static Panel_item tools;
      static Panel_item misc;
      static Panel_item movie;
      static Panel_item macro;
      static Panel_item cancel;

      // Process (PUID) where the signal will sent to
      // and signal type
      static int dest_process_pid;
      static int signal_type;

      // Creation routines
      static Frame window_create_frame(Frame);
      static Xv_font window_find_font(Frame, char *);
      static Panel window_create_panel(Frame, Xv_font);
      static Menu file_create_menu(Frame, Xv_font);
//      static Menu parameters_create_menu(Frame, Xv_font);
      static Menu view_create_menu(Frame, Xv_font);
      static Menu process_create_menu(Frame, Xv_font);
      static Menu tools_create_menu(Frame, Xv_font);
      static Menu misc_create_menu(Frame, Xv_font);
      static Menu macro_create_menu(Frame, Xv_font);

      // Callback functions
      static void menu_notify_handler(Menu, Menu_item);
      static void panel_notify_handler(Panel_item, Event *);
      static void quit_proc(void);

   public:
      static Frame frame;
      static void window_create_main_control_object(void);
      static void window_create_panel_items(void);

      // Callback function to handler message
      static void msg_handler(Ipgmsg *);
};

extern "C" {
    int set_color_icon(Display *display,
		  Window window,
		  char *filename,
		  int width,
		  int height);
    Pixmap get_icon_pixmap(Display *display,
			   Window window,
			   char *filename,
			   int width,
			   int height);
}

Frame Winui::frame=NULL;
int Winui::key_data=0;
Xv_font Winui::font=NULL;
Panel Winui::panel=NULL;
Panel_item Winui::file=NULL;
Panel_item Winui::view=NULL;
Panel_item Winui::process=NULL;
Panel_item Winui::tools=NULL;
Panel_item Winui::misc=NULL;
Panel_item Winui::movie=NULL;
Panel_item Winui::cancel=NULL;
int Winui::dest_process_pid=0;
int Winui::signal_type=0;
Panel_item Winui::macro = 0;

/************************************************************************
*									*
*  Main Program.							*
*									*/
main(int argc, char *argv[])
{
   int sav_argc = argc;
   char **sav_argv;
   int i;

   // Note: The last argv is the name of the startup script
   // Check for a -help argument
   for (i=1; i<=argc-1; i++){
       if (strcasecmp(argv[i], "-help") == 0){
	   fprintf(stderr, USAGE_MSG, argv[argc-1]);
	   // Continue, so that xv_init will print the XView usage
	   break;
       }
   }

   // Make 2 extra places, for window ID and terminating NULL
   sav_argv = (char **)malloc((argc+2) * sizeof(char *));
   for (i=0; i<argc; i++){
       sav_argv[i] = argv[i];
   }
   // Strip off Xview command line arguments
   xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 0);

   init_set_env_name("BROWSERDIR");

   // Create window object and its controller
   Winui::window_create_main_control_object();
   Winui::window_create_panel_items();

   // Create communication message with its version number VERSNUMBER
   Message::create((u_long)(&Winui::msg_handler), VERSNUMBER, "MESSAGE_KEY");

   // Start up the graphics program
   int pid = fork();
   if (!pid){
       // I am the child
       // Put the window ID on end of argv list
       int xid = (int)xv_get(Winui::frame, XV_XID);
       char xid_str[20];
       sprintf(xid_str,"%d", xid);
       /*fprintf(stderr,"ib_ui: xid=%s\n", xid_str);/*CMP*/
       sav_argv[sav_argc++] = xid_str;
       sav_argv[sav_argc] = NULL;
       sav_argv[0] = "ib_graphics";
       execvp(sav_argv[0], sav_argv);
#ifdef NOTDEFINED
       if (argc == 2){
#ifndef DEBUG
	   execlp("ib_graphics", "ib_graphics", xid_str, argv[1], NULL);
#else
	   fprintf(stderr,"ib_graphics %s %s\n", xid_str, argv[1]);
#endif
       }else{
#ifndef DEBUG
	   execlp("ib_graphics", "ib_graphics", xid_str, NULL);
#else
	   fprintf(stderr,"ib_graphics %s\n", xid_str);
#endif
       }
#endif /* NOTDEFINED */

       // Only get here on error
       perror("Can't start ib_graphics; execvp() error");
       exit(1);
   }
   free(sav_argv);

   // Loop for event
   ipgwin_main_loop(Winui::frame);

   return 0;
}

/************************************************************************
*									*
*  Create window objects for user-interface (frame, panel and its 	*
*  items).								*
*									*/
void
Winui::window_create_main_control_object(void)
{
   key_data = (int)xv_unique_key();

   // Create window object
   if ((frame = window_create_frame(NULL)) == NULL)
   {
      STDERR("cannot create frame");
      exit(1);
   }

   // Create font
   if ((font = window_find_font(frame, MY_FONT_FAMILY)) == NULL)
      STDERR_1("cannot find font: %s", MY_FONT_FAMILY);

   // Create window panel
   if ((panel = window_create_panel(frame, font)) == NULL)
   {
      STDERR("cannot create panel");
      exit(1);
   }

   // Notify to quit when window object is destroyed
   notify_interpose_destroy_func(Winui::frame, (Notify_func)(&Winui::quit_proc));
}

/************************************************************************
*									*
*  Create window frame.							*
*  Return created frame handler.					*
*									*/
Frame
Winui::window_create_frame(Frame owner)
{
   Frame win;		// frame handler
   int xpos, ypos;	// window position
   int wd, ht;		// window size
   char initname[128];  // init filename

   // Get the initialized filename
   (void)init_get_win_filename(initname);

   // Get the position of the control panel
   if (init_get_val(initname,"MAIN_CONTROL", "dddd",
		    &xpos, &ypos, &wd, &ht) == NOT_OK)
   {
      // Defaults
      xpos = 0;
      ypos = 0;
      wd = 920;
      ht = 34;
   }

   // Create window
   win = xv_create(owner, FRAME,
		   XV_X, xpos,
		   XV_Y, ypos,
		   XV_WIDTH, wd,
		   XV_HEIGHT, ht,
		   XV_LABEL, "Image Browser",
		   WIN_DYNAMIC_VISUAL, TRUE,
/*
		   XV_VISUAL_CLASS, PseudoColor,
*/
		   FRAME_SHOW_FOOTER, FALSE,
		   NULL);
   Display *display = (Display *)xv_get(win, XV_DISPLAY);
   int xid = (int)xv_get(win, XV_XID);
   int atom = XInternAtom(display, "_MOTIF_WM_HINTS", FALSE);
   if (atom != None){
       // Set Motif hint for no maximize button.
       // Ref: Andrew Taylor. "www.cpsc.ucalgary.ca/~davidt/x11/xwm.html"
       int props[4] = {2, 0, 65, 0};
       XChangeProperty(display, xid, atom, atom, 32, PropModeReplace,
		       (unsigned char *)props, 4);
   }

   // Set the icon
   char iconname[1024];
   init_get_env_name(iconname);
   strcat(iconname, "/browser.xpm2");

   Pixmap ipx = get_icon_pixmap(display, xid, iconname, 64, 64);
   if (ipx){
       Server_image iimg = (Server_image)xv_create(NULL, SERVER_IMAGE,
						   SERVER_IMAGE_PIXMAP, ipx,
						   SERVER_IMAGE_DEPTH, 8,
						   XV_WIDTH, 64,
						   XV_HEIGHT, 64,
						   NULL);
       Icon icon = xv_create(win, ICON,
			     ICON_IMAGE, iimg,
			     XV_VISUAL, DefaultVisual(display, 0),
			     NULL);
       xv_set(win, FRAME_ICON, icon, NULL);
   }else{
       // Preferred icon file not usable--take other steps
       init_get_env_name(iconname);
       strcat(iconname, "/ui.bm");
       if (access(iconname, R_OK) != 0){
	   char msg[1024];
	   sprintf(msg,"Warning: alternate icon image file not found: \"%s\"",
		   iconname);
	   STDERR(msg);
       }else{
	   Server_image icon_image =
	   (Server_image)xv_create(NULL, SERVER_IMAGE,
				   SERVER_IMAGE_BITMAP_FILE, iconname,
				   NULL);
	   Icon icon = (Icon)xv_create(win, ICON,
				       ICON_IMAGE, icon_image,
				       XV_WIDTH, 65,
				       XV_HEIGHT, 26,
				       NULL);
	   xv_set(win, FRAME_ICON, icon, NULL);
       }
   }
   return(win);
}

/************************************************************************
*									*
*  Find the font.							*
*									*/
Xv_font
Winui::window_find_font(Frame obj, char *name)
{
   Xv_font fnt;

   fnt = xv_find(obj,	FONT,
		FONT_FAMILY,	name,
		FONT_STYLE,	FONT_STYLE_BOLD,
		NULL);
   return(fnt);
}

/************************************************************************
*									*
*  Create window panel.							*
*  Return created panel handler.					*
*									*/
Panel
Winui::window_create_panel(Panel owner, Xv_font fnt)
{
   Panel win;

   win = xv_create(owner,	PANEL,
		XV_X,		0,
		XV_Y,		0,
		XV_WIDTH,	WIN_EXTEND_TO_EDGE,
		XV_HEIGHT,	WIN_EXTEND_TO_EDGE,
		XV_FONT,	fnt,
		WIN_BORDER,	FALSE,
		NULL);
   return(win);
}

/************************************************************************
*									*
*  Create panel items.							*
*									*/
void
Winui::window_create_panel_items(void)
{
   int x_pos;		/* starting position of panel item. The number is
			   incremented by the previous created panel-item
			   width plus the panel command gap. */
   int y_pos;		/* starting position of panel item, but not 
			   incremented. */
   
   // Initialize starting position value of panel controller item
   x_pos = 30;
   y_pos = 8;

   file = xv_create(panel,	PANEL_BUTTON,
		XV_X,			x_pos,
		XV_Y,			y_pos,
		PANEL_LABEL_STRING,	"File",
		PANEL_ITEM_MENU,	file_create_menu(NULL, font),
		NULL);
   x_pos += (int)xv_get(file, XV_WIDTH) + PANEL_COMMAND_GAP;
//   parameters = xv_create(panel,	PANEL_BUTTON,
//		XV_X,			x_pos,
//		XV_Y,			y_pos,
//		PANEL_LABEL_STRING,	"Parameters",
//		PANEL_ITEM_MENU,	parameters_create_menu(NULL, font),
//		NULL);
//   x_pos += (int)xv_get(parameters, XV_WIDTH) + PANEL_COMMAND_GAP;
//   view = xv_create(panel,	PANEL_BUTTON,
//		XV_X,			x_pos,
//		XV_Y,			y_pos,
//
////		PANEL_LABEL_STRING,	"View",
////		PANEL_ITEM_MENU,	view_create_menu(NULL, font),
//		PANEL_LABEL_STRING,	"Redisplay",
//		PANEL_NOTIFY_PROC,	(&Winui::panel_notify_handler),
//		XV_KEY_DATA,		key_data,	VIEW_REDISPLAY,
//
//		NULL);
//   x_pos += (int)xv_get(view, XV_WIDTH) + PANEL_COMMAND_GAP;
   view = xv_create(panel, PANEL_BUTTON,
		    XV_X, x_pos,
		    XV_Y, y_pos,
		    PANEL_LABEL_STRING, "Refresh",
		    PANEL_NOTIFY_PROC, (&Winui::panel_notify_handler),
		    XV_KEY_DATA, key_data, VIEW_REDISPLAY,
		    NULL);
   x_pos += (int)xv_get(view, XV_WIDTH) + PANEL_COMMAND_GAP;

   process = xv_create(panel,	PANEL_BUTTON,
		XV_X,			x_pos,
		XV_Y,			y_pos,
		PANEL_LABEL_STRING,	"Process",
		PANEL_ITEM_MENU,	process_create_menu(NULL, font),
		NULL);
   x_pos += (int)xv_get(process, XV_WIDTH) + PANEL_COMMAND_GAP;
   tools = xv_create(panel,	PANEL_BUTTON,
		XV_X,			x_pos,
		XV_Y,			y_pos,
		PANEL_LABEL_STRING,	"Tools",
		     //PANEL_NOTIFY_PROC,	(&Winui::panel_notify_handler),
		     //XV_KEY_DATA,		key_data,	TOOLS_GRAPHICS,
		PANEL_ITEM_MENU,	tools_create_menu(NULL, font),
		NULL);
   x_pos += (int)xv_get(tools, XV_WIDTH) + PANEL_COMMAND_GAP;
   misc = xv_create(panel,	PANEL_BUTTON,
		XV_X,			x_pos,
		XV_Y,			y_pos,
		PANEL_LABEL_STRING,	"Messages",
		PANEL_ITEM_MENU,	misc_create_menu(NULL, font),
		NULL);

   x_pos += (int)xv_get(misc, XV_WIDTH) + PANEL_COMMAND_GAP;
   movie = xv_create(panel,	PANEL_BUTTON,
		XV_X,			x_pos,
		XV_Y,			y_pos,
		XV_KEY_DATA,		key_data,	MOVIE,
		PANEL_LABEL_STRING,	"Movie ...",
		PANEL_NOTIFY_PROC,	(&Winui::panel_notify_handler),
		NULL);
   x_pos += (int)xv_get(movie, XV_WIDTH) + PANEL_COMMAND_GAP;
   macro = xv_create(panel,	PANEL_BUTTON,
		XV_X,			x_pos,
		XV_Y,			y_pos,
		XV_KEY_DATA,		key_data,	MACRO_EXECUTE,
		PANEL_LABEL_STRING,	"Macro ...",
		PANEL_NOTIFY_PROC,	(&Winui::panel_notify_handler),
		NULL);
//   macro = xv_create(panel,	PANEL_BUTTON,
//		XV_X,			x_pos,
//		XV_Y,			y_pos,
//		PANEL_LABEL_STRING,	"Macro",
//		PANEL_ITEM_MENU,	macro_create_menu(NULL, font),
//		NULL);

   x_pos += (int)xv_get(macro, XV_WIDTH) + PANEL_COMMAND_GAP;
   cancel = xv_create(panel,	PANEL_BUTTON,
		XV_X,			x_pos,
		XV_Y,			y_pos,
		XV_KEY_DATA,		key_data,	CANCEL,
		PANEL_LABEL_STRING,	"Cancel",
		PANEL_NOTIFY_PROC,	(&Winui::panel_notify_handler),
		      PANEL_INACTIVE, TRUE,
		NULL);
}

/************************************************************************
*									*
*  Create file menu.							*
*  Return created menu.							*
*									*/
Menu
Winui::file_create_menu(Frame owner, Xv_font fnt)
{
   Menu menu;

   menu = xv_create(owner,	MENU,
   		XV_FONT,		fnt,
		MENU_NOTIFY_PROC,	(&Winui::menu_notify_handler),
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	FILE_LOAD,
			MENU_STRING,	"Load Data ...",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	FILE_SAVE,
			MENU_STRING,	"Save Data ...",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	FILE_FORMAT,
			MENU_STRING,	"Output Format ...",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	FILE_EXIT,
			MENU_STRING,	"Exit",
			NULL,
		NULL);
   return(menu);
}

/************************************************************************
*									*
*  Create parameter menu.						*
*  Return created menu.							*
*									*/
//Menu
//Winui::parameters_create_menu(Frame owner, Xv_font fnt)
//{
//   Menu menu;
//
//   menu = xv_create(owner,	MENU,
//   		XV_FONT,		fnt,
//		MENU_NOTIFY_PROC,	(&Winui::menu_notify_handler),
//		MENU_ITEM,
//			XV_KEY_DATA,	key_data,	PARA_DISPLAY,
//			MENU_STRING,	"Display ...",
//			NULL,
//		MENU_ITEM,
//			XV_KEY_DATA,	key_data,	PARA_SEQUENCE,
//			MENU_STRING,	"Sequence ...",
//			NULL,
//		MENU_ITEM,
//			XV_KEY_DATA,	key_data,	PARA_PLOTTING,
//			MENU_STRING,	"Plotting ...",
//			NULL,
//		MENU_ITEM,
//			XV_KEY_DATA,	key_data,	PARA_GLOBAL,
//			MENU_STRING,	"Global ...",
//			NULL,
//		NULL);
//   return(menu);
//}

/************************************************************************
*									*
*  Create view menu.							*
*  Return created menu.							*
*									*/
Menu
Winui::view_create_menu(Frame owner, Xv_font fnt)
{
   Menu menu;

   menu = xv_create(owner,	MENU,
   		XV_FONT,		fnt,
		MENU_NOTIFY_PROC,	(&Winui::menu_notify_handler),
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	VIEW_REDISPLAY,
			MENU_STRING,	"Redisplay",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	VIEW_PROPERTIES,
			MENU_STRING,	"Properties ...",
			NULL,
		NULL);
   return(menu);
}

/************************************************************************
*									*
*  Create processing menu.						*
*  Return created menu.							*
*									*/
Menu
Winui::process_create_menu(Frame owner, Xv_font fnt)
{
   Menu menu;

   menu = xv_create(owner,	MENU,
   		XV_FONT,		fnt,
		MENU_NOTIFY_PROC,	(&Winui::menu_notify_handler),
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	PROCESS_STATISTICS,
			MENU_STRING,	"ROI Statistics",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	PROCESS_LINE,
			MENU_STRING,	"Line Data",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	PROCESS_CURSOR,
			MENU_STRING,	"Cursor Data",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	PROCESS_ROTATION,
			MENU_STRING,	"Image Rotation ...",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	PROCESS_ARITHMETICS,
			MENU_STRING,	"Image Arithmetic ...",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	PROCESS_MATH,
			MENU_STRING,	"Image Math ...",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	PROCESS_HISTOGRAM,
			MENU_STRING,	"Image Histogram Enhancement ...",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	PROCESS_FILTERING,
			MENU_STRING,	"Image Filtering ...",
			NULL,
		NULL);
   return(menu);
}


/************************************************************************
*									*
*  Create tools menu.							*
*  Return created menu.							*
*									*/
Menu
Winui::tools_create_menu(Frame owner, Xv_font fnt)
{
   Menu menu;

   menu = xv_create(owner,	MENU,
   		XV_FONT,		fnt,
		MENU_NOTIFY_PROC,	(&Winui::menu_notify_handler),
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	TOOLS_GRAPHICS,
			MENU_STRING,	"Graphics Tools ...",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	TOOLS_SLICER,
			MENU_STRING,	"Slice Extractor ...",
			NULL,
		NULL);
   return(menu);
}

/************************************************************************
*									*
*  Create miscellaneous menu.						*
*  Return created menu.							*
*									*/
Menu
Winui::misc_create_menu(Frame owner, Xv_font fnt)
{
   Menu menu;
   Menu infomenu, errmenu;

   infomenu = xv_create(owner,	MENU,
   		XV_FONT,		fnt,
		MENU_NOTIFY_PROC,	(&Winui::menu_notify_handler),
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MISC_INFO_MSG_CLEAR,
			MENU_STRING,	"Clear",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MISC_INFO_MSG_WRITE,
			MENU_STRING,	"Save ... ",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MISC_INFO_MSG_SHOW,
			MENU_STRING,	"Show ... ",
			NULL,
	       NULL);
   errmenu = xv_create(owner,	MENU,
   		XV_FONT,		fnt,
		MENU_NOTIFY_PROC,	(&Winui::menu_notify_handler),
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MISC_ERR_MSG_CLEAR,
			MENU_STRING,	"Clear",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MISC_ERR_MSG_WRITE,
			MENU_STRING,	"Save ... ",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MISC_ERR_MSG_SHOW,
			MENU_STRING,	"Show ... ",
			NULL,
	       NULL);

   menu = xv_create(owner,	MENU,
   		XV_FONT,		fnt,
		MENU_NOTIFY_PROC,	(&Winui::menu_notify_handler),
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MISC_ERR_MSG,
			MENU_STRING,	"Error Messages",
			MENU_PULLRIGHT,	errmenu,
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MISC_INFO_MSG,
			MENU_STRING,	"Info Messages",
			MENU_PULLRIGHT,	infomenu,
			NULL,
//		MENU_ITEM,
//			XV_KEY_DATA,	key_data,	MISC_HELP,
//			MENU_STRING,	"Help ...",
//			NULL,
//		MENU_ITEM,
//			XV_KEY_DATA,	key_data,	MISC_TIME,
//			MENU_STRING,	"Time",
//			NULL,
		NULL);
   return(menu);
}

/************************************************************************
*									*
*  Create macro menu.
*  Return created menu.							*
*									*/
Menu
Winui::macro_create_menu(Frame owner, Xv_font fnt)
{
   Menu menu;

   menu = xv_create(owner,	MENU,
   		XV_FONT,		fnt,
		MENU_NOTIFY_PROC,	(&Winui::menu_notify_handler),
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MACRO_EXECUTE,
			MENU_STRING,	"Exec",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MACRO_RECORD,
			MENU_STRING,	"Record",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MACRO_SHOW,
			MENU_STRING,	"Show",
			NULL,
		MENU_ITEM,
			XV_KEY_DATA,	key_data,	MACRO_SAVE,
			MENU_STRING,	"Save",
			NULL,
		NULL);
   return(menu);
}

/************************************************************************
*									*
*  Handler all selected menus.						*
*									*/
void
Winui::menu_notify_handler(Menu, Menu_item i)
{
   int key_value = (int)xv_get(i, XV_KEY_DATA, key_data);

   if (key_value == FILE_EXIT)
      quit_proc();
   else
      // Send message to the connected process (graphics process)
      Message::msg_send((u_long)UI_COMMAND, (u_long)key_value,
	  "%d: %s", key_value, (char *)xv_get(i, MENU_STRING));
}

/************************************************************************
*									*
*  Handler all selected panel commands.					*
*									*/
void
Winui::panel_notify_handler(Panel_item i, Event *)
{
   int key_value = (int)xv_get(i, XV_KEY_DATA, key_data);

   if (key_value == CANCEL)
   {
      if (dest_process_pid)
	 // Send an interrupt signal to graphics process
         kill(dest_process_pid, signal_type);
      else
	 STDERR("Interrupt routine is not registered");
   }   else {
       //     printf("msg_send(%d, %d, %s ...)\n",
		     //	    (u_long) UI_COMMAND,
		     //	    (u_long) xv_get(i, XV_KEY_DATA, key_data),
		     //	    (char *) xv_get(i, PANEL_LABEL_STRING));
     // Send message to the connected process (graphics process)
     /*fprintf(stderr,"XID=%d\n", (int)xv_get(frame, XV_XID));/*CMP*/
     Message::msg_send((u_long)UI_COMMAND, 
		       (u_long)xv_get(i, XV_KEY_DATA, key_data),
		       "%d: %s",
		       /*(int)xv_get(frame, XV_XID),*/
		       (int)xv_get(i, XV_KEY_DATA, key_data),
		       (char *)xv_get(i, PANEL_LABEL_STRING));
   }
}

/************************************************************************
*									*
*  UI process is about to exit.						*
*									*/
void
Winui::quit_proc()
{
   // Tell the graphics process to exit
   Message::msg_send((u_long)UI_COMMAND, (u_long)FILE_EXIT, NULL);

   // Destroy all message object. This routine MUST be called 
   // before exiting.
   Message::destroy(); 

   // delete winobj;

   exit(0);
}

/************************************************************************
*									*
*  Message received from other process.					*
*									*/
void
Winui::msg_handler(Ipgmsg *msg)
{
   if ((msg->major_id == UI_COMMAND) && (msg->minor_id == CANCEL))
   {
      // Get the destination PID and signal type
      (void)sscanf(msg->msgbuf, "%d %d", &dest_process_pid,
					 &signal_type);
      if (signal_type){
	  xv_set(cancel, PANEL_INACTIVE, FALSE, NULL);
      }else{
	  xv_set(cancel, PANEL_INACTIVE, TRUE, NULL);
      }
   }
   else
   {
      cerr << "Unknown Message (ui):\n"
	   << "  Major: " << msg->major_id << "\n"
	   << "  Minor: " << msg->minor_id << "\n"
	   << "  Msgbuf: " << msg->msgbuf << "\n";
  }
}
