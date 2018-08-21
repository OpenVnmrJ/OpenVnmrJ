/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef UNIX
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <signal.h>
#include <setjmp.h>
#endif

#ifdef MOTIF
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

 /* to avoid include stdarg.h */
#define _STDARG_H
#ifdef OLIT
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/DrawArea.h>
#include <Xol/Exclusives.h>
#include <Xol/Form.h>
#include <Xol/Menu.h>
#include <Xol/OblongButt.h>
#include <Xol/RectButton.h>
#include <Xol/StaticText.h>
#include <Xol/PopupWindo.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#else 
#include <Xm/Xm.h>
#include <Xm/Protocols.h>
#include <Xm/BulletinB.h>
#include <Xm/DrawingAP.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/LabelP.h>
#include <Xm/RowColumnP.h>
#include <Xm/ToggleBG.h>
#endif /* OLIT */
#else /* not MOTIF */
typedef unsigned long XID;
typedef XID Font;
typedef XID Colormap;
typedef XID Pixel;
typedef XID Widget;
typedef XID Window;
typedef XID GC;
typedef XID Atom;
typedef char XImage;
typedef char* Pixmap;
typedef char* XFontStruct;
typedef char Display;
typedef char* XrmDatabase;
typedef char* Arg;
typedef char* XtIntervalId;
typedef char* XmString;
typedef char* XEvent;
typedef char* XtPointer;
typedef char* XmFontList;
typedef int   Dimension;
#endif

#define    VNUM   8     
#define    LVNUM  10
#define    SVNUM  2
#define    ARRAYMAX  12
#define    GRAD_NUM  3

// display mode
#define    TIMEMODE     0x01
#define    POWERMODE    0x02
#define    PHASEMODE    0x04
#define    INTMODE      0x08
#define    DSPLABEL     0x10
#define    DSPVALUE     0x20
#define    DSPMORE      0x40
#define    DSPDECOUPLE  0x80

// RF shape
#define    REAL_SHAPE    0x01
#define    ABS_SHAPE     0x02
#define    IMAGINARY_SHAPE   0x03

#define    RF_POWERS     1
#define    GRAD_POWERS   2

#define    FIDNODE      333
#define    RFONOFF      322
#define    PRGMARK      320
#define    VPRGMARK     321
#define    VDEVON       323
#define    RTDELAY      6

#define    XMARK        0x01
#define    XHGHT        0x02
#define    XWDH         0x04
#define    XFUP         0x08
#define    XBOX         0x10
#define    OBLGR        0x20
#define    NINFO        0x40
#define    XNFUP        0x80
#define    OBIMAGE      0x100
#define    MAIMAGE      0x200
#define    RAMPUP       0x400
#define    RAMPDN       0x800
#define    XLOOP        0x1000

#define    PLACQ        0x01       // acquire in parallel
#define    PLBOX        0x02       // pfx * 2
#define    PL_LOOP      0x04       // loopW
#define    PLETC        0x08       // spW
#define    XDLY         0x2000     // delay in parallel
#define    XPULS        0x4000     // pulse in parallel
#define    XPULS2       0x8000

#define    RSTNODE       0x01
#define    RCHNODE       0x02
#define    RPGNODE       0x04
#define    ASTNODE       0x10
#define    ACHNODE       0x20
#define    APGNODE       0x40


typedef struct _shape_node {
        char    *name;  
        char    *filename;  
        int     type; // RF or GRD
        int     dataSize;
        int     shapeType; 
        int     height; 
        double  power;
        double  minData;
        double  maxData;
        double  *amp;  // amplitude from  RF/GRD file
        double  *phase;  // pahse from RF file
        short   *data;
        struct _shape_node  *next;
  } SHAPE_NODE;

typedef struct  {
        int     val[VNUM];
        double  fval[VNUM];
        char    *vlabel[VNUM], *flabel[VNUM];
        char    *dname, *pattern;
        double  catt, fatt;
   } COMMON_NODE;

typedef struct  {
        int     val[LVNUM];
        double  fval[SVNUM];
        char    *vlabel[LVNUM];
        char    *flabel[SVNUM];    
        char    *dname, *pattern;  
   } INCGRAD_NODE;

typedef struct  {
        int     val[VNUM];
        double  fval[VNUM];
        char    *vlabel[VNUM];
        char    *flabel[VNUM];
        int     op;
   } RT_NODE;

typedef struct  {
        int     x1[TOTALCH]; /* parallel channel starting x */
        int     x2[TOTALCH]; /* parallel channel ending x */
        double  pulses[TOTALCH]; /* parallel channel ending x */
        double  delays[TOTALCH]; /* parallel channel ending x */
   } PAL_NODE;

typedef struct  {
        int     cond;  /* condition  of decoupleron */
        int     val[VNUM];
        double  fval[VNUM];
        char    *vlabel[VNUM], *flabel[VNUM];
        char    **exlabel, **exval;  /*  extra messages */
        int     vnum, fnum, exnum;
   } EX_NODE;

typedef struct  _dock_node {
        int     type;
        int     id;  /* the id of origin node */
        int     ix;  /* the index of  array */
        int     dock; /* front, middle, or end */
        double   time;
        struct _dock_node   *next;
        struct _dock_node   *dnext;
        struct _dock_node   *prev;
  } DOCK_NODE;


typedef struct  _dps_node {
        int     type;
        int     flag;
        int     pflag;   // for parallel
        int     device;
        int     line;
        int     id;      /* the id of itself, for docking */
        int     pid;      /* the source id of duplicated node */
        int     dockAt;  /* the id of obj to be docked */
        int     dockFrom;
        int     dockTo;
        int     dockNum; /* the number of dockList */
        double   rtime;
        double   ptime;
        double   power;
        double   phase;
        double   rtime2; /* the pseudo time for docking */
        double   stime; /* the time of starting */
        double   etime; /* the time of ending */
        double   mtime; /* the time of middle point */
        double   xstime; /* for dpsx program, the time of starting */
        double   xetime; /* for dpsx program, the time of ending */
        double   rg1;    // pre-pusle delay
        double   rg2;    // post-pulse delay
        char    *fname;
        union {
                COMMON_NODE   common_node;
                INCGRAD_NODE  incgrad_node;
                RT_NODE       rt_node;
                PAL_NODE      pal_node;
                EX_NODE       ex_node;
              } node;
        int   x1, x2, x3;
        int   y1, y2, y3;
        int   mx;   /* middle point */
        int   mDocked;   /* middle was docked */
        int   active;
        int   visible;
        int   color;
        int   mode;
        int   wait;
        int   updateLater;
        DOCK_NODE     *lNode;  /* left dock node */
        DOCK_NODE     *mNode;  /* middle dock node */
        DOCK_NODE     *rNode;  /* right dock node */
        SHAPE_NODE    *shapeData; // RF or GRD shape
        struct _dps_node  *next;
        struct _dps_node  *pnode;  /* node of previous */
        struct _dps_node  *bnode;  /* node of brother, e.g. simpulse */
        struct _dps_node  *dnext;  /* next node for drawing */
        struct _dps_node  *knext;  /* the node in the same channel */
        struct _dps_node  *xnext;  /* the parallel node in the same channel */
        struct _dps_node  **dockList;  /* list of docking nodes */
  } SRC_NODE;


typedef struct  _parallel__node {
        int      x;
        double   time;
        struct _parallel__node  *previous;
        struct _parallel__node  *next;
  } PARALLEL_XNODE;

typedef struct  _vangle_node {
        char    *name;
        int     id;
        int     size;
        double   angle_set[3][10];
        struct _vangle_node *next;
  } ANGLE_NODE;

typedef struct  _rfgrp_node {
        char    *name;
        char    *pattern;
        char    *mode;
        int     id;
        int     active;
        int     frqSize;
        int     newFrqList;
        int     val[VNUM];
        double   fval[VNUM];
        double   width;
        double   coarse;
        double   finePower;
        double   startPhase;
        double   frqList[10];
        char    *vlabel[VNUM], *flabel[VNUM];
        struct _rfgrp_node *next;
        struct _rfgrp_node *child;
  } RFGRP_NODE;

typedef struct  _aptable {
        int   id;
        int   *val;
        int   *oval;  /* back up data */
        int   index;
        int   changed;
        int   size;
        int   autoInc;
        int   divn;
        struct _aptable *next;
        } AP_NODE;

typedef struct  _array_node {
        int     code;
        int     type;
        int     id;
        int     dev;
        int     ni;
        char    *name; 
        int     vars;  /* size */
        int     index;
        int     jid;
        int     tx, ty;  
        int     linked;  /* used for diagonal array */
        int     widget_num;
        Widget  index_widget;
        union {
                double   *Val;
                char     **Str;
        } value;
        struct _array_node     *next;
        struct _array_node     *dnext; /* for diagonal array */
   } ARRAY_NODE;

