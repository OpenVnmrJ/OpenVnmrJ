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
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Routines related with Line.						*
*									*
*  1. A line is defined by its two end-points.				*
*  2. A line is interactively created by dragging the LEFT button.	*
*  3. A line can be resized by positioning the cursor close to its	*
*     end-points.  The, hold down the LEFT button and drag.		*
*  4. A line can be moved by positioning the cursor close to a line.	*
*     Then hold down the LEFT button and drag.				*
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
#include "line.h"
#include "common.h"
#include "msgprt.h"
#include "primitive.h"
#include "win_line_info.h"


/************************************************************************
*									*
*  Creator.								*
*									*/
Line::Line(void) : Roitool()
{
   created_type = ROI_LINE;
   
   // Line needs two end points;
   pnt = new Gpoint [2];
   pnt[0].x = G_INIT_POS;	// Initialize
   npnts = 2;
   state = 0;
   visibility = VISIBLE_ALWAYS;
   visible = TRUE;
   resizable = TRUE;
}

/************************************************************************
*									*
*  Creator with initialization to data coords and given owner frame.
*									*/
Line::Line(float x0, float y0, float x1, float y1, Gframe *frame) : Roitool()
{
   created_type = ROI_LINE;
   
   // Line needs two end points;
   pnt = new Gpoint [2];
   pnt[0].x = G_INIT_POS;	// Initialize
   npnts = 2;
   state = 0;
   visibility = VISIBLE_ALWAYS;
   visible = TRUE;
   resizable = TRUE;

   first_point_on_data.x = x0;
   first_point_on_data.y = y0;
   second_point_on_data.x = x1;
   second_point_on_data.y = y1;
   fix_line_direction();
   owner_frame = frame;
   Roi_routine::AppendObject(this, frame);
   update_screen_coords();
}

/************************************************************************
*									*
*  Creator with initialization to given data location.
*									*/
Line::Line(Fpoint end0, Fpoint end1) : Roitool()
{
   created_type = ROI_LINE;
   
   // Line needs two end points;
   pnt = new Gpoint [2];
   pnt[0].x = G_INIT_POS;	// Initialize
   npnts = 2;
   state = 0;
   visibility = VISIBLE_NEVER;	// Note!
   visible = FALSE;		// Note!
   resizable = FALSE;		// Note!
   first_point_on_data = end0;
   second_point_on_data = end1;
   fix_line_direction();
}

/************************************************************************
*									*
*  Destructor.								*
*									*/
Line::~Line(void)
{
   delete[] pnt;
}

/************************************************************************
*									*
*  Return a length of a line (in pixels).				*
*									*/
int
Line::length(void)
{
   int x = pnt[1].x - pnt[0].x;
   int y = pnt[1].y - pnt[0].y;

   return((int)sqrt((double)(x * x + y * y)));
}

/************************************************************************
*									*
*  Update the size of a line.						*
*									*/
ReactionType
Line::create(short x, short y, short)
{
  // Check for the minimum and maximum limit of the graphics area
  if ((x < 0) || (y < 0) ||
      (x > Gdev_Win_Width(gdev)) || (y > Gdev_Win_Height(gdev))
      )
  {
      return REACTION_NONE;
  }
  
  erase();				// Erase previous line

  // Update the points
  pnt[0].x = x; 	pnt[0].y = y;
  pnt[1].x = basex; 	pnt[1].y = basey;
  
  draw();				// Draw a new line

  update_data_coords();
  fix_line_direction();
  some_info(TRUE);

  return REACTION_NONE;
}

/************************************************************************
*									*
*  Checks whether or not the line is long enough to exist.
*  Also updates some variables in the class.
*									*/
ReactionType
Line::create_done(short, short, short)
{
   basex = G_INIT_POS;
   if (length() < (G_APERTURE / 2))
   {
      erase();
      init_point();
      return REACTION_NONE;
   }
   
   // Assign minmax value for a line
   x_min = pnt[0].x;	y_min = pnt[0].y;
   x_max = pnt[1].x;	y_max = pnt[1].y;
   
   com_minmax_swap(x_min, x_max);
   com_minmax_swap(y_min, y_max);
   fix_line_direction();
   some_info();

   return REACTION_CREATE_OBJECT;
}

/************************************************************************
*									*
*  Move a line to a new location.					*
*									*/
ReactionType
Line::move(short x, short y)
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
   
   erase();	// Erase previous line

   // Update new points
   pnt[0].x += dist_x;
   pnt[0].y += dist_y;
   pnt[1].x += dist_x;
   pnt[1].y += dist_y;

   x_min += dist_x;
   x_max += dist_x;
   y_min += dist_y;
   y_max += dist_y;

   draw();	// Draw a line

   basex += dist_x;
   basey += dist_y;

   fix_line_direction();
   some_info(TRUE);

   return REACTION_NONE;
}

/************************************************************************
*									*
*  User just stops moving.						*
*									*/
ReactionType
Line::move_done(short, short)
{
   basex = G_INIT_POS;

   // Assign minmax value for a line
   x_min = pnt[0].x;	y_min = pnt[0].y;
   x_max = pnt[1].x;	y_max = pnt[1].y;
   com_minmax_swap(x_min, x_max);
   com_minmax_swap(y_min, y_max);

   fix_line_direction();
   some_info();

   return REACTION_NONE;
}

/************************************************************************
*                                                                       *
*  Copy this ROI to another Gframe
*                                                                       */
Roitool *
Line::copy(Gframe *gframe)
{
    if (!gframe || !gframe->imginfo){
	return NULL;
    }
    Roitool *tool;
    tool = new Line(first_point_on_data.x, first_point_on_data.y,
		    second_point_on_data.x, second_point_on_data.y,
		    gframe);
    return tool;
}

/************************************************************************
*									*
*  Check whether a line is selected or not.				*
*  There are two possiblities:						*
*     1. Positioning a cursor at the end points will RESIZE a line.	*
*     2. Positioning a cursor close to the line (but not at end points)	*
*        will MOVE a line.						*
*        The algorithm used to select a line is to find the 		*
*        perpendicular distance from a current position to the line.  	*
*        If it is less than "aperture", it means that a line is 	*
*        selected.							*
*  If the line is selected, if will set the 'acttype' variable.		*
*  Return TRUE or FALSE.						*
*									*/
Flag
Line::is_selected(short x, short y)
{
#define Corner_Check(a,b,p,q,v)  com_point_in_rect(a, b, p-v, q-v, p+v, q+v)

   // Check whether or not a line is on the screen
   /*
   if (!roi_state(ROI_STATE_EXIST))
      return(FALSE);
   */

   // ---- Check to RESIZE a line -----
   // Check the end points of a line.  If the cursor is close enough to
   // the end points, we will assign the opposite position of the point
   // to variables 'basex' and 'basey' because this is the way we
   // interactively create a line.
    if (force_acttype != ROI_MOVE){
	if (Corner_Check(x, y, pnt[0].x, pnt[0].y, aperture))
	{
	    basex = pnt[1].x; basey = pnt[1].y;
	    acttype = ROI_RESIZE;
	    return(TRUE);
	}
	else if (Corner_Check(x, y, pnt[1].x, pnt[1].y, aperture))
	{
	    basex = pnt[0].x; basey = pnt[0].y;
	    acttype = ROI_RESIZE;
	    return(TRUE);
	}
    }

   // ----- Check to MOVE a line -----
   // Check if x and y point is within the range of a line
   if (IsNearLine(aperture, x, y, pnt[0].x, pnt[0].y, pnt[1].x, pnt[1].y)) {
      acttype = ROI_MOVE;
      return(TRUE);
   }

   // ----- A line is not selected ------
   return(FALSE);
#undef Corner_Check
}

/************************************************************************
*									*
*  Save the current ROI line endpoints into the following format:	*
*									*
*     # <comments>
*     line
*     X1 Y1                                                             *
*     X2 Y2                                                             *
*                                                                       *
*  where                                                                *
*        # indicates comments
*	 line is the ROI "name"
*        X1 Y1 is the starting point                                    *
*        X2 Y2 is the ending point                                      *
*									*/
void
Line::save(ofstream &outfile)
{
   outfile << name() << "\n";
   outfile << first_point_on_data.x << " " << first_point_on_data.y << "\n";
   outfile << second_point_on_data.x << " " << second_point_on_data.y << "\n";
}

/************************************************************************
*									*
*  Load ROI line from a file which has a format described in "save" 	*
*  routine.								*
*									*/
void
Line::load(ifstream &infile)
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
		msgerr_print("ROI line: Missing data input");
		return;
	    }
	    ndata++;
	}
    }

    if (ndata != 2){
	msgerr_print("ROI line: Incomplete input data points");
	return;
    }

    // Put new ROI in the appropriate frames.
    Gframe *gframe;
    Roitool *tool;
    int i = 1;
    for (gframe=Frame_select::get_selected_frame(i);
	 gframe;
	 gframe = Frame_select::get_selected_frame(++i))
    {
	if (gframe->imginfo){
	    tool = new Line(temp[0].x, temp[0].y, temp[1].x, temp[1].y, gframe);
	    tool->select(ROI_NOREFRESH, TRUE);
	    //gframe->display_data();
	}
    }
}

/************************************************************************
*									*
* Once used to set/show/whatever a pixel, now puts the coordinates      *
* in the Ipoint array passed to it at the location passed to it         *
*                                                                       */
void 
Line::SetPixel (short x, short y, Ipoint *result, short result_index)
{
  result[result_index].x = x ;
  result[result_index].y = (float) y ;
}

// *************************************************************************
// Initializes variables needed to step through all the points on a line.
// Returns address of first pixel in line.
// (STATIC)
// *************************************************************************
float *
Line::InitIterator(int width,	// Width of data set
		   float x0,	// First pixel location on data
		   float y0,
		   float x1,	// Last pixel location on data
		   float y1,
		   float *dat,	// Beginning of data
		   double *ds,	// Out: Stepsize in shorter direction
		   int *np,	// Out: Number of pixels in line
		   int *ip,	// Out: Which step we are on (=1)
		   int *step0,	// Out: Increment through data always done
		   int *step1,	// Out: Increment through data sometimes done
		   double *stest)	// Out: Test for optional step
		   
{
    int first_x_pix = (int)x0;
    int first_y_pix = (int)y0;
    int nx = (int)x1 - first_x_pix;
    int ny = (int)y1 - first_y_pix;
    if (abs(nx) >= abs(ny)){
	*np = abs(nx) + 1;
	float dx = fabs(x1 - x0);
	if (dx == 0){
	    // We get this case if line has zero length.
	    dx = 1.0;
	}
	*ds = (y1 - y0) / dx;

	*stest = (y0 + *ds * (first_x_pix + 0.5 - x0));
	first_y_pix = (int)*stest;
	*stest -= first_y_pix;
	if (nx > 0){
	    *step0 = 1;
	}else{
	    *step0 = -1;
	}
	if (ny >= 0){
	    *step1 = width;
	}else{
	    *step1 = -width;
	}
    }else{
	*np = abs(ny) + 1;
	*ds = ((x1 - x0) / fabs(y1 - y0));
	*stest = (x0 + *ds * (first_y_pix + 0.5 - y0));
	first_x_pix = (int)*stest;
	*stest -= first_x_pix;

	if (ny > 0){
	    *step0 = width;
	}else{
	    *step0 = -width;
	}
	if (nx >= 0){
	    *step1 = 1;
	}else{
	    *step1 = -1;
	}
    }
    
    *ip = 1;
    if (*ds < 0){
	*ds = -*ds;
	*stest = 1 - *stest;
    }
    return dat + first_x_pix + width * first_y_pix;
}

// *************************************************************************
// Returns the address of the first data pixel in this ROI.
// Initializes variables needed by NextPixel() to step through all the
// pixels in the ROI.
// *************************************************************************
float *
Line::FirstPixel()
{
    Imginfo *img = owner_frame->imginfo;
    data = InitIterator(img->GetFast(),
			first_point_on_data.x,
			first_point_on_data.y,
			second_point_on_data.x,
			second_point_on_data.y,
			(float *)img->GetData(),
			&pix_step,
			&npix,
			&ipix,
			&req_data_step,
			&opt_data_step,
			&test);
    return data;
}

// *************************************************************************
// After initialization by calling FirstPixel(), each call to NextPixel()
// returns the address of the next data pixel that is inside this ROI.
// Successive calls to NextPixel() step through all the data in the ROI.
// If no pixels are left, returns 0.
// *************************************************************************
float *
Line::NextPixel()
{
    if (ipix >= npix){
	return 0;
    }else{
	ipix++;
	data += req_data_step;
	if ((test += pix_step) > 1){
	    data += opt_data_step;
	    test--;
	}
	return data;
    }
}


/************************************************************************
*									*
* Ramani's Bresenham implementation. 
*									*/
void
Line::SlowLine(short x1, short y1, short x2, short y2, Ipoint * result)
{
    int dx,
        dy,
        xi,
        yi,
        xsi,
        ysi,
        r,
        temp,
        runlimit,
        rbig,
        rsmall;
    short result_index = 0;	/* next element of result to be filled in */

#define PixelWidth 1

    /* printf("\nLine: x1 = %d  y1 = %d  x2 = %d  y2 = %d ",x1,y1,x2,y2); */
    if (x2 < x1 && y2 < y1) {
	temp = x1;
	x1 = x2;
	x2 = temp;
	temp = y1;
	y1 = y2;
	y2 = temp;
    }
    xsi = x1;
    ysi = y1;
    dx = x2 - x1;
    dy = y2 - y1;

    if (abs(dx) >= abs(dy)) {
	rbig = 2 * dx - 2 * dy;
	rsmall = 2 * dy;
	if (dy >= 0 && dx >= 0) {
	    xi = x1;
	    runlimit = dx + x1;
	    r = 2 * dy - dx;
	    for (; xi <= runlimit; xi += PixelWidth) {
		Line::SetPixel(xi, ysi, result, result_index++);
		if (r >= 0) {
		    ysi += PixelWidth;
		    r -= rbig;

		} else {
		    r += rsmall;
		}
	    }
	    return;
	}
	if (dy <= 0 && dx >= 0) {
	    dy = -dy;
	}
	if (dy >= 0 && dx <= 0) {
	    xi = x2;
	    dx = -dx;
	    ysi = y2;
	    runlimit = dx + x2;
	} else {
	    xi = x1;
	    runlimit = dx + x1;
	}
	r = 2 * dy - dx;
	for (; xi <= runlimit; xi += PixelWidth) {
	    Line::SetPixel(xi, ysi, result, result_index++);
	    if (r >= 0) {
		ysi -= PixelWidth;
		r -= 2 * dx - 2 * dy;
	    } else {
		r += 2 * dy;
	    }
	}
    } else {
	if (dx >= 0 && dy >= 0) {
	    r = 2 * dx - dy;
	    for (yi = y1; yi <= dy + y1; yi += PixelWidth) {
		Line::SetPixel(xsi, yi, result, result_index++);
		if (r >= 0) {
		    xsi += PixelWidth;
		    r -= 2 * dy - 2 * dx;
		} else {
		    r += 2 * dx;
		}
	    }
	    return;
	}
	if ((dx <= 0 && dy >= 0) || (dx >= 0 && dy <= 0)) {
	    if (dy < 0) {
		dy = -dy;
		xsi = x2;
	    }
	    if (dx < 0) {
		dx = -dx;
		yi = y1;
	    } else {
		yi = y2;
		xsi = x2;
	    }
	    r = 2 * dx - dy;
	    runlimit = dy + yi;
	    for (; yi <= runlimit; yi += PixelWidth) {
	Line::SetPixel(xsi, yi, result, result_index++);
		if (r >= 0) {
		    xsi -= PixelWidth;
		    r -= 2 * dy - 2 * dx;
		} else {
		    r += 2 * dx;
		}
	    }
	    return;
	}
    }
}

/************************************************************************
*									*
*  Calculate some line info  			                        *
*									*/
void
Line::some_info(int moving)
{
    char str[100];		// holds text to be displayed in info window
    int i;			// counter
    int num_points ;		// number of data points underneath this line
    float *pdata ;		// pointer to image data  (intensities)

    // Don't do anything if there is no line profile display active
    if (! Win_line_info::winlineinfo){
	return;
    }

    // Do not do anything if we are not the active ROI
    if (position_in_active_list(this) > 0){
	return;
    }

    Gframe *gf = owner_frame;
    Imginfo *img = gf->imginfo;

    //fix_line_direction();
    
    // Calculate and print out the line length
    // ASSUME THE "SPAN" (GetRatioFast/GetRatioMedium) GIVES THE DISTANCE
    // FROM ONE EDGE OF THE PICTURE TO THE OTHER, RATHER THAN THE
    // DISTANCE BETWEEN CENTERS OF THE OPPOSITE EDGE DATELS.
    // THEREFORE, WE DIVIDE BY THE NUMBER OF DATELS, RATHER THAN THE
    // NUMBER OF DATELS MINUS 1.
    float xscale = img->GetRatioFast() / img->GetFast();
    float yscale = img->GetRatioMedium() / img->GetMedium();
    float lenx = second_point_on_data.x - first_point_on_data.x;
    float lenxcm = lenx * xscale;
    float leny = second_point_on_data.y - first_point_on_data.y;
    float lenycm = leny * yscale;
    float line_len =  (double) sqrt((lenxcm*lenxcm) + (lenycm*lenycm));
    sprintf (str, "Length of line: %.4g cm", line_len);
    Win_line_info::show_line_length(str);
    
    // Print the line end coords.
    sprintf (str,"Coordinates: (%.1f, %.1f) to (%.1f, %.1f)",
	     first_point_on_data.x, first_point_on_data.y,
	     second_point_on_data.x, second_point_on_data.y);

    Win_line_info::show_coordinates(str) ;

    int x0 = (int)first_point_on_data.x;
    int x1 = (int)second_point_on_data.x;
    int y0 = (int)first_point_on_data.y;
    int y1 = (int)second_point_on_data.y;
    num_points = abs(x1 - x0);
    if ( abs(y1 - y0) > num_points ){
	num_points = abs(y1 - y0);
    }
    num_points++;

    if (gf->imginfo->type != TYPE_FLOAT){
	msgerr_print ("Data type not supported");
	return;
    }

    float *project = 0;
    if (Win_line_info::projection_type == ON_LINE){
	project = new float [num_points];
	for (pdata=FirstPixel(), i=0; pdata, i<num_points; pdata=NextPixel()){
	    project[i++] = *pdata;
	}
    }else if (! moving){	// projection_type == ACROSS_LINE
	// Increase number of points, so we don't miss any pixels.
	num_points *= 2;
	project = new float [num_points];
	// Step through "num_points" equally spaced points along the line.
	Fpoint orig;
	Fpoint slope;
	Fpoint clip[2];
	Fpoint ends[2];
	float dx = lenx / (num_points - 1);
	float dy = leny / (num_points - 1);
	orig.x = first_point_on_data.x;
	orig.y = first_point_on_data.y;
	slope.x = -lenycm / xscale;	// Slope of normal to the line
	slope.y = lenxcm / yscale;	//  in pixel units.
	clip[0].x = clip[0].y = 0;
	clip[1].x = img->GetFast() - 1;
	clip[1].y = img->GetMedium() - 1;

	float t;
	for (i=0; i<num_points; i++, orig.x += dx, orig.y += dy){
	    // Erect a normal to the line at this (orig) location that
	    // just fits in the data space.  The slope of the normal is
	    // (-lenx/leny).  The following routine returns the endpoints
	    // of the required line.
	    extend_line(orig,		// A point on the line
			slope,		// Slope of the line
			clip,		// Clipping corners
			ends);		// Returns endpoints
	    // Find the maximum value along the line.
	    Line *line = new Line(ends[0], ends[1]);
	    line->owner_frame = gf;
	    t = *(pdata = line->FirstPixel());
	    for ( ; pdata; pdata=line->NextPixel()){
		if (*pdata > t){
		    t = *pdata;
		}
	    }
	    project[i] = t;
	    delete line;
	}
    }
    if (project){
	Win_line_info::set_projection_type(-1);
	Win_line_info::show_projection(project, num_points, line_len);
	delete [] project;
    }
}      //  Line::some_info

// ************************************************************************
// Handy distance finding function for "extend_line()".
// ************************************************************************
inline float distsq(Fpoint p0, Fpoint p1)
{
    float t1 = p0.x - p1.x;
    float t2 = p0.y - p1.y;
    return (t1 * t1 + t2 * t2);
}


// ************************************************************************
// Takes a line specified by a point and slope and
// returns the end points of the line segment that is inside
// a specified clipping rectangle.
// Assumes clip[0].x < clip[1].x and clip[0].y < clip[1].y
// Assumes the line passes through the clipping rectangle.
// ************************************************************************
void
extend_line(Fpoint origin,		// A point on the line
	    Fpoint slope,		// The slope of the line
	    Fpoint *clip,		// Two corners of clipping rectangle
	    Fpoint *endpts)		// Returns the two end points
{
    int i;
    int endindex = 0;
    // Prepare to find up to four end points.  (If the line passes
    // through two opposite corners of the clip rectangle, we find
    // each end point twice.)
    Fpoint end[4];

    // First, deal with special cases of vertical and horizontal lines.
    if (slope.x == 0){
	endpts[0].x = endpts[1].x = origin.x;
	endpts[0].y = clip[0].y;
	endpts[1].y = clip[1].y;
	return;
    }

    if (slope.y == 0){
	endpts[0].y = endpts[1].y = origin.y;
	endpts[0].x = clip[0].x;
	endpts[1].x = clip[1].x;
	return;
    }

    // Now deal with the general case.
    float xcut, ycut;
    for (i=0; i<2; i++){
	xcut = origin.x + (clip[i].y - origin.y) * slope.x / slope.y;
	if (xcut >= clip[0].x && xcut <= clip[1].x){
	    // Line crosses the y clip line between the x clip lines.
	    end[endindex].x = xcut;
	    end[endindex].y = clip[i].y;
	    endindex++;
	}
	ycut = origin.y + (clip[i].x - origin.x) * slope.y / slope.x;
	if (ycut >= clip[0].y && ycut <= clip[1].y){
	    end[endindex].x = clip[i].x;
	    end[endindex].y = ycut;
	    endindex++;
	}
    }
    if (endindex == 0){
	// Line does not intersect clip rectangle.
	msgerr_print("Internal error in extend_line()");
	end[0].x = end[0].y = 0;
    }
    endpts[0] = end[0];
    if (endindex <= 1){
	endpts[1] = endpts[0];
    }else{
	// We have found more than one end point.
	// We will return the first one, and the one that is farthest from
	// that one.
	float t;
	endpts[1] = end[1];
	float d = distsq(end[0], end[1]);
	for (i=2; i<endindex; i++){
	    if ( (t=distsq(end[0], end[i])) > d){
		d = t;
		endpts[1] = end[i];
	    }
	}
    }
    return;
}


// ************************************************************************
//
// ************************************************************************
void Line::update_screen_coords()
{
    pnt[0].x = x_min = data_to_xpix(first_point_on_data.x);
    pnt[0].y = y_min = data_to_ypix(first_point_on_data.y);
    pnt[1].x = x_max = data_to_xpix(second_point_on_data.x);
    pnt[1].y = y_max = data_to_ypix(second_point_on_data.y);
}


// ************************************************************************
//
// ************************************************************************
void Line::update_data_coords()
{
    first_point_on_data.x = xpix_to_data(pnt[0].x);
    first_point_on_data.y = ypix_to_data(pnt[0].y);
    second_point_on_data.x = xpix_to_data(pnt[1].x);
    second_point_on_data.y = ypix_to_data(pnt[1].y);
}

void
Line::rot90_data_coords(int datawidth)
{
    double t;

    t = first_point_on_data.x;
    first_point_on_data.x = first_point_on_data.y;
    first_point_on_data.y = datawidth - t;
    t = second_point_on_data.x;
    second_point_on_data.x = second_point_on_data.y;
    second_point_on_data.y = datawidth - t;
}

void
Line::flip_data_coords(int datawidth)
{
    first_point_on_data.x = datawidth - first_point_on_data.x;
    second_point_on_data.x = datawidth - second_point_on_data.x;
}

void
Line::fix_line_direction()
{
    // Flip the order of the data points, if required.
    if (first_point_on_data.x > second_point_on_data.x ||
	(first_point_on_data.x == second_point_on_data.x &&
	 first_point_on_data.y > second_point_on_data.y))
    {
	// Flip the line end to end
	Fpoint t = first_point_on_data;
	first_point_on_data = second_point_on_data;
	second_point_on_data = t;
    }
}
