/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

static char *Sid(){
    return "@(#)point.c 18.1 03/21/08 (c)1991-92 SISCO";
}

/************************************************************************
*									*
*  Doug Landau 								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Routines related with points (cursor functions)			*
*									*
*  1. A line is interactively created by dragging the LEFT button.	*
*  2. A line can be moved by positioning the cursor close to a it. 	*
*     Then hold down the LEFT button and drag.				*
*									*
*************************************************************************/
#include <math.h>
#include "ddllib.h"
#include "stderr.h"
#include "graphics.h"
#include "gtools.h"

#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "roitool.h"
#include "point.h"
#include "common.h"
#include "msgprt.h"
#include "primitive.h"
#include "win_point_info.h"

int Point::id = 0;

/************************************************************************
*									*
*  Creator.								*
*									*/
Point::Point(void) : Roitool()
{
   created_type = ROI_POINT;
   
   // Point is defined by one point!
   pnt = new Gpoint [1];
   pnt[0].x = G_INIT_POS;	// Initialize
   npnts = 1;
   state = 0;
   visibility = VISIBLE_ALWAYS;
   visible = TRUE;
   resizable = TRUE;
   myID = ++id;
}

/************************************************************************
*									*
*  Creator.								*
*									*/
Point::Point(float x, float y, Gframe *frame) : Roitool()
{
   created_type = ROI_POINT;
   
   // Point is defined by one point!
   pnt = new Gpoint [1];
   pnt[0].x = G_INIT_POS;	// Initialize
   npnts = 1;
   state = 0;
   visibility = VISIBLE_ALWAYS;
   visible = TRUE;
   resizable = TRUE;
   myID = ++id;

   loc_data.x = x;
   loc_data.y = y;
   owner_frame = frame;
   Roi_routine::AppendObject(this, frame);
   update_screen_coords();
}

/************************************************************************
*									*
*  Destructor.								*
*									*/
Point::~Point(void)
{
   delete[] pnt;
}


/************************************************************************
*									*
*  Update the position of a point.
*									*/
ReactionType
Point::create(short x, short y, short)
{
  // Check for the minimum and maximum limit of the graphics area
  if ((x < 0) || (y < 0) || (x > Gdev_Win_Width(gdev)) ||
      (y > Gdev_Win_Height(gdev)))
    return REACTION_NONE;
  erase();				// Erase previous point
  
  // Update the points
  pnt[0].x = x; 	pnt[0].y = y;
  
  draw();				// Draw a new point
  update_data_coords();
  some_info(TRUE);
  return REACTION_NONE;
}

/************************************************************************
*									*
*  Updates some variables in the class.			*
*									*/
ReactionType
Point::create_done(short, short, short)
{
   x_min = x_max = pnt[0].x;
   y_min = y_max = pnt[0].y;
   basex = G_INIT_POS;
   update_data_coords();
   some_info();
   return REACTION_CREATE_OBJECT;
}

/************************************************************************
*									*
*  Move a point to a new location.					*
*									*/
ReactionType
Point::move(short x, short y)
{
   int max_wd = Gdev_Win_Width(gdev);	// maximum width
   int max_ht = Gdev_Win_Height(gdev);	// maximum height
   short dist_x = x - basex;		// distance x
   short dist_y = y - basey;		// distance y

   keep_roi_in_image(&dist_x, &dist_y);

   // Same position, do nothing
   if ((!dist_x) && (!dist_y)){
       return REACTION_NONE;
   }

   erase();	// Erase previous point

   // Update new point
   pnt[0].x += dist_x;
   pnt[0].y += dist_y;

   x_min = x_max = pnt[0].x;
   y_min = y_max = pnt[0].y;

   draw();	// Draw a point

   basex += dist_x;
   basey += dist_y;

   some_info(TRUE);

   return REACTION_NONE;
}

/************************************************************************
*									*
*  User just stops moving.						*
*									*/
ReactionType
Point::move_done(short, short)
{
   x_min = x_max = pnt[0].x;
   y_min = y_max = pnt[0].y;

   some_info();

   basex = G_INIT_POS ;
   return REACTION_NONE;
}

/************************************************************************
*                                                                       *
*  Copy this ROI to another Gframe
*                                                                       */
Roitool *
Point::copy(Gframe *gframe)
{
    if (!gframe || !gframe->imginfo){
	return NULL;
    }
    Roitool *tool;
    tool = new Point(loc_data.x, loc_data.y, gframe);
    return tool;
}

/************************************************************************
*                                                                       *
*  Erase a point.                                                          *
*                                                                       */
void
Point::erase()
{
    if ( ! redisplay_bkg(x_min-HAIR_LEN, y_min-HAIR_LEN,
			 x_max+HAIR_LEN, y_max+HAIR_LEN)){
	draw();	// (Assumes drawing in XOR mode)
    }
    roi_clear_state(ROI_STATE_EXIST);
}

/************************************************************************
*                                                                       *
*  Draw a point.                                                          *
*                                                                       */
void
Point::draw(void)
{
   if (pnt[0].x == G_INIT_POS)
      return;

   set_clip_region(FRAME_CLIP_TO_IMAGE);
   calc_xyminmax();

   if (visibility != VISIBLE_NEVER && visible != FALSE) {
     g_draw_line (gdev, pnt[0].x - HAIR_LEN, pnt[0].y,
                        pnt[0].x - 2, pnt[0].y, color);
     g_draw_line (gdev, pnt[0].x + HAIR_LEN, pnt[0].y,
                        pnt[0].x + 2, pnt[0].y, color);
     g_draw_line (gdev, pnt[0].x, pnt[0].y - HAIR_LEN, 
                        pnt[0].x, pnt[0].y - 2, color);
     g_draw_line (gdev, pnt[0].x, pnt[0].y + HAIR_LEN,
                        pnt[0].x, pnt[0].y + 2, color);
   }

   roi_set_state(ROI_STATE_EXIST);
    
   if (roi_state(ROI_STATE_MARK)){
       Point::mark();
   }
   set_clip_region(FRAME_NO_CLIP);
}


/************************************************************************
*                                                                       *
*  Mark a point.                                                        *
*                                                                       */
void
Point::mark(void)
{
    if (!markable) return;
    if (pnt[0].x == G_INIT_POS) return;

    draw_mark(pnt[0].x + HAIR_LEN, pnt[0].y + HAIR_LEN);
    draw_mark(pnt[0].x + HAIR_LEN, pnt[0].y - HAIR_LEN);
    draw_mark(pnt[0].x - HAIR_LEN, pnt[0].y + HAIR_LEN);
    draw_mark(pnt[0].x - HAIR_LEN, pnt[0].y - HAIR_LEN);
}
 

/************************************************************************
*									*
*  Calculate some cursor info
*  The "moving" flag is true if the mouse button is still depressed.
*									*/
void
Point::some_info(int moving)
{
    Gframe *gf;
    char str[100];
    static int prev_id = 0;		// ID of the previous point
    static Fpoint prev_loc2D = {0,0};	// Position of last point in cm
    static D3Dpoint prev_loc3D = {0,0,0}; // Previous position in magnet frame
    static int curr_id = 0;
    static Fpoint curr_loc2D = {0,0};	// Position of last point in cm
    static D3Dpoint curr_loc3D = {0,0,0}; // Current position
    // Implementation note: The current and previous cursor positions are
    //		stored in the static variables curr_... and prev_... at
    //		the end of this routine only if the mouse button has just
    //		been released (moving=FALSE).  What gets stored where
    //		depends on the IDs of the cursor (point) objects.  The idea is
    //		that we need to print the distance from the current cursor
    //		to the previous different cursor.  We don't want to
    //		overwrite the previous position of the other cursor with
    //		the position of the current one.
    //	If "this->myID" is the same as curr_id, the curr_... variables are
    //	just updated.  If it is different, the curr_... variables replace
    //	the prev_... ones and the new values are placed in the curr_...
    //	variables.  The distances are always those between the
    //	just retrieved location, and the most recent other location with
    //	a different ID.

    // Check that there is something to do
    if (! Win_point_info::winpointinfo){
	return;
    }

    // Do not do anything if we are not the active ROI
    if (position_in_active_list(this) > 0){
	return;
    }

    // Find which frame we are in
    gf = owner_frame;
    Imginfo *img = gf->imginfo;

    // Which data pixel we're on
    int x = (int)loc_data.x;
    int y = (int)loc_data.y;

    // Location in the magnet frame (cm)
    D3Dpoint loc3D = img->pixel_to_magnet_frame(loc_data);

    /******************
    fprintf(stderr,"Point location: (%g, %g, %g)\n",
    loc3D.x, loc3D.y, loc3D.z);
    ******************/

    // Calculate the scale factors
    // ASSUME THE "SPAN" (GetRatioFast/GetRatioMedium) GIVES THE DISTANCE
    // FROM ONE EDGE OF THE PICTURE TO THE OTHER, RATHER THAN THE
    // DISTANCE BETWEEN CENTERS OF THE OPPOSITE EDGE PIXELS.
    // THEREFORE, WE DIVIDE BY THE NUMBER OF PIXELS, RATHER THAN THE
    // NUMBER OF PIXELS MINUS 1.
    float wd = img->GetRatioFast();
    float ht = img->GetRatioMedium();
    float xscale = wd / img->GetFast();
    float yscale = ht / img->GetMedium();
    Fpoint loc2D;
    loc2D.x = loc_data.x * xscale;
    loc2D.y = loc_data.y * yscale;

    // Display intensity and coordinate info.
    float *data = (float *)gf->imginfo->GetData();
    float intensity = data[(y * img->GetFast()) + x];
    sprintf (str, "Intensity: %.4g", intensity);
    Win_point_info::show_intensity(str) ;
    //sprintf (str, "Coordinates: %.1f, %.1f (pixels)", loc_data.x, loc_data.y);
    sprintf (str, "Coordinates: %.4g, %.4g, %.4g (cm)",
	     loc3D.x, loc3D.y, loc3D.z);
    Win_point_info::show_coordinates(str) ;

    // Find distances from previously selected point
    int found = 0;
    double dx, dy, dz;
    float dx2, dy2;
    if (curr_id && curr_id != myID){
	dx = curr_loc3D.x - loc3D.x;
	dy = curr_loc3D.y - loc3D.y;
	dz = curr_loc3D.z - loc3D.z;
	dx2 = curr_loc2D.x - loc2D.x;
	dy2 = curr_loc2D.y - loc2D.y;
	found++;
    }else if (prev_id && prev_id != myID){
	dx = prev_loc3D.x - loc3D.x;
	dy = prev_loc3D.y - loc3D.y;
	dz = prev_loc3D.z - loc3D.z;
	dx2 = prev_loc2D.x - loc2D.x;
	dy2 = prev_loc2D.y - loc2D.y;
	found++;
    }

    // Display the relevant distances
    // (We should really figure out if the two images are in the same
    // location and orientation, and, if they are, only print out the
    // "distance"--which is the same as the projected distance.  If the
    // orientations of the two images differ, the "projected distance"
    // doesn't make much sense.)
    if (found){
	float dist2D = sqrt(dx2*dx2 + dy2*dy2);
	double dist3D = sqrt(dx*dx + dy*dy + dz*dz);
	sprintf(str,"3D distance from last point: %.4g cm", dist3D);
	Win_point_info::show_3Ddist(str);
	sprintf(str,"Projected distance from last point: %.4g cm", dist2D);
	Win_point_info::show_distance(str);
    }else{
	Win_point_info::show_3Ddist("");
	Win_point_info::show_distance("");
    }
	
    // Remember facts about this point
    if (! moving){
	//fprintf(stderr,"prev=%d, curr=%d, my=%d\n", prev_id, curr_id, myID);
	if (curr_id == myID){
	    curr_loc3D = loc3D;
	    curr_loc2D = loc2D;
	}else{
	    // ... and previous point
	    prev_id = curr_id;
	    prev_loc3D = curr_loc3D;
	    prev_loc2D = curr_loc2D;
	    curr_id = myID;
	    curr_loc3D = loc3D;
	    curr_loc2D = loc2D;
	}
    }
}

/************************************************************************
*									*
*  Check whether a point is selected or not.				*
*  This will happen when the mouse button is clicked within "aperture"
*  pixels of the point. 						*
*  Return TRUE or FALSE.						*
*									*/
Flag
Point::is_selected(short x, short y)
{

  if ((abs(pnt[0].x - x) < aperture) && (abs(pnt[0].y - y) < aperture))
  {
    acttype = ROI_MOVE ;
    return (TRUE);
  }
  else
    return (FALSE);
}

/************************************************************************
*									*
*  Save the current ROI point into the following format:		*
*									*
*     # <comments>                                                      *
*     X Y                                                               *
*                                                                       *
*  where                                                                *
*        # indicates comments
*	 point
*        X,Y is the point                                       	*
*									*/
void
Point::save(ofstream &outfile)
{
   outfile << name() << "\n";
   outfile << loc_data.x << " " << loc_data.y << "\n";
}

/************************************************************************
*									*
*  Load ROI point from a file.  Format is described in "save" routine.
*									*/
void
Point::load(ifstream &infile)
{
    const int buflen=128;
    char buf[buflen];
    int ndata=0;
    Fpoint temp;

    while ((ndata != 1) && infile.getline(buf, buflen)){
	if (buf[0] == '#'){
	    continue;
	}else if (strspn(buf, "\t ") == strlen(buf)){	// Ignore blank lines
	    continue;
	}else{
	    if (sscanf(buf,"%f %f", &(temp.x), &(temp.y)) != 2){
		msgerr_print("ROI point: Missing data input");
		return;
	    }
	    ndata++;
	}
    }

    if (ndata != 1){
	msgerr_print("ROI point: Incomplete input data points");
	return;
    }

    // Put a new ROI in the appropriate frames
    Gframe *gframe;
    Roitool *tool;
    int i = 1;
    for (gframe=Frame_select::get_selected_frame(i);
	 gframe;
	 gframe = Frame_select::get_selected_frame(++i))
    {
	if (gframe->imginfo){
	    tool = new Point(temp.x, temp.y, gframe);
	    tool->select(ROI_NOREFRESH, TRUE);
	    //gframe->display_data();
	}
    }
}

//
void Point::update_screen_coords()
{
    pnt[0].x = x_min = x_max = data_to_xpix(loc_data.x);
    pnt[0].y = y_min = y_max = data_to_ypix(loc_data.y);
}

//
void Point::update_data_coords()
{
    loc_data.x = xpix_to_data(pnt[0].x);
    loc_data.y = ypix_to_data(pnt[0].y);
}

void
Point::rot90_data_coords(int datawidth)
{
    double t;

    t = loc_data.x;
    loc_data.x = loc_data.y;
    loc_data.y = datawidth - t;
}

void
Point::flip_data_coords(int datawidth)
{
    loc_data.x = datawidth - loc_data.x;
}

// *************************************************************************
// Returns the address of the first (and only) data pixel in this ROI.
// *************************************************************************
float *
Point::FirstPixel()
{
    Imginfo *img = owner_frame->imginfo;
    int data_width = img->GetFast();

    float *data = ((float *)img->GetData()
		   + (int)loc_data.x + data_width * (int)loc_data.y);
    return data;
}
