/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
#ifndef _ZOOM_H
#define _ZOOM_H
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
#include "gframe.h"

// Indicate which zoom-line is selected.  Accessed through bit
#define ZLINE_LEFT      1
#define ZLINE_RIGHT     2
#define ZLINE_TOP       4
#define ZLINE_BOTTOM    8
#define	ZLINE_ALL	16

// Menu properties
typedef enum
{
   Z_ZOOM,
   Z_UNZOOM,
   Z_BIND,
   Z_COLOR,
   Z_APERTURE,
   Z_REPLICATE,
   Z_INTERPOLATE,
   Z_FULLSCREEN,
   Z_UNFULLSCREEN,
   Z_TRACKING,
   Z_FACTOR,
   Z_PAN
} Zoom_props;

class Zoomf
{
   private:
      friend class Zoom_routine;

      static Flag bind;		// bind all selected frames for zoom-lines
      static int color;		// zoom-line color
      static int aperture;      // cursor sensitivity for zoom-lines
      static int max_tracks;
      static int zline;         // selected zoom lines
      static float zoomfactor;	// How much to quickzoom
//      static Flag select_zoom_lines(register Imginfo *, int x, int y);
//      static void move_lines(Imginfo *, int, int, int, int);
//      static void move_all_lines(Imginfo *, int, int, int, int);
//      static void draw_zlinex1(Imginfo *);
//      static void draw_zlinex2(Imginfo *);
//      static void draw_zliney1(Imginfo *);
//      static void draw_zliney2(Imginfo *);

   public:
      static int get_zoom_color(void) { return(color); }
      static void zoom_image(Zoom_props);  // Take only Z_ZOOM or Z_UNZOOM
      static void zoom_full(Zoom_props);  // Take only Z_ZOOM or Z_UNZOOM
      static void zoom_quick(Gframe *, int x, int y, Zoom_props inout);
      static void draw_zoom_lines(void);	// At all selected frames
//      static void draw_zoom_lines(Imginfo *);	// At a specific frame
      static int set_attr(int, char *);
      static int Factor(int argc, char **argv, int, char **);
};
#endif _ZOOM_H
