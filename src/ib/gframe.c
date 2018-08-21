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
*  Programming Notes:							*
*  -----------------							*
*  									*
*  This file contain two main class routines: class Gframe and 		*
*  Frame_select.  Gframe is used to hold the information of all frames,	*
*  and Frame_select will hold information of all selected frames. In	*
*  other words, Frame_select is to manipulate selected frames only.	*
*									*
*  Description								*
*  -----------								*
*									*
*  A frame is the place where an image or spectrum can be drawn.  This	*
*  file contains routines to create/resize/move/copy/delete a frame.  	*
*									*
*  To create a frame:							*
*	Hold the LEFT mouse button down, drag, and relase the button.	*
*  To move a frame:							*
* 	Click the LEFT mouse button inside the frame, hold the LEFT	*
*  	mouse button down, and drag.					*
*  To resize a frame:							*
*	Click the LEFT mouse button inside the frame, position the mouse*
*	cursor close the either four corners of the frame, hold the LEFT*
*	mouse button down, and drag.					*
*  To copy a frame:							*
*	Click the LEFT mouse button inside the frame, hold down 'ctrl'	*
*   	key, hold the LEFT mouse button down, and drag.			*
*  To delete a frame:							*
*	User the properties menu.					*
*  To select a frame:							*
*	User the properties menu.					*
*									*
*  LEFT mouse button is used to select a specific frame, and unselect 	*
*       all other  frames.						*
*  MIDDLE mouse button is used to toggle between 'select' and 'unselect'*
*       of a frame.  It doesn't affect all other frames.		*
*  RIGHT mouse button is NOT functional.				*
*  									*
*  NOTE 								*
*  ----									*
*  - a frame should be large enough in order to exist.			*
*  - a frame must not overlap with other frames.			*
*									*
*************************************************************************/

//#define DEBUG

// This debug should be taken out at FINAL release
#define	DEBUG_BETA	1


#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
// #include <stream.h>
#ifdef LINUX
// #include <strstream>
#else
// #include <strstream.h>
#endif
#include <math.h>

#include "ddllib.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "stderr.h"
#include "common.h"
#include "msgprt.h"
#include "interrupt.h"
#include "filelist_id.h"
#include "initstart.h"
#include "zoom.h"
#include "inputwin.h"
#include "confirmwin.h"
#include "ddlfile.h"
#include "statmacs.h"
#include "debug.h"
#include "file_format.h"
#include "macroexec.h"
#include "voldata.h"


#define	Corner_Check(a,b,p,q,v)  com_point_in_rect(a, b, p-v, q-v, p+v, q+v)
#define RMAX 5
#define CMAX 5

// The following two definitions should be defined but are not in the
// current release of SABER-C++


extern "C"
{
#ifndef LINUX
   int gethostname(char *, int);
#endif
   char *getlogin(void);
};

extern void win_print_msg(char *, ...);
extern Gtools_routine *func[NUM_GTOOL+1];
extern int canvas_width ;
extern int canvas_height ;
extern int canvas_stx ;
extern int canvas_sty ;
extern void canvas_repaint_proc(void);

#ifdef DEBUG
#define DEBUG_FRAME debug_frame();
#else
#define DEBUG_FRAME
#endif DEBUG

// For the sake of clearity between erase and draw, these macros are defined.
// They are actually the same.
#define	Erase_1(e)		draw(e)
#define	Erase_2(p,e)		(p)->draw(e)

// NB: If these are different (even 0 and 1) junk can get left on screen
//     Both 0 also seems to leave junk!
//     Anything >1 fails miserably.
static const int line_thin=1;	// 1 pixel line width--fast draw
static const int line_fat=1;	// 1 pixel line width

static const int m_size = 8;	// Length of corner markers
static const int m_curve = 2;	// Width of corner markers

//Initialize static class members
int Gframe::color = 0;
Gdev *Gframe::gdev = NULL;
int Gframe::num_frame = 0;
Flag Gframe::title = FALSE;
Faction_type Gframe::acttype = FRAME_NO_ACTION;
short Gframe::basex = 0;
short Gframe::basey = 0;
Flag Frame_routine::active = FALSE;


// This variable consists a list of all frames created on the screen,
// except the first item. The first item of the list is a TEMPORARY item
// serves as a 'working' buffer (item).  DON'T count the first item as
// a frame.
Gframe *framehead=NULL;

// targetframe points the the next frame that data will be displayed in.

Gframe *targetframe=NULL;

int auto_skip_frame = TRUE;
int load_silently = TRUE;

// This variable consists a list of all selected frames, which its
// pointers (item) 'frameptr' point to the same address as in Gframe.
// Also note that the first item of the list is a TEMPORARY item which
// serves as a 'working' buffer.  DON'T count the first item as a 
// selected frame.
Frame_select *selecthead=NULL;

char *magic_string_list[] = {"sisdata", "fdf/startup", NULL};


/************************************************************************
*									*
*  Creator.								*
*  Create the first item of Gframe and Frame_select which will serve	*
*  as a 'working' buffer.						*
*  Note that this routine is guaranteed to (MUST) be called first before*
*  calling other Frame_routines.					*
*  (This function can only be called once.)				*
*									*/
Frame_routine::Frame_routine(Gdev *gd)
{
  active = TRUE;	/* Default is gtool Frame to be active */
  
  if (framehead == NULL)  {
    framehead = new Gframe;
    Gframe::gdev = gd;
    Gframe::basex = Gframe::basey = G_INIT_POS;
    Gframe::acttype = FRAME_NO_ACTION;
    Gframe::color = G_Get_Stcms1(gd) + 5;	/* Set the frame color */
    
    /* select_flag = 1; */
    /* Register the notify functions for loading/saving file */
//    filelist_notify_func(FILELIST_WIN_ID, FILELIST_NEW,
//			 (long)&Frame_routine::load_data,
//			 (long)&Gframe::menu_save_image,
//			 (long)&Frame_routine::load_data_all);
  } else {
    STDERR("Warning: Frame_routine: has already been called");
  }
  
  if (selecthead == NULL) {
    Menu menu_split;
    Menu menu_delete;
    Menu menu_clear;
    Menu menu_color;
    int i;
    int j;
    int k;
    Menu_item mi;
    static char label[CMAX*RMAX][6];
    
    /* Create select frame item */
    selecthead = new Frame_select((Gframe *) NULL, 0);
    
    menu_split =
    xv_create(NULL, MENU_COMMAND_MENU,
	      MENU_NOTIFY_PROC, &Gframe::menu_handler,
	      NULL);
    for (i=1, k=0; i<=CMAX; i++){
	for (j=1; j<=RMAX; j++, k++){
	    if (i==1 && j==1){
		strcpy(label[k], "other");
	    }else{
		sprintf(label[k],"%2dx%d", j, i);
	    }
	    mi = (Menu_item)xv_create(NULL, MENUITEM,
				      MENU_STRING, label[k],
				      MENU_CLIENT_DATA,	F_MENU_SPLIT,
				      MENU_RELEASE,
				      NULL);
	    xv_set(menu_split, MENU_APPEND_ITEM, mi, NULL);
	}
    }
    xv_set(menu_split, MENU_NROWS, RMAX, MENU_NCOLS, CMAX, NULL);
    
    menu_delete =
      xv_create(NULL,	MENU_COMMAND_MENU,
		MENU_NOTIFY_PROC,		&Gframe::menu_handler,
		MENU_ITEM, MENU_STRING,		"Selected Frame(s)",
		MENU_CLIENT_DATA,	F_MENU_DELETE_SELECT, NULL,
		MENU_ITEM, MENU_STRING,		"Unselected Frame(s)",
		MENU_CLIENT_DATA,	F_MENU_DELETE_UNSELECT, NULL,
		MENU_ITEM, MENU_STRING,		"Empty Frame(s)",
		MENU_CLIENT_DATA,	F_MENU_DELETE_EMPTY, NULL,
		MENU_ITEM, MENU_STRING,		"All Frames",
		MENU_CLIENT_DATA,	F_MENU_DELETE_ALL, NULL,
		NULL);

    menu_clear =
      xv_create(NULL,	MENU_COMMAND_MENU,
		MENU_NOTIFY_PROC,		&Gframe::menu_handler,
		MENU_ITEM, MENU_STRING,		"Selected Frame(s)",
		MENU_CLIENT_DATA,	F_MENU_CLEAR_SELECT, NULL,
		MENU_ITEM, MENU_STRING,		"Unselected Frame(s)",
		MENU_CLIENT_DATA,	F_MENU_CLEAR_UNSELECT, NULL,
		MENU_ITEM, MENU_STRING,		"All Frames",
		MENU_CLIENT_DATA,	F_MENU_CLEAR_ALL, NULL,
		NULL);

    int ncolors = G_Get_Sizecms1(gd);
    int ncols = 2;
    int nrows = 1 + (ncolors - 1) / ncols;

    menu_color = xv_create(NULL, MENU_COMMAND_MENU,
			   MENU_NOTIFY_PROC, &Gframe::frame_set_color,
			   MENU_NCOLS, ncols,
			   MENU_NROWS, nrows,
			   NULL);

    for (i=0; i<ncolors; i++){
	mi = (Menu_item)xv_create(NULL, MENUITEM,
				  MENU_IMAGE, get_color_chip(i),
				  MENU_VALUE, (Xv_opaque)i,
				  NULL);
	xv_set(menu_color, MENU_APPEND_ITEM, mi,
	       MENU_NCOLS, ncols,
	       MENU_NROWS, nrows,
	       NULL);
    }
    
    /* Create properties menu for frame */
    props_menu =
      xv_create(NULL,          MENU_COMMAND_MENU,
		MENU_GEN_PIN_WINDOW, Gtools::get_gtools_frame(), "Frame Props",
		MENU_ITEM,
		MENU_STRING,            "Split Selected Frames",
		MENU_NOTIFY_PROC,		&Gframe::menu_handler,
		MENU_CLIENT_DATA,	F_MENU_SPLIT,
		MENU_PULLRIGHT,		menu_split,
		NULL,

		MENU_ITEM,
		MENU_STRING,            "Delete",
		MENU_NOTIFY_PROC,		&Gframe::menu_handler,
		MENU_CLIENT_DATA,	F_MENU_DELETE_SELECT,
		MENU_PULLRIGHT,		menu_delete,
		NULL,

		MENU_ITEM,
		MENU_STRING,            "Clear",
		MENU_NOTIFY_PROC,		&Gframe::menu_handler,
		MENU_CLIENT_DATA,	F_MENU_CLEAR_SELECT,
		MENU_PULLRIGHT,		menu_clear,
		NULL,

		MENU_ITEM,
		MENU_STRING,            "Select All Frames",
		MENU_NOTIFY_PROC,		&Gframe::menu_handler,
		MENU_CLIENT_DATA,	F_MENU_SELECT_ALL,
		NULL,

//		MENU_ITEM,
//		MENU_STRING,            "Title",
//		MENU_CLIENT_DATA,	F_MENU_TITLE,
//		NULL,

		MENU_ITEM,
		MENU_STRING,            "Load Frame Layout",
		MENU_NOTIFY_PROC,		&Gframe::menu_handler,
		MENU_CLIENT_DATA,	F_MENU_LOAD,
		MENU_GEN_PULLRIGHT,	&Gframe::menu_load_pullright,
		NULL,

		MENU_ITEM,
		MENU_STRING,            "Save Frame Layout ...",
		MENU_NOTIFY_PROC,		&Gframe::menu_handler,
		MENU_CLIENT_DATA,	F_MENU_SAVE,
		NULL,

		MENU_ITEM,
		MENU_STRING,            "Frame Color",
		MENU_NOTIFY_PROC,		&Gframe::menu_handler,
		MENU_PULLRIGHT,		menu_color,
		NULL,

//		MENU_ITEM,
//		MENU_STRING,            "Un-Fullscreen",
//		MENU_CLIENT_DATA,	F_MENU_UNFULL,
//		NULL,
//
//		MENU_ITEM,
//		MENU_STRING,            "Fullscreen",
//		MENU_CLIENT_DATA,	F_MENU_FULL,
//		NULL,

		NULL);
    
    filelist_notify_func(FILELIST_GFRAME_ID, FILELIST_NEW, NULL,
			 (long)&Gframe::menu_save);
    
    /* Set the proper directory path */
    char dirname[1024];
    (void)init_get_env_name(dirname);
    (void)strcat(dirname, "/gframe");
    filelist_set_directory(FILELIST_GFRAME_ID, dirname);
  }

  // Create popup for custom frame splitting
  split_frame = NULL;
  split_popup = NULL;

  // Get the initialized file of window position
    char initname[1024];	// init file
    (void)init_get_win_filename(initname);

  // Get the position of the control panel
  int xpos;
  int ypos;
  Panel split_panel;
  if (init_get_val(initname, "FRAME_SPLITTER", "dd", &xpos, &ypos) == NOT_OK){
      xpos = 100;
      ypos = 100;
  }

  split_frame = xv_create(NULL, FRAME, 
			  WIN_DYNAMIC_VISUAL, TRUE,
			  NULL);

  split_popup = xv_create(split_frame, FRAME_CMD,
			  XV_X, xpos,
			  XV_Y, ypos,
			  FRAME_LABEL, "Frame Splitter",
			  FRAME_CMD_PUSHPIN_IN, TRUE,
			  FRAME_SHOW_RESIZE_CORNER, TRUE,
			  NULL);

  split_panel = (Panel)xv_get(split_popup, FRAME_CMD_PANEL);

  split_rows_widget =
  xv_create(split_panel, PANEL_NUMERIC_TEXT,
	    PANEL_VALUE_DISPLAY_LENGTH, 4,
	    PANEL_LABEL_STRING, "Rows",
	    PANEL_VALUE, 1,
	    PANEL_MIN_VALUE, 1,
	    PANEL_MAX_VALUE, 100,
	    NULL);

  split_cols_widget =
  xv_create(split_panel, PANEL_NUMERIC_TEXT,
	    PANEL_VALUE_DISPLAY_LENGTH, 4,
	    PANEL_LABEL_STRING, "Cols",
	    PANEL_VALUE, 1,
	    PANEL_MIN_VALUE, 1,
	    PANEL_MAX_VALUE, 100,
	    NULL);

  xv_create(split_panel, PANEL_BUTTON,
	    PANEL_LABEL_STRING, "Split",
	    PANEL_NOTIFY_PROC, Frame_routine::split_callback,
	    NULL);

  window_fit(split_panel);
  window_fit(split_popup);
  window_fit(split_frame);
}

void
Frame_routine::split_callback(Panel_item, int, Event *)
{
    Frame_routine *fr = (Frame_routine *)func[GTOOL_FRAME];
    int rows = (int)xv_get(fr->split_rows_widget, PANEL_VALUE);
    int cols = (int)xv_get(fr->split_cols_widget, PANEL_VALUE);
    Frame_select::split(rows, cols);
}

void
Frame_routine::show_splitter_popup()
{
    xv_set(split_popup, XV_SHOW, TRUE, NULL);
}

/************************************************************************
*									*
*  Initializing anything related to Frame.  It is called when the user	*
*  just selects the gtool Frame.					*
*									*/
void
Frame_routine::start(Panel props, Gtype)
{
  active = TRUE;
  xv_set(props, PANEL_ITEM_MENU, props_menu, NULL);
  Gtools::set_props_label("Frame Properties");
}

/************************************************************************
*									*
*  Clean-up routine (about to leave gtool Frame).  It is called when	*
*  the user has selected another tool.					*
*									*/
void
Frame_routine::end(void)
{
  // Set the line width back to the default
  G_Set_LineWidth(Gframe::gdev, 1);
  active = FALSE;
}

/************************************************************************
*									*
*  This function will be called if no other gtools events are executed. *
*  For example, while gtools ROI box is active, and no ROI box event	*
*  is detected, then the program will pass the event to this function.  *
*									*
*  This function will execute an event related to select and deselect a	*
*  frame only.								*
*									*/
void
Frame_routine::frame_task(Event *e, Routine_type type)
{
  short x = event_x(e);
  short y = event_y(e);

  if (type == ROUTINE_FRAME_SELECT) {
    if (framehead->is_selected(ACTION_SELECT, x, y)) {
      Gframe::acttype = FRAME_SELECT;
      framehead->action(x, y);
      Gframe::acttype = FRAME_SELECT_DONE; 
      framehead->action(x, y);
    }
  }

  if (type == ROUTINE_FRAME_TOGGLE) {
    if (framehead->is_selected(ACTION_SELECT, x, y)) {
      Gframe::acttype = FRAME_TOGGLE;
      framehead->action(x, y);
      Gframe::acttype = FRAME_TOGGLE_DONE; 
      framehead->action(x, y);
    }
    return;
  }
  
  switch (event_action(e))  {
    
  case ACTION_SELECT: // == LEFT_BUTTON
    
    if (event_is_down(e)) {
      if (framehead->is_selected(ACTION_SELECT, x, y) &&
	  (Gframe::acttype == FRAME_SELECT))
	framehead->action(x, y);
      else
	Gframe::acttype = FRAME_NO_ACTION;
    } else {
      if (Gframe::acttype == FRAME_SELECT) {
	Gframe::acttype = FRAME_SELECT_DONE; 
	framehead->action(x, y);
      }
      Gframe::acttype = FRAME_NO_ACTION;
    }
    break;
    
  case ACTION_ADJUST:  // == MIDDLE_BUTTON
    
    // Middle button will handle toggle only
    process(e);
    break;

  case ACTION_CUT:
    printf("ACTION_CUT event received\n");
    framehead->remove(F_MENU_DELETE_SELECT);
    break;

  case WIN_RESIZE:
    
    printf("WIN_RESIZE event received\n");
    break;
    
  case WIN_REPAINT:
    
    printf("WIN_REPAINT event received\n");
    break;
  }
}

/************************************************************************
*									*
*  Mouse event Graphics-tool: Frame.					*
*  This function will be called if there is an event related to a frame.*
*  Always return ROUTINE_DONE.						*
*									*/
Routine_type
Frame_routine::process(Event *e)
{
  short x = event_x(e);
  short y = event_y(e);
  
  switch (event_action(e))    {
    
  case LOC_DRAG:
    framehead->action(x, y);
    break;
      
  case ACTION_SELECT: // == LEFT_BUTTON
    if (event_is_down(e))	{
      if (framehead->is_selected(ACTION_SELECT, x, y))  {
	if (event_ctrl_is_down(e))  {
	  Gframe::acttype = FRAME_COPY;
	  framehead->action(x, y);	// Copy
	} else {	
	  framehead->action(x, y);	// Resize, Move, or Select
	}
      } else {
	Gframe::acttype = FRAME_CREATE;
	framehead->action(x, y);		// Create
      }
    } else {
      switch (Gframe::acttype)	    {
	
      case FRAME_CREATE:
	Gframe::acttype = FRAME_CREATE_DONE; break;
	
      case FRAME_SELECT:
	Gframe::acttype = FRAME_SELECT_DONE; break;
	
      case FRAME_RESIZE:
	Gframe::acttype = FRAME_RESIZE_DONE; break;
	
      case FRAME_MOVE:
	Gframe::acttype = FRAME_MOVE_DONE; break;
	
      case FRAME_COPY:
	Gframe::acttype = FRAME_COPY_DONE; break;
	
      default: 
	Gframe::acttype = FRAME_NO_ACTION;
	return(ROUTINE_DONE);
      }
      
      framehead->action(x, y);
      Gframe::acttype = FRAME_NO_ACTION;
    }
    break;
      
  case ACTION_ADJUST: // == MIDDLE_BUTTON
    if (event_is_down(e)) {
      if (framehead->is_selected(ACTION_ADJUST, x, y))
	framehead->action(x, y);		// Toggle
    } else {
      if (Gframe::acttype == FRAME_TOGGLE)	{
	Gframe::acttype = FRAME_TOGGLE_DONE;
	framehead->action(x, y);
      }
	  
      Gframe::acttype = FRAME_NO_ACTION;
    }
    break;
      
  case ACTION_MENU:  // == RIGHT_BUTTON
    break;
  }
  return(ROUTINE_DONE);
}

/*************************************************************************
*									 *
* After the user loads an image into a predesignated frame, this routine *
* is called to find the next free frame suitable for loading the next    *
* image.								 *
* [STATIC]								 */

void
Frame_routine::FindNextFreeFrame() {

  // "bestframebefore" will contain the best suitable frame in the list
  // of available frames up to the current "targetframe" (default frame).
  // "bestframeafter" will contain the best suitable frame in the list
  // of frames after the current targetframe.
  
  Gframe *bestframebefore, *bestframeafter;

  if (!framehead) return; // What's going on here?  No frames!
  
  if (!targetframe) {
    // No default frame, start from the head of the list
    targetframe = framehead->next;
    return;
  }
  
  // First look for a suitable frame starting with the frame immediately
  // after the target frame.

  bestframeafter = targetframe;
  while (bestframeafter = bestframeafter->next) {
    if (bestframeafter->imginfo == 0) {
      // This looks like a good frame that is free
      targetframe = bestframeafter;
      return;
    }
  }

  // At this point, we have come to the end of the frame list.  Repeat
  // the search again but this time, start with the head of the list.

  bestframebefore = framehead->next; // this is the first frame in the list

  while (bestframebefore && (bestframebefore != targetframe)) {
    if (bestframebefore->imginfo == 0) {
      // This looks like a good free frame
      targetframe = bestframebefore;
      return;
    }
    bestframebefore = bestframebefore->next;
  }

  // At this point, we have come around full circle in our search for the
  // next free frame.  In this case, just select the frame following the
  // current target frame.  If none exist, just select the head of the list

  targetframe = targetframe->next;
  if (!targetframe) {
    targetframe = framehead->next;
  }
}



/************************************************************************
*									*
*  Creator for Gframe.							*
*									*/
Gframe::Gframe(void)
{
  next = NULL;
  x1 = y1 = x2 = y2 = G_INIT_POS;
  select = FALSE;
  clean = TRUE;
  title = FALSE;
  xoff = yoff = 0.0;
  imginfo = NULL;
  params = NULL;
  overlay_list = new ImginfoList();
}


/************************************************************************
*									*
*  Destructor for bigframe.						*
*									*/
void
Gframe::bye_big_gframe(Gframe *g)
{
  g->draw(line_fat) ;
}

/************************************************************************
*									*
*  Return number of Gframes, not including the "working-buffer" frame.
*  STATIC
*									*/
int
Gframe::numFrames(void)
{
    return num_frame;
}

/************************************************************************
*									*
*  Creator for bigframe.						*
*  STATIC
*									*/
Gframe*
Gframe::big_gframe(void)
{
    Gframe *g ;

    g = new Gframe ;
    g->next = NULL;
    g->x1 = canvas_stx ;
    g->y1 = canvas_sty ;
    g->x2 = canvas_width - 1;
    g->y2 = canvas_height - 1;
    g->select = FALSE;
    g->clean = TRUE;
    g->title = FALSE;
    g->xoff = g->yoff = 0.0;
    g->imginfo = NULL;
    g->params = NULL;
    g->draw(line_fat); 
    g->append(); 
    g->set_select(TRUE); 
    return(g);
}

/************************************************************************
*									*
*  Destructor of Gframe.						*
*  Before a frame is destroyed, do necessary clean-up.			*
*									*/
Gframe::~Gframe()
{
  // Unmark if it is marked
  if (select)
    {
      select = FALSE;
      mark();
    }
  
  /* Delete image data information moved to remove_image routine - mrh	*/
  
  // Erase the frame from the screen
  Erase_1(line_fat);
  
  num_frame -= 1;
}

/************************************************************************
*									*
*  Action routines to create/select/copy a frame.			*
*  This function will serve as an interface to Frame_routine.  		*
*									*/
void
Gframe::action(short x, short y)
{
  switch (acttype)
    {
    case FRAME_CREATE:
      if (basex == G_INIT_POS)
	{
	  selecthead->deselect();
	  basex = x; basey = y;
	  x1 = G_INIT_POS;
	}
      else
	resize(x, y);
      break;
      
    case FRAME_CREATE_DONE:
      if (x1 != G_INIT_POS)
	{
	  Erase_1(line_thin);
	  
	  // A frame exists if it is larger enough
	  if (((x2 - x1) >= min_width()) &&
	      ((y2 - y1) >= min_height()))
	    {
	      draw(line_fat);
	      mark();
	      selecthead->insert(framehead);
	      append();
	    }
	}
      basex = G_INIT_POS;	/* Reinitialize */
      break;
      
    case FRAME_SELECT:
      selecthead->deselect();
      selecthead->frameptr->mark();
      selecthead->insert();
      acttype = FRAME_NO_ACTION;
      targetframe = selecthead->next->frameptr;

      // Check for 3D data
      /*{
	  char *spatial_rank;
	  Imginfo *img = targetframe->imginfo;
	  if (img){
	      img->st->GetValue("spatial_rank", spatial_rank);
	      if (strcmp(spatial_rank, "3dfov") == 0){
		  VolData::extract_slices(targetframe);
	      }
	  }
	  }/* 3D data no longer attached to gframes */

      break;
      
    case FRAME_TOGGLE:
      selecthead->frameptr->mark();
      if (selecthead->frameptr->select){
	  selecthead->frameptr->set_select(FALSE);
	  selecthead->remove(REMOVE_SELECT_ONE_ITEM);
      }else{
	  targetframe = selecthead->frameptr;
	  selecthead->insert();
      }
      acttype = FRAME_NO_ACTION;
      break;
      
    case FRAME_RESIZE:
      // Note that 'basex' and 'basey' has been set prior to calling
      // this routine. (look at routine "is_selected" and
      // "corner_selected")
      resize(x, y);
      break;
      
    case FRAME_MOVE:
      if (basex == G_INIT_POS)
	{
	  targetframe = selecthead->frameptr;
	  basex = x; basey = y;
	  x1 = G_INIT_POS;
	}
      else
	move(x, y, TRUE);
      break;
      
      
    case FRAME_RESIZE_DONE:
    case FRAME_MOVE_DONE:
      if (x1 != G_INIT_POS)
	selecthead->frameptr->update_position();
      else
	{
	  // This is necessary in case the user just wants to 
	  // select this  specific frame.  Hence we deselect all
	  // other frames and select only this frame.  
	  selecthead->deselect();
	  selecthead->frameptr->mark();
	  selecthead->insert();
	}
      basex = G_INIT_POS;	/* Reinitialize */
      break;
      
    case FRAME_COPY:
      if (basex == G_INIT_POS)
	{
	  basex = x; basey = y;
	  x1 = G_INIT_POS;
	}
      else
	move(x, y, FALSE);
      break;
      
    case FRAME_COPY_DONE:
      if (x1 != G_INIT_POS)
	{
	  Erase_1(line_thin);
	  
	  /* Make sure the copied frame is not overlapped with other */
	  /* frame.						       */
	  if (!overlap_frame(TRUE))
	    {
	      draw(line_fat);
	      if (selecthead->frameptr->select)
		{
	          mark();
	          selecthead->insert(framehead);
		}
	      append();
	    }
	}
      basex = G_INIT_POS;	/* Reinitialize */
      break;
      
    case FRAME_NO_ACTION:
    case FRAME_SELECT_DONE:
      case FRAME_TOGGLE_DONE:
	 break;
   }
}

/************************************************************************
*									*
*  Update the frame size.						*
*  Note that the frame cannot be overlapped with any other frame.	*
*									*/
void
Gframe::resize(short x, short y)
{
    short nx1, ny1, nx2, ny2;
    static int prevx;
    static int prevy;
    
    if (x1 == G_INIT_POS){
	prevx = x2;
	prevy = y2;
    }
#ifdef LINUX
    constrain(&Gframe::overlap_resized_frame, prevx, prevy, &x, &y);
#else
    constrain(Gframe::overlap_resized_frame, prevx, prevy, &x, &y);
#endif 

    // Get the constrained frame coordinates
    nx1 = basex;
    ny1 = basey;
    nx2 = prevx = x;
    ny2 = prevy = y;
    com_minmax_swap(nx1, nx2);
    com_minmax_swap(ny1, ny2);
    
    // Erase previous frame (if necessary)
    if (x1 != G_INIT_POS){
	Erase_1(line_thin);
    }
    
    /* Update the current size */
    x1 = nx1;
    x2 = nx2;
    y1 = ny1;
    y2 = ny2;
    
    // Draw a frame
    draw(line_thin);
}

/************************************************************************
*									*
*  Update (the current selected) frame position.  This happens if the 	*
*  frame is resized or moved.						*
*  The current position is in the first item list.  Hence copy position	*
*  info	from the first item to the selected frame.			*
*									*/
void
Gframe::update_position(void)
{
  // Erase current thin frame line
  Erase_2(framehead,line_thin);
  
  // Erase previous frame area
  clear();
  
  // Erase previous frame
  Erase_1(line_fat);
  
  if (select)	// Unmark
    mark();
  
  // Update current frame size
  x1 = framehead->x1;
  y1 = framehead->y1;
  x2 = framehead->x2;
  y2 = framehead->y2;
  
  draw(line_fat);
  
  if (select)	// Mark
    mark();
  
  // Display the image if it exists
  if (imginfo){
      update_image_position();
      imginfo->update_screen_coordinates();	// Update ROI positions
      display_data();
  }
}

/************************************************************************
*									*
*  Note that the frame can not be moved into out of bound of graphics	*
*  region (canvas).  In addition, it cannot overlap any other frame	*
*									*/
void
Gframe::constrain(Flag(Gframe::*func)(int, int),
		  int xok, int yok, short *px, short *py)
{
    int x = *px;
    int y = *py;
    
    if ((this->*func)(x, y)){
	// Doesnt fit in requested location
	// Move as far as possible along line from previous location
	float dx = x - xok;
	float dy = y - yok;
	int i;
	int n;
	float stepx;
	float stepy;
	if (fabs(dx) > fabs(dy)){
	    n = (int)fabs(dx);
	    stepx = copysign(1.0, dx);
	    stepy = copysign(dy/dx, dy); // (dx guaranteed non-zero)
	}else{
	    n = (int)fabs(dy);
	    stepx = copysign(dx/dy, dx);
	    stepy = copysign(1.0, dy);
	}
	float fx = xok + stepx;
	float fy = yok + stepy;
	float tx = xok;
	float ty = yok;
	// Scan along the line
	for (i=0; i<n; fx+=stepx, fy+=stepy, i++){
	    if ((this->*func)((int)fx, (int)fy)){
		break;
	    }
	    tx = fx;
	    ty = fy;
	}
	// Go back to last good place
	fx = tx;
	fy = ty;

	// Can we move in x or y alone?
	int istepx = (int)copysign(1.0, dx);
	int istepy = (int)copysign(1.0, dy);
	n = abs((x - (int)fx));
	for (i=0; i<n; fx += istepx, i++){
	    if ((this->*func)((int)fx, (int)fy)){
		break;
	    }
	    tx = fx;
	}
	fx = tx;
	n = abs((y - (int)fy));
	for (i=0; i<n; fy += istepy, i++){
	    if ((this->*func)((int)fx, (int)fy)){
		break;
	    }
	    ty = fy;
	}
	fy = ty;

	*px = (short)fx;
	*py = (short)fy;
    }
}

/************************************************************************
*									*
*  Move the frame.							*
*  Note that the frame can not be moved into out of bound of graphics	*
*  region (canvas).  In addition, it cannot overlap any other frame	*
*  if the "frame_overlap_flag" flag is TRUE.				*
*									*/
void
Gframe::move(short x, short y, Flag frame_overlap_flag)
{
    Gframe *ptr;			// temporary pointer
    short dist_x, dist_y;	// distance to move
    short nx1, ny1, nx2, ny2;	// new position
    int max_x, max_y;		// maximum position to be moved to
    static int prevx;
    static int prevy;
    
    max_x = Gdev_Win_Width(gdev);
    max_y = Gdev_Win_Height(gdev);
    
    // find the distance the cursor has moved
    dist_x = x - basex;
    dist_y = y - basey;
    
    // Same position, do nothing
    if ((!dist_x) && (!dist_y))
    return ;
    
    ptr = selecthead->frameptr;
    if (x1 == G_INIT_POS){	// The first time the frame is moved
	// A selected frame is about to be moved.  Get the frame boundary
	// information (top-left and bottom-right corner points)
	// update the points location.  Check for graphics boundary and 
	// overlapped frame.
	if (((nx1 = ptr->x1 + dist_x) < 0) || ((ny1 = ptr->y1 + dist_y) < 0) ||
	    ((nx2 = ptr->x2 + dist_x) > max_x) || 
	    ((ny2 = ptr->y2 + dist_y) > max_y) ||
	    (frame_overlap_flag && overlap_frame(nx1, ny1, nx2, ny2, FALSE)))
	{
	    // Wont fit there; initialize to orig. location
	    x = basex;
	    y = basey;
	    nx1 = ptr->x1;
	    ny1 = ptr->y1;
	    nx2 = ptr->x2;
	    ny2 = ptr->y2;
	}
    }else{
	// The frame is being dragged
	// update the points location.  Check for graphics boundary and
	// overlapped frame.
#ifdef LINUX
        constrain(&Gframe::overlap_resized_frame, prevx, prevy, &x, &y);
#else
	constrain(Gframe::overlap_moved_frame, basex, basey, &x, &y);
#endif
	nx1 = x1 + x - basex;
	ny1 = y1 + y - basey;
	nx2 = x2 + x - basex;
	ny2 = y2 + y - basey;
	if (ptr->imginfo){
	    ptr->imginfo->pixstx += x - basex;
	    ptr->imginfo->pixsty += y - basey;
	}

	draw(line_thin);	// Erase previous frame
    }
    
    // Update the new points.  Assign the points in the first item of the
    // frame list (which serves as a working frame buffer)
    x1 = nx1;
    y1 = ny1;
    x2 = nx2;
    y2 = ny2;
    
    draw(line_thin);
    
    // Update the base point
    basex = x; basey = y;
}

/************************************************************************
*									*
*  Add a new frame into the FIRST item of the list in Gframe.  Note that*
*  the "working buffer" frame has become the actual frame.  We create	*
*  a new frame to serve as "working buffer".				*
*									*/
Gframe *
Gframe::insert(void)
{
  Gframe *temp = new Gframe;
  
  temp->next = this;
  
  // Increase the number frame
  num_frame += 1;
  
  return (temp);
}

/************************************************************************
*									*
*  Add a new frame to the end of the list of Gframes.
*									*/
void
Gframe::append()
{
    Gframe *temp;
    if (framehead == this){
	/* This is the working buffer, so we need to create a new one */
	framehead = new Gframe;
	framehead->next = this->next;
    }
    for (temp=framehead; temp->next; temp=temp->next);
    /* temp points to last gframe */
    temp->next = this;
    this->next = NULL;
  
    num_frame += 1;
}

/************************************************************************
*									*
*  Add a new frame into the FIRST item of the list in Gframe.  Note that*
*  the "working buffer" frame has become the actual frame.  We create	*
*  a new frame to serve as "working buffer".				*
*									*/
Gframe *
Gframe::insert_after(void)
{
  Gframe *temp = new Gframe;
  
  temp->next = next;
  next = temp;
  
  // Increase the number frame
  num_frame += 1;
  
  return (this);
}

/************************************************************************
* remove_image								*
*	Removes an image from the selected Gframe			*
*									*/
void
Gframe::remove_image(void)
{
   // Erase image from graphics frame.
   clear();		

  // Delete image data information
  detach_imginfo(imginfo);

  // Remove any overlays;
  RemoveAllOverlays();
  
  // Delete parameter set
  if (params)
    {
      PS_release(params);
      params = NULL;
    }
  
}

/************************************************************************
*									*
*  Remove images from frames.
*  [MACRO interface]
*  argv[0]: (char *) Which frames to clear:
*		"selected" | "unselected" | "all"
*  [STATIC Function]							*
*									*/
int
Gframe::Clear(int argc, char **argv, int, char **)
{
    argc--; argv++;

    char *mode;
    F_props_menu type;

    if (argc != 1){
	ABORT;
    }
    mode = argv[0];

    if (strcasecmp(mode, "selected") == 0){
	type = F_MENU_CLEAR_SELECT;
    }else if (strcasecmp(mode, "unselected") == 0){
	type = F_MENU_CLEAR_UNSELECT;
    }else if (strcasecmp(mode, "all") == 0){
	type = F_MENU_CLEAR_ALL;
    }else{
	ABORT;
    }

    framehead->clear_frame(type);
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Remove images frome all selected frames from the list. 		*
*									*/
void
Gframe::clear_frame(F_props_menu type)
{
  Gframe *ptr, *prev;		// loop pointers
  char *stype;
  
  if (type == F_MENU_CLEAR_ALL)
    {
	stype = "all";
        prev = this;
        while (ptr = prev->next)
	{
	    ptr->remove_image();
	    prev = ptr;
	}
    }
  else if (type == F_MENU_CLEAR_SELECT)
    {
	stype = "selected";
        prev = this;
        while (ptr = prev->next)
	{
	    if (ptr->select)
	    {
	      ptr->remove_image();
	    }
	    prev = ptr;
	}
    }
  else if (type == F_MENU_CLEAR_UNSELECT)
    {
	stype = "unselected";
        prev = this;
        while (ptr = prev->next)
	{
	    if (!ptr->select)
	    {
	      ptr->remove_image();
	    }
	    prev = ptr;
	}
    }
  macroexec->record("frame_clear('%s')\n", stype);
}

/************************************************************************
*									*
*  Display the image and overlays associated with this frame.
*/
void
Gframe::display_data()
{

    if (! imginfo){
	msgerr_print("Gframe:No data in frame!\n");
	return;
    }
    ImginfoIterator element(overlay_list);
    Imginfo *overlay;

    if (! update_all_image_positions()) {
    	return;
    }

    display_init();
    set_clip_region(FRAME_NO_CLIP);	// Needed to write pixmaps
    imginfo->display_data(this);
    while (element.NotEmpty()){
	overlay = ++element;
	overlay->overlay_data(this);
    }

    set_clip_region(FRAME_NO_CLIP);
    display_end();
}

/************************************************************************
*									*
*  Remove images and delete frames.
*  [MACRO interface]
*  argv[0]: (char *) Which frames to delete:
*		"selected" | "unselected" | "empty" | "all"
*  [STATIC Function]							*
*									*/
int
Gframe::Delete(int argc, char **argv, int, char **)
{
    argc--; argv++;

    char *mode;
    F_props_menu type;

    if (argc != 1){
	ABORT;
    }
    mode = argv[0];

    if (strcasecmp(mode, "selected") == 0){
	type = F_MENU_DELETE_SELECT;
    }else if (strcasecmp(mode, "unselected") == 0){
	type = F_MENU_DELETE_UNSELECT;
    }else if (strcasecmp(mode, "all") == 0){
	type = F_MENU_DELETE_ALL;
    }else if (strcasecmp(mode, "empty") == 0){
	type = F_MENU_DELETE_EMPTY;
    }else{
	ABORT;
    }

    framehead->remove(type);
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Remove images AND Delete all selected frames from the list.		*
*									*/
void
Gframe::remove(F_props_menu type)
{
  Gframe *ptr, *prev;		// loop pointers
  char *stype;
  
  if (type == F_MENU_DELETE_ALL)
    {
      stype = "all";
      while (ptr = next)
	{
	  next = ptr->next;
          ptr->remove_image();
          if (targetframe == ptr) targetframe=framehead;
	  delete ptr;
	}
      selecthead->remove(REMOVE_SELECT_ALL_ITEM);
    }
  else if (type == F_MENU_DELETE_EMPTY)
    {
      stype = "empty";
      prev = this;
      while (ptr = prev->next)
	{
	  if (!ptr->imginfo){
	      prev->next = ptr->next;
	      if (targetframe == ptr) targetframe=framehead;
	      if (ptr->select){
		  selecthead->remove(ptr);
	      }
	      delete ptr;
	    }
	  else
	    prev = ptr;
	}
    }
  else if (type == F_MENU_DELETE_SELECT)
    {
      stype = "selected";
      prev = this;
      while (ptr = prev->next)
	{
	  if (ptr->select)
	    {
	      prev->next = ptr->next;
	      ptr->remove_image();
	      if (targetframe == ptr) targetframe=framehead;
	      delete ptr;
	    }
	  else
	    prev = ptr;
	}
      selecthead->remove(REMOVE_SELECT_ALL_ITEM);
    }
  else if (type == F_MENU_DELETE_UNSELECT)
    {
      stype = "unselected";
      prev = this;
      while (ptr = prev->next)
	{
	  if (!ptr->select)
	    {
	      prev->next = ptr->next;
	      ptr->remove_image();
	      if (targetframe == ptr) targetframe=framehead;
	      delete ptr;
	    }
	  else
	    prev = ptr;
	}
    }
  macroexec->record("frame_delete('%s')\n", stype);
}

/************************************************************************
*									*
*  This routine is to search through for all frames to see if the mouse	*
*  is close enough or inside the frame.  If it is, the the action is	*
*  the following:							*
*	- select the frame if it is not selected and the mouse cursor	*
*         inside the frame.						*
*       - (the frame is selected) resize the frame if the mouse is	*
*         close enough to the corner of the frame.			*
*       - (the frame is selected) move the frame if the mouse 		*
*          cursor is inside the frame.					*
*									*
*  The selected frame will be pointed by 'selechead->frameptr'.		*
*									*/
Flag
Gframe::is_selected(int xview_act, // only ACTION_SELECT or ACTION_ADJUST
		    short x, short y)
{
  register Gframe *ptr;
  
  for (ptr=next; ptr; ptr=ptr->next)
    {
      // (xview) ACTION_ADJUST can not be used to resize or move the 
      // the frame.  SO this block only allows ACTION_SELECT
      if ((ptr->select)	&& (xview_act == ACTION_SELECT))
	{
	  if (ptr->corner_selected(x, y))
	    {
	      acttype = FRAME_RESIZE;
	      selecthead->frameptr = ptr;
	      return(TRUE);
	    }
	  else if (com_point_in_rect(x, y, ptr->x1, ptr->y1, ptr->x2, ptr->y2))
	    {
	      acttype = FRAME_MOVE;
	      selecthead->frameptr = ptr;
	      return(TRUE);
	    }
	}
      else if (!ptr->select || xview_act == ACTION_ADJUST)
	{
	  if (com_point_in_rect(x, y, ptr->x1, ptr->y1, ptr->x2, ptr->y2))
	    {
	      selecthead->frameptr = ptr;
	      acttype = (xview_act == ACTION_SELECT) ? 
		FRAME_SELECT : FRAME_TOGGLE;
	      return(TRUE);
	    }
	}
      else
      {
	  fprintf(stderr,"IB internal error: xview_act=%d\n", xview_act);
	  return TRUE;
      }
    }
  selecthead->frameptr = NULL;
  return(FALSE);
}

/************************************************************************
*									*
*  Find out if this is a selected frame.
*									*/
Flag
Gframe::is_a_selected_frame()
{
    return select;
}

/************************************************************************
*									*
*  Draw a frame.							*
*									*/
void
Gframe::draw(int line_wd)
{
  int linewidth = G_Get_LineWidth(gdev);
  G_Set_LineWidth(gdev, line_wd);
  g_draw_rect(gdev, x1, y1, x2-x1, y2-y1, color);
  G_Set_LineWidth(gdev, linewidth);	// Set back to default
}

/************************************************************************
*									*
*  Check if a point is close enough to the corner of the frame.		*
*  If it is, then set the base point to the opposite (across) the	*
*  corner (because this is the way we interactively create a frame).	*
*  Return TRUE or FALSE.						*
*  (This event will detect the RESIZE routine).				*
*									*/
Flag
Gframe::corner_selected(short x, short y)	// point to be tested
{
  
  if (Corner_Check(x, y, x1, y1, G_APERTURE))	// upper left
    {
      basex = x2; basey = y2;
      framehead->x1 = G_INIT_POS;
      framehead->x2 = x1; framehead->y2 = y1;
      return(TRUE);
    }
  else if (Corner_Check(x, y, x2, y1, G_APERTURE))	// upper right
    {
      basex = x1; basey = y2;
      framehead->x1 = G_INIT_POS;
      framehead->x2 = x2; framehead->y1 = y1;
      return(TRUE);
    }
  else if (Corner_Check(x, y, x2, y2, G_APERTURE))	// lower right
    {
      basex = x1; basey = y1;
      framehead->x1 = G_INIT_POS;
      framehead->x2 = x2; framehead->y2 = y2;
      return(TRUE);
    }
  else if (Corner_Check(x, y, x1, y2, G_APERTURE))	// lower left
    {
      basex = x2; basey = y1;
      framehead->x1 = G_INIT_POS;
      framehead->x2 = x1; framehead->y2 = y1;
      return(TRUE);
    }
  
  return(FALSE);
}
/************************************************************************
*									*
*  Do a necessary set-up or clean-up before displaying image.		*
*									*/
void
Gframe::display_init()
{
    // Clear display-region
    clear();
    
    // Unmark if it is marked
    if (select){
	mark();
    }
}

/************************************************************************
*									*
*  Do a necessary clean-up after displaying image.			*
*									*/
void
Gframe::display_end()
{
    // Print out title if it is required.  First try to use medium font,
    // and it it doesn't fit in, use small font.  If it still doesn't fit
    // in, clip the title.
    
    // Mark a frame if it is selected
    if (select){
	mark();
    }
    
    // Indicate the frame is not clean
    clean = FALSE; 
}

/************************************************************************
*									*
*  Return TRUE if the specified location would cause this frame to
*  overlap another frame or go off the canvas, else FALSE.
*  The location is relative to basex, basey.
*  Note that the current selected frame is in variable
*  "selecthead->frameptr".  We ignore overlaps with ourself
*  (the current selected frame).
*									*/
Flag
Gframe::overlap_moved_frame(int x, int y)
{
    short m1 = (short)(x1 + x - basex);
    short n1 = (short)(y1 + y - basey);
    short m2 = (short)(x2 + x - basex);
    short n2 = (short)(y2 + y - basey);
    int maxx = Gdev_Win_Width(gdev) - 1;
    int maxy = Gdev_Win_Height(gdev) - 1;
    if (m1<0 || n1<0 || m2>maxx || n2>maxy){
	return TRUE;
    }
    return (overlap_frame(m1, n1, m2, n2, FALSE));
}

/************************************************************************
*									*
*  Return TRUE if the specified location would cause this frame to
*  overlap another frame or go off the canvas, else FALSE.
*  The location is relative to basex, basey.
*  Note that the current selected frame is in variable
*  "selecthead->frameptr".  We ignore overlaps with ourself
*  (the current selected frame).
*									*/
Flag
Gframe::overlap_resized_frame(int x, int y)
{
    short m1 = basex;
    short n1 = basey;
    short m2 = x;
    short n2 = y;
    com_minmax_swap(m1, m2);
    com_minmax_swap(n1, n2);
  
    int maxx = Gdev_Win_Width(gdev) - 1;
    int maxy = Gdev_Win_Height(gdev) - 1;
    if (m1<0 || n1<0 || m2>maxx || n2>maxy){
	return TRUE;
    }
    return (overlap_frame(m1, n1, m2, n2, FALSE));
}

/************************************************************************
*									*
*  Return TRUE if the current selected frame is overlapped with	others	*
*  frame, else FALSE.							*
*  Note that the current selected frame is in variable 			*
*  "selecthead->frameptr".  If "self_check" flag is FALSE, we ignore 	*
*  the current selected frame. The usage of "self_check" is to indicate	*
*  whether we include current selected frame to be checked or not.	*
*									*/
Flag
Gframe::overlap_frame(short m1, short n1, short m2, short n2, Flag self_check)
{
  register Gframe *ptr;
  
  for (ptr=next; ptr; ptr=ptr->next)
    {
      if (self_check || (ptr != selecthead->frameptr))
	{
          if (com_point_in_rect(m1,n1,ptr->x1,ptr->y1,ptr->x2,ptr->y2) ||
              com_point_in_rect(m2,n1,ptr->x1,ptr->y1,ptr->x2,ptr->y2) ||
              com_point_in_rect(m2,n2,ptr->x1,ptr->y1,ptr->x2,ptr->y2) ||
              com_point_in_rect(m1,n2,ptr->x1,ptr->y1,ptr->x2,ptr->y2) ||
	      com_point_in_rect(ptr->x1,ptr->y1,m1,n1,m2,n2) ||
	      com_point_in_rect(ptr->x2,ptr->y1,m1,n1,m2,n2) ||
	      com_point_in_rect(ptr->x2,ptr->y2,m1,n1,m2,n2) ||
	      com_point_in_rect(ptr->x1,ptr->y2,m1,n1,m2,n2) ||
	      ((m1 > ptr->x1) && (m2 < ptr->x2) && 
	       (n1 < ptr->y1) && (n2 > ptr->y2)) ||
	      ((m1 < ptr->x1) && (m2 > ptr->x2) && 
	       (n1 > ptr->y1) && (n2 < ptr->y2)))
	    return(TRUE);
	}
    }
  return(FALSE);
}

/************************************************************************
*									*
*  Clear the frame region.						*
*  Note that it only erases the region if it is NOT clean (containing)	*
*  spectrum or images. Hence, it we draw something inside the frame, we	*
*  have to set the flag "clean" to FALSE.				*
*									*/
void
Gframe::clear(void)
{
  if (!clean)	// Clean flag not currently set (reset) reliably /*CMP*/
    {
      if (select)		// Unmark
	mark();
      Erase_1(line_fat);
      g_clear_area(gdev, x1, y1, x2-x1, y2-y1, 0); 
      draw(line_fat);
      if (select)		// mark
	mark();
      clean = TRUE;
    }
}

/************************************************************************
*									*
*  Mark the frame.  If the frame is currently marked, it will be unmarked.*
*  Note that it also draws zoom-lines if gtools Zoom is active and if	*
*  the frame contains an image.						*
*									*/
void
Gframe::mark(void)
{
  Gpoint pnts[6];

  //
  // N.B. Since the g_fill_polygon() routine works like XFillPolygon(),
  //	it fills in pixels that are on the top or left boundary, but
  //	not pixels on the bottom or right boundary.  Therefore, in order
  //	to position the four corners the same with respect to the frame
  //	corners, we shift the marker coordinates one down from the top
  //	boundary (y1+1) and one right from the left boundary (x1+1),
  //	while the right and bottom boundaries are used as is (x2 and y2).
  //	Thus, the corner marks do not overlap the frames.
  //
  pnts[0].x = x1+1;			pnts[0].y = y1+1;
  pnts[1].x = pnts[0].x+m_size;		pnts[1].y = pnts[0].y;
  pnts[2].x = pnts[1].x;		pnts[2].y = pnts[1].y+m_curve;
  pnts[3].x = pnts[2].x-m_size+m_curve;	pnts[3].y = pnts[2].y;
  pnts[4].x = pnts[3].x;		pnts[4].y = pnts[3].y-m_curve+m_size;
  pnts[5].x = pnts[0].x;		pnts[5].y = pnts[4].y;
  g_fill_polygon(gdev, pnts, 6, color);
  
  pnts[0].x = x2;			pnts[0].y = y1+1;
  pnts[1].x = pnts[0].x-m_size;		pnts[1].y = pnts[0].y;
  pnts[2].x = pnts[1].x;		pnts[2].y = pnts[1].y+m_curve;
  pnts[3].x = pnts[2].x+m_size-m_curve; pnts[3].y = pnts[2].y;
  pnts[4].x = pnts[3].x;		pnts[4].y = pnts[3].y-m_curve+m_size;
  pnts[5].x = pnts[0].x;		pnts[5].y = pnts[4].y;
  g_fill_polygon_default(gdev, pnts, 6);
  
  pnts[0].x = x2;			pnts[0].y = y2;
  pnts[1].x = pnts[0].x-m_size;		pnts[1].y = pnts[0].y;
  pnts[2].x = pnts[1].x;		pnts[2].y = pnts[1].y-m_curve;
  pnts[3].x = pnts[2].x+m_size-m_curve;	pnts[3].y = pnts[2].y;
  pnts[4].x = pnts[3].x;		pnts[4].y = pnts[3].y+m_curve-m_size;
  pnts[5].x = pnts[0].x;		pnts[5].y = pnts[4].y;
  g_fill_polygon_default(gdev, pnts, 6);
  
  pnts[0].x = x1+1;			pnts[0].y = y2;
  pnts[1].x = pnts[0].x+m_size;		pnts[1].y = pnts[0].y;
  pnts[2].x = pnts[1].x;		pnts[2].y = pnts[1].y-m_curve;
  pnts[3].x = pnts[2].x-m_size+m_curve;	pnts[3].y = pnts[2].y;
  pnts[4].x = pnts[3].x;		pnts[4].y = pnts[3].y+m_curve-m_size;
  pnts[5].x = pnts[0].x;		pnts[5].y = pnts[4].y;
  g_fill_polygon_default(gdev, pnts, 6);
  
  if (Zoom_routine::zoom_active() && imginfo) 
			imginfo->draw_zoom_lines(Zoomf::get_zoom_color());
  if (select) targetframe = this;
}

/************************************************************************
*									*
*  Split this frame into multiple frames.
*									*/
void
Gframe::split(int numrow, int numcol)
{
  int row, col;	// loop counter for row and columns
  short size_x, size_y;// current size
  
  size_x = ((x2 - x1) - numcol + 1) / numcol;
  size_y = ((y2 - y1) - numrow + 1) / numrow;
  
  if (size_x < min_width() || size_y < min_height()){
    msginfo_print("Frame too small to be split %d X %d\n", numrow, numcol);
    return;
  }
  
  // Clear area inside the frame
  clear();
  
  // Erase current frame
  Erase_1(line_fat);
  mark();
  
  Gframe *fptr;
  
  for (row = numrow-1; row >= 0; row--) {
    for (col = numcol-1; col >= 0; col--) {
      if (row == 0 && col == 0) {
	// This codes just change the size of the original frame
	x2 = x1 + size_x;
	y2 = y1 + size_y;
	draw(line_fat);
	mark();
	
	// Display data
	if (imginfo)
	  display_data();
      } else {
	fptr = insert_after()->next;
	fptr->x1 = x1 + (size_x + 1) * col;
	fptr->y1 = y1 + (size_y + 1) * row;
	fptr->x2 = fptr->x1 + size_x;
	fptr->y2 = fptr->y1 + size_y;
	fptr->draw(line_fat);
      }
    }
  }
}

/************************************************************************
*									*
*  Select frames.
*  [MACRO interface]
*  argv[0]: (char *) Which frames to select:
*			"all"
*  [STATIC]
*									*/
int
Gframe::Select(int argc, char **argv, int, char **)
{
    char *arg;
    int ival;
    Gframe *gf;

    argc--; argv++;

    if (argc < 1){
	ABORT;
    }

    while (argc--){
	arg = *argv++;
	if (strcasecmp(arg, "all") == 0){
	    framehead->select_all();
	}else if (strcasecmp(arg, "none") == 0){
	    selecthead->deselect();
	}else if (sscanf(arg, "%i", &ival) == 1){
	    if (gf = get_frame_by_number(ival)){
		gf->select_frame();
	    }
	}else{
	    ABORT;
	}
    }
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Add this frame to the selection list.
*  It is also made the "targetframe" for loading images.
*									*/
void
Gframe::select_frame()
{
    char buf[80];
  
    if (!select){
	set_select(TRUE);
	mark();
	selecthead->insert(this);
    }
    targetframe = this;

    sprintf(buf,"frame_select(%d)\n", get_frame_number());
    macroexec->record(buf);
}

/************************************************************************
*									*
*  Select all the frames.						*
*									*/
void
Gframe::select_all(void)
{
  Gframe *ptr;
  
  for (ptr=next; ptr; ptr=ptr->next)    {
    if (!ptr->select)	{
      ptr->mark();
      selecthead->insert(ptr);
    }
  }
  macroexec->record("frame_select('all')\n");
}

/************************************************************************
*                                                                       *
*  Return x1 where x1,y1 is top left corner of frame                    *
*                                                                       */
short Gframe::get_top_left_x(void)
{ return x1; }

/************************************************************************
*                                                                       *
*  Return y1 where x1,y1 is top left corner of frame                    *
*                                                                       */
short Gframe::get_top_left_y(void)
{ return y1; }

/************************************************************************
*                                                                       *
*  Return x2 where x2,y2 is top left corner of frame                    *
*                                                                       */
short Gframe::get_bottom_right_x(void)
{ return x2; }

/************************************************************************
*                                                                       *
*  Return y2 where x2,y2 is bottom right corner of frame                *
*                                                                       */
short Gframe::get_bottom_right_y(void)
{ return y2; }

/************************************************************************
*                                                                       *
*  Return first frame.                                                  *
*                                                                       */
Gframe *
Gframe::get_first_frame(void)
{
  return framehead->next;
}

/************************************************************************
*                                                                       *
*  Return next frame.                                                   *
*                                                                       */
Gframe *
Gframe::get_next_frame(Gframe *g)
{
  return g->next;
}

/************************************************************************
*                                                                       *
*  Return the handle of a frame with a given number
*                                                                       */
Gframe *
Gframe::get_frame_by_number(int n)
{
    int i;
    Gframe *gptr;

    for (i=1, gptr= Gframe::get_first_frame();
	 i<n && gptr;
	 i++, gptr= Gframe::get_next_frame(gptr))
    { }
    if (i != n){
	return NULL;
    }else{
	return gptr;		// (Could be NULL)
    }
}

/************************************************************************
*                                                                       *
*  Return order of this frame in Gframe list
*                                                                       */
int
Gframe::get_frame_number()
{
    Gframe *gf = get_first_frame();
    int n = 1;
    while (gf && gf != this){
	gf = get_next_frame(gf);
	n++;
    }
    if (gf){
	return n;
    }else{
	return -1;
    }
}

/************************************************************************
*                                                                       *
*  Check to see if a frame really exists.
*  [STATIC function]							*
*                                                                       */
int
Gframe::exists(const Gframe *g)
{
    Gframe *f;
    for ( (f = framehead->next); f; f = f->next){
	if (f == g){
	    return TRUE;
	}
    }
    return FALSE;
}

/************************************************************************
*									*
*  Properties menu handler for frame.					*
*  <STATIC function>							*
*									*/
void
Gframe::menu_handler(Menu m, Menu_item i)
{
  F_props_menu cldata;
  int row, col;
  int num_menu_items;
  Menu_item mi;
  
  switch (cldata = (F_props_menu)xv_get(i, MENU_CLIENT_DATA))    {
    
  case F_MENU_SPLIT:
      if (sscanf((char *)xv_get(i, MENU_STRING),"%d x %d", &row, &col) == 2){
	  Frame_select::split(row, col);
      }else{
	  ((Frame_routine *)func[GTOOL_FRAME])->show_splitter_popup();
      }
      break;
    
  case F_MENU_DELETE_SELECT:
  case F_MENU_DELETE_ALL:
  case F_MENU_DELETE_UNSELECT:
  case F_MENU_DELETE_EMPTY:
    framehead->remove(cldata);
    break;
    
  case F_MENU_CLEAR_SELECT:
  case F_MENU_CLEAR_ALL:
  case F_MENU_CLEAR_UNSELECT:
    framehead->clear_frame(cldata);
    break;
    
  case F_MENU_SELECT_ALL:
    framehead->select_all();
    break; 
    
  case F_MENU_TITLE:
    title = title ? FALSE : TRUE;
    break;
    
  case F_MENU_LOAD:
    // We don't need to handler this routine because it is handled by 
    // menu_load_pullright and
    // menu_load because it generates a pullright menu.
    break;
    
  case F_MENU_SAVE:
    {
      char dirname[1024];
      (void)init_get_env_name(dirname);
      (void)strcat(dirname, "/gframe");      
      filelist_win_show(FILELIST_GFRAME_ID, FILELIST_SAVE, dirname,
			"File Browser: Graphics Frame");
    }
    break;

  case F_MENU_FULL:
    num_menu_items = (int) xv_get(m, MENU_NITEMS, NULL);
    mi = (Menu_item) xv_create(NULL, MENUITEM,
                     MENU_STRING,              "Un-Fullscreen",
                     MENU_NOTIFY_PROC,         &Gframe::menu_handler,
		     MENU_CLIENT_DATA,         F_MENU_UNFULL,
                     MENU_RELEASE,
                     NULL);
    xv_set (m, MENU_REMOVE, num_menu_items, NULL);
    xv_set (m, MENU_APPEND_ITEM, mi, NULL);

    Zoomf::zoom_full(Z_FULLSCREEN) ;
    break;

  case F_MENU_UNFULL:
    num_menu_items = (int) xv_get(m, MENU_NITEMS, NULL);
    mi = (Menu_item) xv_create(NULL, MENUITEM,
                     MENU_STRING,              "Fullscreen",
                     MENU_NOTIFY_PROC,         &Gframe::menu_handler,
		     MENU_CLIENT_DATA,         F_MENU_FULL,
                     MENU_RELEASE,
                     NULL);
    xv_set (m, MENU_REMOVE, num_menu_items, NULL);
    xv_set (m, MENU_APPEND_ITEM, mi, NULL);

      Zoomf::zoom_full(Z_UNFULLSCREEN) ;
    break;
  }
}
   

/************************************************************************
*                                                                       *
*  User has selected a pulright menu for load procedure.  Generate      *
*  the listing of directory files.  Note that it registers              *
*  "menu_load" function to call if there is a selection.            	*
*  [STATIC function]							*
*                                                                       */
Menu
Gframe::menu_load_pullright(Menu_item mi, Menu_generate op)
{
  if (op == MENU_DISPLAY)    {
    char filename[128];
    
    /* Get the proper directory path */
    (void)init_get_env_name(filename);
    (void)strcat(filename, "/gframe");
    return(filelist_menu_pullright(mi, op, 
				   (u_long)&Gframe::menu_load, filename));
  }
  return(filelist_menu_pullright(mi, op, NULL, NULL));
}

/************************************************************************
*									*
*  Load a set of Gframes
*  [MACRO interface]
*  argv[0]: (char *) Full path of file
*  [STATIC Function]							*
*									*/
int
Gframe::Load(int argc, char **argv, int, char **)
{
    argc--; argv++;

    if (argc != 1){
	ABORT;
    }
    char *filename = argv[0];
    char fname[1024];
    if (*filename == '/'){
	*fname = '\0';
    }else{
	init_get_env_name(fname);	/* Get the directory path */
	strcat(fname, "/gframe/");
    }
    strcat(fname, filename);
    menu_load("", fname);
    return PROC_COMPLETE;
}

/************************************************************************
*                                                                       *
*  The user has selected a menu item. Execute to load Gframe.           *
*  It loads a file containg format described in "Gframe::menu_save".	*
*  [STATIC function]							*
*                                                                       */
void
Gframe::menu_load(char *dirpath,       // directory path name
                  char *name)          // filename to be loaded
{
  ifstream infile;     // input stream
  char filename[128];  // complete filename
  char buf[128];	// input buffer
  int select_flag;	// value of 0 or 1
  Gframe *lastframe;
  
  // Make sure user has specified the input filename
  if (*name == NULL)
    {
      msgerr_print("menu_load: Need to specify input filename for loading");
      return;
    }
  
  (void)sprintf(filename, "%s/%s", dirpath, name);
  
  infile.open(filename, ios::in);
  if (infile.fail())    {
    msgerr_print("menu_load:Couldn't open ``%s'' for reading", filename);
    return;
  }

  // Find the last gframe
  for (lastframe=framehead; lastframe->next; lastframe=lastframe->next);
  
  // Read input a line at a time.  Then, use frame "working" buffer to
  // store the input values
  while (infile.getline(buf, 128))   {
    if (buf[0] == '#')
      continue;
    
    if (sscanf(buf,"%hd %hd %hd %hd %f %f %d", &framehead->x1,
	      &framehead->y1, &framehead->x2, &framehead->y2, &framehead->xoff,
	       &framehead->yoff, &select_flag) != 7)      {
	msgerr_print("menu_load:Incorrect input found in %s", filename);
	break;
      }
	
    // A frame exists if it is large enough
    if (framehead->x2 - framehead->x1 < min_width()
	|| framehead->y2 - framehead->y1 < min_height())
    {
	  msgerr_print("menu_load: A frame is too small:(%hd,%hd) (%hd,%hd)",
		       framehead->x1, framehead->y1,
		       framehead->x2, framehead->y2);
	  continue;
	}

    // A frame should be at least partly within graphics area
    if ((framehead->x1 >= Gdev_Win_Width(gdev)) ||
	(framehead->y1 >= Gdev_Win_Height(gdev)) || 
	(framehead->x2 <= 0) || (framehead->y2 <= 0))
    {
	msgerr_print("menu_load:A frame is out of bounds:(%hd,%hd) (%hd,%hd)",
		     framehead->x1, framehead->y1,
		     framehead->x2, framehead->y2);
	continue;
    }
	  
    // A frame can not overlap with other frame
    if (framehead->overlap_frame(TRUE))   {
      msgerr_print("menu_load:One frame overlaps another:(%hd,%hd) (%hd,%hd)",
		   framehead->x1, framehead->y1, framehead->x2, framehead->y2);
      continue;
    }

    framehead->draw(line_fat);

    lastframe->insert_after();	// Append the (null) frame.
    lastframe = lastframe->next;

    // Copy in the values
    lastframe->x1 = framehead->x1;
    lastframe->y1 = framehead->y1;
    lastframe->x2 = framehead->x2;
    lastframe->y2 = framehead->y2;
    lastframe->xoff = framehead->xoff;
    lastframe->yoff = framehead->yoff;

    if (select_flag)  {
      lastframe->mark();
      selecthead->insert(lastframe);
    }
  }
  
  infile.close();
  macroexec->record("frame_load('%s')\n", filename);
}


/************************************************************************
*                                                                       *
*  Frame_routine::redraw_frame(void)						*
*                                                                       *
*  This procedure should get called whenever the system requires the    *
*  graphics application to redraw all of its objects.                   *
*                                                                       *
*                                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

     
void Frame_routine::redraw_frame(void)
{
#define	BACK_COLOR	0
  G_Set_Op(Gframe::gdev,GXcopy);
  g_clear_area(Gframe::gdev,
	       0, 0,
	       Gdev_Win_Width(Gframe::gdev), Gdev_Win_Height(Gframe::gdev),
	       BACK_COLOR);
  
  Frame_data::redraw_all_images();

  Gframe *frame_ptr = framehead->next;
  while (frame_ptr) {
    frame_ptr->draw(line_fat);	
    frame_ptr = frame_ptr->next;
    }
  G_Set_Op(Gframe::gdev,GXxor);
#undef BACK_COLOR
}

/*********************************************************************
*								
*    Get Gframe from position ( x, y )
*
*/
Gframe *
Gframe::get_gframe_with_pos(short x, short y, int *frameno)
{
    Gframe *ptr;
    int i;

    if (frameno){
	*frameno = 0;
    }
    for (i=1, ptr=framehead->next; ptr; i++, ptr=ptr->next) {
	if( com_point_in_rect(x, y, ptr->x1, ptr->y1, ptr->x2, ptr->y2))
	{
	    //	 printf(" ptr : %10ld \n", ptr ) ;
	    //	 printf(" ptr->frame_no : %6d \n", ptr->frame_no ) ;
	    if (frameno){
		*frameno = i;
	    }
	    return( ptr );
	}
    }
    return( NULL ) ;
}

/************************************************************************
*									*
*  Save a set of Gframes
*  [MACRO interface]
*  argv[0]: (char *) Full path of file
*  [STATIC Function]							*
*									*/
int
Gframe::Save(int argc, char **argv, int, char **)
{
    argc--; argv++;

    if (argc != 1){
	ABORT;
    }
    char *filename = argv[0];
    char fname[1024];
    if (*filename == '/'){
	*fname = '\0';
    }else{
	init_get_env_name(fname);	/* Get the directory path */
	strcat(fname, "/gframe/");
    }
    strcat(fname, filename);
    menu_save("", fname);
    return PROC_COMPLETE;
}

/************************************************************************
*                                                                       *
*  The user wants to save Gframe. Execute to save Gframe.               *
*  The result of the outfile contains the following format:		*
*									*
*     # <comments>                                                      *
*     x1 y1 x2 y2 xoff yoff <0|1>					*
*									*
*  where								*
*	# indicates comments						*
*	x1 y1 y x2 y2 are posistions of top-left/bottom-right corners	*
*	xoff yoff are x and y offset of image/spectrum			*
*       <0:1> is either value 0 or 1 (not select or select).		*
*  [STATIC Function]							*
*                                                                       */
void
Gframe::menu_save(char *dirpath,       // directory path name
                  char *name)          // filename to be loaded
{                                            
   ofstream outfile;    // output stream     
   char filename[128];  // complete filename 
   long clock;          // number of today seconds
   char *tdate;         // pointer to the time
   char *tlogin;        // pointer to login name
   char thost[80];      // hostname buffer   
   Gframe *gptr;	// Gframe pointer
   char buf[128];	// output format
                                             
   // Check if there is any frame on the graphics area
   if (framehead->next == NULL)
   {                                         
      msgerr_print("menu_save:No Frame exists");         
      return;                                
   }                                         
                                             
   // Check if user has specified an output name
   if (*name == NULL)
   {
      msgerr_print("menu_save:Need to specify output filename for saving");
      return;
   }

   (void)sprintf(filename,"%s/%s", dirpath, name);
                                             
   outfile.open(filename, ios::out);         
   if (outfile.fail())                                                    
   {
      msgerr_print("menu_dave:Couln'd open ``%s'' for writing", filename);
      return;                                                           
   }
                                       
   // Output the comments                  
   clock = time(NULL);
   if ((tdate = ctime(&clock)) != NULL)      
      tdate[strlen(tdate)-1] = 0;
   if ((tlogin = (char *)cuserid(NULL)) == NULL)
      msgerr_print("menu_save:Warning: Couldn't find login name");
   if (gethostname(thost,80) != 0)
      msgerr_print("menu_save:Warning:Couldn't find host name");
                                             
   outfile << "# ** Created by " << tlogin << " on " << tdate
           << " at machine " << thost << " **" << "\n";
   outfile << "# frame: x1 y1 x2 y2 x_off y_off select_flag\n" ;
                     
                                             
   for (gptr=framehead->next; gptr; gptr=gptr->next)
   {
      // NOTE: I try to use: outfile << form("........");
      //       but it doesnt work
      (void)sprintf(buf,"%4hd %4hd %4hd %4hd  %1.6f  %1.6f  %d\n",
	 gptr->x1, gptr->y1, gptr->x2, gptr->y2, gptr->xoff,
	 gptr->yoff, (int)gptr->select);
      outfile << buf;
   }

   outfile.close();
    
   filelist_update();
   macroexec->record("frame_save('%s')\n", filename);
}

/************************************************************************
*                                                                       *
*  Set the frame color.
*  <STATIC>
*/
void
Gframe::frame_set_color(Menu, Menu_item item)
{
    int newcolor = (int)xv_get(item, MENU_VALUE);
    int oldcolor = color;

    // Erase each frame and draw it with new color.
    for (Gframe *ptr=framehead->next; ptr; ptr=ptr->next)
    {
	color = oldcolor ;
	Erase_2(ptr,line_fat);
	if (ptr->select) ptr->mark();
	color = newcolor;
	ptr->draw(line_fat);
	if (ptr->select) ptr->mark();
    }
}

/************************************************************************
*									*
*  Get image inside a frame.						*
*  Return a pointer to an image or NULL.				*
*									*/
XImage *
Gframe::get_image(short pix_x, short pix_y,	// starting pixel point
		  short pix_wd, short pix_ht)	// width and height
{
   XImage *ximg;

   if (imginfo == NULL)
   {
      msgerr_print("get_image:No image in a frame\n");
      return(NULL);
   }

   // Check for image boundary
   if ((pix_x < imginfo->pixstx) || (pix_y < imginfo->pixsty) ||
       ((pix_x+pix_wd) > (imginfo->pixstx+imginfo->pixwd)) ||
       ((pix_y+pix_ht) > (imginfo->pixsty+imginfo->pixht)))
   {
      msgerr_print(
	 "get_image:Requested dimension (x=%d,y=%d) (w=%d,h=%d) is out of limit",
	 (int)pix_x, (int)pix_y, (int)pix_wd, (int)pix_ht);
      return(NULL);
   }

   if (select)		// Erase the mark
      mark();

   ximg = XGetImage(gdev->xdpy, gdev->xid, pix_x, pix_y, pix_wd, pix_ht, 
	  AllPlanes, ZPixmap); 

   if (select)		// Draw the mark
      mark();

   if (ximg == NULL)
   {
       msgerr_print("get_image:Couldn't get image:XGetImage");
       return(NULL);
   }
   return(ximg);
}


/************************************************************************
*									*
*  Set the clip region to correspond to a particular frame.
*/
void
Gframe::set_clip_region(ClipStyle style)
{
    if (style == FRAME_NO_CLIP){
	// Turn off clipping
	XSetClipMask(gdev->xdpy,	// Display device
		     gdev->xgc,		// Graphics context
		     None);		// No clipping
    }else if (style == FRAME_CLIP_TO_IMAGE && imginfo){
	// Clip to area of image
	XRectangle rectangle[1];
	rectangle->x = imginfo->pixstx;
	rectangle->y = imginfo->pixsty;
	rectangle->width = imginfo->pixwd;
	rectangle->height = imginfo->pixht;
	XSetClipRectangles(gdev->xdpy,	// Display device
			   gdev->xgc,	// Graphics context
			   0, 0,	// Clip origin
			   rectangle,	// Rectangle to clip to
			   1,		// Only 1 rectangle,
			   Unsorted);	//  and it's not sorted
    }else if (style == FRAME_CLIP_TO_FRAME){
	// Clip to interior of frame
	XRectangle rectangle[1];
	rectangle->x = min_x();
	rectangle->y = min_y();
	rectangle->width = max_x() - min_x() + 1;
	rectangle->height = max_y() - min_y() + 1;
	XSetClipRectangles(gdev->xdpy,	// Display device
			   gdev->xgc,	// Graphics context
			   0, 0,	// Clip origin
			   rectangle,	// Rectangle to clip to
			   1,		// Only 1 rectangle,
			   Unsorted);	//  and it's not sorted
    }
}



//======================================================================
//
//  			DEBUG CODES
//
//======================================================================

void
Gframe::debug_frame()
{
   Gframe *ptr;

   msginfo_print("\nNumber of frame %d\n", num_frame);
   msginfo_print("  Address\t(x1,y1) (x2,y2)  [xoff,yoff]  select  next\n");
   for (ptr=framehead; ptr; ptr=ptr->next)
   {
      if (ptr == framehead)
	 msginfo_print("<<");
      msginfo_print("  0x%x--> (%3d,%3d) (%3d,%3d)", ptr, ptr->x1,
	  ptr->y1, ptr->x2, ptr->y2);
      msginfo_print(" [%1.4f,%1.4f]", ptr->xoff, ptr->yoff);
      msginfo_print("\t%s",ptr->select ? "TRUE" : "FALSE");
      if (ptr->next)
         msginfo_print("\t0x%x",ptr->next);
      else
         msginfo_print("\tNIL",ptr->next);
      if (ptr == framehead)
	 msginfo_print(">>\n");
      else
	 msginfo_print("\n");
   }
}

/************************************************************************
*                                                                       *
*  redraw Out of Date images
*  [STATIC Function]							*
*                                                                       */
void
Frame_data::redraw_ood_images()
{
    for (Gframe *ptr = framehead; ptr; ptr=ptr->next){
	if (ptr != framehead){
	    if (ptr->imginfo && ptr->imginfo->display_ood){
		ptr->display_data();
	    }
	}
    }
}

/************************************************************************
*                                                                       *
*  redraw_all_images()
*  [STATIC Function]							*
*                                                                       */
void
Frame_data::redraw_all_images()
{
    for (Gframe *ptr = framehead; ptr; ptr=ptr->next){
	if (ptr != framehead){
	    if (ptr->imginfo){
		ptr->display_data();
	    }
	}
    }
}

/************************************************************************
*                                                                       *
* Constructs a pathname "name" based on the template "tmplate" and
* the filename of the image in this gframe.
* Only the last component of the path (the filename part) is changed.
* The wildcard "*" is replaced with everything in the image's filename
* except the final ".xxxx".  So, if the image's filename is image0001.fdf,
* and the template is "*.tiff", the output name is "image0001.tiff".
* The wildcard "#" is replaced with the first numeric field in the
* image's filename, so "junk#.tiff" becomes "junk0001.tiff".
*
* Returns TRUE if a substitution is made.
* Returns FALSE if the name is the same as the template.
*                                                                       */
int
Gframe::wildcard_sub(char *name, // The constructed name
		     char *tmplate, // The filename template
		     int idx	// Default index number for file
		     )
{
    char base[1024];
    static char *digits = "0123456789";
    char imgname[1024];
    int n;
    char numeric[1024];
    char *pend;
    char *pin;
    char *pout;
    char *fname;

    if (!this->imginfo || (!strchr(tmplate, '*') && !strchr(tmplate, '#')))
    {
	// Cannot expand the template.  The name is the template.
	strcpy(name, tmplate);
	return FALSE;
    }

    // Get the base filename
    fname = this->imginfo->GetFilename();
    if (fname){
	strcpy(imgname, fname);
	if (imgname[strlen(imgname) - 1] == '/'){
	    // Name ends in "/"; clip it off
	    imgname[strlen(imgname) - 1] = '\0';
	}
	if (pin = strrchr(imgname, '/')){
	    pin++;
	}else{
	    pin = imgname;
	}
	if (pend = strrchr(imgname, '.')){
	    n = pend - pin;
	}else{
	    n = strlen(pin);
	}
	strncpy(base, pin, n);
	base[n] = '\0';

	// Get the (first) numeric field in the base filename
	if (pin = strpbrk(base, digits)){
	    n = strspn(pin, digits);
	    strncpy(numeric, pin, n);
	}else{
	    n = 0;
	}
	numeric[n] = '\0';
    }

    //
    // Get defaults ready, if necessary
    //
    if (!fname || !n){
	sprintf(numeric,"%04d", idx);
    }
    if (!fname){
	sprintf(base,"image%04d", idx);
    }

    // Parse the template and construct file name.
    pout = name;
    // Put in the path (if any)
    pin = tmplate;
    if (pend = strrchr(pin, '/')){
	n = pend - pin + 1;
	strncpy(pout, pin, n);	// "/dir1/dir2/"
	pin += n;
	pout += n;
    }
    // Add the filename
    for ( ;*pin; pin++){
	if (*pin == '*'){
	    strcpy(pout, base);
	    pout += strlen(base);
	}else if (*pin == '#'){
	    strcpy(pout, numeric);
	    pout += strlen(numeric);
	}else{
	    *pout++ = *pin;
	}
    }
    *pout = '\0';

    return TRUE;
}

/************************************************************************
*                                                                       *
*  The user wants to save the image in the selected Gframe.             *
*                                                                       *
*  This procedure will save the image in the ddl file format            *
*  [STATIC Function]							*
*                                                                       */
void
Gframe::menu_save_image(char *dirpath,       // directory path name
			char *name)          // filename to be loaded
{
    int i;
    int saveok;
    ofstream outfile;		// output stream     
    char filename[1024];	// complete filename
    char fnametemplate[1024];
    Gframe *gptr;		// Gframe pointer
    
    // Make sure at least one frame is selected.
    if (selecthead == NULL || selecthead->next == NULL ||
	selecthead->next->frameptr == NULL)
    {
	msgerr_print("menu_save: No Frame or Image to save.");         
	return;                                
    }                                         

    // Check if user has specified an output name
    if (*name == NULL)
    {
	msgerr_print("menu_save: Need to specify output filename for saving");
	return;
    }

    // Check if multiple frames selected
    int wildcard = FALSE;
    if (strchr(name, '*') || strchr(name, '#')){
	wildcard = TRUE;
    }
    if (Frame_select::get_selected_frame(2)){
	if (!wildcard){
	    msgerr_print("menu_save: More than one frame is selected.");
	    msgerr_print
	    ("    Need to have a wild card ('*' or '#') in the file name.");
	    return;
	}
    }

    (void)sprintf(fnametemplate,"%s/%s", dirpath, name);
    strcpy(filename, fnametemplate);
    for (gptr=Frame_select::get_selected_frame(i=1); 
	 gptr;
	 gptr=Frame_select::get_selected_frame(++i))
    {
	saveok = TRUE;
	if (gptr->wildcard_sub(filename, fnametemplate, i)){
	    struct stat statbuf;
	    int staterr = stat(filename, &statbuf);
	    if ( !staterr && !confirm_overwrite(filename) ){
		saveok = FALSE;
	    }
	}
	if (saveok){
	    outfile.open(filename, ios::out);         
	    if (outfile.fail()){
		msgerr_print("menu_save_image:Couldn't open \"%s\" for writing",
			     filename);
		return;
	    }
	    outfile.close();

	    if (!gptr || !(gptr->imginfo) || !(gptr->imginfo->st)) {
		msgerr_print("menu_save_image: Nothing to save.");
		return;
	    }else{
		if (!fileformat || fileformat->want_fdf()){
		    //gptr->imginfo->st->SaveSymbolsAndData(filename);
		    gptr->imginfo->ddldata_write(filename);
		}else{
		    // Save data in selected file format
		    fileformat->write_data(gptr, filename);
		}
	    }
	    filelist_update();
	}
    }
    
}

//
// Calc the matrix of frames with given "aspect" ratio (width/height)
// needed to fill a space sized "xtotal" by "ytotal" with at least "n" frames.
// Returns TRUE if everything fits, otherwise returns FALSE and sets
// "rows" and "cols" to the maximum numbers allowed.
//
// STATIC
int
Gframe::frame_matrix(int n, float aspect, int xtotal, int ytotal,
		     int *rows, int *cols)
{
    int nc;			// Number of columns
    int nr;			// Number of rows
    int maxc;			// Max nbr of columns allowed
    int maxr;			// Max nbr of rows allowed
    int x;			// Width of frames
    int y;			// Height of frames
    int nf;			// Number of frames

    maxc = xtotal / min_width();
    maxr = ytotal / min_height();

    // Make the rows just fit
    for (nc=1, nf=0; nf<n && nc<=maxc; nc++){
	x = xtotal / nc;
	y = (int)(x / aspect);
	nr = ytotal / y;
	if (nr > maxr) {nr = maxr;}
	nf = nc * nr;
    }
    if (nf < n){
	*rows = maxr;
	*cols = maxc;
	return FALSE;
    }
    *rows = nr;
    *cols = nc;

    // See if we can do better by making the columns just fit
    for (nr=1; nf<n && nr<=maxr; nr++){
	y = ytotal / nr;
	x = (int)(y * aspect);
	nc = xtotal / x;
	if (nc > maxc) {nc = maxc;}
	nf = nc * nr;
    }
    if (nf >= n && nf < *rows * *cols){
	*rows = nr;
	*cols = nc;
    }

    return TRUE;
}

// Returns min allowed height of Gframes
// STATIC
int
Gframe::min_height()
{
    return margin("top") + margin("bottom") + 3;
}

// Returns min allowed width of Gframes
// STATIC
int
Gframe::min_width()
{
    return margin("left") + margin("right") + 3;
}

//
// Return the width of a Gframe margin.
// The argument is one of "top", "bottom", "left", or "right",
// or just an initial substring of one of these.
//
// STATIC
int
Gframe::margin(char *)
{
    // For now, all margins are the same!
    return m_curve + 1;
}

//
// Return the coordinates of the corners of the rectangle in this Gframe
// that we're allowed to write on.
//
int Gframe::min_x()
{
    return x1 + margin("left");
}
int Gframe::min_y()
{
    return y1 + margin("top");
}
int Gframe::max_x()
{
    return x2 - margin("right");
}
int Gframe::max_y()
{
    return y2 - margin("bottom");
}


// Update the pixel addresses of the corners of the image, based on
// shape of displayed data and shape, size, and position of the gframe.
//
Flag Gframe::update_image_position(Imginfo *iminfo)
{
    if (! iminfo){
	return FALSE;
    }

    if (iminfo->disp_type == SPECTRUM
	|| iminfo->disp_type == SPECTRUM_RAW
	|| iminfo->disp_type == STACK_PLOT
	|| iminfo->disp_type == FILTER_SPECTRUM)
    {
	// Get limits for spectra
	update_spectrum_position(iminfo);
    }else{
	iminfo->update_image_position(min_x(),
				      min_y(),
				      max_x(),
				      max_y());
    }
    return TRUE;
}

// Update the pixel addresses of the corners of the image, based on
// shape of displayed data and shape, size, and position of the gframe.
//
Flag Gframe::update_image_position()
{
    return update_image_position(imginfo);
}

// Update the pixel addresses of the corners of the image, based on
// shape of displayed data; the shape, size, and position of the gframe;
// AND a given range of physical coordinates to cover.
Flag Gframe::update_image_position(Imginfo *iminfo,
				   double xmin, double ymin,
				   double xmax, double ymax)
{
    if (! iminfo){
	return FALSE;
    }

    iminfo->update_image_position(min_x(),
				  min_y(),
				  max_x(),
				  max_y(),
				  xmin, ymin, xmax, ymax);
    return TRUE;
}

Flag
Gframe::update_all_image_positions()
{
    int i;

    if ( ! imginfo ){
	return FALSE;
    }

    int listlen = 2 + overlay_list->Count();
    Imginfo **imglist = (Imginfo **)malloc(listlen * sizeof(Imginfo **));
    if ( ! imglist){
	msgerr_print("Memory allocation error.");
	return FALSE;
    }

    imglist[0] = imginfo;
    ImginfoIterator element(overlay_list);
    Imginfo *overlay;
    for (i=1; element.NotEmpty(); i++){
	overlay = ++element;
	imglist[i] = overlay;
    }
    imglist[i] = 0;

    Flag rtn = update_all_image_positions(imglist,
					  min_x(),
					  min_y(),
					  max_x(),
					  max_y());

    free((char *)imglist);
    return rtn;
}

#define SPC_STANDARD_ORIGIN_X 0.1
#define SPC_STANDARD_USED_WIDTH 0.8

void
Gframe::update_spectrum_position(Imginfo *img)
{
    int x_origin = min_x();
    int width = max_x() - x_origin;
    int y_origin = min_y();
    int height = max_y() - y_origin;

    img->pixstx = (short)(x_origin + SPC_STANDARD_ORIGIN_X * width);
    img->pixwd = (short)(SPC_STANDARD_USED_WIDTH * width);
    img->pixsty = (short)y_origin;
    img->pixht = (short)height;
}

//
// Update the pixel positions of all images, i.e., both the main imginfo
// and any overlays.
// The displayed portions of the various data sets is assumed already set
// by "datastx", etc.
// Fits all the display enabled portions of the data sets into the gframe.
// Normally returns TRUE.  Returns FALSE if there is no imginfo or the position
// information is missing.
//
// FUTURE UPGRADE: If more data would fit in the gframe at the determined
// scale, change datastx, etc. to show as much as fits without changing
// the scale.
//
Flag
Gframe::update_all_image_positions(Imginfo **imglist,
				   int ul_x, int ul_y,
				   int lr_x, int lr_y)
{
    int i;
    const float tol = 0.01;

    // What are we dealing with?
    if (imglist[0]->disp_type == SPECTRUM
	|| imglist[0]->disp_type == SPECTRUM_RAW
	|| imglist[0]->disp_type == STACK_PLOT
	|| imglist[0]->disp_type == FILTER_SPECTRUM)
    {
	// Get limits for spectra
	update_spectrum_position(imglist[0]);
	// Assume dimensions of overlays on spectra will be the same.
	//if (imglist[1] != NULL){
	//    fprintf(stderr,
	//	    "update_all_image_positions(): overlay on a spectrum!\n");
	//}
	return TRUE;
    }

    // Get the physical corners of each image.
    double xx0[3], xx1[3];
    if (!imglist[0]->get_user_coords_of_displayed_data(&xx0[0],
						       &xx0[1],
						       &xx0[2],
						       &xx1[0],
						       &xx1[1],
						       &xx1[2]))
    {
	return FALSE;
    }
    double zavg = (xx0[2] + xx1[2]) / 2;	// Z-coord of middle of slab
    double thk = fabs(xx0[2] - xx1[2]);	// Thickness of slab

    double xmin, ymin, xmax, ymax;
    if (xx0[0] < xx1[0]){
	xmin = xx0[0]; xmax = xx1[0];
    }else{
	xmin = xx1[0]; xmax = xx0[0];
    }
    if (xx0[1] < xx1[1]){
	ymin = xx0[1]; ymax = xx1[1];
    }else{
	ymin = xx1[1]; ymax = xx0[1];
    }

    double xxx0[3], xxx1[3];	// Upper-left corner and bottom-right corner
    double xxx2[3];		// Upper-right corner
    double orientation[9];
    if ( ! imglist[0]->GetOrientation(orientation) ){
	msgerr_print("No orientation information for base data");
	return FALSE;
    }
    double thkavg;
    double xx_width, xx_height;
    Imginfo *overlay;
    for (i=1; imglist[i]; i++){
	overlay = imglist[i];
	if(!overlay->get_user_coords_of_displayed_data(&xxx0[0],
						       &xxx0[1],
						       &xxx0[2],
						       &xxx1[0],
						       &xxx1[1],
						       &xxx1[2]))
	{
	    return FALSE;
	}
	xxx2[0] = xxx1[0];			// X-coord of upper-right corner
	xxx2[1] = xxx0[1];			// Y-coord of upper-right corner
	xxx2[2] = (xxx0[2] + xxx1[2]) / 2;	// Z-coord of middle of slab
	thkavg = (thk + fabs(xxx0[2] - xxx1[2])) / 2;	// Avg of this and main
	xxx0[2] = xxx1[2] = xxx2[2];	// Now just look at middle of slab

	xx_width = fabs(xxx0[0] - xxx1[0]);
	xx_height = fabs(xxx0[1] - xxx1[1]);

	// Convert coords of overlay to system of main imginfo
	if (   ! overlay->rotate_coords(orientation, xxx0)
	    || ! overlay->rotate_coords(orientation, xxx1)
	    || ! overlay->rotate_coords(orientation, xxx2) )
	{
	    return FALSE;
	}

	// Check that the overlay is in the same plane as the main image.
	// Look at the transformed z-coord of each corner and see if the
	// data slabs overlap.
	if (   fabs(xxx0[2] - zavg) > thkavg
	    || fabs(xxx1[2] - zavg) > thkavg
	    || fabs(xxx2[2] - zavg) > thkavg)
	{
	    msgerr_print("Warning: overlay data in different plane.");
	}

	// Check that overlay data is not rotated wrt. main image.
	if (   fabs(xxx0[1] - xxx2[1]) > tol * xx_height
	    || fabs(xxx1[0] - xxx2[0]) > tol * xx_width)
	{
	    msgerr_print("Overlay data is rotated w.r.t. base data.");
	    return FALSE;
	}

	// Check that overlay data is not flipped wrt. main image.
	Flag err = FALSE;
	if ( (xx0[0] - xx1[0]) * (xxx0[0] - xxx1[0]) < 0){
	    err = TRUE;
	    msgerr_print("Overlay data is flipped left-to-right.");
	}
	if ( (xx0[1] - xx1[1]) * (xxx0[1] - xxx1[1]) < 0){
	    err = TRUE;
	    msgerr_print("Overlay data is flipped top-to-bottom.");
	}
	if (err){
	    return FALSE;
	}

	// Update xmin, etc.
	if (xxx0[0] < xmin) xmin = xxx0[0];
	if (xxx0[0] > xmax) xmax = xxx0[0];
	if (xxx1[0] < xmin) xmin = xxx1[0];
	if (xxx1[0] > xmax) xmax = xxx1[0];

	if (xxx0[1] < ymin) ymin = xxx0[1];
	if (xxx0[1] > ymax) ymax = xxx0[1];
	if (xxx1[1] < ymin) ymin = xxx1[1];
	if (xxx1[1] > ymax) ymax = xxx1[1];
    }

    // Swap min and max values if data was originally reversed.
    if (xx0[0] > xx1[0]){
	double tmp = xmin;
	xmin = xmax;
	xmax = tmp;
    }
    if (xx0[1] > xx1[1]){
	double tmp = ymin;
	ymin = ymax;
	ymax = tmp;
    }

    // Update "pixstx", etc. in all the imginfos
    if ( ! imglist[0]->update_image_position(ul_x, ul_y,
					     lr_x, lr_y,
					     xmin, ymin, xmax, ymax))
    {
	msgerr_print("Can't determine image positions.");
	return FALSE;
    }

    for (i=1; imglist[i]; i++){
	overlay = imglist[i];
	// Rotate coords back to this frame
	overlay->GetOrientation(orientation);
	xxx0[0] = xmin; xxx0[1] = ymin; xxx0[2] = 0;
	xxx1[0] = xmax; xxx1[1] = ymax; xxx1[2] = 0;
	imglist[0]->rotate_coords(orientation, xxx0);
	imglist[0]->rotate_coords(orientation, xxx1);
	if ( ! overlay->update_image_position(ul_x, ul_y,
					      lr_x, lr_y,
					      xxx0[0], xxx0[1],
					      xxx1[0], xxx1[1]))
	{
	    msgerr_print("Can't determine image positions.");
	    return FALSE;
	}
    }
    return TRUE;
}

/************************************************************************
*									*
*  This routine is called after the user has completed the definition   *
*  of an object.  It will append the new object to the object list      *
*									*/
Flag
Gframe::AppendOverlay(Imginfo* ovrlay)
{
    Imginfo *tmpimginfo;
    if (!ovrlay) return FALSE;
    
    // Now the object must be appended to the object list
    overlay_list->Append(ovrlay);
    // fake an attachment in order to increment the ref_count for the
    // previous append operation.
    attach_imginfo(tmpimginfo, ovrlay);
    tmpimginfo = NULL;
    
    return(TRUE);
}

/************************************************************************
*									*
*  This routine is called to delete an imginfo from the       		*
*  overlay_list in a Gframe.          					*
*   				 					*/
void
Gframe::RemoveAllOverlays(void)
{
    if (overlay_list->Count() > 0)
    {
	Imginfo *tmpimginfo;
	while (tmpimginfo = overlay_list->Pop()){
    	     detach_imginfo(tmpimginfo);

	}
    }
}
