/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef ASPDIS1D_H
#define ASPDIS1D_H

#include <list>
#include "AspFrame.h"

class AspDis1D {

public:

    static void display(spAspFrame_t frame); 
    static int asp1D(int argc, char *argv[], int retc, char *retv[]);
    static void setZoom(int frameID, int x, int y, int prevX, int prevY);
    static void showCursor(int frameID, int x, int y, int mode, int cursor);
    static void specVS(int frameID, int clicks, double factor);
    static void zoomSpec(int frameID, int x, int y, int mode);
    static void panSpec(int frameID, int x, int y, int prevX, int prevY, int mode);
    static void setArray(spAspFrame_t frame, int x, int y, int prevX, int prevY, int mode);
    
    static bool selectThresh(spAspFrame_t frame, int x, int y);
    static int getThreshPix(spAspCell_t cell);
    static void moveThresh(spAspFrame_t frame, int x, int y, int prevX, int prevY);
    static void drawThresh(spAspCell_t cell, bool selected);
    static spAspTrace_t selectTrace(spAspFrame_t frame, int x, int y);
    static void deleteTrace(spAspFrame_t frame, spAspTrace_t trace);
    static void showFields(spAspFrame_t frame, spAspCell_t cell);
    static void setPhase0(spAspFrame_t frame, int x, int y, int prevX, int prevY, int mode);
    static void moveTrace(spAspFrame_t frame, spAspTrace_t trace, int y, int prevY);
    static string getColor(string key);
    static void unselectTraces(spAspFrame_t frame);
    static void alignSpec(spAspFrame_t frame, char *peakFile=NULL);
    static int loadData(spAspFrame_t frame, bool sel=false);

private:

    static void dssh(spAspFrame_t frame, int argc, char *argv[]);
    static void dss(spAspFrame_t frame, int argc, char *argv[]);
    static void nxm(spAspFrame_t frame, int argc, char *argv[]);
    static void ds(spAspFrame_t frame, int argc, char *argv[]);
    static int mspec(spAspFrame_t frame, int argc, char *argv[]);
    static void dsExpand(spAspFrame_t frame, int argc, char *argv[]);
    static void setSpecRegion(double c1, double c2, double scale=1.0, bool showCursor=false);
    static int loadData(spAspFrame_t frame, int argc, char *argv[]);

};

#endif /* ASPDIS1D_H */
