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

#ifndef _WIN_ROTATION_H
#define _WIN_ROTATION_H

typedef enum{
    ROT_90 = 0,
    ROT_180,
    ROT_270,
    ROT_FLIP_E_W,
    ROT_FLIP_N_S,
    ROT_FLIP_NE_SW,
    ROT_FLIP_NW_SE
} Rottype;

typedef struct{
    char *file;
    char *cmd;
    Rottype rottype;
} RotTbl;

// Class used to create rotation controller
class Win_rot
{
  private:
    static RotTbl rot_tbl[];

    Frame frame;		// Parent
    Frame popup;		// Popup frame (subframe)

    static void done_proc(Frame);
    static void execute(Panel_item);
    static void rotate(Rottype);
    Server_image get_rot_image(Rottype);

  public:
    Win_rot(void);
    ~Win_rot(void);
    void show_window() { xv_set(popup, XV_SHOW, TRUE, NULL); }
    static int Rotate(int argc, char **argv, int retc, char **retv);
};

#endif /* _WIN_ROTATION_H */
