/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef AIPAXISINFO_H
#define AIPAXISINFO_H

#include <list>

#include "aipViewInfo.h"
#include "aipGframe.h"

#define INDEX_SHOW 1  
#define INDEX_WEST 2  
#define INDEX_EAST 3  
#define INDEX_SOUTH 4  
#define INDEX_NORTH 5  
#define INDEX_TICK 6  
#define INDEX_CROSS 7  
#define INDEX_FOV 8  

#define INDEX_WIDTH 1  
#define INDEX_HEIGHT 2  
#define INDEX_WX 3  
#define INDEX_WY 4  
#define INDEX_EX 5  
#define INDEX_EY 6  
#define INDEX_SX 7  
#define INDEX_SY 8  
#define INDEX_NX 9  
#define INDEX_NY 10 

typedef enum {
    AX_NONE = 0,
    AX_WEST = 1,
    AX_EAST = 2,
    AX_SOUTH = 4,
    AX_NORTH = 8
} axisDisp_t;

class AxisInfo {

public:

    static int pixstx, pixsty, pixwd, pixht;
    static bool showAxis;
     
    static bool canShowAxis(spGframe_t gf);
    static void displayAxis(spGframe_t gf);
    static void displayAxis_west(spGframe_t gf, bool inflg=true);
    static void displayAxis_east(spGframe_t gf, bool inflg=true);
    static void displayAxis_south(spGframe_t gf, bool inflg=true);
    static void displayAxis_north(spGframe_t gf, bool inflg=true);
    static void drawVertAxis(spGframe_t gf, int xoff, int yoff, int minpixy, int maxpixy, int tickpix);
    static void drawHorizAxis(spGframe_t gf, int xoff, int yoff, int minpixx, int maxpixx, int tickpix);
    static void drawString(spGframe_t gf, char *str, int x, int y, int color, int direction); 
    static int calcAxis(double dmin, double dmax, int pmin, int pmax, 
	double &first, double &inc, double &bigTickMag, int &nticks);

    static void drawCenterLines(spGframe_t gf);

    static void showIntensity(spGframe_t gf, int x, int y, bool updateSlaves=false, double intensity=0.0);

    static bool canShowLabel(spGframe_t gf, int x, int y);
    static void showPosition(spGframe_t gf, int x, int y, bool showLabel=true, bool updateSlaves=false); 
    static void showDistance(spGframe_t gf, int x1, int y1, int x2, int y2, double dis=0.0);
    static int getPositionString(spGframe_t gf, int x, int y, char *str);
    static int getDistanceString(spGframe_t gf, int x1, int y1, int x2, int y2, char *str, double dis=0.0);

    static void pixToLogical(spViewInfo_t view, double px, double py, double pz, double &mx, double &my, double &mz);
    static void logicalToPix(spViewInfo_t view, double mx, double my, double mz, double &px, double &py, double &pz);

    static void pixToMagnet(spViewInfo_t view, double px, double py, double pz, double &mx, double &my, double &mz);
    static void magnetToPix(spViewInfo_t view, double mx, double my, double mz, double &px, double &py, double &pz);

    static void pixToUser(spViewInfo_t view, double px, double py, double pz, double &ux, double &uy, double &uz);
    static void userToPix(spViewInfo_t view, double ux, double uy, double uz, double &px, double &py, double &pz);

    static void pixToData(spViewInfo_t view, double px, double py, double &dx, double &dy);
    static void dataToPix(spViewInfo_t view, double dx, double dy, double &px, double &py);
    static int mapIndex(spViewInfo_t view, int i);

    static void initAxis(spGframe_t gf);
     // AXIS DISPLAY PROPERTIES

    static int isDisplayed;		// AX_NONE | AX_SIDE1 | AX_SIDE2 | AX_LABEL 
    static string units;		// E.g., "cm", "mm", "pixel","point","ppm" 

    // Other properties specified globally (color, tick size, in/out ...) 

    static int axisColor;
    static int labelColor;
    static int tickSize;
    static double horizSize;
    static double vertSize;
    static double westX;
    static double westY;
    static double eastX;
    static double eastY;
    static double southX;
    static double southY;
    static double northX;
    static double northY;
    static int axisSize;
    static bool inflg;
    static bool crosshair;
    static bool magnetFrame;

private:

    static int minPixSize;
    static int minPixPerCm;
};

#endif /* AIPAXISINFO_H */
