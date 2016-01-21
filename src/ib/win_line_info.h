/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef _WIN_LINE_INFO_H
#define _WIN_LINE_INFO_H


/************************************************************************
*									*
*									*
*************************************************************************
*									*
*  Doug Landau     							*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*  Window routines related to the line functions                	*
*									*
*************************************************************************/


/*  this structure holds an x and an intensity and is used by  */
/*  line.c to give to win_info */
typedef struct{
    short x;
    float y;
} Ipoint;

enum projection_type_type { ON_LINE, ACROSS_LINE } ;

// Class used to create info controller
class Win_line_info
{

 private:
      Frame frame;	// Parent
      Frame popup;	// Popup frame (subframe)

      static void done_proc(Frame);
      static int data_color;
      static int axis_color;
      static Gdev *gdev ;
      static void canvas_repaint();
      static int can_width ;
      static int can_height ;
      static Canvas can ;
      static float *profile_buf;
      static int profile_npoints;
      static double profile_length;
      static Panel_item proj_select;
 
   public:
      Win_line_info(void);
      ~Win_line_info(void);
      void show_window() { xv_set(popup, XV_SHOW, TRUE, NULL); }
      static Panel_item coordinates;
      static Panel_item line_length;
      static Panel_item vs;
      static Panel_item max_intensity;
      static void show_vs(char *);
      static void show_max(char *);
      static void show_line_length(char *);
      static void show_coordinates(char *);
      static void clear_projection();
      static void show_projection();
      static void show_projection(float *buf, int npoints, double length);
      static void change_projection_type(Panel_item, int, Event*);
      static void set_projection_type(int);
      static void reset();
      static float max_max_intensity;
      static float min_min_intensity;
      static projection_type_type projection_type;
      static Win_line_info *winlineinfo;
};


#endif _WIN_LINE_INFO_H

