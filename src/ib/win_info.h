/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef _WIN_INFO_H
#define _WIN_INFO_H


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
*  Window routines related to the cursor/line functions                	*
*									*
*************************************************************************/


/*  this structure holds an x and an intensity and is used by  */
/*  line.c to give to win_info */
typedef
struct _fpoint
{
  short x;
  float y;
} Fpoint;

// Class used to create info controller
class Win_info
{

 private:
      Frame frame;	// Parent
      Frame popup;	// Popup frame (subframe)

      static void done_proc(Frame);
      static int color ;
      static Gdev *gdev ;
      static void canvas_resize(void);
      static int can_width ;
      static int can_height ;
      static Canvas can ;
 
   public:
      Win_info(void);
      ~Win_info(void);
      void show_window() { xv_set(popup, XV_SHOW, TRUE, NULL); }
      void point_intensity(char *);
      static Panel_item intensity;
      static Panel_item distance;
      static Panel_item coordinates;
      static Panel_item last_coords;
      static Panel_item line_length;
      static void win_info_line_length(char *);
      static void win_info_last_coords(char *);
      static void win_info_point_coords(char *);
      static void win_info_point_distance(char *);
      static void win_info_point_intensity(char *);
      static void win_info_project(Fpoint *, Fpoint *, int, double vs);
      static void change_projection_type(Panel_item, int, Event*);
};


#endif _WIN_INFO_H

