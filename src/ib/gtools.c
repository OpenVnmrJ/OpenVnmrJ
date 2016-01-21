/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/


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
*									*
*************************************************************************/
#include <stdio.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/svrimage.h>
#include "stderr.h"
#include "initstart.h"
#include "graphics.h"
#include "macrolist_ib.h"
#include "gtools.h"
#include "inputwin.h"
#include "ibcursors.h"
#include "macroexec.h"

extern void win_print_msg(char *format, ...);
extern void win_print_msg_field(int field_index, char *format, ...);

// Graphics-tools name (should be ended with NULL).  Its order should
// be CONSISTENT with enumerator Gtype (in gtool.h).
static char *gtools_name[] = {
  "selector.bm",
  "frame.bm",
  "zoom.bm",
  "vs.bm",
  "line.bm",
  "point.bm",
  "rect.bm",
//  "oval.bm",
  "pgon.bm",
  "pgon.open.bm",
  "alpha.bm",
  "math.bm",
  NULL };


// Initialize static class members
Flag Roi_routine::active = 0;
Flag Zoom_routine::active = 0;
Flag Vs_routine::active = 0;
Flag Math_routine::active = 0;
Frame Gtools::popup = 0;       // Pop-up window
Menu Gtools::menu = 0;         // Menu for initialization
Panel Gtools::panel = 0;       // Panel control area
Panel_item Gtools::tools = 0;  // Panel choice (of graphics tools)
Panel_item Gtools::props = 0;  // Panel buton menu (shared by all gtools)
Gdev *Gtools::gdev = 0;        // This is a handler for drawing routine
Cms Gtools::control_cms = 0;
  
// Pointer to the function of gtools
Gtools_routine *func[NUM_GTOOL+1];
Gtype gtype;       // Gtools type

/************************************************************************
*									*
*  Create all necessary graphics tools as well as its user control	*
*  panel.								*
*  (Called only once for initialization).				*
*  (STATIC function)							*
*									*/
void
Gtools::create(Frame owner, Gdev *graphics_device)
{
   int x_pos, y_pos;	// position of pop-up window on the screen
   int nrows;		// number of rows for graphics-tools
   char initname[128];  // filename of initialization file
   char workbuf[128];	// temporary working buffer
   int i;		// loop counter
   Server_image image;	// graphics-tool item as an image

   if (owner == NULL)
   {
      STDERR("gtool_win_create:Pop-up frame should be owned by a frame");
      exit(1);
   }

   // Get the initialization file for window position
   (void)init_get_win_filename(initname);

   // Get the position and # rows of the graphics tools 
   if (init_get_val(initname, "GRAPHICS_TOOLS", "ddd",
       &x_pos, &y_pos, &nrows) == NOT_OK)
   {
      // Defaults
      x_pos = 0;
      y_pos = 0;
      nrows = 2;
   }

   // Create pop-up frame
   popup = (Frame)xv_create(owner, FRAME_CMD,
			    FRAME_LABEL, "Tools",
			    XV_X, x_pos,
			    XV_Y, y_pos,
			    NULL);

   // Get a panel from pop-up frame
   panel = xv_get(popup,    FRAME_CMD_PANEL);

   // Create properties Button-menu
   menu = xv_create(NULL, MENU_COMMAND_MENU,
		    MENU_ITEM,
		    MENU_STRING, "No Properties",
		    NULL,
		    NULL);
   props = xv_create(panel, PANEL_BUTTON,
		     XV_X, 6,
		     PANEL_LABEL_STRING, "ROI Properties",
		     PANEL_ITEM_MENU, menu,
		     NULL);


   // Note that we first create panel-choice as non-exclusive panel   
   // so that panel-choice (as a default) will put its choice items  
   // in a nice and neat format.  Then all panel-items (as images) are
   // created one by one.  After that, panel-choice is set to be     
   // exclusive (only one graphics-tool item can be selected at a time).

   tools = xv_create(panel,	PANEL_CHOICE,
	XV_X,		6,
   	XV_Y,		(int)xv_get(props, XV_HEIGHT) + 15,
	PANEL_CHOOSE_ONE,	FALSE,
	PANEL_CHOICE_NROWS,	nrows,
	PANEL_NOTIFY_PROC,	select_tool,
	NULL);

   // Get path- name for location where graphics-tool bitmap
   (void)init_get_env_name(initname);

   // Create graphics-tools image as a panel choice
   for (i=0; gtools_name[i] ; i++)
   {
      (void)sprintf(workbuf, "%s/%s", initname, gtools_name[i]);

      image = (Server_image)xv_create(NULL,	SERVER_IMAGE,
		SERVER_IMAGE_BITMAP_FILE,	workbuf,
		NULL);

      // Check whether image creation is successful or not
      if (image == NULL)
      {
	 // Error
	 xv_destroy_safe(popup);
	 STDERR_1("Gtools: cannot create %s", workbuf);
	 exit(1);
      }

      xv_set(tools,
	PANEL_CHOICE_IMAGE,	i,	image,
	NULL);
   }

   xv_set(tools,
	PANEL_CHOOSE_ONE,	TRUE, 
	PANEL_VALUE,		GTOOL_FRAME,
	NULL);

   window_fit(panel);
   window_fit(popup);

   gdev = graphics_device;

   // Register all classes (Polymorphism) with a base-class
   func[GTOOL_FRAME] = new Frame_routine(gdev);
   func[GTOOL_ZOOM] = new Zoom_routine(gdev);
   func[GTOOL_VS] = new Vs_routine(gdev);
   func[GTOOL_LINE] = new Roi_routine(gdev);	// ROI tool
   func[GTOOL_POINT] = func[GTOOL_LINE];	 	// ROI tool
   func[GTOOL_RECT] = func[GTOOL_LINE];	 	// ROI tool
//   func[GTOOL_OVAL] = func[GTOOL_LINE];	 	// ROI tool
   func[GTOOL_PGON] = func[GTOOL_LINE];	 	// ROI tool
   func[GTOOL_PGON_OPEN] = func[GTOOL_LINE];	 	// ROI tool
   func[GTOOL_TEXT] = func[GTOOL_LINE];
   func[GTOOL_SELECTOR] = func[GTOOL_LINE];
   func[GTOOL_MATH] = new Math_routine(gdev);
   func[NUM_GTOOL] = NULL;

   // Selected gtool_type.  Its initialized value is NUM_GTOOL which
   // indicates no gtool is selected.					   
   gtype = NUM_GTOOL;

   select_tool(NULL, GTOOL_SELECTOR);
}

/************************************************************************
 *									*
 *  Change the label on the Properties button
 *  (STATIC function)							*
 *									*/
void
Gtools::set_props_label(char *name)
{
    xv_set(props, PANEL_LABEL_STRING, name, NULL);
}

/************************************************************************
*									*
*  Change the tool or, with no argument, pop up the gtools panel
*  [MACRO interface]
*  argv[0]: (char *) Which tool type:
*		"roi" | "frame" | "zoom" | "display" | "line" | "point"
*		| "box" | "polygon" | "polyline" | "label"
*  [STATIC Function]							*
*									*/
int
Gtools::Tool(int argc, char **argv, int, char **)
{
    argc--; argv++;

    if (argc == 0){
	show_tool();
    }else if (argc != 1){
	ABORT;
    }else{
	Gtype type;
	char *arg = argv[0];
	if (strcasecmp(arg, "roi") == 0){
	    type = GTOOL_SELECTOR;
	}else if (strcasecmp(arg, "frame") == 0){
	    type = GTOOL_FRAME;
	}else if (strcasecmp(arg, "zoom") == 0){
	    type = GTOOL_ZOOM;
	}else if (strcasecmp(arg, "display") == 0){
	    type = GTOOL_VS;
	}else if (strcasecmp(arg, "line") == 0){
	    type = GTOOL_LINE;
	}else if (strcasecmp(arg, "point") == 0){
	    type = GTOOL_POINT;
	}else if (strcasecmp(arg, "box") == 0){
	    type = GTOOL_RECT;
	}else if (strcasecmp(arg, "polygon") == 0){
	    type = GTOOL_PGON;
	}else if (strcasecmp(arg, "polyline") == 0){
	    type = GTOOL_PGON_OPEN;
	}else if (strcasecmp(arg, "label") == 0){
	    type = GTOOL_TEXT;
	}else if (strcasecmp(arg, "math") == 0){
	    type = GTOOL_MATH;
	}else{
	    ABORT;
	}
	select_tool((Panel_item)0, type);
    }
    return PROC_COMPLETE;
}

/************************************************************************
 *									*
 *  Change the tool according to the type passed by argument.		*
 *  (STATIC function)							*
 *									*/
void
Gtools::select_tool(Panel_item, int tool_type)
{
  extern Roitool *active_tool;
  extern Roitool *roitool[ROI_NUM];
  
  // If Selecting the same tool, do nothing (otherwise selector box breaks)
  if (gtype == (Gtype)tool_type)
    return;
  
  // Ending previous gtool
  if (func[gtype])    {
    func[gtype]->end();
    
    // Hide the input window if there is
    inputwin_hide();
  }
  
  gtype = (Gtype)tool_type;
  
  xv_set(tools,
	 PANEL_CHOOSE_ONE,	TRUE, 
	 PANEL_VALUE,		tool_type,
	 NULL);
  
  // Reset menu
  xv_set(props, PANEL_ITEM_MENU, menu, NULL);
  
  // Initializing current gtool
  if (func[gtype])
    func[gtype]->start(props, gtype);
  
  win_print_msg_field(1, "");
  
  switch (gtype)    {
    
  case GTOOL_SELECTOR:
    active_tool = roitool[ROI_SELECTOR];
    win_print_msg("Awaiting Selection");
    set_cursor_shape(IBCURS_SELECT_POINT);
    func[GTOOL_LINE]->show_props_menu();
    macroexec->record("tool('roi')\n");
    break;
    
  case GTOOL_FRAME:
    win_print_msg
    ("Frame: Drag left mouse to define image frame; release to finish.");
    set_cursor_shape(IBCURS_FRAME);
    func[GTOOL_FRAME]->show_props_menu();
    macroexec->record("tool('frame')\n");
    break;

  case GTOOL_ZOOM:
    win_print_msg("Zoom: Left mouse adjusts zoom window");
    set_cursor_shape(IBCURS_ZOOM);
    func[GTOOL_ZOOM]->show_props_menu();
    macroexec->record("tool('zoom')\n");
    break;
    
  case GTOOL_VS:
    win_print_msg
    ("Vertical Scale: Click left mouse at desired maximum intensity point.");
    set_cursor_shape(IBCURS_VSCALE);
    func[GTOOL_VS]->show_props_menu();
    macroexec->record("tool('display')\n");
    break;

  case GTOOL_LINE:
    active_tool = roitool[ROI_LINE];
    win_print_msg
    ("Line: Drag left mouse from start location; release to finish");
    set_cursor_shape(IBCURS_DRAW);
    func[GTOOL_LINE]->show_props_menu();
    macroexec->record("tool('line')\n");
    break;
    
  case GTOOL_POINT:
    active_tool = roitool[ROI_POINT];
    win_print_msg("Point: Click left mouse to choose point");
    set_cursor_shape(IBCURS_DRAW);
    func[GTOOL_LINE]->show_props_menu();
    macroexec->record("tool('point')\n");
    break;
    
  case GTOOL_RECT:
    active_tool = roitool[ROI_BOX];
    win_print_msg
    ("Rectangle: Drag left mouse at desired location; release to finish");
    set_cursor_shape(IBCURS_DRAW);
    func[GTOOL_LINE]->show_props_menu();
    macroexec->record("tool('box')\n");
    break;
    
//  case GTOOL_OVAL:
//    active_tool = roitool[ROI_OVAL];
//    win_print_msg("Oval: Drag left mouse from desired location.");
//    set_cursor_shape(IBCURS_DRAW);
//    break;
    
  case GTOOL_PGON:
    active_tool = roitool[ROI_POLYGON];
    win_print_msg
    ("Closed Polygon: Drag left mouse; click middle mouse to end.");
    set_cursor_shape(IBCURS_DRAW);
    func[GTOOL_LINE]->show_props_menu();
    macroexec->record("tool('polygon')\n");
    break;
    
  case GTOOL_PGON_OPEN:
    active_tool = roitool[ROI_POLYGON_OPEN];
    win_print_msg("Open Polygon: Drag left mouse; click middle mouse to end.");
    set_cursor_shape(IBCURS_DRAW);
    func[GTOOL_LINE]->show_props_menu();
    macroexec->record("tool('polyline')\n");
    break;
    
  case GTOOL_TEXT:
    active_tool = roitool[ROI_TEXT];
    win_print_msg("Alpha: Click left mouse at desired text location.");
    set_cursor_shape(IBCURS_TEXT);
    func[GTOOL_LINE]->show_props_menu();
    macroexec->record("tool('label')\n");
    break;
    
  case GTOOL_MATH:
    active_tool = roitool[ROI_MATH];
    win_print_msg("Math: Click left mouse in desired frame.");
    set_cursor_shape(IBCURS_MATH);
    func[GTOOL_MATH]->show_props_menu();
    macroexec->record("tool('math')\n");
    break;
    
  case NUM_GTOOL:
    win_print_msg("Graphics-tool: (bug) NEVER reach here");
    break;
  }
}

/************************************************************************
 *									*
 *  Bring the pinned up properties menu for this tool to the foreground
 *  (if it exists).
 *									*/
void
Gtools_routine::show_props_menu()
{
    Frame pframe = (Frame)xv_get(props_menu, MENU_PIN_WINDOW);
    if (pframe){
	xv_set(pframe, XV_SHOW, TRUE, NULL);
    }
}
  
// Show graphics tools panel
void
Gtools::show_tool()
{
    xv_set(popup,
	   FRAME_CMD_PUSHPIN_IN, TRUE,
	   XV_SHOW, TRUE,
	   NULL);
    macroexec->record("tool\n");
}
