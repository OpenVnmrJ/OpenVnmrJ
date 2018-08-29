/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/************************************************/
/*						*/
/* graphics.c	-	graphics drivers	*/
/*						*/
/* Provides the necessary routines for graphics */
/* display on the sun screen and different      */
/* graphics terminals, as well as on several    */
/* hp plotters. Tektronix graphics language is  */
/* used for the terminals, and HPGL for the     */
/* plotters. Raster graphics is used for	*/
/* printer graphics.				*/
/* The type of display graphics is defined by   */
/* the environment variable "graphics", which   */
/* must be present. The type of plotter is      */
/* defined by the global variable "plotter"     */
/* Graphics display and plotting use identical  */
/* function calls. Plotting is started by a     */
/* call to "setplotter()". Subsequent graphics  */
/* calls are then routed to the plotter.        */
/* Screen graphics is started by a call to      */
/* "setdisplay()". Subsequent graphics calls    */
/* are then routed to the screen. A global      */
/* variable plot is set to 1 during plotting    */
/* and to 0 during screen display.		*/
/*						*/
/* Important note concerning VMS programming:   */
/*   Those plotters which are Raster oriented   */
/*   for example, the LaserJet 150, are not	*/
/*   supported under VMS at this time, as the	*/
/*   rasterization calls upon Sun-written	*/
/*   routines not available on VMS.  These	*/
/*   have raster = 1 or 2.  Raster = 3 or 4	*/
/*   represent PostScript devices, which ARE    */
/*   available under VMS.			*/
/*						*/
/* This version includes the Tek 4x05 and 4x07  */
/* terminals. 					*/
/************************************************/
#include "vnmrsys.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define FILE_IS_GRAPHICS
#include "graphics.h"
#undef FILE_IS_GRAPHICS
#ifdef VNMR
#include "group.h"
#endif 
#include "pvars.h"
#include "wjunk.h"

#ifdef UNIX
#include <sys/file.h>
#endif 


#ifdef  DEBUG
extern int      Tflag;
#define TPRINT0(str) \
	if (Tflag) fprintf(stderr,str)
#define TPRINT1(str, arg1) \
	if (Tflag) fprintf(stderr,str,arg1)
#define TPRINT2(str, arg1, arg2) \
	if (Tflag) fprintf(stderr,str,arg1,arg2)
#define TPRINT3(str, arg1, arg2, arg3) \
	if (Tflag) fprintf(stderr,str,arg1,arg2,arg3)
#define TPRINT4(str, arg1, arg2, arg3, arg4) \
	if (Tflag) fprintf(stderr,str,arg1,arg2,arg3,arg4)
#else 
#define TPRINT0(str) 
#define TPRINT1(str, arg1) 
#define TPRINT2(str, arg1, arg2) 
#define TPRINT3(str, arg1, arg2, arg3) 
#define TPRINT4(str, arg1, arg2, arg3, arg4) 
#endif 

extern int active;                      /* Required for Tektronix */

#ifdef VNMRJ
extern int get_vj_overlayType();
extern int getFrameID();
extern void preset_graphics_font(char *fontName);
#endif
extern void Wgmode();
extern int  WisSunColor();
extern int  Wistek();
extern void sun_rast(short *src, int pnts, int times, int x, int y);

#define	C_A		0x01	/* used in white select	*/
#define	C_L		0x0c	/* clear memory		*/
#define	C_P		0x10	/* used in black select	*/
#define	C_X		0x18	/* exit graphic mode	*/
#define	C_Y		0x19	/* graphics erase cmd	*/
#define	GRAF		0x1d	/* enter graphics mode	*/
#define	ESC		0x1b	/* escape character	*/
#define	C__		0x1f	/* enter alphagraphics	*/
#define	HP7550_steps    40.0    /* plotter steps per mm */

/*  At this time the application has a choice of 48 colors on the
    Tektronix screen (0 - 47).  Because the Tektronix hardware
    has only 16 colors, different application colors will map to
    the same Tektronix color index.  Fortunately, this is not
    very difficult as the application actually uses 3 sets of
    16 hues and considerable overlap exists between the 3 sets.

    For raster displays (2D spectra), the "change_color" routine
    specifies whether a particular color is visible or not,
    depending on whether the associated intensity is above the
    threshold.  Thus a second Tektronix color index array is
    required to specify whether a particular color is visible.

    On the SUN color console, colors 48 and above are grey tones.
    These are NOT supported on the Tektronix.			*/

#define NUM_TEK_COLORS	48

#define OVERLAID_ALIGNED 3

#define  iabs( x )	(((x) < 0) ? -(x) : (x))

static char tek_colors[ NUM_TEK_COLORS ] = {
    '0', '5', '9', '6', '5', '1', ':', '9',
    '9', '7', '8', '2', '2', '=', '6', '4',
    '0', '0', '6', '<', '4', ';', '5', ':',
    '3', '9', '7', '8', '2', '=', '6', '<',
    '1', '3', ':', '5', ';', '4', '<', '6',
    '0', '3', '9', '7', '8', '2', '6', '1',
};
static char tek_dispclr[ NUM_TEK_COLORS ] = {
    '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '0', '0',
};

static int sa,sb,sc,sd,se;
static int fullWindow = 0;
static int saveSpecData = 0;
static double showFields = 1;
int     xgrx,xgry,g_color,g_mode;
int	ygraphoff;
int     mnumxpnts,mnumypnts,ymin,ymultiplier;
int     xcharpixels, ycharpixels; // font size of axis number
int     char_width,char_height,char_ascent,char_descent; // other font size
int     aip_mnumxpnts, aip_mnumypnts, aip_xcharpixels, aip_ycharpixels, aip_ymin;
int     plot;		/* 0 = graphics display, 1 = plotting */
int     in_plot_mode;	/* 0 = graphics mode, 1 = plotting */
int	raster;		/* 1 = raster graphics on, 2= reversed raster */
int     right_edge;  /* size of margin on a graphics terminal */
int     left_edge;
int     top_edge;
int     bottom_edge;
int     BACK;
short   xorflag;
double  wcmax,wc2max;
double  ppmm;
double  yppmm;
char *AxisNumFontName = "AxisNum";
char *AxisLabelFontName = "AxisLabel";
char *DefaultFontName = "Default";


#define MAXSHADES 65
#define SHADESIZE 8
int gray_matrix[SHADESIZE][SHADESIZE] =
    {  {49,18,62,16,47,24,56,17}, {12,34, 1,39, 8,33, 4,61},
       {44,10,40,28,52,21,36,14}, {19,63,45, 0,25,51, 5,42},
       {43, 7,32,35,57,22,27,55}, {29,53,20,46, 6,60,41,11},
       {54, 3,50, 9,38,31, 2,37}, {13,59,23,58,30,15,48,26} };
/*
  { 0,32,8,40,2,34,10,42,48,16,56,24,50,18,58,26,
    12,44,4,36,14,46,6,38,60,28,52,20,62,30,54,22,
    3,35,11,43,1,33,9,41,51,19,59,27,49,17,57,25,
    15,47,7,39,13,45,5,37,63,31,55,23,61,29,53,21 };
*/

/*  Allocate space for YBAR display.  */

static char ybars_buf[ 1026 ];

/*  The following three globals keep track of the display buffer
    for graphical output.					*/

static char	*curBufPtr  = NULL;
static char	*curBufBase = NULL;
static int	curBufSize  = 0;
static int coordinate(int x, int y);
int sun_window();
int _setdisplay_sub();

/*  Use this routine instead of printf to effect graphical output
    to the GraphOn, HDS and Tektronix terminals.

    If no graphics buffer is defined, then the characters are sent
    directly to stdout.

    Otherwise, the routine uses vsprintf() to obtain the exact
    string of characters to be sent.  If insufficent space exists
    to hold the additional characters, then those already present
    are sent first and the buffer pointer is reset to the base.

    If insufficent space still exists, the characters are sent
    directly to stdout; otherwise they are stored in the buffer.  */

static void Gprintf(char *format, ...)
{
    va_list     vargs;
    char	tbuf[ 2050 ];
    int 	sp_used, tlen;

    va_start(vargs, format);
    if (curBufPtr == NULL)
      vfprintf(stdout,format,vargs);
    else
    {
	vsprintf( &tbuf[ 0 ], format, vargs );
	tlen = strlen( &tbuf[ 0 ] );
	sp_used = curBufPtr - curBufBase;

/*  Leave room for the NUL charactrer in the graphics buffer.  */

	if (sp_used + tlen >= curBufSize)
	{
	    *curBufPtr = '\0';
#ifdef UNIX
	    printf( "%s", curBufBase );
	    fflush( stdout );
#else 
	    vms_outputchars( curBufBase, curBufPtr-curBufBase );
#endif 
	    curBufPtr = curBufBase;
	    sp_used = 0;
	}

	if (tlen >= curBufSize)
#ifdef UNIX
	{
	    printf( "%s", &tbuf[ 0 ] );
	    fflush( stdout );
	}
#else 
	  vms_outputchars( &tbuf[ 0 ], strlen( &tbuf[ 0 ] ) );
#endif 
	else
	{
	    *curBufPtr = '\0';
	    strcat( curBufPtr, &tbuf[ 0 ] );
	    curBufPtr += tlen;
	}
    }
    va_end(vargs);
}

static void
GinitBufPtr(char *bptr, int size )
{
    fflush(stdout);			/* Send out that which is present */
    curBufPtr  = curBufBase = bptr;
    curBufSize = size;
}

static void
GoutputBuf()
{
    if ( curBufPtr > curBufBase ) {
	*curBufPtr = '\0';
#ifdef UNIX
	printf( "%s", curBufBase );
	fflush( stdout );
#else 
	vms_outputchars( curBufBase, curBufPtr-curBufBase );
#endif 
    }

    curBufPtr = curBufBase;
}

static void
GquitBuf()
{
    if ( curBufPtr > curBufBase ) {
	*curBufPtr = '\0';
#ifdef UNIX
	printf( "%s", curBufBase );
	fflush( stdout );
#else 
	vms_outputchars( curBufBase, curBufPtr-curBufBase );
#endif 
    }

    curBufSize = 0;
    curBufBase = curBufPtr = NULL;
}

void
init_ParameterLine()
{
#ifdef VNMRJ
    if (P_getreal(GLOBAL,"mfShowFields" ,&showFields, 1))
      showFields = 1;
    if(getFrameID() > 1) showFields = 0;
#endif
}

/**************************************************/
/*						  */
/* setdisplay: initialize for graphics display    */
/* set plot=0 and set up the following variables: */
/* mnumxpnts	total number of points, x axis    */
/* mnumypnts    total number of points, y axis    */
/* xcharpixels	number of pixels per char, x axis */
/* ycharpixels  number of pixels per char, y axis */
/* ymin		zero baseline above bottom	  */
/*		(leaves room for scale		  */
/* ygraphoff	leaves two lines parameter display*/
/* right_edge   size of right hand margin         */
/*						  */
/**************************************************/
int
setdisplay()
{
   in_plot_mode = 0;
   return( (*_setdisplay)() );
}
#ifdef SUN
int
sun_setdisplay()
{
    ymultiplier = 1;
    BACK = 0;
    sun_window();
    return ( _setdisplay_sub() );
}
#endif 
#ifndef SIS
int
graphon_setdisplay()
{
    mnumxpnts   = 1000;
    mnumypnts   = 620;
    xcharpixels = 14;
    ycharpixels = 22;
    char_width = 14;
    char_height = 22;
    right_edge  = 46;
    ymultiplier = 2;
    BACK = 0;
    return ( _setdisplay_sub() );
}

int
tek4x05_setdisplay()
{
    mnumxpnts	 = 480;
    mnumypnts	 = 270;
    xcharpixels = 6;
    ycharpixels = 12;
    char_width = 6;
    char_height = 12;
    right_edge  = 28;
    ymultiplier = 1;
    BACK = 0;
    return ( _setdisplay_sub() );
}

int
tek4x07_setdisplay()
{
    mnumxpnts   = 640;
    mnumypnts   = 360;
    xcharpixels = 8;
    ycharpixels = 16;
    char_width = 8;
    char_height = 16;
    right_edge  = 36;
    ymultiplier = 1;
    BACK = 0;
    return ( _setdisplay_sub() );
}

int
hds_setdisplay()
{
    mnumxpnts   = 700;
    mnumypnts   = 280;
    xcharpixels = 9;
    ycharpixels = 12;
    char_width = 9;
    char_height = 12;
    right_edge  = 32;
    ymultiplier = 1;
    BACK = 0;
    return ( _setdisplay_sub() );
}

int
default_setdisplay()
{
    mnumxpnts   = 700;
    mnumypnts   = 280;
    xcharpixels = 9;
    ycharpixels = 12;
    char_width = 9;
    char_height = 12;
    right_edge  = 32;
    ymultiplier = 1;
    BACK = 0;
    return ( _setdisplay_sub() );
}
#endif 

extern int getOverlayMode();
int _setdisplay_sub()
{   int e;
    int org_mnumypnts;
    int textHeight;   // height of GraphText font
    int labelHeight;  // height of AxisLabel font
    // double dv;
    char   fw[4];
    plot = 0;	/* turn off plotting mode */
    raster = 0;
    active_gdevsw = &(gdevsw_array[C_TERMINAL]);
    org_mnumypnts = mnumypnts; 
    textHeight = ycharpixels;
    labelHeight = ycharpixels;
#ifdef VNMRJ
      preset_graphics_font(AxisLabelFontName);
      labelHeight = char_height;
      preset_graphics_font(DefaultFontName);
      textHeight = char_height;
#endif
    ygraphoff   = textHeight;
    mnumypnts  -= ygraphoff;
    xorflag = 0;   /* initialize to normal mode of display */

    /* get chart paper size */
    wcmax = 400.0; wc2max = 205.0;
#ifdef VNMR
    if ( (e=P_getreal(GLOBAL,"wcmax"  ,&wcmax,  1)) )
      { P_err(e,"global ","wcmax:"); }
    if ( (e=P_getreal(GLOBAL,"wc2max" ,&wc2max, 1)) )
      { P_err(e,"global ","wc2max:"); }

    /* show labels below scale? */
    init_ParameterLine();

    /* enough space for labels below scale? */
    // ymin = BASEOFFSET * mnumypnts / (wc2max+BASEOFFSET);

    /**********
    dv = 0.0;
    P_getreal(CURRENT,"arraydim",&dv,1);
    if (dv > 1.0)
       ymin = 3*ycharpixels;
    else
       ymin = 3*ycharpixels;
    **********/
    ymin = ycharpixels * 2 + labelHeight;
    if (showFields > 0) {
       if (getOverlayMode() == OVERLAID_ALIGNED) {
           // ygraphoff += 4*ycharpixels - ymin;
           // mnumypnts -= 4*ycharpixels - ymin;
           e = ycharpixels + labelHeight + textHeight * 2;
	   ygraphoff += e - ymin;
           mnumypnts -= e - ymin;
       } else {
	   // ygraphoff += 3*ycharpixels - ymin;
           // mnumypnts -= 3*ycharpixels - ymin;
           e = labelHeight + textHeight * 2;
	   ygraphoff += e - ymin;
           mnumypnts -= e - ymin;
       }
    }
    else {
       ygraphoff = 0;
       mnumypnts = org_mnumypnts;
    }

    // add 4mm to ymin
    ymin += (int)(4*(mnumypnts-ymin) / wc2max);

    /* preserve aspect ratio for plotting */
    fullWindow = 0;
    if (P_getstring(GLOBAL, "wysiwyg", fw, 1, 2) == 0)
    {
	if (fw[0] == 'n' || fw[0] == 'N')
	    fullWindow = 1;
    }
    if (!fullWindow || !Wissun())
    {
      if (((float)(mnumxpnts-right_edge)/(float)mnumypnts) >=
                                 (wcmax/(wc2max+BASEOFFSET)))
         mnumxpnts = (int)((float)mnumypnts*wcmax /
                                 (wc2max+BASEOFFSET))+right_edge;
      else
         mnumypnts = (int)((float)(mnumxpnts-right_edge) *
                                 (wc2max+BASEOFFSET)/wcmax);
    }

    mnumypnts   /= ymultiplier;
    ygraphoff   /= ymultiplier;
    ycharpixels /= ymultiplier;

#endif 
    ppmm = (mnumxpnts-right_edge)/wcmax;
    yppmm = (mnumypnts - ymin) / wc2max;

   /***
    if (fullWindow && Wissun())
        yppmm = (mnumypnts - ymin) / wc2max;
    else
        yppmm = ppmm;
    ***/

    e = (int) (yppmm * SCALE_Y_OFFSET);
    if (e < 2)
       e = 2;
    ymin += e;
    ymin        /= ymultiplier;

    yppmm = (mnumypnts - ymin) / wc2max;

    return 0;
}
/***************/
int
show_plotterbox()
/***************/
{ 
#ifdef VNMRJ
  return 0;
#else

  Wgmode();
  (*(*active_gdevsw)._color)(BLUE,BLUE);
  (*(*active_gdevsw)._amove)(0,mnumypnts/20);
  (*(*active_gdevsw)._adraw)(0,0);
  (*(*active_gdevsw)._adraw)(mnumxpnts/40,0);
  (*(*active_gdevsw)._amove)(39*mnumxpnts/40-right_edge,0);
  (*(*active_gdevsw)._adraw)(mnumxpnts-1-right_edge,0);
  (*(*active_gdevsw)._adraw)(mnumxpnts-1-right_edge,mnumypnts/20);
  (*(*active_gdevsw)._amove)(39*mnumxpnts/40-right_edge,mnumypnts-1);
  (*(*active_gdevsw)._adraw)(mnumxpnts-1-right_edge,mnumypnts-1);
  (*(*active_gdevsw)._adraw)(mnumxpnts-1-right_edge,19*mnumypnts/20);
  (*(*active_gdevsw)._amove)(mnumxpnts/40,mnumypnts-1);
  (*(*active_gdevsw)._adraw)(0,mnumypnts-1);
  (*(*active_gdevsw)._adraw)(0,19*mnumypnts/20);
#endif
}


/****************************************************/
/*						    */
/* endgraphics: finish graphics display or plotting */
/*						    */
/****************************************************/
int
endgraphics()
{
   return ( (*(*active_gdevsw)._endgraphics)() );
}

int
default_endgraphics() { return 0; }

/***************************************************/
/*						   */
/* graf_clear: clear graphics plane and enter      */
/* graphics mode, if in display mode. Do nothing,  */
/* if in plot mode.				   */
/*						   */
/***************************************************/
int
graf_clear()
{
   return ( (*(*active_gdevsw)._graf_clear)() );
}

void
default_graf_clear()
{  
   Wclear(2);
}

int
sunGraphClear()
{
#ifdef SUN
   return ( (*_sunGraphClear)() );
#else
   return 0;
#endif 
}


/******/
int color(int c)
/******/
{
   return ( (*(*active_gdevsw)._color)(abs(c),c) );
}

#ifndef SIS
/******************/
void graphon_hds_color(int c, int dum)
/******************/
{
  (void) dum;
  g_color = c;
  if (g_color<0) g_color=0;
  if ((c==BACK) || (c==BLACK))
    Gprintf("%c%c",ESC,16);	/* Set data off */
  else
    Gprintf("%c%c",ESC,1);	/* Set data on */
  if (xorflag)
    Gprintf("%c\025",ESC);
}

/**********/
void
tek_color(int c, int dum)
/**********/
{ int tindex;		/*  TINDEX is the TEK color index */

  (void) dum;
  g_color = c;
  if (g_color<0) g_color=0;
  if ((c==BACK) || (c==BLACK))
    tindex = 0;				/*  Black */
  else if (c >= NUM_TEK_COLORS)
    tindex = 1;				/*  White */
  else tindex = c;
  Gprintf("%cML%c",ESC,tek_colors[ tindex ]); 	/*  Line color */
  Gprintf("%cMT%c",ESC,tek_colors[ tindex ]); 	/*  Text color */
}

#endif 

/*********/   /* no sun provision yet */
int
charsize(double f)
/*********/
{
   return ( (*(*active_gdevsw)._charsize)(f) );
}

int
fontsize(double f)
{
   return ( (*(*active_gdevsw)._fontsize)(f) );
}


/**************************************/
/* move to x,y without drawing a line */
/* IMPORTANT NOTE:  This routine puts */
/* the GraphOn in graphics mode, even */
/* if Wgmode() has not been called    */
/**************************************/
int amove(int x, int y)
{
   return ( (*(*active_gdevsw)._amove)(x,y) );
}

/**************************************/
/* IMPORTANT NOTE:  This routine puts */
/* the GraphOn in graphics mode, even */
/* if Wgmode() has not been called    */
/**************************************/
int
graphon_hds_amove(int x, int y)
{ xgrx=x; /* save x and y coordinates */
  xgry=y;  /* sun just uses these */
  Gprintf("%c",GRAF); /* move character */
  sa=sb=sc=sd=0;	/* force all 4 chars to be send */
  coordinate(x,y);	/* move to current position */
  return 0;
}

int
tek_amove(int x, int y)
{ xgrx=x; /* save x and y coordinates */
  xgry=y;  /* sun just uses these */
  if (active != 2)                /* If the Tek terminal is not in */
  {                               /* graphics mode, it will be now */
    Gprintf( "%c%%!0", ESC );
    active = 2;
  }
  sa=sb=sc=sd=se=0;       /* force all 5 chars to be sent */
                          /* with next draw command.      */
  return 0;
}

int
default_amove(int x, int y)
{ xgrx=x; /* save x and y coordinates */
  xgry=y;  /* sun just uses these */
  return 0;
}

/****************************************************************/
/* move x,y relative to current position without drawing a line */
/****************************************************************/
int rmove(int x, int y)
{
  (*(*active_gdevsw)._amove)(xgrx+x,xgry+y);
  return 0;
}

/************************************************/
/* calculate and send the x,y coordinates in	*/
/* Tektronics code				*/
/************************************************/
static int coordinate(int x, int y)
{ return (*_coord0)(x,ymultiplier*(y+ygraphoff));
}

#ifdef XXX
static int coord0(int x, int y)
{
   return ( (*_coord0)(x,y) );
}
#endif

int graphon_hds_coord0(int x, int y)
{
    register int yy;
    int	yhigh,ylow,xhigh,xlow;

    /* add vertical offset for old style knob labels */
    yy = y;

/*  GraphOn, HDS accept Tek 4010 style 10 bit codes, with a
    maximum of 4 characters per x, y position.		*/

    yhigh=0x20+((yy>>5)&0x1f);	/* y-high */
    ylow=0x60+(yy&0x1f);		/* y_low  */
    xhigh=0x20+((x>>5)&0x1f);	/* x-high */
    xlow=0x40+(x&0x1f);		/* x-low  */
    /* determine most efficient method of going to coordinate */
    if (sa!=yhigh) 
    {   if (sc!=xhigh)
	   Gprintf("%c%c%c%c",yhigh,ylow,xhigh,xlow);
	else
	if(sb!=ylow)
	   Gprintf("%c%c%c",yhigh,ylow,xlow);
	else
	   Gprintf("%c%c",yhigh,xlow); 
    }
    else
    {   if(sc!=xhigh)
	 Gprintf("%c%c%c",ylow,xhigh,xlow);
         else
	 {   if(sb!=ylow)
	        Gprintf("%c%c",ylow,xlow);
	     else
	        Gprintf("%c",xlow);
         }
    }
    sa=yhigh; sb=ylow; sc=xhigh; sd=xlow;		/* save last values */
    return 0;
}

int tek_coord0(int x, int y)
{
    int	yhigh,ylow,xhigh,xlow,extra;

/*  Tektronix terminals can accept 12 bit codes, with a
    maximum of 5 characters per x, y position.		*/
    yhigh=0x20+((y>>7)&0x1f);		/* y_high */
    extra=((y&3)<<2)+(x&3)+0x60;		/* extra  */
    ylow=0x60+((y>>2)&0x1f);		/* y_low  */
    xhigh=0x20+((x>>7)&0x1f);		/* x_high */
    xlow=0x40+((x>>2)&0x1f);		/* x_low  */

    if (sa!=yhigh)
    {
       if (sc!=xhigh)
       {
	  Gprintf( "%c%c%c%c%c", yhigh, extra, ylow, xhigh, xlow );
       }
       else
       {
	  Gprintf( "%c%c%c%c", yhigh, extra, ylow, xlow );
       }
    }
    else
    {
       if (sc!=xhigh) {
	  Gprintf( "%c%c%c%c", extra, ylow, xhigh, xlow );
       }
       else {
	  Gprintf( "%c%c%c", extra, ylow, xlow );
       }
    }
    sa=yhigh; sb=ylow; sc=xhigh; sd=xlow; se=extra;
    return 0;
}

int
default_coord0(int x, int y)
{
   (void) x;
   (void) y;
   return 0;
}

/********************************************/
/* draw a line relative to current position */
/********************************************/
int rdraw(int x, int y)
{
   return ( (*(*active_gdevsw)._rdraw)(x,y) );
}

#ifndef SIS
int
default_rdraw(int x, int y)
{
   (*(*active_gdevsw)._adraw)(x+xgrx,y+xgry);
   return 0;
}
#endif 

/************************************/
/* draw a line to absolute position */
/************************************/
int adraw(int x, int y)
{
   return ( (*(*active_gdevsw)._adraw)(x,y) );
}

#ifndef SIS
int
graphon_hds_adraw(int x, int y)
{
   xgrx = x; xgry = y;
   coordinate(xgrx,xgry);	/* draw to new  point */
   return 0;
}

int
tek_adraw(int x, int y)
{
  int		incre, iter, limit, new_x, new_y, old_x, old_y, use_x;

/*  Take the easy way out, if possible.  */

     if (x-xgrx == 0 || y-xgry == 0) {
	     Gprintf( "%cRR", ESC);
	     coordinate( xgrx, xgry );
	     coordinate( x, y );
	     if ((g_color==BLACK) || (g_color==BACK))
	       Gprintf( "0" );
	     else if (g_color >= NUM_TEK_COLORS)
	       Gprintf( "1" );
	     else
	     Gprintf( "%c", tek_colors[ g_color ] );
     }

/*  Have to 'rasterize' the vector into a series of
 vertical or horizontal vectors.			*/

     else {
	     use_x = iabs( x-xgrx ) < iabs( y-xgry );
	     if (use_x) {
		     limit = iabs( x-xgrx ) + 1;
		     incre = ( x < xgrx ) ? -1 : 1;
	     }
	     else {
		     limit = iabs( y-xgry ) + 1;
		     incre = ( y < xgry ) ? -1 : 1;
	     }

	     old_x = xgrx;
	     old_y = xgry;

/*  For each pixel in the shorter direction...  */

	     for (iter = 0; iter < limit; iter++) {
		     if (use_x) {
			     new_x = old_x;
			     new_y = (y-xgry) * (iter+1) / limit + xgry;
		     }
		     else {
			     new_x = (x-xgrx) * (iter+1) / limit + xgrx;
			     new_y = old_y;
		     }
		     Gprintf( "%cRR", ESC );
		     coordinate( old_x, old_y );
		     coordinate( new_x, new_y );
		     if ((g_color==BLACK) || (g_color==BACK))
		       Gprintf( "0" );
		     else if (g_color >= NUM_TEK_COLORS)
		       Gprintf( "1" );
		     else
		       Gprintf( "%c", tek_colors[ g_color ] );
		     if (use_x) {
			     old_x += incre;
			     old_y = new_y;
		     }
		     else {
			     old_x = new_x;
			     old_y += incre;
		     }
	     }
     }

/*  Update internal position values.  */

     xgrx = x;
     xgry = y;
     return 0;
}

int
default_adraw(int x, int y)
{
   (void) x;
   (void) y;
   return 0;
}
#endif 

/***************************************************/
/* display or plot a character at present position */
/***************************************************/
int dchar(char ch)
{
   return ( (*(*active_gdevsw)._dchar)(ch) );
}

#ifndef SIS
int
graphon_hds_dchar(char ch)
{
   Gprintf("%c%c",C__,ch);	/* alpha graphics mode	*/
   if (xorflag)
      Gprintf("%c\025",ESC);
   xgrx += xcharpixels;	/* char inc. x-position	*/
   return 0;
}

int
tek4x05_dchar(char ch)
{

/*  Tektronix.  The LF command requires Graphics coordinates,
    rather than Pixel coordinates.  The X and Y values have to
    be multiplied by the indicated factors.  Prevent any further
    modification by using the COORD0 routine.  The LF command is
    necessary to position the Graphics beam for the LT command.  */

   Gprintf( "%cLF", ESC );
   (*_coord0)( ((xgrx+2)*128)/15,
	   (ymultiplier*(xgry+ygraphoff)*392)/45
	   );

   Gprintf( "%cLT1%c", ESC, ch );
   xgrx += xcharpixels;	/* char inc. x-position	*/
   return 0;
}

int
tek4x07_dchar(char ch)
{

/*  Tektronix.  The LF command requires Graphics coordinates,
    rather than Pixel coordinates.  The X and Y values have to
    be multiplied by the indicated factors.  Prevent any further
    modification by using the COORD0 routine.  The LF command is
    necessary to position the Graphics beam for the LT command.  */

   Gprintf( "%cLF", ESC );
   (*_coord0)( ((xgrx+2)*32)/5,
	   (ymultiplier*(xgry+ygraphoff)*98)/15
	 );

   Gprintf( "%cLT1%c", ESC, ch );
   xgrx += xcharpixels;	/* char inc. x-position	*/
   return 0;
}

int
default_dchar(char ch)
{
   (void) ch;
   // xgrx += xcharpixels;	/* char inc. x-position	*/
   xgrx += char_width;	/* char inc. x-position	*/
   return 0;
}
#endif 

/************************************************/
/* display or plot a string at present position */
/************************************************/
int dstring(char *s)
{
   return ( (*(*active_gdevsw)._dstring)(s) );
}

#ifndef SIS
int
default_dstring(char *s)
{ int i;
   i = 0;
   while (s[i]) 
      (*(*active_gdevsw)._dchar)(s[i++]);
   return 0;
}
#endif 

/************************************************************/
/* display or plot a vertical character at present position */
/************************************************************/
int dvchar(char ch)
{
   return ( (*(*active_gdevsw)._dvchar)(ch) );
}

#ifndef SIS
int
graphon_hds_dvchar(char ch)
{
    xgry += xcharpixels;
    Gprintf("%c%c",C__,ch);	/* alpha graphics mode	*/
    if (xorflag)
      Gprintf("%c\025",ESC);
   return 0;
}

int
tek4x05_dvchar(char ch)
{
   xgry += xcharpixels;
   Gprintf( "%cMRE:0", ESC );
   Gprintf( "%cLF", ESC );
   (*_coord0)( ((xgrx+2)*128)/15,
	    (ymultiplier*(xgry+ygraphoff)*392)/45
	 );

   Gprintf( "%cLT1%c", ESC, ch );
   Gprintf( "%cMR00", ESC );
   return 0;
}

int
tek4x07_dvchar(char ch)
{
  xgry += xcharpixels;
  Gprintf( "%cMRE:0", ESC );
  Gprintf( "%cLF", ESC );
  (*_coord0)( ((xgrx+2)*32)/5,
	  (ymultiplier*(xgry+ygraphoff)*98)/15
	);

  Gprintf( "%cLT1%c", ESC, ch );
  Gprintf( "%cMR00", ESC );
   return 0;
}

int
default_dvchar(char ch)
{
   (void) ch;
   // xgry += xcharpixels;
   xgry += char_height;
   return 0;
}
#endif 

/*********************************************************/
/* display or plot a vertical string at present position */
/*********************************************************/
int dvstring(char *s)
{
   return ( (*(*active_gdevsw)._dvstring)(s) );
}

int vstring(char *s)
{
   return ( (*(*active_gdevsw)._vstring)(s) );
}

int
graphon_hds_dvstring(char *s)
{ int i,l;
  i = 0;
  l = strlen(s);
  for (i=l-1; i>=0; i--)
    { (*(*active_gdevsw)._dvchar)(s[i]);
      (*(*active_gdevsw)._amove)(xgrx,xgry);
    }
  return 0;
}

int
default_dvstring(char *s)
{
   (void) s;
   return 0;
}

int
default_vstring(char *s)
{
   (void) s;
   return 0;
}

int
graf_no_op()
{ return 0; }

/********************/
void clear_ybar(register struct ybar *ybar_ptr, register int n)  /* blanks a region of ybars */
/********************/
{ while (n--)
    { ybar_ptr->mn = 1;
      ybar_ptr->mx = 0;
      ybar_ptr++;
    }
}

/***************************************/
void ybars(int dfpnt, int depnt, struct ybar *out, int vertical, int maxv, int minv)  /* draws a spectrum */
/***************************************/
{
    if (out) {
	(*(*active_gdevsw)._ybars)(dfpnt,depnt,out,vertical,maxv,minv);
    }
}

/*
 *  Bound the ybars between the maxv and minv values.
 *  Special care is taken for ybars which are cleared as in
 *  integral blanking.  In that case,  the value of ybar.mx
 *  is less than ybar.mn
 */
/***********************************************/
void range_ybar(register int dfpnt, register int depnt, register struct ybar *out,
                register int maxv, register int minv)
/***********************************************/
{
   register int i;

   for (i=dfpnt; i<depnt; i++)
      if (out[i].mx >= out[i].mn)
      { if (out[i].mx>maxv)
	{ out[i].mx = maxv; 
	  if (out[i].mn>maxv) out[i].mn=maxv; 
        }
        if (out[i].mn<minv)
	{ out[i].mn = minv;
	  if (out[i].mx<minv) out[i].mx=minv;
        }
      }
}

void
default_ybars()
{}

#ifndef SIS
/*******************************************/
void
tek_ybars(int dfpnt, int depnt, struct ybar *out, int vertical, int maxv, int minv)  /* draws a spectrum */
/*******************************************/
{ register int i,j,k;
	
  /* first check vertical limits */
  if (maxv)
    range_ybar(dfpnt,depnt,out,maxv,minv);
  Wgmode();
  GinitBufPtr( &ybars_buf[ 0 ], 1024 );
  if (xorflag)
     Gprintf("%cRU!74",ESC);
 
  i = dfpnt;
  while (i<depnt)
    { /* skip any blank area */
      while ((out[i].mx<out[i].mn)&&(i<depnt))
	i++;
      if (i<depnt)
	{ if (out[i].mn==out[i].mx)
	    { j=i;
	      i++;
	      /* first check for min=max and find vector */
	      /* vector is less than 45 degree up or down */
	      while ((out[i].mn==out[i].mx) && (i<depnt)
		  &&(abs(out[(i+j)>>1].mn-((out[j].mn+out[i].mn)>>1))<=1))
		  i++;
	      i--;
	      if (vertical)
		{ (*(*active_gdevsw)._amove)(mnumxpnts-out[j].mn,j+1);
		  (*(*active_gdevsw)._adraw)(mnumxpnts-out[i].mn,i+1);
		}
	      else
		{ (*(*active_gdevsw)._amove)(j+1,out[j].mn);
		  (*(*active_gdevsw)._adraw)(i+1,out[i].mn);
		}
	    }
	  else if ((i<depnt-1) && (out[i].mx==out[i+1].mn-1))
	    /* find vector more than 45 degree up */
	    { j=i;
	      k=out[j].mx-out[j].mn;
	      i++;
	      while ((out[i-1].mx==out[i].mn-1) && (i<depnt)
		  &&(out[i].mx>=out[i].mn)
		  &&(abs(out[i].mx-out[i].mn-k)<=1))
		  i++;
	      i--;
	      if (vertical)
		{ (*(*active_gdevsw)._amove)(mnumxpnts-out[j].mn,j+1);
		  (*(*active_gdevsw)._adraw)(mnumxpnts-out[i].mx,i+1);
		}
	      else
		{ (*(*active_gdevsw)._amove)(j+1,out[j].mn);
		  (*(*active_gdevsw)._adraw)(i+1,out[i].mx);
		}
	    }
	  else if ((i<depnt-1) && (out[i].mn==out[i+1].mx+1))
	    /* find vector more than 45 degree down */
	    { j=i;
	      k=out[j].mx-out[j].mn;
	      i++;
	      while ((out[i-1].mn==out[i].mx+1) && (i<depnt)
		  &&(out[i].mx>=out[i].mn)
		  &&(abs(out[i].mx-out[i].mn-k)<=1))
		  i++;
	      i--;
	      if (vertical)
		{ (*(*active_gdevsw)._amove)(mnumxpnts-out[j].mx,j+1);
		  (*(*active_gdevsw)._adraw)(mnumxpnts-out[i].mn,i+1);
		}
	      else
		{ (*(*active_gdevsw)._amove)(j+1,out[j].mx);
		  (*(*active_gdevsw)._adraw)(i+1,out[i].mn);
		}
	    }
	  else /* none of the exceptions */
	    { if (vertical)
		{ (*(*active_gdevsw)._amove)(mnumxpnts-out[i].mn,i+1);
		  (*(*active_gdevsw)._adraw)(mnumxpnts-out[i].mx,i+1);
		}
	      else
		{ (*(*active_gdevsw)._amove)(i+1,out[i].mn);
		  (*(*active_gdevsw)._adraw)(i+1,out[i].mx);
		}
	    }
	}
      i++;
    }
    GquitBuf();
}

/***************************************************/
void
graphon_hds_ybars(int dfpnt, int depnt, struct ybar *out, int vertical, int maxv, int minv)  /* draws a spectrum */
/***************************************************/
{ register int i,j,k;
	
  /* first check vertical limits */
  if (maxv)
    range_ybar(dfpnt,depnt,out,maxv,minv);
  Wgmode();
  GinitBufPtr( &ybars_buf[ 0 ], 1024 );
  if (xorflag)
    Gprintf("%c\025",ESC);
  i = dfpnt;
  while (i<depnt)
    { /* skip any blank area */
      while ((out[i].mx<out[i].mn)&&(i<depnt))
	i++;
      if (i<depnt)
	{ if (out[i].mn==out[i].mx)
	    { j=i;
	      i++;
	      /* first check for min=max and find vector */
	      /* vector is less than 45 degree up or down */
	      while ((out[i].mn==out[i].mx) && (i<depnt)
		  &&(abs(out[(i+j)>>1].mn-((out[j].mn+out[i].mn)>>1))<=1))
		  i++;
	      i--;
	      if (vertical)
		{ (*(*active_gdevsw)._amove)(mnumxpnts-out[j].mn,j+1);
		  (*(*active_gdevsw)._adraw)(mnumxpnts-out[i].mn,i+1);
		}
	      else
		{ (*(*active_gdevsw)._amove)(j+1,out[j].mn);
		  (*(*active_gdevsw)._adraw)(i+1,out[i].mn);
		}
	    }
	  else if ((i<depnt-1) && (out[i].mx==out[i+1].mn-1))
	    /* find vector more than 45 degree up */
	    { j=i;
	      k=out[j].mx-out[j].mn;
	      i++;
	      while ((out[i-1].mx==out[i].mn-1) && (i<depnt)
		  &&(out[i].mx>=out[i].mn)
		  &&(abs(out[i].mx-out[i].mn-k)<=1))
		  i++;
	      i--;
	      if (vertical)
		{ (*(*active_gdevsw)._amove)(mnumxpnts-out[j].mn,j+1);
		  (*(*active_gdevsw)._adraw)(mnumxpnts-out[i].mx,i+1);
		}
	      else
		{ (*(*active_gdevsw)._amove)(j+1,out[j].mn);
		  (*(*active_gdevsw)._adraw)(i+1,out[i].mx);
		}
	    }
	  else if ((i<depnt-1) && (out[i].mn==out[i+1].mx+1))
	    /* find vector more than 45 degree down */
	    { j=i;
	      k=out[j].mx-out[j].mn;
	      i++;
	      while ((out[i-1].mn==out[i].mx+1) && (i<depnt)
		  &&(out[i].mx>=out[i].mn)
		  &&(abs(out[i].mx-out[i].mn-k)<=1))
		  i++;
	      i--;
	      if (vertical)
		{ (*(*active_gdevsw)._amove)(mnumxpnts-out[j].mx,j+1);
		  (*(*active_gdevsw)._adraw)(mnumxpnts-out[i].mn,i+1);
		}
	      else
		{ (*(*active_gdevsw)._amove)(j+1,out[j].mx);
		  (*(*active_gdevsw)._adraw)(i+1,out[i].mn);
		}
	    }
	  else /* none of the exceptions */
	    { if (vertical)
		{ (*(*active_gdevsw)._amove)(mnumxpnts-out[i].mn,i+1);
		  (*(*active_gdevsw)._adraw)(mnumxpnts-out[i].mx,i+1);
		}
	      else
		{ (*(*active_gdevsw)._amove)(i+1,out[i].mn);
		  (*(*active_gdevsw)._adraw)(i+1,out[i].mx);
		}
	    }
	}
      i++;
    }
    GquitBuf();
}
#endif 

static void
save_spec_raw_data(short *buf, int pnx, int onx, int vo)
{
#ifdef VNMRJ
  int   i, v, k;
  char  fpath[MAXPATHL];
  FILE  *fd;

  sprintf(fpath, "%s/spec.dat", userdir);
  fd = fopen(fpath, "w");
  if (fd == NULL)
      return;
  fprintf(fd, "#size  %d\n", pnx);
  fprintf(fd, "#width  %d\n", onx);
  k = 0;
  for (i = 0; i < pnx; i++) {
      v = *buf + vo;
      buf++;
      fprintf(fd, "%d ", v);
      k++;
      if (k >= 10) {
          k = 0;
          fprintf(fd, "\n");
      }
  }
  fclose(fd);
  writelineToVnmrJ("vnmrjcmd", "GRAPHICS2 show specData");
#endif
}


/***************************/
void
expand(short *bufpnt, int pnx, struct ybar *out, int onx, int vo)
/***************************/
{ int ix,ox;
  register int lasty,nexty,thisy,stependi;
  register float cury,curstep;
  register double xstep,stepend;
  double    rnd;

  if (saveSpecData)
     save_spec_raw_data(bufpnt, pnx, onx, vo);
  rnd = 0.1 / (double) onx;
  xstep = (double)(onx-1)/(double)(pnx-1);
  ox = 0;
  stepend = 0.0;
  out->mn = out->mx = thisy = lasty = *bufpnt++ + vo;
  out++;
  for (ix=1; ix<pnx; ix++)
    { stepend += xstep;
      stependi = (int)(stepend + rnd);
      nexty    = *bufpnt++ + vo;
      if (stependi==ox)				/*  Prevent division by 0  */
       curstep = 0.0;				/*  Note that CURSTEP not  */
      else					/*  used if STEPENDI == OX  */
       curstep = (float)(nexty-lasty)/(float)(stependi-ox);
      cury     = thisy;
      do
	{ if (stependi==ox) thisy = nexty;	/*  See!!  */
	  else              thisy = (int)(cury += curstep);
	  if (thisy==lasty)     {out->mn = thisy;   out->mx = thisy; }
	  else if (thisy>lasty) {out->mn = lasty+1; out->mx = thisy; }
	  else                  {out->mn = thisy;   out->mx = lasty-1; }
	  ox++; out++; lasty = thisy;
	}
	while (ox<stependi);
    }
}

/***************************/
void
expand32(int *bufpnt, int pnx, struct ybar *out, int onx, int vo)
/***************************/
{ int ix,ox;
  register int lasty,nexty,thisy,stependi;
  register float cury,curstep;
  register double xstep,stepend;
  double    rnd;

  rnd = 0.1 / (double) onx;
  xstep = (double)(onx-1)/(double)(pnx-1);
  ox = 0;
  stepend = 0.0;
  out->mn = out->mx = thisy = lasty = *bufpnt++ + vo;
  out++;
  for (ix=1; ix<pnx; ix++)
    { stepend += xstep;
      stependi = (int)(stepend + rnd);
      nexty    = *bufpnt++ + vo;
      if (stependi==ox)				/*  Prevent division by 0  */
       curstep = 0.0;				/*  Note that CURSTEP not  */
      else					/*  used if STEPENDI == OX  */
       curstep = (float)(nexty-lasty)/(float)(stependi-ox);
      cury     = thisy;
      do
	{ if (stependi==ox) thisy = nexty;	/*  See!!  */
	  else              thisy = (int)(cury += curstep);
	  if (thisy==lasty)     {out->mn = thisy;   out->mx = thisy; }
	  else if (thisy>lasty) {out->mn = lasty+1; out->mx = thisy; }
	  else                  {out->mn = thisy;   out->mx = lasty-1; }
	  ox++; out++; lasty = thisy;
	}
	while (ox<stependi);
    }
}

/*****************************/
void compress(short *bufpnt, int pnx, struct ybar *out0, int onx, int vo)
/*****************************/
{ register int inx; /* counter through input points */
  register double d; /* counter through input data points */
  register double f; /* compression factor */
  register int v,v1;
  register struct ybar *out;
  register int cnt;

  if (saveSpecData)
     save_spec_raw_data(bufpnt, pnx, onx, vo);
  out = out0;
  f = (double)(onx)/(double)pnx;
  d = 0.0;
  v = *bufpnt + vo;
  cnt = 1;
  out->mn = v; out->mx = v;
  // out->mn = 32767; out->mx = -32767;
  for (inx=0; inx<pnx; inx++)
  {   v = *bufpnt++ + vo;
      if (v>out->mx) out->mx = v;
      if (v<out->mn) out->mn = v;
      d += f;
      if ( (d >= 1.0) && (cnt < onx) ) {
          d -= 1.0;
          /*********
	  v = out->mn; v1 = out->mx;
	  out++;
	  out->mn = v1; out->mx = v;
          *********/

           out++;
           cnt++;
           v1 = *bufpnt + vo;
           if (v1 > v) {
               out->mn = v + 1; out->mx = v1;
           }
           else  {
               if (v1 < v) {
                   out->mn = v1; out->mx = v - 1;
               }
               else {
                   out->mn = v1; out->mx = v1;
               }
           }
        }
    }
}

/********************************/
void fid_expand(register short *bufpnt, int pnx, register struct ybar *out, int onx, int vo)
/********************************/
{ register int ix,ox,stependi;
  register int thisy;
  register double xstep,stepend;
  register int dot_add, dot2;
  register double    rnd;

  ox = dot_add = 0;
  rnd = 0.1 / (double) onx;
  xstep = (double)onx/(double)(pnx < 2 ? 1 : (pnx - 1));
  if ((xstep >= ppmm + 1) && ((ppmm >= 2.0) || (xstep > 2)))
  {
    dot_add = ppmm;
    if (dot_add >= xstep)
      dot_add = xstep - 1;
    if (dot_add < 2)
      dot_add = 2;
    dot2 = dot_add / 2;
    dot_add = 2 * dot2 + 1;
    ox = 0;
    stepend = -dot2 - 1;
    stependi = dot2;
    for (ix=0; ix<pnx; ix++)
    {
      thisy = *bufpnt++ + vo;
      if (stependi > onx)
	  stependi = onx;
      while (ox < stependi)
      {
          out->mn = thisy - dot2;
          out->mx = thisy + dot2;
          ox++;
          out++;
      }
      stepend += xstep;
      stependi = (int) (stepend + rnd);
      if (stependi > onx)
	  stependi = onx;
      while (ox < stependi)
      {
          out->mn = 1;
          out->mx = 0;
	  ox++;
	  out++;
      }
      stependi += dot_add;
    }
  }
  else
    expand(bufpnt,pnx,out,onx,vo);
  TPRINT4("ox= %d dot_add= %d onx= %d xstep= %d\n",ox,dot_add,onx,(int)xstep);
}

/********************************/
int grayscale_box(int oldx, int oldy, int x, int y, int shade)
/********************************/
{
   return ( (*(*active_gdevsw)._grayscale_box)(oldx,oldy,x,y,shade) );
}

#ifndef SIS
/********************************************/
int
graphon_hds_grayscale_box(int oldx, int oldy, int x, int y, int shade)
/********************************************/
{ int shade0,i,j,doit;
  if ((x<1)||(y<1)) return 1;
  xgrx = oldx;
  xgry = oldy;
  shade0 = shade;
  if (shade0>=MAXSHADES) shade0=MAXSHADES-1;
  doit = 0;
  sa=sb=sc=sd=0;	/* force all 4 chars to be send */
  for(i=0; i<x; i++)
    for(j=0; j<ymultiplier*y; j++)
      { if (shade0>(gray_matrix[(xgrx+i)%SHADESIZE][(xgry+j)%SHADESIZE]))
	  { if (!doit)
	      { Gprintf("%c",28); /* plot point mode */
		(*_coord0)(xgrx+i,ymultiplier*(xgry+ygraphoff)+j);
		doit = 1;
	      }
	    else
	      (*_coord0)(xgrx+i,ymultiplier*(xgry+ygraphoff)+j);
	  }
      }
  if (doit)
    Gprintf("%c",29); /* vector mode */
  xgrx += x;
  xgry += y;
  return 0;
}

/****************************************/
int
default_grayscale_box(int oldx, int oldy, int x, int y, int shade)
/****************************************/
{
  (void) shade;
  if ((x<1)||(y<1)) return 1;
  xgrx = oldx;
  xgry = oldy;
  xgrx += x;
  xgry += y;
  return 0;
}
#endif 

/****************************************************/
/* display or plot a box at current graphics cursor */
/* position in the current color                    */
/****************************************************/
int box(int x, int y) 
{
   return ( (*(*active_gdevsw)._box)(x,y) );
}

#ifndef SIS
int
graphon_hds_box(int x, int y) 
{
  Gprintf("\033\002");  /* turn on block fill */
  if ((g_color==BACK) || (g_color==BLACK))
    Gprintf("%c%c",ESC,16);	/* Set data off */
  coordinate(xgrx,xgry); /* send present coordinate */
  (*(*active_gdevsw)._rdraw)(x-1,y-1); /* send coord diagonal corner of box */
  Gprintf("\033\003");  /* turn off block fill */
  return 0;
}

int
tek_box(int x, int y) 
{
    Gprintf("%cRR",ESC);
    coordinate(xgrx,xgry);
    coordinate(xgrx+x-1,xgry+y-1);
    if ((g_color==BACK) || (g_color==BLACK))
     Gprintf( "0" );
    else
    if (g_color < NUM_TEK_COLORS)
     Gprintf( "%c", tek_colors[ g_color ] );
    else
     Gprintf( "1" );
    return 0;
}

int
default_box(int x, int y) 
{
   (void) x;
   (void) y;
   return 0;
}
#endif 

/**********/
int
grf_batch(int on)
/**********/
{
   return ( (*_grf_batch)(on) );
}

#ifndef SIS
/*******************/
int
default_grf_batch(int on)
/*******************/
{
   (void) on;
   return 0;
}
#endif 

#ifdef SUN
int sun_window()
{
   int sunColor_sun_window(), default_sun_window();
   if ( _sun_window == NULL )
   {
      if ( WisSunColor() )
	 _sun_window = sunColor_sun_window;
      else
	 _sun_window = default_sun_window;
   }
   return ( (*_sun_window)() );
}

#endif 

/******************/
int change_contrast(double a, double b)
/******************/
{
   return ( (*_change_contrast)(a,b) );
}

/***************************/
int
default_change_contrast(double a, double b)
/***************************/
{
   (void) a;
   (void) b;
   return 1;
}


/*******************/
int
change_color(int num, int on)
/*******************/
{
   return ( (*_change_color)(num,on) );
}

/**********************/
int
tek_change_color(int num, int on)
/**********************/
{
   if ((num<0)||(num>48)) return 1;
   if (on) 
      tek_dispclr[num]=tek_colors[num];
   else
      tek_dispclr[num]='0';
   return 1;
}

/**************************/
int
default_change_color(int num, int on)
/**************************/
{
   (void) num;
   (void) on;
   return 1;
}

static void encodeTekInt(int ival)
{
	int	i1, i2, i3, jval;

	jval = (ival < 0) ? -ival : ival;
	i1 = (jval >> 10) + 64;
	i2 = ((jval >> 4) % 64) + 64;
	i3 = jval % 16 + 32;
	if ( ival >= 0 ) i3 += 16;

	if ( i1 != 64 ) Gprintf( "%c", i1 );
	if ( i2 != 64 ) Gprintf( "%c", i2 );
	Gprintf( "%c", i3 );
}


/* rast is not in gdevsw - it should be */
/*******************/
void rast(short *src, int n, int times, int x, int y)
/*******************/
{

#ifdef SUN
    if (WisSunColor()) 
    {
	sun_rast(src,n,times,x,y);
    }
    else
#endif 
    if (Wistek()) {
        char *cp;
        int cc,cx,cy,iter,jter,t1;

	GinitBufPtr( &ybars_buf[ 0 ], 1024 );
        cp = (char *) src;
        cx = x;
        cy = y;
        for (jter = 0; jter < times; jter++) {
	    Gprintf( "\033RH" );
	    coordinate( cx, cy );
	    Gprintf( "\033RP" );
	    encodeTekInt( n );
	    encodeTekInt( (n*2+2)/3 );
	    iter = 0;
	    while (iter < n) {
                cc = tek_dispclr[ *(cp++) % 48 ] - '0';
                iter++;
                t1 = cc << 2;
                if (iter < n)
                  cc = tek_dispclr[ *(cp++) % 48 ] - '0';
                else
                  cc = 0;
                iter++;
                t1 += ((cc >> 2) + 32);
                Gprintf( "%c", t1 );
                t1 = (cc & 3) << 4;
                if (iter < n)
                  cc = tek_dispclr[ *(cp++) % 48 ] - '0';
                else
                  cc = 0;
                iter++;
                t1 += (cc + 32);
                if (iter-2 < n)
                  Gprintf( "%c", t1 );
            }
            cy++;
            cp = (char *) src;
            if (jter-1 < times) GoutputBuf();
        }
        GquitBuf();
    } 
}

/****************************************************/
/* report mode of graphical display, XOR or normal  */
/****************************************************/
int GisXORmode()
{
	return( xorflag );
}

/****************************************************/
/* put terminal into the normal mode of display     */
/****************************************************/
int normalmode()
{
   return ( (*(*active_gdevsw)._normalmode)() );
}

#ifndef SIS
int
graphon_hds_normalmode()
{
   xorflag = 0;
   Gprintf("%c",ESC);
   return 1;
}

int
tek_normalmode()
{
   xorflag = 0;
   Gprintf("%cRU!;4",ESC);
   return 1;
}

int
default_normalmode()
{
   xorflag = 0;
   return 1;
}
#endif 

/****************************************************/
/* put terminal into the xor mode of display        */
/****************************************************/
int xormode()
{
   return ( (*(*active_gdevsw)._xormode)() );
}

#ifndef SIS
int
graphon_hds_xormode()
{
  xorflag = 1;
  Gprintf("%c\025",ESC);
   return 1;
}

int
tek_xormode()
{
  xorflag = 1;
  Gprintf("%cRU!74",ESC);
   return 1;
}

int
default_xormode()
{
  xorflag = 1;
   return 1;
}
#endif 

/**************************************/
void ParameterLine(int line, int column, int scolor, char *string)
/**************************************/
{
  char	local_string[ 120 ];
  int l,savecolor, x, y;
  int charsperline;

  if ((line<0)||(line>2)) return;

#ifdef VNMRJ
  if (showFields < 1)
      return;
  if (get_vj_overlayType() >= OVERLAID_ALIGNED)
      return;
#endif

  if (95* char_width > mnumxpnts)
     charsperline  = mnumxpnts / char_width;
  else
     charsperline  = 94;

  if (column+1 > charsperline)		/* beyond right margin? */
     return;			 /* then don't display anything */

  l = strlen(string);

  /*if (column+l>74)
    Werrprintf("ParameterLine: String too long!");*/

  if (l==0)
  {
     l = charsperline-column;	/* erase to end of line, if empty string */
     local_string[ 0 ] = '\0';
  }
  else if (column+l > charsperline)
  {
     l = charsperline-column;	/* truncate strings that are too long */
     strncpy( &local_string[ 0 ], string, l );
     local_string[ l ] = '\0';
  }
  else
     strcpy( &local_string[ 0 ], string );

  x = column * char_width;
  y = (2-line) * char_height - ygraphoff;
  // (*(*active_gdevsw)._amove)(column*xcharpixels,(2-line)*ycharpixels-ygraphoff);
  (*(*active_gdevsw)._amove)(x, y);
  savecolor = g_color;
  if (scolor<0) (*(*active_gdevsw)._color)(-scolor,-scolor);
  else (*(*active_gdevsw)._color)(BACK,BACK);
  (*(*active_gdevsw)._box)(char_width*l, char_height);
  if (scolor<0) (*(*active_gdevsw)._color)(BACK,BACK);
  else (*(*active_gdevsw)._color)(scolor,scolor);
  (*(*active_gdevsw)._amove)(x, y);
/*
  (*(*active_gdevsw)._amove)(column*xcharpixels,
			     (2-line)*ycharpixels-ygraphoff+ycharpixels/5);
*/
  (*(*active_gdevsw)._dstring)(&local_string[ 0 ]);
  (*(*active_gdevsw)._color)(savecolor,savecolor);
}

/*******************/
int usercoordinate(int y)
/*******************/
{
   return ( (*_usercoordinate)(y) );
}

/***********************/
int sun_usercoordinate(int y)
/***********************/
{
    return (mnumypnts-y-1);
}
#ifndef SIS
/***************************/
int default_usercoordinate(int y)
/***************************/
{
    return (y/ymultiplier-ygraphoff+5);
}
#endif 

void show_spec_raw_data(int n)
{
#ifdef VNMRJ
    char cmd[20];

    cmd[0] = '\0';
    Wgetgraphicsdisplay(cmd, 18);

    if (strlen(cmd) > 0) {
        strcat(cmd, "\n");
        saveSpecData = n;
        execString(cmd);
        saveSpecData = 0;
    }
#endif
}

