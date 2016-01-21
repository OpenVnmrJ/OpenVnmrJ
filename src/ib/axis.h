/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
// @(#)axis.h 18.1 03/21/08 (c)1992-93 SISCO 

#ifndef _GRID_H
#define _GRID_H

class Axis{
  public:
    Axis(Gdev *,				// Graphics context
	 double = 0.0, double = 0.0,	// Lower left corner (user coords)
	 double = 1.0, double = 1.0);		// Upper right corner
    int range(double, double, double, double);	// Set new user range
    int location(int, int, int, int);		// Position axes on canvas
    int color(int);				// Set color of axes
    int box(int);				// Set box on or off
    int little_ticks(char);			// Set the tick interval
    int big_ticks(char);			// Set the big-tick interval
    int number(char);				// Number the axis
    int u_to_x(double);		// Converts user x-coord to hdwe x-coord
    int u_to_y(double);		// Converts user y-coord to hdwe y-coord
    double x_to_u(int);		// Converts hdwe x-coord to user x-coord
    double y_to_u(int);		// Converts hdwe y-coord to user y-coord
    void plot();				// Draw the axes
  private:
    void set_conversion_constants();	// Initializes scale factors
    char *label_string(char, double);	// Makes a number label string for axis

    Gdev *gdev;			// Points to graphic context

    double user_x0, user_y0;	// Lower left corner of plot in user coords
    double user_x1, user_y1;	// Upper right corner

    int pix_x0, pix_y0;		// Lower left corner of plot in hdwe coords
    int pix_x1, pix_y1;		// Upper right corner

    double a_utox;		// User to hdwe conversion: coefficient
    double b_utox;		// User to hdwe conversion: constant term
    double a_utoy;
    double b_utoy;

    double a_xtou;		// Hdwe to user coord conversion
    double b_xtou;
    double a_ytou;
    double b_ytou;

    int axis_color;		// Color of axes and labels
    int want_box;		// True if we should draw a box around area
    
    int xit_index;		// Index into "interval_table" for x-axis
    int yit_index;

    int shortx;			// Length of short x-ticks (hdwe units)
    int shorty;			// Length of short y-ticks (hdwe units)
    double x_tick_interval;	// Distance between x-ticks (user units)
    double y_tick_interval;
    double first_x_tick;	// Position of first x-tick (user units)
    double first_y_tick;

    int longx;			// Length of long x-ticks (hdwe units)
    int longy;			// Length of long y-ticks (hdwe units)
    double x_bigtick_interval;	// Distance between big x-ticks (user units)
    double y_bigtick_interval;
    double first_x_bigtick;	// Position of first big x-tick (user units)
    double first_y_bigtick;

    double x_number_interval;	// Distance between x-axis numbers (user units)
    double y_number_interval;
    double first_x_number;	// Position of first number (user units)
    double first_y_number;
};

#endif /* ~ _GRID_H */
