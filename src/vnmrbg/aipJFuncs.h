/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef FALSE
#define TRUE 1
#define FALSE 0
#endif

extern "C" {
    // Defined in graphics_win.c

    void aip_setColor(int color);
    void aip_setCursor(const char *name);
    void aip_clearRect(int x, int y, int w, int h);
    void aip_drawRect(int x, int y, int w, int h, int color);
    void aip_drawOval(int x, int y, int w, int h, int color);
    void aip_drawLine(int x, int y, int x2, int y2, int color);
    void aip_drawPolyline(Dpoint_t *pnts, int npts, int color);
    void aip_fillPolygon(Gpoint_t *pnts, int npts, int color);
    void aip_loadFont(int size);
    void aip_getTextExtents(const char *str, int size, int *ascent, int *descent, int *width);
    void aip_drawString(const char *str, int x, int y, int clear, int color);
    void aip_setClipRectangle(int x, int y, int w, int h);
    XID aip_allocateBackupPixmap(XID pix,int w,int h);
    void aip_freeBackupPixmap(XID bs);
    XID aip_setDrawable(XID id);
    void aip_copyImage(XID src, XID dst,int xs,int ys,int w,int h,int xd,int yd);
    Pixmap aip_displayImage(char *data, int colormapID, int transparency, int x, int y, int w, int h, bool keep_pixmap);
    void aip_initGrayscaleCMS(int n);
    void aip_removeTimeOut(XID t);
    XtIntervalId  aip_addTimeOut(unsigned long, void(*), char *);

}

