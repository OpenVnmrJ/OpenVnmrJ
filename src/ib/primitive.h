/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _PRIMITIVE_H
#define _PRIMITIVE_H
/************************************************************************
*									
*
*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*************************************************************************/

/* structure of points linked list */
typedef struct _prim_point_list
{
   short x;
   short y;
   struct _prim_point_list *next;
} Prim_point_list;

/************************************************************************
*                                                                       *
*  Create a linked list of points which forms a line.                   *
*  Its uses BRESENHAM's algorithm as described in                       *
*  "Fundamental of Interactive Computer Graphics" by J.D Foley and      *
*  A. Van Dam on page 435 (section 11.2), 1981.                         *
*                                                                       *
*  Return an address of linked list pointer.                            *
*                                                                       */
extern Prim_point_list *
prim_create_line(int x1, int y1,         /* Point 1 */
	         int x2, int y2);           /* Point 2 */
/************************************************************************
*                                                                       *
*  Destroy a linked list of point.                                      *
*                                                                       */
extern void
prim_destroy_line(Prim_point_list *pnt);

#endif _PRIMITIVE_H
