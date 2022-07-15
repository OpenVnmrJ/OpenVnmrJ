/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/***************/
/* graphics.h  */
/***************/

#ifndef graphics_header_included
#define graphics_header_included

#define MODELESS        0
#define SELECT_MODE     1
#define ZOOM_MODE       2
#define PAN_MODE        3
#define INSET_MODE      4
#define VS_MODE         5
#define STRETCH_MODE    6
#define PHASE_MODE    7
#define THRESHOLD_MODE    8
#define REGION_MODE    9
#define LVLTLT_MODE    10
#define TRACE_MODE    11
#define SPWP_MODE    12
#define SCWC_MODE    13
#define USER_MODE    14

#define MONOCHROME   111

#define MAX_PLANES  4                              /* for raster plotting */
                        /* HP-DeskJet printer has 4 color ink cartridges */
                                      /* Cyan, Magenta, Yellow and Black */
#define	BLACK	0
#define	RED	1
#define	YELLOW	2
#define	GREEN	3
#define	CYAN	4
#define	BLUE	5
#define	MAGENTA	6
#define	WHITE	7

#define BG_IMAGE_COLOR  (WHITE+1)	/* color used for spectrum   */
#define FID_COLOR       (WHITE+2)	/* color used for fid        */
#define IMAG_COLOR      (WHITE+3)
#define SPEC_COLOR      (WHITE+4)	/* color used for spectrum   */
#define INT_COLOR       (WHITE+5)	/* color used for integral   */
#define PARAM_COLOR     (WHITE+6)	/* color used for parameters */
#define SCALE_COLOR     (WHITE+7)	/* color used for scale      */
#define THRESH_COLOR    (WHITE+8)
#define SPEC2_COLOR     (WHITE+9)	/* color used for spectrum   */
#define SPEC3_COLOR     (WHITE+10)	/* color used for spectrum   */
#define CURSOR_COLOR    (WHITE+11)	/* color used for parameters */
#define FG_IMAGE_COLOR  (WHITE+12)	/* color used for spectrum   */

// these contour colors are no longer used by dcon or dpcon.
// but they are still used by dsww and dfww.
#define FIRST_AV_COLOR		(FG_IMAGE_COLOR+1)
#define NUM_AV_COLORS		16
#define CONT0_COLOR		(FIRST_AV_COLOR) // CONT0_COLOR = 20
#define CONT1_COLOR		(CONT0_COLOR+1)
#define CONT2_COLOR		(CONT0_COLOR+2)
#define CONT3_COLOR		(CONT0_COLOR+3)
#define CONT4_COLOR		(CONT0_COLOR+4)
#define CONT5_COLOR		(CONT0_COLOR+5)
#define CONT6_COLOR		(CONT0_COLOR+6)
#define CONT7_COLOR		(CONT0_COLOR+7)
#define CONT8_COLOR		(CONT0_COLOR+8)
#define CONT9_COLOR		(CONT0_COLOR+9)
#define CONT10_COLOR		(CONT0_COLOR+10)
#define CONT11_COLOR		(CONT0_COLOR+11)
#define CONT12_COLOR		(CONT0_COLOR+12)
#define CONT13_COLOR		(CONT0_COLOR+13)
#define CONT14_COLOR		(CONT0_COLOR+14)
#define CONT15_COLOR		(CONT0_COLOR+15)

#define FIRST_PH_COLOR		(FIRST_AV_COLOR+NUM_AV_COLORS) // FIRST_PH_COLOR=36
#define NUM_PH_COLORS		15
#define ZERO_PHASE_LEVEL	(FIRST_PH_COLOR+7)
#define CONTM7_COLOR		(ZERO_PHASE_LEVEL-7)
#define CONTM6_COLOR		(ZERO_PHASE_LEVEL-6)
#define CONTM5_COLOR		(ZERO_PHASE_LEVEL-5)
#define CONTM4_COLOR		(ZERO_PHASE_LEVEL-4)
#define CONTM3_COLOR		(ZERO_PHASE_LEVEL-3)
#define CONTM2_COLOR		(ZERO_PHASE_LEVEL-2)
#define CONTM1_COLOR		(ZERO_PHASE_LEVEL-1)
#define CONTP0_COLOR		(ZERO_PHASE_LEVEL)
#define CONTP1_COLOR		(ZERO_PHASE_LEVEL+1)
#define CONTP2_COLOR		(ZERO_PHASE_LEVEL+2)
#define CONTP3_COLOR		(ZERO_PHASE_LEVEL+3)
#define CONTP4_COLOR		(ZERO_PHASE_LEVEL+4)
#define CONTP5_COLOR		(ZERO_PHASE_LEVEL+5)
#define CONTP6_COLOR		(ZERO_PHASE_LEVEL+6)
#define CONTP7_COLOR		(ZERO_PHASE_LEVEL+7) // CONTP7_COLOR=50

#define PEAK_COLOR		    (CONTP7_COLOR+1)
#define NUM_COLOR		    (CONTP7_COLOR+2)
#define AV_BOX_COLOR		(CONTP7_COLOR+3)
#define PH_BOX_COLOR		(CONTP7_COLOR+4)
#define LABEL_COLOR		    (CONTP7_COLOR+5)
#define ORANGE              (CONTP7_COLOR+6)

#define XPLANE_COLOR        (CONTP7_COLOR+7)
#define YPLANE_COLOR        (CONTP7_COLOR+8)
#define ZPLANE_COLOR        (CONTP7_COLOR+9)
#define UPLANE_COLOR        (CONTP7_COLOR+10)

#define ABSVAL_FID_COLOR (CONTP7_COLOR+11)	/* color used for fid absval */
#define FID_ENVEL_COLOR (CONTP7_COLOR+12)	/* envelope around fid */

#define PINK_COLOR          (CONTP7_COLOR+13) // PINK_COLOR=63
#define GRAY_COLOR          (CONTP7_COLOR+14)

#define FIRST_GRAY_COLOR    (CONTP7_COLOR+15)  // 64
#define NUM_GRAY_COLORS		64 	// last gray color 128
#define GRAYCUR			100

#define LAST_GRAY_COLOR    (FIRST_GRAY_COLOR+63)  // 128 
#define VP1_COLOR	  (LAST_GRAY_COLOR+1)	
#define VP2_COLOR	  (LAST_GRAY_COLOR+2)	
#define VP3_COLOR	  (LAST_GRAY_COLOR+3)	
#define VP4_COLOR	  (LAST_GRAY_COLOR+4)	
#define VP5_COLOR	  (LAST_GRAY_COLOR+5)	
#define VP6_COLOR	  (LAST_GRAY_COLOR+6)	
#define VP7_COLOR	  (LAST_GRAY_COLOR+7)	
#define VP8_COLOR	  (LAST_GRAY_COLOR+8)	
#define VP9_COLOR	  (LAST_GRAY_COLOR+9)	

#define VP1_NCOLOR	  (LAST_GRAY_COLOR+10) // for negative contour colors	
#define VP2_NCOLOR	  (LAST_GRAY_COLOR+11)	
#define VP3_NCOLOR	  (LAST_GRAY_COLOR+12)	
#define VP4_NCOLOR	  (LAST_GRAY_COLOR+13)	
#define VP5_NCOLOR	  (LAST_GRAY_COLOR+14)	
#define VP6_NCOLOR	  (LAST_GRAY_COLOR+15)	
#define VP7_NCOLOR	  (LAST_GRAY_COLOR+16)	
#define VP8_NCOLOR	  (LAST_GRAY_COLOR+17)	
#define VP9_NCOLOR	  (LAST_GRAY_COLOR+18)	

#define ACTIVE_COLOR            252	
#define SELECT_COLOR            253	
#define ROI_COLOR            254	

#define BOX_BACK            255 // plot box background color

#define BORDER_COLOR            256	

#define SPEC4_COLOR	(BORDER_COLOR+1)
#define SPEC5_COLOR	(BORDER_COLOR+2)
#define SPEC6_COLOR	(BORDER_COLOR+3)
#define SPEC7_COLOR	(BORDER_COLOR+4)
#define SPEC8_COLOR	(BORDER_COLOR+5)
#define SPEC9_COLOR	(BORDER_COLOR+6)
#define SPEC10_COLOR	(BORDER_COLOR+7)

#define AXIS_LABEL_COLOR	(BORDER_COLOR+8)
#define AXIS_NUM_COLOR		(BORDER_COLOR+9)
#define INTEG_MARK_COLOR	(BORDER_COLOR+10)
#define INTEG_LABEL_COLOR	(BORDER_COLOR+11)
#define INTEG_NUM_COLOR		(BORDER_COLOR+12)
#define PEAK_MARK_COLOR		(BORDER_COLOR+13)
#define PEAK_LABEL_COLOR	(BORDER_COLOR+14)
#define PEAK_NUM_COLOR		(BORDER_COLOR+15)
#define TEXT_COLOR		(BORDER_COLOR+16) // 272

// colors for phase sensitive contours
#define FIRST_PH_CLR		(TEXT_COLOR+1) // FIRST_PH_CLR=273
#define NUM_PH_CLRS	 	33	
#define ZERO_PH_LEVEL	(FIRST_PH_CLR+16)
#define CONTM16_CLR		(ZERO_PH_LEVEL-16)
#define CONTM15_CLR		(ZERO_PH_LEVEL-15)
#define CONTM14_CLR		(ZERO_PH_LEVEL-14)
#define CONTM13_CLR		(ZERO_PH_LEVEL-13)
#define CONTM12_CLR		(ZERO_PH_LEVEL-12)
#define CONTM11_CLR		(ZERO_PH_LEVEL-11)
#define CONTM10_CLR		(ZERO_PH_LEVEL-10)
#define CONTM9_CLR		(ZERO_PH_LEVEL-9)
#define CONTM8_CLR		(ZERO_PH_LEVEL-8)
#define CONTM7_CLR		(ZERO_PH_LEVEL-7)
#define CONTM6_CLR		(ZERO_PH_LEVEL-6)
#define CONTM5_CLR		(ZERO_PH_LEVEL-5)
#define CONTM4_CLR		(ZERO_PH_LEVEL-4)
#define CONTM3_CLR		(ZERO_PH_LEVEL-3)
#define CONTM2_CLR		(ZERO_PH_LEVEL-2)
#define CONTM1_CLR		(ZERO_PH_LEVEL-1)
#define CONTP0_CLR		(ZERO_PH_LEVEL)		// 289 
#define CONTP1_CLR		(ZERO_PH_LEVEL+1)
#define CONTP2_CLR		(ZERO_PH_LEVEL+2)
#define CONTP3_CLR		(ZERO_PH_LEVEL+3)
#define CONTP4_CLR		(ZERO_PH_LEVEL+4)
#define CONTP5_CLR		(ZERO_PH_LEVEL+5)
#define CONTP6_CLR		(ZERO_PH_LEVEL+6)
#define CONTP7_CLR		(ZERO_PH_LEVEL+7)
#define CONTP8_CLR		(ZERO_PH_LEVEL+8)
#define CONTP9_CLR		(ZERO_PH_LEVEL+9)
#define CONTP10_CLR		(ZERO_PH_LEVEL+10)
#define CONTP11_CLR		(ZERO_PH_LEVEL+11)
#define CONTP12_CLR		(ZERO_PH_LEVEL+12)
#define CONTP13_CLR		(ZERO_PH_LEVEL+13)
#define CONTP14_CLR		(ZERO_PH_LEVEL+14)
#define CONTP15_CLR		(ZERO_PH_LEVEL+15)
#define CONTP16_CLR		(ZERO_PH_LEVEL+16) // CONTP16_CLR=305

// absolute value contours use the same colors for positive phase contours
#define FIRST_AV_CLR		(ZERO_PH_LEVEL)
#define NUM_AV_CLRS	 	16	

#define EXP_SPEC	306
#define DS_SPEC		307
#define CRAFT_SPEC	308
#define RESIDUAL_SPEC	309
#define SUM_SPEC	310
#define MODEL_SPEC	311
#define REF_SPEC	312
#define NOT_USED	313
#define ROLI		314
#define CRAFT_ROIS	315
#define ALIGNMENT_ROIS	316
#define ALIGNMENT2_ROIS	317
#define ALIGNMENT3_ROIS	318
#define SEGMENT_ROIS	319
#define ANNO_LINE_COLOR	320

#define MAX_COLOR		   320 

#define SPEC_LINE_MIN	1
#define SPEC_LINE_MAX	2
#define AXIS_LINE	3
#define CUSOR_LINE	4
#define CROSSHAIR	5
#define THRESHOLD_LINE	6
#define INTEG_LINE	7
#define INTEG_MARK	8
#define PEAK_MARK	9

#define AXIS_LABEL	10
#define AXIS_NUM	11
#define PEAK_LABEL	12
#define PEAK_NUM	13
#define INTEG_LABEL	14
#define INTEG_NUM	15
#define TEXT		16

#define AVPeakBox	17
#define PHPeakBox	18

#define W_STATUS 	1
#define W_GRAPHICS	2
#define W_COMMAND	3
#define W_SCROLL	4

/* Here is a stupendously clever programming trick (warning) */
#ifdef FILE_IS_GDEVSW
#define uextern
#define vextern extern
#else
#ifdef FILE_IS_GRAPHICS
#define uextern extern
#define vextern
#else
#define uextern extern
#define vextern extern
#endif
#endif

/* The following items allow switching between different terminal types at    */
/* boot time, rather than every time a graphics function is executed.         */

vextern int (*_setdisplay)();		/* Pointer to correct setdisplay fcn  */
vextern int (*_coord0)();		/* Pointer to correct coord0 fcn      */
vextern int (*_sunGraphClear)();	/* Pointer to correct sunGraph... fcn */
vextern int (*_grf_batch)();		/* Pointer to correct grf_batch fcn   */
vextern int (*_sun_window)();		/* Pointer to correct sun_window fcn  */
vextern int (*_change_contrast)();	/* Pointer to correct change_c... fcn */
vextern int (*_change_color)();		/* Pointer to correct change_c... fcn */
vextern int (*_usercoordinate)();	/* Pointer to correct usercoor... fcn */

/* The following items allow switching between different terminal, printer,   */
/* and plotter devices when that switch is made, rather than each time a      */
/* graphics function is called.						      */

struct gdevsw {
   int (*_endgraphics)();
   int (*_graf_clear)();
   int (*_color)();
   int (*_charsize)();
   int (*_amove)();
   int (*_rdraw)();
   int (*_adraw)();
   int (*_dchar)();
   int (*_dstring)();
   int (*_dvchar)();
   int (*_dvstring)();
   int (*_ybars)();
   int (*_normalmode)();
   int (*_xormode)();
   int (*_grayscale_box)();
   int (*_box)();
   int (*_fontsize)();
   int (*_vchar)();
   int (*_vstring)();
   int (*_dimage)();
};

#define C_PLOT 		0	/* Indicates plotting, but not raster */
#define C_RASTER 	1	/* Indicates raster plotting	      */
#define C_TERMINAL	2	/* Indicates output to terminal       */
#define C_PSPLOT	3	/* PostScript Plotting facility       */
#define MGDEV C_PSPLOT+1 


uextern struct gdevsw gdevsw_array[MGDEV];   /* Array of structures of    */
					     /* pointers to graphics      */
					     /* functions.  Set at bootup */
					     /* time.			  */
uextern struct gdevsw *active_gdevsw;	/* Pointer to an entry of the above   */
					/* array.  Set when changing among    */
					/* plot, raster, and terminal output. */

struct ybar { int mn,mx; };

#undef uextern
#undef vextern

#ifndef FILE_IS_GRAPHICS
#ifdef SIS

/* Macros to pointers to functions */
#define setdisplay()		(*_setdisplay)()
#define coord0(X,Y)		(*_coord0)((X),(Y))
#define sunGraphClear()		(*_sunGraphClear)()
#define grf_batch(ON)		(*_grf_batch)((ON))
#define change_contrast(A,B)	(*_change_contrast)((A),(B))
#define change_color(NUM,ON)	(*_change_color)((NUM),(ON))
#define usercoordinate(Y)	(*_usercoordinate)((Y))

/* Macros for pointers to members of structures of functions */
#define endgraphics()		(*(*active_gdevsw)._endgraphics)()
#define graf_clear()		(*(*active_gdevsw)._graf_clear)()
#define color(C)		(*(*active_gdevsw)._color)(abs(C),(C))
#define charsize(F)		(*(*active_gdevsw)._charsize)((F))
#define amove(X,Y)		(*(*active_gdevsw)._amove)((X),(Y))
#define rdraw(X,Y)		(*(*active_gdevsw)._rdraw)((X),(Y))
#define adraw(X,Y)		(*(*active_gdevsw)._adraw)((X),(Y))
#define dchar(CH)		(*(*active_gdevsw)._dchar)((CH))
#define dstring(S)		(*(*active_gdevsw)._dstring)((S))
#define dvchar(CH)		(*(*active_gdevsw)._dvchar)((CH))
#define dvstring(S)		(*(*active_gdevsw)._dvstring)((S))
#define ybars(A,B,C,D,E,F) \
   (*(*active_gdevsw)._ybars)((A),(B),(C),(D),(E),(F))
#define normalmode()		(*(*active_gdevsw)._normalmode)()
#define xormode()		(*(*active_gdevsw)._xormode)()
#define grayscale_box(A,B,C,D,E) \
   (*(*active_gdevsw)._grayscale_box)((A),(B),(C),(D),(E))
#define box(X,Y) (*(*active_gdevsw)._box)((X),(Y))

#define fontsize(F)		(*(*active_gdevsw)._fontsize)((F))
#define vchar(CH)		(*(*active_gdevsw)._vchar)((CH))
#define vstring(S)		(*(*active_gdevsw)._vstring)((S))
#define dimage(X,Y,W,H) (*(*active_gdevsw)._dimage)((X),(Y),(W), (H))

#endif // SIS

extern void init_ParameterLine();
extern int setdisplay();
extern int show_plotterbox();
extern int endgraphics();
extern int graf_clear();
extern int sunGraphClear();
extern int color(int c);
extern int charsize(double f);
extern int fontsize(double f);
extern int amove(int x, int y);
extern int rmove(int x, int y);
extern int rdraw(int x, int y);
extern int adraw(int x, int y);
extern int dchar(char ch);
extern int dstring(char *s);
extern int dvchar(char ch);
extern int dvstring(char *s);
extern int vstring(char *s);
extern int graf_no_op();
extern void clear_ybar(struct ybar *ybar_ptr, int n);
extern void ybars(int dfpnt, int depnt, struct ybar *out, int vertical, int maxv, int minv);
extern void range_ybar(int dfpnt, int depnt, struct ybar *out,
                int maxv, int minv);
extern void expand(short *bufpnt, int pnx, struct ybar *out, int onx, int vo);
extern void expand32(int *bufpnt, int pnx, struct ybar *out, int onx, int vo);
extern void compress(short *bufpnt, int pnx, struct ybar *out0, int onx, int vo);
extern void fid_expand(short *bufpnt, int pnx, struct ybar *out, int onx, int vo);
extern int grayscale_box(int oldx, int oldy, int x, int y, int shade);
extern int box(int x, int y); 
extern int grf_batch(int on);
extern int sun_window();
extern int change_contrast(double a, double b);
extern int change_color(int num, int on);
extern int GisXORmode();
extern int normalmode();
extern int xormode();
extern void ParameterLine(int line, int column, int scolor, char *string);
extern int usercoordinate(int y);
extern int getOptName(int id, char *name);
extern int getOptID(char *name, int *id);
extern double getOptRatio(double a, double b);

#endif // (not) FILE_IS_GRAPHICS

#define BASEOFFSET 		20	/* base line offset in mm */
#define SCALE_Y_OFFSET 		5       /* dscale y offset in mm */


extern double wcmax,wc2max;
extern int mnumxpnts,mnumypnts,ymin,xcharpixels,ycharpixels,ymultiplier;
extern int char_width,char_height,char_ascent,char_descent;
extern int aip_mnumxpnts, aip_mnumypnts, aip_xcharpixels, aip_ycharpixels, aip_ymin;
extern double ppmm;
extern int     plot;	/* 0 = graphics display, 1 = plotting */
extern int     right_edge;  /* size of margin on a graphics terminal */
extern int     left_edge;
extern int     top_edge;
extern int     bottom_edge;
extern char *AxisNumFontName;
extern char *AxisLabelFontName;
extern char *DefaultFontName;

extern int BACK;

#ifndef AIPPOINTS
#define AIPPOINTS

/* these structure were defined in aipStructs.h  */
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

typedef struct D3Dpoint {
    double x, y, z;
} D3Dpoint_t;

#endif

#endif
