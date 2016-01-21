/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _ROITOOL_H
#define _ROITOOL_H
/************************************************************************
*									
*
*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/
#include <iostream>
#include <fstream>
// #include <stream.h>
#include "stringedit.h"
#include "macrolist_ib.h"
/* #include "gtools.h" */

// Mark size for marking ROI
#define	MARK_SIZE	3

// State used by Roitool to indicate that ROI is on the graphics, ROI
// marked, ROI is being created, or ROI existence.  Note that not all
// ROI tools use these bits
// It is accessed through bit by variable 'state' in class Roitool
#define ROI_STATE_MARK 		1
#define ROI_STATE_EXIST 	2
#define	ROI_STATE_CREATE	4

// ROI properties menu
typedef enum {
  ROI_MENU_DELETE,
  ROI_MENU_UNDELETE,
  ROI_MENU_MARK,
  ROI_MENU_LOAD,
  ROI_MENU_SAVE,
  ROI_MENU_APERTURE,
  ROI_MENU_TRACKING,
  ROI_MENU_BIND
} Roi_props_menu;

// ROI event action
typedef enum {
  ROI_NO_ACTION,
  ROI_CREATE,
  ROI_CREATE_DONE,
  ROI_RESIZE,
  ROI_RESIZE_DONE,
  ROI_MOVE,
  ROI_MOVE_DONE,
  ROI_ROTATE,			// only apply to Oval
  ROI_ROTATE_DONE		// only apply to Oval
} Raction_type;

typedef enum {
  VISIBLE_ALWAYS,
  VISIBLE_WHEN_MOVED,
  VISIBLE_NEVER
} Visibility;

// The following typedef contains possible return values for an action
// that is invoked by an ROI object

typedef enum {
  REACTION_NONE,
  REACTION_CREATE_OBJECT,
  REACTION_DELETE_OBJECT
} ReactionType;

// Indicate the drawing mode for the selected object:
typedef enum {
    ROI_XOR,
    ROI_COPY,
    ROI_NOREFRESH
} RoiDrawMode;

// Graphics drawing mode and color
struct Gmode {
    int color;
    int op;
};

class Stats;

// Base tool class.  This class is a base class for ROI routines
class Roitool {
    
  protected:
    
friend class Roi_routine;
    
    // To keep from drawing ROIs outside of image data
    void keep_point_in_image(short *x, short *y);

    // The type of this object at creation time
    Roitype created_type;
    
    // Determines the visibity of the ROI
    Visibility visibility;
    
    // Temporary variables used by the member functions in this class.
    // Variables 'basex' and 'basey' are served as initialized point
    // (base point for moving/creating/resizing)
    short basex;
    short basey;

    static Raction_type force_acttype; // Force selection for move, rotate
    static Gdev *gdev;       	// Graphics device
    static int aperture;	// Sensitivity value to resize a corner
    static int max_active_tools; // # of tools to adjust in real time
    static int copy_color;     	// Current default for drawing in "copy" mode
    static int active_color;	// Color used while moving ROI
    static int xor_color;	// Value used for XOR'ing stuff
    static int color;		// Current color value to use.
    static int bind;		// Bind ROI modifications
    int my_color;		// Normal (copy) color for this ROI
    int npnts;                	// Number of points
    Gpoint *pnt;              	// Pointer to a list of points
    short state;		// State of ROI
    static Gframe *curr_frame;	// Remembers frame while creating ROI
    static Gpoint sel_down;	// Remembers where button went down
    static Pixmap bkg_pixmap_id; // Backup store of stuff "behind" ROIs
    static int bkg_width;	// Dimensions of entire bkg pixmap
    static int bkg_height;
    short bkg_pixmap;		// True if bkg for this ROI is stored in pixmap
    //short pmstx, pmsty;	// Location of ul corner of pixmap on canvas
    //short pmendx, pmendy;	// Location of lr corner of pixmap on canvas
    
    short x_min, y_min;		// Upper left corner of ROI bounding rectangle 
    short x_max, y_max;		// Lower right corner of ROI bounding rectangle
    unsigned short resizable : 1;
    unsigned short visible  : 1;
    unsigned short markable : 1;
    
  public:
    Roitool();
    virtual ~Roitool();
    Gframe *owner_frame;	// Frame containing ROI
    
    static Roitool *get_selected_tool();
    static int frame_has_a_selected_tool(Gframe *);
    Roitype GetType(){ return created_type;};	// What type of ROI this is
    Flag redisplay_bkg(int, int, int, int);	// Refresh the background image
    Flag save_bkg();			// Create pixmap of ROI's background
    void forget_bkg();			// Mark stored background invalid
    static Flag allocate_bkg();		// Get pixmap to store ROI backgrounds
    static void release_bkg();		// Release pixmap of ROI backgrounds
    static XID set_drawable(XID);	// Set the default drawable for display
    
    // Set attribute (registered to be a call-back function)
    // Used to set color value and aperture (sensitivity)
    static int set_attr(int id, char *);
    static void roi_menu_color(Menu, Menu_item);

    // Initialize points. It is enough to initialize x of the first point
    void init_point(void)
    { if (pnt) pnt[0].x = G_INIT_POS; basex = G_INIT_POS; }
    
    // Routine related to state
    short roi_state(short s)      { return (state & (s)); }
    void roi_clear_state(short s) { state &= ~(s); }
    void roi_set_state(short s)   { state |= (s); }
    
    // Action of graphics event such as create/move/resize
    static Raction_type acttype;
    
    // To create/move/resize 
    ReactionType action(short x, short y, short action = 0);
    // Create/move/resize all active ROIs
    ReactionType active_action(short x, short y, short action = 0);

    // Make every tool in the active_tools list like the first one
    static void clone_first_active();

    static int position_in_active_list(Roitool *);
    
    // Text handler
    virtual int handle_text(char c) {return printf("%c", c);}
    
    // Set drawing mode (and corresponding color)
    Gmode setxor();
    Gmode setcopy();
    Gmode setactive();
    Gmode setGmode(Gmode);
    
    // Basic ROI functions
    virtual char *name(void) = 0;
    virtual ReactionType create(short, short, short) = 0;
    virtual ReactionType create_done(short, short, short) = 0;
    virtual ReactionType resize(short, short) = 0;
    virtual ReactionType resize_done(short, short) = 0;
    virtual ReactionType move(short, short) = 0;
    virtual ReactionType move_done(short, short) = 0;
    virtual Roitool *copy(Gframe *);
    virtual void draw(void);
    virtual void erase(void);
    virtual void mark();
    void refresh(RoiDrawMode copymode=ROI_XOR);
    virtual void select(RoiDrawMode copymode=ROI_XOR, int appendflag=FALSE);
    virtual void deselect();
    virtual Flag is_selected(short, short) = 0;	// Is cursor in ROI?
    virtual Flag is_selectable(short x, short y){
	return is_selected(x, y);} // "is_selectable()" does not modify the ROI
    Flag is_selected();				// Is ROI in selection list?
    void draw_mark(int x, int y);		// Draw ROI marker at (x, y)
    void update_xyminmax(int x, int y);	// Update x_min, y_min, x_max, y_max
    void calc_xyminmax();		// Calculate x_min, etc. from pnt[]
    
    // FocusIn tells the roitool that it should be the focus of input and it
    // should take appropriate actions (such as highlighting or other init)
    virtual int Focus_In() { return FALSE ; }
    // FocusOut tells the roitool that it is no longer the input focus
    // and should take appropriate action (especially in undoing the effects
    // of FocusIn()).  Both FocusIn and FocusOut return TRUE if they care
    // about being the input focus, FALSE otherwise.
    virtual int Focus_Out() { return FALSE ; }
    
    // Recreate creates a new instance of this roitool
    // It allows subclasses to create new instances of the proper subclass
    // without the code having to know which subclass is being created.
    virtual Roitool* recreate() {return this;}
    
    // Only apply to Oval
    virtual ReactionType rotate(short, short)
    {printf("Can't rotate this object\n"); return REACTION_NONE; }
    virtual ReactionType rotate_done(short, short)	// Only apply to Oval
    {return REACTION_NONE;}

    // ROI I/O functions
    virtual void save(std::ofstream &) = 0;
    virtual void load(std::ifstream &) = 0;
    static void load_roi(std::ifstream &);
    
    // Only apply to Polygon
    virtual ReactionType mouse_middle(void){ return REACTION_NONE; }
    virtual ReactionType mouse_right(void) { return REACTION_NONE; }
    
    // For Histogram statistics
    void get_minmax(double *min, double *max);
    int histostats(int nbins, double min, double max, Stats *stats);
    virtual void histostats(int *hist, int nbins, double *min, double *max,
		    double *median, double *mean, double *sdv,
		    double *area);
    virtual void some_info(int ifmoving=FALSE);  // look in point.c and line.c
    
    // Used for image segmentation
    void zero_out(double min, double max);
    static void segment_selected_rois(Gframe *gptr,
				      int min_defined,
				      double min,
				      int max_defined,
				      double max);

    //Used to run through all pixels in ROI
    virtual float *FirstPixel();
    virtual float *NextPixel();

    // Functions to convert from pixel to data coords and back
    float xpix_to_data(int xp);
    float ypix_to_data(int yp);
    int data_to_xpix(float x);
    int data_to_ypix(float y);

    // Used after ROI creation/mod
    virtual void update_data_coords();

    // Used after image rotation/reflection
    virtual void rot90_data_coords(int datawidth);
    virtual void flip_data_coords(int datawidth);

    // Used after window move/resize/zoom
    virtual void update_screen_coords();

    // Clips ROI move parameters to keep ROI on image
    void keep_roi_in_image(short *x_motion, short *y_motion);

    // Sets the display clip region
    void set_clip_region(ClipStyle style);

    // Redraw the ROI in copy mode to make it solid
    void draw_solid();

    // Returns TRUE is specified ROI is "similar" to this one
    // in type, shape, and location.
    Flag matches(Roitool *);
};

Declare_ListClass(Roitool);

class RoitoolIterator {
public:
  RoitoolIterator(RoitoolList* r) : rtl(r) {
    if (rtl)
      next = rtl->First();
    else
      next = 0;
  }
  
  ~RoitoolIterator() {};
  
  int NotEmpty() { return (next ? TRUE : FALSE) ; }
  int  IsEmpty() { return (next ? FALSE : TRUE) ; }
  
  RoitoolIterator& GotoLast()  {
    if (rtl)
      next = rtl->Last();
    return *this;
  }
  
  RoitoolIterator& GotoFirst() {
    if (rtl)
      next = rtl->First();
    return *this;
  }
  
  Roitool *operator++() {
    RoitoolLink* curr;
    curr = next;
    if (next)
      next = next->Next();
    if (curr)
      return curr->Item();
    else
      return 0;
  }
  
  Roitool *operator--() {
    RoitoolLink* curr;
    curr = next;
    if (next)
      next = next->Prev();
    if (curr)
      return curr->Item();
    else
      return 0;
  }
  
private:
  
  RoitoolLink* next;
  RoitoolList* rtl;
  
};

// Linked-list to store a number of points
class Lpoint
{
  public:
    short x, y;			// point
    Lpoint(short xval, short yval){x=xval; y=yval;}
    ~Lpoint(){}
};

Declare_ListClass(Lpoint);

class LpointIterator {
public:
  LpointIterator(LpointList* r) : rtl(r) {
    if (rtl)
      next = rtl->First();
    else
      next = 0;
  }
  
  ~LpointIterator() {};
  
  int NotEmpty() { return (next ? TRUE : FALSE) ; }
  int  IsEmpty() { return (next ? FALSE : TRUE) ; }
  
  LpointIterator& GotoLast()  {
    if (rtl)
      next = rtl->Last();
    return *this;
  }
  
  LpointIterator& GotoFirst() {
    if (rtl)
      next = rtl->First();
    return *this;
  }
  
  Lpoint *operator++() {
    LpointLink* curr;
    curr = next;
    if (next)
      next = next->Next();
    if (curr)
      return curr->Item();
    else
      return 0;
  }
  
  Lpoint *operator--() {
    LpointLink* curr;
    curr = next;
    if (next)
      next = next->Prev();
    if (curr)
      return curr->Item();
    else
      return 0;
  }
  
private:
  
  LpointLink* next;
  LpointList* rtl;
  
};

// A location in the real plane
struct Fpoint
{
    float x;
    float y;
};

// A location in 3D space
struct D3Dpoint{
    double x, y, z;
};

#endif
