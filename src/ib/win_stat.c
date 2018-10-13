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

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Routine related to statistics.					*
*									*
*************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include "stderr.h"
//#include "process.h"
#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "roitool.h"
#include "common.h"
#include "convert.h"
#include "initstart.h"
#ifdef LINUX
// #include <strstream>
#else
// #include <strstream.h>
#endif
#include "graphics.h"
#include "math.h"
#include "ddllib.h"
#include "contrast.h"
#include "macroexec.h"
#include "win_stat.h"

Create_ListClass(Stats);

StatsLink::~StatsLink() {}

StatsLink& StatsLink::Print(){
    printf("object[%d]\n", item);
    return *this;
}

#define IRINT(x) ((x) >= 0 ? (int)((x)+0.5) : (-(int)(-(x)+0.5)))
#define DEFAULT_VERT_GAP  20
#define MIN_BINS 2
#define MAX_BINS 2048

extern RoitoolList *selected_ROIs;
extern void win_print_msg(char *, ...);

extern void win_print_msg(char *, ...);
static Win_stat *winstat=NULL;
static Stats tot_stat;

// Initialize static class members
Canvas Win_stat::can = 0;
Gdev *Win_stat::gdev = 0;
int Win_stat::data_color = 0;
int Win_stat::axis_color = 0;
int Win_stat::can_width = 0;
int Win_stat::can_height = 0;
int Win_stat::can_left_margin = 0;
int Win_stat::can_right_margin = 0;
int Win_stat::can_top_margin = 0;
int Win_stat::can_bottom_margin = 0;
double Win_stat::fboundary1 = 0;
double Win_stat::fboundary2 = 0;
int Win_stat::old_boundary1= 0;
int Win_stat::old_boundary2 = 0;
int Win_stat::boundary1_active = 0;
int Win_stat::boundary2_active = 0;
int Win_stat::boundary1_drawn = 0;
int Win_stat::boundary2_drawn = 0;
int Win_stat::boundary1_defined = 0;
int Win_stat::boundary2_defined = 0;
int Win_stat::histogram_showing = 0;
int Win_stat::updt_flag = 1;
char *Win_stat::xnames[] = {
    "gframe","roi","z","$",
    "area","volume","integrated","mean","median","min","max","sdv",0
};
char *Win_stat::ynames[] = {
    "area","volume","integrated","mean","median","min","max","sdv",0
};
Axis *Win_stat::axis = 0;

Panel_item Win_stat::pmin = 0;
Panel_item Win_stat::pmax = 0;
Panel_item Win_stat::pmedian = 0;
Panel_item Win_stat::parea = 0;
Panel_item Win_stat::pmean = 0;
Panel_item Win_stat::pstdv = 0;
Panel_item Win_stat::pname = 0;
Panel_item Win_stat::pvolume = 0;
Panel_item Win_stat::pautoupdate = 0;
double Win_stat::low_range = 0;
double Win_stat::high_range = 0;
int Win_stat::buckets = 0;
double Win_stat::absolute_min = 0;
double Win_stat::absolute_max = 0;
Panel_item Win_stat::dump_filename = 0;
Panel_item Win_stat::range_choice = 0;
Panel_item Win_stat::range_label = 0;
Panel_item Win_stat::show_low_bound = 0;
Panel_item Win_stat::show_high_bound = 0;
Panel_item Win_stat::show_low_range = 0;
Panel_item Win_stat::show_high_range = 0;
Panel_item Win_stat::show_buckets = 0;
Panel_item Win_stat::show_mask = 0;
Panel_item Win_stat::show_user_parm = 0;
Panel_item Win_stat::abscissa_choice = 0;
Panel_item Win_stat::ordinate_choice = 0;
Panel_item Win_stat::segment_image_button = 0;
Panel_item Win_stat::segment_roi_button = 0;
int Win_stat::boundary1 = 0;
int Win_stat::boundary2 = 0;
int Win_stat::boundary_aperture = 0;
StatsList *Win_stat::statlist = 0;

Gframe *win_stat_gframe_pointer ;   // Points to image operations are done on

/************************************************************************
*                                                                       *
*  Creator of window.							*
*									*/
Win_stat::Win_stat(void)
{
   Panel panel;		// panel
   int indent;		// Temp panel item indentation value
   int xitempos;	// current panel item position
   int yitempos;	// current panel item position
//   Panel_item item;	// Panel item
   int xpos, ypos;      // window position
   char initname[128];	// init file
   Siscms *siscms;
 

   (void)init_get_win_filename(initname);
   // Get the position of the control panel
   if (init_get_val(initname, "WINPRO_STATS", "dd", &xpos, &ypos) == NOT_OK){
      xpos = 400;
      ypos = 20;
   }

   frame = xv_create(NULL, FRAME,
		     WIN_DYNAMIC_VISUAL, TRUE,
		     NULL);

   int frame_width = 650;
   int frame_height = 400;
   popup = xv_create(frame, FRAME_CMD,
	XV_X,		xpos,
	XV_Y,		ypos,
        XV_WIDTH,       frame_width,
        XV_HEIGHT,      frame_height,
	FRAME_LABEL,	"Statistics",
	FRAME_DONE_PROC,	&Win_stat::done_proc,
	FRAME_SHOW_RESIZE_CORNER,	TRUE,
	FRAME_CMD_PUSHPIN_IN,	TRUE,
	NULL);
   
   panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);

   Win_stat::can_width = frame_width ;
   Win_stat::can_height = 170 ;

   Win_stat::can = xv_create(popup, CANVAS,
			     XV_X, 0,
			     XV_Y, frame_height - can_height,
			     //XV_WIDTH, WIN_EXTEND_TO_EDGE, //can_width,
			     //XV_HEIGHT, WIN_EXTEND_TO_EDGE, //can_height,
			     WIN_BORDER, FALSE,
			     OPENWIN_SHOW_BORDERS, FALSE,
			     WIN_DYNAMIC_VISUAL, TRUE,
			     CANVAS_FIXED_IMAGE, FALSE,
			     CANVAS_REPAINT_PROC, Win_stat::win_stat_show,
			     NULL);
 
   xitempos = 10;
   yitempos = 10;

   pname = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Name:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   yitempos += DEFAULT_VERT_GAP;
   pmin = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Min:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   yitempos += DEFAULT_VERT_GAP;
   pmax = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Max:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   yitempos += DEFAULT_VERT_GAP;
   pmedian = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Median:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   yitempos += DEFAULT_VERT_GAP;
   pmean = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Mean:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   yitempos += DEFAULT_VERT_GAP;
   pstdv = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Stdv:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   yitempos += DEFAULT_VERT_GAP;
   parea = xv_create(panel,		PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Area:",
		XV_X,			xitempos,
		XV_Y,			yitempos,
		NULL);

   yitempos += DEFAULT_VERT_GAP;
   pvolume = xv_create(panel, PANEL_MESSAGE,
		       PANEL_LABEL_STRING, "Volume:",
		       XV_X, xitempos,
		       XV_Y, yitempos,
		       NULL);

   yitempos += DEFAULT_VERT_GAP;

   pautoupdate =
   xv_create(panel, PANEL_CHOICE_STACK,
	     XV_X,              xitempos,
	     XV_Y,              yitempos,
	     PANEL_LABEL_STRING,  "Update:",
	     PANEL_CHOICE_STRINGS, "Now", "Manual",
	     	"Auto (On ROI Change)", "Auto (On ROI Drag)", NULL,
	     PANEL_VALUE, 2,
	     PANEL_NOTIFY_PROC, Win_stat::win_stat_updtflg,
	     NULL);

   yitempos += DEFAULT_VERT_GAP + 10;
   xv_create(panel, PANEL_BUTTON,
	     XV_X,              xitempos,
	     XV_Y,              yitempos,
	     PANEL_LABEL_STRING,  "Print Stats",
	     PANEL_NOTIFY_PROC, Win_stat::win_stat_print,
	     NULL);

   xv_create(panel, PANEL_BUTTON,
	     PANEL_LABEL_STRING,  "Dump Data",
	     PANEL_NOTIFY_PROC, Win_stat::win_data_dump,
	     NULL);

   dump_filename =
   xv_create(panel, PANEL_TEXT,
	     XV_Y,              yitempos,
	     PANEL_LABEL_STRING,  "File:",
	     PANEL_NOTIFY_LEVEL,  PANEL_ALL,
	     PANEL_VALUE, "",
	     PANEL_VALUE_DISPLAY_LENGTH, 30,
	     NULL);

   xitempos = 255;
   yitempos = 10 + DEFAULT_VERT_GAP;
   range_choice =
   xv_create(panel, PANEL_CHOICE_STACK,
	     XV_X, xitempos,
	     XV_Y, yitempos,
	     PANEL_LAYOUT, PANEL_HORIZONTAL,
	     PANEL_LABEL_STRING, "Statistics/Histogram limits:",
	     PANEL_NOTIFY_PROC, Win_stat::range_select,
	     PANEL_CHOICE_STRINGS, "Span ROI intensities",
				   "Span image intensities",
				   "Match segmentation limits",
				   "User specified:",
				   NULL,
	     PANEL_VALUE, 0,
	     NULL);

   yitempos += DEFAULT_VERT_GAP;
   xitempos += 25;
   show_low_range =
   xv_create(panel, PANEL_TEXT,
	     XV_X,              xitempos,
	     XV_Y,              yitempos,
	     PANEL_LABEL_STRING,  "Min:",
	     PANEL_NOTIFY_PROC, Win_stat::win_stat_calc,
	     PANEL_VALUE, "0.0",
	     PANEL_VALUE_DISPLAY_LENGTH, 10,
	     NULL);

   indent = (int)xv_get(show_low_range, XV_WIDTH) + 10;
   xitempos += indent;
   show_high_range =
   xv_create(panel, PANEL_TEXT,
	     XV_X,              xitempos,
	     XV_Y,              yitempos,
	     PANEL_LABEL_STRING,  "Max:",
	     PANEL_NOTIFY_PROC, Win_stat::win_stat_calc,
	     PANEL_VALUE, "0.0",
	     PANEL_VALUE_DISPLAY_LENGTH, 10,
	     NULL);
   xitempos -= indent;
   xitempos -= 25;

   yitempos += DEFAULT_VERT_GAP;
   show_buckets =
   xv_create(panel, PANEL_TEXT,
	     XV_X,              xitempos,
	     XV_Y,              yitempos,
	     PANEL_LABEL_STRING,  "Histogram bins:",
	     PANEL_NOTIFY_PROC, Win_stat::win_stat_calc,
	     PANEL_VALUE, "100",
	     PANEL_VALUE_DISPLAY_LENGTH, 5,
	     NULL);

   yitempos += DEFAULT_VERT_GAP;
   range_label =
   xv_create(panel,		PANEL_MESSAGE,
	     PANEL_LABEL_STRING,	"Image segmentation keep range:",
	     XV_X,			xitempos,
	     XV_Y,			yitempos,
	     NULL);

   yitempos += DEFAULT_VERT_GAP;
   xitempos += 25;
   show_low_bound =
   xv_create(panel, PANEL_TEXT,
	     XV_X,              xitempos,
	     XV_Y,              yitempos,
	     PANEL_LABEL_STRING,  "Min:",
	     PANEL_NOTIFY_PROC, Win_stat::bounds_panel_handler,
	     PANEL_VALUE, "-infinity",
	     PANEL_VALUE_DISPLAY_LENGTH, 10,
	     NULL);

   indent = (int)xv_get(show_low_bound, XV_WIDTH) + 10;
   xitempos += indent;
   show_high_bound =
   xv_create(panel, PANEL_TEXT,
	     XV_X,              xitempos,
	     XV_Y,              yitempos,
	     PANEL_LABEL_STRING,  "Max:",
	     PANEL_NOTIFY_PROC, Win_stat::bounds_panel_handler,
	     PANEL_VALUE, "+infinity",
	     PANEL_VALUE_DISPLAY_LENGTH, 10,
	     NULL);

   xitempos -= indent;
   yitempos += DEFAULT_VERT_GAP;
   segment_image_button =
   xv_create(panel, PANEL_BUTTON,
	     XV_X,              xitempos,
	     XV_Y,              yitempos,
	     PANEL_LABEL_STRING,  "Segment images",
	     PANEL_NOTIFY_PROC, Win_stat::apply,
	     NULL);

   indent = (int)xv_get(segment_image_button, XV_WIDTH) + 10;
   xitempos += indent;
   segment_roi_button =
   xv_create(panel, PANEL_BUTTON,
	     XV_X,              xitempos,
	     XV_Y,              yitempos,
	     PANEL_LABEL_STRING,  "Segment ROIs",
	     PANEL_NOTIFY_PROC, Win_stat::apply,
	     NULL);
   xitempos -= indent;


   //
   //  Buttons for histograms only
   //
   yitempos += DEFAULT_VERT_GAP;
   int opt_yitempos = yitempos;
   show_mask =
   xv_create(panel, PANEL_CHECK_BOX,
	     XV_X, xitempos,
	     XV_Y, yitempos,
	     PANEL_LAYOUT, PANEL_HORIZONTAL,
	     PANEL_LABEL_STRING, "Show segmented region:",
	     PANEL_VALUE, 1,
	     NULL);
   xitempos -= 25;


   //
   //  Buttons for scatterplots only
   //
   yitempos = opt_yitempos;
   ordinate_choice =
   xv_create(panel, PANEL_CHOICE_STACK,
	     XV_X, xitempos,
	     XV_Y, yitempos,
	     PANEL_LAYOUT, PANEL_HORIZONTAL,
	     PANEL_LABEL_STRING, "Y-coord:",
	     PANEL_NOTIFY_PROC, Win_stat::ordinate_callback,
	     PANEL_CHOICE_STRINGS, "Area",
				   "Volume",
				   "Integrated Intensity",
	     			   "Mean Intensity",
				   "Median Intensity",
				   "Minimum Intensity",
				   "Maximum Intensity",
				   "SDV of Intensity",
				   NULL,
	     PANEL_VALUE, 0,
	     PANEL_SHOW_ITEM, FALSE,
	     NULL);

   yitempos += DEFAULT_VERT_GAP;
   abscissa_choice =
   xv_create(panel, PANEL_CHOICE_STACK,
	     XV_X, xitempos,
	     XV_Y, yitempos,
	     PANEL_LAYOUT, PANEL_HORIZONTAL,
	     PANEL_LABEL_STRING, "X-coord:",
	     PANEL_NOTIFY_PROC, Win_stat::abscissa_callback,
	     PANEL_CHOICE_STRINGS, "Gframe number",
	     			   "ROI number",
				   "Slice Location",
				   "User Parameter:",
	     			   "Area",
				   "Volume",
				   "Integrated Intensity",
	     			   "Mean Intensity",
				   "Median Intensity",
				   "Minimum Intensity",
				   "Maximum Intensity",
				   "SDV of Intensity",
				   NULL,
	     PANEL_VALUE, 0,
	     PANEL_SHOW_ITEM, FALSE,
	     NULL);

   indent = (int)xv_get(abscissa_choice, XV_WIDTH);
   xitempos += indent;
   yitempos += 6;
   show_user_parm =
   xv_create(panel, PANEL_TEXT,
	     XV_X,              xitempos,
	     XV_Y,              yitempos,
	     PANEL_LABEL_STRING,  "",
	     PANEL_VALUE, "",
	     PANEL_VALUE_DISPLAY_LENGTH, 12,
	     PANEL_SHOW_ITEM, FALSE,
	     PANEL_NOTIFY_LEVEL,  PANEL_SPECIFIED,
	     PANEL_NOTIFY_STRING, "\n\r\t",
	     PANEL_NOTIFY_PROC, Win_stat::win_stat_show,
	     NULL);
   xitempos -= indent;
   yitempos -= 6;

   // Get initialized colormap file
   (void)init_get_cmp_filename(initname);
 
   // Create Siscms colormap structure and load the colormap from
   // initialized file.
   // Note that the order of colorname is important because that is the
   // order the colormap (red/gren/blue) will be loaded.
   if ((siscms = (Siscms *)siscms_create(initname, "mark-color",
       "gray-color", "false-color")) == NULL)
   {
      STDERR("Win_stat_info:siscms_create:cannot create siscms");
      exit(1);
   }
 
   // Register canvas event to handle mouse
   xv_set(canvas_paint_window(can),
          WIN_CONSUME_EVENTS,
          LOC_DRAG,
          WIN_MOUSE_BUTTONS, NULL,
          WIN_EVENT_PROC,    Win_stat::stat_canvas_handler,
	  NULL);


   // Create graphics device (bind it to this canvas).  This gdev will be
   // used in every graphics drawing into this canvas.
   // Assume that the SIS colormap segment has already been set by someone else.
   if ( (gdev = (Gdev *)g_device_attach(can, siscms)) == NULL)
   {
      STDERR("Win_stat:g_device_attach:cannot create graphics device");
      exit(1);
   }
 
   // Assign colors for graph and axes from mark-color selections (segment #1).
   data_color = G_Get_Stcms1(gdev) + 6;
   axis_color = G_Get_Stcms1(gdev) + 4;
 
   (void) init_get_win_filename(initname);


   buckets = 100;
   fboundary1 = fboundary2 = -1.0;
   boundary1_active = boundary2_active = FALSE;
   boundary1 = boundary2 = old_boundary1 = old_boundary2 = 0;
   boundary_aperture = 4 ;
   can_left_margin = 70;
   can_right_margin = 20;
   can_top_margin = 20;
   can_bottom_margin = 30 ;
   low_range = 0;
   high_range = 1;

   axis = new Axis(gdev);
   statlist = new StatsList;
}

/************************************************************************
*                                                                       *
*  Create the statistics window
*									*/
void
Win_stat::create()
{
    if (winstat == NULL){
	// Need to create the window
	winstat = new Win_stat;
    }
}

/************************************************************************
*                                                                       *
*  Draw histogram bar graph on canvas created above			*
*									*/
void
Win_stat::draw_graph(int *hist,		// Array of histogram values
		     int nbins,		// # of bins in histogram
		     double min,	// Intensity at bottom of first bin
		     double max)	// Intensity at top of last bin
{
    Gpoint *point, *point2 ;
    int i, j, k;
    char str[80];
    static int init = 1;
    int s0 = TRUE;		// Include bin with y=0 in vertical scaling
    
    if (winstat)
    {
	can_width = (int) xv_get(can, XV_WIDTH);
	can_height = (int) xv_get(can, XV_HEIGHT);
    }
    
    //clear the canvas
    g_clear_area_default(gdev, 0, 0, can_width, can_height);
    boundary1_drawn = boundary2_drawn = FALSE;
    histogram_showing = TRUE;
    
    // Set range of intensities over which data is binned.
    init = 0 ;
    sprintf (str, "%.4g", low_range);
    xv_set(show_low_range, PANEL_VALUE, str, NULL);
    sprintf (str, "%.4g", high_range);
    xv_set(show_high_range, PANEL_VALUE, str, NULL);
    
    // Check on the x-scaling.  Make sure we use an integral number of
    // pixels per bin, or an integral number of bins per pixel.
    // We create the new array "phist" to hold the possibly re-binned values.
    // The scaling factor "max_hist" is the max of the values in phist[].
    int sum;
    int *phist;
    int max_hist = 0;
    int n_pixels = can_width - can_left_margin - can_right_margin;
    int bin0 = nbins * (0 - min) / (max - min);
    if (nbins > n_pixels){
	// Data will be binned down from what's in *hist.
	int bins_per_pix = 1 + (nbins - 1) / n_pixels;
	n_pixels = 1 + (nbins -1) / bins_per_pix;
	phist = new int[n_pixels];
	for (i=0, j=0; i<nbins; j++){
	    for (k=0, sum=0; k<bins_per_pix && i<nbins; i++, k++){
		sum += hist[i];
	    }
	    phist[j] = sum;
	    if (sum > max_hist && (s0 || i-bins_per_pix > bin0 || i <= bin0)){
		max_hist = sum;
	    }
	}
    }else{		// nbins <= n_pixels
	// One or more pixels will get the datum for each histogram level.
	int pix_per_bin = (n_pixels - 1) / nbins;
	n_pixels = nbins * pix_per_bin;
	phist = new int[n_pixels];
	for (i=0, j=0; i<nbins; i++){
	    for (k=0; k<pix_per_bin; j++, k++){
		phist[j] = hist[i];
	    }
	    if (hist[i] > max_hist && (s0 || i != bin0)){
		max_hist = hist[i];
	    }
	}
    }
    if (!max_hist && hist[bin0]) {max_hist = hist[bin0];}

    // Don't use XOR'ing
    G_Set_Op(Win_stat::gdev, GXcopy);
    
    // Set up the axes
    axis->range(min, 0.0, max, max_hist);
    axis->location(can_left_margin, can_height - can_bottom_margin,
		   can_left_margin + n_pixels, can_top_margin);
    axis->color(axis_color);
    axis->number('x');
    axis->number('y');
    axis->plot();
    
    point = new Gpoint;
    point2 = new Gpoint;
    
    // Plot the data
    point->y = (short)axis->u_to_y(0.0);
    point->x = point2->x = can_left_margin;
    for (i = 0; i < n_pixels; i++)
    {
	if (phist[i]){
	    point2->y = (short)axis->u_to_y( phist[i] );
	    g_draw_line(gdev, point->x, point->y,
			point2->x, point2->y, data_color);
	}
	point->x = ++(point2->x);
    }
    
    delete[] phist;
    delete point ;
    delete point2 ;
    
    both_bounds_set();
}

/************************************************************************
*                                                                       *
*  Draw scatterplot graph of y vs. index number
*									*/
void
Win_stat::draw_scatterplot(double *x,		// Abscissas to graph
			   double *y,		// The values to graph
			   int nvalues)	// The number of values
{
    const short MARK_RADIUS = 4;
    int i;

    // Find the min and max of the data
    double xmin = *x;
    double xmax = *x;
    double ymin = *y;
    double ymax = *y;
    for (i=1; i<nvalues; i++){
	if (x[i] > xmax){
	    xmax = x[i];
	}else if (x[i] < xmin){
	    xmin = x[i];
	}
	if (y[i] > ymax){
	    ymax = y[i];
	}else if (y[i] < ymin){
	    ymin = y[i];
	}
    }

    // Get the current canvas size
    if (winstat){
	can_width = (int) xv_get(can, XV_WIDTH);
	can_height = (int) xv_get(can, XV_HEIGHT);
    }

    // Clear the canvas
    g_clear_area_default(gdev, 0, 0, can_width, can_height);
    histogram_showing = FALSE;

    // Don't use XOR'ing
    G_Set_Op(Win_stat::gdev, GXcopy);

    // Set up scale factors and plot the axes
    axis->range(xmin, ymin, xmax, ymax);
    axis->location(can_left_margin, can_height - can_bottom_margin,
		   can_width - can_right_margin, can_top_margin);
    axis->color(axis_color);
    axis->number('x');
    axis->number('y');
    axis->plot();

    // Mark a cross at each point
    short x0, y0, x1, y1;
    for (i=0; i<nvalues; i++){
	x0 = (short)axis->u_to_x(x[i]) - MARK_RADIUS;
	x1 = 2 * MARK_RADIUS + x0;
	y0 = y1 = (short)axis->u_to_y(y[i]);
	g_draw_line(gdev, x0, y0, x1, y1, data_color);

	x0 = x1 = (short)axis->u_to_x(x[i]);
	y0 = (short)axis->u_to_y(y[i]) - MARK_RADIUS;
	y1 = 2 * MARK_RADIUS + y0;
	g_draw_line(gdev, x0, y0, x1, y1, data_color);
    }
}

/************************************************************************
*                                                                       *
*  Destructor of window.						*
*									*/
Win_stat::~Win_stat(void)
{
    xv_destroy_safe(frame);
    delete axis;
    delete statlist;
}


/************************************************************************
*                                                                       *
*  Dismiss the popup window.						*
*  [STATIC]								*
*									*/
void
Win_stat::done_proc(Frame subframe)
{
  xv_set(subframe, XV_SHOW, FALSE, NULL);
  //cout << "delete winstat = " << winstat << endl;
  /*delete winstat;
    winstat = NULL;*/
  win_print_msg("Statistics: Exit");
}

/************************************************************************
*									*
*  Comparison function for qsort done in win_stat_calc().
*									*/
static int
cmp_z_posn(const void *stat1, const void *stat2)
{
    float z1 = (*(Stats **)stat1)->z_location;
    float z2 = (*(Stats **)stat2)->z_location;
    if (z1 > z2){
	return 1;
    }else if (z1 < z2){
	return -1;
    }else{
	return 0;
    }
}

/************************************************************************
*									*
*  Calculate statistics within selected ROIs.
*  If the "low_range" and "high_range" panel fields are set, only
*  pixels with intensities within the given range are considered
*  in the statistics.
*  [MACRO interface]
*  no arguments
*  [STATIC Function]							*
*									*/
int
Win_stat::Update(int argc, char **, int, char **)
{
    argc--;

    if (argc != 0){
	ABORT;
    }
    win_stat_calc();
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Update the statistics.
*  The "level" variable indicates who is calling:
*	1 - Action due to a mouse click
*	2 - Action due to a mouse drag
*  STATIC
*									*/
void
Win_stat::win_stat_update(int level)
{
    // Only do it if the stat window is up & auto updating enabled
    if (winstat){
	int updtflag = (int)xv_get(winstat->pautoupdate, PANEL_VALUE);
	if (level < updtflag && (int)xv_get(winstat->popup, XV_SHOW)){
	    Win_stat::win_stat_calc();
	}
    }
}

/************************************************************************
*									*
*  STATIC
*									*/
void
Win_stat::win_stat_updtflg()
{
    static int flag = 2;

    int updtflag = (int)xv_get(winstat->pautoupdate, PANEL_VALUE);
    if (flag < 2 && updtflag >= 2){
	Win_stat::win_stat_calc();
    }else if (updtflag == 0){
	xv_set(winstat->pautoupdate, PANEL_VALUE, flag, NULL);
	updtflag = flag;
	Win_stat::win_stat_calc();
    }
    flag = updtflag;
}

/************************************************************************
*									*
*  Calculate statistics within selected ROIs.
*  If the "low_range" and "high_range" panel fields are set, only
*  pixels with intensities within the given range are considered
*  in the statistics.
*
*  STATIC
*									*/
void
Win_stat::win_stat_calc()
{
    int i;
    double last_z_location = 0;
    char last_name[1025];
    Imginfo *iptr;	// image information pointer
    double min, max;	// minimum and maximum pixel values

    win_print_msg("Statistics: Processing ...");

    // Update Win_stat::buckets
    (void) bucket_set((Panel_item)0, 0, (Event *)0);

    // See what kind of histogram ranging we're using
    int range_type = (int)xv_get(range_choice, PANEL_VALUE);

    // Get rid of any leftover statistics
    Stats *stats;
    while (stats=statlist->Pop()){
	delete stats;
    }

    // Loop over all selected ROIs
    RoitoolIterator element(selected_ROIs);
    Roitool *roitool;
    int diff_names = FALSE;
    int diff_slices = FALSE;
    int nslices = 0;
    tot_stat.npixels = 0;
    *last_name = 0;
    while (roitool = ++element){
	win_stat_gframe_pointer = roitool->owner_frame ;
	iptr = roitool->owner_frame->imginfo;

	if (range_type == 0){
	    // Set min and max to range in ROI
	    roitool->get_minmax(&min, &max);
	}else if (range_type == 1){
	    // Set min and max to values for this image
	    iptr->get_minmax(&min, &max);
	}else if (range_type == 2){
	    // Set to segmentation limits, if we have them
	    // --otherwise ROI limits
	    both_bounds_set();
	    roitool->get_minmax(&min, &max);
	    if (boundary1_defined){
		min = fboundary1;
	    }
	    if (boundary2_defined){
		max = fboundary2;
	    }
	}else if (range_type == 3){
	    // Set min and max to user specfied values
	    low_range_set(show_low_range, 0, 0);
	    high_range_set(show_high_range, 0, 0);
	    min = Win_stat::low_range;
	    max = Win_stat::high_range;
	}else{
	    msgerr_print("win_stat_calc(): Internal error: invalid range_type");
	    return;
	}

	stats = new Stats;
	if (! roitool->histostats(buckets, min, max, stats) ){
	    delete stats;
	}else{
	    if (tot_stat.npixels == 0){
		// Init summary statistics first time through
		if (stats->npixels){
		    tot_stat.min = stats->min;
		    tot_stat.max = stats->max;
		    tot_stat.median = stats->median * stats->npixels;
		    tot_stat.mean = stats->mean * stats->npixels;
		    tot_stat.sdv = stats->sdv * stats->npixels;
		    tot_stat.area = stats->area;
		    tot_stat.volume = stats->volume;
		    tot_stat.npixels = stats->npixels;
		}
	    }else{
		// Accumulate summary statistics
		if (stats->npixels){
		    if (stats->max > tot_stat.max){
			tot_stat.max = stats->max;
		    }
		    if (stats->min < tot_stat.min){
			tot_stat.min = stats->min;
		    }
		    tot_stat.median += stats->median * stats->npixels;
		    tot_stat.mean += stats->mean * stats->npixels;
		    tot_stat.sdv += stats->sdv * stats->npixels;
		    tot_stat.area += stats->area;
		    tot_stat.volume += stats->volume;
		    tot_stat.npixels += stats->npixels;
		}
		if (stats->z_location != last_z_location){
		    diff_slices = TRUE;
		}
	    }
	    last_z_location = stats->z_location;

	    // Set the file name--note that it is clipped to fit into "fname"
	    if (iptr->GetFilename() == NULL){
		strcpy(stats->fname,"<no-name>");
	    }else{
		char fullname[1025];
		strcpy(fullname, iptr->GetDirpath());
		if (fullname[strlen(fullname) - 1] != '/'){
		    strcat(fullname, "/");
		}
		strcat(fullname, iptr->GetFilename());
		// Delete extra leading "/"
		char *cp = fullname;
		for ( ; *cp == '/' && *(cp+1) == '/'; cp++);
		strcpy(stats->fname, com_clip_len_front(cp, STATS_MAXSTR));
	    }
	    if (*last_name){
		if (strcmp(stats->fname, last_name)){
		    diff_names = TRUE;
		}
	    }
	    strcpy(last_name, stats->fname);

	    // Set the imginfo pointer
	    stats->imginfo = iptr;
	    statlist->Push(stats);
	    nslices++;
	}
    }
    if (tot_stat.npixels > 0){
	tot_stat.mean /= tot_stat.npixels;
    }

    // Now that we have the mean--recalculate the overall SDV
    {
	double dx;
	double sd;
	StatsIterator element(statlist);
	Stats **stats = new Stats*[nslices];
	for (i=0; i < nslices; i++){
	    stats[i] = ++element;
	}
	tot_stat.sdv = 0;
	for (i=0; i<nslices; i++){
	    dx = (stats[i]->mean - tot_stat.mean);
	    sd = stats[i]->sdv;
	    tot_stat.sdv += stats[i]->npixels * (sd * sd + dx * dx);
	}
	tot_stat.sdv = sqrt(tot_stat.sdv / tot_stat.npixels);
	delete [] stats;
    }

    if (diff_names){
	strcpy(tot_stat.fname, "");
    }else{
	strcpy(tot_stat.fname, last_name);
    }

    // Median cannot be calculated correctly from our summary stats.

    // If slices have diff z-positions, recalculate volume, based on
    // interpolation of area in between slabs.
    if (diff_slices){
	strcpy(tot_stat.vol_label, "3D-Volume");
	// Get a list of ROI stats sorted by z position
	StatsIterator element(statlist);
	Stats **stats = new Stats*[nslices];
	for (i=0; i < nslices; i++){
	    stats[i] = ++element;
	}
	qsort(stats, nslices, sizeof(Stats *), cmp_z_posn);

	// Now calculate the volume
	double prev_a = stats[0]->area;
	double prev_v = stats[0]->volume;
	double prev_z = stats[0]->z_location;
	double this_a;
	double this_v;
	double this_z;
	// Sum up all the volume in the first slab
	for (i=1; i<nslices && stats[i]->z_location == prev_z; i++){
	    prev_a += stats[i]->area;
	    prev_v += stats[i]->volume;
	}
	// "i" now points to slab with different z-position
	tot_stat.volume = prev_v;
	// Now look at the rest of the slabs
	while (i<nslices){
	    this_a = stats[i]->area;
	    this_v = stats[i]->volume;
	    this_z = stats[i]->z_location;
	    // Sum all the volumes in this slab
	    for (++i ; i<nslices && stats[i]->z_location == this_z; i++){
		this_a += stats[i]->area;
		this_v += stats[i]->volume;
	    }
	    // "i" now points to slab with different z-position
	    tot_stat.volume += ( (this_z - prev_z) * (this_a + prev_a) / 2
				- prev_v / 2 + this_v / 2);
	    prev_a = this_a;
	    prev_v = this_v;
	    prev_z = this_z;
	}
	delete [] stats;
    }else{
	strcpy(tot_stat.vol_label, "Volume");
    }

    // Now update the pop-up statistics window
    //winstat->show_window();
    // Need to repaint the window explicitly
    win_stat_show();
    
    win_print_msg("Statistics: Done.");
    macroexec->record("stat_update\n");
}

/************************************************************************
*									*
*  Calculate Statistics.						*
*  It calculates statistics within selected ROIs and displays the
*  results.
*
* STATIC
*									*/
void
Win_stat::win_statistics_show()
{
    xv_set(winstat->popup, XV_SHOW, TRUE, NULL);
    Win_stat::win_stat_calc();
}

/************************************************************************
*									*
*  Display only those buttons appropriate for the number of ROIs selected
*									*/
void
Win_stat::show_my_buttons(int nrois)
{
    //xv_set(range_choice, PANEL_SHOW_ITEM, TRUE, NULL);
    if ( (int)xv_get(range_choice, PANEL_VALUE) == 3 ){
	xv_set(show_low_range, PANEL_SHOW_ITEM, TRUE, NULL);
	xv_set(show_high_range, PANEL_SHOW_ITEM, TRUE, NULL);
    }else{
	xv_set(show_low_range, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(show_high_range, PANEL_SHOW_ITEM, FALSE, NULL);
    }
    //xv_set(show_buckets, PANEL_SHOW_ITEM, TRUE, NULL);
    
    if (nrois == 1){
	xv_set(ordinate_choice, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(abscissa_choice, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(show_user_parm, PANEL_SHOW_ITEM, FALSE, NULL);

	xv_set(show_mask, PANEL_SHOW_ITEM, TRUE, NULL);
    }else if (nrois > 1){
	xv_set(show_mask, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(ordinate_choice, PANEL_SHOW_ITEM, TRUE, NULL);
	xv_set(abscissa_choice, PANEL_SHOW_ITEM, TRUE, NULL);
	if ((int)xv_get(abscissa_choice, PANEL_VALUE) == 3){
	    xv_set(show_user_parm, PANEL_SHOW_ITEM, TRUE, NULL);
	}else{
	    xv_set(show_user_parm, PANEL_SHOW_ITEM, FALSE, NULL);
	}
    }
}

/************************************************************************
*									*
*  Set the abscissa variable for stat scatterplots
*  [STATIC Function]							*
*									*/
void
Win_stat::abscissa_callback(Panel_item, int value, Event *)
{
    win_stat_show();

    // Write out the appropriate macro command
    if (*xnames[value] != '$'){
	macroexec->record("stat_xcoord('%s')\n", xnames[value]);
    }else{
	// User variable
	char user_var[100];
	strncpy(user_var,
		(char *)xv_get(show_user_parm, PANEL_VALUE),
		sizeof(user_var));
	user_var[sizeof(user_var)-1] = 0;
	if ( strspn(user_var," \t") != strlen(user_var) ){
	    // The is non-whitespace in the "show_user_parm" field.
	    // Write out "$varname"
	    macroexec->record("stat_xcoord('$%s')\n", user_var);
	}
	delete [] user_var;
    }
}

/************************************************************************
*									*
*  Set the abscissa variable for stat scatterplots
*  [MACRO interface]
*  argv[0]: (char *) Variable name:
*	roi | z | area | volume | integrated | mean | median | min | max | sdv
*	OR a user variable name that appears in the data header
*  [STATIC Function]							*
*									*/
int
Win_stat::Xcoord(int argc, char **argv, int, char **)
{
    argc--; argv++;

    if (argc != 1){
	ABORT;
    }

    char *name = argv[0];
    int i;
    for (i=0; xnames[i]; i++){
	if (strcmp(name, xnames[i]) == 0){
	    break;
	}
    }
    if (!xnames[i]){
	// Name not found--assume it is a header parameter
	for (i=0; xnames[i]; i++){
	    if (strcmp(xnames[i], "$") == 0){
		break;
	    }
	}			// i set to "User parameter"
	if (*name == '$'){
	    name++;
	}
	xv_set(show_user_parm, PANEL_VALUE, name, NULL);
    }
    xv_set(abscissa_choice, PANEL_VALUE, i, NULL);
    abscissa_callback(0, i, 0);
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Set the ordinate variable for stat scatterplots
*  [STATIC Function]							*
*									*/
void
Win_stat::ordinate_callback(Panel_item, int value, Event *)
{
    win_stat_show();
    macroexec->record("stat_ycoord('%s')\n", ynames[value]);
}

/************************************************************************
*									*
*  Set the ordinate variable for stat scatterplots
*  [MACRO interface]
*  argv[0]: (char *) Variable name
*	area | volume | integrated | mean | median | min | max | sdv
*  [STATIC Function]							*
*									*/
int
Win_stat::Ycoord(int argc, char **argv, int, char **)
{
    argc--; argv++;

    if (argc != 1){
	ABORT;
    }

    char *name = argv[0];
    int i;
    for (i=0; ynames[i]; i++){
	if (strcmp(name, ynames[i]) == 0){
	    break;
	}
    }
    if (!ynames[i]){
	ABORT;
    }
    xv_set(ordinate_choice, PANEL_VALUE, i, NULL);
    ordinate_callback(0, i, 0);
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Display Statistics.
*									*/
void
Win_stat::win_stat_show()
{
    Stats *stats;
    int nstats = statlist->Count();

    // Show the right buttons
    show_my_buttons(nstats);

    if (nstats == 1){
	// If only one ROI, plot the histogram
	stats = statlist->Top();
	if (stats->npixels){
	    draw_graph(stats->histogram->counts,
		       stats->histogram->nbins,
		       stats->histogram->bottom,
		       stats->histogram->top);
	}else{
	    msgerr_print("No pixels within histogram intensity range");
	}
	win_stat_update_panel(stats, nstats);
    }else if (nstats > 1){
	// Find out what to plot
	int ordinate = (int)xv_get(ordinate_choice, PANEL_VALUE);
	int abscissa = (int)xv_get(abscissa_choice, PANEL_VALUE);

	// Gather data for all ROIs, and draw a graph
	char user_var[100];
	strncpy(user_var,
		(char *)xv_get(show_user_parm, PANEL_VALUE),
		sizeof(user_var));
	user_var[sizeof(user_var)-1] = 0;
	int data_ok = TRUE;
	double *xdata = new double[nstats];
	double *ydata = new double[nstats];
	double *stepwidth = new double[nstats];
	StatsIterator element(statlist);
	for (int i=0; stats = ++element; i++){
	    switch (abscissa){
	      case 0:	// Gframe #
		xdata[i] = stats->framenum;
		break;
	      case 1:	// ROI #
		xdata[i] = i;
		break;
	      case 2:	// z_location
		xdata[i] = stats->z_location;
		stepwidth[i] = stats->thickness;
		break;
	      case 3:	// User variable
		data_ok = stats->imginfo->st->GetValue(user_var, xdata[i]);
		break;
	      case 4:	// Area
		xdata[i] = stats->area;
		break;
	      case 5:	// Volume
		xdata[i] = stats->area * stats->thickness;
		break;
	      case 6:	// Integrated Intensity
		xdata[i] = stats->mean * stats->npixels;
		break;
	      case 7:	// Mean Intensity
		xdata[i] = stats->mean;
		break;
	      case 8:	// Median Intensity
		xdata[i] = stats->median;
		break;
	      case 9:	// Minimum Intensity
		xdata[i] = stats->min;
		break;
	      case 10:	// Maximum Intensity
		xdata[i] = stats->max;
		break;
	      case 11:	// Standard Deviation of Intensity
		xdata[i] = stats->sdv;
		break;
	    }
	    switch (ordinate){
	      case 0:	// Area
		ydata[i] = stats->area;
		break;
	      case 1:	// Volume
		ydata[i] = stats->area * stats->thickness;
		break;
	      case 2:	// Integrated Intensity
		ydata[i] = stats->mean * stats->npixels;
		break;
	      case 3:	// Mean Intensity
		ydata[i] = stats->mean;
		break;
	      case 4:	// Median Intensity
		ydata[i] = stats->median;
		break;
	      case 5:	// Minimum Intensity
		ydata[i] = stats->min;
		break;
	      case 6:	// Maximum Intensity
		ydata[i] = stats->max;
		break;
	      case 7:	// Standard Deviation of Intensity
		ydata[i] = stats->sdv;
		break;
	    }
	}

	if (data_ok){
	    // Plot it out
	    draw_scatterplot(xdata, ydata, nstats);
	    
	    // Print out the summary statistics
	    win_stat_update_panel(&tot_stat, nstats);
	} else if ( strspn(user_var," \t") != strlen(user_var) ){
	    // We didn't find "user_var", and it's not all white space.
	    char tbuf[1024];
	    sprintf(tbuf,"User parameter \"%s\" not found.", user_var);
	    msgerr_print(tbuf);
	}

	//XFlush(Gframe::gdev->xdpy);
	delete [] xdata;
	delete [] ydata;
	delete [] stepwidth;
    }
}

/************************************************************************
*									*
*  Print summary statistics in the panel area.
*									*/
void
Win_stat::win_stat_update_panel(Stats *stat, int nslices)
{
    char str[256];
    char *name = com_clip_len_front(stat->fname, 60);

    if (stat->npixels){
	sprintf(str,"Min: %.4g", stat->min);
	xv_set(pmin, PANEL_LABEL_STRING, str, NULL);
	sprintf(str,"Max: %.4g", stat->max);
	xv_set(pmax, PANEL_LABEL_STRING, str, NULL);
	sprintf(str,"Mean: %.4g", stat->mean);
	xv_set(pmean, PANEL_LABEL_STRING, str, NULL);
	sprintf(str,"Stdv: %.4g", stat->sdv);
	xv_set(pstdv, PANEL_LABEL_STRING, str, NULL);
    }else{
	xv_set(pmin, PANEL_LABEL_STRING, "", NULL);
	xv_set(pmax, PANEL_LABEL_STRING, "", NULL);
	xv_set(pmean, PANEL_LABEL_STRING, "", NULL);
	xv_set(pstdv, PANEL_LABEL_STRING, "", NULL);
    }
    sprintf(str,"Area: %.4g sq cm", stat->area);
    xv_set(parea, PANEL_LABEL_STRING, str, NULL);
    sprintf(str,"%s: %.4g cc", stat->vol_label, stat->volume);
    xv_set(pvolume, PANEL_LABEL_STRING, str, NULL);

    if (nslices == 1 && stat->npixels){
	sprintf(str,"Median: %.4g", stat->median);
	xv_set(pmedian, PANEL_LABEL_STRING, str, NULL);
    }else{
	xv_set(pmedian, PANEL_LABEL_STRING, "", NULL);
    }
    if (*name){
	sprintf(str,"Name: %s", name);
	xv_set(pname, PANEL_LABEL_STRING, str, NULL);
    }else{
	xv_set(pname, PANEL_LABEL_STRING, "", NULL);
    }
}
    
/************************************************************************
*									*
*  Print statistics.
*  [MACRO interface]
*  argv[0]: (int) file path
*  argv[1]: (int) file access mode ("a" or "w")
*  [STATIC Function]							*
*									*/
int
Win_stat::Print(int argc, char **argv, int, char **)
{
    argc--;
    argv++;

    switch (argc) {
      case 0:
	win_stat_print();
	break;
      case 1:
	win_stat_write(argv[0],"w");
	break;
      case 2:
	if (strlen(argv[1]) == 1 && (*argv[1] == 'w' || *argv[1] == 'a')) {
	    win_stat_write(argv[0], argv[1]);
	} else {
	    ABORT;
	}
	break;
      default:
	ABORT;
	break;
    }
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Dump ROI data.
*  [MACRO interface]
*  argv[0]: (int) file path
*  argv[1]: (int) file access mode ("a" or "w")
*  [STATIC Function]							*
*									*/
int
Win_stat::Dump(int argc, char **argv, int, char **)
{
    argc--;
    argv++;

    switch (argc) {
      case 0:
	win_data_dump();
	break;
      case 1:
	win_dump(argv[0],"a");
	break;
      case 2:
	if (strlen(argv[1]) == 1 && (*argv[1] == 'w' || *argv[1] == 'a')) {
	    win_dump(argv[0], argv[1]);
	} else {
	    ABORT;
	}
	break;
      default:
	ABORT;
	break;
    }
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Dump ROI data
*									*/
void
Win_stat::win_data_dump()
{
    FILE *fd = 0;
    char fname[MAXPATHLEN];

    // See if we have a dump file
    if (1==sscanf((char *)xv_get(dump_filename, PANEL_VALUE),"%1024s", fname)){
	//fprintf(stderr,"fname='%s'\n", fname);
	fd = fopen(fname, "a");
    }
    if (fd) {
	fclose(fd);
    } else {
	*fname = '\0';
    }
    win_dump(fname, "a");
}


/************************************************************************
*									*
*  Dump ROI data
*									*/
void
Win_stat::win_dump(char *fname, char *mode)
{
    float *pdat;
    FILE *fd = 0;
    int npix;

    fd = fopen(fname, mode);

    RoitoolIterator element(selected_ROIs);
    Roitool *roitool;

    if (!fd){
	// Output to info window--but make sure there is not too much.
	for (npix=0; roitool = ++element; ){
	    for (pdat=roitool->FirstPixel(); pdat; pdat=roitool->NextPixel()){
		npix++;
	    }
	}
	if (npix > 128){
	    msgerr_print("Too many values to dump in the info window (%d)",
			 npix);
	    msgerr_print(" Specify a File name and retry.");
	    return;
	}
    }

    // Loop over selected ROI's
    element.GotoFirst();
    while (roitool = ++element){
	fstat_print(fd, "ROI data:\n");
	for (pdat=roitool->FirstPixel(); pdat; pdat=roitool->NextPixel()){
	    fstat_print(fd, "%g\n", *pdat);
	}
	fstat_print(fd, "End ROI data\n");
    }
    if (fd){
	fclose(fd);
    }
    if (*fname=='\0' && *mode=='a') {
	macroexec->record("stat_dump\n");
    } else if (*mode == 'a') {
	macroexec->record("stat_dump('%s')\n", fname);
    } else {
	macroexec->record("stat_dump('%s','%s')\n", fname, mode);
    }
}

/************************************************************************
*									*
*  Write statistics to a file.
*									*/
void
Win_stat::win_stat_write(char *fname, char *mode)
{
    int i;
    char msg[MAXPATHLEN+80];
    FILE *fd = fopen(fname, mode);
    if (!fd) {
	sprintf(msg,"Cannot open file \"%s\" for writing.", fname);
	msgerr_print(msg);
	return;
    }
    if (statlist->Count() ){
	fprintf(fd," ROI Statistics:");
	Stats *stats = statlist->Top();
	int n = strlen(stats->fname) - 16 - 4;
	for (i=0; i<n; i++) {
	    fprintf(fd," ");
	}
	fprintf(fd,"Name\tPixels\tArea\tMin\tMax\tMedian\tMean\tStdv\n");
    }
    // Loop over all Stats in the list
    StatsIterator element(statlist);
    Stats *stats;
    while (stats = ++element){
	if (stats->npixels){
	    fprintf(fd,"%s\t%.0f\t%.4g\t%.4g\t%.4g\t%.4g\t%.4g\t%.4g\n",
		    stats->fname, stats->npixels, stats->area,
		    stats->min, stats->max, stats->median,
		    stats->mean, stats->sdv);
	}else{
	    fprintf(fd,"%s %.0f %.4g\n",
		    stats->fname, stats->npixels, stats->area);
	}
    }
    if (statlist->Count() > 1){
	fprintf(fd," Total: Area=%.4g  %s=%.4g",
		tot_stat.area, tot_stat.vol_label, tot_stat.volume);
	if (tot_stat.npixels){
	    fprintf(fd,", Sdv=%.4g\n", tot_stat.sdv);
	}else{
	    fprintf(fd,"\n");
	}
    }
    fclose(fd);
    macroexec->record("stat_print('%s','%s')\n", fname, mode);
}

/************************************************************************
*									*
*  Print statistics.
*									*/
void
Win_stat::win_stat_print()
{
    char name[128];	// Shortened file name

    // Print out header, if we're going to have any data to print
    if (statlist->Count() ){
	msginfo_print("ROI Statistics:            Name  Pixels      Area       Min       Max    Median      Mean      Stdv\n");
    }

    // Loop over all Stats in the list
    StatsIterator element(statlist);
    Stats *stats;
    while (stats = ++element){
	strcpy(name, com_clip_len_front(stats->fname, 28));
	if (stats->npixels){
	    msginfo_print("%31s %7.0f %9.4g %9.4g %9.4g %9.4g %9.4g %9.4g\n",
			  name, stats->npixels, stats->area,
			  stats->min, stats->max, stats->median,
			  stats->mean, stats->sdv);
	}else{
	    msginfo_print("%31s %7.0f %9.4g\n",
			  name, stats->npixels, stats->area);
	}
    }
    if (statlist->Count() > 1){
	msginfo_print("    Total: Area=%.4g  %s=%.4g",
		      tot_stat.area, tot_stat.vol_label, tot_stat.volume);
	if (tot_stat.npixels){
	    msginfo_print("  Sdv=%.4g\n", tot_stat.sdv);
	}else{
	    msginfo_print("\n");
	}
    }
    macroexec->record("stat_print\n");
}

/************************************************************************
*                                                                       *
*  low_range_set                                                        *
*                                                                       */
Panel_setting 
Win_stat::low_range_set(Panel_item item, int, Event *)
{
  if ( sscanf((char *)xv_get(item, PANEL_VALUE), "%lf", &low_range) < 1 ){
      low_range = 0.0 ;
  }
  return (PANEL_INSERT) ;
}

/************************************************************************
*                                                                       *
*  high_range_set                                                        *
*                                                                       */
Panel_setting 
Win_stat::high_range_set(Panel_item item, int, Event *)
{
  if ( sscanf((char *)xv_get(item, PANEL_VALUE), "%lf", &high_range) < 1 ){
      high_range = 0.0 ;
  }
  return (PANEL_INSERT) ;
}

/************************************************************************
*                                                                       *
*  bounds_panel_handler
*                                                                       */
Panel_setting
Win_stat::bounds_panel_handler(Panel_item, int, Event *)
{
    int range_type = (int)xv_get(range_choice, PANEL_VALUE);
    if (range_type == 2){
	win_stat_calc();
    }else{
	both_bounds_set();
    }
    return (PANEL_INSERT) ;
}

/************************************************************************
*                                                                       *
*  both_bounds_set
*                                                                       */
void
Win_stat::both_bounds_set()
{
    char range[1024];
    
    strncpy(range, (char *)xv_get(show_low_bound, PANEL_VALUE), sizeof(range));

    if ( strncmp(range, "-inf", 4) == 0 ){
	boundary1_defined = 0;
    }else if ( sscanf(range, "%lf", &fboundary1) == 1 ){
	boundary1_defined = 1;
    }else{
	boundary1_defined = 0;
    }

    strncpy(range, (char *)xv_get(show_high_bound, PANEL_VALUE), sizeof(range));

    if ( strncmp(range, "+inf", 4) == 0 || strncmp(range, "inf", 3) == 0 ){
	boundary2_defined = 0;
    }else if ( sscanf (range, "%lf", &fboundary2) == 1 ){
	boundary2_defined = 1;
    }else{
	boundary2_defined = 0;
    }

    redraw_boundaries_from_user_coords();
    
    return;
}

/************************************************************************
*									*
*  Set the number of bins in a histogram
*  [MACRO interface]
*  argv[0]: (int) Number of bins
*  [STATIC Function]							*
*									*/
int
Win_stat::Bins(int argc, char **argv, int, char **)
{
    argc--; argv++;

    if (argc != 1){
	ABORT;
    }
    xv_set(show_buckets, PANEL_VALUE, argv[0], NULL);
    bucket_set(0, 0, 0);
    return PROC_COMPLETE;
}

/************************************************************************
 *                                                                       *
 *  bucket_set                                                        *
 *                                                                       */
Panel_setting 
Win_stat::bucket_set(Panel_item, int, Event *)
{
    char sbuckets[10];
    double dbins;
    int tbuckets;

    if (sscanf((char *)xv_get(show_buckets, PANEL_VALUE),"%lf", &dbins) != 1
	|| dbins < MIN_BINS)
    {
	tbuckets = MIN_BINS;
    }else if (dbins > MAX_BINS){
	tbuckets = MAX_BINS;
    }else{
	tbuckets = IRINT(dbins);
    }
    if (tbuckets != buckets || (double)tbuckets != dbins){
	buckets = tbuckets;
	sprintf(sbuckets,"%d", buckets);
	xv_set(show_buckets, PANEL_VALUE, sbuckets, NULL);
	macroexec->record("stat_bins(%d)\n", buckets);
    }
    return (PANEL_INSERT);
}

/************************************************************************
*                                                                       *
*  Handle changes in histogram range type
*                                                                       */
void
Win_stat::range_select(Panel_item, int value, Event *)
{
    if (value == 3){
	xv_set(show_low_range, PANEL_SHOW_ITEM, TRUE, NULL);
	xv_set(show_high_range, PANEL_SHOW_ITEM, TRUE, NULL);
    }else{
	xv_set(show_low_range, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(show_high_range, PANEL_SHOW_ITEM, FALSE, NULL);
    }
    win_stat_calc();
}

/************************************************************************
*                                                                       *
*  Apply boundaries to image                                            *
*                                                                       */
void
Win_stat::apply(Panel_item item, Event *)
{
    int i, size ;                   // counter and size of data
    float *fdata;                   // pointer to floating point data
    Gframe *gptr ;                   
    Roitool *r = NULL;
    
    // Read the boundary values from the panel items
    both_bounds_set();
    
    if (item == segment_roi_button){
	// Do for all images with selected ROIs
	if (! Roitool::get_selected_tool() ){
	    msgerr_print("No ROIs are selected.");
	    return;
	}
	for (gptr=Gframe::get_first_frame();
	     gptr;
	     gptr=Gframe::get_next_frame(gptr))
	{
	    if (Roitool::frame_has_a_selected_tool(gptr)){
		// (If it has an ROI, we know it also has an image)
		
		// Do the ROI segmentation
		Roitool::segment_selected_rois(gptr,
					       boundary1_defined, fboundary1,
					       boundary2_defined, fboundary2);
		
		// Image has changed--free old pixmap.
		XFreePixmap(gptr->imginfo->display, gptr->imginfo->pixmap);
		gptr->imginfo->pixmap = 0;
		Frame_data::display_data(gptr,
					 gptr->imginfo->datastx,
					 gptr->imginfo->datasty,
					 gptr->imginfo->datawd,
					 gptr->imginfo->dataht,
					 gptr->imginfo->vs,
					 FALSE);
	    }
	}
    }else if (item == segment_image_button){
	// Do for all selected frames
	for (gptr = Frame_select::get_first_selected_frame();
	     gptr;
	     gptr = Frame_select::get_next_selected_frame(gptr))
	{
	    if (gptr->imginfo){
		// Zero out all intensities outside the boundaries
		fdata = (float *)gptr->imginfo->GetData(); 
		size = gptr->imginfo->GetFast() * gptr->imginfo->GetMedium();
		
		if (boundary1_defined && boundary2_defined){
		    for (i=0 ; i< size; i++){
			if ((*fdata < fboundary1) || (*fdata > fboundary2)){
			    *fdata = 0;
			}
			fdata++;
		    }
		}else if (boundary1_defined){
		    for (i=0 ; i< size; i++){
			if (*fdata < fboundary1){
			    *fdata = 0;
			}
			fdata++;
		    }
		}else if (boundary2_defined){
		    for (i=0 ; i< size; i++){
			if (*fdata > fboundary2){
			    *fdata = 0;
			}
			fdata++;
		    }
		}else{
		    msgerr_print("No limits specified for segmentation.");
		    return;
		}
		
		// Image has changed--free old pixmap.
		XFreePixmap(gptr->imginfo->display, gptr->imginfo->pixmap);
		gptr->imginfo->pixmap = 0;
		Frame_data::display_data(gptr,
					 gptr->imginfo->datastx,
					 gptr->imginfo->datasty,
					 gptr->imginfo->datawd,
					 gptr->imginfo->dataht,
					 gptr->imginfo->vs,
					 FALSE);
	    }
	}
    }
}

/************************************************************************
*                                                                       *
*  Canvas repainter.
*                                                                       */
void
Win_stat::canvas_repaint()
{
    win_stat_show();
}



/************************************************************************
*                                                                       *
*  Canvas event handler.                                                *
*                                                                       */
void
Win_stat::stat_canvas_handler(Xv_window, Event *event)
        // Note that it actually takes 4 arguments, but the last
        // two are not used (Notify_arg arg, Notify_event_type type)
        // Note that Xv_window is also not used.
{
    short x = event_x(event);

    // Ignore mouse events when we're showing a scatterplot
    int range_type = (int)xv_get(range_choice, PANEL_VALUE);
    if (histogram_showing){
	switch (event_action(event))
	{
	  case ACTION_SELECT: // == LEFT_BUTTON
	    if (event_is_down(event)){ 
		boundary_set(x);
	    }else if (event_is_up(event)){
		if (range_type == 2) {win_stat_calc();}
		boundary1_active = boundary2_active = FALSE;
		contrast_mask_off();
	    }
	    break ;
	    
	  case LOC_DRAG:       // Moving with either button down
	    boundary_move(x);
	    break ;
	    
	  case ACTION_ADJUST: // == MIDDLE_BUTTON
	    if (!event_is_down(event)){
		boundary_remove(x);
		if (range_type == 2) {win_stat_calc();}
	    }
	    break ;
	    
	  default:
	    break ;
	}
    }
}


/************************************************************************
*                                                                       *
*  Sets boundaries                                                      *
*                                                                       */
void
Win_stat::boundary_set(short where)
{
    if ( boundary1_defined && abs(where - boundary1) < boundary_aperture ){
	// Work on boundary1
	boundary1_active = TRUE;
    }else if (boundary2_defined && abs(where - boundary2) < boundary_aperture){
	// Change boundary2
	boundary2_active = TRUE;
    }else if ( !boundary1_defined ){
	if ( !boundary2_defined || where <= boundary2 ){
	    // Create boundary1
	    boundary1 = where;
	    boundary1_defined = 1;
	    boundary1_active = TRUE;
	}else{	// ( boundary2_defined && where > boundary2 )
	    // Swap boundaries
	    boundary1 = boundary2;
	    boundary1_defined = 1;
	    boundary2 = where;
	    boundary2_active = TRUE;
	}
    }else if ( !boundary2_defined ){
	if ( where >= boundary1 ){
	    // Create boundary2
	    boundary2 = where;
	    boundary2_defined = 1;
	    boundary2_active = TRUE;
	}else{
	    // Swap boundaries
	    boundary2 = boundary1;
	    boundary2_defined = 1;
	    boundary1 = where;
	    boundary1_active = TRUE;
	}
    }
    redraw_boundaries();
}


/************************************************************************
*                                                                       *
*  Removes boundaries                                                   *
*                                                                       */
void
Win_stat::boundary_remove(short where)
{
    if (boundary1_defined && abs(where - boundary1) < boundary_aperture){
//	if (boundary2_defined){
//	    boundary1 = boundary2;
//	    boundary2_defined = FALSE;
//	}else{
	    boundary1_defined = 0;
//	}
    }else if (boundary2_defined && abs(where - boundary2) < boundary_aperture){
	//remove second boundary
	boundary2_defined = 0;
    }

    redraw_boundaries();
}


/************************************************************************
*                                                                       *
*  Moves boundaries                                                     *
*                                                                       */
void
Win_stat::boundary_move(short where)
{
    if (boundary1_active){
	if (where > can_left_margin) 
	    boundary1 = where;
	else 
	    boundary1 = can_left_margin;
    }else if (boundary2_active){
	if (where > can_left_margin) 
	    boundary2 = where;
	else 
	    boundary2 = can_left_margin;
    }

    redraw_boundaries();
}


/************************************************************************
*                                                                       *
*  redraws boundaries                                                   *
*                                                                       */
void
Win_stat::redraw_boundaries(void)
{
  Gpoint *p1, *p2 ;
  char str[1024];

  G_Set_Op(Win_stat::gdev, GXxor);
  p1 = new Gpoint;
  p2 = new Gpoint;

  // Erase old boundaries, if they have been drawn
  if (boundary1_drawn)
  {
    p1->x = p2->x = old_boundary1 ;
    p1->y = 1 ;
    p2->y = can_height ;
    g_draw_line (gdev, p1->x, p1->y, p2->x, p2->y, axis_color);
    boundary1_drawn = FALSE;
  } 

  if (boundary2_drawn)
  {
    p1->x = p2->x = old_boundary2 ;
    p1->y = 1 ;
    p2->y = can_height ;
    g_draw_line (gdev, p1->x, p1->y, p2->x, p2->y, axis_color);
    boundary2_drawn = FALSE;
  } 

  // Draw new boundaries
  if (boundary1_defined)
  {
    old_boundary1 = boundary1 ;
    p1->x = p2->x = boundary1 ;
    p1->y = 1 ;
    p2->y = can_height ;
    g_draw_line (gdev, p1->x, p1->y, p2->x, p2->y, axis_color);
    boundary1_drawn = TRUE;
  } 

  // Don't draw second boundary if it's on top of the first
  if ( boundary2_defined && !(boundary1_drawn && (boundary1 == boundary2)) )
  {
    old_boundary2 = boundary2 ;
    p1->x = p2->x = boundary2 ;
    p1->y = 1 ;
    p2->y = can_height ;
    g_draw_line (gdev, p1->x, p1->y, p2->x, p2->y, axis_color);
    boundary2_drawn = TRUE;
  } 

  // Swap boundaries if appropriate
  if (boundary1_defined && boundary2_defined && boundary1 > boundary2){
      int i = boundary2;
      boundary2 = boundary1;
      boundary1 = i;
  }

  // Update values displayed on panel
  // Don't update if value on panel already matches cursor position
  if (boundary1_defined){
      if ( !( ( sscanf((char *)xv_get(show_low_bound, PANEL_VALUE),
		       "%lf", &fboundary1) == 1 )
	     && (axis->u_to_x(fboundary1) == boundary1)) )
      {
	  fboundary1 = axis->x_to_u(boundary1);
	  sprintf(str,"%.4g", fboundary1);
	  xv_set(show_low_bound, PANEL_VALUE, str, NULL);
      }
  }else{
      xv_set(show_low_bound, PANEL_VALUE, "-infinity", NULL);
  }

  if (boundary2_defined){
      if ( !( ( sscanf((char *)xv_get(show_high_bound, PANEL_VALUE),
		     "%lf", &fboundary2) == 1 )
	   && (axis->u_to_x(fboundary2) == boundary2)) )
      {
	  fboundary2 = axis->x_to_u(boundary2);
	  sprintf(str,"%.4g", fboundary2);
	  xv_set(show_high_bound, PANEL_VALUE, str, NULL);
      }
  }else{
      xv_set(show_high_bound, PANEL_VALUE, "+infinity", NULL);
  }

  // Show segmentation mask if it is enabled and mouse button is down
  if ( ((int)xv_get(show_mask, PANEL_VALUE) & 1)
      && (boundary1_active || boundary2_active) ){
      mask_on(fboundary1, fboundary2);
  }

  delete p1 ; 
  delete p2 ; 
}

/************************************************************************
*                                                                       *
*  Redraws boundaries, assuming user coords (fboundary1, 2) are correct.
*  Updates canvas coords (boundary1, 2).
*                                                                       */
void
Win_stat::redraw_boundaries_from_user_coords(void)
{
    Gpoint *p1, *p2 ;
    char str[1024];
    
    if ( histogram_showing ){
	G_Set_Op(Win_stat::gdev, GXxor);
	p1 = new Gpoint;
	p2 = new Gpoint;
	
	if (boundary1_drawn)
	{
	    p1->x = p2->x = old_boundary1 ;
	    p1->y = 1 ;
	    p2->y = can_height ;
	    g_draw_line (gdev, p1->x, p1->y, p2->x, p2->y, axis_color);
	    boundary1_drawn = FALSE;
	} 
	
	if (boundary2_drawn)
	{
	    p1->x = p2->x = old_boundary2 ;
	    p1->y = 1 ;
	    p2->y = can_height ;
	    g_draw_line (gdev, p1->x, p1->y, p2->x, p2->y, axis_color);
	    boundary2_drawn = FALSE;
	} 
	
	if (boundary1_defined)
	{
	    boundary1 = axis->u_to_x(fboundary1);
	    old_boundary1 = boundary1 ;
	    p1->x = p2->x = boundary1 ;
	    p1->y = 1 ;
	    p2->y = can_height ;
	    g_draw_line (gdev, p1->x, p1->y, p2->x, p2->y, axis_color);
	    boundary1_drawn = TRUE;
	} 
	
	if (boundary2_defined)
	{
	    boundary2 = axis->u_to_x(fboundary2);
	    old_boundary2 = boundary2 ;
	    p1->x = p2->x = boundary2 ;
	    p1->y = 1 ;
	    p2->y = can_height ;
	    g_draw_line (gdev, p1->x, p1->y, p2->x, p2->y, axis_color);
	    boundary2_drawn = TRUE;
	}

	// Swap boundaries if necessary
	if (boundary1_defined && boundary2_defined && fboundary1 > fboundary2){
	    double t = fboundary1;
	    fboundary1 = fboundary2;
	    fboundary2 = t;
	    int i = boundary1;
	    boundary1 = boundary2;
	    boundary2 = i;
	}

	// Update values displayed on panel
	if (boundary1_defined){
	    sprintf(str,"%.4g", fboundary1);
	    xv_set(show_low_bound, PANEL_VALUE, str, NULL);
	}else{
	    xv_set(show_low_bound, PANEL_VALUE, "-infinity", NULL);
	}
	
	if (boundary2_defined){
	    sprintf(str,"%.4g", fboundary2);
	    xv_set(show_high_bound, PANEL_VALUE, str, NULL);
	}else{
	    xv_set(show_high_bound, PANEL_VALUE, "+infinity", NULL);
	}
	
	delete p1 ; 
	delete p2 ;
    }
}


/************************************************************************
*                                                                       *
*  draws boundaries
*  Called when a new display is put up.
*                                                                       */
void
Win_stat::draw_boundaries(void)
{
  // Restore previous boundaries if they are within our new range.
  // If they aren't, forget them.
  if (boundary1_defined){
      boundary1 = axis->u_to_x(fboundary1);
      if (boundary1<can_left_margin || boundary1>can_width-can_right_margin){
	  boundary1_defined = FALSE;
      }
  }
  if (boundary2_defined){
      boundary2 = axis->u_to_x(fboundary2);
      if (boundary2<can_left_margin || boundary2>can_width-can_right_margin){
	  boundary2_defined = FALSE;
      }
  }

//  if (boundary2_defined && ! boundary1_defined){
//      boundary1 = boundary2;
//      fboundary1 = fboundary2;
//      boundary1_defined = TRUE;
//      boundary2_defined = FALSE;
//  }

  G_Set_Op(gdev, GXxor);
  Gpoint *p1 = new Gpoint;
  Gpoint *p2 = new Gpoint;

  if (boundary1_defined)
  {
    old_boundary1 = boundary1 ;
    p1->x = p2->x = boundary1 ;
    p1->y = 1 ;
    p2->y = can_height ;
    g_draw_line (gdev, p1->x, p1->y, p2->x, p2->y, axis_color);
    boundary1_drawn = TRUE;
  }else{
    boundary1_drawn = FALSE;
  }

  if (boundary2_defined)
  {
    old_boundary2 = boundary2 ;
    p1->x = p2->x = boundary2 ;
    p1->y = 1 ;
    p2->y = can_height ;
    g_draw_line (gdev, p1->x, p1->y, p2->x, p2->y, axis_color);
    boundary2_drawn = TRUE;
  }else{
    boundary2_drawn = FALSE;
  }

  delete p1 ; 
  delete p2 ; 
}

/************************************************************************
*                                                                       *
*  Masks portion of image outside region marked by boundaries.
*                                                                       */
void
Win_stat::mask_on(double min, double max)
{
    if (!histogram_showing){
	return;
    }

    Roitool *tool;
    if (! (tool = Roitool::get_selected_tool()) ){
	return;
    }

    VsFunc *vf = tool->owner_frame->imginfo->vsfunc;
    if (!vf || !vf->lookup){
	return;
    }

    int negflag = vf->negative ? -1 : 1;

    float scale = vf->lookup->size / (vf->max_data - vf->min_data);
    int mn = (int)( (min - vf->min_data) * scale);
    if (mn >= vf->lookup->size){
	mn = vf->lookup->table[vf->lookup->size - 1] + negflag;
    }else if (min < vf->min_data){
	mn = vf->lookup->table[0] - negflag;
    }else{
	mn = vf->lookup->table[mn];
    }
    int minflag = negflag * boundary1_defined;

    int mx = (int)( (max - vf->min_data) * scale);
    if (mx >= vf->lookup->size){
	mx = vf->lookup->table[vf->lookup->size - 1] + negflag;
    }else if (max < vf->min_data){
	mx = vf->lookup->table[0] - negflag;
    }else{
	mx = vf->lookup->table[mx];
    }
    int maxflag = negflag * boundary2_defined;

    contrast_mask_on(minflag, mn, maxflag, mx);
}

void
Win_stat::fstat_print(FILE *fd, char *format, ...)
{
    char msgbuf[1024];	/* message buffer */
    va_list vargs;	/* variable argument pointer */

    va_start(vargs, format);
    (void)vsprintf(msgbuf, format, vargs);
    va_end(vargs);

    // See if we have a dump file
    if (fd) {
	fprintf(fd,"%s", msgbuf);
    }else{
	msginfo_print("%s", msgbuf);
    }
}
