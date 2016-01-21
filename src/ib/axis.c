/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

static char *Sid(){
    return "@(#)axis.c 18.1 03/21/08 (c)1992 SISCO";
}

// #include <stream.h>
#include <string.h>
#include <math.h>
#include "graphics.h"
#include "axis.h"

#define IRINT(x) ((x) >= 0 ? (int)((x)+0.5) : (-(int)(-(x)+0.5)))

#ifndef OK
#define OK 0
#define ERROR -1
#endif

// The following table defines where to place the axis tick marks.
// The range of the plot (= max-min) is scaled by some power of ten
// (called "dex") such that the scaled range is in the half open
// interval (0.1, 1.0].  This scaled range is used to decide what
// coordinate values get tick marks.  The scaled range is compared to
// the "breakpoint"s in the table.  A table row applies to those cases
// where the scaled range is less than "breakpoint" (but larger than
// the previous breakpoint, if any).  The other entries give the
// intervals between little ticks, between big ticks,and between
// number labels in user units.  The table entry in use is kept in the
// private variables xit_index, and yit_index.
static struct {
    double breakpoint;
    double interval;
    double big_interval;
    double number_interval;
} interval_table[] = {
    {0.15, 0.005, 0.02, 0.02},
    {0.30, 0.010, 0.05, 0.05},
    {0.60, 0.020, 0.10, 0.10},
    {1.00, 0.050, 0.20, 0.20}
};

Axis::Axis(Gdev *gd,
	   double x0, double y0,	// Lower left corner in user coords
	   double x1, double y1)	// Upper right corner in user coords
{
    gdev = gd;

    user_x0 = x0;
    user_y0 = y0;
    user_x1 = x1;
    user_y1 = y1;

    location(0, 400, 600, 0);		// Set a rather arbitrary location

    shortx = 5;
    shorty = 5;
    x_tick_interval = 0;
    y_tick_interval = 0;
    first_x_tick = 0;
    first_y_tick = 0;

    longx = 10;
    longy = 10;
    x_bigtick_interval = 0;
    y_bigtick_interval = 0;
    first_x_bigtick = 0;
    first_y_bigtick = 0;

    x_number_interval = 0;
    y_number_interval = 0;
    first_x_number = 0;
    first_y_number = 0;

    axis_color = G_Get_Stcms1(gdev) + 4;	// Select default color from
    						//  "mark-color" choices.
    want_box = TRUE;
}

int
Axis::range(double x0, double y0,	// Lower left corner in user coords
	    double x1, double y1)	// Upper right corner in user coords
{
    if (x0 == x1){
	if (x0 == 0){
	    user_x0 = -1;
	    user_x1 = 1;
	}else{
	    user_x0 = 0.9 * x0;
	    user_x1 = 1.1 * x1;
	}
    }else{
	user_x0 = x0;
	user_x1 = x1;
    }

    if (y0 == y1){
	if (y0 == 0){
	    user_y0 = -1;
	    user_y1 = 1;
	}else{
	    user_y0 = 0.9 * y0;
	    user_y1 = 1.1 * y1;
	}
    }else{
	user_y0 = y0;
	user_y1 = y1;
    }

    set_conversion_constants();

    return OK;
}

char *
Axis::label_string(char axis,
		   double value)
{
    int index;
    double number_interval;
    char fmt_buf[10];
    static char label_buf[100];

    if (axis == 'x'){
	index = xit_index;
	number_interval = x_number_interval;
    }else if (axis == 'y'){
	index = yit_index;
	number_interval = y_number_interval;
    }else{
	fprintf(stderr,"Axis::label_string(): illegal axis specified: %c\n",
		axis);
	fprintf(stderr,"                     ... must be 'x' or 'y'\n");
	return 0;
    }

    if (fabs(value/number_interval) < 0.001){
	strcpy(label_buf, "0");
	return label_buf;
    }

    if (fabs(number_interval) > 1e5 || fabs(number_interval) < 1e-5){
	sprintf(label_buf, "%.2e", value);
    }else{
	double test = number_interval;
	double tol = fabs(0.001 * number_interval);
    	int dd;
	for (dd=0; fabs(rint(test)-test) > tol; dd++,test *= 10,tol *= 10);
	sprintf(fmt_buf, "%%.%df", dd);
	sprintf(label_buf, fmt_buf, value);
    }
    return label_buf;
}

int
Axis::u_to_x(double user_x)
{
    return IRINT(a_utox * user_x + b_utox);
}

int
Axis::u_to_y(double user_y)
{
    return IRINT(a_utoy * user_y + b_utoy);
}

double
Axis::x_to_u(int x)
{
    return a_xtou * x + b_xtou;
}

double
Axis::y_to_u(int y)
{
    return a_ytou * y + b_ytou;
}

int
Axis::location(int x0, int y0,		// Position of lower left corner
	       int x1, int y1)		// Position of upper right (hdwe units)
{
    if (x0 == x1 || y0 == y1){
	return ERROR;
    }

    pix_x0 = x0;
    pix_y0 = y0;
    pix_x1 = x1;
    pix_y1 = y1;

    set_conversion_constants();

    return OK;
}

void
Axis::set_conversion_constants()
{
    // Set constants for conversion of user to pixel coordinates
    a_utox = (pix_x1 - pix_x0) / (user_x1 - user_x0);
    b_utox = pix_x0 - user_x0 * (pix_x1 - pix_x0) / (user_x1 - user_x0);
    a_utoy = (pix_y1 - pix_y0) / (user_y1 - user_y0);
    b_utoy = pix_y0 - user_y0 * (pix_y1 - pix_y0) / (user_y1 - user_y0);

    // Set constants for conversion of pixel to user coordinates
    a_xtou = (user_x1 - user_x0) / (pix_x1 - pix_x0);
    b_xtou = user_x0 - pix_x0 * (user_x1 - user_x0) / (pix_x1 - pix_x0);
    a_ytou = (user_y1 - user_y0) / (pix_y1 - pix_y0);
    b_ytou = user_y0 - pix_y0 * (user_y1 - user_y0) / (pix_y1 - pix_y0);
}

int
Axis::color(int new_color)
{
    axis_color = new_color;
    return OK;
}

int
Axis::box(int new_value)
{
    want_box = new_value ? TRUE : FALSE;
    return OK;
}

int
Axis::little_ticks(char axis)		// Which axis: 'x' or 'y'
{

    double min, max;			// Range of data (left to right)

    if (axis == 'x'){
	max = user_x1;
	min = user_x0;
    }else if (axis == 'y'){
	max = user_y1;
	min = user_y0;
    }else{
	fprintf(stderr,"Axis::little_ticks: illegal axis specified: %c\n",
		axis);
	fprintf(stderr,"                     ... must be 'x' or 'y'\n");
	return ERROR;
    }
    
    double range = max - min;
    if (range == 0.0){
	/*fprintf(stderr,"Axis::little_ticks: range of data is 0\n");*/
	return ERROR;
    }

    // Scale "range" to the interval (0.1, 1.0]
    // Set "dex" to a power of 10 such that 0.1 < dex*range <= 1.0
    double dex = range < 0 ? -1.0 : 1.0;	// Make product positive
    for ( ; dex*range > 1.0; dex /= 10.0);	// Make it small enough
    for ( ; dex*range <= 0.1; dex *= 10.0);	// Make it big enough

    // Select the appropriate tick interval from the table.
    int index;
    for (index=0; dex*range>interval_table[index].breakpoint; index++);

    // Set the appropriate class variables.

    if (axis == 'x'){
	x_tick_interval = interval_table[index].interval / dex;
	first_x_tick = ceil(min/x_tick_interval) * x_tick_interval;
	xit_index = index;
    }else if (axis == 'y'){
	y_tick_interval = interval_table[index].interval / dex;
	first_y_tick = ceil(min/y_tick_interval) * y_tick_interval;
	yit_index = index;
    }

    return OK;
}

int
Axis::big_ticks(char axis)			// "axis" is 'x' or 'y'.
{
    double first_little_tick;		// Position in user units.
    double little_tick_interval;	// User units (<0 if axis backwards).
    double axis_end;			// Right end of axis in user units.
    int index;				// Index into interval table.

    // Get the basic constants to do calculations.
    if (axis == 'x'){
	// Initialize little ticks
	if (little_ticks('x') == ERROR){
	    return ERROR;
	}
	first_little_tick = first_x_tick;
	little_tick_interval = x_tick_interval;
	axis_end = user_x1;
	index = xit_index;
    }else if (axis == 'y'){
	// Initialize little ticks
	if (little_ticks('y') == ERROR){
	    return ERROR;
	}
	first_little_tick = first_y_tick;
	little_tick_interval = y_tick_interval;
	axis_end = user_y1;
	index = yit_index;
    }else{
	fprintf(stderr,"Axis::big_ticks: illegal axis specified: %c\n",
		axis);
	fprintf(stderr,"                     ... must be 'x' or 'y'\n");
	return ERROR;
    }

    // Set the scaling factor.
    double dex = interval_table[index].interval / little_tick_interval;
    
    // Look up what big-tick interval to use.
    double bigtick_interval = interval_table[index].big_interval / dex;

    // First big tick goes at first little tick divisible by big tick
    double first_bigtick;
    for (first_bigtick=first_little_tick;
	 fabs( remainder(first_bigtick, bigtick_interval) ) >
					fabs(0.001 * bigtick_interval);
	 first_bigtick += little_tick_interval)
    ;

    // Set the local class variables.
    if (axis == 'x'){
	x_bigtick_interval = bigtick_interval;
	first_x_bigtick = first_bigtick;
    }else if (axis == 'y'){
	y_bigtick_interval = bigtick_interval;
	first_y_bigtick = first_bigtick;
    }
    return OK;
}

int
Axis::number(char axis)			// "axis" is 'x' or 'y'.
{
    double first_bigtick;		// Position in user units.
    double bigtick_interval;		// User units (<0 if axis backwards).
    double axis_end;			// Right end of axis in user units.
    int index;				// Index into interval table.

    // Get the basic constants to do calculations.
    if (axis == 'x'){
	// Initialize big ticks
	if (big_ticks('x') == ERROR){
	    return ERROR;
	}
	first_bigtick = first_x_bigtick;
	bigtick_interval = x_bigtick_interval;
	axis_end = user_x1;
	index = xit_index;
    }else if (axis == 'y'){
	// Initialize big ticks
	if (big_ticks('y') == ERROR){
	    return ERROR;
	}
	first_bigtick = first_y_bigtick;
	bigtick_interval = y_bigtick_interval;
	axis_end = user_y1;
	index = yit_index;
    }else{
	fprintf(stderr,"Axis::number(): illegal axis specified: %c\n",
		axis);
	fprintf(stderr,"                     ... must be 'x' or 'y'\n");
	return ERROR;
    }

    // Set the scaling factor.
    double dex = interval_table[index].big_interval / bigtick_interval;
    
    // Look up what number interval to use.
    double number_interval = interval_table[index].number_interval / dex;

    // First number goes at first big tick divisible by number spacing
    double first_number;
    for (first_number = first_bigtick;
	 fabs( remainder(first_number, number_interval) ) >
					fabs(0.001 * number_interval);
	 first_number += bigtick_interval)
    ;

    // Set the local class variables.
    if (axis == 'x'){
	x_number_interval = number_interval;
	first_x_number = first_number;
    }else if (axis == 'y'){
	y_number_interval = number_interval;
	first_y_number = first_number;
    }
    return OK;
}

void
Axis::plot()
{
    // Space for g_get_string_width() to return stuff:
    int width;
    int height;
    int ascent;
    int descent;
    int direction;
    
    // Not designed to work with XOR'ing turned on
    int save_g_mode = G_Get_Op(gdev);
    G_Set_Op(gdev, GXcopy);

    // Draw enclosing box
    if (want_box || x_tick_interval){
	g_draw_line(gdev, pix_x0, pix_y0, pix_x1, pix_y0, axis_color);
    }
    if (want_box || y_tick_interval){
	g_draw_line(gdev, pix_x0, pix_y1, pix_x0, pix_y0, axis_color);
    }
    if (want_box){
	g_draw_line(gdev, pix_x1, pix_y0, pix_x1, pix_y1, axis_color);
	g_draw_line(gdev, pix_x1, pix_y1, pix_x0, pix_y1, axis_color);
    }

    // Small x-ticks
    double sgn = copysign(1.0, x_tick_interval);
    if (x_tick_interval){
	for (double x=first_x_tick; x*sgn <= user_x1*sgn; x += x_tick_interval){
	    g_draw_line(gdev,
			u_to_x(x), pix_y0,
			u_to_x(x), pix_y0+shortx,
			axis_color);
	}
    }

    // Big x-ticks
    if (x_bigtick_interval){
	for (double x=first_x_bigtick; x*sgn <= user_x1*sgn;
	     x += x_bigtick_interval)
	{
	    g_draw_line(gdev,
			u_to_x(x), pix_y0,
			u_to_x(x), pix_y0+longx,
			axis_color);
	}
    }

    // Put in the x-axis labels
    char *xlabel;
    if (x_number_interval){
	for (double x=first_x_number; x*sgn <= user_x1*sgn;
	     x += x_number_interval)
	{
	    xlabel = label_string('x', x);
	    g_get_string_width(gdev, FONT_SMALL, xlabel,
			       width, height, ascent, descent, direction);
	    g_draw_string(gdev,
			  (int)(u_to_x(x) - width / 2),
			  pix_y0 + longx + height + 4,
			  FONT_SMALL,
			  xlabel,
			  axis_color);
	}
    }

    // Small y-ticks
    sgn = copysign(1.0, y_tick_interval);
    if (y_tick_interval){
	for (double y=first_y_tick; y*sgn <= user_y1*sgn; y += y_tick_interval){
	    g_draw_line(gdev,
			pix_x0, u_to_y(y),
			pix_x0-shorty, u_to_y(y),
			axis_color);
	}
    }

    // Big y-ticks
    if (y_bigtick_interval){
	for (double y=first_y_bigtick; y*sgn <= user_y1*sgn;
	     y += y_bigtick_interval)
	{
	    g_draw_line(gdev,
			pix_x0, u_to_y(y),
			pix_x0-longy, u_to_y(y),
			axis_color);
	}
    }

    // Put in the y-axis labels
    char *ylabel;
    if (y_number_interval){
	for (double y=first_y_number; y*sgn <= user_y1*sgn;
	     y += y_number_interval)
	{
	    ylabel = label_string('y', y);
	    g_get_string_width(gdev, FONT_SMALL, ylabel,
			       width, height, ascent, descent, direction);
	    g_draw_string(gdev,
			  pix_x0 - width - longx - 4,
			  (int)(u_to_y(y) + ascent/2.0),
			  FONT_SMALL,
			  ylabel,
			  axis_color);
	}
    }

    // Restore the graphics writing mode for this canvas
    G_Set_Op(gdev, save_g_mode);
}

/*
main()
{
    Axis axis(0, 0.0021, 1.0, 0.0, 11.0);
    axis.location(50, 390, 590, 10);
    axis.color(6);
    if (axis.number('x')) return ERROR;
    if (axis.big_ticks('y')) return ERROR;
    if (axis.number('y')) return ERROR;
    axis.plot();
    return OK;
}
*/
