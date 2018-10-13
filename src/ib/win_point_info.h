/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef _WIN_POINT_INFO_H
#define _WIN_POINT_INFO_H


/************************************************************************
*									*
*  @(#)win_point_info.h 18.1 03/21/08 (c)1991-92 SISCO
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
*  Window routines related to the cursor functions                	*
*									*
*************************************************************************/


class Win_point_info
{

 private:
      Frame frame;	// Parent
      Frame popup;	// Popup frame (subframe)

      static void done_proc(Frame);
 
   public:
      Win_point_info(void);
      ~Win_point_info(void);
      void show_window() { xv_set(popup, XV_SHOW, TRUE, NULL); }
      void point_intensity(char *);
      static Panel_item intensity;
      static Panel_item distance;
      static Panel_item space_dist;
      static Panel_item coordinates;
      static void show_3Ddist(char *);
      static void show_coordinates(char *);
      static void show_distance(char *);
      static void show_intensity(char *);
      static Win_point_info *winpointinfo;
};

#endif _WIN_POINT_INFO_H

