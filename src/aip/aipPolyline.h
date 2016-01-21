/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPPOLYLINE_H
#define AIPPOLYLINE_H

#include "aipPolygon.h"

class Polyline : public Polygon
{
public:
    Polyline(spGframe_t gf, int x, int y); // Init degenerate polygon
    Polyline(spGframe_t gf, spCoordVector_t dpts, bool pixflag=false); // Init to points on data

    int findDuplicateVertex(int tolerance);
    Roi *copy(spGframe_t gframe);
    void draw();
    float *firstPixel();
    float *nextPixel();
    void someInfo(bool isMoving=false, bool clear=false);
    const char *name(void) { return ("Polyline"); }

private:
    // Used to iterate along a polyline
    int ivertex;
    int ipix;
    int npix;
    int opt_data_step;
    int req_data_step;
    double pix_step;
    double test;

    static bool firstTime;
};

#endif /* AIPPOLYLINE_H */
