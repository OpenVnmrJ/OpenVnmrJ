/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _POINT_H
#define _POINT_H
/************************************************************************
*									
*  @(#)point.h 18.1 03/21/08 (c)1991-92 SISCO
*
*************************************************************************
*									
*  Doug Landau  
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/
#include <iostream>
#include <fstream>

#define HAIR_LEN	9

// Its basetool "pnt" consists of 1 point

class Point : public Roitool
{
  private:
    Fpoint loc_data;	// Position of point in data space.
    int myID;		// Unique ID # for this point.
    static int id;	// Running ID number for next point created.

  public:
    Point(void);
    Point(float x, float y, Gframe *);
    ~Point(void);			// Currently Not used by ROI Point

    // Used after ROI creation/mod
    void update_data_coords();

    // Used after rotating/reflecting image
    void rot90_data_coords(int datawidth);
    void flip_data_coords(int datawidth);

    // Used after window move/resize/zoom
    void update_screen_coords();

      // Note that x,y is current position of the cursor
      char *name(void) { return ("Point"); }
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
      void erase();
      virtual void draw(void);
      virtual void mark(void);
      Flag is_selected(short x, short y); // Select a point (move or resize)
      void save(std::ofstream &);
      void load(std::ifstream &);
      virtual Roitool* recreate() {return new Point;}

    float *FirstPixel();
      virtual void some_info(int ifmoving=FALSE);
};

#endif _POINT_H
