/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

// Vnmrbg jFunc keys

package vnmr.util;
public interface VnmrKey
{
    static final int GRAPH = 1;
    static final int PORT = 2;
    static final int HOST = 3;
    static final int MISC = 4;
    static final int VEXIT = 1;
    static final int VCLOSE = 2;
    static final int VSYNC = 4;
    static final int VSIZE = 5;
    static final int VMOUSE = 6;
    static final int VMENU = 7;
    static final int PVAL = 9;
    static final int PGRP = 10;
    static final int PSTATUS = 12;
    static final int PMINMAX = 11;
    static final int SHOWCOND = 14;
    static final int PGLOBAL = 15;
    static final int WINID = 16;
    static final int PEXPR = 17;
    static final int VFLUSH = 18;
    static final int VPANEL = 19;
    static final int PAINT = 21;
    static final int FONTSIZE = 22;
    static final int REDRAW = 23;
    static final int XEVENT = 24;
    static final int XINPUT = 29;
    static final int XMOVE = 30;
    static final int XSHOW = 31;
    static final int XREGION = 32;
    static final int WINID2 = 33;  // tearoff window id 
    static final int PAINT2 = 34; // paint tearoff window
    static final int VSIZE2 = 35;
    static final int XMASK = 37; // mouse mask
    static final int XMASK2 = 38; // another mouse mask
    static final int XCOPY = 39; // copy pixmap to window
    static final int VLAYOUT = 40; // layout parameter
    static final int XREGION2 = 42;
    static final int FRMSIZE = 43;
    static final int FRMID = 44;
    static final int FRMLOC = 45;
    static final int OVLYTYPE = 46;
    static final int SCRNDPI = 47;  // screen resolution
    static final int JPRINT = 48;  // plot screen image
    static final int JSYNC = 49;  // the end of plot screen
    static final int PSIZE = 50;
    static final int FRMSTAUS = 51;
    static final int WHEEL = 52;
    static final int AIPMENU = 53;
    static final int APPLYCMP = 54;    // set aip colormap 
    static final int IMGORDER = 57;    // set aip image z order
    static final int SELECTIMG = 58;    // select aip image
    static final int TABLEACT = 60;    // canvas table changed
    static final int PAINTALL = 67;  // repaint all objs in canvas
    static final int CSIDISP = 68;  // graphics switch to csi
    static final int OPAQUE = 69;  // transparency level
    static final int PVSIZE = 70;  // pre-resize
    static final int CSIOPENED = 71;
    static final int SAVESPECDATA = 72;
    static final int CSIWINDOW = 88;  // open/close csi window
    static final int FRM_FONTSIZE = 89;
    static final int FONTARRAY = 90;
    static final int FVERTICAL = 103;
    static final int CLOSEALL = 0;
    static final int CLOSEGRAPH = 1;
    static final int CLOSECOMMON = 2;
    static final int CLOSECALPHA = 3;
    static final int VNMRADDR = 4;
}

