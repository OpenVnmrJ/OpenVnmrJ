/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPGRAPHICSWIN_H
#define AIPGRAPHICSWIN_H

#ifdef MOTIF
#include <X11/Intrinsic.h>
#else
#include "vjXdef.h"
#endif


#include "aipGraphics.h"
#include "aipStructs.h"
#include "aipInterpolation.h"
#include "aipVsInfo.h"

class GraphicsWin
{
public:
    static void setCursor(const string cursType);
    static string getCursor();
    static void clearWindow();
    static void validateRect(int& x, int& y, int& w, int& h);
    static void clearRect(int x, int y, int w, int h);
    static void drawRect(int x, int y, int w, int h, int color);
    static void drawOval(int x, int y, int w, int h, int color);
    static void setClipRectangle(int x1, int y1, int x2, int y2);
    static void drawPolyline(Dpoint_t *pts, int npts, int color);
    static void drawLine(int x1, int y1, int x2, int y2, int color);
    static void drawLine(double x1, double y1, double x2, double y2, int color);
#if defined (MOTIF) && (ORIG)
    static void drawLines(Display *dpy, Drawable xid, GC gc,
                          double xoff, double yoff, Dpoint_t *pts, int npts);
#endif
    static void fillPolygon(Gpoint_t *pts, int npts, int color);
    static const char *getFontName(int size);
    static int loadFont(int size);
    static void getTextExtents(const char *str, int size,
                               int *ascent, int *descent, int *width);
    static void drawString(const char *str, int x, int y,
                           int color, int width, int ascent, int descent);
    static BackStore_t allocateBackingStore(BackStore_t bs, int wd, int ht);
    static bool allocateCanvasBacking();
    static XID_t getCanvasBackingId();
    static void freeBackingStore(BackStore_t bs);
    static XID_t setDrawable(XID_t);
    static bool copyImage(XID_t src, XID_t dst,
			  int src_x, int src_y,
			  int src_wd, int src_ht,
			  int dst_x, int dst_y);
    
private:

};

typedef struct _graphics_dev
{
    Display *xdpy;		/* X display device */
    int scrn;			/* Screen number to use */
    XID xid;			/* X window to be drawn */
    GC xgc;			/* X graphics context, used for default */
    GC pxgc;			/* X graphics context for pixmap operations */
#if defined (MOTIF) && (ORIG)
    XGCValues xgcval;		/* X graphics context values to be masked */
#endif
    Pixel *cmap;		/* Vnmr color index to X pixel mapping */
    PCMF cmapFunc;    		/* Function to set pixel color */
} gdev_t;

/*
 * Which side of the image the first line of data corresponds to.
 */
typedef enum {
    ORIENT_TOP,        /* if data lines start at top image scan lines */
    ORIENT_BOTTOM,     /* Vnmr phasefiles, bass ackwards */
    ORIENT_LEFT,
    ORIENT_RIGHT
} dataOrientation_t;

/*
 * The following macros are used to manipulate one of XGCValues.
 * The macros don't necessary represent all values in XGCValues. They
 * are defined when it is necessary.
 */
#if defined (MOTIF) && (ORIG)
#define	G_Set_Op(gd,val) (gd).xgcval.function = (val); \
	XChangeGC((gd).xdpy, (gd).xgc, GCFunction, &((gd).xgcval))
#define G_Set_Color(gd,val)  XSetForeground((gd).xdpy, (gd).xgc, \
	((gd).xgcval.function == GXxor) ? \
        ((gd).xcolor[0] ^ (gd).xcolor[val]) : (gd).xcolor[val]);
#define	G_Get_Op(gd) (gd).xgcval.function

/***
#define	G_Set_LineWidth(gd,val) (gd).xgcval.line_width = (val); \
	XChangeGC((gd).xdpy, (gd).xgc, GCLineWidth, &((gd).xgcval))
#define	G_Set_LineStyle(gd,val) (gd).xgcval.line_style = (val); \
	XChangeGC((gd).xdpy, (gd).xgc, GCLineStyle, &((gd).xgcval))
#define	G_Get_LineWidth(gd) (gd).xgcval.line_width
#define	G_Get_LineStyle(gd) (gd).xgcval.line_style
#define	G_Get_Font(gd) (gd).xgcval.font
***/

#endif

Pixmap
aipDisplayImage(float *data,
		int width,
		int height);/*TEST*/

Pixmap
aipDisplayImage(colormapSegment_t idx,  /* index of palette to use */
		float *data,            /* pointer to data points */
		int datasetWidth,    	/* # data points per scanline */
		int datasetHeight,      /* # scanlines (height of) data */
		int srcStx,             /* x offset into source data */
		int srcSty,             /* y offset into source data */
		int srcWd,              /* # of data width to use , must be */
                                        /* <= points_per_line */
		int srcHt,              /* # of data height to use, must be */
                                        /* <= scan_lines */
		int pix_stx,            /* Screen x-coord of first pixel */
		int pix_sty,            /* Screen y-coord of first pixel */
		int pixWd,              /* # of width pixels to draw */
		int pixHt,              /* # of height pixels to draw */
		dataOrientation_t direction,  /* Which side data starts on */
		spVsInfo_t vsfunc, 	/* Vertical scaling of data */
		interpolation_t smooth,	/* How to interpolate between data */
		bool keep_pixmap);	/* Return a pixmap of the image */

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
		bool keep_pixmap); // If true, try to return pixmap

void
aipPrintImage(float *data,
	      int datasetWidth,
	      int datasetHeight,
	      int srcStx,//TODO: doubleize
	      int srcSty,//TODO: doubleize
	      int srcWd,//TODO: doubleize
	      int srcHt,//TODO: doubleize
	      int pixWd,
	      int pixHt,
	      spVsInfo_t vsfunc,
	      interpolation_t smooth);

int getWinWidth();
int getWinHeight();
void aipInitGrayscaleCMS();

extern "C" {
    // Duplicated from aipGraphics.h
    void aipFreePixmap(XID_t pixmap);
	   }

#endif /* (not) AIPGRAPHICS_H */
