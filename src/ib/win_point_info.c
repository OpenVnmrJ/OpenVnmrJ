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
*  Doug Landau     							*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Window routines related to the cursor functions                	*
*									*
*************************************************************************/
#include "stderr.h"
// #include <stream.h>    breaks new compilers CPP headers don't use .h, replaced with iostream & fstream below
#include <iostream>
#include <fstream>
#include <xview/xview.h>
#include <xview/panel.h>
#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "initstart.h"
#include "convert.h"
/* #include "process.h" */
#include "interrupt.h"
#include "ipgwin.h"
#ifdef LINUX
#include <strstream>
#else
#include <strstream.h>
#endif
#include "win_point_info.h"

#define DEFAULT_VERT_GAP  20

extern void win_print_msg(char *, ...);

// Initialize static class members
Panel_item Win_point_info::intensity = 0;
Panel_item Win_point_info::distance = 0;
Panel_item Win_point_info::space_dist = 0;
Panel_item Win_point_info::coordinates = 0;
Win_point_info *Win_point_info::winpointinfo = NULL;


/************************************************************************
*                                                                       *
*  Show the window.							*
*									*/
void
winpro_point_show(void)
{
   if (Win_point_info::winpointinfo == NULL)
      Win_point_info::winpointinfo = new Win_point_info;
   else
      Win_point_info::winpointinfo->show_window();
}

/************************************************************************
*                                                                       *
*  Creator of window.							*
*									*/
Win_point_info::Win_point_info(void)
{
   Panel panel;		// panel
   int xitempos;	// current panel item position
   int yitempos;	// current panel item position
   int xpos, ypos;      // window position
   char initname[128];	// init file

   (void)init_get_win_filename(initname);
   // Get the position of the control panel
   if (init_get_val(initname, "WINPRO_CURSOR", "dd", &xpos, &ypos) == NOT_OK)
   {
      xpos = 400;
      ypos = 20;
   }

   frame = xv_create(NULL, FRAME, NULL);

   popup = xv_create(frame, FRAME_CMD,
	XV_X,		xpos,
	XV_Y,		ypos,
        XV_WIDTH,       300,
        XV_HEIGHT,      100,
	FRAME_LABEL,	"Cursor Data",
	FRAME_DONE_PROC,	&Win_point_info::done_proc,
	FRAME_SHOW_RESIZE_CORNER,	TRUE,
	FRAME_CMD_PUSHPIN_IN,	TRUE,
	NULL);
   
   panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);


   xitempos = 10;
   yitempos = 10;
   intensity = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 10;
   yitempos += DEFAULT_VERT_GAP;
   coordinates = xv_create(panel,	PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 10;
   yitempos += DEFAULT_VERT_GAP;
   distance = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);


   xitempos = 10;
   yitempos += DEFAULT_VERT_GAP;
   space_dist = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

/*
   window_fit(panel);
   window_fit(popup);
   window_fit(frame);
*/

   xv_set(popup, XV_SHOW, TRUE, NULL);
   (void) init_get_win_filename(initname);
}


/************************************************************************
*                                                                       *
*  Print the 3D distance
*									*/
void
Win_point_info::show_3Ddist(char *msg)
{
   xv_set(space_dist, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*                                                                       *
*  Put the coordinates up                                               *
*									*/
void
Win_point_info::show_coordinates(char *msg)
{
   xv_set(coordinates, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*                                                                       *
*  Display the distance between the current and previous points.
*									*/
void
Win_point_info::show_distance(char *msg)
{
   xv_set(distance, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*                                                                       *
*  Put the point intensity up	                                        *
*									*/
void
Win_point_info::show_intensity(char *msg)
{
   xv_set(intensity, PANEL_LABEL_STRING, msg, NULL);
}


/************************************************************************
*                                                                       *
*  Destructor of window.						*
*									*/
Win_point_info::~Win_point_info(void)
{
   xv_destroy_safe(frame);
}



/************************************************************************
*                                                                       *
*  Dismiss the popup window.						*
*  [STATIC]								*
*									*/
void
Win_point_info::done_proc(Frame subframe)
{
  xv_set(subframe, XV_SHOW, FALSE, NULL);
  //cout << "delete winpointinfo = " << winpointinfo << endl;
  delete winpointinfo;
  winpointinfo = NULL;
  win_print_msg("Info: Exit");
}
