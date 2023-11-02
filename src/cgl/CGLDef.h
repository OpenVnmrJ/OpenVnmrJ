/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

#ifndef CGLDEF_H_
#define CGLDEF_H_

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#define CVSIZE     1000

#ifndef NULL
#define NULL 	0
#endif
#define PI		3.14159265358979323846
#define RPD		(2.0*PI/360.0)
#define MALLOC(a,b,c)   if((c=(b*)malloc((a)*sizeof(b)))==NULL)\
  							{printf("malloc error\n");}
#define FREE(a)   		if(a!=NULL) {::free(a);a=NULL;}
#define DELETE(a)   	if(a!=NULL) {delete (a);a=NULL;}

#define MAXSLICES 1024

enum {

    NONE=0,
    ARB=1,
    GLSL=2,
    SHADER=0x3,

	X_AXIS = 0,
    Y_AXIS = 1,
    Z_AXIS = 2,

    FRONT = 0,
    BACK = 1,

    PLUS_X = 0,
    PLUS_Y = 1,
    PLUS_Z = 2,
    MINUS_X = 3,
    MINUS_Y = 4,
    MINUS_Z = 5,

    // flags

    INVALIDVIEW=    0x01,
    FROMFRNT=       0x02,
    LOCKAXIS=       0x04,

	// NOTE: these definitions must match those in JGLDef.java

    // render bitfields

    // 1 trace draw mode

    SHOWPOINTS     = 0x00000000,
    SHOWLINES      = 0x00000001,
    SHOWPOLYGONS   = 0x00000002,
    SHOWLTYPE      = 0x00000003,
    SHOWHIDE       = 0x00000004,

    // 2 projection mode

    SHOW1D         = 0x00000000,
    SHOW1DSP       = 0x00000010,
    SHOW2D         = 0x00000020,
    SHOW2DSP       = 0x00000030,
    SHOW3D         = 0x00000040,
    SHOWPTYPE      = 0x00000070,
    //  3 color mode

    SHOWONECOL     = 0x00000000,
    SHOWIDCOL      = 0x00000080,
    SHOWHTCOL      = 0x00000100,
    SHOWCTYPE      = 0x00000180,

    //  4 color palette choice

    SHOWABSCOLS    = 0x00000000,
    SHOWPHSCOLS    = 0x00000200,
    SHOWGRAYS      = 0x00000400,
    SHOWCTRCOLS    = 0x00000600,
    SHOWPALETTE    = 0x00000600,

    // 5 texture options

    SHOWBLEND      = 0x00000800,
    SHOWCLAMP      = 0x00001000,
    SHOWXPARANCY   = 0x00002000,
    SHOWTEXOPTS    = SHOWBLEND|SHOWCLAMP|SHOWXPARANCY,

    // 6 2D effects

    SHOWCONTOURS   = 0x00004000,
    SHOWLIGHTING   = 0x00008000,
    LINESMOOTH     = 0x00010000,
    SHOWGRID       = 0x00020000,

    // 7 complex data display options

    SHOWREAL       = 0x00000000,
    SHOWIMAG       = 0x00100000,
    SHOWRANDI      = 0x00200000,
    SHOWABSVAL     = 0x00300000,
    SHOWDTYPE      = 0x00300000,

    // 8 sliceplane axis (3D data only)

    SHOWZ          = 0x00000000,
    SHOWY          = 0x00400000,
    SHOWX          = 0x00800000,
    SHOWAXIS       = 0x00C00000,

    // 9 Color table range mode

    SHOWCLIPLOW    = 0x01000000,
    SHOWCLIPHIGH   = 0x02000000,
    CLIPMODE       = SHOWCLIPHIGH|SHOWCLIPLOW,

    // 10 3D shader mode

    VOL      	   = 0x00000000,
    MIP   		   = 0x04000000,
    ISO     	   = 0x08000000,
    ABS     	   = 0x10000000,
    SHADERMODE     = 0x0C000000,

    // data display bitcodes
    // NOTE: these definitions must match those in CGLRenderer.h (jni)

    // data type (g3dtype) flags

    D1=        0x00000000,
    D2=        0x00000001,
    D3=        0x00000002,
    DIM =      0x00000003,
    COMPLEX=   0x00000008,
    FID =      0x00000000|COMPLEX,
    SPECTRUM = 0x00000004|COMPLEX,
    IMAGE=     0x00000000,
    DCONI=     0x00000004,
    DTYPE =    0x0000000C,

    // drawing options

	LINES=     0x00000000,
	POINTS=    0x00000001,
	POLYGONS=  0x00000002,
	ZMASK=     0x00000003,
	DRAWMODE = LINES|POINTS|POLYGONS|ZMASK,
	ONETRACE=  0x00000000,
	OBLIQUE=   0x00000004,
	TWOD=      0x00000008,
	SLICES=    0x0000000C,
	THREED=    0x00000010,
	PROJECTION = ONETRACE|OBLIQUE|TWOD|THREED|SLICES,
	COLONE=    0x00000000,
	COLIDX=    0x00000020,
	COLHT=     0x00000040,
	COLTYPE =  COLONE|COLIDX|COLHT,
	CLAMP=     0x00000080,
	BLEND=     0x00000100,
	XPARANCY=  0x00000200,
	GRID=      0x00000400,
	CONTOURS=  0x00001000,
	LIGHTING=  0x00002000,
	REVERSED=  0x00004000,
	Z =        0x00000000,
	Y =        0x00010000,
	X =        0x00020000,
	SLICEPLANE =  Z|Y|X,
	FIXAXIS =  0x00040000,
	REVDIR  =  0x00080000,
	VOLSHADER= 0x00000000,
	MIPSHADER= 0x00100000,
	ISOSHADER= 0x00200000,
	CONSHADER= 0x00300000,
	SHADERTYPE=VOLSHADER|MIPSHADER|ISOSHADER|CONSHADER,
	CLIPLOW=   0x00400000,
	CLIPHIGH=  0x00800000,

    // color ID codes

    NUMSTDCOLS     = 16,
    NUMABSCOLS     = 16,
    NUMPHSCOLS     = 14,
    NUMGRAYS       = 256,
    NUMCTRCOLS     = 33,
    NUMCOLORS      = 1024,

    STDCOLS        = 0,
    ABSCOLS        = 1,
    PHSCOLS        = 2,
    GRAYCOLS       = 3,
    CTRCOLS        = 4,
    PALETTE        = 0x07,
    NEWCTABLE      = 0x08,
    TEXFLAGS       = NEWCTABLE,
    HISTOGRAM      = 0x10,
    COLORMODE      = HISTOGRAM,

    BGCOLOR        = 0,
    REALCOLOR      = 1,
    IMAGCOLOR      = 2,
    PHSREALCOLOR   = 3,
    GRIDCOLOR      = 15,

    NEWTEXMAP      = 0x01,
    NEWTEXGEOM     = 0x02,
    NEWLAYOUT      = 0x04,
    NEWTEXMODE     = 0x08,
    NEWSHADER      = 0x10,
    USELIGHTING    = 0x20

};

#endif /*CGLDEF_H_*/

