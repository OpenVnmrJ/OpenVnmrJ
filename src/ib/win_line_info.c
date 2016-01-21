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
*  Window routines related to the cursor/line functions                	*
*									*
*************************************************************************/
#include "stderr.h"
// #include <stream.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "sisfile.h"
#include "params.h"
#include "gframe.h"
#include "roitool.h"
#include "initstart.h"
#include "convert.h"
/* #include "process.h" */
#include "interrupt.h"
#include "ipgwin.h"
#ifdef LINUX
// #include <strstream>
#else
// #include <strstream.h>
#endif
#include "axis.h"
#include "win_line_info.h"
#include "line.h"

#define DEFAULT_VERT_GAP  20
#define DEFAULT_CANVAS_WIDTH 300
#define DEFAULT_CANVAS_HEIGHT 125

extern void win_print_msg(char *, ...);

// Initialize static class members
int Win_line_info::data_color = 0;
int Win_line_info::axis_color = 0;
Gdev *Win_line_info::gdev = 0;
int Win_line_info::can_width = 0;
int Win_line_info::can_height = 0;
Canvas Win_line_info::can = 0;
Panel_item Win_line_info::coordinates = 0;
Panel_item Win_line_info::line_length = 0;
Panel_item Win_line_info::vs = 0;
Panel_item Win_line_info::max_intensity = 0;
Panel_item Win_line_info::proj_select = 0;
float Win_line_info::max_max_intensity = 0;
float Win_line_info::min_min_intensity = 0;
Win_line_info *Win_line_info::winlineinfo = NULL;
projection_type_type Win_line_info::projection_type = ON_LINE ;
float *Win_line_info::profile_buf = 0;
int Win_line_info::profile_npoints = 0;
double Win_line_info::profile_length = 0;

/************************************************************************
*                                                                       *
*  Show the window.							*
*									*/
void
winpro_line_show(void)
{
   if (Win_line_info::winlineinfo == NULL)
      Win_line_info::winlineinfo = new Win_line_info;
   else
      Win_line_info::winlineinfo->show_window();
}

/************************************************************************
*                                                                       *
*  Creator of window.							*
*									*/
Win_line_info::Win_line_info(void)
{
   Panel panel;		// panel
   int xitempos;	// current panel item position
   int yitempos;	// current panel item position
   Panel_item item;	// Panel item
   int xpos, ypos;      // window position
   char initname[128];	// init file
   // Canvas can;          // canvas handler
   Siscms *siscms;
   // Gdev *gdev ;
   // int color ;

   (void)init_get_win_filename(initname);
   // Get the position of the control panel
   if (init_get_val(initname, "WINPRO_LINE", "dd", &xpos, &ypos) == NOT_OK)
   {
      xpos = 400;
      ypos = 20;
   }

   frame = xv_create(NULL, FRAME,
		     WIN_DYNAMIC_VISUAL, TRUE,
		     NULL);

   popup = xv_create(frame, FRAME_CMD,
	XV_X,		xpos,
	XV_Y,		ypos,
        XV_WIDTH,       400,
        XV_HEIGHT,      200,
	FRAME_LABEL,	"Line Data",
	FRAME_DONE_PROC,	&Win_line_info::done_proc,
	FRAME_SHOW_RESIZE_CORNER,	TRUE,
	FRAME_CMD_PUSHPIN_IN,	TRUE,
	NULL);
   
   panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);


   yitempos = 10;
   xitempos = 10;
   coordinates = xv_create(panel,	PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Coordinates: ",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 280;
   item = xv_create(panel, 	        PANEL_BUTTON,
                XV_X,           	xitempos,
                XV_Y,           	yitempos,
                PANEL_LABEL_STRING,     "Reset Scale",
                PANEL_NOTIFY_PROC,      (&Win_line_info::reset),
                NULL);

   xitempos = 10;
   yitempos += DEFAULT_VERT_GAP;
   line_length = xv_create(panel,	PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Length of line: ",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 10;
   yitempos += DEFAULT_VERT_GAP;
   max_intensity = xv_create(panel,	PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Max: ",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 175;
   yitempos -= DEFAULT_VERT_GAP/3;
   int pval = projection_type == ON_LINE ? 0 : 1;
   proj_select =
   xv_create(panel,			 PANEL_CHOICE,
             XV_X,             		 xitempos,
             XV_Y,             		 yitempos,
             PANEL_LABEL_STRING,	 "Display:",
             PANEL_CHOICE_STRINGS,  	 "On Line", "Projection", NULL,
             PANEL_NOTIFY_PROC, 	 &Win_line_info::change_projection_type,
             PANEL_VALUE, pval,
             NULL);

   // Get initialized colormap file
   (void)init_get_cmp_filename(initname);

   // Create Siscms colormap structure and load the colormap from
   // initialized file.
   // Note that the order of colorname is important because that is the
   // order the colormap (red/gren/blue) will be loaded.
   if ((siscms = (Siscms *)siscms_create(initname, "mark-color",
       "gray-color", "false-color")) == NULL)
   {
      STDERR("win_line_info:siscms_create:cannot create siscms");
      exit(1);
   }

   can = xv_create(popup,       CANVAS,
                XV_X,           0,
                XV_Y,           75,
                XV_WIDTH,       WIN_EXTEND_TO_EDGE,
                XV_HEIGHT,      WIN_EXTEND_TO_EDGE,
                WIN_BORDER,     FALSE,
		OPENWIN_SHOW_BORDERS,	FALSE,
                WIN_DYNAMIC_VISUAL,     TRUE,
                CANVAS_RESIZE_PROC,	canvas_repaint,
                CANVAS_REPAINT_PROC,	canvas_repaint,
                NULL);
 
   // Create graphics device (bind it to this canvas).  This gdev will be
   // used in every graphics drawing into this canvas
   if ((gdev = (Gdev *)g_device_attach(can, siscms)) == NULL)
   {
      STDERR("win_line_info: g_device_create: cannot create graphics device");
      exit(1);
   }
 
   // Assign colors for graph and axes from mark-color selections (segment #1).
   data_color = G_Get_Stcms1(gdev) + 6;
   axis_color = G_Get_Stcms1(gdev) + 4;

/*
   window_fit(panel);
   window_fit(popup);
   window_fit(frame);
*/

   xv_set(popup, XV_SHOW, TRUE, NULL);
   (void)init_get_win_filename(initname);

   max_max_intensity = min_min_intensity = 0.0 ;
}

/************************************************************************
*                                                                       *
*  Put a projection up                                                  *
*                                                                       */
void
Win_line_info::show_projection(float *buf, int npoints, double length)
{
    int i;

    if (profile_buf) delete [] profile_buf;
    profile_buf = new float [npoints];
    for (i=0; i<npoints; i++){
	profile_buf[i] = buf[i];
    }
    profile_npoints = npoints;
    profile_length = length;
    show_projection();
}

/************************************************************************
*                                                                       *
*  Put a projection up                                                  *
*                                                                       */
void
Win_line_info::show_projection()
{
    const int can_bottom_margin = 30;
    const int can_top_margin = 12;
    const int can_left_margin = 80;
    const int can_right_margin = 20;
    int i;
    float max_value;
    float min_value;
    Gpoint *points ;
    float can_scale_factor;
    float width_scale_factor;
    char str[80] ;
    
    if (!winlineinfo || !profile_npoints) return ;
    
    // clear the canvas 
    clear_projection();

    int npts = profile_npoints;
    if (npts < 2){
	npts = 2;
    }
    points = new Gpoint [npts] ;
    points[0].x = points[0].y = 0;
    
    // Find maximum
    min_value = 0;
    max_value = profile_buf[0];
    for (i=1; i < profile_npoints; i++)
    {
	if (max_value < profile_buf[i]){
	    max_value = profile_buf[i];
	}
	if (min_value > profile_buf[i]){
	    min_value = profile_buf[i];
	}
    }
    if (max_max_intensity < max_value){
	max_max_intensity = max_value;
    }
    if (min_min_intensity > min_value){
	min_min_intensity = min_value;
    }
    
    can_scale_factor = ((can_height - can_bottom_margin - can_top_margin)
			/ (max_max_intensity - min_min_intensity ));
    width_scale_factor = ((can_width - can_left_margin - can_right_margin)
			  / (float)(npts - 1));
    
    for (i=0; i < profile_npoints; i++)
    {
	points[i].x = can_left_margin + (int)(i * width_scale_factor);
	points[i].y = (can_height - can_bottom_margin -
		       (int)((profile_buf[i] - min_min_intensity)
			     * can_scale_factor ));
    }
    if (profile_npoints < 2){
	points[1].x = can_width - can_right_margin;
	points[1].y = points[0].y;
    }
    
    // Don't use XOR'ing
    G_Set_Op(gdev, GXcopy);
    
    // Set up and draw the axes
    Axis axis(gdev, 0.0, min_min_intensity, profile_length, max_max_intensity);
    axis.location(can_left_margin, can_height - can_bottom_margin,
		  can_width - can_right_margin, can_top_margin);
    axis.color(axis_color);
    axis.number('x');
    axis.number('y');
    axis.plot();
    // Draw the y=0 zero baseline
    int ybase = can_height - can_bottom_margin
		- (int)((0 - min_min_intensity) * can_scale_factor);
    g_draw_line(gdev, can_left_margin, ybase,
		can_width - can_right_margin, ybase, axis_color);
    
    g_draw_connected_lines (gdev, points, npts, data_color);
    
    sprintf (str, "Max: %.6g\n", max_value);
    show_max(str);
    
    delete[] points ;
}

/************************************************************************
*                                                                       *
*  Put the line length up                                               *
*									*/
void
Win_line_info::show_line_length(char *msg)
{
  if (winlineinfo)
    xv_set(Win_line_info::line_length, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*                                                                       *
*  Put the vertical scale up						*
*									*/
void
Win_line_info::show_vs(char *msg)
{
  if (winlineinfo)
    xv_set(Win_line_info::vs, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*                                                                       *
*  Put the max intensity up						*
*									*/
void
Win_line_info::show_max(char *msg)
{
  if (winlineinfo)
    xv_set(Win_line_info::max_intensity, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*                                                                       *
*  Put the coordinates up                                               *
*									*/
void
Win_line_info::show_coordinates(char *msg)
{
  if (winlineinfo) 
   xv_set(Win_line_info::coordinates, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*                                                                       *
*  Destructor of window.						*
*									*/
Win_line_info::~Win_line_info(void)
{
    profile_npoints = 0;
    xv_destroy_safe(frame);
}

/************************************************************************
*                                                                       *
*  Dismiss the popup window.						*
*  [STATIC]								*
*									*/
void
Win_line_info::done_proc(Frame subframe)
{
  xv_set(subframe, XV_SHOW, FALSE, NULL);
  //cout << "delete winlineinfo = " << winlineinfo << endl;
  delete winlineinfo;
  winlineinfo = NULL;
  win_print_msg("Info: Exit");
}


/************************************************************************
*                                                                       *
*  
*									*/
void
Win_line_info::change_projection_type(Panel_item, int value, Event *)
{
    projection_type = value ? ACROSS_LINE : ON_LINE;
    clear_projection();
    // Put up the new plot
    Roitool *tool = Roitool::get_selected_tool();
    if (tool){
	tool->some_info();
    }
}


/************************************************************************
*                                                                       *
*									*/
void
Win_line_info::set_projection_type(int value)
{
    if (value >= 0){
	projection_type = value > 0 ? ACROSS_LINE : ON_LINE;
	xv_set(proj_select,
	       PANEL_VALUE, projection_type,
	       PANEL_INACTIVE, TRUE,
	       NULL);
    }else{
	xv_set(proj_select, PANEL_INACTIVE, FALSE, NULL);
    }
}


// **********************************************************************
//	Handler for canvas repaint events
//	(STATIC)
// **********************************************************************
void
Win_line_info::canvas_repaint()
{
    Roitool *tool = Roitool::get_selected_tool();
    if (tool && profile_npoints == 0){
	tool->some_info();
    }else{
	show_projection();
    }
}

/************************************************************************
*                                                                       *
*  clear the canvas                                                     *
*                                                                       */
void
Win_line_info::clear_projection(void)
{

  if (!winlineinfo) return;

  can_width = (int) xv_get(can, XV_WIDTH);
  can_height = (int) xv_get(can, XV_HEIGHT);

  //clear the canvas
  g_clear_area_default(gdev, 0, 0, can_width, can_height);

}


/************************************************************************
*                                                                       *
*  Reset scale to autoscale the next projection                          *
*                                                                       */
void
Win_line_info::reset(void)
{
    max_max_intensity = min_min_intensity = 0;
    canvas_repaint();
}
