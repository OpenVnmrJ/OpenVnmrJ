/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPLINE_H
#define AIPLINE_H

#include <iostream>
#include <fstream>

#include "aipGframe.h"
#include "aipRoi.h"

//  This structure holds an x and an Intensity.  Used for line profiles.
typedef struct {
    short x;
    float y;
} Ipoint;

// Its basetool "pnt" consists of 2 points which define end-points
// of a line
class Line : public Roi {
private:
    void SlowLine(short x0, short y0, short x1, short y1, Ipoint *result);
    void SetPixel(short x, short y, Ipoint *result, short result_index);

    //Dpoint first_point_on_data;   // Posn of line end in data space
    //Dpoint second_point_on_data;

    // Used to implement "pixel iterator" function.
    float *data;
    int ipix;
    int npix;
    int opt_data_step;
    int req_data_step;
    double pix_step;
    double test;

    static bool firstTime;
    static bool showMIP;

public:
    const static string baseFilename;

    Line(spGframe_t gf, int x, int y); // Init both ends on given pixel
    Line(spGframe_t gf, spCoordVector_t pntData, bool pixflag=false); // Init to points on data
    Line(Dpoint_t, Dpoint_t); // Initialize to given data location
    //Line(float, float, float, float, spGframe_t); // Init location and owner

    // Used after ROI creation/mod
    //void update_data_coords();

    // Used after window move/resize/zoom
    //void update_screen_coords();

    // Note that x,y is current position of the cursor
    const char *name(void) {
        return ("Line");
    }
    ReactionType create(short x, short y, short action = (ReactionType)NULL);
    //ReactionType create_done(short x, short y, short action = (ReactionType)NULL);
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
    bool is_selected(short x, short y); // Select a line (move or resize)
    void save(std::ofstream &, bool pixflag=false);
    //virtual Roi* recreate() {return new Line;}

    double length(void); // Return the length of a line

    virtual void someInfo(bool isMoving=false, bool clear=false);
    static void clearInfo();

    static void drawProfile(float *data, int npts, double len, string ylabel);
    static float *InitIterator(int width, int height, float x0, // First pixel location on data
            float y0, float x1, // Last pixel location on data
            float y1, float *dat, // Beginning of data
            double *ds, // Out: Stepsize in shorter direction
            int *np, // Out: Number of pixels in line
            int *ip, // Out: Which step we are on (=1)
            int *step0, // Out: Increment through data always done
            int *step1, // Out: Increment through data sometimes done
            double *stest); // Out: Test for optional step
    float *firstPixel();
    float *nextPixel();
    //Dpoint first_data_point() { return pntData[0]; }
    //Dpoint last_data_point() { return pntData[1]; }

    double distanceFrom(int x, int y, double far);
};

void extend_line(Dpoint origin, // A point on the line
        Dpoint slope, // The slope of the line
        Dpoint *clip, // Two corners of clipping rectangle
        Dpoint *endpts); // Returns the two end points

#endif /* AIPLINE_H */

