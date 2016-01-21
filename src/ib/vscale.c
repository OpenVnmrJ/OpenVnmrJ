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

#include <stdio.h>
#include <string.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <math.h>

#include "graphics.h"
#include "stderr.h"
#include "vscale.h"

#define LIN_KNOTS 0
#define SPL_KNOTS 0
#define LOG_KNOTS 1
#define PWR_KNOTS 2
#define CRV_KNOTS 3
#define NUL_KNOTS 4

struct Fpoint{
    float x;
    float y;
};

typedef struct _VsInfo{
    Gdev *gdev;
    Siscms *cm;
    Frame popup;
    Panel panel;
    Panel_item func_choice;
    Panel_item scale;
    Panel_item exponent;
    Panel_item power;
    Panel_item negative;
    int negative_val;
    Panel_item min_data;
    float min_data_val;
    Panel_item range_label;
    Panel_item max_data;
    float max_data_val;
    Panel_item oflow_color;
    Panel_item uflow_color;
    Canvas canvas;
    int gs_start;	/* Index of start of greyscale map */
    int gs_num;		// Number of greyscale entries

    int canvas_width;	/* width of canvas */
    int canvas_height; /* height of canvas */
    Gpoint *curve_pts;
    int n_curve_pts;
    Pixmap bkg_pixmap;
    void (*notify_func)(VsFunc *);
    Fpoint *knotlists[NUL_KNOTS];
    int nknots[3];
    int myknots;
    int uflow_cmi;
    int oflow_cmi;
    char *command;
    VsFunc *default_vsfunc;
} VsInfo;

static VsInfo vsi;

#define BG_COLOR 0
#define FG_COLOR 1

#define MAXGRAY 255

#define LEFT_END 1
#define RIGHT_END 2

static Menu make_color_menu(void(*callback)(Menu, Menu_item));
static void oflow_callback(Menu, Menu_item);
static void uflow_callback(Menu, Menu_item);
static void mark(int x, int y, int color);
static void canvas_repaint_proc();
static void canvas_resize_proc(Canvas, int, int);
static void canvas_event_proc(Xv_Window, Event *event);
static void build_background();
static void draw_curve();
static void draw_knots(int n, Fpoint *list);
static void mark(Gdev *, int x, int y, int color);
static void draw_background();
static void build_curve_data(VsFunc *);
static void report_state();
static void update_callback(Panel_item, int, Event *);
static void settings_update();
static void func_update();
static void set_ouflow(VsFunc *);
static VsFunc *linear_scale();
static VsFunc *spline_scale();
static VsFunc *log_scale();
static VsFunc *power_scale();
static VsFunc *curve_scale();
static void delete_knot(int index);
static int create_knot(float x, float y);
void spline_points_interp(int n0, float *xa, float *ya, float *y2a, int n, float *y);
extern void cspline(float *x, float *y, int n, float yp1, float ypn, float *y2);
static float knot2pwrparm(float x, float y);
static float knot2logparm(float x, float y);
static Fpoint exp2knot(float curvature);
static Fpoint pwr2knot(float power);
static void set_func_command();
static void set_negative_command();
static void set_domain_command();
static char * knotlist2string(char *, int n, Fpoint *list);
static float bound(float in, float bound1, float bound2);

/* This function create the vscale command frame, panel and canvas, */
/* initialized to the variables indicated by the user. */
void
vs_win_create(Frame owner,	/* owner frame handle */
	      int x,		/* x location of window */
	      int y,		/* y location of window */
	      int wd,		/* width of canvas */
	      int ht,		/* height of canvas */
	      Siscms *siscms,	/* SIS color map */
	      int gs_start,	/* start index of greyscale map */
	      int gs_size)	/* size of greyscale map */
{
    Menu color_menu;

    /* Initialize variables */
    vsi.cm = siscms;
    vsi.gs_start = gs_start;
    vsi.gs_num = gs_size;

    if (wd == 0){
	vsi.canvas_width = 350;
    }else{
    	vsi.canvas_width = wd;
    }
    if (ht == 0){
	vsi.canvas_height = 350;
    }else{
    	vsi.canvas_height = ht;
    }

    vsi.curve_pts = (Gpoint *)malloc(sizeof(Gpoint) *  65536 * 2);
    vsi.n_curve_pts = 0;/*CMP*/
    if (!vsi.curve_pts){
	perror("vs_win_create(): Out of memory for curve_pts");
	exit(1);
    }

    vsi.popup =
    xv_create(owner, FRAME_CMD,
	      XV_X, x,
	      XV_Y, y,
	      WIN_SHOW, FALSE,
	      FRAME_LABEL, "Vertical Scaling",
	      FRAME_CMD_PUSHPIN_IN, TRUE,
	      FRAME_SHOW_RESIZE_CORNER, TRUE,
	      FRAME_SHOW_FOOTER, TRUE,
	      NULL);

    vsi.panel = (Panel) xv_get(vsi.popup, FRAME_CMD_PANEL);
    xv_set(vsi.panel, PANEL_LAYOUT, PANEL_VERTICAL, NULL);

    vsi.func_choice =
    xv_create(vsi.panel, PANEL_CHOICE,
	      PANEL_LAYOUT, PANEL_VERTICAL,
	      PANEL_CHOICE_STRINGS, "Curve", "Linear", "Spline", NULL,
	      PANEL_NOTIFY_PROC, update_callback,
	      NULL);

    vsi.negative =
    xv_create(vsi.panel, PANEL_TOGGLE,
	      /*PANEL_NEXT_COL, -1,*/
	      PANEL_CHOICE_STRINGS, "Negative", NULL,
	      PANEL_NOTIFY_PROC, update_callback,
	      NULL);

    /*Rect *rect = (Rect*)xv_get(vsi.func_choice, PANEL_CHOICE_RECT, 2);
    vsi.exponent =
    xv_create(vsi.panel, PANEL_TEXT,
	      XV_X, rect->r_left + rect->r_width + 5,
	      XV_Y, rect->r_top + 5,
	      PANEL_VALUE, "0",
	      PANEL_VALUE_DISPLAY_LENGTH, 7,
	      PANEL_VALUE_STORED_LENGTH, 7,
	      PANEL_NOTIFY_PROC, update_callback,
	      NULL);

    rect = (Rect*)xv_get(vsi.func_choice, PANEL_CHOICE_RECT, 3);
    vsi.power =
    xv_create(vsi.panel, PANEL_TEXT,
	      XV_X, rect->r_left + rect->r_width + 5,
	      XV_Y, rect->r_top + 5,
	      PANEL_VALUE, "1",
	      PANEL_VALUE_DISPLAY_LENGTH, 7,
	      PANEL_VALUE_STORED_LENGTH, 7,
	      PANEL_NOTIFY_PROC, update_callback,
	      NULL);
	      */

    // Dummy menu--the real one is created and attached below
    color_menu = (Menu)xv_create(NULL, MENU,
				 NULL);
    vsi.oflow_color =
    xv_create(vsi.panel, PANEL_BUTTON,
	      PANEL_NEXT_COL, -1,
	      PANEL_ITEM_MENU, color_menu,
	      PANEL_LABEL_STRING, "Overflow",
	      NULL);

    vsi.uflow_color =
    xv_create(vsi.panel, PANEL_BUTTON,
	      PANEL_ITEM_MENU, color_menu,
	      PANEL_LABEL_STRING, "Underflow",
	      NULL);

    vsi.min_data =
    xv_create(vsi.panel, PANEL_TEXT,
	      XV_X, 0,
	      XV_Y, ((int)xv_get(vsi.negative, XV_Y)
		     + (int)xv_get(vsi.negative, XV_HEIGHT) + 10),
	      PANEL_VALUE, "0",
	      PANEL_VALUE_DISPLAY_LENGTH, 10,
	      PANEL_VALUE_STORED_LENGTH, 10,
	      PANEL_NOTIFY_PROC, update_callback,
	      NULL);

    vsi.range_label =
    xv_create(vsi.panel, PANEL_MESSAGE,
	      XV_Y, (int)xv_get(vsi.min_data, XV_Y),
	      PANEL_LABEL_STRING, "<- Data Range ->",
	      PANEL_LABEL_BOLD, FALSE,
	      NULL);
    xv_set(vsi.range_label,
	   XV_X, (vsi.canvas_width - (int)xv_get(vsi.range_label, XV_WIDTH))/2,
	   NULL);

    vsi.max_data =
    xv_create(vsi.panel, PANEL_TEXT,
	      XV_Y, (int)xv_get(vsi.min_data, XV_Y),
	      PANEL_VALUE, "0.03",
	      PANEL_VALUE_DISPLAY_LENGTH, 10,
	      PANEL_VALUE_STORED_LENGTH, 10,
	      PANEL_NOTIFY_PROC, update_callback,
	      NULL);
    xv_set(vsi.max_data,
	   XV_X, vsi.canvas_width - (int)xv_get(vsi.max_data, XV_WIDTH),
	   NULL);

    window_fit_height(vsi.panel);

    vsi.canvas =
    xv_create(vsi.popup, 	CANVAS,
	      CANVAS_AUTO_EXPAND,	TRUE,
	      CANVAS_AUTO_SHRINK,	TRUE,
	      CANVAS_FIXED_IMAGE,	FALSE,
	      WIN_BELOW,		vsi.panel,
	      XV_HEIGHT,		vsi.canvas_height,
	      XV_WIDTH,			vsi.canvas_width,
	      CANVAS_HEIGHT,		vsi.canvas_height,
	      CANVAS_WIDTH,		vsi.canvas_width,
	      OPENWIN_SHOW_BORDERS,	FALSE,
	      CANVAS_REPAINT_PROC,	canvas_repaint_proc,
	      CANVAS_RESIZE_PROC,	canvas_resize_proc,
	      NULL);

    vsi.gdev = g_device_create(vsi.canvas, vsi.cm);
    G_Set_Op(vsi.gdev, GXcopy);

    color_menu = make_color_menu(oflow_callback);
    xv_set(vsi.oflow_color, PANEL_ITEM_MENU, color_menu, NULL);
    color_menu = make_color_menu(uflow_callback);
    xv_set(vsi.uflow_color, PANEL_ITEM_MENU, color_menu, NULL);

    xv_set(canvas_paint_window(vsi.canvas),
	   WIN_EVENT_PROC, canvas_event_proc,
	   WIN_CONSUME_EVENTS,
	     LOC_DRAG,
	     ACTION_SELECT,
	     NULL,
	   NULL);

    window_fit(vsi.canvas);
    xv_set(vsi.panel, XV_WIDTH, (int)xv_get(vsi.canvas, XV_WIDTH), NULL);
    window_fit(vsi.popup);

    vsi.uflow_cmi = vsi.gs_start;
    vsi.oflow_cmi = vsi.gs_start + vsi.gs_num - 1;

    // Note that LIN_KNOTS are same as SPL_KNOTS
    vsi.knotlists[LIN_KNOTS] = (Fpoint *)malloc(4 * sizeof(Fpoint));
    vsi.knotlists[LOG_KNOTS] = (Fpoint *)malloc(3 * sizeof(Fpoint));
    vsi.knotlists[PWR_KNOTS] = (Fpoint *)malloc(3 * sizeof(Fpoint));
    vsi.knotlists[CRV_KNOTS] = (Fpoint *)malloc(3 * sizeof(Fpoint));
    if (!vsi.knotlists[LIN_KNOTS]
	|| !vsi.knotlists[LOG_KNOTS]
	|| !vsi.knotlists[PWR_KNOTS]
	|| !vsi.knotlists[CRV_KNOTS] )
    {
	perror("vs_win_create(): Out of memory for knotlists");
	exit(1);
    }
    vsi.nknots[LIN_KNOTS] = 4;
    vsi.knotlists[LIN_KNOTS][0].x = vsi.knotlists[LIN_KNOTS][0].y = 0;
    vsi.knotlists[LIN_KNOTS][1].x = vsi.knotlists[LIN_KNOTS][1].y = 0.33;
    vsi.knotlists[LIN_KNOTS][2].x = vsi.knotlists[LIN_KNOTS][2].y = 0.67;
    vsi.knotlists[LIN_KNOTS][3].x = vsi.knotlists[LIN_KNOTS][3].y = 1;

    vsi.nknots[LOG_KNOTS] = 3;
    vsi.knotlists[LOG_KNOTS][0].x = vsi.knotlists[LOG_KNOTS][0].y = 0;
    vsi.knotlists[LOG_KNOTS][1].x = vsi.knotlists[LOG_KNOTS][1].y = 0.5;
    vsi.knotlists[LOG_KNOTS][2].x = vsi.knotlists[LOG_KNOTS][2].y = 1;

    vsi.nknots[PWR_KNOTS] = 3;
    vsi.knotlists[PWR_KNOTS][0].x = vsi.knotlists[PWR_KNOTS][0].y = 0;
    vsi.knotlists[PWR_KNOTS][1].x = vsi.knotlists[PWR_KNOTS][1].y = 0.5;
    vsi.knotlists[PWR_KNOTS][2].x = vsi.knotlists[PWR_KNOTS][2].y = 1;

    vsi.nknots[CRV_KNOTS] = 3;
    vsi.knotlists[CRV_KNOTS][0].x = vsi.knotlists[CRV_KNOTS][0].y = 0;
    vsi.knotlists[CRV_KNOTS][1].x = vsi.knotlists[CRV_KNOTS][1].y = 0.5;
    vsi.knotlists[CRV_KNOTS][2].x = vsi.knotlists[CRV_KNOTS][2].y = 1;


    func_update();
}	/* end of vs_win_create() */

static void
oflow_callback(Menu menu, Menu_item mi)
{
    int cmi = (int)xv_get(mi, MENU_VALUE);
    if (cmi<0){
	int i = vsi.default_vsfunc->lookup->size;
	cmi = vsi.default_vsfunc->lookup->table[i-1];
    }
    vsi.oflow_cmi = vsi.default_vsfunc->oflow_cmi = cmi;
    report_state();
}

static void
uflow_callback(Menu menu, Menu_item mi)
{
    int cmi = (int)xv_get(mi, MENU_VALUE);
    if (cmi<0){
	cmi = vsi.default_vsfunc->lookup->table[0];
    }
    vsi.uflow_cmi = vsi.default_vsfunc->uflow_cmi = cmi;
    report_state();
}

static Menu
make_color_menu(void(*callback)(Menu, Menu_item))
{
    int i;
    Menu_item mi;
    Menu rtn;

    int ncolors = G_Get_Sizecms1(vsi.gdev);
    int ncols = 2;
    int nrows = 1 + (ncolors - 1) / ncols;

    rtn = (Menu)xv_create(NULL, MENU_COMMAND_MENU,
			  MENU_NOTIFY_PROC, callback,
			  MENU_NCOLS, ncols,
			  MENU_NROWS, nrows,
			  NULL);

    mi = (Menu_item)xv_create(NULL, MENUITEM,
			      MENU_STRING, "Off",
			      MENU_VALUE, (Xv_opaque)(-1),
			      NULL);
    xv_set(rtn, MENU_APPEND_ITEM, mi,
	   MENU_NCOLS, ncols,
	   MENU_NROWS, nrows,
	   NULL);

    for (i=0; i<ncolors; i++){
	mi = (Menu_item)xv_create(NULL, MENUITEM,
				  MENU_IMAGE, get_color_chip(i),
				  MENU_VALUE, (Xv_opaque)i,
				  NULL);
	xv_set(rtn, MENU_APPEND_ITEM, mi,
	       MENU_NCOLS, ncols,
	       MENU_NROWS, nrows,
	       NULL);
    }
    return rtn;
}

static void
canvas_resize_proc(Canvas, int w, int h)
{
    /*fprintf(stderr,"canvas_resize_proc(%d, %d)\n", w, h);/*CMP*/
    vsi.canvas_width = w;
    vsi.canvas_height = h;
    xv_set(vsi.range_label,
	   XV_X, (vsi.canvas_width - (int)xv_get(vsi.range_label, XV_WIDTH))/2,
	   NULL);
    xv_set(vsi.max_data,
	   XV_X, vsi.canvas_width - (int)xv_get(vsi.max_data, XV_WIDTH),
	   NULL);
    build_background();
    build_curve_data(vsi.default_vsfunc);
}

static void
draw_curve()
{
    int x;
    int y;

    /*fprintf(stderr,"draw_curve()\n");/*CMP*/
    //g_draw_connected_lines(vsi.gdev, vsi.curve_pts, vsi.gs_num + 1, FG_COLOR);
    g_draw_connected_lines(vsi.gdev, vsi.curve_pts, vsi.n_curve_pts, FG_COLOR);
    draw_knots(vsi.nknots[vsi.myknots], vsi.knotlists[vsi.myknots]);
}

static void
draw_knots(int n, Fpoint *list)
{
    int i;
    int h = vsi.canvas_height;
    int w = vsi.canvas_width - 1;
    int x;
    int y;

    for (i=0; i<n; i++){
	x = (int)(list[i].x * w);
	y = (int)(h - list[i].y * h);
	mark(vsi.gdev, x, y, FG_COLOR);
    }
}

static void
mark(Gdev *gdev, int x, int y, int color)
{
    //g_draw_rect(gdev, x-3, y-3, 6, 6, color);
    G_Set_Color(gdev, color);
    XFillRectangle(gdev->xdpy, gdev->xid, gdev->xgc, x-3, y-3, 7, 7);
}

static void
draw_background()
{
    /*fprintf(stderr,"draw_background()\n");/*CMP*/
    if (!vsi.bkg_pixmap){
	build_background();
    }

    XCopyArea(vsi.gdev->xdpy, vsi.bkg_pixmap, vsi.gdev->xid, vsi.gdev->xgc,
	      0, 0, vsi.canvas_width, vsi.canvas_height, 0, 0);
}

static void
build_curve_data(VsFunc *vsfunc)
{
    int i;
    int j;
    int icol = vsi.gs_start;
    float ncols = vsi.gs_num;
    float h = vsi.canvas_height;
    float w = vsi.canvas_width - 1;
    u_char *tbl = vsfunc->lookup->table;
    int tsize = vsfunc->lookup->size;

    /*fprintf(stderr,"build_curve_data()\n");/*CMP*/
    int col = tbl[0];
    vsi.curve_pts[0].x = 0;
    vsi.curve_pts[0].y = (int)(h - h / ncols * (0.5 + col - icol) + 0.5);
    for (i=j=1; i<tsize; i++){
	if (tbl[i] != col){
	    vsi.curve_pts[j].x = (int)(i * w / tsize + 0.5);
	    vsi.curve_pts[j].y = vsi.curve_pts[j-1].y;
	    j++;
	    col = tbl[i];
	    vsi.curve_pts[j].x = vsi.curve_pts[j-1].x;
	    vsi.curve_pts[j].y = (int)(h - h / ncols * (0.5 + col - icol) + 0.5);
	    j++;
	}
    }
    vsi.curve_pts[j].x = (int)w;
    vsi.curve_pts[j].y = vsi.curve_pts[j-1].y;
    vsi.n_curve_pts = j + 1;
    if (vsi.negative_val){
	for (i=0; i<vsi.n_curve_pts; i++){
	    vsi.curve_pts[i].y = h - vsi.curve_pts[i].y;
	}
    }
}

static void
build_background()
{
    int bands = 5;
    int i;
    int j;
    int k;
    int xcolor;
    int	depth;
    XWindowAttributes  win_attr;

    /*fprintf(stderr,"build_background()\n");/*CMP*/
    xcolor = G_Get_Stcms2(vsi.gdev) + G_Get_Sizecms2(vsi.gdev) - 1;
    G_Set_Op(vsi.gdev, GXcopy);

    if (vsi.bkg_pixmap){
	XFreePixmap(vsi.gdev->xdpy, vsi.bkg_pixmap);
	vsi.bkg_pixmap = 0;
    }
    depth = DefaultDepth(vsi.gdev->xdpy, DefaultScreen(vsi.gdev->xdpy));
    if (XGetWindowAttributes(vsi.gdev->xdpy, vsi.gdev->xid, &win_attr))
        depth = win_attr.depth;
/*
        depth = win_attr.visual->bits_per_rgb;
*/

    vsi.bkg_pixmap =
    XCreatePixmap(vsi.gdev->xdpy, vsi.gdev->xid,
		  vsi.canvas_width, vsi.canvas_height, depth);
    XID old_xid = vsi.gdev->xid;
    vsi.gdev->xid = vsi.bkg_pixmap;
    for (i=0; i<vsi.canvas_height; i++){
	k = vsi.negative_val ? i : vsi.canvas_height - i - 1;
	j = ( k * vsi.gs_num ) / vsi.canvas_height;
	g_draw_line(vsi.gdev, 0, i, vsi.canvas_width-1, i, vsi.gs_start+j);
    }
    vsi.gdev->xid = old_xid;
}

static void
canvas_repaint_proc()
{
    /*fprintf(stderr,"repaint()\n");/*CMP*/
    if (!vsi.canvas){
	return;
    }

    draw_background();
    draw_curve();
}

/* This function registers the vscale function in the vscale module. */
/* This is the function that gets called when the user changes the */
/* vscale in the window */
void
vs_notify_func(void (*vs_proc)(VsFunc *))
{
    vsi.notify_func = vs_proc;
}

/* This function shows the vscale command frame */
void
vs_win_show()
{
    xv_set(vsi.popup, WIN_SHOW, TRUE, NULL);
}

static void
canvas_event_proc(Xv_Window, Event *event)
{
    int i;
    register int x, y;		// location of mouse pointer on canvas
    float fx;			// Normalized location of mouse on canvas
    float fy;
    register int x0, y0;
    float dis;
    static int knot = -1;	// Which knot mouse controls (-1=>none)
    static float mctl = 0.5;	// Relative Y distance of ctl point from here
    static float fyother = 0;	// Y position of other line end

    x = (int) event_x(event);
    y = (int) event_y(event);
    fx = (float)x / (vsi.canvas_width - 1);
    fy = (float)(vsi.canvas_height - 1 - y) / (vsi.canvas_height - 1);

    switch (event_action(event)){
      case ACTION_SELECT:
      case ACTION_ADJUST:
	if (event_is_down(event)){
	    /*fprintf(stderr,"%.4f, %.4f, k=%.3f\n",
		    fx, fy, knot2logparm(fx, fy));/*CMP*/
	    /* Find out which knot we want to change */
	    /* See if mouse pointer <= 2 * G_APERTURE pixels from a knot. */
	    int tst = 4 * G_APERTURE * G_APERTURE;
	    knot = -1;
	    for (i=0; i<vsi.nknots[vsi.myknots]; i++){
		x0 = (int)(vsi.knotlists[vsi.myknots][i].x
			   * (vsi.canvas_width - 1) + 0.5);
		y0 = (int)(vsi.canvas_height
			   * (1-vsi.knotlists[vsi.myknots][i].y) + 0.5);
		if ((dis=(x-x0)*(x-x0) + (y-y0)*(y-y0)) <= tst){
		    knot = i;
		    tst = dis;
		}
	    }
	    if (vsi.myknots == LIN_KNOTS || vsi.myknots == SPL_KNOTS){
		if (event_action(event) == ACTION_SELECT){
		    if (knot < 0){
			knot = create_knot(fx, fy);
		    }
		}else{		// ACTION_ADJUST
		    // Cannot delete end points
		    if (knot > 0 && knot < vsi.nknots[vsi.myknots]-1){
			delete_knot(knot);
		    }
		    knot = -1;
		}
	    }else if (vsi.myknots == CRV_KNOTS){
		// Remember relative position of control knot
		if (knot == 0){
		    fyother = vsi.knotlists[vsi.myknots][2].y;
		}else if (knot == 2){
		    fyother = vsi.knotlists[vsi.myknots][0].y;
		}
		if (fabs(fyother - fy) > 5.0 / (1 + vsi.canvas_height)){
		    mctl = ((vsi.knotlists[vsi.myknots][1].y - fy)
			    / (fyother-fy));
		}
	    }
	}else{
	    /* (event_is_up) */
	    set_func_command();
	    func_update();
	    report_state();	// Notify client of new settings
	    knot = -1;
	    return;
	}
	break;

      case LOC_DRAG:
	if (knot == -1){
	    return;
	}
	break;

      default:
	return;
    }

    // Apply constraints to adjustment of knots
    if (fy < 0){ // Always stay on canvas
	fy = 0;
    }else if (fy > 1){
	fy = 1;
    }
    if (vsi.myknots == CRV_KNOTS){ // Special contraints for these
	if (knot == 0 || knot == 2){
	    // Keep control knot between end points
	    /*fprintf(stderr,"knot=%d, mctl=%f, fyother=%f, fy=%f\n",
		    knot, mctl, fyother, fy);/*CMP*/
	    vsi.knotlists[vsi.myknots][1].y = fy + mctl * (fyother - fy);
	}else if (knot == 1){
	    fy = bound(fy,
		       vsi.knotlists[vsi.myknots][0].y,
		       vsi.knotlists[vsi.myknots][2].y);
	}
    }
    if (knot > 0 && knot < vsi.nknots[vsi.myknots] - 1){
	// Knots cannot move past neighbors in x direction
	if (fx <= vsi.knotlists[vsi.myknots][knot-1].x){
	    fx = (vsi.knotlists[vsi.myknots][knot-1].x
		  + 1.0 / (vsi.canvas_width - 1));
	}else if (fx >= vsi.knotlists[vsi.myknots][knot+1].x){
	    fx = (vsi.knotlists[vsi.myknots][knot+1].x
		  - 1.0 / (vsi.canvas_width - 1));
	}
	vsi.knotlists[vsi.myknots][knot].x = fx;
	vsi.knotlists[vsi.myknots][knot].y = fy;
    }else if (knot == 0){	// First knot stays at left edge
	vsi.knotlists[vsi.myknots][knot].x = 0;
	vsi.knotlists[vsi.myknots][knot].y = fy;
    }else if (knot == vsi.nknots[vsi.myknots] - 1){ // Last knot at right edge
	vsi.knotlists[vsi.myknots][knot].x = 1;
	vsi.knotlists[vsi.myknots][knot].y = fy;
    }
    func_update();
}

static void
mark(int x, int y, int color)
{
    g_draw_rect(vsi.gdev, x-3, y-3, 6, 6, color);
}

static void
report_state()
{
    if (vsi.notify_func && vsi.default_vsfunc){
	delete[] vsi.default_vsfunc->command;
	vsi.default_vsfunc->command = NULL;
	if (vsi.command){
	    vsi.default_vsfunc->command = vsi.command;
	    vsi.command = NULL;
	}
	(*vsi.notify_func)(vsi.default_vsfunc);
    }
}

static void
update_callback(Panel_item, int, Event *)
{
    settings_update();
    func_update();
    report_state();
}

static void
settings_update()
{
    static char *func = "";
    static float exponent = 0;
    static float power = 1;
    static float max = 0.03;
    static float min = 0;
    int i;
    char buf[20];
    char *string;
    float value;
    int ival;

    // Check for new functional form
    i = (int)xv_get(vsi.func_choice, PANEL_VALUE);
    string = (char *)xv_get(vsi.func_choice, PANEL_CHOICE_STRING, i);
    if (strcmp(string, func) != 0){
	if (func && *func){
	    free(func);
	}
	func = strdup(string);
	set_func_command();
    }

    // Check for new exponent in log function
    /*value = 0;
    string = (char *)xv_get(vsi.exponent, PANEL_VALUE);
    sscanf(string,"%f", &value);
    if (value < -100) value = -100;
    if (value > 100) value = 100;
    sprintf(buf,"%-.3g",value);
    if (value != exponent){
	exponent = value;
	xv_set(vsi.exponent, PANEL_VALUE, buf, NULL); // Set label
	// Set control knot position
	vsi.knotlists[LOG_KNOTS][1] = exp2knot(value);
	set_func_command();
	}*/

    // Check for new exponent in power function
    /*value = 0;
    string = (char *)xv_get(vsi.power, PANEL_VALUE);
    sscanf(string,"%f", &value);
    if (value < 0.01) value = 0.01;
    if (value > 100) value = 100;
    sprintf(buf,"%-.3g",value);
    if (value != power){
	power = value;
	xv_set(vsi.power, PANEL_VALUE, buf, NULL); // Set label
	vsi.knotlists[PWR_KNOTS][1] = pwr2knot(value);
	set_func_command();
	}*/

    ival = (int)xv_get(vsi.negative, PANEL_VALUE);
    if (ival != vsi.negative_val){
	vsi.negative_val = ival;
	build_background();
	set_negative_command();
    }

    value = 0;
    string = (char *)xv_get(vsi.min_data, PANEL_VALUE);
    sscanf(string,"%f", &value);
    vsi.min_data_val = value;

    value = 0;
    string = (char *)xv_get(vsi.max_data, PANEL_VALUE);
    sscanf(string,"%f", &value);
    vsi.max_data_val = value;
    if (vsi.min_data_val != min || vsi.max_data_val != max){
	min = vsi.min_data_val;
	max = vsi.max_data_val;
	set_domain_command();
    }
}

static void
func_update()
{
    char *label;
    int value;

    value = (int)xv_get(vsi.func_choice, PANEL_VALUE);
    label = (char *)xv_get(vsi.func_choice, PANEL_CHOICE_STRING, value);
    delete vsi.default_vsfunc;
    if (strcasecmp(label,"linear") == 0){
	vsi.default_vsfunc = linear_scale();
	vsi.myknots = LIN_KNOTS;
	xv_set(vsi.popup, FRAME_LEFT_FOOTER,
	       "LEFT moves/creates, MIDDLE deletes points", NULL);
    }else if (strcasecmp(label,"spline") == 0){
	vsi.default_vsfunc = spline_scale();
	vsi.myknots = SPL_KNOTS;
	xv_set(vsi.popup, FRAME_LEFT_FOOTER,
	       "LEFT moves/creates, MIDDLE deletes points", NULL);
    }else if (strcasecmp(label,"log") == 0){
	vsi.default_vsfunc = log_scale();
    }else if (strcasecmp(label,"power") == 0){
	vsi.default_vsfunc = power_scale();
    }else if (strcasecmp(label,"curve") == 0){
	vsi.default_vsfunc = curve_scale();
	vsi.myknots = CRV_KNOTS;
	xv_set(vsi.popup, FRAME_LEFT_FOOTER, "LEFT moves points", NULL);
    }
}

static void
set_ouflow(VsFunc *vsfunc)
{
    if (vsi.uflow_cmi < vsi.gs_start){
	vsfunc->uflow_cmi = vsi.uflow_cmi;
    }else{
	/*if (vsi.negative_val){
	    vsfunc->uflow_cmi = vsi.gs_start + vsi.gs_num - 1;
	}else{
	    vsfunc->uflow_cmi = vsi.gs_start;
	}*/
	vsfunc->uflow_cmi = vsfunc->lookup->table[0];
    }
    if (vsi.oflow_cmi < vsi.gs_start){
	vsfunc->oflow_cmi = vsi.oflow_cmi;
    }else{
	/*if (vsi.negative_val){
	    vsfunc->oflow_cmi = vsi.gs_start;
	}else{
	    vsfunc->oflow_cmi = vsi.gs_start + vsi.gs_num - 1;
	}*/
	vsfunc->oflow_cmi = vsfunc->lookup->table[vsfunc->lookup->size - 1];
    }
}

static VsFunc *
linear_scale()
{
    int nbins;
    int ncols;
    int i;
    int j;
    float value;
    float x;
    float y;
    int iy;

    /*fprintf(stderr,"linear_scale(): nknots=%d\n", vsi.nknots[myknots]);/*CMP*/
    nbins = 1024;
    ncols = vsi.gs_num;
    VsFunc *rtn = new VsFunc(nbins);
    rtn->min_data = vsi.min_data_val;
    rtn->max_data = vsi.max_data_val;
    rtn->negative = vsi.negative_val;

    j = 1;
    for (i=0; i<nbins; i++){
	x = (float)i / (nbins - 1);
	while (vsi.knotlists[vsi.myknots][j].x < x && j < vsi.nknots[vsi.myknots]-1){
	    j++;
	    /*fprintf(stderr,"j=%d\n", j);/*CMP*/
	}
	y = ((x - vsi.knotlists[vsi.myknots][j-1].x) * (vsi.knotlists[vsi.myknots][j].y - vsi.knotlists[vsi.myknots][j-1].y)
	     / (vsi.knotlists[vsi.myknots][j].x - vsi.knotlists[vsi.myknots][j-1].x) + vsi.knotlists[vsi.myknots][j-1].y);
	iy = (int)(y * (ncols - 1) + 0.5);
	if (vsi.negative_val){
	    rtn->lookup->table[i] = vsi.gs_start + ncols - 1 - iy;
	}else{
	    rtn->lookup->table[i] = vsi.gs_start + iy;
	}
    }
    set_ouflow(rtn);
    build_curve_data(rtn);
    canvas_repaint_proc();
    return rtn;
}

static VsFunc *
spline_scale()
{
    int i;
    int iy;
    int ncols = vsi.gs_num;
    int nbins = 1024;
    float *x = new float[vsi.nknots[vsi.myknots]];
    float *y = new float[vsi.nknots[vsi.myknots]];
    float *d2y = new float[vsi.nknots[vsi.myknots]];
    float *ycurve = new float[nbins];

    /*fprintf(stderr,"spline_scale(): nknots=%d\n", vsi.nknots[vsi.myknots]);/*CMP*/
    VsFunc *rtn = new VsFunc(nbins);
    rtn->min_data = vsi.min_data_val;
    rtn->max_data = vsi.max_data_val;
    rtn->negative = vsi.negative_val;

    for (i=0; i<vsi.nknots[vsi.myknots]; i++){
	x[i] = vsi.knotlists[vsi.myknots][i].x;
	y[i] = vsi.knotlists[vsi.myknots][i].y;
    }
    cspline(x, y, vsi.nknots[vsi.myknots], 1.0e30, 1.0e30, d2y);
    /*CMP*//*for (i=0; i<vsi.nknots[vsi.myknots]; i++){
	fprintf(stderr,"x=%.4f, y=%.4f, y''=%.4f\n", x[i], y[i], d2y[i]);
    }*/

    spline_points_interp(vsi.nknots[vsi.myknots], x, y, d2y, nbins, ycurve);

    for (i=0; i<nbins; i++){
	if (ycurve[i] >= 1){
	    iy = ncols - 1;
	}else if (ycurve[i] <= 0){
	    iy = 0;
	}else{
	    iy = (int)(ycurve[i] * (ncols - 1) + 0.5);
	}
	if (vsi.negative_val){
	    rtn->lookup->table[i] = vsi.gs_start + ncols - 1 - iy;
	}else{
	    rtn->lookup->table[i] = vsi.gs_start + iy;
	}
    }
    set_ouflow(rtn);
    build_curve_data(rtn);
    canvas_repaint_proc();
    delete[] x;
    delete[] y;
    delete[] d2y;
    delete[] ycurve;
    return rtn;
}

static VsFunc *
log_scale()
{
    char buf[20];
    int bound;
    float expo;
    int i;
    int j;
    int nbins;
    int ncols;
    float scale;
    float x;

    ncols = vsi.gs_num;
    nbins = 4 * ncols;
    VsFunc *rtn = new VsFunc(nbins);
    rtn->min_data = vsi.min_data_val;
    rtn->max_data = vsi.max_data_val;
    rtn->negative = vsi.negative_val;

    expo = knot2logparm(vsi.knotlists[vsi.myknots][1].x,
		     vsi.knotlists[vsi.myknots][1].y);
    sprintf(buf,"%-.3g", expo);
    xv_set(vsi.exponent, PANEL_VALUE, buf, NULL);

    if (expo < 0){
	scale = 1.0 / (1.0 - exp(expo));
    }else if (expo > 0){
	scale = 1.0 / expm1(expo);
    }else{
	return linear_scale();
    }

    x = bound = 0;
    for (i=j=0; i<ncols; i++){
	// Here is the function:
	if (expo < 0){
	    x = scale * (1.0 - exp((expo * (i + 1)) / ncols));
	}else{
	    x = scale * expm1((expo * (i + 1)) / ncols);
	}
	bound = (int)(nbins * x);
	if (bound >= nbins){
	    bound = nbins - 1;
	}
	if (vsi.negative_val){
	    for ( ; j<bound; j++){
		rtn->lookup->table[j] = vsi.gs_start + vsi.gs_num - 1 - i;
	    }
	}else{
	    for ( ; j<bound; j++){
		rtn->lookup->table[j] = i + vsi.gs_start;
	    }
	}
    }

    // Fill out the end of the curve with the last value
    if (vsi.negative_val){
	for ( ; j<nbins; j++){
	    rtn->lookup->table[j] = vsi.gs_start;
	}
    }else{
	for ( ; j<nbins; j++){
	    rtn->lookup->table[j] = ncols - 1 + vsi.gs_start;
	}
    }
    set_ouflow(rtn);

    build_curve_data(rtn);
    canvas_repaint_proc();
    return rtn;
}

static VsFunc *
power_scale()
{
    char buf[20];
    int bound;
    int h;
    int i;
    int j;
    int nbins;
    int ncols;
    char *string;
    float power;
    float value;
    int w;
    float x;

    h = vsi.canvas_height - 1;
    w = vsi.canvas_width - 1;

    power = knot2pwrparm(vsi.knotlists[vsi.myknots][1].x,
			 vsi.knotlists[vsi.myknots][1].y);

    sprintf(buf,"%-.3g", power);
    xv_set(vsi.power, PANEL_VALUE, buf, NULL);

    ncols = vsi.gs_num;
    if (power > 1){
	value = pow(ncols, power);
	if (value >= 1024){
	    nbins = 1024;
	}else{
	    nbins = (int)value;
	}
    }else if(power < 1){
	value = 5 * ncols / power;
	if (value >= 1024){
	    nbins = 1024;
	}else{
	    nbins = (int)value;
	}
    }else{
	nbins = ncols;
    }
    VsFunc *rtn = new VsFunc(nbins);
    string = (char *)xv_get(vsi.min_data, PANEL_VALUE);
    sscanf(string,"%f", &value);
    rtn->min_data = value;
    string = (char *)xv_get(vsi.max_data, PANEL_VALUE);
    sscanf(string,"%f", &value);
    rtn->max_data = value;
    rtn->negative = vsi.negative_val;

    x = bound = 0;
    for (i=j=0; i<ncols; i++){
	// Here is the function:
	x = pow((double)(i+1)/ncols, power);
	bound = (int)(nbins * x);
	if (bound >= nbins){
	    bound = nbins - 1;
	}
	if (vsi.negative_val){
	    for ( ; j<bound; j++){
		rtn->lookup->table[j] = vsi.gs_start + vsi.gs_num - 1 - i;
	    }
	}else{
	    for ( ; j<bound; j++){
		rtn->lookup->table[j] = i + vsi.gs_start;
	    }
	}
    }

    // Fill out the end of the curve with the last value
    if (vsi.negative_val){
	for ( ; j<nbins; j++){
	    rtn->lookup->table[j] = vsi.gs_start;
	}
    }else{
	for ( ; j<nbins; j++){
	    rtn->lookup->table[j] = ncols - 1 + vsi.gs_start;
	}
    }
    set_ouflow(rtn);

    build_curve_data(rtn);
    canvas_repaint_proc();

    return rtn;
}

//
// Beta correction function:
//	B(t) = t^(-ln(a)/ln(2))		0 <= a <= 1
// can be approximated by:
//	b(t) = t / ( (1/(a-2)) * (1-t) + 1)
// Ref: Christophe Schlick, Graphics Gems IV, pg. 401
// or
//	f(t) = t / (t - a * t + a)
// Ref: Christophe Schlick, Graphics Gems IV, pg. 422
//
static VsFunc *
curve_scale()
{
#define BMAX 10.0
#define AMAX 1000.0
#define NBINS 1024
    float a;
    float b;
    float c;
    int i;
    int iy;
    float x;
    float y;
    int icol;
    int fcol;
    int ncols = vsi.gs_num;
    int nbins = 1024;
    float d1;
    float d2;
    float d3;
    float tmp;
    int quadrant;

    VsFunc *rtn = new VsFunc(nbins);
    rtn->min_data = vsi.min_data_val;
    rtn->max_data = vsi.max_data_val;
    rtn->negative = vsi.negative_val;

    //
    // Note that the action depends on which "quadrant" the control
    // point is in.  "Quadrants" are divided like this (they are numbered
    // differently in the Image Browser manual):
    //
    // 	 +--------------------+
    //	 |\                 / |
    //	 |  \      4      /   |
    //	 |    \         /     |
    //	 |      \     /       |
    //	 |  1     \ /    3    |
    //	 |        / \         |
    //	 |      /     \       |
    //	 |    /         \     |
    //	 |  /      2      \   |
    //	 |/     	    \ |
    //	 +--------------------+
    //

    x = vsi.knotlists[vsi.myknots][1].x;
    y = vsi.knotlists[vsi.myknots][1].y;
    float y0 = vsi.knotlists[vsi.myknots][0].y;
    float y1 = vsi.knotlists[vsi.myknots][2].y;
    if (y0 == y1){
	y = 0;
    }else{
	y = (y - y0) / (y1 - y0);
    }
    if (vsi.negative_val){
	icol = vsi.gs_start + (ncols - 1) - (ncols - 1) * y0;
	fcol = vsi.gs_start + (ncols - 1) - (ncols - 1) * y1;
    }else{
	icol = vsi.gs_start + (ncols - 1) * y0;
	fcol = vsi.gs_start + (ncols - 1) * y1;
    }
    if (x + y <= 1){
	if (x <= y){
	    // Left quadrant
	    quadrant = 1;
	    tmp = x; x = y; y = tmp;
	}else{
	    // Bottom quadrant
	    quadrant = 2;
	}
    }else{
	if (x < y){
	    // Top quadrant
	    quadrant = 4;
	    x = 1 - x;
	    y = 1 - y;
	}else{
	    // Right quadrant
	    quadrant = 3;
	    tmp = x; x = 1 - y; y = 1 - tmp;
	}
    }
    if (x == 1 || y == 0){
	b = BMAX;
	a = AMAX;
    }else{
	d1 = x > 0.5 ? 1 - x : x;
	b = d1 / y;
	if (y < d1 * pow(2, 1.0 - BMAX)){
	    b = BMAX;
	}else{
	    b = log(2 * d1 / y) / log(2.0);
	}
	a = (x * pow(y, -1/b) - x) / (1 - x);
    }
    /*fprintf(stderr,"curve_scale(): x=%.3g, y=%.3g, a=%.4g, b=%.4g\n",
	    x, y, a, b);/*CMP*/
    for (i=0, x=0; i<nbins; i++, x += 1.0/NBINS){
	if (x == 0){
	    y = 0;
	}else if (x == 1){
	    y = 1;
	}else{
	    switch (quadrant){
	      case 1:
		y = a * pow(x, 1/b) / (1 + pow(x, 1/b) * (a - 1));
		break;
	      case 2:
		y = pow(x / (x - a * x + a), b);
		break;
	      case 3:
		y = 1 - a * pow(1-x, 1/b) / (1 + pow(1-x, 1/b) * (a - 1));
		break;
	      case 4:
		y = 1 - pow((1-x) / ((1-x) - a * (1-x) + a), b);
		break;
	    }
	}
	rtn->lookup->table[i] = (int)(icol + y * (fcol - icol) + 0.5);
    }
    set_ouflow(rtn);
    build_curve_data(rtn);
    canvas_repaint_proc();
    return rtn;
#undef BMAX
#undef AMAX
#undef NBINS
}

VsFunc *
get_default_vsfunc()
{
    return vsi.default_vsfunc;
}

void
set_vscale(float datamax)
{
    char buf[20];

    sprintf(buf,"%-.4g", datamax);
    xv_set(vsi.max_data, PANEL_VALUE, buf, NULL);
    settings_update();
    func_update();
}

void
set_vscale(float datamax, float datamin)
{
    char buf[20];

    sprintf(buf,"%-.4g", datamin);
    xv_set(vsi.min_data, PANEL_VALUE, buf, NULL);
    sprintf(buf,"%-.4g", datamax);
    xv_set(vsi.max_data, PANEL_VALUE, buf, NULL);
    settings_update();
    func_update();
}

static void
delete_knot(int nth)
{
    int i;
    Fpoint *newlist;

    /* Do not allow deletion of end points */
    if (nth > 0 && nth < (vsi.nknots[vsi.myknots]-1)){
	newlist = (Fpoint *)malloc((vsi.nknots[vsi.myknots]-1) * sizeof(Fpoint));
	if (!newlist){
	    perror("delete_knot(): Out of memory");
	    exit(1);
	}
	for (i=0; i<nth; i++){
	    newlist[i] = vsi.knotlists[vsi.myknots][i];
	}
	for (i=nth+1; i<vsi.nknots[vsi.myknots]; i++){
	    newlist[i-1] = vsi.knotlists[vsi.myknots][i];
	}
	free(vsi.knotlists[vsi.myknots]);
	vsi.knotlists[vsi.myknots] = newlist;
	vsi.nknots[vsi.myknots]--;
    }
}

static int
create_knot(float x, float y)
{
    int i;
    int addbefore = 0;
    Fpoint *newlist;

    if (x > 0 && x < 1 && y > 0 && y < 1){
	for (i=1; i<vsi.nknots[vsi.myknots]; i++){
	    if (x < vsi.knotlists[vsi.myknots][i].x){
		addbefore = i;
		break;
	    }
	}
	newlist = (Fpoint *)malloc((vsi.nknots[vsi.myknots]+1) * sizeof(Fpoint));
	if (!newlist){
	    perror("create_knot(): Out of memory");
	    exit(1);
	}
	for (i=0; i<addbefore; i++){
	    newlist[i] = vsi.knotlists[vsi.myknots][i];
	}
	newlist[i].x = x;
	newlist[i].y = y;
	for (i=addbefore; i<vsi.nknots[vsi.myknots]; i++){
	    newlist[i+1] = vsi.knotlists[vsi.myknots][i];
	}
	free(vsi.knotlists[vsi.myknots]);
	vsi.knotlists[vsi.myknots] = newlist;
	vsi.nknots[vsi.myknots]++;
    }
    return addbefore;
}

//
// Spline interpolation.
// Interpolates an array of points that just spans the data points.
// Note that the first and last interpolated points have the same x values
// as the first and last data points.
// The original data is assumed to have domain [0,1].
// Note the differences between this routine and csplint().
//
void
spline_points_interp(int n0,	// Number of points in the original data.
		     float *xa,	// The data abcissas.
		     float *ya,	// The data ordinates.
		     float *y2a, // Second derivative array from cspline.
		     int n,	 // Number of points to interpolate.
		     float *y )	 // Returns the interpolated points.
{
    int		klo, khi, k;
    float	a;
    float	b;
    float	h;
    float	x;

    klo = -1;
    khi = 0;
    for( k=0; k<n; k++ ){
	x = (float)k / (n - 1);
	while (xa[khi] <= x && khi < n0 - 1){
	    klo++;
	    khi++;
	    h = xa[khi] - xa[klo];
	    /*fprintf(stderr,"x=%f, klo=%d, y''=%f\n", x, klo, y2a[klo]);/*CMP*/
	}
	a = (xa[khi] - x) / h;
	b = (x - xa[klo]) / h;
	/*fprintf(stderr,"k=%d, a=%.2f, b=%.2f\n", klo, a, b);/*CMP*/
	y[k] = (a * ya[klo] + b * ya[khi]
		+ ( (a*a*a-a) * y2a[klo] + (b*b*b-b) * y2a[khi] ) * h * h
		/ 6.0);
    }
}

//
// Solve x = y^p  for p.
//
#define PMAX 100
#define PMIN 0.01
static float
knot2pwrparm(float x, float y)
{
    float p;

    if (x == y){
	p = 1;
    }else if (y == 0){
	p = 0.01;
    }else if (y >= 1){
	p = 100;
    }else{
	p = log(x) / log(y);
	if (p > 100){
	    p = 100;
	}else if (p < 0.01){
	    p = 0.01;
	}
    }
    return p;
}

//
// Solve x = (1 - exp(k*y)) / (1 - exp(k))  for k.
//
#define F(y, k) ((k)==0 ? (y) : ((1 - exp((k)*(y))) / (1 - exp(k))))
#define KMAX 100
#define KMIN -100
static float
knot2logparm(float x, float y)
{
    float k;
    float kold;
    float klo;
    float khi;

    if (x == y){
	k = 0;
    }else if (y == 0 || x == 1){
	k = KMIN;
    }else if (y == 1 || x == 0){
	k = KMAX;
    }else if (x >= F(y, KMIN)){
	k = KMIN;
    }else if (x <= F(y, KMAX)){
	k = KMAX;
    }else{
	khi = KMAX;
	klo = KMIN;
	for (k=0, kold=1; fabs(k - kold) > 0.005; ){
	    kold = k;
	    if (F(y, k) > x){
		k = (k + khi) / 2;
		klo = kold;
	    }else{
		k = (k + klo) / 2;
		khi = kold;
	    }
	}
    }
    return k;
}

static Fpoint
exp2knot(float curvature)
{
    Fpoint rtn;

    rtn.y = vsi.knotlists[LOG_KNOTS][1].y;
    if (curvature < 0){
	rtn.x = (1 - exp(curvature*rtn.y)) / (1 - exp(curvature));
    }else if (curvature > 0){
	rtn.x = expm1(curvature*rtn.y) / expm1(curvature);
    }else{
	rtn.x = 0.5;
    }
    return rtn;
}

static Fpoint
pwr2knot(float power)
{
    Fpoint rtn;

    rtn.y = vsi.knotlists[PWR_KNOTS][1].y;
    rtn.x = pow(rtn.y, power);
    return rtn;
}

static void
set_func_command()
{
    char *label;
    int value;
    char *cmd;
    char *cmd2;

    /*fprintf(stderr,"set_func_command()\n");/*CMP*/
    value = (int)xv_get(vsi.func_choice, PANEL_VALUE);
    label = (char *)xv_get(vsi.func_choice, PANEL_CHOICE_STRING, value);
    if (strcasecmp(label,"linear") == 0){
	vsi.myknots = LIN_KNOTS;
	//xv_set(vsi.exponent, PANEL_INACTIVE, TRUE, NULL);
	//xv_set(vsi.power, PANEL_INACTIVE, TRUE, NULL);
	cmd = knotlist2string("linear", vsi.nknots[vsi.myknots],
				     vsi.knotlists[vsi.myknots]);
    }else if (strcasecmp(label,"spline") == 0){
	vsi.myknots = SPL_KNOTS;
	//xv_set(vsi.exponent, PANEL_INACTIVE, TRUE, NULL);
	//xv_set(vsi.power, PANEL_INACTIVE, TRUE, NULL);
	cmd = knotlist2string("spline", vsi.nknots[vsi.myknots],
				     vsi.knotlists[vsi.myknots]);
    }else if (strcasecmp(label,"log") == 0){
	vsi.myknots = LOG_KNOTS;
	//xv_set(vsi.exponent, PANEL_INACTIVE, FALSE, NULL);
	//xv_set(vsi.power, PANEL_INACTIVE, TRUE, NULL);
	float expo = knot2logparm(vsi.knotlists[vsi.myknots][1].x,
				  vsi.knotlists[vsi.myknots][1].y);
	cmd = new char[32];
	sprintf(cmd,"'log', %-.3g", expo);
    }else if (strcasecmp(label,"power") == 0){
	vsi.myknots = PWR_KNOTS;
	//xv_set(vsi.exponent, PANEL_INACTIVE, TRUE, NULL);
	//xv_set(vsi.power, PANEL_INACTIVE, FALSE, NULL);
	float power = knot2pwrparm(vsi.knotlists[vsi.myknots][1].x,
				   vsi.knotlists[vsi.myknots][1].y);
	cmd = new char[32];
	sprintf(cmd,"'power', %-.3g", power);
    }else if (strcasecmp(label,"curve") == 0){
	vsi.myknots = CRV_KNOTS;
	//xv_set(vsi.exponent, PANEL_INACTIVE, TRUE, NULL);
	//xv_set(vsi.power, PANEL_INACTIVE, TRUE, NULL);
	cmd = knotlist2string("curve", vsi.nknots[vsi.myknots],
				     vsi.knotlists[vsi.myknots]);
    }

    if (vsi.command){
	cmd2 = new char[strlen(vsi.command) + strlen(cmd) + 3];
	sprintf(cmd2,"%s, %s", vsi.command, cmd);
    }else{
	cmd2 = new char[strlen(cmd) + 1];
	sprintf(cmd2,"%s", cmd);
    }
    delete[] vsi.command;
    delete[] cmd;
    vsi.command = cmd2;
}

static void
set_negative_command()
{
    char *cmd;
    /*fprintf(stderr,"set_negative_command()\n");/*CMP*/
    if (vsi.command){
	cmd = new char[strlen(vsi.command) + 32];
	sprintf(cmd,"%s, ", vsi.command);
    }else{
	cmd = new char[32];
	*cmd = '\0';
    }
    if (vsi.negative_val){
	strcat(cmd, "'negative'");
    }else{
	strcat(cmd, "'positive'");
    }

    delete[] vsi.command;
    vsi.command = cmd;
}

static void
set_domain_command()
{
    char *cmd;
    /*fprintf(stderr,"set_domain_command()\n");/*CMP*/
    if (vsi.command){
	cmd = new char[strlen(vsi.command) + 48];
	sprintf(cmd,"%s, 'domain', %-.4g, %-.4g",
		vsi.command, vsi.min_data_val, vsi.max_data_val);
    }else{
	cmd = new char[48];
	sprintf(cmd,"'domain', %-.4g, %-.4g", vsi.min_data_val, vsi.max_data_val);
    }
    delete[] vsi.command;
    vsi.command = cmd;
}

static char *
knotlist2string(char *label, int n, Fpoint *list)
{
    int buflen = strlen(label) + 13 * n + 10;
    char *rtn = new char[buflen]; // 13 chars per knot (+ room for flubs)
    char *pc = rtn;
    int i;

    sprintf(pc,"'%s'", label);
    pc += strlen(pc);
    for (i=0; i<n; i++){
	sprintf(pc,", %-.3f,%-.3f", list[i].x, list[i].y);
	pc += strlen(pc);
    }
    /*fprintf(stderr,"knotlist=<%s>\n", rtn);/*CMP*/
    return rtn;
}

static float
bound(float in, float bound1, float bound2)
{
    float min;
    float max;

    min = bound1 < bound2 ? bound1 : bound2;
    max = bound1 > bound2 ? bound1 : bound2;
    if (in < min){
	in = min;
    }else if (in > max){
	in = max;
    }
    return in;
}
