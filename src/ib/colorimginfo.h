/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
#ifndef _COLORIMGINFO_H
#define _COLORIMGINFO_H
/*===================================================================

  "@(#)colorimginfo.h 18.1 03/21/08 (c)1993 SISCO"

=====================================================================

    Header File Information
		Source file  : colorimginfo.h
		Package name : CSI Level-2

    Copyright (c) 1993  by Spectroscopy Imaging Systems Corporation
=====================================================================*/

/*
 *	include files
 */
#include	"imginfo.h"
#include	"P_colormap.h"

/*
 *   class ColorImginfo
 */
class ColorImginfo : public Imginfo
{
  public:
    enum {maxbands = 4};	// Max number of colors we can specify.
    int nbands;			// Number of colors per data point
    float vmin[maxbands];	// Value to display at 0 intensity
    float vmax[maxbands];	// Value to display at max intensity

    ColorImginfo(void);
    ColorImginfo(ColorImginfo* from);
    ~ColorImginfo(void);

    virtual void display_data(Gframe *gframe);

    // zoom functions
    virtual Gframe *zoom_full(Gframe *);
    virtual void zoom(Gframe *);
    virtual void unzoom(Gframe *);
//    virtual void draw_zoom_lines(int);
//    virtual void draw_zlinex1(int);
//    virtual void draw_zlinex2(int);
//    int select_zoom_lines(int x, int y, int aperture, int zline);
//    void move_zlines(int x, int y, int prevx, int prevy,
//					 int color, int zline);
//    void move_all_zlines(int, int, int, int, int);
//    void replace_zlines(float, float, float, float, int);

//    virtual void unzoom_full();	// Not needed at this point.

    // Data functions

//    virtual float calculate_new_image_vs(int, int, int) {return 0.0;};
//    virtual Flag set_new_image_vs(float) {return FALSE;};
//    virtual void keep_point_in_image(short *x, short *y);
//    virtual int XDataToScreen(float xdata);
//    virtual int YDataToScreen(float ydata);
//    virtual float XScreenToData(int xp);
//    virtual float YScreenToData(int yp);
//    virtual void update_scale_factors();
//    void update_data_coordinates();	// Update everything in the 
//						// display list
//    void update_screen_coordinates();	// Update everything in the 
//						// display list

  private:
    void make_dithered_image(float *cdata, int nx, int ny);
};

#endif _COLORIMGINFO_H
