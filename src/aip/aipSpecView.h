/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPSPECVIEW_H
#define AIPSPECVIEW_H

#include "sharedPtr.h"
#include "aipVnmrFuncs.h"
#include "aipGraphicsWin.h"

typedef float float2[2];
typedef float float3[3];

typedef struct {
  int index;
  int npts;
  Dpoint_t polygon[12];
} graphInfo_t;

class SpecView 
{
public:

    SpecView(graphInfo_t gInfo); 
    ~SpecView();

    // draw in frame (in pixels in the graphics area)
    // return number traces
    void drawSpec(float *d, int n, int step, double vscale, double yoff, int fx, int fy, int fw, int fh, int color);
    void drawGrid(int fx, int fy, int fw, int fh, int color);
    void drawNumber(int fx, int fy, int fw, int fh, int color);
    bool isSelected(int x, int y);
    bool isSelected() {return selected;}
    void select(bool b) {selected=b;}
    void setShow(bool b) {show=b;}

    graphInfo_t *getGraphInfo() {return &gInfo;}
    static int getIndex(graphInfo_t g);
    int getIndex() {return index;}

    static void drawPolyline(float *d, int n, int step, double vx, double vy, double vw, double vh, int color, bool autoVS, double vScale, double yoff);

protected:
    
private:
   double voxx, voxy, voxw, voxh; // voxel in pixels in the graphics area.
   graphInfo_t gInfo;
   int index;
   
   bool selected;
   void setViewFrame();
   bool canDrawGrid(int fx, int fy, int fw, int fh);
   bool canDrawSpec(int fx, int fy, int fw, int fh);
   bool show;

};
typedef boost::shared_ptr<SpecView> spSpecView_t;
#endif /* AIPSPECVIEW_H */
