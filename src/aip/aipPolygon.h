/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPPOLYGON_H
#define AIPPOLYGON_H

#include <iostream>
#include <fstream>

#include "aipEdgelist.h"
#include "aipGframe.h"
#include "aipRoi.h"

class Polygon : public Roi
{
protected:
    void initEdgelist();

    bool yedgeOOD;      // yedge structure is Out Of Date
    float toprow;       // Top of polygon in data space
    float botrow;       // Bottom of polygon in data space
    Edgelist **ydataedge;   // Polygon edge list in data space
    short lastx;
    short lasty;

    // Used to implement "pixel iterator" function.
    int data_width;     // Number of data points per row
    int roi_height;     // Number of rows in ROI
    int row;            // Current row counter (0=top of ROI)
    int ipoint;         // Current point counter
    int ix;         // X-coord of current point
    int iy;         // Y-coord of current point
    float *data;        // Points to current datum
    float *beg_of_row;      // Beginning of current data row
    float *end_of_segment;  // Last datum inside current segment
    Edgelist *edge_ptr;     // Points to current edgelist entry

    LpointList *pntlist;    // Lpoint list (used during creation)

    Edgelist **yedge;       // Polygon edge list
    int nyedges;        // Number of entries in the list
    short dist_yedge;       // distance needs to update 'yedge'

    short min_segment_length;  // minimum segment length

    void build_yedge(void);
    void delete_yedge(void);
    void build_ydataedge(void);
    void delete_ydataedge(void);
    bool point_inside_polygon(Edgelist *edge, short x) {
        return(Edgelist::point_inside_xedge(edge, x));
    }

public:
    Polygon(); // Init basic polygon
    Polygon(spGframe_t gf, int x, int y); // Init degenerate polygon
    Polygon(spGframe_t gf, spCoordVector_t dpts, bool pixflag=false); // Init to points on data
    //Polygon(float, float, float, float, spGframe_t ); // Init location and owner
    ~Polygon();

    // Used after ROI creation/mod
    //void update_data_coords();

    // Used after window move/resize/zoom
    //void update_screen_coords();

    // Note that x,y is current position of the cursor
    const char *name(void) { return ("Polygon"); }
    virtual void flagUpdate() { yedgeOOD = true; }
    ReactionType create(short x, short y, short action = 0);
    ReactionType create_done();
    ReactionType resize(short x, short y) {
    return create(x, y);
    }
    ReactionType resize_done() {
    create_done();
    return REACTION_NONE;
    } 
    ReactionType move(short x, short y);    
    ReactionType move_done(short x, short y);
    int add_point(short x, short y);
    virtual Roi *copy(spGframe_t);
    virtual void draw(void);
    bool is_selected(short x, short y); // Select a polygon (move or resize)
    //int getHandle(int x, int y); // Select a handle for resize
    void save(std::ofstream &, bool pixflag=false);
    static void load(std::ifstream &, bool pixflag=false);
    //virtual Roi* recreate() {return new Polygon;}

    //virtual void some_info(int ifmoving=FALSE);

    int findDuplicateVertex(int tolerance);  // Returns index of dup vertex
    bool deleteVertex(int index);             // delete vertex[index]
    bool insertVertex(int index, int x, int y); // insert vertex[index]

    float *firstPixel();
    float *nextPixel();

    double distanceFrom(int x, int y, double far);
    int closestSide(int x, int y, double& dist);

protected:
    bool closed;
};

#endif /* AIPPOLYGON_H */

