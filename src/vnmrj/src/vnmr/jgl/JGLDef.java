/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

public interface JGLDef {
    static final int SEL_BOX = 5;
    static final int SEL_AXIS = 6;
    static final int SET_SVECT = 7;
    static final int SEL_SPLANE = 8;
    static final int SEL_WPLANE = 9;
    static final int SEL_XPLANE = 10;
    static final int SEL_YPLANE = 11;
    static final int SEL_ZPLANE = 12;
    static final int SEL_XAXIS = 13;
    static final int SEL_YAXIS = 14;
    static final int SEL_ZAXIS = 15;
    static final int SEL_TRACE = 16;
    static final int SEL_POINT = 17;

    static final int NONE = 0;
    static final int ARB = 1;
    static final int GLSL = 2;
    static final int SHADER = 0x3;

    static final int X_AXIS = 0;
    static final int Y_AXIS = 1;
    static final int Z_AXIS = 2;

    static final int FRONT = 0;
    static final int BACK = 1;

    static final int PLUS_X = 0;
    static final int PLUS_Y = 1;
    static final int PLUS_Z = 2;
    static final int MINUS_X = 3;
    static final int MINUS_Y = 4;
    static final int MINUS_Z = 5;
       
    static final int G3D = 100; // jFunc id code i.e. jFunc(100,..)
    
    // reset bit codes
    
    static final int RESETSTATUS=    0x2000;
    static final int RESETSHOW=      0x1000;
    static final int RESETCWIDTH=    0x0800;
    static final int RESETVROT=      0x0400;
    static final int RESETVPNT=      0x0200;
    static final int RESETSVECT=     0x0100;
    static final int RESETWIDTH=     0x0080;
    static final int RESETINTENSITY= 0x0040;
    static final int RESETXPARENCY=  0x0020;
    static final int RESETRESOLUTION=0x0010;
    static final int RESETSLICE=     0x0008;
    static final int RESETROTATION=  0x0004;
    static final int RESETZOOM=      0x0002;
    static final int RESETOFFSET=    0x0001;
    static final int RESETLOC=       RESETWIDTH|RESETVROT|RESETVPNT|RESETSLICE;
    static final int RESETVIEW=      RESETLOC|RESETSVECT|RESETZOOM|RESETROTATION|RESETOFFSET;
    static final int RESETCOLORS=    RESETINTENSITY|RESETXPARENCY;
    static final int RESETCONTOURS=  RESETRESOLUTION|RESETCWIDTH;
    static final int RESETALL=       0x0FFF;
    
    //====== g3ds [status/control] bitfields ============
    
    // g3ds[1] STATUS
    
    static final int STATRESET      = 0x00000000;
    static final int STATRUNNING    = 0x00000001;
    static final int STATNEXT       = 0x00000002;
    static final int STATREVERSE    = 0x00000004;
    static final int STATPHASING    = 0x00000008;
    static final int STATINDEX      = 1;
    static final int STATFIELD      = 0;
    static final int STATBITS       = 4;

    // g3ds[2] LOCKS

    static final int SLICELOCK      = 0x00000010; // set true for imaging!
    static final int AXISLOCK       = 0x00000020;
    static final int VROTLOCK       = 0x00000040;
    static final int CLIPVPLANES    = 0x00000080;
    static final int LOCKS          = AXISLOCK|SLICELOCK|VROTLOCK;
    static final int LOCKSINDEX     = 2;
    static final int LOCKSFIELD     = 4;
    static final int LOCKSBITS      = 4;

    // g3ds[3] AUTO
    
    static final int AUTOINDEX     = 3;
    static final int AUTOFIELD     = 8;
    static final int AUTOBITS      = 8;
    static final int AUTOSCALE     = 0x00000100;
    static final int AUTOXFER      = 0x00000200;
    static final int AUTORESET     = 0x00000400;
    static final int AUTOFRONTFACE = 0x00000800;
    static final int AUTOSELECT    = 0x00001000;
    
    // g3ds[4]  COM

    static final int SHOWORTHO     = 0x00010000;
    static final int PACEDISPLAY   = 0x00020000;
    static final int NOMMAP        = 0x00040000;
    static final int COMINDEX      = 4;
    static final int COMFIELD      = 16;
    static final int COMBITS       = 4;

    // g3ds[5] Animate mode   
    
    static final int ANIMATESLC     = 0x00100000;
    static final int ANIMATEROT     = 0x00200000;
    static final int ANIMATEMODE    = ANIMATESLC|ANIMATEROT;
    static final int ANIMATIONINDEX = 5;
    static final int ANIMATIONFIELD = 20;
    static final int ANIMATIONBITS  = 2;
    
    // g3ds[6] mouse mode   

    static final int INTENSITY      = 0x00000000;
    static final int TRANSPARENCY   = 0x00400000; 
    static final int RESOLUTION     = 0x00800000;
    static final int ISOSURFACE     = 0x00C00000;
    static final int MOUSEMODE      = 0x00C00000;
    static final int MMODEINDEX     = 6;
    static final int MMODEFIELD     = 22;
    static final int MMODEBITS      = 2;

   //====== g3dp [prefs] bitfields ============

    // g3dp[1] used in vj gui but not in java code
    
    static final int VJINDEX        = 1;
    static final int VJFIELD        = 0;
    static final int VJBITS         = 4;
    static final int SHOWADVANCED   = 0x00000001;
    static final int SHOWPERCENT    = 0x00000002;
    static final int VPJOINED       = 0x00000004;

   // g3dp[2] 2D tools show bits

    static final int VLABELS        = 0x00000010;
    static final int VTEXT          = 0x00000020;
    static final int VCURSORS       = 0x00000040;
    static final int VGRID          = 0x00000080;
    static final int VTOOLSINDEX    = 2;
    static final int VTOOLSFIELD    = 4;
    static final int VTOOLSBITS     = 4;

   // g3dp[3] 3D tools show bits

    static final int V3DTOOLS       = 0x00000100;
    static final int V3DDATA        = 0x00000200;
    static final int V3DPNT         = 0x00000400;
    static final int V3DROT         = 0x00000800;
    static final int V3DPNTROT      = V3DPNT|V3DROT;
    static final int V3DBOX         = 0x00001000;
    static final int V3DSWIDTH      = 0x00002000;
    static final int V3DROTAXIS     = 0x00004000;
    static final int V3DTOOLSINDEX  = 3;
    static final int V3DTOOLSFIELD  = 8;
    static final int V3DTOOLSBITS   = 8;
 
    //============== g3di [show] bitfields ===============

    // g3di[1] trace draw mode

    static final int SHOWPOINTS     = 0x00000000;
    static final int SHOWLINES      = 0x00000001;
    static final int SHOWPOLYGONS   = 0x00000002;
    static final int SHOWLTYPE      = 0x00000003;
    static final int SHOWHIDE       = 0x00000004;
    static final int LTYPEINDEX     = 1;
    static final int LTYPEFIELD     = 0;
    static final int LTYPEBITS      = 4;

    // g3di[2] projection mode
    
    static final int SHOW1D         = 0x00000000;
    static final int SHOW1DSP       = 0x00000010;   
    static final int SHOW2D         = 0x00000020;
    static final int SHOW2DSP       = 0x00000030;   
    static final int SHOW3D         = 0x00000040;
    static final int SHOWPTYPE      = 0x00000070;
    static final int PTYPEINDEX     = 2;
    static final int PTYPEFIELD     = 4;
    static final int PTYPEBITS      = 3;

    // g3di[3] color mode
    
    static final int SHOWONECOL     = 0x00000000;
    static final int SHOWIDCOL      = 0x00000080;
    static final int SHOWHTCOL      = 0x00000100;
    static final int SHOWCTYPE      = 0x00000180;
    static final int CTYPEINDEX     = 3;
    static final int CTYPEFIELD     = 7;
    static final int CTYPEBITS      = 2;

    // g3di[4] color table choice
    
    static final int SHOWABSCOLS    = 0x00000000;
    static final int SHOWPHSCOLS    = 0x00000200;
    static final int SHOWGRAYS      = 0x00000400;
    static final int SHOWCTRCOLS    = 0x00000600;
    static final int SHOWPALETTE    = 0x00000600;
    static final int PALETTEINDEX   = 4;
    static final int PALETTEFIELD   = 9;
    static final int PALETTEBITS    = 2;

    // g3di[5] texture options
    
    static final int SHOWBLEND      = 0x00000800;
    static final int SHOWCLAMP      = 0x00001000;
    static final int SHOWXPARANCY   = 0x00002000;
    static final int SHOWTEXOPTS    = SHOWBLEND|SHOWCLAMP|SHOWXPARANCY;
    static final int TEXOPTSINDEX   = 5;
    static final int TEXOPTSFIELD   = 11;
    static final int TEXOPTSBITS    = 3;

    // g3di[6] 2D effects   

    static final int SHOWCONTOURS   = 0x00004000;
    static final int SHOWLIGHTING   = 0x00008000;
    static final int LINESMOOTH     = 0x00010000;
    static final int SHOWGRID       = 0x00020000;
    static final int EFFECTSINDEX   = 6;
    static final int EFFECTSFIELD   = 14;
    static final int EFFECTSBITS    = 6;

    // g3di[7] complex data display options

    static final int SHOWREAL       = 0x00000000;
    static final int SHOWIMAG       = 0x00100000; 
    static final int SHOWRANDI      = 0x00200000;
    static final int SHOWDTYPE      = 0x00300000;
    static final int DTYPEINDEX     = 7;
    static final int DTYPEFIELD     = 20;
    static final int DTYPEBITS      = 2;
    
    // g3di[8] sliceplane axis (3D data)
    
    static final int SHOWZ          = 0x00000000;
    static final int SHOWY          = 0x00400000;
    static final int SHOWX          = 0x00800000;
    static final int SHOWAXIS       = 0x00C00000;
    static final int AXISINDEX      = 8;
    static final int AXISFIELD      = 22;
    static final int AXISBITS       = 2;

    // g3di[9] Color table range mode   
    
    static final int SHOWCLIPLOW    = 0x01000000;
    static final int SHOWCLIPHIGH   = 0x02000000;
    static final int CLIPMODE       = SHOWCLIPHIGH|SHOWCLIPLOW;
    static final int CTMODEINDEX    = 9;
    static final int CTMODEFIELD    = 24;
    static final int CTMODEBITS     = 2;

    // g3di[10] 3D shader mode   
    
    static final int VOL      		= 0x00000000;
    static final int MIP   			= 0x04000000; 
    static final int ISO     		= 0x08000000;
    static final int ABS            = 0x10000000;
    
    static final int SHADERMODE     = 0x0C000000;

    static final int SHADERINDEX    = 10;
    static final int SHADERFIELD    = 26;
    static final int SHADERBITS     = 3;


    //  ============== g3dtype bitfields ===============

    static final int DATADIMINDEX   = 1;
    static final int DATADIMFIELD   = 0;
    static final int DATADIMBITS    = 2;

    static final int DATATYPEINDEX  = 2;
    static final int DATATYPEFIELD  = 2;
    static final int DATATYPEBITS   = 2;

    // render flags (passed to GLRendererIF in render function)
    // NOTE: these definitions must match those in CGLRenderer.h (cgl)

    static final int INVALIDVIEW =   0x01;
    static final int FROMFRNT=       0x02;
    static final int LOCKAXIS=       0x04;

    // g3dtype flags (passed to GLRendererIF in setData function)
    // NOTE: these definitions must match those in CGLRenderer.h (cgl)
   
    static final int D1=        0x00000000;
    static final int D2=        0x00000001;
    static final int D3=        0x00000002;
    static final int DIM =      0x00000003;
    static final int COMPLEX=   0x00000008;
    static final int FID =      0x00000000|COMPLEX;
    static final int SPECTRUM = 0x00000004|COMPLEX;
    static final int IMAGE=     0x00000000;
    static final int DCONI=     0x00000004;
    static final int DTYPE =    0x0000000C;
  
    // RendererIF options bitcodes

    static final int LINES=     0x00000000; 
    static final int POINTS=    0x00000001; 
    static final int POLYGONS=  0x00000002; 
    static final int ZMASK=     0x00000003; 
    static final int DRAWMODE = LINES|POINTS|POLYGONS|ZMASK;
    static final int ONETRACE=  0x00000000; 
    static final int OBLIQUE=   0x00000004;
    static final int TWOD=      0x00000008;     
    static final int SLICES=    0x0000000C;     
    static final int THREED=    0x00000010;     
    static final int PROJECTION = ONETRACE|OBLIQUE|TWOD|THREED|SLICES;
    static final int COLONE=    0x00000000;     
    static final int COLIDX=    0x00000020;     
    static final int COLHT=     0x00000040;     
    static final int COLTYPE =  COLONE|COLIDX|COLHT;
    static final int CLAMP=     0x00000080;     
    static final int BLEND=     0x00000100;     
    static final int XPARANCY=  0x00000200;     
    static final int GRID=      0x00000400;     
    static final int CONTOURS=  0x00001000;     
    static final int LIGHTING=  0x00002000;     
    static final int REVERSED=  0x00004000;     
    static final int Z =        0x00000000;
    static final int Y =        0x00010000;
    static final int X =        0x00020000;
    static final int SLICEPLANE =  Z|Y|X;    
    static final int FIXAXIS =  0x00040000;
    static final int REVDIR  =  0x00080000;
    static final int VOLSHADER= 0x00000000;
    static final int MIPSHADER= 0x00100000;
    static final int ISOSHADER= 0x00200000;
    static final int CONSHADER= 0x00300000;
    static final int SHADERTYPE=VOLSHADER|MIPSHADER|ISOSHADER|CONSHADER;
    static final int CLIPLOW=   0x00400000;
    static final int CLIPHIGH=  0x00800000;
   
    // color ID codes

    static final int STDCOLS        = 0;
    static final int ABSCOLS        = 1;
    static final int PHSCOLS        = 2;
    static final int GRAYCOLS       = 3;
    static final int CTRCOLS        = 4;
    static final int PALETTE        = 0x07;
    static final int NEWCTABLE      = 0x08;    
    static final int TEXFLAGS       = NEWCTABLE;
    static final int HISTOGRAM      = 0x10;
    static final int COLORMODE      = HISTOGRAM;

    static final int NUMSTDCOLS     = 16;
    static final int NUMABSCOLS     = 16;
    static final int NUMPHSCOLS     = 14;
    static final int NUMCTRCOLS     = 33;
    static final int NUMGRAYS       = 256;
    static final int NUMCOLORS      = 1024;
    static final int CVSIZE         = 1024;

    //  colors used by Renderers
    
    static final int BGCOLOR        = 0;
    static final int REALCOLOR      = 1;
    static final int IMAGCOLOR      = 2;
    static final int PHSREALCOLOR   = 3;
    
    // colors used by JGLGraphics
    
    static final int CURS0R1COLOR   = 4;
    static final int CURS0R2COLOR   = 5;
    static final int PHSCURSORCOLOR = 6;
    static final int IMGBGCOLOR     = 7;
    static final int IMGFGCOLOR     = 8;   
    static final int XAXISCOLOR     = 9;
    static final int YAXISCOLOR     = 10;
    static final int ZAXISCOLOR     = 11;
    static final int EYECOLOR       = 12;
    static final int TEXTCOLOR      = 13;
    static final int BOXCOLOR       = 14;
    static final int GRIDCOLOR      = 15;
    
    static final int NEWTEXMAP      = 0x01;
    static final int NEWTEXGEOM     = 0x02;
    static final int NEWLAYOUT      = 0x04;
    static final int NEWTEXMODE     = 0x08;
    static final int NEWSHADER      = 0x10;
    static final int USELIGHTING    = 0x20;
}
