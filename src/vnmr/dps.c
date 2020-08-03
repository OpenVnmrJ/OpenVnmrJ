/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "vnmrsys.h"
#include "graphics.h"
#include "dpsdef.h"
#include "vfilesys.h"
#include "pvars.h"
#include "wjunk.h"
#include "dps.h"
#include "group.h"
#include "symtab.h"
#include "tools.h"
#include "variables.h"
#include "REV_NUMS.h"
#include "allocate.h"
#include "dps_menu.icon"

extern Pixel get_vnmr_pixel_by_name();
extern void Wturnoff_mouse();
extern void getVnmrInfo(int okToSet, int okToSetSpin, int overRideSpin);
extern void P_endPipe(int fd);
extern void cleanup_pars();
extern void sun_dhstring(char *s, int x, int y);
extern void Wactivate_mouse();
extern void register_dismiss_proc(char *name, int (*func)());
extern void set_turnoff_routine(int (*funct)());
extern void Wprintf(char *format, ...);
extern int do_mkdir(char *dir, int psub, mode_t mode);
extern int run_calcdim();
extern int nvAcquisition();
extern int initacqqueue(int argc, char *argv[]);
extern int page();
extern int textWidth();
extern int setplotter();
extern int set_wait_child(int pid);
extern int acq(int argc, char *argv[], int retc, char *retv[]);
extern int P_sendGPVars(int tree, int group, int fd);
extern int P_sendVPVars(int tree, char *name, int fd);
extern int raster;
extern int graf_width, graf_height;
extern int char_ascent, raster_ascent;
extern int useXFunc; 
extern FILE *plot_file();

#ifdef MOTIF
extern GC  vnmr_gc;
#ifdef VNMRJ
extern GC  pxm_gc;
#endif 
extern Font    x_font;
extern XID     xid;
extern Display *xdisplay;
extern Pixmap  canvasPixmap;
extern Pixel   get_vnmr_pixel();
#endif 

#ifdef VNMRJ
extern void show_color(int num, int red, int grn, int blu);
extern void vnmrj_color_name(int index, char *s);
extern void graph_batch(int on);
extern void clear_pixmap(int x, int y, int w, int h);
extern void removeGraphFunc();
extern int clearPixmap();
extern int isJprintMode();
extern int is_vplot_session(int q);
extern Pixel  search_pixel();
extern Pixel  get_vnmr_pixel(int n);
extern void  dpsx(SRC_NODE *node);
extern void  dpsx_init(int imageType, int debug, char *seqName);
extern void  dpsx_setValues(int type, int num, int *values);
extern void  dpsx_setDoubles(int type, int num, double *values);
extern void  set_vj_color(char *colorName);
extern void  vj_dstring(char *s, int x, int y);
extern void set_background_region(int x, int y, int w, int h, int color, int alpha);
#endif 

#define    paramLen    32
#define    argLen      1024
// #define    GRAD_NUM    3
#define	   NSEC		1
#define    USEC 	2
#define    MSEC 	3
#define    SECD  	4
#define    ModuleMode 	0x01
#define    VgradMode 	0x02
#define    DevOnMode    0x04
#define    NegativeMode 0x10
#define    AMPBLANK     0x20
#define    PrgMode      0x40
#define    MODULE	1
#define    NVGRAD	3
#define    MOVESTEP	5
#define    XGAP		3

#define    NODOCK       -1
#define    DOCKFRONT    2
#define    DOCKMID      3
#define    DOCKEND      4

// definitions for something between parallelstart and parallelend
#define    PDOCK_A1     9
#define    PDOCK_A2     8
#define    PDOCK_A3     7
#define    PDOCK_A4     6
#define    PDOCK_A5     5
#define    PDOCK_MID    4
#define    PDOCK_B1     3
#define    PDOCK_B2     2
#define    PDOCK_B3     1
#define    PDOCK_LAST   0

#define    LABELEN	512
#define    GATEPNT	7
#define    LOOPNT	3
#define    ROTORPNT	7

#define    T_REAL   	1
#define    T_STRING 	2

#define    VXRC		1
#define    GEMINI	2
#define    GEMINIH	3
#define    GEMINIB	4
#define    GEMINIBH	5
#define    UNITY	6
#define    UNITYPLUS	7

static SRC_NODE   *src_start_node = NULL;
static SRC_NODE   *disp_start_node = NULL;
static SRC_NODE   *draw_start_node = NULL;
static SRC_NODE   *draw_end_node = NULL;
static SRC_NODE   *src_end_node = NULL;
static SRC_NODE   *hilitNode = NULL;
static SRC_NODE   *fid_node;
static SRC_NODE   *status_node[TOTALCH];
static SRC_NODE   *chOn_node[TOTALCH];
static SRC_NODE   *info_node[TOTALCH];
static SRC_NODE   *prg_node[TOTALCH];
static SRC_NODE   *sync_node[TOTALCH];
static SRC_NODE   *kz_node[TOTALCH];
static SRC_NODE   *spare_src_node = NULL;
static SRC_NODE   *angle_node = NULL;
static SRC_NODE   *parallel_start_node = NULL;
static DOCK_NODE  *dock_src_node = NULL;
static DOCK_NODE  *dock_start_node = NULL;
static ANGLE_NODE *angle_start_node = NULL;
static ANGLE_NODE *rotate_start_node = NULL;
static RFGRP_NODE *grp_start_node = NULL;
static PARALLEL_XNODE *parallel_x_list = NULL;
static SHAPE_NODE *shape_list = NULL;
static struct ybar *ybarData = NULL;
static double	  *dock_times;
static int	  *dock_pnts;
static int	  *dock_infos;
static int	  dock_time_len = 0;
static int	  dock_size;
static int	  isHomo = 0;
static int	  isNvpsg = 0;
static int	  ixNum = 1;
static int        firstDps = 1;
static int        whichTree = CURRENT;
static int        ybarSize = 0;
static int        advancedDps = 0;
static size_t     sizeT;

static AP_NODE  *apstart = NULL;
static ARRAY_NODE   *array_start_node = NULL;

#define OR	0
#define XOR	1
#define CLOSE	1
#define DRAW	2
#define FULL	3
#define MAGNIFY	4
#define SHRINK	5
#define EXPAND	6
#define LSHIFT	7
#define RSHIFT	8
#define PREV	9
#define NEXT    10
#define PLOT   11
#define STOPTIMER   12
#define RDRAW   13
#define REDO    14
#define PROPERTY  16
#define BROWSE  17
#define FASTER  18
#define SLOWER  19
#define STOPBR  20
#define CHANGESCAN    21
#define UP	1
#define DOWN	2
#define BUTNUM1   6
#define BUTNUM2   6

static char *but_label_1[] = {"Full", "+20%", "-20%", "Expand", "Plot", "Close" };
static int  but_val_1[BUTNUM1] = {FULL, MAGNIFY, SHRINK, EXPAND, PLOT, CLOSE};
#ifdef OLIT
static int  but_type_1[BUTNUM1] = {0, 0, 0, 0, 0, 0};
#endif

static char *but_label_2[] = {"->", "<-", "-Scan", "+Scan", "Redo","Property"};
static int  but_val_2[BUTNUM2] = {RSHIFT, LSHIFT, PREV, NEXT, REDO, PROPERTY};
#ifdef MOTIF
static int  but_type_2[BUTNUM2] = {1, 1, 2, 2, 0, 0};

static char *but_label_3[] = {"Faster", "Slower", "Stop"};
static int  but_val_3[3] = {FASTER, SLOWER, STOPBR};
static int  but_type_3[3] = {0, 0, 0};
#endif

static int	create_dps_info();
static int	measure_xpnt();
static int 	build_dps_node();
static int	execute_ps();
static void	disp_info();
static void	dps_config();
static void	draw_dps();
static void	draw_node();
static void	save_dps_resource();
static void 	read_dps_resource();
static double    get_exp_time();
static void	do_but_proc();
static void	do_class_proc();
static void	do_mode_proc();
static void     update_vj_array();
static void     link_parallel_node(SRC_NODE *node);
#ifndef VNMRJ
static void     popup_dps_pannel();
#endif 
static void     clear_dps_info();
static void     update_chan();
static void     disp_dps_time();
static int      write_array_opt();
static void     simulate_psg();
static void     dot_line();
static void     scrn_line();
static void     draw_pulse();
static void     draw_shape_pulse();
static void     draw_grpshaped();
static void     draw_gen_pulse();
static void     main_shaped();
static void     spin_scrn_line();
static void     fidWAVE();
static void     exec_transient();
#ifdef VNMRJ
static void     batch_draw();
static void     send_info_to_vj();
extern int      vj_x_cursor(int i, int *old_pos, int new_pos, int c);
#endif

static  Pixmap	backMap = 0;
static  Pixel   winBack;
#ifdef MOTIF
static  Pixel   winBg, winFg;
#ifndef VNMRJ
static  void  create_dps_pannel();
static void put_dps_info(char *data);
#endif
#endif
static  void  disp_new_psg();
static  void  disp_psg_name();
static  void  delay_mark();
static  void  draw_menu_icon();
static  void  clear_array_list();

static  int     close_dps_menu();
static  Window  shellWin = 0;
static  void    stop_dps_browse();
static  void	clean_dps_info_win();
#ifdef MOTIF
static  Widget  dpsShell = 0;
static  Widget  dpsMenu = 0;
static  Colormap cmap = 0;
static  Display *dpy = NULL;
static  GC	dps_text_gc = 0;
static  XrmDatabase  xbase = NULL;
static  XFontStruct *tfontInfo = NULL;
#endif
static  XFontStruct *dfontInfo = NULL;
#ifndef VNMRJ
static  void  recreate_array_index_widget();
#endif 

static  Widget  array_i_widgets[ARRAYMAX];
#ifdef MOTIF
static  Widget  array_t_widgets[ARRAYMAX];
static XmFontList  infoFontList = NULL;
#endif

static  int     pfx, pfy, chascent, chdescent;
#ifdef OLIT
static  GC	dps_but_gc = 0;
static  XFontStruct *bfontInfo = NULL;
static  int     but_h, but_w, but_ascent;
#endif
static  int     dch_h, dch_w;
static  int     dch_ascent = 12;
static  int     dch_descent = 4;
#ifdef MOTIF
static  int     text_h, text_w, text_ascent, text_descent;
#endif
static  int     x_margin, orgx_margin;
static  int  	lineGap;
static  int  	dpsWidth;
static  int     predvx[RFCHAN_NUM][5], predsx[RFCHAN_NUM][5];
static  int     gradsx[GRAD_NUM];
static  int     gradvx[GRAD_NUM];
static  int     pul_v_r, pul_s_r;  /* lines of pulse label */
static  int     dec_v_r, dec_s_r;  /* lines of dec label */
static  int     colorwindow = 0;
static  int  	mouse_x = -1;
static  int  	mouse_x2 = -1;
static  int  	mouse_y;
static  int  	statusY;
static  int  	delay_x1, delay_x2;
static  int  	dpsPlot, fidWidth;
static  int  	obsChan;
static  int  	noStatus;
static  int  	pulseHeight, gradHeight;
static  int  	pulseMin;
static  int	acqAct, exPnts;
static  int  	dBase, pBase, acqBase;
static  int  	dXBase, pXBase;
static  int     numch = 0;
static  int     numrfch = 1;
static  int  	cyclephase;
static  int  	rfchan[TOTALCH]; /* chanels exist in system */
static  int     activeCh[TOTALCH];
static  int     chanMap[TOTALCH];
static  int     chMod[TOTALCH];
static  int     ampOn[TOTALCH];
static  int     chx[TOTALCH];
static  int     chyc[TOTALCH];
static  int     orgy[TOTALCH];
static  int  	visibleCh[TOTALCH]; /* visible chanels */
static  int     RTindex[RTMAX];
static  int     RTval[RTMAX];
static  int     rfType[RFCHAN_NUM];
static  int     argIndex;
static  int     delMark[TOTALCH];
static  int	dispMode = TIMEMODE | DSPLABEL | DSPVALUE | DSPDECOUPLE;
                   /* label, value, timing, decoupling */
static  int	rfShapeType = ABS_SHAPE;
static  int	gradMin = 5;
static  int	menuUp = 0;
static  int	menu_x, menu_y;
static  int	spW, spH;
static  int	xOffset, xDiff;
static  int	transient, sys_nt;
static  int	dps_ver, dpsFine;
static  int	debug = 0;
static  int	jout, jline;
static  int	parallel, tbug;
static  int	newArray = 0;
static  int	arraydim = 1;
static  int	homodecp = 0;
static  int	image_flag = 0;
static  int	gmax;
static  int	mgmax;
static  int	in_browse_mode = 0;
static  int	ap_op_ok;
static  int	time_num;
static  int	tr_inc;
static  int	vjControl = 0;
#ifdef MOTIF
static  int	bWidth;
static  int	time_y, info_y, scan_y;
static  int	txHeight, txWidth;
static  int     backY;
#endif
static  int     timeFlag;
static	int	JFlag;
static  int     xorFlag = 0;
static  int     backW, backH;
#ifndef VNMRJ
static  int	line_num = 0;
static  int     backX;
#endif
static  int     menuW = 0;
static  int     menuH = 0;
static  int	menuX = 0;
static  int	menuY = 0;
static  int	loopW = 3;
static  int	loopH = 5;
static  int  	newAcq = 0;
static  int     id2, id3, id4;
static  int  	first_round = 1;
static  int  	showTimeMark = 1;
static  int     xwin = 1; 

/* dock info */
static  int     dockStart = 0;  /* the id of the first dock node */
static  int	dock_len = 0;
static  int     hasDock = 0;
static  int     hasIf = 0;
static  int     lastId = 0; /* the last dock id */
static  int     nodeId = NODOCK; /* the current node */
static  int     dockAt = NODOCK; /* the node to be docked */
static  int     dockFrom = NODOCK;  /* the position of dock */
static  int     dockTo = NODOCK;
static  int     rcvrsNum = 0;
static  int     cmd_error = 0;
static  int     parallelChan = 0;
static  int     inParallel = 0;
static  int     inLoop = 0;
static  int     isDispObj = 0;
static  int     hasTmpNode = 0;
static  int     *dock_N_array = NULL;
static  int     *dock_X_array = NULL;
static  int     sp_x[TOTALCH];
static  int     sp_y[TOTALCH];
static  char    *nodeName;
static SRC_NODE   **dock_list = NULL;
static SRC_NODE   **dock_tmp_list = NULL;
static SRC_NODE   *shgrad_node[GRAD_NUM];

#define MAX_RLLOOP 5
static  double  rlloopTime[MAX_RLLOOP+1];
static  int     rlloopMult[MAX_RLLOOP+1];
static  char    psgfile[MAXPATH];
static  char    dpsfile[MAXPATH];
static  char 	inputs[MAXPATH * 2];
static  char    *dps_label = NULL;
static  char    *cmd_name = NULL;
static  char    *argArray[64];
static  char    rcvrsStr[RFCHAN_NUM+4];
static  char    rcvrsType[RFCHAN_NUM+4];
static  char    dmstr[RFCHAN_NUM][paramLen];
static  char    dmmstr[RFCHAN_NUM][paramLen];
static  char    dnstr[RFCHAN_NUM][12];
static  char    gradtype[8];
static  char    gradshape[4];
static  char    dpscmd[12];
static  char    info_data[512];

static  double	expTime;
static  double	dMax, pMax;
static  double 	xRatio, xMult;
static  double  delayCnt, pulseCnt;
static  double  delayXCnt, pulseXCnt;
static  double  coarseInit[RFCHAN_NUM];
static  double  fineInit[RFCHAN_NUM];
static  double   powerCh[RFCHAN_NUM];
static  double   powerMax[RFCHAN_NUM];
static  double   coarseAttn[RFCHAN_NUM];
static  double   fineAttn[RFCHAN_NUM];
static  double   fineStep[RFCHAN_NUM];
static  double   phaseStep[RFCHAN_NUM];
static  double   RTdelay[RTDELAY];
static  double   rfPhase[RFCHAN_NUM];
static  double   offsetInit[RFCHAN_NUM]; /* freq offset */
static  double   frqOffset[RFCHAN_NUM];
static  double   exPhase[RFCHAN_NUM];
static  double   dmf_val[RFCHAN_NUM];
static  double   timeCh[TOTALCH];
static  double   parallel_time;
static  double   pplvl;
static  double   dlp, dhp;
static  double   cattn[RFCHAN_NUM];
static  double   fattn[RFCHAN_NUM];
static  double   gradMax[GRAD_NUM];
static  double   gradMaxLevel = 10.0;
static  double   gradMinLevel = 0.0;
static  double   minPw;
static  double   atVal;
static  double   minTime = 1.0e-8;
static  double   minTime2 = 1.0e-9;
static  double   rfShapeMax = 0.0;
static  double   rfShapeMin = 0.0;
static  double   gradShapeMax = 0.0;
static  double   gradShapeMin = 0.0;

/* the region of dps */
static  double	winX, winY, winW, winH;
static  double	plotX, plotY, plotW, plotH;
static  int	dpsX, dpsY, dpsX2, dpsY2;
static  int	dpsW, dpsH, frameW, frameH;

static  int	clearDraw, clearPlot; /* clean before drawing */
static  int	plotName = 1;
static  int	plotChanName = 1;
static  int	winChanName = 1;
static  char	*buttonFont, *dpsFont, *textFont;
static  char	*nbuttonFont, *ndpsFont, *ntextFont;


#define  BCOLOR		0   /* baseline color */
#define  PCOLOR		1   /* pulse color */
#define  SCOLOR		2   /* color for second */
#define  MCOLOR		3   /* color for millisecond */
#define  UCOLOR		4   /* color for microsecond */
#define  DCOLOR		5   /* delay color */
#define  LCOLOR		6   /* label color */
#define  PWCOLOR	7   /* power color */
#define  PHCOLOR	8   /* phase color */
#define  CHCOLOR	9   /* channel label color */
#define  STATCOLOR	10  /* status label color */
#define  ACQCOLOR	11  /* acquire color */
#define  ONCOLOR	12  /* on color */
#define  OFFCOLOR	13  /* off color */
#define  MRKCOLOR	14  /* mark color */
#define  HWLCOLOR	15  /* hw loop color */
#define  PELCOLOR	16  /* pe loop color */

#define  HCOLOR		17  /* highlight color */
#define  ICOLOR		18  /* info color */
#define  FCOLOR		19  /* function color */
#define  COLORS		20
#define  PRCOLOR	99  /* previuos color */

static  int	vjColor = 256-COLORS;  /* 256 - COLORS  */

static  Pixel   dpsPix[COLORS];
static  int	colorChanged[COLORS];

static  char    *dpsColorNames[COLORS] = { 
	        "DpsBase",   // BCOLOR
	        "DpsPulse",  // PCOLOR
	        "DpsS",
	        "DpsMS",
	        "DpsUS",
	        "DpsDelay",
	        "DpsLabel",
	        "DpsPwr",
	        "DpsPhs",
	        "DpsChannel",
	        "DpsStatus",
	        "DpsAcquire",
	        "DpsOn",
	        "DpsOff",
	        "DpsMark",
	        "DpsHWLoop",
	        "DpsPELoop",
	        "DpsSelected",
	        "DpsInfo",
	        "DpsFunc"
            };

static  char    *dpsColors[COLORS] = { 
	        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
            };
static  char    *defColors[] = {
	        "blue",       /* BCOLOR */
	        "green",      /* PCOLOR */
	        "white",      /* SCOLOR */
	        "gold",       /* MCOLOR */
			"cyan",       /* UCOLOR */ 
			"green",      /* DCOLOR */ 
			"yellow",     /* LCOLOR */ 
			"blue",       /* PWCOLOR */ 
			"red",        /* PHCOLOR */ 
			"yellow",     /* CHCOLOR */
			"yellow",     /* STATCOLOR */ 
			"orange",     /* ACQCOLOR */ 
			"yellow",     /* ONCOLOR */ 
			"white",      /* OFFCOLOR */ 
			"cyan",       /* MRKCOLOR */ 
			"orange",     /* HWLCOLOR */ 
			"yellow",     /* PELCOLOR */ 
	        "gray",       /* HCOLOR */
			"blue",       /* ICOLOR */ 
			"red",        /* FCOLOR */ 
			};

static char *ch_label[] = {"Tx", "Tx", "Dec", "Dec2", "Dec3", "Dec4", "dec5",
                           "Rcvr", "Null" };
static char *gradient_label[] = {"  X", "  Y", "  Z"};
static char *image_label[] = {"Gro", "Gpe", "Gss"};
static char *dm_labels[] = {"dm","dm","dm","dm2","dm3","dm4","dm5", "dm6",
                            "dm7" };
static char *dmm_labels[] = {"dmm","dmm","dmm","dmm2","dmm3","dmm4","dmm5",
                             "dmm6", "dmm7" };
static char *dmf_labels[] = {"dmf","dmf","dmf","dmf2","dmf3","dmf4","dmf5",
                             "dmf6", "dmf7" };
static char *dn_labels[] = {"tn","tn","dn","dn2","dn3","dn4","dn5", "dn6",
                             "dn7" };
static char *off_labels[] = {"tof","tof","dof","dof2","dof3","dof4","dof5",
                             "dof6", "dof7" };

static int lk_power_table[49] = {
         0,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  4,
         4,  4,  5,  6,  6,  7,  8,  9, 10, 11, 13, 14,
        16, 18, 20, 23, 25, 28, 32, 36, 40, 45, 51, 57,
        64, 72, 80, 90,101,113,128,143,161,180,202,227, 255};

static  char *def_fid_phase_label = "oph";
static  char *parallelSync = "parallelsync";
static  char *parallelStart = "parallelstart";
static  char *shapeDir = "shapelib";
static  char *sync_label = "sync";
static  char *fid_label;

static FILE  *fin = NULL;
static FILE  *jfout = NULL;
#ifdef VNMRJ
static FILE  *fdps = NULL;
#endif 

/*  See go.c, where programs and declarations with identical names are defined.

    One can remove the static keyword so that programs outside of the local file
    can access block_sigchld and restore_sigchld and then remove the duplicate
    from one file.  You also need to rework the program interface so the original
    signal mask becomes an argument to block_sigchld and restore_sigchld.	*/

static sigset_t origset;

static void
block_sigchld()
{
	sigset_t	nochld;

	sigemptyset( &nochld );
	sigaddset( &nochld, SIGCHLD );
	sigprocmask( SIG_BLOCK, &nochld, &origset );
}

static void
restore_sigchld()
{
	sigprocmask( SIG_SETMASK, &origset, NULL );
}


static jmp_buf		brokenpipe;
static struct sigaction	origpipe;

static void
sigpipe()
{
	longjmp( brokenpipe, -1 );
}

static void
catch_sigpipe()
{
	sigset_t		qmask;
	struct sigaction	newpipe;

	sigemptyset( &qmask );
	sigaddset( &qmask, SIGPIPE );
	newpipe.sa_handler = sigpipe;
	newpipe.sa_mask = qmask;
	newpipe.sa_flags = 0;
	sigaction( SIGPIPE, &newpipe, &origpipe );
}

#ifdef VNMRJ
/**
static void
tout(s)
char *s;
{
	if (fdps == NULL)
	{
	   fdps = fopen("/tmp/tdps", "w");
	   if (fdps == NULL)
		return;
	}
	fprintf(fdps, "%s \n", s);
}
**/
#endif 


#ifdef MOTIF
static void
set_font(Font ft)
{
 	if (ft == 0) {
	    return;
	}
	if (!xwin)
	    return;
	XSetFont(xdisplay, vnmr_gc, ft);
#ifdef VNMRJ
 	if (pxm_gc == NULL) {
	   return;
	}
	XSetFont(xdisplay, pxm_gc, ft);
#endif 
}
#endif 

static void
restore_sigpipe()
{
	sigaction( SIGPIPE, &origpipe, NULL );
}

/*  End of new routines added January 1996 and described in go.c  */

static void clear_cursor()
{
        int  tmpx;

	if (mouse_x2 >= 0 || mouse_x >= 0)
	{
#ifdef VNMRJ
            tmpx = mouse_x;  
            vj_x_cursor(0, &tmpx, 0, CURSOR_COLOR);
            tmpx = mouse_x2;  
            vj_x_cursor(1, &tmpx, 0, CURSOR_COLOR);
#else
	    xormode();
	    color(CURSOR_COLOR);
	    if (mouse_x >= 0)
	    {
	    	amove(mouse_x, 5);
	    	adraw(mouse_x, mouse_y);
	    }
	    if (mouse_x2 >= 0)
	    {
	    	amove(mouse_x2, 5);
	    	adraw(mouse_x2, mouse_y);
	    }
	    normalmode();
#endif 
	}
	mouse_x = -1;
	mouse_x2 = -1;
}

static void  mouse_move(x, y, button)
int x, y, button;
{
	int	oldx;

	if (button == 1)
	    return;
	if (x < 1 || x > graf_width)
	    x = -1;
	if (button == 0)
	{
	    if (x == mouse_x2)
		return;
	    oldx = mouse_x;
	    mouse_x = x;
	}
	else
	{
	    if (x == mouse_x)
		return;
	    oldx = mouse_x2;
	    mouse_x2 = x;
	}
#ifdef VNMRJ
        grf_batch(1);
	if (oldx > 0 || x > 0) {
           if (x < 0) x = 0;
	   if (button == 0)
              vj_x_cursor(0, &oldx, x, CURSOR_COLOR);
           else
              vj_x_cursor(1, &oldx, x, CURSOR_COLOR);
        }
        grf_batch(0);
#else
	xormode();
	color(CURSOR_COLOR);
	if (oldx > 0)
	{
	    amove(oldx, 5);
	    adraw(oldx, mouse_y);
	}
	if (x > 0)
	{
	    amove(x, 5);
	    adraw(x, mouse_y);
	}
	normalmode();
#endif
}


static SRC_NODE *search_node(dev, x, y, mark, anyDelay)
int	dev, x, y, mark, anyDelay;
{
	int	   yh, ym;
	int	   dy, dd, ddx, dx;
	SRC_NODE   *snode, *cnode;

	snode = info_node[dev];
	cnode = NULL;
	if (dev >= GRADX)
	{
	    yh = orgy[dev] + gradHeight + lineGap;
	    ym = orgy[dev] - gradHeight;
	}
        else
	{
    	    yh = orgy[dev] + pulseHeight + lineGap;
	    ym = orgy[dev] - pulseHeight;
	}
        if (anyDelay)
        {
    	    yh = orgy[TODEV] + pulseHeight;
	    ym = statusY;
        }
	dy = yh;
        dx = dpsW;
	while (snode != NULL)
	{
	     if (x >= snode->x1 && x <= snode->x3)
	     {
		if (!(snode->flag & NINFO))
		{
		    if (!(snode->flag & XMARK))
		    {
			if (snode->flag & XBOX)
			{
		            if (y >= snode->y3 && y <= snode->y2) {
                                if (snode->type == STATUS) {
                                   dd = 2; 
                                   cnode = snode;
                                }
                                else
				   return(snode);
                            }
			}
			else if (y <= yh && y >= ym)
			{
			   dd = abs(y - snode->y2);
			   if (cnode != NULL) {
				if (cnode->type == DELAY) {
				    if (dy > 10)
				       dy = dd + 1; 
				}
				else if (snode->type == DELAY) {
				    if (dd > 10)
				       dd = dy + 1; 
				}
				if (snode->flag & XLOOP) 
				       dd = 1; 
                                if (snode->type == FIDNODE) {
                                    if (cnode == hilitNode)
				       dd = 1; 
				}
			   }
			   if (dd <= dy) {
                                ddx = x - snode->x1;
                                if (ddx < dx) { 
				     cnode = snode;
				     dy = dd;
				     dx = ddx;
                                }
			    }
			}
		    }
		    else if (mark) /* XMARK: power, phase ... */
		    {
		        if (y >= snode->y3 && y <= snode->y2)
			    return(snode);
		    }
		}
	     }
	     snode = snode->knext;
	}
	return(cnode);
}

static SRC_NODE *search_parallel_node(x, y)
int	x, y;
{
        SRC_NODE *node;
        int k;

        node = parallel_start_node;
        k = 0;
        while (node != NULL) {
	     if (x >= node->x1 && x <= node->x3)
                 break;
             node = node->xnext;
             k++;
             if (k > 100) // bad 
                 return (NULL);
        }
        return (node);
}


static void mouse_but(but, up, x, y)
int but, up, x, y;
{
	int	   chan, locatedChan;
	SRC_NODE   *pnode;

#ifdef  SUN
	if (but == 0 && Wissun())
	{
	   if (x > menu_x && x < menu_x + menu_width)
	   {
	        if ( !menuUp && !up)
		{
	    	   if (y > 5 && y < menu_height + 5)
		   {
#ifndef VNMRJ
			menuUp = 1;
			popup_dps_pannel();
#endif 
		   }
		}
		return;
	   }
	}
#endif 
	if (but != 1)  /* not middle button */
	{
	    mouse_move(x, y, but);
	    return;
	}
	if (!up)
		return;
	if (!dpsPlot && !advancedDps)
 	    mnumypnts = graf_height;
	y = mnumypnts - y;
	if (y < dpsY || y > dpsY2)
		return;
	if (x < x_margin - spW / 2 || x > dpsX2)
		return;
	pnode = NULL;
	locatedChan = 0;
	x -= xDiff;
	if (dispMode & DSPMORE)
	     pnode = search_node(0, x, y, 1, 0);
	if (pnode == NULL)
             pnode = search_parallel_node(x, y);
	if (pnode == NULL)
	{
	   for(chan = GRADX; chan <= GRADZ; chan++)
	   {
	     if (rfchan[chan])
	     {
		if (y >= orgy[chan]-gradHeight && y <= orgy[chan]+gradHeight)
		{
		     locatedChan = chan;
		     break;
		}
	     }
	   }
	   if (locatedChan <= 0)
	   {
	     for(chan = GRADX - 1; chan >= TODEV; chan--)
	     {
	         if (rfchan[chan])
	         {
		     if (y >= orgy[chan]-lineGap && y < orgy[chan-1])
		     {
		          locatedChan = chan;
		          break;
		     }
		 }
	     }
	   }
	}
	if (locatedChan > 0)
	{
	    chan = locatedChan;
	    while (chan >= TODEV)
	    {
		pnode = search_node(chan, x, y, 0, 0);
		if (pnode != NULL)
		    break;
		chan--;
	    }
	}
        if (pnode == NULL)
	    pnode = search_node(TODEV, x, y, 0, 1);
        else if (pnode->visible < 1)
            pnode = NULL;

	if (pnode != NULL && pnode != hilitNode)
	{
            
            if ((pnode->type == XEND) && (pnode->bnode != NULL))
                  pnode = pnode->bnode;
            grf_batch(1);
	    if (hilitNode)
	    {
	   	draw_node(hilitNode, 2);
	    }
	    hilitNode = pnode;
	    draw_node(pnode, 0);
	    disp_info(pnode);
            grf_batch(0);
	}
}


static int turnoff_dps()
{
	if (!Wissun())
	{
	   Wgmode();
	   Wturnoff_mouse();
	}
        return 0;
}

static void print_array()
{
	ARRAY_NODE   *node;

	node = array_start_node;
	while (node != NULL)
	{
	   fprintf(stderr, " array: %s has %d elements\n", node->name, node->vars);
	   node = node->next;
	}
}


#ifdef SUN

static void set_dps_color(name, value)
char  *name, *value;
{
#ifdef VNMRJ
	int num, r, g, b, k;
	Pixel pix;

	num = -1;
	if (strcmp(name, "DpsBaseColor") == 0)
	    num = BCOLOR;
	if (strcmp(name, "DpsPulseColor") == 0)
	    num = PCOLOR;
	else if (strcmp(name, "DpsSColor") == 0)
	    num = SCOLOR;
	else if (strcmp(name, "DpsMSColor") == 0)
	    num = MCOLOR;
	else if (strcmp(name, "DpsUSColor") == 0)
	    num = UCOLOR;
	else if (strcmp(name, "DpsDelayColor") == 0)
	    num = DCOLOR;
	else if (strcmp(name, "DpsLabelColor") == 0)
	    num = LCOLOR;
	else if (strcmp(name, "DpsPwrColor") == 0)
	    num = PWCOLOR;
	else if (strcmp(name, "DpsPhsColor") == 0)
	    num = PHCOLOR;
	else if (strcmp(name, "DpsChannelColor") == 0)
	    num = CHCOLOR;
	else if (strcmp(name, "DpsStatusColor") == 0)
	    num = STATCOLOR;
	else if (strcmp(name, "DpsAcquireColor") == 0)
	    num = ACQCOLOR;
	else if (strcmp(name, "DpsOnColor") == 0)
	    num = ONCOLOR;
	else if (strcmp(name, "DpsOffColor") == 0)
	    num = OFFCOLOR;
	else if (strcmp(name, "DpsMarkColor") == 0)
	    num = MRKCOLOR;
	else if (strcmp(name, "DpsHWLoopColor") == 0)
	    num = HWLCOLOR;
	else if (strcmp(name, "DpsPELoopColor") == 0)
	    num = PELCOLOR;
	else if (strcmp(name, "DpsSelectedColor") == 0)
	    num = HCOLOR;
	else if (strcmp(name, "DpsInfoColor") == 0)
	    num = ICOLOR;
	else if (strcmp(name, "DpsFuncColor") == 0)
	    num = FCOLOR;

	if (num < 0)
	    return;
	if (value[0] == '0' && value[1] == 'x') {
	    k = (int) strtol(value, NULL, 16);
	    r = (k >> 16) & 0xFF;
	    g = (k >> 8) & 0xFF;
	    b = k & 0xFF;
	    if (!xwin) {
		show_color(num+vjColor, r, g, b);
		return;
	    }
	    pix = search_pixel(r, g, b);
	    dpsPix[num] = pix;
	}
	else {
	    if (!xwin) {
		vnmrj_color_name(num+vjColor, value);
		return;
	    }
	    pix = get_vnmr_pixel_by_name(value);
	    dpsPix[num] = pix;
	}
	batch_draw();
#endif 
}

static
char *get_dps_resource(class, name)
char  *class, *name;
{
#ifdef MOTIF
	char       *str_type;
        XrmValue   rval;

        if (!Wissun() || xwin <= 0)
	    return((char *) NULL);
	if (xbase == NULL)
	{
	   xbase = XtDatabase(xdisplay);
	   if (xbase == NULL)
	       return((char *) NULL);
	}
	sprintf(info_data, "%s*%s", class, name);
	if (XrmGetResource(xbase, info_data, class, &str_type, &rval))
	   return((char *) rval.addr);
	if (XrmGetResource(xbase, name, class, &str_type, &rval))
	   return((char *) rval.addr);
#endif
	return((char *) NULL);
}

#endif 

static  char
*allocateNode(size_t n)
{
     char *d;
     d = allocateWithId(n, "dpsnode");
     if (d != NULL)
	bzero(d, n);
     return (d);
}

static  char
*allocateTmpNode(size_t n)
{
     char *d;
     d = allocateWithId(n, "dpstmp");
     if (d != NULL) {
        hasTmpNode = 1;
	bzero(d, n);
     }
     return (d);
}

static  char
*allocateArray(size_t n)
{
     return (allocateWithId(n, "dpsarray")); 
}

static  char
*allocateAp(size_t n)
{
     return (allocateWithId(n, "dpsap")); 
}

static  void
releaseNode()
{
     SHAPE_NODE  *sh_node;

     sh_node = shape_list;
     while (sh_node != NULL) {
         if (sh_node->name != NULL)
             free(sh_node->name);
         if (sh_node->filename != NULL)
             free(sh_node->filename);
         if (sh_node->amp != NULL)
             free(sh_node->amp);
         if (sh_node->phase != NULL)
             free(sh_node->phase);
         if (sh_node->data != NULL)
             free(sh_node->data);
         sh_node = sh_node->next;
     }
     if (ybarData != NULL) {
         free(ybarData);
         ybarData = NULL;
         ybarSize = 0;
     }
     releaseWithId("dpsnode");
}

static  void
releaseTmpNode()
{
     if (hasTmpNode != 0)
        releaseWithId("dpstmp");
     hasTmpNode = 0;
     parallel_start_node = NULL;
     parallel_x_list = NULL;
}

static  void
releaseArray()
{
     releaseWithId("dpsarray");
}

static  void
releaseAp()
{
     releaseWithId("dpsap");
}

static  ARRAY_NODE
*new_arraynode(type, code, num, name)
 int	type, code, num;
 char   *name;
{
     ARRAY_NODE   *datanode, *pnode;
     int i;
     char    **sval;
     double   *fval; 

     pnode = array_start_node;
     while (pnode != NULL)
     {
	 if ((pnode->code == code) && (strcmp(pnode->name, name) == 0))
		return(pnode);
	 if (pnode->next == NULL)
		break;
	 pnode = pnode->next;
     }

     if((datanode = (ARRAY_NODE *) allocateArray(sizeof(ARRAY_NODE))) == 0)
	return(NULL);
     sizeT = (size_t) num;
     if (type == T_STRING)
     {
	datanode->value.Str = (char **) allocateArray(sizeof(char *) * sizeT);
	if (datanode->value.Str == NULL)
	    return(NULL);
        sval = datanode->value.Str;
        for (i = 0; i < num; i++)
            sval[i] = NULL;
     }
     else
     {
	datanode->value.Val = (double *) allocateArray (sizeof(double) * sizeT);
	if (datanode->value.Val == NULL)
	    return(NULL);
        fval = datanode->value.Val;
        for (i = 0; i < num; i++)
            fval[i] = 0.0;
     }
     datanode->name = (char *) allocateArray (strlen(name)+1);
     strcpy(datanode->name, name);
     datanode->dev = 1;
     datanode->ni = 0;
     datanode->id = -2;
     datanode->index = 0;
     datanode->type = type;
     datanode->code = code;
     datanode->linked = 0;
     datanode->vars = num;
     datanode->next = NULL;
     datanode->dnext = NULL;
     if (array_start_node == NULL)
        array_start_node = datanode;
     else
	pnode->next = datanode;
     return(datanode);
}


static
char *get_label(item)
int	item;
{
	int    len;
	char   *data;
	char   *retdata;
	static int    labelPtr = 0;

	if (item >= argIndex) {
            if (cmd_name != NULL) {
                if (cmd_error == 0)
	            Werrprintf("dps error: argument of psg command '%s' was missing.\n", cmd_name);
            }
            else if (argIndex > 1)
	        Werrprintf("dps error: data(label) index in %s was %d out of boundary(%d).\n",argArray[1], item, argIndex);
            cmd_error++;
	    return((char *)NULL);
        }
	len = strlen(argArray[item]);
	if (dps_label == NULL || (labelPtr+len) >= LABELEN)
	{
     	     if((data = (char *) allocateNode (LABELEN)) == 0)
	     {
		Wprintf("dps error: memory allocation failed.\n");
	        return((char *)NULL);
             }
	     dps_label = data;
	     labelPtr = 0;
	}
	retdata = dps_label;
	data = argArray[item];
	if (*data == '?' && len > 1)
	{
	     data++;
	     len--;
	}
	strcpy(dps_label, data);
	dps_label = dps_label + len + 1;
	labelPtr = labelPtr + len + 1;
	
	return(retdata);
}


static int
get_intg(item)
int	item;
{
	int  data;
	char k;

	if (item >= argIndex) {
            if (cmd_name != NULL) {
                if (cmd_error == 0)
	           Werrprintf("dps error: argument of psg command '%s' was missing.\n", cmd_name);
            }
            else if (debug && argIndex > 1)
	       Wprintf("dps error: data(int) index in %s was %d out of boundary(%d).\n",argArray[1], item, argIndex);
            cmd_error++;
	    return(-1);
        }
	k = argArray[item][0];
	if (k < '0' || k > '9')
	{
	    if (k != '-' && k != '+' && k != '.')
		return(-1);
	}
	data = atoi(argArray[item]);
	return(data);
}


static double
get_float(item)
int	item;
{
	double  fval;
	char k;

	if (item >= argIndex) {
            if (cmd_name != NULL) {
                if (cmd_error == 0)
	            Werrprintf("dps error: argument of psg command '%s' was missing.\n", cmd_name);
            }
            else if (argIndex > 1)
	       Wprintf("dps error: data(double) index in %s was %d out of boundary(%d).\n", argArray[1],item, argIndex);
            cmd_error++;
	    return(-1.0);
        }
	k = argArray[item][0];
	if (k < '0' || k > '9')
	{
	    if (k != '-' && k != '+' && k != '.')
		return(-1.0);
	}
	fval = atof(argArray[item]);
	return(fval);
}


static double
dps_get_real(tree, name, item, failed_val)
int	tree, item;
char	*name;
double   failed_val;
{
    	double  tmpval;
	double	retval;

        if (name == NULL)
            return failed_val;
	if ( P_getreal(tree, name, &tmpval, item) >= 0 )
		retval = (double) tmpval;
	else
		retval = failed_val;
	return(retval);
}


static  AP_NODE
*new_apnode()
{
     AP_NODE   *node, *apnode;
     int       id, num;

     id = get_intg(1);
     apnode = apstart;
     while (apnode != NULL)
     {
	if (apnode->id == id)
		return(apnode);
	if (apnode->next == NULL)
		break;
	apnode = apnode->next;
     }
     if((node = (AP_NODE *) allocateAp (sizeof(AP_NODE))) == 0)
	return(NULL);
     num = get_intg(2);
     sizeT = (size_t) num;
     node->val = (int *) allocateAp (sizeof(int) * sizeT);
     if (node->val == NULL)
	return(NULL);
     node->oval = (int *) allocateAp (sizeof(int) * sizeT);
     if (node->oval == NULL)
	return(NULL);
     node->id = id;
     node->size = num;
     node->index = 0;
     node->changed = 0;
     node->autoInc = get_intg(3);
     node->divn = get_intg(4);
     node->next = NULL;
     if (apstart == NULL)
	apstart = node;
     else
	apnode->next = node;
     return(node);
}

static void
clean_src_data()
{
     releaseNode();
     src_start_node = NULL;
     disp_start_node = NULL;
     src_end_node = NULL;
     spare_src_node = NULL;
     dock_src_node = NULL;
     angle_start_node = NULL;
     rotate_start_node = NULL;
     angle_node = NULL;
     grp_start_node = NULL;
     dock_times = NULL;
     hilitNode = NULL;
     dps_label = NULL;
     dock_list = NULL;
     dock_N_array = NULL;
     shape_list = NULL;
     dock_len = 0;
     dock_time_len = 0;
}



static  SRC_NODE
*new_dpsnode()
{
	SRC_NODE   *datanode;

	if((datanode = (SRC_NODE *) allocateNode(sizeof(SRC_NODE))) == 0)
	{
             Werrprintf("dps error: memory allocation failed.\n");
             clean_src_data();
	     return(NULL);
	}
	datanode->type = DUMMY;
	datanode->flag = XHGHT | XWDH;
	datanode->line = jline;
        datanode->dockAt = NODOCK;
        datanode->dockTo = NODOCK;
        datanode->dockFrom = NODOCK;
        datanode->visible = 1;
        datanode->wait = 1;
/*
	datanode->fname = NULL;
	datanode->type = DUMMY;
	datanode->device = 0;
	datanode->ptime = 0.0;
	datanode->power = 0.0;
	datanode->phase = 0.0;
	datanode->flag = XHGHT | XWDH;
	datanode->mode = 0; 
	datanode->rtime = 0.0;
	datanode->rtime2 = 0.0;
	datanode->line = jline;
	datanode->next = NULL;
	datanode->pnode = NULL;
	datanode->bnode = NULL;
	datanode->dnext = NULL;
	datanode->knext = NULL;
	datanode->dockList = NULL;
        datanode->id = 0;
        datanode->pid = 0;
        datanode->dockAt = NODOCK;
        datanode->dockTo = NODOCK;
        datanode->dockFrom = NODOCK;
        datanode->mDocked = 0;
        datanode->stime = 0;
        datanode->etime = 0;
        datanode->mtime = 0;
        datanode->mx = 0;
        datanode->dockNum = 0;
        datanode->lNode = NULL;
        datanode->mNode = NULL;
        datanode->rNode = NULL;
*/
	return(datanode);
}

static  SHAPE_NODE
*new_shapenode()
{
     SHAPE_NODE  *sh_node, *pnode;

     if ((sh_node = (SHAPE_NODE *) allocateNode(sizeof(SHAPE_NODE))) == 0)
     {
         Werrprintf("dps error: memory allocation failed.\n");
         clean_src_data();
         return(NULL);
     }
     sh_node->dataSize = 0;
     sh_node->shapeType = 0;
     sh_node->power = 99999;
     if (shape_list == NULL)
         shape_list = sh_node;
     else {
         pnode = shape_list;
         while (pnode->next != NULL)
             pnode = pnode->next;
         pnode->next = sh_node;
     }
     return(sh_node);
}


static  DOCK_NODE
*new_docknode(snode)
DOCK_NODE   *snode;
{
	DOCK_NODE   *node;

	if (snode == NULL) {
	     if (dock_src_node != NULL)
	        return(dock_src_node);
	}
	else if (snode->next != NULL)
	     return(snode->next);
	if((node = (DOCK_NODE *) allocateNode(sizeof(DOCK_NODE))) == NULL)
	     return(NULL);
	node->next = NULL;
	if (dock_src_node == NULL)
	     dock_src_node = node;
	if (snode != NULL)
	     snode->next = node;
	return(node);
}

static  SRC_NODE
*new_sparenode(snode)
SRC_NODE   *snode;
{
	SRC_NODE   *datanode;

	if (snode != NULL && snode->next != NULL)
	     return(snode->next);
	if((datanode = (SRC_NODE *) allocateNode(sizeof(SRC_NODE))) == 0)
	     return(NULL);
	datanode->line = jline;
	datanode->visible = 1;
/*
	datanode->next = NULL;
	datanode->bnode = NULL;
	datanode->dnext = NULL;
	datanode->knext = NULL;
*/
	if (spare_src_node == NULL)
	     spare_src_node = datanode;
	if (snode != NULL)
	     snode->next = datanode;
	return(datanode);
}

static void
copy_src_node(snode, dnode, attr)
SRC_NODE  *snode, *dnode;
int	  attr;
{
	COMMON_NODE  *csnode, *cdnode;
	int	k;

	dnode->type = snode->type;
	dnode->flag = snode->flag;
	dnode->device = snode->device;
	dnode->rtime = snode->rtime;
	dnode->rtime2 = snode->rtime2;
	dnode->ptime = snode->ptime;
	dnode->fname = snode->fname;
	dnode->mode = snode->mode;
	dnode->power = snode->power;
	dnode->phase = snode->phase;
	dnode->x1 = snode->x1;
	dnode->x2 = snode->x2;
	dnode->x3 = snode->x3;
	dnode->y1 = snode->y1;
	dnode->y2 = snode->y2;
	dnode->y3 = snode->y3;
	if (attr == 1)
	{
	   csnode = (COMMON_NODE *) &snode->node.common_node;
	   cdnode = (COMMON_NODE *) &dnode->node.common_node;
	   for(k = 0; k < 7; k++)
	   {
	      cdnode->val[k] = csnode->val[k];
	      cdnode->vlabel[k] = csnode->vlabel[k];
	   }
	   for(k = 0; k < 5; k++)
	   {
	      cdnode->fval[k] = csnode->fval[k];
	      cdnode->flabel[k] = csnode->flabel[k];
	   }
	   cdnode->dname = csnode->dname;
	   cdnode->pattern = csnode->pattern;
	   cdnode->catt = csnode->catt;
	   cdnode->fatt = csnode->fatt;
	}
}


static int
get_rf_shape(SRC_NODE *node, char *shapeName)
{
   int      k, capacity, count;
   double   d1, d2, d3;
   double   min, max, *amp, *ampPtr, *phase, *phasePtr;
   SHAPE_NODE *sh_node;
   FILE    *fd;
   char    str[MAXPATH], *data;
   struct  stat   f_stat;

   if (shapeName == NULL)
      return(1);
   if ((int)strlen(shapeName) <= 1) {
      if (*shapeName == '?') {
         node->visible = 0;
         return(1);
      }
   } 
   sh_node = shape_list;
   while (sh_node != NULL) {
      if (sh_node->name != NULL && sh_node->type == 1) {
          if (strcmp(sh_node->name, shapeName) == 0) {
              if (sh_node->amp == NULL)
                  node->shapeData = NULL;
              else
                  node->shapeData = sh_node;
              return(1);
          }
      }
      sh_node = sh_node->next;
   }
   sh_node = new_shapenode();
   if (sh_node == NULL)
      return(0);
   node->shapeData = sh_node;
   sh_node->type = 1;
   sh_node->name = (char *) malloc(strlen(shapeName) + 2);
   strcpy(sh_node->name, shapeName);
   strcpy(str, "");
   fd = NULL;
   k = appdirFind(shapeName, shapeDir, str, ".RF", R_OK);
   if ( k != 0)
       fd = fopen(str, "r");
   if (fd == NULL) {
       sprintf(str, "%s/%s/%s.RF", userdir, shapeDir, shapeName);
       fd = fopen(str, "r");
       if (fd == NULL) {
          sprintf(str, "%s/%s/%s.RF",systemdir, shapeDir, shapeName);
          fd = fopen(str, "r");
       }
       if (fd == NULL) {
          sprintf(str, "%s/imaging/%s/%s.RF",systemdir,shapeDir, shapeName);
          fd = fopen(str, "r");
       }
       if (fd == NULL) {
          // Werrprintf("dps error: could not open shape file '%s'.\n", shapeName);
          node->shapeData = NULL;
          return(1);
       }
   }
   sh_node->filename = (char *) malloc(strlen(str) + 2);
   strcpy(sh_node->filename, str);
   // fsync(fileno(fd));
   if (strstr(str, userdir) != NULL)
   {
       fclose(fd);
       count = 0;
       for (k = 0; k < 5; k++) {
          if (stat(str, &f_stat) >= 0) {
             if (count == (int)f_stat.st_size)
                break;
             count = (int)f_stat.st_size;
          }
          if (k == 0)
             usleep(5.0e+4);
          else
             usleep(1.0e+5);
       }
       fd = fopen(str, "r");
   }
   capacity = 512;
   amp = (double *) malloc(sizeof(double) * capacity);
   if (amp == NULL) {
       return(1);
   }
   phase = (double *) malloc(sizeof(double) * capacity);
   if (phase == NULL) {
       free(amp);
       return(1);
   }
     
   count = 0;
   min = 1024.0;
   max = -1024.0;
   ampPtr = amp;
   phasePtr = phase;
   while ((data = fgets(str, MAXPATH, fd)) != NULL)
   {
       k = (int) strlen(str);
       while (k > 3) {
          if (*data == '#') {
              k = 0;
              break;
          }
          if (*data >= '0' && *data <= '9')
              break;
          data++;
          k--;
       }
       if (k < 5)
          continue;
       // phase, amplitude, repeat
       if (sscanf(str, "%lf%lf%lf", &d1, &d2, &d3) != 3)
          continue;
       k = (int) d3; // repeat
       if (d2 < min)  min = d2;
       if (d2 > max)  max = d2;
       if (capacity <= (count + k)) {
           if (k > 1000)
               capacity = capacity + k + 256;
           else
               capacity = capacity + 1024;
           amp = (double *) realloc(amp, sizeof(double) * capacity);
           if (amp == NULL) {
               free(phase);
               return (1);
           }
           phase = (double *) realloc(phase, sizeof(double) * capacity);
           if (phase == NULL) {
               free(amp);
               return (1);
           }
           ampPtr = amp + count;
           phasePtr = phase + count;
       }
       while (k > 0) {
          *phasePtr = d2 * cos(d1);
          *ampPtr = d2;
          k--;
          ampPtr++;
          phasePtr++;
          count++;
       }
   }
   fclose(fd);

   sh_node->amp = amp;
   sh_node->phase = phase;
   sh_node->dataSize = count;
   sh_node->minData = min;
   sh_node->maxData = max;
   count++;
   sh_node->data = (short *) malloc(sizeof(short) * count);
   if (max > rfShapeMax)
       rfShapeMax = max;
   if (min < rfShapeMin)
       rfShapeMin = min;

   return(1);
}

static int
get_gradient_shape(SRC_NODE *node, char *shapeName)
{
   int      k, capacity, count;
   double   d1, d2;
   double   min, max, *dpr, *amp;
   SHAPE_NODE *sh_node;
   FILE    *fd;
   char    str[MAXPATH], *data;

   node->updateLater = 0;
   if (shapeName == NULL)
      return(1);
   if ((int)strlen(shapeName) <= 1) {
      if (*shapeName == '?') {
         node->visible = 0;
         return(1);
      }
   } 
   sh_node = shape_list;
   while (sh_node != NULL) {
      if (sh_node->name != NULL && sh_node->type == GRADDEV) {
          if (strcmp(sh_node->name, shapeName) == 0) {
              node->shapeData = sh_node;
              node->updateLater = 1;
              return(1);
          }
      }
      sh_node = sh_node->next;
   }
   sh_node = new_shapenode();
   if (sh_node == NULL)
      return(0);
   node->shapeData = sh_node;
   sh_node->type = GRADDEV;
   sh_node->name = (char *) malloc(strlen(shapeName) + 2);
   strcpy(sh_node->name, shapeName);
   strcpy(str, "");
   fd = NULL;
   k = appdirFind(shapeName, shapeDir, str, ".GRD", R_OK);
   if ( k != 0)
       fd = fopen(str, "r");
   if (fd == NULL) {
       sprintf(str, "%s/%s/%s.GRD", userdir, shapeDir, shapeName);
       fd = fopen(str, "r");
       if (fd == NULL) {
          sprintf(str, "%s/%s/%s.GRD",systemdir, shapeDir, shapeName);
          fd = fopen(str, "r");
       }
       if (fd == NULL) {
          sprintf(str, "%s/imaging/%s/%s.GRD",systemdir,shapeDir, shapeName);
          fd = fopen(str, "r");
       }
       if (fd == NULL) {
         //  Werrprintf("dps error: could not open shape file '%s'.\n", shapeName);
          node->shapeData = NULL;
          return(1);
       }
   }
   sh_node->filename = (char *) malloc(strlen(str) + 2);
   strcpy(sh_node->filename, str);
   if (strstr(str, userdir) != NULL)
       usleep(1000);
   capacity = 256;
   amp = (double *) malloc(sizeof(double) * capacity);
   if (amp == NULL)
   {
       fclose(fd);
       return(1);
   }
   count = 0;
   min = 32767.0;
   max = -32767.0;
   dpr = amp;
   while ((data = fgets(str, MAXPATH, fd)) != NULL)
   {
       k = (int) strlen(str);
       while (k > 2) {
          if (*data == '#') {
              k = 0;
              break;
          }
          if (*data >= '0' && *data <= '9')
              break;
          data++;
          k--;
       }
       if (k < 3)
          continue;
       //  amplitude, repeat
       if (sscanf(str, "%lf%lf", &d1, &d2) != 2)
          continue;
       k = (int) d2; // repeat
       if (d1 < min)  min = d1;
       if (d1 > max)  max = d1;
       if (capacity <= (count + k)) {
           if (k > 1000)
               capacity = capacity + k + 128;
           else
               capacity = capacity + 1024;
           amp = (double *) realloc(amp, sizeof(double) * capacity);
           if (amp == NULL)
               return (1);
           dpr = amp + count;
       }
       while (k > 0) {
          *dpr = d1;
          k--;
          dpr++;
          count++;
       }
   }
   fclose(fd);

   sh_node->amp = amp;
   sh_node->dataSize = count;
   sh_node->minData = min;
   sh_node->maxData = max;
   count++;
   sh_node->data = (short *) malloc(sizeof(short) * count);
   if (max > gradShapeMax)
       gradShapeMax = max;
   if (min < gradShapeMin)
       gradShapeMin = min;
   node->updateLater = 1;

   return(1);
}

static void
convert_time_val(val)
double 	val;
{
	int     h, m, s;

	if (val <= 0.0)
	   val = 0.0;

	h = (int) ((int)val / 3600);
	m = (int) (((int)val % 3600) / 60);
	s = (int) ((int)val % 60);
	sprintf(inputs, "%02d:%02d:%02d", h, m, s);
}

static void
set_rcvrs_param()
{
        int n, k;

        k = (int) dps_get_real(GLOBAL,"numrcvrs",1, 0.0);
        if (k < 1)
        {
             k = (int) dps_get_real(SYSTEMGLOBAL,"numrcvrs",1, 0.0);
             if (k < 1)
                k = (int) dps_get_real(CURRENT,"numrcvrs",1, 0.0);
        }
        if (k > numrfch)
            k = numrfch;
        rcvrsNum = k;
        k = numrfch + 2;
	if (P_getstring(CURRENT, "rcvrstype", rcvrsType, 1, k) < 0) {
	    if (P_getstring(GLOBAL, "rcvrstype", rcvrsType, 1, k) < 0) {
	       if (P_getstring(SYSTEMGLOBAL, "rcvrstype", rcvrsType, 1, k) < 0)
                  strcpy(rcvrsType, "ssss");
            }
        }

	if (P_getstring(CURRENT, "rcvrs", rcvrsStr, 1, k) < 0) {
	    if (P_getstring(GLOBAL, "rcvrs", rcvrsStr, 1, k) < 0) {
	       if (P_getstring(SYSTEMGLOBAL, "rcvrs", rcvrsStr, 1, k) < 0)
                  strcpy(rcvrsStr, "ynnnn");
            }
        }
        if (rcvrsType[0] != 'm')
            strcpy(rcvrsStr, "ynnnn");
        n = 0;
        for (k = 0; k <= RFCHAN_NUM; k++) {
            if (n >= rcvrsNum)
                rcvrsStr[k] = '\0';
            else if (rcvrsStr[k] == 'y' || rcvrsStr[k] == 'Y')
                n++;
        }
}


static void
set_sys_param()
{
	int	dev, k, mercury;
	char    tmpstr[12];
	double	sfrq, dfrq, h1frq;

    	for(dev = 0; dev < RFCHAN_NUM; dev++)
    	{
	    powerMax[dev] = 63.0;
	    coarseInit[dev] = 63.0;
	    fineInit[dev] = 1.0;
	    powerCh[dev] = 63.0;
	}
        for(dev = 0; dev < TOTALCH; dev++)
	{
	     rfchan[dev] = 0;
	     activeCh[dev] = 0;
	     status_node[dev] = NULL;
	     visibleCh[dev] = 1;
	}
        if (!(dispMode & DSPDECOUPLE)) {
             for (dev = DODEV; dev <= DO5DEV; dev++)
	         visibleCh[dev] = 0;
        }
        numrfch = (int) dps_get_real(SYSTEMGLOBAL,"numrfch",1, 2.0);
        if (numrfch < 1)
	     numrfch = 1;
        if (numrfch >= RFCHAN_NUM)
	     numrfch = RFCHAN_NUM - 1;
        for(dev = 1; dev <= numrfch; dev++)
	     activeCh[dev] = 1;
        set_rcvrs_param();
	cyclephase = 0;
    	if (P_getstring(CURRENT, "cp", tmpstr, 1, 2) >= 0)
	{
	      if (tmpstr[0] == 'y' || tmpstr[0] == 'Y')
	      	cyclephase = 1;
	}
	isHomo = 0;
    	if (P_getstring(CURRENT, "homo", tmpstr, 1, 2) >= 0)
	{
	      if (tmpstr[0] == 'y' || tmpstr[0] == 'Y')
	      	isHomo = 1;
	}
	if (P_getstring(GLOBAL, "gradientshaping", gradshape, 1, 3) < 0)
              strcpy(gradshape, "ss");
	if (P_getstring(SYSTEMGLOBAL, "gradtype", gradtype, 1, 7) < 0)
              strcpy(gradtype, "nnnnnn");
	for(dev = 0; dev < GRAD_NUM; dev++)
	{
/*
	      if (gradtype[dev] == 's' || gradtype[dev] == 'S')
                  gradMax[dev] = 2048;
	      else if (gradtype[dev] == 'l' || gradtype[dev] == 'L')
                  gradMax[dev] = 2048;
	      else
                  gradMax[dev] = 32767;
*/
              gradMax[dev] = 2.0;
	}
        gradMaxLevel = 2.0;
        gradMinLevel = 0.0;
	dlp =  dps_get_real(CURRENT,"dlp", 1, 0.0);
	dhp =  dps_get_real(CURRENT,"dhp", 1, -1.0);
	pplvl =  dps_get_real(CURRENT,"pplvl", 1, 0.0);
    	for (dev = 0; dev < RFCHAN_NUM; dev++)
	{
	     cattn[dev] = dps_get_real(SYSTEMGLOBAL,"cattn", dev, 1.0);
	     fattn[dev] =  dps_get_real(SYSTEMGLOBAL,"fattn", dev, 1.0);
             if (fattn[dev] < 1.0)
                fattn[dev] = 1.0;
	     fineStep[dev] = fattn[dev];
	     if (P_getstring(CURRENT, dn_labels[dev], dnstr[dev], 1, 10) < 0)
                strcpy(dnstr[dev], "00");
	     if (P_getstring(CURRENT, dm_labels[dev], dmstr[dev], 1, 30) < 0)
                strcpy(dmstr[dev], "nnnnn");
	     if (P_getstring(CURRENT, dmm_labels[dev], dmmstr[dev], 1, 30) < 0)
                strcpy(dmmstr[dev], "nnnnn");
	     dmf_val[dev] = dps_get_real(CURRENT,dmf_labels[dev], 1, 0.0);
	     offsetInit[dev] = dps_get_real(SYSTEMGLOBAL,off_labels[dev], 1, 0.0);
             chanMap[dev] = dev;
	}
	cattn[0] = fattn[0] = 0.0;

	sfrq = dps_get_real(CURRENT,"sfrq", 1, 0.0);
	dfrq = dps_get_real(CURRENT,"dfrq", 1, 0.0);
	mercury = 0;
	if (P_getstring(SYSTEMGLOBAL, "Console", tmpstr, 1, 10) >= 0)
	{
	     if (strcmp(tmpstr, "mercury") == 0)
		mercury = 1;
	}
	if (P_getstring(SYSTEMGLOBAL, "rftype", tmpstr, 1, 9) < 0)
             strcpy(tmpstr, "00");
	k = (int) strlen(tmpstr);
	for(dev = k; dev < RFCHAN_NUM; dev++)
	     tmpstr[dev] = tmpstr[k - 1];
	if (P_getstring(SYSTEMGLOBAL, "amptype", info_data, 1, 9) < 0)
             strcpy(info_data, "00000");
	k = (int) strlen(info_data);
	for (dev = k; dev < RFCHAN_NUM; dev++)
	     info_data[dev] = info_data[k - 1];
	for (dev = 1; dev < RFCHAN_NUM; dev++)
	{
	  switch (tmpstr[dev - 1]) {
	    case 'f':
	           rfType[dev] = GEMINIB;  /* Gemini broad band */
	    	   fineStep[dev] = 1.0;
	    	   fineInit[dev] = dlp;
	    	   powerMax[dev] = 63.5;
		   break;
	    case 'e':
	    	   fineStep[dev] = 1.0;
	    	   powerMax[dev] = 63.5;
	    	   fineInit[dev] = dlp;
		   if (mercury)
			rfType[dev] = GEMINIB;
		   else
	           	rfType[dev] = GEMINI;   /* Gemini 1H/13C  */
		   break;
	    case 'c':
	    case 'C':
	    case 'd':
	    case 'D':
	           rfType[dev] = UNITYPLUS;
		   break;
	    default:
	     	   rfType[dev] = UNITY;
		   if (info_data[dev - 1] == 'c')  /* class c amp */
		   {
	    	   	fineStep[dev] = 1.0;
/**
	     	   	rfType[dev] = VXRC;
**/
		   }
		   break;
	    }
            if (mercury)
            {  fineStep[dev] = 255.0;
               rfType[dev] = GEMINIBH;
            }
	}

	if (strcmp(dnstr[TODEV], dnstr[DODEV]) == 0)
	    homodecp = 1;
	else
	    homodecp = 0;
	if (rfType[TODEV] < UNITY)
	{
	    homodecp = 0;
	    rfType[DODEV] = rfType[TODEV];
	    h1frq = dps_get_real(SYSTEMGLOBAL,"h1freq", 1, 0.0);
	    if (rfType[TODEV] == VXRC)
	    {
	   	if (dhp <= 0.0)
		    dhp = 1.0;
	   	powerMax[TODEV] = dhp;
	   	if (homodecp)
		    powerMax[DODEV] = 39.0;
	   	else
		    powerMax[DODEV] = 255.0;
	    }
	    else if (rfType[TODEV] <= GEMINIH)
	    {
		if (sfrq >= h1frq * 0.5) /* homo decoupler */
		{
	    	    coarseInit[DODEV] = dlp;
		    powerMax[DODEV] = 2047.0;
		    rfType[DODEV] = GEMINIH;
		    homodecp = 1;
		}
		else
		{
		    powerMax[DODEV] = 1.0;
	    	    coarseInit[DODEV] = dhp;
		}
	    }
	    else /* GEMINIB */
	    {
	    	dhp = dps_get_real(CURRENT,"dpwr", 1, -1.0);
		if (sfrq >= h1frq * 0.5)
		{
	    	    fineStep[TODEV] = 1023.0;
	    	    fineInit[TODEV] = dlp;
		    rfType[TODEV] = GEMINIBH;
		}
		if (dfrq >= h1frq * 0.5)
		{
	    	    fineStep[DODEV] = 1023.0;
	    	    fineInit[DODEV] = dlp;
		    rfType[DODEV] = GEMINIBH;
		}
                if (mercury)
                {  fineStep[TODEV] = 255.0;
                   fineStep[DODEV] = 255.0;
                   rfType[TODEV] = GEMINIBH;
                   rfType[DODEV] = GEMINIBH;
	    	   fineInit[TODEV] = dps_get_real(CURRENT,"tpwrf", 1, 255.0);
	    	   fineInit[DODEV] = dps_get_real(CURRENT,"dpwrf", 1, 255.0);
                }
	    	coarseInit[TODEV] = dps_get_real(CURRENT,"tpwr", 1, 0.0);
	    	coarseInit[DODEV] = dps_get_real(CURRENT,"dpwr", 1, 0.0);
	    }
	}
}

static int
check_psg_file(fname)
char    *fname;
{
	int	 n;

	if (fin != NULL) {
	    fclose(fin);
	    fin = NULL;
	}
	for (n = 0; n < 3; n++)
	{
	    if ((fin = fopen(fname, "r")) != NULL)
                break;
	    sleep(1);
	}
	if (fin == NULL) {
	    Werrprintf("dps error: run %s failed.", psgfile);
	    return(0);
	}

	return(1);
}

static
int is_real_number(data)
char *data;
{
	while (*data != '\0')
	{
	    if (*data < '0' || *data > '9')
	    {
		if (*data != '.' && *data != '-' && *data != '+')
		    return(0);
	    }
	    data++;
	}
	return(1);
}

static
void scan_args(argc, argv)
int argc;
char **argv;
{
        timeFlag = 0;
    	dpsPlot = 0;

        if (strcmp(argv[0], "exptime") == 0)
            timeFlag = 1;
	else if (argv[0][0] == 'p')
	{
    	    dpsPlot = 1;
	}
}

static
int check_args(int argc, char *argv[], int retc, char *retv[])
{
	int	n, gflag;
     	double   fval;
	char    *sfile;

	sfile = NULL;
        whichTree = CURRENT;
        timeFlag = 0;
	JFlag   = 0;
    	dpsPlot = 0;
	plotW = 2000.0;
	plotH = 2000.0;
	plotX = 0.0;
	plotY = 0.0;
	winW = 2000.0;
	winH = 2000.0;
	winX = 0.0;
	winY = 0.0;
	gflag = 0;
	clearPlot = 1;
	clearDraw = 1;
        if (strcmp(argv[0], "exptime") == 0)
            timeFlag = 1;
	else if (argv[0][0] == 'p')
	{
    	    dpsPlot = 1;
	    if (strcmp(argv[0], "ppsn") == 0)
		    clearPlot = 0;
	}
	else
	{
	    if (strcmp(argv[0], "dpsn") == 0)
		    clearDraw = 0;
	    if (strcmp(argv[0], "ndps") == 0)
	         advancedDps = 1;
	}
	if (wcmax < 1.0)
	    wcmax = 1.0;
	if (wc2max < 1.0)
	    wc2max = 1.0;
	n = 1;
        dpsFine=0;
	while (n < argc)
	{
             if (argv[n][0] == '-' && (int) strlen(argv[n]) >= 2)
	     {
		if (argv[n][1] == 'd')
		{
		    debug = 1;
		    if (argv[n][2] == '0')
			debug = 0;
		    if (argv[n][2] == '2')
			debug = 2;
		    else if (argv[n][2] == '3')
			debug = 3;
		    else if (argv[n][2] == '4')
			debug = 4;
		}
		if (argv[n][1] == 'j')
		{
		    jout = 1;
		}
		if (argv[n][1] == 'p')
		{
		    tbug = 1;
		}
		if ( ! strcmp(argv[n],"-fine")  )
                {  dpsFine = 1;
                }
	     }
	     else if (is_real_number(argv[n]))
	     {
		fval = atof(argv[n]);
		if (fval < 0.0)
		    fval = 0.0;
		if (!(gflag & 0x01))
		{
		    if (fval > (wcmax - wcmax / 30.0))
			fval = wcmax - wcmax / 30.0;
		    if (dpsPlot)
			plotX = fval;
		    else
			winX = fval;
		    gflag = gflag | 0x01;
		}
		else if (!(gflag & 0x02))
		{
		    if (fval > (wc2max - wc2max / 50.0))
			fval = wc2max - wc2max / 50.0;
		    if (dpsPlot)
			plotY = fval;
		    else
			winY = fval;
		    gflag = gflag | 0x02;
		}
		else if (!(gflag & 0x04))
		{
		    if (dpsPlot)
			plotW = atof(argv[n]);
		    else
			winW = atof(argv[n]);
		    gflag = gflag | 0x04;
		}
		else if (!(gflag & 0x08))
		{
		    if (dpsPlot)
			plotH = atof(argv[n]);
		    else
			winH = atof(argv[n]);
		    gflag = gflag | 0x08;
		}
	     }
	     else {
                if ( timeFlag &&  ! strcmp(argv[n],"usertree") )
                {
                   if (P_testread(USERTREE) )
                   {
                      char msg[MAXPATH];
                      strcpy(msg,"exptime: usertree is empty");
                      if (retc == 0)
                         Werrprintf(msg);
                      else if (retc > 1)
                         retv[1] = newString(msg);
                      return(0);
                   }
                   whichTree = USERTREE;
                }
                else if ( timeFlag &&  ! access(argv[n], R_OK) )
                {
                   whichTree = TEMPORARY;
                   P_treereset(TEMPORARY);
                   if (P_read(TEMPORARY,argv[n]) || P_testread(TEMPORARY) )
                   {
                      char msg[2*MAXPATH];
                      sprintf(msg, "exptime: could not get parameter %s",argv[n]);
                      if (retc == 0)
                         Werrprintf(msg);
                      else if (retc > 1)
                         retv[1] = newString(msg);
                      P_treereset(TEMPORARY);
                      whichTree = CURRENT;
                      return(0);
                   }
                }
                else if ((sfile == NULL) && (strcmp(argv[n],"again") != 0) )
		    sfile = argv[n];
	     }
	     n++;
	}
	if (tbug) 
	{
	   if (sfile != NULL)
		strcpy(dpsfile, sfile);
	   else
                strcpy(dpsfile, "/tmp/testdata");
           return(1);
	}
	if (sfile == NULL)
        {  
    	   if (P_getstring(CURRENT, "seqfil", dpsfile, 1, MAXPATH-1))
           {   Werrprintf("dps error: parameter 'seqfil' does not exist");
               return(0);
           }
	   sfile = dpsfile;
	}
        else
	   strcpy(dpsfile, sfile);

        if (*sfile != '/')
        {   
            char    psgJname[MAXPATH];
            char    psgJpath[MAXPATH];
            int     jFound, cFound;

            strcpy(psgJname,sfile);
            strcat(psgJname,".psg");
            jFound = appdirFind(psgJname, "seqlib", psgJpath, "", R_OK|X_OK|F_OK);
            cFound = appdirFind(sfile, "seqlib", psgfile, "", R_OK|X_OK|F_OK);
            if ( (cFound == 0) && (jFound == 0) )
            {
                strcpy(psgfile, sfile);
                if ( ! timeFlag)
                   if (retc == 0)
	              Werrprintf("dps error: file '%s(.psg)' does not exist", psgfile);
                return(0);
            }
            if (cFound == 0)
            {
                /* No C PSG, only JPSG found */
                strcpy(psgfile, psgJpath);
            }
            else if ((jFound) && (jFound <= cFound) )
            {
                /* Both found, and Jpsg is earliest in search path */
               strcpy(psgfile, psgJpath);
            }
            return(1);
	}
	else
        {
	    strcpy (psgfile, sfile);
            if (access(psgfile, 0) == 0)
               return(1);
        }
        {
            char msg[MAXPATH];

            sprintf(msg,"dps error: sequence '%s' does not exist", psgfile);
            if (retc == 0)
               Werrprintf(msg);
            else if (retc > 1)
               retv[1] = newString(msg);
        }
        return(0);
}	

static void
remove_dps_file(char *name)
{
        char fname[MAXPATH];

        sprintf(fname, "%s/%s", curexpdir, name);
	unlink(fname);
}


static int
exec_sequence(int firstime, int argc, char *argv[])
{
    int     child;
    int     ret;
    int     ready;
    int     pipe1[2];
    int     pipe2[2];
    int     suflag;   /* setup flag,0=GO,1-6=different alias's of GO */
    /* --- child and pipe variables --- */
    char    pipe1_0[3];
    char    pipe1_1[3];
    char    pipe2_0[3];
    char    pipe2_1[3];
    int     psg_busted; /* see go.c */

    if (debug > 1)  
	fprintf(stderr, "DPS: execute sequence\n");
    /* now we need to take care array stuff */
    if (run_calcdim())
    {
       if (debug > 1)  
	   fprintf(stderr, " run_calcdim failed, terminated.\n");
       Werrprintf("dps error: arraydim calculation failed, terminated. ");
       return(0);
    }

    /*------------------------------------------------------------
     * --- Fork & Exec DPS, then pipe parameter over to DPS 
     * --- sends over parameters through the pipe to
     * --- the child and returns to vnmr without waiting. 
     *------------------------------------------------------------*/
    suflag = 0;
    P_creatvar(CURRENT, "goid", ST_STRING);
    P_setgroup(CURRENT, "goid", G_ACQUISITION);
    if (firstime)
    {
	strcpy(argv[0], "dps");
	if (initacqqueue(argc,argv))
    	{
            if (debug > 1)  
	        fprintf(stderr, " initacqqueue failed, terminated.\n");
            Werrprintf("dps error: initialization failed, terminated. ");
            disp_acq("");
            return(0);
    	}
    }

    P_creatvar(CURRENT, "com$string", ST_STRING);
    P_setgroup(CURRENT, "com$string", G_ACQUISITION);
    P_creatvar(CURRENT,"appdirs",ST_STRING);
    P_setgroup(CURRENT,"appdirs",G_ACQUISITION);
    P_setstring(CURRENT,"appdirs",getAppdirValue(),1);
    P_creatvar(CURRENT,"when_mask",ST_INTEGER);
    P_setgroup(CURRENT,"when_mask",G_ACQUISITION);
    P_setreal( CURRENT,"when_mask", 0.0, 1 );
    getVnmrInfo(0,0,0);

    ret = pipe(pipe1); /* make first pipe */
    /*
     *  The first pipe is used to send parameters to DPS 
     */
    if(ret == -1)
    {   Werrprintf("dps error: could not create system pipes!");
        return(0);
    }
    ret = pipe(pipe2); /* make second pipe */
    /*
     *  The second pipe is used to cause go to wait for DPS to
     *  complete.  This is only used in automation mode.
     */
    if(ret == -1)
    {   Werrprintf("dps error: could not create system pipes!");
        return(0);
    }
    child = fork();  /* fork a child */
    if (child != 0)	/* if parent set signal handler to reap process */
        set_wait_child(child);

    if (child == 0)
    {	char suflagstr[10];
        char Rev_Num[10];

        sprintf(pipe1_0,"%d",pipe1[0]);
        sprintf(pipe1_1,"%d",pipe1[1]);
        sprintf(pipe2_0,"%d",pipe2[0]);
        sprintf(pipe2_1,"%d",pipe2[1]);
        sprintf(suflagstr,"%d",suflag);
        sprintf(Rev_Num,"%d",GO_PSG_REV);

        if (debug > 1)  
	    fprintf(stderr, "DPS: execute psg %s\n", psgfile);
        ret = execl(psgfile,dpscmd,Rev_Num,pipe1_0,pipe1_1,pipe2_0,pipe2_1,suflagstr,NULL);

        if (debug > 1)  
	    fprintf(stderr, "DPS: execute psg failed.\n");
	Werrprintf("dps error: %s could not be executed", psgfile);
	return(0);
    }

    close(pipe1[0]);  /* parent closes its read end of first pipe */
    close(pipe2[1]);  /* parent closes its write end of second pipe */

    psg_busted = 0;   /* see go.c */
    catch_sigpipe();  /* see go.c */
    block_sigchld();  /* see go.c */
    if (setjmp( brokenpipe ) == 0)
    {
        if (debug > 1)  
          fprintf(stderr, "DPS: send vars to sequence\n");
        P_sendGPVars(SYSTEMGLOBAL,G_ACQUISITION,pipe1[1]);
        P_sendGPVars(GLOBAL,G_ACQUISITION,pipe1[1]);/* send global tree to DPS */ 
        P_sendVPVars(GLOBAL,"curexp",pipe1[1]);
		/* send current experiment directory to DPS */ 
        P_sendVPVars(GLOBAL,"userdir",pipe1[1]);
		/* send user directory to DPS */ 
        P_sendVPVars(GLOBAL,"systemdir",pipe1[1]);
		/* send system directory to DPS */
        P_sendGPVars(CURRENT,G_ACQUISITION,pipe1[1]);/* send current tree to DPS */ 
        P_endPipe(pipe1[1]);   /* send end character */
        if (debug > 1)  
          fprintf(stderr, "DPS: end send vars to sequence\n");
    }
    else {
        if (debug > 1)  
          fprintf(stderr, "DPS: pulse sequence failed to start.\n");
       Werrprintf( "%s: pulse sequence failed to start", argv[ 0 ] );
       Wscrprintf( "Check your swap space and the shared libraries that your pulse sequence uses\n" );
       psg_busted = 1;
    }

    restore_sigchld();/* see go.c */
    restore_sigpipe();/* see go.c */

    close(pipe1[1]);  /* parent closes its write end of first pipe */

    if (timeFlag)
        ready = 1;
    else
        ready = 0;
    if (psg_busted == 0)
    {
        if (debug > 1)  
          fprintf(stderr, "DPS: read psg message ... \n");
	inputs[0] = '0';
	inputs[1] = '\0';
	while(1)
	{
          sizeT = read(pipe2[0],inputs, MAXPATH);
          if (debug > 1)  
             fprintf(stderr, "DPS read psg status:  %zu chars\n", sizeT);
          if(sizeT <= 0)
          {
             if ( (sizeT == -1) && (errno == EINTR))
                continue;
             else
             {
                if (debug > 1)  
                     fprintf(stderr, "DPS read psg interrupted.\n");
                break;
             }
          }
          else
          {
             ready = 1;
	     inputs[(int)sizeT] = '\0';
          }
/*
	  inputs[(int)sizeT] = '\0';
    	  if (debug)  
		fprintf(stderr, "  read from psg => %s\n", inputs);
*/
          // 1: new dps
          // 2: dps time
          // 3: dps redo

	  if (inputs[0] == '1')
          {
		disp_new_psg();
          }
	  else if (inputs[0] == '2')
	  {
		sscanf(inputs, "%*d%lg", &expTime);
	 	disp_dps_time();
/*
    	  	if (debug)  
		   fprintf(stderr, "  exptime: %f\n", expTime);
		else
*/
		   remove_dps_file(DPS_TIME);
	  }
	}
        if (debug > 1)  
          fprintf(stderr, "DPS: read psg message done. \n");
        if (ready < 1)
          Werrprintf( "dps error: run '%s' pulse sequence failed.", psgfile);
    }
    close(pipe2[0]);  /* parent closes its read end of first pipe */
    cleanup_pars();
    if (debug > 1)  
        fprintf(stderr, "DPS: psg was finished  %d \n", ready);
    return(ready);
}



static Pixel
get_default_pixels(char *name)
{
#ifdef MOTIF
        XColor   xcolor, xcolor2;

	if (!xwin)
	   return (99);
	if(cmap == 0)
	   cmap = XDefaultColormap(xdisplay, DefaultScreen(xdisplay));
	if (name == NULL)
	   return (99);
	if(XAllocNamedColor(xdisplay, cmap, name, &xcolor, &xcolor2))
	   return (xcolor.pixel);
	else
#endif
	   return (99);
}


static void
change_dps_defaults()
{
     int	k;

     if (!Wissun() || Bnmr)
	return;
     k = 0;

     for (k = 0; k < COLORS; k++)
     {
	if (colorChanged[k])
	{
	    if (k == ICOLOR || k == FCOLOR) {
#ifdef VNMRJ
	        if (!xwin)
		     vnmrj_color_name(k+vjColor, dpsColors[k]);
		else
#endif
	           dpsPix[k] = get_default_pixels(dpsColors[k]);
	    }
	    else {
#ifdef VNMRJ
	        if (!xwin)
		     vnmrj_color_name(k+vjColor, dpsColors[k]);
		else
#endif
	            dpsPix[k] = get_vnmr_pixel_by_name(dpsColors[k]);
	    }
	}
	colorChanged[k] = 0;
     }
     if (!xwin)
	return;

#ifdef MOTIF
     if (ndpsFont != NULL)
     {
	if (dpsFont != NULL)
	{
	   if (strcmp(dpsFont, ndpsFont) != 0)
		free(dpsFont);
	   else
	   {
		free(ndpsFont);
		ndpsFont = NULL;
	   }
	}
	if (ndpsFont)
	{
	   dpsFont = ndpsFont;
	   ndpsFont = NULL;
	   if (dfontInfo)
	   {
		XUnloadFont(xdisplay, dfontInfo->fid);
                XFreeFontInfo(NULL, dfontInfo, 1);
	   }
	   if((dfontInfo = XLoadQueryFont(xdisplay, dpsFont)) == NULL)
               dfontInfo = XLoadQueryFont(xdisplay, "8x13");
	   dch_ascent = dfontInfo->max_bounds.ascent;
           dch_descent = dfontInfo->max_bounds.descent;
           dch_h = dch_ascent + dch_descent;
           dch_w = dfontInfo->max_bounds.width;
	   set_font(dfontInfo->fid);

	   pfx = dch_w;
	   pfy = dch_h;
	   chascent = dch_ascent;
	   chdescent = dch_descent;
	}
     }
     if (ntextFont != NULL)
     {
	if (textFont != NULL)
	{
	   if (strcmp(textFont, ntextFont) != 0)
		free(textFont);
	   else
	   {
		free(ntextFont);
		ntextFont = NULL;
	   }
	}
	if (ntextFont)
	{
	   textFont = ntextFont;
	   ntextFont = NULL;
	   if (tfontInfo)
	   {
		XUnloadFont(xdisplay, tfontInfo->fid);
                XFreeFontInfo(NULL, tfontInfo, 1);
	   }
	   if((tfontInfo = XLoadQueryFont(xdisplay, textFont)) == NULL)
               tfontInfo = XLoadQueryFont(xdisplay, "8x13");
	   if (tfontInfo)
		infoFontList = XmFontListCreate (tfontInfo, "text");
           text_ascent = tfontInfo->max_bounds.ascent;
	   text_descent = tfontInfo->max_bounds.descent;
	   text_h = text_ascent + text_descent;
	   text_w = tfontInfo->max_bounds.width;

	   scan_y = text_h;
	   info_y = text_h * 2 + text_descent + 2;
	   if (dps_text_gc)
	       XSetFont(xdisplay, dps_text_gc, tfontInfo->fid);

#ifndef VNMRJ
	   if (tfontInfo)
	       recreate_array_index_widget();
#endif 
	}
     }
     if (menuUp)
     {
	popup_dps_pannel();
	if (hilitNode)
            disp_info(hilitNode);
     }
#endif  /* MOTIF */
}


static  char *dps_attrs[] = {"font", "color", "region", "clear", "chan_name", "file_name"};
static  char *colorItem[] = {"back","pulse", "base", "highlight", "second",
			"msecond", "usecond", "info", "command", "delay",
			"label", "power", "phase" };

/*  dps defaults:
*
*  font    {graphics,plotter}  {graph,info}        font_name
*  color   {graphics,plotter}  colorItem           color_name
*  region  {graphics,plotter}  {x,y,width,height}  value
*  file_name  plotter  {1, 0}
*/


static void
set_dps_attr(argc, argv)
int	argc;
char    **argv;
{
	int	window, id, k, n;
     	double   fval;

        n = sizeof(dps_attrs) / sizeof(*dps_attrs);
	for (id = 0; id < n; id++)
	{
	    if (strcmp(dps_attrs[id], argv[0]) == 0)
		break;
	}
	if (id >= 6)
	{
	     Werrprintf("dps: unknown argument(0): '%s'", argv[0]);
	     return;
	}
	window = 1;
	if (argv[1][0] < '0' || argv[1][0] > '9')
	{
	    if (strcmp(argv[1], "plotter") == 0)
		   window = 0;
	}
	else
	    window = atoi(argv[1]);
	if (window < 0 || window > 1)
	{
	     Werrprintf("dps: unknown argument(1): '%s'", argv[1]);
	     return;
	}
	switch (id) {
	  case  0:  /* font*/
		  if (argc < 4)
		        return;
		  id = 0;
		  if (argv[2][0] < '0' || argv[2][0] > '9')
		  {
		 	if (strcmp(argv[2], "graph") == 0)
			    id = 0;
		 	else if (strcmp(argv[2], "info") == 0)
			    id = 1;
		  }
		  else
		        id = atoi(argv[2]);
		  k = 1;
		  if(id == 0)  /* graph font */
		  {
			if (dpsFont != NULL)
			{
			    if (strcmp(dpsFont, argv[3]) == 0)
				k = 0;
			}
			if (k)
			{
			    ndpsFont = (char *) malloc(strlen(argv[3]) + 1);
			    strcpy(ndpsFont, argv[3]);
			}
		  }
		  else if(id == 1)  /* text font */
		  {
			if (textFont != NULL)
			{
			    if (strcmp(textFont, argv[3]) == 0)
				k = 0;
			}
			if (k)
			{
			    ntextFont = (char *) malloc(strlen(argv[3]) + 1);
			    strcpy(ntextFont, argv[3]);
			}
		  }
		  else  /* button font */
		  {
		        if (buttonFont != NULL)
			{
			    if (strcmp(buttonFont, argv[3]) == 0)
				k = 0;
			}
			if (k)
			{
			    nbuttonFont = (char *) malloc(strlen(argv[3]) + 1);
			    strcpy(nbuttonFont, argv[3]);
			}
		  }
		  break;
	  case  1:  /* color */
		  if (argc < 4)
		        return;
		  k = atoi(argv[2]);
		  if (argv[2][0] < '0' || argv[2][0] > '9')
		  {
                     n = sizeof(colorItem) / sizeof(*colorItem);
		     for(k = 0; k < n; k++)
		     {
			if (strcmp(colorItem[k], argv[2]) == 0)
			    break;
		     }
		  }
		  else
		     k = atoi(argv[2]);
		  if (k < COLORS)
		  {
		      if (dpsColors[k] != NULL)
		      {
			if (strcmp(dpsColors[k], argv[3]) != 0)
			{
			    free(dpsColors[k]);
			    dpsColors[k] = NULL;
			}
		      }
		      if (dpsColors[k] == NULL)
		      {
			dpsColors[k] = (char *) malloc(strlen(argv[3])+1);
			strcpy(dpsColors[k], argv[3]);
	    		colorChanged[k] = 1;
		      }
		  }
		  break;
	  case  2:  /* region */
		  if (argc < 4)
		        return;
		  k = 4;
		  if (argv[2][0] < '0' || argv[2][0] > '9')
		  {
		     if (strcmp(argv[2], "x") == 0)
			k = 0;
		     else if (strcmp(argv[2], "y") == 0)
			k = 1;
		     else if (strcmp(argv[2], "width") == 0)
			k = 2;
		     else if (strcmp(argv[2], "height") == 0)
			k = 3;

		  }
		  else
		     k = atoi(argv[2]);
		  if (k > 3)
		  {
	     	     Werrprintf("dps: unknown argument(2): '%s'", argv[2]);
		     return;
		  }
		  fval = atof(argv[3]);
		  if (fval < 0.0)
		     fval = 0.0;
		  switch (k) {
		     case  0:
			     if (window)
				   winX = fval;
			     else
				   plotX = fval;
			     break;
		     case  1:
			     if (window)
				   winY = fval;
			     else
				   plotY = fval;
			     break;
		     case  2:  /* width */
			     if (window)
				   winW = fval;
			     else
				   plotW = fval;
			     break;
		     case  3: /* height */
			     if (window)
				   winH = fval;
			     else
				   plotH = fval;
			     break;
		  }
		  break;
	  case  3:  /* clean */
		  if (argc < 3)
		        return;
		  if (window)
		     clearDraw = atoi(argv[2]);
		  else
		     clearPlot = atoi(argv[2]);
		  break;
	  case  4:  /* channel name */
		  if (argc < 3)
		        return;
		  if (window)
		     winChanName = atoi(argv[2]);
		  else
		     plotChanName = atoi(argv[2]);
		  break;
	  case  5:  /* file name */
		  if (argc < 3)
		        return;
		  if (!window)
		     plotName = atoi(argv[2]);
		  break;
	}
}


static void
read_dps_defaults()
{
     int     k, quote, vargc;
     char    *data, *Vargv[6];
     FILE    *fd;

     sprintf(inputs, "%s/templates/dps/setup",userdir);
     if ((fd = fopen(inputs, "r")) == NULL)
     {
           sprintf(inputs, "%s/user_templates/dps/setup",systemdir);
     	   fd = fopen(inputs, "r");
     }
     if (fd == NULL)
	 return;
     while ((data = fgets(inputs, 200, fd)) != NULL)
     {
	k = strlen(data) - 1;
	if (k > 4)
	{
	    data = &inputs[k];
	    while (*data == '\n')
	    {
	    	*data = '\0';
	    	data--;
	    }
	}
	else
	    continue;
	data = inputs;
	vargc = 0;
	while (*data != '\0')
        {
            while (*data == ' ' || *data == '\t')
                data++;
	    if (*data == '\0')
		break;
	    quote = 0;
	    if (*data == '\"')
	    {
		quote = 1;
		while (*data == '\"')
		    data++;
		while (*data == ' ')
		    data++;
	    }
            Vargv[vargc] = data;
            vargc++;
            if (vargc > 5)
                break;
            while (*data != '\0')
            {
		if (quote)
		{
                    if ((*data == '\"') && (*(data - 1) != '\\'))
                    {
                       *data = '\0';
                       data++;
                       break;
		    }
                }
                else
		{
		    if (*data == ' ' || *data == ',')
		    {
                       *data = '\0';
                       data++;
                       break;
		    }
		}
		data++;
	    }
        }
	if (vargc > 2)
	    set_dps_attr(vargc, Vargv);
     }
     fclose(fd);
}


/*********
static void
read_vj_defaults()
{
     char    name[32], value[32];
     char    *data;
     FILE    *fd;

     sprintf(inputs, "%s/persistence/DisplayOptions",userdir);
     if ((fd = fopen(inputs, "r")) == NULL)
     {
	  return;
     }
     while ((data = fgets(inputs, 200, fd)) != NULL)
     {
	 if (sscanf(data,"%s%s", name, value) != 2)
	    continue;
	if (strncmp(name, "Dps", 3) == 0) {
	    set_dps_color(name, value); 
	}
     }

     fclose(fd);
}
**********/

static void
init_dps_default()
{
#ifdef SUN
	int	num;
	char	*data;

	plotName = 1;
	plotChanName = 1;
	winChanName = 1;
	buttonFont = NULL;
	dfontInfo = NULL;
	dpsFont = NULL;
	textFont = NULL;
	nbuttonFont = NULL;
	ndpsFont = NULL;
	ntextFont = NULL;
        if (Bnmr)
	    return;
	for (num = 0; num < COLORS; num++)
	{
	    if (dpsColors[num] == NULL)
	    {
	       dpsColors[num] = (char *) malloc(strlen(defColors[num]) + 1);
	       strcpy(dpsColors[num], defColors[num]);
	       colorChanged[num] = 1;
	    }
	}
	for (num = 0; num < ARRAYMAX; num++)
	    array_i_widgets[num] = 0;
        read_dps_defaults();
        read_dps_resource();
	if (ndpsFont == NULL)
	{
	  if ((data = get_dps_resource("Vnmr*dps*graph", "font")) == NULL)
	    data = get_dps_resource("Vnmr*dps*graph", "fontList");
	  if (data == NULL)
	  {
	    ndpsFont = (char *) malloc(5);
	    strcpy(ndpsFont, "8x13");
	  }
	  else
	  {
	    ndpsFont = (char *) malloc(strlen(data) + 1);
	    strcpy(ndpsFont, data);
	  }
	}
	if (ntextFont == NULL)
	{
	  if ((data = get_dps_resource("Vnmr*dps*info", "font")) == NULL)
	    data = get_dps_resource("Vnmr*dps*info", "fontList");
	  if (data == NULL)
	  {
	    ntextFont = (char *) malloc(5);
	    strcpy(ntextFont, "8x13");
	  }
	  else
	  {
	    ntextFont = (char *) malloc(strlen(data) + 1);
	    strcpy(ntextFont, data);
	  }
	}
	if (nbuttonFont == NULL)
	{
	  if ((data = get_dps_resource("Vnmr*dps*button", "font")) == NULL)
	    data = get_dps_resource("Vnmr*dps*button", "fontList");
	  if (data == NULL)
	  {
	    nbuttonFont = (char *) malloc(5);
	    strcpy(nbuttonFont, "8x13");
	  }
	  else
	  {
	    nbuttonFont = (char *) malloc(strlen(data) + 1);
	    strcpy(nbuttonFont, data);
	  }
	}
	change_dps_defaults();
#ifdef VNMRJ
	// read_vj_defaults();
	writelineToVnmrJ("dps", "window on");
        if (P_getstring( GLOBAL, "appmode", inputs, 1, MAXPATH ) < 0)
	    strcpy(inputs, "");
        if (strcmp(inputs, "imaging") == 0)
            writelineToVnmrJ("dps", "view decoupling on");
        else {
            dispMode = dispMode | DSPDECOUPLE;
            writelineToVnmrJ("dps", "view decoupling off");
        }
        if (rfShapeType == REAL_SHAPE)
            writelineToVnmrJ("dps", "rfshape Real");
        else if (rfShapeType == ABS_SHAPE)
            writelineToVnmrJ("dps", "rfshape Absolute");
        else if (rfShapeType == IMAGINARY_SHAPE)
            writelineToVnmrJ("dps", "rfshape Imaginary");
        set_dps_color("DpsFuncColor", "0x606060");
#endif 
        set_dps_color("DpsSimColor", "0x606060");
#endif 
}

#ifndef VNMRJ
static  void
save_org_image()
{
	int	depth;
#ifdef MOTIF
	if (!Wissun())
	    return;
	if (!xwin)
	    return;
	if (backMap == 0 || backW < dpsW || backH < dpsH)
	{
	    if (backMap)
	    	XFreePixmap(xdisplay, backMap);
	    depth = DefaultDepth(xdisplay, XDefaultScreen(dpy));
	    backW = (dpsW / 8 + 1) * 8;
	    backH = (dpsH / 8 + 1) * 8;
	    backX = dpsX;
	    backMap = XCreatePixmap(xdisplay, xid, backW, backH, depth);
	}
	if (backMap)
	{
	    clear_cursor();
	    backX = dpsX;
	    backY = mnumypnts - dpsY2;
	    if (canvasPixmap)
		XCopyArea(xdisplay,canvasPixmap, backMap,vnmr_gc, dpsX, backY, 
				backW, backH, 0, 0);
	    else
		XCopyArea(xdisplay,xid, backMap,vnmr_gc, dpsX, backY, 
				backW, backH, 0, 0);
	}
#endif /* MOTIF */
}
#endif 

static void
run_scopeview()
{
#ifdef VNMRJ
     if (disp_start_node == NULL)
         return;
     if ((image_flag & OBIMAGE) && !(image_flag & MAIMAGE))
         dpsx_init(1, debug, psgfile);
     else
         dpsx_init(0, debug, psgfile);
     dpsx_setValues(TODEV, TOTALCH, rfchan);
     dpsx_setValues(DEVON, TOTALCH, visibleCh);
     dpsx_setDoubles(RF_POWERS, RFCHAN_NUM, powerMax);
     dpsx_setDoubles(GRAD_POWERS, GRAD_NUM, gradMax);

     dpsx(disp_start_node);
#endif 
}

static void
clear_dps_window()
{
       if (dpsPlot)
           return;
#ifdef VNMRJ
        sunGraphClear();
        set_background_region(0, 0, graf_width, graf_height, BOX_BACK, 100);
       /**
        color(BOX_BACK);
        amove(0, 0);
        box(graf_width, graf_height);
       **/
#else
        clear_pixmap(0, 0, graf_width, graf_height);
#endif


}

static void
batch_draw()
{
	if (src_start_node == NULL || disp_start_node == NULL)
	    return;
#ifdef SUN
	clear_cursor();
	if (!Wissun())
	    return;
	if (!dpsPlot && !advancedDps)
 	    mnumypnts = graf_height;
/*
	if (!xwin)
	    clearDraw = 1;
*/
	clearDraw = 1;

	if (clearDraw)
	{
	    graph_batch(1);
	    if (xwin)
	       clearPixmap();
	    else
               clear_dps_window();
       	    draw_dps();
	    graph_batch(0);
	}
	else
	{
#ifdef MOTIF
	    if (backMap)
	    {
		if (canvasPixmap)
		    XCopyArea(xdisplay,backMap,canvasPixmap,vnmr_gc, 0, 0, 
				backW, backH, dpsX, backY);
		else
	            XCopyArea(xdisplay,backMap,xid,vnmr_gc, 0, 0, backW, backH,
			 	dpsX, backY);
	    }
	    else
	    {
	        xormode();
	        xorFlag = 1;
	    }
/*
	    if (canvasPixmap)
*/
		graph_batch(1);
       	    draw_dps();
	    normalmode();
	    xorFlag = 0;
/*
		graph_batch(0);
*/
#endif
	}
	draw_menu_icon();
#else 
       	draw_dps();
#endif 
}



static  void
clean_dps_house()
{
#ifdef SUN
    	stop_dps_browse();
#endif 

	clean_src_data();
    	if (array_start_node != NULL)
    	{
             releaseArray();
	     array_start_node = NULL;
    	}
    	if (apstart != NULL)
    	{
             releaseAp();
	     apstart = NULL;
    	}
        releaseTmpNode();
#ifdef SUN
	clear_array_list();
	clean_dps_info_win();
#endif 
    	newArray = 1;
	hilitNode = NULL;
}

static void
open_dps_menu()
{
#ifdef SUN
        if (Wissun())
	{
#ifndef VNMRJ
	    if (dpsShell == NULL)
	     	    create_dps_pannel();
	    if (menuUp)
	      	    popup_dps_pannel();
#else 
	    writelineToVnmrJ("dps", "window on");
	    sprintf(info_data, "title dps: %s", psgfile);
	    writelineToVnmrJ("dps", info_data);
	    send_info_to_vj();
#endif 
	    register_dismiss_proc("dps", close_dps_menu);
        }
#endif 
        Wactivate_mouse(mouse_move, mouse_but, NULL);
}

static void
run_dps(int origin)
{

    if (create_dps_info(origin))
    {
        if (origin && !dpsPlot && !jout && !advancedDps)
        {
   	   Wshow_graphics();
           Wgmode(); /* goto tek graphics and set screen 2 active */
	   set_turnoff_routine(turnoff_dps);
   	   Wsetgraphicsdisplay("dps");
           execString("menu('main')\n");
#ifndef VNMRJ
	   if (!clearDraw)
	      save_org_image();
#endif 
        }
	if (!Wissun() || dpsPlot)
           draw_dps();
	else if (!jout && advancedDps == 0)
           batch_draw();
        if (origin && !dpsPlot && !jout)
	{
           if (advancedDps == 0)
               open_dps_menu();
           else
               run_scopeview();
        }    
    }
}


static void
exec_vj_cmd(argc,argv)
int argc;
char *argv[];
{
    int k, num;

    if (argc < 3)
	return;
    
#ifdef VNMRJ
    if (isJprintMode())
	return;
#endif 
    vjControl = 1;
    if (strcmp(argv[2], "button") == 0) {
        if (argc < 4)
	    return;
        if (strcmp(argv[3], "Stop") == 0) {
		do_but_proc(STOPTIMER);
		return;
	}
        if (strcmp(argv[3], "Redraw") == 0) {
		do_but_proc(RDRAW);
		return;
	}
        if (strcmp(argv[3], "scopeview") == 0) {
                run_scopeview();
		return;
	}
	for (k = 0; k < BUTNUM1; k++) {
          if (strcmp(argv[3], but_label_1[k]) == 0) {
		do_but_proc(but_val_1[k]);
		return;
	  }
	}
	for (k = 0; k < BUTNUM2; k++) {
          if (strcmp(argv[3], but_label_2[k]) == 0) {
		do_but_proc(but_val_2[k]);
		return;
	  }
	}
	return;
    }
    if (strcmp(argv[2], "class") == 0) {
        if (argc < 4)
	    return;
	num = -1;
	if (strcmp(argv[3], "time") == 0)
	    num = TIMEMODE;
	else if (strcmp(argv[3], "power") == 0)
	    num = POWERMODE;
	else if (strcmp(argv[3], "phase") == 0)
	    num = PHASEMODE;
	if (num > 0) {
	    do_class_proc(num);
	    save_dps_resource();
        }
	return;
    }
    if (strcmp(argv[2], "mode") == 0) {
        if (argc < 5)
	    return;
	k = atoi(argv[4]);
	num = -1;
	if (strcmp(argv[3], "label") == 0)
	    num = DSPLABEL;
	else if (strcmp(argv[3], "value") == 0)
	    num = DSPVALUE;
	else if (strcmp(argv[3], "more") == 0)
	    num = DSPMORE;
	else if (strcmp(argv[3], "decoupling") == 0)
	    num = DSPDECOUPLE;
	if (num > 0) {
	    do_mode_proc(num, k);
	    save_dps_resource();
        }
	return;
    }
    if (strcmp(argv[2], "array") == 0) {
	update_vj_array(argc, argv);
	return;
    }
    if (strcmp(argv[2], "color") == 0) {
	if (argc < 5)
	   return;
	// set_dps_color(argv[3], argv[4]);
	// do_but_proc(DRAW);
	return;
    }
    if (strcmp(argv[2], "transient") == 0) {
	k = atoi(argv[3]);
        if (k > 0 && k <= sys_nt) {
            transient = k;
            exec_transient();
        }
	return;
    }
    if (strcmp(argv[2], "rfshape") == 0) {
        if (argc < 4)
	    return;
        k = rfShapeType;
        rfShapeType = ABS_SHAPE;
        if (strcmp(argv[3], "real") == 0)
            rfShapeType = REAL_SHAPE;
        else if (strcmp(argv[3], "imaginary") == 0)
            rfShapeType = IMAGINARY_SHAPE;
        if (rfShapeType != k) {
	    save_dps_resource();
            batch_draw();
        }
	return;
    }
}

int
dps(int argc, char *argv[], int retc, char *retv[])
{
    double   etime;
    int	    k, reDraw;

    if (Bnmr)
    {
	if (strcmp(argv[0], "dps") == 0 || strcmp(argv[0], "dpsn") == 0)
            RETURN;
    }
    reDraw = 0;
    advancedDps = 0;

    if (debug > 0)
	fprintf(stderr, " dps( ");
    for (k = 1; k < argc; k++) {
       if (debug > 0)
	   fprintf(stderr, "%s, ", argv[k]);
       if (strstr(argv[k], "redisplay") != NULL) {
           if (strstr(argv[k], "param") != NULL)
               RETURN;
       }
       if (strcmp(argv[k], "again") == 0) {
            if (src_start_node == NULL)
                RETURN;
            reDraw = 1;
       }
    }
    if (debug > 0)
	fprintf(stderr, ")\n");

    xwin = 1;
#ifdef VNMRJ
    xwin = useXFunc;
#endif 
    ixNum = 1;
    scan_args(argc, argv);
    if (firstDps && !timeFlag && !dpsPlot)
    {
        strcpy (psgfile, "");
	init_dps_default();
        reDraw = 0;
	firstDps = 0;
    }
    if (argc > 1 && (strcmp(argv[1], "-vj") == 0))
    {
	exec_vj_cmd(argc,argv);
        // removeGraphFunc();
	RETURN;
    }
    if (argc > 1 && (strcmp(argv[1], "-setup") == 0))
    {
        // removeGraphFunc();
	if (argc > 4)
	    set_dps_attr(argc - 2, argv + 2);
	else
            read_dps_defaults();
	showTimeMark = 1;
	change_dps_defaults();
	if (draw_start_node)
	{
	    if (clearDraw == 0 && backMap == 0)
		batch_draw();
	    batch_draw();
	}
        RETURN;
    }
    // debug = 0;
    jout = 0;
    tbug = 0;
    parallel = 0;
#ifdef VNMRJ
	mouse_x = -1;
	mouse_x2 = -1;
#endif 
    jline = 0;
    if (fin != NULL)
	fclose(fin);
    fin = NULL;
    if (jfout != NULL)
	fclose(jfout);
    jfout = NULL;
#ifdef VNMRJ
    if (fdps != NULL)
	fclose(fdps);
    fdps = NULL;
    isNvpsg = nvAcquisition();
#endif 
    if (reDraw) {
        // Wgetgraphicsdisplay(inputs, 12);
        // if (strcmp(inputs, "dps") == 0) {
        xOffset = 0;
        xMult = 1.0;
        setdisplay();
        dps_config();
        batch_draw();
        open_dps_menu();
   	Wsetgraphicsdisplay("dps");
        execString("menu('main')\n");
        RETURN;
    }
    if (!check_args(argc, argv, retc, retv))
    {
         if (retc > 0)
             retv[ 0 ] = newString("-1.0");
         RETURN;
    }
    k = strlen(psgfile);
    if ( ! strcmp(psgfile+k-3,"psg") )
    {
       JFlag = 1; /* jpsg */
    }
    if (timeFlag)
    {
        removeGraphFunc();
        sprintf(dpscmd, "dpst%d", debug);  /*  only get time info  */
        if (whichTree != CURRENT)
           P_exch(CURRENT, whichTree);
    }
    else
    {
	clean_dps_house();
	if (dpsPlot)
	{
            removeGraphFunc();
    	    strcpy(dpscmd, "dpsp0");
	}
	else
	{
	    if (debug)
    	       sprintf(dpscmd, "dpsd%d", debug);
	    else
    	       strcpy(dpscmd, "dps");
            if (advancedDps == 0) {
               setdisplay();
               clear_dps_window();
	       Wturnoff_mouse();
   	       Wsetgraphicsdisplay("");
            }
	    if (Wissun())
		winBack = get_vnmr_pixel(BACK);
            expTime = -3.0;
	    first_round = 1;
	}
    }
    newAcq = 0;
    if (JFlag)
    {
       acq(argc,argv,0,NULL);
       if ( ! timeFlag) disp_new_psg();
    }
    else
    {
       if (tbug)
       {
          disp_new_psg();
       }
       else if (!exec_sequence(1, argc,argv))
       {
	   remove_dps_file(DPS_TIME);
           if (debug == 0)
	      remove_dps_file(DPS_DATA);
	   remove_dps_file(DPS_TABLE);
    	   if (retc > 0)
	   {
               retv[ 0 ] = newString("-1.0");
	   }
           if ( !timeFlag && !dpsPlot && !advancedDps)
	       Wturnoff_mouse();
           if (whichTree != CURRENT)
              P_exch(CURRENT, whichTree);
           if (whichTree == TEMPORARY)
              P_treereset(TEMPORARY);
           whichTree = CURRENT;
           RETURN;
       }
    }
    if (timeFlag)
    {
        etime = get_exp_time();
        if (retc < 1)
        {
	    convert_time_val(etime);
            Winfoprintf("exptime is %s",inputs);
        }
        else
	{
            sprintf(info_data, "%f", etime);
            retv[ 0 ] = newString(info_data);
	}
        if (whichTree != CURRENT)
           P_exch(CURRENT, whichTree);
        if (whichTree == TEMPORARY)
           P_treereset(TEMPORARY);
        whichTree = CURRENT;
	timeFlag = 0;
    }
    else
    {
	if (expTime == -3.0)
	   expTime = get_exp_time();
	disp_dps_time();
        if (retc > 0)
             retv[ 0 ] = newString("1.0");
    }
    RETURN;
}


static void
disp_new_psg()
{
    image_flag = 0;
    xMult = 1.0;
    xOffset = 0;
    set_sys_param();
    if (dpsPlot)
    {
	Wturnoff_mouse();
        if(setplotter())
	   return;
	close_dps_menu();
    }
    run_dps(1);
    if (debug > 2)
	print_array();
}


static void
rerun_dps()
{
char	*parg[4];
char	varg[4][10];
int	pargc;
    strcpy(&varg[0][0],"dps");
    parg[0]=&varg[0][0];
    parg[1]=NULL;
    pargc=1;
    if (dpsFine) 
    {   strcpy(&varg[1][0],"-fine");
        parg[1] = &varg[1][0];
        parg[2] = NULL;
	pargc=2;
    }
    
    if (apstart != NULL)
    {
        releaseAp();
	apstart = NULL;
    }
    ixNum++;
    sprintf(dpscmd, "dps_%d%d", debug, ixNum);
    batch_draw(); /* remove the previous dps */
    if (JFlag)
    {
       acq(pargc,parg,0,NULL);
/*       if ( ! timeFlag) disp_new_psg(); */
    }
    else
    {
       if (!exec_sequence(0, 0, (char **)NULL))
       {
	   remove_dps_file(DPS_TIME);
           if (debug == 0)
	       remove_dps_file(DPS_DATA);
	   return;
       }
    }
    xMult = 1.0;
    xOffset = 0;
    set_sys_param();
    run_dps(0);
}

static void
init_rt_index()
{
        int     n;

        for(n = 0; n < RTMAX; n++)
            RTindex[n] = n;
        id2 = 0;
        id3 = 0;
        id4 = 0;
}


/* clear real time values */
static void
init_rt_values()
{
	int	n;
	int     *val, *oval;
	AP_NODE *apnode;

	for(n = 0; n < RTMAX; n++)
		RTval[n] = 0;
    	RTval[ZERO] = 0;
    	RTval[ONE] = 1;
    	RTval[TWO] = 2;
    	RTval[THREE] = 3;
	RTval[ID2] = id2;
        RTval[ID3] = id3;
        RTval[ID4] = id4;
	if (!first_round)
           RTval[IX] = 1;

	for(n = 0; n < RTDELAY; n++)
		RTdelay[n] = 0.0;
	/* restore AP table */
	apnode = apstart;
	while (apnode != NULL)
	{
	     apnode->index = 0;
	     if (apnode->changed)
	     {
		val = apnode->val;
		oval = apnode->oval;
		for(n = 0; n < apnode->size; n++)
	     	   *val++ = *oval++;
		apnode->changed = 0;
	     }
	     apnode = apnode->next;
	}
}


static int
add_grad_count(gid)
char  gid;
{
	int	ret;

	ret = 0;
	switch (gid) {
   	   case 'x':
   	   case 'X':
		     ret = 0;
		     break;
	   case 'y': 
	   case 'Y': 
		     ret = 1;
		     break;
	   case 'z': 
	   case 'Z': 
		     ret = 2;
		     break;
	   default:
		     Wprintf("dps error: '%c' is an illegal gradient\n", gid);
		     return(0);
		     break;
	}
        if (gradtype[ret] == 'n')
		  return(0);
/***
	switch (gradtype[ret]) {
	   case 'P': case 'p':
    	   case 'Q': case 'q':
    	   case 'W': case 'w':
    	   case 'L': case 'l':
    	   case 'S': case 's':
			break;
	   default:
		     Wprintf("no gradient configured on '%c'\n", gradtype[ret]);
		     return(0);
		     break;
	}
***/
	ret += GRADX;
	rfchan[ret] = 1;
	return(ret);
}


static void
reset_text_pos()
{
	int	n, k;
        SHAPE_NODE  *sh_node;

        for (n=0; n < RFCHAN_NUM; n++)
	{
            for (k = 0; k < 5; k++)
            {
                 predsx[n][k] = 3 * pfx;
                 predvx[n][k] = 3 * pfx;
	    }
        }
        k = x_margin - pfx * 2; 
	for (n = 0; n < GRAD_NUM; n++)
	{
	    gradsx[n] = k;
	    gradvx[n] = k;
	}
        sh_node = shape_list;
        while (sh_node != NULL) {
             sh_node->power = 99999; 
             sh_node = sh_node->next;
        } 
}


static void
set_y_params()
{
        int     i, n, cy, totalCh, gap, rfChans;

        if (dpsPlot)
	{
	     if (plotH < wc2max / 50.0)
		plotH = wc2max / 50.0;
	     if (plotH > wc2max)
		plotH = wc2max;
	     if (plotY + plotH > wc2max)
		plotH = wc2max - plotY;
	     dpsH = mnumypnts * (plotH / wc2max);
	     dpsY = mnumypnts * (plotY / wc2max);
             if (plotName)
	    	statusY = pfy * 3 + 10;
	     else
	        statusY = pfy + 20;
	     if (plotH < wc2max * 0.7 || plotW < wcmax * 0.7)
		lineGap = pfy * 2;
	     else
	        lineGap = pfy * 5;
	}
	else
	{
             if (Wissun() && !advancedDps)
	         mnumypnts = graf_height;
	     else
	         graf_height = mnumypnts;
	     frameH = mnumypnts;
	     if (winH < wc2max / 30.0)
		winH = wc2max / 30.0;
	     if (winH > wc2max)
		winH = wc2max;
	     if (winY + winH > wc2max)
		winH = wc2max - winY;
             dpsH = mnumypnts * (winH / wc2max); 
	     dpsY = mnumypnts * (winY / wc2max);
	     lineGap = pfy * 2;
	     mouse_y = mnumypnts - menu_height - 6;
             if (hasDock)
	         statusY = pfy;         /*   status line  */
             else
	         statusY = pfy + 10;
	}
	dpsY2 = dpsY + dpsH;
        rfChans = numch;
        if (!(dispMode & DSPDECOUPLE))
            rfChans = 1;
        if (rfChans < 1)
            rfChans = 1;
	totalCh = rfChans + rfchan[GRADX] + rfchan[GRADY] + rfchan[GRADZ] + rfchan[RCVRDEV];
	gap = pfy / 2 + 1;
        dec_v_r = 4; // space reserved for text 
        dec_s_r = 3;
	if (dpsH < 40 * pfy)  /* small window */
        {
	    if (totalCh > 2)
            {
               dec_v_r = 2;
               dec_s_r = 1;
	    }
            else
            {
               dec_v_r = 3;
               dec_s_r = 2;
	    }
        }
        else
	{
	    if (totalCh > 2)
            {
               dec_v_r = 3;
               dec_s_r = 2;
	    }
        }
        pul_v_r = dec_v_r + 1;
        pul_s_r = dec_s_r + 1;
	if(totalCh == 1)
	    pul_s_r = pul_v_r;
        for(;;)
	{
	    i = pul_v_r + pul_s_r + (dec_s_r + dec_v_r) * (rfChans-1);
	    n = i * pfy + lineGap * totalCh + gap * rfChans + 4;
	    cy = dpsH - statusY - n;
	    pulseHeight = cy / totalCh;
	    if (pulseHeight > pfy * 5)
		break;
	    if (pul_s_r > 0 || dec_s_r > 0)
	    {
		if (pul_s_r <= 1 && pulseHeight > pfy * 2)
		    break;
		if (dec_s_r > 0 && dec_s_r >= pul_s_r )
		{
		    dec_s_r--;
		    dec_v_r--;
		}
		else if (pul_s_r > 0)
		{
		    pul_v_r--;
		    pul_s_r--;
		}
		if (lineGap > pfy)
		    lineGap = pfy;
		else if (lineGap > 4)
		    lineGap -= 3;
	    }	
	    else if (lineGap > 4)
		lineGap -= 3;
	    else
	        break;
	}
	pulseMin = pulseHeight / 5;
	if (pulseHeight < pfy * 2)
	{
	    pul_v_r = pul_s_r = dec_s_r = dec_v_r = 0;
	    lineGap = 4;
	    n = lineGap * totalCh + gap * rfChans + 4;
	    pulseHeight = (dpsH - statusY - n) / totalCh;
	    if (pulseHeight < 4)
		pulseHeight = 4;
	    pulseMin = pulseHeight / 4;
	}
	if(pulseHeight > dpsH / 3)
	    pulseHeight = dpsH / 3;
	gradHeight = pulseHeight / 2;
        gradMin = gradHeight / 8;
	if (gradMin < 4)
	    gradMin = 4;
        if (gradHeight < gradMin)
            gradHeight = gradMin;

	statusY += dpsY;
	cy = statusY + lineGap;
	orgy[0] = dpsY2;
	for (n = 1; n <= GRADZ; n++)
	    orgy[n] = cy;
	if (rfchan[RCVRDEV]) {
	    orgy[RCVRDEV] = cy + gradHeight;
            cy = cy + lineGap + pulseHeight;
        }
	for (n = GRADZ; n >= GRADX; n--)
	{
            orgy[n] = cy + gradHeight;
	    if (rfchan[n] && visibleCh[n])
                cy = cy + lineGap + pulseHeight;
	}
	for (n = DO5DEV; n >= DODEV; n--)
	{
            orgy[n] = cy + dec_s_r * pfy + gap;
	    if (rfchan[n] && visibleCh[n])
                cy = orgy[n] + lineGap + pulseHeight + dec_v_r * pfy;
	}
	orgy[TODEV] = cy  + pul_s_r * pfy + gap;

        for (n=TODEV; n <= GRADZ; n++)
            chyc[n] = orgy[n];
	reset_text_pos();
}


static  void
set_xy_params()
{
	int	chan_name;

	if (dpsPlot)
	{
	     if (plotW < wcmax * 0.7 || plotH < wc2max * 0.7)
		charsize(0.7);
	     else
		charsize(1.0);
	}
        pfx = xcharpixels;
        pfy = ycharpixels;
	chascent = ycharpixels - char_ascent;
	if (WisSunColor() && !dpsPlot)
              colorwindow = 1;
        else
              colorwindow = 0;
	chascent = pfy * 0.8;
	chan_name = 1;
	if (dpsPlot)
	{
	     if (plotW < wcmax / 50.0)
		plotW = wcmax / 50.0;
	     if (plotW > wcmax)
		plotW = wcmax;
	     if (plotW + plotX > wcmax)
		plotW = wcmax - plotX;
	     dpsW = mnumxpnts * (plotW / wcmax);
	     dpsX = mnumxpnts * (plotX / wcmax);
	     dpsX2 = dpsX + dpsW;
	     if (!plotChanName)
                chan_name = 0;
#ifdef SUN
/*
	     chascent = raster_ascent;
	     chdescent = pfy - chascent;
*/
	     chdescent = pfy * 0.3;
#endif 
	}
	else 
        {
	     chdescent = 0;
#ifdef SUN
	     chascent = char_ascent;
#endif 
#ifdef SUN
             if (Wissun())
	     {
	        frameW = graf_width;
		if (xwin) {
		   pfx = dch_w;
		   pfy = dch_h;
	           chascent = dch_ascent;
	           chdescent = dch_descent;
		}
	     }
	     else
#endif 
	     {
#ifndef VNMRJ
                if (Wisgraphon())
	           frameW = 1000;
                else
#endif
	           frameW = mnumxpnts;
	        chdescent = pfy - chascent;
	     }
	     if (winW < wcmax / 30.0)
		winW = wcmax / 30.0;
	     if (winW > wcmax)
		winW = wcmax;
	     if (winW + winX > wcmax)
		winW = wcmax - winX;
             dpsW = frameW * (winW / wcmax); 
             dpsX = frameW * (winX / wcmax); 
             dpsX2 = dpsW + dpsX;
	     if (!winChanName)
	        chan_name = 0;
        }
        x_margin = dpsX + 5 * pfx * chan_name + 2;
	orgx_margin = x_margin;
	set_y_params();
}


static void
set_chan_params()
{
    	int     n, k;

	activeCh[TODEV] = 1;
	rfchan[0] = 0;
	k = 0;
	for(n = TODEV; n < RFCHAN_NUM; n++)
	{
	    if ((int)strlen(dnstr[n]) <= 0 || dnstr[n][0] == '0')
		activeCh[n] = 0;
	    rfchan[n] = activeCh[n];
	    if (activeCh[n])
		k++;
	}
	numch = 0;
	for (n = TODEV; n < RFCHAN_NUM; n++)
	     numch = numch + rfchan[n];
	set_xy_params();
}

static int
balance_mid_points(node)
SRC_NODE  *node;
{
   int  d1, d2, dx, p1, p2, p3;

   p1 = node->lNode->ix;
   p2 = node->mNode->ix;
   p3 = node->rNode->ix;
   d1 = dock_pnts[p2] - dock_pnts[p1];
   d2 = dock_pnts[p3] - dock_pnts[p2];
   if (d2 <= d1) {
	dx = d1 - d2;
	dock_pnts[p3] = dock_pnts[p3] + dx;
	return (dx);
   }
   dx = d2 - d1;
   for (d1 = p1+1; d1 <= p2; d1++) {
	if (dock_infos[d1] == 0)
	    break;
   }
   if (d1 > p2)
	d1 = p1 + 1;
   while (d1 <= p3) {
	dock_pnts[d1] += dx;
	d1++;
   }
   return (dx);
}


static int
measure_dock_xpnt(snode, lastNode, doFlag, firstTime, dMode)
SRC_NODE  *snode, *lastNode;
int       doFlag, firstTime, dMode;
{
   DOCK_NODE   *cnode;
   SRC_NODE    *dnode;
   COMMON_NODE *comnode;
   int        dev, off_flag, k;
   int        vinc, ix, isExpand;
   int        hasDelay, hasPulse, hasBox;
   int        obj_type;
   int        cx, cx2;
   int        dflag, verbos;
   double      rinc;
   double      time1;
   static int   p_base, d_base, a_base;
   static int   p_xbase, d_xbase;
   static double x_ratio = 1.0;

   isExpand = 0;
   verbos = 0;
   if (firstTime && debug > 1) {
        verbos = 1;
        fprintf(stderr, " measure dock x position,  flag= %d\n", doFlag);
        fprintf(stderr, "  dock num = %d\n", dock_len);
   }

   p_base = (double) pBase * xMult;
   d_base = (double) dBase * xMult;
   a_base = (double) acqBase * xMult;
   p_xbase = (double) pXBase * xMult;
   d_xbase = (double) dXBase * xMult;
   if (doFlag != 3) /* not expanding */
   {
        if (a_base > pfx * 8)
            a_base = pfx * 8;
        x_ratio = xRatio;
   }
   else
   {
	isExpand = 1;
        if (a_base > pfx * 12)
                a_base = pfx * 12;
   }
   if (a_base % 2 != 0)
	a_base++;
   if (doFlag == 0 || firstTime)
   {
	pulseCnt = 0.0;
	pulseXCnt = 0.0;
	delayCnt = 0.0;
	delayXCnt = 0.0;
	acqAct = 0;
	exPnts = 0;
	for (k = 0; k < dock_time_len; k++) {
	   dock_pnts[k] = 0;
	}
   }
   off_flag = 0;
   parallel = 0;
   hasDelay = 0;
   hasPulse = 0;
   hasBox = 0;
   cx2 = 0;
   time1 = 0;

   cnode = dock_start_node;
   if (cnode != NULL)
      time1 = cnode->time;
   while (cnode != NULL)
   {
	dnode = dock_list[cnode->id];
        dev = dnode->device;
	ix = cnode->ix;
	if (verbos) {
	   fprintf(stderr, " %d.%d.%d ", cnode->id, cnode->dock, ix);
	}
	obj_type = 0;
        dflag = 0;
	if (doFlag > 0) {
	    if (isExpand)
                cx = dock_pnts[cnode->ix] * xMult;
	    else
                cx = dock_pnts[cnode->ix];
	    if (cnode->dock == DOCKFRONT) {
                dnode->x1 = cx;
		if ((dnode->flag & XMARK) && (dev > 0))
                {
                   dnode->x1 = cx - spW / 2 - 1;
                   dnode->x2 = cx;
                   dnode->x3 = dnode->x1 + spW;
                   if (doFlag == 1)
                   {
                      if (sp_x[dev] <= dnode->x1 || dnode->y2 > sp_y[dev])
                      {
                        sp_x[dev] = cx;
                        dnode->y2 = dnode->y2 + spH + pfy / 2 + 2;
                        dnode->flag = dnode->flag | XNFUP;
                      }
                      else
                      {
                        dnode->y2 = sp_y[dev] + spH;
                        dnode->flag = dnode->flag & ~XNFUP;
                      }
                      sp_y[dev] = dnode->y2+2;
                   }
                }
	    }
            else if (cnode->dock == DOCKEND) {
                dnode->x2 = cx;
                dnode->x3 = cx;
            }
	    else {  /* DOCKMID */
                dnode->mx = cx;
            }
	    cx2 = cx;
	}
	else {
           cx = cx2;
	   if (cnode->time > time1 && cnode->dock == DOCKFRONT) {
	      vinc = 0;
	      if (hasBox) {
		    vinc = pfx / 2;
                    exPnts = exPnts + vinc;
	      }
	      else {
		    if (hasPulse) {
                        vinc = p_xbase;
                        pulseXCnt = pulseXCnt + 1.0;
		    }
		    else { /* hasDelay */
			vinc = d_xbase;
			delayXCnt = delayXCnt + 1.0;
		    }
	      }
	      cx += vinc;
	   }
	   if (dock_pnts[ix] < cx)
	      dock_pnts[ix] = cx;
	   else
	      cx = dock_pnts[ix];
	   if (cnode->dock == DOCKFRONT) {
	      dnode->x1 = cx;
           }
	}  /* doFlag == 0 */

        switch (cnode->type) {
        case DELAY:
        case VDELAY:
        case INCDLY:
        case VDLST:
	        if (cnode->dock == DOCKFRONT) {
		    hasDelay++;
		    break;
		}
		obj_type = DELAY;
                break;
        case ZGRAD:
        case SHGRAD:
        case SHVGRAD:
        case SHINCGRAD:
        case PESHGR:
        case DPESHGR:
        case OBLSHGR:
        case SH2DVGR:
        case PEOBLVG:
        case PEOBLG:
	        if (cnode->dock == DOCKFRONT) {
                    if (doFlag > 0 && dev > 0)
                        off_flag = RCHNODE;
		    hasDelay++;
		    break;
		}
		obj_type = DELAY;
                break;
        case PULSE:
        case SMPUL:
        case SPINLK:
	        if (cnode->dock == DOCKFRONT) {
		    hasPulse++;
                    if (doFlag > 0)
                        off_flag = RSTNODE | RCHNODE | RPGNODE;
		    break;
		}
		obj_type = PULSE;
		dflag = 1;
                break;
        case SHPUL:
        case SHVPUL:
        case SMSHP:
        case APSHPUL:
        case OFFSHP:
        case SHACQ:
	        if (cnode->dock == DOCKFRONT) {
		    hasPulse++;
                    if (doFlag > 0)
                        off_flag = RSTNODE | RCHNODE | RPGNODE;
		    break;
		}
		obj_type = SHPUL;
		dflag = 1;
                break;
        case FIDNODE:
	        if (cnode->dock == DOCKFRONT) {
		    dnode->x2 = cx + fidWidth;
                    exPnts += fidWidth;
		    break;
		}
                if (doFlag < 1)
                    dock_pnts[ix] = cx + fidWidth;
                break;
        case STATUS:
        case SETST:
        case DCPLON:
	        if (cnode->dock == DOCKFRONT) {
                    if (doFlag > 0)
                        off_flag = ASTNODE;
		    break;
		}
                break;
        case DCPLOFF:
	        if (cnode->dock == DOCKFRONT) {
                    if (doFlag > 0) {
                        off_flag = RSTNODE;
                        dnode->x2 = dnode->x1;
                        dnode->x3 = dnode->x1;
		    }
		    break;
		}
                break;
        case DEVON:
	        if (cnode->dock == DOCKFRONT) {
                    if (doFlag > 0 && dev > 0)
                    {
                       comnode = (COMMON_NODE *) &dnode->node.common_node;
                       if (comnode->val[0] > 0)
                           off_flag = RSTNODE | ACHNODE;
                       else
                           off_flag = RCHNODE | RPGNODE;
                    }
                }
                break;
        case RFONOFF:
	        if (cnode->dock == DOCKFRONT && doFlag > 0) {
                    dnode->x3 = cx + spW;
                }
                break;
        case GRAD:
        case INCGRAD:
        case OBLGRAD:
        case PEGRAD:
        case RGRAD:
        case MGPUL:
        case MGRAD:
        case VGRAD:
	        if (cnode->dock == DOCKFRONT && doFlag > 0) {
                    if (dev > 0)
                        off_flag = ACHNODE;
                }
                break;
        case PRGON:
        case PRGOFF:
	        if (cnode->dock == DOCKFRONT && doFlag > 0 && dev > 0) {
                    if (prg_node[dev] != NULL)
                    {
                        prg_node[dev]->x2 = cx;
                        prg_node[dev]->x3 = cx;
                    }
                    if (cnode->type == PRGON)
                        prg_node[dev] = dnode;
                    else
                        prg_node[dev] = NULL;
                }
                break;
        case ACQUIRE:
        case JFID:
        case XTUNE:  /* set4Tune    */
        case XMACQ:  /* XmtNAcquire */
        case SPACQ:  /* SweepNAcquire */
	        if (cnode->dock == DOCKFRONT) {
                    if (doFlag > 0)
                        off_flag = RSTNODE | RCHNODE | RPGNODE;
		    break;
		}
		obj_type = ACQUIRE;
		dflag = 1;
                break;
        case HLOOP:
        case GLOOP:
        case SLOOP:
        case ENDHP:
        case ENDSP:
        case ENDSISLP:
        case MSLOOP:
        case PELOOP:
        case PELOOP2:
	        if (cnode->dock == DOCKFRONT) {
                    exPnts += loopW;
                    if (doFlag > 0)
			dnode->flag = dnode->flag | XLOOP;
		    break;
		}
	        if (cnode->dock == DOCKEND) {
                    if (doFlag < 1) {
                        dock_pnts[ix] = cx + loopW;
		    }
		}
                break;
        case XGATE:
        case ROTORP:
        case ROTORS:
        case BGSHIM:
        case SHSELECT:
        case GRADANG:
        case ROTATEANGLE:
	        if (cnode->dock == DOCKFRONT) {
                    if (doFlag < 1)
		        hasBox++;
		    break;
		}
                if (doFlag < 1) {
	            if (cnode->dock == DOCKMID)
		        vinc = pfx / 2 + 1;
		    else
		        vinc = pfx / 2 + 1;
		    k = dnode->x1 + vinc;
		    if (k > dock_pnts[ix])
		        dock_pnts[ix] = k;
		    else {
		        if (cnode->time > time1)
		    	    dock_pnts[ix] += vinc;
		    }
	            if (cnode->dock == DOCKEND) {
		        hasBox--;
		        k = cx - dnode->x1;
		        if (k < pfx+2)
			    dock_pnts[ix] = dnode->x1 + pfx + 2;
		    }
		}
		dflag = 1;
                break;
        default:
                break;
        }
        if (doFlag > 0)
		obj_type = 0;
	if (obj_type == DELAY) {
	    rinc = dnode->rtime / dMax;
            if (rinc > 1.0)
                rinc = 1.0;
            rinc = rinc * x_ratio + 1.0;
            vinc = rinc * d_base;
            if (cnode->dock == DOCKMID)
                vinc = vinc / 2;
            k = dnode->x1 + vinc;
            if (k > dock_pnts[ix]) {
                dock_pnts[ix] = k;
            }
            else {  /* something else happened */
                if (cnode->time > time1) {
                     if (hasPulse) {
                          pulseXCnt += 1.0;
                          dock_pnts[ix] += p_xbase;
                     }
                     else {
                          delayXCnt += 1.0;
                          dock_pnts[ix] += d_xbase;
                     }
                }
            }
            if (cnode->dock == DOCKEND) {
                hasDelay--;
                if (dnode->stime == time1) {
                      /* nothing else within this delay */
                      if (k == dock_pnts[ix])
                          delayCnt = delayCnt + rinc;
                }
            }
	}
	if (obj_type == PULSE || obj_type == SHPUL) {
	    rinc = dnode->rtime / pMax;
            if (rinc > 1.0)
                rinc = 1.0;
            rinc = rinc * x_ratio + 1.0;
	    if (obj_type == SHPUL)
               vinc = rinc * p_base * 2;
	    else
               vinc = rinc * p_base;
            if (cnode->dock == DOCKMID)
                vinc = vinc / 2;
            k = dnode->x1 + vinc;
            if (k > dock_pnts[ix]) {
                dock_pnts[ix] = k;
            }
            else {
                if (cnode->time > time1) {
                    pulseXCnt += 1.0;
                    dock_pnts[ix] += p_xbase;
                 }
            }
            if (cnode->dock == DOCKEND) {
                 hasPulse--;
		 if (hasPulse == 0) {
		    if (obj_type == SHPUL)
                             pulseCnt = pulseCnt + rinc * 2.0;
		    else
                             pulseCnt = pulseCnt + rinc;
		 }
/*
                 if (dnode->stime == time1) {
                      if (k == dock_pnts[ix]) {
			  if (obj_type == SHPUL)
                             pulseCnt = pulseCnt + rinc * 2.0;
			  else
                             pulseCnt = pulseCnt + rinc;
		      }
                 }
*/
            }
	}
	if (obj_type == ACQUIRE) {
            if (cnode->dock == DOCKMID)
                vinc = a_base / 2;
	    else
                vinc = a_base;
            k = dnode->x1 + vinc;
            if (k > dock_pnts[ix]) {
                dock_pnts[ix] = k;
            }
            else {
                if (cnode->time > time1) {
                    acqAct++;
                    dock_pnts[ix] += a_base / 2;
                }
            }
            if (cnode->dock == DOCKEND) {
                if (dnode->stime == time1) {
                    if (k == dock_pnts[ix])
                       acqAct++;
                }
            }
	}
	if (doFlag < 1) {
            if (cnode->dock == DOCKEND) {
                if (dnode->mDocked) {
                     k = balance_mid_points(dnode);
                     exPnts += k;
                }
	        dnode->x2 = dock_pnts[ix];
	    }
            cx2 = dock_pnts[ix];
	}
	if (firstTime && dflag) {
	    if (cnode->dock == DOCKEND) {
		k = dnode->lNode->ix + 1;
		while (k <= ix) {
		   dock_infos[k] = 1;
		   k++;
		}
	    }
	}

        if (off_flag > 0)
        {
            while (dnode != NULL)
            {
                dev = dnode->device;
                if (off_flag & RSTNODE || off_flag & ASTNODE)
                {
                   if (status_node[dev] != NULL)
                   {
                     status_node[dev]->x2 = cx;
                     status_node[dev]->x3 = cx;
                     status_node[dev] = NULL;
                   }
                   if (off_flag & ASTNODE)
                     status_node[dev] = dnode;
                }
                if (off_flag & RCHNODE || off_flag & ACHNODE)
                {
                   if (chOn_node[dev] != NULL)
                   {
                     chOn_node[dev]->x2 = cx;
                     chOn_node[dev]->x3 = cx;
                     chOn_node[dev] = NULL;
                   }
                   if (off_flag & ACHNODE)
                     chOn_node[dev] = dnode;
                }
                if (off_flag & RPGNODE)
                {
                   if (prg_node[dev] != NULL)
                   {
                        prg_node[dev]->x2 = cx;
                        prg_node[dev]->x3 = cx;
                        prg_node[dev] = NULL;
                   }
                }
                dnode = dnode->bnode;
            }
            off_flag = 0;
        }
	time1 = cnode->time;
        chx[dev] = cx2;
	if (cnode->dock <= 0)
	    break;
        cnode = cnode->dnext;
   } /* while loop */

   if (doFlag > 0)
   {
        for (dev = 0; dev < TOTALCH; dev++)
        {
            if (status_node[dev] != NULL)
            {
                status_node[dev]->x2 = cx2;
                status_node[dev]->x3 = cx2;
            }
            if (chOn_node[dev] != NULL)
            {
                chOn_node[dev]->x2 = cx2;
                chOn_node[dev]->x3 = cx2;
            }
            if (prg_node[dev] != NULL)
            {
                prg_node[dev]->x2 = cx2;
                prg_node[dev]->x3 = cx2;
            }
        }
   }
   if (verbos)
       fprintf(stderr, "\n measure dock x position done, width %d \n", cx2);

   dpsWidth = cx2;
   return(cx2);
}

static void
calculate_x_ratio(start_node, end_node, verbos)
 SRC_NODE   *start_node, *end_node;
 int  verbos;
{
	int      s_width, loop_f;
	int	 diff;
	double    ff;

	loopW = 3;
	loopH = 5;
	if (!dpsPlot)
	{
	    if (dpsW < 800)
	    	spW = 8;
	    else if (dpsW < 1000)
	    	spW = 10;
	    else
	    	spW = 12;
	    if (dpsH < 600)
	        spH = 8;
	    else if (dpsH < 800)
	        spH = 10;
	    else
	        spH = 12;
	}
	else
	{
	    spW = pfx * 1.5;
	    spH = pfx;
	    if (pfx > 18) {
		loopW = 5;
		loopH = 10;
	    }
	}

	fidWidth = dpsW / 10;
	if (fidWidth > 8 * pfx)
	     fidWidth = 8 * pfx;
	if (fidWidth < 3 * pfx)
	     fidWidth = 3 * pfx;
	acqBase = dpsW / 18;
	if (acqBase < pfx)
	     acqBase = pfx;
	else if (acqBase > 6 * pfx)
	     acqBase = pfx * 6;
	xRatio = 1.0;
	pBase = pfx;
	dBase = 4 * pBase;
	pXBase = pBase;
	dXBase = dBase / 2;
	if (dMax <= 0.0)
	    dMax = 1.0;
	if (pMax <= 0.0)
	    pMax = 1.0;
	measure_xpnt(start_node, end_node, 2, 1, verbos);
	s_width = dpsX2 - x_margin - 2 * pfx;
	if (debug > 1) {
	    fprintf(stderr," window wh %d %d  \n", s_width, dpsH); 
	    fprintf(stderr,"  dps width %d  extra %d \n", dpsWidth, exPnts); 
	    fprintf(stderr, " pulses %g %g  delays %g %g \n", pulseCnt, pulseXCnt, delayCnt, delayXCnt);
	    fprintf(stderr, " acquires %d \n", acqAct);
	}
	diff = s_width - exPnts;
	ff = delayCnt * 8 + delayXCnt * 4 + pulseCnt * 2 + pulseXCnt;
	if (diff > 0)
	{
	    if (ff < 16)
		ff = 16;
	    pXBase = diff / ((int) ff);
	    if (pXBase < 2)
		pXBase = 2;
            if (pXBase > (pfx * 2)) {
	        ff = ff + delayCnt * 4 + delayXCnt * 4;
                if (delayCnt < 1.0)
                    ff = ff + delayXCnt * 4;
	        pXBase = diff / ((int) ff);
	        dXBase = pXBase * 8;
	        dBase = pXBase * 12;
                if (delayCnt < 1.0)
	            dXBase = pXBase * 12;
            }
            else {
	        dXBase = pXBase * 4;
	        dBase = pXBase * 8;
            }
	    pBase = pXBase * 2;
	}
        else {  // many acquires in sequence
	    if (ff < 10)
		ff = 10;
            if (acqAct > 0)
	        ff = ff + acqAct * 3;
            else
	        ff = ff + 8;
	    pXBase = s_width / ((int) ff);
	    if (pXBase < 2)
		pXBase = 2;
	    pBase = pXBase * 2;
	    dBase = pXBase * 8;
	    dXBase = pXBase * 4;
	    acqBase = pXBase * 3;
        }

	loop_f = 0;
	while(loop_f < 7)
	{
	     loop_f++;
	     measure_xpnt(start_node, end_node, 0, 0, 0);
	     if (debug > 1)
	        fprintf(stderr," dps %d. width %d  extra %d win %d\n",loop_f,dpsWidth,exPnts,s_width); 
	     if (dpsWidth <= s_width) {
	        diff = s_width - dpsWidth;
	        if (diff < pfx * 2)
		{
	           if (debug > 3)
	        	fprintf(stderr,"  break loop  %d\n", loop_f); 
		    break;
		}
	     }
	     else
	        diff = dpsWidth - s_width;
	     if (dpsWidth > s_width)
	     {
                if ((loop_f == 1) && (diff > s_width)) {
                    pBase = pBase / 2;
                    if (pBase < 2)
                        pBase = 2;
                    dBase = dBase / 2;
                    if (dBase < pBase)
                        dBase = pBase;
                    pXBase = pXBase / 2;
                    dXBase = dXBase / 2;
                    acqBase = acqBase / 2;
                    if (acqBase < pBase)
                        acqBase = pBase;
                    fidWidth = fidWidth / 2;
                    if (fidWidth < pfx * 2)
                        fidWidth = pfx * 2;
		    continue;
                }
		if (pXBase > 2)
		{
		    if (pXBase > pfx) {
		       diff = diff - pulseXCnt * 2;
		       pXBase-= 2;
		    }
		    else {
		       diff = diff - pulseXCnt;
		       pXBase--;
		    }
		}
		if (diff > 0 && dXBase > 2)
		{
		    if (dXBase > pfx) {
		       diff = diff - delayXCnt * 2;
		       dXBase-= 2;
		    }
		    else {
		       diff = diff - delayXCnt;
		       dXBase--;
		    }
		}
		if (diff > 0 && pBase > 4)
		{
		    if (pBase > pfx * 2) {
		       diff = diff - pulseCnt * 2;
		       pBase-= 2;
		    }
		    else {
		       diff = diff - pulseCnt;
		       pBase--;
		    }
		}
		if (diff > 0 && dBase > pBase)
		{
		    diff = diff - delayCnt * 2;
		    dBase -= 2;
		}
		if (diff > 0)
		{
   		     if (acqAct > 0) {
			if (acqAct > 6) {
		            if (acqBase >= pfx) {
			        acqBase -= 2;
		                diff = diff - acqAct * 2;
			    }
		            else if (acqBase > 4) {
			        acqBase--;
		                diff = diff - acqAct;
			    }
			}
		        else if (acqBase > pfx * 2) {
			    acqBase -= 2;
		            diff = diff - acqAct * 2;
			}
		     }
		     else if (fidWidth > pfx * 5)
		     {
			fidWidth -= pfx / 2;
		        diff -= pfx / 2;
		     }
		}
		if (diff > pfx)
		{
		     if (xRatio >= 0.2)
			xRatio = xRatio - 0.2;
		}
		continue;
	     }
	     else  /* dpsWidth < s_width */
	     {
		if (diff > pulseCnt)
		{
		    pBase++;
		    diff = diff - pulseCnt;
		}
		if (diff > pulseXCnt && pXBase < pBase) {
		    pXBase++;
		    diff = diff - pulseXCnt;
		}
		if (diff > delayCnt)
		{
		    if (dBase < 4 * pBase)
		    {
                        if (diff > (2 * delayCnt)) {
			    dBase+= 2;
			    diff = diff - 2 * delayCnt;
                        }
                        else {
			    dBase++;
			    diff = diff - delayCnt;
                        }
		    }
		}
		if (diff > delayXCnt)
		{
		    if (dXBase < pBase)
		    {
			dXBase++;
			diff = diff - delayXCnt;
		    }
		}
		if  (acqAct > 0 && diff > acqAct)
		{
		    if (acqBase < pfx * 5)
		    {
                        if (acqAct > 6) {
			    acqBase += 1;
			    diff = diff - acqAct;
                        }
                        else { 
			    acqBase += pfx / 4;
			    diff = diff -  pfx * acqAct / 4;
                        }
		    }
		}
		else
		{   
                    if (fidWidth < pfx * 8)
		    {
		    	fidWidth += pfx / 2;
		    	diff -= pfx / 2;
		    }
		}
		if (diff > pfx)
		{
		    if (xRatio <= 0.8)
			xRatio += 0.2;
		}
	     }
	}
	measure_xpnt(start_node, end_node, 1, 0, 0);
	if (debug > 1)
	     fprintf(stderr,"  dps width %d  extra %d \n", dpsWidth, exPnts); 
}

static void
dump_dock_time()
{
     int  i, k;
     SRC_NODE  *node, **xnodes;

     for (k = 0; k < dock_len; k++) {
	node = dock_list[k];
        if (node != NULL) {
	    fprintf(stderr, " item %d time %g %g %g,  ", node->id, node->stime, node->mtime, node->etime);
	    if (node->dockList != NULL) {
	        xnodes = node->dockList;
	        fprintf(stderr, "  docked number %d, docked by ", node->dockNum);
		for (i = 0; i < node->dockNum; i++) {
		   if (xnodes[i] != NULL)
		      fprintf(stderr, " %d ", xnodes[i]->id);
		}
	    }
	    fprintf(stderr, " \n");
	}
     }
}

static void
dump_dock_link()
{
     DOCK_NODE  *dk;

     dk = dock_start_node;
     fprintf(stderr, "** dock list ** \n");
     while (dk != NULL) {
	 fprintf(stderr, " item %d time %g dock %d  ix %d \n", dk->id, dk->time, dk->dock, dk->ix);
	dk = dk->dnext;
     }
}


static void
cal_dock_time(node)
SRC_NODE  *node;
{
     double  x;
     SRC_NODE  *snode;

     snode = dock_list[node->dockAt];
     switch (node->dockTo) {
	case DOCKFRONT:
		x = snode->stime;
	        break;
	case DOCKMID:
		x = snode->mtime;
		snode->mDocked = 1;
	        break;
	case DOCKEND:
		x = snode->etime;
	        break;
	default: /* wrong */
		x = snode->etime;
	        break;
     }
     switch (node->dockFrom) {
	case DOCKFRONT:
		node->stime = x;
		node->etime = x + node->rtime2;
	        break;
	case DOCKMID:
		node->stime = x - node->rtime2 / 2;
		node->etime = node->stime + node->rtime2;
		node->mDocked = 1;
	        break;
	case DOCKEND:
		node->etime = x;
		node->stime = x - node->rtime2;
	        break;
	default: /* this is wrong */
		node->stime = x;
		node->etime = x + node->rtime2;
	        break;
     }
     if (node->dockFrom == DOCKMID)
        node->mtime = x;
     else
        node->mtime = node->stime + node->rtime2 / 2;
}


static void
xset_dock_time(node)
SRC_NODE  *node;
{
     int m;
     SRC_NODE  **xnodes;

     cal_dock_time(node);
     if (node->dockList != NULL) {
         xnodes = node->dockList;
         for (m = 0; m < node->dockNum; m++) {
	      if (xnodes[m] != NULL)
	         xset_dock_time(xnodes[m]);
	 }
     }
}


static int
set_dock_link()
{
     int    k, m, i;
     SRC_NODE  *node, *nd, **xnodes;

     lastId = 0;
     if (dock_list == NULL)
         return(0);
     for (k = 0; k < dock_len; k++) {
        m = dock_N_array[k];
        if (m > 0) {
            node = dock_list[k];
            if (node == NULL) {
        	if((node = new_dpsnode()) == NULL)
                    return(0);
                node->id = k;
                node->flag = 0;
                node->fname = "empty";
                node->dockAt = k;
                node->dockTo = DOCKFRONT;
                node->dockFrom = DOCKFRONT;
                dock_list[k] = node;
            }
            sizeT = (size_t) m * sizeof(SRC_NODE *);
            if ((xnodes = (SRC_NODE **) allocateNode(sizeT)) == 0)
                return (0);
            node->dockList = xnodes;
            node->dockNum = m;
	}
     }
     for (k = 0; k < dock_len; k++) {
        node = dock_list[k];
        if (node == NULL) {
            continue;
        }
        if (node->dockAt == NODOCK) {
            Werrprintf("dps error: dps object %d has no dock point.\n", node->id);
            continue;
        }
        lastId = k;
        if (node->dockAt != node->id)
        {
            nd = dock_list[node->dockAt];
            if (nd == NULL) {
               Werrprintf("dps error: dps object %d was missing.\n", node->dockAt);
               return (0);
            }
            if (nd->dockList != NULL) {
                m = nd->dockNum;
                xnodes = nd->dockList;
                for (i = 0; i < m; i++) {
                    if (xnodes[i] == NULL) {
                        xnodes[i] = node;
                        break;
                    }
                }
            }
        }
     }
     if (debug > 1)
        fprintf(stderr, " last dps object %d  \n", lastId);
     return (1);
}

static int
set_dock_time()
{
     int    k, first_id;
     double  time0;
     SRC_NODE  *node, *node1, **xnodes;

     first_id = 999;
     for (k = 0; k < dock_len; k++) {
	node = dock_list[k];
	if (node != NULL) {
	    if (node->dockAt == node->id) {
	        first_id = node->id;
		break;
	    }
	}
     }
     if (first_id == 999) {
	Werrprintf("dps error: dps could not find the origin dock point. \n"); 
	return (0);
     }
     node1 = dock_list[first_id];
     if (node1->dockList == NULL) {
	Werrprintf("dps error: the dps link was broken. \n"); 
	return (0);
     }
     if (debug > 1) {
	fprintf(stderr, " dps first node is %d \n", first_id);
     }
     node1->stime = 0;
     node1->etime = node1->rtime2;
     xnodes = node1->dockList;
     for (k = 0; k < node1->dockNum; k++) {
	if (xnodes[k] != NULL)
	    xset_dock_time(xnodes[k]);
     }
     time0 = 0;
     for (k = 0; k < dock_len; k++) {
	if (dock_list[k] != NULL) {
	    if (dock_list[k]->stime < time0)
		time0 = dock_list[k]->stime;
	}
     }
     if (time0 != 0) {
	time0 = 0 - time0;
        for (k = 0; k < dock_len; k++) {
	    node = dock_list[k];
	    if (node != NULL) {
	   	node->stime = node->stime + time0;
	   	node->mtime = node->mtime + time0;
	   	node->etime = node->etime + time0;
	    }
	}
     }
     if (debug > 3)
	dump_dock_time();
     return (1);
}

static int
sort_dock_node()
{
     int i, k, pindex, num_dock;
     SRC_NODE  *s, *s1, *d, *p, *nd;
     DOCK_NODE  *dk, *pdk;

     if (disp_start_node == NULL)
	return (0);
     if (set_dock_time() < 1)
	return (0);
     for (i = 0; i < dock_len; i++) {
	if (dock_list[i] != NULL) {
	   dock_list[i]->active = 0;
	}
     }
     d = disp_start_node;
     s = d;
     while (d != NULL) {
	d->active = 1;
	if (d->stime < s->stime)
	    s = d;
	d = d->dnext;
     }
     dock_list[s->id] = NULL;
     d = s;
     s1 = s;
     s->dnext = NULL;
     s->pnode = NULL;
     for (i = 0; i < dock_len; i++) {
	if (dock_list[i] == NULL || dock_list[i]->active == 0)
	    continue;
	nd = dock_list[i];
	nd->dnext = NULL;
        if (nd->stime >= d->stime) {
	     d->dnext = nd;
	     nd->pnode = d;
	     d = nd;
        }
        else {
	    p = d;
	    k = 0;
	    while (p != NULL) {
		if (nd->stime >= p->stime)
		   break;
		p = p->pnode;
	    } 
	    if (p == NULL) {
		nd->dnext = s;
		s->pnode = nd;
		nd->pnode = NULL;
		s = nd;
	    }
	    else {
		nd->pnode = p;
		nd->dnext = p->dnext;
		if (p->dnext != NULL) {
		    p->dnext->pnode = nd;
		}
		p->dnext = nd;
	    }
	    while (nd->dnext != NULL)
		nd = nd->dnext;
	    d = nd;
        }
     }
     while (1) {
	if (d->dnext == NULL)
	    break;
        d = d->dnext;
     }
     fid_node->stime = d->etime;
     fid_node->etime = d->etime + 0.1;
     if (fid_node->active) {
         d->dnext = fid_node;
         dock_list[dock_len] = fid_node;
         fid_node->id = dock_len;
     }
     dock_list[s1->id] = s1;
     disp_start_node = s;
     dk = NULL;
     num_dock = 1;
     while (s != NULL) {
        if ((dk = new_docknode(dk)) == NULL)
	   break;
        num_dock++;
	dk->type = s->type;
	dk->id = s->id;
	dk->dock = DOCKFRONT;
	dk->time = s->stime;
	s->lNode = dk;
	if (s->mDocked) {
            dk = new_docknode(dk);
	    dk->type = s->type;
	    dk->id = s->id;
	    dk->dock = DOCKMID;
	    dk->time = s->mtime;
	    s->mNode = dk;
            num_dock++;
	}
	if (s->rtime2 > 0.0) {
            dk = new_docknode(dk);
	    dk->type = s->type;
	    dk->id = s->id;
	    dk->dock = DOCKEND;
	    dk->time = s->etime;
	    s->rNode = dk;
            num_dock++;
	}
	s = s->dnext;
     }
     dk = new_docknode(dk);
     num_dock++;
     dk->type = 0;
     dk->dock = 0;
     dk->id = 0;
     dk->dnext = NULL;
     pdk = dock_src_node;
     pdk->prev = NULL;
     pdk->dnext = NULL;
     dock_start_node = pdk;
     dk = pdk->next;
     if (dock_list[pdk->id]->rtime2 > 0)
	pindex = pdk->id;
     else
	pindex = 0;
     /* sort dock list */
     while (dk != NULL) {
        dk->dnext = NULL;
	if (dk->dock == 0) /* last one */
	{
	    break;
	}
	while (pdk != NULL) {
	   if (pdk->time < dk->time)
		break;
	   if (pdk->time == dk->time) {
	      if (dk->dock == DOCKFRONT)
		  break;
	      else {
	          if (pindex != dk->id)
		     break;
	      }
	   }
	   pdk = pdk->prev;
	}
	if (pdk == NULL) {
	   dk->dnext = dock_start_node;
	   dk->prev = NULL;
	   dock_start_node->prev = dk;
	   dock_start_node = dk;
	}
	else {
	   dk->prev = pdk;
	   dk->dnext = pdk->dnext;
	   if (pdk->dnext != NULL) {
		pdk->dnext->prev = dk;
	   }
	   pdk->dnext = dk;
	}
	pdk = dk;
	while (pdk->dnext != NULL)
	   pdk = pdk->dnext;
        dk = dk->next;
        if (dk->dock == DOCKFRONT) {
           s = dock_list[dk->id];
	   if (s->rtime2 > 0) 
	       pindex = dk->id;
	}
     }

     k = num_dock + 1;
     if (dock_time_len < k || dock_times == NULL) {
	 dock_time_len = k;
         sizeT = (size_t) k;
         dock_times = (double *) allocateNode (sizeof(double) * sizeT);
         dock_pnts = (int *) allocateNode (sizeof(int) * sizeT);
         dock_infos = (int *) allocateNode (sizeof(int) * sizeT);
     }
     for (i = 0; i < k; i++) {
         dock_times[i] = 0;
         dock_infos[i] = 0;
     }
     k = 0;
     dk = dock_start_node;
     while (dk != NULL) {
	if (dock_times[k] != dk->time)
		k++;
	dock_times[k] = dk->time;
	dk->ix = k;
	dk = dk->dnext;
     }
     dock_size = k;

     if (debug > 2)
	dump_dock_link();
     return(1);
}


static int
create_dps_info(origin)
int	origin;
{
	int k;

	clean_src_data();
	hilitNode = NULL;

/*
	if (tbug)
            strcpy(dpsfile, "/tmp/testdata");
	else
*/
	if (!tbug)
            sprintf(dpsfile, "%s/%s", curexpdir, DPS_DATA);
        if ( !check_psg_file(dpsfile))
	     return(0);
	dps_ver = 0;
	hasDock = 0;
	hasIf = 0;
	dockStart = 999;
	nodeId = NODOCK;
	if (origin) {
            init_rt_index();
	}
	init_rt_values();
	build_dps_node(origin);
	if (fin != NULL)
	    fclose(fin);
	fin = NULL;
        if (!debug && !jout) {
	     remove_dps_file(DPS_DATA);
	     remove_dps_file(DPS_TABLE);
	}
	if (src_start_node == NULL)
	     return(0);
/**
	if (dps_ver != GO_PSG_REV)
	{
	     Werrprintf("dps error: wrong version of psg, please use 'seqgen' to compile pulse_sequence.");
	     printf("dps version: %d  seqfil version: %d \n", GO_PSG_REV, dps_ver);
	     return(0);
	}
**/

       // if(origin)
	     set_chan_params();
	clear_cursor();

  	if (hasDock) {
     	     if (dock_list == NULL) {
	         Werrprintf("dps error: psg is empty.");
	         return(0);
	     }
     	     if (debug > 1)
		 fprintf(stderr, " dps set_dock_link ...\n");
	     k = set_dock_link();
	     if (k <= 0)
	         return(0);
	}

	transient = 1;
	ap_op_ok = 1;
        execute_ps(transient);
	if (disp_start_node == NULL)
	{
	     Werrprintf("dps error: psg data is empty when ct is 0.");
	     return(0);
	}
/*
	draw_start_node = disp_start_node;
	draw_end_node = NULL;
*/
	calculate_x_ratio(disp_start_node, NULL, debug);
/*
	if (origin)
	    calculate_x_ratio(disp_start_node, NULL, debug);
	else
	    measure_xpnt(disp_start_node, NULL, 1, 0, 0);
*/
	if (debug > 1)
	    fprintf(stderr, " %d pulses  %d delays\n", (int) pulseCnt, (int)delayCnt);
	if (pulseCnt <= 0.0 && delayCnt <= 0.0)
	{
	    if (acqAct <= 0)
	        Werrprintf("dps error: psg data is empty");
	}
	if (origin && jout) {
            sprintf(dpsfile, "%s/%s", curexpdir, DPS_TABLE);
	    jfout = fopen(dpsfile, "w");
	    if (jfout != NULL) {
		init_rt_values();
		for (k = 0; k < sys_nt; k++) {
		    fprintf(jfout, "ct= %d\n", k+1);
	    	    simulate_psg(k, 1);
		}
	    }
	}
        if (jfout != NULL) {
	    fclose(jfout);
	    jfout = NULL;
	}
	
	return(1);
}




static void
parse_args(source)
 char	*source;
{
	int   len, quot;
	char  *data;

	argIndex = 0;
	len = 0;
	quot = 0;
	data = source;
	while(*data == ' ' || *data == '\t')
	     data++;
	argArray[0] = data;
	while (1)
	{
	     switch (*data) {
		case '\t':
		case ' ':
		   if (!quot)
		   {
		       if (len > 0)
		       {
			   *data = '\0';
			   data++;
			   while (*data != '\n' && *data != '\0')
			   {
			       if (*data == ' ')
				  data++;
			       else
				  break;
			   }
			   len = 0;
			   argIndex++;
			   argArray[argIndex] = data;
			   if (argIndex >= 64)
				return;
			}
		    }
		    else
		    {
			data++;
			len++;
		    }
		    break;
		case '"':
		    if (!quot)
			quot = 1;
		    else
			quot = 0;
		    data++;
		    len++;
		    break;
		case '\0':
		case '\n':
		    if (len > 0)
		    {
			*data = '\0';
			argIndex++;
		    }
		    return;
		    break;
		default:
		    data++;
		    len++;
		    break;
		}
	}
}

static  double
get_exp_time()
{
        int     type;
        double   retval;

        retval = 0.0;
	if (tbug)
             return(retval);
        sprintf(dpsfile, "%s/%s", curexpdir, DPS_TIME);
        if ( !check_psg_file(dpsfile))
             return(retval);
        while (fgets(inputs, 510, fin) != NULL)
        {
                parse_args(inputs);
                if (argIndex < 2)
                    continue;
                type = atoi(argArray[0]);
                if (type == EXPTIME)
                {
                    retval = get_float(1);
                    break;
                }
        }
	if (fin != NULL)
	    fclose(fin);
	fin = NULL;
        if (!debug)
	     remove_dps_file(DPS_TIME);
        return(retval);
}


static int
link_pulse_node(s_node)
SRC_NODE  *s_node;
{
	int		num_dev, items, m;
	int		k, vnum, fnum, shaped;
	double		max, t;
	SRC_NODE	*newnode, *curnode;
	COMMON_NODE	*cnode, *pnode;

	curnode = s_node;
	pnode = (COMMON_NODE *) &s_node->node.common_node;
	k = 2;
	num_dev = get_intg(k++);
	shaped = get_intg(k++);
	vnum = get_intg(k++);
	fnum = get_intg(k++);
	if (fnum > 6)
	     fnum = 6;
	pnode->flabel[0] = (char *) get_label(k++); // rof1
	pnode->fval[0] = (double) get_float(k++);
	pnode->flabel[1] = (char *) get_label(k++); // rof2
	pnode->fval[1] = (double) get_float(k++);
	s_node->rg1 = pnode->fval[0];
	s_node->rg2 = pnode->fval[1];
	if (hasDock)
	    num_dev = 1;
	
	cnode = pnode;
	max = 0.0;
	while (num_dev > 0)
	{
	    curnode->device = get_intg(k++);
            if (curnode->device < 0)
                curnode->device = 0;
	    if (shaped) {
	        cnode->pattern = (char *) get_label(k++);
                if (get_rf_shape(curnode, cnode->pattern) <= 0)
                     return(0);
            }
	    items = 0;
	    while(items < vnum)
	    {
		cnode->vlabel[items] = get_label(k++);
		cnode->val[items] = get_intg(k++);
		items++;
	    }
	    items = 0;
	    m = 2;
	    while(items < fnum)
	    {
		cnode->flabel[m] = get_label(k++);
		cnode->fval[m] = get_float(k++);
		items++;
		m++;
	    }
	    activeCh[curnode->device] = 1;
/**
	    status_node[curnode->device] = NULL;
	    chMod[curnode->device] = 0;
**/
            t = cnode->fval[2];
            if (curnode->type == SHACQ) {
                t = t + cnode->fval[3];
            }
            else {
                if (cnode->flabel[0][0] != '-')
                    t = t + cnode->fval[0];
                if (newAcq != 2)
                    t = t + cnode->fval[1];
            }
	    // if (cnode->fval[2] > max)
	    if (t > max)
		max = t;
	    curnode->ptime = cnode->fval[2];
	    if (curnode->ptime > 0.0)
		curnode->rtime2 = curnode->ptime;
	    else
		curnode->rtime2 = minTime;
	    num_dev--;
	    if (num_dev > 0)
	    {
        	if((newnode = new_dpsnode()) == NULL)
		    return(0);
		newnode->type = s_node->type;
		newnode->fname = s_node->fname;
		curnode->bnode = newnode;
		curnode->line = jline;
		curnode = newnode;
		cnode = (COMMON_NODE *) &newnode->node.common_node;
		cnode->flabel[0] = pnode->flabel[0];
		cnode->flabel[1] = pnode->flabel[1];
		cnode->fval[0] = pnode->fval[0];
		cnode->fval[1] = pnode->fval[1];
		curnode->rg1 = s_node->rg1;
		curnode->rg2 = s_node->rg2;
	    }
	}
	curnode = s_node;
	while (curnode != NULL)
	{
	    curnode->rtime = max;
            if (max > 0.0)
	        curnode->rtime2 = max;
	    curnode = curnode->bnode;
	}
	return(1);
}


static void
rt_op_node(snode) 
SRC_NODE  *snode;
{
	int	 k, inum, fnum, m;
	RT_NODE  *node;

	node = (RT_NODE *) &snode->node.rt_node;
	node->op = (int) get_intg(2);
	k = 3;
	inum = get_intg(k++);
	fnum = get_intg(k++);
	m = 0;
	while (inum > 0)
	{
	      node->vlabel[m] = get_label(k++);
	      node->val[m] = get_intg(k++);
	      inum--;
	      m++;
	}
	m = 0;
	while (fnum > 0)
	{
	      node->flabel[m] = get_label(k++);
	      node->fval[m] = get_float(k++);
	      fnum--;
	      m++;
	}
}

static void
link_ex_node(snode) 
SRC_NODE  *snode;
{
	int	  k, xnum, inum, fnum, m;
    	EX_NODE   *node;

	node = (EX_NODE *) &snode->node.ex_node;
	k = 3;
	xnum = get_intg(k++);
	inum = get_intg(k++);
	fnum = get_intg(k++);
        if (inum > VNUM) // something wrong
            inum = VNUM; 
        if (fnum > VNUM)
            fnum = VNUM; 
	node->vnum = inum;
	node->fnum = fnum;
	node->exnum = xnum;
	if (xnum > 0) {
              sizeT = (size_t) xnum * sizeof(char *);
	      node->exlabel = (char **) allocateNode(sizeT);
	      node->exval = (char **) allocateNode(sizeT);
	      m = 0;
	      while (m < xnum)
	      {
	      	 node->exlabel[m] = get_label(k++);
	         node->exval[m] = get_label(k++);
	         m++;
	      }
  	}
	m = 0;
	while (m < inum)
	{
	      node->vlabel[m] = get_label(k++);
	      node->val[m] = get_intg(k++);
	      m++;
	}
	m = 0;
	while (m < fnum)
	{
	      node->flabel[m] = get_label(k++);
	      node->fval[m] = get_float(k++);
	      m++;
	}
}

static void
link_com_node(snode) 
SRC_NODE  *snode;
{
	int	      k, inum, fnum, m;
    	COMMON_NODE   *cnode;

	cnode = (COMMON_NODE *) &snode->node.common_node;
	k = 3;
	inum = get_intg(k++);
	fnum = get_intg(k++);
	m = 0;
	while (inum > 0)
	{
	      cnode->vlabel[m] = get_label(k++);
	      cnode->val[m] = get_intg(k++);
	      inum--;
	      m++;
	}
	m = 0;
	while (fnum > 0)
	{
	      cnode->flabel[m] = get_label(k++);
	      cnode->fval[m] = get_float(k++);
	      if (debug>2)
                 fprintf(stderr,"items=%d, label='%s',val=%f\n",
				m,cnode->flabel[m],cnode->fval[m]);
	      fnum--;
	      m++;
	}
}


static int
link_grad_node(snode) 
SRC_NODE  *snode;
{
	int	      k, m, inum, fnum;
	int	      *val;
	double         *fval;
	char	      **vlabel, **flabel;
	char	      *dname, *pname;
    	COMMON_NODE   *cnode = NULL;
    	INCGRAD_NODE  *incgnode = NULL;

	if (snode->type == INCGRAD)
	{
	     incgnode = (INCGRAD_NODE *) &snode->node.incgrad_node;
	     fval = incgnode->fval;
	     val = incgnode->val;
	     vlabel = incgnode->vlabel;
	     flabel = incgnode->flabel;
	     incgnode->pattern = NULL;
	}
	else
	{
	     cnode = (COMMON_NODE *) &snode->node.common_node;
	     fval = cnode->fval;
	     val = cnode->val;
	     vlabel = cnode->vlabel;
	     flabel = cnode->flabel;
	     cnode->pattern = NULL;
	}
        pname = NULL;
	k = 3;
	inum = get_intg(k++);
	fnum = get_intg(k++);
	dname = get_label(k++); /* gradient name */
	if (snode->type == INCGRAD)
	{
	     incgnode->dname = dname;
	     if (snode->device == 12)  /* shaped_gradient ... */
	     {
	         incgnode->pattern = get_label(k++);
	     	 pname = incgnode->pattern;
	     }
	}
	else
	{
	     cnode->dname = dname;
	     if (snode->device == 12)  /* shaped_gradient ... */
	     {
	         cnode->pattern = get_label(k++);
	     	 pname = cnode->pattern;
	     }
	}
	if (snode->device == 12 && pname != NULL)
	{
	     if (!strcmp(pname, "rampup") || !strcmp(pname, "ramp_hold"))
		 snode->flag |= RAMPUP;
	     else if (!strcmp(pname, "rampdn") || !strcmp(pname, "ramp_down"))
		 snode->flag |= RAMPDN;
             get_gradient_shape(snode, pname);
	}
	m = 0;
	while (inum > 0)
	{
	      vlabel[m] = get_label(k++);
	      val[m] = get_intg(k++);
	      inum--;
	      m++;
	}
	m = 0;
	while (fnum > 0)
	{
	      flabel[m] = get_label(k++);
	      fval[m] = get_float(k++);
              if (debug>2) 
		 fprintf(stderr,"items=%d, label='%s',val=%f\n",
                        m,cnode->flabel[m],cnode->fval[m]);
	      fnum--;
	      m++;
	}
	snode->device = add_grad_count(*dname);
	switch (snode->type) {
	 case	GRAD:
		     flabel[0] = vlabel[0];
		     snode->power = (double) val[0];
		     if (val[0] == 0)
		     {
			if (vlabel[0][0] == '0') /* level is 0 */
			{
		           snode->flag = snode->flag | NINFO;
			   val[2] = 0;    /* no label */
			}
		     }
		     break;
	 case	MGRAD:
	 case	MGPUL:
	 case	OBLGRAD:
	             if (snode->type != OBLGRAD)
                     {
		         image_flag = image_flag | MAIMAGE;
		         snode->flag = snode->flag | MAIMAGE;
                     }
		     image_flag = image_flag | OBIMAGE;
		     snode->flag = snode->flag | OBLGR;
		     snode->power = fval[0];
		     val[2] = 1;
		     if (fval[0] == 0.0)
		     {
			if (flabel[0][0] == '0') /* level is 0 */
			{
		           snode->flag = snode->flag | NINFO;
			   val[2] = 0;    /* no label */
			}
		     }
		     break;
	 case	RGRAD:  // fval[0]: amp, fval[3]: multiplier(gradalt)
		     snode->power = fval[0];
		     val[2] = 1;
		     if (fval[0] == 0.0)
		     {
			if (flabel[0][0] == '0') /* level is 0 */
			{
		           snode->flag = snode->flag | NINFO;
			   val[2] = 0;    /* no label */
			}
		     }
		     break;
	 case	PEGRAD:  // val[0]: mult, fval[0]: stat, fval[1]: step, favl[2]: limit
		     image_flag = image_flag | OBIMAGE;
		     snode->flag = snode->flag | OBLGR;
		     snode->power = fval[0];
		     val[2] = 1;
		     if (fval[1] == 0.0)
		     {
			if (flabel[1][0] == '0') /* step is 0 */
			{
			   val[2] = 0;
			   if (fval[0] == 0.0 && flabel[0][0] == '0')
				snode->flag = snode->flag | NINFO;
			}
		     }
		     break;
	 case	ZGRAD:
	 case	SHGRAD:
	 case	SHVGRAD:
	 case	SH2DVGR:
	 case	SHINCGRAD:
	             if (snode->type == ZGRAD)
                     {
		         val[0] = 1;  /* set loop */
		         val[1] = 1;  /* set wait */
                     }
		     snode->power = fval[1];
		     if (fval[0] > 0.0) {
			snode->rtime = fval[0];
			snode->rtime2 = fval[0];
			snode->ptime = fval[0];
		     }
		     else
			snode->rtime2 = minTime; 
		     break;
	 case	OBLSHGR:
	 case	PEOBLVG:
	 case	PEOBLG:
	 case	PESHGR:
	 case	DPESHGR:
		     snode->power = fval[1];
		     image_flag = image_flag | OBIMAGE;
		     snode->flag = snode->flag | OBLGR;
		     if (fval[0] > 0.0) {
			snode->rtime = fval[0];
			snode->rtime2 = fval[0];
			snode->ptime = fval[0];
		     }
		     else
			snode->rtime2 = minTime;
		     break;
	}
	if (snode->type == DPESHGR) {
	    chOn_node[snode->device] = snode;
        }
        else {
	    if (snode->type == DPESHGR2) {
                if (chOn_node[snode->device] != NULL) {
                    chOn_node[snode->device]->bnode = snode;
	            chOn_node[snode->device] = NULL;
                }
            }
        }

	k = snode->device - GRADX;
        if (k >= 0 && k < GRAD_NUM) {
            if (fabs(snode->power) > gradMax[k])
                gradMax[k] = fabs(snode->power);
            if (gradMax[k] > gradMaxLevel)
                gradMaxLevel = gradMax[k];
            if (snode->power < gradMinLevel)
                gradMinLevel = snode->power;
            return(1);
        }
        return(0);
}


static void
set_rf_power(dev)
int	dev;
{
	double   fval;

        if (dev >= RFCHAN_NUM)
            return;
	switch (rfType[dev]) {
	 case UNITY:
	 case UNITYPLUS:
		fval = fineAttn[dev];
		if (fval <= 0.0)
	    	    fval = 1.0;
		if (fineStep[dev] > 1)
		{
		   // if (fval > 15) 
		        powerCh[dev] = coarseAttn[dev]
			    + 20.0 * log10(fval / fineStep[dev]);
		  //  else
		  //    powerCh[dev] = coarseAttn[dev] - 48.2;
		}
		else
		    powerCh[dev] = coarseAttn[dev];
		break;
	 case VXRC:
		if (!homodecp )
		{
		    if (dev > TODEV)
			powerCh[dev] = dhp;
		}
		else
		{
		    if (dhp < 0.0)
			powerCh[dev] = dlp;
		    else
			powerCh[dev] = dhp;
		}
		break;
	 case GEMINI:
		if (dev == TODEV)
		    powerCh[dev] = powerMax[dev];
		else
		    powerCh[dev] = coarseAttn[dev];
		break;
	 case GEMINIH:
	 case GEMINIB:
		powerCh[dev] = coarseAttn[dev];
		break;
	 case GEMINIBH:
		fval = fineAttn[dev];
		if (fval <= 0.0)
	    	    fval = 1.0;
		if (fineStep[dev] > 1)
		{
		    // if (fval > 15)
	    	       powerCh[dev] = coarseAttn[dev] 
			   + 20.0 * log10(fval / fineStep[dev]);
		    // else
		    //    powerCh[dev] = coarseAttn[dev] - 48.2;
		}
		else
	    	    powerCh[dev] = coarseAttn[dev];
		break;
	}
}

static ARRAY_NODE 
*search_array_node(code, id)
int	code, id;
{
	ARRAY_NODE  *anode;

	anode = array_start_node;
	while (anode != NULL)
	{
	    if (anode->code == code && anode->id == id)
		break;
	    anode = anode->next;
	}
	return(anode);
}


static void
link_array_element()
{
	int	num, type, k, m, code;
	char    *name, **sval;
	double   *fval;
	ARRAY_NODE   *node;

	k = 0;
	code = (int) get_intg(k++);
	m = (int) get_intg(k++);
	type = (int) get_intg(k++);
	num = (int) get_intg(k++);
	name = get_label(k++);
	if (debug > 1)
	     fprintf(stderr, " array %s has %d elements\n", name, num);
        if((node = (ARRAY_NODE *) new_arraynode(type, code, num, name)) == NULL)
	     return;
	node->id = m;
	if (node->code == SARRAY) /* system global array */
	{
	   if (strcmp(name, "ni") == 0) {
	      node->ni = 1;
              type = T_REAL;
           }
	   else if (strcmp(name, "ni2") == 0) {
	      node->ni = 2;
              type = T_REAL;
           }
	   else if (strcmp(name, "ni3") == 0) {
	      node->ni = 3;
              type = T_REAL;
           }
	}
	if (node->ni > 0)
	    num = 2;
        node->type = type;
	m = (int) get_intg(k++);
	if (type == T_STRING)
	{
	     sval = node->value.Str;
	     while (k < argIndex)
	     {
                if (m >= node->vars)
                    break;
		name = get_label(k);
		sval[m] = (char *) allocateArray (strlen(name)+1);
		strcpy(sval[m], name);
		if (debug > 1)
		   fprintf(stderr, "%s ", name);
		m++;
		k++;
	     }
	}
	else
	{
	     fval = node->value.Val;
	     while (k < argIndex)
	     {
                if (m >= node->vars)
                    break;
		fval[m] = get_float(k);
		if (debug > 1)
	   	    fprintf(stderr, "%s %f ", argArray[k], fval[m]);
		m++;
		k++;
	     }
	}
	if (debug > 1)
	     fprintf(stderr, "\n");
}



static void
link_ap_element()
{
	int	k, m;
	int     *val, *oval;
 	AP_NODE *apnode;

	apnode = new_apnode();
	if (apnode == NULL)
	    return;
	m = (int) get_intg(5);
	val = apnode->val;
	oval = apnode->oval;
	k = 6;
	while (k < argIndex)
	{
	     val[m] = get_intg(k);
	     oval[m] = val[m];
	     k++;
	     m++;
	}
}

static void
assign_list_dev()
{
	int	code, id, dev;
	ARRAY_NODE   *node;

	code = get_intg(1);
	id = get_intg(2);
	dev = get_intg(3);
	node = search_array_node(code, id);
	if (node != NULL)
	    node->dev = dev;
}

static void
assign_angle_list(int rotate)
{
	int	k, n, row, id;
        ANGLE_NODE *node, *pnode, *snode;

	if (debug > 1)
             fprintf(stderr, "assign_angle_list ... \n");
        id = get_intg(1); 
        if (id < 1) 
             return;
        if (rotate > 0)
            snode = rotate_start_node;
        else
            snode = angle_start_node;
        node = snode;
        while (node != NULL) {
             if (node->id == id)
                break;
             node = node->next;
        }
        if (node == NULL) {
	    node = (ANGLE_NODE *) allocateNode(sizeof(ANGLE_NODE));
            if (node == NULL)
                return;
            node->next = NULL;
            node->id = id; 
            node->name = get_label(2); 
            if (snode == NULL) {
                if (rotate > 0)
                   rotate_start_node = node;
                else
                   angle_start_node = node;
            }
            else {
                pnode = snode;
                while (pnode->next != NULL)
                    pnode = pnode->next;
                pnode->next = node;
            }
        }
        row = get_intg(3);
        node->size = get_intg(4); 
        n = get_intg(5);
        if (row < 0 || row > 2 || n < 1)
            return;
        if (n > 10)
            n = 10;
        for (k = 0; k < n; k++)
            node->angle_set[row][k] = get_float(k+6); 
}


static void
setup_rfgrp_list(listFlag)
int listFlag;
{
	int	id, k, n, fnum, inum;
        RFGRP_NODE *node, *pnode;

	if (debug > 1)
             fprintf(stderr, "setup_rfgrp_list ... \n");
        id = get_intg(3); 
        if (id < 1) 
             return;
        node = grp_start_node;
        while (node != NULL) {
             if (node->id == id)
                break;
             node = node->next;
        }
        if (node == NULL) {
	    node = (RFGRP_NODE *) allocateNode(sizeof(RFGRP_NODE));
            if (node == NULL)
                return;
            node->next = NULL;
            node->child = NULL;
            node->id = id;
            if (grp_start_node == NULL)
                 grp_start_node = node;
            else {
                 pnode = grp_start_node;
                 while (pnode->next != NULL)
                       pnode = pnode->next;
                 pnode->next = node;
            }
        }
        if (listFlag != 0) {
            // freqlist elements 
            fnum = get_intg(4); 
            if (fnum > 10)
                fnum = 10;
            n = 0;
            k = 5;
            while (fnum > 0) {
                node->frqList[n] = get_float(k++);
                fnum--;
                n++;
            }

            return;
        }
        node->pattern = get_label(4); 
        node->mode = get_label(5); 
        inum = get_intg(6);
        fnum = get_intg(7);
        k = 8;
        n = 0;
        while (inum > 0)
        {
             // the first is the size of freqlist
              node->vlabel[n] = get_label(k++);
              node->val[n] = get_intg(k++);
              inum--;
              n++;
        }
        n = 0;
        while (fnum > 0)
        {
             // pw, cpower, fpower, startphase, freqlist label
              node->flabel[n] = get_label(k++);
              node->fval[n] = get_float(k++);
              fnum--;
              n++;
        }
        node->width = node->fval[0];
        node->frqSize = node->val[0];
        node->coarse = node->fval[1];
        node->finePower = node->fval[2];
        node->startPhase = node->fval[3];
}

static void
setup_rfgrp_attr(SRC_NODE *snode, int type)
{
	int	      n;
    	COMMON_NODE   *cnode;
        RFGRP_NODE    *rfroot, *rfnode, *tnode;

	cnode = (COMMON_NODE *) &snode->node.common_node;
        n = cnode->val[0];
        if (n <= 0)
            return;
        rfroot = grp_start_node;
        while (rfroot != NULL) {
             if (rfroot->id == n)
                break;
             rfroot = rfroot->next;
        }
        if (rfroot == NULL)
             return;
        snode->rtime = rfroot->width;
        snode->ptime = snode->rtime;
        if (type == GRPPULSE) {
             cnode->pattern = rfroot->pattern;
             snode->rg1 = cnode->fval[0];
             snode->rg2 = cnode->fval[1];
             if (cnode->pattern != NULL)
                get_rf_shape(snode, cnode->pattern);
             return;
        }
        if (type <= 0)
             return;
        n = cnode->val[1];  // channel id
        rfnode = rfroot->child;
        while (rfnode != NULL) {
             if (rfnode->id == n)
                 break;
             rfnode = rfnode->next;
        }
        if (rfnode == NULL) {
             rfnode = (RFGRP_NODE *) allocateNode(sizeof(RFGRP_NODE));
             if (rfnode == NULL)
                 return;
             rfnode->active = 1;
             rfnode->id = n;
             rfnode->next = NULL;
             rfnode->pattern = rfroot->pattern;
             rfnode->mode = rfroot->mode;
             rfnode->frqSize = rfroot->frqSize;
             rfnode->width = rfroot->width;
             rfnode->coarse = rfroot->coarse;
             rfnode->finePower = rfroot->finePower;
             rfnode->startPhase = rfroot->startPhase;
             rfnode->newFrqList = 0;
             if (rfroot->child == NULL)
                  rfroot->child = rfnode;
             else {
                  tnode = rfroot->child;
                  while (tnode != NULL) {
                      if (tnode->id > rfnode->id) {
                           rfnode->next = tnode;
                           if (rfroot == grp_start_node)
                               rfroot->child = rfnode;
                           else 
                               rfroot->next = rfnode;
                           break;
                      }
                      if (tnode->next == NULL) {
                           tnode->next = rfnode;
                           break;
                      }
                      rfroot = tnode;
                      tnode = tnode->next;
                  }
             }
        }
        
        switch (type) {
            case  GRPNAME:
                   rfnode->pattern = get_label(9);
                   break;
            case  GRPPWR:
                   rfnode->coarse = cnode->fval[0];
                   rfnode->flabel[1] = cnode->flabel[0];
                   rfnode->fval[1] = cnode->fval[0];
                   break;
            case  GRPPWRF:
                   rfnode->finePower = cnode->fval[0];
                   rfnode->flabel[2] = cnode->flabel[0];
                   rfnode->fval[2] = cnode->fval[0];
                   break;
            case  GRPONOFF:
                   rfnode->active = cnode->val[2];
                   break;
            case  GRPSPHASE:
                   rfnode->startPhase = cnode->fval[0];
                   rfnode->flabel[3] = cnode->flabel[0];
                   rfnode->fval[3] = cnode->fval[0];
                   break;
            case  GRPFRQLIST:
                   rfnode->newFrqList = 1;
                   rfnode->flabel[4] = cnode->flabel[0];
                   break;
        }
}


static void
setup_initial_val(type)
int	type;
{
	int id, k;

	id = get_intg(1);
        if (id < 0 && type != SARRAY)
            return;
	switch (type) {
	 case  RFPWR:  /* coarse and fine power */
		if (id >= GRADX)
		    return;
		if (rfType[TODEV] < UNITY)
		    return;
	    	coarseInit[id] = get_float(2);
		fineInit[id] = get_float(3);
	    	coarseAttn[id] = coarseInit[id];
		fineAttn[id] = fineInit[id];
		break;
	 case  RTADDR: /* set the addr of real time variable */
		if (id == OLDT1)
		{
                    RTindex[id] = (int) get_intg(2);
                    RTindex[T1] = (int) get_intg(2);
		}
		else if (id  < RTMAX)
                    RTindex[id] = (int) get_intg(2);
		break;
	 case  RFID:
                for (k = 1; k < 5; k++) {
                   id = get_intg(k);
                   if (id > 0)
                      chanMap[k] = id;
                }
		break;
	 case  VERNUM:  /* version */
		dps_ver = id;
                if (get_intg(2) > 0) {
                    isNvpsg = 1;
                }
		break;
	 case  DARRAY:  /* array from create_dealy_list */
	 case  SARRAY:  /* global array */
	 case  FARRAY:  /* array from freq_list */
	 case  OARRAY:  /* array from offset_list */
	 case  PARRAY:  /* array from poffset_list */
	 case  XARRAY:  /* array from diagonal array */
		link_array_element();
		break;
	 case  ARYDIM:
	    	arraydim = (int) get_intg(1);
		break;
	 case  LISTDEV:  /* channel of offset_list or freq_list */
		assign_list_dev();
		break;
	 case  APTBL:  /* AP Table  */
		link_ap_element();
		break;
	 case  NEWACQ:  /* INOVA */
		if (id)
		{
		   newAcq = id;
		   minPw = 0.1e-6;
		}
		else
		{
		   newAcq = 0;
		   minPw = 0.2e-6;
		}
/*
		if (debug)
		   fprintf(stderr, " newacq= %d\n", id);
*/
		break;
	 case  ROTATELIST:  /* create_rotation_list */
                assign_angle_list(1);
		break;
	 case  ANGLELIST:  /* create_rotation_list */
                assign_angle_list(0);
		break;
	}
}

static void
set_dock_info()
{
        hasDock = 1;
        nodeName = get_label(1);
        nodeId = (int) get_intg(2);
        dockAt = (int) get_intg(3);
        dockFrom = (int) get_intg(4);
        dockTo = (int) get_intg(5);
	if (nodeId < 0)
	   nodeId = 0;
        if (nodeId >= 0 && nodeId < dockStart)
           dockStart = nodeId;
	if (dockAt < 0)
	   dockAt = 0;
}

static void
dump_docknode()
{
     int k;
     SRC_NODE  *node;

     if (hasDock == 0 || dock_list == NULL)
         return;
     for (k = 0; k < dock_len; k++) {
         node = dock_list[k];
         if (node != NULL) {
            fprintf(stderr, " item %d  %s  dock at %d from %d  to  %d ", node->id, node->fname, node->dockAt, node->dockFrom, node->dockTo);
            fprintf(stderr, " time %g \n", node->rtime);
         }
     }
}

/*  add the node to the list */
static int
link_dock_list(node)
SRC_NODE  *node;
{
      int k, m;

      if (nodeId == NODOCK || nodeName == NULL || node->fname == NULL)
          return(0);
      if (strcmp(nodeName, node->fname) != 0)
          return(0);
      node->id = nodeId;
      node->dockAt = dockAt;
      node->dockFrom = dockFrom;
      node->dockTo = dockTo;
      if (dock_len <= nodeId || dock_len <= dockAt) {
          if (nodeId < dockAt)
             k = dockAt + 20;
	  else
             k = nodeId + 20;
          sizeT = (size_t) (k+1);
	  if((dock_tmp_list = (SRC_NODE **) allocateNode 
             (sizeof(SRC_NODE *) * sizeT)) == 0)
	  {
             clean_src_data();
             return(0);
          }
	  if((dock_X_array = (int *) allocateNode
		(sizeof(int) * sizeT)) == 0)
	  {
             clean_src_data();
             return(0);
          }
          for (m = 0; m < k; m++) {
             dock_tmp_list[m] = NULL;
             dock_X_array[m] = 0;
	  }
          for (m = 0; m < dock_len; m++) {
             dock_tmp_list[m] = dock_list[m];
             dock_X_array[m] = dock_N_array[m];
	  }
          dock_list = dock_tmp_list;
          dock_N_array = dock_X_array;
          dock_len = k;
      }
      dock_list[nodeId] = node;
      nodeId = NODOCK;
      /*  add the number of dcoked nodes for this node */
      dock_N_array[dockAt] = dock_N_array[dockAt] + 1;
      return(1);
}

static
SRC_NODE *dup_tmp_node(snode)
SRC_NODE  *snode;
{
     SRC_NODE  *xnode;
     int       dev;

     dev = snode->device;
     xnode = (SRC_NODE *) allocateTmpNode(sizeof(SRC_NODE));
     xnode->type = DELAY;
     xnode->flag = XHGHT | XWDH;
     xnode->pflag = XDLY;
     xnode->device = dev;
     xnode->fname = snode->fname;
     xnode->stime = snode->stime;
     xnode->etime = snode->etime;
     xnode->rtime = snode->rtime;
     xnode->ptime = snode->ptime;
     xnode->rtime2 = snode->rtime2;
     xnode->visible = 1;
     xnode->dnext = NULL;
     xnode->y1 = snode->y1;
     xnode->y2 = snode->y2;
     xnode->y3 = snode->y3;
     xnode->phase = rfPhase[dev];
     xnode->phase = powerCh[dev];

     return (xnode);
}

static void
link_disp_node(dnode)
 SRC_NODE  *dnode;
{
	static SRC_NODE   *cur_disp_node;

	if (disp_start_node == NULL)
	    disp_start_node = dnode;
	else
	    cur_disp_node->dnext = dnode;
	dnode->dnext = NULL;
	cur_disp_node = dnode;
        isDispObj = 1;
        if (inParallel)
        {
            link_parallel_node(dnode);
            if (inLoop)
            {
               rlloopTime[inLoop] += dnode->rtime;
fprintf(stderr,"add loop time %g\n",dnode->rtime);
            }
        }
}

static double
adjust_kz_loop(snode)
SRC_NODE  *snode;
{
     int dev;
     double    time0, x_time, count;
     SRC_NODE  *knode, *xnode;
     COMMON_NODE *comnode;

     dev = snode->device;
     if (dev < 0 || kz_node[dev] == NULL)
         return (snode->stime);
     knode = kz_node[dev];
     comnode = (COMMON_NODE *) &knode->node.common_node;
     count = (double)comnode->val[3];
     time0 = count * (snode->stime - knode->stime - knode->rtime2 - minTime2);
     x_time = comnode->fval[0] - time0;
     if (x_time <= 0.0)
         return (snode->stime);
     xnode = dup_tmp_node(snode);
     comnode = (COMMON_NODE *) &xnode->node.common_node;
     xnode->rtime = x_time;
     xnode->rtime2 = x_time;
     xnode->dockAt = PDOCK_A4;
     xnode->fname = "kzloop delay";
     comnode->fval[0] = xnode->rtime;
     comnode->flabel[0] = knode->fname;
     xnode->ptime = xnode->rtime;
     link_disp_node(xnode);
     
     x_time = knode->stime + comnode->fval[0];
     return (x_time);
}

static 
void add_parallel_sync_node(snode)
SRC_NODE  *snode;
{
     SRC_NODE  *dnode, *pnode;

     if (parallelChan < 1 || snode->pnode == NULL)
         return;
     if (sync_node[parallelChan] != NULL)
         return;
     dnode = (SRC_NODE *) new_dpsnode();
     if (dnode == NULL)
         return;
     pnode = snode->pnode;
     pnode->next = dnode;
     dnode->next = snode;
     dnode->fname = parallelSync;
     dnode->type = PARALLELSYNC;
     dnode->flag = XMARK;
     dnode->active = -1;
     dnode->visible = 1;
     dnode->rtime = 0.0;
     dnode->dockAt = PDOCK_B1;
     dnode->device = parallelChan;
     dnode->node.common_node.val[0] = 0;
}

//  called by build_dps_node
static 
void start_build_parallel_chan(snode)
SRC_NODE  *snode;
{
     int  chan;
     char *name;

     name = snode->node.common_node.vlabel[0];
     chan = 0;
     snode->device = chan;
     if (name == NULL)
         return;
     if (!strcmp(name,"obs"))
         chan = TODEV;
     else if (!strcmp(name,"dec"))
         chan = DODEV;
     else if (!strcmp(name,"dec2"))
         chan = DO2DEV;
     else if (!strcmp(name,"dec3"))
         chan = DO3DEV;
     else if (!strcmp(name,"dec4"))
         chan = DO4DEV;
     else if (!strcmp(name,"rcvr"))
         chan = RCVRDEV;
     else if (!strcmp(name,"grad")) {
         if (gradtype[0] != 'n')
             chan = GRADX;
         else if (gradtype[1] != 'n')
             chan = GRADY;
         else if (gradtype[2] != 'n')
             chan = GRADZ;
     }
     activeCh[chan] = 1;
     if (chan < RFCHAN_NUM) {
         if ((int)strlen(dnstr[chan]) <= 0 || dnstr[chan][0] == '0')
             chan = 0;
     }
     snode->device = chan;
     if (parallelChan != chan)
         add_parallel_sync_node(snode);
     parallelChan = chan;
}

//  called by build_dps_node
static 
void end_build_parallel_chan(snode)
SRC_NODE  *snode;
{
     int   k;

     snode->device = TODEV;
     add_parallel_sync_node(snode);
     for(k = 0; k < TOTALCH; k++)
         sync_node[k] = NULL;
     parallelChan = 0;
}

//  called by build_dps_node
static 
SRC_NODE *finish_build_parallel_chan(snode)
SRC_NODE  *snode;
{
     SRC_NODE  *dnode;

     dnode = new_dpsnode();
     if (dnode == NULL)
         return (snode);
     snode->next = dnode;
     dnode->device = parallelChan;
     dnode->next = NULL;
     dnode->type = PARALLELEND;
     dnode->flag = XMARK;
     parallelChan = 0;
     return (dnode);
}

static 
void add_parallel_node(rootNode, snode)
SRC_NODE  *rootNode, *snode;
{
    SRC_NODE     *pnode, *xnode;
    double  t = snode->stime;

    pnode = rootNode;
    xnode = pnode->dnext;
    while (xnode != NULL) {
        if (xnode->stime > t)
            break;
        if (xnode->stime == t && xnode->device != snode->device) {
            if (snode->dockAt >= xnode->dockAt)
                break;
        }
        pnode = xnode;
        xnode = pnode->dnext;
    }
    pnode->dnext = snode;
    snode->dnext = xnode;
}


//  called by simulation
static 
void link_parallel_node(snode)
SRC_NODE  *snode;
{
     COMMON_NODE  *cmnode;
     SRC_NODE     *pnode;
     SRC_NODE     *xnode;
     SRC_NODE     *rootNode;
     double  parallel_times[TOTALCH];
     double t_time, t_max;
     int  addXdealy;
     int dev = snode->device;

     if (dev < 1 || dev >= GRADX)
         dev = parallelChan;
     snode->xnext = NULL;
     if (snode->type == PARALLELSTART) {
         if (sync_node[dev] == NULL)
             sync_node[dev] = snode;
         return;
     }
     if (snode->type != PARALLELEND) {
         pnode = sync_node[dev];
         if (pnode != NULL) {
             while (pnode->xnext != NULL)
                 pnode = pnode->xnext;
             pnode->xnext = snode;
         }
         return;
     }
     t_max = 0.0;
     for (dev = 1; dev < TOTALCH; dev++) {
         pnode = sync_node[dev];
         if (pnode != NULL) {
            t_time = 0.0;
            while (pnode != NULL) {
                t_time += pnode->rtime;
                pnode = pnode->xnext;
            }
            if (t_time > t_max)
                t_max = t_time;
         }
     }
     snode->rtime = 0.0;
     snode->stime = snode->stime + t_max;

     // set PARALLELSYNC time
     for (dev = 1; dev < TOTALCH; dev++) {
         pnode = sync_node[dev];
         if (pnode == NULL)
            continue;
         t_time = 0.0;
         xnode = NULL;
         while (pnode != NULL) {
             if (pnode->type == PARALLELSYNC) {
                 xnode = pnode;
                 pnode->rtime = 0.0;
                 pnode->rtime2 = 0.0;
             }
             else
                 t_time += pnode->rtime;
             pnode = pnode->xnext;
         }
         if (xnode != NULL) {
             xnode->rtime = t_max - t_time;
             if (xnode->rtime < 0.0)
                  xnode->rtime = 0.0;
             xnode->rtime2 = xnode->rtime;
             xnode->ptime = xnode->rtime;
             if (xnode->rtime > dMax)
                 dMax = xnode->rtime;
         }
     }
     t_max = 0.0;
     for (dev = 1; dev < TOTALCH; dev++) {
         pnode = sync_node[dev];
         parallel_times[dev] = 0.0;
         if (pnode != NULL) {
            t_time = pnode->stime;
            while (pnode != NULL) {
                pnode->stime = t_time;
                t_time = pnode->stime + pnode->rtime2;
                pnode->etime = t_time;
                pnode = pnode->xnext;
            }
            parallel_times[dev] = t_time;
            if (t_time > t_max)
                t_max = t_time;
         }
     }
     // extend the last time
     for (dev = 1; dev < TOTALCH; dev++) {
         pnode = sync_node[dev];
         if (pnode != NULL) {
            t_time = pnode->stime;
            while (pnode != NULL) {
                if (pnode->etime == parallel_times[dev])
                    pnode->etime = t_max;
                pnode = pnode->xnext;
            }
         }
     }
     if (snode->stime < t_max) 
        snode->stime = t_max;
     snode->etime = snode->stime;
     if (parallel_start_node == NULL)
         return;
     parallel_start_node->dnext = NULL;
     rootNode = parallel_start_node;
     for (dev = 1; dev < TOTALCH; dev++) {
         pnode = sync_node[dev];
         rootNode = parallel_start_node;
         while (pnode != NULL) {
             pnode->dnext = NULL;
             if (pnode->type == PARALLELSYNC && pnode->rtime <= 0.0) {
                  if (pnode->active == -1) {
                      pnode = pnode->xnext;
                      continue; 
                  }
             }
             add_parallel_node(rootNode, pnode);
             addXdealy = 0;
             if (pnode->type == PARALLELSYNC) {
                 if (pnode->rtime > 0.0) {
                    addXdealy = 1;
                    xnode = dup_tmp_node(pnode);
                    xnode->active = -1;
                    xnode->bnode = pnode;
                    xnode->dockAt = PDOCK_A4;
                    xnode->rtime2 = pnode->rtime;
                    xnode->ptime = pnode->rtime;
                    cmnode = (COMMON_NODE *) &xnode->node.common_node;
                    cmnode->fval[0] = pnode->rtime;
                    cmnode->flabel[0] = sync_label;
                    add_parallel_node(pnode, xnode);
                    xnode->xnext = pnode->xnext;
                    pnode = xnode;
                 }
             }
             else
                 addXdealy = 1;
             rootNode = pnode;
             if (addXdealy) {
                 xnode = dup_tmp_node(pnode);
                 xnode->type = XEND;
                 xnode->flag = 0;
                 xnode->stime = pnode->etime;
                 xnode->etime = pnode->etime;
                 xnode->rtime = 0.0;
                 xnode->ptime = 0.0;
                 xnode->rtime2 = pnode->rtime;
                 xnode->bnode = pnode;
                 xnode->dockAt = PDOCK_B2;
                 add_parallel_node(pnode, xnode);
                 rootNode = xnode;
             }
             pnode = pnode->xnext;
         }
     }
     add_parallel_node(rootNode, snode);
     for (dev = 0; dev < TOTALCH; dev++)
         sync_node[dev] = NULL;
}

/***********************************************************************
  The common input format are:
    type function_call channel numberOfInteger numberOfFloat integer... float...

  The pulse input format are:
    type function_call numberOfChannel shaped_flag numberOfInteger numOfFloat
	channel integer... float...
	...
  The gradient input format are:
    type function_call 11  gradient numberOfInteger num_float
	integer... float...
    type function_call 12(shaped)  numberIfInteger num_float gradient pattern
	integer... float...
************************************************************************/

static int
build_dps_node(origin)
int	origin;
{
    int	          type, dev, org_dev, item, rval, k;
    char          *label;
    SRC_NODE      *new_node, *cur_node, *t_node;
    COMMON_NODE   *comnode;
    EX_NODE       *exnode;

    if (origin)
        sys_nt = (int) dps_get_real(CURRENT,"nt",1, 1.0);
    cmd_name = NULL;
    if((src_start_node = new_dpsnode()) == NULL)
	return(0);
    src_start_node->type = DUMMY;
    if((fid_node = new_dpsnode()) == NULL)
	return(0);
    fid_node->type = FIDNODE;
    fid_node->id = 12345;
    fid_node->fname = "fid";
    fid_node->device = TODEV;
    fid_node->rtime2 = 0.1;
    fid_node->active = 0;
    fid_node->dnext = NULL;
    parallelChan = 0;
    inParallel = 0;
    inLoop = 0;
    comnode = (COMMON_NODE *) &fid_node->node.common_node;
    atVal = dps_get_real(CURRENT,"at", 1, 0.0);
    comnode->fval[0] = atVal;
    fid_node->ptime = atVal;
    fid_label = def_fid_phase_label;
    comnode->flabel[1] = fid_label;

    cur_node = src_start_node;
    obsChan = TODEV;
    minPw = 0.2e-6;
    rfShapeMax = 0.0;
    rfShapeMin = 0.0;
    gradShapeMax = 0.0;
    gradShapeMin = 0.0;
    if (debug > 1)
	fprintf(stderr, " build dps node\n");
    for (dev = 0; dev < TOTALCH; dev++) {
	sync_node[dev] = NULL;
	chOn_node[dev] = NULL;
    }
    while (fgets(inputs, 510, fin) != NULL)
    {
	jline++;
	parse_args(inputs);
	if (argIndex <= 0)
	     continue;
	type = atoi(argArray[0]);
	if (debug > 3)
	    fprintf(stderr, "%d %d, ", type, argIndex);
	if (type >= 900)
	{
             if (type == DOCK)
                 set_dock_info();
             else
	         setup_initial_val(type);
	     continue;
	}
	if (type <= 0 || type > LASTELEM )
	    continue;
        if((new_node = new_dpsnode()) == NULL)
            return(0);
	cur_node->next = new_node;
	new_node->pnode = cur_node;
	cur_node = new_node;
	new_node->type = type;
	new_node->fname = (char *)get_label(1);
        cmd_name = new_node->fname; 
        cmd_error = 0;
	dev = (int) get_intg(2);
        if (dev < 0)
            dev = 0;
        org_dev = dev;
	new_node->device = dev;
	if (dev == 11 || dev == 12) /* gradient statement */
	{
	    dev = link_grad_node(new_node);
            if (dev <= 0) {
                cur_node = new_node->pnode;
                cur_node->next = NULL;
	        continue;
            }
            if (hasDock) {
                if (!link_dock_list(new_node))
		    return (0);
	    }
	    continue;
	}
	comnode = (COMMON_NODE *) &new_node->node.common_node;

	switch (type) {
	case  PULSE: 
	case  SMPUL:   /* simpulse, sim3pulse */
	case  SHPUL:   /* shaped_pulse or decshaped_pulse.. */
	case  SHVPUL:  /* shapedvpulse */
	case  SMSHP:   /* simshaped_pulse */
	case  APSHPUL: /* apshaped_pulse or apshaped_decpulse.. */
	case  OFFSHP:  /* genRFShapedPulseWithOffset */
	case  SHACQ:   /* ShapedXmtNAcquire  */
	case  SPACQ:   /* SweepNAcquire  */
		if ( link_pulse_node(new_node) <= 0)
			return(0);
		break;
	case  RVOP:  /* real time operation */
	case  TBLOP: /* AP table operation */
	case  TSOP:  /* AP and integer operation */
		rt_op_node(new_node);
		dev = 0;
		break;
	case  STATUS: 
		link_com_node(new_node); 
		label = comnode->vlabel[0];
		k = comnode->val[0];
		comnode->val[1] = 0;
                dev = org_dev;
                new_node->device = org_dev;
		for (item = DODEV; item < RFCHAN_NUM; item++)
		{
        	    if((t_node = new_dpsnode()) == NULL)
		        return(0);
	    	    status_node[item] = t_node;
		    t_node->type = STATUS;
		    t_node->device = item;
		    t_node->fname = cur_node->fname;
		    // t_node->ptime = 1.0;
		    comnode = (COMMON_NODE *) &t_node->node.common_node;
		    comnode->vlabel[0] = label;
		    comnode->val[0]  = k;
		    comnode->val[1]  = 0;

                    if (dmstr[item][k] != 'n' && dmstr[item][k] !='N')
		       activeCh[item] = 1;

		    cur_node->next = t_node;
		    cur_node = t_node;
		}
		break;
	case  DECPWR:
		link_com_node(new_node); 
		rval = (int) comnode->fval[0];
		k = 0;
		while (k < 49)
		{
		    if (lk_power_table[k] >= rval)
                        break;
                    k++;
                }
		comnode->fval[1] = k + 1;
		comnode->val[0] = 1;
		break;
	case  DELAY: 
	case  SPINLK:
	case  SAMPLE:
                if (inParallel) {
                    dev = parallelChan;
                    new_node->device = parallelChan;
                }
		link_com_node(new_node); 
		if (comnode->fval[0] > 0.0) {
		    new_node->rtime = comnode->fval[0];
		    new_node->rtime2 = comnode->fval[0];
		    new_node->ptime = comnode->fval[0];
		}
		else
		    new_node->rtime2 = minTime;
	        if (type == SAMPLE)
		    status_node[TODEV] = NULL;
                break;
	case  JFID: 
	case  ACQUIRE: 
                if (inParallel) {
                    dev = parallelChan;
                    new_node->device = parallelChan;
                }
		link_com_node(new_node); 
		k = 0;
		if (hasDock || type == JFID)
		    k = 1;
		if ( k == 0 && (strncmp(new_node->fname,"explicit",8) != 0))
                {
                   comnode->fval[2] = comnode->fval[0] * comnode->fval[1] / 2.0;
		   if (comnode->fval[2] > 0.0) {
		      new_node->rtime = comnode->fval[2];
		      new_node->ptime = comnode->fval[2];
                   }
                }
		else if (newAcq > 1)
		{
		    new_node->rtime = comnode->fval[2];
		    new_node->ptime = comnode->fval[2];
                }
		if (new_node->rtime > 0.0)
		      new_node->rtime2 = new_node->rtime;
		else
		      new_node->rtime2 = minTime;
		// status_node[TODEV] = NULL;
		status_node[dev] = NULL;
                break;

	case  SETST: /* setstatus */
		link_com_node(new_node); 
                dev = org_dev;
                new_node->device = org_dev;
		status_node[dev] = new_node;
                break;

	case  DEVON:   /* channel on or off */
		link_com_node(new_node); 
        	if((t_node = new_dpsnode()) == NULL)
            		return(0);
		new_node->next = t_node;
		cur_node = t_node;
		t_node->fname = new_node->fname;
		t_node->device = dev;
		t_node->node.common_node.vlabel[0] = comnode->vlabel[0];
		t_node->node.common_node.val[0] = comnode->val[0];
		link_com_node(t_node); 
		status_node[dev] = NULL;
		if(comnode->val[0])
		{
		    t_node->type = RFONOFF;
		}
		else /* set channel off */
		{
		    t_node->type = type;
		    new_node->type = RFONOFF;
		}
		break;
	case PRGON:
	case PRGOFF:
	case VPRGON:  // obsprgonOffset...
		link_com_node(new_node); 
        	if((t_node = new_dpsnode()) == NULL)
            		return(0);
		cur_node->next = t_node;
		t_node->type = PRGMARK;
		t_node->fname = new_node->fname;
		t_node->device = dev;
		if (type == PRGON || type == XPRGON || type == VPRGON)
		{
		   t_node->node.common_node.val[0] = 1;
		   t_node->node.common_node.flabel[0] = comnode->flabel[0];
		   t_node->node.common_node.fval[0] = comnode->fval[0];
		   t_node->node.common_node.flabel[1] = comnode->flabel[1];
		   t_node->node.common_node.fval[1] = comnode->fval[1];
		   t_node->node.common_node.flabel[2] = comnode->flabel[2];
		   t_node->node.common_node.fval[2] = comnode->fval[2];
                   if (type == VPRGON)
                   {
                        t_node->type = VPRGMARK;
		        t_node->node.common_node.flabel[3] = comnode->flabel[3];
		        t_node->node.common_node.fval[3] = comnode->fval[3];
                   }
		}
		else
		   t_node->node.common_node.val[0] = 0;
		link_com_node(t_node); 
		cur_node = t_node;
        	if((t_node = new_dpsnode()) == NULL)
            	    return(0);
		cur_node->next = t_node;
		link_com_node(t_node); 
                t_node->type = XDEVON;
		t_node->fname = new_node->fname;
		t_node->device = dev;
		if (type == PRGON || type == XPRGON)
		   t_node->node.common_node.val[0] = 1;
                else
		   t_node->node.common_node.val[0] = 0;
                if (type == VPRGON)
                {
		   t_node->node.common_node.val[0] = 1;
                   t_node->type = VDEVON;
                }
		cur_node = t_node;
		break;
	case  GLOOP: 
                if (inParallel) {
                    dev = parallelChan;
                    new_node->device = parallelChan;
                }
		link_com_node(new_node); 
		comnode->val[3] = comnode->val[0];
		comnode->vlabel[3] = comnode->vlabel[0];
		break;

	case  DECLVL: 
	case  POWER:
        case  PWRF:
        case  PWRM:
        case  RLPWRF:  /* rlpwrf or rlpwrm */
        case  RLPWRM:  /* rlpwrf or rlpwrm */
        case  VRLPWRF:  /* vdecpwrf, vdecpwrf, vdec2pwrf */
	case  ROTATE:
		link_com_node(new_node); 
                dev = org_dev;
                new_node->device = org_dev;
		break;
	case  XGATE:
	case  ROTORP:
	case  ROTORS:
	case  BGSHIM:
	case  SHSELECT:
	case  GRADANG:
        case  ROTATEANGLE:
        case  EXECANGLE:
                if (inParallel) {
                    dev = parallelChan;
                    new_node->device = parallelChan;
                }
		link_com_node(new_node); 
		new_node->rtime2 = minTime2;
		break;
	case  EXPTIME:
		expTime = get_float(1);
		break;
	case  PARALLEL: 
		link_com_node(new_node); 
		new_node->flag = comnode->val[0];
		break;
	case  JPSGIF: 
                hasIf = 1;
		link_com_node(new_node); 
		new_node->flag = XMARK;
		break;
	case  VDELAY: 
	case  VDLST: 
                if (inParallel) {
                    dev = parallelChan;
                    new_node->device = parallelChan;
                }
		link_com_node(new_node); 
		break;
	case  DCPLON: 
		link_ex_node(new_node); 
	        exnode = (EX_NODE *) &new_node->node.ex_node;
		exnode->cond = 1;
		for (k = 0; k < exnode->exnum; k++) {
		   if (strcmp(exnode->exlabel[k], "condition") == 0) {
		      if (strcmp(exnode->exval[k], "true") != 0)
			  exnode->cond = 0;
		      break;
		   }
		}
		break;
	case  XTUNE:
        case  XMACQ:
		link_com_node(new_node); 
		new_node->rtime = atVal;
		if (atVal > 0.0)
		    new_node->rtime2 = atVal;
		else
		    new_node->rtime2 = minTime;
		if (type == XMACQ)
		    new_node->phase = comnode->val[0];
		new_node->ptime = new_node->rtime;
		break;
        case  INITGRPPULSE:
                setup_rfgrp_list(0);
		break;
        case  INITFRQLIST:
                setup_rfgrp_list(1);
		break;
        case  GRPPULSE:
		link_com_node(new_node); 
		setup_rfgrp_attr(new_node, type);
		break;
        case  GRPNAME:
		link_com_node(new_node); 
		setup_rfgrp_attr(new_node, type);
		break;
        case  GRPPWR:
		link_com_node(new_node); 
		setup_rfgrp_attr(new_node, type);
		break;
        case  GRPPWRF:
		link_com_node(new_node); 
		setup_rfgrp_attr(new_node, type);
		break;
        case  GRPONOFF:
		link_com_node(new_node); 
		setup_rfgrp_attr(new_node, type);
		break;
        case  GRPSPHASE:
		link_com_node(new_node); 
		setup_rfgrp_attr(new_node, type);
		break;
        case  GRPFRQLIST:
		link_com_node(new_node); 
		setup_rfgrp_attr(new_node, type);
		break;
        case  ACTIVERCVR:
                comnode->dname = get_label(5);
                comnode->pattern = get_label(6);
		break;
        case  PARALLELSTART:  // build_node
                inParallel = 1;
		link_com_node(new_node);
                // set XLOOP for search priority
	        new_node->flag = XLOOP;
		new_node->rtime2 = minTime2;
		start_build_parallel_chan(new_node);
                dev = new_node->device;
                if (dev == RCVRDEV)
                    rfchan[RCVRDEV] = 1;
		break;
        case  PARALLELEND:
	        link_com_node(new_node); 
                dev = 1;
                new_node->device = 1;
		new_node->flag = XLOOP;
		end_build_parallel_chan(new_node);
                dev = new_node->device;
                inParallel = 0;
		break;
        case  PARALLELSYNC:
		link_com_node(new_node);
                new_node->dockAt = PDOCK_A4;
                if (inParallel) {
                    dev = parallelChan;
                    new_node->device = parallelChan;
                    sync_node[parallelChan] = new_node;
                    new_node->flag = XMARK;
                }
		break;
	default:
                if (inParallel) {
                    dev = parallelChan;
                    new_node->device = parallelChan;
                }
		link_com_node(new_node); 
/*
		new_node->flag = XMARK;
*/
		break;
	}
	if (dev > 0 && dev < TOTALCH)
		activeCh[dev] = 1;
        if (hasDock) {
              if (!link_dock_list(new_node)) {
	         Werrprintf("dps error: '%s' id was missing.\n", new_node->fname);
		 break;
	      }
	}
    }
    if (inParallel) {
	Werrprintf("dps error:  parallelend was missing.\n");
        cur_node = finish_build_parallel_chan(cur_node);
    }
    cur_node->next = fid_node;
    gmax = 100;
    mgmax = 32000;
    if (image_flag & OBIMAGE || image_flag & MAIMAGE)
    {
	gmax = (int) dps_get_real(CURRENT,"gmax", 1, 100.0);
	mgmax = (int) dps_get_real(CURRENT,"gradstepsz", 1, 1.0);
    }
    if (debug > 1)
	fprintf(stderr, " build...done\n");
    if (debug > 2)
	dump_docknode();
    return(1);
}


static int
get_APval(tbl, item)
 int    tbl, item;
{
	int	id;
	AP_NODE *ap_node, *t_node;

	id = tbl - RTindex[T1];
	if (id < 0 || id >= 60)
	    return(0);
	t_node = NULL;
	ap_node = apstart;
	while (ap_node != NULL)
	{
	    if (ap_node->id == id)
	    {
		t_node = ap_node;
		break;
	    }
	    ap_node = ap_node->next;
	}
	if (t_node == NULL)
	    return(0);
	if (t_node->autoInc)
	{
	    item = t_node->index;
	    t_node->index++;
	    if (t_node->index >= t_node->size)
		t_node->index = 0;
	}
	if (t_node->divn > 1)
	   item = (item / t_node->divn) % t_node->size + 1;
	else
	   item = item % t_node->size;
	if (item >= t_node->size)
	   item = 0;
	if (item < 0)
	   item = 0;
	return(t_node->val[item]);
}


static int
get_RTval(rtindex)
	int    rtindex;
{
	int	 item;

	if (rtindex >= RTindex[T1] && rtindex < RTindex[T1] + 60)
	{ /*  Aptable index  */
	     item = get_APval(rtindex, RTval[CT]);
	     return(item);
	}
	for (item = 1; item < T1; item++)
        {
             if (rtindex == RTindex[item])
                 break;
        }
        if (item >= T1)
             return(0);
        return(RTval[item]);
}

static void
set_RTval(rtindex, val)
	int    rtindex, val;
{
	int	 item;

	if (rtindex >= RTindex[T1] && rtindex < RTindex[T1] + 60)
	{ /*  Aptable index  */
	     return;
	}

	for (item = 1; item < T1; item++)
	{
	     if (rtindex == RTindex[item])
		 break;
	}
	if (item < T1)
	     RTval[item] = val;
}


/*   val[] is the address of real-time variable */
static void
rt_operation(snode)
    SRC_NODE     *snode;
{
	int	 v1, v2, v3;
	int	 *val;
	RT_NODE  *node;

	node = (RT_NODE *) &snode->node.rt_node;
	val = node->val;
	switch (node->op) {
	 case RADD:
		v1 = get_RTval(val[0]) + get_RTval(val[1]);
		set_RTval(val[2], v1);
		break;
	 case RASSIGN:
		v1 = get_RTval(val[0]);
		set_RTval(val[1], v1);
		break;
	 case RDBL:
		v1 = 2 * get_RTval(val[0]);
		set_RTval(val[1], v1);
		break;
	 case RDECR:
		v1 = get_RTval(val[0]) - 1;
		set_RTval(val[0], v1);
		break;
	 case RDIVN:
		v2 = get_RTval(val[1]);
		if (v2 == 0)
		    v2 = 1;
		v1 = get_RTval(val[0]) / v2;
		set_RTval(val[2], v1);
		break;
	 case RHLV:
		v1 = get_RTval(val[0]) / 2;
		set_RTval(val[1], v1);
		break;
	 case RINCR:
		v1 = get_RTval(val[0]) + 1;
		set_RTval(val[0], v1);
		break;
	 case RMOD2:
		v1 = get_RTval(val[0]);
		v2 = v1 % 2;
		set_RTval(val[1], v2);
		break;
	 case RMOD4:
		v1 = get_RTval(val[0]);
		v2 = v1 % 4;
		set_RTval(val[1], v2);
		break;
	 case RMODN:
		v1 = get_RTval(val[0]);
		v2 = get_RTval(val[1]);
		if (v2 == 0)
		   v2 = 1;
		v3 = v1 % v2;
		set_RTval(val[2], v3);
		break;
	 case RMULT:
		v1 = get_RTval(val[0]) * get_RTval(val[1]);
		set_RTval(val[2], v1);
		break;
	 case RSUB:
		v1 = get_RTval(val[0]) - get_RTval(val[1]);
		set_RTval(val[2], v1);
		break;
	}
}

static void
ap_operation(snode)
    SRC_NODE     *snode;
{
	int	 v1, v2, v3, size, k;
	int	 *val_1, *val_2;
	RT_NODE  *node;
	AP_NODE  *ap_node, *des_node, *m_node;

	if (!ap_op_ok)
	    return;
	node = (RT_NODE *) &snode->node.rt_node;
	v1 = node->val[0] - RTindex[T1];
	if (v1 >= 60 || v1 < 0) /* out of boundary */
	     return;
	v2 = node->val[1] - RTindex[T1];
	if (v2 >= 60 || v2 < 0) /* out of boundary */
	     return;
	v3 = node->val[2];
	des_node = NULL;
	m_node = NULL;
	ap_node = apstart;
	while (ap_node != NULL)
	{
	    if (ap_node->id == v1)
		    des_node = ap_node;
	    else if (ap_node->id == v2)
		    m_node = ap_node;
	    ap_node = ap_node->next;
	}
	if (des_node == NULL || m_node == NULL)
		return;
	size = m_node->size;
	if (size > des_node->size)
	{
            sizeT = (size_t) size;
	    val_1 = des_node->val;
     	    val_2 = (int *) allocateAp (sizeof(int) * sizeT);
	    if (val_2 == NULL)
		    return;
	    for (k = 0; k < des_node->size; k++)
		    *val_2++ = *val_1++;
	    while (k < size)
	    {
		*val_2++ = 0;
		k++;
	    }
	    des_node->val = val_2;
	    des_node->size = size;
     	    val_1 = (int *) allocateAp (sizeof(int) * sizeT);
	    if (val_1 != NULL)
	    {
	        for (k = 0; k < size; k++)
		    *val_1++ = *val_2++;
	    }
	}
	val_1 = des_node->val;
	val_2 = m_node->val;
	des_node->changed = 1;
		
	switch (node->op) {
	 case RADD:
		for (k = 0; k < size; k++)
		{ 
		     *val_1 = *val_1 + *val_2;
		     val_1++;
		     val_2++;
		}
		break;
	 case RDIVN:
		for (k = 0; k < size; k++)
		{
		     if (*val_2 != 0)
		         *val_1 = *val_1 / *val_2;
		     val_1++;
		     val_2++;
		}
		break;
	 case RSUB:
		for (k = 0; k < size; k++)
		{
		     *val_1 = *val_1 - *val_2;
		     val_1++;
		     val_2++;
		}
		break;
	 case RMULT:
		for (k = 0; k < size; k++)
		{
		     *val_1 = *val_1 * *val_2;
		     val_1++;
		     val_2++;
		}
		break;
	 default:
		return;
		break;
	}
	if (v3 > 0)
	{
	    val_1 = des_node->val;
	    for (k = 0; k < size; k++)
	    {
		*val_1 = *val_1 % v3;
		val_1++;
	    }
	}
}


static void
apv_operation(snode)
    SRC_NODE     *snode;
{
	int	 v1, v2, v3, size, k;
	int	 *val_1;
	RT_NODE  *node;
	AP_NODE  *ap_node, *des_node;

	if (!ap_op_ok)
	    return;
	node = (RT_NODE *) &snode->node.rt_node;
	v1 = node->val[0] - RTindex[T1];
	if (v1 >= RTMAX || v1 < 0) /* out of boundary */
	     return;
	v2 = node->val[1];
	v3 = node->val[2];
	des_node = NULL;
	ap_node = apstart;
	while (ap_node != NULL)
	{
	    if (ap_node->id == v1)
	    {
		des_node = ap_node;
		break;
	    }
	    ap_node = ap_node->next;
	}
	if (des_node == NULL)
	    return;
	val_1 = des_node->val;
	size = des_node->size;
	des_node->changed = 1;
	switch (node->op) {
	 case RADD:
		for (k = 0; k < size; k++)
		{ 
		     *val_1 = *val_1 + v2;
		     val_1++;
		}
		break;
	 case RDIVN:
		if (v2 == 0)
		     return;
		for (k = 0; k < size; k++)
		{ 
		     *val_1 = *val_1 / v2;
		     val_1++;
		}
		break;
	 case RSUB:
		for (k = 0; k < size; k++)
		{
		     *val_1 = *val_1 - v2;
		     val_1++;
		}
		break;
	 case RMULT:
		for (k = 0; k < size; k++)
		{
		     *val_1 = *val_1 * v2;
		     val_1++;
		}
		break;
	 default:
		return;
		break;
	}
	if (v3 > 0)
	{
	    val_1 = des_node->val;
	    for (k = 0; k < size; k++)
	    {
		*val_1 = *val_1 % v3;
		val_1++;
	    }
	}
}

static int
link_acquire_node(snode)
 SRC_NODE  *snode;
{
     int k, n, dev;
     SRC_NODE     *xnode, *new_node;
     COMMON_NODE  *cnode, *cnode2;

     if (rfchan[snode->device])
         dev = snode->device;
     else
         dev = 0;
     if (rcvrsNum < 1 || inParallel) {
         if (dev > 0) {
              snode->y1 = orgy[dev];
              snode->y2 = orgy[dev];
              snode->y3 = orgy[dev];
         }
         return dev;
     }
     xnode = snode;
     while (xnode != NULL) {
         xnode->device = 0;
         xnode->active = 0;
         xnode = xnode->bnode;
     }
     xnode = snode;
     new_node = snode;
     cnode =  (COMMON_NODE *) &snode->node.common_node;
     dev = 0;
     n = numrfch;
     if (n < numch)
        n = numch;
     for (k = 0; k < n; k++) {
         if (rfchan[k+1] != 0 && (rcvrsStr[k] == 'y' || rcvrsStr[k] == 'Y')) {
              dev = k + 1;
              if (new_node == NULL) {
                 new_node = new_dpsnode();
                 if (new_node == NULL)
                     return dev;
                 xnode->bnode = new_node;
                 new_node->type = snode->type;
                 new_node->fname = snode->fname;
                 new_node->bnode = NULL;
                 new_node->next = NULL;
              }

              new_node->device = dev;
              new_node->active = 1;
              new_node->rtime = snode->rtime;
              new_node->rtime2 = snode->rtime2;
              new_node->ptime = snode->ptime;
              new_node->phase = snode->phase;
              new_node->y1 = orgy[dev];
              new_node->y2 = orgy[dev];
              new_node->y3 = orgy[dev];
              cnode2 =  (COMMON_NODE *) &new_node->node.common_node;
              cnode2->flabel[0] = cnode->flabel[0];
              cnode2->flabel[1] = cnode->flabel[1];
              cnode2->fval[0] = cnode->fval[0];
              cnode2->fval[1] = cnode->fval[1];
              cnode2->fval[2] = cnode->fval[2];
              cnode2->fval[7] = cnode->fval[7];
              xnode = new_node;
              new_node = xnode->bnode;
         }
     }
     return dev;
}

static
SRC_NODE *chk_if_stmt(node, xmode)
	SRC_NODE  *node;
	int	   xmode;
{
	SRC_NODE     *cnode;
    	COMMON_NODE  *comnode;
	int	     level, val, v1, v2;

	cnode = node;
	comnode = (COMMON_NODE *) &cnode->node.common_node;
        val = 0;
	if (xmode == 1)  /* ifzero  */
	{
	    val = get_RTval(comnode->val[0]);
	    if (val == 0)
	    	return(node);
	}
	if (xmode == 3)  /* ifrt...  */
	{
	    v1  = get_RTval(comnode->val[1]);
	    v2  = get_RTval(comnode->val[2]);
            switch (comnode->val[0]) {
                case RT_LT:
                         if (v1 < v2)
                            val = 1;
                         break;
                case RT_GT:
                         if (v1 > v2)
                            val = 1;
                         break;
                case RT_GE:
                         if (v1 >= v2)
                            val = 1;
                         break;
                case RT_LE:
                         if (v1 <= v2)
                            val = 1;
                         break;
                case RT_EQ:
                         if (v1 == v2)
                            val = 1;
                         break;
                case RT_NE:
                         if (v1 != v2)
                            val = 1;
                         break;
            }
	    set_RTval(comnode->val[3], val);
	    if (val != 0)
	    	return(node);
	}
	level = 0;
	while (cnode->next != NULL)
	{
	   cnode = cnode->next;
	   switch (cnode->type) {
	    case IFZERO:
			level++;
			break;
	    case ELSENZ:
			if (xmode != 2 && level == 0)
			    return(cnode);
			break;
	    case ENDIF:
			if (level == 0)
			    return(cnode);
			level--;
			break;
	    }
	}
/*
	if (level > 0)
             Wprintf("missing 'endif' in 'ifzero' group.\n");
*/
	return(cnode);
}



static void
set_chan_height(snode, vpower)
SRC_NODE   *snode;
double      vpower;
{
	int   height, dev;

	dev = snode->device;
	if (vpower > 0.0)
	{
            height = vpower * (double) pulseHeight / powerMax[dev];
	    if (height < pulseMin)
	         height = pulseMin;
	    if (height > pulseHeight)
	         height = pulseHeight;
	}
	else {
	    height = pulseMin;
        }
        snode->y2 = height + orgy[dev];
        chyc[dev] = snode->y2;
}

static void
set_grad_height(snode, vpower)
SRC_NODE   *snode;
double      vpower;
{
	int   dev, height, min;
	COMMON_NODE *cnode;

	dev = snode->device - GRADX;
        if (dev < 0 || dev >= GRAD_NUM)
             return;
/*
	if (snode->flag & MAIMAGE)
	   height = ((double) gradHeight * vpower) / mgmax;
	else if (snode->flag & OBLGR)
	   height = ((double) gradHeight * vpower) / gmax;
	else
	   height = ((double) gradHeight * vpower) / gradMax[dev];
        if (vpower > gradMax[dev])
	   gradMax[dev] = vpower;
*/
        
        if (vpower > gradMaxLevel)
	   gradMaxLevel = vpower;
	height = ((double) gradHeight * vpower) / gradMaxLevel;
	if (height > gradHeight)
	    height = gradHeight;
	if (height < -gradHeight)
	    height = -gradHeight;
	cnode = (COMMON_NODE *) &snode->node.common_node;
	if (cnode->pattern != NULL) // shapedgradient
            min = gradHeight / 3;
        else
            min = gradMin;
	if (vpower > 0.0)
	{
	    if (height < min)
	         height = min;
	}
	else if (vpower < 0.0)
	{
	    if (height > -min)
		height = -min;
	}
        dev = snode->device;
	snode->y2 = height + orgy[dev];
	chyc[dev] = snode->y2;
}

static void
get_APelem(snode)
    SRC_NODE      *snode;
{
	int    val;
    	COMMON_NODE   *comnode;
	  
	comnode = (COMMON_NODE *) &snode->node.common_node;
	val = get_RTval(comnode->val[1]);
	val = get_APval(comnode->val[0], val);
	set_RTval(comnode->val[2], val);
}

static void
set_chan_power(node, d_link)
SRC_NODE  *node;
int	  d_link;
{
	int	    dev;
	COMMON_NODE *comnode;

	dev = node->device;
	set_rf_power(dev);
	if (d_link)
	{
	    link_disp_node(node);
	    comnode = (COMMON_NODE *) &node->node.common_node;
	    comnode->fval[1] = coarseAttn[dev];
	    comnode->fval[2] = fineAttn[dev];
	    node->power = powerCh[dev];
	    comnode->catt = coarseAttn[dev];
	    comnode->fatt = fineAttn[dev];
	    if (node->flag & XHGHT)
	        set_chan_height(node, powerCh[dev]);
	    node->y3 = node->y2;
	}
}



static void
simple_simulate(ct)
int	ct;
{
    SRC_NODE      *curnode;
    COMMON_NODE   *comnode;

   	curnode = src_start_node;
   	while (curnode != NULL)
   	{
	   comnode = (COMMON_NODE *) &curnode->node.common_node;
	   switch (curnode->type) {
	    case RVOP:
		  rt_operation(curnode);
                  break;
	    case  TBLOP: /* AP table operation */
		  ap_operation(curnode);
                  break;
	    case  TSOP:  /* AP and integer operation */
		  apv_operation(curnode);
                  break;
	    case SETVAL:  /* initval  */
                  if (ct == 0)
                    set_RTval(comnode->val[0], (int)comnode->fval[0]);
                  break;
	    case IFZERO:
 		  curnode = chk_if_stmt(curnode, 1);
                  break;
	    case ELSENZ:
		  curnode = chk_if_stmt(curnode, 2);
                  break;
	    case IFRT:
		  curnode = chk_if_stmt(curnode, 3);
		  break;		
	    case SLOOP:
	    case GLOOP:
            case KZLOOP:
  		  set_RTval(comnode->val[1], 1);
                  break;
 	    case GETELEM:
		  get_APelem(curnode);
                  break;
	    case INITDLY:
		  if (ct == 0)
                  {
                      if (comnode->val[0] < RTDELAY)
                           RTdelay[comnode->val[0]] = (int)comnode->fval[0];
                  }
                  break;
            case MSLOOP:
            case PELOOP:
            case PELOOP2:
		  if (*comnode->vlabel[0] == 'c')
 		  {
		      if (comnode->fval[0] > 0.5)
                          set_RTval(comnode->val[1], (int)comnode->fval[0]);
                      else
                          set_RTval(comnode->val[1], 1);
                  }
                  else if (*comnode->vlabel[0] == 's')
                  {
                      if (curnode->type == MSLOOP && comnode->fval[0] < 1)
                      {
                          set_RTval(comnode->val[1], (int)comnode->fval[0]);
                          set_RTval(comnode->val[2], 0);
                      }
                      else if (curnode->type == PELOOP || curnode->type == PELOOP2)
                      {
                          if (comnode->fval[0] > 0.5)
                             set_RTval(comnode->val[1], (int) comnode->fval[0]);
                          else
                          {
                             set_RTval(comnode->val[1], 0);
                             set_RTval(comnode->val[2], 0);
                          }
                      }
                  }
                  break;
            case RLLOOP:
                  set_RTval(comnode->val[1], comnode->val[0]);
                  set_RTval(comnode->val[2], 1);
                  break;
            case NWLOOP:
                  set_RTval(comnode->val[0], (int) comnode->fval[0]);
                  break;
	    }
            curnode = curnode->next;
       }
       ap_op_ok = 0;
}


static void
simulate_psg(ct, do_link)
int	ct, do_link;
{
    int   	  exist, k, k2, k3;
    int		  dev, oph;
    int		  acquireCnt;
    int		  shGrad, gradDev;
    int		  exFlag;
    int		  halfHeight;
    int		  *val;
    char          status;
    double	  rval;
    double	  t_time, p_time;
    double	  curPower;
    double	  *fval;
    SRC_NODE      *curnode, *dpnode;
    SRC_NODE      *spnode;
    COMMON_NODE   *comnode;
    INCGRAD_NODE  *incgnode;
    ARRAY_NODE    *anode;
    EX_NODE       *exnode;


   if (debug > 1)
	fprintf(stderr, "  simulate: ct= %d  link= %d\n", ct, do_link);
   if (ct < 0)
      ct = 0;
   if (cyclephase)
      oph = ct % 4;
   else
      oph = 0;
   RTval[OPH] = oph;
   RTval[CT] = ct;
   RTval[CTSS] = ct;
   if (first_round)
      RTval[IX] = ct;
   if ( !do_link )
   {
	simple_simulate(ct);
	return;
   }
   fid_label = def_fid_phase_label;
   acquireCnt = 0;
   pulseCnt = 0.0;
   delayCnt = 0.0;
   pMax = 0.0;
   dMax = 0.0;
   t_time = 0.0;
   parallelChan = 0;
   inParallel = 0;
   releaseTmpNode();
   for(dev = 0; dev < RFCHAN_NUM; dev++)
   {
	coarseAttn[dev] = coarseInit[dev];
	fineAttn[dev] = fineInit[dev];
	set_rf_power(dev);
	rfPhase[dev] = 0.0;
	frqOffset[dev] = offsetInit[dev];
	exPhase[dev] = 0.0;
	phaseStep[dev] = 90.0;
   }
   for(dev = 0; dev < TOTALCH; dev++)
   {
	chyc[dev] = orgy[dev];
	chMod[dev] = 0;
	ampOn[dev] = 0;
	prg_node[dev] = NULL;
	status_node[dev] = NULL;
	chOn_node[dev] = NULL;
	sync_node[dev] = NULL;
	kz_node[dev] = NULL;
   }
   gradDev = GRADX;
   for (dev = GRADX; dev <= GRADZ; dev++) {
        if (rfchan[dev]) {
            gradDev = dev; // for parallel gradient
            break;
        }
   }

   set_rcvrs_param();
   disp_start_node = NULL;
   curnode = src_start_node;
   spnode = spare_src_node;
   p_time = 0.0;
   while (curnode != NULL)
   {
	comnode = (COMMON_NODE *) &curnode->node.common_node;
	val = comnode->val;
	fval = comnode->fval;
	dev = curnode->device;
        if (inParallel && dev >= GRADX) {
           if (rfchan[dev] == 0) {
               dev = gradDev;
               curnode->device = gradDev;
           }
        }
	curnode->y1 = chyc[dev];
	curnode->y2 = chyc[dev];
	curnode->y3 = orgy[dev];
	shGrad = 0;
	exFlag = 0;
	halfHeight = 0;
        isDispObj = 0;

        if(debug > 2) {
	    if (curnode->fname != NULL) 
	     fprintf(stderr, "%s ", curnode->fname);
	}
	if (dev < GRADX)
	{
	     curnode->phase = rfPhase[dev];
	     curnode->power = powerCh[dev];
	     comnode->catt = coarseAttn[dev];
	     comnode->fatt = fineAttn[dev];
	}
        if (inParallel && curnode->dockAt == NODOCK)
             curnode->dockAt = PDOCK_MID;
        curnode->stime = t_time;
	switch (curnode->type) {
	case DELAY:
		link_disp_node(curnode);
/**
		curnode->flag = XWDH;
**/
		delayCnt++;
		if (curnode->rtime > dMax)
		    dMax = curnode->rtime;
		curnode->y3 = chyc[dev];
		fval[1] = rfPhase[dev];
		curnode->phase = rfPhase[dev];
		curnode->power = powerCh[dev];
                curnode->dockAt = PDOCK_A5;
/*
                comnode->catt = coarseAttn[dev];
                comnode->fatt = fineAttn[dev];
*/
	        break;
	case PULSE:
	case SHPUL:
	case SHVPUL:
	case SMPUL:
	case SMSHP:
	case OFFSHP:
	case SHACQ:
	case SPACQ:
	case GRPPULSE:
		exist = 0;
		dpnode = curnode;
		if (jfout)
		   fprintf(jfout, "%d ", curnode->line);
		while (dpnode != NULL)
		{
		    comnode = (COMMON_NODE *) &dpnode->node.common_node;
		    k = dpnode->device;
		    if (rfchan[k])
		         exist = 1;
		    comnode->val[3] = get_RTval(comnode->val[0]); /* phase */
		    if (curnode->type == SHVPUL)
		       comnode->val[5] = get_RTval(comnode->val[1]); /* amp */
                    if ( (newAcq > 1) /*  && (curnode->type == PULSE) */ ){
                       if ( strcmp(comnode->flabel[3],"Unaltered") ) 
		       {  coarseAttn[k] = comnode->fval[3];
		       }
		       if ( strcmp(comnode->flabel[4],"Unaltered")  )
		       {  fineAttn[k] = comnode->fval[4];
		       }
                       set_rf_power(k);
		    }
		    if (jfout)
		       fprintf(jfout, " %d", comnode->val[3] % 4);
		    dpnode->power = powerCh[k];
		    comnode->catt = coarseAttn[k];
		    comnode->fatt = fineAttn[k];
		    if (newAcq == 2) 
                    {
		        rval = exPhase[k] + (double)comnode->val[3]/10.0;
                    }
                    else
                    {
		        rval = exPhase[k] + 90.0 * comnode->val[3];
                    }
		    /* dpnode->phase = fmod(rval, 360.0); */
		    dpnode->phase = rval;
		    rfPhase[dev] = rval;
		    dpnode->y1 = orgy[k];
		    set_chan_height(dpnode, dpnode->power);
		    chyc[k] = orgy[k];
/**
		    chMod[k] = 0;
		    dpnode->mode = ampOn[k];
**/
		    dpnode->y3 = orgy[k];
		    dpnode = dpnode->bnode;
		}
		if (jfout)
		   fprintf(jfout, "\n");
		if (exist)
		{
		    link_disp_node(curnode);
		    if (curnode->type == SPACQ || curnode->type == SHACQ)
		        acquireCnt++;
		    if (curnode->type == SHPUL || curnode->type == SMSHP ||
			curnode->type == SHVPUL || curnode->type == GRPPULSE )
		    	pulseCnt += 2;
		    else
		    	pulseCnt++;
		    if (curnode->rtime > pMax)
		        pMax = curnode->rtime;
		    exFlag = ASTNODE | ACHNODE | RSTNODE | RCHNODE | APGNODE | RPGNODE;
		}
	        break;
	case STATUS:
	case SETST: /* setstatus */
	        if (curnode->type == STATUS) {
	            if (rfchan[dev])
	            {
		        k = strlen(dmstr[dev]);
                        if (k > val[0])
                            status = dmstr[dev][val[0]];
                        else
                            status = dmstr[dev][k-1];
                        if (status != 'n' && status !='N')
		    	    val[1] = 1;
	            }
	        }
		if (dev == 0)
		{
		    val[1] = 1;
		    link_disp_node(curnode);
		}
		else if (rfchan[dev])
		{
		    curnode->flag = XWDH | XBOX;
		    curnode->rtime2 = minTime;
		    link_disp_node(curnode);
		    if (val[1])  /* if on */
		    {
		        link_disp_node(curnode);
                        chMod[dev] = ModuleMode;
		        curnode->y1 = orgy[dev];
/*
		        curnode->y2 = orgy[dev] + pulseHeight / 3;
*/
		        curnode->y3 = orgy[dev];
		        set_chan_height(curnode, powerCh[dev]);
			curnode->power = powerCh[dev];
		    	comnode->catt = coarseAttn[dev];
		    	comnode->fatt = fineAttn[dev];
		        status_node[dev] = curnode;
		    }
		    else
		    {
                        chMod[dev] = 0;
                        chyc[dev] = orgy[dev];
           	        curnode->y1 = orgy[dev];
           	        curnode->y2 = orgy[dev];
			curnode->power = 0.0;
		        status_node[dev] = NULL;
		    }
                }
	        break;
	case RVOP:
		rt_operation(curnode);
		break;
	case TBLOP: /* AP table operation */
		ap_operation(curnode);
                break;
	case TSOP:  /* AP and integer operation */
		apv_operation(curnode);
                break;
	case SETVAL:  /* initval  */
		if (ct == 0)
		    set_RTval(val[0], (int)fval[0]);
		break;
	case IFZERO:
		curnode = chk_if_stmt(curnode, 1);
		break;		
	case ELSENZ: 
		curnode = chk_if_stmt(curnode, 2);
		break;		
	case IFRT:
		curnode = chk_if_stmt(curnode, 3);
		break;		
	case ENDIF: 
	case DUMMY: 
	case SETOBS:
		break;
	case SPON:
		curnode->flag = XMARK;
		if (rfchan[dev])
		{
		    link_disp_node(curnode);
		    curnode->y3 = chyc[dev];
		}
		break;
	case ACQUIRE:
	case JFID:
	case SAMPLE:
		curnode->phase = (double)((RTval[OPH] * 90) % 360);
		comnode->flabel[7] = fid_label;
		fval[7] = (double)RTval[OPH];
                if (link_acquire_node(curnode) != 0) {
		    link_disp_node(curnode);
		    acquireCnt++;
/** don't turn off transmitter 
		    chyc[dev] = orgy[dev];
		    chMod[dev] = 0;
**/
                   /**
		    curnode->y1 = orgy[dev];
		    curnode->y2 = orgy[dev];
                   ***/
		    if (jfout)
		       fprintf(jfout, "0  %d\n",RTval[OPH] % 4);
		    exFlag = ASTNODE | ACHNODE | APGNODE;
		}
		break;
	case AMPBLK:
		curnode->flag = XMARK;
		if (rfchan[dev])
		{
		    link_disp_node(curnode);
/**
		    if ( !val[0])
		        ampOn[dev] = AMPBLANK;
		    else
		        ampOn[dev] = 0;
**/
		    curnode->y3 = chyc[dev];
		}
		break;
	case DEVON:
	case VDEVON:
	case XDEVON:
		if (rfchan[dev])
		{
		    link_disp_node(curnode);
		    curnode->phase = rfPhase[dev];
		    if (val[0] > 0)  /* it is on */
		    {
		    	curnode->power = powerCh[dev];
			chMod[dev] |= DevOnMode;
                        if (curnode->type == VDEVON)
                        {
                            halfHeight = 1;
		    	    curnode->power = powerMax[dev] / 2.0;
                        }
		        set_chan_height(curnode, curnode->power);
		    	comnode->catt = coarseAttn[dev];
		    	comnode->fatt = fineAttn[dev];
			chOn_node[dev] = curnode;
			status_node[dev] = NULL;
		        exFlag = APGNODE | RSTNODE;
		    }
		    else
		    {
		    	curnode->power = 0.0;
			chMod[dev] = chMod[dev] & (~DevOnMode);
			curnode->y2 = orgy[dev];
		        chOn_node[dev] = NULL;
		    }
		    curnode->y3 = curnode->y2;
		    chyc[dev] = curnode->y2;
		}
		break;
	case RFONOFF:
		curnode->flag = XMARK;
		if (rfchan[dev])
		{
		    link_disp_node(curnode);
		    curnode->phase = rfPhase[dev];
		    comnode->catt = coarseAttn[dev];
		    comnode->fatt = fineAttn[dev];
		    if (val[0] > 0)  /* it is on */
		    	curnode->power = powerCh[dev];
		    else
		    	curnode->power = 0.0;
		    comnode->flabel[4] = NULL;
		    curnode->y3 = chyc[dev];
		}
		break;
	 case SHINCGRAD:
	 	if (dev)
		{
		    shGrad = 1;
			/*  a1 * x1 */
		    k = fval[2] * get_RTval(val[2]);
			/*  a2 * x2 */
		    k2 = fval[3] * get_RTval(val[3]);
			/*  a3 * x3 */
		    k3 = fval[4] * get_RTval(val[4]);
		    curnode->power = fval[1] + k + k2 + k3;
		    if (jfout)
		      fprintf(jfout, "%d  %d %d %d\n", curnode->line, k, k2, k3);
		}
		break;

	case SHVGRAD:
		if (dev)
		{
		    shGrad = 1;
		    val[0] = get_RTval(val[2]); /* loop */
		    k = get_RTval(val[3]); /* mult */
		    curnode->power = fval[1] + fval[2] * k;
		    if (jfout)
		      fprintf(jfout, "%d  %d  %d\n", curnode->line, val[0], k);
		}
		break;
	case PESHGR:
	case DPESHGR:
	case DPESHGR2:
		val[5] = get_RTval(val[2]); /* mult */
		if (jfout)
		  fprintf(jfout, "%d  %d\n", curnode->line, val[5]);
                curnode->power = fval[1] + fval[2] * val[5];
		if (dev) {
                    if (curnode->type != DPESHGR2)
		        shGrad = 1;
                    k = dev - GRADX;
                    if (fabs(curnode->power) > gradMax[k]) {
                        gradMax[k] = fabs(curnode->power);
                        if (gradMax[k] > gradMaxLevel)
                            gradMaxLevel = gradMax[dev];
                    }
                }
		break;
	case SH2DVGR:
		if (dev)
		    shGrad = 1;
		break;
	case PEOBLVG:
		if (dev)
		{
		    shGrad = 1;
		    val[3] = get_RTval(val[1]); /* mult */
		    if (jfout)
		      fprintf(jfout, "%d  %d\n", curnode->line, val[3]);
		}
		break;
	case SHGRAD:
	case OBLSHGR:
	case PEOBLG:
		if (dev)
		    shGrad = 2;
		break;
	case ZGRAD:
		if (dev) {
		    shGrad = 3;
                    rval = fmodf((double)ct, 2.0);
                    if (rval != 0.0)
                        curnode->power = fval[1] * fval[2];
                    else
                        curnode->power = fval[1];
                    curnode->dockAt = PDOCK_A5;
                }
		break;
	case RGRAD:
		if (dev)
		{
                     rval = fmodf((double)ct, 2.0);
                     if (rval != 0.0)
                         curnode->power = fval[0] * fval[3];
                     else
                         curnode->power = fval[0];
		     link_disp_node(curnode);
		     set_grad_height(curnode, curnode->power);
		     chMod[dev] = 0;
		     curnode->y3 = curnode->y2;
		}
		break;
	case MGPUL:
	case MGRAD:
	case GRAD:
		if (dev)
		{
		     link_disp_node(curnode);
		     set_grad_height(curnode, curnode->power);
		     chMod[dev] = 0;
		     curnode->y3 = curnode->y2;
		}
		break;
	case PEGRAD:
		if (dev)
		{
		     link_disp_node(curnode);
		     chMod[dev] = 0;
		     val[1] = get_RTval(val[0]);  /* mult */
		     if (jfout)
		       fprintf(jfout, "%d  %d\n", curnode->line, val[1]);
		     if (val[2] == 0)  /* step is 0 */
		     {
		        rval = curnode->power;
			if (rval == 0.0 && !(curnode->flag & NINFO))
				rval = 0.1;
		        set_grad_height(curnode, rval);
		     }
		     else {
		        curnode->power = fval[0] + fval[1] * val[1];
		     	curnode->y2 = orgy[dev] + gradHeight / 2;
                     }
		     chyc[dev] = orgy[dev];
/**
		     curnode->y1 = curnode->y2;
**/
		     curnode->y3 = orgy[dev];
		}
		break;
	case OBLGRAD:
		if (dev)
		{
		     link_disp_node(curnode);
		     rval = curnode->power;
		     if (val[2])
		     {
			if (rval == 0.0)
			    rval = 0.1;
		     }
		     set_grad_height(curnode, rval);
		     chMod[dev] = 0;
		     chyc[dev] = orgy[dev];
/**
		     curnode->y1 = curnode->y2;
**/
		     curnode->y3 = orgy[dev];
		}
		break;
	case VGRAD:
		if (dev)
		{
		     link_disp_node(curnode);
		     k = get_RTval(val[2]);  /* mult */
		     if (jfout)
		       fprintf(jfout, "%d  %d\n", curnode->line, k);
		     curnode->power = val[0] + val[1] * k;
		     val[3] = k;
		     curnode->y2 = orgy[dev] + gradHeight / 2;
		     curnode->y1 = curnode->y2;
		     chMod[dev] = VgradMode;
		     chyc[dev] = orgy[dev];
		}
		break;
	case PRGMARK:
	case VPRGMARK:
		curnode->flag = XMARK;
		if (rfchan[dev])
		{
		     link_disp_node(curnode);
		     curnode->y3 = chyc[dev];
		     curnode->power = powerCh[dev];
		}
		break;
	case PRGON:
	case VPRGON:
	case PRGOFF:
	case XPRGON:
	case XPRGOFF:
		if (rfchan[dev])
		{
                     curnode->power = powerCh[dev];
                     if (curnode->type == VPRGON)
                     {
                         halfHeight = 1;
                         curnode->power = powerMax[dev] / 2.0;
                     }
		     if (curnode->type == PRGON || curnode->type == VPRGON || curnode->type == XPRGON)
		     {
		    	 curnode->flag = XBOX;
		         if (chMod[dev] & DevOnMode)
			 {
			    link_disp_node(curnode);
			    set_chan_height(curnode, curnode->power);
			 }
			 prg_node[dev] = curnode;
		         chMod[dev] |= PrgMode;
		         curnode->y3 = orgy[dev];
		         exFlag = RSTNODE;
		     }
		     else
		     {
			 link_disp_node(curnode);
		         chMod[dev] &= ~PrgMode;
			 prg_node[dev] = NULL;
		         curnode->y3 = chyc[dev];
		     }
		     curnode->power = powerCh[dev];
		     comnode->catt = coarseAttn[dev];
		     comnode->fatt = fineAttn[dev];
		} 
		break;
	case DECLVL:
		curnode->flag = XMARK;
		if(rfchan[dev])
		{
		   link_disp_node(curnode);
		   switch (rfType[dev]) {
		    case GEMINIB:
		    case GEMINIBH:
		       		if (val[0])  /* on */
				   powerCh[dev] = pplvl;
		       		else
			  	   powerCh[dev] = dhp;
				break;
		    case GEMINI:
		    case GEMINIH:
				if (val[0])
				{
			    	   if (homodecp)  /* homo decoupler */
					powerCh[dev] = dlp;
			    	   else
					powerCh[dev] = dhp;
				}
				else
			    	   powerCh[dev] = 0.0;
				break;
		    case VXRC:
				if (val[0])
				{
			    	   powerCh[dev] = dhp;
			    	   if (homodecp)  /* homo decoupler */
			    	   {
					if (dhp < 0.0)
			            	   powerCh[dev] = dlp;
			    	   }
				}
				else
			    	   powerCh[dev] = 0.0;
				break;
		     default:
				if (val[0])
			    	   powerCh[dev] = powerMax[dev];
				else
			    	   powerCh[dev] = 0.0;
				break;
		   }
		   curnode->power = powerCh[dev];
		   curnode->y3 = chyc[dev];
		   exFlag = APGNODE | ACHNODE | ASTNODE;
		}
		break;
	case RLPWR:
	case DECPWR:
	case XRLPWR:
		curnode->flag = XMARK;
		if (rfchan[dev])
		{
/**
		   if (dev == DODEV && dhp2)
		   {
			curnode->power = powerCh[dev];
		        curnode->y3 = chyc[dev];
			link_disp_node(curnode);
		        break;
		   }
**/
		   coarseAttn[dev] = fval[0];
		   set_chan_power(curnode, do_link);
/*
		   comnode->catt = coarseAttn[dev];
		   comnode->fatt = fineAttn[dev];
*/
		   exFlag = APGNODE | ACHNODE | ASTNODE;
		}
		break;
	case VDELAY:
	case VDLST:
		link_disp_node(curnode);
		val[2] = get_RTval(val[0]);
		if (jfout)
		     fprintf(jfout, "%d  %d\n", curnode->line, val[2]);
                curnode->dockAt = PDOCK_A5;
		fval[0] = 0.0;
	        if (curnode->type == VDLST)
		{
		     comnode->flabel[2] = NULL;
		     anode = search_array_node(DARRAY, val[1]);
		     if (anode != NULL)
		     {
		        if (val[2] < anode->vars)
			      fval[0] = anode->value.Val[val[2]];
			comnode->flabel[2] = anode->name;
		     }
		}
		else
		{
		   rval = (double)val[2];
		   switch(val[1]) {
		     case NSEC:
                           if (isNvpsg) {
                              if (rval < 4.0)
                                  rval = 4.0;
			      fval[0] = rval * 12.5 * 1.0e-9;
                           }
                           else {
			      if (newAcq)
				  fval[0] = (100 + rval * 12.5) * 1.0e-9;
			      else
				  fval[0] = (200 + (rval - 2) * 25) * 1.0e-9;
                           }
			   break;
		     case USEC:
			   fval[0] = rval * 1.0e-6;
			   break;
		     case MSEC:
			   fval[0] = rval * 1.0e-3;
			   break;
		     case SECD:
			   fval[0] = rval;
			   break;
		   }
		}
		if (fval[0] > 0.0)
		    curnode->rtime = fval[0];
		if (curnode->rtime > dMax)
		    dMax = curnode->rtime;
		if (curnode->rtime > 0.0)
		    curnode->rtime2 = curnode->rtime;
		else
		    curnode->rtime2 = minTime;
		curnode->ptime = fval[0];
		delayCnt++;
		curnode->y3 = curnode->y2;
		curnode->phase = rfPhase[dev];
		curnode->power = powerCh[dev];
		comnode->flabel[0] = comnode->vlabel[0];
		break;

	case SPINLK:
		if (rfchan[dev])
		{
		   link_disp_node(curnode);
		   val[3] = get_RTval(val[1]);  /* phase */
		   if (jfout)
		     fprintf(jfout, "%d  %d \n", curnode->line, val[3] % 4);
		   curnode->power = powerCh[dev];
		   set_chan_height(curnode, curnode->power);
		   curnode->y1 = orgy[dev];
		   chyc[dev] = orgy[dev];
                   comnode->catt = coarseAttn[dev];
                   comnode->fatt = fineAttn[dev];
		}
		break;
	case XGATE:
	case BGSHIM:
	case SHSELECT:
	case GRADANG:
        case ROTATEANGLE:
	case EXECANGLE:
		if (rfchan[dev])
		   link_disp_node(curnode);
		curnode->flag = XWDH | XBOX;
		curnode->y2 = chyc[dev] + pfy / 2;
		curnode->y3 = curnode->y2 - pfy;
		curnode->rtime2 = minTime2; 
		break;
	case SLOOP:
	case HLOOP:
		link_disp_node(curnode);
                curnode->dockAt = PDOCK_A3;
		val[3] = get_RTval(val[0]);
		if (jfout)
		     fprintf(jfout, "%d  %d\n", curnode->line, val[3]);
		if (curnode->type == SLOOP)
		   set_RTval(val[1], val[3]);
		comnode->vlabel[3] = comnode->vlabel[0];
		curnode->y3 = chyc[dev];
		curnode->y2 = orgy[dev] + pulseHeight + 1;
		curnode->rtime2 = minTime2;
		curnode->flag = curnode->flag | XLOOP;
		if (jfout)
		     fprintf(jfout, "%d  %d\n", curnode->line, val[3]);
		break;
	case GLOOP:
		link_disp_node(curnode);
                curnode->dockAt = PDOCK_A3;
		curnode->y3 = chyc[dev];
		curnode->y2 = orgy[dev] + pulseHeight + 1;
		curnode->rtime2 = minTime2;
		break;
	case ENDSP:
	case ENDHP:
		link_disp_node(curnode);
                curnode->dockAt = PDOCK_B3;
		curnode->y3 = chyc[dev];
		curnode->y2 = orgy[dev] + pulseHeight + 1;
		curnode->rtime2 = minTime2;
		curnode->flag = curnode->flag | XLOOP;
		break;
	case ROTORP:
	case ROTORS:
		link_disp_node(curnode);
		curnode->flag = XWDH | XBOX;
		curnode->y2 = chyc[dev] + pfy / 2;
		curnode->y3 = curnode->y2 - pfy;
		val[1] = get_RTval(val[0]);
		if (jfout)
		     fprintf(jfout, "%d  %d\n", curnode->line, val[1]);
		if (curnode->type == ROTORS)
		{
		    curnode->rtime = val[1] * 1.0e-7; /* 100 nanoseconds */
		    if (curnode->rtime > 0.0)
			curnode->rtime2 = curnode->rtime;
		    else
			curnode->rtime2 = minTime2;
		    curnode->ptime = curnode->rtime;
		}
		if (jfout)
		     fprintf(jfout, "%d  %d\n", curnode->line, val[1]);
		break;
	case SETRCV:
		curnode->device = 1;
		val[1] = get_APval(val[0], ct);
		if (jfout)
		     fprintf(jfout, "%d  %d\n", curnode->line, val[1] % 4);
		RTval[OPH] = val[1];
		fid_label = comnode->vlabel[0];
		curnode->flag = XMARK;
		curnode->y1 = chyc[1];
		curnode->y2 = chyc[1];
		curnode->y3 = chyc[1];
		if (rfchan[1])
		    link_disp_node(curnode);
		break;
	case GETELEM:
		get_APelem(curnode);
		break;
	case INITDLY:
		if (ct == 0)
		{
		    if (val[0] < RTDELAY)
		    	RTdelay[val[0]] = fval[0];
                    curnode->dockAt = PDOCK_A5;
		}
		break;
	case INCDLY:
		link_disp_node(curnode);
/**
		curnode->flag = XWDH;
**/
                curnode->dockAt = PDOCK_A5;
		curnode->y3 = chyc[dev];
		val[2] =  get_RTval(val[0]);
		if (jfout)
		     fprintf(jfout, "%d  %d\n", curnode->line, val[2]);
		if (val[1] < RTDELAY)
		    fval[3] = RTdelay[val[1]];
		else
		    fval[3] = 0.0;
		fval[0] = val[2] * fval[3];
		if (fval[0] > 0.0)
		     curnode->rtime = fval[0];
		if (curnode->rtime > 0.0)
		     curnode->rtime2 = curnode->rtime;
		else
		     curnode->rtime2 = minTime;
		curnode->ptime = fval[0];
		delayCnt++;
		if (curnode->rtime > dMax)
		    dMax = curnode->rtime;
		curnode->phase = rfPhase[dev];
		curnode->power = powerCh[dev];
		comnode->flabel[0] = comnode->vlabel[0];
		break;
	case PHASE:
	   	curnode->flag = XMARK;
		if (rfchan[dev])
		{
		    link_disp_node(curnode);
		    val[1] =  get_RTval(val[0]);
		    if (jfout)
		       fprintf(jfout, "%d  %d\n", curnode->line, val[1] % 4);
		    if (newAcq == 2)
                    {
                       rval = exPhase[dev] + (double)val[1]/10.0;
                    }
                    else
                       rval = exPhase[dev] + 90.0 * val[1];
		    /* rfPhase[dev] = fmod(rval, 360.0); */
		    rfPhase[dev] = rval;
		    curnode->phase = rfPhase[dev];
		    fval[1] = rfPhase[dev];
		    curnode->power = powerCh[dev];
		    curnode->y3 = chyc[dev];
		}
		break;
	case POWER:
	case PWRF:
	case PWRM:
		curnode->flag = XMARK;
		if (rfchan[dev])
		{
		   val[1] = get_RTval(val[0]);
		   if (jfout)
		       fprintf(jfout, "%d  %d\n", curnode->line, val[1]);
		   fval[0] = (double) val[1];
		   comnode->flabel[0] = comnode->vlabel[0];
		   if (curnode->type == POWER)
		   	coarseAttn[dev] = fval[0];
		   else
		   	fineAttn[dev] = fval[0];
		   set_chan_power(curnode, do_link);
/*
		   comnode->catt = coarseAttn[dev];
		   comnode->fatt = fineAttn[dev];
*/
		   exFlag = APGNODE | ACHNODE | ASTNODE;
		}
		break;
	case VRLPWRF:
	case RLPWRF:
	case RLPWRM:
	case XRLPWRF:
	        if (curnode->type == VRLPWRF) {
		   val[1] = get_RTval(val[0]);
		   fval[0] = (double) val[1];
		   comnode->flabel[0] = comnode->vlabel[0];
                }
		curnode->flag = XMARK;
		if (rfchan[dev])
		{ 
		   fineAttn[dev] = fval[0];
		   set_chan_power(curnode, do_link);
/*
		   comnode->catt = coarseAttn[dev];
		   comnode->fatt = fineAttn[dev];
*/
		   exFlag = APGNODE | ACHNODE | ASTNODE;
		}
		break;
	case SPHASE: /* xmtrphase */
	case PSHIFT: /* phaseshift */
		curnode->flag = XMARK;
		if (rfchan[dev])
		{
		   link_disp_node(curnode);
		   val[1] = get_RTval(val[0]);
		   if (jfout)
		       fprintf(jfout, "%d  %d\n", curnode->line, val[1] % 4);
		   if (curnode->type == SPHASE)
		   	fval[0] = phaseStep[dev];
		   rval = fval[0] * (double) val[1];
		   /* rval = fmod(rval, 360.0); */
		   exPhase[dev] = rval;
		   rfPhase[dev] = rval;
		   fval[1] = rval;
		   curnode->y3 = curnode->y2;
		}
		break;
	case PHSTEP:
		if (rfchan[dev])
		   phaseStep[dev] = fval[0];
		break;		
	case VFREQ:  /* vfreq */
	case VOFFSET: 
		curnode->flag = XMARK;
		if (curnode->type == VFREQ)
		     k = FARRAY;
		else
		     k = OARRAY;
		anode = search_array_node(k, val[0]);
	        if (anode != NULL)
		{
		     val[2] = get_RTval(val[1]);
		     if (jfout)
		       fprintf(jfout, "%d  %d\n", curnode->line, val[2]);
		     if (val[2] < anode->vars)
			fval[0] = anode->value.Val[val[2]];
		     else
			fval[0] = 0.0;
		     comnode->flabel[2] = anode->name;
		     dev = anode->dev;
		     curnode->y1 = chyc[dev];
		     curnode->y2 = chyc[dev];
		     curnode->y3 = chyc[dev];
		     
		     if (rfchan[dev])
			link_disp_node(curnode);
		}
		break;		
	case LOFFSET:
		comnode->val[2] = get_RTval(comnode->val[1]);
		if (jfout)
		    fprintf(jfout, "%d  %d\n", curnode->line, comnode->val[2]);
		if (rfchan[dev])
		   link_disp_node(curnode);
		curnode->flag = XMARK;
		curnode->y3 = chyc[dev];
		break;
	case LKHLD: 
	case LKSMP: 
	case OFFSET:
	case POFFSET:
	case RCVRON:
	case HDSHIMINIT:
	case ROTATE:
        case TRIGGER: /* triggerSelect */
		if (rfchan[dev])
		   link_disp_node(curnode);
		curnode->flag = XMARK;
		curnode->y3 = chyc[dev];
		break;
	case ACQ1: /* startacq */
		if (rfchan[dev])
		   link_disp_node(curnode);
		curnode->flag = XMARK | XWDH;
		curnode->y3 = chyc[dev];
		curnode->ptime = fval[0];
		break;
	case APSHPUL:
		exist = 0;
		dpnode = curnode;
		if (jfout)
		       fprintf(jfout, "%d ", curnode->line);
		while (dpnode != NULL)
		{
		    comnode = (COMMON_NODE *) &dpnode->node.common_node;
		    k = dpnode->device;
		    if (rfchan[k])
		 	exist = 1;
		    comnode->val[3] = get_RTval(comnode->val[0]); /* phase */
				      /* power table */
		    comnode->val[4] = get_RTval(comnode->val[1]);
				      /* phase table */
		    comnode->val[5] = get_RTval(comnode->val[2]); 
		    if (jfout) {
		        fprintf(jfout, " %d %d %d", comnode->val[3] % 4, 
				comnode->val[4], comnode->val[5]);
		    }
		    dpnode->power = powerCh[k];
		    exPhase[k] = 0;
		    if (rfchan[k])
		    	set_chan_height(dpnode, curnode->power);
		    dpnode->y1 = orgy[k];
		    dpnode->y3 = orgy[k];
		    chyc[k] = orgy[k];
		    chMod[k] = 0;
		    dpnode->mode = ampOn[k];
		    dpnode = dpnode->bnode;
		}
		if (jfout)
		       fprintf(jfout, "\n");
		if (exist)
		{
			link_disp_node(curnode);
			pulseCnt = pulseCnt + 2;
			if (curnode->rtime > pMax)
		            pMax = curnode->rtime;
		}
		break;
	case FIDNODE:
		if (acquireCnt == 0)
		{
                    val[1] = RTval[OPH];
                    curnode->active = 1;
                    curnode->phase = (RTval[OPH] * 90 ) % 360;
                    comnode->flabel[1] = fid_label;
                    link_acquire_node(curnode);
                    link_disp_node(curnode);
		    if (jfout)
		          fprintf(jfout, "0  %d\n",RTval[OPH] % 4);
		}
		else
		    curnode->active = 0;
		break;
	case ENDSISLP:
		if (*comnode->vlabel[0] == 'c')
			link_disp_node(curnode);
		curnode->rtime2 = minTime2;
		curnode->y2 = orgy[dev] + pulseHeight + 1;
		break;
	case MSLOOP:
	case PELOOP:
	case PELOOP2:
		if (*comnode->vlabel[0] == 'c')
		{
		    if (fval[0] > 0.5)
			k = (int)fval[0];
		    else
			k = 1;
		    set_RTval(val[1], k);
		    val[3] = k;
		    comnode->vlabel[3] = comnode->vlabel[1];
		    link_disp_node(curnode);
		}
		else if (*comnode->vlabel[0] == 's')
		{
		     if (curnode->type == MSLOOP && fval[0] < 1)
		     {
			set_RTval(val[1], (int)fval[0]);
			set_RTval(val[2], 0);
		     }
		     else if (curnode->type == PELOOP || curnode->type == PELOOP2)
		     {
			if (fval[0] > 0.5)
			     set_RTval(val[1], (int) fval[0]);
			else
			{
			     set_RTval(val[1], 0);
			     set_RTval(val[2], 0);
			}
		     }
		}
		curnode->rtime2 = minTime2;
		curnode->y2 = orgy[dev] + pulseHeight + 1;
                curnode->dockAt = PDOCK_A3;
		curnode->flag = curnode->flag | XLOOP;
		break;
	case INCGRAD:
		if (dev)
		{
		    incgnode = (INCGRAD_NODE *) &curnode->node.incgrad_node;
		    link_disp_node(curnode);
		    val = incgnode->val;
		    fval = incgnode->fval;
		    val[7] = val[0] + val[1] * get_RTval(val[4]);
		    val[8] = val[2] * get_RTval(val[5]);
		    val[9] = val[3] * get_RTval(val[6]);
		    curnode->power = (double)(val[7] + val[8] + val[9]);
		    set_grad_height(curnode, curnode->power);
		    curnode->y3 = curnode->y2;
		    if (jfout)
		       fprintf(jfout, "%d %d %d %d\n", curnode->line,
			    val[7], val[8], val[9]);
		}
		break;
	case APOVR:
	case APTABLE:
	case INITSCAN:
	case VSCAN:
		curnode->flag = 0;
		break;
	case PARALLEL:
		link_disp_node(curnode);
		break;
	case DCPLON:   /* decoupleron */
		if (rfchan[dev])
                {
		    exnode = (EX_NODE *) &curnode->node.ex_node;
                    curnode->flag = XWDH | XHGHT | XBOX;
                    link_disp_node(curnode);
/*
		    chMod[dev] = ModuleMode;
*/
                    curnode->y1 = orgy[dev];
		    curnode->y3 = orgy[dev];
                    curnode->power = exnode->fval[0];
                    set_chan_height(curnode, curnode->power);
                    status_node[dev] = curnode;
		}
		break;
	case DCPLOFF:  /* decoupleroff */
		if (rfchan[dev])
                {
                    link_disp_node(curnode);
		    chMod[dev] = 0;
                    curnode->y1 = orgy[dev];
		    curnode->y3 = orgy[dev];
                    chyc[dev] = orgy[dev];
                    status_node[dev] = NULL;
		}
		break;
	case XTUNE:
        case XMACQ:
		if (rfchan[dev])
		{
		    link_disp_node(curnode);
		    curnode->y1 = orgy[dev];
		    curnode->y2 = orgy[dev];
		    exFlag = ASTNODE | ACHNODE | APGNODE;
		    if (curnode->type == XMACQ) {
	     		curnode->phase = val[0];
		        acquireCnt++;
		    }
		}
		break;
        case RDBYTE:  /*  readMRIUserByte */
		val[1] = get_RTval(val[0]);
		curnode->flag = XMARK;
		curnode->ptime = fval[0];
		link_disp_node(curnode);
		break;
        case WRBYTE:  /*  writeMRIUserByte */
        case SETGATE: /*  setMRIUserGates */
		val[1] = get_RTval(val[0]);
		curnode->flag = XMARK;
		link_disp_node(curnode);
		break;
	case SETANGLE:
		val[5] = get_RTval(val[3]);
	   	curnode->flag = XMARK;
		link_disp_node(curnode);
                angle_node = curnode;
		break;
	case ACTIVERCVR:
                curnode->flag = XMARK;
                link_disp_node(curnode);
                curnode->y3 = chyc[TODEV];
	        curnode->device = TODEV;
                if (comnode->pattern != NULL)
                   strncpy(rcvrsStr, comnode->pattern, RFCHAN_NUM);
		break;
	case PARALLELSTART:  // simulate
                if (inParallel < 1) {
                    dpnode = (SRC_NODE *) allocateTmpNode(sizeof(SRC_NODE));
                    if (dpnode == NULL) {
                        Werrprintf("dps error: memory allocation failed.\n");
                        return;
                    }
                    inParallel = 1;
                       dpnode->stime = t_time;
                       dpnode->etime = t_time;
                       dpnode->device = 0;
                       dpnode->type = XDOCK;
                       dpnode->flag = XMARK;
                       dpnode->fname = parallelStart;
                       dpnode->visible = 1;
                       dpnode->y2 = orgy[0] - 4;
                       dpnode->y3 = statusY + 6;
                       dpnode->dockAt = PDOCK_A1;
                       link_disp_node(dpnode);
                       parallel_start_node = dpnode;
                       p_time = t_time;
                }
                else {
                       t_time = p_time;
                       curnode->stime = t_time;
                }
                parallelChan = dev;
                curnode->y3 = orgy[dev];
                curnode->y2 = orgy[dev] + pulseHeight;
                curnode->dockAt = PDOCK_A2;
                link_disp_node(curnode);
		break;
	case PARALLELEND:
                t_time = p_time;
                curnode->stime = p_time;
                    curnode->rtime = 0.0;
                    curnode->dockAt = PDOCK_LAST;
                    link_disp_node(curnode);
                    parallel_start_node = NULL;
                    curnode->y3 = orgy[TODEV];
                    curnode->y2 = orgy[TODEV] + pulseHeight;
                    link_disp_node(curnode);
                parallelChan = 0;
                inParallel = 0;
		break;
	case PARALLELSYNC:
                curnode->flag = XMARK;
                if (dev > 0) {
                    curnode->rtime = 0.0;
                    link_disp_node(curnode);
                    curnode->y3 = chyc[dev];
                }
		break;
        case RLLOOP:
                // val[1] is a real time variable of the # of times to cycle
                // val[2] is a real time variable that will be the loop counter
                set_RTval(comnode->val[1], comnode->val[0]);
                set_RTval(comnode->val[2], 1);
                if (inLoop < MAX_RLLOOP)
                {
                   inLoop++;
                   rlloopTime[inLoop] = 0;
                   rlloopMult[inLoop] = comnode->val[0] -1;
                }
                link_disp_node(curnode);
		curnode->y2 = orgy[dev] + pulseHeight + 1;
                curnode->y3 = chyc[dev];
                curnode->dockAt = PDOCK_A3;
                curnode->rtime2 = minTime2;
		curnode->flag = curnode->flag | XLOOP;
                break;
        case RLLOOPEND:
                if (inLoop)
                {
                   curnode->rtime = rlloopTime[inLoop] * rlloopMult[inLoop];
                   inLoop--;
                }
                link_disp_node(curnode);
		curnode->y2 = orgy[dev] + pulseHeight + 1;
                curnode->y3 = chyc[dev];
                curnode->rtime2 = minTime2;
                curnode->dockAt = PDOCK_B3;
		curnode->flag = curnode->flag | XLOOP;
                break;
        case KZLOOP:
                if (dev > 0) {
                    link_disp_node(curnode);
		    val[3] = get_RTval(val[0]);
                    set_RTval(val[1], 1);
		    curnode->y2 = orgy[dev] + pulseHeight + 1;
                    curnode->y3 = chyc[dev];
                    curnode->dockAt = PDOCK_A3;
	            kz_node[dev] = curnode;
                    curnode->rtime2 = minTime2;
		    curnode->flag = curnode->flag | XLOOP;
                }
                break;
        case KZLOOPEND:
                if (dev > 0) {
                    t_time = adjust_kz_loop(curnode);
		    curnode->y2 = orgy[dev] + pulseHeight + 1;
                    curnode->y3 = chyc[dev];
                    curnode->dockAt = PDOCK_B3;
                    link_disp_node(curnode);
	            kz_node[dev] = NULL;
                    curnode->rtime2 = minTime2;
		    curnode->flag = curnode->flag | XLOOP;
                }
                break;
        case NWLOOP:
                link_disp_node(curnode);
                set_RTval(val[0], (int)fval[0]);
                set_RTval(val[1], 0);
		val[3] = (int)fval[0];
                curnode->rtime2 = minTime2;
                curnode->y2 = orgy[dev] + pulseHeight + 1;
                curnode->dockAt = PDOCK_A3;
                curnode->flag = curnode->flag | XLOOP;
                break;
        case NWLOOPEND:
                link_disp_node(curnode);
                curnode->rtime2 = minTime2;
                curnode->y2 = orgy[dev] + pulseHeight + 1;
                curnode->dockAt = PDOCK_A3;
                curnode->flag = curnode->flag | XLOOP;
                break;
	default:
		if (debug > 1)
		    Wprintf("dps error:  unknown type: %d\n", curnode->type);
		break;
	}

        if (isDispObj) {
            t_time += curnode->rtime2;
            curnode->etime = t_time;
        }
	if (shGrad > 0)
	{
	    link_disp_node(curnode);
	    delayCnt++;
	    if (curnode->rtime > dMax)
		dMax = curnode->rtime;
	    if (shGrad > 1)
	    {
		set_grad_height(curnode, curnode->power);
		if (shGrad != 3)
		     curnode->y1 = orgy[dev];
		else   /* ZGRAD */
		     curnode->y3 = orgy[dev];
	    }
	    else  /* vgradient ... */
	    {
		curnode->y2 = orgy[dev] + gradHeight * 3 / 4;
	    }
	    if (curnode->flag & RAMPUP)
	    {
		curnode->y3 = curnode->y2;
		curnode->y1 = orgy[dev];
	    }
	    if (curnode->flag & RAMPDN)
	    {
		curnode->y1 = chyc[dev];
		curnode->y3 = orgy[dev];
	    }
	    chyc[dev] = curnode->y3;
	    chMod[dev] = 0;
	}
	if (exFlag)
	{
	    dpnode = curnode;
	    while (dpnode != NULL)
	    {
		k = dpnode->device;
                if (halfHeight)
                   curPower = powerMax[k] / 2.0;
                else
                   curPower = powerCh[k];
	        if (rfchan[k] == 0)
	        {
		   dpnode = dpnode->bnode;
		   continue;
	        }
		if (dpnode->ptime < minPw)
		{
		   if (exFlag & ASTNODE && status_node[k] != NULL)
		   {
			spnode = new_sparenode(spnode);
			if (spnode != NULL)
			{
		   	   copy_src_node(status_node[k], spnode, 1);
		    	   set_chan_height(spnode, curPower);
		   	   link_disp_node(spnode);
		   	   comnode = (COMMON_NODE *) &spnode->node.common_node;
	    		   spnode->power = powerCh[k];
	    		   spnode->phase = rfPhase[k];
		   	   comnode->catt = coarseAttn[k];
		   	   comnode->fatt = fineAttn[k];
			   chyc[k] = spnode->y2;
			}
		   }
		   if (exFlag & ACHNODE && chOn_node[k] != NULL)
		   {
			spnode = new_sparenode(spnode);
			if (spnode != NULL)
			{
		   	   copy_src_node(chOn_node[k], spnode, 1);
			   spnode->y1 = chyc[k];
		    	   set_chan_height(spnode, curPower);
			   spnode->y3 = chyc[k];
		   	   link_disp_node(spnode);
		   	   comnode = (COMMON_NODE *) &spnode->node.common_node;
	    		   spnode->power = powerCh[k];
	    		   spnode->phase = rfPhase[k];
		   	   comnode->catt = coarseAttn[k];
		   	   comnode->fatt = fineAttn[k];
			}
		   }
		   if (exFlag & APGNODE &&(chMod[k] & DevOnMode) &&
				 prg_node[k] != NULL)
		   {
			spnode = new_sparenode(spnode);
			if (spnode != NULL)
			{
		   	   copy_src_node(prg_node[k], spnode, 1);
			   spnode->y1 = orgy[k];
		    	   set_chan_height(spnode, curPower);
			   spnode->y3 = orgy[k];
		   	   link_disp_node(spnode);
		   	   comnode = (COMMON_NODE *) &spnode->node.common_node;
	    		   spnode->power = powerCh[k];
	    		   spnode->phase = rfPhase[k];
		   	   comnode->catt = coarseAttn[k];
		   	   comnode->fatt = fineAttn[k];
			}
		   }
		}
		if (dpnode->ptime >= minPw)
		{
		   if (exFlag & RSTNODE)
			status_node[k] = NULL;
		   if (exFlag & RCHNODE)
		   {
			chOn_node[k] = NULL;
		        dpnode->mode = ampOn[k]; 
		        chMod[k] = 0;
		   }
		   if (exFlag & RPGNODE)
			prg_node[k] = NULL;
		}
		dpnode = dpnode->bnode;
	    }
	}
	curnode->mode = chMod[dev] | ampOn[dev];
   	curnode = curnode->next;
   }
   if (dMax <= 0.0)
	dMax = 1.0;
   if (pMax <= 0.0)
	pMax = 1.0;
   ap_op_ok = 0;
#ifdef SUN
   if ( do_link && !dpsPlot)
	disp_dps_time();
   if (hasDock)
	sort_dock_node();
#endif 
}


static int
execute_ps(trans)
int	trans;
{
	int   ct;

	if (trans < 1)
	     trans = 1;
	ct = RTval[CT];
	if (debug > 0)
	    fprintf(stderr, " execute_ps: transient= %d  ct= %d\n", trans, ct);
	if (trans == ct)
	     return(0);
	if (trans < ct)
	{
	     init_rt_values();
	     ct = 0;
	}
	while (ct < trans - 1)
	{
	     simulate_psg(ct, 0);
	     ct++;
	}
	simulate_psg(ct, 1);
	RTval[CT] = trans;
	RTval[CTSS] = trans;
   	if (first_round)
      	    RTval[IX] = ct;
	return(1);
}


static void
expand_sim_width(pnode, dev, x)
SRC_NODE  *pnode;
int	  dev, x;
{
   SRC_NODE    *cnode;

   cnode = pnode->dnext;
   while (cnode != NULL)
   {
	if (cnode->type == PARALLEL)
	    return;
	if (cnode->device == dev) {
	   switch (cnode->type) {
	     case DELAY:
	     case VDELAY:
	     case INCDLY:
	     case VDLST:
	     case PULSE:
	     case SHPUL:
	     case SHVPUL:
	     case APSHPUL:
	     case OFFSHP:
	     case SHACQ:
	     case SPACQ:
	     case GRPPULSE:
			cnode->x2 = x;
			cnode->x3 = x;
			return;
			break;
	   }
	}
	cnode = cnode->dnext;
   } /* while loop */
}


static int xEndStatus;
static int mx_a_base, mx_d_base, mx_p_base;
static int mx_d_xbase, mx_p_xbase;
static double  mx_x_ratio = 1.0;


static void clear_parallel_xs() 
{
     PARALLEL_XNODE *node;

     if (parallel_x_list == NULL)
        return;
     node = parallel_x_list;
     while (node != NULL) {
        node->x = 0;
        node->time = -1;
        node = node->next;
     }
}

static PARALLEL_XNODE *get_parallel_timenode(n) 
double n;
{
     PARALLEL_XNODE *xnode;

     if (parallel_x_list == NULL)
         return(NULL);
     xnode = parallel_x_list;
     while (xnode != NULL) {
        if (xnode->time == n)
            break;
        xnode = xnode->next;
     }
     return (xnode);
}

static void reset_parallel_x2s(node, doFlag) 
SRC_NODE *node;
int  doFlag;
{
     SRC_NODE *tnode;
     SRC_NODE *bnode;
     PARALLEL_XNODE *xnode;
     int       x;
     double    etime;

     if (parallel_start_node == NULL)
        return;
     etime = -1;
     x = 0;
     tnode = parallel_start_node;
     while (tnode != NULL) {
        if (etime != tnode->etime) {  // get new x
            xnode = get_parallel_timenode(tnode->etime);
            if (xnode == NULL)
               break;
            x = xnode->x;
            etime = xnode->time;
        }
	if ((tnode->flag & XMARK) == 0) {
            tnode->x2 = x;
            tnode->x3 = x;
        }
	if (tnode->type == XEND) {
            bnode = tnode->bnode;
            if (bnode != NULL) {
               bnode->x2 = x;
               bnode->x3 = x;
            }
        }
        tnode = tnode->dnext;
     }
     parallel_time = node->etime;
     parallel_start_node = node;
}

static int set_parallel_x2(enode, x, c_time, doFlag) 
SRC_NODE  *enode;
double    c_time;
int       x, doFlag;
{
     SRC_NODE  *node;
     PARALLEL_XNODE *xnode, *pnode, *new_node;
     int       retX, dx, dmin, k;
     double    rinc, d;

     retX = x;
     if (enode->type == XEND)
        node = enode->bnode;
     else
        node = enode;
     if (node == NULL)
        return retX;
     xnode = parallel_x_list;
     while (xnode != NULL) {
        if (xnode->time > node->etime || xnode->time == node->etime)
           break; 
        if (xnode->time < 0) {
           xnode->time = node->etime;
           xnode->x = x;
           break; 
        }
        if (xnode->next == NULL)
           break; 
        xnode = xnode->next;
     }
     if (xnode == NULL || xnode->time != node->etime) {
        new_node = (PARALLEL_XNODE *) allocateTmpNode(sizeof(PARALLEL_XNODE));
        if (new_node == NULL)
           return retX;
        new_node->time = node->etime;
        new_node->x = retX;
        if (parallel_x_list == NULL)
            parallel_x_list = new_node;
        else {
            if (xnode->time < node->etime) {
                xnode->next = new_node;
                new_node->previous = xnode;
            }
            else {
                pnode = xnode->previous;
                new_node->next = xnode;
                xnode->previous = new_node;
                new_node->previous = pnode;
                if (pnode != NULL)
                    pnode->next = new_node;
            }
        }
        xnode = new_node;
     }
     if (xnode->x < retX)
        xnode->x = retX;
     else
        retX = xnode->x;
     if (node->pflag == 0)
        return retX;
     
     dx = retX - node->x1;
     if (dx < 0)
         dx = 0;
     if (node->pflag & PLBOX) {
         dx = pfx + 2 - dx;
         if (dx > 0) {
             retX += dx;
             exPnts += dx;
         }
     }
     else if (node->pflag & PL_LOOP) {
         if (dx < loopW) {
             dx = loopW - dx;
             retX += dx;
             exPnts += dx;
         }
     }
     else if (node->pflag & PLACQ) {
         if (node->type == SHACQ)
             dmin = mx_a_base * 2;
         else
             dmin = mx_a_base;
         dx = dmin - dx;
         if (dx > 0) {
             if (dx <= mx_a_base)
                 acqAct++;
             else
                 acqAct+= 2;
             retX += dx;
             exPnts += dx;
         }
     }
     if (node->pflag & XDLY) {
         k = 0;
         if (c_time < node->etime) {
             if (retX > node->x1) {
                 if (xEndStatus || (retX - node->x1) < (mx_d_base / 3))
                    k = 2;
                 else
                    k = 1;
             }
             else
                 k = 2;
          }
          else if (c_time == node->etime) {
             if (retX <= node->x1)
                 k = 2;
          }

          xEndStatus = 0;
          if (k == 1) {
             delayXCnt = delayXCnt + 1.0;
             retX = retX + mx_d_xbase;
             xEndStatus = 3;
          }
          else if (k == 2) {
             rinc = node->rtime / dMax;
             if (rinc > 1.0)
                 rinc = 1.0;
             rinc = rinc * mx_x_ratio + 1.0;
             delayCnt = delayCnt + rinc;
             retX = retX + rinc * mx_d_base;
             xEndStatus = 4;
          }
     }
     else if (node->pflag & XPULS || node->pflag & XPULS2) {
          if (node->pflag & XPULS2)
              d = 2.0;
          else
              d = 1.0;
          k = 0;
          if (c_time < node->etime) {
              if (retX > node->x1)
                  k = 1;
              else
                  k = 2;
          }
          else if (c_time == node->etime) {
              if (retX <= node->x1)
                  k = 2;
          }
          xEndStatus = 1;
          if (k == 1) {
              pulseXCnt = pulseXCnt + d;
              retX = retX + d * mx_p_xbase;
          }
          if (k == 2) {
              rinc = node->rtime / pMax;
              if (rinc > 1.0)
                  rinc = 1.0;
              rinc = rinc * mx_x_ratio * d + 1.0;
              pulseCnt = pulseCnt + rinc;
              retX = retX + rinc * mx_p_base;
          }
     }
     xnode->x = retX;

     if (parallel_time != enode->etime)
         reset_parallel_x2s(enode, doFlag);
     return retX;
}

// set and save the left x position
static int set_parallel_x1(node, x, doFlag) 
SRC_NODE  *node;
int       x, doFlag;
{
     PARALLEL_XNODE *xnode, *pnode, *new_node;
     int retX;

     sync_node[node->device] = node; // save the node in parallel mode
                                     // every node was appended a
                                     // XEND node to adjust its x2 position
     retX = x;
     xnode = parallel_x_list;
     while (xnode != NULL) {
        if (xnode->time > node->stime)
           break; 
        if (xnode->time == node->stime) {
           // it is already set by other channel
            if (xnode->x > retX)
                retX = xnode->x;
            else
                xnode->x = retX;
           return retX;
        }
        if (xnode->time < 0) {
           xnode->time = node->stime;
           xnode->x = x;
           return retX;
        }
        if (xnode->next == NULL)
           break; 
        xnode = xnode->next;
     }
     new_node = (PARALLEL_XNODE *) allocateTmpNode(sizeof(PARALLEL_XNODE));
     if (new_node == NULL)
        return retX;
     new_node->time = node->stime;
     new_node->x = retX;
     if (parallel_x_list == NULL) {
        parallel_x_list = new_node;
        return retX;
     }
     if (xnode->time < node->stime) {
        xnode->next = new_node;
        new_node->previous = xnode;
        return retX;
     }
     pnode = xnode->previous;
     new_node->next = xnode;
     xnode->previous = new_node;
     new_node->previous = pnode;
     if (pnode != NULL)
         pnode->next = new_node;
     return retX;
}

static void
adjust_gradient_x(int allGrads, int flag, SRC_NODE *node)
{
   int	dev, vinc;
   double  r, dv, dt;
   SRC_NODE *grad;

   if (node->type == DPESHGR2)
       return;
   if (allGrads == 0) {
       dev = node->device - GRADX;
       if (dev >= GRAD_NUM || dev < 0 || shgrad_node[dev] == NULL)
           return;
       grad = shgrad_node[dev];
       if (grad->xetime >= node->xstime) {
           grad->x2 = node->x1;
           grad->x3 = node->x1;
       }
       shgrad_node[dev] = NULL;
       return;
   }
   for (dev = 0; dev < GRAD_NUM; dev++)
   {
       grad = shgrad_node[dev];
       if (grad == NULL)
           continue;
       dt = node->xstime - grad->xetime;
       dv = fabs(dt);
       if (dv < minTime) {
           grad->x2 = node->x1;
           grad->x3 = node->x1;
           dt = node->xetime - grad->xetime;
           if (dt > minTime)
               shgrad_node[dev] = NULL;
           else {
               grad->x2 = node->x2;
               grad->x3 = node->x2;
           }
       }
       else if (grad->xetime > node->xstime) {
           dt = node->xetime - grad->xetime;
           dv = fabs(dt);
           if (dv < minTime) {  // grad->xetime == node->xetime)
               grad->x2 = node->x2;
               grad->x3 = node->x2;
               if (dt > minTime)
                   shgrad_node[dev] = NULL;
           }
           else if (grad->xetime < node->xetime) {
               r = (grad->xetime - node->xstime) / (node->xetime - node->xstime);
               vinc = (int) (r * (node->x2 - node->x1));
               if (vinc < 1)
                    vinc = 1;
               grad->x2 = node->x1 + vinc;
               grad->x3 = grad->x2;
               shgrad_node[dev] = NULL;
           }
           else {  // grad->xetime > node->xetime
               if (grad->x2 < node->x2) {
                   grad->x2 = node->x2 + 1;
                   grad->x3 = grad->x2;
               }
           }
       }
   }
}

static int
measure_xpnt(pnode, lastNode, doFlag, firstTime, verbos)
SRC_NODE  *pnode, *lastNode;
int	  doFlag, firstTime, verbos;
{
   SRC_NODE    *cnode, *dnode;
   SRC_NODE    *cpnode; /* for parallel node */
   COMMON_NODE *comnode, *xnode;
   PAL_NODE    *plnode = NULL, *plnode2;
   int	      points, cx, cx2, incX;
   int	      dev, grad_wait, off_flag, k;
   int	      gradch;
   double      rinc, d, p, rincX;
   double      t_time, time0;
   double      p_time;

   if (debug > 1)
	fprintf(stderr, " measure x  flag= %d\n", doFlag);

   if (doFlag > 0)
   {
	for(dev = 0; dev < TOTALCH; dev++)
	{
	     sp_x[dev] = -1;
	     sp_y[dev] = orgy[dev] - 1;
	     status_node[dev] = NULL;
	     chOn_node[dev] = NULL;
	     prg_node[dev] = NULL;
	}
   }
   for(dev = 0; dev < GRAD_NUM; dev++)
   {
       shgrad_node[dev] = NULL;
   }
   for(dev = 0; dev < TOTALCH; dev++) {
	chx[dev] = 0;
	timeCh[dev] = 0.0;
   }
   if (hasDock) {
        return (measure_dock_xpnt(pnode, lastNode, doFlag, firstTime, verbos));
   }
   clear_parallel_xs();

   t_time = 0.0;
   p_time = 0.0;
   time0 = 0.0;
   pulseCnt = 0.0;
   pulseXCnt = 0.0;
   delayCnt = 0.0;
   delayXCnt = 0.0;
   acqAct = 0;
   exPnts = 0;

   cx = 0;
   points = 0;
   off_flag = 0;
   grad_wait = 0;
   parallel = 0;
   cpnode = NULL;
   mx_p_base = (double) pBase * xMult;    /* pulse */
   mx_d_base = (double) dBase * xMult;    /* delay */
   mx_a_base = (double) acqBase * xMult;  /* acquire */
   mx_p_xbase = (double) pXBase * xMult;
   mx_d_xbase = (double) dXBase * xMult;
   if (doFlag != 3) /* not expanding */
   {
   	if (mx_a_base > pfx * 8)
	    mx_a_base = pfx * 8;
   	mx_x_ratio = xRatio;
   }
   else
   {
   	if (mx_a_base > pfx * 12)
	    mx_a_base = pfx * 12;
   }
   if (mx_a_base % 2 != 0)
	mx_a_base++;
   cnode = disp_start_node;
   while (cnode != NULL && cnode != pnode)
   {
	if (cnode->type == PARALLEL)
	{
	    if (cnode->flag)
		parallel++;
	    else
		parallel--;
	}
	cnode = cnode->dnext;
   }
   if (parallel < 0)
	parallel = 0;
   parallelChan = 0;
   inParallel = 0;
   xEndStatus = 0;
   parallel_time = -1.0;
   parallel_start_node = NULL;

   cnode = pnode;
   while (cnode != NULL && cnode != lastNode)
   {
	dev = cnode->device;
	if (parallel > 0 && dev >= 0)
	    cx = chx[dev];
	else {
	    cx = points;
            if (inParallel)
                cx = set_parallel_x1(cnode, cx, doFlag);
        }
	cx2 = cx;
        time0 = t_time;
        incX = 0;
	rincX = 0.0;
        grad_wait = 1;
        if ((dev > 0) && (cnode->flag & XMARK)) {
            if (!(cnode->flag & XWDH)) {
                grad_wait = 0;
                cnode->wait = 0;
            }
        }
        if (time0 < timeCh[dev]) {
            time0 = timeCh[dev];
            rinc = time0 - t_time;
            if (rinc > minTime) {
                rincX = (time0 - t_time) / dMax + 0.3;  
                if (rincX > 1.0)
                    rincX = 1.0;
                incX = (int) (rincX * mx_d_base);
                if (incX < 3) {
                    incX = 3;
                    cx = cx + incX;
                }
                else {
                    cx = cx + incX;
                    incX = 0;
                }
            }
        }
	cnode->xstime = time0;
        cnode->xetime = time0 + cnode->ptime + cnode->rg1 + cnode->rg2;
	cnode->x1 = cx;
	cnode->x2 = cx;
	cnode->x3 = cx;
	if (doFlag > 0)
	{
	    if ((dev > 0) && (cnode->flag & XMARK))
	    {
		cnode->x1 = cx - spW / 2 - 1;
		cnode->x3 = cnode->x1 + spW;
		if (doFlag == 1)
		{
		   if (sp_x[dev] <= cnode->x1 || cnode->y2 > sp_y[dev])
		   {
		    	sp_x[dev] = cx;
		    	cnode->y2 = cnode->y2 + spH + pfy / 2 + 2;
			cnode->flag = cnode->flag | XNFUP;
		   }
		   else
		   {
		    	cnode->y2 = sp_y[dev] + spH;
		    	cnode->y3 = sp_y[dev];
			cnode->flag = cnode->flag & ~XNFUP;
		   }
		   if (cnode->type == ACQ1)
		    	cnode->y2 = cnode->y2 + spH / 2;
		   sp_y[dev] = cnode->y2+2;
	        }
	    }
	}
        if (dev >= GRADX)
            adjust_gradient_x(0, doFlag, cnode);
	rinc = 0.0;

	switch (cnode->type) {
	case DELAY:
	case VDELAY:
	case INCDLY:
	case VDLST:
	case ZGRAD:
                if (inParallel) {
		   cnode->pflag = XDLY;
                }
                else {
		   rinc = cnode->rtime / dMax;
		   if (rinc > 1.0)
		       rinc = 1.0;
		   rinc = rinc * mx_x_ratio + 1.0;
		   if (parallel > 0) {
		       if (plnode != NULL)
		   	   plnode->delays[dev] += rinc;
		       cx2 = cx + rinc * mx_d_base;
		   }
		   else {
                       if (cnode->rtime > 0.0) {
		           delayCnt = delayCnt + rinc;
		           cx2 = cx + rinc * mx_d_base;
                       }
                       else {
		           delayXCnt = delayXCnt + 1.0;
		           cx2 = cx + mx_d_xbase;
                       }
		   }
                }
                if (cnode->type == ZGRAD) {
                   if (doFlag > 0 && dev > 0)
                       off_flag = RCHNODE;
                }
		break;
	case SHGRAD:
	case SHVGRAD:
	case SHINCGRAD:
	case PESHGR:
	case DPESHGR:
	case OBLSHGR:
	case SH2DVGR:
	case PEOBLVG:
	case PEOBLG:
		if (doFlag > 0 && dev > 0)
		    off_flag = RCHNODE;
		comnode = (COMMON_NODE *) &cnode->node.common_node;
		grad_wait = comnode->val[1];
                cnode->wait = comnode->val[1];
                if (inParallel) {
		   cnode->pflag = XDLY;
                   // sync_node[dev] = cnode;
                }
                else {
		   rinc = cnode->rtime / dMax;
		   if (rinc > 1.0)
		       rinc = 1.0;
		   rinc = rinc * mx_x_ratio + 1.0;
		   if ( grad_wait ) {
		       if (parallel > 0) {
		           if (plnode != NULL)
			       plnode->delays[dev] += rinc;
		       }
		       else
		           delayCnt = delayCnt + rinc;
		   }
		   rinc = rinc * mx_d_base;
	           cnode->x2 = cx + rinc;
                   cnode->x3 = cnode->x2;
		}
		gradch = dev - GRADX;
                if (gradch < 0 || gradch >= GRAD_NUM)
                   gradch = 0;
		shgrad_node[gradch] = cnode;

		if (firstTime > 0)
		{
                     if (gradch == 2 && comnode->pattern != NULL) {
                         for (k = 0; k < 3; k++) {
                              /* duplicate wait flag */
		     	      if (shgrad_node[k] != NULL) {
		                  xnode = (COMMON_NODE *) &shgrad_node[k]->node.common_node;
                                  xnode->vlabel[6] = comnode->vlabel[1];
                                  xnode->val[6] = comnode->val[1];
                              }
                          }
                      }
                }
		if (doFlag > 0)
                {
		     // shgrad_dtime[gradch] = time0 + cnode->rtime;
		}
		if ( grad_wait )
		     cx2 = cx + rinc;
		break;
	case DPESHGR2:
		grad_wait = 0;
                cnode->wait = 0;
		break;
	case PULSE:
	case SMPUL:
	case SPINLK:
                if (inParallel) {
		   cnode->pflag = XPULS;
                   // sync_node[dev] = cnode;
		   cx2 = cx;
                }
                else {
		   rinc = cnode->rtime / pMax;
		   if (rinc > 1.0)
		       rinc = 1.0;
		   rinc = rinc * mx_x_ratio + 1.0;
		   if (parallel > 0) {
		       if (plnode != NULL)
		   	   plnode->pulses[dev] += rinc;
		   }
		   else
		       pulseCnt = pulseCnt + rinc;
		   cx2 = cx + rinc * mx_p_base;
                }
		if (doFlag > 0)
		{
		    off_flag = RSTNODE | RCHNODE | RPGNODE;
		    dnode = cnode;
		    while (dnode != NULL)
		    {
			dnode->x1 = cx;
			dnode->x2 = cx2;
			dnode->x3 = cx2;
			dnode = dnode->bnode;
		    }
		}
		break;
	case SHPUL:
	case SHVPUL:
	case SMSHP:
	case APSHPUL:
	case OFFSHP:
	case GRPPULSE:
                if (inParallel) {
		   cnode->pflag = XPULS2;
                   // sync_node[dev] = cnode;
		   cx2 = cx;
                }
                else {
		   rinc = cnode->rtime / pMax;
		   if (rinc > 1.0)
		       rinc = 1.0;
		   rinc = rinc * mx_x_ratio + 1.0;
		   if (parallel > 0) {
		       if (plnode != NULL)
			   plnode->pulses[dev] = plnode->pulses[dev] + 2 * rinc ;
		   }
		   else
		       pulseCnt = pulseCnt + 2 * rinc;
		   cx2 = cx + 2.0 * rinc * mx_p_base;
                }
		if (doFlag > 0)
		{
		    off_flag = RSTNODE | RCHNODE | RPGNODE;
		    dnode = cnode;
		    while (dnode != NULL)
		    {
			dnode->x1 = cx;
			dnode->x2 = cx2;
			dnode->x3 = cx2;
			dnode = dnode->bnode;
		    }
		}
		break;
	case FIDNODE:
		exPnts += fidWidth;
		cx2 = cx + fidWidth;
		if (doFlag > 0) {
		    dnode = cnode;
		    while (dnode != NULL)
		    {
		        dnode->x1 = cx;
		        dnode->x2 = cx2;
		        dnode->x3 = cx2;
		        dnode = dnode->bnode;
		    }
		}
		break;
	case STATUS:
	case SETST:
		if (doFlag > 0)
		{
		    off_flag = ASTNODE;
		    comnode = (COMMON_NODE *) &cnode->node.common_node;
/*
		    if (dev == 0 || comnode->val[1])
		        off_flag = ASTNODE;
*/
		}
		break;
	case DEVON:
	case VDEVON:
	case XDEVON:
		if (doFlag > 0 && dev > 0)
		{
		    comnode = (COMMON_NODE *) &cnode->node.common_node;
		    if (comnode->val[0] > 0)
		        off_flag = RSTNODE | ACHNODE;
		    else
		        off_flag = RCHNODE | RPGNODE;
		}
		break;
	case RFONOFF:
	    	cnode->x1 = cx;
	    	cnode->x3 = cx + spW;
	        grad_wait = 0;
                cnode->wait = 0;
		break;
	case GRAD:
	case INCGRAD:
	case OBLGRAD:
	case PEGRAD:
	case RGRAD:
	case MGPUL:
	case MGRAD:
	case VGRAD:
		if (doFlag > 0 && dev > 0)
		{
		    off_flag = ACHNODE;
/*
		    if (cnode->flag & NINFO)
		        off_flag = RCHNODE;
		    else
		        off_flag = ACHNODE;
*/
		}
		break;
	case PRGON:
	case VPRGON:
	case PRGOFF:
	case XPRGON:
	case XPRGOFF:
		if (doFlag > 0 && dev > 0)
		{
		    if (prg_node[dev] != NULL)
		    {
			prg_node[dev]->x2 = cx;
			prg_node[dev]->x3 = cx;
		    }
		    if (cnode->type == PRGON || cnode->type == VPRGON || cnode->type == XPRGON)
			prg_node[dev] = cnode;
		    else
			prg_node[dev] = NULL;
		}
		break;
	case ACQUIRE:
	case JFID:
	case XTUNE:
        case XMACQ:
        case SPACQ:
	case SHACQ:
	case SAMPLE:
		if (doFlag > 0)
		    off_flag = RSTNODE | RCHNODE | RPGNODE;
                if (inParallel) {
                   cnode->pflag = PLACQ;
                   if (cnode->rtime2 <= 0.0)
                       cnode->rtime2 = minTime;
                }
                else {
		   acqAct++;
		   exPnts += mx_a_base;
                   cx2 = cx + mx_a_base;
		   if (cnode->type == SHACQ) {
		       acqAct++;
		       exPnts = exPnts + mx_a_base + 2;
                       cx2 = cx2 + mx_a_base + 2;
                   }
		}
		break;
	case HLOOP:
	case GLOOP:
	case SLOOP:
	case ENDHP:
	case ENDSP:
	case ENDSISLP:
	case MSLOOP:
	case PELOOP:
	case PELOOP2:
                if (inParallel) {
                   cnode->pflag = PL_LOOP;
                   cnode->rtime2 = minTime;
		   cx2 = cx;
                }
                else {
		   exPnts += loopW;
		   cx2 = cx + loopW;
                }
		break;
	case NWLOOP:
	case NWLOOPEND:
                if (inParallel) {
                   cnode->pflag = PL_LOOP;
                   cnode->rtime2 = minTime;
		   cx2 = cx;
                }
                else {
		   exPnts += loopW;
		   cx2 = cx + loopW;
                }
		break;
	case XGATE:
	case ROTORP:
	case ROTORS:
	case BGSHIM:
	case SHSELECT:
	case GRADANG:
        case ROTATEANGLE:
        case EXECANGLE:
                if (inParallel) {
                   cnode->pflag = PLBOX;
                   cx2 = cx;
                }
                else {
		   exPnts = exPnts + pfx + 2;
		   cx2 = cx + pfx + 2;
                }
		break;
	case PARALLEL:
		cnode->pnode = NULL;
		cnode->bnode = NULL;
                cx2 = cx;
		for(k = 0; k < TOTALCH; k++)
		{
		    if (chx[k] > cx2)
			cx2 = chx[k];
		}
/*
		for(k = 0; k < TOTALCH; k++)
		{
		    chx[k] = cx2;
		}
*/
		if (cnode->flag)
		{
		    plnode = (PAL_NODE *) &cnode->node.pal_node;
		    for(k = 0; k < TOTALCH; k++) {
			plnode->pulses[k] = 0.0;
			plnode->delays[k] = 0.0;
		    }
		    if (parallel > 0 && cpnode != NULL) { /* nested */
			cnode->pnode = cpnode;
			cpnode->bnode = cnode;
		    }
		    parallel++;
		    cnode->x1 = cx2;
		    cpnode = cnode;
		}
		else
		{
		    if (doFlag > 0 && cpnode != NULL) {
			for(k = 0; k < TOTALCH; k++) {
			    if (plnode->pulses[k] == 0.0 ||
				      plnode->delays[k] == 0.0)
			    {
				if (plnode->pulses[k] != 0.0 || plnode->delays[k] != 0.0)
				expand_sim_width(cpnode, k, cx2);
			    }
			}
		    }
		    if (parallel > 1 && cpnode != NULL) { /* nested */
			plnode = (PAL_NODE *) &cpnode->node.pal_node;
			if (cpnode->pnode != NULL) {
			    plnode2 = (PAL_NODE *) &cpnode->pnode->node.pal_node;
			    for(k = 0; k < TOTALCH; k++) {
				plnode2->pulses[k] += plnode->pulses[k];
				plnode2->delays[k] += plnode->delays[k];
			    }
			}
		    }
		    if (parallel == 1 && cpnode != NULL) {
			plnode = (PAL_NODE *) &cpnode->node.pal_node;
			d = 0.0;
			p = 0.0;
			for(k = 0; k < TOTALCH; k++) {
			    if (plnode->pulses[k] > p)
				p = plnode->pulses[k];
			    if (plnode->delays[k] > d)
				d = plnode->delays[k];
			}
			delayCnt = delayCnt + d;
			pulseCnt = pulseCnt + p;
		    }
		    cnode->x1 = cx2;
		    parallel--;
		    if (parallel <= 0) {
			parallel = 0;
			cpnode = NULL;
		    }
		    else if (cpnode != NULL) 
			cpnode = cpnode->pnode;
		}
		for(k = 0; k < TOTALCH; k++)
		{
		    chx[k] = cx2;
		}
		if (cpnode != NULL)
		    plnode = (PAL_NODE *) &cpnode->node.pal_node;
		else
		    plnode = NULL;
		break;
	case XDOCK:
                cx2 = cx;
                for(k = 0; k < TOTALCH; k++)
                {
                    if (chx[k] > cx2)
                        cx2 = chx[k];
                }
                for(k = 0; k < TOTALCH; k++)
                    chx[k] = cx2;
                if (doFlag > 0) {
                    for (k = 0; k < TOTALCH; k++)
                        sync_node[k] = NULL;
                }
                exPnts += 3;
                cx2 += 3;
                parallelChan = dev;
                inParallel = 1;
	        cnode->x1 = cx2 - 3;
	        cnode->x3 = cx2 + 3;
	        grad_wait = 0;
                cnode->wait = 0;
                clear_parallel_xs();
                parallel_start_node = NULL;
		break;
	case PARALLELSTART:  // measure x
	        grad_wait = 0;
                cnode->wait = 0;
                inParallel = 1;
                parallelChan = dev;
                cx2 = cx;
	        cnode->x1 = cx2 - 3;
	        cnode->x2 = cx2 - 2;
	        cnode->x3 = cx2 + 3;
                if (parallel_start_node == NULL)
                    parallel_start_node = cnode;
		break;
	case PARALLELEND:
	        grad_wait = 0;
                cnode->wait = 0;
                cx2 = cx;
                if (inParallel) {
                    for(k = 0; k < TOTALCH; k++)
                    {
                        if (chx[k] > cx2)
                            cx2 = chx[k];
                    }
                    for(k = 0; k < TOTALCH; k++) {
                        chx[k] = cx2;
                    }
                    if (doFlag > 0)
                        reset_parallel_x2s(cnode, doFlag);
	            cnode->x2 = cx2;
                    exPnts += 3;
                    cx2 += 3;
	            cnode->x1 = cx2 - 3;
	            cnode->x3 = cx2 + 3;
                }
                clear_parallel_xs();
                parallelChan = 0;
                inParallel = 0;
                parallel_start_node = NULL;
		break;
	case PARALLELSYNC:
	        grad_wait = 0;
                cnode->wait = 0;
		break;
	case XEND:
		cpnode = cnode->bnode;
                if (cpnode == NULL)
                    break;
                cx2 = set_parallel_x2(cnode, cx, p_time, doFlag);
	        cnode->x2 = cx2;
	        cnode->x3 = cx2;
                if (cpnode->type != PARALLELSYNC) {
                    cpnode->x2 = cx2; 
                    cpnode->x3 = cx2; 
                }
	        grad_wait = 0;
                cnode->wait = 0;
		break;
	case RLLOOP:
	case RLLOOPEND:
	case KZLOOP:
	case KZLOOPEND:
                cx2 = cx;
                if (inParallel) {
                   cnode->pflag = PL_LOOP;
                   cnode->rtime2 = minTime;
                }
                else {
		    exPnts += loopW;
		    cx2 += loopW;
                }
		break;
        case RDBYTE:
        case WRBYTE:
                cnode->xetime = cnode->xstime;
		break;
	default:
/**
		if (doFlag > 0)
		{
		    if (cnode->flag & XHGHT)
			off_flag = RSTNODE;
		}
**/
		break;
	}
	if (doFlag > 0 && grad_wait)
	{
	    cnode->x2 = cx2;
	    cnode->x3 = cx2;
            adjust_gradient_x(1, doFlag, cnode);
        }

        timeCh[dev] = cnode->xetime;
	if (grad_wait) {
	    t_time = cnode->xetime;
            delayCnt = delayCnt + rincX;
            exPnts = exPnts + incX;
        }
	if (off_flag > 0)
	{
	    dnode = cnode;
	    while (dnode != NULL)
	    {
	        dev = dnode->device;
		if (off_flag & RSTNODE || off_flag & ASTNODE)
		{
		   if (status_node[dev] != NULL)
		   {
	    	     status_node[dev]->x2 = cx;
	    	     status_node[dev]->x3 = cx;
	    	     status_node[dev] = NULL;
		   }
		   if (off_flag & ASTNODE)
		     status_node[dev] = cnode;
		}
		if (off_flag & RCHNODE || off_flag & ACHNODE)
		{
	           if (chOn_node[dev] != NULL)
		   {
	             chOn_node[dev]->x2 = cx;
	             chOn_node[dev]->x3 = cx;
	             chOn_node[dev] = NULL;
		   }
	           if (dev > 1 && (off_flag & ACHNODE))
		     chOn_node[dev] = cnode;
		}
		if (off_flag & RPGNODE)
		{
		   if (prg_node[dev] != NULL)
		   {
			prg_node[dev]->x2 = cx;
			prg_node[dev]->x3 = cx;
			prg_node[dev] = NULL;
		   }
		}
		dnode = dnode->bnode;
	    }
	    off_flag = 0;
	}
	if (parallel <= 0)
	    points = cx2;
        if (cx2 > chx[dev])
            chx[dev] = cx2;
        if (inParallel)
            p_time = cnode->stime;
        else
            p_time = cnode->etime;
	cnode = cnode->dnext;
   } /* while loop */
   if (doFlag > 0)
   {
	for (dev = 0; dev < TOTALCH; dev++)
	{
	    if (status_node[dev] != NULL)
	    {
		status_node[dev]->x2 = points;
		status_node[dev]->x3 = points;
	    }
	    if (chOn_node[dev] != NULL)
	    {
		chOn_node[dev]->x2 = points;
		chOn_node[dev]->x3 = points;
	    }
	    if (prg_node[dev] != NULL)
	    {
		prg_node[dev]->x2 = points;
		prg_node[dev]->x3 = points;
	    }
	}
   }
   
   dpsWidth = points;
   return(points);
}

static void
add_parallel_search_node(node)
SRC_NODE  *node;
{
    SRC_NODE *pnode;

    node->xnext = NULL;
    pnode = parallel_start_node;
    while (pnode != NULL) {
         if (pnode->xnext == NULL)
             break;
         pnode = pnode->xnext;
    }
    if (pnode == NULL)
         parallel_start_node = node;
    else
         pnode->xnext = node;
}

static void
dps_color(num, save)
int	num, save;
{
	Pixel  pix;
	static Pixel  prev = 0;

	if (dpsPlot)
	{
	    // color(num);
	    color(num + vjColor);
	    return;
	}
	if (!Wissun())
	    return;
	if (num < COLORS) {
	    pix = dpsPix[num];
	}
	else {
	    pix = prev;
	}
	if (save) {
	   prev = pix;
	}
	if (!xwin) {
            if (num >= COLORS)
               num = 0;
            set_vj_color(dpsColorNames[num]);
	    return;
	}
	color(num);
}

static double
modify_float_value(fv, precision)
double   fv;
int precision;
{
    double dv, dp;
    int   iv;

    dv = fv;
    dp = 1000;
    if (precision > 3) {
        if (precision == 4)
           dp = 10000;
        if (precision == 5)
           dp = 100000;
        else
           dp = 1000000;
    }
    iv = (int) (dv * dp);
    dv = ((double) iv) / dp;
    return dv;
}

static void
dps_string(int x, int y, char *str)
{
      amove(x, y);
#ifdef VNMRJ
      if (dpsPlot)
          dstring(str);
      else {
          vj_dstring(str, x, y);
      }
#else
      dstring(str);
#endif
}

static void
disp_label(chan, x, x2, y, label)
int   chan, x, x2, y;
char  *label;
{
	int   width, len, posx, posx2, posy, down;
	int   level;
	char  *data;

	if (label == NULL)
	     return;
	if (!(dispMode & DSPLABEL))
	     return;
	dps_color(LCOLOR, 0);
        len = strlen(label);
	if (len <= 0)
	     return;
	if (label[0] == '"' && label[len-1] == '"')
	     return;
	width = pfx * len;
	if (x < x_margin)
             x = x_margin;
        if (x2 >= dpsX2)
             x2 = dpsX2 - 1;
        posx = x + (x2 - x - width) / 2;
        strcpy(inputs, label);
	data = inputs;
	if (posx < x_margin)
	{
	    if (xOffset > 0)
		return;
	    if (posx < dpsX)
		posx = dpsX;
	}
	posx2 = posx + width;
        while (posx2 > dpsX2 && len > 0)
        {
            len--;
            data[len] = '\0';
            posx2 -= pfx;
        }
        if (len <= 0)
            return;

	if (chan >= GRADX)  /* for gradient */
	{
	     if (posx >= gradsx[chan - GRADX])
	        posy = y - pfy - 2; 
             else {
                posx = gradsx[chan - GRADX];
                if ((posx + width) > x2)
                   return;
	        posy = y - pfy * 1.5;
	        posx2 = posx + width;
             }
	     gradsx[chan - GRADX] = posx2 + pfx / 3;
             amove(posx, posy);
#ifdef VNMRJ
                if (dpsPlot)
                    dstring(data);
                else {
                    posx = (x + x2) / 2;
                    sun_dhstring(data, posx, posy);
                }
#else
                dstring(data);
#endif
	     return;
	}
	if (chan == TODEV)
	     level = pul_s_r;
	else
	     level = dec_s_r;
        if (len > 0 && len < paramLen)
        {
           for (down = 0; down < level; down++)
	   {
	      if (posx > predsx[chan][down])
	             break;
	   }
           if (down < level)
	   {
                predsx[chan][down] = posx2 + pfx / 3;
		down++;
	        posy = y - pfy * down - 2; 
                amove(posx, posy);
#ifdef VNMRJ
                if (dpsPlot)
                    dstring(data);
                else {
                    posx = (x + x2) / 2;
                    sun_dhstring(data, posx, posy);
                }
#else
                dstring(data);
#endif
	   }
        }
}

static void
disp_dual_label(chan, x, x2, y, node )
int   chan, x, x2, y;
SRC_NODE  *node;
{
     int   k;
     SRC_NODE    *dnode;
     COMMON_NODE *comnode;
     char  label[256];

     comnode = (COMMON_NODE *) &node->node.common_node;
     if (comnode->pattern == NULL)
        return;
     k = (int) strlen(comnode->pattern);
     if (k > 200) {
        k = 200;
        strncpy(label, comnode->pattern, k);
        label[k] = '\0';
     }
     else
        strcpy(label, comnode->pattern);
     // dnode = node->next;
     dnode = node->bnode;
     if (dnode != NULL && dnode->type == DPESHGR2) {
        comnode = (COMMON_NODE *) &dnode->node.common_node;
        if (comnode->pattern != NULL) {
            strcat(label, ",");
            k = k + (int) strlen(comnode->pattern);
            if (k < 254)
               strcat(label, comnode->pattern);
            else
               strcat(label, "...");
        }
     }
     
     disp_label(chan, x, x2, y, label);
}

static int
adjust_time_value(sval, long_label)
double   sval;
int	long_label;
{
   double  value;
   double  dv;

   value = sval * 1.0e+6;  // convert to micro second
   if (value >= 1000.0)
   {
      value = value / 1000.0; // milli second
      if (value >= 1000.0)
      {
         if (colorwindow)
            dps_color(SCOLOR, 0);
         value = value / 1000.0;  // second
         sprintf(inputs, "%g", value);
         return SECD;
      }
      if (colorwindow)
         dps_color(MCOLOR, 0);
      dv = modify_float_value(value, 6);
      sprintf(inputs, "%g", dv);
      return MSEC;
   }
   if (colorwindow)
       dps_color(UCOLOR, 0);
   if (sval < 0.0 && ! long_label)
   {
       sprintf(inputs, "?");
       return 0;
   }
   if (value == 0.0)
   {
       if (long_label)
           sprintf(inputs, "0.0 seconds");
       else
           sprintf(inputs, "0.0");
       return 0;
   }
   dv = modify_float_value(value, 6);
   sprintf(inputs, "%g", dv);
   return USEC;
}

static void
modify_time_value(sval, long_label)
double   sval;
int	long_label;
{
    int retv, k, k0, len;
    char  *p;

    retv = adjust_time_value(sval, long_label);
    if (retv < 1)
        return;

    p = rindex(inputs, '.');
    if (p != NULL) {
        if (!long_label) {
           len = (int) strlen(inputs);
           k0 = (int) (p - inputs);
           k = k0 + 1;
           while (k < len) {
              if (inputs[k] != '0')
                 break;
              k++;
           }
           k = k - k0 + 1;   
           if (k > 5)
               k = 5;
           if (k < 4)
               k = 4;
           inputs[k0+k] = '\0';
        }
    }
    else
        strcat(inputs, ".0");
    if (long_label) {
        if (retv == SECD)
           strcat(inputs, " seconds");
        else if (retv == MSEC)
           strcat(inputs, " msec");
        else if (retv == USEC)
           strcat(inputs, " usec");
        return;
    }
    if (!colorwindow ) {
        if (retv == SECD)
           strcat(inputs, "sec");
        else if (retv == MSEC)
           strcat(inputs, "ms");
        else if (retv == USEC)
           strcat(inputs, "us");
    }
}

static void
dpsDstring(str, px, py, x1, x2)
char *str;
int   px, py, x1, x2;
{
	amove(px, py);
        if (!xwin) {
#ifdef VNMRJ
           if (dpsPlot)
	      dstring(str);
           else {
              px = (x1 + x2) / 2;
              sun_dhstring(str, px, py);
           }
#else
	   dstring(str);
#endif
           return;
	}
	while(*str != '\0')
	{
	   if (*str != '.')
	   {
		dchar(*str++);
		px += pfx;
	   }
	   else
	   {
		rmove(-pfx / 4, 0);
		dchar(*str++);
		px += pfx * 0.6;
	   }
	   amove(px, py);
	}
}


static void
disp_value(chan, x, x2, y, val, mode)
int     chan, x, x2, y, mode;
double   val;
{
	int   posx, posy, i, width, wx, wh;
	int   level;
	char  *data;

	if (!(dispMode & DSPVALUE))
	     return;
        if (mode == PHASEMODE)
        {
             if (val > 360.0)
                 val = fmodf(val, 360.0);
        }
	if (chan == TODEV)
	     level = pul_v_r;
	else
	     level = dec_v_r;
	if (level <= 0) {
             if (chan == TODEV)
                wh = mnumypnts;
             else
                wh = orgy[chan -1];
             posy = y + pfy;
             if (posy > (wh - pfy))
	        return;
        }
	if (mode == TIMEMODE)
	     modify_time_value(val, 0);
	else
	{
	     if (mode == POWERMODE)
		dps_color(PWCOLOR, 0);
	     else
		dps_color(PHCOLOR, 0);
	     if (mode == INTMODE)
             	sprintf(inputs, "%d", (int)val);
	     else
                sprintf(inputs, "%g", modify_float_value(val, 3));
             //   sprintf(inputs, "%.1f", val);
	}
        width = pfx * strlen(inputs) - 4;
	if (x < x_margin)
	     x = x_margin;
	if (x2 >= dpsX2)
	     x2 = dpsX2 - 1;
	wx = x2 - x;
        posx = x + (wx - width) / 2;
	posy = y + chdescent;
	if (width <= wx)
	{
	     dpsDstring(inputs, posx, posy, x, x2);
	     return;
	}
	data = inputs;
	if (posx < x_margin)
	{
	    if (xOffset > 0)
		return;
	    if (posx < dpsX)
		posx = dpsX;
	}
	i = strlen(data);
	wx = posx + pfx * i;
	while (wx > dpsX2 && i > 0)
	{
	    i--;
	    data[i] = '\0';
	    wx -= pfx;
	}
	if (i <= 0)
	    return;
	
	if (chan >= GRADX)  /* for gradient */
	{
	     if (posx > gradvx[chan - GRADX])
	     {
		gradvx[chan - GRADX] = wx + pfx / 2;
	        dpsDstring(inputs, posx, posy, x, x2);
	     }
	     return;
	}
	     
	if (y == orgy[chan])  /* it is from delay func */
	{
	     if (posx > delay_x1)
	     {
		delay_x1 = wx + pfx / 2;
	     }
	     else if (posx > delay_x2)
	     {
	        posy = posy + pfy;
		delay_x2 = wx + pfx / 2;
	     }
	     else
	     	return;
	     dpsDstring(inputs, posx, posy, x, x2);
	     return;
	}
	for (i = 0; i < level; i++)
	{
	     if (posx > predvx[chan][i])
	            break;
	}
	if ( i < level)
	{
	     posy = orgy[chan] + pulseHeight + pfy * i + chdescent; 
	     if (posy < y) {  /* this is loop only */
		if (i < (level - 1)) {
		    posy = y; 
		    i++;
		}
	     }
             predvx[chan][i] = wx + pfx / 2;
	     dpsDstring(inputs, posx, posy, x, x2);
	}
	return;
}


static void
disp_time_mark()
{
        int   xs, ys, xflag;

	if (!showTimeMark)
	    return;
	if ((dispMode & TIMEMODE) && colorwindow)
        {
		// showTimeMark = 0;
		xflag = xorFlag;
		if (xorFlag)
		{
	    	   normalmode();
		   xorFlag = 0;
		}
                xs = graf_width - pfx * 4 - menu_width - 10;
                ys = mnumypnts - pfy - 2;
                amove(xs, ys);
	        dps_color(SCOLOR, 0);
                box(pfx, pfy);
                // amove(xs + pfx + 3, ys);
                // dstring("sec");
                dps_string(xs + pfx + 3, ys, "sec");
                ys = ys - pfy;
                amove(xs, ys);
	        dps_color(MCOLOR, 0);
                box(pfx, pfy);
                // amove(xs + pfx + 3, ys);
                // dstring("ms");
                dps_string(xs + pfx + 3, ys, "ms");
                ys = ys - pfy;
                amove(xs, ys);
	        dps_color(UCOLOR, 0);
                box(pfx, pfy);
                // amove(xs + pfx + 3, ys);
                // dstring("us");
                dps_string(xs + pfx + 3, ys, "us");
		if (xflag)
		{
	    	   xormode();
		   xorFlag = 1;
		}
        }
}


static void
disp_chan_mark()
{
	int	k;

	if (dpsPlot && !plotChanName)
            return;
        if (!dpsPlot && !winChanName)
            return;
        dps_color(CHCOLOR, 0);
	for (k = TODEV; k < RFCHAN_NUM; k++)
	{
	    if (rfchan[k] && visibleCh[k])
	    {
		// amove(dpsX, orgy[k] + ycharpixels / 5);
                // dstring(ch_label[k]);
                dps_string(dpsX, orgy[k] + ycharpixels / 5, ch_label[k]);
		// amove(dpsX, orgy[k] - ycharpixels * 4/ 5);
                if (dnstr[k][0] != '0') {
                   // dstring(dnstr[k]);
                   dps_string(dpsX, orgy[k] - ycharpixels * 4/ 5, dnstr[k]);
                }
                else {
                   // dstring("''");
                   dps_string(dpsX, orgy[k] - ycharpixels * 4/ 5, "''");
                }
	    }
	}
	for (k = GRADX; k <= GRADZ; k++)
	{
	    if (rfchan[k])
	    {
		// amove(dpsX, orgy[k] - ycharpixels / 5);
		if ((image_flag & OBIMAGE) && !(image_flag & MAIMAGE)) {
                    // dstring(image_label[k - GRADX]);
                    dps_string(dpsX, orgy[k] - ycharpixels / 5, image_label[k - GRADX]);
                }
		else {
                    // dstring(gradient_label[k - GRADX]);
                    dps_string(dpsX, orgy[k] - ycharpixels / 5, gradient_label[k - GRADX]);
                }
	    }
	}
        if (rfchan[RCVRDEV]) {
	    // amove(dpsX, orgy[RCVRDEV] - ycharpixels / 5);
            // dstring(ch_label[RCVRDEV]);
            dps_string(dpsX, orgy[RCVRDEV] - ycharpixels / 5, ch_label[RCVRDEV]);
        }
	if (!dpsPlot && !clearDraw)
	{
	    if (backMap == 0)
	    	return;
	}
	if (in_browse_mode || transient > 1)
	{
	    sprintf(info_data, "Scan: %d", transient);
	    // amove(dpsX, dpsY2 - pfy);
	    // dstring(info_data);
            dps_string(dpsX, dpsY2 - pfy, info_data);
	}
}

static void
draw_dps()
{
	SRC_NODE   *node, *stnode;
        int     k, dev;
	FILE    *plotFd;

	if (debug > 1)
	    fprintf(stderr, "  draw dps\n");
	if (src_start_node == NULL || disp_start_node == NULL)
	    return;
#ifdef MOTIF
	if (!dpsPlot && dfontInfo)
	    set_font(dfontInfo->fid);
#endif
#ifdef VNMRJ
	if (!dpsPlot)
            grf_batch(1);
#endif
	disp_chan_mark();
	for (dev = 0; dev < TOTALCH; dev++)
	{
	    chyc[dev] = orgy[dev];
	    info_node[dev] = NULL;
	    chMod[dev] = 0;	
	    chx[dev] = x_margin;
	    delMark[dev] = -99;
	}
	reset_text_pos();

	node = disp_start_node;
	draw_start_node = NULL;
	stnode = NULL;
	xDiff = x_margin - xOffset;
	while (node != NULL)
	{
	    if (node->type == STATUS && node->device == 0)
		stnode = node;
	    else
	    {
		if ((node->flag & XMARK) == 0)
		{
		    if (node->x2 > xOffset)
		    {
		       draw_start_node = node;
		       break;
		    }
		}
		else
		{
		    if (node->x2 >= xOffset)
		    {
		       draw_start_node = node;
		       break;
		    }
		}
		dev = node->device;
		chyc[dev] = node->y3;
		chMod[dev] = node->mode;
	    }
	    node = node->dnext;
	}
	if (draw_start_node == NULL)
             return;
	draw_end_node = draw_start_node->dnext;
        while (draw_end_node != NULL)
        {
             if (draw_end_node->x1 + xDiff >= dpsX2)
                    break;
             draw_end_node = draw_end_node->dnext;
        }

	delay_x1 = 0;
	delay_x2 = 0;
	noStatus = 1;
	if (dpsPlot) {
            if (raster >= 3 && raster <= 4) {
		plotFd = plot_file();
		if (is_vplot_session(1) < 1) {
		    if (plotFd != NULL) {
			fprintf(plotFd,"1 setlinewidth\n");
		    }
		}
	    }
	}
	if (debug > 2)
	    fprintf(stderr, "  start drawing ...\n");
	parallelChan = 0;
        parallel_start_node = NULL;
	node = draw_start_node;
	while (node != draw_end_node)
	{
	    draw_node(node, 1);
	    node = node->dnext;
	}
	if (stnode)
	    draw_node(stnode, 1);
        if (dpsWidth + xDiff <= dpsX2)
           k = dpsWidth;
        else
           k = dpsX2 - xDiff;
	if (src_end_node == NULL)
	{
            if((src_end_node = new_dpsnode()) == NULL)
                return;
	}
	src_end_node->x1 = k;
	src_end_node->x2 = k;
	src_end_node->x3 = k;
	src_end_node->type = DUMMY;

	for (dev = TODEV; dev < TOTALCH; dev++)
	{
	    if (visibleCh[dev]) {
	        if (delMark[dev] != -99)
		    delay_mark(delMark[dev], 0, 0, dev);
	        src_end_node->device = dev;
	        src_end_node->y1 = chyc[dev];
	        update_chan(src_end_node);
            }
	}
        if (hasDock) {
	     node = draw_start_node;
	     while (node != draw_end_node)
	     {
		 if (node->type == DELAY)
	             draw_node(node, 3);
	         node = node->dnext;
	     }
	}
        else {
	    dps_color(PCOLOR, 0);
	    amove(x_margin, statusY);
	    adraw(k+xDiff, statusY);
        }
	
	if (dpsPlot) {
	    dps_color(PCOLOR, 0);
            disp_psg_name(psgfile);
	}
	if (!dpsPlot)
	{
	    dps_color(PCOLOR, 0);
	    disp_time_mark();
	    if (hilitNode)
	    {
		if (hilitNode->flag & XMARK)
		    node = info_node[0];
		else
		    node = info_node[hilitNode->device];
		while (node != NULL)
		{
		     if (node == hilitNode)
			break;
		     node = node->knext;
		}
		hilitNode = node;
	    }
	    if (hilitNode)
	    {
	        draw_node(hilitNode, 0);
	    	disp_info(hilitNode);
	    }
	    else
    	   	clear_dps_info();
	}
	parallelChan = 0;
        if (debug > 1)
	    fprintf(stderr, "  draw dps done\n");
#ifdef MOTIF
	if (!dpsPlot && dfontInfo)
	    set_font(x_font);
#endif
#ifdef VNMRJ
	if (!dpsPlot)
            grf_batch(0);
#endif
}



static void
delay_mark(x1, x2, x3, dev)
int	x1, x2, x3, dev;
{
/*
     if (dev != TODEV)
	return;
*/
     if (dev >= GRADX)
	return;
 
     if (delMark[dev] == -99 || delMark[dev] > x_margin)
     {
	if (x1 > x_margin && x1 >= delMark[dev])
	{
		dps_color(PCOLOR, 0);
		if (delMark[dev] != -99) {
		   amove(delMark[dev], chyc[dev]-3);
		   rdraw(0, 5);
		}
		if (x1 > delMark[dev] && x2 > x1)
		{
		   amove(x1, chyc[dev]-3);
		   rdraw(0, 5);
		}
	}
     }
     if (x3 >= delMark[dev])
        delMark[dev] = x2;
}


static void
draw_rf_onoff(node, on)
SRC_NODE   *node;
int	   on;
{
	int	x, y, w, h;

	x = node->x2 + xDiff;
	if (x < dpsX)
            return;
	h = spH;
	w = spW;
	node->x1 = node->x2 - spW / 2;
	node->x3 = node->x1 + spW;
	y = node->y2 - h;
	amove(x, y);
	rdraw(0, h);
        y++;
	while (w > 1)
	{
	    amove(x, y);
	    rdraw(w, 0);
	    y++;
	    w -= 2;
	}
	if (node->flag & XNFUP)
	{
	    amove(x, node->y3 + 2);
	    rdraw(0, y - node->y3);
	}
}

static void
draw_sync(node)
SRC_NODE   *node;
{
        int     x, y, w, h;

        x = node->x2 + xDiff;
        if (x < dpsX)
            return;
        w = spW / 4;
        h = spH / 2;
        node->x1 = node->x2 - spW / 2;
        node->x3 = node->x1 + spW;
        y = node->y2 - h;
        amove(x - w, y);
        rdraw(0, h);
        rdraw(spW / 2, 0);
        rdraw(0, -h);
        w = spW / 2;
        amove(x - w, y);
        rdraw(w, -h);
        rdraw(w, h);
        amove(x - w, y);
        w = spW / 4;
        rdraw(w, 0);
        amove(x + w, y);
        rdraw(w, 0);
        if (node->flag & XNFUP)
        {
            amove(x, node->y3 + 2);
            rdraw(0, spH - 2);
        }
}



static void
draw_rotate(node, dlink)
SRC_NODE   *node;
int        dlink;
{
	int	x, y, x1, y1, x2, w, w2;

	x2 = node->x2 + xDiff + 1;
	if (x2 < dpsX)
            return;
	node->x1 = node->x2 - spW / 2;
	node->x3 = node->x1 + spW;
	x = node->x1 + xDiff + 1;
	y = node->y2;
	w = spW - 2;
	amove(x, y);
	rdraw(w, 0);
	rdraw(0, -spH);
	rdraw(-w, 0);
	rdraw(0, spH);
	y1 = y - spH * 0.6;
	w2 = w / 6;
	if (w2 < 2) w2 = 2;
	x1 = x - w2;
	x2 = x + w2;
	while (x1 <= x2) {
	    amove(x1, y1);
	    adraw(x, y);
	    x1++;
	}
	x = x + w;
	y = y - spH;
	y1 = y + spH * 0.6;
	x1 = x - w2;
	x2 = x + w2;
	while (x1 <= x2) {
	    amove(x1, y1);
	    adraw(x, y);
	    x1++;
	}
	if (dlink && (node->flag & XNFUP))
	{
	    x = node->x2 + xDiff;
	    amove(x, node->y3 + 2);
	    rdraw(0, spH - 2);
	}
}

/* startacq */
static void
draw_acq1(node, dlink)
SRC_NODE   *node;
int	   dlink;
{
	int	x, y, x1, y1, w, h;
	double deg, incD;

	x = node->x2 + xDiff;
	if (x < dpsX)
            return;
	node->x1 = node->x2 - spW / 2;
	node->x3 = node->x1 + spW;
	x = node->x1 + xDiff;
	y = node->y2 - spH * 0.7;
	amove(x - 1, y);
	rdraw(spW + 3, 0);
	h = spH * 0.7;
	incD = 6.28 * 1.25 / (double) spW;
	deg = 6.28 - 3.14 * 0.25;
	amove(x, y);
	x1 = x+1;
        for (w = 0; w < spW; w++) {
	    deg += incD;
	    y1 = y + h * sin(deg);
	    adraw(x1, y1);
	    x1++;
	}

	if (dlink > 0 && (node->flag & XNFUP))
	{
	    x = node->x2 + xDiff;
	    amove(x, node->y3 + 1);
	    rdraw(0, spH - 3);
	}
}

static void
draw_power(node, solid)
SRC_NODE   *node;
int	   solid;
{
	int	x, y, w, h;

	x = node->x2 + xDiff;
	if (x < dpsX)
            return;
	node->x1 = node->x2 - spW / 2;
	node->x3 = node->x1 + spW;
	x = node->x1 + xDiff;
	y = node->y2 - spH;
	if (solid)
	{
	    w = spW;
	    h = spH;
	    while (h > 0)
	    {
		amove(x, y);
		rdraw(w, 0);
		if (h > 0)
		{
		    h--;
		    y++;
		}
		if (w > 1)
		{
		    x++;
		    w -= 2;
		}
	     }
	}
	else
	{
 	     amove(x, y);
	     rdraw(spW, 0);
	     rdraw(-spW/2, spH);
	     rdraw(-spW/2, -spH);
	}
	if (node->flag & XNFUP)
	{
	    x = node->x2 + xDiff;
	    amove(x, node->y3 + 2);
	    rdraw(0, spH - 2);
	}
}

static void
draw_offset(node, solid)
SRC_NODE   *node;
int	   solid;
{
	int	h, w;
	int	x, x2, y, y2;

	x = node->x2 + xDiff;
	if (x < dpsX)
            return;
	node->x1 = node->x2 - spW / 2;
	node->x3 = node->x1 + spW;
	x = node->x1 + xDiff;
	x2 = x + spW;
	y2 = node->y2;
	y = y2 - spH;
 	amove(x, y);
	if ( !solid )
	{
	    rdraw(spW, 0);
	    rdraw(0, spH);
	    rdraw(-spW, -spH);
	}
	else
	{
	    h = spH;
	    w = spW;
	    while((h > 0) || (w > 0))
	    {
		amove(x2 - w, y);
		rdraw(w, h);
		if (w > 0)
		    w--;
		if (h > 0)
		    h--;
	     }
	}
	 
	if (node->flag & XNFUP)
	{
	    x2 = node->x2 + xDiff;
	    amove(x2, node->y3 + 2);
	    rdraw(0, spH - 2);
	}
}

static void
draw_spare(SRC_NODE *node)
{
	int	h, w;
	int	x, y;

	x = node->x2 + xDiff;
	if (x < dpsX)
            return;
	w = spW / 2;
	h = spH / 2;
	node->x1 = node->x2 - w;
	node->x3 = node->x1 + spW;
	y = node->y2 - h;
	x = node->x1 + xDiff;
 	amove(x, y);
	rdraw(w, h);
	rdraw(w, -h);
 	amove(x, y);
	rdraw(w, -h);
	rdraw(w, h);

	if (node->flag & XNFUP)
	{
	    amove(node->x2 + xDiff, node->y3 + 2);
	    rdraw(0, spH - 2);
	}
}

static void
draw_mrd(node)
SRC_NODE   *node;
{
	int	h, w;
	int	x, y, x2, y2, x3;

	x = node->x1 + xDiff;
	if (x < dpsX)
            return;
	w = spW / 2;
	h = spH / 2;
	y2 = node->y2;
	y = node->y2 - h;
	x = node->x2 + xDiff - w;
 	amove(x, y);
	rdraw(w, h);
	rdraw(w, -h);
 	amove(x, y);
	rdraw(w, -h);
	rdraw(w, h);
	if (node->type == RDBYTE) {
	    y2 = node->y2 - spH + 1;
	    x2 = x + w;
	    for (h = 1; h < spH; h++) {
 	       amove(x, y);
 	       adraw(x2, y2);
               y2++;
            }
	}
	else if (node->type == WRBYTE) {
	    x2 = x + spW;
	    x = x + w;
	    y2 = y;
	    y = node->y2 - spH + 1;
	    for (h = 1; h < spH; h++) {
 	       amove(x, y);
 	       adraw(x2, y2);
               y++;
            }
	}
	if (node->type == TRIGGER || node->type == SETANGLE) {
	    y2 = node->y2;
	    x2 = node->x2 + xDiff;
	    x3 = x + 1;
	    for (w = 1; w < spW; w++) {
 	       amove(x3, y);
 	       adraw(x2, y2);
	       x3++;
            }
            if (node->type == SETANGLE) {
	       y2 = node->y2 - spH;
	       x3 = x + 1;
	       for (w = 1; w < spW; w++) {
 	          amove(x3, y);
 	          adraw(x2, y2);
	          x3++;
               }
            }
	}
	else if (node->type == SETGATE) {
	    y2 = node->y2 - spH;
	    x2 = node->x2 + xDiff;
	    x = x + 1;
	    for (w = 1; w < spW; w++) {
 	       amove(x, y);
 	       adraw(x2, y2);
	       x++;
            }
	}
        
	if (node->flag & XNFUP)
	{
	    amove(node->x2 + xDiff, node->y3 + 2);
	    rdraw(0, spH - 2);
	}
}




static void
draw_phase(node)
SRC_NODE   *node;
{
	int	x, x2, y, w, h, dh;

	x2 = node->x2 + xDiff;
	if (x2 < dpsX)
            return;
	w = spW / 2;
	h = spH / 2;
	node->x1 = node->x2 - w;
	node->x3 = node->x1 + spW;
	x = x2 - w;
	y = node->y2 - h;
	amove(x, y);
	rdraw(spW, 0);
	dh = 1;
	w--;
	while (dh <= h)
	{
	    amove(x2, y + dh);
	    rdraw(w, 0);
	    amove(x2, y - dh);
	    rdraw(w, 0);
	    if (w > 1)
		w -= 2;
	    dh++;
	}
	amove(x, y + 1);
	adraw(x2, y + 1);
	amove(x, y - 1);
	adraw(x2, y - 1);
	    
	if (node->flag & XNFUP)
	{
	    amove(x2, node->y3 + 2);
	    rdraw(0, spH);
	}
}

static void
draw_lock(node, amp, on)
SRC_NODE   *node;
int	   amp, on;
{
	int	x, x2, y;

	x2 = node->x2 + xDiff;
	if (x2 < dpsX)
            return;
	node->x1 = node->x2 - spW / 2;
	node->x3 = node->x1 + spW;
	x = node->x1 + xDiff;
	y = node->y2 - spH;
 	amove(x, y);
	rdraw(spW, 0);
	rdraw(0, spH);
	rdraw(-spW, 0);
	rdraw(0, -spH);
	if (node->flag & XNFUP)
	{
	    x2 = node->x2 + xDiff;
	    amove(x2, node->y3 + 2);
	    rdraw(0, spH - 2);
	}
	if (amp)
	{
 	    // amove(x + spW / 2, y);
	    // rdraw(0, spH);
 	    // amove(x, y + spH / 2);
	    // rdraw(spW, 0);
            if (on == 0) {
               amove(x, y + spH / 2);
               box(spW / 2, spH / 2);
               amove(x + spW / 2, y);
               box(spW / 2, spH / 2);
            }
            else {
 	       amove(x + spW / 2, y);
	       rdraw(0, spH);
 	       amove(x, y + spH / 2);
	       rdraw(spW, 0);
            }
	}
}

static void
draw_square_mark(node, which)
SRC_NODE   *node;
int	   which;
{
	int	x, x2, y, y2;

	x = node->x1 + xDiff;
	x2 = node->x3 + xDiff;
	if (x < dpsX)
        {
            if (debug > 1)
	       fprintf(stderr,"x=%f, xDiff=%f, sum<dpsX=%f, returning\n",
			(double) node->x1, (double) xDiff, (double) dpsX);
            return;
        }
	y = node->y2;
	y2 = node->y3;
 	amove(x, y);
	adraw(x2, y);
	adraw(x2, y2);
	adraw(x, y2);
	adraw(x, y);
	x = x + (x2 - x - pfx) / 2;
	amove(x, y - chascent);
	switch (which) {
	 case  XGATE:
		    // dstring("x");
                    dps_string(x, y - chascent, "x");
		    break;
	 case  ROTORP: 
		    // dstring("r");
                    dps_string(x, y - chascent, "r");
		    break;
	 case  ROTORS: 
		    // dstring("R");
                    dps_string(x, y - chascent, "R");
		    break;
	 case  SPINLK: 
		    // dstring("s");
                    dps_string(x, y - chascent, "s");
		    break;
	 case  BGSHIM: 
		    // dstring("d");
                    dps_string(x, y - chascent, "d");
		    break;
	 case  SHSELECT: 
		    // dstring("w");
                    dps_string(x, y - chascent, "w");
		    break;
	 case  GRADANG: 
	 case  ROTATEANGLE: 
	 case  EXECANGLE: 
		    // dstring("A");
                    dps_string(x, y - chascent, "A");
		    break;
	}
}


static void
draw_prg_mark(node, amp)
SRC_NODE   *node;
int	   amp;
{
	int	x, x2, y;

	x2 = node->x2 + xDiff;
	if (x2 < dpsX)
            return;
	node->x1 = node->x2 - spW / 2;
	node->x3 = node->x1 + spW;
	x = node->x1 + xDiff;
	y = node->y2 - spH;
 	amove(x, y);
        box(spW, spH);
	if (node->flag & XNFUP)
	{
	    amove(x2, node->y3 + 2);
	    rdraw(0, spH);
	}
	color(BLACK);
 	amove(x+2, y+1);
	rdraw(spW / 2 - 2, spH - 3);
	rdraw(spW / 2 - 2, 3 - spH);
 	amove(x+3, y+1);
	rdraw(spW / 2 - 3, spH - 4);
	rdraw(spW / 2 - 3, 4 - spH);
}

static void
rcv_base(sx, sy, sh, sw, solid)
int sx, sy, sh, sw, solid;
{
	double  deg, step;
	int     y2, x2, i;

	step = 3.14 / sw;
	deg = 0;
	x2 = sx;
	y2 = sy;
	if (sx < x_margin)
	{
	     i = x_margin - sx;
	     sw = sw - i;
	     if (sw <= 0)
		return;
	     deg = step * i;
	     y2 = sy + sh * sin(deg);
	     x2 = sx + i;
	}
	amove(sx, sy);
	adraw(sx + sw, sy);
	amove(x2, y2);
        if (solid) 
        {
	    for(i = 0; i < sw; i++)
	    {
	   	deg = deg + step;
	    	y2 = sy + sh * sin(deg);
		amove(x2, y2);
		adraw(x2, sy);
	    	x2++;
		if (x2 >= dpsX2)
		    break;
	    }
	}
        else
        {
	    for(i = 0; i < sw; i++)
	    {
	   	deg = deg + step;
	    	y2 = sy + sh * sin(deg);
		adraw(x2, y2);
	    	x2++;
		if (x2 >= dpsX2)
		    break;
	    }
	}
}


static void
draw_receiver(node, solid)
SRC_NODE   *node;
int	   solid;
{
	int	x, x2, y, y2;

	x2 = node->x2 + xDiff;
	if (x2 < dpsX-1)
            return;
	node->x1 = node->x2 - spW / 2;
	node->x3 = node->x1 + spW;
	x = x2 - spW / 2;
	y2 = node->y2;
	y = y2 - spH;
	rcv_base(x, y, spH / 2, spW, solid);
	if (solid)
	{
	    amove(x-1, y2);
 	    adraw(x2, y);
	    adraw(x+spW+1, y2);
	}
	else
	{
	    amove(x-1, y2);
 	    adraw(x2 - 2, y + 3);
	    amove(x2 + 2, y + 3);
	    adraw(x+spW+1, y2);
	}
	if (node->flag & XNFUP)
	{
	    amove(x2, node->y3 + 2);
	    rdraw(0, spH);
	}
}


static void
adjust_shaped_pulse_height(snode)
SRC_NODE  *snode;
{
    int  k, dev;
    double *src, dh, dm, devMax, maxPower;
    short v, *dest;
    SHAPE_NODE   *sh_node;
    COMMON_NODE *comnode;

    sh_node = snode->shapeData;
    if (sh_node->data == NULL)
        return;
    dev = snode->device;
    if (snode->power == 0.0) {
        comnode = (COMMON_NODE *) &snode->node.common_node;
        if (snode->type == OBLSHGR) {
            snode->visible = 0;
            return;
        }
        else if (snode->type == PESHGR || snode->type == DPESHGR ||
                                snode->type == DPESHGR2) {
            if (comnode->fval[2] == 0.0) {  // step
                snode->visible = 0;
                return;
            }
        }
    } 
    snode->visible = 1;
    dest = sh_node->data;
    if (sh_node->type == GRADDEV) {
        if (sh_node->shapeType == ABS_SHAPE) {
            if (sh_node->power == snode->power)
                 return;
        }
        sh_node->shapeType = ABS_SHAPE;
        src = sh_node->amp;
    }
    else { // RF transmitter
        if (sh_node->shapeType == rfShapeType) {
            if (sh_node->power == snode->power)
                 return;
        }
        src = sh_node->amp;
        sh_node->shapeType = rfShapeType;
        if (rfShapeType == REAL_SHAPE)
            src = sh_node->phase;
    }
    devMax = fabs(sh_node->maxData);
    if (fabs(sh_node->minData) > devMax)
        devMax = fabs(sh_node->minData);
    if (devMax == 0.0) {
        snode->visible = 0;
        /***
        for (k = 0; k < sh_node->dataSize; k++) {
            *dest = 0;
            dest++;
        }
        ***/
        return;
    }
    sh_node->power = snode->power;

    if (sh_node->type == GRADDEV) {
        maxPower = gradMaxLevel;
        if (fabs(gradMinLevel) > maxPower)
            maxPower = fabs(gradMinLevel);
        dh = (double)gradHeight;
    }
    else {
        maxPower = powerMax[dev];
        dh = (double)pulseHeight;
    }
    if (fabs(snode->power) > maxPower)
        maxPower = fabs(snode->power);
    if (maxPower == 0.0)
        maxPower = 2.0;
    dm = fabs(snode->power / maxPower) * 0.7 + 0.3;
    if (dm > 1.0)
        dm = 1.0;
    if (sh_node->type == GRADDEV) {
        if (snode->power < 0.0)
            dm = -dm;
    }
    if (snode->y2 < (dh + orgy[dev]))
         snode->y2 = dh + orgy[dev];
    dh = dh * dm / devMax;
    for (k = 0; k < sh_node->dataSize; k++) {
        v = (short) (*src * dh);
        if (v == 0 && *src != 0.0) {
            v = 1;
            if (*src < 0.0 && dh > 0.0)
                v = -1;
            else if (*src > 0.0 && dh < 0.0)
                v = -1;
        }
        *dest = v;
        dest++;
        src++;
    }
}

static int
draw_rf_shape(snode, level)
   SRC_NODE  *snode;
   int        level;
{
    SHAPE_NODE   *sh_node;
    int     dev, scrnX, scrnW, dataX, dataW, x1, x2, y;
    double  dv, dv0;
    struct ybar *barPr;

    x1 = snode->x1 + xDiff;
    x2 = snode->x2 + xDiff;
    if (x1 >= dpsX2 || x2 <= x_margin)
        return (1);
    dev = snode->device;
    if (snode->visible < 1) {
        if (x1 < x_margin)
           x1 = x_margin;
        if (x2 > dpsX2)
           x2 = dpsX2;
        dps_color(BCOLOR, 0);
        amove(x1, orgy[dev]);
        adraw(x2, orgy[dev]);
        return (1);
    }
    sh_node = snode->shapeData;
    if (sh_node == NULL || sh_node->amp == NULL || sh_node->data == NULL) {
        return (0);
    }
    dataW = sh_node->dataSize;
    scrnW = snode->x2 - snode->x1;
    if (scrnW < 2 || dataW < 2)
        return (0);
    if (ybarData == NULL || ybarSize < dpsX2 ) {
        if (ybarData != NULL) {
           free(ybarData);
           ybarData = NULL;
           ybarSize = 0;
        }
        ybarData = (struct ybar *) malloc(sizeof(struct ybar)*(dpsX2+4));
        if (ybarData == NULL)
           return (0);
        ybarSize = dpsX2;
    }

    if (sh_node->power != snode->power || sh_node->shapeType != rfShapeType)
        adjust_shaped_pulse_height(snode);
    // else if (snode->power == 0.0)
    //     snode->visible = 0;
    
    scrnX = x1;
    dataX = 0;
    if (x1 < x_margin || x2 > dpsX2) {
        if (x1 < x_margin ) {
           scrnX = x_margin;
           dv = (double) (x_margin - x1);
           dv0 = dv / (double) scrnW;
           if (dv0 < 0.0)
              dv0 = 0.0;
           if (dv0 > 0.99)
              dv0 = 0.99;
           dataX = (int) (dv0 * sh_node->dataSize);
           dataW = sh_node->dataSize - dataX;
           scrnW = x2 - scrnX;
        }
        if (x2 > dpsX2) {
           dv = (double) (dpsX2 + xOffset - x_margin - snode->x1);
           dv0 = dv / (double) (snode->x2 - snode->x1);
           if (dv0 <= 0.0)
              dv0 = 0.01;
           if (dv0 > 1.0)
              dv0 = 1.0;
           x2 = (int)(dv0 * sh_node->dataSize);
           dataW = x2 - dataX;
           scrnW = dpsX2 - scrnX - 1;
        }
    }

    if ((scrnX + scrnW) > dpsX2) {
         scrnW = dpsX2 - scrnX;
         if (scrnW < 2)
             return (0);
    }
    if (snode->visible < 1) {
        dps_color(BCOLOR, 0);
        amove(scrnX, orgy[dev]);
        adraw(scrnX+scrnW, orgy[dev]);
        return(1);
    }

    if (dataW >= scrnW)
        compress(sh_node->data+dataX, dataW, ybarData+scrnX, scrnW, orgy[dev]);
    else
        expand(sh_node->data+dataX, dataW, ybarData+scrnX, scrnW, orgy[dev]);
    ybars(scrnX, scrnX+scrnW, ybarData, 0, 0, 0);
    dev = snode->device;
    if (snode->updateLater > 0) {   // gradient shaped pulse
        y = orgy[dev];
        barPr = ybarData+scrnX;
        if (barPr->mn >= y)
            snode->y1 = barPr->mn;
        else
            snode->y1 = barPr->mx;
        barPr = ybarData+scrnX+scrnW-1;
        if (barPr->mn >= y)
            snode->y3 = barPr->mn;
        else
            snode->y3 = barPr->mx;
        return (1);
    }
    if (level > 0) {
        x1 = snode->x1 + xDiff;
        x2 = snode->x2 + xDiff;
        y = orgy[dev];
        if (x1 >= x_margin) {
             barPr = ybarData+scrnX;
             if (barPr->mn != y && barPr->mx != y) { 
                 amove(x1, y);
                 if (barPr->mn > y)
                     adraw(x1, barPr->mn - 1);
                 else if (barPr->mx < y)
                     adraw(x1, barPr->mx + 1); 
             }
        }
        if (x2 < dpsX2) {
             barPr = ybarData+scrnX+scrnW-1;
             if (barPr->mn != y && barPr->mx != y) { 
                 scrnX = x2 - 1;
                 amove(scrnX, y);
                 if (barPr->mn > y)
                     adraw(scrnX, barPr->mn - 1); 
                 else if (barPr->mx < y)
                     adraw(scrnX, barPr->mx + 1); 
             }
        }
    }
    
    return (1);
}

static int
draw_gradient_shape(node, level, twoShape, checkPower, solid)
   SRC_NODE  *node;
   int        level, twoShape, checkPower, solid;
{
    int  dev, x1, x2, h;

    dev = node->device;
    if (dev < 1)
       return(0);
    h = node->y2 - orgy[dev];
    if (h < 2)
        h = gradHeight / 3;
    node->y3 = orgy[dev]; 
    if (draw_rf_shape(node, level) > 0)
        return(1);
    x1 = node->x1 + xDiff;
    x2 = node->x2 + xDiff;
    if (x1 < x_margin)
       x1 = x_margin;
    if (x2 >= dpsX2)
       x2 = dpsX2 - 1;
    if (checkPower) {
        if (node->power <= 0.0)
            h = h / 2;
    }
    main_shaped(x1, orgy[dev], h, x2 - x1, solid);
    if (twoShape)
        main_shaped(x1, orgy[dev], h * 2 / 3, x2 - x1, solid);
    return (0);
}

static void
acquireWave(node, show_label, sx, dx)
  SRC_NODE  *node;
  int	    show_label, sx, dx;
{
	double  deg, rate, dr;
	int	chan;
	int	x1;
        int     y1, y2, i, m, k, h1, h2, width;

	x1 = sx;
	width = dx - sx;
/****
	if ( !show_label )
	{
	    if (x1 < x_margin)
		x1 = x_margin;
	}
	else if (x1 < 1)
	    x1 = 1;
****/
	chan = node->device;
        if (width >= pfx * 4)
	    rate = 0.7;
	else
	    rate = 0.6;
	y1 = orgy[chan];
        h1 = pulseHeight * rate;
        deg = 0.0;
	k = width / 8;
	h2 = h1;
	for(m = 0; m < k; m++) 
            h2 = h2 * rate;
	amove(x1, y1);
	dr = 6.28 / 4.0;
        for (i = 0; i < width / 2; i++)
        {
	   deg = deg + dr;
           if (deg >= 6.28)
           {   deg = 0.0;
	       k--;
	       h2 = h1;
	       for(m = 0; m < k; m++)
		   h2 = h2 * rate;
               y2 = y1;
	   }
           else
               y2 = y1 +  h2 * sin(deg);
	   if (x1 < x_margin)
                amove(x1, y2);
           else
                adraw(x1, y2);
	   x1++;
           if (x1 >= dpsX2)
                break;
	}
        for (i = 0; i < width / 2; i++)
        {
	   deg = deg + dr;
           if (deg >= 6.28)
           {   deg = 0.0;
               h1 = h1 * rate;
               y2 = y1;
	   }
           else
               y2 = y1 +  h1 * sin(deg);
	   if (x1 < x_margin)
                amove(x1, y2);
           else
                adraw(x1, y2);
	   x1++;
           if (x1 >= dpsX2)
                break;
	}
	if (x1 < dpsX2)
        	adraw(x1, y1);
	y1 = orgy[chan] + pulseHeight * rate;
	if (show_label)
	{
	    if (dispMode & TIMEMODE)
               disp_value(chan, sx, dx, y1, node->rtime, TIMEMODE);
	    else if (dispMode & PHASEMODE)
               disp_value(chan, sx, dx, y1, node->phase, PHASEMODE);
	}
}

static void
nvAcquire(node, num)
SRC_NODE  *node;
int num;
{
      int    loop, sx, dx, k, w, ws, x, x2, sy, y, step;
      double rx, sh, rh;
      double deg, roff, roff2, incD;

      sx = node->x1 + xDiff;
      dx = node->x2 + xDiff;
      w = dx - sx;
      if (num == 3)
	 w = w - 2;
      if (w < 3)
	 return;
      if (dpsPlot)
         step = 6;
      else
         step = 3;
      if (dx >= dpsX2)
         dx = dpsX2 - 1;
      while(1) {
         loop = w / step;
	 if (num == 3) {
	     if (loop >= 26)
	        break;
	 }
	 else {
	     if (loop >= 13)
	        break;
	 }
	 if (step > 1)
	    step--;
	 else
	    break;
      }
      sh = (double) pulseHeight * 0.6;
      if (sh < (double) (pfy * 4))
         sh = (double) pulseHeight * 0.7;
      if (step < 2)
         sh = (double) pulseHeight * 0.5;
      roff = 3.14 / 6.0;
      roff2 = roff * 0.5;
      x = sx;
      ws = w / 4;
      if (num == 3) {
	  if (ws < 1)
	     ws = 1;
          incD = (3.14 - roff) / (double) ws;
          x2 = sx + ws;
          rh = sh * 0.5;
	  y = rh * sin(roff);
	  while (roff2 < roff) {
	      sy = sh * sin(roff2);
	      if (sy > y)
		  break;
	      roff2 += 0.05;
	  }
      }
      else {
          incD = 3.14 / (double) w;
          x2 = dx;
          rh = sh;
      }
      deg = 0.0;
      sy = orgy[node->device];
      amove(sx, sy);
      for (loop = 0; loop < num; loop++) {
          if (x2 > x_margin) {
	      if (x < x_margin) {
                 rx = x_margin - x;
                 deg = deg + incD * rx;
		 x = x_margin;
      		 amove(x, sy);
	      }
	      while (x <= x2) {
          	  k = (int)(rh * sin(deg));
		  amove(x, sy - k);
		  rdraw(0, k + k);
		  x += step;
          	  deg += incD * (double)step;
	      }
	  }
	  else
	      x = x2;
	  if (num <= 1)
	      break;
	  if (loop == 0) {
              incD = (3.14 - roff2 * 2.0) / (double) (ws * 2);
	      deg = roff2 + incD * (x - x2);
	      x2 = x2 + ws * 2;
	      if (x2 > dx)
		x2 = dx;
              rh = sh;
	  }
	  else {
              incD = (3.14 - roff) / (double) ws;
	      deg = roff + incD * (x - x2);
	      x2 = dx - 1;
              rh = sh * 0.5;
	  }
      }
}

static void
mtAcquire(node, show_label, sx, dx, num)
  SRC_NODE  *node;
  int	    show_label, sx, dx, num;
{
	int   chan, x1, y1, y2, i, k, h;

	if (dx <= x_margin)
	    return;
    	nvAcquire(node, num);
	x1 = sx;
	if (x1 < x_margin)
	    x1 = x_margin;
	chan = node->device;
	if (show_label)
        {
            if (dispMode & TIMEMODE) {
		y1 = orgy[chan] + pulseHeight * 0.9;
                disp_value(chan, sx, dx, y1, node->rtime, TIMEMODE);
                dps_color(PRCOLOR, 0);
	    }
        }
	y1 = orgy[chan] + pulseHeight * 0.8;
	y2 = y1 -  spW / 2;
	if ((dx - x1) < spW)
	    return;
        if (dx < dpsX2 - 2) {
	   amove(dx - 1, orgy[chan]);
	   adraw(dx - 1, y2);
	}
	x1 = dx - spW * 2;
        amove(x1 + spW, y2);
        adraw(dx - 1, y2);
        amove(x1, y1);
        rdraw(spW, 0);
        rdraw(0, -spW);
        rdraw(-spW, 0);
        rdraw(0, spW);
        k = spW / 2;
        h = k / 2;
        for (i = 0; i < k; i++) {
            amove(x1 + spW, y2);
            rdraw(k, h);
            h--;
        }
        y2 = y1 - spW;
        amove(x1, y2);
        rdraw(k, spW);
        rdraw(k, -spW);
        amove(x1 + k / 2, y2 + k);
        rdraw(k, 0);
}

static void
spAcquire(node, show_label, sx, dx)
  SRC_NODE  *node;
  int	    show_label, sx, dx;
{
	int   w, x1, x2, y, y1, k, h, h2, up, step;
	double deg, incD;
	COMMON_NODE *comnode;

	if (dx <= x_margin)
	    return;
        up = 1;
	comnode = (COMMON_NODE *) &node->node.common_node;
        if (comnode->fval[3] >= comnode->fval[4])
           up = 0;
	x1 = node->x1 + xDiff;
	w = node->x2 - node->x1;
	h2 = pulseHeight * 2 / 10;
	if (up) {
	   h = pulseHeight * 4 / 10;
	   step = w / 3;
	   deg = 0.0;
	}
	else {
	   h = pulseHeight * 9 / 10;
	   step = w * 2 / 3;
	   deg = 3.14 * 0.35;
	}
	if (step < 2)
	   step = 2;
	incD = (3.14 - deg) / (double) step;
	k = 0;
	deg = 3.14;
	while (k < step) {
	   y =  (int)(h * sin(deg));
	   if (y >= h2) {
	      k--;
	      break;
	   }
	   k++;
	   deg = deg - incD;
	}
	step = step - k;
	if (up) 
	   deg = 0.0;
	else
	   deg = 3.14 * 0.35;
	x2 = x1 + step;
	if (step < 0)
	   step = 0;
        y = orgy[node->device];
	if (x2 > x_margin) {
	   k = 0;
	   if (x1 < x_margin) {
	        k = x_margin - x1;
		x1 = x_margin;
		deg = deg + incD * k;
	   }
	   y1 = y + (int)(h * sin(deg));
	   amove(x1, y1);
	   while (k < step) {
		deg += incD;
		y1 = y + (int)(h * sin(deg));
		adraw(x1, y1);
		x1++;
		k++;
	   }
	   x1--;
	}
	else
	   x1 = x2;
	y1 = y;
	if (up) {
	   h = pulseHeight * 9 / 10;
	   deg = 3.14 * 0.35;
	}
	else {
	   h = pulseHeight * 4 / 10;
	   deg = 0.0;
	}
	step = node->x2 + xDiff - x1;
	if (step < 2)
	   step = 2;
	incD = (3.14 - deg) / (double) step;
	deg = 0.0;
	k = 0;
	while (k < step) {
	   y1 =  (int)(h * sin(deg));
	   if (y1 >= h2) {
	       deg = deg - incD;
	       break;
	   }
	   deg = deg + incD;
	   k++;
	}
        if (up)
	   incD = (3.14 - deg - 3.14 * 0.35) / (double) step;
	else
	   incD = (3.14 - deg) / (double) step;
	if (x1 < x_margin) {
	   k = x_margin - x1;
	   x1 = x_margin;
	   deg = deg + incD * k;
	   y1 = y + (int)(h * sin(deg));
	}
	y1 = y + (int)(h * sin(deg));
	amove(x1, y1);
	while (x1 <= dx) {
	   y1 = y + (int)(h * sin(deg));
	   adraw(x1, y1);
	   deg += incD;
	   x1++;
	}
}


static void
draw_vgradient(x, x2, y, height, solid)
int  x, x2, y, height, solid;
{
	int    y2, gap, yt;

	if (x > x2)
	{
	    gap = x2;
	    x2 = x;
	    x = gap;
	}
	y2 = y - height;
	if (x >= x_margin)
	{
	    amove(x, y);
	    adraw(x, y2);
	}
	else
	    x = x_margin;
	amove(x2, y);
	adraw(x2, y2);
	gap = height / 5;
	if (gap < 3)
	    gap = 3;
	if (gap > 10)
	    gap = 10;
	yt = y;
	while (yt >= y2)
	{
	   if (solid)
	   {
	      amove(x, yt);
	      adraw(x2, yt);
	   }
	   else
	      dot_line(x, x2, yt, yt);
	   yt -= gap;
	}
}

static void
draw_prgoffset(x, x2, y, height, level)
int  x, x2, y, height, level;
{
        int    y2;

        if (x > x2)
        {
            y2 = x2;
            x2 = x;
            x = y2;
        }
        if (x2 < x)
            return;
        y2 = y + height;
        if (x >= x_margin)
        {
            amove(x, y);
            adraw(x, y2);
        }
        else
            x = x_margin;
        amove(x, y2);
        adraw(x2, y2);
        amove(x2, y);
        adraw(x2, y2);
        if (level > 0)
            color(FIRST_GRAY_COLOR + 38);
        else
            dps_color(HCOLOR, 1);
        amove(x + 1, y);
        box(x2 - x-1 , height - 1);
}


static void
LS_mark(x, yh, yl)
int   x, yh, yl;
{
     int k;
     for (k = 1; k < loopW; k++) {
	dot_line(x, x, yh, yl);
	dot_line(x, x + loopH, yh, yh + loopH);
	dot_line(x, x + loopH, yl, yl - loopH);
        x++;
     }
}

static void
RS_mark(x, yh, yl)
int   x, yh, yl;
{
     int k;
     for (k = 1; k < loopW; k++) {
	dot_line(x, x, yh, yl);
	dot_line(x, x - loopH, yh, yh + loopH);
	dot_line(x, x - loopH, yl, yl - loopH);
        x++;
     }
}

static void
LH_mark(x, yh, yl)
int   x, yh, yl;
{
     int k;
     for (k = 1; k < loopW; k++) {
	amove(x, yh);
	adraw(x, yl);
	amove(x, yh);
	adraw(x + loopH, yh + loopH);
	amove(x, yl);
	adraw(x + loopH, yl - loopH);
	x++;
     }
}

static void
RH_mark(x, yh, yl)
int   x, yh, yl;
{
     int k;
     for (k = 1; k < loopW; k++) {
	amove(x, yh);
	adraw(x, yl);
	amove(x, yh);
	adraw(x - loopH, yh + loopH);
	amove(x, yl);
	adraw(x - loopH, yl - loopH);
	x++;
     }
}

static void
draw_tune(node, show_label, sx, dx)
  SRC_NODE  *node;
  int	    show_label, sx, dx;
{
	int     w, h, x, y, x1, y1, steps;
	double  deg, incr, fw, fw2, fh, fh2;
        int     y2, x2, i, k;

	x = node->x1 + xDiff;
	y = orgy[node->device];
	w = node->x2 - node->x1;
	if (x+w <= x_margin)
             return;
	h = w / 2;
        x1 = x;
        x2 = x + h;
        y1 = y;
	fw = (double) h;
	if (h > pulseHeight)
           h = pulseHeight;
	steps = w * 2;
        incr = 3.14 / steps;
        deg = 3.14;
        if (x < x_margin)
        {
             i = x_margin - x;
	     i = i * 2;
             steps = steps - i;
             if (steps <= 0)
                return;
             deg = deg - incr * i;
             y1 = y + h * sin(deg);
             x1 = x2 + fw * cos(deg);
        }
	x = x2;
        amove(x1, y1);
        for (i = 0; i < steps; i++)
        {
             deg = deg - incr;
             x2 = x + fw * cos(deg);
             y2 = y + h * sin(deg);
	     if (x2 != x1 || y2 != y1) {
                adraw(x2, y2);
                x1 = x2;
                y1 = y2;
                if (x2 >= dpsX2)
                    break;
             }
        }
	steps = 4;
        incr = 3.14 / 4.0;
        deg = 3.14;
        fw2 = fw - spW / 4;
	if (fw2 < 4.0)
	   fw2 = fw;
	fh = h;
        fh2 = fh - spW / 4;
        for (i = 1; i < steps; i++)
        {
             deg = deg - incr;
             x2 = x + fw * cos(deg);
             y2 = y + fh * sin(deg);
             x1 = x + fw2 * cos(deg);
             y1 = y + fh2 * sin(deg);
	     if (x2 > x_margin) {
                amove(x2, y2);
                adraw(x1, y1);
             }
        }

	if (x < x_margin)
	    return;
	w = node->x2 - node->x1;
	k = h * 3 / 4;
	if (k > spW)
	    h = k;
	if (w > spW)
	    w = spW / 2;  
	else
	    w = w / 2;  
	if (w < 4)
	    w = 4;
	x1 = x - w;
	y1 = y + h;
	y2 = y1 - w;
	amove(x1, y2);
	adraw(x, y1);
	adraw(x + w, y2);
	amove(x1, y2);
	rdraw(w / 2, 0);
	amove(x + w / 2, y2);
	adraw(x + w, y2);
	x1 = x + w / 2;
	x2 = x - w / 2;
	amove(x2, y2);
	adraw(x2, y);
	adraw(x1, y);
	adraw(x1, y2);
}

static int
find_change_point(cnode, mode)
SRC_NODE	*cnode;
int		mode;
{
	int	    dev, sx, sx2, dx, pmode;
    	SRC_NODE    *dnode;

	dev = cnode->device;
	sx = cnode->x1;
	sx2 = cnode->x2;
	dx = cnode->x1;
	if (mode & POWERMODE)
	     pmode = 1;
	else
	     pmode = 0;
	dnode = cnode->dnext;
	while (dnode != NULL)
	{
	    if (dnode->device == dev)
	    {
		if (dnode->x1 >= sx2)
		{
		    dx = sx2;
		    break;
		}
		if (pmode)
		{
		    if (dnode->power != cnode->power)
		    {
			 dx = dnode->x1;
			 break;
		    }
		}
		else  /*  phase */
		{
		    if (dnode->phase != cnode->phase)
		    {
			 dx = dnode->x1;
			 break;
		    }
		}
	    }
	    dnode = dnode->dnext;
	}
	sx = dx + xDiff;
	return(sx);
}

/*
  d_level 0: draw shape only whith hilit color.
  d_level 1: draw shape and value whith normal color.
  d_level 2: draw shape only whith normal color.
*/

static void
draw_node(cnode, d_level)
SRC_NODE	*cnode;
int		d_level;
{
    int		dev, k;
    int		cx, cy, dx;
    int		d_link;
    SRC_NODE    *dnode;
    COMMON_NODE  *comnode;
    EX_NODE      *exnode;

	if (debug > 2) {
	    if (cnode->fname != NULL)
	       fprintf(stderr, "%d.%s %d,%d \n", cnode->id, cnode->fname, cnode->x1, cnode->x2);
	}
	if (cnode->flag & XMARK)
	{
	     if (!(dispMode & DSPMORE))
		return;
	}
	dev = cnode->device;
	if (visibleCh[dev] == 0)
	    return;
	d_link = 0;
	if (d_level > 0)
	{
	     if (d_level == 1)
	     {
		d_link = 1;
                if (cnode->updateLater <= 0)
                    update_chan(cnode);
	     }
	     dps_color(PCOLOR, 1);
	}
	else
             dps_color(HCOLOR, 1);
	cx = cnode->x1 + xDiff;
	if (cx >= dpsX2)
	     return;
	dx = cnode->x2 + xDiff;

	if (!(cnode->flag & XMARK))
	{
	     if (dx <= x_margin)
	     {
	        if (cnode->flag & XWDH) {
		    if (dx > chx[dev])
	    	       chx[dev] = dx;
		}
	        if (cnode->flag & XHGHT)
	            chyc[dev] = cnode->y3;
	        chMod[dev] = cnode->mode;
		return;
	     }
	}
	else if (dx < x_margin)
		return;
	if (dx >= dpsX2)
	     dx = dpsX2 - 1;
	if (cx < x_margin)
             cx = x_margin;
	cy = orgy[dev];
	comnode = (COMMON_NODE *) &cnode->node.common_node;
	switch (cnode->type) {
	case ACQUIRE:
	case SAMPLE:
		if (d_link) {
                    if (dev == TODEV)
		       delay_mark(cx - 1, 0, dx, dev);
                }
	        if (d_level > 0)
                    dps_color(ACQCOLOR, 0);
		if (d_link) {
                    dnode = cnode;
                    while (dnode != NULL)
                    {
                        if (dnode->device > 0) {
                           if (dnode != cnode) {
                              dnode->x1 = cnode->x1;
                              dnode->x2 = cnode->x2;
                              dnode->x3 = cnode->x3;
                              update_chan(dnode);
                           }
		           acquireWave(dnode, d_link, cx, dx);
                           if(dx > chx[dnode->device])
                              chx[dnode->device] = dx;
                        }
                        dnode = dnode->bnode;
                    }
                }
                else if (dev > 0)
		    acquireWave(cnode, d_link, cx, dx);
   
		break;
	case DELAY:
	case INCDLY:
	case VDELAY:
	case VDLST:
		cy = cnode->y2;
		if (d_level > 0) {
                    if (cy == orgy[dev])
                        dps_color(DCOLOR, 0);
                }
		if (cnode->rtime <= 0.0)
		    dot_line(cx, dx, cy, cy);
		else
		{
		    amove(cx, cy);
		    adraw(dx, cy);
		}
		if (cnode->type != DELAY)
		{
		    k = cx + (dx - cx) / 2 - 2;
		    amove(k, cy + 1);
		    rdraw(4, 0);
		    amove(k, cy - 1);
		    rdraw(4, 0);
		}
		// if (d_link && cy == orgy[dev])
		if (d_link)
                {
		    if (cy == orgy[dev])
		        delay_mark(cx, dx, dx, dev);
		    if (dispMode & TIMEMODE)
		    {
                          disp_value(dev,cx,dx,cy, comnode->fval[0], TIMEMODE);
		          disp_label(dev,cx,dx,orgy[dev],comnode->flabel[0]);
		    }
        	    else if (cnode->mode & DevOnMode)
		    {
		       if (dispMode & POWERMODE)
		       {
			  if (cnode->power != powerCh[dev])
			  {
                            disp_value(dev, cx, dx, cy,cnode->power,POWERMODE);
			     powerCh[dev] = cnode->power;
			  }
		       }
		       else if (dispMode & PHASEMODE && cnode->phase != rfPhase[dev])
		       {
                          disp_value(dev, cx, dx, cy, cnode->phase, PHASEMODE);
			  rfPhase[dev] = cnode->phase;
		       }
		    }
		}
	        if (d_level == 3) {
		    dps_color(PCOLOR, 0);
             	    if (cx > x_margin) {
                       amove(cx, cy - 3); 
                       rdraw(0, loopH);
		    }
                    amove(dx, cy - 3); 
                    rdraw(0, loopH);
		}
		break;
	case PULSE:
	case SMPUL:
		if (d_link)
		{
		    dnode = cnode;
		    while (dnode != NULL)
		    {
		        k = dnode->device;
			if (rfchan[k])
			{
		    	    dps_color(PCOLOR, 0);
			    draw_pulse(dnode, d_link, 0);
			    if(dx > chx[k])
			        chx[k] = dx;
			    chyc[k] = dnode->y3;
			    chMod[k] = dnode->mode;
			}
			dnode = dnode->bnode;
		        if (dnode != NULL)
			{
	     		     update_chan(dnode);
			}
		    }
/*
		    if (cx > chx[dev])
*/
		    delay_mark(cx - 1, 0, dx, dev);
		}
		else
		{
		     draw_pulse(cnode, d_link, 0);
		}
		break;		
	case SHPUL:
	case SHVPUL:
	case SMSHP:
	case APSHPUL:
	case OFFSHP:
		if (d_link)
		{
		    dnode = cnode;
		    while (dnode != NULL)
		    {
		        k = dnode->device;
			if (rfchan[k] != 0)
			{
			    dps_color(PCOLOR, 0);
			    draw_shape_pulse(dnode, d_link);
			    if(dx > chx[k])
			       chx[k] = dx;
			    chyc[k] = dnode->y3;
			    chMod[k] = dnode->mode;
			}
			dnode = dnode->bnode;
		        if (dnode != NULL)
			{
	     		     update_chan(dnode);
			}
		    }
		    delay_mark(cx - 1, 0, dx, dev);
		}
		else
		    draw_shape_pulse(cnode, d_link);
		break;		
	case GRPPULSE:
		if (d_link)
		{
		    dps_color(PCOLOR, 0);
		    draw_grpshaped(cnode, d_link);
		    delay_mark(cx - 1, 0, dx, dev);
		}
		else
		    draw_grpshaped(cnode, d_link);
		break;		
	case SHACQ:
	        dps_color(ACQUIRE, 0);
		if (d_link)
		    delay_mark(cx - 1, 0, dx, dev);
		mtAcquire(cnode, d_link, cx, dx, 3);
		break;		
        case SPACQ:
		if (d_link)
		    delay_mark(cx - 1, 0, dx, dev);
	        if (d_level > 0)
                    dps_color(PRCOLOR, 0);
		spAcquire(cnode, d_link, cx, dx);
		break;		
	case HLOOP:
		if ((newAcq<2) && d_level == 1 && (dispMode & TIMEMODE))
                {
                    disp_value(dev, cx, cx+loopW, cy+pulseHeight+loopH+
			 chdescent, (double)comnode->val[3], INTMODE);
                }
	        if (d_level > 0)
		    dps_color(HWLCOLOR, 0);
		else
                    dps_color(HCOLOR, 1);
	    	LH_mark(cx, cy+pulseHeight, cy);
		break;		
	case ENDHP:
		if (d_level > 0)
		    dps_color(HWLCOLOR, 0);
		RH_mark(cx, cy+pulseHeight, cy);
		break;		
	case SLOOP:
	case GLOOP:
	case MSLOOP:
	case PELOOP:
	case PELOOP2:
	case NWLOOP:
		if (d_level == 1 && (dispMode & TIMEMODE) && 
		    ((newAcq!=2) || (cnode->type == GLOOP)) )
                {
                    disp_value(dev, cx, cx+loopW, cy+pulseHeight+loopH+
			chdescent, (double)comnode->val[3], INTMODE);
                }
		if (d_level > 0)
		    dps_color(PELCOLOR, 0);
		else
                    dps_color(HCOLOR, 1);
		LS_mark(cx, cy+pulseHeight, cy);
		break;
	case ENDSP:
	case ENDSISLP:
	case NWLOOPEND:
		if (d_level > 0)
		    dps_color(PELCOLOR, 0);
		RS_mark(cx, cy+pulseHeight, cy);
		break;
	case STATUS:
	case SETST:
		if (dev == 0 && cx != dx)
		{
		    noStatus = 0;
		    cx = cnode->x1 + xDiff;
		    if (cx >= x_margin)
		    {
		    	amove(cx, statusY+3);
        		if (dpsPlot)
		    	    rdraw(0, -15);
			else
		    	    rdraw(0, -10);
		    }
		    else
			cx = x_margin;
		    dx = cnode->x2 + xDiff;
		    if (dx < dpsX2)
		    {
		    	amove(dx, statusY+3);
        		if (dpsPlot)
		    	    rdraw(0, -15);
			else
		    	    rdraw(0, -10);
		    }
		    else
			dx = dpsX2 - 1;
		    if (comnode->vlabel[0] != NULL)
		    {
			k = cx + (dx - cx - pfx) / 2;
			amove(k, statusY - pfy - 5);
			dps_color(STATCOLOR, 0);
			// dstring(comnode->vlabel[0]);
                        dps_string(k, statusY - pfy - 5, comnode->vlabel[0]);
		     }
		}
		if (dev > 0)
		{
/*
		     if (cnode->mode & AMPBLANK)
	     		k = 0;
		     else
*/
	     		k = 1;
		     if (cx != dx)
		     {
		        if (cnode->mode & ModuleMode)
	                   scrn_line(cx,dx,orgy[dev],cnode->y2,k);
			else
			{
			   if (d_level)
		  	   	dps_color(BCOLOR, 0);
                           dnode = fid_node;
                           while (dnode != NULL)
                           {
                              if (dnode->device == dev && dnode->active)
                                  break;
                              dnode = dnode->bnode;
                           }
                           if (dnode != NULL) {
                              k = cnode->x1 + xDiff;
                              if (dx > k)
                                  dx = k;
                           }
                           if (cx < dx) { 
			      amove(cx, cnode->y1);
			      adraw(dx, cnode->y1);
                           }
			}
		        if(d_link && comnode->val[1] && (dispMode & POWERMODE))
           	           disp_value(dev,cx,dx,cnode->y2,cnode->power,POWERMODE);
		        chyc[dev] = cnode->y2;
		     }
/**
		     else if (chyc[dev] > cnode->y2)
		     {
			amove(cx, cnode->y2);
			adraw(cx, chyc[dev]);
		     }
**/
		}
		break;
	case GRAD:
	case RGRAD:
	case MGPUL:
	case MGRAD:
		draw_gen_pulse(cnode, d_link);
		if (d_link && !(cnode->flag & NINFO))
		{
		     if(dispMode & POWERMODE)
		     {
			k = cnode->y2;
			if (k < orgy[dev])
			   k = orgy[dev];
           	        disp_value(dev,cx,dx,k,cnode->power,POWERMODE);
			if (cnode->type == RGRAD || cnode->type == GRAD)
	   	           disp_label(dev,cx,dx,orgy[dev],comnode->flabel[0]);
		     }
		     if ((dispMode & TIMEMODE) && (cnode->type == MGPUL))
		     {
			cy = cnode->y2;
			if (cy <= orgy[dev])
		     	    cy = orgy[dev] + gradHeight / 3;
           	        disp_value(dev, cx, dx, cy, comnode->fval[1], TIMEMODE);
		     }
		}
		break;
	case OBLGRAD:
		if (cx == dx)
		     break;
		draw_gen_pulse(cnode, d_link);
		if (d_link && !(cnode->flag & NINFO))
		{
		     if(dispMode & POWERMODE)
                     {
			k = cnode->y2;
			if (k < orgy[dev])
			   k = orgy[dev];
           		disp_value(dev,cx,dx,k,cnode->power,POWERMODE);
	   	        disp_label(dev,cx,dx,orgy[dev],comnode->flabel[0]);
                     }
		}
		break;

	case OBLSHGR:
	case SHGRAD:
	case PEOBLG:
		cy = cnode->y2;
/**
		if (cy <= orgy[dev])
		     cy = orgy[dev] + gradHeight / 3;
**/
		if (dx <= cx)
		     break;
                if (cnode->visible == 0) {  // pattern is empty
                     draw_rf_shape(cnode, d_link);
		     break;
                }
		if (cnode->rtime <= 0.0)
		     k = 0;
		else
		     k = 1;
                if (cnode->shapeData != NULL) {
                     cnode->visible = k;
                     if (draw_gradient_shape(cnode, d_link, 0, 0, k) > 0)
                         cy = orgy[dev] + gradHeight;
                }
                else {
		  if (cnode->flag & RAMPUP)
                  {
		     if ( k == 0 )
			dot_line(cx, dx, orgy[dev], cy);
		     else
		     {
			amove(cx, orgy[dev]);
			adraw(dx, cy);
		     }
		  }
		  else if (cnode->flag & RAMPDN)
                  {
		     if ( k == 0 )
                     {
			dot_line(cx, dx, cy, orgy[dev]);
		     }
		     else
		     {
			amove(cx, cy);
			adraw(dx, orgy[dev]);
		     }
		  }
		  else
		  {
		     main_shaped(cx, orgy[dev], cy - orgy[dev], dx - cx, k);
		     cy = orgy[dev] + gradHeight;
		  }
		}
		if (d_link && cnode->visible)
		{
		    if (hasDock)
		       delay_mark(cx, 0, dx, dev);
/*
		    else
		       delay_mark(cx, dx, dx, dev);
*/
			if (cy <= orgy[dev])
			     cy = orgy[dev];
			if (dispMode & TIMEMODE)
		        {
           	          disp_value(dev, cx, dx, cy, comnode->fval[0], TIMEMODE);
		          if (lineGap > 4)
	   		     disp_label(dev,cx,dx,orgy[dev],comnode->pattern);
		        }
		        else if (dispMode & POWERMODE)
		        {
           	            disp_value(dev,cx,dx,cy,cnode->power,POWERMODE);
		            if (lineGap > 4)
	   		       disp_label(dev,cx,dx,orgy[dev],comnode->flabel[1]);
		        }
		}
		break;
	case PESHGR:
	case SHVGRAD:
	case SHINCGRAD:
	case PEOBLVG:
	case DPESHGR:
                // h = cnode->y2 - orgy[dev];
		cy = cnode->y2;
                if (cnode->type == DPESHGR) {
                    dnode = cnode->bnode;
                    if (dnode != NULL) {
                       if (d_level > 0)
                           color(YELLOW);
                       dnode->x1 = cnode->x1;
                       dnode->x2 = cnode->x2;
                       dnode->x3 = cnode->x3;
                       draw_gradient_shape(dnode, d_link, 0, 0, 1);
                       if (d_level > 0)
                           dps_color(PCOLOR, 0);
                    }
                }
                if (comnode->flabel[2] != NULL && comnode->fval[2] != 0.0)
                { // gradient step > 0
                    if (draw_gradient_shape(cnode, d_link, 1, 1, 1) > 0)
		        cy = orgy[dev] + gradHeight;
                    /********
                    if (cnode->power > 0.0) {
		      main_shaped(cx, orgy[dev], h, dx - cx, 1);
		      main_shaped(cx, orgy[dev], h * 2 / 3, dx - cx, 1);
                    }
                    else {
		      main_shaped(cx, orgy[dev], h / 2, dx - cx, 1);
		      main_shaped(cx, orgy[dev], h / 3, dx - cx, 1);
                    }
                    ********/
                }
                else {
                    if (draw_gradient_shape(cnode, d_link, 0, 0, 1) > 0)
		        cy = orgy[dev] + gradHeight;
		    //   main_shaped(cx, orgy[dev], h, dx - cx, 1);
                }
		if (d_link && cnode->visible)
		{
		    if (hasDock)
		        delay_mark(cx, 0, dx, dev);
/*
		    else
		        delay_mark(cx, dx, dx, dev);
*/
		    if (dispMode & TIMEMODE)
		    {
           	       disp_value(dev, cx, dx, cy, comnode->fval[0], TIMEMODE);
                         
		       if (lineGap > 4) {
	                  if (cnode->type == DPESHGR)
                             disp_dual_label(dev,cx,dx,orgy[dev],cnode);
                          else
	   		     disp_label(dev,cx,dx,orgy[dev],comnode->pattern);
                        }
		     }
		     else if (dispMode & POWERMODE)
		     {
           	            disp_value(dev,cx,dx,cy,cnode->power,POWERMODE);
		     }
		}
		break;		
	case PEGRAD:
		if (dx > cx)
		{
		    k = cnode->y2;
		    if (k < orgy[dev])
			k = orgy[dev];
		    if (comnode->val[2] == 0)
		    {
			draw_gen_pulse(cnode, d_link);
			if (d_link && !(cnode->flag & NINFO))
			{
		            if (dispMode & POWERMODE)
			    {
           	               disp_value(dev,cx,dx,k,cnode->power,POWERMODE);
	   		       disp_label(dev,cx,dx,orgy[dev],comnode->flabel[0]);
			    }
			}
		    }
		    else
		    {
		        draw_vgradient(cx,dx,cnode->y2, gradHeight, 1);
			if (d_link && !(cnode->flag & NINFO))
			{
		            if (dispMode & POWERMODE)
           	               disp_value(dev,cx,dx,k,cnode->power,POWERMODE);
			}
		    }
		}
		break;		
	case PRGON:
	case XPRGON:
		k = cnode->x2 + xDiff;
		draw_vgradient(cx,k,cnode->y2, cnode->y2 - orgy[dev],1);
		break;		
        case VPRGON:
                k = cnode->x2 + xDiff;
                draw_prgoffset(cx,k, orgy[dev], cnode->y2 - orgy[dev], d_level);
                break;
	case PRGMARK:
	case VPRGMARK:
	     	if (d_level > 0)
		{
		    if (comnode->val[0])
			dps_color(MRKCOLOR, 0);
		    else
			dps_color(OFFCOLOR, 0);
		}
		draw_prg_mark(cnode, 0);
		break;		
	case VGRAD:
	case INCGRAD:
		if (dx > cx)
		{
		     draw_vgradient(cx,dx,cnode->y2, gradHeight,1);
/*
		     if (d_link && (dispMode & POWERMODE))
           	         disp_value(dev,cx,dx,cnode->y2, -1.0,TIMEMODE);
*/
		}
		break;		
	case ZGRAD:
		if (dx <= cx)
		     break;
		cy = cnode->y2;
                k = 0;
                if (gradshape[0] != 'y')
		    draw_gen_pulse(cnode, d_link);
                else {
                    // h = cnode->y2 - orgy[dev];
                    if (cnode->power >= 0.0)
                       k = draw_gradient_shape(cnode, d_link, 0, 0, 1);
                    else
                       k = draw_gradient_shape(cnode, d_link, 0, 0, 0);
                    /***
                    if (cnode->power >= 0.0)
		       main_shaped(cx, orgy[dev], h, dx - cx, 1);
                    else
		       main_shaped(cx, orgy[dev], h, dx - cx, 0);
                    ***/
                }
                if (k > 0)
		    cy = orgy[dev] + gradHeight;
		if (d_link && cnode->visible)
		{
/*
		   if (!hasDock)
		       delay_mark(cx, dx, dx, dev);
*/
		     cy = cnode->y2;
		     if (cy < orgy[dev])
			cy = orgy[dev];
		     if (dispMode & TIMEMODE)
		     {
           	          disp_value(dev, cx, dx, cy, comnode->fval[0], TIMEMODE);
		          if (lineGap > 4)
	   		     disp_label(dev,cx,dx,orgy[dev],comnode->flabel[0]);
		     }
		     else if (dispMode & POWERMODE)
		     {
           	          disp_value(dev,cx,dx,cy,cnode->power,POWERMODE);
		          if (lineGap > 4)
	   		     disp_label(dev,cx,dx,orgy[dev],comnode->flabel[1]);
		     }
		}
		break;		
	case DEVON:
		if (d_link)
		{
		    cy = cnode->y2;
		    cx = cnode->x1 + xDiff;
		    if (cx >= x_margin)
		    {
		     	amove(cx, cnode->y1);
		     	adraw(cx, cnode->y2);
		    }
		    else
			cx = x_margin;
		    if (dx < cx)
		    {
		        dx = cx;
		       	cnode->y3 = cnode->y1;
		    }
		    if (comnode->val[0])  /* device is on */
		    {
/**
		     	amove(cx, cnode->y2);
		     	adraw(dx, cnode->y2);
**/
		        if (!hasDock)
		            delay_mark(cx - 1, 0, dx, dev);
			rfPhase[dev] = -1.0;
			powerCh[dev] = -99.0;
			if (dx > cx && (dispMode & TIMEMODE) == 0)
			{
			   k = find_change_point(cnode, dispMode);
		       	   if (k > cx)
			   {
			     rfPhase[dev] = cnode->phase;
			     powerCh[dev] = cnode->power;
/*
			     if (rfType[dev] >= GEMINIH && k > cx)
*/
	                     if (cnode->type == DEVON) {
		       	       if (dispMode & POWERMODE)
			         disp_value(dev,cx,k,cy,cnode->power,POWERMODE);
		       	       else if (dispMode & PHASEMODE)
			         disp_value(dev,cx,k,cy,cnode->phase,PHASEMODE);
			     }
			   }
			}
			dx = cx;
		    }
		    else
		     	 adraw(dx, cnode->y2);
		    chyc[dev] = cnode->y3;
		}
		else   /*  highlighted */
		{
		    amove(cx, cnode->y2);
		    adraw(dx, cnode->y2);
		}
		break;
	case VDEVON:
		if (d_link)
		{
		    chyc[dev] = cnode->y3;
		    cy = cnode->y2;
		}
		break;
	case RFONOFF:
	     	if (d_level > 0)
		{
		    if (comnode->val[0])
			dps_color(ONCOLOR, 0);
		    else
			dps_color(OFFCOLOR, 0);
		}
		draw_rf_onoff(cnode, comnode->val[0]);
		break;
		      
	case PWRF:
	case PWRM:
	case RLPWRF:
	case VRLPWRF:
	case RLPWRM:
	     	if (d_level > 0)
		    dps_color(ONCOLOR, 0);
		draw_power(cnode, 0);
		break;
	case DECLVL:
	     	if (d_level > 0)
		{
		    if (comnode->val[0])
			dps_color(ONCOLOR, 0);
		    else
			dps_color(OFFCOLOR, 0);
		}
		draw_power(cnode, 1);
		break;
	case RLPWR:
	case DECPWR:
	case POWER:
	     	if (d_level > 0)
		    dps_color(ONCOLOR, 0);
		draw_power(cnode, 1);
		break;
	case OFFSET:
	case POFFSET:
	     	if (d_level > 0)
		    dps_color(MRKCOLOR, 0);
		draw_offset(cnode, 1);
		break;
	case LOFFSET:
	     	if (d_level > 0)
	     	    dps_color(MRKCOLOR, 0);
		draw_offset(cnode, 1);
		break;
	case VFREQ:
	case VOFFSET:
	     	if (d_level > 0)
		    dps_color(MRKCOLOR, 0);
		draw_offset(cnode, 0);
		break;
	case  LKHLD: 
	     	if (d_level > 0)
		    dps_color(MRKCOLOR, 0);
		draw_lock(cnode, 0, 1);
                break;
	case  LKSMP: 
		draw_lock(cnode, 0, 1);
                break;
	case AMPBLK:
	     	if (d_level > 0)
		{
		    if (comnode->val[0])  /* ampunblank */
		        dps_color(OFFCOLOR, 0);
		    else
		        dps_color(ONCOLOR, 0);
		}
		draw_lock(cnode, 1, comnode->val[0]);
	        chMod[dev] = cnode->mode;
                break;
	case  SPINLK: 
		if (dx > cx)
		{
	            spin_scrn_line(cx,dx,orgy[dev],cnode->y2);
		    if (d_link)
		    {
		        if (!hasDock)
		          delay_mark(cx - 1, 0, dx, dev);
		    }
		}
                break;
	case  XGATE:
	case  ROTORP: 
	case  ROTORS: 
	case  BGSHIM: 
	case  SHSELECT: 
	case  GRADANG: 
		if (d_link > 0)
		   delay_mark(cx -1, 0, dx, TODEV);
	     	if (d_level > 0) {
                   dps_color(PRCOLOR, 0);
		   COMMON_NODE *tmpnode;
		   tmpnode = (COMMON_NODE *) &cnode->node.common_node;
		   if ((cnode->type==BGSHIM) && (tmpnode->val[2]==0))
		      dps_color(OFFCOLOR, 0);
		}
		draw_square_mark(cnode, cnode->type);
		break;		
        case  ROTATEANGLE:
        case  EXECANGLE:
		if (d_link > 0)
		    delay_mark(cx -1, 0, dx, TODEV);
	     	if (d_level > 0)
                    dps_color(MRKCOLOR, 0);
		draw_square_mark(cnode, cnode->type);
		break;		
	case  FIDNODE: 
	case  JFID: 
		if (d_link > 0) {
                    if (dev == TODEV)
		        delay_mark(cx - 1, 0, dx, dev);
                    dnode = cnode;
                    while (dnode != NULL)
                    {
                        if (dnode->device > 0) {
                           if (dnode != cnode) {
                              dnode->x1 = cnode->x1;
                              dnode->x2 = cnode->x2;
                              dnode->x3 = cnode->x3;
                              update_chan(dnode);
                              if(dx > chx[dnode->device])
                                  chx[dnode->device] = dx;
                           }
                           fidWAVE(dnode, d_level);
                           // if(dx > chx[dnode->device])
                           //    chx[dnode->device] = dx;
                           chyc[dnode->device] = dnode->y3;
                        }
                        dnode = dnode->bnode;
                    }
                }
                else
		    fidWAVE(cnode, d_level);
		delay_mark(0, 0, dx, dev);
		break;		
	case SETRCV:
	     	if (d_level > 0)
		    dps_color(ONCOLOR, 0);
		draw_phase(cnode);
		break;		
	case  SPHASE: /* xmtrphase */
	     	if (d_level > 0)
		    dps_color(ONCOLOR, 0);
		draw_phase(cnode);
		break;		
	case  PSHIFT: /* phaseshift */
	     	if (d_level > 0)
		    dps_color(OFFCOLOR, 0);
		draw_phase(cnode);
		break;		
	case  PHASE: 
	     	if (d_level > 0)
		    dps_color(ONCOLOR, 0);
		draw_phase(cnode);
		break;		
	case SPON:
	     	if (d_level > 0)
		{
		    if (comnode->val[0])  /* sp_on */
		        dps_color(ONCOLOR, 0);
		    else
		        dps_color(OFFCOLOR, 0);
		    if (cnode->fname != NULL)
                    {
		        if ( ! strcmp(cnode->fname,"set_dmf") ||
		              ! strcmp(cnode->fname,"modulation") )
                        {
                              dps_color(MRKCOLOR, 0);
                        }
		    }
		}
		draw_spare(cnode);
                break;
	case RCVRON:
	     	if (d_level > 0)
		{
		    if (comnode->val[0])  /* on */
		        dps_color(ONCOLOR, 0);
		    else
		        dps_color(OFFCOLOR, 0);
		}
		draw_receiver(cnode, comnode->val[1]);
		break;		
	case PRGOFF:
	case XPRGOFF:
		break;
	case HDSHIMINIT:
	     	if (d_level > 0)
		    dps_color(ONCOLOR, 0);
		cx = cnode->x1 + xDiff;
	        amove(cx,cnode->y2-spH);
		rdraw(spW, 0);
		rdraw(0, spH);
		rdraw(-spW, 0);
		rdraw(0, -spH);
	        scrn_line(cx,cx+spW,cnode->y2-spH,cnode->y2, 1);
		if (cnode->flag & XNFUP)
		{
	    	    amove(cnode->x2+xDiff, cnode->y3 + 2);
	    	    rdraw(0, spH);
		}
		break;
	case PARALLEL:
/*
		if (tbug) {
			dps_color(OFFCOLOR, 0);
	    		amove(cx, 5);
	    		adraw(cx, mouse_y);
		}
*/
		break;
	case ROTATE:
	     	if (d_level > 0)
		    dps_color(ONCOLOR, 0);
		draw_rotate(cnode, d_level);
		break;
	case DCPLON:
		if (dev > 0)
                {
                     if (cx != dx)
                     {
	                exnode = (EX_NODE *) &cnode->node.ex_node;
                        scrn_line(cx,dx,orgy[dev],cnode->y2, exnode->cond);
                        if (d_link && (dispMode & POWERMODE))
                           disp_value(dev,cx,dx,cnode->y2,cnode->power,POWERMODE);
                     }
                     chyc[dev] = cnode->y2;
                }
		break;
	case DCPLOFF:
	case XDEVON:
		break;
	case XTUNE:
		if (d_link)
		    delay_mark(cx - 1, 0, dx, dev);
	     	if (d_level > 0)
                   dps_color(PRCOLOR, 0);
		draw_tune(cnode, d_link, cx, dx);
		break;		
        case XMACQ:
		if (d_link)
		    delay_mark(cx - 1, 0, dx, dev);
	     	if (d_level > 0)
                    dps_color(PRCOLOR, 0);
		mtAcquire(cnode, d_link, cx, dx, 1);
		break;		
	case ACQ1:
	     	if (d_level > 0)
		    dps_color(ONCOLOR, 0);
		draw_acq1(cnode, d_link);
		break;
	case TRIGGER:
	case SETGATE:
	case RDBYTE:
	case WRBYTE:
	     	if (d_level > 0)
		    dps_color(ONCOLOR, 0);
		draw_mrd(cnode);
		break;
	case SETANGLE:
	     	if (d_level > 0)
		    dps_color(ONCOLOR, 0);
		draw_mrd(cnode);
		break;
	case ACTIVERCVR:
                if (d_level > 0)
                {
                    dps_color(ONCOLOR, 0);
                }
                draw_receiver(cnode, 1);
		break;
	case  PARALLELSTART: /* draw */
	     	if (d_level > 0) {
                    parallelChan = dev;
		    if (d_link) {
                        delMark[dev] = dx + 4;
                    }
                }
		break;
	case  PARALLELEND: /* draw */
                dx += 2;
	     	if (d_level > 0) {
                    cnode->y2 = orgy[0] - 4;
                    cnode->y3 = statusY + 6;
		    dps_color(OFFCOLOR, 0);
		    if (d_link) {
                        add_parallel_search_node(cnode);
                        for(k = 0; k < TOTALCH; k++)
                             delMark[k] = dx;
                    }
                }
                amove(dx, cnode->y2);
                adraw(dx, cnode->y3);
                parallelChan = 0;
		break;
	case  XEND:
		break;
	case  XDOCK:
                dx++;
	     	if (d_level > 0) {
                    cnode->y2 = orgy[0] - 4;
                    cnode->y3 = statusY + 6;
		    dps_color(ONCOLOR, 0);
		    if (d_link)
                        add_parallel_search_node(cnode);
                }
                amove(dx, cnode->y2);
                adraw(dx, cnode->y3);
		break;
	case  PARALLELSYNC:
		if (d_level > 0)
		    dps_color(ONCOLOR, 0);
                draw_sync(cnode);
		break;
	case  RLLOOP:
	case  KZLOOP:
		if (d_level > 0)
		    dps_color(PELCOLOR, 0);
		else
                    dps_color(HCOLOR, 1);
		LH_mark(cx, cy+pulseHeight, cy);
		break;
	case  RLLOOPEND:
	case  KZLOOPEND:
		if (d_level > 0)
		    dps_color(PELCOLOR, 0);
		RH_mark(cx, cy+pulseHeight, cy);
		break;
	default:
		if (debug)
		    Wprintf("dps draw: unknown code %d \n", cnode->type);
		break;		
	}
	if (d_link)
	{
            if (cnode->updateLater > 0)
                update_chan(cnode);
	    if (cnode->flag & XWDH) {
		if(dx > chx[dev])
	    	   chx[dev] = dx;
	    }
	    if (cnode->flag & XHGHT)
	        chyc[dev] = cnode->y3;
	    chMod[dev] = cnode->mode;
	}
}


static void
draw_gen_pulse(node, flag)
   SRC_NODE  	*node;
   int		flag;
{
	int	 mode, x1, x2, x3, y1, y2;
	int	 solid, yd;

	solid = 1;
	mode = node->mode;
/*
	if (mode & AMPBLANK)
	     solid = 0;
*/
/*
	if (node->flag & NINFO)
	     dps_color(BCOLOR, 0);
*/
        // if (node->rtime <= 0.0)
        //      solid = 0;

	y1 = node->y1;
	y2 = node->y2;
	x1 = node->x1 + xDiff;
	x2 = node->x2 + xDiff;
	if (y1 != y2 && x1 >= x_margin)
	{
	      if (solid)
	      {
	 	  amove(x1, y1);
		  adraw(x1, y2);
	      }
	      else
		 dot_line(x1, x1, y1, y2);
	}
	if ( x1 == x2 )
	     return;
	if (x2 >= dpsX2)
	{
	     x2 = dpsX2 - 1;
	     x3 = 0;
	}
	else
	     x3 = 1;
	if (node->flag & NINFO)
	     dps_color(BCOLOR, 0);
	if (x1 < x_margin)
             x1 = x_margin;
	if (mode == 0)
	{
	      if (solid)
	      {
	 	  amove(x1, y2);
		  adraw(x2, y2);
		  if (x3)
		     adraw(x2, node->y3);
	      }
	      else
	      {
		 dot_line(x1, x2, y2, y2);
		 if (x3)
		     dot_line(x2, x2, y2, node->y3);
	      }
	}
	else if (mode & ModuleMode)
	{
	      if (y2 == orgy[node->device])
		   dot_line(x1, x2, y2, y2);
	      else
	           scrn_line(x1, x2, orgy[node->device], y2, solid);
	}
	else if (mode & VgradMode)
	{
	      yd = y2 - orgy[node->device];
	      draw_vgradient(x1, x2, y2, yd, solid);
	}
	else if (mode & DevOnMode)
	{
	      if (solid)
	      {
		  amove(x1, y2);
		  adraw(x2, y2);
	      }
	      else
		  dot_line(x1, x2, y2, y2);
	}
}

static void
update_chan(node)
   SRC_NODE  	*node;
{
	int	  chan, yd, x1, x2, y1, y2;
	int	  solid;
	SRC_NODE  *snode;

	chan = node->device;
	if (rfchan[chan] == 0)
	    return;
	if (node->flag & XMARK)
	    chan = 0;
	x2 = node->x1 + xDiff;
	y2 = node->y1;
	node->knext = NULL;
	node->pnode = NULL;
	if (node->type != DUMMY)
	{
	    if (info_node[chan] != NULL)
	    {
	    	snode = info_node[chan];
	    	while (snode->knext != NULL)
		    snode = snode->knext;
	   	snode->knext = node;
		node->pnode = snode;
	    }
	    else
	        info_node[chan] = node;
	}
	if (node->flag & XMARK) {
            if (node->type != PARALLELSYNC)
	        return;
        }

	if (x2 < x_margin)
	     return;

	x1 = chx[chan];
	if ( x2 < x1)
	     return;
	y1 = chyc[chan];
	solid = 1;
/*
	if (chMod[chan] & AMPBLANK)
	     solid = 0;
*/
	dps_color(PCOLOR, 0);
	if ((node->flag & XHGHT) && (y1 != y2))
	{
	     if (solid)
	     {
	 	  amove(x2, y1);
		  adraw(x2, y2);
	     }
	     else
		 dot_line(x2, x2, y1, y2);
	     chyc[chan] = y2;
	}
	if (!(node->flag & XWDH) || x2 <= x1)
	     return;
	if (chMod[chan] & VgradMode)
	{
	      yd = y1 - orgy[chan];
	      draw_vgradient(x1, x2, y1, yd, solid);
	}
	else 
	{
	      if (y1 == orgy[chan])
	      { 
		  if (chMod[chan] == 0 || node->type == XDEVON)
		      dps_color(BCOLOR, 0);
	      }
	      if (solid)
	      {
	 	  amove(x1, y1);
		  adraw(x2, y1);
	      }
	      else
		 dot_line(x1, x2, y1, y1);
	}
}



static void
draw_pulse(snode, show_value, sweep_pulse) 
   SRC_NODE  *snode;
   int	     show_value, sweep_pulse;
{
	int	     chan, x1, x2, xp1, xp2, y1, y2;
	int	     yp1, w, h, d, t, dx;
	double	     r;
	COMMON_NODE  *cnode;

	chan = snode->device;
	if (rfchan[chan] == 0)
	    return;
	cnode = (COMMON_NODE *) &snode->node.common_node;
	x2 = snode->x2 + xDiff;
	y2 = snode->y2;
	y1 = orgy[chan];
	x1 = snode->x1 + xDiff;
	if (x1 < x_margin)
	{
	    x1 = x_margin;
	    xp1 = 0;
	}
	else
	    xp1 = 1;
	if (x2 >= dpsX2)
	{
	     x2 = dpsX2 - 1;
	     xp2 = 0;
	}
	else
	     xp2 = 1;
	    
/*
	if ((cnode->fval[2] <= 0.0) || (snode->mode & AMPBLANK))
*/
	if (cnode->fval[2] <= 0.0)
	{
	    if (xp1)
                dot_line(x1, x1, y1, y2);
	    dot_line(x1, x2, y2, y2);
	    if (xp2)
	        dot_line(x2, x2, y2, y1);
	}
	else
	{
	    if (xp1)
            {
                amove(x1, y1);
                adraw(x1, y2);
            }
            else
                amove(x_margin, y2);
	    adraw(x2, y2);
	    if (xp2)
	        adraw(x2, y1);
	}
	if (sweep_pulse > 0) {
	    h = y2 - y1;
	    w = snode->x2 - snode->x1;
	    r = (double) h / (double) w;
	    if (r > 1.0)
	       dx = (double)spW / r;
	    else
	       dx = (double)spW * r;
	    if (dx < 2)
		dx = 2;
	    if (h > 4 || w > 4) {
	       d = spW;
	       t = w / d; 
	       while (t < 4) {
		   if (d <= 2)
		      break;
		   d--;
		   t = w / d;
	       }
	       while (t > 2) {
	           xp2 = h / t;
		   if (xp2 >= (spW / 2))
		      break;
		   d++;
		   t = w / d;
	       }
	       xp1 = snode->x1 + xDiff;
	       t = 0;
	       while (xp1 <= x1) {
		   t += d;
	           xp1 = snode->x1 + xDiff + t;
	       }
	       t = t - d;
	       if (sweep_pulse > 1)  /* downward */
	          yp1 = y2 - (double) t * r;
	       else
	          yp1 = y1 + (double) t * r;
               amove(x1, yp1);
	       t = t + d;
	       xp2 = x2 - dx;
	       while (xp1 < xp2) {
                   adraw(xp1, yp1);
	           if (sweep_pulse > 1)
	              yp1 = y2 - (double) t * r;
		   else
	              yp1 = y1 + (double) t * r;
		   adraw(xp1, yp1);
		   t += d;
	           xp1 += d;
	       }
	       adraw(xp2, yp1);
	       if (sweep_pulse > 1)
                   adraw(x2, y1);
	       else
                   adraw(x2, y2);
	    }
	    else {
	       xp1 = snode->x1 + xDiff;
	       if (sweep_pulse > 1) {
	           yp1 = y2 - h * (x1 - xp1) / (x2 - xp1);
		   amove(x1, yp1);
                   adraw(x2, y1);
	       }
	       else {
	           yp1 = y1 + h * (x1 - xp1) / (x2 - xp1);
		   amove(x1, yp1);
                   adraw(x2, y2);
	       }
	    }
	    y1 = orgy[chan];
	    y2 = snode->y2;
	    x2 = snode->x2 + xDiff;
	    if (x2 >= dpsX2)
	        x2 = dpsX2 - 1;
	    x1 = snode->x1 + xDiff;
	    if (x1 < x_margin)
	        x1 = x_margin;
	}
	if ( !show_value )
	    return;
	if (dispMode & TIMEMODE)
	{
              disp_value(chan, x1, x2, y2, cnode->fval[2], TIMEMODE);
	      disp_label(chan, x1, x2, orgy[chan],cnode->flabel[2]);
	}
	else if (dispMode & POWERMODE)
	{
/*
	      if (rfType[chan] >= GEMINIH)
*/
                  disp_value(chan, x1, x2, y2, snode->power, POWERMODE);
	}
	else if (dispMode & PHASEMODE)
	{
	      if (snode->type != SPACQ) {
                 disp_value(chan, x1, x2, y2, snode->phase, PHASEMODE);
	         disp_label(chan, x1, x2, orgy[chan],cnode->vlabel[0]);
	      }
	}
}


static void
sub_shaped(sx, sy, sh, sw, solid)
	int sx, sy, sh, sw, solid;
{
	double  deg, step;
	int     y2, x2, i, hop, gap, k;

	if (sx+sw <= x_margin)
             return;
	step = 3.14 / sw;
	deg = 3.14;
	x2 = sx;
	y2 = sy;
	if (sx < x_margin)
	{
	     i = x_margin - sx;
	     sw = sw - i;
	     if (sw <= 0)
		return;
	     deg = deg + step * i;
	     y2 = sy + sh * sin(deg);
	     x2 = sx + i;
	}
	amove(x2, y2);
        if (solid > 0)
        {
	    for(i = 0; i < sw; i++)
	    {
	    	x2++;
	   	deg = deg + step;
	    	y2 = sy + sh * sin(deg);
	    	adraw(x2, y2);
	    }
	}
	else
	{
	    if (dpsPlot)
	    {
	    	gap = pfx / 3;
	    	if (gap > 3)
			gap = 3;
		k = 0;
	    	for(i = 0; i < sw; i++)
	    	{
	   	    deg = deg + step;
		    hop = sy + sh * sin(deg);
		    if (k >= 0)
		    {
		    	amove(sx, hop);
		    	adraw(sx, hop);
		    	y2 = hop;
		    	sx = x2;
		    }
		    k++;
		    if (k == gap)
			k = -gap;
		    x2++;
	    	}
	    }
	    else {
	        gap = 3;
	    	for(i = 0; i < sw; i++)
	    	{
	   	    deg = deg + step;
		    hop = sy + sh * sin(deg);
		    if ((abs(hop - y2) >= gap) || (x2 - sx >= gap))	
		    {
		    	amove(sx, hop);
		    	adraw(sx, hop);
		    	y2 = hop;
		    	sx = x2;
		    }
		    x2++;
		}
	    }
	    sx = x2;
	}
}

static void
draw_shape_pulse(snode, level)
   SRC_NODE  *snode;
   int	      level;
{
	COMMON_NODE  *cnode;
	int	     chan, x1, x2, y1, y2, solid;
	int	     ws, wx, hs, hx, mx;

	chan = snode->device;
	if (rfchan[chan] == 0)
	    return;
	cnode = (COMMON_NODE *) &snode->node.common_node;
	if (strlen(cnode->pattern) == 1 && *cnode->pattern == '?')
	{
	    draw_pulse(snode, level, 0);
	    return;
	}
	x1 = snode->x1 + xDiff;
	x2 = snode->x2 + xDiff;
        if (x2 <= x_margin)
	    return;
        if (draw_rf_shape(snode, level) < 1) {
	   y1 = snode->y1;
	   y2 = snode->y2;
	   wx = (x2 - x1) / 2;
	   ws = (x2 - x1) / 4;
	   if (snode->mx > 0)
	      mx = snode->mx + xDiff;
	   else
	      mx = x1 + wx;
	   if (cnode->fval[2] <= 0.0)
	      solid = 0;
	   else
	      solid = 1;
	   hx = y2 - y1;
	   hs = pfy / 2;
	   sub_shaped(x1, y1, hs, ws, solid);
	   x1 = mx - ws;
	   wx = ws * 2 + 1;
	   main_shaped(x1, y1, hx, wx, solid);
	   x1 = x2 - ws;
	   sub_shaped(x1, y1, hs, ws, solid);
	   if (snode->type == OFFSHP) {
	       x1 = snode->x1 + xDiff + ws / 2;
	       x2 = snode->x2 + xDiff - ws / 2;
	       y1 = y1 + hx / 4;
	       y2 = y1 + hx / 2;
	       if (x1 < x_margin)
		   x1 = x_margin;
	       amove(x1, y1);
	       adraw(x2, y2);
	       y1 = snode->y1;
	       y2 = snode->y2;
	   }
	}
        else {
	   y1 = snode->y1;
	   y2 = snode->y2;
	}
	if ( level <= 0 || snode->visible < 1)
	    return;
	x1 = snode->x1 + xDiff;
	x2 = snode->x2 + xDiff;
	if (x1 < x_margin)
	    x1 = x_margin;
	if (dispMode & TIMEMODE)
	{
              disp_value(chan, x1, x2, y2, cnode->fval[2], TIMEMODE);
	      disp_label(chan, x1, x2, y1, cnode->pattern);
	}
	else if (dispMode & POWERMODE)
	{
/*
	      if (rfType[chan] >= GEMINIH)
*/
	    COMMON_NODE  *pnode;
	    pnode = (COMMON_NODE *) &snode->node.common_node;
	    if (newAcq==2 && snode->type==SHPUL)
	    {
	       double tmp;
	       // if ((int)pnode->fatt > 15) 
	       if ((int)pnode->fatt > 0) 
               {  tmp = pnode->catt +20.0 * log10(pnode->fatt/fineStep[chan]);
               }
               else tmp = pnode->catt - 48.2;
               disp_value(chan, x1, x2, y2, tmp, POWERMODE);
	    }
            else
	    {
                 disp_value(chan, x1, x2, y2, snode->power, POWERMODE);
	    }
                 
	}
	else if (dispMode & PHASEMODE)
	{
             disp_value(chan, x1, x2, y2, snode->phase, PHASEMODE);
	     disp_label(chan, x1, x2, y1, cnode->vlabel[0]);
	}
}

static void
draw_origin_grpshaped(SRC_NODE *snode)
{
        int          x1, x2, y1;
        int          ws, wx, hx;
        double  deg; 

        x1 = snode->x1 + xDiff;
        x2 = snode->x2 + xDiff;
        wx = x2 - x1;
/*
        if (snode->mx > 0)
           mx = snode->mx + xDiff;
        else
           mx = x1 + wx;
 */
        y1 = snode->y1;
        hx = pulseHeight / 2;
        main_shaped(x1, y1, hx, wx, 1);
        ws =  wx / 6;
        x1 = x1 + ws;
        deg = (double)ws * 3.14 / (double)wx;
        y1 = y1 + (double)hx * sin(deg);
        hx = pulseHeight / 3;
        wx =  wx - ws;
        main_shaped(x1, y1, hx, wx, 1);
        x1 = x1 + ws;
        deg = (double)ws * 3.14 / (double)wx;
        y1 = y1 + (double)hx * sin(deg);
        hx = pulseHeight / 4;
        main_shaped(x1, y1, hx, wx - ws, 1);
}

static void
draw_grpshaped(snode, show_label)
   SRC_NODE  *snode;
   int       show_label;
{
        COMMON_NODE  *cnode;
        int  x1, x2, y0, y1, dev;

        y0 = 0;
        if (draw_rf_shape(snode, show_label) > 0) {
            dev = snode->device;
            y1 = orgy[dev];
            y0 = pulseHeight / 8;
            if (y0 < 2)
                y0 = 2;
            orgy[dev] = y1 + y0;
            draw_rf_shape(snode, 0);
            orgy[dev] = y1;
        }
        else
            draw_origin_grpshaped(snode);
        if ( !show_label )
            return;
        if ((dispMode & TIMEMODE) == 0)
            return;
        cnode = (COMMON_NODE *) &snode->node.common_node;
        x1 = snode->x1 + xDiff;
        x2 = snode->x2 + xDiff;
        y1 = snode->y1;
        if (x1 < x_margin)
            x1 = x_margin;
        disp_value(1, x1, x2, y1 + pulseHeight + y0, snode->ptime, TIMEMODE);
        disp_label(1, x1, x2, y1, cnode->pattern);
}

/************************************************************************
*       draw sine wave to represent acquistion                          *
************************************************************************/
static void
fidWAVE(node, flag)
SRC_NODE  *node;
int	  flag;
{
	double deg, step;
	int    x, y, high, w;
        int    y2, i;
    	COMMON_NODE   *comnode;

	x = node->x1 + xDiff;
	y = node->y2;
	w = node->x2 - node->x1;
	high = pulseHeight * 0.6;
	if (pul_s_r > 0 && dec_v_r > 0)
	{
	    if (high >= (pul_s_r + dec_v_r) * pfy - 2)
		  high = (pul_s_r + dec_v_r) * pfy - 2;
	}
	comnode = (COMMON_NODE *) &node->node.common_node;
	if (flag == 1)
	{
	    if (dispMode & TIMEMODE) {
	       if (node->type == JFID)
                  disp_value(node->device, x, x+w, y+high, node->rtime, TIMEMODE);
	       else
                  disp_value(node->device, x, x+w, y+high, comnode->fval[0], TIMEMODE);
	    }
	    if (dispMode & PHASEMODE)
               disp_value(node->device, x, x+w, y+high, node->phase, PHASEMODE);
	}
	if (flag > 0)
	    dps_color(ACQCOLOR, 0);
	if (isHomo) {
	    if (x < x_margin)
		x = x_margin;
	    i = node->x2 + xDiff;
	    if (i > dpsX2)
		i = dpsX2;
	    scrn_line(x, i, y - high, y + high,1);
	}
        
        deg = 0.0;
	if (x >= x_margin)
	{
	   amove(x, y);
	   x = x + 2;
	   adraw(x, y);
	}
	step = (6.28 * 3.0) / (double) w;
	if (step < 0.314)
	    step = 0.314;
        for (i = 0; i < w; i++)
        {
	   deg = deg + step;
           if (deg > 6.28)
           {   deg = 0.0;
               high = high * 0.6;
               // y2 = y;
               continue;
	   }
           else
               y2 = y +  high * sin(deg);
	   if (x >= x_margin)
               adraw(x, y2);
	   else
               amove(x, y2);
	   x++;
           if (x >= dpsX2)
                break;
	}
}
	       

static void
dot_line(x1, x2, y1, y2)
int  x1, x2, y1, y2;
{
        int    i, gap, steps, xa, ya;
	double x, y, incrx, incry, len;

        if (x1 > x2)
	{
		i = x1;
		x1 = x2;
		x2 = i;
		i = y1;
		y1 = y2;
		y2 = i;
	}
	x = x2 - x1;
	y = y2 - y1;
	len = sqrt(x*x + y*y);
	if (dpsPlot)
	{
	    gap = pfx / 3;
	    if (gap > 5)
		gap = 5;
	}
	else
	    gap = 3;
	steps = (int)(len / gap);
	incrx = x / (double)steps;
	incry = y / (double)steps;
	i = 0;
	while(i < steps)
	{
	    i++;
	    xa = x1 + (int)(incrx * i + 0.5);
	    ya = y1 + (int)(incry * i + 0.5);
            amove(xa, ya);
	    adraw(xa, ya);
	}
	amove(x2, y2);
	adraw(x2, y2);
}



static void
scrn_line(xs, xk, ys, yk, solid)
int 	xs, xk, ys, yk, solid;
{
	int     xh, xw, yh, yw;
	int     dens;

	if (xk < xs || yk < ys)
	     return;
	if (yk == ys)
	{
	     if (solid)
	     {
	      	amove(xs, ys);
		adraw(xk, ys);
	     }
	     else
		dot_line(xs, xk, ys, ys);
	     return;
	}

	yh = yk - 2;
	xh = xs;
	if(dpsPlot)
		dens = pfx / 3;
	else
		dens = 4;
        if (!solid) {
	     xw = xs;
	     xh = 0;
	     while( yh >= ys) {
		while (xw <= xk) {
            	    amove(xw, yh);
	    	    adraw(xw, yh);
		    xw += dens;
		}
		yh -= dens;
		if (xh == 0) {
		    xw = xs + dens / 2;
		    xh = 1;
		}
		else {
		    xw = xs;
		    xh = 0;
		}
	     }
	     return;
	}
	while( yh >= ys)
	{
		amove(xs, yh);
		if ((yk - yh) < (xk - xh))
		{
		     xw = xh + yk - yh;
		     if (solid)
			adraw(xw, yk);
		     else
			dot_line(xs, xw, yh, yk);
		}
		else
		{
		     yw = yh + xk - xh;
		     if (solid)
			adraw(xk, yw);
		     else
			dot_line(xs, xk, yh, yw);
		}
		yh = yh - dens;
	}
	xh = xs + ys - yh;
	yh = ys;
	while( xh <= xk)
	{
		amove(xh, ys);
		if ((yk - ys) < (xk - xh))
		{
		     xw = xh + yk - ys;
		     if (solid)
			adraw(xw, yk);
		     else
			dot_line(xh, xw, ys, yk);
		}
		else
		{
		     yw = ys + xk - xh;
		     if (solid)
			adraw(xk, yw);
		     else
			dot_line(xh, xk, ys, yw);
		}
		xh = xh + dens;
	}
        if ((xh - xk) <= 2)
	{
		if (solid)
		{
		    amove(xk, ys);
		    adraw(xk, ys);
		}
		else
		{
		    dot_line(xk, xk, ys, ys);
		}
	}
}


static void
main_shaped(sx, sy, sh, sw, solid)
	int sx, sy, sh, sw, solid;
{
	double  deg, step, gap, dy;
	int     y2, x2, i, hop, diff, k;

	if (sx+sw <= x_margin)
             return;
	if (sw <= 2)
             return;
	step = 3.14 / (double)sw;
	deg = 0.0;
	x2 = sx;
	y2 = sy;
        dy = (double) sh;
	if (sx < x_margin)
	{
	     i = x_margin - sx;
	     sw = sw - i;
	     if (sw <= 0)
		return;
	     deg = step * (double)i;
	     y2 = sy + (int) (dy * sin(deg));
	     x2 = sx + i;
	}
	amove(x2, y2);
        if (solid > 0)  /*  solid line  */
        {
	    for (i = 0; i < sw; i++)
	    {
	   	deg += step;
	    	hop = (int) (dy * sin(deg));
	    	y2 = sy + hop;
		adraw(x2, y2);
	    	x2++;
		if (x2 >= dpsX2)
		    break;
	    }
            return;
	}
	else   /*  dot line  */
	{
	    if (dpsPlot) {
		gap = pfx / 3;
		if (gap > 5)
		    gap = 5;
		diff = 0;
		k = gap * 2;
	    	for(i = 0; i < sw; i++)
	    	{
	   	    deg = deg + step;
		    hop = sy + (int) (dy * sin(deg));
		    if (diff >= 0)
		    {
			amove(x2, hop);
			adraw(x2, hop);
		    }
		    x2++;
		    if (x2 >= dpsX2)
		    	break;
		    diff++;
		    if (diff > k)
			diff = 0 - gap;
	    	}
	    }
	    else {
		gap = 3;
	    	amove(sx, sy);
	    	adraw(sx, sy);
	    	for(i = 0; i < sw; i++)
	    	{
	   	    deg = deg + step;
		    hop = sy + (int) (dy * sin(deg));
		    diff = 0;
		    if ((abs(hop - y2) >= gap) || (x2 - sx > gap))	
		    {
/*
		    	if (dpsPlot && raster >= 1 && raster <= 2)
                    	{
                            if (hop > y2)
                                diff = 4;
                            else if (hop < y2)
                                diff = -4;
                            y2 = y2 + diff;
                            while (abs(hop - y2) > 3)
                            {
                             	amove(x2, y2);
                            	adraw(x2, y2);
                             	y2 = y2 + diff;
                            }
                        }
*/
			y2 = hop;
			sx = x2;
			amove(sx, y2);
			adraw(sx, y2);
		    }
		    x2++;
		    if (x2 >= dpsX2)
		    	break;
	    	}
	    }
	    sx = x2;
	}
}


static void
spin_scrn_line(xs, xk, ys, yk)
int 	xs, xk, ys, yk;
{
	int	xd, yd, gap;
	int	x1, y1, x2, y2;

	xd = xk - xs;
	yd = yk - ys;
	if(dpsPlot)
	    gap = pfx / 3;
	else
	    gap = 4;
	while (yd > 0)
	{
	    y1 = ys + yd;
	    amove(xs, y1);
	    if (yd > xd)
	    {
		x2 = xk;
		y2 = y1 - xd;
	    }
	    else
	    {
		y2 = ys;
		x2 = xs + yd;
	    }
	    adraw(x2, y2);
	    yd -= gap;
	}
	xd -= gap;
	yd = yk - ys;
	while (xd > 0)
	{
	    x1 = xk - xd;
	    amove(x1, yk);
	    if (xd > yd)
	    {
		x2 = x1 + yd;
		y2 = ys;
	    }
	    else
	    {
		y2 = yk - xd;
		x2 = xk;
	    }
	    adraw(x2, y2);
	    xd -= gap;
	}
	amove(xk, yk);
        rdraw(0, 0); 
}


#ifdef SUN

#ifdef  SUN

static void
menu_color(c)
int	c;
{
#ifndef  VNMRJ
	if (c == RED)
	   XSetForeground (dpy, dps_text_gc, dpsPix[FCOLOR]);
	else
	   XSetForeground (dpy, dps_text_gc, dpsPix[ICOLOR]);
#endif 
}

#endif 


static int
cal_name_width(name, min_scale)
char   *name;
double  min_scale;
{
	int     len, dx;
	double   incr_step;

	len = strlen(name);
	if (len <= 0)
	     return(0);
	if (plotW < wcmax || plotH < wc2max)
	     incr_step = 1.0;
	else
             incr_step = 1.2;
        charsize(incr_step);
#ifdef  SUN
        if (raster >= 1 && raster <= 2)
             dx = textWidth(name, 1);
	else
             dx = xcharpixels * len;
#else 
        dx = xcharpixels * len;
#endif 
        while (dpsW < dx)
        {
              incr_step = incr_step - 0.2;
              if (incr_step < min_scale)  /*  character size is too small */
                  break;
              charsize(incr_step);
#ifdef  SUN
              if (raster >= 1 && raster <= 2)
                  dx = textWidth(name, 1);
	      else
                  dx = xcharpixels * len;
#else 
              dx = xcharpixels * len;
#endif 
        }
	return(dx);
}



static void
disp_psg_name(message)
	char  *message;
{
	int     sx, sy;
	int     dx;
	char	*mess2;

        if(!dpsPlot || !plotName)
	     return;
	dx = cal_name_width(message, 0.6);
	if (dx > dpsW)
	{
	    if ((mess2 = strrchr(message, '/')) != NULL)
	    {
		message = mess2 + 1;
	    	dx = cal_name_width(message, 0.3);
	    }
	}
        sx = dpsX + (dpsW - dx) / 2;
        if (sx < 0)
              sx = 0;
	sy = dpsY + ycharpixels / 2;
        amove(sx, sy);
#ifdef VNMRJ
        if (dpsPlot)
            dstring(message);
        else {
            sx = dpsX + dpsW / 2;
            sun_dhstring(message, sx, sy);
        }
#else
        dstring(message);
#endif
        charsize(1.0);
}
#endif 

static void
info_print(char *format, ...)
{
        va_list  vargs;

        va_start(vargs, format);
        vsprintf(info_data, format, vargs);
        va_end(vargs);
#ifdef VNMRJ
	sprintf(inputs, "info add %s\n", info_data);
	writelineToVnmrJ("dps", inputs);
#else 
	put_dps_info(info_data);
#endif 
}


static void
disp_channel(int dev) {

    if (dev >= RFCHAN_NUM)
       return;
    if (dnstr[dev][0] == '0')
       info_print(" Channel: %d", dev);
    else {
       info_print(" Channel: %d (%s)", chanMap[dev], dnstr[dev]);
    }
}

static void
disp_pulse_info(cnode, shaped, ap)
SRC_NODE  *cnode;
int	  shaped, ap;
{
#ifdef SUN
	int	     dev;
        double        fv;
	COMMON_NODE  *pnode;
        SHAPE_NODE   *sh_node;

	pnode = (COMMON_NODE *) &cnode->node.common_node;
	dev = cnode->device;
	// info_print(" Channel: %d", dev);
        disp_channel(dev);
	if (shaped && *pnode->pattern != '?')
	    info_print(" Pattern: %s", pnode->pattern);
        if (cnode->shapeData != NULL) {
           sh_node = cnode->shapeData;
	   info_print("  Steps:      %d", sh_node->dataSize, inputs);
	   info_print("  Maximum:    %g", sh_node->maxData, inputs);
	   info_print("  Minimum:    %g", sh_node->minData, inputs);
        }
	modify_time_value(pnode->fval[2], 1);
	info_print(" Width:   %s = %s", pnode->flabel[2],inputs);
	if (newAcq > 1) {
           fv = (double) pnode->val[3] / 10.0;

	   info_print(" Phase set:   %s = %g",pnode->vlabel[0], modify_float_value(fv, 3));
        }
	else
	   info_print(" Phase set:   %s = %d",pnode->vlabel[0],pnode->val[3]);
        fv = fmodf(cnode->phase, 360.0);
	info_print(" Phase:       %g degree", modify_float_value(fv, 3));
        if ( (newAcq > 1) /* && (cnode->type==PULSE) */ )  {
	        info_print("    2D/3D/4D:  %s",pnode->flabel[6]);
		info_print("    Acq. Quad: %s",pnode->flabel[7]);
        }
	if (ap)
	{
	    info_print(" Power table:  %s", pnode->vlabel[1]); 
	    info_print(" Phase table:  %s", pnode->vlabel[2]); 
	}
	else
	{
/*
	    if (rfType[dev] >= GEMINIH)
*/
	    if (newAcq==2 && ! ap)
	    {
               if (cnode->type != SHACQ)
               {  info_print(" Power:   %g dB", modify_float_value(cnode->power, 3));
	          if ( ! strncmp(pnode->flabel[3],"Dbl_",4) )
	             info_print("    Coarse: %g ", pnode->catt);
                  else
                     info_print("    Coarse: %s = %g ",pnode->flabel[3],pnode->catt);
	          if (  ! strncmp(pnode->flabel[4],"Dbl_",4) )
	             info_print("    Fine:   %g ", pnode->fval[4]);
                  else
                     info_print("    Fine:   %s = %g ",pnode->flabel[4],pnode->fatt);
	          if (  ! strncmp(pnode->flabel[5],"Dbl_",4) )
	             info_print(" Offset:   %g ", pnode->fval[5]);
                  else if ( ! strcmp(pnode->flabel[5],"Unaltered") )
	             info_print(" Offset:   %s ", pnode->flabel[5]);
                  else
                     info_print(" Offset:   %s = %g ",pnode->flabel[5],pnode->fval[5]);
	       }
/************************
               else
               {  
	          double tmp;
                  if ((int)pnode->fval[3] > 15)
                  {  tmp = pnode->catt +20.0 * log10(pnode->fval[3]/fineStep[dev]);
                  }
                  else tmp = pnode->catt - 48.2;
                  info_print(" Power:  %.1f dB",tmp);
                  info_print(" Coarse Power:  %g ", pnode->catt);
                  if ( strcmp(pnode->flabel[3],"Unaltered") )
		     info_print(" Fine Power:    %g ", pnode->fval[3]);
		  else
		     info_print(" Fine Power:    %g ", pnode->fatt);
               
***********************/
	    }
            else
	    {
        	if (cnode->type == SHVPUL)   {
		   info_print(" Amp:   %s = %d", pnode->vlabel[1], pnode->val[5]);
		}
	        info_print(" Power:       %g dB", modify_float_value(cnode->power, 3));
	       	if (rfType[dev] >= UNITY)
		{
	          	info_print(" Coarse Power:  %g ", pnode->catt);
	          	if (fineStep[dev] > 1)
	              	    info_print(" Fine Power:    %g ", pnode->fatt);
	       	}
                if (cnode->type == OFFSHP) {
                     if (pnode->flabel[3] != NULL)
                        info_print(" Offset:        %s = %g ",pnode->flabel[3],pnode->fval[3]);
	       	}
	    }
	}
        if (cnode->type == SHACQ) {
	    modify_time_value(pnode->fval[3], 1);
	    info_print(" Pre-delay:  %s = %s", pnode->flabel[3], inputs);
	    return;
	}
	if (pnode->flabel[0][0] != '-')
        {
	   modify_time_value(pnode->fval[0], 1);
	   info_print(" Pre-delay:  %s = %s", pnode->flabel[0], inputs);
	}
	if (newAcq != 2)
	{
	   modify_time_value(pnode->fval[1], 1);
           info_print(" Post-delay: %s = %s", pnode->flabel[1], inputs);
	}
#endif 
}

static void
disp_grad_info(snode)
SRC_NODE     *snode;
{
#ifdef SUN
	COMMON_NODE  *cnode;
        SHAPE_NODE   *sh_node;

	cnode = (COMMON_NODE *) &snode->node.common_node;

	if (cnode->pattern)
	{
	    info_print(" Pattern:  %s", cnode->pattern);
            if (snode->shapeData != NULL) {
               sh_node = snode->shapeData;
	       info_print("  Steps:   %d", sh_node->dataSize, inputs);
	       info_print("  Maximum: %g", sh_node->maxData, inputs);
	       info_print("  Minimum: %g", sh_node->minData, inputs);
            }
/*
	    modify_time_value(cnode->fval[0], 1);
*/
	    modify_time_value(snode->rtime, 1);
	    info_print(" Width:    %s = %s", cnode->flabel[0],inputs);
/*
	    if (cnode->val[0] == 0)
		cnode->val[0] = 1;
            info_print(" Loops:    %s = %d", cnode->vlabel[0], cnode->val[0]);
            if (cnode->val[1] > 0)
              info_print(" Wait:   %s = %d", cnode->vlabel[1], cnode->val[1]);
*/
	}
	else
	     info_print(" Level:   %s = %g", cnode->flabel[0],
                                  modify_float_value(snode->power, 3));
#endif 
}

static void
disp_grad_xinfo(snode)
SRC_NODE     *snode;
{
#ifdef SUN
	COMMON_NODE  *cnode;

	cnode = (COMMON_NODE *) &snode->node.common_node;
	if (cnode->pattern)
	{
	    if (cnode->val[0] == 0)
		cnode->val[0] = 1;
            if (cnode->vlabel[0][0] != '0') {
               if (strcmp(cnode->vlabel[0], "1.00") != 0)
                  info_print(" Loops:    %s = %d", cnode->vlabel[0], cnode->val[0]);
            }
            if (cnode->vlabel[6] != NULL) {
               info_print(" Wait:     %s = %d", cnode->vlabel[6], cnode->val[6]);
            }
	}
#endif 
}

static void
disp_pe_grad_info(snode)
SRC_NODE  *snode;
{
#ifdef SUN
     COMMON_NODE  *comnode;
     COMMON_NODE  *comnode2;
     SRC_NODE    *snode2;
     SHAPE_NODE  *sh_node;
     char    **vlabel;
     char    **flabel;
     char    **vlabel2;
     char    **flabel2;


     comnode = (COMMON_NODE *) &snode->node.common_node;
     if (comnode == NULL)
        return;
     comnode2 = NULL;
     if (snode->type == DPESHGR) {
        // snode2 = snode->next;
        snode2 = snode->bnode;
        if (snode2 != NULL && snode2->type == DPESHGR2) {
            comnode2 = (COMMON_NODE *) &snode2->node.common_node;
            if (comnode2 != NULL) {
                 if (comnode2->pattern == NULL)
                      comnode2 = NULL;
            }
        }
     }
     if (comnode2 != NULL)
	info_print(" Patterns: %s, %s", comnode->pattern, comnode2->pattern);
     else
	info_print(" Pattern:  %s", comnode->pattern);
     if (snode->shapeData != NULL) {
        sh_node = snode->shapeData;
        info_print("  Steps:   %d", sh_node->dataSize, inputs);
        info_print("  Maximum: %g", sh_node->maxData, inputs);
        info_print("  Minimum: %g", sh_node->minData, inputs);
     }
     modify_time_value(snode->rtime, 1);
     info_print(" Width:    %s = %s", comnode->flabel[0],inputs);
     vlabel = comnode->vlabel;
     flabel = comnode->flabel;
     if (comnode2 != NULL) {
         flabel2 = comnode2->flabel;
         vlabel2 = comnode2->vlabel;
     }
     else {
         flabel2 = NULL;
         vlabel2 = NULL;
     }
     if (flabel[1] != NULL) {
        if (strcmp(flabel[1], "0.00") != 0)
            info_print(" Level:    %s = %g", flabel[1], comnode->fval[1]);
        else
            info_print(" Level:    0");
        if (flabel2 != NULL && flabel2[1] != NULL) {
            if (strcmp(flabel2[1], "0.00") != 0)
               info_print(" Level(2): %s = %g", flabel2[1], comnode2->fval[1]);
            else
               info_print(" Level(2): 0");
        }
     }
     if (flabel[2] != NULL) {
        if (strcmp(flabel[2], "0.00") != 0)
            info_print(" Step:     %s = %g", flabel[2], comnode->fval[2]);
        else
            info_print(" Step:     0");
        if (flabel2 != NULL && flabel2[2] != NULL) {
            if (strcmp(flabel2[2], "0.00") != 0)
                info_print(" Step(2):  %s = %g", flabel2[2], comnode2->fval[2]);
            else
                info_print(" Step(2):  0");
        }
     }
     if (vlabel[2] != NULL) {
        if (strcmp(vlabel[2], "0.00") != 0)
            info_print(" Mult:     %s = %d", vlabel[2], comnode->val[5]);
        else
            info_print(" Mult:     0");
        if (vlabel2 != NULL && vlabel2[2] != NULL) {
            if (strcmp(vlabel2[2], "0.00") != 0)
                info_print(" Mult(2):  %s = %d", vlabel2[2], comnode2->val[5]);
            else
                info_print(" Mult(2):  0");
        }
     }
     disp_grad_xinfo(snode);
#endif 
}

static void
disp_grad_name(cnode)
COMMON_NODE     *cnode;
{
#ifdef SUN
	int	k;

	if (image_flag & OBIMAGE || image_flag & MAIMAGE)
	{
            k = 9;
            if (*cnode->dname == 'x' || *cnode->dname == 'X')
                k = 0;
            else if (*cnode->dname == 'y' || *cnode->dname == 'Y')
                k = 1;
            else if (*cnode->dname == 'z' || *cnode->dname == 'Z')
                k = 2;
	    if (k >= 0 && k < 3)
	    {
	        info_print(" Channel:  %s", image_label[k]);
		return;
	    }
	}
	info_print(" Channel:  %s", cnode->dname);
#endif 
}

static void
disp_rfgroup_info(snode)
SRC_NODE  *snode;
{
      int n, k;
      COMMON_NODE  *cnode;
      RFGRP_NODE   *rnode, *rfroot;
      char         s[180];
      char         v[16];

      cnode = (COMMON_NODE *) &snode->node.common_node;
      n = cnode->val[0];
      
      rfroot = grp_start_node;
      while (rfroot != NULL) {
           if (rfroot->id == n)
                break;
           rfroot = rfroot->next;
      }
      if (rfroot == NULL) {
           info_print(" rfgroup not initiated");
           return;
      }
      info_print(" Pattern:  %s", rfroot->pattern);
      info_print(" Mode:     %s", rfroot->mode);
      modify_time_value(rfroot->width, 1);
      info_print(" Width:    %s = %s", rfroot->flabel[0], inputs);
      info_print(" Coarse Power:  %s = %g", rfroot->flabel[1], rfroot->coarse);
      info_print(" Fine   Power:  %s = %g", rfroot->flabel[2], rfroot->finePower);
      info_print(" Start  phase:  %s = %g", rfroot->flabel[3], rfroot->startPhase);
      info_print(" Phase  cycle:  %s = %d", cnode->vlabel[1], cnode->val[1]);
      info_print(" Freq   cycle:  %s = %d", cnode->vlabel[2], cnode->val[2]);
      info_print(" Freq   size:   %s = %d", rfroot->vlabel[0], rfroot->frqSize);
      info_print(" Freq   list:   %s", rfroot->flabel[4]);
      n = rfroot->val[0];
      k = 0;
      sprintf(s, "    list: ");
      while (k < n) {
          sprintf(v, "%g", rfroot->frqList[k]);
          strcat(s, v);
          k++;
          if (k > 5 || k >= n)
              break;
          strcat(s, ", ");
      }
      if (k < n)
          strcat(s, "... ");
      info_print("%s", s);
      rnode = rfroot->child;
      while (rnode != NULL) {
          info_print("  ");
          if (rnode->active == 0)
              info_print(" Channel %d was off", rnode->id);
          else {
              info_print(" Channel %d:  ", rnode->id);
              if (rnode->pattern != rfroot->pattern)
                  info_print("  Pattern: %s ", rnode->pattern);
              if (rnode->mode != rfroot->mode)
                  info_print("  Mode:     %s ", rnode->mode);
              if (rnode->coarse != rfroot->coarse)
                  info_print("  Coarse power: %s = %g ", rnode->flabel[1], rnode->coarse);
              if (rnode->finePower != rfroot->finePower)
                  info_print("  Fine power: %s = %g ", rnode->flabel[2], rnode->finePower);
              if (rnode->startPhase != rfroot->startPhase)
                  info_print("  Start phase: %s = %g ", rnode->flabel[3],  rnode->startPhase);
              if (rnode->newFrqList != 0)
                  info_print("  Freq list:    %s ", rnode->flabel[4]);
          }
          rnode = rnode->next;
      }
}

static int
print_rotatelist_data(int n, char *label, char *label2)
{
      ANGLE_NODE *node;
      double *f;
      int i, k, m;
      char s[180];
      char v[16];
      static char *l[] = {"x", "y", "z"};

      if (rotate_start_node == NULL)
          return 0;
      node = rotate_start_node;
      while (node != NULL) {
          if (node->id == n)
              break;
          node = node->next;
      }
      if (node == NULL)
          return 0;
      info_print(" Angle list name:  %s", node->name);
      info_print(" List  size:       %d", node->size);
      info_print(" Rotate index:     %s", label);
      info_print(" Mode:             %s", label2);
      info_print("   ");
      if (node->size < 1)
          return 0;
      m = node->size;
      if (m > 4)
          m = 4;
      for (k = 0; k < 3; k++) {
          f = node->angle_set[k];
          sprintf(s, " list  %s: ", l[k]);
          for (i = 0; i < m; i++) {
             sprintf(v, "%g, ", f[i]);
             strcat(s, v);
          }
          if (m < node->size)
             strcat(s, "...");
          info_print("%s", s);
      }
      return 0;
}

static void
print_angle_data(int angleId, int index)
{
     ANGLE_NODE *node;
     char s[180];
     char v[16];
     int  k, m;
     double *f;

     if (angle_start_node == NULL) {
         info_print(" no angle list");
         return;
     }
     node = angle_start_node;
     while (node != NULL) {
         if (node->id == angleId)
            break;
         node = node->next;
     }
     if (node == NULL) {  
         info_print(" no angle data");
         return;
     }
     info_print(" List size:  %d", node->size);
     info_print(" List data:");
     m = node->size;
     if (m > 6)
         m = 6;
     strcpy(s, "   ");
     f = node->angle_set[0];
     for (k = 0; k < m; k++) {
         sprintf(v, "%g", f[k]);
         strcat(s, v);
         if (k < (m-1))
            strcat(s, ", ");
     }
     if (m < node->size)
         strcat(s, " ...");
     info_print("%s", s);
}

static void
disp_info(cnode)
SRC_NODE     *cnode;
{
#ifdef SUN
	int	k, dev, *val;
	double   phase;
	double   fv;
	double   *fval;
	char	**vlabel;
	char	**flabel;
	COMMON_NODE  *comnode;
	INCGRAD_NODE *incgnode;
	EX_NODE      *exnode;

#ifndef VNMRJ
	if (shellWin == NULL)
	    return;
#endif 

	clear_dps_info();
	if (cnode == NULL)
	    return;
	menu_color(RED);
#ifdef VNMRJ
	if (cnode->fname != NULL)
	{
	    sprintf(inputs, "element   %s\n", cnode->fname);
            writelineToVnmrJ("dps", inputs);
	}
#else 
	if (cnode->fname != NULL)
	    info_print("%s",cnode->fname);
#endif 
/*
        if (cnode->mode & AMPBLANK)
	    info_print(" Channel %d amplifier is blanked", cnode->device);
*/
/*
        if (cnode->mode & DevOnMode)
	    info_print(" Channel %d is ON", cnode->device);
*/
	menu_color(BLUE);
	comnode = (COMMON_NODE *) &cnode->node.common_node;
	val = comnode->val;
	fval = comnode->fval;
	vlabel = comnode->vlabel;
	flabel = comnode->flabel;
	dev = cnode->device;
        phase = fmodf(cnode->phase, 360.0);
	switch (cnode->type) {
	case ACQUIRE:
	case JFID:
		if ((newAcq > 1) && (cnode->fname != NULL) && 
                      (!strncmp(cnode->fname,"explicit",8)) )
                {
		   info_print(" Dsp      = %s",vlabel[0]);
		   info_print(" Fsq      = %s",vlabel[1]);

		   info_print(" Sw       = %g",fval[4]);
		   info_print(" Oversamp = %g",fval[1]);
		   modify_time_value(fval[5],1);
		   info_print(" Rate     = %s",inputs);
		   modify_time_value(fval[3],1);
		   info_print(" Rof2     = %s",inputs);
		   info_print(" At       = %g",fval[2]);
		   modify_time_value(fval[0],1);
		   info_print(" Alfa     = %s",inputs);
		}
		else
		{
		   if ( ! strcmp(flabel[0],"''") )
                      info_print(" Points:     %g", fval[0]);
                   else
                      info_print(" Points:     %s = %g", flabel[0], fval[0]);
	 	   modify_time_value(fval[2], 1);
		   if ( ! strcmp(flabel[1],"''") )
		      info_print(" Interval:   %s", inputs);
		   else
		      info_print(" Interval:   %s = %s", flabel[1], inputs);
 /*
		   info_print(" Phase:    %s = %g", flabel[7], cnode->phase);
 */
		   info_print(" Phase set:  %s = %g", flabel[7], fval[7]);
		   info_print(" Phase:      %g degree", phase);
		}
		break;		
	case SAMPLE:
	 	modify_time_value(fval[0], 1);
		info_print(" Duration:   %s = %s", flabel[0], inputs);
		break;		
	case DELAY:
        	if (cnode->mode & DevOnMode)
		{
                    phase = fmodf(fval[1], 360.0);
		    info_print(" Phase:  %g degree", phase);
		    info_print(" Power:  %g dB", cnode->power);
	    	    if ((rfType[dev] >= UNITY) && (newAcq>1))
		    {
			info_print(" Coarse Power: %g ", comnode->catt);
	       		if (fineStep[dev] > 1)
			    info_print(" Fine Power:   %g ", comnode->fatt);
		    }
		}
	 	modify_time_value(fval[0], 1);
		info_print(" Time:   %s = %s", flabel[0], inputs);
		break;
	case INCDLY:
        	if (cnode->mode & DevOnMode)
		{
		    info_print(" Phase:  %g degree", phase);
		    info_print(" Power:  %g dB", cnode->power);
		}
		info_print(" Count:  %s = %d", vlabel[0], val[2]);
		info_print(" Index:  %s = %g", vlabel[1], fval[3]);
	 	modify_time_value(fval[0], 1);
		info_print(" Time:   %s", inputs);
		break;
	case VDELAY:
	case VDLST:
        	if (cnode->mode & DevOnMode)
		{
/*
		    info_print(" Power is %.2f dB", cnode->power);
		    info_print(" Phase is %.2f degree", cnode->phase);
*/
		}
		if (cnode->type == VDLST)
		{
		   if (flabel[2] != NULL)
		      info_print(" List(%d):   %s", val[1], flabel[2]);
		   else
		      info_print(" List(%d):   ???", val[1]);
		   info_print(" Index:      %s = %d", vlabel[0], val[2]);
		}
		else
		{
		   info_print(" Time base:  %s", vlabel[1]);
		   info_print(" Count:      %s = %d", vlabel[0], val[2]);
		}
	 	modify_time_value(fval[0], 1);
		info_print(" Delay time: %s", inputs);
		break;
	case PULSE:
	case SMPUL:
		disp_pulse_info(cnode, 0, 0);
		break;
	case SHPUL:
	case SHVPUL:
	case SMSHP:
	case OFFSHP:
		disp_pulse_info(cnode, 1, 0);
		break;
	case GRPPULSE:
		disp_rfgroup_info(cnode);
		break;
	case APSHPUL:
		disp_pulse_info(cnode, 1, 1);
		break;
	case DEVON:
	case RFONOFF:
		dev = cnode->device;
		if (val[0])
		{
/**
		     if (cnode->mode & PrgMode)
		     {
			menu_color(RED);
		        info_print(" Waveform generator is ON");
			if (flabel[4])
		          info_print(" Waveform pattern: %s", flabel[4]);
			menu_color(BLUE);
		     }
**/
		    info_print(" Set channel %d ON", dev);
		    info_print(" Phase:      %g degree", phase);
		    info_print(" Power:      %g dB", cnode->power);
	    	    if (dev < RFCHAN_NUM && rfType[dev] >= UNITY)
		    {
			info_print(" Coarse Power: %g ", comnode->catt);
	       		if (fineStep[dev] > 1)
			    info_print(" Fine Power:   %g ", comnode->fatt);
		    }
		}
		else
		     info_print(" Set channel %d OFF", dev);
		break;
		      
	case STATUS:
		dev = cnode->device;
                disp_channel(dev);
		k = (int) strlen(dmstr[dev]);
                if (k > val[0])
		    k = val[0];
                else
                    k = k-1;
		info_print(" Status(%s): %c ", vlabel[0], dmstr[dev][k]);
		if (val[1] && (dev > 0) && (dev < RFCHAN_NUM))
		{
		    if ( dev > TODEV)
		    {
			k = (int) strlen (dmmstr[dev]);
			if (k > val[0])
			   k = val[0];
			else
			   k--;
			info_print(" %s: %c", dmm_labels[dev], dmmstr[dev][k]);
			info_print(" %s: %g", dmf_labels[dev], dmf_val[dev]);
		    }
	    	    if (dev < RFCHAN_NUM && rfType[dev] >= UNITY)
	    	    {
		        info_print(" Power:        %g dB", cnode->power);
		        info_print(" Coarse Power: %g ", comnode->catt);
	       		if (fineStep[dev] > 1)
		            info_print(" Fine Power:   %g ", comnode->fatt);
	    	    }
		    else
		        info_print(" Power: %g dB", cnode->power);
		}
		break;
	case GRAD:
	case RGRAD:
	case MGRAD:
	case SHVGRAD:
	case SH2DVGR:
	case SHINCGRAD:
		disp_grad_name(comnode);
/*
		info_print(" Channel:  %s", comnode->dname);
*/
		disp_grad_info(cnode);
		disp_grad_xinfo(cnode);
		break;
	case MGPUL:
		disp_grad_name(comnode);
		disp_grad_info(cnode);
	 	modify_time_value(fval[1], 1);
		info_print(" Width: %s = %s", flabel[1],inputs);
		disp_grad_xinfo(cnode);
		break;
	case PESHGR:
	case DPESHGR:
		disp_grad_name(comnode);
                disp_pe_grad_info(cnode);
/**
		disp_grad_info(cnode);
                if (flabel[1] != NULL) {
                   if (strcmp(flabel[1], "0.00") != 0)
		      info_print(" Level:    %s = %g", flabel[1], fval[1]);
                   else
		      info_print(" Level:    0");
                }
                if (flabel[2] != NULL) {
                   if (strcmp(flabel[2], "0.00") != 0)
		       info_print(" Step:     %s = %g", flabel[2], fval[2]);
                   else
		       info_print(" Step:     0");
                }
                if (vlabel[2] != NULL) {
                   if (strcmp(vlabel[2], "0.00") != 0)
		       info_print(" Mult:     %s = %d", vlabel[2], val[2]);
                   else
		       info_print(" Mult:     0");
                }
**/
/*
                if (flabel[3] != NULL && fval[2] != 0.0) {
                   if (strcmp(flabel[3], "0.00") != 0)
		      info_print(" Limit:    %s = %g", flabel[3], fval[3]);
                }
		disp_grad_xinfo(cnode);
        if (flabel[4] != NULL)
		   info_print(" Angle:    %s = %g", flabel[4], fval[4]);
*/
		break;
	case SHGRAD:
		disp_grad_name(comnode);
		disp_grad_info(cnode);
/*
		info_print(" Amplitude: %s = %.1f", flabel[1], fval[1]);
*/
		info_print(" Level:    %g", fval[1]);
		disp_grad_xinfo(cnode);
		break;
	case VGRAD:
		disp_grad_name(comnode);
		info_print(" Intercept: %s = %d", vlabel[0], val[0]);
		info_print(" Slope:     %s = %d", vlabel[1], val[1]);
		info_print(" Mult:      %s = %d", vlabel[2], val[3]);
		info_print(" Level:     %g", cnode->power);
		break;
	case OBLSHGR:
		disp_grad_name(comnode);
		disp_grad_info(cnode);
		info_print(" Level:    %s = %g",flabel[1], fval[1]);
		disp_grad_xinfo(cnode);
		break;
	case PEOBLG:
		disp_grad_name(comnode);
		disp_grad_info(cnode);
		info_print(" Level:    %g", fval[1]);
		break;
	case PEOBLVG:
		disp_grad_name(comnode);
		disp_grad_info(cnode);
		info_print(" Level:    %g", fval[1]);
		info_print(" Mult:     %s = %d", flabel[1], val[3]);
		info_print(" Step:     %s = %g", flabel[3], fval[3]);
		info_print(" Incr:     %g", fval[2]);
		disp_grad_xinfo(cnode);
		break;
	case ZGRAD:
		// info_print(" Level:   %s = %g", flabel[1], fval[1]);
		info_print(" Level:   %s = %g", flabel[1], cnode->power);
		modify_time_value(fval[0], 1);
		info_print(" Width:   %s = %s", flabel[0],inputs);
		break;
	case OBLGRAD:
		disp_grad_name(comnode);
		info_print(" Level: %s = %g", flabel[0], fval[0]);
/*
		if (newAcq==0) 
                {  info_print(" Level:      %g", fval[1]);
		   info_print(" Angle: %s = %g", flabel[2], fval[2]);
                }
*/
		break;
	case PEGRAD:
		disp_grad_name(comnode);
		// info_print(" Level:   %s = %g", flabel[0],fval[0]);
		info_print(" Level:   %s = %g", flabel[0], cnode->power);
		info_print(" Step:    %s = %g", flabel[1],fval[1]);
		info_print(" Mult:    %s = %d", vlabel[0], val[1]);
/*
                if (flabel[3] != NULL)
		   info_print(" Angle:   %s = %g", flabel[3], fval[3]);
*/
		break;
	case POWER:
	case PWRF:
	case PWRM:
	case RLPWR:
	case RLPWRF:
	case VRLPWRF:
	case RLPWRM:
	case DECPWR:
		// info_print(" Channel:      %d", cnode->device);
                disp_channel(cnode->device);
		info_print(" Set:          %s = %g", flabel[0], fval[0]);
		info_print(" Power:        %g dB", cnode->power);
	    	if (rfType[cnode->device] >= UNITY)
		{
		   info_print(" Coarse Power: %g ", comnode->catt);
	       	   if (fineStep[cnode->device] > 1)
		       info_print(" Fine Power:   %g", comnode->fatt);
	  	}
		break;
	case DECLVL:
		if (fval[0])
		{
		    // info_print(" Channel:  %d", cnode->device);
                    disp_channel(cnode->device);
		    info_print(" Power:    %g dB", cnode->power);
		}
		break;
	case PHASE:
		// info_print(" Channel:   %d", cnode->device);
                disp_channel(cnode->device);
		if (newAcq == 2) 
                {
                   fv = (double) val[1] / 10.0;
                   info_print(" Set:       %s = %g", vlabel[0], fv);
		}
		else
		{  info_print(" Set:       %s = %d", vlabel[0], val[1]);
		}
                phase = fmodf(fval[1], 360.0);
 		info_print(" Phase:     %g", phase);
		break;
	case SETRCV:
                k = val[1] % 360;
		// info_print(" Phase:  %s = %d", vlabel[0], val[1]);
		info_print(" Phase:  %s = %d", vlabel[0], k);
		break;
	case SPHASE:
	case PSHIFT: /* phaseshift */
		// info_print(" Channel:  %d", cnode->device);
                disp_channel(cnode->device);
		if (cnode->type == PSHIFT)
		   info_print(" Base:     %s = %g", flabel[0], fval[0]);
		else
		   info_print(" Step:     %g", fval[0]);
		info_print(" Mult:     %s = %d", vlabel[0], val[1]);
                phase = fmodf(fval[1], 360.0);
		info_print(" Result:   %g degree", phase);
		break;
	case XGATE:
		info_print(" Number of events:  %s = %g", flabel[0], fval[0]);
		break;		
	case ROTATE:
	case ACQ1:
		for (k = 0; k < 6; k++) 
		{
		   if (flabel[k] != NULL)
		     info_print(" %s = %g", flabel[k], fval[k]);
		}
		break;		
	case SHSELECT:
		modify_time_value(fval[0], 1);
		info_print(" %s = %s", flabel[0], inputs);
		break;		
	case GRADANG:
		info_print("Psi   = %g",fval[0]);
		info_print("Phi   = %g",fval[1]);
		info_print("Theta = %g",fval[2]);
		
		break;
	case BGSHIM:
		info_print(" Shim list: %s",vlabel[1]);
		break;
	case PRGOFF:
		info_print(" End programmable decoupling on channel %d.", cnode->device);
		break;
	case PRGON:
	case XPRGON:
	case PRGMARK:
	        if (cnode->type == PRGMARK) {
		    if (!val[0]) {
		       if ((newAcq > 1) && (cnode->fname != NULL) && (cnode->fname[0] != 'd'))
		          info_print(" Duration:  0.450 usec");
	               return;
		    }
                }
		// info_print(" Channel:    %d", cnode->device);
                disp_channel(cnode->device);
		if ( newAcq != 2) {
                   info_print(" Pattern:        %s", flabel[0]);
		   modify_time_value(fval[1], 1);
		   info_print(" 90_pulselength: %s = %s", flabel[1],inputs);
		   info_print(" Resolution:     %s = %g", flabel[2], fval[2]);
                }
		else
		{  if (cnode->fname != NULL && cnode->fname[0] == 'd')
		   {
		    info_print(" Pattern:    %s", flabel[0]);
		    info_print(" Resolution: %s = %g", flabel[1],fval[1]);
		   }
		   else
		    info_print(" Duration:   0.450 usec");
		}
	    	if (rfType[cnode->device] >= UNITY)
		{
		   info_print(" Power:        %g dB", cnode->power);
		   info_print(" Coarse Power: %g ", comnode->catt);
	       	   if (fineStep[cnode->device] > 1)
		       info_print(" Fine Power:   %g", comnode->fatt);
	  	}
		break;
	case VPRGMARK:
        case VPRGON:
                // info_print(" Channel:          %d", cnode->device);
                disp_channel(cnode->device);
                info_print(" Pattern:          %s", flabel[0]);
                modify_time_value(fval[1], 1);
                info_print(" 90-deg length:    %s = %s", flabel[1],inputs);
                info_print(" Angle resolution: %s = %g", flabel[2], fval[2]);
                info_print(" Freq. offset:     %s = %g Hz", flabel[3], fval[3]);
                break;
	case SLOOP:
	case GLOOP:
		if (newAcq==2) {
		   info_print(" Loop increment:      %s",vlabel[1]);
		   info_print(" Loop condition:      %s",vlabel[3]);
                }
		else {
		   info_print(" Number of loops: %s = %d", vlabel[3], val[3]);
		   info_print(" Loop counter:    %s", vlabel[1]);
                }
		break;
	case HLOOP:
        case MSLOOP:
        case PELOOP:
        case PELOOP2:
		if (newAcq==2)
		   info_print(" Loop condition:      %s",vlabel[3]);
		else {
		   info_print(" Number of loops:  %s = %d", vlabel[3], val[3]);
		   // info_print(" Index:         %s", vlabel[2]);
                }
		break;
        case ENDSISLP:
                info_print(" Loop counter:  %s ", vlabel[1]);
		break;
        case NWLOOP:
		info_print(" Number of loops: %s = %d", flabel[0], (int)fval[0]);
                info_print(" Loop counter:    %s ", vlabel[1]);
		break;
        case NWLOOPEND:
                info_print(" Loop counter:  %s ", vlabel[0]);
		break;
	case SETST:
		// info_print(" Channel: %d", cnode->device);
                disp_channel(cnode->device);
		if (val[1])
		{
		     info_print(" Status: On");
		     info_print(" Mode:   %s", vlabel[0]);
		     if(val[2])
			info_print(" Sync:   On");
		     else
			info_print(" Sync:   Off");
		     info_print(" Frequency: %s = %g", flabel[0], fval[0]);
		}
		else
		     info_print(" Status:  Off");
		break;
	case OFFSET:
		// info_print(" Channel:   %d", cnode->device);
                disp_channel(cnode->device);
		info_print(" Set:       %s = %g", flabel[0], fval[0]);
		info_print(" Frequency offset %g Hz", fval[0]);
		break;
	case FIDNODE:
		menu_color(RED);
		info_print("Acquisition");
		menu_color(BLUE);
	 	modify_time_value(fval[0], 1);
		info_print(" Time:   at = %s", inputs);
                k = val[1] % 360;
		info_print(" Phase:  %s = %d", flabel[1], k);
		// info_print(" Phase:  %s = %d", flabel[1], val[1]);
		if (isHomo)
		    info_print(" homo       = 'y'");
                if (dev > 1 && rcvrsNum > 1) {
		   info_print(" numrcvrs is %d", rcvrsNum);
		   info_print(" rcvrs is '%s'", rcvrsStr);
                }
		break;
	case  SPINLK:  /* spinlock */
		info_print(" Pattern:    %s", vlabel[0]);
	 	modify_time_value(fval[0], 1);
		info_print(" Width:      %s = %s", flabel[0], inputs);
                k = val[3] % 360;
		info_print(" Phase:      %s = %d", vlabel[1], k);
		// info_print(" Phase:      %s = %d", vlabel[1], val[3]);
		info_print(" Resolution: %s = %g", flabel[1], fval[1]);
		info_print(" Cycles:     %s = %d", vlabel[2], val[2]);
		info_print(" Power:      %g dB", cnode->power);
		dev = cnode->device;
	    	if (rfType[dev] >= UNITY)
		{
		    info_print(" Coarse Power: %g ", comnode->catt);
	       	    if (fineStep[dev] > 1)
			info_print(" Fine Power:   %g ", comnode->fatt);
		}
		break;
	case  POFFSET: 
		// info_print(" Channel:   %d", cnode->device);
                disp_channel(cnode->device);
		info_print(" Position:  %s = %g", flabel[0], fval[0]);
		info_print(" Level:     %s = %g", flabel[1], fval[1]);
		info_print(" Offset:    %s = %g Hz", flabel[2], fval[2]);
		break;
	case  LOFFSET: 
		// info_print(" Channel:   %d", cnode->device);
                disp_channel(cnode->device);
		info_print(" Position:  %s (array)", flabel[0]);
		info_print(" Level:     %s = %g", flabel[1], fval[1]);
		info_print(" Slices:    %s = %g", flabel[2], fval[2]);
		info_print(" Offset:    %s = %g Hz", flabel[3], fval[3]);
		info_print(" Lists:     %s = %d ", vlabel[0], val[0]);
		info_print(" APindex:   %s = %d ", vlabel[1], val[2]);
		break;
	case  ROTORP:
	case  ROTORS:
		info_print(" Set:   %s = %d", vlabel[0], val[1]);
		break;
	case VFREQ: 
		if (flabel[2] != NULL)
		   info_print(" List(%d):  %s", val[0], flabel[2]);
		else
		   info_print(" List:  %s = %d", vlabel[0], val[0]);
	  	info_print(" Index: %s = %d", vlabel[1], val[2]);
		info_print(" Frequency:  %g", fval[0]);
		break;
	case VOFFSET: 
		if (flabel[2] != NULL)
		   info_print(" List(%d):   %s", val[0], flabel[2]);
		else
		   info_print(" List:    %s = %d", vlabel[0], val[0]);
		info_print(" Index:   %s = %d", vlabel[1], val[2]);
		info_print(" Offset:  %g", fval[0]);
		break;
	case INCGRAD:
		incgnode = (INCGRAD_NODE *) &cnode->node.incgrad_node;
		val = incgnode->val;
		vlabel = incgnode->vlabel;
		flabel = incgnode->flabel;
		info_print(" Channel:     %s", incgnode->dname);
		info_print(" Base:        %s = %d", vlabel[0], val[0]);
		info_print(" Incr params: %s, %s, %s", vlabel[1],vlabel[2],vlabel[3]);
		info_print(" Incr values: %d, %d, %d", val[1],val[2],val[3]);
		info_print(" Mult params: %s, %s, %s", vlabel[4],vlabel[5],vlabel[6]);
		info_print(" Mult values: %d, %d, %d", val[7],val[8],val[9]);
		info_print(" Level:       %g", cnode->power);
		break;
	case SPON:
		if ((cnode->fname != NULL) && (!strcmp(cnode->fname,"set_dmf")))
		{  info_print(" Set:   %s = %g", flabel[0], fval[0]);
		}
		else if ((cnode->fname != NULL) && (!strcmp(cnode->fname,"modulation")))
		{  info_print(" Set:   %s = %s", flabel[0], flabel[1]);
		}
		else if (val[0])
		     info_print(" Spare line %s is ON", vlabel[0]);
		else
		     info_print(" Spare line %s is OFF", vlabel[0]);
		break;
	case RCVRON:
		if (val[0])
		     info_print(" Turn on receiver");
		else
		     info_print(" Turn off receiver");
		break;
	case DCPLON:
	        exnode = (EX_NODE *) &cnode->node.ex_node;
		flabel = exnode->exlabel;
		vlabel = exnode->exval;
		for (k = 0; k < exnode->exnum; k++)
		     info_print(" %s:  %s ", flabel[k], vlabel[k]);
		val = exnode->val;
		vlabel = exnode->vlabel;
		for (k = 0; k < exnode->vnum; k++)
		     info_print(" %s = %d", vlabel[k], val[k]);
		fval = exnode->fval;
		flabel = exnode->flabel;
		for (k = 0; k < exnode->fnum; k++)
		     info_print(" %s = %g", flabel[k], exnode->fval[k]);
		break;
	case  XTUNE: 
		// info_print(" Channel:   %d", cnode->device);
                disp_channel(cnode->device);
		info_print(" %s = %g", flabel[0], fval[0]);
		break;
        case XMACQ:
		// info_print(" Channel:   %d", cnode->device);
                disp_channel(cnode->device);
		info_print(" %s = %d", vlabel[0], val[0]);
		for (k = 0; k < 5; k++) 
		{
		   if (flabel[k] != NULL)
		     info_print(" %s = %g", flabel[k], fval[k]);
		}
		break;
        case SPACQ:
		// info_print(" Channel: %d", cnode->device);
                disp_channel(cnode->device);
		info_print(" Steps:   %s = %d", vlabel[0], val[0]);
                info_print(" Start:   %s = %g", flabel[3], fval[3]);
                info_print(" End:     %s = %g", flabel[4], fval[4]);
                if (flabel[7] != NULL)
                   info_print(" Offset:  %s = %g", flabel[7], fval[7]);
		info_print("          %s = %g", flabel[5], fval[5]);
		info_print("          %s = %g", flabel[6], fval[6]);
		break;
	case SHACQ:
		disp_pulse_info(cnode, 1, 0);
		for (k = 5; k < 7; k++)
                {
                   if (flabel[k] != NULL)
                     info_print(" %s = %g", flabel[k], fval[k]);
                }
		break;
	case TRIGGER:
		info_print(" Input line:   %s = %d", vlabel[0], val[0]);
		break;
	case RDBYTE:
		info_print(" Data at:   %s ", vlabel[0]);
	 	modify_time_value(fval[0], 1);
		info_print(" Duration:  %s = %s", flabel[0], inputs);
		break;
	case WRBYTE:
	case SETGATE:
		info_print(" Data:   %s = %d", vlabel[0], val[1]);
		break;
	case SETANGLE:
		info_print(" Name:   %s", vlabel[1]);
		info_print(" Mode:   %s", vlabel[2]);
		info_print(" Index:  %s = %d", vlabel[3], val[5]);
		print_angle_data(val[0], val[5]);
		break;
	case ROTATEANGLE:
		print_rotatelist_data(val[1], vlabel[2], vlabel[0]);
		break;
	case EXECANGLE:
		// print_angle_data();
		break;
	case ACTIVERCVR:
                if (comnode->dname != NULL)
		   info_print(" Set: %s = '%s'", comnode->dname, comnode->pattern);
		info_print("      numrcvrs is %d", rcvrsNum);
		break;
	case RLLOOP:
                info_print(" Number of loops:  %s = %d", vlabel[0], val[0]);
                // info_print(" Real time variable: %s ", vlabel[1]);
                info_print(" Loop counter:     %s ", vlabel[2]);
		break;
	case KZLOOP:
	 	modify_time_value(fval[0], 1);
                info_print(" Loop duration:  %s = %s ", flabel[0], inputs);
                // info_print(" Real time variable: %s = %d ", vlabel[0], val[3]);
                info_print(" Loop counter:   %s ", vlabel[1]);
		break;
	case RLLOOPEND:
	case KZLOOPEND:
                info_print(" Loop counter:  %s ", vlabel[0]);
		break;
	case PARALLELSTART:
                info_print(" Channel:  %s", vlabel[0]);
		break;
	}
#ifdef VNMRJ
        writelineToVnmrJ("dps", "info end \n");
#endif 
#endif 
}



#ifndef VNMRJ
static void
draw_icon(icon_bits, x, y, w, h)
char    icon_bits[];
int	x, y, w, h;
{
	int	y2, x2, cols, rows, count, dcount;
	unsigned char  data, bit;

	amove(x, y);
	for(rows = 0; rows < h; rows++)
	{
	    dcount = rows * w / 8;
	    data = icon_bits[dcount];
	    bit = 0x01;
	    y2 = y - rows;
	    x2 = x;
	    for(cols = 0; cols < w / 8; cols++)
	    {
		bit = 0x01;
		for (count = 0; count < 4; count++)
		{
		    if (data & bit)
		    {
		    	amove(x2, y2);
		    	adraw(x2, y2);
		    }
		    bit = bit << 1;
		    x2++;
		}
		bit = 0x10;
		for (count = 0; count < 4; count++)
		{
		    if (data & bit)
		    {
		    	amove(x2, y2);
		    	adraw(x2, y2);
		    }
		    bit = bit << 1;
		    x2++;
		}
		dcount++;
		data = icon_bits[dcount];
	    }
	}
}
#endif 


static void
draw_menu_icon()
{
#ifdef VNMRJ
	 menu_x = graf_width + 10;
	 menu_y = dpsH + 10;
	 return;
#else 
	int	w;

	if (!dpsPlot && Wissun())
	{
	    w = 2 * pfx;
	    if (!menuUp)
	    {
		menu_x = graf_width - menu_width - 5;
		menu_y = mnumypnts - menu_height - 5;
                amove(menu_x, menu_y);
                dps_color(OFFCOLOR, 0);
		draw_icon(menu_bits, menu_x+2, mnumypnts-2,
				 	menu_width, menu_height);
	    }
	    else
	    {
                amove(menu_x-1, menu_y);
                color(BACK);
                box(menu_width+5, menu_height+5);
	    }
	
	}
	else
	{
	     menu_x = graf_width + 10;
	     menu_y = dpsH + 10;
	}
#endif 
}


#ifdef  SUN

#ifndef VNMRJ

static  Widget  dpsFrame;
static  Widget  butpan_1, butpan_2, butpan_3;
static  Widget  drawPannel = 0;
static  Window  infoWin;
static  Widget  classBut[3];
static  Widget  modeBut[5];
static  Arg     args[16];
static  Dimension  winWidth, winHeight = 0;
static  Atom    deleteAtom;

#endif 

static  Widget  butpan_4;
static  double 	browse_timer = 3.0;


typedef struct  _array_widget {
	int	 ux, dx, lx;
	int	 y, width;
	int	 index;
	int	 active;
	ARRAY_NODE  *anode;
	struct _array_widget  *next;
	struct _array_widget  *prev;
  } ARRAY_WIDGET;

static ARRAY_WIDGET  *arrayList = NULL;

#ifdef MOTIF
static ARRAY_WIDGET  *cur_array_node = NULL;
static void misc_but_proc();
static void dps_browse_proc();
#endif

#ifdef OLIT
typedef struct  _button_rec {
        int     id;
        int     val;
	Display *dpy;
	GC	gc;
        Widget  button;
        Window  win;
        int     width;
        int     height;
	int     label_y;
        int     label_len;
        char    *label;
        void    (*func)();
	Pixel   but_Fg, but_Bg;
        }  button_rec;
static button_rec *dps_but1_rec[BUTNUM1];
static button_rec *dps_but2_rec[BUTNUM2];
extern button_rec  *create_nmr_button();

#else 
#ifndef VNMRJ
static Widget  button_2[7];
static Widget  button_3[3];
#endif 

#endif 


void (*timerfunc)();
XtIntervalId  timerId = 0;


#ifdef MOTIF

static void
timerproc()
{
        if(timerId == 0)
            return;
        timerId = 0;
        if (timerfunc != NULL)
            timerfunc();
}

#endif 

static void
inittimer(timsec,intvl,funccall)
double timsec;
double intvl;
void (*funccall) ();
{
#ifdef MOTIF
    unsigned long  msec;

    if(timerId != 0)
    {
         XtRemoveTimeOut(timerId);
         timerId = 0;
    }
    if (vjControl > 0)
        return;
    timerfunc = funccall;
    if(funccall != NULL)
    {
         msec = timsec * 1000;  /* milliseconds */
         timerId = (XtIntervalId) XtAddTimeOut(msec, 
			(XtTimerCallbackProc)timerproc, NULL);
    }
#else
    long  msec;

    if (funccall == NULL)
	return;   
    if (vjControl > 0)
        return;
    msec = (long) (timsec * 1000000);
    usleep(msec);
    funccall();
#endif
}

static void
popdn_menu(isActive)
int isActive;
{
#ifdef VNMRJ
	inittimer(0.0, 0.0, NULL);
	if (isActive)
	    writelineToVnmrJ("dps", "menu down");
	else
	    writelineToVnmrJ("dps", "window off");
#else 
	if (shellWin)
        {
            stop_dps_browse();
            if (debug > 1)
                fprintf(stderr, " close menu ...\n");
            XtPopdown(dpsShell);
            sprintf(inputs, "dialog('dps/menu','exit')\n");
            execString(inputs);
        }
#endif 
}

static int
close_dps_menu()
{
	if (Bnmr)
	    return 0;
#ifdef VNMRJ
	save_dps_resource();
        clear_cursor();
#endif 
	popdn_menu(0);
	menuUp = 0;
#ifdef MOTIF
	set_font(x_font);
#endif
	mouse_x = -1;
	mouse_x2 = -1;
   	disp_start_node = NULL;
	if (backMap)
	{
#ifdef MOTIF
	    XFreePixmap(dpy, backMap);
#endif
	    backMap = 0;
	    backW = 0;
	    backH = 0;
	}
	return 1;
}

#ifdef MOTIF
void
resize_shell(w, client_data, xev)
  Widget          w;
  XtPointer       client_data;
  XEvent          *xev;
{
	int	rx, ry;
	Window  win;
	XWindowAttributes  attrs;

        if (xev->type == ConfigureNotify)
        {
	    if (XGetWindowAttributes(dpy, shellWin, &attrs))
	    {
		if(attrs.map_state == IsUnmapped)
                    return;
	    }
            menuW = xev->xconfigure.width;
            menuH = xev->xconfigure.height;
            menuX = xev->xconfigure.x;
            menuY = xev->xconfigure.y;
	    XTranslateCoordinates(dpy, shellWin, DefaultRootWindow(dpy),
                        0, 0, &rx, &ry, &win);
	    if (XGetWindowAttributes(dpy, win, &attrs))
       	    { 
                menuX = attrs.x;
                menuY = attrs.y;
	    }
            XtSetArg(args[0], XtNwidth, &winWidth);
            XtSetArg(args[1], XtNheight, &winHeight);
	    XtGetValues(drawPannel, args, 2);
	    return;
        }
        if (xev->type == UnmapNotify)
	{
	    save_dps_resource();
	    if (debug > 1)
		fprintf(stderr, " unmap menu \n");
	    popdn_menu(1);
	    if (menuUp)
            {
               menuUp = 0;
               draw_menu_icon();
            }
	    return;
        }
}
#endif /* MOTIF */


#ifndef VNMRJ
static void
clear_time_window()
{
	XSetForeground (dpy, dps_text_gc, winBg);
	XClearArea(dpy,infoWin,0,time_y,winWidth,info_y - time_y - 1, FALSE);
}
#endif 


#ifndef VNMRJ
static void
clear_info_window()
{
	XSetForeground (dpy, dps_text_gc, winBg);
	XClearArea(dpy,infoWin,0,info_y,winWidth,winHeight - info_y, FALSE);
}
#endif 



static void
draw_array_data(anode, modify_widget)
ARRAY_NODE    *anode;
int	      modify_widget;
{
#ifdef MOTIF
	ARRAY_NODE    *xnode;

	if (modify_widget)
	{
	    if (anode->index_widget != NULL)
	    {
		if (anode->code == SARRAY || anode->code == XARRAY)
	   	  sprintf(info_data, "%d", anode->index + 1);
	 	else
	   	  sprintf(info_data, "%d", anode->index);
#ifdef OLIT
	   	XtSetArg (args[0], XtNstring, info_data);
           	XtSetValues(anode->index_widget, args, 1);
#else 
		XmTextSetString(anode->index_widget, info_data);
#endif 
	    }
	}
	XClearArea(dpy, infoWin, anode->tx, anode->ty-text_ascent,
			winWidth - anode->tx, text_h, FALSE);
	menu_color(BLUE);
	sprintf(info_data, "= ");
        xnode = anode;
        while (xnode != NULL)
        {
	    xnode->index = anode->index;
	    if (xnode->type == T_STRING)
            {
	    	sprintf(inputs, "%s", xnode->value.Str[xnode->index]);
	    }
	    else
	    {
	        if (xnode->ni)
	            sprintf(inputs, "%g", xnode->value.Val[0] + 
				xnode->value.Val[1] * xnode->index);
	    	else
	            sprintf(inputs, "%g", xnode->value.Val[xnode->index]);
	    }

	    strcat(info_data, inputs);
	    xnode = xnode->dnext;
	    if (xnode != NULL)
		strcat (info_data, ", ");
	}
	XDrawString (dpy, infoWin, dps_text_gc, anode->tx, anode->ty, 
			info_data, strlen(info_data));
#endif /* MOTIF */
}



#ifdef  OLIT

#define  XmNset   XtNset

static Widget
create_button(parent, label, num, func, type)
Widget  parent;
char    *label;
int     num, type;
void    (*func)();
{
	Widget  button;

        n = 0;
        XtSetArg (args[n], XtNlabel, label);  n++;
        button = XtCreateManagedWidget("",
                        oblongButtonWidgetClass, parent, args, n);
	if (type > 0)
	{
	    XtAddEventHandler(button, ButtonPressMask, False,
				 func, (XtPointer)num);
	    if (type == 1)
	         XtAddEventHandler(button, ButtonReleaseMask, False,
				 func, (XtPointer)STOPTIMER);
	    if (type == 2)
	         XtAddEventHandler(button, ButtonReleaseMask, False,
				 func, (XtPointer)RDRAW);
	}
	else
            XtAddCallback(button, XtNselect, func, (XtPointer) num);
	return(button);
}

static void
dpsShell_exit(w, client_data, event )
Widget w;
char *client_data;
void *event;
{
        OlWMProtocolVerify      *olwmpv;
	XClientMessageEvent  *mess;
	char    *mname;

        olwmpv = (OlWMProtocolVerify *) event;

        if (olwmpv->msgtype == OL_WM_DELETE_WINDOW) {
		if (debug > 1)
		    fprintf(stderr, " menu exit\n");
	        XtPopdown(dpsShell);
		menuUp = 0;
		draw_menu_icon();
	    	sprintf(inputs, "dialog('dps/menu','exit')\n");
	    	execString(inputs);
        }
}


Widget
create_rc_widget(parent, bwidget, cwidget, type, label)
Widget  parent, bwidget, *cwidget;
int	type;
char    *label;
{
	Widget   twidget, pwidget;

	n = 0;
        XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
        XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
        XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
/**
        XtSetArg(args[n], XtNxOffset, 4); n++;
	XtSetArg(args[n], XtNxAttachOffset, 4); n++;
**/
	XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
        XtSetArg(args[n], XtNvPad, 1); n++;
        XtSetArg(args[n], XtNvSpace, 1); n++;
        XtSetArg (args[n], XtNmeasure, 1);  n++;
	if (bwidget != NULL)
	{
	    XtSetArg(args[n], XtNyRefWidget, bwidget); n++;
	}
	pwidget = XtCreateManagedWidget("button",
                        controlAreaWidgetClass, parent, args, n);
	if (label != NULL)
	{
	    n = 0;
            XtSetArg (args[n], XtNstring, label);  n++;
            XtCreateManagedWidget("", staticTextWidgetClass, pwidget, args, n);
	}

	n = 0;
	if (type == XOR)
        {
	    twidget = XtCreateManagedWidget ("", exclusivesWidgetClass,
			pwidget, args, n);
	}
	else
        {
	    XtSetArg (args[n], XtNlayoutType, OL_FIXEDROWS);  n++;
            XtSetArg (args[n], XtNmeasure, 1);  n++;
	    twidget = XtCreateManagedWidget ("", controlAreaWidgetClass,
			pwidget, args, n);
	}
	*cwidget = twidget;
	return(pwidget);

}

static Widget
create_blt_widget(parent, bwidget, w, h)
Widget  parent, bwidget;
int	w, h;
{
	Widget   twidget;

	n = 0;
        XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
        XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
        XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
	XtSetArg(args[n], XtNxResizable, TRUE);  n++;
	XtSetArg(args[n], XtNyRefWidget, bwidget); n++;
	XtSetArg(args[n], XtNwidth, w); n++;
	XtSetArg(args[n], XtNheight, h); n++;
	twidget = XtCreateManagedWidget("", bulletinBoardWidgetClass,
                        parent, args, n);
   	return(twidget);
}



static Widget
create_toggle_button(parent, label, func, val)
Widget  parent;
char    *label;
void    (*func)();
int     val;
{
        Widget  button;

	n = 0;
        XtSetArg (args[n], XtNlabel, label); n++;
        button = XtCreateManagedWidget ("",
                        rectButtonWidgetClass, parent, args, n);
        XtAddCallback(button, XtNselect, func, (XtPointer) val);
        XtAddCallback(button, XtNunselect, func, (XtPointer) val);
	return(button);
}

static void
dps_but_proc(w, c_data, x_event)
Widget  w;
caddr_t c_data;
XEvent  *x_event;
{
      static int        set = 0;
      int		type;
      button_rec        *b_rec;


      if (c_data == NULL)
          return;
      b_rec = (button_rec *) c_data;
      type = b_rec->val;
      switch (x_event->type)  {
          case ButtonRelease:
                if (!set)
                     return;
		if (type == 0)
                     b_rec->func(w, (XtPointer)b_rec->id, NULL);
		else if (type == 1)
                     b_rec->func(w, (XtPointer)STOPTIMER, NULL);
		else if (type == 2)
                     b_rec->func(w, (XtPointer)RDRAW, NULL);
                draw_button(b_rec, 0);
                set = 0;
                break;
          case LeaveNotify:
                if (!set)
                     return;
		if (type == 1)
                     b_rec->func(w, (XtPointer)STOPTIMER, NULL);
		else if (type == 2)
                     b_rec->func(w, (XtPointer)RDRAW, NULL);
                draw_button(b_rec, 0);
                set = 0;
                break;
          case ButtonPress:
                if (set)
                     return;
                set = 1;
		if (type > 0)
                     b_rec->func(w, (XtPointer)b_rec->id, NULL);
                draw_button(b_rec, 1);
                break;
        }

}

static
create_attr_buttons(parent)
 Widget parent;
{
	int	x, y, k;
	int	len, width; 
	button_rec *b_rec;

	x = 4;
	y = 4;
	for (k = 0; k < BUTNUM1; k++)
	{
	   len = strlen(but_label_1[k]);
	   width = XTextWidth(bfontInfo, but_label_1[k], len) + 8;
	   b_rec = create_nmr_button(x, y, width, but_h, but_label_1[k], parent, NULL, dps_but_proc);
	   b_rec->id = but_val_1[k];
	   b_rec->val = but_type_1[k];
	   b_rec->func = misc_but_proc;
	   b_rec->dpy = dpy;
	   b_rec->gc = dps_but_gc;
	   b_rec->label_y = but_ascent;
	   b_rec->label_len = len;
	   b_rec->but_Fg = winFg;
	   b_rec->but_Bg = winBg;
	   dps_but1_rec[k] = b_rec;
	   x = x + width + 4;
	}
}


static
create_attr2_buttons(parent, map)
 Widget parent;
 int	map;
{
	int	x, y, k;
	int	len, width; 
	button_rec *b_rec;

	x = 4;
	y = 8 + but_h;
	for (k = 0; k < BUTNUM2; k++)
	{
	   len = strlen(but_label_2[k]);
	   width = XTextWidth(bfontInfo, but_label_2[k], len) + 8;
	   b_rec = create_nmr_button(x, y, width, but_h, but_label_2[k], parent, dps_but2_rec[k], dps_but_proc);
	   b_rec->id = but_val_2[k];
	   b_rec->val = but_type_2[k];
	   b_rec->func = misc_but_proc;
	   b_rec->dpy = dpy;
	   b_rec->gc = dps_but_gc;
	   b_rec->label_y = but_ascent;
	   b_rec->label_len = len;
	   b_rec->but_Fg = winFg;
	   b_rec->but_Bg = winBg;
	   dps_but2_rec[k] = b_rec;
	   x = x + width + 4;
	   if (map)
	   {
	      if (k != 4)  /* 4 is Redo button */
	          XtMapWidget(b_rec->button);
	      else if (array_start_node != NULL)
	          XtMapWidget(b_rec->button);
	   }
	}
}


static void
create_attr3_buttons()
{
	int	x, y, k;
	int	len, width; 

	x = 4;
	y = 8 + but_h;
	for (k = 4; k >= 0; k--)
	   XtUnmapWidget(dps_but2_rec[k]->button);
	for (k = 0; k < 3; k++)
	{
	   len = strlen(but_label_3[k]);
	   width = XTextWidth(bfontInfo, but_label_3[k], len) + 8;
	   create_nmr_button(x, y, width, but_h, but_label_3[k], NULL, dps_but2_rec[k], dps_but_proc);
	   dps_but2_rec[k]->id = but_val_3[k];
	   dps_but2_rec[k]->val = but_type_3[k];
	   dps_but2_rec[k]->func = dps_browse_proc;
	   dps_but2_rec[k]->label_len = len;
	   x = x + width + 8;
	   XtMapWidget(dps_but2_rec[k]->button);
	}
}


static
set_dps_but_xid()
{
	int	k;

	for (k = 0; k < BUTNUM1; k++)
	   dps_but1_rec[k]->win = XtWindow(dps_but1_rec[k]->button);
	for (k = 0; k < BUTNUM2; k++)
	   dps_but2_rec[k]->win = XtWindow(dps_but2_rec[k]->button);
}


static void
index_motion_cb(w, c_data, call_data)
  Widget          w;
  XtPointer       c_data;
  XtPointer       call_data;
{
	ARRAY_NODE *anode;
	int	   k;
	char	   *data, *data2;

	XtSetArg (args[0], XtNuserData, &anode);
	XtGetValues(w, args, 1);
	if (anode == NULL || anode->index_widget == NULL)
	     return;
	XtSetArg (args[0], XtNstring, &data);
        XtGetValues(anode->index_widget, args, 1);
	data2 = data;
	while (*data2 == ' ')
	     data2++;
	data = data2;
	while (*data2 != '\0')
	{
	     if (*data2 < '0' || *data2 > '9')
	     {
		*data2 = '\0';
		break;
	     }
	     data2++;
	}
	if ((int) strlen(data) <= 0)
	     k = 0;
	else
	{
	     k = atoi(data) - 1;
	     if (k < 0)
		k = 0;
	     if (k >= anode->vars)
	     {
		k = anode->vars - 1;
		if (anode->code == SARRAY || anode->code == XARRAY)
		   sprintf(info_data, "%d", k+1);
		else
		   sprintf(info_data, "%d", k);
		XtSetArg (args[0], XtNstring, info_data);
        	XtSetValues(anode->index_widget, args, 1);
	     }
	}
	if (anode->index != k)
	{
	     anode->index = k;
	     draw_array_data(anode, 0);
	}
}


static void
index_modify_cb(w, c_data, call_data)
  Widget          w;
  XtPointer       c_data;
  XtPointer       call_data;
{
        OlTextModifyCallData    *modifyPtr;

        modifyPtr = (OlTextModifyCallData *) call_data;
        modifyPtr->ok = FALSE;
        if (*(modifyPtr->text) >= '0' && *(modifyPtr->text) <= '9')
             modifyPtr->ok = TRUE;
        if ( *(modifyPtr->text) == 0 )
             modifyPtr->ok = TRUE;
}

static void
create_array_index_widget(parent)
Widget  parent;
{
	int	k;
	Widget  txWidget;

	n = 0;
	XtSetArg(args[n], XtNx, 30); n++;
	XtSetArg(args[n], XtNy, 30); n++;
	XtSetArg(args[n], XtNfontColor, dpsPix[ICOLOR]); n++;
	XtSetArg(args[n], XtNcharsVisible, 7); n++;
	for (k = 0; k < ARRAYMAX; k++)
	{
	   array_i_widgets[k] = XtCreateManagedWidget("text", 
		textFieldWidgetClass, parent, args, n);
	   XtSetArg(args[0], XtNtextEditWidget, &txWidget);
	   XtGetValues(array_i_widgets[k], args, 1);
	   if (txWidget != NULL)
	   {
		array_t_widgets[k] = txWidget;
	        XtAddCallback(txWidget, XtNmodifyVerification,
			index_modify_cb, NULL);
	        XtAddCallback(txWidget, XtNmotionVerification,
			index_motion_cb, NULL);
	        XtSetValues(txWidget, args, 1);
	   }
	   else
		array_t_widgets[k] = NULL;
	}
}


#else /* OLIT */

XmString        xmstr;

#ifdef MOTIF
static Widget
create_button(parent, label, num, func, type, x, y)
Widget  parent;
char    *label;
int     num, type;
int     x, y;
void    (*func)();
{
	Widget   button;

        int n = 0;
        xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
        XtSetArg (args[n], XmNx, x);  n++;
        XtSetArg (args[n], XmNy, y);  n++;
        XtSetArg (args[n], XmNlabelString, xmstr);  n++;
	XtSetArg (args[n], XmNtraversalOn, FALSE);  n++;
        button = (Widget) XmCreatePushButton(parent, "button", args, n);
        XtManageChild (button);
	if (type == 1)
	{
            XtAddCallback(button, XmNarmCallback, func, (XtPointer)num);
            XtAddCallback(button, XmNdisarmCallback, func, (XtPointer)STOPTIMER);
	}
	else if (type == 2)
	{
            XtAddCallback(button, XmNarmCallback, func, (XtPointer)num);
            XtAddCallback(button, XmNdisarmCallback, func, (XtPointer)RDRAW);
	}
	else
            XtAddCallback(button, XmNactivateCallback, func, (XtPointer)num);
        XmStringFree(xmstr);
	return(button);
}
 
static void
dpsShell_exit()
{
	if (debug > 1)
	    fprintf(stderr, " menu exit ...\n");
	dpsShell = 0;
	shellWin = 0;
	menuUp = 0;
	draw_menu_icon();
}


static Widget
create_rc_widget(parent, bwidget, type, label)
Widget  parent, bwidget;
int	type;
char    *label;
{
#ifdef MOTIF
        int     n, k;
        Widget  twidget, lwidget;

	lwidget = NULL;
	k = 0;
	if (bwidget)
	{
            XtSetArg (args[k], XmNtopAttachment, XmATTACH_WIDGET); k++;
            XtSetArg (args[k], XmNtopWidget, bwidget); k++;
	}
	else
	{
            XtSetArg (args[k], XmNtopAttachment, XmATTACH_FORM); k++;
	}
	if (label != NULL)
	{
           n = k;
           xmstr = XmStringLtoRCreate(label, XmSTRING_DEFAULT_CHARSET);
           XtSetArg (args[n], XmNlabelString, xmstr);  n++;
           XtSetArg (args[n], XmNx, 4);  n++;
           XtSetArg (args[n], XmNtopOffset, 6);  n++;
           XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
           lwidget = (Widget)XmCreateLabel(parent, "button", args, n);
           XtManageChild (lwidget);
           XmStringFree(xmstr);
	}

        n = k;
	if (lwidget != NULL)
	{
            XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
            XtSetArg (args[n], XmNleftWidget, lwidget); n++;
	}
	else
	{
           XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	}
        XtSetArg (args[n], XmNorientation, XmHORIZONTAL); n++;
        XtSetArg (args[n], XmNpacking, XmPACK_TIGHT);  n++;
	if (type == XOR)
	{
	    XtSetArg(args[n], XmNradioBehavior, TRUE); n++;
	}
   
        twidget = (Widget) XmCreateRowColumn(parent, "button", args, n);
        XtManageChild (twidget);
        return(twidget);
#else
        return(0);
#endif
}

static Widget
create_blt_widget(parent, bwidget, attach_bottom)
Widget  parent, bwidget;
int	attach_bottom;
{
#ifdef MOTIF
        int     k;
        Widget  twidget;

	k = 0;
	if (bwidget)
	{
            XtSetArg (args[k], XmNtopAttachment, XmATTACH_WIDGET); k++;
            XtSetArg (args[k], XmNtopWidget, bwidget); k++;
	}
	else
	{
            XtSetArg (args[k], XmNtopAttachment, XmATTACH_FORM); k++;
	}
        XtSetArg (args[k], XmNleftAttachment, XmATTACH_FORM); k++;
	XtSetArg (args[k], XmNrightAttachment, XmATTACH_FORM);  k++;
	if (attach_bottom)
	{
	    XtSetArg (args[k], XmNbottomAttachment, XmATTACH_FORM); k++;
	}
            XtSetArg (args[k], XmNresizable, TRUE);  k++;
	XtSetArg (args[k], XmNmarginHeight, 1);  k++;
	XtSetArg (args[k], XmNmarginWidth, 1);  k++;
	XtSetArg (args[k], XmNborderWidth, 0); k++;
        twidget = XmCreateBulletinBoard(parent, "", args, k);
        return(twidget);
#else
        return(0);
#endif
}


static Widget
create_toggle_button(parent, label, func, val)
Widget  parent;
char    *label;
void    (*func)();
int     val;
{
#ifdef MOTIF
        Widget  button;

        int n =0;
	XtSetArg (args[n], XmNtraversalOn, FALSE); n++;
	XtSetArg (args[n], XmNborderWidth, 1); n++;
        button = (Widget) XmCreateToggleButtonGadget(parent, label, args, n);
        XtManageChild (button);
        XtAddCallback(button, XmNvalueChangedCallback, func, (XtPointer) val);
        return(button);
#else
        return(0);
#endif
}

static void
create_attr_buttons(parent)
 Widget parent;
{
#ifdef MOTIF
	int	k, x, y;
	Dimension  bw;
	Widget  but;

	x = 4;
	y = 2;
	for(k = 0; k < BUTNUM1; k++)
	{
	   but = create_button(parent,but_label_1[k],but_val_1[k],misc_but_proc,0, x, y);
	   XtSetArg(args[0], XtNwidth, &bw);
      	   XtGetValues(but, args, 1);
           x = x + bw + 4;
	}
#endif

}
#endif

static void
create_attr2_buttons(parent, map)
 Widget parent;
 int    map;
{
#ifdef MOTIF
	int	k, x, y;
	Dimension  bw;

	for(k = 2; k >= 0; k--)
	{
		if (button_3[k] != NULL)
	            XtUnmapWidget(button_3[k]);
	}
	x = 4;
	y = 2;
	for(k = 0; k < BUTNUM2; k++)
	{
	   if (button_2[k] == NULL)
	   {
	      button_2[k] = create_button(parent, but_label_2[k], but_val_2[k],
			 misc_but_proc, but_type_2[k], x, y);
	      XtSetArg(args[0], XtNwidth, &bw);
      	      XtGetValues(button_2[k], args, 1);
              x = x + bw + 4;
	    }
	    if (map)
	    {
	       if (k == 5)
	       {
		   if (array_start_node != NULL)
	               XtMapWidget(button_2[k]);
		   else
	               XtUnmapWidget(button_2[5]);
	       }
	       else
	            XtMapWidget(button_2[k]);
	    }
	}
#endif
}

static void
create_attr3_buttons()
{
#ifdef MOTIF
	int	k, x, y;
	Dimension  bw;

	for (k = 5; k >= 0; k--)
	{
	    if (button_2[k] != NULL)
		XtUnmapWidget(button_2[k]);
	}
	x = 4;
	y = 2;
	for(k = 0; k < 3; k++)
	{
	   if (button_3[k] == NULL)
	   {
	      button_3[k] = create_button(butpan_4, but_label_3[k], but_val_3[k],
			 dps_browse_proc, but_type_3[k], x, y);
	      XtSetArg(args[0], XtNwidth, &bw);
      	      XtGetValues(button_3[k], args, 1);
              x = x + bw + 6;
	   }
	   XtMapWidget(button_3[k]);
	}
#endif
}


#ifdef MOTIF
static void
index_change_cb(w, c_data, call_data)
  Widget          w;
  XtPointer       c_data;
  XtPointer       call_data;
{
#ifdef MOTIF
	ARRAY_NODE *anode;
	int	   k;
	char	   *data;
	XmTextVerifyCallbackStruct  *mptr;

	mptr = (XmTextVerifyCallbackStruct *)call_data;
	XtSetArg (args[0], XmNuserData, &anode);
	XtGetValues(w, args, 1);
	if (anode == NULL || anode->index_widget == NULL)
	     return;
	data = (char *) XmTextGetString(w);
	if ((int) strlen(data) <= 0)
	     k = 0;
	else
	{
	     k = atoi(data) - 1;
	     if (k < 0)
		k = 0;
	     if (k >= anode->vars)
	     {
		k = anode->vars - 1;
		if (anode->code == SARRAY || anode->code == XARRAY)
		   sprintf(info_data, "%d", k+1);
		else
		   sprintf(info_data, "%d", k);
		XmTextSetString(w, info_data);
	     }
	}
	if (anode->index != k)
	{
	     anode->index = k;
	     draw_array_data(anode, 0);
	}
#endif
}


static void
index_modify_cb(w, c_data, call_data)
  Widget          w;
  XtPointer       c_data;
  XtPointer       call_data;
{
#ifdef MOTIF
	XmTextVerifyCallbackStruct  *mptr;

	mptr = (XmTextVerifyCallbackStruct *)call_data;
        mptr->doit = FALSE;
        if (*mptr->text->ptr >= '0' && *mptr->text->ptr <= '9')
             mptr->doit = TRUE;
        if ( *mptr->text->ptr == 0 )
             mptr->doit = TRUE;
#endif
}


static void
create_array_index_widget(parent)
Widget  parent;
{
#ifdef MOTIF
	int	k;
	Widget  txWidget;
	Position y;

	int n = 0;
	XtSetArg(args[n], XmNx, 0); n++;
	if (winHeight > 0)
	    y = winHeight - text_h - 14;
	else
	    y = text_h * 12;
	if ( y < 0)
	    y = 0;
	XtSetArg(args[n], XmNy, y); n++;
        XtSetArg (args[n], XtNmappedWhenManaged, FALSE); n++;
	XtSetArg (args[n], XmNshadowThickness, 1); n++;
	XtSetArg(args[n], XmNmarginHeight, 1); n++;
	XtSetArg(args[n], XmNwidth, text_w * 7); n++;
	XtSetArg(args[n], XmNverifyBell, FALSE); n++;
	XtSetArg(args[n], XmNresizeWidth, FALSE); n++;
	XtSetArg(args[n], XmNforeground, dpsPix[ICOLOR]); n++;
	if (infoFontList)
	{
	    XtSetArg(args[n], XmNfontList,  infoFontList); n++;
	}
	for (k = 0; k < ARRAYMAX; k++)
	{
	   txWidget = (Widget)XmCreateText(parent, "text", args, n);
	   XtManageChild(txWidget);
	   array_i_widgets[k] = txWidget;
	   array_t_widgets[k] = txWidget;
	   XtAddCallback(txWidget, XmNmodifyVerifyCallback,
			index_modify_cb, NULL);
	   XtAddCallback(txWidget, XmNvalueChangedCallback,
			index_change_cb, NULL);
	}
#endif 
}
#endif



#endif 


static void
do_class_proc(num)
int num;
{
	int	mode;

	if (!in_browse_mode && (clearDraw == 0 && backMap == 0))
	     batch_draw();
        mode = 0;
	switch (num) {
	 case  TIMEMODE:   /* timing */
		mode = POWERMODE | PHASEMODE;
		break;
	 case  POWERMODE:   /* power */
		mode = TIMEMODE | PHASEMODE;
		break;
	 case  PHASEMODE:   /* phase */
		mode = TIMEMODE | POWERMODE;
		break;
	 default:
		return;
		break;
	}
	dispMode = dispMode | num;
	dispMode = dispMode & (~mode);
	if (!in_browse_mode)
	     batch_draw();
}

#ifdef MOTIF
static void
class_but_proc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
	int	num;
	Boolean set;

	num = (int) client_data;
        XtSetArg (args[0], XmNset, &set);
	XtGetValues(widget, args, 1);
	if (!set)
	       return;
	do_class_proc(num);
}
#endif

static void
do_mode_proc(num, set)
int	num, set;
{
	int	k, mode;

	if (!in_browse_mode && (clearDraw == 0 && backMap == 0))
	     batch_draw();
	switch (num) {
	 case  DSPLABEL:   /* label  */
		mode = DSPLABEL;
		break;
	 case  DSPVALUE:   /* value  */
		mode = DSPVALUE;
		break;
	 case  DSPMORE:   /* all  */
		mode = DSPMORE;
		break;
	 case  DSPDECOUPLE:   /* show decoupling channel  */
		mode = DSPDECOUPLE;
		break;
	 default:
		return;
		break;
	}
	if (set)
	     dispMode = dispMode | mode;
	else
	     dispMode = dispMode & (~mode);
	if (!in_browse_mode) {
             if (mode == DSPDECOUPLE) {
                 for (k = DODEV; k <= DO5DEV; k++)
                     visibleCh[k] = set;
                 dps_config();
             }
	     batch_draw();
        }
}

#ifdef MOTIF
static void
mode_but_proc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
	int	num;
	Boolean set;

	num = (int) client_data;
        XtSetArg (args[0], XmNset, &set);
	XtGetValues(widget, args, 1);
	do_mode_proc(num, (int) set);
}
#endif


static void
dps_move_left()
{
	int	newOffset;
        int     mvStep;

        mvStep = dpsW / 6;
        if (xMult > 1.4)
            mvStep = dpsW / 3;
        if (mvStep < MOVESTEP)
            mvStep = MOVESTEP;
	if (x_margin > orgx_margin)
	{
            /**
	    if (time_num > 5)
	        newOffset = x_margin - 4 * MOVESTEP;
	    else
	        newOffset = x_margin - MOVESTEP;
            **/
	    newOffset = x_margin - mvStep;
	    if (newOffset < orgx_margin)
		newOffset = orgx_margin;
	    if (x_margin != newOffset)
	    {
		if (clearDraw == 0 && backMap == 0)
	            batch_draw();
		x_margin = newOffset;
	        batch_draw();
	    }
	    time_num++;
	    inittimer(0.2, 0.0, dps_move_left);
	}
	else if (xOffset < dpsWidth - 10)
	{
           /**
	    if (time_num > 5)
	        newOffset = xOffset + 6 * MOVESTEP;
	    else if (time_num > 3)
	        newOffset = xOffset + 4 * MOVESTEP;
	    else if (time_num > 1)
	        newOffset = xOffset + 2 * MOVESTEP;
	    else
	        newOffset = xOffset + MOVESTEP;
            **/
	    newOffset = xOffset + mvStep;
	    if (newOffset >= dpsWidth - 20)
		newOffset = dpsWidth - 20;
	    if (xOffset != newOffset)
	    {
		if (clearDraw == 0 && backMap == 0)
		    batch_draw();
		xOffset = newOffset;
	        batch_draw();
	    }
	    time_num++;
	    inittimer(0.2, 0.0, dps_move_left);
	}
}


static void
dps_move_right()
{
	int	newOffset;
	int	mvStep;

        mvStep = dpsW / 6;
        if (xMult > 1.4)
            mvStep = dpsW / 3;
        if (mvStep < MOVESTEP)
            mvStep = MOVESTEP;

	if (xOffset > 0)
	{
           /***
	    if (time_num > 5)
	        newOffset = xOffset - 6 * MOVESTEP;
	    if (time_num > 3)
	  	newOffset = xOffset - 4 * MOVESTEP;
	    else if (time_num > 1)
	  	newOffset = xOffset - 2 * MOVESTEP;
	    else
	  	newOffset = xOffset - MOVESTEP;
            ***/
	    newOffset = xOffset - mvStep;
	    if (newOffset < 0)
		newOffset = 0;
	    if (xOffset != newOffset)
	    {
		if (clearDraw == 0 && backMap == 0)
	            batch_draw();
		xOffset = newOffset;
	        batch_draw();
	    }
	    time_num++;
	    inittimer(0.2, 0.0, dps_move_right);
	}
	else if (x_margin < dpsX2)
	{
            /***
	    if (time_num > 5)
	        newOffset = x_margin + 6 * MOVESTEP;
	    else
	  	newOffset = x_margin + MOVESTEP;
            ***/
	    newOffset = x_margin + mvStep;
	    if (newOffset > dpsX2 - 20)
		newOffset = dpsX2 - 20;
	    if (x_margin != newOffset)
	    {
		if (clearDraw == 0 && backMap == 0)
	            batch_draw();
		x_margin = newOffset;
	        batch_draw();
	    }
	    time_num++;
	    inittimer(0.2, 0.0, dps_move_right);
	}
}


static void
adjust_x_offset()
{
	int	dx;

	dx = hilitNode->x1 + (hilitNode->x2 - hilitNode->x1) / 2;
	xOffset = dx - (dpsW + dpsX - x_margin) / 2;
	if (xOffset < 0)
	    xOffset = 0;
}
	
static void
dps_config()
{
	set_xy_params();
	simulate_psg(transient - 1, 1);
	calculate_x_ratio(disp_start_node, NULL, 0);
}


static void
define_expand_range(set)
int	set;
{
	int	x1, x2, sx, sx2;
	double   dx;
	SRC_NODE  	 *node;
	static double     dr1, dr2, dr3;
	static SRC_NODE  *snode = NULL;
	static SRC_NODE  *dnode = NULL;

        x1 = x2 = 0;
	if (set == 1)
	{
	   if (mouse_x < mouse_x2)
	   {
	     	x1 = mouse_x;
	     	x2 = mouse_x2;
	   }
	   else
	   {
	     	x1 = mouse_x2;
	     	x2 = mouse_x;
	   }
	   if (x2 <= x_margin)
	     	return;
	   if (x1 >= dpsWidth + xDiff)
	        return;
	   if ((x2 - x1) <= 2)
	     	return;
	}
	else if (set == 2)
	{
	   x1 = x_margin;
	   x2 = dpsX2;
	}

	if (set)
	{
	   node = draw_start_node;
           if (x1 < (node->x1 + xDiff))
               x1 = node->x1 + xDiff;
	   snode = NULL;
	   dnode = NULL;
	   while (node != NULL)
	   {
	     	if (node->device > 0)
	     	{
		   if (snode == NULL)
		   {
		       if (node->x2 + xDiff >= x1 && x1 >= node->x1 + xDiff)
			   snode = node;
		   }
		   if (dnode == NULL)
		   {
		       if (node->x2 + xDiff >= x2 && x2 >= node->x1 + xDiff)
		       {
			   dnode = node;
			   break;
		       }
		   }
	        }
	        node = node->dnext;
	   }
	   if (snode == NULL)
		return;
	   dx = (double) (snode->x2 - snode->x1);
	   if (dx < 2.0)
		dx = 2.0;
	   dr1 = (double)(snode->x2 + xDiff - x1) / dx;
	   sx = snode->x2 - (dx * dr1);
	   if (dnode)
	   {
		dx = (double) (dnode->x2 - dnode->x1);
		if (dx < 2.0)
	           dx = 2.0;
		dr2 = (double)(x2 - dnode->x1 - xDiff) / dx;
		sx2 = dnode->x1 + (dx * dr2);
	   }
	   else
	   {
		sx2 = dpsWidth;
		dx = sx2 - sx;
		dr3 = dx / (double) (dpsX2 - x_margin);
	   }
	   if (set == 2)
		return;
	   dx = sx2 - sx;
	   dx = (double) (dpsX2 - x_margin - pfx * 2) / dx;
	   xMult = xMult * dx;
	   measure_xpnt(disp_start_node, NULL, 3, 0, 0);
	   xOffset = snode->x2 - (double)(snode->x2 - snode->x1) * dr1;
	   if (xOffset < 0)
		xOffset = 0;
	   return;
	} /* if set */
	if (snode == NULL)
	{
	   xOffset = 0;
	   return;
	}
	dx = (double) (snode->x2 - snode->x1);
	if (dx < 0.0)
	   dx = 0.0;
	sx = snode->x2 - (dx * dr1);
	if (dnode)
	{
	   dx = (double) (dnode->x2 - dnode->x1);
	   if (dx < 0.0)
	      dx = 0.0;
	   sx2 = dnode->x1 + (dx * dr2);
	   dx = sx2 - sx;
	   x1 = dpsX2 - x_margin - pfx * 2;
	}
	else
	{
	   dx = dpsWidth - sx;
	   x1 = (dpsX2 - x_margin) * dr3;
	}
	dx = (double) x1 / dx;
	xMult = xMult * dx;
	measure_xpnt(disp_start_node, NULL, 3, 0, 0);
	xOffset = snode->x2 - (double)(snode->x2 - snode->x1) * dr1;
	if (xOffset < 0)
	   xOffset = 0;
}


static void
expand_draw()
{
	if (clearDraw == 0 && backMap == 0)
	    batch_draw();
	define_expand_range(1);
	batch_draw();
}


static  void
print_transient()
{
#ifdef VNMRJ
	sprintf(info_data, "scan Scan:  %d      nt: %d", transient, sys_nt);
	writelineToVnmrJ("dps", info_data);
	sprintf(info_data, "nt  %d", sys_nt);
	writelineToVnmrJ("dps", info_data);
	sprintf(info_data, "transient  %d", transient);
	writelineToVnmrJ("dps", info_data);
#else 
	menu_color(BLUE);
	sprintf(info_data, "Scan:    %d      nt:   %d", transient, sys_nt);
	XDrawString (dpy, infoWin, dps_text_gc, XGAP, scan_y + text_h, info_data,
			strlen(info_data));
#endif 
}


static void
change_transient()
{
#ifndef VNMRJ
	int	y;
#endif

	inittimer(0.0, 0.0, NULL);
	if (tr_inc > 0)
	{
	    if (transient >= sys_nt)
		return;
	    transient++;
	}
	else
	{
	    if (transient <= 1)
	     	return;
	    transient--;
	}
/*
	clear_time_window();
*/
#ifndef VNMRJ
	y = scan_y + text_descent;
	XSetForeground (dpy, dps_text_gc, winBg);
	XClearArea(dpy,infoWin,0,y,winWidth,info_y - y - 1, FALSE);
#endif 
	time_num++;
	print_transient();
	if (time_num > 1)
	    inittimer(0.2, 0.0, change_transient);
	else
	    inittimer(0.8, 0.0, change_transient);
}


static void
exec_transient()
{
	double  	oldMaxd, oldMaxp;

	if (clearDraw == 0 && backMap == 0)
             batch_draw();
	oldMaxd = dMax;
	oldMaxp = pMax;
        if (execute_ps(transient) < 1)
	     return;
	if (disp_start_node == NULL)
	{
	     Werrprintf("dps error: empty data when ct is %d.", transient);
	     return;
	}
	dMax = oldMaxd;
	pMax = oldMaxp;
	measure_xpnt(disp_start_node, NULL, 1, 0, 0);
        batch_draw();
}

static void
browse_timeout_proc()
{
	ARRAY_WIDGET  *wnode;
	ARRAY_NODE    *anode;

	transient++;
	if (transient > sys_nt)
	{
	   transient = 1;
	   if (arrayList != NULL)
	   {
		wnode = arrayList;
		while (wnode->next != NULL)
			wnode = wnode->next;
		while (wnode != NULL)
		{
		    anode = wnode->anode;
		    anode->index += 1;
		    if (anode->index >= anode->vars)
		       anode->index = 0;
		    draw_array_data(anode, 1);
		    if (anode->index != 0)
			break;
		    wnode = wnode->prev;
		}
		if (write_array_opt())
		{
                     rerun_dps();
		     inittimer(browse_timer+1.0, 0.0, browse_timeout_proc);
		     return;
		}
	   }
	}
	exec_transient();
	inittimer(browse_timer, 0.0, browse_timeout_proc);
}

static void
clear_array_opt()
{
	remove_dps_file(DPS_ARRAY);
}

static int
write_array_opt()
{
	int	      k;
	double 	      fval;
	struct   stat f_stat;
	ARRAY_WIDGET  *node;
	ARRAY_NODE    *anode;
	FILE          *fout;

	if (arrayList == NULL)
	    return(0);
        sprintf(dpsfile, "%s/%s", curexpdir, DPS_ARRAY);
	if((fout = fopen(dpsfile, "w")) == NULL)
	    return(0);
	node = arrayList;
	first_round = 1;
	while (node != NULL)
	{
	    anode = node->anode;
	    if ((anode->code != SARRAY) && (anode->code != XARRAY))
	    {
		node = node->next;
		continue;
	    }
	    while (anode != NULL)
	    {
		if (anode->index > 0)
		    first_round = 0;
	        fprintf(fout, "%s %d ", anode->name, anode->type);
	    	if (anode->type == T_STRING) {
                   if (anode->value.Str[anode->index] != NULL)
	             fprintf(fout, "%s  %d\n",  anode->value.Str[anode->index], anode->index);
                }
                else
	    	{
		   if (anode->ni > 0)
		   {
		      fval = anode->value.Val[0] + anode->value.Val[1] * anode->index;
		      if (anode->ni == 1)
                         id2 = anode->index;
                      else if (anode->ni == 2)
                         id3 = anode->index;
                      else if (anode->ni == 3)
                         id4 = anode->index;
		   }
		   else
		      fval = anode->value.Val[anode->index];
	           fprintf(fout, " %g  %d\n", fval, anode->index);
	    	}
		anode = anode->dnext;
	    }
	    node = node->next;
	}
	fsync(fileno(fout));
	fclose(fout);

	for (k = 0; k < 5; k++)
	{
            if (stat(dpsfile, &f_stat) >= 0) {
	        if (f_stat.st_size > 0)
		   break;
            }
            usleep(1.0e+5); // 100 ms
	}
	return(1);
}

#ifdef MOTIF
static void
dps_menu_proc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
	int	k;

	k = (int) client_data;
	switch (k) {
	 case  CLOSE:
		if (debug > 1)
	    	    fprintf(stderr, " close menu \n");
	        popdn_menu(1);
		menuUp = 0;
                draw_menu_icon();
		break;
	 case  PROPERTY:
     		sprintf(info_data, "%s/templates/dps/setup",userdir);
		sprintf(inputs, "dialog('dps/menu','%s','nowait')\n", info_data);
		execString(inputs);
		showTimeMark = 1;
		break;
	}
}
#endif


static void
do_but_proc(int num)
{
	int	old_off, old_diff, old_ratio;
	double   old_mult;

	switch (num) {
	 case  CLOSE:
		if (debug > 1)
	    	    fprintf(stderr, " close menu \n");
		popdn_menu(1);
		menuUp = 0;
		draw_menu_icon();
		break;
	 case  DRAW:
		if (src_start_node == NULL)
	     	     return;
		if (clearDraw == 0 && backMap == 0)
             	     batch_draw();
		setdisplay();
		if (frameW != graf_width || frameH != graf_height)
		     dps_config();
		batch_draw();
		break;
	 case  FULL:
		if (src_start_node == NULL)
	     	     return;
		if (clearDraw == 0 && backMap == 0)
             	     batch_draw();
		clear_dps_info();
		xOffset = 0;
		xMult = 1.0;
		setdisplay();
/*
                if (Wissun())
	            frameW = graf_width;
*/
		if (frameW != graf_width || frameH != graf_height)
		     dps_config();
		else
		{
                     if (Wissun())
			mnumypnts = graf_height;
		     set_xy_params();
	  	     measure_xpnt(disp_start_node, NULL, 2, 0, 0);
		}
		batch_draw();
		break;
	 case  MAGNIFY:
		if (src_start_node == NULL)
	     	     return;
		if (in_browse_mode)
		     return;
		if (clearDraw == 0 && backMap == 0)
             	     batch_draw();
		xMult = xMult * 1.2;
		xOffset = xOffset * 1.2;
	  	measure_xpnt(disp_start_node, NULL, 3, 0, 0);
		if (hilitNode != NULL)
		     adjust_x_offset();
		batch_draw();
		break;
	 case  SHRINK:
		if (src_start_node == NULL)
	     	     return;
		if (in_browse_mode)
		     return;
		if (clearDraw == 0 && backMap == 0)
             	     batch_draw();
		xMult = xMult * 0.8;
		xOffset = xOffset * 0.8;
	  	measure_xpnt(disp_start_node, NULL, 3, 0, 0);
		if (hilitNode != NULL)
		     adjust_x_offset();
		batch_draw();
		break;
	 case  EXPAND:
		if (src_start_node == NULL)
	     	     return;
		if (in_browse_mode)
		     return;
		expand_draw();
		break;
	 case  LSHIFT:
		if (src_start_node == NULL)
	     	     return;
	        time_num = 0;
		dps_move_left();
		break;
	 case  RSHIFT:
		if (src_start_node == NULL)
	     	     return;
	        time_num = 0;
		dps_move_right();
		break;
	 case  PREV:
		if (src_start_node == NULL)
	     	     return;
	        time_num = 0;
		tr_inc = 0;
		change_transient();
		break;
	 case  NEXT:
		if (src_start_node == NULL)
	     	     return;
	        time_num = 0;
		tr_inc = 1;
		change_transient();
		break;
	 case  RDRAW:
		if (src_start_node == NULL)
	     	     return;
	        time_num = 0;
	    	inittimer(0.0, 0.0, NULL);
		exec_transient();
		break;
	 case  REDO:
		if (tbug)
	     	     return;
		if (src_start_node == NULL)
	     	     return;
		if (write_array_opt())
		{
		     clear_dps_info();
		     if (frameW != graf_width || frameH != graf_height)
		     {
		        setdisplay();
		        dps_config();
		     }
		     rerun_dps();
		     if (!debug)
		        clear_array_opt();
		}
                else {
                     if (strlen(psgfile) > 0)
                         sprintf(inputs, "dps('%s')\n", psgfile);
                     else
                         sprintf(inputs, "dps\n");
                     execString(inputs);
                     return;
                }
		break;
	 case  BROWSE:
		if (tbug)
	     	     return;
		if (src_start_node == NULL)
	     	     return;
	    	inittimer(0.0, 0.0, NULL);
		create_attr3_buttons();
		in_browse_mode = 1;
	    	inittimer(browse_timer, 0.0, browse_timeout_proc);
		break;
	 case  STOPTIMER:
	        time_num = 0;
	    	inittimer(0.0, 0.0, NULL);
		break;
	 case  PROPERTY:
#ifdef VNMRJ
     		sprintf(info_data, "property %s/templates/dps/setup",userdir);
		writelineToVnmrJ("dps", info_data);
#else 
     		sprintf(info_data, "%s/templates/dps/setup",userdir);
		sprintf(inputs, "dialog('dps/menu','%s','nowait')\n", info_data);
		execString(inputs);
#endif 
		showTimeMark = 1;
		break;
	 case  PLOT:
		if (src_start_node == NULL)
	     	     return;
		if (in_browse_mode)
		     return;
		define_expand_range(2);
		if (draw_start_node == NULL)
	   	    return;
        	if(setplotter())
	   	    return;
		old_off = xOffset;
		old_diff = xDiff;
		old_mult = xMult;
		old_ratio = xRatio;
		dpsPlot = 1;
		xOffset = 0;
		xMult = 1.0;
		set_xy_params();
		simulate_psg(transient-1, 1);
		calculate_x_ratio(disp_start_node, NULL, 0);
		if (old_mult != 1.0 || old_off != 0)
		    define_expand_range(0);
		draw_dps();
		page(0, NULL);

		/* recover from plotter mode */
		dpsPlot = 0;
        	setdisplay();
		xOffset = 0;
		xMult = 1.0;
		set_xy_params();
		simulate_psg(transient-1, 1);
		calculate_x_ratio(disp_start_node, NULL, 0);
		xOffset = old_off;
		xMult = old_mult;
		xRatio = old_ratio;
		xDiff = old_diff;
		if (old_mult != 1.0)
	   	    measure_xpnt(disp_start_node, NULL, 3, 0, 0);
           	Wactivate_mouse(mouse_move, mouse_but, NULL);
		break;
	}
}

#ifdef MOTIF
static void
misc_but_proc(widget, client_data, call_data)
Widget  widget;
caddr_t  client_data;
caddr_t  call_data;
{
	do_but_proc((int) client_data);
}
#endif

static  void
stop_dps_browse()
{
	if (shellWin && in_browse_mode)
	{
	    inittimer(0.0, 0.0, NULL);
	    in_browse_mode = 0;
	    create_attr2_buttons(butpan_4, 1);
	}
}

#ifdef MOTIF
static void
dps_browse_proc(widget, cl_data, call_data)
Widget  widget;
caddr_t  cl_data;
caddr_t  call_data;
{
	int	num;

	num = (int) cl_data;
	switch (num) {
	 case  FASTER:
		if (browse_timer > 1.0)
		     browse_timer -= 1.0;
		else if (browse_timer > 0.2)
		     browse_timer -= 0.2;
		break;
	 case  SLOWER:
		if (browse_timer < 1.0)
		     browse_timer += 0.2;
		else
		     browse_timer += 1.0;
		break;
	 case  STOPBR:
		stop_dps_browse();
		break;
	}
}
#endif


static 
ARRAY_WIDGET *new_w_node()
{
	ARRAY_WIDGET  *node;

	if((node = (ARRAY_WIDGET *) allocateWithId 
             (sizeof(ARRAY_WIDGET), "dpswnode")) == 0)
	   return(NULL);
	node->index = 0;
	node->next = NULL;
	node->prev = NULL;
	return(node);
}


#ifdef MOTIF
static void
draw_b_button(node, button, focus)
ARRAY_WIDGET  *node;
int	      button, focus;
{
	int	x, y, h, w;

	if (button == UP)
	     x = node->ux;
	else
	     x = node->dx;
	y = node->y - text_ascent;
	
	if (!focus)
	{
	     XClearArea(dpy, infoWin, x, y, bWidth, bWidth, FALSE);
	     XSetForeground (dpy, dps_text_gc, winFg);
	     XDrawRectangle(dpy, infoWin, dps_text_gc, x, y, bWidth, bWidth);
	     menu_color(BLUE);
	}
	else
	{
	     XSetForeground (dpy, dps_text_gc, winFg);
	     XFillRectangle(dpy, infoWin, dps_text_gc, x, y, bWidth, bWidth);
	     XSetForeground (dpy, dps_text_gc, winBg);
	}
	w = x + bWidth / 2 - 1;
	XFillRectangle(dpy, infoWin, dps_text_gc, w, y + 3, 3, bWidth - 5);
	w = bWidth - 2;
	h = w / 2;
	x++;
	if (button == UP)
	{
	     y = y + h + 2;
	     while ( h > 0 )
	     {
		XDrawLine(dpy, infoWin, dps_text_gc, x, y, x + w, y);
		h--;
		y--;
		if (w > 1)
		{
		    x++;
		    w -= 2;
		}
	     }
	}
	else
	{
	     y = y + bWidth - h - 2;
	     while ( h > 0 )
	     {
		XDrawLine(dpy, infoWin, dps_text_gc, x, y, x + w, y);
		h--;
		y++;
		if (w > 1)
		{
		    x++;
		    w -= 2;
		}
	     }
	}
}

static void
change_array_info()
{
	int	      k;
	ARRAY_NODE    *anode;

	if (cur_array_node == NULL)
	     return;
	anode = cur_array_node->anode;
	if (cur_array_node->active == UP)
	     k = anode->index + 2;
	else
	     k = anode->index;
	if (k <= 0 || k > anode->vars)
	{
	    inittimer(0.0, 0.0, NULL);
	    return;
	}
	anode->index = k - 1;
	draw_array_data(anode, 1);
	time_num++;
	if (time_num > 1)
	    inittimer(0.2, 0.0, change_array_info);
	else
	    inittimer(0.8, 0.0, change_array_info);
}
#endif /* MOTIF */


static void
modify_array_name(anode, data)
ARRAY_NODE    *anode;
char	      *data;
{
	ARRAY_NODE    *xnode;
	char	      data2[16];
	int   v, k;

	if (strcmp("nt", anode->name) == 0) {
	    for (k = 0; k < anode->vars; k++)
	    {
    	       v = (int) dps_get_real(CURRENT,"nt",k+1, 1.0);
		if (v > sys_nt)
		    sys_nt = v;
	    }
	}
	if (anode->code == XARRAY)
	{
	    sprintf(data, "(%s", anode->name);
	    xnode = anode->dnext;
	    while (xnode != NULL)
	    {
		strcat(data, ",");
		strcat(data, xnode->name);
		xnode = xnode->dnext;
	    }
	    sprintf(data2, ")[%d]: ", anode->vars);
	    strcat(data, data2);
	}
	else if (anode->code == SARRAY)
	    sprintf(data, "%s[%d]: ", anode->name, anode->vars);
	else
	{
	   if (anode->id >= 0 && anode->id <= 255)
		sprintf(data, "%s(%d)[%d]: ", anode->name, anode->id, anode->vars);
	   else
	       sprintf(data, "%s[%d]: ", anode->name, anode->vars);
	}
}


#ifndef VNMRJ

static void
disp_array_node()
{
#ifdef MOTIF
	int	      sLine;
	ARRAY_WIDGET  *node;
	ARRAY_NODE    *anode;


	if (arrayList == NULL)
	    return;
	menu_color(BLUE);
	sprintf(info_data, "Arraydim:  %d", arraydim);
	XDrawString (dpy, infoWin, dps_text_gc, XGAP, text_h,
				info_data,strlen(info_data));
	sLine = 0;
	node = arrayList;
	while (node != NULL)
	{
	    menu_color(BLUE);
	    anode = node->anode;
	    modify_array_name(anode, info_data);
	    XDrawString (dpy, infoWin, dps_text_gc, XGAP, node->y,
				info_data,strlen(info_data));
	    draw_array_data(anode, 0);
	    draw_b_button(node, UP, 0);
	    draw_b_button(node, DOWN, 0);
	    if (sLine == 0 && anode->code != SARRAY && anode->code != XARRAY)
	    {
		XSetForeground (dpy, dps_text_gc, winFg);
		XDrawLine(dpy, infoWin, dps_text_gc, 0, node->y - text_h - 4,
				winWidth, node->y - text_h - 4);
		sLine = 1;
	    }
	    node = node->next;
	}
	XSetForeground (dpy, dps_text_gc, winFg);
	XDrawLine(dpy, infoWin, dps_text_gc, 0, time_y-1, winWidth, time_y-1);
#endif /* MOTIF */
}
#endif



#ifdef MOTIF
static void
b_button_func(widget, c_data, x_event)
Widget  widget;
caddr_t c_data;
XEvent  *x_event;
{
        int     k, x, y, y1;
	ARRAY_WIDGET  *node;

	if (arrayList == NULL)
	    return;
        k = (int) c_data;
	if (k == 0)
	{
	    if (cur_array_node != NULL)
	    {
		inittimer(0.0, 0.0, NULL);
		draw_b_button(cur_array_node, cur_array_node->active, 0);
		cur_array_node = NULL;
	    }
	    return;
	}
	cur_array_node = NULL;
	x = x_event->xbutton.x;
	y = x_event->xbutton.y;
	if ( y >= time_y)
	    return;
	node = arrayList;
	while (node != NULL)
	{
	    y1 = node->y - bWidth;
	    if (y < y1)
		return;
	    if (node->y >= y)
		break;
	    node = node->next;
	}
	if (node == NULL)
	    return;
	node->active = 0;
	if (x >= node->ux)
	{
	    if (x <= node->ux + bWidth)
		node->active = UP;
	   else if (x >= node->dx && x <= node->dx + bWidth)
		node->active = DOWN;
	}
	if (node->active == 0)
	    return;

	cur_array_node = node;
	time_num = 0;
	draw_b_button(cur_array_node,cur_array_node->active, 1);
	change_array_info();
}
#endif /* MOTIF */

static void
clear_array_list()
{
	if (arrayList != NULL)
	{
	    releaseWithId("dpswnode");
	    arrayList = NULL;
	}
}

static void
clean_dps_info_win()
{
#ifdef MOTIF
	int	k;

	if (shellWin == 0)
	    return;
	for(k = 0; k < ARRAYMAX; k++)
	{
	    if (array_i_widgets[k] != NULL)
	    	XtUnmapWidget(array_i_widgets[k]);
	}
	XClearWindow(dpy, infoWin);
#endif
}

#ifndef VNMRJ

static void
position_array_pannel()
{
#ifdef MOTIF
	int	      x, y, len, sLine;
	int	      num;
	Position      posx, posy;
	ARRAY_WIDGET  *wnode;
	ARRAY_NODE    *anode;

	bWidth = (text_h / 2 - 1) * 2;
	y = text_h;
	sLine = 0;
	wnode = arrayList;
	num = 0;
	while (wnode != NULL)
	{
	    anode = wnode->anode;
	    if (anode->code != SARRAY && anode->code != XARRAY && !sLine)
	    {
	    	y += 2;
		sLine = 1;
	    }
	    wnode->y = y + txHeight;
	    modify_array_name(anode, inputs);
	    len = strlen(inputs);
	    x = text_w * len + XGAP;
	    wnode->ux = x;
	    x = x + bWidth + XGAP;
	    wnode->dx = x;
	    x = x + bWidth + XGAP;
	    if (num < ARRAYMAX && array_i_widgets[num] != NULL)
	    {
	       anode->index_widget = array_i_widgets[num];
#ifdef OLIT
	       XtSetArg (args[0], XtNuserData, (XtPointer) anode);
#else 
	       XtSetArg (args[0], XmNuserData, (XtPointer) anode);
#endif 
               XtSetValues(array_t_widgets[num], args, 1);

	       posx = x;
#ifdef OLIT
	       posy = y + 6;
#else 
	       posy = y + txHeight - text_h;
#endif 
	       XtMoveWidget(array_i_widgets[num], posx, posy);
		if (anode->code == SARRAY || anode->code == XARRAY)
	   	  sprintf(info_data, "%d", anode->index + 1);
	 	else
	   	  sprintf(info_data, "%d", anode->index);
#ifdef OLIT
               XtSetArg (args[0], XtNstring, info_data);
               XtSetValues(array_i_widgets[num], args, 1);
#else 
	       XmTextSetString(array_i_widgets[num], info_data);
#endif 
	       XtMapWidget(array_i_widgets[num]);
	    }
	    wnode->lx = x + txWidth + 4;
	    anode->tx = x + txWidth + 4;
	    y = y + txHeight;
	    anode->ty = y;
	    num++;
	    wnode = wnode->next;
	}
	for(len = num; len < ARRAYMAX; len++)
	    XtUnmapWidget(array_i_widgets[len]);
	if (num <= 0)
	    time_y =  0;
	else
	    time_y = txHeight * (num + 1) + text_descent + 2;
	scan_y =  time_y + text_h;
	info_y = scan_y + text_h + text_descent + 2;
#endif /* MOTIF */
}
#endif


static void
create_array_list()
{
	ARRAY_WIDGET  *wnode, *pwnode;
	ARRAY_NODE    *anode, *xnode;

	clear_array_list();
	newArray = 0;
	anode = array_start_node;
	while (anode != NULL)
	{
	    if (anode->code == XARRAY)  /* diagonal array */
	    {
		xnode = anode->next;
		while (xnode != NULL)
		{
		    if ((xnode->code == XARRAY) && (xnode->id == anode->id))
		    {
			anode->dnext = xnode;
			xnode->linked = 1;
			break;
		    }
		    xnode = xnode->next;
		}
	     }
	     anode = anode->next;
	}
	anode = array_start_node;
        pwnode = arrayList;
	while (anode != NULL)
	{
	    if (anode->linked)
	    {
		anode = anode->next;
		continue;
	    }
	    wnode = new_w_node();
	    anode->index_widget = 0;
	    if (wnode == NULL)
		break;
	    if (arrayList == NULL)
	        arrayList = wnode;
	    else
	    {
		pwnode->next = wnode;
		wnode->prev = pwnode;
	    }
	    pwnode = wnode;
	    wnode->anode = anode;
	    anode = anode->next;
	}
}

#ifndef VNMRJ

static void
create_array_pannel()
{
	create_array_list();
	position_array_pannel();
}
#endif 

static void
jsend_array_value(node)
ARRAY_NODE    *node;
{
#ifdef VNMRJ
	ARRAY_NODE    *xnode;

        xnode = node;
	if (node->code == SARRAY || node->code == XARRAY)
	    sprintf(info_data, "array val %d %d ", xnode->jid, node->index+1);
	else
	    sprintf(info_data, "array val %d %d ", xnode->jid, node->index);
        while (xnode != NULL)
        {
            xnode->index = node->index;
            if (xnode->type == T_STRING)
            {
                if (xnode->value.Str[xnode->index] == NULL)
                    return;
                sprintf(inputs, "%s", xnode->value.Str[xnode->index]);
            }
            else
            {
                if (xnode->ni)
                    sprintf(inputs, "%g", xnode->value.Val[0] +
                                xnode->value.Val[1] * xnode->index);
                else
                    sprintf(inputs, "%g", xnode->value.Val[xnode->index]);
            }
            strcat(info_data, inputs);
            xnode = xnode->dnext;
            if (xnode != NULL)
                strcat (info_data, ", ");
	}
	writelineToVnmrJ("dps", info_data);
#endif 
}


static void
send_info_to_vj()
{
#ifdef VNMRJ
	ARRAY_WIDGET  *wnode;
        ARRAY_NODE    *anode;
	int num;

	writelineToVnmrJ("dps", "clear");
	sprintf(info_data, "array dim  arraydim: %d", arraydim);
	writelineToVnmrJ("dps", info_data);
	create_array_list();
	wnode = arrayList;
        num = 0;
        while (wnode != NULL)
        {
            anode = wnode->anode;
            anode->jid = num;
	    if (anode->code == SARRAY || anode->code == XARRAY)
	       sprintf(info_data, "array new %d %d 1 %d", num, anode->code, anode->vars);
	    else
	       sprintf(info_data, "array new %d %d 0 %d", num, anode->code, anode->vars - 1);
	    writelineToVnmrJ("dps", info_data);
	    modify_array_name(anode, inputs);
	    sprintf(info_data, "array name %d %s ", num, inputs);
	    writelineToVnmrJ("dps", info_data);
	    jsend_array_value(anode);
	    wnode = wnode->next;
	    num++;
        }
	disp_dps_time();
	writelineToVnmrJ("dps", "array end");
        	if (dispMode & TIMEMODE)
		    writelineToVnmrJ("dps", "class time");
        	else if (dispMode & POWERMODE)
		    writelineToVnmrJ("dps", "class power");
        	else if (dispMode & PHASEMODE)
		    writelineToVnmrJ("dps", "class phase");
		if(dispMode & DSPLABEL)
		    writelineToVnmrJ("dps", "mode label");
		if(dispMode & DSPVALUE)
		    writelineToVnmrJ("dps", "mode value");
		if(dispMode & DSPMORE)
		    writelineToVnmrJ("dps", "mode more");
		if(dispMode & DSPDECOUPLE)
		    writelineToVnmrJ("dps", "mode decoupling");

#endif 
}

static void
update_vj_array(argc, argv)
int argc;
char *argv[];
{
#ifdef VNMRJ
    int id, type, index;
    ARRAY_WIDGET  *wnode;
    ARRAY_NODE    *anode;

    if (argc < 6)
	return;
    id = atoi(argv[3]);
    type = atoi(argv[4]);
    index = atoi(argv[5]);
    wnode = arrayList;
    while (wnode != NULL)
    {
	anode = wnode->anode;
        if (anode->jid == id && anode->code == type) {
	     anode->index = index;
	     jsend_array_value(anode);
	     return;
	}
	wnode = wnode->next;
    }
#endif 
}

#ifndef VNMRJ
static void
popup_dps_pannel()
{

#ifdef MOTIF
	if (!Wissun())
	    return;

#ifdef VNMRJ
	if (shellWin == 0)
	    return;
#endif 
	if (dpsShell == NULL)
	     create_dps_pannel();
	if (shellWin)
	{
	     menuUp = 1;
	     if (!XtIsManaged(dpsShell))
		XtMapWidget(dpsShell);
	     if (array_start_node == NULL)
	     {
#ifdef OLIT
		XtUnmapWidget(dps_but2_rec[4]->button);
#else 
		if (button_2[4] != NULL)
		    XtUnmapWidget(button_2[4]);
#endif 
	     }
	     else if (!in_browse_mode)
	     {
#ifdef OLIT
		XtMapWidget(dps_but2_rec[4]->button);
#else 
		if (button_2[4] != NULL)
		    XtMapWidget(button_2[4]);
#endif 
	     }
	     if (newArray)
		create_array_pannel();
	     XClearWindow(dpy, infoWin);
	     disp_array_node();
	     disp_dps_time();
	     if (debug > 1)
		fprintf(stderr, " popup menu \n");
	     XtPopup (dpsShell, XtGrabNone);
	     XRaiseWindow(dpy, shellWin);
	     draw_menu_icon();
	}
#endif /* MOTIF */
}
#endif 

static void
disp_dps_time()
{
#ifdef VNMRJ
	if (expTime != -3.0)
	    convert_time_val(expTime);
	else
	    strcpy(inputs, " ");
	sprintf(info_data, "exptime Exp Time:  %s", inputs);
	writelineToVnmrJ("dps", info_data);
	print_transient();
	return;
#else 
	int	y;

	if (shellWin == NULL)
		return;
	clear_time_window();
	y = time_y + text_h;
	menu_color(BLUE);
	if (expTime != -3.0)
	    convert_time_val(expTime);
	else
	    strcpy(inputs, " ");
	sprintf(info_data, "Exp Time:  %s", inputs);
	XDrawString (dpy, infoWin, dps_text_gc, XGAP, y,info_data,
				strlen(info_data));
	print_transient();
	XSetForeground (dpy, dps_text_gc, winFg);
	XDrawLine(dpy, infoWin, dps_text_gc, 0, info_y-1, winWidth, info_y-1);
#endif 
}



static void
clear_dps_info()
{
#ifdef VNMRJ
        writelineToVnmrJ("dps", "info new \n");
#else 
	if (shellWin == NULL)
		return;
	XFlush(dpy);
	clear_info_window();
	line_num = 1;
#endif 
}


#ifndef VNMRJ
static void
put_dps_info(char *data)
{
#ifdef MOTIF
	if ((shellWin == 0) || (data == NULL))
	     return;
	XDrawString (dpy, infoWin, dps_text_gc, XGAP, info_y + line_num * text_h, data,
				 strlen(data));
	line_num++;
#endif
}
#endif

#ifndef VNMRJ

static void
expose_panel()
{
	disp_array_node();
	disp_dps_time();
	if (hilitNode)
            disp_info(hilitNode);
}

static void
create_class_buttons(parent)
 Widget parent;
{
#ifdef MOTIF
	int	m;

	classBut[0] = create_toggle_button(parent, "Timing", 
				class_but_proc, TIMEMODE);
	classBut[1] = create_toggle_button(parent, "Power",
			 	class_but_proc, POWERMODE);
	classBut[2] = create_toggle_button(parent, "Phase",
			 	class_but_proc, PHASEMODE);
        if (dispMode & TIMEMODE)
	     m = 0;
	else if (dispMode & POWERMODE)
	     m = 1;
	else
	     m = 2;
        XtSetArg (args[0], XmNset, TRUE);
	XtSetValues(classBut[m], args, 1);
#endif
}

static void
create_mode_buttons(parent)
 Widget parent;
{
#ifdef MOTIF
	modeBut[0]= create_toggle_button(parent, "Label", 
				mode_but_proc, DSPLABEL);
	modeBut[1]= create_toggle_button(parent, "Value",
				mode_but_proc, DSPVALUE);
	modeBut[2]= create_toggle_button(parent, "More",
				mode_but_proc, DSPMORE);
        XtSetArg (args[0], XmNset, TRUE);
	if(dispMode & DSPLABEL)
	    XtSetValues(modeBut[0], args, 1);
	if(dispMode & DSPVALUE)
	    XtSetValues(modeBut[1], args, 1);
	if(dispMode & DSPMORE)
	    XtSetValues(modeBut[2], args, 1);
#endif
}

static void
show_dps_menu(w, data, ev)
Widget          w;
caddr_t         data;
XEvent          *ev;
{
#ifdef MOTIF
        if (ev->xbutton.button != 3)
                return;
        XmMenuPosition(dpsMenu, (XButtonPressedEvent *)ev);
        XtManageChild(dpsMenu);
#endif 
}

#endif



#ifndef VNMRJ
static void
create_dps_pannel()
{
#ifdef MOTIF
	Widget  twidget;
#ifdef OLIT
	Widget  pwidget;
#endif 
	Dimension  tw, th;
        int n;

	if (debug > 1)
	    fprintf(stderr, " create dps pannel ...\n");
	if (menuW > 0 && menuH > 0)
	    sprintf(info_data, "%dx%d+%d+%d", menuW, menuH, menuX, menuY);
	else
	    strcpy(info_data, "300x360-0+0");
        n = 0;
        XtSetArg (args[n], XtNallowShellResize, TRUE); n++;
        XtSetArg (args[n], XtNtitle, "Dps panel"); n++;
        XtSetArg (args[n], XtNiconName, "dps"); n++;
        XtSetArg (args[n], XtNgeometry, info_data); n++;
        XtSetArg (args[n], XtNmappedWhenManaged, FALSE); n++;
	XtSetArg(args[n], XtNallowShellResize, TRUE);  n++;
#ifdef OLIT
        XtSetArg(args[n], XtNresizeCorners, TRUE);  n++;
#endif 
	dpsShell = XtCreateApplicationShell("Vnmr", transientShellWidgetClass,
			args, n);

	dpy = XtDisplay(dpsShell);
	if (tfontInfo == NULL)
	{
	   if((tfontInfo = XLoadQueryFont(dpy, textFont)) == NULL)
               tfontInfo = XLoadQueryFont(dpy, "9x15");
           text_ascent = tfontInfo->max_bounds.ascent;
	   text_descent = tfontInfo->max_bounds.descent;
	   text_h = text_ascent + text_descent;
	   text_w = tfontInfo->max_bounds.width;
	}

	time_y = 0;
	scan_y = text_h;
	info_y = text_h * 2 + text_descent + 2;

        n = 0;
#ifdef  OLIT
	dps_but_gc = DefaultGC(dpy, DefaultScreen(dpy));
	dpsFrame = (Widget)XtCreateManagedWidget("dps",formWidgetClass, 
				dpsShell, args, n);
	twidget = (Widget)create_rc_widget(dpsFrame,NULL,&butpan_1,XOR,"Class:");
        XtSetArg (args[0], XtNbackground, &winBg);
        XtSetArg (args[1], XtNforeground, &winFg);
        XtGetValues(twidget, args, 2);

 	create_class_buttons(butpan_1);

	twidget = (Widget)create_rc_widget(dpsFrame,twidget,&butpan_2,OR,"Show: ");
	create_mode_buttons(butpan_2);
	n = 0;
	XtSetArg (args[n], XtNfont, &bfontInfo); n++;
        XtSetArg (args[n], XtNfontColor, &winFg); n++;
        XtSetArg (args[n], XtNbackground, &winBg); n++;
	XtGetValues(modeBut[0], args, n);

        but_ascent = bfontInfo->max_bounds.ascent;
	but_h = but_ascent + bfontInfo->max_bounds.descent + 8;
	but_w = bfontInfo->max_bounds.width;
	XSetFont(dpy, dps_but_gc, bfontInfo->fid);
	
	butpan_3 = create_blt_widget(dpsFrame, twidget, but_w * 12, but_h * 2 + 12);
	create_attr_buttons(butpan_3);
	for(n = 0; n < BUTNUM2; n++)
	    dps_but2_rec[n] = NULL;
	create_attr2_buttons(butpan_3, 0);
	setup_button_env(dpy, butpan_3);
	butpan_4 = butpan_3;

        n = 0;
        XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
        XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
        XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
        XtSetArg (args[n], XtNxResizable, TRUE);  n++;
	XtSetArg (args[n], XtNborderWidth, 1); n++;
	XtSetArg (args[n], XtNyResizable, TRUE);  n++;
	XtSetArg (args[n], XtNyRefWidget, butpan_3); n++;
	XtSetArg (args[n], XtNyAttachBottom, TRUE);  n++;
        drawPannel = XtCreateManagedWidget("", drawAreaWidgetClass,
                         dpsFrame, args, n);

#else 
	dpsFrame = (Widget)XmCreateForm(dpsShell, "dps", args, n);
	butpan_1 = (Widget) create_rc_widget(dpsFrame, NULL, XOR, "Class:");
 	create_class_buttons(butpan_1);
	butpan_2 = (Widget) create_rc_widget(dpsFrame, butpan_1, OR, "Show: ");
	create_mode_buttons(butpan_2);
	butpan_3 = (Widget) create_blt_widget(dpsFrame, butpan_2, 0);
	XtManageChild(butpan_3);
	create_attr_buttons(butpan_3);
	butpan_4 = (Widget) create_blt_widget(dpsFrame, butpan_3, 0);
	XtManageChild(butpan_4);
	for(n = 0; n < 5; n++)
	    button_2[n] = NULL;
	for(n = 0; n < 3; n++)
	    button_3[n] = NULL;
	create_attr2_buttons(butpan_4, 0);

	n = 0;
        XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);  n++;
        XtSetArg (args[n], XmNresizable, TRUE);  n++;
        XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
        XtSetArg (args[n], XmNtopWidget, butpan_4); n++;
	XtSetArg (args[n], XmNborderWidth, 1); n++;
	drawPannel = XtCreateManagedWidget("", xmDrawingAreaWidgetClass,
                         dpsFrame, args, n);
        XtSetArg (args[0], XtNbackground, &winBg);
        XtSetArg (args[1], XtNforeground, &winFg);
        XtGetValues(drawPannel, args, 2);

#endif  /* OLIT */
	XtManageChild(dpsFrame);

	XtRealizeWidget (dpsShell);

	infoWin = XtWindow (drawPannel);
	dps_text_gc = XCreateGC(dpy, infoWin, 0, 0);
	XSetFont(dpy, dps_text_gc, tfontInfo->fid);
        n = 0;
        XtSetArg (args[n], XtNwidth, &tw); n++;
        XtSetArg (args[n], XtNheight, &th); n++;
        XtGetValues(dpsShell, args, n);

#ifdef  OLIT
	create_but_corner_shape(dpy, dps_but_gc, 4, but_h);
	set_dps_but_xid();
	XSetForeground (dpy, dps_but_gc, winFg);

        OlAddCallback( dpsShell, XtNwmProtocol, dpsShell_exit, NULL );
	XtAddCallback(drawPannel, XtNexposeCallback, expose_panel, 1);
#else 
	XtAddCallback(drawPannel, XmNexposeCallback,
                      (XtCallbackProc) expose_panel, (XtPointer) 1);
        deleteAtom = XmInternAtom(dpy, "WM_DELETE_WINDOW",FALSE);
        XmAddProtocolCallback(dpsShell, XM_WM_PROTOCOL_ATOM(dpsShell),
                        deleteAtom, (XtCallbackProc) dpsShell_exit, (XtPointer) 0);
#endif 
        XtSetArg(args[0], XtNwidth, &winWidth);
        XtSetArg(args[1], XtNheight, &winHeight);
	XtGetValues(drawPannel, args, 2);
	create_array_index_widget(drawPannel);
	n = 0;
        XtSetArg (args[n], XtNwidth, &tw); n++;
        XtSetArg (args[n], XtNheight, &th); n++;
	XtGetValues(array_i_widgets[0], args, n);
	txWidth = (int) tw;
	txHeight = (int) th + 2;

	XtAddEventHandler(drawPannel, ButtonPressMask, False,
			 (XtEventHandler) b_button_func, (XtPointer)1);
	XtAddEventHandler(drawPannel, ButtonReleaseMask, False,
			 (XtEventHandler) b_button_func, (XtPointer)0);


#ifdef  OLIT
	n = 0;
        XtSetArg (args[n], XtNallowShellResize, TRUE); n++;
        XtSetArg (args[n], XtNresizeCorners, FALSE); n++;
        dpsMenu = XtCreatePopupShell("Menu",
                         menuShellWidgetClass, drawPannel, args, n);
        XtSetArg (args[0], XtNmenuPane, &pwidget);
        XtGetValues (dpsMenu, args, 1);
	n = 0;
        XtSetArg (args[n], XtNlabel, "Properties...");  n++;
        twidget = XtCreateManagedWidget("", oblongButtonWidgetClass,
				 pwidget, args, n);
        XtAddCallback(twidget, XtNselect, dps_menu_proc, (XtPointer) PROPERTY);
        n = 0;
        XtSetArg (args[n], XtNlabel, "Close");  n++;
        twidget = XtCreateManagedWidget("", oblongButtonWidgetClass,
				 pwidget, args, n);
        XtAddCallback(twidget, XtNselect, dps_menu_proc, (XtPointer) CLOSE);
#else 
	dpsMenu = (Widget) XmCreatePopupMenu(dpsFrame, "Menu", NULL, 0);
        xmstr = XmStringLtoRCreate("Properties...", XmSTRING_DEFAULT_CHARSET);
        XtSetArg(args[0], XmNlabelString, xmstr);
        twidget = (Widget) XmCreatePushButton(dpsMenu, "button", args, n);
        XtAddCallback(twidget, XmNactivateCallback, (XtCallbackProc)dps_menu_proc,
				(XtPointer)PROPERTY);
        XtManageChild(twidget);
        xmstr = XmStringLtoRCreate("Close", XmSTRING_DEFAULT_CHARSET);
        XtSetArg(args[0], XmNlabelString, xmstr);
        twidget = (Widget) XmCreatePushButton(dpsMenu, "button", args, n);
        XtAddCallback(twidget, XmNactivateCallback, (XtCallbackProc)dps_menu_proc,
				(XtPointer)CLOSE);
        XtManageChild(twidget);
	XtAddEventHandler(dpsFrame, ButtonPressMask, False,
			(XtEventHandler) show_dps_menu,NULL);

#endif  /* OLIT */

	shellWin = XtWindow (dpsShell);
        XtAddEventHandler(dpsShell,SubstructureNotifyMask, False, 
			(XtEventHandler) resize_shell, NULL);

        if (debug > 1)
            fprintf(stderr, " create dps pannel done.\n");
	newArray = 1;
#endif  /* MOTIF */
}

#endif 

static void
save_dps_resource()
{
	FILE   *fd;

#ifndef VNMRJ
	if (menuH <= 0 || menuW <= 0) /* menu never open */
	    return;
#endif 
	sprintf(inputs, "%s/templates/dps/defaults", userdir);
        fd = fopen(inputs,"w+");
        if (fd == NULL)
        {
	    sprintf(inputs,"%s/templates/dps",userdir);
            do_mkdir(inputs, 1, 0777);
	    sprintf(inputs, "%s/templates/dps/defaults", userdir);
            fd = fopen(inputs,"w+");
            if (fd == NULL)
	       return;
	}
	/* geometry */
	fprintf(fd, "1 %d %d %d %d\n", menuX, menuY, menuW, menuH);
	/* mode */
	fprintf(fd, "2 %d\n", dispMode);
	fprintf(fd, "3 %d\n", rfShapeType);

	fclose(fd);
}

static void
read_dps_resource()
{
	FILE    *fd;
	char	*data;
	int	type;

	sprintf(inputs, "%s/templates/dps/defaults", userdir);
        fd = fopen(inputs,"r");
        if (fd == NULL)
	    return;
	while ((data = fgets(inputs, 256, fd)) != NULL)
        {
	    if (sscanf(data, "%d", &type) == 1)
	    {
		switch (type) {
		 case 1:  /* geom */
			sscanf(data, "%d%d%d%d%d", &type, &menuX, &menuY, 
					&menuW, &menuH);
			break;
		 case 2:  /* mode */
			sscanf(data, "%d%d", &type, &dispMode);
			break;
		 case 3:  /* rf shape */
                        rfShapeType = ABS_SHAPE;
			sscanf(data, "%d%d", &type, &rfShapeType);
                        if (rfShapeType != REAL_SHAPE && rfShapeType != IMAGINARY_SHAPE)
                            rfShapeType = ABS_SHAPE;
			break;
		}
	     }
	}
	fclose(fd);
}

#ifndef VNMRJ

#ifdef OLIT
static
recreate_array_index_widget()
{
}

#else 


static void
recreate_array_index_widget()
{
#ifdef MOTIF
	int	k;
	Dimension  tw, th;

	if (drawPannel == NULL)
	    return;
	if (array_i_widgets[0] == NULL)
	    return;
	for (k = 0; k < ARRAYMAX - 1; k++)
	{
	    XtDestroyWidget(array_i_widgets[k]);
	    array_i_widgets[k] = NULL;
	}
	create_array_index_widget(drawPannel);

        XtSetArg (args[0], XtNwidth, &tw);
        XtSetArg (args[1], XtNheight, &th);
	XtGetValues(array_i_widgets[0], args, 2);
	txWidth = (int) tw;
	txHeight = (int) th + 2;
	clean_dps_info_win();
	position_array_pannel();
	expose_panel();
#endif 
}

#endif 
#endif 
#endif 

