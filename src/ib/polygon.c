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
    return "@(#)polygon.c 18.1 03/21/08 (c)1991-92 SISCO";
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
*  Routines related to Polygon.						*
*									*  
*  1. A polygon is defined by its vertices, it should have at least	*
*     3 vertices.							*
*  2. A polygon is created by interactively drawing its segmments	*
*     (connecting two vertices).  Its vertex position is determined by	*
*     clicking the LEFT mouse button.  Ending creating polygon is done	*
*     by clicking either the MIDDLE or RIGHT button.			*
*  3. A polygon can be resized by positioning the cursor close to its	*
*     vertices.  Then hold down the LEFT button and drag.		*
*  4. A polygon can be moved by holding down the LEFT button while the 	*
*     mouse cursor position is inside a polygon.			*
*									*
*************************************************************************/
/*
#define DEBUG 1
#define	DEBUG_FILL	1
*/


#include <math.h>     
#include "stderr.h"
#include "graphics.h"
#include "gtools.h"

#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "roitool.h"
#include "polygon.h"
#include "line.h"
#include "common.h"
#include "msgprt.h"
#include "macroexec.h"

     
#ifdef DEBUG
#define DEBUG_PRINT	debug_print()
#else
#define	DEBUG_PRINT
#endif DEBUG

#define Max(i,j) (i>j?i:j)
#define Min(i,j) (i<j?i:j)

/************************************************************************
*                                                                       *
*  FindDuplicateVertex returns the index of the first vertex that is	*
*  "near" its successor vertex [near is defined to be within the	*
*  stated tolerance].                                             	*
*									*/
int
Polygon::FindDuplicateVertex(int tolerance)
{
  for (int i = 0; i < npnts; i++ ) {
    if (abs(pnt[i].x - pnt[(i+1) % npnts].x) <= tolerance &&
	abs(pnt[i].y - pnt[(i+1) % npnts].y) <= tolerance) {
      //printf("duplicate vertex[%d] \n", i);
      return i;
    }
  }
  return -1;
}

/************************************************************************
*                                                                       *
*  InsertVertex will insert a vertex after the indicated index position	*
*  at the location [x,y].  NOTE:  You should call build_yedge at some	*
*  point after inserting a new vertex.  It is not done here because it  *
*  is a slow operation that should only be done when absolutely needed  */
int
Polygon::InsertVertex(int index, int x, int y) {

  if (index < 0 || npnts < 3) return FALSE;
  
  if (closed) {
    // Cant insert vertex after last point since this is a closed polygon
    if (index > npnts - 1) return FALSE;
    
  } else {
    // Can only insert a vertex up to the last point
    if (index > npnts - 2) return FALSE;
  }

  erase();
  Gpoint* oldpnt = pnt;
  npnts++;
  pnt = new Gpoint[npnts];

  int i;
  for (i = 0; i <= index; i++) {
    pnt[i].x = oldpnt[i].x;
    pnt[i].y = oldpnt[i].y;
  }

  for (i = npnts-1; i > index+1; i--) {
    pnt[i].x = oldpnt[i-1].x;
    pnt[i].y = oldpnt[i-1].y;
  }
  
  // Insert the new point here
  pnt[i].x = x ; pnt[i].y = y;
  
  draw();
  delete[] oldpnt;
  update_data_coords();
  return(TRUE);
}

/************************************************************************
*                                                                       *
*  DeleteVertex will delete the vertex at the specified index position	*
*									*/
int
Polygon::DeleteVertex(int index) {

  if (index < 0 || index > npnts-1 || npnts <= 3) return FALSE;

  Gpoint* oldpnt = pnt;
  Fpoint *oldfpnt = points_on_data;
  npnts--;
  pnt = new Gpoint[npnts];
  points_on_data = new Fpoint[npnts];

  int i;
  for (i = 0; i < index; i++) {
    pnt[i].x = oldpnt[i].x;
    pnt[i].y = oldpnt[i].y;
    points_on_data[i].x = oldfpnt[i].x;
    points_on_data[i].y = oldfpnt[i].y;
  }

  for (; i < npnts; i++) {
    pnt[i].x = oldpnt[i+1].x;
    pnt[i].y = oldpnt[i+1].y;
    points_on_data[i].x = oldfpnt[i+1].x;
    points_on_data[i].y = oldfpnt[i+1].y;
  }

  delete[] oldpnt;
  delete[] oldfpnt;
  //printf("npnts = %d\n", npnts);
  update_data_coords();
  return TRUE;
}

/************************************************************************
*                                                                       *
*  Make the current position to be vertex if it not not in the same	*
*  position with the previous vertex.					*
*  Note that the polygon is NOT yet completed.  This routine just 	*
*  inserts a new position into the LAST of vertices list.	        *
*									*/

ReactionType
Polygon::create_done(short x, short y, short action)
{
  
  if (roi_state(ROI_STATE_EXIST))  return REACTION_NONE;
  // Check for the minimum and maximum limit of the graphics area
  if ((x < 0) || (y < 0) || (x > Gdev_Win_Width(gdev)) ||
      (y > Gdev_Win_Height(gdev))) {
	return REACTION_NONE;
      }
  
  // The folowing statement is executed only when a user just clicks
  // the left button and releases it (no draging)
  if (pntlist->Count() == 0)   {
    pntlist->Push(new Lpoint(x, y));
    basex = x; basey = y;
    lastx = x; lasty = y;
    //g_draw_line(gdev, x, y, x, y, color);
    roi_set_state(ROI_STATE_CREATE);
    return REACTION_NONE;
  } else {
    // Do NOT allow two consecutive vertices to be the same position
    if ((abs(x - pntlist->Top()->x) < min_segment_length &&
	 abs(y - pntlist->Top()->y) < min_segment_length)) {
	   return REACTION_NONE;
	 }
  }
  // Get bounding rectangle for last line drawn.
  int xmin = pntlist->Top()->x < lastx ? pntlist->Top()->x : lastx;
  int ymin = pntlist->Top()->y < lasty ? pntlist->Top()->y : lasty;
  int xmax = pntlist->Top()->x > lastx ? pntlist->Top()->x : lastx;
  int ymax = pntlist->Top()->y > lasty ? pntlist->Top()->y : lasty;
  if (action == LOC_MOVE) {
      if (redisplay_bkg(xmin, ymin, xmax, ymax)){
	  LpointIterator plist(pntlist);
	  Lpoint *vertex;
	  Lpoint *prev;
	  prev = ++plist;
	  while (vertex=++plist){
	      g_draw_line(gdev, prev->x, prev->y, vertex->x, vertex->y, color);
	      prev = vertex;
	  }
      }else{
	  if (lastx != G_INIT_POS) {
	      g_draw_line(gdev, pntlist->Top()->x, pntlist->Top()->y,
			  lastx, lasty, color);
	  }
      }
      g_draw_line(gdev, pntlist->Top()->x, pntlist->Top()->y, x, y, color);
  }else{
      if (redisplay_bkg(xmin, ymin, xmax, ymax)){
	  pntlist->Push(new Lpoint(x,y));

	  LpointIterator plist(pntlist);
	  Lpoint *vertex;
	  Lpoint *prev;
	  prev = ++plist;
	  while (vertex=++plist){
	      g_draw_line(gdev, prev->x, prev->y, vertex->x, vertex->y, color);
	      prev = vertex;
	  }
      }else{
	  if (lastx != G_INIT_POS) {
	      g_draw_line(gdev, pntlist->Top()->x, pntlist->Top()->y,
			  lastx, lasty, color);
	      lastx = G_INIT_POS;
	  }
	  // Add this in order to obtain the correct erasing at the vertices
	  g_draw_line(gdev, x, y, x, y, color);
	  pntlist->Push(new Lpoint(x,y));
      }
      g_draw_line(gdev, pntlist->Top()->x, pntlist->Top()->y, x, y, color);
      basex = x; basey = y;
  }
  lastx = x; lasty = y;
  some_info(TRUE);
  return REACTION_NONE;
}

/************************************************************************
*                                                                       *
*  Creator.                                                             *
*                                                                       */
Polygon::Polygon(void) : Roitool()
{
  // Initialize all variables
  lastx = G_INIT_POS;
  lasty = G_INIT_POS;
  pntlist = new LpointList();
  created_type = ROI_POLYGON;
  points_on_data = NULL;
  pnt = NULL;
  npnts = 0;
  x_min = x_max = y_min = y_max =0;
  yedge = NULL;
  ydataedge = NULL;
  dist_yedge = 0;
  min_segment_length = 8;
  closed = TRUE;
  visible = TRUE;
  fixed_endpoints = FALSE;
  visibility = VISIBLE_ALWAYS;
  
  state = 0;
}

/************************************************************************
*                                                                       *
*  Creator.                                                             *
*                                                                       */
Polygon::Polygon(int npoints,
		 Fpoint *points,
		 Gframe *frame, int closeflag) : Roitool()
{
  // Initialize all variables
  lastx = G_INIT_POS;
  lasty = G_INIT_POS;
  pntlist = new LpointList();
  created_type = ROI_POLYGON;
  x_min = x_max = y_min = y_max =0;
  yedge = NULL;
  ydataedge = NULL;
  dist_yedge = 0;
  min_segment_length = 8;
  closed = TRUE;
  visible = TRUE;
  fixed_endpoints = FALSE;
  visibility = VISIBLE_ALWAYS;
  
  state = 0;

  closed = closeflag;
  npnts = npoints;
  pnt = new Gpoint[npnts];
  points_on_data = new Fpoint[npnts];
  int i;
  for (i=0; i<npnts; i++){
      points_on_data[i] = points[i];
  }
  owner_frame = frame;
  Roi_routine::AppendObject(this, frame);
  update_screen_coords();
  build_ydataedge();
  build_yedge();
}

/************************************************************************
*                                                                       *
*  Destructor.                                                          *
*                                                                       */
Polygon::~Polygon(void)
{
    if (pnt){
	delete[] pnt;
    }
    if (points_on_data){
	delete[] points_on_data;
    }
    if (pntlist){
	Lpoint *lpnt;
	while (lpnt = pntlist->Pop()){
	    delete lpnt;
	}
	delete pntlist;
    }

    if (yedge){
	// These y_min and y_max values are old values
	Edgelist::free_ybucket(yedge, nyedges);
	delete[] yedge;
	yedge = NULL;
    }
    delete_ydataedge();
}

Roitool*
Polygon::recreate()
{
  Polygon* p = new Polygon;

  p->closed = closed;
  return p;
}

/************************************************************************
 *                                                                       *
 *  This routine updates the position (to be candidate of vertex).	*
 *									*/

ReactionType
Polygon::create(short x, short y, short action)
{
  return create_done(x, y, action);
}

/************************************************************************
 *                                                                       *
 *  Polygon is completed.						*
 *  A polygon is said to exist if number vertices is greater than 2.	*
 *									*/
ReactionType
Polygon::create_completed(void)
{
  Lpoint *ptr;		// loop pointer
  int i;		// loop counter
  
  if (roi_state(ROI_STATE_EXIST))
    return REACTION_NONE;
  
  create_done(lastx, lasty);
  // Not even 1 vertex is created.  Do nothing
  if (pntlist->Count() == 0)
    return REACTION_NONE;
  
  // Initialize
  roi_clear_state(ROI_STATE_CREATE);
  
  // A polygon must contain at least 3 vertices.  If it doesn't,
  // a polygon is not created
  if (pntlist->Count() < 3)   {
    // For count == 1, we have to erase a line.  For
    // count == 2, we don't because a line doesn't appear
    // due to XOR drawing.
    if (pntlist->Count() == 1)      {
      erase_created();
      g_draw_line(gdev, basex, basey, basex, basey, color);
    }
    
    while (pntlist->Pop());	// Empty the Lpoint list
    basex = G_INIT_POS;
    return REACTION_DELETE_OBJECT;
  }
  
  // Initialize
  basex = G_INIT_POS;
  
  // Delete previous polygon (if any)
  if (pnt) {
    delete[] pnt;
  }
  
  // Alocate memory for new polygon vertices
  pnt = new Gpoint[npnts = pntlist->Count()];
  points_on_data = new Fpoint[npnts];
  
  // Copy the Lpoint list into polygon list "pnt"
  LpointIterator plist(pntlist);
  Lpoint *vertex;
  for (i=0; vertex=++plist; i++){
      pnt[i].x = vertex->x;
      pnt[i].y = vertex->y;
  }
  
  if ( ! redisplay_bkg(x_min, y_min, x_max, y_max)){
      // Erase a current polygon.  Should erase it segment by segment since
      // a polygon is being created segment by segment.  Note that we erase
      // the first vertex since it is drawn an even number of times
      g_draw_line(gdev, pnt[0].x, pnt[0].y, pnt[0].x, pnt[0].y, color);
      for (i=npnts-1; i>0; i--)  {
	  g_draw_line(gdev, pnt[i].x, pnt[i].y, pnt[i-1].x, pnt[i-1].y,  color);
	  g_draw_line(gdev, pnt[i].x, pnt[i].y, pnt[i].x, pnt[i].y, color);
      }
  }
  
  DEBUG_PRINT;
  
  while (pntlist->Pop());	// Empty the Lpoint list
  
  // Draw a completed polygon
  draw();
  
  build_yedge();

  update_data_coords();
  macroexec->record("roi_create('%s'", closed ? "polygon" : "polyline");
  for (i=0; i<npnts; i++){
      macroexec->record(", %.3f,%.3f",
			points_on_data[i].x, points_on_data[i].y);
  }
  macroexec->record(")\n");

  some_info(FALSE);
  return REACTION_CREATE_OBJECT;
}

/************************************************************************
 *                                                                      *
 *  delete_yedge frees and resets the yedge list to NULL.  Look at	*
 *  Edgelist::build_ybucket for more details.				*
 *									*/
void
Polygon::delete_yedge(void) {
  
  if (yedge)  {
    // These y_min and y_max values are old values
    Edgelist::free_ybucket(yedge, nyedges);
    delete[] yedge;
    yedge = NULL;
  }
}


/************************************************************************
 *                                                                      *
 *  delete_ydataedge frees and resets the ydataedge list to NULL.  Look at
 *  Edgelist::build_ybucket for more details.				*
 *									*/
void
Polygon::delete_ydataedge(void) {
  
  if (ydataedge)  {
    // These toprow and botrow values are old values
    Edgelist::free_ybucket(ydataedge, (int)botrow - (int)toprow + 1);
    delete[] ydataedge;
    ydataedge = NULL;
  }
}


/************************************************************************
 *                                                                       *
 *  Build 'yedge' which consists of polygon edges.  Look at 		*
 *  Edgelist::build_ybucket for more details.				*
 *									*/
void
Polygon::build_yedge(void)
{
    int i;

    // Free previous polygon ybucket.  Note that this statement should
    // come before "find_minmax" which will change the value of y_min
    // and y_max.
    delete_yedge();
    
    
    // Find the current minimum and maximum polygon boundary points
    find_minmax();
    
    // Allocate memory for a polygon ybucket 'yedge'
    nyedges = y_max - y_min + 1;
    yedge = new Edgelist* [nyedges];
    
    // Clear all allocation to zero
    for (i=0; i<nyedges; yedge[i++]=NULL);
    
    // Build polygon ybucket. That is, each yedge[] consists of a list
    // of x's values which forms polygon's edges.
    Edgelist::build_ybucket(yedge, nyedges, pnt, npnts, y_min);
}

/************************************************************************
 *                                                                       *
 *  Build 'ydataedge' which consists of polygon edges.  Look at
 *  Edgelist::build_ybucket for more details.				*
 *									*/
void
Polygon::build_ydataedge(void)
{
    int i;

    // Free previous polygon ybucket.
    delete_ydataedge();

    // Round and copy the list of data-space vertices to a "Gpoint" array
    // Find the current top and bottom polygon boundary points
    Gpoint *ipoints = new Gpoint[npnts];
    toprow = botrow = points_on_data[0].y;
    for (i=0; i<npnts; i++){
	ipoints[i].x = (int)points_on_data[i].x;
	ipoints[i].y = (int)points_on_data[i].y;
	if (points_on_data[i].y > botrow){
	    botrow = points_on_data[i].y;
	}else if (points_on_data[i].y < toprow){
	    toprow = points_on_data[i].y;
	}
    }

    // Allocate memory for a polygon ybucket 'ydataedge' and initialize to 0.
    int nrows = (int)botrow - (int)toprow + 1;
    ydataedge = new Edgelist* [nrows];
    for (i=0; i<nrows; ydataedge[i++] = NULL);

    // Build polygon ybucket. That is, each ydataedge[] consists of a list
    // of x's values which forms polygon's edges.
    Edgelist::build_ybucket(ydataedge, nrows, ipoints, npnts, (int)toprow);

    delete[] ipoints;
}

/************************************************************************
 *                                                                       *
 *  Draw part of polygon's segments. That is, a line from the last vertex*
 *  to the current position (basex, basey), and from the current 	*
 *  position the the first vertex.					*
 *  It is called during created a polygon.				*
 *									*/
void
Polygon::draw_created(void)
{
    if (pntlist->Count() > 1){
	g_draw_line(gdev, pntlist->Top()->x, pntlist->Top()->y,
		    basex, basey, color);
    }
    Lpoint *vertex = pntlist->First()->Item();
    g_draw_line(gdev, basex, basey, vertex->x, vertex->y, color);
}

/************************************************************************
 *                                                                      *
 *  Erase part of a polygon.  That is, two lines between
 *  'vertex_selected' and the neighboring two vertices.
 *  It is called when a user needs to resize a polygon.			*
 *  									*/
void
Polygon::erase_resize(void)
{

    if (bkg_pixmap){
	//
	// Erase the two segments
	//
	set_clip_region(FRAME_CLIP_TO_IMAGE);
	if (vertex_selected == 0){
	    if (closed){
		redisplay_bkg(pnt[npnts-1].x, pnt[npnts-1].y,
			      pnt[0].x, pnt[0].y);
	    }
	}else{
	    redisplay_bkg(pnt[vertex_selected-1].x, pnt[vertex_selected-1].y,
			  pnt[vertex_selected].x, pnt[vertex_selected].y);
	}
	
	if (vertex_selected == (npnts-1)){
	    if (closed) {
		redisplay_bkg(pnt[vertex_selected].x, pnt[vertex_selected].y,
			      pnt[0].x, pnt[0].y);
	    }
	}else{
	    redisplay_bkg(pnt[vertex_selected].x, pnt[vertex_selected].y,
			  pnt[vertex_selected+1].x, pnt[vertex_selected+1].y);
	}
    }else{
	//
	// XOR the two segments
	//
	draw_resize();
    }
    set_clip_region(FRAME_NO_CLIP);
}

/************************************************************************
 *                                                                      *
 *  Draw part of polygon's segment.  That is, two lines between
 *  'vertex_selected' and the neighboring two vertices.
 *  It is called when a user needs to resize a polygon.			*
 *  									*/
void
Polygon::draw_resize(void)
{
    set_clip_region(FRAME_CLIP_TO_IMAGE);

    if (bkg_pixmap){
	//
	// Redraw entire polygon
	//
	draw();
    }else{
	//
	// Redraw the two segments that moved
	//
	if (vertex_selected == 0){
	    if (closed){
		g_draw_line(gdev, pnt[npnts-1].x, pnt[npnts-1].y,
			    pnt[0].x, pnt[0].y, color);
	    }
	}else{
	    g_draw_line(gdev,
			pnt[vertex_selected-1].x, pnt[vertex_selected-1].y,
			pnt[vertex_selected].x, pnt[vertex_selected].y, color);
	}
	
	if (vertex_selected == (npnts-1)){
	    if (closed) {
		g_draw_line(gdev,
			    pnt[vertex_selected].x, pnt[vertex_selected].y,
			    pnt[0].x, pnt[0].y, color);
	    }
	}else{
	    g_draw_line(gdev,
			pnt[vertex_selected].x, pnt[vertex_selected].y,
			pnt[vertex_selected+1].x, pnt[vertex_selected+1].y,
			color);
	}
	
	g_draw_points(gdev, pnt+vertex_selected, 1, color);
	
	if (roi_state(ROI_STATE_MARK)){
	    draw_mark(pnt[vertex_selected].x, pnt[vertex_selected].y);
	}
    }
    set_clip_region(FRAME_NO_CLIP);
}

/************************************************************************
 *                                                                       *
 *  Draw a multi connected lines to form a polygon.			*
 *  It is called when a polygon is completed.				*
 *									*/
void
Polygon::draw(void)
{
    int i;
    if ((pnt == NULL) || (pnt[0].x == G_INIT_POS))
    return;

    set_clip_region(FRAME_CLIP_TO_IMAGE);

    if (visibility != VISIBLE_NEVER && visible != FALSE){
	XPoint *point = new XPoint[npnts+1];
	for (i=0; i<npnts; i++){
	    point[i].x = pnt[i].x;
	    point[i].y = pnt[i].y;
	}
	int npoints;
	if (closed){
	    npoints = npnts + 1;
	    point[npnts].x = pnt[0].x;
	    point[npnts].y = pnt[0].y;
	}else{
	    npoints = npnts;
	}
	G_Set_Color(gdev, color);
	XSetLineAttributes(gdev->xdpy, gdev->xgc,
			   0, LineSolid, CapButt, JoinBevel);
	XDrawLines(gdev->xdpy, gdev->xid, gdev->xgc, point, npoints,
		   CoordModeOrigin);
	delete[] point;
    }
    roi_set_state(ROI_STATE_EXIST);

    if (roi_state(ROI_STATE_MARK)){
	mark();
    }
    set_clip_region(FRAME_NO_CLIP);
}

/************************************************************************
 *                                                                       *
 *  Resize a polygon.  One of the polygon's has been selected.  We need	*
 *  to change the position of the vertex and redraw two lines connected	*
 *  to that point.							*
 *									*/
ReactionType
Polygon::resize(short x, short y)
{
  // Check for the minimum and maximum limit of the graphics area
  if ((x < 0) || (y < 0) || (x > Gdev_Win_Width(gdev)) ||
      (y > Gdev_Win_Height(gdev)))
    return REACTION_NONE;

  
  erase_resize();
  // Update the current vertex
  pnt[vertex_selected].x = x;
  pnt[vertex_selected].y = y;
  
  draw_resize();
  some_info(TRUE);
  return REACTION_NONE;
}

/************************************************************************
 *                                                                       *
 *  Since one the polygon's vertex has been changed, we need to rebuild	*
 *  'yedge'.								*
 *									*/
ReactionType
Polygon::resize_done(short, short)
{
  int index = FindDuplicateVertex(aperture/2);  
  if (index >= 0) {
    if (closed && index < npnts || (!closed) && index < npnts - 1) {
      erase();
      DeleteVertex(index);
      draw();
    }
  }
  build_yedge();
  
  some_info(FALSE);
  return REACTION_NONE;
}

int
IsNearLine(int tolerance, int x, int y, int x1, int y1, int x2, int y2) {

  if (x > Max(x1, x2) + tolerance || x < Min(x1, x2) - tolerance ||
      y > Max(y1, y2) + tolerance || y < Min(y1, y2) - tolerance)
  {
      return FALSE;
  }
  
  int xpa = x - x1;	// distance from x1 to x
  int ypa = y - y1;	// distance from y1 to y
  int xba = x2 - x1;	// distance from x1 to x2
  int yba = y2 - y1;	// distance from y1 to y2

  double temp = xpa * xba + ypa * yba;	// vector product

  int result = (xpa * xpa + ypa * ypa) -
    (int) (temp * temp / (double)(xba * xba + yba * yba));

  if (abs(result) < (tolerance * tolerance))   {
    return(TRUE);
  } else {
    return(FALSE);
  }
}

int
Polygon::LocateSegment(int tolerance, int x, int y) {

  int index = -1;

  if (force_acttype == ROI_MOVE || npnts < 3){
      return index;
  }
  
  for (int i = 0 ; i < npnts; i++) {
    if (IsNearLine(tolerance, x, y,
		   pnt[i].x, pnt[i].y,
		   pnt[(i+1)%npnts].x, pnt[(i+1)%npnts].y)) {
      index = i;
      break;
    }
  }

  if (closed &&  index >= npnts || (!closed) && index >= (npnts-1)) {
    index = -1;
    //cout << "No vertex found" << endl;
  } else {
    //cout << "That point is near vertex " << index << endl;
  }
  return index;
}


    
/************************************************************************
 *                                                                       *
 *  Move a polygon to a new location.					*
 *									*/
ReactionType
Polygon::move(short x, short y)
{
  int i; 
  int max_wd = Gdev_Win_Width(gdev); // maximum width
  int max_ht = Gdev_Win_Height(gdev); // maximum height
  short dist_x = x - basex;	      // distance x
  short dist_y = y - basey;	      // distance y

  keep_roi_in_image(&dist_x, &dist_y);

  // Same position, do nothing
  if ((!dist_x) && (!dist_y))
    return REACTION_NONE;
  

  // Don't draw the handles while we're moving (too slow)
  //short state = roi_state(ROI_STATE_MARK);
  //roi_clear_state(ROI_STATE_MARK);

  erase();
  
  // Update new min-max
  x_min += dist_x;
  y_min += dist_y;
  x_max += dist_x;
  y_max += dist_y;
  
  // Update 'dist_yedge' which will be used to update 'yedge' list later
  dist_yedge += dist_x;
  
  // Update polygon vertices
  for (i=0; i<npnts; i++)
    {
      pnt[i].x += dist_x;
      pnt[i].y += dist_y;
    }
  
  draw();

  // Restore handles
  //roi_set_state(state);
  
  basex += dist_x;
  basey += dist_y;
  some_info(TRUE);

  return REACTION_NONE;
} 

/************************************************************************
 *                                                                       *
 *  A polygon has been moved into a new location, update all 'yedge'	*
 *  list.								*
 *									*/
ReactionType
Polygon::move_done(short, short)
{
  Edgelist::update_ybucket(yedge, nyedges, dist_yedge);
  
  // Need to reinitialize 
  basex = G_INIT_POS;
  dist_yedge = 0;
  some_info(FALSE);
  return REACTION_NONE;
}

/************************************************************************
*                                                                       *
*  Copy this ROI to another Gframe
*                                                                       */
Roitool *
Polygon::copy(Gframe *gframe)
{
    if (!gframe || !gframe->imginfo){
	return NULL;
    }
    Roitool *tool;
    tool = new Polygon(npnts, points_on_data, gframe, closed);
    return tool;
}

/*************************************************************************
 *                                                                       *
 *  Check to see a polygon is selected or not.				 *
 *  There are two possiblities:                                          *
 *       1. Positioning a cursor at either vertex will RESIZE it. 	 *
 *       2. Positioning a cursor near a line segment will split segment. *
 *       3. Positioning a cursor inside a polygon will MOVE polygon.     *
 *                                                                       *
 *  If a polygon is selected, it will set the 'acttype' variable.        *
 *  Return TRUE or FALSE.                                                *
 *									*/
Flag
Polygon::is_selected(short x, short y)
{
#define Vertex_Check(a,b,p,q,v)  com_point_in_rect(a, b, p-v, q-v, p+v, q+v)
  int i, index;
  
  // ------ Check to RESIZE a polygon -----
  // Note that basex and basey are not used
  int dist= 0;
  int d, dx, dy;
  int isfound = FALSE;
  if (force_acttype != ROI_MOVE){
      for (i=0; i<npnts; i++){
	  if (Vertex_Check(x, y, pnt[i].x, pnt[i].y, aperture)){
	      dx = (x - pnt[i].x);
	      dy = (y - pnt[i].y);
	      d = dx * dx + dy * dy;
	      if (isfound == FALSE || d < dist){
		  isfound = TRUE;
		  dist = d;
		  vertex_selected = i;
	      }
	      acttype = ROI_RESIZE;
	  }
      }
  }
  if (isfound){
      return(TRUE);
  }
  
  // ---- Check polygon MOVE or vertex insertion -----
  index = LocateSegment(aperture/2, x, y);
  if ( index != -1) {
    // Then user wants to split this segment and insert and new vertex
    InsertVertex(index, x, y);
    acttype = ROI_RESIZE;
    vertex_selected = index+1;
    return(TRUE);
  } else if (com_point_in_rect(x, y, x_min, y_min, x_max, y_max) &&
	     point_inside_polygon(yedge[y-y_min], x))    {
    acttype = ROI_MOVE;
    return(TRUE);
  }
  
  return(FALSE);
}

/*************************************************************************
 *                                                                       *
 *  Check to see a polygon is selected or not.				 *
 *  Identical to "is_selected()" except never modifies the ROI.
 *                                                                       *
 *  If a polygon is selected, it will set the 'acttype' variable.        *
 *  Return TRUE or FALSE.                                                *
 *									*/
Flag
Polygon::is_selectable(short x, short y)
{
#define Vertex_Check(a,b,p,q,v)  com_point_in_rect(a, b, p-v, q-v, p+v, q+v)
  int i, index;
  
  // ------ Check to RESIZE a polygon -----
  // Note that basex and basey are not used
  int dist= 0;
  int d, dx, dy;
  int isfound = FALSE;
  if (force_acttype != ROI_MOVE){
      for (i=0; i<npnts; i++){
	  if (Vertex_Check(x, y, pnt[i].x, pnt[i].y, aperture)){
	      dx = (x - pnt[i].x);
	      dy = (y - pnt[i].y);
	      d = dx * dx + dy * dy;
	      if (isfound == FALSE || d < dist){
		  isfound = TRUE;
		  dist = d;
		  vertex_selected = i;
	      }
	      acttype = ROI_RESIZE;
	  }
      }
  }
  if (isfound){
      return(TRUE);
  }
  
  // ---- Check polygon MOVE or vertex insertion -----
  index = LocateSegment(aperture/2, x, y);
  if ( index != -1) {
    // Then user wants to split this segment and insert and new vertex
    // InsertVertex(index, x, y);
    acttype = ROI_RESIZE;
    vertex_selected = index+1;
    return(TRUE);
  } else if (com_point_in_rect(x, y, x_min, y_min, x_max, y_max) &&
	     point_inside_polygon(yedge[y-y_min], x))    {
    acttype = ROI_MOVE;
    return(TRUE);
  }
  
  return(FALSE);
}

/************************************************************************
 *									*
 *  Find lowest and largest value of all edge points.                    *
 *									*/
void
Polygon::find_minmax(void)
{
  register int i;
  
  x_min = x_max = pnt[0].x;
  y_min = y_max = pnt[0].y;
  
  for (i=1; i<npnts; i++)
    {
      if (x_min > pnt[i].x)
	x_min = pnt[i].x;
      if (x_max < pnt[i].x)
	x_max = pnt[i].x;
      if (y_min > pnt[i].y)
	y_min = pnt[i].y;
      if (y_max < pnt[i].y)
	y_max = pnt[i].y;
    }
}

/************************************************************************
 *									*
 *  Save the current polygon vertices into the following format:
 *									*
 *  	# <comments>                                                    *
 *	 polygon
 *       n                                                               *
 *       X1 Y1                                                           *
 *       X2 Y2                                                           *
 *       X3 Y3                                                           *
 *       .....                                                           *
 *       Xn Yn                                                           *
 *  where                                                                *
 *       # indicates comments                                            *
 *       n number of vertices, and                                       *
 *       X1 Y1    is the first vertex
 *	 . . .
 *	 Xn Yn	  is the last vertex
 *
 *  If the polygon is closed, the first and last points are the same,
 *  and n is the number of vertices + 1.
 *									*/
void
Polygon::save(ofstream &outfile)
{
    int i;
    
    outfile << name() << "\n";
    if (closed){
	outfile << npnts+1 << "\n";
    }else{
	outfile << npnts << "\n";
    }

    for (i=0; i<npnts; i++){
	outfile << points_on_data[i].x << " " << points_on_data[i].y << "\n";
    }
    if (closed){
	outfile << points_on_data[0].x << " " << points_on_data[0].y << "\n";
    }
}

/************************************************************************
*									*
*  Load ROI polygon.  File format is described in "save" routine.
*  The name line in the file ("polygon") has already been read by the
*  routine that calls this one.
*									*/
void
Polygon::load(ifstream &infile)
{
    const int buflen=128;
    char buf[buflen];
    int ndata=0;
    int isclosed;
    Fpoint *temp;

    // Read the number of points to expect
    while ( infile.getline(buf, buflen) ){
	if (buf[0] == '#'){
	    continue;
	}else if (strspn(buf, "\t ") == strlen(buf)){	// Ignore blank lines
	    continue;
	}else{
	    if (sscanf(buf,"%d", &ndata) != 1){
		msgerr_print("ROI polygon: Missing number of vertices");
		return;
	    }else{
		break;
	    }
	}
    }
    if (ndata < 3){
	msgerr_print("ROI polygon. Number vertices must be at least 3");
	return;
    }
    temp = new Fpoint[ndata];

    // Read in the data
    int i = 0;
    while ((i != ndata) && infile.getline(buf, buflen)){
	if (buf[0] == '#'){
	    continue;
	}else if (strspn(buf, "\t ") == strlen(buf)){	// Ignore blank lines
	    continue;
	}else{
	    if (sscanf(buf,"%f %f", &(temp[i].x), &(temp[i].y)) != 2){
		msgerr_print("ROI polygon: Missing data input");
		return;
	    }
	    i++;
	}
    }

    if (ndata != i){
	msgerr_print("ROI polygon: Incomplete input data points");
	return;
    }

    // Check if polygon is closed
    if (temp[0].x == temp[ndata-1].x && temp[0].y == temp[ndata-1].y){
	ndata--;
	isclosed = TRUE;
    }else{
	isclosed = FALSE;
    }

    // Put new ROIs in the appropriate frames
    Gframe *gframe;
    Roitool *tool;
    i = 1;
    for (gframe=Frame_select::get_selected_frame(i);
	 gframe;
	 gframe = Frame_select::get_selected_frame(++i))
    {
	if (gframe->imginfo){
	    tool = new Polygon(ndata, temp, gframe, isclosed);
	    tool->select(ROI_NOREFRESH, TRUE);
	    //gframe->display_data();
	}
    }
    delete [] temp;
}

// ************************************************************************
// Returns the address of the first data pixel in this ROI.
// Initializes variables needed by NextPixel() to step through all the
// pixels in the ROI.
// ************************************************************************
float *
Polygon::FirstPixel()
{
    Imginfo *img = owner_frame->imginfo;
    data_width = img->GetFast();
    if (closed){
	beg_of_row = (float *)img->GetData() + (int)toprow * data_width;
	row = 0;
	roi_height = (int)botrow - (int)toprow + 1;
	edge_ptr = ydataedge[0];
	if (!edge_ptr){
	    return NULL;
	}
	end_of_segment = beg_of_row + edge_ptr->next->x_edge;
	data = beg_of_row + edge_ptr->x_edge;
    }else{
	ivertex = 1;
	data = Line::InitIterator(data_width,
				  points_on_data[0].x,
				  points_on_data[0].y,
				  points_on_data[1].x,
				  points_on_data[1].y,
				  (float *)img->GetData(),
				  &pix_step,
				  &npix,
				  &ipix,
				  &req_data_step,
				  &opt_data_step,
				  &test);
    }
    return data;
}
    

// *************************************************************************
// After initialization by calling FirstPixel(), each call to NextPixel()
// returns the address of the next data pixel that is inside this ROI.
// Successive calls to NextPixel() step through all the data in the ROI.
// If no pixels are left, returns 0.
// *************************************************************************
float *
Polygon::NextPixel()
{
    if (closed){
	if (data < end_of_segment){
	    return ++data;
	}else if ((edge_ptr=edge_ptr->next->next) && edge_ptr->next){
	    end_of_segment = beg_of_row + edge_ptr->next->x_edge;
	    data = beg_of_row + edge_ptr->x_edge;
	    return data;
	}else if (row < roi_height - 1){
	    row++;
	    beg_of_row += data_width;
	    edge_ptr = ydataedge[row];
	    end_of_segment = beg_of_row + edge_ptr->next->x_edge;
	    data = beg_of_row + edge_ptr->x_edge;
	    return data;
	}else{
	    return 0;
	}
    }else{
	if (ipix >= npix) {
	    if (++ivertex >= npnts){
		return 0;
	    }else{
		Imginfo *img = owner_frame->imginfo;
		data = Line::InitIterator(data_width,
					  points_on_data[ivertex-1].x,
					  points_on_data[ivertex-1].y,
					  points_on_data[ivertex].x,
					  points_on_data[ivertex].y,
					  (float *)img->GetData(),
					  &pix_step,
					  &npix,
					  &ipix,
					  &req_data_step,
					  &opt_data_step,
					  &test);
		return data;
	    }
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
}

/************************************************************************
*									*
*  Calculate some info  			                        *
*									*/
void
Polygon::some_info(int moving)
{
    char str[100];		// holds text to be displayed in info window
    int i;
    int j;

    // Only polylines return info, not polygons
    // Don't do anything until we have at least 2 points
    // Don't do anything if there is no line profile display active
    // Do not do anything if we are not the primary active ROI
    if (closed
	|| npnts < 2
	|| ! Win_line_info::winlineinfo
	|| position_in_active_list(this) > 0)
    {
	return;
    }

    Gframe *gf = owner_frame;
    Imginfo *img = gf->imginfo;

    // Calculate and print out the line length
    // ASSUME THE "SPAN" (GetRatioFast/GetRatioMedium) GIVES THE DISTANCE
    // FROM ONE EDGE OF THE PICTURE TO THE OTHER, RATHER THAN THE
    // DISTANCE BETWEEN CENTERS OF THE OPPOSITE EDGE DATELS.
    // THEREFORE, WE DIVIDE BY THE NUMBER OF DATELS, RATHER THAN THE
    // NUMBER OF DATELS MINUS 1.
    float xscale = img->GetRatioFast() / img->GetFast();
    float yscale = img->GetRatioMedium() / img->GetMedium();
    float lenx;
    float lenxcm;
    float leny;
    float lenycm;
    float line_len = 0;
    float *segment_len = new float [npnts]; // Segment numbers start at 1
    for (i=1; i<npnts; i++){
	lenx = points_on_data[i].x - points_on_data[i-1].x;
	lenxcm = lenx * xscale;
	leny = points_on_data[i].y - points_on_data[i-1].y;
	lenycm = leny * yscale;
	segment_len[i] = sqrt((lenxcm*lenxcm) + (lenycm*lenycm));
	line_len += segment_len[i];
    }
    sprintf (str, "Length of polyline: %.4g cm", line_len);
    Win_line_info::show_line_length(str);

    // Print the line end coords.
    sprintf (str,"Coordinates: (%.1f, %.1f) to (%.1f, %.1f)",
	     points_on_data[0].x, points_on_data[0].y,
	     points_on_data[npnts-1].x, points_on_data[npnts-1].y);
    Win_line_info::show_coordinates(str) ;

    // Calculate line profile info
    float *ldata = (float *)img->GetData();
    int pixperrow = img->GetFast();
    float scale = xscale < yscale ? xscale : yscale; // Min cm per pixel
    scale /= 2;			// cm between points we sample
    int num_points = line_len / scale + 1;
    float *profile = new float [num_points];
    float x, y, s, cs, dx, dy, ds;
    cs = 0;			// Current location along segment (cm)
    ds = line_len / (num_points - 1); // Distance between samples
    for (i=1, j=0; i<npnts; i++){
	s = segment_len[i];
	dx = (points_on_data[i].x - points_on_data[i-1].x) * ds / s;
	dy = (points_on_data[i].y - points_on_data[i-1].y) * ds / s;
	x = points_on_data[i-1].x + cs * dx / ds;
	y = points_on_data[i-1].y + cs * dy / ds;
	for (;
	     cs <= s && j < num_points;
	     j++, x += dx, y += dy, cs += ds)
	{
	    profile[j] = *(ldata + (int)x + (int)y * pixperrow);
	}
	cs -= s;
    }
    num_points = j;		// Last point could be missed (roundoff)
    Win_line_info::set_projection_type(0);
    Win_line_info::show_projection(profile, num_points, line_len);
    delete [] profile;
    delete [] segment_len;
}

//=====================================================================
//
//                      DEBUG CODES
//
//======================================================================
#ifdef DEBUG
void
Polygon::debug_print()
{
  msginfo_print("Lpoint: number of vertices: %d\n", pntlist->Count());
  LpointIterator plist(pntlist);
  Lpoint *vertex;
  while (vertex=++plist){
    msginfo_print("\t%3d %3d\n", vertex->x, vertex->y);
  }
  
  msginfo_print("Polygon: number of vertices: %d\n", npnts);
  for (int i=0; i<npnts; i++)
    msginfo_print("\t%3d %3d\n", pnt[i].x, pnt[i].y);
}
#endif DEBUG


// ************************************************************************
//
// ************************************************************************
void
Polygon::update_screen_coords()
{
    int i;

    for (i=0; i<npnts; i++){
	pnt[i].x = data_to_xpix(points_on_data[i].x);
	pnt[i].y = data_to_ypix(points_on_data[i].y);
    }
    build_yedge();
}


// ************************************************************************
//
// ************************************************************************
void
Polygon::update_data_coords()
{
    int i;

    // Allocate new array--polygon may have grown a vertex.
    delete[] points_on_data;
    points_on_data = new Fpoint[npnts];

    for (i=0; i<npnts; i++){
	points_on_data[i].x = xpix_to_data(pnt[i].x);
	points_on_data[i].y = ypix_to_data(pnt[i].y);
    }
    if (npnts){
	// Polygon has been created
	build_ydataedge();
    }
}

void
Polygon::rot90_data_coords(int datawidth)
{
    int i;
    double t;

    for (i=0; i<npnts; i++){
	t = points_on_data[i].x;
	points_on_data[i].x = points_on_data[i].y;
	points_on_data[i].y = datawidth - t - 1;
    }
    build_ydataedge();
}

void
Polygon::flip_data_coords(int datawidth)
{
    int i;

    for (i=0; i<npnts; i++){
	points_on_data[i].x = datawidth - points_on_data[i].x - 1;
    }
    build_ydataedge();
}
