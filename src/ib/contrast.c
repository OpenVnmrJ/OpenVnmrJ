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

#include <stdio.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <math.h>
#include <string.h>

#include "graphics.h"
#include "stderr.h"
#include "contrast_priv.h"
#include "contrast.h"

/* Linear interp y[x], given x and pointer to y[0] (assumes y[1] exists too) */
#define LIN_INTERP(x, py) ((u_char)( *(py) + (x) * (*((py)+1) - *(py))))

static Frame contrast_popup;
static Panel contrast_panel;
static Canvas contrast_canvas = NULL;

static int left_y = 0;			/* left end pixel value */
static int def_left_y = 0;		/* default left end pixel value */
static int right_y = MAXGRAY;		/* right end pixel value */
static int def_right_y = MAXGRAY;	/* default right end pixel value */
static int current_left_y = 0;		/* Where we last set the line */
static int current_right_y = MAXGRAY;

static int width;	/* width of canvas */
static int height;	/* height of canvas */
static int gs_num;	/* size of greyscale map */
static Gpoint *curve_pts = NULL;
static Pixmap bkg_pixmap = NULL;

static Cmap colormap_mem;

static int min_grey_shown;
static int max_grey_shown;
static int low_mask_on;
static int high_mask_on;

static u_char red_mask;
static u_char green_mask;
static u_char blue_mask;
static u_char red_himask;
static u_char green_himask;
static u_char blue_himask;
static u_char red_solar;
static u_char green_solar;
static u_char blue_solar;

#define BG_COLOR 0
#define FG_COLOR 1
#define HIMASK_COLOR 7
#define MASK_COLOR 3
#define SOLAR_COLOR 2

static Panel_item show_solar;
static Panel_item gamma_switch;
static Panel_item gamma_widget;
static Panel_item log_switch;
static Panel_item contrast_widget;
static struct _contrast state;

static void mark(Gdev *gdev, int x, int y, int color);
static void contrast_canvas_repaint_proc();
static void contrast_canvas_resize_proc(Canvas, int, int);
static void curve_func();
static void gamma_switch_callback(Panel_item, int, Event *);
static void gamma_callback(Panel_item, int, Event *);
static void log_switch_callback(Panel_item, int, Event *);
static void contrast_callback(Panel_item, int, Event *);

/* This function create the contrast command frame, panel and canvas, */
/* initialized to the variables indicated by the user. */
void
contrast_win_create(Frame owner,	/* owner frame handle */
		    int x,		/* x location of window */
		    int y,		/* y location of window */
		    int wd,		/* width of canvas */
		    int ht,		/* height of canvas */
		    Siscms *siscms,	/* SIS color map */
		    int gs_start,	/* start index of greyscale map */
		    int gs_size)	/* size of greyscale map */
{
    Gdev *gdev;

    /* set up the static variables */
    gs_num = gs_size;
    min_grey_shown = 0;
    max_grey_shown = gs_size;
    low_mask_on = high_mask_on = FALSE;

    red_himask = siscms->r[HIMASK_COLOR];
    green_himask = siscms->g[HIMASK_COLOR];
    blue_himask = siscms->b[HIMASK_COLOR];

    red_mask = siscms->r[MASK_COLOR];
    green_mask = siscms->g[MASK_COLOR];
    blue_mask = siscms->b[MASK_COLOR];

    red_solar = siscms->r[SOLAR_COLOR];
    green_solar = siscms->g[SOLAR_COLOR];
    blue_solar = siscms->b[SOLAR_COLOR];

    current_left_y = def_left_y;
    current_right_y = def_right_y;

    if (wd == 0){
	width = XMAX;
    }else{
    	width = wd;
    }
    if (ht == 0){
	height = YMAX;
    }else{
    	height = ht;
    }
    curve_pts = (Gpoint *)malloc(sizeof(Gpoint) *  gs_num);
    if (!curve_pts){
	perror("contrast_win_create(): Out of memory for curve_pts");
	exit(1);
    }

    contrast_popup =
    xv_create(owner, FRAME_CMD,
	      XV_X, x,
	      XV_Y, y,
	      WIN_SHOW, FALSE,
	      FRAME_LABEL, "Gamma Correction",
	      FRAME_CMD_PUSHPIN_IN, TRUE,
	      FRAME_SHOW_RESIZE_CORNER, TRUE,
	      NULL);

    contrast_panel = (Panel) xv_get(contrast_popup, FRAME_CMD_PANEL);
    xv_set(contrast_panel, PANEL_LAYOUT, PANEL_HORIZONTAL, NULL);

    show_solar =
    xv_create(contrast_panel, PANEL_CHECK_BOX,
	      PANEL_LAYOUT, PANEL_HORIZONTAL,
	      PANEL_NOTIFY_PROC, solar_set,
	      PANEL_LABEL_STRING, "Show Saturation:",
	      PANEL_VALUE, 1,
	      NULL);


    /*xv_create(contrast_panel, 	PANEL_BUTTON,
	      PANEL_NOTIFY_PROC,	contrast_reset,
	      PANEL_LABEL_STRING,	"Reset",
	      PANEL_ITEM_X_GAP, 100,
	      NULL);*/

    gamma_switch =
    xv_create(contrast_panel, PANEL_CHOICE,
	      PANEL_NEXT_ROW, -1,
	      PANEL_LABEL_STRING, "Gamma",
	      PANEL_CHOICE_STRINGS, "Off", "On", NULL,
	      PANEL_NOTIFY_PROC, gamma_switch_callback,
	      NULL);

    gamma_widget =
    xv_create(contrast_panel, PANEL_TEXT,
	      PANEL_LABEL_STRING, "Value:",
	      PANEL_VALUE, "5.0",
	      PANEL_VALUE_DISPLAY_LENGTH, 7,
	      PANEL_VALUE_STORED_LENGTH, 7,
	      PANEL_NOTIFY_PROC, gamma_callback,
	      NULL);
    state.gamma = 5.0;

    log_switch =
    xv_create(contrast_panel, PANEL_CHOICE,
	      PANEL_NEXT_ROW, -1,
	      PANEL_LABEL_STRING, "Log",
	      PANEL_CHOICE_STRINGS, "Off", "On", NULL,
	      PANEL_NOTIFY_PROC, log_switch_callback,
	      PANEL_VALUE, 1,
	      NULL);

    contrast_widget =
    xv_create(contrast_panel, PANEL_TEXT,
	      PANEL_LABEL_STRING, "Contrast:",
	      PANEL_VALUE, "100",
	      PANEL_VALUE_DISPLAY_LENGTH, 7,
	      PANEL_VALUE_STORED_LENGTH, 7,
	      PANEL_NOTIFY_PROC, contrast_callback,
	      NULL);
    state.contrast = 100;

    state.x0 = 0;
    state.x1 = gs_num - 1;
    state.y0 = 0;
    state.y1 = 255;

    window_fit_height(contrast_panel);

    contrast_canvas =
    xv_create(contrast_popup, 	CANVAS,
	      CANVAS_AUTO_EXPAND,	TRUE,
	      CANVAS_AUTO_SHRINK,	TRUE,
	      CANVAS_FIXED_IMAGE,	FALSE,
	      WIN_BELOW,		contrast_panel,
	      XV_HEIGHT,		height,
	      XV_WIDTH,		width,
	      CANVAS_HEIGHT,		height,
	      CANVAS_WIDTH,		width,
	      /*WIN_BORDER,		FALSE,
		OPENWIN_NO_MARGIN,	TRUE,*/
	      OPENWIN_SHOW_BORDERS,	FALSE,
	      XV_KEY_DATA,		CONTRAST_NOTIFY_FUNC, NULL,
	      XV_KEY_DATA,		SOLAR_NOTIFY_FUNC, NULL,
	      XV_KEY_DATA,		SISCMS,		siscms,
	      XV_KEY_DATA,		GDEV,		NULL,
	      XV_KEY_DATA,		GS_START,	gs_start,
	      XV_KEY_DATA,		NEW_CANVAS,	TRUE,
	      CANVAS_REPAINT_PROC,	contrast_canvas_repaint_proc,
	      CANVAS_RESIZE_PROC,	contrast_canvas_resize_proc,
	      NULL);

    gdev = g_device_create(contrast_canvas, siscms);
    xv_set(contrast_canvas, XV_KEY_DATA, GDEV, gdev, NULL);
    G_Set_Op(gdev, GXcopy);

    xv_set(canvas_paint_window(contrast_canvas),
	   WIN_EVENT_PROC, canvas_event_proc,
	   WIN_CONSUME_EVENTS,
	     LOC_DRAG,
	     ACTION_SELECT,
	     NULL,
	   NULL);
  
    xv_set(contrast_panel, XV_WIDTH, (int)xv_get(contrast_canvas, XV_WIDTH),
		NULL);

    window_fit(contrast_popup);

    state.xhandle = state.yhandle = 0;
    set_gamma_switch(0);
    set_log_switch(1);
    curve_func();
    contrast_canvas_repaint_proc();
}	/* end of contrast_win_create() */

static void
contrast_canvas_resize_proc(Canvas, int w, int h)
{
    width = w;
    height = h;
    build_background();
    build_curve_data();
}

static void
draw_curve()
{
    Gdev *gdev;
    int x;
    int y;

    gdev = (Gdev *)xv_get(contrast_canvas, XV_KEY_DATA, GDEV);
    g_draw_connected_lines(gdev, curve_pts, gs_num, FG_COLOR);

    x = (state.x0 * (width - 1)) / (gs_num-1);
    y = (height - 1) - (state.y0 * (height - 1)) / (MAXGRAY-1);
    mark(gdev, x, y, FG_COLOR);

    x = (state.x1 * (width - 1)) / (gs_num-1);
    y = (height - 1) - (state.y1 * (height - 1)) / (MAXGRAY-1);
    mark(gdev, x, y, FG_COLOR);

    if (state.ifgamma){
	x = (state.xhandle * (width - 1)) / (gs_num-1);
	y = (height - 1) - (state.yhandle * (height - 1)) / (MAXGRAY-1);
	mark(gdev, x, y, FG_COLOR);
    }
}

static void
draw_background()
{
    Gdev *gdev;

    if (!bkg_pixmap){
	build_background();
    }

    gdev = (Gdev *)xv_get(contrast_canvas, XV_KEY_DATA, GDEV);
    XCopyArea(gdev->xdpy, bkg_pixmap, gdev->xid, gdev->xgc, 0, 0,
	      width, height, 0, 0);
}

static void
build_curve_data()
{
    int i;
    int j;
    int k;
    Siscms *siscms;
    int gs_start;
    register u_char *r_ptr;

    /* Initialize pointer to grey_scale section of colormap */
    siscms  = (Siscms *) xv_get(contrast_canvas, XV_KEY_DATA, SISCMS);
    gs_start = (int) xv_get(contrast_canvas, XV_KEY_DATA, GS_START);
    r_ptr = siscms->r + gs_start;
    for (i=0; i<gs_num; i++){
	j = (i * (width - 1)) / (gs_num - 1);
	k = height - 1 - (*r_ptr++ * (height - 1)) / 255;
	if (j>= width){
	    fprintf(stderr,"*** j>width (%d > %d\n", j, width);
	    j = width - 1;
	}
	curve_pts[i].x = j;
	curve_pts[i].y = k;
    }
}

static void
build_background()
{
    Gdev * gdev;
    Siscms	*siscms;
    int 	gs_start;
    /*register u_char 	*r_ptr;*/
    int bands = 5;
    int i;
    int j;
    int k;
    int ii;
    int jj;
    int kk;
    int xcolor;
    int depth;
    XWindowAttributes   win_attr;

    /* retreive the graphics device handle from the canvas */
    gdev = (Gdev *)xv_get(contrast_canvas, XV_KEY_DATA, GDEV);

    /* Initialize pointer to grey_scale section of colormap */
    siscms  = (Siscms *) xv_get(contrast_canvas, XV_KEY_DATA, SISCMS);
    gs_start = (int) xv_get(contrast_canvas, XV_KEY_DATA, GS_START);
    xcolor = G_Get_Stcms2(gdev) + G_Get_Sizecms2(gdev) - 1;
    G_Set_Op(gdev,GXcopy);

    if (bkg_pixmap){
	XFreePixmap(gdev->xdpy, bkg_pixmap);
	bkg_pixmap = 0;
    }
    depth = DefaultDepth(gdev->xdpy, DefaultScreen(gdev->xdpy));
    if (XGetWindowAttributes(gdev->xdpy,  gdev->xid, &win_attr))
        depth = win_attr.depth;
/*
        depth = win_attr.visual->bits_per_rgb;
*/

    bkg_pixmap =
    XCreatePixmap(gdev->xdpy, gdev->xid,
		  width, height, depth);
    XID old_xid = gdev->xid;
    gdev->xid = bkg_pixmap;
    for (i=0; i<width; i++){
	j = (i * (gs_num - 1)) / width; /* where we are in colormap */
	for (ii=0, jj=1; ii<bands; ii++, jj*=2){
	    k = (j / jj) * jj;
	    g_draw_line(gdev,
			i, (ii*height)/bands,
			i, ((ii+1) * height)/bands,
			gs_start+k);
	}
    }
    gdev->xid = old_xid;
}

static void
contrast_canvas_repaint_proc()
{
    if (!contrast_canvas){
	return;
    }
    draw_background();
    draw_curve();
}

/*
 * Sets the solarization flag--whether to display saturated pixels in
 * a weird color.
 */
void
solarization(int flag)
{
    if (flag){
	xv_set(show_solar, PANEL_VALUE, 1, NULL);
    }else{
	xv_set(show_solar, PANEL_VALUE, 0, NULL);
    }
    solar_set();
}

void
solar_set()
{
    void (*func)(int);
    int flag;

    func = (void (*)(int))xv_get(contrast_canvas,
				 XV_KEY_DATA,
				 SOLAR_NOTIFY_FUNC);
    flag = (int)xv_get(show_solar, PANEL_VALUE);
    if (func){
	(*func)(flag);
    }
    contrast_set();
}


/* This function registers the solarization function in the contrast module. */
/* This is the function that gets called when the user changes the */
/* solarization value in the window */
void
solarization_notify_func(long solar_proc)
{
    xv_set(contrast_canvas,
	XV_KEY_DATA, SOLAR_NOTIFY_FUNC, solar_proc,
	NULL);
}

/* This function registers the contrast function in the contrast module. */
/* This is the function that gets called when the user changes the */
/* contrast in the window */
void
contrast_notify_func(long contrast_proc)
{
    xv_set(contrast_canvas,
	XV_KEY_DATA, CONTRAST_NOTIFY_FUNC, contrast_proc,
	NULL);
}	/* end of contrast_notify_func() */

/* This function shows the contrast command frame */
void
contrast_win_show()
{
    xv_set(contrast_popup, WIN_SHOW, TRUE, NULL);
}

void
canvas_event_proc(Xv_Window, Event *event)
{
    int delta;		 /* vertical change in gray scale line */
    register int x, y;	 /* location of mouse pointer in canvas */
    register int x1, y1; /* location of left end of line in canvas */
    register int x2, y2; /* location of right end of line in canvas */
    static int select_lock;/* tells if mouse is locked on left edge of line */
    register int x_gray; /* gray scale values of coordinate on canvas */
    register int y_gray; /* gray scale values of coordinate on canvas */
    float fx;
    float fy;

    x = (int) event_x(event);
    y = (int) event_y(event);

    switch (event_action(event))
    {
	case ACTION_SELECT :
	    if (event_is_down(event))
    	    /* find out whether we are changing the left end, right */
    	    /* end, or the middle. Right and left ends mean slope */
    	    /* changes. Middle just means gray center changes. */
	    {
  	    	/* Determine x,y pixel coordinates of left line end */
		x1 = (state.x0 * width) / (gs_num-1);
		y1 = height - (state.y0 * height) / (MAXGRAY-1);

  	    	/* Determine x,y pixel coordinates of right line end */
		x2 = (state.x1 * width) / (gs_num-1);
		y2 = height - (state.y1 * height) / (MAXGRAY-1);

	    	/* Then see if mouse pointer < 2*G_APERTURE pixels away */
	    	/* from either line end. */
		int tst = 2 * G_APERTURE;
		if ((abs(x-x1) < tst) && (abs(y-y1) < tst)){
		    select_lock = LEFT_END;
		}else if ((abs(x-x2) < tst) && (abs(y-y2) < tst)){
	    	    select_lock = RIGHT_END; 
		}else{
		    if (!state.ifgamma){
			select_lock = FALSE;
		    }else if (state.iflog){
			select_lock = GAMMALOG;
		    }else{
			select_lock = GAMMA;
		    }
		}
	    }else if(select_lock){
		/* (event_is_up) */
		contrast_canvas_repaint_proc();
		report_state();	// Notify client of new settings
		select_lock = FALSE;
		return;
	    }
	    break;

	case LOC_DRAG :
	    if (select_lock == FALSE)
		return;
	    break;

	/*case LOC_WINEXIT :
	    select_lock = FALSE;
	    return;*/

	default :
	    return;
    }

    /* Limit the action to inside the canvas */
    if (y < 0){
	y = 0;
    }else if (y > height-1){
	y = height - 1;
    }
    if (x < 0){
	x = 0;
    }else if (x > width-1){
	x = width -1 ;
    }

    switch (select_lock)
    {
      case LEFT_END:
	state.x0 = (x * (gs_num - 1)) / (width - 1);
	state.y0 = (height - 1 - y) * (MAXGRAY - 1) / (height - 1);
	break;

      case RIGHT_END:
	state.x1 = (x * (gs_num - 1)) / (width - 1);
	state.y1 = (height - 1 - y) * (MAXGRAY - 1) / (height - 1);
	break;

      case GAMMALOG:
	state.xhandle = (x * (gs_num - 1)) / (width - 1);
	state.yhandle = (height - 1 - y) * (MAXGRAY - 1) / (height - 1);
	// Set gamma to make the line pass through this point
	fx = (gs_num - 1) * (float)x / (width - 1);
	fx = (fx - state.x0) / (state.x1 - state.x0);
	fx *= log(state.contrast);
	fy = 255 * (1.0 - (float)y / (height - 1)) - state.y0;
	if (fy >= state.y1 - state.y0){
	    // Avoid division by 0
	    state.gamma = 15;
	}else{
	    state.gamma = (log((exp(fx) - 1)/(state.contrast - 1))
			      / log(fy / (state.y1 - state.y0)));
	}
	set_gamma(state.gamma);
	break;

      case GAMMA:
	state.xhandle = (x * (gs_num - 1)) / (width - 1);
	state.yhandle = (height - 1 - y) * (MAXGRAY - 1) / (height - 1);
	// Set gamma to make the line pass through this point
	fx = (gs_num - 1) * ((float)x / (width - 1));
	fx = (fx - state.x0) / (state.x1 - state.x0);
	fy = (MAXGRAY - 1) * (1.0 - (float)y / (height - 1));
	fy = (fy - state.y0) / (state.y1 - state.y0);
	if (fy >= 1){
	    state.gamma = 15;
	}else{
	    state.gamma = log(fx) / log(fy);
	}
	set_gamma(state.gamma);
	break;
    }

    curve_func();		// Load new colormap
    contrast_canvas_repaint_proc(); // Redraw everything
}

static void
set_gamma_switch(int ifon)
{
    state.ifgamma = ifon;
    xv_set(gamma_switch, PANEL_VALUE, ifon, NULL);
    xv_set(gamma_widget, PANEL_INACTIVE, !ifon, NULL);
    xv_set(log_switch, PANEL_INACTIVE, !ifon, NULL);
    xv_set(contrast_widget, PANEL_INACTIVE, !ifon || !state.iflog, NULL);
}

static void
gamma_switch_callback(Panel_item, int ifon, Event *)
{
    set_gamma_switch(ifon);
    curve_func();
    contrast_canvas_repaint_proc();
    report_state();
}

static void
gamma_callback(Panel_item, int, Event *)
{
    curve_func();
    contrast_canvas_repaint_proc();
    report_state();
}

static void
set_gamma(float value)
{
    char buf[20];

    if (value < 0.1) value = 0.1;
    if (value > 15.0) value = 15.0;
    state.gamma = value;
    sprintf(buf,"%.3f", value);
    xv_set(gamma_widget, PANEL_VALUE, buf, NULL);
}

static float
get_gamma()
{
    float numval;
    char *value = 0;
    
    value = (char *)xv_get(gamma_widget, PANEL_VALUE);
    sscanf(value,"%f", &numval);
    set_gamma(numval);
    return state.gamma;
}

static void
set_log_switch(int ifon)
{
    state.iflog = ifon;
    xv_set(contrast_widget, PANEL_INACTIVE, !ifon, NULL);
}

static void
log_switch_callback(Panel_item, int ifon, Event *)
{
    set_log_switch(ifon);
    curve_func();
    contrast_canvas_repaint_proc();
    report_state();
}

static void
contrast_callback(Panel_item, int, Event *)
{
    curve_func();
    contrast_canvas_repaint_proc();
    report_state();
}

static void
set_contrast(float value)
{
    char buf[20];

    if (value < 0) value = 0;
    if (value > 1.0e6) value = 1.0e6;
    state.contrast = value;
    sprintf(buf,"%.0f", value);
    xv_set(contrast_widget, PANEL_VALUE, buf, NULL);
}

static int
get_contrast()
{
    float numval;
    char buf[20];
    char *value = 0;
    
    value = (char *)xv_get(contrast_widget, PANEL_VALUE);
    sscanf(value,"%f", &numval);
    if (numval < 2.0) numval = 2.0;
    if (numval > 1.0e6) numval = 1.0e6;
    state.contrast = (int)(numval + 0.5);
    sprintf(buf,"%d", state.contrast);
    xv_set(contrast_widget, PANEL_VALUE, buf, NULL);
    return state.contrast;
}

static void 
curve_func()
{
    register int 	i;
    register u_char 	*r_ptr;
    register u_char 	*g_ptr;
    register u_char 	*b_ptr;
    register float 	slopef;
    register float	x;
    register int	value;
    float max_contrast;
    float gamma;
    float y0;
    float ydelta;
    float beta;

    Gdev	*gdev;
    Siscms	*siscms;
    int 	gs_start;

    max_contrast = (float)get_contrast();
    gamma = get_gamma();

    /* get the local variables from the canvas XV_KEY_DATA fields */
    siscms  = (Siscms *) xv_get(contrast_canvas, XV_KEY_DATA, SISCMS);
    gs_start = (int) xv_get(contrast_canvas, XV_KEY_DATA, GS_START);

    /* initialize pointers to grey_scale section of colormap */
    r_ptr = siscms->r + gs_start; 
    g_ptr = siscms->g + gs_start; 
    b_ptr = siscms->b + gs_start; 

    /* Calculate handy constants. */
    y0 = state.y0;
    ydelta = state.y1 - state.y0;
    beta = (max_contrast - 1) / pow(ydelta, gamma);

    if (state.gamma){
	// Position the gamma adjustment handle
	if (state.xhandle <= 1){
	    state.xhandle = (2 * state.x0 + state.x1) / 3;
	}
    }

    /* now calculate the new colormap values */
    for (i=0; i < gs_num; i++, r_ptr++, g_ptr++, b_ptr++){
	if (i <= state.x0){
	    value = state.y0;
	}else if (i >= state.x1){
	    value = state.y1;
	}else{
	    x = (float)(i - state.x0) / (state.x1 - state.x0);

	    if (state.ifgamma){
		if (state.iflog){
		    // Gamma correction w/ log scaling
		    x *= log(max_contrast);
		    value = (int)(pow((exp(x) - 1)/beta, 1/gamma) + y0 + 0.5);
		}else{
		    // Gamma correction w/ linear scaling
		    value = ydelta * pow(x, 1/gamma) + y0 + 0.5;
		}
		if (i == state.xhandle){
		    state.yhandle = value;
		}
	    }else{
		// Linear correction
		value = ydelta * x + y0 + 0.5;
	    }
	}
	if (low_mask_on > 0 && i < min_grey_shown
	    || low_mask_on < 0 && i > min_grey_shown)
	{
	    *r_ptr = red_mask;
	    *g_ptr = green_mask;
	    *b_ptr = blue_mask;
	}else if (high_mask_on > 0 && i > max_grey_shown
		  || high_mask_on < 0 && i < max_grey_shown)
	{
	    *r_ptr = red_himask;
	    *g_ptr = green_himask;
	    *b_ptr = blue_himask;
	}else if (value < 0){
	    *r_ptr = *g_ptr = *b_ptr = 0;
	}else if (value > 255){
	    *r_ptr = *g_ptr = *b_ptr = (u_char)255;
	}else{
	    *r_ptr = *g_ptr = *b_ptr = (u_char)value;
	}
    }
    if ( (int)xv_get(show_solar, PANEL_VALUE) ){
	*(--r_ptr) = red_solar;
	*(--g_ptr) = green_solar;
	*(--b_ptr) = blue_solar;
    }
    
    /* Load the colormap into canvas */
    gdev = (Gdev *) xv_get (contrast_canvas, XV_KEY_DATA, GDEV);
    G_Update_Colormap(gdev);

    build_curve_data();
}

static void 
mark(Gdev *gdev, int x, int y, int color)
{
    g_draw_rect(gdev, x-3, y-3, 6, 6, color);
}

void
contrast_set()
{
    curve_func();
    contrast_canvas_repaint_proc();
}

void
set_contrast(float lefty, float righty, float gamma, float contrast)
{
    float x0 = 0;
    float x1 = 1;
    float y0 = 0;
    float y1 = 1;

    if (lefty != righty){
	if (lefty > 1 ){
	    if (righty < 1){
		y0 = 1;
		x0 = (1 - lefty) / (righty - lefty);
	    }
	}else if (lefty < 0){
	    if (righty > 0){
		y0 = 0;
		x0 = (0 - lefty) / (righty - lefty);
	    }
	}else{
	    y0 = lefty;
	    x0 = 0;
	}

	if (righty > 1){
	    if (lefty < 1){
		y1 = 1;
		x1 = (1 - lefty) / (righty - lefty);
	    }
	}else if (righty < 0){
	    if (lefty > 0){
		y1 = 0;
		x1 = (0 - lefty) / (righty - lefty);
	    }
	}else{
	    y1 = righty;
	    x1 = 1;
	}

	state.x0 = x0 * (gs_num - 1);
	state.x1 = x1 * (gs_num - 1);
	state.y0 = y0 * (MAXGRAY - 1);
	state.y1 = y1 * (MAXGRAY - 1);
    }

    if (gamma == 0){
	// Turn off gamma correction
	set_gamma_switch(FALSE);
    }else if (contrast == 0){
	set_gamma(gamma);
	set_gamma_switch(TRUE);
	set_log_switch(FALSE);
    }else{
	set_gamma(gamma);
	set_contrast(contrast);
	set_gamma_switch(TRUE);
	set_log_switch(TRUE);
    }
    curve_func();
    contrast_canvas_repaint_proc();

    report_state();
}

void
contrast_mask_on(int min_active, int min, int max_active, int max)
{
    int gs_start = (int) xv_get(contrast_canvas, XV_KEY_DATA, GS_START);
    low_mask_on = min_active;
    min_grey_shown = min - gs_start;
    high_mask_on = max_active;
    max_grey_shown = max - gs_start;
    contrast_set();
}

void
contrast_mask_off()
{
    low_mask_on = high_mask_on = FALSE;
    contrast_set();
}

void
report_state()
{
    void (*func)(float, float, float, float);
    float fy0;
    float fy1;
    float fg = 0;		// gamma
    float fc = 0;		// monitor contrast

    func = (void (*)(float, float, float, float))xv_get(contrast_canvas,
							XV_KEY_DATA,
							CONTRAST_NOTIFY_FUNC);
    fy0 = state.y0 + ((state.y0 - state.y1)
		      * (float)(-state.x0)
		      / (state.x0 - state.x1));
    fy1 = state.y1 + ((state.y1 - state.y0)
		      * (float)(gs_num - 1 - state.x1)
		      / (state.x1 - state.x0));
    if (state.ifgamma){
	fg = state.gamma;
	if (state.iflog){
	    fc = state.contrast;
	}
    }
    if (func){
	(*func)( fy0/(MAXGRAY - 1), fy1/(MAXGRAY - 1), fg, fc);
    }
}    

void
save_colormap()
{
    /* Get the local variables from the canvas XV_KEY_DATA fields */
    Siscms *siscms  = (Siscms *) xv_get(contrast_canvas, XV_KEY_DATA, SISCMS);
    int gs_start = (int) xv_get(contrast_canvas, XV_KEY_DATA, GS_START);
    int nbytes = gs_num * sizeof(u_char);

    /* Initialize pointers to grey_scale section of colormap */
    u_char *r_ptr = siscms->r + gs_start; 
    u_char *g_ptr = siscms->g + gs_start; 
    u_char *b_ptr = siscms->b + gs_start; 

    memcpy(colormap_mem.red, siscms->r + gs_start, nbytes);
    memcpy(colormap_mem.green, siscms->g + gs_start, nbytes);
    memcpy(colormap_mem.blue, siscms->b + gs_start, nbytes);
}

void
restore_colormap(float xmin,	/* y = 0 intercept, for 0<=x<=1 */
		 float xmax)	/* y = gs_num-1 intercept */
{
    int x0 = (int)rint(xmin * (gs_num-1));
    int x1 = (int)rint(xmax * (gs_num-1));

    /* Get the local variables from the canvas XV_KEY_DATA fields */
    Siscms *siscms  = (Siscms *) xv_get(contrast_canvas, XV_KEY_DATA, SISCMS);
    int gs_start = (int) xv_get(contrast_canvas, XV_KEY_DATA, GS_START);
    Gdev *gdev = (Gdev *) xv_get (contrast_canvas, XV_KEY_DATA, GDEV);

    /* Initialize pointers to grey_scale section of colormap */
    u_char *r_ptr = siscms->r + gs_start; 
    u_char *g_ptr = siscms->g + gs_start; 
    u_char *b_ptr = siscms->b + gs_start; 
    u_char *mr_ptr = colormap_mem.red;
    u_char *mg_ptr = colormap_mem.green;
    u_char *mb_ptr = colormap_mem.blue;
    if (x0 == 0 && x1 == gs_num-1){
	int nbytes = gs_num * sizeof(u_char);
	memcpy(r_ptr, mr_ptr, nbytes);
	memcpy(g_ptr, mg_ptr, nbytes);
	memcpy(b_ptr, mb_ptr, nbytes);
    }else{
	u_char *mem_end = mr_ptr + gs_num;
	u_char *gs_end;
	/* Fill part <= x0 with first value */
	if (x0 <= 0){
	    gs_end = r_ptr;
	}else if (x0 >= gs_num-1){
	    gs_end = r_ptr + gs_num - 1;
	}else{
	    gs_end = r_ptr + x0;
	}
	while (r_ptr < gs_end){
	    *r_ptr++ = *mr_ptr;
	    *g_ptr++ = *mg_ptr;
	    *b_ptr++ = *mb_ptr;
	}
	
	/* Fill part between x0 and x1 */
	float dxmem = (gs_num - 1.0)/ (x1 - x0);
	float xmem = dxmem;
	gs_end = siscms->r + gs_start; /* Reset to beginning */
	if (x1 > gs_num-1){
	    gs_end += gs_num - 1;
	}else if (x1 > 0){
	    gs_end += x1;
	}
	int ioff = xmin >= 0 ? 0 : (int)((gs_num - 1) * xmin / (xmin - xmax));
	int ix;
	while (r_ptr < gs_end){
	    ix = (int)floor(xmem);
	    if (ix + ioff >= gs_num - 1){
		*r_ptr++ = *(mr_ptr + gs_num - 1);
		*g_ptr++ = *(mg_ptr + gs_num - 1);
		*b_ptr++ = *(mb_ptr + gs_num - 1);
	    }else{
		*r_ptr++ = LIN_INTERP(xmem - ix, mr_ptr + ix + ioff);
		*g_ptr++ = LIN_INTERP(xmem - ix, mg_ptr + ix + ioff);
		*b_ptr++ = LIN_INTERP(xmem - ix, mb_ptr + ix + ioff);
	    }
	    xmem += dxmem;
	}
	
	/* Fill part >= x1 */
	gs_end = siscms->r + gs_start + gs_num - 1; /* End of colormap */
	while (r_ptr <= gs_end){
	    *r_ptr++ = (int)rint( *(mr_ptr + gs_num - 1) );
	    *g_ptr++ = (int)rint( *(mg_ptr + gs_num - 1) );
	    *b_ptr++ = (int)rint( *(mb_ptr + gs_num - 1) );
	}
    }
    /* Load the colormap into canvas */
    G_Update_Colormap(gdev);
}
