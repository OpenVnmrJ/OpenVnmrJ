/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

//#include <math.h>
//#include "stderr.h"
//#include "msgprt.h"
//#include "aipGraphics.h"
//#include "gtools.h"

//#include "imginfo.h"
//#include "params.h"
//#include "gframe.h"
//#include "roitool.h"
//#include "polygon.h"
#include "aipRoi.h"
#include "aipEdgelist.h"

/*
#define	DEBUG	1
*/

// This debug code will be taken out in Beta Release
//#define	DEBUG_BETA	1

#ifdef DEBUG_BETA
static void debug_edge(Edgelist **, int, int, pntPix_t, int);
static void debug_point(LpointList *);
#endif 

#ifdef DEBUG
#define DEBUG_EDGE(a,b,c,d,e) debug_edge(a,b,c,d,e)
#define DEBUG_POINT(a) debug_point(a)
#else 
#define	DEBUG_EDGE(a,b,c,e,f)
#define DEBUG_POINT(a)
#endif 

// Return the previous value.  We assume that the "current" value should
// be smaller than "max_limit".
inline int previous_value(int current, int max_limit)
{
   return(current ? (current - 1) : (max_limit - 1));
}

// Return the next value.  We assume that the "current" value should
// be smaller than "max_limit".
inline int next_value(int current, int max_limit)
{
   return((current+1) % max_limit);
}

/************************************************************************
*									*
*  Build 'yedge'.  'yedge' consists of an array of 'y' edge value of a	*
*  polygon (after an offset 'y_min').  Each 'y' (yedge[y]) edge consists*
*  of an even number pairs  of 'x' edge values in increasing order.  	*
*									*
*  For example: 'yedge' might look like the following			*
*  	y    x-pair							*
*	-    ------							*
*	0    20,45,67,201						*
*	1    21,46,67,106						*
*	2    22,23,90,105,108,203					*
*	3    23,67 							*
*	4    23,80,100,201						*
*	5    56,56,86,86						*
*	6    57,90,90,105,208,300					*
*	. . .								*
*   									*
*  Polygon edges will be						*
*  	y           x boundary						*
*       -           ----------						*
*       0+offset    20 to 67, 100 to 201				*
*       1+offset    21 to 67, 90  to 106				*
*       2+offset    22 to 23, 90  to 105, 108 to 203			*
*	3+offset    23 to 67						*
*       4+offset    23 to 80, 100 to 201				*
*    	5+offset    56 to 86,						*
*	6+offset    57 to 90, 90  to 105, 208 to 300			*
*       .....								*
*									*
*  Since building the polygon edges is very complex, several checks	*
*  are implemented to make sure we get the proper sorted list.		*
*									*/
void
Edgelist::build_ybucket(
	Edgelist **yedge,	// yedge list
        int /*nyedges*/,	// number array of yedge
	pntPix_t gpnt,		// vertices 
	int ngpnts,		// number of vertices
	int offset)		// offset of 'y' position value
{
   int i;		// loop counter
   //Lpoint *lpnt=NULL;	// pointer of a list of edge points
   LpointList pntlist;

   for (i=0; i<ngpnts-1; i++)
   {
      // Only build edges if it is NOT a horizontal straight line
      if (gpnt[i].y != gpnt[i+1].y)
      {
         build_line_points(pntlist, (short) gpnt[i].x, (short) gpnt[i].y, 
			   (short) gpnt[i+1].x, (short) gpnt[i+1].y);
         sort_edge_list(yedge, offset, pntlist);
      }
   }

   // Last segment connecting the last vertex to the first vertex
   if (gpnt[ngpnts-1].y != gpnt[0].y)
   {
      build_line_points(pntlist, (short) gpnt[ngpnts-1].x, (short) gpnt[ngpnts-1].y,
			(short) gpnt[0].x, (short) gpnt[0].y);
      sort_edge_list(yedge, offset, pntlist);
   }

   DEBUG_EDGE(yedge, nyedges, offset, gpnt, ngpnts);

   // Delete all Lpoint list entries
   pntlist.clear();

   // Get rid of one of the intersection of vertices if it is
   // NEITHER local minima nor local maxima
   check_local_minmax(yedge, offset, gpnt, ngpnts);
   DEBUG_EDGE(yedge, nyedges, offset, gpnt, ngpnts);

   // Check straight line
   check_straight_line(yedge, offset, gpnt, ngpnts);
   DEBUG_EDGE(yedge, nyedges, offset, gpnt, ngpnts);

#ifdef DEBUG_BETA
   {
       int nth;
       Edgelist *xptr;
       int y;
       int err = false;
       for (y=0; y<nyedges; y++)
       {
	   // Check for odd number of items
	   for (nth=0, xptr=yedge[y]; xptr; xptr=xptr->next, nth++);
	   if (nth & 1){
	       STDERR_1("DEBUG_BETA:build_ybucket:Odd number at (y=%d)",
			y+offset);
	       err = true;
	       break;
	   }
       }

       if (err){
	   fprintf(stderr,"%d ", y);
	   for (nth=0, xptr=yedge[y]; xptr; xptr=xptr->next, nth++){
	       fprintf(stderr," %d", xptr->x_edge);
	   }
	   fprintf(stderr,"\n");
	   debug_edge(yedge, nyedges, offset, gpnt, ngpnts);
       }
   }
#endif 
}

/************************************************************************
*									*
*  Update Edgelist items at a certain distance.				*
*									*/
void
Edgelist::update_ybucket(
	Edgelist **yedge, 	// yedge list
	int nyedges,		// number array of yedge
	int dist)		// distance to be updated
{
   register Edgelist *xptr;	// pointer to the current edgelist item
   register int i;		// loop counter
   register int rdist=dist;	// distance to be updated

   if ((yedge == NULL) || (dist == 0))
      return;
   
   for (i=0; i<nyedges; i++)
   {
      for (xptr=yedge[i]; xptr; xptr=xptr->next)
	 xptr->x_edge += rdist;
   }
}

/************************************************************************
 *									*
 *  Copy y values from 'lpnt' to 'yedge' so that each yedge[y-offset] 	*
 *  will consist of a list of x edge (x_small, x_large) values.		*
 *  									*/
void
Edgelist::sort_edge_list(Edgelist **yedge,	// yedge list
			 int offset,		// offset of 'y' position value
			 LpointList& lpnt)	// A list of edge points

{
    Edgelist *xptr;		// pointer of current (created) yedge item
    Edgelist *x2ptr, *prev_x2ptr; // temporary pointers of edgelist
    bool done;			  // done or not done

    LpointList::iterator ptptr;		// pointer to the current edge point
    for (ptptr=lpnt.begin(); ptptr!=lpnt.end(); ptptr++) {
	xptr = new Edgelist(ptptr->x);
	int y = ptptr->y;
	if (yedge[y - offset] == NULL) {
	    yedge[y - offset] = xptr;
	} else {
	    // Sort x_edge (inside the list) in increasing order
	    for (prev_x2ptr = x2ptr = yedge[y - offset], done = false;
		 x2ptr && (!done);
		 prev_x2ptr = x2ptr, x2ptr = x2ptr->next)
	    {
		if (x2ptr->x_edge > xptr->x_edge){
		    if (prev_x2ptr == x2ptr){
			// Insert at the FIRST
			xptr->next = x2ptr;
			yedge[y - offset] = xptr;
		    }else{
			// Insert at the MIDDLE
			xptr->next = x2ptr;
			prev_x2ptr->next = xptr;
		    }
		    done = true;
		}
	    }
	    if (!done){
		// Insert at the LAST
		prev_x2ptr->next = xptr;
	    }
	} // end of else (yedge[y - offset] == NULL)
    } // end of loop over lpnt list.
}

/************************************************************************
*									*
*  This function calculates the list of points defining an edge.
*  There is one point for each "y" pixel value, from py1 to py2,
*  inclusive.  The "x" value corresponding to each "y" value is the
*  pixel nearest to the line exactly joining (px1, py1) and (px2, py2).
*
*  Note that it will store all points in a buffer pointed by 'lpnt'.	*
*  If 'lpnt' is NULL, it will be created. 				*
*									*/
void
Edgelist::build_line_points(LpointList& lpnt, // Point list to be filled
			    short px1, short py1, // Start point of line
			    short px2, short py2) // End point of line
{
    int y, endy, yinc; 
    double rx = px1;
    double dx = (double)(px2 - px1) / abs(py2 - py1);

    // Empty the list
    lpnt.clear();

    // Store the first pixel
    Lpoint tmpPt(px1, py1);
    lpnt.push_back(tmpPt);

    // Store the rest of the pixels
    endy = py2;
    yinc = endy > py1 ? 1 : -1;
    for (rx = px1; tmpPt.y != endy; ) {
	rx += dx;
	tmpPt.x = (short)(rx + 0.5);
	tmpPt.y += yinc;
	lpnt.push_back(tmpPt);
    }
}

/************************************************************************
*									*
*  This routine will remove one of the 'duplicate' vertices.  However,	*
*  it is only removed if the vertex is NEITHER local minimum nor	*
*  maximum.								*
*									*
*    A		    B							*
*     _______________							*
*     \	    F        \							*
*      \    /\        \ C						*
*	\  /  \	      /	 						*
*        \/    \_____/ 							*
*        G     E     D							*
*									*
*   Points A, B, F are considered local maxima				*
*   Points D, E, G are considered local minima				*
*									*/
void
Edgelist::check_local_minmax(
	Edgelist **yedge,	// yedge list
	int offset,		// offset of 'y' position value
	pntPix_t gpnt,		// vertices 
	int num)		// number of vertices
{
   int i;	// loop counter

   // Find the local minima or maxima. If those vertices are local
   // minima or maxima, they will not be deleted from the list.
   // Otherwise, one of those vertices will be deleted in order to
   // have the even number of each "yedge[]" list.

   // FIRST item
   if (!(((gpnt[0].y >= gpnt[num-1].y) && (gpnt[0].y >= gpnt[1].y)) ||
	((gpnt[0].y <= gpnt[num-1].y) && (gpnt[0].y <= gpnt[1].y))) )
      remove_one_edge(yedge[(int)gpnt[0].y - offset], (short) gpnt[0].x);
   
   // MIDDLE items
   for (i=1; i<num-1; i++)
   {
      if (!(((gpnt[i].y >= gpnt[i-1].y) && (gpnt[i].y >= gpnt[i+1].y)) ||
	   ((gpnt[i].y <= gpnt[i-1].y) && (gpnt[i].y <= gpnt[i+1].y))) )
         remove_one_edge(yedge[(int)gpnt[i].y - offset], (short) gpnt[i].x);
   }

   // LAST item
   if (!(((gpnt[num-1].y >= gpnt[0].y) && (gpnt[num-1].y >= gpnt[num-2].y)) ||
	((gpnt[num-1].y <= gpnt[0].y) && (gpnt[num-1].y <= gpnt[num-2].y))) )
      remove_one_edge(yedge[(int)gpnt[num-1].y - offset], (short) gpnt[num-1].x);
}

/************************************************************************
*									*
*  Check for horizontal segment(s) within a polygon.  If a polygon
*  contains a horizontal segment, it causes a duplicate "x" edge in the
*  folowing cases:
*  case 1:
*		previous y value (less than y1)
*		  \							*
*		   \							*
*		    \							*
*  	    (x1,y1) *----*----*----* (x2,y2)				*
*			           /					*
*				  /					*
*				 /					*
*				/					*
*			      next y value (greater than y2)		*
*									*
*  	the previous y is less than y1 and the next y is greater than	*
*	y2								*
*									*
*  case 2:								*
*		previous y value (less than y2)
*		  \							*
*		   \							*
*		    \							*
*  	    (x2,y2) *----*---------* (x1,y1)				*
*			           /					*
*				  /					*
*				 /					*
*				/					*
*			      next y value (greater than y1)		*
*									*
*  	the previous y is less than y2 and the next y is greater than	*
*	y1								*
*									*
*  Note that there may be any number of segments between x1 and x2, all
*  with the same y.  None of the horizontal segments introduce anything
*  into the edge list, but the segments at the ends put in points at
*  (x1, y1) and (x2, y2).  Hence, we need to remove either x1 or x2 from
*  a sorted yedge list at y1 (Note that y1 = y2).  To determine which
*  one to remove, we count the items in the edge list for y1.  We remove
*  the smaller value of (x1,x2) if the remaining item will be an even
*  item in the list, and larger of (x1,x2) if it will be an odd item in
*  the list.
*									*/
void
Edgelist::check_straight_line(
	Edgelist **yedge,	// yedge list
	int offset,		// offset of 'y' position value
	pntPix_t gpnt,		// vertices 
	int ngpnts)		// number of vertices
{
    int v;		// loop counter for vertex
    int prev_v;		// previous vertex
    int last_v;		// Last vertex to look at
    int nny;		// y value of next vertex
    int prev_prev_v;	// previous of previous vertex
    int ppy;		// y value of prev_prev_v
    int y;		// y value of this v

    // Set prev_v to last gpnt that has same y as first gpnt
    y = (int) gpnt[0].y;
    for (prev_v = 0;
	 y == gpnt[prev_prev_v = previous_value(prev_v, ngpnts)].y;
	 prev_v = prev_prev_v)
    {
	if (prev_v == 1){
	    /*msgerr_print("check_straight_line(): polygon has zero height.");*/
	    return;
	}
    }
    last_v = prev_prev_v;
    ppy = (int) gpnt[prev_prev_v].y;

    // Now step through all remaining verticies, looking for horizontal runs
    for (v=0; v <= last_v; prev_v = ++v, ppy = y){
	y = (int) gpnt[v].y;
	// Go to last vertex with current y value
	while (v < last_v && (nny = (int) gpnt[v+1].y) == y){
	    v++;
	}
	// Check for extra segment
	if (prev_v != v && ((ppy - y) * (nny - y)) < 0){
	    Edgelist *xptr;	// pointer to x_edge items
	    int numth;		// nth number item
	    int small_x;	// smallest x value in this run
	    int large_x;	// largest x value in this run

	    if (gpnt[v].x > gpnt[prev_v].x)
	    {
		small_x = (int) gpnt[prev_v].x;
		large_x = (int) gpnt[v].x;
	    }
	    else
	    {
		small_x = (int) gpnt[v].x;
		large_x = (int) gpnt[prev_v].x;
	    }

	    // Remove one of the x_edge items
	    for (xptr=yedge[y-offset], numth=1; xptr; 
		 xptr=xptr->next, numth++)
	    {
		if (xptr->x_edge == small_x){
		    if (numth & 1){
			remove_one_edge(yedge[y-offset], large_x);
		    }else{
			remove_one_edge(yedge[y-offset], small_x);
		    }
		    break;
		}
	    }
	    if (!xptr){
		fprintf(stderr,"No x_edge found at x=%d, y=%d",
			small_x, gpnt[v].y);
	    }
	}
    }
}

/************************************************************************
*									*
*  Remove an item from yedge[y] list.					*
*									*/
void
Edgelist::remove_one_edge(
	Edgelist *&edge,	// yedge list at specific y position
	short x)		// point to be tested
{
   register Edgelist *xptr, *prev_xptr;	// traversed pointers

   xptr = edge;
   if (xptr->x_edge == x) 		// FIRST item
   {
      edge = xptr->next;	// Note that we change address of "edge".
      				//  That is why we pass it as a reference
      delete xptr;
      return;
   }

   for (prev_xptr=xptr, xptr=xptr->next; xptr;	// MIDDLE or LAST item
	prev_xptr=xptr, xptr=xptr->next)
   {
      if (xptr->x_edge == x)
      {
         prev_xptr->next = xptr->next;
         delete xptr;
         return;
      }
   }

#ifdef DEBUG_BETA
   STDERR_1("DEBUG_BETA:remove_one_edge:Cannot find vertex %d", (int)x);
#endif 
}

/************************************************************************
*									*
*   Check the point(x,y) if it is inside the xedge.  That is,           *
*   	check whether an x value is inside a pair of edge items.	*
*									*
*   Return false : if it is NOT inside.                     		*
*          true : if it is.                                             * 
*									*
*   Note that this routine is used when 'yedge' has been completely	*
*   sorted.								*
*                                                                       */
bool
Edgelist::point_inside_xedge(
	Edgelist *xptr,		// yedge list at specific y position
	short x)		// x point to be tested
{
   // Each y in yedge list should consists even number of x edge
   for (; xptr; xptr=xptr->next->next)
   {
      if ((x >= xptr->x_edge) && (x <= xptr->next->x_edge))
         return(true);
   }

   return (false);
}

/************************************************************************
*									*
*   Free all Edgelist in the 'yedge'.					*
*									*/
void
Edgelist::free_ybucket(
	Edgelist **yedge, 	// edge list
	int nyedges)		// number of array of yedge
{
   register Edgelist *xptr;	// list pointer
   register int i;		// loop counter

   for (i=0; i<nyedges; i++)
   {
      while (xptr = yedge[i])
      {
	 yedge[i] = xptr->next;
	 delete xptr;
      }
   }
}

//======================================================================
//
//                      DEBUG CODES
//
//======================================================================
#ifdef DEBUG_BETA
static void debug_edge(Edgelist **edge, int nyedges, int offset, 
			Gpoint *gpnt, int ngpnts)
{
   static int count=0;;
   Edgelist *ptr;
   FILE *fl;
   char buf[100];
   (void)sprintf(buf, "DEBUG_edge%d", count++);
   fl = fopen(buf, "w");
   fprintf(fl,"----------- Polygon Vertices -------\n");
   for (int j=0; j<ngpnts; j++)
      fprintf(fl, "%2d: (%3d, %3d)\n", j, gpnt[j].x, gpnt[j].y);
   fprintf(fl,"----------- Polygon Edges -------\n");
   for (int i=0; i<nyedges; i++)
   {
      ptr = edge[i] ;
      fprintf(fl,"%3d: ", i+offset);
      while (ptr)
      {
	 fprintf(fl,"%3d ", ptr->x_edge);
	 ptr = ptr->next;
      }
      fprintf(fl,"\n");
   }
   fclose(fl);
}

static void debug_point(LpointList *lpnt)
{
   fprintf(stderr," ------Polygon Vertices--------\n");
   LpointIterator plist(lpnt);
   Lpoint *pnt;
   while (pnt=plist++){
       fprintf(stderr,"( %3d, %3d)\n", pnt->x, pnt->y);
   }
}
#endif 
