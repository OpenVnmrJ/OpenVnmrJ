/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _POLYGON_H
#define _POLYGON_H

#include <iostream>
#include <fstream>
using namespace std;

/************************************************************************
*									
*
*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/

// This class is specifically designed to build edgelist of a polygon and
// an oval. Look at edgelist.c for function descriptions.
// (Note that NOT all functions are used by oval)
class Edgelist
{
   private:
      static void sort_edge_list(Edgelist **, int, LpointList *);
      static void build_line_points(LpointList *, short, short, short, short);
      static void check_local_minmax(Edgelist **, int, Gpoint *, int);
      static void check_straight_line(Edgelist **, int, Gpoint *, int);
      static void remove_one_edge(Edgelist *&, short);

   public:
      short x_edge;
      Edgelist *next;

      Edgelist(short x)
         : x_edge(x), next(NULL) {}

      static void build_ybucket(Edgelist **, int, Gpoint *, int, int);
      static void free_ybucket(Edgelist **, int);
      static void update_ybucket(Edgelist **, int, int);
      static Flag point_inside_xedge(Edgelist *, short);
};

// Its basetool "pnt" contains all vertices.  While creating, it uses
// 'Lpoint' to temporary store the vertices since it doesn't know how
// many vertices will be created.
class Polygon : public Roitool
{
   private:
      Fpoint *points_on_data;	// "npnts" vertices of polygon in data space.
      float toprow;		// Top of polygon in data space
      float botrow;		// Bottom of polygon in data space
      Edgelist **ydataedge;	// Polygon edge list in data space
      short lastx;
      short lasty;
      
      // Used to implement "iterator" function.
      int data_width;		// Number of data points per row
      int roi_height;		// Number of rows in ROI
      int row;			// Current row counter (0=top of ROI)
      int ipoint;		// Current point counter
      int ix;			// X-coord of current point
      int iy;			// Y-coord of current point
      float *data;		// Points to current datum
      float *beg_of_row;	// Beginning of current data row
      float *end_of_segment;	// Last datum inside current segment
      Edgelist *edge_ptr;	// Points to current edgelist entry

      // Used to iterate along a polyline
      int ivertex;
      int ipix;
      int npix;
      int opt_data_step;
      int req_data_step;
      double pix_step;
      double test;

      LpointList *pntlist;	// Lpoint list (used during creation)

      Edgelist **yedge;		// Polygon edge list
      int nyedges;		// Number of entries in the list
      short dist_yedge;		// distance needs to update 'yedge'

      int vertex_selected;	// vertex selected number (for RESIZE)

      char *name(void) { return "polygon"; }
      void draw_created(void);
      void erase_created(void) { draw_created(); }
      ReactionType create_completed(void);	// Polygon is completed

      void draw_resize(void);
      void erase_resize(void);
      void find_minmax(void);		// Assign x_min,...y_max value
      void build_yedge(void);		// build 'yedge'
      void delete_yedge(void);
      void build_ydataedge(void);	// build 'ydataedge'
      void delete_ydataedge(void);
      Flag point_inside_polygon(Edgelist *edge, short x)
      {
	 return(Edgelist::point_inside_xedge(edge, x));
      }
      short min_segment_length;  // minimum segment length
      unsigned closed : 1;   // if TRUE, then this polygon is closed, else open
      unsigned fixed_endpoints : 1;  // if TRUE, then the first and last
                                     // endpoints cannot be user adjusted

   public:
      Polygon(void);
      Polygon(int npoints, Fpoint *points, Gframe *frame, int closeflag);
      ~Polygon(void);	// Currently not used by ROI Polygon

      // Used after ROI creation/mod
      void update_data_coords();

      // Used after rotating/reflecting image
      void rot90_data_coords(int datawidth);
      void flip_data_coords(int datawidth);    

      // Used after window move/resize/zoom
      void update_screen_coords();

      // Note that x,y is current position of the cursor
      ReactionType create(short x, short y, short action = NULL);
      ReactionType create_done(short x, short y, short action = NULL);
      virtual Roitool* recreate();
      ReactionType resize(short x, short y);	
      ReactionType resize_done(short x, short y);
      ReactionType move(short x, short y);	
      ReactionType move_done(short x, short y);
      Roitool *copy(Gframe *);
      void draw(void);
      Flag is_selected(short x, short y); 	// (move or resize)
      Flag is_selectable(short x, short y); 	// (move or resize)
      void save(ofstream &);
      void load(ifstream &);

      virtual void some_info(int ifmoving=FALSE);

      int FindDuplicateVertex(int tolerance);  // Returns index of dup vertex
      int DeleteVertex(int index);             // delete vertex[index]
      int InsertVertex(int index, int x, int y); // insert vertex[index]

      // LocateSegment finds the polygon segment that is at the
      // given location [x,y] within the specified tolerance.
      // Note that segment 0 is the first segment.  If the function
      // returns -1, then no segment was found.
      
      int LocateSegment(int tolerance, int x, int y);

      // Currently these functions are used to indicate
      // the polygon is completed.
      ReactionType mouse_middle(void) { return create_completed(); }
      ReactionType mouse_right(void) { return create_completed(); }

      int is_closed() { return closed; }
      void set_closed(int _closed) { closed = _closed; }

      float *FirstPixel();
      float *NextPixel();

#ifdef DEBUG
      void debug_print(void);
#endif DEBUG
};

#endif _POLYGON_H
