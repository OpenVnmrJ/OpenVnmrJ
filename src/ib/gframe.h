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

#ifndef _GFRAME_H
#define _GFRAME_H
/************************************************************************
*
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA  94538
*
*************************************************************************/

#include "params.h"

// Properties menu
typedef enum
{
   F_MENU_SPLIT,
   F_MENU_DELETE_SELECT,
   F_MENU_DELETE_UNSELECT,
   F_MENU_DELETE_EMPTY,
   F_MENU_DELETE_ALL,
   F_MENU_CLEAR_SELECT,
   F_MENU_CLEAR_UNSELECT,
   F_MENU_CLEAR_ALL,
   F_MENU_SELECT_ALL,
   F_MENU_LOAD,
   F_MENU_SAVE,
   F_MENU_TITLE,
   F_MENU_FULL,
   F_MENU_UNFULL
} F_props_menu;


// Action type, depending on the mouse buttons (used in Gframe)
typedef enum
{
   FRAME_NO_ACTION,     // No action
   FRAME_CREATE,        // Create a frame
   FRAME_CREATE_DONE,
   FRAME_SELECT,        // Select a frame
   FRAME_SELECT_DONE,
   FRAME_TOGGLE,	// Toggle a frame (select or deselect)
   FRAME_TOGGLE_DONE,
   FRAME_RESIZE,	// resize a frame
   FRAME_RESIZE_DONE,
   FRAME_MOVE,		// move a frame
   FRAME_MOVE_DONE,
   FRAME_COPY,          // Copy a frame
   FRAME_COPY_DONE
} Faction_type;

// Types of clip regions that may be set
typedef enum{
    FRAME_NO_CLIP,
    FRAME_CLIP_TO_IMAGE,
    FRAME_CLIP_TO_FRAME
} ClipStyle;

// Indicate to remove one item or all items (used in Frame_select)
typedef enum
{
   REMOVE_SELECT_ONE_ITEM,
   REMOVE_SELECT_ALL_ITEM
} Remtype;


// Forward class declarations
class Frame_data;
class Imginfo;
class ImginfoList;

// Graphics frame object
class Gframe
{
   private:
      friend class Frame_routine; 
      friend class Frame_data; 

      // Temporary variables used by the member functions in this class.
      // Variables 'basex' and 'basey' are served as initialized point
      // (base point for moving/creating/resizing a frame).
      static short basex, basey;

      static int color;		// frame color
      static Flag title;	// Indicate need to display title
      static int num_frame;	// number of frames (doesn't include 
				// "working-buffer" frame)

      static Faction_type acttype; // State of graphics event such as
				   // create/move/resize/copy/select

      Gframe *next;		// next item on the list
      short x1,y1,x2,y2;	// position of top-left/bottom-right corner
				//  of the frame
      Flag select;		// indicate a frame is selected or not
      Flag clean;		// indicate a frame contains something or 
				//  NOT (used by function "clear")
      
      float xoff, yoff;		// x,y offset (for displaying spectrum)
				// currently not used and reserved for future

      // Width of margin: "top", "bottom", "left", or "right"
      static int margin(char *sidename);

      // Return min allowed dimensions of Gframes
      static int min_height();
      static int min_width();

      // Calc the matrix of frames with given aspect ratio needed to fill
      // the given space with at least N frames.
      static int frame_matrix(int n, float aspect, int width, int height,
			      int *rows, int *cols);
    
      // Update the frame size.  However, it won't let the frame overlapped
      void resize(short x, short y);	// cursor position

      // Update frame position
      void update_position(void);

      // Draw a frame
      void draw(int line_width);

      // Insert a new item into the list
      Gframe *insert(void);	// At beginning of list
      void append();		// At end of list

      // Insert a new item into the list after current gframe
      Gframe *insert_after(void);

      // Return TRUE if the frame overlaps with others, else FALSE.
      Flag overlap_frame(short m1, short n1,	// top-left corner point
		short m2, short n2,		// bottom-right corner point
		Flag self_check);		// check include itself
      Flag overlap_frame(Flag self_check)
      {
	 return(overlap_frame(x1, y1, x2, y2, self_check));
      }
      Flag overlap_moved_frame(int refx, int refy);
      Flag overlap_resized_frame(int refx, int refy);
      void constrain(Flag(Gframe::*func)(int x, int y),
			   int xok, int yok, short *x, short *y);

      // Return TRUE if a cursor position is close enough to the frame
      // corner, else FALSE
      Flag corner_selected(short x, short y);

      // Expand a template into a file name
      int wildcard_sub(char *, char *, int index=0);

      void display_init();
      void display_end();

   public:
      static Gdev *gdev;	 // graphics device handler
      Imginfo *imginfo;		 // Image/Spectrum-display information
      ImginfoList *overlay_list; // List of overlays on a gframe
      ParamSet params;           // SISCO parameters for the image/spectra

      static int numFrames();	// Returns number of Gframes

      // These routine related to save/load graphics frames position
      // to/from a file
      static void menu_handler(Menu, Menu_item);
      static Menu menu_load_pullright(Menu_item, Menu_generate op);
      static void menu_load(char *, char *);
      static int Load(int, char **, int, char **);
      static void menu_save(char *, char *);
      static int Save(int, char **, int, char **);
      static void menu_save_image(char *, char *);

      // Set the frame colors
      static void frame_set_color(Menu, Menu_item);

      static int set_attr(int, char *);	// set attribute routine

      Gframe(void);
      ~Gframe(void);

      // Perform all the necessary events to create/resize/move/etc a frame
      void action(short x, short y);	// cursor position

      // Return TRUE or FALSE to see if the cursor is inside any frame
      // If frame is selected, a selected frame pointer is assigned in
      // a varible 'frameptr' of Frame_select first item
      Flag is_selected(
		int xview_act,		// ACTION_SELECT or ACTION_ADJUST
		short x, short y);	// cursor position

      // Find out what frame contains a specific point.
      Gframe *is_at(short x, short y);

      // Find out if this is a selected frame
      Flag is_a_selected_frame();

      // Move a frame (to a new location)
      void move(short x, short y,	// current cursor position
		Flag frame_overlap);	// TRUE for non overlapped

      // Mark or Unmark the frame at 4 corners
      void mark(void);

      // Remove frame(s) from the list
      void remove(F_props_menu);
      static int Delete(int, char **, int, char **);

      // Clear image(s) from frames in the list
      void clear_frame(F_props_menu);
      static int Clear(int, char **, int, char **);

      // Display (refresh) the image contained in this frame
      void display_data();

      // Set the value of item 'select'
      void set_select(Flag flag) { select = flag; }

      // Split a frame into multiple frames
      void split(int row, int col);

      // Select all the frames
      void select_all(void);
      // Select frame(s)
      static int Select(int, char **, int, char **);
      void select_frame();

      // Return a frame from a specified screen position
      static Gframe *get_gframe_with_pos(short x, short y, int *frameno=NULL);

      // Remove image from frame
      void remove_image();

      // Erase the frame region
      void clear(void);

      // Set a new display item inform frame it has data displayed
      void newdisplayitem(void) { clean = FALSE; }

      // Get image inside a frame
      XImage *get_image(short pixx, short pixy, short pixwd, short pixht);

      // Set the clip region for a frame
      void set_clip_region(ClipStyle style=FRAME_CLIP_TO_IMAGE);

      /* For debugging */
      void debug(...);

      // Check to see if a frame exists
      int exists(const Gframe *);

      static Gframe *get_first_frame();
      static Gframe *get_next_frame(Gframe*);
      static Gframe *get_frame_by_number(int);
      int get_frame_number();

      short get_top_left_x();
      short get_top_left_y();
      short get_bottom_right_x();
      short get_bottom_right_y();

      int min_x();		// Area of frame that we can write on.
      int min_y();
      int max_x();
      int max_y();

      static Gframe *big_gframe(void);
      static void bye_big_gframe(Gframe *);

      void debug_frame(void);

      // See how data fits in frame; update Imginfo items
      Flag update_image_position(Imginfo *);
      Flag update_image_position();
      Flag update_image_position(Imginfo *, double xmin, double ymin,
				 double xmax, double ymax);

      // See how to get all displayed data into frame--main imginfo & overlays
      // Update Imginfo items: pixstx, ...
      Flag update_all_image_positions();
      Flag update_all_image_positions(Imginfo **, int, int, int, int);
      void update_spectrum_position(Imginfo *);

      // Appending/Removing items from the overlay list
      Flag AppendOverlay(Imginfo* imginfo);
      void RemoveAllOverlays(void);

};

// This class keeps track of all selected frames in Gframe list
// Note that its pointers point to the item in Gframe list
// The purpose of this class is to arrange the order of the frames which
// the user has selected.
class Frame_select
{
   private:
      friend class Gframe;	// Let Gframe access to these members
      friend class Frame_data;	// Let Frame_data access to these members
      friend class Frame_routine;  // Ditto for Frame_Routine

      int numnth;		// frame number
      Gframe *frameptr;		// point to the selected Gframe
      Frame_select *next;	// next item

      // Insert a selected frame into selected frame list
      void insert(void);

   public:
      Frame_select(Gframe *frameptr, int numnth);
      ~Frame_select(void);

      // Deselect all selected frames
      void deselect(void);
      void deselect(Gframe *);

      // Remove selected frame(s)
      void remove(Remtype);
      void remove(Gframe *);

      // Split each selected frame into multiple frames
      static void split(int row, int col);
      static int Split(int argc, char **argv, int retc, char **retv);

      // Return a selected frame from the nth list
      static Gframe *get_selected_frame(int nth);

      // Run through the list of selected frames
      static Gframe *get_first_selected_frame();
      static Gframe *get_next_selected_frame(Gframe *); // Not efficient

      // Insert a selected frame into selected frame list
      void insert(Gframe *gf)
      {
	 frameptr = gf;
	 insert();
      }
};

class DDLSymbolTable;

// Routines to manipulate sisdata in file "frame_data.c"
class Frame_data
{
private:
  static void display_init(Gframe *);
  static void display_end(Gframe *);

public:
  static int load_ddl_data(
		       Gframe *, 		// frame to be loaded
		       char *path,		// directory path name
		       char *name,		// filename to be loaded
		       int *display_data, // IN/OUT: display control flag
		       int load_silently = FALSE,
		       DDLSymbolTable *st=NULL,
		       int math_result = FALSE); // Flag output from math func
  static int load_data_oldformat(
		       Gframe *, 		// frame to be loaded
		       char *path,		// directory path name
		       char *name,
		       int display_data = TRUE, // display control flag
		       int load_silently = FALSE); // filename to be loaded
  static int load_params(
			 Gframe *gframe,	// frame to be loaded
			 char *path,		// directory path name
			 char *name,		// filename to be loaded
			 int silent);           // don't report errors
			     
  static void display_data(Gframe *gframe,	// frame containing data
			   int src_stx,
			   int src_sty,    	// data source starting point
			   int src_wd,
			   int src_ht,       	// data source width and height
			   float vs,		// vertical scale
			   int init = TRUE);    // init and deinit canvas? 
			     
  static void display_data(Gframe *gframe,	// frame containing data
			   Imginfo *imginfo,
			   int src_stx,
			   int src_sty,    	// data source starting point
			   int src_wd,
			   int src_ht,       	// data source width and height
			   float vs,		// vertical scale
			   int init = TRUE);    // init and deinit canvas?

  static int SetHdr(int argc, char **argv, int retc, char **retv);
  static void set_hdr_parm(char *name, // Run external prog to get parm values
			   char *user_prog);

  static void redraw_all_images();              // refreshes all images

  static void redraw_ood_images();		// Refreshes Out Of Date images
};

#endif _GFRAME_H
