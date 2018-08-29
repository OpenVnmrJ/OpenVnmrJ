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
    return "@(#)box.c 18.1 03/21/08 (c)1991-92 SISCO";
}

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
*  Routines related with Box.						*
*									*
*  1. A box is defined by the upper-left corner point and bottom-right	*
*     corner point.							*
*  2. A box is interactively created by dragging the LEFT button.	*
*  3. A box can be resized by positioning the cursor close to its     	*
*     corners.  Then hold down the LEFT button and drag.      		*
*  4. A box can be moved by holding down the LEFT button while the    	*
*     mouse cursor position is inside a box.                        	*
*									*
*************************************************************************/
#include <math.h>
#include "stderr.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "roitool.h"
#include "msgprt.h"
#include "box.h"
#include "common.h"

extern Gframe *framehead;	// List of all gframes--defined in gframe.c

/************************************************************************
*                                                                       *
*  Creator.                                                             *
*                                                                       */
Box::Box(void) : Roitool()
{
   created_type = ROI_BOX;

   // Box needs two points
   pnt = new Gpoint [2];
   pnt[0].x = G_INIT_POS;	// Initialize
   npnts = 2;

   state = 0;
   visibility = VISIBLE_ALWAYS;
   visible = TRUE;
   resizable = TRUE;
}

/************************************************************************
*                                                                       *
*  Creator.                                                             *
*                                                                       */
Box::Box(float x0, float y0, float x1, float y1, Gframe *frame) : Roitool()
{
   created_type = ROI_BOX;

   // Box needs two points
   pnt = new Gpoint [2];
   pnt[0].x = G_INIT_POS;	// Initialize
   npnts = 2;

   state = 0;
   visibility = VISIBLE_ALWAYS;
   visible = TRUE;
   resizable = TRUE;

   ul_corner_on_data.x = x0;
   ul_corner_on_data.y = y0;
   lr_corner_on_data.x = x1;
   lr_corner_on_data.y = y1;
   owner_frame = frame;
   Roi_routine::AppendObject(this, frame);
   update_screen_coords();
}

/************************************************************************
*                                                                       *
*  Destructor.                                                          *
*                                                                       */
Box::~Box(void)
{
   delete[] pnt;
}

/************************************************************************
*                                                                       *
*  Update the size of a box.                                            *
*                                                                       */
ReactionType
Box::create(short x, short y, short)
{
   // Check for the minimum and maximum limit of the graphics area
   if ((x < 0) || (y < 0)
       || (x > Gdev_Win_Width(gdev)) || (y > Gdev_Win_Height(gdev)))
   {
       return REACTION_NONE;
   }

   Box::erase();

   // Update the points
   pnt[0].x = x;        pnt[0].y = y;
   pnt[1].x = basex;    pnt[1].y = basey;

   // Make sure that pnt[0] contains upper left-corner point and
   // pnt[1] contain lower-left corner point
   com_minmax_swap(pnt[0].x, pnt[1].x);
   com_minmax_swap(pnt[0].y, pnt[1].y);

   draw();

   return REACTION_NONE;
}

/************************************************************************
*                                                                       *
*  Checks whether the area of the box is large enough to keep it.
*									*/
ReactionType
Box::create_done(short, short, short)
{
   basex = G_INIT_POS;
   if (area() < (G_APERTURE * G_APERTURE / 4))
   {
      Box::erase();
      init_point();
      return REACTION_DELETE_OBJECT;
   }

   Gmode mode = setcopy();
   roi_set_state(ROI_STATE_MARK);
   draw();
   setGmode(mode);

   // Assign minmax value.  Note that pnt[0] < pnt[1] for its x and y
   x_min = pnt[0].x;
   y_min = pnt[0].y;
   x_max = pnt[1].x;
   y_max = pnt[1].y;

   return REACTION_CREATE_OBJECT;
}

/************************************************************************
*                                                                       *
*  Change the dimensions of the box non-interactively                   *
*                                                                       */

void
Box::redimension(short width, short height)
{
  if (width < 1 || height < 1) return;

  if (pnt) {
    Box::erase();
    pnt[1].x = pnt[0].x + width;
    pnt[1].y = pnt[0].y + height;
    draw();
  }
}
    
   
/************************************************************************
*                                                                       *
*  Move a box to a new location.                                        *
*                                                                       */
ReactionType
Box::move(short x, short y)
{
   int max_wd = Gdev_Win_Width(gdev);   // maximum width
   int max_ht = Gdev_Win_Height(gdev);  // maximum height
   short dist_x = x - basex;            // distance x
   short dist_y = y - basey;            // distance y
 
   keep_roi_in_image(&dist_x, &dist_y);

   // Same position, do nothing
   if ((!dist_x) && (!dist_y)){
       return REACTION_NONE;
   }

   Box::erase();

   // Update new points
   pnt[0].x += dist_x;
   pnt[0].y += dist_y;
   pnt[1].x += dist_x;
   pnt[1].y += dist_y;
 
   x_min = pnt[0].x;
   x_max = pnt[1].x;
   y_min = pnt[0].y;
   y_max = pnt[1].y;

   draw();

   basex += dist_x;
   basey += dist_y;

   return REACTION_NONE;
}

/************************************************************************
*                                                                       *
*  User just stops moving.						*
*                                                                       */
ReactionType
Box::move_done(short, short)
{
    basex = G_INIT_POS;

    // Assign minmax value.  Note that pnt[0] < pnt[1] for its x and y
    x_min = pnt[0].x;	y_min = pnt[0].y;
    x_max = pnt[1].x;	y_max = pnt[1].y;
    return REACTION_NONE;
}

/************************************************************************
*                                                                       *
*  Copy this ROI to another Gframe
*                                                                       */
Roitool *
Box::copy(Gframe *gframe)
{
    if (!gframe || !gframe->imginfo){
	return NULL;
    }
    Roitool *tool;
    tool = new Box(ul_corner_on_data.x, ul_corner_on_data.y,
		   lr_corner_on_data.x, lr_corner_on_data.y,
		   gframe);
    return tool;
}

/************************************************************************
*                                                                       *
*  Draw a box.								*
*									*/
void
Box::draw(void)
{
    if (pnt[0].x == G_INIT_POS){
	return;
    }
    calc_xyminmax();

    if (created_type != ROI_SELECTOR){
	set_clip_region(FRAME_CLIP_TO_IMAGE);
    }
    
    if (visibility != VISIBLE_NEVER && visible != FALSE){
	g_draw_rect(gdev, pnt[0].x, pnt[0].y, pnt[1].x - pnt[0].x,
		    pnt[1].y - pnt[0].y, color);
    }
    roi_set_state(ROI_STATE_EXIST);
    
    if (roi_state(ROI_STATE_MARK)){
	mark();
    }
    set_clip_region(FRAME_NO_CLIP);
}

/************************************************************************
 *                                                                      *
 *  Check whether a box is selected or not.				*
 *  There are two possiblities:                                         *
 *    	1. Positioning a cursor at either corner will RESIZE a box.	*
 *	2. Positioning a cursor inside a box will MOVE a box.		*
 *									*
 *  If a box is selected, it will set the 'acttype' variable.		*
 *  Return TRUE or FALSE.                                               *
 *                                                                      */
Flag
Box::is_selected(short x, short y)
{
#define Corner_Check(a,b,p,q,v)  com_point_in_rect(a, b, p-v, q-v, p+v, q+v)
  
  // Check if a box is on the screen
  /*
    if (!roi_state(ROI_STATE_EXIST))
    return(FALSE);
    */
  
  // ----- Check to RESIZE a box -----
  // Check for the corner of a box.  If the cursor is close enough to
  // the end points, we will assign the opposite position of the point 
  // to variables 'basex' and 'basey' because this is the way we
  // interactively create a box.
  
  if ((force_acttype != ROI_MOVE) && resizable) {
    if (Corner_Check(x, y, pnt[0].x, pnt[0].y, aperture))   {
      // Upper-left
      basex = pnt[1].x; basey = pnt[1].y;
      acttype = ROI_RESIZE;
      return(TRUE);
      
    } else if (Corner_Check(x, y, pnt[1].x, pnt[0].y, aperture))    {
      // Upper-right
      basex = pnt[0].x; basey = pnt[1].y;
      acttype = ROI_RESIZE;
      return(TRUE);
      
    } else if (Corner_Check(x, y, pnt[1].x, pnt[1].y, aperture)) {
      // Bottom-right
      basex = pnt[0].x; basey = pnt[0].y;
      acttype = ROI_RESIZE;
      return(TRUE);
      
    } else if (Corner_Check(x, y, pnt[0].x, pnt[1].y, aperture))  {
      // Bottom-left
      basex = pnt[1].x; basey = pnt[0].y;
      acttype = ROI_RESIZE;
      return(TRUE);
    }
  }
  if (com_point_in_rect(x,y, pnt[0].x,pnt[0].y, pnt[1].x,pnt[1].y))  {
    // ----- Check to MOVE a box ------
    acttype = ROI_MOVE;
    return(TRUE);
  }
  
  // ----- A Box is not selected ----
  return(FALSE);
#undef Corner_Check
}

/************************************************************************
*                                                                       *
*  Mark the box.							*
*									*/
void
Box::mark(void)
{
    if (!markable) return;

    draw_mark(pnt[0].x, pnt[0].y);
    draw_mark(pnt[1].x, pnt[0].y);
    draw_mark(pnt[0].x, pnt[1].y);
    draw_mark(pnt[1].x, pnt[1].y);
}

/************************************************************************
*                                                                       *
*  Return box's area.							*
*									*/
int
Box::area(void)
{
   return((pnt[1].x - pnt[0].x) * (pnt[1].y - pnt[0].y));
}

/************************************************************************
*									*
*  Save the current ROI box endpoints into the following format:	*
*									*
*     # <comments>                                                      *
*     X1 Y1                                                             *
*     X2 Y2                                                             *
*                                                                       *
*  where                                                                *
*     # <comments>                                                      *
*     X1 Y1                                                             *
*     X2 Y2                                                             *
*									*/
void
Box::save(ofstream &outfile)
{
   outfile << name() << "\n";
   outfile << ul_corner_on_data.x << " " << ul_corner_on_data.y << "\n";
   outfile << lr_corner_on_data.x << " " << lr_corner_on_data.y << "\n";
}

/************************************************************************
*									*
*  Load ROI box from a file which has a format described in "save" 	*
*  routine.								*
*									*/
void
Box::load(ifstream &infile)
{
    const int buflen=128;
    char buf[buflen];
    int ndata=0;
    Fpoint temp[2];

    while ((ndata != 2) && infile.getline(buf, buflen)){
	if (buf[0] == '#'){
	    continue;
	}else if (strspn(buf, "\t ") == strlen(buf)){	// Ignore blank lines
	    continue;
	}else{
	    if (sscanf(buf,"%f %f", &(temp[ndata].x), &(temp[ndata].y)) != 2){
		msgerr_print("ROI box: Missing data input");
		return;
	    }
	    ndata++;
	}
    }

    if (ndata != 2){
	msgerr_print("ROI box: Incomplete input data points");
	return;
    }

    // Create a new box ROI

    // Put it in the appropriate frames
    Gframe *gframe;
    Roitool *tool;
    int i = 1;
    for (gframe=Frame_select::get_selected_frame(i);
	 gframe;
	 gframe = Frame_select::get_selected_frame(++i))
    {
	if (gframe->imginfo){
	    tool = new Box(temp[0].x, temp[0].y, temp[1].x, temp[1].y, gframe);
	    tool->select(ROI_NOREFRESH, TRUE);
	    //gframe->display_data();
	}
    }
}

// *************************************************************************
// Returns the address of the first data pixel in this ROI.
// Initializes variables needed by NextPixel() to step through all the
// pixels in the ROI.
// *************************************************************************
float *
Box::FirstPixel()
{
    Imginfo *img = owner_frame->imginfo;
    data_width = img->GetFast();
    beg_of_row = ((float *)img->GetData() +		// First pixel in row
		  (int)(ul_corner_on_data.y) * data_width +
		  (int)(ul_corner_on_data.x));
    roi_width = ((int)(lr_corner_on_data.x) -		// # of columns in ROI
		 (int)(ul_corner_on_data.x) + 1);
    end_of_row = beg_of_row + roi_width - 1;		// Last ROI pixel in row
    roi_height = ((int)(lr_corner_on_data.y) -		// # of rows in ROI
		  (int)(ul_corner_on_data.y) + 1);
    row = 1;
    if (roi_width < 1 || roi_height < 1){
	return 0;
    }else{
	return (data = beg_of_row);
    }
}

// *************************************************************************
// After initialization by calling FirstPixel(), each call to NextPixel()
// returns the address of the next data pixel that is inside this ROI.
// Successive calls to NextPixel() step through all the data in the ROI.
// If no pixels are left, returns 0.
// *************************************************************************
float *
Box::NextPixel()
{
    if (data < end_of_row){
	return ++data;
    }else if (row < roi_height){
	row++;
	data = (beg_of_row += data_width);
	end_of_row = beg_of_row + roi_width - 1;
	return data;
    }else{
	return 0;
    }
}


// ************************************************************************
//
// ************************************************************************
void Box::update_screen_coords()
{
    pnt[0].x = x_min = data_to_xpix(ul_corner_on_data.x);
    pnt[0].y = y_min = data_to_ypix(ul_corner_on_data.y);
    pnt[1].x = x_max = data_to_xpix(lr_corner_on_data.x);
    pnt[1].y = y_max = data_to_ypix(lr_corner_on_data.y);
}

// ************************************************************************
//
// ************************************************************************
void Box::update_data_coords()
{
    ul_corner_on_data.x = xpix_to_data(pnt[0].x);
    ul_corner_on_data.y = ypix_to_data(pnt[0].y);
    lr_corner_on_data.x = xpix_to_data(pnt[1].x);
    lr_corner_on_data.y = ypix_to_data(pnt[1].y);
}

void
Box::rot90_data_coords(int datawidth)
{
    double ulx = ul_corner_on_data.x;
    double lry = lr_corner_on_data.y;
    double lrx = lr_corner_on_data.x;
    ul_corner_on_data.x = ul_corner_on_data.y;
    lr_corner_on_data.y = datawidth - ulx;
    lr_corner_on_data.x = lry;
    ul_corner_on_data.y = datawidth - lrx;
}

void
Box::flip_data_coords(int datawidth)
{
    double t = ul_corner_on_data.x;
    ul_corner_on_data.x = datawidth - lr_corner_on_data.x;
    lr_corner_on_data.x = datawidth - t;
}
