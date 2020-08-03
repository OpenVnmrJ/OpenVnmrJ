/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/********************************************************/
/*							*/
/*  ds	-	display a 1D spectrum	 		*/
/*  select -    select a trace				*/
/*							*/
/*	note:  function names which begin with "b_" are	*/
/*	       called when buttons (function keys) are	*/
/*	       used.  function names which begin with	*/
/*	       "m_" are called when the mouse is used	*/
/********************************************************/

#include "vnmrsys.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "data.h"
#include "disp.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "tools.h"
#include "pvars.h"
#include "wjunk.h"
#include "variables.h"
#include "allocate.h"
#include "buttons.h"
#include "init_display.h"
#include "dscale.h"
#include "aipCInterface.h"

extern int is_aip_window_opened();
extern int     start_from_ft;   /* set by ft if ds is to be executed */
extern int     inRepaintMode;
extern float  *get_data_buf(int trace);
extern int   grafIsOn;
extern int  rel_data();
extern int  rel_spec();
extern int  cur_spec();
extern void x_crosshair(int pos, int off1, int off2, double val1, int yval, double val2);
extern void y_crosshair(int pos, int off1, int off2, double val);
extern void finishInsetFrame(int butpressx, int butpressy, int x, int y);
extern int save_data(char *exppath, float *spec1, double mult1, float *spec2,
              double mult2, float *tmp_spec, int shift, int addsub_mode);
extern void popBackingStore();
extern void releasevarlist();
extern void refresh_graf();
extern int selectpl(int argc, char *argv[]);
extern void maxfloat(register float  *datapntr, register int npnt,
                     register float  *max);
extern void integ(register float  *fptr, register float  *tptr, register int npnt);
extern int z_delete(double freq);
extern int z_add(double freq);
extern void set_sp_wp(double *spval, double *wpval, double swval,
             int pts, double ref);
extern int checkphase_datafile();
extern int phasepars();
extern void getCurrentZoomFromList(double *c2, double *d2, double *c1, double *d1);
extern void addNewZoom(char *cmd, double c2, double d2, double c1, double d1);
extern void getNextZoom(double *c2, double *d2, double *c1, double *d1);
extern void addi_dscale();
extern void reset_dscale();

/* argument lists need to be fixed */
extern void set_turnoff_routine();
extern void ds_phase_data();
extern int get_drawVscale();
extern void getSpwpInfo(double *spx, double *wpx, double *spy, double *wpy);
extern void set_line_thickness(const char *thick);

// for mspec (multi spec) display
extern int getShowMspec(); // whether in mspec mode
extern int setSpecColor(int i); // color for selected spec
extern int getSpecYoff(int i); // vertical offset for selected spec 
extern float *calc_mspec(int curSpec, int trace, int fpoint, int dcflag, int normok, int *newspec);
extern void clearMspec();
extern void getVLimit(int *vmin, int *vmax);
extern void drawPlotBox();
extern void setButtonMode(int);
extern int getDscaleDecimal(int);
extern int getAlignvp();

#ifdef CLOCKTIME
extern int ds_timer_no;
#endif  // CLOCKTIME


#define CALIB 		1000.0
#define COMPLETE 	0
#define ERROR 		1
#define FALSE           0
#define TRUE            1
#define CURSOR_MODE	1
#define BOX_MODE	5
#define ADD_MODE        0
#define SUB_MODE        1
#define MIN_MODE        2
#define SPEC3_VS        100.0
#define CLVL		10000.0		/* from integ.c */

#define NOTOVERLAID_NOTALIGNED 0
#define OVERLAID_NOTALIGNED 1
#define NOTOVERLAID_ALIGNED 2
#define OVERLAID_ALIGNED 3
#define STACKED 4
#define UNSTACKED 5

#define ALIGN_1D_Y  4

#define MAXMIN(val,max,min) \
  if (val > max) val = max; \
  else if (val < min) val = min

#ifdef  DEBUG
extern int debug1;
#define DPRINT0(str) \
	if (debug1) fprintf(stderr,str)
#define DPRINT1(str, arg1) \
	if (debug1) fprintf(stderr,str,arg1)
#define DPRINT2(str, arg1, arg2) \
	if (debug1) fprintf(stderr,str,arg1,arg2)
#define DPRINT3(str, arg1, arg2, arg3) \
	if (debug1) fprintf(stderr,str,arg1,arg2,arg3)
#define DPRINT4(str, arg1, arg2, arg3, arg4) \
	if (debug1) fprintf(stderr,str,arg1,arg2,arg3,arg4)
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5) \
	if (debug1) fprintf(stderr,str,arg1,arg2,arg3,arg4,arg5)
#else
#define DPRINT0(str) 
#define DPRINT1(str, arg2) 
#define DPRINT2(str, arg1, arg2) 
#define DPRINT3(str, arg1, arg2, arg3) 
#define DPRINT4(str, arg1, arg2, arg3, arg4) 
#define DPRINT5(str, arg1, arg2, arg3, arg4, arg5)
#endif

#define ALIGN_1D_X  3
#define ALIGN_1D_Y  4
#define ALIGN_1D_XY  6
extern int ycharpixels;  // this is set by graphics.c 
extern void set_vscale(int off, double vscale);

static int s_overlayMode = 0;
static int     oldx_cursor[3],oldy_cursor[2],idelta;
static char    addsubpath[MAXPATHL];
static char    intmod[8];
static int     spec1,spec2,spec3,next,erase1,erase2,erase3;
static int     color1,color2,color3;
static int    *curindex,*curerase,curcolor;
static int     this_is_addi;
static int     this_is_inset;
static double *cur_vp,vp_spec2,vp_spec3;
static double *cur_vs,vs_spec2,vs_spec3;
static double sp_spec1,sp_spec2;
static double  lvltlt_sense;
static double  phasefine;
static float  *cur_scl,scl_spec2;
static float  *phase_data;
static float  *curspec = NULL;
static float  *spectrum;
static float  *spec2ptr;
static float  *spec3ptr;
static float  *integral;
static float   dispcalib;
static float   scale;
static int     updateflag;
static int     intflag;
static int     intresets;
static int     regions;
static int     dscaleRevFlag = 0;
static int     threshflag;
static int     threshY;
static int     resetflag;
static int     phaseflag;
static int     newphaseflag = FALSE;
static int     spwpflag;
static int     lvltltflag;
static int     alt_menu;
static int     do_lp;
static int     addi_mode;
static int     select1D = TRUE;
static int     select2D = FALSE;
static int     old_mnumxpnts, old_mnumypnts;
static int     interfero;
static int     revAxis;
static int     thMin;
static int     doClear = 1;
static int     dispCursor = 1;
// for baseline display
static int baselineBufID;
extern float *getBcdata(int, int, int);
extern void clearBcdata();

static FILE   *mark_textfptr = NULL;
static char    mark_textfname[ MAXPATHL ];
static int mspec[MAXSPEC];
static int merase[MAXSPEC];
static double oldsp, oldwp, oldvs;
static int graph_flag = FALSE; // TRUE is lines or box is drawn over ds

extern void do_dpf_dpir(); 
extern void get_dpf_dpir(int *dpf_flag, int *dpir_flag); 
extern void redo_dpf_dpir(); 
extern void set_dpf_flag(int b, char *cmd);
extern void set_dpir_flag(int b, char *cmd);

/*  The next four variables were original declared static local to
    m_newphase, as fp, np, dfp and dnp.  To facilitate the introduction
    of new features, which require access to these variables outside of
    m_newphase, they were move from m_newphase scope to file scope and
    renamed phase_fp ... phase_dnp.  Notice that m_lvltlt has its own
    dnp static ...

    A last_ph variable in m_newphase was not modified or moved.

    June 3, 1998							*/
    
static int     phase_fp;
static int     phase_np;
static int     phase_dfp;
static int     phase_dnp;
static int     old_ds_mode = -1;
static int     isXorOn = 0;

int	       ds_mode;

/* Function prototypes */

static void b_cursor();
static void ds_m_newcursor(int, int, int, int);
static void m_newcursor(int, int, int, int);
static void cr_line();
static void b_integral();
static void ds_multiply(int, int);
static void turnoff_ds();
static void specdisp();
static void intdisp();
static void show_addsub();
static void combinespec(float *, float *, float *);
static void set_spec(float *, double *, double *, float *, double, int *, int *);
static void addi_disp(int, int, int);
static void addi_buttons();
static void b_scwc();
extern double ppm2Hz(int, double);
extern double Hz2ppm(int direction, double val);
extern double convert4ppm(int direction, double val);
extern int x_cursor(int *old_pos, int new_pos);
extern int y_cursor(int *old_pos, int new_pos);
void ds_sendSpecInfo(int frame);
int dsChanged();

#ifdef VNMRJ
void ds_sendCursor(double c1, double d1, int mode);
void ds_sendCrosshair(double c1, double f1);
void ds_noCrosshair();
extern void m_noCrosshair();
extern int getButtonMode();
extern int isJprintMode();
static int mapAxis(int, double *, double *, double *, double *, char *, char *, char *);
void getAxis(char *, int);
extern void getFrameG(int id, int *x, int *y, int *w, int *h);
extern int VnmrJViewId;
extern int vj_x_cursor(int i, int *old_pos, int new_pos, int c);
extern int vj_y_cursor(int i, int *old_pos, int new_pos, int c);
void ds_sendSpwp(double, double, int);
void ds_nextZoomin(double *c2, double *c1, double *d2, double *d1);
void ds_prevZoomin(double *c2, double *c1, double *d2, double *d1);
int showCursor();
extern int getFrameID();
void ds_checkSpwp(double *s2, double *w2, int id);
extern int getOverlayMode();
extern int getActiveWin();
extern int getChartMode();
extern void getSweepInfo(double *, double *, double *, double *, char *,char *);
extern void appendJvarlist(const char *name);
extern int bph();
void setBph0(int trace, float value);
void setBph1(int trace, float value);
int isInset();
extern void vj_xoron();
extern void vj_xoroff();
extern int showPlotBox();
extern void get_nuc_name(int direction, char *nucname, int n);

#endif

/*  Special notes concerning select_init:  It is called in many places
    where there is a possibility the display parameters are now set for
    the plotter instead of the graphics screen.  We call select_init
    because calling init2d itself could possibly require recalculating
    the spectrum.  One must DO_CHECK2D if one does DO_SPECPARS, since
    the latter operation requires the various dimensions to be set up
    and specified correctly when examining traces from a 2D data set.	*/

int getVpSpecColor() {
   double d;
   if(P_getreal(GLOBAL,"colorMode", &d, 1)) d=0.0;
   if(d == 0.0) return SPEC_COLOR;
   else if(VnmrJViewId == 1) return VP1_COLOR; 
   else if(VnmrJViewId == 2) return VP2_COLOR; 
   else if(VnmrJViewId == 3) return VP3_COLOR; 
   else if(VnmrJViewId == 4) return VP4_COLOR; 
   else if(VnmrJViewId == 5) return VP5_COLOR; 
   else if(VnmrJViewId == 6) return VP6_COLOR; 
   else if(VnmrJViewId == 7) return VP7_COLOR; 
   else if(VnmrJViewId == 8) return VP8_COLOR; 
   else if(VnmrJViewId == 9) return VP9_COLOR; 
   else return SPEC_COLOR;
}

/******************/
static char *dsstatus(char *stat)
/******************/
{
    static char status[16] = "";

    if (stat) {
	strncpy(status, stat, sizeof(status));
	status[sizeof(status)-1] = '\0';
    }
    return status;
}

/*************/
int getdsstat(int argc, char *argv[], int retc, char *retv[])
/*************/
{
    (void) argc;
    (void) argv;
    if (retc > 0) {
	retv[0] = newString(dsstatus(0));
    }
    RETURN;
}


/************/
int currentindex()
/************/
/* return the index of the currently active spectrum       */
{
  if (specIndex == 0)
    return(1);
  else
    return(specIndex);
}

int get_ds_cursor_flag()
{
    return dispCursor;
}

void set_ds_cursor_flag(int n)
{
    dispCursor = n;
}


int get_ds_threshold_flag()
{
    return threshflag;
}

int get_ds_threshold_loc()
{
    return threshY;
}

void set_ds_threshold_flag(int n)
{
    threshflag = n;
    if (n <= 0)
       threshY = 0;
}

void set_ds_threshold_loc(int n)
{
    threshY = n;
}

/******************/
static void ds_reset()
/******************/
{
  if (get_dis_setup() != GRAPHICS)
     select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE);
#ifdef VNMRJ
  vj_x_cursor(0, &oldx_cursor[0],0, CURSOR_COLOR);
  vj_x_cursor(1, &oldx_cursor[1],0, CURSOR_COLOR);
  vj_y_cursor(0, &oldy_cursor[0],0, CURSOR_COLOR);
  // color(THRESH_COLOR);
  vj_x_cursor(2, &oldx_cursor[2],0, THRESH_COLOR);
  vj_y_cursor(1, &oldy_cursor[1],0, THRESH_COLOR);
  oldx_cursor[0] = 0;
  oldx_cursor[1] = 0;
  oldx_cursor[2] = 0;
  oldy_cursor[0] = 0;
  oldy_cursor[1] = 0;
#else
  Wgmode();
  xormode();
  color(CURSOR_COLOR);
  x_cursor(&oldx_cursor[0],0);
  x_cursor(&oldx_cursor[1],0);
  y_cursor(&oldy_cursor[0],0);
  color(THRESH_COLOR);
  x_cursor(&oldx_cursor[2],0);
  y_cursor(&oldy_cursor[1],0);
  normalmode();
#endif
  if (intflag && regions && resetflag)
  {
    resetflag = FALSE;
    intdisp();
  }
  resetflag = FALSE;
  spwpflag = FALSE;
  dsstatus("");
}

/*************************************/
static void update_xcursor(int cursor_number, int x)
/*************************************/
{
  int c;

#ifdef VNMRJ
  if(cursor_number != 2 && !showCursor()) return;
  if (cursor_number == 2) {
     char thickName[64];
     getOptName(THRESHOLD_LINE,thickName);
     set_line_thickness(thickName);
     c = THRESH_COLOR;
  } else
     c = CURSOR_COLOR;
  vj_x_cursor(cursor_number, &oldx_cursor[cursor_number],x, c);
  oldx_cursor[cursor_number] = x;
#else
  Wgmode();
  xormode();
  color((cursor_number == 2) ? THRESH_COLOR : CURSOR_COLOR);
  x_cursor(&oldx_cursor[cursor_number],x);
  normalmode();
#endif
}

/*************************************/
static void update_ycursor(int cursor_number, int y)
/*************************************/
{ 
  int c;

#ifdef VNMRJ
  if(cursor_number != 1 && !showCursor()) return;
  if (cursor_number == 1) {
     char thickName[64];
     getOptName(THRESHOLD_LINE,thickName);
     set_line_thickness(thickName);
     c = THRESH_COLOR;
  } else {
     c = CURSOR_COLOR;
  }
  vj_y_cursor(cursor_number, &oldy_cursor[cursor_number],y, c);
  oldy_cursor[cursor_number] = y;
#else
  Wgmode();
  xormode();
  /* cursor_number == 1 is used for the threshhold line */
  color((cursor_number == 1) ? THRESH_COLOR : CURSOR_COLOR);
  y_cursor(&oldy_cursor[cursor_number],y);
  normalmode();
#endif
}

/*************************************/
static void ds_m_newcursor(int butnum, int x, int y, int moveflag)
/*************************************/
{
  if ((butnum == 3) && (ds_mode != BOX_MODE) && (!this_is_addi))  {
    Wturnoff_buttons();
    set_turnoff_routine(turnoff_ds);
    ds_mode = BOX_MODE;
    b_cursor();
    m_newcursor(butnum,x,y,moveflag);
    if (P_setstring(GLOBAL,"crmode","b",0))
      Werrprintf("Unable to set variable \"crmode\".");
    execString("menu\n");
    set_turnoff_routine(turnoff_ds);
    }
  else {
    m_newcursor(butnum,x,y,moveflag);
    }

#ifdef VNMRJ
  ds_sendCursor(cr, delta, ds_mode);
#endif
}

/*************************************/
static void m_newcursor(int butnum, int x, int y, int moveflag)
/*************************************/
{
  (void) moveflag;
  int dez = getDscaleDecimal(0)+2;
  if (get_dis_setup() != 1) 
  {
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  }
  Wgmode();
  if ((butnum==3) && (ds_mode!=BOX_MODE))	/*  box button */
  {
    if (ds_mode!=CURSOR_MODE)
    {
      ds_reset();
      ds_mode = -1;
      b_cursor();
    }
    ds_mode = BOX_MODE;
    InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,NOUNIT,
                         BLANK_NAME,-PARAM_COLOR,SCALED,dez);
    dsstatus("BOX");
    if (this_is_addi)
      addi_buttons();
  }

  if (butnum==2)
    ds_multiply(x,y);
  else
  {
    set_cursors((ds_mode == BOX_MODE),(butnum == 1),&x,
                 oldx_cursor[0],oldx_cursor[1],&idelta,dfpnt,dnpnt);
    if (ds_mode==BOX_MODE)
    {
      if (butnum==1)	/* cursor button */
        update_xcursor(1,x+idelta);
      else
      {
        delta  = idelta  * wp  / dnpnt ;
        update_xcursor(1,oldx_cursor[0]+idelta);
        UpdateVal(HORIZ,DELTA_NAME,delta,SHOW);
      }
    }
    if (butnum==1)	/* cursor button */ 
    {
      update_xcursor(0,x);
      if (interfero || dscaleRevFlag)
         cr = (x - dfpnt) * wp  / dnpnt  + sp;
      else if (revAxis)
         cr = (x - dfpnt - dnpnt) * wp  / dnpnt  + sp;
      else
         cr = - (x - dfpnt ) * wp  / dnpnt  + wp  + sp;
      UpdateVal(HORIZ,CR_NAME,cr,SHOW);
    }
  }
  if (ds_mode == BOX_MODE)  {
    if (P_setstring(GLOBAL,"crmode","b",0))
      Werrprintf("Unable to set variable \"crmode\".");
    }
  else  {
    if (P_setstring(GLOBAL,"crmode","c",0))
      Werrprintf("Unable to set variable \"crmode\".");
    }
}

/*******************/
static void b_cursor()
/*******************/
{ int x,x1,save;
  double crsav,deltasav;

  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  ds_mode = (ds_mode == CURSOR_MODE) ? BOX_MODE : CURSOR_MODE;
  Wturnoff_mouse();
  if (this_is_addi)
    addi_buttons();
  if (interfero || dscaleRevFlag)
     x  = dfpnt  + dnpnt  * (cr  - sp) / wp;
  else if (revAxis)
     x  = dfpnt  + dnpnt -  dnpnt  * (sp  - cr) / wp;
  else
     x  = dfpnt  + dnpnt  * (wp  - cr  + sp ) / wp;
  x1 = x + dnpnt  * delta  / wp;
  crsav = cr;
  deltasav = delta;
  idelta = save  = x1 - x;
  m_newcursor(1,x,0,0);
#ifdef VNMRJ
  if (isJprintMode())
      ds_mode = old_ds_mode;
#endif
  if (ds_mode==BOX_MODE)
  {
    idelta  = save;
    m_newcursor(3,x1,0,0);
    dsstatus("BOX");
  }
  else
  { update_xcursor(1,0);  /* erase the second cursor */
    dsstatus("CURSOR");
  }
  if ((crsav >= sp) && (crsav <= sp + wp))
  {
    cr = crsav;
    UpdateVal(HORIZ,CR_NAME,cr,NOSHOW);
  }
  if ((cr - deltasav >= sp) && (deltasav > 0.0))
  {
    delta = deltasav;
    UpdateVal(HORIZ,DELTA_NAME,delta,NOSHOW);
  }
  cr_line();
  dispCursor = 1;

  if(threshflag) update_ycursor(1,threshY);
  else update_ycursor(1,0);

  activate_mouse(ds_m_newcursor,ds_reset);
  if ( ! this_is_addi)
     aspFrame("ds",0,0,0,0,0);
}

/**************/
static void cr_line()
/**************/
{
  int dez = getDscaleDecimal(0)+2;
  DispField1(FIELD1, PARAM_COLOR,(intflag) ? "io" : "vp");
  DispField2(FIELD1,-PARAM_COLOR,(intflag) ? io : *cur_vp, 1);
  InitVal(FIELD3,HORIZ,CR_NAME, PARAM_COLOR,NOUNIT,
                       CR_NAME,-PARAM_COLOR,SCALED,dez);
  DispField1(FIELD2, PARAM_COLOR,(intflag) ? "is" : "vs");
  DispField2(FIELD2,-PARAM_COLOR,(intflag) ? is : *cur_vs, 1);

  if (ds_mode==BOX_MODE)
    InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,NOUNIT,
                         DELTA_NAME,-PARAM_COLOR,SCALED,dez);
  else
    InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,NOUNIT,
                         BLANK_NAME, PARAM_COLOR,SCALED,dez);
}

/**************/
static void ph_line()
/**************/
{
  if (isXorOn)
      vj_xoroff();
  DispField1(FIELD1, PARAM_COLOR, "vp");
  DispField2(FIELD1,-PARAM_COLOR, *cur_vp, 1);
  InitVal(FIELD3,HORIZ,RP_NAME, PARAM_COLOR,NOUNIT,
                       RP_NAME,-PARAM_COLOR,NOT_SCALED,1);
  DispField1(FIELD2, PARAM_COLOR, "vs");
  DispField2(FIELD2,-PARAM_COLOR, *cur_vs, 1);
  InitVal(FIELD4,HORIZ,LP_NAME, PARAM_COLOR,NOUNIT,
                       LP_NAME,-PARAM_COLOR,NOT_SCALED,1);
  if (isXorOn)
      vj_xoron();
}

/*****************/
static void newspec(int inset, int fp, int np, int dfp, int dnp)
/*****************/
{ int res,res2;

  if (rel_data())
  {
     Wturnoff_buttons();
     set_turnoff_routine(turnoff_ds);
  }
  else if ((curspec = spectrum = calc_spec(specIndex-1,0,FALSE,TRUE,&updateflag))==0)
  {
     Wturnoff_buttons();
     set_turnoff_routine(turnoff_ds);
  }
  else if ((phase_data = get_data_buf(specIndex-1))==0)
  {
     Wturnoff_buttons();
     set_turnoff_routine(turnoff_ds);
  }
  else
  {
     int vmin = 1;
     int vmax = mnumypnts - 3;
     getVLimit(&vmin, &vmax);

     erase1 = 0;
     erase2 = 0;
     res = oldx_cursor[0];
     res2 = oldx_cursor[1];
     oldx_cursor[0] = oldx_cursor[1] = oldx_cursor[2] = oldy_cursor[0] = oldy_cursor[1] = 0;
     calc_ybars(spectrum+fpnt,1,*cur_vs * *cur_scl,dfpnt,dnpnt,npnt,
             dfpnt2 + (int)(dispcalib * *cur_vp),next);
     Wclear_graphics();
     show_plotterbox();
     drawPlotBox();
     ResetLabels();
     displayspec(dfpnt,dnpnt,0,&next,&spec1,&erase1,vmax,vmin,SPEC_COLOR);
     update_xcursor(0,res);
     update_xcursor(1,res2);
     update_ycursor(1,threshY);
     if (inset)
     {
       calc_ybars(spectrum+fp,1,*cur_vs * *cur_scl,dfp,dnp,np,
          dfpnt2 + (int)(dispcalib * *cur_vp),next);
       erase1 = 0;
       displayspec(dfp,dnp,0,&spec1,&spec1,&erase1,0,0,SPEC_COLOR);
       displayspec(dfp,dnp,0,&next,&spec2,&erase2,vmax,vmin,SPEC_COLOR);
     }
     if (dscale_on())
       new_dscale(FALSE,TRUE);
     ph_line();
  }
}

/*****************/
static int exit_phase()
/*****************/
{
  updateflag = TRUE;
  if (rel_data())
  {
     Wturnoff_buttons();
     set_turnoff_routine(turnoff_ds);
  }
  else if (phaseflag)
  {
    aspFrame("resetVscale",0,0,0,0,0);
    if ((curspec = spectrum = calc_spec(specIndex-1,0,FALSE,TRUE,&updateflag))==0)
      return(ERROR);
    Wclear_graphics();
    show_plotterbox();
    ResetLabels();
    oldx_cursor[0] = oldx_cursor[1] = oldx_cursor[2] = oldy_cursor[0] = oldy_cursor[1] = 0;
    erase1 = 0;
    erase2 = 0;
    curindex = &spec1;
    curerase = &erase1;
    curcolor = getVpSpecColor();
    specdisp();
    if (dscale_on())
      new_dscale(FALSE,TRUE);
    if (ds_mode < 0)
      b_cursor();
    phaseflag = FALSE;
  }
  return(COMPLETE);
}

/*****************/
static void phase_disp(int xpos, int *fp, int *np, int *dfp, int *dnp)
/*****************/
{
  *np = npnt * phaseflag / 100;
  if (*np<4) *np = 4;
  *fp = datapoint(sp + wp + rflrfp - ((xpos - dfpnt) * wp / dnpnt),sw,fn/2);
  *fp -= *np / 2;
  if (*fp<fpnt) *fp = fpnt;
  if (*fp + *np >= fpnt + npnt) *fp = fpnt + npnt - *np;
  *dnp = dnpnt * phaseflag / 100;
  if (*dnp > dnpnt) *dnp = dnpnt;
  *dfp = dfpnt + (int)((double)dnpnt * (double)(*fp - fpnt) / (double)npnt);
  if (*dfp < dfpnt) *dfp = dfpnt;
  if (*dfp + *dnp >= dfpnt + dnpnt) *dfp = dfpnt + dnpnt - *dnp;
  DPRINT3("\n npnt=%d phaseflag=%d np=%d\n",npnt,phaseflag,*np);
  DPRINT5(" x position=%d fp=%d np=%d dfp=%d dnp=%d\n",
                   xpos,*fp,*np,*dfp,*dnp);
}

/*****************/
static int phasit(int fp, int np, int dfp, int dnp, double delt)
/*****************/
{
  float		*phase_spec;
  double	factor;
  int		i;
  float		*ptr, start, incr;
  int vmin = 1;
  int vmax = mnumypnts - 3;
  getVLimit(&vmin, &vmax);

  if ((phase_spec = (float *) allocateWithId(sizeof(float) * (np + 1),"ds"))==0)
  {
    Werrprintf("cannot allocate phasing buffer");
    Wturnoff_buttons();
    return(ERROR);
  }

  factor = (double) (np-1) / (double) ((fn/2) - 1);
  DPRINT5("\nlp=%g rp=%g delt=%g lp1=%g rp1=%g\n",lp,rp,delt,
        lp * factor,rp + lp * (1.0 - factor) - delt);
  ds_phase_data(specIndex - 1, phase_data, phase_spec, fp, np,
                lp * factor, rp + lp * (1.0 - factor) - delt);
/* do dc level correction for phasing inset spectrum */
  ptr=phase_spec;
  incr = (tlt/CLVL)/(float)(fn/2);
  start = lvl/CLVL + incr*(fp-1);
  for (i=0;i<np;i++)
  {
    *ptr++ +=start;
    start += incr;
  }
/* end dc level correction */
  drawPlotBox();
  calc_ybars(phase_spec,1,*cur_vs * *cur_scl,dfp,dnp,np,
         dfpnt2 + (int)(dispcalib * *cur_vp),next);
  release(phase_spec);
  displayspec(dfp,dnp,0,&next,&spec2,&erase2,vmax,vmin,SPEC_COLOR);
  if (do_lp) {
      dsstatus("1st PH");
  } else {
      dsstatus("0th PH");
  }
  return(COMPLETE);
}

void phase_cursors(int x)
{ int dum,x2;

  if (x - dnpnt/10 < dfpnt)
  {
    x = dfpnt;
    x2 = dfpnt + (dnpnt / 5);
  }
  else if (x + dnpnt/10 > dfpnt + dnpnt)
  {
    x = dfpnt + dnpnt - dnpnt/10;
    x2 = dfpnt + dnpnt;
  }
  else
  {
    x -= dnpnt/10;
    x2 = x + dnpnt/5;
  }
  set_cursors(0,1,&x,oldx_cursor[0],oldx_cursor[1],&dum,dfpnt,dnpnt);
  update_xcursor(0,x);
  set_cursors(0,1,&x2,oldx_cursor[1],oldx_cursor[0],&dum,dfpnt,dnpnt);
  update_xcursor(1,x2);
}

/************************************/
static int m_newphase(int butnum, int x, int y, int moveflag)
/************************************/
{ double lp_change;
  int    dum;
  static int    last_ph;
  static double phase_cr;
  double delt,fine;
  extern float vs_mult();

  if (butnum == 5)
  {
    if (phase_dnp)
    {
      y = oldy_cursor[0];
      newspec(TRUE,phase_fp,phase_np,phase_dfp,phase_dnp);
      update_ycursor(0,y);
    }
    else
      newspec(FALSE,phase_fp,phase_np,phase_dfp,phase_dnp);
    return(COMPLETE);
  }
  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  if (butnum == 4)
  {
    phase_dnp = phase_dfp = 0;
    update_xcursor(0,oldx_cursor[0]);
    update_xcursor(1,oldx_cursor[1]);
    ph_line();
    return(COMPLETE);
  }
  if ((x<dfpnt)||(x>=dfpnt+dnpnt)) return 0;
  Wgmode();
  if (butnum == 2)
  {
    dsstatus("PHASE");
    if (x<dfpnt + (int)(0.05 * (double)dnpnt))
    {
      *cur_vp = (double) (y - dfpnt2) / dispcalib;
      P_setreal(CURRENT,"vp",*cur_vp,0);
    }
    else
    {
      *cur_vs *= vs_mult(x,y,mnumypnts,dfpnt2 + (int)(dispcalib * *cur_vp),
                  dfpnt,dnpnt,spec1);
      MAXMIN(*cur_vs,1.0e9,1.0e-6);
      P_setreal(CURRENT,"vs",*cur_vs,0);
    }
    if (phase_dnp)
    {
      y = oldy_cursor[0];
      newspec(TRUE,phase_fp,phase_np,phase_dfp,phase_dnp);
      update_ycursor(0,y);
    }
    else
      newspec(FALSE,phase_fp,phase_np,phase_dfp,phase_dnp);
  }
  else
  {
/*
    y = mnumypnts - y;
*/
    if (!moveflag)
    {
      int newyval = 0;

      if (newphaseflag)
      {
         int mask = mouseMask();
         if ((mask & ctrl) || (mask & shift))
         {
            dsstatus("1st PH");
            if (mask & ctrl)
            {
               phase_disp(x,&phase_fp,&phase_np,&phase_dfp,&phase_dnp);
               phase_cr = sp + wp - (double) (x - dfpnt) * wp / (double) dnpnt;
               update_xcursor(0,x);
            }
            else if (phase_dnp == 0)
            {
               phase_disp(x,&phase_fp,&phase_np,&phase_dfp,&phase_dnp);
               phase_cr = sp + wp - (double) (x - dfpnt) * wp / (double) dnpnt;
               update_xcursor(0,x);
            }
            do_lp = x;
            newyval = 1;
         }
         else
         {
             dsstatus("0th PH");
             update_xcursor(0,x);
             do_lp = 0;
             phase_cr = sp + wp - (double) (x - dfpnt) * wp / (double) dnpnt;
             newyval = 1;
         }
      }
      else if (phase_dnp == 0)  /* this is true on the first mouse button only */
      {
        dsstatus("0th PH");
        phase_cursors(x);
        x = (oldx_cursor[0] + oldx_cursor[1]) / 2;
        phase_cr = sp + wp - (double) (x - dfpnt) * wp / (double) dnpnt;
        newyval = 1;
      }
      else if ((x < oldx_cursor[0]) || (x > oldx_cursor[1]))
      {
        dsstatus("1st PH");
        do_lp = phase_dfp + phase_dnp / 2;
        do_lp = (oldx_cursor[0] + oldx_cursor[1]) / 2;
        phase_cursors(x);
        phase_cr = sp + wp -
                   (double) (do_lp - dfpnt) * wp / (double) dnpnt;
        newyval = 1;
      }
      if (newyval)
      {
        if (do_lp != x)
           phase_disp(x,&phase_fp,&phase_np,&phase_dfp,&phase_dnp);
        set_cursors(0,1,&y,
                 oldy_cursor[0],oldy_cursor[1],&dum,dfpnt2,mnumypnts - dfpnt2);
        newspec(TRUE,phase_fp,phase_np,phase_dfp,phase_dnp);
        update_ycursor(0,y);
        last_ph = y;
      }
    }
    if (y != last_ph)
    {
      int updateLP = 0;
      fine = (butnum == 1) ? 1.0 : phasefine;
      if (do_lp)
      {
        delt = sw / 10.0;
        if (delt > 3600.0)
          delt = 3600.0;
        else if (delt < 360.0)
          delt = 360.0;
        lp_change = (delt / (double) mnumypnts) * (double) (y-last_ph) / fine;
        lp_change *= (double) (fn / 2) / (double) (fn / 2 - (phase_fp + phase_np / 2));
        lp += lp_change;
        rp -= lp_change * (phase_cr + rflrfp) / sw;
        if(bph()>0) {
          Winfoprintf("manual phase %d %f",c_buffer,lp);
          setBph1(c_buffer,lp);
	} else {
          updateLP = 1;
        }
      }
      else
      {
        rp += (180.0 / mnumypnts) * (y - last_ph) / fine;
      }

      delt = lp * (double) phase_fp / (double) ( fn/2 - 1 );
      phasit(phase_fp,phase_np,phase_dfp,phase_dnp,delt);
      if(bph()>0) {
        Winfoprintf("manual phase rp %d %f",c_buffer,rp);
        setBph0(c_buffer,rp);
      } else {
         if (isXorOn)
           vj_xoroff();
         UpdateVal(HORIZ,RP_NAME,rp,SHOW);
         if (updateLP)
            UpdateVal(HORIZ,LP_NAME,lp,SHOW);
         if (isXorOn)
            vj_xoron();
      }
      last_ph = y;
    }
  }
  grf_batch(0);
  if ( ! this_is_addi)
     aspFrame("ds",0,0,0,0,0);
  return(COMPLETE);
}

/*---------------------------------------
|					|
|	       b_phase()/0		|
|					|
|   This function is activated by se-	|
|   lecting the PHASE menu button.	|
|					|
+--------------------------------------*/
static void b_phase()
{
   double		value;

#ifdef VNMRJ
  vj_xoron();
  isXorOn = 1;
#endif

  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );

/*******************************************
*  Check to see if the display parameters  *
*  are set for a phased display.           *
*******************************************/

   if (phasepars())
   {
      Werrprintf("Cannot phase this data.");
      ds_mode = -1;
      b_cursor();
      return;
   }
   phaseflag = TRUE;
   Wturnoff_mouse();
   Wgmode();
   if (intflag)
      b_integral();
   ds_reset();
   dsstatus("PHASE");
   do_lp = FALSE;

/**********************************************
*  Check to see if the data are phaseable.    *
*  For 2D data, check to see if the data are  *
*  hypercomplex.  If they are, select only    *
*  the {ReRe, ReIm} part or the {ReRe, ImRe}  *
*  part of the complex set.                   *
**********************************************/

   if (checkphase_datafile())
   {
      Werrprintf("Cannot phase this data");
      dsstatus("");
      return;
   }

/*********************************************
*  Select size of phasing region.  Activate  *
*  interactive phasing.                      *
*********************************************/

   ds_mode = -1;
/*
   if (P_getreal(GLOBAL, "phasing", &value, 1))
   {
      phaseflag = 20;
   }
   else
   {
      phaseflag = (int) (value + 0.5);
      if (phaseflag < 10)
      {
         phaseflag = 10;
      }
      else if (phaseflag > 100)
      {
         phaseflag = 100;
      }
   }
 */
   phaseflag = 100;
   if (P_getreal(GLOBAL, "phasef", &value, 1))
   {
      phasefine = 8.0;
   }
   else
   {
      phasefine = value;
      if (phasefine < 0.01)
      {
         phasefine = 0.01;
      }
      else if (phasefine > 100)
      {
         phasefine = 100;
      }
   }

   if (this_is_addi)
     addi_buttons();
   m_newphase(4, dfpnt + dnpnt / 2, dfpnt2 + mnumypnts / 2, 0);
   update_ycursor(1, threshY);
   activate_mouse(m_newphase, exit_phase);
   if ( ! this_is_addi)
   {
      aspFrame("ds",0,0,0,0,0);
      aspFrame("resetVscale",0,0,0,0,0);
   }
}


/************************************/
static void m_thcursor(int butnum, int x, int y, int moveflag)
/************************************/
{ int dum;

  (void) moveflag;
  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  if (butnum==2)
    ds_multiply(x,y);
  else if (butnum==1)
  {
    int ymin;
/*
    y = mnumypnts - y;
*/
    ymin = dfpnt2 + (int) (dispcalib * *cur_vp);
    if (ymin < 1)
       ymin = 1;
    set_cursors(0,(butnum == 1),&y,
                 oldy_cursor[1],oldy_cursor[0],&dum,1,mnumypnts);
    if (y < ymin + thMin)
      y = ymin + thMin;
    if (y < 1)
       y = 1;
    threshY = y;
    update_ycursor(1,y);
    th = (y - ymin) / dispcalib;
    P_setreal(CURRENT,"th",th,0);
    DispField2(FIELD3,-PARAM_COLOR, th, 1);
    if(!isInset()) redo_dpf_dpir();
  }
}

/**********************************************/
static void set_wp(double *sp1, double *sp2,
                   double spold, double wpold, double sp_delta)
/**********************************************/
{
  double factor;

  factor = sp_delta / (wp - wpold);
  if (*sp2 + wp + rflrfp + sp_delta > sw)
  {
    wp = (sw - *sp2 - wpold - rflrfp) * (1.0 + factor) + wpold;
    *sp1 = sp = spold + (wp - wpold) * factor;
    *sp2 += (wp - wpold) * factor;
  }
  else if (*sp2 + rflrfp + sp_delta < 0)
  {
    wp = (-*sp2 - rflrfp) / factor + wpold;
    *sp1 = sp = spold + (wp - wpold) * factor;
    *sp2 += (wp - wpold) * factor;
  }
  else
    *sp2 += (wp - wpold) * factor;
}

/****************************/
static void check_spwp(double spold, double wpold)
/****************************/
{
  double sp_delta;

  if (*curindex == spec1)
    sp_spec1 = sp;
  else
    sp_spec2 = sp;
  sp_delta = sp - spold;
  if (wp != wpold)
    if (*curindex == spec1)
      set_wp(&sp_spec1,&sp_spec2,spold,wpold,sp_delta);
    else
      set_wp(&sp_spec2,&sp_spec1,spold,wpold,sp_delta);
  else if ((sp != spold) && (*curindex == spec3))
  {
    if (sp_spec1 + wp + rflrfp + sp_delta > sw)
    {
      sp_delta = sw - wp - rflrfp - sp_spec1;
      sp_spec2 = sp = spold + sp_delta;
      sp_spec1 += sp_delta;
    }
    else if (sp_spec1 + rflrfp + sp_delta < 0)
    {
      sp_spec2 = sp -= sp_spec1 + rflrfp + sp_delta;
      sp_spec1 = -rflrfp;
    }
    else
    {
      sp_spec1 += sp_delta;
    }
  }
}

/************************************/
static void m_spwp(int butnum, int x, int y, int mask)
/************************************/
{
	double freq, spsav, wpsav;
	int dum;
	static int spon, wpon;
	static double frqset;
	static double vpsave=0;

        if ( ! this_is_addi)
	   P_getreal(CURRENT,"vp",cur_vp,1);

	if (get_dis_setup() != 1)
		select_init(0, GRAPHICS, NO_FREQ_DIM, NO_HEADERS, DO_CHECK2D,
				DO_SPECPARS, NO_BLOCKPARS, NO_PHASEFILE);
	if (butnum == 4)
		spon = wpon = FALSE;
	else if ((x < dfpnt) || (x >= dfpnt + dnpnt))
		return;
	else if (butnum == 2)
		ds_multiply(x, y);
	else {
		spsav = sp;
		wpsav = wp;
		if (butnum == 3) {
			if (!wpon) {
				set_cursors(0, 1, &x, oldx_cursor[0], oldx_cursor[1], &dum,
						dfpnt, dnpnt);
				frqset = (double) (x - dfpnt) / (double) dnpnt;
				update_xcursor(0, x);
				set_cursors(0, 1, &y, oldy_cursor[0], oldy_cursor[1], &dum,
						dfpnt2, mnumypnts - dfpnt2);
				update_ycursor(0, y);
				wpon = y;
			} else {
				wp += (wp / mnumypnts) * (y - wpon);
				if (wp > sw) {
					wp = sw;
					sp = -rflrfp;
				} else {
					if (wp < 20.0 * sw / fn)
						wp = 20.0 * sw / fn;
					sp -= (wp - wpsav) * (1.0 - frqset);
				}
			}
			UpdateVal(HORIZ, WP_NAME, wp, SHOW);
			spon = FALSE;
		} else {
			if (spon) {
				if (interfero)
					freq = sp + (x - dfpnt) * wp / dnpnt;
				else if (revAxis)
					freq = sp + (x - dfpnt - dnpnt) * wp / dnpnt;
				else
					freq = sp + wp - (x - dfpnt) * wp / dnpnt;
				sp -= freq - frqset;
			} else
				update_ycursor(0, 0);
			set_cursors(0, 1, &x, oldx_cursor[0], oldx_cursor[1], &dum, dfpnt,
					dnpnt);
			if (interfero)
				frqset = sp + (x - dfpnt) * wp / dnpnt;
			else if (revAxis)
				frqset = sp + (x - dfpnt - dnpnt) * wp / dnpnt;
			else
				frqset = sp + wp - (x - dfpnt) * wp / dnpnt;
			if (!spon)
				update_xcursor(0, x);
			spon = TRUE;
			wpon = FALSE;
		}
                if (revAxis)
                   set_sp_wp_rev(&sp,&wp,sw,fn/2,rflrfp);
                else
		   set_sp_wp(&sp, &wp, sw, fn / 2, rflrfp);
		if ((wpon) && (wp == wpsav))
			sp = spsav;
		if (this_is_addi)
			check_spwp(spsav, wpsav);
		if ((sp != spsav) || (wp != wpsav) || (*cur_vp!=vpsave)) {
#ifdef VNMRJ
			ds_checkSpwp(&sp, &wp, VnmrJViewId);
#endif
			UpdateVal(HORIZ, SP_NAME, sp, NOSHOW);
			UpdateVal(HORIZ, WP_NAME, wp, NOSHOW);
			P_setreal(CURRENT, "hzmm", wp / wc, 0);
			if (spon) {
				if (interfero)
					update_xcursor(0, dfpnt + (int) ((double) dnpnt * (frqset
							- sp) / wp));
			        else if (revAxis)
					update_xcursor(0, dfpnt + (int) ((double) dnpnt * (sp
							- frqset) / wp));
				else
					update_xcursor(0, dfpnt + (int) ((double) dnpnt * (sp + wp
							- frqset) / wp));
			}
			exp_factors(TRUE);
			if ((this_is_addi) && (*curindex == spec3))
				show_addsub();
			else
				specdisp();

			if (intflag)
				intdisp();
			if (dscale_on())
				new_dscale(TRUE, TRUE);
			UpdateVal(HORIZ, SP_NAME, sp, SHOW);
			if (this_is_addi) {
				if (*curindex != spec3) {
					if (wp != wpsav)
						addi_disp((*curindex != spec1), (*curindex != spec2),
								FALSE);
					show_addsub();
				} else
					addi_disp(TRUE, TRUE, FALSE);
			}
			vpsave=*cur_vp;
		}
	}
        if ( ! this_is_addi)
	   aspFrame("ds",0,0,0,0,0);
}

/*****************/
static void b_spwp()
/*****************/
{
  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  Wgmode();
  if (this_is_addi)
     spwpflag = (!spwpflag);
  if (spwpflag)
  {
    if (phaseflag)
      Wturnoff_mouse();
    Wturnoff_mouse();
    ds_mode = BOX_MODE; /* force b_cursor() into the CURSOR_MODE */
    if (this_is_addi)
      addi_buttons();
    activate_mouse(m_spwp,ds_reset);
    m_spwp(4, 0, 0, 0);
    dsstatus("sp wp");
    InitVal(FIELD3,HORIZ,SP_NAME, PARAM_COLOR,NOUNIT,
                         SP_NAME,-PARAM_COLOR,SCALED,2);
    InitVal(FIELD4,HORIZ,WP_NAME, PARAM_COLOR,NOUNIT,
                         WP_NAME,-PARAM_COLOR,SCALED,2);
  }
  else
    b_cursor();
}

/*****************/
static void b_thresh()
/*****************/
{ static int oldmode;
  int y,dum;

  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  Wgmode();
  if (threshflag)
  {
    vInfo  info;

    Wturnoff_mouse();
    oldmode = ds_mode;
    P_getVarInfo(CURRENT,"th",&info);
    thMin = (int) (dispcalib * info.minVal);
    activate_mouse(m_thcursor,ds_reset);
    y = dfpnt2 + (int) (dispcalib * (th + *cur_vp));
    set_cursors(0,TRUE,&y,
                 oldy_cursor[1],oldy_cursor[0],&dum,dfpnt2,mnumypnts - dfpnt2);
    if (y < 1)
      y = 1;
    threshY = y;
    update_ycursor(1,y);
    dsstatus("thresh");
    DispField1(FIELD3, PARAM_COLOR, "th");
    DispField2(FIELD3,-PARAM_COLOR, th, 1);
    InitVal(FIELD4,HORIZ,BLANK_NAME, PARAM_COLOR,NOUNIT,
                         BLANK_NAME, PARAM_COLOR,SCALED,2);
  }
  else
  {
    threshY = 0;
    ds_mode = (oldmode == CURSOR_MODE) ? -1 : CURSOR_MODE;
    b_cursor();
  }
}

/*****************/
static void exit_lvltlt()
/*****************/
{
  if (lvltltflag)
  {
    lvltltflag = FALSE;
    specdisp();
    InitVal(FIELD3,HORIZ,BLANK_NAME, PARAM_COLOR,NOUNIT,
                         BLANK_NAME, PARAM_COLOR,SCALED,2);
    InitVal(FIELD4,HORIZ,BLANK_NAME, PARAM_COLOR,NOUNIT,
                         BLANK_NAME, PARAM_COLOR,SCALED,2);
    ds_reset();
  }
}

/************************************/
static int m_lvltlt(int butnum, int x, int y, int moveflag)
/************************************/
{
  int    dum;
  static int    dnp,last_ph;
  double fine;
  extern float vs_mult();

  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  if (butnum == 4)
  {
    dnp = 0;
    update_xcursor(0,oldx_cursor[0]);
    update_xcursor(1,oldx_cursor[1]);
    update_xcursor(2,oldx_cursor[2]);
    return(COMPLETE);
  }
  if ((x<dfpnt)||(x>=dfpnt+dnpnt)) return 0;
  Wgmode();
  if (butnum == 2)
  {
    if (x<dfpnt + (int)(0.05 * (double)dnpnt))
    {
      io = (double) (y - dfpnt2) / dispcalib - *cur_vp;
      P_setreal(CURRENT,"io",io,0);
      DispField2(FIELD1,-PARAM_COLOR, io, 1);
    }
    else
    {
      is *= vs_mult(x,y,mnumypnts,dfpnt2 + (int)(dispcalib * (io + *cur_vp)),
                  dfpnt,dnpnt,spec2);
      MAXMIN(is,1.0e9,1.0e-6);
      P_setreal(CURRENT,"is",is,0);
      DispField2(FIELD2,-PARAM_COLOR, is, 1);
    }
    if (dnp)
    {
      y = oldy_cursor[0];
      update_ycursor(0,y);
    }
    if (intflag)
      intdisp();
  }
  else
  {
/*
    y = mnumypnts - y;
*/
    if (!moveflag)
    {
      int newyval = 0;
      if (dnp == 0)  /* this is true on the first mouse button only */
      {
        dsstatus("lvl");
        phase_cursors(x);
        x = (oldx_cursor[0] + oldx_cursor[1]) / 2;
        newyval = 1;
        do_lp = 0;
        dnp = 1;
      }
      else if ((x < oldx_cursor[0]) || (x > oldx_cursor[1]))
      {
        if (dnp == 1)
        {
          int  fpt,index,lastpt;
          double value;

          dsstatus("tlt");
          do_lp = (oldx_cursor[0] + oldx_cursor[1]) / 2;
          update_xcursor(2,do_lp);
          do_lp = datapoint(sp + wp + rflrfp - ((do_lp - dfpnt) * wp / dnpnt),sw,fn/2);
          lastpt = fpt = fpnt;
          index = 1;
          DPRINT2("fpnt=%d npnt=%d\n",fpnt,npnt);
          while ((index <= intresets) && (lastpt < do_lp))
          {
            if (P_getreal(CURRENT,"lifrq",&value,index))
              value = 0.0;          /*  no resets defined  */
            lastpt = datapoint(value,sw,fn/2);
            if ((lastpt < do_lp) && (lastpt > fpt))
              fpt = lastpt;
            index++;
          }
          do_lp += fpt;
          dnp = 2;
        }
        else
        {
          dsstatus("lvl");
          do_lp = 0;
          dnp = 1;
          update_xcursor(2,0);
        }
        phase_cursors(x);
        x = (oldx_cursor[0] + oldx_cursor[1]) / 2;
        newyval = 1;
      }
      if (newyval)
      {
        set_cursors(0,1,&y,
                 oldy_cursor[0],oldy_cursor[1],&dum,dfpnt2,mnumypnts - dfpnt2);
        update_ycursor(0,y);
        last_ph = y;
      }
    }
    if (y != last_ph)
    {
      fine = (butnum == 1) ? 1.0 : 8.0;
      if (do_lp)
      {
        double temp;

        temp = lvltlt_sense * (100.0 / mnumypnts) * (y - last_ph) / fine;
        tlt += temp;
        lvl -= ((double) do_lp/(double) (fn / 2)) * temp / 2.0;
        P_setreal(CURRENT,"tlt",tlt,0);
        DispField2(FIELD4,-PARAM_COLOR, tlt, 1);
      }
      else
      {
        lvl += lvltlt_sense * (100.0 / mnumypnts) * (y - last_ph) / fine;
      }
      P_setreal(CURRENT,"lvl",lvl,0);
      DispField2(FIELD3,-PARAM_COLOR, lvl, 1);
      aspFrame("resetVscale",0,0,0,0,0);
      if ((curspec = spectrum = calc_spec(specIndex-1,0,FALSE,TRUE,&updateflag))==0)
        return(ERROR);
      if (intflag)
        intdisp();
      last_ph = y;
    }
  }
  return(COMPLETE);
}

/*****************/
static void b_lvltlt()
/*****************/
{
  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
   if (!intflag)
      b_integral();
   Wturnoff_mouse();
   Wgmode();
   ds_reset();
   dsstatus("dc");
   DispField1(FIELD3, PARAM_COLOR, "lvl");
   DispField2(FIELD3,-PARAM_COLOR, lvl, 1);
   DispField1(FIELD4, PARAM_COLOR, "tlt");
   DispField2(FIELD4,-PARAM_COLOR, tlt, 1);
   lvltltflag = TRUE;
   do_lp = 0;
   ds_mode = -1;
   if (P_getreal(CURRENT,"lvltlt",&lvltlt_sense,1))
      lvltlt_sense = 1.0;
   else
   {
      if (lvltlt_sense <= 0.0)
          lvltlt_sense = 1.0;
   }
   if (this_is_addi)
     addi_buttons();
   m_lvltlt(4,dfpnt + dnpnt / 2,dfpnt2 + mnumypnts / 2,0);
   update_ycursor(1,threshY);
   grf_batch(0);
   activate_mouse(m_lvltlt,exit_lvltlt);
}

/************************************/
static void m_intreset(int butnum, int x, int y, int moveflag)
/************************************/
{ vInfo  info;

  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  Wgmode();
  if (butnum == 2)
    ds_multiply(x,y);
  else if ((x<dfpnt)||(x>=dfpnt+dnpnt))
    return;
  else if (!moveflag)
  { if (butnum == 1)
      z_add(sp + wp + rflrfp - ((x - dfpnt) * wp / dnpnt));
    else
      z_delete(sp + wp + rflrfp - ((x - dfpnt) * wp / dnpnt));
    intresets = ((P_getVarInfo(CURRENT,"lifrq",&info)) ? 1 : info.size);
    resetflag = TRUE;
    if (intflag)
      intdisp();
  }
}

/*****************/
static void exit_z()
/*****************/
{
  InitVal(FIELD3,HORIZ,BLANK_NAME, PARAM_COLOR,NOUNIT,
                       BLANK_NAME, PARAM_COLOR,SCALED,2);
  InitVal(FIELD4,HORIZ,BLANK_NAME, PARAM_COLOR,NOUNIT,
                       BLANK_NAME, PARAM_COLOR,SCALED,2);
  ds_reset();
}

/*****************/
static void b_z()
/*****************/
{
  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  Wturnoff_mouse();
  Wgmode();
  ds_reset();
  resetflag = TRUE;
  ds_mode = -1;
  if (intflag && regions)
    intdisp();
  activate_mouse(m_intreset,exit_z);
  ParameterLine(1,COLUMN(FIELD3),PARAM_COLOR,"add     ");
  ParameterLine(2,COLUMN(FIELD3),PARAM_COLOR,"reset   ");
  ParameterLine(1,COLUMN(FIELD4),PARAM_COLOR,"remove  ");
  ParameterLine(2,COLUMN(FIELD4),PARAM_COLOR,"reset   ");
/* commented out above 4 lines for VNMRJ? */

}

/*  The `ds_mark' routine is also used by the MARK command in FULL.C  */
/* specptr	pointer to spectrum
 * cr_val	value of CR
 * del_val	value of DELTA
 * mark_mode	BOX_MODE or CURSOR_MODE
 * display_flag	display results if TRUE
 * button_flag	called from button push if TRUE
 * int_v_ptr	address to store integral
 * max_i_ptr	address to store max intensity
 */

int ds_mark(float *specptr, double cr_val, double del_val,
            int mark_mode, int display_flag, int button_flag,
            float *int_v_ptr, float *max_i_ptr)
{
	int	 newfile, num_entry, npt;
	float	*ptr;
	float	 datamax, intval;
        double   mark_scl;

/*  The global variables `cr' and `delta' are NOT to be used in
    this routine; rather, use the arguments `cr_val' and `del_val'.  */

	strcpy(&mark_textfname[ 0 ],curexpdir);
#ifdef UNIX
	strcat(&mark_textfname[ 0 ],"/mark1d.out");
#else
	strcat(&mark_textfname[ 0 ],"mark1d.out");
#endif

/*  If `access' returns 0, the mark file already exists, thus `newfile'
    should also be zero.  If `access' returns a non-zero value, assume
    it failed because no file was present.				*/

	newfile = access( &mark_textfname[ 0 ], 0);
        num_entry = 1;
        if (!newfile && display_flag)
        {
           register int c;

           mark_textfptr=fopen(&mark_textfname[ 0 ],"r");
           while ((c = getc(mark_textfptr)) != EOF)
              if (c == '\n')
                 num_entry++;
           fclose(mark_textfptr);
           num_entry -= 1;
        }
	if ((mark_textfptr=fopen(&mark_textfname[ 0 ],"a")) == 0) {
		Werrprintf("cannot open file %s",&mark_textfname[ 0 ]);
		return 1;
	}
	if (newfile) {
		fprintf(mark_textfptr,
         "(high) frequency  (max) height       low frequency      integral\n"
		);
	}

/*  `button_flag' != 0 implies this routine was called as a result of a
    button push; i. e., from `b_mark'.

    The `mark' routine has previously called `init2d',
    so `vs' (along with `fn', `sw' and other global variables referenced
    here) have valid values.
 */

        mark_scl = vs;
  	if (normflag)
	  mark_scl *= normalize;

/*  `ptr' points to the point in the spectrum referneced by `cr_val'.	*/

	ptr = specptr + datapoint(cr_val+rflrfp,sw,fn/2);

/*  box mode (integrate over a range)	*/

	if (mark_mode == BOX_MODE) {
		intval = 0.0;
		npt = (int)(fabs(del_val) * (double) ((fn/2) - 1) / sw) + 1;
		maxfloat(ptr,npt, &datamax);
		datamax *= mark_scl;
		while (npt--)
		  intval += *ptr++;
		intval *= insval;
		fprintf(mark_textfptr,
	    "%10.3f        %10.3f        %10.3f        %10.3f\n",
             cr_val,datamax,cr_val-del_val,intval
		);
		if (display_flag)
		  Winfoprintf(
	   "Entry %d: cr=%10.3f and %10.3f, max height=%10.3f, integral=%10.3f",
            num_entry,cr_val,cr_val-del_val,datamax,intval
		);
	}

/*  cursor mode

    If this subroutine was called because the MARK button was pushed and
    the display is not expanded enough to show individual points as separate 
    pixels, the computer searches the vicinity of the cursor for the largest
    value in the spectrum; the area to search decreases as the expansion
    increases.

    If the MARK command is to search also, remove the check of `button_flag'.	*/

	else {
                npt = (int) (3.0 * (double) npnt / (double) dnpnt) + 1;
		if (npt >= 3 && button_flag != 0) {
			ptr -= npt / 2;
			if (ptr < specptr)
			  ptr = specptr;
			else if (ptr + npt >= specptr + fn/2)
			  ptr = specptr + fn/2 - npt - 1;
			maxfloat(ptr,npt, &datamax);
		}
		else
		  datamax = *ptr;

		datamax *= mark_scl;
		intval = 0.0;

		fprintf(mark_textfptr,"%10.3f        %10.3f\n",cr_val,datamax);
		if (display_flag)
		  Winfoprintf("Entry %d: cr=%10.3f, height=%10.3f",
                               num_entry,cr_val,datamax);
	}

	if (int_v_ptr != NULL)
	  *int_v_ptr = intval;
	if (max_i_ptr != NULL)
	  *max_i_ptr = datamax;
	fclose( mark_textfptr );

	return 0;
}

#ifdef OLD
/*************/
static void b_mark()
/*************/
{
  if (get_dis_setup() != 1)
    select_init( 0, GRAPHICS, NO_FREQ_DIM, NO_HEADERS,
        DO_CHECK2D, DO_SPECPARS, NO_BLOCKPARS, NO_PHASEFILE);
  Wgmode();
  ds_mark( spectrum, cr, delta, ds_mode, TRUE, TRUE, NULL, NULL );
}
#endif

/*****************/
static void b_integral()
/*****************/
{ int res __attribute__((unused));

  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  if (phaseflag)
    Wturnoff_mouse();
  Wgmode();
  intflag = !intflag;
  if (intflag)
  {
    intdisp();
    res=P_setstring(CURRENT,"intmod",(regions) ? INT_PARTIAL : INT_FULL,0);
    DispField1(FIELD1, PARAM_COLOR, "io");
    DispField2(FIELD1,-PARAM_COLOR, io, 1);
    DispField1(FIELD2, PARAM_COLOR, "is");
    DispField2(FIELD2,-PARAM_COLOR, is, 1);
  }
  else
  {
    erase2 = 0;
    displayspec(dfpnt,dnpnt,0,&spec2,&spec2,&erase2,0,0,INT_COLOR);
    erase2 = 0;
    res=P_setstring(CURRENT,"intmod",INT_OFF,0);
    DispField1(FIELD1, PARAM_COLOR, "vp");
    DispField2(FIELD1,-PARAM_COLOR, *cur_vp, 1);
    DispField1(FIELD2, PARAM_COLOR, "vs");
    DispField2(FIELD2,-PARAM_COLOR, *cur_vs, 1);
  }
  ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
  b_cursor();
}

/****************/
static void b_expand()
/****************/
{ double spsav,wpsav;
  double c2,d2;

  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  ds_reset();
  spsav = sp;
  wpsav = wp;
  if (ds_mode!=BOX_MODE)
  {
    dsstatus("FULL");
    /* set sp,wp for full display */
    sp  = - rflrfp;
    wp  = sw;
#ifdef VNMRJ
    ds_checkSpwp(&sp, &wp, VnmrJViewId);
    getCurrentZoomFromList(&c2,&d2,NULL,NULL);
    if(d2<0) {
	c2=cr;d2=delta;
    }
    cr=c2; delta=d2;
       UpdateVal(HORIZ,CR_NAME,cr,NOSHOW);
       UpdateVal(HORIZ,DELTA_NAME,delta,NOSHOW);
#endif
    if (revAxis)
       set_sp_wp_rev(&sp,&wp,sw,fn/2,rflrfp);
    else
       set_sp_wp(&sp,&wp,sw,fn/2,rflrfp);
    ds_mode=CURSOR_MODE; /* force b_cursor() into the BOX_MODE */
  }
  else
  {
    dsstatus("EXPAND");
    /* set sp,wp according to expansion box */
    if (interfero)
       sp  = cr;
    else if (revAxis)
       sp  = cr + delta;
    else
       sp  = cr - delta;
    wp  = delta;
    if (revAxis)
       set_sp_wp_rev(&sp,&wp,sw,fn/2,rflrfp);
    else
       set_sp_wp(&sp,&wp,sw,fn/2,rflrfp);
    if (interfero)
       cr = sp;
    else if ( !revAxis)
       cr = sp + wp;
    delta = wp;
    ds_mode = BOX_MODE; /* force b_cursor() into the CURSOR_MODE */
#ifdef VNMRJ
       addNewZoom("ds",cr,delta,0,0);
       ds_checkSpwp(&sp, &wp, VnmrJViewId);
       getNextZoom(&cr,&delta,NULL,NULL);
       UpdateVal(HORIZ,CR_NAME,cr,NOSHOW);
       UpdateVal(HORIZ,DELTA_NAME,delta,NOSHOW);
#endif
  }
  Wgmode();
  Wturnoff_mouse();
  if (this_is_addi)
    check_spwp(spsav,wpsav);
  /* store the parameters */
  UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
  UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
  P_setreal(CURRENT,"hzmm",wp/wc,0);
  exp_factors(TRUE);
  if (this_is_addi)
  {
    show_addsub();
    addi_disp(TRUE,TRUE,FALSE);
  }
  else
    specdisp();
  if (intflag)
    intdisp();
  if (dscale_on())
    new_dscale(MAYBE,TRUE);

  b_cursor();
#ifdef VNMRJ
  ds_sendSpwp(sp, wp, 1);
  if(is_aip_window_opened() && dsChanged()) aipSpecViewUpdate();
#endif
  if(graph_flag) execString("ds('again')\n");
  else if(!isInset()) redo_dpf_dpir();
  if ( ! this_is_addi)
     aspFrame("ds",0,0,0,0,0);
}

#ifdef OLD
/*************/
static void b_next()
/*************/
{
  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  Wgmode();
  Wturnoff_mouse();
  alt_menu = (!alt_menu);
  ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
  b_cursor();
}
#endif

extern int menuflag;
/****************/
static void b_return()
/****************/
{
  Wturnoff_buttons();
  Wsetgraphicsdisplay("ds");
  EraseLabels();
}

void ds_newVs(double newVs, char *ax) {
    char axis[8];
    getAxis(axis, 7);
    if(strcasecmp(axis,ax) != 0) return;

      *cur_vs = newVs;

      MAXMIN(*cur_vs,1.0e9,1.0e-6);
      P_setreal(CURRENT,"vs",*cur_vs,0);
      DispField2(FIELD2,-PARAM_COLOR, *cur_vs, 1);
      if ((this_is_addi) && (*curindex != spec3))
        show_addsub();
      specdisp();
}

void ds_sendVs()
{
   int i, vps;
   double d;
   char cmd[MAXSTR], axis[8];

    if (P_getreal(GLOBAL,"syncVs", &d, 1) != 0) d = 0.0;
    if(d < 0.5) return;

   if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
   else vps = (int)d;
   if(vps < 2) return;

   getAxis(axis, 7);

   for(i=0; i<vps; i++) {
        if(i+1 != VnmrJViewId) {
          if (P_getreal(GLOBAL,"jvpexps", &d, i+1) != 0) d = 0.0;
          if(d < 0.5) continue;

          sprintf(cmd, "vnmrjcmd('CR %d trackCursor(\\'vs\\', %f, \\'%s\\')')\n", i+1, vs, axis);
          execString(cmd);
        }
   }
}

/************************/
static void ds_multiply(int x, int y)
/************************/
{ extern float vs_mult();

  Wgmode();
  if ((x<dfpnt)||(x>=dfpnt+dnpnt)) return;

  // side spectrum of overlay mode
  if(getOverlayMode() == OVERLAID_ALIGNED) {
    int yoff;
    int chartMode = getChartMode();
    if(intflag) return;
    yoff = dfpnt2;
    if (chartMode == ALIGN_1D_Y) {
      yoff = mnumypnts-dfpnt2 + 4*ycharpixels;
    } else if(chartMode == ALIGN_1D_X || chartMode == ALIGN_1D_XY) {
      yoff = dfpnt2+dnpnt2+ycharpixels;
    }
    *cur_vs *= vs_mult(x,y,mnumypnts,yoff, dfpnt,dnpnt,*curindex);
    P_setreal(CURRENT,"vs",*cur_vs,0);
    execString("vs=vs\n");
    //specdisp();
    return;
  } 


  if ((curspec = spectrum = calc_spec(specIndex-1,0,FALSE,TRUE,&updateflag))==0)
     return;
  if(get_drawVscale()) new_dscale(TRUE,FALSE);
  if (intflag)
  {
    if (x<dfpnt + (int)(0.05 * (double)dnpnt))
    {
      io = (double) (y - dfpnt2) / dispcalib - *cur_vp;
      P_setreal(CURRENT,"io",io,0);
      DispField2(FIELD1,-PARAM_COLOR, io, 1);
    }
    else
    {
      is *= vs_mult(x,y,mnumypnts,dfpnt2 + (int)(dispcalib * (io + *cur_vp)),
                  dfpnt,dnpnt,spec2);
      MAXMIN(is,1.0e9,1.0e-6);
      P_setreal(CURRENT,"is",is,0);
      DispField2(FIELD2,-PARAM_COLOR, is, 1);
    }
    if (intflag) intdisp();
  }
  else
  {
    if (x<dfpnt + (int)(0.05 * (double)dnpnt))
    {
      *cur_vp = (double) (y - dfpnt2) / dispcalib;
      P_setreal(CURRENT,"vp",*cur_vp,0);
      DispField2(FIELD1,-PARAM_COLOR, *cur_vp, 1);
      if (dscale_on() && !get_drawVscale())
        new_dscale(TRUE,TRUE);
    }
    else
    {
      *cur_vs *= vs_mult(x,y,mnumypnts,dfpnt2 + (int)(dispcalib * *cur_vp), 
		dfpnt,dnpnt,*curindex);

      MAXMIN(*cur_vs,1.0e9,1.0e-6);
      P_setreal(CURRENT,"vs",*cur_vs,0);
      DispField2(FIELD2,-PARAM_COLOR, *cur_vs, 1);
      if ((this_is_addi) && (*curindex != spec3))
        show_addsub();
    }
    specdisp();
    if(get_drawVscale()) new_dscale(FALSE,TRUE);
#ifdef VNMRJ
    if(is_aip_window_opened() && dsChanged()) aipSpecViewUpdate();
    if(!isInset()) redo_dpf_dpir();
    ds_sendVs();
    if ( ! this_is_addi)
    {
       aspFrame("ds",0,0,0,0,0);
    }
#endif
  }
}

/*****************/
static void b_dscale()
/*****************/
{
  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
  if (dscale_on())
  {
    new_dscale(TRUE,FALSE);
#ifdef VNMRJ
    P_setreal(GLOBAL, "mfShowAxis", 0.0, 1);
#endif
  }
  else {
#ifdef VNMRJ
    P_setreal(GLOBAL, "mfShowAxis", 1.0, 0);
#endif
    new_dscale(FALSE,TRUE);
  }
  ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
  b_cursor();
#ifdef VNMRJ
  appendJvarlist("mfShowAxis");
#endif
}

/*****************/
static void turnoff_ds()
/*****************/
{
  char path[MAXPATHL];
  int savegrafIsOn;

  savegrafIsOn = grafIsOn;
  grafIsOn = 0;
  Wgmode();
  lvltltflag = FALSE;
  this_is_inset = FALSE;
  if (phaseflag)
     Wturnoff_mouse();
  phaseflag = FALSE;
  Wturnoff_mouse();
  ds_reset();
  endgraphics();
  if (integral)
    release(integral);
  integral = 0;
  rel_spec();
  if (this_is_addi)  {
    this_is_addi = FALSE;
    if (*curindex == spec1)
      sp_spec1 = sp;
    else
      sp_spec2 = sp;
    P_setreal(CURRENT,"vs",vs,0);
    P_setreal(CURRENT,"vp",vp,0);
    UpdateVal(HORIZ,SP_NAME,sp_spec1,NOSHOW);
    P_setreal(TEMPORARY,"vs",vs_spec2,0);
    P_setreal(TEMPORARY,"vp",vp_spec2,0);
    P_setreal(TEMPORARY,"sp",sp_spec2,0);
    if (spec3ptr)
    {
      D_close(D_USERFILE);
      release(spec3ptr);
      spec3ptr = 0;
    }
      
    D_getparfilepath(CURRENT, path, addsubpath);
    P_save(TEMPORARY,path);
    }
  grafIsOn = savegrafIsOn;
}

// do the following for side spectrum.
static void overlayspecdisp() {
    int chartMode = getChartMode();
    int vmin = 1;
    int vmax = mnumypnts - 3;
    //getVLimit(&vmin, &vmax);

    curcolor = getVpSpecColor();
    Wclear(2);
    //drawPlotBox();
    if(chartMode == ALIGN_1D_X) { 
       int yoff = dfpnt2+dnpnt2+ycharpixels;
       calc_ybars(curspec+fpnt,1,*cur_vs * *cur_scl,dfpnt,dnpnt,npnt, yoff,next);
       displayspec(dfpnt,dnpnt,0,&next,curindex,curerase,vmax,vmin,curcolor);
    } else if(chartMode == ALIGN_1D_Y) { 
       // note, in this case, "vertical" flag is set in vnmrj.
       // dfpnt,dnpnt and dfpnt2,dnpnt2 are swapped (set by frame_set_pnt).
       // mnumxpnts and mnumypnts, and mouse x,y are also swapped.
       int yoff = mnumypnts-dfpnt2 + 4*ycharpixels;
       calc_ybars(curspec+fpnt,1,*cur_vs * *cur_scl,dfpnt,dnpnt,npnt, yoff,next);
       displayspec(dfpnt,dnpnt,0,&next,curindex,curerase,vmax,vmin,curcolor);
    } else if(chartMode == ALIGN_1D_XY) { 
    // this will be the case to display side spectrum for a homo 2D 

       int fpts, npts;
       double hzpp, spx,wpx,spy,wpy;
       int erase=0;
       // calculate spectrum region for vertical dimension, based on the dconi's spy,wpy. 
       // this works only if homo 2D is symmetrical 
       fpts=fpnt;
       npts=npnt;

       getSpwpInfo(&spx,&wpx,&spy,&wpy);
       spy = ppm2Hz(HORIZ, spy);
       wpy = ppm2Hz(HORIZ, wpy);
       hzpp = sw/((double)(fn/2));
       npnt = (int)(wpy/hzpp + 0.01) + 1;
       if (get_axis_freq(HORIZ))
       {
        fpnt = (int)((sw-spy-rflrfp-wpy)/hzpp + 0.01);
       }
       else
       {
        fpnt = (int)(spy/hzpp + 0.01);
       }

       // Now draw vertical side spectrum (similar to y projection).
       // note, in this case, "vertical" flag is NOT set in vnmrj.
       // dfpnt,dnpnt and dfpnt2,dnpnt2 are NOT swapped.
       // but mnumxpnts and mnumypnts, and mouse x,y are NOT swapped.
       int yoff = mnumxpnts-dfpnt + 4*ycharpixels;
       vmin = 1;
       vmax = mnumxpnts - 3;
       calc_ybars(curspec+fpnt,1,*cur_vs * *cur_scl,dfpnt2,dnpnt2,npnt, yoff,next);
       displayspec(dfpnt2,dnpnt2,1,&next,curindex,&erase,vmax,vmin,curcolor);
   
       // restore fpnt, npnt for horizontal spectrum region.
       fpnt=fpts;
       npnt=npts;
       yoff = dfpnt2+dnpnt2+ycharpixels;
       vmin = 1;
       vmax = mnumypnts - 3;

       calc_ybars(curspec+fpnt,1,*cur_vs * *cur_scl,dfpnt,dnpnt,npnt, yoff,next);
       displayspec(dfpnt,dnpnt,0,&next,curindex,curerase,vmax,vmin,curcolor);

    }
}

// MAXSPEC=6
// Note: first spec uses curspec, curindex and curerase.
// call setCurSpec(i) to select spec, then calc_spec will get data for that spec.
static void mspecdisp() {
    // set pars for vertical scale display
    double vscale = *cur_vs * *cur_scl;
    int yoff = dfpnt2 + (int)(dispcalib * *cur_vp);
    int i, soff, scolor;
    int vmin = 1;
    int vmax = mnumypnts - 3;
    getVLimit(&vmin, &vmax);
    drawPlotBox();
    set_vscale(yoff,vscale);

    // start from 1 (second spec)
    for(i=1; i<MAXSPEC; i++) { 
      if((spectrum = calc_mspec(i, specIndex-1,0,FALSE,TRUE,&updateflag))) {
         soff = getSpecYoff(i); 
	 scolor=setSpecColor(i);
         calc_ybars(spectrum+fpnt,1,vscale,dfpnt,dnpnt,npnt,yoff+soff,next);
         displayspec(dfpnt,dnpnt,0,&next,&mspec[i],&merase[i],vmax,vmin,scolor);
      }
    }
    // now display first spec
    i=0;
    if((curspec = spectrum = calc_mspec(i, specIndex-1,0,FALSE,TRUE,&updateflag))) {
      soff = getSpecYoff(i); 
      scolor=setSpecColor(i);
      curcolor=scolor;
      calc_ybars(curspec+fpnt,1,vscale,dfpnt,dnpnt,npnt,yoff+soff,next);
      displayspec(dfpnt,dnpnt,0,&next,curindex,curerase,vmax,vmin,scolor);
    }
    color(curcolor);

}

/****************/
static void specdisp()
/****************/
{
  if(getShowMspec() > 0) mspecdisp();
  else if(getOverlayMode() == OVERLAID_ALIGNED) overlayspecdisp();
  else {

    // set pars for vertical scale display
    float *bcdata;
    double vscale = *cur_vs * *cur_scl;
    int yoff = dfpnt2 + (int)(dispcalib * *cur_vp);
    int vmin = 1;
    int vmax = mnumypnts - 3;
    getVLimit(&vmin, &vmax);
    drawPlotBox();
    set_vscale(yoff,vscale);

    if ( ! this_is_addi )
       curcolor = getVpSpecColor();
    calc_ybars(curspec+fpnt,1,vscale,dfpnt,dnpnt,npnt,yoff,next);
    displayspec(dfpnt,dnpnt,0,&next,curindex,curerase,vmax,vmin,curcolor);
    
    if((bcdata = getBcdata(specIndex, fn/2, 0)) != NULL ) {
      // bcdata is pointed to bcdata in integ.c
      calc_ybars(bcdata+fpnt,1,vscale,dfpnt,dnpnt,npnt,yoff,next);
      displayspec(dfpnt,dnpnt,0,&next,&baselineBufID,curerase,vmax,vmin,PINK_COLOR);
    }

  } 
}

/*******************************/
/* blanks a region of ybars */
/*******************************/
static void clear_int_ybar(struct ybar *ybar_ptr, int n)
{ register int i;

  i = n;
  (ybar_ptr + n - 1)->mn = 1;
  (ybar_ptr + n - 1)->mx = 0;
  if (i >= 2)
  {
    (ybar_ptr + n - 2)->mn = 1;
    (ybar_ptr + n - 2)->mx = 0;
  }
  while (i > 1)
  { ybar_ptr->mn = 1;
    ybar_ptr->mx = 0;
    ybar_ptr++;
    ybar_ptr->mn = 1;
    ybar_ptr->mx = 0;
    i -= 3;                 /* clear every other pair of points */
    ybar_ptr += 2;
  }
}

/****************/
static void intdisp()
/****************/
{
  extern struct ybar *outindex();
  struct ybar *out_ptr;
  double  vs1;
  double value;
  int    index;
  int    point;
  int    lastpt;
  float  factor;
  int chartMode = getChartMode();
  int vmin = 1;
  int vmax = mnumypnts - 3;
  getVLimit(&vmin, &vmax);
  drawPlotBox();
  if(chartMode == ALIGN_1D_X || chartMode == ALIGN_1D_Y|| chartMode == ALIGN_1D_XY) return;

  if ((!integral) || (updateflag))
  {
    if (!integral)
      if ((integral =
          (float *) allocateWithId(sizeof(float) * fn / 2,"ds"))==0)
      {
        Werrprintf("cannot allocate integral buffer");
        Wturnoff_buttons();
        return;
      }
    if ((curspec = spectrum = calc_spec(specIndex-1,0,FALSE,TRUE,&updateflag))==0)
       return;
    integ(spectrum,integral,fn / 2);
    updateflag = FALSE;
  }
  vs1 = is * dispcalib / ( (double)fn / 128.0 );

  DPRINT1("integral scale factor=%g\n",vs1);
  int_ybars(integral,vs1,dfpnt,dnpnt,fpnt,npnt,
             dfpnt2 + (int)(dispcalib * (io + *cur_vp)),next,intresets,sw,fn);
  out_ptr = outindex(next) + dfpnt;
  if (intresets>1)
  {
    if (dnpnt > npnt)
      factor = (float) dnpnt / (float) (npnt -1);
    else 
      factor = (float) dnpnt / (float) npnt;
    lastpt = fpnt;
    index = 1;
    DPRINT2("fpnt=%d npnt=%d\n",fpnt,npnt);
    while ((index <= intresets) && (lastpt < fpnt + npnt - 1))
    {
      if (P_getreal(CURRENT,"lifrq",&value,index))
        value = 0.0;          /*  no resets defined  */
      point = datapoint(value,sw,fn/2);
      if (point>fpnt)
      {
        if (point>fpnt+npnt-1)
          point = fpnt + npnt -1;
        if (lastpt < fpnt)
          lastpt = fpnt;
        if (point < lastpt)
          point = fpnt + npnt -1;
        DPRINT2("start=%d num=%d \n",
               (int) (factor*(float)(lastpt-fpnt)),
               2 + (int) (factor * (float)(point-lastpt)));
        if ((regions) && (index % 2))
          if (resetflag)
            clear_int_ybar(out_ptr + (int) (factor*(float)(lastpt-fpnt)),
                       2 + (int) (factor * (float)(point-lastpt)));
          else
            clear_ybar(out_ptr + (int) (factor*(float)(lastpt-fpnt)),
                       2 + (int) (factor * (float)(point-lastpt)));
        else
          clear_ybar(out_ptr + (int) (factor*(float)(point-1-fpnt)),
                     2 + (int) factor);
      }
      index++;
      lastpt = point;
    }
  }
  displayspec(dfpnt,dnpnt,0,&next,&spec2,&erase2,vmax,vmin,INT_COLOR);
}

/*************/
static void setwindows()
/*************/
{
  if (doClear)
     Wclear_graphics();
  else
     doClear = 1;
  change_color(1,1);        /* reset the color table */
  refresh_graf();           /* some window systems needs refresh */
  Wgmode(); /* goto tek graphics and set screen 2 active */
  Wshow_graphics();
}

/*---------------------------------------
|					|
|		selecT()/4		|
|					|
+--------------------------------------*/
int selecT(int argc, char *argv[], int retc, char *retv[])
{
   int		arg_no;


   arg_no = 1;
   if ((arg_no < argc) && !isReal(argv[arg_no]) )
   {
      select2D = ( (strcmp(argv[arg_no], "f1f3") == 0) ||
		   (strcmp(argv[arg_no], "f3f1") == 0) ||
		   (strcmp(argv[arg_no], "f2f3") == 0) ||
		   (strcmp(argv[arg_no], "f3f2") == 0) ||
		   (strcmp(argv[arg_no], "f1f2") == 0) ||
		   (strcmp(argv[arg_no], "f2f1") == 0) ||
                   (strcmp(argv[arg_no], "f1f4") == 0) ||
                   (strcmp(argv[arg_no], "f4f1") == 0) ||
                   (strcmp(argv[arg_no], "f2f4") == 0) ||
                   (strcmp(argv[arg_no], "f4f2") == 0) ||
                   (strcmp(argv[arg_no], "f3f4") == 0) ||
                   (strcmp(argv[arg_no], "f4f3") == 0) ||
		   (strcmp(argv[arg_no], "proj") == 0) ||
		   ( (strcmp(argv[arg_no], "next") == 0) && select2D ) ||
		   ( (strcmp(argv[arg_no], "prev") == 0) && select2D ) ||
		   ( (argv[arg_no][0] == 'p') && (argv[arg_no][1] == 'l') ) );

      select1D = (!select2D);
      if ( (strcmp(argv[arg_no], "next") != 0) &&
	   (strcmp(argv[arg_no], "prev") != 0) &&
	   select1D )
      {
         Werrprintf("select: invalid argument %s",argv[arg_no]);
         return(ERROR);
      }
   }
   else
   {
      select2D = FALSE;
      select1D = TRUE;
   }

   if (select2D)
   {
      if ( selectpl(argc, argv) )
         return(ERROR);
   }
   else if (select1D)
   {
      if (argc > 2)
      {
         Werrprintf("select: valid 1D arguments are 'prev', 'next' or a spectrum index");
         ABORT;
      }

      if (argc == 2)
      {
         if ( isReal(argv[1]) )
         {
            specIndex = (int) stringReal(argv[1]);
            if (specIndex < 1)
               specIndex = 1;
         }
         else
         {
            if ( strcmp(argv[1], "next") == 0 )
            {
               specIndex += 1;
            }
            else if ( strcmp(argv[1], "prev") == 0 ) 
            { 
               specIndex -= 1;
               if (specIndex < 1)
                  specIndex = 1;
            }
            else
            {
               Werrprintf("select: valid 1D arguments are 'prev', 'next' or a spectrum index");
               ABORT;
            }
         }
      }

      if (retc > 0)
      {
         if (retc > 1)
         {
            Werrprintf("Only one value returned in %s", argv[0]);
            ABORT;
         }
         else
         {
            retv[0] = realString((double) currentindex());
         }
      }
      else
      {
         disp_specIndex(specIndex);
      }
   }
   else
   {
      Werrprintf("usage - select:  invalid type of data selected");
      ABORT;
   }

   return(COMPLETE);
}

/****************************/
int ds_checkinput(int argc, char *argv[], int *trace)
/****************************/
{ int arg_no;

  arg_no = 1;

  if (argc>arg_no)
  {
    if (isReal(argv[arg_no]))
    {
      clearMspec();
      *trace = (int) stringReal(argv[arg_no]);
      arg_no++;
    }
  }

  DPRINT1("current trace= %d\n",*trace);
  return(COMPLETE);
}

/****************/
static int check_int()
/****************/
/* set intflag, regions, and number of integral resets */
{ int    res;
  vInfo  info;
  double value;
  
  intresets = ((P_getVarInfo(CURRENT,"lifrq",&info)) ? 1 : info.size);
  if (P_getreal(CURRENT,"lifrq",&value,intresets))
    value = 0.0;
  if ((value > 0.5) || (value < 0.0))
  {
     intresets++;
     P_setreal(CURRENT,"lifrq",0.0,intresets);
  }
  if ( (res=P_getstring(CURRENT,"intmod",intmod,1,8)) )
  { P_err(res,"intmod",":"); return(ERROR); }
  intflag = (strcmp(intmod,INT_OFF) != 0); 
  if (intflag)
  {
    regions = (strcmp(intmod,INT_PARTIAL) == 0); 
    res=P_setactive(CURRENT,"lifrq",(regions) ? ACT_ON : ACT_OFF);
  }
  else
    regions = ((P_getVarInfo(CURRENT,"lifrq",&info)) ? FALSE : info.active);
  return(COMPLETE);
}

static void init_mspec()
{
  int i;
  plot = FALSE;
  ds_mode = -1;
  if (P_setstring(GLOBAL,"crmode","c",0))
    Werrprintf("Unable to set variable \"crmode\".");
  threshflag  = FALSE;
  resetflag  = FALSE;
  spwpflag = FALSE;
  lvltltflag = FALSE;
  alt_menu = FALSE;
  dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;

  for(i=0;i<MAXSPEC;i++) {
    mspec[i]=i;
    merase[i]=0;
  } 
  spec1  = 0;
  spec2  = 1;
  next   = MAXSPEC;
  erase1 = 0;
  erase2 = 0;
  curindex = &spec1;
  curerase = &erase1;
  cur_vp   = &vp;
  cur_vs   = &vs;
  phaseflag = FALSE;
  oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0;
  init_display();
  setwindows();
  ResetLabels();
}

/*************/
static void init_vars1()
/*************/
{
  plot = FALSE;
  ds_mode = -1;
  if (P_setstring(GLOBAL,"crmode","c",0))
    Werrprintf("Unable to set variable \"crmode\".");
  threshflag  = FALSE;
  resetflag  = FALSE;
  spwpflag = FALSE;
  lvltltflag = FALSE;
  alt_menu = FALSE;
  dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;

  spec1  = 0;
  spec2  = 1;
  next   = 2;
  baselineBufID = 4;
  erase1 = 0;
  erase2 = 0;
  curindex = &spec1;
  curerase = &erase1;
  curcolor = color1 = getVpSpecColor();
  cur_vp   = &vp;
  cur_vs   = &vs;
  phaseflag = FALSE;
  oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0;
  init_display();
  setwindows();
  ResetLabels();
}

/*************/
static int init_vars2()
/*************/
{
/*  Following test & action removed; the MARK output file
    is closed between MARK operations.				*/

/*if (mark_textfptr != NULL)
    {
      fclose(mark_textfptr);
      mark_textfptr = NULL;
    }*/
/*spec1  = 0;
  spec2  = 1;
  next   = 2;
  erase1 = 0;
  erase2 = 0;
  curindex = &spec1;
  curerase = &erase1;
  curcolor = color1 = SPEC_COLOR;
  cur_vp   = &vp;
  cur_vs   = &vs;
  phaseflag = FALSE;
  dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
  oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0;
  init_display();
  setwindows();
  ResetLabels();*/

  dispcalib = (float) (mnumypnts-ymin) / (float) wc2max;
  integral = 0;

  aspFrame("resetVscale",0,0,0,0,0);
  if ((curspec = spectrum = calc_spec(specIndex-1,0,FALSE,TRUE,&updateflag))==0)
    return(ERROR);
  scale = dispcalib;
  if (normflag)
    scale *= normalize;
  if (threshflag)
  {
    threshY = dfpnt2 + (int) (dispcalib * (th + *cur_vp));
    if (threshY < 1)
      threshY = 1;
  }
  cur_scl = &scale;
  return(COMPLETE);
}

/*************/
static int init_ds(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  (void) retc;
  (void) retv;
  if (ds_checkinput(argc,argv,&specIndex)) return(ERROR);
  if(init2d(GET_REV,GRAPHICS)) return(ERROR);
  interfero = (get_axis_freq(HORIZ) == 0);
  revAxis = 0;
  if ( ! interfero )
  {
     double scaleval, spval, wpval;

     get_scale_pars( HORIZ, &spval, &wpval, &scaleval, &revAxis );
  }
  if ((specIndex < 1) || (specIndex > nblocks * specperblock))
  { if (!WgraphicsdisplayValid(argv[0]) && (argc>1) && (isReal(argv[1])))
      Werrprintf("spectrum %d does not exist",specIndex);
    specIndex = 1;
  }
  if(getShowMspec() > 1) init_mspec();
  else init_vars1();
  graph_flag=FALSE;
  start_from_ft = 0;
  ds_sendSpecInfo(1);
  if ( ! this_is_addi)
  {
     aspFrame("clearAspSpec",0,0,0,0,0);
     aspFrame("resetVscale",0,0,0,0,0);
  }
  return(COMPLETE);
}

// this is a test for whether to update csi grid display
// csi grid display only update for sp,wp amd vs.
int dsChanged() {
  if(sp != oldsp || wp != oldwp || vs != oldvs) return 1;
  else return 0;
}

/*************/
int ds(int argc, char *argv[], int retc, char *retv[])
/*************/
{
  int redisplay_flag, screen_changed, do_menu=FALSE;
  int fidshim = FALSE;
  int redoMode, old_threshflag;
  int vsThumbWheel = 0;
  int vsflag=0;

  if (Bnmr)
  {
    if ( (argc == 2) && isReal(argv[1]) )
       selecT(argc,argv,retc,retv);
    return(COMPLETE);
  }

#ifdef VNMRJ
  // by default xor is off, but will be turned on by b_phase()
  if (isXorOn)
      vj_xoroff();
  isXorOn = 0;
#endif

  if(retc>0 && argc>1 && strcmp(argv[1],"thresh") == 0) {
    if(threshflag) retv[0]=realString(1.0);
    else retv[0]=realString(0.0);
    return(COMPLETE);
  } else if(retc>0) {
    int dpf_flag=0, dpir_flag=0;
    get_dpf_dpir(&dpf_flag, &dpir_flag);
    if(argc>0 && strcmp(argv[1],"dpf") ==0) retv[0]=realString((double)dpf_flag);
    else if(argc>0 && strcmp(argv[1],"dpir") ==0) retv[0]=realString((double)dpir_flag);
    return(COMPLETE);
  }

  if (argc > 1 && strcmp(argv[1],"exit") == 0)
  {
      if (WgraphicsdisplayValid(argv[0]))
      {
         Wturnoff_buttons();
         Wsetgraphicsdisplay("");
         EraseLabels();
      }
      return(COMPLETE);
  }
  if ((argc == 3) && (strcmp(argv[1],"vsAdj") == 0) )
  {
     if (phaseflag)
     {
      *cur_vs = atof(argv[2]);
      MAXMIN(*cur_vs,1.0e9,1.0e-6);
      P_setreal(CURRENT,"vs",*cur_vs,0);
      m_newphase(5,0,0,0);
      return(COMPLETE);
     }
  }

/*
int i;
for(i=0;i<argc;i++)
Winfoprintf("##ds %d %d %s",argc,i,argv[i]);
*/

  select2D = FALSE;
  select1D = TRUE;
  doClear = 1;
  redoMode = FALSE;
  old_threshflag = threshflag;
  y_crosshair(0,0,0,0);
  screen_changed = 0;

#ifdef VNMRJ
  if(argc == 1) {
    int dpf_flag=0, dpir_flag=0;
    get_dpf_dpir(&dpf_flag, &dpir_flag);
    if(dpf_flag) set_dpf_flag(0,"");
    if(dpir_flag) set_dpir_flag(0,"");
    if(dpf_flag || dpir_flag) screen_changed = 1; 
  }

  if (!isInset() && argc>1 && ! strcmp(argv[argc-1], "redisplay parameters")) {
	  int i;
          for(i=0;i<argc;i++) {
            if( strstr(argv[i],"lifrq") != NULL || strstr(argv[i],"llfrq") != NULL ||
                strstr(argv[i],"liamp") != NULL || strstr(argv[i],"llamp") != NULL ) {
                screen_changed = 1;
            }
          }
  }

  if (argc>1) {
       if ( ! strcmp(argv[argc-1], "redisplay parameters") || 
            ! strcmp(argv[1], "again") ||
            ! strcmp(argv[1], "redisplay") ) {
            redoMode = TRUE;
       }
       if (showPlotBox() && !isInset() && ! strcmp(argv[argc-1], "redisplay parameters")) {
          int i;
	  for(i=0;i<argc;i++) { 
	    if( strstr(argv[i],"wc") != NULL || strstr(argv[i],"sc") != NULL ||
		strstr(argv[i],"wc2") != NULL || strstr(argv[i],"sc2") != NULL ) {
		screen_changed = 1;
		//old_ds_mode = ds_mode;
	    }
	    if( strcmp(argv[i],"vs") == 0) vsflag=1;
	  }
       }
       if( strcmp(argv[1], "again") && strcmp(argv[1], "redisplay") &&
		strcmp(argv[1], "toggle") && strcmp(argv[1], "dscale") && 
		strcmp(argv[1], "spwp") && strcmp(argv[1], "expand") ) {
	   clearBcdata(); // if not above, clear baseline buffer in integ.c
       } 
  } else clearBcdata(); 

  if(!redoMode && !old_threshflag) setButtonMode(SELECT_MODE); 

  if (isJprintMode() || redoMode)
       old_ds_mode = ds_mode;
#endif

  if (argc > 1 && strcmp(argv[1],"fidshim") == 0) {
      fidshim = TRUE;
  }

  if (!fidshim) {
      Wturnoff_buttons();
      if (WgraphicsdisplayValid(argv[0])) {
          set_turnoff_routine(turnoff_ds);
      }
  }

  setdisplay();
  if(!isInset() && !screen_changed)
    screen_changed = ( (old_mnumxpnts != mnumxpnts) || (old_mnumypnts != mnumypnts) || inRepaintMode);

#ifdef CLOCKTIME
  /* Turn on a clocktime timer */
  (void)start_timer ( ds_timer_no );
#endif

  if (argc == 2)
  {
    if (strcmp(argv[1],"toggle") == 0)
    {
      if ((WgraphicsdisplayValid(argv[0]) || WgraphicsdisplayValid("addi"))
		 && ( ! start_from_ft))
      {
	//threshflag = 0;
	setButtonMode(SELECT_MODE); 
        b_cursor();
        RETURN;
      }
      else
      {
        Werrprintf("Must be in ds to use %s option",argv[1]);
        ABORT;
      }
    }
    else if (strcmp(argv[1],"noclear") == 0)
    {
       doClear = 0;
       releasevarlist();
    }
    else if (strcmp(argv[1],"restart") == 0)
    {
      if ((WgraphicsdisplayValid(argv[0]) || WgraphicsdisplayValid("addi"))
		 && ( ! start_from_ft))
      {
	if (this_is_inset)
	  this_is_inset = FALSE;
        if (ds_mode == CURSOR_MODE)
          ds_mode = BOX_MODE;
        else if (ds_mode == BOX_MODE)
          ds_mode = CURSOR_MODE;
        b_cursor();
        grf_batch(0);
        RETURN;
      }
      else
      {
        Werrprintf("Must be in ds to use %s option",argv[1]);
        ABORT;
      }
    }
    else if (strcmp(argv[1],"expand") == 0)
    {
      if ((WgraphicsdisplayValid(argv[0]) || WgraphicsdisplayValid("addi"))
		 && ( ! start_from_ft))
      {
        b_expand();
        grf_batch(0);
        RETURN;
      }
      else
      {
        Werrprintf("Must be in ds to use %s option",argv[1]);
        ABORT;
      }
    }
    else if (strcmp(argv[1],"spwp") == 0)
    {
      if ((WgraphicsdisplayValid(argv[0]) || WgraphicsdisplayValid("addi"))
		 && ( ! start_from_ft))
      {
        spwpflag = (!spwpflag);
  	if (spwpflag) setButtonMode(SPWP_MODE);
  	else setButtonMode(SELECT_MODE);
        b_spwp();
        grf_batch(0);
        RETURN;
      }
      else
      {
        Werrprintf("Must be in ds to use %s option",argv[1]);
        ABORT;
      }
    }
    else if (strstr(argv[1],"phase"))
    {
      if ((WgraphicsdisplayValid(argv[0]) || WgraphicsdisplayValid("addi"))
		 && ( ! start_from_ft))
      {
        newphaseflag = (strcmp(argv[1],"newphase") == 0);
   	setButtonMode(PHASE_MODE);
        b_phase();
        grf_batch(0);
        RETURN;
      }
      else
      {
        Werrprintf("Must be in ds to use %s option",argv[1]);
        ABORT;
      }
    }
    else if (strcmp(argv[1],"thresh") == 0)
    {
      if ((WgraphicsdisplayValid(argv[0]) || WgraphicsdisplayValid("addi"))
		 && ( ! start_from_ft))
      {
        threshflag = (threshflag == 0);
  	if (threshflag) setButtonMode(THRESHOLD_MODE);
  	else setButtonMode(SELECT_MODE);
        b_thresh();
        if (threshflag)
            dispCursor = 0;
        grf_batch(0);
        RETURN;
      }
      else
      {
        Werrprintf("Must be in ds to use %s option",argv[1]);
        ABORT;
      }
    }
    else if (strcmp(argv[1],"z") == 0)
    {
      if ((WgraphicsdisplayValid(argv[0]) || WgraphicsdisplayValid("addi"))
		 && ( ! start_from_ft))
      {
  	setButtonMode(REGION_MODE);
        b_z();
        grf_batch(0);
        RETURN;
      }
      else
      {
        Werrprintf("Must be in ds to use %s option",argv[1]);
        ABORT;
      }
    }
    else if (strcmp(argv[1],"dscale") == 0)
    {
      if ((WgraphicsdisplayValid(argv[0]) || WgraphicsdisplayValid("addi"))
		 && ( ! start_from_ft))
      {
        b_dscale();
        grf_batch(0);
        RETURN;
      }
      else
      {
        Werrprintf("Must be in ds to use %s option",argv[1]);
        ABORT;
      }
    }
    else if (strcmp(argv[1],"rev") == 0)
    {
       dscaleRevFlag = 1;
       releasevarlist();
    }
    else if (strcmp(argv[1],"lvltlt") == 0)
    {
      if ((WgraphicsdisplayValid(argv[0]) || WgraphicsdisplayValid("addi"))
		 && ( ! start_from_ft))
      {
   	setButtonMode(LVLTLT_MODE);
        b_lvltlt();
        grf_batch(0);
        RETURN;
      }
      else
      {
        Werrprintf("Must be in ds to use %s option",argv[1]);
        ABORT;
      }
    }
    else if (strcmp(argv[1],"scwc") == 0)
    {
      if ((WgraphicsdisplayValid(argv[0]) || WgraphicsdisplayValid("addi"))
		 && ( ! start_from_ft))
      {
        b_scwc();
        grf_batch(0);
        RETURN;
      }
      else
      {
        Werrprintf("Must be in ds to use %s option",argv[1]);
        ABORT;
      }
    }
    else if ( (strcmp(argv[1],"again") == 0) || (strcmp(argv[1],"redisplay") == 0) )
    {
        redoMode = 1;
    }
    else if (strcmp(argv[1],"fidshim"))
    {
      /* Already checked, but avoid error of default "else" */
      if (!isReal(argv[1]))
      {
        Werrprintf("Illegal ds option : %s",argv[1]);
        ABORT;
      }
    }
  }

  if (fidshim) {
      erasespec(*curindex, 0, curcolor);
      *curerase = 0;
      if (check_int()) {
          return(ERROR);
      }
      if (init_vars2()) {
          return(ERROR);
      }
      specdisp();
      grf_batch(0);
      return COMPLETE;
  }

/*  Redisplay Parameters?  */

  redisplay_flag = 0;
  if (argc > 1)
  {
    
    if (!screen_changed && strcmp( argv[ argc-1 ], "redisplay parameters" ) == 0)
    {
            argc--;

/*  autoRedisplay (in lexjunk.c) is supposed
    to insure the next test returns TRUE.	*/

	    if (WgraphicsdisplayValid( argv[ 0 ] ))
            {
              redisplay_flag = 1;
              if ( (argc == 2) && ! strcmp(argv[1],"vs") )
              {
                 vsThumbWheel = 1;
              }
            }
    }
    if (strcmp( argv[ 1 ], "redisplay" ) == 0) {
	    if (WgraphicsdisplayValid( argv[ 0 ] ))
              redisplay_flag = 1;
    }
  }

/*  Test for start-ds-from-beginning vs. refresh display.
    ds starts from the beginning if any of the following are true:
    1.  The current graphics command is not "ds"
    2.  The command has an argument list and the 1st one is a number
    3.  The current spectrum is not defined.
    4.  The program is executing as a consequence of the `ft' command.
    5.  The redisplay parameters flag is 0.
    6.  The screen has changed.  (screen_changed flag is set.)		*/

  if (!WgraphicsdisplayValid(argv[0]) ||
      ((argc>1) && (isReal(argv[1]))) ||
      (cur_spec() < 0) || (start_from_ft) ||
      redisplay_flag == 0 || screen_changed)
  {
/*  Come here if ds program is starting from the beginning.
    Turn off the buttons.  Clear the graphics screen
    (Wclear_graphics <= init_vars1 <= init_ds).			*/

    oldsp=sp;
    oldwp=wp;
    oldvs=vs;
    if (init_ds(argc,argv,retc,retv))
      return(ERROR);

    if(redoMode) {
       threshflag = old_threshflag;
    }

    // do this when screen_changed, so ds_mode won't be forced to CURSOR_MODE. 
    if(screen_changed || redoMode) ds_mode = (old_ds_mode == BOX_MODE) ? CURSOR_MODE : -1;

    if ( (argc>1)  && ! strcmp(argv[1], "redisplay") )
    {
       reset_dscale();
    }
    else
    {
       do_menu = TRUE;
    }
    redisplay_flag = 0;
    if (argc == 1)
    {
       setButtonMode(SELECT_MODE);
       ds_mode = -1; /* force b_cursor() into the CURSOR_MODE */
    }
    if ( (argc == 2) && ! strcmp(argv[1],"rev") )
       dscaleRevFlag  = TRUE;
    else
       dscaleRevFlag  = FALSE;
    set_turnoff_routine(turnoff_ds);
  }
  else
  {

/*  Come here if refreshing an older ds display.  */

/*	Old test:							  */
/*  if (WgraphicsdisplayValid(argv[0]) && (argc>1) && (!isReal(argv[1]))) */
/*									  */
/*	Replaced with test of `redisplay_flag'.				  */
/*	Notice in the old version this block was skipped if "ds" was	  */
/*	not the current graphics command; and "ds" must be current for    */
/*	`redisplay_flag' to be true.					  */

      if (redisplay_flag) {
          int res, update;
          vInfo info;
          char tmpStr[MAXSTR];

          /* We only check the first parameter in the arg list */
          update = 1;
          if (strcmp( argv[ 1 ], "redisplay" ) == 0)
          {
             strcpy(tmpStr, "vs");
          }
          else
          {
             strcpy(tmpStr, argv[1]);
             strtok(tmpStr,",");
          }
          if ((res = P_getVarInfo(CURRENT,tmpStr,&info)) == 0)
          {
              if (info.group != G_DISPLAY)
              {
                  update = 0;
              }
          }
          else
          {
             if (strcmp(tmpStr,"dummy"))
             {
                update = 0;
             }
          }
          if (update == 0)
          {
              if (threshflag) {
                 //threshflag = 0;
                 b_thresh();
              }
              if (dispCursor) {
                 ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
                 b_cursor();
              }
              return( COMPLETE );
          }
          if ((strcmp(argv[1],"intmod") == 0) && (argc == 2))  {
              if (intflag) {
                  erasespec( spec2, 0, INT_COLOR );
                  erase2 = 0;
              }
              if (check_int()) return(ERROR);
              if (intflag)
                  intdisp();
              ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
              b_cursor();
              return(COMPLETE);
          }
      }

    erasespec( *curindex, 0, curcolor );
    *curerase = 0;

/*  This value of `intflag' is from the last time ds ran.  If it is set,
    assume the integral is displayed and take it down by displaying the
    same set of ybars (index is `spec2').  We do not want to use the
    "erase old" feature in `displayspec', so we send it zero for the
    corresponding argument.  Set `erase2' to 0 so the integral won't
    get "erased" again, with undesirable results.

    `intflag' for the current ds is set by `check_int'.			*/

    if (intflag) {

      erasespec( spec2, 0, INT_COLOR );
      erase2 = 0;
    }
    xormode();
    show_plotterbox();
    normalmode();
    if (select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
       ))
         return(ERROR);
    ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
    setButtonMode(SELECT_MODE); 
    redisplay_flag = 1;

  }
  Wsetgraphicsdisplay(argv[0]);
  if(vsThumbWheel && get_drawVscale()) new_dscale(TRUE,FALSE);
  disp_specIndex(specIndex);
  this_is_addi = FALSE;
  this_is_inset = FALSE;
  if (check_int()) return(ERROR);
  if (init_vars2()) return(ERROR);
  show_plotterbox();
  specdisp();
  if (intflag)
    intdisp();
  if(vsThumbWheel && get_drawVscale()) new_dscale(FALSE,TRUE);
  else if(dscale_on() && ! vsThumbWheel) new_dscale(redisplay_flag,TRUE);

  if(threshflag > 0) b_thresh();

  if (showCursor()) b_cursor();
  else cr_line();

  if(!isInset() && getOverlayMode() == 0) {
	do_dpf_dpir();
  }

  grf_batch(0);
  releasevarlist();
  if (do_menu)  {
    execString("menu('ds_1')\n");
    }
  set_turnoff_routine(turnoff_ds);

/*  Save the current pixel size of the screen,
    so we can tell if it changes.		*/

  old_mnumxpnts = mnumxpnts;
  old_mnumypnts = mnumypnts;

#ifdef CLOCKTIME
  /* Turn off the clocktime timer */
  (void)stop_timer ( ds_timer_no );
#endif

#ifdef VNMRJ
  /* make sure Java graphics is updated */
  popBackingStore();

  // update CSI spectra if in CSI display mode (two windows).
  if(is_aip_window_opened() && dsChanged()) aipSpecViewUpdate();
  if ( ! this_is_addi)
     aspFrame("ds",0,0,0,0,0);
#endif

  if(vsflag) ds_sendVs();

  return(COMPLETE);
}

#ifdef OLD
/*****************/
static void b_shift()
/*****************/
{
  float *ptr1,*ptr2;
  double ratio;
  int    npt;

  Wgmode();
  npt = datapoint(cr+rflrfp,sw,fn/2);
  if (*curindex == spec1)
  {
    sp_spec1 = sp;
    ptr1 = spectrum + npt;
    ptr2 = spec2ptr + npt + datapoint(sw-sp_spec1+sp_spec2,sw,fn/2);
  }
  else
  {
    sp_spec2 = sp;
    ptr1 = spectrum + npt + datapoint(sw-sp_spec2+sp_spec1,sw,fn/2);
    ptr2 = spec2ptr + npt;
  }
  DPRINT2("b_shift: npt= %d, offset= %d\n",
               npt,datapoint(sw-sp_spec1+sp_spec2,sw,fn/2));
  if (ds_mode==BOX_MODE)
  {
    float datamax;
    float intval = 0.0;
  
    npt = (int)(fabs(delta) * (double) ((fn/2) - 1) / sw) + 1;
    maxfloat(ptr1,npt, &datamax);
    while (npt--)
      intval += *ptr1++;
    Winfoprintf("cr=%10.3f and %10.3f, max height=%10.3f, integral =%10.3f",
            cr,cr-delta,datamax * *cur_vs * *cur_scl,
            intval * insval );
  }
  else
  {
    Winfoprintf("cr=%10.3f, spec1=%10.3f, spec2=%10.3f",
                 cr,*ptr1 * vs * scale,*ptr2 * vs_spec2 * scl_spec2);
    if (addi_mode == SUB_MODE)
    {
      ratio = *ptr1 * vs * scale / (*ptr2 * vs_spec2 * scl_spec2);
      ratio = fabs(ratio);
      if (*curindex == spec1)
        *cur_vs /= ratio;
      else
        *cur_vs *= ratio;
    }
  }
  specdisp();
  show_addsub();
  ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
  b_cursor();
}
#endif

/***************/
static void b_select()
/***************/
{
  if (*curindex == spec1)
  {
    curcolor = color2;
    sp_spec1 = sp;
    set_spec(spec2ptr,&vp_spec2,&vs_spec2,&scl_spec2,sp_spec2,&spec2,&erase2);
    ParameterLine(2,COLUMN(FIELD5),INT_COLOR,"addsub ");
  }
  else if (*curindex == spec2)
  {
    curcolor = color3;
    sp_spec2 = sp;
    set_spec(spec3ptr,&vp_spec3,&vs_spec3,&scl_spec2,sp_spec2,&spec3,&erase3);
    ParameterLine(2,COLUMN(FIELD5),THRESH_COLOR,"result ");
  }
  else
  {
    curcolor = color1;
    set_spec(spectrum,&vp,&vs,&scale,sp_spec1,&spec1,&erase1);
    ParameterLine(2,COLUMN(FIELD5),SPEC_COLOR,"current");
  }
  ds_mode = -1;
  b_cursor();
}

/***************/
static void show_addsub()
/***************/
{ float *savspec,*savscl;
  int   *savindex,*saverase,savcolor;
  double *savvp,*savvs,savsp;

  savspec = curspec;
  savindex = curindex;
  saverase = curerase;
  savcolor = curcolor;
  savvp = cur_vp;
  savvs = cur_vs;
  savscl = cur_scl;
  savsp = sp;
  curcolor = color3;
  combinespec(spectrum,spec2ptr,spec3ptr);
  set_spec(spec3ptr,&vp_spec3,&vs_spec3,&scl_spec2,sp_spec2,&spec3,&erase3);
  specdisp();
  set_spec(savspec,savvp,savvs,savscl,savsp,savindex,saverase);
  curcolor = savcolor;
}

/***************/
static void b_addsub()
/***************/
{ 
  addi_mode = (addi_mode + 1) % 3;
  show_addsub();
  if (addi_mode == ADD_MODE)
    ParameterLine(2,COLUMN(FIELD6),PARAM_COLOR,"add");
  else if (addi_mode == SUB_MODE)
    ParameterLine(2,COLUMN(FIELD6),PARAM_COLOR,"sub");
  else if (addi_mode == MIN_MODE)
    ParameterLine(2,COLUMN(FIELD6),PARAM_COLOR,"min");

  ds_mode = (ds_mode == BOX_MODE) ? CURSOR_MODE : -1;
  b_cursor();
}

/**************************************/
static void combinespec(float *in1ptr, float *in2ptr, float *outptr)
/**************************************/
{
  register float *tmp;
  register int    i;
  register float  mult;
  int             fpnt_spec2;

  DPRINT3("combine: sp= %g, spec1= %g, spec2= %g\n",sp,sp_spec1,sp_spec2);
  fpnt_spec2 = datapoint(sp_spec2 + rflrfp + wp,sw,fn/2);
  tmp     = outptr + fpnt_spec2;
  in1ptr += datapoint(sp_spec1 + rflrfp + wp,sw,fn/2);
  in2ptr += fpnt_spec2;
  DPRINT4("fpt_spec1= %d, fpt_spec2= %d, index= %d, fpnt= %d\n",
       datapoint(sp_spec1+rflrfp+wp,sw,fn/2),fpnt_spec2,
       (*curindex!=spec1)+1,fpnt);
  mult = (vs * scale) / (SPEC3_VS * scl_spec2);
  if (addi_mode == MIN_MODE)
  { register float mult2,temp1,temp2;

    mult2 = vs_spec2 / SPEC3_VS;
    for (i=0; i<npnt; i++)
    {
      temp1 = mult  * *in1ptr++;
      temp2 = mult2 * *in2ptr++;
      *tmp++ = (fabs((double) temp1) < fabs((double) temp2)) ? temp1 : temp2;
    }
  }
  else
  {
    if (addi_mode == SUB_MODE)
      mult *= - 1.0;
    for (i=0; i<npnt; i++)
      *tmp++ = mult * (*in1ptr++);
    mult = vs_spec2 / SPEC3_VS;
    tmp = outptr + fpnt_spec2;
    for (i=0; i<npnt; i++)
      *tmp++ += mult * (*in2ptr++);
  }
}

/*********************************************************/
static void set_spec(float *ptr, double *c_vp, double *c_vs, float *c_scl,
                     double c_sp, int *c_inx, int *c_erase)
/*********************************************************/
{
  curspec = ptr;
  cur_vp = c_vp;
  cur_vs = c_vs;
  cur_scl = c_scl;
  sp = c_sp;
  fpnt = datapoint(sp+rflrfp+wp,sw,fn/2);
  curindex = c_inx;
  curerase = c_erase;
}

/****************/
static void addi_disp(int first, int second, int third)
/****************/
{
  float *savspec,*savscl;
  int   *savindex,*saverase,savcolor;
  double *savvp,*savvs,savsp;

  savvp = cur_vp;
  savvs = cur_vs;
  savsp = sp;
  savspec = curspec;
  savindex = curindex;
  saverase = curerase;
  savcolor = curcolor;
  savscl   = cur_scl;
  if (first)
  {
    curcolor = color1;
    set_spec(spectrum,&vp,&vs,&scale,sp_spec1,&spec1,&erase1);
    specdisp();
  }
  if (second)
  {
    curcolor = color2;
    set_spec(spec2ptr,&vp_spec2,&vs_spec2,&scl_spec2,sp_spec2,&spec2,&erase2);
    specdisp();
  }
  if (third)
  {
    curcolor = color3;
    set_spec(spec3ptr,&vp_spec3,&vs_spec3,&scl_spec2,sp_spec2,&spec3,&erase3);
    specdisp();
  }
  curcolor = savcolor;
  set_spec(savspec,savvp,savvs,savscl,savsp,savindex,saverase);
  if (first && second && third)
    b_cursor();
}

/***********************/
static double addsub_sp()
/***********************/
{
  double sw_spec2,ref,pos,sp_spec2,hzpp;
  int    pt;

  if (P_getreal(TEMPORARY,"sw",&sw_spec2,1))
    Werrprintf("error getting sw from addsub exp");
  if (P_getreal(TEMPORARY,"sp",&sp_spec2,1))
    Werrprintf("error getting sp from addsub exp");
  if (P_getreal(TEMPORARY,"rfl",&ref,1))
    Werrprintf("error getting rfl from addsub exp");
  if (P_getreal(TEMPORARY,"rfp",&pos,1))
    Werrprintf("error getting rfp from addsub exp");
  pt = datapoint(sp_spec2 + ref - pos + wp,sw_spec2,fn/2);
  hzpp = sw / (double) ((fn / 2) );
  sp_spec2 = sw - wp - rflrfp - (double) pt * hzpp; 
  if (sp_spec2 + rflrfp < 0)
    sp_spec2 = -rflrfp;
  else if (sp_spec2 + wp + rflrfp > sw)
    sp_spec2 = sw - wp - rflrfp;
  return(sp_spec2); 
}

/*****************/
int start_addi(float *ptr2, float *ptr3, double scl, char *path)
/*****************/
/* spec2ptr points to the addsub phase file */
/* spec3ptr points to allocated space which will hold sum/difference spectra */
{
  set_turnoff_routine(turnoff_ds);
  strcpy(addsubpath,path);
  addi_mode = ADD_MODE;
  spec2ptr = ptr2;
  spec3ptr = ptr3;
  this_is_addi = TRUE;
  this_is_inset = FALSE;
  init_vars1();
  if (init_vars2()) return(ERROR);
  if (dscale_on())
    new_dscale(FALSE,TRUE);
  intflag = FALSE;
  color2 = INT_COLOR;
  color3 = THRESH_COLOR;
  spec3 = 3;
  erase3 = 0;
  scl_spec2 = scl * dispcalib;
  if (P_getreal(TEMPORARY,"vs",&vs_spec2,1))
    Werrprintf("error getting vs from addsub exp");
  if (P_getreal(TEMPORARY,"vp",&vp_spec2,1))
    Werrprintf("error getting vp from addsub exp");
  sp_spec2 = addsub_sp();
  sp_spec1 = sp;
  if (vp == vp_spec2)
    vp_spec2 += wc2max / 4.0;
  vp_spec3 = vp_spec2 + wc2max / 4.0;
  vs_spec3 = SPEC3_VS;
  exp_factors(TRUE);
  combinespec(spectrum,spec2ptr,spec3ptr);
  addi_disp(TRUE,TRUE,TRUE);
  ParameterLine(1,COLUMN(FIELD5),PARAM_COLOR,"active");
  ParameterLine(2,COLUMN(FIELD5),color1,"current");
  ParameterLine(1,COLUMN(FIELD6),PARAM_COLOR,"mode");
  ParameterLine(2,COLUMN(FIELD6),PARAM_COLOR,"add");
  return(COMPLETE);
}

/*****************/
static void b_save()
/*****************/
{
  int shift;

  if (*curindex == spec1)
    sp_spec1 = sp;
  else
    sp_spec2 = sp;
  shift = datapoint(sp_spec2 + rflrfp + wp,sw,fn/2) -
          datapoint(sp_spec1 + rflrfp + wp,sw,fn/2);
  if (addi_mode == SUB_MODE)
    scale = -scale;
  if ( save_data(addsubpath, spec2ptr, vs_spec2/SPEC3_VS, spectrum,
		    vs*scale/(SPEC3_VS*scl_spec2), spec3ptr, shift,
		    addi_mode) )
  {
     Werrprintf("error saving data in %s",addsubpath);
  }

  vs_spec2 = vs_spec3 * dispcalib / scl_spec2;
  b_return();
  D_close(D_DATAFILE);
  Wsetgraphicsdisplay("addi");
}

/*****************/
static void addi_buttons()
/*****************/
{ char *switch1, *switch2, *switch3 = "";
  char *labelptr[9];
  PFV   cmd[9];

#ifdef VNMRJ
  char *switch4;
  if (ds_mode==BOX_MODE)
    { switch1 = "3 1D1cur.gif Cursor";
      switch2 = "3 1Dexpand.gif Expand";
    }
  else
    { switch1 = "3 1D2cur.gif Box";
      switch2 = "3 1Dfull.gif Full";
    }
  if (addi_mode == ADD_MODE)
    switch3 = "3 addimodesub.gif Sub";
  else if (addi_mode == SUB_MODE)
    switch3 = "3 addimodemin.gif Min";
  else if (addi_mode == MIN_MODE)
    switch3 = "3 addimodeadd.gif Add";

  if (*curindex == spec1)
    switch4 = "3 addiselectg.gif Select";
  else if (*curindex == spec2)
    switch4 = "3 addiselecty.gif Select";
  else
    switch4 = "3 addiselectb.gif Select";
  labelptr[0] = switch1;		cmd[0] = b_cursor;
  labelptr[1] = switch4;		cmd[1] = b_select;
  labelptr[2] = switch2;		cmd[2] = b_expand;
  labelptr[3] = "3 1Dspwp.gif sp wp";	cmd[3] = b_spwp;
  labelptr[4] = "3 1Dscale.gif Show/Hide Axis";	cmd[4] = addi_dscale;
  labelptr[5] = switch3;		cmd[5] = b_addsub;
  labelptr[6] = "3 addisave.gif Save";	cmd[6] = b_save;
  labelptr[7] = "3 return.gif Return";	cmd[7] = b_return;
  Wactivate_buttons(8, labelptr, cmd, turnoff_ds, "addi");

#else

  if (ds_mode==BOX_MODE)
    { switch1 = "Cursor";
      switch2 = "Expand";
    }
  else
    { switch1 = " Box  ";
      switch2 = " Full ";
    }
  if (addi_mode == ADD_MODE)
    switch3 = "Sub";
  else if (addi_mode == SUB_MODE)
    switch3 = "Min";
  else if (addi_mode == MIN_MODE)
    switch3 = "Add";
  labelptr[0] = switch1;	cmd[0] = b_cursor;
  labelptr[1] = "Select";	cmd[1] = b_select;
  labelptr[2] = switch2;	cmd[2] = b_expand;
  labelptr[3] = "sp wp";	cmd[3] = b_spwp;
  labelptr[4] = switch3;	cmd[4] = b_addsub;
  labelptr[5] = "Save";		cmd[5] = b_save;
  labelptr[6] = "Return";	cmd[6] = b_return;
  Wactivate_buttons(7, labelptr, cmd, turnoff_ds, "addi");
#endif
  Wgmode();
}

/***************************************/
/*  Subroutines for the inset command  */
/***************************************/

static int ok_to_inset()
{
	if (WgraphicsdisplayValid( "ds" ) == 0) {
		Werrprintf( "First start ds before entering inset" );
		return( -1 );
	}

	if (ds_mode != BOX_MODE) {
		Werrprintf( "inset requires display of two cursors" );
		return( -1 );
	}

	return( 0 );
}

static void modify_pars_for_inset()
{
	int	rev;
	double	crval, deltaval, scaleval, spval, wpval, vpval;

	get_scale_pars( HORIZ, &spval, &wpval, &scaleval, &rev );
	get_cursor_pars( HORIZ, &crval, &deltaval );
	P_getreal(CURRENT,"vp",&vpval,1);

	sc += wc * (crval - deltaval - spval) / wpval;
	wc *= deltaval/wpval;

	P_setreal( CURRENT, "vp", vpval + 50.0, 0);
	P_setreal( CURRENT, "sc", sc, 0);
	P_setreal( CURRENT, "wc", wc, 0);

	spval = crval - deltaval;
	wpval = deltaval;

	UpdateVal( HORIZ, SP_NAME, spval, NOSHOW );
	UpdateVal( HORIZ, WP_NAME, wpval, NOSHOW );
}

static int init_inset()
{
	int	rev;
	double	scaleval, spval, wpval;

	if (select_init(
		0,
		GRAPHICS,
		NO_FREQ_DIM,
		NO_HEADERS,
		DO_CHECK2D,
		DO_SPECPARS,
		NO_BLOCKPARS,
		NO_PHASEFILE
	  ))
	    return( -1 );

/*  Since select_init can change sp and/or wp so the values fall
    on boundaries which the digital resolution (the Fourier number)
    defines, adjust cursor and delta values to match sp / wp.	*/

	get_scale_pars( HORIZ, &spval, &wpval, &scaleval, &rev );
	UpdateVal( HORIZ, DELTA_NAME, wpval, NOSHOW );
	UpdateVal( HORIZ, CR_NAME, spval+wpval, NOSHOW );

	erase1 = 0;				/* do not erase current spectrum */
	erase2 = 0;				/* do not erase current integral */
	this_is_addi = FALSE;
	this_is_inset = TRUE;

	return( 0 );
}

/*  Note:  The chart and plot parameters increase as the location moves to
	   the left, opposite of the usual convention for the number line.
	   The display first point and display number of points increase as
	   ther location moves to the right.				*/

static void m_scwc(int butnum, int x, int y, int moveflag )
{
	int	max_xpnts, new_xcursor[ 2 ];
	double	dmax_xpnts;

        (void) moveflag;
	if (get_dis_setup() != 1)
	  select_init(
		0,
		GRAPHICS,
		NO_FREQ_DIM,
		NO_HEADERS,
		DO_CHECK2D,
		DO_SPECPARS,
		NO_BLOCKPARS,
		NO_PHASEFILE
	  );
	if (butnum == 2) {
		ds_multiply( x, y );
	}
	else if (butnum == 1 || butnum == 3) {
		max_xpnts = mnumxpnts - right_edge;
		dmax_xpnts = (double) max_xpnts;
		if (butnum == 1) {
			if (x + dnpnt > max_xpnts)
			  x = max_xpnts - dnpnt;
			sc = (double) (wcmax * (max_xpnts - x - dnpnt)) / dmax_xpnts;
			sc = (double) ( (int) (sc * 10.0) / 10);
		}
		else if (butnum == 3) {
			double	oldwc;

		   /*  Keep right edge of the spectrum to the left of the
		       right side of the plotter box.  Keep right edge to
		       the right of the left edge of the spectrum.	*/

			if (x > max_xpnts)	
			  x = max_xpnts;
			if (x <= dfpnt)
			  x = dfpnt+1;

                        oldwc = wc;
			wc = (double) (wcmax * (x - dfpnt)) / dmax_xpnts;
			wc = (double) ( (int) (wc * 10.0) / 10);

		   /*  New start of chart is the old start of chart less
		       the difference between the new width of chart and
		       the old width of chart.  If the width of chart
		       increases, the new start of chart decreases. This
		       keeps the left edge of the chart constant.  Avoid
		       roundoff errors by calculating the new start of
		       chart using only chart parameters, without using
		       any pixel values.				*/

                        sc = sc - (wc - oldwc);
			sc = (double) ( (int) (sc * 10.0) / 10);

		   /*  Only the right button changes width of chart.  */
	
			P_setreal( CURRENT, "wc", wc, 0 );
			DispField2(FIELD4, -PARAM_COLOR, wc, 1 );
		}

	   /*  Both left and right buttons change start of chart  */

		P_setreal( CURRENT, "sc", sc, 0);
		DispField2(FIELD3, -PARAM_COLOR, sc, 1 );

	   /*  Recalculate numbers relating to display of spectrum  */

		exp_factors( TRUE );
		specdisp();
		if (intflag)
		  intdisp();

	   /*  Reposition cursors.  Multiply cursor fraction of plot
	       (and cursor - delta fraction for 2nd cursor) by display
	       number of points before converting value to integer.	*/

		new_xcursor[ 0 ] = (int)((sp + wp - cr)/wp * (double) dnpnt) + dfpnt;
		update_xcursor( 0, new_xcursor[ 0 ] );
		if (ds_mode == BOX_MODE) {
			new_xcursor[ 1 ] = dfpnt +
				(int)((sp + wp - cr + delta)/wp * (double) dnpnt);
			update_xcursor( 1, new_xcursor[ 1 ] );
		}

	   /*  Display a new scale if needed, erasing the old scale.  */

		if (dscale_on())
		  new_dscale(TRUE,TRUE);
	}
}

static void inset_labels()
{
	DispField1(FIELD1,  PARAM_COLOR, (intflag) ? "io" : "vp");
	DispField2(FIELD1, -PARAM_COLOR, (intflag) ? io : *cur_vp, 1);
	DispField1(FIELD3,  PARAM_COLOR, "sc" );
	DispField2(FIELD3, -PARAM_COLOR, sc, 1 );
	DispField1(FIELD2,  PARAM_COLOR, (intflag) ? "is" : "vs");
	DispField2(FIELD2, -PARAM_COLOR, (intflag) ? is : *cur_vs, 1);
	DispField1(FIELD4,  PARAM_COLOR, "wc" );
	DispField2(FIELD4, -PARAM_COLOR, wc, 1 );
}

/*static void b_plot()
{
	execString( "pl\n" );
	if (dscale_on())
	  execString( "pscale\n" );
}*/

/*static b_ds()
{
	this_is_inset = FALSE;
	b_cursor();
}*/

static void b_scwc()
{
	if (get_dis_setup() != 1)
	  select_init(
		0,
		GRAPHICS,
		NO_FREQ_DIM,
		NO_HEADERS,
		DO_CHECK2D,
		DO_SPECPARS,
		NO_BLOCKPARS,
		NO_PHASEFILE
	  );
	inset_labels();
	activate_mouse( m_scwc, ds_reset );
        dsstatus("sc wc");
        setButtonMode(SCWC_MODE);
}

int inset(int argc, char *argv[], int retc, char *retv[])
{
        (void) argc;
        (void) argv;
        (void) retc;
        (void) retv;
	if (ok_to_inset() != 0)
	  ABORT;
      if(argc>1 && strcmp(argv[1],"frame")==0) {

        int x0, y0, x1, y1;
        if (interfero) {
            x0  = dfpnt  + dnpnt  * (cr  - sp) / wp;
        } else if (revAxis) {
            x0  = dfpnt  + dnpnt - dnpnt  * (sp  - cr) / wp;
        } else {
            x0  = dfpnt  + dnpnt  * (wp  - cr  + sp ) / wp;
        }
        x1 = x0 + dnpnt  * delta  / wp;
        y0 = 0;
        y1 = mnumypnts/2;

        finishInsetFrame(x0, y0, x1, y1);

      } else {

	Wturnoff_buttons();
 	execString("menu('inset')\n");
        set_turnoff_routine(turnoff_ds);
	modify_pars_for_inset();
	if (init_inset())
	  ABORT;
	specdisp();
	if (intflag)
	  intdisp();
	if (dscale_on())
	  new_dscale( FALSE, TRUE );
	b_scwc();
	this_is_inset=TRUE;
      }
	RETURN;
}

#ifdef VNMRJ

void getAxis(char * axis, int n)
{
   get_nuc_name(HORIZ, axis, n);
}

void ds_sendCursor(double c2, double d2, int mode)
{
   int i, vps;
   double d;
   char cmd[MAXSTR], axis[8];

    if (P_getreal(GLOBAL,"trackCursor", &d, 1) != 0) d = 0.0;
    if(d < 0.5) return;

   if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
   else vps = (int)d;
   if(vps < 2) return;

   if (interfero)
      d = c2 - sp;
   else if (revAxis)
      d  = sp - c2;
   else
      d = wp + sp - c2;
   if(d < 0.5 || d > wp) return;  

   getAxis(axis, 7);

   c2 = Hz2ppm(HORIZ, c2);
   d2 = Hz2ppm(HORIZ, d2);

   for(i=0; i<vps; i++) {
        if(i+1 != VnmrJViewId) {
          if (P_getreal(GLOBAL,"jvpexps", &d, i+1) != 0) d = 0.0;
          if(d < 0.5) continue;

          sprintf(cmd, "vnmrjcmd('CR %d trackCursor(\\'cursor\\', %f, %f, %d, \\'%s\\')')\n", i+1, c2, d2, mode, axis);
          execString(cmd);

          //writelineToVnmrJ("CR ", cmd);
        }
   }
}

void ds_newCursor4freq(int num, double c1, double c2, 
            double d1, double d2, int mode, char *tr, char *ax1, char *ax2)
{
/* if cursor is from a 1d spectrum, only c1, d1 are passed */

      int x, x1, save;
      double d, f2;
      char axis[8];
/* return if cursorTracking is off */
    (void) tr;
    if (P_getreal(GLOBAL,"trackCursor", &d, 1) != 0) d = 0.0;
    if(d < 0.5) return;

/* if cursor come from 1 2d spectrum, mode could be TRACE_MODE */
/* it will become CURSOR_MODE for 1d*/

    if(mode == TRACE_MODE) mode = CURSOR_MODE;

    getAxis(axis, 7);

    if(mapAxis(num, &c1, &c2, &d1, &d2, axis, ax1, ax2) == 0) return;
    c2 = ppm2Hz(HORIZ, c2);
    d2 = ppm2Hz(HORIZ, d2);

    if (interfero)
    {
      f2 = c2 - sp;
      x  = dfpnt  + dnpnt  * f2 / wp;
    }
    else if (revAxis)
    {
      f2 = sp - c2;
      x  = dfpnt + dnpnt - dnpnt  * f2 / wp;
    }
    else
    {
      f2 = wp + sp - c2;
      x  = dfpnt  + dnpnt  * f2 / wp;
    }
    if(f2 < 0.5 || f2 > wp) return;

    ds_mode = mode;
    cr = c2;
    delta = d2;

    x1 = x + dnpnt  * delta  / wp;

  idelta = save  = x1 - x;
  m_newcursor(1,x,0,0);
  if (ds_mode==BOX_MODE)
  {
    idelta  = save;
    m_newcursor(3,x1,0,0);
    //dsstatus("BOX");
  }
  else
  { update_xcursor(1,0);  /* erase the second cursor */
    //dsstatus("CURSOR");
  }
}

/*************************************/
static void update_crosshair(int x, double f2)
/*************************************/
{
/* new functions in graphicas_win to draw crosshair */

    if(!get_drawVscale())
       x_crosshair(x, 0, mnumypnts, f2, 0, 0);
    else { // determine magnitude yval.
       double hzpp = sw/((double)(fn/2));
       double freq = convert2Hz(HORIZ, f2);
       int off = (int)((sw-freq-rflrfp)/hzpp + 0.01);
       float *yval = curspec+off;
       x_crosshair(x, 0, mnumypnts, f2, 1, yval[0]);
    }
}

void ds_newCrosshair(int x, int y)
{
/* y is not used */
    double c2, freq;

      if (interfero)
      {
         c2  =   (x - dfpnt ) * wp  / dnpnt  + sp;
         freq = c2 - sp;
      }
      else if (revAxis)
      {
         c2  =   (x - dfpnt - dnpnt ) * wp  / dnpnt  + sp;
         freq = sp - c2;
      }
      else
      {
         c2  = - (x - dfpnt ) * wp  / dnpnt  + wp  + sp;
         freq = wp + sp - c2;
      }

   if(freq < 0.5 || freq > wp || y < 0.5 || y > mnumypnts) {
  	m_noCrosshair();
	return;
   }

        freq = Hz2ppm(HORIZ, c2);

       update_crosshair(x, convert4ppm(HORIZ, freq));

      ds_sendCrosshair(c2, freq);
}

void ds_sendCrosshair(double c2, double f2)
{
/* if is active vp, call trackCursor('crosshair',c1, c2) of other vps */

   int i, vps;
   double d;
   char cmd[MAXSTR], axis[8];

    if (P_getreal(GLOBAL,"trackCursor", &d, 1) != 0) d = 0.0;
    if(d < 0.5) return;

    if (P_getreal(GLOBAL,"crosshair", &d, 1) != 0) d = 0.0;
    if(d < 0.5) return;

    if (P_getreal(GLOBAL,"overlayMode", &d, 1) != 0) d = 0.0;
    if((int)d >= OVERLAID_ALIGNED) return;

   if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
   else vps = (int)d;
   if(vps < 2) return;

   if (interfero)
      d = c2 - sp;
   else if (revAxis)
      d  = sp - c2;
   else
      d = wp + sp - c2;
   if(d < 0.5 || d > wp) return;  

   getAxis(axis, 7);

   c2 = Hz2ppm(HORIZ, c2);

   for(i=0; i<vps; i++) {
	
        if(i+1 != VnmrJViewId) {
          if (P_getreal(GLOBAL,"jvpexps", &d, i+1) != 0) d = 0.0;
          if(d < 0.5) continue;

          sprintf(cmd, "vnmrjcmd('CR %d trackCursor(\\'crosshair\\', %f, %f, \\'%s\\')')\n", i+1, c2, f2, axis);
          execString(cmd);

          //writelineToVnmrJ("CR ", cmd);
        }
   }
}

void ds_newCrosshair4freq(int num, double c1, double c2, 
double f1, double f2, char *tr, char *ax1, char *ax2)
{
/* if cursor is from a 1d spectrum, only c1, d1 are passed */

      int x;
      double d, x1;
      char axis[8];

    (void) tr;
/* return if cursorTracking is off */
    if (P_getreal(GLOBAL,"crosshair", &d, 1) != 0) d = 0.0;
    if(d < 0.5) return;

    getAxis(axis, 7);

    if(mapAxis(num, &c1, &c2, &f1, &f2, axis, ax1, ax2) == 0) return;

    c2 = ppm2Hz(HORIZ, c2);

    if (interfero)
    {
      x1 = c2 - sp;
      x  = dfpnt  + dnpnt  * x1 / wp;
    }
    else if (revAxis)
    {
      x1 = sp - c2;
      x  = dfpnt + dnpnt - dnpnt  * x1 / wp;
    }
    else
    {
      x1 = wp + sp - c2;
      x  = dfpnt  + dnpnt  * x1 / wp;
    }
    if(x1 < 0.5 || x1 > wp) {
       ds_noCrosshair();
       return;
    }

    update_crosshair(x, convert4ppm(HORIZ, f2));
}

void ds_noCrosshair() {
   update_crosshair(0, 0);
}

void ds_inset(float c, float d)
{
/*
	if (ok_to_inset() != 0)
	  ABORT;
*/
  if (get_dis_setup() != 1)
    select_init(
        0,
        GRAPHICS,
        NO_FREQ_DIM,
        NO_HEADERS,
        DO_CHECK2D,
        DO_SPECPARS,
        NO_BLOCKPARS,
        NO_PHASEFILE
    );
//  ds_reset();

  cr = c;
  delta = d;
    dsstatus("EXPAND");
    /* set sp,wp according to expansion box */
    if (interfero)
       sp  = cr;
    else if (revAxis)
       sp  = cr + delta;
    else
       sp  = cr - delta;
    wp  = delta;
    if (revAxis)
       set_sp_wp_rev(&sp,&wp,sw,fn/2,rflrfp);
    else
       set_sp_wp(&sp,&wp,sw,fn/2,rflrfp);
    delta = wp;
    if (interfero)
       cr = sp;
    else if (revAxis)
       cr = sp - delta;
    else
       cr = sp + delta;
  UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
  UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
    UpdateVal(HORIZ,CR_NAME,cr,NOSHOW);
    UpdateVal(HORIZ,DELTA_NAME,delta,NOSHOW);
    ds_mode = BOX_MODE; /* force b_cursor() into the CURSOR_MODE */
  b_cursor();

	inset_labels();
        dsstatus("sc wc");

	Wturnoff_buttons();
 	execString("menu('inset')\n");
        set_turnoff_routine(turnoff_ds);
	modify_pars_for_inset();
	if (init_inset())
	  return;
	specdisp();
	if (intflag)
	  intdisp();
	if (dscale_on())
	  new_dscale( FALSE, TRUE );
	b_scwc();
}

void ds_setCursor(int x0, int x1)
{
  if (interfero) {
         cr = + (x0 - dfpnt ) * wp  / dnpnt  + sp;
         delta = + (x1 - dfpnt ) * wp  / dnpnt  + sp;
         delta = cr - delta;
  } else if (revAxis) {
         cr = (x0 - dfpnt - dnpnt) * wp  / dnpnt  + sp;
         delta = (x1 - dfpnt - dnpnt) * wp  / dnpnt  + sp;
         delta = delta - cr;
  } else {
         cr = - (x0 - dfpnt ) * wp  / dnpnt  + wp + sp;
         delta = - (x1 - dfpnt ) * wp  / dnpnt  + wp  + sp;
         delta = cr - delta;
  }

  return;
}

void ds_zoom(int mode)
{
    ds_mode = mode;

#ifdef VNMRJ
    if (isXorOn)
         vj_xoroff();
    isXorOn = 0;
#endif

    b_expand();

    if(!showCursor()) {
       ds_mode=CURSOR_MODE; 
       b_cursor();
    }

  return;
}

int showCursor() {
    double d;
    if (!P_getreal(GLOBAL,"aspMode", &d, 1) && d>0 ) return 0;

    int mode = getButtonMode();

    if (mode < 2 || mode == 7 || mode > 8) return 1;
    else return 0;
}

void ds_spwp(int but, int x, int y, int mflag)
{
   m_spwp(but, x, y, mflag);
    ds_sendSpwp(sp, wp, but);

    if ( get_axis_freq(HORIZ) )
        cr  = sp + delta;
    else
        cr = sp;
    delta  = fabs(wp);

   //if(!showCursor()) {
      ds_mode=CURSOR_MODE; 
      b_cursor();
  // }
  if(graph_flag) execString("ds('again')\n");
  else if(!isInset()) redo_dpf_dpir();
}

void ds_sendSpwp(double c2, double d2, int but)
{
   int i, vps;
   double d;
   char cmd[MAXSTR], axis[8];

    if(getFrameID() > 1) return;

    if (P_getreal(GLOBAL,"trackAxis", &d, 1) != 0) d = 0.0;
    if(d < 0.5 && getOverlayMode() < NOTOVERLAID_ALIGNED) return;

   if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
   else vps = (int)d;
   if(vps < 2) return;

   getAxis(axis, 7);

   c2 = Hz2ppm(HORIZ, c2);
   d2 = Hz2ppm(HORIZ, d2);

   for(i=0; i<vps; i++) {
        if(i+1 != VnmrJViewId) {
          if (P_getreal(GLOBAL,"jvpexps", &d, i+1) != 0) d = 0.0;
          if(d < 0.5) continue;
          sprintf(cmd, "vnmrjcmd('CH %d trackCursor(\\'spwp\\', %f, %f, %d, \\'%s\\')')\n", i+1, c2, d2, but, axis);
//fprintf(stderr,"updateOtherViewport %s\n", cmd);
          execString(cmd);

          //writelineToVnmrJ("CR ", cmd);
        }
   }
}

void ds_newSpwp(int num, double c1, double c2, 
double d1, double d2, int but, char *tr, char *ax1, char *ax2)
{
/* if cursor is from a 1d spectrum, only c1, d1 are passed */

    double d;
    char axis[8];

    (void) but;
    (void) tr;
    if (P_getreal(GLOBAL,"trackAxis", &d, 1) != 0) d = 0.0;
    if(d < 0.5 && getOverlayMode() < NOTOVERLAID_ALIGNED) return;

    getAxis(axis, 7);

    if(mapAxis(num, &c1, &c2, &d1, &d2, axis, ax1, ax2) == 0) return;

    c2 = ppm2Hz(HORIZ, c2);
    d2 = ppm2Hz(HORIZ, d2);

    sp = c2;
    wp = d2;
    
    ds_checkSpwp(&sp, &wp, VnmrJViewId);

    P_setreal(CURRENT, "sp", sp, 1);
    P_setreal(CURRENT, "wp", wp, 1);

    execString("ds('again')\n");

    return;
}

int mapAxis(int num, double *c1, double *c2,
double *f1, double *f2, char *axis, char *ax1, char *ax2)
{
/* we know whether the cursor comes from a 1d or 2d by num */
/* num = 2 is 1d, num = 4 is 2d */
/* now figure out which aixs it maps to */
    if(num < 2) {
        /* from a 1d spectrum */
        if(strcasecmp(axis, ax2) == 0) {
            return 1;
        }
        return 0;

    } else {
        /* from a 2d spectrum */
	if(strcasecmp(axis, ax1) == 0 && strcasecmp(axis, ax2) == 0 
	&& getOverlayMode() == OVERLAID_ALIGNED && getChartMode() == ALIGN_1D_Y) {
           *c2 = *c1;
           *f2 = *f1;
           return 1;
	} else if(strcasecmp(axis, ax1) == 0 && strcasecmp(axis, ax2) == 0) {
           return 1;
	} else if(strcasecmp(axis, ax1) == 0) {
           *c2 = *c1;
           *f2 = *f1;
           return 1;
	} else if(strcasecmp(axis, ax2) == 0) {
           return 1;
	} 
        return 0;
    }
}

void ds_zoomCenterPeak(int x, int y, int but)
{
    double freq, s2, w2, d;
    double d1, d2, c1, c2;

    (void) y;
    if (interfero) {
           freq = sp + (x - dfpnt) * wp / dnpnt;
    } else if (revAxis) {
           freq = sp + (x - dfpnt - dnpnt) * wp / dnpnt;
    } else {
           freq = sp + wp - (x - dfpnt) * wp / dnpnt;
    }

    w2 = wp;
    s2 = freq - wp/2;
    if(s2 < -rflrfp) {
	w2 = 2*(freq + rflrfp); 
    } 
    d = freq + wp/2;
    if(d > (sw-rflrfp)) {
	d = 2*(sw - freq - rflrfp); 
	if(d < w2) w2 = d;
    }

    sp = freq - w2/2; 

    if(but == 3) {
	ds_prevZoomin(&c2, &c1, &d2, &d1);
    } else if(w2 == wp) {
      	ds_nextZoomin(&c2, &c1, &d2, &d1);
    } else {
	wp = w2; 
	c2 = freq + w2/2;
	d2 = w2;
    }

   cr=c2;
   delta=d2;
   ds_zoom(BOX_MODE);
}

void ds_centerPeak(int x, int y, int but)
{
    double freq, s2, d2;

    if (interfero) {
           freq = sp + (x - dfpnt) * wp / dnpnt;
    } else if (revAxis) {
           freq = sp + (x - dfpnt - dnpnt) * wp / dnpnt;
    } else {
           freq = sp + wp - (x - dfpnt) * wp / dnpnt;
    }

    s2 = freq - wp/2;
    d2 = freq + wp/2;
    if(s2 < -rflrfp || d2 > (sw-rflrfp)) {
	ds_zoomCenterPeak(x, y, but);
	return;
    }

    sp=s2;
    P_setreal(CURRENT, "sp", sp, 1);
    ds_checkSpwp(&sp, &wp, VnmrJViewId);

    execString("ds('again')\n");

    if (interfero) {
        cr = sp;
    } else if (revAxis) {
        cr  = sp - delta;
    } else {
        cr  = sp + delta;
    }
    delta  = fabs(wp);

    ds_sendSpwp(sp, wp, 1);
}

void ds_nextZoomin(double *c2, double *c1, double *d2, double *d1)
{
     double d, slope;
     *c2 = sp; 
     *d2 = wp; 

   if(P_getreal(GLOBAL, "zoomSlope", &slope, 1)) slope = 0.2;

     d = slope*(*d2);
     if( !interfero)
     {
        if ( revAxis)
        {
           *c2 -= *d2;
           *c2 += 0.5*d;
        }
        else
        {
           *c2 += *d2;
           *c2 -= 0.5*d;
        }
     }
     *d2 -= d;

     if(c1!=NULL) *c1 = 0;
     if(d1!=NULL) *d1 = 0;
}

void ds_prevZoomin(double *c2, double *c1, double *d2, double *d1)
{
     double d, slope;
     *c2 = sp; 
     *d2 = wp; 

   if(P_getreal(GLOBAL, "zoomSlope", &slope, 1)) slope = 0.2;

     d = slope*(*d2);
     if( !interfero)
     {
        if ( revAxis)
        {
           *c2 -= *d2;
           *c2 -= 0.5*d;
        }
        else
        {
           *c2 += *d2;
           *c2 += 0.5*d;
        }
     }
     *d2 += d;

     if(c1!=NULL) *c1 = 0;
     if(d1!=NULL) *d1 = 0;
}

void ds_sendSpecInfo(int frame) {

/* cannot use init_display get_ functions because this function 
may be called by P_read. 
*/
    char axis2[8], line[MAXSTR];
    double l, p, s2max, w2max, s2, w2;

    if(frame != 1) return;

    if(P_getreal(CURRENT, "sw", &w2max, 1)) w2max = sw;
    if(P_getreal(CURRENT, "rfl", &l, 1)) l = 0;
    if(P_getreal(CURRENT, "rfp", &p, 1)) p = 0;
    s2max = -(l-p);
    if(P_getreal(CURRENT, "sp", &s2, 1)) s2 = sp;
    if(P_getreal(CURRENT, "wp", &w2, 1)) w2 = wp;
    getAxis(axis2, 7);

    s2max = Hz2ppm(HORIZ, s2max);
    w2max = Hz2ppm(HORIZ, w2max);
    s2 = Hz2ppm(HORIZ, s2);
    w2 = Hz2ppm(HORIZ, w2);
/*
Winfoprintf("dataInfo1 %d %d %s %s %f %f %f %f\n", VnmrJViewId, frame, "ds", axis2, s2max, w2max, s2, w2);
*/
    sprintf(line, "dataInfo %d %d %s %s %f %f %f %f\n", VnmrJViewId, frame, "ds", axis2, s2max, w2max, s2, w2);

    writelineToVnmrJ("vnmrjcmd", line);
}

void ds_overlaySpec(int mode)
{
    if(!(mode < 2 && s_overlayMode < 2)) {

      ds_checkSpwp(&sp, &wp, VnmrJViewId);
      UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
      UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
      execString("ds('again')\n");
      ds_sendSpwp(sp, wp, 1);
    }

    s_overlayMode = mode;
}

void ds_checkSpwp(double *s2, double *w2, int id)
{
/* s2, w2 are in Hz */
    double d, sx, wx, ex, sy, wy, e2;
    int i, mode, tmpMode;
    char ax1[8], ax2[8];
    char axis[8];
    mode = getOverlayMode();

    tmpMode = mode;
    if (P_getreal(GLOBAL,"trackAxis", &d, 1) != 0) d = 0.0;
    if(d < 0.5 && getOverlayMode() < NOTOVERLAID_ALIGNED) return;

    if(mode != UNSTACKED && mode != STACKED && s_overlayMode == STACKED) {
 	mode = UNSTACKED;	
    } else if(mode < NOTOVERLAID_ALIGNED && s_overlayMode == 3) { 
 	mode = NOTOVERLAID_ALIGNED;	
    } 
    s_overlayMode = tmpMode;

    if(mode < NOTOVERLAID_ALIGNED) return;

    getSweepInfo(&sx, &wx, &sy, &wy, ax2,ax1);

    getAxis(axis, 7);
    if(strcmp(axis,ax1) == 0) {
       sx = ppm2Hz(HORIZ, sy);
       ex = ppm2Hz(HORIZ, wy) + sx;
    } else if(strcmp(axis,ax2) == 0) {
       sx = ppm2Hz(HORIZ, sx);
       ex = ppm2Hz(HORIZ, wx) + sx;
    } else return;

    e2 = *s2 + *w2;

    if(mode < STACKED) {
	if(sx > *s2) *s2 = sx;
	if(ex < e2 && ex > *s2) *w2 = ex-*s2;
    } else {

      double xshift0, yshift0, xshift, yshift;
      int activeWin, vps, n = 0, ind = 0, activeInd = 0;
      
      if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
      else vps = (int)d;
      if(vps < 2) return;

      //activeWin = getActiveWin();
      activeWin = getAlignvp();

      for(i=0; i<vps; i++) {
          if (P_getreal(GLOBAL,"jvpexps", &d, i+1) != 0) d = 0.0;
          if(d < 0.5) continue;

	  if(i < id) ind++;
	  n++; 
	  if((i+1) == activeWin) activeInd = n; 
      }

      if(P_getreal(GLOBAL, "spshift", &d, 1)) xshift0 = 10.0;
      else xshift0 = d;
      if(P_getreal(GLOBAL, "vpshift", &d, 1)) yshift0 = 10.0;
      else yshift0 = d;

      xshift = (ind - activeInd)*xshift0; 
      yshift = (n-ind)*yshift0; 
/*
      xshift = (ind-1)*xshift0; 
      yshift = (n-ind)*yshift0; 
*/
	if(mode == STACKED) {
          (*s2) += xshift;
	  P_setreal(CURRENT, "vp", yshift, 1);
        } else if(mode == UNSTACKED) {
          //(*s2) -= xshift;
	  P_setreal(CURRENT, "vp", 0, 1);
	}

	if(ind == activeInd && sx > ((*s2)-xshift0*(activeInd-1)))
	  *s2 = sx + xshift0*(activeInd-1);
	else 

	if(sx > *s2) *s2 = sx;

	if(ind == activeInd && ex < (e2+xshift0*(n-activeInd)) 
		&& (ex - xshift0*(n-activeInd) - *s2)>0) 
	  *w2 = ex - xshift0*(n-activeInd) - *s2;
	else 

 	if(ex < e2 && ex>*s2) *w2 = ex-*s2;
    }
}

int isInset() {
  int buttonMode=getButtonMode();
  return (this_is_inset == 1 || buttonMode == SPWP_MODE ||
	buttonMode == SCWC_MODE);
}

void set_graph_flag(int flg) {
  graph_flag = flg;
}
#endif
