/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* 
 */

/*************************************************************************
*
*  Steve York
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA  94538
*
*************************************************************************/

#ifndef contrast_priv_h
#define contrast_priv_h

/* This is the private include file to be used only by the source */
/* code of the contrast library. The normal public functions are */
/* declared in contrast.h */

/* These are all of the XV_KEY_DATA fields used in the contrast windowing */
/* package. */
#define CONTRAST_FUNC		100	/* type of curve to draw (not used) */
#define CONTRAST_NOTIFY_FUNC	101	/* user function to call at change */
#define CENTER			102	/* x val at gray scale half height */
#define DEF_CENTER		103	/* default center */
#define LEFT_Y			104	/* pixel value at start index */
#define RIGHT_Y			105	/* pixel value at last index */
#define DEF_LEFT_Y		106	/* default pixel value */
#define DEF_RIGHT_Y		107	/* default pixel value */
#define SISCMS			109	/* pointer to SISCO colormap */
#define GDEV			110	/* graphics device handle */
#define GS_START		111	/* start index of gray scale colormap */
#define GS_NUM			112	/* number of entries in gs colormap */
#define NEW_CANVAS		113	/* tells whether canvas is clean */
#define SOLAR_NOTIFY_FUNC	114	/* user function to call at change */

/* These constants define which part of the line you grabbed */
#define LEFT_END		1	/* left 1/4 of line */
#define MIDDLE			2	/* center half of line */
#define RIGHT_END		3	/* right 1/4 of line */
#define GAMMA			4
#define GAMMALOG		5

/* These define the size of the contrast canvas */
#define	XMAX			256
#define YMAX			256

#define MAXGRAY			256
#define HALFGRAY		128

struct _contrast{
    float gamma;
    int contrast;
    int iflog;
    int ifgamma;
    int x0;			/* C-map index at left end of curve */
    int y0;			/* C-map entry at left end of curve */
    int x1;			/* C-map index at right end of curve */
    int y1;			/* C-map entry at right end of curve */
    int xhandle;
    int yhandle;
};

typedef struct {
    u_char red[MAXGRAY];
    u_char green[MAXGRAY];
    u_char blue[MAXGRAY];
} Cmap;
void
canvas_event_proc(Xv_Window window, Event *event);

void
show_line(Canvas canvas, int left_y, int right_y);

void
redraw_contrast_canvas(Canvas canvas, Gdev *gdev);

void
draw_line(Gdev *gdev, int left_y, int right_y, int color, Gpoint *points);

void 
contrast_reset(void);

void
contrast_set(void);

void
solar_set(void);

static void set_gamma(float);
static float get_gamma();
static void set_contrast(float);
static int get_contrast();
static void build_curve_data();
static void build_background();
static void draw_curve();
static void draw_background();
static void report_state();
static void set_log_switch(int);
static void set_gamma_switch(int);

Panel_setting
contrast_text_proc(Panel_item item, Event *event);

#endif /* contrast_priv_h */
