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

#ifndef _GTOOLS_H
#define _GTOOLS_H
/************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/

#include "graphics.h"
#include "gframe.h"

#ifndef _CSI_GTOOLS_H

// Graphics-tool type.  It should start with zero
typedef enum
{
  GTOOL_SELECTOR = 0,
  GTOOL_FRAME,
  GTOOL_ZOOM,
  GTOOL_VS,
  GTOOL_LINE,
  GTOOL_POINT,
  GTOOL_RECT,
//  GTOOL_OVAL,
  GTOOL_PGON,
  GTOOL_PGON_OPEN,
  GTOOL_TEXT,
  GTOOL_MATH,
  NUM_GTOOL			// This is used for indicating number of TOOLS.
				// It must be the last one.
} Gtype;

#endif

typedef enum
{
    GTOOL_REFRESH,
    GTOOL_NOREFRESH
} GRefreshFlag;

// This enum is used by Gtools_routine (for Frame)
typedef enum
{
  ROUTINE_DONE,
  ROUTINE_FRAME_SELECT,
  ROUTINE_FRAME_TOGGLE
} Routine_type;

// ROI shape (or type)
/*#define ROI_NUM 5*/
typedef enum
{
  ROI_SELECTOR = 0,
  ROI_LINE,
  ROI_POINT,
  ROI_BOX,
  ROI_OVAL,
  ROI_POLYGON,
  ROI_POLYGON_OPEN,
  ROI_TEXT,
  ROI_MATH,
  MARKER_VLINE,
  MARKER_CHAIR,
  MARKER_CIRCLE,
  MARKER_GRID,
  ROI_NUM
} Roitype;

// Base class for initialization (polymorphism).
// Its routines serves as a an interface between events and drawings
class Gtools_routine
{
   protected:
      Menu props_menu;
      Menu color_menu;

   public:
      virtual void start(Panel, Gtype) = 0;
      virtual Routine_type process(Event *) = 0;
      virtual void end(void) = 0;
      virtual void frame_task(Event *, Routine_type) {}
      virtual void redraw(void) {}
      void show_props_menu();
};

// Graphcis Frame routines
class Frame_routine : public Gtools_routine
{
   private:
      static Flag active;	// indicate this tool is active or not
      Frame split_frame;
      Frame split_popup;
      Panel_item split_rows_widget;
      Panel_item split_cols_widget;
      static int memPctThreshold;
      static void split_callback(Panel_item, int, Event *);

   public:
      Frame_routine(Gdev *);
      void start(Panel, Gtype);
      Routine_type process(Event *);
      void end(void);
      void frame_task(Event *, Routine_type);
      static void redraw_frame(void);
      static Flag frame_active() { return (active); }
      void show_splitter_popup();

      // 
      static int load_data(char * dir, char *file);
      static int load_data_file(char * dir, char *file);
      static int Load(int argc, char **argv, int retc, char **retv);
      static int load_data_all(char * dir, char *file);
      static int LoadAll(int argc, char **argv, int retc, char **retv);
      static void save_data(char *, char *);
      static int Save(int argc, char **argv, int retc, char **retv);
      static int MemThreshold(int argc, char **argv, int retc, char **retv);
      static void FindNextFreeFrame();
};

// Defined in roitool.h. It is used as reference for Roi_routine
class Roitool;

// Roi routines
class Roi_routine : public Gtools_routine
{
   private:
      static Flag active;	// indicate this tool is active or not
      static Gtype gtooltype;	// Gtool to revert to because of Shift down

      static Roitype gtoolTypeToRoitoolType(Gtype);
      static Gtype toolToGtoolType(Roitool *);
      static Roitype toolToRoitoolType(Roitool *);
      static void fontsize_menu_handler(Menu, Menu_item);
      static void menu_handler(Menu, Menu_item);
      static Menu roi_menu_load_pullright(Menu_item, Menu_generate op);
      static Menu_item bind_menu_item;
      static void roi_menu_load(char *, char *);
      static void roi_menu_save(char *, char *);
      static void set_bind(int);
      void finish_tool_creation(short, short, short);
      Roitool* create_roitool(Gframe *);

   public:
      static int Load(int argc, char **argv, int retc, char **retv);
      static int Save(int argc, char **argv, int retc, char **retv);
      static int Bind(int argc, char **argv, int retc, char **retv);
      Roi_routine(Gdev *);
      void start(Panel, Gtype);
      Routine_type process(Event *);
      static int AppendObject(class Roitool*, Gframe *);
      static int DeleteObject(class Roitool*,
			      GRefreshFlag refresh_flag=GTOOL_REFRESH);
      static void DeleteSelectedTools(GRefreshFlag refresh_flag=GTOOL_REFRESH,
				      Flag confirm=TRUE);
      static int Delete(int argc, char **argv, int retc, char **retv);
      static void SelectToolsInBox(Gpoint *corners);
      static int UnDeleteObject();
      static void SetRoiType(Roitype, Gtype);
      void end(void);
      static Flag roi_active(void) { return(active); }

      // Return active ROI tool
      static Roitool *get_roi_tool(void);

      // These routines are typically used to erase, perform necessary
      // tasks and redraw ROI back.  Note that after calling roi_erase,
      // you should call roi_draw.  They are called by functions outside
      // Roitool or static function in Roitool
      static void roi_erase(void);
      static void roi_draw(void);

      // Return the number nth of the frame list where its ROI
      // is located within its image boundary.  Return value of 0
      // indicates no ROI lies within frame's image boundary
      static int get_nth_roi_frame(void);
};

// Zoom routines
class Zoom_routine: public Gtools_routine
{
   private:
      static void menu_handler(Menu, Menu_item);
      static Flag active;	// indicate this tool is active or not
   
   public:
      Zoom_routine(Gdev *);
      void start(Panel, Gtype);
      Routine_type process(Event *);
      void end(void);
      static Flag zoom_active(void) { return(active); }
};

// Vs routines
class Vs_routine: public Gtools_routine
{
   private:
      static void menu_handler(Menu, Menu_item);
      static Flag active;	// indicate this tool is active or not
   
   public:
      Vs_routine(Gdev *);
      void start(Panel, Gtype);
      Routine_type process(Event *);
      void end(void);
      static Flag vs_active(void) { return(active); }
};

// Math routines
class Math_routine: public Gtools_routine
{
   private:
      static void menu_handler(Menu, Menu_item);
      static Flag active;	// indicate this tool is active or not
   
   public:
      Math_routine(Gdev *);
      void start(Panel, Gtype);
      Routine_type process(Event *);
      void end(void);
      static Flag math_active(void) { return(active); }
};

extern Gtools_routine *func[NUM_GTOOL+1];

// ----- PRIVATE to gtools.c -----
// Structures for gtool window handlers.  It has only 1 instance, so
// its members are all declared as static.
extern Gtype gtype;

class Gtools
{
private:
  static Frame popup;       // Pop-up window
  static Menu menu;         // Menu for initialization
  static Panel panel;       // Panel control area
  static Panel_item tools;  // Panel choice (of graphics tools)
  static Panel_item props;  // Panel buton menu (shared by all gtools)
  static Gdev *gdev;        // This is a handler for drawing routine
  static Cms control_cms;
  
  
public:
  // Creator
  static void create(Frame, Gdev *);

  // Get the gtools popup frame
  static Frame get_gtools_frame(){
      return popup;
  }

  // Change the "Properites"button label
  static void set_props_label(char *);
  
  // Change the type of a tool
  static void select_tool(u_long i, int tool_type);
  static int Tool(int argc, char **argv, int, char **);
  
  // Redraw objects
  static void redraw() {
    func[GTOOL_FRAME]->redraw();
  }
  
  // Show graphics tools
  static void show_tool();
  
  // Handler event from graphics area.  It calls an appropriate tool
  // to do its task.  There are two routines to be called here.
  // First, it calls a specific tool to handler appropriate events.
  // If thhe tool has already handled the event, then it will return
  // ROUTINE_DONE.  If the tool doesn't handle the event, it will pass
  // it to 'frame_task' (which its main purpose is to select or deselect
  // a frame).  Look at 'frame_task' for more details.
  static void handler_event(Event *e)
    {
      if (func[gtype]) 
	{
	  Routine_type type;
	  if ((type = func[gtype]->process(e)) != ROUTINE_DONE)
	    func[GTOOL_FRAME]->frame_task(e, type);
	}
    }
};


// Helper functions

// IsNearLine returns TRUE if point x,y is near the line segment defined
// by endpoints (x1,y1) and (x2,y2).  Nearness is specified by the
// tolerance parameter

extern int IsNearLine(int tolerance,int x,int y,int x1,int y1,int x2,int y2);

#endif
