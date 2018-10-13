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
/************************************************/
/* calcon is now unneeded */

/****
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
***/

#include "vnmrsys.h"
#include "allocate.h"
#include "pvars.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef LINUX
#include <sys/time.h>
#endif

#ifdef __INTERIX
#include <arpa/inet.h>
#else
#ifdef SOLARIS
#include <sys/types.h>
#endif
#include <netinet/in.h>
#ifdef SOLARIS
#include <inttypes.h>
#endif
#endif

#include "graphics.h"
#include "aipGraphics.h"
#include "aipCFuncs.h"
#include "aipCInterface.h"
#include "iplan.h"
#include "wjunk.h"

#include "group.h"
#ifndef INTERACT
#include "buttons.h"
#include "symtab.h"

#endif 

#ifdef UNIX
#include <sys/file.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif 

#ifdef MOTIF
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#else
#include "vjXdef.h"
#endif 

/* #include "sockets.h" */

#define COLORSIZE       256
#define  bool	char		/* The definition used by Sun  */
#define  INIT_WIDTH      600
#define  INIT_HEIGHT     400

#define CMS_VNMRSIZE	(MAX_COLOR+NUM_GRAY_COLORS+1)
#define NOTOVERLAID_NOTALIGNED 0
#define OVERLAID_NOTALIGNED 1
#define NOTOVERLAID_ALIGNED 2
#define OVERLAID_ALIGNED 3
#define STACKED 4
#define UNSTACKED 5
#define ALIGN_1D_X  3
#define ALIGN_1D_Y  4
#define ALIGN_2D 5

extern int     xgrx,xgry,g_color,g_mode;
// extern int     mnumxpnts,mnumypnts,xcharpixels,ycharpixels;
extern int     raster;	/* 1 = raster graphics on, 2= reversed raster */
extern int     right_edge;  /* size of margin on a graphics terminal */
extern short   xorflag;
extern int     BACK;
extern int     raster_resolution,raster_charsize,save_raster_charsize;
extern int     interuption;
extern int     VnmrJViewId;
extern int     grafIsOn, autografIsOn;
/* extern int     javafd; */
extern double  gray_offset;
extern double  graysl,graycntr;
extern FILE   *plotfile;

#ifndef INTERACT
extern char   systemdir[];
extern char   userdir[];
extern char   help_name[];
extern char   fontName[];
extern double get_ploter_ppmm();
extern int   (*mouseMove)();
extern int   (*mouseButton)();
extern int   (*mouseReturn)();
extern int   (*mouseClick)();
extern int   (*mouseGeneral)();
extern int   (*aip_mouseMove)();
extern int   (*aip_mouseButton)();
extern int   (*aip_mouseReturn)();
extern int   (*aip_mouseClick)();
extern int   (*aip_mouseGeneral)();
extern void  Jset_aip_mouse_mode(int on);

#else
static char  systemdir[ MAXPATHL ];
static char  userdir[ MAXPATHL ];
#endif 

extern void  ps_color(int c, int dum);
extern void  ps_ellipse(int x, int y, int w, int h);
extern void  ps_rect(int x, int y, int w, int h);
extern void  ps_round_rect(int x, int y, int w, int h);
extern void  ps_Dpolyline(Dpoint_t *pnts, int npts);
extern void  ps_fillPolygon(Gpoint_t *pnts, int npts);
extern void  ps_arrow(int x, int y, int x2, int y2, int thick);
extern void  ps_flush();
extern int  ps_linewidth(int n);

extern unsigned char *get_red_colors();
extern unsigned char *get_green_colors();
extern unsigned char *get_blue_colors();
extern unsigned char *get_ps_red_colors();
extern unsigned char *get_ps_green_colors();
extern unsigned char *get_ps_blue_colors();

void show_color();
void set_jframe_id(int id);
void set_ybar_style(int n);

#define SYSFONT	 0
#define USERFONT 1
static int raster_plot_color = MONOCHROME;

struct varfontInfo {
	     int  size;
             int  width, height, ascent;
             int  start, last;
             int  order, bytes;
	     int  which; /* userdir or systemdir */
	     time_t mtime;
             char name[12];
             char *data;
	};

#define FONTNUM         3
static struct varfontInfo *varFont[FONTNUM];
static struct varfontInfo *rasterFont = NULL;

static char    *pixmap = NULL;
static int      pixmap_c = 0;  /* total cells in pixmap */
static char    *r_pixmap[] = {NULL, NULL, NULL, NULL};

#ifdef MOTIF
static char    *ximg_data = NULL;
static char    *curFontName = NULL;
static unsigned char    *tmp_src = NULL;
static int      tmp_src_len = 0;
static int      backing_store = 0;
static int 	n_gplanes = 1;
static int 	pix_planes = 0;
static int   	color_num = COLORSIZE;
static int      one_plane = 0;
static int      region_x = 0;
static int      region_y = 0;
static int      region_x2 = 9999;
static int      region_y2 = 9999;
static int      max_color_level = 0;
static int      rast_image_width = 0;
static int      rast_do_flag = 0;
static int      def_lineWidth = 0;
static Pixel    *hPix = NULL;
static Pixel    textColor = 0;
static Pixel    idle_pix = 999;
static Pixel    active_pix = 999;
static Pixmap   charmap = 0;
static Pixmap   bannermap = 0;
static XID  xid2 = 0;
#endif

static int      dynamic_color = 0;
static int      backing_width = 0;
static int      backing_height = 0;
static int      old_winWidth = 0;
static int      old_winHeight = 0;
static int      wait_gin = 0;
static int      is_Open = 0;
static int      is_dconi = 0;
static int	raster_cols = 0;
static int	curwin = 1;
static int	isPrintBg = 0;
static int	transCount = 0;
static int	noXorYbar = 1; // no xor by default
static int     *userFontList = NULL;
static int     *sysFontList = NULL;
static Pixel    xcolor = 0;
static Pixel    sun_colors[COLORSIZE+120];
static Pixel    xwhite = 1;  /* x server white and black color  */
static Pixel    xblack = 0;
static int   	y_cursor_pos[3] = {0, 0, 0};
static int   	x_cursor_pos[3] = {0, 0, 0};
static int   	y_cursor_x2[3] = {0, 0, 0}; /* for crosshair */
static int   	x_cursor_y2[3] = {0, 0, 0}; /* for corsshair */
static int      x_cross_pos = 0;
static int      x_cross_len = 0;
static int      y_cross_pos = 0;
static int      y_cross_off1 = 0;
static int      y2_cross_pos = 0;
static int      y_cross_len = 0;
static int	newCursor = 1;
static int	batch_on = 0;
static int	win_show = 1;
static int	winDraw = 1;
static int	org_winDraw = 1;
static int	isSuspend = 0;
static int	in_batch = 0;
static int	aip_in_batch = 0;
static int	scalable_font = -1;
static int      win_width = INIT_WIDTH;
static int      win_height = INIT_HEIGHT;
static int      main_win_width = INIT_WIDTH;
static int      main_win_height = INIT_HEIGHT;
static int      main_orgx = 0;
static int      main_orgy = 0;
static int      csi_win_width = INIT_WIDTH;
static int      csi_win_height = INIT_HEIGHT;
static int      csi_active = 0;
static int      csi_opened = 0;
static int      grid_rows = 1;
static int      grid_cols = 1;
static int      grid_width = 1;
static int      orgx = -1;
static int      orgy = 0;
static int      csi_orgx = 0;
static int      csi_orgy = 0;
static int      orgx2 = 0;
static int      orgy2 = 0;
static int      org_orgx = 0;
static int      org_orgy = 0;
static int      gx1, gx2;
static int      gy1, gy2;
static int      mx1, mx2;
static int      my1, my2;
static int      *depth_list = NULL;
static int      depth_list_count = 0;
static int      bksflag = 1;
static int      xmapDraw = 1;
static int	sizeChanged = 0;
static int	aipRegion = 0;
static int	aipGrayMap = 0;
static int	batchOk = 0;
static int	ginFunc = 0;
static int	batchCount = 0;
static int	aip_batchCount = 0;
static int	dcon_thTop = 0;
static int	dcon_thBot = 0;
static int	org_dcon_thTop = 0;
static int	org_dcon_thBot = 0;
static int	XGAP = 4;
static int	YGAP = 4;
static int      inPrintSetupMode = 0;
static int      inRsizesMode = 0;
static int      hourglass_on = 0; /* hourglass_on not really necessary? keep as failsafe */
static int      penThick = 1;
static int      rgbAlpha = 100;

Display *xdisplay = NULL;
XID      xid = 0;
int      useXFunc = 0;
int      grid_num = 1;
int      graf_width = INIT_WIDTH;
int      graf_height = INIT_HEIGHT;
int      csi_graf_width = INIT_WIDTH;
int      csi_graf_height = INIT_HEIGHT;
/***  moved to graphics.h
int      char_width = 8;
int      char_height = 12;
int      char_ascent = 15;
int      char_descent = 2;
***/
int      raster_ascent = 2;
int      prtCharWidth = 8;
int      prtCharHeight = 12;
int      prtChar_ascent = 15;
int      prtChar_descent = 2;
int      prtWidth = 2;
int      prtHeight = 2;
int      screen = 0;
int      ibmServer = 0;
int      overlayType = 0;
int      inRepaintMode = 0;
char     bannerFont[32] = "";
char     fontFamily[120];
Colormap xcolormap;
static XID  org_xid = 0;
static XID  aip_xid = 9;
static GC   aip_gc = 0;
static int  win_char_width = 8;
static int  win_char_height = 12;
static int  win_char_ascent = 15;
static int  win_char_descent = 2;
static int  csi_char_width = 8;
static int  csi_char_height = 12;
static int  csi_char_ascent = 15;
static int  csi_char_descent = 2;
static int  rast_gray_shades = 4;
static int  scrnWidth = 100;
static int  scrnHeight = 100;
static int  clearOverlayMap = 1;
static int  overlayNum = 0;
static int  isTrueColor = 0;
static int  isAipMovie = 0;
static int  waitMovieDoneRet = 0;
static int  keepOverlay = 0;
static int  g_color_levels = 1;
static int  isClearBox = 0;
static int  translucent = 0;
static double bannerSize = 1.0;

static int  gray_150[16] = 
		{ 56, 16, 0, 40, 24, 48, 32, 8, 0, 40, 56, 24, 32, 8, 16, 48 };
static int  gray_300[36] = 
		{ 42, 26, 0, 18, 36, 60, 30, 39, 46, 33, 22, 15,
		  12, 53, 56, 50, 8, 4, 18, 36, 60, 42, 26, 0,
		  33, 22, 15, 30, 39, 46, 50, 8, 4, 12, 53, 56 };
static int  gray_600[64] = 
		{ 32, 26, 14, 22, 30, 36, 48, 40, 42, 10, 0, 6, 20, 52, 62, 56,
		  50, 18, 4, 2, 12, 44, 58, 60, 38, 28, 16, 8, 24, 34, 46, 54,
		  30, 36, 48, 40, 32, 26, 14, 22, 20, 52, 62, 56, 42, 10, 0, 6,
		  12, 44, 58, 60, 50, 18, 4, 2, 24, 34, 46, 54, 38, 28, 16, 8 };

int	*raster_gray_matrix;

#define MAXSHADES 65
#define SHADESIZE 8
#define GRAPHCMDLEN 20 
#define CMDLEN 1000

extern int gray_matrix[SHADESIZE][SHADESIZE];

static char *namedColors[] = {"Black", "Red", "Yellow", "Green",
			      "Cyan", "Blue", "Magenta", "White",
			      NULL};
static char *codedColors[] = {"Image Background", "Real FID", "Imaginary FID",
			      "Spectrum", "Integral", "Parameters",
			      "Scale", "Threshold", "Spectrum 2",
			      "Spectrum 3", "Cursor", "Image Foreground",
			      NULL};

static char *aipRedisplay = "aipRedisplay";
static char *defaultMapDir = "default";
static char *MAPNAME = "image.cmap";
static char *iFUNC = "#func";
static char *AIPFUNC = "#aip";

#define GRIDROWMAX 12 
#define GRIDCOLMAX 12 
#define GRIDNUMMAX 64
#define FONTARRAY 5

typedef struct _gcmd {
	int	len;
	int	size;
	int	graphType;
	int	reExec;
	char    *cmd;
	struct  _gcmd *next;
	} GCMD;

typedef struct _vjfont {
         char   *name;  // VJ font name
         char   font[20];  // java font name
         int    widths[FONTARRAY];
         int    heights[FONTARRAY];
         int    ascents[FONTARRAY];
         int    descents[FONTARRAY];
         int    index;
         int    frameW, frameH;
         int    rtWidth, rtHeight;   // adjusted width and height
         int    rtAscent, rtDescent;
         double  ratioAscent; // the ratio of ascent / descent
         double  ratioWH;  // the ratio of width / height
	 time_t  ftime;
	 struct  _vjfont *next;
      } VJFONT;

typedef struct  {
        int     active;
        int     frameId;
        int     isClosed;
	int	x, y;
	int	width, height;
	int	isVertical;   // orientation
        int     old_width, old_height;
        int     fontW, fontH;
        int     ascent, descent; /* font ascent and descent */
        int     axis_fontW, axis_fontH; // font of AxisNum
        int     axis_ascent, axis_descent;
        int     prtX, prtY;
        int     prtW, prtH;
        int     prtFontW, prtFontH;
        int     prtAscent, prtDescent;
	int	graphType;
	int	reExec;
        int     cmdSize;
        char    menu[128];
        char    graphics[64];
        char    graphMode[GRAPHCMDLEN+2];  /* graphicsDisplay */
        Pixmap  xmap;
        int     mouseActive;
        int     mouseJactive;
        int     (*mouseMove)();
        int     (*mouseButton)();
        int     (*mouseReturn)();
        int     (*mouseClick)();
        int     (*mouseGeneral)();
        GCMD    *cmdlist;
        VJFONT  *font;
       } WIN_STRUCT;

typedef struct _prtcmd {
	char    *cmd;
	struct  _prtcmd *next;
      } PRTCMD;

typedef struct {
        int x1, x2;
        int y1, y2;
      } PRTLINEBOX;

typedef struct {
        int    vertical;
        int    start;
        int    end;
        int    maxv, minv;
        int    ybarYpnts;
        int    ybarXpnts;
        struct ybar *points;
      } PRTYBAR;

typedef struct _prtdata {
	int     type;
	int     color;
        union {
           PRTLINEBOX lineBox;
           PRTYBAR specbar;
        } data;
	struct  _prtdata *next;
      } PRTDATA;

// static WIN_STRUCT win_dim[GRIDROWMAX][GRIDCOLMAX];
static WIN_STRUCT win_dim[GRIDNUMMAX];
static int win_dim_ready = 0;
static int frameNums = 0;
static int active_frame = 0;
static int old_active_frame = -1;
static int isNewGraph = 0;
static int isWinRedraw = 0;
static int initWinRedraw = 0;
static int isIgnored = 0;
static int isGraphCmd = 0;
static int isGraphFunc = 0;
static int isClearCmd = 0;
static int isAutoRedraw = 0;
static int isTopframeOn = 0;
static int isTopOnTop = 1;
static int rexecCode;
static int redoCode;
static int originEexecCode = 0;
static int originRedoCode = 0;
static int execDepth = 0;

static WIN_STRUCT *frame_info = NULL;
static WIN_STRUCT *new_info = NULL;
static WIN_STRUCT *main_frame_info = NULL;
static WIN_STRUCT *aip_frame_info = NULL;
static WIN_STRUCT *curActiveWin = NULL;
static PRTDATA *printData = NULL;
static VJFONT *vjFontList = NULL;
static VJFONT *defaultFont = NULL;

extern int Gphsd;
extern char Jvbgname[];
extern void getDscaleFrameInfo();
extern int get2Dflag();
extern int setFullChart(int d2flag);
extern int adjustFull();
extern int traceMode;


#define JLEN	6400
#define KLEN	120
#define YLEN	256

#define JGRAPH	1
#define JCLEAR	1  /* clear java window and image buffer */
#define JXBAR	2
#define JTEXT	3
#define JVTEXT	4
#define JOK	5
#define JRASTER	6
#define JRGB	7
#define JCOLORI	8
#define JLINE  9
#define JXORMODE 10
#define JNORMALMODE 11
#define JBOX    12
#define JCOLORTABLE 13
#define JCNTMAP     14
#define JREFRESH 15
#define JWINBUSY 16
#define JWINFREE 17
#define JBGCOLOR 18
#define JRTEXT 19
#define JRBOX  20
#define JSTEXT 21
#define JSFONT 22
#define JGIN   23
#define JVCURSOR   24
#define JCOLORNAME   26
#define JBATCHON     27
#define JBATCHOFF    28
#define JCOPY        29
#define JCOPY2       30
#define JCLEAR2	     31
#define JBAR         32
#define JYBAR        33  /* new ybar */
#define JVIMAGE      34  /*  overlay image */
#define ICURSOR      35
#define JDCURSOR     36
#define JBANNER      37
#define ACURSOR      38  /* aip style cursor */
#define JFGCOLOR     39
#define JGRADCOLOR   40  /* iplan intensity color */
#define JPRECT       41  /* iplan rectangle  */
#define JPARC        42  /* iplan drawarc  */
#define JPOLYGON     43  /* iplan fillPolygon  */
#define JCROSS       44  /* iplan cross  */
#define JREGION      45
#define JHCURSOR     46  /* crosshair cursor */
#define JFRAME       47
#define CSCOLOR      49  /* cursor color */
#define REEXEC       50  /* isReexecCmd  */
#define JSYNC        51  /* end of jprint  */
#define JALPHA       52  /* graphics alpha mode */
#define JHTEXT       53
#define JMOVIE_START 54
#define JMOVIE_NEXT  55
#define JMOVIE_END   56
#define FRMPRTSYNC   57
#define TICTEXT      58
#define JXORON       59  // enable xor
#define JXOROFF      60  // disable xor
#define XYBAR        61  // non xor ybar
#define PENTHICK     62  // for spectrum and lines
#define AIP_TRANSPARENT  63
#define WINPAINT     64
#define SPECTRUMWIDTH  65  // for spectrum only
#define LINEWIDTH    66  // for lines only
#define AIPID        67
#define TRANSPARENT  68
#define SPECTRUM_MIN  69
#define SPECTRUM_MAX  70
#define SPECTRUM_RATIO 71
#define LINE_THICK    72
#define GRAPH_FONT    73
#define GRAPH_COLOR   74
#define NOCOLOR_TEXT  75   // no color argument
#define BACK_REGION   76
#define ICURSOR2      77
#define THSCOLOR      78  /* threshold color */
#define IMG_SLIDES_START 79
#define IMG_SLIDES_END   80
#define ENABLE_TOP_FRAME   81
#define CLEAR_TOP_FRAME   82
#define JTABLE      83
#define JARROW      84
#define JROUNDRECT  85  // round rectangle
#define JOVAL       86  // oval
#define RGB_ALPHA   87
#define VBG_WIN_GEOM 88
#define RAISE_TOP_FRAME  89
#define FONTSIZE      90

#define JFRAME_OPEN     1
#define JFRAME_CLOSE    2
#define JFRAME_CLOSEALL 3
#define JFRAME_ACTIVE   4
#define JFRAME_IDLE     5
#define JFRAME_IDLEALL  6
#define JFRAME_ENABLE     7
#define JFRAME_ENABLEALL  8
#define JFRAME_CLOSEALLText 9
#define JGINCURSOR   1
#define JDEFCURSOR   2

#define ITYPE        101
#define IRECT        102
#define ILINE        103
#define IOVAL        104
#define IPOLYLINE    105
#define IPOLYGON     106
#define IBACKUP      107
#define IFREEBK      108
#define IWINDOW      109
#define ICOPY        110
#define IRASTER      111
#define IGRAYMAP     112
#define ICLEAR       113
#define ITEXT	     114
#define ICOLOR	     115
#define IPLINE	     116  /* for iplan line */
#define IPTEXT	     117  /* for iplan text */
#define DPCON	     118

#define SET3PMODE    119
#define SET3PCURSOR  120
#define ICSIWINDOW   121
#define ICSIDISP     122
#define OPENCOLORMAP 123  // open colormap
#define SETCOLORMAP  124  //  switch colormap
#define SETCOLORINFO 125  //  add image info
#define SELECTCOLORINFO 126  //  set image info selected
#define ICSIORIENT   127
#define IVTEXT	     128
#define ANNFONT      129
#define ANNCOLOR     130
#define AIPCODE      1010

static struct  {
	int	type;
	int	code;
     } j0p;

static struct  {
	int	type;
	int	code;
     } jx0p;

static struct  {
	int	type;
	int	code;
     } aipj0p;

static struct  {
	int	type;
	int	code;
	int	size;
	int	p;
     } j1p;

static struct  {
	int	type;
	int	code;
	int	size;
	int	p;
     } aipj1p;

static struct  {
	int	type;
	int	code;
	int	size;
	int	p[4];
     } j4p;

static struct  {
	int	type;
	int	code;
	int	size;
	int	p[4];
     } aipj4p;

static struct  {
	int	type;
	int	code;
	int	size;
	int	p[6];
     } j6p;

static struct  {
	int	type;
	int	code;
	int	size;
	int	p[6];
     } aipj6p;

static struct  {
	int	type;
	int	code;
	int	size;
	int	p[8];
     } j8p;

static struct  {
	int	type;
	int	code;
	int	size;
	int	p[8];
     } aipj8p;

typedef struct {
	int	type;
	int	subtype;
	int	id;
	int	x;
	int	y;
	int	color;
	int	len;
	char	*str;
	} jText;

typedef struct {
	int	type;
	int	subtype;
	int	id;
	int	x;
	int	y;
	int	w;
	int	h;
	int	color;
	int	len;
	char	*str;
	} jxText;

static struct {
	int	type;
	int	subtype;
	int	id;
	int	len;
	short	data[JLEN * 2];
	} jybar;

static struct {
	int	type;
	int	subtype;
	int	id;
	int	len;
	short	data[JLEN * 2];
	} aipjybar;


typedef struct {
	int	type;
	int	subtype;
	int	len;
	int	x;
	int	y;
	int	repeat;
	unsigned char	*data;
	} JIMAGE;

static JIMAGE   *jimage = NULL;
static char 	*jimage_data = NULL;
static int 	jimage_size = 0;


typedef struct _jtabcolor {
	int	red;
	int	grn;
	int	blu;
	} jtabcolor;

static struct _jtcolor {
	int	type;
	int	subtype;
	int	tabsize;
	int	taboffset;
	jtabcolor table[CMS_VNMRSIZE];
	} jtcolor;


#ifdef MOTIF
typedef struct _ipcolor {
	int	color;
	int	levels;
	Pixel   *pix;
	struct  _ipcolor *next;
	} IPCOLOR;

static IPCOLOR  *ipcolor = NULL;
#endif

static int  j1pLen = 0;
static int  j4pLen = 0;
static int  j6pLen = 0;
static int  j8pLen = 0;
static int  xdebug = 0;
static int  csidebug = 0;
static int  jRaster = JRASTER;
static int  intLen = sizeof(int);
static int  ybarHead = sizeof(int) * 4;
static int  paletteSize = 0;
static int  lastPalette = GRAYSCALE_COLORS + 1;
static int  curPalette = 0;
static int  curTransparence = 0;
// static palette_t palettes[6];
static palette_t *paletteList = NULL;

#ifdef MOTIF
static XFontStruct    *structOfbannerFont = NULL;
static XFontStruct    *bannerFontInfo = NULL;
static XFontStruct    *scaleFontInfo = NULL;
/*
static XFontStruct    *annotateFontInfo = NULL;
 */
static XFontStruct    *xstruct = NULL;
static XFontStruct    *org_xstruct = NULL;
static GC       vchar_gc = NULL;
static GC       grid_gc = NULL;
Pixmap   canvasPixmap = 0;
Pixmap   canvasPixmap2 = 0;
Pixmap   overlayMap = 0;
Pixmap   org_Pixmap = 0;
static XPoint   points[YLEN];
static XPoint   mpoints[YLEN];
static XSegment segments[YLEN];
static XSegment msegments[YLEN];
static Cursor   ginCursor = 0;
/*
static Cursor   orgCursor = 0;
 */
static Region   xregion = NULL;
GC       vnmr_gc = NULL;
GC       pxm_gc = NULL;
GC       pxm2_gc = NULL;
GC       ovly_gc = NULL;
GC       org_gc = NULL;
Font     x_font = 0;
Font     org_x_font = 0;
XColor   vnmr_colors[COLORSIZE+120];
static   XImage  *rast_image = NULL;
static XPoint   *xpoints = NULL;
static int  xpoints_len = 0;
static int  def_lineStyle = LineSolid;
static int  def_capStyle = CapProjecting;
static int  def_joinStyle = JoinRound;

typedef struct  _font_rec {
        int     id;
        int     name_len;
        int     new_name;
        char    *name;
        XFontStruct *fstruct;
        struct  _font_rec *next;
        } FONT_REC;

static FONT_REC  *font_lists = NULL;

#else
GC     vnmr_gc = 0;
GC     pxm_gc = 1;
GC     pxm2_gc = 2;
GC     ovly_gc = 3;
GC     org_gc = 0;
Pixmap  canvasPixmap = 1;
Pixmap  org_Pixmap = 1;
Pixmap  canvasPixmap2 = 2;
Pixmap  overlayMap = 3;
char   *x_font = NULL;

#endif

static XID *pixmap_array = NULL; /* pixmap list for image browser */
static int pixmap_size = 0;
static int pixmap_index = 5;
static int aipFrameDraw = 0;
static int aipFrameMap = 0;
static int aipFrameWidth = 0;
static int aipFrameHeight = 0;
static int frameWidth = 0;
static int frameHeight = 0;
static int s_overlayMode = 0;
static int s_activeWin = -1;
static int s_alignvp = -1;
static int s_chartMode = 0;
static int jtext_len = 0;

static char x_cross_info[12];
static char y_cross_info[12];

static char s_jvpInfo[12];
static char gmode[GRAPHCMDLEN+8];
static char execCmdStr[CMDLEN+4];
static char tmpStr[MAXPATH * 2];
static char *jtext_data = NULL;
#ifdef OLD
static char lastPlotCmd[GRAPHCMDLEN];
#endif

static double scrnDpi = 90.0;
static double s_spx = 0.0;
static double s_wpx = 0.0;
static double s_spy = 0.0;
static double s_wpy = 0.0;
static double s_sp2 = 0.0;
static double s_wp2 = 0.0;
static double s_sp1 = 0.0;
static double s_wp1 = 0.0;
static char s_axis2[8];
static char s_axis1[8];

/** ybars data info **/
static int ybarSize;
#ifdef OLD
static struct ybar *ybarData = NULL;
static int ybarCapacity = 0;
static int verticalYbar = 0;
static int ybarMax;
static int ybarMin;
static int ybarColor;
static int ybarYpnts;
static int ybarStart;
static int ybarEnd;
#endif

/* these structure were defined in aipStructs.h  */

/******

typedef struct Gpoint {
    int x;
    int y;
} Gpoint_t;

typedef struct Fpoint {
    float x;
    float y;
} Fpoint_t;

typedef struct Dpoint {
    double x;
    double y;
} Dpoint_t;

******/

typedef struct _timeOut {
        XID    id; 
        int    active; 
        long   sec;  /* start time of second */
        long   usec;  /* start time of microsecond */
        long   tv_sec;  /* interval time of second */
        long   tv_usec;  /* interval time of microsecond */
        int    valSize; 
	void   (*func)();
        char   *retVal; 
        struct  _timeOut *next;
       } TIME_NODE;
        
static TIME_NODE *timeOut_list = NULL;

/*****************************/
extern void update3PCursors();
extern void  overlaySpec(int);
extern int isTextFrame(int);
extern int isDrawable(int);
extern void frame_updateG(char *cmd, int id, int x, int y, int w, int h);
extern void read_jcmd();
extern void init_colors();
extern void get_color(int index, int *red, int *grn, int *blu);
extern int loadAndExec(char *buffer);
extern int isReexecCmd(char *sName);
extern int getCmdRedoType(char *sName);
extern int sun_setdisplay();
extern int sun_window();
extern int xormode();
extern int normalmode();
extern int Wisactive_mouse();
extern int WisJactive_mouse();
extern int set_wait_child(int pid);
extern int get_plot_planes();
extern int graphToVnmrJ( void *message, int messagesize );
extern int get_raster_color(int index);
extern void redraw_overlay_image();
extern void repaint_overlay_image();
extern void iplan_set_win_size(int x, int y, int w, int h);
extern void setVjGUiPort(int on);
extern void setVjUiPort(int on);
extern void setVjPrintMode(int on);
extern void openPrintPort();
extern void showError(char *msg);
extern void setPrintScrnMode(int s);
extern void Wturnoff_message();
extern void Wturnon_message();
extern double getpaperwidth();
extern double getpaperheight();
extern int jplot_charsize(double f);
extern void aipSelectViewLayer(int imgId, int order, int mapId);
extern int getDscaleDecimal(int);

void setup_aip();
void aip_initGrayscaleCMS(int n);
void auto_switch_graphics_font();
void open_var_font(char *name, int i, int w, int s);
void list_fonts(int i, int *num, char *d);
void convert_raster(XImage *img, int pnts, void *src);
void redrawCanvas();
void open_scalable_font(char *name, int size, int check);
void open_bfont(char *name);
void make_table();
void set_win_size(int x, int y, int w, int h);
// void set_frame_size(int x, int y, int w, int h);
void window_redisplay();
void flush_draw();
static void repaint_frames(int n, int do_flush);
int dconi_x_rast_cursor(int *old_pos, int new_pos, int off1, int off2, int num);
int dconi_y_rast_cursor(int *old_pos, int new_pos, int off1, int off2, int num);
int open_color_palette(char *name);
static void init_color_palette();
static FILE *iplotFd = NULL;
static FILE *iplotMainFd = NULL;
static FILE *iplotTopFd = NULL;
int transparency_level = 0;


void jSetGraphicsWinId(char *idStr, int useX )
{
	long nid;

	nid = atol( idStr );
	if (nid <= 0 || useX <= 0)
	     nid = 0;
	if (xdisplay == NULL)
	     nid = 0;
	if (nid == 0) {
	     if (useXFunc || xid > 0)
	        is_Open = 0;
	     useXFunc = 0;
	     xid = 0;
	     org_xid = 0;
	     orgx = 0;
	     orgy = 0;
	     org_orgx = 0;
	     org_orgy = 0;
#ifdef MOTIF
             if (canvasPixmap > 5)
                XFreePixmap(xdisplay, canvasPixmap);
             if (canvasPixmap2 > 5)
                XFreePixmap(xdisplay, canvasPixmap2);
             if (overlayMap > 5)
                XFreePixmap(xdisplay, overlayMap);
#endif
	     if (is_Open == 0)
		sun_window();
	     canvasPixmap = 1;
	     canvasPixmap2 = 2;
	     org_Pixmap = 1;
	     overlayMap = 3;
	     screen = 1;
	     setup_aip();
	     return;
        }
	if (useXFunc <= 0)
	{
	    if (canvasPixmap < 5) {
	        canvasPixmap = 0;
	        org_Pixmap = 0;
            }
	    if (canvasPixmap2 < 5)
	        canvasPixmap2 = 0;
	    if (overlayMap < 5)
	        overlayMap = 0;
        }
        else
	{
	     if (nid == xid)
	         return;
	}
	useXFunc = 1;
	xid = nid;
	org_xid = nid;
	is_Open = 0;
#ifdef MOTIF
        if (org_orgx > 0 && isSuspend == 0 && xregion != NULL)
             winDraw = org_winDraw;
        else
             winDraw = 0;
#endif
	sun_window();
        setup_aip();
}

void jSetGraphicsWinId2(long num )
{
#ifdef MOTIF
        XSetWindowAttributes   attrs;
	if (xdisplay == NULL || !useXFunc)
	     return;
	if (num > 0 && xid2 <= 0) {
	   xid2 = num;
    	   XSetWindowBackground(xdisplay, xid2, xblack);
           attrs.backing_store = Always;
           XChangeWindowAttributes(xdisplay, xid2, CWBackingStore, &attrs);
	}
#endif
}

void jSetGraphicsShow(int show)
{
	if (!useXFunc) {
            if (xdebug > 1)
                fprintf(stderr, " jSetGraphicsShow  %d \n", show);
            if (show > 0)
                waitMovieDoneRet = 0;
	    return;
        }
	win_show = show;
	if (show) {
	     if (in_batch && canvasPixmap)
		batch_on = 1;
	     else
		batch_on = 0;
	}
	else {
		batch_on = 1;
	}
	if (win_show && batch_on == 0)
	     winDraw = 1;
	else 
	     winDraw = 0;
	org_winDraw = winDraw;
#ifdef MOTIF
        if (isSuspend > 0 || orgx < 0 || xregion == NULL)
	     winDraw = 0;
#endif
}

static int getGraphicsCmd()
{
     gmode[0] = '\0';
     Wgetgraphicsdisplay(gmode, GRAPHCMDLEN);
     if (csi_opened) {
         if (strncmp(gmode, aipRedisplay, 10) == 0)
             return (0);
     }
     return ((int) strlen(gmode));
}


static void clear_frame_graphFunc(WIN_STRUCT *info)
{
      GCMD  *glist;

      if (info == NULL)
          return;
      if (xdebug > 1)
          fprintf(stderr, "  clear graphics cmd \n");
      strcpy(info->graphics, "");
      strcpy(info->graphMode, "");
      info->graphType = 0;
      info->reExec = 0;
      info->mouseActive = 0;
      info->mouseJactive = 0;
      info->mouseMove = NULL;
      info->mouseButton = NULL;
      info->mouseReturn = NULL;
      strcpy(info->menu, "");
      glist = info->cmdlist;
      while (glist != NULL) {
          glist->size = 0;
          glist->graphType = 0;
          glist->reExec = 0;
          if (glist->cmd != NULL)
              strcpy(glist->cmd, "");
          glist = glist->next;
      }
}

static void
copy_frame_info(WIN_STRUCT *dest, WIN_STRUCT *src)
{
     if (src == NULL) { 
         strcpy(dest->graphics,"");
         strcpy(dest->graphMode,"");
         strcpy(dest->menu,"");
         return;
     }
     dest->active = src->active;
     dest->x = src->x;
     dest->y = src->y;
     dest->isClosed = src->isClosed;
     dest->isVertical = src->isVertical;
     dest->width = src->width;
     dest->height = src->height;
     dest->xmap = src->xmap;
     dest->fontW = src->fontW;
     dest->fontH = src->fontH;
     dest->ascent = src->ascent;
     dest->descent = src->descent;
     dest->axis_fontW = src->axis_fontW;
     dest->axis_fontH = src->axis_fontH;
     dest->axis_ascent = src->axis_ascent;
     dest->axis_descent = src->axis_descent;
     dest->graphType = src->graphType;
     dest->reExec = src->reExec;
     dest->cmdlist = src->cmdlist;
     dest->cmdSize = src->cmdSize;
     dest->mouseMove = src->mouseMove;
     dest->mouseButton = src->mouseButton;
     dest->mouseReturn = src->mouseReturn;
     dest->mouseClick = src->mouseClick;
     dest->mouseGeneral = src->mouseGeneral;
     dest->font = src->font;
     strcpy(dest->graphics,src->graphics);
     strcpy(dest->graphMode,src->graphMode);
     strcpy(dest->menu,src->menu);
}

static void
create_frame_info_struct(int n)
{
     int   newSize, k;
     WIN_STRUCT  *oldf, *newf;

     if (n < 0)
         n = 0;
     if (frame_info != NULL && frameNums > n) {
          main_frame_info = &frame_info[0];
          return;
     }
     newSize = n + 4;
     new_info = (WIN_STRUCT *) malloc(newSize * sizeof(WIN_STRUCT));
     if (new_info == NULL) {
          Werrprintf("Couldn't allocate memory for frame struct.");
          return;
     }
     if (frame_info != NULL) {
          for (k = 0; k < frameNums; k++) {
              oldf = &frame_info[k];
              newf = &new_info[k];
              copy_frame_info(newf, oldf);
              newf->frameId = k;
          }
          free(frame_info);
     }
     for (k = frameNums; k < newSize; k++) {
          newf = &new_info[k];
          newf->frameId = k;
          newf->active = 0;
          newf->x = 0;
          newf->y = 0;
          newf->width = win_width;
          newf->height = win_height;
          newf->isClosed = 1;
          newf->isVertical = 0;
          newf->xmap = 0;
          newf->mouseActive = 0;
          newf->mouseMove = NULL;
          newf->mouseButton = NULL;
          newf->mouseReturn = NULL;
          newf->mouseClick = NULL;
          newf->mouseGeneral = NULL;
          newf->cmdlist = NULL;
          newf->font = NULL;
          newf->cmdSize = 0;
          newf->graphType = 0;
          newf->reExec = 0;
          strcpy(newf->graphics,"");
          strcpy(newf->graphMode,"");
          strcpy(newf->menu,"");
     }
     frame_info = new_info;
     frameNums = newSize;
     main_frame_info = &frame_info[0];
     main_frame_info->isClosed = 0;
     main_frame_info->isVertical = 0;
     if (active_frame < 1) {
         main_frame_info->x = org_orgx;
         main_frame_info->y = org_orgy;
         main_frame_info->width = win_width;
         main_frame_info->height = win_height;
         main_frame_info->fontW = char_width;
         main_frame_info->fontH = char_height;
         main_frame_info->ascent = char_ascent;
         main_frame_info->descent = char_descent;
         main_frame_info->axis_fontW = char_width;
         main_frame_info->axis_fontH = char_height;
         main_frame_info->axis_ascent = char_ascent;
         main_frame_info->axis_descent = char_descent;
         strcpy(main_frame_info->graphics, "");
         strcpy(main_frame_info->graphMode, "");
         strcpy(main_frame_info->menu, "");
      }
}

static WIN_STRUCT *getCurrentFrameStruct()
{
     if (main_frame_info == NULL)
         create_frame_info_struct(1);
     if (active_frame > 0)
         return (&frame_info[active_frame]);
     return main_frame_info;
}

// turn on or off aip_frame_info
static void
switch_aip_frame_mode(int on)
{
     WIN_STRUCT *f;

     if (aip_frame_info == NULL) {
         aip_frame_info = (WIN_STRUCT *) malloc(sizeof(WIN_STRUCT));
         if (aip_frame_info == NULL)
              return;
         bzero(aip_frame_info, sizeof(WIN_STRUCT));
         copy_frame_info(aip_frame_info, NULL);
         aip_frame_info->active = 0;
     }
     if (aip_frame_info->active == on)
         return;
     f = NULL;
     if (frameNums > 0) {
         if (active_frame > 0)
             f = &frame_info[1];
         else
             f = main_frame_info;
     }
     aip_frame_info->active = on;
     if (f == NULL)
         return;
     if (on) {
         copy_frame_info(aip_frame_info, f);
         clear_frame_graphFunc(f); 
         strcpy(aip_frame_info->graphics, aipRedisplay);
         Wturnoff_mouse();
         return;
     }
     else {
         clear_frame_graphFunc(f); 
         copy_frame_info(f, aip_frame_info);
         strcpy(f->graphics, aipRedisplay);
         if (csi_opened) {
             f->mouseMove = aip_mouseMove;
             f->mouseButton = aip_mouseButton;
             f->mouseReturn = aip_mouseReturn;
             f->mouseClick = aip_mouseClick;
             f->mouseGeneral = aip_mouseGeneral;
         }
         if (strlen(f->menu) > 0) {
             sprintf(execCmdStr, "menu('%s')\n", f->menu);
             execString(execCmdStr);
         }
     }
}

// called by handler.c before loadAndExec
void newLogCmd(char *cmd, int len)
{
     int debugPrint;

     originRedoCode = 0; 
     execDepth = 0;
     if (isWinRedraw != 0)
         return;
     if (csidebug > 2) {
         if (len > 5) {
            if (cmd[0] == 'j' && cmd[1] == 'M') {
                fprintf(stderr, "jMove ....... \n");
            }
         }
     }
     if (xdebug > 2) {
         debugPrint = 0;
         if (xdebug > 4)
            debugPrint = 1;
         else if (xdebug > 3) {
            if (len > 5) {
               if (cmd[0] == 'j') {
                    if (cmd[1] != 'M' && cmd[1] != 'F')  // jMove, jFunc
                        debugPrint = 1;
               }
               else
                    debugPrint = 1;
            }
            else
               debugPrint = 1;
         }
         if (debugPrint)
            fprintf(stderr, "vbg %d: start... %s", VnmrJViewId, cmd);
     }
     // isNewGraph = 0;
     isGraphFunc = 0;
     isClearCmd = 0;
     originEexecCode = 0;
}

// called by handler.c after loadAndExec
void saveLogCmd(char *cmd, int len)
{ 
     WIN_STRUCT  *fmInfo;
     int debugPrint;

     if (isWinRedraw != 0)
         return;
     if (initWinRedraw) {
         initWinRedraw = 0;
         return;
     }
     if (xdebug > 1 && isGraphFunc != 0) {
         fprintf(stderr, "   cmd '%s'     has graphics func.\n", cmd);
     }
     if (xdebug > 2) {
         debugPrint = 0;
         if (xdebug > 4)
            debugPrint = 1;
         else if (xdebug > 3) {
             if (len > 5) {
                if (cmd[0] == 'j') {
                    if (cmd[1] != 'M' && cmd[1] != 'F')  // jMove, jFunc
                        debugPrint = 1;
                }
                else
                    debugPrint = 1;
             }
             else
                debugPrint = 1;
         }
         if (debugPrint)
            fprintf(stderr, "vbg %d:  end... %s", VnmrJViewId, cmd);
     }
     if (isGraphFunc != 0 || isClearCmd != 0) {
         if (rexecCode > 0 || isClearCmd > 0) {
            fmInfo = getCurrentFrameStruct();
            if (fmInfo != NULL)
            {
               if (getGraphicsCmd() > 0) {
                   if (isReexecCmd(gmode) > 0) {
                       strcpy(fmInfo->graphics, gmode);
                       strcpy(fmInfo->graphMode, gmode);
                       fmInfo->graphType = getCmdRedoType(gmode);
                       fmInfo->reExec = 1;
                       if (xdebug > 1)
                          fprintf(stderr, "  save graphics cmd(2) '%s'\n", fmInfo->graphics);
                   }
               }
            }
         }
         isGraphFunc = 0;
         rexecCode = 0;
         isClearCmd = 0;
     }
}

int in_window_redisplay()
{
     return isWinRedraw;
}

void copy_inset_frame_cmd(int newId, int oldId)
{
     WIN_STRUCT  *newf, *oldf;

     if (newId == oldId)
         return;
     if (newId < 1)
         return;
     if (newId < 1 || oldId < 1 || oldId >= frameNums)
         return;
     if (newId >= frameNums)
         create_frame_info_struct(newId);
     newf = &frame_info[newId];
     oldf = &frame_info[oldId];
     if (newf == NULL || oldf == NULL)
         return;
     clear_frame_graphFunc(newf);
     strcpy(newf->graphics, oldf->graphics);
     strcpy(newf->graphMode, oldf->graphMode);
     strcpy(newf->menu, oldf->menu);
     newf->mouseActive = oldf->mouseActive;
     newf->graphType = oldf->graphType;
     newf->reExec = oldf->reExec;
}

static void cleanCmdNode(WIN_STRUCT *info, GCMD *node)
{
     GCMD  *pnode;

     if (node == NULL || info == NULL)
         return;
     node->size = 0;
     node->graphType = 0;
     if (node->cmd != NULL)
        strcpy(node->cmd, "");
     if (node->next == NULL)
         return;
     pnode = info->cmdlist;
     while (pnode != NULL) {
         if (pnode == node) { // the root node
             pnode = NULL;
             break;
         }
         if (pnode->next == NULL)
             return;
         if (node == pnode->next)
             break;
         pnode = pnode->next;
     }
     if (node == info->cmdlist) {
         info->cmdlist = node->next;
         pnode = info->cmdlist;
     }
     else {
         if (pnode == NULL)
             return;
         pnode->next = node->next;
     }
     node->next = NULL;
     while (pnode->next != NULL)
         pnode = pnode->next;
     pnode->next = node;
}

static void removeDuplicateCmd(WIN_STRUCT *info, char *name)
{
     GCMD  *xnode;
     char  *pindex;
     int   n;

     if (info == NULL)
         return;
     xnode = info->cmdlist;
     while (xnode != NULL) {
         if (xnode->size > 0 && xnode->cmd != NULL) {
             pindex = index(xnode->cmd, '(');
             if (pindex != NULL) {
                 n = pindex - xnode->cmd;
                 if (strncmp(xnode->cmd, name, n) == 0)
                     break;
             }
             else if (strcmp(xnode->cmd, name) == 0)
                 break;
         }
         xnode = xnode->next;
     }
     if (xnode == NULL)
         return;
     cleanCmdNode(info, xnode);
}


void saveGraphFunc(char *argv[], int argc, int reexec, int redo)
{
}

void ignoreGraphFunc(int n)
{
     isIgnored = n; 
}

void startGraphFunc(char *cmd, int reexec, int redo)
{

}

void
finishGraphFunc()
{

}

void
setAutoRedisplayMode(int on)
{
     GCMD *glist;
     WIN_STRUCT *fmInfo;

      if (isPrintBg != 0)
          return;
      if (isAutoRedraw)
          return;
      if (frame_info == NULL)
          return;
      if (on) {
          isWinRedraw = 1;
          isNewGraph = 0;
          return;
      }
      if (isNewGraph <= 0) {
          isWinRedraw = 0;
          return;
      }
      fmInfo = getCurrentFrameStruct();
      if (fmInfo == NULL)
          return;
      isAutoRedraw = 1;
      glist = fmInfo->cmdlist;
      while (glist != NULL) {
          if (glist->size > 0 && glist->cmd != NULL) {
              if (strlen(glist->cmd) > 0) {
                  if (glist->graphType >= 5) {
                     sprintf(tmpStr, "%s\n", glist->cmd);
                     if (xdebug > 0)
                         fprintf(stderr, " autoRedisplay cmd:  %s", tmpStr);
                     execString(tmpStr);
                  }
                  else
                     glist->size = 0;
              }
          }
          glist = glist->next;
      }
      isWinRedraw = 0;
      isNewGraph = 0;
      isAutoRedraw = 0;
}

// called by exec.c
void startExecGraphFunc(char *name, int reexec, int redo)
{
      if (redo < 1 || redo > 9)
         return;
      if (isWinRedraw != 0)
         return;
      if (xdebug > 4)
         fprintf(stderr, "  gstart level %d:  %s\n", execDepth, name);
      if (redo >= 5) {
         isGraphCmd = isGraphFunc;
         isGraphFunc = 0;
      }
      if (execDepth == 0)
      {
          originEexecCode = reexec;
          originRedoCode = redo;
          rexecCode = reexec;
          redoCode = redo;
          if (redo == 5)
              strcpy(execCmdStr, "");
          else
              strcpy(execCmdStr, name);
      }
      else if (reexec > 0)
      {
          if (originRedoCode != 5)
          {
              if (xdebug > 1)
                 fprintf(stderr, "   level %d:  %s  reexec %d\n", execDepth, name, reexec);
              rexecCode = reexec;
              redoCode = redo;
          }
      }
      execDepth++;
}

void saveExecGraphFunc(char *name, int type, int argc, char *argv[])
{
    int i, len;

    if (isWinRedraw != 0)
        return;
 
    if (execDepth != 1 || argc < 1)
        return;
    
    if (isGraphFunc == 0) {
        isGraphFunc = isGraphCmd;
        if (type == 5)
            return;
    }
    if (xorflag)
        return;
    isGraphFunc = 1;
    // strcpy(execCmdStr, argv[0]);
    // len = strlen(argv[0]);
    len = strlen(name);
    if (len >= CMDLEN)
        return;

    strcpy(execCmdStr, name);
    if (argc > 1)
    {
        strcat(execCmdStr, "(");
        len += 2;
        for (i = 1; i < argc; i++)
        {
            len = len + strlen(argv[i]) + 3;
            if (len >= CMDLEN)
                break;
            strcat(execCmdStr, "'");
            strcat(execCmdStr, argv[i]);
            strcat(execCmdStr, "'");
            if (i < (argc - 1))
               strcat(execCmdStr, ",");
        }
        strcat(execCmdStr, ")");
    }
}

void
endExecGraphFunc(char *name, int reexec, int redo, int status)
{
     int k, oldRedoCode, aipCmd;
     WIN_STRUCT  *fmInfo;
     GCMD  *glist, *gnext;

     if (isWinRedraw != 0)
         return;
     if (initWinRedraw) {
         initWinRedraw = 0;
         isGraphFunc = 0;
         return;
     }
     aipCmd = 0;
     if (csi_opened && (strncmp(name, "aip", 3) == 0))
         aipCmd = 1;
     if (isNewGraph)
     {
         isClearCmd = 1;
         if (xdebug > 1)
             fprintf(stderr, "  end cmd '%s', clear graphics called\n", name);
         if (aipCmd == 0) {
             fmInfo = getCurrentFrameStruct();
             clear_frame_graphFunc(fmInfo);
         }
         isNewGraph = 0;
     }
     if ( isGraphFunc != 0 && originRedoCode <= 0) {
         if (xdebug > 2)
            fprintf(stderr, "  cmd '%s' did graphics func\n", name);
     }
     if (redo < 1 || redo > 9 || aipCmd)
         return;
     if (xdebug > 4)
        fprintf(stderr, "   gend level %d:  %s\n", execDepth, name);
     if (execDepth <= 0)
         return;
     execDepth--;
     if (status < 1) // command failed
     {
         isGraphFunc = 0;
         isClearCmd = 0;
     }
     if (execDepth > 0 || isGraphFunc <= 0)
         return;
     if (xdebug > 1)
        fprintf(stderr, "     graphics func %s\n", name);
     oldRedoCode = originRedoCode;
     originRedoCode = 0;
     isGraphFunc = 0;
     fmInfo = getCurrentFrameStruct();
     if (fmInfo == NULL)
         return;
     if (rexecCode > 0)
     {
         if (getGraphicsCmd() > 0) {
             if (isReexecCmd(gmode) > 0) {
                 fmInfo->graphType = redo;
                 fmInfo->reExec = 1;
                 strcpy(fmInfo->graphics, gmode);
                 strcpy(fmInfo->graphMode, gmode);
                 if (xdebug > 1)
                     fprintf(stderr, "  save graphics cmd(1) '%s'\n", gmode);
             }
         }
         return;
     }
     if (oldRedoCode == 4) // not graphics command
     {
         return;
     }
     if (strlen(execCmdStr) < 1)
         return;
     if (redo == 7)
        removeDuplicateCmd(fmInfo, name);
     gnext = NULL;
     glist = fmInfo->cmdlist;
     while (glist != NULL) {
         if (oldRedoCode <= 3) {
             if (glist->size > 0 && glist->cmd != NULL) {
                 if (strcmp(glist->cmd, execCmdStr) == 0)
                 {
                     glist->graphType = oldRedoCode;
                     return;
                 }
             }
         }
         if (glist->size <= 0) // it is a free node
         {
             gnext = glist;
             break;
         }
         if (glist->next == NULL)
             break;
         glist = glist->next;
     }
     if (gnext == NULL) {
         if (xdebug > 1)
             fprintf(stderr, " alloc new graphics redraw node  \n");
         gnext = (GCMD *) malloc(sizeof(GCMD));
         if (gnext == NULL)
             return;
         if (fmInfo->cmdlist == NULL || glist == NULL)
             fmInfo->cmdlist = gnext;
         else
             glist->next = gnext;
         gnext->size = 0;
         gnext->len = 0;
         gnext->cmd = NULL;
         gnext->next = NULL;
     }
     k = (int) strlen(execCmdStr) + 2;
     if (gnext->len < k) {
         if (gnext->cmd != NULL)
             free(gnext->cmd);
         if (xdebug > 1)
             fprintf(stderr, "  alloc new graphics redraw string \n");
         gnext->cmd = (char *) malloc(k);
         gnext->len = k;
     }
     if (gnext->cmd == NULL) {
         gnext->len = 0;
         gnext->size = 0;
         return;
     }
     strcpy(gnext->cmd, execCmdStr);
     gnext->size = k;
     gnext->graphType = oldRedoCode;
}

static int
removeSubCmd(WIN_STRUCT *info, char *cmd)
{
     GCMD  *xnode;

     xnode = info->cmdlist;
     while (xnode != NULL) {
         if (xnode->size > 0 && xnode->cmd != NULL) {
             if (strcmp(xnode->cmd, cmd) == 0) {
                 break;
             }
         }
         xnode = xnode->next;
     }
     if (xnode == NULL)
         return 0;
     cleanCmdNode(info, xnode);
     return 1;
}


void
removeGraphSubFunc(char *cmd)
{
     WIN_STRUCT  *fmInfo;
     int   retV;

     if (isWinRedraw != 0)
         return;
     fmInfo = getCurrentFrameStruct();
     if (fmInfo == NULL)
         return;
     retV = 1;
     while (retV) {
         retV = removeSubCmd(fmInfo, cmd);
     }
}


void
addGraphSubFunc(char *cmd)
{
     WIN_STRUCT  *fmInfo;
     GCMD  *gnode, *xnode;
     int   k;

     if (isWinRedraw != 0)
         return;
     fmInfo = getCurrentFrameStruct();
     if (fmInfo == NULL)
         return;
     k = 1;
     while (k) {
         k = removeSubCmd(fmInfo, cmd);
     }
     gnode = fmInfo->cmdlist;
     xnode = NULL;
     while (gnode != NULL) {
         if (gnode->size <= 0)
         {
             xnode = gnode;
             break;
         }
         if (gnode->next == NULL)
             break;
         gnode = gnode->next;
     }
     if (xnode == NULL) {
         xnode = (GCMD *) malloc(sizeof(GCMD));
         if (xnode == NULL)
             return;
         if (fmInfo->cmdlist == NULL || gnode == NULL)
             fmInfo->cmdlist = xnode;
         else
             gnode->next = xnode;
         xnode->size = 0;
         xnode->len = 0;
         xnode->cmd = NULL;
         xnode->next = NULL;
     }
     k = (int) strlen(cmd) + 2;
     if (xnode->len < k) {
         if (xnode->cmd != NULL)
             free(xnode->cmd);
         xnode->cmd = (char *) malloc(k);
         xnode->len = k;
     }
     if (xnode->cmd == NULL) {
         xnode->len = 0;
         xnode->size = 0;
         return;
     }
     strcpy(xnode->cmd, cmd);
     xnode->size = k;
     xnode->graphType = 6;
}

void
clearGraphFunc()
{
       int k;

       if (isWinRedraw != 0)
           return;
       redoCode = 0;
       clear_frame_graphFunc(main_frame_info);
       if (frame_info != NULL) {
           for (k = 0; k < frameNums; k++)
               clear_frame_graphFunc(&frame_info[k]);
       }
       execString("imagefile('clear')\n");
}

void
removeGraphFunc()
{
       if (isWinRedraw != 0)
           return;
      /***
       redoCode = 0;
      ***/
}

static void
set_aip_win_size(int x, int y, int w, int h)
{
     if (w < 20 || h < 20)
         return;
     iplan_set_win_size(x, y, w, h);
     aipSetGraphicsBoundaries(x, y, w, h);
}

static int
set_frame_font(WIN_STRUCT *frame, const char *fontName)
{
#ifndef INTERACT
     int fw, fh, n, lines, isAxisNum;
     int width, height;
     VJFONT *ftNode = NULL;

     if (vjFontList == NULL)
         return(0);
     ftNode = frame->font;
     if (ftNode != NULL) {
         if (strcmp(fontName, ftNode->name) != 0)
             ftNode = NULL;
     }
     if (ftNode == NULL) {
         ftNode = vjFontList;
         while (ftNode != NULL) {
             if (strcmp(fontName, ftNode->name) == 0)
                  break;
             ftNode = ftNode->next;
         }
         if (ftNode == NULL) {
             ftNode = defaultFont;
             if (ftNode == NULL)
                  return(0);
         }
         frame->font = ftNode;
     }
     if (frame->isVertical) {
         width = frame->height;
         height = frame->width;
     }
     else {
         width = frame->width;
         height = frame->height;
     }
     isAxisNum = 0;
     if (strcmp(fontName, AxisNumFontName) == 0)
         isAxisNum = 1;
     if (abs(ftNode->frameW - width) < 5) {
         if (abs(ftNode->frameH - height) < 5) {
            frame->fontW = ftNode->rtWidth;
            frame->fontH = ftNode->rtHeight;
            frame->ascent = ftNode->rtAscent;
            frame->descent = ftNode->rtDescent;
            if (isAxisNum) {
                frame->axis_fontW = ftNode->rtWidth;
                frame->axis_fontH = ftNode->rtHeight;
                frame->axis_ascent = ftNode->rtAscent;
                frame->axis_descent = ftNode->rtDescent;
            }
            return(1);
         }
     }
     lines = 20;
     for (n = 0; n < FONTARRAY; n++) {
         fw = ftNode->widths[n];
         fh = ftNode->heights[n];
         if (width >= fw * 70) {
             if (height >= fh * lines)
                  break;
         }
         lines = lines - 2;
     }
     if (n >= FONTARRAY)
         n = FONTARRAY - 1;
     frame->fontW = fw;
     frame->fontH = fh;
     frame->ascent = ftNode->ascents[n];
     frame->descent = ftNode->descents[n];
     ftNode->index = n;
     ftNode->rtWidth = fw;
     ftNode->rtHeight = fh;
     ftNode->rtAscent = frame->ascent;
     ftNode->rtDescent = frame->descent;
     ftNode->frameW = width;
     ftNode->frameH = height;
     frame->font = ftNode;
     if (isAxisNum) {
         frame->axis_fontW = frame->fontW;
         frame->axis_fontH = frame->fontH;
         frame->axis_ascent = frame->ascent;
         frame->axis_descent = frame->descent;
     }
     return(1);
#else
     return(0);
#endif
}


void set_vj_font_array(char *name, int index, int w, int h, int a, int d)
{
    int n;
    VJFONT *node;
    WIN_STRUCT *frame;

    if (vjFontList == NULL || index >= FONTARRAY || index < 0)
       return;
    if (strcmp(name, "RESET") == 0) {
       if (frame_info != NULL) {
           for (n = 0; n < frameNums; n++) {
               frame = (WIN_STRUCT *) &frame_info[n];
               if (frame->isClosed == 0) {
                   set_frame_font(frame, AxisNumFontName);
                   set_frame_font(frame, DefaultFontName);
               }
           }
       }
       return;
    }
    node = vjFontList;
    while (node != NULL) {
        if (strcmp(node->name, name) == 0)
            break;
        node = node->next;
    }
    if (node == NULL)
        return;
    node->widths[index] = w;
    node->heights[index] = h;
    node->ascents[index] = a;
    node->descents[index] = d;
    node->frameW = 0;
}

static VJFONT
*add_vj_font(VJFONT *list, char *name, char *font, int w, int h, int a, int d)
{
    int n;
    VJFONT *node, *pnode;

    pnode = NULL;
    node = list;
    while (node != NULL) {
        if (strcmp(node->name, name) == 0)
            break;
        pnode = node;
        node = node->next;
    }
    if (node == NULL) {
        node = (VJFONT *) calloc(1, sizeof(VJFONT));
        node->name = malloc(strlen(name) + 2);
        strcpy(node->name, name);
        if (pnode != NULL)
           pnode->next = node;
    }
    strcpy(node->font, font);
    if (h < 5)
        h = 15;
    if (a >= h) {
        a = h - 2;
        d = 2;
    }
    else if (a < 5) {
        a = h - 2;
        d = 2;
    }
    for (n = 0; n < FONTARRAY; n++) {
        node->widths[n] = w - n;
        node->heights[n] = h - n;
        node->ascents[n] = a - n;
        node->descents[n] = d;
    }
    node->ratioAscent = (double) a / (double) h;
    node->ratioWH = (double) w / (double) h;
    node->rtWidth = w;
    node->rtHeight = h;
    node->rtAscent = a;
    node->rtDescent = d;
    node->index = 0;
    return node;
}

static void
read_vj_font_list()
{
    FILE *fd;
    struct stat f_stat;
    VJFONT *node;
    char  *data, font[20];
    int  w, h, a, d, n;

    sprintf(tmpStr, "%s/persistence/.Fontlist", userdir);
    if (stat(tmpStr, &f_stat) < 0)
        return;
    if (vjFontList != NULL) {
        if (vjFontList->ftime == f_stat.st_mtime)
            return;
    }
    fd = fopen(tmpStr, "r");
    if (fd == NULL)
         return;
    scrnDpi = 90.0;
    a = 12;
    d = 3;
    w = 10;
    h = 15;
    while ((data = fgets(tmpStr, 200, fd)) != NULL) {
        if (sscanf(data, "%s%s%d%d%d%d", gmode, font, &w, &h, &a, &d) > 3) {
            node = add_vj_font(vjFontList, gmode, font, w, h, a, d);
            if (vjFontList == NULL)
                vjFontList = node;
            if (strncmp(node->name, "ScreenD", 7) == 0) {
                if (node->rtWidth  > 60)
                    scrnDpi = (double)node->rtWidth ;
            }
            else {
                if (strcmp(node->name, DefaultFontName) == 0)
                    defaultFont = node;
                else if (defaultFont == NULL)
                    defaultFont = node;
            }
        }
    }
    fclose(fd);
    vjFontList->ftime = f_stat.st_mtime;

    sprintf(tmpStr, "%s/persistence/.Fontarray", userdir);
    if (stat(tmpStr, &f_stat) < 0)
        return;
    fd = fopen(tmpStr, "r");
    if (fd == NULL)
        return;
    while ((data = fgets(tmpStr, 200, fd)) != NULL) {
        if (sscanf(data, "%s%d%d%d%d%d", gmode, &n, &w, &h, &a, &d) > 4) {
             set_vj_font_array(gmode, n, w, h, a, d);
        }
    }
    fclose(fd);
}

static void
create_vj_font_list()
{
     read_vj_font_list();
     if (defaultFont == NULL)
         defaultFont = add_vj_font(vjFontList,DefaultFontName,DefaultFontName,10,15,12,3);
     if (vjFontList == NULL)
         vjFontList = defaultFont;
}

static void
reset_frame_plot_fonts(double r) {
     VJFONT *ftNode = NULL;
     int  n;

     if (vjFontList == NULL)
         return;
     ftNode = vjFontList;
     while (ftNode != NULL) {
         for (n = 0; n < FONTARRAY; n++) {
             ftNode->widths[n] = (int) (r * ftNode->widths[n]);
             ftNode->heights[n] = (int) (r * ftNode->heights[n]);
             ftNode->ascents[n] = (int) (r * ftNode->ascents[n]);
             ftNode->descents[n] = (int) (r * ftNode->descents[n]);
         }
         n = 0;
         ftNode->rtWidth = ftNode->widths[n];
         ftNode->rtHeight = ftNode->heights[n];
         ftNode->rtAscent = ftNode->ascents[n];
         ftNode->rtDescent = ftNode->descents[n];
         ftNode->frameW = 1;
         ftNode->frameH = 1;
         ftNode->index = n;

         ftNode = ftNode->next;
     }
}

static void
set_font_size_with_frame(WIN_STRUCT *frame)
{
    if (frame == NULL || plot != 0)
        return;
     xcharpixels = frame->axis_fontW;
     ycharpixels = frame->axis_fontH;
     char_width = frame->fontW;
     char_height = frame->fontH;
     char_ascent = frame->ascent;
     char_descent = frame->descent;
}

static void
set_vj_font_size(int w, int h, int a, int d)
{
     if (w < 3 || h < 5)
         return;
     if (a < 3)
         a = h - 2;
     if (d < 1)
         d = 1;
     if (xcharpixels == char_width) {  // not in plot mode
         xcharpixels = w;
         ycharpixels = h;
     }
     if (csi_active == 0) {
          win_char_width = w;
          win_char_height = h;
          win_char_ascent = a;
          win_char_descent = d;
     }
     char_width = w;
     char_height = h;
     char_ascent = a;
     char_descent = d;
}

void set_font_size(int w, int h, int a, int d)
{
     if (inPrintSetupMode != 0) {
         prtCharWidth = w;
         prtCharHeight = h;
         if (a > 0)
             prtChar_ascent = a;
         if (d > 0)
             prtChar_descent = d;
         return;
      }

      if (vjFontList == NULL)
         create_vj_font_list();
      //  set_vj_font_size(w, h, a, d);
}

void
jgraphics_init()
{
     jtcolor.type = htonl(1);
     j0p.type = htonl(2);
     jx0p.type = htonl(2);
     j1p.type = htonl(3);
     j4p.type = htonl(3);
     j6p.type = htonl(4);
     j8p.type = htonl(4);
     jybar.type = htonl(5);
     /* jText.type = htonl(6);  */

     j1p.size = htonl(1);
     j4p.size = htonl(4);
     j6p.size = htonl(6);
     j8p.size = htonl(8);

     jybar.id = htonl(0);
     jybar.len = htonl(0);
     ybarHead = sizeof(int) * 4;

     intLen = sizeof(int);
     j1pLen = intLen * 4;
     j4pLen = intLen * 7;
     j6pLen = intLen * 9;
     j8pLen = intLen * 11;

     aipj0p.type = htonl(AIPCODE + 2);
     aipj1p.type = htonl(AIPCODE + 3);
     aipj4p.type = htonl(AIPCODE + 3);
     aipj6p.type = htonl(AIPCODE + 4);
     aipj8p.type = htonl(AIPCODE + 4);
     aipjybar.type = htonl(AIPCODE + 5);

     aipj1p.size = htonl(1);
     aipj4p.size = htonl(4);
     aipj6p.size = htonl(6);
     aipj8p.size = htonl(8);

     aipjybar.id = htonl(0);
     aipjybar.len = htonl(0);

     char_width = 8;
     char_height = 15;
     char_ascent = 12;
     char_descent = 3;
     xcharpixels = char_width;
     ycharpixels = char_height;

     if (main_frame_info == NULL)
         create_frame_info_struct(1);
     set_aip_win_size(0, 0, INIT_WIDTH, INIT_HEIGHT);
}

// on  > 0: turn on batch
//     = 0: turn off batch and refresh graphics window if necessary
static void
jBatch(int on)
{
     if (isPrintBg != 0 || inRepaintMode != 0)
        return;
     if (on) {
        if (in_batch == 0) {
            if (csidebug)
               fprintf(stderr, " batch  ON \n");
            jx0p.code = htonl(JBATCHON);
            graphToVnmrJ(&jx0p, intLen * 2);
        }
        batchCount = 1;
        in_batch = 1;
        return;
     }
     if (batchCount != 0) {
        if (in_batch == 0) {
            if (csidebug)
               fprintf(stderr, " batch  ON \n");
            jx0p.code = htonl(JBATCHON);
            graphToVnmrJ(&jx0p, intLen * 2);
        }
        if (csidebug)
            fprintf(stderr, " batch  OFF \n");
        jx0p.code = htonl(JBATCHOFF);
        graphToVnmrJ(&jx0p, intLen * 2);
     }
     batchCount = 0;
     in_batch = 0;
}

static void
aipBatch(int on)
{
     if (csi_opened < 1) {
        jBatch(on);
        return;
     }
        
     if (isPrintBg != 0 || inRepaintMode != 0)
        return;
     if (on) {
        if (aip_in_batch == 0) {
            if (csidebug)
                fprintf(stderr, " aip batch  ON \n");
            aipj0p.code = htonl(JBATCHON);
            graphToVnmrJ(&aipj0p, intLen * 2);
        }
        aip_batchCount = 1;
        aip_in_batch = 1;
        return;
     }
     if (aip_batchCount != 0) {
        if (aip_in_batch == 0) {
            if (csidebug)
                fprintf(stderr, " aip batch  ON \n");
            aipj0p.code = htonl(JBATCHON);
            graphToVnmrJ(&aipj0p, intLen * 2);
        }
        if (csidebug)
            fprintf(stderr, " aip batch  OFF \n");
        aipj0p.code = htonl(JBATCHOFF);
        graphToVnmrJ(&aipj0p, intLen * 2);
     }
     aip_batchCount = 0;
     aip_in_batch = 0;
}

// enforce vj reapint (includes csi window)
static void
jPaintAll()
{
     if (isPrintBg != 0 || inRepaintMode != 0)
        return;
     if (batchCount != 0) {
        if (in_batch == 0) {
            if (csidebug)
                fprintf(stderr, "  paint batch  ON \n");
            jx0p.code = htonl(JBATCHON);
            graphToVnmrJ(&jx0p, intLen * 2);
        }
        if (csidebug)
            fprintf(stderr, "  paint all \n");
        jx0p.code = htonl(WINPAINT);
        graphToVnmrJ(&jx0p, intLen * 2);
     }
     batchCount = 0;
     in_batch = 0;
}

static void
end_iplot_func()
{
     if (iplotFd != NULL)
         fprintf(iplotFd, "#endfunc\n");
}

static void
j1p_func(int f, int p)
{
     // if (isSuspend)
     //    return;
     if (iplotFd != NULL) {
         fprintf(iplotFd, "%s %d 1 %d\n",iFUNC, f, p);
         return;
     }
     if (in_batch == 0)
         jBatch(1);
     j1p.code = htonl(f);
     j1p.p = htonl(p);
     graphToVnmrJ(&j1p, j1pLen);
}

static void
aip_j1p_func(int f, int p)
{
     if (iplotFd != NULL) {
         fprintf(iplotFd, "%s %d 1 %d\n", AIPFUNC, f, p);
         // end_iplot_func();
         return;
     }
     if (in_batch == 0)
         jBatch(1);
     aipj1p.code = htonl(f);
     aipj1p.p = htonl(p);
     graphToVnmrJ(&aipj1p, j1pLen);
}

static void
j4p_func(int f, int p0, int p1, int p2, int p3)
{
     // if (isSuspend)
     //    return;
     if (iplotFd != NULL) {
         fprintf(iplotFd, "%s %d 4 %d %d %d %d\n",iFUNC, f, p0, p1, p2, p3);
         // end_iplot_func();
         return;
     }
     if (in_batch == 0)
         jBatch(1);
     j4p.code = htonl(f);
     j4p.p[0] = htonl(p0);
     j4p.p[1] = htonl(p1);
     j4p.p[2] = htonl(p2);
     j4p.p[3] = htonl(p3);
     graphToVnmrJ(&j4p, j4pLen);
     isGraphFunc = 1;
}

static void
j4xp_func(int f, int p0, int p1, int p2, int p3)
{
     // if (isSuspend)
     //    return;
     if (iplotFd != NULL) {
         fprintf(iplotFd, "%s %d 4 %d %d %d %d\n", iFUNC, f, p0, p1, p2, p3);
         // end_iplot_func();
         return;
     }
     j4p.code = htonl(f);
     j4p.p[0] = htonl(p0);
     j4p.p[1] = htonl(p1);
     j4p.p[2] = htonl(p2);
     j4p.p[3] = htonl(p3);
     graphToVnmrJ(&j4p, j4pLen);
}

static void
aip_j4p_func(int f, int p0, int p1, int p2, int p3)
{
     if (iplotFd != NULL) {
         fprintf(iplotFd, "%s %d 4 %d %d %d %d\n", AIPFUNC, f, p0, p1, p2, p3);
         // end_iplot_func();
         return;
     }
     if (in_batch == 0)
         jBatch(1);
     aipj4p.code = htonl(f);
     aipj4p.p[0] = htonl(p0);
     aipj4p.p[1] = htonl(p1);
     aipj4p.p[2] = htonl(p2);
     aipj4p.p[3] = htonl(p3);
     graphToVnmrJ(&aipj4p, j4pLen);
     isGraphFunc = 1;
}

static void
j6p_func(int f, int p0, int p1, int p2, int p3, int p4, int p5)
{
     // if (isSuspend)
     //    return;
     if (iplotFd != NULL) {
         fprintf(iplotFd, "%s %d 6 %d %d %d %d %d %d\n",iFUNC, f,p0,p1,p2,p3,p4,p5);
         // end_iplot_func();
         return;
     }
     if (in_batch == 0)
         jBatch(1);
     j6p.code = htonl(f);
     j6p.p[0] = htonl(p0);
     j6p.p[1] = htonl(p1);
     j6p.p[2] = htonl(p2);
     j6p.p[3] = htonl(p3);
     j6p.p[4] = htonl(p4);
     j6p.p[5] = htonl(p5);
     graphToVnmrJ(&j6p, j6pLen);
     isGraphFunc = 1;
}

static void
j6p_xfunc(int f, int p0, int p1, int p2, int p3, int p4, int p5)
{
     if (iplotFd != NULL) {
         fprintf(iplotFd, "%s %d 6 %d %d %d %d %d %d\n",iFUNC, f,p0,p1,p2,p3,p4,p5);
         // end_iplot_func();
         return;
     }
     j6p.code = htonl(f);
     j6p.p[0] = htonl(p0);
     j6p.p[1] = htonl(p1);
     j6p.p[2] = htonl(p2);
     j6p.p[3] = htonl(p3);
     j6p.p[4] = htonl(p4);
     j6p.p[5] = htonl(p5);
     graphToVnmrJ(&j6p, j6pLen);
}

static void
aip_j6p_func(int f, int p0, int p1, int p2, int p3, int p4, int p5)
{
     if (iplotFd != NULL) {
         fprintf(iplotFd, "%s %d 6 %d %d %d %d %d %d\n",AIPFUNC, f,p0,p1,p2,p3,p4,p5);
         // end_iplot_func();
         return;
     }
     if (in_batch == 0)
         jBatch(1);
     aipj6p.code = htonl(f);
     aipj6p.p[0] = htonl(p0);
     aipj6p.p[1] = htonl(p1);
     aipj6p.p[2] = htonl(p2);
     aipj6p.p[3] = htonl(p3);
     aipj6p.p[4] = htonl(p4);
     aipj6p.p[5] = htonl(p5);
     graphToVnmrJ(&aipj6p, j6pLen);
     isGraphFunc = 1;
}

/**
static void
j8p_func(int f, int p0, int p1, int p2, int p3, int p4,
         int p5, int p6, int p7)
{
     // if (isSuspend)
     //    return;
     j8p.code = htonl(f);
     j8p.p[0] = htonl(p0);
     j8p.p[1] = htonl(p1);
     j8p.p[2] = htonl(p2);
     j8p.p[3] = htonl(p3);
     j8p.p[4] = htonl(p4);
     j8p.p[5] = htonl(p5);
     j8p.p[6] = htonl(p6);
     j8p.p[7] = htonl(p7);
     graphToVnmrJ(&j8p, j8pLen);
     isGraphFunc = 1;
}
***/

static void
aip_j8p_func(int f, int p0, int p1, int p2, int p3, int p4,
         int p5, int p6, int p7)
{
     if (iplotFd != NULL) {
         fprintf(iplotFd, "%s %d 8 %d %d %d %d %d %d ",AIPFUNC, f,p0,p1,p2,p3,p4,p5);
         fprintf(iplotFd, "%d  %d\n",p6, p7);
         // end_iplot_func();
         return;
     }
     aipj8p.code = htonl(f);
     aipj8p.p[0] = htonl(p0);
     aipj8p.p[1] = htonl(p1);
     aipj8p.p[2] = htonl(p2);
     aipj8p.p[3] = htonl(p3);
     aipj8p.p[4] = htonl(p4);
     aipj8p.p[5] = htonl(p5);
     aipj8p.p[6] = htonl(p6);
     aipj8p.p[7] = htonl(p7);
     graphToVnmrJ(&aipj8p, j8pLen);
     isGraphFunc = 1;
}

static void
aip_Polyline(Dpoint_t *pnts, int npts)
{
    int k, k2, len;
    int p;
    short  *dxp, *dyp, x, y; 

    if (npts < 1 || pnts == NULL)
        return;
    if (iplotFd != NULL) {
        fprintf(iplotFd, "%s  %d 2 %d  0\n", AIPFUNC, IPOLYLINE, npts);
        k2 = 0;
        for (k = 0; k < npts; k++) {
            fprintf(iplotFd, "%d %d ", (int) pnts[k].x, (int) pnts[k].y);
            k2++;
            if (k2 >= 40) {
                fprintf(iplotFd, "\n");
                k2 = 0;
            }
        }
        if (k2 > 0)
            fprintf(iplotFd, "\n");
        end_iplot_func();
        return;
    }
    k = 0;
    aipjybar.subtype = htonl(IPOLYLINE);
    while (k < npts) {
        len = npts - k;
        if (len < 2)
            break;
        if (len > (JLEN - 4))
            len = JLEN - 4;
        dxp = &aipjybar.data[2];
        dyp = &aipjybar.data[3];
        p = k + len;
        for (k2 = k; k2 < p; k2++) {
             // *dxp = htons((short) pnts[k2].x);
             // *dyp = htons((short) pnts[k2].y);
             x = (short) pnts[k2].x;
             y = (short) pnts[k2].y;
             if (x < 0)
                 x = 0;
             if (y < 0)
                 y = 0;
             *dxp = htons(x);
             *dyp = htons(y);
             dxp += 2;
             dyp += 2;
        }
        aipjybar.data[0] = htons((short)len);
        aipjybar.data[1] = 0;
        len++;
        aipjybar.len = htonl(len);
        p = ybarHead + sizeof(short) * len * 2;
        graphToVnmrJ(&aipjybar, p);
        k = k + len - 1;
    }
    isGraphFunc = 1;
}

static void
aip_Polygon_2(Gpoint_t *pnts, int npts)
{
    int k, k2, len;
    int p;
    short  *dxp, *dyp, x, y; 

    if (npts < 1 || pnts == NULL)
        return;
    if (iplotFd != NULL) {
        fprintf(iplotFd, "%s  %d 2 %d  0\n", AIPFUNC, IPOLYGON, npts);
        k2 = 0;
        for (k = 0; k < npts; k++) {
            fprintf(iplotFd, "%d %d ", pnts[k].x, pnts[k].y);
            k2++;
            if (k2 >= 40) {
                fprintf(iplotFd, "\n");
                k2 = 0;
            }
        }
        if (k2 > 0)
            fprintf(iplotFd, "\n");
        end_iplot_func();
        return;
    }

    k = 0;
    aipjybar.subtype = htonl(IPOLYGON);
    while (k < npts) {
        len = npts - k;
        if (len < 2)
            break;
        if (len > (JLEN - 4))
            len = JLEN - 4;
        dxp = &aipjybar.data[2];
        dyp = &aipjybar.data[3];
        p = k + len;
        for (k2 = k; k2 < p; k2++) {
             // *dxp = htons((short) pnts[k2].x);
             // *dyp = htons((short) pnts[k2].y);
             x = (short) pnts[k2].x;
             y = (short) pnts[k2].y;
             if (x < 0)
                 x = 0;
             if (y < 0)
                 y = 0;
             *dxp = htons(x);
             *dyp = htons(y);
             dxp += 2;
             dyp += 2;
        }
        aipjybar.data[0] = htons((short)len);
        aipjybar.data[1] = 0;
        len++;
        aipjybar.len = htonl(len);
        p = ybarHead + sizeof(short) * len * 2;
        graphToVnmrJ(&aipjybar, p);
        k = k + len - 1;
    }
    isGraphFunc = 1;
}

static void
aip_Polygon_3(int id, float2 *pnts, int npts)
{
    int k, k2, len;
    int p;
    short  *dxp, *dyp; 

    if (npts < 1 || pnts == NULL)
        return;
    if (iplotFd != NULL) {
        fprintf(iplotFd, "%s  %d 2 %d 0\n", iFUNC, JPOLYGON, npts);
        k2 = 0;
        for (k = 0; k < npts; k++) {
            fprintf(iplotFd, "%d ", (int) pnts[k2][0]);
            fprintf(iplotFd, "%d ", aip_mnumypnts - (int) pnts[k2][1]);
            k2++;
            if (k2 >= 40) {
                fprintf(iplotFd, "\n");
                k2 = 0;
            }
        }
        if (k2 > 0)
            fprintf(iplotFd, "\n");
        end_iplot_func();
        return;
    }

    k = 0;
    aipjybar.subtype = htonl(JPOLYGON);
    aipjybar.id = htonl(id);
    while (k < npts) {
        len = npts - k;
        if (len < 2)
            break;
        if (len > (JLEN - 4))
            len = JLEN - 4;
        dxp = &aipjybar.data[2];
        dyp = &aipjybar.data[3];
        p = k + len;
        for (k2 = k; k2 < p; k2++) {
             *dxp = htons((short) pnts[k2][0]);
             *dyp = htons((short) (aip_mnumypnts - (int)pnts[k2][1]));
             dxp += 2;
             dyp += 2;
        }
        aipjybar.data[0] = htons((short)len);
        aipjybar.data[1] = 0;
        len++;
        aipjybar.len = htonl(len);
        p = ybarHead + sizeof(short) * len * 2;
        graphToVnmrJ(&aipjybar, p);
        k = k + len - 1;
    }
    isGraphFunc = 1;
}



/* copy from backup pixmap to window */
static void
jCopyArea(int x, int y, int w, int h)
{
    if (csidebug > 1)
        fprintf(stderr," jCopyArea  %d %d %d %d\n", x, y, w, h);
     if (aip_xid > 0)
         aip_j4p_func(JCOPY2, x, y, w, h);
     else
         j4xp_func(JCOPY2, x, y, w, h);
}

void jSuspendGraphics(int s)
{
#ifdef MOTIF
/*
        XWindowAttributes  attr;
 */

        if (!useXFunc)
             return;
	isSuspend += s;
	if (isSuspend <= 0) {
	     isSuspend = 0;
	     winDraw = org_winDraw;
             if (useXFunc) {
                 if (orgx < 1 || xregion == NULL)
	            winDraw = 0;
             }
	}
	else {
	        winDraw = 0;
	}
/*
        if (win_show && isSuspend > 0) {
             XGetWindowAttributes(xdisplay,  xid, &attr);
        }
*/
#endif
}

int edit_color_ok()
{
      return(dynamic_color);
}

static void 
jvnmr_cmd(int cmd)
{
      if (iplotFd != NULL) {
          fprintf(iplotFd, "%s %d 1 0 \n",iFUNC, cmd);
          return;
      }
      j0p.code = htonl(cmd);
      graphToVnmrJ(&j0p, intLen * 2);
}

static void 
aip_cmd(int cmd)
{
      if (iplotFd != NULL) {
          fprintf(iplotFd, "%s %d 1 0 \n",AIPFUNC, cmd);
          return;
      }
      aipj0p.code = htonl(cmd);
      graphToVnmrJ(&aipj0p, intLen * 2);
}

static void
copy_grid_windows()
{
    if (grid_num <= 1)
        return;
#ifdef MOTIF
    if (useXFunc) {
        int  m;
        WIN_STRUCT *a;

        for ( m = 0; m < grid_num; m++) {
           a = &win_dim[m];
           if (a->xmap) {
               XCopyArea(xdisplay, a->xmap, org_Pixmap, grid_gc, 0, 0,
                        a->width, a->height, a->x, a->y);
               XCopyArea(xdisplay, a->xmap, xid, vnmr_gc, 0, 0,
                        a->width, a->height, a->x+org_orgx, a->y+org_orgy);
           }
        }
    }
#endif
}

/* dummy functions for image planning */
void
popBackingStore()
{
   if (isWinRedraw != 0)
        return;
   if (useXFunc) {
#ifdef MOTIF
	if (canvasPixmap && winDraw) {
           XCopyArea(xdisplay, canvasPixmap, xid, vnmr_gc, 0, 0,
                        graf_width, graf_height, orgx, orgy);
        }
#endif
   }
   else {
	// jvnmr_cmd(JCOPY);
        batchCount = 1;
        jBatch(0);
   }
}

void
toggleBackingStore(int onoff)
{
      bksflag = onoff;
      xmapDraw = 0;
      if (onoff > 0 && canvasPixmap != 0)
          xmapDraw = 1;
}

int
getBackingStoreFlag()
{
    return bksflag;
}

int
getBackingWidth()
{
    return backing_width;
}

int
getBackingHeight()
{
    return backing_height;
}

int
isJprintMode()
{
     return isPrintBg;
}

double
getScreenDpi()
{
    return scrnDpi;
}

void set_main_win_size(int x, int y, int w, int h)
{
        if (w < 10)
            w = 10;
        if (h < 10)
            h = 10;
        if (inPrintSetupMode != 0) {
            prtWidth = w;
            prtHeight = h;
            return;
        }
        main_orgx = x;
        main_orgy = y;
        main_win_width = w;
        main_win_height = h;
        win_width = w;
        win_height = h;
        if (main_frame_info == NULL)
            create_frame_info_struct(1);
        main_frame_info->x = x;
        main_frame_info->y = y;
        if (w != main_frame_info->width || h != main_frame_info->height) {
            main_frame_info->width = w;
            main_frame_info->height = h;
            set_frame_font(main_frame_info, AxisNumFontName);
            set_frame_font(main_frame_info, DefaultFontName);
        }
}


void jvnmr_sync(int n, int is_aip)
{
       int pid = 0;
       int k;
       double rh;
       WIN_STRUCT  *f;

       if (n == 0) {
          if (is_aip)
              aip_cmd(JOK);
          else
              jvnmr_cmd(JOK);
          return;
       }
       if (n == 3) {
          inRsizesMode = 1;
          return;
       }
       if (n == 4) {
          inRsizesMode = 0;
          return;
       }
       if (n == 5) {
          inPrintSetupMode = 0;
          setVjGUiPort(1); // turn on graphics socket port
          return;
       }
       if (n == 1) {  // start print canvas
          inPrintSetupMode = 1;
          return;
       }
       if (n == 2) {  // end print canvas
          inPrintSetupMode = 0;
          if (prtWidth < 10 || prtHeight < 10)
          {
              jvnmr_cmd(JSYNC);
              return;
          }
          openPrintPort();
          if ((pid = fork()) < 0)
          {
              showError("Could not fork background process to do print!");
              jvnmr_cmd(JSYNC);
              return;
          }
	  if (pid != 0) {
              // setVjGUiPort(0); // turn off graphics socket port
              set_wait_child(pid); 
          }
	  if (pid == 0)
          {
               isPrintBg = 1;
               transCount = 0;
               plot = 1;
               if (xdebug > 0)
                   fprintf(stderr, "start paint vnmrbg %d \n", VnmrJViewId);
#ifdef OLD
               lastPlotCmd[0] = '\0';
#endif
               inRsizesMode = 1;
               setPrintScrnMode(1);
               setVjUiPort(0);  // turn off UI socket port
               setVjPrintMode(1);  // turn on print socket port
               curPalette = 0;
               create_vj_font_list();
               rh = (double) prtCharHeight / (double) defaultFont->heights[0];
               if (rh < 0.5)
                  rh = 0.5;
               if (rh > 20.0)
                  rh = 20.0;
               reset_frame_plot_fonts(rh);
               xcharpixels = char_width;
               ycharpixels = char_height;
               set_font_size(prtCharWidth,prtCharHeight,prtChar_ascent,prtChar_descent);
               set_aip_win_size(0, 0, prtWidth, prtHeight);
               set_main_win_size(0, 0, prtWidth, prtHeight);
               if (frame_info != NULL && active_frame > 0) {
                  for (k = 0; k < frameNums; k++) {
                      f = (WIN_STRUCT *) &frame_info[k];
                      if (f != NULL) {
                           f->x = f->prtX;
                           f->y = f->prtY;
                           f->width = f->prtW;
                           f->height = f->prtH;
                           f->fontW = f->prtFontW;
                           f->fontH = f->prtFontH;
                           f->ascent = f->prtAscent;
                           f->descent = f->prtDescent;
                           f->axis_fontW = f->prtFontW;
                           f->axis_fontH = f->prtFontH;
                           f->axis_ascent = f->prtAscent;
                           f->axis_descent = f->prtDescent;
                           f->font = NULL;
                           if (f->isClosed == 0) {
                               set_frame_font(f, AxisNumFontName);
                               set_frame_font(f, DefaultFontName);
                           }
                      }
                  }
               }
               set_frame_font(main_frame_info, AxisNumFontName);
               set_frame_font(main_frame_info, DefaultFontName);
               inRsizesMode = 0;
               Wturnoff_message();
               isWinRedraw = 1;
               initWinRedraw = 1;
               repaint_frames(1, 1);
/*
               execString("repaint('all')\n");
*/

               if (xdebug > 0)
                   fprintf(stderr, "end paint vnmrbg %d \n", VnmrJViewId);
               jvnmr_cmd(JSYNC);
               exit(1);
          }
       }
}

int 
get_vj_overlayType()
{
    return overlayType;
}

void jset_overlayType(int n)
{
      overlayType = n;
}

void jset_csi_overlayType(int n)
{
      // overlayType = n;
}

void
jset_overlaySpecInfo(char *cmd, int overlayMode, char *jvpInfo, 
	int activeWin, int alignvp, int chartMode, double spx, double wpx,
	double spy, double wpy, double sp2, double wp2, double sp1, double wp1,char *axis2,char *axis1)
{
//    if(strcasecmp(s_jvpInfo,jvpInfo) != 0) {
        P_setstring(GLOBAL,"jvpinfo",jvpInfo,1);
        appendJvarlist("jvpinfo");
        // writelineToVnmrJ("pnew 1", "jvpinfo"); 
//    }
    if(s_overlayMode != overlayMode) {
        P_setreal(GLOBAL,"overlayMode",(double)overlayMode,1);
        appendJvarlist("overlayMode");
        // writelineToVnmrJ("pnew 1", "overlayMode"); 
    }

     s_chartMode = chartMode;
       s_overlayMode = overlayMode;
       strncpy(s_jvpInfo, jvpInfo, 10);
       s_activeWin = activeWin+1;
       s_alignvp = alignvp+1;
     if(s_axis2 == NULL || (strcmp(axis2,"null") != 0 && strlen(axis2)>0)) strcpy(s_axis2,axis2);
     if(s_axis1 == NULL || (strcmp(axis1,"null") != 0 && strlen(axis1)>0)) strcpy(s_axis1,axis1);
     if(wpx > 0 || wpy > 0) {
       s_spx = spx;
       s_wpx = wpx;
       s_spy = spy;
       s_wpy = wpy;
       s_sp2 = sp2;
       s_wp2 = wp2;
       s_sp1 = sp1;
       s_wp1 = wp1;
     }
/*
Winfoprintf("jset_overlaySpecInfo %s %d %s %d %d %d %d %f %f %f %f %f %f %f %f\n",
	cmd, overlayMode, jvpInfo,VnmrJViewId, 
	s_activeWin,s_alignvp,chartMode,spx,wpx,spy,wpy,sp2,wp2,sp1,wp1);
*/
     if(strcmp(cmd,"init") == 0 || strcmp(cmd,"update") == 0) return;
     if(overlayMode > OVERLAID_NOTALIGNED && s_alignvp != VnmrJViewId) return;

     overlaySpec(s_overlayMode);

}

void
jset_csi_overlaySpecInfo(char *cmd, int overlayMode, char *jvpInfo, 
	int activeWin, int alignvp, int chartMode, double spx, double wpx,
	double spy, double wpy, double sp2, double wp2, double sp1, double wp1)
{
}

int getOverlayMode() {
   return s_overlayMode;
}

int getActiveWin() {
   return s_activeWin;
}

int getAlignvp() {
   return s_alignvp;
}

int getChartMode() {
   return s_chartMode;
}

void getSweepInfo(double *spx, double *wpx, double *spy, double *wpy, char *axis2, char *axis1) {
     *spx = s_spx;
     *wpx = s_wpx;
     *spy = s_spy;
     *wpy = s_wpy;
     strcpy(axis2,s_axis2);
     strcpy(axis1,s_axis1);
}
 
void getSpecInfo(double *spx, double *wpx, double *spy, double *wpy, char *axis2, char *axis1) {
     *spx = s_sp2;
     *wpx = s_wp2;
     *spy = s_sp1;
     *wpy = s_wp1;
     strcpy(axis2,s_axis2);
     strcpy(axis1,s_axis1);
}
 
void
jvnmr_init() /* initialize file descriptor */
{
	jvnmr_cmd(JCLEAR);
        create_vj_font_list();
/*	sync(); */
/*	flush( Gphsd ); */
}

/*----------------------------------------------------------------------*/
/*  routines for hourglass and default cursors.                         */
/*----------------------------------------------------------------------*/
void set_hourglass_cursor()
{
	hourglass_on = 1;
	// alarm(1);
        jvnmr_cmd(JWINBUSY);
}

void restore_original_cursor()
{
	if (!useXFunc) {
	    jvnmr_cmd(JWINFREE);
	    return;
	}
	if (hourglass_on < 2) {
	    hourglass_on = 0;
	    return;
	}
	hourglass_on = 0;
	// alarm(0);
/**
#ifdef MOTIF
	if (useXFunc) {
	    if (orgCursor == 0)
		orgCursor = XCreateFontCursor(xdisplay, XC_left_ptr);
	    if (orgCursor != 0) {
                XSynchronize(xdisplay, 1);
                XDefineCursor(xdisplay, xid, orgCursor);
                XSynchronize(xdisplay, 0);
	    }
	}
#endif
**/
 	jvnmr_cmd(JWINFREE);
}

void send_hourglass_cursor()
{
        TIME_NODE  *tnode;
        struct timeval clock;
        struct timezone tzone;
        long   sec, usec;

        tnode = timeOut_list;
        if (tnode != NULL) {
            gettimeofday(&clock,&tzone);
            while (tnode != NULL) {
                if (tnode->active > 0) {
                   sec = clock.tv_sec - tnode->sec;
                   usec = clock.tv_usec - tnode->usec;
                   if (sec > tnode->tv_sec)
                       break;
                   if (sec >= tnode->tv_sec) {
                       if (usec >= tnode->tv_usec)
                           break;
                   }
                }
                tnode = tnode->next;
            }
            if (tnode != NULL) {
                tnode->active = 0;
                tnode->func(tnode->retVal, NULL);
	        return;
            }
        }

	if (hourglass_on == 1)
	{
	  hourglass_on = 2;
	  jvnmr_cmd(JWINBUSY);
	  return;
	}
	else /* this branch never called? keep as failsafe */
	{
	  jvnmr_cmd(JWINFREE);
	}
}

static void
drawWinBorder(WIN_STRUCT *win)
{
     if (!useXFunc)
          return;
#ifdef MOTIF
     if (!winDraw)
          return;
     if (win == NULL) {
          if (curActiveWin == NULL)
              return;
          win = curActiveWin;
     }
     if (win->active)
          XSetForeground(xdisplay, grid_gc, active_pix);
     else
          XSetForeground(xdisplay, grid_gc, idle_pix);
     XDrawRectangle(xdisplay, xid, grid_gc, win->x + org_orgx - grid_width,
          win->y + org_orgy - grid_width, win->width+grid_width, win->height+grid_width);
#endif
}

void
sun_sunGraphClear()
{
   int  i;

   for (i = 0; i < 3; i++)
   {
        x_cursor_pos[i] = 0;
        y_cursor_pos[i] = 0;
   }
   x_cross_pos = 0;
   y_cross_pos = 0;
/*
   if (curwin <= 0)
	return;
*/
   clearOverlayMap = 1;

#ifdef INTERACT
   BACK = BLACK;
#endif 
   isNewGraph = 1;
   normalmode();
   // clearGraphFunc();
   if (useXFunc) {
#ifdef MOTIF
	if (orgx < 0 || xregion == NULL)
	    return;
	if (winDraw) {
           XSetForeground(xdisplay, vnmr_gc, xblack);
           XFillRectangle(xdisplay, xid, vnmr_gc, orgx, orgy, graf_width,
                        graf_height);
           XSetForeground(xdisplay, vnmr_gc, xcolor);
	}
        if (canvasPixmap != 0) {
           XSetForeground(xdisplay, pxm_gc, xblack);
           XFillRectangle(xdisplay, canvasPixmap, pxm_gc, 0, 0, graf_width,
                        graf_height);
           XSetForeground(xdisplay, pxm_gc, xcolor);
        }
        if (grid_num > 1)
           drawWinBorder(NULL);
#endif
   }
   else {
        jBatch(1);
	jvnmr_cmd(JCLEAR);
        isGraphFunc = 0;
        if (isPrintBg) {
           if (printData != NULL)
               free(printData);
           printData = NULL;
        }
   }
   if (!keepOverlay) {
        // overlayNum = 0;
        // clear_overlay_image();
   }
}

void
default_sunGraphClear() {}



Pixel
get_vnmr_pixel(int c)
{
  if (!useXFunc)
     return (1);
  if (c < 0)
        c = BACK;
  if (c >= CMS_VNMRSIZE)
        c = CMS_VNMRSIZE - 1;
  return(sun_colors[c]);
}

/*****************/
void
sun_raster_color(int c, int dum)
/*****************/
{

/*  In XOR mode, the color index must be XOR'ed with BLACK because
    the background pixels will each have a color index of BLACK, a
    non-zero value.  See the routines to set XOR mode, too.	*/

  if (c < 0)
        c = BACK;
  g_color = c;
  if (!useXFunc) {
	j1p_func(JCOLORI, c);
        if (csi_opened)
            aip_j1p_func(JCOLORI, c);
	return;
  }
#ifdef MOTIF
  xcolor = sun_colors[c];
  if (c > CMS_VNMRSIZE)
        xcolor = sun_colors[BACK];
  if (xorflag)
        xcolor = xcolor ^ sun_colors[BACK];
  XSetForeground(xdisplay, vnmr_gc, xcolor);
  XSetForeground(xdisplay, pxm_gc, xcolor);
#endif 
}

/*****************/
void raster_color(int c, int dum)
/*****************/
{
  if (c <= 0 || c == BACK || c == BLACK)
        c = 0;

  raster_plot_color = get_raster_color(c);
  if (xorflag)
        xcolor = xcolor ^ c;
  else
        xcolor = c;
}


/**************/
void default_color(int c)
/**************/
{
  if (Wissun())
  {
#ifdef INTERACT
	if(BACK != WHITE)
	{
           if (c > 0)
                xcolor = xwhite;
           else
                xcolor = xblack;
           if (xorflag)
                xcolor = xcolor ^ xblack;
	}
	else
	{
           if (c > 0 && c != WHITE)
                xcolor = xblack;
           else
                xcolor = xwhite;
           if (xorflag)
                xcolor = xcolor ^ xwhite;
	}
#else 
        if (!useXFunc) {
	    j1p_func(JCOLORI, c);
            if (csi_opened)
                aip_j1p_func(JCOLORI, c);
	    return;
        }
        if (c > 0)
                xcolor = xwhite;
        else
                xcolor = xblack;
        if (xorflag)
                xcolor = xcolor ^ xblack;
#endif 
#ifdef MOTIF 
        XSetForeground(xdisplay, vnmr_gc, xcolor);
        XSetForeground(xdisplay, pxm_gc, xcolor);
#endif 
  }
}


/*****************/   /* no sun provision yet */
int default_charsize(double f)
/*****************/
{ 
   return (10);
}

void
sun_charsize(double f)
{ /* character size */
}


int
get_text_width(const char *text)
{
    int w;

#ifdef MOTIF 
    if (xstruct != NULL)
        w = XTextWidth(xstruct, text, (int)strlen(text));
    else
        w = xcharpixels * strlen(text);
#else
    w = xcharpixels * strlen(text);
#endif
    return (w);
}

int
get_font_ascent()
{
    return (char_ascent);
}

int
get_font_descent()
{
#ifdef MOTIF 
    if (xstruct != NULL)
        return (xstruct->max_bounds.descent);
#endif
    return (char_descent);
}

int
get_font_width()
{
    return (xcharpixels);
}

int
get_font_height()
{
    return (ycharpixels);
}

int
get_window_width()
{
    return (win_width);
}

int
get_window_height()
{
    return (win_height);
}

int
get_csi_window_width()
{
    return (csi_win_width);
}

int
get_csi_window_height()
{
    return (csi_win_height);
}

int
get_frame_width()
{
    return (graf_width);
}

int
get_frame_height()
{
    return (graf_height);
}

int
get_csi_frame_width()
{
    return (csi_graf_width);
}

int
get_csi_frame_height()
{
    return (csi_graf_height);
}

void
clear_x_font(XFontStruct *fStruct)
{
#ifdef MOTIF 
    if (fStruct == NULL || (!useXFunc))
        return;
    XUnloadFont(xdisplay, fStruct->fid);
    XFreeFontInfo(NULL, fStruct, 1);
#endif
}

void
set_x_font(XFontStruct *fontStruct)
{
#ifdef MOTIF 
    if (fontStruct == NULL || (!useXFunc))
        return;
    xstruct = fontStruct;
    x_font = fontStruct->fid;
    char_ascent = fontStruct->max_bounds.ascent;
    char_descent = fontStruct->max_bounds.descent;
    char_width = fontStruct->max_bounds.width;
    char_height = char_ascent + char_descent;
    xcharpixels = char_width;
    ycharpixels = char_height;
    XSetFont(xdisplay, vnmr_gc, x_font);
    XSetFont(xdisplay, pxm_gc, x_font);
#endif
}

void
save_origin_font()
{
#ifdef MOTIF
     if (useXFunc)
         org_xstruct = xstruct;
#endif
}

void
recover_origin_font()
{
#ifdef MOTIF
     if (org_xstruct != NULL)
     {
        set_x_font(org_xstruct);
     }
#endif
}

static void
addPrintLineBox(int type, int x1,int y1, int x2, int y2)
{
    PRTDATA *prtData;
    PRTDATA *newData;
    PRTLINEBOX *pline;
    int     isEqual;
    
    prtData = printData;
    isEqual = 0;
    while (prtData != NULL) {
        if (prtData->type == type) {
            pline = (PRTLINEBOX *) &prtData->data.lineBox;
            if (pline->x1 == x1 && pline->x2 == x2) {
                if (pline->y1 == y1 && pline->y2 == y2)
                   isEqual = 1;
            }
        }
        if (isEqual)
            break;
        prtData = prtData->next;
    }
    if (isEqual) {
        prtData->type = 0;
        return;
    }
    newData = (PRTDATA *) malloc(sizeof(PRTDATA));
    if (newData == NULL)
        return;
    newData->type = type;
    newData->color = g_color;
    newData->next = NULL;
    pline = (PRTLINEBOX *) &newData->data.lineBox;
    pline->x1 = x1;
    pline->x2 = x2;
    pline->y1 = y1;
    pline->y2 = y2;
    if (printData == NULL) {
        printData = newData;
        return;
    }
    prtData = printData;
    while (prtData->next != NULL)
        prtData = prtData->next;
    prtData->next = newData;
}


static void
draw_jline(int dx0, int dy0, int dx1, int dy1) /* draw single line segment */
{
      j4p_func(JLINE, dx0,dy0,dx1,dy1);
}

void sun_rdraw(int x, int y)
{
/*
   if (curwin <= 0)
	return;
*/
   gx1 = xgrx + orgx;
   my1 = mnumypnts - xgry - 1;
   gy1 = my1 + orgy;
   mx1 = xgrx;
   xgrx += x;
   xgry += y;
   gx2 = xgrx + orgx;
   my2 = mnumypnts - xgry - 1;
   gy2 = my2 + orgy;
   mx2 = xgrx;
   if (!useXFunc) {
       if (isPrintBg != 0 && xorflag != 0)
           addPrintLineBox(JLINE, mx1, my1, mx2, my2);
       else
           j4p_func(JLINE, mx1, my1, mx2, my2);
       return;
   }
#ifdef MOTIF
   if (winDraw)
      XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy2);
   if (xmapDraw)
      XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx2, my2);
#endif
}

#ifdef MOTIF
XSegment  x_segments[200];
XSegment  m_segments[200];
#endif

static int   adrawX = -1;
static int   adrawY = -1;
static int   dp_index = 0;
static int   dp_max = 0;
static uint16_t *dpconI;
static uint16_t *dpconLen;
static short dp_len;

static  int  x_index = 0;
#define  PAD  0xeeee

static void
iplot_ybars(int num)
{
    int i, k, len, index0, color, pnts;
    short px, py;
    short *ptrX, *ptrY;

    if (iplotFd == NULL || num <= 0)
        return;
    index0 = 0;
    pnts = num * 2;
    while (index0 < pnts) {
        px = ntohs(jybar.data[index0]);
        py = ntohs(jybar.data[index0+1]);
        len = (int) px;
        if (len < 0)
             break;
        index0++;
        py = ntohs(jybar.data[index0]);
        color = (int) py;
        index0++;
        if ((index0 + len * 2) > pnts)
             break;
        ptrX = (short *)&jybar.data[index0];
        ptrY = (short *)&jybar.data[index0+1];
        k = (int) (ntohl(jybar.subtype));

        if (len > 0)
            fprintf(iplotFd, "%s  %d 2 %d %d\n", iFUNC, k, len, color);
        i = 0;
        for (k = 0; k < len; k++) {
            px = ntohs(*ptrX);
            py = ntohs(*ptrY);
            fprintf(iplotFd, "%d %d ", px, py);
            ptrX += 2;
            ptrY += 2;
            i++;
            if (i >= 40) {
                fprintf(iplotFd, "\n");
                i = 0;
            }
        }
        if (i > 0)
            fprintf(iplotFd, "\n");
        if (len > 0) {
            end_iplot_func();
            index0 = index0 + len * 2;
        }
    }
}

static void
iplot_rast(int is_aip, void *src, int pnts, int times, int x, int y)
{
    int i, k;
    unsigned char  *r_data;

    if (iplotFd == NULL || pnts <= 0)
        return;
    if (jimage == NULL)
        return;

    k = (int) (ntohl(jimage->subtype));
    if (is_aip)
        fprintf(iplotFd, "%s %d ", AIPFUNC, k);
    else
        fprintf(iplotFd, "%s %d ", iFUNC, k);
    fprintf(iplotFd, " 4 %d %d %d %d\n", pnts, x, y, times);
    r_data = (unsigned char *) src;
    i = 0;
    for(k = 0; k < pnts; k++) {
        fprintf(iplotFd, "%d ", *r_data);
        r_data++; 
        i++;
        if (i >= 80) {
             fprintf(iplotFd, "\n");
             i = 0;
        }
    }
    if (i > 0)
         fprintf(iplotFd, "\n");
    end_iplot_func();
}

void new_x_adraw(int start_flag)
{
    if (useXFunc) {
        if (start_flag)
           x_index = 0;
        return;
    }
    if (start_flag) {
        jBatch(1);
        jybar.subtype = htonl(DPCON);
        jybar.len = 0;
        dp_max = JLEN - 20;
        dp_index = 0;
        adrawX = -1;
        adrawY = -1;
        dpconI = (uint16_t *)&jybar.data[0];
    }
    else {
        *dpconLen = htons(dp_len);
    }
    *dpconI = htons(0);
    dpconLen = dpconI;
    dpconI++;
    *dpconI = htons((uint16_t) g_color);
    dpconI++;
    dp_index++;
    dp_len = 0;
}

/* if draw 200 ybars, also need flush_draw */
void
flush_draw()
{
    int k;

    if (!useXFunc) {
        jBatch(1);
        if (dp_index > 4) {
            *dpconLen = htons(dp_len);
            *dpconI++ = PAD;
            *dpconI++ = PAD;
            dp_index++;
            jybar.len = htonl(dp_index);
            k = ybarHead + sizeof(short) * dp_index * 2;
            if (isSuspend == 0) {
               if (iplotFd != NULL)
                   iplot_ybars(dp_index - 1);
               else
                   graphToVnmrJ(&jybar, k);
            }
           // new_x_adraw(1);
            isGraphFunc = 1;
        }
        new_x_adraw(1);
        return;
    }
    if (x_index > 0)
    {
#ifdef MOTIF
        if (winDraw)
            XDrawSegments(xdisplay, xid, vnmr_gc, &x_segments[0], x_index);
	if (xmapDraw)
            XDrawSegments(xdisplay, canvasPixmap, pxm_gc, &m_segments[0], x_index);
#endif
        x_index = 0;
    }
}

void
x_adraw(int x, int y)
{
   uint16_t dp_y1, dp_y2;

   if (!useXFunc) {
	dp_y1 = mnumypnts-y;
        if (adrawX != xgrx || adrawY != xgry) {
            new_x_adraw(0);
	    dp_y2 = mnumypnts- xgry;
            *dpconI++ = htons(xgrx);
            *dpconI++ = htons(dp_y2);
            dp_len = 1;
            dp_index++;
        }
        *dpconI++ = htons((uint16_t) x);
        *dpconI++ = htons(dp_y1);
        dp_len++;
        dp_index++;
        if (dp_index >= dp_max) {
            flush_draw();
        }
        else {
	    adrawX = x;
	    adrawY = y;
        }
        xgrx = x;
        xgry = y;
	return;
    }
#ifdef MOTIF
    x_segments[x_index].x1 = xgrx + orgx;
    x_segments[x_index].x2 = x + orgx;
    x_segments[x_index].y1 = mnumypnts-xgry-1 + orgy;
    x_segments[x_index].y2 = mnumypnts-y-1 + orgy;
    if (xmapDraw) {
        m_segments[x_index].x1 = xgrx;
        m_segments[x_index].x2 = x;
        m_segments[x_index].y1 = mnumypnts-xgry-1;
        m_segments[x_index].y2 = mnumypnts-y-1;
    }
    x_index++;
    if (x_index >= 200)
    {
        if (winDraw)
            XDrawSegments(xdisplay, xid, vnmr_gc, &x_segments[0], x_index);
	if (xmapDraw)
            XDrawSegments(xdisplay, canvasPixmap, pxm_gc, &m_segments[0], x_index);
        x_index = 0;
    }
#endif
    xgrx = x; 
    xgry = y;
}

void sun_adraw(int x, int y)
{
   gx1 = xgrx + orgx;
   mx1 = xgrx;
   my1 = mnumypnts - xgry - 1;
   gy1 = my1 + orgy;
   if (useXFunc) {
       if (xgrx == x && xgry == y) {
#ifdef MOTIF
           if (winDraw)
              XDrawPoint(xdisplay, xid, vnmr_gc, gx1, gy1);
	   if (xmapDraw)
              XDrawPoint(xdisplay, canvasPixmap, pxm_gc, mx1, my1);
#endif
           return;
	}
   }
   xgrx = x; 
   xgry = y;
   mx2 = x;
   my2 = mnumypnts - y - 1;
   if (!useXFunc) {
        if (isPrintBg != 0 && xorflag != 0)
           addPrintLineBox(JLINE, mx1, my1, mx2, my2);
        else
   	   j4p_func(JLINE, mx1, my1, mx2, my2);
	return;
   }
   gx2 = x + orgx;
   gy2 = mnumypnts - y - 1 + orgy;
#ifdef MOTIF
   if (winDraw)
       XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy2);
   if (xmapDraw)
       XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx2, my2);
#endif
}

static void
aip_dstring(int is_aip, int type, int id, int x, int y, int c, const char *s)
{
    jText    *jt;
    char     *jdata;
    int	 len, jsize, pSize;

    if (s == NULL)
	return;
    len = strlen(s);
    if (len <= 0)
	return;
    if (iplotFd != NULL) {
        if (is_aip)
            fprintf(iplotFd, "%s ", AIPFUNC);
        else
            fprintf(iplotFd, "%s ", iFUNC);
        fprintf(iplotFd, "%d 4 %d %d %d %d\n", type, id, x, y, c);
        fprintf(iplotFd, "%s\n", s);
        // end_iplot_func();
	return;
    }
    if (in_batch == 0)
        jBatch(1);
    len = sizeof(char) * len;
    pSize = intLen * 7;
    jsize = pSize + len;
    if (jtext_len <= jsize)
    {
	if (jtext_data != NULL)
	   free(jtext_data);
        jtext_len = jsize + 8;
	jtext_data = (char *) malloc(jtext_len);
	if (jtext_data == NULL) {
           jtext_len = 0;
	   return;
        }
    }
    jt = (jText *) jtext_data;
    jdata = (char *)(jtext_data + pSize);
    if (is_aip)
	jt->type = htonl(AIPCODE + 6);
    else
	jt->type = htonl(6);
    jt->subtype = htonl(type);
    jt->id = htonl(id);
    jt->x = htonl(x);
    jt->y = htonl(y);
    jt->len = htonl(len);
    jt->color = htonl(c);
    bcopy(s, jdata, len);
    graphToVnmrJ(jtext_data, jsize);
    isGraphFunc = 1;
}

static void
vnmrj_dstring(int type, int id, int x, int y, int c, const char *s)
{
    aip_dstring(0, type, id, x, y, c, s);
}

void vnmrj_color_name(int index, char *s)
{
    vnmrj_dstring(JCOLORNAME, index, index, index, g_color, s);
}

static void
xj_dstring(int type, int id, int x, int y, int w, int h, int color, char *s) {
    jxText   *jt;
    char     *jdata;
    int  len, jsize, pSize;


    if (s == NULL)
        return;
    len = strlen(s);
    if (len <= 0)
        return;
    if (iplotFd != NULL) {
        fprintf(iplotFd, "%s ", iFUNC);
        fprintf(iplotFd, "%d 6 %d %d %d %d %d %d\n", type, id, x, y,w, h, color);
        fprintf(iplotFd, "%s\n", s);
        // end_iplot_func();
        return;
    }
    len = sizeof(char) * len;
    pSize = intLen * 9;
    jsize = pSize + len;
    if (jtext_len <= jsize)
    {
        if (jtext_data != NULL)
           free(jtext_data);
        jtext_len = jsize + 8;
        jtext_data = (char *) malloc(jtext_len);
        if (jtext_data == NULL) {
           jtext_len = 0;
           return;
        }
    }
    jt = (jxText *) jtext_data;
    jdata = (char *)(jtext_data + pSize);
    jt->type = htonl(7);
    jt->subtype = htonl(type);
    jt->id = htonl(id);
    jt->x = htonl(x);
    jt->y = htonl(y);
    jt->w = htonl(w);
    jt->h = htonl(h);
    jt->len = htonl(len);
    jt->color = htonl(color);
    bcopy(s, jdata, len);
    graphToVnmrJ(jtext_data, jsize);
    isGraphFunc = 1;
}


int jbanner(char *s)
{
   if (useXFunc)
       return(0);
   vnmrj_dstring(JBANNER,0, 0, 0,g_color, s);
   return(1);
}

void sun_dchar(char ch)
{
  static  char str[4];

   my1 = mnumypnts-xgry;
   if (!useXFunc) {
	str[0] = ch;
  	str[1] = '\0';
	vnmrj_dstring(JTEXT, 0, xgrx, my1, g_color, str);
   }
   else {
        my1 = my1 - char_descent;
        gx1 = xgrx + orgx;
        gy1 = my1 + orgy;
#ifdef MOTIF
	if (winDraw)
	     XDrawString(xdisplay, xid, vnmr_gc, gx1, gy1, &ch, 1);
        if (xmapDraw)
	     XDrawString(xdisplay,canvasPixmap, pxm_gc, xgrx, my1, &ch, 1);
#endif
   }
   xgrx += xcharpixels;
}

static int ticX = -1;
static int ticY = -1;

void set_tic_location(int x, int y)
{
    ticX = x;
    ticY = y;
}

void sun_dstring(char *s)
{
    int	 len;

    len = strlen(s);
    if (len <= 0)
	return;
    if (!useXFunc) {
        if (ticX >= 0) {
            my1 = mnumypnts - ticY;
	    vnmrj_dstring(TICTEXT, 0, ticX, my1, g_color, s);
	    return;
        }
        my1 = mnumypnts-xgry;
	vnmrj_dstring(JTEXT, 0, xgrx, my1, g_color, s);
        xgrx += xcharpixels * len;
	return;
    }
    my1 = mnumypnts - xgry - char_descent;
    gx1 = xgrx + orgx;
    gy1 = my1+orgy;
#ifdef MOTIF
    if (winDraw)
	 XDrawString(xdisplay, xid, vnmr_gc, gx1, gy1, s, len);
    if (xmapDraw)
	 XDrawString(xdisplay, canvasPixmap, pxm_gc, xgrx, my1, s, len);
#endif
    xgrx += xcharpixels * len;

/* don't call XFlush. it may hang up the system */
/*
    XFlush(xdisplay);
*/
}

void sun_dhstring(char *s, int x, int y)
{
   if (!useXFunc) {
        my1 = mnumypnts - y;
	vnmrj_dstring(JHTEXT, 0, x, my1, g_color, s);
        return;
   }
   sun_dstring(s);
}

void vj_dstring(char *s, int x, int y)
{
   y = mnumypnts - y;
   if (!useXFunc) {
        my1 = mnumypnts - y;
	vnmrj_dstring(NOCOLOR_TEXT, 0, x, y, 0, s);
        return;
   }
   sun_dstring(s);
}

void sun_dvstring(char *str)
{
#ifdef MOTIF
  register int i,j,index;
  int	  k, len;
  unsigned long  pixel;
  XImage  *tmp_image;
  static  int  pix_h = 0;
  static  int  pix_w = 0;
#endif
  int	  w;

   w = get_text_width(str);
   if (!useXFunc) {
        gy1 = mnumypnts-xgry;
        vnmrj_dstring(JVTEXT, 0, xgrx, gy1, g_color, str);
        xgry = xgry + w + 1;
        return;
   }
#ifdef MOTIF
   if (!one_plane)
       return;
   len = strlen(str);
   if (pix_w < w || pix_h != ycharpixels)
   {
       if (charmap != 0)
           XFreePixmap(xdisplay, charmap);
       pix_h = ycharpixels;
       pix_w = w;
       charmap = XCreatePixmap(xdisplay, xid, w, pix_h, 1);
       if (charmap == 0) {
           pix_w = 0;
           return;
       }
       if (vchar_gc == NULL)
           vchar_gc = XCreateGC(xdisplay, charmap, 0, 0);
       if (x_font)
           XSetFont(xdisplay, vchar_gc, x_font);
  }
  if (charmap == 0)
	return;
  XSetForeground(xdisplay, vchar_gc, 0);
  XFillRectangle(xdisplay, charmap, vchar_gc, 0, 0, w, ycharpixels);
  XSetForeground(xdisplay, vchar_gc, 1);
  XDrawString(xdisplay, charmap, vchar_gc, 0, char_ascent, str, len);
  tmp_image = XGetImage(xdisplay, charmap, 0, 0, w, ycharpixels,
                                1, XYPixmap);
  index = 0;
  k = 0;
  for(i = 0; i < w; i++)
  {
      for(j=0; j < ycharpixels; j++)
      {
          pixel = XGetPixel(tmp_image, i, j);
          if (pixel)
          {
               mpoints[index].x = xgrx+j + k;
               mpoints[index].y = mnumypnts-xgry-1-i;
               points[index].x = mpoints[index].x + orgx;
               points[index].y = mpoints[index].y + orgy;
               index++;
               if (index >= YLEN) 
               {
    		   if (winDraw)
                       XDrawPoints(xdisplay, xid,vnmr_gc, &points[0], index,
                                         CoordModeOrigin);
                   if (xmapDraw)
                       XDrawPoints(xdisplay, canvasPixmap, pxm_gc,
                                         &mpoints[0], index, CoordModeOrigin);
                   index = 0;
               }
          }
      }
   }
   XDestroyImage(tmp_image);
   if (index > 0)
   {
    	if (winDraw)
           XDrawPoints(xdisplay, xid, vnmr_gc, &points[0], index, CoordModeOrigin);
        if (xmapDraw)
           XDrawPoints(xdisplay, canvasPixmap, pxm_gc, &mpoints[0], index, CoordModeOrigin);
   }
#endif
  xgry = xgry + w + 1;
}

void
sun_dvchar(char ch)
{ 
  static  char str[4];

/*
  if ((ch < 1) || (ch > 250))
  {   xgry += xcharpixels;
      return 0;
  }
*/
  str[0] = ch;
  str[1] = '\0';
  if (!useXFunc) {
      gy1 = mnumypnts-xgry;
      vnmrj_dstring(JVTEXT, 0, xgrx, gy1, g_color, str);
      xgry += xcharpixels;
      return;
  }
  sun_dvstring(str);
}


void
sun_vstring(char *str)
{
#ifdef MOTIF
   int	  w, k;
#endif
   int	  len;

   len = strlen(str);
   if (len < 1)
        return;
   if (!useXFunc) {
	gy1 = mnumypnts-xgry;
	vnmrj_dstring(JVTEXT, 0, xgrx, gy1, g_color, str);
        xgry = xgry - ycharpixels * len;
	return;
   }
#ifdef MOTIF
   for (k = 0; k < len; k++) {
       if (xstruct != NULL) {
           w = XTextWidth(xstruct, str, 1);
           gx1 = xgrx + orgx + (xcharpixels - w) / 2;
       }
       else
           gx1 = xgrx + orgx;
       if (winDraw) {
            gy1 = mnumypnts-xgry-1+orgy;
	    XDrawString(xdisplay, xid, vnmr_gc, gx1, gy1, str, 1);
       }
       if (xmapDraw) {
	    my1 = mnumypnts-xgry-1;
	    XDrawString(xdisplay, canvasPixmap, pxm_gc, xgrx, my1, str, 1);
       }
       str++;
       xgry -= ycharpixels;
   }
#endif
}

int sun_vchar(char ch)
{ 
  static  char str[4];

  str[0] = ch;
  str[1] = '\0';
  if (!useXFunc) {
      gy1 = mnumypnts-xgry;
      vnmrj_dstring(JVTEXT, 0, xgrx, gy1, g_color, str);
      xgry -= ycharpixels;
      return 1;
  }
  sun_vstring(str);
  return 0;
}

void
sun_dimage(int x, int y, int w, int h) 
{
}

void
vj_alpha_mode(int n)
{
     j1p_func(JALPHA, n);
}

void
vj_reexec_cmd(int n)
{
     j1p_func(REEXEC, n);
}

static void
vj_dconi_cursor(int which, int id, int pos, int len)
{
    j4p_func(ICURSOR, which, id, pos, len);
}

static void
draw_newCursor()
{
    int  i, old_color, old_mode;

    if (!winDraw || useXFunc < 1)
        return;
    old_color = g_color;
    old_mode = xorflag;
    if (xorflag)
        normalmode();
    color(CURSOR_COLOR);
    for(i = 0; i < 3; i++)
    {
	gx1 = orgx + x_cursor_pos[i];
	gy1 = orgy + y_cursor_pos[i];
	if (useXFunc) {
#ifdef MOTIF
	    if (x_cursor_pos[i] > 0) {
		XDrawLine(xdisplay, xid, vnmr_gc, gx1, orgy + 1,
                        gx1, orgy+x_cursor_y2[i]);
	    }
            if (y_cursor_pos[i] > 0) {
                XDrawLine(xdisplay, xid, vnmr_gc, orgx + 1, gy1,
                        orgx + y_cursor_x2[i], gy1);
	    }
#endif
	}
/*
	else {
	    if (x_cursor_pos[i] > 0)
	       draw_jline(gx1, 1, gx1, x_cursor_y2[i]);
            if (y_cursor_pos[i] > 0)
	       draw_jline(1, gy1, y_cursor_x2[i], gy1);
	}
*/
   }
/*
   if (!useXFunc) {
       jvnmr_cmd(JDCURSOR);
   }
*/
   if(old_mode)
      xormode();
   color(old_color);
}

// j_v_ybars and j_h_ybars will draw ybar in paint (non-xor) mode

static void
j_v_ybars(int dfpnt, int depnt, struct ybar *out)
{
    int    i, jindex, last, dsize, skip;
    short  p0, p1, p2, p3;
    short  pn, px, cc;
    short  opn, opx;
    short  *sizeX, *sizeY;
    unsigned short  *ptrX, *ptrY;

    jybar.subtype = htonl(XYBAR);
    jybar.len = 0;
    last = JLEN - 12;

    sizeX = &jybar.data[0];
    sizeY = &jybar.data[1];
    ptrX = (unsigned short *)&jybar.data[2];
    ptrY = (unsigned short *)&jybar.data[3];
    jindex = 1;
    cc = 0;
    jBatch(1);
    isGraphFunc = 1;

    p0 = -1;
    opx = -1;
    opn = -1;
    skip = 0;
    for (i=dfpnt; i<depnt; i++)
    {
        px = mnumxpnts - out[i].mn;
        pn = mnumxpnts - out[i].mx;
        if (px < pn) {
           if (skip > 0 && p0 >= 0) { // add previous points
                *ptrX = htons(p0);
                ptrX += 2;
                *ptrY = htons(mnumypnts - i + 1);
                ptrY += 2;
                jindex++;
           }
           if (jindex > 1) {
                cc = (short) jindex - 1;
                jindex++;
                jybar.len = htonl(jindex);
                dsize = ybarHead + sizeof(short) * jindex * 2;
                *sizeX = htons(cc);
                *sizeY = htons(mnumypnts);
                *ptrX = PAD;
                ptrX++;
                *ptrX = PAD;
                if (iplotFd != NULL)
                    iplot_ybars(jindex);
                else
                    graphToVnmrJ(&jybar, dsize);
            }
            i++;
            while (i < depnt) {
                if (out[i].mx >= out[i].mn) {
                    i--;
                    break;
                }
                i++;
            }
            jindex = 1;
            ptrX = (unsigned short *)&jybar.data[2];
            ptrY = (unsigned short *)&jybar.data[3];
            p0 = -1;
            opx = -1;
            skip = 0;
            continue;
        }
        /* data is in mnumxpnts space */
        if (px == pn) {
            if (p0 == px) {
                skip++;
                continue;
            }
            if (skip > 0) {
                if (p0 >= 0) {
                    *ptrX = htons(p0);
                    ptrX += 2;
                    *ptrY = htons(mnumypnts - i + 1);
                    ptrY += 2;
                    jindex++;
                }
                skip = 0;
            }
            if (opx >= 0) {
                if (opx <= px) {
                    p1 = opx;
                }
                else {
                    if (opn >= px)
                        p1 = opn;
                    else
                        p1 = px;
                }
                if (p0 != p1) {
                    *ptrX = htons(p1);
                    ptrX += 2;
                    *ptrY = htons(mnumypnts - i + 1);
                    ptrY += 2;
                    jindex++;
                }
            }
            *ptrX = htons(px);
            ptrX += 2;
            *ptrY = htons(mnumypnts - i);
            ptrY += 2;
            jindex++;
            p0 = px;
            opx = -1;
        }
        else {
            if (skip > 0) {
                if (p0 >= 0) {
                   *ptrX = htons(p0);
                   ptrX += 2;
                   *ptrY = htons(mnumypnts - i + 1);
                   ptrY += 2;
                   jindex++;
                }
                skip = 0;
                opx = -1;
            }
            if (opx >= 0) { // the previous was a segment
                p1 = -1;
                p2 = -1;
                if (pn >= opx) {
                   p1 = opx;
                   p2 = pn;
                   p3 = px;
                }
                else if (px <= opn) {
                   p1 = opn;
                   p2 = px;
                   p3 = pn;
                }
                if (p1 < 0) {
                   if (px <= opx && px >= opn) {
                       p1 = px;
                       p2 = px;
                       p3 = pn;
                   }
                   else {
                       if (pn <= opx && pn >= opn) {
                           p1 = pn;
                           p2 = pn;
                           p3 = px;
                       }
                       else {
                           p1 = p0;
                           p3 = px;
                       }
                   }
                }
                if (p0 != p1) {
                   *ptrX = htons(p1);
                   ptrX += 2;
                   *ptrY = htons(mnumypnts - i + 1);
                   ptrY += 2;
                   jindex++;
                }
                if (p2 < 0) {
                   *ptrX = htons(p0);
                   ptrX += 2;
                   *ptrY = htons(mnumypnts - i);
                   ptrY += 2;
                   jindex++;
                   p2 = pn;
                }
                p1 = p2;
                p0 = p3;
            }
            else { 
                if (p0 < px) {
                    if (p0 > pn) {
                       *ptrX = htons(p0);
                       ptrX += 2;
                       *ptrY = htons(mnumypnts - i);
                       ptrY += 2;
                       jindex++;
                    }
                    p1 = pn;
                    p0 = px;
                }
                else {
                    p1 = px;
                    p0 = pn;
                }
            }

            *ptrX = htons(p1);
            ptrX += 2;
            *ptrY = htons(mnumypnts - i);
            ptrY += 2;

            *ptrX = htons(p0);
            ptrX += 2;
            *ptrY = htons(mnumypnts - i);
            ptrY += 2;
            jindex += 2;
            opx = px;
            opn = pn;
        } // end of px != pn
        if (jindex >= last) {
            cc = (short) jindex - 1;
            jindex++;
            jybar.len = htonl(jindex);
            dsize = ybarHead + sizeof(short) * jindex * 2;
            *sizeX = htons(cc);
            *sizeY = htons(mnumypnts);
            *ptrX = PAD;
            ptrX++;
            *ptrX = PAD;
            if (iplotFd != NULL)
                iplot_ybars(jindex);
            else
                graphToVnmrJ(&jybar, dsize);

            ptrX = (unsigned short *)&jybar.data[2];
            ptrY = (unsigned short *)&jybar.data[3];
            jindex = 1;
            skip = 0;
            p0 = -1;
            opx = -1;
            if (isPrintBg != 0) {
                if (dsize > 600) {
                    transCount++;
                    if (transCount > 5) {
                        transCount = 0;
                        usleep(10000);
                    }
                }
            }
        }
    } // end of for loop

    if (skip > 0 && p0 >= 0) {
        *ptrX = htons(p0);
        ptrX += 2;
        *ptrY = htons(mnumypnts - depnt + 1);
        ptrY += 2;
        jindex++;
    }

    if (jindex < 2)
        return;
    cc = (short) jindex - 1;
    jindex++;
    jybar.len = htonl(jindex);
    dsize = ybarHead + sizeof(short) * jindex * 2;
    *sizeX = htons(cc);
    *sizeY = htons(mnumypnts);
    *ptrX = PAD;
    ptrX++;
    *ptrX = PAD;
    if (iplotFd != NULL)
        iplot_ybars(jindex);
    else
        graphToVnmrJ(&jybar, dsize);
    if (isPrintBg != 0) {
        if (dsize > 600) {
            transCount++;
            if (transCount > 5) {
                transCount = 0;
                usleep(10000);
            }
        }
    }
}

static void
j_h_ybars(int dfpnt, int depnt, struct ybar *out)
{
    int    i, jindex, last, dsize, skip;
    short  p0, p1, p2, p3;
    short  pn, px, cc;
    short  opn, opx;
    short  *sizeX, *sizeY;
    unsigned short  *ptrX, *ptrY;

    jybar.subtype = htonl(XYBAR);
    jybar.len = 0;
    last = JLEN - 12;

    sizeX = &jybar.data[0];
    sizeY = &jybar.data[1];
    ptrX = (unsigned short *)&jybar.data[2];
    ptrY = (unsigned short *)&jybar.data[3];
    jindex = 1;
    cc = 0;
    jBatch(1);
    isGraphFunc = 1;

    p0 = -1;
    opx = -1;
    opn = -1;
    skip = 0;
    for (i=dfpnt; i<depnt; i++)
    {
        pn = out[i].mn;
        px = out[i].mx;
        if (px < pn) {
           if (skip > 0 && p0 >= 0) { // add previous points
               *ptrX = htons(i-1);
                ptrX += 2;
                *ptrY = htons(mnumypnts - p0);
                ptrY += 2;
                jindex++;
           }
           if (jindex > 1) {
                cc = (short) jindex - 1;
                jindex++;
                jybar.len = htonl(jindex);
                dsize = ybarHead + sizeof(short) * jindex * 2;
                *sizeX = htons(cc);
                *sizeY = htons(mnumypnts);
                *ptrX = PAD;
                ptrX++;
                *ptrX = PAD;
                if (iplotFd != NULL)
                    iplot_ybars(jindex);
                else
                    graphToVnmrJ(&jybar, dsize);
            }
            i++;
            while (i < depnt) {
                if (out[i].mx >= out[i].mn) {
                    i--;
                    break;
                }
                i++;
            }
            jindex = 1;
            ptrX = (unsigned short *)&jybar.data[2];
            ptrY = (unsigned short *)&jybar.data[3];
            p0 = -1;
            opx = -1;
            skip = 0;
            continue;
        }
       /* data is in mnumypnts space */
        if (px == pn) {
            if (p0 == px) {
                skip++;
                continue;
            }
            if (skip > 0) {
                if (p0 >= 0) {
                   *ptrX = htons(i-1);
                   ptrX += 2;
                   *ptrY = htons(mnumypnts - p0);
                   ptrY += 2;
                   jindex++;
                }
                skip = 0;
            }
            if (opx >= 0) {
                if (opx <= px) {
                    p1 = opx;
                }
                else {
                    if (opn >= px)
                        p1 = opn;
                    else
                        p1 = px;
                }
                if (p0 != p1) {
                    *ptrX = htons(i-1);
                    ptrX += 2;
                    *ptrY = htons(mnumypnts - p1);
                    ptrY += 2;
                    jindex++;
                }
            }
            *ptrX = htons(i);
            ptrX += 2;
            *ptrY = htons(mnumypnts - px);
            ptrY += 2;
            jindex++;
            p0 = px;
            opx = -1;
        }
        else {
            if (skip > 0) {
                if (p0 >= 0) {
                   *ptrX = htons(i-1);
                   ptrX += 2;
                   *ptrY = htons(mnumypnts - p0);
                   ptrY += 2;
                   jindex++;
                }
                skip = 0;
                opx = -1;
            }
            if (opx >= 0) { // the previous was a segment
                p1 = -1;
                p2 = -1;
                if (pn >= opx) {
                   p1 = opx;
                   p2 = pn;
                   p3 = px;
                }
                else if (px <= opn) {
                   p1 = opn;
                   p2 = px;
                   p3 = pn;
                }
                if (p1 < 0) {
                   if (px <= opx && px >= opn) {
                       p1 = px;
                       p2 = px;
                       p3 = pn;
                   }
                   else {
                       if (pn <= opx && pn >= opn) {
                           p1 = pn;
                           p2 = pn;
                           p3 = px;
                       }
                       else {
                           p1 = p0;
                           p3 = px;
                       }
                   }
                }
                if (p0 != p1) {
                   *ptrX = htons(i-1);
                   ptrX += 2;
                   *ptrY = htons(mnumypnts - p1);
                   ptrY += 2;
                   jindex++;
                }
                if (p2 < 0) {
                   *ptrX = htons(i);
                   ptrX += 2;
                   *ptrY = htons(mnumypnts - p0);
                   ptrY += 2;
                   jindex++;
                   p2 = pn;
                }
                p1 = p2;
                p0 = p3;
            }
            else {
                if (p0 < px) {
                    if (p0 > pn) {
                       *ptrX = htons(i);
                       ptrX += 2;
                       *ptrY = htons(mnumypnts - p0);
                       ptrY += 2;
                       jindex++;
                    }
                    p1 = pn;
                    p0 = px;
                }
                else {
                    p1 = px;
                    p0 = pn;
                }
            }

            *ptrX = htons(i);
            ptrX += 2;
            *ptrY = htons(mnumypnts - p1);
            ptrY += 2;

            *ptrX = htons(i);
            ptrX += 2;
            *ptrY = htons(mnumypnts - p0);
            ptrY += 2;
            jindex += 2;

            opx = px;
            opn = pn;
        } // end of px != pn

        if (jindex >= last) {
            cc = (short)jindex - 1;
            jindex++;
            jybar.len = htonl(jindex);
            dsize = ybarHead + sizeof(short) * jindex * 2;
            *sizeX = htons(cc);
            *sizeY = htons(mnumypnts);
            *ptrX = PAD;
            ptrX++;
            *ptrX = PAD;
            if (iplotFd != NULL)
                iplot_ybars(jindex);
            else
                graphToVnmrJ(&jybar, dsize);

            ptrX = (unsigned short *)&jybar.data[2];
            ptrY = (unsigned short *)&jybar.data[3];
            jindex = 1;
            skip = 0;
            p0 = -1;
            opx = -1;
            if (isPrintBg != 0) {
                if (dsize > 600) {
                    transCount++;
                    if (transCount > 5) {
                        transCount = 0;
                        usleep(10000);
                    }
                }
            }
        }
    }  /*  for i loop */
    if (skip > 0 && p0 >= 0) {
        *ptrX = htons(depnt-1);
        ptrX += 2;
        *ptrY = htons(mnumypnts - p0);
        ptrY += 2;
        jindex++;
    }
    if (jindex < 2)
        return;

    cc = (short)jindex - 1;
    jindex++;
    jybar.len = htonl(jindex);
    dsize = ybarHead + sizeof(short) * jindex * 2;
    *sizeX = htons(cc);
    *sizeY = htons(mnumypnts);
    *ptrX = PAD;
    ptrX++;
    *ptrX = PAD;
    if (iplotFd != NULL)
        iplot_ybars(jindex);
    else
        graphToVnmrJ(&jybar, dsize);
    if (isPrintBg != 0) {
        if (dsize > 600) {
            transCount++;
            if (transCount > 5) {
                transCount = 0;
                usleep(10000);
            }
        }
    }
}

static void
j_ybars(int dfpnt, int depnt, struct ybar *out, int vertical, int maxv, int minv)
{
  int    i, jindex, last, dsize;
  unsigned short  *dxp, *dyp;
  short  *indx, *indy;
  short   p1, mvDir;
  short   pn, px, dp, cc, mxv;

  if (isSuspend || out == NULL)
      return;

  if (xorflag && noXorYbar > 0) {
      if (vertical)
          j_v_ybars(dfpnt, depnt, out);
      else
          j_h_ybars(dfpnt, depnt, out);
      return;
  }

  jybar.subtype = htonl(JYBAR);
  jybar.len = 0;
  last = JLEN - 12;

  indx = &jybar.data[0];
  indy = &jybar.data[1];
  dxp = (unsigned short *)&jybar.data[2];
  dyp = (unsigned short *)&jybar.data[3];
  jindex = 1;
  cc = 0;
  jBatch(1);

  isGraphFunc = 1;
  if (vertical)
  {
     p1 = out[dfpnt].mn;
     mxv = (short)mnumxpnts;
     if (mxv <= depnt)
        mxv = depnt + 1;
     for (i=dfpnt; i<depnt; i++)
     {
        px = mnumxpnts - out[i].mn;
        pn = mnumxpnts - out[i].mx;
        if (px < pn) {
            if (jindex > 1) {
               jindex++;
               jybar.len = htonl(jindex);
               dsize = ybarHead + sizeof(short) * jindex * 2;
               *indx = htons(cc);
               *indy = htons(mxv);
               *dxp = PAD;
               dxp++;
               *dxp = PAD;
               if (iplotFd != NULL)
                   iplot_ybars(jindex);
               else
                   graphToVnmrJ(&jybar, dsize);
            }
            i++;
            jindex = 1;
            cc = 0;
            while (i < depnt) {
                px = mnumxpnts - out[i].mn;
                pn = mnumxpnts - out[i].mx;
                if (px >= pn) {
                    indx = &jybar.data[0];
                    indy = &jybar.data[1];
                    dxp = (unsigned short *)&jybar.data[2];
                    dyp = (unsigned short *)&jybar.data[3];
                    p1 = out[i].mn;
                    i--;
                    break;
                }
                i++;
            }
            continue;
        }
        /* is in mnumypnts space */
        dp = 0;
        mvDir = 1;
        if (p1 > pn) {
            mvDir = 2;
            if (p1 > px)
                dp = p1 - px;
            else {
                dp = p1 - pn;
                if (dp > 1) {
                   dp = px - p1;
                }
                else
                   mvDir = 1;
            }
        }
        else
            dp = pn - p1;
        if (dp > 1) {
                   *indx = htons(cc);
                   *indy = htons(mxv);
                   jindex++;
                   indx = (short *)dxp;
                   indy = (short *)dyp;
                   cc = 0;
                   dxp += 2;
                   dyp += 2;
        }
        *dyp = htons(mnumypnts - i);
        dyp += 2;
        if (pn != px) {
            if (mvDir == 1) {   /* rightward */
                     *dxp = htons(pn);
                     dxp += 2;
                     *dxp = htons(px);
                     dxp += 2;
                     *dyp = htons(mnumypnts - i);
                     dyp += 2;
                     p1 = px;
            }
            else {
                     *dxp = htons(px);
                     dxp += 2;
                     *dxp = htons(pn);
                     dxp += 2;
                     *dyp = htons(mnumypnts-i);
                     dyp += 2;
                     p1 = pn;
            }
            jindex += 2;
            cc += 2;
        }
        else
        {
            *dxp = htons(px);
            dxp += 2;
            jindex++;
	    cc++;
            p1 = px;
        }
        if (jindex >= last) {
            jindex++;
            jybar.len = htonl(jindex);
            dsize = ybarHead + sizeof(short) * jindex * 2;
            *indx = htons(cc);
            *indy = htons(mxv);
            *dxp = PAD;
            dxp++;
            *dxp = PAD;
            if (iplotFd != NULL)
                iplot_ybars(jindex);
            else
                graphToVnmrJ(&jybar, dsize);
            indx = &jybar.data[0];
            indy = &jybar.data[1];
            dxp = (unsigned short *)&jybar.data[2];
            dyp = (unsigned short *)&jybar.data[3];
            jindex = 1;
            cc = 0;
         }
      }  /*  for i loop  */

  }  /* end of vertical */
  else /* horizontal */
  {
     p1 = out[dfpnt].mn;
     mxv = mnumypnts;
     if (mxv <= depnt)
        mxv = depnt + 1; 
     for (i=dfpnt; i<depnt; i++)
     {
        pn = out[i].mn;
        px = out[i].mx;
        if (px < pn) {
           if (jindex > 1) {
                jindex++;
                jybar.len = htonl(jindex);
                dsize = ybarHead + sizeof(short) * jindex * 2;
                *indx = htons(cc);
                *indy = htons(mxv);
                *dxp = PAD;
                dxp++;
                *dxp = PAD;
                if (iplotFd != NULL)
                    iplot_ybars(jindex);
                else
                    graphToVnmrJ(&jybar, dsize);
            }
            i++;
            jindex = 1;
            cc = 0;
            while (i < depnt) {
                if (out[i].mx >= out[i].mn) {
                    indx = &jybar.data[0];
                    indy = &jybar.data[1];
                    dxp = (unsigned short *)&jybar.data[2];
                    dyp = (unsigned short *)&jybar.data[3];
                    p1 = out[i].mn;
                    i--;
                    break;
                }
                i++;
            }
            continue;
        }
        /* is in mnumypnts space */
        dp = 0;
        mvDir = 1; /* upward */
        if (p1 > pn) {
            mvDir = 2; /* downward */
            if (p1 > px)
                dp = p1 - px;
            else {
                dp = p1 - pn;
                if (dp > 1)
                   dp = px - p1;
                else
                   mvDir = 1;
            }
        }
        else
            dp = pn - p1;
        if (dp > 1) {
            *indx = htons(cc);
            *indy = htons(mxv);
            jindex++;
            indx = (short *)dxp;
            indy = (short *)dyp;
            cc = 0;
            dxp += 2;
            dyp += 2;
        }
        *dxp = htons(i);
        dxp += 2;
        if (px != pn) {
            if (mvDir == 1) {
                  *dyp = htons(mnumypnts - pn);
                  dyp += 2;
                  *dxp = htons(i);
                  *dyp = htons(mnumypnts - px);
                  dxp += 2;
                  dyp += 2;
                  p1 = px;
            }
            else {
                  *dyp = htons(mnumypnts - px);
                  dyp += 2;
                  *dxp = htons(i);
                  *dyp = htons(mnumypnts - pn);
                  dxp += 2;
                  dyp += 2;
                  p1 = pn;
            }
            jindex += 2;
            cc += 2;
        }
        else {
            *dyp = htons(mnumypnts - px);
            dyp += 2;
            jindex++;
            cc++;
            p1 = px;
        }
        if (jindex >= last) {
                jindex++;
                jybar.len = htonl(jindex);
                dsize = ybarHead + sizeof(short) * jindex * 2;
                *indx = htons(cc);
                *indy = htons(mxv);
                *dxp = PAD;
                dxp++;
                *dxp = PAD;
                if (iplotFd != NULL)
                    iplot_ybars(jindex);
                else
                    graphToVnmrJ(&jybar, dsize);

                indx = &jybar.data[0];
                indy = &jybar.data[1];
                dxp = (unsigned short *)&jybar.data[2];
                dyp = (unsigned short *)&jybar.data[3];
                jindex = 1;
                cc = 0;
                if (isPrintBg != 0) {
                    if (dsize > 600) {
                        transCount++;
                        if (transCount > 5) {
                            transCount = 0;
                            usleep(10000);
                        }
                    }
                }
        }
     }  /*  for i loop */
  }
  if (jindex > 1) {
     jindex++;
     jybar.len = htonl(jindex);
     dsize = ybarHead + sizeof(short) * jindex * 2;
     *indx = htons(cc);
     *indy = htons(mxv);
     *dxp = PAD;
     dxp++;
     *dxp = PAD;
     if (iplotFd != NULL)
         iplot_ybars(jindex);
     else
         graphToVnmrJ(&jybar, dsize);
     if (isPrintBg != 0) {
         if (dsize > 600) {
             transCount++;
             if (transCount > 5) {
                 transCount = 0;
                 usleep(10000);
             }
         }
     }
  }
  // draw_newCursor();
}

static void
paint_printData()
{
    PRTDATA *prtData;
    PRTLINEBOX  *pline;
    PRTYBAR     *pbar;

    normalmode();
    color(g_color);
    prtData = printData;
    while (prtData != NULL) {
        if (g_color != prtData->color)
            color(prtData->color);
        if (prtData->type == JLINE || prtData->type == JBOX) {
            pline = (PRTLINEBOX *) &prtData->data.lineBox;
            j4p_func(prtData->type, pline->x1, pline->y1, pline->x2, pline->y2);
        }
        if (prtData->type == JYBAR) {
            pbar = (PRTYBAR *) &prtData->data.specbar;
            mnumypnts = pbar->ybarYpnts;
            mnumxpnts = pbar->ybarXpnts;
            aip_mnumypnts = mnumypnts;
            aip_mnumxpnts = mnumxpnts;
            j_ybars(pbar->start, pbar->end, pbar->points, pbar->vertical,
                                  pbar->maxv,pbar->minv);
        }
        prtData = prtData->next;
    }    
}

//  paint_ybars support print_screen functionality
static void
paint_ybars()
{
#ifdef OLD
    int oldXorFlag, curColor;

    if (ybarSize < 2)
        return;
    if (ybarData == NULL)
        return;

    if (xdebug > 0)
        fprintf(stderr, "  paint ybars, size %d \n", ybarSize);
    oldXorFlag = xorflag;
    curColor = g_color;
    if (xorflag)
        normalmode();
    color(ybarColor);
    j_ybars(ybarStart,ybarEnd,ybarData,verticalYbar,ybarMax,ybarMin);
    if (oldXorFlag)
        xormode();
    color(curColor);
    ybarSize = 0;
#endif
}

static void
check_paint_ybars(int dfpnt, int depnt, struct ybar *out, int vertical,
                  int maxv, int minv)
{
    int k, len, n;
    int isEqual;
    PRTDATA *prtData;
    PRTDATA *newData;
    PRTYBAR *pbar;
    struct ybar *pdata;

    len = depnt - dfpnt;
    if (len < 4)
        return;
    prtData = printData;
    pdata = NULL;
    isEqual = 0;
    while (prtData != NULL) {
        pbar = (PRTYBAR *) &prtData->data.specbar;
        if (prtData->type == JYBAR && pbar->start == dfpnt &&
                                          pbar->end == depnt) {
            if (prtData->color == g_color && pbar->vertical == vertical) {
               pdata = pbar->points;
               if (len > 30)
                  len = 30;
               isEqual = 1;
               n = dfpnt + len;
               for (k = dfpnt; k < n; k++) {
                  if (pdata[k].mx != out[k].mx) {
                      isEqual = 0;
                      break;
                  }
                  if (pdata[k].mn != out[k].mn) {
                      isEqual = 0;
                      break;
                  }
               }
            }
        }
        if (isEqual)
            break;
        prtData = prtData->next;
    }
    if (isEqual) {
        prtData->type = 0;
        if (pdata != NULL)
            free(pdata);
        return;
    }
    newData = (PRTDATA *) malloc(sizeof(PRTDATA));
    if (newData == NULL)
        return;
    pbar = (PRTYBAR *) &newData->data.specbar;
    pbar->points = (struct ybar *) malloc(sizeof(struct ybar) * (depnt+1));
    pdata = pbar->points;
    if (pdata == NULL)  {
        free(newData);
        return;
    }
    newData->type = JYBAR;
    newData->color = g_color;
    newData->next = NULL;
    pbar->start = dfpnt;
    pbar->end = depnt;
    pbar->vertical = vertical;
    pbar->ybarYpnts = mnumypnts;
    pbar->ybarXpnts = mnumxpnts;
    pbar->maxv = maxv;
    pbar->minv = minv;

    for (k = dfpnt; k < depnt; k++) {
        pdata[k].mx = out[k].mx;
        pdata[k].mn = out[k].mn;
    }
    pdata[k].mx = out[k-1].mx;
    pdata[k].mn = out[k-1].mn;
    if (printData == NULL) {
        printData = newData;
        return;
    } 
    prtData = printData;
    while (prtData->next != NULL) prtData = prtData->next;
    prtData->next = newData;
    return;
}

void
aip_ybars(int dfpnt, int depnt,
          struct ybar *out, int vertical,
          int maxv, int minv)
{
        int oldW, oldH;

        oldW = mnumxpnts;
        oldH = mnumypnts;
        if (csi_opened) {
           oldW = mnumxpnts;
           oldH = mnumypnts;
           mnumxpnts = csi_win_width - 4;
           mnumypnts = csi_win_height - 4;
           jybar.type = htonl(AIPCODE + 5);
        }
        j_ybars(dfpnt,depnt,out,vertical,maxv,minv);
        if (csi_opened) {
           mnumxpnts = oldW;
           mnumypnts = oldH;
           jybar.type = htonl(5);
        }
}


/*******************************************/
void
sun_ybars(register int dfpnt, register int depnt,
          register struct ybar *out, int vertical,
          int maxv, int minv)  /* draws a spectrum */
/*******************************************/
{
#ifdef MOTIF
  register int i;
#endif
  int index1, index2;

    /* first check vertical limits */
    if (maxv)
        range_ybar(dfpnt,depnt,out,maxv,minv);
    if (!useXFunc) {
        if (isPrintBg != 0 && xorflag != 0) {
            check_paint_ybars(dfpnt,depnt,out,vertical,maxv,minv);
            return; 
        }
        j_ybars(dfpnt,depnt,out,vertical,maxv,minv);
        return;
    }
    index1 = index2 = 0;
#ifdef MOTIF
    if (vertical)
    {  for (i=dfpnt; i<depnt; i++)
       {   if (out[i].mx>=out[i].mn) /* is in mnumypnts space */
           {
              if (out[i].mx != out[i].mn)
              {
                  msegments[index2].x1 = mnumxpnts-out[i].mx;
                  msegments[index2].x2 = mnumxpnts-out[i].mn;
                  msegments[index2].y1 = mnumypnts-i;
                  msegments[index2].y2 = mnumypnts-i;

                  segments[index2].x1 = mnumxpnts-out[i].mx + orgx;
                  segments[index2].x2 = mnumxpnts-out[i].mn + orgx;
                  segments[index2].y1 = mnumypnts-i + orgy;
                  segments[index2].y2 = mnumypnts-i + orgy;
                  index2++;
              }
              else
              {
                  mpoints[index1].x = mnumxpnts-out[i].mx;
                  mpoints[index1].y = mnumypnts-i;
                  points[index1].x = mnumxpnts-out[i].mx + orgx;
                  points[index1].y = mnumypnts-i + orgy;
                  index1++;
              }
              if (index1 >= YLEN || index2 >= YLEN)
	      {
		  if (index2 > 0) {
    		     if (winDraw)
		        XDrawSegments(xdisplay,xid,vnmr_gc,&segments[0],index2);
		     if (xmapDraw)
			XDrawSegments(xdisplay, canvasPixmap, pxm_gc,
                                                 &msegments[0], index2);
		  }
                  index2 = 0;
		  if (index1 > 0) {
    		    if (winDraw)
		       XDrawPoints(xdisplay, xid, vnmr_gc, &points[0],
                                index1, CoordModeOrigin);
		    if (xmapDraw)
                        XDrawPoints(xdisplay,canvasPixmap,pxm_gc,&mpoints[0],
				index1, CoordModeOrigin);
		  }
                  index1 = 0;
              }
           }
        }  /*  for loop  */
    }
    else
    {  for (i=dfpnt; i<depnt; i++)
       { if (out[i].mx>=out[i].mn) /* is in mnumypnts space */
         {
            if (out[i].mx != out[i].mn)
            {     
		  msegments[index2].x1 = i;
                  msegments[index2].x2 = i;
                  msegments[index2].y1 = mnumypnts-out[i].mx;
                  msegments[index2].y2 = mnumypnts-out[i].mn;

		  segments[index2].x1 = i + orgx;
                  segments[index2].x2 = i + orgx;
                  segments[index2].y1 = mnumypnts-out[i].mx + orgy;
                  segments[index2].y2 = mnumypnts-out[i].mn + orgy;
                  index2++;
            }
            else
            {
                  mpoints[index1].x = i;
                  mpoints[index1].y = mnumypnts-out[i].mx;
                  points[index1].x = i + orgx;
                  points[index1].y = mnumypnts-out[i].mx + orgy;
                  index1++;
            }
            if (index1 >= YLEN || index2 >= YLEN)
	    {
		  if (index2 > 0) {
    		     if (winDraw)
		        XDrawSegments(xdisplay,xid,vnmr_gc,&segments[0],index2);
		     if (xmapDraw)
			XDrawSegments(xdisplay, canvasPixmap, pxm_gc,
                                                 &msegments[0], index2);
		  }
                  index2 = 0;
		  if (index1 > 0) {
    		     if (winDraw)
		       XDrawPoints(xdisplay, xid, vnmr_gc, &points[0],
                                index1, CoordModeOrigin);
		     if (xmapDraw)
                        XDrawPoints(xdisplay,canvasPixmap,pxm_gc,&mpoints[0],
				index1, CoordModeOrigin);
		  }
                  index1 = 0;
            }
         }
       }  /*  for i loop */
    }
    if (index2 > 0) {
       if (winDraw)
             XDrawSegments(xdisplay, xid, vnmr_gc, &segments[0], index2);
       if (xmapDraw)
             XDrawSegments(xdisplay, canvasPixmap, pxm_gc, &msegments[0],
                                                 index2);
    }
    if (index1 > 0) {
       if (winDraw)
          XDrawPoints(xdisplay, xid, vnmr_gc, &points[0],
                                index1, CoordModeOrigin);
       if (xmapDraw)
          XDrawPoints(xdisplay, canvasPixmap, pxm_gc, &mpoints[0],
                                index1, CoordModeOrigin);
    }
    XFlush(xdisplay);
    draw_newCursor();
#endif  /*  MOTIF */
}

/************************************/
int sun_grayscale_box(int oldx, int oldy, int x, int y, int shade)
/************************************/
{ 
  int shade0,i,j;
  int index, max;
  int jindex, jsize;
  short *jxp, *jyp;

  if (isSuspend)
      return 1;
  if ((x<1) || (y<1)) return 1;
  xgrx = oldx;
  xgry = oldy;
  shade0 = shade;
  index = 0;
  max = JLEN - 8;

  jindex = 0;
  jybar.subtype = htonl(JXBAR);
  jybar.len = 0;
  jxp = &jybar.data[0];
  jyp = &jybar.data[1];
 
  isGraphFunc = 1;
  if (shade0>=MAXSHADES) shade0=MAXSHADES-1;
  for(i=0; i<x; i++)
    for(j=0; j<y; j++)
    { 
      if (shade0>gray_matrix[(xgrx+i)%SHADESIZE][(xgry+j)%SHADESIZE])
      {
	if (useXFunc) {
#ifdef MOTIF
          mpoints[index].x = xgrx+i;
          mpoints[index].y = mnumypnts-xgry-j;
          points[index].x = xgrx+i + orgx;
          points[index].y = mnumypnts-xgry-j + orgy;
          index++;
          if (index > 127)
          {
        	if (winDraw)
                  XDrawPoints(xdisplay, xid, vnmr_gc, &points[0], index,
                                         CoordModeOrigin);
               if (xmapDraw)
                  XDrawPoints(xdisplay, canvasPixmap, pxm_gc, &mpoints[0],
                                         index, CoordModeOrigin);
               index = 0;
          }
#endif
	}
	else {
	  *jxp = htons(xgrx+i);
	  jxp += 2;
          *jyp = htons(mnumypnts-xgry-j);
	  jyp += 2;
          jindex++;
	  *jxp = htons(xgrx+i);
	  jxp += 2;
          *jyp = htons(mnumypnts-xgry-j);
	  jyp += 2;
          jindex++;
	  if (jindex >= max) {
                jybar.len = htonl(jindex);
	        jsize = ybarHead + sizeof(short) * jindex * 2;
                if (iplotFd != NULL)
                    iplot_ybars(jindex);
                else
                    graphToVnmrJ(&jybar, jsize);
                jindex = 0;
  		jxp = &jybar.data[0];
  		jyp = &jybar.data[1];
          }
	}
      }
    }
  if (useXFunc) {
    if (index > 0)
    {
#ifdef MOTIF
      if (winDraw)
         XDrawPoints(xdisplay, xid, vnmr_gc, &points[0], index,
                                 CoordModeOrigin);
      if (xmapDraw)
         XDrawPoints(xdisplay, canvasPixmap, pxm_gc, &mpoints[0], index, CoordModeOrigin);
#endif
    }
  }
  else {
    if (jindex > 0) {
       jybar.len = htonl(jindex);
       jsize = ybarHead + sizeof(short) * jindex * 2;
       if (iplotFd != NULL)
           iplot_ybars(jindex);
       else
           graphToVnmrJ(&jybar, jsize);
    }
  }
  xgrx += x;
  xgry += y;
  return 1;
}


void sun_box(int w, int h) 
{
    if (w < 0 || h < 0)
        return;

    if (isSuspend)
        return;
    if (isClearBox != 0)
        return;
    if (w == 0) w = 1; if (h == 0) h = 1;
    my1 = mnumypnts-xgry-h;
    if (my1 < 0)
        my1 = 0;
    if (useXFunc) {
#ifdef MOTIF
        gx1 = xgrx + orgx;
        gy1 = my1 + orgy;
	if (winDraw)
            XFillRectangle(xdisplay, xid, vnmr_gc, gx1, gy1, w, h);
    	if (xmapDraw)
            XFillRectangle(xdisplay, canvasPixmap, pxm_gc, xgrx, my1, w, h);
#endif
    }
    else {
        if (isPrintBg != 0 && xorflag != 0)
           addPrintLineBox(JBOX, xgrx, my1, w, h);
        else
	   j4p_func(JBOX, xgrx, my1, w, h);
    }
    xgrx += w; xgry += h;
}

void set_graph_clear_flag(int n)
{
    if (isPrintBg != 0)
        isClearBox = n;
}

/***************/
int sun_grf_batch(int on)
/***************/
{
    x_cross_pos = 0;
    y_cross_pos = 0;
    if (!useXFunc) {
/***
	if (on) {
	    jvnmr_cmd(JBATCHON);
            batchCount = 1;
        }
	else {
           if (batchOk) {
	       jvnmr_cmd(JBATCHOFF);
               batchCount = 0;
           }
        }
***/
    }
    return 0;
}


void graph_batch(int on)
{
    x_cross_pos = 0;
    y_cross_pos = 0;

    if (!useXFunc) {
        if (on < 0) {
            jBatch(0);
        }
/***
	if (on > 0) {
	    jvnmr_cmd(JBATCHON);
            batchCount = 1;
        }
	else {
           in_batch = 0;
           if (on < 0) {
              if (batchOk) {
	            jvnmr_cmd(JBATCHOFF);
                    batchCount = 0;
              }
           }
           else if (batchOk) {
	      jvnmr_cmd(JBATCHOFF);
              batchCount = 0;
           }
        }
***/
	return;
    }
    in_batch = on;
    if (on) {
	if (xmapDraw) {
            batch_on = 1;
	}
    }
    else
        batch_on = 0;
    if (win_show && batch_on == 0)
	org_winDraw = 1;
    else
	org_winDraw = 0;
#ifdef MOTIF
    if (isSuspend > 0 || xregion == NULL || orgx < 0 ) 
	winDraw = 0;
    else
	winDraw = org_winDraw;
#endif
    if (on == 0) {
#ifdef MOTIF
	if (winDraw && canvasPixmap != 0) {
            if (xorflag)
                normalmode();
            XCopyArea(xdisplay, canvasPixmap, xid, vnmr_gc, 0, 0,graf_width, graf_height, orgx, orgy);
	}
#endif
    }
}

void set_batch_function(int on)
{
    batchOk = on;
    if (on == 0) {
       if (ginFunc) {
          ginFunc = 0;
          j4p_func(JGIN, 4, 0, 0, 0);
       }
       if (csi_opened)
          aipBatch(on);
       jBatch(on);
       // jPaintAll();
    }
}

int window_refresh(int argc, char *argv[], int retc, char *retv[])
{
      popBackingStore();
      RETURN;
}

// ask VJ to turn on xor painting
void vj_xoron() {
    noXorYbar = 0;
    j1p_func(JXORON, 0);
}

// ask VJ to turn off xor painting
void vj_xoroff() {
    noXorYbar = 1;
    j1p_func(JXOROFF, 0);
}

int xoron(int argc, char *argv[], int retc, char *retv[])
{
    (void) argc;
    (void) argv;
    int k = 1;

    if (argc > 1)
       k = atoi(argv[1]);
    if (k < 1)
       noXorYbar = 0;
    j1p_func(JXORON, k);
    RETURN;
}


int xoroff(int argc, char *argv[], int retc, char *retv[])
{
    (void) argc;
    (void) argv;
    int k = 1;

    if (argc > 1)
       k = atoi(argv[1]);
    if (k < 1)
       noXorYbar = 1;
    j1p_func(JXOROFF, k);
    RETURN;
}

void copy_from_pixmap(int x, int y, int x2, int y2, int w, int h)
{
    if (!win_show)
	return;
    if (!useXFunc) {
        jCopyArea(x, y, w, h);
	return;
    }
#ifdef MOTIF
    if (canvasPixmap)
    {
	if (orgx < 0 || xregion == NULL)
	    return;
        if (xorflag)
            normalmode();
        XCopyArea(xdisplay, canvasPixmap, xid, vnmr_gc, x, y, w, h,
		 x2+orgx, y2+orgy);
    }
#endif
}

void copy_aipmap_to_window(int x, int y, int x2, int y2, int w, int h)
{
    if (!useXFunc) {
        // use jBatch(0) to render image to screen right away
	return;
    }
    copy_from_pixmap(x, y, x2, y2, w, h);
}

void clear_pixmap(int x, int y, int w, int h)
{
    if (!useXFunc) {
        jBatch(1);
        if (csidebug > 1)
           fprintf(stderr,"  clear_pixmap  1:  %d %d %d %d \n", x, y, w, h);
        j6p_func(JCLEAR2, 1, x, y, w, h, 0);
	return;
    }
#ifdef MOTIF
    if (canvasPixmap)
    {
        if (xorflag)
            normalmode();
        XSetForeground(xdisplay, pxm_gc, xblack);
        XFillRectangle(xdisplay, canvasPixmap, pxm_gc, x, y, w, h);
    }
#endif
}

void aip_clear_pixmap(int x, int y, int w, int h)
{
    if (!useXFunc) {
        aipBatch(1);
        if (csidebug > 1)
           fprintf(stderr,"  aip_clear_pixmap  1:  %d %d %d %d \n", x, y, w, h);
        aip_j6p_func(JCLEAR2, 1, x, y, w, h, 0);
	return;
    }
    else
        clear_pixmap(x, y, w, h);
}

static void set_frame_size(int x, int y, int w, int h)
{
        if (w != frameWidth || h != frameHeight)
        {
	    sizeChanged = 1;
            frameWidth = w;
            frameHeight = h;
            if (!useXFunc && active_frame > 0)
                aspFrame("resize", active_frame, x, y, w, h);
        }
        else
            sizeChanged = 0;
        orgx = x;
        orgy = y;
	graf_width = w;
	graf_height = h;
	if (useXFunc) {

#ifdef MOTIF
           if ( isSuspend == 0 && xregion != NULL)
               winDraw = org_winDraw;
#endif
	}

}

void jframeFunc(char *cmd, int id, int x, int y, int w, int h)
{
    int  f;
    WIN_STRUCT   *newf;

    if (cmd == NULL)
	return;
    f = 0;
    if (strcmp(cmd, "open") == 0 || strcmp(cmd, "new") == 0 || strcmp(cmd, "reopen") == 0) {
        f = JFRAME_OPEN;
        if (id < 0)
             id = 0;
        if (frameNums <= id)
            create_frame_info_struct(id);
        newf = &frame_info[id];
        if (newf == NULL)
            return;
        newf->active = 1;
        newf->x = x;
        newf->y = y;
        newf->isClosed = 0;
        newf->isVertical = 0;
        if (w <= 1)
           newf->width = win_width;
        else
           newf->width = w;
        if (h <= 1)
           newf->height = win_height;
        else
           newf->height = h;
        newf->fontW = char_width;
        newf->fontH = char_height;
        newf->ascent = char_ascent;
        newf->descent = char_descent;
        // active_frame = id;
        XGAP = 0;
        YGAP = 0;
        if (getGraphicsCmd() > 0) {
           if (isReexecCmd(gmode) > 0) {
               strcpy(newf->graphics, gmode);
               strcpy(newf->graphMode, gmode);
           }
        }

        if (inRsizesMode == 0) {
	   if (useXFunc) {
               set_frame_size(x, y, w, h);
               mnumxpnts = w;
               mnumypnts = h;
           }
           else {
               set_frame_size(0, 0, w, h);
               if (sizeChanged) {
                   mnumxpnts = w;
                   mnumypnts = h;
               }
           }
        }
	aspFrame(cmd,id,x,y,w,h);
        // set_jframe_id(id);
    }
    else if (strcmp(cmd, "close") == 0) {
        f = JFRAME_CLOSE;
    }
    else {
        if (strcmp(cmd, "closeall") == 0) {
            if (active_frame > 1)
                set_jframe_id(1);
            f = JFRAME_CLOSEALL;
        }
        else if (strcmp(cmd, "closealltext") == 0)
            f = JFRAME_CLOSEALLText;
        else if (strcmp(cmd, "active") == 0) {
            f = JFRAME_ACTIVE;
        }
        else if (strcmp(cmd, "enable") == 0) {
            f = JFRAME_ENABLE;
        }
        else if (strcmp(cmd, "enableAll") == 0) {
            f = JFRAME_ENABLEALL;
        }
        else if (strcmp(cmd, "disable") == 0)
            f = JFRAME_IDLE;
        else if (strcmp(cmd, "disableAll") == 0)
           f = JFRAME_IDLEALL;
    }
    if (f == 0)
	return;

    if (!useXFunc) {
        j6p_xfunc(JFRAME, f, id, x, y, w, h);
	return;
    }
}

#ifndef INTERACT

extern char *get_x_resource();
#endif 

Pixel get_vnmr_pixel_by_name(char *name)
{
#ifdef MOTIF
	XColor   xcolor, xcolor2;

	if (!useXFunc)
	     return(0);
	if(XAllocNamedColor(xdisplay, xcolormap, name, &xcolor, &xcolor2))
	    return(xcolor.pixel);
	else
#endif
	    return(0);
}


static void
setGridGeom()
{
#ifndef INTERACT
     int   k, m, w, h;
     WIN_STRUCT  *win;

     w = (win_width - (grid_cols + 1) * grid_width - 1) / grid_cols;
     h = (win_height - (grid_rows + 1) * grid_width - 1) / grid_rows;
     for (k = 0; k < grid_rows; k++)
     {
         for (m = 0; m < grid_cols; m++)
         {
             win = &win_dim[k * grid_cols + m];
             win->x = m * w + (m + 1) * grid_width;
             win->width = w;
             win->y = k * h + (k + 1) * grid_width;
             win->height = h;
             if (w > win->old_width || h > win->old_height) {
#ifdef MOTIF
                 if (win->xmap != 0)
                      XFreePixmap(xdisplay, win->xmap);
                 win->xmap = XCreatePixmap(xdisplay, xid,
                      w+2*grid_width, h+2*grid_width, n_gplanes);
                 if (win->xmap) {
                      XSetForeground(xdisplay, pxm_gc, xblack);
                      XFillRectangle(xdisplay, win->xmap, pxm_gc, 0, 0,
                            w+2*grid_width, h+2*grid_width);
                 }
#endif
                 win->old_width = w;
                 win->old_height = h;
             }
         }
     }
#endif
}

static void
drawGridLines()
{
#ifndef INTERACT
     int   k;
#ifdef MOTIF
     char  *res;
#endif

     if (grid_num <= 1)
         return;
     if (!useXFunc)
          return;

#ifdef MOTIF
     if (idle_pix == 999)
     {
         if ((res=get_x_resource("Vnmr*gridLine", "idleColor")) != NULL)
             idle_pix = get_vnmr_pixel_by_name(res);
         else
             idle_pix = sun_colors[BLUE];
         if ((res=get_x_resource("Vnmr*gridLine", "activeColor")) != NULL)
                active_pix = get_vnmr_pixel_by_name(res);
         else
                active_pix = sun_colors[RED];
         grid_width = 1;
         if ((res=get_x_resource("Vnmr*gridLine", "width")) != NULL) {
                grid_width = atoi(res);
                if (grid_width < 1) grid_width = 1;
                   XSetLineAttributes(xdisplay, grid_gc, grid_width, LineSolid, CapButt, JoinMiter);
         }
     }
#endif
      
     for (k = 0; k < grid_num; k++)
     {
        if (win_dim[k].active == 0)
             drawWinBorder(&win_dim[k]);
     }
     if (curActiveWin != NULL)
     {
         if (curActiveWin->xmap != 0)
            canvasPixmap = curActiveWin->xmap;
         else
            canvasPixmap = org_Pixmap;
         drawWinBorder(curActiveWin);
	 orgx = curActiveWin->x + org_orgx;
	 orgy = curActiveWin->y + org_orgy;
     }
     if (canvasPixmap != 0 && bksflag)
        xmapDraw = 1;

#endif
}

void init_screen_plane_list(Display *dpy)
{
#ifdef MOTIF
	int	*dlist, m;

	if (!useXFunc)
	     return;
	dlist = (int *)XListDepths (dpy, XDefaultScreen(dpy), &depth_list_count);
	if (dlist != NULL && depth_list_count > 0)
	{
	     depth_list = (int *) malloc(sizeof(int) * depth_list_count);
	     for (m = 0; m < depth_list_count; m++)
	     {
		depth_list[m] = *(dlist+m);
		if (depth_list[m] == 1)
		    one_plane = 1;
	     }
	     XFree(dlist);
	}
#endif
}

int check_get_plane_num(int num)
{
	int	k;

        if (!useXFunc)
	    return (0);
	if (depth_list == NULL || depth_list_count <= 0)
	    return (0);
	for (k = 0; k < depth_list_count; k++)
	{
	    if (depth_list[k] == num)
		return(num);
	}
	if (num == 1)
	    return(0);
	for (k = 0; k < depth_list_count; k++)
	{
	    if (depth_list[k] >= 6)
		return(depth_list[k]);
	}
	return(0);
}

void
list_plane_num()
{
}

void
create_rast_image()
{
#ifdef MOTIF
	XWindowAttributes      win_attr;
	int	len, k;

        if (!useXFunc || xid <= 0)
	    return;
	if (rast_image != NULL)
	{
	    if (rast_image_width >= win_width)
	        return;
            XDestroyImage(rast_image);
	    rast_image = NULL;
	}
	rast_image_width = 0;
	if (!XGetWindowAttributes(xdisplay,  xid, &win_attr))
	       return;
	len = DisplayWidth (xdisplay, XDefaultScreen(xdisplay));
	if (len < win_width)
	    len = win_width;
	k = BitmapUnit(xdisplay) / 8;
	if (k < 4)
	    k = 4;
	ximg_data = (char *) malloc((len +1) * k);

	if (ximg_data != NULL)
	{
	    rast_image = XCreateImage(xdisplay, win_attr.visual, win_attr.depth,
			ZPixmap, 0, ximg_data, len, 1, 8, 0);
	    if (rast_image == NULL)
	    {
	   	rast_image = XCreateImage(xdisplay, win_attr.visual, win_attr.depth,
			XYPixmap, 0, ximg_data, len, 1, 8, 0);
	    }
	    if (rast_image != NULL)
	   	rast_image_width = len;
	}
	if (rast_image == NULL)
	{
	   if ((win_attr.map_state & IsViewable) == 0)
	        return;
	   if (win_attr.width < 6)
		return;
           rast_image = XGetImage(xdisplay, xid, 0, 1, win_attr.width-2, 1, AllPlanes,ZPixmap);
           if (rast_image == NULL)
            	rast_image = XGetImage(xdisplay, xid, 0, 1, win_attr.width-2, 1, AllPlanes,XYPixmap);
           if (rast_image != NULL)
	       rast_image_width = win_attr.width - 2;
	}
	if ((rast_image == NULL) && (canvasPixmap != 0))
	{
             rast_image = XGetImage(xdisplay, canvasPixmap, 0, 0,
			   backing_width, 1, AllPlanes,ZPixmap);
             if (rast_image == NULL) {
                rast_image = XGetImage(xdisplay, canvasPixmap, 0, 0, 
			backing_width, 1, AllPlanes,XYPixmap);
	     }
	     if (rast_image != NULL)
	        rast_image_width = backing_width;
	}
	if (rast_image == NULL)
	     return;

	rast_do_flag = 0;
	if (rast_image->format == ZPixmap)
        {
            rast_do_flag = 99;
            if (rast_image->bitmap_bit_order == MSBFirst)
            {
                rast_do_flag = rast_image->bits_per_pixel / 8;
                if (rast_image->byte_order == LSBFirst)
                    rast_do_flag += 10;
            }
        }
#endif  /*  MOTIF */
}

static int
set_window_dim()
{
	// xcharpixels = char_width;
	// ycharpixels = char_height;
        if (graf_width == 0)
        {
          graf_width = INIT_WIDTH;
          graf_height = INIT_HEIGHT;
        }
	mnumxpnts = graf_width - XGAP;
	mnumypnts = graf_height - YGAP;
	if (mnumxpnts < 0)
	    mnumxpnts = 0;
	if (mnumypnts < 0)
	    mnumypnts = 0;
        if (csi_opened < 1) {
            aip_mnumxpnts = mnumxpnts;
            aip_mnumypnts = mnumypnts;
        }
	if (useXFunc) {
	    create_rast_image();		
	}
	return(0);
}


void set_win_size(int x, int y, int w, int h)
{
	if (w < 0)
	    w = 0;
	if (h < 0)
	    h = 0;
        if (inPrintSetupMode != 0) {
            prtWidth = w;
            prtHeight = h;
            return;
        }
        if (w != win_width || h != win_height)   
	    sizeChanged = 1;
        orgx = x;
        orgy = y;
        org_orgx = x;
        org_orgy = y;
/*
	graf_width = w;
	graf_height = h;
*/
	win_width = w;
	win_height = h;
        if (main_frame_info == NULL)
            create_frame_info_struct(1);
        if (active_frame > 0) {
	    sizeChanged = 0;
        }
        else {
            main_frame_info->x = x;
            main_frame_info->y = y;
            main_frame_info->width = w;
            main_frame_info->height = h;
	    graf_width = w;
	    graf_height = h;
        }
	if (useXFunc) {

#ifdef MOTIF
           if ( isSuspend == 0 && xregion != NULL)
               winDraw = org_winDraw;
#endif

           if (sizeChanged && grid_num > 1)
              setGridGeom();
	   if (old_winWidth < w || old_winHeight < h) {
	          sun_window();
           }
           if (grid_num > 1) {
               drawGridLines();
	       if (sizeChanged && curActiveWin != NULL) {
	           graf_width = curActiveWin->width;
	           graf_height = curActiveWin->height;
                   mnumxpnts = graf_width - XGAP;
                   mnumypnts = graf_height - YGAP;
	           orgx = curActiveWin->x + org_orgx;
	           orgy = curActiveWin->y + org_orgy;
               }
           }
	}
        if (active_frame > 0)
	   frame_updateG("wsize", 0, x, y, w, h);
        else if (!useXFunc)
	   set_window_dim();

        if (csi_opened < 1 && active_frame < 1) {
           aip_mnumxpnts = mnumxpnts;
           aip_mnumypnts = mnumypnts;
	   // iplan_set_win_size(x, y, w, h);
           // aipSetGraphicsBoundaries(x, y, w, h);
           set_aip_win_size(x, y, w, h);
        }

/*
	frame_updateG("wsize", active_frame, x, y, w, h);
*/
}

void get_main_win_size(int *x, int *y, int *w, int *h) {
  *x=main_frame_info->x;
  *y=main_frame_info->y;
  *w=main_frame_info->width;
  *h=main_frame_info->height;
}

void set_csi_win_size(int x, int y, int w, int h)
{
	if (w < 0)
	    w = 0;
	if (h < 0)
	    h = 0;
        if (inPrintSetupMode != 0) {
            prtWidth = w;
            prtHeight = h;
            return;
        }
        csi_orgx = 0;
        csi_orgy = 0;
	csi_win_width = w;
	csi_win_height = h;
        if (main_frame_info == NULL)
            create_frame_info_struct(1);
	csi_graf_width = w;
	csi_graf_height = h;
        if (csi_opened) {
           aip_mnumxpnts = mnumxpnts;
           aip_mnumypnts = mnumypnts;
           // iplan_set_win_size(x, y, w, h);
           set_aip_win_size(0, 0, w, h);
        }
}

void set_win2_size(int x, int y, int w, int h)
{
	orgx2 = x;
	orgy2 = y;
}

int
is_new_java_graphics()
{
     if (!useXFunc && active_frame > 0)
        return (1);
     return(0);
}

void set_jframe_loc(int id, int x, int y, int w, int h)
{
     WIN_STRUCT  *f;

     if (id < 0)
         return;
     aspFrame("loc",id,x,y,w,h);
     frame_updateG("loc", id, x, y, w, h);
     if (frame_info == NULL)
         return;
     f = (WIN_STRUCT *) &frame_info[id];
     if (f == NULL)
          return;
     f->x = x;
     f->y = y;
     f->width = w;
     f->height = h;
}

void set_csi_jframe_loc(int id, int x, int y, int w, int h)
{
}

void set_jframe_vertical(int id, int v)
{
      WIN_STRUCT  *f;

      if (frame_info == NULL || id >= frameNums)
          return;
      f = (WIN_STRUCT *) &frame_info[id];
      if (f == NULL)
          return;
      f->isVertical = v;
}

void set_jframe_size(int id, int x, int y, int w, int h)
{
      WIN_STRUCT  *f;

      if (xdebug > 1)
          fprintf(stderr, " set_jframe_size %d: %d %d \n", id, w, h);
      if (id >= frameNums) {
          jframeFunc("open", id, x, y, w, h);
          return;
      }
      if (frame_info == NULL)
          return;
      if (id <= 0) {
         /*****
          if (inRsizesMode) {
             // for (k = 0; k < frameNums; k++)
             //    frame_info[k].active = 0;
          }
         ******/
          if (active_frame > 0)
             old_active_frame = active_frame;
          return;
      }
      f = (WIN_STRUCT *) &frame_info[id];
      if (f == NULL)
          return;
      f->x = x;
      f->y = y;
      f->active = 1;

      if (w != f->width || h != f->height) {
          f->width = w;
          f->height = h;
          set_frame_font(f, AxisNumFontName);
          set_frame_font(f, DefaultFontName);
      }
      if (id == 1 && csi_opened < 1) {
          aip_mnumxpnts = mnumxpnts;
          aip_mnumypnts = mnumypnts;
          if (w > 12 && h > 12)
              set_aip_win_size(x, y, w, h);
      }
      aspFrame("resize",id,x,y,w,h);
      if (active_frame == id && inRsizesMode == 0) {
          jBatch(1);
	  if (useXFunc) {
             set_frame_size(x, y, w, h);
             mnumxpnts = w;
             mnumypnts = h;
          }
          else {
             set_frame_size(0, 0, w, h);
             if (sizeChanged) {
                 mnumxpnts = w;
                 mnumypnts = h;
             }
          }
	  frame_updateG("resize", id, x, y, w, h);
          window_redisplay();
      }
}

void set_csi_jframe_size(int id, int x, int y, int w, int h)
{
}

void
set_pframe_size(int id, int x, int y, int w, int h)
{
      WIN_STRUCT  *f;

      if (id >= frameNums || frame_info == NULL)
          return;
      f = (WIN_STRUCT *) &frame_info[id];
      if (f == NULL)
          return;
      f->prtX = x;
      f->prtY = y;
      f->prtW = w;
      f->prtH = h;
      f->prtFontW = prtCharWidth;
      f->prtFontH = prtCharHeight;
      f->prtAscent = prtChar_ascent;
      f->prtDescent = prtChar_descent;
}

void
set_csi_pframe_size(int id, int x, int y, int w, int h)
{
}

void
set_jframe_status(int id, int s)
{
      if (frame_info == NULL || id >= frameNums)
          return;
      if (s < 1) {
          frame_info[id].isClosed = 1;
          clear_frame_graphFunc(&frame_info[id]);
      }
      else
          frame_info[id].isClosed = 0;
}

void
set_csi_jframe_status(int id, int s)
{
}

void
set_jframe_id(int id)
{
      WIN_STRUCT *frame, *oldf;
      int old_id;
      char cmd[136];

      isSuspend = 0;

      if (xdebug > 0)
        fprintf(stderr, "  set_jframe_id %d in %d\n", id, frameNums);
      if (frame_info == NULL || frameNums <= id)
          create_frame_info_struct(id);
      if (id <= 0) {
          if (active_frame > 0) {
             active_frame = 0;
	     if (useXFunc)
               set_win_size(main_frame_info->x, main_frame_info->y, win_width, win_height);
             else {
               main_frame_info->x = 0;
               main_frame_info->y = 0;
               main_frame_info->width = win_width;
               main_frame_info->height = win_height;
	       graf_width = win_width;
	       graf_height = win_height;
             }
             for (old_id = 2; old_id < frameNums; old_id++) 
                frame_info[old_id].isClosed = 1;
          }
          old_active_frame = 0;
          is_dconi = 0; 
          return;
      }
      old_id = active_frame;
      if (old_id != id && old_id >= 0) { 
          oldf = &frame_info[old_id];
          if (oldf != NULL) {
#ifndef INTERACT
              oldf->mouseMove = mouseMove;
              oldf->mouseButton = mouseButton;
              oldf->mouseReturn = mouseReturn;
              oldf->mouseClick = mouseClick;
              oldf->mouseGeneral = mouseGeneral;
              oldf->mouseActive = Wisactive_mouse();
              oldf->mouseJactive = WisJactive_mouse();
#endif
              sprintf(oldf->menu, "%s", help_name);
              if (getGraphicsCmd() > 0)
                  sprintf(oldf->graphMode, "%s", gmode);
          }
      }
/*
          if (active_frame > 0)
              Wgetgraphicsdisplay(frame_info[active_frame].graphics, GRAPHCMDLEN);
*/
       frame = &frame_info[id];
       if (sizeChanged) {
           if (getGraphicsCmd() > 0)
              strcpy(frame->graphMode, gmode);
    	   // Wgetgraphicsdisplay(frame->graphMode,GRAPHCMDLEN);
       }

       active_frame = id;
       if (useXFunc) {
            set_frame_size(frame->x, frame->y, frame->width, frame->height);
            mnumxpnts = frame->width;
            mnumypnts = frame->height;
       }
       else {
            set_frame_size(0, 0, frame->width, frame->height);
            if (sizeChanged) {
                mnumxpnts = frame->width;
                mnumypnts = frame->height;
            }
       }
       // set_vj_font_size(frame->fontW, frame->fontH, frame->ascent, frame->descent);
       set_font_size_with_frame(frame);
       is_dconi = 0; 
       if (strncmp(frame->graphics, "dconi", 5) == 0 || strncmp(frame->graphics, "dpcon", 5) == 0)
             is_dconi = 1; 

       frame_updateG("select", id, frame->x, frame->y, frame->width, frame->height);
       getDscaleFrameInfo();
       if (isPrintBg != 0 && xdebug > 0) {
           fprintf(stderr, "--- paint frame %d --- \n", id);
           fprintf(stderr, " frame geom:  %d %d %d %d \n", frame->x, frame->y, frame->width, frame->height);
           fprintf(stderr, " grapics size: %d %d \n", mnumxpnts, mnumypnts);
       }

       if (isDrawable(id) < 1) {
           isSuspend = 1;
           return;
       }

       if (isPrintBg == 0 && inRepaintMode == 0 && active_frame != old_id)
       {
           if (frame->mouseActive == 0 || frame->mouseMove == NULL)
               Wturnoff_mouse();
           else {

               if (frame->mouseJactive != 0) {
                   Jactivate_mouse(frame->mouseMove, frame->mouseButton,
                       frame->mouseClick, frame->mouseGeneral, frame->mouseReturn);
               }
               else
                  Wactivate_mouse(frame->mouseMove, frame->mouseButton, frame->mouseReturn);
           }
           if(strlen(frame->menu) > 0) {
                if(strcmp(frame->menu, help_name) != 0) {
                      sprintf(cmd, "menu('%s')\n", frame->menu);
                      execString(cmd);
                }
           }
       }
       Wsetgraphicsdisplay(frame->graphMode);
       // set full chart size (this is called when window resized).
       if(strstr(frame->graphMode,"dconi") ||
          strstr(frame->graphMode,"ds") || strstr(frame->graphMode,"asp"))
       {
          if(adjustFull()) setFullChart(d2flag);
       }
       // window_redisplay();
}

void
set_active_frame(int id)
{
      WIN_STRUCT *frame;
      int oldActive;

      oldActive = active_frame;

      set_jframe_id(id);
      if (id < 1 || oldActive == id)
          return;

      frame = &frame_info[id];
      if (frame == NULL)
          return;
      if (strlen(frame->graphics) > 0) {
          if (strstr(frame->graphics,"con") != NULL)  // dcon, dconi, dpcon
              return;
      }
	aspFrame("active",id,orgx,orgy,graf_width,graf_height);
      window_redisplay();
      Wturnon_message();
}

void
set_csi_active_frame(int id)
{
}

void
set_win_region(int order, int x, int y, int x2, int y2)
{
#ifdef MOTIF
	XRectangle  rect;

	if (!useXFunc)
	     return;
	if (x2 < x)
	    x2 = x;
	if (y2 < y)
	    y2 = y;
	if (order == 1) {
	    if (xregion != NULL) {
                if (region_x == x && region_x2 == x2) {
                    if (region_y == y && region_y2 == y2)
                       return;
                }
                XDestroyRegion(xregion);
            }
	    xregion = XCreateRegion();
	    region_x = x;
	    region_y = y;
	    region_x2 = x2;
	    region_y2 = y2;
        }
        else if (xregion == NULL)
	    xregion = XCreateRegion();
	rect.x = x;
	rect.y = y;
	rect.width = x2 - x;
	rect.height = y2 - y;
	XUnionRectWithRegion(&rect, xregion, xregion);

	XSetRegion (xdisplay, vnmr_gc, xregion);
        if ( isSuspend == 0 && orgx > 0 )
             winDraw = org_winDraw;
/*
	aipSetCanvasMask(xregion);
*/
/*
	XFlush(xdisplay);
*/
#endif
}

void revalidatePrintCommand(char *name)
{
#ifdef OLD
    if (isPrintBg == 0)
         return;
    if (xdebug > 0)
         fprintf(stderr, " revalidatePrintCommand:  %s  \n", name);
    if (strlen(lastPlotCmd) < 1)
         return;
    getGraphicsCmd();
    if (xdebug > 0)
         fprintf(stderr, "  graphics new:  '%s'  old:  '%s'\n", gmode, lastPlotCmd);
    if (strcmp(lastPlotCmd, gmode) == 0)
         lastPlotCmd[0] = '\0';
#endif
}


static void execPrtCmd(char *cmd, int type) {
    if (xdebug > 0)
        fprintf(stderr, "    exec plot cmd: %s", cmd);
    if (type == 0)
        execString(cmd);
    else
        loadAndExec(cmd);
}

// old func for single viewport and single frame.
// draw one frame only
void window_redisplay() {
    char *cmd, gcmd[68];
    int hasCmd;
    int hasDconi;
    int toSkip;
    WIN_STRUCT *fmInfo;
    GCMD *glist;

    if (isSuspend > 0) {
        return;
    }
    if (frame_info == NULL)
        return;
    if (isWinRedraw <= 0)
        initWinRedraw = 1;
    winDraw = org_winDraw;
    keepOverlay = 1;
    hasCmd = 0;
    hasDconi = 0;
    fmInfo = getCurrentFrameStruct();
    if (fmInfo == NULL)
        return;
    isWinRedraw = 1;
    if (isPrintBg || iplotFd != NULL)
        P_setreal(GLOBAL, "mfShowFields", 0, 1);

    // this calls clearGraphFunc if in aspMode
    aspFrame("redraw",active_frame,0,0,0,0);

    if (xdebug < 1)
        Wturnoff_message();
    if (xdebug > 0)
        fprintf(stderr, "  window_redisplay ... \n");
    cmd = fmInfo->graphics;
    if (active_frame < 2 && (cmd == NULL || strlen(cmd) == 0)) {
      getGraphicsCmd();
      cmd = gmode; 
      if (isReexecCmd(gmode) <= 0)
          cmd = NULL;
    }
    ybarSize = 0;
    if (isPrintBg == 0)
       jBatch(1);
    if (xdebug > 0)
        fprintf(stderr, "   redraw frame %d ... \n", active_frame);
    if (cmd != NULL && strlen(cmd) > 0) {
        is_dconi = 0;
        if (strncmp(cmd, "dconi", 5) == 0 || strncmp(cmd, "dpcon", 5) == 0)
        {
            is_dconi = 1;
            hasDconi = 1;
        }
        if (fmInfo->graphType == 2)
            sprintf(gcmd, "%s('again')\n", cmd);
        else if (fmInfo->graphType == 3)
            sprintf(gcmd, "%s('redisplay')\n", cmd);
        else
            sprintf(gcmd, "%s\n", cmd);
        hasCmd = 1;
        if (xdebug > 0)
            fprintf(stderr, "    exec graphics cmd: %s      type %d \n", gcmd, fmInfo->graphType);
        if (isPrintBg != 0)
            execPrtCmd(gcmd, 0);
        else {
            sun_window();
            execString(gcmd);
        }
    }
    glist = fmInfo->cmdlist;
    while (glist != NULL) {
        if (glist->size > 0 && glist->cmd != NULL) {
            if (strlen(glist->cmd) > 0) {
                toSkip = 0;
                if (strncmp(glist->cmd, "dconi", 5) == 0 || strncmp(glist->cmd,
                        "dpcon", 5) == 0) {
                    is_dconi = 1;
                    if (glist->graphType < 5) {
                        hasDconi++;
                        if (hasDconi > 1)
                           toSkip = 1;
                    }
                }
                else if (strcmp(glist->cmd, "dcon") == 0) {
                    if (hasDconi > 0)
                        toSkip = 1;
                }
                if (toSkip == 0) {
                   if (glist->graphType == 2)
                       sprintf(execCmdStr, "%s('again')\n", glist->cmd);
                   else if (glist->graphType == 3)
                       sprintf(execCmdStr, "%s('redisplay')\n", glist->cmd);
                   else
                       sprintf(execCmdStr, "%s\n", glist->cmd);
                   if (xdebug > 0)
                       fprintf(stderr, "    exec saved cmd: %s", execCmdStr);
                   if (isPrintBg != 0) 
                       execPrtCmd(execCmdStr, 0);
                   else {
                       if (hasCmd == 0)
                           sun_window();
                       execString(execCmdStr);
                   }
                   hasCmd = 1;
                }
            }
        }
        glist = glist->next;
    }
    if (isPrintBg != 0) {
        if (ybarSize > 1)
            paint_ybars();
        paint_printData();
    } 
    if (hasCmd) {
        if (overlayNum > 0)
            redraw_overlay_image();
    } else {
        redrawCanvas();
    }
    update3PCursors();

    Wturnon_message();
    sizeChanged = 0;
    keepOverlay = 0;
    isWinRedraw = isPrintBg;
    if (isPrintBg != 0 || iplotFd != NULL)
        jvnmr_cmd(FRMPRTSYNC);
    isNewGraph = 0;
    if (xdebug > 0)
        fprintf(stderr, "  window_redisplay done \n");
}

static void 
repaint_frames(int paint_all, int do_flush)
{
      int k, activeId;
      WIN_STRUCT *frame;

      if (xdebug > 0)
        fprintf(stderr, " repaint_frames all %d \n", paint_all);
      if (active_frame < 1) {
          active_frame = old_active_frame;
          if (active_frame < 0)
             active_frame = 0;
      }
      inRepaintMode = 0;
      jBatch(1);
      inRepaintMode = 1;
      if (paint_all == 0 || active_frame <= 0) {   
          window_redisplay();
          inRepaintMode = 0;
          if (do_flush)
              jBatch(0);
          Wturnon_message();
          return;
      }
      if (frame_info == NULL) {
          inRepaintMode = 0;
          return;
      }
      keepOverlay = 1;
      activeId = active_frame; 
      grf_batch(1); 
      for (k = 1; k < frameNums; k++) {
          if (k == activeId || frame_info[k].isClosed) 
             continue;
          if (isDrawable(k)) {
             frame = &frame_info[k];
             j6p_func(JFRAME, JFRAME_ACTIVE, k, frame->x, frame->y, frame->width, frame->height);
             set_jframe_id(k);
             sun_setdisplay();
             window_redisplay();
          }
      }
      if (isDrawable(activeId)) {
         frame = &frame_info[activeId];
         j6p_func(JFRAME, JFRAME_ACTIVE, activeId, frame->x, frame->y, frame->width, frame->height);
         set_jframe_id(activeId);
         sun_setdisplay();
         window_redisplay();
      }
      keepOverlay = 0;
      inRepaintMode = 0;
      if (do_flush) {
         if (paint_all)
            jPaintAll();
         else
            jBatch(0);
      }
      Wturnon_message();
}

int window_repaint(int argc, char *argv[], int retc, char *retv[])
{
      int k, paint_all;

      paint_all = 0;
      for (k = 1; k < argc; k++) {
         if (strcasecmp(argv[k], "all") == 0)
               paint_all = 1;
      }
      if (xdebug > 0) {
         if (paint_all)
            fprintf(stderr, " window_repaint ...all  \n");
         else
            fprintf(stderr, " window_repaint ...active only  \n");
      }
      repaint_frames(paint_all, 0);
      // align spectra when window resize.
      if(s_overlayMode == OVERLAID_ALIGNED) overlaySpec(OVERLAID_ALIGNED);
      if (paint_all)
         jPaintAll();
      else
         jBatch(0);
      RETURN;
}

void repaint_canvas(int fromCsi) {
     if (xdebug > 0) {
        if (fromCsi > 0)
            fprintf(stderr, "repaint_canvas called by csi\n");
        else
            fprintf(stderr, "repaint_canvas \n");
     }
     if (isPrintBg != 0)
        return;
     curPalette = 0;
     if (fromCsi == 0) {
        sizeChanged = 1;
        if (active_frame < 1)
           set_win_size(0, 0, main_win_width, main_win_height);
        repaint_frames(1, 0);
        if(s_overlayMode == OVERLAID_ALIGNED) overlaySpec(OVERLAID_ALIGNED);
        jPaintAll();
     }
     else {
        if (csi_opened) {
            sprintf(gmode, "%s\n", aipRedisplay); 
            execString(gmode);
        }
     }
}

void set_csi_font_size(int w, int h, int a, int d)
{
      csi_char_width = w;
      csi_char_height = h;
      if (a > 0)
          csi_char_ascent = a;
      if (d > 0)
          csi_char_descent = d;
      aip_xcharpixels = w;
      aip_ycharpixels = h;
}

void set_frame_font_size(int id, int w, int h, int a, int d)
{
     WIN_STRUCT *f;

     if (id < 0)
         return;
     if (id >= frameNums || frame_info == NULL)
          return;
     f = (WIN_STRUCT *) &frame_info[id];
     if (f == NULL)
          return;
     if (defaultFont != NULL)
          return;
     f->fontW = w;
     f->fontH = h;
     f->ascent = a;
     f->descent = d;
}

static void
switch_iplot_file()
{
     if (iplotTopFd == NULL)
         return;
     if (isTopframeOn)
         iplotFd = iplotTopFd;
     else
         iplotFd = iplotMainFd;
}

void start_iplot(FILE *fd, FILE *topFd, int w, int h) {
#ifndef INTERACT
      double rw, rh, dpi;
      int k, n, fntW, fntH, fntD;
      unsigned char *psReds, *psGreens, *psBlues;
      unsigned char *reds, *greens, *blues;
      WIN_STRUCT *frame;

      if (useXFunc || w < 10 || h < 10)
          return;
      if (fd == NULL)
          return;
      if (xdebug > 0) {
          fprintf(stderr, "start_iplot  wh %d %d \n", w, h);
          fprintf(stderr, "  frames %d  active frame  %d \n",frameNums, active_frame);
      }
      iplotFd = fd;
      iplotMainFd = fd;
      iplotTopFd = topFd;
      fprintf(iplotFd, "#topontop %d\n", isTopOnTop);
      fprintf(iplotFd, "#endSetup\n");
      if (iplotTopFd != NULL)
          fprintf(iplotTopFd, "#endSetup\n");
      inRsizesMode = 1;
      create_vj_font_list();
      dpi = get_ploter_ppmm();
      if (dpi < 2.0)
         dpi = 11.811;
      dpi = dpi * 25.4;
      rh = dpi / scrnDpi;
      if (rh < 0.2)
         rh = 0.5;
      if (rh > 20.0)
         rh = 20.0;
      reset_frame_plot_fonts(rh);
      if (main_frame_info == NULL)
          jgraphics_init();
      if (main_win_width < 10)
          main_win_width = w;
      if (main_win_height < 10)
          main_win_height = h;
      rw = (double) w / (double) main_win_width;
      rh = (double) h / (double) main_win_height;
      graf_width = w;
      graf_height = h;
      // set_main_win_size(0, 0, w, h);
      jplot_charsize(1.0);
      fntW = xcharpixels;
      fntD = ycharpixels * 0.2;
      if (fntD < 2)
          fntD = 2;
      fntH = ycharpixels - fntD;
      char_width = xcharpixels;
      char_height = ycharpixels;
      set_font_size(xcharpixels, ycharpixels, fntH, fntD);
      if (frame_info != NULL) {
          for (k = 0; k < frameNums; k++) {
               frame = &frame_info[k];
               frame->x = (int) (rw * frame->x);
               frame->y = (int) (rh * frame->y);
               if (xdebug > 0 && frame->isClosed == 0)
                  fprintf(stderr, "  frame %d  wh %d %d ", k, frame->width, frame->height);
               frame->width = (int) (rw * frame->width);
               frame->height = (int) (rh * frame->height);
               if (xdebug > 0 && frame->isClosed == 0)
                  fprintf(stderr, "  -> %d %d \n", frame->width, frame->height);
               frame->fontW = fntW;
               frame->fontH = ycharpixels;
               frame->ascent = fntH;
               frame->descent = fntD;
               frame->prtFontW = fntW;
               frame->prtFontH = ycharpixels;
               frame->prtAscent = fntH;
               frame->prtDescent = fntD;
               frame->font = NULL;
               if (frame->isClosed == 0 && k != 0) {
                   set_frame_font(frame, AxisNumFontName);
                   set_frame_font(frame, DefaultFontName);
               }
           }
      }
      main_frame_info->width = w;
      main_frame_info->height = h;
      main_frame_info->font = NULL;
      set_frame_font(main_frame_info, AxisNumFontName);
      set_frame_font(main_frame_info, DefaultFontName);
      inRsizesMode = 0;

      setPrintScrnMode(1);
      setVjGUiPort(0); // turn off graphics socket port
      setVjUiPort(0);
      setVjPrintMode(1);
      curPalette = 0;
      transCount = 0;
      Wturnoff_message();

      set_aip_win_size(0, 0, w, h);
      set_win_size(0, 0, w, h);
      setdisplay();
      isWinRedraw = 1;
      initWinRedraw = 1;

      reds = get_red_colors();
      greens = get_green_colors();
      blues = get_blue_colors();
      psReds = get_ps_red_colors();
      psGreens = get_ps_green_colors();
      psBlues = get_ps_blue_colors();
      if (MAX_COLOR > 256)
         n = 256;
      else
         n = MAX_COLOR;
      for (k = 0; k < n; k++)
         fprintf(iplotFd, "%s %d 4 %d %d %d %d\n", iFUNC, JRGB, k, psReds[k],psGreens[k],psBlues[k]);
      for (k = n; k <= MAX_COLOR; k++) {
         if (psReds[k] == 1 && psGreens[k] == 1 && psBlues[k] == 1)
            fprintf(iplotFd, "%s %d 4 %d %d %d %d\n", iFUNC, JRGB, k, reds[k],greens[k],blues[k]);
         else
            fprintf(iplotFd, "%s %d 4 %d %d %d %d\n", iFUNC, JRGB, k, psReds[k],psGreens[k],psBlues[k]);
      }
      j4p_func(VBG_WIN_GEOM, 0, 0, mnumxpnts, mnumypnts);

      repaint_frames(1, 0);

      /***********
      if (xdebug > 1) {
          set_ybar_style(1);
          set_ybar_style(2);
          set_ybar_style(3);
      }
      ***********/
      fprintf(iplotFd, "#endplot\n");
      if (iplotTopFd != NULL)
          fprintf(iplotTopFd, "#endplot\n");

      iplotFd = NULL;
      iplotTopFd = NULL;
#endif
}

Pixel search_pixel(int r, int g, int b)
{
#ifdef MOTIF
	Pixel pix;
	XColor xc;
	int   n, k, df, dk, gray;
	int   d1, d2, d3, x1, x2, x3;

	if (!useXFunc) {
            return (1);
        }
      	xc.red = r << 8;
	xc.green = g << 8;
	xc.blue = b << 8;
	xc.pixel = 125;
        xc.flags = DoRed | DoGreen | DoBlue;;
        if (XAllocColor(xdisplay, xcolormap, &xc)) {
            return (xc.pixel);
        }
	if (r == g && r == b)
	    gray = 1;
	else
	    gray = 0;
	df = 0;
	k = 0;
	pix = 9999;
	x1 = 255; x2 = 255; x3 = 255;
	dk = 999;
	for (k = 0; k < 30; k++) 
	{
	    for (n = 0; n < color_num; n++) 
	    {
	        d1 = abs(r - vnmr_colors[n].red);
	        if (d1 <= df) {
		     d2 = abs(g - vnmr_colors[n].green);
		     if (d2 <= df) {
			d3 = abs(b - vnmr_colors[n].blue);
			if (d3 <= df) {
			    if (gray) {
			    	if (d1 < x1 && d2 < x2 && d3 < x3) {
				    x1 = d1; x2 = d2; x3 = d3;
				    pix = n;
				}
			    }
			    else {
				x1 = d1 + d2 + d3;
				if (x1 < dk) {
				    dk = x1;
				    pix = n;
				}
			    }
			}
		    }
		}
	    }
	    if (pix != 9999)
	    {
	        return (pix);
	    }
	    df += 5;
	}
#endif
	return (1);
}

static void 
set_palette_size(int n) {
    palette_t *newList;
    palette_t *oldEntry;
    palette_t *newEntry;
    int i;

    if (n < 0)
        n = 0;
    if (paletteSize >= n)
       return;
    newList = (palette_t *)malloc(sizeof(palette_t) * n);
    if (newList == NULL)
       return;
 
    if (paletteList != NULL) {
        for (i = 0; i < paletteSize; i++) {
            oldEntry = &paletteList[i];
            newEntry = &newList[i];
            newEntry->id = i;
            strcpy(newEntry->label, oldEntry->label);
            newEntry->firstColor = oldEntry->firstColor; 
            newEntry->numColors = oldEntry->numColors; 
            newEntry->zeroColor = oldEntry->zeroColor; 
            newEntry->mtime = oldEntry->mtime; 
            newEntry->names = oldEntry->names; 
            newEntry->mapName = oldEntry->mapName; 
            newEntry->fileName = oldEntry->fileName; 
         }
         free(paletteList);
     }
     for (i = paletteSize; i < n; i++) {
         newEntry = &newList[i];
         newEntry->id = i;
         strcpy(newEntry->label, "");
         newEntry->firstColor = 0;
         newEntry->numColors = 0;
         newEntry->zeroColor = 0;
         newEntry->mtime = 0;
         newEntry->names = NULL;
         newEntry->mapName = NULL;
         newEntry->fileName = NULL;
     }

     paletteSize = n;
     paletteList = newList;
     aipSetGraphicsPalettes(paletteList);
}

static void vj_open_color_palette(palette_t *entry) {
    if (entry->mapName == NULL)
       return;
    sprintf(tmpStr, "%s,", entry->mapName);
    if (entry->fileName != NULL)
        strcat(tmpStr, entry->fileName);
    else
        strcat(tmpStr, "NONE");
    aip_dstring(1, OPENCOLORMAP, entry->id, entry->firstColor, entry->numColors,
        0, tmpStr);
}

static void vj_set_color_palette(palette_t *entry, int id) {
    aip_j4p_func(SETCOLORMAP, entry->id, id, 0, 0);
}

static int load_color_palette(char *name) {
    int i, k, appendName;
    struct   stat f_stat;
    FILE  *fd;
    char  data[12];
    char  *p;
    palette_t *emptyEntry;
    palette_t *entry;

    if (paletteList == NULL) {
        init_color_palette();
        if (paletteList == NULL)
            return (-1);
    }
    if (name == NULL)
        return (-1);
    emptyEntry = NULL;
    entry = NULL;
    i = lastPalette - 4;
    if (i < GRAYSCALE_COLORS)
        i = GRAYSCALE_COLORS;
    k = i;
    while (i < paletteSize) {  // search from recently opened map
        entry = &paletteList[i];
        if (entry->mapName == NULL) {
            emptyEntry = entry;
            entry = NULL;
            break;
        }
        if (strcmp(name, entry->mapName) == 0)
            break;
        if (entry->fileName != NULL) {
            if (strcmp(name, entry->fileName) == 0)
                break;
        }
        entry = NULL;
        i++;
    }
    if (entry == NULL) { // search from the beginning
        i = GRAYSCALE_COLORS;
        while (i < k) {
            entry = &paletteList[i];
            if (entry->mapName != NULL) {
                if (strcmp(name, entry->mapName) == 0)
                   break;
                if (entry->fileName != NULL) {
                   if (strcmp(name, entry->fileName) == 0)
                       break;
                }
            }
            entry = NULL;
            i++;
        }
    }
    if (entry == NULL) {  // map was not opened before
        if (emptyEntry != NULL)
            entry = emptyEntry;
        else { 
            i = lastPalette +1;
            if (i >= paletteSize)
                i = GRAYSCALE_COLORS + 5;
            entry = &paletteList[i];
            lastPalette = i;
        }
        if (entry->mapName != NULL) {
            free(entry->mapName);
            entry->mapName = NULL;
        }
        if (entry->fileName != NULL) {
            free(entry->fileName);
            entry->fileName = NULL;
        }
    }
    if (entry->mapName != NULL && entry->fileName != NULL) {
        if (stat(entry->fileName, &f_stat) >= 0) {
            if (entry->mtime == (long) f_stat.st_mtime) {
                return (entry->id);
            }
        }
    }

    appendName = 0;
    strcpy(tmpStr, name);
    fd = fopen(tmpStr, "r");
    if (fd == NULL) {
        if (strstr(name, MAPNAME) == NULL) {
           appendName = 1;
           sprintf(tmpStr, "%s/%s", name, MAPNAME);
           fd = fopen(tmpStr, "r");
        }
    }
#ifndef INTERACT
    if (fd == NULL) {
        if (appendName)
            sprintf(tmpStr, "%s/templates/colormap/%s/%s", userdir, name, MAPNAME);
        else
            sprintf(tmpStr, "%s/templates/colormap/%s", userdir, name);
        fd = fopen(tmpStr, "r");
        if (fd == NULL) {
            if (appendName)
               sprintf(tmpStr, "%s/templates/colormap/%s/%s", systemdir, name, MAPNAME);
            else
               sprintf(tmpStr, "%s/templates/colormap/%s", systemdir, name);
            fd = fopen(tmpStr, "r");
        } 
    }
#endif
    if (fd == NULL) {
        if (strcmp(name, defaultMapDir) != 0) {
           Werrprintf("Could not load colormap \'%s\'.", name);
           return (-1);
        }
    }

    if (entry->mapName == NULL) {
        entry->mapName = malloc(strlen(name) + 2);
        strcpy(entry->mapName, name);
    }
    entry->firstColor = 0;
    entry->numColors = 66;
    entry->zeroColor = 0;
    entry->mtime = 0;

    if (entry->fileName != NULL) {
        if (strcmp(entry->fileName, tmpStr) != 0) {
           free(entry->fileName);
           entry->fileName = NULL;
        }
    }
    if (fd == NULL)
        return (entry->id);
    
    if (entry->fileName == NULL) {
        entry->fileName = (char *) malloc(strlen(tmpStr) + 2);
        strcpy(entry->fileName, tmpStr);
    }
    while ((p = fgets(tmpStr,MAXPATH, fd)) != NULL) {
        if (tmpStr[0] == '#')
           continue;
        if (sscanf(p,"%s%[^\n\r]s",gmode, data) != 2)
           continue;
        if (strcmp(gmode, "size") == 0) {
           entry->numColors = atoi(data) + 2;
           break;
        }
    }
    
    fclose(fd);
    if (stat(entry->fileName, &f_stat) >= 0)
        entry->mtime = (long) f_stat.st_mtime;
    vj_open_color_palette(entry);
    return (entry->id);
}


static void
init_color_palette()
{
    palette_t *entry, *newEntry;
    int i;

    if (paletteSize > (GRAYSCALE_COLORS+20))
       return;
    set_palette_size(GRAYSCALE_COLORS+80);
    if (paletteList == NULL)
       return;
    entry = &paletteList[0];
      strcpy(entry->label, "Named Colors");
      entry->mapName = (char *) malloc(14);
      strcpy(entry->mapName, entry->label);
      entry->id = 0;
      entry->firstColor = BLACK;
      entry->numColors = WHITE + 1;
      entry->zeroColor = -1;
      entry->names = namedColors;

    entry = &paletteList[1];
      strcpy(entry->label, "Coded Colors");
      entry->mapName = (char *) malloc(14);
      strcpy(entry->mapName, entry->label);
      entry->id = 1;
      entry->firstColor = BG_IMAGE_COLOR;
      entry->numColors = FG_IMAGE_COLOR - BG_IMAGE_COLOR + 1;
      entry->zeroColor = -1;
      entry->names = codedColors;

    entry = &paletteList[2];
      strcpy(entry->label, "Absval Data Colors");
      entry->mapName = (char *) malloc(20);
      strcpy(entry->mapName, entry->label);
      entry->id = 2;
      entry->firstColor = FIRST_AV_COLOR;
      entry->numColors = NUM_AV_COLORS;
      entry->zeroColor = FIRST_AV_COLOR;
      entry->names = NULL;

    entry = &paletteList[3];
      strcpy(entry->label, "Signed Data Colors");
      entry->mapName = (char *) malloc(20);
      strcpy(entry->mapName, entry->label);
      entry->id = 3;
      entry->firstColor = FIRST_PH_COLOR;
      entry->numColors = NUM_PH_COLORS;
      entry->zeroColor = ZERO_PHASE_LEVEL;
      entry->names = NULL;

    entry = &paletteList[4];
      // entry->id = GRAYSCALE_COLORS;
      entry->id = 4;
      entry->mtime = 0;
      strcpy(entry->label, "Grayscale Colors");
      entry->mapName = (char *) malloc(20);
      strcpy(entry->mapName, entry->label);
      entry->firstColor = FIRST_GRAY_COLOR;
      entry->numColors = NUM_GRAY_COLORS;
      entry->zeroColor = FIRST_GRAY_COLOR;
      entry->names = NULL;

    entry = &paletteList[5];
      entry->id = 5;
      entry->mtime = 0;
      entry->label[0] = '\0';
      entry->mapName = (char *) malloc(4);
      strcpy(entry->mapName, " ");
      entry->id = GRAYSCALE_COLORS+1;

    aip_initGrayscaleCMS(GRAYSCALE_COLORS);

    i = open_color_palette(defaultMapDir);
    if (i <= 4)
        return;
    entry = &paletteList[4]; // GRAYSCALE_COLORS
    newEntry = &paletteList[i];
    strcpy(entry->mapName, defaultMapDir);
    entry->firstColor = newEntry->firstColor;
    entry->numColors = newEntry->numColors;
    entry->mtime = newEntry->mtime;
    if (newEntry->fileName != NULL) {
        if (entry->fileName != NULL)
            free(entry->fileName);
        entry->fileName = (char *) malloc(strlen(newEntry->fileName) + 2);
        strcpy(entry->fileName, newEntry->fileName);
    }

    vj_open_color_palette(entry);
    vj_set_color_palette(entry, 0);
}


void
setup_aip()
{
    init_color_palette();
    aip_xid = org_xid;
    aip_gc = vnmr_gc;
    aipSetGraphics(xdisplay, screen, xid, vnmr_gc, pxm_gc,
                   sun_colors, paletteList, show_color);
}


void
sunColor_sun_window()
{ 
#ifdef MOTIF
        int     n, k;
	unsigned long mask;
	XGCValues gcValue;
        Visual *vis, *dvis;
/*
        XSetWindowAttributes   attrs;
 */
	XWindowAttributes      win_attributes;
	Pixmap   newPixmap;
#endif

#ifdef INTERACT
    BACK = BLACK;
#else 
    BACK = FIRST_AV_COLOR;
#endif 

    x_cross_pos = 0;
    y_cross_pos = 0;

    if (!useXFunc) {
        getGraphicsCmd();
	if (!is_Open) {
            init_colors();
            make_table();
            is_Open = 1;
	}
        dynamic_color = 1;
	set_window_dim();
        orgx = 0;
        orgy = 0;
        org_orgx = 0;
        org_orgy = 0;
        if (plot) {
           xcharpixels = char_width;
           ycharpixels = char_height;
        }
        if (strcmp(gmode, "dconi") == 0 || is_dconi) {
/*
	   mnumxpnts -= 4;
	   right_edge = mnumxpnts / 80 + xcharpixels * 3 + 4; 
*/
	   //right_edge = mnumxpnts / 80 + xcharpixels * 3; 
	   right_edge = mnumxpnts / 80 + xcharpixels * 1; 
        }
        else
	   right_edge  = 0;
	//   right_edge  = 46;
        // j1p_func(JBGCOLOR, BACK);
        newCursor = 1;
	return;
    }
#ifdef MOTIF
    if (xid <= 0)
	return;
    XSynchronize(xdisplay, 0);

    if (!is_Open && vnmr_gc == NULL)   /* only open window once */
    {
        vnmr_gc = XCreateGC(xdisplay, xid, 0, 0);
        org_gc = vnmr_gc;
	if (pxm_gc == NULL)
            pxm_gc = XCreateGC(xdisplay, xid, 0, 0);
	if (pxm2_gc == NULL)
            pxm2_gc = XCreateGC(xdisplay, xid, 0, 0);
	
	mask = GCLineStyle | GCCapStyle | GCJoinStyle | GCLineWidth;
	XGetGCValues(xdisplay, vnmr_gc, mask, &gcValue);
	def_lineStyle = gcValue.line_style;
	def_capStyle = gcValue.cap_style;
	def_joinStyle = gcValue.join_style;
	def_lineWidth = gcValue.line_width;
	grid_gc = XCreateGC(xdisplay, xid, 0, 0);
	screen = XDefaultScreen(xdisplay);
	scrnWidth = DisplayWidth (xdisplay, screen);
        scrnHeight = DisplayHeight (xdisplay, screen);
   
	if (DoesBackingStore(DefaultScreenOfDisplay(xdisplay)) == NotUseful)
              backing_store = 0;
/*
        else
        {
              attrs.backing_store = Always;
              XChangeWindowAttributes(xdisplay, xid, CWBackingStore, &attrs);
              backing_store = 1;
        }
*/
	init_screen_plane_list(xdisplay);
        dynamic_color = 0;
        n_gplanes = XDisplayPlanes(xdisplay, screen);
#ifdef INTERACT
        xcolormap = XDefaultColormap(xdisplay, screen);
#else 
        vis = dvis = XDefaultVisual (xdisplay, screen);
	if (XGetWindowAttributes(xdisplay,  xid, &win_attributes))
	{
	   vis = win_attributes.visual;
	   n_gplanes = win_attributes.depth;
	   xcolormap = win_attributes.colormap;
	}
	if (xcolormap == 0)
	{
           xcolormap = XDefaultColormap(xdisplay, screen);
	   vis = dvis;
	}

	color_num = DisplayCells(xdisplay, screen);
	if (color_num > COLORSIZE)
	   color_num = COLORSIZE;
	if (vis->class == TrueColor && n_gplanes > 8)
           isTrueColor = 1;
        if (vis->class == PseudoColor || vis->class == DirectColor || vis->class == GrayScale)
        {
            n = XAllocColorCells(xdisplay, xcolormap, FALSE, NULL, 0,
                                         sun_colors, CMS_VNMRSIZE);
            if (n)
                 dynamic_color = 1;
        }
#endif 
        if (x_font)
	{
                XSetFont(xdisplay, vnmr_gc, x_font);
                XSetFont(xdisplay, pxm_gc, x_font);
                xstruct = (XFontStruct *) XQueryFont(xdisplay, x_font);
        	char_ascent = xstruct->max_bounds.ascent;
    		char_descent = xstruct->max_bounds.descent;
		org_xstruct = xstruct;
	}
        if (!dynamic_color)
        {
              for(n=0; n < CMS_VNMRSIZE; n++)
                        sun_colors[n] = 0;
        }
        init_colors();
        make_table();

        g_color = CYAN;
        if (n_gplanes > 6 && !dynamic_color) 
	{
#ifdef INTERACT
           for(n=0; n < 17; n++)
#else 
           for(n=0; n < CMS_VNMRSIZE; n++)
#endif 
	   {
		XAllocColor(xdisplay, xcolormap, &vnmr_colors[n]);
                sun_colors[n] = vnmr_colors[n].pixel;
           }
           for(n=0; n < color_num; n++)
		vnmr_colors[n].pixel = n;
	   XQueryColors(xdisplay, xcolormap, vnmr_colors, color_num);
           for(n = 0; n < color_num; n++)
	   {
		vnmr_colors[n].red = vnmr_colors[n].red >> 8;
		vnmr_colors[n].green = vnmr_colors[n].green >> 8;
		vnmr_colors[n].blue = vnmr_colors[n].blue >> 8;
	   }
	}
	auto_switch_graphics_font();
    } /* end of is_Open */

    if (dynamic_color) {
         XStoreColors(xdisplay, xcolormap, vnmr_colors, CMS_VNMRSIZE);
    }
    set_window_dim();
    xblack = sun_colors[BACK];
    xwhite = sun_colors[WHITE];
    textColor = sun_colors[BLUE];
    XSetBackground(xdisplay, vnmr_gc, xblack);
    XSetForeground(xdisplay, vnmr_gc, xblack);
    XSetForeground(xdisplay, pxm_gc, xblack);
    XSetWindowBackground(xdisplay, xid, xblack);
    right_edge  = 46;
    xcharpixels = char_width;
    ycharpixels = char_height;
    if (old_winWidth < win_width)
       old_winWidth = win_width;
    if (old_winHeight < win_height)
       old_winHeight = win_height;

    if (n_gplanes > 0 && win_height > 0 && win_width > 0)
    {
        if(backing_height < win_height || backing_width < win_width)
        {
		n = backing_width;
		k = backing_height;
		if (backing_height < win_height)
                    backing_height = win_height;
                if (backing_width < win_width)
                    backing_width = win_width;
                
                canvasPixmap = org_Pixmap;
                newPixmap = XCreatePixmap(xdisplay, xid, backing_width + 2,
                    		backing_height, n_gplanes);
		if (newPixmap == 0) {
		    backing_width = 0;
		    backing_height = 0;
		}
		else {
/*
		    if (winDraw) {
			   if (xregion != NULL)
             		     XFillRectangle(xdisplay, xid, vnmr_gc, orgx, orgy, 
				win_width, win_height);
		    }
*/
             	    XFillRectangle(xdisplay, newPixmap, pxm_gc, 0, 0, 
				backing_width, backing_height);
		    if (canvasPixmap > 4)
              	        XCopyArea(xdisplay,canvasPixmap,newPixmap,pxm_gc, 0,0,
				n, k, 0, 0);
		}
                if (canvasPixmap != 0)
                    XFreePixmap(xdisplay, canvasPixmap);
		canvasPixmap = newPixmap;
		org_Pixmap = newPixmap;
		if (canvasPixmap2 != 0) {
                    XFreePixmap(xdisplay, canvasPixmap2);
                    canvasPixmap2 = 0;
                }
                canvasPixmap2 = XCreatePixmap(xdisplay, xid, backing_width + 2,
                                backing_height, n_gplanes);
		/* if no memory for canvasPixmap, then it doesn't make sense
                   to allocate overlayMap */
                if (canvasPixmap) {
                    if (overlayMap) {
                       XFreePixmap(xdisplay, overlayMap);
                       overlayMap = XCreatePixmap(xdisplay, xid, backing_width + 2,
                                backing_height, n_gplanes);
                       if (overlayMap)
                          clearOverlayMap = 1;
                    }
                }
         }
    }
#ifndef INTERACT
    if (grid_num > 1)
    {
        drawGridLines();
    }
    auto_switch_graphics_font();
#endif 
    if (canvasPixmap != 0 && bksflag)
        xmapDraw = 1;
    else
        xmapDraw = 0;
    newCursor = 1; /* if 1 use dconi cursors */
    is_Open = 1;
#endif  /*  MOTIF */
}

void default_sun_window()
{
#ifdef MOTIF
/*
    XSetWindowAttributes   attrs;
 */

    dynamic_color = 0;
#ifdef INTERACT
    BACK = BLACK;
#endif 
    if (!useXFunc) {
	if (!is_Open) {
            init_colors();
            is_Open = 1;
	}
        dynamic_color = 1;
	set_window_dim();
        orgx = 0;
        orgy = 0;
        org_orgx = 0;
        org_orgy = 0;
	right_edge  = 46;
        xcharpixels = char_width;
        ycharpixels = char_height;
        j1p_func(JBGCOLOR, BACK);
        if (csi_opened)
            aip_j1p_func(JBGCOLOR, BACK);
	return;
    }
    if (!is_Open)   /* only open window once */
    {
        if (vnmr_gc) {
            XFreeGC(xdisplay, vnmr_gc);
        }
        vnmr_gc = XCreateGC(xdisplay, xid, 0, 0);
        org_gc = vnmr_gc;
        pxm_gc = XCreateGC(xdisplay, xid, 0, 0);
        pxm2_gc = XCreateGC(xdisplay, xid, 0, 0);
	grid_gc = XCreateGC(xdisplay, xid, 0, 0);
	screen = XDefaultScreen(xdisplay);
	init_screen_plane_list(xdisplay);
        n_gplanes = XDisplayPlanes(xdisplay, screen);
	if (DoesBackingStore(DefaultScreenOfDisplay(xdisplay)) == NotUseful)
              backing_store = 0;
/*
        else
        {
              attrs.backing_store = Always;
              XChangeWindowAttributes(xdisplay, xid, CWBackingStore, &attrs);
              backing_store = 1;
        }
*/
        if (x_font)
	{
              XSetFont(xdisplay, vnmr_gc, x_font);
              XSetFont(xdisplay, pxm_gc, x_font);
              XSetFont(xdisplay, pxm2_gc, x_font);
              xstruct = (XFontStruct *) XQueryFont(xdisplay, x_font);
              char_ascent = xstruct->max_bounds.ascent;
	      org_xstruct = xstruct;
	}
        xblack = XBlackPixel(xdisplay, screen);
        xwhite = XWhitePixel(xdisplay, screen);
        textColor = xblack;
        g_color = xblack;
        is_Open = 1;
	pix_planes = check_get_plane_num(n_gplanes);
    }
    XSetWindowBackground(xdisplay, xid, xblack);
    XSetForeground(xdisplay, vnmr_gc, xwhite);
    XSetBackground(xdisplay, vnmr_gc, xblack);
    XSetBackground(xdisplay, pxm_gc, xblack);
    XSetBackground(xdisplay, pxm2_gc, xblack);
    set_window_dim();
    xcharpixels = char_width;
    ycharpixels = char_height;
    if (old_winWidth < win_width)
       old_winWidth = win_width;
    if (old_winHeight < win_height)
       old_winHeight = win_height;
    right_edge  = 46;
#ifndef INTERACT
    if (!backing_store)
    {
#endif 
      if (pix_planes > 0)
      {
        if(backing_height != win_height || backing_width != win_width)
        {
/*
             	XFillRectangle(xdisplay, xid, vnmr_gc, orgx, orgy, 
			win_width, win_height);
*/
		if (backing_height < win_height)
                    backing_height = win_height;
                if (backing_width < win_width)
                    backing_width = win_width;
		canvasPixmap = org_Pixmap;
                if (canvasPixmap)
                {
                        XFreePixmap(xdisplay, canvasPixmap);
                        canvasPixmap = 0;
                }
                canvasPixmap = XCreatePixmap(xdisplay, xid, backing_width+2,
                        backing_height, pix_planes);
		org_Pixmap = canvasPixmap;
		if (canvasPixmap == 0) {
		    backing_width = 0;
		    backing_height = 0;
		}
		else {
             	    XFillRectangle(xdisplay, canvasPixmap, pxm_gc, 0, 0, 
			backing_width, backing_height);
		}

		if (backing_height < win_height)
                    backing_height = win_height;
                if (backing_width < win_width)
                    backing_width = win_width;
                if (canvasPixmap2)
                {
                        XFreePixmap(xdisplay, canvasPixmap2);
                        canvasPixmap2 = 0;
                }
                canvasPixmap2 = XCreatePixmap(xdisplay, xid, backing_width+2,
                        backing_height, pix_planes);
		if (canvasPixmap2 == 0) {
		    backing_width = 0;
		    backing_height = 0;
		}
		else {
             	    XFillRectangle(xdisplay, canvasPixmap2, pxm2_gc, 0, 0, 
			backing_width, backing_height);
		}
		if (canvasPixmap) {
                    if (overlayMap) {
                       XFreePixmap(xdisplay, overlayMap);
                       overlayMap = XCreatePixmap(xdisplay, xid, backing_width + 2,
                                backing_height, n_gplanes);
                       if (overlayMap)
                           clearOverlayMap = 1;
                    }
                }
         }
      }
#ifndef INTERACT
    }
#endif 
#ifndef INTERACT
    if (grid_num > 1)
	drawGridLines();
    
    auto_switch_graphics_font();
#endif 
    if (canvasPixmap != 0 && bksflag)
        xmapDraw = 1;
    else
        xmapDraw = 0;
#endif  /*  MOTIF */
}

void make_table()
{    int i,k;
     int red,grn,blu;
     float  f1, f2;
     jtabcolor *jtable;

	jtcolor.subtype = htonl(JCOLORTABLE);
	jtcolor.tabsize = htonl(CMS_VNMRSIZE);
	jtcolor.taboffset = htonl(0);
        jtable = jtcolor.table;
        k = FIRST_GRAY_COLOR;
#ifdef MOTIF 
        for (i=0; i < COLORSIZE; i++)
           vnmr_colors[i].flags = DoRed | DoGreen | DoBlue;;
#endif
        for (i=0; i < k; i++)
        {
           get_color(i, &red, &grn, &blu);
	   if (!useXFunc)
	   {
	      jtable[i].red = htonl(red);
	      jtable[i].grn = htonl(grn);
	      jtable[i].blu = htonl(blu);
	   }
	   else {
#ifdef MOTIF 
           vnmr_colors[i].red = red << 8;
           vnmr_colors[i].green = grn << 8;
           vnmr_colors[i].blue = blu << 8;
           vnmr_colors[i].pixel = sun_colors[i];
#endif
	   }
        }
	f1 = 255.0 / NUM_GRAY_COLORS;
	f2 = 0;
	i = FIRST_GRAY_COLOR;
	k = FIRST_GRAY_COLOR + NUM_GRAY_COLORS;
	if (k > CMS_VNMRSIZE)
	   k = CMS_VNMRSIZE;
        while (i < k)
        {
	   red = (int) f2;
	   f2 += f1;
	   if (!useXFunc)
	   {
	      jtable[i].red = htonl(red);
	      jtable[i].grn = htonl(red);
	      jtable[i].blu = htonl(red);
           }
	   else {
#ifdef MOTIF 
	   blu = red << 8;
           vnmr_colors[i].red = blu;
           vnmr_colors[i].green = blu;
           vnmr_colors[i].blue = blu;
           vnmr_colors[i].pixel = sun_colors[i];
#endif
	   }
	   i++;
        }
        get_color(CURSOR_COLOR, &red, &grn, &blu);
	if (!useXFunc) {
           for (i=k; i < CMS_VNMRSIZE; i++)
           {
	      jtable[i].red = htonl(red);
	      jtable[i].grn = htonl(grn);
	      jtable[i].blu = htonl(blu);
           }
	   graphToVnmrJ(&jtcolor, sizeof(jtcolor));
        }
	else {
#ifdef MOTIF 
           for (i=k; i < COLORSIZE; i++)
           {
              vnmr_colors[i].red = red << 8;
              vnmr_colors[i].green = grn << 8;
              vnmr_colors[i].blue = blu << 8;
              vnmr_colors[i].pixel = sun_colors[i];
           }
#endif
        }
}


char *
sendGrayColorsToVj()
{
    static char *filepfx = "/tmp/VjGrayRampFile";
/**
    static int index = 0;
    int i;
    FILE *fd;
    char path1[MAXPATHL];
    char path2[MAXPATHL];
    char *parname = "aipGrayFilename";

#ifdef MOTIF
    sprintf(path1, "%s%d.%d", filepfx, (int)getpid(), index);
    unlink(path1);
    sprintf(path2, "%s%d.%d", filepfx, (int)getpid(), ++index);
    if ((fd = fopen(path2, "w")) != NULL) {
        for (i=FIRST_GRAY_COLOR; i<FIRST_GRAY_COLOR+NUM_GRAY_COLORS; ++i) {
            fprintf(fd,"%d %d %d\n",
                    vnmr_colors[i].red,
                    vnmr_colors[i].green,
                    vnmr_colors[i].blue);
        }
        fclose(fd);
        P_setstring(GLOBAL, parname , path2, 1);
        appendvarlist(parname);
    }
#endif
**/
    return filepfx;
}

void
refresh_graf()
{
/*  macX needs this function to reflect the action of changing color  */
  if (!useXFunc) {
	jvnmr_cmd(JREFRESH);
	return;
  }
#ifdef MOTIF
/*  if (Wissun())
     XCopyArea(xdisplay,xid,xid,vnmr_gc, 0, 0, graf_width, graf_height, 0, 0);
*/
#endif 
}

/***************************/
int sunColor_change_contrast(double a, double b)
/***************************/
{ register int i,k, m, jred, jgrn, jblu;
  register double temp_red,temp_grn,temp_blu;
  register double slope_red,slope_grn,slope_blu;
  int fg_red, fg_grn, fg_blu;
  int bg_red, bg_grn, bg_blu;
  register int maxred,minred,maxgrn,mingrn,maxblu,minblu;
  jtabcolor *jtable = NULL;
  /* a is center and b is gain  NUM_GRAY_COLORS/2,1.0 is standard */
  /*  if colormap can't be changed  */
/*  if(!dynamic_color)
 *        return;
 */

  aipGrayMap = 0;
  
  if (curPalette > 0) {
      curPalette = 0;
      curTransparence = -1;
      aip_j4p_func(SETCOLORMAP, 0, 0, 0, 0);
  }
  
  get_color(BG_IMAGE_COLOR, &bg_red, &bg_grn, &bg_blu);
  get_color(FG_IMAGE_COLOR, &fg_red, &fg_grn, &fg_blu);
  maxred = (bg_red > fg_red) ? bg_red : fg_red;
  minred = (bg_red > fg_red) ? fg_red : bg_red;
  maxgrn = (bg_grn > fg_grn) ? bg_grn : fg_grn;
  mingrn = (bg_grn > fg_grn) ? fg_grn : bg_grn;
  maxblu = (bg_blu > fg_blu) ? bg_blu : fg_blu;
  minblu = (bg_blu > fg_blu) ? fg_blu : bg_blu;
 
  i = FIRST_GRAY_COLOR;
  k = i + NUM_GRAY_COLORS;
  slope_red = ((double) fg_red - (double) bg_red)*b/((double) NUM_GRAY_COLORS-1);
  slope_grn = ((double) fg_grn - (double) bg_grn)*b/((double) NUM_GRAY_COLORS-1);
  slope_blu = ((double) fg_blu - (double) bg_blu)*b/((double) NUM_GRAY_COLORS-1);
  temp_red = (double)(fg_red - bg_red)/2.0 - slope_red*a;
  temp_grn = (double)(fg_grn - bg_grn)/2.0 - slope_grn*a;
  temp_blu = (double)(fg_blu - bg_blu)/2.0 - slope_blu*a;
  if(!useXFunc) {
      dcon_thTop = FIRST_GRAY_COLOR;
      dcon_thBot = 0;
      org_dcon_thTop = FIRST_GRAY_COLOR;
      org_dcon_thBot = 0;
      jtcolor.subtype = htonl(JCOLORTABLE);
      jtcolor.tabsize = htonl(NUM_GRAY_COLORS);
      jtcolor.taboffset = htonl(FIRST_GRAY_COLOR);
      jtable = jtcolor.table;
  }
  while (i<k)
    {
      jred = temp_red;
      if (jred<minred) jred=minred;
      else if (jred>maxred) jred=maxred;
      jgrn = temp_grn;
      if (jgrn<mingrn) jgrn=mingrn;
      else if (jgrn>maxgrn) jgrn=maxgrn;
      jblu = temp_blu;
      if (jblu<minblu) jblu=minblu;
      else if (jblu>maxblu) jblu=maxblu;
      if(useXFunc) {
#ifdef MOTIF 
	  if (dynamic_color) {
      	  	vnmr_colors[i].red = jred << 8;
		vnmr_colors[i].green = jgrn << 8;
		vnmr_colors[i].blue = jblu << 8;
		vnmr_colors[i].pixel = sun_colors[i];
	  }
	  else {
                sun_colors[i] = search_pixel(jred, jgrn, jblu);
	  }
#endif
      }
      else {
	  m = i - FIRST_GRAY_COLOR;
          jtable[m].red = htonl(jred);
          jtable[m].grn = htonl(jgrn);
          jtable[m].blu = htonl(jblu);
      }
      temp_red += slope_red;
      temp_grn += slope_grn;
      temp_blu += slope_blu;
      i++;
   }
   if (useXFunc) {
#ifdef MOTIF 
       if (dynamic_color) {
           XStoreColors(xdisplay, xcolormap,
                        &vnmr_colors[FIRST_GRAY_COLOR], NUM_GRAY_COLORS);
       }
/*
       else {
           sendGrayColorsToVj();
       }
*/
#endif
   }
   else {
	k = intLen * 4 + sizeof(jtabcolor) * NUM_GRAY_COLORS;
   	graphToVnmrJ(&jtcolor, k);
        refresh_graf();
   }
   return (dynamic_color);
}

void show_color(int num, int red, int grn, int blu)
{
    if (num < 0)
        return;
    if (useXFunc) {
#ifdef MOTIF 
	if (dynamic_color) {
    		vnmr_colors[num].red = red << 8;
		vnmr_colors[num].green = grn << 8;
		vnmr_colors[num].blue = blu << 8;
		vnmr_colors[num].pixel = sun_colors[num];
		XStoreColor(xdisplay, xcolormap, &vnmr_colors[num]);
	}
	else {
                sun_colors[num] = search_pixel(red, grn, blu);
	}
#endif
	return;
    }

    j4xp_func(JRGB, num, red, grn, blu);
    if (csi_opened)
        aip_j4p_func(JRGB, num, red, grn, blu);
    if (num == BACK) {
        j1p_func(JBGCOLOR, BACK);
        if (csi_opened)
            aip_j1p_func(JBGCOLOR, BACK);
    }
}

void set_rgb_color(int r, int g, int b)
{
    if (useXFunc) {
#ifdef MOTIF 
        xcolor = search_pixel(r, g, b);
        if (xorflag)
           xcolor = xcolor ^ sun_colors[BACK];
        XSetForeground(xdisplay, vnmr_gc, xcolor);
        XSetForeground(xdisplay, pxm_gc, xcolor);
#endif
	return;
    }
    j4xp_func(JFGCOLOR, r, g, b, 0);
    if (csi_opened)
        aip_j4p_func(JFGCOLOR, r, g, b, 0);
}

/***************************/
int sunColor_change_color(int num, int on)
/***************************/
{
    int red, grn, blu;
 
    if ((num<0) || (num>MAX_COLOR)) return 1;
    if (on)
        get_color(num, &red, &grn, &blu);
    else {
        get_color(BACK, &red, &grn, &blu);
        
    }
    show_color(num,red,grn,blu);
    return (dynamic_color);
}

void
reset_dcon_color_threshold(int top, int bot)
{
   dcon_thTop = top;
   dcon_thBot = bot;
   org_dcon_thTop = dcon_thTop;
   org_dcon_thBot = dcon_thBot;
}

void
set_color_levels(int num)
{

#ifdef MOTIF 
    int k, k2, red, grn, blu;
    XColor  xc;
    IPCOLOR *ip, *ip2;
#endif

    if (num <= 0)
	return;
    g_color_levels = num;
    if (!useXFunc)
	return;
#ifdef MOTIF 
    ip = ipcolor;
    hPix = NULL;
    max_color_level = 0;
    k = 0;
    while (ip != NULL) {
	k++;
	if (ip->color == g_color) {
	   hPix = ip->pix;
	   max_color_level = ip->levels;
	   break;
	}
	ip = ip->next;
    }
    if (k > 30 && ip == NULL) { /* too many colors */
	ip = ipcolor->next;
	ip->color = g_color;
	ip->levels = 1;
	hPix = ip->pix;
	max_color_level = ip->levels;
    }
    
    if (num > max_color_level) {
	if (hPix != NULL) {
	    free(hPix);
	    hPix = NULL;
	}
    }
    if (hPix == NULL) {
	hPix = (Pixel *) malloc(sizeof(Pixel) * (num + 1));
	if (hPix == NULL) {
	    if (ip != NULL)
		ip->pix = NULL;
	    return;
	}
    }
    if ((max_color_level != num)) {
	max_color_level = num;
	for (k = 1; k < num+1; k++) {
    	    xc.pixel = sun_colors[g_color];
   	    XQueryColor(xdisplay, xcolormap, &xc);
    	    red = xc.red >> 8;
    	    grn = xc.green >> 8;
	    blu = xc.blue >> 8;
    	    k2 = max_color_level - k + 1;
	    red = red * k2 / max_color_level; 
	    grn = grn * k2 / max_color_level; 
	    blu = blu * k2 / max_color_level; 
	    hPix[k] = search_pixel(red, grn, blu);
	}
	hPix[0] = hPix[1];
    }
    if (ip == NULL) {
	ip = (IPCOLOR *) calloc(1, sizeof(IPCOLOR));
        if (ip == NULL)
		return;
	ip->levels = num;
	ip->color = g_color;
	ip->pix = hPix;
	if (ipcolor == NULL) {
	    ipcolor = ip;
	    return;
	}
	ip2 = ipcolor;
	while (ip2->next != NULL)
	    ip2 = ip2->next;
	ip2->next = ip;
    }
#endif  /*  MOTIF */
}

void
set_color_intensity(int level)
{
#ifdef MOTIF
    Pixel   pix;
#endif

    // if(level < 0) level = 0;
    if (level < 1) level = 1;

    if (!useXFunc) {
        j4xp_func(JGRADCOLOR, g_color, g_color_levels, level, 0);
        if (csi_opened)
            aip_j4p_func(JGRADCOLOR, g_color, g_color_levels, level, 0);
	return;
    }
#ifdef MOTIF
    if (max_color_level <= 0)
	set_color_levels(12);
    if (hPix == NULL)
	return;

    pix = hPix[level];
    if (xorflag)
        pix = pix ^ sun_colors[BACK];
  
    XSetForeground(xdisplay, vnmr_gc, pix);
    XSetForeground(xdisplay, pxm_gc, pix);
    XSetForeground(xdisplay, pxm2_gc, pix);
#endif
}

/* init_rast is not in gdevsw - it should be */
void init_rast(void *src, int n, int init)
{
}

/* rast is not in gdevsw - it should be */
/*******************/
static void
x_rast(int is_aip, void *src, int pnts, int times, int x, int y,
       Pixel *pixels, Pixmap xmap)
/*******************/
{
   int 		  k;
   unsigned char  *r_data;
   static unsigned char  *jdata;
   static int  jsize = 0;
   static int  jpnts = 0;
#ifdef MOTIF
   char		  *i_data;
   unsigned long  pix;
#endif

   my1 = y;
   if (my1 < 0)
	my1 = 0;
   gy1 = my1 + orgy;
   if (times <= 0 || gy1 <= 0)
        return;
   gx1 = x + orgx;
   mx1 = x;
   if (isSuspend)
        return;
   isGraphFunc = 1;
   if (!useXFunc) {
        jBatch(1);
	if (jpnts != pnts) {
	   jsize = sizeof(int) * 6 + pnts;
	   if (jimage_size < pnts) { 
	      if (jimage_data != NULL)
	   	   free(jimage_data);
	      jimage_data = (char *) malloc(jsize+12);
	      if (jimage_data == NULL) {
	          jimage_size = 0;
		  return;
	      }
	      jimage_size = pnts;
	   }
	   jpnts = pnts;
	   jimage = (JIMAGE *) jimage_data;
	   // jimage->type = htonl(JGRAPH);
	   jimage->subtype = htonl(jRaster);
	   jimage->len = htonl(pnts+4);
   	   jdata = (unsigned char *)(jimage_data + sizeof(int) * 6);
   	}
        if (is_aip)
	   jimage->type = htonl(JGRAPH + AIPCODE);
        else
	   jimage->type = htonl(JGRAPH);
   	jimage->x = htonl(x);
   	jimage->y = htonl(gy1);
   	jimage->repeat = htonl(times);
        r_data = (unsigned char *) src;
        for(k = 0; k < pnts; k++) {
           if (*r_data < dcon_thTop && *r_data > dcon_thBot)
               *r_data = BACK; 
           r_data++;
        }
        
   	// jdata = (unsigned char *)(jimage_data + sizeof(int) * 6);
        bcopy(src, jdata, pnts);
        r_data = jdata + pnts; 
        for(k = 0; k < 4; k++) {
           *r_data = 0xee;
           r_data++;
        }
        if (iplotFd != NULL)
           iplot_rast(is_aip, src, pnts, times, x, y);
        else
   	   graphToVnmrJ(jimage_data, jsize+4);
        if (isPrintBg != 0) {
            transCount++;
            if (jsize > 600) {
               if (transCount > 5) {
                   transCount = 0;
                   usleep(10000);
               }
            }
            else {
               if (transCount > 10) {
                   transCount = 0;
                   usleep(10000);
               }
            }
        }
	return;
   }
#ifdef MOTIF
   if (rast_image == NULL)
	return;
   if (rast_image_width <= pnts)
	pnts = rast_image_width - 2;
   if (rast_do_flag > 0)
   {
        r_data = (unsigned char *) src;
        i_data = rast_image->data;
        switch (rast_do_flag) {
         case 1:  /* 8 bits */
                 for(k = 0; k < pnts; k++)
                 {
                    *i_data = pixels[*r_data++];
                    i_data++;
                 }
                 break;
         case 2:  /* 16 bits */
                 for(k = 0; k < pnts; k++)
                 {
                    pix = pixels[*r_data++];
                    i_data[0] = pix >> 8;
                    i_data[1] = pix;
                    i_data += 2;
                 }
                 break;
         case 3:  /* 24 bits */
                 for(k = 0; k < pnts; k++)
                 {
                    pix = pixels[*r_data++];
                    i_data[0] = pix >> 16;
                    i_data[1] = pix >> 8;
                    i_data[2] = pix;
                    i_data += 3;
                 }
                 break;
         case 4:  /* 32 bits */
                 for(k = 0; k < pnts; k++)
                 {
                    pix = pixels[*r_data++];
                    i_data[0] = pix >> 24;
                    i_data[1] = pix >> 16;
                    i_data[2] = pix >> 8;
                    i_data[3] = pix;
                    i_data += 4;
                 }
                 break;
         case 12:  /* LSBFirst 16 bits */
                 for(k = 0; k < pnts; k++)
                 {
                    pix = pixels[*r_data++];
                    i_data[1] = pix >> 8;
                    i_data[0] = pix;
                    i_data += 2;
                 }
                 break;
         case 13:  /* LSBFirst 24 bits */
                 for(k = 0; k < pnts; k++)
                 {
                    pix = pixels[*r_data++];
                    i_data[2] = pix >> 16;
                    i_data[1] = pix >> 8;
                    i_data[0] = pix;
                    i_data += 3;
                 }
                 break;
         case 14:  /* LSBFirst 32 bits */
                 for(k = 0; k < pnts; k++)
                 {
                    pix = pixels[*r_data++];
                    i_data[3] = pix >> 24;
                    i_data[2] = pix >> 16;
                    i_data[1] = pix >> 8;
                    i_data[0] = pix;
                    i_data += 4;
                 }
                 break;
         default:
                 for(k = 0; k < pnts; k++)
                 {
                    XPutPixel(rast_image, k, 0, pixels[*r_data]);
                    r_data++;
                 }
                 break;
        }
   }
   else  /*  XYPixmap  */
        convert_raster(rast_image, pnts, src);
// pnts--;
   if (winDraw)
        XPutImage(xdisplay, xid, vnmr_gc, rast_image, 0, 0, gx1, gy1, pnts, 1);
   if (xmap)
	XPutImage(xdisplay, xmap, pxm_gc, rast_image, 0, 0, mx1, my1, pnts, 1);
   while(times > 1)
   {
        if (gy1 > 0)
	{
	   if (winDraw)
              XCopyArea(xdisplay,xid,xid,vnmr_gc, gx1, gy1, pnts, 1, gx1, gy1-1);
	   if (xmap)
              XCopyArea(xdisplay,xmap,xmap,pxm_gc, mx1, my1, pnts, 1, mx1, my1-1);
	}
        gy1--;
        my1--;
        times--;
   }
#endif  /*  MOTIF */
}

/* rast is not in gdevsw - it should be */
/*******************/
void sun_rast(void *src, int pnts, int times, int x, int y)
/*******************/
{
     y = mnumypnts - y - 1;
     x_rast(0, src,pnts,times,x,y, sun_colors, canvasPixmap);
}

void
sun_draw_image(src, sw, sh, x, y, w,h, pixels, transColor)
unsigned char *src;
int sw, sh;  /* the width and height of source */
int x,y, w, h; /* the area to draw */
Pixel *pixels;
Pixel transColor;
{
#ifdef MOTIF
    int  row, col;
    int  dx, dy;
    int  r, k;
    int  sflag;
    float rx, ry, fp;
    unsigned char *data;
    XImage *ximg;
    int p;
#endif

    if (!useXFunc)
        return;
#ifdef MOTIF
    dy = y; 
    if (sw > w) {
	rx = (float) sw / (float) w;
	col = w;
    }
    else {
	rx = 1.0;
	col = sw;
    }
    if (sh > h) {
	ry = (float) sh / (float) h;
	row = h;
    }
    else {
	ry = 1.0;
	row = sh;
    }
    sflag = 0;
    if (canvasPixmap == 0) {
        if (xid == org_xid)
            return;
        sflag = 1;
        canvasPixmap = xid;
    }
    if (transColor >= 0 && canvasPixmap != 0) {
        dy = mnumypnts - y;
	if (dy < 0)
	    dy = 0;
	if (dy + row > graf_height)
           row = graf_height - dy;
        if (x + col > graf_width)
           col = graf_width - x;
        ximg = XGetImage(xdisplay, canvasPixmap, x, dy, col, row, AllPlanes,ZPixmap);
        if (ximg == NULL)
            ximg = XGetImage(xdisplay, canvasPixmap, x, dy, col, row, AllPlanes,XYPixmap);
        if (ximg != NULL) {
    	   for (r = 0; r < row; r++) {
	      k = (float) r * ry;
	      data = src + sw * k;
	      fp = 0;
	      for (k = 0; k < col; k++) {
		  dx = fp;
		  p = *(data + dx);
		  if (transColor != p)
		     XPutPixel(ximg, k, r, pixels[p]);
		  fp += rx;
	      }
	   }
   	   if (winDraw) {
  		k = dy + orgy;
   		dx = x + orgx;
        	XPutImage(xdisplay, xid, vnmr_gc, ximg, 0, 0, dx, k, col, row);
	   }
	   XPutImage(xdisplay, canvasPixmap, pxm_gc, ximg, 0, 0, x, dy, col, row);
	   XDestroyImage(ximg);
           if (sflag)
                canvasPixmap = 0;
	   return;
	}
    }
    if (tmp_src_len <= col) {
	tmp_src_len = col + 20;
	if (tmp_src != NULL)
	    free(tmp_src);
	tmp_src = (unsigned char *) malloc(tmp_src_len);
    }
    dy = mnumypnts - dy - 1;
    for (r = 0; r < row; r++) {
	k = (float) r * ry;
	data = src + sw * k;
        if (rx == 1.0)
	    x_rast(0, data, col, 1, x, dy, pixels, canvasPixmap);
	else {
	    fp = 0;
	    for (k = 0; k < col; k++) {
		dx = fp;
		tmp_src[k] = *(data + dx);
		fp += rx;
	    }
	    x_rast(0, tmp_src, col, 1, x, dy, pixels, canvasPixmap);
	}
	dy--;
    }
    if (sflag)
        canvasPixmap = 0;
#endif  /*  MOTIF */
}

void
clear_overlay_area(x, y, w, h)
int x, y, w, h;
{
#ifdef MOTIF

    if (!useXFunc)
        return;
    if (overlayMap == 0)
        return;
    if (isTrueColor) {
        XSetFunction(xdisplay, ovly_gc, GXclear);
        XFillRectangle(xdisplay, overlayMap, ovly_gc, x, y, w, h);
    }
    else {
        XCopyArea(xdisplay, canvasPixmap, overlayMap, ovly_gc, x,y,w,h,x,y);
    }
#endif
}

void
clear_overlay_map()
{
    overlayNum = 0;
    clearOverlayMap = 1;
}

void
draw_overlay_image(src, sw, sh, x, y, w,h, pixels, transColor, hilit)
unsigned char *src;
int sw, sh;  /* the width and height of source */
int x,y, w, h; /* the area to draw */
Pixel *pixels;
Pixel transColor;
int hilit;
{
#ifdef MOTIF
    int  row, col;
    int  dx, dy;
    int  r, k;
    float rx, ry, fp;
    unsigned char *data;
    XImage *ximg;
    int p;
#endif

    if (src == NULL)
        return;
    if (!useXFunc) {
        overlayNum = 1;
        return;
    }

#ifdef MOTIF
    if (x < 0 || y < 0)
        return;
    if (overlayMap == 0) {
        if (canvasPixmap)
           overlayMap = XCreatePixmap(xdisplay, xid, backing_width + 2,
                                backing_height, n_gplanes);
        if (overlayMap == 0) {
            overlayNum = 0;
            return;
        }
        if (ovly_gc == NULL)
            ovly_gc = XCreateGC(xdisplay, xid, 0, 0);
        clearOverlayMap = 1;
    }

    if (clearOverlayMap) {
        if (isTrueColor) {
           XSetFunction(xdisplay, ovly_gc, GXclear);
           XFillRectangle(xdisplay, overlayMap, ovly_gc, 0, 0,
                         win_width, win_height);
        }
        else {
           XSetFunction(xdisplay, ovly_gc, GXcopy);
           XCopyArea(xdisplay, canvasPixmap, overlayMap, ovly_gc,
                0, 0, backing_width, backing_height, 0, 0);
        }
        clearOverlayMap = 0;
    }
    XSetFunction(xdisplay, ovly_gc, GXcopy);
    overlayNum = 1;
    dy = y;
    rx = (float) sw / (float) w;
    col = w;
    ry = (float) sh / (float) h;
    row = h;
    if (dy < 0)
        dy = 0;
    if (dy + row > graf_height)
        row = graf_height - dy;
    if (x + col > graf_width)
        col = graf_width - x;
    ximg = XGetImage(xdisplay, overlayMap, x, dy, col, row, AllPlanes,ZPixmap);
    if (ximg == NULL) {
        ximg = XGetImage(xdisplay, overlayMap, x, dy, col, row, AllPlanes,XYPixmap);
        if (ximg == NULL)
            return;
    }
    for (r = 0; r < row; r++) {
        k = (float) r * ry;
        data = src + sw * k;
        fp = 0;
        for (k = 0; k < col; k++) {
           dx = fp;
           p = *(data + dx);
           if (transColor != p)
               XPutPixel(ximg, k, r, pixels[p]);
           fp += rx;
        }
    }
    XPutImage(xdisplay, overlayMap, ovly_gc, ximg, 0, 0, x, dy, col, row);
    XDestroyImage(ximg);
    if (hilit) {
        color(RED);
        XSetForeground(xdisplay, ovly_gc, xcolor);
        XDrawRectangle(xdisplay, overlayMap, ovly_gc, x, dy, col-1, row-1);
    }

#endif
}

void
disp_overlay_image(cleanWindow, drawCursor)
int cleanWindow, drawCursor;
{
    if (!useXFunc) {
        if (overlayNum > 0)
            jvnmr_cmd(JVIMAGE);
	if (drawCursor)
            draw_newCursor();
        return;
    }
#ifdef MOTIF
    if (org_Pixmap == 0 || winDraw == 0)
        return;
    if (cleanWindow) {
        if (isTrueColor || overlayNum <= 0)
        {
           XCopyArea(xdisplay, org_Pixmap, xid, vnmr_gc, 0, 0,win_width, win_height, orgx, orgy);
        }
    }
    if (overlayNum > 0) {
        if (isTrueColor)
            XSetFunction(xdisplay, org_gc, GXor);
        else
            XSetFunction(xdisplay, org_gc, GXcopy);
        XCopyArea(xdisplay, overlayMap, xid, org_gc, 0, 0, win_width,
                win_height, orgx, orgy);
        if (xorflag)
	    XSetFunction(xdisplay, org_gc, GXxor);
        else
	    XSetFunction(xdisplay, org_gc, GXcopy);
    }
    if (drawCursor)
        draw_newCursor();
#endif
}

void
update_overlay_image(cleanWindow, mouseRelease)
int cleanWindow, mouseRelease;
{
     if (overlayNum > 0) {
        if (!useXFunc || isTrueColor) {
           disp_overlay_image(cleanWindow, 1);
           return;
        }
        overlayNum = 0;
        repaint_overlay_image();
        if (overlayNum > 0) {
           disp_overlay_image(0, 1);
        }
     }
}


void
sun_normalmode()
{
  xorflag = 0;
  if (useXFunc) {
#ifdef MOTIF
	XSetFunction(xdisplay, vnmr_gc, GXcopy);
	XSetFunction(xdisplay, pxm_gc, GXcopy);
#endif
  }
  else {
	jvnmr_cmd(JNORMALMODE);
  }
}

void sun_xormode()
{
  xorflag = 1;
  if (useXFunc) {
#ifdef MOTIF
	XSetFunction(xdisplay, vnmr_gc, GXxor);
	XSetFunction(xdisplay, pxm_gc, GXxor);
#endif
  }
  else {
	jvnmr_cmd(JXORMODE);
  }
}

/*************************************/
void x_rast_cursor(int *old_pos, int new_pos, int off1, int off2, int id)
/*************************************/
{
  if (newCursor) {
	dconi_x_rast_cursor(old_pos,new_pos,off1,off2,id);
	return;
  }

  if (*old_pos != new_pos)
  { 
/*
    my1 = orgy;
    my2 = mnumypnts-off1;
    gy1 = orgy + 1;
    gy2 = mnumypnts-off1 + orgy;
*/
    if (!xorflag)
       xormode();
    color(CURSOR_COLOR); 
    if (*old_pos>0)
    {   /* remove old cursor */
	   xgrx = *old_pos;
	   xgry = 1;
	   sun_adraw(*old_pos, off1);
/*
	   gx1 = *old_pos + orgx;
	   mx1 = *old_pos;
	   if (winDraw)
              XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx1, gy2);
           if (canvasPixmap)
              XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx1, my2);
*/
    }
    if (new_pos>0)
    {
	   xgrx = new_pos;
	   xgry = 1;
	   sun_adraw(new_pos, off1);
/*
	   gx1 = new_pos + orgx;
	   mx1 = new_pos;
	   if (winDraw)
             XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx1, gy2);
           if (canvasPixmap)
             XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx1, my2);
*/
    }
    *old_pos = new_pos;
    normalmode();
   }
}


/*************************************/
void y_rast_cursor(int *old_pos, int new_pos, int off1, int off2, int id)
/*************************************/
{
  if (newCursor) {
	dconi_y_rast_cursor(old_pos,new_pos,off1,off2, id);
	return;
  }
  if (*old_pos != new_pos)
  { 
/*
    mx1 = 1;
    mx2 = off1 + off2;
    gx1 = orgx + 1;
    gx2 = off1 + off2 + orgx;
*/
    if (!xorflag)
        xormode();
    color(CURSOR_COLOR);
    if (*old_pos > 0)
    {   /* remove old cursor */
	xgrx = 1;
	xgry = *old_pos;
	sun_adraw(off1 + off2, *old_pos);
/*
	gy1 =  mnumypnts - *old_pos + orgy;
	my1 =  mnumypnts - *old_pos;
	if (winDraw)
            XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy1);
        if (canvasPixmap)
            XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx2, my1);
*/
    }
    if (new_pos>0)
    {
	xgrx = 1;
	xgry = new_pos;
	sun_adraw(off1 + off2, new_pos);
/*
        gy1 = mnumypnts - new_pos + orgy;
        my1 = mnumypnts - new_pos;
	if (winDraw)
            XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy1);
        if (canvasPixmap)
            XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx2, my1);
*/
    }
    *old_pos = new_pos;
    normalmode();
   }
}

void convert_raster(XImage *ximage, int width, void *data)
{
#ifdef MOTIF
        char  *p_data, *q_data;
        char  *s_data, *d_data;
        int    depth, offset, length;
        int    unit_counter, bit_counter, bit_flag;
        int    i, m;
        char  *r_data;
        int    bit_pad;

        s_data = (char *) data;
        for(i = 0; i < width; i++)
        {
                *s_data = (char)sun_colors[(int) *s_data];
                s_data++;
        }
        depth = ximage->depth;
        if (depth > 8)
            depth = 8;   /* the depth of source data is only 8 bits */
        offset = ximage->xoffset;
        r_data = ximage->data;
        length = ximage->bytes_per_line;
        bit_pad = length * 8 - width - offset;
        p_data = r_data;
        p_data--;
        d_data = p_data;
        s_data = (char *) data;
        unit_counter = 0;
        bit_counter = 0;
        for(i = 0; i < width; i++)
        {
                if (unit_counter == 0)
                {
                        if (ximage->byte_order == LSBFirst)
                        {
                                p_data = p_data + ximage->bitmap_unit / 8;
                                d_data = p_data;
                        }
                        else
                                d_data = d_data + 1;
                        bit_counter = 0;
                        unit_counter = ximage->bitmap_unit;
                }
                if (bit_counter == 8)
                {
                        bit_counter = 0;
                        if (ximage->byte_order == LSBFirst)
                                d_data = d_data - 1;
                        else
                                d_data = d_data + 1;
                }
                bit_flag = 0x80;
                for(m = 0; m < depth; m++)
                {
                        q_data = d_data + m * length;
                        if (*s_data & bit_flag)
                        {
                                if (ximage->bitmap_bit_order == MSBFirst)
                                        *q_data = (*q_data << 1) | 0x01;
                                else
                                        *q_data = (*q_data >> 1) | 0x80;
                        }
                        else
                        {
                                if (ximage->bitmap_bit_order == MSBFirst)
                                        *q_data = *q_data << 1;
                                else
                                        *q_data = (*q_data >> 1) & 0x7F;
                        }
                        bit_flag = bit_flag >> 1;
                }
                bit_counter++;
                unit_counter--;
                s_data++;
        }
#endif  /*  MOTIF */
        return;
}


void
redrawCanvas()
{
#ifdef MOTIF
	int  sh, sw;
#endif

        x_cross_pos = 0;
        y_cross_pos = 0;
        if (!useXFunc) {
	    jvnmr_cmd(JCOPY);
	    if (overlayNum > 0) {
               if (sizeChanged)
                  redraw_overlay_image();
               else
                  disp_overlay_image(0, 0);
            }
	    draw_newCursor();
            return;
	}
#ifdef MOTIF
	if (isSuspend > 0 || winDraw == 0)
	   return;
	if (win_width <= 0 || win_height <= 0)
	    return;
#ifdef INTERACT
        if (canvasPixmap)
           XCopyArea(xdisplay, canvasPixmap, xid, vnmr_gc, 0, 0, win_width,
                        win_height, orgx, orgy);
#else 
        if (canvasPixmap) {
/*
	   Wgetgraphicsdisplay(cmd, GRAPHCMDLEN);
           if ( strcmp(cmd, aipRedisplay) == 0) {
		execString("aipRedisplay\n");	      
		if (overlayNum > 0) {
                    if (sizeChanged)
                        redraw_overlay_image();
                    else
                        update_overlay_image(1, 0);
                }
		return;
	   }
*/

	   sh = win_height;
	   if (sh > backing_height)
		sh = backing_height;
	   if (win_width > backing_width)
		sw = backing_width;
	   else
		sw = win_width; 
           if (grid_num > 1)
               copy_grid_windows();
           else
               XCopyArea(xdisplay, canvasPixmap, xid, vnmr_gc, 0, 0, sw,
                        sh, orgx, orgy);

	   if (overlayNum > 0) {
                if (sizeChanged)
                   redraw_overlay_image();
                else
                   disp_overlay_image(0, 0);
           }
           if (grid_num > 1) {
              drawGridLines(); 
           }
	}
	draw_newCursor();
#endif
#endif  /*  MOTIF */
}

void
repaintCanvas()
{
	if (isSuspend > 0 || winDraw == 0)
	   return;
       
        x_cross_pos = 0;
        y_cross_pos = 0;
	keepOverlay = 1;
        if (sizeChanged)
	   window_redisplay();
	else {
           if (useXFunc)
	       redrawCanvas();
	   else
	       draw_newCursor();
        }
	keepOverlay = 0;
        Wturnon_message();
}

void
redrawCanvas2()
{
#ifdef MOTIF
        if (!useXFunc || xid2 <= 0)
	   return;
	if (win_width <= 0 || win_height <= 0)
	    return;
	XClearWindow(xdisplay, xid2);
#ifdef INTERACT
        if (canvasPixmap)
           XCopyArea(xdisplay, canvasPixmap, xid2, pxm_gc, 0, 0, win_width,
                        win_height, orgx2, orgy2);
#else 
        if (canvasPixmap) {
           XCopyArea(xdisplay, canvasPixmap, xid2, pxm_gc, 0, 0, win_width,
                        win_height, orgx2, orgy2);
	}
#endif
#endif  /*  MOTIF */
}

void
backing_up()
{
/**
     if (!useXFunc)
	   return;
     if (!batch_on)
     {
	if (canvasPixmap)
           XCopyArea(xdisplay, xid, canvasPixmap, pxm_gc, orgx, orgy,
		win_width, win_height, 0, 0);
     }
**/
}


int
textWidth(str, type)
char   *str;
int     type;
{
        if (type == 0)  /*  banner  */
        {
#ifdef MOTIF
            if (useXFunc) {
               if (structOfbannerFont)
                  return(XTextWidth(structOfbannerFont, str, strlen(str)));
               else if (xstruct != NULL)
                  return(XTextWidth(xstruct, str, strlen(str)));
	    }
#endif
	    return(xcharpixels * strlen(str) * bannerSize); 
        }
#ifdef MOTIF
        if (xstruct != NULL)
            return(XTextWidth(xstruct, str, strlen(str)));
#endif
	return(xcharpixels * strlen(str)); 
}

int
depthOfWindow()
{
#ifdef MOTIF
        if (xdisplay != NULL)
	   return(DefaultDepth(xdisplay, XDefaultScreen(xdisplay)));
#endif
	return(8);
}

int
widthOfScreen()
{
#ifdef MOTIF
        if (xdisplay != NULL)
	    return(DisplayWidth (xdisplay, DefaultScreen(xdisplay)));
#endif
	return(800);
}

int
heightOfScreen()
{
#ifdef MOTIF
        if (xdisplay != NULL)
	    return(DisplayHeight (xdisplay, DefaultScreen(xdisplay)));
#endif
	return(600);
}

void
whiteCanvas()
{
	int  i;

        for(i = 0; i < 3; i++) {
             x_cursor_pos[i] = 0;
             y_cursor_pos[i] = 0;
	}
	if (!useXFunc) {
             j1p_func(JBGCOLOR, WHITE);
             if (csi_opened)
                 aip_j1p_func(JBGCOLOR, WHITE);
	     return;
	}
#ifdef MOTIF
#ifdef INTERACT
        if (n_gplanes < 7)  /* assume it is monochrome */
	     BACK = WHITE;
#endif 
        XSetWindowBackground(xdisplay, xid, xwhite);
	if (winDraw)
             XClearArea(xdisplay, xid,orgx,orgy,graf_width,graf_height,False);
        if (canvasPixmap)
        {
             XSetForeground(xdisplay, pxm_gc, xwhite);
             XFillRectangle(xdisplay, canvasPixmap, pxm_gc, 0, 0, 
			graf_width, graf_height);
        }
        XSetForeground(xdisplay, vnmr_gc, textColor);
        XSetForeground(xdisplay, pxm_gc, textColor);
#endif  /*  MOTIF */
}


/*----------------------------------------------------------------------
|       Inverse text print
|
|       This routine  prints text on canvas inverted.  There must
|       be a better way to do this.
+------------------------------------------------------------------------*/
void Inversepw_text(int c, int r, char *s)
{
#ifdef MOTIF
   int    tw;
   long   invertColor;
#endif

   if (!useXFunc) {
       vnmrj_dstring(JRTEXT, 0, c, r, g_color, s);
       return;
   }
#ifdef MOTIF
   XSetForeground(xdisplay, vnmr_gc, textColor);
   XSetForeground(xdisplay, pxm_gc, textColor);
   gx1 = c + orgx;
   gy1 = r + orgy;
   gy2 = r - char_ascent - 1 + orgy;
   my2 = r - char_ascent - 1;
   if (winDraw)
       XDrawString(xdisplay, xid, vnmr_gc, gx1, gy1, s, strlen(s));
   if (xmapDraw)
       XDrawString(xdisplay, canvasPixmap, pxm_gc, c, r, s, strlen(s));
   invertColor = xwhite ^ textColor;
   XSetForeground(xdisplay, vnmr_gc, invertColor);
   XSetForeground(xdisplay, pxm_gc, invertColor);
   xormode();
   tw = get_text_width(s);
   if (winDraw)
       XFillRectangle(xdisplay, xid, vnmr_gc, gx1, gy2, tw, char_height);
   if (xmapDraw)
       XFillRectangle(xdisplay, canvasPixmap, pxm_gc, c, my2, tw, char_height);
   normalmode();
/*
   XSetForeground(xdisplay, vnmr_gc, textColor);
*/
#endif
}

void
dispw_text(c, r, s)
int c; int r; char *s;
{
    if (!useXFunc) {
       color(BLUE);
       vnmrj_dstring(JTEXT, 0, c, r, g_color,s);
       return;
    }
#ifdef MOTIF
    XSetForeground(xdisplay, vnmr_gc, textColor);
    XSetForeground(xdisplay, pxm_gc, textColor);
    r = r - char_descent;
    if (winDraw)
        XDrawString(xdisplay, xid, vnmr_gc, c+orgx, r+orgy, s, strlen(s));
    if (xmapDraw) 
        XDrawString(xdisplay, canvasPixmap, pxm_gc, c, r, s, strlen(s));
/*
    XFlush(xdisplay);
*/
#endif
}


void
Inversepw_box(c,r,s)    int c; int r; char *s;
{
#ifdef MOTIF
   int    tw;
   long   invertColor;
#endif

   if (!useXFunc) {
       vnmrj_dstring(JRBOX, 0, c, r, g_color, s);
       return;
   }
#ifdef MOTIF
   invertColor = xwhite ^ textColor;
   XSetForeground(xdisplay, vnmr_gc, invertColor);
   XSetForeground(xdisplay, pxm_gc, invertColor);
   XSetFunction(xdisplay, vnmr_gc, GXxor);
   XSetFunction(xdisplay, pxm_gc, GXxor);
   tw = get_text_width(s);
   gx1 = c + orgx;
   gy1 = r-char_ascent-1 + orgy;
   my1 = r-char_ascent-1;
   if (winDraw)
       XFillRectangle(xdisplay, xid, vnmr_gc, gx1, gy1, tw, char_height);
   if (xmapDraw)
       XFillRectangle(xdisplay, canvasPixmap, pxm_gc, c, my1, tw, char_height);
   normalmode();
   XSetForeground(xdisplay, vnmr_gc, textColor);
   XSetForeground(xdisplay, pxm_gc, textColor);
#endif
}


XFontStruct
*open_annotate_xfont(fontname)
char   *fontname;
{
#ifdef MOTIF
     XFontStruct  *fstruct;
#endif

     if (!useXFunc) {
        return(NULL);
     }
#ifdef MOTIF
     fstruct = XLoadQueryFont(xdisplay, fontname);
     return(fstruct);
#else
     return(NULL);
#endif
}


XFontStruct
*open_annotate_font(fontname, type, bold, italic, size)
char   *fontname, *type;
int    bold, italic, size;
{
#ifdef MOTIF
     int          count;
     char         **list;
     XFontStruct  *fstruct;
/*
     XFontStruct  *fontInfo;
 */
#endif

     if (!useXFunc) {
        return(NULL);
     }
#ifdef MOTIF
     if (bold)
        sprintf(fontFamily, "-%s-%s-bold-", fontname, type);
     else
        sprintf(fontFamily, "-%s-%s-regular-", fontname, type);
/*
        sprintf(fontFamily, "-monotype-arial-regular-");
*/
     if (italic)
        sprintf(fontFamily, "%si-*--%d-*-*-*-*-0-iso8859-*",fontFamily, size);
     else
        sprintf(fontFamily, "%sr-*--%d-*-*-*-*-0-iso8859-*",fontFamily, size);
/*
     list = XListFontsWithInfo(xdisplay, fontFamily, 1, &count, &fontInfo);
*/
     list = XListFonts(xdisplay, fontFamily, 1, &count);
     if (list == NULL || count <= 0)
        return (NULL);
     if (list[0] == NULL)
        return (NULL);
     fstruct = XLoadQueryFont(xdisplay, list[0]);
     XFreeFontNames(list);
/*
     XFreeFontInfo(list, fontInfo, count);
*/
/*
     if (fstruct == NULL) 
        return (NULL);
     if (annotateFontInfo)
     {
         XUnloadFont(xdisplay, annotateFontInfo->fid);
         XFreeFontInfo(NULL, annotateFontInfo, 1);
     } 
     annotateFontInfo = fstruct;
     set_x_font(annotateFontInfo);
*/
     return(fstruct);
#else
     return(NULL);
#endif
}



int    b_ascent;

void
banner_font(f, fontname)
double f;
char   *fontname;
{ 
    int size;

/*
   if (curwin <= 0)
	return;
    size = (int)(xcharpixels * f);
*/
    size = char_width;

    if (!useXFunc) {
       bannerSize = f;
       return;
    }
#ifdef MOTIF
    if (scalable_font < 0)
   	open_scalable_font("courier", size, 1);
    if (bannermap != 0)
    {
	XFreePixmap(xdisplay, bannermap);
	bannermap = 0;
    }
    if (f <= 1.0 && xstruct != NULL)
	structOfbannerFont = xstruct;
    else
    {
        if ( strcmp(fontname, bannerFont) != 0)
    	{
             if (scalable_font)
	     	open_scalable_font(fontname, size, 0);
	     else
             	open_bfont(fontname);
    	}
        if (bannerFontInfo != NULL)
	     structOfbannerFont = bannerFontInfo;
        else
	     structOfbannerFont = xstruct;
    }
    if (vchar_gc)
         XSetFont(xdisplay, vchar_gc, structOfbannerFont->fid);
    ycharpixels = structOfbannerFont->max_bounds.ascent +
                                 structOfbannerFont->max_bounds.descent;
    xcharpixels = structOfbannerFont->max_bounds.width;
    b_ascent = structOfbannerFont->max_bounds.ascent;
#endif
}

void
open_scalable_font(fontname, size, check)
char   *fontname;
int	size, check;
{
#ifdef MOTIF
        int          count;
        char   	     **list;
        XFontStruct  *fontInfo, *fstruct;
#endif

	if (!useXFunc) {
	    scalable_font = 1;
    	    strcpy(bannerFont, fontname);
	    return;
	}
#ifdef MOTIF
/*
	sprintf(fontFamily, "-*-%s-*-*-*--0-130-122-95-*-0-iso8859-1", fontname);
*/
	sprintf(fontFamily, "-*-%s-*-*-*--0-*-*-*-*-0-iso8859-*", fontname);
        list = XListFontsWithInfo(xdisplay, fontFamily, 1, &count, &fontInfo);
        XFreeFontInfo(list, fontInfo, count);
	if (count <= 0)
	{
	    strcpy(fontFamily, "-*-courier-*-*-*--0-*-*-*-*-0-iso8859-*");
            list = XListFontsWithInfo(xdisplay, fontFamily, 1, &count, &fontInfo);
            XFreeFontInfo(list, fontInfo, count);
	    if (count > 0)
    	      strcpy(bannerFont, "courier");
	}
	else
    	      strcpy(bannerFont, fontname);
	if (check)
	{
	    if (count <= 0)
		scalable_font = 0;
	    else
		scalable_font = 1;
	    return;
	}
	if (count <= 0)
	    return;
	sprintf(fontFamily, "-*-%s-bold-*-*--%d-*-*-*-*-0-iso8859-*", bannerFont, size);
        list = XListFontsWithInfo(xdisplay, fontFamily, 1, &count, &fontInfo);
	if (count <= 0)
	{
	    sprintf(fontFamily, "-*-%s-*-*-*--%d-*-*-*-*-0-iso8859-*", bannerFont, size);
            list = XListFontsWithInfo(xdisplay,fontFamily,1,&count,&fontInfo);
	    if (count <= 0)
	        return;
	}
        if ((fstruct = XLoadQueryFont(xdisplay, list[0]))!=NULL)
        {
            if (bannerFontInfo != NULL)
	    {
	        XUnloadFont(xdisplay, bannerFontInfo->fid);
                XFreeFontInfo(NULL, bannerFontInfo, 1);
	    }
	    scalable_font = 3;
/*
    	    strcpy(bannerFont, fontname);
*/
	    bannerFontInfo = fstruct;
        }
        XFreeFontInfo(list, fontInfo, count);
#endif  /*  MOTIF */
}


void
open_bfont(fontname)
char   *fontname;
{
#ifdef MOTIF
        int     count, num, fwidth;
        int     size, rsize;
        char   *fname, **list;
        XFontStruct  *fontInfo, *fstruct;

	if (!useXFunc)
            return;
        num = 40;
	if (fontname == NULL)
	   strcpy(fontFamily, "*courier-bold-i-*");
	else
	   sprintf(fontFamily, "*%s-bold-*", fontname);
        list = XListFontsWithInfo(xdisplay, fontFamily, 
                                 num, &count, &fontInfo);
        if ( count <= 0)
        {
#ifndef INTERACT
	       Werrprintf("Argument \'%s\' is not valid", fontname);
#endif 
               return;
	}
        size = 36;
        rsize = 0;
        fname = NULL;
        for (num = 0; num < count; num++)
        {
                fstruct = fontInfo + num;
                if ( fstruct != NULL)
                {
                        fwidth = fstruct->max_bounds.width;
			if (fwidth == 0)  /* SGI info */
                            fwidth = fstruct->ascent + fstruct->descent;
                        if (abs(fwidth - size) < abs(rsize - size))
                        {
                                rsize = fwidth;
                                fname = list[num];
                        }
                }
        }
        if (fname == NULL)
                return;
        if ((fstruct = XLoadQueryFont(xdisplay, fname))!=NULL)
        {
               if (bannerFontInfo)
	       {
	           XUnloadFont(xdisplay, bannerFontInfo->fid);
                   XFreeFontInfo(NULL, bannerFontInfo, 1);
	       }
	       bannerFontInfo = fstruct;
        }
    	strcpy(bannerFont, fontname);
        XFreeFontInfo(list, fontInfo, count);
#endif  /*  MOTIF */
}


int
scalable_font_size(int scale)
{
#ifdef MOTIF
	static int  old_scale = 0;
 	XFontStruct  *fstruct;
#endif

	if (!useXFunc) {
	    scale = scale / xcharpixels;
            j1p_func(JSFONT, scale);
	    return(1);
	}
	if (!scalable_font)
	    return(1);
#ifdef MOTIF
	if (old_scale != scale || scalable_font > 1)
	{
	    old_scale = scale;
	    scalable_font = 1;
/*
	    sprintf(fontFamily, "-*-%s-bold-*-*--%d-*-*-*-*-0-iso8859-*", 
			bannerFont, scale * xcharpixels);
*/
	    sprintf(fontFamily, "-*-%s-bold-*-*--%d-*-*-*-*-0-iso8859-*", 
			bannerFont, scale);
            if ((fstruct = XLoadQueryFont(xdisplay, fontFamily)) ==NULL)
	    {
		sprintf(fontFamily,"-*-%s-*-*-*--%d-*-*-*-*-0-iso8859-*",
			bannerFont, scale);
                fstruct = XLoadQueryFont(xdisplay, fontFamily);
	    }
	    if (fstruct != NULL)
	    {
		if (scaleFontInfo != NULL)
		{
		   XUnloadFont(xdisplay, scaleFontInfo->fid);
                   XFreeFontInfo(NULL, scaleFontInfo, 1);
		}
		scaleFontInfo = fstruct;
	    }
	}
	if (scaleFontInfo)
	{
	    structOfbannerFont = scaleFontInfo;
    	    ycharpixels = structOfbannerFont->max_bounds.ascent +
                                 structOfbannerFont->max_bounds.descent;
    	    xcharpixels = structOfbannerFont->max_bounds.width;
    	    b_ascent = structOfbannerFont->max_bounds.ascent;
	    return(ycharpixels);
	}
#endif  /*  MOTIF */
	return(1);
}


void
scalable_font_text(scale, text)
int    scale;
char   *text;
{
#ifdef MOTIF
	int	    y, my;

	if (!useXFunc)
           return;
        XSetFont(xdisplay, vnmr_gc, structOfbannerFont->fid);
        XSetFont(xdisplay, pxm_gc, structOfbannerFont->fid);
	my = graf_height - xgry + b_ascent;
	y = my + orgy;
	if (winDraw)
            XDrawString(xdisplay, xid, vnmr_gc, xgrx + orgx, y, text, strlen(text));
	if (canvasPixmap)
             XDrawString(xdisplay, canvasPixmap, pxm_gc, xgrx, my, text, strlen(text));
        if (x_font) {
            XSetFont(xdisplay, vnmr_gc, x_font);
            XSetFont(xdisplay, pxm_gc, x_font);
	}
/*
        XFlush(xdisplay);
*/
#endif
}

#ifdef MOTIF
static void resetGCStyle() {
	XSetLineAttributes(xdisplay, vnmr_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
}
#endif

void
scaledText(scale, text)
int    scale;
char   *text;
{
#ifdef MOTIF
	static int  old_scale = 0;
  	int  i, j;
  	int  pos_x;
  	int  strWidth, index;
  	unsigned long  pixel;
  	XImage  *tmp_image;
	static  int  px_h = 0;
	static  int  px_w = 0;
#endif
  	int  pos_y;

	if ((int)strlen(text) <= 0)
	     return;
	if (!useXFunc) {
	    color(YELLOW);
	    pos_y = graf_height - xgry + b_ascent;
	    vnmrj_dstring(JSTEXT, 0, xgrx, pos_y, g_color, text);
	    return;
	}
#ifdef MOTIF
	if (scalable_font)
	{
	     scalable_font_text(scale, text);
	     return;
	}
	if (!one_plane)
	     return;
        if (structOfbannerFont == NULL)
	     banner_font(1.0, "courier");
	if (px_h != ycharpixels || px_w != graf_width)
	{
	     if (bannermap != 0)
                  XFreePixmap(xdisplay, bannermap);
	     bannermap = XCreatePixmap(xdisplay, xid, graf_width,ycharpixels,1);
	     px_h = ycharpixels;
	     px_w = graf_width;
	     if (bannermap == 0)
            return;
	     if (vchar_gc == NULL)
	     {
	         vchar_gc = XCreateGC(xdisplay, bannermap, 0, 0);
         	 XSetFont(xdisplay, vchar_gc, structOfbannerFont->fid);
	     }
	}
  	if (charmap == 0)
	     return;
        if (old_scale != scale)
	     old_scale = scale;
        XSetForeground(xdisplay, vchar_gc, 0);
        XFillRectangle(xdisplay, bannermap, vchar_gc, 0, 0,graf_width, ycharpixels);
        XSetForeground(xdisplay, vchar_gc, 1);
        XDrawString(xdisplay, bannermap, vchar_gc, 0, b_ascent, text, strlen(text) );
        strWidth = XTextWidth(structOfbannerFont, text, strlen(text));
	if (strWidth < graf_width - 2)
		strWidth = strWidth + 2;
	else
		strWidth = graf_width;
        tmp_image = XGetImage(xdisplay, bannermap, 0, 0, strWidth, ycharpixels,
                                1, XYPixmap);
	XSetLineAttributes(xdisplay, vnmr_gc, scale, LineSolid,
                                CapNotLast, JoinRound);
        pos_y = graf_height-xgry-1 + orgy;
	index = 0;
        for(j = 0; j < ycharpixels; j++)
        {
        	pos_x = xgrx + orgx;
        	for(i = 0; i < strWidth; i++)
                {
                    pixel = XGetPixel(tmp_image, i, j);
                    if (pixel)
                    {
			segments[index].x1 = pos_x;
                        segments[index].x2 = pos_x + scale;
                        segments[index].y1 = pos_y;
                        segments[index].y2 = pos_y;

                        index++;
                        if(index > 99)
                        {
			    if (winDraw)
                                XDrawSegments(xdisplay, xid, vnmr_gc, segments,
						 index);
    			    if (canvasPixmap)
				XDrawSegments(xdisplay, canvasPixmap, pxm_gc,
					segments, index);
			    index = 0;
			}
                    }
                    pos_x = pos_x + scale;
                }
                pos_y = pos_y + scale;
        }
        XDestroyImage(tmp_image);
	if(index > 0)
	{
	        if (winDraw)
                    XDrawSegments(xdisplay, xid, vnmr_gc, segments, index);
    		if (canvasPixmap)
		    XDrawSegments(xdisplay, canvasPixmap, pxm_gc,
					segments, index);
	}
	resetGCStyle();
#endif  /*  MOTIF */
}
static int numplanes = 1;

/** create memory for HP PCL */
int
create_pixmap()
{
      static int  pixmap_x = 0;
      static int  pixmap_y = 0;
      static int  pixmap_p = 0;
      char        *mem;
      int         i, j;

      raster_cols = (mnumxpnts + 7) / 8;
      numplanes = get_plot_planes();
      if (numplanes > MAX_PLANES)
          numplanes = MAX_PLANES;
      if ( (pixmap == NULL) || (pixmap_x != raster_cols) ||
           (pixmap_y != mnumypnts) || (pixmap_p != numplanes) )
      {  
            releaseWithId("pixmap");
            pixmap_x = raster_cols;
            pixmap_y = mnumypnts;
            pixmap_p = numplanes;
            pixmap_c = raster_cols * mnumypnts;
            for ( i=0; i<numplanes; i++ )
            {
                r_pixmap[i] = (char *) allocateWithId (pixmap_c, "pixmap");
                if (r_pixmap[i] == NULL)
                {
#ifndef INTERACT
                 Werrprintf("Could not create pixel buffer");
#endif 
                 raster = 0;
                 plot = 0;
                 pixmap = NULL;
                 releaseWithId("pixmap");
                 active_gdevsw = &(gdevsw_array[C_TERMINAL]);
                 return (0);
                }
            }
 
            for ( j=0; j<numplanes; j++ )            /* clear all pixmaps */
            {
                mem = r_pixmap[j];
                for (i = 0; i < pixmap_c; i++)
                {
                    *mem = 0;
                    mem++;
                }
            }
      }
      xcolor = 1;
      xorflag = 0;
      return (1);
}

static int  PCLresoultions[] = {96, 100, 120, 150, 160, 180, 200, 240,
                  288, 300, 360, 400, 450, 480, 600 };
// Executive, letter, legal, 11x17, A4, A3
static int    PCLpaperIndexes[] = { 1, 2, 3, 6, 26, 27 };
static double PCLpaperWidths[] = {184.1, 215.9, 215.9, 279.4, 210.8, 297.1 };
static double PCLpaperHeights[] = {266.7, 279.4, 355.6, 431.8, 297.2, 419.1 }; 

static void set_PCL_offset()
{
     double pw, ph; 
     double dw, dh;
     double edge;
     double pclUnit;
     int    paperSize, k, n;

     if (raster_resolution <= 0 || ppmm <= 0.0)
        return;
     pw = getpaperwidth();
     ph = getpaperheight();
     if (pw < 10.0 || ph <= 10.0) // not new device type
        return;
     paperSize = 2;
     n = sizeof(PCLpaperIndexes) / sizeof(int);
     for (k = 0; k < n; k++)
     {
         dw = pw - PCLpaperWidths[k];
         dh = ph - PCLpaperHeights[k];
         if (dw < 2.0 && dw > -2.0) {
            if (dh < 2.0 && dh > -2.0) {
                paperSize = PCLpaperIndexes[k];
                break;
            }
         }
     }

     if (raster == 2) // PCL landscape
     {
        if (ph > pw)
        {
            dw = pw;
            pw = ph;
            ph = dw;
        }
     }
     pclUnit = 300.0;
     fprintf(plotfile,"%c&u%dD",27, (int)pclUnit); // PCL units
     if (paperSize != 2)
        fprintf(plotfile,"%c&l%dA",27, paperSize);

     pclUnit = 720.0;
     edge = 15.0;
     dw = ((double) mnumypnts) / ppmm;
     if (raster == 1)
        dh = ph - bottom_edge - dw - edge;
     else
        dh = 0.0;
     if (dh > 1.0)
     {
         dw = dh * pclUnit / 25.4;
         fprintf(plotfile,"%c&l%dZ",27, (int)dw); // top offset
     }
     edge = 7.0;
     dh = ((double) mnumxpnts) / ppmm;
     if (raster == 1)
         dw = pw - right_edge - dh - edge;
     else
         dw = 0.0;
     if (dw > 1.0)
     {
         dh = dw * pclUnit / 25.4;
         if (dh < 0.0)
            dh = 0.0;
          fprintf(plotfile,"%c&l%dU",27, (int)dh); // left offset
     }
     fprintf(plotfile,"%c*p0x0Y",27);
}

void
dump_raster_image()
{ 
  int   bytes,mx;
  unsigned char *b,*b0;
  register int   n,i,j,bm1,bm2,out;

  unsigned char    *temp1[] = {NULL, NULL, NULL, NULL};
  unsigned char    *temp2[] = {NULL, NULL, NULL, NULL};
  int     mxx[MAX_PLANES], bm11[MAX_PLANES], bm22[MAX_PLANES];

/****
amove(1,1);
rdraw(mnumxpnts,0);
amove(1,1);
rdraw(0,mnumypnts);
amove(1, mnumypnts -1);
rdraw(mnumxpnts,0);
amove(mnumxpnts-1, 1);
rdraw(0,mnumypnts);
***/

  if (plotfile == NULL)
      return;
  bytes = raster_cols;
  for(n=0; n<numplanes; n++)
  {
     pixmap = r_pixmap[n];               /* (0,0) is at bottom left conner */
     if (pixmap == NULL)
        return;
     mxx[n] = 0;
     bm11[n] = 0;
     bm22[n] = 0;
               /* prepare to start from the upper left conner of the pixmap */
     temp1[n] = temp2[n] = (unsigned char *)pixmap + (mnumypnts - 1) * bytes;
  }
     
     set_PCL_offset();

     fprintf(plotfile,"%c*r%dA\n",27,1);   // start graphics at cursor

     mx = mnumypnts;
     if (raster>1) /* raster reversed,landscape */
     {    if (raster_resolution==-192)
          {  bm1 = 128+64;
             /* not sure if b0 initialization is correct */
             /* On the plus side, no one uses 192 resolution any more */
             b0 = (unsigned char *)pixmap;
             for (i=0; (i<mnumxpnts/2) && (!interuption); i++)
             {   if (bm1==128+64)
                 {   b = b0;
                     mx = mnumypnts;
                     while ((mx>2) && (*b==0))
                     {   b -= bytes;
                         mx--;
                     }
                 }
                 fprintf(plotfile,"%c*b%dW",27,(mx+7)/8);
                 b = b0 - (mnumypnts * bytes);
                 bm2 = 128;
                 out = 0;
                 for (j=0; j<mx; j++)
                 {
                     b += bytes;
                     if (*b & bm1)
                         out = out | bm2;
                     bm2 >>= 1;
                     if (bm2==0)
                     {   putc(out,plotfile);
                         out = 0;
                         bm2 = 128;
                     }
                 }
                 if (bm2 != 128)
                     putc(out,plotfile);
                 bm1 >>= 2;
                 if (bm1==0)
                 {   bm1 = 128+64;
                     b0 += 1;
                 }
             }    
          }   
          else   /* ThinkJet_96 & LaserJet_300 REVERSE */
          {
            for( i=0; i<numplanes; i++ )
                 bm11[i] = 128;
            for (i=0; (i<mnumxpnts) && (!interuption); i++)
            {
              for( n=0; n<numplanes; n++)
              {  
                 pixmap = r_pixmap[n];

                 if (bm11[n]==128)
                 {
                      temp1[n] = temp2[n];
                      mxx[n] = mnumypnts;
                      while ((mxx[n]>2) && (*temp1[n]==0))
                      {
                          temp1[n] -= bytes;
                          mxx[n]--;
                      }
                 }
                 if( pixmap != r_pixmap[numplanes-1] )
                     fprintf(plotfile,"%c*b%dV",27,(mxx[n]+7)/8);
                 else
                     fprintf(plotfile,"%c*b%dW",27,(mxx[n]+7)/8);
 
                 temp1[n] = temp2[n] - (mnumypnts * bytes);
                 bm22[n] = 128;
                 out = 0;
 
                 for (j=0; j<mxx[n]; j++)
                 {
                      temp1[n] += bytes;
                      if (*temp1[n] & bm11[n])
                             out = out | bm22[n];
                      bm22[n] >>= 1;
                      if (bm22[n]==0)
                      {   putc(out,plotfile);
                          out = 0;
                          bm22[n] = 128;
                      }  
                 }
                 if (bm22[n]!=128)
                      putc(out,plotfile);
 
                 bm11[n] >>= 1;
                 if (bm11[n]==0)
                 {
                     bm11[n] = 128;
                     temp2[n] += 1;
                 }
 
              } /*end of for (n=0; n<numplanes; n++) */
            } /*end of for */
         }   /* end of else  LaserJet */
     } /* end of if REVERSE  raster>1 */
     else /* raster PORTRAIT (not reversed) */
     {
        if (raster_resolution==-192)
        {
             /* not sure if b and b0 initialization is correct */
             /* On the plus side, no one uses 192 resolution any more */
             b = b0 = (unsigned char *)pixmap;
             for (i=0; (i<mnumypnts) && (!interuption); i+=2)
             {   mx = bytes;
                 while (((b0[mx-1] | (b0+bytes)[mx-1])==0) && (mx>2))
                        mx--;
                 fprintf(plotfile,"%c*b%dW",27,mx);
                 for (j=0; j<mx; j++)
                 {    putc(*b0 | *(b0+bytes),plotfile);
                      b0++;
                 }
                 b -= 2 * bytes;
                 b0 = b;
             }
         }
         else  /* ThinkJet_96 & LaserJet_300 PORTRAIT */
         {
             for (i=0; (i < mnumypnts) && (!interuption); i++)
             {   
               for ( n=0; n<numplanes; n++ )
               {
                 pixmap = r_pixmap[n];
                 mx = bytes;
 
                 while( (*(temp2[n]+(mx-1))==0) && (mx>2) ) mx--;
                 {
                   if( pixmap != r_pixmap[numplanes-1] )
                     fprintf(plotfile,"%c*b%dV",27, mx);
                   else
                     fprintf(plotfile,"%c*b%dW",27, mx);
                 }
                 for (j=0; j<mx; j++)
                    putc(*temp2[n]++,plotfile);
                 temp1[n] -= bytes;
                 temp2[n] = temp1[n];
               }
             }   
         }      
     } /* end of else portrait */
     for( i=0; i<numplanes; i++)
     {   
         releaseWithId("r_pixmap[i]");
         r_pixmap[i] = NULL;
     }   
     pixmap = NULL;

     fprintf(plotfile,"%c*rB",27);   // end raster graphics 
}

/******************/   /* no sun provision yet */
void
fraster_charsize(f)
/******************/
double f;
{ /* character size */
  static int  times = 0;
  static int  numUserFonts = 0, numSysFonts = 0;
  int   size, psize, k, dif, found;
  int   which;
  char  filename[MAXPATHL];
  struct stat  fStatus;

#ifdef INTERACT

   if (P_getstring(GLOBAL,"systemdir",systemdir,1,MAXPATHL))
      strcpy(systemdir,"/vnmr");

   if (P_getstring(GLOBAL,"userdir",userdir,1,MAXPATHL)) {
      strcpy(userdir, (char *)getenv( "HOME" ) );
      strcat(userdir, "/vnmrsys" );
   }
#endif 

    size = (int) ((raster_charsize * f) + 0.5);
    if (times == 0)
    {
        for (k = 0; k < FONTNUM; k++)
        {
            if((varFont[k] = (struct varfontInfo *) allocateWithId
                    (sizeof(struct varfontInfo), "fontinfo")) == 0)
                return;
            varFont[k]->order = 0;
            varFont[k]->width = 0;
            varFont[k]->size = 0;
            varFont[k]->mtime = 0;
            varFont[k]->which = 9;
            varFont[k]->data = NULL;
        }
	times = 1;
    }
    sprintf(filename, "%s/fonts", userdir);
    list_fonts(USERFONT, &numUserFonts, filename);
    sprintf(filename, "%s/fonts", systemdir);
    list_fonts(SYSFONT, &numSysFonts, filename);

    dif = 200;
    psize = 0;
    which = USERFONT;
    for(k = 0; k < numUserFonts; k++)
    {
        if (abs(userFontList[k] - size) < dif)
        {
            psize = userFontList[k];
            dif = abs(psize - size);
        }
    }
    for(k = 0; k < numSysFonts; k++)
    {
        if (abs(sysFontList[k] - size) < dif)
        {
            psize = sysFontList[k];
            dif = abs(psize - size);
	    which = SYSFONT;
        }
    }
    if (psize == 0)
	return;
     if (which == SYSFONT)
        sprintf(filename, "%s/fonts/font%d", systemdir, psize);
     else
        sprintf(filename, "%s/fonts/font%d", userdir, psize);
    /*  check the list of opened fonts */
    for(k = 0; k < FONTNUM; k++)
    {
        if ( varFont[k]->size == psize && varFont[k]->which == which)
        {
	     if (stat (filename, &fStatus) >= 0)
             {
	        if (varFont[k]->mtime != fStatus.st_mtime)
	        {
     	   	   open_var_font(filename, k, which, psize);
		   return;
                }
	     }
             rasterFont = varFont[k];
	     varFont[k]->order = FONTNUM;
             ycharpixels = rasterFont->height + 1;
             xcharpixels = rasterFont->width;
             raster_ascent = rasterFont->ascent;
             return;
        }
    }
    /*  look for an entry in the list */
    found = 0;
    for(k = 0; k < FONTNUM; k++)
    {
        if (varFont[k]->order == 0)
        {
            found = k;
            break;
        }
    }
    if (varFont[found]->order != 0) /* no free entry available, one must be removed */
    {
        found = FONTNUM+1;
        for(k = 0; k < FONTNUM; k++)
        {
             if (varFont[k]->order < found)
                found = k;
             varFont[k]->order = varFont[k]->order - 1;
        }
     }

     if (which == SYSFONT)
        sprintf(filename, "%s/fonts/font%d", systemdir, psize);
     else
        sprintf(filename, "%s/fonts/font%d", userdir, psize);
     open_var_font(filename, found, which, psize);
     save_raster_charsize = size;
     return;
}

void
raster_fontsize(f)
double f;
{ }

void
raster_dimage(x,y,w,h)
int x, y, w, h;
{}

void
raster_amove(x,y)
int x,y;
{ 
  if (x < 0)
        xgrx = 0;
  else if (x >= mnumxpnts)
        xgrx = mnumxpnts - 1;
  else
        xgrx = x;
  if (y < 0)
        xgry = 0;
  else if (y >=  mnumypnts)
        xgry = mnumypnts - 1;
  else
        xgry = y;
  /* do nothing here, just use xgrx and xgry later */
}

void
raster_box(x,y)
int x,y;
{
  if ((g_color!=BLACK) && (g_color!=BACK))
    (*(*active_gdevsw)._grayscale_box)(xgrx,xgry,x,y,MAXSHADES-1);
}

void
raster_pixel_pixel(x, y)
int x,y;
{
    char  *cell, data;
    int    bit, icell;

    icell = y * raster_cols + (x >> 3);
    if (icell >= pixmap_c)
       return;
    cell = pixmap + y * raster_cols + (x >> 3);
    bit = x % 8;
    data = 0x80 >> bit;

    if (xcolor)
        *cell |= data;
    else
        *cell ^= data;
}


int
raster_pixel(x, y)
int x,y;
{

   switch(raster_plot_color)
   {
      case MONOCHROME: pixmap=r_pixmap[0]; raster_pixel_pixel(x, y); break;
 
      case     CYAN: pixmap=r_pixmap[1]; raster_pixel_pixel(x, y); break;

      case  MAGENTA: pixmap=r_pixmap[2]; raster_pixel_pixel(x, y); break;

      case   YELLOW: pixmap=r_pixmap[3]; raster_pixel_pixel(x, y); break;

      case      RED: pixmap=r_pixmap[2]; raster_pixel_pixel(x, y);
                     pixmap=r_pixmap[3]; raster_pixel_pixel(x, y); break;

      case    GREEN: pixmap=r_pixmap[1]; raster_pixel_pixel(x, y);
                     pixmap=r_pixmap[3]; raster_pixel_pixel(x, y); break;

      case     BLUE: pixmap=r_pixmap[1]; raster_pixel_pixel(x, y);
                     pixmap=r_pixmap[2]; raster_pixel_pixel(x, y); break;
 
      case    WHITE:
      case    BLACK: pixmap=r_pixmap[1]; raster_pixel_pixel(x, y);
                     pixmap=r_pixmap[2]; raster_pixel_pixel(x, y);
                     pixmap=r_pixmap[3]; raster_pixel_pixel(x, y); break;
 
      default: // Wscrprintf( "raster_plot_color #%d not supported\n", raster_plot_color );
                     pixmap=r_pixmap[0]; raster_pixel_pixel(x, y); break;
   }
   return(1);
}


void
raster_adraw(x,y)
int x,y;
{
   int   x1, x2, y1, y2, dx, dy, xbase;
   int   tmp, dif, inc1, inc2, inc3;

   xbase = 1;
   if (x < 0)
        x = 0;
   if (y < 0)
        y = 0;
   dx = abs(xgrx - x);
   dy = abs(xgry - y);
   if (dx == 0 && dy == 0)
   {
        raster_pixel(x, y);  /* draw point */
        return;
   }
   if ( dy > dx )
   {
        xbase = 0;
        x1 = xgry;
        x2 = y;
        y1 = xgrx;
        y2 = x;
   }
   else
   {
        x1 = xgrx;
        x2 = x;
        y1 = xgry;
        y2 = y;
   }
   if (x1 > x2)
   {
        tmp = x1;
        x1 = x2;
        x2 = tmp;
        tmp = y1;
        y1 = y2;
        y2 = tmp;
   }
   dx = x2 - x1;
   dy = y2 - y1;
   if ( dy >= 0 )
   {
        dif = 2 * dy - dx;
        inc1 = 2 * dy;
        inc2 = 2 * (dy - dx);
        inc3 = 1;
   }
   else
   {
        dif = -2 * dy - dx;
        inc1 = -2 * dy;
        inc2 = -2 * (dy + dx);
        inc3 = -1;
   }
   if (xbase)
        raster_pixel(x1, y1);
   else
        raster_pixel(y1, x1);
   while ( x1 < x2 )
   {
        x1++;
        if ( dif < 0.0 )
           dif = dif + inc1;
        else
        {
           y1 = y1 + inc3;
           dif = dif + inc2;
        }
        if (xbase)
           raster_pixel(x1, y1);
        else
           raster_pixel(y1, x1);
   }
   xgrx = x;
   xgry = y;
}

void
raster_rdraw(x,y)
int x,y;
{
   raster_adraw(xgrx + x, xgry + y);
}

void
raster_dchar_dchar(ch)
char ch;
{
    char  *cell, *data, *data2;
    unsigned char   bit1, bit2, bit3;
    int    j, k, y;

    if ( rasterFont == NULL || pixmap == NULL)
        return;
    if (ch > rasterFont->last || ch < rasterFont->start)
        return;
    y = xgry + rasterFont->ascent;

    if (y >= mnumypnts)
        return;
    if (xgrx + rasterFont->width >= mnumxpnts)
        return;
    cell = pixmap + y * raster_cols + (xgrx / 8);
    data = rasterFont->data + (ch - rasterFont->start) * rasterFont->bytes;
    k = xgrx % 8;
    bit3 = 0x80 >> k;
    bit1 = 0x80;
    for(j = 0; j < rasterFont->height; j++)
    {
        bit2 = bit3;
        data2 = cell;
        for(k = 0; k < rasterFont->width; k++)
        {
            if (bit1 & *data)
                *data2 = *data2 | bit2;
            bit1 = bit1 >> 1;
            bit2 = bit2 >> 1;
            if (bit1 == 0)
            {
                data++;
                bit1 = 0x80;
            }
            if (bit2 == 0)
            {
                data2++;
                bit2 = 0x80;
            }
        }
        cell = cell - raster_cols;
        if (cell < pixmap)
            break;
    }
}

int
raster_dchar( c )
char c;
{
    if ( rasterFont == NULL )
        return(0);
    if (c == ' ')
    {
        xgrx += rasterFont->width;
        return (1);
    }
    switch(raster_plot_color)
    {
        case MONOCHROME: pixmap=r_pixmap[0]; raster_dchar_dchar(c);
                        break;
        case     CYAN: pixmap=r_pixmap[1]; raster_dchar_dchar(c);
                        break;
        case  MAGENTA: pixmap=r_pixmap[2]; raster_dchar_dchar(c);
                        break;
        case   YELLOW: pixmap=r_pixmap[3]; raster_dchar_dchar(c);
                        break;
        case      RED: pixmap=r_pixmap[2]; raster_dchar_dchar(c);
                       pixmap=r_pixmap[3]; raster_dchar_dchar(c);
                        break;
        case    GREEN: pixmap=r_pixmap[1]; raster_dchar_dchar(c);
                       pixmap=r_pixmap[3]; raster_dchar_dchar(c);
                        break;
        case     BLUE: pixmap=r_pixmap[1]; raster_dchar_dchar(c);
                       pixmap=r_pixmap[2]; raster_dchar_dchar(c);
                        break;
        case    WHITE:
        case    BLACK: pixmap=r_pixmap[1]; raster_dchar_dchar(c);
                       pixmap=r_pixmap[2]; raster_dchar_dchar(c);
                       pixmap=r_pixmap[3]; raster_dchar_dchar(c);
                        break;

        default: // Wscrprintf( "raster_plot_color #%d not supported\n", raster_plot_color );
                       pixmap=r_pixmap[0]; raster_dchar_dchar(c);
                       break;

    }
    xgrx += rasterFont->width;
    return(1);
}

void
raster_dstring(s)        char *s;
{
   int  len;
   char data;

   len = strlen(s);
   while (len > 0)
   {
        data = *s;
        raster_dchar(data);
        s++;
        len--;
   }
}

void raster_dvchar_dvchar(char ch)
{
    char  *cell, *data, *data2;
    unsigned char   bit1, bit2;
    int    j, k, y;

    if ( rasterFont == NULL || pixmap == NULL)
        return;
    if (ch > rasterFont->last || ch < rasterFont->start)
        return;
    if (xgry >= mnumypnts - rasterFont->width ||
                xgrx >= mnumxpnts - rasterFont->height)
        return;
    if (ch == 32) /* space */
    {
       return;
    }
    y = xgry + rasterFont->ascent;
    cell = pixmap + xgry * raster_cols + (xgrx / 8);
    data = rasterFont->data + (ch - rasterFont->start) * rasterFont->bytes;
    k = xgrx % 8;
    bit2 = 0x80 >> k;
    bit1 = 0x80;
    for(j = 0; j <  rasterFont->height; j++)
    {
        data2 = cell;
        for(k = 0; k < rasterFont->width; k++)
        {
            if (bit1 & *data)
                *data2 = *data2 | bit2;
            bit1 = bit1 >> 1;
            if (bit1 == 0)
            {
                data++;
                bit1 = 0x80;
            }
            data2 += raster_cols;
        }
        bit2 = bit2 >> 1;
        if (bit2 == 0)
        {
            cell++;
            bit2 = 0x80;
        }
    }
}

int raster_dvchar(char c )
{
    if ( rasterFont == NULL)
        return (0);
    if (c == ' ')
    {
	xgry += rasterFont->width;
	return(1);
    }
    switch(raster_plot_color)
    {
        case MONOCHROME: pixmap=r_pixmap[0]; raster_dvchar_dvchar(c);
                                                                      break;
        case     CYAN: pixmap=r_pixmap[1]; raster_dvchar_dvchar(c);
                                                                      break;
        case  MAGENTA: pixmap=r_pixmap[2]; raster_dvchar_dvchar(c);
                                                                      break;
        case   YELLOW: pixmap=r_pixmap[3]; raster_dvchar_dvchar(c);
                                                                      break;
        case      RED: pixmap=r_pixmap[2]; raster_dvchar_dvchar(c);
                       pixmap=r_pixmap[3]; raster_dvchar_dvchar(c);
                                                                      break;
 
        case    GREEN: pixmap=r_pixmap[1]; raster_dvchar_dvchar(c);
                       pixmap=r_pixmap[3]; raster_dvchar_dvchar(c);
                                                                      break;
 
        case     BLUE: pixmap=r_pixmap[1]; raster_dvchar_dvchar(c);
                       pixmap=r_pixmap[2]; raster_dvchar_dvchar(c);
                                                                      break;

        case    WHITE:
        case    BLACK: pixmap=r_pixmap[1]; raster_dvchar_dvchar(c);
                       pixmap=r_pixmap[2]; raster_dvchar_dvchar(c);
                       pixmap=r_pixmap[3]; raster_dvchar_dvchar(c);
                                                                      break;

        default: // Wscrprintf( "raster_plot_color #%d not supported\n", raster_plot_color );
                       pixmap=r_pixmap[0]; raster_dvchar_dvchar(c);
                       break;

    }
    xgry += rasterFont->width;
    return(1);
}

void raster_vchar(char c )
{}


/**********************************************/
void
raster_ybars(int dfpnt, int depnt, struct ybar *out,
             int vertical, int maxv, int minv)  /* draws a spectrum */
/**********************************************/
{ register int i;

  /* first check vertical limits */
  if (maxv) 
    range_ybar(dfpnt,depnt,out,maxv,minv);
  if (vertical)
  {    for (i=dfpnt; i<depnt; i++)
       {  if (out[i].mx>=out[i].mn) /* is in mnumypnts space */
          {
             if (out[i].mx != out[i].mn)
	     {
                  raster_amove(mnumxpnts - out[i].mn, i);
                  raster_adraw(mnumxpnts - out[i].mx, i);
             }
             else
                  raster_pixel(mnumxpnts - out[i].mx,i);

          }
       }
  }
  else
  {    for (i=dfpnt; i<depnt; i++)
       {  if (out[i].mx>=out[i].mn) /* is in mnumypnts space */
          {
              if (out[i].mx != out[i].mn)
	      {
                  raster_amove(i, out[i].mn);
                  raster_adraw(i, out[i].mx);
             }
             else
                  raster_pixel(i, out[i].mx);
          }
       }
  }
}


void set_raster_gray_matrix(int size)
{
        int k;
        int n = sizeof(PCLresoultions) / sizeof(int);
        if (size >= 0) {
            for (k = 0; k < n; k++) {
               if (size <= PCLresoultions[k])
                  break;
            }
            if (k >= n)
               k = n -1;
            raster_resolution = PCLresoultions[k];
            size = raster_resolution;
        }
	if (size == 150)
	{
	    rast_gray_shades = 4;
	    raster_gray_matrix = gray_150;
	}
	else if (size == 600)
	{
	    rast_gray_shades = 8;
	    raster_gray_matrix = gray_600;
	}
	else
	{
	    rast_gray_shades = 6;
	    raster_gray_matrix = gray_300;
	}
	gray_offset = 0.0;
}


/***************************************/
int
raster_grayscale_box(oldx,oldy,x,y,shade)
/***************************************/
int oldx,oldy,x,y,shade;
{ int shade0,i,j, k;
  double yshade;

  if ((x<1) || (y<1)) return 1;
  xgrx = oldx;
  xgry = oldy;
/* changes to reflect screen adjustable contrast onto raster */
  yshade = (float) shade - graycntr;
  yshade *= graysl;
  yshade = yshade + graycntr - gray_offset;
  shade0 = (int) yshade;
  if (shade0 < 1) return(0);
/*
  if (shade0 < 0) shade0 = 0;
     shade0 = shade0 % gray_shades;
  if (shade0>=MAXSHADES) shade0=MAXSHADES-1;
  for(i=0; i<x; i++)
    for(j=0; j<y; j++)
      { if (shade0>gray_matrix[(xgrx+i)%rast_gray_shades][(xgry+j)%rast_gray_shades])
          raster_pixel(xgrx+i, xgry+j);
      }
*/
  for(i=0; i<x; i++)
  {
    for(j=0; j<y; j++)
    {
	k = ((xgry+j) % rast_gray_shades) * rast_gray_shades + (xgrx+i)%rast_gray_shades;
        if (shade0 > raster_gray_matrix[k])
            raster_pixel(xgrx+i, xgry+j);

    }
  }
  xgrx += x;
  xgry += y;
  return 0;
}


void
open_fontx(size, width, height, fontid)
int  size, *width, *height;
Font *fontid;
{
}

void
fscaledText(text)
char   *text;
{
}


void
list_fonts(which, num, directory)
  int  which;   /* 0 is systemdir, 1 is userdir  */
  int  *num;
  char *directory;
{
    int		  *list;
    int		   count, ok;
    char	  *name;
    DIR           *dirp;
    struct dirent *dp;

    if(directory == NULL)
	return;
    dirp = opendir(directory);
    if(dirp == NULL)
        return; 
    count = 0;
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
    {
        if ( strncmp(dp->d_name, "font", 4) != 0)
            continue;
	name = dp->d_name + 4;
	ok = 1;
	while (*name != '\0')
	{
	    if (*name < '0' || *name > '9')
	    {
		ok = 0;
		break;
	    }
	    name++;
	}
	if (ok)
	    count++;
    }
    if (count > *num)
    {
	if (which == SYSFONT)
	{
	    if (*num > 0 && sysFontList)
		releaseWithId("sysfont");
	    sysFontList = (int *)allocateWithId (sizeof(int) * (count + 1),
			 "sysfont");
	}
	else
	{
	    if (*num > 0 && userFontList)
		releaseWithId("userfont");
	    userFontList = (int *)allocateWithId (sizeof(int) * (count + 1),
			 "userfont");
	}
    }
    if (which == SYSFONT)
        list = sysFontList;
    else
	list = userFontList;
    if (list == NULL)
    {
	*num = 0;
	closedir(dirp);
	return;
    }
    rewinddir(dirp);
    count = 0;
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
    {
        if ( strncmp(dp->d_name, "font", 4) != 0)
            continue;
	name = dp->d_name + 4;
	ok = 1;
	while (*name != '\0')
	{
	    if (*name < '0' || *name > '9')
	    {
		ok = 0;
		break;
	    }
	    name++;
	}
	if (ok)
	{
	    list[count] = atoi(dp->d_name + 4);
	    count++;
	}
    }
    *num = count;
    closedir(dirp);
}


void
open_var_font(fname, index, which, size)
  char  *fname;
  int    index, which, size;
{
     int   ok, k, bytes, n;
     char *pixdata, fontData[82];
     FILE *fin;
     struct stat  fStatus;

     if (index >= FONTNUM)
        return;
     if (stat (fname, &fStatus) < 0)
        return;
     if ((fin = fopen(fname, "r")) == NULL)
        return;
     ok = 0;
     while ((pixdata = fgets(fontData, 80, fin)) != NULL)
     {
     	if (strncmp(fontData, "varian font", 11) == 0)
        {
	    ok = 1;
	    break;
	}
     }
     if ( !ok )
     {
	fclose(fin);
	return;
     }

     if (varFont[index]->data != NULL)
     {
        releaseWithId(varFont[index]->name);
        varFont[index]->data = NULL;
     }
     varFont[index]->order = 0;

     sprintf(varFont[index]->name, "font%d", index);
     k = fscanf(fin, "%d%d%d", &varFont[index]->width, &varFont[index]->height,
	       &varFont[index]->ascent);
     if (k != 3)
         ok = 0;
     else
     {
         k = fscanf(fin, "%d%d", &varFont[index]->start, &varFont[index]->last);
         if (k != 2)
             ok = 0;
     }
     if (ok)
     {
        bytes = (varFont[index]->width * varFont[index]->height + 7) / 8;
        k = bytes * (varFont[index]->last - varFont[index]->start + 1);
        if (k > 2 && k < fStatus.st_size)
        {
           varFont[index]->data = (char *) allocateWithId((size_t) (k + 2),
               varFont[index]->name);
           if (varFont[index]->data == NULL)
               ok = 0;
        }
        else
           ok = 0;
     }
     if ( !ok )
     {
         fclose(fin);
         return;
     }
     pixdata = varFont[index]->data;
     n = 0;
     for(;;)
     {
        *pixdata = (char) fgetc(fin);
        if ((unsigned char) (*pixdata) == 255)
            break;
        n++;
        if (n > k) {
           k = 0;
           break;
        }
     }
     while (k > 0)
     {
        *pixdata = (char) fgetc(fin);
        k--;
        pixdata++;
     }
     fclose(fin);
     varFont[index]->order = FONTNUM;
     varFont[index]->bytes = bytes;
     varFont[index]->size = size;
     varFont[index]->which = which;
     rasterFont = varFont[index];
     ycharpixels = rasterFont->height + 1;
     xcharpixels = rasterFont->width;
     raster_ascent = rasterFont->ascent;
     varFont[index]->mtime = fStatus.st_mtime;
     return;
}


int
clearPixmap()
{
/*
   if (curwin <= 0)
	return(0);
*/
   if (!useXFunc)
	return(0);
#ifdef MOTIF
   if (canvasPixmap)
   {
        normalmode();
        XSetForeground(xdisplay, pxm_gc, xblack);
        XFillRectangle(xdisplay, canvasPixmap, pxm_gc, 0, 0, backing_width,
			backing_height);
        XSetForeground(xdisplay, pxm_gc, xcolor);
        return(1);
   }
#endif
   return(0);
}


int
x_cursor(old_pos,new_pos)
int *old_pos;
int  new_pos;
{
  if(!old_pos) return 0;
  if (*old_pos != new_pos)
  {
    if (*old_pos>0)
    {
	xgrx = *old_pos;
	xgry = 1;
        sun_rdraw(0,mnumypnts-3);
    }
    if (new_pos>0)
    {
	xgrx = new_pos;
	xgry = 1;
        sun_rdraw(0,mnumypnts-3);
    }
    *old_pos = new_pos;
  }
   return(1);
}


int
y_cursor(old_pos,new_pos)
int *old_pos;
int  new_pos;
{
  int    len;
  if(!old_pos) return 0;

  len = mnumxpnts-right_edge-3;
  if (*old_pos != new_pos)
  {
    if (*old_pos>0)
    {
	xgrx = 1;
	xgry = *old_pos;
        sun_rdraw(len,0);
    }

    if (new_pos>0)
    {
	xgrx = 1;
	xgry = new_pos;
	sun_rdraw(len,0);
    }
    *old_pos = new_pos;
  }
   return(1);
}

//   which: 1 -> x cursor, vertical line
//   which: 2 -> y cursor, horizontal line
//   pos and startPt  are relative values to the frame's origin point
void
vj_crosshair(int which, int pos, int startPt, int length, char *info)
{
     vnmrj_dstring(JHCURSOR, which, pos, startPt, length, info);
}

void x_crosshair(pos,off1,off2, val1, yval, val2)
int pos, off1, off2, yval;
double   val1, val2;
{
#ifdef MOTIF
      int  k;
#endif
      int  x, y, xor, copy_flag;
      int  toVj;

      if (pos > 0)
          x_cross_len  =  mnumypnts - off1;
      if(yval) {
	int dec = getDscaleDecimal(0)+2;
        char str[16];
        sprintf(str, "%%.%df(%%.3g)",dec);
        sprintf(x_cross_info, str,val1,val2);
      } else {
	int dec = getDscaleDecimal(0)+2;
	char str[16];
        sprintf(str, "%%.%df",dec);
        sprintf(x_cross_info, str,val1);
      }
      if (!winDraw)
         return;
      copy_flag = 0;
      xor = xorflag;
      toVj = 0;
      if (xor && useXFunc)
         normalmode();
      if (y_cross_pos > 0 || x_cross_pos > 0) {
          copy_flag = 1;
          toVj = 1;
          if (useXFunc) {
#ifdef MOTIF
            if ( canvasPixmap )
                XCopyArea(xdisplay, canvasPixmap, xid, vnmr_gc, 0, 0,
                      graf_width - 1, graf_height - 1, orgx, orgy);
            color(CURSOR_COLOR);
            for (k = 0; k < 3; k++) {
                x = orgx + x_cursor_pos[k];
                y = orgy + y_cursor_pos[k];
                if (x > orgx) {
                   XDrawLine(xdisplay, xid, vnmr_gc, x, orgy + 1,
                        x, orgy+x_cursor_y2[k]);
                }
                if (y > orgy) {
                   XDrawLine(xdisplay, xid, vnmr_gc, orgx + 1, y,
                        orgx + y_cursor_x2[k], y);
                }
            }
#endif
         }
         else {
            // jvnmr_cmd(JCOPY);
         }
      }
      x_cross_pos = pos;
      y_cross_pos = y2_cross_pos;

      if (x_cross_pos > 0 || y_cross_pos > 0) {
         toVj = 1;
         if (useXFunc)
            color(YELLOW);
      }
      if (x_cross_pos > 0) {
        x = orgx + x_cross_pos;
        y = orgy + 4;
        if (useXFunc) {
#ifdef MOTIF
             XDrawLine(xdisplay, xid, vnmr_gc, x, y, x, orgy+x_cross_len);
             y = y + ycharpixels; 
             XDrawString(xdisplay, xid, vnmr_gc, x+4, y, x_cross_info, strlen(x_cross_info));
#endif
        }
        else {
             // vnmrj_dstring(JHCURSOR, 1, x, y, x_cross_len, x_cross_info);

             vj_crosshair(1, x, y, x_cross_len, x_cross_info);
        }
      }
      if (y_cross_pos > 0) {
        x = orgx + 4 + y_cross_len;
        y = mnumypnts - y_cross_pos + orgy;
        if (useXFunc) {
#ifdef MOTIF
            XDrawLine(xdisplay, xid, vnmr_gc, orgx + y_cross_off1, y, x, y);
            y = y + ycharpixels; 
            XDrawString(xdisplay, xid, vnmr_gc, x, y, y_cross_info, strlen(y_cross_info));
#endif
        }
        else {
             // vnmrj_dstring(JHCURSOR, 2, y_cross_off1, y, y_cross_len - y_cross_off1, y_cross_info);

             vj_crosshair(2, y_cross_off1, y, y_cross_len - y_cross_off1, y_cross_info);
        }
      }
      if (useXFunc) {
         if (xor)
            xormode();
      }
      else if (toVj) {
         if (xor)
             normalmode();
         color(YELLOW);
         vnmrj_dstring(JHCURSOR, 3, x_cross_pos, y_cross_pos, copy_flag, x_cross_info);
         if (xor)
            xormode();
      }
}

void y_crosshair(pos,off1,off2, val)
int pos, off1, off2;
double   val;
{
      int dec = getDscaleDecimal(1)+2;
      char str[16];
      sprintf(str, "%%.%df",dec);
      sprintf(y_cross_info, str,val);
      y2_cross_pos = pos;
      y_cross_off1 = off1;
      y_cross_len = off1 + off2;
}


//   which: 1 -> x cursor, vertical line
//   which: 2 -> y cursor, horizontal line
//   pos and startPt  are relative values to the frame's origin point (top-left)
void
vj_cursor(int which, int id, int pos, int startPt, int length)
{
    if (which > 2)
        return;
    if (pos > 0) {
        if (which == 2 && g_color == THRESH_COLOR)
            j4xp_func(THSCOLOR, which, id, g_color, 0);
        else
            j4xp_func(CSCOLOR, which, id, g_color, 0);
    }
    j6p_func(ICURSOR2, which, id, pos, startPt, length, 0);
}

/*************************************/
int dconi_x_rast_cursor(int *old_pos, int new_pos, int off1, int off2, int num)
/*************************************/
{
  int  i, len;

    if (newCursor && num > 2)
       return(0);
    len = mnumypnts-off1;
    if (!useXFunc) {
        /**
         if (xorflag)
            normalmode();
         color(CURSOR_COLOR);
         j4p_func(ICURSOR, 1, num, new_pos, len);
        **/
        /****
        if (new_pos > 0)
           j4xp_func(CSCOLOR, 1, num, g_color, 0);
        vj_dconi_cursor(1, num, new_pos, len);
        ****/

        vj_cursor(1, num, new_pos, 2, len);
        *old_pos = new_pos;
        return(0);
    }
    if (*old_pos == new_pos)
        return(1);

    if (!newCursor)
        xormode();
    else
        normalmode();
    color(CURSOR_COLOR);
    if (*old_pos>0)
    {   /* remove old cursor */
	gx1 = orgx + *old_pos;
	mx1 = *old_pos;
	if (newCursor)
	{
	   if (useXFunc) {
#ifdef MOTIF
	       if (canvasPixmap && winDraw)
                  XCopyArea(xdisplay, canvasPixmap, xid, vnmr_gc, mx1, 1,
                        1, len, gx1, orgy + 1);
#endif
           }
	   else
               jCopyArea(mx1, 0, 1, len);
           x_cursor_pos[num] = 0;
	}
	else
	{
	   if (useXFunc) {
#ifdef MOTIF
		if (winDraw)
		    XDrawLine(xdisplay, xid, vnmr_gc, gx1, orgy+1, gx1, orgy+len);
#endif
	   }
	   else
	        draw_jline(gx1, 4, gx1, len);
	}
    }
    if (new_pos>0)
    {
	gx1 = orgx + new_pos;
	gy1 = orgy + 1;
	gy2 = orgy + len;

	mx1 = new_pos;
	my1 = 1;
	my2 = len;
	if (newCursor)
	{
	   if (useXFunc) {
#ifdef MOTIF
	       if (winDraw)
	           XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx1, gy2);
#endif
	   }
	   else
	       draw_jline(gx1, gy1, gx1, gy2);
	   x_cursor_pos[num] = new_pos;
           x_cursor_y2[num] = len;
	   for(i = 0; i < 3; i++)
           {
		if (y_cursor_pos[i] > 0)
            	{
		    if (useXFunc) {
#ifdef MOTIF
			if (winDraw) {
                            XDrawLine(xdisplay, xid, vnmr_gc, orgx+1, orgy+y_cursor_pos[i],
                        	orgx+y_cursor_x2[i], orgy+y_cursor_pos[i]);
			}
#endif
		    }
		    else {
		        draw_jline(1, y_cursor_pos[i],
				y_cursor_x2[i], y_cursor_pos[i]);
		    }
            	}
           }
	}
	else
	{
	   if (useXFunc) {
#ifdef MOTIF
		if (winDraw)
		    XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx1, gy2);
#endif
	   }
	   else
		draw_jline(gx1, gy1, gx1, gy2);
	}
    }
    *old_pos = new_pos;
    if (!newCursor)
        normalmode();
    return(1);
}

int
dconi_x_cursor(old_pos,new_pos,num)
int *old_pos;
int  new_pos, num;
{
     dconi_x_rast_cursor(old_pos, new_pos, 10, 0, num);
     return(1);
}


/*************************************/
int dconi_y_rast_cursor(int *old_pos, int new_pos, int off1, int off2, int num)
/*************************************/
{
  int  i, len, yy_old, yy_new;

/*
   if (curwin <= 0)
	return(0);
*/

    if (newCursor && num > 2)
	return(0);
    if (!useXFunc) {
        len = off1+off2;
        i = 0;
        if (new_pos > 0) {
            // j4xp_func(CSCOLOR, 2, num, g_color, 0);
            i = mnumypnts - new_pos;
        }
        // vj_dconi_cursor(2, num, i, len);

        vj_cursor(2, num, i, 2, len);
        *old_pos = new_pos;
        return(1);
    }
  
    if (*old_pos == new_pos)
	return(1);

    if (!newCursor)
   	xormode();
    else
        normalmode();
    color(CURSOR_COLOR);
    len = off1+off2;
    gx1 = orgx + 1;
    gx2 = len + orgx;
    mx1 = 1;
    mx2 = len;
    if (*old_pos > 0)
    {   /* remove old cursor */
        yy_old = mnumypnts-*old_pos;
        gy1 = yy_old + orgy;
        my1 = yy_old;
	if (newCursor)
	{
	   if (useXFunc) {
#ifdef MOTIF
	      if (canvasPixmap && winDraw && yy_old >= 0)
	         XCopyArea(xdisplay, canvasPixmap, xid, vnmr_gc, 1, my1,
                	len, 1, gx1, gy1);
#endif
	   }
	   else
	       jCopyArea(0, my1, len+2, 1);
           y_cursor_pos[num] = 0;
        }
	else
	{
	   if (useXFunc) {
#ifdef MOTIF
		if (winDraw)
		   XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy1);
#endif
	   }
	   else
	        draw_jline(gx1, gy1, gx2, gy1);
	}
    }
    if (new_pos>0)
    {
        yy_new = mnumypnts-new_pos;
        gy1 = yy_new + orgy;
        my1 = yy_new;
	if (newCursor)
	{
	   if (useXFunc) {
#ifdef MOTIF
		if (winDraw)
		   XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy1);
#endif
	   }
	   else
	   	draw_jline(gx1, gy1, gx2, gy1);
           y_cursor_pos[num] = yy_new;
           y_cursor_x2[num] = len;
           for(i = 0; i < 3; i++)
           {
                if (x_cursor_pos[i] > 0)
                {
		     if (useXFunc) {
#ifdef MOTIF
			if (winDraw)
			   XDrawLine(xdisplay, xid, vnmr_gc, orgx+x_cursor_pos[i], orgy+1,
                        	orgx+x_cursor_pos[i], orgy+x_cursor_y2[i]);
#endif
		     }
		     else {
		   	draw_jline(orgx+x_cursor_pos[i], orgy+1,
                        	orgx+x_cursor_pos[i], orgy+x_cursor_y2[i]);
		     }
            	}
           }
        }
	else
	{
	     if (useXFunc) {
#ifdef MOTIF
		if (winDraw)
		   XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy1);
#endif
	     }
	     else
		draw_jline(gx1, gy1, gx2, gy1);
	}
    }
    *old_pos = new_pos;
    if (!newCursor)
       normalmode();
    return(1);
}

int
dconi_y_cursor(old_pos,new_pos, num)
int *old_pos;
int  new_pos, num;
{
    dconi_y_rast_cursor(old_pos, new_pos, mnumxpnts-right_edge, 0, num);
    return(1);
}


static void
set_global_win_info(num)
int	num;
{
#ifndef INTERACT
	curwin = num;
	P_setreal(GLOBAL, "curwin", (double) num, 0);
	P_setreal(GLOBAL, "curwin", (double) grid_rows, 2);
	P_setreal(GLOBAL, "curwin", (double) grid_cols, 3);
#endif 
}

int
vj_x_cursor(n, old_pos,new_pos, c)
int  n, *old_pos, new_pos, c;
{
   if (useXFunc) {
       xormode();
       color(c);
       x_cursor(old_pos,new_pos);
       normalmode();
       return(1);
   }
   if (new_pos > 0)
       j4xp_func(CSCOLOR, 1, n, c, 0);
   if(old_pos) *old_pos = new_pos;
   vj_dconi_cursor(1, n, new_pos, mnumypnts - 2);
   return (1);
}

int
vj_y_cursor(n, old_pos,new_pos, c)
int  n, *old_pos, new_pos, c;
{
   if (useXFunc) {
       xormode();
       color(c);
       y_cursor(old_pos, new_pos);
       normalmode();
       return(1);
   }
   if (new_pos > 0) {
       if (c == THRESH_COLOR)
           j4xp_func(THSCOLOR, 2, n, c, 0);
       else
           j4xp_func(CSCOLOR, 2, n, c, 0);
       vj_dconi_cursor(2, n, mnumypnts-new_pos-1, mnumxpnts - 4);
   }
   else
       vj_dconi_cursor(2, n, 0, 0);
   if(old_pos) *old_pos = new_pos;
   return (1);
}


int
vj_threshold_cursor(n, old_pos,new_pos, c)
int  n, *old_pos, new_pos, c;
{
   if (isPrintBg == 0)
       return vj_y_cursor(n, old_pos,new_pos, c);
   if (useXFunc || (new_pos < 1))
       return (1);
   color(c);
   amove(2, new_pos);
   rdraw(mnumxpnts - 4,0);
   return (1);
}

static void
setNewWin(newActive)
WIN_STRUCT *newActive;
{
	int	i;

        for (i = 0; i < grid_num; i++)
            win_dim[i].active = 0;
	for (i = 0; i < 3; i++)
	{
            x_cursor_pos[i] = 0;
            y_cursor_pos[i] = 0;
	}
	curActiveWin = newActive;
        if (newActive == NULL) {
            canvasPixmap = org_Pixmap;
            grid_num = 1;
            grid_rows = 1;
            grid_cols = 1;
            orgx = org_orgx;
            orgy = org_orgy;
            graf_width = win_width;
            graf_height = win_height;
            mnumxpnts = graf_width - XGAP;
            mnumypnts = graf_height - YGAP;
            set_win_num(1);
            set_win_offset(0, 0);
            set_global_win_info(1);
	    Wsetgraphicsdisplay("");
            if (canvasPixmap != 0 && bksflag)
                 xmapDraw = 1;
            else
                 xmapDraw = 0;
            sun_setdisplay();
            return;
        }
	newActive->active = 1;
	drawWinBorder(newActive);
        set_win_offset(newActive->x, newActive->y);
	curwin = newActive->frameId;
        set_global_win_info(newActive->frameId);
	sun_setdisplay();

	graf_width = newActive->width;
	graf_height = newActive->height;
        mnumxpnts = graf_width - XGAP;
        mnumypnts = graf_height - YGAP;
	orgx = newActive->x + org_orgx;
	orgy = newActive->y + org_orgy;
        if (curActiveWin->xmap != 0)
            canvasPixmap = curActiveWin->xmap;
        if (canvasPixmap != 0 && bksflag)
            xmapDraw = 1;
        else
            xmapDraw = 0;
}

void setWinGeom(int x, int y)
{
#ifndef INTERACT
    if (grid_num <= 1)
        return;
#endif
}


void setActiveWin(int x,  int y)
{
#ifndef INTERACT
    int    k;
    char   cmd[12];
    WIN_STRUCT  *newActive, *win;

    if (grid_num <= 1)
        return;
    k = 0;
    newActive = NULL;
    while (k < grid_num)
    {
       win = &win_dim[k];
       if (win->x <= x && (win->x + win->width) >= x)
       {
            if (win->y <= y && (win->y + win->height) >= y) {
                newActive = win;
                break;
            }
       }
       k++;
    }
    if (newActive == NULL || newActive == curActiveWin)
        return;

    sprintf(cmd, "jwin(%d)\n", newActive->frameId);
    execString(cmd);
#endif

}

static void set_row_col(int r, int c)
{
#ifndef INTERACT
    int  m, active_id;
    WIN_STRUCT *a;

    if (r <= 0)
	r = 1;
    if (c <= 0)
	c = 1;
    m = r * c;
    if (m > GRIDNUMMAX)
        return;
    grid_num = m;
    if (win_dim_ready == 0) {
        win_dim_ready = 1;
        for (m = 0; m < GRIDNUMMAX; m++) {
             a = &win_dim[m];
             a->xmap = 0;
             a->frameId = m + 1;
             a->width = 0;
             a->height = 0;
             a->old_width = 0;
             a->old_height = 0;
        }
    }
    else {
        for (m = 0; m < grid_num; m++)
             win_dim[m].active = 0;
    }
    if (curwin > grid_num)
        active_id = 1;
    else
        active_id = curwin;

    if (grid_rows != r || grid_cols != c)
    {
        grid_rows = r;
        grid_cols = c;
        if (grid_num <= 1) {
            setNewWin(NULL);
#ifdef MOTIF
            if (useXFunc) {
               if (winDraw)
                  XClearArea(xdisplay, xid,orgx,orgy,graf_width,graf_height,False);
               if (canvasPixmap)
               {
                 XSetForeground(xdisplay, pxm_gc, xblack);
                 XFillRectangle(xdisplay, canvasPixmap, pxm_gc, 0, 0,
                         graf_width, graf_height);
               }
            }
#endif
        }
        else {
            setGridGeom();
            copy_grid_windows();
            drawGridLines();
        }
        Wturnoff_mouse();
    }
    set_win_num(grid_num);
    if (grid_num > 1) {
        if (active_id != curwin)
            setActiveWin(win_dim[active_id].x, win_dim[active_id].y);
        else {
            setNewWin(&win_dim[active_id - 1]);
        }
    }
    else {
        canvasPixmap = org_Pixmap;
        set_global_win_info(1);
    }
#endif 
}

#ifdef INTERACT
static int is_small_window()
{
	return(1);
}
#endif 


int
setgrid(argc, argv)
int argc;
char *argv[];
{
    int  r, c;

#ifndef INTERACT
    if (argc < 2)
    {
	Werrprintf("usage: setgrid(rows, cols)");
	RETURN;
    }
    r = atoi(argv[1]);
    if (r > GRIDROWMAX || r < 0)
    {
	Werrprintf("setgrid: number of rows must be within 0 and %d ", GRIDROWMAX);
	RETURN;
    }
    if (r <= 0)
	r = 1;
    c = 1;
    if (argc > 2)
    {
	c = atoi(argv[2]);
	if (c > GRIDCOLMAX || c < 0)
	{
	    Werrprintf("setgrid: number if cols must be within 0 and %d", GRIDCOLMAX);
	    RETURN;
	}
    }
    if (c <= 0)
	c = 1;
    if ((r * c) > GRIDNUMMAX) {
	Werrprintf("setgrid: total number exceeded the maximun number %d.", GRIDNUMMAX);
	RETURN;
    }
    set_row_col(r, c);
#endif 
    RETURN;
}

int setwin(int argc, char *argv[], int retc, char *retv[])
{
#ifndef INTERACT
    int  r, c, m;

    if (argc < 2)
    {
	Werrprintf("usage: setwin(num) or setwin(rows, cols)");
	RETURN;
    }
    r = atoi(argv[1]);
    if ( r < 1 )
        r = 1;
    m = r;
    c = 1;
    if (argc > 2)
    {
        if ( r > grid_rows )
            r = grid_rows;
	c = atoi(argv[2]);
        if (c < 1)
            c = 1;
        if (c > grid_cols)
            c = grid_cols;
        m = (r - 1) * grid_cols + c;
    }
    if (m > grid_num || grid_num <= 1)
    {
        if (argc > 2)
            Werrprintf("setwin: win(%s,%s) does not exist.", argv[1], argv[2]);
        else
	    Werrprintf("setwin: win '%s' does not exist", argv[1]);
	RETURN;
    }
    setNewWin(&win_dim[m - 1]);
#endif 
    RETURN;
}

void
set_default_grid()
{
#ifndef INTERACT
    double  tmpval;
    int	    win, r, c;

    if (P_getreal(GLOBAL, "curwin", &tmpval, 1) < 0)
	return;
    win = (int) tmpval;
    if (win <= 0)
	return;
    if (P_getreal(GLOBAL, "curwin", &tmpval, 2) >= 0)
	r = (int) tmpval;
    else
	r = grid_rows;
    if (P_getreal(GLOBAL, "curwin", &tmpval, 3) >= 0)
	c = (int) tmpval;
    else
	c = grid_cols;
    if (r > GRIDROWMAX || r < 1)
	return;
    if (c > GRIDCOLMAX || c < 1)
	return;
    if (win != curwin || r != grid_rows || c != grid_cols)
    {
	set_row_col(r, c);
    }
#endif 
}

void
pre_processMouse(button, move, release, x, y)
int  button, move, release, x, y;
{
#ifndef INTERACT
    x = x - orgx;
    if (x < 0 || x > graf_width)
	return;
    y = y - orgy;
    if (y < 0 || y > graf_height)
	return;
    processMouse(button, move, release, x, y);
#endif 
}

int queryMouse() 
{
#ifdef MOTIF
	Window  rootWin, chWin;
	int	x, y;
	int	rx, ry;
	unsigned int	mask;

	if (useXFunc) {
	   if (XQueryPointer(xdisplay, xid, &rootWin, &chWin, &rx, &ry,
			&x, &y, &mask))
	    return (mask);
	}
#endif
	return (0);
}

static int gin_x, gin_y, gin_but, gin_event;

void jMouse(int button, int type, int x, int y)
{
	wait_gin = 0;
	gin_x = x;
	gin_y = y;
	gin_but = button;
	gin_event = type;
}

void csi_jMouse(int button, int type, int x, int y)
{
}

static void
getJevent(int etype, int button_ask) 
{
	int stop;

	wait_gin = 1;
	gin_but = 5;
	gin_event = 5;
        ginFunc = 1;
	if (button_ask == 0) {
            // j1p_func(JGIN, 1);
            j4p_func(JGIN, 1, 0, 0, 0);
	    while (wait_gin) {
	        read_jcmd();
	    }
            j1p_func(JGIN, 3);
	    return;
	}
	stop = 0;
        // j1p_func(JGIN, 2);
        j4p_func(JGIN, 2, etype, button_ask, 0);
	while (1) {
	    wait_gin = 1;
	    while (wait_gin) {
		read_jcmd();
	    }
	    switch (gin_event) {
		case 1: /* ButtonRelease */
		case 2: /* ButtonPress */
			if (etype == gin_event) {
			    if ((button_ask == gin_but)  ||
                                (button_ask == 4))
				stop = 1;
			}
			break;
		case 3: /* KeyPress */
			stop = 1;
			break;
	    }
	    if (stop)
		break;
	}
        j1p_func(JGIN, 3);  /* stop gin */
}

/*  for gin command  */
void
getMouse(int event_type, int button_ask, int *retX, int *retY,
         int *b1, int *b2, int *b3)
{
#ifdef MOTIF
	int	x, y;
	int	rx, ry;
	int	todo;
	Window  rootWin, chWin;
	XEvent  xev;
	unsigned int	mask;
#endif


	if (!useXFunc) {
	    getJevent(event_type, button_ask);
            *retX = gin_x;
            *retY = gin_y;
	    *b1 = 0;
	    *b2 = 0;
	    *b3 = 0;
	    if (gin_but == 1)
		*b1 = 1;
	    else if (gin_but == 2)
		*b2 = 1;
	    else if (gin_but == 3)
		*b3 = 1;
	    return;
	}

#ifdef MOTIF
        if (event_type == 1)
           event_type = ButtonRelease;
        else if (event_type == 2)
           event_type = ButtonPress;

	if (ginCursor == 0)
	    ginCursor = XCreateFontCursor(xdisplay, XC_hand2);
	if (button_ask == 0)
	{
	    if (!XQueryPointer(xdisplay, xid, &rootWin, &chWin, &rx, &ry,
			&x, &y, &mask))
	    {
		   x = -1;
		   y = -1;
	    }
	}
	else
	{
	    if (ginCursor != 0)
		  XDefineCursor(xdisplay, xid, ginCursor);
	    hourglass_on = 2;
	    mask = ButtonPressMask | ButtonReleaseMask;
	    todo = 0;
	    x = XGrabPointer(xdisplay, xid, True, mask, GrabModeAsync,
			GrabModeAsync, None, None, CurrentTime);
	    if (x == GrabSuccess) {
	        x = XGrabKeyboard(xdisplay, xid, False, GrabModeAsync,
			GrabModeAsync, CurrentTime);
	        if (x == GrabSuccess)
		    todo = 1;
	    }
	    x = 0;
	    while (todo)
            {
		XtNextEvent(&xev);
		switch (xev.type) {
		  case ButtonRelease:
		    		   if (event_type == ButtonRelease)
				   {
				      if ( (xev.xbutton.button == button_ask) ||
                                           (button_ask == 4) )
					x = 1;
				   }
				   break;
		  case ButtonPress:
		    		   if (event_type == ButtonPress)
				   {
				      if ( (xev.xbutton.button == button_ask) ||
                                           (button_ask == 4) )
					x = 1;
				   }
				   break;
		  case KeyPress:
				   x = 1;
				   break;
		}
		if (x)
		    todo = 0;
	    }
	    if (!XQueryPointer(xdisplay, xid, &rootWin, &chWin, &rx, &ry,
			&x, &y, &mask))
	    {
		x = -1;
		y = -1;
	    }
            XUngrabPointer(xdisplay, CurrentTime);
            XUngrabKeyboard(xdisplay, CurrentTime);
            if (xev.type == KeyPress)
            {
               mask = 0;
    	       if (event_type == ButtonRelease)
                  mask = Button1Mask | Button2Mask | Button3Mask;
            }
	}
/*
        if (curActiveWin)
	{
		
	    x = x - curActiveWin->x;
	    y = y - curActiveWin->y;
	}
*/
	if (x >= 0) x -= orgx;
	if (y >= 0) y -= orgy;
        *retX = x;
        *retY = y;
        *b1 = ((mask & Button1Mask) != 0);
        *b2 = ((mask & Button2Mask) != 0);
        *b3 = ((mask & Button3Mask) != 0);
#endif  /*  MOTIF */
}

static int
select_font_size()
{
	if (graf_height < (scrnHeight * 0.5))
	    return 1;
	if (graf_width < (scrnWidth * 0.5))
	    return 1;
	return 0;
}

static void
set_graphics_xfont()
{
#ifdef MOTIF
	FONT_REC  *flist;
	XFontStruct *nFontInfo;
	int	small_font;
	int	win_num;
	char	*res, attr[32];
	FONT_REC *add_graphics_font();

	if (!useXFunc)
	     return;
	small_font = select_font_size();
	win_num = (grid_rows - 1) * GRIDROWMAX + grid_cols;
	if (small_font == 0)
	    win_num = win_num + GRIDROWMAX * GRIDROWMAX + 1;
	flist = font_lists;
	while (flist != NULL)
	{
	    if (flist->id == win_num)
		break;
	    flist = flist->next;
	}
	if ((flist == NULL) || (flist->name == NULL))
	{
	    if (small_font)
	        strcpy(attr, "graphics*smallfont");
	    else
	        strcpy(attr, "graphics*largefont");
	    res = get_x_resource("Vnmr", attr);
            if (res == NULL) {
	        if (small_font)
	            strcpy(attr, "graphics*small1x1*font");
		else
	            strcpy(attr, "graphics*large1x1*font");
		res = get_x_resource("Vnmr", attr);
	    }
            if (res != NULL)
	        flist = add_graphics_font(res);
	}
	if ((flist == NULL) || (flist->name == NULL))
	    return;
	if ((flist->fstruct == NULL) || flist->new_name)
	{
	    if ((nFontInfo = XLoadQueryFont(xdisplay, flist->name))==NULL)
            {
                Werrprintf("Error: could not load font '%s'", flist->name);
	        return;
            }
	    if (flist->fstruct != NULL)
	    {
	    	XUnloadFont(xdisplay, flist->fstruct->fid);
	    	XFreeFontInfo(NULL, flist->fstruct, 1);
	    }
	    flist->fstruct = nFontInfo;
	}
	else
	    nFontInfo = flist->fstruct;
	curFontName = flist->name;
        xstruct = nFontInfo;
	flist->new_name = 0;
	x_font = nFontInfo->fid;
	char_ascent = nFontInfo->max_bounds.ascent;
        char_descent = nFontInfo->max_bounds.descent;
        char_width = nFontInfo->max_bounds.width;
        char_height = char_ascent + char_descent;
        xcharpixels = char_width;
        ycharpixels = char_height;
        XSetFont(xdisplay, vnmr_gc, x_font);
        XSetFont(xdisplay, pxm_gc, x_font);
#endif 
}


void
auto_switch_graphics_font()
{
	set_graphics_xfont();
}

#ifdef MOTIF
FONT_REC
#else
char
#endif
*add_graphics_font(fname)
char    *fname;
{
#ifdef MOTIF
	FONT_REC  *flist, *plist;
	int	  num, len;

	num = (grid_rows - 1) * GRIDROWMAX + grid_cols;
	if (select_font_size() <= 0) /* large font */
	    num = num + GRIDROWMAX * GRIDROWMAX + 1;
	flist = font_lists;
	while (flist != NULL)
	{
	    if (flist->id == num)
		break;
	    flist = flist->next;
	}
	if (flist == NULL)
	{
	    flist = (FONT_REC *) calloc(1, sizeof(FONT_REC));
	    flist->id = num;
	    flist->new_name = 1;
	    if (font_lists == NULL)
		font_lists = flist;
	    else
	    {
		plist = font_lists;
		while (plist->next != NULL)
		   plist = plist->next;
		plist->next = flist;
	    }
	}
	if (flist->name != NULL)
	{
	    if (strcmp(flist->name, fname) == 0)
		return(flist);
	}
	len = strlen(fname);
	if (len > flist->name_len)
	{
	    if (flist->name != NULL)
	       free(flist->name);
	    flist->name = (char *) malloc(len + 1);
	    flist->name_len = len;
	}
	flist->new_name = 1;
	strcpy(flist->name, fname);
	return (flist);
#else
	return (NULL);
#endif
}


int
setfont(argc, argv)
int argc;
char *argv[];
{
	int	  win_num;

        if (argc < 2)
        {
            Werrprintf("usage: setfont('fontname')");
            RETURN;
        }
	win_num = (grid_rows - 1) * GRIDROWMAX + grid_cols;
	if (select_font_size() <= 0)
	    win_num = win_num + GRIDROWMAX * GRIDROWMAX + 1;
	add_graphics_font(argv[1]);
	set_graphics_xfont();
	RETURN;
}

void
set_aipframe_draw(int on, XID xmap, int w, int h)
{
    aipFrameDraw = on;
    aipFrameMap = (int) xmap;
    aipFrameWidth = w;
    aipFrameHeight = h;
    if (!useXFunc) {
        if (aipFrameMap < 5) {
            aipFrameWidth = 0;
            aipFrameHeight = 0;
        }
        if (csidebug)
            fprintf(stderr, " set_aipframe %d  wh: %d %d \n", aipFrameMap, aipFrameWidth, aipFrameHeight);
        aip_j6p_func(IWINDOW, xmap, 0, 0, aipFrameWidth, aipFrameHeight, 0);
        // if (on && (w > 2 && h > 2))
        //     aip_j6p_func(ICLEAR, xmap, 0,0, w, h, 0);
    }
}


void drawLine(int flag, int x1, int y1, int x2, int y2,
	unsigned int line_width, char* style)
{

#ifdef MOTIF
    int line_style;
#endif
    
    if (aip_xid != org_xid)
        flag = 0;
    if (!useXFunc) {
    	my1 = aip_mnumypnts - y1 - 1;
    	my2 = aip_mnumypnts - y2 - 1;
	aip_j6p_func(IPLINE, flag, line_width, x1, my1, x2, my2);
        batchCount = 1;
	return;
    }
#ifdef MOTIF
    line_style = LineSolid;
/***
    if(strcmp(style, "LineSolid") == 0) line_style = LineSolid;
    if(strcmp(style, "LineOnOffDash") == 0) line_style = LineOnOffDash;
    if(strcmp(style, "LineDoubleDash") == 0) line_style = LineDoubleDash;
    if (flag == 0)
        XSetLineAttributes(xdisplay, vnmr_gc, line_width, line_style,
            CapNotLast, JoinRound);
    else if (flag == 1)
        XSetLineAttributes(xdisplay, pxm_gc, line_width, line_style,
            CapNotLast, JoinRound);
    else if (flag == 2)
        XSetLineAttributes(xdisplay, pxm2_gc, line_width, line_style,
        CapNotLast, JoinRound);
***/

    if (flag == 0 && winDraw) {
    	gx1 = x1 + orgx;
    	gy1 = mnumypnts - y1 - 1 + orgy;
    	gx2 = x2 + orgx;
    	gy2 = mnumypnts - y2 - 1 + orgy;
        XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy2);
    } else if(flag == 1 && canvasPixmap) {
    	mx1 = x1;
    	my1 = mnumypnts - y1 - 1;
    	mx2 = x2;
    	my2 = mnumypnts - y2 - 1;
        XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx2, my2);
    } else if(flag == 2 && canvasPixmap2) {
    	mx1 = x1;
    	my1 = mnumypnts - y1 - 1;
    	mx2 = x2;
    	my2 = mnumypnts - y2 - 1;
        XDrawLine(xdisplay, canvasPixmap2, pxm2_gc, mx1, my1, mx2, my2);
    }
/**
    if (flag == 0 && winDraw)
	XSetLineAttributes(xdisplay, vnmr_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
    if (flag == 1)
	XSetLineAttributes(xdisplay, pxm_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
    else if (flag == 2)
	XSetLineAttributes(xdisplay, pxm2_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
**/
#endif
} 

void drawString(int flag, int x, int y, int size, char* text, int length) 
{
    if (aip_xid != org_xid)
        flag = 0;
    if (!useXFunc) {
    	my1 = aip_mnumypnts - y - 1;
        // vnmrj_dstring(IPTEXT,flag, x, my1, g_color, text);
        aip_dstring(1, IPTEXT,flag, x, my1, g_color, text);
        if (csi_opened)
           aip_batchCount = 1;
        else
           batchCount = 1;
	return;
    }
#ifdef MOTIF
      if (winDraw && flag == 0) {
    	gx1 = x + orgx;
    	gy1 = mnumypnts - y - 1 + orgy;
  	if(size < 16) XDrawString(xdisplay, xid, vnmr_gc, gx1, gy1, text, length);
  	else XDrawString16(xdisplay, xid, vnmr_gc, gx1, gy1, (XChar2b*)text, length);
      } else if (canvasPixmap && flag == 1) {
    	mx1 = x;
    	my1 = mnumypnts - y - 1;
  	if(size < 16) XDrawString(xdisplay, canvasPixmap, pxm_gc, mx1, my1, text, length);
  	else XDrawString16(xdisplay, canvasPixmap, pxm_gc, mx1, my1, (XChar2b*)text, length);
      } else if (canvasPixmap2 && flag == 2) {
    	mx1 = x;
    	my1 = mnumypnts - y - 1;
  	if(size < 16) XDrawString(xdisplay, canvasPixmap2, pxm2_gc, mx1, my1, text, length);
  	else XDrawString16(xdisplay, canvasPixmap2, pxm2_gc, mx1, my1, (XChar2b*)text, length);
      }
#endif
}

void drawRectangle(int flag, int x, int y, unsigned int width, unsigned int hight,  
	unsigned int line_width, char* style)
{
    int x1, y1;
#ifdef MOTIF
    int line_style;
#endif
   
    if (aip_xid != org_xid)
        flag = 0;
    if (!useXFunc) {
    	x1 = x - width / 2;
        y1 = aip_mnumypnts - y - 1 - hight / 2;
	aip_j6p_func(JPRECT, flag, line_width, x1, y1, width, hight);
        if (csi_opened)
           aip_batchCount = 1;
        else
           batchCount = 1;
	return;
    }
#ifdef MOTIF

    width *= 0.5;
    hight *= 0.5;
    line_style = LineSolid;
/*
    if(strcmp(style, "LineSolid") == 0) line_style = LineSolid;
    if(strcmp(style, "LineOnOffDash") == 0) line_style = LineOnOffDash;
    if(strcmp(style, "LineDoubleDash") == 0) line_style = LineDoubleDash;
    if(winDraw && flag == 0)
    	XSetLineAttributes(xdisplay, vnmr_gc, line_width, line_style,
        	CapNotLast, JoinRound);
    else if(canvasPixmap2 && flag == 2)
    	XSetLineAttributes(xdisplay, pxm_gc, line_width, line_style,
        	CapNotLast, JoinRound);
*/

    if(winDraw && flag == 0) {
    	gx1 = x + orgx + width;
    	gy1 = mnumypnts - y - 1 + orgy + hight;
        gx2 = x + orgx + width;
        gy2 = mnumypnts - y - 1 + orgy - hight;
        XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy2);

      	gx1 = gx2;
      	gy1 = gy2;
        gx2 = x + orgx - width;
        gy2 = mnumypnts - y - 1 + orgy - hight;
        XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy2);

      	gx1 = gx2;
      	gy1 = gy2;
        gx2 = x + orgx - width;
        gy2 = mnumypnts - y - 1 + orgy + hight;
        XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy2);

      	gx1 = gx2;
      	gy1 = gy2;
        gx2 = x + orgx + width;
        gy2 = mnumypnts - y - 1 + orgy + hight;
        XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy2);
/*
	XSetLineAttributes(xdisplay, vnmr_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
*/
    } else if(canvasPixmap && flag == 1) {
    	mx1 = x + width;
    	my1 = mnumypnts - y - 1 + hight;
        mx2 = x + width;
        my2 = mnumypnts - y - 1 - hight;
    	XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx2, my2);

    	mx1 = mx2;
    	my1 = my2;
    	mx2 = x - width;
    	my2 = mnumypnts - y - 1 - hight;
    	XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx2, my2);

    	mx1 = mx2;
    	my1 = my2;
    	mx2 = x - width;
    	my2 = mnumypnts - y - 1 + hight;
    	XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx2, my2);

    	mx1 = mx2;
    	my1 = my2;
    	mx2 = x + width;
    	my2 = mnumypnts - y - 1 + hight;
    	XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx2, my2);
/*
	XSetLineAttributes(xdisplay, pxm_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
*/
    } else if(canvasPixmap2 && flag == 2) {
/*
    	XSetLineAttributes(xdisplay, pxm2_gc, line_width, line_style,
        	CapNotLast, JoinRound);
*/
    	mx1 = x + width;
    	my1 = mnumypnts - y - 1 + hight;
        mx2 = x + width;
        my2 = mnumypnts - y - 1 - hight;
    	XDrawLine(xdisplay, canvasPixmap2, pxm2_gc, mx1, my1, mx2, my2);

	mx1 = mx2;
      	my1 = my2;
        mx2 = x - width;
        my2 = mnumypnts - y - 1 - hight;
    	XDrawLine(xdisplay, canvasPixmap2, pxm2_gc, mx1, my1, mx2, my2);

	mx1 = mx2;
      	my1 = my2;
        mx2 = x - width;
        my2 = mnumypnts - y - 1 + hight;
    	XDrawLine(xdisplay, canvasPixmap2, pxm2_gc, mx1, my1, mx2, my2);

	mx1 = mx2;
      	my1 = my2;
        mx2 = x + width;
        my2 = mnumypnts - y - 1 + hight;
    	XDrawLine(xdisplay, canvasPixmap2, pxm2_gc, mx1, my1, mx2, my2);
/*
	XSetLineAttributes(xdisplay, pxm2_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
*/
    } 
#endif  /*  MOTIF */
} 

void drawCircle(int flag, int x, int y, unsigned int size, 
	int angle1, int angle2, unsigned int line_width,  char* style)
{
#ifdef MOTIF
    int line_style;
#endif
    unsigned int width = size;
    unsigned int height = size;

    x -= 0.5*width; 
    y += 0.5*height;
    if (aip_xid != org_xid)
        flag = 0;
    if (!useXFunc) {
    	my1 = aip_mnumypnts - y - 1;
	aip_j8p_func(JPARC, flag, line_width, x, my1, width, height, angle1, angle2);
        batchCount = 1;
	return;
    }
#ifdef MOTIF
    line_style = LineSolid;
/**
    if(strcmp(style, "LineSolid") == 0) line_style = LineSolid;
    if(strcmp(style, "LineOnOffDash") == 0) line_style = LineOnOffDash;
    if(strcmp(style, "LineDoubleDash") == 0) line_style = LineDoubleDash;
**/

    if (winDraw && flag == 0) {
/*
    	XSetLineAttributes(xdisplay, vnmr_gc, line_width, line_style,
        	CapNotLast, JoinRound);
*/
    	gx1 = x + orgx;
    	gy1 = mnumypnts - y - 1 + orgy;

        XDrawArc(xdisplay, xid, vnmr_gc, gx1, gy1, width, height, angle1, angle2);
/*
	XSetLineAttributes(xdisplay, vnmr_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
*/
    } else if (canvasPixmap && flag == 1) {
/*
    	XSetLineAttributes(xdisplay, pxm_gc, line_width, line_style,
        	CapNotLast, JoinRound);
*/
    	mx1 = x;
    	my1 = mnumypnts - y - 1;

        XDrawArc(xdisplay, canvasPixmap, pxm_gc, mx1, my1, width, height, angle1, angle2);
/*
	XSetLineAttributes(xdisplay, pxm_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
*/
    } else if (canvasPixmap2 && flag == 2) {
/*
    	XSetLineAttributes(xdisplay, pxm2_gc, line_width, line_style,
        	CapNotLast, JoinRound);
*/
        mx1 = x;
        my1 = mnumypnts - y - 1;

        XDrawArc(xdisplay, canvasPixmap2, pxm2_gc, mx1, my1, width, height, angle1, angle2);
/*
	XSetLineAttributes(xdisplay, pxm2_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
*/
    }

#endif
} 

void fillPolygon(int flag, float2* pts, int num, char* s, char* m)
{
#ifdef MOTIF
    int i;
    int shape;
    int mode;
#endif

    if (aip_xid != org_xid)
        flag = 0;
    if (!useXFunc) {
	aip_Polygon_3(flag, pts, num);
        batchCount = 1;
        aip_batchCount = 1;
	return;
    }
#ifdef MOTIF
    if (num > xpoints_len) {
	xpoints_len = num;
        if (xpoints != NULL)
	   free(xpoints);
	xpoints = (XPoint*)malloc(sizeof(XPoint)*num);
        if (xpoints == NULL) {
           xpoints_len = 0;
           return;
        }
    }

    shape = Convex;
    mode = CoordModeOrigin;
    if(strcmp(s, "Convex") == 0) shape = Convex;
    if(strcmp(s, "Nonconvex") == 0) shape = Nonconvex;
    if(strcmp(m, "CoordModeOrigin") == 0) mode = CoordModeOrigin;
    if(strcmp(m, "CoordModePrevious") == 0) mode = CoordModePrevious;

    if (winDraw && flag == 0) {

      for(i=0; i<num; i++) {
	xpoints[i].x = (int)pts[i][0] + orgx;
	xpoints[i].y = mnumypnts - (int)pts[i][1] - 1 + orgy;
      }
      XFillPolygon(xdisplay, xid, vnmr_gc, xpoints, num, shape, mode);
    } else if (canvasPixmap && flag == 1) {

      for(i=0; i<num; i++) {
	xpoints[i].x = (int)pts[i][0];
	xpoints[i].y = mnumypnts - (int)pts[i][1] - 1;
      }
      XFillPolygon(xdisplay, canvasPixmap, pxm_gc, xpoints, num, shape, mode);
    } else if (canvasPixmap2 && flag == 2) {

      for(i=0; i<num; i++) {
	xpoints[i].x = (int)pts[i][0];
	xpoints[i].y = mnumypnts - (int)pts[i][1] - 1;
      }
      XFillPolygon(xdisplay, canvasPixmap2, pxm2_gc, xpoints, num, shape, mode);
    }
#endif
} 

void drawCross(int flag, int x, int y, unsigned int size,
        unsigned int line_width, char* style)
{
    int i;
#ifdef MOTIF
    int line_style;
#endif

    if (aip_xid != org_xid)
        flag = 0;
    if (!useXFunc) {
	i = size / 2;
    	mx1 = x - i;
    	my1 = aip_mnumypnts - y - 1 - i;
	aip_j6p_func(JCROSS, flag, line_width, mx1, my1, size, size);
        batchCount = 1;
        aip_batchCount = 1;
	return;
    }
#ifdef MOTIF
    if(strcmp(style, "LineSolid") == 0) line_style = LineSolid;
    if(strcmp(style, "LineOnOffDash") == 0) line_style = LineOnOffDash;
    if(strcmp(style, "LineDoubleDash") == 0) line_style = LineDoubleDash;

    size*= 0.5;

    if (winDraw && flag == 0) {
/*
    	XSetLineAttributes(xdisplay, vnmr_gc, line_width, line_style,
        	CapNotLast, JoinRound);
*/
    	gx1 = x + orgx + size;
    	gy1 = mnumypnts - y - 1 + orgy + size;
    	gx2 = x + orgx - size;
    	gy2 = mnumypnts - y - 1 + orgy - size;
        XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy2);

    	gx1 = x + orgx + size;
    	gy1 = mnumypnts - y - 1 + orgy - size;
    	gx2 = x + orgx - size;
    	gy2 = mnumypnts - y - 1 + orgy + size;
        XDrawLine(xdisplay, xid, vnmr_gc, gx1, gy1, gx2, gy2);
/*
	XSetLineAttributes(xdisplay, vnmr_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
*/
    } else if (canvasPixmap && flag == 1) {
/*
    	XSetLineAttributes(xdisplay, pxm_gc, line_width, line_style,
        	CapNotLast, JoinRound);
*/
    	mx1 = x + size;
    	my1 = mnumypnts - y - 1 + size;
    	mx2 = x - size;
    	my2 = mnumypnts - y - 1 - size;
        XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx2, my2);

    	mx1 = x + size;
    	my1 = mnumypnts - y - 1 - size;
    	mx2 = x - size;
    	my2 = mnumypnts - y - 1 + size;
        XDrawLine(xdisplay, canvasPixmap, pxm_gc, mx1, my1, mx2, my2);
/*
	XSetLineAttributes(xdisplay, pxm_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
*/
    } else if (canvasPixmap2 && flag == 2) {
/*
    	XSetLineAttributes(xdisplay, pxm2_gc, line_width, line_style,
        	CapNotLast, JoinRound);
*/
    	mx1 = x + size;
    	my1 = mnumypnts - y - 1 + size;
    	mx2 = x - size;
    	my2 = mnumypnts - y - 1 - size;
        XDrawLine(xdisplay, canvasPixmap2, pxm2_gc, mx1, my1, mx2, my2);

    	mx1 = x + size;
    	my1 = mnumypnts - y - 1 - size;
    	mx2 = x - size;
    	my2 = mnumypnts - y - 1 + size;
        XDrawLine(xdisplay, canvasPixmap2, pxm2_gc, mx1, my1, mx2, my2);
/*
	XSetLineAttributes(xdisplay, pxm2_gc, def_lineWidth, def_lineStyle,
        	def_capStyle, def_joinStyle);
*/
    }

#endif
}

int getWin_draw()
{
     return(winDraw);
}

#ifdef MOTIF
static Region pActiveMask = NULL; // Active area on pixmap
static Region cActiveMask = NULL; // Active area on canvas

#endif

static
int  get_xmap_id(XID oldId) {
     int  k;
     XID *p;

     if (useXFunc || oldId > 3)
         return (oldId);
     if (pixmap_array == NULL) {
         pixmap_size = 400;
         pixmap_index = 5;
         pixmap_array = (XID *) calloc(pixmap_size+1, sizeof(XID));
         if (pixmap_array == NULL)
             return (4);
     }
     if (pixmap_index >= pixmap_size)
         pixmap_index = 5;
     for (k = pixmap_index; k < pixmap_size; k++) {
         if (pixmap_array[k] == 0) {
              pixmap_array[k] = 1;
              pixmap_index = k;
              return (k);
         }
     }
     pixmap_index = pixmap_size;
     p = (XID *) calloc(pixmap_size+401, sizeof(XID));
     if (p == NULL) {
         pixmap_index = 5;
         return(pixmap_index);
     }
     for (k = 0; k <= pixmap_size; k++)
         p[k] = pixmap_array[k];
     free (pixmap_array);
     pixmap_size += 400;
     pixmap_array = p;
     pixmap_array[pixmap_index] = 1;
     return(pixmap_index);
}

static
void free_xmap_id(XID oldId) {
     int  k;

     if (useXFunc || pixmap_array == NULL)
         return;
     k = (int) oldId;
     if (k < 4)
         return;
     if (k < pixmap_size)
         pixmap_array[k] = 0;
     if (k < pixmap_index)
         pixmap_index = k;
     if (pixmap_index < 5) {
         if (isAipMovie == 0)
            pixmap_index = 5;
     }
}

void
aip_setCursor(type)
char *type;
{
   // vnmrj_dstring(ACURSOR, 0, 0, 0, g_color, type);
   aip_dstring(1, ACURSOR, 0, 0, 0, g_color, type);
}

static void
aip_setColor(int color)
{
     if (color < 0)
        return;
     aip_j6p_func(ICOLOR, color, 0, 0, 0, 0, 0);
}


void aip_clearRect(int x, int y, int w, int h)
{
    if (xdebug > 1)
        fprintf(stderr,"  clear rect %d xywh %d %d %d %d\n",  (int) aip_xid, x, y, w, h);

    if (csi_opened < 1)
        isNewGraph = 1;
    if (!useXFunc) {
        aipBatch(1);
        if (csidebug > 1)
           fprintf(stderr,"  aip_clearRect  %d:  %d %d %d %d \n", (int)aip_xid, x, y, w, h);
        aip_j6p_func(ICLEAR, aip_xid, x, y, w, h, 0);
        return;
    }
#ifdef MOTIF
    xcolor = sun_colors[BLACK];
    if (xmapDraw) {
        XSetForeground(xdisplay, pxm_gc, xcolor);
        XFillRectangle(xdisplay, canvasPixmap, pxm_gc, x, y, w, h);
    }
    if (aip_xid == org_xid) {
        if (!winDraw)
             return;
        x += orgx;
        y += orgy;
/*
        if ((x + w) > region_x2)
             w = region_x2 - x;
        if ((y + h) > region_y2)
             h = region_y2 - y;
*/
    }
    XSetForeground(xdisplay, aip_gc, xcolor);
    XFillRectangle(xdisplay, aip_xid, aip_gc, x, y, w, h);
#endif
}

void aip_drawRect(int x, int y, int w, int h, int color)
{
    if (xorflag)
        normalmode();
    if (xdebug > 2)
        fprintf(stderr,"  drawRect color %d   %d %d %d %d\n", color, x, y, w, h);
    if (plot != 0 && (iplotFd == NULL)) {
       if (raster < 3)
           return;
       if (color >= 0)
           ps_color(color, 0);
       ps_rect(x, mnumypnts - y - h, w, h);
       return;
    }
    if (!useXFunc) {
        aip_j6p_func(IRECT, aip_xid, color, x, y, w, h);
        batchCount = 1;
        aip_batchCount = 1;
        return;
    }
#ifdef MOTIF
    xcolor = sun_colors[BLACK+color];
    if (xmapDraw) {
        XSetForeground(xdisplay, pxm_gc, xcolor);
        XDrawRectangle(xdisplay, canvasPixmap, pxm_gc,  x, y, w, h);
    }
    if (aip_xid == org_xid) {
        if ( !winDraw )
            return;
        x += orgx;
        y += orgy;
    }
    XSetForeground(xdisplay, aip_gc, xcolor);
    XDrawRectangle(xdisplay, aip_xid, aip_gc, x, y, w, h);

#endif
}

void aip_drawOval(int x, int y, int w, int h, int color)
{
    if (xorflag)
        normalmode();
    if (xdebug > 2)
        fprintf(stderr,"  drawOval color %d   %d %d %d %d\n", color, x, y, w, h);
    if (!useXFunc) {
        aip_j6p_func(IOVAL, aip_xid, color, x, y, w, h);
        batchCount = 1;
        aip_batchCount = 1;
        return;
    }
#ifdef MOTIF
    xcolor = sun_colors[BLACK+color];
    if (xmapDraw) {
        XSetForeground(xdisplay, pxm_gc, xcolor);
        XDrawArc(xdisplay, canvasPixmap, pxm_gc,  x, y, w, h, 0, 360*64);
    }
    if (aip_xid == org_xid) {
        if ( !winDraw )
            return;
        x += orgx;
        y += orgy;
    }
    XSetForeground(xdisplay, aip_gc, xcolor);
    XDrawArc(xdisplay, aip_xid, aip_gc,  x, y, w, h, 0, 360*64);

#endif
}

void aip_drawLine(int x, int y, int x2, int y2, int color)
{
    if (xorflag)
        normalmode();
    if (xdebug > 2)
        fprintf(stderr,"  draw line color %d  %d %d %d %d\n", color, x, y, x2, y2);
    if (plot != 0 && (iplotFd == NULL)) {
       if (raster < 3)
           return;
       ps_flush();
       if (color >= 0)
           ps_color(color, 0);
       amove(x, mnumypnts - y);
       adraw(x2, mnumypnts - y2);
       ps_flush();
       return;
    }
    if (!useXFunc) {
        aip_j6p_func(ILINE,aip_xid, color, x, y, x2, y2);
        batchCount = 1;
        aip_batchCount = 1;
        return;
    }
#ifdef MOTIF
    xcolor = sun_colors[BLACK+color];
    if (xmapDraw) {
        XSetForeground(xdisplay, pxm_gc, xcolor);
        XDrawLine(xdisplay, canvasPixmap, pxm_gc, x, y, x2, y2);
    }
    if (aip_xid == org_xid) {
        if ( !winDraw )
           return;
        x += orgx;
        y += orgy;
        x2 += orgx;
        y2 += orgy;
    }
    XSetForeground(xdisplay, aip_gc, xcolor);
    XDrawLine(xdisplay, aip_xid, aip_gc, x, y, x2, y2);

#endif
}

void aip_drawPolyline(Dpoint_t *pnts, int npts, int color)
{
#ifdef MOTIF
    int x1, x2;
    int y1, y2;
    int dx, dy;
    int k;
    int p1, p2;
#endif

    if (xorflag)
        normalmode();
    if (xdebug > 2)
        fprintf(stderr,"  drawPolyline color %d  points %d \n", color, npts);

    if (plot != 0 && (iplotFd == NULL)) {
       if (raster < 3)
           return;
       if (color >= 0)
           ps_color(color, 0);
       ps_Dpolyline(pnts, npts);
       return;
    }

    if (!useXFunc) {
        aip_setColor(color);
        aip_Polyline(pnts, npts);
        batchCount = 1;
        aip_batchCount = 1;
        return;
    }
#ifdef MOTIF
    xcolor = sun_colors[BLACK+color];
    p1 = 0;
    p2 = 0;
    if (xmapDraw) {
        XSetForeground(xdisplay, pxm_gc, xcolor);
        p1 = 1;
    }
    if (aip_xid == org_xid) {
        if (winDraw)
              p2 = 1;
        dx = orgx;
        dy = orgy;
    }
    else {
        p2 = 1;
        dx = 0;
        dy = 0;
    }
    if (p2)
        XSetForeground(xdisplay, aip_gc, xcolor);
    x1 = (int) pnts[0].x;
    y1 = (int) pnts[0].y;
    for (k = 1; k < npts; k++) {
        x2 = (int) pnts[k].x;
        y2 = (int) pnts[k].y;
        if (p1)
            XDrawLine(xdisplay, canvasPixmap, pxm_gc, x1, y1, x2, y2);
        if (p2)
            XDrawLine(xdisplay, aip_xid, aip_gc, x1+dx, y1+dy, x2+dx, y2+dy);
        x1 = x2;
        y1 = y2;
    }

#endif
}

void aip_fillPolygon(Gpoint_t *pnts, int npts, int color)
{
#ifdef MOTIF
    int k;
#endif

    if (xorflag)
        normalmode();
    if (xdebug > 2)
        fprintf(stderr,"  fill Polygon color %d  points %d \n", color, npts);
    if (plot != 0 && (iplotFd == NULL)) {
       if (raster < 3)
           return;
       if (color >= 0)
           ps_color(color, 0);
       ps_fillPolygon(pnts, npts);
       return;
    }

    if (!useXFunc) {
        aip_setColor(color);
        aip_Polygon_2(pnts, npts);
        batchCount = 1;
        aip_batchCount = 1;
        return;
    }
#ifdef MOTIF
    if (npts > xpoints_len) {
        xpoints_len = npts;
        if (xpoints != NULL)
            free(xpoints);
        xpoints = (XPoint*)malloc(sizeof(XPoint)*npts);
        if (xpoints == NULL) {
           xpoints_len = 0;
           return;
        }
    }
    xcolor = sun_colors[BLACK+color];
    for (k = 0; k < npts; k++) {
        xpoints[k].x = (int) pnts[k].x;
        xpoints[k].y = (int) pnts[k].y;
    }
    if (xmapDraw) {
        XSetForeground(xdisplay, pxm_gc, xcolor);
        XFillPolygon(xdisplay, canvasPixmap, pxm_gc, xpoints, npts,
                 Complex, CoordModeOrigin);
    } 
    XSetForeground(xdisplay, aip_gc, xcolor);
    if (aip_xid == org_xid) {
        if ( !winDraw )
             return;
        for (k = 0; k < npts; k++) {
            xpoints[k].x = xpoints[k].x + orgx;
            xpoints[k].y = xpoints[k].y + orgy;
        }
    } 
    XFillPolygon(xdisplay, aip_xid, aip_gc, xpoints, npts,
                 Complex, CoordModeOrigin);

#endif
}

void aip_loadFont(int size)
{
    if (!useXFunc) {
        return;
    }
#ifdef MOTIF

#endif
}

void aip_getTextExtents(str, size, ascent, descent, width)
char  *str;
int   size, *ascent, *descent, *width;
{
    *ascent = char_ascent;
    *descent = char_descent;
    *width = get_text_width(str);
}

// if color is negative integer, keep previous set color
void aip_drawString(char *str, int x, int y, int clear, int color)
{
    if (xorflag)
        normalmode();
    if (plot != 0 && (iplotFd == NULL)) {
        if (clear || plotfile == NULL ||raster < 3)
           return;
        ps_color(color, 0);
        y = mnumypnts - y;
        if (y < 0)
           y = 0;
        amove(x, y);
        dstring(str);
        return;
    }
    if (clear) {
        // Clear this rectangle
        int w = get_text_width(str);
        aip_clearRect(x - 1, y - char_ascent -1, w + 4, char_height + 2);
    }
    if (!useXFunc) {
        //  aip_setColor(color);
        //  vnmrj_dstring(ITEXT, 0, x, y, color, str);
        aip_dstring(1, ITEXT, 0, x, y, color, str);
        batchCount = 1;
        aip_batchCount = 1;
        return;
    }
#ifdef MOTIF
    xcolor = sun_colors[BLACK+color];
    if (xmapDraw) {
         XSetForeground(xdisplay, pxm_gc, xcolor);
        XDrawString(xdisplay, canvasPixmap, pxm_gc, x, y, str, strlen(str));
    }
    if (aip_xid == org_xid) {
        if ( !winDraw )
           return;
        x += orgx;
        y += orgy;
    }
    XSetForeground(xdisplay, aip_gc, xcolor);
    XDrawString(xdisplay, aip_xid, aip_gc, x, y, str, strlen(str));
#endif
}

void aip_drawVString(char *str, int x, int y, int color) {
    if (xorflag)
        normalmode();
    if (plot != 0 && (iplotFd == NULL)) {
        if (plotfile == NULL ||raster < 3)
           return;
        ps_color(color, 0);
        y = mnumypnts - y;
        if (y < 0)
            y = 0;
        amove(x, y);
        dvstring(str);
        return;
    }
    if (!useXFunc) {
        aip_setColor(color);
        //  vnmrj_dstring(ITEXT, 0, x, y, color, str);
        aip_dstring(1, IVTEXT, 0, x, y, color, str);
        batchCount = 1;
        aip_batchCount = 1;
        return;
    }
}


void set_graph_region(int x, int y, int wd, int ht)
{
    if (!useXFunc) {
        j6p_xfunc(JREGION, 1, x, y, wd-1, ht-1, 0);
        return;
    }
#ifdef MOTIF

    if (wd==0 && ht==0) {
        // No clipping on pixmap
        XSetClipMask(xdisplay, pxm_gc, None);
        if (aip_xid == org_xid && xregion != NULL) {
            XSetRegion (xdisplay, vnmr_gc, xregion);
        }
    } else {
        // Clip pixmap to specified rect
        XRectangle rectangle;
        rectangle.width = wd;
        rectangle.height = ht;
        if (aip_xid == org_xid) {
            rectangle.x = x;
            rectangle.y = y;
            if (pActiveMask != NULL)
                XDestroyRegion(pActiveMask);
            pActiveMask = XCreateRegion();
            XUnionRectWithRegion(&rectangle, pActiveMask, pActiveMask);
            XSetRegion(xdisplay, pxm_gc, pActiveMask);
        }

        // Clip canvas to offset rect
        rectangle.x = x + orgx;
        rectangle.y = y + orgy;
        if (cActiveMask != NULL)
            XDestroyRegion(cActiveMask);
        cActiveMask = XCreateRegion();
        XUnionRectWithRegion(&rectangle, cActiveMask, cActiveMask);
        if (aip_xid == org_xid && xregion != NULL) {
            XIntersectRegion(cActiveMask, xregion, cActiveMask);
            XSetRegion(xdisplay, vnmr_gc, cActiveMask);
        }
        // XSetRegion(xdisplay, vnmr_gc, cActiveMask);
    }

#endif
}

void aip_setClipRectangle(int x, int y, int wd, int ht)
{
    if (wd > 0)
        aipRegion = 1; 
    else
        aipRegion = 0; 
    if (!useXFunc)
        aip_j6p_func(JREGION, 0, x, y, wd, ht, 0);
    else
        set_graph_region(x, y, wd, ht);
}

void aip_setRegion(int x, int y, int wd, int ht)
{
    if (aipRegion) {
        return;
    }
    if (!useXFunc)
        aip_j6p_func(JREGION, 0, x, y, wd, ht, 0);
    else
        set_graph_region(x, y, wd, ht);
}

void aip_freeBackupPixmap(XID pixmapId)
{
    if (xdebug)
       fprintf(stderr,"  free backup pixmap %d \n", (int) pixmapId);
    free_xmap_id(pixmapId);
    if (!useXFunc) {
        aip_j6p_func(IFREEBK, pixmapId, 0, 0, 0, 0, 0);
        return;
    }
#ifdef MOTIF
    if (pixmapId != 0)
        XFreePixmap(xdisplay, pixmapId);
#endif
}


XID aip_allocateBackupPixmap(XID oldPixmap,int w,int h)
{
    int  n;
    Pixmap px;

    if (xdebug)
       fprintf(stderr," replace backup pixmap %d, wh  %d  %d \n",(int) oldPixmap, w, h);
    if (!useXFunc) {
        // n = get_xmap_id(0);
        if (oldPixmap != 0)
            aip_freeBackupPixmap(oldPixmap);
        n = get_xmap_id(0);
        if (xdebug)
           fprintf(stderr,"  new backup pixmap %d \n", n);
        if (n == aipFrameMap) {
           if (w > aipFrameWidth || h > aipFrameHeight) {
               if (csidebug)
                   fprintf(stderr,"  aip map %d  wh: %d %d \n", n, w, h);
               aip_j6p_func(IBACKUP, n, w, h, 0, 0, 0);
               aipFrameWidth = w;
               aipFrameHeight = h;
           }
        }
        else {
           if (csidebug)
               fprintf(stderr,"  new aip map %d  wh: %d %d \n", n, w, h);
           aip_j6p_func(IBACKUP, n, w, h, 0, 0, 0);
        }
        px = (Pixmap) n;
        return(px);
    }
    px = 0;
#ifdef MOTIF
    px = XCreatePixmap(xdisplay, xid, w, h, n_gplanes);
    /**
    if (oldPixmap != NULL && px != NULL)
        XCopyArea(xdisplay,oldPixmap, px, pxm_gc, 0,0,w, h, 0, 0);
    **/
    if (oldPixmap != 0)
        XFreePixmap(xdisplay, oldPixmap);
    if (px != 0) {
        XSetForeground(xdisplay, pxm_gc, xblack);
        XFillRectangle(xdisplay, px, pxm_gc, 0, 0, w, h);
    }
    if (xdebug)
        fprintf(stderr,"  new backup pixmap %d \n", (int) px);
#endif
    return (px);
}


XID aip_setDrawable(XID id)
{
    XID oldId = aip_xid;

    aip_xid = id;
    if (active_frame <= 0)
        set_jframe_id(1);
    if (id == org_xid) {
        vnmr_gc = org_gc;
        aip_gc = org_gc;
        xid = org_xid;
        orgx = org_orgx;
        orgy = org_orgy;
        canvasPixmap = org_Pixmap;
    }
    else {
        vnmr_gc = pxm_gc;
        aip_gc = pxm_gc;
        xid = id;
        orgx = 0;
        orgy = 0;
        canvasPixmap = 0;
    }
    if (csidebug)
         fprintf(stderr, " set_aipmap %d  wh: 0 0 \n", (int) id);
    if (!useXFunc)
        aip_j6p_func(IWINDOW, id, 0, 0, 0, 0, 0);
    return oldId;
}

void aip_copyImage(XID src, XID dst,int xs,int ys,int w,int h,int xd,int yd)
{
#ifdef MOTIF
    GC gc;
#endif

    if (csidebug > 1) {
        fprintf(stderr," copy image  %d -> %d:  %d %d %d %d -> %d %d\n",
                (int) src, (int) dst, xs, ys,w, h,xd,yd);
    }
    if (csi_opened < 1)
        isNewGraph = 1;
    if (!useXFunc) {
        aip_j8p_func(ICOPY, src, dst, xs, ys, w, h, xd, yd);
        batchCount = 1;
        return;
    }
#ifdef MOTIF
    if (dst == 0) {
        dst = xid;
/*
        if (canvasPixmap && bksflag) {
            XCopyArea(xdisplay, src, canvasPixmap, pxm_gc,
                      xs, ys, w, h, xd, yd);
        }
*/
    }
    if (dst == xid) {
        if (xmapDraw)
            XCopyArea(xdisplay, src, canvasPixmap, pxm_gc,
                      xs, ys, w, h, xd, yd);
        if ( !winDraw )
            return;
        xd += orgx;
        yd += orgy;
        gc = vnmr_gc;
    }
    else
        gc = pxm_gc;
    XCopyArea(xdisplay, src, dst, gc, xs, ys, w, h, xd, yd);
#endif
}

void aip_initGrayscaleCMS(int n)
{
    palette_t entry;

    if (paletteList == NULL) {
        setup_aip();
        if (paletteList == NULL)
            return;
    }
    if (paletteSize <= n)
        return;

    entry = paletteList[n];
    int firstColor = entry.firstColor;
    int numColors = entry.numColors;
#ifdef MOTIF
    int i, gray;
    int end = firstColor + numColors;
#endif

    aipGrayMap = 1;
    if (!useXFunc) {
        aip_j6p_func(IGRAYMAP, firstColor, numColors, 0, 0, 0, 0);
        return;
    }
#ifdef MOTIF
    for (i = firstColor; i < end; i++) {
        gray = (255 * (i - firstColor)) / (numColors - 1);
        show_color(i, gray, gray, gray);
    }
#endif
}

void aip_movieCmd(char *cmd, const char *fname,  int x, int y, int w, int h)
{
    if (cmd == NULL)
       return;
    if (strcmp(cmd, "start") == 0) { 
        if (fname == NULL)
           return;
        // vnmrj_dstring(JMOVIE_START, x, y, w, h, fname);
        aip_dstring(1, JMOVIE_START, x, y, w, h, fname);
        return;
    }
    if (strcmp(cmd, "next") == 0) { 
        // jvnmr_cmd(JMOVIE_NEXT);
        aip_cmd(JMOVIE_NEXT);
        return;
    }
    if (strcmp(cmd, "done") == 0) { 
        // jvnmr_cmd(JMOVIE_END);
        aip_cmd(JMOVIE_END);
        return;
    }
}

Pixmap aip_displayImage(unsigned char *data, int colormapID, int transparency, int x, int y, int w, int h, bool keep_pixmap)
{
    int  r, k, dx, dy;
    int  nw, nh, nid;
    int  xdraw; 
    Pixmap xmap;

    if (csidebug)
       fprintf(stderr, " display image  %d %d %d %d\n", x, y, w, h);
    if (csi_opened < 1)
        isNewGraph = 1;
    aipBatch(1);
    if (xorflag)
       normalmode();
    if (aipGrayMap == 0)
       aip_initGrayscaleCMS(GRAYSCALE_COLORS);
    if (active_frame <= 0)
        set_jframe_id(1);
    xmap = 0;
    nw = w + 20;
    nh = h + 10;
    if (keep_pixmap) {
        k = 0;
/*
        if (aipFrameMap == 0)
            k = 1;
        else {
            if (aipFrameWidth < w || aipFrameHeight < h)
                k = 1;
            else if (aipFrameWidth > nw * 2 || aipFrameHeight > nh * 2)
                k = 1;
        }
        if (k) {
            if (aipFrameMap != 0)
                aip_freeBackupPixmap(aipFrameMap);
            nid = get_xmap_id(0);
            aipFrameMap = (XID) nid;
        }
        else
            nid = (int) aipFrameMap;
*/
        if (isAipMovie) {
             xmap = 4;
             nid = 4;
             aipFrameWidth = 0;
        }
        else {
            if (aipFrameMap == 0 || aipFrameMap == 4) {
                nid = get_xmap_id(0);
                aipFrameMap = (XID) nid;
            }
            else {
                nid = (int) aipFrameMap;
            }
            xmap = aipFrameMap;
        }
        if (!useXFunc) {
            if (csidebug)
                fprintf(stderr, "  draw aip image %d  colormap %d -> %d\n", nid,curPalette, colormapID);
            if (w > aipFrameWidth || h > aipFrameHeight) {
                aip_j6p_func(IBACKUP, nid, w, h, 0, 0, 0);
                aipFrameWidth = w;
                aipFrameHeight = h;
            }
            if (colormapID >= 0) {
                if (curPalette != colormapID) {
                   if (xdebug || csidebug)
                      fprintf(stderr, "set color map %d \n", colormapID);
                   aip_j4p_func(SETCOLORMAP, colormapID, nid, 0, 0);
                   curPalette = colormapID;
                   curTransparence = -1;
                }
            }
            if (curTransparence != transparency) {
                if (xdebug || csidebug)
                   fprintf(stderr, "set transparency %d \n", transparency);
                curTransparence = transparency;
                aip_j4p_func(AIP_TRANSPARENT, transparency, nid, 0, 0);
            }
            aip_j6p_func(IWINDOW, nid, x, y, 0, 0, 0);
        }
    }
#ifdef MOTIF
    if (useXFunc && keep_pixmap) {
        if (aipFrameMap != 0) {
            k = 0;
            if (aipFrameWidth < w || aipFrameHeight < h)
                k = 1;
            else if (aipFrameWidth > nw * 2 || aipFrameHeight > nh * 2)
                k = 1;
            if (k) {
                XFreePixmap(xdisplay, aipFrameMap);
                aipFrameMap = 0;
            }
        }
        xmap = aipFrameMap;
        if (xmap == 0) {
            xmap = XCreatePixmap(xdisplay, xid, nw, nh, n_gplanes);
        }
        if (xmap != 0) {
            XSetForeground(xdisplay, pxm_gc, xblack);
            XFillRectangle(xdisplay, xmap, pxm_gc, 0, 0, nw, nh);
        }
        if (xdebug) {
            fprintf(stderr, "   new pixmap %d \n", (int) xmap);
        }
    }
#endif
    xdraw = winDraw;
    if (xmap != 0) {
        dx = x;
        dy = y;
       /*********
        if (x > 0 || y > 0) {  // overlay image?
           if (x < w && y < h) {
               dx = x;
               dy = y;
           }
        }
       *********/
        winDraw = 0;
    }
    else {
        if (!winDraw)
            return(xmap);
        dx = x;
        dy = y;
    }
    dcon_thTop = 0;
    dcon_thBot = 0;
    k = 0;
    jRaster = IRASTER;
    for (r = 0; r < h; r++) {
        x_rast(1, data+k, w, 1, dx, dy, sun_colors, xmap);
        k += w;
        dy++;
    }
    winDraw = xdraw;
    jRaster = JRASTER;
    dcon_thTop = org_dcon_thTop;
    dcon_thBot = org_dcon_thBot;
#ifdef MOTIF
    if (xmap != 0 && winDraw) {
        if (useXFunc && aipFrameDraw <= 0)
        {
               XCopyArea(xdisplay, xmap, xid, vnmr_gc, 0, 0, w, h, x+orgx, y+orgy);
        }
    }
#endif
    if (!useXFunc && keep_pixmap) {
        //j6p_xfunc(IWINDOW, aip_xid, 0, 0, 0, 0, 0);
        // aip_j6p_func(IWINDOW, aip_xid, 0, 0, 0, 0, 0);
    }
    return (xmap);
}


void
toggle_graphics_debug(int s, int csi)
{
    if (s < 1) {
       // if (xdebug == 0)
       //    xdebug = 1;
       // else
          xdebug = 0;
    }
    else
       xdebug = s;
    if (csi < 1)
       csidebug = 0;
    else
       csidebug = csi;
}

/***
#ifdef MOTIF
void aip_setCanvasMask(Region region)
{
    if (!useXFunc) {
        return;
    }
    if (visibleMask == NULL)
        visibleMask = XCreateRegion();
    else {
        if (XEqualRegion(region, visibleMask)) {
            return;             // Nothing to do
        }
    }
    XUnionRegion(region, region, visibleMask); // Update visibleMask
    if (cDrawMask == NULL)
        cDrawMask = XCreateRegion();
    if (useActiveMask) {
        XIntersectRegion(region, cActiveMask, cDrawMask);
    } else {
        XUnionRegion(region, region, cDrawMask);
    }
    XSetRegion (xdisplay, vnmr_gc, cDrawMask);
    //  NB: No resize/redraw necessary
}
#endif
***/

void
copy_to_pixmap2(x, y, x2, y2, w, h)
int     x, y, x2, y2, w, h;
{
    if (csidebug > 1)
       fprintf(stderr," copy_to_pixmap2  1 -> 2: %d %d %d %d -> %d %d\n", x, y, w, h, x2, y2);
    if (!useXFunc)
    {
        aip_batchCount = 1;
        aip_j8p_func(ICOPY, 1, 2, x, y, w, h, x, y);
        return;
    }
    if (canvasPixmap && canvasPixmap2)
    {
#ifdef MOTIF
        if (orgx < 0 || xregion == NULL)
            return;
        normalmode();
        XCopyArea(xdisplay, canvasPixmap, canvasPixmap2, pxm2_gc, x, y, w, h, x,
 y);
#endif
    }
}

void
copy_from_pixmap2(x, y, x2, y2, w, h)
int     x, y, x2, y2, w, h;
{
    if (csidebug > 1)
       fprintf(stderr," copy_from_pixmap2  2 -> 1: %d %d  %d %d -> %d %d\n", x, y, w, h, x2, y2);
    if (!useXFunc)
    {
        aip_j8p_func(ICOPY, 2, 1, x, y, w, h, x, y);
        return;
    }
    if (canvasPixmap && canvasPixmap2)
    {
#ifdef MOTIF
        if (orgx < 0 || xregion == NULL)
            return;
        normalmode();
        XCopyArea(xdisplay, canvasPixmap2, canvasPixmap, pxm_gc, x, y, w, h, x,
y);
#endif
    }
}

void aip_removeTimeOut(XID tId)
{
    struct itimerval timeval;
    TIME_NODE  *tnode;

    if (!useXFunc) {
        timeval.it_value.tv_sec = 0;
        timeval.it_value.tv_usec = 0;
        timeval.it_interval.tv_sec = 0;
        timeval.it_interval.tv_usec = 0;
        setitimer(ITIMER_REAL,&timeval, NULL);
        tnode = timeOut_list;
        while (tnode != NULL) {
            if (tnode->id == tId) {
                tnode->active = 0;
                break;
            }
            tnode = tnode->next;
        }
        return;
    }
#ifdef MOTIF
    XtRemoveTimeOut(tId);
#endif
}


XtIntervalId aip_addTimeOut(unsigned long msec, void (*func)(), char *retPtr)
{
    int n;
    struct itimerval timeval;
    TIME_NODE  *tnode, *pnode;
    struct timezone tzone;
    struct timeval clock;

    if (!useXFunc) {
        tnode = timeOut_list;
        pnode = timeOut_list;
        n = 0;
        while (tnode != NULL) { // search for free node
            if (tnode->active == 0)
                break;
            pnode = tnode;
            tnode = tnode->next;
            n++;
        }
        if (tnode == NULL) {
            if (n > 10) {  // too many timers or forgot to turn off
                tnode = timeOut_list;
            }
        }
        if (tnode == NULL) {
            tnode = (TIME_NODE *) calloc(1, sizeof(TIME_NODE));
            if (tnode == NULL)
                return(0);
            tnode->next = NULL;
            tnode->retVal = NULL;
            tnode->valSize = 0;
            if (timeOut_list == NULL) {
                tnode->id = 1;
                timeOut_list = tnode;
            }
            else {
                tnode->id = pnode->id + 1;
                pnode->next = tnode;
            }
        }
        tnode->tv_sec = msec / 1000;
        tnode->tv_usec = (msec - tnode->tv_sec * 1000) * 1000;
        gettimeofday(&clock,&tzone);
        tnode->sec = clock.tv_sec;
        tnode->usec = clock.tv_usec;
        tnode->active = 1;
        tnode->func = func;
        if (retPtr != NULL) {
            if (strlen(retPtr) > tnode->valSize) {
                if (tnode->valSize > 0)
                    free(tnode->retVal);
                tnode->valSize = strlen(retPtr);
                tnode->retVal = (char *) malloc(tnode->valSize + 2);
            }
            strcpy(tnode->retVal, retPtr);
        }
        timeval.it_value.tv_sec = tnode->tv_sec;
        timeval.it_value.tv_usec = tnode->tv_usec;
        timeval.it_interval.tv_sec = 0;
        timeval.it_interval.tv_usec = 0;
        if (setitimer(ITIMER_REAL,&timeval, NULL) < 0)
            tnode->active = 0;
        return(tnode->id);
    }
#ifdef MOTIF
    return (XtAddTimeOut(msec, (XtTimerCallbackProc)func, retPtr));
#else
    return(0);
#endif
}

int is_vj_ready_for_movie(int reset)
{
    if (reset) {
        waitMovieDoneRet = 0;
        jx0p.code = htonl(IMG_SLIDES_END);
        graphToVnmrJ(&jx0p, intLen * 2);
        return(1);
    }
    if (waitMovieDoneRet > 0) {
        waitMovieDoneRet++;
        if (waitMovieDoneRet < 5)  // wait once
           return(0);
        waitMovieDoneRet = 0;
    }
    return(1);
}

void aip_movie_image(int on)
{
    // int oldData = isAipMovie;

    isAipMovie = on;
    if (on > 0) {
        waitMovieDoneRet = 1;
        jx0p.code = htonl(IMG_SLIDES_START);
        graphToVnmrJ(&jx0p, intLen * 2);
    }
    else {
        jBatch(0);
    }

    // if (oldData > 0 && on < 1)
    // if (on < 1)
    //    graph_batch(-9);
}

int getUseXFunc() {
   return useXFunc;
}

void set3Pmode(int m){
    aip_j6p_func(SET3PMODE, m, 0,0,0,0,0);
    //repaint_frames(1, 1);
}
void set3Pcursor(int i, int j, int x1, int y1, int x2, int y2){
    aip_j6p_func(SET3PCURSOR, i, j, x1, y1, x2, y2);
}

void
set_csi_opened(int on) {
   int wasOpened; 
   
   wasOpened = csi_opened;
   if (on < 1)
      csi_opened = 0;
   else
      csi_opened = 1;
   curPalette = 0;
   if (wasOpened == csi_opened)
      return;
   switch_aip_frame_mode(csi_opened);
   Jset_aip_mouse_mode(csi_opened);
   if (csi_opened) {
       aip_xcharpixels = csi_char_width;
       aip_ycharpixels = csi_char_height;
       aip_mnumxpnts = csi_win_width - 4;
       aip_mnumypnts = csi_win_height - 4;
       // iplan_set_win_size(csi_orgx, csi_orgy, csi_win_width, csi_win_height);
       set_aip_win_size(csi_orgx, csi_orgy, csi_win_width, csi_win_height);
   }
   else {
       aip_mnumxpnts = mnumxpnts;
       aip_mnumypnts = mnumypnts;
       // iplan_set_win_size(orgx, orgy, win_width, win_height);
       set_aip_win_size(orgx, orgy, main_win_width, main_win_height);
   }
   if (wasOpened < 1 && csi_opened > 0) {
      sprintf(tmpStr, "menu('aip')\n");
      execString(tmpStr);
  }
}

// open or close csi window
//   don't change func name, it is used by smagic.c
void
set_csi_mode(int on, char *orient) {
    int vertical = 0;

    if (orient[0] == '1' || orient[0] == 'v' || orient[0] == 'V')
        vertical = 1;
    j1p_func(ICSIORIENT, vertical);
    j1p_func(ICSIWINDOW, on);
    set_csi_opened(on);
}


// set csi window for graphics drawing
//   don't change func name, it is used by smagic.c
void
set_csi_display(int on) {
    if (csi_opened < 1)
       return;
    // j1p_func(ICSIDISP, on);
    if (on > 0 && csi_opened) {
       aip_j1p_func(JBGCOLOR, BACK);
       csi_active = 1;
       set_vj_font_size(csi_char_width, csi_char_height, csi_char_ascent, csi_char_descent);
       set_csi_opened(csi_opened);
    }
    else {
       set_vj_font_size(win_char_width, win_char_height, win_char_ascent, win_char_descent);
       if (csi_active > 0) {
          set_win_size(0, 0, main_win_width, main_win_height);
       }
       csi_active = 0;
    }
}

//   called by jFunc
//   from 0 to 1 (opaque to tranparent)
//   don't change func name, it is used by smagic.c
void
vj_set_transparency_level(int type, double n) {
    int v = (int)(n * 100.0);
    if (v > 100)
       v = 100;
    else if (v < 0)
       v = 0;

    translucent = v;

    sprintf(tmpStr, "aipSetTransparency('%d',", v);
    if (type == 0)
       strcat(tmpStr, "'sel')\n");
    else if (type == 1)
       strcat(tmpStr, "'dis')\n");
    else
       strcat(tmpStr, "'all')\n");
    execString(tmpStr);
}


// for both spectrum and lines
// 0: opaque,  1.0: transparent
void
set_transparency_level(double n) {
    int v = (int)(n * 100.0);
    if (v > 100)
       v = 100;
    else if (v < 0)
       v = 0;
    transparency_level = v;
    j1p_func(TRANSPARENT, v);
    if (csi_opened)
        aip_j1p_func(TRANSPARENT, v);
}

// for lines only
void
set_line_width(int thick) {
    if (penThick > 1)
        return;
    if (plot != 0 && (iplotFd == NULL)) {
       if (raster < 3)
           return;
       ps_linewidth(thick);
       return;
    }

    j1p_func(LINEWIDTH, thick);
    if (csi_opened)
        aip_j1p_func(LINEWIDTH, thick);
}

// for spectrum only
void
set_spectrum_width(int thick) {
    if (penThick > 1)
        return;
    j1p_func(SPECTRUMWIDTH, thick);
    if (csi_opened)
        aip_j1p_func(SPECTRUMWIDTH, thick);
}


// for both spectrum and lines
void
set_pen_width(int thick) {
    penThick = thick;
    j1p_func(PENTHICK, thick);
    if (csi_opened)
        aip_j1p_func(PENTHICK, thick);
}

// for spectrum only
void
set_spectrum_thickness(char *min, char *max, double ratio) {
    if (penThick > 1)
        return;
     if (min == NULL || max == NULL)
         return;
     aip_dstring(0, SPECTRUM_MIN, 1, 1, 1, 1, min);
     if (csi_opened)
         aip_dstring(1, SPECTRUM_MIN, 1, 1, 1, 1, min);

     aip_dstring(0, SPECTRUM_MAX, 2, 2, 2, 2, max);
     if (csi_opened)
          aip_dstring(1, SPECTRUM_MAX, 2, 2, 2, 2, max);

     sprintf(tmpStr, "%g", ratio);
     aip_dstring(0, SPECTRUM_RATIO, 3, 3, 3, 3, tmpStr);
     if (csi_opened)
         aip_dstring(1, SPECTRUM_RATIO, 3, 3, 3, 3, tmpStr);
}

// for lines only
void
set_line_thickness(const char *thick) {
     if (penThick > 1)
        return;
     if (thick == NULL)
         return;
     aip_dstring(0, LINE_THICK, 1, 1, 1, 1, thick);
     if (csi_opened)
         aip_dstring(1, LINE_THICK, 1, 1, 1, 1, thick);
}

void
set_graphics_font(const char *fontName) {
     WIN_STRUCT *frame;

     if (fontName == NULL)
         return;
     aip_dstring(0, GRAPH_FONT, 1, 1, 1, 1, fontName);
     if (csi_opened)
         aip_dstring(1, GRAPH_FONT, 1, 1, 1, 1, fontName);
     if (active_frame > 0) {
         frame = &frame_info[active_frame];
         if (set_frame_font(frame, fontName)) {
             if (plot == 0)
                 set_font_size_with_frame(frame);
            // set_vj_font_size(frame->fontW, frame->fontH, frame->ascent, frame->descent);
         }
     }
}

// set font locally.
void
preset_graphics_font(char *fontName) {
     WIN_STRUCT *frame;

     if (fontName == NULL)
         return;
     if (active_frame > 0) {
         frame = &frame_info[active_frame];
         if (set_frame_font(frame, fontName)) {
             if (plot == 0)
                 set_font_size_with_frame(frame);
         }
     }
}

int
is_aip_window_opened() {
   return (csi_opened);
}

void
save_aip_menu_name(char *name) {
   if (aip_frame_info != NULL)
      strcpy(aip_frame_info->menu, name);
}

extern char *realString(double d);
int isCSIMode(int argc, char *argv[], int retc, char *retv[]) {
  if(retc>0) retv[0] = realString((double)csi_opened);
  RETURN;
}

void
set_vj_color(char *colorName) {
     if (colorName == NULL)
         return;
     aip_dstring(0, GRAPH_COLOR, 1, 1, 1, 1, colorName);
     if (csi_opened)
         aip_dstring(1, GRAPH_COLOR, 1, 1, 1, 1, colorName);
}

// open colormap file and assign id
int
open_color_palette(char *name) {
    int n;
    n = load_color_palette(name);
    if (n < 0)
        n = load_color_palette(defaultMapDir);
    if (xdebug)
        fprintf(stderr, "open palette: %d  %s \n", n, name);
    // if (n > 0)
    //     aipSetGraphicsPalettes(paletteList);

    return (n);
}


// type 0: selected  1: displayed   2: all
// id is the aip id.
int
open_color_palette_from_vj(int type, int id, char *name) {
    int n = open_color_palette(name);

    if (id <= 0) {
        curPalette = 0;
        return(0);
    }
    sprintf(tmpStr, "aipSetColormap('%s',", name);
    if (type == 0)
       strcat(tmpStr, "'sel','sel')\n");
    else if (type == 1)
       strcat(tmpStr, "'dis','dis')\n");
    else
       strcat(tmpStr, "'all','all')\n");
    
    execString(tmpStr);

    return (n);
}


int
close_color_palette(char *name) {
       return (-1);
   /************
    int i;
    palette_t *entry;

    if (paletteList == NULL)
       return (-1);
    for (i = GRAYSCALE_COLORS; i < paletteSize; i++) {
        entry = &paletteList[i];
        if (entry->mapName != NULL) {
            if (strcmp(name, entry->mapName) == 0)
               break;
        }
        entry = NULL;
    }
    if (entry == NULL)
       return (-1);
    if (entry->mapName != NULL)
       free(entry->mapName);
    entry->mapName = NULL;
    if (entry->fileName != NULL)
       free(entry->fileName);
    entry->fileName = NULL;
    return (entry->id);
    ************/
}

int
close_color_palette_with_id(int id) {
   /***
    int i;
    palette_t *entry;

    if (paletteList == NULL)
       return (-1);
    for (i = GRAYSCALE_COLORS; i < paletteSize; i++) {
        entry = &paletteList[i];
        if (entry->id == id)
             break;
        entry = NULL;
    }
    if (entry == NULL)
       return (-1);
    if (entry->mapName != NULL)
       free(entry->mapName);
    entry->mapName = NULL;
    if (entry->fileName != NULL)
       free(entry->fileName);
    entry->fileName = NULL;
    ***/
    return (id);
}


// set image palette.
// the aipId is imgBackup.id
// if aipId is zero, then palette will apply to all images

int
set_color_palette_id(int aipId, int paletteId) {
    int i;
    palette_t *entry;

    if (paletteId < 0 || paletteList == NULL)
       return (-1);
    
    entry = NULL;
    for (i = 0; i < paletteSize; i++) {
        entry = &paletteList[i];
        if (entry->id == paletteId) {
            if (entry->mapName != NULL)
               break;
        }
        entry = NULL;
    }
    if (entry == NULL)
       return (-1);
    vj_set_color_palette(entry, aipId);
    return (paletteId); 
}

// bid is imgBackup.id

void set_aip_image_id(int bid, int id) {
    aip_j4p_func(AIPID, bid, id, 0, 0);
}

// order 0: clear all image info
// imgId 0: clear all image info
// order 1: the top-most image in UI list

void set_aip_image_info(imgId, order, mapId, transparency, imgName, selected) 
int imgId, order, mapId, transparency, selected;
char *imgName;
{
    if (order <= 0 || imgId <= 0) {
       imgName = tmpStr;
       strcpy(tmpStr, "none");
    }
    if (imgName == NULL || ((int)strlen(imgName) < 1))
       return;
    aip_dstring(1,SETCOLORINFO,imgId,order,mapId,transparency,imgName);
    if (selected)
       aip_j1p_func(SELECTCOLORINFO, imgId);
}

   // vj UI change the order of image from its list
void change_aip_image_order(int imgId, int order) {

}

   // vj UI select image from its list
void select_aip_image(int imgId, int order, int mapId) {
  curPalette = 0;
  aipSelectViewLayer(imgId,order,mapId);
  window_redisplay();
}

  // alpha 0 is fully transparent, 100 is fully opaque
void set_background_region(int x, int y, int w, int h, int color, int alpha) {
   j6p_func(BACK_REGION, x, y, w, h, color, alpha);
}

// turn on annotation frame
void set_top_frame_on() {
   isTopframeOn = 1;
   if (iplotFd != NULL)
       switch_iplot_file();
   j6p_func(JFRAME, ENABLE_TOP_FRAME, 1, 0, 0, 0, 0);
}

// turn off annotation frame
void set_top_frame_off() {
   isTopframeOn = 0;
   if (iplotFd != NULL)
       switch_iplot_file();
   j6p_func(JFRAME, ENABLE_TOP_FRAME, 0, 0, 0, 0, 0);
}

// clear annotation frame
void clear_top_frame() {
   if (iplotTopFd != NULL) {
       rewind(iplotTopFd);
       fprintf(iplotTopFd, "#endSetup\n");
   }
   j6p_func(JFRAME, CLEAR_TOP_FRAME, 0, 0, 0, 0, 0);
}

void set_top_frame_on_top(int onTop) {
   isTopOnTop = onTop;
   j6p_func(JFRAME, RAISE_TOP_FRAME, onTop, 0, 0, 0, 0);
}

 // the origin point (0, 0) is at top-left corner
void set_clip_region(int x, int y, int w, int h) {
   set_graph_region(x, y,  w, h);
}

 // create java table on canvas
 // color 0: use java default tabel color
 // the origin point (0, 0) is at top-left corner
void open_jTable(int table_id, int x, int y, int width, int height, int color,
                   char *tablefile) {
    if (tablefile == NULL)
        return;
    if (iplotFd != NULL) {
        xj_dstring(JTABLE, table_id, x, y, width, height, color, tablefile);
        return;
    }
    sprintf(tmpStr,"canvas table open id:%d x:%d y:%d w:%d h:%d color:%d file:%s ",
                    table_id, x, y, width, height, color, tablefile);
    writelineToVnmrJ("vnmrjcmd", tmpStr);
    isGraphFunc = 1;
}

void close_jTable(int table_id, int all) {
     if (all)
         sprintf(tmpStr, "canvas table closeall id:%d",table_id);
     else
         sprintf(tmpStr, "canvas table close id:%d",table_id);
     writelineToVnmrJ("vnmrjcmd", tmpStr);
}

int
jTable_changed(int argc, char *argv[] )
{
    int table_id, row;

    table_id = atoi(argv[1]);
    row = atoi(argv[2]);   // changed row number from 0 ... n
    // argv[3] .... are data of columns

    RETURN;
}

  // alpha 0 is fully transparent, 100 is fully opaque
void
set_rgb_alpha(int alpha)
{
   rgbAlpha = alpha;
   j1p_func(RGB_ALPHA, alpha);
}

int
get_rgb_alpha()
{
   return rgbAlpha;
}

 // the origin point (0, 0) is at top-left corner
void
draw_arrow(int x1, int y1, int x2, int y2, int thick, int color)
{
   if (plot != 0 && (iplotFd == NULL)) {
       if (raster < 3)
           return;
       if (color >= 0)
           ps_color(color, 0);
       ps_arrow(x1, mnumypnts - y1, x2, mnumypnts - y2, thick);
       return;
   }
   
   j6p_func(JARROW, x1, y1, x2, y2, thick, color);
}

 // the origin point (0, 0) is at top-left corner
void
draw_round_rect(int x1, int y1, int x2, int y2, int thick, int color)
{
   int n;
   int x0, y0;

   if (plot != 0 && (iplotFd == NULL)) {
       if (raster < 3)
           return;
       if (color >= 0)
           ps_color(color, 0);
       n = ps_linewidth(thick);
       if (x2 < x1) {
           x0 = x1;
           x1 = x2;
           x2 = x0;
       }
       y1 = mnumypnts - y1; 
       y2 = mnumypnts - y2; 
       if (y2 < y1) {
           y0 = y1;
           y1 = y2;
           y2 = y0;
       }
       ps_round_rect(x1, y1, x2 - x1, y2 - y1);
       ps_linewidth(n);
       return;
   }
   j6p_func(JROUNDRECT, x1, y1, x2, y2, thick, color);
}

 // the origin point (0, 0) is at top-left corner
void
draw_oval(int x1, int y1, int x2, int y2, int thick, int color)
{
   int n;
   int x0, y0;

   if (plot != 0 && (iplotFd == NULL)) {
       if (raster < 3)
           return;
       if (color >= 0)
           ps_color(color, 0);
       n = ps_linewidth(thick);
       if (x2 < x1) {
           x0 = x1;
           x1 = x2;
           x2 = x0;
       }
       y1 = mnumypnts - y1; 
       y2 = mnumypnts - y2; 
       if (y2 < y1) {
           y0 = y1;
           y1 = y2;
           y2 = y0;
       }
       ps_ellipse(x1, y1, x2 - x1, y2 - y1);
       ps_linewidth(n);
       return;
   }
   j6p_func(JOVAL, x1, y1, x2, y2, thick, color);
}

// fontName: Serif, SansSerif, Monospaced, Dialog, DialogInput.
// fontStyle: Plain, Bold, Italic, Bold+Italic
// fontSize:  >= 7
// fontColor:  null or color name
void
set_anno_font(const char *fontName, const char *fontStyle, const char *fontColor, int fontSize)
{
    if (plot != 0 && (iplotFd == NULL)) {
       if (raster < 3)
           return;
       
       return;
    }
    sprintf(tmpStr, "%s, %s, %s, %d", fontName, fontStyle, fontColor, fontSize);
    aip_dstring(1, ANNFONT, 0, 0, 0, 0, tmpStr);
}

void
set_anno_color(const char *colorName) {
    if (colorName == NULL)
       return;
    aip_dstring(1, ANNCOLOR, 0, 0, 0, 0, colorName);
}

// this is for testing
void set_ybar_style(int n) {
   char str[MAXPATH * 2];

   int x, y, w, h, d;

   // sun_sunGraphClear();
   if (n == 1) {
      x = mnumxpnts * 0.1;
      y = mnumypnts * 0.1;
      w = mnumxpnts * 0.3;
      h = mnumypnts * 0.6;
      sprintf(str, "%s/table.dat", userdir);
      open_jTable(1, x, y, w, h, 11, str);
      return;
   }
   if (n == 2) {
      x = mnumxpnts * 0.1;
      y = mnumypnts * 0.3;
      w = mnumxpnts * 0.2;
      h = mnumypnts * 0.2;
      d = mnumypnts / 20;
      draw_arrow(x, y, x, y - h, 1, 5);
      y = y + d;
      draw_arrow(x, y, x + h / 2, y - h / 2, 1, 5);
      y = y + d;
      draw_arrow(x, y, x + w, y , 1, 0);
      y = y + d;
      draw_arrow(x, y, x + h / 2, y + h / 2 , 1, 6);
      y = y + d;
      draw_arrow(x, y, x, y + h, 1, 0);

      x = mnumxpnts * 0.4;
      y = mnumypnts * 0.3;
      draw_arrow(x, y, x - h / 2, y - h / 2, 1, 9);

      y = y + d;
      draw_arrow(x, y, x - w, y, 1, 9);

      y = y + d;
      draw_arrow(x, y, x - h / 2, y + h / 2, 1, 9);

     set_rgb_alpha(60);
      x = mnumxpnts * 0.5;
      y = mnumypnts * 0.3;
      draw_arrow(x, y, x, y - h, 6, 5);
     set_rgb_alpha(100);

      y = y + d;
      draw_arrow(x, y, x + w, y - h / 2, 2, 5);

      y = y + d;
      draw_arrow(x, y, x + w, y, 2, 0);

      y = y + d;
      draw_arrow(x, y, x + w, y + h / 2, 2, 6);
     set_rgb_alpha(60);
      y = y + d;
      draw_arrow(x, y, x, y + h, 4, 6);
     set_rgb_alpha(100);
      return;
   }
   if (n == 3) {
      x = mnumxpnts * 0.1;
      y = mnumypnts * 0.1;
      w = mnumxpnts * 0.3;
      h = mnumypnts * 0.6;
      d = w / 8;

      draw_round_rect(x, y, x + w, y + h, 6, 5);
      set_rgb_alpha(60);
      x += d; y += d; w -= d * 2; h -= d * 2;
      draw_round_rect(x, y, x + w, y + h, 6, 5);
      set_rgb_alpha(40);
      x += d; y += d; w -= d * 2; h -= d * 2;
      draw_round_rect(x, y, x + w, y + h, 6, 5);
      x += d; y += d; w -= d * 2; h -= d * 2;
      set_anno_color("green");
      draw_round_rect(x, y, x + w, y + h, 1, -1);

      x = mnumxpnts * 0.5;
      y = mnumypnts * 0.1;
      w = mnumxpnts * 0.3;
      h = w;
      d = w / 4;

      set_rgb_alpha(100);
      draw_oval(x, y, x + w, y + h, 6, 9);
      set_rgb_alpha(60);
      x += d; y += d; w -= 20; h -= d;
      draw_oval(x, y, x + w, y + h, 6, 9);
      set_rgb_alpha(30);
      x += d; y += d; w -= 20; h -= d;
      draw_oval(x, y, x + w, y + h, 6, 9);
      set_rgb_alpha(100);
      return;
   }
   if (n == 4) {
      set_top_frame_on_top(0);
      return;
   }
   if (n == 5) {
      set_top_frame_on_top(1);
      return;
   }
   if (n == 6) {
      y = 30;
      set_anno_font("SansSerif", "Bold", "brown" , 18);
      amove(10, y);
      dstring(" SansSerif Bold 18  brown");
      set_anno_font("SansSerif", "Italic", "blue" , 22);
      y += 24;
      amove(20, y);
      dstring(" SansSerif Italic 22  blue");
      set_anno_font("Dialog", "Monospaced", "green" , 14);
      y += 24;
      amove(30, y);
      dstring(" Dialog Monospaced 14  green");
      set_anno_color("orange");
      y += 24;
      amove(40, y);
      set_anno_font("Serif", "Italic", "null" , 24);
      dstring(" Serif  Italic 24  null -> orange");
      set_anno_color("magenta");
      y = mnumypnts - 40;
      aip_drawString(" magenta color ", 300, y, 0, -1);
      return;
   }
   if (n == 7) {
       aip_drawString("string 14", 200, 400, 0, 18);
       aip_drawVString("vstring 12", 200, 400, 12);
       draw_oval(200, 0, 400, 400, 4, 12);
       draw_oval(400, 400, 800, 600, 4, 18);
      draw_round_rect(700, 300,  900, 0, 8, 5);
      draw_round_rect(920, 300,  1200, 100, 12, 6);
       aip_drawLine(200, 500, 800, 500, 5);
 aip_drawRect(1000, 0, 300, 200, 5);
      draw_arrow(1000, 0, 1200, 400, 2, 6);
      draw_arrow(1000, 60, 1200, 600, 8, 12);
      draw_arrow(1200, 0, 1600, 0, 8, 6);

      return;
   }
   if (n == 8) {
      draw_oval(200, 200, 600, 400, 4, 12);
      draw_oval(200, 400, 400, 800, 4, 32);
      // draw_round_rect(100, 400, 800, 1200, 8, 14);
   }
}
