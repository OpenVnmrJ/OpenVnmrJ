/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;
public interface VGaphDef
{
    static final int CLEAR = 1;
    static final int XBAR = 2;
    static final int TEXT = 3;
    static final int VTEXT = 4;
    static final int GOK = 5;
    static final int RASTER = 6;
    static final int RGB = 7;
    static final int COLORI = 8;
    static final int LINE = 9; // draw line
    static final int XORMODE = 10;
    static final int COPYMODE = 11;
    static final int BOX = 12;
    static final int COLORTABLE = 13;
    static final int CNTMAP = 14;
    static final int REFRESH = 15;
    static final int WINBUSY = 16;
    static final int WINFREE = 17;
    static final int BGCOLOR = 18;
    static final int RTEXT = 19;  // inverse text
    static final int RBOX = 20; // inverse box
    static final int STEXT = 21; // scaled text
    static final int SFONT = 22; // scaled font
    static final int GIN = 23; // gin
    static final int VCURSOR = 24; //  cursor
    static final int ICON = 25;
    static final int COLORNAME = 26;  // color name 
    static final int BATCHON = 27;  // draw to buffer only
    static final int BATCHOFF = 28;  //  batch mode off 
    static final int COPY = 29;  //  copy image buffer to window
    static final int COPY2 = 30;  //  copy part of image buffer to window
    static final int CLEAR2 = 31; // clear image buffer 
    static final int BAR = 32; // draw line into window only
    static final int YBAR = 33;
    static final int VIMAGE = 34; // draw images from imagefile 
    static final int ICURSOR = 35; // set dconi cursor
    static final int DCURSOR = 36; // draw dconi cursor
    static final int BANNER = 37; // draw banner
    static final int ACURSOR = 38; // set cursor by name
    static final int FGCOLOR = 39;
    static final int GCOLOR = 40;
    static final int PRECT = 41;
    static final int PARC = 42;
    static final int POLYGON = 43;
    static final int CROSS = 44;
    static final int REGION = 45;
    static final int HCURSOR = 46; //  crosshair cursor
    static final int JFRAME = 47;  // canvas frame commands
    static final int CLRCURSOR = 48; // clear dconi, ds cursors
    static final int CSCOLOR = 49; // cursor color
    static final int REEXEC = 50; // vbg Reexec Cmd
    static final int PRTSYNC = 51; // end of print canvas
    static final int JALPHA = 52;
    static final int HTEXT = 53;
    static final int MOVIESTART = 54;
    static final int MOVIENEXT = 55;
    static final int MOVIEEND = 56;
    static final int FRMPRTSYNC = 57;
    static final int TICTEXT = 58;
    static final int XORON = 59;
    static final int XOROFF = 60;
    static final int XYBAR = 61; // non xor ybar
    static final int PENTHICK = 62;
    static final int AIP_TRANSPARENT = 63;
    static final int WINPAINT = 64;
    static final int SPECTRUMWIDTH = 65;
    static final int LINEWIDTH = 66;
    static final int AIPID = 67;
    static final int TRANSPARENT = 68;
    static final int SPECTRUM_MIN = 69;
    static final int SPECTRUM_MAX = 70;
    static final int SPECTRUM_RATIO = 71;
    static final int LINE_THICK = 72;
    static final int GRAPH_FONT = 73;
    static final int GRAPH_COLOR = 74;
    static final int NOCOLOR_TEXT = 75;
    static final int BACK_REGION = 76;
    static final int ICURSOR2 = 77; //  new vnmr cursor
    static final int THSCOLOR = 78; //  threshold color
    static final int IMG_SLIDES_START = 79; //  imaging movie mode
    static final int IMG_SLIDES_END = 80;
    static final int ENABLE_TOP_FRAME = 81;
    static final int CLEAR_TOP_FRAME = 82;
    static final int JTABLE = 83;
    static final int JARROW = 84;
    static final int JROUNDRECT = 85;
    static final int JOVAL = 86;
    static final int RGB_ALPHA = 87;
    static final int VBG_WIN_GEOM = 88;
    static final int RAISE_TOP_FRAME = 89;

    static final int JFRAME_OPEN = 1;
    static final int JFRAME_CLOSE = 2;
    static final int JFRAME_CLOSEALL = 3;
    static final int JFRAME_ACTIVE = 4;
    static final int JFRAME_IDLE = 5;
    static final int JFRAME_IDLEALL = 6;
    static final int JFRAME_ENABLE = 7;
    static final int JFRAME_ENABLEALL = 8;
    static final int JFRAME_CLOSEALLText = 9;
    static final int HANDCURSOR = 1; // hand cursor
    static final int DEFCURSOR = 2; // default cursor

    /*  the following definitions are created for image browser */
    static final int IFUNC = 101;
    static final int IRECT = 102;
    static final int ILINE = 103;
    static final int IOVAL = 104;
    static final int IPOLYLINE = 105;
    static final int IPOLYGON = 106;
    static final int IBACKUP = 107;
    static final int IFREEBK = 108;
    static final int IWINDOW = 109;
    static final int ICOPY = 110;
    static final int IRASTER = 111;
    static final int IGRAYMAP = 112;
    static final int ICLEAR = 113;
    static final int ITEXT = 114;
    static final int ICOLOR = 115;
    static final int IPLINE = 116;
    static final int IPTEXT = 117;
    static final int DPCON = 118;
    
    static final int SET3PMODE = 119;
    static final int SET3PCURSOR = 120;
    static final int ICSIWINDOW = 121;
    static final int ICSIDISP = 122;
    static final int OPENCOLORMAP = 123;
    static final int SETCOLORMAP = 124;
    static final int SETCOLORINFO = 125;
    static final int SELECTCOLORINFO = 126;
    static final int ICSIORIENT = 127;
    static final int IVTEXT = 128;
    static final int ANNFONT = 129;
    static final int ANNCOLOR = 130;
    static final int AIPCODE = 1010;
}

