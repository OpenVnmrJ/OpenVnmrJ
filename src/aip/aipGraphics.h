/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef AIPGRAPHICS_H
#define AIPGRAPHICS_H

#ifdef MOTIF
#include <X11/Intrinsic.h>
#else
#include "vjXdef.h"
#endif


#include "aipCStructs.h"

#ifdef	__cplusplus
extern "C" {
#endif


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

    /*void aipFreePixmap(XID_t pixmap);*/

typedef void (*PCMF)(int, int, int, int); /* Pointer to ColorMap Function */
void
aipSetGraphics(Display *, int, XID, GC, GC, Pixel *, palette_t *, PCMF);

void
aipSetGraphicsPalettes(palette_t *);

void
aipSetGraphicsBoundaries(int x, int y, int width, int height);
    /* void aipSetData(float *data, dataInfo_t *dhead); */

#if defined (MOTIF) && (ORIG)
void aipSetCanvasMask(Region region);
#endif

Pixmap
aipDisplayVnmrImage(float *data,
		    int points_per_line,
		    int scan_lines,
		    int src_x,
		    int src_y,
		    int src_width,
		    int src_height,
		    int dest_x,
		    int dest_y,
		    int pix_width,
		    int pix_height,
		    double datamax,
		    colormapSegment_t palette,
		    int smoothing);

#else

/* --------- NON-ANSI/C++ prototypes ------------  */

void aipFreePixmap();
void aipSetGraphics();
void aipSetGraphicsBoundaries();
void aipSetData();
Pixmap aipDisplayVnmrImage();

#endif
 
#ifdef	__cplusplus
}
#endif

#endif /* (not) AIPGRAPHICS_H */
