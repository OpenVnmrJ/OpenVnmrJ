/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef _GRAPHICS_H
#define	_GRAPHICS_H

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

#include <sys/types.h>
#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <xview/font.h>
#include "siscms.h"
#include "vsfunc.h"

/* This aperture value is used to indicate the sensitivity between */
/* an object and the mouse position.  That is, if the distance is  */
/* less than this aperture, the object will be selected.           */
#define	G_APERTURE	8

/* Initialized value for position on the screen.  We use 	   */
/* negative value because all positive values from 0 to 1152 are   */
/* valid values. (Note that the size of Sun Computer screen is	   */
/* 1152 x 900.)							   */
#define	G_INIT_POS	-1

/************************************************************************
*									*
*  Aliases.								*
*									*/
typedef int		Flag;	/* Alias for TRUE and FALSE value */
typedef XPoint		Gpoint; /* struct { short x,y;} */
typedef struct _gfpoint
{
   float x;
   float y;
} Gfpoint;

/************************************************************************
*									*
* Our own graphics device handler.  This contains the information 	*
* related to drawing/rendering something on the Canvas screen.  It has	*
* Xgl and Xlib structures.  Those are:					*
*									*
*	canvas : XView window canvas (area for drawing)			*
*	Display: X11 display structure					*
*	XID    : X11 ID for graphics drawing (related to canvas)	*
*	GC     : X11 graphics context structure 			*
* 	XGCValues: X11 GC values (controlling graphics drawing)		*
*	siscms : User defined colormap.  Typically it is loaded		*
*		 from the file.						*
*       xvcms  : XView colormap (used in canvas)			*
*                (Note that xvcms points to (use) the same red/gree/blue*
*		 in siscms)						*
*       xcolor : a mapping color from user pixel value to X11 pixel.  	*
*		 All pixels need to be converted to this pixel value	*
* 	inv_xcolor : 							*
*		 a mapping color from X11 pixel to user pixel value.	*
*	font   : a standard font of FONT_FAMILY_LUCIDA with size 12 is 	*
*		 loaded.  Also, It actually loads 4 sizes: 10, 12, 14, 	*
* 		 19.  To use one of this sizes, use macros below	*
*		 myfont = G_Get_Font_xxxx(gdev).			*
*									*/
typedef struct _graphics_dev
{
#ifdef XGL
   Xgl_2d_ctx	ctx2d;	/* Xgl 2D graphics context */
#endif XGL
   Canvas	canvas; /* Xview canvas */
   Display     *xdpy;	/* X display device */
   XID		xid;	/* X window to be drawn */
   GC		xgc;	/* X graphics context, used for default */
   XGCValues	xgcval; /* X graphics context values to be masked */
   Siscms      *siscms;	/* Original (user defined) colormap */
   Xv_cmsdata   xvcms;  /* Xview colormap */
   u_long      *xcolor; /* mapping color from original to X color */
   u_char	inv_xcolor[256];   /* mapping X color to original color */
   Xv_font	font;	/* Xview font */
} Gdev;

/************************************************************************
*									*
*  Macros relating to colormap 						*
*  G_Update_Colormap(gd)						*
*  	Load colormap into Gdev canvas					*
*  G_Xcolor(gd,pixel)							*
*  	Get an X11 pixel value, given our own pixel value 		*
*  G_Inverse_Xcolor(gd,xpixel)						*
*	Get our pixel value, given X11 pixel value			*
*  G_Set_Background_Color(gd,color)					*
*	Set Gdev canvas background color				*
*  G_Set_Foreground_Color						*
*	Set Gdev canvas foreground color				*
*									*/
#define	G_Update_Colormap(gd) xv_set((gd)->canvas, \
		WIN_CMS_NAME,   "SIS_CMS_NAME", \
		WIN_CMS_DATA,   &((gd)->xvcms), \
		NULL);
#define	G_Xcolor(gd,pixel) ((gd)->xcolor[(pixel)])
#define	G_Inverse_Xcolor(gd,xpixel) (u_char)((gd)->inv_xcolor[(xpixel)])
#define	G_Set_Background_Color(gd,color) \
	XSetBackground((gd)->xdpy, (gd)->xgc, (gd)->xcolor[color]);
#define	G_Set_Foreground_Color(gd,color) \
	XSetForeground((gd)->xdpy, (gd)->xgc, (gd)->xcolor[color]);

/************************************************************************
*									*
*  The following macros related to font.				*
*  G_Set_Font(gd,xfont)							*
*	Set the font to be used for drawing in Gdev canvas		*
*  G_Get_Font(gd)							*
*	Get the current font in Gdev canvas				*
*  G_Get_Font_Small(gd)							*
*	Get the font size of 10						*
*  G_Get_Font_Medium(gd)						*
*	Get the font size of 12						*
*  G_Get_Font_Large(gd)							*
*	Get the font size of 14						*
*  G_Get_Font_Extralarge(gd)						*
*	Get the font size of 19						*
*									*/
typedef enum
{
   FONT_SMALL		= WIN_SCALE_SMALL,	/* size 10 */
   FONT_MEDIUM		= WIN_SCALE_MEDIUM,	/* size 12 (default) */
   FONT_LARGE		= WIN_SCALE_LARGE,	/* size 14 */
   FONT_EXTRALARGE	= WIN_SCALE_EXTRALARGE,	/* size 19 */
   FONT_DEFAULT
}Gfont_size;

/* Default font */
#define	G_Set_Font(gd,xfont)	(gd)->xgcval.font = (xfont)->fid; \
	XChangeGC((gd)->xdpy, (gd)->xgc, GCFont, &((gd)->xgcval))
#define G_Get_Font(gd)		(gd)->font

/* Get font with specific size. Note that G_Get_Font_Medium equal to */
/* default font, G_Get_Font.					     */
#define	G_Get_Font_Small(gd) (Xv_font)xv_find((gd)->canvas, FONT, \
		FONT_RESCALE_OF, (gd)->font, WIN_SCALE_SMALL, NULL);
#define	G_Get_Font_Medium(gd) (Xv_font)xv_find(gd->canvas, FONT, \
		FONT_RESCALE_OF, (gd)->font, WIN_SCALE_MEDIUM, NULL);
#define	G_Get_Font_Large(gd) (Xv_font)xv_find((gd)->canvas, FONT, \
		FONT_RESCALE_OF, (gd)->font, WIN_SCALE_LARGE, NULL);
#define	G_Get_Font_Extralarge(gd) (Xv_font)xv_find((gd)->canvas, FONT, \
		FONT_RESCALE_OF, (gd)->font, WIN_SCALE_EXTRALARGE, NULL);

/************************************************************************
*									*
* The following macros are used to manipulate one of XGCValues 		*
* The macros don't necessary represent all values in XGCValues. They	*
* are defined when it is necessary.					*
*									*/
#define	G_Set_Op(gd,val) (gd)->xgcval.function = (val); \
	XChangeGC((gd)->xdpy, (gd)->xgc, GCFunction, &((gd)->xgcval))
#define	G_Set_LineWidth(gd,val) (gd)->xgcval.line_width = (val); \
	XChangeGC((gd)->xdpy, (gd)->xgc, GCLineWidth, &((gd)->xgcval))
#define	G_Set_LineStyle(gd,val) (gd)->xgcval.line_style = (val); \
	XChangeGC((gd)->xdpy, (gd)->xgc, GCLineStyle, &((gd)->xgcval))
#define G_Set_Color(gd,val)  XSetForeground((gd)->xdpy, (gd)->xgc, \
	((gd)->xgcval.function == GXxor) ? \
        ((gd)->xcolor[0] ^ (gd)->xcolor[val]) : (gd)->xcolor[val]);
#define	G_Get_Op(gd) (gd)->xgcval.function
#define	G_Get_LineWidth(gd) (gd)->xgcval.line_width
#define	G_Get_LineStyle(gd) (gd)->xgcval.line_style

/************************************************************************
*									*
*  The following macros are related to gdev->siscms colors.		*
*  (Look at siscms.h for more details.)					*
*  G_Get_Stcms1(gd)							*
*  G_Get_Stcms2(gd)							*
*  G_Get_Stcms3(gd)							*
*	Get the starting index position of Siscms colormap 1, 2, and 3	*
*  G_Get_Sizecms1(gd)							*
*  G_Get_Sizecms2(gd)							*
*  G_Get_Sizecms3(gd)							*
*	Get the size of Siscms colormap 1, 2, and 3			*
*  G_Get_Siscms_Red(gd)							*
*  G_Get_Siscms_Green(gd)						*
*  G_Get_Siscms_Blue(gd)						*
*	Get the address pointer of colormap red, green and blue		*
*									*/
#define	G_Get_Stcms1(gd) 	(gd)->siscms->st_cms1
#define	G_Get_Stcms2(gd) 	(gd)->siscms->st_cms2
#define	G_Get_Stcms3(gd) 	(gd)->siscms->st_cms3
#define	G_Get_Sizecms1(gd) 	(gd)->siscms->size_cms1
#define	G_Get_Sizecms2(gd) 	(gd)->siscms->size_cms2
#define	G_Get_Sizecms3(gd) 	(gd)->siscms->size_cms3
#define	G_Get_Siscms_Red(gd)	(gd)->siscms->r
#define	G_Get_Siscms_Green(gd)	(gd)->siscms->g
#define	G_Get_Siscms_Blue(gd)	(gd)->siscms->b

/************************************************************************
*									*
*  Gdev_Win_Width(gd)							*
*	Get the size of Gdev canvas width 				*
*  Gdev_Win_Height(gd)							*
*	Get the size of Gdev canvas height 				*
*									*/
#define	Gdev_Win_Width(gd)	(int)(xv_get((gd)->canvas, XV_WIDTH))
#define	Gdev_Win_Height(gd)	(int)(xv_get((gd)->canvas, XV_HEIGHT))

/************************************************************************
*									*
*  Defininition used to indicate where the data starts drawing image.	*
*									*/
typedef enum
{
    TOP,        /* if data lines start at top image scan lines */
    BOTTOM,     /* Vnmr phasefiles, bass ackwards */
    LEFT,       /* if data lines are left indexed vertical scan lines */
    RIGHT       /* if data lines are right indexed horizontal scan lines */
} Orientation;

/************************************************************************
*									*
* This routine initializes everything related to the graphics.  This	*
* includes graphics context and colormap.				*
* Return the handler of graphics device or NULL.			*
*									*/
extern Gdev *
g_device_create(Canvas canvas, Siscms *siscms);

/************************************************************************
*									*
* This routine initializes almost everything related to the graphics.
* This includes graphics context and colormap info., but the xview
* colormap segment is not actually initialized.
* Return the handler of graphics device or NULL.			*
*									*/
extern Gdev *
g_device_attach(Canvas canvas, Siscms *siscms);

/************************************************************************
*									*
* Destroy the graphics device handler.  Note that it automatically	*
* destroys item 'siscms'.  If the user doesn't want it to be destroyed  *
* the user has to set it to be NULL before calling this routine.	*
*									*/
extern void
g_device_destroy(Gdev *gdev);

/************************************************************************
*									*
* Set 1 pixel value at n positions.					*
* (Equivalent to looping Xgl_context_set_pixel or XDrawPoints)		*
*									*/
extern void
g_draw_points(Gdev *gdev,	/* graphics handler */
	Gpoint *points,		/* an array of points */
	int npoints,		/* number of points */
	int color);		/* color value */

/************************************************************************
*									*
* Draw a line.								*
* (Equivalent to XDrawLine)						*
*									*/
extern void
g_draw_line(Gdev *gdev,		/* graphics handler */
	int x1, int y1,		/* starting point */
	int x2, int y2,		/* ending point */
	int color);		/* color value */


/************************************************************************
*									*
* Draw multi-connected-lines.						*
* (Equivalent to XDrawLines)						*
*									*/
extern void
g_draw_connected_lines(Gdev *gdev,	/* graphics handler */
	Gpoint *points,		/* an array of points to form lines */
	int npoints,		/* number of points */
	int color);		/* color value */

/************************************************************************
*									*
* Draw a rectangle.							*
* (Equivalent to XDrawRectangle but use Xgl may be faster)		*
*									*/
extern void
g_draw_rect(Gdev *gdev,		/* graphics handler */
	int x, int y,		/* starting position */
	int wd, int ht,		/* width and height of the rectangle */
	int color);		/* color value */

/************************************************************************
*									*
* Erase area with all zeros.     	                                *
* (Equivalent to XClearArea but use Xgl may be faster,
* and XClearArea does not work on pixmaps.)
*                                                                       */ 
extern void
g_zero_area(Gdev *gdev,		/* graphics handler */
	int x, int y,		/* starting position */
	int wd, int ht); 	/* width and height of the rectangle */

/************************************************************************
*									*
* Fill area with all ones.     	                                *
* (Equivalent to XClearArea but use Xgl may be faster,
* and XClearArea does not work on pixmaps.)
*                                                                       */ 
extern void
g_fill_area_ones(Gdev *gdev,	/* graphics handler */
	int x, int y,		/* starting position */ 
	int wd, int ht); 	/* width and height of the rectangle */

/************************************************************************
*									*
* Erase area with a specific color.					*
* (Equivalent to XClearArea but use Xgl may be faster)			*
*									*/
extern void
g_clear_area(Gdev *gdev,	/* graphics handler */
	int x, int y,		/* starting position */
	int wd, int ht,		/* width and height to be erased */
	int color);		/* color value */

/************************************************************************
*									*
* Erase area with a default background color.				*
* (Equivalent to XClearArea but use Xgl may be faster)			*
*									*/
extern void
g_clear_area_default(Gdev *gdev,	/* graphics handler */
	int x, int y,			/* starting position */
	int wd, int ht);		/* width and height to be erased */

/************************************************************************
*                                                                       *
*  Fill the polygon.                                                    *
*                                                                       */
extern void
g_fill_polygon(Gdev *gdev, 	/* graphics handler */
	Gpoint *points, 	/* A list of points (type of short) */
	int npoints,		/* Number of points */
	int color);		/* color value */

/************************************************************************
*                                                                       *
*  Fill the polygon with a default foreground color.			*
*                                                                       */
extern void
g_fill_polygon_default(Gdev *gdev, 	/* graphics handler */
	Gpoint *points, 	/* A list of points (type of short) */
	int npoints);		/* Number of points */

/************************************************************************
*                                                                       *
*  Draw a string.							*
*									*/
extern void
g_draw_string(Gdev *gdev,	/* graphics handler */
	int x, int y,		/* starting position */
	Gfont_size size,	/* Font size */
	char *str,		/* string to be drawn */
	int color);		/* color */

/************************************************************************
*                                                                       *
*  Get the dimensions of a string
*									*/
extern int
g_get_string_width(Gdev *gdev,	/* graphics handler */
	Gfont_size size,	/* Font size */
	char *str,		/* string to be drawn */
	int& width,		/* Width of string in pixels */
	int& height,		/* Height of string in pixels */
	int& ascent,		/* Distance from top to baseline */
	int& descent,		/* Distance from baseline to bottom */
	int& direction);	/* FontRightToLeft or FontLeftToRight */

 
/************************************************************************
*                                                                       *
*  This function builds the image pixel array. It duplicates scan lines *
*  when expanding an image, and uses maximum of groups of scan lines 	*
*  when compressing an image. It does not do any smoothing of expanded 	*
*  image data.								*
*									*
*  Orientation is to indicate the format of input data, where its trace *
*  starts from TOP, BOTTOM, LEFT, or RIGHT. (Currently, it supports TOP *
*  and BOTTOM only).  Note that Vnmr phasefile data starts from BOTTOM. *
*									*
*  Note "indata" should point to data which have the size of 		*
*  'points_per_line' * 'scan_lines' (in floating point).  The "outdata"	*
*  should point to the (unsigned character) buffer which have the size	*
*  of 'pix_width * pix_height'.	 The result output data is the original	*
*  pixel data, which has not been converted to xcolor. 			*
*									*/
extern void
g_float_to_pixel(float *indata, /* pointer to input (floating point) data */
        char *outdata,  	/* pointer to output pixel data */
 	int data_wd,    	/* # data points per scan line (width)*/
 	int data_ht,         	/* scan lines (height of) data */
        int src_stx,   		/* x offset in data to start at */
        int src_sty,   		/* y offset in data to start at */
        int src_width,      	/* width of data to use */
        int src_height,     	/* height of data to use */
        int pix_width,    	/* (resulting) image width in pixels */
        int pix_height,    	/* (resulting) image height in pixels */
        float vs,       	/* vertical scale to apply */
	int pixel_offset,	/* pixel offset to add in */
	int num_pixel_level,	/* number of pixel level */
        Orientation direction);	/* indata orientation */

/* Same as above excepts the input data is type of short with maximum 	*/
/* data value of 4095 (12 bits).					*/
extern void
g_short_to_pixel(short *indata, /* pointer to input data */
        char *outdata,  	/* pointer to output pixel data */
 	int data_wd,    	/* # data points per scan line (width)*/
 	int data_ht,         	/* scan lines (height of) data */
        int src_stx,   		/* x offset in data to start at */
        int src_sty,   		/* y offset in data to start at */
        int src_width,      	/* width of data to use */
        int src_height,     	/* height of data to use */
        int pix_width,    	/* (resulting) image width in pixels */
        int pix_height,    	/* (resulting) image height in pixels */
        float vs,       	/* vertical scale to apply */
	int pixel_offset,	/* pixel offset to add in */
	int num_pixel_level,	/* number of pixel level to use */
        Orientation direction);	/* indata orientation */

/* Same as above, except the input data is type of short, and output 	*/
/* data (result) is also type of short.  Data orientation is from   	*/
/* Top to Bottom.  No vertical-scale is needed since the result data 	*/
/* is a compressed or expanded of the input data.		    	*/
void
g_short_to_short(short *indata, /* pointer to input data */
        short *outdata,         /* pointer to output data */
        int data_wd,    	/* # data points per scan line (width)*/
        int data_ht,         	/* scan lines (height of) data */
        int src_stx,            /* x offset in data to start at */
        int src_sty,            /* y offset in data to start at */
        int src_width,          /* width of data to use */
        int src_height,         /* height of data to use */
        int pix_width,          /* (resulting) data width */
        int pix_height);	/* (resulting) data height */

/* Same as above, except the input data is type of float, and output 	*/
/* data (result) is also type of float.  Data orientation is from   	*/
/* Top to Bottom.  No vertical-scale is needed since the result data 	*/
/* is a compressed or expanded of the input data.		    	*/
extern void
g_float_to_float(float *indata, /* pointer to input data */
        float *outdata,         /* pointer to output data */
        int data_wd,    	/* # data points per scan line (width)*/
        int data_ht,         	/* scan lines (height of) data */
        int src_stx,            /* x offset in data to start at */
        int src_sty,            /* y offset in data to start at */
        int src_width,          /* width of data to use */
        int src_height,         /* height of data to use */
        int pix_width,          /* (resulting) data width */
        int pix_height);	/* (resulting) data height */

/************************************************************************
*                                                                       *
*  This routine is used to display 2D image, where the input data is	*
*  absolute floating values (4 bytes/pixel) -- NO negative value.	*
*  									*
*  The siscms index allows the user to specify either the gray-scale 	*
*  colormap or the false color colormap (typically SISCMS_2 or SISCMS_3)*
*  depending on your colormap set-up (Look at "siscms.h")		*
*									*
*  Orientation is to indicate the format of input data, where its trace *
*  starts from TOP, BOTTOM, LEFT, or RIGHT. (Currently, it supports TOP *
*  and BOTTOM only).  Note that Vnmr phasefile data starts from BOTTOM. *
*									*
*									*/
extern Pixmap
g_display_image(Gdev *gdev,     /* graphics device handler */
 	Siscms_type siscms,     /* index of siscms Colormap to use */
 	float *data,            /* pointer to data points */
 	int data_wd,    	/* # data points per scanline */
 	int data_ht,         	/* # scanlines (height of) data */
 	int src_stx,            /* x offset into source data */
 	int src_sty,            /* y offset into source data */
 	int src_wd,             /* # of data width to use , must be */
                                        /* <= points_per_line */
 	int src_ht,             /* # of data height to use, must be */
                                        /* <= scan_lines */
 	int pix_stx,            /* starting pixel (x) position in canvas */
 	int pix_sty,            /* starting pixel (y) poxition in canvas */
 	int pix_wd,             /* # of width pixels to draw in canvas */
 	int pix_ht,             /* # of height pixels to draw in canvas */
 	Orientation direction,  /* end from which data encoding */
 	float vs,               /* vertical scaling of data */
	int keep_pixmap=FALSE);	/* If TRUE, return a pixmap of the image */

/* Same as above, but with a vsfunc instead of vs */
extern Pixmap
g_display_image(Gdev *gdev,     /* graphics device handler */
 	Siscms_type siscms,     /* index of siscms Colormap to use */
 	float *data,            /* pointer to data points */
 	int data_wd,    	/* # data points per scanline */
 	int data_ht,         	/* # scanlines (height of) data */
 	int src_stx,            /* x offset into source data */
 	int src_sty,            /* y offset into source data */
 	int src_wd,             /* # of data width to use , must be */
                                        /* <= points_per_line */
 	int src_ht,             /* # of data height to use, must be */
                                        /* <= scan_lines */
 	int pix_stx,            /* starting pixel (x) position in canvas */
 	int pix_sty,            /* starting pixel (y) poxition in canvas */
 	int pix_wd,             /* # of width pixels to draw in canvas */
 	int pix_ht,             /* # of height pixels to draw in canvas */
 	Orientation direction,  /* end from which data encoding */
 	VsFunc *vsfunc,		/* vertical scaling of data */
	int keep_pixmap=FALSE);	/* If TRUE, return a pixmap of the image */

/* Same as above excepts the input data is type of short with maximum 	*/
/* data value of 4095 (12 bits).					*/
extern void
g_display_short_image(Gdev *gdev,     /* graphics device handler */
 	Siscms_type siscms,     /* index of siscms Colormap to use */
 	short *data,            /* pointer to data points */
 	int data_wd,    	/* # data points per scanline */
 	int data_ht,         	/* # scanlines (height of) data */
 	int src_stx,            /* x offset into source data */
 	int src_sty,            /* y offset into source data */
 	int src_wd,             /* # of data width to use , must be */
                                        /* <= points_per_line */
 	int src_ht,             /* # of data height to use, must be */
                                        /* <= scan_lines */
 	int pix_stx,            /* starting pixel (x) position in canvas */
 	int pix_sty,            /* starting pixel (y) poxition in canvas */
 	int pix_wd,             /* # of width pixels to draw in canvas */
 	int pix_ht,             /* # of height pixels to draw in canvas */
 	Orientation direction,  /* end from which data encoding */
 	float vs);              /* vertical scaling of data */

/************************************************************************
*                                                                       *
*	Create colored server images (pixmaps) to be used for panel or
*	menu choice images.  Makes a color chip for each entry in the
*	first color map segment (the "marker" colors).
*                                                                       */
extern void
initialize_color_chips(Gdev *gd);

/************************************************************************
*                                                                       *
*	Get a color chip.  If "n" is out of range 0 to n_color_chips
*	or the color chips have not been initialized, returns NULL.
*                                                                       */
extern Server_image
get_color_chip(int n);

extern void smooth(void) ;

extern void dont_smooth(void) ;

#endif _GRAPHICS_H
