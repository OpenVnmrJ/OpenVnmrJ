/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/***********************************************/
/*  dconi	-	interactive 2D display */
/***********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "vnmrsys.h"
#include "allocate.h"
#include "data.h"
#include "disp.h"
#include "init2d.h"
#include "graphics.h"
#include "group.h"
#include "sky.h"
#include "pvars.h"
#include "wjunk.h"
#include "buttons.h"
#include "dscale.h"
#include "init_display.h"

static int s_overlayMode = 0;
static int oldx_cursor[2],oldy_cursor[2],idelta,idelta1,xoffset,yoffset;
static int last_trace,leveldisplay_active;
static int rev_h, rev_v;
static int freq_h, freq_v;
static int revAxis_h, revAxis_v;
static int axisonly = 0;

static int level_y0,level_y1,level_x1;
static int level_first;
static int spec1,spec2,plot_spec,next,erase1,erase2,plot_erase;
static int proj_sumflag = 0;
static int old_dconi_mode = -1;
static float *project_buffer;
static float discalib;
static double last_vs,cr1,delta1;
static float *phasfl;

static char mark_textfname[ MAXPATHL ];
static FILE *mark_textfptr = NULL;

char dconi_runstring[128];
int dconi_mode;
int trace_mode;

static char expandMacro[MAXSTR];
static char traceMacro[MAXSTR];
static char againMacro[MAXSTR];

extern void releasevarlist();
extern void refresh_graf();
extern void x_crosshair(int pos, int off1, int off2, double val1, int yval, double val2);
extern void y_crosshair(int pos, int off1, int off2, double val);
extern void restore_original_cursor();
extern void reset_dcon_color_threshold(int top, int bot);
extern void set_graph_clear_flag(int n);
extern void addNewZoom(char *cmd, double c2, double d2, double c1, double d1);
extern void getNextZoom(double *c2, double *d2, double *c1, double *d1);
extern void getCurrentZoomFromList(double *c2, double *d2, double *c1, double *d1);
#ifdef X11
extern int dconi_x_rast_cursor();
extern int dconi_y_rast_cursor();
#endif
extern void calc_hires_ps(float *ptr, double scale, int dfpnt, int depnt, int npnt,
           int vertical, int offset, int maxv, int minv, int dcolor);

/* argument lists need to be fixed */
extern void set_turnoff_routine();

extern int start_from_ft2d;   /* set by ft2d if dconi is to be executed */
extern double graysl,graycntr;
extern int gray_p_flag;
extern int dcon_first_color,dcon_num_colors,dcon_colorflag;
extern int debug1;
extern int menuflag;
extern int in_plot_mode;  /* defined in graphics.c */
extern int hires_ps;
extern int VnmrJViewId;
static int mapAxis(int num, double *c1, double *c2, double *d1, double *d2, 
   char *trace, char *axis1, char * axis2, char *tr, char *ax1, char *ax2);
void getAxes(char *ax2, char *ax1, int len);
void dconi_checkSpwp(double *, double *, double *,  double *, int);
static void displaylevels( int erase_text );
static void update_cursor(int cursor_number, int x, int y);
static void dconi_full();
static int dconi_newcursor(int butnum, int x, int y, int moveflag);
static int dconi_newtrace(int butnum, int x, int y, int moveflag);
static int dconi_multiply(int x, int y, int moveflag);
static int run_dcon();
static void setcolorth(int x, int y);
static int dconi_param_to_runstring();
void turnoff_dconi();
void setcolormap(int firstcolor, int numcolors, int th, int phcolor);
void update_vs_pars();
void set_gray_slope(int x, int y);
void erase_projections();
int init_dconi(int plotflag);
void dconi_cursor();
void dconi_sendSpwp_2D(double c1, double c2, 
	double d1, double d2, int mode, char *trace, char *axis1, char *axis2);
void dconi_sendSpwp_1D(double c2, double d2, int mode, char *axis);
int dconi_currentZoom(double *c2, double *d2, double *c1, double *d1);
void dconi_sendSpecInfo(int frame);
extern int aspFrame(char *, int, int, int, int, int);
int traceMode=0;

#define NOTRACE_MODE	0
#define CURSOR_MODE	1
#define HPROJ_MODE	3
#define VPROJ_MODE	4
#define BOX_MODE	5

#define TRUE		1
#define FALSE		0

#define BAND 10

#define SELECT_BOX_COLOR	0
#define SELECT_GRAY_COLOR	1
#define SELECT_AV_COLOR		2
#define SELECT_PH_COLOR		3

#define MAX_PH_THRESH           6
#define MAX_AV_THRESH           14

#define NOTOVERLAID_NOTALIGNED 0
#define OVERLAID_NOTALIGNED 1
#define NOTOVERLAID_ALIGNED 2
#define OVERLAID_ALIGNED 3
#define STACKED 4
#define UNSTACKED 5

#define ALIGN_2D_Y 6

#define zerofill(data_pntr, npoints_to_fill)			\
			datafill(data_pntr, npoints_to_fill,	\
				 0.0)

#ifdef X11
extern int dconi_x_cursor();
extern int dconi_y_cursor();
#else 
extern int x_cursor();
extern int y_cursor();
#endif 

#ifdef VNMRJ
extern int vj_x_cursor(int i, int *old_pos, int new_pos, int c);
extern int vj_y_cursor(int i, int *old_pos, int new_pos, int c);
extern int isJprintMode();
extern void ignoreGraphFunc(int n);
extern void addGraphSubFunc(char *name);
extern void removeGraphSubFunc(char *name);
extern void set_hourglass_cursor();
extern void restore_original_cursor();
#endif

extern double Hz2ppm(int direction, double val);
extern double ppm2Hz(int direction, double val);
extern double convert4ppm(int direction, double val);
void dconi_sendCursor(double c1, double c2, double d1, double d2, int mode);
void dconi_sendCrosshair(double c1, double c2, double f1, double f2);
void dconi_noCrosshair();
extern void m_noCrosshair();
extern int getButtonMode();
void dconi_sendSpwp(double c1, double c2, double d1, double d2, int mode);
void dconi_nextZoomin(double *c2, double *c1, double *d2, double *d1);
void dconi_nextZoom(double *c2, double *c1, double *d2, double *d1);
void dconi_prevZoom(double *c2, double *c1, double *d2, double *d1);
extern void getFrameG(int id, int *x, int *y, int *w, int *h);
extern int getFrameID();
extern int getOverlayMode();
extern int getActiveWin();
extern int getChartMode();
extern int getAlignvp();
extern void getSweepInfo(double *, double *, double *, double *,char *,char *);
extern int showCursor();
extern void setButtonMode(int);
extern int getDscaleDecimal(int);
extern void get_nuc_name(int direction, char *nucname, int n);
extern void get_sw_par(int direction, double *val);
extern void get_rflrfp(int direction, double *val);

int getAxisOnly()
{
   return(axisonly);
}

static void getFreq(int dim,  double crval, double spval, double wpval, double *freq, int *pnt)
{
   if (dim == HORIZ)
   {
      if ( ! freq_h )
      {
        *freq = crval - spval;
        if (pnt)
           *pnt  = (int)((double)dfpnt  + (double)dnpnt  * *freq / wpval + 0.5);
      }
      else if (revAxis_h)
      {
        *freq = spval - crval;
        if (pnt)
           *pnt  = (int)((double)dfpnt  + (double) dnpnt - (double)dnpnt  * *freq / wpval + 0.5);
      }
      else
      {
        *freq = wpval + spval - crval;
        if (pnt)
           *pnt  = (int)((double)dfpnt  + (double)dnpnt  * *freq / wpval + 0.5);

      }
   }
   else
   {
      if ( ! freq_v )
      {
        *freq = crval - spval;
        if (pnt)
           *pnt  = (int)((double)dfpnt2  + (double)dnpnt2  * *freq / wpval + 0.5);
      }
      else if (revAxis_v)
      {
        *freq = spval - crval;
        if (pnt)
           *pnt  = (int)((double)dfpnt2  + (double) dnpnt2 - (double)dnpnt2  * *freq / wpval + 0.5);
      }
      else
      {
        *freq = wpval + spval - crval;
        if (pnt)
           *pnt  = (int)((double)dfpnt2  + (double)dnpnt2  * *freq / wpval + 0.5);
      }
   }
}

/******************/
void dconi_reset()
/******************/
{
  if (WgraphicsdisplayValid("dconi"))  {
    update_cursor(0,0,0);
    update_cursor(1,0,0);
    }
  last_trace = -1;
  last_vs = 0;
  disp_status("      ");
  if (!d2flag && axisonly)
     setVertAxis();
  xoffset = mnumxpnts - dfpnt + 8*xcharpixels;
  yoffset = dfpnt2 + dnpnt2 + mnumypnts / 50;
  if (project_buffer) release(project_buffer);
  project_buffer = 0;
  trace_mode=0;
}

/*************************************/
static void update_cursor(int cursor_number, int x, int y)
/*************************************/
{
  if(!showCursor()) {
	oldx_cursor[cursor_number] = x;
	oldy_cursor[cursor_number] = y;
#ifdef VNMRJ
        vj_x_cursor(cursor_number, &oldx_cursor[cursor_number],0, CURSOR_COLOR);
        vj_y_cursor(cursor_number, &oldy_cursor[cursor_number],0, CURSOR_COLOR);
#endif
  	return;
  }

  if ((Wistek() || WisSunColor()) && leveldisplay_active &&
                      (dcon_colorflag==SELECT_GRAY_COLOR))
  {
#ifdef X11
    dconi_x_rast_cursor(&oldx_cursor[cursor_number],x,dfpnt2,dnpnt2,cursor_number);
    dconi_y_rast_cursor(&oldy_cursor[cursor_number],y,dfpnt,dnpnt,cursor_number);
#else 
    x_rast_cursor(&oldx_cursor[cursor_number],x,dfpnt2,dnpnt2,cursor_number);
    y_rast_cursor(&oldy_cursor[cursor_number],y,dfpnt,dnpnt,cursor_number);
#endif 
  }
  else
  {
#ifdef X11
  xormode();
  color(CURSOR_COLOR);
  dconi_x_cursor(&oldx_cursor[cursor_number],x, cursor_number);
  dconi_y_cursor(&oldy_cursor[cursor_number],y, cursor_number);
  normalmode();
#else 

#ifdef VNMRJ
  vj_x_cursor(cursor_number, &oldx_cursor[cursor_number],x, CURSOR_COLOR);
  vj_y_cursor(cursor_number, &oldy_cursor[cursor_number],y, CURSOR_COLOR);
#else 
  xormode();
  color(CURSOR_COLOR);
  x_cursor(&oldx_cursor[cursor_number],x);
  y_cursor(&oldy_cursor[cursor_number],y);
  normalmode();
#endif 
#endif 
}}

static void skip_save_graphics_cmd(int n)
{
#ifdef VNMRJ
     ignoreGraphFunc(n);
#endif 
}


/*****************************************/
static void dconi_m_newcursor(int butnum, int x, int y, int moveflag)
/*****************************************/
{
  if (in_plot_mode)
  {
	setdisplay();
	exp_factors(1);
  }
  if ((butnum == 3) && (dconi_mode != BOX_MODE))  {
    Wturnoff_buttons();
    set_turnoff_routine(turnoff_dconi);
    dconi_mode = BOX_MODE;
    dconi_cursor();
    dconi_newcursor(butnum,x,y,moveflag);
    if (P_setstring(GLOBAL,"crmode","b",0))
      Werrprintf("Unable to set variable \"crmode\".");
    execString("menu\n");
    }
  else  {
    dconi_newcursor(butnum,x,y,moveflag);
    }

  if(trace_mode == TRACE_MODE)  
  dconi_sendCursor(cr1, cr, delta1, delta, trace_mode);
  else
  dconi_sendCursor(cr1, cr, delta1, delta, dconi_mode);

}

/*****************************************/
static int dconi_newcursor(int butnum, int x, int y, int moveflag)
/*****************************************/
{ int r;

  int dez;
  Wgmode();
  if (butnum==3)
    { if (leveldisplay_active  && (dcon_colorflag==SELECT_GRAY_COLOR)
                                                   && (x>dfpnt+dnpnt))
        { set_gray_slope(x,y);
          return 0;   
        }
      else
        { if (dconi_mode!=BOX_MODE)	/*  box button */
            { if (dconi_mode!=CURSOR_MODE)
                { dconi_reset();
                  dconi_mode = -1;
                  dconi_cursor();
                }
              dconi_mode = BOX_MODE;
 	      dez = getDscaleDecimal(1)+2;
              InitVal(FIELD2,VERT, DELTA_NAME, PARAM_COLOR,UNIT4,
                                   NO_NAME,-PARAM_COLOR,SCALED,dez);
 	      dez = getDscaleDecimal(0)+2;
              InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,UNIT4,
                                   NO_NAME,-PARAM_COLOR,SCALED,dez);
              disp_status("BOX   ");
              update_vs_pars();
            }
        }
    }
  if (butnum==2)  {
     dconi_multiply(x,y,moveflag);
     }
  else
  {
    set_cursors((dconi_mode == BOX_MODE),(butnum == 1),&x,
           oldx_cursor[0],oldx_cursor[1],&idelta,dfpnt,dnpnt);
    set_cursors((dconi_mode == BOX_MODE),(butnum == 1),&y,
           oldy_cursor[0],oldy_cursor[1],&idelta1,dfpnt2+1,dnpnt2-1);
    if (dconi_mode==BOX_MODE)
    {
      if (butnum==1)	/* cursor button */
        update_cursor(1,x+idelta,y+idelta1);
      else
        update_cursor(1,oldx_cursor[0]+idelta,oldy_cursor[0]+idelta1);
      delta  = idelta  * wp  / dnpnt ;
      delta1 = idelta1 * wp1 / dnpnt2;
      UpdateVal(HORIZ,DELTA_NAME,delta,SHOW);
      UpdateVal(VERT,DELTA_NAME,delta1,SHOW);
    }
    if (butnum==1)	/* cursor button */
    {
      r = 1 + fpnt1 + npnt1 * (y - dfpnt2) / dnpnt2;
      if (r != specIndex)
        { specIndex = r;
          disp_specIndex(specIndex);
        }
      update_cursor(0,x,y);
      if ( ! rev_h)
         cr  = - (x - dfpnt ) * wp  / dnpnt  + wp  + sp;
      else if ( revAxis_h )
         cr  = (x - dfpnt - dnpnt ) * wp  / dnpnt  + sp;
      else
         cr = (x - dfpnt) * wp  / dnpnt  + sp;
      if ( ! rev_v)
         cr1 = - (y - dfpnt2) * wp1 / dnpnt2 + wp1 + sp1;
      else if ( revAxis_v )
         cr1  = (y - dfpnt2 - dnpnt2 ) * wp1  / dnpnt2  + sp1;
      else
         cr1 =   (y - dfpnt2) * wp1 / dnpnt2 + sp1;
      UpdateVal(HORIZ,CR_NAME,cr,SHOW);
      UpdateVal(VERT,CR_NAME,cr1,SHOW);

    }
  }
  Wsetgraphicsdisplay("dconi");

  return 0;
}

/*********************/
static int getxy(int *x, int *y, int *x1, int *y1)
/*********************/
{
  double freq_val;

  get_cursor_pars(HORIZ,&cr,&delta);
  get_cursor_pars(VERT,&cr1,&delta1);

#ifdef DEBUG
  fprintf(stderr,"getxy, dfpnt= %d  dnpnt= %d  wp= %.12g  cr= %.12g  sp= %.12g\n",
	 dfpnt, dnpnt, wp, cr, sp);
  fprintf(stderr,"getxy, dfpnt2= %d  dnpnt2= %d  wp1= %.12g  cr1= %.12g  sp1= %.12g\n",
	 dfpnt2, dnpnt2, wp1, cr1, sp1);
#endif


  getFreq(HORIZ, cr, sp, wp, &freq_val, x);
  getFreq(VERT, cr1, sp1, wp1, &freq_val, y);
  *x1 = (int)((double)*x     + (double)dnpnt  * delta  / wp + 0.5);
  *y1 = (int)((double)*y     + (double)dnpnt2 * delta1 / wp1 + 0.5);

  return 0;
}

/*******************/
void dconi_cursor()
/*******************/
{ int x,y,x1,y1;
  int buttonMode = getButtonMode();
  int dez;
  Wgmode();
  x = DELTA_NAME;
  y = PARAM_COLOR;
  switch (dconi_mode)
    { case CURSOR_MODE:	dconi_mode = BOX_MODE;
                        x = NO_NAME;
                        y = -PARAM_COLOR;
			if (P_setstring(GLOBAL,"crmode","b",0))
			  Werrprintf("Unable to set variable \"crmode\".");
                        break;
      case BOX_MODE:	dconi_mode = CURSOR_MODE;
			if (P_setstring(GLOBAL,"crmode","c",0))
			  Werrprintf("Unable to set variable \"crmode\".");
                        break;
      default:		Wturnoff_mouse();
                        dconi_reset();
                        dconi_mode = CURSOR_MODE;
			if (P_setstring(GLOBAL,"crmode","c",0))
			  Werrprintf("Unable to set variable \"crmode\".");
                        break;
    }
  dez = getDscaleDecimal(1)+2;
  InitVal(FIELD1,VERT, CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,dez);
  InitVal(FIELD2,VERT, DELTA_NAME, PARAM_COLOR,UNIT4,
                       x,          y,SCALED,dez);
  dez = getDscaleDecimal(0)+2;
  InitVal(FIELD3,HORIZ,CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,dez);
  InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,UNIT4,
                       x,          y,SCALED,dez);
  DispField1(FIELD5,PARAM_COLOR,"vsproj");
  DispField1(FIELD6,PARAM_COLOR,"vs2d");
  update_vs_pars();
  activate_mouse(dconi_m_newcursor,dconi_reset);
  if (getxy(&x,&y,&x1,&y1)) return;
#ifdef VNMRJ
  if (isJprintMode())
       dconi_mode = old_dconi_mode;
#endif
  if (dconi_mode==BOX_MODE)
    {
      idelta  = x1 - x;
      idelta1 = y1 - y;
      if (x1>dfpnt+dnpnt)
        x1=dfpnt+dnpnt;
    }
  dconi_newcursor(1,x,y,0);
  if (dconi_mode==BOX_MODE)
    {
      dconi_newcursor(3,x1,y1,0);
    }
  else
    {
      update_cursor(1,0,0);
    }
  Wsetgraphicsdisplay("dconi");

  if(buttonMode != ZOOM_MODE && buttonMode != PAN_MODE)
        setButtonMode(SELECT_MODE);

}

/****************************************/
static void dconi_m_newtrace(butnum,x,y,moveflag)
/****************************************/
int butnum,x,y,moveflag;
{
  if (axisonly)
     return;
  if (butnum == 3)  {
    Wturnoff_buttons();
    set_turnoff_routine(turnoff_dconi);
    dconi_mode = BOX_MODE;
    dconi_cursor();
    dconi_newtrace(butnum,x,y,moveflag);
    if (P_setstring(GLOBAL,"crmode","b",0))
      Werrprintf("Unable to set variable \"crmode\".");
    execString("menu\n");
    }
  else  {
    dconi_newtrace(butnum,x,y,moveflag);
    }
}

/****************************************/
static int dconi_newtrace(int butnum, int x, int y, int moveflag)
/****************************************/
{ int ctrace;
  if (axisonly)
     return(0);
  Wgmode();
  if (butnum==2)
    { dconi_multiply(x,y,moveflag);
    }
  else if (butnum==3)
    { Wturnoff_mouse();
      dconi_reset();
      dconi_mode = CURSOR_MODE; /* force dconi_cursor into BOX_MODE */
      dconi_cursor();
    }
  else if (butnum == 1)
    { dconi_newcursor(1,x,y,0);
      ctrace = fpnt1 + npnt1 * (oldy_cursor[0] - dfpnt2) / dnpnt2;
      if ((last_trace!=ctrace)||(last_vs!=vsproj))
        { if ((phasfl=gettrace(ctrace,fpnt))==0)
            { Wturnoff_buttons(); return 1; }
          if (traceMacro[0] != '\0')
             execString(traceMacro);
          if (calc_ybars(phasfl,1,discalib*vsproj,dfpnt,dnpnt,npnt,
                          yoffset,next))
            return 1;
          if (displayspec(dfpnt,dnpnt,0,&next,&spec1,&erase1,
                          mnumypnts-3,dfpnt2+dnpnt2+3,SPEC_COLOR))
            return 1;
        }
      last_trace = ctrace;
      last_vs = vsproj;
      dconi_newcursor(1,x,y,0);
    }
  Wsetgraphicsdisplay("dconi");
  return 0;
}

/******************/
static void dconi_trace()
/******************/
{ int x,y,x1,y1;
  int dez;
  if (axisonly)
     return;
  Wgmode();
  Wturnoff_mouse();
  dconi_reset();
  set_graph_clear_flag(TRUE);
  /* erase the screen behind the trace */
  color(BACK);
  amove(dfpnt-1,dfpnt2+dnpnt2+3);
  box(dnpnt+2,mnumypnts-dfpnt2-dnpnt2-5);
  erase1 = 0;
  set_graph_clear_flag(FALSE);
  /* activate cursors */
  if (getxy(&x,&y,&x1,&y1))
    return;
  disp_status("TRACE ");
  activate_mouse(dconi_m_newtrace,dconi_reset);
  trace_mode = TRACE_MODE;
  dconi_mode = CURSOR_MODE;
  dez = getDscaleDecimal(1)+2;
  InitVal(FIELD1,VERT, CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,dez);
  InitVal(FIELD2,VERT, DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,dez);
  dez = getDscaleDecimal(0)+2;
  InitVal(FIELD3,HORIZ,CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,dez);
  InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,dez);
  DispField1(FIELD5,PARAM_COLOR,"vsproj");
  DispField1(FIELD6,PARAM_COLOR,"vs2d");
  update_vs_pars();
  dconi_newtrace(1,x,y,0);
  Wsetgraphicsdisplay("dconi");
  setButtonMode(TRACE_MODE);
}

/*******************/
static int update_hproj()
/*******************/
{
  if (axisonly)
     return(0);
  if (plot && hires_ps)
  {
     calc_hires_ps(project_buffer,discalib*vsproj,dfpnt,dnpnt,npnt,0,yoffset,
           mnumypnts-3,dfpnt2+dnpnt2+3,SPEC_COLOR);
     return(0);
  }
  if (calc_ybars(project_buffer,1,discalib*vsproj,dfpnt,dnpnt,npnt,
                 yoffset,next))
    return 1;
  if (plot)
    { plot_erase = 0;
      if (displayspec(dfpnt,dnpnt,0,&next,&plot_spec,&plot_erase,
                  mnumypnts-3,dfpnt2+dnpnt2+3,SPEC_COLOR))
        return 1;
    }
  else
    { if (displayspec(dfpnt,dnpnt,0,&next,&spec1,&erase1,
                  mnumypnts-3,dfpnt2+dnpnt2+3,SPEC_COLOR))
        return 1;
    }
  Wsetgraphicsdisplay("dconi");
  return 0;
}

/*************************/
static void dconi_hproj(sumflag)
/*************************/
int sumflag;
{ int trace,first_trace,num_traces,x,y,x1,y1;
  float *phasfl;
  int dez;
  
  if (axisonly)
     return;
  Wgmode();
  getxy(&x,&y,&x1,&y1);
  dconi_newcursor(1,x,y,0);
  if (dconi_mode==BOX_MODE)
  {
      if (x1>dfpnt+dnpnt)
        x1=dfpnt+dnpnt;
      dconi_newcursor(3,x1,y1,1);
      first_trace = fpnt1 + npnt1 * (y - dfpnt2) / dnpnt2;
      if (first_trace<fpnt1) first_trace = fpnt1;
      num_traces  = npnt1 * (y1 - y) / dnpnt2;
      if (num_traces>npnt1) num_traces=npnt1;
      else if (num_traces<1) num_traces=1;
      if (first_trace>fpnt1+npnt1-num_traces)
        first_trace=fpnt1+npnt1-num_traces;
  }
  else
  {
      first_trace = fpnt1;
      num_traces  = npnt1;
  }
  Wturnoff_mouse();
  dconi_reset();
  disp_status("HPROJ ");
  if ((project_buffer = (float*)allocateWithId(sizeof(float)*npnt,"dconi"))==0)
    { Werrprintf("cannot allocate project buffer space");
      Wturnoff_buttons();
      return;
    }
  /* erase the screen behind the trace */
  set_graph_clear_flag(TRUE);
  color(BACK);
  amove(dfpnt-1,dfpnt2+dnpnt2+3);
  box(dnpnt+2,mnumypnts-dfpnt2-dnpnt2-5);
  erase1 = 0;
  set_graph_clear_flag(FALSE);
  zerofill(project_buffer,npnt);
  for (trace=first_trace; trace < first_trace+num_traces; trace++)
    { if ((phasfl=gettrace(trace,fpnt))==0)
      {
         Wturnoff_buttons();
         return;
      }
      if (sumflag)
        skyadd(phasfl,project_buffer,project_buffer,npnt);
      else
        skymax(phasfl,project_buffer,project_buffer,npnt);
    }
  update_hproj();
  activate_mouse(dconi_m_newcursor,dconi_reset);
  trace_mode = HPROJ_MODE;
  dez = getDscaleDecimal(1)+2;
  InitVal(FIELD1,VERT, CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,dez);
  InitVal(FIELD2,VERT, DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,dez);
  dez = getDscaleDecimal(0)+2;
  InitVal(FIELD3,HORIZ,CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,dez);
  InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,dez);
  DispField1(FIELD5,PARAM_COLOR,"vsproj");
  DispField1(FIELD6,PARAM_COLOR,"vs2d");
  update_vs_pars();
  dconi_newcursor(1,x,y,0);
  Wsetgraphicsdisplay("dconi");
  proj_sumflag = sumflag;
  setButtonMode(SELECT_MODE);
}

/*******************/
static int update_vproj()
/*******************/
{
  if (axisonly)
     return(0);
  if (plot && hires_ps)
  {
     calc_hires_ps(project_buffer,discalib*vsproj,dfpnt2,dnpnt2,npnt1,1,xoffset,
            mnumxpnts-1,xoffset-mnumxpnts/80+1,SPEC_COLOR);
     return(0);
  }
  if (calc_ybars(project_buffer,1,discalib*vsproj,dfpnt2,dnpnt2,npnt1,
                 xoffset,next))
    return 1;
  if (plot)
    { plot_erase = 0;
      if (displayspec(dfpnt2,dnpnt2,1,&next,&plot_spec,&plot_erase,
                  mnumxpnts-1,xoffset-mnumxpnts/80+1,SPEC_COLOR))
        return 1;
    }
  else
    { if (displayspec(dfpnt2,dnpnt2,1,&next,&spec2,&erase2,
                  mnumxpnts-1,xoffset-mnumxpnts/80+1,SPEC_COLOR))
        return 1;
    }
  Wsetgraphicsdisplay("dconi");
  return 0;
}

/*************************/
static void dconi_vproj(sumflag)
/*************************/
int sumflag;
{ int first_point,num_points,trace,x,y,x1,y1;
  register int i;
  register float max;
  register float *p,*phasfl;
  int dez;
  if (axisonly)
     return;
  Wgmode();
  getxy(&x,&y,&x1,&y1);
  dconi_newcursor(1,x,y,0);
  if (dconi_mode==BOX_MODE)
    { if (x1>dfpnt+dnpnt)
        x1=dfpnt+dnpnt;
      dconi_newcursor(3,x1,y1,0);
      first_point = fpnt  + npnt  * (x - dfpnt ) / dnpnt ;
      if (first_point<fpnt) first_point = fpnt;
      num_points  = npnt  * (x1 - x) / dnpnt ;
      if (num_points>npnt) num_points=npnt;
      else if (num_points<1) num_points=1;
      if (first_point>fpnt+npnt-num_points) first_point=fpnt+npnt-num_points;
    }
  else
    {
      first_point = fpnt;
      num_points  = npnt;
    }
  Wturnoff_mouse();
  dconi_reset();
  disp_status("VPROJ ");
  if ((project_buffer=(float*)allocateWithId(sizeof(float)*npnt1,"dconi"))==0)
    { Werrprintf("cannot allocate project buffer space");
      Wturnoff_buttons();
      return;
    }
  /* erase the screen behind the trace */
  set_graph_clear_flag(TRUE);
  color(BACK);
  amove(1,dfpnt2-1);
  box(mnumxpnts-(xoffset-mnumxpnts/80),dnpnt2+2);
  erase2 = 0;
  set_graph_clear_flag(FALSE);
  p = project_buffer;
  for (trace=fpnt1; trace < fpnt1+npnt1; trace++)
    { if ((phasfl=gettrace(trace,first_point))==0)
        { Wturnoff_buttons(); return; }
      i = num_points; max = 0;
      if (sumflag)
        { while (i--)
            max += *phasfl++;
        }
      else
        { while (i--)
            { if (*phasfl>max) max = *phasfl;
              phasfl++;
            }
        }
      *p++ = max;
    }
  update_vproj();
  activate_mouse(dconi_m_newcursor,dconi_reset);
  trace_mode = VPROJ_MODE;
  dez = getDscaleDecimal(1)+2;
  InitVal(FIELD1,VERT, CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,dez);
  InitVal(FIELD2,VERT, DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,dez);
  dez = getDscaleDecimal(0)+2;
  InitVal(FIELD3,HORIZ,CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,dez);
  InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,dez);
  DispField1(FIELD5,PARAM_COLOR,"vsproj");
  DispField1(FIELD6,PARAM_COLOR,"vs2d");
  update_vs_pars();
  dconi_newcursor(1,x,y,0);
  Wsetgraphicsdisplay("dconi");
  proj_sumflag = sumflag;
  setButtonMode(SELECT_MODE);
}

/**************************/
static void dconi_get_new_vs(x,y)
/**************************/
int x,y;
{ int first_trace,first_point,num_traces,x1,y1,trace;
  register int i,num_points;
  register float max;
  register float *phasfl;
  float vs1;
  x -= BAND/2;
  y -= BAND/2;
  x1 = x + BAND;
  y1 = y + BAND;
  first_trace = fpnt1 + npnt1 * (y - dfpnt2) / dnpnt2;
  first_point = fpnt  + npnt  * (x - dfpnt ) / dnpnt ;
  if (first_trace<fpnt1) first_trace = fpnt1;
  num_traces  = npnt1 * (y1 - y) / dnpnt2;
  if (num_traces>npnt1) num_traces=npnt1;
  else if (num_traces<1) num_traces=1;
  if (first_trace>fpnt1+npnt1-num_traces)
    first_trace=fpnt1+npnt1-num_traces;
  if (first_point<fpnt) first_point = fpnt;
  num_points  = npnt  * (x1 - x) / dnpnt ;
  if (num_points>npnt) num_points=npnt;
  else if (num_points<1) num_points=1;
  if (first_point>fpnt+npnt-num_points) first_point=fpnt+npnt-num_points;
  max = 0;
  for (trace=first_trace; trace < first_trace+num_traces; trace++)
    { if ((phasfl=gettrace(trace,first_point))==0)
        { Wturnoff_buttons(); return; }
      for (i=0; i<num_points; i++)
        { if (fabs(*phasfl)>max)
            max = fabs(*phasfl);
          phasfl++;
        }
    }
if (leveldisplay_active && (dcon_colorflag==SELECT_GRAY_COLOR))
  { vs2d = ((double) NUM_GRAY_COLORS)/ max;  
    if (vs2d > 1e8) vs2d=1e8; 
    if (vs2d< 0) vs2d=1.0;
  }
else { 
  if (vs2d > 1e8) vs2d=1e8; 
  if (vs2d <= 0.0) vs2d = 1e-8;
  vs1 = vs2d;
  for (i=0; i<th; i++) vs1 /= 2.0;
  max = max * vs1;
  if (debug1)
    { Wscrprintf("first_trace=%d, num_traces=%d\n",first_trace,num_traces);
      Wscrprintf("first_point=%d, num_points=%d\n",first_point,num_points);
      Wscrprintf("max=%g\n",max);
    }
  if (max<=1.0)
    { if (max<0.02) max=0.02;
      vs2d = vs2d * 2.0 / max;
    }
  else
    { if (max>50) max=50;
      vs2d = vs2d / (2.0 * max);
    }
}
  update_vs_pars();
}

#ifdef OLD
/*****************/
static void b_mark_dconi()
/*****************/
{
   dconi_mark(dconi_mode, TRUE, TRUE, NULL, NULL, NULL, NULL);
}
#endif

/*  DCONI_MARK is now a global subroutine which implements the 2D MARK
    function, either from the DCONI button or as part of the MARK command.	*/

int dconi_mark(int mark_mode, int display_flag, int button_flag, float *int_v_ptr,
               float *max_i_ptr, float *f1_max_ptr, float *f2_max_ptr)
{
	int		 x, y, x1, y1, first_point, first_trace, newfile,
                         num_entry, num_traces, num_points, trace;
	float		 maxcr, maxcr1, intval;
	register float	*phasfl;
	register float	 datamax, sum;
	register int	 i, maxpoint, maxtrace;
        double           scale;
        int              first_ch,last_ch;
        int              first_direction;

/*  If this subroutine was called from the MARK command, initialize
    variables that are normally initialized when DCONI is started.	*/

        if (axisonly)
           return(0);
        (void) display_flag;
	if (button_flag == FALSE ) {
		discalib = (float)(mnumypnts-ymin) / wc2max;
	}

	strcpy(&mark_textfname[ 0 ],curexpdir);
#ifdef UNIX
	strcat(&mark_textfname[ 0 ],"/mark2d.out");
#else 
	strcat(&mark_textfname[ 0 ],"mark2d.out");
#endif 

/*  See comment at corresponding point in DS.C	*/

	newfile = access( &mark_textfname[ 0 ], 0 );
        num_entry = 1;
        if (!newfile)
        {
           register int c;

           mark_textfptr=fopen(&mark_textfname[ 0 ],"r");
           while ((c = getc(mark_textfptr)) != EOF)
              if (c == '\n')
                 num_entry++;
           fclose(mark_textfptr);
           num_entry -= 1;
        }
	mark_textfptr=fopen(&mark_textfname[ 0 ],"a");
	if (mark_textfptr == NULL) {
		Werrprintf("cannot open file %s",&mark_textfname[ 0 ]);
		Wturnoff_buttons();
		return 1;
	}
        get_mark2d_info(&first_ch,&last_ch,&first_direction);
	if (newfile)
        {
	   fprintf(mark_textfptr,
"(high) f%c freq (high) f%c freq  (max) height   low f%c freq    low f%c freq   ",
    first_ch,last_ch,first_ch,last_ch);
	   fprintf(mark_textfptr,"   integral   max:  f%c freq      f%c freq\n",
                   first_ch,last_ch);
	}

/*  Get values for `first_point' (x axis) and `first_trace' (y axis).	*/

	getxy(&x, &y, &x1, &y1);

#ifdef DEBUG
    Wscrprintf( "getxy returned %d  %d  %d  %d\n", x, y, x1, y1 );
#endif 

	if (button_flag == TRUE) {
		Wgmode();
		dconi_newcursor(1,x,y,0);
	}
	first_trace = fpnt1 + npnt1 * (y - dfpnt2) / dnpnt2;
	if (first_trace<fpnt1) first_trace = fpnt1;
	first_point = fpnt  + npnt  * (x - dfpnt ) / dnpnt ;
	if (first_point<fpnt) first_point = fpnt;
        scale = vs2d;
        if (normflag)
          scale *= normalize;

/*  2D box mode.  */

	if (mark_mode == BOX_MODE) {
		if (x1>dfpnt+dnpnt)
	 	  x1=dfpnt+dnpnt;
		if (button_flag == TRUE)
		  dconi_newcursor(3,x1,y1,0);
		num_traces  = npnt1 * (y1 - y) / dnpnt2;
		if (num_traces>npnt1)
		  num_traces=npnt1;
		else if (num_traces<1)
		  num_traces=1;
		if (first_trace>fpnt1+npnt1-num_traces)
		  first_trace=fpnt1+npnt1-num_traces;
		num_points = npnt  * (x1 - x) / dnpnt ;
		if (num_points>npnt)
		  num_points=npnt;
		else if (num_points<1)
		  num_points=1;
		if (first_point>fpnt+npnt-num_points)
		  first_point=fpnt+npnt-num_points;
		sum = 0;
		datamax = -1e+10;

/*  Set initial values for `maxpoint', `maxtrace' to be the initial
    values of the `trace' and `point' loops immediately below.		*/

		maxpoint = first_point;
		maxtrace = first_trace;

		for (trace=first_trace; trace < first_trace+num_traces; trace++) {
			if ((phasfl=gettrace(trace,first_point))==0) {
				Wturnoff_buttons();
				return 1;
			}
			for (i=0; i<num_points; i++) {
				if (*phasfl>datamax) {
					datamax = *phasfl;
					maxpoint = first_point + i;
					maxtrace = trace;
				}
				sum += *phasfl++;
			}
		}

		maxcr = sp + (double)(fpnt+npnt-maxpoint) * wp / npnt;
		maxcr1 = sp1 + (double)(fpnt1+npnt1-maxtrace) * wp1 / npnt1;

		datamax *= scale;
		intval = sum * ins2val;

/********************************************************************************/
/*  When MARK was only available from a button, the position of the right
    cursor (cr-delta) was printed.  This contrasted with the 1D version which
    printed the position of the left cursor (cr).  Now the 2D version also
    prints the position of the left cursor instead of the right cursor.		*/
/********************************************************************************/

             if (first_direction == HORIZ)
             {
		fprintf(mark_textfptr,
	    "%12.3f   %12.3f  %12.3f   %12.3f   %12.3f  %12.3f   %12.3f   %12.3f\n",
	     cr, cr1, datamax, cr-delta, cr1-delta1, intval, maxcr, maxcr1);
		Winfoprintf(
	    "Entry %d: integral=%10.3f max: f%c=%10.3f, f%c=%10.3f, ht=%10.3f",
	     num_entry,intval,first_ch,maxcr,last_ch,maxcr1,datamax);
             }
             else
             {
		fprintf(mark_textfptr,
	    "%12.3f   %12.3f  %12.3f   %12.3f   %12.3f  %12.3f   %12.3f   %12.3f\n",
	     cr1, cr, datamax, cr1-delta1, cr-delta, intval, maxcr1, maxcr);
		Winfoprintf(
	    "Entry %d: integral=%10.3f max: f%c=%10.3f, f%c=%10.3f, ht=%10.3f",
	     num_entry,intval,first_ch,maxcr1,last_ch,maxcr,datamax
		);
             }
	}

/*  2D cursor mode.  */

	else {
		if ((phasfl=gettrace(first_trace,first_point))==0) {
			Wturnoff_buttons();
			return 1;
		}

/*  The integral is always zero in cursor mode.  The coordinates of the maximum
    intensity are the coordinates of the cursors passed to this routine.	*/

		datamax = *phasfl * scale;
		intval = 0.0;

		maxcr = cr;
		maxcr1 = cr1;
             if (first_direction == HORIZ)
             {
		fprintf(mark_textfptr, "%12.3f   %12.3f   %12.3f\n",
	                cr,cr1, datamax);
		Winfoprintf( "Entry %d: f%c=%10.3f, f%c=%10.3f, ht=%10.3f",
	                      num_entry,first_ch,cr,last_ch, cr1, datamax);
             }
             else
             {
		fprintf(mark_textfptr, "%12.3f   %12.3f   %12.3f\n",
	                cr1,cr, datamax);
		Winfoprintf( "Entry %d: f%c=%10.3f, f%c=%10.3f, ht=%10.3f",
	                      num_entry,first_ch,cr1,last_ch, cr, datamax);
             }
	}

	if (int_v_ptr != NULL)
	  *int_v_ptr = intval;
	if (max_i_ptr != NULL)
	  *max_i_ptr = datamax;
        if (first_direction == HORIZ)
        {
	   if (f1_max_ptr != NULL)
	     *f1_max_ptr = maxcr1;
	   if (f2_max_ptr != NULL)
	     *f2_max_ptr = maxcr;
        }
        else
        {
	   if (f1_max_ptr != NULL)
	     *f1_max_ptr = maxcr;
	   if (f2_max_ptr != NULL)
	     *f2_max_ptr = maxcr1;
        }
	fclose( mark_textfptr );
        mark_textfptr = NULL;

	if (button_flag == TRUE)
	  Wsetgraphicsdisplay("dconi");
	return 0;
}

/*****************/
static int dconi_plot()
/*****************/
{
  if (axisonly)
     return(0);
  Wgmode();
  if (init_dconi(2)) return 1;
  if (trace_mode==TRACE_MODE)
    { disp_status("PLOT  ");
      if (plot && hires_ps)
      {
         calc_hires_ps(phasfl,discalib*vsproj,dfpnt,dnpnt,npnt,0,yoffset,
               mnumypnts,dfpnt2+dnpnt2+3,SPEC_COLOR);
      }
      else
      {
         if (calc_ybars(phasfl,1,discalib*vsproj,dfpnt,dnpnt,
                        npnt,yoffset,next))
           return 1;
         plot_erase = 0;
         if (displayspec(dfpnt,dnpnt,0,&next,&plot_spec,&plot_erase,
                           mnumypnts,dfpnt2+dnpnt2+3,SPEC_COLOR))
        return 1;
      }
      disp_status("TRACE ");
    }
  else if (trace_mode==HPROJ_MODE)
    { disp_status("PLOT  ");
      update_hproj();
      disp_status("HPROJ ");
    }
  else if (trace_mode==VPROJ_MODE)
    { disp_status("PLOT  ");
      update_vproj();
      disp_status("VPROJ ");
    }
  else
    Werrprintf("No trace or projection active currently");
  if (init_dconi(1)) return 1;
  Wsetgraphicsdisplay("dconi");
  return 0;
}

/*******************/
static int dconi_expand()
/*******************/
{ 
  if (dconi_mode!=BOX_MODE)
    { dconi_full();
      if (expandMacro[0] != '\0')
        execString(expandMacro);
  	aspFrame("dconi",0,0,0,0,0);
      return 0;
    }
#ifdef VNMRJ
  if(revflag) {
     addNewZoom("dconi",cr1,delta1,cr,delta);
  } else {
     addNewZoom("dconi",cr,delta,cr1,delta1);
  }
#endif
  Wgmode();
  disp_status("EXPAND");
  Wturnoff_mouse();
  erase_projections();
  dconi_reset();
  /* set sp,wp,sp1,wp1 according to expansion box */
  if ( ! freq_h )
     sp  = cr;
  else if ( revAxis_h )
     sp  = cr + delta;
  else
     sp  = cr - delta;
  wp  = delta;
  if ( ! freq_v )
     sp1  = cr1;
  else if ( revAxis_v )
     sp1  = cr1 + delta1;
  else
     sp1  = cr1 - delta1;
  wp1 = delta1;
  dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);
  UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
  UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
  UpdateVal(VERT,SP_NAME,sp1,NOSHOW);
  UpdateVal(VERT,WP_NAME,wp1,NOSHOW);
  run_dcon();
#ifdef VNMRJ
  if(revflag) {
     getNextZoom(&cr1,&delta1,&cr,&delta);
  } else {
     getNextZoom(&cr,&delta,&cr1,&delta1);
  }
  UpdateVal(HORIZ,CR_NAME,cr,NOSHOW);
  UpdateVal(HORIZ,DELTA_NAME,delta,NOSHOW);
  UpdateVal(VERT,CR_NAME,cr1,SHOW);
  UpdateVal(VERT,DELTA_NAME,delta1,NOSHOW);
#endif
  dconi_mode = -1;  /* force dconi_cursor() into the CURSOR_MODE */
  dconi_cursor();
  Wsetgraphicsdisplay("dconi");
  if (expandMacro[0] != '\0')
     execString(expandMacro);
  
#ifdef VNMRJ
  dconi_sendSpwp(sp1, sp, wp1, wp, 1);
#endif
  aspFrame("dconi",0,0,0,0,0);
  return 0;
}

/*****************/
static void dconi_full()
/*****************/
{
  double c2,c1,d2,d1;
  Wgmode();
  disp_status("FULL  ");
  Wturnoff_mouse();
  erase_projections();
  /* set box size to last expansion */
#ifdef VNMRJ
  getCurrentZoomFromList(&c2,&d2,&c1,&d1);
  if(d2 < 0) {
      c2=cr; c1=cr1; d2=delta; d1=delta1;
  }
#else 
  if ( ! freq_h )
     cr = sp;
  else if ( revAxis_h )
     cr  = sp - delta;
  else
     cr  = sp + delta;
  if ( ! freq_v )
     cr1 = sp1;
  else if ( revAxis_v )
     cr1 = sp1 - delta1;
  else
     cr1 = sp1 + delta1;
  delta  = fabs(wp);
  delta1 = fabs(wp1);
#endif
  dconi_reset();
  /* set sp,wp,sp1,wp1 for full display */
  sp  = - rflrfp;
  wp  = sw;
  sp1 = - rflrfp1;
  wp1 = sw1;
  dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);
  /* store the parameters */
  UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
  UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
  UpdateVal(VERT,SP_NAME,sp1,NOSHOW);
  UpdateVal(VERT,WP_NAME,wp1,NOSHOW);
  run_dcon();
  dconi_mode=CURSOR_MODE;
  dconi_cursor();
  Wsetgraphicsdisplay("dconi");
#ifdef VNMRJ
  cr=c2; cr1=c1; delta=d2; delta1=d1;
  UpdateVal(HORIZ,CR_NAME,cr,NOSHOW);
  UpdateVal(HORIZ,DELTA_NAME,delta,NOSHOW);
  UpdateVal(VERT,CR_NAME,cr1,SHOW);
  UpdateVal(VERT,DELTA_NAME,delta1,NOSHOW);

  dconi_sendSpwp(sp1, sp, wp1, wp, 1);
#endif
}


/****************/
/* replace with dconi_redisplay(), no longer used by dconi_multiply() */
void dconi_updatedcon()
/****************/
{
  dconi_reset();
  P_setreal(CURRENT,"vs2d",vs2d,0);
  run_dcon();
  dconi_mode = -1;	/* force cursor mode */
  dconi_cursor();
  Wsetgraphicsdisplay("dconi");
}

/********************/
void turnoff_dconi()
/********************/
{
  setdisplay();
  Wgmode();
  /* set cursors to zero to stop redraw them */
/*
  oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0;
*/
  Wturnoff_mouse();
  dconi_reset();
/* no necessary to EraseLabels */
/*
  EraseLabels();
*/
  endgraphics();
  exit_display();
  releaseAllWithId("dconi");
  D_allrelease();
  if (mark_textfptr!=NULL) {
    fclose(mark_textfptr);
    mark_textfptr=NULL;
  }
/*  Wsetgraphicsdisplay(""); */
}

#ifdef OLD
/*********************************/
static void dconi_redisplay_old()
/*********************************/
{
  int x,y,x1,y1,spos;
  char tmpstr[128];

/* mouse commands don't really help, since mouse is reactivated after
 execString(dcon) is sent, doesn't wait for dcon to finish */
  Wturnoff_mouse();
/*  x = oldx_cursor[0]; x1 = oldx_cursor[1]; y = oldy_cursor[0]; y1 = oldy_cursor[1]; */
/*  erase_dcon_box(); */
  update_cursor(0,0,0);
  update_cursor(1,0,0);
/*  execString("dconi\n"); */
  if (dconi_param_to_runstring() != 0)
  {
    execString("dcon('redisplay')\n");
/*    execString("dcon('noaxis')\n"); */
  }
  else
  {
    strcpy(tmpstr, dconi_runstring);
    spos = strlen( dconi_runstring );
    if (dconi_runstring[spos-1] == ')')
    {
      dconi_runstring[spos-1] = '\0';
      strcat(dconi_runstring, ",'redisplay')\n");
    }
    else
    {
      strcat(dconi_runstring, "('redisplay')\n");
    }
    execString( dconi_runstring );
    strcpy(dconi_runstring, tmpstr);
    spos = strlen( dconi_runstring );
    if (dconi_runstring[spos-1] != '\n')
      strcat(dconi_runstring, "\n");
  }
  if (getxy(&x,&y,&x1,&y1)) return 1;
  update_cursor(0,x,y);
  if (dconi_mode==BOX_MODE)
    update_cursor(1,x1,y1);
  activate_mouse(dconi_m_newcursor,dconi_reset);
}
#endif

/*********************************/
static void dconi_redisplay()
/*********************************/
{
  int x,y,x1,y1,spos;
  char tmpstr[128];

/* mouse commands don't really help, since mouse is reactivated after
 execString(dcon) is sent, doesn't wait for dcon to finish */
  Wturnoff_mouse();
/*  x = oldx_cursor[0]; x1 = oldx_cursor[1]; y = oldy_cursor[0]; y1 = oldy_cursor[1]; */
/*  erase_dcon_box(); */
  update_cursor(0,0,0);
  update_cursor(1,0,0);
/*  execString("dconi\n"); */
  if (strcmp(dconi_runstring,"")==0)
  {
    execString("dcon('redisplay')\n");
  }
  else
  {
    strcpy(tmpstr, dconi_runstring);
    spos = strlen( tmpstr );
    if (tmpstr[spos-2] == ')')
    {
      tmpstr[spos-2] = '\0';
      strcat(tmpstr, ",'redisplay')\n");
    }
    else
    {
      if (tmpstr[spos-1] == '\n')
        tmpstr[spos-1] = '\0';
      strcat(tmpstr, "('redisplay')\n");
    }
    execString( tmpstr );
  }
  if (getxy(&x,&y,&x1,&y1)) return;
  update_cursor(0,x,y);
  if (dconi_mode==BOX_MODE)
    update_cursor(1,x1,y1);
  activate_mouse(dconi_m_newcursor,dconi_reset);
}

void dconi_newVs(double newVs, char *ax1, char *ax2) {
    char axis1[8], axis2[8];
    getAxes(axis2, axis1, 7);
    if((strcasecmp(axis1,ax1) == 0 && strcasecmp(axis2,ax2) == 0)
	|| (strcasecmp(axis1,ax2) == 0 && strcasecmp(axis2,ax1) == 0) ) {

      vs2d = newVs;
      update_vs_pars();
      P_setreal(CURRENT,"vs2d",vs2d,0);
      dconi_redisplay();
    }
}

void dconi_sendVs() {
   int i, vps;
   double d;
   char cmd[MAXSTR], axis1[8], axis2[8];

    if (P_getreal(GLOBAL,"syncVs", &d, 1) != 0) d = 0.0;
    if(d < 0.5) return;

   if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
   else vps = (int)d;
   if(vps < 2) return;

   getAxes(axis2, axis1, 7);

   for(i=0; i<vps; i++) {
        if(i+1 != VnmrJViewId) {
          if (P_getreal(GLOBAL,"jvpexps", &d, i+1) != 0) d = 0.0;
          if(d < 0.5) continue;

          sprintf(cmd, "vnmrjcmd('CR %d trackCursor(\\'vs2d\\', %f, \\'%s\\', \\'%s\\')')\n", i+1, vs2d, axis1, axis2);
//Winfoprintf("dconi_sendVs %s\n", cmd);
          execString(cmd);
        }
   }
}

/*********************************/
static int dconi_multiply(int x, int y, int moveflag)
/*********************************/
{
  float vs_mult();
  if (axisonly)
     return(0);
  Wgmode();
  if (x>dfpnt+dnpnt)
    { setcolorth(x,y);
    }
  else if ((y>dfpnt2+dnpnt2) || (x<dfpnt))
   { switch (trace_mode)
     {case HPROJ_MODE:
      case TRACE_MODE: if ((x<dfpnt)||(x>=dfpnt+dnpnt)) break;
			if (vsproj <= 0.0) vsproj=1e-8;
                        vsproj *= vs_mult(x,y,mnumypnts,yoffset,
                                           dfpnt,dnpnt,spec1);
                        update_vs_pars();
                        if (trace_mode==TRACE_MODE)
                          dconi_newtrace(1,oldx_cursor[0],oldy_cursor[0],0);
                        else
                          update_hproj();
                        break;
      case VPROJ_MODE: if ((y<dfpnt2)||(y>=dfpnt2+dnpnt2)) break;
			if (vsproj <= 0.0) vsproj=1e-8;
                        vsproj *= vs_mult(y,mnumxpnts-x,mnumxpnts,xoffset,
                                           dfpnt2,dnpnt2,spec2);
                        update_vs_pars();
                        update_vproj();
                        break;
      default:		break;
     }
   }
   else
    { if ((x>=dfpnt) && (x<dfpnt+dnpnt) &&
         (y>=dfpnt2) && (y<dfpnt2+dnpnt2) && (!moveflag)) 
       {
#ifdef VNMRJ
         set_hourglass_cursor();
#endif
         dconi_get_new_vs(x,y);
	 P_setreal(CURRENT,"vs2d",vs2d,0);
/*         Wturnoff_mouse();
         dconi_mode = -1; */
#ifdef SUN
/*
 *       Use the following line if no conformation is desired
 */
/*       dconi_updatedcon();
         if (expandMacro[0] != '\0')
         {
           execString(expandMacro);
         }
	 execString("menu\n"); */

	 dconi_redisplay();
#else 
/*
 *       Use the following line if conformation is desired (VMS)
 */
/*         Wactivate_buttons(2,
           "Confirm Redraw",dconi_updatedcon,"Cancel Redraw",dconi_cursor,
           turnoff_dconi,"dconi");*/
#endif 
#ifdef VNMRJ
         restore_original_cursor();
#endif
       }
    }
  Wsetgraphicsdisplay("dconi");
  dconi_sendVs();
  return 0;
}

/********************/
void dconi_newdcon()
/********************/
{
  Wgmode();
  Wturnoff_mouse();
  dconi_reset();
  run_dcon();
  if (!d2flag && axisonly)
     setVertAxis();
  dconi_mode = -1; /* force dconi_cursor() into the CURSOR_MODE */
  dconi_cursor();
  Wsetgraphicsdisplay("dconi");
}

/****************/
static void hproj_max()
/****************/
{ dconi_hproj(0);
}

/****************/
static void hproj_sum()
/****************/
{ dconi_hproj(1);
}

/****************/
static void vproj_max()
/****************/
{ dconi_vproj(0);
}

/****************/
static void vproj_sum()
/****************/
{ dconi_vproj(1);
}

/**************************/
void dconi_redraw_trace()
/**************************/
{
  if (axisonly)
     return;
  Wshow_graphics();
  if (init_dconi(1)) {return;}
  if (!d2flag)
    { Werrprintf("no 2D data in data file");
      return;
    }
  if (trace_mode == TRACE_MODE)
    dconi_trace();
  else if (trace_mode == HPROJ_MODE)
  {
    if (proj_sumflag)
      hproj_sum();
    else
      hproj_max();
  }
  else if (trace_mode == VPROJ_MODE)
  {
    if (proj_sumflag)
      vproj_sum();
    else
      vproj_max();
  }
}

/***************/
static int run_dcon()
/***************/
{ extern int ll2d_draw_peaks();
  execString(dconi_runstring);
  if ((Wistek() || WisSunColor()) && leveldisplay_active && (dcon_num_colors>1))
    displaylevels(0);
  ll2d_draw_peaks();
  return 0;
}

/******************/
int init_dconi(int plotflag)
/******************/
{
  double start,len,axis_scl;

  if (init2d(1,plotflag)) return 1;
  discalib = (float)(mnumypnts-ymin) / wc2max;
  yoffset = dfpnt2 + dnpnt2 + mnumypnts / 50;
  xoffset = mnumxpnts - dfpnt + 8*xcharpixels;
  get_scale_pars(HORIZ,&start,&len,&axis_scl,&rev_h);
  get_scale_pars(VERT,&start,&len,&axis_scl,&rev_v);
  freq_h = get_axis_freq(HORIZ);
  freq_v = get_axis_freq(VERT);
  revAxis_h = 0;
  revAxis_v = 0;
  if ( freq_h )
  {
     revAxis_h = rev_h;
  }
  if ( freq_v )
  {
     revAxis_v = rev_v;
  }
  dconi_sendSpecInfo(1);
  start_from_ft2d = 0;
  Wsetgraphicsdisplay("dconi");
  aspFrame("clearAspSpec",0,0,0,0,0);
  return 0;
}

/*****************/
void erase_projections()
/*****************/
{
  int y;

  set_graph_clear_flag(TRUE);
  color(BACK);
  y = dfpnt2+dnpnt2+3;
  amove(dfpnt-1, y);
  box(dnpnt+2,mnumypnts - y);
  // box(dnpnt+2,mnumypnts-dfpnt2-dnpnt2-5);
  erase1 = 0;
  if (erase2)
    { amove(1,dfpnt2-1);
      box(mnumxpnts-(xoffset-mnumxpnts/80),dnpnt2+2);
      erase2 = 0;
    }
  set_graph_clear_flag(FALSE);
}



/************************/
int dconi(int argc, char *argv[], int retc, char *retv[])
/************************/
{ int update,x,y,x1,y1,argnum;
  int redisplay_param_flag, do_menu=FALSE;
  int calledWithRedisplay = 0;
  extern int ll2d_draw_peaks();
  char cmd[20];

/* Check for special menu commands :
	toggle, trace, expand, plot	*/

  (void) retc;
  (void) retv;

  if (!((argc == 2) && (strcmp(argv[1],"plot") == 0)))  {
    Wturnoff_buttons();
    set_turnoff_routine(turnoff_dconi);
    }
  Wgetgraphicsdisplay(cmd, 20);
  Wshow_graphics();
  if (init_dconi(1)) {return 1;}
  if (argc == 3)
  {
     if (strcmp(argv[1],"expandMacro") == 0)
     {
       strcpy(expandMacro, argv[2]);
       strcat(expandMacro, "\n");
       RETURN;
     }
     if (strcmp(argv[1],"traceMacro") == 0)
     {
       strcpy(traceMacro, argv[2]);
       strcat(traceMacro, "\n");
       RETURN;
     }
     if (strcmp(argv[1],"againMacro") == 0)
     {
       strcpy(againMacro, argv[2]);
       strcat(againMacro, "\n");
       RETURN;
     }
  }

  if(argc < 2 || strcmp(argv[1],"dpcon") == 0) traceMode = 0;

  if (argc == 2)  {
    if (strcmp(argv[1],"toggle") == 0)  {
      if (WgraphicsdisplayValid(argv[0]) && (argc>1))  {
        setButtonMode(SELECT_MODE);
        dconi_cursor();
        RETURN;
        }
      else  {
	Werrprintf("Must be in dconi to use %s option",argv[1]);
	ABORT;
	}
      }
    else if (strcmp(argv[1],"restart") == 0)  {
      if (WgraphicsdisplayValid(argv[0]) && (argc>1))  {
	dconi_mode = (dconi_mode == BOX_MODE) ? CURSOR_MODE : -1;
        dconi_cursor();
        RETURN;
        }
      else  {
	}
      }
    else if (strcmp(argv[1],"trace") == 0)  {
      if (WgraphicsdisplayValid(argv[0]) && (argc>1))  {
        dconi_trace();
        traceMode = TRACE_MODE;
#ifdef VNMRJ
        removeGraphSubFunc("dconi('hproj_max')");
        removeGraphSubFunc("dconi('hproj_sum')");
        addGraphSubFunc("dconi('trace')");
#endif
        RETURN;
        }
      else  {
	Werrprintf("Must be in dconi to use %s option",argv[1]);
	ABORT;
	}
      }
    else if (strcmp(argv[1],"expand") == 0)  {
      if (WgraphicsdisplayValid(argv[0]) && (argc>1))  {
        dconi_expand();
        RETURN;
        }
      else  {
	Werrprintf("Must be in dconi to use %s option",argv[1]);
	ABORT;
	}
      }
    else if (strcmp(argv[1],"plot") == 0)  {
      if (WgraphicsdisplayValid(argv[0]) && (argc>1))  {
        dconi_plot();
        RETURN;
        }
      else  {
	Werrprintf("Must be in dconi to use %s option",argv[1]);
	ABORT;
	}
      }
    else if (strcmp(argv[1],"hproj_max") == 0)  {
      if (WgraphicsdisplayValid(argv[0]) && (argc>1))  {
        hproj_max();
        traceMode = HPROJ_MODE;
        removeGraphSubFunc("dconi('trace')");
        removeGraphSubFunc("dconi('hproj_sum')");
        addGraphSubFunc("dconi('hproj_max')");
        RETURN;
        }
      else  {
	Werrprintf("Must be in dconi to use %s option",argv[1]);
	ABORT;
	}
      }
    else if (strcmp(argv[1],"hproj_sum") == 0)  {
      if (WgraphicsdisplayValid(argv[0]) && (argc>1))  {
        hproj_sum();
        traceMode = HPROJ_MODE;
        removeGraphSubFunc("dconi('trace')");
        removeGraphSubFunc("dconi('hproj_max')");
        addGraphSubFunc("dconi('hproj_sum')");
        RETURN;
        }
      else  {
	Werrprintf("Must be in dconi to use %s option",argv[1]);
	ABORT;
	}
      }
    else if (strcmp(argv[1],"vproj_max") == 0)  {
      if (WgraphicsdisplayValid(argv[0]) && (argc>1))  {
        vproj_max();
        traceMode = VPROJ_MODE;
        removeGraphSubFunc("dconi('vproj_sum')");
        addGraphSubFunc("dconi('vproj_max')");
        RETURN;
        }
      else  {
	Werrprintf("Must be in dconi to use %s option",argv[1]);
	ABORT;
	}
      }
    else if (strcmp(argv[1],"vproj_sum") == 0)  {
      if (WgraphicsdisplayValid(argv[0]) && (argc>1))  {
        vproj_sum();
        traceMode = VPROJ_MODE;
        removeGraphSubFunc("dconi('vproj_max')");
        addGraphSubFunc("dconi('vproj_sum')");
        RETURN;
        }
      else  {
	Werrprintf("Must be in dconi to use %s option",argv[1]);
	ABORT;
	}
      }
    else  {
      if ((strcmp(argv[1],"dcon") != 0) &&
          (strcmp(argv[1],"dpcon") != 0) &&
          (strcmp(argv[1],"ds2d") != 0) &&
          (strcmp(argv[1],"again") != 0) &&
          (strcmp(argv[1],"redisplay") != 0) &&
          (strcmp(argv[1],"restart") != 0))  {
	Werrprintf("Illegal dconi option : %s",argv[1]);
	ABORT;
	}
      }
    }
    if (argc == 1)
    {
      expandMacro[0] = '\0';
      traceMacro[0] = '\0';
      againMacro[0] = '\0';
    }
    
/*  Close `mark2d.out' if it happens to be open.  */

  if (mark_textfptr!=NULL) {
    fclose(mark_textfptr);
    mark_textfptr=NULL;
  }

/*  Redisplay Parameters?  */

  redisplay_param_flag = 0;
  proj_sumflag = 0;
  if (argc > 1)
    if (strcmp( argv[ argc-1 ], "redisplay parameters" ) == 0) {
            argc--;
       calledWithRedisplay = 1;

/*  autoRedisplay (in lexjunk.c) is supposed
    to insure the next test returns TRUE.	*/

	    if (WgraphicsdisplayValid( argv[ 0 ] ))
              redisplay_param_flag = 1;
    }
#ifdef VNMRJ
    if (isJprintMode())
        old_dconi_mode = dconi_mode;
#endif

/*  For now, redisplay parameters in dconi is a formality, as the
    previous programming worked fine when the command was reexecuted.	*/


/*  If dconi was called from ft2d then update will be true.  For ft2d
    sets the current display command to "dconi". By putting "dconi" on
    the redisplay list (see lexjunk.c), it insures that argc > 1.	*/

  update = (WgraphicsdisplayValid(argv[0]) && (argc>1));

/*  The first argument to dconi can be the name of a display command -
    "dcon", "dpcon" or "ds2d"; the word "again" or the word "dconi".
    The last instructs dconi to use the value of the parameter dconi
    in preparing a display commmand.					*/

  if (update)
    { if ((strcmp(argv[1],"dcon")==0) || (strcmp(argv[1],"dpcon")==0) ||
          (strcmp(argv[1],"ds2d")==0) || (strcmp(argv[1],"again")==0) ||
          (strcmp(argv[1],"redisplay")==0) || (strcmp(argv[1],"dconi") == 0))
         update = 0;
      else if ((strcmp(argv[1],"cr")!=0) && (strcmp(argv[1],"delta")!=0) &&
          (strcmp(argv[1],"cr1")!=0) && (strcmp(argv[1],"delta1")!=0) &&
          (strcmp(argv[1],"cr2")!=0) && (strcmp(argv[1],"delta2")!=0) &&
          (strcmp(argv[1],"restart")!=0) && ! redisplay_param_flag)
        {restore_original_cursor(); return 0;}
    }
  else if ((argc>1) && (strcmp(argv[1],"restart")==0))
    {
      update = (WgraphicsdisplayValid("dcon") ||
                WgraphicsdisplayValid("dpcon") ||
                WgraphicsdisplayValid("ds2d"));
       if (!update)
       { Werrprintf("invalid display for restart option");
	 restore_original_cursor();
         return 1;
       }
       else
       {
          leveldisplay_active = WgraphicsdisplayValid("dcon");
       }
    }
  grf_batch(1);

  if (!update)
    { if (!((argc==2) && (strcmp(argv[1],"again")==0)))
        {
          if (((argc==2) && (strcmp(argv[1],"redisplay")==0)))
              { dconi_redisplay();  grf_batch(0); return 1; }

/*  Program comes here if the argument count is not 2
    or if the first argument is not "again"		*/

          argnum = 2;
          axisonly = 0;

    /* If the first argument to dconi is the name of a display command
       or the word "dconi", then prepare a new value for dconi_runstring  */

          if ((argc>=2) && (strcmp(argv[1],"dcon")==0))
            { strcpy(dconi_runstring,"dcon");
              leveldisplay_active = 1;
            }
          else if ((argc>=2) && (strcmp(argv[1],"dpcon")==0))
            { strcpy(dconi_runstring,"dpcon");
              leveldisplay_active = 0;
            }
          else if ((argc>=2) && (strcmp(argv[1],"ds2d")==0))
            { strcpy(dconi_runstring,"ds2d");
              leveldisplay_active = 0;
            }
          else if ((argc>=2) && (strcmp(argv[1],"dconi")==0))
            {
              if (dconi_param_to_runstring() != 0)
	      {
		restore_original_cursor();
                grf_batch(0);
                ABORT;
	      }
              argnum = argc;            /* Since we want dconi to ignore the */
                                        /* rest of its arguments here insure */
                                        /* the program below where those     */
                                        /* arguments are appended to the     */
            }                           /* dconi runstring is skipped        */
          else if (argc < 2)
            {
              if (dconi_param_to_runstring() != 0)
	      {
		restore_original_cursor();
                grf_batch(0);
                ABORT;
	      }
              argnum = argc;
            }
          else 
            { strcpy(dconi_runstring,"dpcon");
              leveldisplay_active = 0;
              argnum = 1;
            }


    /* See note above, where we test for argv[ 1 ] == "dconi"  */

          if ( (argc>argnum) && !  calledWithRedisplay )
            { strcat(dconi_runstring,"(");
              while (argc>argnum)
                { strcat(dconi_runstring,"'");
                  strcat(dconi_runstring,argv[argnum]);
                  if ( ! strcmp(argv[argnum],"axisonly") )
                     axisonly = 1;
                  strcat(dconi_runstring,"'");
                  argnum++;
                  if (argc>argnum)
                    strcat(dconi_runstring,",");
                }
              strcat(dconi_runstring,")");
            }
          strcat(dconi_runstring,"\n");
          if (!d2flag && !axisonly)
            {
              Wsetgraphicsdisplay(cmd);
              Werrprintf("no 2D data in data file");
              ABORT;
            }
          if (!d2flag && axisonly)
            setVertAxis();
        }
/* else */ /* ((argc==2) && (strcmp(argv[1],"again")==0)) */
/* {} */

    /*  Check status returned by executing the "dconi_runstring"  */

      skip_save_graphics_cmd(1);
      if (execString(dconi_runstring)) {
          // restore_original_cursor();
          skip_save_graphics_cmd(0);
          return 1;
      }
      if (!d2flag)
        { Werrprintf("no 2D data in data file");
          return 1;
        }
    }
  else if ((argc>1) && (strcmp(argv[1],"restart")==0))
    {
       update = (WgraphicsdisplayValid(argv[0]) && (argc>1));
    }
  oldx_cursor[0] = oldx_cursor[1] = oldy_cursor[0] = oldy_cursor[1] = 0;
  project_buffer = 0;
  spec1 = 0;
  spec2 = 1;
  next  = 2;
  plot_spec = 3;
  init_display();
  ResetLabels();
  erase1 = 0;
  erase2 = 0;
  plot_erase = 0;
  last_trace = -1;
  last_vs  = 0;
/*  if (!update)	*/
/*    vsproj  = vs;	*/
  skip_save_graphics_cmd(0);
  if (getxy(&x,&y,&x1,&y1)) {
      return 1;
  }   
  Wgmode();
  disp_status("      ");
  erase1 = 0;
  erase2 = 0;

  if (calledWithRedisplay == 0)
     erase_projections();
  if ((Wistek() || WisSunColor()) && leveldisplay_active && (dcon_num_colors>1))
    displaylevels(0);
  if (!update && !((argc==2) && (strcmp(argv[1],"again")==0)))  {
    ll2d_draw_peaks();
    if (P_setstring(GLOBAL,"crmode","c",0))
      Werrprintf("Unable to set variable \"crmode\".");
    do_menu = TRUE;
    }
  else if (!((argc>1) && (strcmp(argv[1],"restart")==0)))  {
    ll2d_draw_peaks();
    }
  if (update)
    { if (dconi_mode==BOX_MODE)
        { dconi_mode=CURSOR_MODE; 	// force to switch to BOX_MODE 
          trace_mode=NOTRACE_MODE;
          dconi_cursor();
        }
      else if (trace_mode==TRACE_MODE)
        { dconi_trace();
        }
      else
        { dconi_mode = BOX_MODE;  	// force to switch to CURSOR_MODE 
          trace_mode=NOTRACE_MODE;
          dconi_cursor();
        }
    }
  else
    { dconi_mode = BOX_MODE;	  	// force to switch to CURSOR_MODE
      trace_mode=NOTRACE_MODE;
      dconi_cursor();
    }
  if (argc>1 && strcmp(argv[1],"again") == 0 && againMacro[0] != '\0')  {
    if (WgraphicsdisplayValid(argv[0]) && (argc>1))  {
      skip_save_graphics_cmd(1);
      execString(againMacro);
      skip_save_graphics_cmd(0);
      grf_batch(0);
      RETURN;
      }
    else  {
      Werrprintf("Must be in dconi to use %s option",argv[1]);
      grf_batch(0);
      ABORT;
      }
    }
  releasevarlist();
  Wsetgraphicsdisplay("dconi");
  if (do_menu && !Bnmr)
  {
    if (axisonly)
       execString("menu('dconi_ao')\n");
    else
       execString("menu('dconi')\n");
  }
  set_turnoff_routine(turnoff_dconi);
  restore_original_cursor();
  grf_batch(0);
  dconi_sendVs();
  aspFrame("dconi",0,0,0,0,0);
  return 0;
}

/**************/
void update_vs_pars()
/**************/
{ int c;

  if (trace_mode!=NOTRACE_MODE)
    c = -PARAM_COLOR;
  else
    c = PARAM_COLOR;
  DispField2(FIELD5,c,vsproj,1);
  DispField2(FIELD6,-PARAM_COLOR,vs2d,1);
  P_setreal(CURRENT,"vsproj",vsproj,0);
}

/*************/
static void displaylevels( int erase_text )
/*************/
{ int i,labelstep;
  char t[16];

  level_y0 = mnumypnts/6; 			/* height above bottom */
  level_y1 = mnumypnts/(1.4 * dcon_num_colors);	/* height of each block */
  level_x1 = mnumxpnts/80;			/* width of block */

  level_first = 0;
  labelstep = 1;
  if (dcon_colorflag==SELECT_PH_COLOR)
    level_first = - dcon_num_colors / 2;
  if (dcon_num_colors>20) labelstep = 10;

  Wgmode();
  /* erase area behind level display */
  color(BACK);
  if (erase_text == 1)
  {
/*  amove(mnumxpnts-level_x1-2,level_y0-1);
    box(level_x1+1,dcon_num_colors*level_y1+2); */
  }
  else
  {
    amove(mnumxpnts-level_x1-2-2*xcharpixels,level_y0-1);
    box(level_x1+2*xcharpixels+1,dcon_num_colors*level_y1+2);
  }
  /* draw the color bar */
  color(SCALE_COLOR);
  amove(mnumxpnts-level_x1-2,level_y0-1);
  rdraw(level_x1+1,0);
  rdraw(0,dcon_num_colors*level_y1+1);
  rdraw(-level_x1-1,0);
  rdraw(0,-dcon_num_colors*level_y1-1);
  /**
  if (Wissun())
    grf_batch(1);
  **/
  for (i=0; i<dcon_num_colors; i++)
     {   if (level_first>=0)
           { if ((i>=th) || WisSunColor())
               color(i+dcon_first_color);
             else
               color(BLACK);
           }
         else
           { if ((i<=(dcon_num_colors>>1)+th) && (i>=(dcon_num_colors>>1)-th) &&
                 !WisSunColor())
               color(BLACK);
             else
               color(i+dcon_first_color);
           }
         if ((dcon_colorflag==SELECT_GRAY_COLOR) && !WisSunColor())
           { grayscale_box(mnumxpnts-level_x1-1,level_y0+level_y1*i,
               level_x1,level_y1,i);
           }
         else
           { amove(mnumxpnts-level_x1-1,level_y0+level_y1*i);
             box(level_x1,level_y1);
           }
     }
  color(SCALE_COLOR);
  if (erase_text != 1)
    for (i=0; i<dcon_num_colors; i += labelstep)
      { sprintf(t,"%3d",i+level_first);
        amove(mnumxpnts-level_x1-2-3*xcharpixels,
                     level_y0+level_y1*i+(level_y1-ycharpixels)+2);
        dstring(t);
      }
  /**
  if (Wissun())
    grf_batch(0);
  **/
}

/********************/
static void setcolorth(int x, int y)
/********************/
{ int newth;
  if (!leveldisplay_active)
    { Werrprintf("Level bar not active in this display");
      return;
    }
  if (!WisSunColor())
    { Werrprintf("Level bar not active in this display");
      return;
    }
  if (x>mnumxpnts-1) return;
  newth = ((y-level_y0) / level_y1);
  if ((newth<0) || (newth >= dcon_num_colors)) return; 
  
  newth += level_first;
  if (newth<0)
    newth = -newth;
  if (dcon_colorflag==SELECT_GRAY_COLOR)
    {  graycntr = (double) newth;
       if (graycntr > NUM_GRAY_COLORS) graycntr = NUM_GRAY_COLORS;  
       if (graycntr < 0.5) graycntr = 0.5;
       change_contrast(graycntr,graysl);
       if (gray_p_flag) P_setreal(CURRENT,"grayctr",graycntr,0);
    }
  else
    { if (level_first>=0)
        { if (newth > MAX_AV_THRESH)
             newth = MAX_AV_THRESH;
        }
      else
        { if (newth > MAX_PH_THRESH)
             newth = MAX_PH_THRESH;
        }
      setcolormap(dcon_first_color,dcon_num_colors,newth,
        dcon_colorflag==SELECT_PH_COLOR);
      th = (double) newth;
      P_setreal(CURRENT,"th",th,0);
    }

  grf_batch(1);
  displaylevels(1);
  dconi_redisplay();
  normalmode();
  restore_original_cursor();
  grf_batch(0);
}

void set_gray_slope(int x, int y)
{  int yy;
   int newsl;

   (void) x;
   newsl = ((y-level_y0) / level_y1);
   yy = newsl-(int) graycntr;
   if (yy < 0) return;
   if (yy == 0) graysl = NUM_GRAY_COLORS;
    else graysl = NUM_GRAY_COLORS/(2.0*yy);
   if (graysl < 0.5) graysl = 0.5;
   change_contrast(graycntr,graysl);
   if (gray_p_flag)  P_setreal(CURRENT,"graysl",graysl,0);
}
/******************************************/
void setcolormap(int firstcolor, int numcolors, int th, int phcolor)
/******************************************/
{ int i;
  if (!phcolor)
    { 
#ifdef VNMRJ
        reset_dcon_color_threshold(firstcolor + th+1, 0);
#endif
        for (i=0; i<numcolors; i++)
           change_color(firstcolor+i,i>th);
    }
  else
    { 
        reset_dcon_color_threshold(firstcolor+(numcolors>>1)+th+1, firstcolor +
             (numcolors>>1)-th-1);
        for (i=0; i<numcolors; i++)
           change_color(firstcolor+i,
             (i<(numcolors>>1)-th) || (i>(numcolors>>1)+th));
    }
  refresh_graf();           /* some window systems needs refresh */
}

/*  This program was adapted from "parse_acqproc_queue" in acqhwcmd.c
    Eventually the two could be combined into one program, with the
    delimiter character provided as an argument.			*/

static char *parse_dconi_value(char *dconi_addr, char *token_addr, int token_len )
{
	int	 cur_len;
	char	*next_token;

	if (dconi_addr == NULL)			/* This should not happen */
	  return( NULL );

	if (*dconi_addr == '\0')		/* Then there are no more tokens */
	  return( NULL );

	next_token = strchr( dconi_addr, ',' );
	if (next_token == NULL)
	  cur_len = strlen( dconi_addr );
	else
	  cur_len = next_token - dconi_addr;

	if (cur_len >= token_len) {
		strncpy( token_addr, dconi_addr, token_len-1 );
		token_addr[ token_len-1 ] = '\0';
	}
	else {
		strncpy( token_addr, dconi_addr, cur_len );
		token_addr[ cur_len ] = '\0';
	}

/*  Next token will be non-NULL if-and-only-if another token is present.
    If so, add one to next token to skip the comma character.
    If not, add the length of the current token so next token
    becomes the address of the terminating NUL character.

    Return address of the next token.					*/

	if (next_token != NULL)
	  next_token++;
	else
	  next_token = dconi_addr + cur_len;

	return( next_token );
}

/*  This program accesses two global static variables,
    dconi_runstring and leveldisplay_active.		*/

static int dconi_param_to_runstring()
{
	int	 first_time_thru, ival;
	char	 dconi_value[ MAXPATHL ], element[ MAXPATHL ];
	char	*dconi_addr;

	ival = P_getstring(
	  CURRENT, "dconi", &dconi_value[ 0 ], 1, sizeof( dconi_value )
	);
	if (ival != 0)
	  strcpy( &dconi_value[ 0 ], "dpcon" );
	else if ((int)strlen( &dconi_value[ 0 ] ) < 1)
	  strcpy( &dconi_value[ 0 ], "dpcon" );

/*  Get the command dconi is to run.  */

	dconi_addr = parse_dconi_value(
	  &dconi_value[ 0 ], &element[ 0 ], sizeof( element )
	);

/*  Command must be one of "dcon", "dpcon", "ds2d, "dconn", dpconn" or "ds2dn".  */

	if ((strcmp( &element[ 0 ], "dcon" ) != 0) &&
	    (strcmp( &element[ 0 ], "dpcon" ) != 0) &&
	    (strcmp( &element[ 0 ], "ds2d" ) != 0) &&
	    (strcmp( &element[ 0 ], "dconn" ) != 0) &&
	    (strcmp( &element[ 0 ], "dpconn" ) != 0) &&
	    (strcmp( &element[ 0 ], "ds2dn" ) != 0)) {
		Werrprintf(
    "Value of dconi parameter does not have a valid display command"
		);
		Wscrprintf( "dconi parameter: '%s'\n", &dconi_value[ 0 ] );
		Wscrprintf(
    "command must be 'dcon', 'dpcon', 'ds2d', 'dconn', 'dpconn' or 'ds2dn'\n"
		);
		return( -1 );
	}
	else
	  strcpy( dconi_runstring, &element[ 0 ] );

/*  Now set 'leveldisplay_active' based on the command dconi will run.  */

	if (strcmp( &element[ 0 ], "dcon" ) == 0)
	  leveldisplay_active = 1;
	else
	  leveldisplay_active = 0;

/*  If nothing else in the dconi parameter but the command, return now.  */

	if (dconi_addr == NULL)			/* Sanity check, not expected */
	  return( 0 );
	if (*dconi_addr == '\0')
	  return( 0 );

	first_time_thru = 262143;
	while ((dconi_addr = parse_dconi_value(
			dconi_addr, &element[ 0 ], sizeof( element )
	)) != NULL) {
		if (first_time_thru) {
			strcat( dconi_runstring, "(" );
			first_time_thru = 0;
		}
		else
		  strcat( dconi_runstring, "," );

		strcat( dconi_runstring, "'" );
		strcat( dconi_runstring, &element[ 0 ] );
		strcat( dconi_runstring, "'" );
	}

/* Sanity check, first time thru should be zero */

	if (first_time_thru == 0)
	  strcat( dconi_runstring, ")" );
	return( 0 );
}

void getAxes(char *ax2, char *ax1, int len)
{
   if(revflag) {
     get_nuc_name(HORIZ, ax1, len);     
     get_nuc_name(VERT, ax2, len);     
   } else {
     get_nuc_name(HORIZ, ax2, len);     
     get_nuc_name(VERT, ax1, len);     
   }
}

void dconi_sendCursor(double c1, double c2, 
	double d1, double d2, int mode)
{
   int i, vps;
   double d, f1, f2;
   char cmd[MAXSTR], trace[5], axis1[8], axis2[8];

    if (P_getreal(GLOBAL,"trackCursor", &d, 1) != 0) d = 0.0;
    if(d < 0.5) return;

   if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
   else vps = (int)d;
   if(vps < 2) return;

    if ( ! freq_h )
      f2 = c2 - sp;
    else if (revAxis_h)
      f2 = sp - c2;
    else
      f2 = wp + sp - c2;
    if ( ! freq_v )
      f1 = c1 - sp1;
    else if (revAxis_v)
      f1 = sp1 - c1;
    else
      f1 = wp1 + sp1 - c1;

    if(f2 < 0.5 && f1 < 0.5) return; 
    if(f2 > wp && f1 > wp1) return; 

   if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");
   getAxes(axis2, axis1, 7);
   c2 = Hz2ppm(HORIZ, c2);
   d2 = Hz2ppm(HORIZ, d2);
   c1 = Hz2ppm(VERT, c1);
   d1 = Hz2ppm(VERT, d1);

   for(i=0; i<vps; i++) {
	if(i+1 != VnmrJViewId) {
          if (P_getreal(GLOBAL,"jvpexps", &d, i+1) != 0) d = 0.0;
          if(d < 0.5) continue;

	  if(trace_mode == TRACE_MODE) mode = trace_mode;
	  if(strstr(trace, "2") != NULL) {
	     sprintf(cmd, "vnmrjcmd('CR %d trackCursor(\\'cursor\\', %f, %f, %f, %f, %d, \\'%s\\', \\'%s\\', \\'%s\\')')\n", i+1, c1, c2, d1, d2, mode, trace, axis1, axis2); 
	  } else {
	     sprintf(cmd, "vnmrjcmd('CR %d trackCursor(\\'cursor\\', %f, %f, %f, %f, %d, \\'%s\\', \\'%s\\', \\'%s\\')')\n", i+1, c1, c2, d1, d2, mode, trace, axis2, axis1); 
	  }

 	  execString(cmd);

	  //writelineToVnmrJ("CR ", cmd);
	}
   }

}

void dconi_newCursor4freq(int num, double c1, double c2, 
	double d1, double d2, int mode, char *tr, char *ax1, char *ax2)
{
/* if cursor is from a 1d spectrum, only c1, d1 are passed */

      int x, y, x1, y1, m;
      double d, f1, f2, cursor1, dt1;
      char trace[5], axis1[8], axis2[8];

/* return if cursorTracking is off */
    if (P_getreal(GLOBAL,"trackCursor", &d, 1) != 0) d = 0.0;
    if(d < 0.5) return;

   if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");

   getAxes(axis2, axis1, 7);

   m = mapAxis(num, &c1, &c2, &d1, &d2, trace, axis1, axis2, tr, ax1, ax2);
   if(m == 2) {
      if(strstr(trace,"2") != NULL) {
   	cursor1 = ppm2Hz(HORIZ, c2);
   	dt1 = ppm2Hz(HORIZ, d2);
   	c2 = ppm2Hz(VERT, c1);
   	d2 = ppm2Hz(VERT, d1);
	c1 = cursor1;
	d1 = dt1;
      } else {
   	c2 = ppm2Hz(VERT, c2);
   	d2 = ppm2Hz(VERT, d2);
   	c1 = ppm2Hz(HORIZ, c1);
   	d1 = ppm2Hz(HORIZ, d1);
      }
   } else if(m == 1 && strlen(axis1) == 0) {
      if(strstr(trace,"2") != NULL) {
   	c1 = ppm2Hz(HORIZ, c2);
   	d1 = ppm2Hz(HORIZ, d2);
 	c2 = cr1;
	d2 = delta1;	
      } else {
   	c2 = ppm2Hz(VERT, c2);
   	d2 = ppm2Hz(VERT, d2);
 	c1 = cr;
	d1 = delta;	
      }
   } else if(m == 1 && strlen(axis2) == 0) {
      if(strstr(trace,"2") != NULL) {
   	c2 = ppm2Hz(VERT, c1);
   	d2 = ppm2Hz(VERT, d1);
 	c1 = cr;
	d1 = delta;	
      } else {
   	c1 = ppm2Hz(HORIZ, c1);
   	d1 = ppm2Hz(HORIZ, d1);
 	c2 = cr1;
	d2 = delta1;	
      }
   } else { 
	return;
   }

    if(c2 == cr1 && c1 == cr && d2 == delta1 && d1 == delta) 
	return;

    m = mode;	

    if(m == TRACE_MODE) {
      trace_mode = TRACE_MODE;
      dconi_mode = CURSOR_MODE;
    } else {
        trace_mode = NOTRACE_MODE;
	dconi_mode = mode;
    }

    getFreq(HORIZ, c1, sp, wp, &f1, NULL);
    getFreq(VERT, c2, sp1, wp1, &f2, NULL);
/*
    if(f2 < 0.5 || f1 < 0.5) return;
    if(f2 > wp1 || f1 > wp) return;

    cr = c1;
    cr1 = c2;
    delta = d1;
    delta1 = d2;

*/
    if(f2 < 0.5 || f2 > wp1) {
        //return;
    } else {
       cr1 = c2;
       delta1 = d2;
    }
    if(f1 < 0.5 || f1 > wp) {
        //return;
    } else {
       cr = c1;
       delta = d1;
    }


  getFreq(HORIZ, cr, sp, wp, &f1, &x);
  getFreq(VERT, cr1, sp1, wp1, &f2, &y);
  x1 = (int)((double)x     + (double)dnpnt  * delta  / wp + 0.5);
  y1 = (int)((double)y     + (double)dnpnt2 * delta1 / wp1 + 0.5);
/*
    if(c2 == cr && c1 == cr1 && d2 == delta && d1 == delta1) 
	return;

    m = mode;	

    if(m == TRACE_MODE) {
      trace_mode = TRACE_MODE;
      dconi_mode = CURSOR_MODE;
    } else {
        trace_mode = NOTRACE_MODE;
	dconi_mode = mode;
    }

    f1 = (rev_v) ? (c1 - sp1) : (wp1 + sp1 - c1);
    f2 = (rev_h) ? (c2 - sp) : (wp + sp - c2);
    if(f2 < 0.5 || f2 > wp) {
	//return;
    } else {
       cr = c2;
       delta = d2;
    }
    if(f1 < 0.5 || f1 > wp1) {
	//return;
    } else {
       cr1 = c1;
       delta1 = d1;
    }

  freq_val = (rev_h) ? (cr - sp) : (wp + sp - cr);
  x  = (int)((double)dfpnt  + (double)dnpnt  * freq_val / wp + 0.5);
  freq_val = (rev_v) ? (cr1 - sp1) : (wp1 + sp1 - cr1);
  y  = (int)((double)dfpnt2 + (double)dnpnt2 * freq_val / wp1 + 0.5);
  x1 = (int)((double)x     + (double)dnpnt  * delta  / wp + 0.5);
  y1 = (int)((double)y     + (double)dnpnt2 * delta1 / wp1 + 0.5);
*/
  if (dconi_mode==BOX_MODE)
    {
      idelta  = x1 - x;
      idelta1 = y1 - y;
      if (x1>dfpnt+dnpnt)
        x1=dfpnt+dnpnt;
    }

  if(trace_mode == TRACE_MODE) dconi_newtrace(1,x,y,0);
  else dconi_newcursor(1,x,y,0);

  if (dconi_mode==BOX_MODE)
    {
      dconi_newcursor(3,x1,y1,0);
      //disp_status("BOX   ");
    }
  else
    {
      update_cursor(1,0,0);
      //disp_status("CURSOR");
    }
}

/* the following are for crosshair */

/*************************************/
static void
update_crosshair(int x, int y, double f2, double f1)
/*************************************/
{
/* new functions in graphicas_win to draw crosshair */
/*
fprintf(stderr, "dconi update_crosshair %d %d %d %f\n", y, dfpnt2,dnpnt2, f1);
fprintf(stderr, "dconi update_crosshair %d %d %d %f\n", x, dfpnt,dnpnt, f2);
*/
/*
    y_crosshair(y, dfpnt, dnpnt, f1);
    x_crosshair(x, dfpnt2,dnpnt2, f2, 0, 0);
*/
    //y_crosshair(y, 1, mnumxpnts-right_edge, f1);
    y_crosshair(y, 1, mnumxpnts-4, f1);
    x_crosshair(x, 1, mnumypnts, f2, 0, 0);
}

void dconi_newCrosshair(int x, int y)
{
    double c1, c2, f1, f2;
    
      if ( ! rev_h)
         c2  = - (x - dfpnt ) * wp  / dnpnt  + wp  + sp;
      else if ( revAxis_h )
         c2  = (x - dfpnt - dnpnt ) * wp  / dnpnt  + sp;
      else
         c2 = (x - dfpnt) * wp  / dnpnt  + sp;
      if ( ! rev_v)
         c1 = - (y - dfpnt2) * wp1 / dnpnt2 + wp1 + sp1;
      else if ( revAxis_v )
         c1  = (y - dfpnt2 - dnpnt2 ) * wp1  / dnpnt2  + sp1;
      else
         c1 =   (y - dfpnt2) * wp1 / dnpnt2 + sp1;

    getFreq(HORIZ, c2, sp, wp, &f2, NULL);
    getFreq(VERT, c1, sp1, wp1, &f1, NULL);
    if(f1 < 0.5 || f2 < 0.5 || f2 > wp || f1 > wp1) {
	m_noCrosshair();
	return;
    }

	f2 = Hz2ppm(HORIZ, c2);
	f1 = Hz2ppm(VERT, c1);

	update_crosshair(x, y, convert4ppm(HORIZ, f2), convert4ppm(VERT, f1));

      dconi_sendCrosshair(c1, c2, f1, f2);
}

void dconi_sendCrosshair(double c1, double c2, double f1, double f2)
{
/* if is active vp, call trackCursor('freq',c1, c2) of other vps */

   int i, vps;
   double d, freq1, freq2;
   char cmd[MAXSTR], trace[5], axis1[8], axis2[8];

    if (P_getreal(GLOBAL,"trackCursor", &d, 1) != 0) d = 0.0; 
    if(d < 0.5) return;

    if (P_getreal(GLOBAL,"crosshair", &d, 1) != 0) d = 0.0;
    if(d < 0.5) return;

    if (P_getreal(GLOBAL,"overlayMode", &d, 1) != 0) d = 0.0;
    if((int)d >= OVERLAID_ALIGNED) return;

   if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
   else vps = (int)d;
   if(vps < 2) return;

    getFreq(HORIZ, c2, sp, wp, &freq2, NULL);
    getFreq(VERT, c1, sp1, wp1, &freq1, NULL);
    if(freq2 < 0.5 && freq1 < 0.5) return; 
    if(freq2 > wp && freq1 > wp1) return;

   if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");

   getAxes(axis2, axis1, 7);

   c2 = Hz2ppm(HORIZ, c2);
   c1 = Hz2ppm(VERT, c1);

   for(i=0; i<vps; i++) {
        if(i+1 != VnmrJViewId) {
          if (P_getreal(GLOBAL,"jvpexps", &d, i+1) != 0) d = 0.0;
          if(d < 0.5) continue;

	  if(strstr(trace, "2") != NULL) {
             sprintf(cmd, "vnmrjcmd('CR %d trackCursor(\\'crosshair\\', %f, %f, %f, %f, \\'%s\\', \\'%s\\', \\'%s\\')')\n", i+1, c1, c2, f1, f2, trace, axis1, axis2);
	  } else {
             sprintf(cmd, "vnmrjcmd('CR %d trackCursor(\\'crosshair\\', %f, %f, %f, %f, \\'%s\\', \\'%s\\', \\'%s\\')')\n", i+1, c1, c2, f1, f2, trace, axis2, axis1);
	  }
          execString(cmd);

          //writelineToVnmrJ("CR ", cmd);
        }
   }
}

void dconi_newCrosshair4freq(int num, double c1, double c2, 
	double f1, double f2, char *tr, char *ax1, char *ax2)
{
/* if cursor is from a 1d spectrum, only c1 is passed */

      int x, y, m;
      double x1, y1;
      double d;
      double freq1 = 0.0;
      double freq2 = 0.0;
      char trace[5], axis1[8], axis2[8];

/* return if cursorTracking is off */
    if (P_getreal(GLOBAL,"crosshair", &d, 1) != 0) d = 0.0;
    if(d < 0.5) return;

   if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");

   getAxes(axis2, axis1, 7);

   m = mapAxis(num, &c1, &c2, &f1, &f2, trace, axis1, axis2, tr, ax1, ax2);

        x1 = 0;
        x = 0;
        y1 = 0;
        y = 0;

   if(m == 2) {
      if(strstr(trace,"2") != NULL) {
   	freq1 = ppm2Hz(HORIZ, c2);
   	freq2 = f2;
   	c2 = ppm2Hz(VERT, c1);
   	f2 = f1;
	c1 = freq1;
	f1 = freq2;
      } else {
   	c2 = ppm2Hz(VERT, c2);
   	c1 = ppm2Hz(HORIZ, c1);
      }
           getFreq(HORIZ, c1, sp, wp, &x1, &x);
	   freq1 = f1;

           getFreq(VERT, c2, sp1, wp1, &y1, &y);
	   freq2 = f2;

   } else if(m == 1 && strlen(axis1) == 0) {
      if(strstr(trace,"2") != NULL) {
   	c1 = ppm2Hz(HORIZ, c2);
   	f1 = f2;
           getFreq(HORIZ, c1, sp, wp, &x1, &x);
 	   freq1 = f1;
      } else {
   	c2 = ppm2Hz(VERT, c2);
           getFreq(VERT, c2, sp1, wp1, &y1, &y);
           freq2 = f2;
      }
   } else if(m == 1 && strlen(axis2) == 0) {
      if(strstr(trace,"2") != NULL) {
   	c2 = ppm2Hz(VERT, c1);
   	f2 = f1;
           getFreq(VERT, c2, sp1, wp1, &y1, &y);
           freq2 = f2;
      } else {
   	c1 = ppm2Hz(HORIZ, c1);
           getFreq(HORIZ, c1, sp, wp, &x1, &x);
 	   freq1 = f1;
      }
   } else { 
	return;
   }
   update_crosshair(x, y, convert4ppm(HORIZ, freq1), convert4ppm(VERT, freq2));
}

void dconi_noCrosshair() {
   update_crosshair(0, 0, 0, 0);
}

void dconi_setInsetCursor(int x0, int y0, int x1, int y1)
{ 
  	y0 = mnumypnts - y0 -1;
  	y1 = mnumypnts - y1 -1;

 	idelta  = x1 - x0;
        idelta1 = y0 - y1;

      if ( ! rev_h) {
         cr  = - (x0 - dfpnt ) * wp  / dnpnt  + wp  + sp;
	 delta = - (x1 - dfpnt ) * wp  / dnpnt  + wp + sp;
         delta = cr - delta;
      } else if ( revAxis_h ) {
         cr  = (x0 - dfpnt - dnpnt ) * wp  / dnpnt  + sp;
	 delta = (x1 - dfpnt - dnpnt ) * wp  / dnpnt  + sp;
         delta = delta - cr;
      } else {
         cr  =   (x0 - dfpnt ) * wp  / dnpnt  + sp;
	 delta = + (x1 - dfpnt ) * wp  / dnpnt  + sp;
         delta = cr - delta;
      }
      if ( ! rev_v) {
         cr1 = - (y1 - dfpnt2) * wp1 / dnpnt2 + wp1 + sp1;
	 delta1 = - (y0 - dfpnt2) * wp1  / dnpnt2  + wp1 + sp1;
         delta1 = cr1 - delta1;
      } else if ( revAxis_v ) {
         cr1  = (y1 - dfpnt2 - dnpnt2 ) * wp1  / dnpnt2  + sp1;
	 delta1 = (y0 - dfpnt2 - dnpnt2 ) * wp1  / dnpnt2  + sp1;
         delta1 = delta1 -cr1;
      } else {
         cr1 =   (y1 - dfpnt2) * wp1 / dnpnt2 + sp1;
	 delta1 = + (y0 - dfpnt2 ) * wp1  / dnpnt2  + sp1;
         delta1 = cr1 - delta1;
      }

      return;
}

void dconi_setCursor(int x0, int y0, int x1, int y1)
{
        dconi_setInsetCursor(x0,y0,x1,y1);
	dconi_mode = BOX_MODE;
}

void dconi_getNextCursor(double *c2, double *c1, double *d2, double *d1)
{
   char trace[8];
   if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");
   if(strstr(trace, "2") != NULL) {
	*d2 = wp;
        if( ! freq_h )
          *c2 = sp;
        else if ( revAxis_h )
          *c2 = sp - wp;
        else
          *c2 = sp + wp;
	*d1 = wp1;
        if( ! freq_v )
          *c1 = sp1;
        else if ( revAxis_v )
          *c1 = sp1 - wp1;
        else
          *c1 = sp1 + wp1;
   } else {
	*d1 = wp;
        if( ! freq_h )
          *c1 = sp;
        else if ( revAxis_h )
          *c1 = sp - wp;
        else
          *c1 = sp + wp;
	*d2 = wp1;
        if( ! freq_v )
          *c2 = sp1;
        else if ( revAxis_v )
          *c2 = sp1 - wp1;
        else
          *c2 = sp1 + wp1;
   }
}

void dconi_setNextCursor(double c2, double c1, double d2, double d1)
{
   if(d2==0 && d1==0) {
     if ( ! freq_h )
       cr = sp;
     else if ( revAxis_h )
       cr  = sp - delta;
     else
       cr  = sp + delta;
     if ( ! freq_v )
       cr1 = sp1;
     else if ( revAxis_v )
       cr1 = sp1 - delta1;
     else
       cr1 = sp1 + delta1;
     delta  = fabs(wp);
     delta1 = fabs(wp1);
     return;
   }

   if(revflag) {
           cr1=c2;
           delta1=d2;
           cr=c1;
           delta=d1;
        } else {
           cr=c2;
           delta=d2;
           cr1=c1;
           delta1=d1;
        }
}

void dconi_zoom(int mode)
{
    dconi_mode = mode;
    dconi_expand();

    if(!showCursor()) {
      dconi_mode=CURSOR_MODE;

      dconi_cursor();
    } 
}

/************************************/
static void m_spwp(int but, int x, int y)
/************************************/
{
   double f2, f1;
   static double spfreq, spfreq1;
   static double oldsp, oldsp1, oldwp, oldwp1;
   char trace[8];
   char cmd[20];

/*
fprintf(stderr,"m_spwp %d %d %d\n", but, x, y);
*/
   y = mnumypnts - y -1;

   if ( ! rev_h) {
         f2  =  - (x - dfpnt ) * wp  / dnpnt;
   } else if ( revAxis_h ) {
         f2  = (x - dfpnt - dnpnt ) * wp  / dnpnt;
   } else {
         f2  =  (x - dfpnt ) * wp  / dnpnt;
   }
   if ( ! rev_v) {
	 f1 =  - (y - dfpnt2) * wp1  / dnpnt2;
   } else if ( revAxis_h ) {
         f1  = (y - dfpnt2 - dnpnt2 ) * wp1  / dnpnt2;
   } else {
	 f1 =  (y - dfpnt2 ) * wp1  / dnpnt2;
   }

   if(but == 4) {
      oldsp = sp;
      oldsp1 = sp1;
      oldwp = wp;
      oldwp1 = wp1;
      spfreq = f2;
      spfreq1 = f1;
      return;
   } else if(but == 1) {

      sp  = oldsp + (spfreq - f2);
      sp1  = oldsp1 + (spfreq1 - f1);
/*
fprintf(stderr,"dconi_spwp1 %f %f %f %f %f %f\n", sp, sp1, spfreq, spfreq1, f2, f1);
*/
   if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");
   if(strstr(trace, "2") != NULL) {
    	dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);
      } else {
    	dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);
      }
      UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
      UpdateVal(VERT,SP_NAME,sp1,NOSHOW);
   } else if(but == 3) {

      wp  = oldwp + (spfreq - f2);
      wp1  = oldwp1 + (spfreq1 - f1);
/*
fprintf(stderr,"dconi_spwp1 %f %f %f %f %f %f\n", sp, sp1, spfreq, spfreq1, f2, f1);
*/
      if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");
      if(strstr(trace, "2") != NULL) {
    	dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);
      } else {
    	dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);
      }
      UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
      UpdateVal(VERT,WP_NAME,wp1,NOSHOW);
   }

   Wgetgraphicsdisplay(cmd, 20);
   if(strstr(cmd,"d") == cmd && strstr(cmd,"con") != NULL) { 
/*
	strcat(cmd,"\n");
       execString(cmd);
*/
   execString("dconi('again')\n");
   //dconi_redisplay();
   // run_dcon();

    dconi_sendSpwp(sp1, sp, wp1, wp, but);
   }
/*
fprintf(stderr,"dconi_spwp2 %f %f %f %f %f %f\n", sp, sp1, wp, wp1, f2, f1);
*/
}

void dconi_spwp(int but, int x, int y, int mflag)
{
   (void) mflag;
   m_spwp(but, x, y);
  if(revflag) {
     dconi_currentZoom(&cr1,&delta1,&cr,&delta);
  } else {
     dconi_currentZoom(&cr,&delta,&cr1,&delta1);
  }
    
    if(!showCursor()) {
      dconi_mode=BOX_MODE;
      dconi_cursor();
    }
}

void dconi_sendSpwp(double c1, double c2, 
	double d1, double d2, int mode)
{
   double f1,f2;
   char trace[5], axis1[8], axis2[8];

   if(getFrameID() > 1) return;

     get_nuc_name(HORIZ, axis2, 7);
     get_nuc_name(VERT, axis1, 7);

   c2 = Hz2ppm(HORIZ, c2);
   d2 = Hz2ppm(HORIZ, d2);
   c1 = Hz2ppm(VERT, c1);
   d1 = Hz2ppm(VERT, d1);

   if (P_getreal(GLOBAL,"trackAxis", &f2, 1) != 0) f2 = 0.0;
   if (P_getreal(GLOBAL,"trackAxis", &f1, 2) != 0) f1 = 0.0;

   if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");

   if(getOverlayMode() >= NOTOVERLAID_ALIGNED || (f2>0 && f1>0)) {
     dconi_sendSpwp_2D(c1,c2,d1,d2,mode,trace,axis1,axis2);
   } else if((f2>0 && strstr(trace, "2") != NULL))
       dconi_sendSpwp_1D(c2,d2,mode,axis2);
   else if(f2>0)
       dconi_sendSpwp_1D(c1,d1,mode,axis1);
   else if((f1>0 && strstr(trace, "1") != NULL)) 
       dconi_sendSpwp_1D(c2,d2,mode,axis2);
   else if(f1>0)
       dconi_sendSpwp_1D(c1,d1,mode,axis1);
}

void dconi_sendSpwp_2D(double c1, double c2, 
	double d1, double d2, int mode, char *trace, char *axis1, char *axis2)
{
   int i, vps;
   double d;
   char cmd[MAXSTR];

   if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
   else vps = (int)d;
   if(vps < 2) return;

   for(i=0; i<vps; i++) {
	if(i+1 != VnmrJViewId) {
          if (P_getreal(GLOBAL,"jvpexps", &d, i+1) != 0) d = 0.0;
          if(d < 0.5) continue;

	  sprintf(cmd, "vnmrjcmd('CH %d trackCursor(\\'spwp\\', %f, %f, %f, %f, %d, \\'%s\\', \\'%s\\', \\'%s\\')')\n", i+1, c1, c2, d1, d2, mode, trace, axis1, axis2); 
 	  execString(cmd);
	}
   }

}

void dconi_sendSpwp_1D(double c2, double d2, int mode, char *axis)
{
   int i, vps;
   double d;
   char cmd[MAXSTR];

   if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
   else vps = (int)d;
   if(vps < 2) return;

   for(i=0; i<vps; i++) {
	if(i+1 != VnmrJViewId) {
          if (P_getreal(GLOBAL,"jvpexps", &d, i+1) != 0) d = 0.0;
          if(d < 0.5) continue;

	  sprintf(cmd, "vnmrjcmd('CH %d trackCursor(\\'spwp\\', %f, %f, %d, \\'%s\\')')\n", i+1, c2, d2, mode, axis);
 	  execString(cmd);
	}
   }
}

void dconi_newSpwp(int num, double c1, double c2, 
	double d1, double d2, int but, char *tr, char *ax1, char *ax2)
{
/* if cursor is from a 1d spectrum, only c1, d1 are passed */

      double d;
      char axis1[8], axis2[8];
      char cmd[20];

    (void) but;
/* return if cursorTracking is off */
    if (P_getreal(GLOBAL,"trackAxis", &d, 1) != 0) d = 0.0;
    if(d < 0.5)
	if (P_getreal(GLOBAL,"trackAxis", &d, 2) != 0) d = 0.0;
    if(d < 0.5 && getOverlayMode() < NOTOVERLAID_ALIGNED) return;

     get_nuc_name(HORIZ, axis2, 7);     
     get_nuc_name(VERT, axis1, 7);     

   if(num == 2) {
	if(strcmp(ax2,axis2) == 0) {	
	   sp = ppm2Hz(HORIZ, c2);
	   wp = ppm2Hz(HORIZ, d2);
	}
	if(strcmp(ax2,axis1) == 0) {
	   sp1 = ppm2Hz(VERT, c2);
	   wp1 = ppm2Hz(VERT, d2);
        }
   } else if(num == 4 && !strcmp(axis1,axis2) && 
	!strcmp(axis1,ax1) && !strcmp(axis1,ax2) ) { // homo and homo

        char trace[5];
        if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");

	if(strcmp(tr,trace) == 0) {
	   sp = ppm2Hz(HORIZ, c2);
	   wp = ppm2Hz(HORIZ, d2);
	   sp1 = ppm2Hz(VERT, c1);
	   wp1 = ppm2Hz(VERT, d1);
	   
	} else {
	   sp = ppm2Hz(HORIZ, c1);
	   wp = ppm2Hz(HORIZ, d1);
	   sp1 = ppm2Hz(VERT, c2);
	   wp1 = ppm2Hz(VERT, d2);
	}
   } else if(num == 4) { 
      
	if(strcmp(ax2,axis2) == 0) {
	   sp = ppm2Hz(HORIZ, c2);
	   wp = ppm2Hz(HORIZ, d2);
        } else if(strcmp(ax1,axis2) == 0) {
	   sp = ppm2Hz(HORIZ, c1);
	   wp = ppm2Hz(HORIZ, d1);
        }
        if(strcmp(ax2,axis1) == 0) {
	   sp1 = ppm2Hz(VERT, c2);
	   wp1 = ppm2Hz(VERT, d2);
        } else if(strcmp(ax1,axis1) == 0) {
	   sp1 = ppm2Hz(VERT, c1);
	   wp1 = ppm2Hz(VERT, d1);
        } 
   } else return;

      dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);

      UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
      UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
      UpdateVal(VERT,SP_NAME,sp1,NOSHOW);
      UpdateVal(VERT,WP_NAME,wp1,NOSHOW);
    
   Wgetgraphicsdisplay(cmd, 20);
   if(strstr(cmd,"d") == cmd && strstr(cmd,"con") != NULL) { 
     execString("dconi('again')\n");

     if(VnmrJViewId == getAlignvp()) {
        dconi_sendSpwp(sp1, sp, wp1, wp, 1);
     }
   }
}

int mapAxis(int num, double *c1, double *c2, double *d1, double *d2, 
   char *trace, char *axis1, char * axis2, char *tr, char *ax1, char *ax2)
{

   double cursor1, dt1;

/* now we figure out how the cursor is mapped to the axes */
    if(num < 4) {
	/* cursor is from a 1d spectrum */
	if(strcasecmp(ax2, axis2) == 0 && strcasecmp(ax2, axis1) == 0) {
	    *c1 = *c2;
	    *d1 = *d2;
	    return 2;
	} else if(strcasecmp(ax2, axis1) == 0) {
	    *c1 = *c2;
	    *d1 = *d2;
	    strcpy(axis2,"");
	    return 1;
	} else if(strcasecmp(ax2, axis2) == 0) {
	    strcpy(axis1,"");
	    return 1;
	} 
	return 0;

    } else {
	/* cursor from a 2d spectrum */
	/* assume c2, d2 is always Y (_send... function is responsible to swab if trace='f2') */
        if(strcasecmp(axis1, axis2) == 0 && strcasecmp(ax1, ax2) == 0 &&
	   strcasecmp(ax2, axis2) == 0) {
	   /* both master and slave are homonuclear and are the same nucleus */
	   if(strstr(tr, "2") == NULL) {
		cursor1=*c1;
		dt1=*d1;	
		*c1=*c2;
		*d1=*d2;
		*c2=cursor1;
		*d2=dt1;
	   }
	   return 2;
	} else if(strcasecmp(axis1, axis2) == 0 && strcasecmp(ax1, ax2) == 0) {
	   /* both master and slave are homonuclear and are not the same nucleus */
	   return 0;
	} else if(strcasecmp(axis1, axis2) == 0) {
	   /* slave is homonuclear */
	   if(strcasecmp(axis1, ax1) == 0) {
		*c2=*c1;
		*d2=*d1;
		return 2;
	   } else if(strcasecmp(axis1, ax2) == 0) {
		*c1=*c2;
		*d1=*d2;
		return 2;
	   }
	   return 0;
	} else if(strcasecmp(ax1, ax2) == 0) {
	   if(strstr(trace, "2") == NULL) {
		cursor1=*c1;
		dt1=*d1;	
		*c1=*c2;
		*d1=*d2;
		*c2=cursor1;
		*d2=dt1;
	   }
	   /* master is homonuclear */
	   if(strcasecmp(axis1, ax1) == 0) {
	      strcpy(axis2,"");
	      return 1;
	   } else if(strcasecmp(axis2, ax1) == 0) {
	      strcpy(axis1,"");
	      return 1;
	   }
	   return 0;
	} else if(strcasecmp(ax1, axis1) == 0 && strcasecmp(ax2, axis2) == 0) {
	   /* master and slave are both heteronuclear */
	   return 2;
	} else if(strcasecmp(ax1, axis2) == 0 && strcasecmp(ax2, axis1) == 0) {
	   cursor1=*c1;
	   dt1=*d1;	
	   *c1=*c2;
	   *d1=*d2;
	   *c2=cursor1;
	   *d2=dt1;
	   return 2;
	} else if(strcasecmp(ax1, axis1) == 0) {
	   strcpy(axis2,"");
	   return 1;
	} else if(strcasecmp(ax1, axis2) == 0) {
	   *c1=*c2;
	   *d1=*d2;
	   strcpy(axis2,"");
	   return 1;
	} else if(strcasecmp(ax2, axis2) == 0) {
	   strcpy(axis1,"");
	   return 1;
	} else if(strcasecmp(ax2, axis1) == 0) {
	   *c2=*c1;
	   *d2=*d1;
	   strcpy(axis1,"");
	   return 1;
	}
	return 0;
    }
}

void dconi_zoomCenterPeak(int x, int y, int but)
{
    double freq1, freq2;
    double s2, s1, w2, w1, d;

      if ( ! rev_h)
         freq2  = - (x - dfpnt ) * wp  / dnpnt  + sp + wp;
      else if ( revAxis_h )
         freq2  = (x - dfpnt - dnpnt ) * wp  / dnpnt  + sp;
      else
         freq2  =   (x - dfpnt ) * wp  / dnpnt + sp;
      if ( ! rev_v)
         freq1= - (y - dfpnt2) * wp1 / dnpnt2 + sp1 + wp1;
      else if ( revAxis_v )
         freq1  = (y - dfpnt2 - dnpnt2 ) * wp1  / dnpnt2  + sp1;
      else
         freq1=   (y - dfpnt2) * wp1 / dnpnt2 + sp1;

    w2 = wp;
    w1 = wp1;

    s2 = freq2 - wp/2;
    if(s2 < -rflrfp) {
        w2 = 2*(freq2 + rflrfp);
    }
    d = freq2 + wp/2;
    if(d > (sw-rflrfp)) {
        d = 2*(sw - freq2 - rflrfp);
        if(d < w2) w2 = d;
    }
    s1 = freq1 - wp1/2;
    if(s1 < -rflrfp1) {
        w1 = 2*(freq1 + rflrfp1);
    }
    d = freq1 + wp1/2;
    if(d > (sw1-rflrfp1)) {
        d = 2*(sw1 - freq1 - rflrfp1);
        if(d < w1) w1 = d;
    }

    	sp = freq2 - w2/2;
    	sp1 = freq1 - w1/2;

    if(but == 3) {
      dconi_prevZoom(&cr, &cr1, &delta, &delta1);
    } else if(wp == w2 || wp1 == w1) {
      dconi_nextZoom(&cr, &cr1, &delta, &delta1);
    } else {
      if(w2 != wp) {
	wp = w2;
        cr = freq2 + w2/2;
        delta = w2;
    	cr1 = sp1;
    	delta1 = wp1;
        if ( freq_v )
        {
           if ( revAxis_v )
              cr1 -= delta1;
           else
              cr1 += delta1;
        }
      }
      if(w1 != wp1) {
	wp1 = w1; 
        cr1 = freq1 + w1/2;
        delta1 = w1;
    	cr = sp;
    	delta = wp;
        if ( freq_h )
        {
           if ( revAxis_h )
              cr -= delta;
           else
              cr += delta;
        }
      }
    }

   dconi_zoom(BOX_MODE);
}

void dconi_centerPeak(int x, int y, int but)
{
    double freq1, freq2;
    double s2, s1, d2, d1;
    char trace[5];

      if ( ! rev_h)
         freq2  = - (x - dfpnt ) * wp  / dnpnt  + wp  + sp;
      else if ( revAxis_h )
         freq2  = (x - dfpnt - dnpnt ) * wp  / dnpnt  + sp;
      else
         freq2 = (x - dfpnt) * wp  / dnpnt  + sp;
      if ( ! rev_v)
         freq1 = - (y - dfpnt2) * wp1 / dnpnt2 + wp1 + sp1;
      else if ( revAxis_v )
         freq1  = (y - dfpnt2 - dnpnt2 ) * wp1  / dnpnt2  + sp1;
      else
         freq1 =   (y - dfpnt2) * wp1 / dnpnt2 + sp1;

      s2 = freq2 - wp/2;
      d2 = freq2 + wp/2;
      s1 = freq1 - wp1/2;
      d1 = freq1 + wp1/2;

      if(s2 < -rflrfp || d2 > (sw-rflrfp) ||
	 s1 < -rflrfp1 || d1 > (sw1-rflrfp1) ) {
	dconi_zoomCenterPeak(x, y, but);
        return;
      }

        sp = freq2 - wp/2;
        sp1 = freq1 - wp1/2;

      if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");
      if(strstr(trace, "2") != NULL) {
    	dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);
      } else {
    	dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);
      }
      UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
      UpdateVal(VERT,SP_NAME,sp1,NOSHOW);

    execString("dconi('again')\n");

  if(revflag) {
     dconi_currentZoom(&cr1,&delta1,&cr,&delta);
  } else {
     dconi_currentZoom(&cr,&delta,&cr1,&delta1);
  }
    
    dconi_sendSpwp(sp1, sp, wp1, wp, 1);
}

void dconi_nextZoomin(double *c2, double *c1, double *d2, double *d1)
{
     char trace[5];
     double d, slope;

   if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");
   if(strstr(trace, "2") != NULL) {
    *c2 = sp;
    *d2 = wp;
    *c1 = sp1;
    *d1 = wp1;
   } else {
    *c1 = sp;
    *d1 = wp;
    *c2 = sp1;
    *d2 = wp1;
   }

     if(P_getreal(GLOBAL, "zoomSlope", &slope, 1)) slope = 0.2;

     d = slope*(*d2);
     if( freq_h)
     {
        if ( revAxis_h)
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

     d = slope*(*d1);
     if( freq_v)
     {
        if ( revAxis_v)
        {
           *c1 -= *d1;
           *c1 += 0.5*d;
        }
        else
        {
           *c1 += *d1;
           *c1 -= 0.5*d;
        }
     }
     *d1 -= d;
}

void dconi_nextZoom(double *c2, double *c1, double *d2, double *d1)
{
     double d, slope;

    *c2 = sp;
    *d2 = wp;
    *c1 = sp1;
    *d1 = wp1;

     if(P_getreal(GLOBAL, "zoomSlope", &slope, 1)) slope = 0.2;

     d = slope*(*d2);
     if( freq_h)
     {
        if ( revAxis_h)
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

     d = slope*(*d1);
     if( freq_v)
     {
        if ( revAxis_v)
        {
           *c1 -= *d1;
           *c1 += 0.5*d;
        }
        else
        {
           *c1 += *d1;
           *c1 -= 0.5*d;
        }
     }
     *d1 -= d;
}

void dconi_prevZoom(double *c2, double *c1, double *d2, double *d1)
{
     double d, slope;

    *c2 = sp;
    *d2 = wp;
    *c1 = sp1;
    *d1 = wp1;

     if(P_getreal(GLOBAL, "zoomSlope", &slope, 1)) slope = 0.2;
     d = slope*(*d2);
     if( freq_h)
     {
        if ( revAxis_h)
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

     if( freq_v)
     {
        if ( revAxis_v)
        {
           *c1 -= *d1;
           *c1 -= 0.5*d;
        }
        else
        {
           *c1 += *d1;
           *c1 += 0.5*d;
        }
     }
     *d1 += d;
}

// f1 for sw1, f2 for sw2, f3 for sw.
void getSw(double *s2max, double *s1max, double *w2max, double *w1max) {

   get_sw_par(HORIZ,w2max);
   get_sw_par(VERT,w1max);
   get_rflrfp(HORIZ,s2max);
   get_rflrfp(VERT,s1max);
  (*s2max) *= -1.0;
  (*s1max) *= -1.0;
}

void dconi_sendSpecInfo(int frame) {
    char axis2[8], axis1[8], line[MAXSTR];
    double s2max, s1max, w2max, w1max, s2, w2, s1, w1, ref1,ref2;
    int rev;

    if(frame != 1) return; 

    get_scale_pars(HORIZ, &s2, &w2, &ref2, &rev);
    get_scale_pars(VERT, &s1, &w1, &ref1, &rev);
    getSw(&s2max, &s1max, &w2max, &w1max);
     get_nuc_name(HORIZ, axis2, 7);     
     get_nuc_name(VERT, axis1, 7);     

    s2max = Hz2ppm(HORIZ, s2max);
    w2max = Hz2ppm(HORIZ, w2max);
    s2 = Hz2ppm(HORIZ, s2);
    w2 = Hz2ppm(HORIZ, w2);
    s1max = Hz2ppm(VERT, s1max);
    w1max = Hz2ppm(VERT, w1max);
    s1 = Hz2ppm(VERT, s1);
    w1 = Hz2ppm(VERT, w1);

    sprintf(line, "dataInfo %d %d %s %s %s %f %f %f %f %f %f %f %f 2\n", VnmrJViewId, frame, "dconi", axis2, axis1, s2max, s1max, w2max, w1max, s2, s1, w2, w1);

    writelineToVnmrJ("vnmrjcmd", line);
}

void dconi_overlaySpec(int mode)
{
    char trace[5];
   if(!(mode < 2 && s_overlayMode < 2)) { 
      if (P_getstring(CURRENT, "trace", trace, 1, 4)) strcpy(trace, "f1");
      if(strstr(trace, "2") != NULL) {
     	dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);
        execString("dconi('again')\n");
        dconi_sendSpwp(sp1, sp, wp1, wp, 1);
      } else {
    	dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);
        execString("dconi('again')\n");
        dconi_sendSpwp(sp1, sp, wp1, wp, 1);
      }
      UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
      UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
      UpdateVal(VERT,SP_NAME,sp1,NOSHOW);
      UpdateVal(VERT,WP_NAME,wp1,NOSHOW);
/*
      dconi_checkSpwp(&sp, &sp1, &wp, &wp1, VnmrJViewId);
      execString("dconi('again')\n");
      dconi_sendSpwp(sp1, sp, wp1, wp, 1);
*/
 
    }
    s_overlayMode = mode;
}

void dconi_checkSpwp(double *s2, double *s1, double *w2, double *w1, int id)
{
/* s2, w2, s1, w1 are in Hz */
    double d, sx, wx, ex, sy, wy, ey, e2, e1;
    int i, mode, tmpMode;
    char ax1[8], ax2[8];
    char axis2[8],axis1[8];
   mode = getOverlayMode();

    tmpMode = mode;
    if (P_getreal(GLOBAL,"trackAxis", &d, 1) != 0) d = 0.0;
    if(d < 0.5)
	if (P_getreal(GLOBAL,"trackAxis", &d, 2) != 0) d = 0.0;
    if(d < 0.5 && getOverlayMode() < NOTOVERLAID_ALIGNED) return;

    if(mode != UNSTACKED && mode != STACKED && s_overlayMode == STACKED) {
        mode = UNSTACKED;
    } else if(mode < NOTOVERLAID_ALIGNED && s_overlayMode == 3) {
        mode = NOTOVERLAID_ALIGNED;
    }  
    s_overlayMode = tmpMode;

    if(mode < NOTOVERLAID_ALIGNED) return;

    getSweepInfo(&sx, &wx, &sy, &wy,ax2,ax1);
    getAxes(axis2, axis1, 7);
    if(strcmp(axis2,ax2) != 0 || strcmp(axis1,ax1) != 0) return;
/*
fprintf(stderr, "dataInfo0 %d %d %f %f %f %f %f %f %f %f\n", VnmrJViewId, id, sx, sy, wx, wy, *s2, *s1, *w2, *w1);
*/
       sx = ppm2Hz(HORIZ, sx);
       ex = ppm2Hz(HORIZ, wx) + sx;
       sy = ppm2Hz(VERT, sy);
       ey = ppm2Hz(VERT, wy) + sy;

    e2 = *s2 + *w2;
    e1 = *s1 + *w1;

    if(mode < STACKED) {
           if(sx > *s2) *s2 = sx;
           if(ex < e2 && ex>*s2) *w2 = ex-*s2;
           if(sy > *s1) *s1 = sy;
           if(ey < e1 && ey>*s1) *w1 = ey-*s1;
    } else {

      double xshift, yshift, xshift0, yshift0;
      int activeWin, vps, n = 0, ind = 0, activeInd = 0;

      if (P_getreal(GLOBAL,"jviewports", &d, 1) != 0) vps = 1;
      else vps = (int)d;
      if(vps < 2) return;

      activeWin = getActiveWin();

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

      xshift = (ind-activeInd)*xshift0;
      yshift = (ind-activeInd)*yshift0;
/*
      xshift = (ind-1)*xshift0;
      yshift = (ind-1)*yshift0;
*/
        if(mode == STACKED) {
/*
fprintf(stderr, "STACKED1 %f %f %f %f\n", *s2, *s1, xshift, yshift);
*/
          (*s2) += xshift;
          (*s1) += yshift;
/*
fprintf(stderr, "STACKED2 %f %f %f %f\n", *s2, *s1, xshift, yshift);
*/
        } else if(mode == UNSTACKED) {
          //(*s2) -= xshift;
          //(*s1) -= yshift;
        }

        if(ind == activeInd && sx > ((*s2)-xshift0*(activeInd-1)))
              *s2 = sx + xshift0*(activeInd-1);
        else 

	if(sx > *s2) *s2 = sx;

        if(ind == activeInd && ex < (e2+xshift0*(n-activeInd)) &&
		(ex -xshift0*(n-activeInd) - *s2) > 0)
              *w2 = ex -xshift0*(n-activeInd) - *s2;
        else 

	if(ex < e2 && ex>*s2) *w2 = ex-*s2;

        if(ind == activeInd && sy > ((*s1)-yshift0*(activeInd-1)))
              *s1 = sy + yshift0*(activeInd-1);
        else 

	if(sy > *s1) *s1 = sy;

        if(ind == activeInd && ey < (e1+yshift0*(n-activeInd)) &&
		(ey -yshift0*(n-activeInd) - *s1)>0)
              *w1 = ey -yshift0*(n-activeInd) - *s1;
        else 

	if(ey < e1 && ey>*s1) *w1 = ey-*s1;
    }
}

int dconi_currentZoom(double *c2, double *d2, double *c1, double *d1) {
   double cx,dx,cy,dy;
   if ( ! freq_h )
        cx = sp;
   else if ( revAxis_h )
        cx  = sp - delta;
   else
        cx  = sp + delta;
   dx  = fabs(wp);
   if ( ! freq_v )
        cy = sp1;
   else if ( revAxis_v )
        cy  = sp1 - delta1;
   else
        cy  = sp1 + delta1;
   dy  = fabs(wp1);
  
   if(revflag) {
     *c2=cy;
     *d2=dy;
     *c1=cx;
     *d1=dx;
   } else {
     *c1=cy;
     *d1=dy;
     *c2=cx;
     *d2=dx;
   }
   return(0);
}
