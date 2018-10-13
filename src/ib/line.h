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

#ifndef _LINE_H
#define _LINE_H
/************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/

#include <iostream>
#include <fstream>
using namespace std;

#include "win_line_info.h"


// Its basetool "pnt" consists of 2 points which define end-points
// of a line
class Line : public Roitool
{
   private:
      void SlowLine (short x0, short y0, short x1, short y1, Ipoint *result);
      void SetPixel (short x, short y, Ipoint *result, short result_index);
      void fix_line_direction();

      Fpoint first_point_on_data;	// Posn of line end in data space
      Fpoint second_point_on_data;

      // Used to implement "iterator" function.
      float *data;
      int ipix;
      int npix;
      int opt_data_step;
      int req_data_step;
      double pix_step;
      double test;

   public:
      Line(void);
      Line(Fpoint, Fpoint);		// Initialize to given data location
      Line(float, float, float, float, Gframe *); // Init location and owner
      ~Line(void);			// Currently Not used by ROI Line

      // Used after ROI creation/mod
      void update_data_coords();

      // Used after rotating/reflecting image
      void rot90_data_coords(int datawidth);
      void flip_data_coords(int datawidth);

      // Used after window move/resize/zoom
      void update_screen_coords();

      // Note that x,y is current position of the cursor
      char *name(void) { return ("line"); }
      ReactionType create(short x, short y, short action = NULL);
      ReactionType create_done(short x, short y, short action = NULL);
      ReactionType resize(short x, short y) {
	return create(x, y);
      }
      ReactionType resize_done(short x, short y) {
	create_done(x, y);
	return REACTION_NONE;
      } 
      ReactionType move(short x, short y); 	
      ReactionType move_done(short x, short y);
      Roitool *copy(Gframe *);
      Flag is_selected(short x, short y); // Select a line (move or resize)
      void save(ofstream &);
      void load(ifstream &);
      virtual Roitool* recreate() {return new Line;}

      int length(void); 	// Return the length of a line

      virtual void some_info(int ifmoving=FALSE);

      static float *InitIterator(int width,
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
		   double *stest);	// Out: Test for optional step
      float *FirstPixel();
      float *NextPixel();
      Fpoint first_data_point() { return first_point_on_data; }
      Fpoint last_data_point() { return second_point_on_data; }
};

void extend_line(Fpoint origin,		// A point on the line
		 Fpoint slope,		// The slope of the line
		 Fpoint *clip,		// Two corners of clipping rectangle
		 Fpoint *endpts);	// Returns the two end points


#endif _LINE_H
