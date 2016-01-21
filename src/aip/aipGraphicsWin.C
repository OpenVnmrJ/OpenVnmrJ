/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined (MOTIF) && (ORIG)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#else
#include "vjXdef.h"
#endif


#include "aipUtils.h"
#include "aipVnmrFuncs.h"
#include "aipStderr.h"
#include "aipStructs.h"
#include "aipGraphics.h"
#include "aipGraphicsWin.h"
#include "aipInterpolation.h"
#include "aipGframeManager.h"
#include "aipJFuncs.h"

// Move these to aipVnmrFuncs.h
extern char PlotterHost[];
extern char LplotterPort[];
extern Pixmap canvasPixmap;


// Here are the function declartions for the static functions local
// to this file. The function declaration for the public funtions
// like aipDisplayImage() are in aipGraphics.h.

static int
convert_float_data(float *indata,// pointer to input (floating point) data
        char *outdata,          // pointer to output pixel data
        int points_per_line,    // # data points per scan line (width)
        int datasetHeight,         // scan lines (height of) data
        int x_offset,           // x offset in data to start at
        int y_offset,           // y offset in data to start at
        int src_width,          // width of data to use
        int src_height,         // height of data to use
        int pix_width,          // (resulting) image width in pixels
        int pix_height,         // (resulting) image height in pixels
        dataOrientation_t direction);  // indata orientation

static int
convert_float_data(float *indata,
		   char *outdata,
		   int pixwd,
		   int pixht);

static void
line_fill_init(int Line_size,  // pixels per line in image
	       int Data_size);  // points per line in data

static void
line_same (float *indata, char *outdata);

static void
line_expand (float *indata, char *outdata);

static void
line_compress (float *indata, char *outdata);

static void
setImagePixel(char **ppout, u_long xcolor);

static float *
gettrace(float *start,           // start of data array
	 int width,              // width of data array
	 int x_offset,           // X offset into array
	 int trace);             // trace # to retrieve

static void
extract_rect_from_data(float *indata,
		       float *outdata,
		       int points_per_line, // Width of input data
		       int datasetHeight, // Height of input data
		       int x_offset, // Position of extracted  rectangle
		       int y_offset,
		       int width, // Size of extracted rectangle
		       int height);

// end of function declarations

// pointer to function to use for filling out image lines with data points
static void (*LineFill)(float *indata, char *outdata);

static bool raw_flag=false;// use to indicate whether to obtain X11 pixel
			// or regular user pixel

static int   line_size; 	// number of image points in an image line
static int   data_size;  // number of data points available to fill image line
static int   D1, D2; // decision-variable deltas for expanding/compressing
static u_long *x_color;
static int bytesPerPixel;
static int pixFormat;

static gdev_t gdev;
static XID canvas;
static BackStore_t canvasBackup; // Backup store of stuff "behind" ROIs
static palette_t *palettes;

#ifndef MOTIF
typedef struct {
    short x, y;
} XPoint;

typedef struct {
    short x, y;
    unsigned short width, height;
} XRectangle;
#endif

static XRectangle viewport = {-1, -1};

static struct {
    uchar_t *lookuptable;
    int tablesize;
    float mindata;
    float maxdata;
    uchar_t uflowcmi;
    uchar_t oflowcmi;
    uchar_t nancmi;		// For points where data = NaN
    float scale;
} vsdat;


#if defined (MOTIF) && (ORIG)
static bool useActiveMask = false;
static Region visibleMask = NULL; // Exposed part of canvas
static Region pActiveMask = NULL; // Active area on pixmap
static Region cActiveMask = NULL; // Active area on canvas
static Region pDrawMask = NULL; // Mask for pixmap drawing
static Region cDrawMask = NULL; // Mask for canvas drawing

static XFontStruct *fontStruct = NULL;
#endif

static int fontSize = 0;
static int    img_size = 0;
static char  *img_data = NULL;

static string currentCursor;

/* STATIC */
void
GraphicsWin::setCursor(const string cursType)
{
    currentCursor = cursType;
#ifdef ORIG
    string cmd("cursor ");
    cmd += cursType;
    WRItelineToVnmrJ("vnmrjcmd", cmd.c_str());
#else
    aip_setCursor(cursType.c_str() );
#endif
}

/* STATIC */
string
GraphicsWin::getCursor()
{
    return currentCursor;
}

/* STATIC */
void
GraphicsWin::clearWindow()
{
    int bkwd = getBackingWidth();
    int bkht = getBackingHeight();
    int wd = bkwd > viewport.width ? bkwd : viewport.width;
    int ht = bkht > viewport.height ? bkht : viewport.height;
    clearRect(0, 0, wd, ht);
}

/* STATIC */
void
GraphicsWin::validateRect(int& x, int& y, int& w, int& h)
{
    if (x < 0) {
	w += x;
	x = 0;
    }
    if (y < 0) {
	h += y;
	y = 0;
    }
    if (w < 0) {
	w = 0;
    }
    if (h < 0) {
	h = 0;
    }
}

/* STATIC */
void
GraphicsWin::clearRect(int x, int y, int w, int h)
{
    // NB: We use XFillRectangle instead of XClearArea, because
    // XClearArea ignores the clip mask.
    // validateRect(x, y, w, h);
#if defined (MOTIF) && (ORIG)
    validateRect(x, y, w, h);
    if (canvasPixmap && getBackingStoreFlag()) {
	XSetForeground(gdev.xdpy, gdev.pxgc,
		       gdev.cmap[palettes[0].firstColor+0]); // Black
	XFillRectangle(gdev.xdpy, canvasPixmap, gdev.pxgc, x, y, w, h);
    }

    if (gdev.xid == canvas && !okToDrawOnCanvas()) {
        return;
    }
    XSetForeground(gdev.xdpy, gdev.xgc,
		   gdev.cmap[palettes[0].firstColor+0]); // Black
    XFillRectangle(gdev.xdpy, gdev.xid, gdev.xgc,
		   viewport.x + x, viewport.y + y, w, h);
#else
    aip_clearRect(x, y, w, h);
#endif
}

/* STATIC */
void
GraphicsWin::drawRect(int x, int y, int w, int h, int color)
{
    validateRect(x, y, w, h);
#if defined (MOTIF) && (ORIG)
    if (canvasPixmap && getBackingStoreFlag()) {
	XSetForeground(gdev.xdpy, gdev.pxgc,
		   gdev.cmap[palettes[0].firstColor+color]);
	XDrawRectangle(gdev.xdpy, canvasPixmap, gdev.pxgc, x, y, w, h);
    }

    GC gc;
    if (gdev.xid == canvas) {
        if (!okToDrawOnCanvas()) {
            return;
        }
	gc = gdev.xgc;
	G_Set_Op(gdev, GXcopy);/*CMP*/
	x += viewport.x;
	y += viewport.y;
    } else {
	gc = gdev.pxgc;
    }
    XSetForeground(gdev.xdpy, gc, gdev.cmap[palettes[0].firstColor+color]);
    XDrawRectangle(gdev.xdpy, gdev.xid, gc, x, y, w, h);
#else
    aip_drawRect(x, y, w, h, color);
#endif
}

/* STATIC */
void
GraphicsWin::drawOval(int x, int y, int w, int h, int color)
{
    //validateRect(x, y, w, h);
#if defined (MOTIF) && (ORIG)
    if (canvasPixmap && getBackingStoreFlag()) {
	XSetForeground(gdev.xdpy, gdev.pxgc,
		   gdev.cmap[palettes[0].firstColor+color]);
	XDrawArc(gdev.xdpy, canvasPixmap, gdev.pxgc, x, y, w, h, 0, 360*64);
    }

    GC gc;
    if (gdev.xid == canvas) {
        if (!okToDrawOnCanvas()) {
            return;
        }
	gc = gdev.xgc;
	G_Set_Op(gdev, GXcopy);/*CMP*/
	x += viewport.x;
	y += viewport.y;
    } else {
	gc = gdev.pxgc;
    }
    XSetForeground(gdev.xdpy, gc, gdev.cmap[palettes[0].firstColor+color]);
    XDrawArc(gdev.xdpy, gdev.xid, gc, x, y, w, h, 0, 360*64);
#else
    aip_drawOval(x, y, w, h, color);
#endif
}

/* STATIC */
void
GraphicsWin::drawLine(int x1, int y1, int x2, int y2, int color)
{
#if defined (MOTIF) && (ORIG)
    if (canvasPixmap && getBackingStoreFlag()) {
	XSetForeground(gdev.xdpy, gdev.pxgc,
		   gdev.cmap[palettes[0].firstColor+color]);
	XDrawLine(gdev.xdpy, canvasPixmap, gdev.pxgc, x1, y1, x2, y2);
    }

    GC gc;
    if (gdev.xid == canvas) {
        if (!okToDrawOnCanvas()) {
            return;
        }
	gc = gdev.xgc;
	x1 += viewport.x;
	y1 += viewport.y;
	x2 += viewport.x;
	y2 += viewport.y;
	G_Set_Op(gdev, GXcopy);/*CMP*/
    } else {
	gc = gdev.pxgc;
    }
    XSetForeground(gdev.xdpy, gc,
		   gdev.cmap[palettes[0].firstColor+color]);
    XDrawLine(gdev.xdpy, gdev.xid, gc, x1, y1, x2, y2);
#else
    aip_drawLine(x1, y1, x2, y2, color);
#endif
}

/* STATIC */
void
GraphicsWin::drawLine(double x1, double y1, double x2, double y2, int color)
{
#if defined (MOTIF) && (ORIG)
    if (canvasPixmap && getBackingStoreFlag()) {
	XSetForeground(gdev.xdpy, gdev.pxgc,
		   gdev.cmap[palettes[0].firstColor+color]);
        if (clipLineToRect(x1, y1, x2, y2, -32767, -32767, 32767, 32767)) {
            XDrawLine(gdev.xdpy, canvasPixmap, gdev.pxgc, x1, y1, x2, y2);
        }
    }

    GC gc;
    if (gdev.xid == canvas) {
        if (!okToDrawOnCanvas()) {
            return;
        }
	gc = gdev.xgc;
	x1 += viewport.x;
	y1 += viewport.y;
	x2 += viewport.x;
	y2 += viewport.y;
        if (!clipLineToRect(x1, y1, x2, y2, -32767, -32767, 32767, 32767)) {
            return;             // line does not enter clipping rectangle
        }
	G_Set_Op(gdev, GXcopy);/*CMP*/
    } else {
	gc = gdev.pxgc;
    }
    XSetForeground(gdev.xdpy, gc,
		   gdev.cmap[palettes[0].firstColor+color]);
    XDrawLine(gdev.xdpy, gdev.xid, gc, x1, y1, x2, y2);
#else
    if (!clipLineToRect(x1, y1, x2, y2, -32767, -32767, 32767, 32767))
	return;
    aip_drawLine((int)x1, (int)y1, (int)x2, (int)y2, color);
#endif
}

/* STATIC */
void
GraphicsWin::drawPolyline(Dpoint_t *pts, int npts, int color)
{
#if defined (MOTIF) && (ORIG)
    if (canvasPixmap && getBackingStoreFlag()) {
	XSetForeground(gdev.xdpy, gdev.pxgc,
                       gdev.cmap[palettes[0].firstColor+color]);
	drawLines(gdev.xdpy, canvasPixmap, gdev.pxgc, 0, 0, pts, npts);
    }

    GC gc;
    if (gdev.xid == canvas) {
        if (!okToDrawOnCanvas()) {
            return;
        }
	gc = gdev.xgc;
	G_Set_Op(gdev, GXcopy);/*CMP*/
    } else {
	gc = gdev.pxgc;
    }
    XSetForeground(gdev.xdpy, gc,
		   gdev.cmap[palettes[0].firstColor+color]);
    drawLines(gdev.xdpy, gdev.xid, gc, viewport.x, viewport.y, pts, npts);
#else
    aip_drawPolyline(pts, npts, color);
#endif
}

#if defined (MOTIF) && (ORIG)
/* STATIC */
void
GraphicsWin::drawLines(Display *dpy, Drawable xid, GC gc,
                       double xoff, double yoff, Dpoint_t *pts, int npts)
{
    Gpoint_t *ipts = new Gpoint_t[npts];
    if(ipts == NULL) return;

    for (int i = 0; i < npts; ++i) {
	ipts[i].x = (int)pts[i].x;
	ipts[i].y = (int)pts[i].y;
    }
    double x1;
    double y1;
    double x2 = xoff + ipts[0].x;
    double y2 = yoff + ipts[0].y;
    for (int i = 1; i < npts; ++i) {
        x1 = x2;
        y1 = y2;
        x2 = xoff + ipts[i].x;
        y2 = yoff + ipts[i].y;
        if (clipLineToRect(x1, y1, x2, y2, -32767, -32767, 32767, 32767)) {
            XDrawLine(dpy, xid, gc, (int)x1, (int)y1, (int)x2, (int)y2);
        }
    }
    delete[] ipts;
}
#endif

/* STATIC */
void
GraphicsWin::fillPolygon(Gpoint_t *pts, int npts, int color)
{
#if defined (MOTIF) && (ORIG)
    int i;
    XPoint *xpts = new XPoint[npts];
    if(xpts == NULL) return;

    for (i=0; i<npts; i++) {
	xpts[i].x = pts[i].x;
	xpts[i].y = pts[i].y;
    }
    if (canvasPixmap && getBackingStoreFlag()) {
	XSetForeground(gdev.xdpy, gdev.pxgc,
		   gdev.cmap[palettes[0].firstColor+color]);
	XFillPolygon(gdev.xdpy, canvasPixmap, gdev.pxgc,
		     xpts, npts, Complex, CoordModeOrigin);
    }

    GC gc;
    if (gdev.xid == canvas) {
        if (!okToDrawOnCanvas()) {
            return;
        }
	gc = gdev.xgc;
	G_Set_Op(gdev, GXcopy);/*CMP*/
	for (i=0; i<npts; i++) {
	    xpts[i].x += viewport.x;
	    xpts[i].y += viewport.y;
	}
    } else {
	gc = gdev.pxgc;
    }
    XSetForeground(gdev.xdpy, gc,
		   gdev.cmap[palettes[0].firstColor+color]);
    XFillPolygon(gdev.xdpy, gdev.xid, gc,
		 xpts, npts, Complex, CoordModeOrigin);
    delete[] xpts;
#else
    aip_fillPolygon(pts, npts, color);
#endif
}

/* STATIC */
const char *
GraphicsWin::getFontName(int size)
{
    switch (size) {
      case 7:
        return "5x7";
      case 8:
        return "5x8";
      case 9:
        return "6x9";
      case 10:
      case 11:
        return "6x10";
      case 12:
        return "6x12";
      case 13:
        return "7x13bold";
      case 14:
        return "7x14bold";
      case 15:
      case 16:
        return "9x15bold";
      case 17:
      case 18:
      case 19:
      case 20:
        return "10x20";
      case 21:
      case 22:
      case 23:
      case 24:
        return "12x24";
      default:
        return "8x16";
    }
}

/* STATIC */
int
GraphicsWin::loadFont(int size)
{
    static int mySize = -1;
    if (mySize == size)
        return true;
#if defined (MOTIF) && (ORIG)

    if (fontStruct != NULL) {
        XFreeFontInfo(NULL, fontStruct, 1);
        mySize = -1;
    }
    fontStruct = XLoadQueryFont(gdev.xdpy, getFontName(size));
    if (fontStruct == NULL) {
        return false;
    }
    XSetFont(gdev.xdpy, gdev.pxgc, fontStruct->fid);
    XSetFont(gdev.xdpy, gdev.xgc, fontStruct->fid);
#else
    aip_loadFont(size);
#endif
    fontSize = size;
    mySize = size;
    return true;
}

/* STATIC */
void
GraphicsWin::getTextExtents(const char *str, int size, // TODO: No font size
                            int *ascent, int *descent, int *width)
{
    if (fontSize != size) {
        if (!loadFont(size)) {
            return;
        }
    }

#if defined (MOTIF) && (ORIG)

    int direction;
    XCharStruct charStruct;
    XTextExtents(fontStruct, str, strlen(str),
                 &direction, ascent, descent, &charStruct);
    *ascent = charStruct.ascent;
    *descent = charStruct.descent;
    *width = charStruct.width;
#else
    aip_getTextExtents(str, size, ascent, descent, width);
#endif
}

/* STATIC */
void
GraphicsWin::drawString(const char *str, int x, int y,
                        int color, int width, int ascent, int descent)
{
    int clear;

    if (width && (ascent + descent))
	clear = 1;
    else
	clear = 0;
#if defined (MOTIF) && (ORIG)
    if (clear) {
        // Clear this rectangle
        clearRect(x - 1, y - 1 - ascent, width + 2, ascent + descent + 2);
    }
    if (canvasPixmap && getBackingStoreFlag()) {
	XSetForeground(gdev.xdpy, gdev.pxgc,
		   gdev.cmap[palettes[0].firstColor+color]);
	XDrawString(gdev.xdpy, canvasPixmap, gdev.pxgc, x, y, str, strlen(str));
    }

    GC gc;
    if (gdev.xid == canvas) {
        if (!okToDrawOnCanvas()) {
            return;
        }
	gc = gdev.xgc;
	x += viewport.x;
	y += viewport.y;
	G_Set_Op(gdev, GXcopy);/*CMP*/
    } else {
	gc = gdev.pxgc;
    }
    XSetForeground(gdev.xdpy, gc, gdev.cmap[palettes[0].firstColor+color]);
    XDrawString(gdev.xdpy, gdev.xid, gc, x, y, str, strlen(str));
#else
    aip_drawString(str, x, y, clear, color);
#endif
}

/* STATIC */
void
GraphicsWin::setClipRectangle(int x, int y, int wd, int ht)
{
    // Sets Graphics Context for Canvas and Pixmaps
    validateRect(x, y, wd, ht);
#if defined (MOTIF) && (ORIG)
    // Canvas is further masked to currently visible part of canvas
    if (wd==0 && ht==0) {
	useActiveMask = false;

	// No clipping on pixmap
	XSetClipMask(gdev.xdpy, gdev.pxgc, None);
	if (!visibleMask) {
	    // Turn off clipping
	    XSetClipMask(gdev.xdpy, gdev.xgc, None);
	} else {
	    // Clip to visible part of canvas
	    if (!cDrawMask) {
		cDrawMask = XCreateRegion();
	    }
	    XUnionRegion(visibleMask, visibleMask, cDrawMask);
	    XSetRegion(gdev.xdpy, gdev.xgc, visibleMask);
	}
    } else {
	useActiveMask = true;

	// Clip pixmap to specified rect
	XRectangle rectangle[1];
	rectangle->x = x;
	rectangle->y = y;
	rectangle->width = wd;
	rectangle->height = ht;
	if (pActiveMask) {
	    XDestroyRegion(pActiveMask);
	}
	pActiveMask = XCreateRegion();
	XUnionRectWithRegion(rectangle, pActiveMask, pActiveMask);
	if (!pDrawMask) {
	    pDrawMask = XCreateRegion();
	}
	XUnionRegion(pActiveMask, pActiveMask, pDrawMask);
	XSetRegion(gdev.xdpy, gdev.pxgc, pDrawMask);

	// Clip canvas to offset rect
	rectangle->x = x + viewport.x;
	rectangle->y = y + viewport.y;
	if (cActiveMask) {
	    XDestroyRegion(cActiveMask);
	}
	cActiveMask = XCreateRegion();
	XUnionRectWithRegion(rectangle, cActiveMask, cActiveMask);
	if (!cDrawMask) {
	    cDrawMask = XCreateRegion();
	}
	if (!visibleMask) {
	    // Clip to specified rect only
	    XUnionRegion(cActiveMask, cActiveMask, cDrawMask);
	} else {
	    // Clip to intersection of visible area with specified rect
	    XIntersectRegion(cActiveMask, visibleMask, cDrawMask);
	}
	XSetRegion(gdev.xdpy, gdev.xgc, cDrawMask);
    }
#else
    aip_setClipRectangle(x, y, wd, ht);
#endif
}

BackStore_t
GraphicsWin::allocateBackingStore(BackStore_t bs, int width, int height)
{
#if defined (MOTIF) && (ORIG)
    XWindowAttributes  win_attr;
#endif
    BackStore_t newbs;

    if (width <= 0 || height <= 0) {
	// No size specified; use size of canvas
	newbs.width = getWinWidth();
	newbs.height = getWinHeight();
    } else {
	newbs.width = width;
	newbs.height = height;
    }

    if (newbs.width > bs.width || newbs.height > bs.height) {
	// Get a new (larger) pixmap.
	if (newbs.width < bs.width) {newbs.width = bs.width;}
	if (newbs.height < bs.height) {newbs.height = bs.height;}
    } else if (bs.id) {
	return bs;              // Use same old pixmap
    }

#if defined (MOTIF) && (ORIG)
    // General XWindows info
    int depth = DefaultDepth(gdev.xdpy, DefaultScreen(gdev.xdpy));
    //vis = DefaultVisual(gdev.xdpy, DefaultScreen(gdev.xdpy));
    if (XGetWindowAttributes(gdev.xdpy,  gdev.xid, &win_attr))
    {
        //vis = win_attr.visual;
        depth = win_attr.depth;
    }
    newbs.id = (XID_t)XCreatePixmap(gdev.xdpy, gdev.xid,
				    newbs.width, newbs.height, depth);
    if (!newbs.id){
	return newbs;           // Failed: return NULL ID (should be old ID?)
    }
    if (bs.id){
	// Copy old contents to new pixmap
	XCopyArea(gdev.xdpy,
		  (XID)bs.id, (XID)newbs.id, // Source, Destination
		  gdev.pxgc,
		  0, 0,		// Source position
		  bs.width, bs.height, // Source size
		  0, 0);	// Destination position
	XFreePixmap(gdev.xdpy, (Pixmap)bs.id);
    }
#else
    newbs.id = aip_allocateBackupPixmap(bs.id, width, height);
#endif
    newbs.datastx = 0;
    newbs.datasty = 0;
    newbs.datawd = 0;
    newbs.dataht = 0;
    newbs.interpolation = 0;
    return newbs;
}

bool
GraphicsWin::allocateCanvasBacking()
{
    canvasBackup = GraphicsWin::allocateBackingStore(canvasBackup,
						     getWinWidth(),
						     getWinHeight());
    return canvasBackup.id != 0;
}

XID_t
GraphicsWin::getCanvasBackingId()
{
    return (XID_t)canvasBackup.id;
}

void
GraphicsWin::freeBackingStore(BackStore_t bs)
{
#if defined (MOTIF) && (ORIG)
    if (bs.id) {
	XFreePixmap(gdev.xdpy, (XID)bs.id);
    }
#else
    aip_freeBackupPixmap(bs.id);
#endif
}

XID_t
GraphicsWin::setDrawable(XID_t id)
{
    XID_t oldid;
#if defined (MOTIF) && (ORIG)
    oldid = (XID_t)gdev.xid;
    if (id) {
	gdev.xid = (XID)id;
    } else {
	gdev.xid = canvas;
    }
#else
    oldid = aip_setDrawable(id);
#endif
    return oldid;
}

bool
GraphicsWin::copyImage(XID_t src, XID_t dst,
		       int src_x, int src_y,
		       int src_wd, int src_ht,
		       int dst_x, int dst_y)
{
    if (src == 0) {
	fprintf(stderr,"GraphicsWin::copyImage(): source image is null ptr\n");
	return false;
    }
    if (src_wd <= 0 || src_ht <= 0) {
	return false;
    }
#if defined (MOTIF) && (ORIG)
    GC gc = gdev.pxgc;
    if (dst == NULL) {
	dst = canvas;
	gc = gdev.xgc;
	if (canvasPixmap && getBackingStoreFlag()) {
	    XCopyArea(gdev.xdpy, (XID)src, canvasPixmap,
		      gdev.pxgc,
		      src_x, src_y, src_wd, src_ht, // Source position and size
		      dst_x, dst_y); // Destination position
	}
        if (!okToDrawOnCanvas()) {
            return true;
        }
	dst_x += viewport.x;
	dst_y += viewport.y;
    }
    XCopyArea(gdev.xdpy, (XID)src, (XID)dst,
	      gc,
	      src_x, src_y, src_wd, src_ht,	// Source position and size
	      dst_x, dst_y);			// Destination position
#else
    aip_copyImage(src, dst, src_x, src_y, src_wd, src_ht, dst_x, dst_y);
#endif
    return true;
}

Pixmap
aipDisplayImage(float *data, int width, int height)
{
    VsInfo vsfunc;
    vsfunc.minData = 0;
    vsfunc.maxData = 0.045;
    vsfunc.uFlowColor = palettes[GRAYSCALE_COLORS].firstColor;
    vsfunc.oFlowColor = (vsfunc.uFlowColor +
			 palettes[GRAYSCALE_COLORS].numColors - 1);
    return aipDisplayImage(GRAYSCALE_COLORS,
			   data,
			   width, height,
			   0, 0,
			   width, height,
			   viewport.x, viewport.y,
			   viewport.width, viewport.height,
			   ORIENT_BOTTOM,
			   spVsInfo_t(&vsfunc),
			   INTERP_REPLICATION,
			   false);
}

Pixmap
aipDisplayImage(colormapSegment_t cmsindex, // index of palette to use
		float *data, 		// pointer to data points
                int datasetWidth,    // data points per scan line
		int datasetHeight,		// # of scan lines in data
                int srcStx,              // x offset into source data
                int srcSty,              // y offset into source data
                int srcWd,          // width in data set, must be
                                        // <= datasetWidth
                int srcHt,         // height to draw in data set
		int dest_x, 		// x destination on canvas
		int dest_y,		// y destination on canvas
		int pixWd,		// width of image in pixels
		int pixHt,		// height of image in pixels
                dataOrientation_t direction,  // data order
		spVsInfo_t vsfunc,		// vertical scaling of data
		interpolation_t smooth,		// Type of interpolation
		bool keep_pixmap)	// If true, try to return pixmap
{
    int i;
    char        *pix_data = 0;
    float	*new_data ;
#if defined (MOTIF) && (ORIG)
    int		prev_op;	// X-lib op to store while drawing
    int         depth;
    XImage 	*ximage;
    Visual      *vis;
    XWindowAttributes  win_attr;

    // Do some checking on function inputs
    if (gdev.xdpy == NULL) {
	STDERR("display_image: Gdev has not been initialized");
	return 0;
    }
#endif

    if (data == (float *) NULL) {
	STDERR("aipDisplayImage: passed NULL data pointer");
	return 0;
    }

    // check the direction value
    if ((direction == ORIENT_LEFT) || (direction == ORIENT_RIGHT)) {
	STDERR("aipDisplayImage: LEFT or RIGHT orientation not supported");
	return 0;
    }

#if defined (MOTIF) && (ORIG)
    // General XWindows info
    depth = DefaultDepth(gdev.xdpy, DefaultScreen(gdev.xdpy));
    vis = DefaultVisual(gdev.xdpy, DefaultScreen(gdev.xdpy));
    if (XGetWindowAttributes(gdev.xdpy,  gdev.xid, &win_attr))
    {
        vis = win_attr.visual;
        depth = win_attr.depth;
    }
    // create the X-Windows image
    // NB: pix_data is not used until ximage is "put" somewhere.
    //     This just sets up the image structure, which includes a
    //     pointer to the data.  Later, we will set the pointer and
    //     initialize the stuff it points to.
    ximage = XCreateImage(gdev.xdpy, vis, depth, ZPixmap, 0,
			  pix_data, pixWd, pixHt, 8, 0);
    if (ximage == NULL) {
	ximage = XCreateImage(gdev.xdpy, vis, depth, XYPixmap, 0,
			      pix_data, pixWd, pixHt, 8, 0);
    }
    if (ximage == NULL) {
	STDERR("aipDisplayImage: XCreateImage() returned NULL pointer");
	delete[] pix_data;
	return 0;
    }

    // Info about image/pixmap format
    bytesPerPixel = ximage->bits_per_pixel / 8;
    pixFormat = -1;		// Put each data bit in a separate plane
    if (ximage->format == ZPixmap || ximage->format == XYPixmap) {
	pixFormat = 0;		// Use inefficient, generic processing
	if (ximage->bitmap_bit_order == MSBFirst || 
		ximage->bitmap_bit_order == LSBFirst) {
	    pixFormat = bytesPerPixel;
	    if (ximage->byte_order == LSBFirst)
		pixFormat += 4;
        }
    }
    //fprintf(stderr,"pixFormat=%d %d\n", pixFormat, ximage->format);/*CMP*/
    if (pixFormat < 1 || pixFormat > 8) {
	STDERR("aipDisplayImage: this pixmap format is not supported");
	return 0;
    }
    // Get space for pixel data
    pix_data = new char[pixWd * pixHt * bytesPerPixel];
    if (pix_data == NULL) {
	STDERR("aipDisplayImage: allocate memory returned NULL pointer");
	return 0;
    }
    ximage->data = pix_data;	// Let our ximage know about it
    raw_flag = false;
#else
    bytesPerPixel = 1;
    pixFormat = 1;
    if (img_data != NULL) {
	if (img_size < (pixWd * pixHt)) {
	    delete[] img_data;
	    img_data = NULL;
        }
    }
    if (img_data == NULL) {
	img_size = pixWd * pixHt;
	img_data = new char[img_size];
    }
    if (img_data == NULL) {
	STDERR("aipDisplayImage: allocate memory returned NULL pointer");
	img_size = 0;
	return 0;
    }
    pix_data = img_data;
    raw_flag = true;
#endif

    // Note about "if (smooth)...else"  below:
    // Of course if smooth is set we want to smooth the image.
    // convert_float_data() expands or compresses the data as
    // necessary and then uses the colormap to produce pixel
    // data.  Thus all that was here originally was what is
    // in the else part - a call to line_fill_init() and a call
    // to convert_float_data().  If we want to smooth, however,
    // we need to separate the two steps - the smoothing is
    // meaningless both before the data is expanded, and after
    // the data is transformed into pixel data - it must happen
    // in between these two steps, and convert_float_data() does
    // both at once.  Therefore to smooth an image, we:
    // - Allocate space to hold the data while smoothing it - after
    //   expanding it and before mapping it to pixel data.  This
    //   is new_data.
    // - Run the data through g_float_to_float(), which performs
    //   the expansion/compression.
    // - Smooth it top to bottom and then left to right.
    // - Proceed as if we were not smoothing - call line_fill_init()
    //   and then convert_float_data(), giving it new_data, which
    //   of course has already been expanded but this won't affect
    //   convert_float_data().
    // - Free new_data as it is no longer needed.

    vsdat.mindata = vsfunc->minData;
    vsdat.maxdata = vsfunc->maxData;
    vsdat.uflowcmi = vsfunc->uFlowColor;
    vsdat.oflowcmi = vsfunc->oFlowColor;
    vsdat.nancmi = vsfunc->uFlowColor; // For now, anyway
    if (vsdat.maxdata < vsdat.mindata) {
        vsdat.mindata = vsfunc->maxData;
        vsdat.maxdata = vsfunc->minData;
    }
    if (vsfunc->table == NULL) {
	vsfunc->size = palettes[cmsindex].numColors;
	vsfunc->table = new u_char[vsfunc->size];
	for (i=0; i<vsfunc->size; i++) {
	    vsfunc->table[i] = palettes[cmsindex].firstColor + i;
	}
    }
    vsdat.lookuptable = vsfunc->table;
    vsdat.tablesize = vsfunc->size;
    if (vsdat.maxdata == vsdat.mindata)
       vsdat.scale = vsdat.tablesize / 2.0;
    else
       vsdat.scale = vsdat.tablesize / (vsdat.maxdata - vsdat.mindata);
#if defined (MOTIF) && (ORIG)
    // Remember the X colormap
    x_color = gdev.cmap;
#endif

    if (smooth == INTERP_LINEAR || smooth == INTERP_CUBIC_SPLINE) {
	new_data = new float[pixWd * pixHt];
	
	// Cubic spline interpolation
	// First use g_float_to_float() to extract the imaged region of
	// the data w/o expansion or compression.
	float *xdata = new float[srcWd * srcHt];

	extract_rect_from_data(data, xdata,
			       datasetWidth, datasetHeight,
			       srcStx, srcSty,
			       srcWd, srcHt);
	
	// Now put that data into the picture-sized buffer.
	aipImageInterpolation(smooth,
			      xdata, srcWd, srcHt,
			      new_data, pixWd, pixHt);
	delete[] xdata;
	
	// initialize the line-fill routine
	line_fill_init (pixWd, pixWd);
	
	// scale and convert data to pixel data
	convert_float_data(new_data, pix_data, pixWd, pixWd,
			   0, 0, pixWd, pixHt,
			   pixWd, pixHt, direction);
	delete[] new_data;
    } else /* Do pixel replication */ {
	// initialize the line-fill routine
	line_fill_init (pixWd, srcWd);
	
	// scale and convert data to pixel data
	convert_float_data(data, pix_data, datasetWidth, datasetHeight,
			   srcStx, srcSty, srcWd, srcHt,
			   pixWd, pixHt, direction);
    }

    Pixmap pixmap = 0;
#if defined (MOTIF) && (ORIG)
    // store the current type of draw operation
    prev_op = G_Get_Op(gdev);

    // set the type of draw operation
    G_Set_Op(gdev, GXcopy);

    if (keep_pixmap){
	// Put the image into a pixmap
	pixmap = XCreatePixmap(gdev.xdpy, gdev.xid,
			       pixWd, pixHt,
			       depth);
	XPutImage(gdev.xdpy, pixmap, gdev.pxgc, ximage, 0, 0,
		  0, 0, pixWd, pixHt);
	
	// Copy it to the screen
        if (gdev.xid != canvas || okToDrawOnCanvas()) {
            XCopyArea(gdev.xdpy, pixmap, gdev.xid, gdev.xgc, 0, 0,
                      pixWd, pixHt,
                      viewport.x + dest_x, viewport.y + dest_y);
        }
    }else{
	// Put image directly on the screen
        if (gdev.xid != canvas || okToDrawOnCanvas()) {
            XPutImage(gdev.xdpy, gdev.xid, gdev.xgc, ximage, 0, 0,
                      viewport.x + dest_x, viewport.y + dest_y,
                      pixWd, pixHt);
        }
    }

    // restore the original X draw operation
    G_Set_Op(gdev, prev_op);

    // destroy the X-Windows image
    XDestroyImage(ximage);
    pix_data = 0;
#else /* ORIG */
    int colormapID = open_color_palette("default");
    int transparency = 0;
    pixmap = aip_displayImage(img_data, colormapID, transparency, dest_x, dest_y, pixWd, pixHt, keep_pixmap);

#endif

    return pixmap;
}

Pixmap
aipDisplayImage(float *data,	// pointer to data array
		int dest_x,	// x destination on canvas
		int dest_y,	// y destination on canvas
		int pixWd,	// width of image in pixels
		int pixHt,	// height of image in pixels
		colormapSegment_t cmsindex, // index of palette to use
		spVsInfo_t vsfunc, // intensity scaling of data
		int colormapID,
		int transparency,
		bool keep_pixmap) // If true, try to return pixmap
{
    int i;
    char 	*pix_data = 0;
#if defined (MOTIF) && (ORIG)
    int		prev_op;	// X-lib op to store while drawing
    int         depth;
    float	*new_data ;
    XImage 	*ximage;
    Visual      *vis;
    XWindowAttributes  win_attr;

    // Do some checking on function inputs
    if (gdev.xdpy == NULL) {
	STDERR("display_image: Gdev has not been initialized");
	return 0;
    }
#endif

    if (data == (float *) NULL) {
	STDERR("aipDisplayImage: passed NULL data pointer");
	return 0;
    }

#if defined (MOTIF) && (ORIG)
    // General XWindows info
    depth = DefaultDepth(gdev.xdpy, DefaultScreen(gdev.xdpy));
    vis = DefaultVisual(gdev.xdpy, DefaultScreen(gdev.xdpy));
    if (XGetWindowAttributes(gdev.xdpy,  gdev.xid, &win_attr))
    {
        vis = win_attr.visual;
        depth = win_attr.depth;
    }
    // create the X-Windows image
    // NB: pix_data is not used until ximage is "put" somewhere.
    //     This just sets up the image structure, which includes a
    //     pointer to the data.  Later, we will set the pointer and
    //     initialize the stuff it points to.
    ximage = XCreateImage(gdev.xdpy, vis, depth, ZPixmap, 0,
			  pix_data, pixWd, pixHt, 8, 0);
    if (ximage == NULL) {
	ximage = XCreateImage(gdev.xdpy, vis, depth, XYPixmap, 0,
			      pix_data, pixWd, pixHt, 8, 0);
    }
    if (ximage == NULL) {
	STDERR("aipDisplayImage: XCreateImage() returned NULL pointer");
	delete[] pix_data;
	return 0;
    }

    // Info about image/pixmap format
    bytesPerPixel = ximage->bits_per_pixel / 8;
    pixFormat = -1;		// Put each data bit in a separate plane
    if (ximage->format == ZPixmap || ximage->format == XYPixmap) {
	pixFormat = 0;		// Use inefficient, generic processing
	if (ximage->bitmap_bit_order == MSBFirst ||
		ximage->bitmap_bit_order == LSBFirst) {
	    pixFormat = bytesPerPixel;
	    if (ximage->byte_order == LSBFirst)
		pixFormat += 4;
        }
    }
    //fprintf(stderr,"pixFormat= %d %d %d %d\n", ZPixmap, XYPixmap, pixFormat, ximage->format);/*CMP*/
    if (pixFormat < 1 || pixFormat > 8) {
	STDERR("aipDisplayImage: this pixmap format is not supported");
	return 0;
    }

    // Get space for pixel data
    pix_data = new char[pixWd * pixHt * bytesPerPixel];
    if (pix_data == NULL) {
	STDERR("aipDisplayImage: allocate memory returned NULL pointer");
	return 0;
    }
    ximage->data = pix_data;	// Let our ximage know about it
    raw_flag = false;
#else
    bytesPerPixel = 1;
    pixFormat = 1;
    if (img_data != NULL) {
        if (img_size < (pixWd * pixHt)) {
            delete[] img_data;
            img_data = NULL;
        }
    }
    if (img_data == NULL) {
        img_size = pixWd * pixHt;
        img_data = new char[img_size];
    }
    if (img_data == NULL) {
        STDERR("aipDisplayImage: allocate memory returned NULL pointer");
        img_size = 0;
        return 0;
    }
    pix_data = img_data;
    raw_flag = true;
#endif

    vsdat.mindata = vsfunc->minData;
    vsdat.maxdata = vsfunc->maxData;
    vsdat.uflowcmi = vsfunc->uFlowColor;
    vsdat.oflowcmi = vsfunc->oFlowColor;
    vsdat.nancmi = vsfunc->uFlowColor; // For now, anyway
    if (vsdat.maxdata < vsdat.mindata) {
        vsdat.mindata = vsfunc->maxData;
        vsdat.maxdata = vsfunc->minData;
    }
    if (vsfunc->table == NULL) {
        if (colormapID >= 0)
	   vsfunc->size = palettes[colormapID].numColors;
        else
	   vsfunc->size = palettes[cmsindex].numColors;
	vsfunc->table = new u_char[vsfunc->size];
	for (i=0; i<vsfunc->size; i++) {
	    vsfunc->table[i] = palettes[cmsindex].firstColor + i;
	}
    }
    vsdat.lookuptable = vsfunc->table;
    vsdat.tablesize = vsfunc->size;
    if (vsdat.maxdata == vsdat.mindata)
       vsdat.scale = vsdat.tablesize / 2.0;
    else
       vsdat.scale = vsdat.tablesize / (vsdat.maxdata - vsdat.mindata);
#if defined (MOTIF) && (ORIG)
    // Remember the X colormap
    x_color = gdev.cmap;
#endif

    // convert data to pixel data
    convert_float_data(data, pix_data, pixWd, pixHt);

    Pixmap pixmap = 0;

#if defined (MOTIF) && (ORIG)
    // store the current type of draw operation
    prev_op = G_Get_Op(gdev);
    // set the type of draw operation
    G_Set_Op(gdev, GXcopy);
    if (keep_pixmap) {
	// Put the image into a pixmap
	pixmap = XCreatePixmap(gdev.xdpy, gdev.xid,
			       pixWd, pixHt,
			       depth);
	XPutImage(gdev.xdpy, pixmap, gdev.pxgc, ximage, 0, 0,
		  0, 0, pixWd, pixHt);
	
	// Copy it to the screen
        if (gdev.xid != canvas || okToDrawOnCanvas()) {
            XCopyArea(gdev.xdpy, pixmap, gdev.xid, gdev.xgc, 0, 0,
                      pixWd, pixHt,
                      viewport.x + dest_x, viewport.y + dest_y);
        }
    } else {
	// Put image directly on the screen
        if (gdev.xid != canvas || okToDrawOnCanvas()) {
            XPutImage(gdev.xdpy, gdev.xid, gdev.xgc, ximage, 0, 0,
                      viewport.x + dest_x, viewport.y + dest_y,
                      pixWd, pixHt);
        }
    }
    // restore the original X draw operation
    G_Set_Op(gdev, prev_op);

    // destroy the X-Windows image
    XDestroyImage(ximage);
    pix_data = 0;
#else
    pixmap = aip_displayImage(img_data, colormapID, transparency, dest_x, dest_y, pixWd, pixHt, keep_pixmap);
#endif

    return pixmap;
}

void
aipPrintImage(float *data, 		// pointer to data points
	      int datasetWidth,    // data points per scan line
	      int datasetHeight,		// # of scan lines in data
	      int srcStx,              // x offset into source data
	      int srcSty,              // y offset into source data
	      int srcWd,          // width in data set, must be
				// <= datasetWidth
	      int srcHt,         // height to draw in data set
	      int pixWd,		// width of image in pixels
	      int pixHt,		// height of image in pixels
	      spVsInfo_t vsfunc,		// vertical scaling of data
	      interpolation_t smooth)		// Type of interpolation
{
    char *pix_data = 0;
    float *new_data ;
    int fd;
    const char *imgFilePath = "/tmp/rawImgDataForPrinter";
    const char *dicomFilePath = "/tmp/dicomDataForPrinter";
    dataOrientation_t direction = ORIENT_TOP;

    // Do some checking on function inputs
    if (data == (float *) NULL) {
	STDERR("aipDisplayImage: passed NULL data pointer");
	return;
    }

    // Get space for pixel data
    pix_data = new char[pixWd * pixHt * bytesPerPixel];
    if (pix_data == NULL) {
	STDERR("aipPrintImage: allocate memory returned NULL pointer");
	return;
    }

    // Note about "if (smooth)...else"  below:
    // Of course if smooth is set we want to smooth the image.
    // convert_float_data() expands or compresses the data as
    // necessary and then uses the colormap to produce pixel
    // data.  Thus all that was here originally was what is
    // in the else part - a call to line_fill_init() and a call
    // to convert_float_data().  If we want to smooth, however,
    // we need to separate the two steps - the smoothing is
    // meaningless both before the data is expanded, and after
    // the data is transformed into pixel data - it must happen
    // in between these two steps, and convert_float_data() does
    // both at once.  Therefore to smooth an image, we:
    // - Allocate space to hold the data while smoothing it - after
    //   expanding it and before mapping it to pixel data.  This
    //   is new_data.
    // - Run the data through g_float_to_float(), which performs
    //   the expansion/compression.
    // - Smooth it top to bottom and then left to right.
    // - Proceed as if we were not smoothing - call line_fill_init()
    //   and then convert_float_data(), giving it new_data, which
    //   of course has already been expanded but this won't affect
    //   convert_float_data().
    // - Free new_data as it is no longer needed.

    vsdat.mindata = vsfunc->minData;
    vsdat.maxdata = vsfunc->maxData;
    vsdat.uflowcmi = vsfunc->uFlowColor;
    vsdat.oflowcmi = vsfunc->oFlowColor;
    vsdat.nancmi = vsfunc->uFlowColor; // For now, anyway
    if (vsfunc->table == NULL) {
    	delete[] pix_data;
    	pix_data = 0;
	return;
    }
    if (vsdat.maxdata < vsdat.mindata) {
        vsdat.mindata = vsfunc->maxData;
        vsdat.maxdata = vsfunc->minData;
    }
    vsdat.lookuptable = vsfunc->table;
    vsdat.tablesize = vsfunc->size;
    if (vsdat.maxdata == vsdat.mindata)
       vsdat.scale = vsdat.tablesize / 2.0;
    else
       vsdat.scale = vsdat.tablesize / (vsdat.maxdata - vsdat.mindata);
#if defined (MOTIF) && (ORIG)
    // Remember the X colormap
    x_color = gdev.cmap;
#endif

    raw_flag = true;
    if (smooth == INTERP_LINEAR || smooth == INTERP_CUBIC_SPLINE) {
	new_data = new float[pixWd * pixHt];
	
	// Cubic spline interpolation
	// First use g_float_to_float() to extract the imaged region of
	// the data w/o expansion or compression.
	float *xdata = new float[srcWd * srcHt];

	extract_rect_from_data(data, xdata,
			       datasetWidth, datasetHeight,
			       srcStx, srcSty,
			       srcWd, srcHt);
	
	// Now put that data into the picture-sized buffer.
	aipImageInterpolation(smooth,
			      xdata, srcWd, srcHt,
			      new_data, pixWd, pixHt);
	delete[] xdata;
	
	// initialize the line-fill routine
	line_fill_init (pixWd, pixWd);
	
	// scale and convert data to pixel data
	convert_float_data(new_data, pix_data, pixWd, pixWd,
			   0, 0, pixWd, pixHt,
			   pixWd, pixHt, direction);
	delete[] new_data;
    } else /* Do pixel replication */ {
	// initialize the line-fill routine
	line_fill_init (pixWd, srcWd);
	
	// scale and convert data to pixel data
	convert_float_data(data, pix_data, datasetWidth, datasetHeight,
			   srcStx, srcSty, srcWd, srcHt,
			   pixWd, pixHt, direction);
    }

    // Write image to file and call printing program
    fd = open(imgFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (!fd) {
	fprintf(stderr,"Cannot open file for image printing: %s\n",
		imgFilePath);
    } else {
	char cmdBuf[1024];
	char *endptr;
	long port;
	int err;

	// Write raw data
	write(fd, pix_data, pixWd * pixHt);
	close(fd);

	// Convert to DICOM format
	fprintf(stderr,"PlotterHost=\"%s\", LplotterPort=\"%s\"\n",
		PlotterHost, LplotterPort);/*CMP*/
	sprintf(cmdBuf,"createdicom -data %s -width %d -height %d -outfile %s",
		imgFilePath, pixWd, pixHt, dicomFilePath);
	fprintf(stderr,"%s\n", cmdBuf);/*CMP*/
	unlink(dicomFilePath);	// Don't be confused by any old file
	err = system(cmdBuf);
	fprintf(stderr,"rtn val = %d\n", err);/*CMP*/

	// Send DICOM file to printer
	port = strtol(LplotterPort, &endptr, 10);
	if (endptr == LplotterPort) {
	    port = 104;		// Default value if conversion fails
	}
	sprintf(cmdBuf,"dicomlpr -host %s -port %d -file %s",
		PlotterHost, (int)port, dicomFilePath);
	fprintf(stderr,"%s\n", cmdBuf);/*CMP*/
	err = system(cmdBuf);
	fprintf(stderr,"rtn val = %d\n", err);/*CMP*/
	//unlink(imgFilePath);
	//unlink(dicomFilePath);
    }

    delete[] pix_data;
    pix_data = 0;

    return;
}			// end of aipDisplayImage()

Pixmap
aipDisplayVnmrImage(float *data, 	    // pointer to data
		    int datasetWidth,    // data points per scan line
		    int datasetHeight, 	    // # of scan lines in data
		    int srcStx,              // x offset into source data
		    int srcSty,              // y offset into source data
		    int srcWd,          // width in data set, must be
		    			    // <= datasetWidth
		    int srcHt,         // height to draw in data set
		    int dest_x, 	    // x destination on canvas
		    int dest_y,		    // y destination on canvas
		    int pixWd,	    // width of image in pixels
		    int pixHt,	    // height of image in pixels
		    double datamax,	    // for vertical scaling of data
		    colormapSegment_t palette,
		    int smoothing)
{
    Pixmap pm;

    spVsInfo_t vsfunc = spVsInfo_t(new VsInfo());
    vsfunc->minData = 0;
    vsfunc->maxData = datamax;
    vsfunc->uFlowColor = palettes[GRAYSCALE_COLORS].firstColor;
    vsfunc->oFlowColor = (vsfunc->uFlowColor +
			 palettes[GRAYSCALE_COLORS].numColors - 1);

    interpolation_t interp;
    switch (smoothing) {
      default: interp = INTERP_REPLICATION; break;
      case 2: interp = INTERP_LINEAR; break;
      case 3: interp = INTERP_CUBIC_SPLINE; break;
    }
    
    pm = aipDisplayImage(palette,
			 data,
			 datasetWidth, datasetHeight,
			 srcStx, srcSty,
			 srcWd, srcHt,
			 dest_x, dest_y,
			 pixWd, pixHt,
			 ORIENT_BOTTOM,
			 vsfunc,
			 interp,
			 true);
    return pm;
}

/*
 * This function extracts a sub-rectangle from a rectangular set
 * of image data.
 */
static void
extract_rect_from_data(float *indata,
		       float *outdata,
		       int datasetWidth, // Width of input data
		       int datasetHeight, // Height of input data
		       int x_offset, // Position of extracted  rectangle
		       int y_offset,
		       int width, // Size of extracted rectangle
		       int height)
{
    int i;
    int ix = x_offset >= 0 ? x_offset : 0;
    int iy = y_offset >= 0 ? y_offset : 0;
    int xlimit = x_offset + width;
    if (xlimit > datasetWidth) { xlimit = datasetWidth; }
    int ylimit = y_offset + height;
    if (ylimit > datasetHeight) { ylimit = datasetHeight; }
    float *pdata;
    for ( ; iy<ylimit; iy++) {
	pdata = indata + iy * datasetWidth + ix;
	i = ix;
	for ( ; i<xlimit; i++) {
	    *outdata++ = *pdata++;
	}
    }
}

/****************************************************************************
 * line_same
 *
 * This function fits the number of data points into a same-size image line.
 *
 * INPUT ARGS:
 *   indata      A pointer to the start of the data points to be loaded into
 *               the image line.
 *   outdata     A pointer to the start of the image line that gets the data.
 * OUTPUT ARGS:
 *   none
 * RETURN VALUE:
 *   none
 * GLOBALS USED:
 *   line_size   The number of image points in an image line.
 *   x_color     Pointer to the X colormap
 * GLOBALS CHANGED:
 *   none
 * ERRORS:
 *   none
 * EXIT VALUES:
 *   none
 * NOTES:
 *   This function is called via the "LineFill" global variable.
 *
 * David Woodworth
 * Spectroscopy Imaging Systems Corporation
 * Fremont, California
 */
static void
line_same (float *indata, char *outdata)
{
    // LOCAL VARIABLES:
    // 
    // p_in     A fast pointer to the data being loaded into the image line.
    // p_out    A fast pointer to the start of the image line.
    // cmi      The colormap index value.
    // i        A counter for the length of the image line.
    //
    float *p_in  = indata;
    float val, vscale;
    char *p_out = outdata;
    int  cmi;
    int    i;
    uchar_t *lktable;

    lktable = vsdat.lookuptable;
    if (raw_flag) {
       vscale = vsdat.scale;
       for (i = line_size; i > 0; --i, ++p_in) {
	   // Test a data value against limits and convert to a
	   // colormap entry.
           val = *p_in;
	   if (val <= vsdat.mindata) {
	       *p_out = vsdat.uflowcmi;
	   } else if (val >= vsdat.maxdata) {
	       *p_out = vsdat.oflowcmi;
	   } else if (!(val < vsdat.maxdata)) {
	       *p_out = vsdat.nancmi;
	   } else {
               cmi = (int)((val - vsdat.mindata) * vscale);
	       *p_out = lktable[cmi];
	   }
       }
       return;
    } else {
       for (i = line_size; i > 0; --i, ++p_in) {
	   // Test a data value against limits and convert to a
	   // colormap entry.
	   if (*p_in <= vsdat.mindata){
	       cmi = vsdat.uflowcmi;
	   }else if (*p_in >= vsdat.maxdata){
	       cmi = vsdat.oflowcmi;
	   } else if (!(*p_in < vsdat.maxdata)) {
	       cmi = vsdat.nancmi;
	   }else{
	       cmi = lktable[(int)((*p_in - vsdat.mindata) * vsdat.scale)];
	   }
	   setImagePixel(&p_out, x_color[cmi]);
	   //*p_out = (char)x_color[cmi];
       }
    }
}  // end of function "line_same"

/****************************************************************************
 *  line_expand
 * 
 * This function fits the number of data points into a LONGER image line.
 * This function converts floating point values into colormap indexes
 * from the selected palette. It maps the data values onto the pixels
 * using expansion. It duplicates values from the group being expanded
 * into the scan line of pixels.
 * 
 *  INPUT ARGS:
 *    indata      A pointer to the start of the data points to be loaded into
 *                the image line.
 *    outdata     A pointer to the start of the image line that gets the data.
 *  OUTPUT ARGS:
 *    none
 *  RETURN VALUE:
 *    none
 *  GLOBALS USED:
 *    line_size   The number of image points in an image line.
 *    D1, D2      Decision-variable deltas used for expanding/compressing the
 *                data points to fill out an image line (see Foley & van Dam).
 *    x_color     Pointer to the X colormap
 *  GLOBALS CHANGED:
 *    none
 *  ERRORS:
 *    none
 *  EXIT VALUES:
 *    none
 *  NOTES:
 *    This function is called via the "LineFill" global variable.
 *    This function uses Bresenham's decision-variable algorithm; for
 *    a formal development of the method, see Section 11.2.2,
 *    "Bresenham's Line Algorithm", in Foley & van Dam, "Fundamentals
 *    of Interactive Computer Graphics" (1981).
 *
 *  David Woodworth
 *  Spectroscopy Imaging Systems Corporation
 *  Fremont, California
 */
static void
line_expand (float *indata, char *outdata)
{
    // LOCAL VARIABLES:
    // 
    // p_in     A fast pointer to the data being loaded into the image line.
    // p_out    A fast pointer to the start of the image line.
    // dv       The decision variable (see Foley & van Dam).
    // cmi      The colormap index.
    // i        A counter for the length of the image line.

    float *p_in  = indata;
    char *p_out = outdata;
    int    dv;
    int cmi;
    int    i;
    uchar_t *lktable;

    // check the imput and output buffer pointers
    if (indata == (float *) NULL)
    {
	STDERR("line_expand: passed NULL indata pointer");
	return;
    }

    if (outdata == (char *) NULL)
    {
	STDERR("line_expand: passed NULL outdata pointer");
	return;
    }

    // set the starting value of the decision variable
    dv = D2;
    lktable = vsdat.lookuptable;

    // Test first data value against limits and convert to an
    // initial colormap entry.
    if (*p_in <= vsdat.mindata){
	cmi = vsdat.uflowcmi;
    }else if (*p_in >= vsdat.maxdata){
	cmi = vsdat.oflowcmi;
    } else if (!(*p_in < vsdat.maxdata)) {
	cmi = vsdat.nancmi;
    }else{
	cmi = lktable[(int)((*p_in - vsdat.mindata) * vsdat.scale)];
    }

    if (raw_flag)
    {
	// Load the first data value into the output buffer
	*p_out++ =  (char)cmi;
	for (i = line_size-1; i > 0; --i, ++p_out)
	{
	    // Adjust the decision variable
	    if (dv < 0){
		dv += D1;
	    }else{
		dv += D2;
		
		// Move to the next input point
		++p_in;
		
		// Test a data value against limits and convert to a
		// colormap entry.
		if (*p_in <= vsdat.mindata){
		    cmi = vsdat.uflowcmi;
		}else if (*p_in >= vsdat.maxdata){
		    cmi = vsdat.oflowcmi;
		} else if (!(*p_in < vsdat.maxdata)) {
		    cmi = vsdat.nancmi;
		}else{
		    cmi =
		      lktable[(int)((*p_in - vsdat.mindata) * vsdat.scale)];
		}
	    }
	    // Load the data value into the output buffer
	    *p_out =  (char)cmi;
	}
    }
    else
    {
	// Load the first data value into the output buffer
	setImagePixel(&p_out, x_color[cmi]);
	for (i = line_size-1; i > 0; --i)
	{
	    // Adjust the decision variable
	    if (dv < 0){
		dv += D1;
	    }else{
		dv += D2;
		
		// move to the next input point
		++p_in;
		
		// Test a data value against limits and convert to a
		// colormap entry.
		if (*p_in <= vsdat.mindata){
		    cmi = vsdat.uflowcmi;
		}else if (*p_in >= vsdat.maxdata){
		    cmi = vsdat.oflowcmi;
		} else if (!(*p_in < vsdat.maxdata)) {
		    cmi = vsdat.nancmi;
		}else{
		    cmi =
		        lktable[(int)((*p_in - vsdat.mindata)
					    * vsdat.scale)];
		}
	    }
	    // Load the data value into the output buffer
	    setImagePixel(&p_out, x_color[cmi]);
	    //*p_out =  (char)x_color[cmi];
	}
    }
}  // end of function "line_expand"

/****************************************************************************
 * line_compress
 *
 * This function fits the number of data points into a SHORTER image line.
 * This function converts floating point values into colormap indexes
 * from the selected palette. It maps the data values onto the pixels
 * using compression. It uses the maximum data value from the group
 * being compressed to convert into the colormap index.
 *
 * INPUT ARGS:
 *   indata      A pointer to the start of the data points to be loaded into
 *               the image line.
 *   outdata     A pointer to the start of the image line that gets the data.
 * OUTPUT ARGS:
 *   none
 * RETURN VALUE:
 *   none
 * GLOBALS USED:
 *   data_size   The number of data points available for an image line.
 *   line_size   The number of image points in an image line.
 *   D1, D2      Decision-variable deltas used for expanding/compressing the
 *               data points to fill out an image line (see Foley & van Dam).
 *   x_color     Pointer to the X colormap
 * GLOBALS CHANGED:
 *   none
 * ERRORS:
 *   none
 * EXIT VALUES:
 *   none
 * NOTES:
 *   This function is called via the "LineFill" global variable.
 *   This function uses Bresenham's decision-variable algorithm; for a
 *   formal development of the method, see Section 11.2.2,
 *   "Bresenham's Line Algorithm", in Foley & van Dam, "Fundamentals
 *   of Interactive Computer Graphics" (1981).
 * BUGS:
 *   Displays the maximum value of the data points binned into a pixel;
 *   does not do any averaging or division of signal between pixels.
 *
 *   Only works on POSITIVE DATA.
 *
 * David Woodworth
 * Spectroscopy Imaging Systems Corporation
 * Fremont, California
 */
static void
line_compress (float *indata, char *outdata)
{
    // LOCAL VARIABLES:
    //
    // p_in     A fast pointer to the data being loaded into the image line.
    // p_out    A fast pointer to the start of the image line.
    // dv       The decision variable (see Foley & van Dam).
    // cmi      The colormap index value.
    // i        A counter for the number of data points to load.
    // max_val  The maximum value of an input data point, which causes the
    //         maximum of sequential deleted data points to be loaded.

    float *p_in  = indata;
    char *p_out = outdata;
    int    dv;
    int    i;
    float max_val;
    int cmi;
#if defined(SOLARIS) || defined(__INTERIX)
    float inf = HUGE_VAL;
#else 
    float inf = HUGE;
#endif 

    max_val = -inf;

    // set the starting value of the decision variable
    dv = D2;

    if (raw_flag)
    {
       for (i = data_size; i > 0; --i, ++p_in)
       {
	   if (*p_in > max_val)
               max_val = *p_in;

	   // adjust the decision variable
	   if (dv < 0)
               dv += D1;
	   else
	   {
               dv += D2;

    	       // Test a data value against limits and convert to a
	       // colormap entry.
	       if (max_val <= vsdat.mindata){
		   cmi = vsdat.uflowcmi;
	       }else if (max_val >= vsdat.maxdata){
		   cmi = vsdat.oflowcmi;
	       } else if (!(*p_in < vsdat.maxdata)) {
		   cmi = vsdat.nancmi;
	       }else{
		   cmi =
		   vsdat.lookuptable[(int)((max_val - vsdat.mindata)
					   * vsdat.scale)];
	       }
	       *p_out++ = (char)cmi;

               max_val = -inf;
	   }
	}
    }
    else
    {
       for (i = data_size; i > 0; --i, ++p_in)
       {
	   if (*p_in > max_val)
               max_val = *p_in;

	   // adjust the decision variable
	   if (dv < 0)
               dv += D1;
	   else
	   {
               dv += D2;

    	       // Test a data value against limits and convert to a
	       // colormap entry.
	       if (max_val <= vsdat.mindata){
		   cmi = vsdat.uflowcmi;
	       }else if (max_val >= vsdat.maxdata){
		   cmi = vsdat.oflowcmi;
	       } else if (!(*p_in < vsdat.maxdata)) {
		   cmi = vsdat.nancmi;
	       }else{
		   cmi =
		   vsdat.lookuptable[(int)((max_val - vsdat.mindata)
					   * vsdat.scale)];
	       }

               // load the data value into the output buffer
	       setImagePixel(&p_out, x_color[cmi]);
	       //*p_out++ = (char)x_color[cmi];

               max_val = -inf;
	   }
	}
    }

}  // end of function "line_compress"

static void
setImagePixel(char **ppout, u_long xcolor)
{
    switch (pixFormat) {
      case 1:			// 8 bits
      case 5:
	*((*ppout)++) = (char)xcolor;
	break;
      case 2:			// 16 bits
	*((*ppout)++) = (char)(xcolor >> 8);
	*((*ppout)++) = (char)(xcolor);
	break;	
      case 3:			// 24 bits
	*((*ppout)++) = (char)(xcolor >> 16);
	*((*ppout)++) = (char)(xcolor >> 8);
	*((*ppout)++) = (char)(xcolor);
	break;	
      case 4:			// 32 bits
	*((*ppout)++) = (char)(xcolor >> 24);
	*((*ppout)++) = (char)(xcolor >> 16);
	*((*ppout)++) = (char)(xcolor >> 8);
	*((*ppout)++) = (char)(xcolor);
	break;	
      case 6:			// 16 bits, LSB first
	*((*ppout)++) = (char)(xcolor);
	*((*ppout)++) = (char)(xcolor >> 8);
	break;	
      case 7:			// 24 bits, LSB first
	*((*ppout)++) = (char)(xcolor);
	*((*ppout)++) = (char)(xcolor >> 8);
	*((*ppout)++) = (char)(xcolor >> 16);
	break;	
      case 8:			// 32 bits, LSB first
	*((*ppout)++) = (char)(xcolor);
	*((*ppout)++) = (char)(xcolor >> 8);
	*((*ppout)++) = (char)(xcolor >> 16);
	*((*ppout)++) = (char)(xcolor >> 24);
	break;
    }	
}

/****************************************************************************
 * line_fill_init
 *
 * This function sets up the global variables internal to this module
 * that the individual line filling routines might have to use. It also
 * chooses which line filling routine gets called by convert_float_data()
 *
 * INPUT ARGS:
 *   Line_size   The number of points in an image line to fill with image data.
 *   Data_size   The number of data points available for an image line.
 * OUTPUT ARGS:
 *   none
 * RETURN VALUE:
 *   none
 * GLOBALS USED:
 *   data_size   The number of data points available for an image line.
 *   line_size   The number of image points in an image line.
 *   D1, D2      Decision-variable deltas used for expanding/compressing the
 *               data points to fill out an image line (see Foley & van Dam).
 *   x_color     Pointer to the X colormap
 *   LineFill    A pointer to the function to use for filling out image lines
 *               with data points.
 * GLOBALS CHANGED:
 *   line_size   The number of image points in an image line.
 *   data_size   The number of data points available for an image line.
 *   D1, D2      Decision-variable deltas used for expanding/compressing the
 *               data points to fill out an image line (see Foley & van Dam).
 *   LineFill    A pointer to the function to use for filling out image lines
 *               with data points.
 * ERRORS:
 *   none
 * EXIT VALUES:
 *   none
 *
 * David Woodworth
 * Spectroscopy Imaging Systems Corporation
 * Fremont, California
 */
static void
line_fill_init (int Line_size,		// pixels per line in image
		int Data_size) 		// points per line in data
{

    // set the sizes of the input and output lines
    line_size = Line_size;
    data_size = Data_size;

    if (data_size == line_size) {
	// the data points exactly fit the number of display points
	LineFill = line_same;
    } else if (data_size < line_size) {
	// expand (repeat some) data points to fill out the display points
	D1 = 2*data_size;
	D2 = D1 - 2*line_size;
	LineFill = line_expand;
    } else {
    // compress (delete some) data points to fit the display points
	D1 = 2*line_size;
	D2 = D1 - 2*data_size;
	LineFill = line_compress;
    }
}  // end of function "line_fill_init"

/****************************************************************************
 * convert_float_data
 *
 * This function builds the image pixel array. It duplicates scan lines
 * when expanding an image, and uses maximum of groups of scan lines
 * when compressing an image. It does not do any smoothing of expanded
 * image data.
 * This function converts floating-point data to n-bit pixel data, depending
 * on the number of gray-levels or false-color levels in the selected color
 * map.
 *
 * INPUT ARGS:
 *   indata	A pointer to the array of input values
 *   outdata	A pointer to the output pixel buffer.
 *   dbufWd	The number of data points in each row (trace).
 *   dbufHt	The number of rows (traces) in the data.
 *   datastx	The X Offset into the data set to start at.
 *   datasty	The Y Offset into the data set to start at.
 *   datawd	The width of the data array to use
 *   dataht	The height of the data array to use
 *   pixwd	The width of the image in pixels.
 *   pixht	The height of the image in pixels.
 * OUTPUT ARGS:
 *   none
 * RETURN VALUE:
 *   OK           Image successfully converted and loaded.
 *   NOT_OK       Error building output image.
 * GLOBALS CHANGED:
 *   none
 * ERRORS:
 *   NOT_OK       Can't read data trace: "gettrace()" returned error.
 *   NOT_OK       Can't allocate temporary memory for building image.
 * EXIT VALUES:
 *   none
 * NOTES:
 *   This function uses Bresenham's decision-variable algorithm; for a
 *   formal development of the method, see Section 11.2.2,
 *   "Bresenham's Line Algorithm", in Foley & van Dam, "Fundamentals
 *   of Interactive Computer Graphics" (1981).
 *
 * David Woodworth
 * Spectroscopy Imaging Systems Corporation
 * Fremont, California
 */
static int
convert_float_data (float *indata,
		    char *outdata,
		    int dbufWd,
		    int dbufHt,	// Not used
		    int datastx,
		    int datasty,
		    int datawd,	// Not used
		    int dataht,
		    int pixwd,
		    int pixht,
		    dataOrientation_t direction)
{
    // LOCAL VARIABLES:
    // 
    // sub_msg     The name of this function, for error messages.
    // trace       A counter for the trace read from the data.
    // max_val     A pointer to a buffer used for generating the max data values
    //             for traces that will be removed during image compression.
    // dv, d1, d2  The decision variable and deltas (see Foley & van Dam).
    // p_in	   A fast pointer to the input buffer.
    // p_out       A fast pointer to the output buffer.
    // i, j        Counters used for image filling and expansion/compression.
    // trace_ptr   Points to the trace being currently processed

    // static char   	sub_msg[] = "convert_float_data:";
    int    		trace;
    float 		*max_val;
    int    		dv, d1, d2;
    register char 	*p_out;
    int    		i;
    register float 	*trace_ptr;

    // set a fast pointer to the output buffer
    p_out = outdata;

    // Build an image from the data: there are 3 possibilities:
    // the data traces exactly fit the number of display lines
    if (dataht == pixht)
    {
	if (direction == ORIENT_TOP)
	{
	    // top down direction
      	    for (trace = datasty; trace < datasty + dataht; trace++)
      	    {
         	if ( (trace_ptr = gettrace (indata, dbufWd, datastx,
		    	trace)) == (float *) NULL)
            	    return (NOT_OK);

         	(*LineFill)(trace_ptr, p_out);
         	p_out += pixwd * bytesPerPixel;
      	    }
	}
	else if (direction == ORIENT_BOTTOM)
	// Vnmr data starts at bottom scan line and comes up
	{
	    for (trace = datasty + dataht - 1; trace >= datasty; trace--)
      	    {
         	if ( (trace_ptr = gettrace (indata, dbufWd, datastx,
		    	trace)) == (float *) NULL)
            	    return (NOT_OK);

         	(*LineFill)(trace_ptr, p_out);
         	p_out += pixwd * bytesPerPixel;
      	    }
	}
    }
    // expand (repeat some of) the data traces to fill out the display lines
    else if (dataht < pixht)
    {
	// set the adjustment values for the decision variable
	d1 = 2 * dataht;
	d2 = d1 - (2 * pixht);

	// set the starting value of the decision variable
	dv = d2;

	if (direction == ORIENT_TOP)
	{
	    // top down direction
            trace_ptr = NULL;
            for (i = 0, trace = datasty; i < pixht; i++, p_out += pixwd * bytesPerPixel)
	    {
             	// adjust the decision variable
             	if (dv < 0)
		{
                    if (trace_ptr)
                    {
                       dv += d1;
                       (void)memcpy(p_out, p_out-pixwd*bytesPerPixel, pixwd * bytesPerPixel);
                    }
                    else
                    {
                       // Only get executed once (at the most)
                       if ( (trace_ptr = gettrace (indata, dbufWd, datastx,
						   trace++)) == (float *)NULL)
                               return (NOT_OK);
                       // load the data trace into the output buffer
                       (*LineFill)(trace_ptr, p_out);
                    }
		}
             	else
             	{
                    dv += d2;

	    	    if ( (trace_ptr = gettrace (indata, dbufWd, datastx,
						trace++)) == (float *) NULL)
            	    	    return (NOT_OK);
                    // load the data trace into the output buffer
                    (*LineFill)(trace_ptr, p_out);
             	}
	    }		// end of for all image scan lines
	} else if (direction == ORIENT_BOTTOM) {
	    // Vnmr data starts at bottom scan line and comes up.
            // Starting with the last data trace, load the display lines.
	    // Preload the first data trace into the display buffer.
	    trace = datasty + dataht - 1;
	    trace_ptr = gettrace(indata, dbufWd, datastx, trace);
	    if (trace_ptr == NULL) {
		return (NOT_OK);
	    }
	    (*LineFill)(trace_ptr, p_out);
	    p_out += pixwd * bytesPerPixel;
	    dv += d1;
	    for (i = 1; i < pixht; i++, p_out += pixwd * bytesPerPixel) {
             	// adjust the decision variable
             	if (dv < 0) {
                    dv += d1;
		    // Copy current data to current display line
		    memcpy(p_out, p_out - pixwd*bytesPerPixel, pixwd * bytesPerPixel);
		} else {
                    dv += d2;
		    // Get next data line
                    --trace;
		    trace_ptr = gettrace(indata, dbufWd, datastx, trace);
		    if (trace_ptr == NULL) {
			return (NOT_OK);
		    }
                    // load the data trace into the display buffer
                    (*LineFill)(trace_ptr, p_out);
             	}
	    }
	}
    } else {
	// Compress (delete some of the data traces)
	int clearbuf = true;

      	// allocate a maximum buffer
      	max_val = new float[dbufWd];

      	// set the adjustment values for the decision variable
      	d1 = 2*pixht;
      	d2 = d1 - 2 * dataht;

      	// set the starting value of the decision variable
	dv = d2;

	if (direction == ORIENT_TOP)
	{
      	    // starting with the first data trace, load the display lines
      	    for (trace = datasty; trace < datasty + dataht; trace++)
      	    {
         	if ( (trace_ptr = gettrace (indata, dbufWd, datastx,
                	trace)) == (float *)NULL)
            	    return (NOT_OK);

         	// load this trace into the maximum buffer
		if (clearbuf){
		    for (i = 0; i < dbufWd; ++i){
			*(max_val+i) = *(trace_ptr+i);
		    }
		    clearbuf = false;
		}else{
		    for (i = 0; i < dbufWd; ++i){
			if (*(trace_ptr+i) > *(max_val+i)){
			    *(max_val+i) = *(trace_ptr+i);
			}
		    }
		}

         	// adjust the decision variable
         	if (dv < 0)
            	    dv += d1;
         	else
         	{
            	    dv += d2;

            	    // load the maximum buffer into the output buffer
            	    (*LineFill)(max_val, p_out);
            	    p_out += pixwd * bytesPerPixel;

            	    // clear the maximum buffer
            	    clearbuf = true;
         	}
	    }	// end of for all traces
	}
	else if (direction == ORIENT_BOTTOM)
	// Vnmr data starts at bottom scan line and comes up
	{
      	    // starting with the last data trace, load the display lines
	    for (trace = datasty + dataht - 1; trace >= datasty; trace--)
      	    {
         	if ( (trace_ptr = gettrace (indata, dbufWd, datastx,
					    trace)) == (float *)NULL)
            	    return (NOT_OK);

         	// load this trace into the maximum buffer
		if (clearbuf){
		    for (i = 0; i < dbufWd; ++i){
			*(max_val+i) = *(trace_ptr+i);
		    }
		    clearbuf = false;
		}else{
		    for (i = 0; i < dbufWd; ++i){
			if (*(trace_ptr+i) > *(max_val+i)){
			    *(max_val+i) = *(trace_ptr+i);
			}
		    }
		}

         	// adjust the decision variable
         	if (dv < 0)
            	    dv += d1;
         	else
         	{
            	    dv += d2;

            	    // load the maximum buffer into the output buffer
            	    (*LineFill)(max_val, p_out);
            	    p_out += pixwd * bytesPerPixel;

            	    // clear the maximum buffer
            	    clearbuf = true;
         	}
	    }		// end of for all traces
	}		// end of if display inverted data
      	delete[] max_val;
    }

    return (OK);

}

/**********************************************************************
 * convert_float_data
 *
 * This function converts floating-point data to n-bit pixel data,
 * depending on the number of gray-levels or false-color levels in the
 * selected color map.  The input and output buffers have the same
 * dimensions; expansion or compression is done.
 *
 * INPUT ARGS:
 *   indata	A pointer to the array of input values
 *   outdata	A pointer to the output pixel buffer.
 *   pixwd	The width of the image in pixels.
 *   pixht	The height of the image in pixels.
 * OUTPUT ARGS:
 *   none
 * RETURN VALUE:
 *   OK           Image successfully converted and loaded.
 *   NOT_OK       Error building output image.
 * GLOBALS CHANGED:
 *   none
 * ERRORS:
 * EXIT VALUES:
 *   none
 */
static int
convert_float_data(float *indata,
		   char *outdata,
		   int pixwd,
		   int pixht)

{
    float *p_in  = indata;
    float val, vscale;
    char *p_out = outdata;
    int bufsize = pixwd * pixht;
    float *enddata = indata + bufsize;
    uchar_t *lktable;
    int  cmi, i;

    vscale = vsdat.scale;
    lktable = vsdat.lookuptable;
    if (raw_flag) {
	for (i = bufsize; i > 0; --i, ++p_in) {
            val = *p_in;
	    if (val <= vsdat.mindata) {
		*p_out = vsdat.uflowcmi;
	    } else if (val >= vsdat.maxdata) {
		*p_out = vsdat.oflowcmi;
	    } else if (!(val < vsdat.maxdata)) {
		*p_out = vsdat.nancmi;
	    } else {
		cmi = (int)((val - vsdat.mindata) * vscale);
		*p_out = lktable[cmi];
	    }
	    p_out++;
	}
	return OK;
    } else if (pixFormat == 1) {
	for (p_in=indata; p_in<enddata; ++p_in) {
	    val = *p_in;
	    if (val <= vsdat.mindata) {
		cmi = vsdat.uflowcmi;
	    } else if (val >= vsdat.maxdata) {
		cmi = vsdat.oflowcmi;
	    } else if (!(val < vsdat.maxdata)) {
		cmi = vsdat.nancmi;
	    } else {
		cmi = lktable[(int)((val - vsdat.mindata) * vscale)];
	    }
	    *p_out++ = (char)x_color[cmi];
	}
    } else {
	for (p_in=indata; p_in<enddata; ++p_in) {
	    val = *p_in;
	    if (val <= vsdat.mindata) {
		cmi = vsdat.uflowcmi;
	    } else if (val >= vsdat.maxdata) {
		cmi = vsdat.oflowcmi;
	    } else if (!(val < vsdat.maxdata)) {
		cmi = vsdat.nancmi;
	    } else {
		cmi = lktable[(int)((val - vsdat.mindata) * vscale)];
	    }
	    setImagePixel(&p_out, x_color[cmi]);
	}
    }
    return OK;
}

// This function retrieves a pointer to a trace from the input data.
// If the datastx is more than the width, this function returns an error.
static float *
gettrace(float *start, int datawd, int datastx, int trace)
{
    if (datastx > datawd)
	return ((float *) NULL);

    return (start + trace * datawd + datastx);
}	// end of gettrace()

void
aipInitGrayscaleCMS()
{
#if defined (MOTIF) && (ORIG)
    if (gdev.xdpy == NULL || gdev.cmapFunc == NULL || palettes == NULL) {
	return;
    }
    if (gdev.cmapFunc == NULL || palettes == NULL) {
	return;
    int firstColor = palettes[GRAYSCALE_COLORS].firstColor;
    int numColors = palettes[GRAYSCALE_COLORS].numColors;
    int end = firstColor + numColors;
    for (int i=firstColor; i<end; i++) {
	int gray = (255 * (i - firstColor)) / (numColors - 1);
	gdev.cmapFunc(i, gray, gray, gray);
    }
    deleteOldFiles(sendGrayColorsToVj());
#else
    aip_initGrayscaleCMS(GRAYSCALE_COLORS);
#endif
}

/*
 * Set static variables needed to write on the display.
 */
void
aipSetGraphics(Display *xdisplay,
	       int xscreen,
	       XID xid,
	       GC vnmr_gc,
	       GC pxm_gc,
	       Pixel *sun_colors,
	       palette_t *colorPalettes,
	       PCMF func)
{
    canvas = xid;		// Remember this

    gdev.xdpy = xdisplay;	// The X display to use
    gdev.scrn = xscreen;	// Which screen
    gdev.xid = xid;		// Which window
    gdev.xgc = vnmr_gc;		// Graphics Context for drawing on screen
    gdev.pxgc = pxm_gc;		// Graphics Context for pixmap operations
    gdev.cmap = sun_colors;	// Colormap
    gdev.cmapFunc = func;
    palettes = colorPalettes;	// Specifies segments of colormap
    VsInfo::setDefaultPalette(&palettes[GRAYSCALE_COLORS]);
    VsInfo::setPaletteList(palettes);

    aipInitGrayscaleCMS();
}

void
aipSetGraphicsPalettes(palette_t *colorPalettes)
{
    palettes = colorPalettes;
    VsInfo::setPaletteList(colorPalettes);
}

/*
 * Set the boundaries of graphics area.
 * Some of this may be covered by VJ popups.
 */
void
aipSetGraphicsBoundaries(int x, int y, int width, int height)
{
    if (viewport.x == x && viewport.y == y &&
	viewport.width == width && viewport.height == height)
    {
	return;
    }
    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height = height;
    //  NB: No resize/redraw necessary
    //GframeManager::get()->resizeFrames();    // Redraw Screen
}

#if defined (MOTIF) && (ORIG)

void
aipSetCanvasMask(Region region)
{
    if (visibleMask == NULL) {
	visibleMask = XCreateRegion();
    } else {
	if (XEqualRegion(region, visibleMask)) {
	    return;		// Nothing to do
	}
    }
    XUnionRegion(region, region, visibleMask); // Update visibleMask
    if (!cDrawMask) {
	cDrawMask = XCreateRegion();
    }
    if (useActiveMask) {
	XIntersectRegion(region, cActiveMask, cDrawMask);
    } else {
	XUnionRegion(region, region, cDrawMask);
    }
    XSetRegion(gdev.xdpy, gdev.xgc, cDrawMask);
    //  NB: No resize/redraw necessary
}
#endif

void
aipFreePixmap(XID_t pm)
{
    if (pm) {
#if defined (MOTIF) && (ORIG)
	XFreePixmap(gdev.xdpy, (Pixmap)pm);
#else
        aip_freeBackupPixmap(pm);
#endif
    }
}

int
getWinWidth()
{
    return viewport.width;
}

int
getWinHeight()
{
    return viewport.height;
}
