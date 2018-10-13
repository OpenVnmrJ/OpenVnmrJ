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
    return "@(#)oval.c 18.1 03/21/08 (c)1991-92 SISCO";
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
*  Routines related to oval.						*
* 									*
*									*
*  1. An oval is defined by						*
*	center (x_ctr, y_ctr): center of an oval			*
*       rectangle (rcorn[]): rectangle of an oval			*
*       radius (rx, ry): distance from center oval to the boundary of	*
*                        a rectangle.					*
*  2. An oval is created based on boundary of rectangle.  That is, 	*
*     oval should be tangent to all four rectangle boundary lines.	*
*     It is interactively created by dragging the LEFT button.		*
*  3. An oval can be resized by positioning the cursor close to its 	*
*     rectangle corners.  Then hold down the LEFT button and drag.      *
*  4. An oval can be moved by holding down the LEFT button while the  	*
*     mouse cursor position is inside an oval.                        	*
*  5. An oval can be rotated by holding down the 'CTRL' keyborad and	*
*     draging the curosr (at any position) left to right or right to	*
*     left.  Each pixel which the cursor has moved (to the left or 	*
*     right) corresponds to the rotation of 1 degree.			*
*									*
*************************************************************************/
/*
#define	DEBUG 1
*/

// The codes in this conditional statement should be taken out in BETA
// release
#define	DEBUG_BETA	1

#include <math.h>
#include "stderr.h"
#include "graphics.h"
#include "gtools.h"

#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "roitool.h"
#include "polygon.h"
#include "oval.h"
#include "common.h"
#include "msgprt.h"

#define IRINT(x) ((x) >= 0 ? (int)((x)+0.5) : (-(int)(-(x)+0.5)))

#ifdef DEBUG
#define DEBUG_PRINT     debug_print()
#else
#define DEBUG_PRINT
#endif DEBUG

// Calculate rotation
inline float rot_x(float x, float y, float cost, float sint)
{
   return(x * cost - y * sint);
}
inline float rot_y(float x, float y, float cost, float sint)
{
   return(x * sint + y * cost);
}

// Return minimum value of 2 values
inline short minvalue_2(short a, short b)
{
   return((a < b) ? a : b);
}

// Return minimum value of 4 values
inline short minvalue_4(short a, short b, short c, short d)
{
   short m = minvalue_2(a,b);
   short n = minvalue_2(c,d);
   return ((m < n) ? m : n);
}

// Return maximum value of 2 values
inline short maxvalue_2(short a, short b)
{
   return((a > b) ? a : b);
}

// Return maximum value of 4 values
inline short maxvalue_4(short a, short b, short c, short d)
{
   short m = maxvalue_2(a,b);
   short n = maxvalue_2(c,d);
   return ((m > n) ? m : n);
}

// Structure used for sorting duplicate edges
struct Xlist
{
   short x;
   Xlist *next;

   static int count;		// Number of items has been created
				// in this list

   Xlist (short xval, Xlist *nextitem) : x(xval), next(nextitem) {count++; }
   ~Xlist (void) { count--;}

   // This function will insert 'xval' into the list if the list
   // doesn't have the same value.
   void insert (short xval)	
   {
      Xlist *ptr;
      for (ptr=this; ptr; ptr=ptr->next)
      {
         if (ptr->x == xval)
	    break;
      }
   
      if (ptr == NULL)			// Insert it at the next item of
         next = new Xlist(xval, next);  //   current receiver
   }

   // Destroy all items, EXCEPT the first one
   void destroy(void)		
   {
      Xlist *ptr;
      while (ptr = next)
      {
	 next = ptr->next;
	 delete ptr;
      }
   }
};

// Initialize static class members
int Xlist::count = 0;
int Ovalpnt::count = 0;

/************************************************************************
*                                                                       *
*  Creator.                                                             *
*                                                                       */
Oval::Oval(void)
{
   created_type = ROI_OVAL;
   
   // INitialize all variables
   pnt = NULL;
   npnts = 0;
   lhead = NULL;
   yedge = NULL;
   x_min = y_min = x_max = y_max = 0;
   dist_yedge = 0;

   Ovalpnt::count = 0;

   theta_degree = 0;
   rx = ry = x_ctr = y_ctr = 0;

   state = 0;
   visibility = VISIBLE_ALWAYS;
   visible = TRUE;
   resizable = TRUE;
}

/************************************************************************
*                                                                       *
*  Creator, with initialization of location and owner.
*                                                                       */
Oval::Oval(Fpoint center, Fpoint *side, Gframe *frame)
{
   created_type = ROI_OVAL;
   
   // Initialize all variables
   pnt = NULL;
   npnts = 0;
   lhead = NULL;
   yedge = NULL;
   x_min = y_min = x_max = y_max = 0;
   dist_yedge = 0;

   Ovalpnt::count = 0;

   theta_degree = 0;
   rx = ry = x_ctr = y_ctr = 0;

   state = 0;
   visibility = VISIBLE_ALWAYS;
   visible = TRUE;
   resizable = TRUE;

   center_on_data = center;
   side_on_data[0] = side[0];
   side_on_data[1] = side[1];

   owner_frame = frame;
   Roi_routine::AppendObject(this, frame);
   update_screen_coords();
}

/************************************************************************
*                                                                       *
*  Destructor.                                                          *
*                                                                       */
Oval::~Oval(void)
{
   if (pnt)
      delete[] pnt;

   if (yedge)
   {
      Edgelist::free_ybucket(yedge, y_max-y_min+1);
      delete[] yedge;
   }
}

/************************************************************************
*                                                                       *
*  Update the size of an oval.						*
*									*/
ReactionType
Oval::create(short x, short y, short)
{
   // Check for the minimum and maximum limit of the graphics area
   if ((x < 0) || (y < 0) || (x > Gdev_Win_Width(gdev)) ||
       (y > Gdev_Win_Height(gdev)))
      return REACTION_NONE;
   
   erase();

   setup_create(x, y);	// Calculate the center position and radius of an oval

   edges_create();	// Create oval (boundary) edges

   sort_pnt_create();	// Copy Ovalpnt list to "pnt", ready to draw

   compute_normalized_rect();	// Compute normalized oval rectangle
   draw();
  return REACTION_CREATE_OBJECT;
}

/************************************************************************
*                                                                       *
*  Creation is done. 							*
*									*/
ReactionType
Oval::create_done(short, short, short)
{
   basex = G_INIT_POS;

   if (lhead == NULL)
   {
      msgerr_print("No Oval is created. Something wrong with input data");
      return REACTION_NONE;
   }

   fill_edge_holes();	// Fill the gap between edges of an oval

   erase();

   build_yedge();	// Build ybucket, each y will have only a pair
			// of x_edge.  Also, it computes
			// x_min, x_max, y_min, and y_max

   sort_pnt_create_done(); // Copies Ovalpnt edges into "pnt"
			// This function must be called AFTER
			// build_edge
   draw();

   DEBUG_PRINT;

   // Delete Ovalpnt list
   Ovalpnt *ovptr;
   while (ovptr = lhead)
   {
      lhead = ovptr->next;
      delete ovptr;
   }
#ifdef DEBUG_BETA
   if (Ovalpnt::count != 0)
   {
      STDERR("DEBUG_BETA:create_done:Ovalpnt::count is not 0");
   }
#endif DEBUG_BETA
   return REACTION_CREATE_OBJECT;
}

/************************************************************************
*                                                                       *
*  Move an oval.							*
*									*/
ReactionType
Oval::move(short x, short y)
{
   int i;
   int max_wd = Gdev_Win_Width(gdev);   // maximum width
   int max_ht = Gdev_Win_Height(gdev);  // maximum height
   short dist_x = x - basex;            // distance x
   short dist_y = y - basey;            // distance y

   keep_roi_in_image(&dist_x, &dist_y);

   if ((!dist_x) && (!dist_y)){
       // Same position, do nothing
       return REACTION_NONE;
   }

   erase();	// Erase at old position (before updating coords)

   // Update min and max
   x_min += dist_x;
   x_max += dist_x;
   y_min += dist_y;
   y_max += dist_y;

   // Update oval center
   x_ctr += dist_x;
   y_ctr += dist_y;

   // Update oval rectangle
   for (i=0; i<4; i++)
   {
      rcorn[i].x += dist_x;
      rcorn[i].y += dist_y;
   }

   // Update 'dist_yedge' which will be used to update 'yedge' list later
   dist_yedge += dist_x;

   // Update Oval edges
   for (i=0; i<npnts; i++)
   {
      pnt[i].x += dist_x;
      pnt[i].y += dist_y;
   }

   compute_normalized_rect();	// Compute normalized oval rectangle

   draw();

   basex += dist_x;
   basey += dist_y;
   return REACTION_NONE;
}

/************************************************************************
*                                                                       *
*  An oval has been moved into a new location, update yedge list.	*
*									*/
ReactionType
Oval::move_done(short, short)
{
   // Update yedge list
   Edgelist::update_ybucket(yedge, y_max-y_min+1, dist_yedge);
 
   // Need to reinitialize
   basex = G_INIT_POS;
   dist_yedge = 0;
   return REACTION_NONE;
}

/************************************************************************
*                                                                       *
*  Rotate an oval.							*
*  The angle is changed by 1 degree/pixel and limited from 0 to 360.	*
*  Rotation is determined by the x position, moving the cursor left to
*  right or right to left. The y position is ignored.
*									*/
ReactionType
Oval::rotate(short x, short)
{
   if (basex == x)	// Effect of 'x' only, 'y' has no effect
      return REACTION_NONE;

   theta_degree += (x - basex);
   theta_degree = (360 + theta_degree) % 360;

   erase();

   edges_create();		// Create oval edges

   sort_pnt_create();		// Copy Ovalpnt edges into "pnt"

   compute_normalized_rect();	// Compute normalized oval rectangle

   draw();

   // Update current position
   basex = x;
   return REACTION_NONE;
}

/************************************************************************
*                                                                       *
*  Before we create oval's edges, we have to figure out the center and	*
*  radius of an oval corresponding to its current rotation angle.	*
*									*
*  Finding the center point is straightforward.  However, in order	*
*  to find the radius (rx and ry), we have to normalize the rectangle	*
*  boundaries to zero degrees.						*
*									*/
void
Oval::setup_create(short x, short y)
{
   float cost, sint;		// cos and sin of degree

   //  Find the center of the box point
   x_ctr = (basex + x) / 2;
   y_ctr = (basey + y) / 2;

   // Normalize (to 0 degree) of rectangle.  We just need to compute
   // only one point and we get the other one by inverse value
   // Note that a rectangle is also shifted to origin (0,0)
   cost = (float)cos(-((double)theta_degree/57.29577951));
   sint = (float)sin(-((double)theta_degree/57.29577951));
   ovnorm1.x = rot_x((float)(basex-x_ctr),(float)(basey-y_ctr),cost,sint);
   ovnorm1.y = rot_y((float)(basex-x_ctr),(float)(basey-y_ctr),cost,sint);
   ovnorm2.x = -ovnorm1.x;
   ovnorm2.y = -ovnorm1.y;

   // Compute radius of an oval
   rx = (int)(fabs(ovnorm2.x - ovnorm1.x) / 2.0 + 0.5);
   ry = (int)(fabs(ovnorm2.y - ovnorm1.y) / 2.0 + 0.5);
}

/************************************************************************
*                                                                       *
*  Create edge-points of an oval with a specific rotation angle.	*
*									*
*  Oval equation is defined as                                          *
*                                                                       *
*       (x/a)**2 + (y/b)**2 = 1 ; --> whole oval             (eq. 1)    *
*    or y = b * sqrt( 1 - (x/a)**2 ) --> 2nd quadrant        (eq. 2)    *
*    or x = a * sqrt( 1 - (y/b)**2 ) --> 2nd quadrant        (eq. 3)    *
*                                                                       *
*         where a = rx (in this program code), and                      *
*               b = ry (in this program code)                           *
*                                                                       *
*  The way we create the oval is to find 1/4 of the oval and copy it    *
*  to the other 3/4 of the oval.  However, we want continous pixels	*
*  along the edge of the oval.  Hence, we use equation 2 and check the 	*
*  value of y given the step value of x.  If this y value is not 	*
*  continuous (prev_y - y > 1), we should start to find x value given 	*
*  the step value of y by using equation 3.				*
*									*/
void
Oval::edges_create(void)
{
   register int x,y;     	// x,y position of the edge point of the oval
   register float fx,fy; 	// floating point of x and y
   register float cost,sint; 	// cosine and sin theta
   register int tempval;     	// temporary value
   register Ovalpnt *ovptr;	// pointer of last item in Ovalpnt list
   register Ovalpnt *prev;	// pointer to previous last item
   register int prev_y=ry ;	// previous y position (initialized to ry)
   register double r2_f;       	// square root of radius (in floating point)

   // If the radius to be too small, don't create
   if ((rx < 2) || (ry < 2))
      return;

   // Calculate the theta value in radian, and its cos and sin
   cost = (float)cos((double)theta_degree/57.29577951);
   sint = (float)sin((double)theta_degree/57.29577951);

   if (lhead == NULL)
      prev = ovptr = lhead = new Ovalpnt;
   else
      prev = ovptr = lhead;

   // Calculate x radius square 
   r2_f = rx * rx;

   // Implementing equation 2
   for (x=0; x<=rx; x++)
   {
      fy = (float)ry * (float)sqrt(1.0 - (double)(x*x)/r2_f); // Eg. 2
      fx = (float)x;
      y = (int) (fy + 0.5);

      // check if y is (continues) skiped 2 pixel or not
      if ((prev_y - y) > 1)
	 break;

      tempval = (int)(rot_x(fx,fy,cost,sint)+0.4999);
      ovptr->x1 = x_ctr + tempval;
      ovptr->x3 = x_ctr - tempval + 1;
      tempval = (int)(rot_y(fx,fy,cost,sint)+0.4999);
      ovptr->y1 = y_ctr + tempval;
      ovptr->y3 = y_ctr - tempval + 1;
      tempval = (int)(rot_x(-fx,fy,cost,sint)+0.4999);
      ovptr->x2 = x_ctr + tempval;
      ovptr->x4 = x_ctr - tempval + 1;
      tempval = (int)(rot_y(-fx,fy,cost,sint)+0.4999);
      ovptr->y2 = y_ctr + tempval;
      ovptr->y4 = y_ctr - tempval + 1;

      prev_y = y;

      // If no previous list available, create a new list for next item
      if (ovptr->next == NULL)
	 ovptr->next = new Ovalpnt;
      prev = ovptr;
      ovptr = ovptr->next;
   }

   // Calculate y radius square 
   r2_f = ry * ry;
   
   // Implementing equation 3
   for (y=prev_y-1; y>=0; y--)
   {
      fx = (float)rx * (float)sqrt(1.0 - (double)(y*y)/r2_f);  // Eq. 3
      fy = (float)y;

      tempval = (int)(rot_x(fx,fy,cost,sint)+0.4999);
      ovptr->x1 = x_ctr + tempval;
      ovptr->x3 = x_ctr - tempval + 1;
      tempval = (int)(rot_y(fx,fy,cost,sint)+0.4999);
      ovptr->y1 = y_ctr + tempval;
      ovptr->y3 = y_ctr - tempval + 1;
      tempval = (int)(rot_x(-fx,fy,cost,sint)+0.4999);
      ovptr->x2 = x_ctr + tempval;
      ovptr->x4 = x_ctr - tempval + 1;
      tempval = (int)(rot_y(-fx,fy,cost,sint)+0.4999);
      ovptr->y2 = y_ctr + tempval ;
      ovptr->y4 = y_ctr - tempval + 1;

      // If no previous list available, create a new list for next item
      if (ovptr->next == NULL)
	 ovptr->next = new Ovalpnt;
      prev = ovptr;
      ovptr = ovptr->next;
   }

   // Terminating the last item
   prev->next = NULL;

   // Delete all the remaining list pointed by ovptr (if any)
   Ovalpnt *ptr;
   while (ptr = ovptr)
   {
      ovptr = ptr->next;
      delete ptr;
   }
}

/************************************************************************
*                                                                       *
*  Before we can display an oval, we have to put all edges in a proper	*
*  format.  That is, copy all edge points into 'pnt'.			*
*  (Note that this routine is called only DURING oval CREATION)		*
*									*/
void
Oval::sort_pnt_create(void)
{
   register int i;		// loop counter
   register Ovalpnt *ovptr;	// oval pointer

   if (pnt)
      delete[] pnt;

   // Create number of points.  Since each Ovalpnt contains 4 points, we
   // multiple it by 4
   pnt = new Gpoint [npnts = Ovalpnt::count * 4];

   i = 0;
   for (ovptr=lhead; ovptr; ovptr=ovptr->next)
   {
      pnt[i].x = ovptr->x1;	pnt[i++].y = ovptr->y1;
      pnt[i].x = ovptr->x2;	pnt[i++].y = ovptr->y2;
      pnt[i].x = ovptr->x3;	pnt[i++].y = ovptr->y3;
      pnt[i].x = ovptr->x4;	pnt[i++].y = ovptr->y4;
   }
}

/************************************************************************
*                                                                       *
*  This routine will copy the Ovalpnt edge points into a temporary	*
*  Xlist.  While copying, it eliminates all duplicate-edges.  Then, it	*
*  copies all edges from Xlist into Gpoint "pnt" which is ready for	*
*  drawing.								*
*  (Note that this routine is called when CREATION is DONE.		*
*									*/
void
Oval::sort_pnt_create_done(void)
{
   register int i;		// loop counter
   register int n;		// loop counter for "pnt"
   register int offset;		// offset value of 'y' in "pnt"
   register Xlist *xptr;	// pointer for Xlist
   Xlist **xhead; 		// temporary buffer for Xlist
   register Ovalpnt *ovptr;	// Ovalpnt pointer

   // Allocate memory and initialize xlist to zero
   xhead = new Xlist * [y_max-y_min+1];
   for (i=0; i<(y_max-y_min+1); i++)
      xhead[i] = NULL;

   // Make sure Xlist counter start at 0
   Xlist::count = 0;

   offset = y_min;
   for (ovptr=lhead; ovptr; ovptr=ovptr->next)
   {
      if (xptr = xhead[ovptr->y1-offset])
         xptr->insert(ovptr->x1); 
      else
	 xhead[ovptr->y1-offset] = new Xlist(ovptr->x1, NULL);
      if (xptr = xhead[ovptr->y2-offset])
         xptr->insert(ovptr->x2); 
      else
	 xhead[ovptr->y2-offset] = new Xlist(ovptr->x2, NULL);
      if (xptr = xhead[ovptr->y3-offset])
         xptr->insert(ovptr->x3); 
      else
	 xhead[ovptr->y3-offset] = new Xlist(ovptr->x3, NULL);
      if (xptr = xhead[ovptr->y4-offset])
         xptr->insert(ovptr->x4); 
      else
	 xhead[ovptr->y4-offset] = new Xlist(ovptr->x4, NULL);
   }

   // Delete all "pnt" points.  Now we have unduplicate edges stored
   // in Xlist.  Copy it into a newly created "pnt"
   delete[] pnt;
   pnt = new Gpoint [npnts = Xlist::count];
   
   n = 0;
   for (i=0; i<(y_max-y_min+1); i++)
   {
      for (xptr=xhead[i]; xptr; xptr=xptr->next)
      {
	 pnt[n].x = xptr->x;
	 pnt[n++].y = i + offset;
      }
   }

   // Delete Xlist
   for (i=0; i<(y_max-y_min+1); i++)
   {
      xhead[i]->destroy();
      delete xhead[i];
   }
   delete[] xhead;

#ifdef DEBUG
   if (Xlist::count != 0)
      STDERR("BUG 1 is found in Oval::duplicate_edges");

   if (n != npnts)
      STDERR("BUG 2 is found in Oval::duplicate_edges\n");
#endif DEBUG
}

/************************************************************************
*                                                                       *
*  Check for gaps between consecutive edge points.  We will fill	*
*  them in, in order to have a solid looking oval.  These holes happen	*
*  due to of "round-off" problem during floating-point calculation.	*
*									*/
void
Oval::fill_edge_holes()
{
   register Ovalpnt *ovptr;		// newely ceated Ovalpnt list
   register Ovalpnt *ptr, *prev_ptr;	// pointer list

   // Fill the holes.  A hole exists if the gap between consecutive
   // edges is gretaer than 1 pixel value.
   for (prev_ptr=lhead, ptr=prev_ptr->next; ptr;
        prev_ptr=ptr, ptr=ptr->next)
   {
      if ((abs(prev_ptr->x1 - ptr->x1) > 1) ||
          (abs(prev_ptr->y1 - ptr->y1) > 1) ||
          (abs(prev_ptr->x2 - ptr->x2) > 1) ||
          (abs(prev_ptr->y2 - ptr->y2) > 1))
      {
	 ovptr = new Ovalpnt;
         ovptr->x1 = (prev_ptr->x1 + ptr->x1)/2 ;
         ovptr->x2 = (prev_ptr->x2 + ptr->x2)/2 ;
         ovptr->x3 = (prev_ptr->x3 + ptr->x3)/2 ;
         ovptr->x4 = (prev_ptr->x4 + ptr->x4)/2 ;
         ovptr->y1 = (prev_ptr->y1 + ptr->y1)/2 ;
         ovptr->y2 = (prev_ptr->y2 + ptr->y2)/2 ;
         ovptr->y3 = (prev_ptr->y3 + ptr->y3)/2 ;
         ovptr->y4 = (prev_ptr->y4 + ptr->y4)/2 ;

	 prev_ptr->next = ovptr;
	 ovptr->next = ptr;
      } 
   }
}

/************************************************************************
 *
 *  Find lowest and highest values of all edge points.
 *									*/
void
Oval::find_minmax(void)
{
    if ( ! lhead){
	x_min = y_min = x_max = y_max = 0;
    }else{
	x_min = minvalue_4(lhead->x1, lhead->x2, lhead->x3, lhead->x4);
	y_min = minvalue_4(lhead->y1, lhead->y2, lhead->y3, lhead->y4);
	x_max = maxvalue_4(lhead->x1, lhead->x2, lhead->x3, lhead->x4);
	y_max = maxvalue_4(lhead->y1, lhead->y2, lhead->y3, lhead->y4);
	register Ovalpnt *op;
	register short temp;
	for (op=lhead->next; op; op=op->next)
	{
	    if ((temp = minvalue_4(op->x1, op->x2, op->x3, op->x4)) < x_min){
		x_min = temp;
	    }
	    if ((temp = minvalue_4(op->y1, op->y2, op->y3, op->y4)) < y_min){
		y_min = temp;
	    }
	    if ((temp = maxvalue_4(op->x1, op->x2, op->x3, op->x4)) > x_max){
		x_max = temp;
	    }
	    if ((temp = maxvalue_4(op->y1, op->y2, op->y3, op->y4)) > y_max){
		y_max = temp;
	    }
	}
    }
}

/************************************************************************
*                                                                       *
*  Insert x point into edgelist.					*
*  Note that we create a pair of items at a time since we know that	*
*  each y has only a pair of items.					*
*									*/
void
Oval::insert_edgelist(Edgelist *&edge, short x)
{
   if (edge)
   {
      if (edge->x_edge > x)
	 edge->x_edge = x;
      else if (edge->next->x_edge < x)
	 edge->next->x_edge = x;
   }
   else
   {
      edge = new Edgelist (x);
      edge->next = new Edgelist (x);
   }
}

/************************************************************************
*                                                                       *
*  Sort the 'x' value for each y value in yedge[].			*
*  Note this is totally different from the list used in polygon.	*
*  Every yedge[] (between y_min to y_max) contains only 1 pair of items.*
*									*/
void
Oval::build_yedge(void)
{
   register int i;		// loop counter
   register Ovalpnt *ovptr;	// Ovalpnt pointer
   register short offset;	// offset value
   
   // Free previus yedge
   if (yedge)
   {
      Edgelist::free_ybucket(yedge, y_max-y_min+1);
      delete[] yedge;
   }

   // Find a new smallest and largest values
   find_minmax();

   yedge = new Edgelist * [y_max-y_min+1];

   // Initialize all yedge[] to NULL
   for (i=0; i<=(y_max-y_min); i++)
      yedge[i] = NULL;

   offset = y_min;
   for (ovptr=lhead; ovptr; ovptr=ovptr->next)
   {
      insert_edgelist(yedge[ovptr->y1-offset], ovptr->x1);
      insert_edgelist(yedge[ovptr->y2-offset], ovptr->x2);
      insert_edgelist(yedge[ovptr->y3-offset], ovptr->x3);
      insert_edgelist(yedge[ovptr->y4-offset], ovptr->x4);
   }
}

/************************************************************************
*                                                                       *
*  Rotate an oval rectangle to a specific angle.  Its rotated rectangle *
*  is used for resizing and marking.					*
*									*/
void
Oval::compute_normalized_rect(void)
{
   float cost, sint;
   int tempval;

   // Rotate the rectangle to current angle and shift it to (x_ctr, y_ctr)
   // Its position (values) will be used for resizing and "marking"
   cost = (float)cos((double)theta_degree/57.29577951);
   sint = (float)sin((double)theta_degree/57.29577951);
   tempval = IRINT(rot_x(ovnorm1.x,ovnorm1.y,cost,sint));
   rcorn[0].x = x_ctr + tempval;
   rcorn[2].x = x_ctr - tempval + 1;
   tempval = IRINT(rot_y(ovnorm1.x,ovnorm1.y,cost,sint));
   rcorn[0].y = y_ctr + tempval;
   rcorn[2].y = y_ctr - tempval + 1;;
   tempval = IRINT(rot_x(ovnorm2.x,ovnorm1.y,cost,sint));
   rcorn[1].x = x_ctr + tempval;
   rcorn[3].x = x_ctr - tempval + 1;
   tempval = IRINT(rot_y(ovnorm2.x,ovnorm1.y,cost,sint));
   rcorn[1].y = y_ctr + tempval;
   rcorn[3].y = y_ctr - tempval + 1;
}

/************************************************************************
*                                                                       *
*  Draw an oval to the screen.						*
*									*/
void
Oval::draw(void)
{
    if ((pnt == NULL) || (pnt[0].x == G_INIT_POS)){
	return;
    }

    set_clip_region(FRAME_CLIP_TO_IMAGE);

    if (visibility != VISIBLE_NEVER && visible != FALSE){
	g_draw_points(gdev, pnt, npnts, color);
    }
    roi_set_state(ROI_STATE_EXIST);
    
    if (roi_state(ROI_STATE_MARK)){
	mark();
    }

    set_clip_region(FRAME_NO_CLIP);
}

/************************************************************************
*                                                                       *
*  Mark the four corners of oval's bounding rectangle.			*
*									*/
void 
Oval::mark(void)
{
    if (!markable) return;
    int i;
    for (i=0; i<4; i++){
	draw_mark(rcorn[i].x, rcorn[i].y);
    }
}

/************************************************************************
*                                                                       *
*  Check to see an oval is selected or not.                             *
*  There are two possiblities:                                          *
*       1. Positioning a cursor at either vertex will RESIZE it.        *
*       2. Positioning a cursor inside a polygon will MOVE it.          *
*                                                                       *
*  If an oval is selected, it will set the 'acttype' variable.          *
*  Return TRUE or FALSE.                                                *
*                                                                       */
Flag
Oval::is_selected(short x, short y)
{
#define Corner_Check(a,b,p,q,v)  com_point_in_rect(a, b, p-v, q-v, p+v, q+v)
   int i;
   
   // ----- Check to RESIZE an oval -----
   // Check for the corner of an oval rectangle box.  If the cursor is
   //  close enough to points, we will assign the opposite position of
   //  the point to variables 'basex' and 'basey' because this is the
   //  way we interactively create an oval.
   for (i=0; i<4; i++)
   {
      if (Corner_Check(x, y, rcorn[i].x, rcorn[i].y, aperture))
      {
         basex = rcorn[(i+2) % 4].x; basey = rcorn[(i+2) % 4].y;
         acttype = ROI_RESIZE;
         return(TRUE);
      }
   }

   // ----- Check to MOVE an oval ------
   if (com_point_in_rect(x, y, x_min, y_min, x_max, y_max) &&
       point_inside_oval(yedge[y-y_min], x))
   {
      acttype = ROI_MOVE;
      return(TRUE);
   }
   
   return(FALSE);
#undef Corner_Check
}

/************************************************************************
*									*
*   Save the current ROI oval into the following format:		*
*									*
*       # <comments>
*	oval
*       Cx Cy
*	S1x S1y
*       S2x S2y
*  where                                                                *
*       # indicates comments                                            *
*	oval is the name of the ROI type
*       Cx and Cy give the position of the center of the ellipse
*       S1x and S1y give the midpoint of one side of a bounding parallelogram
*       S2x and S2y give the midpoint of a neighboring side
*									*/
void
Oval::save(ofstream &outfile)
{
   outfile << name() << "\n";
   outfile << center_on_data.x << " " << center_on_data.y << "\n";
   outfile << side_on_data[0].x << " " << side_on_data[0].y << "\n";
   outfile << side_on_data[1].x << " " << side_on_data[1].y << "\n";
}

/************************************************************************
*									*
*  Load ROI oval from a file which has a format described in "save" 	*
*  routine.								*
*									*/
void
Oval::load(ifstream &infile)
{
    const int buflen=128;
    char buf[buflen];
    int ndata=0;
    Fpoint temp[3];

    while ((ndata != 3) && infile.getline(buf, buflen)){
	if (buf[0] == '#'){
	    continue;
	}else if (strspn(buf, "\t ") == strlen(buf)){	// Ignore blank lines
	    continue;
	}else{
	    if (sscanf(buf,"%f %f", &(temp[ndata].x), &(temp[ndata].y)) != 2){
		msgerr_print("ROI oval: Missing data input");
		return;
	    }
	    ndata++;
	}
    }

    if (ndata != 3){
	msgerr_print("ROI oval: Incomplete input data points");
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
	    tool = new Oval(temp[0], &(temp[1]), gframe);
	    tool->select(ROI_NOREFRESH, TRUE);
	    //gframe->display_data();
	}
    }
}

// ************************************************************************
//
// ************************************************************************
void
Oval::update_screen_coords()
{
    float tx;
    float ty;

    Imginfo *img = owner_frame->imginfo;

    x_ctr = data_to_xpix(center_on_data.x);
    y_ctr = data_to_ypix(center_on_data.y);

    tx = (side_on_data[1].x - center_on_data.x) * img->xscale;
    ty = (side_on_data[1].y - center_on_data.y) * img->yscale;
    ry = (int)sqrt(tx * tx + ty * ty + 0.5);

    tx = (side_on_data[0].x - center_on_data.x) * img->xscale;
    ty = (side_on_data[0].y - center_on_data.y) * img->yscale;
    rx = (int)sqrt(tx * tx + ty * ty + 0.5);

    theta_degree = IRINT(57.29577951 * atan2(ty, tx));
    if (theta_degree < 0.0) theta_degree += 360;

    ovnorm1.x = rx;
    ovnorm1.y = ry;
    ovnorm2.x = -rx;
    ovnorm2.y = -ry;

    edges_create();		// Create oval edges
    sort_pnt_create();		// Copy Ovalpnt edges into "pnt"
    compute_normalized_rect();	// Rotate and translate the "handle" positions
    fill_edge_holes();	// Fill the gap between edges of an oval
    build_yedge();	// Build ybucket, each y will have only a pair
			// of x_edge.  Also, it computes
			// x_min, x_max, y_min, and y_max
    sort_pnt_create_done(); // Copies Ovalpnt edges into "pnt"
			// This function must be called AFTER
			// build_edge
}

// ************************************************************************
//
// ************************************************************************
void
Oval::update_data_coords()
{
    Imginfo *img = owner_frame->imginfo;
    float cost = cos(theta_degree / 57.29577951);
    float sint = sin(theta_degree / 57.29577951);

    center_on_data.x = xpix_to_data(x_ctr);
    center_on_data.y = ypix_to_data(y_ctr);
    side_on_data[0].x = center_on_data.x + rx * cost / img->xscale;
    side_on_data[0].y = center_on_data.y + rx * sint / img->yscale;
    side_on_data[1].x = center_on_data.x - ry * sint / img->xscale;
    side_on_data[1].y = center_on_data.y + ry * cost / img->yscale;
}


// *************************************************************************
// Returns the address of the first data pixel in this ROI.
// Initializes variables needed by NextPixel() to step through all the
// pixels in the ROI.
// *************************************************************************
float *
Oval::FirstPixel()
{
    Imginfo *img = owner_frame->imginfo;
    data_width = img->GetFast();
    int data_xmin = (int)xpix_to_data(x_min);
    int data_xmax = (int)xpix_to_data(x_max);
    int data_ymin = (int)ypix_to_data(y_min);
    int data_ymax = (int)ypix_to_data(y_max);
    data = beg_of_row = ((float *)img->GetData() +	// First pixel in row
		  data_ymin * data_width + data_xmin);
    int roi_width = data_xmax - data_xmin + 1;		// # of columns in ROI

    end_of_row = beg_of_row + roi_width - 1;		// Last ROI pixel in row
    roi_height = data_ymax - data_ymin + 1;		// # of rows in ROI

    // Initialize current x and y positions on data
    xbeg = xnext =  data_xmin - center_on_data.x + 0.5;	// At center of column
    ynext = data_ymin - center_on_data.y + 0.5;		// At center of row

    // Initialize coefficients of ellipse equation
    float px = side_on_data[0].x - center_on_data.x;
    float py = side_on_data[0].y - center_on_data.y;
    float qx = side_on_data[1].x - center_on_data.x;
    float qy = side_on_data[1].y - center_on_data.y;
    A = py * py + qy * qy;
    B = -2.0 * (px * py + qx * qy);
    C = px * px + qx * qx;
    F = 2*px*py*qx*qy - px*px*qy*qy - py*py*qx*qx;


    row = 1;
    return NextPixel();
}

// *************************************************************************
// Evaluates the equation for a conic section centered on the origin:
//	S(x,y) = Axx + Bxy + Cyy + Dx + Ey + F
// Note that D = E = 0.
// This is zero for points on the conic section defined by A,...,F.
// For an ellipse, it is negative if (x,y) is inside and positive if outside.
// *************************************************************************
inline float
conic(float x, float y, float A, float B, float C, float F)
{
    float rtn = (A * x + B * y ) * x + C * y * y + F;
    return rtn;
}

// *************************************************************************
// After initialization by calling FirstPixel(), each call to NextPixel()
// returns the address of the next data pixel that is inside this ROI.
// Successive calls to NextPixel() step through all the data in the ROI.
// If no pixels are left, returns 0.
// *************************************************************************
float *
Oval::NextPixel()
{
    while (row <= roi_height){
	while (data <= end_of_row){
	    if (conic(xnext, ynext, A, B, C, F) < 0){
		xnext++;
		return data++;
	    }
	    data++;
	    xnext++;
	}
	row++;
	data = (beg_of_row += data_width);
	end_of_row += data_width;
	ynext++;
	xnext = xbeg;
    }
    return 0;
}



//======================================================================
//
//                      DEBUG CODES
//
//======================================================================
#ifdef DEBUG
void
Oval::debug_print(void)
{
   msginfo_print(" center(%3d,%3d) radius(%3d,%3d) degree(%d)\n",
                    x_ctr, y_ctr, rx, ry, theta_degree);
   for (Ovalpnt *ovptr=lhead; ovptr; ovptr=ovptr->next)
   {
      msginfo_print("(%3d,%3d),(%3d,%3d),(%3d,%3d),(%3d,%3d)\n",
	     ovptr->x1, ovptr->y1, ovptr->x2, ovptr->y2, ovptr->x3,
	     ovptr->y3, ovptr->x4, ovptr->y4);
   }
}
#endif DEBUG
