/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef lint
   static char *Sid = "Copyright (c) Varian Assoc., Inc.  All Rights Reserved.";
#endif (not) lint

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
*  Routines releated to draw graphics Line.				*
*									*
*************************************************************************/
#include <stdlib.h>
#include <malloc.h>
#include "stderr.h"
#include "primitive.h"

#ifdef __OBJECTCENTER__
#define abs(x) ((x)<0 ? -(x) : (x));
#endif

/************************************************************************
*                                                                       *
*  Create a linked list of points which forms a line.			*
*  Its uses BRESENHAM's algorithm as described in             		*
*  "Fundamental of Interactive Computer Graphics" by J.D Foley and      *
*  A. Van Dam on page 435 (section 11.2), 1981.                         *
*									*
*  Return an address of linked list pointer.				*
*									*/
Prim_point_list *
prim_create_line(int px1, int py1,		/* Point 1 */
	         int px2, int py2)		/* Point 2 */
{
   Prim_point_list *gline=NULL;	/* linked list of points */
   register int dx, dy;		/* distances of x and y */
   register int d;		/* decision value */
   register int incr1, incr2;	/* increment values depending of value 'd' */
   register int x, y;		/* current calculated x and y point */
   register int endx, endy;	/* ending point */
   register int up_down;	/* value of 1 or -1 */
   register Prim_point_list *curptr;	/* current point */

#define	INSERT_LINE_PNT(ptr, x, y) \
   if ((ptr->next = (Prim_point_list *)malloc(sizeof(Prim_point_list))) \
      == NULL) \
   { \
      PERROR("g_create_line:cannot malloc:"); \
      prim_destroy_line(gline); \
      return(NULL); \
   } \
   curptr = curptr->next; curptr->x = x; curptr->y = y; curptr->next = NULL


   if ((px1 < 0) || (py1 < 0) || (px2 < 0) || (py2 < 0))
   {
      STDERR("prim_create_line: Value of points cannot be less than 0");
      WARNING_OFF(Sid);
      return(NULL);
   }

   dx = abs(px2 - px1) ;
   dy = abs(py2 - py1) ;

   if (dx > dy)
   {
      d = (dy << 1) - dx ;
      incr1 = dy << 1 ;
      incr2 = (dy - dx) << 1 ;
      if (px2 > px1)
      {
         x = px1;     y = py1;
         endx = px2;  endy = py2 ;
         up_down = (py1 > py2) ? -1 : 1 ;
      }
      else
      {   
         x = px2;     y = py2;
         endx = px1;  endy = py1;
         up_down = (py1 > py2) ? 1 : -1 ;
      }
      
      /* Store the first point */
      if ((gline = (Prim_point_list *)malloc(sizeof(Prim_point_list)))
	 == NULL)
      {
	 PERROR("g_create_line:cannot malloc:");
	 return(NULL);
      }
      gline->x = x; gline->y = y; gline->next = NULL;
      curptr = gline;
   
      while (x++ < endx)
      {
         if (d < 0)
            d += incr1 ;
         else
         {
            y += up_down;
            d += incr2 ;
         }

	 INSERT_LINE_PNT(curptr, x, y);
      }
 
      /* Force it to end at endpoint */
      if (endy != curptr->y)
      {
	 INSERT_LINE_PNT(curptr, x, y);
      }
   }
   else
   {
      d = (dx << 1) - dy ;
      incr1 = dx << 1 ;
      incr2 = (dx - dy) << 1 ;
      if (py2 > py1)
      {
         x = px1 ;
         y = py1 ;
         endx = px2 ;
         endy = py2 ;
         up_down = (px1 > px2) ? -1 : 1 ;
      }
      else
      {
         x = px2 ;
         y = py2 ;
         endx = px1 ;
         endy = py1 ;
         up_down = (px1 > px2) ? 1 : -1 ;
      } 
      
      /* Store the first point */
      if ((gline = (Prim_point_list *)malloc(sizeof(Prim_point_list)))
	 == NULL)
      {
	 PERROR("g_create_line:cannot malloc:");
	 return(NULL);
      }
      gline->x = x; gline->y = y; gline->next = NULL;
      curptr = gline;

      while (y++ < endy)
      {
         if (d < 0)
            d += incr1 ;
         else
         {
            x += up_down ;
            d += incr2 ;
         }

	 INSERT_LINE_PNT(curptr, x, y);
      }
 
      /* Force it to end at endpoint */
      if (endx != curptr->x)
      {
	 INSERT_LINE_PNT(curptr, x, y);
      }
   }
   return(gline);

#undef INSERT_LINE_PNT
}

/************************************************************************
*                                                                       *
*  Destroy a linked list of point.					*
*									*/
void
prim_destroy_line(Prim_point_list *pnt)
{
   register Prim_point_list *ptr;

   while (ptr = pnt)
   {
      pnt = pnt->next;
      (void)free((char *)ptr);
   }
}
