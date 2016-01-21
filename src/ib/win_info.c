/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
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
#include <stream.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <memory.h>
#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "initstart.h"
#include "convert.h"
#include "process.h"
#include "interrupt.h"
#include "ipgwin.h"
#include "strstream.h"
#include "win_info.h"

#define DEFAULT_VERT_GAP  20
#define DEFAULT_CANVAS_WIDTH 300
#define DEFAULT_CANVAS_HEIGHT 125

extern void win_print_msg(char *, ...);
static void canvas_resize_proc(void);
static Win_info *wininfo=NULL;

enum projection_type_type { ON_LINE, ACROSS_LINE } ;
projection_type_type projection_type = ON_LINE ;

/************************************************************************
*                                                                       *
*  Show the window.							*
*									*/
void
winpro_info_show(void)
{
   if (wininfo == NULL)
      wininfo = new Win_info;
   else
      wininfo->show_window();
}

/************************************************************************
*                                                                       *
*  Creator of window.							*
*									*/
Win_info::Win_info(void)
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
   if (init_get_val(initname, "WINPRO_INFO", "dd", &xpos, &ypos) == NOT_OK)
   {
      WARNING_OFF(Sid);
      xpos = 400;
      ypos = 20;
   }

   frame = xv_create(NULL, FRAME, NULL);

   popup = xv_create(frame, FRAME_CMD,
	XV_X,		xpos,
	XV_Y,		ypos,
        XV_WIDTH,       400,
        XV_HEIGHT,      300,
	FRAME_LABEL,	"Cursor/Line information",
	FRAME_DONE_PROC,	&Win_info::done_proc,
	FRAME_SHOW_RESIZE_CORNER,	TRUE,
	FRAME_CMD_PUSHPIN_IN,	TRUE,
	NULL);
   
   panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);


   xitempos = 10;
   yitempos = 10;
   item = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Point intensity:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 105;
   yitempos = 10;
   intensity = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 10;
   yitempos += DEFAULT_VERT_GAP;
   item = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Last coordinates:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 115;
   last_coords = xv_create(panel,	PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 10;
   yitempos += DEFAULT_VERT_GAP;
   item = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Coordinates:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 85;
   coordinates = xv_create(panel,	PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 10;
   yitempos += DEFAULT_VERT_GAP;
   item = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Distance between points:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 175;
   distance = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 10;
   yitempos += DEFAULT_VERT_GAP;
   item = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Length of line:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 105;
   line_length = xv_create(panel,	PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   xitempos = 10;
   yitempos += DEFAULT_VERT_GAP;
   xv_create(panel,			 PANEL_CHOICE,
             XV_X,             		 xitempos,
             XV_Y,             		 yitempos,
             PANEL_LABEL_STRING,	 "Display:",
             PANEL_CHOICE_STRINGS,  	 "On Line", "Across Line", NULL,
             PANEL_NOTIFY_PROC, 	 &Win_info::change_projection_type,
             PANEL_VALUE, 0,
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
      STDERR("win_info:siscms_create:cannot create siscms");
      exit(1);
   }

   can = xv_create(popup,       CANVAS,
                XV_X,           0,
                XV_Y,           150,
                XV_WIDTH,       WIN_EXTEND_TO_EDGE,
                XV_HEIGHT,      WIN_EXTEND_TO_EDGE,
                WIN_BORDER,     FALSE,
                WIN_DYNAMIC_VISUAL,     TRUE,
                CANVAS_RESIZE_PROC,     &Win_info::canvas_resize,
                CANVAS_REPAINT_PROC,    NULL,
                NULL);
 
   // Create graphics device (bind it to this canvas).  This gdev will be
   // used in every graphics drawing into this canvas
   if ((gdev = (Gdev *)g_device_create(can, siscms)) == NULL)
   {
      STDERR("window_create_objects:g_device_create:cannot create graphics device");
      exit(1);
   }

   // Assign color to be the last gray value
   color = G_Get_Stcms2(gdev) + G_Get_Sizecms2(gdev) - 1;




/*
   window_fit(panel);
   window_fit(popup);
   window_fit(frame);
*/

   xv_set(popup, XV_SHOW, TRUE, NULL);

   (void)init_get_win_filename(initname);


}

/************************************************************************
*                                                                       *
*  Put a projection up                                                  *
*                                                                       */
void
Win_info::win_info_project(Fpoint *fpoints, Fpoint *epoints, int npoints,
                                                             double vs)

{
  const int canvas_margin = 10 ;
  int assign;
  float max_intensity = 0.0 ;
  float min_intensity = 10000 ;
  Gpoint *points ;
  float scale_factor, can_scale_factor ;
  Fpoint *dpoints ;
  char str[80] ;
 
  Win_info::can_width = (int) xv_get(Win_info::can, XV_WIDTH);
  Win_info::can_height = (int) xv_get(Win_info::can, XV_HEIGHT);

  // clear the canvas
  g_clear_area_default(Win_info::gdev,0,0,Win_info::can_width,
                                          Win_info::can_height);

  if (projection_type == ON_LINE)
    dpoints = fpoints ;
  else
    dpoints = epoints ;
  if (dpoints == NULL)  return ;

  points = new Gpoint [npoints] ;

  scale_factor = vs  ;
  printf ( "scale_factor = %f\n", scale_factor);
  for (assign=0; assign < npoints; assign++)
  {
    dpoints[assign].y *= scale_factor ;
  }
  /* find maximum */
  for (assign=0; assign < npoints; assign++)
    {
      if (max_intensity < dpoints[assign].y)
        max_intensity = dpoints[assign].y ;
      if (min_intensity > dpoints[assign].y)
        min_intensity = dpoints[assign].y ;
    }
  can_scale_factor = (can_height - (2*canvas_margin)) / max_intensity  ;
  printf ( "maximum intensity = %f\n", max_intensity) ;
  printf ( "minimum intensity = %f\n", min_intensity) ;
  for (assign=0; assign < npoints; assign++)
  {
    points[assign].x = canvas_margin + dpoints[assign].x ;
    points[assign].y = Win_info::can_height - canvas_margin
                       - (int) (dpoints[assign].y * can_scale_factor );
  }
  printf ( "npoints = %d\n", npoints);
  printf ( "max_intensity * can_scale_factor = %f\n", max_intensity * can_scale_factor);
  // scale_factor = (1.0 / min_intensity) * ( (float)(Win_info::can_height - (2 *canvas_margin)) / (max_intensity * scale_factor) );


  g_draw_connected_lines (Win_info::gdev, points, npoints, Win_info::color);
  delete[] points ;

  sprintf (str, "Max intensity:%f\n", max_intensity);
g_draw_string(Win_info::gdev,300,Win_info::can_height-90,FONT_MEDIUM,str,color);
  sprintf (str, "Vs:%f\n", vs );
g_draw_string(Win_info::gdev,300,Win_info::can_height-60,FONT_MEDIUM,str,color);
  sprintf (str, "Scale:%f\n", (scale_factor * can_scale_factor) );
g_draw_string(Win_info::gdev,300,Win_info::can_height-30,FONT_MEDIUM,str,color);

}

/************************************************************************
*                                                                       *
*  Put the line length up                                               *
*									*/
void
Win_info::win_info_line_length(char *msg)
{
   xv_set(Win_info::line_length, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*                                                                       *
*  Put the last coordinates up                                          *
*									*/
void
Win_info::win_info_last_coords(char *msg)
{
   xv_set(Win_info::last_coords, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*                                                                       *
*  Put the coordinates up                                               *
*									*/
void
Win_info::win_info_point_coords(char *msg)
{
   xv_set(Win_info::coordinates, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*                                                                       *
*  Put the distance between points up                                   *
*									*/
void
Win_info::win_info_point_distance(char *msg)
{
   xv_set(Win_info::distance, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*                                                                       *
*  Put the point intensity up	                                        *
*									*/
void
Win_info::win_info_point_intensity(char *msg)
{
   xv_set(Win_info::intensity, PANEL_LABEL_STRING, msg, NULL);
}


/************************************************************************
*                                                                       *
*  Destructor of window.						*
*									*/
Win_info::~Win_info(void)
{
   xv_destroy_safe(frame);
}



/************************************************************************
*                                                                       *
*  Dismiss the popup window.						*
*  [STATIC]								*
*									*/
void
Win_info::done_proc(Frame subframe)
{
  xv_set(subframe, XV_SHOW, FALSE, NULL);
  if (wininfo) delete wininfo;
  cout << "delete wininfo = " << wininfo << endl;
  wininfo = NULL;
  win_print_msg("Info: Exit");
}


/************************************************************************
*                                                                       *
*  Record the new width and height.  					*
*									*/
void
Win_info::canvas_resize(void)
{
  printf ("in MY canvas resize proc\n" ) ;


  // clear the canvas
  //g_clear_area(Win_info::gdev,0,0,Win_info::can_width,
                                //Win_info::can_height, color);
}


/************************************************************************
*                                                                       *
*  Record the new width and height.  					*
*									*/
void
Win_info::change_projection_type(Panel_item item, int value, Event *event)
{
  WARNING_OFF(item) ;
  WARNING_OFF(event) ;

  if (value)
    projection_type = ACROSS_LINE ;
  else
    projection_type = ON_LINE ; 

  Win_info::can_width = (int) xv_get(Win_info::can, XV_WIDTH);
  Win_info::can_height = (int) xv_get(Win_info::can, XV_HEIGHT);
printf ( "height is:%d\n", Win_info::can_height);

  // clear the canvas
  g_clear_area_default(Win_info::gdev,0,0,Win_info::can_width,
                                          Win_info::can_height);
}


