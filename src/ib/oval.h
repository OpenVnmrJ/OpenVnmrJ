/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _OVALTOOL_H
#define _OVALTOOL_H
/************************************************************************
*									
*  @(#)oval.h 18.1 03/21/08 (c)1991-92 SISCO 
*
*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/


// This structure is used to store oval edge-points because Xlib  doesn't 
// provide to draw an oval with capability of rotation.
// Note that every time this structure is created (by calling 'new'), it will
// increment 'count' by 1, and if it is deleted, it decrements 'count by 1
struct Ovalpnt
{
   short x1, y1;	// edge point in Quadrant 1
   short x2, y2;	// edge point in Quadrant 2
   short x3, y3;	// edge point in Quadrant 3
   short x4, y4;	// edge point in Quadrant 4
   Ovalpnt *next; 

   static int count;		// Number of item in this structures
   Ovalpnt(void) : next(NULL)  { count++; }
   ~Ovalpnt(void) { count--; }
};

// Its base 'pnt' consists number of edge-points of an oval.  During creation,
// it uses temporary buffer 'Ovalpnt' to store its edges.  When, an oval
// is created, it copies edges from 'Ovalpnt' to Gpoint 'pnt'.
class Oval : public Roitool
{
   private:
      // Define ellipse relative to the data coordinates.
      // We specify the center and the midpoints of two adjacent sides
      // of a bounding parallelogram.  There are infinitely many
      // parallelograms that will bound a given ellipse, we use the one
      // that, when projected down to the correct aspect ratio for
      // viewing the image, is a rectangle.  This representation is
      // compatible with the way ellipses are drawn with the mouse.
      Fpoint center_on_data;	// Position in data space
      Fpoint side_on_data[2];	// Midpoints of two sides

      // Parameters for implementing the iterator function
      int row;			// Row index (1...n)
      int roi_height;		// Upper bound on number of rows in ROI
      int data_width;		// Number of pixels in dataset row
      float *data;		// Pointer to current data pixel
      float *beg_of_row;	// Address of first ROI pixel in this row
      float *end_of_row;	// Address of last ROI pixel in this row
      float xbeg;		// X distance from ROI center to 1st ROI column
      float xnext;		// X distance from ROI center to current point
      float ynext;		// Y distance from ROI center to current point
      // Coefficients of ellipse eqn: Axx + Bxy + Cyy + F = 0
      float A;
      float B;
      float C;
      float F;

      short rx,ry;		// radius value of an oval
      short x_ctr, y_ctr;	// center point of an oval
      short theta_degree;	// theta value of rotation in degree

      Gfpoint ovnorm1, ovnorm2;  // Normalized oval rectangle (0 degree)
      Gpoint rcorn[4];		// a rectangle of 4 corners of an oval
	     			//  after rotation to a specific degree

      Ovalpnt *lhead;		// ovallist (used during creation)

      Edgelist **yedge;		// Oval bucket edge list
      short dist_yedge;		// distance needs to update 'yedge'

      void setup_create(short x, short y);
      void edges_create(void);
      void sort_pnt_create(void);
      void sort_pnt_create_done(void);
      void compute_normalized_rect(void);

      void fill_edge_holes(void);	// Fill a gap between edges
      void find_minmax(void);		// Assign x_min,...y_max value
      void insert_edgelist(Edgelist *&, short);
      void build_yedge(void);		// build 'yedge'
      Flag point_inside_oval(Edgelist *edge, short x)
      {
	 return(Edgelist::point_inside_xedge(edge, x));
      }
   
   public:
      Oval(void);
      Oval(Fpoint, Fpoint *, Gframe *);	// Init location and owner
      ~Oval(void);		// Current not used in ROI

      // Used after ROI creation/mod
      void update_data_coords();

      // Used after window move/resize/zoom
      void update_screen_coords();


      // Note that x,y is current position of the cursor
      char *name(void) { return "oval"; }
      ReactionType create(short x, short y, short action = NULL);
      ReactionType create_done(short x, short y, short action = NULL);
      virtual Roitool* recreate() {return new Oval;}
      ReactionType resize(short x, short y) {
	create(x, y);
	return REACTION_NONE;
      }
      ReactionType resize_done(short x, short y) {
	create_done(x, y);
	return REACTION_NONE;
	}
      ReactionType move(short x, short y);
      ReactionType move_done(short x, short y);
      ReactionType rotate(short x, short y);
      ReactionType rotate_done(short x, short y) { return create_done(x, y); }
      void draw(void);
      void mark(void);
      Flag is_selected(short x, short y);
      void save(ofstream &);
      void load(ifstream &);

      float *FirstPixel();	// Iterator functions
      float *NextPixel();

#ifdef DEBUG
      void debug_print(void);
#endif DEBUG
};

#endif _OVALTOOL_H
