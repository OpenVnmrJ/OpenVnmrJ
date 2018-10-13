/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef lint
   static char *Sid = "Copyright (c) Varian Assoc., Inc.  All Rights Reserved.";
#endif (not) lint

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  This file conatins related to adjusting zoom-lines, zooming image.	*
*  it gets the information from an item "Imginfo" in a frame.		*
*									*
*************************************************************************/

#include <math.h>
#include "ddllib.h"
#include "stderr.h"
#include "inputwin.h"
#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "zoom.h"
#include "common.h"
#include "macroexec.h"

extern int canvas_width ;
extern int canvas_height ;
extern int canvas_stx ;
extern int canvas_sty ;
extern void canvas_repaint_proc(void);

//int smooth_zooms = 0 ;

// Initialize static class members
Flag Zoomf::bind;
int Zoomf::color;
int Zoomf::aperture;
int Zoomf::max_tracks = 20;
int Zoomf::zline;
float Zoomf::zoomfactor = 2.0;

static Gframe *big_frame = (Gframe *) 0;	// Static storage location.

/************************************************************************
*									*
*  Creator of zoom-routine.						*
*  (This function can only be called once.)                             *
*									*/
Zoom_routine::Zoom_routine(Gdev *gd)
{
   WARNING_OFF(gd);
   WARNING_OFF(Sid);

   active = FALSE;
   //smooth_zooms = FALSE;

   // Create properties menu for Zoom
   props_menu =
   xv_create(NULL,          MENU,
	     MENU_GEN_PIN_WINDOW, Gtools::get_gtools_frame(), "Zoom Props",
	     MENU_ITEM,
	     MENU_STRING,            "Zoom",
	     MENU_NOTIFY_PROC,               &Zoom_routine::menu_handler,
	     MENU_CLIENT_DATA,       Z_ZOOM,
	     NULL,
	     MENU_ITEM,
	     MENU_STRING,            "Unzoom",
	     MENU_NOTIFY_PROC,               &Zoom_routine::menu_handler,
	     MENU_CLIENT_DATA,       Z_UNZOOM,
	     NULL,
	     MENU_ITEM,
	     MENU_STRING,            "Bind",
	     MENU_NOTIFY_PROC,               &Zoom_routine::menu_handler,
	     MENU_CLIENT_DATA,       Z_BIND,
	     NULL,
	     MENU_ITEM,
	     MENU_STRING,            "Tracking ...",
	     MENU_NOTIFY_PROC,               &Zoom_routine::menu_handler,
	     MENU_CLIENT_DATA,       Z_TRACKING,
	     NULL,
	     MENU_ITEM,
	     MENU_STRING,            "Zoom factor ...",
	     MENU_NOTIFY_PROC,               &Zoom_routine::menu_handler,
	     MENU_CLIENT_DATA,       Z_FACTOR,
	     NULL,
	     // MENU_ITEM,
	     // MENU_STRING,            "Color ...",
	     // MENU_CLIENT_DATA,       Z_COLOR,
	     // NULL,
	     MENU_ITEM,
	     MENU_STRING,            "Cursor Tolerance ...",
	     MENU_NOTIFY_PROC,               &Zoom_routine::menu_handler,
	     MENU_CLIENT_DATA,       Z_APERTURE,
	     NULL,
	     MENU_ITEM,
	     MENU_STRING,            "Pixel Interpolation",
	     MENU_NOTIFY_PROC,               &Zoom_routine::menu_handler,
	     MENU_CLIENT_DATA,       Z_INTERPOLATE,
	     NULL,
	     NULL);

   // Assign the color to be the last gray value
   Zoomf::color = G_Get_Stcms2(gd) + G_Get_Sizecms2(gd) - 1;

   Zoomf::aperture = G_APERTURE;
}

/************************************************************************
*									*
*  Execute user selcted menu.						*
*									*/
void
Zoom_routine::menu_handler(Menu, Menu_item i)
{
   Zoom_props props = (Zoom_props)xv_get(i, MENU_CLIENT_DATA);
   switch (props)
   {
      case Z_ZOOM:
      case Z_UNZOOM:
	 Zoomf::zoom_image(props);
	 break;

      case Z_BIND:
	 if (Zoomf::bind){
	     xv_set(i, MENU_STRING, "Bind", NULL);
	     Zoomf::bind = FALSE;
	 }else{
	     xv_set(i, MENU_STRING, "Unbind", NULL);
	     Zoomf::bind = TRUE;
	 }
	 break;

      case Z_COLOR:
	 inputwin_show((int)Z_COLOR, &Zoomf::set_attr,
	    "Zoom-Line-Color (Range: 0 - 255)");
         break;

      case Z_APERTURE:
	 inputwin_show((int)Z_APERTURE, &Zoomf::set_attr, 
	    "Zoom Cursor Tolerance (Range: 1 - 20)");
         break;

      case Z_TRACKING:
	 inputwin_show((int)Z_TRACKING, &Zoomf::set_attr, 
	    "Max Tracking Zooms (Range: 0 - 200)");
         break;

      case Z_FACTOR:
	 inputwin_show((int)Z_FACTOR, &Zoomf::set_attr, 
	    "Zoom mag. factor (Range: 1 < f <= 5)");
         break;

      case Z_REPLICATE:
	 xv_set(i,
		MENU_STRING, "Pixel Interpolation",
		MENU_CLIENT_DATA, Z_INTERPOLATE,
		NULL);
	 dont_smooth();
	 //smooth_zooms = FALSE ;
         break;

      case Z_INTERPOLATE:
	 xv_set(i,
		MENU_STRING, "Pixel Replication",
		MENU_CLIENT_DATA, Z_REPLICATE,
		NULL);
	 smooth();
	 //smooth_zooms = TRUE ;
         break;
	   
      case Z_FULLSCREEN:		// These cases should not occur
      case Z_UNFULLSCREEN:
      default:
	 break;
   }
}

/************************************************************************
*									*
*  Initialize anything related to zoom.  It is called when the user just*
*  selects the gtool Zoom.						*
*									*/
void
Zoom_routine::start(Panel props, Gtype gtype)
{
   WARNING_OFF(gtype);
   active = TRUE;
   xv_set(props, PANEL_ITEM_MENU, props_menu, NULL);
   Gtools::set_props_label("Zoom Properties");
//   G_Set_LineWidth(Gframe::gdev, 2);
   Zoomf::draw_zoom_lines();	// Draw zoom-lines
}

/************************************************************************
*									*
*  Clean-up routine (about to leave gtool Zoom).  It is called when    	*
*  the user has selected another tool.                                  *
*                                                                       */
void
Zoom_routine::end(void)
{
   Zoomf::draw_zoom_lines();	// Erase zoom-lines
   G_Set_LineWidth(Gframe::gdev, 0);
   active = FALSE;
}

/************************************************************************
*                                                                       *
*  Mouse event Graphics-tool: Zoom.                                    	*
*  This function will be called if there is an event related to Zoom.	*
*                                                                       */
Routine_type
Zoom_routine::process(Event *e)
{
    static float left;		// Starting positions of zoom cursors
    static float right;
    static float top;
    static float bottom;
    static int prevx;
    static int prevy;
    static short firstx;	// first x mouse cursor value
    static short firsty;	// first y mouse cursor value
    static Imginfo *z_imginfo; // Hold the selected image information
    static Gframe *z_gframe;   // Hold the selected frame info
    Gframe *gframe;	// loop for selected frame
    int nth;		// loop counter
    Imginfo *iptr;	// image information pointer
    short x = event_x(e);
    short y = event_y(e);
    int nfrms;
    
    switch (event_action(e))   {
      case LOC_DRAG:
	// Execute only if there is a selected zoomed lines.
	if (Zoomf::zline == 0){
	    return(ROUTINE_FRAME_SELECT);
	}
	// Move the zoom-lines to which the cursor points
	z_imginfo->move_zlines(x - firstx, y - firsty,
			       left, right, top, bottom,
			       Zoomf::color, Zoomf::zline);
	if (Zoomf::bind){ // Move zoom-lines for all selected frames
	    nfrms = 0;
	    for (nth=1, gframe=Frame_select::get_selected_frame(nth); 
		 gframe;
		 nth++, gframe=Frame_select::get_selected_frame(nth))
	    {
		if ((iptr = gframe->imginfo)
		    && (iptr != z_imginfo)
		    && nfrms < Zoomf::max_tracks)
		{
		    iptr->replace_zlines(z_imginfo->zlinex1,
					 z_imginfo->zlinex2,
					 z_imginfo->zliney1,
					 z_imginfo->zliney2,
					 Zoomf::color);
		    nfrms++;
		}  
	    }  /* for */
	}  /* if (Zoomf::bind) */
	prevx = x;
	prevy = y;
	return(ROUTINE_DONE);
	
      case ACTION_SELECT:
	if (event_is_down(e)) {
	    // Move zoom-lines.  Note that we search for all frames
	    for (nth=1, gframe=Frame_select::get_selected_frame(nth); 
		 gframe;
		 nth++, gframe=Frame_select::get_selected_frame(nth))   {
		// A frame should contains an image and the cursor should be
		// within image boundary
		if ((iptr = gframe->imginfo)
		    && com_point_in_rect(x, y, iptr->pixstx - Zoomf::aperture,
					 iptr->pixsty - Zoomf::aperture,
					 iptr->pixstx + iptr->pixwd +
					 Zoomf::aperture,
					 iptr->pixsty + iptr->pixht +
					 Zoomf::aperture))
		{
		    // If no selected zoom-line, it means that it moves all
		    // zoom lines
		    if (!(Zoomf::zline=gframe->imginfo->select_zoom_lines(x, 
				 y, Zoomf::aperture, Zoomf::zline)))
		    {
			Zoomf::zline |= ZLINE_ALL;
		    }
		    prevx = firstx = x;
		    prevy = firsty = y;
		    z_gframe = gframe;
		    z_imginfo = gframe->imginfo;
		    left = z_imginfo->zlinex1;
		    right = z_imginfo->zlinex2;
		    top = z_imginfo->zliney1;
		    bottom = z_imginfo->zliney2;
		    return(ROUTINE_DONE);
		}  /* if com_point_in_rect(...) */
	    } /* for */
	    // Nothing to zoom
	    return(ROUTINE_FRAME_SELECT);
	} else {	// event_is_up(e)
	    // Execute only if there is a selected zoomed line.
	    if (Zoomf::zline == 0){
		return(ROUTINE_FRAME_SELECT);
	    }
	    
	    // Move the zoom-lines to which the cursor points
	    z_imginfo->move_zlines(x - firstx, y - firsty,
				   left, right, top, bottom,
				   Zoomf::color, Zoomf::zline);
	    /*z_imginfo->move_zlines(x, y, prevx, prevy, Zoomf::color, 
	      Zoomf::zline);*/
	    if (Zoomf::bind){
		// Move zoom-lines for all selected frames
		for (nth=1, gframe=Frame_select::get_selected_frame(nth); 
		     gframe;
		     nth++, gframe=Frame_select::get_selected_frame(nth))
		{
		    if ((iptr = gframe->imginfo)
			&& (iptr != z_imginfo))
		    {
			iptr->replace_zlines(z_imginfo->zlinex1,
					     z_imginfo->zlinex2,
					     z_imginfo->zliney1,
					     z_imginfo->zliney2,
					     Zoomf::color);
		    }  
		}  /* for */
	    }  /* if (Zoomf::bind) */
	    Zoomf::zline = 0;
	    return(ROUTINE_DONE);
	}  /* event_is_up */
	break;
	
	
      default:
	break;
	
    }  /* switch */
    
    return(ROUTINE_DONE);
}


/************************************************************************
*                                                                       *
*  Zoom an image in or out by Zoomf::zoomfactor.
*  (STATIC function)							*
*									*/
void
Zoomf::zoom_quick(Gframe *gframe, int x, int y, Zoom_props inout)
{
    Imginfo *img;
    if ( !gframe || !(img = gframe->imginfo)){
	return;
    }
    float factor;
    switch (inout){
      case Z_ZOOM:
	factor = zoomfactor;
	break;
      case Z_UNZOOM:
	factor = 1 / zoomfactor;
	break;
      case Z_PAN:
      default:
	factor = 1.0;
	break;
    }
    img->quickzoom(gframe, x, y, factor);
}

/************************************************************************
*                                                                       *
*  Zoom an image in a frame.  For each selected frame, it will zoom or	*
*  unzoom image. 							*
*  (STATIC function)							*
*									*/
void
Zoomf::zoom_full(Zoom_props props)  // Take only Z_ZOOM or Z_UNZOOM
{
   Gframe *gframe;	// current frame pointer
   Imginfo *imginfo;	// image information

   gframe=Frame_select::get_selected_frame(1); 
   // Only zoom image if a frame contains an image
   if (imginfo = gframe->imginfo){
      if (props == Z_FULLSCREEN)
      {
	big_frame = imginfo->zoom_full(gframe);
      }
      else	// (props == Z_UNZOOM)
      {
        if (big_frame)
          big_frame->bye_big_gframe(big_frame);
        canvas_repaint_proc();
      }
   }
}

/************************************************************************
*                                                                       *
*  Zoom an image in a frame.  For each selected frame, it will zoom or	*
*  unzoom image. 							*
*  (STATIC function)							*
*									*/
void
Zoomf::zoom_image(Zoom_props props)  // Take only Z_ZOOM or Z_UNZOOM
{
    int nframe;		// nth frame for loop counter
    Gframe *gframe;	// current frame pointer
    Imginfo *imginfo;	// image information
    
    // Only zoom image if a frame contains an image
    for (nframe=1, gframe=Frame_select::get_selected_frame(nframe); 
	 gframe;
	 nframe++, gframe=Frame_select::get_selected_frame(nframe))
    {
	if (imginfo = gframe->imginfo){
	    if (props == Z_ZOOM){
		imginfo->zoom(gframe);
	    }else{	// (props == Z_UNZOOM)
		imginfo->unzoom(gframe);
	    }  // if (props == ...)
	}  // if (imginfo ...)
    }
}


/************************************************************************
*                                                                       *
*  Draw zoom-lines for all selected frames.				*
*  (STATIC function)							*
*									*/
void
Zoomf::draw_zoom_lines(void)
{
    int nframe;		// nth frame for loop counter
    Gframe *gframe;	// current frame pointer
    
    // Only draw zoom-lines if a frame contains an image
    for (nframe=1, gframe=Frame_select::get_selected_frame(nframe);
	 gframe;
	 nframe++, gframe=Frame_select::get_selected_frame(nframe))
    {
	if (gframe->imginfo){
	    gframe->imginfo->draw_zoom_lines(color);
	}
    }
}


/************************************************************************
*                                                                       *
*  Set Zoom attributes:color and aperture.                              *
*  Return OK or NOT_OK.                                                 *
*  (STATIC)                                                             *
*                                                                       */
int
Zoomf::set_attr(int id, char *attr_str)
{
   int val ;

   if (id == Z_COLOR)
   {
      if (*attr_str == '?')
	 msginfo_print("Color = %d\n", color);
      else if (((val = atoi(attr_str)) < 0) || (val > 256))
      {
         msgerr_print("Color value is out of limit");
         return(NOT_OK);
      }
      else
      {
         draw_zoom_lines();	// Erase zoom-lines
         color = val ;
         draw_zoom_lines();	// Draw zoom-lines
      }
   }
   else if (id == Z_APERTURE)
   {
      if (*attr_str == '?')
	 msginfo_print("Aperture = %d\n", aperture);
      else if (((val = atoi(attr_str)) < 1) || (val > 20))
      {
         msgerr_print("Aperture value is not within limit");
         return(NOT_OK);
      }   
      else
         aperture = val;
   }
   else if (id == Z_TRACKING)
   {
      if (*attr_str == '?')
	 msginfo_print("Max tracking zooms = %d\n", max_tracks);
      else if (((val = atoi(attr_str)) < 0) || (val > 200))
      {
         msgerr_print("Max tracking zooms value is not within limits");
         return(NOT_OK);
      }   
      else
         max_tracks = val;
   }else if (id == Z_FACTOR){
       float factor = 0;
       sscanf(attr_str,"%g", &factor);
      if (*attr_str == '?')
	 msginfo_print("Zoom mag factor = %g\n", zoomfactor);
      else if (factor <= 1.0 || factor > 5.0)
      {
         msgerr_print("Zoom magnification value is not within limits");
         return(NOT_OK);
      }else{
         zoomfactor = factor;
	 char macrocmd[100];
	 sprintf(macrocmd,"zoom_factor(%g)\n", factor);
	 macroexec->record(macrocmd);
      }
   }      
   return(OK);
}

/************************************************************************
*									*
*  Set the magnification factor for quick zoom
*  [MACRO interface]
*  argv[0]: The factor to set
*  [STATIC Function]							*
*									*/
int
Zoomf::Factor(int argc, char **argv, int, char **)
{
    char *name = argv[0];
    argc--; argv++;

    char usage[100];
    sprintf(usage,"usage: %s(factor)    [1 < factor <= 5]", name);

    if (argc != 1){
	msgerr_print(usage);
	ABORT;
    }

    float factor = 0;
    sscanf(argv[0],"%g", &factor);
    if (factor <= 1.0 || factor > 5.0){
	msgerr_print("Zoom magnification value is not within limits");
	ABORT;
    }else{
	zoomfactor = factor;
	char macrocmd[100];
	sprintf(macrocmd,"zoom_factor(%g)\n", factor);
	macroexec->record(macrocmd);
    }
    
    return PROC_COMPLETE;
}
