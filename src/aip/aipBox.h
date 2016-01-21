/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPBOX_H
#define AIPBOX_H

#include <iostream>
#include <fstream>

#include "aipGframe.h"
#include "aipRoi.h"

class Box : public Roi {
protected:

    // Used to implement "pixel iterator" function.
    float *data;
    float *beg_of_row;
    float *end_of_row;
    int data_width;
    int roi_width;
    int roi_height;
    int row;

private:

    // For creating box
    double opposite_x; // Position of opposite corner
    double opposite_y;

public:
    Box();
    Box(spGframe_t gf, int x, int y); // Init degenerate box on given pixel
    Box(spGframe_t gf, spCoordVector_t pntData, bool pixflag=false); // Init to points on data
    //Box(float, float, float, float, spGframe_t); // Init location and owner

    // Used after ROI creation/mod
    //void update_data_coords();


    // Used after window move/resize/zoom
    //void update_screen_coords();

    // Note that x,y is current position of the cursor
    const char *name(void) {
        return ("Box");
    }
    ReactionType create(short x, short y, short action = 0);
    //ReactionType create_done(short x, short y, short action = NULL);
    ReactionType resize(short x, short y) {
        return create(x, y);
    }
    //ReactionType resize_done(short x, short y) {
    //create_done(x, y);
    //return REACTION_NONE;
    //} 
    ReactionType move(short x, short y);
    ReactionType move_done(short x, short y);
    Roi *copy(spGframe_t);
    virtual void draw(void);
    virtual void mark(void);
    
    bool is_selected(short x, short y); // Select a box (move or resize)
    int getHandle(int x, int y); // Select a handle for resize
    int getHandlePoint(int i, double &x, double &y);
    void save(std::ofstream &, bool pixflag=false);
    
    //virtual Roi* recreate() {return new Box;}

    float *firstPixel();
    float *nextPixel();

    double distanceFrom(int x, int y, double far);
    void setBase(int x, int y); // Base position for moves
};

#endif /* AIPBOX_H */

