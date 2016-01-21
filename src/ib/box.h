/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _BOX_H
#define _BOX_H
/************************************************************************
*
*
*************************************************************************
*
*  Ramani Pichumani
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA  94538
*
*************************************************************************/

#include <iostream>
#include <fstream>

using namespace std;

#include "msgprt.h"

// Its basetool "pnt" consists of 2 points, where pnt[0] defines
// upper-left corner of a box and pnt[1] defines bottom-right corner of
// a box
class Box : public Roitool
{
  protected:
    Fpoint ul_corner_on_data;	// Upper-left corner in data space.
    Fpoint lr_corner_on_data;

  private:
    // Used to implement "iterator" function.
    float *data;
    float *beg_of_row;
    float *end_of_row;
    int data_width;
    int roi_width;
    int roi_height;
    int row;
    
  public:
    Box(void);
    Box(float, float, float, float, Gframe *);
    ~Box(void);	// Currently not used by ROI Box

    // Used after ROI creation/mod
    void update_data_coords();

    // Used after rotating/reflecting image
    virtual void rot90_data_coords(int datawidth);
    virtual void flip_data_coords(int datawidth);    

    // Used after window move/resize/zoom
    void update_screen_coords();

    // Note that x,y is current position of the cursor
    char *name(void) { return "box"; }
    virtual ReactionType create(short x, short y, short action = NULL);
    ReactionType create_done(short x, short y, short action = NULL);
    ReactionType resize(short x, short y) {
	create(x, y);
	return REACTION_NONE;
    }
    ReactionType resize_done(short x, short y) {
	create_done(x, y);
	return REACTION_NONE;
    } 
    ReactionType move(short x, short y);
    virtual ReactionType move_done(short x, short y);
    virtual Roitool *copy(Gframe *);
    virtual void draw(void);
    virtual void mark(void);
    virtual void redimension(short width, short height);
    virtual Roitool* recreate() { return new Box; }
    // Select a box (move or resize)
    virtual Flag is_selected(short x, short y);
    void save(ofstream &);
    void load(ifstream &);
    
    int area(void);			// Return area of a box

    float *FirstPixel();
    float *NextPixel();
    Fpoint ul_corner_data() { return ul_corner_on_data; }
    Fpoint lr_corner_data() { return lr_corner_on_data; }
};

class Selector : public Box {
  public:
    Selector()  :  Box() { created_type = ROI_SELECTOR; }
#ifndef LINUX
    ~Selector() { Box::~Box(); }
#endif
    virtual Roitool* recreate() { return this; }
    void histostats(int *, int, double *, double *,
		    double *, double *, double *, double *){
	msgerr_print("Selector::histostats: Please select an ROI first");
    }
};


#endif
