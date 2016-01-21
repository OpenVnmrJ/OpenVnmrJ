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

#ifndef _WIN_STAT_H
#define _WIN_STAT_H

#include "histogram.h"
#include "axis.h"

class Win_stat{
  private:
    Frame frame;	// Parent
    Frame popup;	// Popup frame (subframe)
    static void done_proc(Frame);
    static void abscissa_callback(Panel_item, int, Event *);
    static void ordinate_callback(Panel_item, int, Event *);
    static void fstat_print(FILE *, char *format, ...);
    static Canvas can ;
    static Gdev *gdev ;
    static int data_color;
    static int axis_color;
    static int can_width;
    static int can_height;
    static int can_left_margin;
    static int can_right_margin;
    static int can_top_margin;
    static int can_bottom_margin;
    static double fboundary1, fboundary2;
    static int old_boundary1, old_boundary2;
    static int boundary1_active, boundary2_active;
    static int boundary1_drawn, boundary2_drawn;
    static int boundary1_defined, boundary2_defined;
    static int histogram_showing;
    static int updt_flag;
    static char *xnames[];
    static char *ynames[];
    static Axis *axis;
    
  public:
    Win_stat(void);
    ~Win_stat(void);
    void show_window() { xv_set(popup, XV_SHOW, TRUE, NULL); }
    static Panel_item pmin;
    static Panel_item pmax;
    static Panel_item pmedian;
    static Panel_item parea;
    static Panel_item pmean;
    static Panel_item pstdv;
    static Panel_item pname;
    static Panel_item pvolume;
    static Panel_item pautoupdate;
    static void create();
    static void draw_graph(int *hist, int nbins, double min, double max);
    static void draw_scatterplot(double *x, double *y, int nvalues);
    static double low_range, high_range;
    static int buckets;
    static double absolute_min, absolute_max;
    static Panel_item dump_filename;
    static Panel_item range_choice;
    static Panel_item range_label;
    static Panel_item show_low_bound;
    static Panel_item show_high_bound;
    static Panel_item show_low_range;
    static Panel_item show_high_range;
    static Panel_item show_buckets;
    static Panel_item show_mask;
    static Panel_item show_user_parm;
    static Panel_item abscissa_choice;
    static Panel_item ordinate_choice;
    static Panel_item segment_image_button;
    static Panel_item segment_roi_button;
    static Panel_setting low_range_set(Panel_item, int, Event*);
    static Panel_setting high_range_set(Panel_item, int, Event*);
    static Panel_setting bounds_panel_handler(Panel_item, int, Event*);
    static void both_bounds_set();
    static Panel_setting bucket_set(Panel_item, int, Event*);
    static void apply_to_change(Panel_item, int, Event*);
    static void data_change(Panel_item, int, Event*);
    static void range_select(Panel_item, int value, Event*);
    static void apply(Panel_item, Event*);
    static void select(Panel_item, Event*);
    static void stat_canvas_handler(Xv_window win, Event *event);
    static void canvas_repaint();
    static int boundary1, boundary2, boundary_aperture ;
    static void boundary_set(short);
    static void boundary_move(short);
    static void boundary_remove(short);
    static void draw_boundaries(void);
    static void redraw_boundaries(void);
    static void redraw_boundaries_from_user_coords(void);
    static void show_my_buttons(int nrois);
    static void mask_on(double min, double max);

    static StatsList *statlist;
    static void win_statistics_show();
    static void win_stat_calc();
    static void win_stat_updtflg();
    static void win_stat_update(int level);
    static int Bins(int argc, char **argv, int retc, char **retv);
    static int Dump(int argc, char **argv, int retc, char **retv);
    static int Print(int argc, char **argv, int retc, char **retv);
    static int Update(int argc, char **argv, int retc, char **retv);
    static int Xcoord(int argc, char **argv, int retc, char **retv);
    static int Ycoord(int argc, char **argv, int retc, char **retv);
    static void win_stat_show();
    static void win_stat_print();
    static void win_stat_write(char *fname, char *mode);
    static void win_data_dump();
    static void win_dump(char *fname, char *mode);
    static void win_stat_update_panel(Stats *stat, int nslices);
};
#endif /* _WIN_STAT_H */
