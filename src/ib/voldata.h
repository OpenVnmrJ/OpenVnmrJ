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

#ifndef VOLDATA_H
#define VOLDATA_H

class SliceSel{
  public:
    Panel_item first;
    Panel_item inc;
    Panel_item last;
};

class VolData{
  public:
    VolData();
    ~VolData();
    void set_data(Imginfo *, int newItem=TRUE);
    Gframe *get_gframe() { return gframe; }
    Imginfo *get_imginfo() { return vimage; }
    void extract_plane(int orient, int nslices, int *slicelist);
    static void extract_slices(Imginfo *);
    static int Extract(int argc, char **argv, int, char **);
    static int Mip(int argc, char **argv, int, char **);

  private:
    Frame frame;
    Frame popup;
    Panel_item orient;
    SliceSel front;
    SliceSel top;
    SliceSel side;
    Panel_item extract_button;
    Panel_item mip_button;
    Panel_item disp_button;
    Panel_item del_button;
    Panel_item fchoice;
    Panel_item pfname;
    Menu file_menu;
    Imginfo *vimage;
    Gframe *gframe;
    int nfast;
    int nmedium;
    int nslow;
    static int front_plane;
    static int top_plane;
    static int side_plane;
    void show_window() { xv_set(popup, XV_SHOW, TRUE, NULL); }
    void extract_plane_header(Imginfo *, int orient,
			      int nslices, int *slicelist);
    static void execute(Panel_item, Event *);
    static void enframe(Panel_item, Event *);
    static void unload(Panel_item, Event *);
    static void fmenu_proc(Menu, Menu_item);
    static Panel_setting slice_callback(Panel_item, int, Event *);
    void extract_planes(int orient, int first, int last, int incr);
    void extract_mip(int orient, int first, int last, int incr);
    int clip_slice(int orient, int slice);
    static int num_text_value(Panel_item);
};

#endif /* VOLDATA_H */


