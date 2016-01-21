/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/


#ifndef ASPUTIL_H
#define ASPUTIL_H

#include <sys/stat.h>
#include "aipVnmrFuncs.h"
#include "aipStructs.h"
#include "aipGraphicsWin.h"

#define HORIZ	1
#define VERT	2

#define MAXTRACE 256
#define MAXSTR2  512 

// for reading a line
#define MAXWORDNUM 128 
#define MAXWORDLEN 16

#define MARK_SIZE 2

#define NOSELECT 0
#define BOXSELECT 1 
#define HANDLE1 2
#define HANDLE2 3
#define HANDLE3 4
#define HANDLE4 5
#define LINE1 6
#define LINE2 7
#define LINE3 8
#define LINE4 9

#define ROI_BAND 1 
#define ROI_BOX 2 

#define HANDLE_SIZE 6 

#define FULL 0 
#define ZOOM_IN 1
#define ZOOM_OUT 2 

#define CENTER 0 
#define PAN_1D 1 // pan either horizontally or vertically
#define PAN_2D 2 // pan both horizontally and vertically 
#define PAN_ZOOM 3 // drag vertically up to zoom out, drag down to zoom in 
#define RESET 4 

#define ARRAY_RESET 0 
#define ARRAY_RESETALL 1 
#define ARRAY_OFFSET 2 
#define ARRAY_VP 3 

#define AUTO 0 
#define COARSE 1 
#define FINE 2 

#define NONE_ASP_MODE 0 
#define ASP_MODE 1 
#define ASP_GRID_MODE 2 

extern "C" {
#include "graphics.h"
#include "group.h"
void getPlotBox_Pix(double *x, double *y, double *w, double *h);
void set_clip_region(int x, int y, int w, int h);
void set_top_frame_on();
void set_top_frame_off();
void clear_top_frame();
void set_background_region(int x, int y, int w, int h, int color, int alpha);
int colorindex(char *colorname, int *index);
int vj_x_cursor(int n, int *old_pos, int new_pos, int c);
int vj_y_cursor(int n, int *old_pos, int new_pos, int c);
void set_graphics_font(const char *fontName);
void set_line_thickness(const char *thick);
void set_spectrum_thickness(char *min, char *max, double ratio);
void set_spectrum_width(int thick);
void set_line_width(int thick);
void set_anno_color(const char *colorName);
void set_anno_font(const char *fontName, const char *fontStyle, const char *fontColor, int fontSize);
void set_transparency_level(double n); // 0: opaque,  1.0: transparent
void draw_arrow(int x1, int y1, int x2, int y2, int thick, int color);
void draw_round_rect(int x1, int y1, int x2, int y2, int thick, int color);
void draw_oval(int x1, int y1, int x2, int y2, int thick, int color);
// old_pos is not used, should be NULL
// n = 0 for first cursor, 1 for second cursor, 2 for threshold
// c is color
extern int VnmrJViewId;
extern int dfpnt;
extern int dnpnt;
extern int dfpnt2;
extern int dnpnt2;
extern double sc;
extern double wc;
extern double sc2;
extern double wc2;
}

/* these are already in graphics.h
extern double wcmax;
extern double wc2max;
extern int ycharpixels;
extern int mnumypnts;
extern int mnumxpnts;
extern int ymin;
extern int right_edge;
*/

typedef enum {                  // spec display 
        SPEC_DS = 0x100,
        SPEC_DSS = 0x200,
        SPEC_GRID = 0x400,
        SPEC_DCON = 0x8000,
        SPEC_THRESH = 0x10000,
        SPEC_LABEL = 0x20000,
        SPEC_INDEX = 0x40000,
} aspDisp_t;

typedef enum {                  // axis display 
        AX_WEST = 0x100,
        AX_EAST = 0x200,
        AX_SOUTH = 0x400,
        AX_NORTH = 0x10000,
        AX_BOX = 0x20000,
        AX_LABEL = 0x40000,
        AX_PPM = 0x80000,
        AX_HZ = 0x100000,
} aspAxis_t;

typedef enum {                  // annotation display 
	ANN_ROIS = 0x100,
	ANN_ANNO = 0x200,
} aspAnnotation_t;

typedef enum {                  // peak display 
	PEAK_MARKING = 0x100,
	PEAK_TOP = 0x200,
	PEAK_VERT = 0x400,
	PEAK_HORIZ = 0x800,
        PEAK_AUTO = 0x10000,
        PEAK_NAME = 0x20000,
        PEAK_VALUE = 0x40000,
        PEAK_NOLINK = 0x80000,
        PEAK_SHORT = 0x100000,
} aspPeakAnn_t;

typedef enum {                  // peak display 
   SHOW_INTEG = 0x100,
   SHOW_VALUE = 0x200,
   SHOW_LABEL = 0x400,
   SHOW_VERT_VALUE = 0x800,
   SHOW_VERT_LABEL = 0x1000,
} aspIntegAnn_t;

#define MARKSIZE 4

#define NONE_SELECTED 0
#define PEAK_SELECTED 1
#define HANDLE_SELECTED 2
#define ROI_SELECTED 3
#define LABEL_SELECTED 4

typedef struct Dbox {
   double x,y,w,h;
} Dbox_t;

typedef struct aspResonance {
   string name;
   string assignedName;
   double freq; //in ppm
} aspResonance_t;

typedef struct aspRegion {
   string name; // nucname
   double first, width; // e.g. (cr, delta), or (sp+sw,wp) in default units 
} aspRegion_t;

typedef struct aspiAxisInfo {
   string name; // nucname
   string label; // such as F1 or F2
   string dunits; // default units such as ppm, mm
   string units; // selected units, such as ppm or Hz.
   double scale; // convert from default units to selected units, e.g., reffrq (ppm to Hz)
   double maxwidth; // sw in default units
   double minfirst; // rflrfp+sw (note, from left to right) in default units
   double width; // wp
   double start; // sp+sw 
   int rev;
   int npts;
   int orient;
} aspAxisInfo_t;

class AspUtil
{
public:
   // these are different frame aip's getReal,etc... by try CURRENT, then GLOBAL. 
   static int getParSize(string varname);
   static bool setReal(string varname, double value, bool notify);
   static bool setReal(string varname, int index, double value, bool notify);
   static bool setString(string varname, string value, bool notify);
   static double getReal(string varname, double defaultVal);
   static double getReal(string varname, int index, double defaultVal);
   static string getString(string varname);
   static string getString(string varname, string defaultVal);
   static bool isActive(string varname);
   static bool setActive(string varname, bool value);

   static int selectLine(int x, int y, int x1, int y1, int x2, int y2,bool handle=false);
   static int select(int x, int y, int px, int py, int pw, int ph, int rank=2, bool handle=false);
   static int select(int x, int y, double px, double py, double pw, double ph, int rank=2, bool handle=false);
   static void drawHandle(int mouseOver, double px, double py, double pw, double ph, int color, int thickness=1);
   static void drawBorderLine(int mouseOver, double px, double py, double pw, double ph, int color, int thickness=1);
   static void drawBox(double px, double py, double pw, double ph, int color, int thickness=1);
   static void drawBox(int px, int py, int pw, int ph, int color, int thickness=1);
   static void drawOval(int px, int py, int pw, int ph, int color, int thickness=1);
   static void drawLine(Dpoint_t p1, Dpoint_t p2, int color, int thickness=1);
   static void drawArrow(Dpoint_t p1, Dpoint_t p2, int color, bool tail=true, bool twoEnds=false, int xsize=8, int ysize=6, int thickness=1);
   static void clearFields(int x, int y, int w, int h);
   static void writeFields(char *str, int x, int y, int w, int h);
   static void drawString(const char *str, int x, int y, int color, const char *fontName, int rotate=0);
   static void drawMark(int x, int y, int w, int h, int color, int thickness=1);
   static void drawMark(int x, int y, int color, int thickness=1);

   static int drawYbars(float *phasfl, int n, int skip, double vx, double vy, double vw, double vh, int dcolor, double scale, double yoff, int vert=0);

   static int getColor(char *name);
   static void getFirstLastStep(char *str, int max, int &first, int &last, int &step);
   static void getDisplayOption(string name, string &value);
   static int selectPolygon(int x, int y, Dpoint_t *poly, int np);
   static string subParamInStr(string str);

private:
};
#endif // ASPUTIL_H
