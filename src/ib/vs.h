/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

/* 
 */

#ifndef _VS_H
#define _VS_H

#include "vsfunc.h"

typedef enum{
    VS_BIND,
    VS_VS,
    VS_CONTRAST
} Vs_props_menu;

class Vscale
{
  private:
    friend class Vs_routine;

    static Flag bind;		// Indicate binding or not
    static Menu_item bind_menu_item;
    static int vs_band;		// the width to find new vs in the image data
    static void set_bind(int);
    static void contrast();
    static float calculate_new_vs(Gframe *, float, float);
    static Flag set_new_vs(Gframe *, float newvs);
    static Flag set_new_vs(Gframe *, float max_data, float min_data,
			   Flag toolupdate);
    static Flag set_new_vs(Gframe *, VsFunc *);
    static void rescale_image(float newvs); // Rescale selected images
    static void rescale_image(float max_data, float min_data, Flag toolupdate);
    static void rescale_image(VsFunc *); // Rescale selected images
    static void rescale_image(float x, float y); // Rescale selected images
    static void rescale_image(float x, float y, Gframe*); // Rescale one image
    static int set_attr(int, char *);
    static void contrast_callback(float, float, float, float);
    static void solar_callback(int);
    static void vscale_callback(VsFunc *);
    static void vscale();

  public:
    static void contrast_init();
    static void vscale_init();
    static int Bind(int argc, char **argv, int retc, char **retv);
    static int Contrast(int argc, char **argv, int retc, char **retv);
    static int Saturate(int argc, char **argv, int retc, char **retv);
    static int Vs(int argc, char **argv, int retc, char **retv);
};

#endif /* _VS_H */
