/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPPOINT_H
#define AIPPOINT_H

#include <iostream>
#include <fstream>

#include "aipGframe.h"
#include "aipRoi.h"

#define HAIR_LEN 6
#define HAIR_GAP 3

class Point : public Roi
{
public:
    Point(spGframe_t gf, int x, int y); // Construct point for pixel location
    Point(spGframe_t gf, spCoordVector_t dpts, bool pixflag=false); // Construct pt from data coord
    ~Point();

    /* Overrides */
    const char *name() { return "Point"; }
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
    double distanceFrom(int x, int y, double far);
    bool is_selected(short x, short y);
    int getHandle(int x, int y);
    void save(std::ofstream &, bool pixflag=false);
    void draw();
    void mark();
    void erase();
    Roi *copy(spGframe_t);
    float *firstPixel();
    static void clearInfo();
    void someInfo(bool isMoving=false, bool clear=false);
    /* End overrides */


private:
    int myID;                   // Unique ID # for this point.
    static int curr_id;         // ID of current info point
    static int prev_id;         // ID of previous info point
    static int id;              // Running ID number for next point created.
    static int currRoiNum;      // Corresponds to curr_id
    static int prevRoiNum;      // Corresponds to prev_id
};

#endif /* AIPPOINT_H */
