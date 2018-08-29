/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPSTRUCTS_H
#define AIPSTRUCTS_H

#include "aipCStructs.h"
#include "aipVsInfo.h"

/*for Linux*/
#ifndef uchar_t
typedef u_char uchar_t;
#endif

#ifndef AIPPOINTS
#define AIPPOINTS

// An integral location in the plane
// NB: Used instead of the "XPoint" type of X11
typedef struct Gpoint {
    int x;
    int y;
} Gpoint_t;

// A location in the real plane
typedef struct Fpoint {
    float x;
    float y;
} Fpoint_t;

// A location in the real plane
typedef struct Dpoint {
    double x;
    double y;
} Dpoint_t;

// A location in 3D space
typedef struct D3Dpoint {
    double x, y, z;
} D3Dpoint_t;

#endif

typedef unsigned long XID_t;

typedef struct backStore {
    XID_t id;
    int width;
    int height;
    double datastx;		// Position on data
    double datasty;
    double datawd;		// Amount of data displayed
    double dataht;
    int interpolation;		// Type of interpolation
    spVsInfo_t vsfunc;		// Intensity scaling
} BackStore_t;

typedef struct imgDataStore {
    float *data;                // Rebinned data
    int width;
    int height;
    double datastx;		// Position on data
    double datasty;
    double datawd;		// Amount of data displayed
    double dataht;
    int interpolation;          // Type of interpolation
    int rotation;
} ImgDataStore_t;


#endif /* AIPSTRUCTS_H */
