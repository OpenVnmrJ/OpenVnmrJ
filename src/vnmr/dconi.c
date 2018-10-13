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
#include <string.h>
#include <math.h>

#include "vnmrsys.h"
#include "data.h"
#include "disp.h"
#include "init2d.h"
#include "graphics.h"
#include "group.h"

static int oldx_cursor[2],oldy_cursor[2],idelta,idelta1,xoffset,yoffset;
static int last_trace,leveldisplay_active;
static int rev_h, rev_v;

static int level_y0,level_y1,level_x1;
static int level_first;
static int spec1,spec2,plot_spec,next,erase1,erase2,plot_erase;
static float *project_buffer;
static float discalib;
static double last_vs,cr1,delta1;
static float *phasfl;

static char mark_textfname[ MAXPATHL ];
static FILE *mark_textfptr = NULL;

char dconi_runstring[128];
char redraw_string[128];
int dconi_mode;
static int trace_mode;

static char expandMacro[MAXSTR];
static char traceMacro[MAXSTR];

extern double graysl,graycntr;
extern int gray_p_flag;
extern int dcon_first_color,dcon_num_colors,dcon_colorflag;
extern int debug1;
extern int menuflag;
extern int in_plot_mode;  /* defined in graphics.c */

#define NOTRACE_MODE	0
#define CURSOR_MODE	1
#define TRACE_MODE	2
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

#define zerofill(data_pntr, npoints_to_fill)			\
			datafill(data_pntr, npoints_to_fill,	\
				 0.0)

extern int calc_ybars();
extern int displayspec();
#ifdef X11
extern int dconi_x_cursor();
extern int dconi_y_cursor();
#else
extern int x_cursor();
extern int y_cursor();
#endif
extern int set_cursors();
extern float vs_mult();
extern struct ybar *outindex();
extern int init_display();
extern int exit_display();
extern char *allocateWithId();
extern FILE *fopen();

/******************/
dconi_reset()
/******************/
{
  if (WgraphicsdisplayValid("dconi"))  {
    update_cursor(0,0,0);
    update_cursor(1,0,0);
    }
  last_trace = -1;
  last_vs = 0;
  disp_status("      ");
  xoffset = mnumxpnts - dfpnt + 8*xcharpixels;
  yoffset = dfpnt2 + dnpnt2 + mnumypnts / 50;
  if (project_buffer) release(project_buffer);
  project_buffer = 0;
  trace_mode=0;
}

/*************************************/
static update_cursor(cursor_number,x,y)
/*************************************/
int cursor_number,x,y;
{ if ((Wistek() || WisSunColor()) && leveldisplay_active &&
                      (dcon_colorflag==SELECT_GRAY_COLOR))
  {
#ifdef X11
    dconi_x_rast_cursor(&oldx_cursor[cursor_number],x,dfpnt2,dnpnt2);
    dconi_y_rast_cursor(&oldy_cursor[cursor_number],y,dfpnt,dnpnt);
#else
    x_rast_cursor(&oldx_cursor[cursor_number],x,dfpnt2,dnpnt2);
    y_rast_cursor(&oldy_cursor[cursor_number],y,dfpnt,dnpnt);
#endif
  }
  else
  {
  xormode();
  color(CURSOR_COLOR);
#ifdef X11
  dconi_x_cursor(&oldx_cursor[cursor_number],x);
  dconi_y_cursor(&oldy_cursor[cursor_number],y);
#else
  x_cursor(&oldx_cursor[cursor_number],x);
  y_cursor(&oldy_cursor[cursor_number],y);
#endif
  normalmode();
}}

/*****************************************/
static dconi_m_newcursor(butnum,x,y,moveflag)
/*****************************************/
int butnum,x,y,moveflag;
{ int ox,oy,ox1,oy1;
  int turnoff_dconi();

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
}

/*****************************************/
static dconi_newcursor(butnum,x,y,moveflag)
/*****************************************/
int butnum,x,y,moveflag;
{ int r;

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
              InitVal(FIELD2,VERT, DELTA_NAME, PARAM_COLOR,UNIT4,
                                   NO_NAME,-PARAM_COLOR,SCALED,2);
              InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,UNIT4,
                                   NO_NAME,-PARAM_COLOR,SCALED,2);
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
      if (rev_h)
         cr  =   (x - dfpnt ) * wp  / dnpnt  + sp;
      else
         cr  = - (x - dfpnt ) * wp  / dnpnt  + wp  + sp;
      if (rev_v)
         cr1 =   (y - dfpnt2) * wp1 / dnpnt2 + sp1;
      else
         cr1 = - (y - dfpnt2) * wp1 / dnpnt2 + wp1 + sp1;
      UpdateVal(HORIZ,CR_NAME,cr,SHOW);
      UpdateVal(VERT,CR_NAME,cr1,SHOW);
    }
  }
  Wsetgraphicsdisplay("dconi");
  return 0;
}

/*********************/
static getxy(x,y,x1,y1)
/*********************/
int *x,*y,*x1,*y1;
{ int r;
  double freq_val;

  get_cursor_pars(HORIZ,&cr,&delta);
  get_cursor_pars(VERT,&cr1,&delta1);

#ifdef DEBUG
    Wscrprintf(
        "getxy, dfpnt= %d  dnpnt= %d  wp= %.12g  cr= %.12g  sp= %.12g\n",
	 dfpnt, dnpnt, wp, cr, sp);
    Wscrprintf(
        "getxy, dfpnt2= %d  dnpnt2= %d  wp1= %.12g  cr1= %.12g  sp1= %.12g\n",
	 dfpnt2, dnpnt2, wp1, cr1, sp1);
#endif

  freq_val = (rev_h) ? (cr - sp) : (wp + sp - cr);
  *x  = (int)((double)dfpnt  + (double)dnpnt  * freq_val / wp + 0.5);
  freq_val = (rev_v) ? (cr1 - sp1) : (wp1 + sp1 - cr1);
  *y  = (int)((double)dfpnt2 + (double)dnpnt2 * freq_val / wp1 + 0.5);
  *x1 = (int)((double)*x     + (double)dnpnt  * delta  / wp + 0.5);
  *y1 = (int)((double)*y     + (double)dnpnt2 * delta1 / wp1 + 0.5);

  return 0;
}

/*******************/
dconi_cursor()
/*******************/
{ int x,y,x1,y1;
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
  InitVal(FIELD1,VERT, CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,2);
  InitVal(FIELD3,HORIZ,CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,2);
  InitVal(FIELD2,VERT, DELTA_NAME, PARAM_COLOR,UNIT4,
                       x,          y,SCALED,2);
  InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,UNIT4,
                       x,          y,SCALED,2);
  DispField1(FIELD5,PARAM_COLOR,"vsproj");
  DispField1(FIELD6,PARAM_COLOR,"vs2d");
  update_vs_pars();
  activate_mouse(dconi_m_newcursor,dconi_reset);
  if (getxy(&x,&y,&x1,&y1)) return 1;
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
      disp_status("BOX   ");
    }
  else
    {
      update_cursor(1,0,0);
      disp_status("CURSOR");
    }
  Wsetgraphicsdisplay("dconi");
}

/****************************************/
static dconi_m_newtrace(butnum,x,y,moveflag)
/****************************************/
int butnum,x,y,moveflag;
{ int ox,oy,ox1,oy1;
  int turnoff_dconi();

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
static dconi_newtrace(butnum,x,y,moveflag)
/****************************************/
int butnum,x,y,moveflag;
{ int ctrace;
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
    }
  Wsetgraphicsdisplay("dconi");
  return 0;
}

/******************/
static dconi_trace()
/******************/
{ int x,y,r,x1,y1;
  Wgmode();
  Wturnoff_mouse();
  dconi_reset();
  /* erase the screen behind the trace */
  color(BACK);
  amove(dfpnt-1,dfpnt2+dnpnt2+3);
  box(dnpnt+2,mnumypnts-dfpnt2-dnpnt2-5);
  erase1 = 0;
  /* activate cursors */
  if (getxy(&x,&y,&x1,&y1)) return 1;
  disp_status("TRACE ");
  activate_mouse(dconi_m_newtrace,dconi_reset);
  trace_mode = TRACE_MODE;
  dconi_mode = CURSOR_MODE;
  InitVal(FIELD1,VERT, CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,2);
  InitVal(FIELD3,HORIZ,CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,2);
  InitVal(FIELD2,VERT, DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,2);
  InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,2);
  DispField1(FIELD5,PARAM_COLOR,"vsproj");
  DispField1(FIELD6,PARAM_COLOR,"vs2d");
  update_vs_pars();
  dconi_newtrace(1,x,y,0);
  Wsetgraphicsdisplay("dconi");
}

/*******************/
static update_hproj()
/*******************/
{
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
static dconi_hproj(sumflag)
/*************************/
int sumflag;
{ int trace,first_trace,num_traces,x,y,x1,y1;
  float *phasfl;
  
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
      return 1;
    }
  /* erase the screen behind the trace */
  color(BACK);
  amove(dfpnt-1,dfpnt2+dnpnt2+3);
  box(dnpnt+2,mnumypnts-dfpnt2-dnpnt2-5);
  erase1 = 0;
  zerofill(project_buffer,npnt);
  for (trace=first_trace; trace < first_trace+num_traces; trace++)
    { if ((phasfl=gettrace(trace,fpnt))==0)
        { Wturnoff_buttons(); return 1; }
      if (sumflag)
        skyadd(phasfl,project_buffer,project_buffer,npnt);
      else
        skymax(phasfl,project_buffer,project_buffer,npnt);
    }
  skywt();
  update_hproj();
  activate_mouse(dconi_m_newcursor,dconi_reset);
  trace_mode = HPROJ_MODE;
  InitVal(FIELD1,VERT, CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,2);
  InitVal(FIELD3,HORIZ,CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,2);
  InitVal(FIELD2,VERT, DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,2);
  InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,2);
  DispField1(FIELD5,PARAM_COLOR,"vsproj");
  DispField1(FIELD6,PARAM_COLOR,"vs2d");
  update_vs_pars();
  dconi_newcursor(1,x,y,0);
  Wsetgraphicsdisplay("dconi");
}

/*******************/
static update_vproj()
/*******************/
{
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
static dconi_vproj(sumflag)
/*************************/
int sumflag;
{ int first_point,num_points,trace,x,y,x1,y1;
  register int i;
  register float max;
  register float *p,*phasfl;
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
      return 1;
    }
  /* erase the screen behind the trace */
  color(BACK);
  amove(1,dfpnt2-1);
  box(mnumxpnts-(xoffset-mnumxpnts/80),dnpnt2+2);
  erase2 = 0;
  p = project_buffer;
  for (trace=fpnt1; trace < fpnt1+npnt1; trace++)
    { if ((phasfl=gettrace(trace,first_point))==0)
        { Wturnoff_buttons(); return 1; }
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
  InitVal(FIELD1,VERT, CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,2);
  InitVal(FIELD3,HORIZ,CR_NAME,    PARAM_COLOR,UNIT4,
                       CR_NAME,   -PARAM_COLOR,SCALED,2);
  InitVal(FIELD2,VERT, DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,2);
  InitVal(FIELD4,HORIZ,DELTA_NAME, PARAM_COLOR,UNIT4,
                       DELTA_NAME, PARAM_COLOR,SCALED,2);
  DispField1(FIELD5,PARAM_COLOR,"vsproj");
  DispField1(FIELD6,PARAM_COLOR,"vs2d");
  update_vs_pars();
  dconi_newcursor(1,x,y,0);
  Wsetgraphicsdisplay("dconi");
}

/**************************/
static dconi_get_new_vs(x,y)
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
        { Wturnoff_buttons(); return 1; }
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

/*****************/
static b_mark_dconi()
/*****************/
{
   dconi_mark(dconi_mode, TRUE, TRUE, NULL, NULL, NULL, NULL);
}

/*  DCONI_MARK is now a global subroutine which implements the 2D MARK
    function, either from the DCONI button or as part of the MARK command.	*/

dconi_mark(mark_mode, display_flag, button_flag, int_v_ptr, max_i_ptr,
           f1_max_ptr, f2_max_ptr)
int mark_mode;
int display_flag;
int button_flag;
float *int_v_ptr;
float *max_i_ptr;
float *f1_max_ptr;
float *f2_max_ptr;
{
	int		 x, y, x1, y1, first_point, first_trace, newfile,
                         num_entry, num_traces, num_points, trace;
	float		 maxcr, maxcr1, intval, tmpval;
	register float	*phasfl;
	register float	 datamax, sum;
	register int	 i, maxpoint, maxtrace;
        double           scale;
        int              first_ch,last_ch;
        int              first_direction;

/*  If this subroutine was called from the MARK command, initialize
    variables that are normally initialized when DCONI is started.	*/

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
        mark_textfptr=NULL;

	if (button_flag == TRUE)
	  Wsetgraphicsdisplay("dconi");
	return 0;
}

/*****************/
static dconi_plot()
/*****************/
{ Wgmode();
  if (init_dconi(2)) return 1;
  if (trace_mode==TRACE_MODE)
    { disp_status("PLOT  ");
      if (calc_ybars(phasfl,1,discalib*vsproj,dfpnt,dnpnt,
                        npnt,yoffset,next))
        return 1;
      plot_erase = 0;
      if (displayspec(dfpnt,dnpnt,0,&next,&plot_spec,&plot_erase,
                        mnumypnts,dfpnt2+dnpnt2+3,SPEC_COLOR))
        return 1;
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
static dconi_expand()
/*******************/
{ if (dconi_mode!=BOX_MODE)
    { dconi_full();
      if (expandMacro[0] != '\0')
        execString(expandMacro);
      return 0;
    }
/*  if (rev_h || rev_v)
    return 0;*/
  Wgmode();
  disp_status("EXPAND");
  Wturnoff_mouse();
  erase_projections();
  dconi_reset();
  /* set sp,wp,sp1,wp1 according to expansion box */
  if ( get_axis_freq(HORIZ) )
     sp  = cr - delta;
  else
     sp  = cr;
  wp  = delta;
  UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
  UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
  if ( get_axis_freq(VERT) )
     sp1  = cr1 - delta1;
  else
     sp1  = cr1;
  wp1 = delta1;
  UpdateVal(VERT,SP_NAME,sp1,NOSHOW);
  UpdateVal(VERT,WP_NAME,wp1,NOSHOW);
  run_dcon();
  dconi_mode = -1; /* force dconi_cursor() into the CURSOR_MODE */
  dconi_cursor();
  Wsetgraphicsdisplay("dconi");
  if (expandMacro[0] != '\0')
     execString(expandMacro);
  return 0;
}

/*****************/
static dconi_full()
/*****************/
{
/*  if (rev_h || rev_v)
    return 0;*/
  Wgmode();
  disp_status("FULL  ");
  Wturnoff_mouse();
  erase_projections();
  dconi_reset();
  /* set box size to last expansion */
  if ( get_axis_freq(HORIZ) )
     cr  = sp + delta;
  else
     cr = sp;
  if ( get_axis_freq(VERT) )
     cr1 = sp1 + delta1;
  else
     cr1 = sp1;
  delta  = fabs(wp);
  delta1 = fabs(wp1);
  /* set sp,wp,sp1,wp1 for full display */
  sp  = - rflrfp;
  wp  = sw;
  sp1 = - rflrfp1;
  wp1 = sw1;
  /* store the parameters */
  UpdateVal(HORIZ,SP_NAME,sp,NOSHOW);
  UpdateVal(HORIZ,WP_NAME,wp,NOSHOW);
  UpdateVal(HORIZ,CR_NAME,cr,NOSHOW);
  UpdateVal(VERT,SP_NAME,sp1,NOSHOW);
  UpdateVal(VERT,WP_NAME,wp1,NOSHOW);
  UpdateVal(VERT,CR_NAME,cr1,NOSHOW);
  run_dcon();
  dconi_mode=CURSOR_MODE;
  dconi_cursor();
  Wsetgraphicsdisplay("dconi");
}

/****************/
dconi_updatedcon()
/****************/
{ dconi_reset();
  P_setreal(CURRENT,"vs2d",vs2d,0);
  run_dcon();
  dconi_mode = -1;	/* force cursor mode */
  dconi_cursor();
  Wsetgraphicsdisplay("dconi");
}

/********************/
int turnoff_dconi()
/********************/
{ int r;
  setdisplay();
  Wgmode();
  Wturnoff_mouse();
  dconi_reset();
  EraseLabels();
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

/*********************************/
static dconi_multiply(x,y,moveflag)
/*********************************/
int x,y,moveflag;
{ float vs_mult();

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
       { dconi_get_new_vs(x,y);
         Wturnoff_mouse();
         dconi_mode = -1;
#ifdef SUN
/*
 *       Use the following line if no conformation is desired
 */
         dconi_updatedcon();
         if (expandMacro[0] != '\0')
         {
           execString(expandMacro);
         }
	 execString("menu\n");
#else
/*
 *       Use the following line if conformation is desired (VMS)
 */
/*         Wactivate_buttons(2,
           "Confirm Redraw",dconi_updatedcon,"Cancel Redraw",dconi_cursor,
           turnoff_dconi,"dconi");*/
#endif
       }
    }
  Wsetgraphicsdisplay("dconi");
  return 0;
}

/********************/
dconi_newdcon()
/********************/
{ Wgmode();
  Wturnoff_mouse();
  dconi_reset();
  run_dcon();
  dconi_mode = -1; /* force dconi_cursor() into the CURSOR_MODE */
  dconi_cursor();
  Wsetgraphicsdisplay("dconi");
}

/****************/
static hproj_max()
/****************/
{ dconi_hproj(0);
}

/****************/
static hproj_sum()
/****************/
{ dconi_hproj(1);
}

/****************/
static vproj_max()
/****************/
{ dconi_vproj(0);
}

/****************/
static vproj_sum()
/****************/
{ dconi_vproj(1);
}

/***************/
static run_dcon()
/***************/
{ extern int ll2d_draw_peaks();
  execString(dconi_runstring);
  if ((Wistek() || WisSunColor()) && leveldisplay_active && (dcon_num_colors>1))
    displaylevels();
  ll2d_draw_peaks();
  return 0;
}

/******************/
init_dconi(plotflag)
/******************/
int plotflag;
{
  double start,len,axis_scl;

  if (init2d(1,plotflag)) return 1;
  discalib = (float)(mnumypnts-ymin) / wc2max;
  yoffset = dfpnt2 + dnpnt2 + mnumypnts / 50;
  xoffset = mnumxpnts - dfpnt + 8*xcharpixels;
  get_scale_pars(HORIZ,&start,&len,&axis_scl,&rev_h);
  get_scale_pars(VERT,&start,&len,&axis_scl,&rev_v);
  return 0;
}

/*****************/
erase_projections()
/*****************/
{ color(BACK);
  amove(dfpnt-1,dfpnt2+dnpnt2+3);
  box(dnpnt+2,mnumypnts-dfpnt2-dnpnt2-5);
  erase1 = 0;
  if (erase2)
    { amove(1,dfpnt2-1);
      box(mnumxpnts-(xoffset-mnumxpnts/80),dnpnt2+2);
      erase2 = 0;
    }
}

/************************/
dconi(argc,argv,retc,retv)
/************************/
int argc,retc;
char *argv[],*retv[];
{ int update,x,y,x1,y1,argnum;
  int redisplay_param_flag, do_menu=FALSE;
  extern int ll2d_draw_peaks();

/* Check for special menu commands :
	toggle, trace, expand, plot	*/

  if (!((argc == 2) && (strcmp(argv[1],"plot") == 0)))  {
    Wturnoff_buttons();
    set_turnoff_routine(turnoff_dconi);
    }
  Wshow_graphics();
  disp_status("IN    ");
  if (init_dconi(1)) {skyrel(); return 1;}
  if (!d2flag)
    { Werrprintf("no 2D data in data file");
      return 1;
    }
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
  }
  if (argc == 2)  {
    if (strcmp(argv[1],"toggle") == 0)  {
      if (WgraphicsdisplayValid(argv[0]) && (argc>1))  {
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
    }
    
/*  Close `mark2d.out' if it happens to be open.  */

  if (mark_textfptr!=NULL) {
    fclose(mark_textfptr);
    mark_textfptr=NULL;
  }

/*  Redisplay Parameters?  */

  redisplay_param_flag = 0;
  if (argc > 1)
    if (strcmp( argv[ argc-1 ], "redisplay parameters" ) == 0) {
            argc--;

/*  autoRedisplay (in lexjunk.c) is supposed
    to insure the next test returns TRUE.	*/

	    if (WgraphicsdisplayValid( argv[ 0 ] ))
              redisplay_param_flag = 1;
    }

/*  For now, redisplay parameters in dconi is a formality, as the
    previous programming worked fine when the command was reexecuted.	*/

  skygo();

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
          (strcmp(argv[1],"dconi") == 0))
        update = 0;
      else if ((strcmp(argv[1],"cr")!=0) && (strcmp(argv[1],"delta")!=0) &&
          (strcmp(argv[1],"cr1")!=0) && (strcmp(argv[1],"delta1")!=0) &&
          (strcmp(argv[1],"cr2")!=0) && (strcmp(argv[1],"delta2")!=0) &&
          (strcmp(argv[1],"restart")!=0))
        {skyrel(); return 0;}
    }
  else if ((argc>1) && (strcmp(argv[1],"restart")==0))
    {
      update = (WgraphicsdisplayValid("dcon") ||
                WgraphicsdisplayValid("dpcon") ||
                WgraphicsdisplayValid("ds2d"));
       if (!update)
       { Werrprintf("invalid display for restart option");
         return 1;
       }
       else
       {
          leveldisplay_active = WgraphicsdisplayValid("dcon");
       }
    }

  if (!update)
    { if (!((argc==2) && (strcmp(argv[1],"again")==0)))
        {

/*  Program comes here if the argument count is not 2
    or if the first argument is not "again"		*/

          argnum = 2;

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
                ABORT;
              argnum = argc;            /* Since we want dconi to ignore the */
                                        /* rest of its arguments here insure */
                                        /* the program below where those     */
                                        /* arguments are appended to the     */
            }                           /* dconi runstring is skipped        */
          else if (argc < 2)
            {
              if (dconi_param_to_runstring() != 0)
                ABORT;
              argnum = argc;
            }
          else 
            { strcpy(dconi_runstring,"dcon");
              leveldisplay_active = 1;
              argnum = 1;
            }

    /* See note above, where we test for argv[ 1 ] == "dconi"  */

          if (argc>argnum)
            { strcat(dconi_runstring,"(");
              while (argc>argnum)
                { strcat(dconi_runstring,"'");
                  strcat(dconi_runstring,argv[argnum]);
                  strcat(dconi_runstring,"'");
                  argnum++;
                  if (argc>argnum)
                    strcat(dconi_runstring,",");
                }
              strcat(dconi_runstring,")");
            }
          strcat(dconi_runstring,"\n");
        }


/*  Check status returned by executing the "dconi_runstring"  */

      if (execString(dconi_runstring)) return 1;
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
  if (getxy(&x,&y,&x1,&y1)) {skyrel(); return 1;}   
  Wgmode();
  disp_status("      ");
  erase1 = 0;
  erase2 = 0;
  erase_projections();
  if ((Wistek() || WisSunColor()) && leveldisplay_active && (dcon_num_colors>1))
    displaylevels();
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
        { dconi_mode=CURSOR_MODE; 	/* force to switch to BOX_MODE */
          trace_mode=NOTRACE_MODE;
          dconi_cursor();
        }
      else if (trace_mode==TRACE_MODE)
        { dconi_trace();
        }
      else
        { dconi_mode = BOX_MODE;  	/* force to switch to CURSOR_MODE */
          trace_mode=NOTRACE_MODE;
          dconi_cursor();
        }
    }
  else
    { dconi_mode = BOX_MODE;	  	/* force to switch to CURSOR_MODE */
      trace_mode=NOTRACE_MODE;
      dconi_cursor();
    }
  releasevarlist();
  Wsetgraphicsdisplay("dconi");
  if (do_menu)
  {
     execString("menu('dconi')\n");
     if (leveldisplay_active) {
	strcpy(redraw_string, argv[0]);
    	if (argc > 1) {
	   strcat(redraw_string, "(");
	   argnum = 1;
           while (argnum < argc) {
           	strcat(redraw_string, "'");
           	strcat(redraw_string, argv[argnum]);
           	strcat(redraw_string, "'");
  	   	argnum++;
           	if (argnum < argc)
              	    strcat(redraw_string, ",");
           }
           strcat(redraw_string, ")");
	}
     }
     strcat(redraw_string,"\n");
  }
  set_turnoff_routine(turnoff_dconi);
  return 0;
}

/**************/
update_vs_pars()
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
displaylevels()
/*************/
{ int i,mx,ii,labelstep;
  char t[16];

  level_y0 = mnumypnts/6; 			/* hight above bottom */
  level_y1 = mnumypnts/(1.4 * dcon_num_colors);	/* hight of each block */
  level_x1 = mnumxpnts/80;			/* width of block */

  level_first = 0;
  labelstep = 1;
  if (dcon_colorflag==SELECT_PH_COLOR)
    level_first = - dcon_num_colors / 2;
  if (dcon_num_colors>20) labelstep = 10;

  Wgmode();
  /* erase area behind level display */
  color(BACK);
  amove(mnumxpnts-level_x1-2-2*xcharpixels,level_y0-1);
  box(level_x1+2*xcharpixels+1,dcon_num_colors*level_y1+2);
  /* draw the color bar */
  color(SCALE_COLOR);
  amove(mnumxpnts-level_x1-2,level_y0-1);
  rdraw(level_x1+1,0);
  rdraw(0,dcon_num_colors*level_y1+1);
  rdraw(-level_x1-1,0);
  rdraw(0,-dcon_num_colors*level_y1-1);
  if (Wissun())
    grf_batch(1);
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
  for (i=0; i<dcon_num_colors; i += labelstep)
    { sprintf(t,"%3d",i+level_first);
      amove(mnumxpnts-level_x1-2-3*xcharpixels,
                     level_y0+level_y1*i+(level_y1-ycharpixels)+2);
      dstring(t);
     }
  if (Wissun())
    grf_batch(0);
}


/********************/
static setcolorth(x,y)
/********************/
int x,y;
{ int newth;
  int pseudoColor;
  int i_mode;

  if (!leveldisplay_active)
    { Werrprintf("Level bar not active in this display");
      return 0;
    }
  if (!WisSunColor())
    { Werrprintf("Level bar not active in this display");
      return 0;
    }
  if (x>mnumxpnts-1) return 0;
  newth = ((y-level_y0) / level_y1);
  if ((newth<0) || (newth >= dcon_num_colors)) return 0; 
  
  pseudoColor = 1;
  newth += level_first;
  if (newth<0)
    newth = -newth;
  if (dcon_colorflag==SELECT_GRAY_COLOR)
    {  graycntr = (double) newth;
       if (graycntr > NUM_GRAY_COLORS) graycntr = NUM_GRAY_COLORS;  
       if (graycntr < 0.5) graycntr = 0.5;
       pseudoColor = change_contrast(graycntr,graysl);
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
      pseudoColor = setcolormap(dcon_first_color,dcon_num_colors,newth,
        dcon_colorflag==SELECT_PH_COLOR);
      th = (double) newth;
      P_setreal(CURRENT,"th",th,0);
    }
    if (!pseudoColor) {
        update_cursor(0,0,0);
        update_cursor(1,0,0);
	i_mode = dconi_mode;
        graph_batch(3);
        execString(redraw_string);
        graph_batch(-3);
	dconi_mode = i_mode;
        if (i_mode == BOX_MODE)
            dconi_mode = CURSOR_MODE;
        else if (i_mode == CURSOR_MODE)
            dconi_mode = BOX_MODE;
	else
            dconi_mode = -1;
        dconi_cursor();
    }
}

set_gray_slope(x,y)
int x,y;
{  int yy,xx;
   int newsl;
   newsl = ((y-level_y0) / level_y1);
   yy = newsl-(int) graycntr;
   if (yy < 0) return 0;
   if (yy == 0) graysl = NUM_GRAY_COLORS;
    else graysl = NUM_GRAY_COLORS/(2.0*yy);
   if (graysl < 0.5) graysl = 0.5;
   change_contrast(graycntr,graysl);
   if (gray_p_flag)  P_setreal(CURRENT,"graysl",graysl,0);
}
/******************************************/
int
setcolormap(firstcolor,numcolors,th,phcolor)
/******************************************/
int firstcolor,numcolors,th,phcolor;
{ int i, ret;
  ret = 1;
  if (!phcolor)
    { for (i=0; i<numcolors; i++)
        ret = change_color(firstcolor+i,i>th);
    }
  else
    { for (i=0; i<numcolors; i++)
        ret = change_color(firstcolor+i,
          (i<(numcolors>>1)-th) || (i>(numcolors>>1)+th));
    }
  refresh_graf();           /* some window systems needs refresh */
   return(ret);
}

/*  This program was adapted from "parse_acqproc_queue" in acqhwcmd.c
    Eventually the two could be combined into one program, with the
    delimiter character provided as an argument.			*/

static char *parse_dconi_value( dconi_addr, token_addr, token_len )
char *dconi_addr;
char *token_addr;
int token_len;
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
	  strcpy( &dconi_value[ 0 ], "dcon" );
	else if ((int)strlen( &dconi_value[ 0 ] ) < 1)
	  strcpy( &dconi_value[ 0 ], "dcon" );

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
