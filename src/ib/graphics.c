/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef lint
   static char *Sid = "@(#)graphics.c 18.1 03/21/08 20:01:08 (c)1991 SISCO";
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
*  Routine related to graphics drawing.					*
*									*
*  NOTES:								*
*  - Since the index 0 of colormap will map it to some X value 		*
*    (typically in the range of 19 - 23), the actual pixel value	*
*    (background) on the canvas screen (graphics-area) will not		*
*    be zero.  Hence the XOR operation for graphics drawing should be	*
*    XOR with the background value in order to obtain the actual	*
*    pixel value on the screen.						*
*									*
*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "stderr.h"
#include "graphics.h"

static Xv_font *xvfont_small = NULL;
static Xv_font *xvfont_medium = NULL;
static Xv_font *xvfont_large = NULL;
static Xv_font *xvfont_extralarge = NULL;

static int n_color_chips;
static Pixmap *chip_pixmaps;
static Server_image *chip_images;

/************************************************************************
*									*
* This routine initializes everything related to the graphics.  This    *
* includes graphics context and colormap.                               * 
* IMPORTANT: when creating canvas, you should set WIN_DYNAMIC_VISUAL	*
*  to TRUE in order for the colormap to work correctly.			*
* Return the handler of graphics device or NULL.                        *
*									*/
Gdev *
g_device_create(Canvas canvas, Siscms *siscms)
{
   Gdev *gdev;
   int  map_class;

   WARNING_OFF(Sid);

   if ((gdev = (Gdev *)calloc(1, sizeof(Gdev))) == NULL)
   {
      STDERR("cannot create graphics device");
      return(NULL);
   }

   gdev->xdpy = (Display *)xv_get(canvas, XV_DISPLAY);
   gdev->xid = (XID)xv_get(canvas_paint_window(canvas), XV_XID);
   gdev->canvas = canvas;
   map_class = (int) xv_get(canvas, XV_VISUAL_CLASS);

   /* Check if we need to set the colormap for canvas */
   if (siscms)
   {
      int i;

      /* User defined colormap */
      gdev->siscms = siscms;

      /* Xview colormap */
      if (map_class != PseudoColor)
         gdev->xvcms.type = XV_STATIC_CMS;
      else
         gdev->xvcms.type = XV_DYNAMIC_CMS;
      gdev->xvcms.size = siscms->cms_size;
      gdev->xvcms.rgb_count = siscms->cms_size;
      gdev->xvcms.index = 0;
      gdev->xvcms.red = siscms->r;
      gdev->xvcms.green = siscms->g;
      gdev->xvcms.blue = siscms->b;
      
      /* Load the colormap into canvas */
      G_Update_Colormap(gdev);

      /* X pixel values */
      gdev->xcolor = (u_long *)xv_get(gdev->canvas, WIN_X_COLOR_INDICES);

      /* Inverse X pixel values */
      for (i=0; i<siscms->cms_size; i++)
	 gdev->inv_xcolor[(u_char)(gdev->xcolor[i])] = i;
   }
   /*
   XSetWindowBackground(gdev->xdpy, gdev->xid, gdev->xcolor[0]);
   */
   gdev->xgcval.background = gdev->siscms->size_cms1 ? gdev->xcolor[0] :
	 WhitePixel(gdev->xdpy, DefaultScreen(gdev->xdpy));
   gdev->xgcval.foreground = gdev->siscms->size_cms1 ? 
			     gdev->xcolor[gdev->siscms->size_cms1 - 1] :
	 BlackPixel(gdev->xdpy, DefaultScreen(gdev->xdpy));
   gdev->xgcval.function = GXxor;
   gdev->xgc = XCreateGC(gdev->xdpy, gdev->xid,
      GCFunction | GCForeground | GCBackground, &(gdev->xgcval));

   /* Get the font */
   gdev->font = (Xv_font)xv_find(gdev->canvas, FONT,
	FONT_FAMILY,    FONT_FAMILY_LUCIDA,
	FONT_STYLE,	FONT_STYLE_BOLD,
	NULL);

   return(gdev);
}


/************************************************************************
*									*
* This routine initializes everything related to the graphics but does
* not write the colormap out to Xwindows.  This assumes that the
* colormap segment "SIS_CMS_NAME" has already been initialized, and we
* just set up to use it.  We still need to set a lot of colormap
* parameters, so the gdev knows what colors to use to draw things. But
* the actual colors may be different if the default values have been
* changed by someone else.  They better not change the SIZE of the
* colormap or the basic functions of the entries, or we're in trouble.
* IMPORTANT: when creating canvas, you should set WIN_DYNAMIC_VISUAL	*
*  to TRUE in order for the colormap to work correctly.			*
* Return the handler of graphics device or NULL.                        *
*									*/
Gdev *
g_device_attach(Canvas canvas, Siscms *siscms)
{
   Gdev *gdev;
   int   map_class;

   WARNING_OFF(Sid);

   if ((gdev = (Gdev *)calloc(1, sizeof(Gdev))) == NULL)
   {
      STDERR("cannot create graphics device");
      return(NULL);
   }

   gdev->xdpy = (Display *)xv_get(canvas, XV_DISPLAY);
   gdev->xid = (XID)xv_get(canvas_paint_window(canvas), XV_XID);
   gdev->canvas = canvas;
   map_class = (int) xv_get(canvas, XV_VISUAL_CLASS);

   /* Check if we need to set the colormap for canvas */
   /* (But note that if siscms==NULL, we will bomb after this section.) */
   if (siscms)
   {
      int i;

      /* User defined colormap */
      gdev->siscms = siscms;

      /* Xview colormap */
      if (map_class != PseudoColor)
         gdev->xvcms.type = XV_STATIC_CMS;
      else
         gdev->xvcms.type = XV_DYNAMIC_CMS;

      gdev->xvcms.size = siscms->cms_size;
      gdev->xvcms.rgb_count = siscms->cms_size;
      gdev->xvcms.index = 0;
      gdev->xvcms.red = siscms->r;
      gdev->xvcms.green = siscms->g;
      gdev->xvcms.blue = siscms->b;
      
      /* Attach to the SIS colormap */
      xv_set(gdev->canvas, WIN_CMS_NAME, "SIS_CMS_NAME", NULL);

      /* X pixel values */
      gdev->xcolor = (u_long *)xv_get(gdev->canvas, WIN_X_COLOR_INDICES);

      /* Inverse X pixel values */
      for (i=0; i<siscms->cms_size; i++)
	 gdev->inv_xcolor[(u_char)(gdev->xcolor[i])] = i;
   }
   /*
   XSetWindowBackground(gdev->xdpy, gdev->xid, gdev->xcolor[0]);
   */
   gdev->xgcval.background = gdev->siscms->size_cms1 ? gdev->xcolor[0] :
	 WhitePixel(gdev->xdpy, DefaultScreen(gdev->xdpy));
   gdev->xgcval.foreground = gdev->siscms->size_cms1 ? 
			     gdev->xcolor[gdev->siscms->size_cms1 - 1] :
	 BlackPixel(gdev->xdpy, DefaultScreen(gdev->xdpy));
   gdev->xgcval.function = GXxor;
   gdev->xgc = XCreateGC(gdev->xdpy, gdev->xid,
      GCFunction | GCForeground | GCBackground, &(gdev->xgcval));

   /* Get the font */
   gdev->font = (Xv_font)xv_find(gdev->canvas, FONT,
	FONT_FAMILY,    FONT_FAMILY_LUCIDA,
	FONT_STYLE,	FONT_STYLE_BOLD,
	NULL);

   return(gdev);
}


/************************************************************************
*									*
* Destroy the graphics device handler.  Note that it automatically      *
* destroys item 'siscms'.  If the user doesn't want it to be destroyed  *
* the user has to set it to be NULL before calling this routine.        * 
*									*/
void
g_device_destroy(Gdev *gdev)
{
   if (gdev->siscms)
      siscms_destroy(gdev->siscms);

   (void)free((char *)gdev);
}

/************************************************************************
*									*
*  Draw multiple points.						*
*									*/
void
g_draw_points(Gdev *gdev, Gpoint *points, int npoints, int color)
{
   G_Set_Color(gdev, color)
   XDrawPoints(gdev->xdpy, gdev->xid, gdev->xgc, points, npoints,
	      CoordModeOrigin);
}

/************************************************************************
*									*
* Draw a line.                                                          *
* (Equivalent to XDrawLine)                                             *
*                                                                       */ 
void
g_draw_line(Gdev *gdev, int x1, int y1, int x2, int y2, int color)
{ 
   G_Set_Color(gdev, color);
   XDrawLine(gdev->xdpy, gdev->xid, gdev->xgc, x1, y1, x2, y2);
}

/************************************************************************
*									*
* Draw multi-connected-lines.                                           *
*									*/
void
g_draw_connected_lines(Gdev *gdev, Gpoint *points, int npoints, int color)
{
   G_Set_Color(gdev, color)
   XDrawLines(gdev->xdpy, gdev->xid, gdev->xgc, points, npoints,
	      CoordModeOrigin);
}

/************************************************************************
*									*
* Draw a rectangle.                                                     * 
* (Equivalent to XDrawRectangle but use Xgl may be faster)              *
*                                                                       */ 
void
g_draw_rect(Gdev *gdev, int x, int y, int wd, int ht, int color)
{ 
   G_Set_Color(gdev, color);
   XDrawRectangle(gdev->xdpy, gdev->xid, gdev->xgc, x, y, wd, ht);
}

/************************************************************************
*									*
* Erase area with all zeros.     	                                *
* (Equivalent to XClearArea but use Xgl may be faster,
* and XClearArea does not work on pixmaps.)
*                                                                       */ 
void
g_zero_area(Gdev *gdev, int x, int y, int wd, int ht) 
{ 
   int func;

   func = G_Get_Op(gdev);
   G_Set_Op(gdev,GXclear);
   XFillRectangle(gdev->xdpy, gdev->xid, gdev->xgc, x, y, wd, ht);
   G_Set_Op(gdev,func);
}

/************************************************************************
*									*
* Fill area with all ones.     	                                *
* (Equivalent to XClearArea but use Xgl may be faster,
* and XClearArea does not work on pixmaps.)
*                                                                       */ 
void
g_fill_area_ones(Gdev *gdev, int x, int y, int wd, int ht) 
{ 
   int func;

   func = G_Get_Op(gdev);
   G_Set_Op(gdev,GXset);
   XFillRectangle(gdev->xdpy, gdev->xid, gdev->xgc, x, y, wd, ht);
   G_Set_Op(gdev,func);
}

/************************************************************************
*									*
* Erase area with a specific color.                                     *
* (Equivalent to XClearArea but use Xgl may be faster,
* and XClearArea does not work on pixmaps.)
*                                                                       */ 
void
g_clear_area(Gdev *gdev, int x, int y, int wd, int ht, int color) 
{ 
   /*XSetWindowBackground(gdev->xdpy, gdev->xid, gdev->xcolor[color]);
   XClearArea(gdev->xdpy, gdev->xid, x, y, wd, ht, FALSE);*/
   int func;

   func = G_Get_Op(gdev);
   G_Set_Op(gdev,GXcopy);
   XSetForeground(gdev->xdpy, gdev->xgc, gdev->xcolor[color]);
   XFillRectangle(gdev->xdpy, gdev->xid, gdev->xgc, x, y, wd, ht);
   G_Set_Op(gdev,func);
}

/************************************************************************
*									*
* Erase area with a default backgroubd color.                           *
* (Equivalent to XClearArea but use Xgl may be faster)                  *
*                                                                       */ 
void
g_clear_area_default(Gdev *gdev, int x, int y, int wd, int ht) 
{ 
   XClearArea(gdev->xdpy, gdev->xid, x, y, wd, ht, FALSE);
}

/************************************************************************
*									*
*  Fill the polygon.							*
*									*/
void
g_fill_polygon(Gdev *gdev, Gpoint *points, int npoints, int color)
{
   G_Set_Color(gdev, color);
   XFillPolygon(gdev->xdpy, gdev->xid, gdev->xgc, points, npoints,
      Complex, CoordModeOrigin);
}

/************************************************************************
*									*
*  Fill the polygon with a default foreground color.			*
*									*/
void
g_fill_polygon_default(Gdev *gdev, Gpoint *points, int npoints)
{
   XFillPolygon(gdev->xdpy, gdev->xid, gdev->xgc, points, npoints,
      Complex, CoordModeOrigin);
}

/************************************************************************
*									*
*  Routines to get fonts of various sizes.
*									*/
Xv_font *
g_get_font_small(Gdev *gdev)
{
    if (!xvfont_small){
	xvfont_small = (Xv_font *)malloc(sizeof(Xv_font));
	*xvfont_small = (Xv_font)xv_find(gdev->canvas,
					 FONT,
					 FONT_RESCALE_OF,
					 gdev->font,
					 WIN_SCALE_SMALL,
					 NULL);
    }
    return xvfont_small;
}
	
Xv_font *
g_get_font_medium(Gdev *gdev)
{
    if (!xvfont_medium){
	xvfont_medium = (Xv_font *)malloc(sizeof(Xv_font));
	*xvfont_medium = (Xv_font)xv_find(gdev->canvas,
					  FONT,
					  FONT_RESCALE_OF,
					  gdev->font,
					  WIN_SCALE_MEDIUM,
					  NULL);
    }
    return xvfont_medium;
}

Xv_font *
g_get_font_large(Gdev *gdev)
{
    if (!xvfont_large){
	xvfont_large = (Xv_font *)malloc(sizeof(Xv_font));
	*xvfont_large = (Xv_font)xv_find(gdev->canvas,
					 FONT,
					 FONT_RESCALE_OF,
					 gdev->font,
					 WIN_SCALE_LARGE,
					 NULL);
    }
    return xvfont_large;
}

Xv_font *
g_get_font_extralarge(Gdev *gdev)
{
    if (!xvfont_extralarge){
	xvfont_extralarge = (Xv_font *)malloc(sizeof(Xv_font));
	*xvfont_extralarge = (Xv_font)xv_find(gdev->canvas,
					      FONT,
					      FONT_RESCALE_OF,
					      gdev->font,
					      WIN_SCALE_EXTRALARGE,
					      NULL);
    }
    return xvfont_extralarge;
}

/************************************************************************
 *									*
 *  Draw a string.							*
 *									*/
void
g_draw_string(Gdev *gdev, int x, int y, Gfont_size fsize, char *str, int color)
{
    Xv_font *xvfont;	/* XView font structure */
    XFontStruct *xfont;	/* X11 font structure */

   switch (fsize)
   {
      case FONT_SMALL: xvfont = g_get_font_small(gdev); break;
      case FONT_MEDIUM: xvfont = g_get_font_medium(gdev); break;
      case FONT_LARGE: xvfont = g_get_font_large(gdev); break;
      case FONT_EXTRALARGE: xvfont = g_get_font_extralarge(gdev); break;
      case FONT_DEFAULT: xvfont = g_get_font_medium(gdev); break;
   }

   xfont = (XFontStruct *)xv_get(*xvfont, FONT_INFO);

   gdev->xgcval.font = xfont->fid;
   gdev->xgcval.foreground = gdev->xcolor[color];

   XChangeGC(gdev->xdpy, gdev->xgc, GCForeground | GCFont , &(gdev->xgcval));
   G_Set_Color(gdev, color);
   XDrawString(gdev->xdpy, gdev->xid, gdev->xgc, x, y, str, strlen(str));
}


/************************************************************************
*									*
*  Get the width of a string using the specified font                   *
*									*/
int
g_get_string_width(Gdev *gdev, Gfont_size fsize, char *str,
		   int& width, int& height, int& ascent, int& descent,
		   int& direction)
{
  Xv_font *xvfont;	/* XView font structure */
  XFontStruct *xfont;	/* X11 font structure */
  XCharStruct overall;

  switch (fsize)  {
  case FONT_SMALL: xvfont = g_get_font_small(gdev); break;
  case FONT_MEDIUM: xvfont = g_get_font_medium(gdev); break;
  case FONT_LARGE: xvfont = g_get_font_large(gdev); break;
  case FONT_EXTRALARGE: xvfont = g_get_font_extralarge(gdev);break;
  case FONT_DEFAULT: xvfont = g_get_font_medium(gdev); break;
  }

  xfont = (XFontStruct *)xv_get(*xvfont, FONT_INFO);
  XTextExtents(xfont, str, strlen(str),
	       &direction, &ascent, &descent, &overall);
  width = overall.width;
  height = overall.ascent + overall.descent;
  return(width);
}

/*
*	Create colored server images (pixmaps) to be used for panel or
*	menu choice images.  Makes a color chip for each entry in the
*	first color map segment (the "marker" colors).
*	Chips to be accessed later by the get_color_chip() routine.
*/
void
initialize_color_chips(Gdev *gdev)
{
    static short solid_bits[] = {
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff
    };

    n_color_chips = G_Get_Sizecms1(gdev);
    chip_pixmaps = (Pixmap *)malloc(n_color_chips * sizeof(Pixmap));
    chip_images = (Server_image *)malloc(n_color_chips * sizeof(Server_image));
    int i;
    for (i=0; i<n_color_chips; i++){
	chip_pixmaps[i] =
	XCreatePixmapFromBitmapData(gdev->xdpy,
				    gdev->xid,
				    (char *)solid_bits,
				    16, 16,
				    gdev->xcolor[G_Get_Stcms1(gdev) + i],
				    0,
				    DefaultDepth(gdev->xdpy,
						 DefaultScreen(gdev->xdpy)));
	chip_images[i] =
	(Server_image)xv_create(XV_NULL, SERVER_IMAGE,
				SERVER_IMAGE_PIXMAP, chip_pixmaps[i],
				NULL);
    }
}

/************************************************************************
 *                                                                       *
 *  Get a color chip server image.
 *									*/
Server_image
get_color_chip(int n)
{
    if (n >= n_color_chips || n < 0 || !chip_images){
	return 0;
    }else{
	return chip_images[n];
    }
}

