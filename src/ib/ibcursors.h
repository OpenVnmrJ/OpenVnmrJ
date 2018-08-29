/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef _IBCURSORS_H
#define _IBCURSORS_H
/************************************************************************
*									
*  @(#)ibcursors.h 18.1 03/21/08 (c)1993 SISCO
*
*************************************************************************/

#include <X11/cursorfont.h>

#define IBCURS_DEFAULT			XC_center_ptr
#define IBCURS_BOTTOM_LEFT_CORNER	XC_bottom_left_corner
#define IBCURS_BOTTOM_RIGHT_CORNER	XC_bottom_right_corner
#define IBCURS_TOP_LEFT_CORNER		XC_top_left_corner
#define IBCURS_TOP_RIGHT_CORNER		XC_top_right_corner
#define IBCURS_SELECT_POINT		XC_draft_small
#define IBCURS_SELECT_OBJECT		XC_draft_large
#define IBCURS_MOVE_OBJECT		XC_fleur
#define IBCURS_DRAW			XC_pencil
#define IBCURS_ZOOM			XC_tcross
#define IBCURS_BUSY			XC_watch
#define IBCURS_TEXT			XC_xterm
#define IBCURS_VSCALE			XC_circle
#define IBCURS_FRAME			XC_draped_box
#define IBCURS_MATH			XC_iron_cross

#define CSICURS_VOXEL_SELECT		XC_dotbox
#define CSICURS_SPECTRUM_TOOL		XC_sb_v_double_arrow
#define CSICURS_FILTER			XC_box_spiral
#define CSICURS_VERT_MARKER		XC_left_side
#define CSICURS_XHAIR			XC_plus
#define CSICURS_PEAK_PICK		XC_target
#define CSICURS_GRID_MARK		XC_cross

extern int set_cursor_shape(int curs);
extern void warp_pointer(int x, int y);

#endif /* _IBCURSORS_H */
